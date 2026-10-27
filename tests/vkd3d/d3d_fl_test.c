/* Host unit test for d3d_fl.h (no GPU): cc -O1 -o /tmp/d3d_fl_test tests/vkd3d/d3d_fl_test.c && /tmp/d3d_fl_test
 * Cases: G615 as documented in docs/vkd3d-vulkan-requirements.md (2026-10-06: FL11_1, SM6.0, binding tier 3,
 * tiled 0, conservative 1, no ROVs, robustImageAccess2 missing), and the build 115 PanProbe logs of
 * G715 (v11), G72 (v6) and G78 (v9) (worklogs/g615-dxvk/device-runs/2026-10-26-ftl-*-115). */
#undef NDEBUG
#include <assert.h>
#include "d3d_fl.h"

static int
has_block(const struct d3d_fl_result *r, int api, int level, const char *cap_prefix)
{
   for (int i = 0; i < r->nblock; i++)
      if (r->blk[i].api == api && r->blk[i].level == level && !strncmp(r->blk[i].cap, cap_prefix, strlen(cap_prefix)))
         return 1;
   return 0;
}

static int
count_blocks(const struct d3d_fl_result *r, int api, int level)
{
   int n = 0;
   for (int i = 0; i < r->nblock; i++)
      n += r->blk[i].api == api && r->blk[i].level == level;
   return n;
}

/* Common Valhall/Bifrost PanVK caps (no sparse, no ROV, no VRS/mesh/RT). */
static struct d3d_fl_caps
mali_base(void)
{
   struct d3d_fl_caps c;
   memset(&c, 0, sizeof(c));
   c.dxvk_base = 1;
   c.drawIndirectFirstInstance = c.fragmentStoresAndAtomics = c.multiDrawIndirect = c.tessellationShader = 1;
   c.logicOp = c.vertexPipelineStoresAndAtomics = 1;
   c.maxPerStageStorageBuffers = c.maxPerStageStorageImages = 1048576;
   c.residencyStandard2DBlockShape = 1; /* PanVK reports it even without sparse */
   c.samplerFilterMinmax = c.filterMinmaxSingleComponentFormats = 1;
   c.typedUavFormats = 18;
   c.waveOps = c.scalarOrStdLayout = c.shaderInt16 = c.shaderInt8 = c.shaderInt64 = 1;
   c.fp32Ftz = c.fp32Preserve = 1;
   c.transferTimestamps = 1;
   return c;
}

int
main(void)
{
   struct d3d_fl_result r;

   /* ---- G615, docs/vkd3d-vulkan-requirements.md (2026-10-06) ---- */
   struct d3d_fl_caps g615 = mali_base();
   g615.proton_hard_missing = 1; /* robustImageAccess2 (doc 3.2 P6) */
   g615.conservativeExt = 1;     /* "conservative tier 1 ok" */
   g615.denormIndependence = 0;  /* "denormBehaviorIndependence = NONE" -> SM6.0 */
   g615.computeDerivativeGroupLinear = g615.shaderBufferInt64Atomics = g615.fixedOrComputeSubgroupSize = 1; /* 6.6 bits */
   g615.maximalReconvergence = g615.quadControl = 1;                                                     /* 6.7/6.8 bits */
   g615.shaderOutputLayer = 1;        /* layer yes, viewport index no */
   g615.shaderImageInt64Atomics = 1;  /* "sampler feedback 0.9: bits present" */
   d3d_fl_eval(&g615, &r);
   assert(r.d12_max == 0 && !r.d12[D12_110]); /* vkd3d-proton device creation fails on robustImageAccess2 */
   assert(has_block(&r, 12, D12_110, "vkd3d-proton device creation"));
   assert(r.sm == 60 && r.binding == 3 && r.tiled == 0 && r.cons == 1 && !r.rov && r.typed_uav && r.wave_ops);
   assert(r.sfb == 9 && r.vrs == 0 && r.mesh == 0 && r.dxr == 0 && !r.vprt);
   g615.proton_hard_missing = 0; /* doc: "With that fixed it would report FL11_1, SM6.0, tier 3, tiled tier 0, conservative tier 1" */
   d3d_fl_eval(&g615, &r);
   assert(r.d12[D12_110] && r.d12[D12_111] && !r.d12[D12_120] && !r.d12[D12_121] && !r.d12[D12_122]);
   assert(r.d12_max == 2); /* 11_1 */
   assert(count_blocks(&r, 12, D12_111) == 0);
   assert(count_blocks(&r, 12, D12_120) == 1 && has_block(&r, 12, D12_120, "TiledResourcesTier >= 2 (have 0)"));
   assert(count_blocks(&r, 12, D12_121) == 1 && has_block(&r, 12, D12_121, "ROVsSupported")); /* conservative tier 1 ok */
   assert(has_block(&r, 12, D12_122, "HighestShaderModel >= 6.5 (have 6.0)"));
   assert(has_block(&r, 12, D12_122, "DepthBoundsTestSupported") && has_block(&r, 12, D12_122, "VPAndRTArrayIndex"));
   assert(!has_block(&r, 12, D12_122, "SamplerFeedbackTier") && !has_block(&r, 12, D12_122, "Int64ShaderOps"));
   for (int i = 0; i < r.nblock; i++)
      if (r.blk[i].level == D12_120 && r.blk[i].api == 12) /* names every missing Vulkan item, incl. the queue */
         assert(strstr(r.blk[i].need, "sparseBinding") && strstr(r.blk[i].need, "sparse-binding queue") &&
                strstr(r.blk[i].need, "residencyNonResidentStrict") && !strstr(r.blk[i].need, "residencyStandard2DBlockShape"));
   assert(r.d11_max == 4 && r.d11[D11_111] && !r.d11[D11_120]); /* DXVK: FL11_1 */
   assert(has_block(&r, 11, D11_120, "TiledResourcesTier >= 2 (have 0)") && has_block(&r, 11, D11_121, "ROVsSupported"));
   assert(!has_block(&r, 11, D11_121, "ConservativeRasterizationTier"));
   /* Fixing float-controls denorm reporting alone lifts SM6.0 to SM6.8 (doc 3.4). */
   g615.denormIndependence = 1;
   d3d_fl_eval(&g615, &r);
   assert(r.sm == 68 && !has_block(&r, 12, D12_122, "HighestShaderModel"));

   /* ---- G715 (v11), build 115 shiba log: FL11_1, SM6.8, tier 3, tiled 0, conservative 1 ---- */
   struct d3d_fl_caps v11 = g615;
   v11.shaderOutputViewportIndex = v11.depthBounds = 1;
   d3d_fl_eval(&v11, &r);
   assert(r.d12_max == 2 && r.sm == 68 && r.tiled == 0 && r.cons == 1 && r.vprt);
   assert(!has_block(&r, 12, D12_122, "DepthBoundsTestSupported") && !has_block(&r, 12, D12_122, "VPAndRT"));

   /* ---- G72 (v6), build 115 starlte log: FL11_0 (32 storage images per stage), SM6.5, conservative 0 ---- */
   struct d3d_fl_caps v6 = mali_base();
   v6.maxPerStageStorageImages = 32;
   v6.maxPerStageStorageBuffers = 64;
   v6.denormIndependence = 1;  /* ALL on v6/v7 */
   v6.depthBounds = 1;
   d3d_fl_eval(&v6, &r);
   assert(r.d12_max == 1 && r.d12[D12_110] && !r.d12[D12_111] && r.sm == 65 && r.cons == 0 && r.sfb == 0);
   assert(count_blocks(&r, 12, D12_111) == 1 && has_block(&r, 12, D12_111, "64 UAV slots (images, have 32)"));
   assert(has_block(&r, 12, D12_121, "ConservativeRasterizationTier >= 1") && has_block(&r, 12, D12_121, "ROVsSupported"));
   assert(r.d11_max == 4); /* DXVK has no UAV-slot gate: FL11_1 (dxvk_reqs d3d11_max_feature_level 11_1) */

   /* ---- G78 (v9), build 115 oriole log: FL11_1, SM6.8, conservative 0 ---- */
   struct d3d_fl_caps v9 = v11;
   v9.conservativeExt = 0;
   v9.shaderOutputLayer = 0;
   d3d_fl_eval(&v9, &r);
   assert(r.d12_max == 2 && r.sm == 68 && r.cons == 0 && count_blocks(&r, 12, D12_121) == 2);

   /* ---- everything: 12_2 and DXVK 12_1 ---- */
   struct d3d_fl_caps all = v11;
   all.sparseBinding = all.sparseResidencyBuffer = all.sparseResidencyImage2D = all.sparseResidencyImage3D = 1;
   all.sparseResidencyAliased = all.residencyStandard3DBlockShape = all.residencyNonResidentStrict = 1;
   all.shaderResourceResidency = all.shaderResourceMinLod = all.sparseQueue = 1;
   all.pixelInterlock = all.sampleInterlock = 1;
   all.degenerateTrianglesRasterized = all.fullyCoveredFragmentShaderInputVariable = 1;
   all.rayTracingPipeline = all.accelerationStructure = all.rayQuery = all.rayTraversalPrimitiveCulling = 1;
   all.pipelineFsr = all.attachmentFsr = all.primitiveFsr = all.meshShader = all.taskShader = 1;
   d3d_fl_eval(&all, &r);
   assert(r.tiled == 4 && r.cons == 3 && r.rov && r.dxr == 11 && r.vrs == 2 && r.mesh);
   assert(r.d12_max == 5 && r.nblock == 0 && r.d11_max == D11_N && r.d11_tiled == 3);
   /* 12_1 without 12_2 extras; aligned mip size drops tiled to tier 1 (and 12_0 with it). */
   all.meshShader = 0;
   d3d_fl_eval(&all, &r);
   assert(r.d12_max == 4 && count_blocks(&r, 12, D12_122) == 1);
   all.residencyAlignedMipSize = 1;
   d3d_fl_eval(&all, &r);
   assert(r.tiled == 1 && r.d12_max == 2 && r.d11_tiled == 1 && r.d11_max == 4);
   /* Sample interlock missing: D3D12 ROVs off (pixel + sample), DXVK ROVs still on (pixel only). */
   all.residencyAlignedMipSize = 0;
   all.sampleInterlock = 0;
   d3d_fl_eval(&all, &r);
   assert(!r.rov && r.d12_max == 3 && r.d11_max == D11_N);

   /* ---- arch detection ---- */
   assert(d3d_fl_arch(0x13b5, 0xb0080000u, "Mali-G715 MC7") == 11);
   assert(d3d_fl_arch(0, 0, "Mali-G52 r1 MC2") == 7);
   assert(d3d_fl_arch(0, 0, "Mali-G71 MP8") == 6);
   assert(d3d_fl_arch(0, 0, "Mali-G710 MC7") == 10);
   assert(d3d_fl_arch(0, 0, "Mali-G1-Ultra MC12") == 14);
   assert(d3d_fl_arch(0, 0, "Immortalis-G925 MC12") == 13);
   assert(d3d_fl_arch(0, 0, "llvmpipe") == 0);
   assert(strstr(d3d_fl_reach(10, R_CONS), "hardware cannot") && strstr(d3d_fl_reach(11, R_CONS), "driver work"));
   assert(strstr(d3d_fl_reach(11, R_TILED), "needs kernel") && strstr(d3d_fl_reach(7, R_UAV64), "32 per stage"));

   printf("d3d_fl_test: all checks passed\n");
   return 0;
}
