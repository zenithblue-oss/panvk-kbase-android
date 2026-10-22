# G615 (Poco X6 Pro) RC5-dev-bc5 device runs, 2026-10-22

Device: Poco X6 Pro (Mali-G615, arch v11), wifi adb. APK /home/flux/build-out/apks/panprobe-bc5.apk,
package dev.zenithblue.panvktest.bc5 (aapt2 verified). Embedded rc5 .so sha256 e41f0fbf2f1e9d6301f2... and
rc4 .so sha256 c395db09ccf160d0419b... (both verified). rc5 driverInfo = "PanVK-kbase RC5-dev-bc5 (Mesa 26.3.0-devel (git-5a07217f03))",
rc4 = "PanVK-kbase RC4 (git-5437602824)". Every run's DRIVER line matched the requested driver. Each run was identified
by its own "EXTERNAL READY run=<id>" and only that run dir was pulled (raw/<tag>/).
rc5 runs use `--es env MESA_SHADER_CACHE_DISABLE=false` (cache ON) unless stated. The rc4 run used `=true` (cache off; the cache is irrelevant for the rc4 .so baseline).

## Runs

| tag | driver | args | run id | result |
|---|---|---|---|---|
| rc5-all | rc5, cache ON | autorun all | 20261022-132753 | **38/38 PASS**, vkd3d_heap PASS 401 ms, bc_decode PASS 13781 ms, bc_perf PASS 41705 ms |
| vkd3d-1 | rc5, cache ON | vkd3d_heap | 20261022-132822 | PASS 285 ms |
| vkd3d-2 | rc5, cache ON | vkd3d_heap | 20261022-132948 | PASS 212 ms |
| perf-rc4 | rc4 | bc_perf | 20261022-133402 | PASS 29180 ms |
| perf-rc5-on1 | rc5, cache ON | bc_perf | 20261022-133543 | PASS 42051 ms |
| perf-rc5-on2 | rc5, cache ON | bc_perf | 20261022-133710 | PASS 41756 ms |
| perf-rc5-off | rc5, cache OFF (coordinator request) | bc_perf | 20261022-133831 | PASS 42330 ms |

## 1-2. vkd3d_heap fix (csf-v11 209 fork-safe util_queue)

Target met: 38/38 with the cache ON, vkd3d_heap passes in 401 ms (full run), 285 ms and 212 ms standalone
(bc4 failed it with the 12 s ICD watchdog on all four cases). Not flaky across 3 runs.
Cache check: /data/data/dev.zenithblue.panvktest.bc5/cache/mesa_shader_cache exists (6 subdirs, ~2 MB after these runs),
so the cache dir is in use. Filtered logcat of the vkd3d runs (raw/vkd3d-*/logcat-filtered.txt) shows no "Failed to create ... disabling"
message and no cache errors; only the harmless "Access denied finding property vendor.mesa.disk.cache.*" lines.

## 3. bc_perf

Totals (ms): rc4 29180 | rc5 cache ON 42051, 41756 (41705 in the full run) | rc5 cache OFF 42330.
A warm cache does NOT recover ~28 s, and the cache costs nothing measurable (ON 41.8-42.1 s vs OFF 42.3 s).
The slowdown is therefore not the cache. bc4 measured 28.0-28.7 s (5 runs) for the same test, so bc5 bc_perf is about 13-14 s (+47%) slower
than bc4 and about 13 s slower than rc4 in this session (rc4 29.2 s, in line with its bc4-session 28.4-28.8 s).

upload_gpu_ms per format (ms; one run each; spread about 1 ms between runs, so differences under ~1 ms are noise; columns are run tags):

| format | mem_KiB | rc5-all | perf-rc4 | perf-rc5-on1 | perf-rc5-on2 | perf-rc5-off | ab1-bc4 | ab2-bc5 | ab3-bc4 | ab4-bc5 |
|---|---|---|---|---|---|---|---|---|---|---|
| BC1_RGB_UNORM | 2732 | 3.56 | 3.40 | 2.72 | 2.39 | 2.73 | 2.30 | 2.92 | 3.19 | 3.05 |
| BC1_RGBA_SRGB | 2732 | 2.91 | 3.20 | 3.91 | 2.45 | 3.22 | 2.08 | 2.75 | 3.67 | 2.47 |
| BC2_UNORM | 5464 | 5.38 | 4.61 | 3.82 | 3.77 | 3.48 | 4.96 | 3.68 | 3.57 | 5.35 |
| BC3_UNORM | 5464 | 2.48 | 3.48 | 3.02 | 2.78 | 2.93 | 4.08 | 2.97 | 2.90 | 2.69 |
| BC3_SRGB | 5464 | 2.46 | 4.88 | 3.06 | 3.10 | 3.05 | 3.04 | 2.61 | 2.62 | 3.02 |
| BC4_UNORM | 2732 | 1.52 | 2.45 | 1.56 | 1.91 | 1.87 | 1.89 | 1.79 | 1.47 | 1.86 |
| BC4_SNORM | 2732 | 1.68 | 1.60 | 1.65 | 1.87 | 1.84 | 0.77 | 1.83 | 1.34 | 1.92 |
| BC5_UNORM | 5464 | 2.52 | 1.87 | 2.05 | 1.69 | 2.51 | 2.14 | 2.49 | 2.43 | 2.62 |
| BC5_SNORM | 5464 | 2.51 | 2.60 | 1.80 | 2.06 | 2.22 | 2.63 | 2.26 | 1.13 | 2.35 |
| BC6H_UFLOAT | 22644 | 8.89 | 8.27 | 8.55 | 8.33 | 8.46 | 8.54 | 8.95 | 8.34 | 8.66 |
| BC6H_SFLOAT | 44244 | 9.64 | 9.11 | 9.16 | 9.17 | 9.18 | 9.77 | 9.41 | 9.91 | 9.32 |
| BC7_UNORM | 5468 | 6.24 | 5.95 | 5.77 | 5.92 | 6.08 | 5.82 | 6.11 | 5.88 | 5.93 |
| BC7_SRGB | 5468 | 7.30 | 5.75 | 6.47 | 6.02 | 5.84 | 6.26 | 6.03 | 5.83 | 6.14 |
| REF_RGBA8 | 22644 | 2.48 | 2.85 | 2.40 | 2.27 | 2.66 | 2.48 | 2.65 | 2.52 | 2.36 |
| REF_RG8 | 11508 | 1.73 | 1.67 | 2.83 | 2.84 | 1.81 | 2.57 | 1.30 | 1.75 | 3.15 |
| REF_R8 | 5940 | 2.57 | 1.88 | 1.39 | 2.53 | 1.17 | 1.28 | 2.17 | 2.53 | 1.24 |
| REF_RGBA16F | 44244 | 2.63 | 3.16 | 3.14 | 3.29 | 3.52 | 2.96 | 3.52 | 2.75 | 3.50 |
| REF_B10G11R11 | 22644 | 2.33 | 2.31 | 2.66 | 1.90 | 1.55 | 2.61 | 1.77 | 1.55 | 2.31 |
| REF_ETC2_RGBA8 | 5952 | 0.65 | 0.71 | 0.95 | 0.75 | 1.01 | 0.74 | 0.56 | 1.02 | 0.80 |
| REF_ASTC4x4 | 5952 | 1.08 | 0.76 | 0.68 | 0.98 | 0.74 | 0.57 | 0.79 | 0.53 | 1.02 |

Compared with bc4 (earlier results.md and the same-session runs below): the per-format upload_gpu_ms is NOT slower: BC1 ~2.4-3.9 (bc4 2.6-2.8),
BC2 3.5-3.8 (3.70), BC3 2.8-3.1 (3.1-3.4, slightly faster), BC4 1.5-1.9 (1.2), BC5 1.7-2.5 (1.9-2.1), BC6H 8.3-9.2 (8.6-9.8),
BC7 5.8-7.3 (6.3-6.4). So per-upload GPU time is unchanged; the extra ~13 s of wall time is spent outside upload_gpu_ms
(CPU-side work in the test: encode/setup or the BC raw store; not isolated here).

MEM tag=end (bc_perf), mean of the four bc5 runs vs others: see the A/B table below and these rows.

| | rc4 | rc5 bc5 | rc5 bc4 |
|---|---|---|---|
| heap_used_KiB | 8840 | 8840 | 8840 |
| rss_KiB | 188436 | 109404-110124 | 90520-90772 |
| hwm_KiB | 277340 | 378736-380200 | 178796-179052 |
| gpu_map_KiB | 114104 | 28416 | 28416 |

## Same-session A/B (bc4 APK vs bc5 APK, both rc5, cache ON, bc_perf, interleaved)

| tag | APK | run id | bc_perf ms | rss_KiB (end) | hwm_KiB (end) |
|---|---|---|---|---|---|
| ab1-bc4 | bc4 | 20261022-134037 | 28891 | 90772 | 179052 |
| ab2-bc5 | bc5 | 20261022-134218 | 42096 | 109664 | 378928 |
| ab3-bc4 | bc4 | 20261022-134329 | 27680 | 90520 | 178796 |
| ab4-bc5 | bc5 | 20261022-134512 | 42599 | 109684 | 379196 |

Pre-run telemetry (raw/pre-<tag>-telemetry.json, pre-<tag>-thermal.txt): thermal_status NONE / dumpsys "Thermal Status: 0", battery 33.9 C,
charging at 100% before every run, CPU core6 sensor 75.8 C each time. Not thermal and not session drift: bc4 is ~28 s and bc5 is ~42 s on every interleaved run.
The bc4 APK's rc5 driverInfo is "RC5-dev-bc4", same Mesa git hash as bc5.

bc5 is a real +14 s (+49%) regression on bc_perf total. Per-format upload_gpu_ms are the same on both (see table: all within ~1 ms noise, no format carries the
extra time), and the per-test PERF phases A..E and wallA are the same too (~0.25-0.55 ms). So the extra wall time is not in the measured GPU upload or the
per-format phases; it is in work outside them (setup between formats: pipeline/shader compilation or BC encode on CPU, not isolated with the data collected).
bc5 also has ~19 MiB more RSS and 2.1x the peak hwm (379 MB vs 179 MB; bc3-encoder LM_OPTS run was 555 MB). gpu_map is identical (28416 KiB).
Hypothesis (untested): patch 209 (util_queue restart in fork child) changes worker count/concurrency so compile jobs in bc_perf run slower and with more
memory; the cache setting is not involved (OFF 42330 ms).

## Screenshots

screenshots/: ab1-bc4, ab2-bc5, ab3-bc4, ab4-bc5 (-mid/-end), rc5-all-mid.png, rc5-all-late.png, rc5-all-end.png, and <tag>-mid.png / <tag>-end.png for vkd3d-1, vkd3d-2, perf-rc4,
perf-rc5-on1, perf-rc5-on2, perf-rc5-off.
