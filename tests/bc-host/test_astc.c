/* Host test of panvk_bc_astc_enc.h (csf-v11/200): BC6H UF16 -> ASTC 4x4 HDR.
 *
 * 1. Bitstream: every encoded block is decoded by the ARM reference codec
 *    (astcenc, linked as a library) and must match, texel for texel and bit
 *    for bit, the fp16 values the encoder believes it produced.
 * 2. Quality against the exact BC6H decode (Mesa's BPTC decoder), next to
 *    the B10G11R11 rounding the exact path (csf-v11/150) already applies:
 *    PSNR after x/(1+x) tone mapping (8-bit scale), log2 RMSE, and the
 *    worst relative error.
 * 3. Host time per block.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Mesa first: c_subset.h defines min/max/clamp macros. */
#include "util/format/texcompress_bptc_tmp.h"
int astc_ref_init(void);
int astc_ref_decode(const uint8_t *blk, uint16_t *rgba16, int *err_block, int *cem);

#include "c_subset.h"
#include "panvk_bc_astc_enc.h"

uint16_t _mesa_float_to_half_rtz_slow(float val);
uint16_t
_mesa_float_to_half_rtz_slow(float val)
{
   abort();
}

static uint32_t rng = 777;
static uint32_t
rnd32(void)
{
   rng ^= rng << 13;
   rng ^= rng >> 17;
   rng ^= rng << 5;
   return rng;
}
static float
frand(void)
{
   return (rnd32() & 0xffffff) / (float)0x1000000;
}

/* ---- BC6H UF16 "mode 11" (Mesa mode 3) crude encoder: one region, 10-bit
 * endpoints, 4-bit indices. ---- */
static const int W4[16] = {0, 4, 9, 13, 17, 21, 26, 30, 34, 38, 43, 47, 51, 55, 60, 64};

static int
bc6_unq10(int ep)
{
   if (ep == 0)
      return 0;
   if (ep == 1023)
      return 0xffff;
   return ((ep << 15) + 0x4000) >> 9;
}

static void
put_bits(uint8_t *b, int *pos, uint32_t v, int n)
{
   for (int i = 0; i < n; i++, (*pos)++)
      if ((v >> i) & 1)
         b[*pos >> 3] |= 1 << (*pos & 7);
}

static void
bc6_encode(const float rgb[16][3], uint8_t *blk)
{
   int h[16][3];
   int lo[3] = {1 << 30, 1 << 30, 1 << 30}, hi[3] = {0, 0, 0};
   for (int p = 0; p < 16; p++)
      for (int c = 0; c < 3; c++) {
         h[p][c] = _mesa_float_to_half(rgb[p][c]);
         int v = (h[p][c] * 64 + 32) / 31; /* interp domain */
         lo[c] = v < lo[c] ? v : lo[c];
         hi[c] = v > hi[c] ? v : hi[c];
      }
   int e0[3], e1[3];
   for (int c = 0; c < 3; c++) {
      e0[c] = clamp((lo[c] - 32 + 32) / 64, 0, 1023);
      e1[c] = clamp((hi[c] - 32 + 32) / 64, 0, 1023);
   }
   int idx[16];
   for (int p = 0; p < 16; p++) {
      int be = 1 << 30, bi = 0;
      for (int i = 0; i < 16; i++) {
         int e = 0;
         for (int c = 0; c < 3; c++) {
            int a = bc6_unq10(e0[c]), b = bc6_unq10(e1[c]);
            int v = ((64 - W4[i]) * a + W4[i] * b + 32) >> 6;
            int d = ((v * 31) >> 6) - h[p][c];
            e += d * d;
         }
         if (e < be) { be = e; bi = i; }
      }
      idx[p] = bi;
   }
   /* Anchor (texel 0) index MSB must be 0: swap the endpoints if not. */
   if (idx[0] & 8) {
      for (int c = 0; c < 3; c++) { int t = e0[c]; e0[c] = e1[c]; e1[c] = t; }
      for (int p = 0; p < 16; p++) idx[p] = 15 - idx[p];
   }
   memset(blk, 0, 16);
   int pos = 0;
   put_bits(blk, &pos, 0x03, 5);
   for (int c = 0; c < 3; c++) put_bits(blk, &pos, e0[c], 10);
   for (int c = 0; c < 3; c++) put_bits(blk, &pos, e1[c], 10);
   put_bits(blk, &pos, idx[0], 3);
   for (int p = 1; p < 16; p++) put_bits(blk, &pos, idx[p], 4);
}

/* ---- HDR test pictures (linear float) ---- */
static float
hdr_pic(int kind, int x, int y, int c)
{
   switch (kind) {
   case 0: { /* sky: blue gradient, warm sun disc and halo up to ~60 */
      float base = (c == 2 ? 1.2f : c == 1 ? 0.7f : 0.4f) * (0.2f + y / 300.0f);
      float dx = x - 340.0f, dy = y - 90.0f, d2 = dx * dx + dy * dy;
      float sun = 60.0f * expf(-d2 / 300.0f) + 3.0f * expf(-d2 / 6000.0f);
      return base + sun * (c == 0 ? 1.0f : c == 1 ? 0.85f : 0.6f);
   }
   case 1: { /* lightmap: smooth blobs with soft shadow edges, 0..8 */
      float v = 2.0f + 1.5f * sinf(x * 0.03f + c) * cosf(y * 0.02f);
      float sh = 1.0f / (1.0f + expf(-(float)((x + 2 * y) % 97 - 48) * 0.5f));
      return v * (0.15f + 0.85f * sh) * (1.0f + 0.3f * c) + 4.0f * expf(-((x - 200) * (x - 200) + (y - 300) * (y - 300)) / 900.0f);
   }
   case 2: /* noisy texture, 0..2 */
      return 0.8f + 0.5f * sinf(x * 0.9f + 2.0f * c) * sinf(y * 0.7f) + 0.4f * frand();
   default: /* emissive panel: sharp bright shapes on dark */
      return ((x / 16 + y / 16) % 3 == 0) ? 20.0f * (1 + c) : 0.02f + 0.01f * c;
   }
}

static double
tm(double v)
{
   return 255.0 * v / (1.0 + v);
}

static float
b10g11r11_round(float v, int mbits)
{
   /* Round to a float with 'mbits' mantissa bits, 5-bit exponent, no sign. */
   if (v <= 0)
      return 0;
   int e;
   double m = frexp(v, &e); /* v = m * 2^e, 0.5 <= m < 1 */
   double q = ldexp(1.0, mbits + 1);
   m = floor(m * q + 0.5) / q;
   double r = ldexp(m, e);
   if (e < -13) /* denormal range: fixed step 2^-14 / 2^mbits */
      r = floor(v / ldexp(1.0, -14 - mbits) + 0.5) * ldexp(1.0, -14 - mbits);
   return (float)r;
}

struct stats {
   double se_tm, se_log;
   double max_rel;
   long n;
};

static void
acc(struct stats *s, float ref, float v)
{
   double d = tm(ref) - tm(v);
   s->se_tm += d * d;
   double l = log2(fmax(ref, 1e-4)) - log2(fmax(v, 1e-4));
   s->se_log += l * l;
   if (ref > 1e-3) {
      double rel = fabs(v - ref) / ref;
      if (rel > s->max_rel)
         s->max_rel = rel;
   }
   s->n++;
}

static void
report(const char *name, const struct stats *s)
{
   printf("  %-22s tonemapped PSNR %6.2f dB, log2 RMSE %.4f, max rel %.4f\n",
          name, 10 * log10(255.0 * 255.0 / (s->se_tm / s->n)),
          sqrt(s->se_log / s->n), s->max_rel);
}

int
main(int argc, char **argv)
{
   if (astc_ref_init()) {
      printf("astcenc init failed\n");
      return 1;
   }

   int fails = 0;
   long blocks = 0;
   double t_enc = 0;
   const int W = 512;

   for (int kind = -1; kind < 4; kind++) {
      struct stats s_astc = {0}, s_b10 = {0};
      int nb = kind < 0 ? 65536 : (W / 4) * (W / 4);

      for (int bi = 0; bi < nb; bi++) {
         uint8_t bc6[16];
         if (kind < 0) {
            /* Random UF16 BC6H blocks, every mode. */
            for (int i = 0; i < 16; i++)
               bc6[i] = rnd32();
         } else {
            float rgb[16][3];
            int bx = bi % (W / 4), by = bi / (W / 4);
            for (int p = 0; p < 16; p++)
               for (int c = 0; c < 3; c++)
                  rgb[p][c] = hdr_pic(kind, bx * 4 + (p & 3), by * 4 + (p >> 2), c);
            bc6_encode(rgb, bc6);
         }

         float ref[16 * 4];
         decompress_rgb_float_block(4, 4, bc6, ref, 16 * sizeof(float), false);
         for (int p = 0; p < 16; p++)
            for (int c = 0; c < 3; c++)
               g_h[3 * p + c] = _mesa_float_to_half(ref[4 * p + c]);

         struct timespec t0, t1;
         clock_gettime(CLOCK_MONOTONIC, &t0);
         astc_encode_block();
         clock_gettime(CLOCK_MONOTONIC, &t1);
         t_enc += (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) * 1e-9;
         blocks++;

         /* What the encoder thinks it emitted. */
         uint16_t mine[48];
         for (int p = 0; p < 16; p++) {
            int w = ASTC_WT[g_wi[p]];
            for (int c = 0; c < 3; c++)
               mine[3 * p + c] = astc_lns_to_h(((g_e[c] << 4) * (64 - w) + (g_e[3 + c] << 4) * w + 32) >> 6);
         }

         /* Reference decode. */
         uint16_t out[16 * 4];
         int err_block = 0, cem = 0;
         if (astc_ref_decode((const uint8_t *)g_blk, out, &err_block, &cem) || err_block) {
            if (fails++ < 10)
               printf("FAIL decode kind %d blk %d (error block %d)\n", kind, bi, err_block);
            continue;
         }
         for (int p = 0; p < 16; p++) {
            for (int c = 0; c < 3; c++) {
               if (out[4 * p + c] != mine[3 * p + c]) {
                  if (fails++ < 10) {
                     printf("FAIL kind %d blk %d texel %d c %d: astcenc %04x mine %04x "
                            "(v %02x %02x %02x %02x %02x %02x, err %d, cem %u)\n",
                            kind, bi, p, c, out[4 * p + c], mine[3 * p + c],
                            g_v[0], g_v[1], g_v[2], g_v[3], g_v[4], g_v[5],
                            err_block, cem);
                  }
                  break;
               }
            }
            for (int c = 0; c < 3; c++) {
               float r = ref[4 * p + c];
               acc(&s_astc, r, _mesa_half_to_float(out[4 * p + c]));
               acc(&s_b10, r, b10g11r11_round(r, c == 2 ? 5 : 6));
            }
         }
      }
      static const char *names[] = {"random BC6H UF blocks", "sky + sun (0..60)",
                                    "lightmap (0..8)", "noisy texture (0..2)",
                                    "emissive panel (0.02..60)"};
      printf("%s:\n", names[kind + 1]);
      report("ASTC 4x4 HDR (8 bpp)", &s_astc);
      report("B10G11R11 (32 bpp)", &s_b10);
   }
   printf("blocks %ld, bitstream mismatches %d, encode %.2f us/block (host)\n", blocks,
          fails, t_enc * 1e6 / blocks);
   return fails != 0;
}
