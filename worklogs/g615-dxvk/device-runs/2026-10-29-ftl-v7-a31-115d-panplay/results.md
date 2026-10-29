# FTL a31 (Galaxy A31, Mali-G52 MC2, v7 JM) build 115d: PanPlay scenarios 1-9

- Matrix `matrix-147r4ehprzvt6`, project `finalbenchmark-665a8`, results
  `gs://test-lab-rh5tw441b11v6-yb3zb70vx85mm/panvk-115d-a31-play/`; FTL outcome `Passed a31-31-en-portrait`.
- APK `panplay-115d.apk` (full bundle), driver `Mesa 26.3.0-devel (git-b04b8ee36d)`, sha256 `da6f9a0f...`.
  `gameloop-override.json`: warm-up 10 s, measure 30 s. `raw/` has everything but `video.mp4`.

## Result: 22 of 23 passes ok, no DEVICE_LOST

| scn | pass | status | fps | test line |
|---|---|---|---|---|
| 1 | d3d11 | ok | 68.1 | `CUBE: PASS` |
| 2 | d3d9 | ok | 36.3 | `CUBE: PASS` |
| 3 | d3d11 / d3d9 | ok | 67.3 / 36.4 | `CUBE: PASS` |
| 4 | d3d11 old / default | ok | 54.5 / 67.7 | `CUBE: PASS` |
| 4 | d3d9 old / default | ok | 36.1 / 36.4 | `CUBE: PASS` |
| 5 | D3D12 SquidRoom | **fail** | 1.3 | test `DX12: PASS` (63 frames in 48 s) but `still running 93 s after the first frame` (harness killed it) |
| 6 | D3D12 gears | ok | 110.0 | `DX12: PASS test=vkd3d-gears`; screenshot: red, green, blue gear, **no red gear twice** |
| 7 | D3D12 FL 11_0 | ok | 191.8 | PASS |
| 7 | D3D12 FL 11_1 | ok | 190.7 | UNSUPPORTED: `D3D12CreateDevice(11_1) hr=0x80070057`, highest 11_0; uav_vs PASS, **logic_op PASS**, tir REPORTED |
| 7 | D3D12 FL 12_0 | ok | 191.7 | UNSUPPORTED (not created; typed_uav_load PASS, `tiled_tier2` unsupported); **no DEVICE_LOST** (115b: lost on typed UAV 0x51) |
| 7 | D3D12 FL 12_1 / 12_2 | ok | 193.1 / 192.6 | UNSUPPORTED |
| 8 | D3D11 FL 10_0 / 10_1 / 11_0 | ok | 96.9 / 98.7 / 99.4 | PASS |
| 8 | D3D11 FL 11_1 | ok | 98.0 | `PASS uav_vs:PASS,logic_op:PASS,tir:PASS` |
| 8 | D3D10 / D3D9 cube | ok | 66.7 / 36.3 | PASS |
| 9 | d3d12fl_all | ok | 192.5 | 11_0=PASS, 11_1..12_2 UNSUPPORTED, highest 11_0 |
| 9 | d3d11fl_all | ok | 97.5 | 10_0..11_1 all PASS |

Compared with 115b (`../2026-10-28-ftl-v7-a31-115b-panplay/`): D3D11 cube 65.8 -> 68.1, D3D9 35.7 -> 36.3, gears 109 -> 110.0.

Key checks:

- gears correct (115c 0137 fix holds): no red gear twice.
- D3D12 FL 11_1 `logic_op` PASS (115b: FAIL). FL 11_1 itself still not exposed by vkd3d (highest 11_0), D3D12 FL 12_0 typed UAV: no DEVICE_LOST (115b 0x51).
- D3D11 FL 11_1 `tir` PASS.
- No `DEVICE_LOST` in logcat or session logs.
- SquidRoom: very slow (1.3 fps) and the loop timed out with the game still running; also the exit page fault
  `wine: Unhandled page fault on read access to 0000000000000070 at address 0000006FFBDE29F0 (thread 013c)`.

## Screenshots

- `screenshots/video-*.png`: FTL video frames; `screenshots/<run>_<pass>.png`: the game screenshot of each pass.
