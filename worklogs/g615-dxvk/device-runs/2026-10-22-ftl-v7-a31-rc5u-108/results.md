# FTL run: Galaxy A31 (a31, Mali-G52 r1 MC2, Bifrost v7, API 31), PanProbe RC5-universal-108, game-loop scenario 2

- Project phonefinder-hub. gcloud exit 0. That only means the game loop finished; the real results are below.
- Bucket `gs://test-lab-hrb456s6qt79m-kz4jkut67musi/panvk-v7-a31-rc5u-108/`, copied to `raw/a31-31-en-portrait/`: logcat (69949 lines), video.mp4 (345.5 s), game_loop_results/results_scenario_2.json, artifacts/.../autorun.txt and runs/20261022-092001/ (per-test logs and JSON). The APK in the bucket was not kept; it is `/home/flux/build-out/apks/panprobe-rc5u-108.apk` (rc5 .so `276db559`).
- **GPU confirmed:** `panvk: gpu_id 0x74021000 variant 0x2 arch v7 model Mali-G52 r1`, `DEVICE Mali-G52 r1 MC2`, kbase JM uAPI 11.27, atom stride 56. Same model and SoC (Helio P65) as the 107 run.
- Driver line: `DRIVER slot=RC5 driverInfo=PanVK-kbase RC5-universal-108 (Mesa 26.3.0-devel (git-03b39a69c0))`. This is mesa-rc5g HEAD `03b39a69` = RC5-universal-107 + jm-v7-wip 0081 (v7 polygon mode in the shader key) + 0082 (kbase JM timestamp calibration).
- GAMELOOP at 09:12:17.8, AUTORUN DONE at 09:20:01.2 (463 s, same as 107). Auto-upload OK, id edb2f3bf.

## Totals
`AUTORUN DONE pass=39 fail=0 skip=0 total=39 (39 pass / 0 fail / 0 skip)`. results_scenario_2.json agrees: pass 39, fail 0, skip 0. **Up from 38/39 on 107; fill_mode is fixed.**

- All 39 tests PASS with mismatch=0. tess_perf PASS, with no LOWMEM.
- Compliance: DXVK items=94, Bachata S4 items=114 and vkd3d items=100 all PASS, missingHard=[] (same missingSoft lists as 107).
- Per-case SKIP lines inside passing tests are the same three as on 107 and the S9: large_draw `PANVK_FAN_CHUNKS unset`, large_draw `drawIndirectCount not exposed`, depth_stencil `d16s8_all format unsupported`. No failing CASE lines anywhere.
- Logcat: 0 `DEVICE_LOST`, 0 `event=0x51`, 0 `Fatal signal`. The 26 lmkd/lowmemorykiller lines are for other processes; none names PanProbe or the testloop companion.

| test | result | ms | fps / extra | 107 |
|---|---|---|---|---|
| gpu_prerast_slice | PASS | 4280 | | PASS 4288 |
| clip_cull | PASS | 5291 | 2233.9 | PASS 1867.5 |
| multi_viewport | PASS | 5166 | 2647.4 | PASS 2392.4 |
| **fill_mode** | **PASS** | 6630 | 2585.3 | **FAIL mismatch=2303**, 2630.6 |
| bc_decode | PASS | 5194 | | PASS |
| bc_perf | PASS | 66980 | | PASS 67921 |
| tess_perf | PASS | 139544 | 0.4 | PASS 139298 |
| geometry | PASS | 6124 | 2862.7 | PASS 2445.5 |
| tessellation | PASS | 12960 | 2824.9 | PASS 2714.2 |
| xfb | PASS | 5303 | | PASS |
| pipeline_stats | PASS | 5351 | 1938.9 | PASS 1775.9 |
| vertex_stores, gs_viewport_depth, vs_viewport_index, depth_bounds | PASS | 4169, 4175, 4462, 4389 | | PASS |
| large_draw | PASS | 24885 | | PASS 24970 |
| vmr_secondary, tess_cond_state, bachata_reqs, bachata_exec, robustness2 | PASS | 4185, 4240, 4152, 4180, 4149 | | PASS |
| robust_image_access2 | PASS | 7766 | ria2 204.89 ms, ria1 204.59 ms (+0.1 %) | 205.32 / 204.54 (+0.4 %) |
| blend, occlusion_query, descriptor_model, shader_arith, depth_stencil, sampler, draw_params | PASS | 4192, 4184, 4192, 4226, 4188, 4203, 4184 | | PASS |
| submit_stress | PASS | 34149 | | PASS 34097 |
| csf_event, gs_tess_primitive_id, bachata_storage_fmtless, bachata_dynamic_render | PASS | 4649, 4700, 4150, 4178 | | PASS |
| dxvk_reqs, vkd3d_reqs, vkd3d_heap, vkd3d_timeline | PASS | 2109, 2101, 10536, 4251 | | PASS |
| swapchain_lifecycle | PASS | 15911 | 46.6 (resized 47.7) | 47.8 (45.7) |

## fill_mode: 17/17 (0081 verified)
```
CASE list_line_cull_none tris=3 kept=3 ref_red=266 red=266 mismatch=0 PASS     (107: red=0 mismatch=266 FAIL)
CASE strip_restart_line tris=4 kept=4 ref_red=204 red=204 mismatch=0 PASS      (107: red=0 mismatch=271 FAIL)
CASE fan_restart_line_cull_back tris=4 kept=0 ref_red=0 red=0 mismatch=0 PASS  (107: mismatch=47 FAIL)
CASE indirect_indexed_strip_restart_line tris=4 kept=4 ref_red=204 red=204 mismatch=0 PASS
FILL_MODE_FAILS=0
```
- All 12 LINE cases, all 3 POINT cases and both indirect cases PASS; `red` equals `ref_red` in every case.
- This exercises exactly the 107 failure path: one autorun on a fresh install, where the cull test first builds the FILL pipeline with `vs_none` (IDVS binary, stored in the app-dir disk cache) and the LINE pipelines are created afterwards. The logcat has no `disabling` shader-cache line, so the disk cache was active. With 0081 the LINE pipeline now gets its own key and the no-IDVS binary.
- Not covered by this run: fill_mode twice in one autorun, or a second autorun on an already-warm cache from a previous app launch. The cache tag bump to `0x76370108` covers stale 107 entries on upgraded installs; FTL installs fresh, so that path is not exercised here.
- fill_mode fps is 2585.3 vs 2630.6 on 107 (-1.7 %). The 107 number was not real (LINE draws drew nothing), so 108 is the first valid v7 baseline for this test on the A31.

## GPU timestamp calibration (0082 verified)
```
09:12:19.941 kbase: v7 JM, KBASE_IOCTL_MEM_JIT_INIT skipped; ...
09:12:19.953 W MESA: kbase: GPU timestamp counter measured at 25999419 Hz (25998838 / 26000000), cntfrq 13000000 Hz: using 26000000 Hz (PANVK_KBASE_TS_CALIBRATE=0 = cntfrq)
09:12:19.954 panvk: gpu_id 0x74021000 variant 0x2 arch v7 model Mali-G52 r1 ...
```
- The line appears 93 times (there are 94 kbase opens in total). The measured rate ranges from 25999405 to 26002196 Hz (within 0.01 %) and is snapped to exactly 26000000 Hz every time. No `keeping cntfrq` and no `TIMEINFO failed` lines (expected on this SoC: the GPU counter really is 2x cntfrq).
- bc_perf `DEVICE ... ts_period=38.462` (107: `76.923`).
- Calibration cost: about 12 ms per device open (`MEM_JIT_INIT` line at .941, measured line at .953; the default window is 2 x 5 ms).

### bc_perf: GPU time now matches wall time
| fmt | 107 gpu / wall ms (ratio) | 108 gpu / wall ms (ratio) |
|---|---|---|
| BC1_RGB_UNORM | 57.101 / 29.045 (1.97) | 28.259 / 28.772 (0.98) |
| BC3_UNORM | 81.546 / 41.076 (1.99) | 40.722 / 40.977 (0.99) |
| BC5_UNORM | 68.532 / 34.723 (1.97) | 34.342 / 35.958 (0.96) |
| BC6H_UFLOAT | 225.080 / 112.779 (2.00) | 113.217 / 113.907 (0.99) |
| BC7_UNORM | 118.749 / 59.689 (1.99) | 58.961 / 59.242 (1.00) |
| REF_RGBA8 | 14.410 / 7.416 (1.94) | 7.547 / 8.774 (0.86) |
| REF_ASTC4x4 | 3.261 / 1.825 (1.79) | 1.507 / 1.754 (0.86) |

- For the 13 BC formats `upload_gpu_ms / upload_wall_ms` is now 0.93-1.00 (107: 1.97-2.00). GPU time is slightly below wall time, as it should be (wall includes submit and wait overhead). The small REF uploads are 0.73-0.96 for the same reason.
- `A_ms` is now 2.97-3.00 and matches `wallA` (2.99-3.03). On 107 it was 5.95 = 2x wallA.
- The wall numbers themselves are unchanged (BC6H 113.9 vs 112.8-113.5 ms, BC7 59.2-60.0 vs 59.7-60.3 ms, wallA 2.99-3.03 vs 2.99-3.03), so this is a pure reporting fix: `timestampPeriod` for apps on this SoC is now correct.

## tess_perf: PASS, no LOWMEM
`RESULT tess_perf PASS mismatch=0 fps=0.4 ms=139544`, `TESS_PERF_FAILS=0`, no `LOWMEM` line.

| point | 108 avail_kb / rss_kb | 107 avail_kb / rss_kb |
|---|---|---|
| device (default pass) | 1361160 / 19536 | 1410272 / 19344 |
| gs=0 L=1 first frame | 1375516 / 48276 | 1400984 / 48588 |
| gs=1 L=1 first frame | 1340896 / 118528 | 1364248 / 118704 |
| lowest, default pass (gs=1 L=1 done) | 1318324 / 118528 | 1363896 / 118540 |
| teardown (default) | 1345480 / 41828 | 1365632 / 41956 |
| device (opt-off pass) | 1332560 / 52000 | 1384492 / 52140 |
| lowest, opt-off (gs=1 L=1 first frame) | 1238912 / 144648 | 1280240 / 144756 |
| teardown (opt-off) | 1270940 / 68448 | 1291684 / 68472 |

- Peak RSS 118.5 MB (default) and 144.6 MB (opt-off), the same as 107 to within 0.2 MB. MemAvailable minimum is 1.24 GB (107: 1.28 GB; the difference is the FTL device's background state at start, 1.36 vs 1.41 GB), far above the 300 MB LOWMEM floor.
- The v7 batch TLS/WLS lines are unchanged: TLS 8192 KiB per half (stack 2324 B/thread), WLS 32 KiB, tess batch budget 32 MiB.
- **Watch item, unchanged from 107:** teardown leaves +22 MB RSS after the first pass (19.5 -> 41.8 MB) and +16 MB after the second (52.0 -> 68.4 MB). Harmless at this size.

### Timings (ms per frame of 256 draws / 4096 patches)
| L | gs=0 default | gs=1 default | gs=0 opt-off | gs=1 opt-off | 107 gs=0 / gs=1 default |
|---|---|---|---|---|---|
| ref | 2.56 | - | 1.76 | - | 1.87 / - |
| 1 | 124.51 | 394.10 | 122.76 | 382.68 | 121.38 / 391.78 |
| 4 | 639.01 | 695.66 | 641.62 | 684.92 | 643.09 / 688.21 |
| 16 | 5120.92 | 1704.70 | 5256.25 | 1840.63 | 5119.38 / 1693.53 |
| mix | 2458.34 | 1505.60 | 2568.90 | 1584.00 | 2460.39 / 1503.95 |

- Every config is within 3 % of 107 (except the 2 ms `ref` sample, which is noise). Hashes `4807f3f5 / a03404c5 / cec55885 / d4463b35 / b34a251c` are identical to 107, the S9 and the G615 in every gs/opt variant.
- Default options are faster than opt-off for gs=0 L=16 / L=mix (2.6 %, 4.3 %) and gs=1 L=16 / L=mix (7.4 %, 5.0 %). gs=0 L=1, gs=1 L=1 and gs=1 L=4 are 1-3 % slower with defaults (107 had only gs=1 L=1 slower); noise level.

## Perf against 107 (same model, single sample each)
| test | 107 | 108 | delta |
|---|---|---|---|
| bc_perf total ms | 67921 | 66980 | -1.4 % |
| bc_perf BC6H / BC7 upload_wall_ms | 112.8-113.5 / 59.7-60.3 | 113.9 / 59.2-60.0 | flat |
| bc_perf wallA | 2.99-3.03 | 2.99-3.03 | flat |
| tessellation fps | 2714.2 | 2824.9 | +4.1 % |
| geometry fps | 2445.5 | 2862.7 | +17.1 % |
| clip_cull fps | 1867.5 | 2233.9 | +19.6 % |
| multi_viewport fps | 2392.4 | 2647.4 | +10.7 % |
| pipeline_stats fps | 1775.9 | 1938.9 | +9.2 % |
| fill_mode fps | 2630.6 (LINE drew nothing) | 2585.3 | -1.7 %, now valid |
| robust_image_access2 ria2 | 205.32 | 204.89 | flat |
| swapchain_lifecycle fps | 47.8 | 46.6 | -2.5 % |
| tess_perf | see table | | within 3 % |

- No regression. The GPU-bound numbers (bc_perf wall, ria2, tess_perf, swapchain) are flat to within 3 %.
- geometry, clip_cull, multi_viewport and pipeline_stats are 9-20 % faster. 0081 and 0082 do not touch these paths (FILL pipelines' keys only gained one constant bool; 0082 only changes `timestampPeriod`). These are short small-draw, CPU/submit-bound loops, so this is most likely run-to-run and unit-to-unit variance on FTL (a different physical A31 and background load). clip_cull is now back near the A06 v7K number (2372.7), which removes the "-21 % vs A06" note from 107.

## Observation (not a failure): 2 s JM warm-up sleep on every device open
- Each device open spends exactly 2.0 s between `stock JM atom layout ... stride 56` and the next kbase line (09:12:17.940 -> 19.941). It was the same on 107 (08:32:42.891 -> 44.893).
- Source: `src/panfrost/lib/kmod/kbase_kmod.c:1703-1706` in mesa-rc5g, a fixed `nanosleep({.tv_sec = 2})` after `kbase_probe_atom_stride()` on every JM device (added with jm-v9 001).
- With 94 kbase opens, that is about 3 minutes of this 463 s run, and 2 s of every app launch on JM GPUs.
- Possible follow-up (not applied, needs its own device test because the sleep was presumably added to avoid an early-submit fault): make it conditional or much shorter, for example only after a refused stride-72 probe, or replace it with a wait on the first job's completion; and add a `PANVK_KBASE_WARMUP_MS` knob to measure it on the A31, A06 and S9.

## Screenshots
- `screenshots/video-060s-fill_mode-pass.png`: 09:13, 5 pass / 0 fail; fill_mode PASS mismatch=0, FPS 2585.3.
- `screenshots/video-100s-tess_perf.png`: 09:14, 6 pass / 0 fail, tess_perf running.
- `screenshots/video-344s-39pass.png`: 09:20, final `39 pass / 0 fail / 0 skip`, uploaded id edb2f3bf.

Frames were taken with a static ffmpeg 7.0.2 arm64 build, kept in a scratch directory and not committed.

## Verdict
RC5-universal-108 passes 39/39 on the FTL A31 (v7). Both 108 checks from `builds/rc5-universal.md` are met: fill_mode 17/17, and the A31 logs `GPU timestamp counter measured at ~26000000 Hz` with bc_perf `upload_gpu_ms` now 0.93-1.00x `upload_wall_ms`. Still to check on other devices: the `keeping cntfrq` line on the A06 and S9, and fill_mode with an already-warm cache (two runs or two app launches).
