# Mali Vulkan native features per arch (ARM proprietary driver)

Reference of what ARM's own Mali Vulkan driver exposes on Android, per Mali architecture version, so we can tell which PanVK features have a native counterpart in ARM's driver and which PanVK implements purely by emulation. Generated 2026-10-29 from vulkan.gpuinfo.org reports (data fetched, not hand-copied). Raw JSON/HTML lives outside git in `/var/tmp/panvk/gpuinfo/`.

## 1. Sources and method

Each report was fetched twice: the full HTML (`displayreport.php?id=N`, used for device, memory and queue tabs) and the Khronos-profile JSON (`api/v3/getprofile.php?id=N`, used for features, properties, extensions, formats and queue families). The `devsim/getreport.php` endpoint named in the task returns 404; `getprofile.php` (linked as "Full JSON profile" on every report page) is the working JSON. Report IDs for v6/v7/v9 were found through the site's internal DataTables endpoints (`api/internal/devices.php`, `api/internal/reports.php`) by picking the newest ARM-numbered driver (rNN.x.y) for one representative GPU per arch.

| Arch | Report | GPU | Device | ARM driver | Vulkan API | Android | Submitted |
|---|---|---|---|---|---|---|---|
| v6 | [52809](https://vulkan.gpuinfo.org/displayreport.php?id=52809) | Mali-G72 MP3 | TCL 9466X | 26.0.0 | 1.1.131 | 13.0 | 2026-10-27 |
| v7 | [47323](https://vulkan.gpuinfo.org/displayreport.php?id=47323) | Mali-G52 | hardkernel ODROID-N2Plus | 51.0.0 | 1.3.283 | 15.0 | 2026-03-26 |
| v9 | [49875](https://vulkan.gpuinfo.org/displayreport.php?id=49875) | Mali-G77 | Moorechip Retroid Pocket 4 Pro | 54.1.0 | 1.3.303 | 13.0 | 2026-06-30 |
| v10 | [52767](https://vulkan.gpuinfo.org/displayreport.php?id=52767) | Mali-G610 MC6 | OPPO PGZ110 | 54.1.0 | 1.3.305 | 15.0 | 2026-10-25 |
| v11 | [51291](https://vulkan.gpuinfo.org/displayreport.php?id=51291) | Mali-G615 MC6 | INFINIX Infinix X6856 | 44.1.0 | 1.3.247 | 16.0 | 2026-08-23 |
| v12 | [52703](https://vulkan.gpuinfo.org/displayreport.php?id=52703) | Mali-G720 MC7 | Xiaomi 25069PTEBG | 49.1.0 | 1.3.278 | 16.0 | 2026-10-23 |
| v13 | [52411](https://vulkan.gpuinfo.org/displayreport.php?id=52411) | Mali-G925-Immortalis MC12 | OnePlus PLC110 | 49.1.0 | 1.3.278 | 17.0 | 2026-10-09 |
| v14 | [52849](https://vulkan.gpuinfo.org/displayreport.php?id=52849) | Mali-G1-Ultra MC12 | vivo V2509A | 54.1.0 | 1.3.305 | 16.0 | 2026-10-28 |
| v15 | [52414](https://vulkan.gpuinfo.org/displayreport.php?id=52414) | Mali-G2-Ultra-NX MC12 | vivo V2602A | 56.0.0 | 1.4.349 | 17.0 | 2026-10-09 |

Caveats that matter when reading the tables:

- One device per arch. A feature missing from a column may reflect that driver vintage rather than the silicon. The driver spans r26 (v6, Vulkan 1.1) to r56 (v15, Vulkan 1.4). v11 is r44 and v12/v13 are r49 while v10/v14 are r54 and v15 is r56, so v11 especially under-reports (no `VK_EXT_descriptor_buffer`, no `VK_EXT_robustness2`, no dualSrcBlend, logicOp, wideLines).
- v6 (r26) is a Vulkan 1.1 driver: `Vulkan12Features`/`Vulkan13Features` and many extension structs do not exist, shown as `-` (not reported), which is not the same as `n` (reported false).
- v7 is Mali-G52 on an ODROID-N2Plus (Linux-style Amlogic board running Android 15), the newest ARM driver found for Bifrost v7 (r51). The Mali-G76 (also v7) best Android phone report is r38 (Galaxy S10, report 47240) and was not used.
- v9 is Mali-G77 MC9 (Retroid Pocket 4 Pro). Samsung/Google builds use their own driver numbering (for example 512.x on Pixel 6 G78) and were not used.
- gpuinfo stores format properties only for core Vulkan 1.0 format enums that have some support. ASTC HDR (`SFLOAT_BLOCK`) formats and 1.1+ extension formats are not recorded per format, so HDR support is judged from the `textureCompressionASTC_HDR` feature bit only.
- A feature exposed by ARM does not prove fixed-function hardware. ARM may emulate in its compiler or with compute jobs, exactly as PanVK does. Section 8 flags that per item.

## 2. Device, memory and queues

| Property | v6 | v7 | v9 | v10 | v11 | v12 | v13 | v14 | v15 |
|---|---|---|---|---|---|---|---|---|---|
| GPU | Mali-G72 MP3 | Mali-G52 | Mali-G77 | Mali-G610 MC6 | Mali-G615 MC6 | Mali-G720 MC7 | Mali-G925-Immortalis MC12 | Mali-G1-Ultra MC12 | Mali-G2-Ultra-NX MC12 |
| ARM driver | 26.0.0 | 51.0.0 | 54.1.0 | 54.1.0 | 44.1.0 | 49.1.0 | 49.1.0 | 54.1.0 | 56.0.0 |
| Vulkan API | 1.1.131 | 1.3.283 | 1.3.303 | 1.3.305 | 1.3.247 | 1.3.278 | 1.3.278 | 1.3.305 | 1.4.349 |
| Android | 13.0 | 15.0 | 13.0 | 15.0 | 16.0 | 16.0 | 17.0 | 16.0 | 17.0 |
| Device extensions | 56 | 114 | 133 | 142 | 118 | 140 | 142 | 154 | 161 |
| timestampPeriod (ns) | 0 | 41.6667 | 76.9231 | 76.9231 | 76.9231 | 76.9231 | 76.9231 | 0.999001 | 0.999001 |
| Formats recorded | 154 | 166 | 166 | 166 | 164 | 169 | 169 | 171 | 175 |
| Memory heaps (GiB) | 3.8 | 3.6 | 7.7 | 11.1 | 11.2 + 100 MiB | 11.0 + 100 MiB | 11.1 + 100 MiB | 15.0 + 100 MiB | 13.8 + 100 MiB |
| Memory types | 3 | 3 | 3 | 3 | 4 | 4 | 4 | 4 | 10 |
| Distinct type flag sets | 3 | 3 | 3 | 3 | 4 | 4 | 4 | 4 | 5 |
| Protected memory type (heap 1, 100 MiB) | n | n | n | n | Y | Y | Y | Y | Y |
| Queue families | 2q GRAP+COMP+TRAN | 2q GRAP+COMP+TRAN | 2q GRAP+COMP+TRAN | 2q GRAP+COMP+TRAN | 2q GRAP+COMP+TRAN+PROT | 2q GRAP+COMP+TRAN+PROT | 2q GRAP+COMP+TRAN+PROT | 2q GRAP+COMP+TRAN+PROT | 4q GRAP+COMP+TRAN+PROT |

All arches expose one universal queue family (graphics, compute, transfer, 64 timestamp bits, 1x1x1 transfer granularity). No sparse-binding queue anywhere. Only v15 reports 4 queues in that family and sets the ARM data-graph queue flag; v11+ add the protected-memory queue bit. Memory types are UMA: one device-local heap with host-visible coherent/cached types plus a lazily-allocated type for transient attachments. v15 lists 10 types (9 on the main heap, several host-visible coherent variants). v6 reports timestampPeriod 0 and no timestamp support on compute/graphics queues.

## 3. Features

Legend: `Y` reported true, `n` reported false, `-` structure/extension not reported by that driver. Section 3.1 is the curated key list (rows relevant to PanVK emulation, constant or not); 3.2 is every remaining row where the arches differ. Rows are feature names (aliases across KHR/EXT/core structs coalesced).

### 3.1 Key features

| Feature | v6 | v7 | v9 | v10 | v11 | v12 | v13 | v14 | v15 |
|---|---|---|---|---|---|---|---|---|---|
| robustBufferAccess | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| fullDrawIndexUint32 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| imageCubeArray | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| independentBlend | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| geometryShader | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| tessellationShader | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| sampleRateShading | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| dualSrcBlend | n | Y | Y | Y | n | Y | Y | Y | Y |
| logicOp | n | Y | Y | Y | n | Y | Y | Y | Y |
| multiDrawIndirect | n | n | n | Y | Y | Y | Y | Y | Y |
| drawIndirectFirstInstance | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| drawIndirectCount | - | n | n | Y | Y | Y | Y | Y | Y |
| depthClamp | n | n | Y | Y | Y | Y | Y | Y | Y |
| depthBiasClamp | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| fillModeNonSolid | n | n | n | n | n | n | n | n | n |
| depthBounds | n | n | n | n | n | n | n | n | n |
| wideLines | n | Y | Y | Y | n | Y | Y | Y | Y |
| largePoints | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| alphaToOne | n | n | n | n | n | n | n | n | n |
| multiViewport | n | n | n | n | n | n | n | Y | Y |
| samplerAnisotropy | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| textureCompressionETC2 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| textureCompressionASTC_LDR | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| textureCompressionBC | n | n | n | n | n | n | n | n | n |
| textureCompressionASTC_HDR | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| occlusionQueryPrecise | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| pipelineStatisticsQuery | n | n | n | n | n | n | n | n | n |
| vertexPipelineStoresAndAtomics | n | n | n | n | n | n | n | n | n |
| fragmentStoresAndAtomics | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| shaderTessellationAndGeometryPointSize | n | n | n | n | n | n | n | n | n |
| shaderStorageImageMultisample | n | n | n | n | n | n | n | n | n |
| shaderClipDistance | n | n | n | n | n | n | n | n | n |
| shaderCullDistance | n | n | n | n | n | n | n | n | n |
| shaderFloat64 | n | n | n | n | n | n | n | n | n |
| shaderInt64 | n | Y | Y | Y | Y | Y | Y | Y | Y |
| shaderInt16 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| shaderFloat16 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| shaderInt8 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| sparseBinding | n | n | n | n | n | n | n | n | n |
| sparseResidencyBuffer | n | n | n | n | n | n | n | n | n |
| sparseResidencyImage2D | n | n | n | n | n | n | n | n | n |
| variableMultisampleRate | n | n | n | n | n | n | n | n | n |
| inheritedQueries | n | n | n | Y | n | Y | Y | Y | Y |
| transformFeedback | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| geometryStreams | n | n | n | n | n | n | n | n | Y |
| transformFeedbackPreservesProvokingVertex | - | n | n | n | n | n | n | n | n |
| primitivesGeneratedQuery | - | Y | Y | Y | Y | Y | Y | Y | Y |
| robustBufferAccess2 | - | - | n | n | - | - | - | Y | Y |
| robustImageAccess | - | Y | Y | Y | Y | Y | Y | Y | Y |
| robustImageAccess2 | - | - | n | n | - | - | - | Y | Y |
| nullDescriptor | - | - | Y | Y | - | - | - | Y | Y |
| shaderBufferInt64Atomics | - | Y | Y | Y | Y | Y | Y | Y | Y |
| shaderSharedInt64Atomics | - | Y | Y | Y | Y | Y | Y | Y | Y |
| shaderImageInt64Atomics | - | Y | Y | Y | Y | Y | Y | Y | Y |
| sparseImageInt64Atomics | - | n | n | n | n | n | n | n | n |
| descriptorIndexing | - | n | Y | Y | Y | Y | Y | Y | Y |
| runtimeDescriptorArray | - | n | Y | Y | Y | Y | Y | Y | Y |
| descriptorBindingPartiallyBound | - | n | Y | Y | Y | Y | Y | Y | Y |
| descriptorBindingUpdateUnusedWhilePending | - | n | Y | Y | Y | Y | Y | Y | Y |
| bufferDeviceAddress | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| timelineSemaphore | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| multiview | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| multiviewGeometryShader | n | n | n | n | n | n | n | n | n |
| multiviewTessellationShader | n | n | n | n | n | n | n | n | n |
| dynamicRendering | - | Y | Y | Y | Y | Y | Y | Y | Y |
| synchronization2 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| maintenance4 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| maintenance5 | - | - | Y | Y | - | Y | Y | Y | Y |
| extendedDynamicState | - | Y | Y | Y | Y | Y | Y | Y | Y |
| extendedDynamicState2 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| extendedDynamicState2LogicOp | - | n | n | n | n | n | n | n | n |
| extendedDynamicState3PolygonMode | - | n | n | n | - | n | n | n | n |
| extendedDynamicState3ColorBlendEnable | - | n | n | n | - | n | n | n | n |
| extendedDynamicState3DepthClampEnable | - | n | Y | Y | - | Y | Y | Y | Y |
| vertexInputDynamicState | - | Y | Y | Y | - | Y | Y | Y | Y |
| descriptorBuffer | - | - | - | Y | - | - | - | Y | Y |
| graphicsPipelineLibrary | - | - | Y | Y | - | - | - | Y | Y |
| hostImageCopy | - | Y | Y | Y | - | - | - | Y | Y |
| mutableDescriptorType | - | - | Y | Y | - | - | - | Y | Y |
| pipelineFragmentShadingRate | - | - | - | - | Y | Y | Y | Y | Y |
| primitiveFragmentShadingRate | - | - | - | - | Y | Y | Y | Y | Y |
| attachmentFragmentShadingRate | - | - | - | - | Y | Y | Y | Y | Y |
| fragmentDensityMap | - | - | Y | Y | Y | Y | Y | Y | Y |
| rayQuery | - | - | - | - | Y | Y | Y | Y | Y |
| accelerationStructure | - | - | - | - | Y | Y | Y | Y | Y |
| rayTracingPipeline | - | - | - | - | - | - | - | - | Y |
| cooperativeMatrix | - | - | - | - | - | Y | Y | Y | Y |
| conditionalRendering | - | - | - | - | - | - | - | Y | Y |
| fragmentShaderBarycentric | - | - | - | - | - | - | - | Y | Y |
| indexTypeUint8 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| customBorderColors | - | Y | Y | Y | Y | Y | Y | Y | Y |
| depthClipEnable | - | - | Y | Y | - | Y | Y | Y | Y |
| provokingVertexLast | - | Y | Y | Y | Y | Y | Y | Y | Y |
| rectangularLines | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| bresenhamLines | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| smoothLines | n | n | n | n | n | n | n | n | n |
| stippledRectangularLines | n | n | n | n | n | n | n | n | n |
| subgroupSizeControl | - | Y | Y | Y | Y | Y | Y | Y | Y |
| shaderDemoteToHelperInvocation | - | Y | Y | Y | Y | Y | Y | Y | Y |
| shaderIntegerDotProduct | - | Y | Y | Y | Y | Y | Y | Y | Y |
| protectedMemory | - | n | n | n | Y | Y | Y | Y | Y |
| samplerYcbcrConversion | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| renderPassStriped | - | - | - | - | - | Y | Y | Y | Y |
| schedulingControls | - | - | - | Y | - | n | Y | Y | Y |
| micromap | - | - | - | - | - | - | Y | Y | Y |
| swapchainMaintenance1 | - | n | - | n | n | n | Y | n | Y |

### 3.2 Other features that differ between arches

| Feature | v6 | v7 | v9 | v10 | v11 | v12 | v13 | v14 | v15 |
|---|---|---|---|---|---|---|---|---|---|
| accelerationStructureCaptureReplay | - | - | - | - | Y | Y | Y | Y | Y |
| accelerationStructureHostCommands | - | - | - | - | n | n | n | n | n |
| accelerationStructureIndirectBuild | - | - | - | - | Y | Y | Y | Y | Y |
| borderColorSwizzle | - | Y | Y | Y | Y | Y | Y | Y | Y |
| borderColorSwizzleFromImage | - | Y | Y | Y | Y | Y | Y | Y | Y |
| bufferDeviceAddressCaptureReplay | n | n | n | Y | Y | Y | Y | Y | Y |
| computeFullSubgroups | - | Y | Y | Y | Y | Y | Y | Y | Y |
| cooperativeMatrixRobustBufferAccess | - | - | - | - | - | n | n | n | n |
| customBorderColorWithoutFormat | - | Y | Y | Y | Y | Y | Y | Y | Y |
| decodeModeSharedExponent | - | - | Y | Y | Y | Y | Y | Y | Y |
| depthClampZeroOne | - | - | Y | Y | Y | Y | Y | Y | Y |
| descriptorBindingAccelerationStructureUpdateAfterBind | - | - | - | - | Y | Y | Y | Y | Y |
| descriptorBindingInlineUniformBlockUpdateAfterBind | n | Y | Y | Y | Y | Y | Y | Y | Y |
| descriptorBindingSampledImageUpdateAfterBind | - | n | Y | Y | Y | Y | Y | Y | Y |
| descriptorBindingStorageBufferUpdateAfterBind | - | n | Y | Y | Y | Y | Y | Y | Y |
| descriptorBindingStorageImageUpdateAfterBind | - | n | Y | Y | Y | Y | Y | Y | Y |
| descriptorBindingStorageTexelBufferUpdateAfterBind | - | n | Y | Y | Y | Y | Y | Y | Y |
| descriptorBindingUniformBufferUpdateAfterBind | - | n | Y | Y | Y | Y | Y | Y | Y |
| descriptorBindingUniformTexelBufferUpdateAfterBind | - | n | Y | Y | Y | Y | Y | Y | Y |
| descriptorBindingVariableDescriptorCount | - | n | Y | Y | Y | Y | Y | Y | Y |
| descriptorBufferCaptureReplay | - | - | - | Y | - | - | - | Y | Y |
| descriptorBufferImageLayoutIgnored | - | - | - | Y | - | - | - | Y | Y |
| descriptorBufferPushDescriptors | - | - | - | Y | - | - | - | Y | Y |
| deviceFault | - | Y | Y | Y | Y | Y | Y | Y | Y |
| deviceFaultVendorBinary | - | n | n | Y | n | n | n | Y | Y |
| deviceMemoryReport | - | Y | Y | Y | Y | Y | Y | Y | Y |
| dynamicRenderingUnusedAttachments | - | Y | Y | Y | - | Y | Y | Y | Y |
| extendedDynamicState2PatchControlPoints | - | Y | Y | Y | Y | Y | Y | Y | Y |
| extendedDynamicState3AlphaToCoverageEnable | - | n | n | n | - | n | n | n | Y |
| extendedDynamicState3AlphaToOneEnable | - | n | n | n | - | n | n | n | n |
| extendedDynamicState3ColorBlendAdvanced | - | n | n | n | - | n | n | n | n |
| extendedDynamicState3ColorBlendEquation | - | n | n | n | - | n | n | n | n |
| extendedDynamicState3ColorWriteMask | - | n | n | n | - | n | n | n | n |
| extendedDynamicState3ConservativeRasterizationMode | - | n | n | n | - | Y | Y | Y | Y |
| extendedDynamicState3CoverageModulationMode | - | n | n | n | - | n | n | n | n |
| extendedDynamicState3CoverageModulationTable | - | n | n | n | - | n | n | n | n |
| extendedDynamicState3CoverageModulationTableEnable | - | n | n | n | - | n | n | n | n |
| extendedDynamicState3CoverageReductionMode | - | n | n | n | - | n | n | n | n |
| extendedDynamicState3CoverageToColorEnable | - | n | n | n | - | n | n | n | n |
| extendedDynamicState3CoverageToColorLocation | - | n | n | n | - | n | n | n | n |
| extendedDynamicState3DepthClipEnable | - | n | Y | Y | - | Y | Y | Y | Y |
| extendedDynamicState3DepthClipNegativeOneToOne | - | n | n | n | - | n | n | n | n |
| extendedDynamicState3ExtraPrimitiveOverestimationSize | - | n | n | n | - | n | n | n | n |
| extendedDynamicState3LineRasterizationMode | - | Y | Y | Y | - | Y | Y | Y | Y |
| extendedDynamicState3LineStippleEnable | - | n | n | n | - | n | n | n | Y |
| extendedDynamicState3LogicOpEnable | - | n | n | n | - | n | n | n | n |
| extendedDynamicState3ProvokingVertexMode | - | Y | Y | Y | - | Y | Y | Y | Y |
| extendedDynamicState3RasterizationSamples | - | n | n | n | - | n | n | n | n |
| extendedDynamicState3RasterizationStream | - | n | n | n | - | n | n | n | Y |
| extendedDynamicState3RepresentativeFragmentTestEnable | - | n | n | n | - | n | n | n | n |
| extendedDynamicState3SampleLocationsEnable | - | n | n | n | - | n | n | n | n |
| extendedDynamicState3SampleMask | - | Y | Y | Y | - | Y | Y | Y | Y |
| extendedDynamicState3ShadingRateImageEnable | - | n | n | n | - | n | n | n | n |
| extendedDynamicState3TessellationDomainOrigin | - | Y | Y | Y | - | Y | Y | Y | Y |
| extendedDynamicState3ViewportSwizzle | - | n | n | n | - | n | n | n | n |
| extendedDynamicState3ViewportWScalingEnable | - | n | n | n | - | n | n | n | n |
| externalFormatResolve | - | - | Y | Y | - | Y | Y | Y | Y |
| formatA4B4G4R4 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| formatA4R4G4B4 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| formatRgba10x6WithoutYCbCrSampler | - | n | n | n | Y | Y | Y | Y | Y |
| fragmentDensityMapDeferred | - | - | Y | Y | Y | Y | Y | Y | Y |
| fragmentDensityMapDynamic | - | - | Y | Y | Y | Y | Y | Y | Y |
| fragmentDensityMapNonSubsampledImages | - | - | n | n | n | n | n | n | n |
| frameBoundary | - | Y | Y | Y | - | Y | Y | Y | Y |
| globalPriorityQuery | - | Y | Y | Y | Y | Y | Y | Y | Y |
| image2DViewOf3D | - | Y | Y | Y | Y | Y | Y | Y | Y |
| imageCompressionControl | - | Y | Y | Y | Y | Y | Y | Y | Y |
| imageCompressionControlSwapchain | - | - | - | Y | Y | n | Y | Y | Y |
| imageSlicedViewOf3D | - | Y | Y | Y | - | Y | Y | Y | Y |
| inheritedConditionalRendering | - | - | - | - | - | - | - | Y | Y |
| legacyDithering | - | Y | Y | Y | Y | Y | Y | Y | Y |
| micromapCaptureReplay | - | - | - | - | - | - | Y | Y | Y |
| micromapHostCommands | - | - | - | - | - | - | n | n | n |
| minLod | - | - | - | Y | - | - | - | Y | Y |
| multisampledRenderToSingleSampled | - | Y | Y | Y | Y | Y | Y | Y | Y |
| nonSeamlessCubeMap | - | - | Y | Y | - | - | - | Y | Y |
| pipelineCreationCacheControl | - | Y | Y | Y | Y | Y | Y | Y | Y |
| pipelineExecutableInfo | - | - | Y | Y | - | - | - | Y | Y |
| pipelineLibraryGroupHandles | - | - | - | - | - | - | - | - | Y |
| pipelineProtectedAccess | - | Y | Y | Y | Y | Y | Y | Y | Y |
| pipelineRobustness | - | Y | Y | Y | Y | Y | Y | Y | Y |
| presentId | - | - | - | - | - | - | Y | - | Y |
| primitiveTopologyListRestart | - | Y | Y | Y | Y | Y | Y | Y | Y |
| primitiveTopologyPatchListRestart | - | n | n | n | n | n | n | n | n |
| primitivesGeneratedQueryWithNonZeroStreams | - | n | n | n | n | n | n | n | Y |
| primitivesGeneratedQueryWithRasterizerDiscard | - | Y | Y | Y | Y | Y | Y | Y | Y |
| privateData | - | Y | Y | Y | Y | Y | Y | Y | Y |
| rasterizationOrderColorAttachmentAccess | - | Y | Y | Y | Y | Y | Y | Y | Y |
| rasterizationOrderDepthAttachmentAccess | - | Y | Y | Y | Y | Y | Y | Y | Y |
| rasterizationOrderStencilAttachmentAccess | - | Y | Y | Y | Y | Y | Y | Y | Y |
| rayTracingMaintenance1 | - | - | - | - | - | - | - | Y | Y |
| rayTracingPipelineShaderGroupHandleCaptureReplay | - | - | - | - | - | - | - | - | Y |
| rayTracingPipelineShaderGroupHandleCaptureReplayMixed | - | - | - | - | - | - | - | - | n |
| rayTracingPipelineTraceRaysIndirect | - | - | - | - | - | - | - | - | Y |
| rayTracingPipelineTraceRaysIndirect2 | - | - | - | - | - | - | - | n | Y |
| rayTracingPositionFetch | - | - | - | - | - | Y | Y | Y | Y |
| rayTraversalPrimitiveCulling | - | - | - | - | - | - | - | - | Y |
| reportAddressBinding | - | Y | Y | Y | Y | Y | Y | Y | Y |
| sampler2DViewOf3D | - | Y | Y | Y | Y | Y | Y | Y | Y |
| samplerFilterMinmax | - | n | n | Y | Y | Y | Y | Y | Y |
| samplerMirrorClampToEdge | - | Y | Y | Y | Y | Y | Y | Y | Y |
| shaderCoreBuiltins | - | n | Y | Y | Y | Y | Y | Y | Y |
| shaderDeviceClock | - | - | - | - | - | - | - | - | Y |
| shaderDrawParameters | - | Y | Y | Y | Y | Y | Y | Y | Y |
| shaderInputAttachmentArrayDynamicIndexing | - | n | Y | Y | Y | Y | Y | Y | Y |
| shaderInputAttachmentArrayNonUniformIndexing | - | n | Y | Y | Y | Y | Y | Y | Y |
| shaderModuleIdentifier | - | - | - | - | - | - | - | - | Y |
| shaderOutputLayer | - | n | n | Y | Y | Y | Y | Y | Y |
| shaderOutputViewportIndex | - | Y | Y | Y | Y | Y | Y | Y | Y |
| shaderSampledImageArrayNonUniformIndexing | - | n | Y | Y | Y | Y | Y | Y | Y |
| shaderStorageBufferArrayNonUniformIndexing | - | n | Y | Y | Y | Y | Y | Y | Y |
| shaderStorageImageArrayNonUniformIndexing | - | n | Y | Y | Y | Y | Y | Y | Y |
| shaderStorageTexelBufferArrayDynamicIndexing | - | n | Y | Y | Y | Y | Y | Y | Y |
| shaderStorageTexelBufferArrayNonUniformIndexing | - | n | Y | Y | Y | Y | Y | Y | Y |
| shaderSubgroupClock | - | - | - | - | - | - | - | - | Y |
| shaderSubgroupUniformControlFlow | - | Y | Y | Y | - | Y | Y | Y | Y |
| shaderTerminateInvocation | - | Y | Y | Y | Y | Y | Y | Y | Y |
| shaderTileImageColorReadAccess | - | Y | Y | Y | Y | Y | Y | Y | Y |
| shaderTileImageDepthReadAccess | - | Y | Y | Y | Y | Y | Y | Y | Y |
| shaderTileImageStencilReadAccess | - | Y | Y | Y | Y | Y | Y | Y | Y |
| shaderUniformBufferArrayNonUniformIndexing | - | n | Y | Y | Y | Y | Y | Y | Y |
| shaderUniformTexelBufferArrayDynamicIndexing | - | n | Y | Y | Y | Y | Y | Y | Y |
| shaderUniformTexelBufferArrayNonUniformIndexing | - | n | Y | Y | Y | Y | Y | Y | Y |
| shaderZeroInitializeWorkgroupMemory | - | Y | Y | Y | Y | Y | Y | Y | Y |
| subgroupBroadcastDynamicId | - | Y | Y | Y | Y | Y | Y | Y | Y |
| subpassMergeFeedback | - | Y | Y | Y | Y | Y | Y | Y | Y |
| texelBufferAlignment | - | Y | Y | Y | Y | Y | Y | Y | Y |
| vertexAttributeInstanceRateDivisor | - | - | Y | Y | - | Y | Y | Y | Y |
| vertexAttributeInstanceRateZeroDivisor | - | - | n | n | - | n | n | n | Y |
| workgroupMemoryExplicitLayout | - | - | - | - | - | - | - | - | Y |
| workgroupMemoryExplicitLayout16BitAccess | - | - | - | - | - | - | - | - | Y |
| workgroupMemoryExplicitLayout8BitAccess | - | - | - | - | - | - | - | - | Y |
| workgroupMemoryExplicitLayoutScalarBlockLayout | - | - | - | - | - | - | - | - | Y |

Identical on every arch that reports them (not tabulated): 27 features always true, 11 always false. Always-false (non-key) list: bufferDeviceAddressMultiDevice, shaderResourceMinLod, shaderResourceResidency, sparseResidency16Samples, sparseResidency2Samples, sparseResidency4Samples, sparseResidency8Samples, sparseResidencyAliased, sparseResidencyImage3D, stippledBresenhamLines, stippledSmoothLines.

## 4. Extensions

A couple of extensions differ in spec version between arches (marked in appendix A). Table 4.1 lists every extension that is not present on all nine arches; the full list with spec versions is in appendix A.

161 distinct device extensions seen; 105 are not universal.

### 4.1 Extensions that appear or disappear across arches

| Extension | v6 | v7 | v9 | v10 | v11 | v12 | v13 | v14 | v15 |
|---|---|---|---|---|---|---|---|---|---|
| VK_ANDROID_external_format_resolve | - | - | Y | Y | - | Y | Y | Y | Y |
| VK_ARM_rasterization_order_attachment_access | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_ARM_render_pass_striped | - | - | - | - | - | Y | Y | Y | Y |
| VK_ARM_scheduling_controls | - | - | - | Y | - | Y | Y | Y | Y |
| VK_ARM_shader_core_builtins | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_ARM_shader_core_properties | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_4444_formats | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_astc_decode_mode | - | - | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_border_color_swizzle | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_calibrated_timestamps | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_conditional_rendering | - | - | - | - | - | - | - | Y | Y |
| VK_EXT_conservative_rasterization | - | - | - | - | Y | Y | Y | Y | Y |
| VK_EXT_custom_border_color | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_debug_marker | - | Y | Y | Y | - | Y | Y | Y | Y |
| VK_EXT_depth_clamp_zero_one | - | - | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_depth_clip_enable | - | - | Y | Y | - | Y | Y | Y | Y |
| VK_EXT_descriptor_buffer | - | - | - | Y | - | - | - | Y | Y |
| VK_EXT_descriptor_indexing | - | - | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_device_address_binding_report | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_device_fault | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_device_memory_report | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_dynamic_rendering_unused_attachments | - | Y | Y | Y | - | Y | Y | Y | Y |
| VK_EXT_extended_dynamic_state | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_extended_dynamic_state2 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_extended_dynamic_state3 | - | Y | Y | Y | - | Y | Y | Y | Y |
| VK_EXT_external_memory_acquire_unmodified | - | Y | Y | Y | - | Y | Y | Y | Y |
| VK_EXT_external_memory_host | - | - | Y | Y | - | - | - | Y | Y |
| VK_EXT_fragment_density_map | - | - | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_fragment_density_map2 | - | - | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_frame_boundary | - | Y | Y | Y | - | Y | Y | Y | Y |
| VK_EXT_global_priority_query | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_graphics_pipeline_library | - | - | Y | Y | - | - | - | Y | Y |
| VK_EXT_hdr_metadata | - | - | - | - | - | Y | Y | - | Y |
| VK_EXT_host_image_copy | - | Y | Y | Y | - | - | - | Y | Y |
| VK_EXT_image_2d_view_of_3d | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_image_compression_control | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_image_compression_control_swapchain | - | - | - | Y | Y | Y | Y | Y | Y |
| VK_EXT_image_robustness | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_image_sliced_view_of_3d | - | Y | Y | Y | - | Y | Y | Y | Y |
| VK_EXT_image_view_min_lod | - | - | - | Y | - | - | - | Y | Y |
| VK_EXT_legacy_dithering | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_load_store_op_none | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_multisampled_render_to_single_sampled | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_mutable_descriptor_type | - | - | Y | Y | - | - | - | Y | Y |
| VK_EXT_non_seamless_cube_map | - | - | Y | Y | - | - | - | Y | Y |
| VK_EXT_opacity_micromap | - | - | - | - | - | - | Y | Y | Y |
| VK_EXT_pipeline_creation_cache_control | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_pipeline_creation_feedback | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_pipeline_library_group_handles | - | - | - | - | - | - | - | - | Y |
| VK_EXT_pipeline_protected_access | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_pipeline_robustness | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_primitive_topology_list_restart | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_primitives_generated_query | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_private_data | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_provoking_vertex | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_rasterization_order_attachment_access | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_rgba10x6_formats | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_robustness2 | - | - | Y | Y | - | - | - | Y | Y |
| VK_EXT_sampler_filter_minmax | - | - | - | Y | Y | Y | Y | Y | Y |
| VK_EXT_shader_demote_to_helper_invocation | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_shader_image_atomic_int64 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_shader_module_identifier | - | - | - | - | - | - | - | - | Y |
| VK_EXT_shader_tile_image | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_shader_viewport_index_layer | - | - | - | - | - | - | - | Y | Y |
| VK_EXT_subgroup_size_control | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_subpass_merge_feedback | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_swapchain_maintenance1 | - | Y | - | Y | Y | Y | Y | Y | Y |
| VK_EXT_texel_buffer_alignment | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_tooling_info | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_vertex_input_dynamic_state | - | Y | Y | Y | - | Y | Y | Y | Y |
| VK_GOOGLE_decorate_string | - | Y | Y | Y | - | Y | Y | Y | Y |
| VK_GOOGLE_user_type | - | - | Y | Y | - | Y | Y | Y | Y |
| VK_KHR_acceleration_structure | - | - | - | - | Y | Y | Y | Y | Y |
| VK_KHR_calibrated_timestamps | - | - | Y | Y | - | Y | Y | Y | Y |
| VK_KHR_cooperative_matrix | - | - | - | - | - | Y | Y | Y | Y |
| VK_KHR_copy_commands2 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_deferred_host_operations | - | - | - | Y | Y | Y | Y | Y | Y |
| VK_KHR_draw_indirect_count | - | - | - | Y | Y | Y | Y | Y | Y |
| VK_KHR_dynamic_rendering | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_format_feature_flags2 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_fragment_shader_barycentric | - | - | - | - | - | - | - | Y | Y |
| VK_KHR_fragment_shading_rate | - | - | - | - | Y | Y | Y | Y | Y |
| VK_KHR_global_priority | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_maintenance4 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_maintenance5 | - | - | Y | Y | - | Y | Y | Y | Y |
| VK_KHR_map_memory2 | - | - | Y | Y | - | Y | Y | Y | Y |
| VK_KHR_pipeline_executable_properties | - | - | Y | Y | - | - | - | Y | Y |
| VK_KHR_pipeline_library | - | - | Y | Y | - | - | - | Y | Y |
| VK_KHR_present_id | - | - | - | - | - | - | Y | - | Y |
| VK_KHR_push_descriptor | - | - | Y | Y | - | Y | Y | Y | Y |
| VK_KHR_ray_query | - | - | - | - | Y | Y | Y | Y | Y |
| VK_KHR_ray_tracing_maintenance1 | - | - | - | - | - | - | - | Y | Y |
| VK_KHR_ray_tracing_pipeline | - | - | - | - | - | - | - | - | Y |
| VK_KHR_ray_tracing_position_fetch | - | - | - | - | - | Y | Y | Y | Y |
| VK_KHR_shader_atomic_int64 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_shader_clock | - | - | - | - | - | - | - | - | Y |
| VK_KHR_shader_integer_dot_product | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_shader_non_semantic_info | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_shader_subgroup_uniform_control_flow | - | Y | Y | Y | - | Y | Y | Y | Y |
| VK_KHR_shader_terminate_invocation | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_swapchain_mutable_format | - | - | - | Y | Y | Y | Y | Y | Y |
| VK_KHR_synchronization2 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_vertex_attribute_divisor | - | - | Y | Y | - | Y | Y | Y | Y |
| VK_KHR_workgroup_memory_explicit_layout | - | - | - | - | - | - | - | - | Y |
| VK_KHR_zero_initialize_workgroup_memory | - | Y | Y | Y | Y | Y | Y | Y | Y |

## 5. Formats

### 5.1 Compressed formats

Counts are formats with `SAMPLED_IMAGE_BIT` in optimal tiling (all also have linear filtering, blit-src and transfer bits). Totals: ETC2/EAC 10 formats, ASTC LDR 28 formats (14 block sizes x UNORM/SRGB), BC 18 formats.

| Group | v6 | v7 | v9 | v10 | v11 | v12 | v13 | v14 | v15 |
|---|---|---|---|---|---|---|---|---|---|
| BC (BC1-BC7) | 0/18 | 0/18 | 0/18 | 0/18 | 0/18 | 0/18 | 0/18 | 0/18 | 0/18 |
| ETC2/EAC | 10/10 | 10/10 | 10/10 | 10/10 | 10/10 | 10/10 | 10/10 | 10/10 | 10/10 |
| ASTC LDR | 28/28 | 28/28 | 28/28 | 28/28 | 28/28 | 28/28 | 28/28 | 28/28 | 28/28 |
| ASTC HDR (feature bit) | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| textureCompressionBC feature | n | n | n | n | n | n | n | n | n |

No arch has any BC format: ARM's driver never exposes BC1-BC7, so PanVK's BCn emulation has no native counterpart anywhere. ETC2/EAC and ASTC LDR are universal from v6 to v15, and the ASTC HDR bit is set on every arch (including Bifrost v6/v7).

### 5.2 Selected formats (optimal / linear / buffer feature-bit counts)

Each cell is `optimal/linear/buffer` number of feature bits; `none` means the format is not exposed at all (no feature bits in any tiling).

| Format | v6 | v7 | v9 | v10 | v11 | v12 | v13 | v14 | v15 |
|---|---|---|---|---|---|---|---|---|---|
| R8G8B8_UNORM | 8/8/1 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 |
| R8G8B8_SRGB | 8/7/0 | 8/7/0 | 8/7/0 | 8/7/0 | 8/7/0 | 8/7/0 | 8/7/0 | 8/7/0 | 8/8/0 |
| R8G8B8_SINT | 4/4/1 | 5/5/1 | 5/5/1 | 5/5/1 | 4/4/1 | 5/5/1 | 5/5/1 | 5/5/1 | 5/5/1 |
| R8G8B8_SNORM | 5/5/1 | 6/6/1 | 6/6/1 | 6/6/1 | 5/5/1 | 6/6/1 | 6/6/1 | 6/6/1 | 6/6/1 |
| R16G16B16_SFLOAT | 8/8/1 | 8/8/1 | 8/8/1 | 8/8/1 | 8/8/1 | 8/8/1 | 8/8/1 | 8/8/1 | 8/8/1 |
| R16G16B16_UNORM | 4/4/1 | 4/4/1 | 4/4/1 | 7/7/1 | 4/4/1 | 4/4/1 | 4/4/1 | 7/7/1 | 7/7/1 |
| R16G16B16_SINT | 4/4/1 | 4/4/1 | 4/4/1 | 6/6/1 | 6/6/1 | 6/6/1 | 6/6/1 | 6/6/1 | 6/6/1 |
| R32G32B32_SFLOAT | 5/5/1 | 5/5/3 | 5/5/3 | 5/5/3 | 5/5/3 | 5/5/3 | 5/5/3 | 5/5/3 | 5/5/3 |
| R32G32B32_UINT | 4/4/1 | 4/4/3 | 4/4/3 | 4/4/3 | 4/4/3 | 4/4/3 | 4/4/3 | 4/4/3 | 4/4/3 |
| R64_UINT | none | 5/5/0 | 5/5/0 | 5/5/0 | 5/0/0 | 5/5/0 | 5/5/0 | 5/5/0 | 5/5/0 |
| R64_SINT | none | 5/5/0 | 5/5/0 | 5/5/0 | 5/0/0 | 5/5/0 | 5/5/0 | 5/5/0 | 5/5/0 |
| R64G64_SFLOAT | none | none | none | none | none | none | none | none | none |
| A2B10G10R10_UNORM_PACK32 | 9/9/2 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 |
| A2R10G10B10_UNORM_PACK32 | 9/9/1 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 |
| B10G11R11_UFLOAT_PACK32 | 9/9/1 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/2 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 |
| E5B9G9R9_UFLOAT_PACK32 | 5/5/0 | 8/8/2 | 8/8/2 | 8/8/2 | 8/8/2 | 8/8/2 | 8/8/2 | 8/8/2 | 8/8/2 |
| R5G6B5_UNORM_PACK16 | 9/9/0 | 9/9/0 | 9/9/0 | 9/9/0 | 9/9/0 | 9/9/0 | 9/9/0 | 9/9/0 | 9/9/0 |
| B8G8R8A8_UNORM | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 |
| B8G8R8A8_SNORM | 5/5/0 | 6/6/0 | 6/6/0 | 6/6/0 | 6/6/0 | 6/6/0 | 6/6/0 | 6/6/0 | 6/6/0 |
| R16G16B16A16_UNORM | 8/8/3 | 8/8/3 | 8/8/3 | 9/9/3 | 8/8/3 | 8/8/3 | 8/8/3 | 9/9/3 | 9/9/3 |
| R16G16B16A16_SNORM | 8/8/3 | 8/8/3 | 8/8/3 | 9/9/3 | 8/8/3 | 8/8/3 | 8/8/3 | 9/9/3 | 9/9/3 |
| R32G32B32A32_SFLOAT | 8/8/3 | 8/8/3 | 8/8/3 | 9/9/3 | 8/8/3 | 8/8/3 | 8/8/3 | 9/9/3 | 9/9/3 |
| D16_UNORM | 7/7/0 | 7/7/0 | 7/7/0 | 7/7/0 | 7/7/0 | 7/7/0 | 7/7/0 | 7/7/0 | 7/7/0 |
| X8_D24_UNORM_PACK32 | 7/7/0 | 7/7/0 | 7/7/0 | 7/7/0 | 7/0/0 | 7/7/0 | 7/7/0 | 7/7/0 | 7/7/0 |
| D32_SFLOAT | 7/7/0 | 7/7/0 | 7/7/0 | 7/7/0 | 7/7/0 | 7/7/0 | 7/7/0 | 7/7/0 | 7/7/0 |
| D16_UNORM_S8_UINT | none | none | none | none | none | none | none | none | none |
| D24_UNORM_S8_UINT | 7/7/0 | 7/7/0 | 7/7/0 | 7/0/0 | 7/0/0 | 7/0/0 | 7/0/0 | 7/0/0 | 7/0/0 |
| D32_SFLOAT_S8_UINT | 7/0/0 | 7/0/0 | 7/0/0 | 7/0/0 | 7/0/0 | 7/0/0 | 7/0/0 | 7/0/0 | 7/0/0 |
| S8_UINT | 6/6/0 | 6/6/0 | 6/6/0 | 6/6/0 | 6/6/0 | 6/6/0 | 6/6/0 | 6/6/0 | 6/6/0 |
| G8_B8R8_2PLANE_420_UNORM | 4/4/0 | 4/4/0 | 4/4/0 | 4/4/0 | 4/4/0 | 4/4/0 | 4/4/0 | 4/4/0 | 4/4/0 |
| G8_B8_R8_3PLANE_444_UNORM | none | none | none | none | 4/4/0 | 4/4/0 | 4/4/0 | 4/4/0 | 4/4/0 |
| G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16 | none | 4/4/0 | 4/4/0 | 4/4/0 | none | 4/4/0 | 4/4/0 | 4/4/0 | 4/4/0 |
| R10X6_UNORM_PACK16 | none | none | none | none | none | none | none | 8/8/0 | 8/8/0 |
| R12X4_UNORM_PACK16 | none | none | none | none | none | none | none | none | 5/5/0 |

### 5.3 Formats whose support differs between arches

61 of 175 recorded formats differ in at least one tiling/buffer bit set on at least one arch (cell: optimal/linear/buffer bit counts, `-` = not recorded).

| Format | v6 | v7 | v9 | v10 | v11 | v12 | v13 | v14 | v15 |
|---|---|---|---|---|---|---|---|---|---|
| A2B10G10R10_UINT_PACK32 | 7/7/2 | 7/7/3 | 7/7/3 | 7/7/3 | 7/7/3 | 7/7/3 | 7/7/3 | 7/7/3 | 7/7/3 |
| A2B10G10R10_UNORM_PACK32 | 9/9/2 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 |
| A2R10G10B10_SNORM_PACK32 | - | - | - | - | - | - | - | - | 0/0/1 |
| A2R10G10B10_SSCALED_PACK32 | - | 0/0/1 | 0/0/1 | 0/0/1 | 0/0/1 | 0/0/1 | 0/0/1 | 0/0/1 | 0/0/1 |
| A2R10G10B10_UNORM_PACK32 | 9/9/1 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 |
| A2R10G10B10_USCALED_PACK32 | - | 0/0/1 | 0/0/1 | 0/0/1 | 0/0/1 | 0/0/1 | 0/0/1 | 0/0/1 | 0/0/1 |
| B10G11R11_UFLOAT_PACK32 | 9/9/1 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/2 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 |
| B10X6G10X6R10X6G10X6_422_UNORM_4PACK16 | - | 4/4/0 | 4/4/0 | 4/4/0 | - | 4/4/0 | 4/4/0 | 4/4/0 | 4/4/0 |
| B4G4R4A4_UNORM_PACK16 | 8/8/0 | 8/8/0 | 8/8/0 | 8/8/0 | 8/8/0 | 8/8/0 | 8/8/0 | 8/8/0 | 8/8/0 |
| B5G5R5A1_UNORM_PACK16 | 8/8/0 | 9/9/0 | 9/9/0 | 9/9/0 | 9/9/0 | 9/9/0 | 9/9/0 | 9/9/0 | 9/9/0 |
| B8G8R8A8_SNORM | 5/5/0 | 6/6/0 | 6/6/0 | 6/6/0 | 6/6/0 | 6/6/0 | 6/6/0 | 6/6/0 | 6/6/0 |
| B8G8R8A8_SSCALED | - | 0/0/1 | 0/0/1 | 0/0/1 | 0/0/1 | 0/0/1 | 0/0/1 | 0/0/1 | 0/0/1 |
| B8G8R8A8_USCALED | - | 0/0/1 | 0/0/1 | 0/0/1 | 0/0/1 | 0/0/1 | 0/0/1 | 0/0/1 | 0/0/1 |
| B8G8R8_SSCALED | - | 0/0/1 | 0/0/1 | 0/0/1 | 0/0/1 | 0/0/1 | 0/0/1 | 0/0/1 | 0/0/1 |
| B8G8R8_USCALED | - | 0/0/1 | 0/0/1 | 0/0/1 | 0/0/1 | 0/0/1 | 0/0/1 | 0/0/1 | 0/0/1 |
| D24_UNORM_S8_UINT | 7/7/0 | 7/7/0 | 7/7/0 | 7/0/0 | 7/0/0 | 7/0/0 | 7/0/0 | 7/0/0 | 7/0/0 |
| E5B9G9R9_UFLOAT_PACK32 | 5/5/0 | 8/8/2 | 8/8/2 | 8/8/2 | 8/8/2 | 8/8/2 | 8/8/2 | 8/8/2 | 8/8/2 |
| G10X6B10X6G10X6R10X6_422_UNORM_4PACK16 | - | 4/4/0 | 4/4/0 | 4/4/0 | - | 4/4/0 | 4/4/0 | 4/4/0 | 4/4/0 |
| G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16 | - | 4/4/0 | 4/4/0 | 4/4/0 | - | 4/4/0 | 4/4/0 | 4/4/0 | 4/4/0 |
| G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16 | - | 4/4/0 | 4/4/0 | 4/4/0 | - | 4/4/0 | 4/4/0 | 4/4/0 | 4/4/0 |
| G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16 | - | - | - | - | - | 4/4/0 | 4/4/0 | 4/4/0 | 4/4/0 |
| G8_B8_R8_3PLANE_444_UNORM | - | - | - | - | 4/4/0 | 4/4/0 | 4/4/0 | 4/4/0 | 4/4/0 |
| R10X6G10X6B10X6A10X6_UNORM_4PACK16 | - | - | - | - | 8/8/0 | 8/8/0 | 8/8/0 | 8/8/0 | 8/8/0 |
| R10X6G10X6_UNORM_2PACK16 | - | - | - | - | - | - | - | 8/8/0 | 8/8/0 |
| R10X6_UNORM_PACK16 | - | - | - | - | - | - | - | 8/8/0 | 8/8/0 |
| R12X4G12X4B12X4A12X4_UNORM_4PACK16 | - | - | - | - | - | - | - | - | 3/3/0 |
| R12X4G12X4_UNORM_2PACK16 | - | - | - | - | - | - | - | - | 5/5/0 |
| R12X4_UNORM_PACK16 | - | - | - | - | - | - | - | - | 5/5/0 |
| R16G16B16A16_SNORM | 8/8/3 | 8/8/3 | 8/8/3 | 9/9/3 | 8/8/3 | 8/8/3 | 8/8/3 | 9/9/3 | 9/9/3 |
| R16G16B16A16_UNORM | 8/8/3 | 8/8/3 | 8/8/3 | 9/9/3 | 8/8/3 | 8/8/3 | 8/8/3 | 9/9/3 | 9/9/3 |
| R16G16B16_SINT | 4/4/1 | 4/4/1 | 4/4/1 | 6/6/1 | 6/6/1 | 6/6/1 | 6/6/1 | 6/6/1 | 6/6/1 |
| R16G16B16_SNORM | 4/4/1 | 4/4/1 | 4/4/1 | 7/7/1 | 4/4/1 | 4/4/1 | 4/4/1 | 7/7/1 | 7/7/1 |
| R16G16B16_UINT | 4/4/1 | 4/4/1 | 4/4/1 | 5/5/1 | 5/5/1 | 5/5/1 | 5/5/1 | 5/5/1 | 5/5/1 |
| R16G16B16_UNORM | 4/4/1 | 4/4/1 | 4/4/1 | 7/7/1 | 4/4/1 | 4/4/1 | 4/4/1 | 7/7/1 | 7/7/1 |
| R16G16_SNORM | 8/8/3 | 8/8/3 | 8/8/3 | 9/9/3 | 8/8/3 | 8/8/3 | 8/8/3 | 9/9/3 | 9/9/3 |
| R16G16_UNORM | 8/8/3 | 8/8/3 | 8/8/3 | 9/9/3 | 8/8/3 | 8/8/3 | 8/8/3 | 9/9/3 | 9/9/3 |
| R16_SFLOAT | 9/9/2 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 |
| R32G32B32A32_SFLOAT | 8/8/3 | 8/8/3 | 8/8/3 | 9/9/3 | 8/8/3 | 8/8/3 | 8/8/3 | 9/9/3 | 9/9/3 |
| R32G32B32_SFLOAT | 5/5/1 | 5/5/3 | 5/5/3 | 5/5/3 | 5/5/3 | 5/5/3 | 5/5/3 | 5/5/3 | 5/5/3 |
| R32G32B32_SINT | 4/4/1 | 4/4/3 | 4/4/3 | 4/4/3 | 4/4/3 | 4/4/3 | 4/4/3 | 4/4/3 | 4/4/3 |
| R32G32B32_UINT | 4/4/1 | 4/4/3 | 4/4/3 | 4/4/3 | 4/4/3 | 4/4/3 | 4/4/3 | 4/4/3 | 4/4/3 |
| R32G32_SFLOAT | 8/8/3 | 8/8/3 | 8/8/3 | 9/9/3 | 8/8/3 | 8/8/3 | 8/8/3 | 9/9/3 | 9/9/3 |
| R32_SFLOAT | 8/8/3 | 8/8/3 | 8/8/3 | 9/9/3 | 8/8/3 | 8/8/3 | 8/8/3 | 9/9/3 | 9/9/3 |
| R4G4B4A4_UNORM_PACK16 | 8/8/0 | 8/8/0 | 8/8/0 | 8/8/0 | 8/8/0 | 8/8/0 | 8/8/0 | 8/8/0 | 8/8/0 |
| R5G5B5A1_UNORM_PACK16 | 8/8/0 | 9/9/0 | 9/9/0 | 9/9/0 | 9/9/0 | 9/9/0 | 9/9/0 | 9/9/0 | 9/9/0 |
| R64_SINT | - | 5/5/0 | 5/5/0 | 5/5/0 | 5/0/0 | 5/5/0 | 5/5/0 | 5/5/0 | 5/5/0 |
| R64_UINT | - | 5/5/0 | 5/5/0 | 5/5/0 | 5/0/0 | 5/5/0 | 5/5/0 | 5/5/0 | 5/5/0 |
| R8G8B8_SINT | 4/4/1 | 5/5/1 | 5/5/1 | 5/5/1 | 4/4/1 | 5/5/1 | 5/5/1 | 5/5/1 | 5/5/1 |
| R8G8B8_SNORM | 5/5/1 | 6/6/1 | 6/6/1 | 6/6/1 | 5/5/1 | 6/6/1 | 6/6/1 | 6/6/1 | 6/6/1 |
| R8G8B8_SRGB | 8/7/0 | 8/7/0 | 8/7/0 | 8/7/0 | 8/7/0 | 8/7/0 | 8/7/0 | 8/7/0 | 8/8/0 |
| R8G8B8_UINT | 4/4/1 | 5/5/1 | 5/5/1 | 5/5/1 | 4/4/1 | 5/5/1 | 5/5/1 | 5/5/1 | 5/5/1 |
| R8G8B8_UNORM | 8/8/1 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 |
| R8G8_SINT | 7/7/2 | 7/7/3 | 7/7/3 | 7/7/3 | 7/7/3 | 7/7/3 | 7/7/3 | 7/7/3 | 7/7/3 |
| R8G8_SNORM | 9/9/2 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 |
| R8G8_UINT | 7/7/2 | 7/7/3 | 7/7/3 | 7/7/3 | 7/7/3 | 7/7/3 | 7/7/3 | 7/7/3 | 7/7/3 |
| R8G8_UNORM | 9/9/2 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 |
| R8_SINT | 7/7/2 | 7/7/3 | 7/7/3 | 7/7/3 | 7/7/3 | 7/7/3 | 7/7/3 | 7/7/3 | 7/7/3 |
| R8_SNORM | 9/9/2 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 |
| R8_UINT | 7/7/2 | 7/7/3 | 7/7/3 | 7/7/3 | 7/7/3 | 7/7/3 | 7/7/3 | 7/7/3 | 7/7/3 |
| R8_UNORM | 9/9/2 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 | 9/9/3 |
| X8_D24_UNORM_PACK32 | 7/7/0 | 7/7/0 | 7/7/0 | 7/7/0 | 7/0/0 | 7/7/0 | 7/7/0 | 7/7/0 | 7/7/0 |

Notable: v6 lacks R64 (no 64-bit image atomics, `shaderInt64` false); v11 has no linear tiling for R64 and v10+ drop linear tiling of `D24_UNORM_S8_UINT`/`X8_D24` (Valhall CSF) while v6/v7/v9 keep it; 3-channel formats (R8G8B8, R16G16B16, R32G32B32) are exposed on all arches, with fewer buffer bits on v6; extra storage/blend bits on v7+ for 16-bit unorm/snorm and packed 10:10:10:2.

## 6. Key limits and properties

Values as reported. `-` = not reported (driver predates the extension or the extension is absent).


### 6.1 Core limits

| Property | v6 | v7 | v9 | v10 | v11 | v12 | v13 | v14 | v15 |
|---|---|---|---|---|---|---|---|---|---|
| maxImageDimension2D | 16383 | 16384 | 16384 | 16384 | 16384 | 16384 | 16384 | 16384 | 16384 |
| maxImageDimension3D | 16383 | 16384 | 16384 | 16384 | 16384 | 16384 | 16384 | 16384 | 16384 |
| maxImageArrayLayers | 1024 | 4096 | 4096 | 4096 | 4096 | 4096 | 4096 | 4096 | 4096 |
| maxTexelBufferElements | 65536 | 268,435,456 | 268,435,456 | 268,435,456 | 268,435,456 | 268,435,456 | 268,435,456 | 268,435,456 | 268,435,456 |
| maxUniformBufferRange | 65536 | 65536 | 2,147,483,648 | 2,147,483,648 | 2,147,483,648 | 2,147,483,648 | 2,147,483,648 | 2,147,483,648 | 2,147,483,648 |
| maxStorageBufferRange | 268,435,456 | 2,147,483,648 | 2,147,483,648 | 2,147,483,648 | 2,147,483,648 | 2,147,483,648 | 2,147,483,648 | 2,147,483,648 | 2,147,483,648 |
| maxPushConstantsSize | 256 | 256 | 256 | 256 | 256 | 256 | 256 | 256 | 256 |
| maxMemoryAllocationCount | 4,294,967,295 | 16384 | 16384 | 16384 | 16384 | 16384 | 16384 | 16384 | 16384 |
| maxBoundDescriptorSets | 4 | 7 | 7 | 7 | 7 | 7 | 7 | 7 | 7 |
| maxPerStageResources | 365 | 570 | 8,388,608 | 8,388,608 | 500,000 | 500,000 | 500,000 | 8,388,608 | 8,388,608 |
| maxPerSetDescriptors | 65536 | 65536 | 8,388,608 | 8,388,608 | 500,000 | 500,000 | 500,000 | 8,388,608 | 8,388,608 |
| maxPerStageDescriptorSamplers | 128 | 128 | 8,388,608 | 8,388,608 | 500,000 | 500,000 | 500,000 | 8,388,608 | 8,388,608 |
| maxPerStageDescriptorUniformBuffers | 36 | 241 | 8,388,608 | 8,388,608 | 36 | 500,000 | 500,000 | 8,388,608 | 8,388,608 |
| maxPerStageDescriptorStorageBuffers | 35 | 35 | 8,388,608 | 8,388,608 | 500,000 | 500,000 | 500,000 | 8,388,608 | 8,388,608 |
| maxPerStageDescriptorSampledImages | 256 | 256 | 8,388,608 | 8,388,608 | 500,000 | 500,000 | 500,000 | 8,388,608 | 8,388,608 |
| maxPerStageDescriptorStorageImages | 21 | 21 | 8,388,608 | 8,388,608 | 500,000 | 500,000 | 500,000 | 8,388,608 | 8,388,608 |
| maxDescriptorSetUniformBuffers | 216 | 1446 | 8,388,608 | 8,388,608 | 216 | 500,000 | 500,000 | 8,388,608 | 8,388,608 |
| maxBufferSize | - | 3,890,720,768 | 4,294,967,295 | 4,294,967,295 | 8,589,934,592 | 8,589,934,592 | 8,589,934,592 | 8,589,934,592 | 8,589,934,592 |
| maxVertexInputAttributes | 32 | 32 | 32 | 32 | 32 | 32 | 32 | 32 | 32 |
| maxVertexInputBindings | 32 | 32 | 32 | 32 | 32 | 32 | 32 | 32 | 32 |
| maxVertexOutputComponents | 128 | 128 | 128 | 128 | 128 | 128 | 128 | 128 | 128 |
| maxFragmentInputComponents | 128 | 128 | 128 | 128 | 128 | 128 | 128 | 128 | 128 |
| maxFragmentOutputAttachments | 8 | 8 | 8 | 8 | 8 | 8 | 8 | 8 | 8 |
| maxFragmentDualSrcAttachments | 0 | 8 | 8 | 8 | 0 | 8 | 8 | 8 | 8 |
| maxColorAttachments | 8 | 8 | 8 | 8 | 8 | 8 | 8 | 8 | 8 |
| maxFragmentCombinedOutputResources | 64 | 64 | 16,777,224 | 16,777,224 | 1,000,008 | 1,000,008 | 1,000,008 | 16,777,224 | 16,777,224 |
| maxComputeSharedMemorySize | 32768 | 32768 | 32768 | 32768 | 32768 | 32768 | 32768 | 32768 | 32768 |
| maxComputeWorkGroupInvocations | 384 | 384 | 512 | 1024 | 1024 | 1024 | 1024 | 1024 | 1024 |
| maxComputeWorkGroupSize | [384,384,384] | [384,384,384] | [512,512,512] | [1024,1024,1024] | [1024,1024,1024] | [1024,1024,1024] | [1024,1024,1024] | [1024,1024,1024] | [1024,1024,1024] |
| maxDrawIndirectCount | 1 | 1 | 1 | 4,294,967,295 | 4,294,967,295 | 4,294,967,295 | 4,294,967,295 | 4,294,967,295 | 4,294,967,295 |
| maxSamplerAnisotropy | 16 | 16 | 16 | 16 | 16 | 16 | 16 | 16 | 16 |
| maxViewports | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 16 | 16 |
| maxViewportDimensions | [16383,16383] | [16384,16384] | [16384,16384] | [16384,16384] | [16384,16384] | [16384,16384] | [16384,16384] | [16384,16384] | [16384,16384] |
| maxFramebufferWidth | 16383 | 16384 | 16384 | 16384 | 16384 | 16384 | 16384 | 16384 | 16384 |
| maxClipDistances | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| maxCullDistances | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| maxCombinedClipAndCullDistances | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| lineWidthRange | [1,1] | [1,4095.94] | [1,4095.94] | [1,4095.94] | [1,1] | [1,4095.94] | [1,4095.94] | [1,4095.94] | [1,4095.94] |
| lineWidthGranularity | 0 | 0.0625 | 0.0625 | 0.0625 | 0 | 0.0625 | 0.0625 | 0.0625 | 0.0625 |
| pointSizeRange | [1,1024] | [1,1024] | [1,1024] | [1,1024] | [1,1024] | [1,1024] | [1,1024] | [1,1024] | [1,1024] |
| pointSizeGranularity | 0.0625 | 0.0625 | 0.0625 | 0.0625 | 0.0625 | 0.0625 | 0.0625 | 0.0625 | 0.0625 |
| minUniformBufferOffsetAlignment | 16 | 16 | 16 | 16 | 16 | 16 | 16 | 16 | 8 |
| minStorageBufferOffsetAlignment | 64 | 64 | 64 | 64 | 64 | 64 | 64 | 64 | 64 |
| minTexelBufferOffsetAlignment | 64 | 64 | 64 | 64 | 64 | 64 | 64 | 64 | 64 |
| nonCoherentAtomSize | 64 | 64 | 64 | 64 | 64 | 64 | 64 | 64 | 64 |
| timestampPeriod | 0 | 41.6667 | 76.9231 | 76.9231 | 76.9231 | 76.9231 | 76.9231 | 0.999001 | 0.999001 |
| framebufferColorSampleCounts | 1,4 | 1,4,8 | 1,4,8 | 1,4,8,16 | 1,4,8,16 | 1,2,4,8,16 | 1,2,4,8,16 | 1,2,4,8,16 | 1,2,4,8,16 |
| sampledImageColorSampleCounts | 1,4 | 1,4,8 | 1,4,8 | 1,4,8,16 | 1,4,8,16 | 1,2,4,8,16 | 1,2,4,8,16 | 1,2,4,8,16 | 1,2,4,8,16 |
| storageImageSampleCounts | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 |
| pointClippingBehavior | POINT_CLIPPING_BEHAVIOR_USER_CLIP_ | POINT_CLIPPING_BEHAVIOR_USER_CLIP_ | POINT_CLIPPING_BEHAVIOR_USER_CLIP_ | POINT_CLIPPING_BEHAVIOR_USER_CLIP_ | POINT_CLIPPING_BEHAVIOR_USER_CLIP_ | POINT_CLIPPING_BEHAVIOR_USER_CLIP_ | POINT_CLIPPING_BEHAVIOR_USER_CLIP_ | POINT_CLIPPING_BEHAVIOR_USER_CLIP_ | POINT_CLIPPING_BEHAVIOR_ALL_CLIP_P |

### 6.2 Geometry / tessellation

| Property | v6 | v7 | v9 | v10 | v11 | v12 | v13 | v14 | v15 |
|---|---|---|---|---|---|---|---|---|---|
| maxGeometryShaderInvocations | 32 | 32 | 32 | 32 | 32 | 32 | 32 | 32 | 32 |
| maxGeometryInputComponents | 64 | 128 | 128 | 128 | 128 | 128 | 128 | 128 | 128 |
| maxGeometryOutputVertices | 256 | 256 | 256 | 256 | 256 | 256 | 256 | 256 | 256 |
| maxGeometryTotalOutputComponents | 1024 | 2048 | 2048 | 2048 | 2048 | 2048 | 2048 | 2048 | 2048 |
| maxTessellationGenerationLevel | 64 | 64 | 64 | 64 | 64 | 64 | 64 | 64 | 64 |
| maxTessellationPatchSize | 32 | 32 | 32 | 32 | 32 | 32 | 32 | 32 | 32 |
| maxTessellationControlPerVertexInputComponents | 128 | 128 | 128 | 128 | 128 | 128 | 128 | 128 | 128 |

### 6.3 Subgroup

| Property | v6 | v7 | v9 | v10 | v11 | v12 | v13 | v14 | v15 |
|---|---|---|---|---|---|---|---|---|---|
| subgroupSize | 4 | 8 | 16 | 16 | 16 | 16 | 16 | 16 | 16 |
| subgroupSupportedStages | - | fragment,compute | fragment,compute | fragment,compute | 8 flags | 8 flags | 8 flags | 8 flags | 8 flags |
| subgroupSupportedOperations | - | 8 flags | 8 flags | 8 flags | 8 flags | 8 flags | 8 flags | 8 flags | 8 flags |
| subgroupQuadOperationsInAllStages | - | n | n | n | n | n | n | n | n |
| minSubgroupSize | - | 8 | 16 | 16 | 16 | 16 | 16 | 16 | 16 |
| maxSubgroupSize | - | 8 | 16 | 16 | 16 | 16 | 16 | 16 | 16 |
| maxComputeWorkgroupSubgroups | - | 48 | 32 | 64 | 64 | 64 | 64 | 64 | 64 |
| requiredSubgroupSizeStages | - | fragment,compute | fragment,compute | fragment,compute | 8 flags | 8 flags | 8 flags | 8 flags | 8 flags |

### 6.4 Descriptor indexing

| Property | v6 | v7 | v9 | v10 | v11 | v12 | v13 | v14 | v15 |
|---|---|---|---|---|---|---|---|---|---|
| maxUpdateAfterBindDescriptorsInAllPools | - | 0 | 4,294,967,295 | 4,294,967,295 | 4,294,967,295 | 4,294,967,295 | 4,294,967,295 | 4,294,967,295 | 4,294,967,295 |
| shaderSampledImageArrayNonUniformIndexingNative | - | n | n | n | n | n | n | n | n |
| shaderStorageBufferArrayNonUniformIndexingNative | - | n | Y | Y | Y | Y | Y | Y | Y |
| robustBufferAccessUpdateAfterBind | - | n | Y | Y | Y | Y | Y | Y | Y |
| quadDivergentImplicitLod | - | n | n | Y | n | n | n | Y | Y |
| maxDescriptorSetUpdateAfterBindSamplers | - | 768 | 8,388,608 | 8,388,608 | 500,000 | 500,000 | 500,000 | 8,388,608 | 8,388,608 |
| maxPerStageUpdateAfterBindResources | - | 570 | 8,388,608 | 8,388,608 | 500,000 | 500,000 | 500,000 | 8,388,608 | 8,388,608 |
| maxDescriptorSetUpdateAfterBindUniformBuffers | - | 1446 | 8,388,608 | 8,388,608 | 216 | 500,000 | 500,000 | 8,388,608 | 8,388,608 |

### 6.5 Transform feedback

| Property | v6 | v7 | v9 | v10 | v11 | v12 | v13 | v14 | v15 |
|---|---|---|---|---|---|---|---|---|---|
| maxTransformFeedbackStreams | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 4 |
| maxTransformFeedbackBuffers | 4 | 4 | 4 | 4 | 4 | 4 | 4 | 4 | 4 |
| maxTransformFeedbackBufferSize | 268,435,456 | 268,435,456 | 268,435,456 | 268,435,456 | 268,435,456 | 268,435,456 | 268,435,456 | 268,435,456 | 268,435,456 |
| maxTransformFeedbackStreamDataSize | 512 | 512 | 512 | 512 | 512 | 512 | 512 | 512 | 512 |
| maxTransformFeedbackBufferDataSize | 512 | 512 | 512 | 512 | 512 | 512 | 512 | 512 | 512 |
| maxTransformFeedbackBufferDataStride | 512 | 512 | 512 | 512 | 512 | 512 | 512 | 512 | 512 |
| transformFeedbackQueries | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| transformFeedbackStreamsLinesTriangles | n | n | n | n | n | n | n | n | Y |
| transformFeedbackRasterizationStreamSelect | n | n | n | n | n | n | n | n | Y |
| transformFeedbackDraw | n | n | n | n | n | n | n | n | Y |

### 6.6 Multiview, conservative raster, fragment shading rate

| Property | v6 | v7 | v9 | v10 | v11 | v12 | v13 | v14 | v15 |
|---|---|---|---|---|---|---|---|---|---|
| maxMultiviewViewCount | 8 | 8 | 8 | 8 | 8 | 8 | 8 | 16 | 16 |
| maxMultiviewInstanceIndex | 4,294,967,295 | 4,294,967,295 | 4,294,967,295 | 4,294,967,295 | 4,294,967,295 | 4,294,967,295 | 4,294,967,295 | 4,294,967,295 | 4,294,967,295 |
| primitiveOverestimationSize | - | - | - | - | 0.00195312 | 0.00195312 | 0.00195312 | 0.00195312 | 0.00195312 |
| maxExtraPrimitiveOverestimationSize | - | - | - | - | 0 | 0 | 0 | 0 | 0 |
| conservativePointAndLineRasterization | - | - | - | - | n | n | n | n | n |
| degenerateTrianglesRasterized | - | - | - | - | Y | n | n | Y | Y |
| primitiveUnderestimation | - | - | - | - | n | n | n | n | n |
| fullyCoveredFragmentShaderInputVariable | - | - | - | - | n | n | n | n | n |
| conservativeRasterizationPostDepthCoverage | - | - | - | - | n | n | n | n | n |
| fragmentShadingRateNonTrivialCombinerOps | - | - | - | - | Y | Y | Y | Y | Y |
| primitiveFragmentShadingRateWithMultipleViewports | - | - | - | - | n | n | n | n | n |

### 6.7 Sparse

| Property | v6 | v7 | v9 | v10 | v11 | v12 | v13 | v14 | v15 |
|---|---|---|---|---|---|---|---|---|---|
| sparseAddressSpaceSize | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |

### 6.8 Robustness, descriptor buffer, misc

| Property | v6 | v7 | v9 | v10 | v11 | v12 | v13 | v14 | v15 |
|---|---|---|---|---|---|---|---|---|---|
| robustStorageBufferAccessSizeAlignment | - | - | 1 | 1 | - | - | - | 4 | 4 |
| robustUniformBufferAccessSizeAlignment | - | - | 1 | 1 | - | - | - | 256 | 256 |
| descriptorBufferOffsetAlignment | - | - | - | 32 | - | - | - | 32 | 32 |
| maxDescriptorBufferBindings | - | - | - | 7 | - | - | - | 7 | 7 |
| maxResourceDescriptorBufferRange | - | - | - | 4,294,967,295 | - | - | - | 68,719,476,736 | 68,719,476,736 |
| maxVertexAttribDivisor | - | - | 4,294,967,295 | 4,294,967,295 | - | 4,294,967,295 | 4,294,967,295 | 4,294,967,295 | 4,294,967,295 |
| maxPushDescriptors | - | - | 32 | 32 | - | 32 | 32 | 32 | 32 |
| maxPerStageDescriptorUpdateAfterBindSamplers | - | 128 | 8,388,608 | 8,388,608 | 500,000 | 500,000 | 500,000 | 8,388,608 | 8,388,608 |
| maxCustomBorderColorSamplers | - | 4,294,967,295 | 4,294,967,295 | 4,294,967,295 | 4,294,967,295 | 4,294,967,295 | 4,294,967,295 | 4,294,967,295 | 4,294,967,295 |
| maxOpacity2StateSubdivisionLevel | - | - | - | - | - | - | 10 | 10 | 10 |
| maxRenderPassStripes | - | - | - | - | - | 32 | 32 | 32 | 32 |
| fmaRate | - | 0 | 32 | 64 | 128 | 128 | 128 | 128 | 128 |
| discreteQueuePriorities | 2 | 2 | 2 | 2 | 2 | 2 | 2 | 2 | 4 |
| maxRayRecursionDepth | - | - | - | - | - | - | - | - | 32 |

### 6.9 Remaining properties that differ (176)

Descriptor-size and capture-replay fields, UUIDs, tensor/data-graph, pipeline-binary and performance-counter fields are omitted (see raw JSON).

| Property | v6 | v7 | v9 | v10 | v11 | v12 | v13 | v14 | v15 |
|---|---|---|---|---|---|---|---|---|---|
| allowSamplerImageViewPostSubmitCreation | - | - | - | Y | - | - | - | Y | Y |
| blockTexelViewCompatibleMultipleLayers | - | - | Y | Y | - | n | n | Y | Y |
| bufferImageGranularity | 4096 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 |
| bufferlessPushDescriptors | - | - | - | n | - | - | - | n | n |
| combinedImageSamplerDescriptorSingleArray | - | - | - | n | - | - | - | n | n |
| cooperativeMatrixSupportedStages | - | - | - | - | - | compute | compute | compute | compute |
| copyDstLayoutCount | - | 2 | 2 | 2 | - | - | - | 2 | 20 |
| copySrcLayoutCount | - | 4 | 4 | 4 | - | - | - | 4 | 21 |
| defaultRobustnessImages | - | PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR | PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR | PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR | PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR | PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR | PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR | PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR | PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR |
| defaultRobustnessStorageBuffers | - | PIPELINE_ROBUSTNESS_BUFFER_BEHAVIO | PIPELINE_ROBUSTNESS_BUFFER_BEHAVIO | PIPELINE_ROBUSTNESS_BUFFER_BEHAVIO | PIPELINE_ROBUSTNESS_BUFFER_BEHAVIO | PIPELINE_ROBUSTNESS_BUFFER_BEHAVIO | PIPELINE_ROBUSTNESS_BUFFER_BEHAVIO | PIPELINE_ROBUSTNESS_BUFFER_BEHAVIO | PIPELINE_ROBUSTNESS_BUFFER_BEHAVIO |
| defaultRobustnessUniformBuffers | - | PIPELINE_ROBUSTNESS_BUFFER_BEHAVIO | PIPELINE_ROBUSTNESS_BUFFER_BEHAVIO | PIPELINE_ROBUSTNESS_BUFFER_BEHAVIO | PIPELINE_ROBUSTNESS_BUFFER_BEHAVIO | PIPELINE_ROBUSTNESS_BUFFER_BEHAVIO | PIPELINE_ROBUSTNESS_BUFFER_BEHAVIO | PIPELINE_ROBUSTNESS_BUFFER_BEHAVIO | PIPELINE_ROBUSTNESS_BUFFER_BEHAVIO |
| defaultRobustnessVertexInputs | - | PIPELINE_ROBUSTNESS_BUFFER_BEHAVIO | PIPELINE_ROBUSTNESS_BUFFER_BEHAVIO | PIPELINE_ROBUSTNESS_BUFFER_BEHAVIO | PIPELINE_ROBUSTNESS_BUFFER_BEHAVIO | PIPELINE_ROBUSTNESS_BUFFER_BEHAVIO | PIPELINE_ROBUSTNESS_BUFFER_BEHAVIO | PIPELINE_ROBUSTNESS_BUFFER_BEHAVIO | PIPELINE_ROBUSTNESS_BUFFER_BEHAVIO |
| defaultVertexAttributeValue | - | - | - | - | - | - | - | - | DEFAULT_VERTEX_ATTRIBUTE_VALUE_ZER |
| degenerateLinesRasterized | - | - | - | - | n | n | n | n | n |
| depthStencilSwizzleOneSupport | - | - | Y | Y | - | Y | Y | Y | Y |
| descriptorBufferAddressSpaceSize | - | - | - | 4,294,967,295 | - | - | - | 68,719,476,736 | 68,719,476,736 |
| deviceLUID | - | [0,0,0] | [0,0,0] | [0,0,0] | [0,0,0] | [0,0,0] | [0,0,0] | [0,0,0] | [0,0,0] |
| deviceLUIDValid | - | n | n | n | n | n | n | n | n |
| deviceNodeMask | - | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| dynamicPrimitiveTopologyUnrestricted | - | n | n | n | - | n | n | n | n |
| earlyFragmentMultisampleCoverageAfterSampleCounting | - | - | Y | Y | - | Y | Y | Y | Y |
| earlyFragmentSampleMaskTestBeforeSampleCounting | - | - | Y | Y | - | Y | Y | Y | Y |
| externalFormatResolveChromaOffsetX | - | - | CHROMA_LOCATION_MIDPOINT | CHROMA_LOCATION_MIDPOINT | - | CHROMA_LOCATION_MIDPOINT | CHROMA_LOCATION_MIDPOINT | CHROMA_LOCATION_MIDPOINT | CHROMA_LOCATION_MIDPOINT |
| externalFormatResolveChromaOffsetY | - | - | CHROMA_LOCATION_MIDPOINT | CHROMA_LOCATION_MIDPOINT | - | CHROMA_LOCATION_MIDPOINT | CHROMA_LOCATION_MIDPOINT | CHROMA_LOCATION_MIDPOINT | CHROMA_LOCATION_MIDPOINT |
| extraPrimitiveOverestimationSizeGranularity | - | - | - | - | 0 | 0 | 0 | 0 | 0 |
| filterMinmaxImageComponentMapping | - | n | n | Y | Y | Y | Y | Y | Y |
| filterMinmaxSingleComponentFormats | - | n | n | Y | Y | Y | Y | Y | Y |
| fragmentDensityInvocations | - | - | Y | Y | Y | Y | Y | Y | Y |
| fragmentShadingRateClampCombinerInputs | - | - | Y | Y | - | Y | Y | Y | Y |
| fragmentShadingRateStrictMultiplyCombiner | - | - | - | - | Y | Y | Y | Y | Y |
| fragmentShadingRateWithConservativeRasterization | - | - | - | - | Y | Y | Y | Y | Y |
| fragmentShadingRateWithCustomSampleLocations | - | - | - | - | n | n | n | n | n |
| fragmentShadingRateWithFragmentShaderInterlock | - | - | - | - | n | n | n | n | n |
| fragmentShadingRateWithSampleMask | - | - | - | - | Y | Y | Y | Y | Y |
| fragmentShadingRateWithShaderDepthStencilWrites | - | - | - | - | Y | Y | Y | Y | Y |
| fragmentShadingRateWithShaderSampleMask | - | - | - | - | Y | Y | Y | Y | Y |
| graphicsPipelineLibraryFastLinking | - | - | n | n | - | - | - | n | n |
| graphicsPipelineLibraryIndependentInterpolationDecoration | - | - | n | n | - | - | - | Y | Y |
| identicalMemoryTypeRequirements | - | Y | Y | Y | - | - | - | Y | Y |
| identityTransformOrder | - | - | - | - | - | - | - | - | Y |
| image2DViewOf3DSparse | - | - | - | - | - | - | - | - | n |
| integerDotProduct16BitMixedSignednessAccelerated | - | n | n | n | n | n | n | n | n |
| integerDotProduct16BitSignedAccelerated | - | n | n | n | n | n | n | n | n |
| integerDotProduct16BitUnsignedAccelerated | - | n | n | n | n | n | n | n | n |
| integerDotProduct32BitMixedSignednessAccelerated | - | n | n | n | n | n | n | n | n |
| integerDotProduct32BitSignedAccelerated | - | n | n | n | n | n | n | n | n |
| integerDotProduct32BitUnsignedAccelerated | - | n | n | n | n | n | n | n | n |
| integerDotProduct4x8BitPackedMixedSignednessAccelerated | - | n | n | n | n | n | n | n | n |
| integerDotProduct4x8BitPackedSignedAccelerated | - | Y | Y | Y | Y | Y | Y | Y | Y |
| integerDotProduct4x8BitPackedUnsignedAccelerated | - | Y | Y | Y | Y | Y | Y | Y | Y |
| integerDotProduct64BitMixedSignednessAccelerated | - | n | n | n | n | n | n | n | n |
| integerDotProduct64BitSignedAccelerated | - | n | n | n | n | n | n | n | n |
| integerDotProduct64BitUnsignedAccelerated | - | n | n | n | n | n | n | n | n |
| integerDotProduct8BitMixedSignednessAccelerated | - | n | n | n | n | n | n | n | n |
| integerDotProduct8BitSignedAccelerated | - | Y | Y | Y | Y | Y | Y | Y | Y |
| integerDotProduct8BitUnsignedAccelerated | - | Y | Y | Y | Y | Y | Y | Y | Y |
| integerDotProductAccumulatingSaturating16BitMixedSignednessAccelerated | - | n | n | n | n | n | n | n | n |
| integerDotProductAccumulatingSaturating16BitSignedAccelerated | - | n | n | n | n | n | n | n | n |
| integerDotProductAccumulatingSaturating16BitUnsignedAccelerated | - | n | n | n | n | n | n | n | n |
| integerDotProductAccumulatingSaturating32BitMixedSignednessAccelerated | - | n | n | n | n | n | n | n | n |
| integerDotProductAccumulatingSaturating32BitSignedAccelerated | - | n | n | n | n | n | n | n | n |
| integerDotProductAccumulatingSaturating32BitUnsignedAccelerated | - | n | n | n | n | n | n | n | n |
| integerDotProductAccumulatingSaturating4x8BitPackedMixedSignednessAccelerated | - | n | n | n | n | n | n | n | n |
| integerDotProductAccumulatingSaturating4x8BitPackedSignedAccelerated | - | n | Y | Y | Y | Y | Y | Y | Y |
| integerDotProductAccumulatingSaturating4x8BitPackedUnsignedAccelerated | - | n | Y | Y | Y | Y | Y | Y | Y |
| integerDotProductAccumulatingSaturating64BitMixedSignednessAccelerated | - | n | n | n | n | n | n | n | n |
| integerDotProductAccumulatingSaturating64BitSignedAccelerated | - | n | n | n | n | n | n | n | n |
| integerDotProductAccumulatingSaturating64BitUnsignedAccelerated | - | n | n | n | n | n | n | n | n |
| integerDotProductAccumulatingSaturating8BitMixedSignednessAccelerated | - | n | n | n | n | n | n | n | n |
| integerDotProductAccumulatingSaturating8BitSignedAccelerated | - | n | Y | Y | Y | Y | Y | Y | Y |
| integerDotProductAccumulatingSaturating8BitUnsignedAccelerated | - | n | Y | Y | Y | Y | Y | Y | Y |
| layeredShadingRateAttachments | - | - | - | - | Y | Y | Y | Y | Y |
| maxCombinedImageSamplerDescriptorCount | - | - | 1 | 1 | - | 1 | 1 | 1 | 1 |
| maxDescriptorSetAccelerationStructures | - | - | - | - | 500,000 | 500,000 | 500,000 | 8,388,608 | 8,388,608 |
| maxDescriptorSetInlineUniformBlocks | 4 | 192 | 192 | 192 | 192 | 192 | 192 | 192 | 192 |
| maxDescriptorSetSampledImages | 1536 | 1536 | 8,388,608 | 8,388,608 | 500,000 | 500,000 | 500,000 | 8,388,608 | 8,388,608 |
| maxDescriptorSetSamplers | 768 | 768 | 8,388,608 | 8,388,608 | 500,000 | 500,000 | 500,000 | 8,388,608 | 8,388,608 |
| maxDescriptorSetStorageBuffers | 210 | 210 | 8,388,608 | 8,388,608 | 500,000 | 500,000 | 500,000 | 8,388,608 | 8,388,608 |
| maxDescriptorSetStorageImages | 126 | 126 | 8,388,608 | 8,388,608 | 500,000 | 500,000 | 500,000 | 8,388,608 | 8,388,608 |
| maxDescriptorSetSubsampledSamplers | - | - | 8 | 8 | 8 | 8 | 8 | 8 | 8 |
| maxDescriptorSetTotalBuffersDynamic | - | - | 64 | 64 | - | - | - | 64 | 64 |
| maxDescriptorSetTotalStorageBuffersDynamic | - | - | 32 | 32 | - | - | - | 32 | 32 |
| maxDescriptorSetTotalUniformBuffersDynamic | - | - | 32 | 32 | - | - | - | 32 | 32 |
| maxDescriptorSetUpdateAfterBindAccelerationStructures | - | - | - | - | 500,000 | 500,000 | 500,000 | 8,388,608 | 8,388,608 |
| maxDescriptorSetUpdateAfterBindInlineUniformBlocks | 4 | 192 | 192 | 192 | 192 | 192 | 192 | 192 | 192 |
| maxDescriptorSetUpdateAfterBindInputAttachments | - | 9 | 9 | 9 | 9 | 9 | 9 | 9 | 9 |
| maxDescriptorSetUpdateAfterBindSampledImages | - | 1536 | 8,388,608 | 8,388,608 | 500,000 | 500,000 | 500,000 | 8,388,608 | 8,388,608 |
| maxDescriptorSetUpdateAfterBindStorageBuffers | - | 210 | 8,388,608 | 8,388,608 | 500,000 | 500,000 | 500,000 | 8,388,608 | 8,388,608 |
| maxDescriptorSetUpdateAfterBindStorageBuffersDynamic | - | 32 | 32 | 32 | 32 | 32 | 32 | 32 | 32 |
| maxDescriptorSetUpdateAfterBindStorageImages | - | 126 | 8,388,608 | 8,388,608 | 500,000 | 500,000 | 500,000 | 8,388,608 | 8,388,608 |
| maxDescriptorSetUpdateAfterBindTotalBuffersDynamic | - | - | 64 | 64 | - | - | - | 64 | 64 |
| maxDescriptorSetUpdateAfterBindTotalStorageBuffersDynamic | - | - | 32 | 32 | - | - | - | 32 | 32 |
| maxDescriptorSetUpdateAfterBindTotalUniformBuffersDynamic | - | - | 32 | 32 | - | - | - | 32 | 32 |
| maxDescriptorSetUpdateAfterBindUniformBuffersDynamic | - | 32 | 32 | 32 | 32 | 32 | 32 | 32 | 32 |
| maxEmbeddedImmutableSamplerBindings | - | - | - | 7 | - | - | - | 7 | 7 |
| maxEmbeddedImmutableSamplers | - | - | - | 8,388,608 | - | - | - | 8,388,608 | 8,388,608 |
| maxFragmentDensityTexelSize.height | - | - | 32 | 32 | 32 | 64 | 64 | 64 | 64 |
| maxFragmentDensityTexelSize.width | - | - | 32 | 32 | 32 | 64 | 64 | 64 | 64 |
| maxFragmentShadingRateAttachmentTexelSize.height | - | - | - | - | 32 | 32 | 32 | 32 | 32 |
| maxFragmentShadingRateAttachmentTexelSize.width | - | - | - | - | 32 | 32 | 32 | 32 | 32 |
| maxFragmentShadingRateAttachmentTexelSizeAspectRatio | - | - | - | - | 1 | 1 | 1 | 1 | 1 |
| maxFragmentShadingRateCoverageSamples | - | - | - | - | 16 | 16 | 16 | 16 | 16 |
| maxFragmentShadingRateRasterizationSamples | - | - | - | - | SAMPLE_COUNT_4_BIT | SAMPLE_COUNT_4_BIT | SAMPLE_COUNT_4_BIT | SAMPLE_COUNT_4_BIT | SAMPLE_COUNT_4_BIT |
| maxFragmentSize.height | - | - | - | - | 4 | 4 | 4 | 4 | 4 |
| maxFragmentSize.width | - | - | - | - | 4 | 4 | 4 | 4 | 4 |
| maxFragmentSizeAspectRatio | - | - | - | - | 2 | 2 | 2 | 2 | 2 |
| maxFramebufferHeight | 16383 | 16384 | 16384 | 16384 | 16384 | 16384 | 16384 | 16384 | 16384 |
| maxGeometryCount | - | - | - | - | 16,777,215 | 16,777,215 | 16,777,215 | 16,777,215 | 16,777,215 |
| maxImageDimensionCube | 16383 | 16384 | 16384 | 16384 | 16384 | 16384 | 16384 | 16384 | 16384 |
| maxInlineUniformTotalSize | - | 12,582,912 | 12,582,912 | 12,582,912 | 12,582,912 | 12,582,912 | 12,582,912 | 12,582,912 | 12,582,912 |
| maxInstanceCount | - | - | - | - | 16,777,215 | 16,777,215 | 16,777,215 | 16,777,215 | 16,777,215 |
| maxInterpolationOffset | 0.5 | 0.4375 | 0.4375 | 0.4375 | 0.4375 | 0.4375 | 0.4375 | 0.4375 | 0.4375 |
| maxMemoryAllocationSize | 4,052,865,024 | 3,890,720,768 | 8,256,499,712 | 8,589,934,592 | 8,589,934,592 | 8,589,934,592 | 8,589,934,592 | 8,589,934,592 | 8,589,934,592 |
| maxOpacity4StateSubdivisionLevel | - | - | - | - | - | - | 10 | 10 | 10 |
| maxPerStageDescriptorAccelerationStructures | - | - | - | - | 500,000 | 500,000 | 500,000 | 8,388,608 | 8,388,608 |
| maxPerStageDescriptorInlineUniformBlocks | 4 | 32 | 32 | 32 | 32 | 32 | 32 | 32 | 32 |
| maxPerStageDescriptorUpdateAfterBindAccelerationStructures | - | - | - | - | 500,000 | 500,000 | 500,000 | 8,388,608 | 8,388,608 |
| maxPerStageDescriptorUpdateAfterBindInlineUniformBlocks | 4 | 32 | 32 | 32 | 32 | 32 | 32 | 32 | 32 |
| maxPerStageDescriptorUpdateAfterBindInputAttachments | - | 9 | 9 | 9 | 9 | 9 | 9 | 9 | 9 |
| maxPerStageDescriptorUpdateAfterBindSampledImages | - | 256 | 8,388,608 | 8,388,608 | 500,000 | 500,000 | 500,000 | 8,388,608 | 8,388,608 |
| maxPerStageDescriptorUpdateAfterBindStorageBuffers | - | 35 | 8,388,608 | 8,388,608 | 500,000 | 500,000 | 500,000 | 8,388,608 | 8,388,608 |
| maxPerStageDescriptorUpdateAfterBindStorageImages | - | 21 | 8,388,608 | 8,388,608 | 500,000 | 500,000 | 500,000 | 8,388,608 | 8,388,608 |
| maxPerStageDescriptorUpdateAfterBindUniformBuffers | - | 241 | 8,388,608 | 8,388,608 | 36 | 500,000 | 500,000 | 8,388,608 | 8,388,608 |
| maxPrimitiveCount | - | - | - | - | 536,870,911 | 536,870,911 | 536,870,911 | 536,870,911 | 536,870,911 |
| maxRayDispatchInvocationCount | - | - | - | - | - | - | - | - | 1,073,741,824 |
| maxRayHitAttributeSize | - | - | - | - | - | - | - | - | 32 |
| maxSamplerLodBias | 255 | 126 | 126 | 126 | 126 | 126 | 126 | 126 | 126 |
| maxShaderGroupStride | - | - | - | - | - | - | - | - | 4096 |
| maxSubsampledArrayLayers | - | - | 4096 | 4096 | 4096 | 4096 | 4096 | 4096 | 4096 |
| meshAndTaskShaderDerivatives | - | - | - | n | - | - | - | n | n |
| minAccelerationStructureScratchOffsetAlignment | - | - | - | - | 64 | 64 | 64 | 64 | 64 |
| minFragmentDensityTexelSize.height | - | - | 32 | 32 | 32 | 64 | 64 | 64 | 64 |
| minFragmentDensityTexelSize.width | - | - | 32 | 32 | 32 | 64 | 64 | 64 | 64 |
| minFragmentShadingRateAttachmentTexelSize.height | - | - | - | - | 8 | 8 | 8 | 8 | 8 |
| minFragmentShadingRateAttachmentTexelSize.width | - | - | - | - | 8 | 8 | 8 | 8 | 8 |
| minImportedHostPointerAlignment | - | - | 4096 | 4096 | - | - | - | 4096 | 4096 |
| nonStrictSinglePixelWideLinesUseParallelogram | - | - | n | n | - | Y | Y | n | n |
| nonStrictWideLinesUseParallelogram | - | - | n | n | - | Y | Y | n | n |
| nullColorAttachmentWithExternalFormatResolve | - | - | Y | Y | - | Y | Y | Y | Y |
| perBasicBlockGranularity | - | - | - | - | - | - | - | - | n |
| pixelRate | - | 0 | 2 | 4 | 4 | 4 | 4 | 4 | 4 |
| polygonModePointSize | - | - | n | n | - | n | n | n | n |
| protectedNoFault | - | n | n | n | n | n | n | n | n |
| provokingVertexModePerPipeline | - | n | n | n | n | n | n | n | n |
| regionAlignment | - | - | - | - | - | - | - | - | 16 |
| renderPassStripeGranularity.height | - | - | - | - | - | 64 | 64 | 64 | 64 |
| renderPassStripeGranularity.width | - | - | - | - | - | 64 | 64 | 64 | 64 |
| robustFragmentShadingRateAttachmentAccess | - | - | Y | Y | - | - | - | Y | Y |
| rowStrideAlignment | - | - | - | - | - | - | - | - | 64 |
| schedulingControlsFlags | - | - | - | 1 | - | 1 | 1 | 1 | 1 |
| separateDepthStencilAttachmentAccess | - | - | n | Y | - | - | - | Y | Y |
| shaderGroupBaseAlignment | - | - | - | - | - | - | - | - | 64 |
| shaderGroupHandleAlignment | - | - | - | - | - | - | - | - | 32 |
| shaderGroupHandleSize | - | - | - | - | - | - | - | - | 32 |
| shaderInputAttachmentArrayNonUniformIndexingNative | - | n | n | n | n | n | n | n | n |
| shaderStorageImageArrayNonUniformIndexingNative | - | n | n | n | n | n | n | n | n |
| shaderTileImageCoherentReadAccelerated | - | Y | Y | Y | Y | Y | Y | Y | Y |
| shaderTileImageReadFromHelperInvocation | - | Y | Y | Y | Y | Y | Y | Y | Y |
| shaderTileImageReadSampleFromPixelRateInvocation | - | Y | Y | Y | Y | Y | Y | Y | Y |
| shaderUniformBufferArrayNonUniformIndexingNative | - | n | n | n | n | n | n | n | n |
| shaderWarpsPerCore | - | 48 | 32 | 64 | 64 | 64 | 64 | 64 | 64 |
| storageTexelBufferOffsetAlignmentBytes | - | 64 | 64 | 64 | 64 | 64 | 64 | 64 | 64 |
| storageTexelBufferOffsetSingleTexelAlignment | - | n | n | n | n | n | n | n | n |
| subsampledCoarseReconstructionEarlyAccess | - | - | Y | Y | Y | Y | Y | Y | Y |
| subsampledLoads | - | - | n | n | n | n | n | n | n |
| supportedOperations | 6 flags | 8 flags | 8 flags | 8 flags | 8 flags | 8 flags | 8 flags | 8 flags | 8 flags |
| supportedQueues | - | - | - | - | - | - | - | - | graphics,compute,transfer |
| supportedStages | fragment,compute | fragment,compute | fragment,compute | fragment,compute | 8 flags | 8 flags | 8 flags | 8 flags | 8 flags |
| supportsNonZeroFirstInstance | - | - | n | n | - | n | n | n | n |
| texelRate | - | 0 | 4 | 8 | 8 | 8 | 8 | 8 | 8 |
| timestampComputeAndGraphics | n | Y | Y | Y | Y | Y | Y | Y | Y |
| transformFeedbackPreservesTriangleFanProvokingVertex | - | n | n | n | n | n | n | n | n |
| triStripVertexOrderIndependentOfProvokingVertex | - | - | - | - | - | - | - | n | n |
| uniformTexelBufferOffsetAlignmentBytes | - | 4 | 4 | 4 | 4 | 4 | 4 | 4 | 4 |
| uniformTexelBufferOffsetSingleTexelAlignment | - | Y | Y | Y | Y | Y | Y | Y | Y |
| viewportBoundsRange | [-32766,32765] | [-32768,32767] | [-32768,32767] | [-32768,32767] | [-32768,32767] | [-32768,32767] | [-32768,32767] | [-32768,32767] | [-32768,32767] |

## 7. Per-arch headline differences

- **v6 (G72, r26, Vulkan 1.1)**: oldest driver. No dualSrcBlend, logicOp, wideLines, depthClamp, multiDrawIndirect, shaderInt64, timestamps or Vulkan 1.2+ structs; 56 extensions. Still exposes geometryShader, tessellationShader, transformFeedback, ETC2, ASTC LDR and the ASTC HDR bit.
- **v7 (G52, r51)**: gains dualSrcBlend, logicOp, wideLines, shaderInt64 and 64-bit image/buffer atomics, dynamic rendering, synchronization2, subgroup size control, custom border colors. Still no depthClamp, multiDrawIndirect, drawIndirectCount, fragment density map; descriptorIndexing reported false.
- **v9 (G77, r54)**: adds depthClamp, descriptorIndexing, fragment density map, depthClipEnable, nullDescriptor, a larger extension set (133). Still no multiDrawIndirect/drawIndirectCount (JM).
- **v10 (G610, r54, first CSF)**: adds multiDrawIndirect, drawIndirectCount, inheritedQueries, nullDescriptor, descriptor buffer, graphics pipeline library, host image copy, mutable descriptor type, external memory host. robustImageAccess2 and robustBufferAccess2 are false even though `VK_EXT_robustness2` is present. No conservative rasterization, fragment shading rate or ray query. No protected memory.
- **v11 (G615, r44)**: first with conservative rasterization, fragment shading rate, ray query/acceleration structure and protected memory. Old driver, so no dualSrcBlend/logicOp/wideLines, no descriptor buffer/robustness2/host image copy.
- **v12 (G720, r49)**: cooperative matrix, render pass striped, ray tracing position fetch, dualSrcBlend/logicOp/wideLines back.
- **v13 (G925, r49)**: v12 plus opacity micromap and presentId/swapchain_maintenance1.
- **v14 (G1-Ultra, r54)**: multiViewport (maxViewports 16), robustness2 (BA2, RIA2, nullDescriptor), descriptor buffer, host image copy, conditional rendering, fragment shader barycentric, ray tracing maintenance1, shader viewport index/layer, maxMultiviewViewCount 16, timestampPeriod about 1 ns (older arches 76.9 ns, v7 41.7 ns, v6 none).
- **v15 (G2-Ultra-NX, r56, Vulkan 1.4)**: ray tracing pipeline, geometryStreams (4 XFB streams, transformFeedbackDraw), shader clock, workgroup memory explicit layout, shader module identifier, ARM tensor/data-graph/performance-counters-by-region, 10 memory types.

## 8. Native vs PanVK-emulated per arch

The PanVK column is the patched Mesa tree `/var/tmp/panvk/mesa-115g-final` (`src/panfrost/vulkan/panvk_vX_physical_device.c`, read-only). ARM columns are the gpuinfo values from section 3 (`Y` exposed, `n` reported false, `-` not reported). The `worklogs/g615-dxvk/device-runs/2026-10-29-*` folders hold PanProbe pass/fail results, not advertised bit-set dumps, so PanVK advertisement is taken from source.

| Feature | PanVK source | v6 | v7 | v9 | v10 | v11 | v12 | v13 | v14 | v15 |
|---|---|---|---|---|---|---|---|---|---|---|
| geometryShader | true (emulated, all arches) | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| tessellationShader | true (emulated; debug-gated on v6/v7) | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| transformFeedback | true (emulated) | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| geometryStreams | true unless NO_GS | n | n | n | n | n | n | n | n | Y |
| fillModeNonSolid | true (emulated) | n | n | n | n | n | n | n | n | n |
| vertexPipelineStoresAndAtomics | drirc opt-in (default on v9+) | n | n | n | n | n | n | n | n | n |
| depthBounds | perf-flag gated | n | n | n | n | n | n | n | n | n |
| shaderClipDistance / CullDistance | true (emulated) | n/n | n/n | n/n | n/n | n/n | n/n | n/n | n/n | n/n |
| textureCompressionBC | true when BC emulation enabled | n | n | n | n | n | n | n | n | n |
| robustImageAccess2 | v9+; v6/v7 via env/debug | - | - | n | n | - | - | - | Y | Y |
| robustBufferAccess2 | true | - | - | n | n | - | - | - | Y | Y |
| nullDescriptor | true (emulated on v6/v7) | - | - | Y | Y | - | - | - | Y | Y |
| pipelineStatisticsQuery | true (GPU-counted on JM) | n | n | n | n | n | n | n | n | n |
| multiDrawIndirect | true (emulated on JM) | n | n | n | Y | Y | Y | Y | Y | Y |
| drawIndirectCount | v9+ | - | n | n | Y | Y | Y | Y | Y | Y |
| multiViewport | true (v6/v7 via GS) | n | n | n | n | n | n | n | Y | Y |
| sparseBinding | v10+ non-kbase only | n | n | n | n | n | n | n | n | n |
| conservativeRasterization (ext) | v11+ | - | - | - | - | Y | Y | Y | Y | Y |
| alphaToOne | v10+ | n | n | n | n | n | n | n | n | n |
| shaderFloat64 | false | n | n | n | n | n | n | n | n | n |
| logicOp / dualSrcBlend / wideLines | true | n/n/n | Y/Y/Y | Y/Y/Y | Y/Y/Y | n/n/n | Y/Y/Y | Y/Y/Y | Y/Y/Y | Y/Y/Y |
| depthClamp | true | n | n | Y | Y | Y | Y | Y | Y | Y |
| shaderInt64 | true | n | Y | Y | Y | Y | Y | Y | Y | Y |
| extendedDynamicState3 true-count | PanVK exposes about 12 | 0 | 4 | 6 | 6 | 0 | 7 | 7 | 7 | 10 |

### 8.1 Verdicts

Categories: **HW-native likely** (ARM exposes it and a plausible fixed-function path exists), **ARM also emulates** (ARM exposes it, but the evidence says ARM implements it in shaders/compute, so exposure is no proof of HW), **ARM not exposed** (ARM reports false or omits the structure, so PanVK goes beyond ARM, or the ARM driver predates it), **unknown**.

- **geometryShader, tessellationShader**: *ARM also emulates, all arches.* ARM reports both true on every arch from v6 to v15, yet `shaderTessellationAndGeometryPointSize`, `multiviewGeometryShader`, `multiviewTessellationShader` and `primitiveTopologyPatchListRestart` are false. Mali Bifrost/Valhall have no geometry or tessellation fixed-function stage in the tiler pipeline; ARM runs both as extra shader/compute jobs (inference from the architecture, not from the report). The report proves ARM considers them worth exposing even on v6, which lowers conformance risk, but it does not prove HW.
- **transformFeedback (+ geometryStreams)**: *ARM also emulates; streams only on v15.* ARM exposes transformFeedback on all arches with `maxTransformFeedbackStreams=1`. `geometryStreams`, `transformFeedbackDraw`, `transformFeedbackRasterizationStreamSelect` and `transformFeedbackStreamsLinesTriangles` are true only on v15. `transformFeedbackPreservesProvokingVertex` is false everywhere. Mali has no stream-out HW, so capture is shader-written; ARM's v6-v14 limits (one stream, no draw-indirect byte count) are a safe ceiling for PanVK.
- **fillModeNonSolid**: *ARM not exposed (all arches).* False on all nine arches; `polygonModePointSize` false, `extendedDynamicState3PolygonMode` false. A native HW path is unlikely. PanVK emulation is beyond ARM.
- **vertexPipelineStoresAndAtomics**: *ARM not exposed (all arches).* False everywhere, so ARM never offers vertex-stage stores. PanVK's gpu_prerast / per-vertex-job path is beyond ARM. Unknown whether ARM refrains because of the Valhall IDVS speculation hazard mentioned in the PanVK source rather than HW inability.
- **depthBounds**: *ARM not exposed (all arches).* False everywhere; PanVK gates it behind a perf flag.
- **shaderClipDistance / shaderCullDistance**: *ARM not exposed (all arches).* False everywhere (ARM reports only `pointClippingBehavior`). PanVK emulation is PanVK-only.
- **textureCompressionBC**: *ARM not exposed (all arches).* Feature false and 0 of 18 BC formats exposed on every arch. The texture unit has no BC decode; emulation (compute decode or recompression) is the only route and has no native counterpart. Strongest evidence in the dataset.
- **robustImageAccess2 / robustBufferAccess2**: *v14/v15: native or ARM shader lowering (unknown); v9/v10: ARM cannot; others: ARM not exposed.* v14/v15 (r54/r56) report BA2, RIA2 and nullDescriptor true. v9 and v10 (both r54, `VK_EXT_robustness2` present) report BA2 false, RIA2 false, nullDescriptor true, so ARM does not provide RIA2 on v9 or v10: PanVK's v9 emulation, v10 GPU alpha fix-up and software bounds checks exceed ARM. v6/v7/v11/v12/v13 do not expose the extension at all (v11-v13 drivers are older). Whether v14's RIA2 is HW or ARM compiler lowering is unknown.
- **nullDescriptor**: *HW-native likely on v9/v10/v14/v15; unknown elsewhere.* True on v9, v10, v14, v15 where the robustness2 structure is reported; not reported on other drivers. PanVK emulates it with null_descs on v6/v7.
- **pipelineStatisticsQuery**: *ARM not exposed (all arches).* False everywhere; PanVK's GPU-counted queries on JM go beyond ARM. Related `VK_EXT_primitives_generated_query` is exposed by ARM on v7 and v9-v15 (not v6), matching PanVK v9+.
- **multiDrawIndirect, drawIndirectCount**: *JM (v6/v7/v9): ARM not exposed; CSF (v10+): HW-native likely.* ARM reports multiDrawIndirect false on v6, v7, v9 and true on v10-v15; drawIndirectCount true from v10 and false on v7/v9 (`VK_KHR_draw_indirect_count` absent v6-v9). Consistent with CSF having a native indirect path and JM needing emulation, which is what PanVK does.
- **multiViewport**: *v14/v15: HW-native likely; v6-v13: ARM not exposed.* maxViewports is 16 only on v14/v15 (r54/r56); v10 shares r54 with v14 yet reports 1 viewport, so this is arch-specific, not driver vintage. PanVK's multiViewport on v6-v13 is emulation beyond ARM (via the GS path on v6/v7).
- **sparse binding/residency**: *ARM not exposed (all arches).* All sparse features false and no sparse-binding queue on any arch. PanVK's v10+ sparse (non-kbase) goes beyond ARM; kbase sparse is disabled in source anyway.
- **conservativeRasterization**: *HW-native likely on v11+; PanVK matches.* ARM exposes `VK_EXT_conservative_rasterization` on v11-v15 (primitiveOverestimationSize 0.00195 = 1/512, no underestimation, no post-depth coverage, degenerate lines not rasterized); v10 does not. PanVK gates it at v11, so both agree.
- **alphaToOne**: *ARM not exposed (all arches).* False everywhere. PanVK's v10+ advertisement exceeds ARM.
- **logicOp / dualSrcBlend / wideLines / depthClamp / shaderInt64**: *HW-native likely where ARM is Y; v6 and v11 gaps are driver age or unknown.* logicOp, dualSrcBlend and wideLines are true on v7, v9, v10, v12-v15 but false on v6 (r26) and v11 (r44; `maxFragmentDualSrcAttachments`=0, lineWidthRange [1,1]). depthClamp is false on v6/v7; shaderInt64 false on v6. Because v10 and v12 expose them the silicon almost certainly supports them. PanVK advertising them on v6 and v11 is beyond ARM's report; PanProbe/CTS decide whether they hold.
- **shaderFloat64**: *ARM not exposed; PanVK also false.* False everywhere.
- **extendedDynamicState3 subset**: *ARM exposes a subset (v7, v9, v10, v12-v15); PanVK exposes more.* ARM never exposes PolygonMode, ColorBlendEnable/Equation/WriteMask, RasterizationSamples or LogicOpEnable (count row in the table above); PanVK exposes several natively. Driver surface only, no HW lesson.

### 8.2 Top opportunities where PanVK emulates and ARM exposes

Ranked by usefulness and strength of evidence (ARM exposure is necessary, not sufficient):

1. multiDrawIndirect and drawIndirectCount on v10+ (CSF): ARM true from v10, false on JM. Supports a native CSF path; confirm PanVK's CSF path does not emulate.
2. multiViewport / maxViewports 16 on v14/v15 only: arch-specific, HW-native likely; do not expect it on v10-v13.
3. robustImageAccess2 + robustBufferAccess2 on v14/v15: reference for a native-style path there; ARM cannot on v9/v10, so PanVK's v9/v10 emulation is already ahead.
4. nullDescriptor on v9/v10/v14/v15.
5. transformFeedback on all arches, shaped by ARM's limits (1 stream, no transformFeedbackDraw); geometryStreams only on v15. ARM also emulates.
6. geometryShader on all arches, down to v6. Parity with ARM, not HW (no fixed-function GS).
7. tessellationShader on all arches. Same caution; ARM lacks primitiveTopologyPatchListRestart and tess point size.
8. conservativeRasterization on v11+: already matched.
9. logicOp, dualSrcBlend, wideLines on v7/v9/v10/v12+: ARM exposes natively-looking; v6 and v11 gaps in ARM are unexplained.
10. drawIndirectFirstInstance and fullDrawIndexUint32 are true on every ARM arch (v6-v15), so no emulation needed there.

Nothing in this dataset shows ARM exposing BC, fillModeNonSolid, depthBounds, shaderClipDistance/CullDistance, vertexPipelineStoresAndAtomics, pipelineStatisticsQuery, sparse, alphaToOne or shaderFloat64 on any Mali. Those PanVK emulations have no ARM-native proof.

### 8.3 ARM exposes, PanVK source tables do not (extensions)

| Extension (no entry in PanVK physical-device tables) | v6 | v7 | v9 | v10 | v11 | v12 | v13 | v14 | v15 |
|---|---|---|---|---|---|---|---|---|---|
| VK_ANDROID_external_format_resolve | - | - | Y | Y | - | Y | Y | Y | Y |
| VK_ARM_render_pass_striped | - | - | - | - | - | Y | Y | Y | Y |
| VK_EXT_descriptor_buffer | - | - | - | Y | - | - | - | Y | Y |
| VK_EXT_device_fault | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_external_memory_host | - | - | Y | Y | - | - | - | Y | Y |
| VK_EXT_fragment_density_map | - | - | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_fragment_density_map2 | - | - | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_frame_boundary | - | Y | Y | Y | - | Y | Y | Y | Y |
| VK_EXT_image_compression_control_swapchain | - | - | - | Y | Y | Y | Y | Y | Y |
| VK_EXT_opacity_micromap | - | - | - | - | - | - | Y | Y | Y |
| VK_EXT_pipeline_library_group_handles | - | - | - | - | - | - | - | - | Y |
| VK_EXT_pipeline_protected_access | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_shader_viewport_index_layer | - | - | - | - | - | - | - | Y | Y |
| VK_EXT_subpass_merge_feedback | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_acceleration_structure | - | - | - | - | Y | Y | Y | Y | Y |
| VK_KHR_deferred_host_operations | - | - | - | Y | Y | Y | Y | Y | Y |
| VK_KHR_fragment_shader_barycentric | - | - | - | - | - | - | - | Y | Y |
| VK_KHR_fragment_shading_rate | - | - | - | - | Y | Y | Y | Y | Y |
| VK_KHR_ray_query | - | - | - | - | Y | Y | Y | Y | Y |
| VK_KHR_ray_tracing_maintenance1 | - | - | - | - | - | - | - | Y | Y |
| VK_KHR_ray_tracing_pipeline | - | - | - | - | - | - | - | - | Y |
| VK_KHR_ray_tracing_position_fetch | - | - | - | - | - | Y | Y | Y | Y |
| VK_KHR_shared_presentable_image | Y | Y | Y | Y | Y | Y | Y | Y | Y |

Most relevant: `VK_KHR_fragment_shading_rate` (v11+), `VK_EXT_fragment_density_map` and `_map2` (v9+), ray query/acceleration structure (v11+), `VK_EXT_descriptor_buffer` (v10/v14/v15), `VK_EXT_shader_viewport_index_layer` (v14/v15), `VK_EXT_external_memory_host` (v10/v14/v15). The check is a text match against the two PanVK source files; an extension enabled through common Mesa code could be missed.

## Appendix A: all device extensions (spec version; presence per arch)

| Extension | spec | v6 | v7 | v9 | v10 | v11 | v12 | v13 | v14 | v15 |
|---|---|---|---|---|---|---|---|---|---|---|
| VK_ANDROID_external_format_resolve | 1 | - | - | Y | Y | - | Y | Y | Y | Y |
| VK_ANDROID_external_memory_android_hardware_buffer | 3/5 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_ARM_rasterization_order_attachment_access | 1 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_ARM_render_pass_striped | 1 | - | - | - | - | - | Y | Y | Y | Y |
| VK_ARM_scheduling_controls | 1/2 | - | - | - | Y | - | Y | Y | Y | Y |
| VK_ARM_shader_core_builtins | 2 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_ARM_shader_core_properties | 1 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_4444_formats | 1 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_astc_decode_mode | 1 | - | - | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_border_color_swizzle | 1 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_calibrated_timestamps | 2 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_conditional_rendering | 2 | - | - | - | - | - | - | - | Y | Y |
| VK_EXT_conservative_rasterization | 1 | - | - | - | - | Y | Y | Y | Y | Y |
| VK_EXT_custom_border_color | 12 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_debug_marker | 4 | - | Y | Y | Y | - | Y | Y | Y | Y |
| VK_EXT_depth_clamp_zero_one | 1 | - | - | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_depth_clip_enable | 1 | - | - | Y | Y | - | Y | Y | Y | Y |
| VK_EXT_descriptor_buffer | 1 | - | - | - | Y | - | - | - | Y | Y |
| VK_EXT_descriptor_indexing | 2 | - | - | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_device_address_binding_report | 1 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_device_fault | 2 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_device_memory_report | 2 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_dynamic_rendering_unused_attachments | 1 | - | Y | Y | Y | - | Y | Y | Y | Y |
| VK_EXT_extended_dynamic_state | 1 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_extended_dynamic_state2 | 1 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_extended_dynamic_state3 | 2 | - | Y | Y | Y | - | Y | Y | Y | Y |
| VK_EXT_external_memory_acquire_unmodified | 1 | - | Y | Y | Y | - | Y | Y | Y | Y |
| VK_EXT_external_memory_dma_buf | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_external_memory_host | 1 | - | - | Y | Y | - | - | - | Y | Y |
| VK_EXT_fragment_density_map | 2/3 | - | - | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_fragment_density_map2 | 1 | - | - | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_frame_boundary | 1 | - | Y | Y | Y | - | Y | Y | Y | Y |
| VK_EXT_global_priority | 2 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_global_priority_query | 1 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_graphics_pipeline_library | 1 | - | - | Y | Y | - | - | - | Y | Y |
| VK_EXT_hdr_metadata | 3 | - | - | - | - | - | Y | Y | - | Y |
| VK_EXT_host_image_copy | 1 | - | Y | Y | Y | - | - | - | Y | Y |
| VK_EXT_host_query_reset | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_image_2d_view_of_3d | 1 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_image_compression_control | 1 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_image_compression_control_swapchain | 1 | - | - | - | Y | Y | Y | Y | Y | Y |
| VK_EXT_image_drm_format_modifier | 1/2 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_image_robustness | 1 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_image_sliced_view_of_3d | 1 | - | Y | Y | Y | - | Y | Y | Y | Y |
| VK_EXT_image_view_min_lod | 1 | - | - | - | Y | - | - | - | Y | Y |
| VK_EXT_index_type_uint8 | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_inline_uniform_block | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_legacy_dithering | 1/2 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_line_rasterization | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_load_store_op_none | 1 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_multisampled_render_to_single_sampled | 1 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_mutable_descriptor_type | 1 | - | - | Y | Y | - | - | - | Y | Y |
| VK_EXT_non_seamless_cube_map | 1 | - | - | Y | Y | - | - | - | Y | Y |
| VK_EXT_opacity_micromap | 2 | - | - | - | - | - | - | Y | Y | Y |
| VK_EXT_pipeline_creation_cache_control | 3 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_pipeline_creation_feedback | 1 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_pipeline_library_group_handles | 1 | - | - | - | - | - | - | - | - | Y |
| VK_EXT_pipeline_protected_access | 1 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_pipeline_robustness | 1 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_primitive_topology_list_restart | 1 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_primitives_generated_query | 1 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_private_data | 1 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_provoking_vertex | 1 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_queue_family_foreign | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_rasterization_order_attachment_access | 1 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_rgba10x6_formats | 1 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_robustness2 | 1 | - | - | Y | Y | - | - | - | Y | Y |
| VK_EXT_sampler_filter_minmax | 2 | - | - | - | Y | Y | Y | Y | Y | Y |
| VK_EXT_scalar_block_layout | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_separate_stencil_usage | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_shader_demote_to_helper_invocation | 1 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_shader_image_atomic_int64 | 1 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_shader_module_identifier | 1 | - | - | - | - | - | - | - | - | Y |
| VK_EXT_shader_subgroup_ballot | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_shader_subgroup_vote | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_shader_tile_image | 1 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_shader_viewport_index_layer | 1 | - | - | - | - | - | - | - | Y | Y |
| VK_EXT_subgroup_size_control | 2 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_subpass_merge_feedback | 2 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_swapchain_maintenance1 | 1 | - | Y | - | Y | Y | Y | Y | Y | Y |
| VK_EXT_texel_buffer_alignment | 1 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_texture_compression_astc_hdr | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_tooling_info | 1 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_transform_feedback | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_EXT_vertex_input_dynamic_state | 2 | - | Y | Y | Y | - | Y | Y | Y | Y |
| VK_GOOGLE_decorate_string | 1 | - | Y | Y | Y | - | Y | Y | Y | Y |
| VK_GOOGLE_display_timing | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_GOOGLE_user_type | 1 | - | - | Y | Y | - | Y | Y | Y | Y |
| VK_KHR_16bit_storage | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_8bit_storage | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_acceleration_structure | 13 | - | - | - | - | Y | Y | Y | Y | Y |
| VK_KHR_bind_memory2 | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_buffer_device_address | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_calibrated_timestamps | 1 | - | - | Y | Y | - | Y | Y | Y | Y |
| VK_KHR_cooperative_matrix | 2 | - | - | - | - | - | Y | Y | Y | Y |
| VK_KHR_copy_commands2 | 1 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_create_renderpass2 | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_dedicated_allocation | 3 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_deferred_host_operations | 4 | - | - | - | Y | Y | Y | Y | Y | Y |
| VK_KHR_depth_stencil_resolve | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_descriptor_update_template | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_device_group | 4 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_draw_indirect_count | 1 | - | - | - | Y | Y | Y | Y | Y | Y |
| VK_KHR_driver_properties | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_dynamic_rendering | 1 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_external_fence | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_external_fence_fd | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_external_memory | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_external_memory_fd | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_external_semaphore | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_external_semaphore_fd | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_format_feature_flags2 | 2 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_fragment_shader_barycentric | 1 | - | - | - | - | - | - | - | Y | Y |
| VK_KHR_fragment_shading_rate | 2 | - | - | - | - | Y | Y | Y | Y | Y |
| VK_KHR_get_memory_requirements2 | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_global_priority | 1 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_image_format_list | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_imageless_framebuffer | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_incremental_present | 2 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_maintenance1 | 2 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_maintenance2 | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_maintenance3 | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_maintenance4 | 2 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_maintenance5 | 1 | - | - | Y | Y | - | Y | Y | Y | Y |
| VK_KHR_map_memory2 | 1 | - | - | Y | Y | - | Y | Y | Y | Y |
| VK_KHR_multiview | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_pipeline_executable_properties | 1 | - | - | Y | Y | - | - | - | Y | Y |
| VK_KHR_pipeline_library | 1 | - | - | Y | Y | - | - | - | Y | Y |
| VK_KHR_present_id | 1 | - | - | - | - | - | - | Y | - | Y |
| VK_KHR_push_descriptor | 2 | - | - | Y | Y | - | Y | Y | Y | Y |
| VK_KHR_ray_query | 1 | - | - | - | - | Y | Y | Y | Y | Y |
| VK_KHR_ray_tracing_maintenance1 | 1 | - | - | - | - | - | - | - | Y | Y |
| VK_KHR_ray_tracing_pipeline | 1 | - | - | - | - | - | - | - | - | Y |
| VK_KHR_ray_tracing_position_fetch | 1 | - | - | - | - | - | Y | Y | Y | Y |
| VK_KHR_relaxed_block_layout | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_sampler_mirror_clamp_to_edge | 3 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_sampler_ycbcr_conversion | 14 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_separate_depth_stencil_layouts | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_shader_atomic_int64 | 1 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_shader_clock | 1 | - | - | - | - | - | - | - | - | Y |
| VK_KHR_shader_draw_parameters | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_shader_float16_int8 | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_shader_float_controls | 4 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_shader_integer_dot_product | 1 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_shader_non_semantic_info | 1 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_shader_subgroup_extended_types | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_shader_subgroup_uniform_control_flow | 1 | - | Y | Y | Y | - | Y | Y | Y | Y |
| VK_KHR_shader_terminate_invocation | 1 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_shared_presentable_image | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_spirv_1_4 | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_storage_buffer_storage_class | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_swapchain | 70 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_swapchain_mutable_format | 1 | - | - | - | Y | Y | Y | Y | Y | Y |
| VK_KHR_synchronization2 | 1 | - | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_timeline_semaphore | 2 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_uniform_buffer_standard_layout | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_variable_pointers | 1 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_vertex_attribute_divisor | 1 | - | - | Y | Y | - | Y | Y | Y | Y |
| VK_KHR_vulkan_memory_model | 3 | Y | Y | Y | Y | Y | Y | Y | Y | Y |
| VK_KHR_workgroup_memory_explicit_layout | 1 | - | - | - | - | - | - | - | - | Y |
| VK_KHR_zero_initialize_workgroup_memory | 1 | - | Y | Y | Y | Y | Y | Y | Y | Y |
