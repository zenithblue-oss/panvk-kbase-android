# FTL run: Galaxy S9 (starlte, API 29), RC5 only, PanProbe v7M, game-loop scenario 2

- Project: phonefinder-hub; matrix matrix-1mzjw1vxcyhvq; outcome Passed (harness level), test time 313 s
- Bucket: gs://test-lab-hrb456s6qt79m-kz4jkut67musi/panvk-v6-starlte-v7M/
- Console: https://console.firebase.google.com/project/phonefinder-hub/testlab/histories/bh.cfd8dbd2f134833a/matrices/7834756072092350305
- Driver: PanVK-kbase RC5-dev-v7M (Mesa 26.3.0-devel git-bd6d38d376)
- gpu_id 0x62210010, arch v6, Mali-G72 (Exynos 9810)
- Files: panvk-v6-starlte-v7M/starlte-29-en-portrait/{artifacts (autorun.txt, runs/20261021-230339/), logcat, game_loop_results, video.mp4}. The 43 MB panprobe-v7M.apk that gsutil also pulled was deleted (over the 20 MB limit).

## Totals
AUTORUN DONE pass=35 fail=3 skip=0 total=38. Previous v7K run (2026-10-22-ftl-v6-starlte/results.md): 33/38 (5 fail). Net +2.
Compliance (DXVK items=94, Bachata S4 items=114, vkd3d items=100) all PASS, missingHard=[].

## Status of the 5 previous failures
| test | v7K | v7M |
|---|---|---|
| vertex_stores | FAIL | PASS (all 11 cases incl. iter0-5, indexed_dup, restart, instanced, firstvertex, vs_img_redraw; VERTEX_STORES_FAILS=0) |
| robust_image_access2 | FAIL (10 cases, alpha 0 vs 1) | PASS (83 PASS cases, 0 FAIL; ria2 34.01 ms vs ria1 33.02 ms, +3.0%) |
| shader_arith | FAIL | FAIL (same: subgroup_size_control DEVICE_LOST) |
| sampler | FAIL | FAIL (same: gather_offsets) |
| vkd3d_heap | FAIL | FAIL (same: image_heap) |

All other 33 tests PASS (gpu_prerast_slice, clip_cull, multi_viewport, fill_mode, bc_decode, bc_perf, geometry, tessellation, xfb, pipeline_stats, gs_viewport_depth, vs_viewport_index, depth_bounds, large_draw, vmr_secondary, tess_cond_state, bachata_reqs, bachata_exec, robustness2, blend, occlusion_query, descriptor_model, depth_stencil, draw_params, submit_stress, csf_event, gs_tess_primitive_id, bachata_storage_fmtless, bachata_dynamic_render, dxvk_reqs, vkd3d_reqs, vkd3d_timeline, swapchain_lifecycle, plus the two above). swapchain_lifecycle 59.9 fps.

## FAIL lines (autorun.txt)
- `RESULT shader_arith FAIL mismatch=0 fps=0 ms=4218 extra=Bifrost (v7): feature not implemented yet`
- `RESULT sampler FAIL mismatch=0 fps=0 ms=4224 extra=Bifrost (v7): feature not implemented yet`
- `RESULT vkd3d_heap FAIL mismatch=0 fps=0 ms=10643 extra=Bifrost (v7): feature not implemented yet`
SKIP: none.

(The "Bifrost (v7): feature not implemented yet" text is the harness's generic extra; the real cause is below. These are the same three failures as v7K.)

## Per-case excerpts (runs/20261021-230339/*.log)
- shader_arith (INFO features: int8=1 int16=1 int64=1 storage8=1 storage16=1 demote=1 subgroup_size_control=1 compute_full_subgroups=1 zero_init_wg=1 frag_stores=1 subgroupSize=4 minSubgroupSize=4 maxSubgroupSize=4)
  - PASS case int8, int16, int64, storage8, storage16, demote
  - `MESA: error: kbase: JM atom 9 failed: event=0x51 core_req=0x16`
  - `FAIL case subgroup_size_control DEVICE_LOST`
- sampler
  - PASS anisotropy (lod_iso=3.00 lod_aniso2=2.00 lod_aniso16=0.00), mirror_clamp_to_edge, border_opaque_white, border_custom, border_custom_no_format, cube_array, cube_seamless (r0=10 r1=40), cube_non_seamless (r0=10 r1=10), gather
  - `MESA: error: kbase: JM atom 46 failed: event=0x51 core_req=0x16`
  - `FAIL compute wait r=-4`
  - `FAIL case gather_offsets mismatch sample 0 comp 0 got (-255,-255) exp (18,41)`
- vkd3d_heap
  - PASS ssbo_heap, texel_heap, mutable_heap
  - `MESA: error: kbase: JM atom 5 failed: event=0x51 core_req=0x16`
  - `FAIL case image_heap vkWaitForFences r=-4`

All three fail the same way: a JM compute atom (core_req=0x16) ends with event=0x51, then the fence/wait returns -4 (device lost).

## Logcat (logcat, 64122 lines)
- DEVICE_LOST: only shader_arith subgroup_size_control (logcat lines 56850 TAIL and 59941 JSON echo)
- JM atom failures: exactly the three above (atom 9, 46, 5; each in the process log, TAIL and JSON echo)
- GPU/data/job faults, MMU faults, fatal signals, SIGSEGV: none
- dlopen: no dlopen failure of the driver. One linker E line: libvulkan_panfrost_rc5.so needs /system/lib64/libvndksupport.so, "not accessible for the namespace classloader-namespace", followed by `[P0A-V19-FULLPLANE] mapper load failed (1 candidate names, first=mediatek)`, `complete metadata unavailable rc=-95; refusing guessed layout`, `u_gralloc_get_buffer_basic_info failed` (swapchain/gralloc path, logcat line 59133; swapchain_lifecycle still PASS)
- Informational in every process: `kbase: Bifrost JM atom strides 56 and 64 refused, stride 48 (base_jd_atom_v2 without renderpass_id)`

## Screenshots
Test Lab produced no stills (no artifacts/screenshots). ffmpeg is not installed here (not on PATH, not in apk repos, no pip, no cv2), so no frames were extracted. Video (1.6 MB, kept): [video.mp4](panvk-v6-starlte-v7M/starlte-29-en-portrait/video.mp4). Screenshots dir: none.
