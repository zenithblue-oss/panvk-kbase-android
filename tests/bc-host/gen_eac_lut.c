/* Builds EAC_TM_LUT of panvk_bc_etc2.comp (csf-v11/200): for every palette
 * span key, EAC_MAX_CANDS EAC (table, multiplier) pairs, picked greedily
 * so that the best of the first c candidates (what the shader tries with
 * c candidates) has the smallest total error of the shader's own encoder
 * (eac_try_pal) over the training blocks of that key.
 *
 * Training blocks: every BC4 endpoint pair, once with the palette used
 * evenly (8-value blocks 2 texels per entry, 6-value blocks entries 0-5)
 * and NRAND times with random indices. One LUT pair per EAC domain: alpha
 * (BC3, 8-bit), R11 (BC4 UNORM) and signed R11 (BC4 SNORM; the span key
 * is in endpoint units).
 *
 *   ./gen_eac_lut near > near.txt   EAC_NEAR (the nearest-modifier table
 *                                   eac_try_pal uses: paste it first)
 *   ./gen_eac_lut > lut.txt         EAC_TM_LUT
 * (the bodies between the EAC_NEAR / EAC_TM_LUT markers) */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "c_subset.h"
#include "etc2_new.h"
#include "pics.h"

#define NRAND 3
#define NBLK (65536 * (1 + NRAND))
#define NKEY 1536
#define MAXB 6144 /* training blocks per key (evenly subsampled) */

static uint blk_a0[NBLK], blk_a1[NBLK];
static int blk_key[NBLK], order[NBLK];
static int start[NKEY + 1], fill[NKEY + 1];
static float err[MAXB][256];
static double cur[MAXB];
static uint8_t lut[EAC_MAX_CANDS][NKEY];

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

static void
gen_near(void)
{
   static uint8_t near[16 * 2 * EAC_NX];
   for (int t = 0; t < 16; t++) {
      for (int x = 0; x < 2 * EAC_NX; x++) {
         double xv = (double)(x - EAC_NX) / EAC_RES;
         int bi = 0;
         for (int i = 1; i < 8; i++)
            if (fabs(xv - EAC_MOD[t * 8 + i]) < fabs(xv - EAC_MOD[t * 8 + bi]))
               bi = i;
         near[t * 2 * EAC_NX + x] = (uint8_t)(bi | ((EAC_MOD[t * 8 + bi] + 16) << 3));
      }
   }
   print_words(near, 16 * 2 * EAC_NX / 4);
}

int
main(int argc, char **argv)
{
   if (argc > 1 && !strcmp(argv[1], "near")) {
      gen_near();
      return 0;
   }
   for (int kind = 0; kind < 3; kind++) {
      int n = 0;
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

      /* Blocks grouped by key. */
      memset(start, 0, sizeof(start));
      for (int i = 0; i < n; i++)
         start[blk_key[i] + 1]++;
      for (int k = 0; k < NKEY; k++)
         start[k + 1] += start[k];
      memcpy(fill, start, sizeof(fill));
      for (int i = 0; i < n; i++)
         order[fill[blk_key[i]]++] = i;

      int k0 = kind * 512;
      for (int key = k0; key < k0 + 512; key++) {
         int nb = start[key + 1] - start[key];
         if (nb == 0)
            continue;
         int nbc = nb < MAXB ? nb : MAXB;
         for (int j = 0; j < nbc; j++) {
            int i = order[start[key] + (int)((long)j * nb / nbc)];
            bc_decode_rgtc(blk_a0[i], blk_a1[i], kind == 2, kind);
            int lo = pal_lo(), hi = pal_hi();
            /* Alpha: multiplier 0 is a constant block, not a candidate. */
            for (int tm = 0; tm < 256; tm++)
               err[j][tm] = (kind == 0 && tm < 16) ? 1e30f
                                                   : (float)eac_try_pal(kind, tm, lo, hi);
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
      for (int key = k0; key < k0 + 512; key++) {
         if (start[key + 1] > start[key])
            continue;
         int base = key & ~255, s = key & 255, src = -1;
         for (int d = 1; d < 256 && src < 0; d++) {
            if (s - d >= 0 && start[base + s - d + 1] > start[base + s - d])
               src = base + s - d;
            else if (s + d < 256 && start[base + s + d + 1] > start[base + s + d])
               src = base + s + d;
         }
         for (int c = 0; c < EAC_MAX_CANDS; c++)
            lut[c][key] = src < 0 ? 0x10 : lut[c][src];
      }
   }

   print_words(&lut[0][0], EAC_MAX_CANDS * NKEY / 4);
   return 0;
}
