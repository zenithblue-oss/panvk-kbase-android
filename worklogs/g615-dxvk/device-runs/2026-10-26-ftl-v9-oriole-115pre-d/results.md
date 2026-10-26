# FTL oriole (Pixel 6, v9, Mali-G78 MC20) 115pre-d: PanProbe driver env A/B (scenario 3)

Both passes: **39/39** (`AUTORUN DONE pass=39 fail=0 skip=0` twice). FTL outcome: Passed. No DEVICE_LOST, `event=0x`, GPU/MMU
fault or Fatal signal lines in logcat. The six `lowmemorykiller` lines are lmkd status lines at 08:46:06, not kills. The
Mesa W/E lines are the same set as in 113 (gralloc, the v9 tess/gpu_prerast chunk limits, dma-buf import sync), twice as
many because there are two passes.

- Matrix `matrix-21rhl4x3wr0yj` (2026-10-26 15:45 UTC, test 15:46-15:56), results
  `gs://test-lab-hrb456s6qt79m-kz4jkut67musi/panvk-v9-oriole-115pre-d-probe/`.
  Same command as starlte with `--device model=oriole,version=33 --results-dir panvk-v9-oriole-115pre-d-probe`, under
  the Firebase lock.
- APK `/home/flux/build-out/apks/panprobe-115pre-d.apk` (sha256 `ec4a6453...`), rc5 = `/home/flux/build-out/2026-10-26-115pre-d/libvulkan_panfrost.so`
  (sha256 `7f2260f0...`).
- Device: oriole, Android 13 (SDK 33), kbase JM uAPI 11.35, gpu_id `0x92020010`, arch v9, `Mali-G78 MC20`.
- Passes (`assets/env-ab.json` sha256 `94309796...`): 1 `defaults` (cold shader cache), 2 `114-knobs` (26 keys, warm cache).
- **On v9 JM, `114-knobs` is not all of 114.** P-C 0002 (C6, Z/S preload mode) also changes v9 JM, and its 114 value
  (`PANVK_ZS_PRELOAD_ALWAYS=1`) is not in the set, because it would be wrong on v11. So C6 is active in both passes here.
- Of P-D, v9 runs 0002 (one descriptor pool clean per resubmit), 0005 (lazy gpu_prerast arenas), 0006 (no warm-up) and
  0007 (TLS/WLS without the kbase clear). 0001, 0003 and 0004 are v6/v7 only.
- Baseline 113 = `device-runs/2026-10-25-ftl-v9-oriole-rc5u-113` (scenario 2, one cold pass, another oriole).

## Driver identity

| | value |
|---|---|
| `PANPROBE_RESULT` | `driver=Mesa 26.3.0-devel (git-8a4d9f8e50) pass=39 fail=0 skip=0` (both passes) |
| summary.json | driverName `panvk`, driverInfo `Mesa 26.3.0-devel (git-8a4d9f8e50)`, deviceName `Mali-G78 MC20` |

## The env reached the driver

| line | defaults | 114-knobs |
|---|---|---|
| `kbase: JM warm-up off (...)` (pd 0006) | 92 | 0 |
| `kbase: JM warm-up 2000 ms (first JM open in this process)` / `skipped (done once ...)` | 0 | 47 / 45 |
| `panvk: 114 behaviour restored for: ...` (P-A registry) | 0 | 46 |
| `kbase: coherency mode 31 (none), shaderDeviceClock off (strict ...)` / `on (114 rule ...)` | 92 off | 92 on |

pd 0005 does not log. Its effect is in the memory numbers below.

## pd 0005 (D7): no gpu_prerast arenas until a pass needs them

bc_perf creates a VkDevice with every feature enabled, records no draws, and logs its memory (`MEM tag=full_feature_device`):

| | defaults | 114-knobs | 113 |
|---|---|---|---|
| `rss_KiB` | 21632 | 187356 | 187032 |
| `gpu_map_KiB` | 1329892 | 1493732 | 1493732 |

So the defaults no longer commit 160 MiB (64 + 96 MiB) of arenas, or clear them on the CPU, at vkCreateDevice: GPU VA is
exactly 163840 KiB lower, and RSS is 162 MiB lower. tess_perf shows the commit moving to first use (`MEM ... rss_kb`):

| point | defaults | 114-knobs | 113 |
|---|---|---|---|
| `MEM device` (after vkCreateDevice) | 19616 | 183356 | 183792 |
| gs=0 L=ref (no tessellation) | 24432 | 185048 | 188608 |
| gs=0 L=1 (the 96 MiB tess arena is now committed) | 224652 | 283676 | 289912 |
| gs=0 L=16 | 319064 | 378088 | 384332 |
| gs=1 L=1 (now the 64 MiB gpu_prerast arena too) | 426128 | 418788 | 425736 |
| gs=1 L=mix | 698992 | 691692 | 698616 |

Once both arenas are in use, the footprint is the same as before. The first draw that needs an arena pays the commit and
clear. No test shows a correctness change: geometry, tessellation, gs_tess_primitive_id, xfb, vertex_stores and large_draw
(`65534 of 149999` / `65534 of 139999` clamps as before) PASS in both passes.

## Correctness

- tess_perf: all 18 BENCH hashes equal to 113 in both passes (`4807f3f5`, `a03404c5`, `cec55885`, `d4463b35`, `b34a251c`).
- First submit with the warm-up off (pd 0006): no submit retries, "JM atom ... failed" or event errors in 92 device opens.
  The first test (gpu_prerast_slice) passes in 0.08 s instead of 2.1 s.
- Timestamps with the warm-up off: the per-open calibration keeps cntfrq 24.576 MHz, within 103 ppm in `defaults` and 99 ppm
  in `114-knobs`. The bc_perf GPU/wall ratio is 0.94-0.99.

## GPU-bound time: tess_perf (ms per frame)

| case | defaults | 114-knobs | 113 | defaults vs 114-knobs | 114-knobs vs 113 |
|---|---|---|---|---|---|
| opt=default gs=0 L=ref | 1.85 | 2.29 | 2.14 | -19.2% | +7.0% |
| opt=default gs=0 L=1 | 69.15 | 68.69 | 70.28 | +0.7% | -2.3% |
| opt=default gs=0 L=4 | 191.78 | 198.22 | 199.24 | -3.2% | -0.5% |
| opt=default gs=0 L=16 | 886.81 | 898.26 | 894.19 | -1.3% | +0.5% |
| opt=default gs=0 L=mix | 895.97 | 902.13 | 896.28 | -0.7% | +0.7% |
| opt=default gs=1 L=1 | 170.94 | 169.01 | 160.97 | +1.1% | +5.0% |
| opt=default gs=1 L=4 | 512.82 | 532.36 | 529.86 | -3.7% | +0.5% |
| opt=default gs=1 L=16 | 3814.79 | 3818.62 | 3791.19 | -0.1% | +0.7% |
| opt=default gs=1 L=mix | 1996.14 | 2022.38 | 2006.28 | -1.3% | +0.8% |
| opt=old-tess gs=0 L=ref | 1.86 | 1.89 | 1.97 | -1.6% | -4.1% |
| opt=old-tess gs=0 L=1 | 70.56 | 67.98 | 67.29 | +3.8% | +1.0% |
| opt=old-tess gs=0 L=4 | 192.65 | 197.61 | 196.85 | -2.5% | +0.4% |
| opt=old-tess gs=0 L=16 | 888.34 | 892.97 | 898.02 | -0.5% | -0.6% |
| opt=old-tess gs=0 L=mix | 901.70 | 905.01 | 896.69 | -0.4% | +0.9% |
| opt=old-tess gs=1 L=1 | 163.25 | 175.11 | 160.95 | -6.8% | +8.8% |
| opt=old-tess gs=1 L=4 | 508.35 | 523.35 | 524.57 | -2.9% | -0.2% |
| opt=old-tess gs=1 L=16 | 3810.73 | 3803.27 | 3783.47 | +0.2% | +0.5% |
| opt=old-tess gs=1 L=mix | 2015.17 | 1998.49 | 1998.89 | +0.8% | -0.0% |

- `114-knobs` is within 1 % of 113 on most cases; gs=1 L=1 (+5 % / +9 %) and L=ref are noisy.
- With the defaults:
  - gs=0 L=4 is -3.2 % and gs=1 L=4 -3.7 % (old-tess -2.5 % / -2.9 %);
  - L=16 and mix are -1.3 % to +0.8 %;
  - L=1 is within the noise.
  The v9 part of P-D is mostly memory and CPU (0002, 0005-0007), so no larger GPU-time change was expected.
- A2 is unchanged: gs=1 L=16 is 3815 ms against 887 ms for gs=0 (4.3x).
- bc_perf GPU-timed sums over the 20 formats (ms; `defaults` / `114-knobs` / 113):
  - upload 1015.1 / 1015.8 / 1012.3;
  - A 6.8 / 7.0 / 6.8;
  - D 3.3 / 4.4 / 4.1;
  - E 8.4 / 9.3 / 9.1.

## Per-test time (s, from the RESULT lines)

| test | defaults | 114-knobs | 113 | defaults vs 113 | 114-knobs vs 113 | defaults vs 114-knobs |
|---|---|---|---|---|---|---|
| gpu_prerast_slice | 0.1 | 2.1 | 2.1 | -96% | +2% | -96% |
| clip_cull | 1.1 | 3.2 | 3.2 | -64% | +1% | -64% |
| multi_viewport | 1.1 | 3.1 | 3.1 | -65% | +1% | -65% |
| fill_mode | 1.2 | 3.4 | 3.2 | -64% | +6% | -66% |
| bc_decode | 0.3 | 2.2 | 2.5 | -87% | -9% | -85% |
| bc_perf | 36.0 | 36.5 | 37.6 | -4% | -3% | -1% |
| tess_perf | 91.8 | 94.3 | 93.8 | -2% | +1% | -3% |
| geometry | 1.4 | 3.3 | 3.5 | -59% | -4% | -57% |
| tessellation | 4.2 | 6.0 | 6.4 | -35% | -6% | -30% |
| xfb | 0.3 | 2.2 | 2.4 | -87% | -6% | -86% |
| pipeline_stats | 1.2 | 3.2 | 3.3 | -62% | -1% | -62% |
| vertex_stores | 0.2 | 2.2 | 2.2 | -91% | +1% | -91% |
| gs_viewport_depth | 0.2 | 2.2 | 2.3 | -90% | -2% | -90% |
| vs_viewport_index | 0.2 | 2.3 | 2.3 | -90% | -2% | -89% |
| depth_bounds | 0.3 | 2.3 | 2.4 | -88% | -4% | -88% |
| large_draw | 32.2 | 34.4 | 34.6 | -7% | -1% | -6% |
| vmr_secondary | 0.1 | 2.1 | 2.1 | -95% | -1% | -95% |
| tess_cond_state | 0.2 | 2.3 | 2.3 | -93% | -0% | -93% |
| bachata_reqs | 0.1 | 2.2 | 2.3 | -96% | -3% | -96% |
| bachata_exec | 0.1 | 2.1 | 2.1 | -96% | -2% | -95% |
| robustness2 | 0.1 | 2.1 | 2.1 | -95% | -1% | -95% |
| robust_image_access2 | 8.0 | 10.1 | 10.1 | -21% | -0% | -21% |
| blend | 0.1 | 2.2 | 2.2 | -95% | -1% | -95% |
| occlusion_query | 0.1 | 2.1 | 2.1 | -94% | +1% | -94% |
| descriptor_model | 0.1 | 2.1 | 2.1 | -95% | +0% | -95% |
| shader_arith | 0.1 | 2.1 | 2.2 | -95% | -4% | -94% |
| depth_stencil | 0.1 | 2.1 | 2.1 | -95% | -1% | -95% |
| sampler | 0.1 | 2.1 | 2.2 | -95% | -3% | -95% |
| draw_params | 0.1 | 2.2 | 2.2 | -95% | +1% | -95% |
| submit_stress | 30.1 | 32.1 | 32.1 | -6% | +0% | -6% |
| csf_event | 0.4 | 2.7 | 2.9 | -85% | -5% | -84% |
| gs_tess_primitive_id | 0.4 | 2.4 | 2.4 | -83% | -0% | -83% |
| bachata_storage_fmtless | 0.1 | 2.1 | 2.1 | -95% | +1% | -95% |
| bachata_dynamic_render | 0.1 | 2.1 | 2.1 | -95% | -1% | -95% |
| dxvk_reqs | 0.1 | 2.1 | 2.1 | -97% | +0% | -97% |
| vkd3d_reqs | 0.1 | 2.1 | 2.1 | -96% | +0% | -96% |
| vkd3d_heap | 0.4 | 2.4 | 2.4 | -84% | -0% | -84% |
| vkd3d_timeline | 0.2 | 2.3 | 2.2 | -90% | +4% | -90% |
| swapchain_lifecycle | 6.4 | 8.3 | 8.4 | -24% | -1% | -23% |
| total | 219.5 | 297.8 | 299.9 | -27% | -1% | -26% |

- Totals: 219.5 s with the defaults against 297.8 s with `114-knobs` (-26 %) and 299.9 s on 113. `114-knobs` matches 113
  (-0.7 %).
- With 2 s removed from every `114-knobs` test, the totals are 219.5 against 219.8 s (-0.1 %).
  - Only bc_perf is then slower with the defaults (+1.6 s, cold shader cache; 113 minus its 2 s is 35.6 s).
  - tess_perf is 0.6 s faster.

## CPU-bound draw loops (FPS lines)

| test | defaults fps | 114-knobs fps | 113 fps |
|---|---|---|---|
| clip_cull | 3580.6 | 2589.0 | 3350.7 |
| multi_viewport | 3629.6 | 2712.8 | 3564.0 |
| fill_mode | 3800.4 | 2914.4 | 3421.2 |
| geometry | 2992.1 | 2973.7 | 3277.9 |
| tessellation | 2939.9 | 2886.4 | 2959.8 |
| pipeline_stats | 2130.3 | 2171.7 | 2021.9 |
| swapchain_lifecycle | 91.2 | 91.2 | 90.5 |

The first three loops are 4-11 % above 113 with the defaults, even though they run during the same kind of start-of-run
system activity as on the a31 (9961 non-PanVK log lines in 08:45:59-08:46:08). `114-knobs` is 20-23 % below 113 on the same
three. Pass 2 starts while the app uploads the pass 1 results, and that also slows the starlte pass-2 loops. The later loops
are within ±2 % of each other.

## Screenshots

`screenshots/`:
- `video-start.png`: video at 30 s (08:48), pass 1 (defaults) running: 6 pass, gpu_prerast_slice 78 ms (no warm-up).
- `video-mid.png`: video middle (08:52), pass 2 (114-knobs) running: 38 pass, gpu_prerast_slice 2122 ms (2 s warm-up back),
  pass 1 results uploaded (id b2ebeb97).
- `video-end.png`: 3 s before the end (08:55), pass 2 done: 39 pass / 0 fail / 0 skip, uploading results.
- `panprobe-defaults-done.png`: video at 81 s (08:49), pass 1 (defaults) done: 39 pass / 0 fail / 0 skip, uploaded
  (id b2ebeb97), gpu_prerast_slice 78 ms.
- `panprobe-114knobs-swapchain.png`: video at 215 s (08:54), pass 2, swapchain_lifecycle live surface (red/blue).
- `panprobe-114knobs-done.png`: last frame (08:55), pass 2 (114-knobs) done: 39 pass / 0 fail / 0 skip, uploaded
  (id cb9d4189).

The FTL video is 229 s for a 9-minute run; it skips time, so frame times do not map linearly to the clock.

## Files

`raw/oriole-33-en-portrait/` contains:
- `logcat` and `video.mp4`;
- `game_loop_results/results_scenario_3.json`;
- the run directories `runs/20261026-084940-defaults` and `runs/20261026-085452-114-knobs`.

The uploaded APK is not kept.
