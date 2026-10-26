# FTL starlte (Galaxy S9, v6, Mali-G72 MC18) 115pre-d: PanProbe driver env A/B (scenario 3)

Both passes: **39/39** (`AUTORUN DONE pass=39 fail=0 skip=0` twice). FTL outcome: Passed. No DEVICE_LOST, `event=0x`, GPU/MMU
fault, Fatal signal or lowmemorykiller lines in logcat. The Mesa W/E lines are the same set as in 113 (timestamp calibration,
gralloc), twice as many because there are two passes.

- Matrix `matrix-21gpam2zzzhuh` (2026-10-26 15:03 UTC, test 15:04-15:14), results
  `gs://test-lab-hrb456s6qt79m-kz4jkut67musi/panvk-v6-starlte-115pre-d-probe/`.
  `gcloud firebase test android run --type game-loop --app panprobe-115pre-d.apk --scenario-numbers 3 --device model=starlte,version=29 --timeout 30m --results-dir panvk-v6-starlte-115pre-d-probe --directories-to-pull /sdcard/Android/data/dev.zenithblue.panvktest.rc5u/files`,
  under the Firebase lock.
- APK `/home/flux/build-out/apks/panprobe-115pre-d.apk` (sha256 `ec4a6453...`), `dev.zenithblue.panvktest.rc5u` 1.2.7 (vc 15).
- Device: starlte, Android 10 (SDK 29), kbase JM uAPI 11.16, gpu_id `0x62210010`, arch v6, `Mali-G72 MC18`.
- Driver: the bundled rc5 slot = `/home/flux/build-out/2026-10-26-115pre-d/libvulkan_panfrost.so`, sha256 `7f2260f0...`,
  build-id `c4f0d9c9...`: `p115-int` + `patches/staging-115/pd` (7 patches).
- Passes (`assets/env-ab.json`, sha256 `94309796...`, logged by `GAMELOOP envab`): 1 `defaults` (no env, cold shader cache),
  2 `114-knobs` (26 keys: the 21 pa-pc knobs and the 5 pd knobs at their 114 values, `patches/staging-115/pd/NOTES.md`), right
  after pass 1 in the same app session (warm shader cache).
- Baseline 113 = `device-runs/2026-10-25-ftl-v6-starlte-rc5u-113` (scenario 2, one cold pass, another starlte). There is
  no JM 114 run; pass 2 is the 114 behaviour of this build.

## Driver identity

| | value |
|---|---|
| `PANPROBE_RESULT` | `driver=Mesa 26.3.0-devel (git-8a4d9f8e50) pass=39 fail=0 skip=0` (both passes) |
| summary.json | driverName `panvk`, driverInfo `Mesa 26.3.0-devel (git-8a4d9f8e50)`, deviceName `Mali-G72 MC18` |

`8a4d9f8e50` is the `p115-pd` head.

## The env reached the driver

| line (once per device open) | defaults | 114-knobs |
|---|---|---|
| `kbase: JM warm-up off (PANVK_KBASE_JM_WARMUP=0, ...)` (pd 0006) | 92 | 0 |
| `kbase: JM warm-up 2000 ms (first JM open in this process)` | 0 | 47 |
| `kbase: JM warm-up skipped (done once in this process; ...)` | 0 | 45 |
| `panvk: v6 tess batch budget 32 MiB: new batch (varyings N MiB charged, D draws per batch, ...)` (pd 0004, first 3 per process) | `40 MiB charged, 4 draws` x3 | `43 MiB charged, 0 draws` x3 (113: `43 MiB worst case`) |
| `panvk: 114 behaviour restored for: ...` (P-A registry) | 0 | 46 (+1 in the results JSON) |
| `kbase: coherency mode 31 (none), shaderDeviceClock off (strict ...)` / `on (114 rule ...)` | 92 off | 92 on |

## Correctness

- tess_perf: all 18 BENCH hashes equal to 113 in both passes (`4807f3f5`, `a03404c5`, `cec55885`, `d4463b35`, `b34a251c`).
  pd 0004 puts 4 tess draws in a batch (64 instead of 256 batches per frame) with identical images.
- tessellation, gs_tess_primitive_id, geometry, xfb, large_draw, submit_stress, swapchain_lifecycle (61.4 / 61.7 fps) PASS in
  both passes.
- First submit with the warm-up off (pd 0006): the first test of pass 1 (gpu_prerast_slice) passes in 0.23 s instead of 2.2 s.
  There are no submit retries, "JM atom ... failed" lines or event errors in any of the 92 device opens. The timestamp/query
  tests (csf_event, vkd3d_timeline, occlusion_query, pipeline_stats) pass in both passes. As on 113, starlte does not
  advertise timestamps (`cntfrq_el0 is 0 ... TIMEINFO failed ... timestamps not advertised`, once per device open).

## GPU-bound time: tess_perf (ms per frame)

The shader cache does not change these numbers. `old-tess` = `PANVK_TESS_FUSE=0,PANVK_TESS_SETUP_STEP=0,PANVK_TESS_ARENAS=1,PANVK_TESS_WINDING_INLINE=0`.

| case | defaults | 114-knobs | 113 | defaults vs 114-knobs | 114-knobs vs 113 |
|---|---|---|---|---|---|
| opt=default gs=0 L=ref | 1.43 | 3.32 | 2.93 | -56.9% | +13.3% |
| opt=default gs=0 L=1 | 136.94 | 154.01 | 153.72 | -11.1% | +0.2% |
| opt=default gs=0 L=4 | 433.79 | 464.39 | 463.10 | -6.6% | +0.3% |
| opt=default gs=0 L=16 | 2236.79 | 2265.76 | 2266.33 | -1.3% | -0.0% |
| opt=default gs=0 L=mix | 1842.52 | 1857.70 | 1857.13 | -0.8% | +0.0% |
| opt=default gs=1 L=1 | 227.43 | 237.30 | 234.22 | -4.2% | +1.3% |
| opt=default gs=1 L=4 | 511.71 | 520.11 | 519.81 | -1.6% | +0.1% |
| opt=default gs=1 L=16 | 1780.69 | 1792.79 | 1789.07 | -0.7% | +0.2% |
| opt=default gs=1 L=mix | 1737.81 | 1746.96 | 1748.13 | -0.5% | -0.1% |
| opt=old-tess gs=0 L=ref | 1.40 | 2.53 | 1.89 | -44.7% | +33.9% |
| opt=old-tess gs=0 L=1 | 139.67 | 156.46 | 156.23 | -10.7% | +0.1% |
| opt=old-tess gs=0 L=4 | 448.28 | 477.93 | 477.70 | -6.2% | +0.0% |
| opt=old-tess gs=0 L=16 | 2389.02 | 2414.35 | 2413.22 | -1.0% | +0.0% |
| opt=old-tess gs=0 L=mix | 1977.99 | 1998.43 | 1993.55 | -1.0% | +0.2% |
| opt=old-tess gs=1 L=1 | 227.21 | 237.76 | 238.18 | -4.4% | -0.2% |
| opt=old-tess gs=1 L=4 | 529.04 | 537.15 | 537.82 | -1.5% | -0.1% |
| opt=old-tess gs=1 L=16 | 1941.16 | 1945.45 | 1947.30 | -0.2% | -0.1% |
| opt=old-tess gs=1 L=mix | 1878.26 | 1885.80 | 1908.77 | -0.4% | -1.2% |

- `114-knobs` reproduces 113 within 0.3 % on the tessellated cases, except gs=1 L=1 (+1.3 %) and old-tess gs=1 L=mix
  (-1.2 %). The L=ref case (256 plain draws) is a 1-3 ms number and noisy.
- With the pd defaults, every tessellated case is faster. The expected sources are pd 0004 (192 fewer framebuffer spills
  per frame) and pd 0002 (one descriptor pool clean per resubmitted frame instead of one per batch); this run does not
  separate them:
  - L=1: -11 % (gs=0) and -4 % (gs=1);
  - L=4: -6.6 % / -1.6 %;
  - L=16 and mix: -0.2 % to -1.3 %.
- L=ref: 1.4 ms against 2.5-3.3 ms.

Memory (tess_perf `MEM ... rss_kb` after each case; kbase counts GPU pages in RSS):

| case | defaults | 114-knobs | 113 |
|---|---|---|---|
| gs=0 L=1 | 115372 | 113420 | 115424 |
| gs=0 L=16 | 123572 | 117464 | 119480 |
| gs=1 L=16 (every gs=1 case within 0.1 MiB) | 255904 | 216972 | 218956 |
| next VkDevice (`MEM device`, after the teardown) | 25432 | 23420 | 25464 |

**Cost of pd 0004:** four tess draws per batch touch more of the growable (GROW_ON_GPF) varying and GS-scratch buffers:
+6 MiB at gs=0 L=16 and **+38 MiB (+18 %) peak RSS in the gs=1 cases**. It is returned at device teardown. It is bounded
because a batch is closed once its charged varyings or its GS-scratch offset reach the 32 MiB budget.
`PANVK_V7_TESS_BATCH_DRAWS=2` would be a middle point (not run).

## Per-test time (s, from the RESULT lines)

| test | defaults | 114-knobs | 113 | defaults vs 113 | 114-knobs vs 113 | defaults vs 114-knobs |
|---|---|---|---|---|---|---|
| gpu_prerast_slice | 0.2 | 2.2 | 2.1 | -89% | +2% | -89% |
| clip_cull | 1.3 | 3.3 | 3.3 | -61% | +1% | -62% |
| multi_viewport | 1.3 | 3.2 | 3.1 | -59% | +1% | -60% |
| fill_mode | 2.6 | 4.0 | 4.1 | -37% | -2% | -35% |
| bc_decode | 0.3 | 2.2 | 2.6 | -90% | -16% | -88% |
| bc_perf | 36.2 | 36.8 | 38.6 | -6% | -5% | -2% |
| tess_perf | 95.7 | 98.4 | 98.6 | -3% | -0% | -3% |
| geometry | 1.6 | 3.5 | 3.8 | -57% | -7% | -54% |
| tessellation | 3.6 | 5.3 | 6.1 | -40% | -13% | -32% |
| xfb | 1.3 | 3.4 | 3.4 | -61% | -1% | -61% |
| pipeline_stats | 1.4 | 3.3 | 3.4 | -59% | -2% | -58% |
| vertex_stores | 0.2 | 2.2 | 2.2 | -92% | +2% | -92% |
| gs_viewport_depth | 0.1 | 2.2 | 2.2 | -94% | -3% | -94% |
| vs_viewport_index | 0.3 | 2.4 | 2.3 | -87% | +2% | -87% |
| depth_bounds | 0.4 | 2.4 | 2.4 | -85% | +2% | -85% |
| large_draw | 24.2 | 26.2 | 26.3 | -8% | -0% | -8% |
| vmr_secondary | 0.2 | 2.1 | 2.2 | -93% | -2% | -93% |
| tess_cond_state | 0.2 | 2.2 | 2.2 | -92% | -1% | -92% |
| bachata_reqs | 0.1 | 2.2 | 2.1 | -95% | +2% | -95% |
| bachata_exec | 0.1 | 2.2 | 2.2 | -94% | +2% | -94% |
| robustness2 | 0.1 | 2.1 | 2.2 | -93% | -1% | -93% |
| robust_image_access2 | 0.8 | 2.8 | 2.9 | -70% | -2% | -70% |
| blend | 0.2 | 2.2 | 2.2 | -93% | +1% | -93% |
| occlusion_query | 0.1 | 2.2 | 2.2 | -94% | +1% | -94% |
| descriptor_model | 0.1 | 2.2 | 2.1 | -94% | +3% | -94% |
| shader_arith | 0.1 | 2.2 | 2.3 | -94% | -5% | -93% |
| depth_stencil | 0.2 | 2.2 | 2.2 | -93% | +0% | -93% |
| sampler | 0.2 | 2.2 | 2.3 | -92% | -4% | -92% |
| draw_params | 0.1 | 2.2 | 2.2 | -93% | -2% | -93% |
| submit_stress | 30.1 | 32.1 | 32.1 | -6% | +0% | -6% |
| csf_event | 0.3 | 2.8 | 2.9 | -88% | -2% | -88% |
| gs_tess_primitive_id | 0.8 | 2.6 | 2.6 | -72% | -2% | -71% |
| bachata_storage_fmtless | 0.1 | 2.2 | 2.2 | -94% | +2% | -94% |
| bachata_dynamic_render | 0.2 | 2.2 | 2.2 | -92% | -1% | -92% |
| dxvk_reqs | 0.1 | 2.1 | 2.1 | -96% | +1% | -96% |
| vkd3d_reqs | 0.1 | 2.1 | 2.1 | -96% | +1% | -96% |
| vkd3d_heap | 0.4 | 2.5 | 2.4 | -83% | +2% | -83% |
| vkd3d_timeline | 0.3 | 2.3 | 2.3 | -89% | +0% | -89% |
| swapchain_lifecycle | 9.4 | 11.4 | 11.4 | -17% | +0% | -17% |
| total | 215.0 | 292.1 | 295.9 | -27% | -1% | -26% |

- **pd 0006 dominates the totals.** Each test process of pass 2 sleeps 2 s at its first JM device open (47 sleeps). Without
  them, every small test drops from about 2.2 s to 0.1-0.4 s. The total is 215.0 s against 292.1 s (`114-knobs`, -26 %) and
  295.9 s (113, -27 %).
- `114-knobs` runs with a warm shader cache, and its total is within 1.3 % of 113.
- After subtracting the warm-up, the larger tests compare as follows (`defaults` vs `114-knobs` minus 2 s):
  - tess_perf 95.7 vs 96.4 s;
  - large_draw 24.2 vs 24.2 s;
  - bc_perf 36.2 vs 34.8 s (pass 2 has the warm cache; the cold 113 pass minus its 2 s is 36.6 s);
  - submit_stress 30.1 vs 30.1 s;
  - swapchain_lifecycle 9.4 vs 9.4 s.
- No test is slower with the defaults in the raw times. After removing the 2 s, only bc_perf is slower, and that is the
  cold shader cache.

## CPU-bound draw loops (FPS lines)

| test | defaults fps | 114-knobs fps | 113 fps |
|---|---|---|---|
| clip_cull | 2458.3 | 1766.9 | 1992.6 |
| multi_viewport | 2608.7 | 1813.9 | 2554.8 |
| fill_mode | 2914.9 | 1702.1 | 2356.6 |
| geometry | 3390.5 | 2186.5 | 2407.2 |
| tessellation | 3142.5 | 2396.2 | 2031.6 |
| pipeline_stats | 1928.7 | 1411.8 | 1433.4 |
| swapchain_lifecycle | 60.5 | 60.5 | 60.2 |

These 1 s loops of tiny draws measure CPU submission cost, and they are noisy between passes and runs. Pass 2 runs on a device
that has just done 4 minutes of GPU work, and `114-knobs` is 9-29 % below 113 on four of them. With the defaults every loop
is faster than both. That fits pd 0001 (no dead FS copy job per draw and layer) and the pa-pc CPU knobs, but this run cannot
separate them. On the a31 the first three loops went the other way (see that run's results.md, start-of-run system load).

## Screenshots

`screenshots/`:
- `video-start.png`: video at 30 s (08:04), pass 1 (defaults) running: 5 pass, gpu_prerast_slice 233 ms (no warm-up),
  clip_cull 1260 ms.
- `video-mid.png`: video middle (08:09), pass 2 (114-knobs) running, gpu_prerast_slice 2172 ms (2 s warm-up back), pass 1
  results uploaded (id c7386129).
- `video-end.png`: last frames (08:13), pass 2 done: 39 pass / 0 fail / 0 skip, uploaded (id 5e2940a4).
- `panprobe-defaults-done.png`: video at 160 s (08:07), pass 1 (defaults) done: 39 pass / 0 fail / 0 skip, uploaded
  (id c7386129).
- `panprobe-114knobs-done.png`: last frame, pass 2 (114-knobs) done: 39 pass / 0 fail / 0 skip, uploaded (id 5e2940a4).

## Files

`raw/starlte-29-en-portrait/` contains:
- `logcat` and `video.mp4`;
- `game_loop_results/results_scenario_3.json`;
- `artifacts/.../files/autorun.txt`;
- the run directories `runs/20261026-080749-defaults` and `runs/20261026-081257-114-knobs` (per-test logs and JSON,
  `summary.json`, `device_info.json`).

The uploaded APK is not kept.
