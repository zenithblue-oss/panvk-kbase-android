# FTL panther (Pixel 7, Mali-G710, v10 CSF) build 115d: PanPlay scenarios 1-9

- Matrix `matrix-23xxw3c5a1ilv`, project `finalbenchmark-665a8`, results
  `gs://test-lab-rh5tw441b11v6-yb3zb70vx85mm/panvk-115d-panther-play/`; FTL outcome `Passed panther-33-en-portrait`, test time 1415 s.
- APK `panplay-115d.apk` (full bundle with rootfs), driver `Mesa 26.3.0-devel (git-b04b8ee36d)`, sha256 `da6f9a0f...`.
  `gameloop-override.json`: warm-up 10 s, measure 30 s. `raw/` has everything but `video.mp4`.

## Result: 21 of 23 passes ok, no DEVICE_LOST

| scn | pass | status | fps | test line |
|---|---|---|---|---|
| 1 | d3d11 | ok | 575.3 | `CUBE: PASS` |
| 2 | d3d9 | ok | 90.0 | `CUBE: PASS` |
| 3 | d3d11 / d3d9 | ok | 580.1 / 90.0 | `CUBE: PASS` |
| 4 | d3d11 old / default | ok | 112.6 / 569.2 | `CUBE: PASS` |
| 4 | d3d9 old / default | ok | 89.8 / 90.0 | `CUBE: PASS` |
| 5 | D3D12 SquidRoom | ok | 86.0 | `DX12: PASS test=d3d12-multithreading` |
| 6 | D3D12 gears | ok | 407.1 | `DX12: PASS test=vkd3d-gears` |
| 7 | D3D12 FL 11_0 | ok | 452.4 | PASS |
| 7 | D3D12 FL 11_1 | **fail** | 482.2 | `FAIL created uav_vs:PASS,logic_op:FAIL,tir:REPORTED` |
| 7 | D3D12 FL 12_0 | ok | 477.8 | UNSUPPORTED (`tiled_tier2`) |
| 7 | D3D12 FL 12_1 | ok | 487.4 | UNSUPPORTED (`rov`, `conservative` UNSUPPORTED on v10) |
| 7 | D3D12 FL 12_2 | ok | 479.4 | UNSUPPORTED |
| 8 | D3D11 FL 10_0 / 10_1 / 11_0 | ok | 914.6 / 794.1 / 788.1 | PASS |
| 8 | D3D11 FL 11_1 | ok | 787.5 | `PASS uav_vs:PASS,logic_op:PASS,tir:PASS` |
| 8 | D3D10 / D3D9 cube | ok | 517.4 / 90.0 | PASS |
| 9 | d3d12fl_all | **fail** | 485.4 | 11_0=PASS 11_1=FAIL, 12_x UNSUPPORTED |
| 9 | d3d11fl_all | ok | 932.8 | 10_0..11_1 all PASS |

Compared with 115b (`../2026-10-28-ftl-v10-panther-115b-panplay/`): D3D11 cube 572.1 -> 580.1, D3D9 90.0 -> 90.0, SquidRoom
85.0 -> 86.0, gears 431.1 -> 407.1 (-5.6%, same drop as shiba). 115b D3D12 FL 11_1 was DEVICE_LOST on `tir`.

Key checks:

- D3D12 FL 11_1 `tir`: no DEVICE_LOST (`tir REPORTED`, vkd3d-proton ignores ForcedSampleCount). The FAIL verdict is only
  `logic_op FAIL INVERT after writing aaf00fff: ffffffff (want 550ff000)` (D3D12 logic op, open since 115b).
- D3D11 FL 11_1 `tir` PASS (DESC2).
- No `DEVICE_LOST`, `timeout on subqueue`, tiler heap OOM in logcat or session logs.
- SquidRoom exit crash present: `wine: Unhandled page fault on read access to 0000000000000070 at address 0000006FFBDE29F0
  (thread 0140)` after `DX12: PASS` (scenario 5, harness ok).

## Screenshots

- `screenshots/video-*.png`: FTL video frames; `screenshots/<run>_<pass>.png`: the game screenshot of each pass.
