# A06 (Mali-G52 r1, v7 Bifrost JM) PanProbe runs, 2026-10-21

Device: Samsung Galaxy A06 via RTL, adb `localhost:54520`
Fingerprint: `samsung/a06xx/a06:14/UP1A.231005.007/A065FXXS4AYE2:user/release-keys`
PanVK ran in every run: logcat shows `panvk: gpu_id 0x74021000 variant 0x2 arch v7 model Mali-G52 r1`.

| Run | APK / pkg | Driver string | pass | fail | skip |
|---|---|---|---|---|---|
| RC4 | panprobe-v7R.apk, `--es driver rc4` | RC4 (git-5a07217f03) | 14 | 19 | 5 |
| v7R | panprobe-v7R.apk, `--es driver rc5` | RC5-dev-v7R (git-5a07217f03) | 14 | 19 | 5 |
| v7E | panprobe-v7E.apk, `--es driver rc5` | RC5-dev-v7E (git-6489edf524) | 16 | 17 | 5 |
| v7r30 | not run: each APK push takes about 4.5 min at 0.2 MB/s over RTL | - | - | - | - |

Files: `v7R-rc4.log`, `v7R-rc5.log`, `v7E-rc5.log` (logcat for PanProbe and MESA),
`v7r-runs/` (rc4 and rc5 runs), `v7e-runs/20261021-170105/` (per-test logs and json),
`v7E-failure-excerpts.txt` (FAIL/SKIP lines from every failing test).

## Per-test results

| test | RC4 | v7R | v7E |
|---|---|---|---|
| gpu_prerast_slice | PASS | PASS | PASS |
| clip_cull | FAIL CreateDevice -8 | FAIL CreateDevice -8 | **PASS** |
| multi_viewport | FAIL CreateDevice -8 | FAIL CreateDevice -8 | **PASS** |
| fill_mode | FAIL CreateDevice -8 | FAIL CreateDevice -8 | FAIL FILL_MODE_FAILS=4 |
| bc_decode | PASS | PASS | PASS |
| bc_perf | FAIL Query r=-4 | FAIL Wait r=-4 | FAIL Wait r=-4 |
| geometry | FAIL CreateDevice -8 | FAIL CreateDevice -8 | FAIL GEOMETRY_FAILS=3 |
| tessellation | FAIL not exposed | FAIL not exposed | FAIL 4 cases + Wait r=-4 |
| xfb | FAIL not exposed | FAIL not exposed | FAIL WaitForFences r=-4 (5 cases pass first) |
| pipeline_stats | FAIL CreateDevice -8 | FAIL CreateDevice -8 | FAIL fs=0 + Wait r=-4 |
| vertex_stores | SKIP | SKIP | SKIP |
| gs_viewport_depth | FAIL feature missing | FAIL feature missing | FAIL wrong depth |
| vs_viewport_index | FAIL feature missing | FAIL feature missing | FAIL pipeline r=-13 |
| depth_bounds | FAIL | FAIL | FAIL |
| large_draw | FAIL xfb not exposed | FAIL xfb not exposed | FAIL GPU faults |
| vmr_secondary | SKIP | SKIP | SKIP |
| tess_cond_state | FAIL CreateDevice -7 | FAIL CreateDevice -7 | FAIL CreateDevice -7 |
| bachata_reqs | FAIL | FAIL | FAIL |
| bachata_exec | PASS | PASS | PASS |
| robustness2 | SKIP | SKIP | FAIL (newly enabled) |
| robust_image_access2 | FAIL | FAIL | SKIP |
| blend | PASS | PASS | PASS |
| occlusion_query | FAIL Wait r=-4 | FAIL Wait r=-4 | FAIL Wait r=-4 |
| descriptor_model | PASS | PASS | PASS |
| shader_arith | PASS | PASS | PASS |
| depth_stencil | PASS | PASS | PASS |
| sampler | PASS | PASS | PASS |
| draw_params | SKIP | SKIP | SKIP |
| submit_stress | PASS | PASS | PASS |
| csf_event | PASS | PASS | PASS |
| gs_tess_primitive_id | SKIP | SKIP | SKIP |
| bachata_storage_fmtless | PASS | PASS | PASS |
| bachata_dynamic_render | PASS | PASS | PASS |
| dxvk_reqs | FAIL | FAIL | FAIL |
| vkd3d_reqs | FAIL | FAIL | FAIL |
| vkd3d_heap | FAIL | FAIL | FAIL |
| vkd3d_timeline | PASS | PASS | PASS |
| swapchain_lifecycle | PASS | PASS | PASS |

v7R matches RC4 exactly; the only difference is the bc_perf failure point, Query vs Wait.
v7E gains clip_cull and multi_viewport. robustness2 and robust_image_access2 swap places
(robustness2 is now exposed and fails; robustImageAccess2 is no longer advertised, so it skips).

## v7E (RC5 base) failure details

These are grouped by likely root cause. The full excerpts are in `v7E-failure-excerpts.txt`.

### A. GPU job faults: kbase JM atoms fail with event 0x4 / 0x42 (core_req 0x16) and 0x51 / 0x58 / 0x5b (core_req 0x1)
This one cause breaks bc_perf, occlusion_query, pipeline_stats, large_draw, xfb (after 5 cases pass),
tessellation (after the grid cases) and depth_bounds.
- Event codes (from memory of the kbase `BASE_JD_EVENT_*` / GPU exception list; check them against the uapi header before relying on them): 0x04 is TERMINATED (an atom killed because an earlier atom in the chain faulted), 0x42 is JOB_READ_FAULT, 0x51 is INSTR_INVALID_ENC, 0x58 is DATA_INVALID_FAULT, and 0x5b is unknown.
- core_req 0x16 is the CS|T vertex/tiler chain. The first atoms of a process already fail in bc_perf, occlusion_query and pipeline_stats, which points at query-related work (occlusion / pipeline-stat / timestamp) doing an invalid read (0x42), not at the draw itself.
- core_req 0x1 is FS. In depth_bounds, fragment jobs fault with 0x51 (invalid shader encoding) and 0x58 (invalid descriptor). The depth-bounds fragment shader / descriptor on v7 is malformed: A_static gives wrong results (badColor=288, badDepth=1024) and cases B..H time out.
- pipeline_stats: `fs=0 (want>=290)`, so the fragment-invocations statistic is not written on v7.

### B. Primitive restart is wrong for strip/fan topologies (fill_mode, geometry)
- fill_mode: strip_restart_line, fan_restart_line_cull_front, fan_restart_line_cull_back and indirect_indexed_strip_restart_line draw extra red (polygonMode LINE with restart: the restart index is not honoured in the line-fill path).
- geometry: tri_strip_restart_pass, tri_strip_restart_indexed_indirect and primid_strip_restart give black where blue is expected (the GS input assembly ignores the restart).

### C. Tessellation (v7E exposes it, partially working)
- grid_64x48_l16: px=682 of 3072
- grid_indexed_inst2_direct_first1 / _indirect_first1: px=682 of 960 (firstInstance / instance offset)
- grid_tes_to_gs_l16: px=16 of 3072
- The process then faults (atom 19, event 0x4) and fails with `Wait r=-4 line=239`. Large tessellated grids hit a limit (buffer or heap overflow).
- point_mode_l4 is skipped because shaderTessellationAndGeometryPointSize is not exposed.

### D. Viewport / depth from GS and VS
- gs_viewport_depth: depth is constant 0.375 / 0.0 instead of per-viewport expL / expR, so the per-viewport depth range is not applied when the GS picks the viewport.
- vs_viewport_index: shaderOutputViewportIndex is not reported, and CreateGraphicsPipelines returns -13 (VK_ERROR_UNKNOWN).

### E. Device creation / features
- tess_cond_state: CreateDevice r=-7 (VK_ERROR_EXTENSION_NOT_PRESENT), probably conditional_rendering or extended_dynamic_state3.
- bachata_reqs: hard misses are shaderBufferInt64Atomics, shaderSharedInt64Atomics, vertexPipelineStoresAndAtomics and customBorderColorWithoutFormat. Chain CreateDevice gives -8.
- dxvk_reqs: hard misses are descriptorIndexing, descriptorBindingSampledImageUpdateAfterBind, descriptorBindingUpdateUnusedWhilePending and descriptorBindingPartiallyBound.
- vkd3d_reqs: hard misses are robustImageAccess2, descriptorIndexing and 1M UAB descriptors.
- vkd3d_heap: missing descriptorIndexing; mutableDescriptorType is unsupported.

### F. Robustness
- robustness2 null_descriptors: alpha for a null image read is 1.0 (a_img=1.0) but must be 0. A null storage-image / imageLoad has to return (0,0,0,0).
- robust_image_access2: skipped because the feature is not advertised.

### G. Skips (features not exposed)
- vertex_stores and draw_params: vertexPipelineStoresAndAtomics
- vmr_secondary: no 4x sample shading, or variableMultisampleRate without attachments
- gs_tess_primitive_id: a required feature is missing
