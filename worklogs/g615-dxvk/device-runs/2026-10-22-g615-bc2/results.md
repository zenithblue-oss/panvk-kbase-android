# G615 (Poco X6 Pro) RC5-dev-bc2 vs RC4-lowmem, 2026-10-22

Device: Poco X6 Pro (2311DRK48I, Mali-G615 MC6, v10 CSF), wifi adb.
APK: /home/flux/build-out/apks/panprobe-bc2.apk (sha256 8e989d8b...), package
dev.zenithblue.panvktest.bc. In-APK .so shas verified: rc5 = a166aabd...4232
(RC5-dev-bc2, Mesa git-5a07217f03), rc4 = c395db09...c8a5 (RC4-lowmem, git-5437602824).
Installed with `install --no-streaming -r` (Success).

## Full autorun (38 tests)

| driver | pass | fail | skip |
|---|---|---|---|
| RC5 bc2 | 38 | 0 | 0 |
| RC4-lowmem | 37 | 1 | 0 |

RC5 bc2: all 38 PASS, including bc_decode (ms=28115) and bc_perf (57993),
swapchain_lifecycle PASS (116.8 fps).
RC4-lowmem: the only failure is bc_decode (BC_DEVICE_FAILS=32: every BC
format raw=FAIL copy=FAIL, blit=PASS; the RC4 lowmem path does not do raw
store / copy correctly). Everything else PASS, swapchain 89.7 fps.
Raw files: rc5-all/, rc4-all/ (files/autorun.txt, files/runs/).

## BC comparison (bc_decode + bc_perf, alternating rc4, rc5, rc4, rc5)

Runs: cmp-1-rc4, cmp-2-rc5, cmp-3-rc4, cmp-4-rc5 (each: runs dir with logs/json).
bc_decode: rc4 FAIL in both runs (1127 ms, 1028 ms); rc5 PASS both (38765, 39097 ms).
bc_perf: PASS everywhere (rc4 38271, 37051 ms; rc5 75088, 76939 ms).

PanProbe memory figure: `mem_KiB` per format is identical on rc4 and rc5 for
all formats (BC1 2732, BC2/3/5 5464, BC4 2732, BC6H_UF 22644, BC6H_SF 44244,
BC7 5468 KiB; REF_RGBA8 22644, REF_ETC2_RGBA8 5952). No other raw-store or
image-memory lines in the logs besides "BC raw store spills to
/data/user/0/<pkg>/cache".

upload_gpu_ms (mean of the two alternating runs; upload_wall_ms is ~0.3-0.5 higher):

| format | rc4-lowmem | rc5 bc2 | rc5 / rc4 |
|---|---|---|---|
| BC1_RGB_UNORM | 2.89 | 4.95 | 1.71x |
| BC1_RGBA_SRGB | 3.29 | 4.06 | 1.23x |
| BC2_UNORM | 5.10 | 6.18 | 1.21x |
| BC3_UNORM | 3.22 | 6.24 | 1.94x |
| BC3_SRGB | 3.26 | 6.07 | 1.86x |
| BC4_UNORM | 2.48 | 2.89 | 1.17x |
| BC4_SNORM | 1.17 | 3.04 | 2.6x |
| BC5_UNORM | 2.50 | 5.16 | 2.06x |
| BC5_SNORM | 2.80 | 3.69 | 1.32x |
| BC6H_UFLOAT | 8.42 | 8.81 | 1.05x |
| BC6H_SFLOAT | 9.38 | 9.58 | 1.02x |
| BC7_UNORM | 5.91 | 6.93 | 1.17x |
| BC7_SRGB | 5.88 | 6.84 | 1.16x |
| REF_RGBA8 (ref) | 3.14 | 1.99 | n/a (noisy) |
| REF_ETC2_RGBA8 (ref) | 1.04 | 1.14 | n/a |

Per-run raw values for upload_gpu_ms: see the PERF lines in
cmp-*/*/*bc_perf.log. Sampling (A..E_ms) per-format draw times are
indistinguishable between drivers (A/B/C ~0.49 ms, D ~0.25 ms, E 0.25-0.9 ms).
So bc2 trades slower upload (the palette ETC2/EAC transcode on upload)
for correct BC raw/copy behaviour; steady-state sampling cost and memory
footprint are the same. Upload noise run-to-run is ~±1 ms (thermal
MODERATE/SEVERE), e.g. BC5_SNORM rc4 3.72 vs 1.88.
Note: this is rc5 bc2 only; no PANVK_DEBUG=bc_timing or LM_OPTS runs done.

## Telemetry (nativecode-agent, taken before each run)

| before | battery | temp C | thermal | avail MB |
|---|---|---|---|---|
| rc5 full | 91% | 35.4 | SEVERE | 978 |
| rc4 full | 90% | 35.5 | MODERATE | 1044 |
| cmp-1 rc4 | 87% | 34.8 | MODERATE | 1424 |
| cmp-2 rc5 | 87% | 34.8 | NONE | 1507 |
| cmp-3 rc4 | 86% | 34.8 | SEVERE | 1395 |
| cmp-4 rc5 | 86% | 34.8 | SEVERE | 1582 |

Total mem 11541 MB; not charging; no low-memory flag. (Telemetry is from the
NativeCode host agent.) The adb port changed mid-session (44497 -> 45213), same device.
