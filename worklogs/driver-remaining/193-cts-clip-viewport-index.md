# 193: old CTS failures (clip/cull through GS/tess, FS gl_ViewportIndex, depth_bias)

Date: 2026-10-16. Patch: `patches/csf-v11/193-carry-clip-cull-distances-through-the-gs-and-viewportindex-to-the-fs.patch` (not committed).
Build: `g615-v11-csf` OK, BuildID `972fee08c754356b810fae9d77e72fe030f7ca17` (sha256 `e71977f5…79ebf`), 0 compiler warnings in the build log (the incremental rebuild covered all `panvk_vX_shader.c` variants v6-v14).
Strict check: fresh `98c8b2f~1` worktree, `apply-patches.sh --profile g615-v11-csf`, `OK applied=187`, no PATCH-DRIFT. The resulting tree is byte-identical to `work/mesa-f` + 193.
**Not run on a Mali device. No CTS result here comes from this patch.** Device rerun list: `validation/driver-remaining/rerun-193.txt` (458 cases).

Evidence used: `validation/driver-remaining/beta13-device/` (188 failures, the same on b10/b12/b13; 16 pass when run alone) and the beta.17 52725-case lists `validation/driver-remaining/beta17-device/cts-b17{b,d}-52725-nonpass.txt`. Those lists still have all 60 `shader_viewport_index.fragment_shader_2..16` failures. They do not include `clipping.user_defined` or `depth_bias`, so those were not re-measured after beta.13.
CTS sources read: `vktClippingTests.cpp` (ClipDistance namespace), `vktDrawShaderViewportIndexTests.cpp`, and the amber `draw/depth_bias/depth_bias_patch_list_tri_line.amber`.

Line numbers below are in the strict-series tree before 193 (`work/mesa-f` HEAD `2b03d6f`), `src/panfrost/vulkan/panvk_vX_shader.c` unless noted.

## Do the patches released after beta.13 (up to 192) already fix these?

No, none of them do.

- **115** (FS PrimitiveID across VS viewport runs, released in beta.17) is in the same area. It adds a PrimitiveID output to the vp passthrough, but it does not touch ViewportIndex. beta.17 still fails all 60 `fragment_shader_N` cases (`cts-b17b/d` lists).
- **132** (wait for the prerast producer before viewport runs) fixes a compute-queue race that drew nothing. The `fragment_shader_N` geometry is already correct: `vertex_shader_N` passes, and only the colour picked by `color[gl_ViewportIndex]` is wrong. That index is always 0, which is a deterministic compile-time result (below). The clip tests use one viewport and never build a run list.
- **120** (strip table for GS restart draws): the clip tests draw triangle lists without restart.
- **112** (fans, instance caps, conditional chunks), **121** (zero depth range), **191** (firstInstance) and the other prerast patches do not touch the clip/cull or FS ViewportIndex paths.

## Root causes (from code)

### A. User clip distances through a GS (100 of the 128 clipping failures, and the "state leak")

- The VS/TES lowering `panvk_lower_clip_cull_vs` (`:528`) writes CLIP_DIST0/1 with clip lanes only. It pads unused lanes with +1.0 (`:544`) and puts the cull distances in CULL_DIST0/1.
- A GS is not lowered this way. `vk_spirv_to_nir` runs `nir_merge_clip_cull_distance_vars` (`src/vulkan/runtime/vk_nir.c:210`), which packs the cull array right after the clip array inside CLIP_DIST0/1 (`src/compiler/nir/nir_clip_cull_distance_io_utils.c:472`).
- The GS kernel stores only the components the shader writes (`panvk_gs_lower_intr` store_output, `:1773-1800`).
- The GS passthrough copies whole vec4 record slots (`:2287` loop).
  - Clip lanes past `clip_distance_array_size` therefore keep whatever the prerast arena held. The FS kill tests every lane for `< 0` (`panvk_lower_clip_cull_fs`, `:583`). Whether a case passes depends on what earlier draws left in the arena, which matches "16 pass when run alone". Cases with 4 and 8 clip distances fill whole vec4s and always pass, which is also what beta.13 shows.
  - Cull values packed into the clip lanes are tested as clip distances (a per-pixel kill), and nothing culls the primitive.
- A GS (or TCS) that reads `gl_in[].gl_CullDistance` loads CLIP_DIST lanes by location (`:1865-1892`). The VS/TES records hold +1 padding there, because their cull values are in CULL_DIST0/1.
- GS transform feedback is gathered with `nclip = 0` (`:3724`). With cull moved out of the clip slots it needs the real clip count.

### B. `gl_CullDistance` read in the FS (the 28 `vert`/`vert_tess` `clip_cull_*_fragmentshader_read` cases)

- The FS lowering writes 0.0 into every FS cull array element (`:635`).
- CULL_DIST0/1 carry only the "cull < 0" flag. `panvk_lower_cull_dist_flags_intr` (`:3362`) converts them in the HW and passthrough variants (`:3638`, `:3685`).
- The 023 worklog already lists this as a limit ("FS CullDistance inputs read 0"). CTS checks the interpolated value.

### C. `draw.*shader_viewport_index.fragment_shader_2..16` (60)

- A VS or TES that writes ViewportIndex is drawn through `panvk_gpu_prerast_vs_vp_passthrough_nir` (`:1232`). That shader skips VIEWPORT (`:1269`) because it is not a HW varying here: it shares the layer byte.
- The FS is linked against that passthrough layout (`:4060`).
- `pan_nir_resize_varying_io` turns an FS load of a location missing from the layout into 0 (`src/panfrost/compiler/pan_nir_resize_varying_io.c:44`).
- So the FS always reads `gl_ViewportIndex == 0`. `fragment_shader_1` and `_implicit` pass by luck. The GS passthrough has the same gap (`:2293`); it is not in the CTS list.

## What 193 changes (v10+ only; v6-v9 code paths unchanged)

- **VS/TES:** CULL_DIST0/1 now keep the raw distances. The per-vertex cull flags go to two spare generic slots, `VARYING_SLOT_TEX0/1`, which Vulkan never uses.
- **FS:** kills on clip and flags, and reads `gl_CullDistance` from CULL_DIST0/1. `panvk_lower_cull_dist_flags_intr` becomes a no-op on v10+.
- **GS and TCS inputs, and GS outputs:** the cull array goes back to CULL_DIST0 (`panvk_unmerge_cull_vars`). For a lone input array, only its name (`gl_in.gl_CullDistance` from glslang) marks it as cull; an unnamed one stays clip. GS XFB uses the real clip count.
- **GS passthrough** (`panvk_fixup_gs_passthrough`): sets unused clip lanes to +1 and unused cull lanes to 0, and adds the cull flags.
- **VS vp passthrough and GS passthrough:** write a flat int copy of ViewportIndex to `VARYING_SLOT_TEX2`. The FS reads `gl_ViewportIndex` from that slot, or gets 0 when no copy exists.
- **Cost:** 1-2 extra vec4 varyings for shaders with cull distances, and 1 int varying for VS/TES/GS ViewportIndex writers.
- **Patch layout:** the edits in `panvk_vX_shader.c` replace one line with one line. The helpers live at the end of the file and in the new `panvk_vX_clip_vp.h`, so the jm-v9 hunks applied after csf-v11 still apply with no offset. A normal insertion breaks `jm-v9/003,011,012,018,030` under the strict no-offset rule.

## Host compile check (not a device result)

- **Setup:** host debug build `build/host-f` (panthor noop drm-shim, `PAN_GPU_ID=a867`, Mali-G610 v10; the shim's `b8a3` G615 id fails physical-device init), with NIR validation and shader cache off.
- **Coverage:** 144 generated copies of the CTS clip programs, created as pipelines:
  - clip-only and clip+cull;
  - static and dynamic indexing;
  - with and without FS read;
  - VS only, VS+GS, VS+tess, and VS+tess+GS;
  - plus VS and GS ViewportIndex writers whose FS reads `gl_ViewportIndex`.
- **Result:** all 146 pipelines compile and pass NIR validation. Harness: scratchpad `t193/`.
- **NIR dumps:** the GS passthrough pads clip lane 3 with 1.0 for 3 clip distances, stores 5 raw cull lanes and both flag slots, and the GS records hold no CLIP_DIST1. The FS kills on CLIP0/TEX0/TEX1 and reads `gl_CullDistance[2]` from CULL_DIST0. The FS `gl_ViewportIndex` is a flat load of TEX2 for both the VS and the GS case.
- **Pre-existing debug-only problems** had to be bypassed temporarily to get this far (all reverted). They hit any debug build of the series, not just 193:
  1. `panvk_create_depth_bounds_fs` passes `rs = {0}` (`:4874`), so `panvk_buffer_ubo_addr_format` hits `UNREACHABLE` (DEVICE_DEFAULT). In release builds this is undefined behaviour that happens to work.
  2. `pan_nir_lower_mediump_io` asserts for GS (`src/panfrost/compiler/pan_compiler.c:492`, from `panvk_lower_nir_io` in the GS compile).
  3. NIR compact-var validation fails after the GS/TES kernels change stage (`nir_validate.c:1810`).
  4. The TES path asserts VS attribute locations (`:3557`).
  5. Stale dominance metadata after `panvk_gpu_prerast_lower_vs_outputs` (`:3588`), needs `NIR_DEBUG=invalidate_metadata`.

## Not proven / open

- No hardware run. Expected on device:
  - all 128 clipping cases pass;
  - the 60 `fragment_shader_N` cases pass;
  - no change elsewhere.

  The "state leak" explanation (garbage record lanes) fits the data but was not observed directly.
- **Lone GS/TCS cull input arrays:** a shader that reads only `gl_in[].gl_CullDistance` (no clip input) is recognised only by its variable name. An unnamed SPIR-V array (stripped names, some DXVK output) is still read as clip.
- **`depth_bias_patch_list_tri_line`:** root cause still unknown, and 193 does not address it. What the code shows:
  - The test needs constant depth bias (-700 units, D16) on tessellated triangles drawn in polygon-mode LINE.
  - The ZSD descriptor sets the bias for every primitive type (`csf/panvk_vX_cmd_draw.c:2487-2492`).
  - Tess with LINE mode goes through the nested gpu_prerast polygon kernel (`:4943`, `:4506`). That kernel is preceded by a full iterator wait (`:4321`), so the 132-style race does not apply.
  - `triangle_list_line` (the same polygon kernel, no tess) and the tess `fill` and `point` cases reportedly pass.

  Next step on a device: run the case alone with `PANVK_DEBUG=dump` and compare the green line coverage against the red, to tell missing lines from a missing bias. The case is in `rerun-193.txt` with the other 5 depth_bias cases.
- **Regression risk:** the VS/FS clip-cull lowering changed for all v10+ shaders that use cull distances, and every FS that reads `gl_ViewportIndex`. On the device also rerun the beta.17 52725-case list and PanProbe `clip_cull` / `vs_viewport_index` / `gs_viewport_depth`.
