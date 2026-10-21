/* Synthetic test pictures and crude BC encoders for the host tests. */
#pragma once
#include <math.h>
#include <stdint.h>

static uint32_t pic_rng = 12345;
static inline uint32_t
rnd32(void)
{
   pic_rng ^= pic_rng << 13;
   pic_rng ^= pic_rng >> 17;
   pic_rng ^= pic_rng << 5;
   return pic_rng;
}

static inline int
clampi(int v, int lo, int hi)
{
   return v < lo ? lo : (v > hi ? hi : v);
}

/* kind 0: smooth waves with hard-edged inverted patches and +-4 noise (the
 * 197 picture); 1: fine noise texture; 2: flat areas with sharp shapes;
 * 3: normal-map like slowly varying channels. Channel c 0..3. */
static inline int
pic(int kind, int x, int y, int c)
{
   double v;
   switch (kind) {
   default:
   case 0:
      v = 128 + 90 * sin(x * 0.05 + c) * cos(y * 0.031 + 0.7 * c) +
          25 * sin((x + y) * 0.23 + c);
      if (((x / 37) + (y / 23)) % 5 == 0)
         v = 255 - v;
      v += (int)(rnd32() % 9) - 4;
      break;
   case 1:
      v = 120 + 40 * sin(x * 0.7 + 1.3 * c) * sin(y * 0.9) + (int)(rnd32() % 61) - 30;
      break;
   case 2: {
      int cx = x % 64 - 32, cy = y % 64 - 32;
      v = (cx * cx + cy * cy < 400) ? 40 + 60 * c : 200 - 30 * c;
      if ((x / 8 + y / 8) % 7 == 0)
         v = 20;
      break;
   }
   case 3:
      v = 128 + 100 * sin(x * 0.013 + 2 * c) * cos(y * 0.017 - c);
      break;
   }
   return clampi((int)v, 0, 255);
}

/* Crude BC1 colour block encoder (bbox endpoints). */
static inline void
bc1_enc(int rgb[16][3], uint32_t *w0, uint32_t *w1)
{
   int lo[3] = {255, 255, 255}, hi[3] = {0, 0, 0};
   for (int p = 0; p < 16; p++)
      for (int c = 0; c < 3; c++) {
         lo[c] = rgb[p][c] < lo[c] ? rgb[p][c] : lo[c];
         hi[c] = rgb[p][c] > hi[c] ? rgb[p][c] : hi[c];
      }
   uint32_t e0 = ((hi[0] * 31 / 255) << 11) | ((hi[1] * 63 / 255) << 5) | (hi[2] * 31 / 255);
   uint32_t e1 = ((lo[0] * 31 / 255) << 11) | ((lo[1] * 63 / 255) << 5) | (lo[2] * 31 / 255);
   if (e0 < e1) { uint32_t t = e0; e0 = e1; e1 = t; }
   if (e0 == e1) { *w0 = e0 | (e1 << 16); *w1 = 0; return; }
   float pal[4][3];
   float a[3] = {((e0 >> 11) & 31) / 31.0f, ((e0 >> 5) & 63) / 63.0f, (e0 & 31) / 31.0f};
   float b[3] = {((e1 >> 11) & 31) / 31.0f, ((e1 >> 5) & 63) / 63.0f, (e1 & 31) / 31.0f};
   for (int c = 0; c < 3; c++) {
      pal[0][c] = a[c] * 255; pal[1][c] = b[c] * 255;
      pal[2][c] = (a[c] + (b[c] - a[c]) / 3) * 255;
      pal[3][c] = (a[c] + (b[c] - a[c]) * 2 / 3) * 255;
   }
   uint32_t idx = 0;
   for (int p = 0; p < 16; p++) {
      int bi = 0; float be = 1e30f;
      for (int i = 0; i < 4; i++) {
         float e = 0;
         for (int c = 0; c < 3; c++) e += (pal[i][c] - rgb[p][c]) * (pal[i][c] - rgb[p][c]);
         if (e < be) { be = e; bi = i; }
      }
      idx |= bi << (2 * p);
   }
   *w0 = e0 | (e1 << 16);
   *w1 = idx;
}

/* Crude BC4 encoder (8-value mode) for 16 values 0..255. */
static inline void
bc4_enc(int v[16], uint32_t *w0, uint32_t *w1)
{
   int lo = 255, hi = 0;
   for (int p = 0; p < 16; p++) { lo = v[p] < lo ? v[p] : lo; hi = v[p] > hi ? v[p] : hi; }
   if (hi == lo) { if (hi < 255) hi++; else lo--; }
   uint64_t bits = (uint64_t)hi | ((uint64_t)lo << 8);
   for (int p = 0; p < 16; p++) {
      int bi = 0; float be = 1e30f;
      for (int i = 0; i < 8; i++) {
         float pv = i == 0 ? hi : i == 1 ? lo : hi + (lo - hi) * (i - 1) / 7.0f;
         float e = fabsf(pv - v[p]);
         if (e < be) { be = e; bi = i; }
      }
      bits |= (uint64_t)bi << (16 + 3 * p);
   }
   *w0 = (uint32_t)bits;
   *w1 = (uint32_t)(bits >> 32);
}
