#define VK_USE_PLATFORM_ANDROID_KHR 1
#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/hardware_buffer.h>
#include <android/log.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <vulkan/vulkan.h>

#define TAG "PanVKLoaderApp"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

typedef PFN_vkVoidFunction (*icd_gipa_fn)(VkInstance, const char *);

static const uint32_t comp_spv[] = {
#include "../../compute/write_magic.spv.h"
};
#include "../../offscreen/tri.vert.spv.h"
#include "../../offscreen/tri.frag.spv.h"

static int extract_file(const char *src, const char *dst) {
   int in = open(src, O_RDONLY);
   if (in < 0) return -1;
   int out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0755);
   if (out < 0) { close(in); return -1; }
   char buf[65536];
   ssize_t n;
   while ((n = read(in, buf, sizeof(buf))) > 0) {
      if (write(out, buf, n) != n) { close(in); close(out); return -1; }
   }
   close(in);
   close(out);
   return 0;
}

JNIEXPORT jstring JNICALL
Java_org_panvk_loadertest_MainActivity_runPanvkTest(JNIEnv *env, jobject thiz, jobject surface, jstring jAppDir, jstring jZipPath) {
   const char *appDir = (*env)->GetStringUTFChars(env, jAppDir, NULL);
   const char *zipPath = (*env)->GetStringUTFChars(env, jZipPath, NULL);
    char report[8192];
   snprintf(report, sizeof(report), "Starting tests...\n");

   LOGI("PID_SELF=%d COLD_LAUNCH_PROCESS=start uid=%d appDir=%s zipPath=%s",
        getpid(), getuid(), appDir, zipPath);

   char so_path[512];
   snprintf(so_path, sizeof(so_path), "%s/libvulkan_panfrost.so", appDir);

    /* Prefer the ICD just pushed to /data/local/tmp; zip is a fallback. */
    if (access("/data/local/tmp/libvulkan_panfrost.so", R_OK) == 0) {
       char cmd[1024];
       snprintf(cmd, sizeof(cmd), "cp /data/local/tmp/libvulkan_panfrost.so '%s'", so_path);
       system(cmd);
       chmod(so_path, 0755);
    } else if (access(so_path, R_OK) != 0) {
       char cmd[1024];
       snprintf(cmd, sizeof(cmd), "unzip -p '%s' libvulkan_panfrost.so > '%s' 2>/dev/null", zipPath, so_path);
       system(cmd);
       chmod(so_path, 0755);
    }

   if (access(so_path, R_OK) != 0) {
      LOGE("Failed to find driver at %s", so_path);
      return (*env)->NewStringUTF(env, "FAIL: driver not found");
   }

   const int use_system_loader = access("/data/local/tmp/panvk-use-system-loader", R_OK) == 0;
   const char *loader_path = use_system_loader ? "libvulkan.so" : so_path;
   LOGI("Loading driver from %s", loader_path);
   void *h = dlopen(loader_path, RTLD_NOW | RTLD_LOCAL);
   if (!h) {
      LOGE("FAIL dlopen: %s", dlerror());
      return (*env)->NewStringUTF(env, "FAIL dlopen");
   }

   icd_gipa_fn gipa = use_system_loader ? NULL : (icd_gipa_fn)dlsym(h, "vk_icdGetInstanceProcAddr");
   if (!gipa) gipa = (icd_gipa_fn)dlsym(h, "vkGetInstanceProcAddr");
   if (!gipa) {
      LOGE("FAIL gipa");
      return (*env)->NewStringUTF(env, "FAIL missing gipa");
   }

   /* Gate C: loader negotiation and physical device enum */
   PFN_vkCreateInstance ci = (PFN_vkCreateInstance)gipa(NULL, "vkCreateInstance");
   VkApplicationInfo app_info = {.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO, .apiVersion = VK_API_VERSION_1_3};
    const char *inst_exts[] = {
       VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
       VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
       VK_KHR_SURFACE_EXTENSION_NAME,
       VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
       VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
       VK_KHR_SURFACE_MAINTENANCE_1_EXTENSION_NAME,
    };
    const char *layers[] = {"VK_LAYER_BCN_BCnLayer"};
    if (use_system_loader) {
       PFN_vkEnumerateInstanceLayerProperties eilp =
          (PFN_vkEnumerateInstanceLayerProperties)gipa(NULL, "vkEnumerateInstanceLayerProperties");
       uint32_t layer_count = 0;
       VkLayerProperties layer_props[16];
       VkResult layer_res = eilp ? eilp(&layer_count, NULL) : VK_ERROR_INITIALIZATION_FAILED;
       if (layer_res == VK_SUCCESS && layer_count <= 16)
          layer_res = eilp(&layer_count, layer_props);
       LOGI("BCN_LOADER layer_count=%u result=%d", layer_count, layer_res);
       for (uint32_t i = 0; layer_res == VK_SUCCESS && i < layer_count; i++)
          LOGI("BCN_LOADER layer[%u]=%s", i, layer_props[i].layerName);
    }
    VkInstanceCreateInfo ici = {.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, .pApplicationInfo = &app_info, .enabledLayerCount = use_system_loader ? 1 : 0, .ppEnabledLayerNames = use_system_loader ? layers : NULL, .enabledExtensionCount = 6, .ppEnabledExtensionNames = inst_exts};
   VkInstance inst;
   if (ci(&ici, NULL, &inst) != VK_SUCCESS) {
      LOGE("FAIL vkCreateInstance");
      return (*env)->NewStringUTF(env, "FAIL vkCreateInstance");
   }

   PFN_vkEnumeratePhysicalDevices epd = (PFN_vkEnumeratePhysicalDevices)gipa(inst, "vkEnumeratePhysicalDevices");
   uint32_t dev_cnt = 0;
   epd(inst, &dev_cnt, NULL);
   VkPhysicalDevice *devs = malloc(dev_cnt * sizeof(*devs));
   epd(inst, &dev_cnt, devs);
   VkPhysicalDevice phys = VK_NULL_HANDLE;
   PFN_vkGetPhysicalDeviceProperties gpd = (PFN_vkGetPhysicalDeviceProperties)gipa(inst, "vkGetPhysicalDeviceProperties");
   for (uint32_t i = 0; i < dev_cnt; i++) {
      VkPhysicalDeviceProperties p;
      gpd(devs[i], &p);
      LOGI("Device %d: %s (vendor=0x%x api=0x%x)", i, p.deviceName, p.vendorID, p.apiVersion);
      if (strstr(p.deviceName, "Mali")) phys = devs[i];
   }
   free(devs);
    if (phys == VK_NULL_HANDLE) return (*env)->NewStringUTF(env, "FAIL no Mali physical device");
    LOGI("Gate C: PASS");

    if (use_system_loader) {
       static const VkFormat bc_formats[] = {
          VK_FORMAT_BC1_RGB_UNORM_BLOCK, VK_FORMAT_BC1_RGB_SRGB_BLOCK,
          VK_FORMAT_BC1_RGBA_UNORM_BLOCK, VK_FORMAT_BC1_RGBA_SRGB_BLOCK,
          VK_FORMAT_BC2_UNORM_BLOCK, VK_FORMAT_BC2_SRGB_BLOCK,
          VK_FORMAT_BC3_UNORM_BLOCK, VK_FORMAT_BC3_SRGB_BLOCK,
          VK_FORMAT_BC4_UNORM_BLOCK, VK_FORMAT_BC4_SNORM_BLOCK,
          VK_FORMAT_BC5_UNORM_BLOCK, VK_FORMAT_BC5_SNORM_BLOCK,
          VK_FORMAT_BC6H_UFLOAT_BLOCK, VK_FORMAT_BC6H_SFLOAT_BLOCK,
          VK_FORMAT_BC7_UNORM_BLOCK, VK_FORMAT_BC7_SRGB_BLOCK,
       };
       PFN_vkGetPhysicalDeviceFormatProperties gpdfp =
          (PFN_vkGetPhysicalDeviceFormatProperties)gipa(inst, "vkGetPhysicalDeviceFormatProperties");
       PFN_vkGetPhysicalDeviceFeatures gpdf =
          (PFN_vkGetPhysicalDeviceFeatures)gipa(inst, "vkGetPhysicalDeviceFeatures");
       VkPhysicalDeviceFeatures features;
       gpdf(phys, &features);
       LOGI("BCN_FEATURE textureCompressionBC=%u", features.textureCompressionBC);
       for (uint32_t i = 0; i < sizeof(bc_formats) / sizeof(bc_formats[0]); i++) {
          VkFormatProperties fp;
          gpdfp(phys, bc_formats[i], &fp);
          LOGI("BCN_FORMAT format=%d linear=0x%x optimal=0x%x buffer=0x%x", bc_formats[i],
               fp.linearTilingFeatures, fp.optimalTilingFeatures, fp.bufferFeatures);
       }
    }

   /* Gate D: device create */
   PFN_vkGetPhysicalDeviceQueueFamilyProperties gqfp = (PFN_vkGetPhysicalDeviceQueueFamilyProperties)gipa(inst, "vkGetPhysicalDeviceQueueFamilyProperties");
   uint32_t qcnt = 0;
   gqfp(phys, &qcnt, NULL);
   VkQueueFamilyProperties *qprops = malloc(qcnt * sizeof(*qprops));
   gqfp(phys, &qcnt, qprops);
   uint32_t qi = ~0u;
   for (uint32_t i = 0; i < qcnt; i++) {
      if ((qprops[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && (qprops[i].queueFlags & VK_QUEUE_COMPUTE_BIT)) {
         qi = i; break;
      }
   }
   free(qprops);
   if (qi == ~0u) qi = 0;

   float prio = 1.0f;
   VkDeviceQueueCreateInfo qci = {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, .queueFamilyIndex = qi, .queueCount = 1, .pQueuePriorities = &prio};
    const char *dev_exts[] = {
       VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
       VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
       VK_ANDROID_EXTERNAL_MEMORY_ANDROID_HARDWARE_BUFFER_EXTENSION_NAME,
       VK_KHR_BIND_MEMORY_2_EXTENSION_NAME,
       VK_KHR_MAINTENANCE1_EXTENSION_NAME,
       VK_EXT_QUEUE_FAMILY_FOREIGN_EXTENSION_NAME,
       VK_KHR_SWAPCHAIN_EXTENSION_NAME,
       VK_KHR_PRESENT_ID_2_EXTENSION_NAME,
       VK_KHR_PRESENT_WAIT_2_EXTENSION_NAME,
       VK_KHR_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME,
    };
    VkPhysicalDeviceSwapchainMaintenance1FeaturesKHR swapchain_maintenance = {
       .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_KHR,
       .swapchainMaintenance1 = VK_TRUE,
    };
    VkPhysicalDevicePresentWait2FeaturesKHR present_wait2 = {
       .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_WAIT_2_FEATURES_KHR,
       .pNext = &swapchain_maintenance, .presentWait2 = VK_TRUE,
    };
    VkPhysicalDevicePresentId2FeaturesKHR present_id2 = {
       .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_2_FEATURES_KHR,
       .pNext = &present_wait2, .presentId2 = VK_TRUE,
    };
    VkDeviceCreateInfo dci = {.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, .pNext = &present_id2, .queueCreateInfoCount = 1, .pQueueCreateInfos = &qci, .enabledExtensionCount = 10, .ppEnabledExtensionNames = dev_exts};
   PFN_vkCreateDevice cd = (PFN_vkCreateDevice)gipa(inst, "vkCreateDevice");
   VkDevice dev;
   if (cd(phys, &dci, NULL, &dev) != VK_SUCCESS) return (*env)->NewStringUTF(env, "FAIL vkCreateDevice");
   LOGI("Gate D: PASS");

   PFN_vkGetDeviceQueue gdq = (PFN_vkGetDeviceQueue)gipa(inst, "vkGetDeviceQueue");
   VkQueue queue;
   gdq(dev, qi, 0, &queue);

   /* Gate E: compute 10/10 */
   PFN_vkCreateBuffer cb_ = (PFN_vkCreateBuffer)gipa(inst, "vkCreateBuffer");
   PFN_vkGetBufferMemoryRequirements gbmr = (PFN_vkGetBufferMemoryRequirements)gipa(inst, "vkGetBufferMemoryRequirements");
   PFN_vkAllocateMemory am = (PFN_vkAllocateMemory)gipa(inst, "vkAllocateMemory");
   PFN_vkBindBufferMemory bbm = (PFN_vkBindBufferMemory)gipa(inst, "vkBindBufferMemory");
   PFN_vkMapMemory mm = (PFN_vkMapMemory)gipa(inst, "vkMapMemory");
   PFN_vkUnmapMemory um = (PFN_vkUnmapMemory)gipa(inst, "vkUnmapMemory");
   PFN_vkCreateShaderModule csm = (PFN_vkCreateShaderModule)gipa(inst, "vkCreateShaderModule");
   PFN_vkCreateDescriptorSetLayout cdsl = (PFN_vkCreateDescriptorSetLayout)gipa(inst, "vkCreateDescriptorSetLayout");
   PFN_vkCreatePipelineLayout cpl = (PFN_vkCreatePipelineLayout)gipa(inst, "vkCreatePipelineLayout");
   PFN_vkCreateComputePipelines ccp = (PFN_vkCreateComputePipelines)gipa(inst, "vkCreateComputePipelines");
   PFN_vkCreateDescriptorPool cdp = (PFN_vkCreateDescriptorPool)gipa(inst, "vkCreateDescriptorPool");
   PFN_vkAllocateDescriptorSets ads = (PFN_vkAllocateDescriptorSets)gipa(inst, "vkAllocateDescriptorSets");
   PFN_vkUpdateDescriptorSets uds = (PFN_vkUpdateDescriptorSets)gipa(inst, "vkUpdateDescriptorSets");
   PFN_vkCreateCommandPool ccp_ = (PFN_vkCreateCommandPool)gipa(inst, "vkCreateCommandPool");
   PFN_vkAllocateCommandBuffers acb = (PFN_vkAllocateCommandBuffers)gipa(inst, "vkAllocateCommandBuffers");
   PFN_vkBeginCommandBuffer bcb = (PFN_vkBeginCommandBuffer)gipa(inst, "vkBeginCommandBuffer");
   PFN_vkCmdBindPipeline cbp = (PFN_vkCmdBindPipeline)gipa(inst, "vkCmdBindPipeline");
   PFN_vkCmdBindDescriptorSets cbds = (PFN_vkCmdBindDescriptorSets)gipa(inst, "vkCmdBindDescriptorSets");
   PFN_vkCmdDispatch cd_ = (PFN_vkCmdDispatch)gipa(inst, "vkCmdDispatch");
   PFN_vkEndCommandBuffer ecb = (PFN_vkEndCommandBuffer)gipa(inst, "vkEndCommandBuffer");
    PFN_vkCreateFence cf = (PFN_vkCreateFence)gipa(inst, "vkCreateFence");
    PFN_vkQueueSubmit qs = (PFN_vkQueueSubmit)gipa(inst, "vkQueueSubmit");
    PFN_vkWaitForFences wff = (PFN_vkWaitForFences)gipa(inst, "vkWaitForFences");
    PFN_vkResetFences rf = (PFN_vkResetFences)gipa(inst, "vkResetFences");
    PFN_vkCreateSemaphore csem = (PFN_vkCreateSemaphore)gipa(inst, "vkCreateSemaphore");
    PFN_vkDestroySemaphore dsem = (PFN_vkDestroySemaphore)gipa(inst, "vkDestroySemaphore");
    PFN_vkResetCommandBuffer rcb = (PFN_vkResetCommandBuffer)gipa(inst, "vkResetCommandBuffer");
    PFN_vkDeviceWaitIdle dwi = (PFN_vkDeviceWaitIdle)gipa(inst, "vkDeviceWaitIdle");

   VkBufferCreateInfo bci = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = 4096, .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT};
   VkBuffer cbuf;
   cb_(dev, &bci, NULL, &cbuf);
   VkMemoryRequirements cmr;
   gbmr(dev, cbuf, &cmr);
   PFN_vkGetPhysicalDeviceMemoryProperties gpmp = (PFN_vkGetPhysicalDeviceMemoryProperties)gipa(inst, "vkGetPhysicalDeviceMemoryProperties");
   VkPhysicalDeviceMemoryProperties mp;
   gpmp(phys, &mp);
   uint32_t host_mi = 0;
   for (uint32_t i = 0; i < mp.memoryTypeCount; i++) {
      if ((cmr.memoryTypeBits & (1u << i)) && (mp.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
         host_mi = i; break;
      }
   }
   VkMemoryAllocateInfo cmai = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, .allocationSize = cmr.size, .memoryTypeIndex = host_mi};
   VkDeviceMemory cmem;
   am(dev, &cmai, NULL, &cmem);
   bbm(dev, cbuf, cmem, 0);

   VkDescriptorSetLayoutBinding c_bind = {.binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT};
   VkDescriptorSetLayoutCreateInfo c_dlci = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, .bindingCount = 1, .pBindings = &c_bind};
   VkDescriptorSetLayout c_dsl;
   cdsl(dev, &c_dlci, NULL, &c_dsl);
   VkDescriptorPoolSize c_psz = {.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1};
   VkDescriptorPoolCreateInfo c_dpci = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, .maxSets = 1, .poolSizeCount = 1, .pPoolSizes = &c_psz};
   VkDescriptorPool c_pool;
   cdp(dev, &c_dpci, NULL, &c_pool);
   VkDescriptorSetAllocateInfo c_dsai = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, .descriptorPool = c_pool, .descriptorSetCount = 1, .pSetLayouts = &c_dsl};
   VkDescriptorSet c_set;
   ads(dev, &c_dsai, &c_set);
   VkDescriptorBufferInfo c_dbi = {.buffer = cbuf, .offset = 0, .range = 4};
   VkWriteDescriptorSet c_wds = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = c_set, .dstBinding = 0, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .pBufferInfo = &c_dbi};
   uds(dev, 1, &c_wds, 0, NULL);
   VkShaderModuleCreateInfo c_smci = {.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, .codeSize = sizeof(comp_spv), .pCode = comp_spv};
   VkShaderModule c_mod;
   csm(dev, &c_smci, NULL, &c_mod);
   VkPipelineLayoutCreateInfo c_plci = {.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, .setLayoutCount = 1, .pSetLayouts = &c_dsl};
   VkPipelineLayout c_layout;
   cpl(dev, &c_plci, NULL, &c_layout);
   VkComputePipelineCreateInfo c_cpci = {.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO, .stage = {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_COMPUTE_BIT, .module = c_mod, .pName = "main"}, .layout = c_layout};
   VkPipeline c_pipe;
   ccp(dev, VK_NULL_HANDLE, 1, &c_cpci, NULL, &c_pipe);

   VkCommandPoolCreateInfo c_cpoci = {.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, .queueFamilyIndex = qi, .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT};
   VkCommandPool c_cpool;
   ccp_(dev, &c_cpoci, NULL, &c_cpool);
   VkCommandBufferAllocateInfo c_cbai = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, .commandPool = c_cpool, .commandBufferCount = 1};
   VkCommandBuffer c_cmd;
   acb(dev, &c_cbai, &c_cmd);
   VkCommandBufferBeginInfo c_bbi = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
   bcb(c_cmd, &c_bbi);
   cbp(c_cmd, VK_PIPELINE_BIND_POINT_COMPUTE, c_pipe);
   cbds(c_cmd, VK_PIPELINE_BIND_POINT_COMPUTE, c_layout, 0, 1, &c_set, 0, NULL);
   cd_(c_cmd, 1, 1, 1);
   ecb(c_cmd);

   VkFenceCreateInfo fci = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
   VkFence fence;
   cf(dev, &fci, NULL, &fence);

   for (int iter = 0; iter < 10; iter++) {
      uint32_t *p;
      mm(dev, cmem, 0, 4, 0, (void **)&p);
      *p = 0;
      um(dev, cmem);
      rf(dev, 1, &fence);
      VkSubmitInfo si = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = &c_cmd};
      qs(queue, 1, &si, fence);
      wff(dev, 1, &fence, VK_TRUE, 5000000000ULL);
      mm(dev, cmem, 0, 4, 0, (void **)&p);
      if (*p != 0x12345678) {
         LOGE("Compute iter %d failed: 0x%x", iter, *p);
         return (*env)->NewStringUTF(env, "FAIL compute 10/10");
      }
      um(dev, cmem);
   }
   LOGI("Gate E: PASS (10/10 exact magic 0x12345678)");

   /* Gate G: AHardwareBuffer allocate, import, GPU render, CPU readback */
   AHardwareBuffer_Desc ahb_desc = {.width = 64, .height = 64, .layers = 1, .format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM, .usage = AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT | AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN};
   AHardwareBuffer *ahb = NULL;
   AHardwareBuffer_allocate(&ahb_desc, &ahb);
   AHardwareBuffer_Desc ahb_out;
   AHardwareBuffer_describe(ahb, &ahb_out);

   PFN_vkGetAndroidHardwareBufferPropertiesANDROID gahbp = (PFN_vkGetAndroidHardwareBufferPropertiesANDROID)gipa(inst, "vkGetAndroidHardwareBufferPropertiesANDROID");
   VkAndroidHardwareBufferPropertiesANDROID ahbp = {.sType = VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_PROPERTIES_ANDROID};
   gahbp(dev, ahb, &ahbp);

   VkExternalMemoryImageCreateInfo emici = {.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO, .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID};
   VkImageCreateInfo ici2 = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, .pNext = &emici, .imageType = VK_IMAGE_TYPE_2D, .format = VK_FORMAT_R8G8B8A8_UNORM, .extent = {64, 64, 1}, .mipLevels = 1, .arrayLayers = 1, .samples = 1, .tiling = VK_IMAGE_TILING_OPTIMAL, .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, .sharingMode = VK_SHARING_MODE_EXCLUSIVE, .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};
   PFN_vkCreateImage ci_ = (PFN_vkCreateImage)gipa(inst, "vkCreateImage");
   VkImage a_img;
   ci_(dev, &ici2, NULL, &a_img);

   VkImportAndroidHardwareBufferInfoANDROID a_imp = {.sType = VK_STRUCTURE_TYPE_IMPORT_ANDROID_HARDWARE_BUFFER_INFO_ANDROID, .buffer = ahb};
   VkMemoryDedicatedAllocateInfo a_dai = {.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO, .pNext = &a_imp, .image = a_img};
   uint32_t a_mi = 0;
   for (uint32_t i = 0; i < 32; i++) {
      if (ahbp.memoryTypeBits & (1u << i)) { a_mi = i; break; }
   }
   VkMemoryAllocateInfo a_mai = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, .pNext = &a_dai, .allocationSize = ahbp.allocationSize, .memoryTypeIndex = a_mi};
   VkDeviceMemory a_mem;
   am(dev, &a_mai, NULL, &a_mem);

   PFN_vkBindImageMemory2 bim2 = (PFN_vkBindImageMemory2)gipa(inst, "vkBindImageMemory2");
   VkBindImageMemoryInfo a_bmi = {.sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO, .image = a_img, .memory = a_mem, .memoryOffset = 0};
   bim2(dev, 1, &a_bmi);

   PFN_vkCreateImageView civ = (PFN_vkCreateImageView)gipa(inst, "vkCreateImageView");
   VkImageViewCreateInfo a_ivci = {.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, .image = a_img, .viewType = VK_IMAGE_VIEW_TYPE_2D, .format = VK_FORMAT_R8G8B8A8_UNORM, .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1}};
   VkImageView a_view;
   civ(dev, &a_ivci, NULL, &a_view);

   PFN_vkCreateRenderPass crp = (PFN_vkCreateRenderPass)gipa(inst, "vkCreateRenderPass");
   VkAttachmentDescription att = {.format = VK_FORMAT_R8G8B8A8_UNORM, .samples = 1, .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, .storeOp = VK_ATTACHMENT_STORE_OP_STORE, .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
   VkAttachmentReference ref = {.attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
   VkSubpassDescription sp = {.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS, .colorAttachmentCount = 1, .pColorAttachments = &ref};
   VkRenderPassCreateInfo rpci = {.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, .attachmentCount = 1, .pAttachments = &att, .subpassCount = 1, .pSubpasses = &sp};
   VkRenderPass a_rp;
   crp(dev, &rpci, NULL, &a_rp);

   PFN_vkCreateFramebuffer cfb = (PFN_vkCreateFramebuffer)gipa(inst, "vkCreateFramebuffer");
   VkFramebufferCreateInfo fbci = {.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, .renderPass = a_rp, .attachmentCount = 1, .pAttachments = &a_view, .width = 64, .height = 64, .layers = 1};
   VkFramebuffer a_fb;
   cfb(dev, &fbci, NULL, &a_fb);

   float verts[] = {-1.0f, -1.0f, -0.25f, -1.0f, -1.0f, -0.25f};
   VkBufferCreateInfo vbci = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = sizeof(verts), .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT};
   VkBuffer vbuf;
   cb_(dev, &vbci, NULL, &vbuf);
   VkMemoryRequirements vmr;
   gbmr(dev, vbuf, &vmr);
   VkMemoryAllocateInfo vmai = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, .allocationSize = vmr.size, .memoryTypeIndex = host_mi};
   VkDeviceMemory vmem;
   am(dev, &vmai, NULL, &vmem);
   bbm(dev, vbuf, vmem, 0);
   void *vp;
   mm(dev, vmem, 0, sizeof(verts), 0, &vp);
   memcpy(vp, verts, sizeof(verts));
   um(dev, vmem);

   VkShaderModuleCreateInfo a_vsmci = {.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, .codeSize = sizeof(tri_vert_spv), .pCode = tri_vert_spv};
   VkShaderModule a_vsm;
   csm(dev, &a_vsmci, NULL, &a_vsm);
   VkShaderModuleCreateInfo a_fsmci = {.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, .codeSize = sizeof(tri_frag_spv), .pCode = tri_frag_spv};
   VkShaderModule a_fsm;
   csm(dev, &a_fsmci, NULL, &a_fsm);

   VkPipelineShaderStageCreateInfo a_stages[2] = {
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_VERTEX_BIT, .module = a_vsm, .pName = "main"},
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_FRAGMENT_BIT, .module = a_fsm, .pName = "main"},
   };
   VkVertexInputBindingDescription vibd = {.binding = 0, .stride = 2 * sizeof(float), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};
   VkVertexInputAttributeDescription viad = {.location = 0, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = 0};
   VkPipelineVertexInputStateCreateInfo pvisi = {.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, .vertexBindingDescriptionCount = 1, .pVertexBindingDescriptions = &vibd, .vertexAttributeDescriptionCount = 1, .pVertexAttributeDescriptions = &viad};
   VkPipelineInputAssemblyStateCreateInfo piasi = {.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
   VkViewport vp_ = {.x = 0, .y = 0, .width = 64, .height = 64, .minDepth = 0.0f, .maxDepth = 1.0f};
   VkRect2D sc_ = {.offset = {0, 0}, .extent = {64, 64}};
   VkPipelineViewportStateCreateInfo pvsi = {.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, .viewportCount = 1, .pViewports = &vp_, .scissorCount = 1, .pScissors = &sc_};
   VkPipelineRasterizationStateCreateInfo prsi = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, .polygonMode = VK_POLYGON_MODE_FILL, .cullMode = VK_CULL_MODE_NONE, .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE, .lineWidth = 1.0f};
   VkPipelineMultisampleStateCreateInfo pmsi = {.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT};
   VkPipelineColorBlendAttachmentState pcbas = {.blendEnable = VK_FALSE, .colorWriteMask = 0xf};
   VkPipelineColorBlendStateCreateInfo pcbsi = {.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, .attachmentCount = 1, .pAttachments = &pcbas};
   VkPipelineLayoutCreateInfo a_plci = {.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
   VkPipelineLayout a_layout;
   cpl(dev, &a_plci, NULL, &a_layout);

   PFN_vkCreateGraphicsPipelines cgp = (PFN_vkCreateGraphicsPipelines)gipa(inst, "vkCreateGraphicsPipelines");
   VkGraphicsPipelineCreateInfo a_gpci = {.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, .stageCount = 2, .pStages = a_stages, .pVertexInputState = &pvisi, .pInputAssemblyState = &piasi, .pViewportState = &pvsi, .pRasterizationState = &prsi, .pMultisampleState = &pmsi, .pColorBlendState = &pcbsi, .layout = a_layout, .renderPass = a_rp, .subpass = 0};
   VkPipeline a_pipe;
   cgp(dev, VK_NULL_HANDLE, 1, &a_gpci, NULL, &a_pipe);

   VkCommandBuffer a_cmd;
   acb(dev, &c_cbai, &a_cmd);
   bcb(a_cmd, &c_bbi);
   VkClearValue clear = {.color = {{0.0f, 0.0f, 1.0f, 1.0f}}};
   VkRenderPassBeginInfo rpbi = {.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, .renderPass = a_rp, .framebuffer = a_fb, .renderArea = {{0, 0}, {64, 64}}, .clearValueCount = 1, .pClearValues = &clear};
   PFN_vkCmdBeginRenderPass cbrp = (PFN_vkCmdBeginRenderPass)gipa(inst, "vkCmdBeginRenderPass");
   PFN_vkCmdBindVertexBuffers cbvb = (PFN_vkCmdBindVertexBuffers)gipa(inst, "vkCmdBindVertexBuffers");
   PFN_vkCmdDraw cdraw = (PFN_vkCmdDraw)gipa(inst, "vkCmdDraw");
   PFN_vkCmdEndRenderPass cerp = (PFN_vkCmdEndRenderPass)gipa(inst, "vkCmdEndRenderPass");

   cbrp(a_cmd, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
   cbp(a_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, a_pipe);
   VkDeviceSize voff = 0;
   cbvb(a_cmd, 0, 1, &vbuf, &voff);
   cdraw(a_cmd, 3, 1, 0, 0);
   cerp(a_cmd);
   ecb(a_cmd);

   rf(dev, 1, &fence);
   VkSubmitInfo a_si = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = &a_cmd};
   qs(queue, 1, &a_si, fence);
   wff(dev, 1, &fence, VK_TRUE, 5000000000ULL);

   void *a_data = NULL;
   AHardwareBuffer_lock(ahb, AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN, -1, NULL, &a_data);
   uint8_t *a_px = (uint8_t *)a_data;
   uint8_t *in_p = a_px + (8 * ahb_out.stride + 8) * 4;
   uint8_t *cl_p = a_px + (55 * ahb_out.stride + 55) * 4;
   LOGI("Gate G: interior RGBA=%u %u %u %u, clear RGBA=%u %u %u %u", in_p[0], in_p[1], in_p[2], in_p[3], cl_p[0], cl_p[1], cl_p[2], cl_p[3]);
   AHardwareBuffer_unlock(ahb, NULL);
   if (in_p[0] < 200 || cl_p[2] < 200) return (*env)->NewStringUTF(env, "FAIL AHB pixel check");
   LOGI("Gate G: PASS");

     /* Gate H / P6-P8: real Vulkan Android WSI, GPU render, 300 frames */
     ANativeWindow *win = NULL;
     for (int t = 0; t < 50 && !win; t++) {
        win = ANativeWindow_fromSurface(env, surface);
        if (win && ANativeWindow_getWidth(win) > 0 && ANativeWindow_getHeight(win) > 0)
           break;
        if (win) {
           ANativeWindow_release(win);
           win = NULL;
        }
        usleep(100000);
     }
     if (!win) return (*env)->NewStringUTF(env, "FAIL ANativeWindow_fromSurface");

    PFN_vkCreateAndroidSurfaceKHR cas = (PFN_vkCreateAndroidSurfaceKHR)gipa(inst, "vkCreateAndroidSurfaceKHR");
    PFN_vkDestroySurfaceKHR dsurf = (PFN_vkDestroySurfaceKHR)gipa(inst, "vkDestroySurfaceKHR");
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR gpss = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)gipa(inst, "vkGetPhysicalDeviceSurfaceSupportKHR");
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR gpsc = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)gipa(inst, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR gpsf = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)gipa(inst, "vkGetPhysicalDeviceSurfaceFormatsKHR");
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR gpspm = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)gipa(inst, "vkGetPhysicalDeviceSurfacePresentModesKHR");
    PFN_vkCreateSwapchainKHR csc = (PFN_vkCreateSwapchainKHR)gipa(inst, "vkCreateSwapchainKHR");
    PFN_vkDestroySwapchainKHR dsc = (PFN_vkDestroySwapchainKHR)gipa(inst, "vkDestroySwapchainKHR");
    PFN_vkGetSwapchainImagesKHR gsi = (PFN_vkGetSwapchainImagesKHR)gipa(inst, "vkGetSwapchainImagesKHR");
    PFN_vkAcquireNextImageKHR ani = (PFN_vkAcquireNextImageKHR)gipa(inst, "vkAcquireNextImageKHR");
    PFN_vkQueuePresentKHR qp = (PFN_vkQueuePresentKHR)gipa(inst, "vkQueuePresentKHR");
    PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR gpsc2 = (PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR)gipa(inst, "vkGetPhysicalDeviceSurfaceCapabilities2KHR");
    PFN_vkWaitForPresent2KHR wait_present2 = (PFN_vkWaitForPresent2KHR)gipa(inst, "vkWaitForPresent2KHR");
    PFN_vkReleaseSwapchainImagesKHR release_images = (PFN_vkReleaseSwapchainImagesKHR)gipa(inst, "vkReleaseSwapchainImagesKHR");
    LOGI("vkCreateAndroidSurfaceKHR=%p DestroySurface=%p CreateSwapchain=%p Acquire=%p Present=%p",
         (void *)cas, (void *)dsurf, (void *)csc, (void *)ani, (void *)qp);
    if (!cas) {
       LOGE("ANDROID_SURFACE=fail vkCreateAndroidSurfaceKHR=0x0 via ICD GetInstanceProcAddr");
       ANativeWindow_release(win);
       return (*env)->NewStringUTF(env, "FAIL ANDROID_SURFACE vkCreateAndroidSurfaceKHR=0");
    }

    VkAndroidSurfaceCreateInfoKHR asci = {
       .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
       .window = win,
    };
    VkSurfaceKHR vk_surf = VK_NULL_HANDLE;
    if (cas(inst, &asci, NULL, &vk_surf) != VK_SUCCESS || vk_surf == VK_NULL_HANDLE) {
       LOGE("ANDROID_SURFACE=fail vkCreateAndroidSurfaceKHR");
       ANativeWindow_release(win);
       return (*env)->NewStringUTF(env, "FAIL ANDROID_SURFACE create");
    }
    LOGI("ANDROID_SURFACE=pass");

    VkSurfacePresentModeKHR surface_mode = {.sType = VK_STRUCTURE_TYPE_SURFACE_PRESENT_MODE_KHR,
                                             .presentMode = VK_PRESENT_MODE_FIFO_KHR};
    VkPhysicalDeviceSurfaceInfo2KHR surface_info = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR,
                                                     .pNext = &surface_mode, .surface = vk_surf};
    VkSurfacePresentModeCompatibilityKHR compatibility = {.sType = VK_STRUCTURE_TYPE_SURFACE_PRESENT_MODE_COMPATIBILITY_KHR};
    VkSurfaceCapabilities2KHR caps2 = {.sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR,
                                       .pNext = &compatibility};
    VkResult surface_maintenance_result = gpsc2 ? gpsc2(phys, &surface_info, &caps2) : VK_ERROR_EXTENSION_NOT_PRESENT;
    LOGI("PHASE7 VK_KHR_surface_maintenance1 status=%s result=%d compatibleModes=%u",
         surface_maintenance_result == VK_SUCCESS && compatibility.presentModeCount ? "PASS" : "FAIL",
         surface_maintenance_result, compatibility.presentModeCount);

    VkBool32 present_support = VK_FALSE;
    if (!gpss || gpss(phys, qi, vk_surf, &present_support) != VK_SUCCESS || !present_support) {
       LOGE("PRESENT_QUEUE_SUPPORT=fail qf=%u supported=%u", qi, (unsigned)present_support);
       return (*env)->NewStringUTF(env, "FAIL PRESENT_QUEUE_SUPPORT");
    }
    LOGI("PRESENT_QUEUE_SUPPORT=pass qf=%u", qi);

    VkSurfaceCapabilitiesKHR scaps;
    if (!gpsc || gpsc(phys, vk_surf, &scaps) != VK_SUCCESS) {
       return (*env)->NewStringUTF(env, "FAIL SURFACE_CAPS");
    }
    uint32_t sw = scaps.currentExtent.width;
    uint32_t sh = scaps.currentExtent.height;
    if (sw == 0xffffffffu || sh == 0xffffffffu || sw == 0 || sh == 0) {
       int nw = ANativeWindow_getWidth(win);
       int nh = ANativeWindow_getHeight(win);
       sw = nw > 0 ? (uint32_t)nw : 64;
       sh = nh > 0 ? (uint32_t)nh : 64;
    }
    if (sw < scaps.minImageExtent.width) sw = scaps.minImageExtent.width;
    if (sh < scaps.minImageExtent.height) sh = scaps.minImageExtent.height;
    if (sw > scaps.maxImageExtent.width) sw = scaps.maxImageExtent.width;
    if (sh > scaps.maxImageExtent.height) sh = scaps.maxImageExtent.height;
    LOGI("SURFACE_CAPS minImages=%u maxImages=%u extent=%ux%u", scaps.minImageCount, scaps.maxImageCount, sw, sh);

    uint32_t fmt_count = 0;
    gpsf(phys, vk_surf, &fmt_count, NULL);
    if (fmt_count == 0) return (*env)->NewStringUTF(env, "FAIL SURFACE_FORMATS empty");
    VkSurfaceFormatKHR *sformats = malloc(fmt_count * sizeof(*sformats));
    gpsf(phys, vk_surf, &fmt_count, sformats);
    VkSurfaceFormatKHR chosen_fmt = sformats[0];
    for (uint32_t i = 0; i < fmt_count; i++) {
       if (sformats[i].format == VK_FORMAT_R8G8B8A8_UNORM) {
          chosen_fmt = sformats[i];
          break;
       }
    }
    LOGI("SURFACE_FORMATS=%u chosen=0x%x", fmt_count, chosen_fmt.format);
    free(sformats);

    uint32_t pm_count = 0;
    gpspm(phys, vk_surf, &pm_count, NULL);
    if (pm_count == 0) return (*env)->NewStringUTF(env, "FAIL PRESENT_MODES empty");
    VkPresentModeKHR *pmodes = malloc(pm_count * sizeof(*pmodes));
    gpspm(phys, vk_surf, &pm_count, pmodes);
    VkPresentModeKHR chosen_pm = pmodes[0];
    for (uint32_t i = 0; i < pm_count; i++) {
       if (pmodes[i] == VK_PRESENT_MODE_FIFO_KHR) {
          chosen_pm = pmodes[i];
          break;
       }
    }
    LOGI("PRESENT_MODES=%u chosen=%d", pm_count, (int)chosen_pm);
    free(pmodes);

    uint32_t img_count = scaps.minImageCount < 2 ? 2 : scaps.minImageCount;
    if (scaps.maxImageCount && img_count > scaps.maxImageCount) img_count = scaps.maxImageCount;
    VkSwapchainCreateInfoKHR scci = {
       .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
       .surface = vk_surf,
       .minImageCount = img_count,
       .imageFormat = chosen_fmt.format,
       .imageColorSpace = chosen_fmt.colorSpace,
       .imageExtent = {sw, sh},
       .imageArrayLayers = 1,
       .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
       .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
       .preTransform = scaps.currentTransform,
       .compositeAlpha = (scaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
                            ? VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR
                            : VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
       .presentMode = chosen_pm,
       .clipped = VK_TRUE,
       .flags = VK_SWAPCHAIN_CREATE_PRESENT_WAIT_2_BIT_KHR,
    };
    if (!csc) return (*env)->NewStringUTF(env, "FAIL CreateSwapchainKHR=0");
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    if (csc(dev, &scci, NULL, &swapchain) != VK_SUCCESS || swapchain == VK_NULL_HANDLE) {
       LOGE("SWAPCHAIN=fail");
       return (*env)->NewStringUTF(env, "FAIL SWAPCHAIN");
    }
    LOGI("SWAPCHAIN=pass");
    LOGI("PHASE7 WSI functions waitForPresent2=%p releaseSwapchainImages=%p", (void *)wait_present2, (void *)release_images);

    VkFence release_fence;
    VkFenceCreateInfo release_fci = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    cf(dev, &release_fci, NULL, &release_fence);
    uint32_t release_index = 0;
    LOGI("PHASE7 VK_KHR_swapchain_maintenance1 acquire-before-release start");
    VkResult release_acquire = ani(dev, swapchain, 5000000000ULL, VK_NULL_HANDLE, release_fence, &release_index);
    VkResult release_wait = wff(dev, 1, &release_fence, VK_TRUE, 5000000000ULL);
    VkReleaseSwapchainImagesInfoKHR release_info = {.sType = VK_STRUCTURE_TYPE_RELEASE_SWAPCHAIN_IMAGES_INFO_KHR,
       .swapchain = swapchain, .imageIndexCount = 1, .pImageIndices = &release_index};
    VkResult release_result = release_images && release_acquire == VK_SUCCESS && release_wait == VK_SUCCESS
       ? release_images(dev, &release_info) : VK_ERROR_EXTENSION_NOT_PRESENT;
    LOGI("PHASE7 VK_KHR_swapchain_maintenance1 status=%s acquire=%d wait=%d release=%d",
         release_result == VK_SUCCESS ? "PASS" : "FAIL", release_acquire, release_wait, release_result);
    PFN_vkDestroyFence destroy_release_fence = (PFN_vkDestroyFence)gipa(inst, "vkDestroyFence");
    destroy_release_fence(dev, release_fence, NULL);

    gsi(dev, swapchain, &img_count, NULL);
    VkImage *sc_imgs = malloc(img_count * sizeof(*sc_imgs));
    gsi(dev, swapchain, &img_count, sc_imgs);
    LOGI("SWAPCHAIN_IMAGES=%u", img_count);

    VkImageView *sc_views = malloc(img_count * sizeof(*sc_views));
    VkFramebuffer *sc_fbs = malloc(img_count * sizeof(*sc_fbs));
    VkAttachmentDescription wsi_att = {
       .format = chosen_fmt.format, .samples = 1,
       .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
       .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE, .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
       .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };
    VkAttachmentReference wsi_ref = {.attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription wsi_sp = {.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS, .colorAttachmentCount = 1, .pColorAttachments = &wsi_ref};
    VkSubpassDependency wsi_dep = {
       .srcSubpass = VK_SUBPASS_EXTERNAL, .dstSubpass = 0,
       .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
       .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
       .srcAccessMask = 0, .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };
    VkRenderPassCreateInfo wsi_rpci = {
       .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
       .attachmentCount = 1, .pAttachments = &wsi_att, .subpassCount = 1, .pSubpasses = &wsi_sp,
       .dependencyCount = 1, .pDependencies = &wsi_dep,
    };
    VkRenderPass wsi_rp;
    crp(dev, &wsi_rpci, NULL, &wsi_rp);
    for (uint32_t i = 0; i < img_count; i++) {
       VkImageViewCreateInfo ivci = {
          .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, .image = sc_imgs[i],
          .viewType = VK_IMAGE_VIEW_TYPE_2D, .format = chosen_fmt.format,
          .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1},
       };
       civ(dev, &ivci, NULL, &sc_views[i]);
       VkFramebufferCreateInfo wsi_fbci = {
          .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, .renderPass = wsi_rp,
          .attachmentCount = 1, .pAttachments = &sc_views[i], .width = sw, .height = sh, .layers = 1,
       };
       cfb(dev, &wsi_fbci, NULL, &sc_fbs[i]);
    }

    VkDynamicState dyn_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynsi = {
       .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
       .dynamicStateCount = 2, .pDynamicStates = dyn_states,
    };
    VkPipelineViewportStateCreateInfo wsi_pvsi = {
       .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, .viewportCount = 1, .scissorCount = 1,
    };
    VkGraphicsPipelineCreateInfo wsi_gpci = {
       .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, .stageCount = 2, .pStages = a_stages,
       .pVertexInputState = &pvisi, .pInputAssemblyState = &piasi, .pViewportState = &wsi_pvsi,
       .pRasterizationState = &prsi, .pMultisampleState = &pmsi, .pColorBlendState = &pcbsi,
       .pDynamicState = &dynsi, .layout = a_layout, .renderPass = wsi_rp, .subpass = 0,
    };
    VkPipeline wsi_pipe;
    cgp(dev, VK_NULL_HANDLE, 1, &wsi_gpci, NULL, &wsi_pipe);

    enum { FIF = 2 };
    VkSemaphore acquire_sem[FIF], render_sem[FIF];
    VkFence wsi_fence[FIF];
    VkCommandBuffer wsi_cmd[FIF];
    VkSemaphoreCreateInfo semci = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo wsi_fci = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT};
    VkCommandBufferAllocateInfo wsi_cbai = {
       .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, .commandPool = c_cpool, .commandBufferCount = FIF,
    };
    acb(dev, &wsi_cbai, wsi_cmd);
    for (int i = 0; i < FIF; i++) {
       csem(dev, &semci, NULL, &acquire_sem[i]);
       csem(dev, &semci, NULL, &render_sem[i]);
       cf(dev, &wsi_fci, NULL, &wsi_fence[i]);
    }

    PFN_vkCmdSetViewport csvp = (PFN_vkCmdSetViewport)gipa(inst, "vkCmdSetViewport");
    PFN_vkCmdSetScissor cssc = (PFN_vkCmdSetScissor)gipa(inst, "vkCmdSetScissor");
    int acquired = 0, submitted = 0, presented = 0;
    int device_lost = 0, hang = 0, out_of_date = 0, suboptimal = 0;
    LOGI("GPU_RENDER_TO_SWAPCHAIN start frames=300 extent=%ux%u", sw, sh);
    for (int frame = 0; frame < 300; frame++) {
       int f = frame % FIF;
       if (wff(dev, 1, &wsi_fence[f], VK_TRUE, 5000000000ULL) != VK_SUCCESS) {
          hang = 1;
          LOGE("HANG wait fence frame=%d", frame);
          break;
       }
       rf(dev, 1, &wsi_fence[f]);
       uint32_t img_idx = 0;
       VkResult ar = ani(dev, swapchain, 5000000000ULL, acquire_sem[f], VK_NULL_HANDLE, &img_idx);
       if (ar == VK_ERROR_OUT_OF_DATE_KHR) { out_of_date++; LOGE("ACQUIRE out_of_date frame=%d", frame); break; }
       if (ar == VK_ERROR_DEVICE_LOST) { device_lost = 1; LOGE("DEVICE_LOST acquire frame=%d", frame); break; }
       if (ar != VK_SUCCESS && ar != VK_SUBOPTIMAL_KHR) {
          LOGE("ACQUIRE fail frame=%d vr=%d", frame, (int)ar);
          break;
       }
       if (ar == VK_SUBOPTIMAL_KHR) suboptimal++;
       acquired++;

       rcb(wsi_cmd[f], 0);
       VkCommandBufferBeginInfo wsi_bbi = {
          .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
          .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
       };
       bcb(wsi_cmd[f], &wsi_bbi);
       float t = (float)(frame % 256) / 255.0f;
       VkClearValue wsi_clear = {.color = {{t, 0.1f, 1.0f - t, 1.0f}}};
       VkRenderPassBeginInfo wsi_rpbi = {
          .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, .renderPass = wsi_rp,
          .framebuffer = sc_fbs[img_idx], .renderArea = {{0, 0}, {sw, sh}},
          .clearValueCount = 1, .pClearValues = &wsi_clear,
       };
       cbrp(wsi_cmd[f], &wsi_rpbi, VK_SUBPASS_CONTENTS_INLINE);
       VkViewport wsi_vp = {.x = 0, .y = 0, .width = (float)sw, .height = (float)sh, .minDepth = 0.0f, .maxDepth = 1.0f};
       VkRect2D wsi_sc = {.offset = {0, 0}, .extent = {sw, sh}};
       csvp(wsi_cmd[f], 0, 1, &wsi_vp);
       cssc(wsi_cmd[f], 0, 1, &wsi_sc);
       cbp(wsi_cmd[f], VK_PIPELINE_BIND_POINT_GRAPHICS, wsi_pipe);
       VkDeviceSize wsi_voff = 0;
       cbvb(wsi_cmd[f], 0, 1, &vbuf, &wsi_voff);
       cdraw(wsi_cmd[f], 3, 1, 0, 0);
       cerp(wsi_cmd[f]);
       ecb(wsi_cmd[f]);

       VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
       VkSubmitInfo wsi_si = {
          .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
          .waitSemaphoreCount = 1, .pWaitSemaphores = &acquire_sem[f], .pWaitDstStageMask = &wait_stage,
          .commandBufferCount = 1, .pCommandBuffers = &wsi_cmd[f],
          .signalSemaphoreCount = 1, .pSignalSemaphores = &render_sem[f],
       };
       VkResult sr = qs(queue, 1, &wsi_si, wsi_fence[f]);
       if (sr == VK_ERROR_DEVICE_LOST) { device_lost = 1; LOGE("DEVICE_LOST submit frame=%d", frame); break; }
       if (sr != VK_SUCCESS) { LOGE("QUEUE_SUBMIT fail frame=%d vr=%d", frame, (int)sr); break; }
       submitted++;

       VkPresentInfoKHR pi = {
          .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
          .waitSemaphoreCount = 1, .pWaitSemaphores = &render_sem[f],
          .swapchainCount = 1, .pSwapchains = &swapchain, .pImageIndices = &img_idx,
       };
       uint64_t present_id = (uint64_t)frame + 1;
       VkPresentId2KHR present_id_info = {.sType = VK_STRUCTURE_TYPE_PRESENT_ID_2_KHR,
          .swapchainCount = 1, .pPresentIds = &present_id};
       pi.pNext = &present_id_info;
       VkResult pr = qp(queue, &pi);
       if (pr == VK_ERROR_OUT_OF_DATE_KHR) { out_of_date++; LOGE("QUEUE_PRESENT out_of_date frame=%d", frame); break; }
       if (pr == VK_ERROR_DEVICE_LOST) { device_lost = 1; LOGE("DEVICE_LOST present frame=%d", frame); break; }
       if (pr != VK_SUCCESS && pr != VK_SUBOPTIMAL_KHR) {
          LOGE("QUEUE_PRESENT fail frame=%d vr=%d", frame, (int)pr);
          break;
       }
       if (pr == VK_SUBOPTIMAL_KHR) suboptimal++;
       presented++;
       VkPresentWait2InfoKHR wait_info = {.sType = VK_STRUCTURE_TYPE_PRESENT_WAIT_2_INFO_KHR,
          .presentId = present_id, .timeout = 5000000000ULL};
       VkResult present_wait_result = wait_present2 ? wait_present2(dev, swapchain, &wait_info) : VK_ERROR_EXTENSION_NOT_PRESENT;
       if (present_wait_result != VK_SUCCESS) {
          LOGE("PHASE7 VK_KHR_present_wait2 status=FAIL frame=%d result=%d", frame, present_wait_result);
          break;
       }
       if (frame == 0) {
          LOGI("PHASE7 VK_KHR_present_id2 status=PASS presentId=%llu", (unsigned long long)present_id);
          LOGI("PHASE7 VK_KHR_present_wait2 status=PASS presentId=%llu", (unsigned long long)present_id);
       }
       if (frame % 50 == 0) LOGI("PRESENT_FRAMES=%d/300", presented);
    }
    LOGI("ACQUIRE=%d/300 GPU_RENDER_TO_SWAPCHAIN=%d/300 QUEUE_PRESENT=%d/300 PRESENT_FRAMES=%d/300 DEVICE_LOST=%d HANG=%d OUT_OF_DATE=%d SUBOPTIMAL=%d",
         acquired, submitted, presented, presented, device_lost, hang, out_of_date, suboptimal);
    if (dwi) dwi(dev);

    PFN_vkDestroyImageView div_early = (PFN_vkDestroyImageView)gipa(inst, "vkDestroyImageView");
    PFN_vkDestroyFramebuffer dfb_early = (PFN_vkDestroyFramebuffer)gipa(inst, "vkDestroyFramebuffer");
    PFN_vkDestroyPipeline dp_early = (PFN_vkDestroyPipeline)gipa(inst, "vkDestroyPipeline");
    PFN_vkDestroyRenderPass drp_early = (PFN_vkDestroyRenderPass)gipa(inst, "vkDestroyRenderPass");
    PFN_vkDestroyFence df_early = (PFN_vkDestroyFence)gipa(inst, "vkDestroyFence");
    for (int i = 0; i < FIF; i++) {
       df_early(dev, wsi_fence[i], NULL);
       dsem(dev, acquire_sem[i], NULL);
       dsem(dev, render_sem[i], NULL);
    }
    dp_early(dev, wsi_pipe, NULL);
    for (uint32_t i = 0; i < img_count; i++) {
       dfb_early(dev, sc_fbs[i], NULL);
       div_early(dev, sc_views[i], NULL);
    }
    drp_early(dev, wsi_rp, NULL);
    dsc(dev, swapchain, NULL);
    dsurf(inst, vk_surf, NULL);
    free(sc_imgs);
    free(sc_views);
    free(sc_fbs);
    ANativeWindow_release(win);
    if (presented < 300 || device_lost || hang) {
       LOGE("Gate H FAIL present=%d lost=%d hang=%d", presented, device_lost, hang);
       return (*env)->NewStringUTF(env, "FAIL VULKAN_WSI 300");
    }
    LOGI("Gate H: PASS Vulkan WSI 300/300");

   /* Cleanup Vulkan */
   AHardwareBuffer_release(ahb);
   PFN_vkDestroyFence df = (PFN_vkDestroyFence)gipa(inst, "vkDestroyFence");
   PFN_vkDestroyCommandPool dcp = (PFN_vkDestroyCommandPool)gipa(inst, "vkDestroyCommandPool");
   PFN_vkDestroyPipeline dp = (PFN_vkDestroyPipeline)gipa(inst, "vkDestroyPipeline");
   PFN_vkDestroyPipelineLayout dpl = (PFN_vkDestroyPipelineLayout)gipa(inst, "vkDestroyPipelineLayout");
   PFN_vkDestroyShaderModule dsm = (PFN_vkDestroyShaderModule)gipa(inst, "vkDestroyShaderModule");
   PFN_vkDestroyFramebuffer dfb = (PFN_vkDestroyFramebuffer)gipa(inst, "vkDestroyFramebuffer");
   PFN_vkDestroyRenderPass drp = (PFN_vkDestroyRenderPass)gipa(inst, "vkDestroyRenderPass");
   PFN_vkDestroyImageView div = (PFN_vkDestroyImageView)gipa(inst, "vkDestroyImageView");
   PFN_vkDestroyImage di = (PFN_vkDestroyImage)gipa(inst, "vkDestroyImage");
   PFN_vkDestroyBuffer db = (PFN_vkDestroyBuffer)gipa(inst, "vkDestroyBuffer");
   PFN_vkFreeMemory fm = (PFN_vkFreeMemory)gipa(inst, "vkFreeMemory");
   PFN_vkDestroyDevice dd = (PFN_vkDestroyDevice)gipa(inst, "vkDestroyDevice");
   PFN_vkDestroyInstance di_ = (PFN_vkDestroyInstance)gipa(inst, "vkDestroyInstance");

   df(dev, fence, NULL);
   dcp(dev, c_cpool, NULL);
   dp(dev, c_pipe, NULL);
   dpl(dev, c_layout, NULL);
   dsm(dev, c_mod, NULL);
   db(dev, cbuf, NULL);
   fm(dev, cmem, NULL);
   dp(dev, a_pipe, NULL);
   dpl(dev, a_layout, NULL);
   dsm(dev, a_vsm, NULL);
   dsm(dev, a_fsm, NULL);
   dfb(dev, a_fb, NULL);
   drp(dev, a_rp, NULL);
   div(dev, a_view, NULL);
   di(dev, a_img, NULL);
   fm(dev, a_mem, NULL);
   db(dev, vbuf, NULL);
   fm(dev, vmem, NULL);
   dd(dev, NULL);
   di_(inst, NULL);

   LOGI("All gates PASS inside Android app UID=%d PID_SELF=%d COLD_LAUNCH_PROCESS=done",
        getuid(), getpid());

   snprintf(report, sizeof(report),
      "PANVK LOADER APP TEST RESULTS:\n"
      "- App UID: %d (non-root)\n"
      "- Gate C (ICD / Enum): PASS (Mali-G615)\n"
      "- Gate D (Device Create): PASS\n"
      "- Gate E (Compute): PASS (10/10 exact)\n"
      "- Gate G (AHB Import/Render): PASS\n"
       "- Gate H (Vulkan WSI 300): PASS\n"
       "STATUS: ALL GATES PASS", getuid());

   return (*env)->NewStringUTF(env, report);
}
