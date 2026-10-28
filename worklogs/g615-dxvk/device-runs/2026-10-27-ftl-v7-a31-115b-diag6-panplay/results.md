# FTL a31 (Galaxy A31, Mali-G52 r1 MC2, v7 JM) build 115b diag6: D3D11/D3D9 device lost, D3D9 placed map

- Matrix `matrix-1kq23x8ads8us` (project `panvk-device-test`; the `phone-finder-hub` attempt `matrix-3ko646bsguoym`
  hit `TEST_QUOTA_EXCEEDED`); outcome `Passed a31-31-en-portrait`.
- APK `panplay-115.apk` (vc 41). Extra driver `d6` = `/home/flux/build-out/2026-10-27-115b-diag6/libvulkan_panfrost.so`
  (`git-a4bd76a2`): diag4 + decode of the descriptor copy jobs of a failed JM atom (push uniforms, every address
  they read or write checked against `/proc/self/maps`) + gralloc handle log and `MEM_IMPORT` retries (fewer flags,
  the other fds) for the first 4 gralloc BOs.
- `gameloop-override.json`: scenario 1 = D3D11 cube under d6, scenario 2 = D3D9 cube under d6.
  `raw/` has everything but `video.mp4`.

## Result

| task | result |
|---|---|
| D3D11 cube | DEVICE_LOST on the first frame: atoms 14 (vertex/tiler), 15, 16 `event=0x4` |
| D3D9 cube | the 4 MiB vertex buffer placed map works (gralloc fd 1 imported), first frame presented, then DEVICE_LOST: atoms 13-15 `event=0x4` |

**Device lost: the vertex descriptor copy job writes to GPU address 0.** The first unfinished job of the failed atom
is the vertex shader's descriptor copy job. Its push uniforms (`struct pan_nir_desc_copy_info`):

```
kbase: JM atom 14 job 2 table 1 0x6dcac0740 limit 1 ok          <- tables[IMG]
kbase: JM atom 14 job 2 img_attrib 0x0 NULL, copy table 0x6f7ff2680 ok
kbase: JM atom 14 job 2 copy 0: handle 0x10000002 set 1 idx 2 (in set) src 0x6f7f5eda0 ok -> table 1 dst 0x6dcac0740 ok
```

A copy into the IMG table also writes the image's attribute descriptor at `img_attrib_table + idx * 8`, here GPU
address 0: an MMU fault, kbase hard-stops every job of the context, and every atom completes with `event=0x4`
(TERMINATED, no job status). D3D9 shows the same (atom 13 jobs 2 and 6, copies into table 1 with `img_attrib 0x0`).
`attrib_buf_idx_offset` is 0 (no vertex buffer) and the draw is indexed-indirect (helper job 4 ahead of the IDVS job).

Cause (`panvk_draw_prepare_vs_attribs()`, JM): an indirect draw allocates the draw helper's per-vertex-buffer info
array, `num_vbs` entries. The cube's vertex shader pulls its vertices (no vertex buffer), so that is a zero-size
allocation, which `panvk_cmd_alloc_from_pool()` returns as NULL; the function returned
`VK_ERROR_OUT_OF_DEVICE_MEMORY` (its caller ignores it) before it set `img_attrib_table`. Fix: only a non-empty
allocation that fails is out of memory (`panvk/jm: a draw without vertex buffers is not out of memory`, build 115b).

**Placed map: the dma-buf is not fd 0 of the gralloc handle.**

```
kbase: diag6 gralloc BLOB 4194304 B usage 0x1000022: 3 fds, 80 ints
kbase: diag6 gralloc fd[0] 45 -> anon_inode:gralloc_extra, size -1
kbase: diag6 gralloc fd[1] 46 -> /dmabuf:, size 4194304
kbase: diag6 gralloc fd[2] 47 -> /dev/ashmem8cca41a0-..., size 2404
kbase: KBASE_IOCTL_MEM_IMPORT failed: Out of memory              (fd 0, every flag set)
kbase: diag6 import retry, fd[1]
kbase: diag6 import OK with fd[1]
```

Fix: import the handle fd that is a dma-buf (`/proc/self/fd` link `/dmabuf:` or `anon_inode:dmabuf`), folded into
`pan/kmod/kbase: dma-buf BOs from gralloc without a dma-heap or ION heap` (build 115b). Gralloc BOs after the first 4
still imported fd 0 in diag6 and fell back to kbase BOs (`dma-buf allocation of 3609600 bytes failed`).

## Screenshots

- `screenshots/video-start.png`, `video-mid.png`, `video-end.png`: FTL video frames (no harness shot: both cubes were
  lost before the 6 s screenshot).
