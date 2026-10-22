# G615 (Poco X6 Pro) RC5-dev-bc4 vs bc3 vs RC4-lowmem, 2026-10-22

Device: Poco X6 Pro (Mali-G615), wifi adb. APK /home/flux/build-out/apks/panprobe-bc4.apk,
package dev.zenithblue.panvktest.bc4 (aapt2 verified). Embedded rc5 .so sha256 a7b7c58e79ea61fb... and
rc4 .so sha256 c395db09ccf160d0... (both verified). rc5 driverInfo = "PanVK-kbase RC5-dev-bc4".
rc4 driverInfo = "PanVK-kbase RC4 (git-5437602824)".

Every run's DRIVER line was checked against the requested driver and matched (rc4 runs show
slot=RC4, rc5 runs slot=RC5). `--es driver rc4` IS honoured by MainActivity
(apps/panvk-test/.../MainActivity.kt:232-241 lowercases the extra and maps "rc4" to DriverType.RC4).
The bc3 "rc4" invalidity was not a parse bug: those pulls only contained stale run 20261022-103113.
Each run here was identified by its own "EXTERNAL READY run=<id>" and only that run dir was pulled.

## Runs (run id -> dir)

| run | driver | args | id | result |
|---|---|---|---|---|
| rc5-all | rc5 | autorun all | 20261022-123238 | **37/38**: only vkd3d_heap FAIL. bc_decode PASS 13850 ms, bc_perf PASS 28419 ms |
| cmp-1-rc4 | rc4 | bc_perf,bc_decode | 20261022-123403 | bc_decode FAIL (793 ms), bc_perf PASS 28353 ms |
| cmp-2-rc5 | rc5 | bc_perf,bc_decode | 20261022-124049 | 2/2 PASS, bc_decode 153 ms, bc_perf 28590 ms |
| cmp-3-rc4 | rc4 | bc_perf,bc_decode | 20261022-124323 | bc_decode FAIL (886 ms), bc_perf PASS 28750 ms |
| cmp-4-rc5 | rc5 | bc_perf,bc_decode | 20261022-124433 | 2/2 PASS, bc_decode 153 ms, bc_perf 28013 ms |
| cache-1-rc5 | rc5 | bc_perf | 20261022-124545 | PASS 28354 ms |
| cache-2-rc5 | rc5 | bc_perf | 20261022-124702 | PASS 28693 ms |
| lmopts-rc5 | rc5 | bc_perf, env PANVK_BC_LM_OPTS=0xc30000 | 20261022-124848 | PASS 57875 ms |

(bc_decode ms in the full run is long because it runs among all tests; standalone it is 153 ms
on rc5. RC4 bc_decode FAIL: raw=FAIL copy=FAIL blit=PASS for every BC format, same as earlier RC4-lowmem.)

## upload_gpu_ms per format (ms, mean)

RC4 = mean of 2 fresh runs (cmp-1, cmp-3). bc4 = mean of 5 rc5 bc_perf runs (rc5-all, cmp-2, cmp-4, cache-1,
cache-2). bc3 = mean of 5 runs from the bc3 results.md. LM_OPTS = 1 run of bc4 .so with the old bc3 encoder
settings. Per-format run-to-run spread is up to about 1.1 ms (rc4) / 1.4 ms (bc4), so differences under
~0.3 ms are noise. mem_KiB is identical on all drivers.

| format | mem_KiB | RC4-lowmem (fresh) | bc3 | bc4 | bc4 LM_OPTS=0xc30000 | bc4 vs RC4 speed |
|---|---|---|---|---|---|---|
| BC1_RGB_UNORM | 2732 | 2.63 | 3.45 | 2.63 | 3.24 | tie |
| BC1_RGBA_SRGB | 2732 | 2.91 | 3.45 | 2.80 | 3.10 | tie / slightly faster |
| BC2_UNORM | 5464 | 4.64 | 4.41 | 3.70 | 4.86 | bc4 faster (-20%) |
| BC3_UNORM | 5464 | 4.20 | 4.33 | 3.40 | 4.69 | bc4 faster (-19%) |
| BC3_SRGB | 5464 | 4.01 | 4.43 | 3.12 | 4.66 | bc4 faster (-22%) |
| BC4_UNORM | 2732 | 1.81 | 2.56 | 1.24 | 2.88 | bc4 faster (-31%) |
| BC4_SNORM | 2732 | 2.38 | 2.66 | 1.22 | 2.78 | bc4 faster (-49%) |
| BC5_UNORM | 5464 | 2.40 | 3.96 | 1.92 | 4.89 | bc4 faster (-20%) |
| BC5_SNORM | 5464 | 2.71 | 3.60 | 2.13 | 3.80 | bc4 faster (-21%) |
| BC6H_UFLOAT | 22644 | 8.34 | 7.93 | 8.57 | 8.47 | tie (+3%, noise) |
| BC6H_SFLOAT | 44244 | 9.98 | 8.51 | 9.77 | 9.82 | tie |
| BC7_UNORM | 5468 | 6.12 | 5.67 | 6.37 | 5.89 | tie / slightly slower (+4%) |
| BC7_SRGB | 5468 | 5.88 | 5.85 | 6.32 | 6.39 | tie / slightly slower (+7%) |
| REF_RGBA8 (ref) | 22644 | 2.57 | 2.30 | 2.59 | 1.94 | - |
| REF_ETC2_RGBA8 (ref) | 5952 | 0.77 | 0.93 | 0.77 | 0.98 | - |

(bc4 vs bc3: bc4 is faster on every BC format except BC6H/BC7, which are about equal. The bc3 table has no
REF_RG8/R8/RGBA16F/B10G11R11/ASTC rows; full bc4 and RC4 numbers for them are in the per-run bc_perf logs.)

## Totals, pass/fail, memory

| | RC4-lowmem (fresh) | bc3 | bc4 |
|---|---|---|---|
| full autorun | not run | 38/38 | **37/38 (vkd3d_heap FAIL)** |
| bc_decode | FAIL (raw/copy FAIL, blit PASS) | PASS | PASS |
| bc_perf total ms | 28353, 28750 | 57189-57924 (5 runs) | 28013-28693 (5 runs, mean 28414) |
| bc_perf total, old bc3 encoder (LM_OPTS) | - | - | 57875 |

bc_perf wall time halved vs bc3 (57.5 s -> 28.4 s) and equals RC4's wall time on this device now
(RC4 earlier measured 37-38 s in the bc2/bc3 session, so the RC4 baseline moved; compare same-session only).

MEM lines at tag=end (bc_perf run):

| tag=end | RC4-lowmem | bc4 | bc4 LM_OPTS |
|---|---|---|---|
| heap_used_KiB | 8840 | 8840 | 8840 |
| rss_KiB | 188184 / 188692 | 90404-92712 | 348392 |
| hwm_KiB | 277036 / 277608 | 178668-181176 | 554964 |
| gpu_map_KiB (/dev/mali mapping) | 114104 | 28416 | 26964 |

tag=full_feature_device: RC4 rss 211972 KiB, gpu_map 177256 KiB; bc4 rss 46108-46212 KiB, gpu_map 12196 KiB.
tag=before_device: rss ~10 MB, gpu_map 8 KiB on both. after_full_feature_device: rss ~14 MB both.
The old bc3 encoder opts (LM_OPTS=0xc30000) cost 2x wall time and ~3.8x RSS vs the bc4 default.

## Verdict per format: does bc4 beat RC4-lowmem?

| format | speed | memory |
|---|---|---|
| BC1_RGB / BC1_RGBA | tie (no worse) | yes: gpu_map 28 vs 114 MiB, RSS 90 vs 189 MiB (process-wide) |
| BC2 / BC3 (UNORM, SRGB) | **yes**, 19-22% faster | yes |
| BC4 (UNORM, SNORM) | **yes**, 31-49% faster | yes |
| BC5 (UNORM, SNORM) | **yes**, about 20% faster | yes |
| BC6H (UF, SF) | tie (within noise) | yes |
| BC7 (UNORM, SRGB) | tie to slightly slower (+4-7%, inside run spread) | yes |
| correctness | bc4 passes bc_decode, RC4 fails it | |

Memory: per-format mem_KiB is identical, but process memory is much lower on bc4 (RSS about -52%, hwm -35%,
/dev/mali mapping -75%). The bc_perf wall time is a tie with RC4 (28.4 vs 28.5 s).

## REGRESSION: vkd3d_heap fails on bc4 (passed on bc3 in 372 ms)

All four cases (ssbo_heap, texel_heap, image_heap, mutable_heap) hit "ICD watchdog timeout (12 s)" -> RESULT FAIL, ~47.5 s total.
Isolation (rc5, `--es autorun vkd3d_heap`, files in vkd3d_heap-isolation/):

| env | id | result |
|---|---|---|
| (default) original full run | 123238 | FAIL |
| default rerun | 123936 | FAIL (not flaky) |
| PANVK_GPU_PRERAST_ARENA=eager | 123541 | FAIL |
| PANVK_BC_RAW=store | 123709 | FAIL |
| PANVK_BC_LM_OPTS=0xc30000 | 123820 | FAIL |
| MESA_DISK_CACHE_SINGLE_FILE=0 | 124231 | FAIL |
| **MESA_SHADER_CACHE_DISABLE=true** | 124134 | **PASS, 266 ms** |

The Mesa disk shader cache is the cause (cache is now enabled in bc4; see below). The BC and prerast toggles are irrelevant.

## Cache check (rc5 bc_perf twice back to back)

cache-1 28354 ms, cache-2 28693 ms: no speedup (+0.3 s, noise); bc_perf does not exercise cached pipelines
in a way that helps. No "Failed to create /data/.cache ... disabling" message appears in any bc4 run log, and
the app has /data/user/0/dev.zenithblue.panvktest.bc4/cache/mesa_shader_cache (218 subdirs, multi-file cache),
so a cache dir is now used. Remaining logcat noise: "Access denied finding property vendor.mesa.disk.cache.*"
(harmless). BC raw store spills to .../cache and works.

## Screenshots

screenshots/: rc5-all-progress.png, rc5-all-end.png, cmp-1-rc4-end.png, cmp-2-rc5-end.png, cmp-3-rc4-end.png,
cmp-4-rc5-end.png, cache-1-rc5-end.png, cache-2-rc5-end.png. The lmopts screencap and
mid-run shots for the short comparison runs were skipped (screencap/pull hung on the final run; short runs finish
in about 30 s).
