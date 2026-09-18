# Mesa Matrix (upstream truth, not runtime truth)

Source of truth: `docs/features.txt` of the exact Mesa checkout, captured by
`scripts/capture-matrix.py` into `dist/<build>/upstream-feature-matrix.{json,md}`.

Baseline 2026-09-18: PanVK Vulkan 1.4, 210/303 extensions, 69.3% coverage;
1.0/1.2/1.3/1.4 version rows complete upstream. Recent entries include
maintenance5-8, shader_constant_data, shader_atomic_int64,
descriptor_indexing, maximal_reconvergence, quad_control,
subgroup_uniform_control_flow, astc_decode_mode, calibrated_timestamps,
present_timing.

Device runtime matrix (`runtime-feature-matrix.json` from on-device
vulkaninfo/probe) is stored separately. Release manifests distinguish
`matrix_implemented / runtime_exposed / runtime_tested`. Emulator-relevant
captures: apiVersion, robustBufferAccess, geometry/tessellation shaders,
multiViewport, clip/cull distance, float64, depthBounds, wideLines,
largePoints, anisotropy, BC/ETC2/ASTC_LDR, descriptorIndexing,
timelineSemaphore, dynamicRendering, synchronization2, bufferDeviceAddress,
pushDescriptor, swapchain, android_surface, external memory/semaphore.
Feature states: `native / emulated / lowered / unsupported` — never set true
to satisfy an app.
