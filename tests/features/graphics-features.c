#include "vk_test.h"
#include "draw.vert.spv.h"
#include "draw.frag.spv.h"

#define W 64
#define H 64

int main(int argc, char **argv)
{
   if(argc!=2){fprintf(stderr,"usage: %s <ICD.so>\n",argv[0]);return 2;}
   VkPhysicalDeviceDynamicRenderingFeatures dynamic={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,.dynamicRendering=VK_TRUE};
   VkPhysicalDeviceFeatures core={.wideLines=VK_TRUE,.largePoints=VK_TRUE};
   struct vkf_ctx c;if(vkf_init(&c,argv[1],&dynamic,&core,NULL,0))return 1;
   VkPhysicalDeviceProperties props;vkGetPhysicalDeviceProperties(c.physical,&props);
   VkImageCreateInfo ici={.sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,.imageType=VK_IMAGE_TYPE_2D,
      .format=VK_FORMAT_R8G8B8A8_UNORM,.extent={W,H,1},.mipLevels=1,.arrayLayers=1,
      .samples=VK_SAMPLE_COUNT_1_BIT,.tiling=VK_IMAGE_TILING_OPTIMAL,
      .usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
      .sharingMode=VK_SHARING_MODE_EXCLUSIVE};
   VkImage image;VKF_CHECK(vkCreateImage(c.device,&ici,NULL,&image));
   VkMemoryRequirements req;vkGetImageMemoryRequirements(c.device,image,&req);
   uint32_t mt=vkf_memory_type(&c,req.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
   if(mt==UINT32_MAX)mt=vkf_memory_type(&c,req.memoryTypeBits,0);
   VkMemoryAllocateInfo mai={.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,.allocationSize=req.size,.memoryTypeIndex=mt};
   VkDeviceMemory imem;VKF_CHECK(vkAllocateMemory(c.device,&mai,NULL,&imem));VKF_CHECK(vkBindImageMemory(c.device,image,imem,0));
   VkImageViewCreateInfo ivci={.sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,.image=image,.viewType=VK_IMAGE_VIEW_TYPE_2D,
      .format=VK_FORMAT_R8G8B8A8_UNORM,.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1}};
   VkImageView view;VKF_CHECK(vkCreateImageView(c.device,&ivci,NULL,&view));
   struct vkf_buffer rb;if(vkf_buffer_create(&c,W*H*4,VK_BUFFER_USAGE_TRANSFER_DST_BIT,0,&rb))return 1;
   VkShaderModule vs=vkf_shader(&c,(const uint32_t*)draw_vert_spv,draw_vert_spv_len),fs=vkf_shader(&c,(const uint32_t*)draw_frag_spv,draw_frag_spv_len);
   VkPushConstantRange pcr={VK_SHADER_STAGE_VERTEX_BIT,0,4};
   VkPipelineLayoutCreateInfo plci={.sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,.pushConstantRangeCount=1,.pPushConstantRanges=&pcr};
   VkPipelineLayout layout;VKF_CHECK(vkCreatePipelineLayout(c.device,&plci,NULL,&layout));
   VkPipelineShaderStageCreateInfo stages[2]={
      {.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,.stage=VK_SHADER_STAGE_VERTEX_BIT,.module=vs,.pName="main"},
      {.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,.stage=VK_SHADER_STAGE_FRAGMENT_BIT,.module=fs,.pName="main"}};
   VkPipelineVertexInputStateCreateInfo vi={.sType=VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
   VkPipelineInputAssemblyStateCreateInfo ia={.sType=VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
   VkViewport viewport={0,0,W,H,0,1};VkRect2D scissor={{0,0},{W,H}};
   VkPipelineViewportStateCreateInfo vp={.sType=VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,.viewportCount=1,.pViewports=&viewport,.scissorCount=1,.pScissors=&scissor};
   VkPipelineRasterizationStateCreateInfo rs={.sType=VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,.polygonMode=VK_POLYGON_MODE_FILL,.cullMode=VK_CULL_MODE_NONE,.lineWidth=5};
   VkPipelineMultisampleStateCreateInfo ms={.sType=VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT};
   VkPipelineColorBlendAttachmentState cba={.colorWriteMask=0xf};VkPipelineColorBlendStateCreateInfo cb={.sType=VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,.attachmentCount=1,.pAttachments=&cba};
   VkDynamicState dyns[]={VK_DYNAMIC_STATE_LINE_WIDTH};VkPipelineDynamicStateCreateInfo ds={.sType=VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,.dynamicStateCount=1,.pDynamicStates=dyns};
   VkPipelineRenderingCreateInfo rendering={.sType=VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,.colorAttachmentCount=1,.pColorAttachmentFormats=&ici.format};
   VkGraphicsPipelineCreateInfo gp={.sType=VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,.pNext=&rendering,.stageCount=2,.pStages=stages,.pVertexInputState=&vi,.pInputAssemblyState=&ia,.pViewportState=&vp,.pRasterizationState=&rs,.pMultisampleState=&ms,.pColorBlendState=&cb,.pDynamicState=&ds,.layout=layout};
   VkPipeline pipes[3];VkPrimitiveTopology tops[]={VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,VK_PRIMITIVE_TOPOLOGY_LINE_LIST,VK_PRIMITIVE_TOPOLOGY_POINT_LIST};
   for(int i=0;i<3;i++){ia.topology=tops[i];VKF_CHECK(vkCreateGraphicsPipelines(c.device,VK_NULL_HANDLE,1,&gp,NULL,&pipes[i]));}
   VkCommandBuffer cmd=vkf_command(&c);
   VkImageMemoryBarrier first={.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,.dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,.oldLayout=VK_IMAGE_LAYOUT_UNDEFINED,.newLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.image=image,.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1}};
   vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,0,0,NULL,0,NULL,1,&first);
   VkClearValue clear={.color={{0,0,1,1}}};VkRenderingAttachmentInfo color={.sType=VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,.imageView=view,.imageLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR,.storeOp=VK_ATTACHMENT_STORE_OP_STORE,.clearValue=clear};
   VkRenderingInfo ri={.sType=VK_STRUCTURE_TYPE_RENDERING_INFO,.renderArea={{0,0},{W,H}},.layerCount=1,.colorAttachmentCount=1,.pColorAttachments=&color};
   vkCmdBeginRendering(cmd,&ri);
   for(uint32_t i=0;i<3;i++){vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,pipes[i]);vkCmdPushConstants(cmd,layout,VK_SHADER_STAGE_VERTEX_BIT,0,4,&i);if(i==1)vkCmdSetLineWidth(cmd,5);vkCmdDraw(cmd,i==0?3:i==1?2:1,1,0,0);}
   vkCmdEndRendering(cmd);
   VkImageMemoryBarrier copy=first;copy.srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;copy.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;copy.oldLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;copy.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
   vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,NULL,0,NULL,1,&copy);
   VkBufferImageCopy bic={.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1},.imageExtent={W,H,1}};vkCmdCopyImageToBuffer(cmd,image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,rb.buffer,1,&bic);
   if(vkf_submit(&c,cmd))return 1;
   uint8_t *px=vkf_map(&c,&rb);unsigned red=0,green=0,yellow=0;uint64_t checksum=0;
   for(unsigned i=0;i<W*H;i++){uint8_t*r=px+i*4;checksum=checksum*16777619u^(*(uint32_t*)r);red+=r[0]>200&&r[1]<50;green+=r[1]>200&&r[0]<50;yellow+=r[0]>200&&r[1]>200;}
   vkUnmapMemory(c.device,rb.memory);int ok=red>200&&green>100&&yellow>50;
   printf("{\"feature\":\"dynamicRendering\",\"status\":\"%s\",\"beginEnd\":true,\"trianglePixels\":%u,\"checksum\":\"0x%llx\"}\n",ok?"PASS":"FAIL",red,(unsigned long long)checksum);
   printf("{\"feature\":\"wideLinesLargePoints\",\"status\":\"%s\",\"lineWidth\":5,\"greenPixels\":%u,\"pointSize\":11,\"yellowPixels\":%u,\"lineWidthRange\":[%.1f,%.1f],\"pointSizeRange\":[%.1f,%.1f],\"lineWidthGranularity\":%.3f,\"pointSizeGranularity\":%.3f}\n",ok?"PASS":"FAIL",green,yellow,props.limits.lineWidthRange[0],props.limits.lineWidthRange[1],props.limits.pointSizeRange[0],props.limits.pointSizeRange[1],props.limits.lineWidthGranularity,props.limits.pointSizeGranularity);
   for(int i=0;i<3;i++) vkDestroyPipeline(c.device,pipes[i],NULL);
   vkDestroyPipelineLayout(c.device,layout,NULL);vkDestroyShaderModule(c.device,vs,NULL);vkDestroyShaderModule(c.device,fs,NULL);vkf_buffer_destroy(&c,&rb);vkDestroyImageView(c.device,view,NULL);vkDestroyImage(c.device,image,NULL);vkFreeMemory(c.device,imem,NULL);vkf_finish(&c);return !ok;
}
