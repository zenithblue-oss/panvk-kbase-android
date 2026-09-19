#include "vk_test.h"
#include <pthread.h>

static int finish_case(struct vkf_ctx *c,const char *extension,int ok,const char *behavior)
{
   printf("{\"extension\":\"%s\",\"status\":\"%s\",\"evidence\":\"%s\"}\n",
          extension,ok?"PASS":"FAIL",behavior);
   vkf_finish(c); return !ok;
}

static int run_maintenance9(const char *path)
{
   void *lib=dlopen(path,RTLD_NOW|RTLD_LOCAL);
   PFN_vkGetInstanceProcAddr gipa=(PFN_vkGetInstanceProcAddr)dlsym(lib,"vk_icdGetInstanceProcAddr");
   PFN_vkCreateInstance create_instance=(PFN_vkCreateInstance)gipa(NULL,"vkCreateInstance");
   VkApplicationInfo app={.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO,.apiVersion=VK_API_VERSION_1_3};
   VkInstanceCreateInfo ici={.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,.pApplicationInfo=&app};
   VkInstance instance; if(create_instance(&ici,NULL,&instance)!=VK_SUCCESS)return 1;
   PFN_vkEnumeratePhysicalDevices enumerate=(PFN_vkEnumeratePhysicalDevices)gipa(instance,"vkEnumeratePhysicalDevices");
   PFN_vkCreateDevice create_device=(PFN_vkCreateDevice)gipa(instance,"vkCreateDevice");
   PFN_vkDestroyDevice destroy_device=(PFN_vkDestroyDevice)gipa(instance,"vkDestroyDevice");
   PFN_vkDestroyInstance destroy_instance=(PFN_vkDestroyInstance)gipa(instance,"vkDestroyInstance");
   uint32_t count=1;VkPhysicalDevice physical;enumerate(instance,&count,&physical);
   VkPhysicalDeviceMaintenance9FeaturesKHR feature={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_9_FEATURES_KHR,.maintenance9=VK_TRUE};
   const char *extension=VK_KHR_MAINTENANCE_9_EXTENSION_NAME;
   VkDeviceCreateInfo dci={.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,.pNext=&feature,
      .enabledExtensionCount=1,.ppEnabledExtensionNames=&extension};
   VkDevice device;VkResult result=create_device(physical,&dci,NULL,&device);
   printf("{\"extension\":\"%s\",\"status\":\"%s\",\"evidence\":\"zero-queue vkCreateDevice=%d\"}\n",
          extension,result==VK_SUCCESS?"PASS":"FAIL",result);
   if(result==VK_SUCCESS)destroy_device(device,NULL);destroy_instance(instance,NULL);dlclose(lib);
   return result!=VK_SUCCESS;
}

struct submit_data { struct vkf_ctx *c; VkResult result; };
static void *submit_empty(void *opaque)
{
   struct submit_data *data=opaque;
   VkSubmitInfo submit={.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO};
   data->result=vkQueueSubmit(data->c->queue,1,&submit,VK_NULL_HANDLE);
   return NULL;
}

static int run_internal_queues(const char *path)
{
   VkPhysicalDeviceInternallySynchronizedQueuesFeaturesKHR feature={
      .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INTERNALLY_SYNCHRONIZED_QUEUES_FEATURES_KHR,
      .internallySynchronizedQueues=VK_TRUE};
   const char *extension=VK_KHR_INTERNALLY_SYNCHRONIZED_QUEUES_EXTENSION_NAME;
   struct vkf_ctx c;if(vkf_init(&c,path,&feature,NULL,&extension,1))return 1;
   enum {N=8};pthread_t threads[N];struct submit_data data[N];
   for(int i=0;i<N;i++){data[i]=(struct submit_data){&c,VK_ERROR_UNKNOWN};pthread_create(&threads[i],NULL,submit_empty,&data[i]);}
   int ok=1;for(int i=0;i<N;i++){pthread_join(threads[i],NULL);ok&=data[i].result==VK_SUCCESS;}
   ok&=vkQueueWaitIdle(c.queue)==VK_SUCCESS;
   return finish_case(&c,extension,ok,"8 concurrent unsynchronized vkQueueSubmit calls completed");
}

static int run_barrier8(const char *path)
{
   VkPhysicalDeviceMaintenance8FeaturesKHR feature={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_8_FEATURES_KHR,.maintenance8=VK_TRUE};
   const char *extension=VK_KHR_MAINTENANCE_8_EXTENSION_NAME;
   struct vkf_ctx c;if(vkf_init(&c,path,&feature,NULL,&extension,1))return 1;
   PFN_vkCmdPipelineBarrier2 barrier2=(PFN_vkCmdPipelineBarrier2)c.gipa(c.instance,"vkCmdPipelineBarrier2");
   VkMemoryBarrierAccessFlags3KHR access3={.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER_ACCESS_FLAGS_3_KHR,
      .srcAccessMask3=VK_ACCESS_3_NONE_KHR,.dstAccessMask3=VK_ACCESS_3_NONE_KHR};
   VkMemoryBarrier2 barrier={.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,.pNext=&access3,
      .srcStageMask=VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,.dstStageMask=VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT};
   VkDependencyInfo dependency={.sType=VK_STRUCTURE_TYPE_DEPENDENCY_INFO,.memoryBarrierCount=1,.pMemoryBarriers=&barrier};
   VkCommandBuffer cmd=vkf_command(&c);barrier2(cmd,&dependency);
   int ok=!vkf_submit(&c,cmd);
   return finish_case(&c,extension,ok,"submitted VkMemoryBarrierAccessFlags3KHR via vkCmdPipelineBarrier2");
}

static int run_unified(const char *path)
{
   VkPhysicalDeviceUnifiedImageLayoutsFeaturesKHR feature={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_UNIFIED_IMAGE_LAYOUTS_FEATURES_KHR,.unifiedImageLayouts=VK_TRUE};
   const char *extension=VK_KHR_UNIFIED_IMAGE_LAYOUTS_EXTENSION_NAME;
   struct vkf_ctx c;if(vkf_init(&c,path,&feature,NULL,&extension,1))return 1;
   struct vkf_buffer staging;if(vkf_buffer_create(&c,4,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,0,&staging))return 1;
   uint32_t *p=vkf_map(&c,&staging);*p=0x44332211;vkUnmapMemory(c.device,staging.memory);
   VkImageCreateInfo ici={.sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,.imageType=VK_IMAGE_TYPE_2D,.format=VK_FORMAT_R8G8B8A8_UNORM,
      .extent={1,1,1},.mipLevels=1,.arrayLayers=1,.samples=VK_SAMPLE_COUNT_1_BIT,.tiling=VK_IMAGE_TILING_OPTIMAL,
      .usage=VK_IMAGE_USAGE_TRANSFER_DST_BIT,.sharingMode=VK_SHARING_MODE_EXCLUSIVE};
   VkImage image;VKF_CHECK(vkCreateImage(c.device,&ici,NULL,&image));VkMemoryRequirements req;vkGetImageMemoryRequirements(c.device,image,&req);
   uint32_t mt=vkf_memory_type(&c,req.memoryTypeBits,0);VkMemoryAllocateInfo ai={.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,.allocationSize=req.size,.memoryTypeIndex=mt};
   VkDeviceMemory memory;VKF_CHECK(vkAllocateMemory(c.device,&ai,NULL,&memory));VKF_CHECK(vkBindImageMemory(c.device,image,memory,0));
   VkCommandBuffer cmd=vkf_command(&c);VkImageMemoryBarrier ib={.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT,.oldLayout=VK_IMAGE_LAYOUT_UNDEFINED,.newLayout=VK_IMAGE_LAYOUT_GENERAL,
      .srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.image=image,
      .subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1}};
   vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,NULL,0,NULL,1,&ib);
   VkBufferImageCopy copy={.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1},.imageExtent={1,1,1}};
   vkCmdCopyBufferToImage(cmd,staging.buffer,image,VK_IMAGE_LAYOUT_GENERAL,1,&copy);
   int ok=!vkf_submit(&c,cmd);vkDestroyImage(c.device,image,NULL);vkFreeMemory(c.device,memory,NULL);vkf_buffer_destroy(&c,&staging);
   return finish_case(&c,extension,ok,"real image transitioned and copied in VK_IMAGE_LAYOUT_GENERAL");
}

static int run_copy_indirect(const char *path)
{
   VkPhysicalDeviceCopyMemoryIndirectFeaturesKHR feature={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COPY_MEMORY_INDIRECT_FEATURES_KHR,.indirectMemoryCopy=VK_TRUE};
   VkPhysicalDeviceBufferDeviceAddressFeatures bda={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,.pNext=&feature,.bufferDeviceAddress=VK_TRUE};
   const char *extension=VK_KHR_COPY_MEMORY_INDIRECT_EXTENSION_NAME;
   struct vkf_ctx c;if(vkf_init(&c,path,&bda,NULL,&extension,1))return 1;
   PFN_vkCmdCopyMemoryIndirectKHR copy=(PFN_vkCmdCopyMemoryIndirectKHR)c.gipa(c.instance,"vkCmdCopyMemoryIndirectKHR");
   struct vkf_buffer src,dst,commands;
   VkBufferUsageFlags usage=VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT|VK_BUFFER_USAGE_TRANSFER_SRC_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT;
   if(vkf_buffer_create(&c,16,usage,VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,&src)||
      vkf_buffer_create(&c,16,usage,VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,&dst)||
      vkf_buffer_create(&c,sizeof(VkCopyMemoryIndirectCommandKHR),usage,VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,&commands))return 1;
   uint32_t *p=vkf_map(&c,&src);p[0]=0x13579bdf;vkUnmapMemory(c.device,src.memory);
   p=vkf_map(&c,&dst);p[0]=0;vkUnmapMemory(c.device,dst.memory);
   VkBufferDeviceAddressInfo ai={.sType=VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
   ai.buffer=src.buffer;VkDeviceAddress src_address=vkGetBufferDeviceAddress(c.device,&ai);
   ai.buffer=dst.buffer;VkDeviceAddress dst_address=vkGetBufferDeviceAddress(c.device,&ai);
   VkCopyMemoryIndirectCommandKHR *command=vkf_map(&c,&commands);*command=(VkCopyMemoryIndirectCommandKHR){src_address,dst_address,4};vkUnmapMemory(c.device,commands.memory);
   ai.buffer=commands.buffer;VkDeviceAddress command_address=vkGetBufferDeviceAddress(c.device,&ai);
   VkCopyMemoryIndirectInfoKHR info={.sType=VK_STRUCTURE_TYPE_COPY_MEMORY_INDIRECT_INFO_KHR,.copyCount=1,
      .copyAddressRange={command_address,sizeof(*command),sizeof(*command)}};
   VkCommandBuffer cmd=vkf_command(&c);copy(cmd,&info);int ok=!vkf_submit(&c,cmd);
   p=vkf_map(&c,&dst);ok&=p[0]==0x13579bdf;uint32_t got=p[0];vkUnmapMemory(c.device,dst.memory);
   char evidence[96];snprintf(evidence,sizeof(evidence),"vkCmdCopyMemoryIndirectKHR readback=0x%08x",got);
   vkf_buffer_destroy(&c,&commands);vkf_buffer_destroy(&c,&dst);vkf_buffer_destroy(&c,&src);
   return finish_case(&c,extension,ok,evidence);
}

static int run_maintenance7(const char *path)
{
   VkPhysicalDeviceMaintenance7FeaturesKHR feature={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_7_FEATURES_KHR,.maintenance7=VK_TRUE};
   VkPhysicalDeviceDynamicRenderingFeatures dynamic={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,.pNext=&feature,.dynamicRendering=VK_TRUE};
   const char *extension=VK_KHR_MAINTENANCE_7_EXTENSION_NAME;
   struct vkf_ctx c;if(vkf_init(&c,path,&dynamic,NULL,&extension,1))return 1;
   VkCommandBuffer cmd=vkf_command(&c);VkRenderingInfo rendering={.sType=VK_STRUCTURE_TYPE_RENDERING_INFO,
      .flags=VK_RENDERING_CONTENTS_INLINE_BIT_KHR,.renderArea={{0,0},{1,1}},.layerCount=1};
   vkCmdBeginRendering(cmd,&rendering);vkCmdEndRendering(cmd);
   int ok=!vkf_submit(&c,cmd);
   return finish_case(&c,extension,ok,"dynamic rendering submitted with VK_RENDERING_CONTENTS_INLINE_BIT_KHR");
}

int main(int argc,char **argv)
{
   if(argc!=3){fprintf(stderr,"usage: %s <ICD.so> <case>\n",argv[0]);return 2;}
   if(!strcmp(argv[2],"maintenance8"))return run_barrier8(argv[1]);
   if(!strcmp(argv[2],"maintenance9"))return run_maintenance9(argv[1]);
   if(!strcmp(argv[2],"internal-queues"))return run_internal_queues(argv[1]);
   if(!strcmp(argv[2],"unified"))return run_unified(argv[1]);
   if(!strcmp(argv[2],"copy-indirect"))return run_copy_indirect(argv[1]);
   if(!strcmp(argv[2],"maintenance7"))return run_maintenance7(argv[1]);
   return 2;
}
