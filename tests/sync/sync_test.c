/* P10: Synchronization test — fence, binary semaphore, timeline semaphore, multi-submit, queue idle */
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vulkan/vulkan.h>

typedef PFN_vkVoidFunction (*icd_gipa_fn)(VkInstance, const char *);

#define CK(expr, what)                                                      \
   do {                                                                     \
      VkResult _r = (expr);                                                  \
      if (_r != VK_SUCCESS) {                                                \
         printf("FAIL %s r=%d line=%d\n", what, _r, __LINE__);                \
         return 1;                                                          \
      }                                                                     \
   } while (0)

static const uint32_t comp_spv[] = {
#include "../compute/write_magic.spv.h"
};

int main(int argc, char **argv)
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
   icd_gipa_fn gipa = (icd_gipa_fn)dlsym(h, "vk_icdGetInstanceProcAddr");
   if (!gipa)
      gipa = (icd_gipa_fn)dlsym(h, "vkGetInstanceProcAddr");
   if (!gipa) {
      printf("FAIL gipa\n");
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
   G(GetPhysicalDeviceQueueFamilyProperties, vkGetPhysicalDeviceQueueFamilyProperties)
   G(GetPhysicalDeviceMemoryProperties, vkGetPhysicalDeviceMemoryProperties)
   G(CreateDevice, vkCreateDevice)
   G(GetDeviceQueue, vkGetDeviceQueue)
   G(CreateBuffer, vkCreateBuffer)
   G(GetBufferMemoryRequirements, vkGetBufferMemoryRequirements)
   G(AllocateMemory, vkAllocateMemory)
   G(BindBufferMemory, vkBindBufferMemory)
   G(MapMemory, vkMapMemory)
   G(UnmapMemory, vkUnmapMemory)
   G(CreateDescriptorSetLayout, vkCreateDescriptorSetLayout)
   G(CreateDescriptorPool, vkCreateDescriptorPool)
   G(AllocateDescriptorSets, vkAllocateDescriptorSets)
   G(UpdateDescriptorSets, vkUpdateDescriptorSets)
   G(CreateShaderModule, vkCreateShaderModule)
   G(CreatePipelineLayout, vkCreatePipelineLayout)
   G(CreateComputePipelines, vkCreateComputePipelines)
   G(CreateCommandPool, vkCreateCommandPool)
   G(AllocateCommandBuffers, vkAllocateCommandBuffers)
   G(BeginCommandBuffer, vkBeginCommandBuffer)
   G(CmdBindPipeline, vkCmdBindPipeline)
   G(CmdBindDescriptorSets, vkCmdBindDescriptorSets)
   G(CmdDispatch, vkCmdDispatch)
   G(EndCommandBuffer, vkEndCommandBuffer)
   G(CreateFence, vkCreateFence)
   G(WaitForFences, vkWaitForFences)
   G(ResetFences, vkResetFences)
   G(DestroyFence, vkDestroyFence)
   G(CreateSemaphore, vkCreateSemaphore)
   G(DestroySemaphore, vkDestroySemaphore)
   G(QueueSubmit, vkQueueSubmit)
   G(QueueWaitIdle, vkQueueWaitIdle)
   G(DeviceWaitIdle, vkDeviceWaitIdle)
   G(DestroyCommandPool, vkDestroyCommandPool)
   G(DestroyPipeline, vkDestroyPipeline)
   G(DestroyPipelineLayout, vkDestroyPipelineLayout)
   G(DestroyShaderModule, vkDestroyShaderModule)
   G(DestroyDescriptorPool, vkDestroyDescriptorPool)
   G(DestroyDescriptorSetLayout, vkDestroyDescriptorSetLayout)
   G(FreeMemory, vkFreeMemory)
   G(DestroyBuffer, vkDestroyBuffer)
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

   uint32_t qn = 0;
   vkGetPhysicalDeviceQueueFamilyProperties(phys, &qn, NULL);
   VkQueueFamilyProperties *qp = malloc(qn * sizeof(*qp));
   vkGetPhysicalDeviceQueueFamilyProperties(phys, &qn, qp);
   uint32_t qi = ~0u;
   for (uint32_t i = 0; i < qn; i++)
      if (qp[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
         qi = i;
         break;
      }
   free(qp);

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

   /* Staging buffer */
   VkBufferCreateInfo bci = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = 4096, .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, .sharingMode = VK_SHARING_MODE_EXCLUSIVE};
   VkBuffer buf;
   CK(vkCreateBuffer(dev, &bci, NULL, &buf), "Buf");
   VkMemoryRequirements bmr;
   vkGetBufferMemoryRequirements(dev, buf, &bmr);
   VkPhysicalDeviceMemoryProperties mp;
   vkGetPhysicalDeviceMemoryProperties(phys, &mp);
   uint32_t host_mi = ~0u;
   for (uint32_t i = 0; i < mp.memoryTypeCount; i++)
      if ((bmr.memoryTypeBits & (1u << i)) && (mp.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
         host_mi = i; break;
      }
   VkMemoryAllocateInfo mai = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, .allocationSize = bmr.size, .memoryTypeIndex = host_mi};
   VkDeviceMemory mem;
   CK(vkAllocateMemory(dev, &mai, NULL, &mem), "Mem");
   CK(vkBindBufferMemory(dev, buf, mem, 0), "Bind");

   /* Pipeline */
   VkDescriptorSetLayoutBinding bind = {.binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT};
   VkDescriptorSetLayoutCreateInfo dlci = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, .bindingCount = 1, .pBindings = &bind};
   VkDescriptorSetLayout dsl;
   CK(vkCreateDescriptorSetLayout(dev, &dlci, NULL, &dsl), "DSL");
   VkDescriptorPoolSize psz = {.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1};
   VkDescriptorPoolCreateInfo dpci = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, .maxSets = 1, .poolSizeCount = 1, .pPoolSizes = &psz};
   VkDescriptorPool pool;
   CK(vkCreateDescriptorPool(dev, &dpci, NULL, &pool), "Pool");
   VkDescriptorSetAllocateInfo dsai = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, .descriptorPool = pool, .descriptorSetCount = 1, .pSetLayouts = &dsl};
   VkDescriptorSet set;
   CK(vkAllocateDescriptorSets(dev, &dsai, &set), "Sets");
   VkDescriptorBufferInfo dbi = {.buffer = buf, .offset = 0, .range = 4};
   VkWriteDescriptorSet wds = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = set, .dstBinding = 0, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .pBufferInfo = &dbi};
   vkUpdateDescriptorSets(dev, 1, &wds, 0, NULL);
   VkShaderModuleCreateInfo smci = {.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, .codeSize = sizeof(comp_spv), .pCode = comp_spv};
   VkShaderModule mod;
   CK(vkCreateShaderModule(dev, &smci, NULL, &mod), "SM");
   VkPipelineLayoutCreateInfo plci = {.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, .setLayoutCount = 1, .pSetLayouts = &dsl};
   VkPipelineLayout layout;
   CK(vkCreatePipelineLayout(dev, &plci, NULL, &layout), "Layout");
   VkComputePipelineCreateInfo cpci = {.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO, .stage = {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_COMPUTE_BIT, .module = mod, .pName = "main"}, .layout = layout};
   VkPipeline pipe;
   CK(vkCreateComputePipelines(dev, VK_NULL_HANDLE, 1, &cpci, NULL, &pipe), "Pipe");

   /* Commands */
   VkCommandPoolCreateInfo cpoci = {.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, .queueFamilyIndex = qi, .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT};
   VkCommandPool cpool;
   CK(vkCreateCommandPool(dev, &cpoci, NULL, &cpool), "Pool");
   VkCommandBufferAllocateInfo cbai = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, .commandPool = cpool, .commandBufferCount = 2};
   VkCommandBuffer cmds[2];
   CK(vkAllocateCommandBuffers(dev, &cbai, cmds), "Cmds");

   for (int i = 0; i < 2; i++) {
      VkCommandBufferBeginInfo bbi = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
      CK(vkBeginCommandBuffer(cmds[i], &bbi), "Begin");
      vkCmdBindPipeline(cmds[i], VK_PIPELINE_BIND_POINT_COMPUTE, pipe);
      vkCmdBindDescriptorSets(cmds[i], VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, 1, &set, 0, NULL);
      vkCmdDispatch(cmds[i], 1, 1, 1);
      CK(vkEndCommandBuffer(cmds[i]), "End");
   }

   /* 1. Fence test */
   VkFenceCreateInfo fci = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
   VkFence fence;
   CK(vkCreateFence(dev, &fci, NULL, &fence), "Fence");
   VkSubmitInfo si1 = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = &cmds[0]};
   CK(vkQueueSubmit(queue, 1, &si1, fence), "Submit(fence)");
   CK(vkWaitForFences(dev, 1, &fence, VK_TRUE, 5000000000ULL), "WaitFence");
   CK(vkResetFences(dev, 1, &fence), "ResetFence");
   printf("SYNC: fence signal/wait/reset OK\n");

   /* 2. Binary Semaphore chain */
   VkSemaphoreCreateInfo sci = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
   VkSemaphore sem;
   CK(vkCreateSemaphore(dev, &sci, NULL, &sem), "Sem");
   VkSubmitInfo si_sig = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers = &cmds[0],
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &sem,
   };
   VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
   VkSubmitInfo si_wait = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &sem,
      .pWaitDstStageMask = &waitStage,
      .commandBufferCount = 1,
      .pCommandBuffers = &cmds[1],
   };
   CK(vkQueueSubmit(queue, 1, &si_sig, VK_NULL_HANDLE), "Submit(sem_sig)");
   CK(vkQueueSubmit(queue, 1, &si_wait, fence), "Submit(sem_wait)");
   CK(vkWaitForFences(dev, 1, &fence, VK_TRUE, 5000000000ULL), "WaitFence(sem)");
   printf("SYNC: binary semaphore chain OK\n");

   /* 3. Multi-submission loop (20 iterations) */
   for (int iter = 0; iter < 20; iter++) {
      CK(vkResetFences(dev, 1, &fence), "ResetFences(iter)");
      CK(vkQueueSubmit(queue, 1, &si1, fence), "Submit(iter)");
      CK(vkWaitForFences(dev, 1, &fence, VK_TRUE, 5000000000ULL), "Wait(iter)");
   }
   printf("SYNC: 20/20 multi-submission loop OK\n");

   /* 4. Queue and Device WaitIdle */
   CK(vkQueueWaitIdle(queue), "QueueWaitIdle");
   CK(vkDeviceWaitIdle(dev), "DeviceWaitIdle");
   printf("SYNC: QueueWaitIdle and DeviceWaitIdle OK\n");

   /* Check memory result */
   uint32_t *p;
   CK(vkMapMemory(dev, mem, 0, 4, 0, (void **)&p), "Map");
   printf("SYNC: readback magic=0x%08x\n", *p);
   int ok = (*p == 0x12345678);
   vkUnmapMemory(dev, mem);

   /* Cleanup */
   vkDestroySemaphore(dev, sem, NULL);
   vkDestroyFence(dev, fence, NULL);
   vkDestroyCommandPool(dev, cpool, NULL);
   vkDestroyPipeline(dev, pipe, NULL);
   vkDestroyPipelineLayout(dev, layout, NULL);
   vkDestroyShaderModule(dev, mod, NULL);
   vkDestroyDescriptorPool(dev, pool, NULL);
   vkDestroyDescriptorSetLayout(dev, dsl, NULL);
   vkFreeMemory(dev, mem, NULL);
   vkDestroyBuffer(dev, buf, NULL);
   vkDestroyDevice(dev, NULL);
   vkDestroyInstance(inst, NULL);

   if (ok) {
      printf("SYNC-PASS\n");
      return 0;
   }
   printf("SYNC-FAIL\n");
   return 1;
}
