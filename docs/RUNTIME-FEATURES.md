# Runtime Device Capabilities (Mali-G615 MC6)

On-device query of the exact beta.2 ICD (`libvulkan_panfrost.so`), not Mesa Matrix.

Schema for every capability:

```json
{
  "geometryShader": {
    "upstreamImplemented": false,
    "runtimeExposed": false,
    "runtimeTested": false
  }
}
```

- `upstreamImplemented` — Mesa PanVK source advertises/implements the path
- `runtimeExposed` — `vkGetPhysicalDeviceFeatures2` / extension enumerate on this ICD
- `runtimeTested` — a real workload on this tree exercised it (not merely a true bit)

Normative file: `dist/runtime-feature-matrix.json`  
Capture: `scripts/capture-runtime-features` / `scripts/capture-runtime-features.py`

## Capture (Poco X6 Pro, 2026-09-19)

| Field | Value |
|---|---|
| deviceName | Mali-G615 MC6 |
| device | Poco X6 Pro / duchamp |
| vendorID | 0x13b5 |
| deviceID | 0xb8a31030 |
| gpuId | 0xb8a31030 |
| apiVersion | 1.4.363 |
| instanceExtensionCount | 12 |
| deviceExtensionCount | 166 |
| totalExtensionCount | 178 |
| mesaCommit | `5a07217f034b3e50d8c7c7794f97a2df1742613b` |
| kbaseUapi | 1.21 |

Live dump counts supersede the beta.1 8/160 figures (WSI instance extensions are now advertised).

## Exposed vs tested (beta.2)

`runtimeTested=true` only when a real gate exists:

| Capability | upstream | exposed | tested | workload |
|---|---|---|---|---|
| computeShader | Y | Y | Y | Gate E compute 10/10 |
| vertexShader | Y | Y | Y | Gate F / WSI triangle |
| fragmentShader | Y | Y | Y | Gate F / WSI triangle |
| bufferAllocation | Y | Y | Y | Gate D/E mapped buffer |
| ahbImport | Y | Y | Y | Gate G AHB import |
| colorAttachmentRendering | Y | Y | Y | Gate F + swapchain |
| binarySemaphore | Y | Y | Y | sync_test + WSI acquire/present |
| fence | Y | Y | Y | sync_test + WSI in-flight fences |
| swapchainPresentation | Y | Y | Y | Gate H 300/300 present |
| VK_KHR_swapchain | Y | Y | Y | Gate H |
| VK_KHR_android_surface | Y | Y | Y | Gate H surface create |
| externalMemory | Y | Y | Y | AHB / dma-buf import |
| externalSemaphore | Y | Y | Y | WSI binary semaphores |
| robustBufferAccess | Y | Y | N | bit only |
| wideLines | Y | Y | N | bit only |
| largePoints | Y | Y | N | bit only |
| samplerAnisotropy | Y | Y | N | bit only |
| textureCompressionETC2 | Y | Y | N | bit only |
| textureCompressionASTC_LDR | Y | Y | N | bit only |
| descriptorIndexing | Y | Y | N | bit only |
| timelineSemaphore | Y | Y | N | bit only |
| dynamicRendering | Y | Y | N | bit only |
| synchronization2 | Y | Y | N | bit only |
| bufferDeviceAddress | Y | Y | N | bit only |
| VK_KHR_push_descriptor | Y | Y | N | extension listed only |
| geometryShader | N | N | N | not advertised |
| tessellationShader | N | N | N | not advertised |
| multiViewport | N | N | N | not advertised |
| shaderClipDistance | N | N | N | not advertised |
| shaderCullDistance | N | N | N | not advertised |
| shaderFloat64 | N | N | N | not advertised |
| depthBounds | N | N | N | not advertised |
| textureCompressionBC | Y (probe) | N | N | G615 BC formats not exposed |

Do not treat a true feature bit as a pass.

## Instance extensions (12, live dump)

1. `VK_KHR_android_surface`
2. `VK_KHR_device_group_creation`
3. `VK_KHR_external_fence_capabilities`
4. `VK_KHR_external_memory_capabilities`
5. `VK_KHR_external_semaphore_capabilities`
6. `VK_KHR_get_physical_device_properties2`
7. `VK_KHR_get_surface_capabilities2`
8. `VK_KHR_surface`
9. `VK_EXT_debug_report`
10. `VK_EXT_debug_utils`
11. `VK_EXT_headless_surface`
12. `VK_EXT_surface_maintenance1`

Full device-extension list (166) is in `dist/runtime-feature-matrix.json`.
