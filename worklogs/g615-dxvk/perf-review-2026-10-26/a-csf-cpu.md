# CSF CPU overhead review: command recording and submission (reviewer A), 2026-10-26

This is a read-only review. There was no device, no build and no code edit.

- **Tree:** `/home/flux/src/mesa-rc5g`, tag rc5g-114, HEAD `d324a269`.
  - Line numbers are for this tree.
  - Patch attribution comes from `git blame` in the worktree. Commits are named `series: csf-v11/NNN-...`, `csf-v11 NNN: ...`, `jm-v7 NNN: ...`, `series: csf/NNN-...` and `series: kbase-common/...`; `^5a07217f` is the upstream base.
- **Area:** the CPU cost of recording and submitting work on the CSF path (v10/v11: G615 MC6 in the X6 Pro, G710, G715), under DXVK (3.1.1 in the 112 run).
  - Memory footprint (B), GPU-side efficiency (C) and the JM path (D) are other reviews. Overlaps are listed at the end.
- **Build:** release (`scripts/build-android.sh:69`). `mesa_logd` compiles to nothing (`src/util/log.h:79-83`), and asserts are compiled out.
  - Every knob read on a hot path is cached, either in a static or with `DEBUG_GET_ONCE_*`. `PANVK_DEBUG(...)` is a load from a global.
  - So there is no getenv-on-a-hot-path finding.
- **Not repeated:** csf-v11 220-232, and the items marked done in `perf-review-2026-10-22.md`. Two open remainders of that review are covered here:
  - Its item 4 (DESC_STATE is dirtied globally) was fixed only for v6/v7, by jm-v7 097. A2 is the CSF half.
  - Its "flush to transient_offset" (csf-v11 221) never applies to the CS pool. A3 is that remainder.
  - Its K1 is still open and not repeated here: PanVK's Android present calls `DeviceWaitIdle` and copies every frame on the CPU (`panvk_wsi.c:273-293`). It applies only when PanPlay presents through PanVK's Android WSI rather than X11.
- **FO4 caveat:** in-game FO4 on 112 ran at 3.8-7.9 fps under swap pressure (`device-runs/2026-10-24-g615-rc5u-112/results.md`), so FO4 in-game fps cannot show CPU wins yet.
  - Measure with the micro-benches and simpleperf given per item.
  - The FO4 main menu (63-65 fps on 112) is a usable scene for A/B runs.
  - Gains below are estimates from reading the code unless a number is cited.

## Top 10

| # | id | Finding | Kind | Expected gain | Risk | Confidence |
|---|---|---|---|---|---|---|
| 1 | A1 | Depth-only draws re-emit all FS-side state on every draw (since csf-v11 092) | quick | med-high in shadow/prepass passes: about 10 CS instructions, a ZSD and a blend allocation, and a DCD rebuild per draw | low | high |
| 2 | A2 | Any descriptor bind or push dirties both the VS and FS tables on CSF | quick | med-high: VS driver set (18-27 descriptors, 0.6-0.9 KiB) and resource table rebuilt on draws where only FS sets changed | low-med | high (code), med (DXVK pattern) |
| 3 | B1 | KCPU fence wait holds a device mutex across a 20 ms poll, allows one wait per device, and waits on one subqueue at a time | small-med project | med: up to 20 ms extra wake-up latency for a second waiter; up to 12 syscalls cut to 4 per blocking fence wait | med | high (code), med (frequency) |
| 4 | A3 | CS chunks are flushed whole: at least 192 KiB of `dc cvac` per primary command buffer | quick | med for small or many command buffers: about 3000 cache ops fewer per command buffer | low | high (code), med-low (size) |
| 5 | B2 | The X6 Pro is probably ACE-Lite coherent, but the backend does all CPU cache maintenance | project | potentially high: every `dc cvac`/`civac` on the recording and polling paths goes away | high | med |
| 6 | B4 | Userland cache maintenance runs on the recording thread, at End and at descriptor update | med project | med when DXVK's recording thread is the bottleneck | med | med-low |
| 7 | B3 | Emulated timelines force DEFERRED submission | large project | low-med per submit | high | med |
| 8 | A4 | `vkCmdBindVertexBuffers2` dirties VB without comparing | quick | low-med: skips a VS driver-set rebuild on an identical rebind | very low | high (code), low (frequency) |
| 9 | A5 | gpu_prerast eligibility is evaluated three times for every plain draw | quick | low: about 50-150 ns per draw | low | high |
| 10 | M1 | Up to 5 register moves with rarely-changing values on every draw | small project | low: up to 40 B of CS per draw | med | high |

Below the top 10 are B5 (idle subqueues in every command buffer) and M2-M8.

## Quick safe wins

### A1. Depth-only draws re-emit all FS-side state on every draw (csf-v11 092 plus the upstream re-dirty rule)
- **Where:**
  - `src/panfrost/vulkan/panvk_vX_shader.c:6606-6607` (092, `4297cc05`): a NULL fragment-shader bind is replaced by `dev->depth_bounds_fs`.
    - That shader exists whenever the depthBounds feature is enabled (`create_depth_bounds_fs`, `:6531-6534`).
    - DXVK enables depthBounds (perf-review-2026-10-22 item 8).
  - `src/panfrost/vulkan/panvk_cmd_draw.h:533-536`: `fs_required()` returns false for that shader unless the depth-bounds test is on, so `get_fs()` returns NULL.
  - `panvk_cmd_draw.h:603-616` (upstream): `clear_dirty_after_draw` sets FS dirty again after every draw where `fs.shader != get_fs()`.
- **Waste:**
  - Before 092, a pipeline without a pixel shader had `fs.shader == NULL`, so the re-dirty rule never fired.
  - Now it fires on every draw of every pipeline without a pixel shader: shadow maps, depth prepasses and stencil-only passes.
  - FS dirty makes `fs_user_dirty()` true, so each such draw redoes the following, all in `csf/panvk_vX_cmd_draw.c`:
    - `prepare_blend` (`:870-901`): a new BLEND array per draw (`build_blend`, `:841-867`), one entry per RT;
    - `prepare_ds` (`:2504`): a new DEPTH_STENCIL descriptor (`build_zsd`, `:2435`);
    - `prepare_dcd` (`:2840-2925`): `build_dcd_flags` (`:2615-2837`) plus the DCD0/1/2 moves;
    - `set_tiler_idvs_flags` (`:2962`): TILER_FLAGS, and TILER_FLAGS2 on v11;
    - `prepare_fs` (`:2370`) and `prepare_push_uniforms` (`:2412`): the FRAGMENT_SRT/SPD/FAU moves;
    - `prepare_descs` (`:632-642`, a `cmd_prepare_push_descs` walk) and `prepare_fs_desc` (`:602-604`, a memset of the FS descriptor state).
  - `prepare_dcd` moves all three of DCD0, DCD1 and DCD2, because each one's dirty test includes `fs_user_dirty` (`:2846`, `:2890`, `:2899`).
  - In total that is about 10 extra CS instructions (about 80 B) and 1-2 descriptor allocations per draw, for state that has not changed.
- **Expected gain:**
  - An estimated 0.2-0.5 µs of CPU per depth-only draw.
  - The extra CS and descriptor bytes are also gone, so there are fewer BOs to flush (A3).
  - It scales with the number of depth-only draws per frame. Shadow passes in a game like FO4 are likely to be hundreds of draws, but I did not count them here.
- **Risk:** low on CSF.
  - Every input of `fs_required()` is already in `fs_user_dirty()`: the FS bind, RS_RASTERIZER_DISCARD_ENABLE, CB_ATTACHMENT_COUNT, CB_COLOR_WRITE_ENABLES, CB_WRITE_MASKS, MS_ALPHA_TO_COVERAGE_ENABLE, DS_DEPTH_BOUNDS_TEST_ENABLE (added by 092), and RENDER_STATE for `bound_attachments`.
  - So the switch back to "FS used" re-dirties everything even without the per-draw rule.
- **Fix:** on CSF only, replace the re-dirty with a sticky `fs.unused` flag.
  - `clear_dirty_after_draw` sets `fs.unused` instead of setting FS and FS_PUSH_UNIFORMS dirty.
  - `panvk_cmd_draw` recomputes `fs.required` on every draw, before `prepare_descs` (`:6096-6097`). At that point, a draw that finds `fs.unused` set and `get_fs()` non-NULL sets FS and FS_PUSH_UNIFORMS dirty once and clears the flag.
  - Strictly, that transition already sets `fs_user_dirty`, because every input of `fs_required()` is in it. The flag is a safety net.
  - This keeps the old transition semantics and removes the per-draw cost.
  - The macro is shared with JM (`jm/panvk_vX_cmd_draw.c:3239, 3629, 6729`), so guard the change with `PAN_ARCH >= 10` (see the overlaps).
  - Knob: `PANVK_CSF_FS_IDLE_CLEAN=0` restores the per-draw re-dirty.
- **How to measure:**
  - A new `tests/dxvk/vulkan/draw-overhead` mode: 5000 draws per command buffer with a no-FS pipeline into a D32 attachment.
    - CPU ns per vkCmdDraw.
    - CS bytes per draw, from the root chunk size and the chunk count.
  - `PANVK_DEBUG=mem`: cmd_cs and cmd_desc BO counts per frame.
  - simpleperf: `build_zsd`, `build_dcd_flags`, `build_blend` and `set_tiler_idvs_flags` should drop out of the depth-only profile.
- **Confidence:** high for the behaviour, which is traced through the code. Medium for the size.

### A2. Any descriptor bind or push dirties both the VS and FS tables on CSF (upstream TODO, left open by jm-v7 097)
- **Where:**
  - `src/panfrost/vulkan/panvk_vX_cmd_desc_state.c:400-418`, `CmdBindDescriptorSets2KHR`. On PAN_ARCH >= 9 it does:

    ```c
    (void)changed;
    gfx_state_set_dirty(cmdbuf, DESC_STATE);
    ```

    - That is at `:413-415`, from jm-v7 097 (`30f905ca`), which wired the per-set mask for v6/v7 only.
    - Upstream TODO at `:406`: "Invalidate only if the shader tables are disturbed".
  - The push paths set DESC_STATE for any set: `CmdPushDescriptorSet2KHR` (`:454-478`) and `CmdPushDescriptorSetWithTemplate2KHR` (`:481-517`).
- **Waste:**
  - `cmd_desc_state_bind_sets` (`:33-67`) already returns which sets changed, by handle or dynamic offset, but the CSF branch ignores it.
  - DESC_STATE is in both `vs_desc_dirty` (`csf/panvk_vX_cmd_draw.c:423-439`) and `fs_desc_dirty` (`:586-591`). So any bind, even an identical one, rebuilds four things on the next draw:
    - The VS driver set, `prepare_vs_driver_set` (`:347-420`).
      - It holds 16 ATTRIBUTE descriptors (`emit_vs_attrib` with a format lookup for each enabled attribute, and a fixed OOB descriptor for the rest, `:374-385`). It also holds a dummy sampler, the dynamic buffers and one BUFFER per vertex buffer: `desc_count = dyn + MAX_VS_ATTRIBS + 1 + vb_count` (`:360-361`).
      - Each descriptor is 32 B.
    - The VS resource table, `cmd_prepare_shader_res_table` (`panvk_vX_cmd_desc_state.c:310-366`).
    - The FS driver set, `prepare_fs_driver_set` (`:547-583`), which includes the varying ATTRIBUTE descriptors from `emit_varying_descs` (`:498-544`).
    - The FS resource table.
- **Expected gain:**
  - When only FS resources change (the usual per-draw texture change), the VS side stops being rebuilt: 18-27 descriptors packed and 0.6-0.9 KiB of descriptor pool. The reverse holds when only VS resources change.
  - Identical rebinds become free.
  - Estimated 0.3-1 µs per affected draw, plus fewer cmd_desc BOs.
- **Risk:** low-medium. Three couplings must be kept:
  - Dynamic-buffer offsets live in the driver sets (`cmd_fill_dyn_bufs`).
  - The driver-set builders set DESC_STATE themselves (`:417`, `:581`) to force their resource table to be rebuilt.
  - Direct DESC_STATE setters (meta save/restore, ...) must mean "all sets". jm-v7 097 handles this with `v7_dirty_sets_valid` (`panvk_vX_cmd_desc_state.c:69-93`).
- **Fix:**
  1. A bind sets DESC_STATE only if `changed` is non-zero, and keeps `dirty_sets |= changed`. A push adds the pushed set to `dirty_sets`.
  2. Each side rebuilds only when its own state changed:
     - VS: when `dirty_sets & vs->desc_info.used_set_mask` is non-zero, or VS/VB/VI/BASE_INSTANCE is dirty.
     - FS: when `dirty_sets & fs->desc_info.used_set_mask` is non-zero, or `fs_user_dirty`, or VS is dirty (for the varyings).
  3. A driver set is rebuilt only when VB/VI/VS/BASE_INSTANCE changed, or when a used set's dynamic offsets changed. Otherwise only the resource table is re-packed: at most 8 RESOURCE entries of 16 B (`genxml/v10.xml:2084`).
  - Knob: `PANVK_CSF_DESC_DIRTY=0` restores the global dirty, as `PANVK_V7_DESC_DIRTY` (`panvk_v7_knobs.h:88-92`) does for v6/v7.
- **How to measure:**
  - Draw-overhead bench modes "rebind one FS set per draw", "push one set per draw" and "identical rebind per draw".
  - simpleperf: `prepare_vs_driver_set`, `emit_vs_attrib` and `cmd_prepare_shader_res_table`.
  - `PANVK_DEBUG=mem`: cmd_desc count.
  - Check DXVK 3.1.1's binding pattern on PanVK, which has no VK_EXT_descriptor_buffer, with an API dump. The gain scales with how often it binds or pushes a subset of the sets.
- **Confidence:** high for the behaviour. Medium for how often DXVK hits it.

### A3. CS chunks are flushed whole: at least 192 KiB of `dc cvac` per primary command buffer (upstream chunking; csf-v11 221 cannot reach it)
- **Where:**
  - `csf/panvk_vX_cmd_buffer.c:1150-1162`, `alloc_cs_buffer` (upstream): every CS chunk is a 64 KiB allocation from the CS pool, whose slab is also 64 KiB (`:1385-1392`).
  - `panvk_mempool.c:100-118`: a slab-sized allocation leaves 0 bytes free, so it never replaces `transient_bo`. The CS pool therefore never has a transient BO, and the 221 trim (`:283-299`) never applies to it.
  - `panvk_pool_flush_maps` (`:293-307`) then cleans every CS BO whole at `EndCommandBuffer` (`csf/panvk_vX_cmd_buffer.c:648`).
  - All three subqueues are always non-empty:
    - `finish_cs` (`:474-569`) runs on every subqueue (`:637-646`) and always emits instructions: a wait on all scoreboard slots (`:481`), the error save in a primary (`:486-502`) and the CLEAN flush (`:508-516`).
    - For every primary, `finish_queries` (`:572-614`, upstream; called at `:630-632`) also emits into COMPUTE (`PANVK_QUERY_TS_INFO_SUBQUEUE`, `panvk_cmd_ts.h:19`).
    - Its barrier makes VT and FRAG wait and signal unconditionally (`emit_barrier_csf`, `:1001-1044`). Only the COMPUTE-side wait is conditional (`:1050-1055`).
- **Waste:**
  - Every primary command buffer gets at least 3 × 64 KiB = 192 KiB of `dc cvac`, i.e. 3072 cache lines, however little it records.
  - In a large command buffer, the waste is the unwritten tail of each subqueue's last chunk, up to 1024 lines each.
  - Those lines were cleaned at their previous use and not written since, so cleaning them again does nothing useful.
  - The 112 FO4 log (`device-runs/2026-10-24-g615-rc5u-112/raw/fallout4/panvk-mem-27889.log`) shows that CS slabs dominate the pools being flushed. Live cmd_cs BOs (all 64 KiB) numbered 170-211, against 58-70 cmd_desc BOs, in the menu (22-282 s). In game (320-402 s) the figures were 1223-1765 against 474-716.
- **Expected gain:**
  - About 3000 cache-maintenance ops fewer per small command buffer, and up to 1024 per subqueue for large ones.
  - Each op is a cache-maintenance broadcast to the point of coherency. Estimated at tens of µs per small command buffer; this is not measured.
  - DXVK's upload/init command buffers and submit_stress-style loops gain the most.
- **Risk:** low.
  - Only lines that were never written are skipped.
  - Builder patches (`cs_patch_maybe`, chunk-link fixups) land inside written ranges before End.
- **Fix:**
  - Record `(bo, offset, bytes_written)` for each CS chunk: when the builder moves to a new chunk (`alloc_cs_buffer`), and at `cs_end` for the last chunk.
  - At End, flush only those ranges, instead of calling `panvk_pool_flush_maps(&cmdbuf->cs_pool)`. Secondaries take the same path.
  - Knob: `PANVK_CS_FLUSH_USED=0` restores the whole-BO flush.
- **How to measure:**
  - `tests/dxvk/vulkan/submit_stress`: submits/s, and simpleperf on `util_flush_range_no_fence` and `panvk_pool_flush_maps`.
  - vkcube fps on the X6 Pro, which is one draw per frame and so mostly submit cost.
  - An upper bound for all CPU cache maintenance today is `PANVK_DEBUG=no_wb_mmap` (`panvk_device.h:329-334`). It uses WC mappings and issues no dc ops. It is not identical, because WC also changes the cost of the writes themselves.
- **Confidence:** high for the behaviour. Medium-low for the size.

### A4. `vkCmdBindVertexBuffers2` dirties VB without comparing (upstream)
- **Where:**
  - `src/panfrost/vulkan/panvk_vX_cmd_draw.c:1137-1181`: `gfx_state_set_dirty(cmdbuf, VB)` at `:1180` runs unconditionally.
  - Strides go through `vk_cmd_set_vertex_binding_strides`, which already compares (`SET_DYN_VALUE`, `src/vulkan/runtime/vk_graphics_state.c:2161-2169`).
- **Waste:** VB dirty makes `vs_desc_dirty` true (`csf/panvk_vX_cmd_draw.c:438`), so the whole VS driver set and resource table are rebuilt even when every binding is unchanged. This happens when a contiguous range is rebound and only some of its slots changed, or when the same buffers are rebound for each draw.
- **Expected gain:** low-medium. It depends on the rebinding pattern; check it with an API dump.
- **Risk:** very low.
- **Fix:** compare address and size for each binding, and set VB dirty only if one differs. Knob: `PANVK_VB_DIRTY_CMP=0` restores.
- **How to measure:** a draw-overhead mode "same vertex buffers rebound per draw", plus simpleperf on `prepare_vs_driver_set`.
- **Confidence:** high for the behaviour. Low for how often it happens.

### A5. gpu_prerast eligibility is evaluated three times for every plain draw (csf-v11 046/065/078/094)
- **Where:** `panvk_cmd_draw` in `csf/panvk_vX_cmd_draw.c` (`:6052-6146`) makes three calls:
  - `gpu_prerast_wanted` at `:6076` (078, `74abad78`);
  - `gpu_prerast_draw` at `:6122` (046, `32aff138`), for every draw that is not itself generated;
  - `gpu_prerast_needed` again at `:6125`.
- **Waste:** a plain draw (no GS, XFB, tessellation, viewport select, polygon mode or VS side effects) goes through the following:
  - `gpu_prerast_needed` (`:5474-5486`), which calls `gpu_prerast_poly_mode` (`csf/panvk_gpu_prerast.c:53-68`).
  - `gpu_prerast_draw` (`:4368-4376`), which does this:
    - it calls `gpu_prerast_chunk_wanted` (`:5574-5602`), which in turn calls `gpu_prerast_max_verts` (`:5499-5511`) and `panlib_chunk_topology`;
    - it then calls `gpu_prerast_lowers` (`:4353-4364`), which copies the whole `panvk_draw_info`, calls `chunk_wanted` again and calls `gpu_prerast_can_lower` (`panvk_gpu_prerast.c:72-92`); `can_lower` calls `poly_mode` again and returns false.
  - `gpu_prerast_needed` once more.
- **Expected gain:** low, about 50-150 ns per draw (estimate).
- **Risk:** low.
  - `can_lower`'s first test (`panvk_gpu_prerast.c:85-92`) needs one of DEBUG(GPU_PRERAST), poly, gs, xfb, side_effects or vp_select.
  - `wanted` covers all of those except the debug flag. The VS-side `vp_scissor_loc` is covered through `panvk_prerast_viewport_shader`, and XFB through `gpu_prerast_xfb_active` (`:3988-3997`).
- **Fix:** compute `wanted` once per API draw. When `!wanted && !PANVK_DEBUG(GPU_PRERAST)`, skip `gpu_prerast_draw` and the second `needed` call. Knob: `PANVK_PRERAST_FAST_SKIP=0` restores.
- **How to measure:** self time of these functions in the draw-overhead bench (simpleperf).
- **Confidence:** high.

### M1. Up to 5 register moves with rarely-changing values on every draw (upstream; ranked 10)
- **Where:** in `csf/panvk_vX_cmd_draw.c`:
  - `launch_draw`: GLOBAL_ATTRIBUTE_OFFSET = 0 (`:3303`) and INSTANCE_OFFSET = 0 (`:3316`) on every draw, and again at `:3570` and `:3599` on the indirect path.
  - VARY_SIZE on every draw (`:3133`). It changes only with the VS.
  - `prepare_index_buffer` (`:2928-2936`): INDEX_BUFFER_SIZE and INDEX_BUFFER on every indexed draw. They change only with the bound index buffer.
  - Not counted here: INDEX_COUNT, INDEX_OFFSET, INSTANCE_COUNT and VERTEX_OFFSET (`:3305-3310`), which carry per-draw values.
- **Waste:**
  - Up to 5 instructions (40 B) of CS per indexed draw, and 3 (24 B) per non-indexed draw.
  - That means more chunks to flush (A3) and more CS for the firmware to parse.
- **Expected gain:** low, about 20-40 ns of CPU per draw (estimate) plus the CS bytes.
- **Risk:** medium. A missed invalidation would reuse a stale register.
- **Fix:**
  - Keep a per-command-buffer shadow of the IDVS staging registers that draws write, and skip a move whose value is unchanged.
  - Invalidate the shadow at Begin, after every CALL into a secondary, around vk_meta and meta draws, after render-context splits (`split_render_ctx`), and wherever other code writes those registers.
  - Knob: `PANVK_CSF_SR_SHADOW=0` restores.
- **How to measure:** CS bytes per draw and ns per draw in the draw-overhead bench.
- **Confidence:** high for the behaviour.

## Bigger projects

### B1. KCPU fence wait: device mutex held across a 20 ms poll, one wait per device, one subqueue at a time (kbase-common files, csf/007)
- **Where:**
  - `src/panfrost/lib/kmod/kbase_kmod.c:1214-1354`, `kbase_kmod_csf_wait_cqs64` (kbase-common files, `3a143a96`).
  - Its caller is `csf/panvk_vX_gpu_queue.c:966-985`, which waits in 20 ms slices inside `kbase_subqueue_wait_seqno` (`:771-1003`).
  - `kbase_wait_sync_targets` (`:3075-3098`, csf/007-gpu-queue-kbase `980b268f`). It is the CPU wait behind every submitted kbase sync (registered at `:3270-3272`).
  - csf-v11 059 (`:3065-3071`, on by default) turns a submit's waits on same-queue syncs into GPU waits (`:3136-3147`). vkWaitForFences, vkWaitSemaphores, idle waits and the remaining submit waits still come here.
- **Waste:**
  1. **Mutex held across the poll.** `kcpu.lock` covers the whole device. It is taken at `:1228` and held across `kbase_kcpu_poll_fence(..., timeout_ns)` (`:1256`, `:1324`), i.e. for up to a 20 ms slice.
     - A second waiter whose target completes while it waits for that mutex is not woken until the holder's poll returns.
  2. **One outstanding wait per device** (`:1248-1254`). A waiter with a different target gets -1 and falls back to `kbase_kmod_csf_wait_event` (`:1141-1185`). That is a ppoll on the device fd, which wakes on any CSF notification, then a `read()` of one notification.
     - The code says the fd is blocking (`:1165-1168`). If `read()` does block when no record is pending, two such waiters that both see POLLIN for one record race. The loser sleeps in `read()` until the next notification.
     - As I remember the r-series kbase source (not on this machine), its CSF `read()` returns at once with an EVENT record even when none is pending. Whether the race exists depends on the vendor kernel, so it needs a check before it counts.
  3. **One subqueue at a time.** A fence wait goes through the subqueues one after another (`:3087-3095`). Each subqueue wait that blocks costs 4 syscalls:
     - two KCPU_QUEUE_ENQUEUE ioctls (`:1310-1311`), because shipping kbase takes one command per enqueue;
     - a poll of the returned sync-file fd, and its close (`:1324-1328`).

     A fence that waits on all three subqueues can pay this three times: up to 12 syscalls.
- **Expected gain:**
  - Frame pacing: the up-to-20 ms extra wake-up latency for concurrent waiters goes away, as does the possible blocking-read race. The concurrent waiters would be, for example, DXVK's finish thread and a WSI or app wait.
  - Syscalls per blocking fence wait drop from up to 12 to 4.
  - The plain CPU gain is moderate.
- **Risk:** medium. The hard parts are KCPU queue lifetime, fence fd ownership, and the error paths that disable KCPU (`:1335-1350`).
- **Fix:**
  - **Poll without the lock.** Under the lock, enqueue or reuse the wait and `dup()` the fence fd (or refcount it). Unlock, poll, then re-lock to retire the wait.
  - **One wait command for all subqueues.** Wait on all pending subqueue targets with a single CQS_WAIT command (`nr_objs` up to 3). `kbase_kmod_csf_export_cqs64_fence` (`:1357-1430`) already enqueues exactly that shape, on its own KCPU queue.
  - **A small pool of KCPU queues**, for example 4, each with at most one outstanding wait. A single KCPU queue executes in order, so a second wait queued behind the first would be blocked by it; that is why the current code allows only one.
  - **Non-blocking event fallback**, if the check in point 2 finds that `read()` blocks: an O_NONBLOCK read, or a single reader with a condvar broadcast.
  - Knob: `PANVK_KBASE_KCPU_MULTI=0` restores.
- **How to measure:**
  - A two-thread variant of submit_stress: thread B waits on submit N while thread A waits on N+1.
    - Wake-up latency distribution: time after the wait returns, minus GPU completion taken from a timestamp query.
    - Syscalls per wait (`strace -c`, or simpleperf syscall events).
  - DXVK frame-time standard deviation in the FO4 menu.
- **Confidence:** high for the code behaviour. Medium for how often DXVK 3.1.1 plus PanPlay waits from two threads at once.

### B2. The X6 Pro is probably ACE-Lite coherent, but the backend does all CPU cache maintenance (kbase-common files)
- **Where:**
  - `src/panfrost/lib/kmod/kbase_kmod.c:494-508` (`3a143a96`).
  - `to_kbase_mem_flags` (`:2155-2193`) requests BASE_MEM_COHERENT_SYSTEM only for `PAN_KMOD_BO_FLAG_IO_COHERENT`.
  - `cpu_gpu_coherent` is read from the kernel's returned flags (`req.out.flags`, `:2622`, `:2664`, `:2668-2670`), but only the MEM_SYNC path uses it (`:2905-2913`).
  - The userland path, `pan_kmod_queue_bo_map_sync` (`src/panfrost/lib/kmod/pan_kmod.c:321-345`), checks only the pan_kmod flag.
- **Why:**
  - **The encoding.** The comment says the raw mode is "0 = none, 1 = ACE-Lite, 2 = ACE". But:
    - the tree's own `mali_base_kernel.h:575-576` and `:617-620` say this property is the *selected* coherency mode;
    - the coherency-protocol encoding in `include/drm-uapi/panthor_drm.h:266-278` and `panfrost_drm.h:242-244` is ACE_LITE = 0, ACE = 1, NONE = 31.
  - **The X6 Pro value.** The vulkaninfo diff in `docs/universal/v11/README.md:56` shows shaderDeviceClock false on the MC6 dev device and true on MC2.
    - shaderDeviceClock is `timestamp_device_coherent` (`panvk_vX_physical_device.c:702`), which is `coherency_mode != 0`. So the X6 Pro reports 0, which is ACE-Lite (I/O coherent) under that encoding.
    - Two caveats. The property read defaults to 0 if the property is missing (`kbase_kmod.c:502-503`). And the two vulkaninfo runs used different builds (beta.14 and beta.15).
  - **What the driver does anyway.** It treats every BO as non-coherent and performs all of this maintenance:
    - the EndCommandBuffer pool cleans (A3);
    - a clean on every `vkUpdateDescriptorSets*` (`descriptor_set_flush`, `panvk_vX_descriptor_set.c:1047-1069`);
    - ring-entry cleans (`gpu_queue.c:499`, `:708`);
    - a seqno-cell invalidate on every poll (`:829`, `:2514-2519`);
    - the query and BC paths.
- **Expected gain:** potentially the largest single CPU item on the X6 Pro, since every `dc cvac`/`civac` leaves the recording and polling paths. It is not measured. `PANVK_DEBUG=no_wb_mmap` gives an upper bound today.
- **Risk:** high without proof on the device. If the platform is not coherent for these mappings, the GPU reads stale command streams and descriptors (faults, corruption). The GPU-side snoop cost for CS and descriptor fetches is small.
- **Fix:**
  1. Log the raw `coherency_mode` once at info level. This needs a device run.
  2. If it is 0 or 1:
     - Request BASE_MEM_COHERENT_SYSTEM for the CPU-written pools: CS, descriptors, descriptor pools, rings, seqno/event memory.
     - Honour the kernel's answer per BO. `cpu_gpu_coherent` comes from the returned flags; kbase is expected to strip the flag on a non-coherent system, which needs checking in the vendor kernel.
     - For a coherent BO, set `PAN_KMOD_BO_FLAG_IO_COHERENT` so `pan_kmod_queue_bo_map_sync` returns early, and skip `kbase_cache_clean_range` and `kbase_cache_invalidate_range` on coherent rings and cells.
  3. Separately, fix the comment and `timestamp_device_coherent`. That is a correctness issue; see the overlaps.
  - Knob: opt-in. `PANVK_KBASE_COHERENT_POOLS=1` enables it; `=0`, the default until a device A/B passes, is today's behaviour.
- **How to measure:**
  - Correctness: CTS `dEQP-VK.memory.*` and `dEQP-VK.synchronization.*`, the 39-test PanProbe suite, and vkcube.
  - CPU:
    - simpleperf share of the dc ops (`kbase_cache_clean_range`, `util_flush_range_no_fence`, `util_flush_inval_range_no_fence`);
    - submit_stress;
    - the draw-overhead bench.
- **Confidence:** medium. The encoding mismatch is well supported by the in-tree headers. The platform's actual coherency is not verified.

### B3. Emulated timelines force DEFERRED submission (kbase-common 004)
- **Where:**
  - `src/panfrost/vulkan/panvk_physical_device.c:1121-1166` (`a05a6df9`): a binary `kbase_cpu_sync`, which already has WAIT_PENDING (`:1130`), plus `vk_sync_timeline_get_type`.
  - Given that, the runtime picks EMULATED (`src/vulkan/runtime/vk_device.c:49-90`), which means `VK_QUEUE_SUBMIT_MODE_DEFERRED` (`:232-234`).
- **Waste:**
  - Every mode pays for `vk_queue_submit_alloc` (`vk_queue.c:897`). On top of that, each vkQueueSubmit2 in DEFERRED mode pays for:
    - `vk_queue_push_submit` (`:550-560`): the queue mutex and a list add;
    - `vk_device_flush` (`vk_device.c:371-392`), which passes over every queue until no queue makes progress, so at least twice after a submit. Each pass takes each queue's submit mutex (`vk_queue.c:681`) and checks each emulated-timeline wait with a zero-timeout `vk_sync_wait` (`:689-699`).
  - Every timeline signal pays for `vk_sync_timeline_alloc_point_locked` (`vk_sync_timeline.c:177-234`): a gc, then either reuse of a free point or a zalloc plus `vk_sync_init`. For a `kbase_cpu_sync`, the init is an `mtx_init` and a condition-variable init (`panvk_physical_device.c:657-678`).
  - The gc (`vk_sync_timeline.c:333-372`) polls the pending points in order with a zero-timeout wait, until it reaches a busy one. Each poll of a pending point goes through `kbase_wait_sync_targets`, which invalidates and reads the seqno cells.
  - CPU timeline waits walk the points (`:504`).
- **Expected gain:** low-medium: µs per submit, and fewer lock hops on the submit and finish threads.
- **Risk:** high: sync_file import/export, WSI, and CTS synchronization.
- **Fix:**
  - Add a native kbase timeline vk_sync type with TIMELINE | CPU_WAIT | GPU_WAIT | GPU_MULTI_WAIT | WAIT_PENDING | CPU_SIGNAL:
    - it keeps (value → per-subqueue seqno targets) pairs in a small ring;
    - a CPU wait waits on those targets (with B1);
    - a GPU wait reuses `kbase_gpu_waits`.
  - Without WAIT_BEFORE_SIGNAL, the runtime then picks ASSISTED, which means THREADED_ON_DEMAND (`vk_device.c:76-89`, `:236-245`). Submission is immediate unless a wait-before-signal actually happens.
  - Knob: `PANVK_KBASE_NATIVE_TIMELINE=0` restores the emulated timeline.
- **How to measure:**
  - submit_stress submits/s and vkcube fps.
  - CTS `dEQP-VK.synchronization.timeline_semaphore.*` and `synchronization2.*`.
  - simpleperf on `vk_queue_*` and `vk_sync_timeline_*`.
- **Confidence:** high for the mechanism. Medium for the gain.

### B4. Userland cache maintenance runs on the recording thread (pan_kmod, upstream design)
- **Where:**
  - With userland cache ops (the default, `PANVK_KBASE_USER_CACHE_SYNC`, `kbase_kmod.c:1934-1939`), `pan_kmod_queue_bo_map_sync` (`pan_kmod.c:334-352`) issues the dc ops immediately, on the caller's thread. Only the closing barrier is batched per submit (`:347-351`).
  - The callers are EndCommandBuffer (`csf/panvk_vX_cmd_buffer.c:648-649`) and `descriptor_set_flush`, which runs on every `vkUpdateDescriptorSets` and `vkUpdateDescriptorSetWithTemplate` (`panvk_vX_descriptor_set.c:1035-1069`).
- **Waste:**
  - In DXVK those calls run on the recording thread, while vkQueueSubmit2 runs on DXVK's submit thread. Confirm the thread placement with per-thread simpleperf.
  - The submit thread already calls `pan_kmod_flush_bo_map_syncs` before ring emission (`gpu_queue.c:3159`).
- **Expected gain:** medium when the recording thread is the bottleneck. This moves the work rather than removing it; A3 removes part of it, and B2 supersedes this item where coherency holds.
- **Risk:** medium. Every path that hands CPU-written memory to the GPU must run `pan_kmod_flush_bo_map_syncs` first.
  - Submit (`gpu_queue.c:3159`) and the queue init stream (`:1834`) already do.
  - Sparse binds, other queues, and paths that clean with `kbase_cache_clean_range` directly need an audit.
- **Fix:**
  - Add a deferred-userland mode that queues the ranges, merged as the kernel path already does (`pan_kmod.c:357-381`):
    - pool BOs per command buffer, handed to the queue at submit;
    - descriptor sets per descriptor pool, flushed at the next submit on any queue (the GPU only reads sets after a submit).
  - Run the dc ops in `pan_kmod_flush_bo_map_syncs`.
  - Knob: `PANVK_DEFER_CACHE_OPS=0` restores.
- **How to measure:** per-thread CPU time (simpleperf `-t`), and a draw-overhead mode with one `vkUpdateDescriptorSetWithTemplate` per draw.
- **Confidence:** medium-low. The thread layout and the size are not measured.

### B5. Idle subqueues still get a chunk, a ring entry and flushes in every primary (upstream; below the top 10)
- **Where:**
  - `csf/panvk_vX_cmd_buffer.c:617-652` (End).
  - `:572-614`: `finish_queries` runs on COMPUTE for every primary (`:630-632`).
  - `gpu_queue.c:2910-2912`: `cs_is_empty` (`genxml/cs_builder.h:270-274`) is never true after `finish_cs`, which always emits (A3).
- **Waste:** in every primary command buffer, a subqueue with no recorded work still gets:
  - a 64 KiB chunk and its flush (A3);
  - a ring entry (`gpu_queue.c:479-768`): a reserve, the FLUSH_CACHES + CALL + seqno-signal sequence, a clean, and a doorbell or a KICK ioctl;
  - a GPU-side CALL and its flushes.
- **Expected gain:** low for DXVK, where barriers usually touch all three subqueues. Medium for compute-only or transfer-only command buffers.
- **Risk:** medium: barrier and sync-point bookkeeping, and secondaries.
- **Fix:**
  - Skip `finish_queries` when the command buffer cannot have put a timestamp node on `ts_done_chain`.
    - Nodes get there in two ways:
      - from a timestamp write (`panvk_vX_cmd_query.c:480-484`);
      - from `handle_deferred_queries` (`csf/panvk_vX_cmd_draw.c:7497-7548`), which runs whenever fragment jobs are issued (`:7568`, `:7629`) and moves the deferred nodes over from `ts_chain`.
    - The second way can carry nodes written by an earlier command buffer whose render pass this one resumes. So the flag must also be set by a resume, and ORed in from executed secondaries.
    - Today `contains_timestamp_queries` is set only at `panvk_vX_cmd_query.c:592` and read only at `:629`, so CmdExecuteCommands does not propagate it.
    - Every primary that runs `finish_queries` drains `ts_done_chain` at its end (`:609-612`). So a primary that adds nothing finds the chain empty.
  - Skip `finish_cs` on a subqueue that recorded nothing and has no relative sync point.
  - Knob: `PANVK_CSF_SKIP_IDLE_SUBQUEUES=0` restores.
- **How to measure:** submit_stress with a compute-only variant, ring entries per submit, and vkcube fps.
- **Confidence:** high for the behaviour. Low for how often DXVK hits it.

## Minor items (unranked)
- **M2 Push constants.**
  - `CmdPushConstants2KHR` (`panvk_vX_cmd_push_constant.c:110-132`; upstream, with the tessellation and geometry stage bits from csf-v11 065) dirties the VS/FS push uniforms without comparing the bytes or checking `used_push_consts`.
  - Each dirty allocates a new FAU block and repacks all the sysvals (`:10-77`).
  - Fix: memcmp before the memcpy, and dirty a stage only if the range overlaps its used push constants. Knob: `PANVK_PUSH_CMP=0`. Low gain.
- **M3 Descriptor pools.**
  - `panvk_desc_pool_allocate_set` finds a free slot with `__bitset_ffs` starting from word 0 (`panvk_vX_descriptor_set.c:645-646`).
  - `ResetDescriptorPool` (`:757-768`) walks all `max_sets` slots and frees each set's VMA range one at a time.
  - Fix: a next-free hint, and a reset that re-initialises the VMA heap. Low.
- **M4 WAIT_ANY spins.**
  - `kbase_cpu_sync_wait_many` with WAIT_ANY (`panvk_physical_device.c:892-926`) spins on `sched_yield()` for the whole wait. That covers vkWaitForFences with waitAll=false and vkWaitSemaphores with ANY.
  - Fix: block on the earliest target (with B1), or on a per-device condvar. Low for DXVK.
- **M5 dsb per submit.**
  - Each touched subqueue in a submit pays:
    - one `dsb sy` in `kbase_cache_clean_range` (`gpu_queue.c:174`, once per ring entry);
    - one in `kbase_subqueue_publish` (`:755`);
    - on the doorbell path, another (`:762`).
  - That is up to 9 for a 3-subqueue submit.
  - Fix: emit all entries, then do one clean and one dsb. Write all CS_INSERTs, one dsb. Ring the doorbells, one dsb. That is 3 in all. Low.
- **M6 Reset bookkeeping.** On every reset, `panvk_reset_cmdbuf` re-initialises one u_trace per subqueue (`csf/panvk_vX_cmd_buffer.c:1315-1319`) and memsets the whole command-buffer state (`:1324`). Upstream and minor.
- **M7 Dispatch.**
  - Each dispatch allocates a fresh LOCAL_STORAGE descriptor (`csf/panvk_vX_cmd_dispatch.c:146`), even when the shader uses neither TLS nor WLS.
  - A shader with WLS also gets a fresh WLS allocation per dispatch (`:165-171`). The upstream TODO there is about reusing it.
  - Each dispatch writes GLOBAL_ATTRIBUTE_OFFSET and JOB_OFFSET_X/Y/Z as zero (`:240-241`, `:257-259`).
  - Upstream. Low; DXVK is light on compute.
- **M8 Imported sync files are waited on the submit thread** (csf/007).
  - At submit, a wait is turned into a GPU wait only if its sync belongs to this queue or has no owner (`gpu_queue.c:3136-3147`). Any other wait blocks the submitting thread in `vk_sync_wait(..., UINT64_MAX)` (`:3149-3150`).
  - An imported sync file is owned by its sync-file waiter (`panvk_physical_device.c:1039-1053`), so it always takes the blocking path.
  - `vk_common_AcquireImageANDROID` imports the acquire fence that way (`vk_android.c:517-527`). PanVK's own Android WSI imports nothing at acquire (`panvk_wsi.c:223-233`).
  - So this matters only on paths that import sync files every frame. I did not establish which WSI path PanPlay's DXVK uses.
  - Fix: a KCPU FENCE_WAIT followed by a CQS_SET (`mali_base_csf_kernel.h:154-156`) on a per-queue CQS that the CS waits on, which frees the submit thread. Knob: `PANVK_KBASE_GPU_FENCE_WAITS=0`. Low until a frame-path import is shown.

## Checked, no action
- **Tiler heap renewal** (csf-v11 224 plus the `panvk_heap` thread):
  - The spare context is created on that thread, and retired contexts are destroyed there (`gpu_queue.c:2168-2338`).
  - On the submit thread, a renewal is one descriptor write and a 64 B clean (`:2547-2598`), with no drain.
  - The only wait is the backpressure path at 4× the budget (`:3222-3243`), and it waits only for the first entries after the retirement.
- **`panvk_mem_report`** when `PANVK_DEBUG=mem` is off costs one relaxed atomic load per submit (`panvk_priv_bo.c:339-341`). There are no mem-stat atomics on the draw path.
- **Knob reads** are all cached, for example `gpu_queue.c:86-112`, `:3065-3066`, `panvk_mempool.c:287-291` and `panvk_cmd_pool.c:80-160`.
- **Device TLS at End** (csf-v11 230): `get_device_tls` (`csf/panvk_vX_cmd_buffer.c:298-372`) runs only for command buffers that use TLS (`:383`). It takes `dev->tls.lock` and, with the default shrink knob, reads the clock (`:310-314`). Cheap.
- **Draw state** other than A1, A2 and A4 is dirty-gated: blend, ZSD, DCD, IDVS flags, viewport/scissor (`csf/panvk_vX_cmd_draw.c:961`, `:1079`), primitive size (`:1167`), OQ and TLS.
  - Shader binds compare against the bound shader (`panvk_vX_shader.c:6579-6659`).
  - The runtime compares dynamic state (`SET_DYN_VALUE`).
- **Submit fast path:**
  - GPU-side semaphore waits (059) avoid CPU stalls for same-queue waits. M8 covers the rest.
  - A polling wait (timeout 0) returns after one invalidate and one read (`gpu_queue.c:817-873`).
  - When CS_ACTIVE is set, the doorbell is rung without an ioctl (`:738-768`).
  - The 64 KiB ring (`:58`) holds at least 128 entries per subqueue (`KBASE_RING_JOB_MAX_SIZE` is 512, `:62`). `reserve_ring` waits only when the ring is full (`:432-463`).
- **Secondaries and CmdExecuteCommands:** DXVK does not use them.
- **Queries:**
  - Occlusion begin/end dirty OQ, which re-emits the ZSD and DCD0. That is inherent.
  - pstats and VMR tracking return early when they are inactive.

## Overlaps for the other reviewers
- **GPU (C):**
  - Every ring entry runs FLUSH_CACHES with CLEAN_AND_INVALIDATE on L2/LSC and INVALIDATE on the other caches (`gpu_queue.c:629-635`).
  - `flush_id` is forced to 0 whenever any wait seqno is set, including a wait on the entry's own subqueue, which needs no wait at all (`:575-580`). That disables the flush-ID skip for every entry of a submit that waits on a semaphore.
  - Every CS ends with an L2/LSC CLEAN (`csf/panvk_vX_cmd_buffer.c:508-516`).
  - B5 adds a COMPUTE entry per primary.
  - **v10 only:**
    - When only ZSD-side state is dirty (stencil reference or compare mask, depth bias, depth clamp or clip), `prepare_ds` gets `earlyzs = {0}` (`csf/panvk_vX_cmd_draw.c:3136-3140`, upstream), because `prepare_dcd` leaves it untouched when no DCD bit is dirty.
    - `cfg.shader_read_only_z_s` (`:2482`) is then false while the DCD keeps the earlier earlyzs.
    - That is conservative, but the two descriptors disagree.
- **Correctness:** if the encoding in B2 holds, `timestamp_device_coherent = (coherency_mode != 0)` (`kbase_kmod.c:508`) is inverted. shaderDeviceClock would then be exposed on non-coherent systems (31) and hidden on ACE-Lite (0).
- **JM (D):**
  - The depth_bounds_fs substitution (092) and `clear_dirty_after_draw` are shared code, so v6/v7/v9 may pay A1 on every draw for their FS-dependent state (RSD, linking).
  - Check the `fs_user_dirty` consumers in `jm/panvk_vX_cmd_draw.c`.
  - jm-v7 097's per-set dirty tracking is under `#if PAN_ARCH < 9` (`panvk_vX_cmd_desc_state.c:411`). So v9 JM still takes the global DESC_STATE path, like CSF (A2).
- **Memory (B):**
  - A3 and B5 mean every primary command buffer holds at least 192 KiB of CS (3 × 64 KiB chunks) until the pool is reset or trimmed (231).
  - cmd_cs BOs outnumber cmd_desc BOs by about 2.4-3:1 in the 112 FO4 log.
