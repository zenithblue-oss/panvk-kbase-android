# Host bench: lowmem vs bc2 BC -> ETC2/EAC transcoder (2026-10-22)

Harness: `tools/bc-bench/` (`make OLD=/home/flux/src/mesa-lowmem NEW=/home/flux/src/mesa-bc`,
`./bc_bench old|new [reps=5] [dim=2048]`). It compiles the `panvk_bc_etc2.comp`
body of each tree as C (GLSL/C subset shims from `tests/bc-host`), gcc 15 -O2,
aarch64 PRoot, single thread. Input: 2048x2048 texels = 262144 blocks per
format; "random" = random block words, "gradient" = blocks encoded from the
synthetic pictures in `tests/bc-host/pics.h`. Speed = median of 5 passes.
Quality = PSNR (8-bit scale) and max abs error of the ETC2/EAC decode vs the
exact BC decode.

## What is and is not CPU code

- The transcoders are GPU compute shaders (the shader text is C-compatible,
  so only the algorithm's ALU work is measured; host time is a proxy, not GPU time).
- "lowmem" = `/home/flux/src/mesa-lowmem` `panvk_bc_etc2.comp`, which is the
  csf-v11/200 per-texel search encoder (identical to
  `tests/bc-host/etc2_200.h`). The real RC4-lowmem kernels are precompiled
  Mali binaries with no source, so they cannot be run here; this is the
  closest source baseline.
- BC7 and BC6H-exact are fragment/compute GPU decodes with no CPU
  counterpart: not benchable on the host. Default BC6H is `exact` in bc2
  (no transcode). Opt-in `PANVK_BC_BC6H=astc` encoder (csf-v11/202,
  unchanged between trees): 3.32 us/block host (`tests/bc-host/test_astc`),
  0 bitstream mismatches vs astcenc; quality 52-64 dB tonemapped PSNR on
  real-looking HDR content (see lowmem-v2.md).
- The HDR bit, spill to app cache (201) are driver-side GPU/BO/file code:
  not host-benchable here.

## Results (ETC2/EAC encoder, 2048x2048)

| format | input | lowmem Mblk/s | bc2 Mblk/s | speedup | lowmem PSNR | bc2 PSNR | lowmem / bc2 maxerr |
|---|---|---|---|---|---|---|---|
| BC1 | random | 0.377 | 2.482 | 6.6x | 20.64 | 20.20 | 239 / 239 |
| BC1 | gradient | 1.132 | 3.031 | 2.7x | 41.44 | 41.17 | 73 / 73 |
| BC1A | random | 0.454 | 3.172 | 7.0x | 20.99 | 20.89 | 255 / 240 |
| BC1A | gradient | 1.136 | 3.163 | 2.8x | 41.34 | 41.07 | 73 / 73 |
| BC2 | random | 0.216 | 1.634 | 7.6x | 25.60 | 25.44 | 84 / 117 |
| BC2 | gradient | 0.635 | 1.907 | 3.0x | 42.58 | 42.11 | 73 / 73 |
| BC3 | random | 0.235 | 1.670 | 7.1x | 25.91 | 25.82 | 90 / 90 |
| BC3 | gradient | 0.468 | 1.966 | 4.2x | 42.57 | 42.29 | 73 / 73 |
| BC4U | random | 0.578 | 4.658 | 8.1x | 35.62 | 33.26 | 37.6 / 28.8 |
| BC4U | gradient | 0.735 | 5.022 | 6.8x | 52.41 | 52.11 | 7.0 / 11.3 |
| BC4S | random | 0.548 | 4.540 | 8.3x | 35.59 | 33.22 | 37.1 / 28.8 |
| BC4S | gradient | 0.736 | 5.108 | 6.9x | 43.39 | 40.22 | 29.8 / 28.7 |
| BC5U | random | 0.275 | 2.333 | 8.5x | 35.60 | 33.25 | 37.7 / 28.8 |
| BC5U | gradient | 0.376 | 2.583 | 6.9x | 52.85 | 51.87 | 7.6 / 11.3 |
| BC5S | random | 0.271 | 2.289 | 8.4x | 35.59 | 33.22 | 36.9 / 28.8 |
| BC5S | gradient | 0.366 | 2.488 | 6.8x | 43.42 | 40.26 | 29.5 / 28.8 |
| BC6H | any | n/a (GPU decode) | ASTC opt-in: 3.32 us/block | | | 52-64 dB | |
| BC7 | any | n/a (GPU decode) | n/a | | | | |

Peak RSS (`/usr/bin/time -v`, one process per version, includes the
2x4 MiB block buffers): lowmem 10100 KB, bc2 10100 KB (getrusage 9632 KB
both). The encoders are stateless: static const LUTs (EAC_TM_LUT 4.5 KB,
EAC_NEAR 2 KB) and ~40 scalar globals; no per-call allocation, so no host
memory difference exists. bc2's real memory win is driver side (201:
raw blocks leave the image memory; sizes in lowmem-v2.md), which needs the device.

## Conclusions

- Palette encoder: 2.7-8.5x faster in host ALU work than the lowmem
  per-texel encoder (largest on BC4/BC5 and random blocks; ~3x on
  gradient colour blocks). Whether that translates to GPU time needs
  `PANVK_DEBUG=bc_timing` on the device.
- Quality is not free: BC1-BC3 lose 0.1-0.5 dB, BC4/BC5 UNORM random
  2.4 dB, BC4/BC5 SNORM gradient 3.2 dB (BC4S gradient 43.4 -> 40.2). Max
  error is equal or better except BC2 random (117 vs 84) and BC4/5
  gradient (11.3 vs 7.0 for UNORM). Not bit-identical to lowmem.
- No patch added: no host-visible hotspot (no per-call LUT build or
  allocation; LUTs are static const), and any shader change would need the
  SPIR-V headers regenerated and the .so rebuilt for a device-only effect.
  The SNORM BC4/BC5 quality drop vs lowmem is the one thing worth
  investigating next.

## bc3 column (csf-v11/203, 2026-10-22)

Harness: `tools/bc-bench` now builds three versions in one binary and
interleaves them per pass (`make OLD=/home/flux/src/mesa-lowmem
NEW=/home/flux/src/mesa-bc BC2REV=d2f5472`, `./bc_bench [reps] [dim] [mode
mask] [opts]`): lowmem = mesa-lowmem, bc2 = mesa-bc at d2f5472 (202), bc3 =
mesa-bc with csf-v11/203. 2048x2048, 5 reps, opts 0x30000.

| format | input | lowmem Mblk/s | bc2 Mblk/s | bc3 Mblk/s | lowmem PSNR | bc2 PSNR | bc3 PSNR | bc3 maxerr |
|---|---|---|---|---|---|---|---|---|
| BC1 | random | 0.285 | 2.030 | 1.568 | 20.638 | 20.204 | 20.204 | 239 |
| BC1 | gradient | 0.893 | 2.565 | 2.066 | 41.442 | 41.170 | 41.170 | 73 |
| BC1A | random | 0.376 | 2.700 | 2.121 | 20.994 | 20.887 | 20.887 | 240 |
| BC1A | gradient | 0.940 | 2.697 | 2.217 | 41.335 | 41.071 | 41.071 | 73 |
| BC2 | random | 0.175 | 1.419 | 1.214 | 25.604 | 25.445 | 25.445 | 117 |
| BC2 | gradient | 0.511 | 1.707 | 1.500 | 42.577 | 42.110 | 42.110 | 73 |
| BC3 | random | 0.190 | 1.541 | 1.318 | 25.909 | 25.820 | 25.820 | 90 |
| BC3 | gradient | 0.425 | 1.839 | 1.582 | 42.568 | 42.293 | 42.293 | 73 |
| BC4U | random | 0.566 | 4.753 | 4.709 | 35.620 | 33.263 | 33.263 | 28.8 |
| BC4U | gradient | 0.733 | 4.955 | 4.972 | 52.405 | 52.115 | 52.115 | 11.3 |
| BC4S | random | 0.532 | 4.258 | 4.241 | 35.590 | 33.223 | 33.223 | 28.8 |
| BC4S | gradient | 0.725 | 4.765 | 4.752 | 43.394 | 40.223 | 40.223 | 28.7 |
| BC5U | random | 0.272 | 2.274 | 2.284 | 35.602 | 33.249 | 33.249 | 28.8 |
| BC5U | gradient | 0.342 | 2.323 | 2.374 | 52.847 | 51.867 | 51.867 | 11.3 |
| BC5S | random | 0.243 | 2.012 | 2.006 | 35.591 | 33.220 | 33.220 | 28.8 |
| BC5S | gradient | 0.318 | 2.144 | 2.129 | 43.422 | 40.263 | 40.263 | 28.8 |

bc3 output is bit-identical to bc2 (same PSNR and max error on every
row). It is 0.77-0.88x bc2 on the host for BC1-BC3, because 203 moves the
error sums from int to fp32, which is cheap on Mali and not on the host CPU.
**Host Mblk/s is not a GPU proxy**, as the next table shows.

### GPU cost: Valhall shader-db stats (new, `tools/spvstat`)

`tools/spvstat` compiles the SPIR-V the driver embeds, with the mode/opts
spec constants, through the host panfrost compiler for v11 (G615 gpu_id
0xb8a31030). The lowering is close to panvk's (spirv_to_nir,
pan_preprocess, nir_opt_large_constants + load_constant via the inline
pool, explicit IO, pan_postprocess, pan_shader_compile). It prints
`BIFROST_MESA_DEBUG=shaderdb`. These shaders have no loops, so the static
cycle count is the per-block cost (worst case over the branches).
Build it with `sh tools/spvstat/build.sh` and run `sh tools/spvstat/all.sh x.spv`.

| mode | bc2 cycles | bc3 cycles | bc3 fma/cvt/sfu | bc2 compile ms (host) | bc3 compile ms |
|---|---|---|---|---|---|
| BC1 RGB | 79.4 | 58.3 | 25.8 / 51.8 / 26.0 | 1195 | 861 |
| BC1 RGBA | 79.8 | 58.8 | 25.6 / 52.1 / 26.5 | 797 | 822 |
| BC2 | 107.0 | 85.8 | 29.0 / 78.3 / 29.9 | 1739 | 2489 |
| BC3 | 102.4 | 81.1 | 28.9 / 74.2 / 27.9 | 2198 | 2753 |
| BC4 U/S | 28.5 / 28.2 | 30.0 / 29.8 | 3.3 / 28.5 / 6.1 | 2622 / 1631 | 2243 / 2636 |
| BC5 U/S | 52.8 / 52.3 | 54.3 / 53.8 | 6.6 / 52.2 / 8.5 | 3579 / 4383 | 4916 / 3842 |

All modes use 53-59 registers, which gives 1 thread (half occupancy). The
bc2 rows are integer-bound: CVT carries the integer ALU, and SFU carries
the 32-bit IMULs (382 in BC1), while the FMA unit sits almost idle at 7 cycles.
bc3 moves the ETC1 and H squared-error sums to the FMA unit. BC4/BC5 gain
1.5 cycles from the fused raw store (a branch and one store). BC1-BC3 now
spill 17-18 values, but `ls` (37-76) stays under the ALU bound.

Fit to the G615 device data (RC5-dev-bc2, 2026-10-22 results): BC1 5.0 ms
at 79 cycles, BC4 3.0 ms at 28.5, BC3 6.2 ms at 102. That gives upload
≈ 1.9 ms fixed + 0.039 ms/cycle per 2048² chain. So the expected bc3 upload
is BC1 ~4.2, BC3 ~5.3, BC2 ~5.3 ms from the shader alone, before the
fused-copy saving on the fixed part. The fit is a model, not a measurement:
the G615 run must confirm it.

### The exact "HQ" encoder (reference only, not shipped)

This session also wrote an exact encoder. EAC: an exact nearest level, 4
LUT candidates, the least-squares base, then base ±1. ETC1: closed-form
tables with exact scoring of the tables that clamp. H: closed form plus an
exact check. Planar: exact error. It is in
`tools/bc-bench/hq/panvk_bc_etc2_hq.comp`, with its LUT generator in
`hq/gen_eac_lut_hq.c` (1552 keys x 4 candidates, BC2 keys).
At 1024x1024 it meets the original target. Every row is >= lowmem PSNR,
and every row is 2.1-3.2x faster than lowmem on the host. Examples:
BC4U random 35.86 vs 35.59 dB, BC4S gradient 44.62 vs 43.45, BC1 equal.
**On the GPU it is unusable**: BC1 581 cycles (494 fills), BC4 4325
cycles (4052 spills, ls-bound), BC5 8590. That is 7-150x bc2, and it would
be slower than lowmem on the device. Its quality wins need a GPU-shaped
design: real loops over the candidates, no full unroll, under 32 registers.
That is left for later. bc3 therefore keeps bc2's bitstream, and the
existing `PANVK_BC_LM_OPTS` knobs remain the quality opt-in: bits 20-21 =
EAC candidates - 1 (bc2 with 0x330000: BC4U random 35.20 dB at 2x the EAC
cost), plus 0x40000 both flips and 0x80000 all 8 tables.
