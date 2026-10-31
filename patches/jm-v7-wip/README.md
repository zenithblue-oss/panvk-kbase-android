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

## v7M (2026-10-22): 0074+, FTL G72 (v6) open items

Base: `mesa-v7J` HEAD a724b88 (after 0073); same worktree, new branch `v7M`. Apply in file order after 0073. Build record: `worklogs/g615-dxvk/builds/v7M.md`.

- 0074 (jm-v7 101, v6): robustImageAccess2 out-of-bounds alpha for format-less storage image loads and uniform texel buffer fetches comes from the view's hardware format on v6. The 048/083 "invert the hardware alpha" rule is v7 behaviour; on G72 the hardware alpha is nonzero for both rgba8 and r32ui, so all 10 r32ui OOB cases read alpha 0. New `image_deref_format` lowering in nir_lower_descriptors (v6/v7) reads the 22-bit format from the set (image word 2 >> 10, texel buffer word 4 >> 10); alpha is 1 when the v6 alpha swizzle is the constant 1. 084 null texel buffer rule kept. v7 unchanged.
- 0075 (jm-v7 102, v6, compiler): the Bifrost texture offset/MS-index staging word is packed with 32-bit AND/shift/OR on v6 instead of `pack_32_4x8(i2i8())` (8-bit vector ops such as MKVEC.v4i8). G72 sampler gather_offsets (dynamic textureGatherOffset) faulted with INSTR_INVALID_ENC; the constant-offset gather passed. Constant offsets fold to the same immediate. v7+ unchanged. Likely cause, to be confirmed on the device.

## v7N (2026-10-22): 0076, FTL G72 (v6) INSTR_INVALID_ENC

Base: `mesa-v7J` HEAD bd6d38d (after 0075); same worktree, new branch `v7N`. Apply in file order after 0075. Build record: `worklogs/g615-dxvk/builds/v7N.md`.

- 0076 (jm-v7 103, v6, compiler): `nir_intrinsic_ballot` no longer emits WMASK on v6. WMASK is a v7 instruction (it came with the full CLPER and shares its subgroup2/4/8 modifier). A host compile for gpu_id 0x6221 (`tools/bifrost-host-compile`) found WMASK.subgroup4 in all three FTL G72 event 0x51 shaders: shader_arith subgroup_size_control (reduce/scan active mask), sampler gather_offsets (waterfall over the dynamic offset) and vkd3d_heap image_heap (085 texture waterfall). It was in none of the passing shaders. On v6 the ballot is the OR of four CLPER_OLD reads of `src & (1 << lane_id)` (CLPER_OLD returns zero for an inactive lane). The packer asserts that no WMASK reaches a v6 clause. v7 (G52 host compile still WMASK.subgroup8 + CLPER.i32), G31 and v9+ are unchanged. This corrects the 0068 note that WMASK exists on all Bifrost.

## RC5 unified rebase (2026-10-22): 0006-0076 on the 203-patch series (csf-v11 through 208)

The base is now the full 203-patch `g615-v11-csf` series (csf-v11 through 208 = bc4, plus jm-v9 and jm-v7 001-005), not the 197-patch `ab7d630`. `scripts/prepare-rc5-tree.sh --mesa <5a07217 worktree>` builds it strictly; the tree is `/home/flux/src/mesa-rc5`, branch `rc5`, HEAD `4554612f`. Plan and checks: `worklogs/g615-dxvk/builds/rc5-unified-plan.md`.

- 0030 conflicted with csf-v11/205 (the `panvk_vX_device.c` include block). The resolution keeps `util/log.h`, `util/os_misc.h`, `util/u_debug.h`, in that order; there is no code change.
- 0010, 0013, 0035, 0043, 0045, 0046, 0047, 0048, 0050, 0051, 0052 and 0072 applied only with offsets, caused by the 205 lazy-arena code. They were re-exported with `git format-patch`, which changes only the hunk headers and index lines.
- Result: rc5 = v7N + exactly the csf-v11 203-208 delta (normalized diff check), and rc5-base = the bc4 build tree. These files no longer apply with zero offsets on `ab7d630`; use the git history for that base.

## RC5-universal-105 (2026-10-22): 0078, FTL G72 (v6) tess_perf OOM

Base: `mesa-rc5g` HEAD 3093e6a5 (csf-v11 018-219 + 0006-0077); commit 710db1f9. Run record: `worklogs/g615-dxvk/device-runs/2026-10-22-ftl-v6-starlte-rc5u/results.md`.

- 0078 (jm-v7 105, v6/v7): PanProbe tess_perf (256 tessellated GS draws per command buffer) got the S9 LMK-killed. Each nested tess draw had 96+96+24 MiB GS-emulation BOs (PANVK_TESS_GS_MAX_PRIMS) with a 1 MiB Bifrost up-front commit: 768 BOs, 768 MiB. (1) A nested tess draw the CPU knows caps max_indices (and so gs_max_prims) at patches x PANVK_TESS_MAX_PRIMS_PER_PATCH (8704) x vpp; the tessellator drops whole patches past it, so it cannot overrun. `PANVK_V7_TESS_GS_TIGHT=0` = old. (2) Bifrost JM GPU-only growable BOs under 64 MiB commit/grow in 1/64-size steps (64 KiB-1 MiB) instead of 1 MiB + 2 MiB; tiler heaps, tess arena and indirect varying buffer unchanged. `PANVK_V7_GROW_STEP=0` = old. Not device-tested.
- 0079 (jm-v7 106, v6/v7): tess_perf was still LMK-killed with 0078 (the up-front commit was only ~66 MiB). The JM tess memory grows and never shrinks. (1) The indirect varying allocator offset was never reset when a batch was re-issued, so every submit of the same command buffer moved further into the 288 MB GROW_ON_GPF buffer until all of it was committed. Batches now list their allocators and `panvk_reset_issued_batch()` resets them to 0. (2) Per heap half: a second indirect varying buffer and two 256 MiB growable tess GS scratch regions. Each batch gets its own allocator in its half's buffer, and nested tess eval draws bump-allocate their GS emulation buffers from the half's scratch, from 0 in every batch. (3) `PANVK_V7_TESS_BATCH_MB` (default 32, 0 = off) starts a new batch before a tess draw whose worst-case varyings would pass the budget, or when the GS scratch is past it. It also splits before a draw that would cross the ~5k-job split. (4) A draw the CPU knows sizes the tess arena (max_indices, max_patches) for its own patches. Not device-tested. Run record: `worklogs/g615-dxvk/device-runs/2026-10-22-ftl-v6-starlte-rc5u-105/results.md`.
- 0080 (jm-v7 107, v6/v7 and v9 JM): RC5-universal-106 on the FTL S9 was LMK-killed while recording tess_perf gs=0 L=1. MemAvailable was 1.58 GB with 24 MB RSS, then about 1.5 GB was consumed in 4 s, with no new growable BOs. The 0079 budget split every 16-patch draw into its own batch (21.5 MiB worst-case varyings), and every JM batch allocated its own fully committed TLS (stack for all threads of all cores) and WLS buffers. Batches now share one TLS and one WLS buffer per heap half per command buffer; same-half batches are ordered. `PANVK_V7_TLS_SHARE=0` = old. The first 4 TLS/WLS sizes are logged. 0079 review fixes: picking the heap half early keeps `batch->tiler.ctx` (layered/multiview rendering), and nested tess draws with xfb active keep gs_pool buffers. Not device-tested. Run record: `worklogs/g615-dxvk/device-runs/2026-10-22-ftl-v6-starlte-rc5u-106/results.md`.

## RC5-universal-108 (2026-10-22): 0081-0082, FTL Galaxy A31 (v7) fill_mode and timestamps

Base: `mesa-rc5g` HEAD 28b0346c (after 0080); commits 7b3eef67 (0081) and 03b39a69 (0082). Run record: `worklogs/g615-dxvk/device-runs/2026-10-22-ftl-v7-a31-rc5u-107/results.md`.

- 0081 (jm-v7 108, v6/v7): fill_mode failed 13/17 on the A31 (every LINE case drew nothing). jm-v7 075 keeps no_idvs for a VS that fillModeNonSolid may draw in LINE/POINT mode, but `panvk_hash_state()` hashed polygon mode only on v9+, so a LINE pipeline got the FILL pipeline's IDVS binary from the disk cache (or the in-memory pipeline cache while the FILL pipeline is alive; both use the same `shader_key`). New `panvk_v7_vs_may_draw_nonfill()` is the single predicate: the compile decides IDVS with it and the hash includes it for vertex stages (outside `state != NULL`, as the compile treats no state as any polygon mode). The v6/v7 knob-set tag goes 0x76370091 -> 0x76370108, so stale entries miss. v9+ hash unchanged; v6 binaries unchanged (IDVS off since 092), only its keys change. Check: fill_mode 17/17 on a v7 device with a warm cache (run it twice).
- 0082 (jm-v7 109, kbase JM): bc_perf GPU timestamps were 2.0x the wall time on the A31 (MT6768): `timestamp_frequency` was cntfrq_el0 (13 MHz) and the GPU TIMESTAMP counter runs faster there. At device creation three `KBASE_IOCTL_GET_CPU_GPU_TIMEINFO` samples (GPU TIMESTAMP + kernel monotonic) 5 ms apart, polled every 100 us so the GPU stays powered, give two rate estimates; the measured rate is used only if both agree within 2 %, it is in 1 MHz-1 GHz and more than 10 % above cntfrq (snapped to cntfrq x 2^k within 3 %). Otherwise cntfrq is kept, so devices where they agree (A06, S9) are unchanged. `PANVK_KBASE_TS_CALIBRATE=0` = old, `=1` = also CSF and also a slower counter; `PANVK_KBASE_TS_CAL_MS` = window. Not device-tested; look for the `kbase: timestamp calibration` / `GPU timestamp counter measured` log line.
- tess_perf teardown RSS (+23-26 MB per pass on the A31): no driver leak found. `indirect_varying_buffer2` and `tess_gs_scratch[0/1]` are unreffed in device destroy (and the error path); the 0080 `tls_half` pointers borrow tls_pool BOs, which go with the command buffer/pool. `device/tess-perf.c` `teardown()` destroys only the device and instance (no command pool, buffer or memory frees), so the residue is most likely those app objects (still mapped). Not fixed; to confirm, add the frees to the harness and compare.

## RC5-universal-109 (2026-10-22): 0083-0084, FTL S9 (v6) timestamp period 0 and the JM warm-up sleep

Base: `mesa-rc5g` HEAD 03b39a69 (after 0082); commits 8279fc69 (0083) and 1bdf1e0c (0084). Run records: `worklogs/g615-dxvk/device-runs/2026-10-22-ftl-v6-starlte-rc5u-108/results.md` (0083) and `worklogs/g615-dxvk/device-runs/2026-10-22-ftl-v7-a31-rc5u-108/results.md` (0084).

- 0083 (jm-v7 110, kbase JM and CSF): on the FTL Galaxy S9 (Exynos 9810) cntfrq_el0 reads 0, and 0082's `kbase_calibrate_timestamp_frequency()` returned silently on `!timestamp_frequency`, so timestamps were advertised (64 valid bits, timestampComputeAndGraphics) with timestampPeriod 0 (bc_perf `ts_period=0.000`). With cntfrq 0 the rate is now measured with the 0082 samples, for any `PANVK_KBASE_TS_CALIBRATE` except 0 and on CSF too. A failed or unstable measurement is retried once with a 4x window. The result snaps to the nearest of 13, 19.2, 24, 25, 26, 38.4, 50, 52 or 100 MHz within 3 % (else it is rounded to 1 kHz) and is logged at warning level (`cntfrq_el0 is 0; GPU timestamp counter measured at ...`). With no stable rate, `gpu_can_query_timestamp` is cleared: timestampValidBits 0, timestampComputeAndGraphics false, and no calibrated_timestamps, shader_clock or present_timing (log: `... timestamps not advertised`). Every early return now logs one info line. Devices with a nonzero cntfrq behave as in 0082. `PANVK_KBASE_TS_CALIBRATE=0` = old (period 0 on the S9). Check on the S9: the `cntfrq_el0 is 0` line (26 MHz expected), bc_perf `ts_period=38.462`, `upload_gpu_ms` about 0.9-1.0x `upload_wall_ms`.
- 0084 (jm-v7 111, kbase JM, v6/v7/v9): jm-v9 001 (ported from FristOneRR efd07ba) slept 2 s after the atom layout probe on every JM device open (A31: 2.0 s on each of 94 opens, about 3 min of a 463 s run). No reason was found in the patch, the worklogs or the git history. The probe atoms are dependency-only and are completed (or refused with EINVAL) before the sleep, and the Bifrost stride checks after it submit more such atoms without delay, but a kernel race cannot be ruled out. So the delay is kept, only for the first JM open per process; concurrent opens wait on a mutex until it is done. `PANVK_KBASE_JM_WARMUP`: unset/1 = once per process, 2 = every open (old), 0 = never; `PANVK_KBASE_JM_WARMUP_MS` = length (default 2000). Each open logs `kbase: JM warm-up ...`. PanProbe runs each test in its own process with about 2 opens each, so this removes about half the sleeps in a PanProbe run. Check: no new DEVICE_LOST or job faults on the second open in a process; if there are, run with `PANVK_KBASE_JM_WARMUP=2` to confirm.
- Neither patch is device-tested.

## RC5-universal-110 (2026-10-23): low-risk perf round (perf review 2026-10-22), csf-v11 220-222 and 0085-0089

Base: Mesa 5a07217f + the series with csf-v11 018-222, then 0006-0089. `prepare-rc5-tree.sh --csf-max 222 --wip-max 0089`. Review: `worklogs/g615-dxvk/perf-review-2026-10-22.md` (status list at the top).

New csf-v11 patches (common/CSF code, applied before jm-v9):
- 220: vkResetCommandPool(RELEASE_RESOURCES) frees the pool's BO free lists after the command buffers are reset (before, only tls_big_bo_pool was cleaned, and too early), and a new `panvk_TrimCommandPool` frees them and the cached push sets. `PANVK_CMD_POOL_TRIM=0` = old. (K6)
- 221: `panvk_pool_flush_maps()` cleans the transient BO only up to `transient_offset` (64-byte aligned); other BOs stay whole. `PANVK_POOL_FLUSH_FULL=1` = old.
- 222: the two per-dma-buf-import info lines go through `mesa_log(MESA_LOG_DEBUG)` (shown with `MESA_LOG_LEVEL=debug`), the CSF "wait completed successfully" line is `mesa_logd`. Same line count, so no later hunk moves.

Refreshes: `jm-v9/024` (its context had the two import log lines; context only, edited in place) and `0022` (hunk headers only, `git am -3`, after 220 moved panvk_cmd_pool.c). No other patch drifted.

New jm-v7-wip patches:
- 0085 (jm-v7 112): the v6/v7 gs_bo_pool free list is released by 220 too. Knob as 220.
- 0086 (jm-v7 113, kbase JM): the per-batch BO list (every pool BO, O(n^2) dedup) is gone; kbase JM atoms have no BO list and `kbase_atom_slot::bos` had no reader. No knob. (K7)
- 0087 (jm-v7 114, kbase JM): `kbase_drain_events()` reads up to 16 events per `read()` and stops after a short read (kbase_read() returns once the queue is empty). `PANVK_KBASE_EVENT_BATCH=0` = old. The multi-atom JOB_SUBMIT half of K8 is not done.
- 0088 (jm-v7 115, v6/v7): the growable BO line is info only for the first 8 per process, then debug.
- 0089 (jm-v7 116, v6/v7): bindless texel buffers store the element count in the descriptor's pad word (`write_buffer_view_desc()`), and `build_bindless_buf_texel()` reads it instead of a software `udiv` per access. `PANVK_V7_TEXEL_WIDTH=0` = old; it is hashed, and the knob-set tag is now 0x76370116. (item 14, divide half)
- Skipped: item 8 (lazy depth-bounds compile), K4 (BO memset; kbase does not guarantee zeroed pages for reused context-pool pages, and csf-v11 041 exists for zero_initialize_device_memory). Reasons in the review status list.
- None is device-tested.

## RC5-universal-111 (2026-10-23): 0090-0092, FTL Pixel 6 (v9, Mali-G78) tess_perf and large_draw

Base: `mesa-rc5g` HEAD 6cae57f8 (after 0089); commits bc83c08a (0090), 7dd18a82 (0091), 0f470d21 (0092). Run record and diagnosis: `worklogs/g615-dxvk/device-runs/2026-10-23-ftl-v9-oriole-rc5u-110/results.md`. All three are v9 only (`PAN_ARCH == 9` code), so v6/v7/v10/v11 behaviour is unchanged.

- 0090 (jm-v7 117, v9): tess_perf gs=1 L=16 failed with `kbase: JM atom 25 failed: event=0x60` (BASE_JD_EVENT_OUT_OF_MEMORY, tiler heap exhausted) on the vertex/tiler atom (core_req 0x16). The fragment atom and the next vertex/tiler atom failed through their dependencies. Valhall JM MALLOC_VERTEX jobs take every vertex packet from the tiler heap, and the pass-through GS output (about 7M unshared vertices per frame) did not fit in the 128 MiB heaps. Each v9 heap is now 512 MiB (GROW_ON_GPF, 2 MiB steps, so VA, not memory). `PANVK_V9_TILER_HEAP_MB` = size (64-1024, 128 = old). Logs `panvk: v9 tiler heaps 2 x 512 MiB (GROW_ON_GPF)`.
- 0091 (jm-v7 118, v9): large_draw base_inst_direct/_windows/_fan_windows. The v9 CPU instance splits and fan windows drew chunks with `instance.base = firstInstance + offset` (the bug csf-v11 191 fixed on CSF), which shifted gl_BaseInstance and the instance-rate attribute divisor. The chunks now keep firstInstance, and the offset goes to the lowered VS as `params.chunk_instance` (`cmdbuf->state.gpu_prerast_inst_base`). `PANVK_V9_SPLIT_BASE_INST=0` = old.
- 0092 (jm-v7 119, v9): large_draw fan_indirect/fan_indexed_indirect wrote nothing. `panlib_v9_prerast_indirect()` NULLs a draw over the record capacity, and fans are not in the v9 chunk loop. A new `panlib_v9_fan_clamp_cmd` (fan_chunks.cl, v9 only) gives the queries and the lowering a copy of the indirect command clamped to the capacity, like csf-v11 195. `PANVK_V9_FAN_CLAMP=0` = old.
- None is device-tested. Check on a v9 device: 39/39, the heap line, tess_perf gs=1 hashes equal to gs=0, large_draw fan_indirect `65534 of 149999`, base_inst bad=0.

## RC5-universal-112 (2026-10-24): csf-v11 223-229, Fallout 4 memory and the black head

Base: Mesa 5a07217f + the series with csf-v11 018-229, then 0006-0092. `prepare-rc5-tree.sh --csf-max 229 --wip-max 0092`. Analysis: `worklogs/g615-dxvk/fo4-memory-analysis-2026-10-23.md` (fixes 1, 3-6, 8, 9 and black-head cause 2). No new jm-v7-wip patch, and no later patch needed a refresh: the edits in files that jm-v9/jm-v7/jm-v7-wip also touch are either line-neutral or below every later hunk.

New csf-v11 patches:
- 223 (all, report v10+): `PANVK_DEBUG=mem` / `PANVK_MEM_LOG=<path>` (appended, `%p` = pid; else mesa_logi). Every private BO is counted by kind (dev_pool, shader, cmd_cs, cmd_desc, cmd_tls, dev_tls, desc_pool, bc_raw, utrace, other_pool, misc + gpu_prerast arenas), with VkDeviceMemory, kbase tiler heap contexts (live, peak, HEAP_INIT bytes, renewals, sync TERMs, TERM backlog peak), BC raw store events, VmRSS/RssAnon/RssFile/VmSwap and MemAvailable. One `panvk-mem` line per `PANVK_MEM_LOG_MS` (default 10000) from the CSF submit path, `panvk-mem-final` at queue destroy. Off by default. The heap chunks that kbase adds on tiler OOM are not visible from user space: total kbase memory minus `devmem + priv` (read `/proc/mtk_mali/gpu_memory` from adb on MediaTek) is the tiler heap growth.
- 224 (v10+ kbase): `PANVK_KBASE_HEAP_CHUNKS=<initial>[,<max>]`, default 4 initial chunks (was 10) and max unchanged (400); `PANVK_KBASE_HEAP_TERM_MAX`, default 2 retired heaps queued for TERM on the heap thread (was 16), TERM on the submitting thread beyond it (0 = always). Old: `PANVK_KBASE_HEAP_CHUNKS=10,400 PANVK_KBASE_HEAP_TERM_MAX=16`. Logs `kbase: tiler heaps N initial / M max chunks of K KiB`.
- 225 (v10+ BC lowmem; the shared meta checks are false on v6/v7/v9): BC raw stores. (1) A shadow-only BC -> BC copy marks a destination that has raw blocks `bc_raw_stale`; raw reads of it take the lazy path until a raw write covers the whole image. This removes the zero-block decode of a twice-relocated texture (black head cause 2). `PANVK_BC_RAW_STALE=0` = old. (2) No device-wide switch to raw stores at the first skipped lazy read; skips are counted and logged 4 times, then at powers of two. `PANVK_BC_RAW_FLIP=1` = old. (3) No pinning after 2 spill read-backs; `PANVK_BC_RAW_PIN=<n>` (2 = old, default 0 = never).
- 226 (v10+): command pool auto-trim on `vkResetCommandPool(flags = 0)` (DXVK never passes RELEASE_RESOURCES or trims, so 220 did nothing there): each BO free list keeps what the reset command buffers gave back plus a quarter (at least 2) and frees older BOs beyond that. `PANVK_CMD_POOL_AUTOTRIM=0` = old.
- 227 (v10+): outgrown device TLS BOs (097) are freed when the last command buffer that used them is reset; command buffers hold a reference. `PANVK_TLS_FREE_OLD=0` = old.
- 228 (v10+ kbase): 64 KiB u_trace heap on kbase without `MESA_GPU_TRACES` (was 10 MiB; kbase never processes u_trace). `PANVK_UTRACE_CLONE_MEM_SIZE=0xa00000` = old.
- 229 (all, opt-in): `PANVK_MEM_BUDGET=honest` (heapUsage also counts private BOs, budget computed from it) and `PANVK_MEM_BUDGET_HEADROOM_MB=<n>` (budget at least usage + n MiB). Default unchanged.
- Skipped (risk): lower tiler heap max (incremental-render OOM path), re-armed chunk recycling (098 DEVICE_LOST history), on-demand descriptor pool blocks, freeing raw BOs at GPU completion, lazy BC6H/BC7 raw stores.

## RC5-universal-113 (2026-10-25): csf-v11 230-231, Fallout 4 device TLS and command pool churn

Base: Mesa 5a07217f + the series with csf-v11 018-231, then 0006-0092. `prepare-rc5-tree.sh --csf-max 231 --wip-max 0092`. Source: the 112 FO4 memory log (`worklogs/g615-dxvk/device-runs/2026-10-24-g615-rc5u-112/raw/fallout4/panvk-mem-27889.log`). No new jm-v7-wip patch and no refresh: 230 touches only CSF files and `panvk_priv_bo.c/.h` (no later patch touches them); 231's `panvk_cmd_pool.c` edits are line-neutral above the 0022/0085 hunks or below them, and the new `panvk_cmd_pool.h` field sits after the 0022 context.

New csf-v11 patches:
- 230 (v10+ CSF; kbase for the sparse part): device TLS backed only for present shader cores, shrunk after idle. The device TLS is pow2(stack per thread) x THREAD_TLS_ALLOC x core-ID range. FO4's 336 MiB TLS is 21 x 16 MiB: a 21-ID range (sparse Valhall core mask) for 6 present cores, so 15 of 21 per-core slabs were committed for cores that do not exist. `PANVK_TLS_SPARSE=1` (default): one BO with a slab per present core plus a `KBASE_IOCTL_MEM_ALIAS` region over the whole core-ID range (present cores map their own slab, absent core slots alias the first slab), so the GPU layout is unchanged and every address is mapped; any failure falls back to the full BO. `PANVK_TLS_SHRINK_MS=<ms>` (default 10000): after that long without a command buffer needing more than half of the current device TLS, one needing a quarter or less gets a new smaller BO (command buffers keep their 227 references). New info line `panvk: device TLS <B>/thread x <threads> threads x <n> core IDs (<c> cores, mask ...) = <VA> MiB of VA, <backed> MiB backed`. Old: `PANVK_TLS_SPARSE=0 PANVK_TLS_SHRINK_MS=0`.
- 231 (v10+, 226's arch check): command pool auto-trim hysteresis. 226 kept only the last reset's count + 25 %, so varying recordings freed and re-created BOs every reset (656k BOs / 41.9 GB in 6 min of FO4, ~2500 BOs / 10 s in the menu with cmd_cs flat). Each free list now keeps the largest count of the current and previous window of `PANVK_CMD_POOL_AUTOTRIM_WINDOW` resets (default 64) + 25 % (at least 2). `PANVK_CMD_POOL_AUTOTRIM_WINDOW=0` = 226, `PANVK_CMD_POOL_AUTOTRIM=0` = before 226. The pool is now zero-allocated.
- Assumption to check on device (230): the hardware finds a thread's stack by raw core ID with per-core slabs of THREAD_TLS_ALLOC x stack (what the core_id_range sizing already assumes). A wrong assumption shows as wrong tessellation/tess_perf hashes (libpan kernels with 2-4 KiB stacks), not as faults; `PANVK_TLS_SPARSE=0` is the A/B.

## RC5-universal-114 (2026-10-25): csf-v11 232, fix for the 113 sparse-TLS regression

Base: Mesa 5a07217f + the series with csf-v11 018-232, then 0006-0092. `prepare-rc5-tree.sh --csf-max 232 --wip-max 0092`. No new jm-v7-wip patch and no refresh (232 touches only `csf/` files).

113 failed gs_tess_primitive_id on FTL panther (v10) and shiba (v11): the three 600-patch GPU chunk cases had ~300 bad patch IDs, the counts changed between two submits of the same command buffer, and there was no fault (`device-runs/2026-10-25-ftl-v11-shiba-rc5u-113/results.md`).

Root cause (230 sparse TLS, by code): the sparse BO's alias region has one slot of THREAD_TLS_ALLOC x (BO per-thread size) per core ID, and absent-core slots alias slab 0. The hardware places core c's stacks at c x THREAD_TLS_ALLOC x (stack size in the job's TSD). Two kinds of TSD used a smaller stack size than the BO was built for:
- per-job TSDs (`cmd_dispatch_prepare_tls()`: dispatch, precomp, and the gpu_prerast VS/GS/TCS compute jobs) are packed with their own shader's stack size and only take the TLS pointer from the command buffer's descriptor at run time;
- the command buffer's descriptor carries the command buffer's maximum stack size, but the device BO can be bigger than that.
With a smaller stride, present cores land in absent-core slots (all slab 0) or in another core's slab, so threads on different cores share stacks. The chunk path puts the 4096 B/thread gpu_prerast kernels and smaller-stack jobs in one command buffer, and the 24-prim cases do not. A full BO (112) only uses a prefix for a smaller stride, so this never showed before.
- Not shrink: the failing process created exactly one device TLS (4096 B) and ran for 0.25 s, and a shrink needs 10 s idle. The TLS pointer and size are written at EndCommandBuffer, and the command buffer holds a reference (227), so a resubmit (chunk-replay) always sees the BO it was recorded with.
- Not 231: it only trims the pools' free lists at vkResetCommandPool and never touches memory that is in use.

- 232 (v10+ CSF): with `PANVK_TLS_SPARSE` (still default 1), emit_tls writes the sparse BO's per-thread size into the command buffer's descriptor when that size is larger, and the per-job TSD copy (now one helper, `panvk_per_arch(cmd_copy_tls_to_tsd)`) also copies word 0 (TLS Size, the only field in that word). Every job then uses the stride the BO was built for. A stack size larger than a shader needs is always valid, and the BO is sized for it. `PANVK_TLS_SPARSE=0` = the 112 layout and the 112 TSD copy. `PANVK_TLS_SHRINK_MS` and 231 are unchanged. v6/v7/v9 are unchanged.
- FTL RC5-universal-114: shiba (v11) and panther (v10) reached 39/39 with sparse TLS on, tess_perf hashes are equal to 112, and there were no faults.

## csf-v11 233 (2026-10-26): PANVK_FPS_LOG present counter (test infrastructure)

Base: rc5g-114 + 233 (worktree `/home/flux/src/mesa-rc5-fps`, branch `fps-log`, `3071c6df`). `prepare-rc5-tree.sh --mesa /home/flux/src/mesa-rc5-fpsv --csf-max 233 --wip-max 0092` on a fresh 5a07217f worktree printed `BASE-OK applied=228 csf-v11<=233 head=5e2321d2` and `OK base=228 jm-v7-wip=87 head=bf97524f`, with no drift, offset or fuzz (only the old jm-v9/002 whitespace warning). Its tree `dee163e5` equals the fps-log HEAD tree. No jm-v7-wip refresh is needed, because 233 only touches `panvk_wsi.c` and no later patch changes that file. The 115 patches start at 234.

233 is **append-only**: one block at the end of `panvk_wsi.c`, with its own includes, and no existing line changed or moved (the first version, `3a369f2d`, which added includes at the top and a call in `panvk_wsi_init`, was replaced before any build used it). So 115 patches that change `panvk_wsi.c` (the P-C present path) and were made on a tree without 233 still apply with no offset. The one exception is a hunk that itself appends at the end of `panvk_wsi.c`: a hunk with no trailing context must match at end of file, so regenerate that one on top of 233.

- 233 (all archs, JM and CSF; the hooks sit above the per-arch code): `PANVK_FPS_LOG=<path>` (appended, `%p` = pid) and `PANVK_FPS_LOG_MS` (default 1000, minimum 100), both read once per process at the first present (or swapchain destroy), so nothing runs at instance or device init. It counts the presents that queued an image: `vkQueuePresentKHR` through wsi_common (X11 swapchain of PanPlay's Wine games, Wayland, the PanVK Android surface swapchain of PanProbe) and `vkQueueSignalReleaseImageANDROID` (the Android loader's swapchain). Per interval: `fps t_ms=<ms since first present> frames=<n> fps=<x.x> avg_ms=<x.xx> max_ms=<x.xx> total=<N>`. At `vkDestroySwapchainKHR` and at exit / driver unload (atexit): `fps-summary ... reason=swapchain|exit` over the whole run, after the interval in progress has been flushed. The file is opened at the first present, header `# panvk-fps <driverInfo> pid= interval_ms= first_present_unix_ms=`. If the open fails, the lines go to mesa_log. Unset = after that first read, the present path reads one int (relaxed atomic) and calls wsi_common (no getenv in hot paths). Set = one clock read and one mutex per present. Hooks: strong `panvk_QueuePresentKHR` / `panvk_DestroySwapchainKHR` / `panvk_QueueSignalReleaseImageANDROID`, which fill those dispatch slots before `wsi_device_entrypoints` and vk_common (`panvk_device_entrypoints` is added first without overwrite). In the build, the table slots point at the hooks. Device destroy is not hooked (that needs `panvk_vX_device.c`); the exit summary covers it.
- Validation: a host unit test of the counter (gcc, fake clock: 60, 30 and 45 fps, a 100 ms stall, failed presents not counted, two swapchains, the Android path, exit summary; with the variable unset: no file and no state). The `fps` lines add up to the summary. Private build **RC5-universal-114-fps** (not a release): `/home/flux/build-out/2026-10-26-rc5-universal-114-fps-private/libvulkan_panfrost.so`, 30674016 bytes, sha256 `51f6fdbe2a34237b6a765981bf3c17f7f645e5a85752b8053dce3ad0c4b1fd27`, `PanVK-kbase RC5-universal-114-fps (Mesa 26.3.0-devel (git-3071c6dfa2))`, with the same toolchain and flags as 114 and no new warnings. No device run. Harness and fps method: `worklogs/g615-dxvk/panplay-firebase-cubes.md`.

## Build 115 (2026-10-26): 0093-0131, staging-115 promoted

The six `patches/staging-115` packages are now part of this queue, unchanged (`git mv`, same bytes), in their apply
order. So `scripts/prepare-rc5-tree.sh`, whose defaults are now `--csf-max 233 --wip-max 0131`, builds the build 115
release tree, which is the `p115-fix` tree (`213079bc`). The design notes stay in `patches/staging-115/<pkg>/NOTES.md`
(knob tables, 114 values, risks); each one now starts with its new numbers.

| jm-v7-wip | was staging-115 | what |
|---|---|---|
| 0093-0102 | pa 0001-0010 | P-A: CSF CPU record/submit (FS idle clean, descriptor dirty per stage, VB dirty compare, gpu_prerast fast skip, indirect re-emit fix), depth bounds FS, kbase device clock, descriptor pool hint |
| 0103-0112 | pb 0001-0010 | P-B: memory (eager retired-heap TERM, 8 MiB tess VS region, zero only what must be zero, CS chunk flush and 4 KiB chunks, JM pool auto-trim, pressure-aware budget, slab caches; opt-in evictable slabs and heap slots) |
| 0113-0119 | pc 0001-0007 | P-C: present paths (cached sw readback, deferred MIT-SHM reply, Android present fence and thread), INTERSECT ZS preload on v9/v10, preload barrier tracking |
| 0120 | int 0001 | driverInfo like upstream Mesa, `Mesa 26.3.0-devel (git-<sha>)` |
| 0121-0127 | pd 0001-0007 | P-D: JM v6/v7/v9 (FS copy job once, one pool clean per resubmit, tess batches, v9 lazy prerast arenas, JM warm-up off, TLS/WLS no CPU clear) |
| 0128-0131 | fix 0001-0004 | PanPlay on Pixel 8: HOST_COHERENT dma-heap BOs from `system-uncached` on coherency mode 31 (`PANVK_KBASE_UNCACHED_HEAP`), no RandR on an X server without RANDR and no present timing on the CPU copy path, two opt-in diagnostics (`MESA_VK_X11_SW_TIMING`, `PANVK_KBASE_WAIT_STATS`, `MESA_VK_WSI_CREATE_LOG`) |

Every default-on behaviour change has a knob that restores 114 (the NOTES tables); PanProbe's `assets/env-ab.json`
pass `114-knobs` sets them (27 keys; `PANVK_ZS_PRELOAD_ALWAYS=1`, the 114 value on v9/v10 only, is left out, see that file).

Strict check on a fresh 5a07217f worktree (`/home/flux/src/mesa-115-rel`, branch `p115-rel`):
`sh scripts/prepare-rc5-tree.sh --mesa /home/flux/src/mesa-115-rel --csf-max 233 --wip-max 0131` printed
`BASE-OK applied=228 csf-v11<=233 head=6f7117ff` and `OK base=228 jm-v7-wip=126 head=9dde9e7a`, with no PATCH-DRIFT, WIP-DRIFT, offset or fuzz lines (only the old jm-v9/002 trailing-whitespace
warning). Its tree `213079bc` equals `p115-fix`'s (`git diff p115-fix HEAD` is empty). Build 115 is built from this tree:
driverInfo `Mesa 26.3.0-devel (git-9dde9e7a18)`.

## Build 115b (2026-10-27): 0132-0135, Pixel 7 (v10) hang, a31 (v7) device lost and placed map

| jm-v7-wip | what |
|---|---|
| 0132 | JM: a tiling (IDVS) job depends on its descriptor copy / indirect draw helper job. `pan_jc_add_job()` replaces a tiling job's global dependency with the previous tiler job, so the producer goes in as the local dependency |
| 0133 | kbase: without a dma-heap or ION heap (a31, MediaTek 4.14), host-visible BOs come from gralloc (BLOB AHardwareBuffer; the handle fd that is a dma-buf, fd 1 there), so placed maps (WoW64 32-bit, D3D9) work |
| 0134 | CSF on kbase: a stream lets its own deferred syncs land (`cs_wait_slot(DEFERRED_SYNC)`) before it waits on another subqueue. Pixel 7 D3D11 hang: VT and COMPUTE waited in a barrier on FRAG while FRAG waited for VT's deferred tiling-done signal (each subqueue is its own CSG on kbase) |
| 0135 | JM: a draw without vertex buffers is not out of memory. The zero-size helper info allocation (NULL) made `panvk_draw_prepare_vs_attribs()` return before it set the VS image attribute table, and the descriptor copy job wrote to GPU address 0 (a31 D3D11/D3D9 `event=0x4`, DEVICE_LOST) |

Evidence: `worklogs/g615-dxvk/device-runs/2026-10-27-ftl-v10-panther-115b-diag{4,5}-panplay/` and
`2026-10-27-ftl-v7-a31-115b-diag{,4,6}-panplay/`. `scripts/prepare-rc5-tree.sh` defaults are now `--csf-max 233
--wip-max 0135`. Strict check on a fresh 5a07217f worktree (`/home/flux/src/mesa-115b-final`, branch
`p115b-final`): `BASE-OK applied=228 csf-v11<=233 head=c2c80109` and `OK base=228 jm-v7-wip=130 head=ad5cd691`, with
no drift, offset or fuzz lines (only the jm-v9/002 trailing-whitespace warning). Its tree `927190bb` equals the fix
branch `p115b-fix2`'s. driverInfo `Mesa 26.3.0-devel (git-ad5cd69141)`.

## Build 115c (2026-10-28): 0136-0139, D3D12 feature level device losts (Pixel 7, Pixel 8, a31), gears, logic ops

| jm-v7-wip | what |
|---|---|
| 0136 | CSF: the tiler takes the bins of every layer of a tiler context from one heap chunk, and v10/v11 need twice `pan_calc_bins_pointer_size()` per layer. vkd3d-proton's passes without attachments (65536x65536 on v11, 32768x32768 on v10, 256 layers: D3D12 FL 11_1 `tir`, FL 12_1 `conservative`) kept mask `0x1fe0` (171 KiB per layer), the tiler OOMed on every new 1 MiB kbase chunk and kbase terminated the group (DEVICE_LOST). The budget is now (chunk - 64) / (2 x layers of the context) on v10/v11 (/ layers on v12+, not measured). Measured on the Pixel 8 with a chunk size override: `0x1fe0` OOMs with 1 and 2 MiB chunks, not 4 or 8 MiB; `0x1fc0` (43 KiB) OOMs with 512 KiB, not 1 MiB. `PANVK_TILER_LAYER_BUDGET=0` = 115b |
| 0137 | JM: per-instance attribute offsets (firstInstance x stride, the raw vertex offset undo, the indirect draw helper strides) use the bound vertex stride (`dyns->vi_binding_strides`), not the pipeline's. vkd3d-proton creates pipelines with stride 0 and binds the strides dynamically: gears drew all three gears with instance 0's colour and transform (a31: "red gear twice") |
| 0138 | A logic op reads the destination (gallium's `load_dest`): `any_dest_read`, so no forward pixel kill (and no v13 HSR cull). vkd3d-proton's logic op draw forward-killed the draw under it and INVERT read the clear value (D3D12 FL 11_1 `logic_op` `ffffffff` on every arch; DXVK's pixel shaders discard for cull distances, which already disabled FPK there) |
| 0139 | Bifrost scheduler: the message descriptor of ST_CVT, ST_TILE and TEXC is IR src 3 (behind the staging source), not src 2; a same-tuple FMA result there (`t`, ISA mask `0xf7`) is not encodable. vkd3d-proton typed UAV stores compute the conversion descriptor at run time, the scheduler put that LSHIFT_OR in the FMA slot: `+ST_CVT.v4 t1, r0, r1, t(INVALID)`, INSTR_INVALID_ENC (a31 `event=0x51`, D3D12 FL 12_0 `typed_uav_load` DEVICE_LOST). src 3 is now excluded as for BLEND (src 2 stays excluded); test in `test-scheduler-predicates.cpp` |

Evidence: `worklogs/g615-dxvk/device-runs/2026-10-28-ftl-v11-shiba-115c-diag{,3}-panplay/` (tiler budget A/B and
chunk sizes) and `2026-10-28-ftl-v7-a31-115b-shaders-panplay/` (Bifrost disassembly of the failing shader, logic op
and gears). Not a driver bug: D3D11 FL 11_1 `tir` (DXVK 3.1.1 hands the UAV-only draw to the driver with
`rasterizationSamples` 1, diag3). `scripts/prepare-rc5-tree.sh` defaults are now `--csf-max 233 --wip-max 0139`.
Strict check on a fresh 5a07217f worktree (`/home/flux/src/mesa-115c-final`, branch `p115c-final`): `BASE-OK
applied=228 csf-v11<=233 head=02fe7ef8` and `OK base=228 jm-v7-wip=134 head=15fd70c7`, with no drift, offset or fuzz
lines (only the jm-v9/002 trailing-whitespace warning). Its tree `e945011a` equals the fix branch `p115c-fix`'s
(`/home/flux/src/mesa-115c-fix`). driverInfo `Mesa 26.3.0-devel (git-15fd70c7c4)`.

## Build 115d (2026-10-29): 0140, BC lowmem transcoder compile time; D3D11 `tir` is a DXVK bug

| jm-v7-wip | what |
|---|---|
| 0140 | `nir_opt_large_constants` (256-byte threshold) in `panvk_preprocess_nir()` before the first `nir_opt_copy_prop_vars`. The BC lowmem ETC2/EAC transcoder (csf-v11 197, grown by 200/203/206/218) has global const LUTs of 1152, 512 and 128 entries; `nir_lower_variable_initializers` makes them per-element stores and copy_prop_vars tracked each store against each load in every optimization loop (`nir_compare_deref_paths`): 1.4 s to 4.3 s of CPU per transcode pipeline on the G615. PanProbe `bc_decode` without a shader cache: rc4 0.7 s, 115c 15.3 s, 115d 5.0 s (16/16 PASS; warm cache 0.2 s). `bc_perf` upload GPU times unchanged within noise. Small tables stay for the existing pass in `panvk_lower_nir()` |

D3D11 FL 11_1 `tir` is not a driver bug. DXVK (3.1.1 and master) `D3D11RasterizerState::PromoteDesc()` builds
the DESC2 from a DESC1 with `ForcedSampleCount = 0`, so `CreateRasterizerState1` drops the forced count and the
attachment-less draw reaches the driver with `rasterizationSamples` 1 (diag on the TB336FU v9: raster 1, mask
0x1, no 4-sample pipeline created). Through `ID3D11Device3::CreateRasterizerState2` the count is kept and PanVK
runs the draw at 4 samples: `tests/tir` (native Vulkan, primary and secondary command buffers, inheritance
samples 0 and 4) passes 8/8 on v9 and v11, and the d3d11fl `tir` test now checks the DESC2 path (PASS on the
TB336FU with 115c). The DESC1 drop is a DXVK fix for later (runtimes/dxvk).

The rc4-lowmem `bc_decode raw=FAIL copy=FAIL` tester result is the readback bug that csf-v11 197 already fixes;
115c and 115d pass 16/16. `scripts/prepare-rc5-tree.sh` defaults are now `--csf-max 233 --wip-max 0140`. Strict
check on a fresh 5a07217f worktree (`/var/tmp/panvk/mesa-115d-final`, branch `p115d-final`): `BASE-OK applied=228
csf-v11<=233 head=df97bdeb` and `OK base=228 jm-v7-wip=135 head=b04b8ee3`, with no drift, offset or fuzz lines (only
the jm-v9/002 trailing-whitespace warning). Its tree equals the dev branch `p115c-final` + 0140. driverInfo
`Mesa 26.3.0-devel (git-b04b8ee36d)`.

## Build 115e (2026-10-29): 0141, v6/v7 64 storage images per stage (D3D12 FL 11_1)

| jm-v7-wip | what |
|---|---|
| 0141 | v6/v7 `maxPerStageDescriptorStorageImages` 32 to 64 (`panvk_shader.h`), the vkd3d-proton FL 11_1 requirement. The limit is bounded by the hardware: non update-after-bind storage images, storage texel buffers and uniform texel buffers share the image attribute table, two ATTRIBUTE_BUFFER entries each, and ATTRIBUTE "Buffer index" is 9 bits (512). The VS puts up to 16 vertex buffers first, so a stage holds 240 image entries. Uniform texel buffers count against the sampled image limit, so 256 sampled + 32 storage could already overflow (the copy job ORs the index into the format bits). `maxPerStageDescriptorSampledImages` 256 to 176 (D3D: 128 SRVs; zink caps at the 128 samplers), so 176 + 64 = 240; static asserts in `panvk_shader.h` and the VS table setup (`jm/panvk_vX_cmd_draw.c`). `maxPerStageResources` 712 to 664. UAB bindings are bindless (0050) and keep 2^20; v9+ unchanged |

Nothing else capped at 32: per-stage tables are sized from the shader's counts (`cmd_prepare_shader_desc_tables`,
VS `num_vbs + num_imgs`), the copy job indexes by count, and Bifrost image access takes the attribute index in a
register (`LEA_ATTR_TEX`/`LD_ATTR_TEX`; only the `LEA_ATTR_IMM` vertex-attribute form is limited to 16). Push sets
(32 descriptors) go through the same table. Not device-tested yet (v6/v7 via Firebase later); local v9/v11 devices
do not use this path. Strict check on a fresh 5a07217f worktree (`/var/tmp/panvk/mesa-115e-final`, branch
`p115e-final`): `BASE-OK applied=228 csf-v11<=233 head=a5178c1b` and `OK base=228 jm-v7-wip=136 head=e7d836d5`,
no offset or fuzz (only the jm-v9/002 whitespace warning); tree equals `p115d-final` + 0141. Driver
`/var/tmp/panvk/build-out/2026-10-29-115e/libvulkan_panfrost.so`, sha256 `8dea43cf...89d1d`, BuildID `aad440ce...`,
driverInfo `Mesa 26.3.0-devel (git-e7d836d5a6)`. `scripts/prepare-rc5-tree.sh` defaults: `--wip-max 0141`.

## Build 115f (2026-10-29): 0142, v9 tess draws without empty iterations (GPU-emulation perf, handoff item 7)

| jm-v7-wip | what |
|---|---|
| 0142 | v9 JM `v9_tess_draw()` planned every direct draw for the worst case: the chunks, plus 2 x patches / min_keep iterations for chunks that `panlib_tess_draw()` cuts at the index budget, plus one per nested range. Each iteration is about ten barrier jobs (step, VS indirect + VS, TCS grid + TCS, COUNT, draw, WITH_COUNTS, raster) and the extra ones ran empty. When min(max_patches, patches) patches at level 64 fit idx_limit no cut can happen, so the cut term is dropped; when that leaves one chunk, iterations after the first record only step + `panlib_tess_draw` + the raster (the next index range). `PANVK_V9_TESS_EXACT=0` = old. v9 only |

Where the time goes (PanProbe `tess_perf`, 256 draws x 16 quad patches per frame):
- v9 JM (TB336FU, G57): at L=1 the cost is the serialized JM job chain, about 3 us per empty barrier job and 30 us per
  empty iteration; 115e ran 3 iterations per gs=0 draw and 9 per gs=1 draw where 1 is needed. At L>=4 the cost is
  the tessellator: one thread per patch generates serially, and the libpan tessellate kernels use 1.3-2.1 KiB of
  stack per thread (`panlib_tessellate` 2112 B, `panlib_tess_count_draw` up to 5648 B on v11), so 16 patches = one
  warp, latency bound (L=16 is 1.8 us per generated index).
- CSF (X6 Pro, G615): about 500 us per tess draw at L=1 and 0 extra at L=4, so the fixed chain dominates: compute
  steps with full drains on the compute subqueue plus the READY/FREE hand-offs to the vertex/tiler subqueue, which
  kbase runs as separate CSGs. A CS-side poll of the hand-off syncs before SYNC_WAIT (dev experiment) never saw the
  other CSG's signal and cost about 2 us per poll; not shipped.

| device | config (us per draw) | rc4 | 115e | 115f |
|---|---|---|---|---|
| TB336FU v9 | gs=0 L=1 | 213.8 | 217.5 | 155.6 (-28 %) |
| TB336FU v9 | gs=1 L=1 | 562.2 | 532.3 | 368.3 (-31 %) |
| TB336FU v9 | gs=1 L=4 | 1984.9 | 2027.0 | 1717.0 (-15 %) |
| TB336FU v9 | gs=0 L=16 / gs=1 L=16 | 3042.4 / FAIL | 3013.7 / 12526.1 | 2899.8 / 12016.7 |
| X6 Pro v11 | gs=0 L=1 / gs=1 L=1 | 544.3 / 688.3 | 504.1 / 700.5 | 497.4 / 686.7 (unchanged) |

PanProbe 40/40 on both (TB336FU 202.9 s, 115e 206.0 s; X6 Pro 148.2 s, 115e 149.8 s), hashes equal to 115e, no
DEVICE_LOST: `worklogs/g615-dxvk/device-runs/2026-10-29-local-*-115f-*`. v6/v7 (`panvk_tess_draw()`, one pass, no
iteration loop) and CSF (GPU-side `cs_while`, no empty iterations) do not have this cost and are unchanged.

Strict check on a fresh 5a07217f worktree (`/var/tmp/panvk/mesa-115f-final`, branch `p115f-final`):
`BASE-OK applied=228 csf-v11<=233 head=625de24a` and `OK base=228 jm-v7-wip=137 head=0f038f9e`, no offset or fuzz
(only the jm-v9/002 whitespace warning); tree equals `p115e-final` + 0142. Driver
`/var/tmp/panvk/build-out/2026-10-29-115f/libvulkan_panfrost.so`, sha256 `6218671366aaf603...ecc3564`, 30337336
bytes, BuildID `cc715635b38fa263d3cc62ecfa02d935230df189`, driverInfo `Mesa 26.3.0-devel (git-0f038f9e96)`.
`scripts/prepare-rc5-tree.sh` defaults: `--wip-max 0142`. Test APKs (`build-115f.sh`, rc4 slot = `dl-rc4`):
`panprobe-115f.apk` `4d6baf87...`, `panplay-115f.apk` `83cf214d...`; pins unchanged (no release-candidate APKs).

Ideas left, by expected gain:
1. Tessellator stack: keep `CHWTessellator` and its small per-edge/axis arrays in registers (no dynamic indexing, or
   `nir_lower_indirect_derefs` for function_temp in `clc/pan_compile.c`, which needs host tools rebuilt from the
   tree, not `tmp/rel9`). Hits L>=4 on every arch, the biggest part of tess_perf wall time.
2. Parallel generation per patch (per ring/row) instead of one thread per patch (tess review #6).
3. v9 nested (gs=1) draws: per-range iterations are still planned for level 64; a GPU-side NULL of the empty
   iterations' jobs (`panlib_patch_job_type_header`, as `panlib_patch_draw` does) would remove them.
4. CSF: run the tess/GS compute steps on the vertex/tiler subqueue (one CS, no cross-CSG hand-off); needs the IDVS
   register state re-emitted after each compute and its own scoreboard slots.

## Build 115g (2026-10-29): 0143, tessellator state in registers (GPU-emulation perf, handoff item 7) + 0144

| jm-v7-wip | what |
|---|---|
| 0143 | `src/poly/cl/tessellator.h` only. The libpan tessellate kernels generate each patch on one thread; the per-edge arrays (tess-factor contexts, point counts, odd flags) were indexed by the ring-loop edge counter, so NIR put them in scratch, and the early returns of the inlined helpers left stale return-value phis carried around the ring loops (about 45 loop-carried values, so the register allocator spilled inside the loops). Per-edge values are now recomputed from scalars in the loop, the remaining small arrays are read through selects (`TESS_SEL2/3/4`), the odd flags are one bitmask, `PatchIndexValue`/`PlacePointIn1D`/`finalPointPositionTable` have one exit, and the index-patch context is reset at the end of each edge. Same output (tess_perf hashes equal). All archs; no compiler change |
| 0144 | CSF re-emits the DCD flags when the logic op changes (D3D12 FL 11_1 `logic_op`), see "logic_op CSF" below; first shipped in 115g |

Kernel TLS per thread (rel9 host tools, `libpan_shaders`):

| kernel | before | 115g |
|---|---|---|
| v9-v13 `panlib_tessellate` tri WITH_COUNTS (also `panlib_tess_gen` tri) | 1824 B, spills | 0 B, no spills |
| v9-v13 `panlib_tessellate` quad WITH_COUNTS (also `panlib_tess_gen` quad) | 2112 B (340 spills, 1367 fills) | 64 B (9 spills, 385 fills), 7094 -> 4462 instrs |
| v9-v13 quad COUNT (`panlib_tessellate_1`, CSF `panlib_tess_count_draw_2`) | 1312 B / 2208 B | 0 B |
| v6/v7 `panlib_v7_tessellate` tri / quad / quad COUNT | 1780 / 2064 / 1312 B | 0 / 0 / 0 B |

The fused CSF `panlib_tess_count_draw` gen variants (only run when `fuse_gen`) keep 160 B (tri) / 208 B (quad).

PanProbe `tess_perf`, us per draw (256 draws x 16 quad patches), full game loop scenario 2:

| device | config | rc4 | 115f | 115g |
|---|---|---|---|---|
| TB336FU v9 | gs=0 L=1 / L=4 | 213.8 / 679.3 | 155.6 / 584.9 | 111.5 / **281.2** (-52 %) |
| TB336FU v9 | gs=0 L=16 / L=mix | 3042.4 / 2864.3 | 2899.8 / 2724.9 | **1483.4 / 1312.1** (-49 / -52 %) |
| TB336FU v9 | gs=1 L=4 / L=16 / L=mix | 1984.9 / FAIL / - | 1717.0 / 12016.7 / 6355.2 | 1465.1 / 10627.1 / 4904.7 |
| X6 Pro v11 | gs=0 L=1 / L=4 | 544.3 / 577.7 | 497.4 / 500.0 | 383.1 / **278.6** (-44 %) |
| X6 Pro v11 | gs=0 L=16 / L=mix | 2832.9 / 2734.5 | 2316.1 / 2328.7 | **1141.7 / 1078.3** (-51 / -54 %) |
| X6 Pro v11 | gs=1 L=4 / L=16 / L=mix | 873.4 / 4647.7 / 3757.9 | 795.8 / 4127.5 / 3267.1 | 559.7 / 2798.6 / 1936.5 |

PanProbe 40/40 on both (TB336FU 187.0 s, 115f 202.9 s; X6 Pro 122.0 s, 115f 148.2 s), hashes equal to 115f, no
DEVICE_LOST: `worklogs/g615-dxvk/device-runs/2026-10-29-local-*-115g-*`. The first X6 Pro run stopped at
`swapchain_lifecycle` with a CPU SIGSEGV in `panvk_android_swapchain_post()` (WSI CPU-copy present, unchanged since
115f); the rerun passed. PanPlay on TB336FU (full bundle): s3, s6, s8 PASS (D3D11 11_0 tess PASS), s7 PASS except
one D3D12 11_0 `tess` FAIL (zero-area tri patch: 0 px) in the first pass after an app-side crash (Java NPE in the
X server); 11 of 12 runs of that pass PASS on 115g, cause not proven (no A/B against 115f at that repeat count),
see the PanPlay results.md. v6/v7 not tested on a device (no local JM v6/v7): the change is in shared CL source and
only removes TLS there.

Strict check on a fresh 5a07217f worktree (`/var/tmp/panvk/mesa-115g-final`, branch `p115g-final`):
`BASE-OK applied=228 csf-v11<=233 head=0f39027c` and `OK base=228 jm-v7-wip=139 head=2f2a20a7`, no offset or fuzz
(only the jm-v9/002 whitespace warning). Driver `/var/tmp/panvk/build-out/2026-10-29-115g/libvulkan_panfrost.so`,
sha256 `ce6635b2b82ad3a2...b41b6d9e`, 29278008 bytes (kernel binaries 1 MB smaller), BuildID
`30615eb1d40ff97e21bbdd5b67b62eefedd5d72f`, driverInfo `Mesa 26.3.0-devel (git-2f2a20a728)`.
`scripts/prepare-rc5-tree.sh` defaults: `--wip-max 0144`. Test APKs (`build-115g.sh`; PanPlay built from a copy of
`apps/panvk-launcher` at HEAD, without uncommitted work-tree changes): `panprobe-115g.apk` `4ac640a7...`,
`panplay-115g.apk` `14c7b5b4...`; pins unchanged.

Ideas left, by expected gain:
1. gs=1 at L>=4 is now bound by the GS emulation (TB336FU gs=1 L=16 10.6 ms vs gs=0 1.5 ms); the GS kernels use no
   TLS, so the cost is elsewhere (serial GS invocations per primitive, index/XFB writes).
2. Parallel generation per patch (per ring/row) instead of one thread per patch: L=16 is still about 1 us per index.
3. Quad WITH_COUNTS still peaks at 71 live values (block with the triangle `vstore3`); getting under 64 would remove the
   last 64 B of TLS.

## logic_op CSF (2026-10-29): 0144, not yet in a numbered build

| jm-v7-wip | what |
|---|---|
| 0144 | CSF `prepare_dcd()` re-emits DCD_FLAGS_0 (and v13 DCD_FLAGS_2) on `CB_LOGIC_OP_ENABLE` / `CB_LOGIC_OP`. 0138 made `any_dest_read` (so `allow_forward_pixel_to_kill`, v13 `hsr_can_cull`) depend on the logic op, but the CSF dirty check only watched blend enables/equations/write masks: vkd3d-proton's INVERT pipeline differs from the plain one only in the logic op, kept the plain draw's FPK, killed its fragments and inverted the clear (D3D12 FL 11_1 `logic_op` ffffffff, want 550ff000, Pixel 7 v10 / Pixel 8 v11 since 115b). JM emits the DCD every draw, so v7/v9 passed after 0138. |

Repro `tests/logic-op` (native Vulkan, R8G8B8A8_UINT, dynamic rendering, plain draw then a logicOp INVERT pipeline,
20 iterations per case): X6 Pro (G615 v11) 115f 2/4 (`loadop-clear`, `clearimage-load` 20/20 ffffffff), with 0144
4/4; TB336FU (G57 v9) 4/4 on both. 0143 (tess, separate work) is independent: strict check =
`prepare-rc5-tree.sh --wip-max 0142` on a fresh 5a07217f worktree (`/var/tmp/panvk/mesa-lop-strict`, `BASE-OK
applied=228`, `OK base=228 jm-v7-wip=137`) then 0144 applied with no offset/fuzz; tree `90b1b2ef` equals the dev tree
(`p115f-final` + 0144) the tested driver was built from. Driver `/var/tmp/panvk/bionic-lop-dist/libvulkan_panfrost.so`
sha256 `b0d63069b5802da1...`, BuildID `8e82ce80783d63b815711ede0b90213520b79a65` (driverInfo still reads
`git-0f038f9e96`: the build dir was configured before the commit). PanProbe 40/40, no DEVICE_LOST, on X6 Pro
(156.2 s) and TB336FU (218.0 s): `worklogs/g615-dxvk/device-runs/2026-10-29-local-*-lop-panprobe`.

## TLS/WLS 4 GiB window (2026-10-29): 0145, v14 `tessellation` DEVICE_LOST, not yet in a numbered build

| jm-v7-wip | what |
|---|---|
| 0145 | TLS and WLS regions stay inside one 4 GiB window. The GPU adds the per-thread (TLS) / per-workgroup (WLS) offset to the low 32 bits of the base only (kbase uAPI: `BASE_MEM_GPU_VA_SAME_4GB_PAGE` "required for TLS memory"; `pan_emit_tls()` asserts it for WLS, compiled out in release). Nothing enforced it: `cmd_dispatch_prepare_tls()` (CSF WLS) and JM batch TLS/WLS took plain `tls_pool` allocations, and the CSF device TLS (csf-v11 230/232 alias region or full BO) took whatever VA kbase/mmap gave. New `panvk_cmd_alloc_tls_mem()` skips a crossing pool allocation (the next one starts past the boundary or in a new BO); `dev_tls_alias()` / `dev_tls_create()` retry while holding up to 8 crossing regions/BOs |

Root cause of the RTL SM-X846B (Mali-G1-Ultra v14) 115e `tessellation` failure
(`worklogs/g615-dxvk/device-runs/2026-10-29-rtl-v14-sm-x846b-115e-panprobe`): `CSF group 0/1/2 fatal error: status
0x001003c1, sideband 0x5f0005d040` is the kbase AS fault forwarded as a group error: exception 0xc1 = translation fault
level 1, access type 3 = write, address 0x5f0005d040. On that device kbase (MEM_ALLOC fallback, SAME_VA) places the
driver's allocations around 0x6000000000 (ring entries `0x5ffffa12c0`, `0x6000000040`); 0x5f0005d040 is exactly
0x600005d040 - 4 GiB, a TLS/WLS write past 0x6000000000 that lost its carry. It hit right after the device TLS grew to
4096 B/thread x 2048 x 22 core IDs (176 MiB alias region) for the 3072-patch `grid_64x48_l16` case. Other devices put
SAME_VA elsewhere (crossing chance ~ size / 4 GiB per region), so v9-v13 passed. 115g's 0143 shrinks tess TLS and
makes the hit less likely, it does not remove it.

`tess_perf` on the same run is a different failure: `CSF group 0 tiler heap OOM notification` at gs=1 L=16 in the
`PANVK_TESS_FUSE=0` pass (the default pass passed the same scene). kbase only enters incremental rendering when the
group has `BASE_CSF_TILER_OOM_EXCEPTION_FLAG` (`kbase_queue_oom_event`, kbase-src `mali_kbase_csf.c` ~2033), which
panvk does not request (`kbase_kmod.c` KBASE_GROUP_CREATE_IN, csi_handlers 0), so a render pass that needs more than
the 400 x 1 MiB heap with no fragment work in flight is terminated. Not changed by 0145.

Strict check: fresh 5a07217f worktree `/var/tmp/panvk/mesa-v14-dev` (branch `pv14fix`), `prepare-rc5-tree.sh`
defaults (`--wip-max 0144`): `BASE-OK applied=228`, `OK base=228 jm-v7-wip=139`, tree `e6da4cd9` = `p115g-final`;
0145 committed on top (`fbbac61b`). Driver `/var/tmp/panvk/build-out/2026-10-29-v14fix/libvulkan_panfrost.so`
(`/var/tmp/panvk/v14fix-build.sh`, rel9 host tools, 4 jobs), sha256 `330b6a65c471695e...0424ae5`, 29291064 bytes,
BuildID `cce90267b61943cd68ec576d63027b51f520e8b2`, driverInfo `git-fbbac61b35`. APK
`/var/tmp/panvk/build-out/apks/panprobe-v14fix.apk` (`build-v14fix.sh`, rc5 slot, sha256 `50fb9b7e...`).

Tests (rc5 slot, autorun all, upload dry run):
- X6 Pro (G615 v11): PanProbe 40/40, no DEVICE_LOST, 123.6 s (`tess_perf`, `tessellation` PASS).
- TB336FU (G57 v9): 15/15 PASS incl. `tess_perf`, `tessellation`, then the run was cut by a PanPlay session another
  agent started on the device (not a driver failure).
- v14 RTL: UNTESTED (RTL link dropped repeatedly while queued on `rtl.lock`).

## Android present format guard (2026-10-29): 0146, X6 Pro `swapchain_lifecycle` SIGSEGV, not yet in a numbered build

| jm-v7-wip | what |
|---|---|
| 0146 | `panvk_android_swapchain_post()` (present thread, CPU copy into `ANativeWindow_lock()`) copies only when the locked window buffer is `WINDOW_FORMAT_RGBA_8888`/`RGBX_8888`; any other format posts the buffer uncopied (one `mesa_logw`). The copy writes `win_buf.stride * 4` bytes per row whatever the window format is |

Root cause of the 115g X6 Pro SIGSEGV (write, `memcpy`, `panvk present` thread, first 115g PanProbe run): PanProbe
`swapchain_lifecycle` `do_resize()` calls `ANativeWindow_setBuffersGeometry(win, W/2, H/2, 0)` and then recreates the
swapchain. Format 0 resets the window to the Surface default, RGB_565 for PanProbe's `SurfaceView` (no `setFormat`).
Frames the old swapchain queued before the resize are still posted by its present thread; when one is posted
between the app's `setBuffersGeometry` and the new swapchain's `setBuffersGeometry(..., RGBA_8888)`, the locked
buffer is 2 bytes per pixel and the 4-byte row stride writes past its end. Timing-dependent (the 115g rerun passed);
the width/height were already clamped, only the bytes per pixel were assumed.

Strict check: fresh 5a07217f worktree `/var/tmp/panvk/mesa-wsi-final` (branch `pwsi-final`), `prepare-rc5-tree.sh
--wip-max 0146`: `BASE-OK applied=228`, `OK base=228 jm-v7-wip=141 head=f0d27f9a`, tree `f6a4dcc8` = dev worktree
`/var/tmp/panvk/mesa-wsi-dev` (`d66d9347`). Driver `/var/tmp/panvk/build-out/2026-10-29-wsidev/libvulkan_panfrost.so`
(`/var/tmp/panvk/wsi-dev-build.sh`, rel9 host tools, 4 jobs), sha256 `b13af1b99f7d2903...`, 29291240 bytes, BuildID
`d301beb2fb19c334631b719002dfc4c7e87cfdb7`, driverInfo `git-d66d93471c`. APK
`/var/tmp/panvk/build-out/apks/panprobe-wsidev.apk` (`build-wsidev.sh`, rc5 slot, sha256 `8f7259c229cddea4...`).

Tests (rc5 slot, upload dry run, scripts `/var/tmp/panvk/wsi-dev-loop.sh`, `wsi-dev-full.sh`):
- X6 Pro (G615 v11): `--es autorun swapchain_lifecycle` 30/30 PASS, no SIGSEGV; the guard's warning never fired
  (the race was not hit in 30, so the loop shows no regression, not the fix itself). Full PanProbe 40/40, no SIGSEGV.
- TB336FU (G57 v9): full PanProbe 40/40, no SIGSEGV.
- 115g on X6 Pro, same loop: 6/6 PASS (the crash did not reproduce in 6).

D3D12 FL 11_0 `tess` flake (115g PanPlay s7, 1 FAIL in 12 on TB336FU): A/B with `gameloop-tess-repeat.json`
(scenario 7, 24 x `d3d12fl_11_0` per run, `/var/tmp/panvk/wsi-dev-ab.sh`): 115f 24/24 PASS, 115g 24/24 PASS. Not
reproduced (115g 1 FAIL in 36 counting the original 12); the one FAIL came in the first pass right after the app-side NPE crash. 0143 unchanged.

## CSF tiler heap OOM on kbase (2026-10-29): investigation, no patch

The 115e v14 `tess_perf` failure (`CSF group 0 tiler heap OOM notification`, then `timeout on subqueue 0`, at gs=1
L=16 in the `PANVK_TESS_FUSE=0` opt) does not reproduce with 0145 in the tree: v14 RTL `tess_perf` PASS in 5 of 5
runs (v14fix full run; `PANVK_TESS_FUSE=0` with the 400 max chunks, recycling on and off; two full runs below).
115e had no 0145, and a TLS/WLS write that wraps 4 GiB down lands in unrelated memory, so 0145 is the likely fix.

Why panvk cannot survive a pass that outgrows the heap (incremental rendering) on kbase:
- kbase answers a tiler OOM at max_chunks with a NULL chunk only when the group has `BASE_CSF_TILER_OOM_EXCEPTION_FLAG`
  (`handle_oom_event()`, kbase-src `mali_kbase_csf.c` ~2033); otherwise it terminates the group (DEVICE_LOST).
- With the flag (uAPI >= 1.12, 1_6 create layout skipped), kbase no longer terminates, but panvk's TILER_OOM handler
  never runs: the handler is registered on the FRAGMENT CS (`issue_fragment_jobs()`), and on kbase every subqueue is
  its own queue group (`kbase_create_group()`, one CS per group). X6 Pro with `PANVK_KBASE_HEAP_CHUNKS=4,8`: the
  IR counter stayed 0, the register dump area kept its 0xa5 fill, and subqueue 0 hung (timeout instead of OOM).
- Real fix: VERTEX_TILER and FRAGMENT in one kbase group (upstream panthor layout), or an IR handler on the
  VERTEX_TILER CS. Neither is small: 0134 (cross-subqueue waits) and CSG priority/rotation assume one group per
  subqueue.
- FINISH_FRAGMENT chunk recycling without the FRAGMENT_COMPLETED/FRAG_END counters (keeps kbase's `frag_end > vt_end`
  check happy) was tried and is not added: on X6 Pro with 32 max chunks recycling on and off both fail at the same
  pass (gs=0 L=16), with 128 both fail at gs=1 L=16 (single passes larger than the heap), and v14 passes without it.
  Patch kept at `/var/tmp/panvk/oomfix-patch/0147-panvk-recycle-kbase-tiler-heap-chunks-at-FINISH_FRAG.patch`.
  Earlier IR attempt diff: `/var/tmp/panvk/oomfix-wip-ir-attempt.diff`.

Tested build (strict tree with that 0147 on top, recycling on by default; `prepare-rc5-tree.sh --wip-max 0147` on a
fresh 5a07217f worktree `/var/tmp/panvk/mesa-oom-final`, `OK base=228 jm-v7-wip=142 head=2732f11e42d`): .so sha256
`2ebe4e602032be77...`, BuildID `5e205faa85f81c3f...`, driverInfo `git-2732f11e42`. X6 Pro 40/40, TB336FU 40/40,
v14 RTL 39/40 (`tessellation` mismatch=460, no DEVICE_LOST, same failure as the v14fix run). Not a release build.

## v14 tessellation stale chunk data (2026-10-29): 0147 (not the parked tiler-heap 0147 above), v14 UNTESTED

| jm-v7-wip | what |
|---|---|
| 0147 | `gpu_prerast_sync32_take()` on the compute subqueue (start of every iteration of the tessellation loop and of the draw chunk loop, after the previous chunk was drawn): `FLUSH_CACHE2` LSC clean+invalidate, other caches invalidate, wait. Default on for v14+, off before; `PANVK_PRERAST_CHUNK_FLUSH=0/1` overrides |

Failure (v14fix run, `worklogs/g615-dxvk/device-runs/2026-10-29-rtl-v14-sm-x846b-v14fix-panprobe/`): only
`grid_indexed_inst2_direct_first1`/`_indirect_first1` (mismatch 160 each). Knob runs on RTL SM-X846B with the then-installed
`git-2732f11e42` PanProbe (`/var/tmp/panvk/v14tess-knobs/`, tessellation only): defaults total mismatch 390 with
`grid_inst3_indirect_first1` 64 (first=(48,5)), `grid_indexed_*` 160/166 (first=(24,16) and (35,16)); `PANVK_TESS_FUSE=0`
total 3424. The other agent's 0147 run: 460. So the count changes run to run (direct 813 px vs indirect 811 px in one run).
- Not 0143: every failing case is equal-spacing quads L16 (`tcs_idx 7`); quad/tri/frac odd+even/isolines/point all PASS.
- Not 0145: TLS/WLS sizes are unchanged for these cases and the failure is not a fault.
- Pattern: the missing patches are always in the SECOND chunk of a draw (max_patches 507: `grid_indexed` 768 patches per
  instance = chunks [0,507) and [507,768); `grid_inst3` 2 instances in chunk 1, instance 2 in chunk 2), a run of
  patches some way into it (local 5 or 16 in `grid_indexed`, 80 in `grid_inst3`), missing rather than misplaced. Single-chunk
  draws (all other cases) and the tess_perf hashes (all chunks identical data) pass. Every chunk rewrites the same arena
  addresses (VS records, TCS outputs, domain points, coord_allocs, indices) that the previous chunk's kernels and its
  draw read, with no cache maintenance between chunks (the CSF barrier model treats LS as coherent). A stale previous-chunk
  line draws an already-drawn patch again (same position and primitive ID, invisible) instead of the new one.
- Root cause is therefore stale cached chunk data on v14 (inferred, not proven: no device time for the A/B).

Strict check: fresh 5a07217f worktree `/var/tmp/panvk/mesa-v14tess-final`, `prepare-rc5-tree.sh --wip-max 0147`:
`BASE-OK applied=228`, `OK base=228 jm-v7-wip=142 head=f2cc53b08e5`, tree `55cb1525` = dev worktree
`/var/tmp/panvk/mesa-v14tess-dev` (`f87c5894b9d`). Driver `/var/tmp/panvk/build-out/2026-10-29-v14tess/libvulkan_panfrost.so`
(`/var/tmp/panvk/v14tess-build.sh`, 4 jobs), sha256 `25b044d75e5b5b50...`, BuildID `7e1efa8af10cff9a...`, driverInfo
`git-f87c5894b9`. APK `/var/tmp/panvk/build-out/apks/panprobe-v14tess.apk` (`build-v14tess.sh`, rc5 slot).

Tests: X6 Pro (G615 v11) full PanProbe with `PANVK_PRERAST_CHUNK_FLUSH=1` (flush path forced on) 40/40; TB336FU (G57 v9,
JM, flush code not used) 40/40. v14 NOT TESTED (RTL session ended while rtl.lock was held). Next v14 run: PanProbe
`--es autorun tessellation` with defaults (expect 0 mismatch on repeated runs) and with `PANVK_PRERAST_CHUNK_FLUSH=0`
(expect the 64-460 mismatch back); `tess_perf` ms vs the v14fix run for the flush cost (one flush per chunk).

## v14 D3D9 DEVICE_LOST: sparse device TLS (2026-10-29): 0148

| jm-v7-wip | what |
|---|---|
| 0148 | `PANVK_TLS_SPARSE` defaults to 0 on v14 (csf-v11 230 alias region off, full device TLS BO); `=1` still forces the alias. v10-v13 unchanged |

Symptom (115e/115g, RTL SM-X846B, Mali-G1-Ultra v14, `shader_present 0x3300ff`, 22 core IDs for 12 cores): PanPlay
s3 D3D9 cube `VK_ERROR_DEVICE_LOST`. Not a deadlock: ~1-7 s after the first frame kbase reports a GPU MMU fault on
every group of the context (`CSF group N fatal error: status 0x....02c1/02c0/03c1`, exception 0xc0/0xc1 = translation
fault, access READ, once WRITE), the groups stop, and the "timeout snapshot" 10 s later (seqno < target, stream progress
0) is the consequence. Fault addresses are garbage: 0x0, 0x3f800000 (1.0f), 0x3f67f100, 0x3e7ace00, 0x144009d7c00,
0x144000ae5c0, 0x144001dad80, write at 0x5f00000000, i.e. pointers reloaded from memory holding other data.

A/B on the RTL device, 115g PanPlay, d3d9 cube passes of 15-23 s (`worklogs/g615-dxvk/device-runs/2026-10-29-rtl-v14-sm-x846b-tls-sparse/`):
- with the alias (default 115g, `old` knob set, `PANVK_KBASE_IMPORT_SYNC=1`, `PANVK_DEBUG=no_wb_mmap`, DXVK GPL off,
  d3d9 `presentInterval = 0`): faults in 13 of 17 passes; no knob other than the TLS one changes it, so not CPU cache
  maintenance, vsync idle or pipeline libraries.
- `PANVK_TLS_SPARSE=0`: 0 faults in 6 of 6 passes; `PANVK_DEBUG=sync` (its decode context also skips the alias): 0 in 2.
- PanProbe `tessellation` (installed `git-2732f11e42`, no 0147): `PANVK_TLS_SPARSE=1` FAIL mismatch 437/64/510 (3 of 3),
  `=0` PASS mismatch 0 (4 of 4). So the 0147 "stale chunk data" mismatch is the same bug (0147 was inferred, never A/B'd).

Cause: the alias maps the core-ID slots of absent cores onto slab 0 and relies on the hardware placing core c's stacks
in slot c. On v14 that does not hold (the slot mapping is unknown; the full BO covers every slot of the 22-ID range so
it works whatever it is): threads of different cores share stack memory, so a spilled 64-bit pointer is reloaded with
another thread's data. D3D9 (fixed-function lighting shaders, 4096 B/thread device TLS) and the tessellation compute
kernels spill; the D3D11 cube does not. v10 (mask 0x1110055) and v11 (0x130013) run the alias without faults.
Cost on v14: 176 MiB device TLS committed instead of 96 MiB.

Strict check: fresh 5a07217f worktree `/var/tmp/panvk/mesa-d3d9v14-final`, `prepare-rc5-tree.sh --wip-max 0148`:
`BASE-OK applied=228`, `OK base=228 jm-v7-wip=143 head=0cef29d17b1`, tree `ed9e282a` = dev worktree
`/var/tmp/panvk/mesa-d3d9v14-dev` (`41aa2dafce2`). Driver `/var/tmp/panvk/build-out/2026-10-29-d3d9v14/libvulkan_panfrost.so`
(`/var/tmp/panvk/d3d9v14-build.sh`, 4 jobs), sha256 `6cd9c1f507c1c9bc...`, 29294648 bytes, BuildID
`13ac4106911a0f0e8d86a48ce2250678a94da78b`, driverInfo `git-41aa2dafce`. APKs (`build-out/apks/build-d3d9v14.sh`, PanPlay
from a `git archive HEAD` copy of `apps/panvk-launcher`): `panprobe-d3d9v14.apk` `7549e90131516482...` (rc5 slot),
`panplay-d3d9v14.apk` `0f5b1c09cdad4cd0...`.

Tests with that driver: v14 RTL PanPlay d3d9 cube 7 of 7 default passes without a fault (the one `PANVK_TLS_SPARSE=1`
pass faulted), PanProbe `tessellation` 3 of 3 mismatch 0; X6 Pro (G615 v11) full PanProbe 40/40; v14 RTL full PanProbe 40/40 (115g-era v14fix: 39/40, tessellation); TB336FU (G57 v9)
full PanProbe 40/40. A full v14 PanPlay s3 run started but the RTL session ended in pass 1 (d3d11).

## Build 116 (perf) (2026-10-29): 0149-0151, default bundled driver

| jm-v7-wip | what | knob (`=0` restores the old behaviour) |
|---|---|---|
| 0149 | logic ops that do not read the destination (CLEAR, COPY, COPY_INVERTED, SET) no longer set `any_dest_read`, so FPK stays on (0138 turned it off for every logic op) | `PANVK_LOGICOP_FPK` |
| 0150 | v9 JM: compute, precomp (libpan) and `v9_prerast_job` payloads set "Allow merging workgroups" from the shader, as CSF already does | `PANVK_V9_JM_MERGE_WG` |
| 0151 | gpu_prerast kernels (lowered VS/TES, GS kernel, polygon kernel; 1x1x1 workgroups) are compiled with the merged-workgroup divergence options and set `allow_merging_workgroups` when `valhall_can_merge_workgroups()` allows it. Exception: on v9 a lowered VS that reads vertex inputs does not merge (see below). The knob is part of the shader cache key | `PANVK_PRERAST_MERGE_WG` |

v9 exception: with merging for every prerast VS, PanProbe `draw_params` case `divisor` (1 vertex x 6 instances,
divisor 2) failed on TB336FU (instances 6 and 7 read attribute 1004 instead of 1005); X6 Pro passed. Root cause not
found. Without the exception v9 `large_draw` was 7.3 s (10.2 s with it, 24.6 s before). Root-caused and dropped in
0152 (build 118 section below).

Strict check: fresh 5a07217f worktree `/var/tmp/panvk/mesa-116-final`, `prepare-rc5-tree.sh --wip-max 0151`:
`BASE-OK applied=228 csf-v11<=233 head=e9d394f0713`, `OK base=228 jm-v7-wip=146 head=f7e78953ac5`, tree equal to
the dev worktree `/var/tmp/panvk/mesa-116-dev`. Driver `/var/tmp/panvk/build-out/2026-10-29-116/libvulkan_panfrost.so`,
sha256 `d5817e51309f030581460b65c3cf1b59c6cf1bb4f71d69a303e57252a72cd1cf`, 29297400 bytes, BuildID
`87d679ee64ac7df831dbc2f214ab601b1e8282e8`, driverInfo `Mesa 26.3.0-devel (git-f7e78953ac)`. APKs:
`build-out/apks/panprobe-116.apk` `229dd862fc78e772...`, `panplay-116.apk` `03da729d0c5068a5...`.

Tests: PanProbe 40/40 twice on TB336FU (G57 v9) and twice on X6 Pro (G615 v11), no DEVICE_LOST. PanPlay on TB336FU
s3/s5/s6/s7/s8 all ok, no deviceLost. Gains vs the previous series (through 0148), warm cache: TB336FU tess gs=1 L=16
10.6 ms to 3.1 ms per draw, `large_draw` 24.6 s to 10.2 s, PanProbe sum 185.9 s to 143.2 s; X6 Pro tess gs=1 L=16
2.96 ms to 2.39 ms, `xfb` 322 to 223 ms, sum 122.2 s to 119.4 s. Full tables: `docs/PERF-PROGRESS.md`.
v6/v7/v10/v14 untested with 116.

## Build 118 (perf) (2026-10-29): 0152-0153, default bundled driver

"117" is the Firebase build of jm-v7-wip through 0148 (`git-1a0a81aa00`); this perf build is 118.

| jm-v7-wip | what |
|---|---|
| 0152 | v9: the merged lowered VS fetches instance-rate attributes with vertex frequency and computes the element (instance / divisor) itself; drops the 0151 v9 exception |
| 0153 | BC compute decoders (v6/v7/v9, 3D images everywhere) decode one 4x4 block per invocation, bit-identical; BC7 compute uses the fast decoders |

0152 root cause: on the G57 an instance-frequency `LD_ATTR` in a compute job with merged workgroups returns, for
every lane of a 4-lane group, the element of the group's first lane. `draw_params` `divisor` (1 vertex x 6 instances,
divisor 2) got elements 0,0,0,0,2,2 instead of 0,0,1,1,2,2, both with the hardware POT divisor and with the instance
ID divided in the shader and a plain 1D instance attribute (dev build A/B). Vertex-frequency fetches are per lane.
The lowered VS now reads one u32 per attribute slot after the gpu_prerast params (0 = vertex rate, else the divisor,
written by `v9_prerast_vs()`), passes instance / divisor as the vertex ID, and the prerast driver set uses vertex
frequency for every attribute. IDVS draws and CSF are unchanged. TB336FU dev build: PanProbe 40/40, `large_draw`
10.2 s to 7.2 s.

0153: one invocation per 4x4 block in the compute decoders (`vk/bc/panvk_bc_{s3tc,rgtc,bc6,bc7}.comp`, compute
SPIR-V headers regenerated, fragment and `PANVK_BC_BLOCK` variants unchanged; BC7 compute built with
`-DPANVK_BC_FAST`). TB336FU dev build (tree equal to the final): `bc_decode` PASS, `bc_perf` upload GPU ms BC1
38.4 to 13.9, BC3 56.5 to 15.6, BC4 41.7 to 15.5, BC5 48.8 to 15.8, BC6H 99/117 to 37/39, BC7 77.5 to 15.2.

Strict check: fresh 5a07217f worktree `/var/tmp/panvk/mesa-117perf-final`, `prepare-rc5-tree.sh --wip-max 0153`:
`BASE-OK applied=228 csf-v11<=233 head=2738924bfc7`, `OK base=228 jm-v7-wip=148 head=a0412457abc`, tree equal to
the dev worktree `/var/tmp/panvk/mesa-116-dev` (branch p117-dev). Driver
`/var/tmp/panvk/build-out/2026-10-29-118/libvulkan_panfrost.so`, sha256
`7a09e925138833727f95cb0e826b38faff63275c815d42cf9ab71db331df383c`, 29455736 bytes, BuildID
`d3bfed43a62ab7327426a5e22c059e0fc90eef1d`, driverInfo `Mesa 26.3.0-devel (git-a0412457ab)`. APKs:
`build-out/apks/panprobe-118.apk` `0980c534705aebea...`, `panplay-118.apk` `a59f01810c5cfa8a...`.

Tests: X6 Pro (G615 v11) PanProbe 40/40 twice with the final build, no DEVICE_LOST, within noise of 116 (sum
119.2 s vs 119.4 s; 0152 is v9-only, 0153 changes only compute BC decode, which v11 uses for 3D images).
v9 gate: the TB336FU stayed disconnected, so the final build ran on Firebase oriole (Pixel 6, Mali-G78, v9 JM):
PanProbe 40/40, `draw_params` and `bc_decode` PASS, no DEVICE_LOST
(`worklogs/g615-dxvk/device-runs/2026-10-29-ftl-v9-oriole-118-panprobe/`). **118 is the default bundled driver**
(both pins, `prepare-rc5-tree.sh` default `--wip-max 0153`). v6/v7/v10/v14 untested.

## Round 2 (2026-10-29): 0154+, perf plan items 5-23 (not yet in a numbered build)

| jm-v7-wip | what | knob |
|---|---|---|
| 0154 | driverInfo shows the patch level and build date: `bin/git_sha1_gen.py` appends `.panvk-patchlevel` (written by `prepare-rc5-tree.sh`, e.g. `p0157`) and the build date, `Mesa 26.3.0-devel (git-<sha>, p0157, 2026-10-29)` | - |
| 0155 | item 7: the BC transcoder pipelines of a mode compile on 3 background threads from the first BC image of that mode; a recording that needs one waits for it | `PANVK_BC_PREWARM=0` |
| 0156 | item 16 measurement: Android present thread post time (window dequeue, copy, queue) as running averages on the `PANVK_FPS_LOG` lines | - |
| 0157 | item 8: pass-through GS skipped at draw time, the VS runs on the hardware (v9+; not with pipeline statistics queries). `PANVK_GPU_PRERAST_ABI_VERSION` 11 | `PANVK_GS_PT_SKIP=0` |
| 0158 | item 23: `PANVK_GPU_STATS=<file>`: GPU load % from per ring entry GPU timestamps and VRAM bytes, rewritten at most every 500 ms (kbase CSF); PanPlay's MangoHud reads it | env unset = off |
| 0159 | reverts 0147 (v14 per-chunk gpu_prerast cache flush): the v14 A/B on 116 passed `tessellation` 3/3 without it; 0148 (sparse device TLS) was the fix. Simpler than dropping 0147 from the middle of the series | - |
| 0160 | the sw WSI (X11 present without DRI3 dma-buf, every PanPlay game) reads frames through the uncached buffer again: 0115's cached read + per-present DC CIVAC cost more (the rc4 vs 115c+ fps gap, FL 11_1 257 vs 831 fps on the X6 Pro) | `PANVK_WSI_SW_CACHED=1` = 0115 behaviour |
| 0162 | item 7 fix: the BC prewarm compiles the XC_BATCH pipeline only for batchable images (one-layer 2D with mips). 0155 compiled it for every ETC2 mode, which doubled the cold `bc_decode` (9.3 s vs 5.0 s on the X6 Pro) | `PANVK_BC_PREWARM=0` |
| 0163 | item 8 on v9: the pass-through GS skip in the JM `v9_cmd_draw()` (0157 hooked only CSF). TB336FU `strip_gs` 354 to 17.6 us/draw | `PANVK_GS_PT_SKIP=0` |
| 0164 | item 23 on JM (v6/v7/v9): `PANVK_GPU_STATS` GPU load = CPU-observed time with at least one atom of the context in flight (kbase_kmod counts atoms from JOB_SUBMIT to their completion event), reported per submit on a shared busy-time axis; VRAM as 0158. JM chains have no cheap GPU timestamps (PERMON + write jobs). TB336FU `workload_perf` with the env: `load=97..99`, `vram` reported (0158 left JM at 0). JM `vram` counts the full size of growable private BOs (1.35-1.68 GiB in PanPlay), an overestimate | env unset = no tracking |
| 0165 | item 21: v9 primitives-generated query: a direct draw without primitive restart (no GS/tess) is counted on the CPU, one `update_pstats_draw` job at the query end instead of one per draw (sum kept within 32 bits, else the GPU counts). TB336FU `pg_query` 18.6 to 12.2 us/draw (3 alternating runs, `ref` 6.2) | - |
| 0166 | 0164 fix: `PANVK_GPU_STATS` VRAM counts grow-on-fault private BOs (JM tiler heaps, varyings, GS/tess buffers) at their committed size (`KBASE_IOCTL_MEM_QUERY` COMMIT_SIZE per BO at each 500 ms report), as v10+ means it. TB336FU `workload_perf`: 1.46 GiB to 107-118 MiB | env unset = no list, no ioctl |
| 0167 | release blocker fix for 0161: on v14 G1-Ultra (gpu_id `0xe8800010`, RTL Tab S12+ SM-X846B) every build with 0161 (121 to 122b) fails `vkCreateDevice` with DEVICE_LOST: right after `kbase: tiler_heap_init layout=16 bytes` comes `kbase: timeout on subqueue 1: seqno 0`, no queue runs a job (worklog `2026-10-29-rtl-v14-sm-x846b-122`). `PANVK_KBASE_TILER_IR` now defaults by arch: on for v11 (X6 Pro) and v13 (S11 Ultra), where 0161 is verified; off on v10, v12 and v14 (old layout: one group per subqueue, no OOM flag, no per-pass FINISH_FRAGMENT). v10/v12 are untested with 0161, so off is the safe default there | `PANVK_KBASE_TILER_IR=1` forces 0161 on, `=0` off |
| 0168 | 0160 fix per arch: the sw WSI readback (every PanPlay present) defaults to the cached buffer + invalidate on v13+ and stays uncached below. v14 SM-X946B D3D11 cube: 258 fps uncached, 733 cached, 116 766 (RTL, commit 6424169a); v13 lost the same way from 120 on (cube 964 on 118, about 600 after). X6 Pro (v11) keeps uncached: FL 11_1 about 800 fps uncached vs about 470 cached, although its cube is faster cached (339 vs 270). An automatic choice (timing an invalidate + frame read of each type at the first swapchain) was tried and picked cached on the X6 Pro too, so it was dropped. v12 untested, stays uncached | `PANVK_WSI_SW_CACHED=1` cached, `=0` uncached |
| 0169 | 0168 widened: uncached sw readback only on v11, cached on every other arch (as before 0160). TB336FU (v9) PanPlay .so bisect without env (installDrivers kit, `worklogs/g615-dxvk/device-runs/2026-10-30-v14-so-bisect-kit`): D3D12 gears 248/251/257/259 fps on 116/117/118/119, then 220/220/223/218 on 120/121b/122c/122d; D3D11 FL 11_0 212/211/211/214, then 196/199/200/197. 120 only adds 0160 for v9 | `PANVK_WSI_SW_CACHED=1` cached, `=0` uncached |
| 0170 | 0153 cold-compile fix: the first BC emulation decode of a format (v9 compute decoders) compiles the per-texel decoder of 116 (fast to compile) and queues 0153's block decoder (2.7-5x faster decode, ~2.5x the compile time) on a 2-thread minimum-priority queue that starts once no per-texel decoder has compiled for 100 ms (at once after a shader cache hit); later decodes use the block one when it is in the meta cache. Never compiled synchronously. TB336FU cold `bc_decode` 877-886 ms vs 116 900-937 vs 122e 1830; warm 189-246 vs 116 177-182; `bc_level` 818-870 vs 116 775-823 vs 122e 1748 ms | `PANVK_BC_DECODE_TEXEL=0` = 0153 behaviour |
| 0171 | sw WSI readback mode per swapchain from its frame rate: with MIT-SHM + deferred reply each BUFFER_BLIT swapchain measures its mean frame interval in the starting (arch default) mode, the other mode, the starting one again (A, B, A; 120 frames and 500 ms per window, after a 30 frame / 250 ms settle, from 1 s after the first present), keeps B only if 3 % faster than the A mean, re-measures on a 25 % drift (at most every 10 s). `wsi_image_set_cpu_readback_cached()` moves an image to the other memory type right after its CPU copy. X6 Pro (v11) PanPlay: D3D11 cube 352/383 fps (122e cached 370, uncached 271, 116 312-333), FL 11_1 753/709 (cached 562, uncached 759, 116 404-443) | `PANVK_WSI_SW_CACHED=1`/`=0` forces a mode, no measurement |
| 0172 | v10+ lowmem BC cold compile: `bc/gen_etc2_fixed.sh` builds the ETC2/EAC transcoder once per mode at the default `PANVK_BC_LM_OPTS` (0x3020000) and the BC6H block decoder once per signedness, mode as a plain constant, `spirv-opt -O`; lookup tables are private arrays there (glslang copies a const array into a function variable at every dynamic index). The front end (spirv_to_nir + NIR opts on the full 150 KB shader) was 490-690 ms per mode. X6 Pro (v11) cold `bc_decode` 612-648 ms (122f 5192, rc4 526-588), per mode 8-55 ms (BC6H 61-74), `bc_level` 39 ms/op (122f 462, rc4 49), `bc_perf` 28.6 s (rc4 28.7) | `PANVK_BC_XC_FIXED=0` = full shaders |
| 0173 | 0171 decides once per swapchain (a resize makes a new swapchain and probes again): A/B/A windows of 60 frames or 400 ms after a 1 s start delay; flip only if B beats both A windows by 10 %, keep only if B loses to both by 10 %, else retry after 3 s (at most 3 tries) and keep the current mode. Fixes the v12 SM-X920 flip-flop (5 probes in one D3D12 gears run, cube 603 vs 745-762 cached) and the X6 Pro first-probe miss (dev16 cube auto 272 vs cached 350). X6 Pro dev17 (same session): cube auto 711/711 vs cached 704, uncached 323; FL 11_1 auto 1214/933 vs 1093/914; D3D12 gears auto 502/484 vs 504/265 | `PANVK_WSI_SW_CACHED=1`/`=0` forces a mode, no probe |

Build 119 = 0006-0159 (`git-0146a719f6`, driverInfo `Mesa 26.3.0-devel (git-0146a719f6, p0159, 2026-10-29)`,
.so sha256 `186278b8...`, BuildID `ba2ef6fd...`). Build 120 = 0006-0160 (`git-94af3802af`, driverInfo
`Mesa 26.3.0-devel (git-94af3802af, p0160, 2026-10-29)`, .so `/var/tmp/panvk/build-out/2026-10-29-120/libvulkan_panfrost.so`
sha256 `1195a047045b6384480a88924ddf61c1095ace90bfcbb95aa26c6598ff99d716`, 29455192 bytes, BuildID
`6c1061318e24f02cac02a577242c3709fff1d05b`); strict prepare `BASE-OK applied=228`, `OK base=228 jm-v7-wip=155
head=94af3802af9`. **120 is the default bundled driver** (`prepare-rc5-tree.sh` default `--wip-max 0160`).

Build 121b = 0006-0163 (it includes the tiler-OOM 0161), `Mesa 26.3.0-devel (git-07177325ca, p0163, 2026-10-29)`.
- .so: `/var/tmp/panvk/build-out/2026-10-29-121b/libvulkan_panfrost.so`, sha256
  `9f58ee27ad8e4d0b02f43b26bada3e34da5feba5134f8ac1c91b23c4f9cf85ae`, BuildID
  `c5337130bf092b76764921dc321808a88f56df0c`.
- Strict prepare: `OK base=228 jm-v7-wip=158 head=07177325cad`.
- Tested with PanProbe only: 41/41 on the X6 Pro and the TB336FU.
- **Not the default.** The pins stay on 120.

Numbers: `docs/PERF-PROGRESS.md` "Round 2" and "Build 120"; status per item: `docs/PERF-OPTIMIZATION-PLAN.md` section 5.

## kbase tiler heap OOM: incremental rendering (2026-10-29): 0161, not the default, v13/v14 pending

Follows "CSF tiler heap OOM on kbase" above (v13 G925 `tess_perf` ~75% DEVICE_LOST, every failure with
`CSF group 0 tiler heap OOM notification`, `worklogs/.../2026-10-29-rtl-v13-sm-x930-tessperf/`).

| jm-v7-wip | what |
|---|---|
| 0161 | (1) All three subqueues in **one** kbase queue group (CS i = subqueue i, the panthor layout) created with `BASE_CSF_TILER_OOM_EXCEPTION_FLAG`; group create tries the layouts that carry `csi_handlers` before the 1_6 one (MediaTek 1.21). At `max_chunks` kbase now answers the tiler OOM with a NULL chunk instead of terminating the group, and the firmware runs the existing upstream TILER_OOM handler on the fragment CS (incremental rendering: flush the bins with the IR FBDs, FINISH_FRAGMENT the chunks back). That handler only reaches CSs of the group that hit the OOM, which is why it never ran with one group per subqueue. (2) The fragment subqueue issues FINISH_FRAGMENT at the end of every pass again, without the FRAGMENT_COMPLETED increment and without FRAGMENT_END (vt_end/frag_end stay 0, so 098's heap statistics check holds): chunks come back per pass instead of only at heap renewal. Without (2) the IR handler frees nothing once earlier passes hold every chunk and the tiler OOMs forever. `PANVK_KBASE_TILER_IR=0` restores the old layout and behaviour (both parts). |

Root cause (X6 Pro, rooted, dmesg): with a small heap build 119 reproduces the v13 failure exactly
(`PANVK_KBASE_HEAP_CHUNKS=4,32`: 3/3 FAIL, dmesg `Queue group to be terminated, couldn't handle the OoM event`).
kbase `handle_oom_event()`: `chunk_count >= max_chunks` with `pending_frag_cnt == 0` is `-ENOMEM`, which terminates
the group unless the group has the OOM exception flag. Since 098 no chunk came back before heap renewal, so any heap
generation (up to 128 submits or 65536 work units) shared one `max_chunks` budget; why 1024/4096 max chunks still
failed on v13 is not proven (no dmesg on RTL), but both parts of 0161 address the budget and the termination.

X6 Pro results (`worklogs/g615-dxvk/device-runs/2026-10-29-x6pro-tiler-oom/results.md`): `tess_perf` at 4,32
12/13 PASS with 0161 (dev 10/10, strict build 2/3), full PanProbe 41/41 at the default heap (dev and strict) and 41/41
at 4,32, `large_draw` 2/2 at 1,8, PanPlay s3/s5/s7 fast pass ok. No OOM termination in any 0161 run. Perf (same
binary, `PANVK_KBASE_TILER_IR=0` A/B, default heap): heavy tess frames faster (gs=1 L=4 105-111 ms vs 162-185 ms,
FUSE=0 gs=0 L=4 74 vs 135-153), `L=16` within noise, trivial `L=ref` frame +0.2-0.3 ms, `large_draw` equal.

Strict check: fresh 5a07217f worktree `/var/tmp/panvk/mesa-oom2-final`, `prepare-rc5-tree.sh --wip-max 0161`:
`BASE-OK applied=228 csf-v11<=233`, `OK base=228 jm-v7-wip=156 head=84704e1bb7a`, equal to the dev tree
`/var/tmp/panvk/mesa-oom2-dev`. Build oom2final: `/var/tmp/panvk/build-out/2026-10-29-oom2final/libvulkan_panfrost.so`,
sha256 `d0d4a971f470d6d2...`, BuildID `0cf6b04d5cfdacb4...`, driverInfo `git-84704e1bb7, p0161`; APKs
`/var/tmp/panvk/build-out/apks/panprobe-oom2final.apk`, `panplay-oom2final.apk`. Not the default bundled driver
(`prepare-rc5-tree.sh` default stays `--wip-max 0160`).

Open problems:
- **Firmware hang under a very small heap.** With 0161 and `PANVK_KBASE_HEAP_CHUNKS=1,8`, 4 of 10 `tess_perf` runs
  hang (mostly gs=1); with 4,32 1 of 13 (strict build, gs=1 L=4, FUSE=0 opt). Shape: no OOM termination, all three
  subqueues stuck, MCU stops answering (`Timeout on CSG_REQ:STATUS_UPDATE`), kbase resets the GPU, group fatal 0x72
  (kbase SW fault after reset). 1,16 4/4 PASS. Hypotheses, not tested: (a) the tiler's working set (open chunks per
  core and hierarchy level) exceeds a tiny heap, so IR cannot free enough to make progress; (b) the IR handler is
  only armed while the fragment CS sits in `wait_finish_tiling()`; if the fragment CS waits on another subqueue
  (compute tess/GS work that waits on the stalled VT draw) before arming it, nobody services the pending OOM
  exception. (b) would also hit big heaps, so the v13/v14 RTL reruns decide.
- One group per queue changes kbase scheduling for every kbase device (v9 JM excluded): v10/v12/v13/v14 need a full
  PanProbe + PanPlay rerun before 0161 becomes the default.
- Groups created with the 1_6 layout (kernels that only take 32 bytes) silently drop the flag: the one-group layout
  still applies, IR does not, OOM terminates as before.

Next steps: RTL v13 SM-X930 (coordinator run `2026-10-29-rtl-v13-sm-x930-oom2final/`) and v14 SM-X846B: PanProbe
with `tess_perf` x10 at the default heap and at 4,32, PanPlay 1-9. If the v13 hang persists at the default heap,
look at (b): arm the handler before the fragment CS's cross-subqueue waits, or add a VT-side bound. If only tiny
heaps hang, raise the minimum `max_chunks` clamp (8 today) and make 0161 the default.

Build 122 = 0006-0165, `Mesa 26.3.0-devel (git-3506474d17, p0165, 2026-10-29)`.
- .so: `/var/tmp/panvk/build-out/2026-10-29-122/libvulkan_panfrost.so`, sha256
  `acb4a0116987fd807163f438424cec9c6c3fbd0d0712d47d1a3115197cebc81e`, BuildID
  `2f61168ad05aa9204824eb58cd9a50de43e8c785`.
- Strict prepare: `OK base=228 jm-v7-wip=160 head=3506474d173`.
- PanProbe 41/41 cold and warm and PanPlay 1-9 on the X6 Pro and the TB336FU; v6/v7/v10/v12-v14 not run yet.
- **122 is the default bundled driver** (`prepare-rc5-tree.sh` default `--wip-max 0165`).

Build 122b = 0006-0166 (122 + the 0166 VRAM fix), `Mesa 26.3.0-devel (git-e2f5118e2a, p0166, 2026-10-29)`.
- .so: `/var/tmp/panvk/build-out/2026-10-29-122b/libvulkan_panfrost.so`, sha256
  `d8c7e952d8fe716afa86f3ec19ad8dc31110c8ce72b9253fdf0db9d1d6f831ba`, BuildID
  `11aa1731d6e592157c5506b84dc3afd8eb71f69f`.
- Strict prepare: `OK base=228 jm-v7-wip=161 head=e2f5118e2ac`.
- **122b is the default bundled driver** (`prepare-rc5-tree.sh` default `--wip-max 0166`).

Build 122f = 0006-0171, `Mesa 26.3.0-devel (git-e1d5a21c4a, p0171, 2026-10-30)`.
- .so: `/var/tmp/panvk/build-out/2026-10-30-122f/libvulkan_panfrost.so`, sha256
  `6ca4bb85e0395ccd363415910d4ab9d7c04d54229ee7ac06d4ec71f8faa9041b`, BuildID
  `5dff0d019d295b5cc0d915ba6cc01feb8506f681`.
- Strict prepare (`--wip-max 0171`): `OK base=228 jm-v7-wip=166 head=e1d5a21c4a9`.
- PanProbe 41/41 cold and warm on the X6 Pro and the TB336FU; PanPlay 1-9 on the X6 Pro only
  (`worklogs/g615-dxvk/device-runs/2026-10-30-local-122f`).
- **Not the default.** The pins stay on 122e (`--wip-max 0169`) until the TB336FU PanPlay gate passes.
