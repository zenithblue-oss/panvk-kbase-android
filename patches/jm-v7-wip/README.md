# jm-v7 WIP (item 19, Bifrost DXVK feature port)

`scripts/apply-patches.sh` does not apply these. Apply them with `git am`, in order, on top of the full series (`apply-patches.sh --profile g615-v11-csf`, which already applies jm-v7 001-005). They are `git format-patch` files and depend on each other, so plain `git apply` of one alone fails.

Worktrees: `work/mesa-b2` (branch `v7port2`) is the current base: the full series as one commit `30a2037`, with 006-013 on top. `work/mesa-b` (branch `v7port`) has the same 006-013 on the older series (98c8b2f).

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

## Build status (2026-10-16, cloud session)

- Series: Mesa 5a07217f + the current full series (186 patches, `apply-patches.sh --profile g615-v11-csf`, strict: no offsets, no fuzz). This includes android/016 and csf-v11/187-192.
- `git am` of 0006-0013 on a fresh tree with that series applies cleanly. The patches were made with `git format-patch` from that same tree, so no hunk has an offset.
- Rebasing 006-013 onto the new series needed no conflict resolution. The combined diff is identical to the old one on `work/mesa-b`. Of the series changes since 98c8b2f, the only ones in files these patches touch are csf-v11/188 and 189 (`maxFragmentDualSrcAttachments`, denorm properties), and they do not overlap.
- `scripts/build-android.sh --profile g615-v11-csf` (API 29, NDK r30 30.0.16248370) builds the universal ICD with no errors and no warnings in the files 006-013 touch. The full build has 6 warnings, all from the series itself: 5x `-Wc23-extensions` at `csf/panvk_vX_gpu_queue.c:858` (csf-v11/192) and 1 deprecation in `android_stub/backtrace/BacktraceMap.h`. libpanvk_v6/v7/v9/v10-v14 are all built.
- `libvulkan_panfrost.so` from `work/mesa-b2` (current series): BuildID `4f07459194dd25e244169faf1e1d8a0cbe3b2028`, in `dist/b2/`. It is unstripped, 25.98 MB. NEEDED: liblog, libnativewindow, libsync, libm, libz, libdl, libc. It does not import `call_once`.
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
| tessellationShader | **off**. Not implemented, see the plan below. |
| fillModeNonSolid, multiViewport, depthBounds, pipelineStatisticsQuery | off (v9+ only) |

## BC texture compression

BC emulation does not depend on the arch. It was already on for v6/v7 before 013:
- `panvk_bc_emul_enabled()` (csf-v11/108) does not check the arch. It emulates unless every BC format is native. `textureCompressionBC` is `native || emulated`.
- A BC image is LINEAR (`panvk_image_get_mod`). It gets a decoded plane after the raw blocks. Uploads (`CmdCopyMemoryToImage`, `CmdCopyBufferToImage2`) and `CmdCopyImage2` into it decode with the compute shaders in `panvk_vX_cmd_meta.c` (`bc/*.comp`: buffer_reference loads and stores only, 8x8 workgroups). Views sample the decoded plane (`panvk_vX_image_view.c`). The decoded formats (RGBA8, RG16 unorm/snorm, RGBA16F) are plain Bifrost texture formats.
- The fragment decode into compact or AFRC shadows (csf-v11/150, 180, 185) is v10+ only (`image->bc_gfx`, gated on arch >= 10). v6/v7 and v9 always use the compute path. The PanProbe `bc_decode`/`bc_perf` results on the v9 G57 come from that same compute path.

The one gap was on JM: only the CSF `CmdPipelineBarrier2` called `cmd_bc_decode_zero_initialized()` (csf-v11/040). On v6/v7, an image leaving `VK_IMAGE_LAYOUT_ZERO_INITIALIZED_EXT` kept a zeroed decoded plane. It should hold the decode of zero blocks: opaque black for BC1. 013 adds the call on v6/v7. v9 JM has the same gap, and this patch leaves v9 alone.

## Tessellation plan (not done)

I did not implement tessellation. It needs about 1000-1500 new lines across the shader compiler, the JM draw path and libpan. A JM tessellation path has one-shot sizing, chunking and indirect-draw cases that could not be checked without a device. The v9 version needed several device rounds (jm-v9 014, 017, 028) before G57 38/38. Here is what exists and what is missing.

**Already there, from 007, unused:**
- `panvk_gs_push` fields: `tess_params`, `num_prims_addr`, `patch_size`, `out_clip_space`.
- `panvk_nir_lower_tcs_inputs()`: reads TCS per-vertex inputs from the vertex job's varyings, like the GS does. The patch size is dynamic (`patch_size`), there is one workgroup per patch (x the patch, y the instance), and direct, indirect and indexed draws are handled.
- `panvk_nir_lower_tess_params()`: `load_tess_param_buffer_poly` -> `tess_params`.
- `panvk_nir_tes_to_gs()`: turns a TES that `poly_nir_lower_tes` already lowered into a point-in, point-out GS for `panvk_nir_lower_gs`.
- `PANVK_TESS_HEAP_SIZE`, `PANVK_TESS_MAX_INDICES`, `PANVK_TESS_INDIRECT_GRID` in `panvk_gs.h`.
- `num_prims_addr` support in `panvk_nir_lower_gs` (`min(*num_prims_addr, num_input_prims)`). `out_clip_space` support in `lower_emit_vertex`.

**Missing, in order:**
1. **Shader compile (`panvk_vX_shader.c`, `PAN_ARCH < 9`).**
   - TESS_CTRL: take the v9 `MESA_SHADER_TESS_CTRL` case (`nir_lower_io`, `panvk_tess_record_info`, `poly_tcs_per_vertex_outputs`, `poly_tcs_output_stride`, `panvk_tess_lower`, `poly_nir_lower_tcs`). Then run `panvk_nir_lower_tcs_inputs` and `panvk_nir_lower_tess_params`, and compile as compute with `workgroup_size = tcs_vertices_out` (at most 32).
   - TESS_EVAL: `panvk_lower_tes`/`poly_nir_lower_tes`, then `panvk_nir_tes_to_gs` and `panvk_nir_lower_gs` with `input_verts_per_prim = 1`. Store the result in a `panvk_shader_variant.gs`-like record, so the existing GS job emitter can run it.
   - VS with a TCS next: compile in clip space, as for a GS (`next_stage_mask & TESS_CONTROL`).
   - The tess info (domain, spacing, winding, point mode, out patch size, per-vertex/patch output counts) needs a home in the serialized variant. `struct panvk_gpu_prerast_tess_info` is v9+ only, so either un-gate it or copy the fields.
   - `panvk_cmd_bind_shader()`: store TCS/TES (today `assert(shader == NULL)` on v6/v7), set GS dirty, and save/restore both in meta (as 012 does for the GS).
2. **libpan (`draw_helper.cl`, v6/v7 section).**
   - The libpoly tessellator kernels (`panlib_tessellate` over `poly_tess_*_process`, count and write modes) are only built in the `PAN_ARCH >= 9` section. `tessellation.cl` is not in `libpan_shader_files` at all. Build them for v6/v7.
   - `panlib_prefix_sum_tess` is a 1024-wide workgroup, which is likely over the Bifrost limit. kbase reports `max_threads_per_wg`; I expect 384 on G52/G76 and 256 on G71/G72, not checked on a device. Use the serial `panlib_prefix_sum_tess_serial` (v9) or a 256-wide scan. Its output must be the GS emulation's `num_prims_addr` count, plus an index base for the TES-as-GS job, not a tiler draw.
   - A kernel that fills `poly_tess_params` from the draw (and from the indirect command for an indirect draw): patch count, instance count, heap, TCS output buffer, counts buffer.
3. **Draw path (`jm/panvk_vX_cmd_draw.c`, `PAN_ARCH < 9`).** For a draw with a TES bound, emit one job chain:
   - vertex job (clip space, all patch vertices);
   - TCS compute job (grid: patches x instances);
   - tess params job;
   - tessellator count job;
   - prefix sum job;
   - tessellator write job;
   - TES-as-GS job over the tessellator's domain points, with `num_prims_addr` set to the generated count. It writes the GS output buffers sized for `PANVK_TESS_MAX_INDICES`;
   - the tiler job (the existing GS tiler path: `tiler_index_count` patched by the emulation).

   With an application GS after the TES, set `out_clip_space=1` on the TES job and run the app GS over the TES output as a second emulation job, with the TES output as its input buffers. The existing per-layer loop, XFB capture (`emit_xfb_jobs` on the last emulated stage) and `gs.emu` selection apply unchanged. Output past the fixed heap or `PANVK_TESS_MAX_INDICES` must be dropped with a `mesa_logw_once`, as v9 does before 028. Chunking (v9 017/028) can come later.
4. **Features (`panvk_vX_physical_device.c`).** `tessellationShader`, `extendedDynamicState2PatchControlPoints` and the tess limits (copy the v9 values). Keep `shaderTessellationAndGeometryPointSize` off.
5. **Device validation:**
   - PanProbe `tessellation`, `gs_tess_primitive_id`, `large_draw` and `xfb`;
   - CTS `dEQP-VK.tessellation.*` (primitive_discard, winding, spacing, invariance subsets) and `transform_feedback` with TES;
   - then DXVK D3D11 hull/domain shader titles.

   Watch for:
   - TCS barrier hangs (workgroup scope with global memory, see `lower_tcs_instr`);
   - heap overflow on large tess factors;
   - JM job-index overflow (each tessellated draw adds about 8 jobs per layer).

**Estimate:** 3-5 days to write, plus device rounds. DXVK D3D9 and D3D10 do not need tessellation. D3D11 FL11_0 needs it, and DXVK reports a lower feature level without `tessellationShader`.

## Limits (all build-tested only)

- 009: gl_PointSize is not captured (it reads 0), and points drawn through the passthrough rasterize at size 1. gl_PrimitiveID in the FS counts across instances.
- 010: robustBufferAccess2 relies on the hardware bounds of UBO, attribute and texel-buffer descriptors, as JimVulkan reports for G52/G72/G76. That is unverified here.
- No device has run 006-013. A host recording check under the panfrost drm-shim (PAN_GPU_ID 6221, 7212, 7211) is described in 011's message. It covers command recording only: the drm-shim cannot run kbase JM atoms.

## What needs a device

- RTL A06 (G52 MC2 r1, `0x74021000`) and A14 (G52 MC1, `0x72120000`). Baseline PanProbe was 14/38 before 006. Run PanProbe with this build. Expected gains:
  - geometry, xfb, clip_cull, robustness2/null descriptor, bc_decode/bc_perf, vertex_stores, MDI;
  - tessellation cases stay unsupported.
- CTS subsets: `dEQP-VK.geometry.*`, `transform_feedback.*`, `robustness.robustness2.*`, `texture.compressed.*bc*`, `draw.*indirect*`.
- DXVK D3D9 and D3D11 (FL10_x) in PanPlay.
- Move the series to `patches/jm-v7` (006+) once A06/A14 gets past 14/38.
