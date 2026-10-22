/* BC6H block decoder (panvk_bc_bc6.comp -DPANVK_BC_BLOCK, csf-v11/210)
 * against the per-texel decoder it replaces (the same file, compute
 * variant) and Mesa's CPU BPTC decoder.
 *   SF16 -> R16G16B16A16: every texel bit-exact to the per-texel decoder.
 *   UF16 -> B10G11R11: every channel the round-to-nearest-even (saturated)
 *   B10G11R11 value of the per-texel decoder's half; max relative error
 *   against the half reported.
 * Also checks the U-interleaved addresses over a 2x2-tile region with an
 * odd block origin. usage: test_bc6 [blocks] */
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <vector>
#include "bc6tex.cpp"
#include "bc6blk.cpp"

uvec3 gl_GlobalInvocationID;
extern "C" void ref_bc6(const uint8_t *blk, int texel, int sgn, float rgb[3]);
extern "C" void ref_bptc_half_init(void);

static uint32_t rng = 1;
static uint32_t lcg() { rng = rng * 1664525u + 1013904223u; return rng >> 8; }

static float half_f(uint h)
{
   int e = (h >> 10) & 31, m = h & 1023;
   float v = e ? std::ldexp(1.0f + m / 1024.0f, e - 15) : std::ldexp(m / 1024.0f, -14);
   return (h & 0x8000) ? -v : v;
}
static float uf_f(uint v, int mb)
{
   int e = v >> mb, m = v & ((1 << mb) - 1);
   return e ? std::ldexp(1.0f + m / float(1 << mb), e - 15)
            : std::ldexp(m / float(1 << mb), -14);
}
static uint rne(uint h, int s, uint maxf)
{
   uint r = (h + (1u << (s - 1)) - 1 + ((h >> s) & 1)) >> s;
   return r > maxf ? maxf : r;
}
/* pan_tiling.c: element index of (x, y) in a 16x16 tile */
static uint utile(uint x, uint y)
{
   uint r = 0;
   for (int k = 0; k < 4; k++)
      r |= (((x >> k) ^ (y >> k)) & 1) << (2 * k) | ((y >> k) & 1) << (2 * k + 1);
   return r;
}

int main(int argc, char **argv)
{
   const int NB = argc > 1 ? atoi(argv[1]) : 20000;
   static const int modes[] = {0, 1, 2, 3, 6, 7, 10, 11, 14, 15, 18, 22, 26, 30, 19, 31};
   /* Region of 8x8 blocks at block (5, 3) of a 64x48-block level. */
   const int RW = 8, RH = 8, BX0 = 5, BY0 = 3, LW = 64, LH = 48;
   const int tiles_x = LW / 4;
   int fails = 0;

   for (int sgn = 0; sgn < 2; sgn++) {
      const uint T = sgn ? 8 : 4;
      double max_rel = 0, max_ref = 0;
      long mism = 0, sat = 0, ref_mism = 0, texels = 0;
      std::vector<uint8_t> in(RW * RH * 16);
      std::vector<uint32_t> dst(tiles_x * (LH / 4) * 256 * T / 4);

      for (int it = 0; it < NB / (RW * RH) + 1; it++) {
         for (auto &b : in)
            b = lcg() & 0xff;
         for (int b = 0; b < RW * RH; b++) {
            /* a third of the blocks with a forced mode (all of them) */
            if (b % 3 == 0) {
               int m = modes[(it * RW * RH + b) / 3 % 16];
               in[b * 16] = (in[b * 16] & ~(m < 2 ? 3 : 31)) | m;
            }
            /* a few maxed endpoints (0xffff -> 0x7bff, the rounding edge) */
            if (b % 7 == 1)
               for (int k = 0; k < 9; k++)
                  in[b * 16 + 1 + k] = 0xff;
         }
         memset(dst.data(), 0, dst.size() * 4);

         /* block variant over the region */
         bc6blk::XC_MODE = sgn ? 9 : 8;
         bc6blk::registers = {};
         bc6blk::registers.src = host_addr(in.data());
         bc6blk::registers.src_row_B = RW * 16;
         bc6blk::registers.dst = host_addr(dst.data());
         bc6blk::registers.dst_row_B = tiles_x * 256 * T;
         bc6blk::registers.bx0 = BX0;
         bc6blk::registers.by0 = BY0;
         bc6blk::registers.bw = RW;
         bc6blk::registers.bh = RH;
         for (int y = 0; y < RH; y++)
            for (int x = 0; x < RW; x++) {
               gl_GlobalInvocationID = uvec3(x, y, 0);
               bc6blk::is_signed = false;
               bc6blk::shader_main();
            }

         for (int b = 0; b < RW * RH; b++) {
            const uint8_t *blk = &in[b * 16];
            uint32_t tex[16 * 2];
            bc6tex::registers = {};
            bc6tex::registers.src = host_addr(blk);
            bc6tex::registers.src_row_B = 16;
            bc6tex::registers.dst = host_addr(tex);
            bc6tex::registers.dst_row_B = 32;
            bc6tex::registers.width = 4;
            bc6tex::registers.height = 4;
            bc6tex::registers.depth = 1;
            bc6tex::registers.format = sgn ? 144 : 143;
            for (int p = 0; p < 16; p++) {
               gl_GlobalInvocationID = uvec3(p & 3, p >> 2, 0);
               bc6tex::is_signed = false;
               bc6tex::shader_main();
            }
            const int bx = BX0 + b % RW, by = BY0 + b / RW;
            for (int p = 0; p < 16; p++) {
               const uint X = bx * 4 + (p & 3), Y = by * 4 + (p >> 2);
               const uint el = ((Y / 16) * tiles_x + X / 16) * 256 + utile(X & 15, Y & 15);
               const uint *got = &dst[el * T / 4];
               const uint h[3] = {tex[2 * p] & 0xffff, tex[2 * p] >> 16, tex[2 * p + 1] & 0xffff};
               float ref[3];
               ref_bc6(blk, p, sgn, ref);
               texels++;
               for (int c = 0; c < 3; c++) {
                  double d = std::fabs(half_f(h[c]) - ref[c]);
                  if (d > 0) {
                     ref_mism++;
                     max_ref = std::max(max_ref, d);
                  }
               }
               if (sgn) {
                  if (got[0] != tex[2 * p] || got[1] != tex[2 * p + 1]) {
                     if (mism++ < 5)
                        printf("SF mismatch blk %d texel %d mode %d: %08x %08x vs %08x %08x\n",
                               b, p, blk[0] & 31, got[0], got[1], tex[2 * p], tex[2 * p + 1]);
                  }
               } else {
                  const uint want = rne(h[0], 4, 0x7bf) | rne(h[1], 4, 0x7bf) << 11 |
                                    rne(h[2], 5, 0x3df) << 22;
                  if (got[0] != want) {
                     if (mism++ < 5)
                        printf("UF mismatch blk %d texel %d mode %d: %08x vs %08x\n",
                               b, p, blk[0] & 31, got[0], want);
                  }
                  const uint ch[3] = {got[0] & 0x7ff, (got[0] >> 11) & 0x7ff, got[0] >> 22};
                  for (int c = 0; c < 3; c++) {
                     const float hv = half_f(h[c]), uv = uf_f(ch[c], c < 2 ? 6 : 5);
                     if (h[c] > 0x7bf0 && c < 2)
                        sat++;
                     if (hv > 6.2e-5f)
                        max_rel = std::max(max_rel, (double)std::fabs(uv - hv) / hv);
                  }
               }
            }
         }
      }
      printf("%s: %ld texels, block vs per-texel mismatches %ld, %s, "
             "per-texel vs Mesa BPTC: %ld channel diffs (max %g)\n",
             sgn ? "SF16->RGBA16F" : "UF16->B10G11R11", texels, mism,
             sgn ? "bit-exact check" : "RNE check", ref_mism, max_ref);
      if (!sgn)
         printf("  max relative error of the B10G11R11 value vs the half (normal range): %g "
                "(RNE bound 2^-7 = %g, 2^-6 for B); %ld channels at the saturation edge\n",
                max_rel, 1.0 / 128, sat);
      fails += mism != 0;
   }
   printf(fails ? "FAIL\n" : "PASS\n");
   return fails != 0;
}
