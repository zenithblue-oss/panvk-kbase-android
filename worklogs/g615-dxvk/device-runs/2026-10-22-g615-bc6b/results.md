# g615 bc6b device runs (2026-10-22)

Device: Poco X6 Pro (Mali-G615, duchamp) over wifi adb at 10.11.176.130:38911 (port differed from the 43681 in the brief; same phone).
APK: panprobe-bc6b.apk, package dev.zenithblue.panvktest.bc6b (versionCode 15). aapt2 and sha256 checked:
rc5 .so 1791bdd6..., rc4 .so c395db09... (both match). Installed fresh (empty shader cache). Thermal status 0 at the end.
All runs: `--es driver rc5 --es autorun <test>`, no env, cache ON. Every DRIVER line was `RC5-dev-bc6b`.
Each run identified by its new EXTERNAL READY id.

## Summary
- vkd3d_heap cold: PASS, all 4 cases (ssbo_heap, texel_heap, image_heap, mutable_heap), no watchdog, 205 ms.
- vkd3d_heap warm x4: 4/4 PASS.
- autorun all: 38/38 PASS.
- bc_perf x2: PASS, 30.6 s and 28.9 s (28.6 s inside autorun all).
- No failure, so the cache-disable control run was not needed.

## vkd3d_heap
| run | id | result | ms |
|---|---|---|---|
| cold (first after install) | 20261022-144248 | PASS 4/4 | 205 |
| warm 1 | 20261022-145332 | PASS | 275 |
| warm 2 | 20261022-145350 | PASS | 239 |
| warm 3 | 20261022-145409 | PASS | 262 |
| warm 4 | 20261022-145427 | PASS | 192 |

Inside autorun all, vkd3d_heap took 440 ms.

## autorun all (run=20261022-145640)
`AUTORUN DONE pass=38 fail=0 skip=0 total=38`. Slow tests: submit_stress 30124 ms, bc_decode 14641 ms,
swapchain_lifecycle 9009 ms, large_draw 8886 ms, bc_perf 28587 ms.

## bc_perf (upload_gpu_ms)
| format | run 1 (145744) | run 2 (145907) |
|---|---|---|
| BC1_RGB_UNORM | 2.537 | 2.280 |
| BC1_RGBA_SRGB | 2.605 | 2.555 |
| BC2_UNORM | 3.332 | 3.695 |
| BC3_UNORM | 3.035 | 4.332 |
| BC3_SRGB | 3.748 | 4.215 |
| BC4_UNORM | 1.254 | 1.329 |
| BC4_SNORM | 1.594 | 1.628 |
| BC5_UNORM | 2.492 | 1.115 |
| BC5_SNORM | 2.018 | 2.522 |
| BC6H_UFLOAT | 4.645 | 3.570 |
| BC6H_SFLOAT | 3.464 | 5.142 |
| BC7_UNORM | 5.029 | 3.675 |
| BC7_SRGB | 5.468 | 5.186 |
| REF_RGBA8 | 2.296 | 2.913 |
| REF_RG8 | 2.700 | 1.712 |
| REF_R8 | 1.273 | 2.376 |
| REF_RGBA16F | 3.338 | 3.549 |
| REF_B10G11R11 | 1.525 | 2.744 |
| REF_ETC2_RGBA8 | 1.046 | 0.611 |
| REF_ASTC4x4 | 1.038 | 1.011 |

Per-format upload_gpu_ms is noisy (single sample, ~1-5 ms); A..E sample timings are ~0.25-0.55 ms and stable.

MEM lines (run 1 / run 2 same shape):
- before_device: rss 10096 KiB, gpu_map 8 KiB
- full_feature_device: rss 46220 KiB, gpu_map 12196 KiB
- after_full_feature_device: rss 14260 KiB, hwm 46244 KiB, gpu_map 8 KiB
- end: run 1 heap_used 8840, rss 86868, hwm 176432, gpu_map 25720 KiB; run 2 heap_used 8840, rss 86932, hwm 176480, gpu_map 25720 KiB

## Notes
- First warm attempt: `am start` while PanProbe was still in the foreground only delivered the intent to the running
  instance ("Activity not started") and no new run happened. Fixed by `am force-stop` before each start; that attempt is discarded.
- raw/<name>/files is the full pulled files dir (accumulates earlier runs); the run of interest is the newest dir under runs/.
- Screenshots in screenshots/ (<run>-mid.png about 15 s after start, <run>-end.png, final.png).
