/* The csf-v11/200 search encoder (ref/panvk_bc_etc2_200.comp) against the
 * current palette encoder: PSNR of both against the exact BC decode, per
 * mode and block set, and host time per block (a proxy for the shader's
 * ALU work; the GPU number comes from PANVK_DEBUG=bc_timing).
 * Args: [opts_new] [reps] [opts_old]. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "pics.h"
#include "etc_ref.h"

typedef unsigned int uint;
void xc_old(uint mode, const uint *w, uint opts, uint *out);
void xc_new(uint mode, const uint *w, uint opts, uint *out);
void xc_new_dec(uint mode, const uint *w, int rgb[16][3], int ch[16][2], bool tr[16]);

#define NBLK (1 << 16)
static uint blocks[4][NBLK][4]; /* random, pic0 / pic2 / pic3 */

static double
now(void)
{
   struct timespec ts;
   clock_gettime(CLOCK_MONOTONIC, &ts);
   return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static const char *mode_name[8] = {"BC1", "BC1A", "BC2", "BC3", "BC4U", "BC4S", "BC5U", "BC5S"};

/* Squared error (8-bit scale) of the output block against the exact BC
 * decode, summed over channels; *nch = channels compared. */
static double
block_sse(uint mode, const uint *w, const uint *o, int *nch)
{
   int rgb[16][3], ch[16][2];
   bool tr[16];
   xc_new_dec(mode, w, rgb, ch, tr);
   double sse = 0;
   *nch = 0;
   if (mode <= 3) {
      int d[16][3];
      bool dt[16];
      int m;
      bool punch = mode == 1;
      const uint *c = mode <= 1 ? o : o + 2;
      ref_rgb(c[0], c[1], punch, d, dt, &m);
      for (int p = 0; p < 16; p++) {
         if (punch && dt[p] != tr[p])
            sse += 3 * 255.0 * 255.0; /* wrong transparency */
         if (tr[p])
            continue;
         for (int k = 0; k < 3; k++)
            sse += (double)(d[p][k] - rgb[p][k]) * (d[p][k] - rgb[p][k]);
      }
      *nch = 3;
      if (mode >= 2) {
         int v[16];
         ref_eac(o[0], o[1], 0, v);
         for (int p = 0; p < 16; p++)
            sse += (double)(v[p] - ch[p][0]) * (v[p] - ch[p][0]);
         *nch = 4;
      }
      return sse;
   }
   int kind = (mode == 5 || mode == 7) ? 2 : 1;
   double sc = ref_eac_scale(kind);
   int nc = mode >= 6 ? 2 : 1;
   for (int c = 0; c < nc; c++) {
      int v[16];
      ref_eac(o[2 * c], o[2 * c + 1], kind, v);
      for (int p = 0; p < 16; p++)
         sse += (double)(v[p] - ch[p][c]) * (v[p] - ch[p][c]) * sc * sc;
   }
   *nch = nc;
   return sse;
}

int
main(int argc, char **argv)
{
   uint opts = argc > 1 ? strtoul(argv[1], NULL, 0) : 0x30000;
   int reps = argc > 2 ? atoi(argv[2]) : 1;
   uint opts_old = argc > 3 ? strtoul(argv[3], NULL, 0) : 0x30000;

   for (int i = 0; i < NBLK; i++)
      for (int k = 0; k < 4; k++)
         blocks[0][i][k] = rnd32();
   for (int set = 1; set < 4; set++) {
      int kind = set == 1 ? 0 : set == 2 ? 2 : 3;
      for (int i = 0; i < NBLK; i++) {
         int bx = i % 256, by = i / 256;
         int rgb[16][3], v[16];
         for (int p = 0; p < 16; p++) {
            int x = bx * 4 + (p & 3), y = by * 4 + (p >> 2);
            for (int c = 0; c < 3; c++) rgb[p][c] = pic(kind, x, y, c);
            v[p] = pic(kind, x, y, 3);
         }
         uint c0, c1, a0, a1;
         bc1_enc(rgb, &c0, &c1);
         bc4_enc(v, &a0, &a1);
         blocks[set][i][0] = c0; blocks[set][i][1] = c1;
         blocks[set][i][2] = a0; blocks[set][i][3] = a1;
      }
   }

   printf("opts new=%#x old=%#x, %d blocks per set; sets: random, pic0 (waves+edges), pic2 (flat+shapes), pic3 (smooth)\n",
          opts, opts_old, NBLK);
   printf("%-5s %-6s %8s %8s %7s | %9s %9s %7s\n", "mode", "set", "old dB", "new dB",
          "delta", "old us/1k", "new us/1k", "speedup");
   double tot_old = 0, tot_new = 0;
   for (uint mode = 0; mode < 8; mode++) {
      double t_old = 0, t_new = 0;
      for (int set = 0; set < 4; set++) {
         static uint w[NBLK][4];
         for (int i = 0; i < NBLK; i++) {
            memcpy(w[i], blocks[set][i], sizeof(w[i]));
            /* Pictures: BC2/BC3 put alpha first, BC4/BC5 use the BC4 words. */
            if (set && mode >= 2) {
               uint t0 = w[i][0], t1 = w[i][1];
               w[i][0] = w[i][2]; w[i][1] = w[i][3]; w[i][2] = t0; w[i][3] = t1;
               if (mode >= 6) {
                  w[i][2] = blocks[set][(i + 977) % NBLK][2];
                  w[i][3] = blocks[set][(i + 977) % NBLK][3];
               }
            }
         }
         double sse_old = 0, sse_new = 0, n = 0;
         for (int i = 0; i < NBLK; i++) {
            uint o0[4] = {0}, o1[4] = {0};
            int nch;
            xc_old(mode, w[i], opts_old, o0);
            xc_new(mode, w[i], opts, o1);
            sse_old += block_sse(mode, w[i], o0, &nch);
            sse_new += block_sse(mode, w[i], o1, &nch);
            n += 16 * nch;
         }
         double p_old = 10 * log10(255.0 * 255.0 / (sse_old / n));
         double p_new = 10 * log10(255.0 * 255.0 / (sse_new / n));
         double ts_old = 0, ts_new = 0;
         for (int r = 0; r < reps; r++) {
            uint o[4];
            double t = now();
            for (int i = 0; i < NBLK; i++)
               xc_old(mode, w[i], opts_old, o);
            ts_old += now() - t;
            t = now();
            for (int i = 0; i < NBLK; i++)
               xc_new(mode, w[i], opts, o);
            ts_new += now() - t;
         }
         double k = 1e6 / (NBLK * reps / 1000.0);
         static const char *set_name[4] = {"random", "pic0", "pic2", "pic3"};
         printf("%-5s %-6s %8.2f %8.2f %+7.2f | %9.1f %9.1f %6.1fx\n", mode_name[mode],
                set_name[set], p_old, p_new, p_new - p_old, ts_old * k, ts_new * k,
                ts_old / ts_new);
         t_old += ts_old;
         t_new += ts_new;
      }
      tot_old += t_old;
      tot_new += t_new;
   }
   printf("all   %.1fx less host time\n", tot_old / tot_new);
   return 0;
}
