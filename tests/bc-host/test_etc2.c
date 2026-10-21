/* Host test of panvk_bc_etc2.comp (csf-v11/197, palette encoder of 200):
 * bitstream checks against Mesa's ETC2/EAC decoder (texcompress_etc.c) and
 * transcode PSNR against the exact BC decode on the test pictures. */
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "c_subset.h"
#include "etc2_new.h"
#include "pics.h"
#include "etc_ref.h"

static int fails;
#define FAIL(...)                                                            \
   do {                                                                      \
      if (fails++ < 20)                                                      \
         printf(__VA_ARGS__);                                                \
   } while (0)

/* Error of an ETC2 RGB block against the BC1 palette decode (g_c*, g_ci);
 * *abad: transparency mismatches (RGB8A1). */
static int
rgb_err(uint w0, uint w1, bool punch, int *mode, int *abad)
{
   int rgb[16][3];
   bool tr[16];
   ref_rgb(w0, w1, punch, rgb, tr, mode);
   int err = 0;
   *abad = 0;
   for (int p = 0; p < 16; p++) {
      int k = (g_ci >> (2 * p)) & 3;
      bool t = punch && g_c3t && k == 3;
      if (punch && t != tr[p])
         (*abad)++;
      if (t)
         continue;
      int c = pal_c(k);
      err += sq(rgb[p][0] - c_r(c)) + sq(rgb[p][1] - c_g(c)) + sq(rgb[p][2] - c_b(c));
   }
   return err;
}

/* Error of an EAC block against the RGTC palette decode (or BC2 values). */
static int
eac_err(uint w0, uint w1, int kind, const int *bc2)
{
   int v[16], err = 0;
   ref_eac(w0, w1, kind, v);
   for (int p = 0; p < 16; p++)
      err += sq(v[p] - (bc2 ? bc2[p] : pv(texel_bi(p))));
   return err;
}

static void
check_bits(void)
{
   for (uint x = 0; x < 65536; x++) {
      uint t = 0;
      for (int b = 0; b < 16; b++)
         if (x & (1u << b))
            t |= 1u << ((b & 3) * 4 + (b >> 2));
      if (transpose4(x) != t)
         FAIL("FAIL transpose4(%#x) = %#x, want %#x\n", x, transpose4(x), t);
      uint s = 0;
      for (int b = 0; b < 16; b++)
         s |= ((x >> b) & 1u) << (2 * b);
      if (compact_even(s | (rnd32() & 0xaaaaaaaau)) != x)
         FAIL("FAIL compact_even(%#x)\n", s);
   }
}

static uint
rnd_bc1(int it)
{
   uint w0 = rnd32();
   if (it & 1) /* close endpoints */
      w0 = (w0 & 0xffff) | ((((w0 & 0xffff) + (rnd32() % 64) - 32) & 0xffff) << 16);
   return w0;
}

/* Every RGB mode, its bitstream, and the error the encoder reports. */
static void
check_rgb(void)
{
   const uint optset[3] = {0x30000, 0xf0000, 0};
   for (int it = 0; it < 40000; it++) {
      uint w0 = rnd_bc1(it), w1 = rnd32();
      bc_decode_color(w0, w1, false);
      for (int punch = 0; punch < 2; punch++) {
         uint tm = (punch && g_c3t) ? pal_mask(3) : 0u;
         bool transparent = tm != 0;
         int mode, abad;
         for (int flip = 0; flip < 2; flip++) {
            int e = etc_try_flip(flip, punch, transparent, tm, 0x30000);
            int r = rgb_err(g_w0, g_w1, punch, &mode, &abad);
            if (e != r || abad || mode > 1)
               FAIL("FAIL flip%d punch=%d tr=%d enc=%d ref=%d mode=%d abad=%d\n",
                    flip, punch, transparent, e, r, mode, abad);
         }
         if (!transparent) {
            int e = etc_try_h();
            if (e != BIG_ERR) {
               int r = rgb_err(g_w0, g_w1, punch, &mode, &abad);
               if (e != r || mode != 3 || abad)
                  FAIL("FAIL H punch=%d enc=%d ref=%d mode=%d\n", punch, e, r, mode);
            }
            etc_try_planar();
            rgb_err(g_w0, g_w1, punch, &mode, &abad);
            if (mode != 4 || abad)
               FAIL("FAIL planar punch=%d mode=%d\n", punch, mode);
         }
         for (int o = 0; o < 3; o++) {
            int e = etc2_encode_rgb(punch, optset[o]);
            int r = rgb_err(g_w0, g_w1, punch, &mode, &abad);
            if ((g_exact && e != r) || abad)
               FAIL("FAIL rgb punch=%d opts=%#x enc=%d ref=%d abad=%d\n", punch,
                    optset[o], e, r, abad);
         }
      }
   }
}

/* EAC: the error the encoder reports is the Mesa decode's, every kind. */
static void
check_eac(void)
{
   for (int it = 0; it < 40000; it++) {
      uint w0 = rnd32(), w1 = rnd32();
      if (it & 1) /* close endpoints */
         w0 = (w0 & 0xffff00ffu) | ((((w0 & 0xff) + (rnd32() % 16) - 8) & 0xff) << 8);
      for (int kind = 0; kind < 3; kind++) {
         bc_decode_rgtc(w0, w1, kind == 2, kind);
         int tm = eac_encode_pal(kind, it & 2 ? 0x230000 : 0x30000);
         int e = g_sub_t;
         eac_emit(tm);
         int r = eac_err(g_w0, g_w1, kind, NULL);
         if (e != r)
            FAIL("FAIL eac kind=%d enc=%d ref=%d\n", kind, e, r);
      }
      int v[16];
      for (int p = 0; p < 16; p++)
         v[p] = bc2_v(w0, w1, p);
      int tm = eac_encode_bc2(w0, w1);
      int e = g_sub_t;
      eac_emit(tm);
      int r = eac_err(g_w0, g_w1, 0, v);
      if (e != r)
         FAIL("FAIL eac bc2 enc=%d ref=%d\n", e, r);
   }
}

static double
psnr(double sse, double n)
{
   return sse > 0 ? 10 * log10(255.0 * 255.0 / (sse / n)) : 99.0;
}

int
main(int argc, char **argv)
{
   uint opts = argc > 1 ? strtoul(argv[1], NULL, 0) : 0x30000;

   check_bits();
   check_rgb();
   check_eac();
   printf("bitstream checks: fails=%d\n", fails);

   /* Quality on the synthetic pictures, 512x512 each. */
   const int W = 512;
   printf("opts=%#x, PSNR vs the exact BC decode (dB):\n", opts);
   printf("%-5s %8s %8s %8s %8s %8s %8s\n", "pic", "BC1", "BC1A", "BC2 a", "BC3 a",
          "BC4 U", "BC4 S");
   for (int kind = 0; kind < 4; kind++) {
      double sse[6] = {0};
      int modes[5] = {0};
      long n = 0;
      for (int by = 0; by < W / 4; by++)
         for (int bx = 0; bx < W / 4; bx++) {
            int rgb[16][3], v[16];
            for (int p = 0; p < 16; p++) {
               int x = bx * 4 + (p & 3), y = by * 4 + (p >> 2);
               for (int c = 0; c < 3; c++)
                  rgb[p][c] = pic(kind, x, y, c);
               v[p] = pic(kind, x, y, 3);
            }
            uint c0, c1, a0, a1;
            bc1_enc(rgb, &c0, &c1);
            bc4_enc(v, &a0, &a1);
            n += 16;

            int mode, abad;
            transcode_block(XC_BC1_RGB, c0, c1, 0, 0, opts);
            bc_decode_color(c0, c1, false);
            sse[0] += rgb_err(g_out0, g_out1, false, &mode, &abad) / 3.0;
            modes[mode]++;
            /* BC1A: the same colours as a 3-colour block where possible. */
            uint c0a = ((c0 & 0xffff) < (c0 >> 16)) ? c0 : ((c0 >> 16) | (c0 << 16));
            uint c1a = c1;
            transcode_block(XC_BC1_RGBA, c0a, c1a, 0, 0, opts);
            bc_decode_color(c0a, c1a, false);
            sse[1] += rgb_err(g_out0, g_out1, true, &mode, &abad) / 3.0;
            if (abad)
               FAIL("FAIL BC1A transparency %d\n", abad);

            /* BC2 alpha from the 4-bit values, BC3 alpha. */
            uint q0 = 0, q1 = 0;
            int bv[16];
            for (int p = 0; p < 16; p++) {
               uint q = (uint)(v[p] * 15 + 127) / 255;
               bv[p] = q * 17;
               if (p < 8) q0 |= q << (4 * p);
               else q1 |= q << (4 * (p - 8));
            }
            transcode_block(XC_BC2, q0, q1, c0, c1, opts);
            sse[2] += eac_err(g_out0, g_out1, 0, bv);
            transcode_block(XC_BC3, a0, a1, c0, c1, opts);
            bc_decode_rgtc(a0, a1, false, 0);
            sse[3] += eac_err(g_out0, g_out1, 0, NULL);
            for (int k = 1; k < 3; k++) {
               /* SNORM: the same picture shifted by -128 (both endpoint
                * bytes XOR 0x80). */
               uint s0 = k == 2 ? a0 ^ 0x8080u : a0;
               transcode_block(k == 1 ? XC_BC4_U : XC_BC4_S, s0, a1, 0, 0, opts);
               bc_decode_rgtc(s0, a1, k == 2, k);
               double sc = ref_eac_scale(k);
               sse[3 + k] += eac_err(g_out0, g_out1, k, NULL) * sc * sc;
            }
         }
      printf("pic%-2d %8.2f %8.2f %8.2f %8.2f %8.2f %8.2f   modes ind %d diff %d H %d planar %d\n",
             kind, psnr(sse[0], n), psnr(sse[1], n), psnr(sse[2], n), psnr(sse[3], n),
             psnr(sse[4], n), psnr(sse[5], n), modes[0], modes[1], modes[3], modes[4]);
   }
   printf("fails=%d\n", fails);
   return fails != 0;
}
