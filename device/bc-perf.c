/* BCn emulation benchmark (bc-emul-perf).
 *
 * Direct ICD load. For every BC format plus uncompressed references:
 *   MEM     vkGetImageMemoryRequirements of a SIZExSIZE full-mip image
 *   UPLOAD  GPU time of vkCmdCopyBufferToImage of the whole chain (this is
 *           where the emulated driver decodes), best of 3
 *   A/B/C   GPU time of one 1920x1080 fullscreen pass, 4 taps per pixel:
 *           A 1:1 axis aligned bilinear, B 1:1 transposed (walks texture
 *           columns), C 4x minified + 30 deg rotation, trilinear. Median of 5
 *           submits, each 16 additive-blended passes in one render pass
 *           (GPU timestamps and submit-to-fence wall time).
 *           D 2x minified at LOD 0, one tap; E 3x minified + 30 deg at LOD 0,
 *           one tap: both bound by texture bandwidth, not filtering.
 * Content is a procedural image run through tiny block encoders so that
 * lossless framebuffer compression sees realistic data.
 *
 * A probe section lists optimal features, DRM modifiers and fixed-rate
 * compression flags for the formats the emulation could decode to.
 *
 * usage: bc-perf <icd.so> [format-substring]
 * env:   BCPERF_SIZE (default 2048)
 * Output: "PROBE ..." and "PERF ..." lines, then "RESULT DONE".
 */
#include <dlfcn.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include "bc-perf-spv.h"

typedef PFN_vkVoidFunction (*icd_gipa_fn)(VkInstance, const char *);

#define CK(expr, what)                                                         \
   do {                                                                        \
      VkResult _r = (expr);                                                    \
      if (_r != VK_SUCCESS) {                                                  \
         printf("FAIL %s r=%d line=%d\n", what, _r, __LINE__);                 \
         exit(1);                                                              \
      }                                                                        \
   } while (0)

#define MIN2(a, b) ((a) < (b) ? (a) : (b))
#define RT_W 1920
#define RT_H 1080
#define PASSES 16
#define RUNS 5
#define NMODES 5

static const struct {
   VkFormat f;
   const char *name;
} fmts[] = {
   {VK_FORMAT_BC1_RGB_UNORM_BLOCK, "BC1_RGB_UNORM"},
   {VK_FORMAT_BC1_RGBA_SRGB_BLOCK, "BC1_RGBA_SRGB"},
   {VK_FORMAT_BC2_UNORM_BLOCK, "BC2_UNORM"},
   {VK_FORMAT_BC3_UNORM_BLOCK, "BC3_UNORM"},
   {VK_FORMAT_BC3_SRGB_BLOCK, "BC3_SRGB"},
   {VK_FORMAT_BC4_UNORM_BLOCK, "BC4_UNORM"},
   {VK_FORMAT_BC4_SNORM_BLOCK, "BC4_SNORM"},
   {VK_FORMAT_BC5_UNORM_BLOCK, "BC5_UNORM"},
   {VK_FORMAT_BC5_SNORM_BLOCK, "BC5_SNORM"},
   {VK_FORMAT_BC6H_UFLOAT_BLOCK, "BC6H_UFLOAT"},
   {VK_FORMAT_BC6H_SFLOAT_BLOCK, "BC6H_SFLOAT"},
   {VK_FORMAT_BC7_UNORM_BLOCK, "BC7_UNORM"},
   {VK_FORMAT_BC7_SRGB_BLOCK, "BC7_SRGB"},
   /* uncompressed references (native upload, no decode) */
   {VK_FORMAT_R8G8B8A8_UNORM, "REF_RGBA8"},
   {VK_FORMAT_R8G8_UNORM, "REF_RG8"},
   {VK_FORMAT_R8_UNORM, "REF_R8"},
   {VK_FORMAT_R16G16B16A16_SFLOAT, "REF_RGBA16F"},
   {VK_FORMAT_B10G11R11_UFLOAT_PACK32, "REF_B10G11R11"},
   {VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK, "REF_ETC2_RGBA8"},
   {VK_FORMAT_ASTC_4x4_UNORM_BLOCK, "REF_ASTC4x4"},
};

static const struct {
   VkFormat f;
   const char *name;
} probe_fmts[] = {
   {VK_FORMAT_R8G8B8A8_UNORM, "R8G8B8A8_UNORM"},
   {VK_FORMAT_R8G8B8A8_SRGB, "R8G8B8A8_SRGB"},
   {VK_FORMAT_R5G6B5_UNORM_PACK16, "R5G6B5_UNORM"},
   {VK_FORMAT_R8_UNORM, "R8_UNORM"},
   {VK_FORMAT_R8_SNORM, "R8_SNORM"},
   {VK_FORMAT_R8G8_UNORM, "R8G8_UNORM"},
   {VK_FORMAT_R8G8_SNORM, "R8G8_SNORM"},
   {VK_FORMAT_R16G16_UNORM, "R16G16_UNORM"},
   {VK_FORMAT_R16G16B16A16_SFLOAT, "R16G16B16A16_SFLOAT"},
   {VK_FORMAT_B10G11R11_UFLOAT_PACK32, "B10G11R11_UFLOAT"},
   {VK_FORMAT_E5B9G9R9_UFLOAT_PACK32, "E5B9G9R9_UFLOAT"},
   {VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK, "ETC2_RGB8"},
   {VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK, "ETC2_RGBA8"},
   {VK_FORMAT_EAC_R11_UNORM_BLOCK, "EAC_R11"},
   {VK_FORMAT_EAC_R11G11_UNORM_BLOCK, "EAC_RG11"},
   {VK_FORMAT_ASTC_4x4_UNORM_BLOCK, "ASTC_4x4_UNORM"},
   {VK_FORMAT_ASTC_8x8_UNORM_BLOCK, "ASTC_8x8_UNORM"},
   {VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK, "ASTC_4x4_SFLOAT"},
   {VK_FORMAT_BC1_RGB_UNORM_BLOCK, "BC1_RGB_UNORM"},
   {VK_FORMAT_BC7_UNORM_BLOCK, "BC7_UNORM"},
};

static icd_gipa_fn gipa;
static VkInstance inst;
static VkPhysicalDevice phys;
static VkDevice dev;
static uint32_t host_mi, dev_mi;

#define PFN(name) static PFN_vk##name vk##name;
#define DEV_FUNCS(X)                                                           \
   X(CreateBuffer) X(GetBufferMemoryRequirements) X(AllocateMemory)            \
   X(BindBufferMemory) X(MapMemory) X(CreateImage)                             \
   X(GetImageMemoryRequirements) X(BindImageMemory) X(CreateImageView)         \
   X(CreateShaderModule) X(CreatePipelineLayout) X(CreateGraphicsPipelines)    \
   X(CreateDescriptorSetLayout) X(CreateDescriptorPool)                        \
   X(AllocateDescriptorSets) X(UpdateDescriptorSets) X(CreateSampler)          \
   X(CreateCommandPool) X(AllocateCommandBuffers) X(BeginCommandBuffer)        \
   X(EndCommandBuffer) X(CmdBindPipeline) X(CmdBindDescriptorSets)             \
   X(CmdPushConstants) X(CmdDraw) X(CmdPipelineBarrier)                        \
   X(CmdCopyBufferToImage) X(CreateFence) X(QueueSubmit) X(WaitForFences)      \
   X(ResetFences) X(ResetCommandBuffer) X(GetDeviceQueue)                      \
   X(DestroyImage) X(DestroyImageView) X(FreeMemory) X(DestroyBuffer)          \
   X(ResetDescriptorPool) X(CreateRenderPass) X(CreateFramebuffer)             \
   X(CmdBeginRenderPass) X(CmdEndRenderPass) X(CreateQueryPool)                \
   X(CmdResetQueryPool) X(CmdWriteTimestamp) X(GetQueryPoolResults)            \
   X(CmdSetViewport) X(CmdSetScissor) X(CmdBlitImage) X(CmdCopyImageToBuffer)
DEV_FUNCS(PFN)

static uint32_t size_px = 2048;
/* BCPERF_SRC: raw RGBA8 size_px x size_px image used instead of the
 * procedural content (for quality checks on real pictures). */
static uint8_t *src_rgba;
static double ts_period_ns = 1.0;

/* ---------------------------------------------------------------- content */

static uint32_t rng = 0x12345678u;
static uint32_t
lcg(void)
{
   rng = rng * 1664525u + 1013904223u;
   return rng >> 8;
}

/* Smooth bands, rings and a little noise; values 0..1. */
static void
texel(uint32_t x, uint32_t y, uint32_t w, float c[4])
{
   if (src_rgba) {
      const uint8_t *p =
         src_rgba + 4 * ((size_t)(y * (size_px / w)) * size_px + x * (size_px / w));
      for (int i = 0; i < 4; i++)
         c[i] = p[i] / 255.0f;
      return;
   }
   const float u = (float)x / w, v = (float)y / w;
   const float r = sqrtf((u - 0.5f) * (u - 0.5f) + (v - 0.4f) * (v - 0.4f));
   const float n = (float)(lcg() & 255) / 255.0f * 0.06f;
   c[0] = 0.5f + 0.45f * sinf(u * 18.0f + v * 3.0f) + n;
   c[1] = 0.5f + 0.45f * cosf(r * 40.0f) - n;
   c[2] = 0.3f + 0.6f * v * u + n;
   c[3] = 0.6f + 0.35f * sinf(v * 11.0f);
   for (int i = 0; i < 4; i++)
      c[i] = c[i] < 0.0f ? 0.0f : c[i] > 1.0f ? 1.0f : c[i];
}

static uint16_t
f2h(float f)
{
   union { float f; uint32_t u; } v = {f};
   uint32_t e = (v.u >> 23) & 0xff, m = v.u & 0x7fffff;
   uint32_t s = (v.u >> 16) & 0x8000;
   int ne = (int)e - 127 + 15;
   if (ne <= 0)
      return s;
   if (ne >= 31)
      return s | 0x7bff;
   return s | (ne << 10) | (m >> 13);
}

static uint32_t
pack565(const float c[3])
{
   return ((uint32_t)(c[0] * 31.0f + 0.5f) << 11) |
          ((uint32_t)(c[1] * 63.0f + 0.5f) << 5) |
          (uint32_t)(c[2] * 31.0f + 0.5f);
}

static void
unpack565(uint32_t p, float c[3])
{
   c[0] = ((p >> 11) & 31) / 31.0f;
   c[1] = ((p >> 5) & 63) / 63.0f;
   c[2] = (p & 31) / 31.0f;
}

/* BC1 colour block (always 4-colour mode). */
static void
enc_bc1(float px[16][4], uint8_t *o)
{
   float lo[3] = {1, 1, 1}, hi[3] = {0, 0, 0};
   for (int t = 0; t < 16; t++)
      for (int c = 0; c < 3; c++) {
         lo[c] = fminf(lo[c], px[t][c]);
         hi[c] = fmaxf(hi[c], px[t][c]);
      }
   uint32_t c0 = pack565(hi), c1 = pack565(lo);
   if (c0 < c1) {
      uint32_t t = c0;
      c0 = c1;
      c1 = t;
   }
   uint32_t idx = 0;
   if (c0 != c1) {
      float e0[3], e1[3], pal[4][3];
      unpack565(c0, e0);
      unpack565(c1, e1);
      for (int c = 0; c < 3; c++) {
         pal[0][c] = e0[c];
         pal[1][c] = e1[c];
         pal[2][c] = (2 * e0[c] + e1[c]) / 3;
         pal[3][c] = (e0[c] + 2 * e1[c]) / 3;
      }
      for (int t = 0; t < 16; t++) {
         int best = 0;
         float bd = 1e9f;
         for (int k = 0; k < 4; k++) {
            float d = 0;
            for (int c = 0; c < 3; c++)
               d += (px[t][c] - pal[k][c]) * (px[t][c] - pal[k][c]);
            if (d < bd) {
               bd = d;
               best = k;
            }
         }
         idx |= (uint32_t)best << (2 * t);
      }
   }
   o[0] = c0;
   o[1] = c0 >> 8;
   o[2] = c1;
   o[3] = c1 >> 8;
   memcpy(o + 4, &idx, 4);
}

/* BC4 channel block. snorm stores -127..127. */
static void
enc_bc4(float px[16][4], int ch, int snorm, uint8_t *o)
{
   int v[16], lo = 255, hi = -255;
   for (int t = 0; t < 16; t++) {
      float f = px[t][ch];
      v[t] = snorm ? (int)lrintf((f * 2.0f - 1.0f) * 127.0f)
                   : (int)lrintf(f * 255.0f);
      lo = v[t] < lo ? v[t] : lo;
      hi = v[t] > hi ? v[t] : hi;
   }
   uint64_t bits = 0;
   if (hi != lo) {
      for (int t = 0; t < 16; t++) {
         /* palette index: 0=hi, 1=lo, 2..7 interpolated hi->lo */
         int k = (int)lrintf((float)(hi - v[t]) * 7.0f / (hi - lo));
         int idx = k == 0 ? 0 : k == 7 ? 1 : k + 1;
         bits |= (uint64_t)idx << (3 * t);
      }
   }
   o[0] = (uint8_t)hi;
   o[1] = (uint8_t)lo;
   for (int i = 0; i < 6; i++)
      o[2 + i] = bits >> (8 * i);
}

static void
put_bits(uint8_t *o, unsigned *pos, uint32_t v, unsigned n)
{
   for (unsigned i = 0; i < n; i++, (*pos)++)
      if (v & (1u << i))
         o[*pos >> 3] |= 1u << (*pos & 7);
}

/* Project onto the e0->e1 line, return 4-bit indices; fix the anchor. */
static void
line_indices(float px[16][4], int nch, float *e0, float *e1, int idx[16])
{
   float d[4], dd = 0;
   for (int c = 0; c < nch; c++) {
      d[c] = e1[c] - e0[c];
      dd += d[c] * d[c];
   }
   for (int t = 0; t < 16; t++) {
      float p = 0;
      for (int c = 0; c < nch; c++)
         p += (px[t][c] - e0[c]) * d[c];
      int k = dd > 0 ? (int)lrintf(p / dd * 15.0f) : 0;
      idx[t] = k < 0 ? 0 : k > 15 ? 15 : k;
   }
}

/* BC7 mode 6: RGBA 7.7.7.7 + p-bit, one subset, 4-bit indices. */
static void
enc_bc7(float px[16][4], uint8_t *o)
{
   float lo[4] = {1, 1, 1, 1}, hi[4] = {0, 0, 0, 0};
   for (int t = 0; t < 16; t++)
      for (int c = 0; c < 4; c++) {
         lo[c] = fminf(lo[c], px[t][c]);
         hi[c] = fmaxf(hi[c], px[t][c]);
      }
   int idx[16];
   line_indices(px, 4, lo, hi, idx);
   if (idx[0] & 8) {
      float t[4];
      memcpy(t, lo, sizeof(t));
      memcpy(lo, hi, sizeof(t));
      memcpy(hi, t, sizeof(t));
      for (int i = 0; i < 16; i++)
         idx[i] = 15 - idx[i];
   }
   memset(o, 0, 16);
   unsigned pos = 0;
   put_bits(o, &pos, 1u << 6, 7);
   for (int c = 0; c < 4; c++) {
      put_bits(o, &pos, (uint32_t)lrintf(lo[c] * 127.0f), 7);
      put_bits(o, &pos, (uint32_t)lrintf(hi[c] * 127.0f), 7);
   }
   put_bits(o, &pos, 0, 2);
   put_bits(o, &pos, idx[0], 3);
   for (int t = 1; t < 16; t++)
      put_bits(o, &pos, idx[t], 4);
}

/* BC6H mode 11 (10-bit endpoints, one region, 4-bit indices). */
static void
enc_bc6(float px[16][4], int is_signed, uint8_t *o)
{
   float q[16][4], lo[3] = {1e9f, 1e9f, 1e9f}, hi[3] = {0, 0, 0};
   const float div = is_signed ? 62.0f : 31.0f;
   const float qmax = is_signed ? 511.0f : 1023.0f;
   for (int t = 0; t < 16; t++)
      for (int c = 0; c < 3; c++) {
         float v = f2h(px[t][c] * 4.0f) / div;
         q[t][c] = v > qmax ? qmax : v;
         lo[c] = fminf(lo[c], q[t][c]);
         hi[c] = fmaxf(hi[c], q[t][c]);
      }
   int idx[16];
   line_indices(q, 3, lo, hi, idx);
   if (idx[0] & 8) {
      float t[3];
      memcpy(t, lo, sizeof(t));
      memcpy(lo, hi, sizeof(t));
      memcpy(hi, t, sizeof(t));
      for (int i = 0; i < 16; i++)
         idx[i] = 15 - idx[i];
   }
   memset(o, 0, 16);
   unsigned pos = 0;
   put_bits(o, &pos, 0x03, 5);
   for (int c = 0; c < 3; c++)
      put_bits(o, &pos, (uint32_t)lrintf(lo[c]), 10);
   for (int c = 0; c < 3; c++)
      put_bits(o, &pos, (uint32_t)lrintf(hi[c]), 10);
   put_bits(o, &pos, idx[0], 3);
   for (int t = 1; t < 16; t++)
      put_bits(o, &pos, idx[t], 4);
}

static int
is_bc(VkFormat f)
{
   return f >= VK_FORMAT_BC1_RGB_UNORM_BLOCK && f <= VK_FORMAT_BC7_SRGB_BLOCK;
}

static uint32_t
block_B(VkFormat f)
{
   if (f <= VK_FORMAT_BC1_RGBA_SRGB_BLOCK || f == VK_FORMAT_BC4_UNORM_BLOCK ||
       f == VK_FORMAT_BC4_SNORM_BLOCK)
      return 8;
   return 16;
}

/* Bytes per block (BC/ETC2/ASTC: 4x4 block) or per texel (uncompressed). */
static uint32_t
unit_B(VkFormat f, int *blocked)
{
   *blocked = 1;
   if (is_bc(f))
      return block_B(f);
   if (f == VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK ||
       f == VK_FORMAT_ASTC_4x4_UNORM_BLOCK)
      return 16;
   *blocked = 0;
   switch (f) {
   case VK_FORMAT_R8_UNORM:
      return 1;
   case VK_FORMAT_R8G8_UNORM:
      return 2;
   case VK_FORMAT_R16G16B16A16_SFLOAT:
      return 8;
   default:
      return 4;
   }
}

static uint32_t
pack_b10g11r11(const float c[3])
{
   /* positive, < 65504: take the half bits and drop mantissa */
   uint32_t r = f2h(c[0] * 4.0f) >> 4, g = f2h(c[1] * 4.0f) >> 4,
            b = f2h(c[2] * 4.0f) >> 5;
   return r | (g << 11) | (b << 22);
}

/* Encode one mip level of the procedural image into dst. */
static void
encode_level(VkFormat f, uint32_t w, uint32_t h, uint8_t *dst)
{
   int blocked;
   const uint32_t ub = unit_B(f, &blocked);
   if (!blocked) {
      for (uint32_t y = 0; y < h; y++)
         for (uint32_t x = 0; x < w; x++) {
            float c[4];
            texel(x, y, w, c);
            uint8_t *o = dst + (y * w + x) * ub;
            if (f == VK_FORMAT_R16G16B16A16_SFLOAT) {
               uint16_t hv[4] = {f2h(c[0] * 4), f2h(c[1] * 4), f2h(c[2] * 4),
                                 f2h(c[3])};
               memcpy(o, hv, 8);
            } else if (f == VK_FORMAT_B10G11R11_UFLOAT_PACK32) {
               uint32_t p = pack_b10g11r11(c);
               memcpy(o, &p, 4);
            } else {
               for (uint32_t k = 0; k < ub; k++)
                  o[k] = (uint8_t)lrintf(c[k] * 255.0f);
            }
         }
      return;
   }
   const uint32_t bw = (w + 3) / 4, bh = (h + 3) / 4;
   for (uint32_t by = 0; by < bh; by++)
      for (uint32_t bx = 0; bx < bw; bx++) {
         float px[16][4];
         for (int t = 0; t < 16; t++)
            texel(MIN2(bx * 4 + t % 4, w - 1), MIN2(by * 4 + t / 4, h - 1), w,
                  px[t]);
         uint8_t *o = dst + (by * bw + bx) * ub;
         memset(o, 0, ub);
         switch (f) {
         case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
         case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
         case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
         case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
            enc_bc1(px, o);
            break;
         case VK_FORMAT_BC2_UNORM_BLOCK:
         case VK_FORMAT_BC2_SRGB_BLOCK:
            for (int t = 0; t < 16; t++)
               o[t / 2] |= (uint8_t)lrintf(px[t][3] * 15.0f) << (4 * (t & 1));
            enc_bc1(px, o + 8);
            break;
         case VK_FORMAT_BC3_UNORM_BLOCK:
         case VK_FORMAT_BC3_SRGB_BLOCK:
            enc_bc4(px, 3, 0, o);
            enc_bc1(px, o + 8);
            break;
         case VK_FORMAT_BC4_UNORM_BLOCK:
         case VK_FORMAT_BC4_SNORM_BLOCK:
            enc_bc4(px, 0, f == VK_FORMAT_BC4_SNORM_BLOCK, o);
            break;
         case VK_FORMAT_BC5_UNORM_BLOCK:
         case VK_FORMAT_BC5_SNORM_BLOCK:
            enc_bc4(px, 0, f == VK_FORMAT_BC5_SNORM_BLOCK, o);
            enc_bc4(px, 1, f == VK_FORMAT_BC5_SNORM_BLOCK, o + 8);
            break;
         case VK_FORMAT_BC6H_UFLOAT_BLOCK:
         case VK_FORMAT_BC6H_SFLOAT_BLOCK:
            enc_bc6(px, f == VK_FORMAT_BC6H_SFLOAT_BLOCK, o);
            break;
         case VK_FORMAT_BC7_UNORM_BLOCK:
         case VK_FORMAT_BC7_SRGB_BLOCK:
            enc_bc7(px, o);
            break;
         default:
            /* ETC2/ASTC: random payload is fine for sampling cost. */
            for (uint32_t k = 0; k < ub; k++)
               o[k] = lcg();
            break;
         }
      }
}

/* ---------------------------------------------------------------- vulkan */

struct buf {
   VkBuffer b;
   VkDeviceMemory m;
   void *p;
};

static uint32_t
pick_mem(uint32_t bits, uint32_t want)
{
   if (bits & (1u << want))
      return want;
   for (uint32_t i = 0; i < 32; i++)
      if (bits & (1u << i))
         return i;
   return 0;
}

static struct buf
mkbuf(VkDeviceSize size, VkBufferUsageFlags usage)
{
   struct buf r = {0};
   VkBufferCreateInfo ci = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                            .size = size,
                            .usage = usage};
   CK(vkCreateBuffer(dev, &ci, NULL, &r.b), "CreateBuffer");
   VkMemoryRequirements mr;
   vkGetBufferMemoryRequirements(dev, r.b, &mr);
   VkMemoryAllocateInfo ai = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                              .allocationSize = mr.size,
                              .memoryTypeIndex = pick_mem(mr.memoryTypeBits, host_mi)};
   CK(vkAllocateMemory(dev, &ai, NULL, &r.m), "AllocBuf");
   CK(vkBindBufferMemory(dev, r.b, r.m, 0), "BindBuf");
   CK(vkMapMemory(dev, r.m, 0, VK_WHOLE_SIZE, 0, &r.p), "Map");
   return r;
}

static void
freebuf(struct buf *b)
{
   vkDestroyBuffer(dev, b->b, NULL);
   vkFreeMemory(dev, b->m, NULL);
}

static VkCommandBuffer cb;
static VkQueue q;
static VkFence fence;
static VkQueryPool qp;

static void
begin(void)
{
   VkCommandBufferBeginInfo bi = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                  .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
   CK(vkResetCommandBuffer(cb, 0), "ResetCB");
   CK(vkBeginCommandBuffer(cb, &bi), "Begin");
   vkCmdResetQueryPool(cb, qp, 0, 2);
}

static double
submit(void)
{
   CK(vkEndCommandBuffer(cb), "End");
   VkSubmitInfo si = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                      .commandBufferCount = 1,
                      .pCommandBuffers = &cb};
   CK(vkResetFences(dev, 1, &fence), "ResetFence");
   CK(vkQueueSubmit(q, 1, &si, fence), "Submit");
   CK(vkWaitForFences(dev, 1, &fence, VK_TRUE, 20000000000ull), "Wait");
   uint64_t t[2];
   CK(vkGetQueryPoolResults(dev, qp, 0, 2, sizeof(t), t, 8,
                            VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT),
      "Query");
   return (double)(t[1] - t[0]) * ts_period_ns / 1e6;
}

static void
img_barrier(VkImage img, VkImageLayout from, VkImageLayout to,
            VkAccessFlags src, VkAccessFlags dst, VkPipelineStageFlags ss,
            VkPipelineStageFlags ds)
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
                                                  VK_REMAINING_MIP_LEVELS, 0, 1}};
   vkCmdPipelineBarrier(cb, ss, ds, 0, 0, NULL, 0, NULL, 1, &b);
}

static int
cmp_d(const void *a, const void *b)
{
   double x = *(const double *)a, y = *(const double *)b;
   return x < y ? -1 : x > y;
}

static void
probe(void)
{
   PFN_vkGetPhysicalDeviceFormatProperties2 gpfp2 =
      (PFN_vkGetPhysicalDeviceFormatProperties2)gipa(
         inst, "vkGetPhysicalDeviceFormatProperties2");
   PFN_vkGetPhysicalDeviceImageFormatProperties2 gpifp2 =
      (PFN_vkGetPhysicalDeviceImageFormatProperties2)gipa(
         inst, "vkGetPhysicalDeviceImageFormatProperties2");
   for (unsigned i = 0; i < sizeof(probe_fmts) / sizeof(probe_fmts[0]); i++) {
      VkDrmFormatModifierPropertiesEXT mods[32];
      VkDrmFormatModifierPropertiesListEXT ml = {
         .sType = VK_STRUCTURE_TYPE_DRM_FORMAT_MODIFIER_PROPERTIES_LIST_EXT,
         .drmFormatModifierCount = 32,
         .pDrmFormatModifierProperties = mods};
      VkFormatProperties2 fp = {.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2,
                                .pNext = &ml};
      gpfp2(phys, probe_fmts[i].f, &fp);
      VkImageCompressionPropertiesEXT cp = {
         .sType = VK_STRUCTURE_TYPE_IMAGE_COMPRESSION_PROPERTIES_EXT};
      VkImageCompressionControlEXT cc = {
         .sType = VK_STRUCTURE_TYPE_IMAGE_COMPRESSION_CONTROL_EXT,
         .flags = VK_IMAGE_COMPRESSION_FIXED_RATE_DEFAULT_EXT};
      VkPhysicalDeviceImageFormatInfo2 ifi = {
         .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2,
         .pNext = &cc,
         .format = probe_fmts[i].f,
         .type = VK_IMAGE_TYPE_2D,
         .tiling = VK_IMAGE_TILING_OPTIMAL,
         .usage = VK_IMAGE_USAGE_SAMPLED_BIT |
                  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT};
      VkImageFormatProperties2 ifp = {
         .sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2, .pNext = &cp};
      VkResult r = gpifp2(phys, &ifi, &ifp);
      printf("PROBE %s optimal=0x%x mods=%u fixedrate(r=%d flags=0x%x "
             "rates=0x%x)",
             probe_fmts[i].name, fp.formatProperties.optimalTilingFeatures,
             ml.drmFormatModifierCount, r, cp.imageCompressionFlags,
             cp.imageCompressionFixedRateFlags);
      for (uint32_t k = 0; k < ml.drmFormatModifierCount && k < 32; k++)
         printf(" %llx", (unsigned long long)mods[k].drmFormatModifier);
      printf("\n");
   }
}

int
main(int argc, char **argv)
{
   if (argc < 2) {
      printf("usage: %s <icd.so> [format-substring]\n", argv[0]);
      return 2;
   }
   setvbuf(stdout, NULL, _IONBF, 0);
   if (getenv("BCPERF_SIZE"))
      size_px = atoi(getenv("BCPERF_SIZE"));
   if (getenv("BCPERF_SRC")) {
      FILE *sf = fopen(getenv("BCPERF_SRC"), "rb");
      src_rgba = malloc((size_t)size_px * size_px * 4);
      if (!sf || fread(src_rgba, 4, (size_t)size_px * size_px, sf) !=
                    (size_t)size_px * size_px) {
         printf("FAIL BCPERF_SRC needs %ux%u RGBA8\n", size_px, size_px);
         return 1;
      }
      fclose(sf);
   }
   const char *dump_dir = getenv("BCPERF_DUMP");
   const char *filter = argc > 2 ? argv[2] : NULL;

   void *h = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
   if (!h) {
      printf("FAIL dlopen %s\n", dlerror());
      return 1;
   }
   gipa = (icd_gipa_fn)dlsym(h, "vk_icdGetInstanceProcAddr");
   if (!gipa) {
      printf("FAIL no vk_icdGetInstanceProcAddr\n");
      return 1;
   }
   PFN_vkCreateInstance vkCreateInstance =
      (PFN_vkCreateInstance)gipa(NULL, "vkCreateInstance");
   VkApplicationInfo app = {.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                            .apiVersion = VK_API_VERSION_1_3};
   VkInstanceCreateInfo ici = {.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                               .pApplicationInfo = &app};
   CK(vkCreateInstance(&ici, NULL, &inst), "CreateInstance");
#define IG(name) PFN_vk##name vk##name = (PFN_vk##name)gipa(inst, "vk" #name);
   IG(EnumeratePhysicalDevices)
   IG(GetPhysicalDeviceProperties)
   IG(GetPhysicalDeviceFeatures)
   IG(GetPhysicalDeviceFormatProperties)
   IG(GetPhysicalDeviceMemoryProperties)
   IG(GetPhysicalDeviceQueueFamilyProperties)
   IG(CreateDevice)
   IG(GetDeviceProcAddr)

   uint32_t n = 8;
   VkPhysicalDevice devs[8];
   VkResult er = vkEnumeratePhysicalDevices(inst, &n, devs);
   if ((er != VK_SUCCESS && er != VK_INCOMPLETE) || n == 0) {
      printf("FAIL no physical device\n");
      return 1;
   }
   phys = devs[0];
   VkPhysicalDeviceProperties props;
   vkGetPhysicalDeviceProperties(phys, &props);
   VkPhysicalDeviceFeatures feats;
   vkGetPhysicalDeviceFeatures(phys, &feats);
   ts_period_ns = props.limits.timestampPeriod;
   VkQueueFamilyProperties qfp[4];
   uint32_t nq = 4;
   vkGetPhysicalDeviceQueueFamilyProperties(phys, &nq, qfp);
   printf("DEVICE %s driver=0x%x bc=%d etc2=%d astc=%d ts_bits=%u "
          "ts_period=%.3f size=%u\n",
          props.deviceName, props.driverVersion, feats.textureCompressionBC,
          feats.textureCompressionETC2, feats.textureCompressionASTC_LDR,
          qfp[0].timestampValidBits, ts_period_ns, size_px);
   probe();

   VkPhysicalDeviceMemoryProperties mp;
   vkGetPhysicalDeviceMemoryProperties(phys, &mp);
   host_mi = dev_mi = ~0u;
   for (uint32_t i = 0; i < mp.memoryTypeCount; i++) {
      VkMemoryPropertyFlags f = mp.memoryTypes[i].propertyFlags;
      if (host_mi == ~0u && (f & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
          (f & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
         host_mi = i;
      if (dev_mi == ~0u && (f & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
         dev_mi = i;
   }

   float prio = 1.0f;
   VkDeviceQueueCreateInfo qci = {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                  .queueCount = 1,
                                  .pQueuePriorities = &prio};
   VkPhysicalDeviceFeatures en = {
      .textureCompressionBC = feats.textureCompressionBC,
      .textureCompressionETC2 = feats.textureCompressionETC2,
      .textureCompressionASTC_LDR = feats.textureCompressionASTC_LDR};
   VkDeviceCreateInfo dci = {.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                             .queueCreateInfoCount = 1,
                             .pQueueCreateInfos = &qci,
                             .pEnabledFeatures = &en};
   CK(vkCreateDevice(phys, &dci, NULL, &dev), "CreateDevice");
#define DG(name) vk##name = (PFN_vk##name)vkGetDeviceProcAddr(dev, "vk" #name);
   DEV_FUNCS(DG)
   vkGetDeviceQueue(dev, 0, 0, &q);

   /* render target + pipeline */
   VkImageCreateInfo rtci = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                             .imageType = VK_IMAGE_TYPE_2D,
                             .format = VK_FORMAT_R8G8B8A8_UNORM,
                             .extent = {RT_W, RT_H, 1},
                             .mipLevels = 1,
                             .arrayLayers = 1,
                             .samples = VK_SAMPLE_COUNT_1_BIT,
                             .tiling = VK_IMAGE_TILING_OPTIMAL,
                             .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT};
   VkImage rt;
   CK(vkCreateImage(dev, &rtci, NULL, &rt), "RT");
   VkMemoryRequirements mr;
   vkGetImageMemoryRequirements(dev, rt, &mr);
   VkMemoryAllocateInfo mai = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                               .allocationSize = mr.size,
                               .memoryTypeIndex = pick_mem(mr.memoryTypeBits, dev_mi)};
   VkDeviceMemory rtm;
   CK(vkAllocateMemory(dev, &mai, NULL, &rtm), "RTMem");
   CK(vkBindImageMemory(dev, rt, rtm, 0), "RTBind");
   VkImageViewCreateInfo rtvci = {.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                                  .image = rt,
                                  .viewType = VK_IMAGE_VIEW_TYPE_2D,
                                  .format = VK_FORMAT_R8G8B8A8_UNORM,
                                  .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};
   VkImageView rtv;
   CK(vkCreateImageView(dev, &rtvci, NULL, &rtv), "RTView");
   VkAttachmentDescription att = {.format = VK_FORMAT_R8G8B8A8_UNORM,
                                  .samples = VK_SAMPLE_COUNT_1_BIT,
                                  .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                  .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                                  .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                  .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                  .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                                  .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
   VkAttachmentReference ar = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
   VkSubpassDescription sp = {.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                              .colorAttachmentCount = 1,
                              .pColorAttachments = &ar};
   VkRenderPassCreateInfo rpci = {.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                                  .attachmentCount = 1,
                                  .pAttachments = &att,
                                  .subpassCount = 1,
                                  .pSubpasses = &sp};
   VkRenderPass rp;
   CK(vkCreateRenderPass(dev, &rpci, NULL, &rp), "RP");
   VkFramebufferCreateInfo fbci = {.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                                   .renderPass = rp,
                                   .attachmentCount = 1,
                                   .pAttachments = &rtv,
                                   .width = RT_W,
                                   .height = RT_H,
                                   .layers = 1};
   VkFramebuffer fb;
   CK(vkCreateFramebuffer(dev, &fbci, NULL, &fb), "FB");

   VkDescriptorSetLayoutBinding bind = {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                        1, VK_SHADER_STAGE_FRAGMENT_BIT, NULL};
   VkDescriptorSetLayoutCreateInfo dslci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = 1,
      .pBindings = &bind};
   VkDescriptorSetLayout dsl;
   CK(vkCreateDescriptorSetLayout(dev, &dslci, NULL, &dsl), "DSL");
   VkPushConstantRange pcr = {VK_SHADER_STAGE_FRAGMENT_BIT, 0, 28};
   VkPipelineLayoutCreateInfo plci = {.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                                      .setLayoutCount = 1,
                                      .pSetLayouts = &dsl,
                                      .pushConstantRangeCount = 1,
                                      .pPushConstantRanges = &pcr};
   VkPipelineLayout pl;
   CK(vkCreatePipelineLayout(dev, &plci, NULL, &pl), "PL");
   VkShaderModuleCreateInfo vsci = {.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                                    .codeSize = sizeof(bc_perf_vert_spv),
                                    .pCode = bc_perf_vert_spv};
   VkShaderModuleCreateInfo fsci = {.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                                    .codeSize = sizeof(bc_perf_frag_spv),
                                    .pCode = bc_perf_frag_spv};
   VkShaderModule vs, fs;
   CK(vkCreateShaderModule(dev, &vsci, NULL, &vs), "VS");
   CK(vkCreateShaderModule(dev, &fsci, NULL, &fs), "FS");
   VkPipelineShaderStageCreateInfo stages[2] = {
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
       .stage = VK_SHADER_STAGE_VERTEX_BIT, .module = vs, .pName = "main"},
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
       .stage = VK_SHADER_STAGE_FRAGMENT_BIT, .module = fs, .pName = "main"}};
   VkPipelineVertexInputStateCreateInfo vi = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
   VkPipelineInputAssemblyStateCreateInfo ia = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
   VkPipelineViewportStateCreateInfo vps = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .scissorCount = 1};
   VkPipelineRasterizationStateCreateInfo rs = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_NONE,
      .lineWidth = 1.0f};
   VkPipelineMultisampleStateCreateInfo ms = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT};
   /* Additive blend: keeps forward pixel kill from dropping overdrawn passes. */
   VkPipelineColorBlendAttachmentState cba = {
      .blendEnable = VK_TRUE,
      .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
      .dstColorBlendFactor = VK_BLEND_FACTOR_ONE,
      .colorBlendOp = VK_BLEND_OP_ADD,
      .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
      .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
      .alphaBlendOp = VK_BLEND_OP_ADD,
      .colorWriteMask = 0xf};
   VkPipelineColorBlendStateCreateInfo cbs = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .attachmentCount = 1,
      .pAttachments = &cba};
   VkDynamicState dyn[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
   VkPipelineDynamicStateCreateInfo dys = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .dynamicStateCount = 2,
      .pDynamicStates = dyn};
   VkGraphicsPipelineCreateInfo gpci = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .stageCount = 2,
      .pStages = stages,
      .pVertexInputState = &vi,
      .pInputAssemblyState = &ia,
      .pViewportState = &vps,
      .pRasterizationState = &rs,
      .pMultisampleState = &ms,
      .pColorBlendState = &cbs,
      .pDynamicState = &dys,
      .layout = pl,
      .renderPass = rp};
   VkPipeline pipe;
   CK(vkCreateGraphicsPipelines(dev, VK_NULL_HANDLE, 1, &gpci, NULL, &pipe), "GP");

   VkDescriptorPoolSize ps = {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4};
   VkDescriptorPoolCreateInfo dpci = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                                      .maxSets = 4,
                                      .poolSizeCount = 1,
                                      .pPoolSizes = &ps};
   VkDescriptorPool dp;
   CK(vkCreateDescriptorPool(dev, &dpci, NULL, &dp), "DP");
   VkSamplerCreateInfo sci = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                              .magFilter = VK_FILTER_LINEAR,
                              .minFilter = VK_FILTER_LINEAR,
                              .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
                              .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                              .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                              .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                              .maxLod = 16.0f};
   VkSampler smp;
   CK(vkCreateSampler(dev, &sci, NULL, &smp), "Sampler");

   VkCommandPoolCreateInfo cpi = {.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                                  .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT};
   VkCommandPool pool;
   CK(vkCreateCommandPool(dev, &cpi, NULL, &pool), "Pool");
   VkCommandBufferAllocateInfo cbai = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                       .commandPool = pool,
                                       .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                       .commandBufferCount = 1};
   CK(vkAllocateCommandBuffers(dev, &cbai, &cb), "CB");
   VkFenceCreateInfo fci = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
   CK(vkCreateFence(dev, &fci, NULL, &fence), "Fence");
   VkQueryPoolCreateInfo qpci = {.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
                                 .queryType = VK_QUERY_TYPE_TIMESTAMP,
                                 .queryCount = 2};
   CK(vkCreateQueryPool(dev, &qpci, NULL, &qp), "QP");

   const float s30 = 0.5f, c30 = 0.8660254f;
   /* m.xy m.zw o.xy lod */
   const float modes[NMODES][7] = {
      {1, 0, 0, 1, 0.01f, 0.02f, -1},
      {0, 1, 1, 0, 0.01f, 0.02f, -1},
      {4 * c30, -4 * s30, 4 * s30, 4 * c30, 0.01f, 0.02f, -1},
      {2, 0, 0, 2, 0.01f, 0.02f, 0},
      {3 * c30, -3 * s30, 3 * s30, 3 * c30, 0.01f, 0.02f, 0},
   };

   uint32_t mips = 0;
   for (uint32_t s = size_px; s; s >>= 1)
      mips++;

   for (unsigned fi = 0; fi < sizeof(fmts) / sizeof(fmts[0]); fi++) {
      const VkFormat f = fmts[fi].f;
      if (filter && !strstr(fmts[fi].name, filter))
         continue;
      VkFormatProperties fp;
      vkGetPhysicalDeviceFormatProperties(phys, f, &fp);
      if (!(fp.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) ||
          !(fp.optimalTilingFeatures & VK_FORMAT_FEATURE_TRANSFER_DST_BIT)) {
         printf("PERF fmt=%s SKIP unsupported\n", fmts[fi].name);
         continue;
      }

      /* staging: whole chain */
      int blocked;
      const uint32_t ub = unit_B(f, &blocked);
      VkDeviceSize total = 0, lvl_off[16];
      for (uint32_t l = 0; l < mips; l++) {
         uint32_t w = size_px >> l;
         lvl_off[l] = total;
         total += blocked ? (VkDeviceSize)((w + 3) / 4) * ((w + 3) / 4) * ub
                          : (VkDeviceSize)w * w * ub;
         total = (total + 15) & ~15ull;
      }
      struct buf stg = mkbuf(total, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
      rng = 0x12345678u;
      for (uint32_t l = 0; l < mips; l++)
         encode_level(f, size_px >> l, size_px >> l, (uint8_t *)stg.p + lvl_off[l]);

      VkImageCreateInfo ici2 = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                                .imageType = VK_IMAGE_TYPE_2D,
                                .format = f,
                                .extent = {size_px, size_px, 1},
                                .mipLevels = mips,
                                .arrayLayers = 1,
                                .samples = VK_SAMPLE_COUNT_1_BIT,
                                .tiling = VK_IMAGE_TILING_OPTIMAL,
                                .usage = VK_IMAGE_USAGE_SAMPLED_BIT |
                                         VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                         VK_IMAGE_USAGE_TRANSFER_SRC_BIT};
      VkImage img;
      CK(vkCreateImage(dev, &ici2, NULL, &img), "Image");
      vkGetImageMemoryRequirements(dev, img, &mr);
      const VkDeviceSize mem_B = mr.size;
      mai.allocationSize = mr.size;
      mai.memoryTypeIndex = pick_mem(mr.memoryTypeBits, dev_mi);
      VkDeviceMemory im;
      CK(vkAllocateMemory(dev, &mai, NULL, &im), "ImgMem");
      CK(vkBindImageMemory(dev, img, im, 0), "ImgBind");
      VkImageViewCreateInfo vci = {.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                                   .image = img,
                                   .viewType = VK_IMAGE_VIEW_TYPE_2D,
                                   .format = f,
                                   .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0,
                                                        mips, 0, 1}};
      VkImageView view;
      CK(vkCreateImageView(dev, &vci, NULL, &view), "View");

      VkBufferImageCopy regions[16];
      for (uint32_t l = 0; l < mips; l++)
         regions[l] = (VkBufferImageCopy){
            .bufferOffset = lvl_off[l],
            .imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, l, 0, 1},
            .imageExtent = {size_px >> l, size_px >> l, 1}};

      double up_best = 1e9, up_wall = 1e9;
      for (int run = 0; run < 3; run++) {
         struct timespec t0, t1;
         begin();
         img_barrier(img, VK_IMAGE_LAYOUT_UNDEFINED,
                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0,
                     VK_ACCESS_TRANSFER_WRITE_BIT,
                     VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                     VK_PIPELINE_STAGE_TRANSFER_BIT);
         vkCmdWriteTimestamp(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, qp, 0);
         vkCmdCopyBufferToImage(cb, stg.b, img,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mips,
                                regions);
         vkCmdWriteTimestamp(cb, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, qp, 1);
         img_barrier(img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                     VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                     VK_PIPELINE_STAGE_TRANSFER_BIT,
                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
         clock_gettime(CLOCK_MONOTONIC, &t0);
         double g = submit();
         clock_gettime(CLOCK_MONOTONIC, &t1);
         double wall = (t1.tv_sec - t0.tv_sec) * 1e3 + (t1.tv_nsec - t0.tv_nsec) / 1e6;
         up_best = g < up_best ? g : up_best;
         up_wall = wall < up_wall ? wall : up_wall;
      }

      if (dump_dir) {
         /* LOD0 -> RGBA8 by a nearest blit (reads the decoded data). */
         VkImageCreateInfo dci2 = ici2;
         dci2.format = VK_FORMAT_R8G8B8A8_UNORM;
         dci2.mipLevels = 1;
         dci2.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
         VkImage di;
         CK(vkCreateImage(dev, &dci2, NULL, &di), "DumpImage");
         VkMemoryRequirements dmr;
         vkGetImageMemoryRequirements(dev, di, &dmr);
         VkMemoryAllocateInfo dai = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                                     .allocationSize = dmr.size,
                                     .memoryTypeIndex = pick_mem(dmr.memoryTypeBits, dev_mi)};
         VkDeviceMemory dm;
         CK(vkAllocateMemory(dev, &dai, NULL, &dm), "DumpMem");
         CK(vkBindImageMemory(dev, di, dm, 0), "DumpBind");
         struct buf rb = mkbuf((VkDeviceSize)size_px * size_px * 4,
                               VK_BUFFER_USAGE_TRANSFER_DST_BIT);
         begin();
         vkCmdWriteTimestamp(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, qp, 0);
         img_barrier(img, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_SHADER_READ_BIT,
                     VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                     VK_PIPELINE_STAGE_TRANSFER_BIT);
         img_barrier(di, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                     0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                     VK_PIPELINE_STAGE_TRANSFER_BIT);
         const int32_t s = size_px;
         VkImageBlit bl = {{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1}, {{0, 0, 0}, {s, s, 1}},
                           {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1}, {{0, 0, 0}, {s, s, 1}}};
         vkCmdBlitImage(cb, img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, di,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bl, VK_FILTER_NEAREST);
         img_barrier(di, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT,
                     VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                     VK_PIPELINE_STAGE_TRANSFER_BIT);
         VkBufferImageCopy rc = {.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
                                 .imageExtent = {size_px, size_px, 1}};
         vkCmdCopyImageToBuffer(cb, di, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, rb.b, 1, &rc);
         img_barrier(img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT,
                     VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
         vkCmdWriteTimestamp(cb, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, qp, 1);
         submit();
         char path[512];
         snprintf(path, sizeof(path), "%s/%s.rgba", dump_dir, fmts[fi].name);
         FILE *df = fopen(path, "wb");
         if (df) {
            fwrite(rb.p, 4, (size_t)size_px * size_px, df);
            fclose(df);
         }
         freebuf(&rb);
         vkDestroyImage(dev, di, NULL);
         vkFreeMemory(dev, dm, NULL);
      }

      CK(vkResetDescriptorPool(dev, dp, 0), "ResetDP");
      VkDescriptorSetAllocateInfo dsai = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                                          .descriptorPool = dp,
                                          .descriptorSetCount = 1,
                                          .pSetLayouts = &dsl};
      VkDescriptorSet ds;
      CK(vkAllocateDescriptorSets(dev, &dsai, &ds), "DS");
      VkDescriptorImageInfo dii = {smp, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
      VkWriteDescriptorSet wds = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                  .dstSet = ds,
                                  .descriptorCount = 1,
                                  .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                  .pImageInfo = &dii};
      vkUpdateDescriptorSets(dev, 1, &wds, 0, NULL);

      double res[NMODES], resw[NMODES];
      const VkClearValue clear = {0};
      for (int m = 0; m < NMODES; m++) {
         double t[RUNS + 1], tw[RUNS + 1];
         for (int run = 0; run <= RUNS; run++) {
            begin();
            VkRenderPassBeginInfo rpbi = {.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                                          .renderPass = rp,
                                          .framebuffer = fb,
                                          .renderArea = {{0, 0}, {RT_W, RT_H}},
                                          .clearValueCount = 1,
                                          .pClearValues = &clear};
            vkCmdWriteTimestamp(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, qp, 0);
            vkCmdBeginRenderPass(cb, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
            VkViewport vp = {0, 0, RT_W, RT_H, 0, 1};
            VkRect2D sc = {{0, 0}, {RT_W, RT_H}};
            vkCmdSetViewport(cb, 0, 1, &vp);
            vkCmdSetScissor(cb, 0, 1, &sc);
            vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe);
            vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pl, 0,
                                    1, &ds, 0, NULL);
            for (int k = 0; k < PASSES; k++) {
               float pc[7];
               memcpy(pc, modes[m], sizeof(pc));
               pc[4] += 0.003f * k;
               vkCmdPushConstants(cb, pl, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 28, pc);
               vkCmdDraw(cb, 3, 1, 0, 0);
            }
            vkCmdEndRenderPass(cb);
            vkCmdWriteTimestamp(cb, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, qp, 1);
            struct timespec w0, w1;
            clock_gettime(CLOCK_MONOTONIC, &w0);
            t[run] = submit() / PASSES;
            clock_gettime(CLOCK_MONOTONIC, &w1);
            tw[run] = ((w1.tv_sec - w0.tv_sec) * 1e3 +
                       (w1.tv_nsec - w0.tv_nsec) / 1e6) / PASSES;
         }
         /* drop the warm-up run */
         qsort(t + 1, RUNS, sizeof(double), cmp_d);
         res[m] = t[1 + RUNS / 2];
         qsort(tw + 1, RUNS, sizeof(double), cmp_d);
         resw[m] = tw[1 + RUNS / 2];
      }
      printf("PERF fmt=%s mem_KiB=%llu upload_gpu_ms=%.3f upload_wall_ms=%.3f "
             "A_ms=%.3f B_ms=%.3f C_ms=%.3f D_ms=%.3f E_ms=%.3f wallA=%.3f\n",
             fmts[fi].name, (unsigned long long)(mem_B / 1024), up_best,
             up_wall, res[0], res[1], res[2], res[3], res[4], resw[0]);

      vkDestroyImageView(dev, view, NULL);
      vkDestroyImage(dev, img, NULL);
      vkFreeMemory(dev, im, NULL);
      freebuf(&stg);
   }
   printf("RESULT DONE\n");
   return 0;
}
