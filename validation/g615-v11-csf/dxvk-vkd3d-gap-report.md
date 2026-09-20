# G615 DXVK/vkd3d Gap Report

Generated from the direct-ICD capture and exact tagged official profiles. `UNKNOWN` is not failure proof or support proof.

## Provenance

- Device: `Mali-G615 MC6` / `0xb8a31030`
- Capture ICD SHA-256: `576e37de9a3b60dda9a972a6f91c3a50e09238bd31a9888e04215c7b554c803a`
- Pinned Mesa: `5a07217f034b3e50d8c7c7794f97a2df1742613b`
- Audited Mesa origin/main: `e1f3f372c4a661cd0e71a0cdafc4d469d56ecf35`
- Post-pin coherent requirement backports: none

## dxvk-1.10.3 source-derived

### D3D9: FAIL

Source: `v1.10.3` `src/d3d9/d3d9_device.cpp:3887-3950`

| Requirement | Current | Required | Upstream PanVK | Implementation | Action |
|---|---:|---:|---|---|---|
| `geometryShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `fillModeNonSolid` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `shaderClipDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `shaderCullDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `textureCompressionBC` | `False` | `True` | Hardware-dependent; G615 mask is zero | UNSUPPORTED_NATIVE | Keep false unless transparent BC emulation is complete |
| `multiViewport` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |

### D3D10_10_1: FAIL

Source: `v1.10.3` `src/d3d11/d3d11_device.cpp:1927-1992`

| Requirement | Current | Required | Upstream PanVK | Implementation | Action |
|---|---:|---:|---|---|---|
| `geometryShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `fillModeNonSolid` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `shaderClipDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `shaderCullDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `textureCompressionBC` | `False` | `True` | Hardware-dependent; G615 mask is zero | UNSUPPORTED_NATIVE | Keep false unless transparent BC emulation is complete |
| `multiViewport` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `geometryShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `fillModeNonSolid` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `shaderClipDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `shaderCullDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `textureCompressionBC` | `False` | `True` | Hardware-dependent; G615 mask is zero | UNSUPPORTED_NATIVE | Keep false unless transparent BC emulation is complete |
| `multiViewport` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `transformFeedback` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `geometryStreams` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `VK_EXT_transform_feedback` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |

### D3D11_FL11_0: FAIL

Source: `v1.10.3` `src/d3d11/d3d11_device.cpp:1994-2004`

| Requirement | Current | Required | Upstream PanVK | Implementation | Action |
|---|---:|---:|---|---|---|
| `geometryShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `fillModeNonSolid` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `shaderClipDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `shaderCullDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `textureCompressionBC` | `False` | `True` | Hardware-dependent; G615 mask is zero | UNSUPPORTED_NATIVE | Keep false unless transparent BC emulation is complete |
| `multiViewport` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `geometryShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `fillModeNonSolid` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `shaderClipDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `shaderCullDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `textureCompressionBC` | `False` | `True` | Hardware-dependent; G615 mask is zero | UNSUPPORTED_NATIVE | Keep false unless transparent BC emulation is complete |
| `multiViewport` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `transformFeedback` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `geometryStreams` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `tessellationShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |

## vkd3d-proton-2.0 source-derived

Source: `v2.0` `README.md:17-24`

Hard requirements: **PASS (reported values only; P5 workload proof pending)**

| Requirement | Current | Required |
|---|---:|---:|
| `apiVersion >= 1.1` | `True` | `True` |
| `VK_EXT_descriptor_indexing` | `True` | `True` |
| `VK_KHR_timeline_semaphore` | `True` | `True` |
| `required descriptor-indexing features` | `True` | `True` |
| `required UpdateAfterBind limits >= 1000000` | `True` | `True` |

## dxvk-2.7.1

Source: `v2.7.1` `https://raw.githubusercontent.com/doitsujin/dxvk/v2.7.1/VP_DXVK_requirements.json` (`1e219227adeba497e387fbad72847fef4d064667ede946f720d8e8a67bea6edf`)

### VP_DXVK_d3d9_baseline: FAIL

| Requirement | Current | Required | Upstream PanVK | Implementation | Action |
|---|---:|---:|---|---|---|
| `features.VkPhysicalDeviceFeatures.geometryShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.fillModeNonSolid` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderClipDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderCullDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.textureCompressionBC` | `False` | `True` | Hardware-dependent; G615 mask is zero | UNSUPPORTED_NATIVE | Keep false unless transparent BC emulation is complete |

### VP_DXVK_d3d9_optimal: FAIL

| Requirement | Current | Required | Upstream PanVK | Implementation | Action |
|---|---:|---:|---|---|---|
| `features.VkPhysicalDeviceFeatures.geometryShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.fillModeNonSolid` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderClipDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderCullDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.textureCompressionBC` | `False` | `True` | Hardware-dependent; G615 mask is zero | UNSUPPORTED_NATIVE | Keep false unless transparent BC emulation is complete |

### VP_DXVK_d3d10_level_10_1_baseline: FAIL

| Requirement | Current | Required | Upstream PanVK | Implementation | Action |
|---|---:|---:|---|---|---|
| `extensions.VK_EXT_transform_feedback` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFeatures.fillModeNonSolid` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.geometryShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.multiViewport` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderClipDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderCullDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.textureCompressionBC` | `False` | `True` | Hardware-dependent; G615 mask is zero | UNSUPPORTED_NATIVE | Keep false unless transparent BC emulation is complete |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.transformFeedback` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.geometryStreams` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |

### VP_DXVK_d3d11_level_11_0_baseline: FAIL

| Requirement | Current | Required | Upstream PanVK | Implementation | Action |
|---|---:|---:|---|---|---|
| `extensions.VK_EXT_transform_feedback` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFeatures.fillModeNonSolid` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.geometryShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.multiViewport` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderClipDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderCullDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.textureCompressionBC` | `False` | `True` | Hardware-dependent; G615 mask is zero | UNSUPPORTED_NATIVE | Keep false unless transparent BC emulation is complete |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.transformFeedback` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.geometryStreams` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.tessellationShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |

### VP_DXVK_d3d11_level_11_1_baseline: FAIL

| Requirement | Current | Required | Upstream PanVK | Implementation | Action |
|---|---:|---:|---|---|---|
| `extensions.VK_EXT_transform_feedback` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFeatures.fillModeNonSolid` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.geometryShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.multiViewport` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderClipDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderCullDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.textureCompressionBC` | `False` | `True` | Hardware-dependent; G615 mask is zero | UNSUPPORTED_NATIVE | Keep false unless transparent BC emulation is complete |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.transformFeedback` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.geometryStreams` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.tessellationShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.vertexPipelineStoresAndAtomics` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |

### VP_DXVK_d3d11_level_11_1_optimal: FAIL

| Requirement | Current | Required | Upstream PanVK | Implementation | Action |
|---|---:|---:|---|---|---|
| `extensions.VK_EXT_transform_feedback` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFeatures.fillModeNonSolid` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.geometryShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.multiViewport` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderClipDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderCullDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.textureCompressionBC` | `False` | `True` | Hardware-dependent; G615 mask is zero | UNSUPPORTED_NATIVE | Keep false unless transparent BC emulation is complete |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.transformFeedback` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.geometryStreams` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.tessellationShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.vertexPipelineStoresAndAtomics` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |

### VP_DXVK_d3d11_level_12_0_optimal: FAIL

| Requirement | Current | Required | Upstream PanVK | Implementation | Action |
|---|---:|---:|---|---|---|
| `extensions.VK_EXT_transform_feedback` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFeatures.fillModeNonSolid` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.geometryShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.multiViewport` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderClipDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderCullDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.textureCompressionBC` | `False` | `True` | Hardware-dependent; G615 mask is zero | UNSUPPORTED_NATIVE | Keep false unless transparent BC emulation is complete |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.transformFeedback` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.geometryStreams` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.tessellationShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.vertexPipelineStoresAndAtomics` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.shaderResourceResidency` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.shaderResourceMinLod` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.sparseBinding` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyBuffer` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyAliased` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyImage2D` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyStandard2DBlockShape` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyAlignedMipSize` | `None` | `False` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyNonResidentStrict` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |

## dxvk-3.1.1

Source: `v3.1.1` `https://raw.githubusercontent.com/doitsujin/dxvk/v3.1.1/VP_DXVK_requirements.json` (`d490930920a24fb9ac4a0585c68042929acd7e700e04d0a032c79d9fddf1c462`)

### VP_DXVK_d3d9_baseline: FAIL

| Requirement | Current | Required | Upstream PanVK | Implementation | Action |
|---|---:|---:|---|---|---|
| `features.VkPhysicalDeviceFeatures.geometryShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.fillModeNonSolid` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderClipDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderCullDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.textureCompressionBC` | `False` | `True` | Hardware-dependent; G615 mask is zero | UNSUPPORTED_NATIVE | Keep false unless transparent BC emulation is complete |

### VP_DXVK_d3d9_optimal: FAIL

| Requirement | Current | Required | Upstream PanVK | Implementation | Action |
|---|---:|---:|---|---|---|
| `features.VkPhysicalDeviceFeatures.geometryShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.fillModeNonSolid` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderClipDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderCullDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.textureCompressionBC` | `False` | `True` | Hardware-dependent; G615 mask is zero | UNSUPPORTED_NATIVE | Keep false unless transparent BC emulation is complete |

### VP_DXVK_d3d10_level_10_1_baseline: FAIL

| Requirement | Current | Required | Upstream PanVK | Implementation | Action |
|---|---:|---:|---|---|---|
| `extensions.VK_EXT_transform_feedback` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFeatures.fillModeNonSolid` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.geometryShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.multiViewport` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderClipDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderCullDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.textureCompressionBC` | `False` | `True` | Hardware-dependent; G615 mask is zero | UNSUPPORTED_NATIVE | Keep false unless transparent BC emulation is complete |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.transformFeedback` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.geometryStreams` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |

### VP_DXVK_d3d11_level_11_0_baseline: FAIL

| Requirement | Current | Required | Upstream PanVK | Implementation | Action |
|---|---:|---:|---|---|---|
| `extensions.VK_EXT_transform_feedback` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFeatures.fillModeNonSolid` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.geometryShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.multiViewport` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderClipDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderCullDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.textureCompressionBC` | `False` | `True` | Hardware-dependent; G615 mask is zero | UNSUPPORTED_NATIVE | Keep false unless transparent BC emulation is complete |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.transformFeedback` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.geometryStreams` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.tessellationShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |

### VP_DXVK_d3d11_level_11_1_baseline: FAIL

| Requirement | Current | Required | Upstream PanVK | Implementation | Action |
|---|---:|---:|---|---|---|
| `extensions.VK_EXT_transform_feedback` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFeatures.fillModeNonSolid` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.geometryShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.multiViewport` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderClipDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderCullDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.textureCompressionBC` | `False` | `True` | Hardware-dependent; G615 mask is zero | UNSUPPORTED_NATIVE | Keep false unless transparent BC emulation is complete |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.transformFeedback` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.geometryStreams` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.tessellationShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.vertexPipelineStoresAndAtomics` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |

### VP_DXVK_d3d11_level_11_1_optimal: FAIL

| Requirement | Current | Required | Upstream PanVK | Implementation | Action |
|---|---:|---:|---|---|---|
| `extensions.VK_EXT_transform_feedback` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFeatures.fillModeNonSolid` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.geometryShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.multiViewport` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderClipDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderCullDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.textureCompressionBC` | `False` | `True` | Hardware-dependent; G615 mask is zero | UNSUPPORTED_NATIVE | Keep false unless transparent BC emulation is complete |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.transformFeedback` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.geometryStreams` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.tessellationShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.vertexPipelineStoresAndAtomics` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |

### VP_DXVK_d3d11_level_12_0_optimal: FAIL

| Requirement | Current | Required | Upstream PanVK | Implementation | Action |
|---|---:|---:|---|---|---|
| `extensions.VK_EXT_transform_feedback` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFeatures.fillModeNonSolid` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.geometryShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.multiViewport` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderClipDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderCullDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.textureCompressionBC` | `False` | `True` | Hardware-dependent; G615 mask is zero | UNSUPPORTED_NATIVE | Keep false unless transparent BC emulation is complete |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.transformFeedback` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.geometryStreams` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.tessellationShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.vertexPipelineStoresAndAtomics` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.shaderResourceResidency` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.shaderResourceMinLod` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.sparseBinding` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyBuffer` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyAliased` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyImage2D` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyStandard2DBlockShape` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyAlignedMipSize` | `None` | `False` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyNonResidentStrict` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |

### VP_DXVK_d3d11_level_12_1_optimal: FAIL

| Requirement | Current | Required | Upstream PanVK | Implementation | Action |
|---|---:|---:|---|---|---|
| `extensions.VK_EXT_transform_feedback` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFeatures.fillModeNonSolid` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.geometryShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.multiViewport` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderClipDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderCullDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.textureCompressionBC` | `False` | `True` | Hardware-dependent; G615 mask is zero | UNSUPPORTED_NATIVE | Keep false unless transparent BC emulation is complete |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.transformFeedback` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.geometryStreams` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.tessellationShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.vertexPipelineStoresAndAtomics` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.shaderResourceResidency` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.shaderResourceMinLod` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.sparseBinding` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyBuffer` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyAliased` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyImage2D` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyStandard2DBlockShape` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyAlignedMipSize` | `None` | `False` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyNonResidentStrict` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_EXT_fragment_shader_interlock` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT.fragmentShaderPixelInterlock` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceConservativeRasterizationPropertiesEXT.degenerateTrianglesRasterized` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceConservativeRasterizationPropertiesEXT.fullyCoveredFragmentShaderInputVariable` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |

## vkd3d-2.14.1

Source: `v2.14.1` `https://raw.githubusercontent.com/HansKristian-Work/vkd3d-proton/v2.14.1/VP_D3D12_VKD3D_PROTON_profile.json` (`9ff2e08e82f03f214714591e0727a35c164639e71879c9721ca9e5b4bd0ddd17`)

### VP_D3D12_FL_11_0_baseline: FAIL

| Requirement | Current | Required | Upstream PanVK | Implementation | Action |
|---|---:|---:|---|---|---|
| `extensions.VK_EXT_transform_feedback` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRobustness2FeaturesEXT.robustImageAccess2` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.transformFeedback` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.geometryStreams` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.geometryShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.tessellationShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.fillModeNonSolid` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.multiViewport` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.textureCompressionBC` | `False` | `True` | Hardware-dependent; G615 mask is zero | UNSUPPORTED_NATIVE | Keep false unless transparent BC emulation is complete |
| `features.VkPhysicalDeviceFeatures.pipelineStatisticsQuery` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderClipDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderCullDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `properties.VkPhysicalDeviceTransformFeedbackPropertiesEXT.transformFeedbackQueries` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceProperties.limits.bufferImageGranularity` | `64` | `65536` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[0]` | `None` | `VK_SUBGROUP_FEATURE_BASIC_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |

### VP_D3D12_FL_11_1_baseline: FAIL

| Requirement | Current | Required | Upstream PanVK | Implementation | Action |
|---|---:|---:|---|---|---|
| `extensions.VK_EXT_transform_feedback` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRobustness2FeaturesEXT.robustImageAccess2` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.transformFeedback` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.geometryStreams` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.geometryShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.tessellationShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.fillModeNonSolid` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.multiViewport` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.textureCompressionBC` | `False` | `True` | Hardware-dependent; G615 mask is zero | UNSUPPORTED_NATIVE | Keep false unless transparent BC emulation is complete |
| `features.VkPhysicalDeviceFeatures.pipelineStatisticsQuery` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderClipDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderCullDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `properties.VkPhysicalDeviceTransformFeedbackPropertiesEXT.transformFeedbackQueries` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceProperties.limits.bufferImageGranularity` | `64` | `65536` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[0]` | `None` | `VK_SUBGROUP_FEATURE_BASIC_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFeatures.vertexPipelineStoresAndAtomics` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |

### VP_D3D12_FL_12_0_baseline: FAIL

| Requirement | Current | Required | Upstream PanVK | Implementation | Action |
|---|---:|---:|---|---|---|
| `extensions.VK_EXT_transform_feedback` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRobustness2FeaturesEXT.robustImageAccess2` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.transformFeedback` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.geometryStreams` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.geometryShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.tessellationShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.fillModeNonSolid` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.multiViewport` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.textureCompressionBC` | `False` | `True` | Hardware-dependent; G615 mask is zero | UNSUPPORTED_NATIVE | Keep false unless transparent BC emulation is complete |
| `features.VkPhysicalDeviceFeatures.pipelineStatisticsQuery` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderClipDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderCullDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `properties.VkPhysicalDeviceTransformFeedbackPropertiesEXT.transformFeedbackQueries` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceProperties.limits.bufferImageGranularity` | `64` | `65536` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyStandard2DBlockShape` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyAlignedMipSize` | `None` | `False` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyNonResidentStrict` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[0]` | `None` | `VK_SUBGROUP_FEATURE_BALLOT_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[1]` | `None` | `VK_SUBGROUP_FEATURE_BASIC_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[2]` | `None` | `VK_SUBGROUP_FEATURE_VOTE_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[3]` | `None` | `VK_SUBGROUP_FEATURE_SHUFFLE_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[4]` | `None` | `VK_SUBGROUP_FEATURE_QUAD_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[5]` | `None` | `VK_SUBGROUP_FEATURE_ARITHMETIC_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedStages[0]` | `None` | `VK_SHADER_STAGE_COMPUTE_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedStages[1]` | `None` | `VK_SHADER_STAGE_FRAGMENT_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFeatures.vertexPipelineStoresAndAtomics` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.sparseBinding` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyAliased` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyBuffer` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyImage2D` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.shaderResourceResidency` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.shaderResourceMinLod` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.queueFlags[0]` | `None` | `VK_QUEUE_SPARSE_BINDING_BIT` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.queueCount` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |

### VP_D3D12_FL_12_0_optimal: FAIL

| Requirement | Current | Required | Upstream PanVK | Implementation | Action |
|---|---:|---:|---|---|---|
| `extensions.VK_EXT_transform_feedback` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRobustness2FeaturesEXT.robustImageAccess2` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.transformFeedback` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.geometryStreams` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.geometryShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.tessellationShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.fillModeNonSolid` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.multiViewport` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.textureCompressionBC` | `False` | `True` | Hardware-dependent; G615 mask is zero | UNSUPPORTED_NATIVE | Keep false unless transparent BC emulation is complete |
| `features.VkPhysicalDeviceFeatures.pipelineStatisticsQuery` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderClipDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderCullDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `properties.VkPhysicalDeviceTransformFeedbackPropertiesEXT.transformFeedbackQueries` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceProperties.limits.bufferImageGranularity` | `64` | `65536` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyStandard2DBlockShape` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyAlignedMipSize` | `None` | `False` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyNonResidentStrict` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan13Properties.requiredSubgroupSizeStages[0]` | `None` | `VK_SHADER_STAGE_COMPUTE_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFeatures.vertexPipelineStoresAndAtomics` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.sparseBinding` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyAliased` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyBuffer` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyImage2D` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.shaderResourceResidency` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.shaderResourceMinLod` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.queueFlags[0]` | `None` | `VK_QUEUE_SPARSE_BINDING_BIT` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.queueCount` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_EXT_descriptor_buffer` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_AMD_buffer_marker` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceDescriptorBufferFeaturesEXT.descriptorBuffer` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceDescriptorBufferFeaturesEXT.descriptorBufferPushDescriptors` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceExtendedDynamicState2FeaturesEXT.extendedDynamicState2PatchControlPoints` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT.swapchainMaintenance1` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |

### VP_D3D12_FL_12_1_baseline: FAIL

| Requirement | Current | Required | Upstream PanVK | Implementation | Action |
|---|---:|---:|---|---|---|
| `extensions.VK_EXT_transform_feedback` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRobustness2FeaturesEXT.robustImageAccess2` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.transformFeedback` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.geometryStreams` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.geometryShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.tessellationShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.fillModeNonSolid` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.multiViewport` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.textureCompressionBC` | `False` | `True` | Hardware-dependent; G615 mask is zero | UNSUPPORTED_NATIVE | Keep false unless transparent BC emulation is complete |
| `features.VkPhysicalDeviceFeatures.pipelineStatisticsQuery` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderClipDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderCullDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `properties.VkPhysicalDeviceTransformFeedbackPropertiesEXT.transformFeedbackQueries` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.vertexPipelineStoresAndAtomics` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceProperties.limits.bufferImageGranularity` | `64` | `65536` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyStandard2DBlockShape` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyAlignedMipSize` | `None` | `False` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyNonResidentStrict` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[0]` | `None` | `VK_SUBGROUP_FEATURE_BALLOT_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[1]` | `None` | `VK_SUBGROUP_FEATURE_BASIC_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[2]` | `None` | `VK_SUBGROUP_FEATURE_VOTE_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[3]` | `None` | `VK_SUBGROUP_FEATURE_SHUFFLE_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[4]` | `None` | `VK_SUBGROUP_FEATURE_QUAD_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[5]` | `None` | `VK_SUBGROUP_FEATURE_ARITHMETIC_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedStages[0]` | `None` | `VK_SHADER_STAGE_COMPUTE_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedStages[1]` | `None` | `VK_SHADER_STAGE_FRAGMENT_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFeatures.sparseBinding` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyAliased` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyBuffer` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyImage2D` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.shaderResourceResidency` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.shaderResourceMinLod` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.queueFlags[0]` | `None` | `VK_QUEUE_SPARSE_BINDING_BIT` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.queueCount` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_EXT_fragment_shader_interlock` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT.fragmentShaderSampleInterlock` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT.fragmentShaderPixelInterlock` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |

### VP_D3D12_FL_12_2_baseline: FAIL

| Requirement | Current | Required | Upstream PanVK | Implementation | Action |
|---|---:|---:|---|---|---|
| `extensions.VK_EXT_transform_feedback` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRobustness2FeaturesEXT.robustImageAccess2` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.transformFeedback` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.geometryStreams` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.geometryShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.tessellationShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.fillModeNonSolid` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.multiViewport` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.textureCompressionBC` | `False` | `True` | Hardware-dependent; G615 mask is zero | UNSUPPORTED_NATIVE | Keep false unless transparent BC emulation is complete |
| `features.VkPhysicalDeviceFeatures.pipelineStatisticsQuery` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderClipDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderCullDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `properties.VkPhysicalDeviceTransformFeedbackPropertiesEXT.transformFeedbackQueries` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.vertexPipelineStoresAndAtomics` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[0]` | `None` | `VK_SUBGROUP_FEATURE_BALLOT_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[1]` | `None` | `VK_SUBGROUP_FEATURE_BASIC_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[2]` | `None` | `VK_SUBGROUP_FEATURE_VOTE_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[3]` | `None` | `VK_SUBGROUP_FEATURE_SHUFFLE_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[4]` | `None` | `VK_SUBGROUP_FEATURE_QUAD_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[5]` | `None` | `VK_SUBGROUP_FEATURE_ARITHMETIC_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedStages[0]` | `None` | `VK_SHADER_STAGE_COMPUTE_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedStages[1]` | `None` | `VK_SHADER_STAGE_FRAGMENT_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFeatures.sparseBinding` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyAliased` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyBuffer` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyImage2D` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.shaderResourceResidency` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.shaderResourceMinLod` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.queueFlags[0]` | `None` | `VK_QUEUE_SPARSE_BINDING_BIT` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.queueCount` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_EXT_fragment_shader_interlock` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT.fragmentShaderSampleInterlock` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT.fragmentShaderPixelInterlock` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_ray_tracing_pipeline` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_acceleration_structure` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_deferred_host_operations` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_ray_query` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_fragment_shading_rate` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_EXT_pipeline_library_group_handles` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_ray_tracing_maintenance1` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_EXT_mesh_shader` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R32G32_SFLOAT.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R32G32B32_SFLOAT.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16_SFLOAT.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16_SNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16B16A16_SFLOAT.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16B16A16_SNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16B16A16_UNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16_UNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_A2B10G10R10_UNORM_PACK32.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R8G8B8A8_UNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R8G8_UNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R8G8B8A8_SNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R8G8_SNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceAccelerationStructureFeaturesKHR.accelerationStructure` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingPipelineFeaturesKHR.rayTracingPipeline` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingPipelineFeaturesKHR.rayTracingPipelineTraceRaysIndirect` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingPipelineFeaturesKHR.rayTraversalPrimitiveCulling` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayQueryFeaturesKHR.rayQuery` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR.rayTracingMaintenance1` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR.rayTracingPipelineTraceRaysIndirect2` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDevicePipelineLibraryGroupHandlesFeaturesEXT.pipelineLibraryGroupHandles` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceMeshShaderFeaturesEXT.taskShader` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceMeshShaderFeaturesEXT.meshShader` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceMeshShaderFeaturesEXT.primitiveFragmentShadingRateMeshShader` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShadingRateFeaturesKHR.pipelineFragmentShadingRate` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShadingRateFeaturesKHR.primitiveFragmentShadingRate` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShadingRateFeaturesKHR.attachmentFragmentShadingRate` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFeatures.depthBounds` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.sparseResidencyImage3D` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceVulkan12Features.shaderOutputViewportIndex` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceConservativeRasterizationPropertiesEXT.degenerateTrianglesRasterized` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceConservativeRasterizationPropertiesEXT.fullyCoveredFragmentShaderInputVariable` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceFragmentShadingRatePropertiesKHR.fragmentShadingRateNonTrivialCombinerOps` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.queueFlags[0]` | `None` | `VK_QUEUE_TRANSFER_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.timestampValidBits` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.limits.bufferImageGranularity` | `64` | `65536` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyStandard2DBlockShape` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyStandard3DBlockShape` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyAlignedMipSize` | `None` | `False` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyNonResidentStrict` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |

### VP_D3D12_FL_12_2_optimal: FAIL

| Requirement | Current | Required | Upstream PanVK | Implementation | Action |
|---|---:|---:|---|---|---|
| `extensions.VK_EXT_transform_feedback` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRobustness2FeaturesEXT.robustImageAccess2` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.transformFeedback` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.geometryStreams` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.geometryShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.tessellationShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.fillModeNonSolid` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.multiViewport` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.textureCompressionBC` | `False` | `True` | Hardware-dependent; G615 mask is zero | UNSUPPORTED_NATIVE | Keep false unless transparent BC emulation is complete |
| `features.VkPhysicalDeviceFeatures.pipelineStatisticsQuery` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderClipDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderCullDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `properties.VkPhysicalDeviceTransformFeedbackPropertiesEXT.transformFeedbackQueries` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.vertexPipelineStoresAndAtomics` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceVulkan13Properties.requiredSubgroupSizeStages[0]` | `None` | `VK_SHADER_STAGE_COMPUTE_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFeatures.sparseBinding` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyAliased` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyBuffer` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyImage2D` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.shaderResourceResidency` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.shaderResourceMinLod` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.queueFlags[0]` | `None` | `VK_QUEUE_SPARSE_BINDING_BIT` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.queueCount` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_EXT_fragment_shader_interlock` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT.fragmentShaderSampleInterlock` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT.fragmentShaderPixelInterlock` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_ray_tracing_pipeline` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_acceleration_structure` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_deferred_host_operations` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_ray_query` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_fragment_shading_rate` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_EXT_pipeline_library_group_handles` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_ray_tracing_maintenance1` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_EXT_mesh_shader` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R32G32_SFLOAT.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R32G32B32_SFLOAT.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16_SFLOAT.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16_SNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16B16A16_SFLOAT.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16B16A16_SNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16B16A16_UNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16_UNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_A2B10G10R10_UNORM_PACK32.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R8G8B8A8_UNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R8G8_UNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R8G8B8A8_SNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R8G8_SNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceAccelerationStructureFeaturesKHR.accelerationStructure` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingPipelineFeaturesKHR.rayTracingPipeline` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingPipelineFeaturesKHR.rayTracingPipelineTraceRaysIndirect` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingPipelineFeaturesKHR.rayTraversalPrimitiveCulling` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayQueryFeaturesKHR.rayQuery` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR.rayTracingMaintenance1` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR.rayTracingPipelineTraceRaysIndirect2` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDevicePipelineLibraryGroupHandlesFeaturesEXT.pipelineLibraryGroupHandles` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceMeshShaderFeaturesEXT.taskShader` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceMeshShaderFeaturesEXT.meshShader` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceMeshShaderFeaturesEXT.primitiveFragmentShadingRateMeshShader` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShadingRateFeaturesKHR.pipelineFragmentShadingRate` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShadingRateFeaturesKHR.primitiveFragmentShadingRate` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShadingRateFeaturesKHR.attachmentFragmentShadingRate` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFeatures.depthBounds` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.sparseResidencyImage3D` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceVulkan12Features.shaderOutputViewportIndex` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceConservativeRasterizationPropertiesEXT.degenerateTrianglesRasterized` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceConservativeRasterizationPropertiesEXT.fullyCoveredFragmentShaderInputVariable` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceFragmentShadingRatePropertiesKHR.fragmentShadingRateNonTrivialCombinerOps` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.queueFlags[0]` | `None` | `VK_QUEUE_TRANSFER_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.timestampValidBits` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.limits.bufferImageGranularity` | `64` | `65536` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyStandard2DBlockShape` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyStandard3DBlockShape` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyAlignedMipSize` | `None` | `False` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyNonResidentStrict` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_EXT_descriptor_buffer` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_AMD_buffer_marker` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceDescriptorBufferFeaturesEXT.descriptorBuffer` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceDescriptorBufferFeaturesEXT.descriptorBufferPushDescriptors` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceExtendedDynamicState2FeaturesEXT.extendedDynamicState2PatchControlPoints` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT.swapchainMaintenance1` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |

### VP_D3D12_maximum_radv: FAIL

| Requirement | Current | Required | Upstream PanVK | Implementation | Action |
|---|---:|---:|---|---|---|
| `extensions.VK_EXT_transform_feedback` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRobustness2FeaturesEXT.robustImageAccess2` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.transformFeedback` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.geometryStreams` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.geometryShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.tessellationShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.fillModeNonSolid` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.multiViewport` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.textureCompressionBC` | `False` | `True` | Hardware-dependent; G615 mask is zero | UNSUPPORTED_NATIVE | Keep false unless transparent BC emulation is complete |
| `features.VkPhysicalDeviceFeatures.pipelineStatisticsQuery` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderClipDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderCullDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `properties.VkPhysicalDeviceTransformFeedbackPropertiesEXT.transformFeedbackQueries` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.vertexPipelineStoresAndAtomics` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceVulkan13Properties.requiredSubgroupSizeStages[0]` | `None` | `VK_SHADER_STAGE_COMPUTE_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFeatures.sparseBinding` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyAliased` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyBuffer` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyImage2D` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.shaderResourceResidency` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.shaderResourceMinLod` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.queueFlags[0]` | `None` | `VK_QUEUE_SPARSE_BINDING_BIT` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.queueCount` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_ray_tracing_pipeline` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_acceleration_structure` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_deferred_host_operations` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_ray_query` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_fragment_shading_rate` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_EXT_pipeline_library_group_handles` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_ray_tracing_maintenance1` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_EXT_mesh_shader` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R32G32_SFLOAT.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R32G32B32_SFLOAT.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16_SFLOAT.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16_SNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16B16A16_SFLOAT.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16B16A16_SNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16B16A16_UNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16_UNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_A2B10G10R10_UNORM_PACK32.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R8G8B8A8_UNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R8G8_UNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R8G8B8A8_SNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R8G8_SNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceAccelerationStructureFeaturesKHR.accelerationStructure` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingPipelineFeaturesKHR.rayTracingPipeline` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingPipelineFeaturesKHR.rayTracingPipelineTraceRaysIndirect` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingPipelineFeaturesKHR.rayTraversalPrimitiveCulling` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayQueryFeaturesKHR.rayQuery` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR.rayTracingMaintenance1` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR.rayTracingPipelineTraceRaysIndirect2` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDevicePipelineLibraryGroupHandlesFeaturesEXT.pipelineLibraryGroupHandles` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceMeshShaderFeaturesEXT.taskShader` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceMeshShaderFeaturesEXT.meshShader` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceMeshShaderFeaturesEXT.primitiveFragmentShadingRateMeshShader` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShadingRateFeaturesKHR.pipelineFragmentShadingRate` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShadingRateFeaturesKHR.primitiveFragmentShadingRate` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShadingRateFeaturesKHR.attachmentFragmentShadingRate` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFeatures.depthBounds` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.sparseResidencyImage3D` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceVulkan12Features.shaderOutputViewportIndex` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceConservativeRasterizationPropertiesEXT.degenerateTrianglesRasterized` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceConservativeRasterizationPropertiesEXT.fullyCoveredFragmentShaderInputVariable` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceFragmentShadingRatePropertiesKHR.fragmentShadingRateNonTrivialCombinerOps` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.queueFlags[0]` | `None` | `VK_QUEUE_TRANSFER_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.timestampValidBits` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.limits.bufferImageGranularity` | `64` | `65536` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyStandard2DBlockShape` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyStandard3DBlockShape` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyAlignedMipSize` | `None` | `False` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyNonResidentStrict` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_EXT_descriptor_buffer` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_AMD_buffer_marker` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceDescriptorBufferFeaturesEXT.descriptorBuffer` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceDescriptorBufferFeaturesEXT.descriptorBufferPushDescriptors` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceExtendedDynamicState2FeaturesEXT.extendedDynamicState2PatchControlPoints` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT.swapchainMaintenance1` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |

### VP_D3D12_maximum_nv: FAIL

| Requirement | Current | Required | Upstream PanVK | Implementation | Action |
|---|---:|---:|---|---|---|
| `extensions.VK_EXT_transform_feedback` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRobustness2FeaturesEXT.robustImageAccess2` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.transformFeedback` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.geometryStreams` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.geometryShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.tessellationShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.fillModeNonSolid` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.multiViewport` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.textureCompressionBC` | `False` | `True` | Hardware-dependent; G615 mask is zero | UNSUPPORTED_NATIVE | Keep false unless transparent BC emulation is complete |
| `features.VkPhysicalDeviceFeatures.pipelineStatisticsQuery` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderClipDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderCullDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `properties.VkPhysicalDeviceTransformFeedbackPropertiesEXT.transformFeedbackQueries` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.vertexPipelineStoresAndAtomics` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceVulkan13Properties.requiredSubgroupSizeStages[0]` | `None` | `VK_SHADER_STAGE_COMPUTE_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFeatures.sparseBinding` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyAliased` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyBuffer` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyImage2D` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.shaderResourceResidency` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.shaderResourceMinLod` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.queueFlags[0]` | `None` | `VK_QUEUE_SPARSE_BINDING_BIT` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.queueCount` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_EXT_fragment_shader_interlock` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT.fragmentShaderSampleInterlock` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT.fragmentShaderPixelInterlock` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_ray_tracing_pipeline` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_acceleration_structure` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_deferred_host_operations` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_ray_query` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_fragment_shading_rate` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_EXT_pipeline_library_group_handles` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_ray_tracing_maintenance1` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_EXT_mesh_shader` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R32G32_SFLOAT.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R32G32B32_SFLOAT.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16_SFLOAT.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16_SNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16B16A16_SFLOAT.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16B16A16_SNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16B16A16_UNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16_UNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_A2B10G10R10_UNORM_PACK32.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R8G8B8A8_UNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R8G8_UNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R8G8B8A8_SNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R8G8_SNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceAccelerationStructureFeaturesKHR.accelerationStructure` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingPipelineFeaturesKHR.rayTracingPipeline` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingPipelineFeaturesKHR.rayTracingPipelineTraceRaysIndirect` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingPipelineFeaturesKHR.rayTraversalPrimitiveCulling` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayQueryFeaturesKHR.rayQuery` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR.rayTracingMaintenance1` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR.rayTracingPipelineTraceRaysIndirect2` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDevicePipelineLibraryGroupHandlesFeaturesEXT.pipelineLibraryGroupHandles` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceMeshShaderFeaturesEXT.taskShader` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceMeshShaderFeaturesEXT.meshShader` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceMeshShaderFeaturesEXT.primitiveFragmentShadingRateMeshShader` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShadingRateFeaturesKHR.pipelineFragmentShadingRate` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShadingRateFeaturesKHR.primitiveFragmentShadingRate` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShadingRateFeaturesKHR.attachmentFragmentShadingRate` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFeatures.depthBounds` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.sparseResidencyImage3D` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceVulkan12Features.shaderOutputViewportIndex` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceConservativeRasterizationPropertiesEXT.degenerateTrianglesRasterized` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceConservativeRasterizationPropertiesEXT.fullyCoveredFragmentShaderInputVariable` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceFragmentShadingRatePropertiesKHR.fragmentShadingRateNonTrivialCombinerOps` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.queueFlags[0]` | `None` | `VK_QUEUE_TRANSFER_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.timestampValidBits` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.limits.bufferImageGranularity` | `64` | `65536` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyStandard2DBlockShape` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyStandard3DBlockShape` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyAlignedMipSize` | `None` | `False` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyNonResidentStrict` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_EXT_descriptor_buffer` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_AMD_buffer_marker` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceDescriptorBufferFeaturesEXT.descriptorBuffer` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceDescriptorBufferFeaturesEXT.descriptorBufferPushDescriptors` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceExtendedDynamicState2FeaturesEXT.extendedDynamicState2PatchControlPoints` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT.swapchainMaintenance1` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |

## vkd3d-3.0.1

Source: `v3.0.1` `https://raw.githubusercontent.com/HansKristian-Work/vkd3d-proton/v3.0.1/VP_D3D12_VKD3D_PROTON_profile.json` (`d37e753fa81e43251bbd576b17b888c1a7139b3fc8c16bd0e0ec9c7bed7a9db6`)

### VP_D3D12_FL_11_0_baseline: FAIL

| Requirement | Current | Required | Upstream PanVK | Implementation | Action |
|---|---:|---:|---|---|---|
| `extensions.VK_EXT_transform_feedback` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRobustness2FeaturesEXT.robustImageAccess2` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.transformFeedback` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.geometryStreams` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.geometryShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.tessellationShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.fillModeNonSolid` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.multiViewport` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.textureCompressionBC` | `False` | `True` | Hardware-dependent; G615 mask is zero | UNSUPPORTED_NATIVE | Keep false unless transparent BC emulation is complete |
| `features.VkPhysicalDeviceFeatures.pipelineStatisticsQuery` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderClipDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderCullDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `properties.VkPhysicalDeviceTransformFeedbackPropertiesEXT.transformFeedbackQueries` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceProperties.limits.bufferImageGranularity` | `64` | `65536` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[0]` | `None` | `VK_SUBGROUP_FEATURE_BASIC_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |

### VP_D3D12_FL_11_1_baseline: FAIL

| Requirement | Current | Required | Upstream PanVK | Implementation | Action |
|---|---:|---:|---|---|---|
| `extensions.VK_EXT_transform_feedback` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRobustness2FeaturesEXT.robustImageAccess2` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.transformFeedback` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.geometryStreams` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.geometryShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.tessellationShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.fillModeNonSolid` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.multiViewport` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.textureCompressionBC` | `False` | `True` | Hardware-dependent; G615 mask is zero | UNSUPPORTED_NATIVE | Keep false unless transparent BC emulation is complete |
| `features.VkPhysicalDeviceFeatures.pipelineStatisticsQuery` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderClipDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderCullDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `properties.VkPhysicalDeviceTransformFeedbackPropertiesEXT.transformFeedbackQueries` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceProperties.limits.bufferImageGranularity` | `64` | `65536` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[0]` | `None` | `VK_SUBGROUP_FEATURE_BASIC_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFeatures.vertexPipelineStoresAndAtomics` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |

### VP_D3D12_FL_12_0_baseline: FAIL

| Requirement | Current | Required | Upstream PanVK | Implementation | Action |
|---|---:|---:|---|---|---|
| `extensions.VK_EXT_transform_feedback` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRobustness2FeaturesEXT.robustImageAccess2` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.transformFeedback` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.geometryStreams` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.geometryShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.tessellationShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.fillModeNonSolid` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.multiViewport` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.textureCompressionBC` | `False` | `True` | Hardware-dependent; G615 mask is zero | UNSUPPORTED_NATIVE | Keep false unless transparent BC emulation is complete |
| `features.VkPhysicalDeviceFeatures.pipelineStatisticsQuery` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderClipDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderCullDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `properties.VkPhysicalDeviceTransformFeedbackPropertiesEXT.transformFeedbackQueries` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceProperties.limits.bufferImageGranularity` | `64` | `65536` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyStandard2DBlockShape` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyAlignedMipSize` | `None` | `False` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyNonResidentStrict` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[0]` | `None` | `VK_SUBGROUP_FEATURE_BALLOT_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[1]` | `None` | `VK_SUBGROUP_FEATURE_BASIC_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[2]` | `None` | `VK_SUBGROUP_FEATURE_VOTE_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[3]` | `None` | `VK_SUBGROUP_FEATURE_SHUFFLE_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[4]` | `None` | `VK_SUBGROUP_FEATURE_QUAD_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[5]` | `None` | `VK_SUBGROUP_FEATURE_ARITHMETIC_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedStages[0]` | `None` | `VK_SHADER_STAGE_COMPUTE_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedStages[1]` | `None` | `VK_SHADER_STAGE_FRAGMENT_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFeatures.vertexPipelineStoresAndAtomics` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.sparseBinding` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyAliased` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyBuffer` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyImage2D` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.shaderResourceResidency` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.shaderResourceMinLod` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.queueFlags[0]` | `None` | `VK_QUEUE_SPARSE_BINDING_BIT` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.queueCount` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |

### VP_D3D12_FL_12_0_optimal: FAIL

| Requirement | Current | Required | Upstream PanVK | Implementation | Action |
|---|---:|---:|---|---|---|
| `extensions.VK_EXT_transform_feedback` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRobustness2FeaturesEXT.robustImageAccess2` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.transformFeedback` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.geometryStreams` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.geometryShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.tessellationShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.fillModeNonSolid` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.multiViewport` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.textureCompressionBC` | `False` | `True` | Hardware-dependent; G615 mask is zero | UNSUPPORTED_NATIVE | Keep false unless transparent BC emulation is complete |
| `features.VkPhysicalDeviceFeatures.pipelineStatisticsQuery` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderClipDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderCullDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `properties.VkPhysicalDeviceTransformFeedbackPropertiesEXT.transformFeedbackQueries` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceProperties.limits.bufferImageGranularity` | `64` | `65536` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyStandard2DBlockShape` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyAlignedMipSize` | `None` | `False` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyNonResidentStrict` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan13Properties.requiredSubgroupSizeStages[0]` | `None` | `VK_SHADER_STAGE_COMPUTE_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFeatures.vertexPipelineStoresAndAtomics` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.sparseBinding` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyAliased` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyBuffer` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyImage2D` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.shaderResourceResidency` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.shaderResourceMinLod` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.queueFlags[0]` | `None` | `VK_QUEUE_SPARSE_BINDING_BIT` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.queueCount` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_EXT_descriptor_buffer` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_AMD_buffer_marker` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_maintenance10` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceDescriptorBufferFeaturesEXT.descriptorBuffer` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceDescriptorBufferFeaturesEXT.descriptorBufferPushDescriptors` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceExtendedDynamicState2FeaturesEXT.extendedDynamicState2PatchControlPoints` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT.swapchainMaintenance1` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceMaintenance10FeaturesKHR.maintenance10` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |

### VP_D3D12_FL_12_1_baseline: FAIL

| Requirement | Current | Required | Upstream PanVK | Implementation | Action |
|---|---:|---:|---|---|---|
| `extensions.VK_EXT_transform_feedback` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRobustness2FeaturesEXT.robustImageAccess2` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.transformFeedback` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.geometryStreams` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.geometryShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.tessellationShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.fillModeNonSolid` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.multiViewport` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.textureCompressionBC` | `False` | `True` | Hardware-dependent; G615 mask is zero | UNSUPPORTED_NATIVE | Keep false unless transparent BC emulation is complete |
| `features.VkPhysicalDeviceFeatures.pipelineStatisticsQuery` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderClipDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderCullDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `properties.VkPhysicalDeviceTransformFeedbackPropertiesEXT.transformFeedbackQueries` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.vertexPipelineStoresAndAtomics` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceProperties.limits.bufferImageGranularity` | `64` | `65536` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyStandard2DBlockShape` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyAlignedMipSize` | `None` | `False` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyNonResidentStrict` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[0]` | `None` | `VK_SUBGROUP_FEATURE_BALLOT_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[1]` | `None` | `VK_SUBGROUP_FEATURE_BASIC_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[2]` | `None` | `VK_SUBGROUP_FEATURE_VOTE_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[3]` | `None` | `VK_SUBGROUP_FEATURE_SHUFFLE_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[4]` | `None` | `VK_SUBGROUP_FEATURE_QUAD_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[5]` | `None` | `VK_SUBGROUP_FEATURE_ARITHMETIC_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedStages[0]` | `None` | `VK_SHADER_STAGE_COMPUTE_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedStages[1]` | `None` | `VK_SHADER_STAGE_FRAGMENT_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFeatures.sparseBinding` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyAliased` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyBuffer` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyImage2D` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.shaderResourceResidency` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.shaderResourceMinLod` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.queueFlags[0]` | `None` | `VK_QUEUE_SPARSE_BINDING_BIT` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.queueCount` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_EXT_fragment_shader_interlock` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT.fragmentShaderSampleInterlock` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT.fragmentShaderPixelInterlock` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |

### VP_D3D12_FL_12_2_baseline: FAIL

| Requirement | Current | Required | Upstream PanVK | Implementation | Action |
|---|---:|---:|---|---|---|
| `extensions.VK_EXT_transform_feedback` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRobustness2FeaturesEXT.robustImageAccess2` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.transformFeedback` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.geometryStreams` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.geometryShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.tessellationShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.fillModeNonSolid` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.multiViewport` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.textureCompressionBC` | `False` | `True` | Hardware-dependent; G615 mask is zero | UNSUPPORTED_NATIVE | Keep false unless transparent BC emulation is complete |
| `features.VkPhysicalDeviceFeatures.pipelineStatisticsQuery` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderClipDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderCullDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `properties.VkPhysicalDeviceTransformFeedbackPropertiesEXT.transformFeedbackQueries` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.vertexPipelineStoresAndAtomics` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[0]` | `None` | `VK_SUBGROUP_FEATURE_BALLOT_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[1]` | `None` | `VK_SUBGROUP_FEATURE_BASIC_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[2]` | `None` | `VK_SUBGROUP_FEATURE_VOTE_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[3]` | `None` | `VK_SUBGROUP_FEATURE_SHUFFLE_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[4]` | `None` | `VK_SUBGROUP_FEATURE_QUAD_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedOperations[5]` | `None` | `VK_SUBGROUP_FEATURE_ARITHMETIC_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedStages[0]` | `None` | `VK_SHADER_STAGE_COMPUTE_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceVulkan11Properties.subgroupSupportedStages[1]` | `None` | `VK_SHADER_STAGE_FRAGMENT_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFeatures.sparseBinding` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyAliased` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyBuffer` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyImage2D` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.shaderResourceResidency` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.shaderResourceMinLod` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.queueFlags[0]` | `None` | `VK_QUEUE_SPARSE_BINDING_BIT` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.queueCount` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_EXT_fragment_shader_interlock` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT.fragmentShaderSampleInterlock` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT.fragmentShaderPixelInterlock` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_ray_tracing_pipeline` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_acceleration_structure` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_deferred_host_operations` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_ray_query` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_fragment_shading_rate` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_EXT_pipeline_library_group_handles` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_ray_tracing_maintenance1` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_EXT_mesh_shader` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R32G32_SFLOAT.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R32G32B32_SFLOAT.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16_SFLOAT.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16_SNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16B16A16_SFLOAT.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16B16A16_SNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16B16A16_UNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16_UNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_A2B10G10R10_UNORM_PACK32.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R8G8B8A8_UNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R8G8_UNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R8G8B8A8_SNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R8G8_SNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceAccelerationStructureFeaturesKHR.accelerationStructure` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingPipelineFeaturesKHR.rayTracingPipeline` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingPipelineFeaturesKHR.rayTracingPipelineTraceRaysIndirect` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingPipelineFeaturesKHR.rayTraversalPrimitiveCulling` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayQueryFeaturesKHR.rayQuery` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR.rayTracingMaintenance1` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR.rayTracingPipelineTraceRaysIndirect2` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDevicePipelineLibraryGroupHandlesFeaturesEXT.pipelineLibraryGroupHandles` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceMeshShaderFeaturesEXT.taskShader` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceMeshShaderFeaturesEXT.meshShader` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceMeshShaderFeaturesEXT.primitiveFragmentShadingRateMeshShader` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShadingRateFeaturesKHR.pipelineFragmentShadingRate` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShadingRateFeaturesKHR.primitiveFragmentShadingRate` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShadingRateFeaturesKHR.attachmentFragmentShadingRate` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFeatures.depthBounds` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.sparseResidencyImage3D` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceVulkan12Features.shaderOutputViewportIndex` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceConservativeRasterizationPropertiesEXT.degenerateTrianglesRasterized` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceConservativeRasterizationPropertiesEXT.fullyCoveredFragmentShaderInputVariable` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceFragmentShadingRatePropertiesKHR.fragmentShadingRateNonTrivialCombinerOps` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.queueFlags[0]` | `None` | `VK_QUEUE_TRANSFER_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.timestampValidBits` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.limits.bufferImageGranularity` | `64` | `65536` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyStandard2DBlockShape` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyStandard3DBlockShape` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyAlignedMipSize` | `None` | `False` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyNonResidentStrict` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |

### VP_D3D12_FL_12_2_optimal: FAIL

| Requirement | Current | Required | Upstream PanVK | Implementation | Action |
|---|---:|---:|---|---|---|
| `extensions.VK_EXT_transform_feedback` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRobustness2FeaturesEXT.robustImageAccess2` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.transformFeedback` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.geometryStreams` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.geometryShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.tessellationShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.fillModeNonSolid` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.multiViewport` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.textureCompressionBC` | `False` | `True` | Hardware-dependent; G615 mask is zero | UNSUPPORTED_NATIVE | Keep false unless transparent BC emulation is complete |
| `features.VkPhysicalDeviceFeatures.pipelineStatisticsQuery` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderClipDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderCullDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `properties.VkPhysicalDeviceTransformFeedbackPropertiesEXT.transformFeedbackQueries` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.vertexPipelineStoresAndAtomics` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceVulkan13Properties.requiredSubgroupSizeStages[0]` | `None` | `VK_SHADER_STAGE_COMPUTE_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFeatures.sparseBinding` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyAliased` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyBuffer` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyImage2D` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.shaderResourceResidency` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.shaderResourceMinLod` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.queueFlags[0]` | `None` | `VK_QUEUE_SPARSE_BINDING_BIT` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.queueCount` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_EXT_fragment_shader_interlock` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT.fragmentShaderSampleInterlock` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT.fragmentShaderPixelInterlock` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_ray_tracing_pipeline` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_acceleration_structure` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_deferred_host_operations` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_ray_query` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_fragment_shading_rate` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_EXT_pipeline_library_group_handles` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_ray_tracing_maintenance1` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_EXT_mesh_shader` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R32G32_SFLOAT.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R32G32B32_SFLOAT.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16_SFLOAT.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16_SNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16B16A16_SFLOAT.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16B16A16_SNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16B16A16_UNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16_UNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_A2B10G10R10_UNORM_PACK32.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R8G8B8A8_UNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R8G8_UNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R8G8B8A8_SNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R8G8_SNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceAccelerationStructureFeaturesKHR.accelerationStructure` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingPipelineFeaturesKHR.rayTracingPipeline` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingPipelineFeaturesKHR.rayTracingPipelineTraceRaysIndirect` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingPipelineFeaturesKHR.rayTraversalPrimitiveCulling` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayQueryFeaturesKHR.rayQuery` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR.rayTracingMaintenance1` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR.rayTracingPipelineTraceRaysIndirect2` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDevicePipelineLibraryGroupHandlesFeaturesEXT.pipelineLibraryGroupHandles` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceMeshShaderFeaturesEXT.taskShader` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceMeshShaderFeaturesEXT.meshShader` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceMeshShaderFeaturesEXT.primitiveFragmentShadingRateMeshShader` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShadingRateFeaturesKHR.pipelineFragmentShadingRate` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShadingRateFeaturesKHR.primitiveFragmentShadingRate` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShadingRateFeaturesKHR.attachmentFragmentShadingRate` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFeatures.depthBounds` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.sparseResidencyImage3D` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceVulkan12Features.shaderOutputViewportIndex` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceConservativeRasterizationPropertiesEXT.degenerateTrianglesRasterized` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceConservativeRasterizationPropertiesEXT.fullyCoveredFragmentShaderInputVariable` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceFragmentShadingRatePropertiesKHR.fragmentShadingRateNonTrivialCombinerOps` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.queueFlags[0]` | `None` | `VK_QUEUE_TRANSFER_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.timestampValidBits` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.limits.bufferImageGranularity` | `64` | `65536` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyStandard2DBlockShape` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyStandard3DBlockShape` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyAlignedMipSize` | `None` | `False` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyNonResidentStrict` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_EXT_descriptor_buffer` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_AMD_buffer_marker` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_maintenance10` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceDescriptorBufferFeaturesEXT.descriptorBuffer` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceDescriptorBufferFeaturesEXT.descriptorBufferPushDescriptors` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceExtendedDynamicState2FeaturesEXT.extendedDynamicState2PatchControlPoints` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT.swapchainMaintenance1` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceMaintenance10FeaturesKHR.maintenance10` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |

### VP_D3D12_maximum_radv: FAIL

| Requirement | Current | Required | Upstream PanVK | Implementation | Action |
|---|---:|---:|---|---|---|
| `extensions.VK_EXT_transform_feedback` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRobustness2FeaturesEXT.robustImageAccess2` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.transformFeedback` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.geometryStreams` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.geometryShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.tessellationShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.fillModeNonSolid` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.multiViewport` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.textureCompressionBC` | `False` | `True` | Hardware-dependent; G615 mask is zero | UNSUPPORTED_NATIVE | Keep false unless transparent BC emulation is complete |
| `features.VkPhysicalDeviceFeatures.pipelineStatisticsQuery` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderClipDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderCullDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `properties.VkPhysicalDeviceTransformFeedbackPropertiesEXT.transformFeedbackQueries` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.vertexPipelineStoresAndAtomics` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceVulkan13Properties.requiredSubgroupSizeStages[0]` | `None` | `VK_SHADER_STAGE_COMPUTE_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFeatures.sparseBinding` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyAliased` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyBuffer` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyImage2D` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.shaderResourceResidency` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.shaderResourceMinLod` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.queueFlags[0]` | `None` | `VK_QUEUE_SPARSE_BINDING_BIT` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.queueCount` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_ray_tracing_pipeline` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_acceleration_structure` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_deferred_host_operations` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_ray_query` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_fragment_shading_rate` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_EXT_pipeline_library_group_handles` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_ray_tracing_maintenance1` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_EXT_mesh_shader` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R32G32_SFLOAT.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R32G32B32_SFLOAT.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16_SFLOAT.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16_SNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16B16A16_SFLOAT.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16B16A16_SNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16B16A16_UNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16_UNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_A2B10G10R10_UNORM_PACK32.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R8G8B8A8_UNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R8G8_UNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R8G8B8A8_SNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R8G8_SNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceAccelerationStructureFeaturesKHR.accelerationStructure` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingPipelineFeaturesKHR.rayTracingPipeline` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingPipelineFeaturesKHR.rayTracingPipelineTraceRaysIndirect` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingPipelineFeaturesKHR.rayTraversalPrimitiveCulling` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayQueryFeaturesKHR.rayQuery` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR.rayTracingMaintenance1` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR.rayTracingPipelineTraceRaysIndirect2` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDevicePipelineLibraryGroupHandlesFeaturesEXT.pipelineLibraryGroupHandles` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceMeshShaderFeaturesEXT.taskShader` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceMeshShaderFeaturesEXT.meshShader` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceMeshShaderFeaturesEXT.primitiveFragmentShadingRateMeshShader` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShadingRateFeaturesKHR.pipelineFragmentShadingRate` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShadingRateFeaturesKHR.primitiveFragmentShadingRate` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShadingRateFeaturesKHR.attachmentFragmentShadingRate` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFeatures.depthBounds` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.sparseResidencyImage3D` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceVulkan12Features.shaderOutputViewportIndex` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceConservativeRasterizationPropertiesEXT.degenerateTrianglesRasterized` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceConservativeRasterizationPropertiesEXT.fullyCoveredFragmentShaderInputVariable` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceFragmentShadingRatePropertiesKHR.fragmentShadingRateNonTrivialCombinerOps` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.queueFlags[0]` | `None` | `VK_QUEUE_TRANSFER_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.timestampValidBits` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.limits.bufferImageGranularity` | `64` | `65536` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyStandard2DBlockShape` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyStandard3DBlockShape` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyAlignedMipSize` | `None` | `False` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyNonResidentStrict` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_EXT_descriptor_buffer` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_AMD_buffer_marker` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_maintenance10` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceDescriptorBufferFeaturesEXT.descriptorBuffer` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceDescriptorBufferFeaturesEXT.descriptorBufferPushDescriptors` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceExtendedDynamicState2FeaturesEXT.extendedDynamicState2PatchControlPoints` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT.swapchainMaintenance1` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceMaintenance10FeaturesKHR.maintenance10` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |

### VP_D3D12_maximum_nv: FAIL

| Requirement | Current | Required | Upstream PanVK | Implementation | Action |
|---|---:|---:|---|---|---|
| `extensions.VK_EXT_transform_feedback` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRobustness2FeaturesEXT.robustImageAccess2` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.transformFeedback` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceTransformFeedbackFeaturesEXT.geometryStreams` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.geometryShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.tessellationShader` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.fillModeNonSolid` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.multiViewport` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.textureCompressionBC` | `False` | `True` | Hardware-dependent; G615 mask is zero | UNSUPPORTED_NATIVE | Keep false unless transparent BC emulation is complete |
| `features.VkPhysicalDeviceFeatures.pipelineStatisticsQuery` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderClipDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `features.VkPhysicalDeviceFeatures.shaderCullDistance` | `False` | `True` | ABSENT at pinned and origin/main | NOT_IMPLEMENTED | Design/implement after P5 gates; never expose early |
| `properties.VkPhysicalDeviceTransformFeedbackPropertiesEXT.transformFeedbackQueries` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.vertexPipelineStoresAndAtomics` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceVulkan13Properties.requiredSubgroupSizeStages[0]` | `None` | `VK_SHADER_STAGE_COMPUTE_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFeatures.sparseBinding` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyAliased` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyBuffer` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.sparseResidencyImage2D` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceFeatures.shaderResourceResidency` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.shaderResourceMinLod` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.queueFlags[0]` | `None` | `VK_QUEUE_SPARSE_BINDING_BIT` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.queueCount` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_EXT_fragment_shader_interlock` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT.fragmentShaderSampleInterlock` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT.fragmentShaderPixelInterlock` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_ray_tracing_pipeline` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_acceleration_structure` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_deferred_host_operations` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_ray_query` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_fragment_shading_rate` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_EXT_pipeline_library_group_handles` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_ray_tracing_maintenance1` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_EXT_mesh_shader` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R32G32_SFLOAT.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R32G32B32_SFLOAT.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16_SFLOAT.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16_SNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16B16A16_SFLOAT.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16B16A16_SNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16B16A16_UNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R16G16_UNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_A2B10G10R10_UNORM_PACK32.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R8G8B8A8_UNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R8G8_UNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R8G8B8A8_SNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `formats.VK_FORMAT_R8G8_SNORM.VkFormatProperties.bufferFeatures[0]` | `None` | `VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceAccelerationStructureFeaturesKHR.accelerationStructure` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingPipelineFeaturesKHR.rayTracingPipeline` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingPipelineFeaturesKHR.rayTracingPipelineTraceRaysIndirect` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingPipelineFeaturesKHR.rayTraversalPrimitiveCulling` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayQueryFeaturesKHR.rayQuery` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR.rayTracingMaintenance1` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR.rayTracingPipelineTraceRaysIndirect2` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDevicePipelineLibraryGroupHandlesFeaturesEXT.pipelineLibraryGroupHandles` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceMeshShaderFeaturesEXT.taskShader` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceMeshShaderFeaturesEXT.meshShader` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceMeshShaderFeaturesEXT.primitiveFragmentShadingRateMeshShader` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShadingRateFeaturesKHR.pipelineFragmentShadingRate` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShadingRateFeaturesKHR.primitiveFragmentShadingRate` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFragmentShadingRateFeaturesKHR.attachmentFragmentShadingRate` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceFeatures.depthBounds` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceFeatures.sparseResidencyImage3D` | `False` | `True` | Generic PanVK exists; custom Kbase equivalence unproven | BLOCKED_KBASE | Defer sparse; assess Kbase VM semantics only when required |
| `features.VkPhysicalDeviceVulkan12Features.shaderOutputViewportIndex` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceConservativeRasterizationPropertiesEXT.degenerateTrianglesRasterized` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceConservativeRasterizationPropertiesEXT.fullyCoveredFragmentShaderInputVariable` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceFragmentShadingRatePropertiesKHR.fragmentShadingRateNonTrivialCombinerOps` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.queueFlags[0]` | `None` | `VK_QUEUE_TRANSFER_BIT` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `queueFamiliesProperties[0].VkQueueFamilyProperties.timestampValidBits` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.limits.bufferImageGranularity` | `64` | `65536` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyStandard2DBlockShape` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyStandard3DBlockShape` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyAlignedMipSize` | `None` | `False` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `properties.VkPhysicalDeviceProperties.sparseProperties.residencyNonResidentStrict` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_EXT_descriptor_buffer` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_AMD_buffer_marker` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `extensions.VK_KHR_maintenance10` | `None` | `1` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceDescriptorBufferFeaturesEXT.descriptorBuffer` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceDescriptorBufferFeaturesEXT.descriptorBufferPushDescriptors` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceExtendedDynamicState2FeaturesEXT.extendedDynamicState2PatchControlPoints` | `False` | `True` | Pinned runtime does not satisfy; no post-pin matching backport found | NOT_SATISFIED | Investigate in the owning feature phase |
| `features.VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT.swapchainMaintenance1` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |
| `features.VkPhysicalDeviceMaintenance10FeaturesKHR.maintenance10` | `None` | `True` | UNKNOWN | UNKNOWN | Extend capture or implementation audit before claiming support |

