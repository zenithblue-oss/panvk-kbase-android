# FTL panther (Pixel 7, Mali-G710, v10 CSF) build 115b: PanPlay confirm

- Matrix `matrix-1ci26wasrgdwy` (project `finalbenchmark-665a8`; `panvk-device-test` refused `matrix-3h15vqu508ppe`
  with `TEST_QUOTA_EXCEEDED`); outcome `Passed panther-33-en-portrait`.
- APK `/home/flux/build-out/apks/panplay-115b.apk` (sha256 `e5872f5b...`), `dev.zenithblue.panvklauncher` 1.3.0 (vc 43),
  bundled driver build 115b (`d72e3059...`, driverInfo `Mesa 26.3.0-devel (git-ad5cd69141)`, every pass).
- Scenarios 3 (D3D11 + D3D9 cubes), 5 (D3D12 SquidRoom), 6 (D3D12 gears), 7 (D3D12 feature levels), 8 (D3D11 feature
  levels, D3D10 and D3D9 cubes). `gameloop-override.json`: warm-up 10 s, measure 30 s. `raw/` has everything but
  `video.mp4`.
- Before (build 115, `../2026-10-26-ftl-v10-panther-115-panplay/`): the D3D11 cube, the D3D9 cube and gears hung the GPU
  queue after 1.5-17.7 s (`kbase: timeout on subqueue 0`, `error 0x0`), DEVICE_LOST.

## Result: the cube and gears hangs are gone

| scenario | pass | result |
|---|---|---|
| 3 | D3D11 cube | **PASS**, 27444 frames, 572.1 fps (1% low 221.8), no timeout |
| 3 | D3D9 cube | **PASS**, 4260 frames, 90.0 fps (vsync) |
| 5 | D3D12 SquidRoom | PASS, 85.0 fps |
| 6 | D3D12 gears | **PASS**, 20696 frames, 431.1 fps, all three gears |
| 7 | D3D12 FL 11_0 | PASS (tess, gs, compute, mrt, msaa4x), 481.9 fps |
| 7 | D3D12 FL 11_1 | **FAIL, DEVICE_LOST**: `logic_op` INVERT gives `ffffffff` (want `550ff000`), then the `tir` check's submit lost the device (below) |
| 7 | D3D12 FL 12_0 | UNSUPPORTED: not created (`typed_uav_load` and `binding_tier2` pass, `tiled_tier2` unsupported: no sparse on kbase) |
| 7 | D3D12 FL 12_1 | UNSUPPORTED (no ROV, no conservative rasterization) |
| 7 | D3D12 FL 12_2 | UNSUPPORTED |
| 8 | D3D11 FL 10_0, 10_1, 11_0 | PASS, 952-992 fps |
| 8 | D3D11 FL 11_1 | FAIL: `tir` (0 PS invocations with ForcedSampleCount=4); `uav_vs` and `logic_op` pass |
| 8 | D3D10 cube | PASS, 27076 frames |
| 8 | D3D9 cube | PASS, 4311 frames |

No `timeout on subqueue`, CS fault or failed group in any pass but D3D12 FL 11_1.

**New: D3D12 FL 11_1 `tir` check, CSF tiler heap OOM.** The check draws a UAV-only band (no render target) and its
submit is lost:

```
kbase: CSF group 3 tiler heap OOM notification
kbase: timeout on subqueue 0: seqno 8, ls_copy 0, target 10, ..., active 0, error 0x0
kbase: queue syncobj 0: seqno 12 / 1: seqno 17 / 2: seqno 14, error 0x0
D3DFL: feature level=11_1 tir FAIL submit/readback failed
```

`BASE_GPU_QUEUE_GROUP_ERROR_TILER_HEAP_OOM`: kbase could not grow the tiler heap and terminated the group (no CSI
exception handler is requested on kbase, see `kbase_kmod.c`). This is not the cross-subqueue wait cycle that 115b
fixes; it is the "tiler heap OOM" CSF DEVICE_LOST class of `HANDOFF-2026-10-16.md` (A). Open.

## Screenshots

- `screenshots/01-d3d11-default-harness.png`, `02-d3d9-default-harness.png`: the cubes.
- `screenshots/01-d3d12-default-harness.png`: SquidRoom; `01-d3d12gears-default-harness.png`: gears (all three).
- `screenshots/0N-d3d12fl_*`, `0N-d3d11fl_*`, `05-d3d10-default-harness.png`, `06-d3d9-default-harness.png`: the
  feature level tests and the scenario 8 cubes.
- `screenshots/video-start.png`, `video-mid.png`, `video-end.png`: FTL video frames.
