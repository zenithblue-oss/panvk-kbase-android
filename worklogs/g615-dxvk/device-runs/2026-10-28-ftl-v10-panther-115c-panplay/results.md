# FTL panther (Pixel 7, Mali-G710, v10 CSF) build 115c: PanPlay confirm (scenarios 1-9)

- Matrix `matrix-310xn56dzvpjy` (project `finalbenchmark-665a8`), results `gs://test-lab-rh5tw441b11v6-yb3zb70vx85mm/panvk-v10-panther-115c-panplay/`; outcome `Passed panther-33-en-portrait --`.
- APK `/home/flux/build-out/apks/panplay-115c.apk` (sha256 `a16f9096...`), `dev.zenithblue.panvklauncher` 1.3.0 (vc 44), bundled driver build 115c (`eb44336a...`, driverInfo `Mesa 26.3.0-devel (git-15fd70c7c4)`).
- Scenarios 1-9 (1 D3D11 cube, 2 D3D9 cube, 3 both, 4 cube A/B `old` vs `default` knobs, 5 SquidRoom, 6 gears, 7 D3D12 FL 11_0-12_2, 8 D3D11 FL 10_0-11_1 + D3D10 + D3D9 cubes, 9 FL all-levels quick runs). `gameloop-override.json`: warm-up 10 s, measure 30 s (as 115b). `raw/` has everything but `video.mp4`.

## Result: 23 passes, no GPU fault (no CSF group termination, no device lost)

Against 115b (`../2026-10-28-ftl-v10-panther-115b-panplay/`, scenarios 3, 5-8):

- D3D12 FL 11_1 `tir` REPORTED; 115b lost the device there (tiler heap OOM): jm-v7-wip 0136.
- D3D12 FL 11_1 `logic_op` still FAIL (`ffffffff`), as on shiba (fixed later by 0144).
- Cubes: D3D11 565.5-575.3 fps (115b 572.1), D3D9 90.0 (90.0). SquidRoom 84.4 (85.0). Gears 381.3 (115b 431.1; 115d 407.1, 116 415.1 on the same model): run-to-run spread, not investigated.
- D3D11 FL 11_1 `tir` FAIL: DXVK, not the driver (see a31).

The FTL runs of later builds are `../2026-10-29-ftl-*-115d-panplay/` and newer; D3D12 FL 11_1 `logic_op` on v10/v11 is fixed by jm-v7-wip 0144 (CSF re-emits the DCD flags when the logic op changes).

## Passes

| scenario | pass | result | fps | test line |
|---|---|---|---|---|
| 1 | 01-d3d11-default | ok | 575.3 | `CUBE: PASS api=d3d11 frames=27605 (check pixels)` |
| 2 | 01-d3d9-default | ok | 90.0 | `CUBE: PASS api=d3d9 frames=4253 (check pixels)` |
| 3 | 01-d3d11-default | ok | 565.5 | `CUBE: PASS api=d3d11 frames=26831 (check pixels)` |
| 3 | 02-d3d9-default | ok | 90.0 | `CUBE: PASS api=d3d9 frames=4314 (check pixels)` |
| 4 | 01-d3d11-old | ok | 105.2 | `CUBE: PASS api=d3d11 frames=5087 (check pixels)` |
| 4 | 02-d3d11-default | ok | 560.0 | `CUBE: PASS api=d3d11 frames=26848 (check pixels)` |
| 4 | 03-d3d9-old | ok | 84.4 | `CUBE: PASS api=d3d9 frames=4039 (check pixels)` |
| 4 | 04-d3d9-default | ok | 90.0 | `CUBE: PASS api=d3d9 frames=4312 (check pixels)` |
| 5 | 01-d3d12-default | ok | 84.4 | `DX12: PASS test=d3d12-multithreading frames=4052 secs=48.0 fps=84.4 startup=3.9` |
| 6 | 01-d3d12gears-default | ok | 381.3 | `DX12: PASS test=vkd3d-gears frames=18304 secs=48.0 fps=381.3 startup=1.4` |
| 7 | 01-d3d12fl_11_0-default | ok | 393.4 | `D3DFL: PASS api=d3d12 level=11_0 created tess:PASS,gs:PASS,compute:PASS,mrt:PASS,msaa4x:PASS frames=18891 secs=48.0 fps=393.4 startup=2.9` |
| 7 | 02-d3d12fl_11_1-default | fail (test failed: D3DFL: FAIL api=d3d12 level=11_1 created uav_vs:PASS,logic_op:FAIL,tir:REPORTED frames=22877 secs=48.0 fps=476.6 startup=3.8) | 478.8 | `D3DFL: FAIL api=d3d12 level=11_1 created uav_vs:PASS,logic_op:FAIL,tir:REPORTED frames=22877 secs=48.0 fps=476.6 startup=3.8` |
| 7 | 03-d3d12fl_12_0-default | ok | 486.8 | `D3DFL: UNSUPPORTED api=d3d12 level=12_0 not_created typed_uav_load:PASS,binding_tier2:PASS,tiled_tier2:UNSUPPORTED frames=23375 secs=48.0 fps=486.8 startup=1.9` |
| 7 | 04-d3d12fl_12_1-default | ok | 487.2 | `D3DFL: UNSUPPORTED api=d3d12 level=12_1 not_created rov:UNSUPPORTED,conservative:UNSUPPORTED frames=23385 secs=48.0 fps=487.2 startup=1.7` |
| 7 | 05-d3d12fl_12_2-default | ok | 482.9 | `D3DFL: UNSUPPORTED api=d3d12 level=12_2 not_created sm6_5:REPORTED,mesh_shader:UNSUPPORTED,dxr1_1:UNSUPPORTED,vrs_tier2:UNSUPPORTED,sampler_feedback:REPORTED frames=23181 secs=48.0 fps=482.9 startup=1.7` |
| 8 | 01-d3d11fl_10_0-default | ok | 902.7 | `D3DFL: PASS api=d3d11 level=10_0 created gs:PASS,stream_out:PASS frames=43332 secs=48.0 fps=902.7 startup=1.5` |
| 8 | 02-d3d11fl_10_1-default | ok | 753.9 | `D3DFL: PASS api=d3d11 level=10_1 created cube_array:PASS,gather4:PASS frames=36187 secs=48.0 fps=753.9 startup=1.5` |
| 8 | 03-d3d11fl_11_0-default | ok | 760.5 | `D3DFL: PASS api=d3d11 level=11_0 created tess:PASS,compute:PASS frames=36504 secs=48.0 fps=760.5 startup=2.0` |
| 8 | 04-d3d11fl_11_1-default | fail (test failed: D3DFL: FAIL api=d3d11 level=11_1 created uav_vs:PASS,logic_op:PASS,tir:FAIL frames=36598 secs=48.0 fps=762.4 startup=1.8) | 753.3 | `D3DFL: FAIL api=d3d11 level=11_1 created uav_vs:PASS,logic_op:PASS,tir:FAIL frames=36598 secs=48.0 fps=762.4 startup=1.8` |
| 8 | 05-d3d10-default | ok | 491.6 | `CUBE: PASS api=d3d10 frames=24014 (check pixels)` |
| 8 | 06-d3d9-default | ok | 90.0 | `CUBE: PASS api=d3d9 frames=4307 (check pixels)` |
| 9 | 01-d3d12fl_all-default | fail (test failed: D3DFL: FAIL api=d3d12 level=all 11_0=PASS 11_1=FAIL 12_0=UNSUPPORTED 12_1=UNSUPPORTED 12_2=UNSUPPORTED highest=11_1 frames=22419 secs=48.0 fps=467.0 startup=4.1) | 458.5 | `D3DFL: FAIL api=d3d12 level=all 11_0=PASS 11_1=FAIL 12_0=UNSUPPORTED 12_1=UNSUPPORTED 12_2=UNSUPPORTED highest=11_1 frames=22419 secs=48.0 fps=467.0 startup=4.1` |
| 9 | 02-d3d11fl_all-default | fail (test failed: D3DFL: FAIL api=d3d11 level=all 10_0=PASS 10_1=PASS 11_0=PASS 11_1=FAIL frames=42404 secs=48.0 fps=883.4 startup=1.9) | 893.4 | `D3DFL: FAIL api=d3d11 level=all 10_0=PASS 10_1=PASS 11_0=PASS 11_1=FAIL frames=42404 secs=48.0 fps=883.4 startup=1.9` |

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
