/* Phase 5 focused synchronization workloads. Usage: sync-features <ICD.so> */
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

typedef PFN_vkVoidFunction (*icd_gipa_fn)(VkInstance, const char *);

#define CHECK(call, label) do { \
   VkResult r_ = (call); \
   if (r_ != VK_SUCCESS) { \
      printf("{\"feature\":\"%s\",\"status\":\"FAIL\",\"call\":\"%s\",\"result\":%d}\n", feature, label, r_); \
      return 1; \
   } \
} while (0)

#define LOAD(name) PFN_vk##name name = (PFN_vk##name)gipa(instance, "vk" #name); \
   if (!name) { printf("{\"feature\":\"setup\",\"status\":\"FAIL\",\"missing\":\"vk%s\"}\n", #name); return 1; }

int main(int argc, char **argv)
{
   const char *feature = "setup";
   if (argc != 2) {
      fprintf(stderr, "usage: %s <libvulkan_panfrost.so>\n", argv[0]);
      return 2;
   }
   setvbuf(stdout, NULL, _IONBF, 0);
   void *lib = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
   if (!lib) { fprintf(stderr, "dlopen: %s\n", dlerror()); return 1; }
   icd_gipa_fn gipa = (icd_gipa_fn)dlsym(lib, "vk_icdGetInstanceProcAddr");
   if (!gipa) gipa = (icd_gipa_fn)dlsym(lib, "vkGetInstanceProcAddr");
   if (!gipa) { fprintf(stderr, "missing vkGetInstanceProcAddr\n"); return 1; }
   PFN_vkCreateInstance CreateInstance = (PFN_vkCreateInstance)gipa(NULL, "vkCreateInstance");
   VkApplicationInfo ai = { .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                            .pApplicationName = "panvk-phase5-sync",
                            .apiVersion = VK_API_VERSION_1_3 };
   VkInstanceCreateInfo ici = { .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                                .pApplicationInfo = &ai };
   VkInstance instance;
   CHECK(CreateInstance(&ici, NULL, &instance), "vkCreateInstance");

   LOAD(EnumeratePhysicalDevices)
   LOAD(GetPhysicalDeviceProperties)
   LOAD(GetPhysicalDeviceQueueFamilyProperties)
   LOAD(GetPhysicalDeviceFeatures2)
   LOAD(CreateDevice)
   LOAD(GetDeviceQueue)
   LOAD(CreateCommandPool)
   LOAD(AllocateCommandBuffers)
   LOAD(BeginCommandBuffer)
   LOAD(EndCommandBuffer)
   LOAD(CmdPipelineBarrier2)
   LOAD(CreateSemaphore)
   LOAD(GetSemaphoreCounterValue)
   LOAD(WaitSemaphores)
   LOAD(SignalSemaphore)
   LOAD(QueueSubmit)
   LOAD(QueueSubmit2)
   LOAD(QueueWaitIdle)
   LOAD(DestroySemaphore)
   LOAD(DestroyCommandPool)
   LOAD(DestroyDevice)
   LOAD(DestroyInstance)

   uint32_t count = 0;
   CHECK(EnumeratePhysicalDevices(instance, &count, NULL), "vkEnumeratePhysicalDevices(count)");
   VkPhysicalDevice *physical_devices = calloc(count, sizeof(*physical_devices));
   CHECK(EnumeratePhysicalDevices(instance, &count, physical_devices), "vkEnumeratePhysicalDevices");
   VkPhysicalDevice physical = VK_NULL_HANDLE;
   VkPhysicalDeviceProperties props = {0};
   for (uint32_t i = 0; i < count; i++) {
      GetPhysicalDeviceProperties(physical_devices[i], &props);
      if (strstr(props.deviceName, "Mali")) { physical = physical_devices[i]; break; }
   }
   free(physical_devices);
   if (!physical) { printf("{\"feature\":\"setup\",\"status\":\"FAIL\",\"reason\":\"no Mali physical device\"}\n"); return 1; }

   uint32_t queue_count = 0;
   GetPhysicalDeviceQueueFamilyProperties(physical, &queue_count, NULL);
   VkQueueFamilyProperties *queues = calloc(queue_count, sizeof(*queues));
   GetPhysicalDeviceQueueFamilyProperties(physical, &queue_count, queues);
   uint32_t family = UINT32_MAX;
   for (uint32_t i = 0; i < queue_count; i++)
      if (queues[i].queueFlags & (VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT)) { family = i; break; }
   free(queues);
   if (family == UINT32_MAX) { printf("{\"feature\":\"setup\",\"status\":\"FAIL\",\"reason\":\"no queue\"}\n"); return 1; }

   VkPhysicalDeviceTimelineSemaphoreFeatures timeline = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES };
   VkPhysicalDeviceSynchronization2Features sync2 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
      .pNext = &timeline };
   VkPhysicalDeviceFeatures2 features = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
                                          .pNext = &sync2 };
   GetPhysicalDeviceFeatures2(physical, &features);
   if (!timeline.timelineSemaphore || !sync2.synchronization2) {
      printf("{\"feature\":\"setup\",\"status\":\"FAIL\",\"timelineSemaphore\":%u,\"synchronization2\":%u}\n",
             timeline.timelineSemaphore, sync2.synchronization2);
      return 1;
   }
   float priority = 1.0f;
   VkDeviceQueueCreateInfo qci = { .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                   .queueFamilyIndex = family, .queueCount = 1,
                                   .pQueuePriorities = &priority };
   VkDeviceCreateInfo dci = { .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                              .pNext = &sync2, .queueCreateInfoCount = 1,
                              .pQueueCreateInfos = &qci };
   VkDevice device;
   CHECK(CreateDevice(physical, &dci, NULL, &device), "vkCreateDevice");
   VkQueue queue;
   GetDeviceQueue(device, family, 0, &queue);

   VkCommandPoolCreateInfo cpci = { .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                                    .queueFamilyIndex = family };
   VkCommandPool pool;
   CHECK(CreateCommandPool(device, &cpci, NULL, &pool), "vkCreateCommandPool");
   VkCommandBufferAllocateInfo cbai = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                        .commandPool = pool, .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                        .commandBufferCount = 1 };
   VkCommandBuffer cmd;
   CHECK(AllocateCommandBuffers(device, &cbai, &cmd), "vkAllocateCommandBuffers");
   VkCommandBufferBeginInfo cbbi = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
   CHECK(BeginCommandBuffer(cmd, &cbbi), "vkBeginCommandBuffer");
   VkMemoryBarrier2 barrier = { .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
                                .srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
                                .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                .dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT };
   VkDependencyInfo dependency = { .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                                   .memoryBarrierCount = 1, .pMemoryBarriers = &barrier };
   CmdPipelineBarrier2(cmd, &dependency);
   CHECK(EndCommandBuffer(cmd), "vkEndCommandBuffer");

   feature = "timelineSemaphore";
   VkSemaphoreTypeCreateInfo type = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
                                      .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
                                      .initialValue = 0 };
   VkSemaphoreCreateInfo sci = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                                 .pNext = &type };
   VkSemaphore semaphore;
   CHECK(CreateSemaphore(device, &sci, NULL, &semaphore), "vkCreateSemaphore");
   for (uint64_t value = 1; value <= 64; value++) {
      uint64_t wait_value = value - 1;
      VkTimelineSemaphoreSubmitInfo tsi = { .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
                                            .waitSemaphoreValueCount = value > 1,
                                            .pWaitSemaphoreValues = &wait_value,
                                            .signalSemaphoreValueCount = 1,
                                            .pSignalSemaphoreValues = &value };
      VkPipelineStageFlags stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
      VkSubmitInfo submit = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .pNext = &tsi,
                              .waitSemaphoreCount = value > 1,
                              .pWaitSemaphores = &semaphore, .pWaitDstStageMask = &stage,
                              .signalSemaphoreCount = 1, .pSignalSemaphores = &semaphore };
      CHECK(QueueSubmit(queue, 1, &submit, VK_NULL_HANDLE), "vkQueueSubmit(timeline)");
   }
   uint64_t value64 = 64;
   VkSemaphoreWaitInfo wi = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
                              .semaphoreCount = 1, .pSemaphores = &semaphore,
                              .pValues = &value64 };
   CHECK(WaitSemaphores(device, &wi, 5000000000ULL), "vkWaitSemaphores(64)");
   uint64_t cpu_value = 65;
   VkSemaphoreSignalInfo signal = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
                                    .semaphore = semaphore, .value = cpu_value };
   CHECK(SignalSemaphore(device, &signal), "vkSignalSemaphore(65)");
   uint64_t gpu_value = 66;
   VkTimelineSemaphoreSubmitInfo tsi = { .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
                                         .waitSemaphoreValueCount = 1, .pWaitSemaphoreValues = &cpu_value,
                                         .signalSemaphoreValueCount = 1, .pSignalSemaphoreValues = &gpu_value };
   VkPipelineStageFlags stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
   VkSubmitInfo submit = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .pNext = &tsi,
                           .waitSemaphoreCount = 1, .pWaitSemaphores = &semaphore,
                           .pWaitDstStageMask = &stage, .signalSemaphoreCount = 1,
                           .pSignalSemaphores = &semaphore };
   CHECK(QueueSubmit(queue, 1, &submit, VK_NULL_HANDLE), "vkQueueSubmit(cpu-to-gpu)");
   wi.pValues = &gpu_value;
   CHECK(WaitSemaphores(device, &wi, 5000000000ULL), "vkWaitSemaphores(66)");
   uint64_t counter = 0;
   CHECK(GetSemaphoreCounterValue(device, semaphore, &counter), "vkGetSemaphoreCounterValue");
   if (counter != 66) { printf("{\"feature\":\"timelineSemaphore\",\"status\":\"FAIL\",\"counter\":%llu}\n", (unsigned long long)counter); return 1; }
   printf("{\"feature\":\"timelineSemaphore\",\"status\":\"PASS\",\"gpu_chain\":\"1..64\",\"cpu_wait\":64,\"cpu_signal\":65,\"counter\":66}\n");

   feature = "synchronization2";
   VkCommandBufferSubmitInfo cbsi = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                                      .commandBuffer = cmd };
   VkSemaphoreSubmitInfo signal2 = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                                     .semaphore = semaphore, .value = 67,
                                     .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT };
   VkSubmitInfo2 submit2 = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                             .commandBufferInfoCount = 1, .pCommandBufferInfos = &cbsi,
                             .signalSemaphoreInfoCount = 1, .pSignalSemaphoreInfos = &signal2 };
   CHECK(QueueSubmit2(queue, 1, &submit2, VK_NULL_HANDLE), "vkQueueSubmit2");
   uint64_t value67 = 67;
   wi.pValues = &value67;
   CHECK(WaitSemaphores(device, &wi, 5000000000ULL), "vkWaitSemaphores(67)");
   printf("{\"feature\":\"synchronization2\",\"status\":\"PASS\",\"barrier2\":true,\"submit2\":true,\"commandBufferSubmitInfo\":true,\"semaphoreSubmitInfo\":true}\n");

   CHECK(QueueWaitIdle(queue), "vkQueueWaitIdle");
   DestroySemaphore(device, semaphore, NULL);
   DestroyCommandPool(device, pool, NULL);
   DestroyDevice(device, NULL);
   DestroyInstance(instance, NULL);
   dlclose(lib);
   return 0;
}
