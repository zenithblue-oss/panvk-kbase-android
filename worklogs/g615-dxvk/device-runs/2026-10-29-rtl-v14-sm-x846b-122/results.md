# RTL v14 (Mali G1-Ultra) - build 122 / 122c (2026-10-29)

Device (adb): SM-X946B, mt6993, Android 17 (the 116 run listed SM-X846B). Section A = 122c (final, 0167, no env). Section B = original 122 failure and bisect.

## A. Build 122c: driverInfo "Mesa 26.3.0-devel (git-07132536df, p0167, 2026-10-29)", no env

### A1. PanProbe `all` (22:31-22:34 IST): 41/41 PASS, 0 fail, 0 skip
- tess_perf PASS (ms=0.87 per ref run, 20 s test), tessellation PASS mismatch=0 (4207 fps), submit_stress PASS, csf_event PASS, swapchain_lifecycle PASS (122 fps).
- 0161 incremental-rendering path on v14 now works with the default (old layout) heap: no CreateDevice failure.
- Logcat grep (full probe window): 0 DEVICE_LOST, 0 timeout snapshots, 0 tiler OOM, 0 CSF faults, 0 GPU resets.
- Raw: raw/122c-probe/probe-all/ (autorun.txt, logcat-full.txt.gz, run dir), probe-results.png.


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
