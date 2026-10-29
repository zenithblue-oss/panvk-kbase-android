# FTL shiba (Pixel 8, Mali-G715 MC7, v11 CSF) build 115c: PanPlay confirm (scenarios 1-9)

- Matrix `matrix-1k7s0pxdf2ke3` (project `finalbenchmark-665a8`), results `gs://test-lab-rh5tw441b11v6-yb3zb70vx85mm/panvk-v11-shiba-115c-panplay/`; outcome `Passed shiba-34-en-portrait --`.
- APK `/home/flux/build-out/apks/panplay-115c.apk` (sha256 `a16f9096...`), `dev.zenithblue.panvklauncher` 1.3.0 (vc 44), bundled driver build 115c (`eb44336a...`, driverInfo `Mesa 26.3.0-devel (git-15fd70c7c4)`).
- Scenarios 1-9 (1 D3D11 cube, 2 D3D9 cube, 3 both, 4 cube A/B `old` vs `default` knobs, 5 SquidRoom, 6 gears, 7 D3D12 FL 11_0-12_2, 8 D3D11 FL 10_0-11_1 + D3D10 + D3D9 cubes, 9 FL all-levels quick runs). `gameloop-override.json`: warm-up 10 s, measure 30 s (as 115b). `raw/` has everything but `video.mp4`.

## Result: 23 passes, no GPU fault (no CSF group termination, no device lost)

Against 115b (`../2026-10-28-ftl-v11-shiba-115b-panplay/`, scenarios 3, 5-8):

- D3D12 FL 11_1 `tir` REPORTED and FL 12_1 `conservative` **PASS**; both lost the device in 115b (tiler heap OOM, group terminated): jm-v7-wip 0136.
- D3D12 FL 11_1 `logic_op` still FAIL (`ffffffff`): on CSF the DCD flags were not re-emitted when only the logic op changed, so the INVERT draw kept the plain draw's forward pixel kill (fixed later by 0144).
- Cubes: D3D11 412.7-421.7 fps (115b 414.3), D3D9 118.5-119.0 (119.2). SquidRoom 78.1 (78.6), gears 187.8 (187.2).
- D3D11 FL 11_1 `tir` FAIL: DXVK, not the driver (see a31).

The FTL runs of later builds are `../2026-10-29-ftl-*-115d-panplay/` and newer; D3D12 FL 11_1 `logic_op` on v10/v11 is fixed by jm-v7-wip 0144 (CSF re-emits the DCD flags when the logic op changes).

## Passes

| scenario | pass | result | fps | test line |
|---|---|---|---|---|
| 1 | 01-d3d11-default | ok | 421.7 | `CUBE: PASS api=d3d11 frames=20293 (check pixels)` |
| 2 | 01-d3d9-default | ok | 119.0 | `CUBE: PASS api=d3d9 frames=5639 (check pixels)` |
| 3 | 01-d3d11-default | ok | 412.7 | `CUBE: PASS api=d3d11 frames=19584 (check pixels)` |
| 3 | 02-d3d9-default | ok | 118.5 | `CUBE: PASS api=d3d9 frames=5677 (check pixels)` |
| 4 | 01-d3d11-old | ok | 160.4 | `CUBE: PASS api=d3d11 frames=7931 (check pixels)` |
| 4 | 02-d3d11-default | ok | 410.0 | `CUBE: PASS api=d3d11 frames=19616 (check pixels)` |
| 4 | 03-d3d9-old | ok | 117.9 | `CUBE: PASS api=d3d9 frames=5619 (check pixels)` |
| 4 | 04-d3d9-default | ok | 118.6 | `CUBE: PASS api=d3d9 frames=5661 (check pixels)` |
| 5 | 01-d3d12-default | ok | 78.1 | `DX12: PASS test=d3d12-multithreading frames=3749 secs=48.0 fps=78.1 startup=5.2` |
| 6 | 01-d3d12gears-default | ok | 187.8 | `DX12: PASS test=vkd3d-gears frames=9016 secs=48.0 fps=187.8 startup=2.9` |
| 7 | 01-d3d12fl_11_0-default | ok | 211.3 | `D3DFL: PASS api=d3d12 level=11_0 created tess:PASS,gs:PASS,compute:PASS,mrt:PASS,msaa4x:PASS frames=10143 secs=48.0 fps=211.3 startup=4.0` |
| 7 | 02-d3d12fl_11_1-default | fail (test failed: D3DFL: FAIL api=d3d12 level=11_1 created uav_vs:PASS,logic_op:FAIL,tir:REPORTED frames=10231 secs=48.0 fps=213.1 startup=10.8) | 212.1 | `D3DFL: FAIL api=d3d12 level=11_1 created uav_vs:PASS,logic_op:FAIL,tir:REPORTED frames=10231 secs=48.0 fps=213.1 startup=10.8` |
| 7 | 03-d3d12fl_12_0-default | ok | 208.4 | `D3DFL: UNSUPPORTED api=d3d12 level=12_0 not_created typed_uav_load:PASS,binding_tier2:PASS,tiled_tier2:UNSUPPORTED frames=10005 secs=48.0 fps=208.4 startup=2.7` |
| 7 | 04-d3d12fl_12_1-default | ok | 210.1 | `D3DFL: UNSUPPORTED api=d3d12 level=12_1 not_created rov:UNSUPPORTED,conservative:PASS frames=10087 secs=48.0 fps=210.1 startup=10.4` |
| 7 | 05-d3d12fl_12_2-default | ok | 212.6 | `D3DFL: UNSUPPORTED api=d3d12 level=12_2 not_created sm6_5:REPORTED,mesh_shader:UNSUPPORTED,dxr1_1:UNSUPPORTED,vrs_tier2:UNSUPPORTED,sampler_feedback:REPORTED frames=10208 secs=48.0 fps=212.6 startup=3.2` |
| 8 | 01-d3d11fl_10_0-default | ok | 442.9 | `D3DFL: PASS api=d3d11 level=10_0 created gs:PASS,stream_out:PASS frames=21259 secs=48.0 fps=442.9 startup=2.8` |
| 8 | 02-d3d11fl_10_1-default | ok | 411.6 | `D3DFL: PASS api=d3d11 level=10_1 created cube_array:PASS,gather4:PASS frames=19760 secs=48.0 fps=411.6 startup=2.8` |
| 8 | 03-d3d11fl_11_0-default | ok | 413.6 | `D3DFL: PASS api=d3d11 level=11_0 created tess:PASS,compute:PASS frames=19853 secs=48.0 fps=413.6 startup=2.8` |
| 8 | 04-d3d11fl_11_1-default | fail (test failed: D3DFL: FAIL api=d3d11 level=11_1 created uav_vs:PASS,logic_op:PASS,tir:FAIL frames=19464 secs=48.0 fps=405.5 startup=2.9) | 402.5 | `D3DFL: FAIL api=d3d11 level=11_1 created uav_vs:PASS,logic_op:PASS,tir:FAIL frames=19464 secs=48.0 fps=405.5 startup=2.9` |
| 8 | 05-d3d10-default | ok | 400.3 | `CUBE: PASS api=d3d10 frames=19299 (check pixels)` |
| 8 | 06-d3d9-default | ok | 118.5 | `CUBE: PASS api=d3d9 frames=5660 (check pixels)` |
| 9 | 01-d3d12fl_all-default | fail (test failed: D3DFL: FAIL api=d3d12 level=all 11_0=PASS 11_1=FAIL 12_0=UNSUPPORTED 12_1=UNSUPPORTED 12_2=UNSUPPORTED highest=11_1 frames=10021 secs=48.0 fps=208.7 startup=18.6) | 206.4 | `D3DFL: FAIL api=d3d12 level=all 11_0=PASS 11_1=FAIL 12_0=UNSUPPORTED 12_1=UNSUPPORTED 12_2=UNSUPPORTED highest=11_1 frames=10021 secs=48.0 fps=208.7 startup=18.6` |
| 9 | 02-d3d11fl_all-default | fail (test failed: D3DFL: FAIL api=d3d11 level=all 10_0=PASS 10_1=PASS 11_0=PASS 11_1=FAIL frames=20018 secs=48.0 fps=417.0 startup=3.6) | 415.0 | `D3DFL: FAIL api=d3d11 level=all 10_0=PASS 10_1=PASS 11_0=PASS 11_1=FAIL frames=20018 secs=48.0 fps=417.0 startup=3.6` |

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
