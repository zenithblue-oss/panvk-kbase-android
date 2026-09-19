/*
 * bc-format-probe.c — Query every standard BC Vulkan format directly via PanVK,
 * plus ETC2/ASTC sanity controls.
 */

#define VK_USE_PLATFORM_ANDROID_KHR 1
#include <dlfcn.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

struct format_entry {
   VkFormat format;
   const char *name;
};

static const struct format_entry bc_formats[] = {
   { VK_FORMAT_BC1_RGB_UNORM_BLOCK,   "BC1_RGB_UNORM_BLOCK" },
   { VK_FORMAT_BC1_RGB_SRGB_BLOCK,    "BC1_RGB_SRGB_BLOCK" },
   { VK_FORMAT_BC1_RGBA_UNORM_BLOCK,  "BC1_RGBA_UNORM_BLOCK" },
   { VK_FORMAT_BC1_RGBA_SRGB_BLOCK,   "BC1_RGBA_SRGB_BLOCK" },
   { VK_FORMAT_BC2_UNORM_BLOCK,       "BC2_UNORM_BLOCK" },
   { VK_FORMAT_BC2_SRGB_BLOCK,        "BC2_SRGB_BLOCK" },
   { VK_FORMAT_BC3_UNORM_BLOCK,       "BC3_UNORM_BLOCK" },
   { VK_FORMAT_BC3_SRGB_BLOCK,        "BC3_SRGB_BLOCK" },
   { VK_FORMAT_BC4_UNORM_BLOCK,       "BC4_UNORM_BLOCK" },
   { VK_FORMAT_BC4_SNORM_BLOCK,       "BC4_SNORM_BLOCK" },
   { VK_FORMAT_BC5_UNORM_BLOCK,       "BC5_UNORM_BLOCK" },
   { VK_FORMAT_BC5_SNORM_BLOCK,       "BC5_SNORM_BLOCK" },
   { VK_FORMAT_BC6H_UFLOAT_BLOCK,     "BC6H_UFLOAT_BLOCK" },
   { VK_FORMAT_BC6H_SFLOAT_BLOCK,     "BC6H_SFLOAT_BLOCK" },
   { VK_FORMAT_BC7_UNORM_BLOCK,       "BC7_UNORM_BLOCK" },
   { VK_FORMAT_BC7_SRGB_BLOCK,        "BC7_SRGB_BLOCK" },
};

static const struct format_entry control_formats[] = {
   { VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK,   "ETC2_R8G8B8_UNORM_BLOCK" },
   { VK_FORMAT_ASTC_4x4_UNORM_BLOCK,      "ASTC_4x4_UNORM_BLOCK" },
};

static void probe_format_list(VkPhysicalDevice phys,
                              PFN_vkGetPhysicalDeviceFormatProperties gfp,
                              PFN_vkGetPhysicalDeviceFormatProperties2 gfp2,
                              PFN_vkGetPhysicalDeviceImageFormatProperties2 gifp2,
                              const struct format_entry *entries,
                              size_t count,
                              unsigned *out_supported)
{
   unsigned supported = 0;
   for (size_t i = 0; i < count; i++) {
      VkFormat fmt = entries[i].format;
      const char *name = entries[i].name;

      VkFormatProperties fp = { 0 };
      gfp(phys, fmt, &fp);

      VkFormatProperties3 fp3 = {
         .sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_3,
      };
      VkFormatProperties2 fp2 = {
         .sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2,
         .pNext = &fp3,
      };
      if (gfp2)
         gfp2(phys, fmt, &fp2);

      uint32_t opt_flags = fp.optimalTilingFeatures;
      uint64_t opt_flags2 = fp3.optimalTilingFeatures;

      bool has_sampled = (opt_flags & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) != 0;
      bool has_blit_src = (opt_flags & VK_FORMAT_FEATURE_BLIT_SRC_BIT) != 0;
      bool has_linear = (opt_flags & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) != 0;
      bool has_trans_src = (opt_flags & VK_FORMAT_FEATURE_TRANSFER_SRC_BIT) != 0;
      bool has_trans_dst = (opt_flags & VK_FORMAT_FEATURE_TRANSFER_DST_BIT) != 0;

      if (opt_flags != 0 || opt_flags2 != 0)
         supported++;

      printf("[%02zu] %s\n", i + 1, name);
      printf("     Legacy: linear=0x%08x optimal=0x%08x buffer=0x%08x\n",
             fp.linearTilingFeatures, fp.optimalTilingFeatures, fp.bufferFeatures);
      printf("     Properties2: linear=0x%08x optimal=0x%08x buffer=0x%08x\n",
             fp2.formatProperties.linearTilingFeatures,
             fp2.formatProperties.optimalTilingFeatures,
             fp2.formatProperties.bufferFeatures);
      printf("     Properties3: linear=0x%016llx optimal=0x%016llx buffer=0x%016llx\n",
             (unsigned long long)fp3.linearTilingFeatures,
             (unsigned long long)fp3.optimalTilingFeatures,
             (unsigned long long)fp3.bufferFeatures);
      printf("     SAMPLED=%u BLIT_SRC=%u FILTER_LINEAR=%u TRANSFER_SRC=%u TRANSFER_DST=%u\n",
             has_sampled ? 1 : 0, has_blit_src ? 1 : 0, has_linear ? 1 : 0,
             has_trans_src ? 1 : 0, has_trans_dst ? 1 : 0);

      if (gifp2) {
         VkPhysicalDeviceImageFormatInfo2 img_info = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2,
            .format = fmt,
            .type = VK_IMAGE_TYPE_2D,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT,
         };
         VkImageFormatProperties2 img_props = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2,
         };
         VkResult img_res = gifp2(phys, &img_info, &img_props);
         if (img_res == VK_SUCCESS) {
            printf("     Image2D(OPTIMAL, SAMPLED): OK extent=%ux%ux%u maxMip=%u maxArray=%u samples=0x%x\n",
                   img_props.imageFormatProperties.maxExtent.width,
                   img_props.imageFormatProperties.maxExtent.height,
                   img_props.imageFormatProperties.maxExtent.depth,
                   img_props.imageFormatProperties.maxMipLevels,
                   img_props.imageFormatProperties.maxArrayLayers,
                   img_props.imageFormatProperties.sampleCounts);
         } else {
            printf("     Image2D(OPTIMAL, SAMPLED): UNSUPPORTED (res=%d)\n", img_res);
         }
      }
   }
   if (out_supported)
      *out_supported = supported;
}

int main(int argc, char **argv)
{
   const char *so_path = argc > 1 ? argv[1] : "./libvulkan_panfrost.so";
   void *h = dlopen(so_path, RTLD_NOW | RTLD_LOCAL);
   if (!h) {
      fprintf(stderr, "FAIL: dlopen(%s): %s\n", so_path, dlerror());
      return 1;
   }

   PFN_vkGetInstanceProcAddr gipa =
      (PFN_vkGetInstanceProcAddr)dlsym(h, "vk_icdGetInstanceProcAddr");
   if (!gipa)
      gipa = (PFN_vkGetInstanceProcAddr)dlsym(h, "vkGetInstanceProcAddr");
   if (!gipa) {
      fprintf(stderr, "FAIL: cannot find vkGetInstanceProcAddr\n");
      dlclose(h);
      return 1;
   }

   PFN_vkCreateInstance ci =
      (PFN_vkCreateInstance)gipa(NULL, "vkCreateInstance");
   if (!ci) {
      fprintf(stderr, "FAIL: cannot find vkCreateInstance\n");
      dlclose(h);
      return 1;
   }

   VkApplicationInfo app = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .apiVersion = VK_API_VERSION_1_3,
      .pApplicationName = "bc-format-probe",
   };
   VkInstanceCreateInfo ici = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &app,
   };
   VkInstance inst = VK_NULL_HANDLE;
   VkResult res = ci(&ici, NULL, &inst);
   if (res != VK_SUCCESS || inst == VK_NULL_HANDLE) {
      fprintf(stderr, "FAIL: vkCreateInstance returned %d\n", res);
      dlclose(h);
      return 1;
   }

   PFN_vkEnumeratePhysicalDevices epd =
      (PFN_vkEnumeratePhysicalDevices)gipa(inst, "vkEnumeratePhysicalDevices");
   uint32_t phys_cnt = 1;
   VkPhysicalDevice phys = VK_NULL_HANDLE;
   res = epd(inst, &phys_cnt, &phys);
   if (res != VK_SUCCESS || phys_cnt == 0) {
      fprintf(stderr, "FAIL: vkEnumeratePhysicalDevices: res=%d count=%u\n", res, phys_cnt);
      dlclose(h);
      return 1;
   }

   PFN_vkGetPhysicalDeviceProperties gpd =
      (PFN_vkGetPhysicalDeviceProperties)gipa(inst, "vkGetPhysicalDeviceProperties");
   VkPhysicalDeviceProperties pprops;
   gpd(phys, &pprops);
   printf("DEVICE: %s (0x%08x)\n", pprops.deviceName, pprops.deviceID);

   PFN_vkGetPhysicalDeviceFeatures gpf =
      (PFN_vkGetPhysicalDeviceFeatures)gipa(inst, "vkGetPhysicalDeviceFeatures");
   VkPhysicalDeviceFeatures feats;
   gpf(phys, &feats);
   printf("FEATURE: textureCompressionBC = %s\n", feats.textureCompressionBC ? "YES" : "NO");
   printf("FEATURE: textureCompressionETC2 = %s\n", feats.textureCompressionETC2 ? "YES" : "NO");
   printf("FEATURE: textureCompressionASTC_LDR = %s\n", feats.textureCompressionASTC_LDR ? "YES" : "NO");

   PFN_vkGetPhysicalDeviceFormatProperties gfp =
      (PFN_vkGetPhysicalDeviceFormatProperties)gipa(inst, "vkGetPhysicalDeviceFormatProperties");
   PFN_vkGetPhysicalDeviceFormatProperties2 gfp2 =
      (PFN_vkGetPhysicalDeviceFormatProperties2)gipa(inst, "vkGetPhysicalDeviceFormatProperties2");
   PFN_vkGetPhysicalDeviceImageFormatProperties2 gifp2 =
      (PFN_vkGetPhysicalDeviceImageFormatProperties2)gipa(inst, "vkGetPhysicalDeviceImageFormatProperties2");

   printf("\n=== BC FORMAT PROBE (16 FORMATS) ===\n");
   unsigned bc_supported = 0;
   probe_format_list(phys, gfp, gfp2, gifp2, bc_formats,
                     sizeof(bc_formats) / sizeof(bc_formats[0]), &bc_supported);

   printf("\n=== CONTROL FORMATS (ETC2 & ASTC SANITY CHECK) ===\n");
   unsigned ctrl_supported = 0;
   probe_format_list(phys, gfp, gfp2, gifp2, control_formats,
                     sizeof(control_formats) / sizeof(control_formats[0]), &ctrl_supported);

   printf("\n=== SUMMARY ===\n");
   printf("Total BC formats tested: %zu\n", sizeof(bc_formats) / sizeof(bc_formats[0]));
   printf("BC formats with any optimal/linear features: %u\n", bc_supported);
   printf("Native BC support complete: %s\n", bc_supported == 16 ? "YES" : "NO");

   PFN_vkDestroyInstance di = (PFN_vkDestroyInstance)gipa(inst, "vkDestroyInstance");
   if (di) di(inst, NULL);
   dlclose(h);
   return 0;
}
