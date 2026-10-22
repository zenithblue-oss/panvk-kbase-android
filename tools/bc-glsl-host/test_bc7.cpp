/* BC7 fast per-texel decoder (panvk_bc_bc7.comp -DPANVK_BC_FAST,
 * csf-v11/211) against the Granite decoder it replaces (the same file,
 * compute variants) and Mesa's CPU BPTC decoder: every texel of random
 * blocks of every mode (and invalid modes) must be bit-identical.
 * usage: test_bc7 [blocks] */
#include <cstdio>
#include <cstdlib>
#include "bc7tex.cpp"
#include "bc7fast.cpp"

uvec3 gl_GlobalInvocationID;
extern "C" void ref_bc7(const uint8_t *blk, int texel, uint8_t rgba[4]);

static uint32_t rng = 7;
static uint32_t lcg() { rng = rng * 1664525u + 1013904223u; return rng >> 8; }

template <typename R>
static void setup(R &r, const void *src, void *dst, int fmt)
{
   r = {};
   r.src = host_addr(src);
   r.dst = host_addr(dst);
   r.src_row_B = 16;
   r.dst_row_B = 16;
   r.width = 4;
   r.height = 4;
   r.depth = 1;
   r.format = fmt;
}

int main(int argc, char **argv)
{
   const int NB = argc > 1 ? atoi(argv[1]) : 200000;
   long mism = 0, ref_mism = 0, per_mode[9] = {0};
   alignas(16) uint8_t blk[16];

   for (int b = 0; b < NB; b++) {
      for (int k = 0; k < 16; k++)
         blk[k] = lcg() & 0xff;
      /* mode m: bit m set, bits below clear; 1/64 of the blocks invalid */
      int m = b % 64 == 63 ? 8 : b % 8;
      blk[0] = m == 8 ? 0 : (blk[0] & ~((2u << m) - 1)) | (1u << m);
      per_mode[m]++;
      uint32_t a[16], f[16];
      const int fmt = b & 1 ? 146 : 145;
      setup(bc7tex::registers, blk, a, fmt);
      setup(bc7fast::registers, blk, f, fmt);
      for (int p = 0; p < 16; p++) {
         gl_GlobalInvocationID = uvec3(p & 3, p >> 2, 0);
         bc7tex::shader_main();
         bc7fast::shader_main();
      }
      for (int p = 0; p < 16; p++) {
         uint8_t r[4];
         ref_bc7(blk, p, r);
         const uint32_t rw = r[0] | r[1] << 8 | r[2] << 16 | (uint32_t)r[3] << 24;
         if (a[p] != rw)
            ref_mism++;
         if (f[p] != a[p] && mism++ < 5)
            printf("mismatch mode %d texel %d: fast %08x granite %08x mesa %08x\n",
                   m, p, f[p], a[p], rw);
      }
   }
   printf("BC7: %d blocks (modes 0-7: %ld %ld %ld %ld %ld %ld %ld %ld, invalid %ld), "
          "fast vs Granite texel mismatches %ld, Granite vs Mesa BPTC %ld\n",
          NB, per_mode[0], per_mode[1], per_mode[2], per_mode[3], per_mode[4],
          per_mode[5], per_mode[6], per_mode[7], per_mode[8], mism, ref_mism);
   printf(mism ? "FAIL\n" : "PASS\n");
   return mism != 0;
}
