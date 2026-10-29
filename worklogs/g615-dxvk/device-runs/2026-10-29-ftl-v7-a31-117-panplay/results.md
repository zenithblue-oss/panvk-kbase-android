# FTL a31 (Galaxy A31 SM-A315N, Mali-G52 MC2, v7 JM, API 31) build 117: PanPlay scenarios 1-9

- Matrix `matrix-23vfpwtxzd2ff`, project `abhay-byte` (panvk-device-test and finalbenchmark-665a8 returned TEST_QUOTA_EXCEEDED
  at validation; logs not kept in the repo). FTL outcome `Passed a31-31-en-portrait`, ~32 min.
- Build 117 = Mesa 5a07217f + `prepare-rc5-tree.sh` (strict) with csf-v11 <= 233 and jm-v7-wip 0006..0148, driver
  `Mesa 26.3.0-devel (git-1a0a81aa00)`, .so sha256 `758ae1e9860f...` (loaded .so in results.json matches).
  APK `panplay-117.apk` (PanPlay 1.3.0 vc 44 with memfd_create shim 974fa3dd and CNTFRQ fix ac1f84d6, all components + SquidRoom bundled).
- `gameloop-override.json`: warm-up 10 s, measure 30 s. `raw/` has everything but `video.mp4`.

## Result: 23 of 23 passes ok, no DEVICE_LOST

Fault greps (DEVICE_LOST, timeout on subqueue, tiler heap OOM, GPU fault, CS_FATAL, Fatal signal, page fault): none.

| scn | pass | status | fps (115d) | test line |
|---|---|---|---|---|
| 1 | d3d11 | ok | 68.1 (68.1) | `CUBE: PASS` |
| 2 | d3d9 | ok | 36.2 (36.3) | `CUBE: PASS` |
| 3 | d3d11 / d3d9 | ok | 68.8 / 36.1 (67.3 / 36.4) | PASS |
| 4 | d3d11 old / default | ok | 54.2 / 69.0 (54.5 / 67.7) | PASS |
| 4 | d3d9 old / default | ok | 35.8 / 36.1 (36.1 / 36.4) | PASS |
| 5 | D3D12 SquidRoom | ok | 1.3 (1.3, harness fail) | `DX12: PASS test=d3d12-multithreading` |
| 6 | D3D12 gears | ok | 111.3 (110.0) | `DX12: PASS test=vkd3d-gears` |
| 7 | D3D12 FL 11_0 | ok | 187.4 | PASS |
| 7 | D3D12 FL 11_1 | ok | 189.0 | **PASS (created)** `uav_vs:PASS,logic_op:PASS,tir:REPORTED` (115d: `D3D12CreateDevice(11_1) hr=0x80070057`) |
| 7 | D3D12 FL 12_0 / 12_1 / 12_2 | ok | 189.3 / 190.4 / 188.5 | UNSUPPORTED (`tiled_tier2`, rov/conservative, mesh/dxr/vrs) |
| 8 | D3D11 FL 10_0 / 10_1 / 11_0 / 11_1 | ok | 102.1 / 100.9 / 102.9 / 101.8 | PASS (11_1: uav_vs, logic_op, tir PASS) |
| 8 | D3D10 / D3D9 cube | ok | 68.5 / 36.2 | PASS |
| 9 | d3d12fl_all | ok | 191.8 | `11_0=PASS 11_1=PASS 12_0..12_2=UNSUPPORTED highest=11_1` |
| 9 | d3d11fl_all | ok | 100.0 | `10_0=PASS 10_1=PASS 11_0=PASS 11_1=PASS` |

## Highest feature level

- D3D12: **11_1** (was 11_0 in 115d, `../2026-10-29-ftl-v7-a31-115d-panplay/`).
- D3D11: **11_1** (was 11_1 in 115d).

## Compared with 115d

22/23 -> 23/23. SquidRoom now passes the harness (115d: `still running 93 s after the first frame`). fps within noise.
Screenshots are in `screenshots/` (per-pass game frames and FTL video frames).
