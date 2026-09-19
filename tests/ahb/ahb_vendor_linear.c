/* Gate G: real AHardwareBuffer allocate -> import -> bind -> GPU clear ->
 * CPU lock/readback. PASS: pixel read via AHB_lock matches GPU clear color.
 * Usage: ahb <libvulkan_panfrost.so>
 */
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define VK_USE_PLATFORM_ANDROID_KHR 1
#include <vulkan/vulkan.h>
#include <android/hardware_buffer.h>

typedef PFN_vkVoidFunction (*icd_gipa_fn)(VkInstance, const char *);

#define CK(expr, what)                                                      \
   do {                                                                     \
      VkResult _r = (expr);                                                  \
      if (_r != VK_SUCCESS) {                                                \
         printf("FAIL %s r=%d line=%d\n", what, _r, __LINE__);                \
         return 1;                                                          \
      }                                                                     \
   } while (0)

int
main(int argc, char **argv)
{
   if (argc < 2) {
      printf("usage: %s <libvulkan_panfrost.so>\n", argv[0]);
      return 2;
   }
   setvbuf(stdout, NULL, _IONBF, 0);
   void *h = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
   if (!h) {
      printf("FAIL dlopen %s\n", dlerror());
      return 1;
   }
   /* system loader path: vkGetInstanceProcAddr has the same
    * (instance,name) shape as vk_icdGetInstanceProcAddr, so the G()/G0()
    * macros work unchanged for the vendor stack too */
   icd_gipa_fn gipa = (icd_gipa_fn)dlsym(h, "vkGetInstanceProcAddr");
   if (!gipa)
      gipa = (icd_gipa_fn)dlsym(h, "vk_icdGetInstanceProcAddr");
   if (!gipa) {
      printf("FAIL gpa\n");
      return 1;
   }
#define G0(what, name)                                                      \
   PFN_##name name = (PFN_##name)gipa(NULL, "vk" #what);                     \
   if (!name) {                                                             \
      printf("FAIL missing vk" #what "\n");                                  \
      return 1;                                                             \
   }
   G0(CreateInstance, vkCreateInstance)
   VkApplicationInfo app = {.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                            .apiVersion = VK_API_VERSION_1_3};
   VkInstanceCreateInfo ici = {.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                               .pApplicationInfo = &app};
   VkInstance inst;
   CK(vkCreateInstance(&ici, NULL, &inst), "CreateInstance");
#undef G0
#define G(what, name)                                                       \
   PFN_##name name = (PFN_##name)gipa(inst, "vk" #what);                     \
   if (!name) {                                                             \
      printf("FAIL missing vk" #what "\n");                                  \
      return 1;                                                             \
   }
   G(EnumeratePhysicalDevices, vkEnumeratePhysicalDevices)
   G(GetPhysicalDeviceProperties, vkGetPhysicalDeviceProperties)
   G(GetPhysicalDeviceQueueFamilyProperties,
     vkGetPhysicalDeviceQueueFamilyProperties)
   G(CreateDevice, vkCreateDevice)
   G(GetDeviceQueue, vkGetDeviceQueue)
   G(CreateImage, vkCreateImage)
   G(GetImageMemoryRequirements2, vkGetImageMemoryRequirements2)
   G(GetPhysicalDeviceMemoryProperties, vkGetPhysicalDeviceMemoryProperties)
   G(AllocateMemory, vkAllocateMemory)
   G(BindImageMemory2, vkBindImageMemory2)
   G(GetMemoryFdPropertiesKHR, vkGetMemoryFdPropertiesKHR)
   G(CreateCommandPool, vkCreateCommandPool)
   G(AllocateCommandBuffers, vkAllocateCommandBuffers)
   G(BeginCommandBuffer, vkBeginCommandBuffer)
   G(ResetCommandBuffer, vkResetCommandBuffer)
   G(CmdClearColorImage, vkCmdClearColorImage)
   G(CmdPipelineBarrier, vkCmdPipelineBarrier)
   G(EndCommandBuffer, vkEndCommandBuffer)
   G(CreateFence, vkCreateFence)
   G(QueueSubmit, vkQueueSubmit)
   G(WaitForFences, vkWaitForFences)
   G(ResetFences, vkResetFences)
   G(DestroyFence, vkDestroyFence)
   G(FreeCommandBuffers, vkFreeCommandBuffers)
   G(DestroyCommandPool, vkDestroyCommandPool)
   G(DestroyImage, vkDestroyImage)
   G(FreeMemory, vkFreeMemory)
   G(DestroyDevice, vkDestroyDevice)
   G(DestroyInstance, vkDestroyInstance)

   uint32_t n = 0;
   CK(vkEnumeratePhysicalDevices(inst, &n, NULL), "count");
   VkPhysicalDevice *devs = malloc(n * sizeof(*devs));
   CK(vkEnumeratePhysicalDevices(inst, &n, devs), "enum");
   VkPhysicalDevice phys = VK_NULL_HANDLE;
   for (uint32_t i = 0; i < n; i++) {
      VkPhysicalDeviceProperties p;
      vkGetPhysicalDeviceProperties(devs[i], &p);
      if (strstr(p.deviceName, "Mali"))
         phys = devs[i];
   }
   free(devs);
   if (phys == VK_NULL_HANDLE) {
      printf("FAIL no Mali\n");
      return 1;
   }

   uint32_t qn = 0;
   vkGetPhysicalDeviceQueueFamilyProperties(phys, &qn, NULL);
   VkQueueFamilyProperties *qp = malloc(qn * sizeof(*qp));
   vkGetPhysicalDeviceQueueFamilyProperties(phys, &qn, qp);
   uint32_t qi = ~0u;
   for (uint32_t i = 0; i < qn; i++)
      if (qp[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
         qi = i;
         break;
      }
   free(qp);
   if (qi == ~0u) {
      printf("FAIL no graphics queue\n");
      return 1;
   }
   float prio = 1.0f;
   VkDeviceQueueCreateInfo qci = {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                  .queueFamilyIndex = qi,
                                  .queueCount = 1,
                                  .pQueuePriorities = &prio};
   VkDeviceCreateInfo dci = {.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                             .queueCreateInfoCount = 1,
                             .pQueueCreateInfos = &qci};
   VkDevice dev;
   CK(vkCreateDevice(phys, &dci, NULL, &dev), "CreateDevice");
   VkQueue queue;
   vkGetDeviceQueue(dev, qi, 0, &queue);
   /* mode: default GPU-clear; argv[2]=="read" means CPU-fill then
    * GPU copy image->staging buffer (tests GPU read of the AHB). */
   int gpu_write = !(argc > 2 && !strcmp(argv[2], "read"));
   int gpu_read = (argc > 2 && !strcmp(argv[2], "read"));

   /* real AHB */
   AHardwareBuffer_Desc desc = {.width = 64,
                                .height = 64,
                                .layers = 1,
                                .format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM,
                                .usage = gpu_write ?
                                   (AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT |
                                    AHARDWAREBUFFER_USAGE_CPU_READ_RARELY) :
                                   (AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE |
                                    AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN),
                                .stride = 0};
   AHardwareBuffer *ahb = NULL;
   int ar = AHardwareBuffer_allocate(&desc, &ahb);
   if (ar != 0 || !ahb) {
      printf("FAIL AHB allocate r=%d\n", ar);
      return 1;
   }
   printf("G-AHB allocated\n");
   AHardwareBuffer_Desc out;
   AHardwareBuffer_describe(ahb, &out);
   printf("G-AHB stride=%u\n", out.stride);

   /* image bound to the AHB */
   VkExternalMemoryImageCreateInfo ext = {
      .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,
      .handleTypes =
         VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID};
   VkImageCreateInfo ii = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                           .pNext = &ext,
                           .imageType = VK_IMAGE_TYPE_2D,
                           .format = VK_FORMAT_R8G8B8A8_UNORM,
                           .extent = {64, 64, 1},
                           .mipLevels = 1,
                           .arrayLayers = 1,
                           .samples = VK_SAMPLE_COUNT_1_BIT,
                           .tiling = VK_IMAGE_TILING_OPTIMAL,
                           .usage = gpu_write ?
                              (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                               VK_IMAGE_USAGE_TRANSFER_DST_BIT) :
                              (VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                               VK_IMAGE_USAGE_SAMPLED_BIT),
                           .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                           .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};
   VkImage img;
   CK(vkCreateImage(dev, &ii, NULL, &img), "Image");

   VkImageMemoryRequirementsInfo2 ri = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2,
      .image = img};
   VkMemoryRequirements2 r2 = {.sType =
                                  VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
   VkMemoryDedicatedRequirements dr = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS};
   r2.pNext = &dr;
   vkGetImageMemoryRequirements2(dev, &ri, &r2);
   printf("G-reqs size=%llu dedicated=%u\n",
          (unsigned long long)r2.memoryRequirements.size,
          dr.requiresDedicatedAllocation);

   VkImportAndroidHardwareBufferInfoANDROID imp = {
      .sType = VK_STRUCTURE_TYPE_IMPORT_ANDROID_HARDWARE_BUFFER_INFO_ANDROID,
      .buffer = ahb};
   VkMemoryDedicatedAllocateInfo dai = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO,
      .image = img,
      .pNext = &imp};
   VkMemoryAllocateInfo mai = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                               .pNext = &dai,
                               .allocationSize = r2.memoryRequirements.size,
                               .memoryTypeIndex = 0};
   {
      VkPhysicalDeviceMemoryProperties mprop;
      vkGetPhysicalDeviceMemoryProperties(phys, &mprop);
      uint32_t picked = ~0u;
      for (uint32_t i = 0; i < mprop.memoryTypeCount; i++)
         if (r2.memoryRequirements.memoryTypeBits & (1u << i)) {
            picked = i;
            break;
         }
      if (picked == ~0u) {
         printf("FAIL no memory type\n");
         return 1;
      }
      mai.memoryTypeIndex = picked;
   }
   VkDeviceMemory mem;
   VkResult air = vkAllocateMemory(dev, &mai, NULL, &mem);
   if (air != VK_SUCCESS) {
      printf("FAIL AllocateMemory(import) r=%d\n", air);
      return 1;
   }
   printf("G-import OK\n");
   VkBindImageMemoryInfo bii = {.sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO,
                                .image = img,
                                .memory = mem,
                                .memoryOffset = 0};
   CK(vkBindImageMemory2(dev, 1, &bii), "Bind2");

   /* GPU clear to green */
   VkCommandPoolCreateInfo cpoci = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .queueFamilyIndex = qi};
   VkCommandPool cpool;
   CK(vkCreateCommandPool(dev, &cpoci, NULL, &cpool), "Pool");
   VkCommandBufferAllocateInfo cbai = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = cpool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1};
   VkCommandBuffer cmd;
   CK(vkAllocateCommandBuffers(dev, &cbai, &cmd), "Cmd");
   VkFenceCreateInfo fci = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
   VkFence fence;
   CK(vkCreateFence(dev, &fci, NULL, &fence), "Fence");
   VkCommandBufferBeginInfo bbi = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
   CK(vkResetCommandBuffer(cmd, 0), "Reset");
   CK(vkBeginCommandBuffer(cmd, &bbi), "Begin");
   VkImageMemoryBarrier b1 = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                              .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                              .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                              .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                              .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                              .image = img,
                              .subresourceRange = {.aspectMask =
                                                      VK_IMAGE_ASPECT_COLOR_BIT,
                                                   .levelCount = 1,
                                                   .layerCount = 1}};
   vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1,
                        &b1);
   VkClearColorValue green = {.float32 = {0.0f, 1.0f, 0.0f, 1.0f}};
   VkImageSubresourceRange range = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                    .levelCount = 1,
                                    .layerCount = 1};
   vkCmdClearColorImage(cmd, img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        &green, 1, &range);
   VkImageMemoryBarrier b2 = b1;
   b2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
   b2.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
   b2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
   b2.newLayout = VK_IMAGE_LAYOUT_GENERAL;
   vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VK_PIPELINE_STAGE_HOST_BIT, 0, 0, NULL, 0, NULL, 1,
                        &b2);
   CK(vkEndCommandBuffer(cmd), "End");
   VkSubmitInfo si = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                      .commandBufferCount = 1,
                      .pCommandBuffers = &cmd};
   /* fault-in backing pages via CPU before GPU access (lazy-heap test) */
   {
      void *pre = NULL;
      if (AHardwareBuffer_lock(ahb, AHARDWAREBUFFER_USAGE_CPU_READ_RARELY,
                               -1, NULL, &pre) == 0 && pre) {
         volatile uint8_t x = ((volatile uint8_t *)pre)[0];
         ((volatile uint8_t *)pre)[0] = x;
         AHardwareBuffer_unlock(ahb, NULL);
         printf("G-prefault done\n");
      }
   }
   CK(vkQueueSubmit(queue, 1, &si, fence), "Submit");
   VkResult wr = vkWaitForFences(dev, 1, &fence, VK_TRUE,
                                 30 * 1000 * 1000 * 1000ULL);
   printf("G-wait r=%d\n", wr);

   /* CPU readback via AHB lock (always attempt: shows pre-existing bytes
    * even when the GPU job faulted) */
   void *data = NULL;
   int32_t lock_r = AHardwareBuffer_lock(ahb, AHARDWAREBUFFER_USAGE_CPU_READ_RARELY,
                                         -1, NULL, &data);
   if (lock_r != 0 || !data) {
      printf("FAIL AHB lock r=%d\n", lock_r);
      return 1;
   }
   uint8_t *px = data;
   /* stride may exceed width*4 */
   uint8_t *p0 = px + (8 * out.stride + 8) * 4;
   printf("G-locked RGBA=%u %u %u %u (stride px=%u)\n", p0[0], p0[1], p0[2],
          p0[3], out.stride);
   int ok = (p0[1] > 200 && p0[0] < 60 && p0[2] < 60);
   AHardwareBuffer_unlock(ahb, NULL);
   AHardwareBuffer_release(ahb);

   vkDestroyFence(dev, fence, NULL);
   vkFreeCommandBuffers(dev, cpool, 1, &cmd);
   vkDestroyCommandPool(dev, cpool, NULL);
   vkFreeMemory(dev, mem, NULL);
   vkDestroyImage(dev, img, NULL);
   vkDestroyDevice(dev, NULL);
   vkDestroyInstance(inst, NULL);
   if (ok && wr == VK_SUCCESS) {
      printf("G-PASS\n");
      return 0;
   }
   printf("G-FAIL\n");
   return 1;
}
