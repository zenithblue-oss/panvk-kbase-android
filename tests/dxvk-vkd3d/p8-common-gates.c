#include "../features/vk_test.h"
#include "p8-int.comp.spv.h"
#include "p8-sampler.comp.spv.h"
#include "p8.vert.spv.h"
#include "p8.frag.spv.h"

struct resources {
   struct vkf_buffer out, b8, b16, upload;
   VkImage sampled, color;
   VkDeviceMemory sampled_mem, color_mem;
   VkImageView sampled_view, color_view;
   VkSampler sampler;
};

static int
has_extension(struct vkf_ctx *c, const char *name)
{
   uint32_t count = 0;
   VKF_CHECK(vkEnumerateDeviceExtensionProperties(c->physical, NULL, &count, NULL));
   VkExtensionProperties *props = calloc(count, sizeof(*props));
   VKF_CHECK(vkEnumerateDeviceExtensionProperties(c->physical, NULL, &count, props));
   int found = 0;
   for (uint32_t i = 0; i < count; i++)
      found |= !strcmp(props[i].extensionName, name);
   free(props);
   return found;
}

static int
make_image(struct vkf_ctx *c, uint32_t width, VkImageUsageFlags usage,
           VkImage *image, VkDeviceMemory *memory, VkImageView *view)
{
   VkImageCreateInfo ici = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D, .format = VK_FORMAT_R8G8B8A8_UNORM,
      .extent = {width, 1, 1}, .mipLevels = 1, .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT, .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = usage, .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
   };
   VKF_CHECK(vkCreateImage(c->device, &ici, NULL, image));
   VkMemoryRequirements req;
   vkGetImageMemoryRequirements(c->device, *image, &req);
   uint32_t mt = vkf_memory_type(c, req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
   if (mt == UINT32_MAX) mt = vkf_memory_type(c, req.memoryTypeBits, 0);
   VkMemoryAllocateInfo mai = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = req.size, .memoryTypeIndex = mt};
   VKF_CHECK(vkAllocateMemory(c->device, &mai, NULL, memory));
   VKF_CHECK(vkBindImageMemory(c->device, *image, *memory, 0));
   VkImageViewCreateInfo ivci = {.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = *image, .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = VK_FORMAT_R8G8B8A8_UNORM,
      .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};
   VKF_CHECK(vkCreateImageView(c->device, &ivci, NULL, view));
   return 0;
}

static int
make_resources(struct vkf_ctx *c, struct resources *r)
{
   memset(r, 0, sizeof(*r));
   if (vkf_buffer_create(c, 16, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 0, &r->out) ||
       vkf_buffer_create(c, 4, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 0, &r->b8) ||
       vkf_buffer_create(c, 8, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 0, &r->b16) ||
       vkf_buffer_create(c, 8, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 0, &r->upload) ||
       make_image(c, 2, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                  &r->sampled, &r->sampled_mem, &r->sampled_view) ||
       make_image(c, 1, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                  &r->color, &r->color_mem, &r->color_view))
      return 1;
   memset(vkf_map(c, &r->out), 0, 16); vkUnmapMemory(c->device, r->out.memory);
   int8_t *p8 = vkf_map(c, &r->b8); p8[0] = 7; p8[1] = 0;
   vkUnmapMemory(c->device, r->b8.memory);
   int16_t *p16 = vkf_map(c, &r->b16); p16[0] = 300; p16[1] = 0;
   vkUnmapMemory(c->device, r->b16.memory);
   uint32_t *pixels = vkf_map(c, &r->upload);
   pixels[0] = 0xff00ff00u; pixels[1] = 0xff0000ffu;
   vkUnmapMemory(c->device, r->upload.memory);
   VkSamplerCreateInfo sci = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .magFilter = VK_FILTER_NEAREST, .minFilter = VK_FILTER_NEAREST,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
      .addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE,
      .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE};
   VKF_CHECK(vkCreateSampler(c->device, &sci, NULL, &r->sampler));

   VkCommandBuffer cmd = vkf_command(c);
   VkImageMemoryBarrier barriers[2] = {
      {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
       .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
       .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
       .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
       .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
       .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .image = r->sampled,
       .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}},
      {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
       .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
       .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
       .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
       .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
       .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .image = r->color,
       .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}},
   };
   vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      0, 0, NULL, 0, NULL, 2, barriers);
   VkBufferImageCopy copy = {.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
      .imageExtent = {2, 1, 1}};
   vkCmdCopyBufferToImage(cmd, r->upload.buffer, r->sampled,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
   barriers[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
   barriers[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
   barriers[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
   barriers[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
   vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, barriers);
   return vkf_submit(c, cmd);
}

static void
destroy_resources(struct vkf_ctx *c, struct resources *r)
{
   vkDestroySampler(c->device, r->sampler, NULL);
   vkDestroyImageView(c->device, r->color_view, NULL);
   vkDestroyImage(c->device, r->color, NULL);
   vkFreeMemory(c->device, r->color_mem, NULL);
   vkDestroyImageView(c->device, r->sampled_view, NULL);
   vkDestroyImage(c->device, r->sampled, NULL);
   vkFreeMemory(c->device, r->sampled_mem, NULL);
   vkf_buffer_destroy(c, &r->upload); vkf_buffer_destroy(c, &r->b16);
   vkf_buffer_destroy(c, &r->b8); vkf_buffer_destroy(c, &r->out);
}

static int
make_compute(struct vkf_ctx *c, VkPipelineLayout layout, const uint8_t *code,
             size_t size, VkPipeline *pipeline)
{
   VkShaderModule module = vkf_shader(c, (const uint32_t *)code, size);
   VkPipelineCreateFlags2CreateInfo flags2 = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_CREATE_FLAGS_2_CREATE_INFO};
   VkComputePipelineCreateInfo ci = {.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      .pNext = &flags2,
      .stage = {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_COMPUTE_BIT, .module = module, .pName = "main"},
      .layout = layout};
   VkResult result = vkCreateComputePipelines(c->device, VK_NULL_HANDLE, 1, &ci,
                                               NULL, pipeline);
   vkDestroyShaderModule(c->device, module, NULL);
   return result != VK_SUCCESS;
}

static int
make_graphics(struct vkf_ctx *c, VkPipelineLayout layout, VkPipeline *pipeline)
{
   VkShaderModule vs = vkf_shader(c, (const uint32_t *)p8_vert_spv, sizeof(p8_vert_spv));
   VkShaderModule fs = vkf_shader(c, (const uint32_t *)p8_frag_spv, sizeof(p8_frag_spv));
   VkPipelineShaderStageCreateInfo stages[] = {
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
       .stage = VK_SHADER_STAGE_VERTEX_BIT, .module = vs, .pName = "main"},
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
       .stage = VK_SHADER_STAGE_FRAGMENT_BIT, .module = fs, .pName = "main"}};
   VkPipelineVertexInputStateCreateInfo vi = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
   VkPipelineInputAssemblyStateCreateInfo ia = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
   VkViewport viewport = {0, 0, 1, 1, 0, 1};
   VkRect2D scissor = {{0, 0}, {1, 1}};
   VkPipelineViewportStateCreateInfo vp = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1, .pViewports = &viewport, .scissorCount = 1, .pScissors = &scissor};
   VkPipelineRasterizationDepthClipStateCreateInfoEXT depth_clip = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_DEPTH_CLIP_STATE_CREATE_INFO_EXT,
      .depthClipEnable = VK_TRUE};
   VkPipelineRasterizationStateCreateInfo rs = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .pNext = &depth_clip, .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_NONE, .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
      .lineWidth = 1.0f};
   VkPipelineMultisampleStateCreateInfo ms = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT};
   VkPipelineColorBlendAttachmentState attachments[2] = {
      {.colorWriteMask = 0xf}, {.colorWriteMask = 0xf}};
   VkPipelineColorBlendStateCreateInfo cb = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .attachmentCount = 2, .pAttachments = attachments};
   VkFormat formats[2] = {VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM};
   VkPipelineRenderingCreateInfo rendering = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
      .colorAttachmentCount = 2, .pColorAttachmentFormats = formats};
   VkPipelineCreateFlags2CreateInfo flags2 = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_CREATE_FLAGS_2_CREATE_INFO, .pNext = &rendering};
   VkGraphicsPipelineCreateInfo ci = {.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .pNext = &flags2, .stageCount = 2, .pStages = stages, .pVertexInputState = &vi,
      .pInputAssemblyState = &ia, .pViewportState = &vp, .pRasterizationState = &rs,
      .pMultisampleState = &ms, .pColorBlendState = &cb, .layout = layout};
   VkResult result = vkCreateGraphicsPipelines(c->device, VK_NULL_HANDLE, 1, &ci,
                                                NULL, pipeline);
   vkDestroyShaderModule(c->device, fs, NULL); vkDestroyShaderModule(c->device, vs, NULL);
   return result != VK_SUCCESS;
}

static void
bind2(VkCommandBuffer cmd, VkPipelineBindPoint point, VkPipelineLayout layout,
      VkDescriptorSet set)
{
   VkBindDescriptorSetsInfo info = {.sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO,
      .stageFlags = point == VK_PIPELINE_BIND_POINT_COMPUTE ? VK_SHADER_STAGE_COMPUTE_BIT :
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      .layout = layout, .firstSet = 0, .descriptorSetCount = 1, .pDescriptorSets = &set};
   vkCmdBindDescriptorSets2(cmd, &info);
}

int
main(int argc, char **argv)
{
   if (argc != 2) { fprintf(stderr, "usage: %s <ICD.so>\n", argv[0]); return 2; }
   const char *extensions[] = {VK_KHR_MAINTENANCE_5_EXTENSION_NAME,
      VK_KHR_MAINTENANCE_6_EXTENSION_NAME, VK_KHR_LOAD_STORE_OP_NONE_EXTENSION_NAME,
      VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME,
      VK_EXT_DYNAMIC_RENDERING_UNUSED_ATTACHMENTS_EXTENSION_NAME};
   VkPhysicalDeviceDynamicRenderingUnusedAttachmentsFeaturesEXT unused = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_UNUSED_ATTACHMENTS_FEATURES_EXT,
      .dynamicRenderingUnusedAttachments = VK_TRUE};
   VkPhysicalDeviceDepthClipEnableFeaturesEXT depth = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT,
      .pNext = &unused, .depthClipEnable = VK_TRUE};
   VkPhysicalDeviceMaintenance6Features maintenance6 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_6_FEATURES,
      .pNext = &depth, .maintenance6 = VK_TRUE};
   VkPhysicalDeviceMaintenance5Features maintenance5 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_5_FEATURES,
      .pNext = &maintenance6, .maintenance5 = VK_TRUE};
   VkPhysicalDeviceVulkan12Features v12 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
      .pNext = &maintenance5, .samplerMirrorClampToEdge = VK_TRUE,
      .storageBuffer8BitAccess = VK_TRUE, .shaderInt8 = VK_TRUE};
   VkPhysicalDeviceVulkan11Features v11 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
      .pNext = &v12, .storageBuffer16BitAccess = VK_TRUE, .shaderDrawParameters = VK_TRUE};
   VkPhysicalDeviceVulkan13Features v13 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
      .pNext = &v11, .dynamicRendering = VK_TRUE};
   VkPhysicalDeviceFeatures core = {.fragmentStoresAndAtomics = VK_TRUE};
   struct vkf_ctx c;
   if (vkf_init(&c, argv[1], &v13, &core, extensions, 5)) return 1;
   if (!vkCmdBindDescriptorSets2)
      vkCmdBindDescriptorSets2 = (PFN_vkCmdBindDescriptorSets2)c.gipa(c.instance, "vkCmdBindDescriptorSets2KHR");
   if (!vkCmdPushConstants2)
      vkCmdPushConstants2 = (PFN_vkCmdPushConstants2)c.gipa(c.instance, "vkCmdPushConstants2KHR");

   int maintenance_ok = vkCmdBindDescriptorSets2 && vkCmdPushConstants2;
   int failed = !maintenance_ok;
   for (uint32_t i = 0; i < 5; i++) {
      int present = has_extension(&c, extensions[i]);
      printf("extension %s %s\n", extensions[i], present ? "PASS" : "FAIL");
      failed |= !present;
   }
   struct resources r;
   if (make_resources(&c, &r)) return 1;
   VkDescriptorSetLayoutBinding bindings[] = {
      {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_ALL, NULL},
      {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL},
      {2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL},
      {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL}};
   VkDescriptorSetLayoutCreateInfo dlci = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = 4, .pBindings = bindings};
   VkDescriptorSetLayout dsl; VKF_CHECK(vkCreateDescriptorSetLayout(c.device, &dlci, NULL, &dsl));
   VkDescriptorPoolSize sizes[] = {{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3},
                                   {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}};
   VkDescriptorPoolCreateInfo dpci = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = 1, .poolSizeCount = 2, .pPoolSizes = sizes};
   VkDescriptorPool pool; VKF_CHECK(vkCreateDescriptorPool(c.device, &dpci, NULL, &pool));
   VkDescriptorSetAllocateInfo dsai = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = pool, .descriptorSetCount = 1, .pSetLayouts = &dsl};
   VkDescriptorSet set; VKF_CHECK(vkAllocateDescriptorSets(c.device, &dsai, &set));
   VkDescriptorBufferInfo buffers[] = {{r.out.buffer, 0, 16}, {r.b8.buffer, 0, 4},
                                       {r.b16.buffer, 0, 8}};
   VkDescriptorImageInfo image = {r.sampler, r.sampled_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
   VkWriteDescriptorSet writes[4];
   for (uint32_t i = 0; i < 3; i++)
      writes[i] = (VkWriteDescriptorSet){.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = set, .dstBinding = i, .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .pBufferInfo = &buffers[i]};
   writes[3] = (VkWriteDescriptorSet){.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = set, .dstBinding = 3, .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .pImageInfo = &image};
   vkUpdateDescriptorSets(c.device, 4, writes, 0, NULL);
   VkPushConstantRange range = {VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT, 0, 4};
   VkPipelineLayoutCreateInfo plci = {.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1, .pSetLayouts = &dsl, .pushConstantRangeCount = 1,
      .pPushConstantRanges = &range};
   VkPipelineLayout layout; VKF_CHECK(vkCreatePipelineLayout(c.device, &plci, NULL, &layout));
   VkPipeline int_pipe, sampler_pipe, graphics;
   if (make_compute(&c, layout, p8_int_comp_spv, sizeof(p8_int_comp_spv), &int_pipe) ||
       make_compute(&c, layout, p8_sampler_comp_spv, sizeof(p8_sampler_comp_spv), &sampler_pipe) ||
       make_graphics(&c, layout, &graphics)) {
      fprintf(stderr, "FAIL pipeline creation\n"); return 1;
   }

   VkCommandBuffer cmd = vkf_command(&c);
   vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, int_pipe); bind2(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, layout, set);
   uint32_t addend = 11;
   VkPushConstantsInfo push = {.sType = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO,
      .layout = layout, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
      .offset = 0, .size = 4, .pValues = &addend};
   vkCmdPushConstants2(cmd, &push); vkCmdDispatch(cmd, 1, 1, 1);
   VkMemoryBarrier memory = {.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
      .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT, .dstAccessMask = VK_ACCESS_SHADER_READ_BIT};
   vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memory, 0, NULL, 0, NULL);
   vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, sampler_pipe); vkCmdDispatch(cmd, 1, 1, 1);
   if (vkf_submit(&c, cmd)) return 1;
   uint32_t *out = vkf_map(&c, &r.out); int8_t *got8 = vkf_map(&c, &r.b8); int16_t *got16 = vkf_map(&c, &r.b16);
   int ints_ok = out[0] == 318 && got8[1] == 12 && got16[1] == 309;
   int sampler_ok = out[1] == 0x4d434c45u;
   printf("storageBuffer8BitAccess+shaderInt8+storageBuffer16BitAccess %s\n", ints_ok ? "PASS" : "FAIL");
   printf("samplerMirrorClampToEdge %s\n", sampler_ok ? "PASS" : "FAIL");
   failed |= !ints_ok || !sampler_ok;
   vkUnmapMemory(c.device, r.b16.memory); vkUnmapMemory(c.device, r.b8.memory); vkUnmapMemory(c.device, r.out.memory);

   int depth_ok = 1;
   for (uint32_t pass = 0; pass < 2; pass++) {
      cmd = vkf_command(&c);
      VkRenderingAttachmentInfo color[2] = {
         {.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO, .imageView = r.color_view,
          .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
          .loadOp = VK_ATTACHMENT_LOAD_OP_NONE, .storeOp = VK_ATTACHMENT_STORE_OP_NONE},
         {.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO, .imageView = VK_NULL_HANDLE,
          .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
          .loadOp = VK_ATTACHMENT_LOAD_OP_NONE, .storeOp = VK_ATTACHMENT_STORE_OP_NONE}};
      VkRenderingInfo ri = {.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
         .renderArea = {{0, 0}, {1, 1}}, .layerCount = 1,
         .colorAttachmentCount = 2, .pColorAttachments = color};
      vkCmdBeginRendering(cmd, &ri); vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics);
      bind2(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, set);
      float z = pass ? 0.0f : 2.0f;
      vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, 4, &z);
      vkCmdDraw(cmd, 3, 1, 0, 37); vkCmdEndRendering(cmd);
      if (vkf_submit(&c, cmd)) return 1;
      out = vkf_map(&c, &r.out);
      if (!pass) depth_ok &= out[3] == 0;
      else depth_ok &= out[2] == 37 && out[3] > 0;
      vkUnmapMemory(c.device, r.out.memory);
   }
   out = vkf_map(&c, &r.out);
   int graphics_ok = out[2] == 37 && out[3] > 0;
   vkUnmapMemory(c.device, r.out.memory);
   failed |= !depth_ok || !graphics_ok;
   printf("shaderDrawParameters+fragmentStoresAndAtomics %s\n", graphics_ok ? "PASS" : "FAIL");
   printf("depthClipEnable %s\n", depth_ok ? "PASS" : "FAIL");
   printf("maintenance5+maintenance6 %s\n", maintenance_ok ? "PASS" : "FAIL");
   printf("load_store_op_none+dynamicRenderingUnusedAttachments %s\n", graphics_ok ? "PASS" : "FAIL");

   vkDestroyPipeline(c.device, graphics, NULL); vkDestroyPipeline(c.device, sampler_pipe, NULL);
   vkDestroyPipeline(c.device, int_pipe, NULL); vkDestroyPipelineLayout(c.device, layout, NULL);
   vkDestroyDescriptorPool(c.device, pool, NULL); vkDestroyDescriptorSetLayout(c.device, dsl, NULL);
   destroy_resources(&c, &r); vkf_finish(&c);
   return failed;
}
