/* fd-import test: dma_heap buffer -> vkImportMemoryFdKHR as storage buffer
 * memory -> compute write magic -> readback. Splits heap-vs-path for the
 * MTK AHB import failure.
 * Usage: fdimport <libvulkan_panfrost.so>
 */
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <android/hardware_buffer.h>
#include <vulkan/vulkan.h>

typedef PFN_vkVoidFunction (*icd_gipa_fn)(VkInstance, const char *);
typedef struct { int version, numFds, numInts, data[0]; } native_handle_t;
typedef struct AHardwareBuffer AHardwareBuffer;
const native_handle_t *AHardwareBuffer_getNativeHandle(const AHardwareBuffer *b);

#define CK(expr, what)                                                      \
   do {                                                                     \
      VkResult _r = (expr);                                                  \
      if (_r != VK_SUCCESS) {                                                \
         printf("FAIL %s r=%d line=%d\n", what, _r, __LINE__);                \
         return 1;                                                          \
      }                                                                     \
   } while (0)

static const uint32_t comp_spv[] = {
#include "write_magic.spv.h"
};

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
   icd_gipa_fn gipa = (icd_gipa_fn)dlsym(h, "vk_icdGetInstanceProcAddr");
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
   G(GetPhysicalDeviceQueueFamilyProperties,
     vkGetPhysicalDeviceQueueFamilyProperties)
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
   G(ResetCommandBuffer, vkResetCommandBuffer)
   G(CmdBindPipeline, vkCmdBindPipeline)
   G(CmdBindDescriptorSets, vkCmdBindDescriptorSets)
   G(CmdDispatch, vkCmdDispatch)
   G(EndCommandBuffer, vkEndCommandBuffer)
   G(CreateFence, vkCreateFence)
   G(QueueSubmit, vkQueueSubmit)
   G(WaitForFences, vkWaitForFences)
   G(ResetFences, vkResetFences)
   G(DestroyFence, vkDestroyFence)
   G(FreeCommandBuffers, vkFreeCommandBuffers)
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
      if (qp[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
         qi = i;
         break;
      }
   free(qp);
   if (qi == ~0u) {
      printf("FAIL no compute queue\n");
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

   /* MTK AHB pixel fd (fd index 1) */
   AHardwareBuffer_Desc adesc = {.width = 64, .height = 64, .layers = 1,
      .format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM,
      .usage = AHARDWAREBUFFER_USAGE_GPU_DATA_BUFFER |
               AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN};
   AHardwareBuffer *aahb = NULL;
   if (AHardwareBuffer_allocate(&adesc, &aahb) || !aahb) {
      printf("FAIL AHB\n");
      return 1;
   }
   const native_handle_t *ah = AHardwareBuffer_getNativeHandle(aahb);
   printf("I-ahb fds=%d\n", ah->numFds);
   struct { int fd; } ad;
   ad.fd = ah->data[1];

   VkBufferCreateInfo bci = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                             .size = 4096,
                             .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                             .sharingMode = VK_SHARING_MODE_EXCLUSIVE};
   VkBuffer buf;
   CK(vkCreateBuffer(dev, &bci, NULL, &buf), "Buffer");
   VkMemoryRequirements mr;
   vkGetBufferMemoryRequirements(dev, buf, &mr);
   VkPhysicalDeviceMemoryProperties mp;
   vkGetPhysicalDeviceMemoryProperties(phys, &mp);
   uint32_t mi = 0;
   for (uint32_t i = 0; i < mp.memoryTypeCount; i++)
      if (mr.memoryTypeBits & (1u << i)) {
         mi = i;
         break;
      }
   VkImportMemoryFdInfoKHR imp = {.sType =
                                     VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR,
                                  .handleType =
                                     VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT,
                                  .fd = ad.fd};
   VkMemoryAllocateInfo mai = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                               .pNext = &imp,
                               .allocationSize = 4096,
                               .memoryTypeIndex = mi};
   VkDeviceMemory mem;
   CK(vkAllocateMemory(dev, &mai, NULL, &mem), "Import");
   printf("I-import OK\n");
   CK(vkBindBufferMemory(dev, buf, mem, 0), "Bind");

   VkDescriptorSetLayoutBinding bind = {.binding = 0,
                                        .descriptorType =
                                           VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                        .descriptorCount = 1,
                                        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT};
   VkDescriptorSetLayoutCreateInfo dlci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = 1,
      .pBindings = &bind};
   VkDescriptorSetLayout dsl;
   CK(vkCreateDescriptorSetLayout(dev, &dlci, NULL, &dsl), "DSL");
   VkDescriptorPoolSize psz = {.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                               .descriptorCount = 1};
   VkDescriptorPoolCreateInfo dpci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = 1,
      .poolSizeCount = 1,
      .pPoolSizes = &psz};
   VkDescriptorPool pool;
   CK(vkCreateDescriptorPool(dev, &dpci, NULL, &pool), "Pool");
   VkDescriptorSetAllocateInfo dsai = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = pool,
      .descriptorSetCount = 1,
      .pSetLayouts = &dsl};
   VkDescriptorSet set;
   CK(vkAllocateDescriptorSets(dev, &dsai, &set), "Sets");
   VkDescriptorBufferInfo dbi = {.buffer = buf, .offset = 0, .range = 4};
   VkWriteDescriptorSet wds = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                               .dstSet = set,
                               .dstBinding = 0,
                               .descriptorCount = 1,
                               .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                               .pBufferInfo = &dbi};
   vkUpdateDescriptorSets(dev, 1, &wds, 0, NULL);
   VkShaderModuleCreateInfo smci = {.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                                    .codeSize = sizeof(comp_spv),
                                    .pCode = comp_spv};
   VkShaderModule mod;
   CK(vkCreateShaderModule(dev, &smci, NULL, &mod), "SM");
   VkPipelineLayoutCreateInfo plci = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1,
      .pSetLayouts = &dsl};
   VkPipelineLayout layout;
   CK(vkCreatePipelineLayout(dev, &plci, NULL, &layout), "Layout");
   VkComputePipelineCreateInfo cpci = {
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      .stage = {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_COMPUTE_BIT,
                .module = mod,
                .pName = "main"},
      .layout = layout};
   VkPipeline pipe;
   CK(vkCreateComputePipelines(dev, VK_NULL_HANDLE, 1, &cpci, NULL, &pipe),
      "Pipe");
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

   void *p;
   CK(vkMapMemory(dev, mem, 0, 4, 0, &p), "Map");
   *(volatile uint32_t *)p = 0u;
   vkUnmapMemory(dev, mem);
   VkCommandBufferBeginInfo bbi = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
   CK(vkResetCommandBuffer(cmd, 0), "Reset");
   CK(vkBeginCommandBuffer(cmd, &bbi), "Begin");
   vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipe);
   vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, 1,
                           &set, 0, NULL);
   vkCmdDispatch(cmd, 1, 1, 1);
   CK(vkEndCommandBuffer(cmd), "End");
   VkSubmitInfo si = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                      .commandBufferCount = 1,
                      .pCommandBuffers = &cmd};
   CK(vkQueueSubmit(queue, 1, &si, fence), "Submit");
   VkResult wr = vkWaitForFences(dev, 1, &fence, VK_TRUE,
                                 30 * 1000 * 1000 * 1000ULL);
   printf("I-wait r=%d\n", wr);
   int pass = 0;
   if (wr == VK_SUCCESS) {
      CK(vkMapMemory(dev, mem, 0, 4, 0, &p), "Map2");
      uint32_t v = *(volatile uint32_t *)p;
      vkUnmapMemory(dev, mem);
      printf("I-value 0x%08x\n", v);
      pass = (v == 0x12345678u);
   }
   vkDestroyFence(dev, fence, NULL);
   vkFreeCommandBuffers(dev, cpool, 1, &cmd);
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
   if (pass) {
      printf("I-PASS\n");
      return 0;
   }
   printf("I-FAIL\n");
   return 1;
}
