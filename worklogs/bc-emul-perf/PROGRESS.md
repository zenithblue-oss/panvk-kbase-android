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
   cut on BC1/BC3/BC7. Status: done, patch csf-v11/151.
3. Others evaluated, see "Other options".

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

## Method 2: AFRC on the RGBA8 shadow (csf-v11/151)

- RGBA8 shadow (BC1/2/3/7) created with a one-entry DRM modifier list:
  AFRC rot, 32-byte coding units (4 bpc, 16 bpp). `PANVK_BC_AFRC=32|24|16|0`.
- Lib bug found and fixed: AFRC row stride used the mip width rounded down
  to whole 32 px tiles. Small or non-aligned levels got a zero row stride
  (garbage at PSNR 4-8 dB, GPU fault with CU24).
- Quality (`/var/tmp/panvk/bcperf-quality.sh`): BCPERF_SRC = 1024x1024 crop
  of a MiSide frame and of the dxcube test frame, LOD0 blitted to RGBA8 and
  compared with the exact decode (PANVK_BC_AFRC=0). BC's own error vs
  source: 35-44 dB.

| CU | BC1 PSNR / max | BC7 PSNR / max | BC4 (R8) | BC5 (RG8) |
|---|---|---|---|---|
| 32 | 66-75 dB / 4 | 64-73 dB / 5 | 49-52 dB / 26 | 49-51 dB / 50 |
| 24 | 57-64 dB / 15 | 56-64 dB / 12 | 43-46 dB / 54 | 43-46 dB / 53 |
| 16 | 48-55 dB / 38 | 48-55 dB / 40 | 37-40 dB / 96 | 37-40 dB / 108 |

- Decision: CU32 on by default for RGBA8 only. R8/RG8 rejected (25-50 LSB
  max error on height/normal-map style data). CU24/16 opt-in.
- bc_verify with AFRC on fails the 2 LSB tolerance on random blocks
  (max 13-38): expected, the exactness gate runs with PANVK_BC_AFRC=0.

| format | mem MiB (CU32) | E ms | CU24 mem | CU16 mem |
|---|---|---|---|---|
| BC1 | 13.3 (-44%) | 0.248 (2.19x) | 10.7 (-56%) | 8.0 (-67%) |
| BC3 | 16.0 (-40%) | 0.250 (2.16x) | 13.3 (-50%) | 10.7 (-60%) |
| BC7 | 16.0 (-40%) | 0.249 (2.4x) | 13.3 (-50%) | 10.7 (-60%) |

- Cube-compatible images keep the AFBC shadow (AFRC cube sampling not
  tested; bc_decode has no cube case).

## Other options

- Free/shrink the raw plane: rejected. It is needed for image->buffer
  readback, BC-source image copies and BC->BC copies (DXVK sets
  TRANSFER_SRC on every texture; D3D11 CopySubresourceRegion between BC
  textures is used by texture streaming). With AFRC it is now 20% (BC1) /
  33% (BC3/7) of the image; dropping it would need a re-encode path.
- Lazy decode (decode at first sample): rejected. Upload+decode is one-time
  and now 3-6 ms per 2048x2048 chain; laziness would add per-draw tracking.
- Barrier narrowing: done in 150. Buffer->image copies have no barrier
  between raw copy and decode (decode reads the copy source). Image->image
  copies use a FRAGMENT_SHADER read barrier instead of ALL_COMMANDS.
- Decode batching across copy calls: not done. One render pass per region;
  upload is no longer the bottleneck.
- BC->ASTC/ETC2 transcode: not tried (rejected in the plan; AFRC already gives
  ETC2-class bandwidth: E 0.249 ms vs native ETC2 0.365 ms).

## Final (G615, csf-v11/150 + 151, default env)

panvk-test APK proof: side-by-side debug APK `dev.zenithblue.panvktest.bcperf`
(`-PappIdSuffix=.bcperf -PpanvkSo=dist-afrc5`), autorun bc_decode + bc_perf,
logs in `apk-proof/`. bc_decode BC_DEVICE_FAILS=0. bc_verify on the APK dump:
BC4/BC5/BC6H PASS, RGBA8 formats over the 2 LSB tolerance by AFRC on random
blocks (expected); 16/16 PASS with PANVK_BC_AFRC=0 and with bc_compute.

| format | mem MiB base -> final | upload ms | E ms base -> final | speedup |
|---|---|---|---|---|
| BC1 | 24.0 -> 13.3 (-44%) | 5.30 -> 3.3 | 0.544 -> 0.250 | 2.2x |
| BC3 | 26.7 -> 16.0 (-40%) | 7.0 -> 3.9 | 0.541 -> 0.248 | 2.2x |
| BC4 | 24.0 -> 8.5 (-65%) | 4.97 -> 3.0 | 0.537 -> 0.244 | 2.2x |
| BC5 | 26.7 -> 16.6 (-38%) | 5.90 -> 3.5 | 0.540 -> 0.244 | 2.2x |
| BC6H UF | 48.0 -> 27.4 (-43%) | 12.9 -> 8.5 | 1.224 -> 0.320 | 3.8x |
| BC6H SF | 48.0 -> 48.5 (+1%) | 14.4 -> 9.3 | 1.217 -> 0.695 | 1.75x |
| BC7 | 26.7 -> 16.0 (-40%) | 9.9 -> 5.9 | 0.598 -> 0.251 | 2.4x |

Cache-friendly sampling (A/B/C) unchanged at ~0.49 ms (filter bound).
With PANVK_BC_AFRC=24: BC1 -56%, BC3/BC7 -50% (max error 9-15 LSB on real
pictures).

## Game check

Not run. Games run inside the launcher (dev.zenithblue.panvklauncher); using
this driver needs either an imported driver in the launcher (changes its
state) or the run-as harness with an X server. The only X server on the
device (termux-x11 :0) belongs to another running proot desktop session, and
the beta.18 agent is using the device. Needs a slot with the device free:
MiSide menu/gameplay with DXVK_HUD=full, base vs csf-v11/150+151.

## Open risks

- AFRC is lossy by default (CU32): exact-decode tests (CTS compressed-texture
  cases, bc_verify) see 13-38 LSB on random blocks. PANVK_BC_AFRC=0 restores
  exact decode.
- Only G615 (v11) tested. v10 and v12-v14 take the same fragment path; AFRC
  only where pan_query_afrc() says so. v9 and 3D images use the old path.
- AFRC row-stride fix is in lib/ and also affects any other AFRC user
  (none in panvk before this).

Rejected / dropped:
- `force_native_bc` probe (sample BC with the real Mali BC format codes
  despite TEXTURE_FEATURES): dropped on request before it was run.
- RGB565 for BC1: DXVK maps DXGI BC1 to VK BC1_RGBA (alpha), and 565 has no
  sRGB variant, so it would almost never apply. Not implemented.
