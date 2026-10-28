# FTL shiba (Pixel 8, Mali-G715, v11 CSF) 115c diag: D3D12 FL tiler heap OOM

- Matrix `matrix-2f34mkb9cakvw` (project `finalbenchmark-665a8`, results dir `panvk-v11-shiba-115c-diag2-panplay`; the
  first launch died uploading the .so, so the driver went to the bucket first); outcome `Passed shiba-34-en-portrait`.
- APK `panplay-115b.apk` (vc 43). Driver `c1` = `/home/flux/build-out/2026-10-28-115c-diag/libvulkan_panfrost.so`
  (sha256 `49c17a35f4d899d6130c8454be4f02ec84acf507b5d18fba4341fddf3ad04568`): 115b + jm-v7-wip 0136 (tiler bin budget
  divided by the layers of a tiler context; `PANVK_TILER_LAYER_BUDGET=0` = 115b) + diag knobs
  (`PANVK_DIAG_TILER_LOG=1` logs each new tiler config, `PANVK_KBASE_TILER_CHUNK_KB` overrides the 1 MiB kbase chunk).
  Tree `/home/flux/src/mesa-115c-diag` (branch `p115c-diag`). No 0137-0139 in this driver.
- `gameloop-override.json`: scenario 1 = D3D12 FL 11_1 with `fix-log`, `old-log`, `old-2m-log`; scenario 2 = D3D12
  FL 12_1 and D3D11 FL 11_1 with `fix-log`. `raw/` has everything but `video.mp4`.

## Results

| scenario | pass | tiler config of the no-RT pass (65536x65536, 256 layers, 32 tiler contexts) | result |
|---|---|---|---|
| 1 | D3D12 FL 11_1 `fix-log` | mask `0x1fc0`, budget 131064 (1 MiB chunk / 8 layers) | `tir` REPORTED, **no device lost**; `logic_op` FAIL `ffffffff` (no 0138 here) |
| 1 | D3D12 FL 11_1 `old-log` | mask `0x1fe0`, budget 1048512 (115b) | `tir`: `CSF group 3 tiler heap OOM notification`, subqueue timeout, **DEVICE_LOST** |
| 1 | D3D12 FL 11_1 `old-2m-log` | mask `0x1fe0`, budget 2097088, 2 MiB chunks | same OOM and **DEVICE_LOST** |
| 2 | D3D12 FL 12_1 `fix-log` | as `fix-log` | `conservative` **PASS** (1 PS invocation conservative, 0 normal), ROV unsupported, no device lost |
| 2 | D3D11 FL 11_1 `fix-log` | no tiler config with 4 samples or a large area | `uav_vs` and `logic_op` PASS, `tir` FAIL (0 PS invocations with ForcedSampleCount=4) |

## Findings

- The layer budget (0136) removes the OOM: the 11_1 `tir` check and the 12_1 `conservative` check (both DEVICE_LOST
  on 115b) run without a GPU error.
- With the 115b budget the OOM also happens with 2 MiB chunks, although the bins of a full tiler context at mask
  `0x1fe0` (8 layers x 171 KiB at 8 bytes per bin = 1.4 MiB) fit in one. So the rule is not "the bins of the 8 layers
  of a context come from one chunk at 8 bytes per bin": a context needs more (twice that would explain both results).
  `../2026-10-28-ftl-v11-shiba-115c-diag3-panplay/` measures it (4 and 8 MiB chunks with the 115b budget, 512 KiB
  chunks with the layer budget).
- D3D11 `tir`: DXVK's UAV-only draw created no tiler context with 4 samples, so the pass rasterized single-sampled
  (or not at all). diag3 logs the sample state of every draw.

## Screenshots

- `screenshots/01-d3d12fl_11_1-fix-log.png`, `01-d3d12fl_12_1-fix-log.png`, `02-d3d11fl_11_1-fix-log.png`. The
  `old-log` and `old-2m-log` passes lost the device before their screenshot.
