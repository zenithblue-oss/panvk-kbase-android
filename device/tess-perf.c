/* tess_perf: DXVK-like tessellation benchmark (tess perf review 2026-10-22).
 *
 * One command buffer = one frame of DRAWS small tessellated draws (16 quad
 * patches each, 4 control points, VS+TCS+TES(+GS)+FS), each with its own
 * push constant (tess factor + offset), as DXVK records a D3D11 frame of
 * terrain/character patches. The frame is resubmitted for PT_TESS_MS ms
 * (default 700) per config and timed with the fence.
 *
 * Configs: L=ref (same draws, no tessellation: per-draw baseline), L=1, 4,
 * 16 (every patch at that factor) and L=mix (per-patch factors 1..16), each
 * with the GS off and on (pass-through GS, when geometryShader is exposed).
 *
 * A/B inside one run: PT_TESS_AB lists ';'-separated variants, each a
 * ','-separated list of NAME=VALUE env assignments ("default" = none); a
 * new VkDevice is created per variant (the driver reads PANVK_TESS_* at
 * device creation). Default: "default;PANVK_TESS_FUSE=0,
 * PANVK_TESS_SETUP_STEP=0,PANVK_TESS_ARENAS=1,PANVK_TESS_WINDING_INLINE=0"
 * (the second variant is the pre-tess-perf behaviour). The readback of
 * every config must be identical across variants, or the test FAILs.
 *
 * Output: one BENCH line per (variant, config), e.g.
 *   BENCH tess_perf opt=default gs=0 L=16 draws=256 patches=4096 ms=12.34 us_draw=48.2 frames=57
 * then "FPS <fps>" for the first variant's gs=0 L=mix frame.
 *
 * usage: tess-perf <libvulkan_panfrost.so>
 */
#include "../tests/dxvk/vulkan/dx7_harness.h"
#include "tess-perf-spv.h"

#define DRAWS 256
#define PATCHES 16
#define VERTS (PATCHES * 4)

static VkPhysicalDeviceFeatures want;

static void
device_hook(struct dx7 *t, VkDeviceCreateInfo *dci)
{
   memset(&want, 0, sizeof(want));
   want.tessellationShader = t->feats.tessellationShader;
   want.geometryShader = t->feats.geometryShader;
   dci->pEnabledFeatures = &want;
}

static VkPipeline
make_pipe(struct dx7 *t, VkPipelineLayout layout, int ref, int gs)
{
   struct dx7_pipe_desc d = {
      .vs = ref ? tp_vs_ref : tp_vs,
      .vs_size = ref ? sizeof(tp_vs_ref) : sizeof(tp_vs),
      .fs = tp_fs,
      .fs_size = sizeof(tp_fs),
      .gs = gs ? tp_gs : NULL,
      .gs_size = gs ? sizeof(tp_gs) : 0,
      .tcs = ref ? NULL : tp_tcs,
      .tcs_size = ref ? 0 : sizeof(tp_tcs),
      .tes = ref ? NULL : tp_tes,
      .tes_size = ref ? 0 : sizeof(tp_tes),
      .patch_control_points = 4,
      .topology = ref ? VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
                      : VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
      .polygon_mode = VK_POLYGON_MODE_FILL,
      .cull_mode = VK_CULL_MODE_NONE,
   };
   VkPipelineLayout saved = t->layout;
   t->layout = layout;
   VkPipeline p = dx7_pipeline(t, &d);
   t->layout = saved;
   return p;
}

struct cfg {
   const char *name;
   int ref;
   float level;
   int mix;
};

static const struct cfg cfgs[] = {
   {"ref", 1, 0, 0},
   {"1", 0, 1, 0},
   {"4", 0, 4, 0},
   {"16", 0, 16, 0},
   {"mix", 0, 8, 1},
};
#define NCFG (sizeof(cfgs) / sizeof(cfgs[0]))

static void
record(struct dx7 *t, VkCommandBuffer cmd, VkPipeline pipe,
       VkPipelineLayout layout, VkBuffer vb, const struct cfg *c)
{
   VkCommandBufferBeginInfo bbi = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
   CK(vkResetCommandBuffer(cmd, 0), "ResetCmd");
   CK(vkBeginCommandBuffer(cmd, &bbi), "Begin");
   VkClearValue clear = {.color = {{0.0f, 0.0f, 1.0f, 1.0f}}};
   VkRenderPassBeginInfo rpbi = {.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                                 .renderPass = t->rp,
                                 .framebuffer = t->fb,
                                 .renderArea = {{0, 0}, {RT_W, RT_H}},
                                 .clearValueCount = 1,
                                 .pClearValues = &clear};
   vkCmdBeginRenderPass(cmd, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
   vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe);
   VkDeviceSize off = 0;
   vkCmdBindVertexBuffers(cmd, 0, 1, &vb, &off);
   for (uint32_t i = 0; i < DRAWS; i++) {
      float pc[4] = {c->level, c->mix ? 1.0f : 0.0f,
                     0.01f * (float)((int)(i % 8) - 4),
                     0.01f * (float)((int)((i / 8) % 8) - 4)};
      vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_ALL_GRAPHICS, 0,
                         sizeof(pc), pc);
      /* ref: the 64 control points as 21 triangles. */
      vkCmdDraw(cmd, c->ref ? 63 : VERTS, 1, 0, 0);
   }
   vkCmdEndRenderPass(cmd);
   VkImageMemoryBarrier bar = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                               .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                               .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                               .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                               .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                               .image = t->img,
                               .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT,
                                                    0, 1, 0, 1}};
   vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1,
                        &bar);
   VkBufferImageCopy region = {.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT,
                                                    0, 0, 1},
                               .imageExtent = {RT_W, RT_H, 1}};
   vkCmdCopyImageToBuffer(cmd, t->img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                          t->rbuf, 1, &region);
   CK(vkEndCommandBuffer(cmd), "End");
}

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

/* Applies one variant's env list ("default" or "A=1,B=2"); undoes the
 * previous variant's names first. */
static char applied[512];

static void
apply_env(const char *variant)
{
   char buf[512];
   snprintf(buf, sizeof(buf), "%s", applied);
   for (char *s = strtok(buf, ","); s; s = strtok(NULL, ",")) {
      char *eq = strchr(s, '=');
      if (eq)
         *eq = 0;
      unsetenv(s);
   }
   applied[0] = 0;
   if (!strcmp(variant, "default"))
      return;
   snprintf(applied, sizeof(applied), "%s", variant);
   snprintf(buf, sizeof(buf), "%s", variant);
   for (char *s = strtok(buf, ","); s; s = strtok(NULL, ",")) {
      char *eq = strchr(s, '=');
      if (!eq)
         continue;
      *eq = 0;
      setenv(s, eq + 1, 1);
   }
}

#define MAX_VARIANTS 4

int
main(int argc, char **argv)
{
   if (argc < 2) {
      printf("usage: %s <icd>\n", argv[0]);
      return 2;
   }
   const char *ab = getenv("PT_TESS_AB");
   if (!ab)
      ab = "default;PANVK_TESS_FUSE=0,PANVK_TESS_SETUP_STEP=0,"
           "PANVK_TESS_ARENAS=1,PANVK_TESS_WINDING_INLINE=0";
   const char *ms_env = getenv("PT_TESS_MS");
   const double budget_ms = ms_env ? atof(ms_env) : 700.0;

   char abbuf[1024];
   snprintf(abbuf, sizeof(abbuf), "%s", ab);
   char *variants[MAX_VARIANTS];
   int nvar = 0;
   for (char *s = abbuf; s && nvar < MAX_VARIANTS;) {
      char *semi = strchr(s, ';');
      if (semi)
         *semi = 0;
      variants[nvar++] = s;
      s = semi ? semi + 1 : NULL;
   }

   uint32_t hashes[MAX_VARIANTS][2][NCFG];
   int ran[2][NCFG] = {{0}};
   double headline = 0;
   int fails = 0;
   dx7_device_hook = device_hook;

   for (int v = 0; v < nvar; v++) {
      apply_env(variants[v]);
      struct dx7 t;
      dx7_init(&t, argv[1], NULL);
      if (!t.feats.tessellationShader) {
         printf("SKIP tessellationShader not exposed\n");
         return 0;
      }
      printf("variant %d opt=%s device=%s gs=%d\n", v, variants[v],
             t.props.deviceName, t.feats.geometryShader);

      VkPushConstantRange pcr = {VK_SHADER_STAGE_ALL_GRAPHICS, 0, 16};
      VkPipelineLayoutCreateInfo plci = {
         .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
         .pushConstantRangeCount = 1,
         .pPushConstantRanges = &pcr};
      VkPipelineLayout layout;
      CK(vkCreatePipelineLayout(t.dev, &plci, NULL, &layout), "Layout");

      /* 4x4 quad patches over [-0.9, 0.9]^2, corners TL TR BR BL. */
      float verts[VERTS][4];
      for (int p = 0; p < PATCHES; p++) {
         float x0 = -0.9f + 0.45f * (p % 4), y0 = -0.9f + 0.45f * (p / 4);
         float x1 = x0 + 0.45f, y1 = y0 + 0.45f;
         float c[4][2] = {{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}};
         for (int k = 0; k < 4; k++) {
            verts[p * 4 + k][0] = c[k][0];
            verts[p * 4 + k][1] = c[k][1];
            verts[p * 4 + k][2] = 0;
            verts[p * 4 + k][3] = 1;
         }
      }
      VkBuffer vb;
      VkDeviceMemory vbm;
      dx7_buffer(&t, sizeof(verts), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, verts,
                 &vb, &vbm);

      for (int gs = 0; gs < 2; gs++) {
         if (gs && !t.feats.geometryShader)
            continue;
         for (unsigned ci = 0; ci < NCFG; ci++) {
            const struct cfg *c = &cfgs[ci];
            if (gs && c->ref)
               continue;
            VkPipeline pipe = make_pipe(&t, layout, c->ref, gs);
            record(&t, t.cmd, pipe, layout, vb, c);

            submit_wait(&t, t.cmd); /* warm-up, and the checked image */
            hashes[v][gs][ci] = hash_px(t.px);
            submit_wait(&t, t.cmd);

            long frames = 0;
            double start = now_ms(), el = 0;
            do {
               submit_wait(&t, t.cmd);
               frames++;
               el = now_ms() - start;
            } while (el < budget_ms || frames < 3);

            const double ms = el / frames;
            printf("BENCH tess_perf opt=%s gs=%d L=%s draws=%d patches=%d "
                   "ms=%.2f us_draw=%.1f frames=%ld hash=%08x\n",
                   variants[v], gs, c->name, DRAWS, DRAWS * PATCHES, ms,
                   ms * 1000.0 / DRAWS, frames, hashes[v][gs][ci]);
            if (v == 0 && !gs && c->mix)
               headline = 1000.0 / ms;
            if (v == 0)
               ran[gs][ci] = 1;
            else if (ran[gs][ci] && hashes[v][gs][ci] != hashes[0][gs][ci]) {
               printf("MISMATCH opt=%s gs=%d L=%s hash=%08x vs %08x\n",
                      variants[v], gs, c->name, hashes[v][gs][ci],
                      hashes[0][gs][ci]);
               fails++;
            }
            vkDestroyPipeline(t.dev, pipe, NULL);
         }
      }
      /* The device is left alive: the harness has no teardown, and a new
       * one is created for the next variant. */
   }

   if (headline > 0)
      printf("FPS %.1f (opt=%s gs=0 L=mix frame)\n", headline, variants[0]);
   printf("TESS_PERF_FAILS=%d\n", fails);
   printf("%s tess_perf\n", fails ? "FAIL" : "PASS");
   return fails ? 1 : 0;
}
