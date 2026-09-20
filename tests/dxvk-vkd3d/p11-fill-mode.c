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
static PFN_vkGetPhysicalDeviceFeatures2 vkGetPhysicalDeviceFeatures2;
static PFN_vkGetPhysicalDeviceProperties2 vkGetPhysicalDeviceProperties2;
static PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
static PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties;
static PFN_vkCreateDevice vkCreateDevice;
static PFN_vkDestroyDevice vkDestroyDevice;

static int
has_ext(VkPhysicalDevice physical, const char *name)
{
   uint32_t count = 0;
   vkEnumerateDeviceExtensionProperties(physical, NULL, &count, NULL);
   VkExtensionProperties *props = calloc(count, sizeof(*props));
   if (!props)
      return 0;
   vkEnumerateDeviceExtensionProperties(physical, NULL, &count, props);
   int found = 0;
   for (uint32_t i = 0; i < count; i++)
      found |= !strcmp(props[i].extensionName, name);
   free(props);
   return found;
}

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
           const VkPhysicalDeviceFeatures *core, const char *const *exts,
           uint32_t ext_count)
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
      .enabledExtensionCount = ext_count,
      .ppEnabledExtensionNames = exts,
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
   LOAD(vkGetPhysicalDeviceFeatures2);
   LOAD(vkGetPhysicalDeviceProperties2);
   LOAD(vkGetPhysicalDeviceQueueFamilyProperties);
   LOAD(vkEnumerateDeviceExtensionProperties);
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

   VkPhysicalDeviceExtendedDynamicState3FeaturesEXT eds3 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT,
   };
   VkPhysicalDeviceFeatures2 f2 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
      .pNext = &eds3,
   };
   vkGetPhysicalDeviceFeatures2(physical, &f2);

   VkPhysicalDeviceVulkan14Properties p14 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_PROPERTIES,
   };
   VkPhysicalDeviceProperties2 p2 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
      .pNext = &p14,
   };
   vkGetPhysicalDeviceProperties2(physical, &p2);

   int fill = f2.features.fillModeNonSolid == VK_TRUE;
   int dyn = eds3.extendedDynamicState3PolygonMode == VK_TRUE;
   int psize = p14.polygonModePointSize == VK_TRUE;
   printf("device=%s\n", props.deviceName);
   printf("api=%u.%u.%u\n", VK_VERSION_MAJOR(props.apiVersion),
          VK_VERSION_MINOR(props.apiVersion), VK_VERSION_PATCH(props.apiVersion));
   printf("fillModeNonSolid=%s\n", fill ? "YES" : "NO");
   printf("extendedDynamicState3PolygonMode=%s\n", dyn ? "YES" : "NO");
   printf("polygonModePointSize=%s\n", psize ? "YES" : "NO");

   VkPhysicalDeviceFeatures want_fill = {.fillModeNonSolid = VK_TRUE};
   VkResult fill_create = try_create(physical, family, NULL, &want_fill, NULL, 0);
   int fill_reject = fill_create == VK_ERROR_FEATURE_NOT_PRESENT;
   printf("create_fillModeNonSolid_true=%s\n", vk_result_name(fill_create));
   printf("create_fillModeNonSolid_rejected %s\n", fill_reject ? "PASS" : "FAIL");

   int dyn_reject = 1;
   int has_eds3 = has_ext(physical, VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME);
   printf("VK_EXT_extended_dynamic_state3=%s\n", has_eds3 ? "YES" : "NO");
   if (has_eds3) {
      VkPhysicalDeviceExtendedDynamicState3FeaturesEXT want_dyn = {
         .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT,
         .extendedDynamicState3PolygonMode = VK_TRUE,
      };
      const char *ext = VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME;
      VkResult dyn_create = try_create(physical, family, &want_dyn, NULL, &ext, 1);
      dyn_reject = dyn_create == VK_ERROR_FEATURE_NOT_PRESENT;
      printf("create_extendedDynamicState3PolygonMode_true=%s\n",
             vk_result_name(dyn_create));
      printf("create_extendedDynamicState3PolygonMode_rejected %s\n",
             dyn_reject ? "PASS" : "FAIL");
   } else {
      printf("create_extendedDynamicState3PolygonMode_true=SKIP\n");
      printf("create_extendedDynamicState3PolygonMode_rejected PASS\n");
   }

   const char *cases[] = {
      "wireframe_triangle",
      "point_polygon_mode",
      "front_back_culling",
      "depth",
      "stencil",
      "msaa",
      "line_width_interaction",
      "dynamic_raster_state",
      "indexed_draw",
      "indirect_draw",
   };
   for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
      printf("%s=NOT_RUN reason=fillModeNonSolid_disabled\n", cases[i]);

   int failed = fill || dyn || psize || !fill_reject || !dyn_reject;
   printf("result=%s\n", failed ? "FAIL" : "BLOCKED_SAFE_FALSE");
   vkDestroyInstance(instance, NULL);
   return failed;
}
