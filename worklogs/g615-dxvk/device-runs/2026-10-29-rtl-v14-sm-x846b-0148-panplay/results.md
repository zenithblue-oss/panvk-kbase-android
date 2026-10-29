# RTL SM-X846B (Mali-G1-Ultra, v14) - PanPlay on driver 0148 (2026-10-29)

Device Tab S12+ SM-X846B, SDK 37, MT6993. APK panplay-d3d9v14 (launcher 1.3.0 (44)), driver Mesa 26.3.0-devel (git-41aa2dafce) = 0148 (no sparse device TLS on v14).
Device had been reprovisioned (no PanPlay/PanProbe installed), so one `adb install -r` of the 415 MB APK. Defaults used (20 s warm-up / 60 s measure; the override json was not applied).
Run 05:26Z-06:11Z, scenarios 3 8 9 5 1 2 4 6 7 back to back under rtl.lock. Script: /var/tmp/panvk/run-0148-rtl.sh.

## Result: all 9 scenarios PASS (done ok=true), DEVICE_LOST: NO

grep of per-scenario logcat (kbase/mali/panvk/csf/fault/DEVICE_LOST/timeout snapshot/vkWaitForPresent/SubmissionQueue): 0 DEVICE_LOST, 0 `kbase: timeout snapshot`, 0 CSF faults.

| Scn | Test | Result | drv fps |
|---|---|---|---|
| 3 | D3D11 cube | PASS | 756.8 |
| 3 | D3D9 cube (was DEVICE_LOST on 115e/115g) | **PASS** | 120.0 (vsync, 1s-min 119.5) |
| 8 | D3D11 FL 10_0 / 10_1 / 11_0 / 11_1 | PASS x4 | 1181.9 / 1225.2 / 1239.9 / 1220.6 |
| 8 | D3D10 cube / D3D9 cube | PASS / PASS | 775.6 / 120.0 |
| 9 | D3D12 FL all | PASS (11_0, 11_1 PASS; 12_x UNSUPPORTED as expected) | 376.3 |
| 9 | D3D11 FL all | PASS | 1247.1 |
| 5 | D3D12 SquidRoom | PASS | 282.7 |
| 1 | D3D11 cube | PASS | 808.1 |
| 2 | D3D9 cube | PASS | 120.0 |
| 4 | A/B d3d11 old / default | PASS / PASS | 261.2 / 784.9 |
| 4 | A/B d3d9 old / default | PASS / PASS | 120.0 / 120.0 |
| 6 | D3D12 gears | PASS | 317.9 |
| 7 | D3D12 FL 11_0 / 11_1 | PASS / PASS | 356.0 / 368.7 |
| 7 | D3D12 FL 12_0 / 12_1 / 12_2 | UNSUPPORTED (pass) x3 | 368.7 / 367.3 / 376.2 |

Per-pass tables with 1s-min/max, p50/p95/p99: `raw/sN/results.txt`. 0148 fixes the D3D9 cube hang; 0145 sparse TLS on v14 was the cause.
Not committed: video, screenshots.
