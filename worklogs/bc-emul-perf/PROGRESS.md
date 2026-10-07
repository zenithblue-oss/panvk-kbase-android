# BCn emulation performance and memory (bc-emul-perf)

Branch `bc-emul-perf`. Target Mali-G615 MC6 (v11 CSF, mt6897), must keep
working on v10-v14 with the old path as fallback.

## Setup

- Mesa tree: `/var/tmp/panvk/wt-bcperf` (archive of 5a07217f + the full
  g615-v11-csf series from main 5df0941, committed as `series-main-5df0941`).
- Build: `/var/tmp/panvk/bcperf-build.sh <tag> [mesa] [builddir-tag]`
  (LLVM22 `LD_LIBRARY_PATH` workaround, host tools from `tmp/rel9`).
- Device runs: binaries and test ICDs under `/data/local/tmp/bcperf/`, loaded
  directly by path (no system ICD change), serialized with
  `flock /var/tmp/panvk/device.lock`.
- Benchmark: `device/bc-perf.c` (+ `bc-perf.vert/.frag`, `bc-perf-spv.h`).
  2048x2048 full mip chain per format, usage SAMPLED|TRANSFER_SRC|TRANSFER_DST
  (what DXVK asks for). Reports image memory, GPU time of the whole-chain
  vkCmdCopyBufferToImage (where the decode happens) and GPU time per
  1920x1080 fullscreen pass for five sampling modes:
  - A/B/C: 4 bilinear/trilinear taps per pixel (1:1, 1:1 transposed,
    4x minified + rotated). Cache friendly.
  - D: 2x minified, one tap at LOD 0.
  - E: 3x minified + 30 degrees, one tap at LOD 0. Texture-bandwidth bound.
  Passes use additive blending (opaque overdraw is removed by forward pixel
  kill, which first made every pass look free).
- Correctness: `tests/dxvk/vulkan/bc/bc_decode.c` + `bc_verify.py` (all 16
  BC formats, sRGB/UNORM/SNORM, 2 mips, 2 layers, image->image copy, blit,
  raw readback, partial 8x8 sub-update) against Pillow / Mesa CPU decoders.

## Native format support on G615 (probe)

- TEXTURE_FEATURES_0 = 0xc7fe001e: no BC bits (mask 0x0001ff80), ETC2/EAC
  and ASTC LDR/HDR native, bit 25 (AFRC) set.
- panvk exposes AFBC modifiers for RGBA8/RGBA16F/B10G11R11/RG8/R8 etc.; AFRC
  is not used anywhere in panvk and fixed-rate compression is not exposed.
- Storage-image, colour-attachment, blend and linear filtering are available
  on every candidate decoded format (R8, RG8, *_SNORM, RGB565, B10G11R11,
  RGBA16F).

## Baseline (main 5df0941 series, G615)

Correctness: 16/16 VERIFY PASS (BC6H exact, BC7 <= 0.35 LSB).

GPU ms; mem = whole-chain image memory (raw BC plane + decoded plane).

| format | mem MiB | upload ms | A | B | C | D | E (bw) |
|---|---|---|---|---|---|---|---|
| BC1 | 24.0 | 5.30 | 0.481 | 0.488 | 0.489 | 0.252 | 0.544 |
| BC3 | 26.7 | 7.04-8.19 | 0.490 | 0.487 | 0.489 | 0.247 | 0.541 |
| BC4 | 24.0 | 4.97 | 0.486 | 0.485 | 0.485 | 0.243 | 0.537 |
| BC5 | 26.7 | 5.90 | 0.486 | 0.486 | 0.485 | 0.243 | 0.540 |
| BC6H UF | 48.0 | 12.90 | 0.492 | 0.487 | 0.487 | 0.309 | 1.224 |
| BC6H SF | 48.0 | 14.35 | 0.493 | 0.487 | 0.512 | 0.309 | 1.217 |
| BC7 | 26.7 | 9.86 | 0.493 | 0.514 | 0.517 | 0.393 | 0.598 |
| ref RGBA8 (AFBC) | 22.1 | 2.70 | 0.493 | 0.492 | 0.488 | 0.245 | 0.348 |
| ref RGBA8 U-interleaved | 21.3 | 3.69 | 0.490 | 0.490 | 0.491 | 0.247 | 0.41-0.45 |
| ref RGBA8 LINEAR | 21.3 | 3.46 | 0.490 | 0.489 | 0.488 | 0.246 | 0.538 |
| ref R8 | 5.8 | 2.28 | 0.493 | 0.492 | 0.488 | 0.243 | 0.243 |
| ref RG8 | 11.2 | 1.11 | 0.493 | 0.492 | 0.493 | 0.283 | 0.339 |
| ref RGBA16F | 43.2 | 2.62 | 0.493 | 0.491 | 0.487 | 0.247 | 0.851 |
| ref RGBA16F LINEAR | 42.7 | 2.44 | 0.488 | 0.486 | 0.486 | 0.312 | 1.215 |
| ref B10G11R11 | 22.1 | 2.81 | 0.492 | 0.491 | 0.490 | 0.247 | 0.361 |
| ref ETC2 RGBA8 (native) | 5.8 | 0.46 | 0.489 | 0.489 | 0.487 | 0.244 | 0.365 |

Findings:
- Cache-friendly sampling (A/B/C) is filter-rate bound at ~0.49 ms for every
  format, emulated or not. Emulation costs nothing there.
- Bandwidth-bound sampling (E) is where emulation loses: the decoded plane is
  LINEAR, which costs ~55% over AFBC for RGBA8 (0.54 vs 0.35) and the
  RGBA16F BC6H plane is 3.4x slower than a tiled B10G11R11 one
  (1.22 vs 0.36). BC4 in RG16 LINEAR is 2.2x slower than tiled R8.
- Memory: the decoded plane dominates (RGBA8 = 8x BC1, RG16 = 8x BC4,
  RGBA16F = 8x BC6H).
- Upload+decode is 2-5x a plain RGBA8 upload (raw copy, full barrier,
  per-texel compute decode).

## Plan / status

1. Fragment decode into an internal shadow colour image (`bc_shadow`), so the
   decoded plane gets the regular modifier choice (AFBC, else
   U-interleaved) and compact renderable formats: BC4 -> R8, BC5 -> RG8
   (unorm/snorm), BC6H UF16 -> B10G11R11, BC6H SF16 -> RGBA16F, rest RGBA8.
   Decode reads the copy source buffer directly (no barrier after the raw
   copy). Compute + LINEAR stays for 3D images and as fallback
   (`PANVK_DEBUG=bc_compute`); `PANVK_DEBUG=bc_wide` keeps 16-bit storage.
   Status: done, patch csf-v11/150.
2. AFRC on the decoded plane (G615 has the AFRC feature bit) for the memory
   cut on BC1/BC3/BC7. Status: todo.
3. Others to evaluate: BC->BC copies via decoded planes, decode batching.

## Method 1: fragment decode into a compact tiled shadow (csf-v11/150)

Correctness: 16/16 PASS, bc_decode BC_DEVICE_FAILS=0. BC6H UF max rel.
error 0.0132 (B10G11R11 rounding, verify run with BC_BC6U_TOL=0.0157).
PANVK_DEBUG=bc_compute and bc_wide also 16/16; bc_compute numbers match the
baseline (BC6H UF 49156 KiB, E 1.214).

| format | mem MiB | upload ms | A | D | E (bw) | E vs base |
|---|---|---|---|---|---|---|
| BC1 | 24.8 | 3.30 | 0.485 | 0.258 | 0.284 | 1.92x |
| BC1 sRGB | 24.8 | 4.26 | 0.493 | 0.270 | 0.309 | |
| BC2 | 27.4 | 3.93 | 0.489 | 0.248 | 0.292 | 1.89x |
| BC3 | 27.4 | 3.93 | 0.493 | 0.243 | 0.289 | 1.87x |
| BC4 | 8.5 (-65%) | 3.43 | 0.488 | 0.242 | 0.242 | 2.22x |
| BC5 | 16.6 (-38%) | 5.36 | 0.489 | 0.243 | 0.243 | 2.22x |
| BC6H UF | 27.4 (-43%) | 8.46 | 0.492 | 0.243 | 0.321 | 3.81x |
| BC6H SF | 48.5 | 9.32 | 0.488 | 0.249 | 0.695 | 1.75x |
| BC7 | 27.4 | 5.84 | 0.489 | 0.247 | 0.287 | 2.08x |

- Bandwidth-bound sampling reaches or beats the native uncompressed
  reference of the same storage format (decoded content is blocky, so
  AFBC compresses it better than the RGBA8 reference pattern).
- Upload+decode 1.3-1.9x faster (no barrier, tile writeback).
- BC1/2/3/7 memory +3% (AFBC header, 4 KiB alignment of the shadow).
  Needs AFRC or raw-plane reduction for a cut.

Rejected / dropped:
- `force_native_bc` probe (sample BC with the real Mali BC format codes
  despite TEXTURE_FEATURES): dropped on request before it was run.
- RGB565 for BC1: DXVK maps DXGI BC1 to VK BC1_RGBA (alpha), and 565 has no
  sRGB variant, so it would almost never apply. Not implemented.
