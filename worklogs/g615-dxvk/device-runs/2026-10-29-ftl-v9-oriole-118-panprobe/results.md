# FTL oriole (Pixel 6, Mali-G78 MC20, v9 JM, API 33) build 118: PanProbe (scenario 2, rc5 only)

v9 regression check for build 118 (replaces the disconnected TB336FU).

- Matrix `matrix-2j108qa94uxfx`, project `abhay-byte` (account abhay02delhi@gmail.com), FTL outcome `Passed oriole-33-en-portrait`, ~4 min run.
  One run only, no retries. GCS dir `panvk-118-oriole-probe`.
- APK `panprobe-118.apk`, rc5 slot `driverInfo Mesa 26.3.0-devel (git-a0412457ab)` (jm-v7-wip through 0153).
- `raw/` has logcat, game_loop_results json, gcloud.log (video.mp4 dropped). No `artifacts/` dir was produced by FTL this time.

## Result

- **40 PASS of 40**, 0 fail, 0 skip (`AUTORUN DONE pass=40 fail=0 skip=0 total=40`).
- `draw_params` PASS (`RESULT PASS`): 0152 instance-divisor fix OK on v9.
- `bc_decode` PASS (`BC_DEVICE_FAILS=0`): 0153 OK on v9.
- No DEVICE_LOST / `event=0x` in logcat.

## Per-test time (s, delta between consecutive PANPROBE_TEST lines)

| test | s | | test | s |
|---|---|---|---|---|
| gpu_prerast_slice | ~0 | | robust_image_access2 | 8.1 |
| clip_cull | 1.2 | | submit_stress | 30.1 |
| multi_viewport | 1.1 | | swapchain_lifecycle | 6.5 |
| fill_mode | 1.2 | | tessellation | 2.4 |
| bc_decode | 0.9 | | geometry | 1.4 |
| **bc_perf** | **32.4** | | pipeline_stats | 1.2 |
| **tess_perf** | **34.8** | | csf_event | 0.6 |
| **large_draw** | **9.6** | | all others | <= 0.4 |

## Compared with earlier oriole (FTL `../2026-10-26-ftl-v9-oriole-115pre-d/`, driver git-8a4d9f8e50, 39 tests, `defaults` pass)

| test | 115pre-d s | 118 s |
|---|---|---|
| bc_perf | 36.0 | 32.4 (-10%) |
| tess_perf | 91.8 | 34.8 (-62%) |
| large_draw | 32.2 | 9.6 (-70%) |
| bc_decode | 0.3 | 0.9 |

Caveats: different driver (115pre-d to 118 includes the perf work), 39 vs 40 tests (40th is new), different oriole unit and load.
The per-test times here are wall-clock log deltas, 115pre-d used RESULT lines. Same order of magnitude of method, not identical.
