# g615 RC5-universal-107 regression run (2026-10-22)

Device: Poco X6 Pro (Mali-G615 MC6, arch v11), wifi adb 10.11.176.130:42737 (port unchanged). Battery 100% charging, 33.8-34.5 C.
APK /home/flux/build-out/apks/panprobe-rc5u-107.apk (sha256 0c3a8f51...), package dev.zenithblue.panvktest.rc5u (versionCode 15), `adb install -r` OK at 20:59.
The rc5 .so in the APK is `libvulkan_panfrost_rc5.so` sha256 9a6308c8be00f5ea58c59f9a399f329790fc7e2ee55a49a7544becc594eabbbd (matches the build record).
DRIVER line on every run: `PanVK-kbase RC5-universal-107 (Mesa 26.3.0-devel (git-28b0346c01))`.

Baseline: RC5-universal (ab7eaa92), [../2026-10-22-g615-rc5u/results.md](../2026-10-22-g615-rc5u/results.md), autorun run=20261022-185328.

Method (same as the baseline): `am force-stop dev.zenithblue.panvktest.rc5u`, then
`am start -n dev.zenithblue.panvktest.rc5u/dev.zenithblue.panvktest.MainActivity --es driver rc5 --es autorun <all|test>`,
wait for `EXTERNAL READY run=<id>` in `/sdcard/Android/data/dev.zenithblue.panvktest.rc5u/files/autorun.txt`, check the DRIVER line, pull `files/runs/<id>`.

## Summary
- **autorun all (rc5, run=20261022-210206): 39 pass / 0 fail / 0 skip, total 39.** Same as the baseline. No reruns were needed. swapchain_lifecycle passed inside the full run (63.8 fps).
- **tess_perf: every hash is identical to the baseline** (default and legacy, gs=0/1, all L). Per-draw times are within -6%..+7% of the baseline; the new path is still 7-13% faster than legacy. No regression.
- **tessellation: 2680.0 fps inside autorun, 2639.4 / 2769.9 standalone.** Baseline was 2629.5 inside autorun and 2258-2432 standalone. 107 is equal or higher, which is within this test's spread.
- **bc_perf (warm, standalone): 27760 / 28224 ms (mean 27992)**, baseline 27676 / 27517 (mean 27597): +1.4%, within noise. Per-format, BC1_RGB_UNORM (+28%) and BC3_UNORM (+35%) are slower and BC3_SRGB (-22%) is faster. These are the same formats that swung in the baseline run, and the swings cancel out in the total, so I read them as noise. The stable large formats (BC6H, BC7) are within -7..+4%.
- As expected (patches 0078-0080 touch jm-v7 code only), **no v11 difference was found.**
- Filtered logcat has no FATAL, SIGSEGV, DEVICE_LOST or lowmemorykiller lines.
- New in this PanProbe: tess_perf logs `MEM` lines. On v11 the peak was rss 769 MB with 2.43 GB MemAvailable (start 3.05 GB, teardown 3.19 GB / rss 32 MB). It stayed far from the 300 MB LOWMEM guard. There is no baseline value to compare with (the 106 PanProbe added these lines).

## autorun all (run=20261022-210206)
Full output: [raw/all/files/autorun.txt](raw/all/files/autorun.txt). Per-test logs and json: [raw/all/files/runs/20261022-210206/](raw/all/files/runs/20261022-210206/). Logcat (driver/app lines only): [raw/all-logcat-filtered.txt](raw/all-logcat-filtered.txt).

| test | 107 | baseline (185328) |
|---|---|---|
| clip_cull | PASS 2809.9 fps | PASS 2457.5 fps |
| multi_viewport | PASS 2641.2 fps | PASS 2770.3 fps |
| fill_mode | PASS 2546.5 fps | PASS 2349.6 fps |
| geometry | PASS 2819.7 fps | PASS 2657.6 fps |
| tessellation | PASS 2680.0 fps | PASS 2629.5 fps |
| pipeline_stats | PASS 2236.6 fps | PASS 2335.6 fps |
| bc_decode | PASS 15798 ms | PASS 15617 ms |
| bc_perf | PASS 43303 ms (cold cache, first run after install) | PASS 43968 ms (cold) |
| tess_perf | PASS 46847 ms | PASS 48331 ms |
| large_draw | PASS 8035 ms | PASS 8305 ms |
| submit_stress | PASS 30104 ms | PASS 30142 ms |
| swapchain_lifecycle | PASS 63.8 fps | PASS 63.1 fps |
| all other 27 | PASS | PASS |

## Tessellation (fps, 1 s loop)
| run | fps |
|---|---|
| 210206 (inside autorun all) | 2680.0 |
| 210413 standalone | 2639.4 |
| 210417 standalone | 2769.9 |
| baseline 185328 (inside autorun all) | 2629.5 |
| baseline standalone 185406 / 185422 / 185441 | 2376.4 / 2432.3 / 2257.8 |

## tess_perf us_draw (256 draws, 4096 patches), autorun 210206 vs baseline autorun 185328
| opt | gs | L | 107 | baseline | diff | hash = baseline |
|---|---|---|---|---|---|---|
| default | 0 | ref | 3.2 | 2.9 | | yes (4807f3f5) |
| default | 0 | 1 | 461.4 | 476.7 | -3% | yes (a03404c5) |
| default | 0 | 4 | 549.7 | 541.6 | +1% | yes (cec55885) |
| default | 0 | 16 | 2560.7 | 2645.5 | -3% | yes (d4463b35) |
| default | 0 | mix | 2507.5 | 2516.1 | 0% | yes (b34a251c) |
| default | 1 | 1 | 663.2 | 671.4 | -1% | yes |
| default | 1 | 4 | 920.3 | 861.5 | +7% | yes |
| default | 1 | 16 | 4365.8 | 4640.0 | -6% | yes |
| default | 1 | mix | 3547.4 | 3573.9 | -1% | yes |
| legacy | 0 | ref | 4.0 | 2.4 | | yes |
| legacy | 0 | 1 | 549.9 | 552.6 | 0% | yes |
| legacy | 0 | 4 | 607.9 | 591.9 | +3% | yes |
| legacy | 0 | 16 | 2964.7 | 2952.2 | 0% | yes |
| legacy | 0 | mix | 2886.5 | 2880.4 | 0% | yes |
| legacy | 1 | 1 | 699.0 | 689.6 | +1% | yes |
| legacy | 1 | 4 | 882.9 | 885.6 | 0% | yes |
| legacy | 1 | 16 | 4918.1 | 5256.9 | -6% | yes |
| legacy | 1 | mix | 3837.7 | 4112.4 | -7% | yes |

"legacy" = `PANVK_TESS_FUSE=0,PANVK_TESS_SETUP_STEP=0,PANVK_TESS_ARENAS=1,PANVK_TESS_WINDING_INLINE=0`. The baseline's gs=1 L=4 new had a standalone value of 919.6, so the +7% there is noise. The L=ref rows are sub-5 us and not meaningful.

## bc_perf (upload_gpu_ms, warm standalone runs 210336 / 210406)
Totals: 107 27760 / 28224 ms (mean 27992). Baseline 27676 / 27517 (mean 27597).

| format | 107 mean (range) | baseline mean | diff |
|---|---|---|---|
| BC1_RGB_UNORM | 2.82 (2.44-3.19) | 2.20 | +28% (bc6d 2.76; noise-level) |
| BC1_RGBA_SRGB | 2.64 (2.32-2.96) | 2.72 | -3% |
| BC2_UNORM | 3.24 (3.22-3.25) | 3.21 | +1% |
| BC3_UNORM | 3.36 (2.96-3.76) | 2.49 | +35% (bc6d 3.05; noise-level) |
| BC3_SRGB | 3.05 (2.13-3.98) | 3.93 | -22% (noise) |
| BC4_UNORM | 0.84 (0.57-1.12) | 1.60 | noise |
| BC4_SNORM | 1.53 (1.43-1.63) | 1.17 | noise |
| BC5_UNORM | 1.49 (1.23-1.75) | 1.54 | noise |
| BC5_SNORM | 1.49 (0.87-2.12) | 1.70 | noise |
| BC6H_UFLOAT | 4.41 (4.12-4.70) | 4.22 | +4% |
| BC6H_SFLOAT | 4.87 (4.86-4.88) | 4.86 | 0% |
| BC7_UNORM | 5.11 (5.11-5.11) | 5.47 | -7% |
| BC7_SRGB | 5.25 (5.14-5.36) | 5.17 | +2% |

BC1/BC3 vary a lot between runs: each 107 run's values span the baseline's range, and the baseline itself saw BC3_SRGB at +26% vs bc6d. I don't count this as a regression. If it matters, an interleaved A/B with the baseline APK would settle it.

## Screenshots
screenshots/: all-during.png (autorun at 4 tests, 20:59), all.png (end screen, 39 pass / 0 fail / 0 skip, uploading).

## Raw
- [raw/all/](raw/all/): full autorun pull (autorun.txt and runs/20261022-210206). The autorun uploaded as id=485512f1.
- [raw/standalone/files/runs/](raw/standalone/files/runs/): 210336 and 210406 (bc_perf), 210413 and 210417 (tessellation). Its autorun.txt is from the last tessellation run.
- raw/all-logcat-filtered.txt: logcat filtered to panvk/PanProbe/rc5u/FATAL/SIGSEGV/DEVICE_LOST/lmkd lines, captured over autorun all (before the standalone runs).
