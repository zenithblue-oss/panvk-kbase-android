struct TAG(etc1_block) {
   uint32_t pixel_indices;
   int flipped;
   const int *modifier_tables[2];
   UINT8_TYPE base_colors[2][3];
};

static UINT8_TYPE
TAG(etc1_base_color_diff_hi)(UINT8_TYPE in)
{
   return (in & 0xf8) | (in >> 5);
}

static UINT8_TYPE
TAG(etc1_base_color_diff_lo)(UINT8_TYPE in)
{
   static const int lookup[8] = { 0, 1, 2, 3, -4, -3, -2, -1 };

   in = (in >> 3) + lookup[in & 0x7];

   return (in << 3) | (in >> 2);
}

static UINT8_TYPE
TAG(etc1_base_color_ind_hi)(UINT8_TYPE in)
{
   return (in & 0xf0) | ((in & 0xf0) >> 4);
}

static UINT8_TYPE
TAG(etc1_base_color_ind_lo)(UINT8_TYPE in)
{
   return ((in & 0xf) << 4) | (in & 0xf);
}

static UINT8_TYPE
TAG(etc1_clamp)(UINT8_TYPE base, int modifier)
{
   int tmp = (int) base + modifier;

   /* CLAMP(tmp, 0, 255) */
   return (UINT8_TYPE) ((tmp < 0) ? 0 : ((tmp > 255) ? 255 : tmp));
}

static const int TAG(etc1_modifier_tables)[8][4] = {
   {  2,   8,  -2,   -8},
   {  5,  17,  -5,  -17},
   {  9,  29,  -9,  -29},
   { 13,  42, -13,  -42},
   { 18,  60, -18,  -60},
   { 24,  80, -24,  -80},
   { 33, 106, -33, -106},
   { 47, 183, -47, -183}
};
