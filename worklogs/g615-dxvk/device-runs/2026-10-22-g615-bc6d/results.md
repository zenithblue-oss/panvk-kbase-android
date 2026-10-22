# g615 bc6d device runs (2026-10-22)

Device: Poco X6 Pro (Mali-G615 MC6, arch v11), wifi adb 10.11.176.130:42737. APK panprobe-bc6d.apk, package
dev.zenithblue.panvktest.bc6d (versionCode 15). aapt2/sha256 checked: rc5 .so bd6ae709..., rc4 .so c395db09... (both match).
DRIVER lines: rc5 `PanVK-kbase RC5-dev-bc6d (git-5a07217f03)`; rc4 `RC4 (git-5437602824)`. Battery 100% charging, 33 C. Raw logs in raw/, screencaps in screenshots/.

## Summary
- **autorun all (rc5, run=20261022-172406): 38 pass / 0 fail / 1 skip, total 39.** `tessellation` PASS (2877 fps), `tess_perf` PASS (internal default-vs-legacy hashes match). The only skip is `swapchain_lifecycle` ("no surface: live surface not created in 20s", app not in front/other tab).
- **swapchain_lifecycle rerun alone (rc5, run=20261022-175600, screen on/unlocked per screencap): PASS** (120.3 fps). Effective 39/39.
- Steps 2 (tessellation toggles) not needed: tessellation passed. csf-v11 219 fixed the 213 failure.
- tess_perf x2 PASS; tessellation rc4 PASS (2587 fps vs rc5 2877 fps).
- bc_perf A/B: all 4 PASS; rc5 totals 28559/27799 ms, rc4 28363/28365 ms.

## Tessellation
| driver | run | result | fps |
|---|---|---|---|
| rc5 (inside autorun all) 172406 | PASS | 2877.3 |
| rc4 standalone 172811 | PASS | 2587.3 |

## tess_perf us_draw (256 draws, 4096 patches; run1 172611 / run2 172741). new = default opts, legacy = FUSE=0,SETUP_STEP=0,ARENAS=1,WINDING_INLINE=0
| gs | L | new run1 | new run2 | legacy run1 | legacy run2 | new/legacy (mean) | hash new = legacy |
|---|---|---|---|---|---|---|---|
| 0 | ref | 3.0 | 3.1 | 1.9 | 2.5 | | yes (4807f3f5) |
| 0 | 1 | 478.7 | 463.3 | 531.7 | 537.5 | 0.88x | yes (a03404c5) |
| 0 | 4 | 505.3 | 519.7 | 571.1 | 569.9 | 0.90x | yes (cec55885) |
| 0 | 16 | 2531.6 | 2645.6 | 2999.3 | 3016.1 | 0.86x | yes (d4463b35) |
| 0 | mix | 2461.3 | 2504.2 | 2831.0 | 2818.4 | 0.88x | yes (b34a251c) |
| 1 | 1 | 705.6 | 715.3 | 699.4 | 753.5 | 0.98x | yes |
| 1 | 4 | 853.4 | 874.5 | 896.4 | 815.2 | 1.00x | yes |
| 1 | 16 | 4601.7 | 4466.7 | 5017.0 | 5095.1 | 0.90x | yes |
| 1 | mix | 3469.0 | 3698.1 | 3833.5 | 4022.1 | 0.91x | yes |

New path is about 10-14% faster than legacy for gs=0, about 0-10% for gs=1 (L=1/L=4 gs=1 equal within noise). Hashes are identical new vs legacy in every config and across both runs, so the 219 stack fix closed the correctness gap seen in bc6c.

## BC A/B (upload_gpu_ms, mean of 2; range in parentheses; interleaved rc5 174617, rc4 174733, rc5 174849, rc4 175019)
Totals (bc_perf ms): rc4 28363/28365 (mean 28364); rc5 28559/27799 (mean 28179, -0.7%; run-to-run spread 760 ms exceeds the difference).
(A planned first rc4 run was lost to a harness race and replaced by a later rc4 run, so the sequence is rc5, rc4, rc5, rc4.)

| format | RC4 | RC5 bc6d | change |
|---|---|---|---|
| BC1_RGB_UNORM | 2.96 (2.94-2.99) | 2.76 (2.38-3.15) | -7% |
| BC1_RGBA_SRGB | 3.42 (2.92-3.91) | 3.02 (2.82-3.22) | -12% |
| BC2_UNORM | 4.13 (4.10-4.15) | 3.39 (3.16-3.62) | -18% |
| BC3_UNORM | 3.02 (2.91-3.14) | 3.05 (2.15-3.94) | +1% (noise) |
| BC3_SRGB | 4.89 (4.65-5.13) | 3.12 (2.60-3.63) | -36% |
| BC4_UNORM | 1.55 (1.17-1.92) | 0.97 (0.89-1.06) | -37% |
| BC4_SNORM | 1.68 (1.00-2.36) | 0.83 (0.53-1.13) | -50% |
| BC5_UNORM | 2.24 (1.71-2.77) | 1.73 (1.21-2.25) | -23% |
| BC5_SNORM | 3.08 (2.97-3.19) | 1.57 (0.81-2.33) | -49% |
| BC6H_UFLOAT | 7.54 (7.54-7.55) | 4.38 (4.33-4.43) | -42% |
| BC6H_SFLOAT | 8.26 (8.23-8.29) | 4.79 (4.71-4.87) | -42% |
| BC7_UNORM | 6.47 (6.47-6.47) | 5.20 (5.19-5.21) | -20% |
| BC7_SRGB | 6.01 (5.85-6.17) | 5.15 (5.12-5.18) | -14% |
| REF_RGBA8 | 2.46 | 2.24 | noise |
| REF_RG8 | 2.26 | 1.90 | noise |
| REF_R8 | 1.48 | 2.00 | noise (+35%, range overlaps) |
| REF_RGBA16F | 3.48 | 2.95 | noise |
| REF_B10G11R11 | 2.36 | 2.59 | noise |
| REF_ETC2_RGBA8 | 0.79 | 0.66 | noise |
| REF_ASTC4x4 | 0.84 | 0.98 | noise |

BC summary: rc5 bc6d is faster than RC4 on 12 of 13 BC formats (BC3_UNORM equal); stable large wins are BC6H (-42%), BC7 (-14..-20%), BC2/BC3_SRGB (-18..-36%). Matches bc6c within noise.

## Screenshots
screenshots/: all, tessperf1, tessperf2, tess-rc4, bcab2-rc5, bcab3-rc4, bcab4-rc5, bcab5-rc4, pre-swapchain (screen on/unlocked check), swapchain. Raw pulls in raw/ (bcab5-rc4 holds all four bc_perf runs).
