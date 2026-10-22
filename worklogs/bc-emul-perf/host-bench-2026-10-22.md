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
