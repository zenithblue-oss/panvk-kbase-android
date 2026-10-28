# FTL a31 (Galaxy A31, Mali-G52 r1 MC2, v7 JM) build 115b: PanPlay confirm

- Matrix `matrix-3onl6y8xaxo60` (project `panvk-device-test`); outcome `Passed a31-31-en-portrait`.
- APK `/home/flux/build-out/apks/panplay-115b.apk` (sha256 `e5872f5b...`), `dev.zenithblue.panvklauncher` 1.3.0 (vc 43),
  bundled driver build 115b (`d72e3059...`, driverInfo `Mesa 26.3.0-devel (git-ad5cd69141)`, every pass).
- Scenarios 3 (D3D11 + D3D9 cubes), 6 (D3D12 gears), 7 (D3D12 feature levels), 8 (D3D11 feature levels, D3D10 and
  D3D9 cubes). `gameloop-override.json`: warm-up 10 s, measure 30 s. `raw/` has everything but `video.mp4`.
- Before (115 and the 115b diag runs): D3D11 and D3D9 cubes DEVICE_LOST on the first frame (`event=0x4`), D3D9 placed
  map `VK_ERROR_MEMORY_MAP_FAILED` (`../2026-10-27-ftl-v7-a31-115b-diag6-panplay/`).

## Result: both cubes pass, no device lost in the cubes

| scenario | pass | result |
|---|---|---|
| 3 | D3D11 cube | **PASS**, 3157 frames, 65.8 fps (1% low 36.0) |
| 3 | D3D9 cube (i686, WoW64, placed map) | **PASS**, 1614 frames, 35.7 fps (1% low 20.9) |
| 6 | D3D12 gears | PASS, 5234 frames, 109 fps; still only the red gear, drawn twice (not fixed in 115b) |
| 7 | D3D12 FL 11_0 | PASS (tess, gs, compute, mrt, msaa4x) |
| 7 | D3D12 FL 11_1 | FAIL: `logic_op` INVERT gives `ffffffff` (want `550ff000`); `tir` reported (vkd3d-proton ignores ForcedSampleCount); device created at 11_0 only |
| 7 | D3D12 FL 12_0 | FAIL: device created at 11_0 only (`tiled_tier=0`: no sparse on kbase); the `typed_uav_load` check lost the device (`JM atom 3 failed: event=0x51`, INSTR_INVALID_ENC), then no frame for 300 s and the harness killed the pass |
| 7 | D3D12 FL 12_1 | UNSUPPORTED (no ROV, no conservative rasterization) |
| 7 | D3D12 FL 12_2 | UNSUPPORTED |
| 8 | D3D11 FL 10_0, 10_1, 11_0 | PASS |
| 8 | D3D11 FL 11_1 | FAIL: `tir` (0 PS invocations with ForcedSampleCount=4); `uav_vs` and `logic_op` pass |
| 8 | D3D10 cube | PASS, 3096 frames |
| 8 | D3D9 cube | PASS, 1553 frames |

- The two 115b JM fixes work: the cubes' vertex descriptor copy no longer writes to GPU address 0 (no failed atom in
  scenarios 3, 6 and 8), and the D3D9 vertex buffers come from gralloc: no `KBASE_IOCTL_MEM_IMPORT failed`, no
  `vkMapMemory2EXT` failure or `CUBE: FAIL VB` (the D3D9 result of 115 and diag4).
- New finding, not in 115b's scope: D3D12 FL 12_0's typed UAV load compute shader faults with `event=0x51`
  (invalid instruction encoding, a Bifrost compiler issue), which loses the device. 12_0/12_1 are not creatable on
  v7 anyway (tiled resources tier 0, no ROV or conservative rasterization).

## Screenshots

- `screenshots/01-d3d11-default-harness.png`, `02-d3d9-default-harness.png`: the cubes (four colours, HUD).
- `screenshots/01-d3d12gears-default-harness.png`: gears (red gear twice).
- `screenshots/0N-d3d12fl_*`, `0N-d3d11fl_*`, `05-d3d10-default-harness.png`, `06-d3d9-default-harness.png`: the
  feature level tests and the scenario 8 cubes.
- `screenshots/video-start.png`, `video-mid.png`, `video-end.png`: FTL video frames.
