/* One transcoder body (XC_BODY) behind a plain function (XC_FN), so two
 * versions can be linked into one test. */
#include "c_subset.h"
#include XC_BODY

void
XC_FN(uint mode, const uint *w, uint opts, uint *out)
{
   transcode_block(mode, w[0], w[1], w[2], w[3], opts);
   for (int i = 0; i < 4; i++)
      out[i] = g_out[i];
}
