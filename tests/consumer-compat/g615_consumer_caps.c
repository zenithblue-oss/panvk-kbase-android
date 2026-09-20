#define VK_USE_PLATFORM_ANDROID_KHR 1
#define VK_ENABLE_BETA_EXTENSIONS 1
#include <dlfcn.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

#define LINK(a, b) ((a).pNext = &(b))
#define B(name, value) printf("\"%s\":%s", name, (value) ? "true" : "false")
#define U(name, value) printf("\"%s\":%" PRIu64, name, (uint64_t)(value))
#define BF(obj, field) do { if (!first) putchar(','); first = 0; B(#field, (obj).field); } while (0)
#define UP(obj, field) do { if (!first) putchar(','); first = 0; U(#field, (obj).field); } while (0)
#define CHAIN(head, obj) do { (obj).pNext = (head); (head) = &(obj); } while (0)

struct fmt { VkFormat value; const char *name; };
static const struct fmt bc[] = {
 {VK_FORMAT_BC1_RGB_UNORM_BLOCK,"VK_FORMAT_BC1_RGB_UNORM_BLOCK"},{VK_FORMAT_BC1_RGB_SRGB_BLOCK,"VK_FORMAT_BC1_RGB_SRGB_BLOCK"},
 {VK_FORMAT_BC1_RGBA_UNORM_BLOCK,"VK_FORMAT_BC1_RGBA_UNORM_BLOCK"},{VK_FORMAT_BC1_RGBA_SRGB_BLOCK,"VK_FORMAT_BC1_RGBA_SRGB_BLOCK"},
 {VK_FORMAT_BC2_UNORM_BLOCK,"VK_FORMAT_BC2_UNORM_BLOCK"},{VK_FORMAT_BC2_SRGB_BLOCK,"VK_FORMAT_BC2_SRGB_BLOCK"},
 {VK_FORMAT_BC3_UNORM_BLOCK,"VK_FORMAT_BC3_UNORM_BLOCK"},{VK_FORMAT_BC3_SRGB_BLOCK,"VK_FORMAT_BC3_SRGB_BLOCK"},
 {VK_FORMAT_BC4_UNORM_BLOCK,"VK_FORMAT_BC4_UNORM_BLOCK"},{VK_FORMAT_BC4_SNORM_BLOCK,"VK_FORMAT_BC4_SNORM_BLOCK"},
 {VK_FORMAT_BC5_UNORM_BLOCK,"VK_FORMAT_BC5_UNORM_BLOCK"},{VK_FORMAT_BC5_SNORM_BLOCK,"VK_FORMAT_BC5_SNORM_BLOCK"},
 {VK_FORMAT_BC6H_UFLOAT_BLOCK,"VK_FORMAT_BC6H_UFLOAT_BLOCK"},{VK_FORMAT_BC6H_SFLOAT_BLOCK,"VK_FORMAT_BC6H_SFLOAT_BLOCK"},
 {VK_FORMAT_BC7_UNORM_BLOCK,"VK_FORMAT_BC7_UNORM_BLOCK"},{VK_FORMAT_BC7_SRGB_BLOCK,"VK_FORMAT_BC7_SRGB_BLOCK"},
};

static void json_string(const char *s) {
 putchar('"');
 for (; *s; s++) { unsigned char c = *s; if (c == '"' || c == '\\') putchar('\\'); if (c >= 32) putchar(c); }
 putchar('"');
}

static int has_extension(const VkExtensionProperties *exts, uint32_t count, const char *name) {
 for (uint32_t i = 0; i < count; i++) if (!strcmp(exts[i].extensionName, name)) return 1;
 return 0;
}

static void json_bytes(const void *data, size_t size) {
 const unsigned char *p = data; putchar('"');
 for (size_t i = 0; i < size; i++) printf("%02x", p[i]);
 putchar('"');
}

int main(int argc, char **argv) {
 if (argc != 2) { fprintf(stderr, "usage: %s ICD\n", argv[0]); return 2; }
 void *lib = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
 if (!lib) { fprintf(stderr, "dlopen: %s\n", dlerror()); return 1; }
 PFN_vkGetInstanceProcAddr gipa = (PFN_vkGetInstanceProcAddr)dlsym(lib, "vk_icdGetInstanceProcAddr");
 if (!gipa) gipa = (PFN_vkGetInstanceProcAddr)dlsym(lib, "vkGetInstanceProcAddr");
 if (!gipa) return 1;
 PFN_vkCreateInstance create = (PFN_vkCreateInstance)gipa(NULL, "vkCreateInstance");
 VkApplicationInfo ai = {VK_STRUCTURE_TYPE_APPLICATION_INFO, NULL, "g615-consumer-caps", 1, NULL, 0, VK_API_VERSION_1_4};
 VkInstanceCreateInfo ci = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, NULL, 0, &ai};
 VkInstance instance = VK_NULL_HANDLE;
 if (!create || create(&ci, NULL, &instance) != VK_SUCCESS) return 1;
#define GET(type, name) PFN_##type name = (PFN_##type)gipa(instance, #type)
 GET(vkEnumeratePhysicalDevices, enumerate_devices); GET(vkEnumerateDeviceExtensionProperties, enumerate_exts); GET(vkGetPhysicalDeviceQueueFamilyProperties, get_queues);
 GET(vkGetPhysicalDeviceFeatures2, get_features); GET(vkGetPhysicalDeviceProperties2, get_properties);
 GET(vkGetPhysicalDeviceFormatProperties, get_format); GET(vkGetPhysicalDeviceFormatProperties2, get_format2);
 GET(vkGetPhysicalDeviceImageFormatProperties2, get_image_format2); GET(vkDestroyInstance, destroy);
#undef GET
 uint32_t count = 0; if (enumerate_devices(instance, &count, NULL) != VK_SUCCESS || !count) return 1;
 VkPhysicalDevice *devices = calloc(count, sizeof(*devices)); enumerate_devices(instance, &count, devices);
 VkPhysicalDevice physical = devices[0]; free(devices);

 VkPhysicalDeviceRobustness2FeaturesEXT robust2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT};
 VkPhysicalDeviceCustomBorderColorFeaturesEXT border = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_FEATURES_EXT}; LINK(robust2,border);
 VkPhysicalDeviceDepthClipEnableFeaturesEXT depth_clip = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT}; LINK(border,depth_clip);
 VkPhysicalDeviceVertexAttributeDivisorFeaturesKHR divisor = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_FEATURES_KHR}; LINK(depth_clip,divisor);
 VkPhysicalDeviceDynamicRenderingUnusedAttachmentsFeaturesEXT unused = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_UNUSED_ATTACHMENTS_FEATURES_EXT}; LINK(divisor,unused);
 VkPhysicalDeviceTransformFeedbackFeaturesEXT xfb = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT}; LINK(unused,xfb);
 VkPhysicalDeviceVulkan14Features f14 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES}; LINK(xfb,f14);
 VkPhysicalDeviceVulkan13Features f13 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES}; LINK(f14,f13);
 VkPhysicalDeviceVulkan12Features f12 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES}; LINK(f13,f12);
 VkPhysicalDeviceVulkan11Features f11 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES}; LINK(f12,f11);
 VkPhysicalDeviceFeatures2 f2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, &robust2}; get_features(physical,&f2);

 VkPhysicalDevicePushDescriptorPropertiesKHR push = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_DESCRIPTOR_PROPERTIES_KHR};
 VkPhysicalDeviceCustomBorderColorPropertiesEXT borderp = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_PROPERTIES_EXT}; LINK(push,borderp);
 VkPhysicalDeviceTransformFeedbackPropertiesEXT xfbp = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_PROPERTIES_EXT}; LINK(borderp,xfbp);
 VkPhysicalDeviceVulkan14Properties p14 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_PROPERTIES}; LINK(xfbp,p14);
 VkPhysicalDeviceVulkan13Properties p13 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES}; LINK(p14,p13);
 VkPhysicalDeviceVulkan12Properties p12 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES}; LINK(p13,p12);
 VkPhysicalDeviceVulkan11Properties p11 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES}; LINK(p12,p11);
 VkPhysicalDeviceProperties2 p2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, &push}; get_properties(physical,&p2);

 uint32_t ext_count=0; enumerate_exts(physical,NULL,&ext_count,NULL); VkExtensionProperties *exts=calloc(ext_count,sizeof(*exts)); enumerate_exts(physical,NULL,&ext_count,exts);
 /* CAPTURE_GENERATED_DECLARATIONS */
 printf("{\"schemaVersion\":1,\"device\":{"); json_string("name"); putchar(':'); json_string(p2.properties.deviceName);
 printf(",\"vendorID\":%u,\"deviceID\":%u,\"apiVersion\":%u,\"driverVersion\":%u}",p2.properties.vendorID,p2.properties.deviceID,p2.properties.apiVersion,p2.properties.driverVersion);
 printf(",\"extensions\":["); for(uint32_t i=0;i<ext_count;i++){if(i)putchar(',');printf("{");json_string("name");putchar(':');json_string(exts[i].extensionName);printf(",\"specVersion\":%u}",exts[i].specVersion);} printf("]");
 uint32_t queue_count=0; get_queues(physical,&queue_count,NULL); VkQueueFamilyProperties *queues=calloc(queue_count,sizeof(*queues)); get_queues(physical,&queue_count,queues); printf(",\"queueFamilies\":["); for(uint32_t i=0;i<queue_count;i++){if(i)putchar(',');printf("{\"queueFlags\":%u,\"queueCount\":%u,\"timestampValidBits\":%u,\"minImageTransferGranularity\":{\"width\":%u,\"height\":%u,\"depth\":%u}}",queues[i].queueFlags,queues[i].queueCount,queues[i].timestampValidBits,queues[i].minImageTransferGranularity.width,queues[i].minImageTransferGranularity.height,queues[i].minImageTransferGranularity.depth);} printf("]"); free(queues);

 int first=1; printf(",\"features\":{");
#define CORE10(x) BF(f2.features,x)
 CORE10(robustBufferAccess);CORE10(fullDrawIndexUint32);CORE10(imageCubeArray);CORE10(independentBlend);CORE10(geometryShader);CORE10(tessellationShader);CORE10(sampleRateShading);CORE10(dualSrcBlend);CORE10(logicOp);CORE10(multiDrawIndirect);CORE10(drawIndirectFirstInstance);CORE10(depthClamp);CORE10(depthBiasClamp);CORE10(fillModeNonSolid);CORE10(depthBounds);CORE10(wideLines);CORE10(largePoints);CORE10(alphaToOne);CORE10(multiViewport);CORE10(samplerAnisotropy);CORE10(textureCompressionETC2);CORE10(textureCompressionASTC_LDR);CORE10(textureCompressionBC);CORE10(occlusionQueryPrecise);CORE10(pipelineStatisticsQuery);CORE10(vertexPipelineStoresAndAtomics);CORE10(fragmentStoresAndAtomics);CORE10(shaderTessellationAndGeometryPointSize);CORE10(shaderImageGatherExtended);CORE10(shaderStorageImageExtendedFormats);CORE10(shaderStorageImageMultisample);CORE10(shaderStorageImageReadWithoutFormat);CORE10(shaderStorageImageWriteWithoutFormat);CORE10(shaderUniformBufferArrayDynamicIndexing);CORE10(shaderSampledImageArrayDynamicIndexing);CORE10(shaderStorageBufferArrayDynamicIndexing);CORE10(shaderStorageImageArrayDynamicIndexing);CORE10(shaderClipDistance);CORE10(shaderCullDistance);CORE10(shaderFloat64);CORE10(shaderInt64);CORE10(shaderInt16);CORE10(shaderResourceResidency);CORE10(shaderResourceMinLod);CORE10(sparseBinding);CORE10(sparseResidencyBuffer);CORE10(sparseResidencyImage2D);CORE10(sparseResidencyImage3D);CORE10(sparseResidency2Samples);CORE10(sparseResidency4Samples);CORE10(sparseResidency8Samples);CORE10(sparseResidency16Samples);CORE10(sparseResidencyAliased);CORE10(variableMultisampleRate);CORE10(inheritedQueries);
#undef CORE10
#define F11(x) BF(f11,x)
 F11(storageBuffer16BitAccess);F11(uniformAndStorageBuffer16BitAccess);F11(storagePushConstant16);F11(storageInputOutput16);F11(multiview);F11(multiviewGeometryShader);F11(multiviewTessellationShader);F11(variablePointersStorageBuffer);F11(variablePointers);F11(protectedMemory);F11(samplerYcbcrConversion);F11(shaderDrawParameters);
#undef F11
#define F12(x) BF(f12,x)
 F12(samplerMirrorClampToEdge);F12(drawIndirectCount);F12(storageBuffer8BitAccess);F12(uniformAndStorageBuffer8BitAccess);F12(storagePushConstant8);F12(shaderBufferInt64Atomics);F12(shaderSharedInt64Atomics);F12(shaderFloat16);F12(shaderInt8);F12(descriptorIndexing);F12(shaderInputAttachmentArrayDynamicIndexing);F12(shaderUniformTexelBufferArrayDynamicIndexing);F12(shaderStorageTexelBufferArrayDynamicIndexing);F12(shaderUniformBufferArrayNonUniformIndexing);F12(shaderSampledImageArrayNonUniformIndexing);F12(shaderStorageBufferArrayNonUniformIndexing);F12(shaderStorageImageArrayNonUniformIndexing);F12(shaderInputAttachmentArrayNonUniformIndexing);F12(shaderUniformTexelBufferArrayNonUniformIndexing);F12(shaderStorageTexelBufferArrayNonUniformIndexing);F12(descriptorBindingUniformBufferUpdateAfterBind);F12(descriptorBindingSampledImageUpdateAfterBind);F12(descriptorBindingStorageImageUpdateAfterBind);F12(descriptorBindingStorageBufferUpdateAfterBind);F12(descriptorBindingUniformTexelBufferUpdateAfterBind);F12(descriptorBindingStorageTexelBufferUpdateAfterBind);F12(descriptorBindingUpdateUnusedWhilePending);F12(descriptorBindingPartiallyBound);F12(descriptorBindingVariableDescriptorCount);F12(runtimeDescriptorArray);F12(samplerFilterMinmax);F12(scalarBlockLayout);F12(imagelessFramebuffer);F12(uniformBufferStandardLayout);F12(shaderSubgroupExtendedTypes);F12(separateDepthStencilLayouts);F12(hostQueryReset);F12(timelineSemaphore);F12(bufferDeviceAddress);F12(bufferDeviceAddressCaptureReplay);F12(bufferDeviceAddressMultiDevice);F12(vulkanMemoryModel);F12(vulkanMemoryModelDeviceScope);F12(vulkanMemoryModelAvailabilityVisibilityChains);F12(shaderOutputViewportIndex);F12(shaderOutputLayer);F12(subgroupBroadcastDynamicId);
#undef F12
#define F13(x) BF(f13,x)
 F13(robustImageAccess);F13(inlineUniformBlock);F13(descriptorBindingInlineUniformBlockUpdateAfterBind);F13(pipelineCreationCacheControl);F13(privateData);F13(shaderDemoteToHelperInvocation);F13(shaderTerminateInvocation);F13(subgroupSizeControl);F13(computeFullSubgroups);F13(synchronization2);F13(textureCompressionASTC_HDR);F13(shaderZeroInitializeWorkgroupMemory);F13(dynamicRendering);F13(shaderIntegerDotProduct);F13(maintenance4);
#undef F13
 #define F14(x) BF(f14,x)
 F14(globalPriorityQuery);F14(shaderSubgroupRotate);F14(shaderSubgroupRotateClustered);F14(shaderFloatControls2);F14(shaderExpectAssume);F14(rectangularLines);F14(bresenhamLines);F14(smoothLines);F14(stippledRectangularLines);F14(stippledBresenhamLines);F14(stippledSmoothLines);F14(vertexAttributeInstanceRateDivisor);F14(vertexAttributeInstanceRateZeroDivisor);F14(indexTypeUint8);F14(dynamicRenderingLocalRead);F14(maintenance5);F14(maintenance6);F14(pipelineProtectedAccess);F14(pipelineRobustness);F14(hostImageCopy);F14(pushDescriptor);
 #undef F14
 BF(robust2,robustBufferAccess2);BF(robust2,robustImageAccess2);BF(robust2,nullDescriptor);BF(border,customBorderColors);BF(border,customBorderColorWithoutFormat);BF(depth_clip,depthClipEnable);BF(divisor,vertexAttributeInstanceRateDivisor);BF(divisor,vertexAttributeInstanceRateZeroDivisor);BF(unused,dynamicRenderingUnusedAttachments);BF(xfb,transformFeedback);BF(xfb,geometryStreams); printf("}");

 first=1; printf(",\"properties\":{"); UP(p2.properties.limits,maxPushConstantsSize);UP(p2.properties.limits,bufferImageGranularity);UP(push,maxPushDescriptors);UP(borderp,maxCustomBorderColorSamplers);UP(p11,maxMultiviewViewCount);UP(p11,maxMultiviewInstanceIndex);UP(p12,maxUpdateAfterBindDescriptorsInAllPools);UP(p12,maxPerStageDescriptorUpdateAfterBindSamplers);UP(p12,maxPerStageDescriptorUpdateAfterBindUniformBuffers);UP(p12,maxPerStageDescriptorUpdateAfterBindStorageBuffers);UP(p12,maxPerStageDescriptorUpdateAfterBindSampledImages);UP(p12,maxPerStageDescriptorUpdateAfterBindStorageImages);UP(p12,maxDescriptorSetUpdateAfterBindSamplers);UP(p12,maxDescriptorSetUpdateAfterBindUniformBuffers);UP(p12,maxDescriptorSetUpdateAfterBindStorageBuffers);UP(p12,maxDescriptorSetUpdateAfterBindSampledImages);UP(p12,maxDescriptorSetUpdateAfterBindStorageImages);BF(p12,robustBufferAccessUpdateAfterBind);BF(xfbp,transformFeedbackQueries);BF(xfbp,transformFeedbackRasterizationStreamSelect);BF(xfbp,transformFeedbackStreamsLinesTriangles);BF(xfbp,transformFeedbackDraw); printf("}");
 first=1; printf(",\"sparseProperties\":{"); BF(p2.properties.sparseProperties,residencyStandard2DBlockShape);BF(p2.properties.sparseProperties,residencyStandard2DMultisampleBlockShape);BF(p2.properties.sparseProperties,residencyStandard3DBlockShape);BF(p2.properties.sparseProperties,residencyAlignedMipSize);BF(p2.properties.sparseProperties,residencyNonResidentStrict);printf("}");
 /* CAPTURE_GENERATED_JSON */

 printf(",\"bcFormats\":["); for(size_t i=0;i<sizeof(bc)/sizeof(bc[0]);i++){VkFormatProperties legacy={0};VkFormatProperties3 fp3={VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_3};VkFormatProperties2 fp2={VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2,&fp3};VkPhysicalDeviceImageFormatInfo2 info={VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2,NULL,bc[i].value,VK_IMAGE_TYPE_2D,VK_IMAGE_TILING_OPTIMAL,VK_IMAGE_USAGE_SAMPLED_BIT,0};VkImageFormatProperties2 image={VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2};get_format(physical,bc[i].value,&legacy);get_format2(physical,bc[i].value,&fp2);VkResult r=get_image_format2(physical,&info,&image);if(i)putchar(',');printf("{");json_string("name");putchar(':');json_string(bc[i].name);printf(",\"legacy\":{\"linear\":%u,\"optimal\":%u,\"buffer\":%u},\"properties2\":{\"linear\":%u,\"optimal\":%u,\"buffer\":%u},\"properties3\":{\"linear\":%"PRIu64",\"optimal\":%"PRIu64",\"buffer\":%"PRIu64"},\"imageFormatResult\":%d,\"imageFormatProperties\":{\"maxExtent\":{\"width\":%u,\"height\":%u,\"depth\":%u},\"maxMipLevels\":%u,\"maxArrayLayers\":%u,\"sampleCounts\":%u,\"maxResourceSize\":%"PRIu64"}}",legacy.linearTilingFeatures,legacy.optimalTilingFeatures,legacy.bufferFeatures,fp2.formatProperties.linearTilingFeatures,fp2.formatProperties.optimalTilingFeatures,fp2.formatProperties.bufferFeatures,(uint64_t)fp3.linearTilingFeatures,(uint64_t)fp3.optimalTilingFeatures,(uint64_t)fp3.bufferFeatures,r,image.imageFormatProperties.maxExtent.width,image.imageFormatProperties.maxExtent.height,image.imageFormatProperties.maxExtent.depth,image.imageFormatProperties.maxMipLevels,image.imageFormatProperties.maxArrayLayers,image.imageFormatProperties.sampleCounts,(uint64_t)image.imageFormatProperties.maxResourceSize);}printf("]}\n");
 free(exts); destroy(instance,NULL); dlclose(lib); return 0;
}
