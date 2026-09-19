#include "vk_test.h"

#include "phase7-derivatives-quads.spv.h"
#include "phase7-derivatives-linear.spv.h"
#include "phase7-fma.spv.h"
#include "phase7-relaxed.spv.h"
#include "phase7-untyped.spv.h"
#include "phase7-constant-data.spv.h"
#include "phase7-user-type.spv.h"

struct shader_case {
   const char *extension;
   const uint32_t *code;
   size_t code_size;
   uint32_t expected[4];
   uint32_t count;
};

static int
run_shader(struct vkf_ctx *c, const struct shader_case *test)
{
   struct vkf_buffer output;
   if (vkf_buffer_create(c, 16, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 0, &output)) return 1;
   uint32_t *mapped = vkf_map(c, &output);
   memset(mapped, 0xcd, 16);
   vkUnmapMemory(c->device, output.memory);

   VkDescriptorSetLayoutBinding binding = {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
                                             VK_SHADER_STAGE_COMPUTE_BIT, NULL};
   VkDescriptorSetLayoutCreateInfo dsli = {.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount=1,.pBindings=&binding};
   VkDescriptorSetLayout dsl;
   VKF_CHECK(vkCreateDescriptorSetLayout(c->device,&dsli,NULL,&dsl));
   VkDescriptorPoolSize pool_size={VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1};
   VkDescriptorPoolCreateInfo dpi={.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets=1,.poolSizeCount=1,.pPoolSizes=&pool_size};
   VkDescriptorPool pool; VKF_CHECK(vkCreateDescriptorPool(c->device,&dpi,NULL,&pool));
   VkDescriptorSetAllocateInfo dai={.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool=pool,.descriptorSetCount=1,.pSetLayouts=&dsl};
   VkDescriptorSet set; VKF_CHECK(vkAllocateDescriptorSets(c->device,&dai,&set));
   VkDescriptorBufferInfo dbi={output.buffer,0,16};
   VkWriteDescriptorSet write={.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,.dstSet=set,
      .descriptorCount=1,.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,.pBufferInfo=&dbi};
   vkUpdateDescriptorSets(c->device,1,&write,0,NULL);

   VkShaderModule module=vkf_shader(c,test->code,test->code_size);
   if(!module){printf("{\"extension\":\"%s\",\"status\":\"FAIL\",\"evidence\":\"vkCreateShaderModule\"}\n",test->extension);return 1;}
   VkPipelineLayoutCreateInfo pli={.sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount=1,.pSetLayouts=&dsl};
   VkPipelineLayout layout; VKF_CHECK(vkCreatePipelineLayout(c->device,&pli,NULL,&layout));
   VkComputePipelineCreateInfo pci={.sType=VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      .stage={.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,.stage=VK_SHADER_STAGE_COMPUTE_BIT,
              .module=module,.pName="main"},.layout=layout};
   VkPipeline pipeline;
   VkResult pipeline_result=vkCreateComputePipelines(c->device,VK_NULL_HANDLE,1,&pci,NULL,&pipeline);
   if(pipeline_result!=VK_SUCCESS){printf("{\"extension\":\"%s\",\"status\":\"FAIL\",\"evidence\":\"vkCreateComputePipelines=%d\"}\n",test->extension,pipeline_result);return 1;}
   VkCommandBuffer cmd=vkf_command(c);
   vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_COMPUTE,pipeline);
   vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_COMPUTE,layout,0,1,&set,0,NULL);
   vkCmdDispatch(cmd,1,1,1);
   if(vkf_submit(c,cmd)) return 1;
   mapped=vkf_map(c,&output);
   int ok=1;
   for(uint32_t i=0;i<test->count;i++) ok &= mapped[i]==test->expected[i];
   printf("{\"extension\":\"%s\",\"status\":\"%s\",\"evidence\":{\"pipeline\":true,\"dispatch\":true,\"readback\":[%u,%u,%u,%u]}}\n",
          test->extension,ok?"PASS":"FAIL",mapped[0],mapped[1],mapped[2],mapped[3]);
   vkUnmapMemory(c->device,output.memory);
   vkDestroyPipeline(c->device,pipeline,NULL);vkDestroyPipelineLayout(c->device,layout,NULL);
   vkDestroyShaderModule(c->device,module,NULL);vkDestroyDescriptorPool(c->device,pool,NULL);
   vkDestroyDescriptorSetLayout(c->device,dsl,NULL);vkf_buffer_destroy(c,&output);
   return !ok;
}

int main(int argc,char **argv)
{
   if(argc!=3){fprintf(stderr,"usage: %s <ICD.so> <case>\n",argv[0]);return 2;}
   VkPhysicalDeviceComputeShaderDerivativesFeaturesKHR derivatives={
      .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COMPUTE_SHADER_DERIVATIVES_FEATURES_KHR,
      .computeDerivativeGroupQuads=VK_TRUE,.computeDerivativeGroupLinear=VK_TRUE};
   VkPhysicalDeviceShaderFmaFeaturesKHR fma={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FMA_FEATURES_KHR,
      .shaderFmaFloat32=VK_TRUE};
   VkPhysicalDeviceShaderRelaxedExtendedInstructionFeaturesKHR relaxed={
      .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_RELAXED_EXTENDED_INSTRUCTION_FEATURES_KHR,
      .shaderRelaxedExtendedInstruction=VK_TRUE};
   VkPhysicalDeviceShaderUntypedPointersFeaturesKHR untyped={
      .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_UNTYPED_POINTERS_FEATURES_KHR,
      .shaderUntypedPointers=VK_TRUE};
   struct shader_case test={0}; void *feature=NULL;
   if(!strcmp(argv[2],"quads")){test=(struct shader_case){VK_KHR_COMPUTE_SHADER_DERIVATIVES_EXTENSION_NAME,
      (const uint32_t *)phase7_derivatives_quads_spv,phase7_derivatives_quads_spv_len,{0x40400000,0x40400000,0x40400000,0x40400000},4};feature=&derivatives;}
   else if(!strcmp(argv[2],"linear")){test=(struct shader_case){VK_KHR_COMPUTE_SHADER_DERIVATIVES_EXTENSION_NAME,
      (const uint32_t *)phase7_derivatives_linear_spv,phase7_derivatives_linear_spv_len,{0x40400000,0x40400000,0x40400000,0x40400000},4};feature=&derivatives;}
   else if(!strcmp(argv[2],"fma")){test=(struct shader_case){VK_KHR_SHADER_FMA_EXTENSION_NAME,
      (const uint32_t *)phase7_fma_spv,phase7_fma_spv_len,{0x41200000},1};feature=&fma;}
   else if(!strcmp(argv[2],"relaxed")){test=(struct shader_case){VK_KHR_SHADER_RELAXED_EXTENDED_INSTRUCTION_EXTENSION_NAME,
      (const uint32_t *)phase7_relaxed_spv,phase7_relaxed_spv_len,{0xcdcdcdcd},1};feature=&relaxed;}
   else if(!strcmp(argv[2],"untyped")){test=(struct shader_case){VK_KHR_SHADER_UNTYPED_POINTERS_EXTENSION_NAME,
      (const uint32_t *)phase7_untyped_spv,phase7_untyped_spv_len,{42},1};feature=&untyped;}
   else if(!strcmp(argv[2],"constant-data")){test=(struct shader_case){VK_KHR_SHADER_CONSTANT_DATA_EXTENSION_NAME,
      (const uint32_t *)phase7_constant_data_spv,phase7_constant_data_spv_len,{42},1};}
   else if(!strcmp(argv[2],"user-type")){test=(struct shader_case){VK_GOOGLE_USER_TYPE_EXTENSION_NAME,
      (const uint32_t *)phase7_user_type_spv,phase7_user_type_spv_len,{42},1};}
   else return 2;
   const char *extensions[]={test.extension};
   struct vkf_ctx c;if(vkf_init(&c,argv[1],feature,NULL,extensions,1))return 1;
   int fail=run_shader(&c,&test);vkf_finish(&c);return fail;
}
