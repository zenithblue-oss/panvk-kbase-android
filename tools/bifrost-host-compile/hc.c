/* Host compile harness: create the FTL G72 event 0x51 compute pipelines
 * (shader_arith subgroup_size_control, sampler gather_ext, vkd3d_heap
 * image_heap, plus the passing texel_heap) on a host panvk under the
 * panfrost drm-shim, and dump the Bifrost disassembly.
 *
 * Host build (aarch64 Alpine, PATH must hold build/host-tools/bin for mesa_clc):
 *   meson setup BDIR MESA -Dbuildtype=debugoptimized -Dplatforms= -Dgallium-drivers=
 *     -Dvulkan-drivers=panfrost -Dpanfrost-kmds=panfrost,kbase -Dtools=drm-shim
 *     -Dmesa-clc=system -Dprecomp-compiler=system -Dllvm=disabled -Dzstd=disabled ...
 *   ninja -C BDIR src/panfrost/vulkan/libvulkan_panfrost.so src/panfrost/drm-shim/libpanfrost_noop_drm_shim.so
 * Build this file:
 *   gcc -o hc hc.c -IMESA/include -Itests/dxvk/vulkan/sampler -Itests/dxvk/vulkan/shader-arith -Itests/vkd3d/heap -ldl
 * Run (cases: all sgsc gather heap theap; sgsc needs subgroup size 4, so v6 only):
 *   LD_PRELOAD=BDIR/src/panfrost/drm-shim/libpanfrost_noop_drm_shim.so PAN_GPU_ID=6221
 *   PAN_I_WANT_A_BROKEN_VULKAN_DRIVER=1 BIFROST_MESA_DEBUG=shaders MESA_SHADER_CACHE_DISABLE=1
 *   ./hc BDIR/src/panfrost/vulkan/libvulkan_panfrost.so all > out.txt 2>&1
 *   python3 ops.py out.txt     # unique opcodes per shader
 */
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

#include "sampler_spv.h"
#include "shader_arith_spv.h"
#include "vkd3d_heap_spv.h"

#define CHK(x) do { VkResult r_ = (x); if (r_) { fprintf(stderr, "FAIL %s = %d\n", #x, r_); exit(1); } } while (0)

static PFN_vkGetInstanceProcAddr gipa;
static VkInstance inst;
static VkDevice dev;
#define IF(n) PFN_vk##n p##n = (PFN_vk##n)gipa(inst, "vk" #n)

static VkPipelineLayout
make_layout(int nsets, VkDescriptorSetLayoutCreateInfo *infos)
{
   IF(CreateDescriptorSetLayout); IF(CreatePipelineLayout);
   VkDescriptorSetLayout sl[4];
   for (int i = 0; i < nsets; i++)
      CHK(pCreateDescriptorSetLayout(dev, &infos[i], NULL, &sl[i]));
   VkPushConstantRange pc = {VK_SHADER_STAGE_COMPUTE_BIT, 0, 16};
   VkPipelineLayoutCreateInfo pli = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = nsets, .pSetLayouts = sl,
      .pushConstantRangeCount = 1, .pPushConstantRanges = &pc,
   };
   VkPipelineLayout pl;
   CHK(pCreatePipelineLayout(dev, &pli, NULL, &pl));
   return pl;
}

static void
make_pipe(const char *name, const uint32_t *spv, size_t sz, VkPipelineLayout pl,
          const void *stage_pnext, VkPipelineShaderStageCreateFlags sflags,
          const VkSpecializationInfo *spec)
{
   IF(CreateShaderModule); IF(CreateComputePipelines);
   VkShaderModuleCreateInfo smi = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, NULL, 0, sz, spv};
   VkShaderModule sm;
   CHK(pCreateShaderModule(dev, &smi, NULL, &sm));
   VkComputePipelineCreateInfo ci = {
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      .stage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, stage_pnext, sflags,
                VK_SHADER_STAGE_COMPUTE_BIT, sm, "main", spec},
      .layout = pl,
   };
   VkPipeline p;
   fprintf(stderr, "=== HC BEGIN %s\n", name);
   fflush(stdout);
   CHK(pCreateComputePipelines(dev, VK_NULL_HANDLE, 1, &ci, NULL, &p));
   fflush(stdout);
   fprintf(stderr, "=== HC END %s\n", name);
}

int
main(int argc, char **argv)
{
   const char *which = argc > 2 ? argv[2] : "all";
   void *h = dlopen(argv[1], RTLD_NOW);
   if (!h) { fprintf(stderr, "%s\n", dlerror()); return 1; }
   gipa = (PFN_vkGetInstanceProcAddr)dlsym(h, "vk_icdGetInstanceProcAddr");
   PFN_vkCreateInstance pCreateInstance = (PFN_vkCreateInstance)gipa(NULL, "vkCreateInstance");
   VkApplicationInfo ai = {VK_STRUCTURE_TYPE_APPLICATION_INFO, .apiVersion = VK_API_VERSION_1_3};
   VkInstanceCreateInfo ici = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, .pApplicationInfo = &ai};
   CHK(pCreateInstance(&ici, NULL, &inst));
   IF(EnumeratePhysicalDevices); IF(GetPhysicalDeviceFeatures2); IF(CreateDevice);
   IF(GetPhysicalDeviceProperties); IF(EnumerateDeviceExtensionProperties);
   uint32_t n = 1;
   VkPhysicalDevice pd;
   CHK(pEnumeratePhysicalDevices(inst, &n, &pd));
   VkPhysicalDeviceProperties props;
   pGetPhysicalDeviceProperties(pd, &props);
   fprintf(stderr, "device %s api %x\n", props.deviceName, props.apiVersion);

   VkPhysicalDeviceVulkan13Features f13 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
   VkPhysicalDeviceVulkan12Features f12 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES, &f13};
   VkPhysicalDeviceVulkan11Features f11 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES, &f12};
   VkPhysicalDeviceFeatures2 f2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, &f11};
   pGetPhysicalDeviceFeatures2(pd, &f2);
   fprintf(stderr, "sgsc=%d fullsg=%d nonuniform_sampled=%d rtarray=%d gatherExt=%d\n",
           f13.subgroupSizeControl, f13.computeFullSubgroups,
           f12.shaderSampledImageArrayNonUniformIndexing, f12.runtimeDescriptorArray,
           f2.features.shaderImageGatherExtended);

   /* Enable every supported device extension. */
   uint32_t ne = 0;
   pEnumerateDeviceExtensionProperties(pd, NULL, &ne, NULL);
   VkExtensionProperties *ep = calloc(ne, sizeof(*ep));
   pEnumerateDeviceExtensionProperties(pd, NULL, &ne, ep);
   const char **names = calloc(ne, sizeof(char *));
   uint32_t nn = 0;
   for (uint32_t i = 0; i < ne; i++) {
      if (strstr(ep[i].extensionName, "swapchain") || strstr(ep[i].extensionName, "external") ||
          strstr(ep[i].extensionName, "android") || strstr(ep[i].extensionName, "ANDROID"))
         continue;
      names[nn++] = ep[i].extensionName;
   }
   float prio = 1.0f;
   VkDeviceQueueCreateInfo qci = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, .queueCount = 1, .pQueuePriorities = &prio};
   VkDeviceCreateInfo dci = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, &f2, .queueCreateInfoCount = 1,
                             .pQueueCreateInfos = &qci, .enabledExtensionCount = nn,
                             .ppEnabledExtensionNames = names};
   VkResult r = pCreateDevice(pd, &dci, NULL, &dev);
   if (r) {
      fprintf(stderr, "CreateDevice with all exts failed %d, retry without\n", r);
      dci.enabledExtensionCount = 0;
      CHK(pCreateDevice(pd, &dci, NULL, &dev));
   }

   /* shader_arith subgroup_size_control: binding 0 SSBO, spec wg 128, required size 4, full subgroups */
   if (!strcmp(which, "all") || !strcmp(which, "sgsc")) {
      VkDescriptorSetLayoutBinding b = {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT};
      VkDescriptorSetLayoutCreateInfo li = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, .bindingCount = 1, .pBindings = &b};
      VkPipelineLayout pl = make_layout(1, &li);
      uint32_t wg = 128;
      VkSpecializationMapEntry me = {0, 0, 4};
      VkSpecializationInfo si = {1, &me, 4, &wg};
      VkPipelineShaderStageRequiredSubgroupSizeCreateInfo rs = {
         VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO, NULL, 4};
      make_pipe("subgroup_size_control", subgroup_size_control_spv, sizeof(subgroup_size_control_spv), pl,
                &rs, VK_PIPELINE_SHADER_STAGE_CREATE_REQUIRE_FULL_SUBGROUPS_BIT, &si);
   }

   /* sampler gather_ext: 0 combined sampler, 1/2 SSBO */
   if (!strcmp(which, "all") || !strcmp(which, "gather")) {
      VkDescriptorSetLayoutBinding b[3] = {
         {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
         {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
         {2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
      };
      VkDescriptorSetLayoutCreateInfo li = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, .bindingCount = 3, .pBindings = b};
      VkPipelineLayout pl = make_layout(1, &li);
      make_pipe("gather_ext", gather_ext_spv, sizeof(gather_ext_spv), pl, NULL, 0, NULL);
   }

   /* vkd3d image_heap: set0 variable sampled image heap, set1 two SSBOs */
   if (!strcmp(which, "all") || !strcmp(which, "heap")) {
      VkDescriptorBindingFlags fl = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
         VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT |
         VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT;
      VkDescriptorSetLayoutBindingFlagsCreateInfo bf = {
         VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO, NULL, 1, &fl};
      VkDescriptorSetLayoutBinding b0 = {0, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4096, VK_SHADER_STAGE_COMPUTE_BIT};
      VkDescriptorSetLayoutBinding b1[2] = {
         {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
         {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
      };
      VkDescriptorSetLayoutCreateInfo li[2] = {
         {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, &bf,
          VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT, 1, &b0},
         {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, .bindingCount = 2, .pBindings = b1},
      };
      VkPipelineLayout pl = make_layout(2, li);
      make_pipe("image_heap", image_heap_spv, sizeof(image_heap_spv), pl, NULL, 0, NULL);
   }
   /* vkd3d texel_heap (passes on G72): set0 uniform texel heap, set1 empty, set2 two SSBOs */
   if (!strcmp(which, "all") || !strcmp(which, "theap")) {
      VkDescriptorBindingFlags fl = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
         VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;
      VkDescriptorSetLayoutBindingFlagsCreateInfo bf = {
         VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO, NULL, 1, &fl};
      VkDescriptorSetLayoutBinding b0 = {0, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 4096, VK_SHADER_STAGE_COMPUTE_BIT};
      VkDescriptorSetLayoutBinding b2[2] = {
         {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
         {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
      };
      VkDescriptorSetLayoutCreateInfo li[3] = {
         {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, &bf,
          VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT, 1, &b0},
         {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO},
         {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, .bindingCount = 2, .pBindings = b2},
      };
      VkPipelineLayout pl = make_layout(3, li);
      make_pipe("texel_heap", uniform_texel_heap_spv, sizeof(uniform_texel_heap_spv), pl, NULL, 0, NULL);
   }
   fprintf(stderr, "HC DONE\n");
   return 0;
}
