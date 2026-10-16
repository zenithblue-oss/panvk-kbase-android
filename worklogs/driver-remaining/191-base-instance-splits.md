# 191 gl_BaseInstance on gpu_prerast CPU splits (PROGRESS item 24)

Patch: `patches/csf-v11/191-keep-firstinstance-on-gpu-prerast-cpu-instance-splits.patch` (not committed).
Files: `src/panfrost/vulkan/csf/panvk_vX_cmd_draw.c` (`gpu_prerast_draw_split`, `gpu_prerast_lowers`, `gpu_prerast_vs`, `gpu_prerast_split`, `gpu_prerast_split_fan`, `panvk_cmd_draw`), `src/panfrost/vulkan/csf/panvk_cmd_buffer.h` (`state.gpu_prerast_inst_base`).

Status: built for `g615-v11-csf`, reviewed by the author only. **Not run on hardware.**

## Bug

`gpu_prerast_split` (instance ranges when one instance fits, else per-instance vertex windows) and `gpu_prerast_split_fan` drew each chunk with `instance.base = firstInstance + offset`. In the lowered VS:

- `gl_InstanceIndex` = `load_instance_id` + `vs.base_instance`, and `load_instance_id` = invocation y + `params.chunk_instance` (0 for direct draws). The sum was right.
- `gl_BaseInstance` = `vs.base_instance` = firstInstance + offset. Wrong.
- Per-instance attributes: `emit_vs_attrib` adds `base * stride`, then the ATTRIBUTE descriptor divides `load_instance_id` by the divisor. Element read: firstInstance + offset + i/d instead of firstInstance + (offset + i)/d. Divisor 0 read firstInstance + offset. Divisor 1 was right.

The GPU chunk loop already kept firstInstance in its chunk record and put the chunk start in `chunk_instance`.

## Fix

CPU chunks keep `instance.base`. `gpu_prerast_draw_split` sets `cmdbuf->state.gpu_prerast_inst_base` (saved and restored, so nested splits add up) around the chunk's `panvk_cmd_draw`. `gpu_prerast_vs` stores it as `chunk_instance`, or adds it (`cs_add_imm32`) to the planner's `chunk_instance` when the chunk goes on to the GPU chunk loop (an instance range over 2048 instances).

If a chunk is not lowered after all (VS with side effects, lowered variants missing: IDVS fallback), `panvk_cmd_draw` folds the offset back into `instance.base`, as before the patch. `gpu_prerast_lowers` is the decision `gpu_prerast_draw` makes, factored out.

The offset is cmdbuf state, not a `struct panvk_draw_info` field: the jm-v9 series patches the end of that struct in the shared `panvk_cmd_draw.h`, so a new field there would shift their hunks (`apply-patches.sh` now fails on offsets). The csf-only `panvk_cmd_buffer.h` is not touched by jm-v9/jm-v7.

Generated draws (the raster passthrough) never read it: records are indexed by compute invocation IDs.

## Tests (APK `large_draw`, not run)

New VS `vs_base` (MODE 2 of `large_draw.vert`, needs `shaderDrawParameters`): records (vertex index, instance index, per-instance float attribute, gl_BaseInstance + 1). The attribute buffer holds element e = e. Divisor 3 when `VK_EXT_vertex_attribute_divisor` / `vertexAttributeInstanceRateDivisor` is there, else 1 (then only gl_BaseInstance can catch the bug). firstInstance 3.

- `base_inst_direct`: 4 points x 60000 instances, CPU instance ranges of 16384, each one a GPU chunk loop (the `cs_add_imm32` path).
- `base_inst_indirect`: the same, indirect (GPU chunk loop only; reference).
- `base_inst_windows`: 70001 points x 3 instances, per-instance vertex windows (direct `chunk_instance` store).
- `base_inst_fan`: 4 vertex fan x 5000 instances (fan instance split, 2048 per chunk).
- `base_inst_fan_windows`: 150001 vertex fan x 2 instances (`gpu_prerast_split_fan`, about 14 MB of XFB).

Before 191, all but `base_inst_indirect` should fail (attr and base+1 mismatch from the second chunk on). SPIR-V regenerated with `build_spv.sh` (old shaders unchanged, only the glslang version comment). Compiles with NDK clang `-fsyntax-only`.

G615: `adb shell am start -n dev.zenithblue.panvktest/.MainActivity --es driver bundled --es autorun large_draw` with `LARGE_DRAW_ONLY=base_inst` for a quick run, then the whole suite. Also `dEQP-VK.draw.*instanced*`, `dEQP-VK.pipeline.*vertex_input*divisor*` with XFB or a VS store (CTS mostly takes the IDVS path, so it hardly covers this).

## Still open: plan for fans over the arena

Today (unchanged by 191):
- Indexed fan with `primitiveRestartEnable` and more vertices than the arena: `gpu_prerast_split` logs `gpu_prerast_unhandled` and drops the draw. The CPU cannot see the restart positions (the index buffer is GPU memory at record time).
- Indirect fan over the arena: `gpu_prerast_chunk_wanted` is false (fans are not `gpu_prerast_chunkable`), so it is lowered as one arena pass. VS records past `record_capacity` are not written and generated indices stop at `PANVK_GPU_PRERAST_MAX_INVOCATIONS`, so the triangles past the arena are wrong. Same for indirect fans over 2048 instances (one grid with all of them; the G615 hang size).

Plan: make fans a GPU chunk loop topology (est. 1-2 days + G615; the risk is a chunk loop hang, see 120).

1. `panlib_chunk_step` fan mode: whole instances when `verts <= max_verts` (capped at 2048, as now). Else windows like `gpu_prerast_split_fan`: first window at s = 1, then s += win - 1, n = min(win, verts - s), win = max_verts - 1. Each window is the chunk [hub, s .. s+n-1], `prim_base = s - 1`, n - 1 triangles. New `panlib_chunk_draw` fields: `fan_hub`, `fan_first`, `fan_count`.
2. Chunk index buffer: a per-draw buffer of `max_verts + 1` u32 (cmd desc pool). Each compute iteration fills it with a variant of `panlib_fan_chunk_indices` that reads hub/first/count from `c`, launched after the step kernel. Then `cs_wait_slots`, then the lowered VS. For whole-instance chunks it writes the window [0 .. verts-1] (hub 0, first 1).
3. Fan chunks are always lowered as indexed: `index_size` 4, the chunk buffer, firstIndex 0. vertexOffset is the API vertexOffset (indexed) or firstVertex (non-indexed), so the kernel writes the element number for non-indexed draws. `panlib_chunk_step` writes `c->draw` in the 5-word indexed layout for fans, even when the source is non-indexed. `gpu_prerast_vs` must then take `restart_index` from the chunk (UINT32_MAX), not from the source index size.
4. Restart (indexed + restartEnable): run `chunk_scan` with prim = FAN, as for strips. A window ends at its last restart when it has one, so segments are drawn whole. A segment longer than the window is cut like a fan without restart, with `fan_hub` = the position after the last restart before the window (kept in `c` across iterations). The kernel writes restart indices as 0xffffffff. Primitive count: closed segments count len - 2 each (`u_decomposed_prims_for_vertices(FAN)`), plus the n - 1 triangles of a continuation part; `prims_done` gives exact primitive IDs. Skip leading restarts as `panlib_chunk_step` does for strips.
5. `gpu_prerast_split`: indirect fans go to the chunk loop. Direct indexed restart fans go to the chunk loop with the direct record. Direct fans without restart can keep the CPU split.
6. Tests: `fan_indirect` (150001), `fan_indexed_indirect`, `fan_restart_direct`/`_indirect` (segment lengths like `strip_lens`, one longer than the arena), `fan_restart` with `gs_tri` for primitive IDs, and `base_inst_fan` indirect.

## Limits

- An IDVS fallback chunk (lowered variants missing) still shifts `gl_BaseInstance`, as before.
- Instance offsets are 32-bit. `chunk_instance + invocation` wraps only past 2^32 instances.
- Not run on any device. The G615 must run `large_draw` (all cases, 5 times) before release.
