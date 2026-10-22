# A06 (Mali-G52, v7) v7K device run, 2026-10-22

APK panprobe-v7K.apk (sha256 c8231e008e36794a...), libvulkan_panfrost_rc5.so sha256 badc28eb31786c33...
Driver string: PanVK-kbase RC5-dev-v7K (Mesa 26.3.0-devel git-d160802965). Package dev.zenithblue.panvktest.v7k.
Install: --no-streaming -r, 206 s, Success.

## Totals

AUTORUN DONE pass=38 fail=0 skip=0 total=38 (v7J: 37/0/1, swapchain_lifecycle SKIP).
swapchain_lifecycle is now PASS: fps 45.6, resized FPS 56.5, ms=15634
(present_main 6577ms, resize 8ms, present_resized 2122ms, destroy 1ms, cycle 1732ms, teardown 24ms).

No FAIL or SKIP, so no per-test toggle reruns were needed.

## Per-test (all PASS, mismatch=0), ms

gpu_prerast_slice 4256, clip_cull 5303 (2372.7 fps), multi_viewport 5209 (2705.1 fps), fill_mode 7384 (790.8 fps),
bc_decode 5223, bc_perf 63982, geometry 6654 (1942.5 fps), tessellation 12365 (2419.8 fps), xfb 6454,
pipeline_stats 5467 (1343.0 fps), vertex_stores 4170, gs_viewport_depth 4189, vs_viewport_index 4957,
depth_bounds 4411, large_draw 21864, vmr_secondary 4139, tess_cond_state 4277, bachata_reqs 4093,
bachata_exec 4159, robustness2 4154, robust_image_access2 7039, blend 4208, occlusion_query 4144,
descriptor_model 4124, shader_arith 4219, depth_stencil 4191, sampler 4223, draw_params 4173,
submit_stress 34129, csf_event 4709, gs_tess_primitive_id 5534, bachata_storage_fmtless 4159,
bachata_dynamic_render 4114, dxvk_reqs 2087, vkd3d_reqs 2094, vkd3d_heap 10359, vkd3d_timeline 4237,
swapchain_lifecycle 15634 (45.6 fps).

## Perf (full-run, default env)

robust_image_access2: ria2=169.49ms ria1=169.02ms delta=+0.3%
Shader stats ria2/ria1: Instructions 304/210, Tuples 233/161, Clauses 43/28, Registers 17/13, Code size 3136/2272, no spills.
pipeline_stats: 1343.0 fps.

## Perf A/B: NOT RUN

The device dropped (`device 'localhost:54520' not found`) when launching the first A/B autorun, right after the full run
and the pull of files/autorun.txt and files/runs. Per the connection-safety rules no reconnect was attempted.
Neither A/B run (default env vs PANVK_V7_DESC_REORDER=0) was executed. Only the full-run ria numbers above exist.

## Telemetry (host-side)

- Before full run: battery 100%, not charging, 35.5 C, thermal NONE, avail mem 5433/11541 MB.
- After full run: battery 100%, not charging, 33.4 C, thermal NONE, avail mem 4963/11541 MB.
- Not taken before A/B runs (not started).

## Files

autorun_full.txt, runs/20261022-031916/ (logs + json per test).
