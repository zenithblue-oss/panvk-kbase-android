# RTL v14 (Mali G1-Ultra) - build 122 (p0165): REGRESSION, vkCreateDevice fails (2026-10-29 22:05-22:22 IST)

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
