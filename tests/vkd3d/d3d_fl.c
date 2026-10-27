/* D3D feature-level report (query only, no rendering): D3D12 11_0..12_2 as vkd3d-proton 2230755 decides them and
 * D3D11 9_1..12_1 as DXVK 3.1.1 decides them, from this Vulkan driver's features, properties and formats, with
 * every blocking cap per level, the Vulkan feature/extension it needs, and a reachability verdict for this Mali
 * arch. The decision logic is d3d_fl.h (host unit test: d3d_fl_test.c).
 *
 * Output (checker format, parsed by PanProbe parseComplianceLog):
 *   INFO <text>
 *   HARD ok|FAIL hard D3D12 FL11_0 / D3D11 FL11_0      (the floor: RESULT FAIL when either is missing)
 *   SOFT ok|missing D3D12 FL<x> -- <blocking caps>      (one per level above the floor)
 *   SOFT missing D3D12 FL<x> needs <cap> -- <Vulkan side> | v<arch>: <reachability>
 *   RESULT PASS|FAIL
 * Usage: d3d_fl <libvulkan_panfrost.so>
 */
#define _POSIX_C_SOURCE 200809L
#include <dlfcn.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include "d3d_fl.h"

typedef PFN_vkVoidFunction (*icd_gipa_fn)(VkInstance, const char *);

static VkExtensionProperties *exts;
static uint32_t ext_count;
static PFN_vkGetPhysicalDeviceFeatures2 p_GetPhysicalDeviceFeatures2;
static PFN_vkGetPhysicalDeviceProperties2 p_GetPhysicalDeviceProperties2;
static PFN_vkGetPhysicalDeviceFormatProperties2 p_GetPhysicalDeviceFormatProperties2;
static VkPhysicalDevice phys;

static uint32_t
ext_version(const char *name)
{
   for (uint32_t i = 0; i < ext_count; i++)
      if (!strcmp(exts[i].extensionName, name))
         return exts[i].specVersion ? exts[i].specVersion : 1;
   return 0;
}
#define has_ext(n) (ext_version(n) != 0)

/* Chains one extension struct only when the extension is advertised (as vkd3d-proton does). */
static void
query_ext_features(const char *ext, void *s)
{
   if (!has_ext(ext))
      return;
   VkPhysicalDeviceFeatures2 f = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = s };
   p_GetPhysicalDeviceFeatures2(phys, &f);
}

static void
query_ext_props(const char *ext, void *s)
{
   if (!has_ext(ext))
      return;
   VkPhysicalDeviceProperties2 p = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, .pNext = s };
   p_GetPhysicalDeviceProperties2(phys, &p);
}

static VkFormatFeatureFlags2
fmt_features(VkFormat format)
{
   VkFormatProperties3 fp3 = { .sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_3 };
   VkFormatProperties2 fp2 = { .sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2, .pNext = &fp3 };
   p_GetPhysicalDeviceFormatProperties2(phys, format, &fp2);
   return fp3.optimalTilingFeatures | fp3.linearTilingFeatures;
}

static int hard_missing = 0;

static void
item(int hard, int ok, const char *name, const char *note)
{
   if (hard)
      printf("%s %s\n", ok ? "HARD ok" : "FAIL hard", name);
   else if (ok)
      printf("SOFT ok %s\n", name);
   else
      printf("SOFT missing %s -- %s\n", name, note);
   if (hard && !ok)
      hard_missing++;
}

/* "12_0 FAIL: TiledResourcesTier >= 2 (have 0)" text for one level: its own blockers, or the lower level. */
static void
level_note(const struct d3d_fl_result *r, int api, int level, char *out, size_t n)
{
   out[0] = '\0';
   for (int i = 0; i < r->nblock; i++) {
      if (r->blk[i].api != api || r->blk[i].level != level)
         continue;
      size_t l = strlen(out);
      snprintf(out + l, n - l, "%s%s", l ? ", " : "", r->blk[i].cap);
   }
   if (!out[0] && level > 0)
      snprintf(out, n, "needs FL%s first", api == 12 ? d3d_fl_d12_names[level - 1] : d3d_fl_d11_names[level - 1]);
}

int
main(int argc, char **argv)
{
   if (argc < 2) {
      printf("usage: %s <libvulkan_panfrost.so>\n", argv[0]);
      return 2;
   }
   setvbuf(stdout, NULL, _IONBF, 0);

   void *h = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
   if (!h) {
      printf("FAIL dlopen %s\n", dlerror());
      return 1;
   }
   icd_gipa_fn gipa = (icd_gipa_fn)dlsym(h, "vk_icdGetInstanceProcAddr");
   if (!gipa)
      gipa = (icd_gipa_fn)dlsym(h, "vkGetInstanceProcAddr");
   if (!gipa) {
      printf("FAIL gipa\n");
      return 1;
   }
   PFN_vkCreateInstance CreateInstance = (PFN_vkCreateInstance)gipa(NULL, "vkCreateInstance");
   if (!CreateInstance) {
      printf("FAIL missing vkCreateInstance\n");
      return 1;
   }
   VkApplicationInfo app = { .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO, .apiVersion = VK_API_VERSION_1_3 };
   VkInstanceCreateInfo ici = { .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, .pApplicationInfo = &app };
   VkInstance inst = VK_NULL_HANDLE;
   VkResult res = CreateInstance(&ici, NULL, &inst);
   if (res != VK_SUCCESS) {
      printf("FAIL CreateInstance r=%d\n", (int)res);
      return 1;
   }
   PFN_vkEnumeratePhysicalDevices p_EnumeratePhysicalDevices =
      (PFN_vkEnumeratePhysicalDevices)gipa(inst, "vkEnumeratePhysicalDevices");
   PFN_vkGetPhysicalDeviceQueueFamilyProperties p_GetPhysicalDeviceQueueFamilyProperties =
      (PFN_vkGetPhysicalDeviceQueueFamilyProperties)gipa(inst, "vkGetPhysicalDeviceQueueFamilyProperties");
   PFN_vkEnumerateDeviceExtensionProperties p_EnumerateDeviceExtensionProperties =
      (PFN_vkEnumerateDeviceExtensionProperties)gipa(inst, "vkEnumerateDeviceExtensionProperties");
   PFN_vkDestroyInstance p_DestroyInstance = (PFN_vkDestroyInstance)gipa(inst, "vkDestroyInstance");
   p_GetPhysicalDeviceProperties2 = (PFN_vkGetPhysicalDeviceProperties2)gipa(inst, "vkGetPhysicalDeviceProperties2");
   p_GetPhysicalDeviceFeatures2 = (PFN_vkGetPhysicalDeviceFeatures2)gipa(inst, "vkGetPhysicalDeviceFeatures2");
   p_GetPhysicalDeviceFormatProperties2 =
      (PFN_vkGetPhysicalDeviceFormatProperties2)gipa(inst, "vkGetPhysicalDeviceFormatProperties2");
   if (!p_EnumeratePhysicalDevices || !p_GetPhysicalDeviceQueueFamilyProperties || !p_EnumerateDeviceExtensionProperties ||
       !p_DestroyInstance || !p_GetPhysicalDeviceProperties2 || !p_GetPhysicalDeviceFeatures2 ||
       !p_GetPhysicalDeviceFormatProperties2) {
      printf("FAIL missing Vulkan 1.1 query entry points\n");
      return 1;
   }

   uint32_t nd = 0;
   if (p_EnumeratePhysicalDevices(inst, &nd, NULL) != VK_SUCCESS || nd == 0) {
      printf("FAIL EnumeratePhysicalDevices count=0\n");
      return 1;
   }
   VkPhysicalDevice *devs = calloc(nd, sizeof(*devs));
   if (!devs)
      return 1;
   p_EnumeratePhysicalDevices(inst, &nd, devs);
   phys = devs[0];
   for (uint32_t i = 0; i < nd; i++) {
      VkPhysicalDeviceProperties2 p2 = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
      p_GetPhysicalDeviceProperties2(devs[i], &p2);
      if (strstr(p2.properties.deviceName, "Mali")) {
         phys = devs[i];
         break;
      }
   }
   free(devs);
   p_EnumerateDeviceExtensionProperties(phys, NULL, &ext_count, NULL);
   exts = calloc(ext_count ? ext_count : 1, sizeof(*exts));
   if (!exts)
      return 1;
   p_EnumerateDeviceExtensionProperties(phys, NULL, &ext_count, exts);

   /* ---- properties ---- */
   VkPhysicalDeviceVulkan13Properties p13 = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES };
   VkPhysicalDeviceVulkan12Properties p12 = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES, .pNext = &p13 };
   VkPhysicalDeviceVulkan11Properties p11 = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES, .pNext = &p12 };
   VkPhysicalDeviceProperties2 props2 = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, .pNext = &p11 };
   p_GetPhysicalDeviceProperties2(phys, &props2);
   const VkPhysicalDeviceProperties *p = &props2.properties;
   const VkPhysicalDeviceLimits *lim = &p->limits;
   VkPhysicalDeviceTransformFeedbackPropertiesEXT xfbp = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_PROPERTIES_EXT };
   VkPhysicalDeviceConservativeRasterizationPropertiesEXT crp = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONSERVATIVE_RASTERIZATION_PROPERTIES_EXT };
   query_ext_props("VK_EXT_transform_feedback", &xfbp);
   query_ext_props("VK_EXT_conservative_rasterization", &crp);

   /* ---- features ---- */
   VkPhysicalDeviceVulkan13Features f13 = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
   VkPhysicalDeviceVulkan12Features f12 = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES, .pNext = &f13 };
   VkPhysicalDeviceVulkan11Features f11 = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES, .pNext = &f12 };
   VkPhysicalDeviceFeatures2 feat2 = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &f11 };
   p_GetPhysicalDeviceFeatures2(phys, &feat2);
   const VkPhysicalDeviceFeatures *f = &feat2.features;
   VkPhysicalDeviceRobustness2FeaturesEXT rob2 = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT };
   VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT fsi = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT };
   VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT vad = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_FEATURES_EXT };
   VkPhysicalDeviceDescriptorBufferFeaturesEXT dbf = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT };
   VkPhysicalDeviceMaintenance5FeaturesKHR m5 = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_5_FEATURES_KHR };
   VkPhysicalDeviceMaintenance6FeaturesKHR m6 = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_6_FEATURES_KHR };
   VkPhysicalDeviceComputeShaderDerivativesFeaturesKHR csd = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COMPUTE_SHADER_DERIVATIVES_FEATURES_KHR };
   VkPhysicalDeviceShaderMaximalReconvergenceFeaturesKHR mrc = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_MAXIMAL_RECONVERGENCE_FEATURES_KHR };
   VkPhysicalDeviceShaderQuadControlFeaturesKHR qc = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_QUAD_CONTROL_FEATURES_KHR };
   VkPhysicalDeviceMeshShaderFeaturesEXT mesh = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT };
   VkPhysicalDeviceFragmentShadingRateFeaturesKHR fsr = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR };
   VkPhysicalDeviceRayQueryFeaturesKHR rq = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR };
   VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtp = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR };
   VkPhysicalDeviceAccelerationStructureFeaturesKHR as = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR };
   VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT ia64 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_ATOMIC_INT64_FEATURES_EXT };
   query_ext_features("VK_EXT_robustness2", &rob2);
   query_ext_features("VK_EXT_fragment_shader_interlock", &fsi);
   query_ext_features("VK_EXT_vertex_attribute_divisor", &vad);
   query_ext_features("VK_EXT_descriptor_buffer", &dbf);
   query_ext_features("VK_KHR_maintenance5", &m5);
   query_ext_features("VK_KHR_maintenance6", &m6);
   query_ext_features("VK_KHR_compute_shader_derivatives", &csd);
   query_ext_features("VK_KHR_shader_maximal_reconvergence", &mrc);
   query_ext_features("VK_KHR_shader_quad_control", &qc);
   query_ext_features("VK_EXT_mesh_shader", &mesh);
   query_ext_features("VK_KHR_fragment_shading_rate", &fsr);
   query_ext_features("VK_KHR_ray_query", &rq);
   query_ext_features("VK_KHR_ray_tracing_pipeline", &rtp);
   query_ext_features("VK_KHR_acceleration_structure", &as);
   query_ext_features("VK_EXT_shader_image_atomic_int64", &ia64);

   uint32_t qn = 0;
   p_GetPhysicalDeviceQueueFamilyProperties(phys, &qn, NULL);
   VkQueueFamilyProperties *qf = calloc(qn ? qn : 1, sizeof(*qf));
   if (!qf)
      return 1;
   p_GetPhysicalDeviceQueueFamilyProperties(phys, &qn, qf);
   int gfx_compute = 0, sparse_queue = 0, transfer_ts = 0;
   for (uint32_t i = 0; i < qn; i++) {
      if ((qf[i].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) == (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
         gfx_compute = 1;
      if ((qf[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) && qf[i].queueCount)
         sparse_queue = 1;
      /* shortcut: vkd3d-proton's copy queue is a transfer-capable family (graphics/compute imply transfer); any one
       * with timestamps counts. Exact only for single-family drivers (PanVK); upgrade if PanVK adds transfer queues. */
      if ((qf[i].queueFlags & (VK_QUEUE_TRANSFER_BIT | VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) &&
          qf[i].queueCount && qf[i].timestampValidBits)
         transfer_ts = 1;
   }
   free(qf);

   /* ---- vkd3d-proton device creation (same checks as vkd3d_reqs "proton:" hard items, device.c:2495-2672) ---- */
   uint32_t ssbo_align = (uint32_t)lim->minStorageBufferOffsetAlignment;
   int db_path = dbf.descriptorBuffer && dbf.descriptorBufferPushDescriptors &&
                 f12.shaderUniformBufferArrayNonUniformIndexing && !(ssbo_align > 4 && ssbo_align <= 16);
   int limits_1m = p12.maxPerStageDescriptorUpdateAfterBindSampledImages >= 1000000 &&
                   p12.maxPerStageDescriptorUpdateAfterBindStorageImages >= 1000000 &&
                   p12.maxPerStageDescriptorUpdateAfterBindStorageBuffers >= 1000000;
   const int proton_ok[] = {
      p->apiVersion >= VK_API_VERSION_1_3, f11.shaderDrawParameters, f12.samplerMirrorClampToEdge,
      rob2.robustBufferAccess2, rob2.robustImageAccess2, rob2.nullDescriptor, has_ext("VK_KHR_push_descriptor"),
      m5.maintenance5, m6.maintenance6,
      vad.vertexAttributeInstanceRateDivisor && vad.vertexAttributeInstanceRateZeroDivisor &&
         ext_version("VK_EXT_vertex_attribute_divisor") >= 3,
      xfbp.transformFeedbackQueries,
      p13.storageTexelBufferOffsetSingleTexelAlignment || p13.storageTexelBufferOffsetAlignmentBytes == 1,
      p13.uniformTexelBufferOffsetSingleTexelAlignment || p13.uniformTexelBufferOffsetAlignmentBytes == 1,
      f12.descriptorIndexing, f12.shaderStorageTexelBufferArrayNonUniformIndexing,
      f12.shaderStorageImageArrayNonUniformIndexing, f12.descriptorBindingVariableDescriptorCount, db_path || limits_1m,
      gfx_compute,
   };
   struct d3d_fl_caps c;
   memset(&c, 0, sizeof(c));
   for (size_t i = 0; i < sizeof(proton_ok) / sizeof(proton_ok[0]); i++)
      c.proton_hard_missing += !proton_ok[i];

   /* ---- caps ---- */
   c.dxvk_base = f->geometryShader;
   c.drawIndirectFirstInstance = f->drawIndirectFirstInstance;
   c.fragmentStoresAndAtomics = f->fragmentStoresAndAtomics;
   c.multiDrawIndirect = f->multiDrawIndirect;
   c.tessellationShader = f->tessellationShader;
   c.logicOp = f->logicOp;
   c.vertexPipelineStoresAndAtomics = f->vertexPipelineStoresAndAtomics;
   c.maxPerStageStorageBuffers = lim->maxPerStageDescriptorStorageBuffers;
   c.maxPerStageStorageImages = lim->maxPerStageDescriptorStorageImages;
   c.sparseBinding = f->sparseBinding;
   c.sparseResidencyBuffer = f->sparseResidencyBuffer;
   c.sparseResidencyImage2D = f->sparseResidencyImage2D;
   c.sparseResidencyImage3D = f->sparseResidencyImage3D;
   c.sparseResidencyAliased = f->sparseResidencyAliased;
   c.residencyStandard2DBlockShape = p->sparseProperties.residencyStandard2DBlockShape;
   c.residencyStandard3DBlockShape = p->sparseProperties.residencyStandard3DBlockShape;
   c.residencyAlignedMipSize = p->sparseProperties.residencyAlignedMipSize;
   c.residencyNonResidentStrict = p->sparseProperties.residencyNonResidentStrict;
   c.shaderResourceResidency = f->shaderResourceResidency;
   c.shaderResourceMinLod = f->shaderResourceMinLod;
   c.samplerFilterMinmax = f12.samplerFilterMinmax;
   c.filterMinmaxSingleComponentFormats = p12.filterMinmaxSingleComponentFormats;
   c.sparseQueue = sparse_queue;
   static const VkFormat uav[18] = {
      VK_FORMAT_R32_SFLOAT, VK_FORMAT_R32_UINT, VK_FORMAT_R32_SINT, VK_FORMAT_R32G32B32A32_SFLOAT,
      VK_FORMAT_R32G32B32A32_UINT, VK_FORMAT_R32G32B32A32_SINT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R16G16B16A16_UINT,
      VK_FORMAT_R16G16B16A16_SINT, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UINT, VK_FORMAT_R8G8B8A8_SINT,
      VK_FORMAT_R16_SFLOAT, VK_FORMAT_R16_UINT, VK_FORMAT_R16_SINT, VK_FORMAT_R8_UNORM, VK_FORMAT_R8_UINT, VK_FORMAT_R8_SINT,
   };
   for (int i = 0; i < 18; i++) {
      VkFormatFeatureFlags2 ff = fmt_features(uav[i]);
      c.typedUavFormats += (ff & VK_FORMAT_FEATURE_2_STORAGE_IMAGE_BIT) && (ff & VK_FORMAT_FEATURE_2_STORAGE_READ_WITHOUT_FORMAT_BIT);
   }
   c.conservativeExt = has_ext("VK_EXT_conservative_rasterization");
   c.degenerateTrianglesRasterized = crp.degenerateTrianglesRasterized;
   c.fullyCoveredFragmentShaderInputVariable = crp.fullyCoveredFragmentShaderInputVariable;
   c.pixelInterlock = fsi.fragmentShaderPixelInterlock;
   c.sampleInterlock = fsi.fragmentShaderSampleInterlock;
   const VkSubgroupFeatureFlags wave_bits = VK_SUBGROUP_FEATURE_BASIC_BIT | VK_SUBGROUP_FEATURE_VOTE_BIT |
                                            VK_SUBGROUP_FEATURE_ARITHMETIC_BIT | VK_SUBGROUP_FEATURE_BALLOT_BIT |
                                            VK_SUBGROUP_FEATURE_SHUFFLE_BIT | VK_SUBGROUP_FEATURE_QUAD_BIT;
   const VkShaderStageFlags cs_fs = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
   c.waveOps = p11.subgroupSize >= 4 && (p11.subgroupSupportedOperations & wave_bits) == wave_bits &&
               (p11.subgroupSupportedStages & cs_fs) == cs_fs;
   c.scalarOrStdLayout = f12.scalarBlockLayout || f12.uniformBufferStandardLayout;
   c.shaderInt16 = f->shaderInt16;
   c.denormIndependence = p12.denormBehaviorIndependence != VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_NONE;
   c.fp32Ftz = p12.shaderDenormFlushToZeroFloat32;
   c.fp32Preserve = p12.shaderDenormPreserveFloat32;
   c.computeDerivativeGroupLinear = csd.computeDerivativeGroupLinear;
   c.shaderBufferInt64Atomics = f12.shaderBufferInt64Atomics;
   c.shaderInt8 = f12.shaderInt8;
   c.fixedOrComputeSubgroupSize = p13.minSubgroupSize == p13.maxSubgroupSize ||
                                  (p13.requiredSubgroupSizeStages & VK_SHADER_STAGE_COMPUTE_BIT);
   c.maximalReconvergence = mrc.shaderMaximalReconvergence;
   c.quadControl = qc.shaderQuadControl;
   c.shaderOutputViewportIndex = f12.shaderOutputViewportIndex;
   c.shaderOutputLayer = f12.shaderOutputLayer;
   c.shaderInt64 = f->shaderInt64;
   c.depthBounds = f->depthBounds;
   c.transferTimestamps = transfer_ts;
   c.rayTracingPipeline = rtp.rayTracingPipeline;
   c.rayTraversalPrimitiveCulling = rtp.rayTraversalPrimitiveCulling;
   c.accelerationStructure = as.accelerationStructure;
   c.rayQuery = rq.rayQuery;
   c.pipelineFsr = fsr.pipelineFragmentShadingRate;
   c.attachmentFsr = fsr.attachmentFragmentShadingRate;
   c.primitiveFsr = fsr.primitiveFragmentShadingRate;
   c.meshShader = mesh.meshShader;
   c.taskShader = mesh.taskShader;
   c.shaderImageInt64Atomics = ia64.shaderImageInt64Atomics;

   struct d3d_fl_result r;
   d3d_fl_eval(&c, &r);
   int arch = d3d_fl_arch(p->vendorID, p->deviceID, p->deviceName);
   char av[8];
   snprintf(av, sizeof(av), arch ? "v%d" : "v?", arch);

   /* ---- report ---- */
   char line[1024], note[768];
   printf("INFO device %s, Mali %s, apiVersion %u.%u.%u\n", p->deviceName, av, VK_API_VERSION_MAJOR(p->apiVersion),
          VK_API_VERSION_MINOR(p->apiVersion), VK_API_VERSION_PATCH(p->apiVersion));
   line[0] = '\0';
   for (int i = 0; i < D12_N; i++)
      snprintf(line + strlen(line), sizeof(line) - strlen(line), "%s%s %s", i ? ", " : "", d3d_fl_d12_names[i],
               r.d12[i] ? "PASS" : "FAIL");
   printf("INFO D3D12 (vkd3d-proton 2230755): %s; max %s\n", line, r.d12_max ? d3d_fl_d12_names[r.d12_max - 1] : "none");
   printf("INFO D3D12 caps: shader model %d.%d, resource binding tier %d, tiled resources tier %d, conservative raster tier %d, "
          "ROVs %s, typed UAV loads %d/18, wave ops %s, VP/RT index without GS %s, VRS tier %d, mesh tier %d, DXR %s, "
          "sampler feedback %s\n",
          r.sm / 10, r.sm % 10, r.binding, r.tiled, r.cons, r.rov ? "yes" : "no", c.typedUavFormats, r.wave_ops ? "yes" : "no",
          r.vprt ? "yes" : "no", r.vrs, r.mesh, r.dxr == 11 ? "1.1" : r.dxr == 10 ? "1.0" : "none", r.sfb ? "0.9" : "none");
   line[0] = '\0';
   for (int i = 0; i < D11_N; i++)
      snprintf(line + strlen(line), sizeof(line) - strlen(line), "%s%s %s", i ? ", " : "", d3d_fl_d11_names[i],
               r.d11[i] ? "PASS" : "FAIL");
   printf("INFO D3D11 (DXVK 3.1.1): %s; max %s (tiled tier %d, conservative tier %d, ROVs %s)\n", line,
          r.d11_max ? d3d_fl_d11_names[r.d11_max - 1] : "none", r.d11_tiled, r.cons, c.pixelInterlock ? "yes" : "no");
   line[0] = '\0';
   for (int i = D12_110; i <= D12_121; i++)
      if (!r.d12[i])
         snprintf(line + strlen(line), sizeof(line) - strlen(line), "%s%s", line[0] ? ", " : "", d3d_fl_d12_names[i]);
   printf("INFO target D3D12 11_0-12_1 on %s: %s%s; 12_2 out of scope\n", av, line[0] ? "not met, missing " : "met",
          line);
   /* One reachability line per distinct reason that blocks a level up to 12_1. */
   int seen[R_OTHER + 1] = { 0 };
   for (int i = 0; i < r.nblock; i++) {
      const struct d3d_fl_block *b = &r.blk[i];
      if (b->api != 12 || b->level > D12_121 || seen[b->reach])
         continue;
      seen[b->reach] = 1;
      printf("INFO reach FL%s %s on %s: %s\n", d3d_fl_d12_names[b->level], b->cap, av, d3d_fl_reach(arch, b->reach));
   }

   for (int i = 0; i < D12_N; i++) {
      snprintf(line, sizeof(line), "D3D12 FL%s", d3d_fl_d12_names[i]);
      level_note(&r, 12, i, note, sizeof(note));
      item(i == D12_110, r.d12[i], line, note);
   }
   for (int i = 0; i < D11_N; i++) {
      snprintf(line, sizeof(line), "D3D11 FL%s", d3d_fl_d11_names[i]);
      level_note(&r, 11, i, note, sizeof(note));
      item(i == D11_110, r.d11[i], line, note);
   }
   for (int i = 0; i < r.nblock; i++) {
      const struct d3d_fl_block *b = &r.blk[i];
      snprintf(line, sizeof(line), "D3D%d FL%s needs %s", b->api,
               b->api == 12 ? d3d_fl_d12_names[b->level] : d3d_fl_d11_names[b->level], b->cap);
      snprintf(note, sizeof(note), "%s | %s: %s", b->need, av, d3d_fl_reach(arch, b->reach));
      item(0, 0, line, note);
   }
   printf("INFO hard %s, %d blocking caps listed\n", hard_missing ? "FAIL" : "ok", r.nblock);
   p_DestroyInstance(inst, NULL);
   free(exts);
   printf("RESULT %s\n", hard_missing ? "FAIL" : "PASS");
   return hard_missing ? 1 : 0;
}
