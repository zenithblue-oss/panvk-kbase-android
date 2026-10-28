/* Target-independent rasterization (D3D11 FL 11_1 TIR, UAV-only rendering)
 * on Vulkan: dynamic rendering with no attachments, pipeline
 * rasterizationSamples 1 or 4, a band between pixel centres that only the 4x
 * sample rows cover. The FS counts its invocations with an SSBO atomic.
 * Cases mirror DXVK on a tiler (perfHints.preferRenderPassOps): the pass is
 * recorded in a secondary whose VkCommandBufferInheritanceRenderingInfo has
 * rasterizationSamples 0 (no attachments).
 * Usage: tir <libvulkan_panfrost.so>
 * Build: see build.sh. PASS line per case; exit 0 when every case passes.
 */
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

typedef PFN_vkVoidFunction (*icd_gipa_fn)(VkInstance, const char *);

#define CK(expr, what)                                                         \
   do {                                                                        \
      VkResult _r = (expr);                                                    \
      if (_r != VK_SUCCESS) {                                                  \
         printf("FAIL %s r=%d line=%d\n", what, _r, __LINE__);                 \
         exit(1);                                                              \
      }                                                                        \
   } while (0)

static const uint32_t vs_spv[] =
#include "tir.vert.inc"
   ;
static const uint32_t fs_spv[] =
#include "tir.frag.inc"
   ;

#define W 32

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
   PFN_vkCreateInstance vkCreateInstance =
      (PFN_vkCreateInstance)gipa(NULL, "vkCreateInstance");
   VkApplicationInfo app = {.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                            .apiVersion = VK_API_VERSION_1_3};
   VkInstanceCreateInfo ici = {.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                               .pApplicationInfo = &app};
   VkInstance inst;
   CK(vkCreateInstance(&ici, NULL, &inst), "CreateInstance");
#define G(name)                                                                \
   PFN_vk##name vk##name = (PFN_vk##name)gipa(inst, "vk" #name);               \
   if (!vk##name) {                                                            \
      printf("FAIL missing vk" #name "\n");                                    \
      return 1;                                                                \
   }
   G(EnumeratePhysicalDevices)
   G(GetPhysicalDeviceProperties)
   G(GetPhysicalDeviceMemoryProperties)
   G(GetPhysicalDeviceQueueFamilyProperties)
   G(CreateDevice)
   G(GetDeviceQueue)
   G(CreateBuffer)
   G(GetBufferMemoryRequirements)
   G(AllocateMemory)
   G(BindBufferMemory)
   G(MapMemory)
   G(CreateShaderModule)
   G(CreateDescriptorSetLayout)
   G(CreateDescriptorPool)
   G(AllocateDescriptorSets)
   G(UpdateDescriptorSets)
   G(CreatePipelineLayout)
   G(CreateGraphicsPipelines)
   G(CreateCommandPool)
   G(AllocateCommandBuffers)
   G(BeginCommandBuffer)
   G(EndCommandBuffer)
   G(ResetCommandPool)
   G(CmdBeginRendering)
   G(CmdEndRendering)
   G(CmdBindPipeline)
   G(CmdBindDescriptorSets)
   G(CmdPushConstants)
   G(CmdDraw)
   G(CmdExecuteCommands)
   G(CmdFillBuffer)
   G(CmdPipelineBarrier)
   G(CreateFence)
   G(QueueSubmit)
   G(WaitForFences)
   G(ResetFences)

   uint32_t n = 1;
   VkPhysicalDevice phys;
   vkEnumeratePhysicalDevices(inst, &n, &phys);
   VkPhysicalDeviceProperties props;
   vkGetPhysicalDeviceProperties(phys, &props);
   printf("device %s\n", props.deviceName);

   uint32_t qn = 0;
   vkGetPhysicalDeviceQueueFamilyProperties(phys, &qn, NULL);
   VkQueueFamilyProperties qp[8];
   qn = qn > 8 ? 8 : qn;
   vkGetPhysicalDeviceQueueFamilyProperties(phys, &qn, qp);
   uint32_t qi = 0;
   while (qi < qn && !(qp[qi].queueFlags & VK_QUEUE_GRAPHICS_BIT))
      qi++;

   VkPhysicalDeviceVulkan13Features f13 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
      .dynamicRendering = VK_TRUE};
   VkPhysicalDeviceFeatures2 f2 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
      .pNext = &f13,
      .features = {.fragmentStoresAndAtomics = VK_TRUE,
                   .variableMultisampleRate = VK_TRUE}};
   float prio = 1.0f;
   VkDeviceQueueCreateInfo qci = {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                  .queueFamilyIndex = qi,
                                  .queueCount = 1,
                                  .pQueuePriorities = &prio};
   VkDeviceCreateInfo dci = {.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                             .pNext = &f2,
                             .queueCreateInfoCount = 1,
                             .pQueueCreateInfos = &qci};
   VkDevice dev;
   CK(vkCreateDevice(phys, &dci, NULL, &dev), "CreateDevice");
   VkQueue queue;
   vkGetDeviceQueue(dev, qi, 0, &queue);

   VkPhysicalDeviceMemoryProperties mp;
   vkGetPhysicalDeviceMemoryProperties(phys, &mp);
   uint32_t mi = 0;
   const VkMemoryPropertyFlags hv = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
   while (mi < mp.memoryTypeCount && (mp.memoryTypes[mi].propertyFlags & hv) != hv)
      mi++;

   VkBufferCreateInfo bci = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                             .size = 16,
                             .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                      VK_BUFFER_USAGE_TRANSFER_DST_BIT};
   VkBuffer buf;
   CK(vkCreateBuffer(dev, &bci, NULL, &buf), "Buffer");
   VkMemoryRequirements mr;
   vkGetBufferMemoryRequirements(dev, buf, &mr);
   VkMemoryAllocateInfo mai = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                               .allocationSize = mr.size,
                               .memoryTypeIndex = mi};
   VkDeviceMemory mem;
   CK(vkAllocateMemory(dev, &mai, NULL, &mem), "Mem");
   CK(vkBindBufferMemory(dev, buf, mem, 0), "Bind");
   volatile uint32_t *cnt;
   CK(vkMapMemory(dev, mem, 0, 16, 0, (void **)&cnt), "Map");

   VkDescriptorSetLayoutBinding dslb = {.binding = 0,
                                        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                        .descriptorCount = 1,
                                        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT};
   VkDescriptorSetLayoutCreateInfo dslci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = 1,
      .pBindings = &dslb};
   VkDescriptorSetLayout dsl;
   CK(vkCreateDescriptorSetLayout(dev, &dslci, NULL, &dsl), "DSL");
   VkDescriptorPoolSize ps = {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1};
   VkDescriptorPoolCreateInfo dpci = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                                      .maxSets = 1,
                                      .poolSizeCount = 1,
                                      .pPoolSizes = &ps};
   VkDescriptorPool dp;
   CK(vkCreateDescriptorPool(dev, &dpci, NULL, &dp), "DP");
   VkDescriptorSetAllocateInfo dsai = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                                       .descriptorPool = dp,
                                       .descriptorSetCount = 1,
                                       .pSetLayouts = &dsl};
   VkDescriptorSet ds;
   CK(vkAllocateDescriptorSets(dev, &dsai, &ds), "DS");
   VkDescriptorBufferInfo dbi = {buf, 0, 16};
   VkWriteDescriptorSet wds = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                               .dstSet = ds,
                               .descriptorCount = 1,
                               .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                               .pBufferInfo = &dbi};
   vkUpdateDescriptorSets(dev, 1, &wds, 0, NULL);

   VkPushConstantRange pcr = {VK_SHADER_STAGE_FRAGMENT_BIT, 0, 4};
   VkPipelineLayoutCreateInfo plci = {.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                                      .setLayoutCount = 1,
                                      .pSetLayouts = &dsl,
                                      .pushConstantRangeCount = 1,
                                      .pPushConstantRanges = &pcr};
   VkPipelineLayout layout;
   CK(vkCreatePipelineLayout(dev, &plci, NULL, &layout), "Layout");

   VkShaderModuleCreateInfo smci = {.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                                    .codeSize = sizeof(vs_spv),
                                    .pCode = vs_spv};
   VkShaderModule vsm, fsm;
   CK(vkCreateShaderModule(dev, &smci, NULL, &vsm), "VS");
   smci.codeSize = sizeof(fs_spv);
   smci.pCode = fs_spv;
   CK(vkCreateShaderModule(dev, &smci, NULL, &fsm), "FS");
   VkPipelineShaderStageCreateInfo stages[2] = {
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
       .stage = VK_SHADER_STAGE_VERTEX_BIT, .module = vsm, .pName = "main"},
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
       .stage = VK_SHADER_STAGE_FRAGMENT_BIT, .module = fsm, .pName = "main"}};
   VkPipelineVertexInputStateCreateInfo vi = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
   VkPipelineInputAssemblyStateCreateInfo ia = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP};
   VkViewport vp = {0, 0, W, W, 0, 1};
   VkRect2D sc = {{0, 0}, {W, W}};
   VkPipelineViewportStateCreateInfo vps = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1, .pViewports = &vp, .scissorCount = 1, .pScissors = &sc};
   VkPipelineRasterizationStateCreateInfo rs = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_NONE,
      .lineWidth = 1.0f};
   VkPipelineMultisampleStateCreateInfo ms = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
   VkPipelineColorBlendStateCreateInfo cb = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
   VkPipelineRenderingCreateInfo pri = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
   VkGraphicsPipelineCreateInfo gpci = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .pNext = &pri,
      .stageCount = 2, .pStages = stages,
      .pVertexInputState = &vi, .pInputAssemblyState = &ia,
      .pViewportState = &vps, .pRasterizationState = &rs,
      .pMultisampleState = &ms, .pColorBlendState = &cb,
      .layout = layout};
   VkPipeline pipe[2];
   ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
   CK(vkCreateGraphicsPipelines(dev, VK_NULL_HANDLE, 1, &gpci, NULL, &pipe[0]), "Pipe1");
   ms.rasterizationSamples = VK_SAMPLE_COUNT_4_BIT;
   CK(vkCreateGraphicsPipelines(dev, VK_NULL_HANDLE, 1, &gpci, NULL, &pipe[1]), "Pipe4");

   VkCommandPoolCreateInfo cpci = {.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                                   .queueFamilyIndex = qi};
   VkCommandPool pool;
   CK(vkCreateCommandPool(dev, &cpci, NULL, &pool), "Pool");
   VkFenceCreateInfo fci = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
   VkFence fence;
   CK(vkCreateFence(dev, &fci, NULL, &fence), "Fence");

   /* case: name, secondary?, inheritance samples, draws (pipeline index per
    * counter slot, -1 = none). Expect slot with pipe 1 >= 32, pipe 0 == 0. */
   static const struct {
      const char *name;
      int secondary;
      VkSampleCountFlagBits inh;
      int draws[2];
   } cases[] = {
      {"primary-1x", 0, 0, {0, -1}},
      {"primary-4x", 0, 0, {1, -1}},
      {"primary-1x-then-4x", 0, 0, {0, 1}},
      {"secondary-inh0-4x (DXVK)", 1, 0, {1, -1}},
      {"secondary-inh0-1x", 1, 0, {0, -1}},
      {"secondary-inh4-4x", 1, VK_SAMPLE_COUNT_4_BIT, {1, -1}},
      {"secondary-inh0-1x-then-4x", 1, 0, {0, 1}},
      {"secondary-inh0-4x-then-1x", 1, 0, {1, 0}},
   };
   int fails = 0;
   for (unsigned c = 0; c < sizeof(cases) / sizeof(cases[0]); c++) {
      CK(vkResetCommandPool(dev, pool, 0), "ResetPool");
      VkCommandBufferAllocateInfo cbai = {
         .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
         .commandPool = pool,
         .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
         .commandBufferCount = 1};
      VkCommandBuffer prim, sec = VK_NULL_HANDLE;
      CK(vkAllocateCommandBuffers(dev, &cbai, &prim), "Prim");
      VkCommandBufferBeginInfo bbi = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
      VkRenderingInfo ri = {.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
                            .renderArea = {{0, 0}, {W, W}},
                            .layerCount = 1};
      VkCommandBuffer rec = prim;
      if (cases[c].secondary) {
         cbai.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
         CK(vkAllocateCommandBuffers(dev, &cbai, &sec), "Sec");
         VkCommandBufferInheritanceRenderingInfo iri = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO,
            .rasterizationSamples = cases[c].inh};
         VkCommandBufferInheritanceInfo ii = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
            .pNext = &iri};
         VkCommandBufferBeginInfo sbi = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT |
                     VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
            .pInheritanceInfo = &ii};
         CK(vkBeginCommandBuffer(sec, &sbi), "BeginSec");
         ri.flags = VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT;
         rec = sec;
      }
      for (uint32_t s = 0; sec && s < 2; s++) {
         if (cases[c].draws[s] < 0)
            continue;
         vkCmdBindPipeline(rec, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe[cases[c].draws[s]]);
         vkCmdBindDescriptorSets(rec, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &ds, 0, NULL);
         vkCmdPushConstants(rec, layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 4, &s);
         vkCmdDraw(rec, 4, 1, 0, 0);
      }
      if (sec)
         CK(vkEndCommandBuffer(sec), "EndSec");
      CK(vkBeginCommandBuffer(prim, &bbi), "Begin");
      vkCmdFillBuffer(prim, buf, 0, 16, 0);
      VkMemoryBarrier mb = {.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
                            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT |
                                             VK_ACCESS_SHADER_WRITE_BIT};
      vkCmdPipelineBarrier(prim, VK_PIPELINE_STAGE_TRANSFER_BIT,
                           VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 1, &mb, 0,
                           NULL, 0, NULL);
      if (sec) {
         vkCmdBeginRendering(prim, &ri);
         vkCmdExecuteCommands(prim, 1, &sec);
         vkCmdEndRendering(prim);
      } else {
         vkCmdBeginRendering(prim, &ri);
         for (uint32_t s = 0; s < 2; s++) {
            if (cases[c].draws[s] < 0)
               continue;
            vkCmdBindPipeline(prim, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe[cases[c].draws[s]]);
            vkCmdBindDescriptorSets(prim, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &ds, 0, NULL);
            vkCmdPushConstants(prim, layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 4, &s);
            vkCmdDraw(prim, 4, 1, 0, 0);
         }
         vkCmdEndRendering(prim);
      }
      mb.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
      mb.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
      vkCmdPipelineBarrier(prim, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                           VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &mb, 0, NULL, 0,
                           NULL);
      CK(vkEndCommandBuffer(prim), "End");
      VkSubmitInfo si = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                         .commandBufferCount = 1,
                         .pCommandBuffers = &prim};
      CK(vkResetFences(dev, 1, &fence), "ResetFence");
      CK(vkQueueSubmit(queue, 1, &si, fence), "Submit");
      CK(vkWaitForFences(dev, 1, &fence, VK_TRUE, 10ull * 1000 * 1000 * 1000), "Wait");
      int ok = 1;
      for (uint32_t s = 0; s < 2; s++) {
         int p = cases[c].draws[s];
         if (p < 0)
            continue;
         ok &= p ? cnt[s] >= W : cnt[s] == 0;
      }
      printf("%s %-28s counts %u %u\n", ok ? "PASS" : "FAIL", cases[c].name,
             cnt[0], cnt[1]);
      fails += !ok;
   }
   printf("%s %d/%u\n", fails ? "TIR-FAIL" : "TIR-PASS",
          (int)(sizeof(cases) / sizeof(cases[0])) - fails,
          (unsigned)(sizeof(cases) / sizeof(cases[0])));
   return fails != 0;
}
