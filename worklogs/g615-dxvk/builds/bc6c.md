# RC5-dev-bc6c build (2026-10-22): BC2/BC3 without spills, small mips in one dispatch (csf-v11 217-218)

bc6c = bc6b (csf-v11 through 212, with the amended 209) plus the tess patches
csf-v11 213-216 and the BC patches 217-218. It is one test build of the whole
series. **Not run on a device yet**: the numbers below are host checks
(correctness) and static Valhall v11 estimates (speed).

The coordinator asked for three things, based on the bc6 device run
(`device-runs/2026-10-22-g615-bc6/results.md`): fix the BC3 regression, make
BC7_SRGB as fast as BC7_UNORM, and close the gaps against RC4-lowmem on
BC1_RGB, BC2, BC4U, BC5U/S and BC3_SRGB.

## Findings

### BC3 "regression" (+24%, 4.07 vs 3.29 ms): not caused by 212

- spvstat shows that 212 did not change the BC2/BC3 colour path. The
  palette fit (bit 0x1000000) is only used for BC1. BC3 is 54.72 -> 54.78
  cycles, with 18 -> 15 spills, and its output is bit-identical. BC2 is
  78.19 -> 78.28 cycles.
- On the driver side, the only BC3 change from bc5 to bc6 is the pipeline
  key (the `shader` field). The dispatch is the same.
- The device numbers match session noise:
  - The bc6 cache-off run gives BC3 2.75 ms, which equals bc5's 2.81 ms.
    bc5's runs were really cache-off (see the PanProbe buildEnv() bug).
  - REF_RGBA16F, a plain native copy with no BC code, shows the same
    pattern: 4.18 ms with the cache on, 2.77 ms with it off, 3.40 ms on
    RC4.
- Even so, BC2/BC3 colour still spilled. 217 makes it cheaper (see below).

### BC7_SRGB (5.58) vs BC7_UNORM (4.71): same GPU work

- Both formats use the same shader: `bc_decode_shader_index()` is 3, with
  the fast FS. The pipeline key is (shader, bc_format), and bc_format is
  R8G8B8A8_UNORM for both.
- The shadow (AFRC CU 16, `bc8` covers both formats), the raw fuse, the
  bc_perf content (same rng seed) and the render pass are also the same.
- The fast decoder has no sRGB conversion. `is_srgb` only exists on the
  Granite path, where it is unused.
- The gap comes from the samples: UNORM was 5.07 / 3.77 / 5.29 and SRGB
  4.98 / 5.96 / 5.80, so one 3.77 outlier pulls the UNORM mean down. In
  bc4 they were 6.37 / 6.32, in bc5 6.01 / 6.30, and on RC4 6.16 / 6.00.
- There is nothing to change in the driver. To check this on the device,
  compare medians over more runs, or run a single format with
  `bc-perf <icd> BC7_SRGB`.

## Patches

| patch | mesa-bc6c commit | what | toggle back |
|---|---|---|---|
| 217 `transcode-bc2-bc3-colour-through-the-bc1-palette-fit` | 62428afd | BC2/BC3 colour (always 4-colour) through the spill-free palette fit of 212. New PANVK_BC_LM_OPTS bit 0x2000000; the default is now 0x3020000. | `PANVK_BC_LM_OPTS=0x1020000` (bc6 encoder) |
| 218 `transcode-the-small-levels-of-a-bc-upload-in-one-dispatch` | 5c9e232f | ETC2/EAC uploads (BC1-BC5). The whole-level, single-layer regions with <= 4096 blocks run as one dispatch of a new XC_BATCH specialization (spec constant 4), whose level table is in cmdbuf memory. A 2048² chain now needs 4 dispatches instead of 12. | `PANVK_BC_LM_BATCH=0`; any other value is the block limit |

Dev branch: `bc6c` in `/home/flux/src/mesa-bc` (worktree
`/home/flux/src/mesa-bc6c`), on top of ad60b09a (212).

**Strict apply:** a fresh worktree `/home/flux/src/mesa-bc6c-strict` at
5a07217f, then `scripts/apply-patches.sh --profile g615-v11-csf`, gives
`OK applied=213` with no offset or fuzz. jm-v9 and jm-v7 apply on top.

The build includes the tess patches 213-216, with these toggles:

- `PANVK_TESS_FUSE` (213, fused count/scan/draw; there is also
  `PANVK_TESS_FUSE_GEN_MAX`)
- `PANVK_TESS_SETUP_STEP` (214)
- `PANVK_TESS_ARENAS` (215)
- `PANVK_TESS_WINDING_INLINE` (216)

All of these strings are in the .so.

## Ideas not done

- **BC1 under 32 registers.** The palette fit is at 59 registers and is
  CVT-bound: 44 of its 47.7 cycles are on the CVT pipe, mainly shifts,
  masks and CSELs. The fitted upload model is linear in static cycles, so
  the transcode is ALU-bound, and doubling occupancy would mostly hide
  latency that is not the bottleneck. Getting from 59 to 32 or fewer
  registers would need a rewrite of the fit, which bc5.md estimated at
  about -6 dB when done by cutting options. Not attempted.
- **Per-level barriers.** There are none to drop. The xcode upload is a
  single meta_compute_start/end with back-to-back dispatches.
- **Per-dispatch push and descriptor setup.** 218 removes 8 of the 12
  pipeline binds, push uploads and RUN_COMPUTE syncs per chain.
- **BC7.** Each level is a different attachment, so the levels cannot be
  batched. This is unchanged.

## Correctness (host)

- **217:** tools/bc-bench at 1024², bc4 (0x20000) -> bc6c (0x3020000),
  PSNR random / gradient:
  - BC2: 25.450 / 41.444 -> 25.433 / 41.448 dB
  - BC3: 25.914 / 41.582 -> 25.879 / 41.585 dB
  - BC1 / BC1A are unchanged from bc6 (20.079 / 40.446 and 20.911 /
    40.360).

  Every row is within 0.04 dB of bc4, so well inside the 1 dB limit.
- **218:** the shader runs as C++ (tools/bc-glsl-host glsl2cpp.py, with
  harness `/tmp/.../scratchpad/gh/test_batch.cpp`). On a 100x60 chain
  (blocks 25x15, 13x8, 7x4, 3x2, 2x1, 1x1, 1x1), level 0 runs per-region
  and levels 1-6 run batched with 8x8 workgroups. The shadow and raw
  bytes are identical to the per-level dispatches for BC1, BC1A and BC2:
  PASS.
- **Driver:** panvk_vX_cmd_meta.c (v11) compiled with no warnings before
  the full build.
- **bc_decode:** this is the transcode output, so it needs the device
  run. 217 changes the BC2/BC3 bitstream by design (a different ETC2
  colour fit), so it does not need to be bit-exact against bc6.

## spvstat (Valhall v11), bc6 (0x1020000) -> bc6c (0x3020000)

Cycles, registers and spills. These are per-region pipelines (XC_BATCH=0),
which do the 98% of the blocks in level 0-2.

| mode | bc6 | bc6c |
|---|---|---|
| BC1 RGB | 47.69 / 59 / 0 | 47.69 / 59 / 0 (unchanged) |
| BC1 RGBA | 48.50 / 59 / 0 | 48.50 / 59 / 0 (unchanged) |
| BC2 | 78.28 / 59 / 15 | **74.94 / 59 / 0** |
| BC3 | 54.78 / 59 / 15 | **51.41 / 59 / 0** |
| BC4 U/S | 10.13 / 16 / 0 | 10.13 / 16 / 0 (unchanged) |
| BC5 U/S | 14.47 / 19 / 0 | 14.47 / 19 / 0 (unchanged) |

The batch pipeline (XC_BATCH=1, used for small levels only) costs about
3.8 extra cycles per block:

| mode | XC_BATCH=1 |
|---|---|
| BC1 | 51.44 / 57 / 0 |
| BC3 | 55.16 / 57 / 0 |
| BC4 | 13.88 / 22 / 0 |
| BC5S | 18.22 / 22 / 0 |

`tools/spvstat/spvstat` now takes an optional 6th argument for spec
constant 4.

## Projected upload_gpu_ms (not measured)

**Model.** A least-squares fit to the bc6 cache-on means of the 9 BC1-BC5
rows gives upload ≈ 0.965 ms + 0.0325 ms × cycles + 0.656 ms if the
format has 16-byte blocks (BC2/3/5). The RMS error is about 0.3 ms, which
is the session noise.

- 217 saves 0.0325 × 3.4 ≈ 0.11 ms on BC2/BC3, plus the spill traffic,
  which the static model does not see.
- 218 saves 8 dispatches per chain. The per-dispatch cost has not been
  measured; at an assumed 15-30 µs each, that is 0.12-0.24 ms on every
  BC1-BC5 format.

**Estimates.** The bc6 column is the device mean. The bc6c column is that
mean minus the savings above (and, for BC3, also the bc5 cache-off
baseline).

| format | RC4 | bc6 | bc6c projected | vs RC4 |
|---|---|---|---|---|
| BC1_RGB | 3.09 | 2.74 | 2.50-2.62 | -15..-19% |
| BC1_RGBA_SRGB | 2.97 | 2.34 | 2.10-2.22 | -25..-29% |
| BC2 | 4.63 | 3.88 | 3.53-3.65 | -21..-24% |
| BC3_UNORM | 3.29 | 4.07 (cache-off 2.75, bc5 2.81) | 2.46-2.58 (from the 2.75-2.81 baseline); model 3.05-3.17 | -4..-25%, **borderline under the model** |
| BC3_SRGB | 3.10 | 3.18 | same as BC3 | likely faster, borderline |
| BC4_UNORM | 1.58 | 1.37 | 1.13-1.25 | -21..-28% |
| BC4_SNORM | 1.62 | 1.20 | 0.96-1.08 | -33..-41% |
| BC5_UNORM | 2.38 | 2.25 | 2.01-2.13 | -11..-16% |
| BC5_SNORM | 2.05 | 1.78 | 1.54-1.66 | -19..-25% |
| BC6H UF/SF | 8.41 / 9.66 | 3.75 / 4.10 | unchanged | -55% / -58% |
| BC7 UNORM/SRGB | 6.16 / 6.00 | 4.71 / 5.58 | unchanged (same work; expect equal) | about -15..-24% |

**Target.** The target is "clearly faster than RC4 on every format",
against a run-to-run spread of about 1 ms. The projections meet it for
BC1, BC2 and BC4, and for BC5S and BC6H. BC3 depends on whether bc6's
4.07 ms was real; the static evidence says it was not. BC5U stays the
closest.

**What to measure on the device:**

- `PANVK_DEBUG=bc_timing` gives per-operation GPU times.
- An A/B with `PANVK_BC_LM_BATCH=0` measures 218 directly.
- An A/B with `PANVK_BC_LM_OPTS=0x1020000` isolates 217.

## Build

- Thermal was NONE, so 4 jobs. The command:

  ```
  PANVK_KBASE_RELEASE=RC5-dev-bc6c PANVK_JOBS=4 MESA=/home/flux/src/mesa-bc6c-strict
  BDIR=/home/flux/build-bc6c/bionic-r30-RC5-dev-bc6c DDIR=/home/flux/build-out/2026-10-22-bc6c
  HOST_TOOLS=/home/flux/build-cache/host-tools/5a07217f... flock /home/flux/build-cache/build.lock
  sh scripts/build-android.sh --profile g615-v11-csf --api 29 --ndk /opt/android-sdk/ndk/30.0.16248370
  ```

  It printed `OK profile=g615-v11-csf`.
- **Driver:** `/home/flux/build-out/2026-10-22-bc6c/libvulkan_panfrost.so`,
  29331408 bytes, sha256
  `d4cb414a968fffe2c6f85763478b4871c0d45d36197c29cc61e06cabfbd45862`.
  - Its version string is `PanVK-kbase RC5-dev-bc6c (Mesa 26.3.0-devel (git-5a07217f03))`.
  - It contains PANVK_BC_LM_BATCH and the PANVK_TESS_* toggles.
- **APK:** built under the build lock with:

  ```
  ./gradlew assembleDebug -PpanvkSoRc4=/home/flux/build-out/rc4-lowmem/libvulkan_panfrost-android-aarch64.so
  -PpanvkSoRc5=<so above> -PpanprobeSuffix=bc6c
  ```

  - The first tries failed in `configureCMakeDebug` with a NoSuchFileException
    on `.cxx/.../compile_commands.json`. The cause was a stale `.cxx` after
    tess_perf was added to CMakeLists. Removing `app/.cxx` fixed it.
  - Output: `/home/flux/build-out/apks/panprobe-bc6c.apk`, sha256
    `7cba9d2c2372a9ec975bc61ddb4c22bc3775f1a1d67079bc33ec98763120c0a3`.
  - aapt2: `dev.zenithblue.panvktest.bc6c`, versionCode 15, versionName 1.2.7.
  - The .so files inside both match: `libvulkan_panfrost_rc5.so` is
    d4cb414a... and `libvulkan_panfrost_rc4.so` is c395db09... (RC4-lowmem).
  - The APK includes tess_perf (6162649).

## Device plan (G615)

1. Run `--es driver rc5 --es autorun all`. The target is 38/38 with
   bc_decode PASS, which also covers 217's new BC2/BC3 bitstream and 218.
   Tess tests run with 213-216 on by default.
2. Run bc_perf with rc4 and rc5 interleaved (at least 3 each) and compare
   medians. Optionally add A/B runs with `--es env PANVK_BC_LM_BATCH=0`
   and `PANVK_BC_LM_OPTS=0x1020000`.
3. If bc_decode fails:
   - `PANVK_BC_LM_BATCH=0` rules out 218.
   - `PANVK_BC_LM_OPTS=0x1020000` rules out 217.
   - The tess toggles rule out tess regressions (see the tess perf review).
