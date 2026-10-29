# FTL panther (Pixel 7, Mali-G710 MC7, v10 CSF, API 33) build 116: PanPlay scenarios 1-9

- Matrix `matrix-1r24h0upbw4l6`, project `abhay-byte` (panvk-device-test and finalbenchmark-665a8 were out of quota);
  FTL outcome `Passed panther-33-en-portrait`, ~27 min. GCS dir is named `panvk-117-panther-play`; the APK is 116.
- APK `panplay-116.apk` (PanPlay 1.3.0 vc 44, memfd shim 974fa3dd and CNTFRQ fix ac1f84d6 present, all components bundled),
  driver `Mesa 26.3.0-devel (git-f7e78953ac)` (build 116, jm-v7-wip through 0151 incl. 0148), loaded .so sha256 `d5817e51...`.
  `gameloop-override.json`: warm-up 10 s, measure 30 s. `raw/` has everything but `video.mp4`.

## Result: 23 of 23 passes ok, no DEVICE_LOST

Fault greps (DEVICE_LOST, timeout on subqueue, tiler heap OOM, GPU fault, CS_FATAL, Fatal signal, page fault): none.

| scn | pass | status | fps (115d) | test line |
|---|---|---|---|---|
| 1 | d3d11 | ok | 561.4 (575.3) | `CUBE: PASS` |
| 2 | d3d9 | ok | 90.0 (90.0) | `CUBE: PASS` |
| 3 | d3d11 / d3d9 | ok | 572.0 / 90.0 (580.1 / 90.0) | PASS |
| 4 | d3d11 old / default | ok | 112.7 / 556.9 (112.6 / 569.2) | PASS |
| 4 | d3d9 old / default | ok | 89.7 / 90.0 (89.8 / 90.0) | PASS |
| 5 | D3D12 SquidRoom | ok | 85.6 (86.0) | `DX12: PASS test=d3d12-multithreading` |
| 6 | D3D12 gears | ok | 415.1 (407.1) | `DX12: PASS test=vkd3d-gears` |
| 7 | D3D12 FL 11_0 | ok | 478.1 (452.4) | PASS |
| 7 | D3D12 FL 11_1 | ok | 477.4 (482.2) | **PASS** `uav_vs:PASS,logic_op:PASS,tir:REPORTED` (115d: `logic_op FAIL`) |
| 7 | D3D12 FL 12_0 / 12_1 / 12_2 | ok | 479.8 / 480.0 / 475.6 | UNSUPPORTED (`tiled_tier2`, rov/conservative, mesh/dxr/vrs) |
| 8 | D3D11 FL 10_0 / 10_1 / 11_0 / 11_1 | ok | 956.4 / 848.6 / 867.7 / 831.7 (914.6 / 794.1 / 788.1 / 787.5) | PASS |
| 8 | D3D10 / D3D9 cube | ok | 542.1 / 90.0 (517.4 / 90.0) | PASS |
| 9 | d3d12fl_all | ok | 486.2 (485.4, fail) | `11_0=PASS 11_1=PASS 12_0..12_2=UNSUPPORTED highest=11_1` |
| 9 | d3d11fl_all | ok | 937.1 (932.8) | `10_0=PASS 10_1=PASS 11_0=PASS 11_1=PASS` |

## Highest feature level

- D3D12: **11_1** (115d: 11_1 FAIL on logic_op).
- D3D11: **11_1**.

## Compared with 115d (`../2026-10-29-ftl-v10-panther-115d-panplay/`)

21/23 -> 23/23. D3D12 logic_op (open since 115b) now passes; d3d12fl_all no longer fails. Cube and gears fps within +-3%, gears +2%.
