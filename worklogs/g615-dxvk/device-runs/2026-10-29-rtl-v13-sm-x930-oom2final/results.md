# RTL SM-X930 (Immortalis-G925 MC12, v13, gpu_id 0xd8300015, mask 0xee0077) - oom2final (build 120 + jm-v7-wip 0161), 2026-10-29

Complete: PanProbe 41/41 and PanPlay scenarios 1-9 all PASS. Device stayed connected 17:03-17:34 IST (the 16:53 drop before this run is unrelated).
driverInfo reported by the device: `Mesa 26.3.0-devel (git-84704e1bb7, p0161, 2026-10-29)`.
.so on disk: sha256 d0d4a971f470d6d2863975125d54cd03d48214ba72a9f695fe35e34e046c454c, BuildID 0cf6b04d5cfdacb4cd4943b5d11835998176eb4b (unstripped build-out copy; the APK .so is stripped).
Script `raw/run.sh`, progress `raw/progress.txt`. Device on adb `localhost:35395` via rdb, rtl.lock held.

## PanProbe autorun `all`: 41/41 PASS, 0 fail, 0 skip (build 118: 39/40, tess_perf FAIL)
- **tess_perf PASS** (`TESS_PERF_FAILS=0`, `RESULT tess_perf PASS mismatch=0`), both variants (default and TESS_FUSE=0/SETUP_STEP=0/ARENAS=1/WINDING_INLINE=0) ran all 10 gs x L points to completion.
  Hashes identical to 118's good points (L=1 a03404c5, L=4 cec55885, L=16 d4463b35, mix b34a251c). Decisive: no `Wait r=-4`, no `tiler heap OOM notification`, no `timeout snapshot` anywhere in the logcat stream (0 DEVICE_LOST); 118 had the OOM line in every failure.
- Fps/timing (default, gs0/gs1, ms per 256 draws): L=1 22.1/37.2, L=4 40.5/61.2, L=16 150.3/376.2, mix 148.1/208.7. 118 never finished gs=1 L=4, so no like-for-like past that.
- DEVICE_LOST: none. CSF faults: none (no OOM/incremental/fault/timeout line from MESA; `raw/probe-logcat-grep.txt` holds only benign `queue_group_create` lines).
- Memory (PanProbe `dumpsys meminfo` TOTAL PSS sampled every 15 s over the run): 127.6 - 139.5 MB.
- Full stream (local only, 100+ MB): /var/tmp/panvk/rtl-oom2final/logcat-stream.txt.
- Screenshot: [sm-x930-oom2final-panprobe-results.png](sm-x930-oom2final-panprobe-results.png)

## PanPlay (install -r 415 MB-class APK; short window warmup 10 s / measure 30 s; order 3 8 5 9 1 2 4 6 7, 17:09-17:34 IST)
All 9 scenarios completed, every task ok/PASS (D3D12 FL 12_x UNSUPPORTED as expected). Logcat per scenario (`raw/s*/logcat-grep.txt`) and the full stream: 0 DEVICE_LOST, 0 timeout snapshot, 0 tiler OOM, 0 CSF faults.
PSS = `dumpsys meminfo dev.zenithblue.panvklauncher` TOTAL PSS at 30 s into the scenario (build 118 recorded it only for s3: 212 MB there).
| Scn | Test | Result | oom2final fps | 118 fps | delta | PSS |
|---|---|---|---|---|---|---|
| 3 | d3d11 default | PASS | 600.4 | 881.9 | -32% | 208 MB |
| 3 | d3d9 default | PASS | 120.0 | 120.0 | +0% | |
| 8 | d3d11fl_10_0 default | PASS | 1712.5 | 1434.1 | +19% | 205 MB |
| 8 | d3d11fl_10_1 default | PASS | 1729.9 | 1447.6 | +20% | |
| 8 | d3d11fl_11_0 default | PASS | 1744.5 | 1467.3 | +19% | |
| 8 | d3d11fl_11_1 default | PASS | 1733.9 | 1435.6 | +21% | |
| 8 | d3d10 default | PASS | 588.7 | 970.4 | -39% | |
| 8 | d3d9 default | PASS | 120.0 | 120.0 | +0% | |
| 5 | d3d12 default | PASS | 245.6 | 210.2 | +17% | 182 MB |
| 9 | d3d12fl_all default | PASS | 688.8 | 597.2 | +15% | 203 MB |
| 9 | d3d11fl_all default | PASS | 1702.5 | 1410.6 | +21% | |
| 1 | d3d11 default | PASS | 606.1 | 964.8 | -37% | 230 MB |
| 2 | d3d9 default | PASS | 120.0 | 120.0 | +0% | 240 MB |
| 4 | d3d11 old | PASS | 584.8 | 562.5 | +4% | 237 MB |
| 4 | d3d11 default | PASS | 591.7 | 974.3 | -39% | |
| 4 | d3d9 old | PASS | 120.0 | 120.0 | +0% | |
| 4 | d3d9 default | PASS | 120.0 | 120.0 | +0% | |
| 6 | d3d12gears default | PASS | 557.2 | 456.5 | +22% | 230 MB |
| 7 | d3d12fl_11_0 default | PASS | 765.2 | 598.4 | +28% | 210 MB |
| 7 | d3d12fl_11_1 default | PASS | 699.8 | 589.2 | +19% | |
| 7 | d3d12fl_12_0 default | PASS | 669.7 | 621.4 | +8% | |
| 7 | d3d12fl_12_1 default | PASS | 651.6 | 566.5 | +15% | |
| 7 | d3d12fl_12_2 default | PASS | 701.4 | 571.6 | +23% | |

Screenshots: sm-x930-oom2final-panplay-s1..s9.png, sm-x930-oom2final-panprobe-results.png.

## Comparison vs build 118
- tess_perf: FAIL (Wait r=-4, tiler heap OOM) -> PASS, full gs x L matrix, both variants. 0161 incremental rendering fixes the v13 hang.
- D3D12 fps up 8-28% (s5 +17%, s9 +15%, s6 +22%, s7 +8..+28%).
- D3D11/D3D10 fps changed in both directions: FL cubes (s8) +19-21%, s9 D3D11 FL-all +21%, but single-scene D3D11 cubes s1/s3/s4-default are down 32-39% (590-606 vs 880-975) and the D3D10 cube in s8 is -39% (note s4 "old" is +4%, so the old/default A/B gain of 118 is gone). Run-to-run/thermal variance on a shared RTL device is not excluded (118 vs this run were not back to back); worth a rerun of s1/s3 before reading it as a 0161 regression.
- D3D9 unchanged at the 120 fps vsync cap.
- PSS 186-246 MB across scenarios (118 s3 212 MB vs 208 MB here, no growth).
