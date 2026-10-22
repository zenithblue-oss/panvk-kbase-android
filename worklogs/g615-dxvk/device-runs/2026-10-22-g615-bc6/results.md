# G615 (Poco X6 Pro) RC5-dev-bc6 device runs, 2026-10-22

Device Poco X6 Pro (Mali-G615), wifi adb 10.11.176.130:43681. APK /home/flux/build-out/apks/panprobe-bc6.apk, package
dev.zenithblue.panvktest.bc6 (aapt2 verified). rc5 .so sha256 24b5f178..., rc4 .so sha256 c395db09... (both verified).
rc5 driverInfo "PanVK-kbase RC5-dev-bc6", rc4 "PanVK-kbase RC4 (git-5437602824)". Each run identified by its own
"EXTERNAL READY run=<id>"; every DRIVER line matched the requested driver. Thermal status was 0 (NONE) before every interleaved run
(raw/pre-*-thermal.txt). Raw pulls in raw/<tag>/, screenshots in screenshots/ (mid and/or end per run).

## Runs

| tag | driver | args | run id | result |
|---|---|---|---|---|
| rc5-all | rc5 | autorun all (no env) | 20261022-141939 | **37/38**: only vkd3d_heap FAIL (12219 ms watchdog). bc_decode PASS 14578 ms, bc_perf PASS 29057 ms |
| ab1-rc4 | rc4 | bc_perf | 20261022-142031 | PASS 28692 ms |
| vkd3d-nocache | rc5 | vkd3d_heap, env MESA_SHADER_CACHE_DISABLE=true | 20261022-142103 | PASS 303 ms |
| ab2-rc5 | rc5 | bc_perf | 20261022-142146 | PASS 28659 ms |
| env-cacheon-bcperf | rc5 | bc_perf, env MESA_SHADER_CACHE_DISABLE=false | 20261022-142230 | PASS 28191 ms |
| env-cacheon-vkd3d | rc5 | vkd3d_heap, env MESA_SHADER_CACHE_DISABLE=false | 20261022-142251 | FAIL 11619 ms |
| env-panvkdebug | rc5 | bc_perf, env PANVK_DEBUG= | 20261022-142332 | PASS 28933 ms |
| cacheoff-rc5 | rc5 | bc_perf, env MESA_SHADER_CACHE_DISABLE=true | 20261022-142426 | PASS 42581 ms |
| ab3-rc4 | rc4 | bc_perf | 20261022-142509 | PASS 29056 ms |
| ab4-rc5 | rc5 | bc_perf | 20261022-142558 | PASS 28472 ms |

Interleaved order: ab1-rc4, ab2-rc5, (env tests), ab3-rc4, ab4-rc5.
bc_decode PASSED in the full run. The only failure was vkd3d_heap (not a BC test), so the BC toggles
(PANVK_BC_BC6H_DECODE, PANVK_BC_BC7_DECODE, PANVK_BC_LM_OPTS) were skipped on the coordinator's instruction.

## Key finding: shader cache, not patch 209 and not env, explains the bc5 42 s

- Cache ON (default, no env, or env =false): bc_perf 28.2-29.1 s, MEM hwm 176744-184932 KiB.
  Cache OFF (env MESA_SHADER_CACHE_DISABLE=true): 42.6 s, hwm 399876 KiB.
- `PANVK_DEBUG=` env gave 28.9 s and env=false 28.2 s, so passing `--es env` does not change timing. Tests always run in an
  exec'd child through Native.run, env or not (MainActivity.kt buildEnv()).
- MainActivity.kt buildEnv() now collapses duplicate keys; its comment says the bc5 "cache ON" runs really ran with the cache off
  (first-match getenv() bug). That change was already in the working tree before this session and is not mine; not committed here.
- So bc5's 42 s was the cache being off. bc6 and bc4 at 28-29 s have the cache on. bc6 u_queue.c/.h are byte-identical to bc5's
  (patch 209 present in both).
- vkd3d_heap: with the cache really ON it FAILS (12.2 s watchdog; 11.6 s with env=false); with the cache OFF it PASSES in 303 ms.
  So the fork + shader-cache hang is not fixed by 209 in bc6 when the cache is on.

## bc_perf upload_gpu_ms per format (this session, bc6 APK)

Columns are run tags; cacheoff is rc5 with the cache off (excluded from the bc6 mean). Run-to-run spread is up to about 1 ms, so
small rows differ by noise. "bc6 mean" = mean of rc5-all, ab2-rc5, ab4-rc5.

MEM tag=end (rss/hwm/gpu_map KiB): rc4 188736/277648/114104 and 188548/277392/114104; rc5 cache on 94976/184932/25720,
87256/176792/25720, 87208/176744/25720; rc5 cache off 105516/399876/25720.

| format | mem_KiB | ab1-rc4 | ab3-rc4 | rc5-all | ab2-rc5 | ab4-rc5 | cacheoff | RC4 mean | bc6 mean (cache-on rc5) |
|---|---|---|---|---|---|---|---|---|---|
| BC1_RGB_UNORM | 2732 | 3.26 | 2.93 | 2.80 | 3.12 | 2.30 | 2.77 | 3.09 | 2.74 |
| BC1_RGBA_SRGB | 2732 | 3.07 | 2.86 | 2.19 | 2.65 | 2.19 | 3.70 | 2.97 | 2.34 |
| BC2_UNORM | 5464 | 4.68 | 4.58 | 4.78 | 3.46 | 3.41 | 4.79 | 4.63 | 3.88 |
| BC3_UNORM | 5464 | 2.99 | 3.58 | 4.13 | 3.94 | 4.15 | 2.75 | 3.29 | 4.07 |
| BC3_SRGB | 5464 | 3.13 | 3.08 | 4.15 | 2.84 | 2.55 | 2.63 | 3.10 | 3.18 |
| BC4_UNORM | 2732 | 1.14 | 2.02 | 0.72 | 1.86 | 1.54 | 1.59 | 1.58 | 1.37 |
| BC4_SNORM | 2732 | 1.22 | 2.01 | 1.59 | 0.68 | 1.32 | 1.57 | 1.62 | 1.20 |
| BC5_UNORM | 5464 | 2.62 | 2.15 | 2.26 | 2.24 | 2.26 | 2.21 | 2.38 | 2.25 |
| BC5_SNORM | 5464 | 2.38 | 1.72 | 2.26 | 1.90 | 1.17 | 1.73 | 2.05 | 1.78 |
| BC6H_UFLOAT | 21852 | 8.46 | 8.37 | 2.85 | 4.20 | 4.19 | 3.54 | 8.41 | 3.75 |
| BC6H_SFLOAT | 43700 | 9.96 | 9.36 | 3.20 | 4.07 | 5.03 | 5.18 | 9.66 | 4.10 |
| BC7_UNORM | 5468 | 5.83 | 6.50 | 5.07 | 3.77 | 5.29 | 5.07 | 6.16 | 4.71 |
| BC7_SRGB | 5468 | 6.02 | 5.97 | 4.98 | 5.96 | 5.80 | 5.56 | 6.00 | 5.58 |
| REF_RGBA8 | 22644 | 3.00 | 2.58 | 3.01 | 2.88 | 2.96 | 2.28 | 2.79 | 2.95 |
| REF_RG8 | 11508 | 2.93 | 2.66 | 2.23 | 2.79 | 1.99 | 2.99 | 2.80 | 2.34 |
| REF_R8 | 5940 | 2.62 | 1.20 | 1.32 | 1.30 | 1.13 | 1.30 | 1.91 | 1.25 |
| REF_RGBA16F | 44244 | 3.48 | 3.32 | 4.20 | 4.06 | 4.26 | 2.77 | 3.40 | 4.18 |
| REF_B10G11R11 | 22644 | 2.99 | 2.00 | 2.85 | 2.30 | 2.74 | 1.98 | 2.50 | 2.63 |
| REF_ETC2_RGBA8 | 5952 | 0.85 | 0.93 | 1.01 | 0.84 | 0.55 | 0.65 | 0.89 | 0.80 |
| REF_ASTC4x4 | 5952 | 0.90 | 0.88 | 0.95 | 1.05 | 0.63 | 0.93 | 0.89 | 0.88 |


## Comparison: RC4 (this session) vs bc4 vs bc5 vs bc6, upload_gpu_ms

bc4 from ../2026-10-22-g615-bc4/results.md (mean of 5), bc5 from ../2026-10-22-g615-bc5/results.md (mean of six rc5 runs:
rc5-all, perf-rc5-on1/on2/off, ab2-bc5, ab4-bc5; note bc5's "cache on" runs were really cache off). "-" = no row in that file.

| format | RC4 (this session, mean of 2) | bc4 (mean of 5) | bc5 (mean of 9 rc5/bc5 runs, see note) | bc6 (mean of 3, cache on) | bc6 vs RC4 | >=20% faster? |
|---|---|---|---|---|---|---|
| BC1_RGB_UNORM | 3.09 | 2.63 | 2.90 | 2.74 | -11% | no |
| BC1_RGBA_SRGB | 2.97 | 2.80 | 2.95 | 2.34 | -21% | YES |
| BC2_UNORM | 4.63 | 3.70 | 4.25 | 3.88 | -16% | no |
| BC3_UNORM | 3.29 | 3.40 | 2.81 | 4.07 | +24% | no |
| BC3_SRGB | 3.10 | 3.12 | 2.88 | 3.18 | +3% | no |
| BC4_UNORM | 1.58 | 1.24 | 1.75 | 1.37 | -13% | no |
| BC4_SNORM | 1.62 | 1.22 | 1.80 | 1.20 | -26% | YES |
| BC5_UNORM | 2.38 | 1.92 | 2.31 | 2.25 | -5% | no |
| BC5_SNORM | 2.05 | 2.13 | 2.20 | 1.78 | -13% | no |
| BC6H_UFLOAT | 8.41 | 8.57 | 8.64 | 3.75 | -55% | YES |
| BC6H_SFLOAT | 9.66 | 9.77 | 9.31 | 4.10 | -58% | YES |
| BC7_UNORM | 6.16 | 6.37 | 6.01 | 4.71 | -24% | YES |
| BC7_SRGB | 6.00 | 6.32 | 6.30 | 5.58 | -7% | no |
| REF_RGBA8 | 2.79 | 2.59 | 2.47 | 2.95 | +6% | no |
| REF_RG8 | 2.80 | - | 2.28 | 2.34 | -16% | no |
| REF_R8 | 1.91 | - | 1.84 | 1.25 | -35% | YES |
| REF_RGBA16F | 3.40 | - | 3.27 | 4.18 | +23% | no |
| REF_B10G11R11 | 2.50 | - | 2.09 | 2.63 | +5% | no |
| REF_ETC2_RGBA8 | 0.89 | 0.77 | 0.79 | 0.80 | -10% | no |
| REF_ASTC4x4 | 0.89 | - | 0.88 | 0.88 | -1% | no |


## Verdict (bc6 at least 20% faster than RC4?)

- YES: BC6H_UFLOAT (-55%), BC6H_SFLOAT (-58%), BC7_UNORM (-24%). Also BC1_RGBA_SRGB (-21%) and BC4_SNORM (-26%), but these sit near the noise band.
- NO: BC1_RGB (-11%), BC2 (-16%), BC3_UNORM (+24%, 4.07 vs 3.29 ms; bc6 samples 4.13/3.94/4.15 vs 2.75-3.58 elsewhere, possible small regression),
  BC3_SRGB (+3%), BC4_UNORM (-13%), BC5_UNORM (-5%), BC5_SNORM (-13%), BC7_SRGB (-7%).
- bc4 and bc5 were about equal to RC4 on BC6H (8.6/9.8 ms); bc6 cuts it to 3.75/4.10 ms (the 210 BC6H decode).
- REF_* rows are non-BC controls; their swings (REF_RGBA16F +23%, REF_R8 -35%) show the noise floor, so the small BC rows are inconclusive.
