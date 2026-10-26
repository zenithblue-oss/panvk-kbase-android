# FTL a31 (Galaxy A31, v7, Mali-G52 r1 MC2) 115pre-d: PanProbe driver env A/B (scenario 3)

Both passes: **39/39** (`AUTORUN DONE pass=39 fail=0 skip=0` twice). FTL outcome: Passed. No DEVICE_LOST, `event=0x`, GPU/MMU
fault, Fatal signal or lowmemorykiller lines in logcat. The Mesa W/E lines are the same set as in 113 (timestamp calibration
result, gralloc, "v7 support is experimental"), twice as many because there are two passes.

- Matrix `matrix-2nacuceqlxxdw` (2026-10-26 15:25 UTC, test 15:26-15:39), results
  `gs://test-lab-hrb456s6qt79m-kz4jkut67musi/panvk-v7-a31-115pre-d-probe/`.
  Same command as starlte with `--device model=a31,version=31 --results-dir panvk-v7-a31-115pre-d-probe`, under the
  Firebase lock.
- APK `/home/flux/build-out/apks/panprobe-115pre-d.apk` (sha256 `ec4a6453...`), rc5 = `/home/flux/build-out/2026-10-26-115pre-d/libvulkan_panfrost.so`
  (sha256 `7f2260f0...`).
- Device: a31, Android 12 (SDK 31), kbase JM uAPI 11.27, gpu_id `0x74021000` variant 0x2, arch v7, `Mali-G52 r1 MC2`.
- Passes (`assets/env-ab.json` sha256 `94309796...`): 1 `defaults` (cold shader cache), 2 `114-knobs` (26 keys, warm cache).
- Baseline 113 = `device-runs/2026-10-25-ftl-v7-a31-rc5u-113` (scenario 2, one cold pass, another a31). The a31 has large
  run-to-run variance: totals were 364 s on 113, 433 s on 112 and 393 s on 110 with identical v7 code.

## Driver identity

| | value |
|---|---|
| `PANPROBE_RESULT` | `driver=Mesa 26.3.0-devel (git-8a4d9f8e50) pass=39 fail=0 skip=0` (both passes) |
| summary.json | driverName `panvk`, driverInfo `Mesa 26.3.0-devel (git-8a4d9f8e50)`, deviceName `Mali-G52 r1 MC2` |

## The env reached the driver

| line | defaults | 114-knobs |
|---|---|---|
| `kbase: JM warm-up off (...)` (pd 0006) | 92 | 0 |
| `kbase: JM warm-up 2000 ms (first JM open in this process)` / `skipped (done once ...)` | 0 | 47 / 45 |
| `panvk: v7 tess batch budget 32 MiB: new batch (...)` (pd 0004) | `40 MiB charged, 4 draws per batch` x3 | `43 MiB charged, 0 draws per batch` x3 |
| `panvk: 114 behaviour restored for: ...` (P-A registry) | 0 | 46 |
| `kbase: coherency mode 31 (none), shaderDeviceClock off (strict ...)` / `on (114 rule ...)` | 92 off | 92 on |

## Correctness

- tess_perf: all 18 BENCH hashes equal to 113 in both passes (`4807f3f5`, `a03404c5`, `cec55885`, `d4463b35`, `b34a251c`).
- tessellation, gs_tess_primitive_id, geometry, xfb, large_draw, submit_stress, swapchain_lifecycle PASS in both passes.
- First submit with the warm-up off (pd 0006): no submit retries, "JM atom ... failed" or event errors in 92 device opens.
  The first test (gpu_prerast_slice) passes in 0.35 s instead of 2.15 s.
- Timestamps with the warm-up off: the per-open calibration of the GPU counter is 26 MHz in every device open. In
  `defaults`, all 92 measurements are within 83 ppm. In `114-knobs`, one of 92 is 1428 ppm off and is still snapped to
  26 MHz. bc_perf's GPU-timed sums are equal in the three runs, and GPU/wall is 0.96-1.0 (see below).

## GPU-bound time: tess_perf (ms per frame)

| case | defaults | 114-knobs | 113 | defaults vs 114-knobs | 114-knobs vs 113 |
|---|---|---|---|---|---|
| opt=default gs=0 L=ref | 1.94 | 1.99 | 1.91 | -2.5% | +4.2% |
| opt=default gs=0 L=1 | 104.13 | 125.91 | 123.53 | -17.3% | +1.9% |
| opt=default gs=0 L=4 | 620.89 | 640.79 | 640.52 | -3.1% | +0.0% |
| opt=default gs=0 L=16 | 5119.08 | 5127.18 | 5121.02 | -0.2% | +0.1% |
| opt=default gs=0 L=mix | 2452.67 | 2459.47 | 2456.45 | -0.3% | +0.1% |
| opt=default gs=1 L=1 | 370.61 | 380.02 | 390.75 | -2.5% | -2.7% |
| opt=default gs=1 L=4 | 668.53 | 662.23 | 704.61 | +1.0% | -6.0% |
| opt=default gs=1 L=16 | 1699.17 | 1716.81 | 1694.83 | -1.0% | +1.3% |
| opt=default gs=1 L=mix | 1498.98 | 1521.55 | 1507.66 | -1.5% | +0.9% |
| opt=old-tess gs=0 L=ref | 2.02 | 1.91 | 1.94 | +5.8% | -1.5% |
| opt=old-tess gs=0 L=1 | 107.37 | 123.25 | 130.85 | -12.9% | -5.8% |
| opt=old-tess gs=0 L=4 | 628.83 | 653.53 | 658.08 | -3.8% | -0.7% |
| opt=old-tess gs=0 L=16 | 5242.61 | 5247.74 | 5269.50 | -0.1% | -0.4% |
| opt=old-tess gs=0 L=mix | 2540.67 | 2548.56 | 2558.57 | -0.3% | -0.4% |
| opt=old-tess gs=1 L=1 | 367.80 | 383.35 | 382.57 | -4.1% | +0.2% |
| opt=old-tess gs=1 L=4 | 690.84 | 692.52 | 695.10 | -0.2% | -0.4% |
| opt=old-tess gs=1 L=16 | 1847.10 | 1830.17 | 1829.87 | +0.9% | +0.0% |
| opt=old-tess gs=1 L=mix | 1586.34 | 1580.88 | 1571.24 | +0.3% | +0.6% |

- `114-knobs` is within about 2 % of 113 on most cases. The exceptions are gs=1 L=4 (-6 %) and old-tess gs=0 L=1 (-6 %),
  and the a31 varies that much between runs.
- With the pd defaults, the gains are at the low levels:
  - gs=0 L=1: -17 % (default opts) and -13 % (old-tess);
  - gs=0 L=4: -3 % / -4 %;
  - gs=1 L=1: -2.5 % / -4 %.
- L=16 and mix are within ±1.5 %, which is noise on this device.
- A1 is unchanged: gs=0 L=16 is 5119 ms against 1699 ms for gs=1 (3.0x). pd 0004 does not change it: in d-jm.md, A1's
  cost grows with each extra triangle, not with the batches.
- bc_perf GPU-timed sums over the 20 formats (ms; `defaults` / `114-knobs` / 113):
  - upload 691.5 / 697.7 / 694.7;
  - A 59.6 / 59.7 / 59.6;
  - D 40.9 / 41.2 / 41.2;
  - E 52.0 / 53.2 / 53.8.

Memory (tess_perf `MEM ... rss_kb`):

| case | defaults | 114-knobs | 113 |
|---|---|---|---|
| gs=0 L=1 | 48692 | 47200 | 49100 |
| gs=0 L=16 | 56856 | 51244 | 53096 |
| gs=1 L=16 (every gs=1 case within 0.1 MiB) | 155812 | 117320 | 119264 |
| next VkDevice (`MEM device`) | 52456 | 51412 | 52704 |

The cost of pd 0004 is the same as on starlte: **+38 MiB peak RSS in the gs=1 cases** (+33 % here) and +5.5 MiB at gs=0
L=16. It is returned at device teardown.

## Per-test time (s, from the RESULT lines)

| test | defaults | 114-knobs | 113 | defaults vs 113 | 114-knobs vs 113 | defaults vs 114-knobs |
|---|---|---|---|---|---|---|
| gpu_prerast_slice | 0.4 | 2.2 | 2.2 | -84% | -0% | -84% |
| clip_cull | 1.6 | 3.2 | 3.2 | -50% | -0% | -50% |
| multi_viewport | 1.4 | 3.2 | 3.2 | -55% | +0% | -56% |
| fill_mode | 3.2 | 4.4 | 4.5 | -29% | -3% | -27% |
| bc_decode | 1.2 | 2.3 | 3.2 | -62% | -30% | -47% |
| bc_perf | 61.6 | 64.9 | 64.0 | -4% | +1% | -5% |
| tess_perf | 131.1 | 133.3 | 133.8 | -2% | -0% | -2% |
| geometry | 2.2 | 3.6 | 4.2 | -46% | -13% | -38% |
| tessellation | 8.9 | 10.6 | 10.8 | -18% | -2% | -16% |
| xfb | 1.3 | 3.2 | 3.3 | -60% | -0% | -60% |
| pipeline_stats | 1.4 | 3.3 | 3.4 | -60% | -3% | -59% |
| vertex_stores | 0.2 | 2.2 | 2.2 | -92% | +1% | -92% |
| gs_viewport_depth | 0.2 | 2.2 | 2.2 | -91% | -1% | -91% |
| vs_viewport_index | 0.5 | 2.4 | 2.5 | -81% | -3% | -81% |
| depth_bounds | 0.4 | 2.4 | 2.3 | -84% | +2% | -84% |
| large_draw | 21.0 | 23.4 | 22.9 | -8% | +2% | -10% |
| vmr_secondary | 0.2 | 2.1 | 2.1 | -93% | -0% | -93% |
| tess_cond_state | 0.2 | 2.2 | 2.2 | -92% | -1% | -92% |
| bachata_reqs | 0.1 | 2.1 | 2.1 | -94% | -0% | -94% |
| bachata_exec | 0.2 | 2.2 | 2.2 | -93% | -1% | -93% |
| robustness2 | 0.1 | 2.1 | 2.2 | -94% | -2% | -93% |
| robust_image_access2 | 3.7 | 5.5 | 5.7 | -36% | -3% | -34% |
| blend | 0.2 | 2.2 | 2.2 | -93% | +1% | -93% |
| occlusion_query | 0.1 | 2.1 | 2.2 | -94% | -0% | -94% |
| descriptor_model | 0.2 | 2.2 | 2.2 | -93% | -1% | -93% |
| shader_arith | 0.2 | 2.2 | 2.2 | -91% | -2% | -91% |
| depth_stencil | 0.2 | 2.2 | 2.2 | -93% | +1% | -93% |
| sampler | 0.2 | 2.2 | 2.2 | -91% | -1% | -91% |
| draw_params | 0.2 | 2.2 | 2.2 | -93% | -0% | -92% |
| submit_stress | 30.1 | 32.2 | 32.1 | -6% | +0% | -6% |
| csf_event | 0.4 | 2.6 | 2.6 | -86% | +1% | -87% |
| gs_tess_primitive_id | 0.6 | 2.6 | 2.6 | -76% | +1% | -76% |
| bachata_storage_fmtless | 0.1 | 2.2 | 2.2 | -93% | -0% | -93% |
| bachata_dynamic_render | 0.2 | 2.1 | 2.1 | -93% | -0% | -93% |
| dxvk_reqs | 0.1 | 2.1 | 2.1 | -95% | +0% | -95% |
| vkd3d_reqs | 0.1 | 2.1 | 2.1 | -95% | -0% | -95% |
| vkd3d_heap | 0.4 | 2.4 | 2.4 | -85% | -1% | -85% |
| vkd3d_timeline | 0.3 | 2.3 | 2.3 | -87% | +1% | -87% |
| swapchain_lifecycle | 12.6 | 14.3 | 14.2 | -11% | +1% | -12% |
| total | 286.9 | 362.9 | 364.3 | -21% | -0% | -21% |

- Totals: 286.9 s with the defaults against 362.9 s with `114-knobs` (-21 %) and 364.3 s on 113. `114-knobs` matches 113
  (-0.4 %).
- With 2 s removed from every `114-knobs` test, the totals are 286.9 against 284.9 s (+0.7 %).
  - The tests that are then slower with the defaults are the first, compile-heavy and CPU-bound ones: bc_decode +0.9 s,
    fill_mode +0.8 s, geometry +0.6 s, clip_cull +0.4 s, tessellation, swapchain_lifecycle and multi_viewport +0.25-0.3 s.
  - bc_perf is 1.3 s faster.

## CPU-bound draw loops (FPS lines): early-test drop on the a31

| test | defaults fps | 114-knobs fps | 113 fps |
|---|---|---|---|
| clip_cull | 1052.6 | 2609.2 | 2853.4 |
| multi_viewport | 1400.5 | 2695.2 | 2640.0 |
| fill_mode | 1591.5 | 2759.8 | 2775.2 |
| geometry | 3004.4 | 2593.8 | 2825.7 |
| tessellation | 2856.6 | 2778.4 | 2745.4 |
| pipeline_stats | 1839.9 | 1828.4 | 1932.5 |
| swapchain_lifecycle | 44.2 | 45.3 | 45.8 |

clip_cull, multi_viewport and fill_mode are the 2nd-4th tests of the run. In `defaults` they run 3-9.5 s after the
game loop starts, because there is no 2 s warm-up per process anymore.
- In that window the device is busy with system work: 5994 non-PanVK logcat lines in 08:26:57-08:27:06 (serviceentitlement,
  Play services, Finsky, SettingsProvider). The same 9 s are 340 lines a minute later and 920 at the start of pass 2.
- The later draw loops of the same pass (geometry +16 %, tessellation +3 %, pipeline_stats +1 %) are not slower.
- On starlte the same three tests are 39-71 % faster with the defaults.
- So the drop looks like start-of-run system load meeting the earlier start, not a driver regression. The order check
  below tests that.

Order check: pending. A short a31 run follows the oriole run: the six draw-loop tests only, with 114-knobs, defaults,
114-knobs and defaults, through `/data/local/tmp/panprobe-env-ab.json`.

## Screenshots

`screenshots/`:
- `video-start.png`: video at 30 s (08:27), pass 1 (defaults) running: 5 pass, gpu_prerast_slice 353 ms (no warm-up).
- `video-mid.png`: video middle (08:32), pass 1 (defaults) done: 39 pass / 0 fail / 0 skip, uploaded (id 7b6e0a8a),
  gpu_prerast_slice 353 ms. Pass 2 has started, but the list is not updated yet.
- `video-end.png`: 3 s before the end (08:38), pass 2 (114-knobs) done: 39 pass / 0 fail / 0 skip, uploaded (id b02ebc57),
  gpu_prerast_slice 2152 ms. The very last frame is FTL's "Test Loop" screen.
- `panprobe-defaults-done.png`: video at 165 s (08:31), pass 1 (defaults) done: 39 pass / 0 fail / 0 skip, uploaded
  (id 7b6e0a8a).
- `panprobe-114knobs-mid.png`: video at 230 s (08:35), pass 2 (114-knobs) running, gpu_prerast_slice 2152 ms (2 s
  warm-up back).
- `panprobe-114knobs-done.png`: same frame as `video-end.png`, pass 2 (114-knobs) done, uploaded (id b02ebc57).

## Files

`raw/a31-31-en-portrait/` contains:
- `logcat` and `video.mp4`;
- `game_loop_results/results_scenario_3.json`;
- the run directories `runs/20261026-083149-defaults` and `runs/20261026-083808-114-knobs`.

The uploaded APK is not kept.
