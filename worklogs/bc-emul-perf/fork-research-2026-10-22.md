# Fork research: GunaCharanTeja/panvk-kbase-android BCn (2026-10-22)

Research only: no device, no driver edits. The fork code was read from a
`--depth 1` clone of `main` (c52af8b6, 2026-10-16) and was not built or run.

## Which fork

`gh api repos/zenithblue-oss/panvk-kbase-android/forks` lists 10 forks. Only one
has its own BC work:

- **https://github.com/GunaCharanTeja/panvk-kbase-android** (26 stars, the
  only fork with stars and its own releases). It ships G615-only builds,
  "PanVK G615 1.1.x-FC", tested on an OPPO CPH2763 (MT6878, Dimensity 7300,
  **Mali-G615 MC2**).
  - Releases: `g615-1.1.5-FC` (2026-10-16, "BCn textures at native memory,
    faster texture loading") and `g615-1.1.4-FC`.
  - BC branches: `perf/tiled-bcn`, `perf/bc-alpha-fast-path`, `feat/eac-narrow`,
    `debug/bc-stats`.
- HAMsajalah, Tricky-02, Llerey, UWUWns, MVN123..., xodiosx, viniblopes,
  theycallmeboxy and JICA98 are mirrors of our own commits (abhay-byte
  "biblioklept" RC3, jm-v7 worklogs) with at most a README or CI commit on top.

Their own claims (README, CHANGELOG, 1.1.5-FC release notes):

- "Compressed game textures (BC1-BC7) now use the same memory as on a PC GPU":
  half of 1.1.4-FC, and 2.5x less for BC2/BC3.
- "Up to 7x faster conversion", "faster than a plain RGBA8 decode where measured".
- Patch 212 says that Mali decodes LDR ASTC to fp16 and **filters it at half
  rate** unless the view asks for 8-bit ("narrow") decode. 1.1.1-1.1.4-FC
  sampled every BC1/BC2/BC3/BC7 texture that way, as ASTC 4x4 at fp16.
  This is the most likely source of the "low memory but half speed"
  reputation. Upload costs on their MC2 also stay high for BC7/BC6H (22 / 28 ms
  per 2048² set), which shows up as streaming stutter. The fork does not
  claim "half speed" anywhere.

## How the fork does BC (1.1.5-FC = csf-v11 124-130, 172-180, 212-219)

**Where decoding happens:** on the GPU, in compute (plus a fragment pass on
the old AFRC path), **once at upload**. No BC decode happens at sample time.
The decoded/transcoded plane is the only thing the texture unit samples.
There is no CPU decode.

**Target formats** (`panvk_bc_decoded_format()`, patches 213/214/215):

| BC | stored as | bpp | 2048² chain | upload GPU ms (their MC2, random set) |
|---|---|---|---|---|
| BC1 RGB / RGBA | ETC2 RGB8 / RGB8A1 | 4 | 2.7 MiB | 4.95 / 5.13 |
| BC2, BC3 | ETC2 RGBA8 (EAC alpha) | 8 | 5.3 MiB | 7.3 / 8.4 |
| BC4 / BC5 (U+S) | EAC R11 / RG11 (signed EAC for SNORM) | 4 / 8 | 2.7 / 5.3 MiB | 2.7 / 3.7 |
| BC6H UF **and SF** | ASTC 4x4 HDR (SF negatives clamp to 0) | 8 | 5.3 MiB | 28 |
| BC7 | ASTC 4x4 LDR, narrow decode (212) | 8 | 5.3 MiB | 22 |

Fallbacks: `PANVK_BC_ETC=0` (BC1 to ASTC, BC2/BC3 to 12 bpp AFRC + raw) and
`PANVK_BC_EAC=0` (exact R8/RG8).

**How it gets its low memory:** the shadow is at the native BC size, and
**it drops the raw BC blocks** for almost every image (patch 180,
`panvk_bc_can_drop_raw()`). The raw plane is kept only for STORAGE /
attachment / HOST_TRANSFER usage, non-BC mutable views, sparse, alias,
disjoint, external or BLOCK_TEXEL_VIEW_COMPATIBLE images, or
`PANVK_BC_KEEP_RAW=1`. TRANSFER_SRC is **not** on that list, so DXVK
textures drop the raw blocks. Then:

- Uploads transcode straight from the copy-source buffer.
- ZERO_INITIALIZED transcodes one zero block.
- BC to BC `vkCmdCopyImage` copies the transcoded elements (16 B ASTC blocks
  or ETC2/EAC blocks, R8/RG8 quads) with a small compute shader,
  `panvk_bc_copy.comp`, when both images have the same decoded format and
  tiling. Otherwise it re-transcodes from the source's raw plane.
- `vkCmdCopyImageToBuffer` / `CopyImageToMemory` of such an image is
  **skipped**, with a one-time warning. The destination keeps stale data. This
  is the same class of bug as RC4-lowmem's garbage readback, and it fails
  our bc_decode raw check.

**Encoders** (one invocation per 4x4 block, specialised per format by spec
constant, U-interleaved output from patch 176):

- **BC1 to ETC2** (`panvk_bc_s3tc_etc.comp`, 214/215) works in the
  **palette domain**. A BC1 block has only 4 colours, so every fit runs on the
  4 palette entries weighted by their texel counts (`bitCount` on the index
  word), not on 16 texels. It picks **one** flip (the split whose halves vary
  least), then the best modifier table per half, scored with
  `3m² - 2m·s`. T and H modes are tried only on the luminance-contiguous
  splits of the palette, with the distance computed in closed form
  (`S/48` for H). The index bits are produced by bit tricks (compact +
  4x4 transpose). It has no per-texel loops at all.
- **BC4/BC5/BC3-alpha to EAC** (213, 215, 219) uses a **build-time LUT**.
  `panvk_bc_eac_lutgen.c` computes, for each of the 65536 BC4 endpoint pairs
  (e0,e1), the best EAC base/multiplier/table for the 8 palette values. It
  stores the UNORM fit in 16 bits and the SNORM fit in 16 bits: 256 KiB,
  uploaded once per device. The shader does one LUT load per block, then maps
  each BC4 palette index to its nearest EAC level (an 8 -> 8 map), so the
  per-texel work is a table remap. Patch 219 forces exact 0/255 when the
  palette reaches them.
- **BC7 to ASTC** (216): it uses the BC7 mode structure directly. Mode 6
  endpoints become the RGBA line, mode 4/5 rotation becomes ASTC dual plane,
  and opaque blocks use one 3x3 covariance line. Only mode 7 runs the full
  search.
- **BC6H to ASTC HDR** (217/218): LNS conversion once per block, plus a pre-refit.
- Quality they report (vs the BC decode, real game textures): BC1 37.5-37.9 dB,
  BC3 35-36, BC4 49, BC5 46, BC7 46.8, BC6H 50-54 (realistic blocks).

**Feature coverage:** mips and array layers are handled per region and layer
(`bz` / slice stride). sRGB comes from the view (ETC2 sRGB, ASTC sRGB). BC6H
SF is lossy-semantic (negatives become 0). Raw readback works only for
keep-raw images. BC copies are exact with respect to what is sampled, not to
the BC bits.

**Size:** 212-219 (the 1.1.5-FC step) add about 1.3k handwritten lines and
about 14.9k generated SPIR-V/LUT header lines. The whole BC series
(124-130, 172-180, 212-219) is about 140k patch lines, mostly generated headers.
Ours: 197 + 200-203 = 30.7k lines.

**License:** the fork inherits our NOTICE/LICENSES regime. Repo scaffolding is
MIT, patches keep Mesa's MIT license, and the new files carry
`SPDX-License-Identifier: MIT` ("panvk-kbase-android contributors"). The BC7/BC6H
tables are credited to Hans-Kristian Arntzen (MIT). **Reuse is allowed** with
the copyright notice and a credit line, e.g. "BC4/EAC LUT and palette-domain
BC1 to ETC2 encoder adapted from GunaCharanTeja/panvk-kbase-android csf-v11
213-215". The fork credits our AFRC design (csf-v11/150, 151, 180) the same
way.

## Comparison

| | RC4-lowmem (ours, binary) | fork 1.1.5-FC | ours bc3 (197, 200-203) |
|---|---|---|---|
| decode site | GPU compute at upload (precompiled libpan kernels) | GPU compute at upload | GPU compute at upload |
| BC1/BC4 shadow | ETC2 RGB8 / EAC R11, 4 bpp | same | same |
| BC2/BC3/BC5 shadow | ETC2 RGBA8 / EAC RG11, 8 bpp | same | same |
| BC7 shadow | AFRC 8 bpp | ASTC 4x4 LDR (narrow) 8 bpp | AFRC 8 bpp |
| BC6H UF / SF | exact shadow, 22.6 / ~43 MiB | ASTC HDR 5.3 / 5.3 MiB (SF lossy) | ASTC HDR 5.6 / RGBA16F ~43 MiB |
| raw blocks | none | none (except keep-raw usages) | 201 store: transient BO while in flight + spill file |
| readback | garbage | skipped (stale) | exact |
| BC to BC copy | garbage | copies transcoded blocks (consistent) | exact (raw) |
| encoder style | unknown (blob) | palette domain + LUT, one flip | full 16-texel search, both flips, H + planar, 16-table EAC |
| upload BC1 / BC3 / BC4 / BC7 / BC6H | 2.89 / 3.22 / 2.48 / 5.91 / 8.42 ms (X6 Pro, MC6) | 4.95 / 8.4 / 2.7 / 22 / 28 ms (**MC2**) | 3.45 / 4.33 / 2.56 / 5.67 / 7.93 ms (MC6) |

**Is the fork the same as RC4-lowmem?** Mostly, for BC1-BC5. It uses the same
ETC2/EAC targets at native BC size, with no raw blocks and no correct
readback. It differs in three ways:

1. BC to BC copies move transcoded blocks, so they are consistent where
   RC4's were garbage.
2. BC7 goes to ASTC rather than AFRC: same memory, slower to upload and to
   sample.
3. BC6H (UF and SF) goes to ASTC HDR, which is 4-8x smaller than RC4's
   BC6H. The SF clamp is a semantic loss.

The fork's encoders are its own source code. RC4's are blobs.

**Speed caveat:** the fork's numbers come from an MC2 (2 shader cores) and ours
from the X6 Pro's MC6 (6 cores). For ALU-bound transcodes, a rough
core-normalised view puts their BC1 at about 1.7 ms, BC3 at about 2.8 ms and
BC4 at about 0.9 ms on MC6. That would beat both bc3 and RC4. Their BC7
(about 7 ms) and BC6H (about 9 ms) would be slower than ours. This is an
**estimate, not a measurement**. The same bc_perf run on the X6 Pro is the
only real comparison.

**Sampling:** ETC2 RGBA8 decodes at 8 bits per channel and EAC at 16 bits on 1-2
channels, which keeps texels at 32 bpp or less, so filtering runs at full
rate. The same holds for us and for the fork on BC1-BC5. LDR ASTC without
narrow decode, and ASTC HDR, decode to fp16 RGBA (64 bpp), which filters at
**half rate** (fork 212). That applies to the fork's BC7 before 1.1.5 and to
our BC6H UF ASTC HDR shadow (202), whose old B10G11R11 shadow was 32 bpp.
Our earlier mode-E figures were AFRC BC7 at 0.26 ms, random ASTC at 0.84 ms
and B10G11R11 at 0.33 ms.

## Recommendations: lower memory than RC4-lowmem and faster

Top 3 first.

1. **Port the palette-domain / LUT encoders for BC1-BC5 (fork 213-215, MIT).**
   - Replace our 16-texel ETC1 search (BC1 58 cycles/block, 53-59 registers,
     1 thread, 203) with the 4-entry weighted fit and one-flip choice.
   - Replace our 16-table EAC search with the 64K-entry (e0,e1) LUT plus a
     nearest-level remap. We would generate it at build time like
     `panvk_bc_eac_lutgen`, upload it once per device as 256 KiB in a
     device BO, and use it for BC4, BC5 and BC3 alpha.
   - Expected result: a severalfold drop in ALU, registers below 32 (2+
     threads), and much shorter pipeline compiles. Compiles matter while
     the Android disk cache stays disabled (`/data/.cache` EACCES in the bc3
     run).
   - Keep the current exhaustive encoder as the `PANVK_BC_LM_OPTS` quality
     mode.
   - Gate: the host bench (`tools/bc-bench`, `host-bench-2026-10-22.md`) must
     show at most about 1 dB below bc3 on the picture sets, then bc_perf on
     the X6 Pro must beat RC4-lowmem's upload_gpu_ms on BC1-BC5.
   - The fork's LUT fit is unweighted over all 8 palette values. A
     weighted refit only when a block's error exceeds a threshold could win
     back the dB cheaply.
2. **Drop the per-upload raw write by default; copy transcoded blocks
   instead (fork 180 idea), and keep raw only where it can be observed.**
   - Today every DXVK upload writes raw blocks (fused store, 203), creates a
     BO, memsets it on the CPU and later spills it to flash (201). That is
     cost RC4 and the fork do not pay.
   - Proposal: BC to BC copies between lowmem images of the same shadow
     format use a block-copy shader (ETC2/EAC 8 or 16 B, AFRC/ASTC 16 B).
     That is exact with respect to sampling, with no raw needed.
   - Raw blocks are then needed only for image to buffer readback and for
     BC to non-BC copies. DXVK issues those only for staging/readback
     paths, which are rare.
   - Make the raw store lazy and adaptive. Keep it off by default. When a
     readback or BC to non-BC copy first hits a no-raw image, log it once,
     switch the VkDevice to raw-store mode (201) for **subsequently**
     created and uploaded images, and keep `PANVK_BC_RAW=store|plane|off`.
     This gives the same memory as RC4-lowmem with no spill I/O and no BO
     churn, so uploads get faster (the ~1.9 ms fixed part shrinks).
   - Unlike RC4 and the fork, correctness is kept for apps that need it: the
     test apps and bc_decode run with `PANVK_BC_RAW=store`, or flag it per
     app or engine (we already detect DXVK/vkd3d by engine name).
   - Strictly, the first readback after the switch is still wrong for
     images created before it. Document that.
3. **Push memory below RC4 where it is still large: BC6H, and the
   non-BC-but-adjacent costs.**
   - BC6H SF: an opt-in ASTC HDR path (`PANVK_BC_BC6H=astc_sf`, negatives
     become 0, as in the fork) takes ~43 to 5.6 MiB per 2048² chain. Most
     game BC6H SF content (skies, probes) is non-negative. Keep exact as
     default until quality on real data is checked.
   - For both BC6H paths, measure ASTC HDR sampling against B10G11R11 (half
     vs full rate filtering). If the ASTC HDR sampling loss is visible in
     game FPS, offer B10G11R11 for UF as a speed option.
   - Keep BC7 on AFRC 8 bpp. It has the same memory as the fork's ASTC and
     samples faster (0.26 ms against fp16-ASTC half rate); the fork's 22 ms
     BC7 upload on MC2 is also worse than ours. Do **not** follow the fork
     here.
   - From bc3.md: the 160 MiB G615 gpu_prerast arenas are the biggest
     remaining memory gap to RC4. Make them lazy. That is outside BC, but
     it is the easiest way to get below RC4-lowmem in total RSS.

Further ideas (lower priority):

- **Lazy transcode of small mips / untouched layers.** This would transcode
  on first use rather than at upload. It costs complexity (record-time hooks
  in every descriptor bind), and the upload already happens off the frame's
  critical path in DXVK. Not worth it before items 1-2.
- **Opaque BC7 / BC3 to ETC2 RGB8 (4 bpp).** This is not possible with
  shadows placed at `vkGetImageMemoryRequirements` time (lowmem-v2 §2), and it
  would need sparse-style lazy placement. Skip.
- **Narrow decode check:** we have no LDR ASTC shadows today. If ASTC is ever
  used for a UNORM view (for example a BC7 fallback without AFRC), set
  `pview.astc.narrow` as in fork 212.
- **Workgroup size / threads:** after item 1, re-sweep `PANVK_BC_LM_WG`. The
  current 8x8 was picked for the 1-thread, 59-register shader.

## Links

- Fork: https://github.com/GunaCharanTeja/panvk-kbase-android (main c52af8b6)
- Release notes: https://github.com/GunaCharanTeja/panvk-kbase-android/releases/tag/g615-1.1.5-FC
- Patches read:
  - `patches/csf-v11/180-drop-raw-bc-plane.patch`
  - `212-bc-astc-narrow-decode.patch`
  - `213-bc45-eac.patch`
  - `214-bc1-etc2.patch`
  - `215-bc23-etc2.patch`
  - `216-bc7-fast.patch`
  - `217-bc6h-fast.patch`
  - `218-bc6h-prerefit.patch`
  - `219-eac-exact-ends.patch`
- Fork docs: `README.md` (BCn table), `CHANGELOG.md`, `docs/BCN-COMPAT.md`,
  `NOTICE.md`, `LICENSES/`.
- Ours:
  - `patches/csf-v11/197`, `200`-`203`
  - `worklogs/g615-dxvk/builds/bc3.md`
  - `worklogs/g615-dxvk/device-runs/2026-10-22-g615-bc3/results.md`
  - `worklogs/bc-emul-perf/lowmem-v2.md`
  - `worklogs/bc-emul-perf/lowmem-197.md`
