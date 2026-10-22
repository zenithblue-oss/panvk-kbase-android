/* Builds EAC_TM_LUT of panvk_bc_etc2.comp (csf-v11/200, exact encoder
 * since csf-v11/203): for every palette span key, EAC_MAX_CANDS EAC
 * (table, multiplier) pairs, picked greedily so that the best of the first
 * c candidates (what the shader tries with c candidates) has the smallest
 * total error of the shader's own encoder (eac_try_pal: centred and
 * least-squares base, exact levels) over the training blocks of that key.
 *
 * Training blocks: every BC4 endpoint pair, once with the palette used
 * evenly (8-value blocks 2 texels per entry, 6-value blocks entries 0-5)
 * and NRAND times with random indices. One LUT pair per EAC domain: alpha
 * (BC3, 8-bit), R11 (BC4 UNORM) and signed R11 (BC4 SNORM; the span key
 * is in endpoint units). Then 16 BC2 keys (span of the 4-bit alpha):
 * NBC2 blocks per (lo, hi) 4-bit pair with random values in [lo, hi].
 *
 *   ./gen_eac_lut > lut.txt   EAC_TM_LUT (the body between the markers)
 * Build next to a copy of the shader body as etc2_new.h (tail -n +2). */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "c_subset.h"
#include "etc2_new.h"
#include "pics.h"

#define NRAND 3
#define NBC2 1792
#define NBLK (65536 * (1 + NRAND))
#define MAXB 6144 /* training blocks per key (evenly subsampled) */

static uint blk_a0[NBLK], blk_a1[NBLK];
static int blk_key[NBLK], order[NBLK];
static int start[EAC_NKEY + 1], fill[EAC_NKEY + 1];
static float err[MAXB][256];
static double cur[MAXB];
static uint8_t lut[EAC_MAX_CANDS][EAC_NKEY];

static void
print_words(const uint8_t *b, int nw)
{
   for (int i = 0; i < nw; i++) {
      uint w = 0;
      for (int k = 0; k < 4; k++)
         w |= (uint)b[i * 4 + k] << (8 * k);
      printf("%s0x%08xu%s", i % 8 == 0 ? "   " : " ", w,
             i == nw - 1 ? "\n" : (i % 8 == 7 ? ",\n" : ","));
   }
}

/* Palette (or BC2 texels) of training block i, its range in *lo / *hi. */
static void
setup(int kind, int i, int *lo, int *hi)
{
   if (kind == 3) {
      g_isbc2 = true;
      g_aw0 = blk_a0[i];
      g_aw1 = blk_a1[i];
      *lo = 255;
      *hi = 0;
      for (int p = 0; p < 16; p++) {
         *lo = min(*lo, bc2_v(g_aw0, g_aw1, p));
         *hi = max(*hi, bc2_v(g_aw0, g_aw1, p));
      }
      return;
   }
   g_isbc2 = false;
   bc_decode_rgtc(blk_a0[i], blk_a1[i], kind == 2, kind);
   *lo = pal_lo();
   *hi = pal_hi();
}

int
main(void)
{
   for (int kind = 0; kind < 4; kind++) {
      int n = 0;
      if (kind == 3) {
         for (int l = 0; l < 16; l++) {
            for (int h = l; h < 16; h++) {
               for (int r = 0; r < NBC2; r++) {
                  uint w[2] = {0u, 0u};
                  for (int p = 0; p < 16; p++) {
                     uint v = p == 0 ? (uint)l : p == 1 ? (uint)h
                                                        : (uint)l + rnd32() % (uint)(h - l + 1);
                     w[p >> 3] |= v << (4 * (p & 7));
                  }
                  blk_a0[n] = w[0];
                  blk_a1[n] = w[1];
                  blk_key[n] = EAC_KEY_BC2 + h - l;
                  n++;
               }
            }
         }
      } else {
         for (int ep0 = 0; ep0 < 256; ep0++) {
            for (int ep1 = 0; ep1 < 256; ep1++) {
               /* Signed endpoints: -128 decodes as -127. */
               int s0 = kind == 2 ? max((int)(signed char)ep0, -127) : ep0;
               int s1 = kind == 2 ? max((int)(signed char)ep1, -127) : ep1;
               bool r7 = s0 > s1;
               for (int r = 0; r <= NRAND; r++) {
                  uint64_t bits = 0;
                  for (int p = 0; p < 16; p++) {
                     uint64_t b = r == 0 ? (uint64_t)(r7 ? p % 8 : p % 6) : rnd32() % 8;
                     bits |= b << (3 * p);
                  }
                  blk_a0[n] = (uint)ep0 | ((uint)ep1 << 8) | (uint)((bits & 0xffff) << 16);
                  blk_a1[n] = (uint)(bits >> 16);
                  bc_decode_rgtc(blk_a0[n], blk_a1[n], kind == 2, kind);
                  blk_key[n] = eac_key(kind, pal_hi() - pal_lo(), g_six);
                  n++;
               }
            }
         }
      }

      /* Blocks grouped by key. */
      memset(start, 0, sizeof(start));
      for (int i = 0; i < n; i++)
         start[blk_key[i] + 1]++;
      for (int k = 0; k < EAC_NKEY; k++)
         start[k + 1] += start[k];
      memcpy(fill, start, sizeof(fill));
      for (int i = 0; i < n; i++)
         order[fill[blk_key[i]]++] = i;

      int k0 = kind * 512, k1 = kind == 3 ? EAC_NKEY : k0 + 512;
      int ekind = kind == 3 ? 0 : kind;
      for (int key = k0; key < k1; key++) {
         int nb = start[key + 1] - start[key];
         if (nb == 0)
            continue;
         int nbc = nb < MAXB ? nb : MAXB;
         for (int j = 0; j < nbc; j++) {
            int i = order[start[key] + (int)((long)j * nb / nbc)];
            int lo, hi;
            setup(kind, i, &lo, &hi);
            /* Alpha: multiplier 0 is a constant block, not a candidate. */
            for (int tm = 0; tm < 256; tm++)
               err[j][tm] = (ekind == 0 && tm < 16) ? 1e30f
                                                    : (float)eac_try_pal(ekind, tm, lo, hi);
            cur[j] = 1e300;
         }
         for (int c = 0; c < EAC_MAX_CANDS; c++) {
            int best = 16;
            double best_e = 1e300;
            for (int tm = 0; tm < 256; tm++) {
               double e = 0;
               for (int j = 0; j < nbc; j++)
                  e += err[j][tm] < cur[j] ? err[j][tm] : cur[j];
               if (e < best_e) {
                  best_e = e;
                  best = tm;
               }
            }
            lut[c][key] = (uint8_t)best;
            for (int j = 0; j < nbc; j++)
               cur[j] = err[j][best] < cur[j] ? err[j][best] : cur[j];
         }
      }

      /* Keys no block produces: the nearest key that has blocks. */
      for (int key = k0; key < k1; key++) {
         if (start[key + 1] > start[key])
            continue;
         int base = kind == 3 ? k0 : (key & ~255), s = key - base, src = -1;
         int ns = kind == 3 ? 16 : 256;
         for (int d = 1; d < ns && src < 0; d++) {
            if (s - d >= 0 && start[base + s - d + 1] > start[base + s - d])
               src = base + s - d;
            else if (s + d < ns && start[base + s + d + 1] > start[base + s + d])
               src = base + s + d;
         }
         for (int c = 0; c < EAC_MAX_CANDS; c++)
            lut[c][key] = src < 0 ? 0x10 : lut[c][src];
      }
   }
   g_isbc2 = false;

   print_words(&lut[0][0], EAC_MAX_CANDS * EAC_NKEY / 4);
   return 0;
}
