# RTL v14 (Mali G1-Ultra) - build 122 / 122c (2026-10-29)

Device (adb): SM-X946B, mt6993, Android 17 (the 116 run listed SM-X846B). Section A = 122c (final, 0167, no env). Section B = original 122 failure and bisect.

## A. Build 122c: driverInfo "Mesa 26.3.0-devel (git-07132536df, p0167, 2026-10-29)", no env

### A1. PanProbe `all` (22:31-22:34 IST): 41/41 PASS, 0 fail, 0 skip
- tess_perf PASS (ms=0.87 per ref run, 20 s test), tessellation PASS mismatch=0 (4207 fps), submit_stress PASS, csf_event PASS, swapchain_lifecycle PASS (122 fps).
- 0161 incremental-rendering path on v14 now works with the default (old layout) heap: no CreateDevice failure.
- Logcat grep (full probe window): 0 DEVICE_LOST, 0 timeout snapshots, 0 tiler OOM, 0 CSF faults, 0 GPU resets.
- Raw: raw/122c-probe/probe-all/ (autorun.txt, logcat-full.txt.gz, run dir), probe-results.png.


### A2. PanPlay (install 22:36 via `adb install -r`, runs 22:39-23:00 IST), 10 s warm-up + 30 s measure, order 3 8 5 9 1 2 4
Scenarios 6 and 7 NOT run (session time ended; s4 was cut mid-run, partial dir only). Logcat grep per scenario: 0 DEVICE_LOST, 0 timeout snapshots, 0 CSF faults, 0 tiler OOM (all passes ok). D3D9 cube: PASS in s1/s2/s3/s8, 120 fps, no DEVICE_LOST.
Screenshots: raw/122c-panplay/*.png (one per scenario, at t=30 s). Per-scenario harness output: raw/122c-panplay/sN/results.txt, meminfo-sN.txt. MangoHud stats not captured.

| Scn | task | result | drv fps 122c (116) | 1% low 122c (116) | PSS MB (launcher process, 122c) |
|---|---|---|---|---|---|
| 1 | d3d11 cube | PASS | 241.0 (846.5) | 111.0 (104.4) | 221 |
| 2 | d3d9 cube | PASS | 120.0 (120.0) | 64.9 (60.5) | 225 |
| 3 | d3d11 / d3d9 cube | PASS / PASS | 254.8 / 120.0 (967.2 / 120.0) | 109.0 / 66.6 (138.5 / 64.4) | 202 |
| 5 | d3d12 SquidRoom | PASS | 200.4 (288.6) | n/a | 143 |
| 8 | d3d11 FL10_0/10_1/11_0/11_1 | PASS x4 | 857.9 / 850.8 / 855.5 / 847.8 (1187 / 1203 / 1186 / 1189) | 208 / 214 / 212 / 212 (155 / 167 / 168 / 157) | 190 |
| 8 | d3d10 / d3d9 | PASS / PASS | 231.6 / 120.0 (768.7 / 120.0) | 104.7 / 61.5 (72.5 / 62.1) | |
| 9 | d3d12 FL all | PASS (ok=true, 116 harness-timeout) | 376.7 (351.0) | n/a | 187 |
| 9 | d3d11 FL all | PASS | 843.4 (1205.9) | 213.4 (184.2) | |
| 4 | d3d11 old/default, d3d9 old/default | partial, cut at 23:00 | see raw/122c-panplay/s4 | | 224 |
| 6, 7 | D3D12 gears / FL | not run | | | |
PSS: 116 run did not record PSS, no memory comparison possible. 1% low from the harness (dxvk 1%-low column).

### A3. Comparison vs 116 and FLAGS
- No rc4 run exists in these dirs for v14.
- PanProbe: 41 vs 40 tests (122c adds one). Per-test time and fps in raw/122c-probe/probe-vs-116.md. Only flag: pipeline_stats fps -19% (looks like noise, single run); no other test worse beyond 30% time / 15% fps.
- PanPlay FLAG, not explained: D3D11 uncapped cube and FL tests are 25-70% slower than 116 (cube 241-255 vs 808-967; FL10_0..11_1 ~850 vs ~1190; d3d10 232 vs 769; d3d12 SquidRoom 200 vs 289; d3d11 FL all 843 vs 1206). Capped D3D9 (120 fps) equal; D3D12 FL-all slightly better. Steady across the run (1 s min/max within 3% of mean), so not jitter. Not bisected: the device is a different model (SM-X946B, load average ~28 at start) than the 116 run (SM-X846B), so a device/clock difference is possible; but it could also be a 120 to 122c regression. Needs an A/B of 116 vs 122c on THIS device (PanPlay s1 only, ~3 min each).
- 1% low equal or better than 116 in most rows (higher is better).

## B. Build 122 (p0165) original failure + bisect

Device reported by adb: model SM-X946B (not SM-X846B as in the 116 run), board mt6993, display 2960x1848, Android 17. Same G1-Ultra v14 class.
driverInfo "Mesa 26.3.0-devel (git-3506474d17, p0165, 2026-10-29)". Run stopped early on purpose after the bisect (PanPlay not run: DXVK needs a device).

## PanProbe `all`: 3 PASS / 26 FAIL before abort (aborted after ~26 tests, all FAIL)
Every test fails with `FAIL CreateDevice r=-3 line=189` (VK_ERROR_DEVICE_LOST), ~10 s each (timeout). Instance/compliance still reports (DXVK 94 PASS, vkd3d 100 PASS, D3D FL 20 PASS).
Decisive logcat lines (raw/probe-logcat-first-fail.txt):
- `kbase: tiler heaps 4 initial / 400 max chunks of 1024 KiB` / `tiler_heap_init layout=16 bytes`
- `kbase: timeout on subqueue 1: seqno 0 ... stream progress 0x0, insert 192, extract 192, active 0`
- `timeout snapshot subqueue 0/1/2`: all three CS never run any job (marks 0x0, progress 0x0) during device init.
- no tiler OOM / CSF fault / GPU reset line; plain timeout of the very first submit.

## A/B (single `tessellation` test, same device, same session, raw/ab-bisect/)
| build | patch level | tessellation |
|---|---|---|
| 116 | git-f7e78953ac | PASS 3805 fps |
| 117 | 1a0a81aa00 | PASS 3206 |
| 118 | a0412457ab | PASS 3288 |
| 119 | p0159 | PASS 2939 |
| 120 | p0160 | PASS 3939 |
| 121 | p0163 | FAIL CreateDevice |
| 121b / 122a | p0163 | FAIL |
| 122 | p0165 | FAIL |
| 122b | p0166 | FAIL |
| 122 + `PANVK_KBASE_TILER_IR=0` | p0165 | PASS 3703 fps |

Regression window is 0161..0163. 0161 (kbase incremental rendering on tiler heap, new queue-group/tiler-heap layout) is the cause: `PANVK_KBASE_TILER_IR=0` (restores old layout) fixes it. 0162 (BC prewarm) and 0163 (JM only) are not involved. 0161 on v14 was never tested before this run.
Not run: PanProbe tess_perf, PanPlay 1-9, D3D9 cube (would all hit CreateDevice).

## Device
Stayed online. No adb connect/disconnect, no sysfs. Only own processes killed (run script, logcat). Final state: panprobe-122 installed (-r), panplay not reinstalled.
