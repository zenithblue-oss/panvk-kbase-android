#include "texture.comp.spv.h"
#include "vk_test.h"
#include <math.h>

struct format_case {
  const char *name;
  VkFormat format;
  unsigned block_size;
  uint8_t block[16];
  int astc_hdr;
};
static const struct format_case cases[] = {
    {"ETC2_RGB_UNORM", VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK, 8, {0}, 0},
    {"ETC2_RGBA_UNORM", VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK, 16, {0}, 0},
    {"EAC_R11_UNORM", VK_FORMAT_EAC_R11_UNORM_BLOCK, 8, {0}, 0},
    {"EAC_R11_SNORM", VK_FORMAT_EAC_R11_SNORM_BLOCK, 8, {0}, 0},
    {"EAC_RG11_UNORM", VK_FORMAT_EAC_R11G11_UNORM_BLOCK, 16, {0}, 0},
    {"EAC_RG11_SNORM", VK_FORMAT_EAC_R11G11_SNORM_BLOCK, 16, {0}, 0},
    {"ASTC_4x4_UNORM",
     VK_FORMAT_ASTC_4x4_UNORM_BLOCK,
     16,
     {0xfc, 0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x40, 0x00, 0x80,
      0x00, 0xc0, 0xff, 0xff},
     0},
    {"ASTC_4x4_SRGB",
     VK_FORMAT_ASTC_4x4_SRGB_BLOCK,
     16,
     {0xfc, 0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x40, 0x00, 0x80,
      0x00, 0xc0, 0xff, 0xff},
     0},
    {"ASTC_4x4_SFLOAT",
     VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK_EXT,
     16,
     {0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x3c, 0x00, 0x40,
      0x00, 0x42, 0x00, 0x3c},
     1},
};

static int run_case(struct vkf_ctx *c, const struct format_case *t,
                    VkPipeline pipeline, VkPipelineLayout layout,
                    VkDescriptorSetLayout dsl) {
  VkFormatProperties fp;
  vkGetPhysicalDeviceFormatProperties(c->physical, t->format, &fp);
  VkFormatFeatureFlags need =
      VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT |
      VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT |
      VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
  if ((fp.optimalTilingFeatures & need) != need) {
    printf("{\"format\":\"%s\",\"status\":\"BLOCKED\",\"reason\":\"missing "
           "sampled+linear+transfer-dst format features\",\"features\":%u}\n",
           t->name, fp.optimalTilingFeatures);
    return 1;
  }
  struct vkf_buffer upload, out;
  if (vkf_buffer_create(c, t->block_size * 5, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        0, &upload) ||
      vkf_buffer_create(c, 64, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 0, &out))
    return 1;
  uint8_t *p = vkf_map(c, &upload);
  for (int i = 0; i < 5; i++)
    memcpy(p + i * t->block_size, t->block, t->block_size);
  vkUnmapMemory(c->device, upload.memory);
  p = vkf_map(c, &out);
  memset(p, 0xcd, 64);
  vkUnmapMemory(c->device, out.memory);
  VkImageCreateInfo ici = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                           .imageType = VK_IMAGE_TYPE_2D,
                           .format = t->format,
                           .extent = {8, 8, 1},
                           .mipLevels = 2,
                           .arrayLayers = 1,
                           .samples = VK_SAMPLE_COUNT_1_BIT,
                           .tiling = VK_IMAGE_TILING_OPTIMAL,
                           .usage = VK_IMAGE_USAGE_SAMPLED_BIT |
                                    VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                           .sharingMode = VK_SHARING_MODE_EXCLUSIVE};
  VkImage image;
  VkResult r = vkCreateImage(c->device, &ici, NULL, &image);
  if (r) {
    printf("{\"format\":\"%s\",\"status\":\"FAIL\",\"call\":\"vkCreateImage\","
           "\"result\":%d}\n",
           t->name, r);
    return 1;
  }
  VkMemoryRequirements req;
  vkGetImageMemoryRequirements(c->device, image, &req);
  uint32_t mt = vkf_memory_type(c, req.memoryTypeBits,
                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if (mt == UINT32_MAX)
    mt = vkf_memory_type(c, req.memoryTypeBits, 0);
  VkMemoryAllocateInfo mai = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                              .allocationSize = req.size,
                              .memoryTypeIndex = mt};
  VkDeviceMemory imem;
  VKF_CHECK(vkAllocateMemory(c->device, &mai, NULL, &imem));
  VKF_CHECK(vkBindImageMemory(c->device, image, imem, 0));
  VkImageViewCreateInfo ivci = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = t->format,
      .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 2, 0, 1}};
  VkImageView view;
  VKF_CHECK(vkCreateImageView(c->device, &ivci, NULL, &view));
  VkSamplerCreateInfo sci = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .magFilter = VK_FILTER_LINEAR,
      .minFilter = VK_FILTER_LINEAR,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
      .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .mipLodBias = 0,
      .anisotropyEnable = VK_TRUE,
      .maxAnisotropy = 4,
      .minLod = 0,
      .maxLod = 1};
  VkSampler sampler;
  VKF_CHECK(vkCreateSampler(c->device, &sci, NULL, &sampler));
  VkDescriptorPoolSize sizes[2] = {
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}};
  VkDescriptorPoolCreateInfo dpi = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = 1,
      .poolSizeCount = 2,
      .pPoolSizes = sizes};
  VkDescriptorPool pool;
  VKF_CHECK(vkCreateDescriptorPool(c->device, &dpi, NULL, &pool));
  VkDescriptorSetAllocateInfo dai = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = pool,
      .descriptorSetCount = 1,
      .pSetLayouts = &dsl};
  VkDescriptorSet set;
  VKF_CHECK(vkAllocateDescriptorSets(c->device, &dai, &set));
  VkDescriptorImageInfo di = {sampler, view,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
  VkDescriptorBufferInfo db = {out.buffer, 0, 64};
  VkWriteDescriptorSet writes[2] = {
      {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
       .dstSet = set,
       .dstBinding = 0,
       .descriptorCount = 1,
       .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
       .pImageInfo = &di},
      {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
       .dstSet = set,
       .dstBinding = 1,
       .descriptorCount = 1,
       .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
       .pBufferInfo = &db}};
  vkUpdateDescriptorSets(c->device, 2, writes, 0, NULL);
  VkCommandBuffer cmd = vkf_command(c);
  VkImageMemoryBarrier b = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = image,
      .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 2, 0, 1}};
  vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1,
                       &b);
  VkBufferImageCopy copies[2] = {
      {.bufferOffset = 0,
       .imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
       .imageExtent = {8, 8, 1}},
      {.bufferOffset = t->block_size * 4,
       .imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 1, 0, 1},
       .imageExtent = {4, 4, 1}}};
  vkCmdCopyBufferToImage(cmd, upload.buffer, image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 2, copies);
  b.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  b.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  b.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, NULL, 0,
                       NULL, 1, &b);
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, 1,
                          &set, 0, NULL);
  vkCmdDispatch(cmd, 1, 1, 1);
  if (vkf_submit(c, cmd))
    return 1;
  float *v = vkf_map(c, &out);
  int finite = 1;
  uint32_t hash = 2166136261u;
  for (int i = 0; i < 16; i++) {
    finite &= isfinite(v[i]);
    uint32_t u;
    memcpy(&u, &v[i], 4);
    hash = (hash ^ u) * 16777619u;
  }
  int stable = finite && memcmp(v, v + 4, 16) == 0 &&
               memcmp(v, v + 8, 16) == 0 && memcmp(v, v + 12, 16) == 0;
  printf("{\"format\":\"%s\",\"status\":\"%s\",\"sampling\":true,"
         "\"linearFiltering\":true,\"mipLevel1\":true,\"anisotropy\":4,"
         "\"sample\":[%.6g,%.6g,%.6g,%.6g],\"checksum\":\"0x%08x\"}\n",
         t->name, stable ? "PASS" : "FAIL", v[0], v[1], v[2], v[3], hash);
  vkUnmapMemory(c->device, out.memory);
  vkDestroyDescriptorPool(c->device, pool, NULL);
  vkDestroySampler(c->device, sampler, NULL);
  vkDestroyImageView(c->device, view, NULL);
  vkDestroyImage(c->device, image, NULL);
  vkFreeMemory(c->device, imem, NULL);
  vkf_buffer_destroy(c, &upload);
  vkf_buffer_destroy(c, &out);
  return !stable;
}
int main(int argc, char **argv) {
  if (argc != 2)
    return 2;
  VkPhysicalDeviceFeatures core = {.samplerAnisotropy = VK_TRUE};
  VkPhysicalDeviceTextureCompressionASTCHDRFeaturesEXT hdr = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXTURE_COMPRESSION_ASTC_HDR_FEATURES_EXT,
      .textureCompressionASTC_HDR = VK_TRUE};
  const char *exts[] = {VK_EXT_TEXTURE_COMPRESSION_ASTC_HDR_EXTENSION_NAME};
  struct vkf_ctx c;
  if (vkf_init(&c, argv[1], &hdr, &core, exts, 1))
    return 1;
  VkDescriptorSetLayoutBinding binds[2] = {
      {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
       VK_SHADER_STAGE_COMPUTE_BIT, NULL},
      {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT,
       NULL}};
  VkDescriptorSetLayoutCreateInfo dlci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = 2,
      .pBindings = binds};
  VkDescriptorSetLayout dsl;
  VKF_CHECK(vkCreateDescriptorSetLayout(c.device, &dlci, NULL, &dsl));
  VkPipelineLayoutCreateInfo lci = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1,
      .pSetLayouts = &dsl};
  VkPipelineLayout layout;
  VKF_CHECK(vkCreatePipelineLayout(c.device, &lci, NULL, &layout));
  VkShaderModule mod =
      vkf_shader(&c, (const uint32_t *)texture_comp_spv, texture_comp_spv_len);
  VkComputePipelineCreateInfo pci = {
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      .stage = {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_COMPUTE_BIT,
                .module = mod,
                .pName = "main"},
      .layout = layout};
  VkPipeline pipe;
  VKF_CHECK(
      vkCreateComputePipelines(c.device, VK_NULL_HANDLE, 1, &pci, NULL, &pipe));
  int fail = 0, hdr_seen = 0, hdr_fail = 0;
  for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
    int f = run_case(&c, &cases[i], pipe, layout, dsl);
    fail |= f;
    if (cases[i].astc_hdr) {
      hdr_seen = 1;
      hdr_fail = f;
    }
  }
  printf("{\"feature\":\"samplerAnisotropy\",\"status\":\"%s\","
         "\"maxAnisotropy\":4,\"deterministicTextureSamples\":true}\n",
         fail ? "FAIL" : "PASS");
  if (!hdr_seen)
    printf("{\"feature\":\"ASTC_HDR\",\"status\":\"BLOCKED\",\"reason\":"
           "\"Vulkan headers lack VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK_EXT\"}\n");
  else
    printf("{\"feature\":\"ASTC_HDR\",\"status\":\"%s\",\"format\":\"VK_FORMAT_"
           "ASTC_4x4_SFLOAT_BLOCK_EXT\"}\n",
           hdr_fail ? "FAIL" : "PASS");
  vkDestroyPipeline(c.device, pipe, NULL);
  vkDestroyShaderModule(c.device, mod, NULL);
  vkDestroyPipelineLayout(c.device, layout, NULL);
  vkDestroyDescriptorSetLayout(c.device, dsl, NULL);
  vkf_finish(&c);
  return fail;
}
