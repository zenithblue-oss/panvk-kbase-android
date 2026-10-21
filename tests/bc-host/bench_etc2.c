/* 197 vs current transcoder: bit-identical output on random and picture
 * blocks for every mode, and host time per block (a proxy for the shader's
 * ALU work; the GPU number comes from PANVK_DEBUG=bc_timing). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "pics.h"

typedef unsigned int uint;
void xc_old(uint mode, const uint *w, uint opts, uint *out);
void xc_new(uint mode, const uint *w, uint opts, uint *out);

#define NBLK (1 << 16)
static uint blocks[4][NBLK][4]; /* random, pic0..2 as BC1+BC4 words */

static double
now(void)
{
   struct timespec ts;
   clock_gettime(CLOCK_MONOTONIC, &ts);
   return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static const char *mode_name[8] = {"BC1", "BC1A", "BC2", "BC3", "BC4U", "BC4S", "BC5U", "BC5S"};

int
main(int argc, char **argv)
{
   uint opts = argc > 1 ? strtoul(argv[1], NULL, 0) : 0x30000;
   int reps = argc > 2 ? atoi(argv[2]) : 1;

   for (int i = 0; i < NBLK; i++)
      for (int k = 0; k < 4; k++)
         blocks[0][i][k] = rnd32();
   for (int set = 1; set < 4; set++) {
      int kind = set == 1 ? 0 : set == 2 ? 2 : 3;
      for (int i = 0; i < NBLK; i++) {
         int bx = i % 256, by = i / 256;
         int rgb[16][3], v[16], v2[16];
         for (int p = 0; p < 16; p++) {
            int x = bx * 4 + (p & 3), y = by * 4 + (p >> 2);
            for (int c = 0; c < 3; c++) rgb[p][c] = pic(kind, x, y, c);
            v[p] = pic(kind, x, y, 3);
            v2[p] = pic(kind ^ 1, x, y, 1);
         }
         uint c0, c1, a0, a1;
         bc1_enc(rgb, &c0, &c1);
         bc4_enc(v, &a0, &a1);
         /* w0/w1: BC1 colour or BC4 a, w2/w3: BC1 colour / BC4 b. */
         blocks[set][i][0] = c0; blocks[set][i][1] = c1;
         blocks[set][i][2] = a0; blocks[set][i][3] = a1;
         (void)v2;
      }
   }

   int fails = 0;
   printf("opts=%#x, %d blocks per set, sets: random, pic0 (waves+edges), pic2 (flat+shapes), pic3 (smooth)\n",
          opts, NBLK);
   printf("%-5s %10s %10s %7s  (us per 1k blocks, old/new, all sets)\n", "mode", "old", "new", "speedup");
   double tot_old = 0, tot_new = 0;
   for (uint mode = 0; mode < 8; mode++) {
      double t_old = 0, t_new = 0;
      for (int set = 0; set < 4; set++) {
         for (int i = 0; i < NBLK; i++) {
            uint w[4];
            memcpy(w, blocks[set][i], sizeof(w));
            /* Pictures: BC2/BC3 put alpha first, BC4/BC5 use the BC4 words. */
            if (set && (mode == 2 || mode == 3 || mode >= 4)) {
               uint t0 = w[0], t1 = w[1];
               w[0] = w[2]; w[1] = w[3]; w[2] = t0; w[3] = t1;
               if (mode >= 6) { w[2] = blocks[set][(i + 977) % NBLK][2]; w[3] = blocks[set][(i + 977) % NBLK][3]; }
            }
            uint o0[4] = {0}, o1[4] = {0};
            xc_old(mode, w, opts, o0);
            xc_new(mode, w, opts, o1);
            uint nw = (mode == 0 || mode == 1 || mode == 4 || mode == 5) ? 2 : 4;
            if (memcmp(o0, o1, nw * 4)) {
               if (fails++ < 10)
                  printf("MISMATCH %s set %d blk %d: %08x %08x %08x %08x vs %08x %08x %08x %08x\n",
                         mode_name[mode], set, i, o0[0], o0[1], o0[2], o0[3], o1[0], o1[1], o1[2], o1[3]);
            }
         }
         for (int r = 0; r < reps; r++) {
            uint o[4];
            double t = now();
            for (int i = 0; i < NBLK; i++)
               xc_old(mode, blocks[set][i], opts, o);
            t_old += now() - t;
            t = now();
            for (int i = 0; i < NBLK; i++)
               xc_new(mode, blocks[set][i], opts, o);
            t_new += now() - t;
         }
      }
      double k = 1e6 / (4.0 * NBLK * reps / 1000.0);
      printf("%-5s %10.1f %10.1f %6.2fx\n", mode_name[mode], t_old * k, t_new * k, t_old / t_new);
      tot_old += t_old; tot_new += t_new;
   }
   printf("all   %.2fx faster, mismatches %d\n", tot_old / tot_new, fails);
   return fails != 0;
}
