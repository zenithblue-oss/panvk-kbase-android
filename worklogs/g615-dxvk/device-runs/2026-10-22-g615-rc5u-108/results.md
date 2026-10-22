# g615 RC5-universal-108 regression run (2026-10-22)

Device: Poco X6 Pro (Mali-G615 MC6, arch v11), wifi adb 10.11.176.130:42737 (port unchanged). Battery 100% charging, 33.7-34.7 C.
APK /home/flux/build-out/apks/panprobe-rc5u-108.apk (sha256 05ca59df...), package dev.zenithblue.panvktest.rc5u (versionCode 15), `adb install -r` OK at 21:39.
The rc5 .so in the APK is `libvulkan_panfrost_rc5.so` sha256 276db559f1cf3274387e3fd614e34fb9870a07af206a26bff810a5a02d17e131 (matches the build record).
DRIVER line on every run: `PanVK-kbase RC5-universal-108 (Mesa 26.3.0-devel (git-03b39a69c0))`.

Baseline: RC5-universal-107, [../2026-10-22-g615-rc5u-107/results.md](../2026-10-22-g615-rc5u-107/results.md), autorun run=20261022-210206.
Changes since 107: jm-v7-wip 0081 (v7 polygon mode in the shader-cache key, `PAN_ARCH < 9` only) and 0082 (kbase timestamp calibration, JM only by default). Neither should reach v11 (CSF).

Method (same as 107): `am force-stop dev.zenithblue.panvktest.rc5u`, then
`am start -n dev.zenithblue.panvktest.rc5u/dev.zenithblue.panvktest.MainActivity --es driver rc5 --es autorun <all|test>`,
wait for `EXTERNAL READY run=<id>` in `/sdcard/Android/data/dev.zenithblue.panvktest.rc5u/files/autorun.txt`, check the DRIVER line, pull `files/runs/<id>`.

## Summary
- **autorun all (rc5, run=20261022-214210): 39 pass / 0 fail / 0 skip, total 39.** Same as 107. No reruns were needed. swapchain_lifecycle 63.2 fps.
- **tess_perf: all 18 hashes are identical to 107** (default and legacy, gs=0/1, all L). Per-draw times are within -1%..+6% of 107 (L=ref rows excluded); the new path is 5-14% faster than legacy except gs=1 L=4, where it is 4% slower (same on 107: 920.3 vs 882.9).
- **tessellation: 2569.7 fps inside autorun, 2789.2 / 2721.6 standalone.** 107: 2680.0 inside autorun, 2639.4 / 2769.9 standalone. Same range.
- **bc_perf (warm, standalone): 28353 / 28360 ms (mean 28357)**, 107 mean 27992: +1.3%, within noise. Large stable formats (BC6H, BC7) are within -4..+1%.
- **GPU timestamps on v11 unchanged.** `timestampPeriod` is 76.9231 ns (13 MHz) and `timestampValidBits` 64, identical to 107 (device_info.json). In bc_perf, upload_gpu_ms is below upload_wall_ms on all 40 PERF lines (none non-positive), and the BC sum ratio gpu/wall is 0.890 vs 0.873 on 107. Logcat `grep -i timestamp` has only 2 hits, both unrelated WindowManager fields (`capturedLinkTimestamp`, `...EducationTimestamp`), same count as 107; no `calibrat` lines. So 0082 is not active on CSF, as expected.
- Filtered logcat has no FATAL, SIGSEGV, DEVICE_LOST or lowmemorykiller lines.
- tess_perf MEM: peak rss 829 MB, min MemAvailable 2.36 GB (107: 769 MB / 2.43 GB). Far from the 300 MB LOWMEM guard.
- **No v11 difference was found.**

## autorun all (run=20261022-214210)
Full output: [raw/all/files/autorun.txt](raw/all/files/autorun.txt). Per-test logs and json: [raw/all/files/runs/20261022-214210/](raw/all/files/runs/20261022-214210/). Logcat (driver/app lines only): [raw/all-logcat-filtered.txt](raw/all-logcat-filtered.txt).

| test | 108 | 107 (210206) |
|---|---|---|
| clip_cull | PASS 2380.6 fps | PASS 2809.9 fps (RC5-universal: 2457.5) |
| multi_viewport | PASS 2637.8 fps | PASS 2641.2 fps |
| fill_mode | PASS 2287.5 fps | PASS 2546.5 fps (RC5-universal: 2349.6) |
| geometry | PASS 2607.4 fps | PASS 2819.7 fps (RC5-universal: 2657.6) |
| tessellation | PASS 2569.7 fps | PASS 2680.0 fps |
| pipeline_stats | PASS 2330.1 fps | PASS 2236.6 fps |
| bc_decode | PASS 15737 ms | PASS 15798 ms |
| bc_perf | PASS 43527 ms (cold, first run after install) | PASS 43303 ms (cold) |
| tess_perf | PASS 47534 ms | PASS 46847 ms |
| large_draw | PASS 8219 ms | PASS 8035 ms |
| submit_stress | PASS 30105 ms | PASS 30104 ms |
| swapchain_lifecycle | PASS 63.2 fps | PASS 63.8 fps |
| all other 27 | PASS | PASS |

The 1 s fps loops (clip_cull, fill_mode, geometry) are lower than 107 but inside the RC5-universal baseline's spread; these tests vary 10-15% run to run. fill_mode on v11 does not use the 0081 key (`PAN_ARCH < 9`), and its result hash/mismatch is unchanged (PASS, mismatch=0).

## Tessellation (fps, 1 s loop)
| run | fps |
|---|---|
| 214210 (inside autorun all) | 2569.7 |
| 214407 standalone | 2789.2 |
| 214420 standalone | 2721.6 |
| 107 210206 (inside autorun all) | 2680.0 |
| 107 standalone 210413 / 210417 | 2639.4 / 2769.9 |

## tess_perf us_draw (256 draws, 4096 patches), autorun 214210 vs 107 autorun 210206
| opt | gs | L | 108 | 107 | diff | hash = 107 |
|---|---|---|---|---|---|---|
| default | 0 | ref | 3.8 | 3.2 | | yes (4807f3f5) |
| default | 0 | 1 | 491.3 | 461.4 | +6% | yes (a03404c5) |
| default | 0 | 4 | 541.9 | 549.7 | -1% | yes (cec55885) |
| default | 0 | 16 | 2552.6 | 2560.7 | 0% | yes (d4463b35) |
| default | 0 | mix | 2521.3 | 2507.5 | +1% | yes (b34a251c) |
| default | 1 | 1 | 665.5 | 663.2 | 0% | yes |
| default | 1 | 4 | 912.6 | 920.3 | -1% | yes |
| default | 1 | 16 | 4486.3 | 4365.8 | +3% | yes |
| default | 1 | mix | 3505.5 | 3547.4 | -1% | yes |
| legacy | 0 | ref | 3.0 | 4.0 | | yes |
| legacy | 0 | 1 | 549.4 | 549.9 | 0% | yes |
| legacy | 0 | 4 | 605.1 | 607.9 | 0% | yes |
| legacy | 0 | 16 | 2962.7 | 2964.7 | 0% | yes |
| legacy | 0 | mix | 2928.3 | 2886.5 | +1% | yes |
| legacy | 1 | 1 | 699.7 | 699.0 | 0% | yes |
| legacy | 1 | 4 | 875.6 | 882.9 | -1% | yes |
| legacy | 1 | 16 | 5012.1 | 4918.1 | +2% | yes |
| legacy | 1 | mix | 3896.6 | 3837.7 | +2% | yes |

"legacy" = `PANVK_TESS_FUSE=0,PANVK_TESS_SETUP_STEP=0,PANVK_TESS_ARENAS=1,PANVK_TESS_WINDING_INLINE=0`. The default gs=0 L=1 +6% is the same size as the run-to-run swings seen on 107 (-3%..+7%). The L=ref rows are sub-5 us and not meaningful.

## bc_perf (upload_gpu_ms, warm standalone runs 214317 / 214355)
Totals: 108 28353 / 28360 ms (mean 28357). 107 27760 / 28224 ms (mean 27992).

| format | 108 mean (range) | 107 mean | diff |
|---|---|---|---|
| BC1_RGB_UNORM | 2.56 (1.89-3.24) | 2.82 | -9% (noise) |
| BC1_RGBA_SRGB | 2.65 (2.62-2.67) | 2.64 | 0% |
| BC2_UNORM | 4.17 (3.15-5.19) | 3.24 | +29% (one run 5.19; other 3.15 = 107) |
| BC3_UNORM | 2.93 (2.02-3.83) | 3.36 | -13% (noise) |
| BC3_SRGB | 3.44 (3.09-3.79) | 3.05 | +13% (noise) |
| BC4_UNORM | 0.63 (0.57-0.69) | 0.84 | noise |
| BC4_SNORM | 0.94 (0.85-1.03) | 1.53 | noise |
| BC5_UNORM | 1.54 (0.95-2.13) | 1.49 | noise |
| BC5_SNORM | 2.15 (2.12-2.17) | 1.49 | noise |
| BC6H_UFLOAT | 4.28 (4.19-4.38) | 4.41 | -3% |
| BC6H_SFLOAT | 4.67 (4.61-4.73) | 4.87 | -4% |
| BC7_UNORM | 5.18 (5.08-5.28) | 5.11 | +1% |
| BC7_SRGB | 5.11 (5.10-5.12) | 5.25 | -3% |

The small formats swing per run as on 107 (and the RC5-universal baseline); the totals and the large stable formats match. Not a regression.

### Timestamp sanity (v11, 0082 should be inactive)
- device_info.json: timestampPeriod 76.9231, timestampValidBits 64, timestampComputeAndGraphics true; identical to 107.
- bc_perf: upload_gpu_ms < upload_wall_ms on 40/40 PERF lines across both runs (107: 40/40); sum(gpu)/sum(wall) over BC formats 0.890 (107: 0.873).
- Logcat over autorun all: `grep -i timestamp` gives 2 WindowManager lines only (107: 2); `grep -i calibrat` gives 0.

## Screenshots
screenshots/: all-during.png (autorun at 4 tests, 21:39), all.png (end screen, 39 pass / 0 fail / 0 skip, uploading).

## Raw
- [raw/all/](raw/all/): full autorun pull (autorun.txt and runs/20261022-214210). The autorun uploaded as id=62fe3aae.
- [raw/standalone/files/runs/](raw/standalone/files/runs/): 214317 and 214355 (bc_perf), 214407 and 214420 (tessellation). Its autorun.txt is from the last tessellation run.
- raw/all-logcat-filtered.txt: logcat filtered to panvk/PanProbe/rc5u/FATAL/SIGSEGV/DEVICE_LOST/lmkd lines, captured over autorun all (before the standalone runs).
