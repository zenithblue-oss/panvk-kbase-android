# FTL shiba (Pixel 8, Mali-G715, v11 CSF) 115c diag3: tiler chunk size the D3D12 FL no-RT pass needs

- Matrix `matrix-1piqsiy4u9njq` (project `finalbenchmark-665a8`, results dir `panvk-v11-shiba-115c-diag3-panplay`);
  outcome `Passed shiba-34-en-portrait`.
- APK `panplay-115b.apk` (vc 43). Driver `c1` = `/home/flux/build-out/2026-10-28-115c-diag3/libvulkan_panfrost.so`
  (sha256 `5bc8ec4125591a07f6a2128170447e2c7d319c841d0009548dc4adc8c5999e5e`, pushed from
  `gs://test-lab-rh5tw441b11v6-yb3zb70vx85mm/panvk-v11-shiba-115c-diag3-so/`): the diag driver of
  `../2026-10-28-ftl-v11-shiba-115c-diag-panplay/` (0136 with a budget of one chunk per 8 layers, chunk size override)
  plus a log of each draw's sample state (`panvk diag ms`). Tree `/home/flux/src/mesa-115c-diag` `361e1e99`.
- `gameloop-override.json`: scenario 1 = D3D12 FL 11_1 with `old-4m`, `fix-512k`, `old-8m`; scenario 2 = D3D11 FL 11_1
  with `fix-log`. `raw/` has everything but `video.mp4`.

## Results: the no-RT pass (65536x65536, 256 layers, 32 tiler contexts of 8 layers)

| budget | mask | bins per layer (`pan_calc_bins_pointer_size`) | chunk | result |
|---|---|---|---|---|
| one layer (115b) | `0x1fe0` | 174784 B | 1 MiB (diag1) | OOM, DEVICE_LOST |
| one layer (115b) | `0x1fe0` | 174784 B | 2 MiB (diag1) | OOM, DEVICE_LOST |
| one layer (115b) | `0x1fe0` | 174784 B | **4 MiB** | **no OOM**, `tir` REPORTED |
| one layer (115b) | `0x1fe0` | 174784 B | **8 MiB** | **no OOM** |
| chunk / 8 layers | `0x1fc0` | 43712 B | 1 MiB (diag1) | no OOM |
| chunk / 8 layers | `0x1fc0` | 43712 B | **512 KiB** | **OOM, DEVICE_LOST** |

A tiler context needs more than 2 MiB but at most 4 MiB at `0x1fe0`, and more than 512 KiB but at most 1 MiB at
`0x1fc0`: twice the bins of its 8 layers (2 x 8 x 174784 B = 2.67 MiB, 2 x 8 x 43712 B = 683 KiB) fits both, once
(1.33 MiB, 342 KiB) fits neither. So on v11 the tiler takes 16 bytes per bin and layer, not the 8 bytes
`pan_calc_bins_pointer_size()` uses before v12, and the bins of all layers of a context come from one chunk. 0136
(final) divides the budget by 2 x layers on v10/v11: 1 MiB chunks, 8 layers -> 65532 B per layer -> mask `0x1fc0`,
the configuration that passes.

## D3D11 FL 11_1 `tir`

`tir` FAIL as before (0 PS invocations with ForcedSampleCount=4). Every draw of the pass without attachments that
reached the driver had `rasterizationSamples` 1 (`panvk diag ms: raster 1 layout 0 nr 0 attachments 0x0`, recorded
in DXVK's secondary command buffers; no draw with 4 samples). The driver never got a sample count of 4, so the
failure is not in the driver's no-attachment sample handling: DXVK 3.1.1 rasterizes this UAV-only draw
single-sampled.

## Screenshots

- `screenshots/01-d3d12fl_11_1-old-4m.png`, `03-d3d12fl_11_1-old-8m.png`, `01-d3d11fl_11_1-fix-log.png`. The
  `fix-512k` pass lost the device before its screenshot.
