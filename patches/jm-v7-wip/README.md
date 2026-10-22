# jm-v7 WIP (item 19, Bifrost DXVK feature port)

`scripts/apply-patches.sh` does not apply these. Apply them with `git am`, in order, on top of the full series (`apply-patches.sh --profile g615-v11-csf`, which already applies jm-v7 001-005). They are `git format-patch` files and depend on each other, so plain `git apply` of one alone fails.

## Renumbering (v7wip-order): file order = apply order

The files are now numbered so that `git am patches/jm-v7-wip/00*.patch` (glob order) on the 197-patch base (`ab7d630`) reproduces the v7gDI tree (`/home/flux/src/mesa-v7DI`, `v7DI-full`, 0b26a4a). Patch contents are unchanged; subjects keep their "jm-v7 NNN" numbers. Verified in `/home/flux/src/mesa-v7wip-order`: the result differs from 0b26a4a only in `PANVK_DEBUG_RIA2` (`1 << 30` here, the M14 fix in 0019; the v7DI build still had `1 << 29`, colliding with `BC_TIMING`).

Everything below this section and all worklogs (including the v7G/v7H/v7GDI build records) use the OLD numbers. Old -> new:

| Old file number | New | jm-v7 subject |
|---|---|---|
| 0006-0021 | 0006-0021 (unchanged) | 006-022 |
| 0030 | 0022 | 030 |
| 0031 | 0023 | 031 |
| 0032 | 0024 | 023 |
| 0033 | 0025 | 024 |
| 0034 | 0026 | 025 |
| 0040-0049 | 0027-0036 | 040-049 |
| 0035 | 0037 | 035 |
| 0036 | 0038 | 036 |
| 0025 | 0039 | 026 |
| 0026 | 0040 | 027 |
| 0070-0080 | 0041-0051 | 070-080 |
| 0050 | 0052 | 050 |
| 0051 | 0053 | 051 |

Apply order (= file order): 0006-0021, 0022-0026, 0027-0036, 0037, 0038, 0039, 0040, 0041-0051, 0052, 0053.

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
| 0016 | (jm-v7 017) `multiViewport` (16 viewports): the GS emulation selects the viewport per primitive; union scissor/depth in the tiler, per-viewport scissor as an FS clip-distance kill. See "Features added in v7feat" below. |
| 0017 | (jm-v7 018) `fillModeNonSolid`: triangle draws in LINE/POINT polygon mode go through a generated passthrough GS that emits edges or vertices and culls faces itself. See "Features added in v7feat" below. |
| 0018 | (jm-v7 019) kbase: after strides 56 and 64, try the 48-byte JM atom (kbase before uAPI 11.22, no renderpass_id: the G71/G72-era kernels). See "v6 (G71/G72) parity" below. |
| 0019 | (jm-v7 020) `PANVK_DEBUG=ria2`: advertise robustImageAccess2 on v6/v7 with no emulation, to measure the hardware. Off by default. See "robustImageAccess2" below. |

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
| robustImageAccess2 | off by default; `PANVK_DEBUG=ria2` (0019) exposes it for measurement |
| shaderClipDistance/CullDistance | on, through the arch-independent VS store + FS discard lowering. The passthrough also forwards them (009). |
| multiDrawIndirect | 006 |
| textureCompressionBC | on (emulated), see below |
| tessellationShader, extendedDynamicState2PatchControlPoints | 013 (on; `PANVK_DEBUG=no_tess` turns them off) |
| shaderTessellationAndGeometryPointSize | off |
| depthBounds | 0014 (FS emulation) |
| pipelineStatisticsQuery | 0015 |
| multiViewport | 0016 (GS-selected; shaderOutputViewportIndex stays off) |
| fillModeNonSolid | 0017 (passthrough GS; no app GS output in polygon mode) |

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

### multiViewport (0016)

- Only a geometry shader can select a viewport (shaderOutputViewportIndex stays off on v6/v7), and on Bifrost it runs as the compute emulation, which already does the viewport transform from a per-viewport table (007). The table now holds every viewport's transform and scissor box (40-byte entries); an index past viewportCount uses the last viewport.
- The tiler has a single viewport descriptor. While the bound emulation writes gl_ViewportIndex and viewportCount > 1 it gets the union of the scissor boxes and depth ranges (re-emitted on a GS change).
- Each primitive is cut to its own scissor in the FS: the emulation writes the four edge distances, times clip w (so the perspective-interpolated value keeps its sign), to CLIP_DIST1. An FS compiled without the previous stage's layout (always the case after a GS on v6/v7) already kills on negative CLIP_DIST0/1. The GS's own compact clip distances are not carried by the emulation, so the slot is free.
- The FS reads gl_ViewportIndex from the emulation's copy (PANVK_GS_VIEWPORT_INDEX_SLOT); unwritten, it reads constant 0.
- Limits: depth clamp and near/far clip use the union of the depth ranges, not the selected viewport's (differs only when the viewports have different depth ranges). A GS writing non-compact data at CLIP_DIST1 gets no per-viewport scissor.

### fillModeNonSolid (0017)

- The Bifrost tiler has no polygon mode. A triangle topology (list, strip, fan and the adjacency ones) drawn in VK_POLYGON_MODE_LINE or _POINT with no GS bound goes through the geometry emulation with a generated passthrough (`panvk_polygon_gs()`, the 009 builder with a polygon mode, one per mode and class, cached on the vertex shader). It emits each triangle as a closed line strip (3 lines) or as 3 points. The VS is not recompiled: the emulation converts its screen-space position back (`in_screen_space`).
- The tiler cannot cull lines or points, so the passthrough culls faces itself: the Vulkan area rule in framebuffer coordinates (viewport 0 scale) against the draw's cull mode and front face (`panvk_gs_push::polygon_cull`).
- The tess eval draw of a tessellated draw takes the same path (its emulation is sized as for a nested GS).
- Limits: an app GS's triangle output is still filled. A VS with transform feedback draws filled (the xfb passthrough has priority). Flat varyings of an edge come from that edge's first vertex. Points are size 1. No depth bias on the emitted lines/points. Indirect draws keep the emulation cap (4096 input primitives).

### robustImageAccess2 (0019, measurement only)

Not enabled, because it cannot be shown correct without a device:
- Bifrost storage image loads are `LD_ATTR_TEX`/`LEA_ATTR_TEX` on attribute descriptors and `texelFetch` is a TEX fetch. Upstream relies on their hardware bounds for robustImageAccess, but the value an out-of-bounds load returns (zeros, and alpha 1 for formats without alpha) is not documented or measured on G52/G72/G76.
- The v9/v10 fix-up (`bi_lower_robust_image_alpha_v10`) reads the view format from the texture descriptor in the shader. A Bifrost shader cannot address its texture or attribute descriptors (they sit in tables the copy jobs fill), so a formatless storage image or a sampled image has no known alpha at compile time. An exact emulation would need the format passed per binding (for example a driver-set side table), plus a shader bounds check per access.
- `PANVK_DEBUG=ria2` advertises the feature with no change to shaders, so PanProbe `robust_image_access2` (R32_UINT and RGBA8_UNORM, imageLoad/texelFetch/texel buffer/stores/atomics) and the CTS `robustness2` image cases can measure the hardware. If they pass as on G615, the bit can simply be turned on; if only alpha is wrong, the side-table fix-up is the next step.

### v6 (G71/G72) parity

Checked so that v6 takes the v7 path:
- IDs: `pan_model.c` has G71 `PAN_PROD_ID(6,0,0)` and G72 `(6,2,1)`; jm-v7 001 (variant ignored), 002 (no env gate), 003 (no JIT, growable heap) and 004 (atom stride check) all test `arch < 9` or `case 6: case 7:`. Every jm-v7-wip path is `PAN_ARCH < 9` or `PAN_ARCH == 6 || PAN_ARCH == 7` (libpan), never `== 7`.
- Atom stride: G71/G72 phones often run kbase from before uAPI 11.22 (no `renderpass_id`, 48-byte `base_jd_atom_v2`), which refuse 56 and 64. 0018 adds 48 as the last candidate.
- Compiler quirks: `bifrost_get_quirks()` gives G71 `BIFROST_NO_FP32_TRANSCENDENTALS | BIFROST_LIMITED_CLPER` and G72 `BIFROST_LIMITED_CLPER` from the real gpu_id. The precompiled libpan kernels for v6 are built by `panfrost_compile` with gpu_id `0x60000000`, which gets the G71 quirk set, so they are safe on both. The v7 kernels get the G31 set (`LIMITED_CLPER`).
- Gallium v6 workarounds: the `pan_model` quirks (`no_hierarchical_tiling`, `max_4x_msaa`) are Midgard-only rows. The v6 varying swizzle (`PAN_ARCH <= 6` in `get_varying_format`/`varying_format`/`emit_varying_attrs`), the v6 blend-constant shader path and the v6 resolve-to-AFBC limit are upstream PanVK code. 3D AFBC (v7+ only in gallium) is never offered: `pan_mod_afbc_get_format_caps()` gives no `DIM_3D`. Mutable-format AFBC is off on v7- (upstream).
- Not covered: no v6 device has run anything.

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

## Rebase note (2026-10-21)
0006-0019 were refreshed onto the 197-patch `g615-v11-csf` series (csf-v11 up to 202). 0010 had a 19-line offset and 0013 conflicted with csf-v11/200 (`PANVK_DEBUG_BC_TIMING`) in `panvk_instance.[ch]`; both fixed. The set applies strictly (no offset, no fuzz) with `git am` in order. The earlier build statements above (BuildIDs, 189/191/192-patch bases) describe the older base.

## 0030 (jm-v7 030, A06 v7E groups B, C, D)

Base: `/home/flux/src/mesa-v7fixB` (branch `v7fixB`) = the v7E tree (0006-0019 + the v7E tag), then `git am` 0030. Other agents' patches use 0020-0029 and 0040-0049.

- Primitive restart in the GS emulation: `panlib_v7_gs_restart_list()` (serial) rewrites an indexed strip/fan/strip-adjacency draw with restart as a restart-free 32-bit list and its own indexed indirect command; the emulation (and the 0017 polygon passthrough) assembles a list. Fixes fill_mode `*_restart_*` and geometry `*_restart_*`.
- Per-viewport depth (gs_viewport_depth): when the GS-selectable viewports' depth ranges differ, one index-list copy and one tiler job per viewport with its own VIEWPORT (scissor + depth range); `panvk_gs_push::vp_split`. Limit: primitives of different viewports with overlapping scissors lose API order. The csf-v11/193 CLIP_DIST0 overwrite has no v7 counterpart.
- shaderOutputViewportIndex: a last VS/TES writing gl_ViewportIndex runs through the passthrough GS (index in `PANVK_GS_VIEWPORT_INDEX_SLOT`).
- Tessellation caps: 8M indices (160 MB growable arena), 2M primitives into a GS after the TES (emulation buffers from the new growable `gs_pool`), 4M-vertex indirect varying buffer (growable).
- shaderTessellationAndGeometryPointSize: TES point-mode gl_PointSize, GS point output FP32 size array, gl_in[].gl_PointSize in GS/TCS, point passthrough forwards it.
- Status: A06 b2 (sha 86615fbc): fill_mode, geometry, gs_viewport_depth PASS; tessellation rendering cases all PASS (stats_quad_equal_l4 needs 0020); vs_viewport_index A-F PASS, G needs 0031.

## 0031 (jm-v7 031, FS gl_PrimitiveID behind the GS emulation)

- The emulation tiler DCD had instance_primitive_size = 1, so the FS primitive ID (tiler primitive index modulo that size) was always 0. Now the padded output primitive count, or the exact per-instance count when CPU-known and encodable. One passthrough invocation is one primitive, so the slot index is the input primitive ID. A user GS that writes gl_PrimitiveID still gets its output slot index, not the written value.
- 0030/0031 and group A 0020/0021 are independent: they apply in either order (b3 tree: 0030, 0031, 0020, 0021).

## v7G merge (2026-10-21): series order and renumbering

The merged v7G tree (`/home/flux/src/mesa-v7G`, branch `v7G`) is the 197-patch base (`ab7d630`) + this series applied with `git am` in file order. jm-v7 023 needs jm-v7 030 (growable `gs_pool`), so the group A patches after 0021 were renumbered to follow 0030/0031:

- 0022 (jm-v7 023) -> 0032, 0023 (jm-v7 024) -> 0033, 0024 (jm-v7 025) -> 0034. Subjects keep their jm-v7 numbers.
- jm-v7 024 (instanced attribute offset_start) appears once, as 0033.
- Order: 0006-0019, 0020, 0021, 0030, 0031, 0032-0034, 0040-0049.
- Conflicts resolved (both sides kept): 0046 vs 0030 in `libpan/draw_helper.cl` (new kernels next to each other); 0049 vs 0030 in the `emit_gs_job()` push initializer (`.tess_params` next to the psiz/viewport fields).
- 0020-0049 were regenerated with `git format-patch` from the v7G tree, so the files reproduce the tree exactly.
- `PANVK_NULLIMG_INBOUNDS` defaults to 1 (0048); `PANVK_NULLIMG_INBOUNDS=0` restores the 0-byte null image.

## v7G-A4 (2026-10-21): 0025-0029

Made on top of the v7G tree HEAD (`121f9b1`, after 0049) in `/home/flux/src/mesa-v7fixA4` (branch `v7fixA4`), so they apply AFTER 0049 (`git am` 0025.. on the v7G tree), not in file order before 0030.

- 0025 (jm-v7 026): xfb gl_CullDistance capture on v6/v7. The VS's CULL_DIST0/1 hold the FS kill flags (`panvk_lower_cull_dist_flags_intr()`), so jm-v7 023 captured 1/0 flags. A VS/TES whose xfb captures cull also writes the raw distances to TEX0/1 (`PANVK_XFB_RAW_CULL_SLOT0/1`), the passthrough captures them from there. PanProbe xfb vs_clip_cull_capture.
- 0026 (jm-v7 027): tessellated draws up to 1 << 18 patches (was 8192, the serial prefix sum). Workgroup prefix sum in `panlib_v7_tess_prefix()`; direct draws size the tessellator grid for their own patches. PanProbe large_draw tess_*.

## v7G-E2 (2026-10-21): 0035-0036

Made on top of the v7G tree HEAD (`121f9b1`, after 0049) in `/home/flux/src/mesa-v7fixE2` (branch `v7fixE2`), so like 0025/0026 they apply AFTER 0049, not in file order. Checked: v7G HEAD + 0035 + 0036 + 0025 + 0026 + 0070-0080 applies with `git am` (no conflicts).

- 0035 (jm-v7 035): the 0046 VS guard called `nir_def_rewrite_uses_after(&intr->def, phi)` with the intrinsic in the then block and the phi after the if; `is_instr_between()` assumes one block, so the phi's own source became the phi, `phi(phi, undef)` folded to undef and every guarded atomic returned garbage (also for non-indexed draws). vertex_stores iter0-5 (`outv` 1000) and the draw_params hang (`rec[undef]` store). Fixed by pointing the phi source back after the loop (0080's hunk unchanged).
- 0036 (jm-v7 036): the 0049 tess patch-ID path was gated on `info.system_values_read`, which nothing gathers on the v6/v7 GS NIR, so it was never emitted (gs_tess_primitive_id still returned triangle ordinals). Now gated on a `load_primitive_id` intrinsic being present.

## 0050-0051 (jm-v7 050/051, descriptor indexing, mutable, robustImageAccess2)

Base: the merged v7G tree (`mesa-v7G` 121f9b1 = jm-v7 049); worktree `/home/flux/src/mesa-v7DI` branch `v7DI-on-v7G`. Apply after 0049.

- 0050: update-after-bind and mutable bindings are bindless (`panvk_binding_is_bindless()`): storage images and texel buffers are decoded from set memory and accessed with LD_CVT/ST_CVT/global atomics (own bounds checks, OOB loads read the zero page through the view format); textures/samplers of a shader touching such a binding are indexed directly in set memory relative to a device-constant base (64 GiB below the device dummy sampler, new sysvals `desc.direct_idx[]`, `desc.dummy_sampler_idx`, `desc.zero_addr`), RSD texture/sampler count 0xFFFF. Mutable SSBOs fixed; v7 mutable lists exclude UBOs; the 048 robust image pass skips bindless images.
- 0051: exposes descriptorIndexing (all UAB bits, partially bound, update unused while pending), mutableDescriptorType, robustImageAccess2 by default (`PANVK_V7_RIA2=0` off), UAB limits 2^20 (`PANVK_DI_TEX_MAX` caps sampled images/samplers).
- Open hardware question: TEXC register indices above 65535 (vkd3d_heap image case at index 2^20-1). If it fails, retest with `PANVK_DI_TEX_MAX=65535`.

## v7H (2026-10-21): 0070-0080, perf/correctness review fixes

Base: the merged v7G tree (121f9b1, after 0049); worktree `/home/flux/src/mesa-v7perf` (branch `v7perf`). Apply after 0049 (strict `git am` checked with and without 0025/0026 in between; not checked against 0050/0051). Details and build: `worklogs/g615-dxvk/builds/2026-10-21-v7H-build.md`.

- 0019 amended (M14): `PANVK_DEBUG_RIA2 = 1 << 30`; it collided with BC_TIMING.
- 0070 (H3): depth bounds test enable dirties the FS RSD. 0071 (H4): query availability in a WC, GPU-cached BO per pool. 0072 (M4): 16 + 32 job indices per layer. 0073 (M5): 64-bit GS buffer sizing, OOM past 2 GiB. 0074 (M6): unpredicated xfb scan that passes the offsets through when the predicate fails.
- 0075 (H1): IDVS unless the VS feeds the emulation, writes memory, may be polygon-mode drawn, or on G72/G76 (`PANVK_V7_IDVS=0/1`). 0076 (H2) and 0077 (M3): `panvk_shader::alt` alternate compiles picked per draw (unlowered FS while depth bounds is off; screen-space VS while xfb is inactive). 0078 (M1): GS/xfb/restart buffers from gs_pool (`PANVK_V7_GS_DESC=1` = old). 0079 (M2): FS behind a GS discards only on the dist slots the GS writes. 0080 (M7): vertex store dedup with explicit deps, range clear, batched mark atomics, 0 from skipped atomics.

## v7J (2026-10-21): 0057-0058

Base: `mesa-v7I` HEAD f9b7291 (after 0056); worktree `/home/flux/src/mesa-v7J` (branch `v7J`). Apply in file order after 0056.

- 0057 (jm-v7 084): the 0056 txf-on-buffer bounds check inverted the hardware alpha also for the null texel buffer (0-element R32_UINT view), so robustness2 null_descriptors read a_txb=1.0. A 0-element view now reads (0,0,0,0).
- 0058 (jm-v7 085): texture/sampler indices that are divergent (divergence analysis) get a waterfall loop with all lanes active (ballot + read_invocation, uniform break, cap 32) before nir_lower_non_uniform_access. The texture unit takes one index per warp, also from inactive lanes, so the standard loop let lane 1 read lane 0's descriptor (vkd3d_heap image_heap output[1] = index 0 value at 500000 and 32767). `PANVK_TEX_WATERFALL=0` = old lowering.

## v7K (2026-10-21): 0059-0062, perf pass

Base: `mesa-v7J` HEAD f69289a (after 0058); same worktree `/home/flux/src/mesa-v7J`, new branch `v7K`. Apply in file order after 0058. Build record: `worklogs/g615-dxvk/builds/v7K.md`.

- 0059 (jm-v7 086): v6/v7 descriptor loads at a constant set offset are `ACCESS_NON_WRITEABLE | ACCESS_CAN_REORDER`, so CSE/GCM merge and hoist them. The 048 RIA2 pass queries the image size before every access, and each query was its own load_global (A06 bench: 16 loads, 409 vs 210 instructions, 75 vs 28 clauses, ria2 +35-38%). Dynamic-index descriptor loads stay pinned. `PANVK_V7_DESC_REORDER=0` = old loads.
- 0060 (jm-v7 087): the 085 waterfall only takes texture/sampler indices that are divergent AND NonUniform-qualified (non-qualified indices must be dynamically uniform; before 085 they were not lowered at all). copy_prop/algebraic/constant folding/DCE after the loops. `PANVK_TEX_WATERFALL=all` = 085 selection, `=0` = off.
- 0061 (jm-v7 088, review M9): BeginQuery no longer marks its batch after_frag, so render passes with an occlusion/statistics query keep PANVK_OVERLAP. End/timestamp/copy/reset unchanged; v9 unchanged. `PANVK_V7_QUERY_BEGIN_SERIAL=1` = old.
- 0062 (jm-v7 089, review M11): with customBorderColorWithoutFormat, R/B-swapped v7 images without SAMPLED/INPUT_ATTACHMENT usage keep AFBC. `PANVK_V7_CBC_AFBC=0` = old.

## v7L (2026-10-22): 0063+, perf review 2026-10-22

Base: `mesa-v7J` HEAD d160802 (after 0062); same worktree, new branch `v7L`. Apply in file order after 0062. Review: `worklogs/g615-dxvk/perf-review-2026-10-22.md`. Build record: `worklogs/g615-dxvk/builds/v7L.md`.

- 0063 (jm-v7 090, review item 5): the VS copy-desc job runs on every draw of a VS with storage images; the attribute table that holds the image entries is new per draw, so the second draw without a rebind read an unwritten table. Correctness fix, no toggle. PanProbe vertex_stores case `vs_img_redraw`.
- 0064 (jm-v7 091, review item 15): the compile knobs `PANVK_TEX_WATERFALL`, `PANVK_V7_IDVS` and `PANVK_V7_DESC_REORDER` move to `panvk_v7_knobs.h` (resolved once per process) and their resolved values are hashed into the shader cache key on v6/v7, so A/B runs no longer need `MESA_SHADER_CACHE_DISABLE`.
- 0065 (jm-v7 092, v6): IDVS off by default on every v6 GPU (G71 0x6000 was missing from the G72/G76 quirk list). `PANVK_V7_IDVS=1` opts in.
- 0066 (jm-v7 093, v6/v7): kbase atom waits compute the remaining timeout against CLOCK_MONOTONIC (the vk_sync deadline clock) instead of MONOTONIC_RAW, as v9 already did. `PANVK_V7_RAW_CLOCK=1` = old.
- 0067 (jm-v7 094, v6/v7): the non-coherent dma-buf import clean can run on v6/v7 with `PANVK_KBASE_IMPORT_SYNC=1`; default stays v9-only because the clean is a full DMA_BUF_IOCTL_SYNC of every listed BO per submit.
- 0068 (jm-v7 095, v6, compiler): `read_invocation` emits CLPER_OLD.i32 on BIFROST_LIMITED_CLPER GPUs (G71/G72/G31); it emitted CLPER.i32, which those GPUs lack (packer assert). This covers subgroupBroadcast, all shuffles/reductions lowered through it, and the 085 waterfall. Vote/ballot are WMASK (on all Bifrost), so no subgroup feature is gated on v6.
- 0069 (jm-v7 096, review item 3 parts 1-3): strided index min/max search (lane t reads t, t+n, ...; local reduction; one atomic pair per workgroup; at most 64 workgroups). One-workgroup searches store the result directly, so no WRITE_VALUE job. Direct DrawIndexed with <= 4096 indices and a VS without memory writes folds the search into the indexed-indirect helper (`panlib_draw_indexed_indirect_minmax_helper`): one job instead of three. `PANVK_V7_MINMAX=nofold` = separate search, `=0` = WRITE_VALUE + atomics always.
- 0070 (jm-v7 097, review items 4 and 6): identical descriptor rebinds are skipped; binds and pushes record the changed sets, and a draw rebuilds a stage's tables/dyn SSBOs/copy job only when one of its used sets changed (meta restore and other sources still mean every set). Push uniforms are uploaded only when dirty (layer id set for every layer incl. 0; indirect draws leave VS_PUSH_UNIFORMS dirty). Direct-draw attribute tables are reused while VI/VB/VS/padded count/instancing/raw offset are unchanged. `PANVK_V7_DESC_DIRTY=0`, `PANVK_V7_PUSH_CACHE=0`, `PANVK_V7_ATTR_CACHE=0` = old.
- 0071 (jm-v7 098, review item 9): rebinding a shader whose 076/077 alternate is bound is not a change (no VS/FS dirty flip per rebind). `PANVK_V7_ALT_REBIND_DIRTY=1` = old.
- 0072 (jm-v7 099, review K2): with PANVK_OVERLAP, v6/v7 get one 64 MiB GROW_ON_GPF tiler heap BO per half (was one 128 MiB BO, so half 1's first fault committed 64 MiB); hierarchy mask sized for the half. `PANVK_V7_SPLIT_HEAP=0` = old.
- 0073 (jm-v7 100, review item 10): occlusion EndQuery inside an open batch no longer closes/splits the batch; the availability write is deferred (up to 64 per batch) and flushed in an after-frag job at batch close. `PANVK_V7_QUERY_END_DEFER=0` = old.
