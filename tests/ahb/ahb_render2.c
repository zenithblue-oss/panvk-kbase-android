/* Gate G: AHB render triangle -> check via vkCmdCopyImageToBuffer AND AHardwareBuffer_lock */
#define VK_USE_PLATFORM_ANDROID_KHR 1
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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

#include "../offscreen/tri.vert.spv.h"
#include "../offscreen/tri.frag.spv.h"

#define W 64
#define H 64

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
   const char *exts[] = {
      VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
      VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
   };
   VkInstanceCreateInfo ici = {.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                               .pApplicationInfo = &app,
                               .enabledExtensionCount = 2,
                               .ppEnabledExtensionNames = exts};
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
   G(CreateImage, vkCreateImage)
   G(GetImageMemoryRequirements2, vkGetImageMemoryRequirements2)
   G(AllocateMemory, vkAllocateMemory)
   G(BindBufferMemory, vkBindBufferMemory)
   G(BindImageMemory2, vkBindImageMemory2)
   G(MapMemory, vkMapMemory)
   G(UnmapMemory, vkUnmapMemory)
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
   G(CmdPipelineBarrier, vkCmdPipelineBarrier)
   G(CmdCopyImageToBuffer, vkCmdCopyImageToBuffer)
   G(EndCommandBuffer, vkEndCommandBuffer)
   G(CreateFence, vkCreateFence)
   G(QueueSubmit, vkQueueSubmit)
   G(WaitForFences, vkWaitForFences)
   G(GetAndroidHardwareBufferPropertiesANDROID, vkGetAndroidHardwareBufferPropertiesANDROID)

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
      if (qp[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
         qi = i;
         break;
      }
   free(qp);

   float prio = 1.0f;
   VkDeviceQueueCreateInfo qci = {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                  .queueFamilyIndex = qi,
                                  .queueCount = 1,
                                  .pQueuePriorities = &prio};
   const char *dev_exts[] = {
      VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
      VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
      VK_ANDROID_EXTERNAL_MEMORY_ANDROID_HARDWARE_BUFFER_EXTENSION_NAME,
      VK_KHR_BIND_MEMORY_2_EXTENSION_NAME,
      VK_KHR_MAINTENANCE1_EXTENSION_NAME,
      VK_EXT_QUEUE_FAMILY_FOREIGN_EXTENSION_NAME,
   };
   VkDeviceCreateInfo dci = {.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                             .queueCreateInfoCount = 1,
                             .pQueueCreateInfos = &qci,
                             .enabledExtensionCount = 6,
                             .ppEnabledExtensionNames = dev_exts};
   VkDevice dev;
   CK(vkCreateDevice(phys, &dci, NULL, &dev), "CreateDevice");
   VkQueue queue;
   vkGetDeviceQueue(dev, qi, 0, &queue);

   AHardwareBuffer_Desc desc = {
      .width = W,
      .height = H,
      .layers = 1,
      .format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM,
      .usage = AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT | AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN,
   };
   AHardwareBuffer *ahb = NULL;
   AHardwareBuffer_allocate(&desc, &ahb);
   AHardwareBuffer_Desc out_desc;
   AHardwareBuffer_describe(ahb, &out_desc);

   VkAndroidHardwareBufferPropertiesANDROID ahb_props = {
      .sType = VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_PROPERTIES_ANDROID,
   };
   CK(vkGetAndroidHardwareBufferPropertiesANDROID(dev, ahb, &ahb_props), "AHBProps");

   VkExternalMemoryImageCreateInfo emici = {
      .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,
      .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID,
   };
   VkImageCreateInfo ici2 = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .pNext = &emici,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = VK_FORMAT_R8G8B8A8_UNORM,
      .extent = {W, H, 1},
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
   };
   VkImage img;
   CK(vkCreateImage(dev, &ici2, NULL, &img), "Image");

   VkImportAndroidHardwareBufferInfoANDROID imp = {
      .sType = VK_STRUCTURE_TYPE_IMPORT_ANDROID_HARDWARE_BUFFER_INFO_ANDROID,
      .buffer = ahb,
   };
   VkMemoryDedicatedAllocateInfo dai = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO,
      .pNext = &imp,
      .image = img,
   };
   uint32_t chosen_mi = 0;
   for (uint32_t i = 0; i < 32; i++) {
      if (ahb_props.memoryTypeBits & (1u << i)) {
         chosen_mi = i;
         break;
      }
   }
   VkMemoryAllocateInfo mai = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .pNext = &dai,
      .allocationSize = ahb_props.allocationSize,
      .memoryTypeIndex = chosen_mi,
   };
   VkDeviceMemory imem;
   CK(vkAllocateMemory(dev, &mai, NULL, &imem), "AllocMemory(import)");

   VkBindImageMemoryInfo bmi = {
      .sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO,
      .image = img,
      .memory = imem,
      .memoryOffset = 0,
   };
   CK(vkBindImageMemory2(dev, 1, &bmi), "BindImageMemory2");

   VkImageViewCreateInfo ivci = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = img,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = VK_FORMAT_R8G8B8A8_UNORM,
      .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1},
   };
   VkImageView view;
   CK(vkCreateImageView(dev, &ivci, NULL, &view), "View");

   VkAttachmentDescription att = {
      .format = VK_FORMAT_R8G8B8A8_UNORM,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
   };
   VkAttachmentReference ref = {.attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
   VkSubpassDescription sp = {.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS, .colorAttachmentCount = 1, .pColorAttachments = &ref};
   VkRenderPassCreateInfo rpci = {.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, .attachmentCount = 1, .pAttachments = &att, .subpassCount = 1, .pSubpasses = &sp};
   VkRenderPass rp;
   CK(vkCreateRenderPass(dev, &rpci, NULL, &rp), "RP");

   VkFramebufferCreateInfo fbci = {
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .renderPass = rp,
      .attachmentCount = 1,
      .pAttachments = &view,
      .width = W,
      .height = H,
      .layers = 1,
   };
   VkFramebuffer fb;
   CK(vkCreateFramebuffer(dev, &fbci, NULL, &fb), "FB");

   /* readback buffer */
   VkBufferCreateInfo rbci = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = W * H * 4, .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT, .sharingMode = VK_SHARING_MODE_EXCLUSIVE};
   VkBuffer rbuf;
   CK(vkCreateBuffer(dev, &rbci, NULL, &rbuf), "RBuffer");
   VkMemoryRequirements rmr;
   vkGetBufferMemoryRequirements(dev, rbuf, &rmr);
   VkPhysicalDeviceMemoryProperties mp;
   vkGetPhysicalDeviceMemoryProperties(phys, &mp);
   uint32_t host_mi = ~0u;
   for (uint32_t i = 0; i < mp.memoryTypeCount; i++)
      if ((rmr.memoryTypeBits & (1u << i)) && (mp.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
         host_mi = i; break;
      }
   VkMemoryAllocateInfo rmai = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, .allocationSize = rmr.size, .memoryTypeIndex = host_mi};
   VkDeviceMemory rmem;
   CK(vkAllocateMemory(dev, &rmai, NULL, &rmem), "RMem");
   CK(vkBindBufferMemory(dev, rbuf, rmem, 0), "RBind");

   /* Vertex buffer for red triangle */
   float verts[] = {-1.0f, -1.0f, -0.25f, -1.0f, -1.0f, -0.25f};
   VkBufferCreateInfo vbci = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = sizeof(verts), .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, .sharingMode = VK_SHARING_MODE_EXCLUSIVE};
   VkBuffer vbuf;
   CK(vkCreateBuffer(dev, &vbci, NULL, &vbuf), "VBuffer");
   VkMemoryRequirements vmr;
   vkGetBufferMemoryRequirements(dev, vbuf, &vmr);
   VkMemoryAllocateInfo vmai = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, .allocationSize = vmr.size, .memoryTypeIndex = host_mi};
   VkDeviceMemory vmem;
   CK(vkAllocateMemory(dev, &vmai, NULL, &vmem), "VMem");
   CK(vkBindBufferMemory(dev, vbuf, vmem, 0), "VBind");
   void *vp;
   CK(vkMapMemory(dev, vmem, 0, sizeof(verts), 0, &vp), "VMap");
   memcpy(vp, verts, sizeof(verts));
   vkUnmapMemory(dev, vmem);

   /* Shaders and Pipeline */
   VkShaderModuleCreateInfo vsmci = {.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, .codeSize = sizeof(tri_vert_spv), .pCode = tri_vert_spv};
   VkShaderModule vsm;
   CK(vkCreateShaderModule(dev, &vsmci, NULL, &vsm), "VSM");
   VkShaderModuleCreateInfo fsmci = {.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, .codeSize = sizeof(tri_frag_spv), .pCode = tri_frag_spv};
   VkShaderModule fsm;
   CK(vkCreateShaderModule(dev, &fsmci, NULL, &fsm), "FSM");

   VkPipelineShaderStageCreateInfo stages[2] = {
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_VERTEX_BIT, .module = vsm, .pName = "main"},
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_FRAGMENT_BIT, .module = fsm, .pName = "main"},
   };
   VkVertexInputBindingDescription vibd = {.binding = 0, .stride = 2 * sizeof(float), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};
   VkVertexInputAttributeDescription viad = {.location = 0, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = 0};
   VkPipelineVertexInputStateCreateInfo pvisi = {.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, .vertexBindingDescriptionCount = 1, .pVertexBindingDescriptions = &vibd, .vertexAttributeDescriptionCount = 1, .pVertexAttributeDescriptions = &viad};
   VkPipelineInputAssemblyStateCreateInfo piasi = {.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
   VkViewport vp_ = {.x = 0, .y = 0, .width = (float)W, .height = (float)H, .minDepth = 0.0f, .maxDepth = 1.0f};
   VkRect2D sc_ = {.offset = {0, 0}, .extent = {W, H}};
   VkPipelineViewportStateCreateInfo pvsi = {.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, .viewportCount = 1, .pViewports = &vp_, .scissorCount = 1, .pScissors = &sc_};
   VkPipelineRasterizationStateCreateInfo prsi = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, .polygonMode = VK_POLYGON_MODE_FILL, .cullMode = VK_CULL_MODE_NONE, .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE, .lineWidth = 1.0f};
   VkPipelineMultisampleStateCreateInfo pmsi = {.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT};
   VkPipelineColorBlendAttachmentState pcbas = {.blendEnable = VK_FALSE, .colorWriteMask = 0xf};
   VkPipelineColorBlendStateCreateInfo pcbsi = {.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, .attachmentCount = 1, .pAttachments = &pcbas};
   VkPipelineLayoutCreateInfo plci = {.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
   VkPipelineLayout layout;
   CK(vkCreatePipelineLayout(dev, &plci, NULL, &layout), "Layout");

   VkGraphicsPipelineCreateInfo gpci = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .stageCount = 2,
      .pStages = stages,
      .pVertexInputState = &pvisi,
      .pInputAssemblyState = &piasi,
      .pViewportState = &pvsi,
      .pRasterizationState = &prsi,
      .pMultisampleState = &pmsi,
      .pColorBlendState = &pcbsi,
      .layout = layout,
      .renderPass = rp,
      .subpass = 0,
   };
   VkPipeline pipe;
   CK(vkCreateGraphicsPipelines(dev, VK_NULL_HANDLE, 1, &gpci, NULL, &pipe), "GfxPipe");

   /* Command buffer */
   VkCommandPoolCreateInfo cpoci = {.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, .queueFamilyIndex = qi};
   VkCommandPool cpool;
   CK(vkCreateCommandPool(dev, &cpoci, NULL, &cpool), "Pool");
   VkCommandBufferAllocateInfo cbai = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, .commandPool = cpool, .commandBufferCount = 1};
   VkCommandBuffer cmd;
   CK(vkAllocateCommandBuffers(dev, &cbai, &cmd), "Cmd");
   VkFenceCreateInfo fci = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
   VkFence fence;
   CK(vkCreateFence(dev, &fci, NULL, &fence), "Fence");

   VkCommandBufferBeginInfo bbi = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
   CK(vkBeginCommandBuffer(cmd, &bbi), "Begin");
   VkClearValue clear = {.color = {{0.0f, 0.0f, 1.0f, 1.0f}}};
   VkRenderPassBeginInfo rpbi = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass = rp,
      .framebuffer = fb,
      .renderArea = {{0, 0}, {W, H}},
      .clearValueCount = 1,
      .pClearValues = &clear,
   };
   vkCmdBeginRenderPass(cmd, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
   vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe);
   VkDeviceSize off = 0;
   vkCmdBindVertexBuffers(cmd, 0, 1, &vbuf, &off);
   vkCmdDraw(cmd, 3, 1, 0, 0);
   vkCmdEndRenderPass(cmd);

   VkImageMemoryBarrier bar = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = img,
      .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1},
   };
   vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &bar);
   VkBufferImageCopy region = {.imageSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .layerCount = 1}, .imageExtent = {W, H, 1}};
   vkCmdCopyImageToBuffer(cmd, img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, rbuf, 1, &region);
   CK(vkEndCommandBuffer(cmd), "End");

   VkSubmitInfo si = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = &cmd};
   CK(vkQueueSubmit(queue, 1, &si, fence), "Submit");
   CK(vkWaitForFences(dev, 1, &fence, VK_TRUE, 30 * 1000 * 1000 * 1000ULL), "Wait");

   /* Check vkMapMemory of readback buffer */
   void *rp_;
   CK(vkMapMemory(dev, rmem, 0, W * H * 4, 0, &rp_), "RMap");
   uint8_t *rpx = (uint8_t *)rp_;
   printf("Vulkan-RBuffer interior RGBA=%u %u %u %u\n", rpx[(8*W+8)*4+0], rpx[(8*W+8)*4+1], rpx[(8*W+8)*4+2], rpx[(8*W+8)*4+3]);
   printf("Vulkan-RBuffer clear    RGBA=%u %u %u %u\n", rpx[(55*W+55)*4+0], rpx[(55*W+55)*4+1], rpx[(55*W+55)*4+2], rpx[(55*W+55)*4+3]);
   vkUnmapMemory(dev, rmem);

   /* Check AHB lock */
   void *data = NULL;
   int32_t lock_r = AHardwareBuffer_lock(ahb, AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN, -1, NULL, &data);
   if (lock_r == 0 && data) {
      uint8_t *px = (uint8_t *)data;
      uint8_t *in_px = px + (8 * out_desc.stride + 8) * 4;
      uint8_t *clear_px = px + (55 * out_desc.stride + 55) * 4;
      printf("AHB-lock       interior RGBA=%u %u %u %u\n", in_px[0], in_px[1], in_px[2], in_px[3]);
      int nz=0; for(int i=0;i<69632;i++) if(px[i]) nz++; printf("AHB-lock non-zero: %d\n", nz); printf("AHB-lock       clear    RGBA=%u %u %u %u\n", clear_px[0], clear_px[1], clear_px[2], clear_px[3]);
      AHardwareBuffer_unlock(ahb, NULL);
   }

   return 0;
}
