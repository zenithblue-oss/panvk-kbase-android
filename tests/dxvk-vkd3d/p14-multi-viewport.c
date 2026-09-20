#define VK_NO_PROTOTYPES
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

static PFN_vkGetInstanceProcAddr gipa;
static PFN_vkCreateInstance vkCreateInstance;
static PFN_vkDestroyInstance vkDestroyInstance;
static PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
static PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
static PFN_vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFeatures;
static PFN_vkGetPhysicalDeviceFeatures2 vkGetPhysicalDeviceFeatures2;
static PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
static PFN_vkCreateDevice vkCreateDevice;
static PFN_vkDestroyDevice vkDestroyDevice;

static const char *
vk_result_name(VkResult r)
{
   switch (r) {
   case VK_SUCCESS: return "VK_SUCCESS";
   case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
   case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
   default: return "OTHER";
   }
}

static VkResult
try_create(VkPhysicalDevice physical, uint32_t family, void *pnext,
           const VkPhysicalDeviceFeatures *core)
{
   float priority = 1.0f;
   VkDeviceQueueCreateInfo qci = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = family,
      .queueCount = 1,
      .pQueuePriorities = &priority,
   };
   VkDeviceCreateInfo dci = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext = pnext,
      .queueCreateInfoCount = 1,
      .pQueueCreateInfos = &qci,
      .pEnabledFeatures = core,
   };
   VkDevice device = VK_NULL_HANDLE;
   VkResult r = vkCreateDevice(physical, &dci, NULL, &device);
   if (r == VK_SUCCESS)
      vkDestroyDevice(device, NULL);
   return r;
}

int
main(int argc, char **argv)
{
   if (argc != 2) {
      fprintf(stderr, "usage: %s <ICD.so>\n", argv[0]);
      return 2;
   }

   void *lib = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
   if (!lib) {
      fprintf(stderr, "dlopen: %s\n", dlerror());
      return 1;
   }
   gipa = (PFN_vkGetInstanceProcAddr)dlsym(lib, "vk_icdGetInstanceProcAddr");
   if (!gipa)
      gipa = (PFN_vkGetInstanceProcAddr)dlsym(lib, "vkGetInstanceProcAddr");
   if (!gipa) {
      fprintf(stderr, "no vkGetInstanceProcAddr\n");
      return 1;
   }

   vkCreateInstance = (PFN_vkCreateInstance)gipa(NULL, "vkCreateInstance");
   VkApplicationInfo app = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .apiVersion = VK_API_VERSION_1_4,
   };
   VkInstanceCreateInfo ici = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &app,
   };
   VkInstance instance = VK_NULL_HANDLE;
   if (!vkCreateInstance || vkCreateInstance(&ici, NULL, &instance) != VK_SUCCESS) {
      fprintf(stderr, "vkCreateInstance failed\n");
      return 1;
   }

#define LOAD(name) name = (PFN_##name)gipa(instance, #name)
   LOAD(vkDestroyInstance);
   LOAD(vkEnumeratePhysicalDevices);
   LOAD(vkGetPhysicalDeviceProperties);
   LOAD(vkGetPhysicalDeviceFeatures);
   LOAD(vkGetPhysicalDeviceFeatures2);
   LOAD(vkGetPhysicalDeviceQueueFamilyProperties);
   LOAD(vkCreateDevice);
   LOAD(vkDestroyDevice);
#undef LOAD

   uint32_t count = 0;
   if (vkEnumeratePhysicalDevices(instance, &count, NULL) != VK_SUCCESS || !count) {
      fprintf(stderr, "no physical devices\n");
      return 1;
   }
   VkPhysicalDevice *devices = calloc(count, sizeof(*devices));
   vkEnumeratePhysicalDevices(instance, &count, devices);
   VkPhysicalDevice physical = VK_NULL_HANDLE;
   VkPhysicalDeviceProperties props;
   for (uint32_t i = 0; i < count; i++) {
      vkGetPhysicalDeviceProperties(devices[i], &props);
      if (strstr(props.deviceName, "Mali")) {
         physical = devices[i];
         break;
      }
   }
   free(devices);
   if (!physical) {
      fprintf(stderr, "no Mali device\n");
      return 1;
   }

   uint32_t qcount = 0;
   vkGetPhysicalDeviceQueueFamilyProperties(physical, &qcount, NULL);
   VkQueueFamilyProperties *queues = calloc(qcount, sizeof(*queues));
   vkGetPhysicalDeviceQueueFamilyProperties(physical, &qcount, queues);
   uint32_t family = UINT32_MAX;
   for (uint32_t i = 0; i < qcount; i++) {
      if ((queues[i].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) ==
          (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) {
         family = i;
         break;
      }
   }
   free(queues);
   if (family == UINT32_MAX) {
      fprintf(stderr, "no graphics+compute queue\n");
      return 1;
   }

   VkPhysicalDeviceVulkan12Features v12 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
   };
   VkPhysicalDeviceVulkan11Features v11 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
      .pNext = &v12,
   };
   VkPhysicalDeviceFeatures2 f2 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
      .pNext = &v11,
   };
   vkGetPhysicalDeviceFeatures2(physical, &f2);

   int mv = f2.features.multiViewport == VK_TRUE;
   int svoi = v12.shaderOutputViewportIndex == VK_TRUE;
   int mview = v11.multiview == VK_TRUE;
   uint32_t max_vp = props.limits.maxViewports;
   printf("device=%s\n", props.deviceName);
   printf("api=%u.%u.%u\n", VK_VERSION_MAJOR(props.apiVersion),
          VK_VERSION_MINOR(props.apiVersion), VK_VERSION_PATCH(props.apiVersion));
   printf("multiViewport=%s\n", mv ? "YES" : "NO");
   printf("maxViewports=%u\n", max_vp);
   printf("shaderOutputViewportIndex=%s\n", svoi ? "YES" : "NO");
   printf("multiview=%s\n", mview ? "YES" : "NO");
   printf("note=multiview_is_not_multiViewport\n");

   VkPhysicalDeviceFeatures want_mv = {.multiViewport = VK_TRUE};
   VkResult mv_create = try_create(physical, family, NULL, &want_mv);
   int mv_reject = mv_create == VK_ERROR_FEATURE_NOT_PRESENT;
   printf("create_multiViewport_true=%s\n", vk_result_name(mv_create));
   printf("create_multiViewport_rejected %s\n", mv_reject ? "PASS" : "FAIL");

   VkPhysicalDeviceVulkan12Features want_svoi = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
      .shaderOutputViewportIndex = VK_TRUE,
   };
   VkResult svoi_create = try_create(physical, family, &want_svoi, NULL);
   int svoi_reject = svoi_create == VK_ERROR_FEATURE_NOT_PRESENT;
   printf("create_shaderOutputViewportIndex_true=%s\n", vk_result_name(svoi_create));
   printf("create_shaderOutputViewportIndex_rejected %s\n",
          svoi_reject ? "PASS" : "FAIL");

   int limits_one = max_vp == 1;
   printf("maxViewports_is_1 %s\n", limits_one ? "PASS" : "FAIL");

   const char *cases[] = {
      "two_viewports",
      "four_viewports",
      "maximum_supported",
      "different_origins",
      "different_sizes",
      "different_depth_ranges",
      "overlap",
      "dynamic_viewport",
      "dynamic_scissor",
      "viewport_index",
   };
   for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
      printf("%s=NOT_RUN reason=multiViewport_disabled\n", cases[i]);

   int failed = mv || svoi || !mv_reject || !svoi_reject || !limits_one;
   printf("result=%s\n", failed ? "FAIL" : "BLOCKED_SAFE_FALSE");
   vkDestroyInstance(instance, NULL);
   return failed;
}
