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

## Fans over the arena: 195 and 196 (2026-10-16)

The plan that was here is implemented, behind a switch. Patches (not committed):
`patches/csf-v11/195-clamp-indirect-gpu-prerast-fans-to-the-arena.patch`,
`patches/csf-v11/196-run-gpu-prerast-fans-in-the-gpu-chunk-loop-opt-in.patch`.
Files: new `src/panfrost/libpan/fan_chunks.cl` and `fan_chunks.h`, `libpan/meson.build` (one line), `csf/panvk_vX_cmd_draw.c`. Nothing that jm-v9/jm-v7 patch is touched (`draw_helper.cl`, `tess_draw.h`, `panvk_instance.[ch]` keep their lines), so the strict `apply-patches.sh` passes.

Status: built for `g615-v11-csf` (BuildID below), reviewed by the author only. **Not run on hardware.**

### Before

- Indexed fan with `primitiveRestartEnable` and more vertices than the arena: `gpu_prerast_split` logged `gpu_prerast_unhandled` and dropped the draw.
- Indirect fan over the arena: lowered as one pass. VS records past `record_capacity` are not written and generated indices stop at 65536, but the raster draw, XFB, GS and polygon kernels used the API counts, so the triangles past the arena were drawn from stale records and indices. Indirect fans over 2048 instances ran as one grid (the G615 hang size).

### 195, default on: clamp

`panlib_fan_clamp_draw` (1 invocation) runs on the compute queue in `gpu_prerast_vs`, after the CS stored the indirect counts and before the lowered VS, only for indirect fans outside the chunk loop. If vertices x instances > record capacity: vertices = min(n, cap), instances = min(k, cap / n), written to the VS params (`vertex_count`, `instance_count`, `vertex.verts_per_instance`), the VS dispatch grid and the generated draw. Every consumer (lowered VS, passthrough, XFB, GS, polygon kernel) reads those words, so the record layout stays consistent: the first triangles are right and the tail is dropped. Draws that fit are unchanged (the kernel returns early). Cost: one tiny dispatch and a `cs_wait_slots` per indirect fan draw on the gpu_prerast path.

Not fixed by 195: an indirect fan over 2048 instances that fits the arena is still one grid; the primitives generated query and the XFB query count the clamped draw, not the API one.

### 196, opt-in: fans in the GPU chunk loop

Switch: `PANVK_FAN_CHUNKS=1` (env), or on Android `setprop debug.mesa.panvk.fan.chunks 1` (os_get_option property fallback). `DEBUG_GET_ONCE_BOOL_OPTION` in the csf draw file, read once per process, default **off**. Off, nothing changes compared to 195.

On, `gpu_prerast_chunk_wanted` takes indirect fans, and direct indexed restart fans with more vertices than the arena (`gpu_prerast_split` hands them on instead of dropping them). Direct fans without restart keep the CPU split (`gpu_prerast_split_fan`), direct restart fans that fit per instance keep the CPU instance split. Restart fans in polygon line/point mode (no GS) stay off: the polygon kernel walks back to the hub for every vertex, quadratic in a 65536 vertex chunk (the pattern of the 120 GS hang).

In `gpu_prerast_chunk_loop`, fans use `panlib_fan_chunk_step` instead of `panlib_chunk_step`, then `panlib_fan_chunk_fill` (grid of (max_verts + 1) / 64 workgroups), then the lowered VS, each followed by `cs_wait_slots`. The per-draw `struct panlib_fan_chunk` and a `max_verts + 1` u32 chunk index buffer come from the cmd desc pool. The chunk `panvk_draw_info` is an indexed fan of that buffer (index size 4, firstIndex 0, restart = API indexed && restart).

`panlib_fan_chunk_step` (plan steps 1-4):
- `verts <= max_verts`: whole instances, at most 2048 (or `max_insts`), chunk = positions 0..verts-1 raw (hub 0, first 1).
- Else one instance per chunk. No cut fan: window [i, i + max_verts), hub i (leading restarts skipped). After a cut: [hub, i .. i + max_verts - 2], i = last vertex of the window before.
- No restart in the window, or the window reaches the end: the whole window is drawn; if it did not reach the end, the fan is cut, its hub is kept in `f->hub`, the next window starts on the last vertex.
- With restart, a window that holds one and does not reach the end ends at its last restart; the fans in it are drawn whole from their own hubs (the chunk keeps the restarts, as 0xffffffff), the next window starts after it with no hub.
- Primitive counts: a workgroup scan like `chunk_scan` with `u_decomposed_prims_for_vertices(FAN)`. The window of a cut fan is scanned from i - 1 (a vertex of the same fan, never a restart), which stands for the hub. So `prims_done`/`prim_base` (GS primitive ID base) and primitives generated are exact.
- c->draw is the 5-word indexed layout (vertexOffset = API vertexOffset, or firstVertex for non-indexed fans, whose chunk indices are the element numbers), `chunk_vertex` 0, `chunk_instance` the instance.
- Termination: each window moves on by at least one position (fresh windows that are cut move max_verts - 1, cut windows max_verts - 2, restart windows past their last restart, which is at or after i). The number of iterations is at most the number of fans plus verts / (max_verts - 2) per instance.

Indices past the index buffer read as restart with restart on (as `chunk_ends_strip` does for strips), else as 0.

### Build

Fresh tree `98c8b2f~1` + `apply-patches.sh --profile g615-v11-csf` (strict, 191 patches including 195/196, no PATCH-DRIFT; checked on two fresh trees). `build-android.sh --profile g615-v11-csf`: OK, no new warnings (only the old `panvk_vX_gpu_queue.c:858` C23 label one). `dist/i/libvulkan_panfrost.so` (worktree removed): BuildID `14cfa0a842f721a060285e9b65e3039fd996d7ef`, SHA256 `de78a41841ae70b24df79e1f41c7a96e58ac3fc2c8c419a850b6d3e31f93d33a`. This ICD has 195 on and 196 off by default.

### Tests (APK `large_draw`, not run)

- `fan_indirect`, `fan_indexed_indirect`: `fan_direct`/`fan_indexed` as indirect draws. Always run. With 196 off they check that the captured triangles are the first ones and that the counters match what was captured (the clamp); with `PANVK_FAN_CHUNKS=1` or `LARGE_DRAW_FAN_CHUNKS=1` in the test's environment they need all triangles.
- Only with the switch: `fan_instanced_indirect` (4 x 5000), `fan_restart_direct`/`_indirect` (indexed fans of the `strip_lens` lengths, one 70001 vertex fan, empty and 1-3 vertex fans), `gs_fan_restart_direct`/`_indirect` (`gs_tri`, exact primitive IDs), `base_inst_fan_windows_indirect`, `base_inst_fan_indirect`.
- Compiles on the host (`gcc -fsyntax-only -Wall`, no warnings). No SPIR-V changes.

The test cannot see the Android property: with only the property set, also pass `LARGE_DRAW_FAN_CHUNKS=1` to the test.

### G615 to do before 196 goes default on

1. Default (switch off): `large_draw` x5, all PASS (`fan_indirect` / `fan_indexed_indirect` print how many triangles the clamp kept).
2. `setprop debug.mesa.panvk.fan.chunks 1` plus `LARGE_DRAW_FAN_CHUNKS=1`: `large_draw` x5, all PASS, no hang, logcat without `gpu_prerast cannot lower`.
3. With the switch: `dEQP-VK.draw.*triangle_fan*`, `dEQP-VK.transform_feedback.*`, `dEQP-VK.geometry.*`, `dEQP-VK.query_pool.*primitives_generated*`, equal to the baseline.
4. Then flip the default in 196 (`DEBUG_GET_ONCE_BOOL_OPTION(fan_chunks, ..., true)`).

### Risks

- 196 is a new GPU chunk loop topology: a planner bug that does not advance would loop the CS forever (device lost). The step kernel is written to always advance (see Termination), but this is from reading only.
- Restart-heavy fans with odd segment layouts can take many chunk iterations (each a compute + raster handoff): slow, not unbounded.
- The leading restart skip is a serial loop in every invocation, as in `panlib_chunk_step`.
- 195 is default on and adds a dispatch to every indirect fan draw on the gpu_prerast path (XFB, GS, polygon mode, viewport index). Low risk (same pattern as `panlib_chunk_setup`), but also not run.

## Limits

- An IDVS fallback chunk (lowered variants missing) still shifts `gl_BaseInstance`, as before.
- Instance offsets are 32-bit. `chunk_instance + invocation` wraps only past 2^32 instances.
- Not run on any device. The G615 must run `large_draw` (all cases, 5 times, with and without `PANVK_FAN_CHUNKS`) before release.
