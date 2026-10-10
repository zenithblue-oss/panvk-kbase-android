/* Standalone Vulkan robustImageAccess2 test program.
 * Tests VK_EXT_robustness2 robustImageAccess2 semantics on PanVK.
 *
 * Usage: robust_image2 <path-to-libvulkan_panfrost.so>
 */
#include <dlfcn.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if __has_include("../dx7_harness.h")
#include "../dx7_harness.h"
#else
#include "dx7_harness.h"
#endif

#include "robust_image2_spv.h"

#define EXTRA_FUNCS(X) \
   X(CreateComputePipelines) \
   X(CmdDispatch) \
   X(CreateDescriptorSetLayout) \
   X(DestroyDescriptorSetLayout) \
   X(DestroyPipelineLayout) \
   X(CreateDescriptorPool) \
   X(DestroyDescriptorPool) \
   X(AllocateDescriptorSets) \
   X(UpdateDescriptorSets) \
   X(CmdBindDescriptorSets) \
   X(CreateSampler) \
   X(DestroySampler) \
   X(CreateBufferView) \
   X(DestroyBufferView) \
   X(CmdCopyBufferToImage) \
   X(DestroyImage) \
   X(DestroyImageView) \
   X(DestroyBuffer) \
   X(FreeMemory) \
   X(QueueWaitIdle)

EXTRA_FUNCS(DX7_DECL)

static PFN_vkGetInstanceProcAddr gipa;
static VkPhysicalDeviceRobustness2FeaturesEXT supported_rob2;
static VkPhysicalDeviceImageRobustnessFeatures supported_img_rob;
static const char *s_ext_name = NULL;
static int has_pipe_rob, has_exec_props;

static void
device_hook(struct dx7 *t, VkDeviceCreateInfo *dci)
{
   PFN_vkEnumerateDeviceExtensionProperties p_EnumerateDeviceExtensionProperties =
      (PFN_vkEnumerateDeviceExtensionProperties)gipa(t->inst, "vkEnumerateDeviceExtensionProperties");
   PFN_vkGetPhysicalDeviceFeatures2 p_GetPhysicalDeviceFeatures2 =
      (PFN_vkGetPhysicalDeviceFeatures2)gipa(t->inst, "vkGetPhysicalDeviceFeatures2");
   if (!p_GetPhysicalDeviceFeatures2)
      p_GetPhysicalDeviceFeatures2 =
         (PFN_vkGetPhysicalDeviceFeatures2)gipa(t->inst, "vkGetPhysicalDeviceFeatures2KHR");

   if (!p_EnumerateDeviceExtensionProperties || !p_GetPhysicalDeviceFeatures2) {
      printf("FAIL missing instance entry points\n");
      printf("RESULT FAIL\n");
      exit(1);
   }

   uint32_t ext_count = 0;
   CK(p_EnumerateDeviceExtensionProperties(t->phys, NULL, &ext_count, NULL), "enum exts count");
   VkExtensionProperties *exts = calloc(ext_count, sizeof(*exts));
   if (ext_count > 0 && !exts) {
      printf("FAIL out of memory allocating exts\n");
      printf("RESULT FAIL\n");
      exit(1);
   }
   CK(p_EnumerateDeviceExtensionProperties(t->phys, NULL, &ext_count, exts), "enum exts");

   int has_ext = 0, has_khr = 0, has_img_rob = 0;
   for (uint32_t i = 0; i < ext_count; i++) {
      if (!strcmp(exts[i].extensionName, "VK_EXT_pipeline_robustness"))
         has_pipe_rob = 1;
      if (!strcmp(exts[i].extensionName, "VK_KHR_pipeline_executable_properties"))
         has_exec_props = 1;
      if (!strcmp(exts[i].extensionName, "VK_EXT_robustness2"))
         has_ext = 1;
      if (!strcmp(exts[i].extensionName, "VK_KHR_robustness2"))
         has_khr = 1;
      if (!strcmp(exts[i].extensionName, "VK_EXT_image_robustness"))
         has_img_rob = 1;
   }
   free(exts);

   if (has_ext) {
      s_ext_name = "VK_EXT_robustness2";
   } else if (has_khr) {
      s_ext_name = "VK_KHR_robustness2";
   } else {
      printf("FAIL robustness2 extension missing\n");
      printf("RESULT FAIL\n");
      exit(1);
   }

   supported_img_rob.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_ROBUSTNESS_FEATURES;
   supported_img_rob.pNext = NULL;

   supported_rob2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;
   supported_rob2.pNext = &supported_img_rob;

   VkPhysicalDeviceFeatures2 feat2 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
      .pNext = &supported_rob2,
   };
   p_GetPhysicalDeviceFeatures2(t->phys, &feat2);

   printf("INFO robustImageAccess2=%d robustBufferAccess2=%d nullDescriptor=%d\n",
          supported_rob2.robustImageAccess2,
          supported_rob2.robustBufferAccess2,
          supported_rob2.nullDescriptor);

   static VkPhysicalDeviceFeatures2 dev_feat2;
   static VkPhysicalDeviceRobustness2FeaturesEXT dev_rob2;
   static VkPhysicalDeviceImageRobustnessFeatures dev_img_rob;
   static VkPhysicalDevicePipelineRobustnessFeaturesEXT dev_pipe_rob;
   static VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR dev_exec_props;
   static const char *enabled_exts[4];
   uint32_t num_enabled_exts = 0;

   dev_img_rob.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_ROBUSTNESS_FEATURES;
   dev_img_rob.robustImageAccess = VK_TRUE;
   dev_img_rob.pNext = NULL;

   dev_rob2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;
   dev_rob2.robustBufferAccess2 = supported_rob2.robustBufferAccess2;
   dev_rob2.robustImageAccess2 = supported_rob2.robustImageAccess2;
   dev_rob2.nullDescriptor = supported_rob2.nullDescriptor;
   dev_rob2.pNext = &dev_img_rob;

   dev_feat2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
   dev_feat2.features.robustBufferAccess = VK_TRUE;
   dev_feat2.features.shaderStorageImageReadWithoutFormat =
      t->feats.shaderStorageImageReadWithoutFormat;
   dev_feat2.features.shaderStorageImageWriteWithoutFormat =
      t->feats.shaderStorageImageWriteWithoutFormat;
   dev_feat2.pNext = &dev_rob2;

   enabled_exts[num_enabled_exts++] = s_ext_name;
   if (has_img_rob)
      enabled_exts[num_enabled_exts++] = "VK_EXT_image_robustness";
   /* Benchmark only: per-pipeline image robustness and shader statistics. */
   if (has_pipe_rob) {
      dev_pipe_rob.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_ROBUSTNESS_FEATURES_EXT;
      dev_pipe_rob.pipelineRobustness = VK_TRUE;
      dev_pipe_rob.pNext = dev_img_rob.pNext;
      dev_img_rob.pNext = &dev_pipe_rob;
      enabled_exts[num_enabled_exts++] = "VK_EXT_pipeline_robustness";
   }
   if (has_exec_props) {
      dev_exec_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_EXECUTABLE_PROPERTIES_FEATURES_KHR;
      dev_exec_props.pipelineExecutableInfo = VK_TRUE;
      dev_exec_props.pNext = dev_img_rob.pNext;
      dev_img_rob.pNext = &dev_exec_props;
      enabled_exts[num_enabled_exts++] = "VK_KHR_pipeline_executable_properties";
   }

   dci->pNext = &dev_feat2;
   dci->pEnabledFeatures = NULL;
   dci->enabledExtensionCount = num_enabled_exts;
   dci->ppEnabledExtensionNames = enabled_exts;
}

static void
load_extra_funcs(struct dx7 *t, const char *icd)
{
   void *h = dlopen(icd, RTLD_NOW | RTLD_LOCAL);
   if (!h) {
      printf("FAIL dlopen %s\n", dlerror());
      exit(1);
   }
   PFN_vkGetInstanceProcAddr p_gipa =
      (PFN_vkGetInstanceProcAddr)dlsym(h, "vk_icdGetInstanceProcAddr");
   if (!p_gipa)
      p_gipa = (PFN_vkGetInstanceProcAddr)dlsym(h, "vkGetInstanceProcAddr");
   if (!p_gipa) {
      printf("FAIL missing gipa\n");
      exit(1);
   }

   PFN_vkGetDeviceProcAddr gdpa =
      (PFN_vkGetDeviceProcAddr)p_gipa(t->inst, "vkGetDeviceProcAddr");

#define LOAD_EXTRA(n) \
   if (gdpa) \
      vk##n = (PFN_vk##n)gdpa(t->dev, "vk" #n); \
   if (!vk##n) \
      vk##n = (PFN_vk##n)p_gipa(t->inst, "vk" #n); \
   if (!vk##n) { \
      printf("FAIL missing vk" #n "\n"); \
      exit(1); \
   }
   EXTRA_FUNCS(LOAD_EXTRA)
#undef LOAD_EXTRA
}

static VkPipeline
create_compute_pipe(struct dx7 *t, const uint32_t *spv, size_t spv_bytes, VkPipelineLayout layout)
{
   VkShaderModule sm = dx7_module(t, spv, spv_bytes);
   VkComputePipelineCreateInfo cpci = {
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      .stage = {
         .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
         .stage = VK_SHADER_STAGE_COMPUTE_BIT,
         .module = sm,
         .pName = "main",
      },
      .layout = layout,
   };
   VkPipeline pipe = VK_NULL_HANDLE;
   CK(vkCreateComputePipelines(t->dev, VK_NULL_HANDLE, 1, &cpci, NULL, &pipe), "CreateComputePipelines");
   vkDestroyShaderModule(t->dev, sm, NULL);
   return pipe;
}

static VkDeviceMemory
alloc_image_memory(struct dx7 *t, VkImage image)
{
   VkMemoryRequirements mr;
   vkGetImageMemoryRequirements(t->dev, image, &mr);
   uint32_t mi = 0;
   while (!(mr.memoryTypeBits & (1u << mi)))
      mi++;
   VkMemoryAllocateInfo mai = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = mr.size,
      .memoryTypeIndex = mi,
   };
   VkDeviceMemory mem = VK_NULL_HANDLE;
   CK(vkAllocateMemory(t->dev, &mai, NULL, &mem), "AllocImgMem");
   CK(vkBindImageMemory(t->dev, image, mem, 0), "BindImgMem");
   return mem;
}

static inline uint32_t
get_texel_r32(int x, int y, int layer, int level)
{
   return 0x1000u + (uint32_t)level * 0x100u + (uint32_t)layer * 0x10u + (uint32_t)y * 4u + (uint32_t)x;
}

static inline void
get_texel_rgba8(int x, int y, int layer, int level, uint8_t bytes[4])
{
   bytes[0] = (uint8_t)(x * 16 + 1);
   bytes[1] = (uint8_t)(y * 16 + 2);
   bytes[2] = (uint8_t)(layer * 16 + 3 + level * 64);
   bytes[3] = 200;
}

/* The app keeps only the last ~28 lines: remember the first 16 failures and
 * repeat them right before RESULT FAIL (and in a BENCH line for "extra"). */
static char fail_list[16][160];
static int fail_count;

static void
failf(const char *fmt, ...)
{
   char line[160];
   va_list ap;
   va_start(ap, fmt);
   vsnprintf(line, sizeof(line), fmt, ap);
   va_end(ap);
   printf("FAIL case %s\n", line);
   if (fail_count < 16)
      snprintf(fail_list[fail_count], sizeof(fail_list[0]), "%s", line);
   fail_count++;
}

static void
check_uvec4(const char *variant, const char *name,
            const uint32_t got[4], const uint32_t want[4],
            int is_float, int *passes, int *fails)
{
   bool match = true;
   if (is_float) {
      float got_f[4], want_f[4];
      memcpy(got_f, got, sizeof(got_f));
      memcpy(want_f, want, sizeof(want_f));
      for (int i = 0; i < 4; i++) {
         if (fabsf(got_f[i] - want_f[i]) > (1.0f / 510.0f))
            match = false;
      }
   } else {
      for (int i = 0; i < 4; i++) {
         if (got[i] != want[i])
            match = false;
      }
   }

   if (match) {
      printf("PASS case %s.%s\n", variant, name);
      (*passes)++;
   } else {
      failf("%s.%s got=(0x%08x,0x%08x,0x%08x,0x%08x) want=(0x%08x,0x%08x,0x%08x,0x%08x)",
             variant, name, got[0], got[1], got[2], got[3], want[0], want[1], want[2], want[3]);
      (*fails)++;
   }
}

static void
run_variant(struct dx7 *t, const char *variant_name, VkFormat format,
            const uint32_t *spv, size_t spv_bytes, int *passes, int *fails)
{
   int is_r32ui = !strncmp(variant_name, "r32ui", 5);
   /* Formatless variants have no atomics (GLSL needs the r32ui format). */
   int has_atomics = is_r32ui && !strstr(variant_name, "nofmt");

   /* Storage image: 2D, 4x4, arrayLayers 2, mipLevels 1, OPTIMAL tiling,
    * usage STORAGE|TRANSFER_SRC|TRANSFER_DST. View 2D_ARRAY, layers 0..1, GENERAL layout. */
   VkImageCreateInfo aici = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = format,
      .extent = { 4, 4, 1 },
      .mipLevels = 1,
      .arrayLayers = 2,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
   };
   VkImage storage_img = VK_NULL_HANDLE;
   CK(vkCreateImage(t->dev, &aici, NULL, &storage_img), "CreateStorageImage");
   VkDeviceMemory storage_mem = alloc_image_memory(t, storage_img);

   VkImageViewCreateInfo aivci = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = storage_img,
      .viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
      .format = format,
      .subresourceRange = {
         .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
         .baseMipLevel = 0,
         .levelCount = 1,
         .baseArrayLayer = 0,
         .layerCount = 2,
      },
   };
   VkImageView storage_view = VK_NULL_HANDLE;
   CK(vkCreateImageView(t->dev, &aivci, NULL, &storage_view), "CreateStorageImageView");

   /* Sampled image: 2D, 4x4, arrayLayers 2, mipLevels 2 (level1 is 2x2),
    * usage SAMPLED|TRANSFER_DST. View 2D_ARRAY, levels 0..1, layers 0..1,
    * layout SHADER_READ_ONLY_OPTIMAL. */
   VkImageCreateInfo bici = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = format,
      .extent = { 4, 4, 1 },
      .mipLevels = 2,
      .arrayLayers = 2,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
   };
   VkImage sampled_img = VK_NULL_HANDLE;
   CK(vkCreateImage(t->dev, &bici, NULL, &sampled_img), "CreateSampledImage");
   VkDeviceMemory sampled_mem = alloc_image_memory(t, sampled_img);

   VkImageViewCreateInfo bivci = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = sampled_img,
      .viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
      .format = format,
      .subresourceRange = {
         .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
         .baseMipLevel = 0,
         .levelCount = 2,
         .baseArrayLayer = 0,
         .layerCount = 2,
      },
   };
   VkImageView sampled_view = VK_NULL_HANDLE;
   CK(vkCreateImageView(t->dev, &bivci, NULL, &sampled_view), "CreateSampledImageView");

   /* Sampler: NEAREST, mipmapMode NEAREST, CLAMP_TO_EDGE, unnormalizedCoordinates false, maxLod 16. */
   VkSamplerCreateInfo sci = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .magFilter = VK_FILTER_NEAREST,
      .minFilter = VK_FILTER_NEAREST,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
      .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .unnormalizedCoordinates = VK_FALSE,
      .minLod = 0.0f,
      .maxLod = 16.0f,
   };
   VkSampler sampler = VK_NULL_HANDLE;
   CK(vkCreateSampler(t->dev, &sci, NULL, &sampler), "CreateSampler");

   /* Texel buffer: 16 texels filled with nonzero pattern, usage UNIFORM_TEXEL_BUFFER;
    * buffer view offset 0 and range = 4 texels. */
   uint8_t tbuf_init[64];
   if (is_r32ui) {
      uint32_t *t32 = (uint32_t *)tbuf_init;
      for (int i = 0; i < 16; i++)
         t32[i] = 0x5000u + (uint32_t)i;
   } else {
      for (int i = 0; i < 16; i++) {
         tbuf_init[i * 4 + 0] = (uint8_t)(i + 10);
         tbuf_init[i * 4 + 1] = (uint8_t)(i + 20);
         tbuf_init[i * 4 + 2] = (uint8_t)(i + 30);
         tbuf_init[i * 4 + 3] = 250;
      }
   }
   VkBuffer tbuf_buf = VK_NULL_HANDLE;
   VkDeviceMemory tbuf_mem = VK_NULL_HANDLE;
   dx7_buffer(t, 64, VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT, tbuf_init, &tbuf_buf, &tbuf_mem);

   VkBufferViewCreateInfo bvci = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
      .buffer = tbuf_buf,
      .format = format,
      .offset = 0,
      .range = 16, /* 4 texels */
   };
   VkBufferView tbuf_view = VK_NULL_HANDLE;
   CK(vkCreateBufferView(t->dev, &bvci, NULL, &tbuf_view), "CreateBufferView");

   /* Output SSBO: 32 uvec4 (512 bytes), prefilled with 0xDEADBEEF. */
   uint32_t ssbo_init[32 * 4];
   for (int i = 0; i < 32 * 4; i++)
      ssbo_init[i] = 0xDEADBEEFu;
   VkBuffer ssbo_buf = VK_NULL_HANDLE;
   VkDeviceMemory ssbo_mem = VK_NULL_HANDLE;
   dx7_buffer(t, 512, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, ssbo_init, &ssbo_buf, &ssbo_mem);

   /* Staging buffer for image uploads:
    * Offset 0..127: storage image (level 0, 2 layers, 4x4)
    * Offset 128..255: sampled image level 0 (2 layers, 4x4)
    * Offset 256..287: sampled image level 1 (2 layers, 2x2) */
   uint8_t staging_data[288];
   memset(staging_data, 0, sizeof(staging_data));
   for (int layer = 0; layer < 2; layer++) {
      for (int y = 0; y < 4; y++) {
         for (int x = 0; x < 4; x++) {
            int off = layer * 64 + (y * 4 + x) * 4;
            if (is_r32ui) {
               uint32_t v = get_texel_r32(x, y, layer, 0);
               memcpy(staging_data + off, &v, sizeof(v));
            } else {
               get_texel_rgba8(x, y, layer, 0, staging_data + off);
            }
         }
      }
   }
   for (int layer = 0; layer < 2; layer++) {
      for (int y = 0; y < 4; y++) {
         for (int x = 0; x < 4; x++) {
            int off = 128 + layer * 64 + (y * 4 + x) * 4;
            if (is_r32ui) {
               uint32_t v = get_texel_r32(x, y, layer, 0);
               memcpy(staging_data + off, &v, sizeof(v));
            } else {
               get_texel_rgba8(x, y, layer, 0, staging_data + off);
            }
         }
      }
   }
   for (int layer = 0; layer < 2; layer++) {
      for (int y = 0; y < 2; y++) {
         for (int x = 0; x < 2; x++) {
            int off = 256 + layer * 16 + (y * 2 + x) * 4;
            if (is_r32ui) {
               uint32_t v = get_texel_r32(x, y, layer, 1);
               memcpy(staging_data + off, &v, sizeof(v));
            } else {
               get_texel_rgba8(x, y, layer, 1, staging_data + off);
            }
         }
      }
   }
   VkBuffer stage_buf = VK_NULL_HANDLE;
   VkDeviceMemory stage_mem = VK_NULL_HANDLE;
   dx7_buffer(t, 288, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, staging_data, &stage_buf, &stage_mem);

   /* Readback buffer for storage image (both layers: 32 texels = 128 bytes). */
   VkBuffer readback_buf = VK_NULL_HANDLE;
   VkDeviceMemory readback_mem = VK_NULL_HANDLE;
   dx7_buffer(t, 128, VK_BUFFER_USAGE_TRANSFER_DST_BIT, NULL, &readback_buf, &readback_mem);

   /* Descriptor set layout & pipeline layout. */
   VkDescriptorSetLayoutBinding bindings[4] = {
      { .binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT },
      { .binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT },
      { .binding = 2, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT },
      { .binding = 3, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT },
   };
   VkDescriptorSetLayoutCreateInfo dslci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = 4,
      .pBindings = bindings,
   };
   VkDescriptorSetLayout dsl = VK_NULL_HANDLE;
   CK(vkCreateDescriptorSetLayout(t->dev, &dslci, NULL, &dsl), "CreateDSL");

   VkPipelineLayoutCreateInfo plci = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1,
      .pSetLayouts = &dsl,
   };
   VkPipelineLayout playout = VK_NULL_HANDLE;
   CK(vkCreatePipelineLayout(t->dev, &plci, NULL, &playout), "CreatePipelineLayout");

   /* Descriptor pool & descriptor set. */
   VkDescriptorPoolSize pool_sizes[4] = {
      { .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .descriptorCount = 1 },
      { .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 1 },
      { .type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, .descriptorCount = 1 },
      { .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1 },
   };
   VkDescriptorPoolCreateInfo dpci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = 1,
      .poolSizeCount = 4,
      .pPoolSizes = pool_sizes,
   };
   VkDescriptorPool dpool = VK_NULL_HANDLE;
   CK(vkCreateDescriptorPool(t->dev, &dpci, NULL, &dpool), "CreateDP");

   VkDescriptorSetAllocateInfo dsai = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = dpool,
      .descriptorSetCount = 1,
      .pSetLayouts = &dsl,
   };
   VkDescriptorSet dset = VK_NULL_HANDLE;
   CK(vkAllocateDescriptorSets(t->dev, &dsai, &dset), "AllocDS");

   VkDescriptorImageInfo dii0 = {
      .sampler = VK_NULL_HANDLE,
      .imageView = storage_view,
      .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
   };
   VkDescriptorImageInfo dii1 = {
      .sampler = sampler,
      .imageView = sampled_view,
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
   };
   VkDescriptorBufferInfo dbi3 = {
      .buffer = ssbo_buf,
      .offset = 0,
      .range = 512,
   };
   VkWriteDescriptorSet writes[4] = {
      { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = dset, .dstBinding = 0, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .pImageInfo = &dii0 },
      { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = dset, .dstBinding = 1, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .pImageInfo = &dii1 },
      { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = dset, .dstBinding = 2, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, .pTexelBufferView = &tbuf_view },
      { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = dset, .dstBinding = 3, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .pBufferInfo = &dbi3 },
   };
   vkUpdateDescriptorSets(t->dev, 4, writes, 0, NULL);

   VkPipeline pipe = create_compute_pipe(t, spv, spv_bytes, playout);

   /* Record and execute commands. */
   CK(vkResetFences(t->dev, 1, &t->fence), "ResetFence");
   CK(vkResetCommandBuffer(t->cmd, 0), "ResetCmd");
   VkCommandBufferBeginInfo bbi = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
   };
   CK(vkBeginCommandBuffer(t->cmd, &bbi), "BeginCmd");

   VkImageMemoryBarrier pre_upload_bars[2] = {
      {
         .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
         .srcAccessMask = 0,
         .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
         .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
         .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
         .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
         .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
         .image = storage_img,
         .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 2 },
      },
      {
         .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
         .srcAccessMask = 0,
         .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
         .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
         .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
         .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
         .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
         .image = sampled_img,
         .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 2, 0, 2 },
      },
   };
   vkCmdPipelineBarrier(t->cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                        0, 0, NULL, 0, NULL, 2, pre_upload_bars);

   VkBufferImageCopy storage_copies[2] = {
      {
         .bufferOffset = 0,
         .imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
         .imageExtent = { 4, 4, 1 },
      },
      {
         .bufferOffset = 64,
         .imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 1 },
         .imageExtent = { 4, 4, 1 },
      },
   };
   vkCmdCopyBufferToImage(t->cmd, stage_buf, storage_img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 2, storage_copies);

   VkBufferImageCopy sampled_copies[4] = {
      {
         .bufferOffset = 128,
         .imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
         .imageExtent = { 4, 4, 1 },
      },
      {
         .bufferOffset = 128 + 64,
         .imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 1 },
         .imageExtent = { 4, 4, 1 },
      },
      {
         .bufferOffset = 256,
         .imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 1, 0, 1 },
         .imageExtent = { 2, 2, 1 },
      },
      {
         .bufferOffset = 256 + 16,
         .imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 1, 1, 1 },
         .imageExtent = { 2, 2, 1 },
      },
   };
   vkCmdCopyBufferToImage(t->cmd, stage_buf, sampled_img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 4, sampled_copies);

   VkImageMemoryBarrier pre_dispatch_bars[2] = {
      {
         .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
         .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
         .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
         .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
         .newLayout = VK_IMAGE_LAYOUT_GENERAL,
         .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
         .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
         .image = storage_img,
         .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 2 },
      },
      {
         .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
         .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
         .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
         .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
         .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
         .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
         .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
         .image = sampled_img,
         .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 2, 0, 2 },
      },
   };
   vkCmdPipelineBarrier(t->cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                        0, 0, NULL, 0, NULL, 2, pre_dispatch_bars);

   vkCmdBindPipeline(t->cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipe);
   vkCmdBindDescriptorSets(t->cmd, VK_PIPELINE_BIND_POINT_COMPUTE, playout, 0, 1, &dset, 0, NULL);
   vkCmdDispatch(t->cmd, 1, 1, 1);

   VkImageMemoryBarrier post_dispatch_bar = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
      .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
      .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = storage_img,
      .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 2 },
   };
   VkBufferMemoryBarrier ssbo_bar = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
      .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
      .dstAccessMask = VK_ACCESS_HOST_READ_BIT,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .buffer = ssbo_buf,
      .offset = 0,
      .size = 512,
   };
   vkCmdPipelineBarrier(t->cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                        VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_HOST_BIT,
                        0, 0, NULL, 1, &ssbo_bar, 1, &post_dispatch_bar);

   VkBufferImageCopy readback_copies[2] = {
      {
         .bufferOffset = 0,
         .imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
         .imageExtent = { 4, 4, 1 },
      },
      {
         .bufferOffset = 64,
         .imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 1 },
         .imageExtent = { 4, 4, 1 },
      },
   };
   vkCmdCopyImageToBuffer(t->cmd, storage_img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readback_buf, 2, readback_copies);

   VkBufferMemoryBarrier readback_bar = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
      .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
      .dstAccessMask = VK_ACCESS_HOST_READ_BIT,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .buffer = readback_buf,
      .offset = 0,
      .size = 128,
   };
   vkCmdPipelineBarrier(t->cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
                        0, 0, NULL, 1, &readback_bar, 0, NULL);

   CK(vkEndCommandBuffer(t->cmd), "EndCmd");

   VkSubmitInfo si = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers = &t->cmd,
   };
   CK(vkQueueSubmit(t->queue, 1, &si, t->fence), "QueueSubmit");
   CK(vkWaitForFences(t->dev, 1, &t->fence, VK_TRUE, 30ull * 1000000000ull), "WaitForFences");

   /* Read back output buffer and storage image buffer. */
   uint32_t out_r[32][4];
   void *map_ptr = NULL;
   CK(vkMapMemory(t->dev, ssbo_mem, 0, 512, 0, &map_ptr), "MapSSBO");
   memcpy(out_r, map_ptr, 512);
   vkUnmapMemory(t->dev, ssbo_mem);

   uint8_t out_storage[128];
   CK(vkMapMemory(t->dev, readback_mem, 0, 128, 0, &map_ptr), "MapReadback");
   memcpy(out_storage, map_ptr, 128);
   vkUnmapMemory(t->dev, readback_mem);

   /* Define zero texel Z:
    * for R32_UINT Z = (0,0,0,1) as uints.
    * for RGBA8 UNORM Z = (0,0,0,0) as float bits. */
   static const uint32_t z_r32ui[4] = { 0, 0, 0, 1 };
   static const uint32_t z_rgba8[4] = { 0, 0, 0, 0 };
   const uint32_t *Z = is_r32ui ? z_r32ui : z_rgba8;

   /* Storage image loads:
    * r[0] = in bounds texel (1, 2, layer 1)
    * r[1..6] = Z (load_x, load_y, load_layer, load_neg_x, load_x_alias65537, load_layer_alias65536) */
   uint32_t want0[4];
   if (is_r32ui) {
      want0[0] = 0x1019u;
      want0[1] = 0;
      want0[2] = 0;
      want0[3] = 1;
      check_uvec4(variant_name, "load", out_r[0], want0, 0, passes, fails);
   } else {
      float f0[4] = { 17.0f / 255.0f, 34.0f / 255.0f, 19.0f / 255.0f, 200.0f / 255.0f };
      memcpy(want0, f0, sizeof(want0));
      check_uvec4(variant_name, "load", out_r[0], want0, 1, passes, fails);
   }
   check_uvec4(variant_name, "load_x", out_r[1], Z, 0, passes, fails);
   check_uvec4(variant_name, "load_y", out_r[2], Z, 0, passes, fails);
   check_uvec4(variant_name, "load_layer", out_r[3], Z, 0, passes, fails);
   check_uvec4(variant_name, "load_neg_x", out_r[4], Z, 0, passes, fails);
   check_uvec4(variant_name, "load_x_alias65537", out_r[5], Z, 0, passes, fails);
   check_uvec4(variant_name, "load_layer_alias65536", out_r[6], Z, 0, passes, fails);

   /* Sampled image fetches:
    * r[8] = in bounds texel (1, 1, layer 1, level 1)
    * r[9..13] = Z (fetch_lod, fetch_x_lod1, fetch_layer, fetch_neg_y, fetch_lod31) */
   uint32_t want8[4];
   if (is_r32ui) {
      want8[0] = 0x1115u;
      want8[1] = 0;
      want8[2] = 0;
      want8[3] = 1;
      check_uvec4(variant_name, "fetch", out_r[8], want8, 0, passes, fails);
   } else {
      float f8[4] = { 17.0f / 255.0f, 18.0f / 255.0f, 83.0f / 255.0f, 200.0f / 255.0f };
      memcpy(want8, f8, sizeof(want8));
      check_uvec4(variant_name, "fetch", out_r[8], want8, 1, passes, fails);
   }
   check_uvec4(variant_name, "fetch_lod", out_r[9], Z, 0, passes, fails);
   check_uvec4(variant_name, "fetch_x_lod1", out_r[10], Z, 0, passes, fails);
   check_uvec4(variant_name, "fetch_layer", out_r[11], Z, 0, passes, fails);
   check_uvec4(variant_name, "fetch_neg_y", out_r[12], Z, 0, passes, fails);
   check_uvec4(variant_name, "fetch_lod31", out_r[13], Z, 0, passes, fails);

   /* Texel buffer fetches:
    * r[16] = in bounds texel 3
    * r[17..18] = Z (tbuf_oob, tbuf_alias65537) */
   uint32_t want16[4];
   if (is_r32ui) {
      want16[0] = 0x5003u;
      want16[1] = 0;
      want16[2] = 0;
      want16[3] = 1;
      check_uvec4(variant_name, "tbuf", out_r[16], want16, 0, passes, fails);
   } else {
      float f16[4] = { 13.0f / 255.0f, 23.0f / 255.0f, 33.0f / 255.0f, 250.0f / 255.0f };
      memcpy(want16, f16, sizeof(want16));
      check_uvec4(variant_name, "tbuf", out_r[16], want16, 1, passes, fails);
   }
   check_uvec4(variant_name, "tbuf_oob", out_r[17], Z, 0, passes, fails);
   check_uvec4(variant_name, "tbuf_alias65537", out_r[18], Z, 0, passes, fails);

   /* Stores (read back storage image):
    * texel (3,3,1) must equal stored 7 (R32: 7, RGBA8: 255,255,255,255 -> 0xFFFFFFFF)
    * (0,3,1) for R32 must be original+1 (in-bounds atomicAdd)
    * Every other texel of both layers must be unchanged */
   uint32_t got_store = *(uint32_t *)(out_storage + 1 * 64 + (3 * 4 + 3) * 4);
   uint32_t want_store = is_r32ui ? 7u : 0xFFFFFFFFu;
   if (got_store == want_store) {
      printf("PASS case %s.store\n", variant_name);
      (*passes)++;
   } else {
      failf("%s.store got=(0x%08x) want=(0x%08x)", variant_name, got_store, want_store);
      (*fails)++;
   }

   if (has_atomics) {
      uint32_t got_atomic_store = *(uint32_t *)(out_storage + 1 * 64 + (3 * 4 + 0) * 4);
      uint32_t want_atomic_store = 0x101cu + 1u;
      if (got_atomic_store == want_atomic_store) {
         printf("PASS case %s.store_atomic\n", variant_name);
         (*passes)++;
      } else {
         failf("%s.store_atomic got=(0x%08x) want=(0x%08x)",
                variant_name, got_atomic_store, want_atomic_store);
         (*fails)++;
      }
   }

   bool store_oob_pass = true;
   uint32_t bad_got = 0, bad_want = 0;
   for (int layer = 0; layer < 2 && store_oob_pass; layer++) {
      for (int y = 0; y < 4 && store_oob_pass; y++) {
         for (int x = 0; x < 4 && store_oob_pass; x++) {
            if (layer == 1 && y == 3 && x == 3)
               continue;
            if (has_atomics && layer == 1 && y == 3 && x == 0)
               continue;
            int off = layer * 64 + (y * 4 + x) * 4;
            uint32_t cur = *(uint32_t *)(out_storage + off);
            uint32_t exp;
            if (is_r32ui) {
               exp = get_texel_r32(x, y, layer, 0);
            } else {
               uint8_t eb[4];
               get_texel_rgba8(x, y, layer, 0, eb);
               memcpy(&exp, eb, sizeof(exp));
            }
            if (cur != exp) {
               store_oob_pass = false;
               bad_got = cur;
               bad_want = exp;
            }
         }
      }
   }
   if (store_oob_pass) {
      printf("PASS case %s.store_oob\n", variant_name);
      (*passes)++;
   } else {
      failf("%s.store_oob got=(0x%08x) want=(0x%08x)", variant_name, bad_got, bad_want);
      (*fails)++;
   }

   /* Atomics (R32 variant only):
    * r[24].x == original value of texel (0,3,1); r[25..29].x == 0 */
   if (has_atomics) {
      uint32_t want24[4] = { 0x101cu, 0, 0, 0 };
      check_uvec4(variant_name, "atomic", out_r[24], want24, 0, passes, fails);
      uint32_t zero4[4] = { 0, 0, 0, 0 };
      check_uvec4(variant_name, "atomic_x", out_r[25], zero4, 0, passes, fails);
      check_uvec4(variant_name, "atomic_layer", out_r[26], zero4, 0, passes, fails);
      check_uvec4(variant_name, "atomic_xchg_neg", out_r[27], zero4, 0, passes, fails);
      check_uvec4(variant_name, "atomic_cas_alias", out_r[28], zero4, 0, passes, fails);
      check_uvec4(variant_name, "atomic_max_y", out_r[29], zero4, 0, passes, fails);
   }

   /* Sanity check: entries the shader does not write must still be 0xDEADBEEF */
   bool sanity_pass = true;
   uint32_t san_got[4] = { 0 };
   uint32_t san_want[4] = { 0xDEADBEEFu, 0xDEADBEEFu, 0xDEADBEEFu, 0xDEADBEEFu };
   for (int i = 0; i < 32 && sanity_pass; i++) {
      bool is_written = false;
      if (i >= 0 && i <= 6)
         is_written = true;
      if (i >= 8 && i <= 13)
         is_written = true;
      if (i >= 16 && i <= 18)
         is_written = true;
      if (has_atomics && i >= 24 && i <= 29)
         is_written = true;
      if (!is_written) {
         for (int c = 0; c < 4; c++) {
            if (out_r[i][c] != 0xDEADBEEFu) {
               sanity_pass = false;
               memcpy(san_got, out_r[i], sizeof(san_got));
               break;
            }
         }
      }
   }
   if (sanity_pass) {
      printf("PASS case %s.sanity\n", variant_name);
      (*passes)++;
   } else {
      failf("%s.sanity got=(0x%08x,0x%08x,0x%08x,0x%08x) want=(0x%08x,0x%08x,0x%08x,0x%08x)",
             variant_name, san_got[0], san_got[1], san_got[2], san_got[3],
             san_want[0], san_want[1], san_want[2], san_want[3]);
      (*fails)++;
   }

   /* Cleanup Vulkan objects */
   vkDestroyPipeline(t->dev, pipe, NULL);
   vkDestroyPipelineLayout(t->dev, playout, NULL);
   vkDestroyDescriptorPool(t->dev, dpool, NULL);
   vkDestroyDescriptorSetLayout(t->dev, dsl, NULL);
   vkDestroySampler(t->dev, sampler, NULL);
   vkDestroyBufferView(t->dev, tbuf_view, NULL);
   vkDestroyBuffer(t->dev, tbuf_buf, NULL);
   vkFreeMemory(t->dev, tbuf_mem, NULL);
   vkDestroyBuffer(t->dev, ssbo_buf, NULL);
   vkFreeMemory(t->dev, ssbo_mem, NULL);
   vkDestroyBuffer(t->dev, stage_buf, NULL);
   vkFreeMemory(t->dev, stage_mem, NULL);
   vkDestroyBuffer(t->dev, readback_buf, NULL);
   vkFreeMemory(t->dev, readback_mem, NULL);
   vkDestroyImageView(t->dev, storage_view, NULL);
   vkDestroyImage(t->dev, storage_img, NULL);
   vkFreeMemory(t->dev, storage_mem, NULL);
   vkDestroyImageView(t->dev, sampled_view, NULL);
   vkDestroyImage(t->dev, sampled_img, NULL);
   vkFreeMemory(t->dev, sampled_mem, NULL);
}

static double
now_ms(void)
{
   struct timespec ts;
   clock_gettime(CLOCK_MONOTONIC, &ts);
   return ts.tv_sec * 1e3 + ts.tv_nsec / 1e6;
}

/* Storage-image load cost with robustImageAccess2 (device default) against
 * the same shader with image robustness 1 (VK_EXT_pipeline_robustness).
 * v10 adds a bounds check for alpha in the first one only. Informational:
 * prints a BENCH line, never fails the test.
 */
static void
bench(struct dx7 *t)
{
   if (!has_pipe_rob) {
      printf("BENCH skipped: no VK_EXT_pipeline_robustness\n");
      return;
   }

   enum { N = 4096 };
   VkImageCreateInfo ici = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = VK_FORMAT_R8G8B8A8_UNORM,
      .extent = { N, N, 1 },
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_STORAGE_BIT,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
   };
   VkImage img = VK_NULL_HANDLE;
   CK(vkCreateImage(t->dev, &ici, NULL, &img), "BenchImage");
   VkDeviceMemory img_mem = alloc_image_memory(t, img);
   VkImageViewCreateInfo ivci = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = img,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = VK_FORMAT_R8G8B8A8_UNORM,
      .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
   };
   VkImageView view = VK_NULL_HANDLE;
   CK(vkCreateImageView(t->dev, &ivci, NULL, &view), "BenchView");
   VkBuffer out_buf = VK_NULL_HANDLE;
   VkDeviceMemory out_mem = VK_NULL_HANDLE;
   dx7_buffer(t, 16, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, NULL, &out_buf, &out_mem);

   VkDescriptorSetLayoutBinding b[2] = {
      { .binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT },
      { .binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT },
   };
   VkDescriptorSetLayoutCreateInfo dslci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = 2,
      .pBindings = b,
   };
   VkDescriptorSetLayout dsl = VK_NULL_HANDLE;
   CK(vkCreateDescriptorSetLayout(t->dev, &dslci, NULL, &dsl), "BenchDSL");
   VkPipelineLayoutCreateInfo plci = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1,
      .pSetLayouts = &dsl,
   };
   VkPipelineLayout pl = VK_NULL_HANDLE;
   CK(vkCreatePipelineLayout(t->dev, &plci, NULL, &pl), "BenchPL");
   VkDescriptorPoolSize ps[2] = {
      { .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .descriptorCount = 1 },
      { .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1 },
   };
   VkDescriptorPoolCreateInfo dpci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = 1,
      .poolSizeCount = 2,
      .pPoolSizes = ps,
   };
   VkDescriptorPool dp = VK_NULL_HANDLE;
   CK(vkCreateDescriptorPool(t->dev, &dpci, NULL, &dp), "BenchDP");
   VkDescriptorSetAllocateInfo dsai = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = dp,
      .descriptorSetCount = 1,
      .pSetLayouts = &dsl,
   };
   VkDescriptorSet ds = VK_NULL_HANDLE;
   CK(vkAllocateDescriptorSets(t->dev, &dsai, &ds), "BenchDS");
   VkDescriptorImageInfo dii = { .imageView = view, .imageLayout = VK_IMAGE_LAYOUT_GENERAL };
   VkDescriptorBufferInfo dbi = { .buffer = out_buf, .range = 16 };
   VkWriteDescriptorSet w[2] = {
      { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = ds, .dstBinding = 0, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .pImageInfo = &dii },
      { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = ds, .dstBinding = 1, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .pBufferInfo = &dbi },
   };
   vkUpdateDescriptorSets(t->dev, 2, w, 0, NULL);

   /* [0] = robustImageAccess2 (device default), [1] = robustImageAccess. */
   VkPipelineRobustnessCreateInfoEXT rob1 = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_ROBUSTNESS_CREATE_INFO_EXT,
      .storageBuffers = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DEVICE_DEFAULT_EXT,
      .uniformBuffers = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DEVICE_DEFAULT_EXT,
      .vertexInputs = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DEVICE_DEFAULT_EXT,
      .images = VK_PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR_ROBUST_IMAGE_ACCESS_EXT,
   };
   VkPipeline pipe[2];
   VkShaderModule sm = dx7_module(t, ri2_bench_spv, sizeof(ri2_bench_spv));
   for (int i = 0; i < 2; i++) {
      VkComputePipelineCreateInfo cpci = {
         .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
         .pNext = i ? &rob1 : NULL,
         .flags = has_exec_props ? VK_PIPELINE_CREATE_CAPTURE_STATISTICS_BIT_KHR : 0,
         .stage = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = sm,
            .pName = "main",
         },
         .layout = pl,
      };
      CK(vkCreateComputePipelines(t->dev, VK_NULL_HANDLE, 1, &cpci, NULL, &pipe[i]), "BenchPipe");
   }
   vkDestroyShaderModule(t->dev, sm, NULL);

   /* Run each pipeline 1 + 7 times, interleaved, keep the fastest. */
   double best[2] = { 1e30, 1e30 };
   for (int run = 0; run < 16; run++) {
      int i = run & 1;
      CK(vkResetFences(t->dev, 1, &t->fence), "ResetFence");
      CK(vkResetCommandBuffer(t->cmd, 0), "ResetCmd");
      VkCommandBufferBeginInfo bbi = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
      CK(vkBeginCommandBuffer(t->cmd, &bbi), "BeginCmd");
      if (run == 0) {
         VkImageMemoryBarrier bar = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_GENERAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = img,
            .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
         };
         vkCmdPipelineBarrier(t->cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                              VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, NULL,
                              0, NULL, 1, &bar);
      }
      vkCmdBindPipeline(t->cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipe[i]);
      vkCmdBindDescriptorSets(t->cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pl, 0, 1, &ds, 0, NULL);
      vkCmdDispatch(t->cmd, N / 8, N / 8, 1);
      CK(vkEndCommandBuffer(t->cmd), "EndCmd");
      VkSubmitInfo si = {
         .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
         .commandBufferCount = 1,
         .pCommandBuffers = &t->cmd,
      };
      double t0 = now_ms();
      CK(vkQueueSubmit(t->queue, 1, &si, t->fence), "QueueSubmit");
      CK(vkWaitForFences(t->dev, 1, &t->fence, VK_TRUE, 30ull * 1000000000ull), "WaitForFences");
      double ms = now_ms() - t0;
      if (run >= 2 && ms < best[i])
         best[i] = ms;
   }

   char stats[512] = "";
   PFN_vkGetDeviceProcAddr gdpa =
      (PFN_vkGetDeviceProcAddr)gipa(t->inst, "vkGetDeviceProcAddr");
   PFN_vkGetPipelineExecutableStatisticsKHR get_stats = has_exec_props && gdpa ?
      (PFN_vkGetPipelineExecutableStatisticsKHR)gdpa(t->dev, "vkGetPipelineExecutableStatisticsKHR") : NULL;
   if (get_stats) {
      VkPipelineExecutableStatisticKHR st[2][32];
      uint32_t n[2];
      for (int i = 0; i < 2; i++) {
         VkPipelineExecutableInfoKHR ei = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_INFO_KHR,
            .pipeline = pipe[i],
         };
         n[i] = 32;
         for (uint32_t k = 0; k < 32; k++)
            st[i][k] = (VkPipelineExecutableStatisticKHR){ .sType = VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_STATISTIC_KHR };
         if (get_stats(t->dev, &ei, &n[i], st[i]) < 0)
            n[i] = 0;
      }
      size_t len = 0;
      for (uint32_t k = 0; k < n[0] && k < n[1] && len + 64 < sizeof(stats); k++) {
         if (st[0][k].format != VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR)
            continue;
         len += snprintf(stats + len, sizeof(stats) - len, " %s=%llu/%llu",
                         st[0][k].name,
                         (unsigned long long)st[0][k].value.u64,
                         (unsigned long long)st[1][k].value.u64);
      }
   }

   printf("BENCH 4096x4096x16 rgba8 loads ria2=%.2fms ria1=%.2fms delta=%+.1f%% stats(ria2/ria1):%s\n",
          best[0], best[1], (best[0] / best[1] - 1.0) * 100.0, stats);

   vkDestroyPipeline(t->dev, pipe[0], NULL);
   vkDestroyPipeline(t->dev, pipe[1], NULL);
   vkDestroyPipelineLayout(t->dev, pl, NULL);
   vkDestroyDescriptorPool(t->dev, dp, NULL);
   vkDestroyDescriptorSetLayout(t->dev, dsl, NULL);
   vkDestroyBuffer(t->dev, out_buf, NULL);
   vkFreeMemory(t->dev, out_mem, NULL);
   vkDestroyImageView(t->dev, view, NULL);
   vkDestroyImage(t->dev, img, NULL);
   vkFreeMemory(t->dev, img_mem, NULL);
}

int
main(int argc, char **argv)
{
   if (argc < 2) {
      printf("Usage: %s <path-to-libvulkan_panfrost.so>\n", argv[0]);
      return 1;
   }

   void *h = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
   if (!h) {
      printf("FAIL dlopen %s\n", dlerror());
      return 1;
   }
   gipa = (PFN_vkGetInstanceProcAddr)dlsym(h, "vk_icdGetInstanceProcAddr");
   if (!gipa)
      gipa = (PFN_vkGetInstanceProcAddr)dlsym(h, "vkGetInstanceProcAddr");
   if (!gipa) {
      printf("FAIL missing gipa\n");
      return 1;
   }

   dx7_device_hook = device_hook;
   struct dx7 t;
   dx7_init(&t, argv[1], NULL);
   load_extra_funcs(&t, argv[1]);

   /* Unsupported feature: nothing to check (v10 LD_TEX returns alpha 1 OOB). */
   if (!supported_rob2.robustImageAccess2) {
      printf("RESULT SKIP robustImageAccess2 not supported\n");
      return 0;
   }

   int passes = 0;
   int fails = 0;

   run_variant(&t, "r32ui", VK_FORMAT_R32_UINT, ri2_r32ui_spv, sizeof(ri2_r32ui_spv), &passes, &fails);
   run_variant(&t, "rgba8", VK_FORMAT_R8G8B8A8_UNORM, ri2_rgba8_spv, sizeof(ri2_rgba8_spv), &passes, &fails);
   /* Formatless: the alpha fix-up must come from the descriptor format. */
   if (t.feats.shaderStorageImageReadWithoutFormat && t.feats.shaderStorageImageWriteWithoutFormat) {
      run_variant(&t, "r32ui_nofmt", VK_FORMAT_R32_UINT, ri2_r32ui_nofmt_spv, sizeof(ri2_r32ui_nofmt_spv), &passes, &fails);
      run_variant(&t, "rgba8_nofmt", VK_FORMAT_R8G8B8A8_UNORM, ri2_rgba8_nofmt_spv, sizeof(ri2_rgba8_nofmt_spv), &passes, &fails);
   }
   bench(&t);

   if (fails > 0) {
      printf("BENCH failed=%d:", fails);
      for (int i = 0; i < fail_count && i < 16; i++)
         printf(" %s", fail_list[i]);
      printf("\n");
      for (int i = 0; i < fail_count && i < 16; i++)
         printf("FAILED: %s\n", fail_list[i]);
      printf("RESULT FAIL\n");
      return 1;
   } else {
      printf("RESULT PASS\n");
      return 0;
   }
}
