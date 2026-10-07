#version 450
/* GPU EAC R11 encoder (transcode-at-upload cost probe): one fragment per
 * 4x4 block, same search as enc_eac11() in bc-perf.c. Output is the 64-bit
 * block as two little-endian words of the big-endian EAC bytes. */
layout(set = 0, binding = 0) uniform sampler2D tex;
layout(location = 0) out uvec2 blk;
/* fast != 0: one multiplier (range / span, rounded up), base centre +-1. */
layout(push_constant) uniform P { int fast; } p;

const int T[128] = int[](
   -3, -6, -9, -15, 2, 5, 8, 14,   -3, -7, -10, -13, 2, 6, 9, 12,
   -2, -5, -8, -13, 1, 4, 7, 12,   -2, -4, -6, -13, 1, 3, 5, 12,
   -3, -6, -8, -12, 2, 5, 7, 11,   -3, -7, -9, -11, 2, 6, 8, 10,
   -4, -7, -8, -11, 3, 6, 7, 10,   -3, -5, -8, -11, 2, 4, 7, 10,
   -2, -6, -8, -10, 1, 5, 7, 9,    -2, -5, -8, -10, 1, 4, 7, 9,
   -2, -4, -8, -10, 1, 3, 7, 9,    -2, -5, -7, -10, 1, 4, 6, 9,
   -3, -4, -7, -10, 2, 3, 6, 9,    -1, -2, -3, -10, 0, 1, 2, 9,
   -4, -6, -8, -9, 3, 5, 7, 8,     -3, -5, -7, -9, 2, 4, 6, 8);

uint bswap(uint v)
{
   return (v >> 24) | ((v >> 8) & 0xff00u) | ((v << 8) & 0xff0000u) | (v << 24);
}

void main()
{
   ivec2 b = ivec2(gl_FragCoord.xy) * 4;
   int tg[16];
   int lo = 2047, hi = 0;
   for (int i = 0; i < 16; i++) { /* column-major EAC order */
      tg[i] = int(round(texelFetch(tex, b + ivec2(i >> 2, i & 3), 0).r * 2047.0));
      lo = min(lo, tg[i]);
      hi = max(hi, tg[i]);
   }
   uint best = 0xffffffffu, bhi = 0u, blo = 0u;
   for (int t = 0; t < 16; t++) {
      int tmin = T[t * 8 + 3], tmax = T[t * 8 + 7];
      int m0 = (hi - lo) / ((tmax - tmin) * 8);
      int mlo = p.fast != 0 ? min(m0 + 1, 15) : max(m0 - 1, 0);
      int mhi = p.fast != 0 ? mlo : min(m0 + 2, 15);
      for (int mul = mlo; mul <= mhi; mul++) {
         int mm = mul > 0 ? mul * 8 : 1;
         int c = ((lo + hi) / 2 - 4 - (tmax + tmin) * mm / 2) / 8;
         int r = p.fast != 0 ? 1 : 3;
         for (int base = max(c - r, 0); base <= min(c + r, 255); base++) {
            uint err = 0u, ih = 0u, il = 0u;
            for (int i = 0; i < 16; i++) {
               int bd = 0x7fffffff, bk = 0;
               for (int k = 0; k < 8; k++) {
                  int d = tg[i] - clamp(base * 8 + 4 + T[t * 8 + k] * mm, 0, 2047);
                  if (d * d < bd) {
                     bd = d * d;
                     bk = k;
                  }
               }
               err += uint(bd);
               /* 48-bit index field: pixel i at bits 45-3i of the block */
               int sh = 45 - 3 * i;
               if (sh >= 32)
                  ih |= uint(bk) << (sh - 32);
               else if (sh > 29) {
                  il |= uint(bk) << sh;
                  ih |= uint(bk) >> (32 - sh);
               } else
                  il |= uint(bk) << sh;
            }
            if (err < best) {
               best = err;
               bhi = uint(base) << 24 | uint(mul) << 20 | uint(t) << 16 | ih;
               blo = il;
            }
         }
      }
   }
   blk = uvec2(bswap(bhi), bswap(blo));
}
