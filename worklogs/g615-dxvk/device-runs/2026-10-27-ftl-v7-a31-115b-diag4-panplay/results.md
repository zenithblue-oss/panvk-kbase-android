# FTL a31 (Galaxy A31, Mali-G52 r1 MC2, v7 JM) build 115b diag4: D3D11 device lost, D3D9 placed map, gears, SquidRoom

- Matrix `matrix-1m1osgl2krasz` (project `phone-finder-hub`); outcome `Passed a31-31-en-portrait`.
- APK `panplay-115.apk` (vc 41). Extra driver `d4` = `/home/flux/build-out/2026-10-27-115b-diag4/libvulkan_panfrost.so`
  (sha256 `691459164666734d...`, `git-3d82a178`): IDVS job local dependency on its descriptor copy / indirect helper
  job, gralloc BLOB AHardwareBuffer as the dma-buf allocator when there is no dma-heap and no ION heap, raw
  descriptor of the first unfinished job of a failed atom.
- `gameloop-override.json`: scenario 1 = D3D11 cube under d4 and d4-noidvs (`PANVK_V7_IDVS=0`), scenario 2 = D3D9
  cube, D3D12 gears and D3D12 SquidRoom under d4. `raw/` has everything but `video.mp4`.

## Result

| task | result |
|---|---|
| D3D11 cube, d4 | DEVICE_LOST: atoms 14 (vertex/tiler chain), 15 (fragment), 16 (compute) `event=0x4` |
| D3D11 cube, d4-noidvs | DEVICE_LOST, same atoms |
| D3D9 cube | exit 1: `vkMapMemory2EXT failed: -5` (`VK_ERROR_MEMORY_MAP_FAILED`), `CUBE: FAIL VB` |
| D3D12 gears | exit 0, 109 fps, still only the red gear, drawn twice |
| D3D12 SquidRoom | exit 0, renders correctly, 1.2-1.3 fps (about 800 ms per frame) |

- **D3D11: the IDVS dependency was a real bug but not the cause of the kill.** The dump shows the fix in effect (the
  IDVS job now has deps `5/2`: its helper job and the previous tiler job), and the chain still dies the same way: jobs
  0 and 1 DONE, the first descriptor copy job (compute, index 3; index 4 with IDVS off) and everything after it not
  finished, all three atoms `event=0x4` (TERMINATED, with no job status written). An MMU fault of the context gives
  exactly that (kbase hard-stops every running job of the context), and FTL has no kernel log. The descriptor copy
  job copies one descriptor (`job_task_split` 3); its push uniforms, shader and TLS descriptor are not in the dump.
  diag6 decodes them at the failure and checks every address against `/proc/self/maps`.
- **D3D9: the gralloc BLOB is allocated, kbase does not import it.** `kbase: KBASE_IOCTL_MEM_IMPORT failed: Out of
  memory`, then the BO falls back to a kbase BO and the placed map fails. kbase returns ENOMEM for most import
  errors (bad flags, `dma_buf_attach`/map failure, no cookie). diag6 logs the gralloc handle and retries the import
  with fewer flags, the other fds and a CPU-only BLOB.
- **Gears**: unchanged by the dependency fix, so the per-draw data of draws 2 and 3 (color, transform) is the
  problem, not job ordering. Not investigated further in 115b.
- **SquidRoom**: no driver warnings; slow but correct.

## Screenshots

- `screenshots/video-start.png`, `video-mid.png`, `video-end.png`: FTL video frames.
- `screenshots/02-d3d12gears-d4-harness.png`: gears (red gear twice, green and blue missing).
- `screenshots/03-d3d12-d4-harness.png`: SquidRoom.
