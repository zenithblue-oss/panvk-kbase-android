# RTL SM-X926B (Immortalis-G720 MC12, v12) - release gate build 119 (2026-10-29), INCOMPLETE

Driver Mesa 26.3.0-devel git-0146a719f6 p0159 (build 119), panprobe-119.apk / panplay-119.apk. Samsung SM-X926B (gts10u), MT6989, API 35. Device online 14:50 IST. Scripts: /var/tmp/panvk/run-119-probe.sh, run-119-rtl.sh (under rtl.lock). Raw in `raw/`.

## GPU id
Recognised: `gpu_id=0xc8700000 arch=v12`, CSF uAPI 1.21 (sparse_kbase setup line, va_bits=48). HUD shows "Mali-G720-Immortalis MC12". No unknown-id warning.

## PanProbe autorun `all`: 41/41 PASS (14:52-14:55)
- `AUTORUN DONE pass=41 fail=0 skip=0 total=41`. DEVICE_LOST: NO. No timeout snapshots, no tiler heap OOM lines in the logcat grep.
- tess_perf PASS (the test that hung on v13 build 118): all variants completed, hashes a03404c5 (L=1), cec55885 (L=4), d4463b35 (L=16), b34a251c (mix) for gs=0 and gs=1, default and fuse-off options. L=4 gs=1 took 86.7 ms per frame of 256 draws.
- Screenshot: [sm-x926b-119-panprobe-results.png](sm-x926b-119-panprobe-results.png)

## PanPlay (one `adb install -r`, 15:05 done after ~6 min): only scenario 3 completed
Short window `--ei warmup_sec 10 --ei measure_sec 30`. Order planned 3 8 5 9 1 2 4 6 7.
| Scn | Test | Result | drv fps | Screenshot |
|---|---|---|---|---|
| 3 | D3D11 cube | PASS | 535.3 (1 s min/max 365/964, HUD shows 1088.7) | [s3](sm-x926b-119-panplay-s3.png) |
| 3 | D3D9 cube | PASS | 119.8 (vsync 120) | |
| 8 | (started 15:09:21) | NOT COMPLETED, device wedged | HUD showed D3D11 FL11_1 at 1088.7 fps at 15:10 | [s8](sm-x926b-119-panplay-s8.png) |
| 5 9 1 2 4 6 7 | | NOT RUN | | |

Scenario 3 logcat grep: 0 DEVICE_LOST, 0 timeout snapshots, 0 CSF faults, 0 tiler heap lines (the "fault" word matches are Android "default" noise).

## Memory
`dumpsys meminfo dev.zenithblue.panvklauncher` during s3 (`raw/meminfo-s3.txt`): TOTAL PSS 194,687 KB (RSS 342,575 KB; Java heap 41 MB, native heap 32 MB). v13 build 118 was 212,607 KB PSS.

## Device drop
Last good adb call: screencap at 15:10:02 IST, 40 s into scenario 8, with the HUD rendering normally (no DEVICE_LOST observed before it). At the next poll (~15:10:10 onwards) the script got no usable answers; at 15:23 `adb -s localhost:35395 shell` timed out (rc=124) even though `adb devices` still listed it as `device`, and it never recovered (rechecked 15:25). So the RTL session shell was wedged/dropped about 15:10 IST, ~20 min after the device came online, well before the 1 h limit (15:43). Cause unknown: could be the RTL service, not the driver; no logcat could be pulled after the drop, so no GPU fault evidence either way. No adb connect/disconnect, no sysfs, run script killed (my own), lock released, no background processes.

## Comparison
v13 SM-X930 build 118: PanProbe 39/40 (tess_perf hang, DEVICE_LOST), PanPlay 9/9. v12 build 119: PanProbe 41/41 including tess_perf. Same driver family, so the v13 tess_perf tiler-heap OOM does not reproduce on G720 MC12. Earlier G720 MC12 history (PROGRESS.md): 36/36 and 37/37 on rc1 era builds, gs_viewport_depth 8/8 (csf-v11/132). PanPlay s3 D3D11 cube 535 fps (G925 v13 s3: 882), D3D9 cube vsync-capped at 120 on both.

## Open
Scenarios 8, 5, 9, 1, 2, 4, 6, 7 not run on v12. Needs a fresh RTL session (PanPlay install is ~6 min).
