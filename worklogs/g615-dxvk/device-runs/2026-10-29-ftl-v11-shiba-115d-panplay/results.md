# FTL shiba (Pixel 8, Mali-G715, v11 CSF) build 115d: PanPlay scenarios 1-9

- Matrix `matrix-26b3h1v6yc9s1`, project `finalbenchmark-665a8`, results
  `gs://test-lab-rh5tw441b11v6-yb3zb70vx85mm/panvk-115d-shiba-play/`; FTL outcome `Passed shiba-34-en-portrait`, test time 1521 s.
- APK `panplay-115d.apk` (415 MB, full bundle with imagefs_bionic rootfs), `dev.zenithblue.panvklauncher` 1.3.0 (vc 44);
  driver `Mesa 26.3.0-devel (git-b04b8ee36d)`, sha256 `da6f9a0f8f6a3d97...`. `gameloop-override.json`: warm-up 10 s, measure 30 s.
  `raw/` has everything but `video.mp4`.
- This is the first run with a working rootfs since 115b (115c APKs had none).

## Result: 22 of 23 passes ok, no DEVICE_LOST

| scn | pass | status | fps | test line |
|---|---|---|---|---|
| 1 | d3d11 | ok | 416.5 | `CUBE: PASS api=d3d11` |
| 2 | d3d9 | ok | 118.9 | `CUBE: PASS api=d3d9` |
| 3 | d3d11 / d3d9 | ok / ok | 417.8 / 118.3 | `CUBE: PASS` |
| 4 | d3d11 old / default | ok | 143.6 / 411.9 | `CUBE: PASS` |
| 4 | d3d9 old / default | ok | 119.3 / 118.3 | `CUBE: PASS` |
| 5 | D3D12 SquidRoom | ok | 74.9 | `DX12: PASS test=d3d12-multithreading` |
| 6 | D3D12 gears | ok | 176.8 | `DX12: PASS test=vkd3d-gears` |
| 7 | D3D12 FL 11_0 | ok | 206.4 | PASS (tess, gs, compute, mrt, msaa4x) |
| 7 | D3D12 FL 11_1 | **fail** | 204.5 | `FAIL created uav_vs:PASS,logic_op:FAIL,tir:REPORTED` |
| 7 | D3D12 FL 12_0 | ok | 206.4 | UNSUPPORTED (`tiled_tier2`; typed_uav_load, binding_tier2 PASS) |
| 7 | D3D12 FL 12_1 | ok | 210.5 | UNSUPPORTED (`rov`), **`conservative:PASS`** |
| 7 | D3D12 FL 12_2 | ok | 216.7 | UNSUPPORTED (mesh, dxr1_1, vrs_tier2) |
| 8 | D3D11 FL 10_0 / 10_1 / 11_0 | ok | 399.7 / 388.6 / 387.4 | PASS |
| 8 | D3D11 FL 11_1 | ok | 382.4 | `PASS uav_vs:PASS,logic_op:PASS,tir:PASS` |
| 8 | D3D10 / D3D9 cube | ok | 417.8 / 118.3 | PASS |
| 9 | d3d12fl_all | ok | 211.2 | 11_0=PASS 11_1=PASS 12_x UNSUPPORTED (highest 11_1) |
| 9 | d3d11fl_all | ok | 407.7 | 10_0..11_1 all PASS |

Compared with 115b (`../2026-10-28-ftl-v11-shiba-115b-panplay/`): D3D11 cube 414.3 -> 417.8, D3D9 119.2 -> 118.3, SquidRoom
78.6 -> 74.9, gears 187.2 -> 176.8 (within run noise, gears -5.5%), FL 11_0 PASS, FL 12_0/12_2 UNSUPPORTED as before.

Key checks:

- D3D12 FL 11_1 `tir`: no DEVICE_LOST (115b: DEVICE_LOST). It reports `tir REPORTED`: `ForcedSampleCount=4: 0 PS invocations,
  TIR not applied (vkd3d-proton ignores ForcedSampleCount)`. The FL 11_1 verdict is FAIL only from the old `logic_op` problem:
  `logic_op FAIL INVERT after writing aaf00fff: ffffffff (want 550ff000)` (same as 115b; scenario 9 `d3d12fl_all` shows 11_1=PASS).
- D3D12 FL 12_1 `conservative`: PASS, no DEVICE_LOST (115b: DEVICE_LOST).
- D3D11 FL 11_1 `tir` PASS (DESC2 test), logic_op PASS.
- No `DEVICE_LOST`, `timeout on subqueue`, tiler heap OOM, GPU fault in logcat or any session log.
- SquidRoom exit crash present: `wine: Unhandled page fault on read access to 0000000000000070 at address 0000006FFBDE29F0
  (thread 0140)` after `DX12: PASS` (scenario 5, `exit=0`, harness ok).

## Screenshots

- `screenshots/video-*.png`: FTL video frames; `screenshots/<run>_<pass>.png`: the game screenshot of each pass.
