# Runtime Device Capabilities & Features (Mali-G615 MC6)

Direct runtime extraction from `/dev/mali0` on Poco X6 Pro (`duchamp`, Dimensity 8300-Ultra) using PanVK Kbase driver.

## Summary
- **API Version**: Vulkan 1.4.363
- **Total Extensions**: 168
  - **Instance Extensions**: 8
  - **Device Extensions**: 160
- **Driver Version**: Mesa 26.3.0-devel (Commit `5a07217f034b3e50d8c7c7794f97a2df1742613b`)
- **GPU**: Arm Mali-G615 MC6 (`0xb8a31030`)
- **Architecture**: Valhall v11 (CSF frontend)
- **Kernel Interface**: `mali_kbase` CSF uAPI 1.21

---

## Core Feature Set

| Feature Category | Feature | Status | Notes |
|---|---|---|---|
| **Shading & Compute** | `robustBufferAccess` | **YES** | Hardware bounds checking |
| | `fullDrawIndexUint32` | **YES** | 32-bit index support |
| | `imageCubeArray` | **YES** | Cube array texturing |
| | `independentBlend` | **YES** | Per-attachment blending |
| | `sampleRateShading` | **YES** | MSAA rate control |
| | `dualSrcBlend` | **YES** | Dual-source blending |
| | `logicOp` | **YES** | Framebuffer logic ops |
| | `multiDrawIndirect` | **YES** | Multi-draw indirect execution |
| | `drawIndirectFirstInstance`| **YES** | First instance offset |
| | `depthClamp` | **YES** | Depth clamping |
| | `depthBiasClamp` | **YES** | Depth bias clamping |
| | `wideLines` | **YES** | Wide line rasterization |
| | `largePoints` | **YES** | Large point rendering |
| | `samplerAnisotropy` | **YES** | Anisotropic filtering |
| | `occlusionQueryPrecise` | **YES** | Precise occlusion queries |
| | `fragmentStoresAndAtomics` | **YES** | Fragment SSBO/storage image atomics |
| | `shaderInt64` | **YES** | 64-bit integer arithmetic in shaders |
| | `shaderInt16` | **YES** | 16-bit integer arithmetic in shaders |
| | `geometryShader` | **NO** | Native geometry shaders unexposed on v11 CSF |
| | `tessellationShader` | **NO** | Native tessellation unexposed on v11 CSF |
| | `multiViewport` | **NO** | Single viewport |
| | `fillModeNonSolid` | **NO** | Solid fill mode only |
| | `shaderFloat64` | **NO** | No FP64 in hardware |
| **Texture Formats** | `textureCompressionETC2` | **YES** | Hardware ETC2/EAC decompression |
| | `textureCompressionASTC_LDR`| **YES** | Hardware ASTC LDR decompression |
| | `textureCompressionBC` | **NO** | Desktop BCn formats lowered/unsupported |
| **Modern Vulkan (1.1-1.4)** | `descriptorIndexing` | **YES** | Bindless textures and descriptor arrays |
| | `timelineSemaphore` | **YES** | Unified CPU/GPU sync primitives |
| | `bufferDeviceAddress` | **YES** | 64-bit GPU pointer addressing |
| | `scalarBlockLayout` | **YES** | std430 / scalar struct packing |
| | `hostQueryReset` | **YES** | CPU reset of query pools |
| | `vulkanMemoryModel` | **YES** | Vulkan memory model memory coherence |
| | `imagelessFramebuffer` | **YES** | Dynamic attachment binding |
| | `separateDepthStencilLayouts`| **YES** | Independent Z/S layouts |
| | `dynamicRendering` | **YES** | Render passes without VkRenderPass/VkFramebuffer |
| | `synchronization2` | **YES** | Fine-grained dependency barriers |
| | `inlineUniformBlock` | **YES** | Push-like uniform data in descriptors |
| | `maintenance4` | **YES** | Relaxed interface and compilation rules |
| **Platform Interop** | `VK_ANDROID_external_memory_android_hardware_buffer` | **YES** | Direct AHB import/export |
| | `VK_ANDROID_native_buffer` | **YES** | Android WSI swapchain integration |
| | `VK_EXT_external_memory_dma_buf` | **YES** | Linux/Android DMA-BUF import |

---

## Complete Instance Extension List (8)
1. `VK_KHR_device_group_creation` (v1)
2. `VK_KHR_external_fence_capabilities` (v1)
3. `VK_KHR_external_memory_capabilities` (v1)
4. `VK_KHR_external_semaphore_capabilities` (v1)
5. `VK_KHR_get_physical_device_properties2` (v2)
6. `VK_EXT_debug_report` (v10)
7. `VK_EXT_debug_utils` (v2)
8. `VK_EXT_headless_surface` (v1)

---

## Complete Device Extension List (160)
1. `VK_KHR_8bit_storage` (v1)
2. `VK_KHR_16bit_storage` (v1)
3. `VK_KHR_bind_memory2` (v1)
4. `VK_KHR_buffer_device_address` (v1)
5. `VK_KHR_calibrated_timestamps` (v1)
6. `VK_KHR_cooperative_matrix` (v2)
7. `VK_KHR_copy_commands2` (v1)
8. `VK_KHR_create_renderpass2` (v1)
9. `VK_KHR_dedicated_allocation` (v3)
10. `VK_KHR_depth_stencil_resolve` (v1)
11. `VK_KHR_descriptor_update_template` (v1)
12. `VK_KHR_device_group` (v4)
13. `VK_KHR_draw_indirect_count` (v1)
14. `VK_KHR_driver_properties` (v1)
15. `VK_KHR_dynamic_rendering` (v1)
16. `VK_KHR_dynamic_rendering_local_read` (v1)
17. `VK_KHR_external_fence` (v1)
18. `VK_KHR_external_fence_fd` (v1)
19. `VK_KHR_external_memory` (v1)
20. `VK_KHR_external_memory_fd` (v1)
21. `VK_KHR_external_semaphore` (v1)
22. `VK_KHR_external_semaphore_fd` (v1)
23. `VK_KHR_format_feature_flags2` (v2)
24. `VK_KHR_get_memory_requirements2` (v1)
25. `VK_KHR_global_priority` (v1)
26. `VK_KHR_image_format_list` (v1)
27. `VK_KHR_imageless_framebuffer` (v1)
28. `VK_KHR_index_type_uint8` (v1)
29. `VK_KHR_line_rasterization` (v1)
30. `VK_KHR_load_store_op_none` (v1)
31. `VK_KHR_maintenance1` (v2)
32. `VK_KHR_maintenance2` (v1)
33. `VK_KHR_maintenance3` (v1)
34. `VK_KHR_maintenance4` (v2)
35. `VK_KHR_maintenance5` (v1)
36. `VK_KHR_maintenance6` (v1)
37. `VK_KHR_map_memory2` (v1)
38. `VK_KHR_multiview` (v1)
39. `VK_KHR_pipeline_executable_properties` (v1)
40. `VK_KHR_pipeline_library` (v1)
41. `VK_KHR_push_descriptor` (v2)
42. `VK_KHR_relaxed_block_layout` (v1)
43. `VK_KHR_sampler_mirror_clamp_to_edge` (v3)
44. `VK_KHR_sampler_ycbcr_conversion` (v14)
45. `VK_KHR_separate_depth_stencil_layouts` (v1)
46. `VK_KHR_shader_atomic_int64` (v1)
47. `VK_KHR_shader_clock` (v1)
48. `VK_KHR_shader_draw_parameters` (v1)
49. `VK_KHR_shader_expect_assume` (v1)
50. `VK_KHR_shader_float16_int8` (v1)
51. `VK_KHR_shader_float_controls` (v4)
52. `VK_KHR_shader_float_controls2` (v1)
53. `VK_KHR_shader_integer_dot_product` (v1)
54. `VK_KHR_shader_maximal_reconvergence` (v1)
55. `VK_KHR_shader_non_semantic_info` (v1)
56. `VK_KHR_shader_quad_control` (v1)
57. `VK_KHR_shader_subgroup_extended_types` (v1)
58. `VK_KHR_shader_subgroup_rotate` (v2)
59. `VK_KHR_shader_subgroup_uniform_control_flow` (v1)
60. `VK_KHR_shader_terminate_invocation` (v1)
61. `VK_KHR_spirv_1_4` (v1)
62. `VK_KHR_storage_buffer_storage_class` (v1)
63. `VK_KHR_synchronization2` (v1)
64. `VK_KHR_timeline_semaphore` (v2)
65. `VK_KHR_uniform_buffer_standard_layout` (v1)
66. `VK_KHR_variable_pointers` (v1)
67. `VK_KHR_vertex_attribute_divisor` (v1)
68. `VK_KHR_vulkan_memory_model` (v3)
69. `VK_KHR_workgroup_memory_explicit_layout` (v1)
70. `VK_KHR_zero_initialize_workgroup_memory` (v1)
71. `VK_EXT_4444_formats` (v1)
72. `VK_EXT_astc_decode_mode` (v1)
73. `VK_EXT_attachment_feedback_loop_dynamic_state` (v1)
74. `VK_EXT_attachment_feedback_loop_layout` (v2)
75. `VK_EXT_border_color_swizzle` (v1)
76. `VK_EXT_buffer_device_address` (v2)
77. `VK_EXT_calibrated_timestamps` (v2)
78. `VK_EXT_color_write_enable` (v1)
79. `VK_EXT_conditional_rendering` (v2)
80. `VK_EXT_conservative_rasterization` (v1)
81. `VK_EXT_custom_border_color` (v12)
82. `VK_EXT_debug_marker` (v4)
83. `VK_EXT_depth_bias_control` (v1)
84. `VK_EXT_depth_clamp_control` (v1)
85. `VK_EXT_depth_clamp_zero_one` (v1)
86. `VK_EXT_depth_clip_control` (v1)
87. `VK_EXT_depth_clip_enable` (v1)
88. `VK_EXT_descriptor_indexing` (v2)
89. `VK_EXT_device_address_binding_report` (v1)
90. `VK_EXT_device_memory_report` (v2)
91. `VK_EXT_dynamic_rendering_unused_attachments` (v1)
92. `VK_EXT_extended_dynamic_state` (v1)
93. `VK_EXT_extended_dynamic_state2` (v1)
94. `VK_EXT_extended_dynamic_state3` (v2)
95. `VK_EXT_external_memory_acquire_unmodified` (v1)
96. `VK_EXT_external_memory_dma_buf` (v1)
97. `VK_EXT_global_priority` (v2)
98. `VK_EXT_global_priority_query` (v1)
99. `VK_EXT_graphics_pipeline_library` (v1)
100. `VK_EXT_hdr_metadata` (v3)
101. `VK_EXT_host_image_copy` (v1)
102. `VK_EXT_host_query_reset` (v1)
103. `VK_EXT_image_2d_view_of_3d` (v1)
104. `VK_EXT_image_drm_format_modifier` (v2)
105. `VK_EXT_image_robustness` (v1)
106. `VK_EXT_image_sliced_view_of_3d` (v1)
107. `VK_EXT_image_view_min_lod` (v1)
108. `VK_EXT_index_type_uint8` (v1)
109. `VK_EXT_inline_uniform_block` (v1)
110. `VK_EXT_legacy_dithering` (v2)
111. `VK_EXT_line_rasterization` (v1)
112. `VK_EXT_load_store_op_none` (v1)
113. `VK_EXT_map_memory_placed` (v1)
114. `VK_EXT_memory_budget` (v1)
115. `VK_EXT_multisampled_render_to_single_sampled` (v1)
116. `VK_EXT_mutable_descriptor_type` (v1)
117. `VK_EXT_nested_command_buffer` (v1)
118. `VK_EXT_non_seamless_cube_map` (v1)
119. `VK_EXT_physical_device_drm` (v1)
120. `VK_EXT_pipeline_creation_cache_control` (v3)
121. `VK_EXT_pipeline_creation_feedback` (v1)
122. `VK_EXT_pipeline_robustness` (v1)
123. `VK_EXT_primitive_topology_list_restart` (v1)
124. `VK_EXT_private_data` (v1)
125. `VK_EXT_provoking_vertex` (v1)
126. `VK_EXT_queue_family_foreign` (v1)
127. `VK_EXT_rasterization_order_attachment_access` (v1)
128. `VK_EXT_rgba10x6_formats` (v1)
129. `VK_EXT_robustness2` (v1)
130. `VK_EXT_sampler_filter_minmax` (v2)
131. `VK_EXT_scalar_block_layout` (v1)
132. `VK_EXT_separate_stencil_usage` (v1)
133. `VK_EXT_shader_atomic_float` (v1)
134. `VK_EXT_shader_demote_to_helper_invocation` (v1)
135. `VK_EXT_shader_image_atomic_int64` (v1)
136. `VK_EXT_shader_module_identifier` (v1)
137. `VK_EXT_shader_replicated_composites` (v1)
138. `VK_EXT_shader_stencil_export` (v1)
139. `VK_EXT_shader_subgroup_ballot` (v1)
140. `VK_EXT_shader_subgroup_vote` (v1)
141. `VK_EXT_shader_tile_image` (v1)
142. `VK_EXT_shader_uniform_buffer_unsized_array` (v1)
143. `VK_EXT_subgroup_size_control` (v2)
144. `VK_EXT_texel_buffer_alignment` (v1)
145. `VK_EXT_texture_compression_astc_hdr` (v1)
146. `VK_EXT_tooling_info` (v1)
147. `VK_EXT_vertex_attribute_divisor` (v3)
148. `VK_EXT_vertex_input_dynamic_state` (v2)
149. `VK_EXT_ycbcr_2plane_444_formats` (v1)
150. `VK_EXT_ycbcr_image_arrays` (v1)
151. `VK_EXT_zero_initialize_device_memory` (v1)
152. `VK_ANDROID_external_memory_android_hardware_buffer` (v5)
153. `VK_ANDROID_native_buffer` (v8)
154. `VK_ARM_rasterization_order_attachment_access` (v1)
155. `VK_ARM_scheduling_controls` (v2)
156. `VK_ARM_shader_core_builtins` (v2)
157. `VK_ARM_shader_core_properties` (v1)
158. `VK_GOOGLE_decorate_string` (v1)
159. `VK_GOOGLE_hlsl_functionality1` (v1)
160. `VK_VALVE_mutable_descriptor_type` (v1)
