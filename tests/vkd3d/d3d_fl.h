/* D3D feature levels from Vulkan caps, decided the way the translation layers decide them:
 *   D3D12: vkd3d-proton 2230755 (libs/vkd3d/device.c d3d12_device_caps_init_feature_level and the tier helpers,
 *          docs/vkd3d-vulkan-requirements.md 3.3-3.5).
 *   D3D11: DXVK 3.1.1 (src/d3d11/d3d11_features.cpp:186-215, docs/panprobe-dxvk-coverage.md).
 * Pure functions over struct d3d_fl_caps (no Vulkan calls): used by d3d_fl.c (PanProbe checker) and by
 * d3d_fl_test.c (host unit test: cc -o t tests/vkd3d/d3d_fl_test.c && ./t).
 * Per-arch reachability (d3d_fl_reach): worklogs/g615-dxvk/d3d-feature-levels-2026-10-27/README.md. */
#ifndef D3D_FL_H
#define D3D_FL_H
#include <stdio.h>
#include <string.h>

struct d3d_fl_caps {
   int proton_hard_missing; /* vkd3d-proton device-creation checks that fail (the vkd3d_reqs "proton:" hard items) */
   int dxvk_base;           /* geometryShader: the D3D10 cap in DXVK's hard list (full list: dxvk_reqs) */
   /* FL11_0 (DXVK) / FL11_1 */
   int drawIndirectFirstInstance, fragmentStoresAndAtomics, multiDrawIndirect, tessellationShader;
   int logicOp, vertexPipelineStoresAndAtomics;
   unsigned maxPerStageStorageBuffers, maxPerStageStorageImages;
   /* tiled resources */
   int sparseBinding, sparseResidencyBuffer, sparseResidencyImage2D, sparseResidencyImage3D, sparseResidencyAliased;
   int residencyStandard2DBlockShape, residencyStandard3DBlockShape, residencyAlignedMipSize, residencyNonResidentStrict;
   int shaderResourceResidency, shaderResourceMinLod, samplerFilterMinmax, filterMinmaxSingleComponentFormats;
   int sparseQueue;
   int typedUavFormats; /* of the 18 typed-UAV-load formats with STORAGE_READ_WITHOUT_FORMAT */
   /* conservative raster, ROVs */
   int conservativeExt, degenerateTrianglesRasterized, fullyCoveredFragmentShaderInputVariable;
   int pixelInterlock, sampleInterlock;
   /* shader model */
   int waveOps, scalarOrStdLayout, shaderInt16, denormIndependence, fp32Ftz, fp32Preserve;
   int computeDerivativeGroupLinear, shaderBufferInt64Atomics, shaderInt8, fixedOrComputeSubgroupSize;
   int maximalReconvergence, quadControl;
   /* FL12_2 */
   int shaderOutputViewportIndex, shaderOutputLayer, shaderInt64, depthBounds, transferTimestamps;
   int rayTracingPipeline, accelerationStructure, rayQuery, rayTraversalPrimitiveCulling;
   int pipelineFsr, attachmentFsr, primitiveFsr, meshShader, taskShader, shaderImageInt64Atomics;
};

enum { D12_110, D12_111, D12_120, D12_121, D12_122, D12_N };
enum { D11_9X, D11_10X, D11_110, D11_111, D11_120, D11_121, D11_N };
/* Reachability classes of a blocking cap (d3d_fl_reach). */
enum { R_DEVICE, R_UAV64, R_TILED, R_TYPED_UAV, R_ROV, R_CONS, R_SM65, R_VPRT, R_CONS3, R_DXR, R_VRS, R_MESH, R_SFB, R_OTHER };

static const char *const d3d_fl_d12_names[D12_N] = { "11_0", "11_1", "12_0", "12_1", "12_2" };
static const char *const d3d_fl_d11_names[D11_N] = { "9_1-9_3", "10_0-10_1", "11_0", "11_1", "12_0", "12_1" };

/* One blocking cap of one level: what D3D asks for (cap) and the Vulkan side it needs (need). */
struct d3d_fl_block {
   int api, level, reach; /* api 12 or 11; level = D12_* or D11_* */
   char cap[112];
   char need[400];
};

struct d3d_fl_result {
   int sm;                   /* 51, 60, 65, 66, 68 (vkd3d-proton HighestShaderModel) */
   int binding, tiled, cons; /* ResourceBindingTier, TiledResourcesTier (0, 1, 2, 4), ConservativeRasterizationTier */
   int rov, typed_uav, wave_ops, vprt, vrs, mesh, dxr, sfb; /* dxr: 0, 10, 11; sfb: 0 or 9 (tier 0.9) */
   int d12[D12_N], d12_max; /* d12[i]: level i reachable (lower levels included); max: 0 none, 1 = 11_0 .. 5 = 12_2 */
   int d11[D11_N], d11_max, d11_tiled;
   int nblock;
   struct d3d_fl_block blk[48];
};

static void
d3d_fl_block(struct d3d_fl_result *r, int api, int level, int reach, const char *cap, const char *need)
{
   if (r->nblock >= (int)(sizeof(r->blk) / sizeof(r->blk[0])))
      return;
   struct d3d_fl_block *b = &r->blk[r->nblock++];
   b->api = api;
   b->level = level;
   b->reach = reach;
   snprintf(b->cap, sizeof(b->cap), "%s", cap);
   snprintf(b->need, sizeof(b->need), "%s", need);
}

/* Appends " [missing: a, b]" for the false entries of an (ok, name) list. */
static void
d3d_fl_missing(char *dst, size_t n, const int *ok, const char *const *names, int count)
{
   int first = 1;
   for (int i = 0; i < count; i++) {
      if (ok[i])
         continue;
      size_t l = strlen(dst);
      snprintf(dst + l, n - l, "%s%s", first ? " [missing: " : ", ", names[i]);
      first = 0;
   }
   if (!first) {
      size_t l = strlen(dst);
      snprintf(dst + l, n - l, "]");
   }
}

static int
d3d_fl_blocked(const struct d3d_fl_result *r, int api, int level)
{
   for (int i = 0; i < r->nblock; i++)
      if (r->blk[i].api == api && r->blk[i].level == level)
         return 1;
   return 0;
}

static void
d3d_fl_eval(const struct d3d_fl_caps *c, struct d3d_fl_result *r)
{
   char cap[112], need[400];
   memset(r, 0, sizeof(*r));

   /* ---- vkd3d-proton tiers (d3d12_device_determine_tiled_resources_tier etc.) ---- */
   int t1_ok[6] = { c->sparseBinding, c->sparseResidencyAliased, c->sparseResidencyBuffer, c->sparseResidencyImage2D,
                    c->residencyStandard2DBlockShape, c->sparseQueue };
   static const char *const t1_n[6] = { "sparseBinding", "sparseResidencyAliased", "sparseResidencyBuffer",
                                        "sparseResidencyImage2D", "residencyStandard2DBlockShape", "sparse-binding queue" };
   int t2_ok[5] = { c->shaderResourceResidency, c->shaderResourceMinLod, !c->residencyAlignedMipSize,
                    c->residencyNonResidentStrict, c->filterMinmaxSingleComponentFormats };
   static const char *const t2_n[5] = { "shaderResourceResidency", "shaderResourceMinLod", "!residencyAlignedMipSize",
                                        "residencyNonResidentStrict", "filterMinmaxSingleComponentFormats" };
   int t3_ok[2] = { c->sparseResidencyImage3D, c->residencyStandard3DBlockShape };
   static const char *const t3_n[2] = { "sparseResidencyImage3D", "residencyStandard3DBlockShape" };
   int all1 = 1, all2 = 1, all3 = 1;
   for (int i = 0; i < 6; i++) all1 &= t1_ok[i];
   for (int i = 0; i < 5; i++) all2 &= t2_ok[i];
   for (int i = 0; i < 2; i++) all3 &= t3_ok[i];
   r->tiled = !all1 ? 0 : !all2 ? 1 : !all3 ? 2 : 4; /* vkd3d-proton never reports tier 3 */
   r->cons = !c->conservativeExt ? 0 : !c->degenerateTrianglesRasterized ? 1
             : !c->fullyCoveredFragmentShaderInputVariable ? 2 : 3;
   r->rov = c->pixelInterlock && c->sampleInterlock;
   r->typed_uav = c->typedUavFormats == 18;
   r->binding = 3; /* vkd3d-proton: always tier 3 (device.c:9267) */
   r->vprt = c->shaderOutputViewportIndex && c->shaderOutputLayer;
   /* shortcut: VRS sample-count checks and DXR limit/vertex-format checks (device.c:965-1088, 9029-9067) are not
    * replicated; both features are absent on every Mali driver today, upgrade when one starts exposing them. */
   r->vrs = !c->pipelineFsr ? 0 : (c->attachmentFsr && c->primitiveFsr) ? 2 : 1;
   r->mesh = c->meshShader && c->taskShader;
   r->dxr = !(c->rayTracingPipeline && c->accelerationStructure) ? 0
            : (c->rayQuery && c->rayTraversalPrimitiveCulling) ? 11 : 10;
   r->sfb = (c->shaderInt64 && c->shaderImageInt64Atomics) ? 9 : 0;

   /* ---- shader model (device.c:9736-9940) ---- */
   int sm60 = c->waveOps && c->scalarOrStdLayout && c->shaderInt16;
   int sm65 = c->denormIndependence && c->fp32Ftz && c->fp32Preserve;
   int sm66 = c->computeDerivativeGroupLinear && c->shaderBufferInt64Atomics && c->shaderInt8 &&
              c->fixedOrComputeSubgroupSize;
   int sm68 = c->maximalReconvergence && c->quadControl;
   r->sm = !sm60 ? 51 : !sm65 ? 60 : !sm66 ? 65 : !sm68 ? 66 : 68;
   r->wave_ops = r->sm >= 60;

   /* ---- D3D12 levels ---- */
   if (c->proton_hard_missing) {
      snprintf(need, sizeof(need), "%d vkd3d-proton hard requirement%s missing (vkd3d compliance, 'proton:' items)",
               c->proton_hard_missing, c->proton_hard_missing == 1 ? "" : "s");
      d3d_fl_block(r, 12, D12_110, R_DEVICE, "vkd3d-proton device creation", need);
   }
   if (!c->logicOp)
      d3d_fl_block(r, 12, D12_111, R_OTHER, "OutputMergerLogicOp", "VkPhysicalDeviceFeatures.logicOp");
   if (!c->vertexPipelineStoresAndAtomics)
      d3d_fl_block(r, 12, D12_111, R_OTHER, "UAVs at every stage", "vertexPipelineStoresAndAtomics");
   if (c->maxPerStageStorageBuffers < 64) {
      snprintf(cap, sizeof(cap), "64 UAV slots (buffers, have %u)", c->maxPerStageStorageBuffers);
      d3d_fl_block(r, 12, D12_111, R_UAV64, cap, "maxPerStageDescriptorStorageBuffers >= 64");
   }
   if (c->maxPerStageStorageImages < 64) {
      snprintf(cap, sizeof(cap), "64 UAV slots (images, have %u)", c->maxPerStageStorageImages);
      d3d_fl_block(r, 12, D12_111, R_UAV64, cap, "maxPerStageDescriptorStorageImages >= 64");
   }
   if (r->tiled < 2) {
      snprintf(cap, sizeof(cap), "TiledResourcesTier >= 2 (have %d)", r->tiled);
      snprintf(need, sizeof(need), "sparse residency + residency queries");
      d3d_fl_missing(need, sizeof(need), t1_ok, t1_n, 6);
      d3d_fl_missing(need, sizeof(need), t2_ok, t2_n, 5);
      d3d_fl_block(r, 12, D12_120, R_TILED, cap, need);
   }
   if (!r->typed_uav) {
      snprintf(cap, sizeof(cap), "TypedUAVLoadAdditionalFormats (have %d/18)", c->typedUavFormats);
      d3d_fl_block(r, 12, D12_120, R_TYPED_UAV, cap, "STORAGE_READ_WITHOUT_FORMAT on the 18 typed-UAV formats");
   }
   if (!r->rov) {
      static const char *const n[2] = { "fragmentShaderPixelInterlock", "fragmentShaderSampleInterlock" };
      int ok[2] = { c->pixelInterlock, c->sampleInterlock };
      snprintf(need, sizeof(need), "VK_EXT_fragment_shader_interlock pixel + sample");
      d3d_fl_missing(need, sizeof(need), ok, n, 2);
      d3d_fl_block(r, 12, D12_121, R_ROV, "ROVsSupported", need);
   }
   if (r->cons < 1)
      d3d_fl_block(r, 12, D12_121, R_CONS, "ConservativeRasterizationTier >= 1", "VK_EXT_conservative_rasterization");
   if (r->sm < 65) {
      snprintf(cap, sizeof(cap), "HighestShaderModel >= 6.5 (have %d.%d)", r->sm / 10, r->sm % 10);
      d3d_fl_block(r, 12, D12_122, R_SM65, cap,
                   r->sm < 60 ? "subgroup basic/vote/arith/ballot/shuffle/quad in CS+FS, scalar/std UBO layout, shaderInt16"
                              : "denormBehaviorIndependence != NONE + shaderDenormFlushToZeroFloat32 + shaderDenormPreserveFloat32");
   }
   if (!r->vprt)
      d3d_fl_block(r, 12, D12_122, R_VPRT, "VPAndRTArrayIndexFromAnyShaderFeedingRasterizer",
                   "shaderOutputViewportIndex + shaderOutputLayer");
   if (!r->wave_ops)
      d3d_fl_block(r, 12, D12_122, R_SM65, "WaveOps", "SM >= 6.0 (see HighestShaderModel)");
   if (!c->shaderInt64)
      d3d_fl_block(r, 12, D12_122, R_OTHER, "Int64ShaderOps", "shaderInt64");
   if (!c->depthBounds)
      d3d_fl_block(r, 12, D12_122, R_OTHER, "DepthBoundsTestSupported", "depthBounds");
   if (!c->transferTimestamps)
      d3d_fl_block(r, 12, D12_122, R_OTHER, "CopyQueueTimestampQueriesSupported", "timestampValidBits on the transfer queue family");
   if (r->cons < 3) {
      static const char *const n[3] = { "VK_EXT_conservative_rasterization", "degenerateTrianglesRasterized",
                                        "fullyCoveredFragmentShaderInputVariable" };
      int ok[3] = { c->conservativeExt, c->degenerateTrianglesRasterized, c->fullyCoveredFragmentShaderInputVariable };
      snprintf(cap, sizeof(cap), "ConservativeRasterizationTier 3 (have %d)", r->cons);
      need[0] = '\0';
      d3d_fl_missing(need, sizeof(need), ok, n, 3);
      d3d_fl_block(r, 12, D12_122, R_CONS3, cap, need + (need[0] == ' '));
   }
   if (r->tiled < 3) {
      snprintf(cap, sizeof(cap), "TiledResourcesTier >= 3 (have %d)", r->tiled);
      snprintf(need, sizeof(need), "tier 2 + 3D sparse");
      d3d_fl_missing(need, sizeof(need), t3_ok, t3_n, 2);
      d3d_fl_block(r, 12, D12_122, R_TILED, cap, need);
   }
   if (r->dxr < 11)
      d3d_fl_block(r, 12, D12_122, R_DXR, "RaytracingTier >= 1.1",
                   "VK_KHR_acceleration_structure + ray_tracing_pipeline + ray_query + rayTraversalPrimitiveCulling");
   if (r->vrs < 2)
      d3d_fl_block(r, 12, D12_122, R_VRS, "VariableShadingRateTier 2",
                   "VK_KHR_fragment_shading_rate pipeline + primitive + attachment rates");
   if (!r->mesh)
      d3d_fl_block(r, 12, D12_122, R_MESH, "MeshShaderTier 1", "VK_EXT_mesh_shader meshShader + taskShader");
   if (!r->sfb)
      d3d_fl_block(r, 12, D12_122, R_SFB, "SamplerFeedbackTier 0.9",
                   "shaderInt64 + VK_EXT_shader_image_atomic_int64 shaderImageInt64Atomics");
   for (int i = 0; i < D12_N; i++) {
      r->d12[i] = !d3d_fl_blocked(r, 12, i) && (i == 0 || r->d12[i - 1]);
      if (r->d12[i])
         r->d12_max = i + 1;
   }

   /* ---- D3D11 levels (DXVK 3.1.1): 9_1-10_1 need only DXVK's hard list, which includes geometryShader ---- */
   if (!c->dxvk_base)
      d3d_fl_block(r, 11, D11_9X, R_DEVICE, "DXVK device creation", "geometryShader (and the rest of the DXVK hard list)");
   {
      static const char *const n[4] = { "drawIndirectFirstInstance", "fragmentStoresAndAtomics", "multiDrawIndirect",
                                        "tessellationShader" };
      int ok[4] = { c->drawIndirectFirstInstance, c->fragmentStoresAndAtomics, c->multiDrawIndirect, c->tessellationShader };
      if (!(ok[0] && ok[1] && ok[2] && ok[3])) {
         need[0] = '\0';
         d3d_fl_missing(need, sizeof(need), ok, n, 4);
         d3d_fl_block(r, 11, D11_110, R_OTHER, "FL11_0 caps", need + (need[0] == ' '));
      }
   }
   if (!c->logicOp)
      d3d_fl_block(r, 11, D11_111, R_OTHER, "OutputMergerLogicOp", "logicOp");
   if (!c->vertexPipelineStoresAndAtomics)
      d3d_fl_block(r, 11, D11_111, R_OTHER, "UAVs at every stage", "vertexPipelineStoresAndAtomics");
   /* DXVK DetermineTiledResourcesTier: no sparse-queue check, needs samplerFilterMinmax. */
   {
      int ok[10] = { c->sparseBinding, c->sparseResidencyBuffer, c->sparseResidencyImage2D, c->sparseResidencyAliased,
                     c->residencyStandard2DBlockShape, c->shaderResourceResidency, c->shaderResourceMinLod,
                     c->samplerFilterMinmax && c->filterMinmaxSingleComponentFormats, c->residencyNonResidentStrict,
                     !c->residencyAlignedMipSize };
      static const char *const n[10] = { "sparseBinding", "sparseResidencyBuffer", "sparseResidencyImage2D",
                                         "sparseResidencyAliased", "residencyStandard2DBlockShape", "shaderResourceResidency",
                                         "shaderResourceMinLod", "samplerFilterMinmax+filterMinmaxSingleComponentFormats",
                                         "residencyNonResidentStrict", "!residencyAlignedMipSize" };
      int base = ok[0] && ok[1] && ok[2] && ok[3] && ok[4];
      int t2 = base && ok[5] && ok[6] && ok[7] && ok[8] && ok[9];
      r->d11_tiled = !base ? 0 : !t2 ? 1 : (c->sparseResidencyImage3D && c->residencyStandard3DBlockShape) ? 3 : 2;
      if (r->d11_tiled < 2) {
         snprintf(cap, sizeof(cap), "TiledResourcesTier >= 2 (have %d)", r->d11_tiled);
         snprintf(need, sizeof(need), "sparse residency");
         d3d_fl_missing(need, sizeof(need), ok, n, 10);
         d3d_fl_block(r, 11, D11_120, R_TILED, cap, need);
      }
   }
   if (!r->typed_uav) {
      snprintf(cap, sizeof(cap), "TypedUAVLoadAdditionalFormats (have %d/18)", c->typedUavFormats);
      d3d_fl_block(r, 11, D11_120, R_TYPED_UAV, cap, "STORAGE_READ_WITHOUT_FORMAT on the 18 typed-UAV formats");
   }
   if (!c->conservativeExt)
      d3d_fl_block(r, 11, D11_121, R_CONS, "ConservativeRasterizationTier >= 1", "VK_EXT_conservative_rasterization");
   if (!c->pixelInterlock)
      d3d_fl_block(r, 11, D11_121, R_ROV, "ROVsSupported", "VK_EXT_fragment_shader_interlock fragmentShaderPixelInterlock");
   for (int i = 0; i < D11_N; i++) {
      r->d11[i] = !d3d_fl_blocked(r, 11, i) && (i == 0 || r->d11[i - 1]);
      if (r->d11[i])
         r->d11_max = i + 1;
   }
}

/* Mali architecture: PanVK sets deviceID = GPU_ID (arch major in bits 31:28, pan_model.h PAN_ARCH_MAJOR);
 * otherwise from the marketing name. 0 = unknown. */
static int
d3d_fl_arch(unsigned vendor, unsigned device_id, const char *name)
{
   static const struct { const char *n; int arch; } names[] = {
      { "G71", 6 }, { "G72", 6 }, { "G31", 7 }, { "G51", 7 }, { "G52", 7 }, { "G76", 7 },
      { "G57", 9 }, { "G68", 9 }, { "G77", 9 }, { "G78", 9 }, { "G310", 10 }, { "G510", 10 }, { "G610", 10 },
      { "G710", 10 }, { "G615", 11 }, { "G715", 11 }, { "G520", 12 }, { "G620", 12 }, { "G720", 12 },
      { "G625", 13 }, { "G725", 13 }, { "G925", 13 }, { "G1-", 14 }, { "G2-", 15 },
   };
   unsigned a = (device_id >> 28) & 0xf;
   if (vendor == 0x13b5 && a >= 6)
      return (int)a;
   const char *p = name ? strstr(name, "-G") : NULL; /* "Mali-G715 MC7", "Immortalis-G925", "Mali-G1-Ultra" */
   if (!p)
      return 0;
   p++;
   for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
      size_t l = strlen(names[i].n);
      if (!strncmp(p, names[i].n, l) && (names[i].n[l - 1] == '-' || p[l] == ' ' || p[l] == '\0'))
         return names[i].arch;
   }
   return 0;
}

/* Reachability of one blocking cap on Mali arch vN: "<verdict>: <why>". Verdicts: possible with driver work,
 * needs kernel, hardware cannot, unknown. The README table has the same rows with sources. */
static const char *
d3d_fl_reach(int arch, int what)
{
   switch (what) {
   case R_DEVICE:
      return "possible with driver work: see the hard items of the vkd3d/DXVK compliance sections";
   case R_UAV64:
      return arch && arch <= 7 ? "possible with driver work: Bifrost PanVK caps storage images at 32 per stage "
                                 "(panvk_shader.h MAX_PER_STAGE_STORAGE_IMAGES; a table-copy cost choice, not a HW limit)"
                               : "possible with driver work: raise the per-stage UAV limit";
   case R_TILED:
      return "needs kernel: the kbase uAPI has no map-pages-at-a-chosen-GPU-VA (bind/unbind) ioctl "
             "(docs/KBASE-SPARSE-FEASIBILITY.md, NO-GO); then driver work: sparseResidencyAliased (panfrost/mesa#237), "
             "residencyNonResidentStrict, shaderResourceResidency/MinLod (upstream PanVK on panthor is tier 0 too)";
   case R_TYPED_UAV:
      return "possible with driver work: STORAGE_READ_WITHOUT_FORMAT is a format-table entry";
   case R_ROV:
      return "possible with driver work: no Mali driver exposes fragment shader interlock, but the per-pixel ordering "
             "primitive exists (eldest-colour dependency slot 7: bifrost.h BIFROST_SLOT_ELDEST_COLOUR, Valhall flow "
             "'wait' = slots 0,1,2,6,7); unproven for fragments that write no colour";
   case R_CONS:
      return arch >= 11 ? "possible with driver work: the HW has Conservative Rast Mode (genxml v11+)"
                        : "hardware cannot: no Conservative Rast Mode before v11 (first in genxml v11.xml); only "
                          "emulation by triangle dilation (gpu_prerast), not planned";
   case R_CONS3:
      return arch >= 11 ? "unknown: degenerateTrianglesRasterized only on v14, no inner-coverage input on any arch"
                        : "hardware cannot: no conservative rasterization before v11";
   case R_SM65:
      return "possible with driver work: float-controls denorm reporting (PanVK: 32_BIT_ONLY on v9+, ALL on v6/v7)";
   case R_VPRT:
      return "possible with driver work: shaderOutputViewportIndex/Layer from the VS (v10+ have both in build 115)";
   case R_DXR:
      return arch >= 11 ? "hardware only on Immortalis parts (RT unit: Immortalis-G715/G720/G925, Mali-G1-Ultra); "
                          "VK_KHR_ray_tracing_pipeline + ray_query do not exist in PanVK (very large driver work)"
                        : "hardware cannot: no ray tracing unit before Immortalis-G715 (v11)";
   case R_VRS:
      return arch >= 11 ? "possible with driver work: VRS hardware since Mali-G715/G615 (v11); PanVK has no VK_KHR_fragment_shading_rate"
                        : "hardware cannot: no variable rate shading before v11";
   case R_MESH:
      return "hardware cannot natively (no mesh/task stage known on v6-v14); compute emulation = very large driver work";
   case R_SFB:
      return "possible with driver work: shaderImageInt64Atomics";
   default:
      return "possible with driver work";
   }
}
#endif
