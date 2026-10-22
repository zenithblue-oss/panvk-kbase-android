# FTL run: Galaxy S9 (starlte, Mali-G72 MC18, Bifrost v6, API 29), PanProbe RC5-universal-107, game-loop scenario 2

- Project phonefinder-hub. gcloud outcome Passed, exit 0.
- Bucket `gs://test-lab-hrb456s6qt79m-kz4jkut67musi/panvk-v6-starlte-rc5u-107/`, copied to `raw/starlte-29-en-portrait/`: logcat (65551 lines), video.mp4 (345 s), game_loop_results/results_scenario_2.json, artifacts/.../autorun.txt and runs/20261022-082356/ (per-test logs and JSON). The APK in the bucket was not copied; it is `/home/flux/build-out/apks/panprobe-rc5u-107.apk` (rc5 .so `9a6308c8`).
- Driver line: `DRIVER slot=RC5 driverInfo=PanVK-kbase RC5-universal-107 (Mesa 26.3.0-devel (git-28b0346c01))`. This is the 0078 + 0079 + 0080 tree.
- GAMELOOP at 08:17:18.3, AUTORUN DONE at 08:23:56.7 (398 s). Auto-upload OK, id 91950dd9.

## Totals
`AUTORUN DONE pass=39 fail=0 skip=0 total=39 (39 pass / 0 fail / 0 skip)`. results_scenario_2.json agrees: pass 39, fail 0, skip 0.

- This is the first complete run on this device since tess_perf was added. 105 and 106 were killed by the low-memory killer in tess_perf.
- It is a full pass against the v7N baseline (38/38). The 39th test is tess_perf, which v7N did not have.
- Compliance: DXVK items=94, Bachata S4 items=114 and vkd3d items=100 all PASS, missingHard=[].
- Per-case SKIP lines inside passing tests are the same as v7N: large_draw `PANVK_FAN_CHUNKS unset`, large_draw `drawIndirectCount not exposed`, depth_stencil `d16s8_all format unsupported`. There are no `FAIL case` lines.
- Logcat: 0 `DEVICE_LOST`, 0 `event=0x51`, 0 `Fatal signal`.
- The lmkd/kill lines in logcat (about 160) are for other processes, mostly during FTL setup at 08:16-08:17. None of them names PanProbe or the testloop companion.

## tess_perf: PASS, no LOWMEM
`RESULT tess_perf PASS mismatch=0 fps=0.5 ms=104931`. `TESS_PERF_FAILS=0`, with no `FAIL LOWMEM` and no `MEM LOWMEM` line. It ran from 08:18:36 to 08:20:21 in pid 12716.

### Memory curve (PanProbeTess MEM lines)
The default pass is listed first; the opt-off pass (FUSE=0, SETUP_STEP=0, ARENAS=1, WINDING_INLINE=0) behaves the same.

| point | avail_kb | rss_kb |
|---|---|---|
| device | 1619104 | 19456 |
| gs=0 L=ref recorded / done | 1618356 / 1623308 | 24060 |
| gs=0 L=1 recorded | 1594836 | 111504 |
| gs=0 L=1 done | 1589460 | 115608 |
| gs=0 L=4 / L=16 / L=mix done | 1609504 / 1588372 / 1592956 | 115620 / 119472 / 119392 |
| gs=1 L=1 recorded / first frame | 1549964 / 1507296 | 161664 / 219120 |
| gs=1 L=4 / L=16 / L=mix done | 1527088 / 1562616 / 1562156 | 219028 / 219044 / 218980 |
| teardown (default) | 1564808 | 15384 |
| opt-off pass, lowest point (gs=1 L=1 first frame) | 1403472 | 219232 |
| teardown (opt-off) | 1407608 | 15432 |

- The process peaks at about 219 MB RSS. MemAvailable never drops below 1.40 GB, which is far above the 300 MB LOWMEM floor.
- Memory is flat within each config and goes back to 15 MB RSS at teardown.
- Compare 106: there, about 1.5 GB was used up within 4 s of recording gs=0 L=1.

### Batch TLS/WLS (0080)
```
08:18:41.893 panvk: v6 batch TLS 41472 KiB (stack 2324 B/thread), half 1, shared per half
08:18:41.893 panvk: v6 batch WLS 432 KiB, half 1, shared per half
08:18:41.929 panvk: v6 batch TLS 41472 KiB (stack 2324 B/thread), half 0, shared per half
08:18:41.930 panvk: v6 batch WLS 432 KiB, half 0, shared per half
08:18:43.056 / 43.057  same pair again for half 1 and half 0 (next command buffer, L=4)
```
- There is exactly one TLS and one WLS buffer per heap half per command buffer, as 0080 intended. The log is capped at the first 4 allocations per process.
- The tessellation stack is 2324 B/thread, which needs **40.5 MiB of TLS per half (81 MiB per command buffer)**. A plain draw needs 5184 KiB (304 B/thread). This 81 MiB matches the RSS step from 24 MB to 111 MB at gs=0 L=1.
- Under 0079 alone, about 256 batches each allocating 40.5 MiB would need about 10 GB. That explains the 105/106 LMK kills. 0080 fixes it.
- `v6 tess batch budget 32 MiB: new batch (varyings 43 MiB worst case, GS scratch 0 MiB)` still appears, capped at 3 lines. Each 16-patch draw still gets its own batch, but those batches now share the TLS and WLS buffers.
- Other processes: earlier tests log 5184 KiB TLS with 54 KiB WLS, and one 296 B/thread case with 864 KiB WLS (pid 12255, bc_decode at 08:17:49).

### Timings (ms per frame of 256 draws / 4096 patches; us_draw)
| L | gs=0 default | gs=1 default | gs=0 opt-off | gs=1 opt-off |
|---|---|---|---|---|
| ref (no tess) | 1.75 (6.8) | - | 1.68 (6.6) | - |
| 1 | 154.86 (604.9) | 235.30 (919.1) | 155.95 (609.2) | 235.56 (920.2) |
| 4 | 463.61 (1811.0) | 519.27 (2028.4) | 476.66 (1861.9) | 554.75 (2167.0) |
| 16 | 2265.30 (8848.8) | 1799.69 (7030.0) | 2411.81 (9421.1) | 1967.02 (7683.7) |
| mix | 1857.62 (7256.3) | 1745.18 (6817.1) | 1997.29 (7801.9) | 1892.27 (7391.7) |

- Hashes: ref `4807f3f5`, L=1 `a03404c5`, L=4 `cec55885`, L=16 `d4463b35`, L=mix `b34a251c`.
  - They are identical across gs=0/gs=1 and default/opt-off, and they match the G615 rc5u/bc6d runs, so output is correct.
- The default options are 0-9 % faster than opt-off; the largest gains are at L=16 and L=mix (up to 9 % at gs=1 L=16).
- Absolute cost is high: 155 ms per frame even at L=1. This is the known cost of one batch, and one fragment pass, per 16-patch draw under the 32 MiB budget.

## bc_perf and tessellation compared with v7N (same device)
| test | v7N | rc5u-107 | delta |
|---|---|---|---|
| bc_perf total ms | 42000 | 41911 | -0.2 % |
| bc_perf wallA, BC1..BC7 (13 formats) | 0.946-1.117 | 0.933-1.134 | noise, ±0.1 per format |
| tessellation FPS | 1679.9 | 1592.8 | -5.2 % |
| tessellation ms | 8242 | 7954 | |
| tessellation cases | 24/24 PASS | 24/24 PASS, same px counts | |

- bc_perf `upload_gpu_ms` and `A_ms..E_ms` are 0.000 on v6 in both runs, so only wall numbers can be compared.

Other FPS compared with v7N, which are worth watching:

| test | v7N | rc5u-107 | delta |
|---|---|---|---|
| geometry | 2495.5 | 1890.9 | **-24 %** |
| clip_cull | 2032.8 | 1919.6 | -5.6 % |
| pipeline_stats | 1302.1 | 1437.8 | +10 % |
| multi_viewport | 1621.1 | 1612.3 | -0.5 % |
| fill_mode | 1675.3 | 1660.2 | -0.9 % |
| swapchain_lifecycle | 59.5 | 59.9 | flat |

- The geometry drop is not a failure (mismatch=0), but it is larger than run-to-run noise. Possible causes: the 0079/0080 GS-path changes (heap-half picking and GS scratch per batch), or the larger shared TLS being sized for the worst-case stack.
  - A single FTL sample cannot tell these apart. A/B test on a device with `PANVK_V7_TLS_SHARE=0` and `PANVK_V7_TESS_GS_TIGHT=0` before RC5 release.

## Failures
None. No fix is needed for correctness or memory.

## Screenshots
- `screenshots/video-030s-start.png`: autorun at 3 pass.
- `screenshots/video-120s-tess_perf.png`: 08:20, 6 pass, tess_perf running.
- `screenshots/video-330s-39pass.png`: final `39 pass / 0 fail / 0 skip`, uploading results.

Frames were taken with a static ffmpeg 7.0.2 arm64 build, kept in a scratch directory and not committed.
