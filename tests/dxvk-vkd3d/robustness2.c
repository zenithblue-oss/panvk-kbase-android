#include "../features/vk_test.h"
#include "robustness2.spv.h"

struct resources {
   struct vkf_buffer results, uniform, storage, texels;
   VkBufferView uniform_texel, storage_texel;
   VkImage image;
   VkDeviceMemory image_memory;
   VkImageView image_view;
   VkSampler sampler;
};

static int
make_resources(struct vkf_ctx *c, struct resources *r)
{
   memset(r, 0, sizeof(*r));
   if (vkf_buffer_create(c, 68, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 0, &r->results) ||
       vkf_buffer_create(c, 512, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 0, &r->uniform) ||
       vkf_buffer_create(c, 32, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 0, &r->storage) ||
       vkf_buffer_create(c, 8, VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT |
                              VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT, 0, &r->texels))
      return 1;
   memset(vkf_map(c, &r->results), 0xff, 68); vkUnmapMemory(c->device, r->results.memory);
   memset(vkf_map(c, &r->uniform), 0x25, 512); vkUnmapMemory(c->device, r->uniform.memory);
   uint32_t *p = vkf_map(c, &r->storage); p[0] = 37; p[4] = 71;
   vkUnmapMemory(c->device, r->storage.memory);
   p = vkf_map(c, &r->texels); p[0] = 37; p[1] = 71;
   vkUnmapMemory(c->device, r->texels.memory);

   VkBufferViewCreateInfo bv = { .sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
      .buffer = r->texels.buffer, .format = VK_FORMAT_R32_UINT, .range = 4 };
   VKF_CHECK(vkCreateBufferView(c->device, &bv, NULL, &r->uniform_texel));
   VKF_CHECK(vkCreateBufferView(c->device, &bv, NULL, &r->storage_texel));

   VkImageCreateInfo ici = { .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D, .format = VK_FORMAT_R32G32B32A32_UINT,
      .extent = {1, 1, 1}, .mipLevels = 1, .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT, .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
               VK_IMAGE_USAGE_TRANSFER_DST_BIT, .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED };
   VKF_CHECK(vkCreateImage(c->device, &ici, NULL, &r->image));
   VkMemoryRequirements req; vkGetImageMemoryRequirements(c->device, r->image, &req);
   uint32_t mt = vkf_memory_type(c, req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
   if (mt == UINT32_MAX) mt = vkf_memory_type(c, req.memoryTypeBits, 0);
   VkMemoryAllocateInfo mai = { .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = req.size, .memoryTypeIndex = mt };
   VKF_CHECK(vkAllocateMemory(c->device, &mai, NULL, &r->image_memory));
   VKF_CHECK(vkBindImageMemory(c->device, r->image, r->image_memory, 0));
   VkImageViewCreateInfo iv = { .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = r->image, .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = VK_FORMAT_R32G32B32A32_UINT,
      .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1} };
   VKF_CHECK(vkCreateImageView(c->device, &iv, NULL, &r->image_view));
   VkSamplerCreateInfo sci = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .magFilter = VK_FILTER_NEAREST, .minFilter = VK_FILTER_NEAREST,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST };
   VKF_CHECK(vkCreateSampler(c->device, &sci, NULL, &r->sampler));

   VkCommandBuffer cmd = vkf_command(c);
   VkImageMemoryBarrier barrier = { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT, .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout = VK_IMAGE_LAYOUT_GENERAL, .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .image = r->image,
      .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1} };
   vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                        0, 0, NULL, 0, NULL, 1, &barrier);
   VkClearColorValue clear = {.uint32 = {37, 0, 0, 0}};
   vkCmdClearColorImage(cmd, r->image, VK_IMAGE_LAYOUT_GENERAL, &clear, 1, &barrier.subresourceRange);
   return vkf_submit(c, cmd);
}

static void
destroy_resources(struct vkf_ctx *c, struct resources *r)
{
   vkDestroySampler(c->device, r->sampler, NULL);
   vkDestroyImageView(c->device, r->image_view, NULL);
   vkDestroyImage(c->device, r->image, NULL);
   vkFreeMemory(c->device, r->image_memory, NULL);
   vkDestroyBufferView(c->device, r->storage_texel, NULL);
   vkDestroyBufferView(c->device, r->uniform_texel, NULL);
   vkf_buffer_destroy(c, &r->texels); vkf_buffer_destroy(c, &r->storage);
   vkf_buffer_destroy(c, &r->uniform); vkf_buffer_destroy(c, &r->results);
}

static int
run(struct vkf_ctx *c, struct resources *r)
{
   const VkDescriptorType types[] = {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
      VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_DESCRIPTOR_TYPE_SAMPLER,
      VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE};
   VkDescriptorSetLayoutBinding bindings[9];
   for (uint32_t i = 0; i < 9; i++)
      bindings[i] = (VkDescriptorSetLayoutBinding){i, types[i], i == 8 ? 2 : 1,
                                                   VK_SHADER_STAGE_COMPUTE_BIT, NULL};
   VkDescriptorBindingFlags flags[9] = {0};
   flags[5] = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
   VkDescriptorSetLayoutBindingFlagsCreateInfo fci = {
      .sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
      .bindingCount=9,.pBindingFlags=flags};
   VkDescriptorSetLayoutCreateInfo dlci = {.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pNext=&fci,.flags=VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
      .bindingCount=9,.pBindings=bindings};
   VkDescriptorSetLayout dsl; VKF_CHECK(vkCreateDescriptorSetLayout(c->device,&dlci,NULL,&dsl));
   VkDescriptorPoolSize sizes[] = {{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,2},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1},{VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,1},
      {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,1},{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,3},
      {VK_DESCRIPTOR_TYPE_SAMPLER,1},{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,1}};
   VkDescriptorPoolCreateInfo dpci = {.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .flags=VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,.maxSets=1,
      .poolSizeCount=7,.pPoolSizes=sizes};
   VkDescriptorPool pool; VKF_CHECK(vkCreateDescriptorPool(c->device,&dpci,NULL,&pool));
   VkDescriptorSetAllocateInfo ai = {.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool=pool,.descriptorSetCount=1,.pSetLayouts=&dsl};
   VkDescriptorSet set; VKF_CHECK(vkAllocateDescriptorSets(c->device,&ai,&set));

   VkDescriptorBufferInfo buffers[] = {{r->results.buffer,0,68},{r->uniform.buffer,0,16},
      {r->storage.buffer,0,16},{VK_NULL_HANDLE,0,VK_WHOLE_SIZE},{VK_NULL_HANDLE,0,VK_WHOLE_SIZE}};
   VkDescriptorImageInfo images[] = {{VK_NULL_HANDLE,r->image_view,VK_IMAGE_LAYOUT_GENERAL},
      {r->sampler,VK_NULL_HANDLE,VK_IMAGE_LAYOUT_UNDEFINED},
      {VK_NULL_HANDLE,r->image_view,VK_IMAGE_LAYOUT_GENERAL},
      {VK_NULL_HANDLE,VK_NULL_HANDLE,VK_IMAGE_LAYOUT_GENERAL},
      {VK_NULL_HANDLE,VK_NULL_HANDLE,VK_IMAGE_LAYOUT_GENERAL}};
   VkDescriptorImageInfo indexed_images[] = {
      {VK_NULL_HANDLE,r->image_view,VK_IMAGE_LAYOUT_GENERAL},
      {VK_NULL_HANDLE,VK_NULL_HANDLE,VK_IMAGE_LAYOUT_GENERAL}};
   VkBufferView views[] = {r->uniform_texel,r->storage_texel,VK_NULL_HANDLE,VK_NULL_HANDLE};
   VkWriteDescriptorSet writes[9] = {
      {.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,.dstSet=set,.dstBinding=0,.descriptorCount=1,.descriptorType=types[0],.pBufferInfo=&buffers[0]},
      {.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,.dstSet=set,.dstBinding=1,.descriptorCount=1,.descriptorType=types[1],.pBufferInfo=&buffers[1]},
      {.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,.dstSet=set,.dstBinding=2,.descriptorCount=1,.descriptorType=types[2],.pBufferInfo=&buffers[2]},
      {.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,.dstSet=set,.dstBinding=3,.descriptorCount=1,.descriptorType=types[3],.pTexelBufferView=&views[0]},
      {.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,.dstSet=set,.dstBinding=4,.descriptorCount=1,.descriptorType=types[4],.pTexelBufferView=&views[1]},
      {.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,.dstSet=set,.dstBinding=5,.descriptorCount=1,.descriptorType=types[5],.pImageInfo=&images[0]},
      {.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,.dstSet=set,.dstBinding=6,.descriptorCount=1,.descriptorType=types[6],.pImageInfo=&images[1]},
      {.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,.dstSet=set,.dstBinding=7,.descriptorCount=1,.descriptorType=types[7],.pImageInfo=&images[2]},
      {.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,.dstSet=set,.dstBinding=8,.descriptorCount=2,.descriptorType=types[8],.pImageInfo=indexed_images},
   };
   vkUpdateDescriptorSets(c->device,9,writes,0,NULL);

   VkPushConstantRange range = {VK_SHADER_STAGE_COMPUTE_BIT,0,8};
   VkPipelineLayoutCreateInfo lci = {.sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount=1,.pSetLayouts=&dsl,.pushConstantRangeCount=1,.pPushConstantRanges=&range};
   VkPipelineLayout layout; VKF_CHECK(vkCreatePipelineLayout(c->device,&lci,NULL,&layout));
   VkShaderModule module = vkf_shader(c,(const uint32_t *)robustness2_spv,sizeof(robustness2_spv));
   if (!module) return 1;
   VkComputePipelineCreateInfo pci = {.sType=VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      .stage={.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
              .stage=VK_SHADER_STAGE_COMPUTE_BIT,.module=module,.pName="main"},.layout=layout};
   VkPipeline pipeline; VKF_CHECK(vkCreateComputePipelines(c->device,VK_NULL_HANDLE,1,&pci,NULL,&pipeline));
   vkDestroyShaderModule(c->device,module,NULL);
   for (uint32_t test = 0; test < 14; test++) {
      if (test == 6) writes[5].pImageInfo = &images[3];
      if (test == 7) writes[7].pImageInfo = &images[3];
      if (test == 8) {
         writes[5].pImageInfo = &images[0];
         vkUpdateDescriptorSets(c->device,1,&writes[5],0,NULL);
         writes[6].pImageInfo = &images[3];
      }
      if (test == 9) writes[1].pBufferInfo = &buffers[3];
      if (test == 10) writes[2].pBufferInfo = &buffers[4];
      if (test == 11) writes[3].pTexelBufferView = &views[2];
      if (test == 12) writes[4].pTexelBufferView = &views[3];
      if (test >= 6 && test <= 12) vkUpdateDescriptorSets(c->device,1,&writes[test == 7 ? 7 : test == 8 ? 6 : test == 9 ? 1 : test == 10 ? 2 : test == 11 ? 3 : test == 12 ? 4 : 5],0,NULL);
      VkCommandBuffer cmd = vkf_command(c);
      vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_COMPUTE,pipeline);
      vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_COMPUTE,layout,0,1,&set,0,NULL);
      uint32_t push[] = {test, 1};
      vkCmdPushConstants(cmd,layout,VK_SHADER_STAGE_COMPUTE_BIT,0,8,push);
      vkCmdDispatch(cmd,1,1,1);
      if (vkf_submit(c,cmd)) return 1;
   }
   uint32_t test = 14;
   writes[5].pImageInfo = &images[0];
   vkUpdateDescriptorSets(c->device,1,&writes[5],0,NULL);
   VkCommandBuffer cmd = vkf_command(c);
   vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_COMPUTE,pipeline);
   vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_COMPUTE,layout,0,1,&set,0,NULL);
   uint32_t push[] = {test, 0};
   vkCmdPushConstants(cmd,layout,VK_SHADER_STAGE_COMPUTE_BIT,0,8,push);
   vkCmdDispatch(cmd,1,1,1);
   VKF_CHECK(vkEndCommandBuffer(cmd));
   writes[5].pImageInfo = &images[3];
   vkUpdateDescriptorSets(c->device,1,&writes[5],0,NULL);
   VkFence fence; VkFenceCreateInfo fi={.sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
   VKF_CHECK(vkCreateFence(c->device,&fi,NULL,&fence));
   VkSubmitInfo si={.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO,.commandBufferCount=1,.pCommandBuffers=&cmd};
   VKF_CHECK(vkQueueSubmit(c->queue,1,&si,fence));
   VKF_CHECK(vkWaitForFences(c->device,1,&fence,VK_TRUE,30000000000ULL));
   vkDestroyFence(c->device,fence,NULL);

   test = 15;
   cmd = vkf_command(c);
   vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_COMPUTE,pipeline);
   vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_COMPUTE,layout,0,1,&set,0,NULL);
   push[0] = test;
   vkCmdPushConstants(cmd,layout,VK_SHADER_STAGE_COMPUTE_BIT,0,8,push);
   vkCmdDispatch(cmd,1,1,1);
   if (vkf_submit(c,cmd)) return 1;

   static const char *names[] = {"oob_uniform_buffer_read","oob_storage_buffer_read",
      "oob_storage_buffer_write","oob_uniform_texel_buffer","oob_storage_texel_buffer",
      "oob_image_access","null_sampled_image","null_storage_image","null_sampler",
      "null_uniform_buffer","null_storage_buffer","null_uniform_texel_buffer",
      "null_storage_texel_buffer","descriptor_indexing_null","update_after_bind_null"};
   const uint32_t expected[] = {0,0,37,0,37,0,0,0,0,0,0,0,0,0,0};
   uint32_t *got = vkf_map(c,&r->results); int failed = 0;
   for (uint32_t i=0;i<15;i++) {
      if (i == 8) {
         printf("{\"case\":\"null_sampler\",\"status\":\"NOT_APPLICABLE\",\"actual\":%u,\"reason\":\"nullDescriptor does not permit null sampler handles\"}\n",got[i]);
         continue;
      }
      const char *status = got[i] == expected[i] ? "PASS" : "FAIL";
      printf("{\"case\":\"%s\",\"status\":\"%s\",\"expected\":%u,\"actual\":%u}\n",
             names[i],status,expected[i],got[i]);
      failed |= got[i] != expected[i];
   }
   if (got[15] != 37) {
      fprintf(stderr,"oob image write changed in-bounds texel: %u\n",got[15]);
      failed = 1;
   }
   if (got[16] != 1) { fprintf(stderr,"null storage write case did not execute\n"); failed = 1; }
   vkUnmapMemory(c->device,r->results.memory);
   uint32_t *p = vkf_map(c,&r->storage);
   printf("{\"case\":\"null_storage_buffer_write\",\"status\":\"%s\",\"expected\":37,\"actual\":%u}\n",
          p[0] == 37 ? "PASS" : "FAIL",p[0]);
   failed |= p[0] != 37 || p[4] != 71; vkUnmapMemory(c->device,r->storage.memory);
   p = vkf_map(c,&r->texels); failed |= p[1] != 71; vkUnmapMemory(c->device,r->texels.memory);
   vkDestroyPipeline(c->device,pipeline,NULL); vkDestroyPipelineLayout(c->device,layout,NULL);
   vkDestroyDescriptorPool(c->device,pool,NULL); vkDestroyDescriptorSetLayout(c->device,dsl,NULL);
   return failed;
}

int
main(int argc, char **argv)
{
   if (argc != 2) { fprintf(stderr,"usage: %s <ICD.so>\n",argv[0]); return 2; }
   VkPhysicalDeviceDescriptorIndexingFeatures indexing = {
      .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
      .shaderSampledImageArrayNonUniformIndexing=VK_TRUE,
      .descriptorBindingSampledImageUpdateAfterBind=VK_TRUE};
   VkPhysicalDeviceRobustness2FeaturesEXT robust = {
      .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT,.pNext=&indexing,
      .robustBufferAccess2=VK_TRUE,.robustImageAccess2=VK_FALSE,.nullDescriptor=VK_TRUE};
   VkPhysicalDeviceVulkan13Features v13 = {
      .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,.pNext=&robust,
      .robustImageAccess=VK_TRUE};
   const char *extensions[] = {VK_EXT_ROBUSTNESS_2_EXTENSION_NAME};
   struct vkf_ctx c; if (vkf_init(&c,argv[1],&v13,NULL,extensions,1)) return 1;
   struct resources r; if (make_resources(&c,&r)) return 1;
   int failed=run(&c,&r); destroy_resources(&c,&r); vkf_finish(&c); return failed;
}
