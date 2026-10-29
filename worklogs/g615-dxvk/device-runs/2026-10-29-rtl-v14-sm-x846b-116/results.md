# RTL SM-X846B (Mali-G1-Ultra, v14) - release gate build 116 (2026-10-29)

Driver git-f7e78953ac (build 116), panprobe-116.apk / panplay-116.apk. Device online 11:53 IST, session limit ~12:50.
Scripts: /var/tmp/panvk/run-116-probe.sh, run-116-rtl.sh (under rtl.lock).

## PanProbe autorun `all` (11:55-11:58 IST): 40/40 PASS
All 40 tests PASS, incl. tessellation (mismatch=0, 3520.9 fps), tess_perf, submit_stress, csf_event, swapchain_lifecycle, sparse_kbase (pass=12 fail=0 skip=2).
Compliance: DXVK PASS (94), Bachata S4 PASS (114), vkd3d PASS (100), D3D FL PASS (20); only soft misses (FL12_x as before).
Logcat grep (kbase/mali/panvk/csf/fault/DEVICE_LOST/timeout snapshot): 0 faults, 0 timeout snapshots, 0 DEVICE_LOST.
Raw: raw/probe-all/ (autorun.txt, logcat-grep.txt).


## PanPlay (install 12:03-12:05, one `adb install -r`, 415 MB): all 9 scenarios PASS, DEVICE_LOST: NO
Short window via `--ei warmup_sec 10 --ei measure_sec 30` (applied; the old gameloop_config json was not). Order 3 8 5 9 1 2 4 6 7, 12:07-12:37 IST.
Logcat grep per scenario: 0 DEVICE_LOST, 0 `kbase: timeout snapshot`, 0 CSF faults.
| Scn | Test | Result | drv fps (116 vs 0148) |
|---|---|---|---|
| 3 | D3D11 / D3D9 cube | PASS / PASS | 967.2 / 120.0 (756.8 / 120.0) |
| 8 | D3D11 FL 10_0 10_1 11_0 11_1 | PASS x4 | 1187.2 / 1203.3 / 1186.4 / 1188.7 (1181.9 / 1225.2 / 1239.9 / 1220.6) |
| 8 | D3D10 / D3D9 cube | PASS / PASS | 768.7 / 120.0 (775.6 / 120.0) |
| 5 | D3D12 SquidRoom | PASS | 288.6 (282.7) |
| 9 | D3D11 FL all | PASS | 1205.9 (1247.1) |
| 9 | D3D12 FL all | harness-timeout on short window ("still running 93 s", done ok=false); rerun with default 20/60 s: PASS, highest 11_1, 12_x UNSUPPORTED as expected | 369.3 (376.3) |
| 1 | D3D11 cube | PASS | 846.5 (808.1) |
| 2 | D3D9 cube | PASS | 120.0 (120.0) |
| 4 | A/B d3d11 old / default | PASS / PASS | 260.2 / 775.7 (261.2 / 784.9) |
| 4 | A/B d3d9 old / default | PASS / PASS | 120.0 / 120.0 |
| 6 | D3D12 gears | PASS | 304.6 (317.9) |
| 7 | D3D12 FL 11_0 / 11_1 | PASS / PASS | 377.5 / 358.6 (356.0 / 368.7) |
| 7 | D3D12 FL 12_0 / 12_1 / 12_2 | UNSUPPORTED (pass) x3 | 376.2 / 373.4 / 365.6 |
The s9 D3D12 all-levels pass needs ~88 s, longer than the short window; that is a harness-window artifact, not a driver fault (rerun in `raw/s9-rerun-default`, 12:40-12:45). fps matches 0148 within noise. No regression.

## 0147 A/B: `PANVK_PRERAST_CHUNK_FLUSH=0` PanProbe tessellation x3 (12:38-12:40 IST)
PASS mismatch=0 x3 (4181 / 4752 / 4785 fps); default env on 116: PASS mismatch=0 (3521 fps, plus 3 of 3 on 0148). Flush=0 does not change the result, so patch 0147 is NOT needed (v14 tessellation mismatch was the sparse TLS bug, fixed by 0148). Env passed via `--es env "'PANVK_PRERAST_CHUNK_FLUSH=0'"`. Raw: raw/tess0147-N/.

## Device
Stayed online through the last step (12:45 IST), no drop seen. No adb connect/disconnect, no sysfs, lock released, no background processes.
