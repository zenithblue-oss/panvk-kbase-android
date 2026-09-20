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
static PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;

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
           uint32_t ext_count, const char *const *exts)
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
   LOAD(vkGetDeviceProcAddr);
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

   uint32_t ext_count = 0;
   vkEnumerateDeviceExtensionProperties(physical, NULL, &ext_count, NULL);
   VkExtensionProperties *exts = calloc(ext_count, sizeof(*exts));
   vkEnumerateDeviceExtensionProperties(physical, NULL, &ext_count, exts);
   int has_ext = 0;
   for (uint32_t i = 0; i < ext_count; i++) {
      if (strcmp(exts[i].extensionName, VK_EXT_TRANSFORM_FEEDBACK_EXTENSION_NAME) == 0)
         has_ext = 1;
   }
   free(exts);

   VkPhysicalDeviceTransformFeedbackFeaturesEXT xfb = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT,
   };
   VkPhysicalDeviceFeatures2 f2 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
      .pNext = &xfb,
   };
   vkGetPhysicalDeviceFeatures2(physical, &f2);

   VkPhysicalDeviceTransformFeedbackPropertiesEXT xfbp = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_PROPERTIES_EXT,
   };
   VkPhysicalDeviceProperties2 p2 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
      .pNext = &xfbp,
   };
   vkGetPhysicalDeviceProperties2(physical, &p2);

   int tf = xfb.transformFeedback == VK_TRUE;
   int gs = xfb.geometryStreams == VK_TRUE;
   int queries = xfbp.transformFeedbackQueries == VK_TRUE;
   int draw = xfbp.transformFeedbackDraw == VK_TRUE;
   int rast = xfbp.transformFeedbackRasterizationStreamSelect == VK_TRUE;
   int lines = xfbp.transformFeedbackStreamsLinesTriangles == VK_TRUE;

   printf("device=%s\n", props.deviceName);
   printf("api=%u.%u.%u\n", VK_VERSION_MAJOR(props.apiVersion),
          VK_VERSION_MINOR(props.apiVersion), VK_VERSION_PATCH(props.apiVersion));
   printf("VK_EXT_transform_feedback=%s\n", has_ext ? "YES" : "NO");
   printf("transformFeedback=%s\n", tf ? "YES" : "NO");
   printf("geometryStreams=%s\n", gs ? "YES" : "NO");
   printf("transformFeedbackQueries=%s\n", queries ? "YES" : "NO");
   printf("transformFeedbackDraw=%s\n", draw ? "YES" : "NO");
   printf("transformFeedbackRasterizationStreamSelect=%s\n", rast ? "YES" : "NO");
   printf("transformFeedbackStreamsLinesTriangles=%s\n", lines ? "YES" : "NO");
   printf("maxTransformFeedbackStreams=%u\n", xfbp.maxTransformFeedbackStreams);
   printf("maxTransformFeedbackBuffers=%u\n", xfbp.maxTransformFeedbackBuffers);
   printf("vertexPipelineStoresAndAtomics=%s\n",
          f2.features.vertexPipelineStoresAndAtomics ? "YES" : "NO");
   printf("geometryShader=%s\n", f2.features.geometryShader ? "YES" : "NO");

   const char *xfb_ext = VK_EXT_TRANSFORM_FEEDBACK_EXTENSION_NAME;
   VkResult ext_create = try_create(physical, family, NULL, 1, &xfb_ext);
   int ext_reject = ext_create == VK_ERROR_EXTENSION_NOT_PRESENT;
   printf("create_VK_EXT_transform_feedback=%s\n", vk_result_name(ext_create));
   printf("create_extension_rejected %s\n", ext_reject ? "PASS" : "FAIL");

   /* Without the extension, the XFB feature struct in pNext is ignored.
    * FEATURE_NOT_PRESENT is also acceptable. SUCCESS does not enable XFB. */
   VkPhysicalDeviceTransformFeedbackFeaturesEXT want = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT,
      .transformFeedback = VK_TRUE,
   };
   VkResult feat_create = try_create(physical, family, &want, 0, NULL);
   int feat_ignored = feat_create == VK_SUCCESS ||
                      feat_create == VK_ERROR_FEATURE_NOT_PRESENT ||
                      feat_create == VK_ERROR_EXTENSION_NOT_PRESENT;
   printf("create_transformFeedback_true=%s\n", vk_result_name(feat_create));
   printf("create_transformFeedback_pnext_without_ext %s\n",
          feat_ignored ? "PASS" : "FAIL");
   printf("note=pnext_ignored_unless_extension_enabled\n");

   VkPhysicalDeviceTransformFeedbackFeaturesEXT want_streams = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT,
      .geometryStreams = VK_TRUE,
   };
   VkResult streams_create = try_create(physical, family, &want_streams, 0, NULL);
   int streams_ignored = streams_create == VK_SUCCESS ||
                         streams_create == VK_ERROR_FEATURE_NOT_PRESENT ||
                         streams_create == VK_ERROR_EXTENSION_NOT_PRESENT;
   printf("create_geometryStreams_true=%s\n", vk_result_name(streams_create));
   printf("create_geometryStreams_pnext_without_ext %s\n",
          streams_ignored ? "PASS" : "FAIL");

   const char *cmds[] = {
      "vkCmdBindTransformFeedbackBuffersEXT",
      "vkCmdBeginTransformFeedbackEXT",
      "vkCmdEndTransformFeedbackEXT",
      "vkCmdDrawIndirectByteCountEXT",
   };
   for (size_t i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++)
      printf("%s=NOT_IMPLEMENTED\n", cmds[i]);

   const char *cases[] = {
      "single_buffer",
      "multiple_buffers",
      "offset",
      "size",
      "counter_buffer",
      "resume",
      "pause_end",
      "draw",
      "indexed_draw",
      "instanced_draw",
      "indirect_draw",
      "overflow",
      "barriers",
      "queries",
      "multiple_command_buffers",
   };
   for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
      printf("%s=NOT_RUN reason=transformFeedback_disabled\n", cases[i]);

   int extra_props = queries || draw || rast || lines ||
                     xfbp.maxTransformFeedbackStreams != 0 ||
                     xfbp.maxTransformFeedbackBuffers != 0;
   printf("extra_xfb_properties_off %s\n", extra_props ? "FAIL" : "PASS");

   int failed = has_ext || tf || gs || !ext_reject || !feat_ignored ||
                !streams_ignored || extra_props;
   printf("result=%s\n", failed ? "FAIL" : "BLOCKED_SAFE_FALSE");
   vkDestroyInstance(instance, NULL);
   return failed;
}
