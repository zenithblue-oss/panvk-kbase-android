/* D3D12 FL 11_1 logic_op on Vulkan (vkd3d-proton translation): R8G8B8A8_UINT
 * target, dynamic rendering, a plain draw of (ff,0f,f0,aa) then a pipeline
 * with logicOpEnable + VK_LOGIC_OP_INVERT over it. Expect 550ff000.
 * Cases: render pass loadOp CLEAR (vkd3d deferred clear) and
 * vkCmdClearColorImage + loadOp LOAD; each repeated ITER times.
 * Usage: lop <libvulkan_panfrost.so>
 * Build: see build.sh. LOP-PASS when every iteration reads 550ff000.
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
#include "lop.vert.inc"
   ;
static const uint32_t fs_spv[] =
#include "lop.frag.inc"
   ;

#define W 64
#define ITER 20

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
   G(CreateImage)
   G(GetImageMemoryRequirements)
   G(BindImageMemory)
   G(CreateImageView)
   G(AllocateMemory)
   G(BindBufferMemory)
   G(MapMemory)
   G(CreateShaderModule)
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
   G(CmdDraw)
   G(CmdPipelineBarrier)
   G(CmdClearColorImage)
   G(CmdCopyImageToBuffer)
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
      .features = {.logicOp = VK_TRUE}};
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
   uint32_t mi = 0, di = 0;
   const VkMemoryPropertyFlags hv = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
   while (mi < mp.memoryTypeCount && (mp.memoryTypes[mi].propertyFlags & hv) != hv)
      mi++;
   while (di < mp.memoryTypeCount &&
          !(mp.memoryTypes[di].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
      di++;

   const VkFormat fmt = VK_FORMAT_R8G8B8A8_UINT;
   VkImageCreateInfo imci = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                             .imageType = VK_IMAGE_TYPE_2D,
                             .format = fmt,
                             .extent = {W, W, 1},
                             .mipLevels = 1,
                             .arrayLayers = 1,
                             .samples = VK_SAMPLE_COUNT_1_BIT,
                             .tiling = VK_IMAGE_TILING_OPTIMAL,
                             .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                      VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                      VK_IMAGE_USAGE_TRANSFER_DST_BIT};
   VkImage img;
   CK(vkCreateImage(dev, &imci, NULL, &img), "Image");
   VkMemoryRequirements mr;
   vkGetImageMemoryRequirements(dev, img, &mr);
   VkMemoryAllocateInfo mai = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                               .allocationSize = mr.size,
                               .memoryTypeIndex = di};
   VkDeviceMemory imem;
   CK(vkAllocateMemory(dev, &mai, NULL, &imem), "ImgMem");
   CK(vkBindImageMemory(dev, img, imem, 0), "BindImg");
   VkImageViewCreateInfo ivci = {.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                                 .image = img,
                                 .viewType = VK_IMAGE_VIEW_TYPE_2D,
                                 .format = fmt,
                                 .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};
   VkImageView view;
   CK(vkCreateImageView(dev, &ivci, NULL, &view), "View");

   VkBufferCreateInfo bci = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                             .size = W * W * 4,
                             .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT};
   VkBuffer buf;
   CK(vkCreateBuffer(dev, &bci, NULL, &buf), "Buffer");
   vkGetBufferMemoryRequirements(dev, buf, &mr);
   mai.allocationSize = mr.size;
   mai.memoryTypeIndex = mi;
   VkDeviceMemory mem;
   CK(vkAllocateMemory(dev, &mai, NULL, &mem), "Mem");
   CK(vkBindBufferMemory(dev, buf, mem, 0), "Bind");
   volatile uint32_t *px;
   CK(vkMapMemory(dev, mem, 0, W * W * 4, 0, (void **)&px), "Map");

   VkPipelineLayoutCreateInfo plci = {.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
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
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
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
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
      .pSampleMask = NULL};
   VkPipelineDepthStencilStateCreateInfo dss = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .depthCompareOp = VK_COMPARE_OP_ALWAYS};
   /* D3D12 defaults: ONE/ZERO/ADD, blend disabled, write mask all. */
   VkPipelineColorBlendAttachmentState cba = {
      .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
      .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
      .colorBlendOp = VK_BLEND_OP_ADD,
      .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
      .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
      .alphaBlendOp = VK_BLEND_OP_ADD,
      .colorWriteMask = 0xf};
   VkPipelineColorBlendStateCreateInfo cb = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .logicOp = VK_LOGIC_OP_NO_OP,
      .attachmentCount = 1,
      .pAttachments = &cba};
   VkPipelineRenderingCreateInfo pri = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
      .colorAttachmentCount = 1,
      .pColorAttachmentFormats = &fmt};
   VkGraphicsPipelineCreateInfo gpci = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .pNext = &pri,
      .stageCount = 2, .pStages = stages,
      .pVertexInputState = &vi, .pInputAssemblyState = &ia,
      .pViewportState = &vps, .pRasterizationState = &rs,
      .pMultisampleState = &ms, .pDepthStencilState = &dss,
      .pColorBlendState = &cb,
      .layout = layout};
   VkPipeline plain, inv;
   CK(vkCreateGraphicsPipelines(dev, VK_NULL_HANDLE, 1, &gpci, NULL, &plain), "Plain");
   cb.logicOpEnable = VK_TRUE;
   cb.logicOp = VK_LOGIC_OP_INVERT;
   CK(vkCreateGraphicsPipelines(dev, VK_NULL_HANDLE, 1, &gpci, NULL, &inv), "Invert");

   VkCommandPoolCreateInfo cpci = {.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                                   .queueFamilyIndex = qi};
   VkCommandPool pool;
   CK(vkCreateCommandPool(dev, &cpci, NULL, &pool), "Pool");
   VkFenceCreateInfo fci = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
   VkFence fence;
   CK(vkCreateFence(dev, &fci, NULL, &fence), "Fence");

   static const char *names[] = {"loadop-clear", "clearimage-load",
                                 "loadop-clear-invert-only", "loadop-clear-plain-only"};
   int fails = 0, total = 0;
   for (unsigned c = 0; c < 4; c++) {
      int bad = 0;
      uint32_t last = 0;
      for (int it = 0; it < ITER; it++) {
         CK(vkResetCommandPool(dev, pool, 0), "ResetPool");
         VkCommandBufferAllocateInfo cbai = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1};
         VkCommandBuffer cmd;
         CK(vkAllocateCommandBuffers(dev, &cbai, &cmd), "Cmd");
         VkCommandBufferBeginInfo bbi = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                         .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
         CK(vkBeginCommandBuffer(cmd, &bbi), "Begin");
         VkImageMemoryBarrier ib = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                                    .srcAccessMask = 0,
                                    .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                                    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                                    .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                    .image = img,
                                    .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};
         VkClearColorValue zero = {0};
         if (c == 1) {
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &ib);
            vkCmdClearColorImage(cmd, img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &zero, 1,
                                 &ib.subresourceRange);
            ib.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            ib.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
         }
         ib.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
         ib.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
         vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
                              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0,
                              NULL, 1, &ib);
         VkRenderingAttachmentInfo rai = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = view,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = c == 1 ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE};
         VkRenderingInfo ri = {.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
                               .renderArea = {{0, 0}, {W, W}},
                               .layerCount = 1,
                               .colorAttachmentCount = 1,
                               .pColorAttachments = &rai};
         vkCmdBeginRendering(cmd, &ri);
         if (c != 2) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, plain);
            vkCmdDraw(cmd, 3, 1, 0, 0);
         }
         if (c != 3) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, inv);
            vkCmdDraw(cmd, 3, 1, 0, 0);
         }
         vkCmdEndRendering(cmd);
         ib.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
         ib.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
         ib.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
         ib.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
         vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                              VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &ib);
         VkBufferImageCopy bic = {.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
                                  .imageExtent = {W, W, 1}};
         vkCmdCopyImageToBuffer(cmd, img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buf, 1, &bic);
         VkMemoryBarrier mb = {.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
                               .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                               .dstAccessMask = VK_ACCESS_HOST_READ_BIT};
         vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
                              0, 1, &mb, 0, NULL, 0, NULL);
         CK(vkEndCommandBuffer(cmd), "End");
         VkSubmitInfo si = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                            .commandBufferCount = 1,
                            .pCommandBuffers = &cmd};
         CK(vkResetFences(dev, 1, &fence), "ResetFence");
         CK(vkQueueSubmit(queue, 1, &si, fence), "Submit");
         CK(vkWaitForFences(dev, 1, &fence, VK_TRUE, 10ull * 1000 * 1000 * 1000), "Wait");
         const uint32_t want = c == 2 ? 0xffffffffu : c == 3 ? 0xaaf00fffu : 0x550ff000u;
         uint32_t badpx = 0;
         for (int i = 0; i < W * W; i++)
            badpx += px[i] != want;
         last = px[(W / 2) * W + W / 2];
         bad += badpx != 0;
      }
      printf("%s %-26s %d/%d iterations bad, center %08x\n", bad ? "FAIL" : "PASS",
             names[c], bad, ITER, last);
      fails += bad != 0;
      total++;
   }
   printf("%s %d/%d\n", fails ? "LOP-FAIL" : "LOP-PASS", total - fails, total);
   return fails != 0;
}
