// C wrapper around the ARM astc-encoder decoder (reference for test_astc).
#include <cstdint>
#include <cstdio>
#include "astcenc.h"

static astcenc_context *ctx;

extern "C" int
astc_ref_init(void)
{
   astcenc_config cfg;
   if (astcenc_config_init(ASTCENC_PRF_HDR, 4, 4, 1, ASTCENC_PRE_FASTEST, 0, &cfg) ||
       astcenc_context_alloc(&cfg, 1, &ctx))
      return -1;
   return 0;
}

/* Decode one 4x4 block to RGBA fp16. Returns 0 on success; *err_block and
 * *cem describe the block (astcenc_get_block_info). */
extern "C" int
astc_ref_decode(const uint8_t *blk, uint16_t *rgba16, int *err_block, int *cem)
{
   uint8_t data[16];
   for (int i = 0; i < 16; i++)
      data[i] = blk[i];
   void *slices[1] = {rgba16};
   astcenc_image img = {4, 4, 1, ASTCENC_TYPE_F16, slices};
   astcenc_swizzle swz = {ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A};
   if (astcenc_decompress_image(ctx, data, 16, &img, &swz, 0) != ASTCENC_SUCCESS)
      return -1;
   astcenc_decompress_reset(ctx);
   astcenc_block_info info;
   astcenc_get_block_info(ctx, data, &info);
   *err_block = info.is_error_block;
   *cem = info.color_endpoint_modes[0];
   return 0;
}
