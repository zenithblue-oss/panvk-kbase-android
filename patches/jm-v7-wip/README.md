# jm-v7 WIP (item 19, Bifrost DXVK feature port)

`scripts/apply-patches.sh` does not apply these. Apply them with `git am`, in order, on top of the full series (`apply-patches.sh --profile g615-v11-csf`, which already applies jm-v7 001-005). They are `git format-patch` files and depend on each other, so plain `git apply` of one alone fails.

Worktrees: `work/mesa-b2` (branch `v7port2`) is the previous base: the full series as one commit `30a2037`, with the old 006-013 on top (014 = `5995e81`; old numbering, includes the dropped BC patch). `work/mesa-j` is the current base: the strict series as one commit `a709962` on Mesa `98c8b2f~1`, with the new 006-013 on top (013 = tessellation, `608f61d`). `work/mesa-b` (branch `v7port`) is the oldest base.

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
| (old 0013) | Dropped. The BC zero-init decode call in the JM `CmdPipelineBarrier2` is now `patches/jm-v9/031`, which adds it without a PAN_ARCH gate, so v6/v7 get it from the series. See "BC texture compression" below. |
| 0013 | New: tessellation on v6/v7 (`tessellationShader`, `extendedDynamicState2PatchControlPoints`). See "Tessellation (0013)" below. The subject line still says "jm-v7 014" (old numbering). |
| 0014 | (jm-v7 015) `depthBounds`: the csf-v11/092 FS emulation (LD_TILE of the stored depth with the Bifrost Z_STENCIL message, SampleMask cleared outside the bounds) on v6/v7, the device depth-bounds FS, a ZS tile read in the RSD while the test is on, and the late ZS update with EarlyFragmentTests (jm-v9/018). See "Features added in v7feat" below. |
| 0015 | (jm-v7 016) `pipelineStatisticsQuery` and `transformFeedbackQueries`, GPU-counted in the v9 JM report layout. See "Features added in v7feat" below. |

## Build status (2026-10-16, cloud session)

- Series: Mesa 98c8b2f~1 + the current full series (`apply-patches.sh --profile g615-v11-csf`, 189 patches applied, strict: no offsets, no fuzz). This includes jm-v9/031 and csf-v11 up to 194.
- `git am -3` of the old 0006-0012 and old 0014 on that series applied with no conflict (0014 did not need 0013's context). `git am` of the new 0006-0013 on a second fresh tree of the series gives a tree identical to `work/mesa-j` HEAD (same tree hash `b226095`).
- Only one call to `cmd_bc_decode_zero_initialized` exists in the JM barrier (from jm-v9/031), one in CSF.
- `scripts/build-android.sh --profile g615-v11-csf` (API 29, NDK r30) ends OK with no warnings in the build log.
- `libvulkan_panfrost.so` from `work/mesa-j` with 006-013: BuildID `09587b23aab8a8ac2787c49f75823aefbc765632`, in `dist/j/`. Unstripped.
- Older builds (previous base and numbering, 006-013 with the dropped BC patch): `5e59814c33319fdf66c69ca4a40a9f411c8e275f` (`dist/b2/`); 006-013 on the 98c8b2f series: `7a670a0ecfd47d3dae02ec3670973af1680e5017` (`dist/b/`).

## Feature state on v6/v7 after 013

| Feature | State |
|---|---|
| geometryShader, geometryStreams | 007 (compute emulation) |
| transformFeedback | 008 (from a GS), 009 (from a VS, through the passthrough GS) |
| transformFeedbackDraw | 009 |
| transformFeedbackQueries | 0015 (from the capture scan) |
| robustBufferAccess2, nullDescriptor | 010 |
| robustImageAccess2 | off: the out-of-bounds alpha value on Bifrost is unknown |
| shaderClipDistance/CullDistance | on, through the arch-independent VS store + FS discard lowering. The passthrough also forwards them (009). |
| multiDrawIndirect | 006 |
| textureCompressionBC | on (emulated), see below |
| tessellationShader, extendedDynamicState2PatchControlPoints | 013 (on; `PANVK_DEBUG=no_tess` turns them off) |
| shaderTessellationAndGeometryPointSize | off |
| depthBounds | 0014 (FS emulation) |
| pipelineStatisticsQuery | 0015 |
| fillModeNonSolid, multiViewport | off (in progress) |

## Features added in v7feat (0014+)

Files are numbered one below their subject (file 0014 = "jm-v7 015"), as 0013 = "jm-v7 014". Base: `/home/flux/src/mesa-v7feat` (branch `v7feat`): Mesa 5a07217 + the 192-patch `g615-v11-csf` series as one commit, then `git am` of 0006-0013 and the new patches.

### depthBounds (0014)

- Same design as v10+: when the pipeline may enable the test (static or dynamic), the FS loops over the samples (uniform sample index), loads the stored depth with `nir_load_tile_pan` (Bifrost `LD_TILE` with `z_stencil`, which `bi_message_type_for_instr()` sends as a Z_STENCIL message) and clears the SampleMask bit of samples outside `[min, max]`. Bounds, enable and sample count come from the `ds.*` graphics sysvals, now filled on every arch.
- The device depth-bounds FS (bound when the app has no FS) is created on v6/v7.
- v6/v7 RSD: while the test is on, `zs_read = PAN_EARLYZS_ZS_TILEBUF_READ_NO_OPT`; with EarlyFragmentTests and ZS writes or an occlusion query, the ZS update is forced late (the jm-v9/018 / csf-v11/116 rule).
- Cost: a lowered FS writes SampleMask, so it has no FPK or early ZS even while the test is off. DXVK makes the test dynamic, so every DXVK FS is lowered.
- Unknown until a device run: LD_TILE of Z16 and Z24S8 depth formats on Bifrost.

### pipelineStatisticsQuery, transformFeedbackQueries (0015)

Reports use the v9 JM layout (two u64 per statistic; kernels and shaders write the second half; results are the sum), so `vkGetQueryPoolResults` and `panlib_copy_pstats_query_result` take the v9 paths.

| Statistic | Counted by |
|---|---|
| IA vertices, IA primitives, VS invocations, TCS patches | `panlib_v7_update_pstats_draw()`, one thread per API draw (the draw's Vk[Indexed]DrawIndirectCommand; written at record time for vkCmdDraw). Primitive restart: serial index scan (the v9 kernel needs 16-wide subgroups). |
| TES invocations | the same kernel on the tess eval draw's generated command (one invocation per tessellator index, which is how the TES runs). |
| Clipping invocations/primitives | the draw kernel without a GS; with a GS, the emulation (stream 0 primitives). |
| GS invocations, GS primitives | the emulation itself, at the end of every live invocation (`panvk_gs_push::pstats`, four counter addresses). The xfb passthrough is not counted as a GS. |
| FS invocations | the hardware occlusion counter while no occlusion query owns it (as v9 JM). Bifrost writes it per core, so it points at a per-query scratch (one u64 per core, zeroed by GPU write jobs), which `panlib_v7_pstats_sum_cores()` adds into the report at vkCmdEndQuery. |
| CS invocations | `panlib_v7_update_pstats_dispatch()`. |

Transform feedback stream queries: `panlib_xfb_scan` adds the primitives written and needed of its stream to the active query (`libpan_xfb_draw::query`). A stream that captures to no buffer is still scanned while a query is active.

Limits (ponytail, same as v9 JM): FS invocations miss fragments killed by late ZS, count samples with MSAA, and are not counted while an occlusion query is active. Layered (non-multiview) draws count layer 0 only. vk_meta draws are not counted. Stream queries count only while transform feedback is active (the capture only runs then).

## BC texture compression

BC emulation does not depend on the arch. It is on for v6/v7:
- `panvk_bc_emul_enabled()` (csf-v11/108) does not check the arch. It emulates unless every BC format is native. `textureCompressionBC` is `native || emulated`.
- A BC image is LINEAR (`panvk_image_get_mod`). It gets a decoded plane after the raw blocks. Uploads (`CmdCopyMemoryToImage`, `CmdCopyBufferToImage2`) and `CmdCopyImage2` into it decode with the compute shaders in `panvk_vX_cmd_meta.c` (`bc/*.comp`: buffer_reference loads and stores only, 8x8 workgroups). Views sample the decoded plane (`panvk_vX_image_view.c`). The decoded formats (RGBA8, RG16 unorm/snorm, RGBA16F) are plain Bifrost texture formats.
- The fragment decode into compact or AFRC shadows (csf-v11/150, 180, 185) is v10+ only (`image->bc_gfx`, gated on arch >= 10). v6/v7 and v9 always use the compute path. The PanProbe `bc_decode`/`bc_perf` results on the v9 G57 come from that same compute path.

The one gap was on JM: only the CSF `CmdPipelineBarrier2` called `cmd_bc_decode_zero_initialized()` (csf-v11/040), so an image leaving `VK_IMAGE_LAYOUT_ZERO_INITIALIZED_EXT` kept a zeroed decoded plane. The former 0013 added the call on v6/v7; `patches/jm-v9/031` now adds it for all JM archs, so 0013 was dropped.

## Tessellation (0013)

0013 implements `tessellationShader` and `extendedDynamicState2PatchControlPoints` on v6/v7, with the v9 limits (level 64, patch size 32, 128/120/4096 TCS components). `PANVK_DEBUG=no_tess` turns both off again. `shaderTessellationAndGeometryPointSize` stays off. v9+ is unchanged: every new path is `PAN_ARCH < 9` (the JM draw file, the v6/v7 libpan section, a `#else` in the shader compiler).

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
- No device has run 006-013. A host recording check under the panfrost drm-shim (PAN_GPU_ID 6221, 7212, 7211) is described in 011's message. It covers command recording only: the drm-shim cannot run kbase JM atoms.

## What needs a device

- RTL A06 (G52 MC2 r1, `0x74021000`) and A14 (G52 MC1, `0x72120000`). Baseline PanProbe was 14/38 before 006. Run PanProbe with this build. Expected gains:
  - geometry, xfb, clip_cull, robustness2/null descriptor, bc_decode/bc_perf, vertex_stores, MDI;
  - tessellation cases are now expected to run (0013): PanProbe `tessellation`, `gs_tess_primitive_id`, `large_draw`.
- CTS subsets: `dEQP-VK.tessellation.*` (watch for TCS barrier hangs, truncated large draws, winding), `dEQP-VK.geometry.*`, `transform_feedback.*`, `robustness.robustness2.*`, `texture.compressed.*bc*`, `draw.*indirect*`.
- DXVK D3D9 and D3D11 in PanPlay. With 0013, DXVK can report FL11_0, so check that D3D11 titles that ran at FL10_x before still run (fall back with `PANVK_DEBUG=no_tess`).
- Move the series to `patches/jm-v7` (006+) once A06/A14 gets past 14/38.
