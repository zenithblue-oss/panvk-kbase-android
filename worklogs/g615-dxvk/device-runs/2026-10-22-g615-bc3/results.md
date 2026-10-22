# G615 (Poco X6 Pro) RC5-dev-bc3 vs bc2 vs RC4-lowmem, 2026-10-22

Device: Poco X6 Pro (2311DRK48I, Mali-G615), wifi adb. Package
dev.zenithblue.panvktest.bc3 (rc5 = RC5-dev-bc3, Mesa git-5a07217f03).
APK /home/flux/build-out/apks/panprobe-bc3.apk (already installed).

## Pass/fail

| run | result |
|---|---|
| rc5 bc3 full autorun (rc5-all) | 38/38 PASS, bc_decode ms=29862, bc_perf ms=57220 |
| cmp-4-rc5 (bc_decode,bc_perf) | 2/2 PASS, bc_decode 29328 ms, bc_perf 57924 ms |
| RC5 bc2 (earlier run) | 38/38 PASS, bc_decode 28115 (cmp avg 38765/39097), bc_perf 57993 (cmp 75088/76939) |
| RC4-lowmem (earlier run) | 37/38, bc_decode FAIL (raw=FAIL copy=FAIL, blit=PASS), bc_perf PASS (38271/37051) |

## IMPORTANT: cmp-1-rc4, cmp-2-rc5, cmp-3-rc4 are INVALID

The interrupted runner's pulls cmp-1/2/3 all contain only run 20261022-103113,
which is byte-identical to rc5-all (DRIVER line is RC5-dev-bc3 even in the
"rc4" dirs; wall.txt says "timeout"). They are stale copies of the full rc5
run, not fresh runs. No valid bc3-era RC4 data exists, so the RC4 column below
is from the bc2 results.md. The device files dir accumulates run dirs across
runs, so every pull also contains older run dirs; the fresh runs here are:
cmp-4-rc5 = 110248, cache-1 = 110438, cache-2 = 110556, timing = 110730.

## upload_gpu_ms per format, mem_KiB

bc3 = mean of 5 rc5 bc_perf runs (rc5-all, cmp-4, cache-1, cache-2, timing).
RC4 and bc2 = values from the bc2 results.md (mean of 2 runs; formats it did not
list are "-"). mem_KiB identical on all drivers (per bc2 notes).

| format | mem_KiB | RC4-lowmem | bc2 | bc3 |
|---|---|---|---|---|
| BC1_RGB_UNORM | 2732 | 2.89 | 4.95 | 3.45 |
| BC1_RGBA_SRGB | 2732 | 3.29 | 4.06 | 3.45 |
| BC2_UNORM | 5464 | 5.10 | 6.18 | 4.41 |
| BC3_UNORM | 5464 | 3.22 | 6.24 | 4.33 |
| BC3_SRGB | 5464 | 3.26 | 6.07 | 4.43 |
| BC4_UNORM | 2732 | 2.48 | 2.89 | 2.56 |
| BC4_SNORM | 2732 | 1.17 | 3.04 | 2.66 |
| BC5_UNORM | 5464 | 2.50 | 5.16 | 3.96 |
| BC5_SNORM | 5464 | 2.80 | 3.69 | 3.60 |
| BC6H_UFLOAT | 22644 | 8.42 | 8.81 | 7.93 |
| BC6H_SFLOAT | 44244 | 9.38 | 9.58 | 8.51 |
| BC7_UNORM | 5468 | 5.91 | 6.93 | 5.67 |
| BC7_SRGB | 5468 | 5.88 | 6.84 | 5.85 |
| REF_RGBA8 (ref) | 22644 | 3.14 | 1.99 | 2.30 |
| REF_ETC2_RGBA8 (ref) | 5952 | 1.04 | 1.14 | 0.93 |

| totals | RC4-lowmem | bc2 | bc3 |
|---|---|---|---|
| bc_perf total ms | ~37-38k | ~58k (full run) | 57189-57924 (5 runs) |
| bc_decode | FAIL | PASS | PASS (29-30 s) |
| pass/fail | 37/38 | 38/38 | 38/38 |

bc3 is faster than bc2 on every BC format (e.g. BC2 6.18 -> 4.41, BC3 6.24 -> 4.33,
BC5_UNORM 5.16 -> 3.96) and now within ~0.5-2 ms of RC4-lowmem; bc_perf total
wall time is unchanged vs bc2 (still ~57 s). Sampling A..E_ms unchanged (A/B/C
~0.44 ms, D ~0.22, E ~0.33).

## Cache check (rc5 bc_perf, two separate am starts)

| run | dir | bc_perf ms |
|---|---|---|
| cache-1 | cache-1-rc5 (110438) | 57189 |
| cache-2 | cache-2-rc5 (110556) | 57509 |
| timing (bc_timing) | timing-rc5 (110730) | 57507 |

No speedup on the second start: no cache hit effect (+0.3 s, within noise).
The app log contains `Failed to create /data/.cache for shader cache (Permission denied)---disabling.`
so the Mesa disk shader cache is disabled in the app (cache dir /data/.cache is
not writable; the driver is not given a writable cache path). logcat -d (filtered
for /data/.cache, disk_cache, .cache) shows only 37 "Access denied finding property
vendor.mesa.disk.cache.* / shader.cache.max.size" warnings per run
(cache-*/logcat-cache.txt), no other disk_cache errors. The bc "raw store" spills
to /data/user/0/dev.zenithblue.panvktest.bc3/cache, which works.

## PANVK_DEBUG=bc_timing (timing-rc5, passed via `--es env PANVK_DEBUG=bc_timing`)

PASS, 57507 ms. 39 "bc_timing" lines (3 per format, 13 formats). First-call
transcode/decode times at 2048x2048, 12 regions: BC1_RGB 7.9 ms (then 4.8, 3.7),
BC1_RGBA 6.7, BC2 8.9, BC3_UNORM 9.0, BC3_SRGB 5.5, BC4 3.8/4.1, BC5 6.5/6.7
("xcode"), BC6H 12.2/12.4 and BC7 9.4/9.3 ("decode"). Repeat calls are faster
(BC1: 7.9 -> 4.8 -> 3.7 ms).

## Screenshots

- cmp-4: screenshots/cmp-4-rc5-progress.png, screenshots/cmp-4-rc5-end.png
- cache-1: screenshots/cache-1-rc5-progress.png, screenshots/cache-1-rc5-end.png
- cache-2: screenshots/cache-2-rc5-progress.png, screenshots/cache-2-rc5-end.png
- timing: screenshots/timing-rc5-progress.png, screenshots/timing-rc5-end.png
- earlier runner: screenshots/rc5-all-*.png, cmp-1/2/3 screenshots (of stale/invalid runs)
