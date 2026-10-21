/* Mesa's ETC2/EAC decoder (ref/, verbatim from texcompress_etc.c) behind
 * two block decode helpers for the host tests. */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned GLuint;
typedef short GLshort;
typedef unsigned short GLushort;
typedef int GLint;
typedef signed char GLbyte;
typedef bool GLboolean;
#define TAG(x) x
#define UINT8_TYPE uint8_t
#define UNREACHABLE(x) abort()
#ifndef CLAMP
#define CLAMP(x, a, b) ((x) < (a) ? (a) : ((x) > (b) ? (b) : (x)))
#endif
#include "ref_etc1.h"
#include "ref_etc2a.h"
#include "ref_etc2b.h"

static inline void
ref_words_to_bytes(unsigned w0, unsigned w1, uint8_t *b)
{
   for (int i = 0; i < 4; i++) {
      b[i] = (w0 >> (8 * i)) & 0xff;
      b[4 + i] = (w1 >> (8 * i)) & 0xff;
   }
}

/* ETC2 RGB8 / RGB8A1 block: rgb[p][c], transparent[p], mode (0 ind,
 * 1 diff, 2 T, 3 H, 4 planar). */
static inline void
ref_rgb(unsigned w0, unsigned w1, bool punch, int rgb[16][3], bool tr[16],
        int *mode)
{
   uint8_t b[8];
   struct etc2_block blk;
   memset(&blk, 0, sizeof(blk));
   ref_words_to_bytes(w0, w1, b);
   etc2_rgb8_parse_block(&blk, b, punch);
   *mode = blk.is_ind_mode ? 0 : blk.is_diff_mode ? 1 : blk.is_t_mode ? 2 :
           blk.is_h_mode ? 3 : 4;
   for (int p = 0; p < 16; p++) {
      uint8_t d[4] = {0, 0, 0, 255};
      etc2_rgb8_fetch_texel(&blk, p & 3, p >> 2, d, punch);
      for (int c = 0; c < 3; c++)
         rgb[p][c] = d[c];
      tr[p] = punch && d[3] == 0;
   }
}

/* EAC block in the 'kind' domain (0: alpha 0..255, 1: R11 0..2047,
 * 2: signed R11 -1023..1023), without Mesa's bit replication. */
static inline void
ref_eac(unsigned w0, unsigned w1, int kind, int v[16])
{
   uint8_t b[8];
   struct etc2_block blk;
   memset(&blk, 0, sizeof(blk));
   ref_words_to_bytes(w0, w1, b);
   etc2_alpha8_parse_block(&blk, b);
   for (int p = 0; p < 16; p++) {
      int x = p & 3, y = p >> 2;
      if (kind == 0) {
         uint8_t d[4];
         etc2_alpha8_fetch_texel(&blk, x, y, d);
         v[p] = d[3];
         continue;
      }
      int idx = etc2_get_pixel_index(&blk, x, y);
      int mod = etc2_modifier_tables[blk.table_index][idx];
      if (kind == 1) {
         v[p] = blk.multiplier
                   ? etc2_clamp2(((blk.base_codeword << 3) | 4) + ((mod * blk.multiplier) << 3))
                   : etc2_clamp2(((blk.base_codeword << 3) | 4) + mod);
      } else {
         GLbyte bc = (GLbyte)blk.base_codeword;
         if (bc == -128)
            bc = -127;
         v[p] = blk.multiplier ? etc2_clamp3((bc << 3) + ((mod * blk.multiplier) << 3))
                               : etc2_clamp3((bc << 3) + mod);
      }
   }
}

/* 8-bit scale of an EAC domain value step. */
static inline double
ref_eac_scale(int kind)
{
   return kind == 0 ? 1.0 : kind == 1 ? 255.0 / 2047.0 : 127.5 / 1023.0;
}
