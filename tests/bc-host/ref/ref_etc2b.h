static uint8_t
etc2_base_color1_t_mode(const uint8_t *in, GLuint index)
{
   uint8_t R1a = 0, x = 0;
   /* base col 1 = extend_4to8bits( (R1a << 2) | R1b, G1, B1) */
   switch(index) {
   case 0:
      R1a = (in[0] >> 3) & 0x3;
      x = ((R1a << 2) | (in[0] & 0x3));
      break;
   case 1:
      x = ((in[1] >> 4) & 0xf);
      break;
   case 2:
      x = (in[1] & 0xf);
      break;
   default:
      /* invalid index */
      break;
   }
   return ((x << 4) | (x & 0xf));
}

static uint8_t
etc2_base_color2_t_mode(const uint8_t *in, GLuint index)
{
   uint8_t x = 0;
   /*extend 4to8bits(R2, G2, B2)*/
   switch(index) {
   case 0:
      x = ((in[2] >> 4) & 0xf );
      break;
   case 1:
      x = (in[2] & 0xf);
      break;
   case 2:
      x = ((in[3] >> 4) & 0xf);
      break;
   default:
      /* invalid index */
      break;
   }
   return ((x << 4) | (x & 0xf));
}

static uint8_t
etc2_base_color1_h_mode(const uint8_t *in, GLuint index)
{
   uint8_t x = 0;
   /* base col 1 = extend 4to8bits(R1, (G1a << 1) | G1b, (B1a << 3) | B1b) */
   switch(index) {
   case 0:
      x = ((in[0] >> 3) & 0xf);
      break;
   case 1:
      x = (((in[0] & 0x7) << 1) | ((in[1] >> 4) & 0x1));
      break;
   case 2:
      x = ((in[1] & 0x8) |
           (((in[1] & 0x3) << 1) | ((in[2] >> 7) & 0x1)));
      break;
   default:
      /* invalid index */
      break;
   }
   return ((x << 4) | (x & 0xf));
}

static uint8_t
etc2_base_color2_h_mode(const uint8_t *in, GLuint index)
{
   uint8_t x = 0;
   /* base col 2 = extend 4to8bits(R2, G2, B2) */
   switch(index) {
   case 0:
      x = ((in[2] >> 3) & 0xf );
      break;
   case 1:
      x = (((in[2] & 0x7) << 1) | ((in[3] >> 7) & 0x1));
      break;
   case 2:
      x = ((in[3] >> 3) & 0xf);
      break;
   default:
      /* invalid index */
      break;
   }
   return ((x << 4) | (x & 0xf));
}

static uint8_t
etc2_base_color_o_planar(const uint8_t *in, GLuint index)
{
   GLuint tmp;
   switch(index) {
   case 0:
      tmp = ((in[0] >> 1) & 0x3f); /* RO */
      return ((tmp << 2) | (tmp >> 4));
   case 1:
      tmp = (((in[0] & 0x1) << 6) | /* GO1 */
             ((in[1] >> 1) & 0x3f)); /* GO2 */
      return ((tmp << 1) | (tmp >> 6));
   case 2:
      tmp = (((in[1] & 0x1) << 5) | /* BO1 */
             (in[2] & 0x18) | /* BO2 */
             (((in[2] & 0x3) << 1) | ((in[3] >> 7) & 0x1))); /* BO3 */
      return ((tmp << 2) | (tmp >> 4));
    default:
      /* invalid index */
      return 0;
   }
}

static uint8_t
etc2_base_color_h_planar(const uint8_t *in, GLuint index)
{
   GLuint tmp;
   switch(index) {
   case 0:
      tmp = (((in[3] & 0x7c) >> 1) | /* RH1 */
             (in[3] & 0x1));         /* RH2 */
      return ((tmp << 2) | (tmp >> 4));
   case 1:
      tmp = (in[4] >> 1) & 0x7f; /* GH */
      return ((tmp << 1) | (tmp >> 6));
   case 2:
      tmp = (((in[4] & 0x1) << 5) |
             ((in[5] >> 3) & 0x1f)); /* BH */
      return ((tmp << 2) | (tmp >> 4));
   default:
      /* invalid index */
      return 0;
   }
}

static uint8_t
etc2_base_color_v_planar(const uint8_t *in, GLuint index)
{
   GLuint tmp;
   switch(index) {
   case 0:
      tmp = (((in[5] & 0x7) << 0x3) |
             ((in[6] >> 5) & 0x7)); /* RV */
      return ((tmp << 2) | (tmp >> 4));
   case 1:
      tmp = (((in[6] & 0x1f) << 2) |
             ((in[7] >> 6) & 0x3)); /* GV */
      return ((tmp << 1) | (tmp >> 6));
   case 2:
      tmp = in[7] & 0x3f; /* BV */
      return ((tmp << 2) | (tmp >> 4));
   default:
      /* invalid index */
      return 0;
   }
}

static GLint
etc2_get_pixel_index(const struct etc2_block *block, int x, int y)
{
   int bit = ((3 - y) + (3 - x) * 4) * 3;
   int idx = (block->pixel_indices[1] >> bit) & 0x7;
   return idx;
}

static uint8_t
etc2_clamp(int color)
{
   /* CLAMP(color, 0, 255) */
   return (uint8_t) CLAMP(color, 0, 255);
}

static GLushort
etc2_clamp2(int color)
{
   /* CLAMP(color, 0, 2047) */
   return (GLushort) CLAMP(color, 0, 2047);
}

static GLshort
etc2_clamp3(int color)
{
   /* CLAMP(color, -1023, 1023) */
   return (GLshort) CLAMP(color, -1023, 1023);
}

static void
etc2_rgb8_parse_block(struct etc2_block *block,
                      const uint8_t *src,
                      GLboolean punchthrough_alpha)
{
   unsigned i;
   GLboolean diffbit = false;
   static const int lookup[8] = { 0, 1, 2, 3, -4, -3, -2, -1 };

   const int R_plus_dR = (src[0] >> 3) + lookup[src[0] & 0x7];
   const int G_plus_dG = (src[1] >> 3) + lookup[src[1] & 0x7];
   const int B_plus_dB = (src[2] >> 3) + lookup[src[2] & 0x7];

   /* Reset the mode flags */
   block->is_ind_mode = false;
   block->is_diff_mode = false;
   block->is_t_mode = false;
   block->is_h_mode = false;
   block->is_planar_mode = false;

   if (punchthrough_alpha)
      block->opaque = src[3] & 0x2;
   else
      diffbit = src[3] & 0x2;

   if (!diffbit && !punchthrough_alpha) {
      /* individual mode */
      block->is_ind_mode = true;

      for (i = 0; i < 3; i++) {
         /* Texture decode algorithm is same for individual mode in etc1
          * & etc2.
          */
         block->base_colors[0][i] = etc1_base_color_ind_hi(src[i]);
         block->base_colors[1][i] = etc1_base_color_ind_lo(src[i]);
      }
   }
   else if (R_plus_dR < 0 || R_plus_dR > 31){
      /* T mode */
      block->is_t_mode = true;

      for(i = 0; i < 3; i++) {
         block->base_colors[0][i] = etc2_base_color1_t_mode(src, i);
         block->base_colors[1][i] = etc2_base_color2_t_mode(src, i);
      }
      /* pick distance */
      block->distance =
         etc2_distance_table[(((src[3] >> 2) & 0x3) << 1) |
                             (src[3] & 0x1)];

      for (i = 0; i < 3; i++) {
         block->paint_colors[0][i] = etc2_clamp(block->base_colors[0][i]);
         block->paint_colors[1][i] = etc2_clamp(block->base_colors[1][i] +
                                                block->distance);
         block->paint_colors[2][i] = etc2_clamp(block->base_colors[1][i]);
         block->paint_colors[3][i] = etc2_clamp(block->base_colors[1][i] -
                                                block->distance);
      }
   }
   else if (G_plus_dG < 0 || G_plus_dG > 31){
      int base_color_1_value, base_color_2_value;

      /* H mode */
      block->is_h_mode = true;

      for(i = 0; i < 3; i++) {
         block->base_colors[0][i] = etc2_base_color1_h_mode(src, i);
         block->base_colors[1][i] = etc2_base_color2_h_mode(src, i);
      }

      base_color_1_value = (block->base_colors[0][0] << 16) +
                           (block->base_colors[0][1] << 8) +
                           block->base_colors[0][2];
      base_color_2_value = (block->base_colors[1][0] << 16) +
                           (block->base_colors[1][1] << 8) +
                           block->base_colors[1][2];
      /* pick distance */
      block->distance =
         etc2_distance_table[(src[3] & 0x4) |
                             ((src[3] & 0x1) << 1) |
                             (base_color_1_value >= base_color_2_value)];

      for (i = 0; i < 3; i++) {
         block->paint_colors[0][i] = etc2_clamp(block->base_colors[0][i] +
                                                block->distance);
         block->paint_colors[1][i] = etc2_clamp(block->base_colors[0][i] -
                                                block->distance);
         block->paint_colors[2][i] = etc2_clamp(block->base_colors[1][i] +
                                                block->distance);
         block->paint_colors[3][i] = etc2_clamp(block->base_colors[1][i] -
                                                block->distance);
      }
   }
   else if (B_plus_dB < 0 || B_plus_dB > 31) {
      /* Planar mode */
      block->is_planar_mode = true;

      /* opaque bit must be set in planar mode */
      block->opaque = true;

      for (i = 0; i < 3; i++) {
         block->base_colors[0][i] = etc2_base_color_o_planar(src, i);
         block->base_colors[1][i] = etc2_base_color_h_planar(src, i);
         block->base_colors[2][i] = etc2_base_color_v_planar(src, i);
      }
   }
   else if (diffbit || punchthrough_alpha) {
      /* differential mode */
      block->is_diff_mode = true;

      for (i = 0; i < 3; i++) {
         /* Texture decode algorithm is same for differential mode in etc1
          * & etc2.
          */
         block->base_colors[0][i] = etc1_base_color_diff_hi(src[i]);
         block->base_colors[1][i] = etc1_base_color_diff_lo(src[i]);
      }
   }

   if (block->is_ind_mode || block->is_diff_mode) {
      int table1_idx = (src[3] >> 5) & 0x7;
      int table2_idx = (src[3] >> 2) & 0x7;

      /* Use same modifier tables as for etc1 textures if opaque bit is set
       * or if non punchthrough texture format
       */
      block->modifier_tables[0] = (!punchthrough_alpha || block->opaque) ?
                                  etc1_modifier_tables[table1_idx] :
                                  etc2_modifier_tables_non_opaque[table1_idx];
      block->modifier_tables[1] = (!punchthrough_alpha || block->opaque) ?
                                  etc1_modifier_tables[table2_idx] :
                                  etc2_modifier_tables_non_opaque[table2_idx];

      block->flipped = (src[3] & 0x1);
   }

   block->pixel_indices[0] =
      (src[4] << 24) | (src[5] << 16) | (src[6] << 8) | src[7];
}

static void
etc2_rgb8_fetch_texel(const struct etc2_block *block,
                      int x, int y, uint8_t *dst,
                      GLboolean punchthrough_alpha)
{
   const uint8_t *base_color;
   int modifier, bit, idx, blk;

   /* get pixel index */
   bit = y + x * 4;
   idx = ((block->pixel_indices[0] >> (15 + bit)) & 0x2) |
         ((block->pixel_indices[0] >>      (bit)) & 0x1);

   if (block->is_ind_mode || block->is_diff_mode) {
      /* check for punchthrough_alpha format */
      if (punchthrough_alpha) {
         if (!block->opaque && idx == 2) {
            dst[0] = dst[1] = dst[2] = dst[3] = 0;
            return;
         }
         else
            dst[3] = 255;
      }

      /* Use pixel index and subblock to get the modifier */
      blk = (block->flipped) ? (y >= 2) : (x >= 2);
      base_color = block->base_colors[blk];
      modifier = block->modifier_tables[blk][idx];

      dst[0] = etc2_clamp(base_color[0] + modifier);
      dst[1] = etc2_clamp(base_color[1] + modifier);
      dst[2] = etc2_clamp(base_color[2] + modifier);
   }
   else if (block->is_t_mode || block->is_h_mode) {
      /* check for punchthrough_alpha format */
      if (punchthrough_alpha) {
         if (!block->opaque && idx == 2) {
            dst[0] = dst[1] = dst[2] = dst[3] = 0;
            return;
         }
         else
            dst[3] = 255;
      }

      /* Use pixel index to pick one of the paint colors */
      dst[0] = block->paint_colors[idx][0];
      dst[1] = block->paint_colors[idx][1];
      dst[2] = block->paint_colors[idx][2];
   }
   else if (block->is_planar_mode) {
      /* {R(x, y) = clamp255((x × (RH − RO) + y × (RV − RO) + 4 × RO + 2) >> 2)
       * {G(x, y) = clamp255((x × (GH − GO) + y × (GV − GO) + 4 × GO + 2) >> 2)
       * {B(x, y) = clamp255((x × (BH − BO) + y × (BV − BO) + 4 × BO + 2) >> 2)
       */
      int red, green, blue;
      red = (x * (block->base_colors[1][0] - block->base_colors[0][0]) +
             y * (block->base_colors[2][0] - block->base_colors[0][0]) +
             4 * block->base_colors[0][0] + 2) >> 2;

      green = (x * (block->base_colors[1][1] - block->base_colors[0][1]) +
               y * (block->base_colors[2][1] - block->base_colors[0][1]) +
               4 * block->base_colors[0][1] + 2) >> 2;

      blue = (x * (block->base_colors[1][2] - block->base_colors[0][2]) +
              y * (block->base_colors[2][2] - block->base_colors[0][2]) +
              4 * block->base_colors[0][2] + 2) >> 2;

      dst[0] = etc2_clamp(red);
      dst[1] = etc2_clamp(green);
      dst[2] = etc2_clamp(blue);

      /* check for punchthrough_alpha format */
      if (punchthrough_alpha)
         dst[3] = 255;
   }
   else
      UNREACHABLE("unhandled block mode");
}

static void
etc2_alpha8_fetch_texel(const struct etc2_block *block,
      int x, int y, uint8_t *dst)
{
   int modifier, alpha, idx;
   /* get pixel index */
   idx = etc2_get_pixel_index(block, x, y);
   modifier = etc2_modifier_tables[block->table_index][idx];
   alpha = block->base_codeword + modifier * block->multiplier;
   dst[3] = etc2_clamp(alpha);
}

static void
etc2_r11_fetch_texel(const struct etc2_block *block,
                     int x, int y, uint8_t *dst)
{
   GLint modifier, idx;
   GLshort color;
   /* Get pixel index */
   idx = etc2_get_pixel_index(block, x, y);
   modifier = etc2_modifier_tables[block->table_index][idx];

   if (block->multiplier != 0)
      /* clamp2(base codeword × 8 + 4 + modifier × multiplier × 8) */
      color = etc2_clamp2(((block->base_codeword << 3) | 0x4)  +
                          ((modifier * block->multiplier) << 3));
   else
      color = etc2_clamp2(((block->base_codeword << 3) | 0x4)  + modifier);

   /* Extend 11 bits color value to 16 bits. OpenGL ES 3.0 specification
    * allows extending the color value to any number of bits. But, an
    * implementation is not allowed to truncate the 11-bit value to less than
    * 11 bits."
    */
   color = (color << 5) | (color >> 6);
   ((GLushort *)dst)[0] = color;
}

static void
etc2_signed_r11_fetch_texel(const struct etc2_block *block,
                            int x, int y, uint8_t *dst)
{
   GLint modifier, idx;
   GLshort color;
   GLbyte base_codeword = (GLbyte) block->base_codeword;

   if (base_codeword == -128)
      base_codeword = -127;

   /* Get pixel index */
   idx = etc2_get_pixel_index(block, x, y);
   modifier = etc2_modifier_tables[block->table_index][idx];

   if (block->multiplier != 0)
      /* clamp3(base codeword × 8 + modifier × multiplier × 8) */
      color = etc2_clamp3((base_codeword << 3)  +
                         ((modifier * block->multiplier) << 3));
   else
      color = etc2_clamp3((base_codeword << 3)  + modifier);

   /* Extend 11 bits color value to 16 bits. OpenGL ES 3.0 specification
    * allows extending the color value to any number of bits. But, an
    * implementation is not allowed to truncate the 11-bit value to less than
    * 11 bits. A negative 11-bit value must first be made positive before bit
    * replication, and then made negative again
    */
   if (color >= 0)
      color = (color << 5) | (color >> 5);
   else {
      color = -color;
      color = (color << 5) | (color >> 5);
      color = -color;
   }
   ((GLshort *)dst)[0] = color;
}

static void
etc2_alpha8_parse_block(struct etc2_block *block, const uint8_t *src)
{
   block->base_codeword = src[0];
   block->multiplier = (src[1] >> 4) & 0xf;
   block->table_index = src[1] & 0xf;
   block->pixel_indices[1] = (((uint64_t)src[2] << 40) |
                              ((uint64_t)src[3] << 32) |
                              ((uint64_t)src[4] << 24) |
                              ((uint64_t)src[5] << 16) |
                              ((uint64_t)src[6] << 8)  |
                              ((uint64_t)src[7]));
}

static void
etc2_r11_parse_block(struct etc2_block *block, const uint8_t *src)
{
   /* Parsing logic remains same as for etc2_alpha8_parse_block */
    etc2_alpha8_parse_block(block, src);
}

