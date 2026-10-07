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

## Limits pass (2026-10-07, csf-v11/180)

Question: can BCn get smaller or faster, or are we at the limit of the
G615 feature set? Tools: `device/bc-perf.c` gained `XC_*` entries
(BCPERF_XC=1: BC encode, CPU decode, CPU re-encode to EAC R11 / EAC RG11 /
ETC2 RGB8, sampled natively), `--quality` (host-only transcode PSNR),
`BCPERF_EACENC=1` (GPU EAC R11 encoder `bc-perf-eac.frag`, one fragment
per block, timed after BC4_UNORM) and a BC1_RGBA punch-through encoder.
Scripts: `/var/tmp/panvk/xc/{build,run,qdump,verify,apk}.sh`, results
`/var/tmp/panvk/xc/`. Device 192.168.1.34:37885 under device.lock.

### Native references (G615, 2048x2048 full chain, no raw plane)

| format | mem MiB | E ms | note |
|---|---|---|---|
| R8 (AFBC) | 5.8 | 0.250 | tap floor (D = 0.242-0.25 for every format) |
| RG8 (AFBC) | 11.2 | 0.250 | |
| RGBA8 (AFBC) | 22.1 | 0.355 | |
| B10G11R11 (AFBC) | 22.1 | 0.375 | |
| RGBA16F (AFBC) | 43.2 | 0.91 | |
| EAC R11 | 3.2 | 0.246 | |
| EAC RG11 | 5.8 | 0.363 | |
| ETC2 RGB8 | 3.2 | 0.366 | |
| ETC2 RGBA8 | 5.8 | 0.368 | |
| ASTC 4x4 LDR | 5.8 | 0.84 | random payload |

Only EAC R11 samples at the tap floor. ETC2/EAC RG11 are 1.5x and ASTC
3.4x slower than the AFRC/AFBC shadows in bandwidth-bound sampling.

### Transcode prototypes (BC -> native at upload)

Quality: CPU encoders with wide searches (upper bound for a GPU encoder),
decoded by the GPU (LOD0 dumps) and by the CPU (`--quality`; both agree
to 0.3 dB, so the bitstreams are right). 1024x1024 crops of a MiSide frame
/ the dxcube frame.

| transcode | vs exact BC decode | vs source (BC alone) | E ms | chain MiB (raw + native) |
|---|---|---|---|---|
| BC4 -> EAC R11 | 51.1 / 54.7 dB, max 12 | 43.79 / 46.41 (44.55 / 47.05) | 0.246 | 8.5 -> 5.9 |
| BC5 -> EAC RG11 | 51.3 / 54.7 dB, max 12 | 44.09 / 46.43 (44.87 / 47.06) | 0.363 | 16.6 -> 11.1 |
| BC1 -> ETC2 RGB8 | 40.2 / 45.1 dB, max 53-56 | 34.03 / 35.91 (35.03 / 35.75) | 0.366 | 10.7 -> 5.9 |

Upload cost, GPU EAC R11 encoder (same search as the CPU one), 2048x2048
LOD0: 891 ms (52.0 dB vs BC4), fast variant (one multiplier, base +-1)
126 ms (51.5 dB). Whole chain x4/3: 168-1190 ms, against 3.1-3.6 ms for
the fragment BC4 decode of the whole chain (50-380x). The ETC2 CPU
encoder costs 585-940 us per block, 15-80x the EAC one.

Verdict: rejected. BC4 -> EAC R11 saves 2.6 MiB per 2048^2 chain at equal
speed, but loses 0.6-0.7 dB against the source (the class of the rejected
4 bpc R8 AFRC, -0.9 dB) and costs 40x+ the decode on upload. BC5 and BC1
transcodes are 1.5x slower to sample and BC1 -> ETC2 loses 1 dB. BC7/BC3
-> ASTC 4x4 or ETC2 RGBA8 (8 bpp) would land at the same size as AFRC
CU16 (8 bpp, -0.5 dB for BC7, no encoder) and sample 1.5-3.4x slower: not
prototyped. BC6H -> ASTC HDR: no negative values (SF), HDR encoder, ASTC
sampling 0.84 ms vs 0.33: not prototyped.

### AFRC rate per format (csf-v11/180, done)

Second picture with alpha (`src-mixa`: MiSide RGB + dxcube luma as alpha),
exact vs AFRC decode, PSNR vs the source:

| format | exact | CU32 | CU24 | CU16 |
|---|---|---|---|---|
| BC1 colour | 35.03 | 35.04 | 35.05 | 34.95 |
| BC2 alpha | 31.35 | 31.35 | 31.33 | |
| BC3 alpha | 47.14 | 47.05 | 46.44 (max 46 LSB) | |
| BC7 colour | 38.75 | 38.73 | 38.65 | 38.21 |
| BC4 (R8) | 44.55 | 43.61 | 41.42 | 37.15 |
| BC5 (RG8) | 44.87 | 43.72 | 41.34 | 36.89 |

BC1_RGBA punch-through (3-colour mode, 27% transparent texels), vs exact:
alpha max 34 LSB at CU32, 70 at CU24; opaque colour 52.7 / 46.6 dB; no
texel crosses the 0.5 alpha-test threshold at either rate.

csf-v11/180: BC1/BC2 shadows default to CU24 (12 bpp); BC3/BC7 keep CU32
(BC3 alpha -0.7 dB at 24, BC7 is the high-quality format); R8/RG8 stay
uncompressed (-0.9 to -1.1 dB at the lowest-loss rate). PANVK_BC_AFRC
still overrides all. Sampling cost does not depend on the rate.

### Raw plane and other memory

- DXVK always sets TRANSFER_SRC|TRANSFER_DST on textures
  (`d3d11_texture.cpp:38`), so dropping the raw plane for images without
  TRANSFER_SRC gains nothing for DXVK. UMA: no cheaper memory to move it
  to. Lazy/sparse: every upload writes it. Kept. Share of the chain after
  180: BC1 25%, BC3/BC7 33%, BC4 31%, BC5 32%, BC6H UF 19%, SF 11%.
- AFBC header + alignment overhead: BC4 +0.5 MiB (6%), BC5 +0.6 (4%), BC6H
  UF +0.7 (3%); AFRC shadows ~0. Not worth losing AFBC bandwidth for.
- BC6H SF: RGBA16F has no AFRC mode (AFRC covers equal-width <= 12-bit
  channels; B10G11R11 is mixed-width too) and ASTC HDR has no negative
  values. 48.5 MiB is the floor on this feature set.

### Speed and upload

- A/B/C (cache friendly): 0.48-0.49 ms for every format, emulated or
  native: filter-rate bound.
- D/E (bandwidth bound): BC1/2/3/4/5/7 at 0.244-0.26 ms = the tap floor
  (native R8 0.25). BC6H UF 0.33 (native B10G11R11 0.375), BC6H SF 0.72
  (native RGBA16F 0.91). Nothing left on this hardware.
- Upload: fragment decode 3-6.8 ms per 2048^2 chain vs plain RGBA8 copy
  2.4-3.7 ms vs native ETC2 1.0 ms.
- NFS (nfs-23197 perf, 160 s): CPU time in BC paths 208 ms (0.11% of
  on-CPU time, 85% in the first 70 s): 112 ms recording decode passes
  (21 ms of it first-use meta pipeline compile), 70 ms
  `panvk_bc_shadow_info` -> `debug_get_num_option` -> Android property
  lookups (fixed by csf-v11/171's per-name cache), 34 ms barrier scan in
  `cmd_bc_decode_zero_initialized`. GPU decode time is not in the CPU
  profile.

### Final (G615, csf-v11/150 + 151 + 180)

| format | mem MiB 151 -> 180 | E ms | best measured alternative (rejected) | native-format reference |
|---|---|---|---|---|
| BC1 | 13.3 -> 10.7 | 0.245-0.255 | CU16 8.0 (opt-in) / ETC2 xcode 5.9 | ETC2 RGB8 3.2, E 0.366 |
| BC2 | 16.0 -> 13.3 | 0.253 | | |
| BC3 | 16.0 | 0.26 | CU24 13.3 (alpha -0.7 dB) | ETC2 RGBA8 5.8, E 0.368 |
| BC4 | 8.5 | 0.244 | EAC R11 xcode 5.9 | EAC R11 3.2, E 0.246 |
| BC5 | 16.6 | 0.25 | RG8 AFRC 10.7 / EAC RG11 xcode 11.1 | EAC RG11 5.8, E 0.363 |
| BC6H UF | 27.4 | 0.33 | (ASTC HDR, not prototyped) | ASTC 5.8, E 0.84 |
| BC6H SF | 48.5 | 0.72 | none | RGBA16F 43.2, E 0.91 |
| BC7 | 16.0 | 0.26 | CU24 13.3 (-0.1 dB) | ASTC 4x4 5.8, E 0.84 |

Proof: bc_decode + bc_verify (device binary, PANVK_BC_AFRC=0,
BC_BC6U_TOL=0.0157) 16/16, BC_DEVICE_FAILS=0. panvk-test APK
(`dev.zenithblue.panvktest.bcperf`, bundled dist-xc180, BuildID
`4be67989`, series 001-154 + 180): PanProbe autorun all 37/37, bc_decode
BC_DEVICE_FAILS=0, bc_perf BC1 10932 KiB / BC2 13664 KiB. Default-env
bc_verify fails the 2 LSB tolerance on the RGBA8 formats (AFRC on random
blocks, BC1/BC2 max 54-70 LSB now), as expected.
