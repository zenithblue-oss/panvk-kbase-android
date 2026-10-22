# FTL run: Galaxy S9 (starlte, Mali-G72 MC18, Bifrost v6, API 29), PanProbe RC5-universal-108, game-loop scenario 2

- Project phonefinder-hub. gcloud exit 0.
- Bucket `gs://test-lab-hrb456s6qt79m-kz4jkut67musi/panvk-v6-starlte-rc5u-108/`, copied to `raw/starlte-29-en-portrait/`:
  - logcat (65058 lines) and video.mp4 (406 s)
  - game_loop_results/results_scenario_2.json
  - artifacts/.../autorun.txt and runs/20261022-095045/ (85 files: per-test logs and JSON)
- The APK in the bucket was not copied. It is `/home/flux/build-out/apks/panprobe-rc5u-108.apk` (rc5 .so `276db559`).
- Driver line: `DRIVER slot=RC5 driverInfo=PanVK-kbase RC5-universal-108 (Mesa 26.3.0-devel (git-03b39a69c0))`. This is mesa-rc5g `03b39a69`: 107 plus jm-v7-wip 0081 and 0082.
- GAMELOOP started at 09:44:09.1 and AUTORUN DONE came at 09:50:45.9 (397 s). Auto-upload OK, id dcdb3380.

## Totals
`AUTORUN DONE pass=39 fail=0 skip=0 total=39 (39 pass / 0 fail / 0 skip)`. results_scenario_2.json agrees: 39 PASS.

- Every test passes, with mismatch=0. This is the same as 107 (39/39).
- fill_mode passes all 17 of its cases (`CASE ... PASS`), so the 0081 v6 cache-key change did not break it. The keys are new on this build, so this was a cold cache.
- Compliance: DXVK items=94, Bachata S4 items=114 and vkd3d items=100 all PASS, missingHard=[].
- Per-case SKIP lines are the same as in 107:
  - large_draw: `PANVK_FAN_CHUNKS unset`
  - large_draw: `drawIndirectCount not exposed`
  - depth_stencil: `d16s8_all format unsupported`
- There are no `FAIL case` lines.
- Logcat has no `DEVICE_LOST`, no `event=0x51` and no `Fatal signal` lines. No lmkd kill names PanProbe or the testloop companion.

## GPU timestamp calibration (0082): no log line, timestampPeriod is still 0
- Logcat has **no** `kbase: timestamp calibration` line and no `GPU timestamp counter measured` line. There are 92-93 kbase device opens (for example `JM driver, uAPI version 11.16`), and their other `I MESA` lines do appear, so info-level logging is not being filtered.
- bc_perf `DEVICE Mali-G72 MC18 ... ts_bits=64 ts_period=0.000`. 107 shows the same thing on this device. The A31 (v7) on the same 108 build shows `ts_period=38.462`.
- bc_perf `upload_gpu_ms` and `A_ms..E_ms` are 0.000 for all 20 formats. `upload_wall_ms` is 8.6 / 7.5 (BC1), 10.5-11.0 (BC2/BC3), 7.9-8.1 (BC4), 8.7-9.2 (BC5), 24.2-24.8 (BC6H) and 14.3-14.9 (BC7). There is no gpu/wall ratio to report.

### Why
- `kbase_calibrate_timestamp_frequency()` returns without logging when `!props->gpu_can_query_timestamp || !props->timestamp_frequency`.
- `gpu_can_query_timestamp` is `gpu_id != 0`, and gpu_id is 0x62210010, so that is true.
- `timestamp_frequency` comes only from `mrs cntfrq_el0` (csf-v11 057, `kbase_kmod.c:510-517` in mesa-rc5g).
- `panvk_get_gpu_system_timestamp_period()` returns 0 only when one of those two props is 0, and `timestampPeriod` uses that function on v6 and v7 alike (jm-v7 021).
- So on this S9 (Exynos 9810), **CNTFRQ_EL0 reads 0 from EL0.** Exynos firmware often does not program CNTFRQ, and the kernel takes the arch-timer rate from the device tree instead.
- This is an inference: the code has no other path to `ts_period=0`, but no log prints cntfrq directly.
- Effect: since jm-v7 021 the S9 exposes timestamp queries (`timestampComputeAndGraphics`, 64 valid bits) with `timestampPeriod = 0`. Any app that converts query deltas to time gets 0 ns. Rendering is not affected.
- 0082 assumed the S9 would "keep cntfrq". In fact it never gets as far as measuring.

### Proposed fix (not built)
In `kbase_calibrate_timestamp_frequency()` (`src/panfrost/lib/kmod/kbase_kmod.c`), treat `timestamp_frequency == 0` as "unknown, measure it" instead of returning early:

```c
   if (mode == 0)
      return;
   const bool no_cntfrq = !props->timestamp_frequency;
   if (!props->gpu_can_query_timestamp || (mode < 0 && is_csf && !no_cntfrq))
      return;
   ...
   /* after the stability / 1 MHz..1 GHz check */
   if (no_cntfrq) {
      /* snap to a common SoC timer rate within 1 %, else round to 1 kHz */
      static const double rates[] = { 19.2e6, 24e6, 25e6, 26e6, 38.4e6, 52e6 };
      uint64_t freq = (uint64_t)(meas / 1000.0 + 0.5) * 1000;
      for (unsigned i = 0; i < ARRAY_SIZE(rates); i++)
         if (fabs(meas - rates[i]) <= 0.01 * rates[i]) { freq = rates[i]; break; }
      mesa_logw("kbase: cntfrq_el0 is 0; GPU timestamp counter measured at %.0f Hz "
                "(%.0f / %.0f): using %" PRIu64 " Hz", meas, f[0], f[1], freq);
      props->timestamp_frequency = freq;
      props->timestamp_cycles_to_ns_factor = 1e9 / freq;
      return;
   }
```

- Also log the early return at info level (`cntfrq 0, timestamps unknown`), so a run cannot skip the calibration silently again.
- If the measurement is unstable and cntfrq is 0, the safer choice is to clear `gpu_can_query_timestamp`, which sets `timestampComputeAndGraphics = false`, rather than advertise `timestampPeriod = 0`.
- To check it: on the S9, look for `cntfrq_el0 is 0 ... using 26000000 Hz` (26 MHz is expected for Exynos 9810), `ts_period=38.462`, and bc_perf `upload_gpu_ms` at roughly 0.9-1.0x `upload_wall_ms`.

## tess_perf: PASS, no LOWMEM
`RESULT tess_perf PASS mismatch=0 fps=0.5 ms=104906` (107: 104931). `TESS_PERF_FAILS=0`, with no `LOWMEM` lines.

| point | avail_kb | rss_kb |
|---|---|---|
| device | 1639820 | 19356 |
| gs=0 L=1 recorded / done | 1606340 / 1598620 | 111512 / 115616 |
| gs=1 L=1 first frame (default) | 1535960 | 219240 |
| teardown (default) | 1591508 | 15672 |
| opt-off lowest (gs=1 L=1 first frame) | **1435924** | 219532 |
| teardown (opt-off) | 1454772 | 15736 |

- RSS peaks at about 219 MB and returns to about 15.7 MB at teardown.
- MemAvailable bottoms out at 1.44 GB. In 107 the low point was 1.40 GB. Both are far above the 300 MB floor.
- The v6 batch TLS/WLS lines (0080) are unchanged: 5184 KiB TLS (304 B/thread) and 54 KiB WLS per half for the plain-draw tests.
- The hashes match 107 and are identical across gs=0/1 and default/opt-off: ref `4807f3f5`, L=1 `a03404c5`, L=4 `cec55885`, L=16 `d4463b35`, L=mix `b34a251c`.

Timings in ms per frame:

| L | gs=0 def 107 / 108 | gs=1 def 107 / 108 | gs=0 off 107 / 108 | gs=1 off 107 / 108 |
|---|---|---|---|---|
| ref | 1.75 / 2.73 | - | 1.68 / 1.75 | - |
| 1 | 154.86 / 154.85 | 235.30 / 236.43 | 155.95 / 155.93 | 235.56 / 238.71 |
| 4 | 463.61 / 464.14 | 519.27 / 527.18 | 476.66 / 476.96 | 554.75 / 535.71 |
| 16 | 2265.30 / 2271.25 | 1799.69 / 1795.13 | 2411.81 / 2413.42 | 1967.02 / 1953.80 |
| mix | 1857.62 / 1858.99 | 1745.18 / 1762.83 | 1997.29 / 1996.28 | 1892.27 / 1911.15 |

- The tessellated configs are flat to within about 1.5 %.
- The default ref is 2.73 ms against 1.75 ms. That is a single 1-2 ms number, and the opt-off ref is unchanged, so this is noise.

## Perf compared with 107 (same model, different FTL unit and run)
| test | 107 | 108 | delta |
|---|---|---|---|
| bc_perf total ms | 41911 | 42022 | +0.3 % |
| bc_perf wallA, BC1..BC7 (13 formats) | 0.933-1.134 | 0.934-1.069 | noise (BC1_RGB 1.134 -> 0.937, BC7_SRGB 0.958 -> 1.069) |
| tessellation FPS | 1592.8 | 1786.7 | +12 % |
| geometry FPS | 1890.9 | 2285.6 | **+21 %** |
| clip_cull FPS | 1919.6 | 1912.5 | -0.4 % |
| fill_mode FPS | 1660.2 | 1703.3 | +2.6 % |
| multi_viewport FPS | 1612.3 | 2005.4 | +24 % |
| pipeline_stats FPS | 1437.8 | 1743.9 | +21 % |
| swapchain_lifecycle FPS | 59.9 | 59.8 | flat |

- 0081 changes only v6 cache keys (v6 binaries are unchanged, with IDVS off), and 0082 changes nothing on this device. So the FPS gains are FTL unit and run variance, not driver changes. The A31 108 run showed the same 9-20 % spread on these short loops.
- geometry is back near the v7N number (2495.5). The -24 % "geometry drop" flagged in 107 was most likely variance, not 0079/0080. It still deserves an on-device A/B before release.

## Failures
None in the test results. One issue to fix: 0082 does not cover this device because cntfrq_el0 is 0, so `timestampPeriod` stays 0 on the S9 (see the proposed fix above).

## Screenshots
- `screenshots/video-030s-3pass.png`: 09:44, autorun at 3 pass.
- `screenshots/video-150s-6pass-tess_perf.png`: 09:46, 6 pass, tess_perf running.
- `screenshots/video-400s-39pass.png`: 09:50, final `39 pass / 0 fail / 0 skip`, uploading results.

Frames were taken with a static ffmpeg 7.0.2 arm64 build, kept in a scratch directory and not committed.
