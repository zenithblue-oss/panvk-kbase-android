# FTL oriole (Pixel 6, Mali-G78 MC20, v9 JM) build 115c: PanPlay confirm (scenarios 1-9)

- Matrix `matrix-244u69nlbat5b` (project `finalbenchmark-665a8`), results `gs://test-lab-rh5tw441b11v6-yb3zb70vx85mm/panvk-v9-oriole-115c-panplay/`; outcome `Passed oriole-33-en-portrait` (Testing API).
- APK `/home/flux/build-out/apks/panplay-115c.apk` (sha256 `a16f9096...`), `dev.zenithblue.panvklauncher` 1.3.0 (vc 44), bundled driver build 115c (`eb44336a...`, driverInfo `Mesa 26.3.0-devel (git-15fd70c7c4)`).
- Scenarios 1-9 (1 D3D11 cube, 2 D3D9 cube, 3 both, 4 cube A/B `old` vs `default` knobs, 5 SquidRoom, 6 gears, 7 D3D12 FL 11_0-12_2, 8 D3D11 FL 10_0-11_1 + D3D10 + D3D9 cubes, 9 FL all-levels quick runs). `gameloop-override.json`: warm-up 10 s, measure 30 s (as 115b). `raw/` has everything but `video.mp4`.

## Result: 23 passes, no GPU fault; D3D12 highest feature level 11_1

Testing API: `FINISHED`, outcome `SUCCESS` (the local gcloud wait hung after the phone slept and was stopped; the results were pulled from the bucket).

Against 115b (`../2026-10-28-ftl-v9-oriole-115b-panplay/`, scenarios 3, 5-8):

- D3D12 FL 11_1 **PASS** (`logic_op` `550ff000`; 115b `ffffffff`): jm-v7-wip 0138. `fl-quick` reports `highest=11_1`.
- Cubes: D3D11 170.4-173.5 fps (115b 170.4), D3D9 89.9-90.0 (89.8). SquidRoom 27.1 (27.2), gears 323.8 (323.0).
- D3D11 FL 11_1 `tir` FAIL: DXVK, not the driver (see a31).

The FTL runs of later builds are `../2026-10-29-ftl-*-115d-panplay/` and newer; D3D12 FL 11_1 `logic_op` on v10/v11 is fixed by jm-v7-wip 0144 (CSF re-emits the DCD flags when the logic op changes).

## Passes

| scenario | pass | result | fps | test line |
|---|---|---|---|---|
| 1 | 01-d3d11-default | ok | 170.4 | `CUBE: PASS api=d3d11 frames=8081 (check pixels)` |
| 2 | 01-d3d9-default | ok | 90.0 | `CUBE: PASS api=d3d9 frames=4260 (check pixels)` |
| 3 | 01-d3d11-default | ok | 173.5 | `CUBE: PASS api=d3d11 frames=8289 (check pixels)` |
| 3 | 02-d3d9-default | ok | 89.9 | `CUBE: PASS api=d3d9 frames=4310 (check pixels)` |
| 4 | 01-d3d11-old | ok | 116.7 | `CUBE: PASS api=d3d11 frames=5615 (check pixels)` |
| 4 | 02-d3d11-default | ok | 171.1 | `CUBE: PASS api=d3d11 frames=8120 (check pixels)` |
| 4 | 03-d3d9-old | ok | 89.9 | `CUBE: PASS api=d3d9 frames=4311 (check pixels)` |
| 4 | 04-d3d9-default | ok | 89.9 | `CUBE: PASS api=d3d9 frames=4309 (check pixels)` |
| 5 | 01-d3d12-default | ok | 27.1 | `DX12: PASS test=d3d12-multithreading frames=1303 secs=48.0 fps=27.1 startup=3.2` |
| 6 | 01-d3d12gears-default | ok | 323.8 | `DX12: PASS test=vkd3d-gears frames=15546 secs=48.0 fps=323.8 startup=1.2` |
| 7 | 01-d3d12fl_11_0-default | ok | 716.8 | `D3DFL: PASS api=d3d12 level=11_0 created tess:PASS,gs:PASS,compute:PASS,mrt:PASS,msaa4x:PASS frames=34407 secs=48.0 fps=716.8 startup=1.9` |
| 7 | 02-d3d12fl_11_1-default | ok | 711.6 | `D3DFL: PASS api=d3d12 level=11_1 created uav_vs:PASS,logic_op:PASS,tir:REPORTED frames=34158 secs=48.0 fps=711.6 startup=2.5` |
| 7 | 03-d3d12fl_12_0-default | ok | 713.2 | `D3DFL: UNSUPPORTED api=d3d12 level=12_0 not_created typed_uav_load:PASS,binding_tier2:PASS,tiled_tier2:UNSUPPORTED frames=34237 secs=48.0 fps=713.2 startup=2.1` |
| 7 | 04-d3d12fl_12_1-default | ok | 712.9 | `D3DFL: UNSUPPORTED api=d3d12 level=12_1 not_created rov:UNSUPPORTED,conservative:UNSUPPORTED frames=34222 secs=48.0 fps=712.9 startup=1.7` |
| 7 | 05-d3d12fl_12_2-default | ok | 709.8 | `D3DFL: UNSUPPORTED api=d3d12 level=12_2 not_created sm6_5:REPORTED,mesh_shader:UNSUPPORTED,dxr1_1:UNSUPPORTED,vrs_tier2:UNSUPPORTED,sampler_feedback:REPORTED frames=34073 secs=48.0 fps=709.8 startup=1.8` |
| 8 | 01-d3d11fl_10_0-default | ok | 244.7 | `D3DFL: PASS api=d3d11 level=10_0 created gs:PASS,stream_out:PASS frames=11746 secs=48.0 fps=244.7 startup=1.4` |
| 8 | 02-d3d11fl_10_1-default | ok | 242.3 | `D3DFL: PASS api=d3d11 level=10_1 created cube_array:PASS,gather4:PASS frames=11630 secs=48.0 fps=242.3 startup=1.5` |
| 8 | 03-d3d11fl_11_0-default | ok | 247.5 | `D3DFL: PASS api=d3d11 level=11_0 created tess:PASS,compute:PASS frames=11883 secs=48.0 fps=247.5 startup=1.7` |
| 8 | 04-d3d11fl_11_1-default | fail (test failed: D3DFL: FAIL api=d3d11 level=11_1 created uav_vs:PASS,logic_op:PASS,tir:FAIL frames=11603 secs=48.0 fps=241.7 startup=1.4) | 244.2 | `D3DFL: FAIL api=d3d11 level=11_1 created uav_vs:PASS,logic_op:PASS,tir:FAIL frames=11603 secs=48.0 fps=241.7 startup=1.4` |
| 8 | 05-d3d10-default | ok | 165.7 | `CUBE: PASS api=d3d10 frames=7916 (check pixels)` |
| 8 | 06-d3d9-default | ok | 90.0 | `CUBE: PASS api=d3d9 frames=4315 (check pixels)` |
| 9 | 01-d3d12fl_all-default | ok | 706.7 | `D3DFL: DONE api=d3d12 level=all 11_0=PASS 11_1=PASS 12_0=UNSUPPORTED 12_1=UNSUPPORTED 12_2=UNSUPPORTED highest=11_1 frames=33922 secs=48.0 fps=706.7 startup=2.8` |
| 9 | 02-d3d11fl_all-default | fail (test failed: D3DFL: FAIL api=d3d11 level=all 10_0=PASS 10_1=PASS 11_0=PASS 11_1=FAIL frames=11929 secs=48.0 fps=248.5 startup=2.0) | 250.0 | `D3DFL: FAIL api=d3d11 level=all 10_0=PASS 10_1=PASS 11_0=PASS 11_1=FAIL frames=11929 secs=48.0 fps=248.5 startup=2.0` |

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
