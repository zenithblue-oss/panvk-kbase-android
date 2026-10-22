# RC5-dev-bc6 build (2026-10-22): faster BC1 / BC6H / BC7 uploads (csf-v11 210-212)

bc6 = bc5 (csf-v11 through 209) plus 210, 211 and 212. The goal is BC1, BC6H
and BC7 uploads strictly faster than RC4-lowmem on G615, with a 20% margin
or more if possible. **Not run on a device yet**: every number below is
host-verified (correctness) or a static Valhall v11 estimate (speed).

Same-session upload_gpu_ms baselines, RC4 / bc4: BC1 RGB 2.63 / 2.63,
BC1 SRGB 2.91 / 2.80, BC6H UF 8.34 / 8.57, BC6H SF 9.98 / 9.77,
BC7 6.12 / 5.88 (RC4) vs 6.37 / 6.32 (bc4).

## Patches

| patch | mesa-bc commit | what | toggle back to bc5 |
|---|---|---|---|
| 210 `decode-bc6h-once-per-block-into-the-shadow` | d323f60d | BC6H UF/SF: one compute invocation per 4x4 block, written straight into the u-interleaved B10G11R11 / RGBA16F shadow (new bc_lowmem 4, xcode plumbing). The raw block is stored by the same dispatch. | `PANVK_BC_BC6H_DECODE=fragment` |
| 211 `decode-bc7-with-constant-bit-offsets` | 8683b927 | BC7 fragment decode (AFRC shadow, so it still needs a render pass) with constant-offset fields per mode, arithmetic weights and fp32 interpolation. The raw block is written by the decode FS, so there is no separate raw-copy pass. | `PANVK_BC_BC7_DECODE=granite` |
| 212 `transcode-bc1-through-a-palette-fit` | ad60b09a | BC1 RGB / RGBA -> ETC2: palette fit on the luma axis (after the fork's csf-v11/214), packed palette, output address before the transcode. No spills. | `PANVK_BC_LM_OPTS=0x20000` (bc4/bc5 encoder; the new default is 0x1020000) |

`patches/csf-v11/210-212`: fresh worktree at 5a07217f plus
`scripts/apply-patches.sh --profile g615-v11-csf` gives `OK applied=207`,
with no drift (jm-v9 / jm-v7 still apply on top). 211 keeps its new
`#include`s in the late include block of `panvk_vX_cmd_meta.c`, because
jm-v9/011 touches the top of the file.

## Plan items

1. **Per-block decode.**
   - BC6H: done as compute (210). The shadow is u-interleaved for xcoded
     images, so a storage write can produce it.
   - BC7: the AFRC shadow can only be written by a render pass. A compute
     staging pass would add an RGBA8 staging write plus a blit, which costs
     about REF_RGBA8 (~2.5 ms) on top. So the fragment decoder stayed, and
     its per-texel cost was cut by 2-3x instead (211).
2. **BC1 without spills**: 212.
3. **Per-dispatch fixed cost.**
   - BC6H's per-mip render passes are now dispatches with no render pass
     setup.
   - BC6H and BC7 no longer have the separate raw-copy pass
     (copy_memory_to_image_raw, ~0.65 ms at REF_ETC2 size).
   - BC7's small mips cannot share a render pass, because each level is a
     different attachment.
   - Batching BC1's small levels into one dispatch was not done.

## Correctness (host, tools/bc-glsl-host)

The shaders are compiled as C++ with `glsl2cpp.py` and `glsl_shim.hpp`
(`make MESA=<tree> O=<out> run`).

- **test_bc6** (320k texels, all modes):
  - SF block decode is bit-exact to the per-texel decoder.
  - UF is the RNE-saturated B10G11R11 of the per-texel half (max relative
    error 0.0154, inside BC_BC6U_TOL 0.0157). The fragment path also rounds
    to B10G11R11, so BC6H exact stays exact.
  - The per-texel decoder equals Mesa's CPU BPTC decoder.
  - The u-tile addresses match pan_tiling.
- **test_bc7** (200k blocks of every mode plus invalid modes, 3.2M
  texels): the fast decoder is bit-identical to Granite, and Granite equals
  Mesa BPTC. So BC7 -> AFRC is bit-exact to before.
- **BC1** (subagent host C build, 1M random plus structured blocks):
  - bc-bench PSNR at 1024^2 vs 0x20000 (random / gradient): BC1 -0.11 /
    0.00 dB, BC1A -0.00 / -0.00 dB. This is within the 1 dB limit.
  - BC2-BC5 output is bit-identical at the new default. Every mode is
    bit-identical at 0x20000 and 0xc30000.
- **SPIR-V headers**: the existing headers (bc7 compute and FS, bc6, etc2
  except 212's) are unchanged apart from glslang's version comment. Only
  the new `panvk_bc_bc6_blk_spv.h` and `panvk_bc_bc7_fs_fast_spv.h` are
  added.

## spvstat (Valhall v11), before -> after

**BC6H**, cycles per 256 texels with the mode fixed:

| | before | after (210) |
|---|---|---|
| decoder | per-texel: 3870 instrs, 120.9 static cycles, 30 regs | block: UF 110.2 static / 52 regs / 0 spills; SF 142.1 / 43 regs |
| bc_perf mode 11 | 12.25 x 16 = 196 | UF 34.6, SF ~38-39.5 |
| any mode (range) | 136-229 | UF 34.6-48.8, SF 37.9-55.0 |

**BC7**, cycles per warp of 16 texels (Granite -> fast):

| mode | Granite | fast |
|---|---|---|
| 0 | 15.5 | 6.16 |
| 1 | 14.3 | 4.97 |
| 2 | 13.6 | 4.97 |
| 3 | 13.9 | 4.81 |
| 4 | 9.2 | 5.38 |
| 5 | 8.5 | 4.72 |
| 6 (bc_perf) | 6.8 | 4.13 |
| 7 | 16.8 | 5.78 |
| static, all modes | 2425 instrs, 75.4 cycles, 30 regs | 1163 instrs, 30.7 cycles, 26 regs, 2 threads, 0 spills |

**BC1** (etc2 transcoder), cycles / regs / spills:

| mode | 0x20000 | 0x1020000 |
|---|---|---|
| BC1 | 50.78 / 59 / 14 | 47.69 / 59 / 0 |
| BC1A | 50.97 / 59 / 14 | 48.50 / 59 / 0 |
| BC2 | 78.19 / 57 / 18 | 78.28 / 59 / 15 |
| BC3 | 54.72 / 57 / 18 | 54.78 / 59 / 15 |

## Projected upload_gpu_ms (not measured)

The model is fitted to bc4: time ≈ 3.57 ms + 0.408 ms x cycles per warp,
and the raw-copy pass is ~0.65 ms.

| format | RC4 | bc6 projection | margin |
|---|---|---|---|
| BC6H UF | 8.34 | 2.5-4 ms | over 50% |
| BC6H SF | 9.98 | 3-5 ms | over 50% |
| BC7 | 5.88-6.12 | ~4.6 ms | ~22-25%, borderline |
| BC1 RGB / SRGB | 2.63 / 2.91 | ALU -6% and no spill traffic | likely faster, **20% not expected** |

BC1's time is mostly fixed cost, since REF_ETC2 alone is ~0.7 ms. Without
the scratch traffic of the 14 spills it should be clearly faster, but a
20% margin needs the small-level dispatch batching, which was not done.

## Build

- **Command:** `PANVK_KBASE_RELEASE=RC5-dev-bc6 PANVK_JOBS=4
  MESA=/home/flux/src/mesa-bc6-strict BDIR=/home/flux/build-bc6/bionic-r30-RC5-dev-bc6
  DDIR=/home/flux/build-out/2026-10-22-bc6 ... flock /home/flux/build-cache/build.lock
  sh scripts/build-android.sh --profile g615-v11-csf --api 29 --ndk /opt/android-sdk/ndk/30.0.16248370`.
  Thermal was NONE, so 4 jobs.
- **Driver .so:** `/home/flux/build-out/2026-10-22-bc6/libvulkan_panfrost.so`,
  27932168 bytes, sha256
  `24b5f17896cc4e8dd69b43439e31d5687b58aeab6409584546e28ca679f040c6`.
  It contains RC5-dev-bc6, PANVK_BC_BC6H_DECODE and PANVK_BC_BC7_DECODE.
  An earlier check build of 206 patches (without 212) also exited 0.
- **APK:** built with `./gradlew assembleDebug -PpanvkSoRc4=/home/flux/build-out/rc4-lowmem/libvulkan_panfrost-android-aarch64.so
  -PpanvkSoRc5=<so above> -PpanprobeSuffix=bc6`, under the build lock.
  - Output: `/home/flux/build-out/apks/panprobe-bc6.apk`, sha256
    `ae346e2dc6a608d9f1bae64b96e9b4a76b95decba3e0a8bac4399bd2325c3325`.
  - aapt2: `dev.zenithblue.panvktest.bc6`, versionCode 15, versionName 1.2.7.
  - The rc5 .so inside is 24b5f178... (matches). The rc4 .so is c395db09...
    (RC4-lowmem, matches).
  - The working tree had another agent's uncommitted MainActivity.kt
    change, so it is in the APK. It collapses duplicate env keys so that
    `--es env` overrides the defaults (last value wins).

## Device plan (G615)

1. Run `--es driver rc5 --es autorun all`. The target is 38/38 with
   bc_decode PASS.
2. Run bc_perf on rc5 and rc4 in the same session. The targets are BC6H
   and BC7 at least 20% below RC4, and BC1 below RC4.
3. If something fails or regresses, bisect with the toggles:
   - `PANVK_BC_BC6H_DECODE=fragment`
   - `PANVK_BC_BC7_DECODE=granite`
   - `PANVK_BC_LM_OPTS=0x20000`
4. Check memory: BC6H keeps the same shadow size, and only its modifier is
   now u-interleaved instead of AFBC. Compare the bc_perf / heap numbers
   with bc5.
