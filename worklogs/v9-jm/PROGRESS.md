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

## What's next

- **Phase 2 rest:** tessellation, multiViewport and pipeline statistics (see blockers above). G57 device run of 011-013: PanProbe `xfb`, `geometry`, `clip_cull`, `vertex_stores`, `gpu_prerast_slice`, `large_draw`.
- **Phase 3:** fill mode (non-solid), depth bounds (GPU emulation, no v9 hardware field), `vmr_secondary` (Noysz 0059: no-attachment `nr_samples` / `evaluate_per_sample` in `v9_cmd_draw`), and the `bc_decode` blit.
- **Phase 4:** G77 row (006, needs a tester) and a full PanProbe run (up to 38 tests) on the G57.
- **Open items from phase 1:**
  - device validation of 003 and 007-010;
  - CTS multiview, `draw.*indirect*`, `synchronization.basic.event`;
  - a multi-atom `JOB_SUBMIT` (rest of 0068).
