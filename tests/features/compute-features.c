#include "vk_test.h"

#include "descriptor.comp.spv.h"
#include "bda.comp.spv.h"
#include "push.comp.spv.h"
#include "robust.comp.spv.h"

static int
make_compute(struct vkf_ctx *c, const uint32_t *spv, size_t size,
             VkDescriptorSetLayout dsl, uint32_t push_size,
             VkPipelineLayout *layout, VkPipeline *pipeline)
{
   VkShaderModule module = vkf_shader(c, spv, size);
   if (!module) return 1;
   VkPushConstantRange range = { .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                                 .size = push_size };
   VkPipelineLayoutCreateInfo lci = { .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1, .pSetLayouts = &dsl, .pushConstantRangeCount = push_size != 0,
      .pPushConstantRanges = push_size ? &range : NULL };
   VKF_CHECK(vkCreatePipelineLayout(c->device, &lci, NULL, layout));
   VkComputePipelineCreateInfo pci = { .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      .stage = { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                 .stage = VK_SHADER_STAGE_COMPUTE_BIT, .module = module, .pName = "main" },
      .layout = *layout };
   VkResult r = vkCreateComputePipelines(c->device, VK_NULL_HANDLE, 1, &pci, NULL, pipeline);
   vkDestroyShaderModule(c->device, module, NULL);
   if (r != VK_SUCCESS) { fprintf(stderr, "compute pipeline result=%d\n", r); return 1; }
   return 0;
}

static int
run_bda(struct vkf_ctx *c)
{
   struct vkf_buffer input, output;
   if (vkf_buffer_create(c, 16, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
         VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT, &input) ||
       vkf_buffer_create(c, 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 0, &output)) return 1;
   uint32_t *p = vkf_map(c, &input);
   p[0] = 3; p[1] = 5; p[2] = 7; p[3] = 0x2468ace0;
   vkUnmapMemory(c->device, input.memory);
   p = vkf_map(c, &output); *p = 0; vkUnmapMemory(c->device, output.memory);
   VkDescriptorSetLayoutBinding binding = { .binding = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT };
   VkDescriptorSetLayoutCreateInfo dlci = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                                            .bindingCount = 1, .pBindings = &binding };
   VkDescriptorSetLayout dsl; VKF_CHECK(vkCreateDescriptorSetLayout(c->device, &dlci, NULL, &dsl));
   VkDescriptorPoolSize ps = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 };
   VkDescriptorPoolCreateInfo dpci = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = 1, .poolSizeCount = 1, .pPoolSizes = &ps };
   VkDescriptorPool pool; VKF_CHECK(vkCreateDescriptorPool(c->device, &dpci, NULL, &pool));
   VkDescriptorSetAllocateInfo ai = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = pool, .descriptorSetCount = 1, .pSetLayouts = &dsl };
   VkDescriptorSet set; VKF_CHECK(vkAllocateDescriptorSets(c->device, &ai, &set));
   VkDescriptorBufferInfo obi = { output.buffer, 0, 4 };
   VkWriteDescriptorSet write = { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = set, .dstBinding = 0, .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .pBufferInfo = &obi };
   vkUpdateDescriptorSets(c->device, 1, &write, 0, NULL);
   VkPipelineLayout layout; VkPipeline pipeline;
   if (make_compute(c, (const uint32_t *)bda_comp_spv, bda_comp_spv_len, dsl, 8,
                    &layout, &pipeline)) return 1;
   VkBufferDeviceAddressInfo info = { .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                                      .buffer = input.buffer };
   VkDeviceAddress address = vkGetBufferDeviceAddress(c->device, &info);
   VkCommandBuffer cmd = vkf_command(c);
   vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
   vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, 1, &set, 0, NULL);
   vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, 8, &address);
   vkCmdDispatch(cmd, 1, 1, 1);
   if (vkf_submit(c, cmd)) return 1;
   p = vkf_map(c, &output); uint32_t got = *p; vkUnmapMemory(c->device, output.memory);
   printf("{\"feature\":\"bufferDeviceAddress\",\"status\":\"%s\",\"address\":\"0x%llx\",\"readback\":\"0x%08x\"}\n",
          got == 0x2468ace0 ? "PASS" : "FAIL", (unsigned long long)address, got);
   vkDestroyPipeline(c->device, pipeline, NULL); vkDestroyPipelineLayout(c->device, layout, NULL);
   vkDestroyDescriptorPool(c->device, pool, NULL); vkDestroyDescriptorSetLayout(c->device, dsl, NULL);
   vkf_buffer_destroy(c, &input); vkf_buffer_destroy(c, &output);
   return got != 0x2468ace0;
}

static int
run_push_or_robust(struct vkf_ctx *c, int robust)
{
   struct vkf_buffer input, output;
   if (vkf_buffer_create(c, robust ? 4 : 8, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 0, &input) ||
       vkf_buffer_create(c, 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 0, &output)) return 1;
   uint32_t *p = vkf_map(c, &input); p[0] = robust ? 0xdeadbeef : 0x12345678; vkUnmapMemory(c->device, input.memory);
   p = vkf_map(c, &output); *p = 0xffffffff; vkUnmapMemory(c->device, output.memory);
   VkDescriptorSetLayoutBinding bindings[2] = {
      { .binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT },
      { .binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT }};
   VkDescriptorSetLayoutCreateInfo dlci = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .flags = robust ? 0 : VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR,
      .bindingCount = 2, .pBindings = bindings };
   VkDescriptorSetLayout dsl; VKF_CHECK(vkCreateDescriptorSetLayout(c->device, &dlci, NULL, &dsl));
   VkDescriptorPool pool = VK_NULL_HANDLE; VkDescriptorSet set = VK_NULL_HANDLE;
   if (robust) {
      VkDescriptorPoolSize ps = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2 };
      VkDescriptorPoolCreateInfo pi = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
         .maxSets = 1, .poolSizeCount = 1, .pPoolSizes = &ps };
      VKF_CHECK(vkCreateDescriptorPool(c->device, &pi, NULL, &pool));
      VkDescriptorSetAllocateInfo ai = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
         .descriptorPool = pool, .descriptorSetCount = 1, .pSetLayouts = &dsl };
      VKF_CHECK(vkAllocateDescriptorSets(c->device, &ai, &set));
   }
   VkDescriptorBufferInfo infos[2] = {{input.buffer, 0, input.size}, {output.buffer, 0, 4}};
   VkWriteDescriptorSet writes[2];
   for (uint32_t i = 0; i < 2; i++) writes[i] = (VkWriteDescriptorSet){
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = set, .dstBinding = i,
      .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .pBufferInfo = &infos[i] };
   if (robust) vkUpdateDescriptorSets(c->device, 2, writes, 0, NULL);
   VkPipelineLayout layout; VkPipeline pipeline;
   const uint32_t *code = (const uint32_t *)(robust ? robust_comp_spv : push_comp_spv);
   size_t size = robust ? robust_comp_spv_len : push_comp_spv_len;
   if (make_compute(c, code, size, dsl, 0, &layout, &pipeline)) return 1;
   VkCommandBuffer cmd = vkf_command(c);
   vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
   if (robust) vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, 1, &set, 0, NULL);
   else vkCmdPushDescriptorSetKHR(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, 2, writes);
   vkCmdDispatch(cmd, 1, 1, 1);
   if (vkf_submit(c, cmd)) return 1;
   p = vkf_map(c, &output); uint32_t got = *p; vkUnmapMemory(c->device, output.memory);
   uint32_t expected = robust ? 0 : (0x12345678 ^ 0xa5a5a5a5);
   printf("{\"feature\":\"%s\",\"status\":\"%s\",\"readback\":\"0x%08x\",\"expected\":\"0x%08x\"}\n",
          robust ? "robustBufferAccess" : "pushDescriptor", got == expected ? "PASS" : "FAIL", got, expected);
   vkDestroyPipeline(c->device, pipeline, NULL); vkDestroyPipelineLayout(c->device, layout, NULL);
   if (pool) vkDestroyDescriptorPool(c->device, pool, NULL);
   vkDestroyDescriptorSetLayout(c->device, dsl, NULL);
   vkf_buffer_destroy(c, &input); vkf_buffer_destroy(c, &output);
   return got != expected;
}

static int
run_descriptor(struct vkf_ctx *c)
{
   struct vkf_buffer output, inputs[4], staging;
   if (vkf_buffer_create(c, 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 0, &output) ||
       vkf_buffer_create(c, 4, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 0, &staging)) return 1;
   uint32_t *p = vkf_map(c, &output); *p = 0; vkUnmapMemory(c->device, output.memory);
   uint8_t *pixel = vkf_map(c, &staging); pixel[0] = 23; pixel[1] = pixel[2] = 0; pixel[3] = 255;
   vkUnmapMemory(c->device, staging.memory);
   for (uint32_t i = 0; i < 4; i++) {
      if (vkf_buffer_create(c, 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 0, &inputs[i])) return 1;
      p = vkf_map(c, &inputs[i]); *p = 98 + i; vkUnmapMemory(c->device, inputs[i].memory);
   }
   VkImageCreateInfo ici = { .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, .imageType = VK_IMAGE_TYPE_2D,
      .format = VK_FORMAT_R8G8B8A8_UNORM, .extent = {1,1,1}, .mipLevels = 1, .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT, .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE };
   VkImage image; VKF_CHECK(vkCreateImage(c->device, &ici, NULL, &image));
   VkMemoryRequirements req; vkGetImageMemoryRequirements(c->device, image, &req);
   uint32_t mt = vkf_memory_type(c, req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
   if (mt == UINT32_MAX) mt = vkf_memory_type(c, req.memoryTypeBits, 0);
   VkMemoryAllocateInfo mai = { .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = req.size, .memoryTypeIndex = mt };
   VkDeviceMemory imem; VKF_CHECK(vkAllocateMemory(c->device, &mai, NULL, &imem));
   VKF_CHECK(vkBindImageMemory(c->device, image, imem, 0));
   VkImageViewCreateInfo vci = { .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, .image = image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D, .format = VK_FORMAT_R8G8B8A8_UNORM,
      .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1} };
   VkImageView view; VKF_CHECK(vkCreateImageView(c->device, &vci, NULL, &view));
   VkSamplerCreateInfo sci = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .magFilter = VK_FILTER_NEAREST, .minFilter = VK_FILTER_NEAREST,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST, .maxLod = 1 };
   VkSampler sampler; VKF_CHECK(vkCreateSampler(c->device, &sci, NULL, &sampler));
   VkDescriptorSetLayoutBinding bindings[3] = {
      {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL},
      {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4, VK_SHADER_STAGE_COMPUTE_BIT, NULL},
      {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4, VK_SHADER_STAGE_COMPUTE_BIT, NULL}};
   VkDescriptorBindingFlags flags[3] = {0, VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
      VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT};
   VkDescriptorSetLayoutBindingFlagsCreateInfo fci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
      .bindingCount = 3, .pBindingFlags = flags };
   VkDescriptorSetLayoutCreateInfo dlci = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pNext = &fci, .bindingCount = 3, .pBindings = bindings };
   VkDescriptorSetLayout dsl; VKF_CHECK(vkCreateDescriptorSetLayout(c->device, &dlci, NULL, &dsl));
   VkDescriptorPoolSize sizes[2] = {{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 5},
                                    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4}};
   VkDescriptorPoolCreateInfo dpci = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = 1, .poolSizeCount = 2, .pPoolSizes = sizes };
   VkDescriptorPool pool; VKF_CHECK(vkCreateDescriptorPool(c->device, &dpci, NULL, &pool));
   uint32_t variable_count = 4;
   VkDescriptorSetVariableDescriptorCountAllocateInfo vai = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO,
      .descriptorSetCount = 1, .pDescriptorCounts = &variable_count };
   VkDescriptorSetAllocateInfo dsai = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .pNext = &vai, .descriptorPool = pool, .descriptorSetCount = 1, .pSetLayouts = &dsl };
   VkDescriptorSet set; VKF_CHECK(vkAllocateDescriptorSets(c->device, &dsai, &set));
   VkDescriptorBufferInfo obi = {output.buffer, 0, 4}, ibi = {inputs[2].buffer, 0, 4};
   VkDescriptorImageInfo iii = {sampler, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
   VkWriteDescriptorSet writes[3] = {
      {.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,.dstSet=set,.dstBinding=0,.descriptorCount=1,
       .descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,.pBufferInfo=&obi},
      {.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,.dstSet=set,.dstBinding=1,.dstArrayElement=2,.descriptorCount=1,
       .descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,.pBufferInfo=&ibi},
      {.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,.dstSet=set,.dstBinding=2,.dstArrayElement=3,.descriptorCount=1,
       .descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,.pImageInfo=&iii}};
   vkUpdateDescriptorSets(c->device, 3, writes, 0, NULL);
   VkPipelineLayout layout; VkPipeline pipeline;
   if (make_compute(c, (const uint32_t *)descriptor_comp_spv, descriptor_comp_spv_len,
                    dsl, 0, &layout, &pipeline)) return 1;
   VkCommandBuffer cmd = vkf_command(c);
   VkImageMemoryBarrier to_dst = { .sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT,.oldLayout=VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.image=image,
      .subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1}};
   vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,NULL,0,NULL,1,&to_dst);
   VkBufferImageCopy copy={.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1},.imageExtent={1,1,1}};
   vkCmdCopyBufferToImage(cmd,staging.buffer,image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);
   VkImageMemoryBarrier to_read=to_dst; to_read.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
   to_read.dstAccessMask=VK_ACCESS_SHADER_READ_BIT; to_read.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
   to_read.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
   vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,0,NULL,0,NULL,1,&to_read);
   vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_COMPUTE,pipeline);
   vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_COMPUTE,layout,0,1,&set,0,NULL);
   vkCmdDispatch(cmd,1,1,1);
   if (vkf_submit(c,cmd)) return 1;
   p=vkf_map(c,&output); uint32_t got=*p; vkUnmapMemory(c->device,output.memory);
   printf("{\"feature\":\"descriptorIndexing\",\"status\":\"%s\",\"runtimeArray\":true,\"partiallyBound\":true,\"variableCount\":4,\"sampledNonUniformIndex\":3,\"storageNonUniformIndex\":2,\"checksum\":%u}\n",got==123?"PASS":"FAIL",got);
   vkDestroyPipeline(c->device,pipeline,NULL); vkDestroyPipelineLayout(c->device,layout,NULL);
   vkDestroyDescriptorPool(c->device,pool,NULL); vkDestroyDescriptorSetLayout(c->device,dsl,NULL);
   vkDestroySampler(c->device,sampler,NULL); vkDestroyImageView(c->device,view,NULL);
   vkDestroyImage(c->device,image,NULL); vkFreeMemory(c->device,imem,NULL);
   vkf_buffer_destroy(c,&staging); for(uint32_t i=0;i<4;i++) vkf_buffer_destroy(c,&inputs[i]);
   vkf_buffer_destroy(c,&output); return got!=123;
}

int main(int argc, char **argv)
{
   if (argc != 2) { fprintf(stderr,"usage: %s <ICD.so>\n",argv[0]); return 2; }
   VkPhysicalDeviceDescriptorIndexingFeatures indexing = {
      .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
      .shaderSampledImageArrayNonUniformIndexing=VK_TRUE,
      .shaderStorageBufferArrayNonUniformIndexing=VK_TRUE,.runtimeDescriptorArray=VK_TRUE,
      .descriptorBindingPartiallyBound=VK_TRUE,.descriptorBindingVariableDescriptorCount=VK_TRUE};
   VkPhysicalDeviceBufferDeviceAddressFeatures bda = {
      .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,.pNext=&indexing,
      .bufferDeviceAddress=VK_TRUE};
   VkPhysicalDeviceFeatures core = {.robustBufferAccess=VK_TRUE};
   const char *exts[]={VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME};
   struct vkf_ctx c; if(vkf_init(&c,argv[1],&bda,&core,exts,1)) return 1;
   int fail=run_descriptor(&c)|run_bda(&c)|run_push_or_robust(&c,0)|run_push_or_robust(&c,1);
   vkf_finish(&c); return fail;
}
