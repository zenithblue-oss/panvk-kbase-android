/* workload_perf: real-workload perf cases for the emulated paths of the
 * perf plan (docs/PERF-OPTIMIZATION-PLAN.md items 7, 8, 13, 18-22).
 *
 * Every case records one command buffer = one frame of the pattern a
 * DXVK/vkd3d game or an emulator (Yuzu/Eden/Citron, Winlator, Dolphin,
 * RPCS3) produces, resubmits it for PT_WP_MS ms (default 400) and prints
 *   BENCH workload_perf case=<name> ms=<ms per frame> us_op=<us per op> fps=<frames/s> ops=<n>
 * Each emulated case has a "ref" case without the emulated feature; the
 * emulation cost is the difference. Images that must not change with the
 * feature (pass-through GS, XFB, predicate true, queries) are compared with
 * the ref image (MISMATCH = FAIL).
 *
 *  ref / gs_pass / gs_layer / xfb_vs / xfb_gs : 512 small draws x 48 tris,
 *     pass-through GS, GS writing gl_Layer, transform feedback with and
 *     without a GS (DXVK stream output, emulator GS/XFB)          items 8
 *  strip_ref / strip_gs : 256 indexed strips with primitive restart,
 *     with a pass-through GS                                       item 13
 *  cond_true / cond_false : 512 draws, each under its own conditional
 *     rendering block (D3D predication)                            item 19
 *  ts_ref / ts_pair : 64 draws, with a timestamp pair and the result read
 *     back every frame (DXVK HUD)                                  item 20
 *  pg_query : 512 draws inside a primitives-generated query        item 21
 *  msaa_ref / msaa_a2one / msaa_sshade : 4x MSAA 512^2, 16 full-screen
 *     draws, alphaToOne, sample shading                            item 18
 *  tex_rgba / tex_bgra : 16 full-screen draws sampling a 512^2 RGBA8 /
 *     BGRA8 texture with a custom border colour (no format)        item 22
 *  bc_level : cold device (MESA_SHADER_CACHE_DISABLE), create and upload
 *     6 BC textures 1024^2 with mips (BC1, BC3, BC4, BC5, BC6H, BC7) as a
 *     level load, ms from the first vkCreateImage to the upload fence;
 *     also device_ms (vkCreateDevice + harness setup)               item 7
 * Present cost (item 16) is in swapchain_lifecycle (BENCH present).
 *
 * usage: workload-perf <libvulkan_panfrost.so>
 */
#include "../tests/dxvk/vulkan/dx7_harness.h"
#include "workload-perf-spv.h"

#define X_FUNCS(X)                                                             \
   X(EnumerateDeviceExtensionProperties) X(GetPhysicalDeviceFeatures2)         \
   X(GetPhysicalDeviceFormatProperties) X(CmdBindTransformFeedbackBuffersEXT)  \
   X(CmdBeginTransformFeedbackEXT) X(CmdEndTransformFeedbackEXT)               \
   X(CmdBeginConditionalRenderingEXT) X(CmdEndConditionalRenderingEXT)         \
   X(CreateQueryPool) X(CmdResetQueryPool) X(CmdWriteTimestamp)                \
   X(GetQueryPoolResults) X(CmdBeginQuery) X(CmdEndQuery) X(CreateSampler)     \
   X(CreateDescriptorSetLayout) X(CreateDescriptorPool)                        \
   X(AllocateDescriptorSets) X(UpdateDescriptorSets) X(CmdBindDescriptorSets)  \
   X(CmdCopyBufferToImage) X(DestroyImage) X(DeviceWaitIdle) X(DestroyDevice)  \
   X(DestroyInstance)
#define X_DECL(n) static PFN_vk##n x##n;
X_FUNCS(X_DECL)

static PFN_vkGetInstanceProcAddr gipa;

static void
load_x(struct dx7 *t)
{
#define X_LOAD(n) x##n = (PFN_vk##n)gipa(t->inst, "vk" #n);
   X_FUNCS(X_LOAD)
}

/* ---- device: enable what the cases need, when exposed ---------------- */
static VkPhysicalDeviceFeatures want;
static VkPhysicalDeviceTransformFeedbackFeaturesEXT f_xfb = {
   .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT};
static VkPhysicalDeviceConditionalRenderingFeaturesEXT f_cond = {
   .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONDITIONAL_RENDERING_FEATURES_EXT};
static VkPhysicalDevicePrimitivesGeneratedQueryFeaturesEXT f_pg = {
   .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRIMITIVES_GENERATED_QUERY_FEATURES_EXT};
static VkPhysicalDeviceCustomBorderColorFeaturesEXT f_cbc = {
   .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_FEATURES_EXT};
static const char *exts[8];
static int has_xfb, has_cond, has_pg, has_cbc;

static int
has_ext(VkExtensionProperties *e, uint32_t n, const char *name)
{
   for (uint32_t i = 0; i < n; i++)
      if (!strcmp(e[i].extensionName, name))
         return 1;
   return 0;
}

static void
device_hook(struct dx7 *t, VkDeviceCreateInfo *dci)
{
   PFN_vkEnumerateDeviceExtensionProperties enum_ext =
      (PFN_vkEnumerateDeviceExtensionProperties)gipa(
         t->inst, "vkEnumerateDeviceExtensionProperties");
   PFN_vkGetPhysicalDeviceFeatures2 feats2 =
      (PFN_vkGetPhysicalDeviceFeatures2)gipa(t->inst,
                                             "vkGetPhysicalDeviceFeatures2");
   uint32_t n = 0;
   enum_ext(t->phys, NULL, &n, NULL);
   VkExtensionProperties *e = calloc(n, sizeof(*e));
   enum_ext(t->phys, NULL, &n, e);
   f_xfb.pNext = &f_cond;
   f_cond.pNext = &f_pg;
   f_pg.pNext = &f_cbc;
   f_cbc.pNext = NULL;
   VkPhysicalDeviceFeatures2 f2 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &f_xfb};
   feats2(t->phys, &f2);
   has_xfb = has_ext(e, n, VK_EXT_TRANSFORM_FEEDBACK_EXTENSION_NAME) &&
             f_xfb.transformFeedback;
   has_cond = has_ext(e, n, VK_EXT_CONDITIONAL_RENDERING_EXTENSION_NAME) &&
              f_cond.conditionalRendering;
   has_pg = has_ext(e, n, VK_EXT_PRIMITIVES_GENERATED_QUERY_EXTENSION_NAME) &&
            f_pg.primitivesGeneratedQuery;
   has_cbc = has_ext(e, n, VK_EXT_CUSTOM_BORDER_COLOR_EXTENSION_NAME) &&
             f_cbc.customBorderColors;
   free(e);
   uint32_t ne = 0;
   void *chain = NULL;
#define ADD(flag, name, st)                                                    \
   if (flag) {                                                                 \
      exts[ne++] = name;                                                       \
      (st).pNext = chain;                                                      \
      chain = &(st);                                                           \
   }
   ADD(has_xfb, VK_EXT_TRANSFORM_FEEDBACK_EXTENSION_NAME, f_xfb)
   ADD(has_cond, VK_EXT_CONDITIONAL_RENDERING_EXTENSION_NAME, f_cond)
   ADD(has_pg, VK_EXT_PRIMITIVES_GENERATED_QUERY_EXTENSION_NAME, f_pg)
   ADD(has_cbc, VK_EXT_CUSTOM_BORDER_COLOR_EXTENSION_NAME, f_cbc)
   /* Only what is used: keep the rest of the feature structs off. */
   f_xfb.geometryStreams = 0;
   f_pg.primitivesGeneratedQueryWithRasterizerDiscard = 0;
   f_pg.primitivesGeneratedQueryWithNonZeroStreams = 0;
   memset(&want, 0, sizeof(want));
   want.geometryShader = t->feats.geometryShader;
   want.alphaToOne = t->feats.alphaToOne;
   want.sampleRateShading = t->feats.sampleRateShading;
   want.textureCompressionBC = t->feats.textureCompressionBC;
   dci->pEnabledFeatures = &want;
   dci->enabledExtensionCount = ne;
   dci->ppEnabledExtensionNames = exts;
   dci->pNext = chain;
}

static void
teardown(struct dx7 *t)
{
   xDeviceWaitIdle(t->dev);
   xDestroyDevice(t->dev, NULL);
   xDestroyInstance(t->inst, NULL);
}

/* ---- helpers ---------------------------------------------------------- */
static double
now_ms(void)
{
   struct timespec ts;
   clock_gettime(CLOCK_MONOTONIC, &ts);
   return ts.tv_sec * 1e3 + ts.tv_nsec / 1e6;
}

static void
submit_wait(struct dx7 *t, VkCommandBuffer cmd)
{
   VkSubmitInfo si = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                      .commandBufferCount = 1,
                      .pCommandBuffers = &cmd};
   CK(vkResetFences(t->dev, 1, &t->fence), "ResetFence");
   CK(vkQueueSubmit(t->queue, 1, &si, t->fence), "Submit");
   CK(vkWaitForFences(t->dev, 1, &t->fence, VK_TRUE, 30ull * 1000000000ull),
      "Wait");
}

static uint32_t
hash_px(const uint8_t *px)
{
   uint32_t h = 2166136261u;
   for (int i = 0; i < RT_W * RT_H * 4; i++)
      h = (h ^ px[i]) * 16777619u;
   return h;
}

static uint32_t
mem_type(struct dx7 *t, uint32_t bits)
{
   (void)t;
   uint32_t i = 0;
   while (!(bits & (1u << i)))
      i++;
   return i;
}

static VkImage
make_image(struct dx7 *t, VkFormat fmt, uint32_t w, uint32_t h, uint32_t mips,
           VkSampleCountFlagBits samples, VkImageUsageFlags usage,
           VkImageView *view)
{
   VkImageCreateInfo ci = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                           .imageType = VK_IMAGE_TYPE_2D,
                           .format = fmt,
                           .extent = {w, h, 1},
                           .mipLevels = mips,
                           .arrayLayers = 1,
                           .samples = samples,
                           .tiling = VK_IMAGE_TILING_OPTIMAL,
                           .usage = usage,
                           .sharingMode = VK_SHARING_MODE_EXCLUSIVE};
   VkImage img;
   CK(vkCreateImage(t->dev, &ci, NULL, &img), "Image");
   VkMemoryRequirements mr;
   vkGetImageMemoryRequirements(t->dev, img, &mr);
   VkMemoryAllocateInfo mai = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                               .allocationSize = mr.size,
                               .memoryTypeIndex = mem_type(t, mr.memoryTypeBits)};
   VkDeviceMemory m;
   CK(vkAllocateMemory(t->dev, &mai, NULL, &m), "ImgMem");
   CK(vkBindImageMemory(t->dev, img, m, 0), "ImgBind");
   if (view) {
      VkImageViewCreateInfo vci = {.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                                   .image = img,
                                   .viewType = VK_IMAGE_VIEW_TYPE_2D,
                                   .format = fmt,
                                   .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT,
                                                        0, mips, 0, 1}};
      CK(vkCreateImageView(t->dev, &vci, NULL, view), "View");
   }
   return img;
}

static void
barrier(VkCommandBuffer cmd, VkImage img, uint32_t mips, VkImageLayout from,
        VkImageLayout to, VkAccessFlags src, VkAccessFlags dst,
        VkPipelineStageFlags ss, VkPipelineStageFlags ds)
{
   VkImageMemoryBarrier b = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                             .srcAccessMask = src,
                             .dstAccessMask = dst,
                             .oldLayout = from,
                             .newLayout = to,
                             .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                             .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                             .image = img,
                             .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0,
                                                  mips, 0, 1}};
   vkCmdPipelineBarrier(cmd, ss, ds, 0, 0, NULL, 0, NULL, 1, &b);
}

struct pipe_desc {
   const uint32_t *vs, *gs, *fs;
   size_t vs_size, gs_size, fs_size;
   VkPrimitiveTopology topo;
   int restart, no_vinput, a2one, sshade;
   VkSampleCountFlagBits samples;
   VkRenderPass rp;
   VkPipelineLayout layout;
   uint32_t w, h;
};

static VkPipeline
make_pipe(struct dx7 *t, const struct pipe_desc *d)
{
   VkShaderModule vs = dx7_module(t, d->vs, d->vs_size);
   VkShaderModule fs = dx7_module(t, d->fs, d->fs_size);
   VkShaderModule gs = d->gs ? dx7_module(t, d->gs, d->gs_size) : VK_NULL_HANDLE;
   VkPipelineShaderStageCreateInfo st[3] = {
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
       .stage = VK_SHADER_STAGE_VERTEX_BIT, .module = vs, .pName = "main"},
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
       .stage = VK_SHADER_STAGE_FRAGMENT_BIT, .module = fs, .pName = "main"},
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
       .stage = VK_SHADER_STAGE_GEOMETRY_BIT, .module = gs, .pName = "main"}};
   VkVertexInputBindingDescription vb = {0, 16, VK_VERTEX_INPUT_RATE_VERTEX};
   VkVertexInputAttributeDescription va = {0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0};
   VkPipelineVertexInputStateCreateInfo vi = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = d->no_vinput ? 0 : 1,
      .pVertexBindingDescriptions = &vb,
      .vertexAttributeDescriptionCount = d->no_vinput ? 0 : 1,
      .pVertexAttributeDescriptions = &va};
   VkPipelineInputAssemblyStateCreateInfo ia = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = d->topo,
      .primitiveRestartEnable = d->restart};
   VkViewport vp = {0, 0, (float)d->w, (float)d->h, 0, 1};
   VkRect2D sc = {{0, 0}, {d->w, d->h}};
   VkPipelineViewportStateCreateInfo vps = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1, .pViewports = &vp, .scissorCount = 1, .pScissors = &sc};
   VkPipelineRasterizationStateCreateInfo rs = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_NONE,
      .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
      .lineWidth = 1.0f};
   VkPipelineMultisampleStateCreateInfo ms = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = d->samples ? d->samples : VK_SAMPLE_COUNT_1_BIT,
      .sampleShadingEnable = d->sshade,
      .minSampleShading = 1.0f,
      .alphaToOneEnable = d->a2one};
   /* Alpha blending, as a game's particles/UI over the scene. */
   VkPipelineColorBlendAttachmentState ba = {
      .blendEnable = VK_TRUE,
      .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
      .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      .colorBlendOp = VK_BLEND_OP_ADD,
      .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
      .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
      .alphaBlendOp = VK_BLEND_OP_ADD,
      .colorWriteMask = 0xf};
   VkPipelineColorBlendStateCreateInfo cb = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .attachmentCount = 1, .pAttachments = &ba};
   VkGraphicsPipelineCreateInfo ci = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .stageCount = gs ? 3 : 2, .pStages = st,
      .pVertexInputState = &vi, .pInputAssemblyState = &ia,
      .pViewportState = &vps, .pRasterizationState = &rs,
      .pMultisampleState = &ms, .pColorBlendState = &cb,
      .layout = d->layout, .renderPass = d->rp};
   VkPipeline p;
   CK(vkCreateGraphicsPipelines(t->dev, VK_NULL_HANDLE, 1, &ci, NULL, &p), "Pipe");
   vkDestroyShaderModule(t->dev, vs, NULL);
   vkDestroyShaderModule(t->dev, fs, NULL);
   if (gs)
      vkDestroyShaderModule(t->dev, gs, NULL);
   return p;
}

static double budget_ms = 400;
static double fps_headline;
static int fails;

/* Times an already recorded frame; returns ms per frame. */
static double
bench(struct dx7 *t, VkCommandBuffer cmd, const char *name, int ops,
      void (*per_frame)(struct dx7 *, void *), void *data)
{
   submit_wait(t, cmd);
   submit_wait(t, cmd);
   long frames = 0;
   double start = now_ms(), el;
   do {
      submit_wait(t, cmd);
      if (per_frame)
         per_frame(t, data);
      frames++;
      el = now_ms() - start;
   } while (el < budget_ms || frames < 3);
   const double ms = el / frames;
   printf("BENCH workload_perf case=%s ms=%.3f us_op=%.2f fps=%.1f ops=%d "
          "frames=%ld\n",
          name, ms, ms * 1000.0 / ops, 1000.0 / ms, ops, frames);
   return ms;
}

/* ---- geometry cases (harness 64x64 target) ---------------------------- */
#define DRAWS 512
#define TRIS 48
#define STRIP_DRAWS 256
#define QUADS 64

enum geo { G_PLAIN, G_XFB, G_COND, G_PG };

struct geo_rec {
   VkPipeline pipe;
   VkBuffer vb, ib, xfb, pred;
   VkQueryPool pg;
   enum geo kind;
   int strip, draws;
};

static void
record_geo(struct dx7 *t, VkCommandBuffer cmd, const struct geo_rec *g)
{
   VkCommandBufferBeginInfo bbi = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
   CK(vkResetCommandBuffer(cmd, 0), "ResetCmd");
   CK(vkBeginCommandBuffer(cmd, &bbi), "Begin");
   if (g->kind == G_PG)
      xCmdResetQueryPool(cmd, g->pg, 0, 1);
   VkClearValue clear = {.color = {{0.0f, 0.0f, 0.0f, 1.0f}}};
   VkRenderPassBeginInfo rpbi = {.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                                 .renderPass = t->rp, .framebuffer = t->fb,
                                 .renderArea = {{0, 0}, {RT_W, RT_H}},
                                 .clearValueCount = 1, .pClearValues = &clear};
   vkCmdBeginRenderPass(cmd, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
   vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, g->pipe);
   VkDeviceSize off = 0;
   vkCmdBindVertexBuffers(cmd, 0, 1, &g->vb, &off);
   if (g->strip)
      vkCmdBindIndexBuffer(cmd, g->ib, 0, VK_INDEX_TYPE_UINT32);
   if (g->kind == G_XFB) {
      VkDeviceSize xo = 0, xs = VK_WHOLE_SIZE;
      xCmdBindTransformFeedbackBuffersEXT(cmd, 0, 1, &g->xfb, &xo, &xs);
      xCmdBeginTransformFeedbackEXT(cmd, 0, 0, NULL, NULL);
   }
   if (g->kind == G_PG)
      xCmdBeginQuery(cmd, g->pg, 0, 0);
   for (int i = 0; i < g->draws; i++) {
      float pc[4] = {0.013f * i, 0, 0.002f * (float)(i % 16 - 8),
                     0.002f * (float)((i / 16) % 16 - 8)};
      vkCmdPushConstants(cmd, t->layout,
                         VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                         0, 16, pc);
      if (g->kind == G_COND) {
         VkConditionalRenderingBeginInfoEXT cr = {
            .sType = VK_STRUCTURE_TYPE_CONDITIONAL_RENDERING_BEGIN_INFO_EXT,
            .buffer = g->pred};
         xCmdBeginConditionalRenderingEXT(cmd, &cr);
      }
      if (g->strip)
         vkCmdDrawIndexed(cmd, QUADS * 5, 1, 0, 0, 0);
      else
         vkCmdDraw(cmd, TRIS * 3, 1, 0, 0);
      if (g->kind == G_COND)
         xCmdEndConditionalRenderingEXT(cmd);
   }
   if (g->kind == G_PG)
      xCmdEndQuery(cmd, g->pg, 0);
   if (g->kind == G_XFB)
      xCmdEndTransformFeedbackEXT(cmd, 0, 0, NULL, NULL);
   vkCmdEndRenderPass(cmd);
   barrier(cmd, t->img, 1, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
           VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
           VK_PIPELINE_STAGE_TRANSFER_BIT);
   VkBufferImageCopy region = {.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
                               .imageExtent = {RT_W, RT_H, 1}};
   vkCmdCopyImageToBuffer(cmd, t->img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                          t->rbuf, 1, &region);
   CK(vkEndCommandBuffer(cmd), "End");
}

static uint32_t ref_hash, strip_ref_hash;
static uint8_t ref_px[RT_W * RT_H * 4];

static double
run_geo(struct dx7 *t, const char *name, struct geo_rec *g,
        const struct pipe_desc *pd, uint32_t want_hash)
{
   g->pipe = make_pipe(t, pd);
   record_geo(t, t->cmd, g);
   submit_wait(t, t->cmd);
   const uint32_t h = hash_px(t->px);
   if (want_hash && h != want_hash) {
      int bad = 0, first = -1;
      if (!g->strip)
         for (int i = 0; i < RT_W * RT_H; i++)
            if (memcmp(t->px + i * 4, ref_px + i * 4, 4) && !bad++)
               first = i;
      printf("MISMATCH case=%s hash=%08x ref=%08x px_diff=%d", name, h,
             want_hash, bad);
      if (first >= 0)
         printf(" first=(%d,%d) got=%u,%u,%u,%u ref=%u,%u,%u,%u", first % RT_W,
                first / RT_W, t->px[first * 4], t->px[first * 4 + 1],
                t->px[first * 4 + 2], t->px[first * 4 + 3], ref_px[first * 4],
                ref_px[first * 4 + 1], ref_px[first * 4 + 2],
                ref_px[first * 4 + 3]);
      printf("\n");
      fails++;
   }
   /* Only the two ref cases set the reference (cond_false draws nothing). */
   if (!strcmp(name, "ref")) {
      ref_hash = h;
      memcpy(ref_px, t->px, sizeof(ref_px));
   } else if (!strcmp(name, "strip_ref")) {
      strip_ref_hash = h;
   }
   const double ms = bench(t, t->cmd, name, g->draws, NULL, NULL);
   vkDestroyPipeline(t->dev, g->pipe, NULL);
   return ms;
}

static void
geo_cases(struct dx7 *t)
{
   /* 48 small triangles scattered over the target, z = triangle id. */
   float v[TRIS * 3][4];
   for (int i = 0; i < TRIS; i++) {
      float cx = -0.85f + 0.25f * (i % 8), cy = -0.85f + 0.3f * (i / 8);
      float c[3][2] = {{cx, cy}, {cx + 0.2f, cy}, {cx, cy + 0.22f}};
      for (int k = 0; k < 3; k++) {
         v[i * 3 + k][0] = c[k][0];
         v[i * 3 + k][1] = c[k][1];
         v[i * 3 + k][2] = (float)i;
         v[i * 3 + k][3] = 1;
      }
   }
   /* Strips: 64 quads (4 indices + restart) over a 16x16 vertex grid. */
   float sv[256][4];
   for (int i = 0; i < 256; i++) {
      sv[i][0] = -0.9f + 1.8f * (i % 16) / 15.0f;
      sv[i][1] = -0.9f + 1.8f * (i / 16) / 15.0f;
      sv[i][2] = (float)i;
      sv[i][3] = 1;
   }
   uint32_t idx[QUADS * 5];
   for (int q = 0; q < QUADS; q++) {
      int x = (q % 8) * 2, y = (q / 8) * 2, b = y * 16 + x;
      uint32_t *o = idx + q * 5;
      o[0] = b, o[1] = b + 1, o[2] = b + 16, o[3] = b + 17, o[4] = 0xffffffffu;
   }
   VkBuffer vb, svb, ib, xfb = VK_NULL_HANDLE, pred_t = VK_NULL_HANDLE,
                         pred_f = VK_NULL_HANDLE;
   VkDeviceMemory m;
   dx7_buffer(t, sizeof(v), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, v, &vb, &m);
   dx7_buffer(t, sizeof(sv), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, sv, &svb, &m);
   dx7_buffer(t, sizeof(idx), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, idx, &ib, &m);
   const uint32_t one = 1, zero = 0;
   if (has_cond) {
      dx7_buffer(t, 4, VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT, &one, &pred_t, &m);
      dx7_buffer(t, 4, VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT, &zero, &pred_f, &m);
   }
   if (has_xfb)
      dx7_buffer(t, 16u << 20, VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT,
                 NULL, &xfb, &m);

   struct pipe_desc pd = {.vs = wp_vs, .vs_size = sizeof(wp_vs),
                          .fs = wp_fs, .fs_size = sizeof(wp_fs),
                          .topo = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                          .rp = t->rp, .layout = t->layout, .w = RT_W, .h = RT_H};
   struct geo_rec g = {.vb = vb, .xfb = xfb, .kind = G_PLAIN, .draws = DRAWS};
   run_geo(t, "ref", &g, &pd, 0);

   if (t->feats.geometryShader) {
      struct pipe_desc p2 = pd;
      p2.gs = wp_gs, p2.gs_size = sizeof(wp_gs);
      run_geo(t, "gs_pass", &g, &p2, ref_hash);
      p2.gs = wp_gs_layer, p2.gs_size = sizeof(wp_gs_layer);
      run_geo(t, "gs_layer", &g, &p2, ref_hash);
   }
   if (has_xfb) {
      struct pipe_desc p2 = pd;
      struct geo_rec gx = g;
      gx.kind = G_XFB;
      p2.vs = wp_vs_xfb, p2.vs_size = sizeof(wp_vs_xfb);
      run_geo(t, "xfb_vs", &gx, &p2, ref_hash);
      if (t->feats.geometryShader) {
         p2 = pd;
         p2.gs = wp_gs_xfb, p2.gs_size = sizeof(wp_gs_xfb);
         const double ms = run_geo(t, "xfb_gs", &gx, &p2, ref_hash);
         fps_headline = 1000.0 / ms;
      }
   }

   struct pipe_desc ps = pd;
   ps.topo = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
   ps.restart = 1;
   struct geo_rec gs_ = {.vb = svb, .ib = ib, .kind = G_PLAIN, .strip = 1,
                         .draws = STRIP_DRAWS};
   run_geo(t, "strip_ref", &gs_, &ps, 0);
   if (t->feats.geometryShader) {
      ps.gs = wp_gs, ps.gs_size = sizeof(wp_gs);
      run_geo(t, "strip_gs", &gs_, &ps, strip_ref_hash);
   }

   if (has_cond) {
      struct geo_rec gc = g;
      gc.kind = G_COND;
      gc.pred = pred_t;
      run_geo(t, "cond_true", &gc, &pd, ref_hash);
      gc.pred = pred_f;
      run_geo(t, "cond_false", &gc, &pd, 0);
   }
   if (has_pg) {
      VkQueryPoolCreateInfo qci = {.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
                                   .queryType = VK_QUERY_TYPE_PRIMITIVES_GENERATED_EXT,
                                   .queryCount = 1};
      struct geo_rec gp = g;
      gp.kind = G_PG;
      CK(xCreateQueryPool(t->dev, &qci, NULL, &gp.pg), "PGPool");
      run_geo(t, "pg_query", &gp, &pd, ref_hash);
      uint64_t prims = 0;
      xGetQueryPoolResults(t->dev, gp.pg, 0, 1, 8, &prims, 8,
                           VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
      if (prims != (uint64_t)DRAWS * TRIS) {
         printf("MISMATCH case=pg_query prims=%llu want=%d\n",
                (unsigned long long)prims, DRAWS * TRIS);
         fails++;
      }
   }
}

/* ---- timestamps: 64 draws + a timestamp pair read back per frame ------ */
struct ts_data {
   VkQueryPool pool;
};

static void
ts_read(struct dx7 *t, void *data)
{
   struct ts_data *d = data;
   uint64_t r[2];
   CK(xGetQueryPoolResults(t->dev, d->pool, 0, 2, sizeof(r), r, 8,
                           VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT),
      "TSResults");
}

static void
ts_cases(struct dx7 *t)
{
   float v[TRIS * 3][4] = {{0}};
   for (int i = 0; i < TRIS * 3; i++)
      v[i][0] = -0.8f + 0.03f * (i % 50), v[i][1] = -0.8f + 0.3f * ((i / 3) % 5),
      v[i][3] = 1;
   VkBuffer vb;
   VkDeviceMemory m;
   dx7_buffer(t, sizeof(v), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, v, &vb, &m);
   struct pipe_desc pd = {.vs = wp_vs, .vs_size = sizeof(wp_vs),
                          .fs = wp_fs, .fs_size = sizeof(wp_fs),
                          .topo = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                          .rp = t->rp, .layout = t->layout, .w = RT_W, .h = RT_H};
   VkPipeline pipe = make_pipe(t, &pd);
   VkQueryPoolCreateInfo qci = {.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
                                .queryType = VK_QUERY_TYPE_TIMESTAMP,
                                .queryCount = 2};
   struct ts_data d;
   CK(xCreateQueryPool(t->dev, &qci, NULL, &d.pool), "TSPool");
   for (int ts = 0; ts < 2; ts++) {
      VkCommandBuffer cmd = t->cmd;
      VkCommandBufferBeginInfo bbi = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
      CK(vkResetCommandBuffer(cmd, 0), "ResetCmd");
      CK(vkBeginCommandBuffer(cmd, &bbi), "Begin");
      if (ts) {
         xCmdResetQueryPool(cmd, d.pool, 0, 2);
         xCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, d.pool, 0);
      }
      VkClearValue clear = {.color = {{0, 0, 0, 1}}};
      VkRenderPassBeginInfo rpbi = {.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                                    .renderPass = t->rp, .framebuffer = t->fb,
                                    .renderArea = {{0, 0}, {RT_W, RT_H}},
                                    .clearValueCount = 1, .pClearValues = &clear};
      vkCmdBeginRenderPass(cmd, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
      vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe);
      VkDeviceSize off = 0;
      vkCmdBindVertexBuffers(cmd, 0, 1, &vb, &off);
      for (int i = 0; i < 64; i++) {
         float pc[4] = {0.01f * i, 0, 0, 0};
         vkCmdPushConstants(cmd, t->layout,
                            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                            0, 16, pc);
         vkCmdDraw(cmd, TRIS * 3, 1, 0, 0);
      }
      vkCmdEndRenderPass(cmd);
      if (ts)
         xCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, d.pool, 1);
      CK(vkEndCommandBuffer(cmd), "End");
      bench(t, cmd, ts ? "ts_pair" : "ts_ref", 1, ts ? ts_read : NULL, &d);
   }
   vkDestroyPipeline(t->dev, pipe, NULL);
}

/* ---- fragment cases: 512^2 targets ------------------------------------ */
#define FW 512
#define FH 512
#define FDRAWS 16

static VkRenderPass
make_rp(struct dx7 *t, VkFormat fmt, VkSampleCountFlagBits samples)
{
   VkAttachmentDescription att[2] = {
      {.format = fmt, .samples = samples,
       .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
       .storeOp = samples > 1 ? VK_ATTACHMENT_STORE_OP_DONT_CARE
                              : VK_ATTACHMENT_STORE_OP_STORE,
       .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
       .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
      {.format = fmt, .samples = VK_SAMPLE_COUNT_1_BIT,
       .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
       .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
       .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
       .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}};
   VkAttachmentReference ref = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
   VkAttachmentReference res = {1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
   VkSubpassDescription sub = {.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                               .colorAttachmentCount = 1,
                               .pColorAttachments = &ref,
                               .pResolveAttachments = samples > 1 ? &res : NULL};
   VkRenderPassCreateInfo ci = {.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                                .attachmentCount = samples > 1 ? 2 : 1,
                                .pAttachments = att,
                                .subpassCount = 1,
                                .pSubpasses = &sub};
   VkRenderPass rp;
   CK(vkCreateRenderPass(t->dev, &ci, NULL, &rp), "RP");
   return rp;
}

static void
record_frag(struct dx7 *t, VkCommandBuffer cmd, VkRenderPass rp, VkFramebuffer fb,
            VkPipeline pipe, VkPipelineLayout layout, VkDescriptorSet *set)
{
   VkCommandBufferBeginInfo bbi = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
   CK(vkResetCommandBuffer(cmd, 0), "ResetCmd");
   CK(vkBeginCommandBuffer(cmd, &bbi), "Begin");
   VkClearValue clear = {.color = {{0.1f, 0.1f, 0.1f, 1}}};
   VkRenderPassBeginInfo rpbi = {.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                                 .renderPass = rp, .framebuffer = fb,
                                 .renderArea = {{0, 0}, {FW, FH}},
                                 .clearValueCount = 1, .pClearValues = &clear};
   vkCmdBeginRenderPass(cmd, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
   vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe);
   if (set)
      xCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1,
                             set, 0, NULL);
   for (int i = 0; i < FDRAWS; i++)
      vkCmdDraw(cmd, 3, 1, 0, 0);
   vkCmdEndRenderPass(cmd);
   CK(vkEndCommandBuffer(cmd), "End");
}

static void
msaa_cases(struct dx7 *t)
{
   const VkFormat f = VK_FORMAT_R8G8B8A8_UNORM;
   VkRenderPass rp = make_rp(t, f, VK_SAMPLE_COUNT_4_BIT);
   VkImageView v[2];
   make_image(t, f, FW, FH, 1, VK_SAMPLE_COUNT_4_BIT,
              VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &v[0]);
   make_image(t, f, FW, FH, 1, VK_SAMPLE_COUNT_1_BIT,
              VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &v[1]);
   VkFramebufferCreateInfo fbci = {.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                                   .renderPass = rp, .attachmentCount = 2,
                                   .pAttachments = v, .width = FW, .height = FH,
                                   .layers = 1};
   VkFramebuffer fb;
   CK(vkCreateFramebuffer(t->dev, &fbci, NULL, &fb), "FB");
   static const struct {
      const char *name;
      int a2one, sshade;
   } c[] = {{"msaa_ref", 0, 0}, {"msaa_a2one", 1, 0}, {"msaa_sshade", 0, 1}};
   for (unsigned i = 0; i < 3; i++) {
      if ((c[i].a2one && !t->feats.alphaToOne) ||
          (c[i].sshade && !t->feats.sampleRateShading))
         continue;
      struct pipe_desc pd = {.vs = wp_vs_full, .vs_size = sizeof(wp_vs_full),
                             .fs = wp_fs, .fs_size = sizeof(wp_fs),
                             .topo = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                             .no_vinput = 1, .a2one = c[i].a2one,
                             .sshade = c[i].sshade,
                             .samples = VK_SAMPLE_COUNT_4_BIT, .rp = rp,
                             .layout = t->layout, .w = FW, .h = FH};
      VkPipeline p = make_pipe(t, &pd);
      record_frag(t, t->cmd, rp, fb, p, t->layout, NULL);
      bench(t, t->cmd, c[i].name, FDRAWS, NULL, NULL);
      vkDestroyPipeline(t->dev, p, NULL);
   }
}

static void
tex_cases(struct dx7 *t)
{
   VkDescriptorSetLayoutBinding b = {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
                                     VK_SHADER_STAGE_FRAGMENT_BIT, NULL};
   VkDescriptorSetLayoutCreateInfo dlci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = 1, .pBindings = &b};
   VkDescriptorSetLayout dsl;
   CK(xCreateDescriptorSetLayout(t->dev, &dlci, NULL, &dsl), "DSL");
   VkPushConstantRange pcr = {VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 16};
   VkPipelineLayoutCreateInfo plci = {.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                                      .setLayoutCount = 1, .pSetLayouts = &dsl,
                                      .pushConstantRangeCount = 1,
                                      .pPushConstantRanges = &pcr};
   VkPipelineLayout layout;
   CK(vkCreatePipelineLayout(t->dev, &plci, NULL, &layout), "Layout");
   VkDescriptorPoolSize ps = {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2};
   VkDescriptorPoolCreateInfo dpci = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                                      .maxSets = 2, .poolSizeCount = 1,
                                      .pPoolSizes = &ps};
   VkDescriptorPool pool;
   CK(xCreateDescriptorPool(t->dev, &dpci, NULL, &pool), "DPool");

   /* The sampler: clamp to a custom border colour, no format given (DXVK). */
   VkSamplerCustomBorderColorCreateInfoEXT cbc = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CUSTOM_BORDER_COLOR_CREATE_INFO_EXT,
      .customBorderColor = {.float32 = {0.9f, 0.4f, 0.1f, 1.0f}},
      .format = VK_FORMAT_UNDEFINED};
   const int custom = has_cbc && f_cbc.customBorderColorWithoutFormat;
   VkSamplerCreateInfo sci = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                              .pNext = custom ? &cbc : NULL,
                              .magFilter = VK_FILTER_LINEAR,
                              .minFilter = VK_FILTER_LINEAR,
                              .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
                              .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
                              .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
                              .borderColor = custom ? VK_BORDER_COLOR_FLOAT_CUSTOM_EXT
                                                       : VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
                              .maxLod = 0};
   VkSampler smp;
   CK(xCreateSampler(t->dev, &sci, NULL, &smp), "Sampler");

   const VkFormat rtf = VK_FORMAT_R8G8B8A8_UNORM;
   VkRenderPass rp = make_rp(t, rtf, VK_SAMPLE_COUNT_1_BIT);
   VkImageView rtv;
   make_image(t, rtf, FW, FH, 1, VK_SAMPLE_COUNT_1_BIT,
              VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &rtv);
   VkFramebufferCreateInfo fbci = {.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                                   .renderPass = rp, .attachmentCount = 1,
                                   .pAttachments = &rtv, .width = FW, .height = FH,
                                   .layers = 1};
   VkFramebuffer fb;
   CK(vkCreateFramebuffer(t->dev, &fbci, NULL, &fb), "FB");

   /* Texture content from a staging buffer (as a game upload). */
   VkBuffer sb;
   VkDeviceMemory sm;
   uint32_t *texels = malloc(FW * FH * 4);
   for (int i = 0; i < FW * FH; i++)
      texels[i] = 0xff000000u | (i * 2654435761u >> 8);
   dx7_buffer(t, FW * FH * 4, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, texels, &sb, &sm);
   free(texels);

   struct pipe_desc pd = {.vs = wp_vs_full, .vs_size = sizeof(wp_vs_full),
                          .fs = wp_fs_tex, .fs_size = sizeof(wp_fs_tex),
                          .topo = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                          .no_vinput = 1, .rp = rp, .layout = layout,
                          .w = FW, .h = FH};
   VkPipeline pipe = make_pipe(t, &pd);
   static const struct {
      const char *name;
      VkFormat f;
   } c[] = {{"tex_rgba", VK_FORMAT_R8G8B8A8_UNORM},
            {"tex_bgra", VK_FORMAT_B8G8R8A8_UNORM}};
   for (unsigned i = 0; i < 2; i++) {
      VkImageView tv;
      VkImage ti = make_image(t, c[i].f, FW, FH, 1, VK_SAMPLE_COUNT_1_BIT,
                              VK_IMAGE_USAGE_SAMPLED_BIT |
                                 VK_IMAGE_USAGE_TRANSFER_DST_BIT, &tv);
      VkCommandBufferBeginInfo bbi = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
      CK(vkResetCommandBuffer(t->cmd, 0), "ResetCmd");
      CK(vkBeginCommandBuffer(t->cmd, &bbi), "Begin");
      barrier(t->cmd, ti, 1, VK_IMAGE_LAYOUT_UNDEFINED,
              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, VK_ACCESS_TRANSFER_WRITE_BIT,
              VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
      VkBufferImageCopy r = {.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
                             .imageExtent = {FW, FH, 1}};
      xCmdCopyBufferToImage(t->cmd, sb, ti, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &r);
      barrier(t->cmd, ti, 1, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT,
              VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
              VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
      CK(vkEndCommandBuffer(t->cmd), "End");
      submit_wait(t, t->cmd);

      VkDescriptorSetAllocateInfo dai = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                                         .descriptorPool = pool,
                                         .descriptorSetCount = 1,
                                         .pSetLayouts = &dsl};
      VkDescriptorSet set;
      CK(xAllocateDescriptorSets(t->dev, &dai, &set), "DSet");
      VkDescriptorImageInfo ii = {smp, tv, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
      VkWriteDescriptorSet w = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                .dstSet = set, .descriptorCount = 1,
                                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                .pImageInfo = &ii};
      xUpdateDescriptorSets(t->dev, 1, &w, 0, NULL);
      record_frag(t, t->cmd, rp, fb, pipe, layout, &set);
      bench(t, t->cmd, c[i].name, FDRAWS, NULL, NULL);
   }
   vkDestroyPipeline(t->dev, pipe, NULL);
}

/* ---- BC level load, cold device --------------------------------------- */
#define BC_DIM 1024
#define BC_MIPS 11

static void
bc_level(const char *icd)
{
   static const VkFormat fmts[] = {
      VK_FORMAT_BC1_RGB_UNORM_BLOCK, VK_FORMAT_BC3_UNORM_BLOCK,
      VK_FORMAT_BC4_UNORM_BLOCK,     VK_FORMAT_BC5_UNORM_BLOCK,
      VK_FORMAT_BC6H_UFLOAT_BLOCK,   VK_FORMAT_BC7_UNORM_BLOCK};
   const int nf = sizeof(fmts) / sizeof(fmts[0]);
   setenv("MESA_SHADER_CACHE_DISABLE", "true", 1);
   struct dx7 t;
   const double d0 = now_ms();
   dx7_init(&t, icd, NULL);
   const double device_ms = now_ms() - d0;
   load_x(&t);
   if (!t.feats.textureCompressionBC) {
      printf("BENCH workload_perf case=bc_level skipped=no_bc device_ms=%.1f\n",
             device_ms);
      teardown(&t);
      unsetenv("MESA_SHADER_CACHE_DISABLE");
      return;
   }
   /* Every mip of a 1024^2 BC image: 16 B per block for BC3/5/6H/7, 8 for
    * BC1/4. Staging holds the largest chain (16 B blocks). */
   VkDeviceSize chain = 0;
   for (int l = 0; l < BC_MIPS; l++) {
      uint32_t bw = ((BC_DIM >> l) + 3) / 4;
      chain += (VkDeviceSize)bw * bw * 16;
   }
   uint8_t *data = malloc(chain);
   uint32_t s = 12345;
   for (VkDeviceSize i = 0; i < chain; i++)
      data[i] = (s = s * 1103515245u + 12345u) >> 24;
   VkBuffer sb;
   VkDeviceMemory sm;
   dx7_buffer(&t, chain, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, data, &sb, &sm);
   free(data);

   const double t0 = now_ms();
   VkImage img[6];
   for (int i = 0; i < nf; i++)
      img[i] = make_image(&t, fmts[i], BC_DIM, BC_DIM, BC_MIPS, VK_SAMPLE_COUNT_1_BIT,
                          VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                          NULL);
   const double create_ms = now_ms() - t0;
   VkCommandBufferBeginInfo bbi = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
   CK(vkResetCommandBuffer(t.cmd, 0), "ResetCmd");
   CK(vkBeginCommandBuffer(t.cmd, &bbi), "Begin");
   for (int i = 0; i < nf; i++) {
      const uint32_t bb = (fmts[i] == VK_FORMAT_BC1_RGB_UNORM_BLOCK ||
                           fmts[i] == VK_FORMAT_BC4_UNORM_BLOCK) ? 8 : 16;
      VkBufferImageCopy r[BC_MIPS];
      VkDeviceSize off = 0;
      for (int l = 0; l < BC_MIPS; l++) {
         const uint32_t d = BC_DIM >> l, bw = (d + 3) / 4;
         r[l] = (VkBufferImageCopy){.bufferOffset = off,
                                    .imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, l, 0, 1},
                                    .imageExtent = {d, d, 1}};
         off += (VkDeviceSize)bw * bw * bb;
      }
      barrier(t.cmd, img[i], BC_MIPS, VK_IMAGE_LAYOUT_UNDEFINED,
              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, VK_ACCESS_TRANSFER_WRITE_BIT,
              VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
      xCmdCopyBufferToImage(t.cmd, sb, img[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            BC_MIPS, r);
      barrier(t.cmd, img[i], BC_MIPS, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT,
              VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
              VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
   }
   CK(vkEndCommandBuffer(t.cmd), "End");
   const double record_ms = now_ms() - t0 - create_ms;
   submit_wait(&t, t.cmd);
   const double total = now_ms() - t0;
   printf("BENCH workload_perf case=bc_level ms=%.1f us_op=%.0f fps=%.2f ops=%d "
          "device_ms=%.1f create_ms=%.1f record_ms=%.1f gpu_wait_ms=%.1f\n",
          total, total * 1000.0 / nf, 1000.0 / total, nf, device_ms, create_ms,
          record_ms, total - create_ms - record_ms);
   for (int i = 0; i < nf; i++)
      xDestroyImage(t.dev, img[i], NULL);
   teardown(&t);
   unsetenv("MESA_SHADER_CACHE_DISABLE");
}

int
main(int argc, char **argv)
{
   if (argc < 2) {
      printf("usage: %s <icd>\n", argv[0]);
      return 2;
   }
   const char *e = getenv("PT_WP_MS");
   if (e)
      budget_ms = atof(e);
   void *h = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
   gipa = h ? (PFN_vkGetInstanceProcAddr)dlsym(h, "vk_icdGetInstanceProcAddr") : NULL;
   if (!gipa) {
      printf("FAIL dlopen %s\n", argv[1]);
      return 1;
   }
   dx7_device_hook = device_hook;

   bc_level(argv[1]);

   struct dx7 t;
   dx7_init(&t, argv[1], NULL);
   load_x(&t);
   printf("workload_perf device=%s gs=%d xfb=%d cond=%d pg=%d cbc=%d/%d a2one=%d "
          "sshade=%d\n",
          t.props.deviceName, t.feats.geometryShader, has_xfb, has_cond, has_pg,
          has_cbc, f_cbc.customBorderColorWithoutFormat, t.feats.alphaToOne,
          t.feats.sampleRateShading);
   geo_cases(&t);
   ts_cases(&t);
   msaa_cases(&t);
   tex_cases(&t);
   teardown(&t);

   if (fps_headline > 0)
      printf("FPS %.1f (xfb_gs frame: 512 draws, GS + XFB)\n", fps_headline);
   printf("WORKLOAD_PERF_FAILS=%d\n", fails);
   printf("%s workload_perf\n", fails ? "FAIL" : "PASS");
   return fails ? 1 : 0;
}
