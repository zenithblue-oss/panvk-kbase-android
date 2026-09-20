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

   VkPhysicalDeviceVulkan11Features v11 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
   };
   VkPhysicalDeviceFeatures2 f2 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
      .pNext = &v11,
   };
   vkGetPhysicalDeviceFeatures2(physical, &f2);

   int tess = f2.features.tessellationShader == VK_TRUE;
   int tess_psiz = f2.features.shaderTessellationAndGeometryPointSize == VK_TRUE;
   int mv_tess = v11.multiviewTessellationShader == VK_TRUE;
   printf("device=%s\n", props.deviceName);
   printf("api=%u.%u.%u\n", VK_VERSION_MAJOR(props.apiVersion),
          VK_VERSION_MINOR(props.apiVersion), VK_VERSION_PATCH(props.apiVersion));
   printf("tessellationShader=%s\n", tess ? "YES" : "NO");
   printf("shaderTessellationAndGeometryPointSize=%s\n", tess_psiz ? "YES" : "NO");
   printf("multiviewTessellationShader=%s\n", mv_tess ? "YES" : "NO");
   printf("maxTessellationGenerationLevel=%u\n",
          props.limits.maxTessellationGenerationLevel);
   printf("maxTessellationPatchSize=%u\n",
          props.limits.maxTessellationPatchSize);
   printf("maxTessellationControlPerVertexInputComponents=%u\n",
          props.limits.maxTessellationControlPerVertexInputComponents);
   printf("maxTessellationControlPerVertexOutputComponents=%u\n",
          props.limits.maxTessellationControlPerVertexOutputComponents);
   printf("maxTessellationControlPerPatchOutputComponents=%u\n",
          props.limits.maxTessellationControlPerPatchOutputComponents);
   printf("maxTessellationControlTotalOutputComponents=%u\n",
          props.limits.maxTessellationControlTotalOutputComponents);
   printf("maxTessellationEvaluationInputComponents=%u\n",
          props.limits.maxTessellationEvaluationInputComponents);
   printf("maxTessellationEvaluationOutputComponents=%u\n",
          props.limits.maxTessellationEvaluationOutputComponents);

   VkPhysicalDeviceFeatures want_tess = {.tessellationShader = VK_TRUE};
   VkResult tess_create = try_create(physical, family, &want_tess);
   int tess_reject = tess_create == VK_ERROR_FEATURE_NOT_PRESENT;
   printf("create_tessellationShader_true=%s\n", vk_result_name(tess_create));
   printf("create_tessellationShader_rejected %s\n", tess_reject ? "PASS" : "FAIL");

   int limits_zero =
      props.limits.maxTessellationGenerationLevel == 0 &&
      props.limits.maxTessellationPatchSize == 0 &&
      props.limits.maxTessellationControlPerVertexInputComponents == 0 &&
      props.limits.maxTessellationControlPerVertexOutputComponents == 0 &&
      props.limits.maxTessellationControlPerPatchOutputComponents == 0 &&
      props.limits.maxTessellationControlTotalOutputComponents == 0 &&
      props.limits.maxTessellationEvaluationInputComponents == 0 &&
      props.limits.maxTessellationEvaluationOutputComponents == 0;
   printf("tessellation_limits_zero %s\n", limits_zero ? "PASS" : "FAIL");

   const char *cases[] = {
      "tcs",
      "tes",
      "triangles",
      "quads",
      "isolines",
      "equal_spacing",
      "fractional_even",
      "fractional_odd",
      "cw",
      "ccw",
      "outer_levels",
      "inner_levels",
      "per_vertex_outputs",
      "per_patch_outputs",
      "barriers",
      "multiple_control_points",
   };
   for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
      printf("%s=NOT_RUN reason=tessellationShader_disabled\n", cases[i]);

   int failed = tess || tess_psiz || mv_tess || !tess_reject || !limits_zero;
   printf("result=%s\n", failed ? "FAIL" : "BLOCKED_SAFE_FALSE");
   vkDestroyInstance(instance, NULL);
   return failed;
}
