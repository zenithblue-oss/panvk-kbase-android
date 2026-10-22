/* One transcoder body (XC_BODY) behind a plain function (XC_FN), so two
 * versions can be linked into one test. XC_NEW: the current body (scalar
 * g_out0..3), which also exports the exact BC decode xc_new_dec(). */
#include <string.h>
#include "c_subset.h"
#include XC_BODY

#ifdef XC_LUT
/* csf-v11/206: the expanded EAC endpoint pair tables (eac_fast()). */
#include "panvk_bc_eac_lut.h"
static uint32_t xc_lut_tab[PANVK_BC_EAC_KINDS][2 * PANVK_BC_EAC_KEYS];
static bool xc_lut_init;
#endif

void
XC_FN(uint mode, const uint *w, uint opts, uint *out)
{
#ifdef XC_LUT
   if (!xc_lut_init) {
      for (int k = 0; k < PANVK_BC_EAC_KINDS; k++)
         panvk_bc_eac_expand(k, xc_lut_tab[k]);
      xc_lut_init = true;
   }
   xc_eac_lut = xc_lut_tab[mode == XC_BC3 ? 0 : (mode == XC_BC4_S || mode == XC_BC5_S) ? 2 : 1];
#endif
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
         rgb[p][0] = pal_r(k);
         rgb[p][1] = pal_g(k);
         rgb[p][2] = pal_b(k);
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
