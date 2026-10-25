# FTL shiba (Pixel 8, v11, Mali-G715) RC5-universal-114

AUTORUN DONE pass=39 fail=0 skip=0. `PANPROBE_RESULT driver=PanVK-kbase RC5-universal-114 (Mesa 26.3.0-devel (git-d324a2699e)) pass=39 fail=0 skip=0`. The .so is 57018130, arch v11, Mali-G715 (gpu_id 0xb8a21020), API 34. FTL outcome: Passed.

## gs_tess_primitive_id: fixed (FAIL on 113, csf-v11 232)

```
CASE direct PASS / direct-replay PASS / indirect PASS / indirect-replay PASS
CASE chunk n=3600 expect=3600 PASS
CASE chunk-replay n=3600 expect=3600 PASS
CASE indirect-chunk n=3600 expect=3600 PASS
RESULT PASS
```

113 had `FAIL chunk ids bad=347`, `chunk-replay bad=261` and `indirect-chunk bad=306`.

tess_perf: all 18 BENCH hashes are identical to 112 (4807f3f5, a03404c5, cec55885, d4463b35, b34a251c, for default and legacy opts, gs=0 and gs=1). gs=0 L=16 takes 592 ms (113: 738, 112: 612) and gs=1 L=16 takes 1105 ms (113: 1256, 112: 1005).

## TLS

Every "panvk: device TLS" line in logcat (count, line). These are the same as on 113, because 232 changes only the stride the TSDs carry, not the BOs:

```
7  panvk: device TLS 4096 B/thread x 2048 threads x 25 core IDs (7 cores, mask 0x1110055) = 200 MiB of VA, 56 MiB backed
2  panvk: device TLS 2048 B/thread x 2048 threads x 25 core IDs (7 cores, mask 0x1110055) = 100 MiB of VA, 28 MiB backed
2  panvk: device TLS 256 B/thread x 2048 threads x 25 core IDs (7 cores, mask 0x1110055) = 12 MiB of VA, 3 MiB backed
```

The sparse TLS stays on (`PANVK_TLS_SPARSE=1` default), so the 4096 B/thread case backs 56 MiB instead of 200 MiB (7 of 25 core-ID slabs). No DEVICE_LOST, event=0x or Fatal signal lines appear in logcat.

## Why 113 failed and 114 passes

In 230's sparse BO, each core-ID slot is THREAD_TLS_ALLOC x (the BO's per-thread size) wide, and the slots of absent cores alias slab 0. The hardware places core c's stacks at c x THREAD_TLS_ALLOC x (the stack size in the job's TSD). On 113, per-job TSDs (dispatch, precomp, and the gpu_prerast VS/GS/TCS compute jobs) carried their own shader's smaller stack size, so present cores landed in absent-core slots (which are slab 0) and threads shared stacks. 232 gives every TSD the BO's stride: the command buffer descriptor gets the BO's per-thread size, and per-job TSDs copy word 0 (TLS Size) along with the pointer. Shrink and 231 played no part: the 113 gs_tess process created one BO and lasted 0.25 s.

## Timing (ms from the logcat RESULT lines)

| test | 114 | 113 | 112 | delta vs 112 |
|---|---|---|---|---|
| gpu_prerast_slice | 0.1s | 0.1s | 0.1s | +13% |
| clip_cull | 1.2s | 1.2s | 1.2s | +1% |
| multi_viewport | 1.1s | 1.1s | 1.1s | -2% |
| fill_mode | 1.3s | 1.5s | 1.3s | -1% |
| bc_decode | 12.3s | 12.0s | 13.3s | -7% |
| bc_perf | 68.8s | 59.5s | 76.1s | -10% |
| tess_perf | 47.3s | 47.5s | 44.6s | +6% |
| geometry | 1.6s | 1.5s | 1.5s | +7% |
| tessellation | 2.5s | 2.2s | 2.4s | +5% |
| xfb | 0.4s | 0.3s | 0.4s | -4% |
| pipeline_stats | 1.2s | 1.2s | 1.2s | -2% |
| vertex_stores | 0.2s | 0.1s | 0.1s | +42% |
| gs_viewport_depth | 0.2s | 0.3s | 0.2s | -5% |
| vs_viewport_index | 0.3s | 0.2s | 0.3s | -24% |
| depth_bounds | 0.2s | 0.2s | 0.1s | +6% |
| large_draw | 6.4s | 6.3s | 6.0s | +6% |
| vmr_secondary | 0.1s | 0.1s | 0.1s | -7% |
| tess_cond_state | 0.2s | 0.2s | 0.3s | -33% |
| bachata_reqs | 0.1s | 0.1s | 0.1s | -4% |
| bachata_exec | 0.1s | 0.1s | 0.1s | -22% |
| robustness2 | 0.1s | 0.1s | 0.1s | -7% |
| robust_image_access2 | 0.7s | 0.6s | 0.6s | +12% |
| blend | 0.1s | 0.1s | 0.1s | -11% |
| occlusion_query | 0.1s | 0.1s | 0.1s | -6% |
| descriptor_model | 0.1s | 0.1s | 0.1s | +2% |
| shader_arith | 0.2s | 0.1s | 0.1s | +45% |
| depth_stencil | 0.1s | 0.1s | 0.1s | +27% |
| sampler | 0.1s | 0.1s | 0.1s | +19% |
| draw_params | 0.1s | 0.2s | 0.1s | +14% |
| submit_stress | 30.1s | 30.1s | 30.1s | +0% |
| csf_event | 0.3s | 0.3s | 0.3s | +5% |
| gs_tess_primitive_id | 0.2s | 0.2s | 0.4s | -43% |
| bachata_storage_fmtless | 0.1s | 0.1s | 0.1s | +19% |
| bachata_dynamic_render | 0.1s | 0.1s | 0.1s | +9% |
| dxvk_reqs | 0.1s | 0.1s | 0.1s | +59% |
| vkd3d_reqs | 0.0s | 0.1s | 0.1s | -17% |
| vkd3d_heap | 0.3s | 0.3s | 0.3s | -6% |
| vkd3d_timeline | 0.3s | 0.3s | 0.3s | +9% |
| swapchain_lifecycle | 9.2s | 9.2s | 9.2s | +0% |
| total | 187s | 177s | 192s | -3% |

bc_perf moves run to run, as on 110-113, and 232 does not touch BC. No other test moves outside noise.

Raw files are in raw/ (the uploaded APK is not kept).

Screenshots (frames from the FTL video): ![start](screenshots/video-start.png) ![mid](screenshots/video-mid.png) ![end](screenshots/video-end.png)
