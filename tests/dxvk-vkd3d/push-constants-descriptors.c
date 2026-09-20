#include "../features/vk_test.h"
#include "push.comp.spv.h"
#include "push.frag.spv.h"
#include "push.vert.spv.h"

#define DESC_COUNT 32
#define PHASE_COUNT 6
#define WORD_COUNT 64

struct test_resources {
   struct vkf_buffer output;
   struct vkf_buffer values[DESC_COUNT];
   struct vkf_buffer replacement;
   VkImage image;
   VkDeviceMemory image_memory;
   VkImageView image_view;
};

static uint32_t
signature(const uint32_t *words)
{
   return words[0] ^ words[31] ^ words[47] ^ words[62] ^ words[63];
}

static int
has_extension(struct vkf_ctx *c, const char *name)
{
   uint32_t count = 0;
   VKF_CHECK(vkEnumerateDeviceExtensionProperties(c->physical, NULL, &count, NULL));
   VkExtensionProperties *props = calloc(count, sizeof(*props));
   VKF_CHECK(vkEnumerateDeviceExtensionProperties(c->physical, NULL, &count, props));
   int found = 0;
   for (uint32_t i = 0; i < count; i++)
      found |= strcmp(props[i].extensionName, name) == 0;
   free(props);
   return found;
}

static int
make_resources(struct vkf_ctx *c, struct test_resources *r)
{
   memset(r, 0, sizeof(*r));
   if (vkf_buffer_create(c, PHASE_COUNT * 4 * sizeof(uint32_t),
                         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 0, &r->output))
      return 1;
   memset(vkf_map(c, &r->output), 0, r->output.size);
   vkUnmapMemory(c->device, r->output.memory);

   for (uint32_t i = 1; i < DESC_COUNT; i++) {
      if (vkf_buffer_create(c, sizeof(uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                            0, &r->values[i]))
         return 1;
      *(uint32_t *)vkf_map(c, &r->values[i]) = 100 + i;
      vkUnmapMemory(c->device, r->values[i].memory);
   }
   if (vkf_buffer_create(c, sizeof(uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                         0, &r->replacement))
      return 1;
   *(uint32_t *)vkf_map(c, &r->replacement) = 999;
   vkUnmapMemory(c->device, r->replacement.memory);

   VkImageCreateInfo ici = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = VK_FORMAT_R8G8B8A8_UNORM,
      .extent = {1, 1, 1},
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
   };
   VKF_CHECK(vkCreateImage(c->device, &ici, NULL, &r->image));
   VkMemoryRequirements req;
   vkGetImageMemoryRequirements(c->device, r->image, &req);
   uint32_t mt = vkf_memory_type(c, req.memoryTypeBits,
                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
   if (mt == UINT32_MAX)
      mt = vkf_memory_type(c, req.memoryTypeBits, 0);
   VkMemoryAllocateInfo mai = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                               .allocationSize = req.size,
                               .memoryTypeIndex = mt};
   VKF_CHECK(vkAllocateMemory(c->device, &mai, NULL, &r->image_memory));
   VKF_CHECK(vkBindImageMemory(c->device, r->image, r->image_memory, 0));
   VkImageViewCreateInfo ivci = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = r->image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = VK_FORMAT_R8G8B8A8_UNORM,
      .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
   };
   VKF_CHECK(vkCreateImageView(c->device, &ivci, NULL, &r->image_view));
   return 0;
}

static void
destroy_resources(struct vkf_ctx *c, struct test_resources *r)
{
   vkDestroyImageView(c->device, r->image_view, NULL);
   vkDestroyImage(c->device, r->image, NULL);
   vkFreeMemory(c->device, r->image_memory, NULL);
   vkf_buffer_destroy(c, &r->replacement);
   for (uint32_t i = 1; i < DESC_COUNT; i++)
      vkf_buffer_destroy(c, &r->values[i]);
   vkf_buffer_destroy(c, &r->output);
}

static void
push_all_descriptors(VkCommandBuffer cmd, VkPipelineBindPoint bind_point,
                     VkPipelineLayout layout,
                     struct test_resources *r)
{
   VkDescriptorBufferInfo infos[DESC_COUNT];
   infos[0] = (VkDescriptorBufferInfo){r->output.buffer, 0, r->output.size};
   for (uint32_t i = 1; i < DESC_COUNT; i++)
      infos[i] = (VkDescriptorBufferInfo){r->values[i].buffer, 0, sizeof(uint32_t)};
   VkWriteDescriptorSet write = {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstBinding = 0,
      .descriptorCount = DESC_COUNT,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .pBufferInfo = infos,
   };
   vkCmdPushDescriptorSetKHR(cmd, bind_point, layout, 0, 1, &write);
   memset(infos, 0xa5, sizeof(infos));
}

static void
replace_last_descriptor(VkCommandBuffer cmd, VkPipelineBindPoint bind_point,
                        VkPipelineLayout layout,
                        struct test_resources *r)
{
   VkDescriptorBufferInfo info = {r->replacement.buffer, 0, sizeof(uint32_t)};
   VkWriteDescriptorSet write = {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstBinding = 0,
      .dstArrayElement = DESC_COUNT - 1,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .pBufferInfo = &info,
   };
   vkCmdPushDescriptorSetKHR(cmd, bind_point, layout, 0, 1, &write);
   memset(&info, 0x5a, sizeof(info));
}

static void
push_phase(VkCommandBuffer cmd, VkPipelineLayout layout, VkShaderStageFlags stages,
           uint32_t phase, uint32_t *source, uint32_t *shadow)
{
   source[2] = phase;
   if (phase == 0) {
      vkCmdPushConstants(cmd, layout, stages, 0, 128, source);
      memcpy(shadow, source, 128);
   } else {
      vkCmdPushConstants(cmd, layout, stages, 8, 4, &source[2]);
      shadow[2] = source[2];
      if (phase == 1) {
         vkCmdPushConstants(cmd, layout, stages, 128, 64, &source[32]);
         memcpy(&shadow[32], &source[32], 64);
      } else if (phase == 2) {
         vkCmdPushConstants(cmd, layout, stages, 192, 60, &source[48]);
         memcpy(&shadow[48], &source[48], 60);
      } else if (phase == 3) {
         vkCmdPushConstants(cmd, layout, stages, 252, 4, &source[63]);
         shadow[63] = source[63];
      } else {
         vkCmdPushConstants(cmd, layout, stages, 0, 256, source);
         memcpy(shadow, source, 256);
      }
   }
}

static int
make_compute_pipeline(struct vkf_ctx *c, VkPipelineLayout layout,
                      VkPipeline *pipeline)
{
   VkShaderModule module = vkf_shader(c, (const uint32_t *)push_comp_spv,
                                      sizeof(push_comp_spv));
   if (!module)
      return 1;
   VkComputePipelineCreateInfo ci = {
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      .stage = {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_COMPUTE_BIT,
                .module = module,
                .pName = "main"},
      .layout = layout,
   };
   VkResult result = vkCreateComputePipelines(c->device, VK_NULL_HANDLE, 1, &ci,
                                               NULL, pipeline);
   vkDestroyShaderModule(c->device, module, NULL);
   return result != VK_SUCCESS;
}

static int
make_graphics_pipeline(struct vkf_ctx *c, VkPipelineLayout layout,
                       VkPipeline *pipeline)
{
   VkShaderModule vs = vkf_shader(c, (const uint32_t *)push_vert_spv,
                                  sizeof(push_vert_spv));
   VkShaderModule fs = vkf_shader(c, (const uint32_t *)push_frag_spv,
                                  sizeof(push_frag_spv));
   if (!vs || !fs)
      return 1;
   VkPipelineShaderStageCreateInfo stages[] = {
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
       .stage = VK_SHADER_STAGE_VERTEX_BIT, .module = vs, .pName = "main"},
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
       .stage = VK_SHADER_STAGE_FRAGMENT_BIT, .module = fs, .pName = "main"},
   };
   VkPipelineVertexInputStateCreateInfo vi = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
   VkPipelineInputAssemblyStateCreateInfo ia = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
   VkViewport viewport = {0, 0, 1, 1, 0, 1};
   VkRect2D scissor = {{0, 0}, {1, 1}};
   VkPipelineViewportStateCreateInfo vp = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1, .pViewports = &viewport,
      .scissorCount = 1, .pScissors = &scissor};
   VkPipelineRasterizationStateCreateInfo rs = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .polygonMode = VK_POLYGON_MODE_FILL, .cullMode = VK_CULL_MODE_NONE,
      .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE, .lineWidth = 1.0f};
   VkPipelineMultisampleStateCreateInfo ms = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT};
   VkPipelineColorBlendAttachmentState attachment = {.colorWriteMask = 0xf};
   VkPipelineColorBlendStateCreateInfo cb = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .attachmentCount = 1, .pAttachments = &attachment};
   VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
   VkPipelineRenderingCreateInfo rendering = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
      .colorAttachmentCount = 1, .pColorAttachmentFormats = &format};
   VkGraphicsPipelineCreateInfo ci = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .pNext = &rendering,
      .stageCount = 2, .pStages = stages,
      .pVertexInputState = &vi, .pInputAssemblyState = &ia,
      .pViewportState = &vp, .pRasterizationState = &rs,
      .pMultisampleState = &ms, .pColorBlendState = &cb,
      .layout = layout,
   };
   VkResult result = vkCreateGraphicsPipelines(c->device, VK_NULL_HANDLE, 1, &ci,
                                                NULL, pipeline);
   vkDestroyShaderModule(c->device, fs, NULL);
   vkDestroyShaderModule(c->device, vs, NULL);
   return result != VK_SUCCESS;
}

static int
check_results(struct vkf_ctx *c, struct test_resources *r, const uint32_t *expected,
              int graphics)
{
   uint32_t *actual = vkf_map(c, &r->output);
   int failed = 0;
   for (uint32_t phase = 0; phase < PHASE_COUNT; phase++) {
      uint32_t desc = graphics ? (101 ^ (phase == 5 ? 999 : 131))
                               : (phase == 5 ? 999 : 131);
      uint32_t want[] = {expected[phase], graphics ? expected[phase] : 101, desc};
      for (uint32_t i = 0; i < 3; i++) {
         uint32_t got = actual[phase * 4 + i];
         if (got != want[i]) {
            fprintf(stderr, "FAIL %s phase=%u word=%u got=%u expected=%u\n",
                    graphics ? "graphics" : "compute", phase, i, got, want[i]);
            failed = 1;
         }
      }
   }
   vkUnmapMemory(c->device, r->output.memory);
   return failed;
}

static int
run_compute(struct vkf_ctx *c, struct test_resources *r, VkPipelineLayout layout,
            VkPipeline pipeline)
{
   memset(vkf_map(c, &r->output), 0, r->output.size);
   vkUnmapMemory(c->device, r->output.memory);
   uint32_t source[WORD_COUNT], shadow[WORD_COUNT] = {0}, expected[PHASE_COUNT];
   uint32_t initial[WORD_COUNT] = {0};
   for (uint32_t i = 0; i < WORD_COUNT; i++) source[i] = 0x10203040u + i * 0x101u;
   VkCommandBuffer cmd = vkf_command(c);
   vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
   push_all_descriptors(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, layout, r);
   vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, 256, initial);
   for (uint32_t phase = 0; phase < PHASE_COUNT; phase++) {
      push_phase(cmd, layout, VK_SHADER_STAGE_COMPUTE_BIT, phase, source, shadow);
      expected[phase] = signature(shadow);
      if (phase == 5)
         replace_last_descriptor(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, layout, r);
      vkCmdDispatch(cmd, 1, 1, 1);
   }
   if (vkf_submit(c, cmd)) return 1;
   return check_results(c, r, expected, 0);
}

static int
run_graphics(struct vkf_ctx *c, struct test_resources *r, VkPipelineLayout layout,
             VkPipeline pipeline)
{
   memset(vkf_map(c, &r->output), 0, r->output.size);
   vkUnmapMemory(c->device, r->output.memory);
   uint32_t source[WORD_COUNT], shadow[WORD_COUNT] = {0}, expected[PHASE_COUNT];
   uint32_t initial[WORD_COUNT] = {0};
   for (uint32_t i = 0; i < WORD_COUNT; i++) source[i] = 0x50607080u + i * 0x101u;
   VkCommandBuffer cmd = vkf_command(c);
   VkImageMemoryBarrier barrier = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = r->image,
      .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
   };
   vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
                        0, NULL, 0, NULL, 1, &barrier);
   VkRenderingAttachmentInfo attachment = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = r->image_view,
      .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
   };
   VkRenderingInfo rendering = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .renderArea = {{0, 0}, {1, 1}}, .layerCount = 1,
      .colorAttachmentCount = 1, .pColorAttachments = &attachment,
   };
   vkCmdBeginRendering(cmd, &rendering);
   vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
   push_all_descriptors(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, r);
   vkCmdPushConstants(cmd, layout,
                      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                      0, 256, initial);
   for (uint32_t phase = 0; phase < PHASE_COUNT; phase++) {
      push_phase(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                 phase, source, shadow);
      expected[phase] = signature(shadow);
      if (phase == 5)
         replace_last_descriptor(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, r);
      vkCmdDraw(cmd, 3, 1, 0, 0);
   }
   vkCmdEndRendering(cmd);
   if (vkf_submit(c, cmd)) return 1;
   return check_results(c, r, expected, 1);
}

int
main(int argc, char **argv)
{
   if (argc != 2) {
      fprintf(stderr, "usage: %s <libvulkan_panfrost.so>\n", argv[0]);
      return 2;
   }
   const char *extensions[] = {VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME};
   VkPhysicalDeviceDynamicRenderingFeatures dynamic = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
      .dynamicRendering = VK_TRUE,
   };
   struct vkf_ctx c;
   if (vkf_init(&c, argv[1], &dynamic, NULL, extensions, 1)) return 1;
   VkPhysicalDevicePushDescriptorPropertiesKHR push_props = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_DESCRIPTOR_PROPERTIES_KHR};
   VkPhysicalDeviceProperties2 props = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, .pNext = &push_props};
   vkGetPhysicalDeviceProperties2(c.physical, &props);
   int extension = has_extension(&c, VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
   printf("device=%s maxPushConstantsSize=%u VK_KHR_push_descriptor=%s maxPushDescriptors=%u\n",
          c.props.deviceName, c.props.limits.maxPushConstantsSize,
          extension ? "true" : "false", push_props.maxPushDescriptors);
   if (!extension || c.props.limits.maxPushConstantsSize < 256 ||
       push_props.maxPushDescriptors < DESC_COUNT || !vkCmdPushDescriptorSetKHR) {
      fprintf(stderr, "FAIL required push capability missing\n");
      return 1;
   }

   VkDescriptorSetLayoutBinding binding = {
      .binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .descriptorCount = DESC_COUNT,
      .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT};
   VkDescriptorSetLayoutCreateInfo dlci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR,
      .bindingCount = 1, .pBindings = &binding};
   VkDescriptorSetLayout dsl;
   VKF_CHECK(vkCreateDescriptorSetLayout(c.device, &dlci, NULL, &dsl));
   VkPushConstantRange range = {
      .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT |
                    VK_SHADER_STAGE_FRAGMENT_BIT,
      .offset = 0, .size = 256};
   VkPipelineLayoutCreateInfo plci = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1, .pSetLayouts = &dsl,
      .pushConstantRangeCount = 1, .pPushConstantRanges = &range};
   VkPipelineLayout layout;
   VKF_CHECK(vkCreatePipelineLayout(c.device, &plci, NULL, &layout));
   VkPipeline compute, graphics;
   if (make_compute_pipeline(&c, layout, &compute) ||
       make_graphics_pipeline(&c, layout, &graphics)) {
      fprintf(stderr, "FAIL pipeline creation\n");
      return 1;
   }
   struct test_resources resources;
   if (make_resources(&c, &resources)) return 1;
   int failed = run_compute(&c, &resources, layout, compute) |
                run_graphics(&c, &resources, layout, graphics);
   printf("push-constant boundaries: 128, 192, byte-255, 256 %s\n",
          failed ? "FAIL" : "PASS");
   printf("push-descriptor compute/graphics 32, replacement, record-lifetime %s\n",
          failed ? "FAIL" : "PASS");
   printf("invalid boundaries: size=255 requires 4-byte alignment; size=260 exceeds reported limit\n");
   destroy_resources(&c, &resources);
   vkDestroyPipeline(c.device, graphics, NULL);
   vkDestroyPipeline(c.device, compute, NULL);
   vkDestroyPipelineLayout(c.device, layout, NULL);
   vkDestroyDescriptorSetLayout(c.device, dsl, NULL);
   vkf_finish(&c);
   return failed;
}
