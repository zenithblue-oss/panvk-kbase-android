# jm-v7 WIP (item 19, Bifrost DXVK feature port)

`scripts/apply-patches.sh` does not apply these. Apply them with `git am`, in order, on top of the full series (`apply-patches.sh --profile g615-v11-csf`, which already applies jm-v7 001-005). They are `git format-patch` files and depend on each other, so plain `git apply` of one alone fails.

Worktrees: `work/mesa-b2` (branch `v7port2`) is the current base: the full series as one commit `30a2037`, with 006-014 on top (014 = `5995e81`). `work/mesa-b` (branch `v7port`) has the same 006-013 on the older series (98c8b2f).

**Nothing here has run on a Bifrost device.** All results below are from builds and code review.

## Patches

| Patch | What it does |
|---|---|
| 0006 | Draw prerequisites on v6/v7: no IDVS, multiDrawIndirect, a position buffer. |
| 0007 | Geometry shaders, emulated with a compute job between the vertex job and the tiler job (`panvk_nir_lower_gs.c`, `panvk_gs.h`). Includes geometryStreams. |
| 0008 | Transform feedback, captured in the geometry shader emulation (staging records, `panlib_xfb_scan`/`panlib_xfb_copy`, counter buffers, pause/resume). |
| 0009 | Transform feedback from a vertex shader with no geometry shader, through a generated passthrough geometry shader, one per input primitive class, cached on the vertex shader. Also `transformFeedbackDraw` (`vkCmdDrawIndirectByteCountEXT`) on v6/v7. |
| 0010 | `robustBufferAccess2` and `nullDescriptor`. Bifrost has no NULL descriptor, so a null write copies a descriptor built on a zeroed BO or a 1x1 image with a ZERO swizzle. Null SSBOs force the bounds-checked SSBO address format. Ported from JimVulkan/mesa-unified-drivers (MIT, Copyright 2026 XclipseTools). |
| 0011 | Fixes 007/008: invalid NIR (phi else-values built after `nir_pop_if`), the GS `xfb_info` and union info left on the compute shader, and Instance Primitive Size 0 in the GS tiler DCD (a padded field that cannot encode 0). |
| 0012 | New (review of 009-011): `meta_gfx_start`/`meta_gfx_end` did not save or restore the bound geometry shader on v6/v7. A meta pipeline bind clears it, so the first draw after a blit, clear or resolve ran without the app's GS and without XFB capture. It is now saved, cleared and restored, as v9+ does. |
| 0013 | New: BC emulation on v6/v7. See "BC texture compression" below. |
| 0014 | New: tessellation on v6/v7 (`tessellationShader`, `extendedDynamicState2PatchControlPoints`). See "Tessellation (0014)" below. |

## Build status (2026-10-16, cloud session)

- Series: Mesa 5a07217f + the current full series (186 patches, `apply-patches.sh --profile g615-v11-csf`, strict: no offsets, no fuzz). This includes android/016 and csf-v11/187-192.
- `git am` of 0006-0014 on a fresh worktree of `30a2037` applies cleanly and gives a tree identical to `v7port2` (checked after 0014).
- `git am` of 0006-0013 on a fresh tree with that series applies cleanly. The patches were made with `git format-patch` from that same tree, so no hunk has an offset.
- Rebasing 006-013 onto the new series needed no conflict resolution. The combined diff is identical to the old one on `work/mesa-b`. Of the series changes since 98c8b2f, the only ones in files these patches touch are csf-v11/188 and 189 (`maxFragmentDualSrcAttachments`, denorm properties), and they do not overlap.
- `scripts/build-android.sh --profile g615-v11-csf` (API 29, NDK r30 30.0.16248370) builds the universal ICD with no errors and no warnings in the files 006-013 touch. The full build has 6 warnings, all from the series itself: 5x `-Wc23-extensions` at `csf/panvk_vX_gpu_queue.c:858` (csf-v11/192) and 1 deprecation in `android_stub/backtrace/BacktraceMap.h`. libpanvk_v6/v7/v9/v10-v14 are all built.
- `libvulkan_panfrost.so` from `work/mesa-b2` with 006-014: BuildID `5e59814c33319fdf66c69ca4a40a9f411c8e275f`, sha256 `aac8a07e9ca2eb49...`, in `dist/b2/`. It is unstripped, 26.57 MB. No warnings in the files 014 touches (checked by rebuilding them). Before 014 (006-013): BuildID `4f07459194dd25e244169faf1e1d8a0cbe3b2028`, 25.98 MB. NEEDED: liblog, libnativewindow, libsync, libm, libz, libdl, libc. It does not import `call_once`.
- From `work/mesa-b` (old series, same 006-013): BuildID `7a670a0ecfd47d3dae02ec3670973af1680e5017`, in `dist/b/`.

## Feature state on v6/v7 after 013

| Feature | State |
|---|---|
| geometryShader, geometryStreams | 007 (compute emulation) |
| transformFeedback | 008 (from a GS), 009 (from a VS, through the passthrough GS) |
| transformFeedbackDraw | 009 |
| transformFeedbackQueries | off |
| robustBufferAccess2, nullDescriptor | 010 |
| robustImageAccess2 | off: the out-of-bounds alpha value on Bifrost is unknown |
| shaderClipDistance/CullDistance | on, through the arch-independent VS store + FS discard lowering. The passthrough also forwards them (009). |
| multiDrawIndirect | 006 |
| textureCompressionBC | on (emulated), see below |
| tessellationShader, extendedDynamicState2PatchControlPoints | 014 (on; `PANVK_DEBUG=no_tess` turns them off) |
| shaderTessellationAndGeometryPointSize | off |
| fillModeNonSolid, multiViewport, depthBounds, pipelineStatisticsQuery | off (v9+ only) |

## BC texture compression

BC emulation does not depend on the arch. It was already on for v6/v7 before 013:
- `panvk_bc_emul_enabled()` (csf-v11/108) does not check the arch. It emulates unless every BC format is native. `textureCompressionBC` is `native || emulated`.
- A BC image is LINEAR (`panvk_image_get_mod`). It gets a decoded plane after the raw blocks. Uploads (`CmdCopyMemoryToImage`, `CmdCopyBufferToImage2`) and `CmdCopyImage2` into it decode with the compute shaders in `panvk_vX_cmd_meta.c` (`bc/*.comp`: buffer_reference loads and stores only, 8x8 workgroups). Views sample the decoded plane (`panvk_vX_image_view.c`). The decoded formats (RGBA8, RG16 unorm/snorm, RGBA16F) are plain Bifrost texture formats.
- The fragment decode into compact or AFRC shadows (csf-v11/150, 180, 185) is v10+ only (`image->bc_gfx`, gated on arch >= 10). v6/v7 and v9 always use the compute path. The PanProbe `bc_decode`/`bc_perf` results on the v9 G57 come from that same compute path.

The one gap was on JM: only the CSF `CmdPipelineBarrier2` called `cmd_bc_decode_zero_initialized()` (csf-v11/040). On v6/v7, an image leaving `VK_IMAGE_LAYOUT_ZERO_INITIALIZED_EXT` kept a zeroed decoded plane. It should hold the decode of zero blocks: opaque black for BC1. 013 adds the call on v6/v7. v9 JM has the same gap, and this patch leaves v9 alone.

## Tessellation (0014)

0014 implements `tessellationShader` and `extendedDynamicState2PatchControlPoints` on v6/v7, with the v9 limits (level 64, patch size 32, 128/120/4096 TCS components). `PANVK_DEBUG=no_tess` turns both off again. `shaderTessellationAndGeometryPointSize` stays off. v9+ is unchanged: every new path is `PAN_ARCH < 9` (the JM draw file, the v6/v7 libpan section, a `#else` in the shader compiler).

**Design.** A tessellated draw is recorded as two draws in one job chain (`panvk_tess_draw()` in `jm/panvk_vX_cmd_draw.c`):

1. The vertex shader over the API draw. It is compiled for clip space when a TCS follows (`next_stage_mask`), linked with every generic slot kept and no fragment shader. Its tiler job is replaced by the tess control job (`emit_tess_tcs_job()`).
   - `panlib_v7_tess_setup` reads the draw's Vk[Indexed]DrawIndirectCommand (written on the CPU for vkCmdDraw), fills `poly_tess_params` and patches the INVOCATION of the TCS job. It is a NULL job when there are no patches.
   - The TCS is compiled as compute: `panvk_nir_lower_tcs_inputs` (from 007), then `poly_nir_lower_tcs`, then `panvk_nir_lower_tess_params`. There is one workgroup per patch and instance (local size = output vertices, at most 32), and output barriers become workgroup barriers on global memory. It reads the vertex shader's varyings the way the GS emulation does, including indexed, indirect and instanced draws.
2. The libpoly tessellator, as new v6/v7 libpan kernels: `panlib_v7_tessellate` (64 wide, COUNT mode), `panlib_v7_tess_prefix` (single thread, serial), then `panlib_v7_tessellate` again (WITH_COUNTS mode, with the ccw swap of `panlib_tessellate`).
   - The 1024-wide `panlib_prefix_sum_tess` is not used.
   - The prefix sum keeps whole patches while their indices fit the budget, and writes the VkDrawIndirectCommand of step 3.
3. An indirect draw of the tessellator indices with the TES as its vertex shader.
   - The TES is compiled through the VS path: `poly_nir_lower_tes(to_hw_vs)`, then vertex *i* loads raw id `index_buffer[i]`, and the tess params come from a new graphics sysval.
   - It goes through the normal JM indirect path (`panvk_cmd_draw_indirect`): the draw helper sizes the vertex and tiler jobs and allocates the varyings for the real count. Layers, views, the GS emulation (007) after the TES and XFB (008/009 passthrough, built from the TES layout) apply unchanged.
   - 007's `panvk_nir_tes_to_gs` is removed: the TES is a vertex shader, not a point GS.

Every tess kernel is a barrier job. The TCS output, the counts and the tessellator output live in one 64 MB device arena (`panvk_device::tess_arena`, NO_MMAP | ALLOC_ON_FAULT, allocated only when the app enables tessellationShader). Each draw's first arena write waits for every earlier job of the chain. There is no CPU readback, and direct, indexed and indirect draws (including multiDrawIndirect and vkCmdDrawIndirectByteCountEXT) take the same path. The batch is split before the draw when its jobs (32 + 16 per layer) would overflow the 16-bit job index. The bound TCS/TES are saved and restored across meta operations.

**Limits (build- and host-tested only):**
- At most 8192 patches per draw (fewer with large TCS outputs, from the arena size): whole instances, or the first patches of the first instance. Direct draws log `mesa_logw_once`, indirect ones drop silently.
- At most 1M (2^20) tessellator indices per draw, and no more than the indirect varying buffer still holds (minus 1 MB). Whole patches past either limit are dropped on the GPU.
- With a GS or XFB after the TES: at most 8192 primitives (`PANVK_TESS_GS_MAX_PRIMS`), which sizes the GS emulation buffers.
- The prefix sum is serial (one thread over at most 8192 patches).
- FS gl_PrimitiveID after tessellation without a GS is the tiler's primitive index, not the patch ID. Clip/cull distances must come from the TES. Point mode draws at size 1.
- No transform feedback queries or pipeline statistics on v6/v7, as before.

**Host recording check (drm-shim).** I built a host debug build of `work/mesa-b2` (`build/host-b2`: debugoptimized, asserts on, `-Dtools=drm-shim`) and ran `validation/jm-v7-wip/tess-host-check/tesstest.c` (shaders in the same directory, `glslangValidator -V`) on `PAN_GPU_ID` 7212 (G52), 6221 (G72) and 7211 (G76). The run used `PAN_I_WANT_A_BROKEN_VULKAN_DRIVER=1`, `NOSUBMIT=1`, `LD_PRELOAD=libpanfrost_noop_drm_shim.so`, and passed the ICD path as argv[1].
- Pipelines: tri (TCS with a barrier, a patch output, a dynamic input array index and push constants; TES with gl_PrimitiveID and tess levels), quads (fractional_odd, cw, LOWER_LEFT origin), isolines, point mode, TES+GS, and TES with XFB.
- Draws, on a 2-layer framebuffer, recorded twice: vkCmdDraw with instances, vkCmdDrawIndexed, vkCmdDrawIndirect (drawCount 2), vkCmdDrawIndexedIndirect, a draw after vkCmdClearAttachments (meta restore), TES+GS direct and indirect, XFB Begin/draw/End, and vkCmdDrawIndirectByteCountEXT.
- Result: compiles with NIR validation, records with no assert, and valgrind is clean.
- gdb counts per run: 30 tessellated draws, 30 TCS jobs, 16 GS emulation jobs. That is 2 layers on the GS and XFB draws, as expected.
- The check found and fixed 2 bugs before the commit: TES input derefs left behind, and the Bifrost vertex ID lowering missing on the TES.
- Submission is not covered: the kbase sync code asserts under the panfrost drm-shim. Nothing ran on a GPU.

## Limits (all build-tested only)

- 009: gl_PointSize is not captured (it reads 0), and points drawn through the passthrough rasterize at size 1. gl_PrimitiveID in the FS counts across instances.
- 010: robustBufferAccess2 relies on the hardware bounds of UBO, attribute and texel-buffer descriptors, as JimVulkan reports for G52/G72/G76. That is unverified here.
- No device has run 006-014. A host recording check under the panfrost drm-shim (PAN_GPU_ID 6221, 7212, 7211) is described in 011's message. It covers command recording only: the drm-shim cannot run kbase JM atoms.

## What needs a device

- RTL A06 (G52 MC2 r1, `0x74021000`) and A14 (G52 MC1, `0x72120000`). Baseline PanProbe was 14/38 before 006. Run PanProbe with this build. Expected gains:
  - geometry, xfb, clip_cull, robustness2/null descriptor, bc_decode/bc_perf, vertex_stores, MDI;
  - tessellation cases are now expected to run (014): PanProbe `tessellation`, `gs_tess_primitive_id`, `large_draw`.
- CTS subsets: `dEQP-VK.tessellation.*` (watch for TCS barrier hangs, truncated large draws, winding), `dEQP-VK.geometry.*`, `transform_feedback.*`, `robustness.robustness2.*`, `texture.compressed.*bc*`, `draw.*indirect*`.
- DXVK D3D9 and D3D11 in PanPlay. With 014, DXVK can report FL11_0, so check that D3D11 titles that ran at FL10_x before still run (fall back with `PANVK_DEBUG=no_tess`).
- Move the series to `patches/jm-v7` (006+) once A06/A14 gets past 14/38.
