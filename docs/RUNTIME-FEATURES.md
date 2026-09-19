# Runtime Device Capabilities (Mali-G615 MC6)

This page describes the unpublished beta.3 candidate tested on Poco X6 Pro on
2026-09-19. The beta.2 tag remains the immutable 178-extension release.

Normative file: `validation/g615-v11-csf/runtime-feature-matrix.json`.

## Status model

- **Native exposed**: reported by the base `libvulkan_panfrost.so`.
- **Native tested**: a real target-device workload passed; enumeration alone is not a test.
- **Compat/emulated**: implemented outside the base ICD and identified separately.
- **Unsupported**: not exposed by the base ICD.
- **Platform-inapplicable**: upstream desktop/display capability excluded from Android/Bionic.

The matrix has per-extension `sourceImplemented`, `runtimeExposed`,
`runtimeTested`, and `platform` fields. Emulated features add `nativeHardware`,
`nativeDriverExposed`, `nativeVulkanFeatureBit`, `compatImplementation`,
`compatAvailable`, and `compatTested`.

## Candidate capture

| Field | beta.2 tag | beta.3 candidate |
|---|---:|---:|
| Instance extensions | 12 | 13 |
| Device extensions | 166 | 181 |
| Total extensions | 178 | 194 |
| Mesa | `5a07217f034b3e50d8c7c7794f97a2df1742613b` | same pinned commit |
| Android ICD SHA-256 | `2366a5c392286553b156057f15840b22428eb501953b224ed0c215436df4f301` | `576e37de9a3b60dda9a972a6f91c3a50e09238bd31a9888e04215c7b554c803a` |

Candidate: Vulkan 1.4.363, Mali-G615 MC6 `0xb8a31030`, CSF, Kbase UAPI 1.21.

## Native tested

Phase 5 passed `descriptorIndexing`, `timelineSemaphore`, `dynamicRendering`,
`synchronization2`, `bufferDeviceAddress`, push descriptors,
`robustBufferAccess`, `samplerAnisotropy`, `wideLines`, `largePoints`, ETC2,
ASTC LDR, and ASTC HDR workloads. Beta.2 compute, graphics, allocation, AHB,
sync, Android surface, swapchain, 300-present, cold-launch, and glibc gates also
passed against the candidate.

The 16 newly exposed and tested extensions are:

`VK_GOOGLE_user_type`, `VK_KHR_compute_shader_derivatives`,
`VK_KHR_copy_memory_indirect`, `VK_KHR_internally_synchronized_queues`,
`VK_KHR_maintenance7`, `VK_KHR_maintenance8`, `VK_KHR_maintenance9`,
`VK_KHR_present_id2`, `VK_KHR_present_wait2`,
`VK_KHR_shader_constant_data`, `VK_KHR_shader_fma`,
`VK_KHR_shader_relaxed_extended_instruction`,
`VK_KHR_shader_untyped_pointers`, `VK_KHR_surface_maintenance1`,
`VK_KHR_swapchain_maintenance1`, and `VK_KHR_unified_image_layouts`.

Other enumerated extensions are **native exposed**, not automatically native tested.

## Unsupported

`geometryShader`, `tessellationShader`, `multiViewport`, `shaderClipDistance`,
`shaderCullDistance`, `shaderFloat64`, `depthBounds`, `fillModeNonSolid`, and
native `textureCompressionBC` remain false. Three source-supported KHR
extensions were deliberately disabled after required workloads were not
completed: `VK_KHR_depth_clamp_zero_one`, `VK_KHR_pipeline_binary`, and
`VK_KHR_robustness2`. `VK_GOOGLE_display_timing` remains blocked on a real
Android timing implementation.

## Compat/emulated

The experimental `bcn_layer` Android binary built, was discovered, loaded, and
intercepted 16 BC format queries through Android's Vulkan loader. That loader
selected the system Mali ICD, not staged PanVK; device creation failed before
known-block tests. Therefore `compatAvailable=true`, `compatLoadTested=true`,
`compatTested=false`, and `compatComposesWithPanvk=false`. Native BC fields and
the native Vulkan feature bit remain false. The layer is excluded from both
candidate packages and is not enabled by default.

## Platform-inapplicable

Android excludes `VK_KHR_display`, `VK_KHR_get_display_properties2`,
`VK_KHR_wayland_surface`, `VK_KHR_xcb_surface`, `VK_KHR_xlib_surface`,
`VK_EXT_acquire_drm_display`, `VK_EXT_acquire_xlib_display`,
`VK_EXT_direct_mode_display`, `VK_EXT_display_control`, and
`VK_EXT_display_surface_counter`.

Evidence: `validation/g615-v11-csf/beta3-phase5-features-2026-09-19.json`,
`beta3-phase7-extension-workloads-2026-09-19.json`,
`beta3-phase8-summary.json`, and `beta3-phase6-bcn.json` in the same directory.
