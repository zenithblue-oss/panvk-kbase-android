/* Mesa's CPU BPTC decoder (util/format/texcompress_bptc_tmp.h) as the
 * reference of the BC6H/BC7 host tests, like tests/dxvk/vulkan/bc/bc_ref.c. */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "util/format/texcompress_bptc_tmp.h"

struct _util_cpu_caps_state_t _util_cpu_caps_state = {.detect_done = 1};
void _util_cpu_detect_once(void) {}

void
ref_bc6(const uint8_t *blk, int texel, int sgn, float rgb[3])
{
   float px[4];
   fetch_rgb_float_from_block(blk, px, texel, sgn);
   memcpy(rgb, px, 3 * sizeof(float));
}

void
ref_bc7(const uint8_t *blk, int texel, uint8_t rgba[4])
{
   fetch_rgba_unorm_from_block(blk, rgba, texel);
}
