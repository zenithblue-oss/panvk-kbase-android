# RTL SM-X930 (Immortalis-G925 MC12, v13) - release gate build 118 (2026-10-29)

Driver git-a0412457ab (build 118), panprobe-118.apk / panplay-118.apk. Device online 12:57 IST, all steps done 13:30 IST (limit 13:50).
Scripts: /var/tmp/panvk/run-118-probe.sh, run-118-rtl.sh (under rtl.lock). Raw in `raw/`.

## GPU id
Recognised: `gpu_id=0xd8300015 arch=v13`, CSF uAPI 1.30 (sparse_kbase setup line). No unknown-id warning.

## PanProbe autorun `all`: 39/40 PASS, 1 FAIL (tess_perf)
- **tess_perf FAIL**: GPU hang then `Wait r=-4` (VK_ERROR_DEVICE_LOST) at line 253, after variants `gs=1 L=4`. Logs `kbase: timeout snapshot subqueue 0/1/2` (seqno 792, target 793, jobs 793, error 0x0). Raw: `raw/probe-all/20261029-162826...` run dir, `tess_perf.log`. Variants before the hang had matching hashes (a03404c5 L=1 gs0 and gs1; cec55885 L=4 gs0 and gs1). New test (PanProbe 1.3.1), no earlier v13 baseline for it; earlier v13 runs were 37/37 (rc1) and 38/38 (rc3, G925 not rerun), without tess_perf.
- All other tests after tess_perf PASS (tessellation, geometry, submit_stress, csf_event, swapchain_lifecycle, vkd3d, dxvk, d3d_fl, etc.), so the device recovered. sparse_kbase pass=18 fail=0 skip=0.
- DEVICE_LOST: YES, in tess_perf only (the failing test itself).
- Screenshot: [sm-x930-118-panprobe-results.png](sm-x930-118-panprobe-results.png)

## PanPlay (one `adb install -r`, 415 MB): all 9 scenarios PASS, no DEVICE_LOST
Short window `--ei warmup_sec 10 --ei measure_sec 30`; order 3 8 5 9 1 2 4 6 7, 13:05-13:29 IST. s9 D3D12 all-levels completed (ok=true) in the short window this time. Logcat grep per scenario: 0 DEVICE_LOST, 0 timeout snapshot, 0 CSF faults.
| Scn | Test | Result | drv fps | Screenshot |
|---|---|---|---|---|
| 3 | D3D11 / D3D9 cube | PASS / PASS | 881.9 / 120.0 | [s3](sm-x930-118-panplay-s3.png) |
| 8 | D3D11 FL 10_0 10_1 11_0 11_1 | PASS x4 | 1434.1 / 1447.6 / 1467.3 / 1435.6 | [s8](sm-x930-118-panplay-s8.png) |
| 8 | D3D10 / D3D9 cube | PASS / PASS | 970.4 / 120.0 | |
| 5 | D3D12 multithreading | PASS | 210.2 | [s5](sm-x930-118-panplay-s5.png) |
| 9 | D3D12 FL all | PASS (11_0, 11_1; 12_x UNSUPPORTED as expected) | 597.2 | [s9](sm-x930-118-panplay-s9.png) |
| 9 | D3D11 FL all | PASS (10_0..11_1) | 1410.6 | |
| 1 | D3D11 cube | PASS | 964.8 | [s1](sm-x930-118-panplay-s1.png) |
| 2 | D3D9 cube | PASS | 120.0 | [s2](sm-x930-118-panplay-s2.png) |
| 4 | A/B d3d11 old / default | PASS / PASS | 562.5 / 974.3 (+73%) | [s4](sm-x930-118-panplay-s4.png) |
| 4 | A/B d3d9 old / default | PASS / PASS | 120.0 / 120.0 | |
| 6 | D3D12 gears | PASS | 456.5 | [s6](sm-x930-118-panplay-s6.png) |
| 7 | D3D12 FL 11_0 / 11_1 | PASS / PASS | 598.4 / 589.2 | [s7](sm-x930-118-panplay-s7.png) |
| 7 | D3D12 FL 12_0 / 12_1 / 12_2 | UNSUPPORTED (pass) x3 | 621.4 / 566.5 / 571.6 | |

## Memory
`dumpsys meminfo dev.zenithblue.panvklauncher` during s3 (`raw/meminfo-s3.txt`): TOTAL PSS 212,607 KB (Java heap 66 MB, native heap 13 MB).

## Comparison
No G925 PanPlay baseline in worklogs (TEST_DEVICES.md lists SM-X930 only as a planned RTL target; rc1 PanProbe 37/37, rc3 38/38 for v13 in PROGRESS/memory). Versus v14 SM-X846B build 116: G925 is faster on D3D11 FL cubes (1435 vs 1190 fps) and D3D12 FL (590 vs 370 fps), similar on D3D9 (vsync 120). Regression to chase: tess_perf hang on v13 (tessellation + GS variants with L=4 patches=4096).

## Device
Stayed online through the end (13:30 IST). No adb connect/disconnect, no sysfs, lock released, no background processes.
