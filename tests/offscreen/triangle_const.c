/* Gate F: offscreen partial-triangle render to 64x64 RGBA8 + readback.
 * Clear = blue; triangle covers bottom-left corner in red.
 * PASS: top-right pixel stays clear blue, bottom-left interior pixel is red
 * (not a clear/blit false positive: two distinct colors in known regions).
 * Usage: triangle <libvulkan_panfrost.so>
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

#include "tri_const.vert.spv.h"
#include "tri.frag.spv.h"

#define W 64
#define H 64

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
   G(GetPhysicalDeviceFormatProperties, vkGetPhysicalDeviceFormatProperties)
   G(CreateDevice, vkCreateDevice)
   G(GetDeviceQueue, vkGetDeviceQueue)
   G(CreateBuffer, vkCreateBuffer)
   G(GetBufferMemoryRequirements, vkGetBufferMemoryRequirements)
   G(AllocateMemory, vkAllocateMemory)
   G(BindBufferMemory, vkBindBufferMemory)
   G(MapMemory, vkMapMemory)
   G(UnmapMemory, vkUnmapMemory)
   G(CreateImage, vkCreateImage)
   G(GetImageMemoryRequirements, vkGetImageMemoryRequirements)
   G(BindImageMemory, vkBindImageMemory)
   G(CreateImageView, vkCreateImageView)
   G(CreateRenderPass, vkCreateRenderPass)
   G(CreateFramebuffer, vkCreateFramebuffer)
   G(CreateShaderModule, vkCreateShaderModule)
   G(CreatePipelineLayout, vkCreatePipelineLayout)
   G(CreateGraphicsPipelines, vkCreateGraphicsPipelines)
   G(CreateCommandPool, vkCreateCommandPool)
   G(AllocateCommandBuffers, vkAllocateCommandBuffers)
   G(BeginCommandBuffer, vkBeginCommandBuffer)
   G(ResetCommandBuffer, vkResetCommandBuffer)
   G(CmdBeginRenderPass, vkCmdBeginRenderPass)
   G(CmdBindPipeline, vkCmdBindPipeline)
   G(CmdBindVertexBuffers, vkCmdBindVertexBuffers)
   G(CmdDraw, vkCmdDraw)
   G(CmdEndRenderPass, vkCmdEndRenderPass)
   G(CmdCopyImageToBuffer, vkCmdCopyImageToBuffer)
   G(CmdPipelineBarrier, vkCmdPipelineBarrier)
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
   G(DestroyFramebuffer, vkDestroyFramebuffer)
   G(DestroyRenderPass, vkDestroyRenderPass)
   G(DestroyImageView, vkDestroyImageView)
   G(FreeMemory, vkFreeMemory)
   G(DestroyImage, vkDestroyImage)
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
      if ((qp[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
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
   printf("F-device ready (family %u)\n", qi);

   VkPhysicalDeviceMemoryProperties mp;
   vkGetPhysicalDeviceMemoryProperties(phys, &mp);
   uint32_t host_mi = ~0u, dev_mi = ~0u;
   for (uint32_t i = 0; i < mp.memoryTypeCount; i++) {
      VkMemoryPropertyFlags f = mp.memoryTypes[i].propertyFlags;
      if (host_mi == ~0u &&
          (f & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) ==
               (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
         host_mi = i;
      if (dev_mi == ~0u && (f & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
         dev_mi = i;
   }
   if (host_mi == ~0u) {
      printf("FAIL no host memory\n");
      return 1;
   }
   if (dev_mi == ~0u)
      dev_mi = host_mi;

   /* vertex buffer: bottom-left triangle */
   float verts[] = {-1.0f, -1.0f, -0.25f, -1.0f, -1.0f, -0.25f};
   VkBufferCreateInfo vbci = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                              .size = sizeof(verts),
                              .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                              .sharingMode = VK_SHARING_MODE_EXCLUSIVE};
   VkBuffer vbuf;
   CK(vkCreateBuffer(dev, &vbci, NULL, &vbuf), "VBuffer");
   VkMemoryRequirements vmr;
   vkGetBufferMemoryRequirements(dev, vbuf, &vmr);
   VkDeviceMemory vmem;
   VkMemoryAllocateInfo vmai = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                                .allocationSize = vmr.size,
                                .memoryTypeIndex = host_mi};
   CK(vkAllocateMemory(dev, &vmai, NULL, &vmem), "VMem");
   CK(vkBindBufferMemory(dev, vbuf, vmem, 0), "VBind");
   void *vp;
   CK(vkMapMemory(dev, vmem, 0, sizeof(verts), 0, &vp), "VMap");
   memcpy(vp, verts, sizeof(verts));
   vkUnmapMemory(dev, vmem);

   /* color image */
   VkImageCreateInfo ici2 = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                             .imageType = VK_IMAGE_TYPE_2D,
                             .format = VK_FORMAT_R8G8B8A8_UNORM,
                             .extent = {W, H, 1},
                             .mipLevels = 1,
                             .arrayLayers = 1,
                             .samples = VK_SAMPLE_COUNT_1_BIT,
                             .tiling = VK_IMAGE_TILING_OPTIMAL,
                             .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                      VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                             .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                             .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};
   VkImage img;
   CK(vkCreateImage(dev, &ici2, NULL, &img), "Image");
   VkMemoryRequirements imr;
   vkGetImageMemoryRequirements(dev, img, &imr);
   uint32_t imi = dev_mi;
   if (!(imr.memoryTypeBits & (1u << imi))) {
      imi = host_mi;
      for (uint32_t i = 0; i < mp.memoryTypeCount; i++)
         if (imr.memoryTypeBits & (1u << i)) {
            imi = i;
            break;
         }
   }
   VkDeviceMemory imem;
   VkMemoryAllocateInfo imai = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                                .allocationSize = imr.size,
                                .memoryTypeIndex = imi};
   CK(vkAllocateMemory(dev, &imai, NULL, &imem), "IMem");
   CK(vkBindImageMemory(dev, img, imem, 0), "IBind");

   VkImageViewCreateInfo ivci = {.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                                 .image = img,
                                 .viewType = VK_IMAGE_VIEW_TYPE_2D,
                                 .format = VK_FORMAT_R8G8B8A8_UNORM,
                                 .subresourceRange = {.aspectMask =
                                                         VK_IMAGE_ASPECT_COLOR_BIT,
                                                      .levelCount = 1,
                                                      .layerCount = 1}};
   VkImageView view;
   CK(vkCreateImageView(dev, &ivci, NULL, &view), "View");

   /* readback buffer */
   VkBufferCreateInfo rbci = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                              .size = W * H * 4,
                              .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                              .sharingMode = VK_SHARING_MODE_EXCLUSIVE};
   VkBuffer rbuf;
   CK(vkCreateBuffer(dev, &rbci, NULL, &rbuf), "RBuffer");
   VkMemoryRequirements rmr;
   vkGetBufferMemoryRequirements(dev, rbuf, &rmr);
   VkDeviceMemory rmem;
   VkMemoryAllocateInfo rmai = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                                .allocationSize = rmr.size,
                                .memoryTypeIndex = host_mi};
   CK(vkAllocateMemory(dev, &rmai, NULL, &rmem), "RMem");
   CK(vkBindBufferMemory(dev, rbuf, rmem, 0), "RBind");

   /* render pass */
   VkAttachmentDescription att = {.format = VK_FORMAT_R8G8B8A8_UNORM,
                                  .samples = VK_SAMPLE_COUNT_1_BIT,
                                  .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                  .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                                  .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                                  .finalLayout =
                                     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL};
   VkAttachmentReference ref = {.attachment = 0,
                                .layout =
                                   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
   VkSubpassDescription sub = {.pipelineBindPoint =
                                  VK_PIPELINE_BIND_POINT_GRAPHICS,
                               .colorAttachmentCount = 1,
                               .pColorAttachments = &ref};
   VkRenderPassCreateInfo rpci = {.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                                  .attachmentCount = 1,
                                  .pAttachments = &att,
                                  .subpassCount = 1,
                                  .pSubpasses = &sub};
   VkRenderPass rp;
   CK(vkCreateRenderPass(dev, &rpci, NULL, &rp), "RP");
   VkFramebufferCreateInfo fbci = {.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                                   .renderPass = rp,
                                   .attachmentCount = 1,
                                   .pAttachments = &view,
                                   .width = W,
                                   .height = H,
                                   .layers = 1};
   VkFramebuffer fb;
   CK(vkCreateFramebuffer(dev, &fbci, NULL, &fb), "FB");

   /* graphics pipeline */
   VkShaderModuleCreateInfo vsmci = {.sType =
                                        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                                     .codeSize = sizeof(tri_const_vert_spv),
                                     .pCode = tri_const_vert_spv};
   VkShaderModule vsm, fsm;
   CK(vkCreateShaderModule(dev, &vsmci, NULL, &vsm), "VS");
   VkShaderModuleCreateInfo fsmci = {.sType =
                                        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                                     .codeSize = sizeof(tri_frag_spv),
                                     .pCode = tri_frag_spv};
   CK(vkCreateShaderModule(dev, &fsmci, NULL, &fsm), "FS");
   VkPipelineShaderStageCreateInfo stages[2] = {
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
       .stage = VK_SHADER_STAGE_VERTEX_BIT,
       .module = vsm,
       .pName = "main"},
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
       .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
       .module = fsm,
       .pName = "main"}};
   VkVertexInputBindingDescription vbind = {.binding = 0,
                                            .stride = 8,
                                            .inputRate =
                                               VK_VERTEX_INPUT_RATE_VERTEX};
   VkVertexInputAttributeDescription vattr = {.location = 0,
                                              .binding = 0,
                                              .format = VK_FORMAT_R32G32_SFLOAT,
                                              .offset = 0};
   VkPipelineVertexInputStateCreateInfo vi = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = &vbind,
      .vertexAttributeDescriptionCount = 1,
      .pVertexAttributeDescriptions = &vattr};
   VkPipelineInputAssemblyStateCreateInfo ia = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
   VkViewport vport = {0, 0, W, H, 0, 1};
   VkRect2D scis = {{0, 0}, {W, H}};
   VkPipelineViewportStateCreateInfo vps = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .pViewports = &vport,
      .scissorCount = 1,
      .pScissors = &scis};
   VkPipelineRasterizationStateCreateInfo rast = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_NONE,
      .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
      .lineWidth = 1.0f};
   VkPipelineMultisampleStateCreateInfo ms = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT};
   VkPipelineColorBlendAttachmentState blend = {.colorWriteMask = 0xF};
   VkPipelineColorBlendStateCreateInfo cb = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .attachmentCount = 1,
      .pAttachments = &blend};
   VkPipelineLayoutCreateInfo plci = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
   VkPipelineLayout layout;
   CK(vkCreatePipelineLayout(dev, &plci, NULL, &layout), "Layout");
   VkGraphicsPipelineCreateInfo gpci = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .stageCount = 2,
      .pStages = stages,
      .pVertexInputState = &vi,
      .pInputAssemblyState = &ia,
      .pViewportState = &vps,
      .pRasterizationState = &rast,
      .pMultisampleState = &ms,
      .pColorBlendState = &cb,
      .layout = layout,
      .renderPass = rp};
   VkPipeline pipe;
   CK(vkCreateGraphicsPipelines(dev, VK_NULL_HANDLE, 1, &gpci, NULL, &pipe),
      "GfxPipe");
   printf("F-pipeline ready\n");

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
   VkClearValue clear = {.color = {{0.0f, 0.0f, 1.0f, 1.0f}}};
   VkRenderPassBeginInfo rpbi = {.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                                 .renderPass = rp,
                                 .framebuffer = fb,
                                 .renderArea = {{0, 0}, {W, H}},
                                 .clearValueCount = 1,
                                 .pClearValues = &clear};
   vkCmdBeginRenderPass(cmd, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
   vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe);
   VkDeviceSize off = 0;
   vkCmdBindVertexBuffers(cmd, 0, 1, &vbuf, &off);
   vkCmdDraw(cmd, 3, 1, 0, 0);
   vkCmdEndRenderPass(cmd);
   VkImageMemoryBarrier bar = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                               .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                               .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                               .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                               .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                               .image = img,
                               .subresourceRange = {.aspectMask =
                                                       VK_IMAGE_ASPECT_COLOR_BIT,
                                                    .levelCount = 1,
                                                    .layerCount = 1}};
   vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1,
                        &bar);
   VkBufferImageCopy region = {.imageSubresource = {.aspectMask =
                                                       VK_IMAGE_ASPECT_COLOR_BIT,
                                                    .layerCount = 1},
                               .imageExtent = {W, H, 1}};
   vkCmdCopyImageToBuffer(cmd, img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                          rbuf, 1, &region);
   CK(vkEndCommandBuffer(cmd), "End");
   VkSubmitInfo si = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                      .commandBufferCount = 1,
                      .pCommandBuffers = &cmd};
   CK(vkQueueSubmit(queue, 1, &si, fence), "Submit");
   CK(vkWaitForFences(dev, 1, &fence, VK_TRUE, 30 * 1000 * 1000 * 1000ULL),
      "Wait");
   printf("F-submit complete\n");

   void *rp_;
   CK(vkMapMemory(dev, rmem, 0, W * H * 4, 0, &rp_), "RMap");
   uint8_t *px = rp_;
   /* Vulkan origin top-left: interior (8,55) bottom-left red; clear (55,8) blue */
   uint8_t *in_px = px + (55 * W + 8) * 4;
   uint8_t *clear_px = px + (8 * W + 55) * 4;
   printf("F-interior RGBA=%u %u %u %u\n", in_px[0], in_px[1], in_px[2],
          in_px[3]);
   printf("F-clear    RGBA=%u %u %u %u\n", clear_px[0], clear_px[1], clear_px[2],
          clear_px[3]);
   int red = 0, blue = 0, other = 0;
   for (int y = 0; y < H; y++)
      for (int x = 0; x < W; x++) {
         uint8_t *q = px + (y * W + x) * 4;
         if (q[0] > 200 && q[1] < 60 && q[2] < 60) {
            if (red < 8)
               printf("F-red at %d,%d EXTRA", x, y);
            red++;
         } else if (q[2] > 200 && q[0] < 60 && q[1] < 60)
            blue++;
         else {
            if (other < 8)
               printf("F-other %u %u %u %u at %d,%d EXTRA", q[0], q[1], q[2],
                      q[3], x, y);
            other++;
         }
      }
   printf("F-counts red=%d blue=%d other=%d EXTRA", red, blue, other);
   int ok = (in_px[0] > 200 && in_px[1] < 60 && in_px[2] < 60) &&
            (clear_px[2] > 200 && clear_px[0] < 60 && clear_px[1] < 60);
   vkUnmapMemory(dev, rmem);

   vkDestroyFence(dev, fence, NULL);
   vkFreeCommandBuffers(dev, cpool, 1, &cmd);
   vkDestroyCommandPool(dev, cpool, NULL);
   vkDestroyPipeline(dev, pipe, NULL);
   vkDestroyPipelineLayout(dev, layout, NULL);
   vkDestroyShaderModule(dev, vsm, NULL);
   vkDestroyShaderModule(dev, fsm, NULL);
   vkDestroyFramebuffer(dev, fb, NULL);
   vkDestroyRenderPass(dev, rp, NULL);
   vkDestroyImageView(dev, view, NULL);
   vkFreeMemory(dev, imem, NULL);
   vkDestroyImage(dev, img, NULL);
   vkFreeMemory(dev, rmem, NULL);
   vkDestroyBuffer(dev, rbuf, NULL);
   vkFreeMemory(dev, vmem, NULL);
   vkDestroyBuffer(dev, vbuf, NULL);
   vkDestroyDevice(dev, NULL);
   vkDestroyInstance(inst, NULL);

   if (ok) {
      printf("F-PASS\n");
      return 0;
   }
   printf("F-FAIL\n");
   return 1;
}
