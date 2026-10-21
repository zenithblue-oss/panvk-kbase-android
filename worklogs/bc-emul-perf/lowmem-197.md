# csf-v11/197: BC lowmem (ETC2/EAC transcode), recreated

Recreation of the lost RC4-lowmem BC patch from the binary analysis in
`worklogs/g615-dxvk/rc4-binary-analysis.md` (section "Result 3"), with the
readback/copy bug fixed. Patch: `patches/csf-v11/197-bc-lowmem-etc2-transcode.patch`.

## Behaviour

- Gate (`panvk_bc_lowmem_init()`, physical device): v10+ (CSF), BC
  emulation on, the fragment decode path available (no
  `PANVK_DEBUG=bc_compute`/`no_gs`, the 0x480 debug mask of the binary), and
  all ten ETC2/EAC formats native. On by default as in the released
  binary-patched RC4 lowmem; `PANVK_BC_MODE=exact` turns it off (any other
  value except `lowmem` also falls back to exact, with a warning).
- Log: `panvk: BC emulation on, lowmem (native compressed mask 0x...)`.
- Per image (`panvk_image_init()`): 2D `bc_gfx` images, `OPTIMAL` tiling,
  none of SPARSE_*, DISJOINT, ALIAS, BLOCK_TEXEL_VIEW_COMPATIBLE, no
  `VkExternalMemoryImageCreateInfo`. BC4/BC5 with MUTABLE_FORMAT need a
  format list with only their own format (the EAC shadow fixes the
  signedness). Everything else keeps the exact path.
  - `bc_lowmem = 1` (BC1-BC5): `bc_shadow` is an ETC2/EAC image
    (U-interleaved, SAMPLED only): BC1 RGB -> ETC2 RGB8, BC1 RGBA -> ETC2
    RGB8A1, BC2/BC3 -> ETC2 RGBA8, BC4 -> EAC R11 (signed), BC5 -> EAC RG11
    (signed). sRGB comes from the view.
  - `bc_lowmem = 2` (BC6H/BC7): the exact RC4 shadow, raw plane dropped.
  - `bc_no_raw`: the raw block plane gets no memory when the image has
    neither TRANSFER_SRC nor HOST_TRANSFER usage.
- Transcoder: `src/panfrost/vulkan/bc/panvk_bc_etc2.comp`, GLSL compute
  shader built to SPIR-V (`panvk_bc_etc2_spv.h`, `gen_spv.sh`) and run
  through vk_meta, the same mechanism as the 022/150 BC decoders (RC4
  used libpan kernels; the precompiled blobs are not recoverable). One
  invocation per block: exact BC decode, then ETC1 differential/individual
  (both flips), ETC2 H and planar modes for colour, EAC with a 16-table
  search for alpha/R11/RG11. It writes straight into the U-interleaved
  plane.
- `PANVK_BC_LM_OPTS` is kept, with new bit meanings (default `0x30000`):
  `0x10000` try the planar mode, `0x20000` try the H mode, `0x40000` EAC
  base codeword +-1 refinement (off; +0.5 dB on BC4, 3x the EAC work).
- No device BO: the zero source for ZERO_INITIALIZED of a raw-less image
  is one zeroed block row from the command buffer pool, read with zero
  pitches.

## The readback / vkCmdCopyImage bug and the chosen fix

RC4 lowmem never stored the raw blocks, so `vkCmdCopyImage` from BC images,
image->buffer readback and host copies returned garbage for all 16 formats.

Options (from the analysis doc): (a) keep the raw plane for images that can
be read back, (b) copy/transcode from the decoded data, (c) no lowmem for
TRANSFER_SRC images.

Chosen: (a), at the granularity Vulkan allows. Raw blocks can only be
read back from an image with TRANSFER_SRC (or HOST_TRANSFER) usage, so:

- TRANSFER_SRC images keep the raw plane (exactly as before), and every
  readback and BC-source copy is bit-exact. They still get the ETC2/EAC
  shadow, so BC1-BC5 shrink anyway: raw + ETC2 is 2x the BC size, against
  raw + AFRC/AFBC shadow before (BC1 2048^2 chain about 8.2 -> 5.5 MiB,
  BC3 16.4 -> 11 MiB, BC4 8.5 -> 5.5, BC5 16.6 -> 11). BC6H/BC7 with
  TRANSFER_SRC are unchanged.
- Images without TRANSFER_SRC drop the raw plane (RC4-lowmem numbers:
  BC1/BC3/BC4/BC5 = BC size, BC7/BC6H minus the raw plane). Copies into
  them transcode/decode from the source's raw plane (a copy source always
  has TRANSFER_SRC) or, for a non-BC source, from a staging copy in
  command buffer memory.

Trade-off: DXVK sets TRANSFER_SRC on every texture, so under DXVK the gain
is the ETC2-vs-shadow part only (about -33% on BC1-BC5, 0 on BC6H/BC7),
not RC4-lowmem's -66%. (b) would give the full gain to DXVK and keep
BC->BC copies consistent with what is sampled, but raw readback
(image->buffer, BC->uncompressed copies) would still be wrong without a
BC re-encoder, so it does not fix the bug. (c) is (a) without the shadow
saving on TRANSFER_SRC images.

## Host verification (done)

`/tmp/.../scratchpad/test_etc2.c` (not committed) includes the shader body
as C (it is written in the common GLSL/C subset) and checks it against
Mesa's ETC2/EAC decoder (`src/mesa/main/texcompress_etc.c`, verbatim):

- 20000 random BC blocks: for every mode (diff/individual both flips,
  punch-through, H, planar, EAC alpha/R11/signed R11) the encoder's own
  error equals the error of Mesa's decode of the emitted block, and mode
  detection/punch-through alpha match. 0 failures.
- 512x512 synthetic picture (gradients, edges, +-4 noise), crude BC
  encoder, PSNR of the transcode vs the exact BC decode:
  - opts 0x30000: BC1 RGB 38.7 dB, BC3 colour 38.7 dB, BC3 alpha 49.1 dB,
    BC4 UNORM 49.3 dB (modes: diff 58%, planar 33%, H 7%, ind 2%).
  - opts 0 (ETC1 modes only): BC1 34.6 dB. 0x70000: EAC +0.5 dB.

## Device test plan (not run yet)

Run on the G615 (Poco X6 Pro) with the 197 build, default env unless noted.

1. Log: `logcat | grep "BC emulation"` shows `on, lowmem`;
   `PANVK_BC_MODE=exact` shows `on` and behaves like RC4.
2. PanProbe / `tests/dxvk/vulkan/bc/bc_decode.c` + `bc_verify.py`, all 16
   formats, sRGB/UNORM/SNORM, 2 mips, 2 layers, image->image copy, blit,
   raw readback, partial 8x8 sub-update:
   - `PANVK_BC_MODE=exact`: 16/16 as before (regression gate).
   - default: raw readback and image->image copies must be bit-exact for
     all 16 formats (bc_decode uses TRANSFER_SRC, so the raw plane is
     kept); sampled values: BC6H/BC7 unchanged, BC1-BC5 within the
     transcode error (PSNR gate below, not the 2 LSB tolerance).
3. vkCmdCopyImage BC test, all 16 formats, for each format:
   (a) BC (TRANSFER_SRC) -> BC (TRANSFER_SRC): readback of dst == src blocks;
   (b) BC -> BC without TRANSFER_SRC on dst (no raw plane): sample dst ==
       sample src (same transcode of the same blocks: bit-exact for
       BC1-BC5, exact decode for BC6H/BC7);
   (c) R32G32_UINT / R32G32B32A32_UINT -> BC without TRANSFER_SRC (staging
       path): sample == sampling the same blocks uploaded from a buffer;
   (d) BC -> R32G32(B32A32)_UINT: readback == source blocks;
   (e) ZERO_INITIALIZED -> SHADER_READ on raw and raw-less images: samples
       equal the decode of all-zero blocks.
4. PSNR vs exact: `bcperf-quality.sh` (MiSide and dxcube 1024^2 crops),
   LOD0 blitted to RGBA8, default vs `PANVK_BC_MODE=exact`. Expect
   BC1/BC2/BC3 colour >= 38 dB, alpha/BC4/BC5 >= 48 dB (RC4 lowmem
   measured 39-50 dB). Also try `PANVK_BC_LM_OPTS=0x70000`.
5. `device/bc-perf.c` (SAMPLED|TRANSFER_SRC|TRANSFER_DST): image memory
   per format (expect about 2x BC size for BC1-BC5, unchanged BC6H/BC7),
   plus a run with `BCPERF_USAGE` without TRANSFER_SRC if available
   (expect RC4-lowmem numbers: BC1 2.7, BC3 5.5, BC7 5.5, BC6H 22.6 MiB).
   Upload ms per 2048^2 chain: the transcoder costs more than the
   fragment decode; record it and try opts 0x10000/0 if it is too slow.
   Peak RSS: no host staging is used (the RC4 +100 MB should be gone).
6. CTS: `dEQP-VK.texture.compressed.*bc*`, `dEQP-VK.api.copy_and_blit.*bc*`,
   `dEQP-VK.memory.zero_initialize_device_memory.image_transition.bc*`.
7. Game check (MiSide / NFS) default vs exact: no visual corruption,
   memory in DXVK_HUD.
