#define VK_USE_PLATFORM_ANDROID_KHR 1
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

int main(int argc, char **argv) {
   if (argc < 2) return 1;
   void *h = dlopen(argv[1], RTLD_NOW);
   if (!h) { printf("dlopen fail\n"); return 1; }
   PFN_vkGetInstanceProcAddr gipa = (PFN_vkGetInstanceProcAddr)dlsym(h, "vk_icdGetInstanceProcAddr");
   if (!gipa) gipa = (PFN_vkGetInstanceProcAddr)dlsym(h, "vkGetInstanceProcAddr");

   PFN_vkEnumerateInstanceExtensionProperties eiep = (PFN_vkEnumerateInstanceExtensionProperties)gipa(NULL, "vkEnumerateInstanceExtensionProperties");
   uint32_t ie_cnt = 0;
   eiep(NULL, &ie_cnt, NULL);
   VkExtensionProperties *ie_props = malloc(ie_cnt * sizeof(*ie_props));
   eiep(NULL, &ie_cnt, ie_props);

   printf("=== INSTANCE EXTENSIONS (%u) ===\n", ie_cnt);
   for (uint32_t i = 0; i < ie_cnt; i++)
      printf("  %s (v%u)\n", ie_props[i].extensionName, ie_props[i].specVersion);

   PFN_vkCreateInstance ci = (PFN_vkCreateInstance)gipa(NULL, "vkCreateInstance");
   VkApplicationInfo app = {.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO, .apiVersion = VK_API_VERSION_1_3};
   VkInstanceCreateInfo ici = {.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, .pApplicationInfo = &app};
   VkInstance inst;
   ci(&ici, NULL, &inst);

   PFN_vkEnumeratePhysicalDevices epd = (PFN_vkEnumeratePhysicalDevices)gipa(inst, "vkEnumeratePhysicalDevices");
   uint32_t n = 1;
   VkPhysicalDevice phys;
   epd(inst, &n, &phys);

    PFN_vkGetPhysicalDeviceProperties2 gpd2 = (PFN_vkGetPhysicalDeviceProperties2)gipa(inst, "vkGetPhysicalDeviceProperties2");
    VkPhysicalDeviceDriverProperties drv = {
       .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES,
    };
    VkPhysicalDeviceProperties2 p2 = {
       .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
       .pNext = &drv,
    };
    gpd2(phys, &p2);
    printf("=== PHYSICAL DEVICE ===\n");
    printf("  deviceName: %s\n", p2.properties.deviceName);
    printf("  deviceID: 0x%08x\n", p2.properties.deviceID);
    printf("  vendorID: 0x%04x\n", p2.properties.vendorID);
    printf("  apiVersion: %u.%u.%u\n",
           VK_VERSION_MAJOR(p2.properties.apiVersion),
           VK_VERSION_MINOR(p2.properties.apiVersion),
           VK_VERSION_PATCH(p2.properties.apiVersion));
    printf("  driverVersion: 0x%x\n", p2.properties.driverVersion);
    printf("  driverID: %u\n", (unsigned)drv.driverID);
    printf("  driverName: %s\n", drv.driverName);
    printf("  driverInfo: %s\n", drv.driverInfo);

   PFN_vkEnumerateDeviceExtensionProperties edep = (PFN_vkEnumerateDeviceExtensionProperties)gipa(inst, "vkEnumerateDeviceExtensionProperties");
   uint32_t de_cnt = 0;
   edep(phys, NULL, &de_cnt, NULL);
   VkExtensionProperties *de_props = malloc(de_cnt * sizeof(*de_props));
   edep(phys, NULL, &de_cnt, de_props);
   printf("=== DEVICE EXTENSIONS (%u) ===\n", de_cnt);
   for (uint32_t i = 0; i < de_cnt; i++)
      printf("  %s (v%u)\n", de_props[i].extensionName, de_props[i].specVersion);

   PFN_vkGetPhysicalDeviceFeatures2 gpf2 = (PFN_vkGetPhysicalDeviceFeatures2)gipa(inst, "vkGetPhysicalDeviceFeatures2");
   VkPhysicalDeviceVulkan11Features f11 = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
   VkPhysicalDeviceVulkan12Features f12 = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES, .pNext = &f11};
   VkPhysicalDeviceVulkan13Features f13 = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES, .pNext = &f12};
   VkPhysicalDeviceFeatures2 f2 = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &f13};
   gpf2(phys, &f2);

   printf("=== VULKAN 1.0 CORE FEATURES ===\n");
#define F(x) printf("  %-36s : %s\n", #x, f2.features.x ? "YES" : "NO")
   F(robustBufferAccess);
   F(fullDrawIndexUint32);
   F(imageCubeArray);
   F(independentBlend);
   F(geometryShader);
   F(tessellationShader);
   F(sampleRateShading);
   F(dualSrcBlend);
   F(logicOp);
   F(multiDrawIndirect);
   F(drawIndirectFirstInstance);
   F(depthClamp);
   F(depthBiasClamp);
   F(fillModeNonSolid);
   F(depthBounds);
   F(wideLines);
   F(largePoints);
   F(alphaToOne);
   F(multiViewport);
   F(samplerAnisotropy);
   F(textureCompressionETC2);
   F(textureCompressionASTC_LDR);
   F(textureCompressionBC);
   F(occlusionQueryPrecise);
   F(pipelineStatisticsQuery);
   F(vertexPipelineStoresAndAtomics);
   F(fragmentStoresAndAtomics);
   F(shaderTessellationAndGeometryPointSize);
   F(shaderImageGatherExtended);
   F(shaderStorageImageExtendedFormats);
   F(shaderStorageImageMultisample);
   F(shaderStorageImageReadWithoutFormat);
   F(shaderStorageImageWriteWithoutFormat);
   F(shaderUniformBufferArrayDynamicIndexing);
   F(shaderSampledImageArrayDynamicIndexing);
   F(shaderStorageBufferArrayDynamicIndexing);
   F(shaderStorageImageArrayDynamicIndexing);
   F(shaderClipDistance);
   F(shaderCullDistance);
   F(shaderFloat64);
   F(shaderInt64);
   F(shaderInt16);
   F(shaderResourceResidency);
   F(shaderResourceMinLod);
   F(sparseBinding);
   F(sparseResidencyBuffer);
   F(sparseResidencyImage2D);
   F(sparseResidencyImage3D);
   F(sparseResidency2Samples);
   F(sparseResidency4Samples);
   F(sparseResidency8Samples);
   F(sparseResidency16Samples);
   F(sparseResidencyAliased);
   F(variableMultisampleRate);
   F(inheritedQueries);
#undef F

   printf("=== VULKAN 1.1 / 1.2 / 1.3 FEATURES ===\n");
#define F12(x) printf("  %-36s : %s\n", #x, f12.x ? "YES" : "NO")
#define F13(x) printf("  %-36s : %s\n", #x, f13.x ? "YES" : "NO")
   F12(descriptorIndexing);
   F12(timelineSemaphore);
   F12(bufferDeviceAddress);
   F12(scalarBlockLayout);
   F12(hostQueryReset);
   F12(vulkanMemoryModel);
   F12(imagelessFramebuffer);
   F12(separateDepthStencilLayouts);
   F13(dynamicRendering);
   F13(synchronization2);
   F13(inlineUniformBlock);
   F13(maintenance4);
#undef F12
#undef F13

   return 0;
}
