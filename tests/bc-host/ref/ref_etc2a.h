struct etc2_block {
   int distance;
   uint64_t pixel_indices[2];
   const int *modifier_tables[2];
   bool flipped;
   bool opaque;
   bool is_ind_mode;
   bool is_diff_mode;
   bool is_t_mode;
   bool is_h_mode;
   bool is_planar_mode;
   uint8_t base_colors[3][3];
   uint8_t paint_colors[4][3];
   uint8_t base_codeword;
   uint8_t multiplier;
   uint8_t table_index;
};

static const int etc2_distance_table[8] = {
   3, 6, 11, 16, 23, 32, 41, 64 };

static const int etc2_modifier_tables[16][8] = {
   {  -3,   -6,   -9,  -15,   2,   5,   8,   14},
   {  -3,   -7,  -10,  -13,   2,   6,   9,   12},
   {  -2,   -5,   -8,  -13,   1,   4,   7,   12},
   {  -2,   -4,   -6,  -13,   1,   3,   5,   12},
   {  -3,   -6,   -8,  -12,   2,   5,   7,   11},
   {  -3,   -7,   -9,  -11,   2,   6,   8,   10},
   {  -4,   -7,   -8,  -11,   3,   6,   7,   10},
   {  -3,   -5,   -8,  -11,   2,   4,   7,   10},
   {  -2,   -6,   -8,  -10,   1,   5,   7,    9},
   {  -2,   -5,   -8,  -10,   1,   4,   7,    9},
   {  -2,   -4,   -8,  -10,   1,   3,   7,    9},
   {  -2,   -5,   -7,  -10,   1,   4,   6,    9},
   {  -3,   -4,   -7,  -10,   2,   3,   6,    9},
   {  -1,   -2,   -3,  -10,   0,   1,   2,    9},
   {  -4,   -6,   -8,   -9,   3,   5,   7,    8},
   {  -3,   -5,   -7,   -9,   2,   4,   6,    8},
};

static const int etc2_modifier_tables_non_opaque[8][4] = {
   { 0,   8,   0,    -8},
   { 0,   17,  0,   -17},
   { 0,   29,  0,   -29},
   { 0,   42,  0,   -42},
   { 0,   60,  0,   -60},
   { 0,   80,  0,   -80},
   { 0,   106, 0,  -106},
   { 0,   183, 0,  -183}
};
