/* Throughput and quality of the BC1-BC5 -> ETC2/EAC transcoder body of
 * three versions over a dim x dim texture of BC blocks:
 *   lowmem = mesa-lowmem (csf-v11/200 per-texel encoder),
 *   bc3    = mesa-bc at csf-v11/203 (Makefile BC2REV),
 *   bc4    = mesa-bc HEAD (csf-v11/206: EAC endpoint pair table, opts).
 * Opts (argv[4], default 0x30000) go to every version; the bc4 default is
 * 0x20000 and 0xc30000 is the bc3 encoder.
 * The versions are timed interleaved (one pass of each per rep), so their
 * ratio holds under thermal throttling. Inputs are deterministic: "random"
 * (random words) and "gradient" (blocks encoded from the synthetic pictures
 * of tests/bc-host/pics.h). Quality = PSNR (8-bit scale) and max abs error
 * of the ETC2/EAC decode vs the exact BC decode.
 * Usage: bc_bench [reps=5] [dim=2048] [mode mask=0xff] */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/resource.h>
#include "pics.h"
#include "etc_ref.h"

typedef unsigned int uint;
void xc_old(uint mode, const uint *w, uint opts, uint *out);
void xc_bc2(uint mode, const uint *w, uint opts, uint *out);
void xc_new(uint mode, const uint *w, uint opts, uint *out);
void xc_new_dec(uint mode, const uint *w, int rgb[16][3], int ch[16][2], bool tr[16]);

#define NV 3
static const char *ver_name[NV] = {"lowmem", "bc3", "bc4"};
static void (*ver_fn[NV])(uint, const uint *, uint, uint *) = {xc_old, xc_bc2, xc_new};
static const char *mode_name[8] = {"BC1", "BC1A", "BC2", "BC3", "BC4U", "BC4S", "BC5U", "BC5S"};

static double
now(void)
{
   struct timespec ts;
   clock_gettime(CLOCK_MONOTONIC, &ts);
   return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static int
cmpd(const void *a, const void *b)
{
   double x = *(const double *)a, y = *(const double *)b;
   return x < y ? -1 : x > y;
}

/* Squared error (8-bit scale) vs the exact BC decode; *nch channels compared;
 * *maxe = max abs error (8-bit scale). */
static double
block_sse(uint mode, const uint *w, const uint *o, int *nch, double *maxe)
{
   int rgb[16][3], ch[16][2];
   bool tr[16];
   xc_new_dec(mode, w, rgb, ch, tr);
   double sse = 0, me = 0;
#define ACC(d_) do { double e_ = fabs(d_); sse += e_ * e_; if (e_ > me) me = e_; } while (0)
   if (mode <= 3) {
      int d[16][3];
      bool dt[16];
      int m;
      bool punch = mode == 1;
      const uint *c = mode <= 1 ? o : o + 2;
      ref_rgb(c[0], c[1], punch, d, dt, &m);
      for (int p = 0; p < 16; p++) {
         if (punch && dt[p] != tr[p]) {
            sse += 3 * 255.0 * 255.0;
            me = 255;
         }
         if (tr[p])
            continue;
         for (int k = 0; k < 3; k++)
            ACC(d[p][k] - rgb[p][k]);
      }
      *nch = 3;
      if (mode >= 2) {
         int v[16];
         ref_eac(o[0], o[1], 0, v);
         for (int p = 0; p < 16; p++)
            ACC(v[p] - ch[p][0]);
         *nch = 4;
      }
      *maxe = me;
      return sse;
   }
   int kind = (mode == 5 || mode == 7) ? 2 : 1;
   double sc = ref_eac_scale(kind);
   int nc = mode >= 6 ? 2 : 1;
   for (int c = 0; c < nc; c++) {
      int v[16];
      ref_eac(o[2 * c], o[2 * c + 1], kind, v);
      for (int p = 0; p < 16; p++)
         ACC((v[p] - ch[p][c]) * sc);
   }
   *nch = nc;
   *maxe = me;
   return sse;
}

int
main(int argc, char **argv)
{
   int reps = argc > 1 ? atoi(argv[1]) : 5;
   int dim = argc > 2 ? atoi(argv[2]) : 2048;
   int mmask = argc > 3 ? (int)strtol(argv[3], 0, 0) : 0xff;
   int bw = dim / 4, nblk = bw * bw;
   uint opts = argc > 4 ? (uint)strtoul(argv[4], 0, 0) : 0x30000;
   if (reps < 1 || reps > 16)
      reps = 5;

   uint (*in)[4] = malloc((size_t)nblk * 16);
   uint (*out)[4] = malloc((size_t)nblk * 16);
   printf("dim=%d blocks=%d reps=%d (Mblk/s = median pass, versions interleaved)\n",
          dim, nblk, reps);
   printf("%-5s %-9s", "mode", "input");
   for (int v = 0; v < NV; v++)
      printf(" %7s-Mblk/s %6s-dB %6s-max", ver_name[v], ver_name[v], ver_name[v]);
   printf(" bc4/lowmem bc4/bc3\n");
   for (uint mode = 0; mode < 8; mode++) {
      if (!(mmask & (1 << mode)))
         continue;
      for (int input = 0; input < 2; input++) {
         for (int i = 0; i < nblk; i++) {
            if (input == 0) {
               for (int k = 0; k < 4; k++)
                  in[i][k] = rnd32();
               continue;
            }
            int bx = i % bw, by = i / bw;
            int kind = ((bx >> 5) + (by >> 5)) % 3;
            kind = kind == 0 ? 0 : kind == 1 ? 2 : 3;
            int rgb[16][3], v[16], v2[16];
            for (int p = 0; p < 16; p++) {
               int x = (bx * 4 + (p & 3)) & 1023, y = (by * 4 + (p >> 2)) & 1023;
               for (int c = 0; c < 3; c++)
                  rgb[p][c] = pic(kind, x, y, c);
               v[p] = pic(kind, x, y, 3);
               v2[p] = pic(kind, y, x, 1);
            }
            uint c0, c1, a0, a1, b0, b1;
            bc1_enc(rgb, &c0, &c1);
            bc4_enc(v, &a0, &a1);
            bc4_enc(v2, &b0, &b1);
            if (mode <= 1) {
               in[i][0] = c0; in[i][1] = c1; in[i][2] = 0; in[i][3] = 0;
            } else if (mode == 2) { /* 4-bit alpha from v */
               uint lo = 0, hi = 0;
               for (int p = 0; p < 16; p++) {
                  uint a = (uint)(v[p] >> 4) & 15;
                  if (p < 8) lo |= a << (4 * p); else hi |= a << (4 * (p - 8));
               }
               in[i][0] = lo; in[i][1] = hi; in[i][2] = c0; in[i][3] = c1;
            } else if (mode == 3) {
               in[i][0] = a0; in[i][1] = a1; in[i][2] = c0; in[i][3] = c1;
            } else {
               in[i][0] = a0; in[i][1] = a1; in[i][2] = b0; in[i][3] = b1;
            }
         }
         double ts[NV][16], rate[NV], psnr[NV], maxe[NV];
         for (int r = 0; r < reps; r++) {
            for (int v = 0; v < NV; v++) {
               double t = now();
               for (int i = 0; i < nblk; i++)
                  ver_fn[v](mode, in[i], opts, out[i]);
               ts[v][r] = now() - t;
            }
         }
         for (int v = 0; v < NV; v++) {
            qsort(ts[v], reps, sizeof(double), cmpd);
            rate[v] = nblk / ts[v][reps / 2] / 1e6;
            /* quality (single pass) */
            double sse = 0, n = 0, mx = 0;
            for (int i = 0; i < nblk; i++) {
               int nch;
               double me;
               ver_fn[v](mode, in[i], opts, out[i]);
               sse += block_sse(mode, in[i], out[i], &nch, &me);
               n += 16 * nch;
               if (me > mx) mx = me;
            }
            psnr[v] = sse > 0 ? 10 * log10(255.0 * 255.0 / (sse / n)) : 99.0;
            maxe[v] = mx;
         }
         printf("%-5s %-9s", mode_name[mode], input ? "gradient" : "random");
         for (int v = 0; v < NV; v++)
            printf(" %14.3f %9.3f %10.1f", rate[v], psnr[v], maxe[v]);
         printf(" %9.2fx %6.2fx\n", rate[2] / rate[0], rate[2] / rate[1]);
         fflush(stdout);
      }
   }
   struct rusage ru;
   getrusage(RUSAGE_SELF, &ru);
   printf("peak_rss_kb=%ld\n", ru.ru_maxrss);
   return 0;
}
