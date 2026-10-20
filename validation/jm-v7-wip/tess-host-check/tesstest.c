/* Host recording check for jm-v7 014 tessellation under the panfrost
 * drm-shim: compiles tess pipelines and records/submits tessellated draws. */
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { VkResult r_ = (x); if (r_ != VK_SUCCESS) { fprintf(stderr, "FAIL %s = %d (line %d)\n", #x, r_, __LINE__); exit(1); } } while (0)

static PFN_vkGetInstanceProcAddr gipa;
static VkInstance inst;
static VkDevice dev;
#define IFN(n) PFN_##n n = (PFN_##n)gipa(inst, #n)
#define DFN(n) n = (PFN_##n)gdpa(dev, #n)

static PFN_vkGetDeviceProcAddr gdpa;
static PFN_vkCreateShaderModule vkCreateShaderModule;
static PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines;

static VkShaderModule load(const char *path)
{
   FILE *f = fopen(path, "rb");
   if (!f) { perror(path); exit(1); }
   fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
   uint32_t *code = malloc(n); fread(code, 1, n, f); fclose(f);
   VkShaderModuleCreateInfo ci = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, NULL, 0, n, code};
   VkShaderModule m; CHECK(vkCreateShaderModule(dev, &ci, NULL, &m));
   free(code);
   return m;
}

static VkPipeline make_pipeline(VkPipelineLayout layout, VkRenderPass rp,
                                const char *tesc, const char *tese,
                                const char *geom, unsigned cps, int raster_discard)
{
   VkPipelineShaderStageCreateInfo st[5] = {0};
   unsigned n = 0;
   const char *paths[5] = {"vs.vert.spv", tesc, tese, geom, "col.frag.spv"};
   VkShaderStageFlagBits stages[5] = {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
      VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, VK_SHADER_STAGE_GEOMETRY_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};
   for (unsigned i = 0; i < 5; i++) {
      if (!paths[i]) continue;
      st[n].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      st[n].stage = stages[i];
      st[n].module = load(paths[i]);
      st[n].pName = "main";
      n++;
   }
   VkPipelineVertexInputStateCreateInfo vi = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
   VkPipelineInputAssemblyStateCreateInfo ia = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, NULL, 0, VK_PRIMITIVE_TOPOLOGY_PATCH_LIST};
   VkPipelineTessellationDomainOriginStateCreateInfo dor = {VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_DOMAIN_ORIGIN_STATE_CREATE_INFO, NULL, VK_TESSELLATION_DOMAIN_ORIGIN_LOWER_LEFT};
   VkPipelineTessellationStateCreateInfo ts = {VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO, cps == 4 ? &dor : NULL, 0, cps};
   VkViewport vp = {0, 0, 64, 64, 0, 1};
   VkRect2D sc = {{0, 0}, {64, 64}};
   VkPipelineViewportStateCreateInfo vps = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, NULL, 0, 1, &vp, 1, &sc};
   VkPipelineRasterizationStateCreateInfo rs = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
   rs.polygonMode = VK_POLYGON_MODE_FILL; rs.cullMode = VK_CULL_MODE_BACK_BIT; rs.lineWidth = 1.0f;
   rs.rasterizerDiscardEnable = raster_discard;
   VkPipelineMultisampleStateCreateInfo ms = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
   ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
   VkPipelineColorBlendAttachmentState att = {0}; att.colorWriteMask = 0xf;
   VkPipelineColorBlendStateCreateInfo cb = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
   cb.attachmentCount = 1; cb.pAttachments = &att;
   VkGraphicsPipelineCreateInfo ci = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
   ci.stageCount = n; ci.pStages = st; ci.pVertexInputState = &vi; ci.pInputAssemblyState = &ia;
   ci.pTessellationState = &ts; ci.pViewportState = &vps; ci.pRasterizationState = &rs;
   ci.pMultisampleState = &ms; ci.pColorBlendState = &cb; ci.layout = layout; ci.renderPass = rp;
   VkPipeline p; CHECK(vkCreateGraphicsPipelines(dev, VK_NULL_HANDLE, 1, &ci, NULL, &p));
   return p;
}

int main(int argc, char **argv)
{
   void *so = dlopen(argc > 1 ? argv[1] : "libvulkan_panfrost.so", RTLD_NOW);
   if (!so) { fprintf(stderr, "%s\n", dlerror()); return 1; }
   gipa = (PFN_vkGetInstanceProcAddr)dlsym(so, "vk_icdGetInstanceProcAddr");
   PFN_vkCreateInstance vkCreateInstance = (PFN_vkCreateInstance)gipa(NULL, "vkCreateInstance");
   VkApplicationInfo ai = {VK_STRUCTURE_TYPE_APPLICATION_INFO, NULL, "tesstest", 1, NULL, 0, VK_API_VERSION_1_1};
   VkInstanceCreateInfo ici = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, NULL, 0, &ai};
   CHECK(vkCreateInstance(&ici, NULL, &inst));
   IFN(vkEnumeratePhysicalDevices); IFN(vkGetPhysicalDeviceFeatures2); IFN(vkCreateDevice);
   IFN(vkGetPhysicalDeviceProperties); IFN(vkGetPhysicalDeviceMemoryProperties);
   gdpa = (PFN_vkGetDeviceProcAddr)gipa(inst, "vkGetDeviceProcAddr");
   uint32_t npd = 1; VkPhysicalDevice pd;
   CHECK(vkEnumeratePhysicalDevices(inst, &npd, &pd));
   VkPhysicalDeviceProperties props; vkGetPhysicalDeviceProperties(pd, &props);
   printf("device %s, maxTessGenLevel %u, patch %u\n", props.deviceName,
          props.limits.maxTessellationGenerationLevel, props.limits.maxTessellationPatchSize);
   VkPhysicalDeviceTransformFeedbackFeaturesEXT xfbf = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT};
   VkPhysicalDeviceFeatures2 f2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, &xfbf};
   vkGetPhysicalDeviceFeatures2(pd, &f2);
   printf("tess %u gs %u xfb %u\n", f2.features.tessellationShader, f2.features.geometryShader, xfbf.transformFeedback);
   if (!f2.features.tessellationShader) return 2;
   float prio = 1.0f;
   VkDeviceQueueCreateInfo qci = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, NULL, 0, 0, 1, &prio};
   const char *exts[] = {"VK_EXT_transform_feedback"};
   VkDeviceCreateInfo dci = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, &f2, 0, 1, &qci, 0, NULL, 1, exts, NULL};
   CHECK(vkCreateDevice(pd, &dci, NULL, &dev));

   PFN_vkGetDeviceQueue vkGetDeviceQueue; PFN_vkCreateImage vkCreateImage; PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements;
   PFN_vkAllocateMemory vkAllocateMemory; PFN_vkBindImageMemory vkBindImageMemory; PFN_vkCreateImageView vkCreateImageView;
   PFN_vkCreateRenderPass vkCreateRenderPass; PFN_vkCreateFramebuffer vkCreateFramebuffer; PFN_vkCreatePipelineLayout vkCreatePipelineLayout;
   PFN_vkCreateBuffer vkCreateBuffer; PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements; PFN_vkBindBufferMemory vkBindBufferMemory;
   PFN_vkMapMemory vkMapMemory; PFN_vkCreateCommandPool vkCreateCommandPool; PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers;
   PFN_vkBeginCommandBuffer vkBeginCommandBuffer; PFN_vkEndCommandBuffer vkEndCommandBuffer; PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass;
   PFN_vkCmdEndRenderPass vkCmdEndRenderPass; PFN_vkCmdBindPipeline vkCmdBindPipeline; PFN_vkCmdDraw vkCmdDraw;
   PFN_vkCmdDrawIndexed vkCmdDrawIndexed; PFN_vkCmdDrawIndirect vkCmdDrawIndirect; PFN_vkCmdDrawIndexedIndirect vkCmdDrawIndexedIndirect;
   PFN_vkCmdBindIndexBuffer vkCmdBindIndexBuffer; PFN_vkCmdPushConstants vkCmdPushConstants; PFN_vkQueueSubmit vkQueueSubmit;
   PFN_vkQueueWaitIdle vkQueueWaitIdle; PFN_vkCmdClearAttachments vkCmdClearAttachments;
   PFN_vkCmdBindTransformFeedbackBuffersEXT vkCmdBindTransformFeedbackBuffersEXT;
   PFN_vkCmdBeginTransformFeedbackEXT vkCmdBeginTransformFeedbackEXT; PFN_vkCmdEndTransformFeedbackEXT vkCmdEndTransformFeedbackEXT;
   PFN_vkCmdDrawIndirectByteCountEXT vkCmdDrawIndirectByteCountEXT; PFN_vkDeviceWaitIdle vkDeviceWaitIdle;
   PFN_vkDestroyDevice vkDestroyDevice; PFN_vkFreeCommandBuffers vkFreeCommandBuffers; PFN_vkResetCommandBuffer vkResetCommandBuffer;
   DFN(vkGetDeviceQueue); DFN(vkCreateImage); DFN(vkGetImageMemoryRequirements); DFN(vkAllocateMemory); DFN(vkBindImageMemory);
   DFN(vkCreateImageView); DFN(vkCreateRenderPass); DFN(vkCreateFramebuffer); DFN(vkCreatePipelineLayout); DFN(vkCreateBuffer);
   DFN(vkGetBufferMemoryRequirements); DFN(vkBindBufferMemory); DFN(vkMapMemory); DFN(vkCreateCommandPool);
   DFN(vkAllocateCommandBuffers); DFN(vkBeginCommandBuffer); DFN(vkEndCommandBuffer); DFN(vkCmdBeginRenderPass);
   DFN(vkCmdEndRenderPass); DFN(vkCmdBindPipeline); DFN(vkCmdDraw); DFN(vkCmdDrawIndexed); DFN(vkCmdDrawIndirect);
   DFN(vkCmdDrawIndexedIndirect); DFN(vkCmdBindIndexBuffer); DFN(vkCmdPushConstants); DFN(vkQueueSubmit); DFN(vkQueueWaitIdle);
   DFN(vkCmdClearAttachments); DFN(vkCmdBindTransformFeedbackBuffersEXT); DFN(vkCmdBeginTransformFeedbackEXT);
   DFN(vkCmdEndTransformFeedbackEXT); DFN(vkCmdDrawIndirectByteCountEXT); DFN(vkDeviceWaitIdle); DFN(vkDestroyDevice);
   DFN(vkFreeCommandBuffers); DFN(vkResetCommandBuffer);
   DFN(vkCreateShaderModule); DFN(vkCreateGraphicsPipelines);

   VkQueue q; vkGetDeviceQueue(dev, 0, 0, &q);
   VkPhysicalDeviceMemoryProperties mp; vkGetPhysicalDeviceMemoryProperties(pd, &mp);
   uint32_t host_type = 0;
   for (uint32_t i = 0; i < mp.memoryTypeCount; i++)
      if (mp.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) { host_type = i; break; }

   const uint32_t layers = 2;
   VkImageCreateInfo imci = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, NULL, 0, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM,
      {64, 64, 1}, 1, layers, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT};
   VkImage img; CHECK(vkCreateImage(dev, &imci, NULL, &img));
   VkMemoryRequirements mr; vkGetImageMemoryRequirements(dev, img, &mr);
   VkMemoryAllocateInfo mai = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, NULL, mr.size, ffs(mr.memoryTypeBits) - 1};
   VkDeviceMemory imem; CHECK(vkAllocateMemory(dev, &mai, NULL, &imem)); CHECK(vkBindImageMemory(dev, img, imem, 0));
   VkImageViewCreateInfo ivci = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, NULL, 0, img, VK_IMAGE_VIEW_TYPE_2D_ARRAY,
      VK_FORMAT_R8G8B8A8_UNORM, {0}, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, layers}};
   VkImageView iv; CHECK(vkCreateImageView(dev, &ivci, NULL, &iv));
   VkAttachmentDescription ad = {0, VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR,
      VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
   VkAttachmentReference ar = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
   VkSubpassDescription sd = {0, VK_PIPELINE_BIND_POINT_GRAPHICS, 0, NULL, 1, &ar};
   VkRenderPassCreateInfo rpci = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, NULL, 0, 1, &ad, 1, &sd};
   VkRenderPass rp; CHECK(vkCreateRenderPass(dev, &rpci, NULL, &rp));
   VkFramebufferCreateInfo fbci = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, NULL, 0, rp, 1, &iv, 64, 64, layers};
   VkFramebuffer fb; CHECK(vkCreateFramebuffer(dev, &fbci, NULL, &fb));
   VkPushConstantRange pcr = {VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, 0, 8};
   VkPipelineLayoutCreateInfo plci = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, NULL, 0, 0, NULL, 1, &pcr};
   VkPipelineLayout pl; CHECK(vkCreatePipelineLayout(dev, &plci, NULL, &pl));

   VkPipeline p_tri = make_pipeline(pl, rp, "tri.tesc.spv", "tri.tese.spv", NULL, 3, 0);
   printf("pipeline tri ok\n");
   VkPipeline p_quad = make_pipeline(pl, rp, "quad.tesc.spv", "quad.tese.spv", NULL, 4, 0);
   printf("pipeline quad ok\n");
   VkPipeline p_iso = make_pipeline(pl, rp, "quad.tesc.spv", "iso.tese.spv", NULL, 4, 0);
   printf("pipeline iso ok\n");
   VkPipeline p_pts = make_pipeline(pl, rp, "tri.tesc.spv", "pts.tese.spv", NULL, 3, 0);
   printf("pipeline points ok\n");
   VkPipeline p_gs = make_pipeline(pl, rp, "tri.tesc.spv", "tri.tese.spv", "pass.geom.spv", 3, 0);
   printf("pipeline tess+gs ok\n");
   VkPipeline p_xfb = make_pipeline(pl, rp, "tri.tesc.spv", "xfb.tese.spv", NULL, 3, 0);
   printf("pipeline tess+xfb ok\n");

   VkBufferCreateInfo bci = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, NULL, 0, 1 << 20,
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
      VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT | VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT_EXT};
   VkBuffer buf; CHECK(vkCreateBuffer(dev, &bci, NULL, &buf));
   vkGetBufferMemoryRequirements(dev, buf, &mr);
   VkMemoryAllocateInfo bmai = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, NULL, mr.size, host_type};
   VkDeviceMemory bmem; CHECK(vkAllocateMemory(dev, &bmai, NULL, &bmem)); CHECK(vkBindBufferMemory(dev, buf, bmem, 0));
   uint32_t *map; CHECK(vkMapMemory(dev, bmem, 0, VK_WHOLE_SIZE, 0, (void **)&map));
   uint16_t *idx = (uint16_t *)map;
   for (int i = 0; i < 12; i++) idx[i] = (uint16_t)(i % 4);
   VkDrawIndirectCommand *di = (VkDrawIndirectCommand *)(map + 64);
   di[0] = (VkDrawIndirectCommand){6, 2, 0, 0}; di[1] = (VkDrawIndirectCommand){3, 1, 0, 0};
   VkDrawIndexedIndirectCommand *dii = (VkDrawIndexedIndirectCommand *)(map + 128);
   dii[0] = (VkDrawIndexedIndirectCommand){12, 1, 0, 0, 0};

   VkCommandPoolCreateInfo cpci = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, NULL, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, 0};
   VkCommandPool pool; CHECK(vkCreateCommandPool(dev, &cpci, NULL, &pool));
   VkCommandBufferAllocateInfo cbai = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, NULL, pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1};
   VkCommandBuffer cmd; CHECK(vkAllocateCommandBuffers(dev, &cbai, &cmd));

   for (int iter = 0; iter < 2; iter++) {
   VkCommandBufferBeginInfo cbbi = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
   CHECK(vkBeginCommandBuffer(cmd, &cbbi));
   VkClearValue cv = {{{0, 0, 0, 1}}};
   VkRenderPassBeginInfo rpbi = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, NULL, rp, fb, {{0, 0}, {64, 64}}, 1, &cv};
   vkCmdBeginRenderPass(cmd, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
   float pc[2] = {4.0f, 0}; memcpy(&pc[1], &(int){1}, 4);
   vkCmdPushConstants(cmd, pl, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, 0, 8, pc);
   vkCmdBindIndexBuffer(cmd, buf, 0, VK_INDEX_TYPE_UINT16);

   vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, p_tri);
   vkCmdDraw(cmd, 6, 3, 0, 0);
   vkCmdDrawIndexed(cmd, 9, 1, 0, 0, 0);
   vkCmdDrawIndirect(cmd, buf, 256, 2, sizeof(VkDrawIndirectCommand));
   vkCmdDrawIndexedIndirect(cmd, buf, 512, 1, sizeof(VkDrawIndexedIndirectCommand));
   VkClearAttachment ca = {VK_IMAGE_ASPECT_COLOR_BIT, 0, {{{1, 0, 0, 1}}}};
   VkClearRect cr = {{{0, 0}, {8, 8}}, 0, 1};
   vkCmdClearAttachments(cmd, 1, &ca, 1, &cr);
   vkCmdDraw(cmd, 3, 1, 0, 0); /* after meta: tess shaders restored */
   printf("tri draws recorded\n");

   vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, p_quad);
   vkCmdDraw(cmd, 8, 1, 0, 0);
   vkCmdDrawIndexedIndirect(cmd, buf, 512, 1, sizeof(VkDrawIndexedIndirectCommand));
   vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, p_iso);
   vkCmdDraw(cmd, 4, 2, 0, 0);
   vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, p_pts);
   vkCmdDraw(cmd, 3, 1, 0, 0);
   printf("quad/iso/points recorded\n");

   vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, p_gs);
   vkCmdDraw(cmd, 6, 1, 0, 0);
   vkCmdDrawIndirect(cmd, buf, 256, 1, sizeof(VkDrawIndirectCommand));
   printf("tess+gs recorded\n");

   VkDeviceSize xoff = 4096, xsize = 65536, coff = 128 * 1024;
   vkCmdBindTransformFeedbackBuffersEXT(cmd, 0, 1, &buf, &xoff, &xsize);
   vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, p_xfb);
   vkCmdBeginTransformFeedbackEXT(cmd, 0, 1, &buf, &coff);
   vkCmdDraw(cmd, 3, 1, 0, 0);
   vkCmdDrawIndexed(cmd, 6, 1, 0, 0, 0);
   vkCmdEndTransformFeedbackEXT(cmd, 0, 1, &buf, &coff);
   vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, p_tri);
   vkCmdDrawIndirectByteCountEXT(cmd, 1, 0, buf, coff, 0, 32);
   printf("tess+xfb recorded\n");

   vkCmdEndRenderPass(cmd);
   CHECK(vkEndCommandBuffer(cmd));
   VkSubmitInfo si = {VK_STRUCTURE_TYPE_SUBMIT_INFO, NULL, 0, NULL, NULL, 1, &cmd};
   if (!getenv("NOSUBMIT")) {
      CHECK(vkQueueSubmit(q, 1, &si, VK_NULL_HANDLE));
      CHECK(vkQueueWaitIdle(q));
   }
   CHECK(vkResetCommandBuffer(cmd, 0));
   printf("iteration %d submitted\n", iter);
   }
   if (!getenv("NOSUBMIT"))
      CHECK(vkDeviceWaitIdle(dev));
   printf("PASS\n");
   return 0;
}
