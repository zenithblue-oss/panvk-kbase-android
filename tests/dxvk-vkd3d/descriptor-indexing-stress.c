#include "../features/vk_test.h"

#include "kind-0.spv.h"
#include "kind-1.spv.h"
#include "kind-2.spv.h"
#include "kind-3.spv.h"
#include "kind-4.spv.h"
#include "kind-5.spv.h"

enum descriptor_kind {
   SAMPLED_IMAGE,
   STORAGE_IMAGE,
   STORAGE_BUFFER,
   SAMPLER,
   UNIFORM_TEXEL_BUFFER,
   STORAGE_TEXEL_BUFFER,
   KIND_COUNT,
};

static const char *kind_names[KIND_COUNT] = {
   "sampled_image", "storage_image", "storage_buffer", "sampler",
   "uniform_texel_buffer", "storage_texel_buffer",
};

static const VkDescriptorType kind_types[KIND_COUNT] = {
   VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
   VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_SAMPLER,
   VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
};

static const unsigned char *kind_spv[KIND_COUNT] = {
   descriptor_kind_0_spv, descriptor_kind_1_spv, descriptor_kind_2_spv,
   descriptor_kind_3_spv, descriptor_kind_4_spv, descriptor_kind_5_spv,
};

static const size_t kind_spv_size[KIND_COUNT] = {
   sizeof(descriptor_kind_0_spv), sizeof(descriptor_kind_1_spv),
   sizeof(descriptor_kind_2_spv), sizeof(descriptor_kind_3_spv),
   sizeof(descriptor_kind_4_spv), sizeof(descriptor_kind_5_spv),
};

struct resources {
   struct vkf_buffer result;
   struct vkf_buffer value;
   VkImage image;
   VkDeviceMemory image_memory;
   VkImageView image_view;
   VkSampler sampler;
   VkBufferView buffer_view;
};

static int
make_resources(struct vkf_ctx *c, struct resources *r)
{
   memset(r, 0, sizeof(*r));
   if (vkf_buffer_create(c, 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 0, &r->result) ||
       vkf_buffer_create(c, 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                            VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT |
                            VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT, 0, &r->value))
      return 1;
   uint32_t *p = vkf_map(c, &r->result); *p = 0; vkUnmapMemory(c->device, r->result.memory);
   p = vkf_map(c, &r->value); *p = 37; vkUnmapMemory(c->device, r->value.memory);

   VkBufferViewCreateInfo bvci = { .sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
      .buffer = r->value.buffer, .format = VK_FORMAT_R32_UINT, .range = 4 };
   VKF_CHECK(vkCreateBufferView(c->device, &bvci, NULL, &r->buffer_view));
   VkImageCreateInfo ici = { .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D, .format = VK_FORMAT_R8G8B8A8_UNORM,
      .extent = {1, 1, 1}, .mipLevels = 1, .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT, .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
               VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED };
   VKF_CHECK(vkCreateImage(c->device, &ici, NULL, &r->image));
   VkMemoryRequirements req; vkGetImageMemoryRequirements(c->device, r->image, &req);
   uint32_t mt = vkf_memory_type(c, req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
   if (mt == UINT32_MAX) mt = vkf_memory_type(c, req.memoryTypeBits, 0);
   VkMemoryAllocateInfo mai = { .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = req.size, .memoryTypeIndex = mt };
   VKF_CHECK(vkAllocateMemory(c->device, &mai, NULL, &r->image_memory));
   VKF_CHECK(vkBindImageMemory(c->device, r->image, r->image_memory, 0));
   VkImageViewCreateInfo ivci = { .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = r->image, .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = VK_FORMAT_R8G8B8A8_UNORM,
      .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1} };
   VKF_CHECK(vkCreateImageView(c->device, &ivci, NULL, &r->image_view));
   VkSamplerCreateInfo sci = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .magFilter = VK_FILTER_NEAREST, .minFilter = VK_FILTER_NEAREST,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST };
   VKF_CHECK(vkCreateSampler(c->device, &sci, NULL, &r->sampler));
   VkCommandBuffer cmd = vkf_command(c);
   VkImageMemoryBarrier barrier = { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED, .newLayout = VK_IMAGE_LAYOUT_GENERAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = r->image, .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1} };
   vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                        0, 0, NULL, 0, NULL, 1, &barrier);
   VkClearColorValue clear = {.float32 = {37.0f / 255.0f, 0.0f, 0.0f, 1.0f}};
   vkCmdClearColorImage(cmd, r->image, VK_IMAGE_LAYOUT_GENERAL, &clear, 1, &barrier.subresourceRange);
   if (vkf_submit(c, cmd)) return 1;
   return 0;
}

static void
destroy_resources(struct vkf_ctx *c, struct resources *r)
{
   vkDestroySampler(c->device, r->sampler, NULL);
   vkDestroyImageView(c->device, r->image_view, NULL);
   vkDestroyImage(c->device, r->image, NULL);
   vkFreeMemory(c->device, r->image_memory, NULL);
   vkDestroyBufferView(c->device, r->buffer_view, NULL);
   vkf_buffer_destroy(c, &r->value);
   vkf_buffer_destroy(c, &r->result);
}

static int
make_pipeline(struct vkf_ctx *c, enum descriptor_kind kind, VkDescriptorSetLayout dsl,
              VkPipelineLayout *layout, VkPipeline *pipeline)
{
   VkPushConstantRange range = { VK_SHADER_STAGE_COMPUTE_BIT, 0, 4 };
   VkPipelineLayoutCreateInfo lci = { .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1, .pSetLayouts = &dsl, .pushConstantRangeCount = 1,
      .pPushConstantRanges = &range };
   VKF_CHECK(vkCreatePipelineLayout(c->device, &lci, NULL, layout));
   VkShaderModule module = vkf_shader(c, (const uint32_t *)kind_spv[kind], kind_spv_size[kind]);
   if (!module) return 1;
   VkComputePipelineCreateInfo pci = { .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      .stage = { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
         .stage = VK_SHADER_STAGE_COMPUTE_BIT, .module = module, .pName = "main" },
      .layout = *layout };
   VkResult result = vkCreateComputePipelines(c->device, VK_NULL_HANDLE, 1, &pci, NULL, pipeline);
   vkDestroyShaderModule(c->device, module, NULL);
   if (result != VK_SUCCESS) fprintf(stderr, "pipeline kind=%s result=%d\n", kind_names[kind], result);
   return result != VK_SUCCESS;
}

static void
write_descriptor(struct vkf_ctx *c, VkDescriptorSet set, enum descriptor_kind kind,
                 uint32_t binding, uint32_t element, uint32_t count, struct resources *r)
{
   VkDescriptorBufferInfo bi = { r->value.buffer, 0, 4 };
   VkDescriptorImageInfo ii = { r->sampler, r->image_view, VK_IMAGE_LAYOUT_GENERAL };
   VkWriteDescriptorSet write = { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = set, .dstBinding = binding, .dstArrayElement = element,
      .descriptorCount = count, .descriptorType = kind_types[kind] };
   VkDescriptorBufferInfo *bis = NULL;
   VkDescriptorImageInfo *iis = NULL;
   VkBufferView *views = NULL;
   if (kind == STORAGE_BUFFER) {
      bis = malloc((size_t)count * sizeof(*bis));
      for (uint32_t i = 0; i < count; i++) bis[i] = bi;
      write.pBufferInfo = bis;
   } else if (kind <= SAMPLER) {
      iis = malloc((size_t)count * sizeof(*iis));
      for (uint32_t i = 0; i < count; i++) iis[i] = ii;
      write.pImageInfo = iis;
   } else {
      views = malloc((size_t)count * sizeof(*views));
      for (uint32_t i = 0; i < count; i++) views[i] = r->buffer_view;
      write.pTexelBufferView = views;
   }
   vkUpdateDescriptorSets(c->device, 1, &write, 0, NULL);
   free(views); free(iis); free(bis);
}

static int
run_case(struct vkf_ctx *c, enum descriptor_kind kind, uint32_t count,
         struct resources *r)
{
   uint32_t *result_value = vkf_map(c, &r->result);
   *result_value = 0;
   vkUnmapMemory(c->device, r->result.memory);
   VkDescriptorSetLayoutBinding bindings[3] = {
      {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL},
      {1, kind_types[kind], count, VK_SHADER_STAGE_COMPUTE_BIT, NULL},
      {2, kind == SAMPLER ? VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE : VK_DESCRIPTOR_TYPE_SAMPLER,
       1, VK_SHADER_STAGE_COMPUTE_BIT, NULL},
   };
   uint32_t binding_count = (kind == SAMPLED_IMAGE || kind == SAMPLER) ? 3 : 2;
   VkDescriptorBindingFlags flags[3] = {0,
      VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
      VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT |
      VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
      VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT, 0};
   /* Vulkan requires the variable-count binding to be numerically highest. */
   if (binding_count == 3) {
      bindings[1].binding = 2; bindings[2].binding = 1;
   }
   VkDescriptorSetLayoutBindingFlagsCreateInfo fci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
      .bindingCount = binding_count, .pBindingFlags = flags };
   VkDescriptorSetLayoutCreateInfo dlci = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pNext = &fci, .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
      .bindingCount = binding_count, .pBindings = bindings };
   VkDescriptorSetLayout dsl; VKF_CHECK(vkCreateDescriptorSetLayout(c->device, &dlci, NULL, &dsl));
   VkDescriptorPoolSize sizes[3] = {{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}, {kind_types[kind], count},
      {bindings[2].descriptorType, binding_count == 3}};
   uint32_t pool_size_count = binding_count;
   if (kind == STORAGE_BUFFER) {
      sizes[0].descriptorCount += count;
      pool_size_count = 1;
   }
   VkDescriptorPoolCreateInfo dpci = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
      .maxSets = 1, .poolSizeCount = pool_size_count, .pPoolSizes = sizes };
   VkDescriptorPool pool; VKF_CHECK(vkCreateDescriptorPool(c->device, &dpci, NULL, &pool));
   uint32_t variable_count = count;
   VkDescriptorSetVariableDescriptorCountAllocateInfo vai = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO,
      .descriptorSetCount = 1, .pDescriptorCounts = &variable_count };
   VkDescriptorSetAllocateInfo ai = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .pNext = &vai, .descriptorPool = pool, .descriptorSetCount = 1, .pSetLayouts = &dsl };
   VkDescriptorSet set; VKF_CHECK(vkAllocateDescriptorSets(c->device, &ai, &set));
   VkDescriptorSet exhausted;
   VkResult exhaustion = vkAllocateDescriptorSets(c->device, &ai, &exhausted);
   if (exhaustion != VK_ERROR_OUT_OF_POOL_MEMORY && exhaustion != VK_ERROR_FRAGMENTED_POOL) {
      fprintf(stderr, "pool exhaustion kind=%s count=%u result=%d\n", kind_names[kind], count, exhaustion);
      return 1;
   }
   VkDescriptorBufferInfo result_info = {r->result.buffer, 0, 4};
   VkDescriptorImageInfo fixed = {r->sampler, r->image_view, VK_IMAGE_LAYOUT_GENERAL};
   VkWriteDescriptorSet base[2] = {
      {.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,.dstSet=set,.dstBinding=0,.descriptorCount=1,
       .descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,.pBufferInfo=&result_info},
      {.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,.dstSet=set,.dstBinding=1,.descriptorCount=1,
       .descriptorType=bindings[2].descriptorType,.pImageInfo=&fixed},
   };
   vkUpdateDescriptorSets(c->device, binding_count == 3 ? 2 : 1, base, 0, NULL);
   uint32_t variable_binding = binding_count == 3 ? 2 : 1;
   write_descriptor(c, set, kind, variable_binding, 0, 1, r);
   VkPipelineLayout layout; VkPipeline pipeline;
   if (make_pipeline(c, kind, dsl, &layout, &pipeline)) return 1;
   VkCommandBuffer cmd = vkf_command(c);
   VkImageMemoryBarrier image_barrier = { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
      .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_GENERAL, .newLayout = VK_IMAGE_LAYOUT_GENERAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = r->image, .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1} };
   vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                        0, 0, NULL, 0, NULL, 1, &image_barrier);
   vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
   vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, 1, &set, 0, NULL);
   /* UpdateAfterBind: populate the descriptor selected by the submitted workload after binding. */
   write_descriptor(c, set, kind, variable_binding, count - 1, 1, r);
   uint32_t last = count - 1;
   vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, 4, &last);
   vkCmdDispatch(cmd, 1, 1, 1);
   if (vkf_submit(c, cmd)) return 1;
   uint32_t *p = vkf_map(c, &r->result); uint32_t got = *p; vkUnmapMemory(c->device, r->result.memory);
   if (got != 74) { fprintf(stderr, "readback kind=%s count=%u got=%u\n", kind_names[kind], count, got); return 1; }

   /* Keep the set pending at an event, then update an unreferenced descriptor. */
   VkEvent event; VkEventCreateInfo eci = { .sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO };
   VKF_CHECK(vkCreateEvent(c->device, &eci, NULL, &event));
   cmd = vkf_command(c);
   vkCmdWaitEvents(cmd, 1, &event, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                   0, NULL, 0, NULL, 0, NULL);
   vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
   vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, 1, &set, 0, NULL);
   uint32_t zero = 0; vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, 4, &zero);
   vkCmdDispatch(cmd, 1, 1, 1); VKF_CHECK(vkEndCommandBuffer(cmd));
   VkFence fence; VkFenceCreateInfo ff = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
   VKF_CHECK(vkCreateFence(c->device, &ff, NULL, &fence));
   VkSubmitInfo si = { .sType=VK_STRUCTURE_TYPE_SUBMIT_INFO,.commandBufferCount=1,.pCommandBuffers=&cmd };
   VKF_CHECK(vkQueueSubmit(c->queue, 1, &si, fence));
   write_descriptor(c, set, kind, variable_binding, count - 1, 1, r);
   VKF_CHECK(vkSetEvent(c->device, event));
   VKF_CHECK(vkWaitForFences(c->device, 1, &fence, VK_TRUE, 30000000000ULL));
   vkDestroyFence(c->device, fence, NULL); vkDestroyEvent(c->device, event, NULL);

   VKF_CHECK(vkFreeDescriptorSets(c->device, pool, 1, &set));
   VKF_CHECK(vkAllocateDescriptorSets(c->device, &ai, &set));
   VKF_CHECK(vkResetDescriptorPool(c->device, pool, 0));
   VKF_CHECK(vkAllocateDescriptorSets(c->device, &ai, &set));
   vkDestroyDescriptorPool(c->device, pool, NULL);
   VKF_CHECK(vkCreateDescriptorPool(c->device, &dpci, NULL, &pool));
   ai.descriptorPool = pool; VKF_CHECK(vkAllocateDescriptorSets(c->device, &ai, &set));
   printf("{\"type\":\"%s\",\"count\":%u,\"status\":\"PASS\",\"updateAfterBind\":true,\"variableCount\":true,\"partiallyBound\":true,\"unusedWhilePending\":true,\"runtimeArray\":true,\"nonUniformIndex\":%u,\"poolExhaustion\":true,\"freeResetRecreate\":true}\n",
          kind_names[kind], count, count - 1);
   vkDestroyDescriptorPool(c->device, pool, NULL);
   vkDestroyPipeline(c->device, pipeline, NULL); vkDestroyPipelineLayout(c->device, layout, NULL);
   vkDestroyDescriptorSetLayout(c->device, dsl, NULL);
   return 0;
}

int
main(int argc, char **argv)
{
   if (argc != 2) { fprintf(stderr, "usage: %s <ICD.so>\n", argv[0]); return 2; }
   VkPhysicalDeviceDescriptorIndexingFeatures indexing = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
      .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
      .shaderStorageBufferArrayNonUniformIndexing = VK_TRUE,
      .shaderStorageImageArrayNonUniformIndexing = VK_TRUE,
      .shaderUniformTexelBufferArrayNonUniformIndexing = VK_TRUE,
      .shaderStorageTexelBufferArrayNonUniformIndexing = VK_TRUE,
      .descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
      .descriptorBindingStorageImageUpdateAfterBind = VK_TRUE,
      .descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE,
      .descriptorBindingUniformTexelBufferUpdateAfterBind = VK_TRUE,
      .descriptorBindingStorageTexelBufferUpdateAfterBind = VK_TRUE,
      .descriptorBindingUpdateUnusedWhilePending = VK_TRUE,
      .descriptorBindingPartiallyBound = VK_TRUE,
      .descriptorBindingVariableDescriptorCount = VK_TRUE,
      .runtimeDescriptorArray = VK_TRUE,
   };
   struct vkf_ctx c; if (vkf_init(&c, argv[1], &indexing, NULL, NULL, 0)) return 1;
   struct resources r; if (make_resources(&c, &r)) return 1;
   const uint32_t counts[] = {1000, 10000, 100000, 1000000};
   int failed = 0;
   for (uint32_t kind = 0; kind < KIND_COUNT && !failed; kind++)
      for (uint32_t i = 0; i < sizeof(counts) / sizeof(counts[0]) && !failed; i++)
         failed = run_case(&c, kind, counts[i], &r);
   destroy_resources(&c, &r); vkf_finish(&c);
   return failed;
}
