/* Host test of panvk_bc_etc2.comp (csf-v11/197, 198): bitstream check against Mesa's ETC2/EAC
 * decoder (texcompress_etc.c) and transcode PSNR vs exact BC decode. */
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "c_subset.h"
#include "etc2_new.h"
#include "pics.h"

/* Mesa reference decoder. */
typedef unsigned GLuint;
typedef short GLshort;
typedef unsigned short GLushort;
typedef int GLint;
typedef signed char GLbyte;
typedef bool GLboolean;
#define TAG(x) x
#define UINT8_TYPE uint8_t
#define UNREACHABLE(x) abort()
#define CLAMP(x, a, b) clamp(x, a, b)
#include "ref_etc1.h"
#include "ref_etc2a.h"
#include "ref_etc2b.h"

static void
words_to_bytes(uint w0, uint w1, uint8_t *b)
{
   for (int i = 0; i < 4; i++) {
      b[i] = (w0 >> (8 * i)) & 0xff;
      b[4 + i] = (w1 >> (8 * i)) & 0xff;
   }
}

/* Mesa decode of an RGB(A1) block, error vs g_r/g_g/g_b (+ transparency). */
static int
ref_rgb_err(uint w0, uint w1, bool punch, int *mode_out, int *alpha_bad)
{
   uint8_t b[8];
   struct etc2_block blk;
   memset(&blk, 0, sizeof(blk));
   words_to_bytes(w0, w1, b);
   etc2_rgb8_parse_block(&blk, b, punch);
   *mode_out = blk.is_ind_mode ? 0 : blk.is_diff_mode ? 1 : blk.is_t_mode ? 2 :
               blk.is_h_mode ? 3 : 4;
   int err = 0;
   *alpha_bad = 0;
   for (int p = 0; p < 16; p++) {
      uint8_t d[4] = {0, 0, 0, 255};
      etc2_rgb8_fetch_texel(&blk, p & 3, p >> 2, d, punch);
      if (punch) {
         bool t = d[3] == 0;
         if (t != px_t(p))
            (*alpha_bad)++;
         if (t)
            continue;
      }
      err += sq(d[0] - px_r(p)) + sq(d[1] - px_g(p)) + sq(d[2] - px_b(p));
   }
   return err;
}

static int
ref_eac_err(uint w0, uint w1, int kind, double *sse_norm)
{
   uint8_t b[8];
   struct etc2_block blk;
   memset(&blk, 0, sizeof(blk));
   words_to_bytes(w0, w1, b);
   etc2_alpha8_parse_block(&blk, b);
   int err = 0;
   for (int p = 0; p < 16; p++) {
      int x = p & 3, y = p >> 2;
      int v;
      if (kind == 0) {
         uint8_t d[4];
         etc2_alpha8_fetch_texel(&blk, x, y, d);
         v = d[3];
      } else {
         /* Undo Mesa's bit replication to get the 11-bit value. */
         int idx = etc2_get_pixel_index(&blk, x, y);
         int mod = etc2_modifier_tables[blk.table_index][idx];
         if (kind == 1) {
            v = blk.multiplier ? etc2_clamp2(((blk.base_codeword << 3) | 4) + ((mod * blk.multiplier) << 3))
                               : etc2_clamp2(((blk.base_codeword << 3) | 4) + mod);
            uint8_t d[2];
            etc2_r11_fetch_texel(&blk, x, y, d);
         } else {
            GLbyte bc = (GLbyte)blk.base_codeword;
            if (bc == -128)
               bc = -127;
            v = blk.multiplier ? etc2_clamp3((bc << 3) + ((mod * blk.multiplier) << 3))
                               : etc2_clamp3((bc << 3) + mod);
         }
      }
      err += sq(v - g_v[p]);
   }
   double scale = kind == 0 ? 1.0 : kind == 1 ? 255.0 / 2047.0 : 127.5 / 1023.0;
   *sse_norm = err * scale * scale;
   return err;
}

static int fails;

static void
check_rgb_modes(bool punch_fmt, uint opts)
{
   bool transparent = false;
   for (int p = 0; p < 16; p++)
      transparent |= punch_fmt && px_t(p);
   int mode, abad;
   for (int flip = 0; flip < 2; flip++) {
      int e = etc_try_flip(flip, punch_fmt, transparent);
      int r = ref_rgb_err(g_w0, g_w1, punch_fmt, &mode, &abad);
      if (e != r || abad || mode > 1) {
         if (fails++ < 10)
            printf("FAIL flip%d punch=%d tr=%d enc=%d ref=%d mode=%d abad=%d\n",
                   flip, punch_fmt, transparent, e, r, mode, abad);
      }
   }
   if (transparent)
      return;
   int e = etc_try_h(punch_fmt, BIG_ERR);
   if (e != BIG_ERR) {
      int r = ref_rgb_err(g_w0, g_w1, punch_fmt, &mode, &abad);
      if (e != r || mode != 3 || abad) {
         if (fails++ < 10)
            printf("FAIL H punch=%d enc=%d ref=%d mode=%d\n", punch_fmt, e, r, mode);
      }
   }
   e = etc_try_planar();
   int r = ref_rgb_err(g_w0, g_w1, punch_fmt, &mode, &abad);
   if (e != r || mode != 4 || abad) {
      if (fails++ < 10)
         printf("FAIL planar punch=%d enc=%d ref=%d mode=%d\n", punch_fmt, e, r, mode);
   }
}

int
main(int argc, char **argv)
{
   uint opts = argc > 1 ? strtoul(argv[1], NULL, 0) : 0x30000;

   /* 1. Bitstream: random BC1/BC1A/BC4/BC5 blocks, every mode checked. */
   for (int it = 0; it < 20000; it++) {
      uint w0 = rnd32(), w1 = rnd32();
      if (it & 1)
         w0 = (w0 & 0xffff) | ((w0 & 0xffff) + (rnd32() % 64) - 32) << 16;
      bc_decode_color(w0, w1, false);
      check_rgb_modes(false, opts);
      check_rgb_modes(true, opts);
      for (int kind = 0; kind < 3; kind++) {
         bc_decode_rgtc(w0, w1, kind == 2, kind);
         double sn;
         int nb = 2;
         for (int k = 0; k < nb; k++) {
            eac_encode(kind, k ? opts | OPT_EAC_REFINE : opts);
            /* Re-derive the encoder's error. */
            int r = ref_eac_err(g_w0, g_w1, kind, &sn);
            (void)r;
         }
      }
   }

   /* EAC exactness: encoder error must match Mesa's decode. */
   for (int it = 0; it < 20000; it++) {
      uint w0 = rnd32(), w1 = rnd32();
      for (int kind = 0; kind < 3; kind++) {
         bc_decode_rgtc(w0, w1, kind == 2, kind);
         eac_encode(kind, opts);
         uint a = g_w0, b = g_w1;
         /* Brute-force error of the emitted block with our model. */
         uint8_t bytes[8];
         words_to_bytes(a, b, bytes);
         int base = kind == 2 ? (int)(signed char)bytes[0] : bytes[0];
         int mult = bytes[1] >> 4, t = bytes[1] & 15;
         uint64_t idx = 0;
         for (int i = 2; i < 8; i++) idx = (idx << 8) | bytes[i];
         int e_model = 0;
         for (int p = 0; p < 16; p++) {
            int pos = ((3 - (p >> 2)) + (3 - (p & 3)) * 4) * 3;
            int m = EAC_MOD[t * 8 + ((idx >> pos) & 7)];
            e_model += sq(eac_value(kind, base, mult, m) - g_v[p]);
         }
         double sn;
         int r = ref_eac_err(a, b, kind, &sn);
         if (r != e_model) {
            if (fails++ < 20)
               printf("FAIL eac kind=%d model=%d ref=%d\n", kind, e_model, r);
         }
      }
   }

   /* 2. Quality on a synthetic picture, 512x512. */
   const int W = 512;
   double sse_rgb[2] = {0, 0}, sse_eac[3] = {0, 0, 0}, sse_a = 0;
   int modes[5] = {0};
   long n = 0;
   for (int by = 0; by < W / 4; by++)
      for (int bx = 0; bx < W / 4; bx++) {
         int rgb[16][3], v[16];
         for (int p = 0; p < 16; p++) {
            int x = bx * 4 + (p & 3), y = by * 4 + (p >> 2);
            for (int c = 0; c < 3; c++) rgb[p][c] = pic(0, x, y, c);
            v[p] = pic(0, x, y, 3);
         }
         uint c0, c1, a0, a1;
         bc1_enc(rgb, &c0, &c1);
         bc4_enc(v, &a0, &a1);
         n += 16;

         /* BC1 RGB and BC3 colour. */
         for (int f = 0; f < 2; f++) {
            transcode_block(f ? XC_BC3 : XC_BC1_RGB, f ? a0 : c0, f ? a1 : c1, c0, c1, opts);
            bc_decode_color(c0, c1, f == 1);
            int mode, abad;
            int r = ref_rgb_err(g_out[f ? 2 : 0], g_out[f ? 3 : 1], false, &mode, &abad);
            sse_rgb[f] += r;
            if (f == 0) modes[mode]++;
            if (f == 1) {
               bc_decode_rgtc(a0, a1, false, 0);
               double sn;
               ref_eac_err(g_out[0], g_out[1], 0, &sn);
               sse_a += sn;
            }
         }
         for (int kind = 1; kind < 3; kind++) {
            transcode_block(kind == 1 ? XC_BC4_U : XC_BC4_S, a0, a1, 0, 0, opts);
            bc_decode_rgtc(a0, a1, kind == 2, kind);
            double sn;
            ref_eac_err(g_out[0], g_out[1], kind, &sn);
            sse_eac[kind] += sn;
         }
      }
   printf("opts=%#x fails=%d\n", opts, fails);
   printf("modes ind %d diff %d T %d H %d planar %d\n", modes[0], modes[1], modes[2], modes[3], modes[4]);
   printf("PSNR vs exact BC decode: BC1 RGB %.2f dB, BC3 colour %.2f dB, BC3 alpha %.2f dB\n",
          10 * log10(255.0 * 255 / (sse_rgb[0] / (3.0 * n))),
          10 * log10(255.0 * 255 / (sse_rgb[1] / (3.0 * n))),
          10 * log10(255.0 * 255 / (sse_a / n)));
   printf("PSNR vs exact BC decode: BC4 UNORM %.2f dB, BC4 SNORM %.2f dB (8-bit scale)\n",
          10 * log10(255.0 * 255 / (sse_eac[1] / n)),
          10 * log10(255.0 * 255 / (sse_eac[2] / n)));
   return fails != 0;
}
