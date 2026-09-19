# PanVK Kbase Android Driver

Standalone patch/build layer around pinned upstream Mesa that produces an
open Mesa PanVK driver talking directly to Android's proprietary
`mali_kbase` kernel interface (`/dev/mali0`).

Target repository name: `JICA98/panvk-kbase-android`

Primary consumers:

1. **Samba S3** — Android/Bionic, in-process custom Vulkan driver
2. **Bachata S4** — ARM64 glibc Vulkan ICD inside the managed runtime
3. **NativeCode AI** — ARM64 glibc PRoot validation/development

Secondary (only after primary driver is correct): Winlator /
AdrenoTools-style loaders, GameHub / component injectors, other Android apps
capable of loading an alternate Vulkan ICD.

This is NOT a Samba-specific or Winlator-specific fork. One driver
source/patch stack, adapter packages around it.

## Reference device

- Phone: Poco X6 Pro (2311DRK48I, `duchamp`)
- SoC: MediaTek Dimensity 8300-Ultra (MT6897)
- GPU: Mali-G615 MC6, Pan arch v11, CSF frontend
- Kernel interface: `/dev/mali0` (`mali_kbase`)
- Observed GPU ID string: `Mali-G615 6 cores r1p3 0xB8A3`

## Layout

```text
sources.lock          exact Mesa commit + reference repos (no moving branches)
profiles/             per-GPU Kbase profiles (arch/gpu-id/frontend/uapi)
patches/              qualified patch families (never one unqualified blob)
meson/                cross files (android-aarch64, linux-aarch64-native)
scripts/              fetch / patch / build / package / validate / release
tests/                kbase-probe, vulkan-smoke, compute, offscreen, ahb,
                      android-surface, sync, android-loader-app
docs/                 architecture, build, portability, matrix, profiles,
                      app-compat, release
.github/workflows/   build / release / source-drift
```

## Quick start (Poco X6 Pro)

See `docs/BUILD-POCO-X6-PRO.md`.

```sh
./scripts/fetch-mesa.sh
./scripts/apply-patches.sh --profile g615-v11-csf
./scripts/build-android.sh --profile g615-v11-csf
./scripts/validate-binary.sh --abi android dist/android-g615-v11-csf/libvulkan_panfrost.so
./scripts/package-android-adpkg.sh --profile g615-v11-csf
```

## Release policy

Tiers: `dev` (build only) < `alpha` (compute/offscreen) < `beta` (WSI +
app-loader on one profile) < `rc` (primary consumer) < `stable` (multi-device
+ soak). First Poco build is never `stable`. Releases are immutable; a patch
change creates a new release even if the Mesa SHA is unchanged.

## Capability truth

Two separate sets are stored per release: `UPSTREAM_MATRIX_CAPABILITIES`
(from `docs/features.txt` of the exact Mesa checkout) and
`RUNTIME_DEVICE_CAPABILITIES` (from on-device `vulkaninfo`/probe). Only
runtime-tested capabilities may be used for compatibility claims. No fake
feature bits.

## Runtime Features & Extensions (Mali-G615 MC6 / CSF uAPI 1.21)

Full specification and per-capability test breakdown in [`docs/RUNTIME-FEATURES.md`](docs/RUNTIME-FEATURES.md). Machine-readable dump: `dist/runtime-feature-matrix.json`.

- **Vulkan API Version**: `1.4.363`
- **Total Extensions Exposed**: `178` (12 Instance, 166 Device)
- **Driver**: Mesa `26.3.0-devel` (Commit `5a07217f034b3e50d8c7c7794f97a2df1742613b`)
- **GPU**: Mali-G615 MC6 (`0xb8a31030`)
- **Kernel Interface**: `mali_kbase` (CSF uAPI 1.21, `/dev/mali0`)
- **Patch Series ID**: `sha256:b0573b926c81fb19e2ea67a9ab2465e3dcea4e8f0660db0012f8c98cce31bd4f`

### Exposed vs Tested Feature Matrix

Only runtime-exercised features are claimed as verified. Feature bits set to true without dedicated test workloads are marked exposed-only.

| Feature / Capability | Upstream | Exposed | Runtime Tested | Workload / Verification |
|---|:---:|:---:|:---:|---|
| `computeShader` | **YES** | **YES** | **PASS** | Gate E 10/10 iterations (exact `0x12345678`) |
| `vertexShader` | **YES** | **YES** | **PASS** | Gate F offscreen & Gate H swapchain triangle |
| `fragmentShader` | **YES** | **YES** | **PASS** | Gate F offscreen & Gate H swapchain triangle |
| `bufferAllocation` | **YES** | **YES** | **PASS** | Gate D device & Gate E host-visible mapped buffer |
| `ahbImport` | **YES** | **YES** | **PASS** | Gate G AHB import & CPU readback (`16384` B) |
| `colorAttachmentRendering` | **YES** | **YES** | **PASS** | Gate F offscreen & Gate H WSI swapchain |
| `binarySemaphore` | **YES** | **YES** | **PASS** | P10 submit chaining & WSI acquire/present |
| `fence` | **YES** | **YES** | **PASS** | P10 queue fence wait/reset & WSI frames |
| `swapchainPresentation` | **YES** | **YES** | **PASS** | Gate H 300/300 frames on `ANativeWindow` |
| `VK_KHR_swapchain` | **YES** | **YES** | **PASS** | Real Vulkan swapchain acquire/submit/present |
| `VK_KHR_android_surface` | **YES** | **YES** | **PASS** | Real Vulkan Android surface via ICD GIPA |
| `externalMemory` | **YES** | **YES** | **PASS** | AHB allocation & DMA-buf import |
| `externalSemaphore` | **YES** | **YES** | **PASS** | WSI binary semaphore acquire/present wait |
| `descriptorIndexing` | **YES** | **YES** | *untested* | Feature bit exposed |
| `timelineSemaphore` | **YES** | **YES** | *untested* | Feature bit exposed |
| `bufferDeviceAddress` | **YES** | **YES** | *untested* | Feature bit exposed |
| `dynamicRendering` | **YES** | **YES** | *untested* | Feature bit exposed |
| `synchronization2` | **YES** | **YES** | *untested* | Feature bit exposed |
| `inlineUniformBlock` | **YES** | **YES** | *untested* | Feature bit exposed |
| `maintenance4` | **YES** | **YES** | *untested* | Feature bit exposed |
| `robustBufferAccess` | **YES** | **YES** | *untested* | Feature bit exposed |
| `samplerAnisotropy` | **YES** | **YES** | *untested* | Feature bit exposed |
| `textureCompressionETC2` | **YES** | **YES** | *untested* | Feature bit exposed |
| `textureCompressionASTC_LDR` | **YES** | **YES** | *untested* | Feature bit exposed |
| `shaderInt64` / `shaderInt16` | **YES** | **YES** | *untested* | Feature bit exposed |
| `geometryShader` | **NO** | **NO** | **NO** | Valhall v11 CSF unexposed |
| `tessellationShader` | **NO** | **NO** | **NO** | Valhall v11 CSF unexposed |
| `multiViewport` | **NO** | **NO** | **NO** | Not supported |
| `shaderFloat64` | **NO** | **NO** | **NO** | Not supported |
| `textureCompressionBC` | **NO** | **NO** | **NO** | G615 BC formats not exposed |

<details>
<summary><b>Click to expand full list of 178 extensions (12 instance + 166 device)</b></summary>

#### Instance Extensions (12)
- `VK_EXT_debug_report`
- `VK_EXT_debug_utils`
- `VK_EXT_headless_surface`
- `VK_EXT_surface_maintenance1`
- `VK_KHR_android_surface`
- `VK_KHR_device_group_creation`
- `VK_KHR_external_fence_capabilities`
- `VK_KHR_external_memory_capabilities`
- `VK_KHR_external_semaphore_capabilities`
- `VK_KHR_get_physical_device_properties2`
- `VK_KHR_get_surface_capabilities2`
- `VK_KHR_surface`

#### Device Extensions (166)
`VK_ANDROID_external_memory_android_hardware_buffer`, `VK_ANDROID_native_buffer`, `VK_ARM_rasterization_order_attachment_access`, `VK_ARM_scheduling_controls`, `VK_ARM_shader_core_builtins`, `VK_ARM_shader_core_properties`, `VK_EXT_4444_formats`, `VK_EXT_astc_decode_mode`, `VK_EXT_attachment_feedback_loop_dynamic_state`, `VK_EXT_attachment_feedback_loop_layout`, `VK_EXT_border_color_swizzle`, `VK_EXT_buffer_device_address`, `VK_EXT_calibrated_timestamps`, `VK_EXT_color_write_enable`, `VK_EXT_conditional_rendering`, `VK_EXT_conservative_rasterization`, `VK_EXT_custom_border_color`, `VK_EXT_debug_marker`, `VK_EXT_depth_bias_control`, `VK_EXT_depth_clamp_control`, `VK_EXT_depth_clamp_zero_one`, `VK_EXT_depth_clip_control`, `VK_EXT_depth_clip_enable`, `VK_EXT_descriptor_indexing`, `VK_EXT_device_address_binding_report`, `VK_EXT_device_memory_report`, `VK_EXT_dynamic_rendering_unused_attachments`, `VK_EXT_extended_dynamic_state`, `VK_EXT_extended_dynamic_state2`, `VK_EXT_extended_dynamic_state3`, `VK_EXT_external_memory_acquire_unmodified`, `VK_EXT_external_memory_dma_buf`, `VK_EXT_global_priority`, `VK_EXT_global_priority_query`, `VK_EXT_graphics_pipeline_library`, `VK_EXT_hdr_metadata`, `VK_EXT_host_image_copy`, `VK_EXT_host_query_reset`, `VK_EXT_image_2d_view_of_3d`, `VK_EXT_image_drm_format_modifier`, `VK_EXT_image_robustness`, `VK_EXT_image_sliced_view_of_3d`, `VK_EXT_image_view_min_lod`, `VK_EXT_index_type_uint8`, `VK_EXT_inline_uniform_block`, `VK_EXT_legacy_dithering`, `VK_EXT_line_rasterization`, `VK_EXT_load_store_op_none`, `VK_EXT_map_memory_placed`, `VK_EXT_memory_budget`, `VK_EXT_multisampled_render_to_single_sampled`, `VK_EXT_mutable_descriptor_type`, `VK_EXT_nested_command_buffer`, `VK_EXT_non_seamless_cube_map`, `VK_EXT_physical_device_drm`, `VK_EXT_pipeline_creation_cache_control`, `VK_EXT_pipeline_creation_feedback`, `VK_EXT_pipeline_robustness`, `VK_EXT_present_timing`, `VK_EXT_primitive_topology_list_restart`, `VK_EXT_private_data`, `VK_EXT_provoking_vertex`, `VK_EXT_queue_family_foreign`, `VK_EXT_rasterization_order_attachment_access`, `VK_EXT_rgba10x6_formats`, `VK_EXT_robustness2`, `VK_EXT_sampler_filter_minmax`, `VK_EXT_scalar_block_layout`, `VK_EXT_separate_stencil_usage`, `VK_EXT_shader_atomic_float`, `VK_EXT_shader_demote_to_helper_invocation`, `VK_EXT_shader_image_atomic_int64`, `VK_EXT_shader_module_identifier`, `VK_EXT_shader_replicated_composites`, `VK_EXT_shader_stencil_export`, `VK_EXT_shader_subgroup_ballot`, `VK_EXT_shader_subgroup_vote`, `VK_EXT_shader_tile_image`, `VK_EXT_shader_uniform_buffer_unsized_array`, `VK_EXT_subgroup_size_control`, `VK_EXT_swapchain_maintenance1`, `VK_EXT_texel_buffer_alignment`, `VK_EXT_texture_compression_astc_hdr`, `VK_EXT_tooling_info`, `VK_EXT_vertex_attribute_divisor`, `VK_EXT_vertex_input_dynamic_state`, `VK_EXT_ycbcr_2plane_444_formats`, `VK_EXT_ycbcr_image_arrays`, `VK_EXT_zero_initialize_device_memory`, `VK_GOOGLE_decorate_string`, `VK_GOOGLE_hlsl_functionality1`, `VK_KHR_16bit_storage`, `VK_KHR_8bit_storage`, `VK_KHR_bind_memory2`, `VK_KHR_buffer_device_address`, `VK_KHR_calibrated_timestamps`, `VK_KHR_cooperative_matrix`, `VK_KHR_copy_commands2`, `VK_KHR_create_renderpass2`, `VK_KHR_dedicated_allocation`, `VK_KHR_depth_stencil_resolve`, `VK_KHR_descriptor_update_template`, `VK_KHR_device_group`, `VK_KHR_draw_indirect_count`, `VK_KHR_driver_properties`, `VK_KHR_dynamic_rendering`, `VK_KHR_dynamic_rendering_local_read`, `VK_KHR_external_fence`, `VK_KHR_external_fence_fd`, `VK_KHR_external_memory`, `VK_KHR_external_memory_fd`, `VK_KHR_external_semaphore`, `VK_KHR_external_semaphore_fd`, `VK_KHR_format_feature_flags2`, `VK_KHR_get_memory_requirements2`, `VK_KHR_global_priority`, `VK_KHR_image_format_list`, `VK_KHR_imageless_framebuffer`, `VK_KHR_index_type_uint8`, `VK_KHR_line_rasterization`, `VK_KHR_load_store_op_none`, `VK_KHR_maintenance1`, `VK_KHR_maintenance2`, `VK_KHR_maintenance3`, `VK_KHR_maintenance4`, `VK_KHR_maintenance5`, `VK_KHR_maintenance6`, `VK_KHR_map_memory2`, `VK_KHR_multiview`, `VK_KHR_pipeline_executable_properties`, `VK_KHR_pipeline_library`, `VK_KHR_present_id`, `VK_KHR_present_wait`, `VK_KHR_push_descriptor`, `VK_KHR_relaxed_block_layout`, `VK_KHR_sampler_mirror_clamp_to_edge`, `VK_KHR_sampler_ycbcr_conversion`, `VK_KHR_separate_depth_stencil_layouts`, `VK_KHR_shader_atomic_int64`, `VK_KHR_shader_clock`, `VK_KHR_shader_draw_parameters`, `VK_KHR_shader_expect_assume`, `VK_KHR_shader_float16_int8`, `VK_KHR_shader_float_controls`, `VK_KHR_shader_float_controls2`, `VK_KHR_shader_integer_dot_product`, `VK_KHR_shader_maximal_reconvergence`, `VK_KHR_shader_non_semantic_info`, `VK_KHR_shader_quad_control`, `VK_KHR_shader_subgroup_extended_types`, `VK_KHR_shader_subgroup_rotate`, `VK_KHR_shader_subgroup_uniform_control_flow`, `VK_KHR_shader_terminate_invocation`, `VK_KHR_spirv_1_4`, `VK_KHR_storage_buffer_storage_class`, `VK_KHR_swapchain`, `VK_KHR_swapchain_mutable_format`, `VK_KHR_synchronization2`, `VK_KHR_timeline_semaphore`, `VK_KHR_uniform_buffer_standard_layout`, `VK_KHR_variable_pointers`, `VK_KHR_vertex_attribute_divisor`, `VK_KHR_vulkan_memory_model`, `VK_KHR_workgroup_memory_explicit_layout`, `VK_KHR_zero_initialize_workgroup_memory`, `VK_VALVE_mutable_descriptor_type`
</details>

## License

Mesa code remains under its upstream licenses (see `LICENSES/` and
`NOTICE.md`). Build/patch/test scaffolding in this repository is MIT unless
otherwise noted.
