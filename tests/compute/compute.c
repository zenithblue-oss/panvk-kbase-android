/* Gates D+E: device create, queue, mapped buffer, deterministic compute.
 * Dispatches 1x1x1 compute writing 0x12345678; repeats 10x. PASS = 10/10.
 * Usage: compute <libvulkan_panfrost.so>
 */
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

/* Generated from write_magic.comp via glslangValidator (see below). */
static const uint32_t comp_spv[] = {
#include "write_magic.spv.h"
};

static const uint32_t nop_spv[] = {
#include "nop.spv.h"
};


int
main(int argc, char **argv)
{
   if (argc < 2) {
      printf("usage: %s <libvulkan_panfrost.so> [nop]\n", argv[0]);
      return 2;
   }
   int use_buf = !(argc > 2 && !strcmp(argv[2], "nop"));
   setvbuf(stdout, NULL, _IONBF, 0);
   const uint32_t *spv = use_buf ? comp_spv : nop_spv;
   size_t spv_size = use_buf ? sizeof(comp_spv) : sizeof(nop_spv);
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
#define GIPA(what, name)                                                    \
   PFN_##name name = (PFN_##name)gipa(NULL, "vk" #what);                     \
   if (!name) {                                                             \
      printf("FAIL missing vk" #what "\n");                                  \
      return 1;                                                             \
   }
   GIPA(CreateInstance, vkCreateInstance)

   VkApplicationInfo app = {.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                            .apiVersion = VK_API_VERSION_1_3};
   VkInstanceCreateInfo ici = {.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                               .pApplicationInfo = &app};
   VkInstance inst;
   CK(vkCreateInstance(&ici, NULL, &inst), "CreateInstance");
   fprintf(stderr, "T-after-CreateInstance\n");

#undef GIPA
#define GIPA(what, name)                                                    \
   PFN_##name name = (PFN_##name)gipa(inst, "vk" #what);                     \
   if (!name) {                                                             \
      printf("FAIL missing vk" #what "\n");                                  \
      return 1;                                                             \
   }
   GIPA(EnumeratePhysicalDevices, vkEnumeratePhysicalDevices)
   GIPA(GetPhysicalDeviceProperties, vkGetPhysicalDeviceProperties)
   GIPA(GetPhysicalDeviceQueueFamilyProperties,
        vkGetPhysicalDeviceQueueFamilyProperties)
   GIPA(GetPhysicalDeviceMemoryProperties, vkGetPhysicalDeviceMemoryProperties)
   GIPA(CreateDevice, vkCreateDevice)
   GIPA(GetDeviceQueue, vkGetDeviceQueue)
   GIPA(CreateBuffer, vkCreateBuffer)
   GIPA(GetBufferMemoryRequirements, vkGetBufferMemoryRequirements)
   GIPA(AllocateMemory, vkAllocateMemory)
   GIPA(BindBufferMemory, vkBindBufferMemory)
   GIPA(MapMemory, vkMapMemory)
   GIPA(UnmapMemory, vkUnmapMemory)
   GIPA(CreateDescriptorSetLayout, vkCreateDescriptorSetLayout)
   GIPA(CreateDescriptorPool, vkCreateDescriptorPool)
   GIPA(AllocateDescriptorSets, vkAllocateDescriptorSets)
   GIPA(UpdateDescriptorSets, vkUpdateDescriptorSets)
   GIPA(CreateShaderModule, vkCreateShaderModule)
   GIPA(CreatePipelineLayout, vkCreatePipelineLayout)
   GIPA(CreateComputePipelines, vkCreateComputePipelines)
   GIPA(CreateCommandPool, vkCreateCommandPool)
   GIPA(AllocateCommandBuffers, vkAllocateCommandBuffers)
   GIPA(BeginCommandBuffer, vkBeginCommandBuffer)
   GIPA(ResetCommandBuffer, vkResetCommandBuffer)
   GIPA(CmdBindPipeline, vkCmdBindPipeline)
   GIPA(CmdBindDescriptorSets, vkCmdBindDescriptorSets)
   GIPA(CmdDispatch, vkCmdDispatch)
   GIPA(EndCommandBuffer, vkEndCommandBuffer)
   GIPA(CreateFence, vkCreateFence)
   GIPA(QueueSubmit, vkQueueSubmit)
   GIPA(WaitForFences, vkWaitForFences)
   GIPA(ResetFences, vkResetFences)
   GIPA(DestroyFence, vkDestroyFence)
   GIPA(FreeCommandBuffers, vkFreeCommandBuffers)
   GIPA(DestroyCommandPool, vkDestroyCommandPool)
   GIPA(DestroyPipeline, vkDestroyPipeline)
   GIPA(DestroyPipelineLayout, vkDestroyPipelineLayout)
   GIPA(DestroyShaderModule, vkDestroyShaderModule)
   GIPA(DestroyDescriptorPool, vkDestroyDescriptorPool)
   GIPA(DestroyDescriptorSetLayout, vkDestroyDescriptorSetLayout)
   GIPA(FreeMemory, vkFreeMemory)
   GIPA(DestroyBuffer, vkDestroyBuffer)
   GIPA(DestroyDevice, vkDestroyDevice)
   GIPA(DestroyInstance, vkDestroyInstance)

   /* pick Mali */
   uint32_t n = 0;
   CK(vkEnumeratePhysicalDevices(inst, &n, NULL), "count");
   fprintf(stderr, "T-after-count n=%u\n", n);
   VkPhysicalDevice *devs = malloc(n * sizeof(*devs));
   CK(vkEnumeratePhysicalDevices(inst, &n, devs), "enum");
   fprintf(stderr, "T-after-enum\n");
   VkPhysicalDevice phys = VK_NULL_HANDLE;
   for (uint32_t i = 0; i < n; i++) {
      fprintf(stderr, "T-loop %u\n", i);
      VkPhysicalDeviceProperties p;
      vkGetPhysicalDeviceProperties(devs[i], &p);
      fprintf(stderr, "T-gpp %u: %s\n", i, p.deviceName);
      if (strstr(p.deviceName, "Mali"))
         phys = devs[i];
   }
   free(devs);
   if (phys == VK_NULL_HANDLE) {
      printf("FAIL no Mali\n");
      return 1;
   }
   printf("D-OK physical device\n");

   /* queue family with COMPUTE */
   uint32_t qn = 0;
   fprintf(stderr, "T-before-qfams\n");
   vkGetPhysicalDeviceQueueFamilyProperties(phys, &qn, NULL);
   fprintf(stderr, "T-after-qfams qn=%u\n", qn);
   VkQueueFamilyProperties *qp = malloc(qn * sizeof(*qp));
   fprintf(stderr, "T-before-qfams2\n");
   vkGetPhysicalDeviceQueueFamilyProperties(phys, &qn, qp);
   fprintf(stderr, "T-after-qfams2\n");
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
   fprintf(stderr, "T-before-CreateDevice qi=%u\n", qi);
   CK(vkCreateDevice(phys, &dci, NULL, &dev), "CreateDevice");
   fprintf(stderr, "T-after-CreateDevice\n");
   printf("D-OK vkCreateDevice\n");
   VkQueue queue;
   vkGetDeviceQueue(dev, qi, 0, &queue);

   /* storage buffer; argv[2]=="big" uses 16KB like the AHB test */
   VkDeviceSize bufsize = (argc > 2 && !strcmp(argv[2], "big")) ? 16384 : 4;
   VkBufferCreateInfo bci = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                             .size = bufsize,
                             .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                             .sharingMode = VK_SHARING_MODE_EXCLUSIVE};
   VkBuffer buf;
   CK(vkCreateBuffer(dev, &bci, NULL, &buf), "CreateBuffer");
   VkMemoryRequirements mr;
   vkGetBufferMemoryRequirements(dev, buf, &mr);
   VkPhysicalDeviceMemoryProperties mp;
   fprintf(stderr, "T-before-memprops\n");
   vkGetPhysicalDeviceMemoryProperties(phys, &mp);
   fprintf(stderr, "T-after-memprops types=%u heaps=%u\n", mp.memoryTypeCount,
           mp.memoryHeapCount);
   uint32_t mi = ~0u;
   for (uint32_t i = 0; i < mp.memoryTypeCount; i++)
      if ((mr.memoryTypeBits & (1u << i)) &&
          (mp.memoryTypes[i].propertyFlags &
           (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) ==
             (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
         mi = i;
         break;
      }
   if (mi == ~0u) {
      printf("FAIL no host-coherent memory\n");
      return 1;
   }
   VkMemoryAllocateInfo mai = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                               .allocationSize = mr.size,
                               .memoryTypeIndex = mi};
   VkDeviceMemory mem;
   fprintf(stderr, "T-before-AllocMem size=%llu mi=%u\n",
           (unsigned long long)mr.size, mi);
   CK(vkAllocateMemory(dev, &mai, NULL, &mem), "AllocateMemory");
   fprintf(stderr, "T-after-AllocMem\n");
   CK(vkBindBufferMemory(dev, buf, mem, 0), "BindBufferMemory");
   fprintf(stderr, "T-after-Bind\n");
   printf("D-OK memory+bind (Gate D device side done)\n");

   /* descriptor + pipeline (built once) */
   VkDescriptorSetLayout dsl = VK_NULL_HANDLE;
   VkDescriptorPool pool = VK_NULL_HANDLE;
   VkDescriptorSet set = VK_NULL_HANDLE;
   if (use_buf) {
   VkDescriptorSetLayoutBinding bind = {.binding = 0,
                                        .descriptorType =
                                           VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                        .descriptorCount = 1,
                                        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT};
   VkDescriptorSetLayoutCreateInfo dlci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = 1,
      .pBindings = &bind};
   CK(vkCreateDescriptorSetLayout(dev, &dlci, NULL, &dsl), "DSL");
   VkDescriptorPoolSize psz = {.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                               .descriptorCount = 1};
   VkDescriptorPoolCreateInfo dpci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = 1,
      .poolSizeCount = 1,
      .pPoolSizes = &psz};
   CK(vkCreateDescriptorPool(dev, &dpci, NULL, &pool), "Pool");
   VkDescriptorSetAllocateInfo dsai = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = pool,
      .descriptorSetCount = 1,
      .pSetLayouts = &dsl};
   CK(vkAllocateDescriptorSets(dev, &dsai, &set), "AllocSets");
   VkDescriptorBufferInfo dbi = {.buffer = buf, .offset = 0, .range = 4};
   VkWriteDescriptorSet wds = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                               .dstSet = set,
                               .dstBinding = 0,
                               .descriptorCount = 1,
                               .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                               .pBufferInfo = &dbi};
   vkUpdateDescriptorSets(dev, 1, &wds, 0, NULL);
   } /* use_buf */

   VkShaderModuleCreateInfo smci = {.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                                    .codeSize = spv_size,
                                    .pCode = spv};
   VkShaderModule mod;
   fprintf(stderr, "T-before-ShaderModule size=%zu\n", spv_size);
   CK(vkCreateShaderModule(dev, &smci, NULL, &mod), "ShaderModule");
   fprintf(stderr, "T-after-ShaderModule\n");
   VkPipelineLayoutCreateInfo plci = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = use_buf ? 1 : 0,
      .pSetLayouts = use_buf ? &dsl : NULL};
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
   fprintf(stderr, "T-before-ComputePipeline\n");
   CK(vkCreateComputePipelines(dev, VK_NULL_HANDLE, 1, &cpci, NULL, &pipe),
      "ComputePipeline");
   fprintf(stderr, "T-after-ComputePipeline\n");

   VkCommandPoolCreateInfo cpoci = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .queueFamilyIndex = qi};
   VkCommandPool cpool;
   CK(vkCreateCommandPool(dev, &cpoci, NULL, &cpool), "CmdPool");
   VkCommandBufferAllocateInfo cbai = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = cpool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1};
   VkCommandBuffer cmd;
   CK(vkAllocateCommandBuffers(dev, &cbai, &cmd), "AllocCmd");
   VkFenceCreateInfo fci = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
   VkFence fence;
   CK(vkCreateFence(dev, &fci, NULL, &fence), "Fence");

   int pass = 0;
   for (int iter = 0; iter < 10; iter++) {
      void *p;
      CK(vkMapMemory(dev, mem, 0, 4, 0, &p), "Map");
      fprintf(stderr, "T-mapped p=%p\n", p);
      *(volatile uint32_t *)p = 0u;
      vkUnmapMemory(dev, mem);

      VkCommandBufferBeginInfo bbi = {
         .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
         .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
      CK(vkResetCommandBuffer(cmd, 0), "ResetCmd");
      CK(vkBeginCommandBuffer(cmd, &bbi), "Begin");
      vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipe);
      if (use_buf)
         vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, layout,
                                 0, 1, &set, 0, NULL);
      vkCmdDispatch(cmd, 1, 1, 1);
      CK(vkEndCommandBuffer(cmd), "End");
      VkSubmitInfo si = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                         .commandBufferCount = 1,
                         .pCommandBuffers = &cmd};
      CK(vkQueueSubmit(queue, 1, &si, fence), "Submit");
      CK(vkWaitForFences(dev, 1, &fence, VK_TRUE, 30 * 1000 * 1000 * 1000ULL),
         "WaitFences");
      CK(vkResetFences(dev, 1, &fence), "ResetFences");

      CK(vkMapMemory(dev, mem, 0, 4, 0, &p), "Map2");
      uint32_t v = *(volatile uint32_t *)p;
      vkUnmapMemory(dev, mem);
      if (!use_buf) {
         pass++;
      } else if (v == 0x12345678u) {
         pass++;
      } else {
         printf("E-iter %d: got 0x%08x (want 0x12345678)\n", iter, v);
      }
   }
   printf("E-compute %d/10\n", pass);

   vkDestroyFence(dev, fence, NULL);
   vkFreeCommandBuffers(dev, cpool, 1, &cmd);
   vkDestroyCommandPool(dev, cpool, NULL);
   vkDestroyPipeline(dev, pipe, NULL);
   vkDestroyPipelineLayout(dev, layout, NULL);
   vkDestroyShaderModule(dev, mod, NULL);
   if (use_buf) {
      vkDestroyDescriptorPool(dev, pool, NULL);
      vkDestroyDescriptorSetLayout(dev, dsl, NULL);
   }
   vkFreeMemory(dev, mem, NULL);
   vkDestroyBuffer(dev, buf, NULL);
   vkDestroyDevice(dev, NULL);
   vkDestroyInstance(inst, NULL);

   if (pass == 10) {
      printf("D+E-PASS\n");
      return 0;
   }
   printf("D+E-FAIL\n");
   return 1;
}
