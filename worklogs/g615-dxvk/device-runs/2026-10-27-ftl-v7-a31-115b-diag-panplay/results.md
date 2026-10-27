# FTL a31 (Galaxy A31, Mali-G52 r1 MC2, v7 JM) build 115b diag: D3D11 device lost, D3D9 placed map, gears

- Matrix `matrix-5eclxbodic7ba` (project `phone-finder-hub`); outcome `Passed a31-31-en-portrait`.
- APK `panplay-115.apk` (vc 41). Extra drivers: `dg2` = `/home/flux/build-out/2026-10-27-115b-diag2/libvulkan_panfrost.so`
  (sha256 `5702fed2...`, `git-2b703a0863`: 115 + failed-atom job chain dump, ION heap list with an MTK `ion_mm_heap`
  fallback, dma-buf alloc fallback); `r114` = build 114 (`git-3071c6dfa2`).
- `gameloop-override.json`: scenario 1 = D3D11 cube under rel / dg2 / r114 / dg2-old (the 27 default-on 115 knobs at
  their 114 values) / dg2-warm / dg2-zero; scenario 2 = D3D9 cube and D3D12 gears under dg2 and r114.
  `raw/` has everything but `video.mp4`.

## Result

| task | rel | dg2 | r114 | dg2-old | dg2-warm | dg2-zero |
|---|---|---|---|---|---|---|
| D3D11 cube | DEVICE_LOST | DEVICE_LOST | DEVICE_LOST | DEVICE_LOST | atom 14 killed | atom 14 killed |
| D3D9 cube | - | map failed | map failed | - | - | - |
| D3D12 gears | - | red gear twice | exit 3 | - | - | - |

- **D3D11: every driver and knob set dies on the same atom.** `JM atom 14 failed: event=0x4 core_req=0x16`, then
  atoms 15 (fragment) and 16 (compute) with event 0x4 (TERMINATED, a hard stop: the context was killed, with no
  job status written). The chain dump is the same in every pass (same addresses): jobs 0 and 1 DONE, job 2 (index 3,
  compute) and everything after it not started or not finished. The dependencies show a bug (diag4 fixed it, and
  the atoms still die the same way, so it is not the cause of the kill: `../2026-10-27-ftl-v7-a31-115b-diag4-panplay/`):

  ```
  job 2 index 3  type 4 (compute) deps 0/0   vertex descriptor copy
  job 3 index 4  type 4 (compute) deps 0/0   fragment descriptor copy
  job 4 index 5  type 4 (compute) deps 0/3   indexed-indirect helper (patches the IDVS job)
  job 5 index 6  type 10 (IDVS)   deps 0/2   <- should wait for index 5; waits only for the previous tiler job (2)
  ```

  `pan_jc_add_job()` replaces the global dependency of a tiling job (IDVS is one) with the previous tiler job, and
  panvk passed the IDVS job's producer (descriptor copy job for direct draws, the helper that writes its draw
  parameters for indexed/indirect draws) as that global dependency. The IDVS job runs alongside the jobs that write its
  descriptors and draw parameters. Fix: pass the producer as dependency_1, as `add_tiler_jobs()` does (diag4 and the
  fix patch). Gears shows only the red gear, drawn twice and offset (`screenshots/03-d3d12gears-dg2-harness.png`);
  the dependency fix does not change that (diag4). Not a 115 regression: the 114 driver fails the same way.
- **D3D9: no dma-buf allocator.** `ION_IOC_HEAP_QUERY failed: Invalid argument`, `no usable ION system heap`, no
  `/dev/dma_heap` (Android 12 on a MediaTek 4.14 kernel), so host-visible memory are kbase BOs and the placed map of
  the first 4 MiB vertex buffer fails (`VK_ERROR_MEMORY_MAP_FAILED`, `CUBE: FAIL VB`). diag4 and the fix allocate
  those BOs from gralloc (a BLOB AHardwareBuffer) instead.
- dg2-warm and dg2-zero: same atom 14 kill; the harness did not flag DEVICE_LOST only because DXVK did not log it
  before the exit.

## Screenshots

- `screenshots/video-start.png`, `video-mid.png`, `video-end.png`: FTL video frames.
- `screenshots/03-d3d12gears-dg2-harness.png`: gears with the diag2 driver (red gear twice, green and blue missing).
