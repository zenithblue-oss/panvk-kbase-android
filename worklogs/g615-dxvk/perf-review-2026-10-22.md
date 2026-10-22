# PanVK perf and efficiency review, 2026-10-22

This is a read-only review: no device, no builds, no code edits.

- **Trees:**
  - `/home/flux/src/mesa-v7J`, branch v7K, HEAD d160802 (base-series plus jm-v7-wip 0006-0062).
  - `/home/flux/src/mesa-bc`, HEAD d2f5472 (csf-v11 with BC 197-202).
- **Line numbers:** for mesa-v7J unless marked `[bc]`.
- **Not repeated:** items in the earlier review (H1-H8, M1-M16 and its LOW list). Of those, H5, M8, M12, M13, M15 and M16 are deferred (see `builds/v7K.md`).
- **Impact:** a rough guess for draw-bound DXVK on G52 (A06), unless noted otherwise.
- **Tags:** **[now]** means the fix is safe without a device. **[dev]** means it needs a device A/B.
- **Upstream items:** some items are upstream PanVK JM design rather than series code. They are marked *(upstream)*. They are listed because the v6/v7 series now carries DXVK-class workloads through them.

## Overall top 15
1. Item 1: one kbase atom per dispatch.
2. Item 3: three extra jobs per indexed draw.
3. Item 2: every barrier splits the batch.
4. Item 4: DESC_STATE is dirty globally.
5. K1: Android present does DeviceWaitIdle plus a CPU copy.
6. Item 5 (correctness): the VS storage-image table is stale.
7. K2: the v6/v7 tiler heap commits too much memory.
8. Item 10: BC shadows are linear and wide on Bifrost.
9. Item 11: each BC upload adds a barrier.
10. Item 7: EndQuery splits the pass.
11. K3: the 160 MiB prerast arenas on G615.
12. Item 9: an alt-shader dirty flip on every rebind.
13. Item 6: push uniforms and attributes on every draw.
14. Item 12: one raw-store BO per upload.
15. Item 8: double FS compiles.

The rest follow: K4, item 13/K5, K6, item 14, item 15, K7, K8.

## Ranked items

### 1. Every vkCmdDispatch is its own batch, so one kbase atom per dispatch *(upstream)* [dev]
- **Where:** `jm/panvk_vX_cmd_dispatch.c:179-180` (close plus open before the dispatch) and `:362` (close after).
- **Issue:** each dispatch becomes its own job chain and atom. That means:
  - its own TLS/WLS allocation (`jm/panvk_vX_cmd_buffer.c:135-148`);
  - an atom-boundary cache flush;
  - kbase submit and event overhead.

  The following all hit this path: vk_meta compute copies and blits, the BC transcode and decode dispatches, the GS-less compute work of DXVK and vkd3d, and the internal precomp kernels recorded outside a render pass.
- **Impact:** High for compute-heavy work (vkd3d, texture streaming with BC transcodes, vk_meta copies). Each atom costs tens of µs of submit and scheduling on kbase JM, and back-to-back dispatches cannot overlap.
- **Fix:**
  - Keep a "compute batch" open (one with no FBD) across consecutive dispatches. Close it only on a render-pass start, an event or query op, or an explicit barrier (see item 2).
  - Order the dispatches inside the chain with `pan_jc_add_job(..., barrier = true, ...)` only when a barrier was recorded in between. Otherwise use no barrier, so independent dispatches overlap.
  - Size TLS/WLS as the batch's max, as draws already do.
- **Risk:** Medium. Ordering is implied today by the batch split, so every barrier-free dispatch sequence must be truly independent per spec. That holds, since Vulkan requires a barrier for any RAW between dispatches.
- **Measure:** PanProbe compute bench with 1000 small dispatches in one command buffer: wall ms per submit, plus a count of kbase `JOB_SUBMIT` atoms (strace -c or `PANVK_DEBUG=trace`). Re-run bc_timing upload ms.

### 2. vkCmdPipelineBarrier2 always closes the batch *(upstream; the FIXME is in the code)* [dev]
- **Where:** `jm/panvk_vX_cmd_buffer.c:438-452`.
- **Issue:** every barrier splits the batch, whatever its stages and access masks, and even when the batch holds only compute or transfer jobs. Inside a render pass (feedback loops, `dynamic_rendering_local_read`), a split costs a full tile store and reload. BC uploads add their own internal barrier per upload (`[bc] panvk_vX_cmd_meta.c:1497-1517`; see item 11), so every texture upload costs two batches on JM.
- **Impact:** High together with item 1. Medium alone. DXVK and vkd3d record one or more barriers per few dispatches or copies.
- **Fix:** when the current batch has no FBD (no tiler or fragment work), do not close it. Instead:
  - set a `pending_barrier` flag so that the next job added to `vtc_jc` gets `barrier = true`;
  - if the barrier's dst access includes SHADER_SAMPLED_READ, UNIFORM_READ or INDIRECT, insert a `MALI_JOB_TYPE_CACHE_FLUSH` job (clean+invalidate L2/LSC/texture) with the barrier bit.

  Keep the batch split for barriers with image layout transitions, for barriers inside a render pass, and for anything involving the fragment chain.
- **Risk:** Medium. The JM job manager's per-job barrier and cache-flush semantics on Bifrost must be checked on device; the CTS memory-model and synchronization tests cover this.
- **Measure:** PanProbe barrier bench (dispatch, barrier, dispatch, repeated 1000×): ms per submit and atoms per submit. Run synchronization2 and memory_model tests for regressions.

### 3. Every direct vkCmdDrawIndexed costs 3 extra GPU jobs, serially chained *(upstream)* [now: kernel; dev: fold]
- **Where:**
  - `jm/panvk_vX_cmd_draw.c:3769-3812`: DrawIndexed becomes an indirect draw through a desc-pool command.
  - `jm/panvk_vX_cmd_draw.c:3166-3216`: a WRITE_VALUE job, then the `index_minmax_search` compute job, then the indexed-indirect helper job (`:3315-3343`), and only then the vertex and tiler jobs.
- **Kernel issue:** `libpan/draw_helper.cl:1511-1563`. Each thread reads 1024 *consecutive* indices serially. The loads are uncoalesced (stride 1024×idx_size between lanes), and a 300-index draw is a single thread doing 300 dependent loads. That thread then does two global atomics per active thread.
- **Not the same as the earlier review:** its LOW item covered only the grid size. This item is about the job count and the kernel shape.
- **Impact:** High for DXVK, where almost every draw is indexed. That is 3 more jobs per draw in the vertex/tiler chain, each with a dependency, so the front end serializes on them. At about 2-5 µs per JM job and around 2000 draws per frame, the GPU-side cost is several ms per frame.
- **Fix:**
  1. **[now]** Rewrite the min/max kernel to be strided: lane `t` reads `t, t+N, ...`. Do a subgroup or workgroup reduction, then one atomic per workgroup.
  2. **[now]** Drop the WRITE_VALUE job. The helper can initialise min/max itself when `cpu_count` is known and the grid is 1 workgroup. Alternatively, init the result on the CPU in the desc pool, which is CPU-written anyway, as `bitmap.cpu` already is at `:3247`.
  3. **[dev]** For `cpu_count <= 4096`, fold the min/max into the indexed-indirect helper itself: one 64- or 256-thread workgroup with a local reduction, then patch the descriptors. One job instead of three.
- **Risk:**
  - 1 and 2 are low.
  - 3 is medium: the helper must stay a single workgroup, and the per-draw index range limit must hold.
- **Measure:** PanProbe draw-call bench with 2000 indexed draws of 300 indices: GPU ms per frame (vertex-chain time from timestamps) and job count per batch, before and after.

### 4. DESC_STATE dirty is global: any descriptor bind rebuilds VS and FS tables and copy jobs *(upstream TODO)* [now]
- **Where:**
  - `panvk_vX_cmd_desc_state.c:328-347` sets `gfx_state_set_dirty(DESC_STATE)` for any set; the code says "TODO: Invalidate only if the shader tables are disturbed".
  - `jm/panvk_vX_cmd_draw.c:2058-2111` then rebuilds both stages' tables, dyn SSBOs and push sets, and emits *two* COMPUTE copy-desc jobs (`:3036-3049`).
- **Issue:** DXVK binds per-stage sets and rebinds them often. Rebinding only the FS set still re-runs the VS copy job, and rebinding an identical set (same handle, same dynamic offsets) costs the same.
- **Impact:** High on CPU and Medium on GPU. That is up to 2 extra compute jobs per draw and about 6-10 desc-pool allocations.
- **Fix:**
  - Keep a `dirty_sets` bitmask in `panvk_descriptor_state`.
  - In `cmd_desc_state_bind_sets`, skip a set whose handle and dynamic offsets are unchanged.
  - Rebuild the VS side only when `dirty_sets & vs.used_set_mask`, and the FS side only when `dirty_sets & fs.used_set_mask`.
  - Dynamic-offset-only changes need only `cmd_prepare_dyn_ssbos` and `panvk_cmd_fill_dyn_ubos`, not the copy job, unless dyn UBOs live in the copied table.
- **Risk:** Low to medium. Push sets and the vertex attribute/image table coupling (see item 5) must stay correct.
- **Measure:** PanProbe draw bench that rebinds one FS set per draw: CPU µs per draw (simpleperf) and copy-desc jobs per frame.

### 5. Correctness: VS storage images read stale descriptors on the second draw *(reachable since 0032/jm-v7 045)* [now]
- **Where:**
  - `jm/panvk_vX_cmd_draw.c:1032-1126`: `panvk_draw_prepare_vs_attribs` allocates a *new* attribute table every draw. With `num_imgs`, the image attributes sit at `MAX_VS_ATTRIBS` in it and `vs.desc.img_attrib_table` is repointed (`:1118-1123`).
  - The VS copy-desc job that fills the image entries runs only on `DESC_STATE || VS` dirty (`:2087-2091`).
- **Effect:** a second draw with the same descriptors gets an image attribute table that was never written, so it reads uninitialised desc-pool memory. The vertex stage can fault or read garbage. 0032 exposed vertexPipelineStoresAndAtomics on v6/v7, which makes this reachable. Bindless images (0052) are not affected.
- **Fix (pick one):**
  - When `num_imgs`, emit the VS copy-desc job on every draw.
  - Or, better, together with item 6: reuse the attribute table when the VB/VI state and the draw's instancing class are unchanged.
- **Risk:** low.
- **Measure:** add a PanProbe vertex_stores case with two consecutive draws using a VS imageStore and no rebind in between. It should fail today.

### 6. Attribute descriptors and push uniforms are re-emitted on every draw and layer *(upstream)* [now]
- **Where:**
  - Attributes: `jm/panvk_vX_cmd_draw.c:1128-1141, 2080`. The comment at `:1026-1030` says "always re-emit".
  - Push uniforms: `:2161-2176`. `cmd_prepare_gfx_push_uniforms` runs unconditionally, even though `cmd_prepare_draw_sysvals` already computes `VS_PUSH_UNIFORMS` and `FS_PUSH_UNIFORMS` precisely (`panvk_vX_cmd_draw.c:1093-1133`).
  - Varying buffer descriptors (`:633`) are rebuilt as well.
- **Impact:** Medium on CPU, roughly 1-2 KB of desc-pool writes per draw. Each allocation is cleaned again at EndCommandBuffer, and on every resubmit when the pool is flushed (see the kbase items).
- **Fix:**
  - Gate the VS and FS push-uniform uploads on their dirty bits. Keep the previous GPU pointer otherwise, as CSF does.
  - Cache the attribute and attribute-buffer tables while VB/VI are not dirty, `instance.count <= 1`, `instance.base == 0` and `padded_vertex_count` is unchanged. In that case the descriptors do not depend on the draw.
  - Indexed draws patch these tables on the GPU, so those keep per-draw tables.
- **Risk:** low. The helper's in-place patching must still get a private copy.
- **Measure:** the CPU µs-per-draw bench, plus desc-pool bytes per frame (`PANVK_DEBUG=` pool stats or a counter).

### 7. Every EndQuery inside a render pass splits the batch *(upstream)* [dev]
- **Where:** `jm/panvk_vX_cmd_query.c:376-380`, which calls `cmd_close_batch` whenever a batch is open.
- **Issue:** occlusion queries per object (DXVK predication and occlusion culling) give one fragment pass, with a tile store and reload, per query.
- **Not the same as earlier items:** M9 and 0061 only kept PANVK_OVERLAP, and M10 was about timestamps.
- **Impact:** High for occlusion-query-heavy games, where a frame turns into N render passes. Low elsewhere.
- **Fix:** queue the ended queries on the batch (`batch->query_ends`). Emit the availability and report-copy jobs once, after the batch's fragment job, when the batch closes naturally. The OQ counter is already final then.
- **Risk:** Medium, because of query ordering against CopyQueryPoolResults within the same pass. Copies already close the batch.
- **Measure:** PanProbe occlusion_query bench with 100 queries in one pass: fragment jobs per frame and ms per frame.

### 8. Most fragment shaders compile twice for the 0047 depth-bounds alternate [now]
- **Where:** `panvk_vX_shader.c:5429` (`panvk_v7_wants_alt`) and `:5548-5586`.
- **Issue:** DXVK enables depthBounds and makes it dynamic, so nearly every FS gets a second full compile: NIR lowering, the backend and RA. Both are serialized into the cache (`:5986`). An xfb VS compiles three times (main, passthrough GS, 0048 alt).
- **Impact:** Medium-High for pipeline-compile stutter and cache size (2×). No per-draw cost.
- **Fix:** compile the lowered variant lazily, on the first draw with depth bounds on, protected by a cmpxchg like `passthrough_gs`. Alternatively, share the front end: clone the NIR right before `panvk_compile_nir` and run `panvk_lower_depth_bounds` late.
- **Risk:** low for the late clone. The lazy option compiles on the draw path in the rare depth-bounds case.
- **Measure:** PanProbe pipeline-compile bench (ms per vkCreateGraphicsPipelines) and the pipeline-cache blob size.

### 9. Rebinding the same pipeline flips FS/VS dirty twice per draw (0047/0048) [now]
- **Where:** `panvk_vX_shader.c:6501-6514` (bind compares against the stored shader, which is the alt) and `jm/panvk_vX_cmd_draw.c:2944-2981`.
- **Issue:** after a draw the state holds the alt compile. A rebind of the main shader therefore looks like a change, and the next draw flips back. Each flip:
  - sets FS dirty, causing a relink (`panvk_per_arch(link_shaders)` allocates from the desc pool), the FS tables, the FS copy job and an RSD rebuild;
  - and the same on the VS side.
- **Impact:** Medium on CPU and GPU, since DXVK rebinds pipelines very often. It compounds item 4.
- **Fix:** in bind, compare against `cur->alt_parent ? cur->alt_parent : cur` and skip if equal. Alternatively keep the main shader in the state and the effective one in a separate field.
- **Risk:** low.
- **Measure:** the CPU µs-per-draw bench with a same-pipeline rebind every draw, and RSD allocations per frame.

### 10. BC compute shadows on v6/v7/v9 are LINEAR and use wide formats [dev]
- **Where:**
  - `[bc] panvk_image.c:350` forces `DRM_FORMAT_MOD_LINEAR` for `bc_emul`.
  - `:632-645`: the decoded plane inherits it.
  - `panvk_image.h:147-174`: BC4/5 decode to RG16, BC6H to RGBA16F.
  - The compact formats and tiled shadows are gated on arch >= 10 (`panvk_image.c:902-905`; mesa-v7J `:938-939`).
- **Issue:** every BC sample on Bifrost reads a linear 32-64 bpp shadow. BC4 ends up at 8× its native bits per pixel.
- **Impact:** High for texture-bound DXVK on v6/v7, through texture cache misses. Memory is about 2× the compact formats.
- **Fix:**
  - Give the compute shadow the U_INTERLEAVED modifier. The decoder then writes U-interleaved addresses, reusing the ETC2 transcoder's addressing.
  - Use `panvk_bc_compact_format(fmt, false)`: R8/RG8 for BC4/5, B10G11R11 for BC6H UF16.
- **Risk:** Medium. The address swizzle must match `pan_image_layout` exactly.
- **Measure:** PanProbe bc_perf on G52 (minified full-screen BC1/BC5/BC6H sampling): frame ms and image size.

### 11. Each BC upload adds an internal ALL_COMMANDS barrier, which is a batch split on JM [now]
- **Where:** `[bc] panvk_vX_cmd_meta.c:1497-1517` (mesa-v7J `:1470-1483`). The sequence is: raw copy, then `bc_decode_begin` (an ALL_COMMANDS barrier), then the decode from the raw plane. `cmd_copy_image` does the same (`:1943-1957`).
- **Issue:** N texture uploads cost 2N batches and atoms on JM. On CSF, each upload is a wait on all subqueues.
- **Fix:**
  - Decode straight from the source buffer: pass a `bc_decode_src` built from `addressRange.address`, `addressRowLength` and `addressImageHeight`. The bc_gfx path already does this (comment at `:1458`). Then drop the barrier.
  - For image-to-image copies, decode from `bc_raw_plane_src(src)`.
- **Impact:** High on JM during load and streaming, Medium on CSF.
- **Risk:** low.
- **Measure:** atoms per submit on the PanProbe texture-upload test, and decode µs with `PANVK_DEBUG=bc_timing`.

### 12. BC raw store: one new WB-mapped BO per uploaded image, all of them resident until reset [dev]
- **Not M15:** M15 was about the spill directory and skipping the pread.
- **Where:** `[bc] panvk_image.c:2055-2123` (`bc_raw_materialize` and `panvk_bc_raw_acquire`) and `:2126-2166` (release).
- **Issue:** every DXVK texture has TRANSFER_SRC, so every upload goes through the raw store. Each upload costs:
  - a `panvk_priv_bo_create(WB_MMAP)`, which is create, GPU map and mmap;
  - a GPU copy;
  - at reset, a full flush and invalidate, a pwrite, `sync_file_range(WAIT_AFTER)`, then unref and munmap.

  At the load-time peak, all of that raw memory is resident at once.
- **Impact:** Medium-High for memory (the lowmem goal is lost at peak) and Medium for CPU and syscalls.
- **Fix:**
  - For whole-image uploads from host-visible buffers, defer the raw write to submit. pwrite straight from the buffer's host mapping into the spill file, with no raw BO and no GPU copy.
  - At minimum, suballocate the raw BOs from slabs.
- **Risk:** Medium. It needs a buffer-address-to-mapping lookup and correct ordering.
- **Measure:** log the high-water mark of `dev->bc_raw.resident_B` (bc_timing) and `dumpsys meminfo` PSS during the texture-load run.

### 13. Per-batch TLS allocation, sized for the whole GPU, on every batch [now]
- **Where:** `jm/panvk_vX_cmd_buffer.c:130-148`. Every batch with `tls.size` allocates `next_pow2(tls) × thread_tls_alloc × core_id_range` from `tls_pool`.
- **Issue:** with GS/tess/BC kernels that spill, every batch (and every dispatch, see item 1) allocates its own full stack. Example: 512 B TLS on G52 MC2 is about 1 MB per batch, so 50 batches per command buffer is about 50 MB of pool. The pool keeps all of it until reset.
- **Impact:** Medium for memory footprint. Fewer and larger BOs also mean fewer page faults.
- **Fix:** keep two TLS buffers per command buffer (ping-pong, because PANVK_OVERLAP lets batch N+1's vertex work run beside batch N's fragment work), sized to the max seen, and reuse them across batches. Grow by reallocating only when a larger size is needed.
- **Risk:** Medium. Correctness depends on JM chain ordering between batches that are not adjacent; with ping-pong, batch N+2 runs after N has finished.
- **Measure:** desc/tls pool bytes per frame (a pool counter), and `/proc/<pid>/smaps` GPU mappings during a DXVK frame loop.

### 14. Bindless texel-buffer and image accesses: a software divide and non-hoistable descriptor loads (0052/0059) [now]
- **Where:**
  - `panvk_vX_nir_lower_descriptors.c:858`: `nir_udiv(avail, stride)` is emitted on every bindless texel-buffer access. Bifrost has no integer divide, so it is about 20-40 instructions.
  - `:597-606`: the 0059 reorder flags apply only to constant indices. `load_bindless_desc` (`:826`) and `build_bindless_img_texel` (`:885`) reload the descriptor on each access.
- **Impact:** Medium-High for vkd3d (mutable/UAB heaps make everything bindless). Low for DXVK D3D11.
- **Fix:**
  - Store the element count in the buffer-view descriptor's pad word at creation (`panvk_vX_buffer_view.c:58-60`) and use `ult(x, width)`. Fallback: a 64-bit `x*stride+off < size` check with no divide.
  - Clamp dynamic indices with `umin(idx, array_size-1)`, which makes the loads `NON_WRITEABLE | CAN_REORDER`.
- **Risk:** low to medium. The pad-word layout is from the commit text and must be checked; a null descriptor must get width 0.
- **Measure:** shaderdb instruction and clause counts, and ms per dispatch on the PanProbe vkd3d_heap/texel-buffer bench.

### 15. v7 compile knobs are missing from the shader cache key [now]
- **Where:** `panvk_vX_shader.c:2527`. `panvk_hash_state` does not hash these knobs:
  - `PANVK_TEX_WATERFALL` (`:3338`)
  - `PANVK_V7_IDVS` (`:4612`)
  - `PANVK_V7_DESC_REORDER` (`panvk_vX_nir_lower_descriptors.c:532`)
- **Impact:** Medium for the *validity* of every PanProbe A/B. Toggling a knob silently reuses the cached binary from the previous setting.
- **Fix:** hash the resolved values under `PAN_ARCH < 9`.
- **Risk:** none.
- **Measure:** knob=0 then knob=1 without `MESA_SHADER_CACHE_DISABLE`. The shaderdb counts must differ.

## Submit, BO and memory items (K1-K8)

### K1. Android present waits on the whole device and copies every frame on the CPU [dev]
- **Where:** `panvk_wsi.c:273`, `275-294` (`panvk_android_swapchain_queue_present`).
- **Issue:** every present calls `DeviceWaitIdle`. It then copies the whole AHB into the window row by row: `ANativeWindow_lock`, then `AHardwareBuffer_lock(CPU_READ_OFTEN)`, then a memcpy, then `unlockAndPost`.
  - `acquire_next_image` (`:223-233`) is round-robin with no fence.
  - On X11 without `PANVK_KBASE_DRI3` or `WSI_X11_TERMUX`, kbase uses `sw_device` (`:534`), so CPU/SHM presents are a silent default.
- **Impact:** High for frame pacing. CPU and GPU never overlap, and a 1080p frame adds about 8 MB of CPU reads from GPU memory, a few ms per frame.
- **Fix:**
  - Queue the AHB images with `ANativeWindow_queueBuffer`/`dequeueBuffer` and a fence fd.
  - At minimum, wait only on that image's last-submit sync instead of the whole device.
- **Risk:** Medium (WSI work). JM has no sync_file export (see the v6 section).
- **Measure:** PanProbe swapchain test: CPU time of `vkQueuePresentKHR` per frame, and FPS against GPU timestamp time.

### K2. Both PANVK_OVERLAP tiler-heap halves live in one GROW_ON_GPF 128 MiB BO on v6/v7 [dev]
- **Where:** `jm/panvk_vX_cmd_buffer.c:351-364` and `panvk_vX_device.c:852-855`.
- **Issue:** kbase grows a GROW_ON_GPF region contiguously, committing every page up to the fault, as the v9 comment at `:336-340` notes.
  - Half 1 starts at +64 MiB, so its first fault commits at least 64 MiB. That memory stays committed for the life of the device.
  - `panvk_select_tiler_hierarchy_mask` (`:369-370`) is still passed the full 128 MiB, not the half.
- **Impact:** High for memory on v6/v7, where low-RAM G71/G72/G52 phones are the targets.
- **Fix:** use two separate heap BOs, as v9 does with `tiler_heap2`. Size each at 32-64 MiB and pass the half size to the hierarchy mask.
- **Risk:** low to medium. Heap-OOM behaviour changes, and whether the Bifrost tiler takes chunks from Top is unverified.
- **Measure:** `KBASE_IOCTL_MEM_QUERY` COMMIT_SIZE of the heap after a render loop, with `PANVK_OVERLAP=0` and `=1`, or the `dumpsys meminfo` GL mtrack figure.

### K3. The gpu_prerast arenas commit 160 MiB per VkDevice and memset it on the CPU (v9+, including G615) [dev]
- **Where:** `panvk_vX_device.c:791-819` and `145-157`; the memset is at `kbase_kmod.c:2400-2412`.
- **Issue:** the 64 + 96 MiB arenas are fully committed WB BOs whenever geometryShader, transformFeedback or vertexPipelineStoresAndAtomics is enabled, and DXVK always enables them.
- **Impact:** High for memory and Medium for vkCreateDevice time on G615.
- **Fix:** commit explicitly with `KBASE_IOCTL_MEM_COMMIT` on the first gpu_prerast draw, or start small and grow. Gate the tess arena on the creation of a real tess pipeline. Do not rely on alloc-on-fault, which lost output on MediaTek.
- **Risk:** Medium.
- **Measure:** vkCreateDevice ms and PSS for a device with DXVK's features enabled.

### K4. `kbase_kmod_bo_alloc` memsets every committed page on the CPU, plus a MEM_SYNC [dev]
- **Where:** `kbase_kmod.c:2400-2412`.
- **Issue:** this runs for every vkAllocateMemory and every pool slab. kbase already hands out zeroed pages, so the memset most likely duplicates work, and it stutters DXVK's 32-256 MB chunk allocations.
- **Impact:** Medium.
- **Fix:** drop the memset and MEM_SYNC, or keep them only for WB CPU-visible BOs behind a debug option.
- **Risk:** low to medium. First check on device that fresh pages read back as zero.
- **Measure:** ms per vkAllocateMemory of 64 and 256 MB, and a zero-readback check.

### K5. TLS/WLS allocated per batch [now]
This is the same finding as item 13. The kbase-slice review reached it independently and adds:
- TLS BOs are NO_MMAP and not growable, so the memory is fully committed.
- The memory then stays in `tls_big_bo_pool`.

### K6. Command-pool BOs are never released [now]
- **Where:** `panvk_cmd_pool.c:86-92`.
- **Issue:**
  - With RELEASE_RESOURCES, `tls_big_bo_pool` is cleaned *before* `vk_common_ResetCommandPool`. That call then splices the command buffers' BOs back into the free lists, so nothing is released.
  - There is no `TrimCommandPool`, so the desc/varying/gs/tls free lists only grow, and gs slabs stay at their GPU-faulted committed size.
- **Impact:** Medium for memory, in long sessions and after loading screens.
- **Fix:** clean all bo_pools *after* `vk_common_ResetCommandPool`, and implement TrimCommandPool to free the free lists.
- **Risk:** low.
- **Measure:** PSS after vkResetCommandPool(RELEASE_RESOURCES) or vkTrimCommandPool.

### K7. Dead per-batch BO-list gathering with an O(n²) dedup [now]
- **Where:** `jm/panvk_vX_gpu_queue.c:463-506`.
- **Issue:** every batch builds a VLA of all the command buffer's pool BOs and dedups it in O(n²). kbase never uses the list: `kbase_atom_alloc_try` copies 8 entries into `s->bos` (`kbase_kmod.c:2792-2794`), and nothing reads `s->bos`.
- **Impact:** Low to Medium on CPU per submit, growing with command buffer size.
- **Fix:** pass NULL/0 on JM and delete the gathering.
- **Risk:** low.
- **Measure:** simpleperf on vkQueueSubmit for a command buffer with 30 batches.

### K8. One JOB_SUBMIT per atom, and poll plus read per event [dev]
- **Where:** `kbase_kmod.c:3052-3057` (`nr_atoms = 1`) and `2846-2866`.
- **Issue:** each batch costs 2 submit ioctls, and each completion costs 2 syscalls.
- **Impact:** Low to Medium alone. It multiplies with items 1, 2 and 7.
- **Fix:**
  - Submit all atoms of a vkQueueSubmit in one array, so a frag pre_dep on a vtc atom in the same call works.
  - Read up to 16 events per `read()`.
- **Risk:** low to medium, from atom-slot bookkeeping on partial failure.
- **Measure:** `strace -c` syscalls per frame.

### Smaller submit notes
- **Logging:** `mesa_logi` fires on every growable v6/v7 BO (`kbase_kmod.c:2292`), which includes each gs_pool slab, and on every dma-buf import (`:2179`). Downgrade both to debug.
- **Flushing:** `panvk_pool_flush_maps` (`panvk_mempool.c:277-288`) flushes whole 64 KB slabs, including the unused tail of the transient slab. Flush only up to `transient_offset`.
- **v9 only:** `kbase_kmod_jm_clean_imports` cleans every non-coherent dma-buf in full on every submit.

## Lower-priority notes
- **AFBC (0062/jm-v7 089):** AFBC comes back only for R/B-swapped images with no SAMPLED or INPUT_ATTACHMENT usage (`panvk_image.c:219-224`). DXVK render targets always carry SAMPLED, so M11 is still largely open for DXVK. Options: a drirc to hide `customBorderColorWithoutFormat` for DXVK on v7, or reswizzle the border colour in the sampler. [dev]
- **RIA2 on by default (0053):** robustImageAccess2 is now the default on v6/v7, so DXVK/vkd3d pay a branch plus a txs on every storage-image store or atomic (`panvk_vX_shader.c:2973`). Redirecting OOB stores to x = width, as loads already do, would remove the branch if the hardware drops attribute stores past the width. [dev]
- **Waterfall analysis (0058/0060):** `nir_divergence_analysis` runs on every shader (`panvk_vX_shader.c:3167`), and getenv is called per compile (`:3336`). Return early when no tex has a non_uniform flag. [now]
- **GS offs table:** the GS slot-offset table is memcpy'd from the link on every GS draw (`jm/panvk_vX_cmd_draw.c:775-781`, 1 KB). Store it once per link in `link->` priv memory. [now]
- **Bindless sysval loop (0052):** the per-draw loop over the bindless sysvals and set addresses (`panvk_vX_cmd_draw.c:1067-1092`) runs even when DESC_STATE is clean. `set_gfx_sysval` compares, so the cost is small. Gate it on DESC_STATE or VS/FS dirty. [now]
- **BC follow-ups:**
  - The exact fragment decode does one render pass plus one image view per region/mip (`[bc] panvk_vX_cmd_meta.c:706-773`). Cache per-level views, and use compute for non-AFBC shadows.
  - The ZERO_INITIALIZED decode allocates a zero row per level and ends with an ALL_COMMANDS barrier (`:1077-1087`, `:1425-1428`).
  - CmdCopyImage2 into a transcoded destination goes through the destination's raw blocks (`:1880-1941`).
  - Each BC image's layout is built twice (`panvk_image.c:611-631`).
- **Dropped v6/v7 checks:** 0035 (048), 0045 (074), 0054 (081), 0057 (084) and 0061 (088) were checked. Their cost is compile-time only or confined to the feature, with no per-draw cost.

## v6 (Mali-G71/G72) readiness

**Build: arch 6 is compiled in.**
- `scripts/build-android.sh:70` builds only the Vulkan driver (`-Dvulkan-drivers=panfrost -Dgallium-drivers=`, kmd kbase).
- `src/panfrost/vulkan/meson.build:76,90,147` has `bifrost_archs = [6, 7]` and `jm_archs = [6, 7, 9]`, and the per-arch loop includes 6. So `panvk_v6` is built and linked.
- `libpan/meson.build:15` builds the libpan kernels for 6.
- There is no meson option for the panvk arch list. The gallium panfrost archs do not matter, because gallium is disabled.

**Runtime gate:** `panvk_physical_device.c:1241-1253` rejects arch 6 and 7 with INCOMPATIBLE_DRIVER unless `PAN_I_WANT_A_BROKEN_VULKAN_DRIVER` is set.
- No script in `scripts/` sets it.
- It appears only in the prebuilt `.so` files under `apps/*/build/...`, so the APKs presumably set it, as the A06 v7 runs work.
- Every new consumer (launcher, Winlator-style hosts) must set it too. Otherwise, add `case 6: case 7:` to the accepted list, behind a drirc or build option.

**v6-specific gating in jm-v7-wip:**
- All new paths are `PAN_ARCH < 9` or `== 6 || == 7` (README "v6 parity"), so v6 takes the v7 code.
- v6-specific branches exist only for:
  - the null-image swizzle (`panvk_vX_device.c:551`);
  - the upstream FPK rule (`jm/panvk_vX_cmd_draw.c:475`);
  - the blend constants (`panvk_vX_blend.c:259`);
  - the frame shaders (`panvk_vX_cmd_frame_shaders.c:502`);
  - the AFBC direct resolve (`panvk_vX_cmd_draw.c:121`);
  - the v6 varying swizzle in `panvk_vX_shader.c:6195-6237`.
- No v7-only gate blocks v6.

**Concerns:**
- **IDVS on G71 (0046/jm-v7 075):** the IDVS quirk list (`panvk_vX_shader.c:4614-4616`) contains G72 (0x6201) and G76 (0x7201), but not G71 (0x6000, mask 0xff0f gives 0x6000). G71 is the same generation as G72, where INDEXED_VERTEX was measured to complete without writing positions. G71 is therefore the one v6 part that defaults to IDVS. **Recommendation:** treat all of `arch == 6` as quirked (IDVS off) until measured, keeping `PANVK_V7_IDVS=1` as the opt-in.
- **Atom size:** G71/G72 kernels are often pre-11.22 kbase. 0018 adds the 48-byte atom as the last stride candidate; it is untested on hardware.
- **Compiler quirks:** G71's `BIFROST_NO_FP32_TRANSCENDENTALS` is handled by gpu_id, and the libpan v6 kernels are built with the G71 quirk set. The BC compute decoders and transcoders are SPIR-V compiled at runtime with the real gpu_id, so they are fine too.
- **Arch-10 gate on lowmem BC:** `panvk_bc_lowmem_enabled` (`panvk_physical_device.c:1878`) and `bc_gfx` (`panvk_image.c:918`) are arch >= 10. v6/v7 always get the linear full-size compute shadows (item 10), which costs the most memory on exactly the low-RAM G71/G72 phones. Bifrost samples ETC2/EAC natively, so lifting the gate for no-raw images (the raw store is `#if PAN_ARCH >= 10`) would give tiled 4-8 bpp shadows. The catch is H8's encoder cost.
- **JM deadline clock:** `jm/panvk_vX_gpu_queue.c:357-361` still uses CLOCK_MONOTONIC_RAW for `PAN_ARCH != 9`, which covers v6 and v7. The comment says this turned timeouts into DEVICE_LOST on v9, but it was fixed only there. Use CLOCK_MONOTONIC for every JM arch. [now]
- **Dma-buf import clean:** the non-coherent dma-buf import clean is gated to arch 9 (`kbase_kmod.c:2167`, `gpu_queue.c:401-405`). On v6/v7, dma-heap host-visible or shareable memory may keep dirty CPU lines, which is the "black dashes" bug. Extend the gate to arch < 9 when `!cpu_gpu_coherent`. [dev]
- **No sync_file export below arch 10:** `panvk_physical_device.c:1153-1154`. Combined with K1, Android present on v6/v7 is fully synchronous.
- **Tiler heap:** K2 (both tiler-heap halves in one GROW_ON_GPF BO) is v6/v7-specific and hurts most on G71/G72-class devices with little RAM.
- **Coverage:** no v6 device has run anything. All v6 evidence is drm-shim host recording on `PAN_GPU_ID` 6221.

## Suggested order
1. **[now] correctness and cheap wins:** item 5 (VS image stale table), item 15 (cache key), item 9 (alt rebind), item 4 (per-set dirty), item 6 (push uniforms and attribute tables), item 3 parts 1 and 2 (min/max kernel, drop WRITE_VALUE), item 11 (BC upload barrier), and the G71 IDVS quirk.
2. **[dev] structural JM work:** item 1 (compute batch), item 2 (barrier as CACHE_FLUSH job), item 7 (deferred EndQuery), item 3 part 3 (fold min/max into the helper), item 13 (TLS reuse).
3. **[dev] BC on Bifrost:** item 10 (tiled compact shadows), item 12 (raw store at submit), then lowmem on v6/v7.
4. **Compile time:** item 8 (lazy depth-bounds alt), item 14 (bindless loads).
