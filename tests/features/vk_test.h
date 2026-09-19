#ifndef PANVK_FEATURE_VK_TEST_H
#define PANVK_FEATURE_VK_TEST_H

#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#define VKF_LIST(X) \
   X(EnumeratePhysicalDevices) X(GetPhysicalDeviceProperties) \
   X(GetPhysicalDeviceProperties2) X(GetPhysicalDeviceFeatures2) \
   X(GetPhysicalDeviceQueueFamilyProperties) X(GetPhysicalDeviceMemoryProperties) \
   X(GetPhysicalDeviceFormatProperties) X(CreateDevice) X(GetDeviceQueue) \
   X(CreateBuffer) X(GetBufferMemoryRequirements) X(AllocateMemory) \
   X(BindBufferMemory) X(MapMemory) X(UnmapMemory) X(FreeMemory) X(DestroyBuffer) \
   X(CreateImage) X(GetImageMemoryRequirements) X(BindImageMemory) X(DestroyImage) \
   X(CreateImageView) X(DestroyImageView) X(CreateSampler) X(DestroySampler) \
   X(CreateDescriptorSetLayout) X(DestroyDescriptorSetLayout) \
   X(CreateDescriptorPool) X(DestroyDescriptorPool) X(AllocateDescriptorSets) \
   X(UpdateDescriptorSets) X(CreateShaderModule) X(DestroyShaderModule) \
   X(CreatePipelineLayout) X(DestroyPipelineLayout) X(CreateComputePipelines) \
   X(CreateGraphicsPipelines) X(DestroyPipeline) X(CreateCommandPool) \
   X(DestroyCommandPool) X(AllocateCommandBuffers) X(BeginCommandBuffer) \
   X(EndCommandBuffer) X(CmdBindPipeline) X(CmdBindDescriptorSets) \
   X(CmdPushConstants) X(CmdDispatch) X(CmdPipelineBarrier) X(CmdCopyBufferToImage) \
   X(CmdCopyImageToBuffer) X(CmdDraw) X(CmdSetLineWidth) X(CreateFence) \
   X(DestroyFence) X(QueueSubmit) X(WaitForFences) X(QueueWaitIdle) \
   X(GetBufferDeviceAddress) X(CmdBeginRendering) X(CmdEndRendering) X(DestroyDevice) \
   X(DestroyInstance)

#define DECL(name) static PFN_vk##name vk##name __attribute__((unused));
VKF_LIST(DECL)
#undef DECL
static PFN_vkCmdPushDescriptorSetKHR vkCmdPushDescriptorSetKHR __attribute__((unused));

struct vkf_ctx {
   void *lib;
   PFN_vkGetInstanceProcAddr gipa;
   VkInstance instance;
   VkPhysicalDevice physical;
   VkPhysicalDeviceProperties props;
   VkPhysicalDeviceMemoryProperties memory;
   VkDevice device;
   VkQueue queue;
   uint32_t family;
   VkCommandPool pool;
};

struct vkf_buffer {
   VkBuffer buffer;
   VkDeviceMemory memory;
   VkDeviceSize size;
};

#define VKF_CHECK(x) do { VkResult r_ = (x); if (r_ != VK_SUCCESS) { \
   fprintf(stderr, "FAIL:%s:%d result=%d\n", #x, __LINE__, r_); return 1; } } while (0)

static __attribute__((unused)) uint32_t
vkf_memory_type(struct vkf_ctx *c, uint32_t bits, VkMemoryPropertyFlags flags)
{
   for (uint32_t i = 0; i < c->memory.memoryTypeCount; i++)
      if ((bits & (1u << i)) &&
          (c->memory.memoryTypes[i].propertyFlags & flags) == flags)
         return i;
   return UINT32_MAX;
}

static __attribute__((unused)) int
vkf_init(struct vkf_ctx *c, const char *path, void *feature_chain,
         const VkPhysicalDeviceFeatures *core, const char *const *extensions,
         uint32_t extension_count)
{
   memset(c, 0, sizeof(*c));
   c->lib = dlopen(path, RTLD_NOW | RTLD_LOCAL);
   if (!c->lib) { fprintf(stderr, "dlopen: %s\n", dlerror()); return 1; }
   c->gipa = (PFN_vkGetInstanceProcAddr)dlsym(c->lib, "vk_icdGetInstanceProcAddr");
   if (!c->gipa) c->gipa = (PFN_vkGetInstanceProcAddr)dlsym(c->lib, "vkGetInstanceProcAddr");
   PFN_vkCreateInstance create = (PFN_vkCreateInstance)c->gipa(NULL, "vkCreateInstance");
   VkApplicationInfo app = { .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                             .apiVersion = VK_API_VERSION_1_3 };
   VkInstanceCreateInfo ici = { .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                                .pApplicationInfo = &app };
   VKF_CHECK(create(&ici, NULL, &c->instance));
#define LOAD(name) vk##name = (PFN_vk##name)c->gipa(c->instance, "vk" #name);
   VKF_LIST(LOAD)
#undef LOAD
   uint32_t count = 0;
   VKF_CHECK(vkEnumeratePhysicalDevices(c->instance, &count, NULL));
   VkPhysicalDevice *devices = calloc(count, sizeof(*devices));
   VKF_CHECK(vkEnumeratePhysicalDevices(c->instance, &count, devices));
   for (uint32_t i = 0; i < count; i++) {
      vkGetPhysicalDeviceProperties(devices[i], &c->props);
      if (strstr(c->props.deviceName, "Mali")) { c->physical = devices[i]; break; }
   }
   free(devices);
   if (!c->physical) { fprintf(stderr, "no Mali device\n"); return 1; }
   vkGetPhysicalDeviceMemoryProperties(c->physical, &c->memory);
   uint32_t qcount = 0;
   vkGetPhysicalDeviceQueueFamilyProperties(c->physical, &qcount, NULL);
   VkQueueFamilyProperties *queues = calloc(qcount, sizeof(*queues));
   vkGetPhysicalDeviceQueueFamilyProperties(c->physical, &qcount, queues);
   c->family = UINT32_MAX;
   for (uint32_t i = 0; i < qcount; i++)
      if ((queues[i].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) ==
          (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) { c->family = i; break; }
   free(queues);
   if (c->family == UINT32_MAX) { fprintf(stderr, "no graphics+compute queue\n"); return 1; }
   float priority = 1.0f;
   VkDeviceQueueCreateInfo qci = { .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = c->family, .queueCount = 1, .pQueuePriorities = &priority };
   VkDeviceCreateInfo dci = { .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext = feature_chain, .queueCreateInfoCount = 1, .pQueueCreateInfos = &qci,
      .enabledExtensionCount = extension_count, .ppEnabledExtensionNames = extensions,
      .pEnabledFeatures = core };
   VKF_CHECK(vkCreateDevice(c->physical, &dci, NULL, &c->device));
   vkGetDeviceQueue(c->device, c->family, 0, &c->queue);
   vkCmdPushDescriptorSetKHR = (PFN_vkCmdPushDescriptorSetKHR)c->gipa(c->instance, "vkCmdPushDescriptorSetKHR");
   VkCommandPoolCreateInfo pci = { .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                                   .queueFamilyIndex = c->family };
   VKF_CHECK(vkCreateCommandPool(c->device, &pci, NULL, &c->pool));
   return 0;
}

static __attribute__((unused)) int
vkf_buffer_create(struct vkf_ctx *c, VkDeviceSize size, VkBufferUsageFlags usage,
                  VkMemoryAllocateFlags alloc_flags, struct vkf_buffer *out)
{
   out->size = size;
   VkBufferCreateInfo bi = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                             .size = size, .usage = usage,
                             .sharingMode = VK_SHARING_MODE_EXCLUSIVE };
   VKF_CHECK(vkCreateBuffer(c->device, &bi, NULL, &out->buffer));
   VkMemoryRequirements req;
   vkGetBufferMemoryRequirements(c->device, out->buffer, &req);
   uint32_t type = vkf_memory_type(c, req.memoryTypeBits,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
   if (type == UINT32_MAX) { fprintf(stderr, "no coherent host memory\n"); return 1; }
   VkMemoryAllocateFlagsInfo afi = { .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
                                     .flags = alloc_flags };
   VkMemoryAllocateInfo ai = { .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .pNext = alloc_flags ? &afi : NULL, .allocationSize = req.size, .memoryTypeIndex = type };
   VKF_CHECK(vkAllocateMemory(c->device, &ai, NULL, &out->memory));
   VKF_CHECK(vkBindBufferMemory(c->device, out->buffer, out->memory, 0));
   return 0;
}

static __attribute__((unused)) void *vkf_map(struct vkf_ctx *c, struct vkf_buffer *b)
{
   void *p = NULL;
   if (vkMapMemory(c->device, b->memory, 0, b->size, 0, &p) != VK_SUCCESS) return NULL;
   return p;
}

static __attribute__((unused)) VkShaderModule
vkf_shader(struct vkf_ctx *c, const uint32_t *code, size_t size)
{
   VkShaderModule module = VK_NULL_HANDLE;
   VkShaderModuleCreateInfo ci = { .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                                   .codeSize = size, .pCode = code };
   if (vkCreateShaderModule(c->device, &ci, NULL, &module) != VK_SUCCESS) return VK_NULL_HANDLE;
   return module;
}

static __attribute__((unused)) int
vkf_submit(struct vkf_ctx *c, VkCommandBuffer cmd)
{
   VKF_CHECK(vkEndCommandBuffer(cmd));
   VkFence fence;
   VkFenceCreateInfo fi = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
   VKF_CHECK(vkCreateFence(c->device, &fi, NULL, &fence));
   VkSubmitInfo si = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                       .commandBufferCount = 1, .pCommandBuffers = &cmd };
   VKF_CHECK(vkQueueSubmit(c->queue, 1, &si, fence));
   VkResult r = vkWaitForFences(c->device, 1, &fence, VK_TRUE, 30000000000ULL);
   vkDestroyFence(c->device, fence, NULL);
   if (r != VK_SUCCESS) { fprintf(stderr, "submit wait result=%d\n", r); return 1; }
   return 0;
}

static __attribute__((unused)) VkCommandBuffer
vkf_command(struct vkf_ctx *c)
{
   VkCommandBuffer cmd = VK_NULL_HANDLE;
   VkCommandBufferAllocateInfo ai = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = c->pool, .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, .commandBufferCount = 1 };
   if (vkAllocateCommandBuffers(c->device, &ai, &cmd) != VK_SUCCESS) return VK_NULL_HANDLE;
   VkCommandBufferBeginInfo bi = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                   .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
   if (vkBeginCommandBuffer(cmd, &bi) != VK_SUCCESS) return VK_NULL_HANDLE;
   return cmd;
}

static __attribute__((unused)) void
vkf_buffer_destroy(struct vkf_ctx *c, struct vkf_buffer *b)
{
   vkDestroyBuffer(c->device, b->buffer, NULL);
   vkFreeMemory(c->device, b->memory, NULL);
}

static __attribute__((unused)) void
vkf_finish(struct vkf_ctx *c)
{
   vkQueueWaitIdle(c->queue);
   vkDestroyCommandPool(c->device, c->pool, NULL);
   vkDestroyDevice(c->device, NULL);
   vkDestroyInstance(c->instance, NULL);
   dlclose(c->lib);
}
#endif
