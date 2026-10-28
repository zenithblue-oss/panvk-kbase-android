# FTL oriole (Pixel 6, Mali-G78, v9 JM) build 115b: PanPlay confirm

- Matrix `matrix-2jw3txkeo277h` (project `finalbenchmark-665a8`); outcome `Passed oriole-33-en-portrait`.
- APK `panplay-115b.apk` (sha256 `e5872f5b...`, vc 43), bundled driver build 115b (`d72e3059...`, driverInfo
  `Mesa 26.3.0-devel (git-ad5cd69141)`, every pass).
- Scenarios 3, 5, 6, 7, 8 as on panther (`../2026-10-28-ftl-v10-panther-115b-panplay/`). `gameloop-override.json`:
  warm-up 10 s, measure 30 s. `raw/` has everything but `video.mp4`.
- Baseline: build 115 (`../2026-10-26-ftl-v9-oriole-115-panplay/`): D3D11 168.1 fps, D3D9 90.0 fps (vsync), SquidRoom
  27.0 fps, gears 232.7 fps.

## Result: no regression, no GPU error in any pass

| scenario | pass | result |
|---|---|---|
| 3 | D3D11 cube | PASS, 8181 frames, 170.4 fps (1% low 113.2) |
| 3 | D3D9 cube | PASS, 4257 frames, 89.8 fps (vsync) |
| 5 | D3D12 SquidRoom | PASS, 27.2 fps |
| 6 | D3D12 gears | PASS, 15506 frames, 323.0 fps |
| 7 | D3D12 FL 11_0 | PASS (tess, gs, compute, mrt, msaa4x), 709.7 fps |
| 7 | D3D12 FL 11_1 | FAIL: `logic_op` INVERT gives `ffffffff` (want `550ff000`); `tir` reported (vkd3d-proton ignores ForcedSampleCount); no device lost |
| 7 | D3D12 FL 12_0 | UNSUPPORTED: not created (`typed_uav_load` and `binding_tier2` pass, `tiled_tier2` unsupported) |
| 7 | D3D12 FL 12_1, 12_2 | UNSUPPORTED |
| 8 | D3D11 FL 10_0, 10_1, 11_0 | PASS, 240-248 fps |
| 8 | D3D11 FL 11_1 | FAIL: `tir` (0 PS invocations with ForcedSampleCount=4); `uav_vs` and `logic_op` pass |
| 8 | D3D10 cube | PASS, 7721 frames |
| 8 | D3D9 cube | PASS, 4309 frames |

No failed JM atom, timeout or DEVICE_LOST in any pass. The D3D12 FL 11_1 `tir` draw without a render target that
loses the device on panther and shiba (CSF tiler heap OOM) runs here.

## Screenshots

- `screenshots/01-d3d11-default-harness.png`, `02-d3d9-default-harness.png`, `01-d3d12-default-harness.png`,
  `01-d3d12gears-default-harness.png`, the `0N-d3d12fl_*` and `0N-d3d11fl_*` shots, `05-d3d10-default-harness.png`,
  `06-d3d9-default-harness.png`.
- `screenshots/video-start.png`, `video-mid.png`, `video-end.png`: FTL video frames.
