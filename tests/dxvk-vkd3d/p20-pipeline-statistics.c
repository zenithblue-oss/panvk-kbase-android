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
static PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties;

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
try_create(VkPhysicalDevice physical, uint32_t family,
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

static int
has_extension(VkPhysicalDevice physical, const char *name)
{
   uint32_t count = 0;
   vkEnumerateDeviceExtensionProperties(physical, NULL, &count, NULL);
   VkExtensionProperties *exts = calloc(count, sizeof(*exts));
   if (!exts)
      return 0;
   vkEnumerateDeviceExtensionProperties(physical, NULL, &count, exts);
   int found = 0;
   for (uint32_t i = 0; i < count; i++) {
      if (strcmp(exts[i].extensionName, name) == 0) {
         found = 1;
         break;
      }
   }
   free(exts);
   return found;
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
   LOAD(vkEnumerateDeviceExtensionProperties);
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

   VkPhysicalDeviceFeatures2 f2 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
   };
   vkGetPhysicalDeviceFeatures2(physical, &f2);

   int stats = f2.features.pipelineStatisticsQuery == VK_TRUE;
   int oq = f2.features.occlusionQueryPrecise == VK_TRUE;
   int prims_ext = has_extension(physical, "VK_EXT_primitives_generated_query");
   int xfb_ext = has_extension(physical, "VK_EXT_transform_feedback");

   printf("device=%s\n", props.deviceName);
   printf("api=%u.%u.%u\n", VK_VERSION_MAJOR(props.apiVersion),
          VK_VERSION_MINOR(props.apiVersion), VK_VERSION_PATCH(props.apiVersion));
   printf("pipelineStatisticsQuery=%s\n", stats ? "YES" : "NO");
   printf("occlusionQueryPrecise=%s\n", oq ? "YES" : "NO");
   printf("VK_EXT_primitives_generated_query=%s\n", prims_ext ? "YES" : "NO");
   printf("VK_EXT_transform_feedback=%s\n", xfb_ext ? "YES" : "NO");

   VkPhysicalDeviceFeatures want_stats = {.pipelineStatisticsQuery = VK_TRUE};
   VkResult stats_create = try_create(physical, family, &want_stats);
   int stats_reject = stats_create == VK_ERROR_FEATURE_NOT_PRESENT;
   printf("create_pipelineStatisticsQuery_true=%s\n", vk_result_name(stats_create));
   printf("create_pipelineStatisticsQuery_rejected %s\n", stats_reject ? "PASS" : "FAIL");

   VkPhysicalDeviceFeatures none = {0};
   VkResult default_create = try_create(physical, family, &none);
   printf("create_default_features=%s\n", vk_result_name(default_create));
   printf("create_default_features_ok %s\n",
          default_create == VK_SUCCESS ? "PASS" : "FAIL");

   const char *cases[] = {
      "IA_VERTICES",
      "IA_PRIMITIVES",
      "VS_INVOCATIONS",
      "GS_INVOCATIONS",
      "GS_PRIMITIVES",
      "CLIPPING_INVOCATIONS",
      "CLIPPING_PRIMITIVES",
      "FS_INVOCATIONS",
      "TCS_PATCHES",
      "TES_INVOCATIONS",
      "CS_INVOCATIONS",
   };
   for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
      printf("%s=NOT_RUN reason=pipelineStatisticsQuery_disabled\n", cases[i]);

   int failed = stats || !stats_reject || default_create != VK_SUCCESS;
   printf("result=%s\n", failed ? "FAIL" : "BLOCKED_SAFE_FALSE");
   vkDestroyInstance(instance, NULL);
   return failed;
}
