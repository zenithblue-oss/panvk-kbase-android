# FTL shiba (Pixel 8, Mali-G715, v11 CSF) build 115b: PanPlay confirm

- Matrix `matrix-17jre5qstw8qy` (project `finalbenchmark-665a8`); outcome `Passed shiba-34-en-portrait`.
- APK `panplay-115b.apk` (sha256 `e5872f5b...`, vc 43), bundled driver build 115b (`d72e3059...`, driverInfo
  `Mesa 26.3.0-devel (git-ad5cd69141)`, every pass).
- Scenarios 3, 5, 6, 7, 8 as on panther (`../2026-10-28-ftl-v10-panther-115b-panplay/`). `gameloop-override.json`:
  warm-up 10 s, measure 30 s. `raw/` has everything but `video.mp4`.
- Baseline: build 115 (`../2026-10-26-ftl-v11-shiba-115-panplay/`): D3D11 395.0 fps, D3D9 117.9 fps, SquidRoom
  74.3 fps, gears 166.4 fps.

## Result

| scenario | pass | result |
|---|---|---|
| 3 | D3D11 cube | PASS, 19879 frames, 414.3 fps (1% low 72.4) |
| 3 | D3D9 cube | PASS, 5655 frames, 119.2 fps (vsync) |
| 5 | D3D12 SquidRoom | PASS, 78.6 fps |
| 6 | D3D12 gears | PASS, 8986 frames, 187.2 fps |
| 7 | D3D12 FL 11_0 | PASS (tess, gs, compute, mrt, msaa4x) |
| 7 | D3D12 FL 11_1 | **FAIL, DEVICE_LOST**: `logic_op` INVERT gives `ffffffff` (want `550ff000`), the `tir` check's submit lost the device |
| 7 | D3D12 FL 12_0 | UNSUPPORTED: not created (`typed_uav_load` and `binding_tier2` pass, `tiled_tier2` unsupported) |
| 7 | D3D12 FL 12_1 | **FAIL, DEVICE_LOST**: `rov` unsupported, the `conservative` check's submit lost the device |
| 7 | D3D12 FL 12_2 | UNSUPPORTED |
| 8 | D3D11 FL 10_0, 10_1, 11_0 | PASS, 384-399 fps |
| 8 | D3D11 FL 11_1 | FAIL: `tir` (0 PS invocations with ForcedSampleCount=4); `uav_vs` and `logic_op` pass |
| 8 | D3D10 cube | PASS, 19739 frames |
| 8 | D3D9 cube | PASS, 5660 frames |

No hang or GPU error in the cubes, SquidRoom, gears or the D3D11 passes. Both D3D12 device losts are the panther
signature: the check draws without a render target and kbase terminates the group on a tiler heap it cannot grow.

```
kbase: CSF group 3 tiler heap OOM notification
kbase: timeout on subqueue 0: seqno 8, ls_copy 0, target 10, ..., active 0, error 0x0     (11_1 tir)
kbase: timeout on subqueue 0: seqno 2, ls_copy 0, target 4, ..., active 0, error 0x0      (12_1 conservative)
```

Open (not the 115b wait cycle).

## Screenshots

- `screenshots/01-d3d11-default-harness.png`, `02-d3d9-default-harness.png`, `01-d3d12-default-harness.png`,
  `01-d3d12gears-default-harness.png`, the `0N-d3d12fl_*` and `0N-d3d11fl_*` shots, `05-d3d10-default-harness.png`,
  `06-d3d9-default-harness.png`.
- `screenshots/video-start.png`, `video-mid.png`, `video-end.png`: FTL video frames.
