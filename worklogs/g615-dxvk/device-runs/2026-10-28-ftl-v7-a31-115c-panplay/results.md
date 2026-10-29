# FTL a31 (Galaxy A31, Mali-G52 r1 MC2, v7 JM) build 115c: PanPlay confirm (scenarios 1-9)

- Matrix `matrix-toe6cbnsyvqla` (project `finalbenchmark-665a8`), results `gs://test-lab-rh5tw441b11v6-yb3zb70vx85mm/panvk-v7-a31-115c-panplay/`; outcome `Passed a31-31-en-portrait --`.
- APK `/home/flux/build-out/apks/panplay-115c.apk` (sha256 `a16f9096...`), `dev.zenithblue.panvklauncher` 1.3.0 (vc 44), bundled driver build 115c (`eb44336a...`, driverInfo `Mesa 26.3.0-devel (git-15fd70c7c4)`).
- Scenarios 1-9 (1 D3D11 cube, 2 D3D9 cube, 3 both, 4 cube A/B `old` vs `default` knobs, 5 SquidRoom, 6 gears, 7 D3D12 FL 11_0-12_2, 8 D3D11 FL 10_0-11_1 + D3D10 + D3D9 cubes, 9 FL all-levels quick runs). `gameloop-override.json`: warm-up 10 s, measure 30 s (as 115b). `raw/` has everything but `video.mp4`.

## Result: 23 passes, no GPU fault (no `JM atom` failure, no device lost)

Against 115b (`../2026-10-28-ftl-v7-a31-115b-panplay/`, scenarios 3, 6, 7, 8):

- D3D12 FL 12_0 `typed_uav_load` **PASS** (`8090c0a0`, 0/64 wrong); 115b lost the device there (`event=0x51`, INSTR_INVALID_ENC): jm-v7-wip 0139.
- Gears draws all three gears (`screenshots/01-d3d12gears-default-harness.png`); 115b drew the red gear twice: 0137. 105.0 fps (115b 109).
- D3D12 FL 11_1 `logic_op` **PASS** (`550ff000`); 115b `ffffffff`: 0138. The device is still created at 11_0 only (v7: 32 storage images per stage), so the pass reports UNSUPPORTED for 11_1.
- Cubes: D3D11 64.5-64.9 fps (115b 65.8), D3D9 35.7-35.8 (35.7). SquidRoom 1.3 fps (115: 0.9; GPU-bound on Mali-G52 MC2). Scenario 4 `old` knobs: D3D11 54.2 fps vs 63.9 `default`.
- D3D11 FL 11_1 `tir` FAIL: DXVK 3.1.1 hands the UAV-only draw to the driver single-sampled (`../2026-10-28-ftl-v11-shiba-115c-diag3-panplay/`), not a driver bug.

The FTL runs of later builds are `../2026-10-29-ftl-*-115d-panplay/` and newer; D3D12 FL 11_1 `logic_op` on v10/v11 is fixed by jm-v7-wip 0144 (CSF re-emits the DCD flags when the logic op changes).

## Passes

| scenario | pass | result | fps | test line |
|---|---|---|---|---|
| 1 | 01-d3d11-default | ok | 64.9 | `CUBE: PASS api=d3d11 frames=3108 (check pixels)` |
| 2 | 01-d3d9-default | ok | 35.8 | `CUBE: PASS api=d3d9 frames=1611 (check pixels)` |
| 3 | 01-d3d11-default | ok | 64.5 | `CUBE: PASS api=d3d11 frames=3103 (check pixels)` |
| 3 | 02-d3d9-default | ok | 35.8 | `CUBE: PASS api=d3d9 frames=1709 (check pixels)` |
| 4 | 01-d3d11-old | ok | 54.2 | `CUBE: PASS api=d3d11 frames=2574 (check pixels)` |
| 4 | 02-d3d11-default | ok | 63.9 | `CUBE: PASS api=d3d11 frames=3069 (check pixels)` |
| 4 | 03-d3d9-old | ok | 35.4 | `CUBE: PASS api=d3d9 frames=1696 (check pixels)` |
| 4 | 04-d3d9-default | ok | 35.7 | `CUBE: PASS api=d3d9 frames=1709 (check pixels)` |
| 5 | 01-d3d12-default | ok | 1.3 | `DX12: PASS test=d3d12-multithreading frames=62 secs=48.2 fps=1.3 startup=6.5` |
| 6 | 01-d3d12gears-default | ok | 105.0 | `DX12: PASS test=vkd3d-gears frames=5043 secs=48.0 fps=105.0 startup=2.3` |
| 7 | 01-d3d12fl_11_0-default | ok | 180.4 | `D3DFL: PASS api=d3d12 level=11_0 created tess:PASS,gs:PASS,compute:PASS,mrt:PASS,msaa4x:PASS frames=8661 secs=48.0 fps=180.4 startup=3.6` |
| 7 | 02-d3d12fl_11_1-default | ok | 178.1 | `D3DFL: UNSUPPORTED api=d3d12 level=11_1 not_created uav_vs:PASS,logic_op:PASS,tir:REPORTED frames=8550 secs=48.0 fps=178.1 startup=21.9` |
| 7 | 03-d3d12fl_12_0-default | ok | 177.4 | `D3DFL: UNSUPPORTED api=d3d12 level=12_0 not_created typed_uav_load:PASS,binding_tier2:PASS,tiled_tier2:UNSUPPORTED frames=8517 secs=48.0 fps=177.4 startup=3.5` |
| 7 | 04-d3d12fl_12_1-default | ok | 180.4 | `D3DFL: UNSUPPORTED api=d3d12 level=12_1 not_created rov:UNSUPPORTED,conservative:UNSUPPORTED frames=8660 secs=48.0 fps=180.4 startup=3.2` |
| 7 | 05-d3d12fl_12_2-default | ok | 181.2 | `D3DFL: UNSUPPORTED api=d3d12 level=12_2 not_created sm6_5:REPORTED,mesh_shader:UNSUPPORTED,dxr1_1:UNSUPPORTED,vrs_tier2:UNSUPPORTED,sampler_feedback:UNSUPPORTED frames=8701 secs=48.0 fps=181.2 startup=3.4` |
| 8 | 01-d3d11fl_10_0-default | ok | 92.7 | `D3DFL: PASS api=d3d11 level=10_0 created gs:PASS,stream_out:PASS frames=4449 secs=48.0 fps=92.7 startup=2.4` |
| 8 | 02-d3d11fl_10_1-default | ok | 91.6 | `D3DFL: PASS api=d3d11 level=10_1 created cube_array:PASS,gather4:PASS frames=4400 secs=48.0 fps=91.6 startup=2.2` |
| 8 | 03-d3d11fl_11_0-default | ok | 89.3 | `D3DFL: PASS api=d3d11 level=11_0 created tess:PASS,compute:PASS frames=4288 secs=48.0 fps=89.3 startup=2.1` |
| 8 | 04-d3d11fl_11_1-default | fail (test failed: D3DFL: FAIL api=d3d11 level=11_1 created uav_vs:PASS,logic_op:PASS,tir:FAIL frames=4380 secs=48.0 fps=91.2 startup=2.2) | 90.9 | `D3DFL: FAIL api=d3d11 level=11_1 created uav_vs:PASS,logic_op:PASS,tir:FAIL frames=4380 secs=48.0 fps=91.2 startup=2.2` |
| 8 | 05-d3d10-default | ok | 63.7 | `CUBE: PASS api=d3d10 frames=3078 (check pixels)` |
| 8 | 06-d3d9-default | ok | 35.8 | `CUBE: PASS api=d3d9 frames=1711 (check pixels)` |
| 9 | 01-d3d12fl_all-default | ok | 182.1 | `D3DFL: DONE api=d3d12 level=all 11_0=PASS 11_1=UNSUPPORTED 12_0=UNSUPPORTED 12_1=UNSUPPORTED 12_2=UNSUPPORTED highest=11_0 frames=8744 secs=48.0 fps=182.1 startup=22.2` |
| 9 | 02-d3d11fl_all-default | fail (test failed: D3DFL: FAIL api=d3d11 level=all 10_0=PASS 10_1=PASS 11_0=PASS 11_1=FAIL frames=4460 secs=48.0 fps=92.9 startup=2.8) | 92.7 | `D3DFL: FAIL api=d3d11 level=all 10_0=PASS 10_1=PASS 11_0=PASS 11_1=FAIL frames=4460 secs=48.0 fps=92.9 startup=2.8` |

GPU fault lines (kbase CSF group / JM atom / device lost): 0

## Screenshots

- `screenshots/01-d3d11-default-harness.png`
- `screenshots/01-d3d11-old-harness.png`
- `screenshots/01-d3d11fl_10_0-default-harness.png`
- `screenshots/01-d3d12-default-harness.png`
- `screenshots/01-d3d12fl_11_0-default-harness.png`
- `screenshots/01-d3d12fl_all-default-harness.png`
- `screenshots/01-d3d12gears-default-harness.png`
- `screenshots/01-d3d9-default-harness.png`
- `screenshots/02-d3d11-default-harness.png`
- `screenshots/02-d3d11fl_10_1-default-harness.png`
- `screenshots/02-d3d11fl_all-default-harness.png`
- `screenshots/02-d3d12fl_11_1-default-harness.png`
- `screenshots/02-d3d9-default-harness.png`
- `screenshots/03-d3d11fl_11_0-default-harness.png`
- `screenshots/03-d3d12fl_12_0-default-harness.png`
- `screenshots/03-d3d9-old-harness.png`
- `screenshots/04-d3d11fl_11_1-default-harness.png`
- `screenshots/04-d3d12fl_12_1-default-harness.png`
- `screenshots/04-d3d9-default-harness.png`
- `screenshots/05-d3d10-default-harness.png`
- `screenshots/05-d3d12fl_12_2-default-harness.png`
- `screenshots/06-d3d9-default-harness.png`
- `screenshots/video-end.png`
- `screenshots/video-mid.png`
- `screenshots/video-start.png`
