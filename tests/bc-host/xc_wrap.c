/* One transcoder body (XC_BODY) behind a plain function (XC_FN), so two
 * versions can be linked into one test. XC_NEW: the current body (scalar
 * g_out0..3), which also exports the exact BC decode xc_new_dec(). */
#include <string.h>
#include "c_subset.h"
#include XC_BODY

void
XC_FN(uint mode, const uint *w, uint opts, uint *out)
{
   transcode_block(mode, w[0], w[1], w[2], w[3], opts);
#ifdef XC_NEW
   out[0] = g_out0;
   out[1] = g_out1;
   out[2] = g_out2;
   out[3] = g_out3;
#else
   for (int i = 0; i < 4; i++)
      out[i] = g_out[i];
#endif
}

#ifdef XC_NEW
/* Exact BC decode of a block for 'mode': rgb[p][c] (BC1 / BC2-3 colour),
 * ch[p][0..1] (alpha or RGTC channels, in the EAC domain of the mode),
 * tr[p] (BC1A transparent texels). */
void
xc_new_dec(uint mode, const uint *w, int rgb[16][3], int ch[16][2], bool tr[16])
{
   memset(rgb, 0, 16 * 3 * sizeof(int));
   memset(ch, 0, 16 * 2 * sizeof(int));
   memset(tr, 0, 16 * sizeof(bool));
   if (mode <= XC_BC3) {
      bool bc1 = mode <= XC_BC1_RGBA;
      bc_decode_color(bc1 ? w[0] : w[2], bc1 ? w[1] : w[3], !bc1);
      for (int p = 0; p < 16; p++) {
         int k = (g_ci >> (2 * p)) & 3;
         int c = pal_c(k);
         rgb[p][0] = c_r(c);
         rgb[p][1] = c_g(c);
         rgb[p][2] = c_b(c);
         tr[p] = mode == XC_BC1_RGBA && g_c3t && k == 3;
      }
      if (mode == XC_BC2) {
         for (int p = 0; p < 16; p++)
            ch[p][0] = bc2_v(w[0], w[1], p);
      } else if (mode == XC_BC3) {
         bc_decode_rgtc(w[0], w[1], false, 0);
         for (int p = 0; p < 16; p++)
            ch[p][0] = pv(texel_bi(p));
      }
      return;
   }
   bool snorm = mode == XC_BC4_S || mode == XC_BC5_S;
   for (int c = 0; c < (mode >= XC_BC5_U ? 2 : 1); c++) {
      bc_decode_rgtc(w[2 * c], w[2 * c + 1], snorm, snorm ? 2 : 1);
      for (int p = 0; p < 16; p++)
         ch[p][c] = pv(texel_bi(p));
   }
}
#endif
