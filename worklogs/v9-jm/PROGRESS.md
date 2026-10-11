# Mali v9 (Valhall JM) progress

Target: Mali-G57 MC2 (Lenovo TB336FU, MT6835, gpu_id 0x90930010, kbase JM uAPI 11.38).
Backend: `patches/jm-v9/` on top of the g615-v11-csf series (universal ICD, v9 + v10-v14).
Reference: Noysz/panvk-g99-jm (local clone `/var/tmp/panvk/refs/noysz`, HEAD 6ce0d46e419a, MIT new files), rebased on a Mesa about two weeks newer than our pin 5a07217f, so every Noysz patch is ported by hand onto our FristOneRR-based 001/002 code, never swapped in.

Starting point (PanProbe on the G57, beta.17 candidate): 2/17. Failures: 9x vkCreateDevice -7/-8 (features not exposed), `gpu_prerast_slice` DEVICE_LOST on the "replay" case, `bc_decode` blit, `vmr_secondary`.

## Phase 1: Vulkan 1.3, resubmit fix, async submit, multi-draw, robustness2

Status: patches written, full series applies, Android universal build OK. **Not tested on the device:** no G57 was attached on 2026-10-11 (`adb devices` listed only a CPH2691 and the Poco X6 Pro G615). Per the plan, no other device was used for v9.

Build:
- Tree: `/var/tmp/panvk/v9/mesa-p1` (Mesa 5a07217f + the 152-patch series; one commit per changed jm-v9 patch).
- `.so`: `/var/tmp/panvk/dist-v9-p1/libvulkan_panfrost.so`, unstripped, 32.5 MB.
- BuildID `8f1586aa77848baf176b128f708ea99ee860a73b`, sha256 `baf3c41e2e7c505f3b3044da798a85ff586c7a4a8c30182c3121049824499b14`.
- Env: `HOST_TOOLS=tmp/rel9/src/build/host-tools/bin`, `LD_LIBRARY_PATH=/var/tmp/panvk/llvm22/usr/lib`, `BDIR=/var/tmp/panvk/v9/build-p1`.
- No new warnings in the touched files.

### Patches

| Patch | What | Noysz source |
|---|---|---|
| 003 (replaced) | v9 multiview and layered rendering; the 1.1 override is dropped, so v9 reports the upstream 1.3 | 0042, 0048, `writes_layer` from 0063 |
| 007 | Reset the MALLOC_VERTEX vertex array when a command buffer is resubmitted | 0072 |
| 008 | Apply `vkCmdSetEvent` when the GPU work before it completes | 0068 (event part) |
| 009 | multiDrawIndirect, drawIndirectCount, gl_DrawID, firstInstance for instance-rate attributes in indirect draws | 0044, 0039 |
| 010 | robustBufferAccess2 and nullDescriptor on v9 | 0045, 0071 |

#### 003: Vulkan 1.3 on v9

The 1.2/1.3 core feature list is shared with v10+ (`panvk_vX_physical_device.c`): descriptors, the compiler, sync2, dynamic rendering, inline uniform blocks, BDA, maintenance4, host query reset, and timeline semaphores (the kbase software timeline, same as CSF kbase). The JM-specific parts are the command and queue code.

Audit method: compare the `panvk_v9_*` and `panvk_v10_*` entry points in the built `.so` (`llvm-nm`).
- Before phase 1, v9 lacked `CmdDrawIndirectCount` and `CmdDrawIndexedIndirectCount` (1.2 core commands). 009 adds both.
- What v10 still has that v9 lacks: `CmdExecuteCommands` and extension-only entry points (XFB, conditional rendering, multi-draw EXT, copy-memory-indirect). None of those extensions is exposed on v9. `CmdExecuteCommands` comes from `vk_cmd_enqueue` on JM.
- `CmdWriteTimestamp2` is a stub. That is valid: `timestampValidBits` is 0.

Gap found: multiview, which is core since 1.1 and was already advertised. The v9 draw path drew into layer 0 only (tiler context 0). A VS that read ViewIndex hit an unlowered `load_view_index`: the v10 path lowers it with `nir_lower_multiview`, but that lowering is gated to v10-13. Layered vk_meta copies, which write gl_Layer, had the same layer-0 limit.

The fix uses the Bifrost scheme for v9:
- Each draw is recorded once per view, or once per layer when the VS writes gl_Layer, with `layer_id` as a sysval.
- `lower_layer_writes()` now runs on v9.
- Each layer gets its own tiler context with `layer_offset = layer`, so the FBD picks internal list 0.
- The per-layer DCD offset is Bifrost-only.
- `writes_layer` is serialized with the shader. A VS that does not write gl_Layer goes to layer 0 only.

Not required for 1.3, still off on v9: geometry/tessellation shaders, clip/cull distance, multiViewport, fillModeNonSolid, depthBounds, pipelineStatisticsQuery, transform feedback (phases 2-3).

#### 007: resubmit fix (gpu_prerast_slice replay)

Found by reading the code; not yet confirmed on the device.
- PanProbe `gpu_prerast_slice` "replay" (`tests/dxvk/vulkan/gpu_prerast_slice.c:563-588`) submits a command buffer, waits (passes), then submits the same command buffer again. The second `vkWaitForFences` returned DEVICE_LOST.
- On reissue, `panvk_queue_submit_batch()` cleared the job headers and the tiler heap/context, but a v9 MALLOC_VERTEX job writes its allocated vertex packet pointer and strides back into its own Draw "Vertex array" (`genxml/v9.xml`: "Written by hardware in MallocVertexShader job mode").
- On the second run the job starts from that stale pointer and faults, so the atom fails and the fence wait reports DEVICE_LOST.
- The batch now keeps its MALLOC_VERTEX jobs in a list, and the reissue path packs back the CPU value (Packet = 1, pointer and strides 0).
- It is a list rather than a job type check because the indirect patch kernel can turn a MALLOC_VERTEX job into a NULL job for one run, and NULL jobs also come from other paths.

#### 008: async submit (0068)

Most of 0068 already exists in our 001/002 layer:
- kbase atoms with pre_dep chains (vertex/tiler to fragment DATA, ORDER on the previous atom);
- no CPU wait per job chain;
- fences and semaphores resolved lazily from atom numbers.

Noysz's base was fully synchronous, so 0068 was a rewrite there.

Ported: `vkCmdSetEvent` now takes effect at GPU completion, not at submit. The event records the atom, and `vkGetEventStatus` polls it with a zero timeout.

Not ported:
- One `JOB_SUBMIT` per `vkQueueSubmit`. This is a performance change and needs a multi-atom submit in 001.
- The X11 present-thread fence wait. That is common WSI code and affects all archs.

Known limit: `vkCmdResetEvent` still takes effect at submit.

#### 009: multi-draw indirect

What changed:
- `vkCmdDraw*Indirect` was already unrolled into one patch+MALLOC_VERTEX pair per draw. Each pair now gets its index in a v9 `draw_id` sysval.
- The `*Count` variants unroll maxDrawCount draws. The GPU patch kernel turns draws at or past the count into NULL jobs.
- `maxDrawIndirectCount` is 65535 on v9.
- A draw starts a new batch before the 16-bit job index overflows.

A review by gpt-6.1-sol found a v9 indirect bug that predates this patch: firstInstance was not applied to instance-rate attributes. The same kernel now rewrites the Attribute Offset (`offset + firstInstance / divisor * stride`). It rewrites rather than adds, so a resubmitted command buffer does not accumulate the offset.

#### 010: robustness2

- Noysz 0071 (software bounds checks for texel buffers) is already in our csf-v11/122 for arch 9 and 10. 122 was measured on this G57 with a forced-on test build: CTS robustness2 buffer subset 2324 pass / 0 fail, `buffer_access` 808 / 0.
- 010 only moves the gates: `robustBufferAccess2`, `nullDescriptor` and `VK_EXT_robustness2` now start at v9.
- `robustImageAccess2` stays off on v9.

### Series checks

- `scripts/apply-patches.sh` on a fresh Mesa 5a07217f tree: 152 patches applied, rc 0. The resulting git tree hash is identical to the build tree (4424058a).
- `patch --dry-run -F0` on each patch, against the tree state just before it:
  - new 003 and 007-010: 0 offsets, 0 fuzz;
  - older patches with offsets: jm-v9/001 (9 hunks), 002 (3), 004 (2), and about 15 kbase-common/csf/csf-v11 patches.
- `apply-patches.sh` accepts offsets silently: `git apply` uses no fuzz but does allow line offsets. It also falls back to `git apply --recount`, which `csf/007-gpu-queue-kbase.patch` needs (bad hunk counts). Not changed in this phase; refreshing those patches is a separate job.
- v10-v14: every change is gated `PAN_ARCH == 9`, `<= 9` or `>= 9` where v10+ already had the feature. The `draw_helper.cl` change is inside `#if PAN_ARCH == 9`. The csf-v11 series still applies and builds (same universal build).

### Device results

None yet: no G57 attached.

To run once it is back:
1. Build PanProbe 1.2.4-dev with `-PpanvkSo=/var/tmp/panvk/dist-v9-p1/libvulkan_panfrost.so`.
2. Install it with `timeout ... adb install -r`. Never uninstall it and never use `pm clear`.
3. Run all tests.

Expected changes:
- `gpu_prerast_slice` replay should pass (007).
- `robustness2` null descriptor cases should run instead of skipping.
- DXVK 3.1.1 should get past "Device does not support Vulkan 1.3".

## Phase 2: transform feedback, geometry shaders, clip/cull, vertex stores

Status: 011-013 written, full series (155 patches) applies, Android universal build OK. **Not tested on a G57:** none was attached on 2026-10-11 (only the Poco X6 Pro G615).

Build:
- Tree: `/var/tmp/panvk/v9/mesa-p2` (branch `p2` = mesa-p1 + one commit per patch 011-013).
- `.so`: `/var/tmp/panvk/dist-v9-p2/libvulkan_panfrost.so`, unstripped, 32.9 MB.
- BuildID `0ddd690afc261d4e9a580281fddc650b520e1a28`, sha256 `7e2423dfc2f2928d027e8b1fe354a4c8148faa0cf13e0011a3bfd2054b41d9cd`.

Approach: no Noysz code is copied. v10+ already runs these stages through the csf-v11 `gpu_prerast` framework: lowered and passthrough shader variants, a device record arena, and libpan kernels for the XFB capture, the GS strip table and so on. 011 lifts those shared gates from `>= 10` to `>= 9`. Only the command side is new, and it lives in the v9 section of `jm/panvk_vX_cmd_draw.c`:
- what CSF writes with `cs_store`, the CPU fills into per-draw memory;
- every producer job is a JM barrier job in the vertex/tiler chain;
- the records are drawn through MALLOC_VERTEX with the passthrough VS or GS.

### Patches

| Patch | What | Noysz source |
|---|---|---|
| 011 | `VK_EXT_transform_feedback` (one stream without a GS, 4 buffers, `transformFeedbackDraw`); direct and indirect draws | 0060, 0064 (adjacency is handled by the shared capture) |
| 012 | `geometryShader`, `geometryStreams`, 4 XFB streams, stream select, v10 GS limits; GS and indirect | 0063, 0065 |
| 013 | `shaderClipDistance`, `shaderCullDistance` (8), `vertexPipelineStoresAndAtomics` | 0066 (VPS, clip/cull part) |

#### 011: transform feedback

- New kernels, all inside `#if PAN_ARCH == 9`:
  - `panlib_v9_xfb_begin` / `panlib_v9_xfb_end`: counter load and store.
  - `panlib_v9_prerast_indirect`: reads one `VkDraw(Indexed)IndirectCommand` and patches the params, the lowered-VS COMPUTE job (type and grid), the first vertex / base instance sysvals and the generated draw command. It also patches the GS job and counts when a GS is bound.
- Begin always runs a job, so a resubmitted command buffer starts from offset 0.
- Direct draws larger than the arena are split on the CPU (instance windows, vertex chunks, fan chunks).
- Rasterizer-discard draws skip the generated draw.
- Review by gpt-6.1-sol found three bugs, all fixed before export:
  - JM index offsets are in bytes, but the params and CSF use elements;
  - resubmit offsets;
  - the rasterizer-discard draw.

#### 012: geometry shaders

- The GS kernel is a COMPUTE job.
- Its params and its draw command are in per-draw memory. A `panlib_fill_scalar` job zeroes the draw command (resubmit-safe).
- `panlib_gs_strip_table` builds the strip table for primitive restart.
- Layered GS: the test that drops other layers moved from the GS (`lower_layer_writes`) to the GS passthrough, so XFB keeps real positions. `v9_cmd_draw` now takes `writes_layer` from the GS.

#### 013: clip/cull distances, vertex stores

- Clip/cull distances use the shared lowering, which is already arch-agnostic. The FS discard is added only when the previous stage writes distances, so FPK stays on for every other linked pipeline.
- A VS with stores runs once per vertex as the lowered VS. A draw that cannot be lowered falls back to IDVS.

### Series checks

- `apply-patches.sh` on fresh Mesa 5a07217f: 155 applied, rc 0. The tree hash is identical to mesa-p2 (09ff67fa).
- `patch -F0` for 011-013 on the tree just before each one: 0 offsets, 0 fuzz.
- v10-v14: every behaviour change is gated `== 9` or `<= 9`. The v10+ feature and limit values are unchanged.
- The CSF `panvk_gpu_prerast.c` is also built for v9. Its `<panvk_cmd_buffer.h>` include now picks `jm/` there and `csf/` on CSF.

### Device results

- G57: none attached.
- G615 v11 CSF regression check with this `.so`:
  - PanProbe 1.2.5, side-by-side package `dev.zenithblue.panvktest.v9p2`, bundled driver BuildID checked;
  - result 37/38. The only failure is `swapchain_lifecycle` ("FAIL no surface"): the screen was dozing during the headless autorun, not a driver issue.

### Not done in phase 2 (blockers)

- **Tessellation (0062).**
  - The csf-v11 tess path is a command-stream loop: chunks of patches, then TES output ranges, with a "more" flag set on the GPU. JM has no loop.
  - Needs a JM design. One option: CPU-unrolled chunks for direct draws, with a GPU no-op tail, plus a worst-case bound for TES feeding a GS or XFB.
  - Noysz's version is a different single-pass design on an upstream poly heap. It does not support indirect draws or tess followed by a GS.
- **multiViewport (rest of 0066).**
  - The VS/GS viewport passthroughs already do the x/y transform and scissor in the shader.
  - CSF splits the draw into per-viewport runs only for the depth clamp, again with a command-stream loop.
  - JM plan: one draw when all viewports share a depth range, else one draw per viewport with a `viewport_id` sysval filter (like layers).
  - The packed-index primitive ID of the VS passthrough also needs the run indices.
- **Pipeline statistics (0067).** JM queries are occlusion only. Needs JM query-pool support for the shared pstats counters.
- Indirect XFB/GS draws with more than the arena holds (up to 65536 vertices x instances) are dropped. CSF splits them on the GPU.
- To check on the device:
  - clip/cull distances written by a GS (the GS passthrough has no cull-flag conversion; the same shared code runs on v10+);
  - the 160 MB arenas committed per device on v9.

## Phase 2b: tessellation, multiViewport, queries, large indirect draws

Status: 014-017 written, full series (159 patches) applies, Android universal build OK, G615 regression 38/38. **Not tested on a G57:** none attached on 2026-10-11 (only the Poco X6 Pro).

Build:
- Tree: `/var/tmp/panvk/v9/mesa-p2`, branch `p2b` (= `p2` + one commit per patch 014-017).
- `.so`: `/var/tmp/panvk/dist-v9-p2b/libvulkan_panfrost.so`, unstripped, 32.9 MB, clean build from the fresh-applied tree.
- BuildID `849597ace7f921c550c9ebdb465e3a6ffbcbeae1`, sha256 `f3c30c8887f542d5ae17595ebbdbf766243c6426154d3b5041b3bbc9685cadb8`.
- Warnings: 6, all pre-existing (5x `csf/panvk_vX_gpu_queue.c:858`, 1x android_stub `BacktraceMap.h`).

### Patches

| Patch | What | New v9 feature bits |
|---|---|---|
| 014 | Tessellation through gpu_prerast. GS after tess and XFB after tess are supported. | `tessellationShader`, `extendedDynamicState2PatchControlPoints`, v10 tess limits |
| 015 | Multi-viewport and `gl_ViewportIndex` from the VS, TES or GS | `multiViewport` (16), `shaderOutputViewportIndex` |
| 016 | Pipeline statistics, primitives generated and XFB stream queries | `pipelineStatisticsQuery`, `transformFeedbackQueries`, `EXT_primitives_generated_query` (+ with rasterizer discard) |
| 017 | Indirect gpu_prerast draws that do not fit the arena are split on the GPU | none |

No Noysz code was copied. Every patch ports the csf-v11 design (MIT) onto the jm-v9 backend. The kernels that are only for v9 sit in `#if PAN_ARCH == 9`.

#### 014: tessellation

- CSF runs `gpu_prerast_tess` as a command-stream loop. Each iteration runs, in order:
  1. `panlib_tess_step`;
  2. the lowered VS;
  3. the TCS;
  4. the tessellator count;
  5. `panlib_tess_draw`;
  6. the tessellator write;
  7. the draw (or the TES lowered as a VS when a GS, XFB or viewport index follows).
- JM has no loop, so the CPU unrolls the iterations into the job chain. Every job is a barrier.
- An iteration past the end of the draw does no work:
  - its grids are zero, so the new `panlib_v9_compute_job_grid` turns those COMPUTE jobs into NULL jobs;
  - the draw patch turns the empty draw into a NULL job.
- No CPU readback.
- Iteration count:
  - direct draws get exactly what the chunk plan needs;
  - indirect draws get 8 chunks;
  - when the TES output is nested (fed to a GS, XFB or viewport index), it is cut into ranges that fit the TES records and the GS slots.
- The GS primitive ID base comes from the GPU (`panlib_v9_copy32`).
- Limits:
  - direct draws over 4096 iterations lose the rest (`mesa_loge_once`);
  - indirect draws over 8 chunks lose the rest;
  - a layered tess draw runs the whole tess once per layer;
  - XFB is captured on layer 0 only.
- `shaderTessellationAndGeometryPointSize` stays v10+, because the v9 point size path is unverified.

#### 015: multiViewport

- The passthrough VS or GS does the per-viewport transform and the scissor (clip planes). The hardware scissor is the framebuffer box, the same as CSF.
- The depth clamp is per viewport:
  1. `panlib_vp_runs` builds the viewport runs;
  2. the new `panlib_v9_vp_partition` packs the runs of each viewport;
  3. one indexed indirect draw per viewport, each with that viewport's `minimum_z`/`maximum_z`.
- When all depth ranges are equal, one merged draw is used instead.
- Limit: when the depth ranges differ, primitives are reordered by viewport. CSF keeps the run order.

#### 016: queries

- Report layout is the same as CSF.
- Every counter is added on the GPU:
  - **Draws:** `panlib_update_pstats_draw` runs once per API draw, not per layer. Direct draws get a draw command written at record time. It counts IA vertices and primitives, VS invocations, and clipping (only without a GS or tess). Primitives generated uses the same kernel, with only the IA primitive slot.
  - **GS, tess and XFB:** these kernels count their own output, once per layered draw.
  - **Compute shader invocations:** `panlib_update_pstats_dispatch`.
  - **Fragment shader invocations:** the occlusion counter, while no occlusion query owns it (same ponytail as CSF).
- Begin zeroes the reports with WRITE_VALUE jobs. End closes the batch and writes availability per view, clamped to the pool.
- The pstats copy kernel now reads the one-dword v9 availability objects.
- `TIMESTAMP` stays v10+.

#### 017: large indirect gpu_prerast draws

- This is CSF `gpu_prerast_chunk_loop` with the loop unrolled:
  1. `panlib_chunk_setup`;
  2. 8 iterations, each a `panlib_chunk_step` followed by the lowering of that chunk as an indirect draw.
- The chunk start reaches the lowered VS through the `chunk` field of `panlib_v9_prerast_indirect`.
- A draw the arena holds is one chunk. Iterations past the end become NULL jobs.
- `panlib_chunk_draw.max_insts`: 0 keeps the v10+ cap of 2048 instances; v9 keeps whole instance ranges, as before.
- Indirect strips with primitive restart are now lowered too.
- Limits:
  - a draw that needs more than 8 chunks loses the rest (before this patch the whole draw was dropped);
  - fans keep the old path;
  - without a GS, `gl_PrimitiveID` restarts in each chunk.

### Review (gpt-6.1-sol) and fixes

Fixed before export:
- 014: chunk counts are now computed in 64 bits (`DIV_ROUND_UP` could overflow).
- 016: primitives-generated queries clear and signal every multiview slot. The multiview slot count is clamped to the pool (`render.view_mask` outlives the render pass). A layered draw counts GS/tess kernel output only on its first layer.
- 017: `gl_DrawID` is kept (the chunk no longer resets `draw_index`). The instance cap no longer truncates draws that fit before.

Not fixed, documented limits shared with CSF or listed above:
- the 8-chunk and 4096-iteration bounds;
- viewport reordering;
- the framebuffer-box scissor;
- no FS invocation count while an occlusion query is active.

### Series checks

- `apply-patches.sh` on fresh Mesa 5a07217f (`/var/tmp/panvk/v9/mesa-chk`): 159 applied, rc 0. The tree hash is identical to mesa-p2 `p2b` (d5ebd79f).
- `patch -p1 -F0` for 014-017 on the tree just before each one: 0 offsets, 0 fuzz.
- v10-v14: the only shared-code change is the `max_insts` field (0 on CSF, so the old cap applies). Pool types and copy kernels are gated `>= 9` or `== 9`; v6/v7 are unchanged.

### Device results

- G57: none attached.
- G615 v11 CSF regression with this `.so`:
  - PanProbe 1.2.5 debug, side-by-side package `dev.zenithblue.panvktest.v9p2`, `adb install -r`;
  - the bundled `.so` BuildID is checked as 849597ac;
  - the screen was kept awake with `KEYCODE_WAKEUP` only;
  - result **38/38 pass** (`swapchain_lifecycle` passes now). Run: `/var/tmp/panvk/v9/run-p2b/`.

## Phase 3: depth bounds, per-sample shading, VMR, fill mode, feature audit

Status: 018-020 written. The full series (162 patches) applies. The universal Android build is OK. G615 regression is 38/38. **Not tested on a G57:** none was attached on 2026-10-11 (only the Poco X6 Pro, G615).

Build:
- Tree: `/var/tmp/panvk/v9/mesa-p2`, branch `p3` (one commit per patch 018-020 on top of `p2b`).
- `.so`: `/var/tmp/panvk/dist-v9-p3/libvulkan_panfrost.so`. It is unstripped (32.9 MB) and is a clean build of the fresh-applied tree.
- BuildID `d2364fa6e38c9a3954b1940ab2873e6b6a1e8021`, sha256 `9af0c399b4e9c71cc17af20daefb8f6236d09716c57e66427e39003869ab8ffa`.
- Warnings: 6, all pre-existing (5x `csf/panvk_vX_gpu_queue.c:858`, 1x android_stub `BacktraceMap.h`).

### Patches

| Patch | What | New v9 feature bits |
|---|---|---|
| 018 | Depth bounds FS emulation (csf-v11 092/116) enabled on v9. The v9 DCD gets Evaluate per-sample and Alpha-to-coverage. Attachmentless draws split the batch when `rasterizationSamples` changes (Noysz 0059). | `depthBounds`, `variableMultisampleRate` |
| 019 | `polygonMode` LINE/POINT through the gpu_prerast polygon kernel, as on CSF | `fillModeNonSolid` |
| 020 | `mesa_logw_once` when an indirect draw takes a path with a chunk cap (014/017) | none |

#### 018: depth bounds, per-sample shading, VMR

- Depth bounds:
  - the shader side (`#if PAN_ARCH >= 10` changed to `>= 9`) reads tile depth with LD_TILE, tests it against the `ds.bounds` sysvals and writes SampleMask;
  - `dev->depth_bounds_fs` is used when no FS is bound;
  - there is no v9 hardware field, so this is GPU emulation only.
- 116 FPK rules are kept. An early-fragment-tests FS that writes ZS or feeds an occlusion query gets `MALI_PIXEL_KILL_FORCE_LATE` while bounds are on. This does not apply when the FS can discard, writes its own SampleMask, or alpha-to-coverage is on.
- DCD `evaluate_per_sample` is set for sample shading or a blend shader with more than 1 sample. `alpha_to_coverage` comes from dynamic state. Before this patch, v9 had neither bit.
- VMR (`vmr_secondary`):
  - `v9_attachmentless_samples()` runs first in `v9_cmd_draw`;
  - with no attachments and a sample count different from the batch, it closes the batch (only if its FBD was already emitted) and opens a new one;
  - it then sets `fb.nr_samples` and resets `fb.layout.sample_count`.

#### 019: fill mode

- `v9_prerast_poly_mode()` is NONE when a GS is bound, as on CSF (`gpu_prerast_poly_mode` is skipped with a GS on v10+ too).
- Otherwise:
  1. the lowered VS writes positions to the arena;
  2. a `GPU_POLYGON` compute job culls, applies facing and writes edge/point indices to `POLY_INDEX_OFFSET` plus an indirect draw;
  3. the tiler draws LINES/POINTS with primitive restart.
- Indirect draws patch the polygon job grid through the `gs_job` slot of `panlib_v9_prerast_indirect`. The draw command is zeroed with `panlib_fill_scalar` first.

#### 020: chunk caps

- The caps are not raised. Each chunk unrolls a full job set into the command buffer, so raising them costs command memory on every indirect draw.
- The warning fires once per process at record time:
  - for indirect gpu_prerast draws (`V9_PRERAST_INDIRECT_CHUNKS`);
  - for indirect tessellated draws (`V9_TESS_INDIRECT_CHUNKS`).

### bc_decode (not fixed, blocker)

- On the G57, the BC decode output and compute sampling are bit-identical to the G615 (same nonzero counts). BC emulation is therefore correct on v9.
- The failing step is the vk_meta fragment blit into a 128-bit RGBA32F destination. The lib texture code is shared with v10+, so the suspect is the JM v9 fragment/blend/FB path for 128-bit render targets, or the blit push-descriptor path.
- Root cause is not confirmed. It needs a G57 to bisect (blit to RGBA8 vs RGBA32F, and copy vs blit).

### Feature audit (v9 vs G615 v10), DXVK 3.1.1 FL11_0/FL11_1

Real on v9 now:
- GS, tessellation, `multiViewport`, clip/cull distance;
- `multiDrawIndirect`, `drawIndirectFirstInstance`;
- `depthClamp`, `depthClipEnable`, `depthBounds`;
- `robustBufferAccess2`, `nullDescriptor`, maintenance5/6;
- `logicOp`, VPS&A (drirc default on), `fragmentStoresAndAtomics`;
- XFB + queries, pipeline statistics;
- `fillModeNonSolid`, `variableMultisampleRate`;
- `textureCompressionBC` (emulated; see the bc_decode blit above).

Still v10+ only. None of these is required for FL11_0/FL11_1 in DXVK 3.1.1:
- `alphaToOne`, `shaderTessellationAndGeometryPointSize`, `shaderFloat16`, `samplerFilterMinmax`;
- `robustImageAccess2`, `shaderOutputLayer`, `conditionalRendering`, `EXT_multi_draw`;
- `nestedCommandBuffer`, rasterization order access, `memoryPriority`/pageable;
- ycbcr extras, timestamps, `indirectMemoryCopy`, sparse.

### Review (gpt-6.1-sol) and fixes

- Fixed in 019: `fs.required` was computed only on the plain draw path, so the tess and gpu_prerast paths called `get_fs()` with a stale value. It is now computed first in `v9_cmd_draw_layer()`.
- Not fixed, parity with CSF: polygon mode is ignored when a GS is bound.

### Series checks

- `apply-patches.sh` on fresh Mesa 5a07217f (`mesa-chk`): 162 applied, rc 0. The tree hash is identical to mesa-p2 `p3` (e843809b).
- `patch -p1 -F0` for 018-020: 0 offsets, 0 fuzz.
- v10-v14: the shared-code changes are `#if PAN_ARCH >= 10` to `>= 9` gates only. v10+ behaviour is unchanged.

### Device results

- G57: none attached.
- G615 regression:
  - PanProbe debug, package `dev.zenithblue.panvktest.v9p2`, `adb install -r`;
  - the bundled `.so` BuildID is checked as d2364fa6;
  - result **38/38 pass**. Run: `/var/tmp/panvk/v9/run-p3/`.

## Phase 4: model rows, kbase JM fixes, import cache clean, bc_decode knob

Status: 021-025 written. The full series (167 patches) applies. The universal Android build is OK. G615 regression is 38/38. **Not tested on a G57 or any other v9 part:** only the Poco X6 Pro (G615) was attached on 2026-10-11.

Build:
- Tree: `/var/tmp/panvk/v9/mesa-p2`, branch `p4` (one commit per patch 021-025 on top of `p3`).
- `.so`: `/var/tmp/panvk/dist-v9-p4/libvulkan_panfrost.so`, unstripped, 32977736 bytes.
- BuildID `d86062454a3798a2022e1157869a8772d62ffb8e`, sha256 `45f5ca54b2485629f5314909a7585e093dd593ab8d2f8b2bfd4a7ef09fbe9604`.
- Warnings: all pre-existing (5x `csf/panvk_vX_gpu_queue.c:858`).

### Patches

| Patch | What | New v9 feature bits |
|---|---|---|
| 021 | G78 `(9, 2, 2)` and G78AE `(9, 0, 5)` model rows. Arch 9 ids with no exact row match on arch major and product major, as kbase does (mask `0xf00f`) | none |
| 022 | kbase JM: atom layout probe, DATA dependencies inside one submit, 2 s teardown wait, wait clock fix | none |
| 023 | v9 tiler heap committed in 2 MiB steps (`BASE_MEM_TILER_ALIGN_TOP`), second heap BO for `PANVK_OVERLAP` | none |
| 024 | v9 JM: clean non-coherent dma-buf imports before submit (`DMA_BUF_IOCTL_SYNC`), off with `PANVK_KBASE_IMPORT_SYNC=0` | none |
| 025 | `PANVK_V9_RT128` bisect knob for 128-bit render targets | none |

#### 021: model rows

- kbase product ids (mask `0xf00f`): `0x9000` G77, `0x9001`/`0x9003` G57, `0x9002` G78, `0x9004` G68, `0x9005` G78AE.
- G77 `(9, 0, 0)` from 006 agrees with malisx2 (`0x9000`) and gs101 (`PAN_PROD_ID(9, 0, 0)`). G68 `(9, 2, 4)` is upstream !43414, already in our base.
- gs101's G78 reports `0x9202`, so the row is `(9, 2, 2)`. The product-major fallback covers parts with another arch minor.
- All v9 rows use the G57 tile buffer sizes (16 KiB colour, 8 KiB depth), as malisx2 does for every arch 9 part.
- G78 transaction elimination: gs101 reports stale tiles. PanVK enables CRC only on v10+ (`panvk_image.c`), so v9 needs no quirk.

#### 022: kbase JM robustness (from malisx2)

- Atom layout: a probe atom at stride 72 tells stock kbase (EINVAL, use the 56-byte v2 layout) from MediaTek kbase with `frame_nr` (accepted, keep 64). Before this, a stock kernel could get stride 64 and read every atom 8 bytes off.
- Fault propagation: atoms of the same `vkQueueSubmit` now use DATA dependencies, so a fault in an early batch fails the fences. Dependencies on earlier submits and semaphore waits stay ORDER.
- Teardown: `destroy_gpu_queue` waits up to 2 s for the last atoms before the tiler heap and pools are freed.
- Wait clock: v9 waits use `CLOCK_MONOTONIC`, the same clock as the vk_sync deadlines (found in review).
- Already present, not ported: 256-atom cap (one atom per `JOB_SUBMIT`), render pass split (v9 splits at 5 * 1024 jobs and before the 16-bit job index overflows), TLS sizing (`pan_query_thread_tls_alloc`), `COHERENT_LOCAL` (already the default flag).

#### 023: tiler heap commit

- The 128 MiB heap is `GROW_ON_GPF`. Its Top pointed at the end of the region, and the tiler takes chunks from both ends, so the first frame committed the whole region.
- Now: one 2 MiB commit step with `BASE_MEM_TILER_ALIGN_TOP` (JM-only), and Top is base + 2 MiB. A kernel that refuses the flag (EINVAL or ENOMEM) gets the old allocation.
- `PANVK_OVERLAP` gets a second 128 MiB heap BO instead of two halves of one.

#### 024: imported memory cache clean

- faux123 reports black dashes in 32-bit DXVK games on a G77, fixed by a cache clean of imported memory before submit.
- On JM, host-visible memory is a dma-heap buffer imported into kbase. If the kernel does not grant `COHERENT_SYSTEM`, CPU writes can stay in the CPU cache.
- Such imports are listed. Each `vkQueueSubmit` with command buffers runs `DMA_BUF_IOCTL_SYNC` (END | WRITE) on them, which is GPU-safe (no CPU emulation). Coherent kernels are not affected.
- logcat shows "dma-buf imports are not CPU-coherent" once when the path is active.

#### 025 and bc_decode (not fixed)

- Review found no v9-specific bug for one single-sampled RGBA32F target. pan_fb.c and gallium's pan_desc.c (which renders RAW128 correctly on G57 in Mesa CI) emit the same tile size (256 px), colour allocation (4 KiB), RAW128 formats, strides and offsets. malisx2 selects the same.
- Bisect steps on a G57, PanProbe `bc_decode` each time:
  1. `PANVK_V9_RT128=tile` (64-pixel tiles). If it passes, the tile buffer budget is wrong for 128-bit RTs.
  2. `PANVK_V9_RT128=noround`. If it passes, the blend rounding is wrong.
  3. `PANVK_V9_RT128=raw`. If it passes, the blend conversion is wrong.
  4. If none pass: replace the blit value with a constant in `vk_meta_blit_resolve.c:336` (shader vs output path).
  5. Try `PANVK_DEBUG=noafbc` and `PANVK_DEBUG=linear` to rule out the source image layout.

### Review (gpt-6.1-sol) and fixes

- Fixed in 022: the wait function read `CLOCK_MONOTONIC_RAW` against `CLOCK_MONOTONIC` deadlines.
- Fixed in 023: ENOMEM was not handled in the `TILER_ALIGN_TOP` fallback.
- Fixed in 024: no EINTR/EAGAIN retry on `DMA_BUF_IOCTL_SYNC`.

### Series checks

- `apply-patches.sh` on fresh Mesa 5a07217f (`mesa-chk`): 167 applied, rc 0. The tree hash is identical to mesa-p2 `p4`.
- `patch -p1 -F0` for 021-025: 0 offsets, 0 fuzz.
- v10-v14: all new code is gated on v9 or on the JM kbase path. v10+ behaviour is unchanged.

### Device results

- G57 / G68 / G77 / G78 / G78AE: none attached.
- G615 regression:
  - PanProbe debug, package `dev.zenithblue.panvktest.v9p2`, `adb install -r`;
  - the bundled `.so` BuildID is checked as d8606245;
  - result **38/38 pass**. Run: `/var/tmp/panvk/v9/run-p4/`.
  - The device's `logd` was stopped during the run, so the results were read from the app's `files/autorun.txt`.

## What's next

- **Phase 2 device run (G57):** 011-017. Run PanProbe `xfb`, `geometry`, `clip_cull`, `vertex_stores`, `gpu_prerast_slice`, `large_draw`, `multi_viewport` and tessellation, and CTS `tessellation.*`, `query_pool.statistics_query.*` and `primitives_generated_query.*`. Watch for hangs from the 256-wide workgroup kernels (`panlib_chunk_step`, restart counting) and from the TCS barrier on the G57.
- **Phase 3 device run (G57):** 018-020. Run PanProbe `depth_bounds`, `fill_mode` and `vmr_secondary`, then bisect the `bc_decode` 128-bit blit.
- **Phase 4 device run (G57):** 021-025. Check the logcat "JM atom layout" and "dma-buf imports are not CPU-coherent" lines, the tiler heap commit (`/proc/<pid>/smaps` before and after), the `bc_decode` bisect steps above, and a full PanProbe run (38 tests). G77/G78/G78AE testers: `vkEnumeratePhysicalDevices` and PanProbe.
- **Open items from phase 1:**
  - device validation of 003 and 007-010;
  - CTS multiview, `draw.*indirect*`, `synchronization.basic.event`;
  - a multi-atom `JOB_SUBMIT` (rest of 0068).
