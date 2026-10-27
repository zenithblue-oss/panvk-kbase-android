# Staging 115, package P-C: present paths, GPU-side passes and barriers

> **Promoted (2026-10-26):** these 7 patches (0001-0007) are now `patches/jm-v7-wip/0113-0119` (the same files, renumbered in apply order: patch 000N here is jm-v7-wip 112 + N). `scripts/prepare-rc5-tree.sh` applies them (default `--wip-max 0131`, the build 115 tree); see `patches/jm-v7-wip/README.md`, section "Build 115". The `patches/staging-115/...` paths and the apply/export commands below are from before the move.

Source: `worklogs/g615-dxvk/perf-review-2026-10-26/c-gpu.md` (C2, C4, C6, C7, C8, C10),
`d-jm.md` (D13, which is the 2026-10-22 review's K1), `a-csf-cpu.md` (M8), and the
correctness items in `README.md` of the same directory (unnamed `wsi_device_options` field,
flush ID forced to 0 on own-subqueue waits, v10 ZSD/DCD early-ZS disagreement).

- **Applies on top of P-A and P-B.** Apply `patches/staging-115/pa/` 0001-0010, then
  `patches/staging-115/pb/` 0001-0010, then these seven. Branch `p115-pc`, worktree
  `/home/flux/src/mesa-115-pc`, 7 commits on `p115-pb` (head f8e249a0, which is on
  `p115-pa` 3e959a5e, on tag `rc5g-114`). They do not apply to a bare `rc5g-114` (they use
  P-A's knob registry `panvk_perf_knobs.h`).
- Export: `git -C /home/flux/src/mesa-115-pc format-patch p115-pb..p115-pc -o patches/staging-115/pc/`.
- Knobs: 0003-0007 use P-A's registry (`panvk_perf_knobs.h`, bits 8-12, names in
  `panvk_perf_knob_names[]` in `panvk_instance.c`): resolved once at the first
  `vkCreateInstance()` with `os_get_option()`
  (so `debug.mesa.*` properties work on Android), on unless `<NAME>=0`, and logged once as
  "panvk: 114 behaviour restored for: ...". The two WSI knobs are copied into
  `struct wsi_device` at physical-device init. C6 reuses csf-v11/061's
  `PANVK_ZS_PRELOAD_ALWAYS` (`DEBUG_GET_ONCE_BOOL_OPTION`, cached). Nothing reads the
  environment on a hot path.
- Private build: `/home/flux/build-out/dev-115-pc/libvulkan_panfrost.so` (profile
  g615-v11-csf, API 29, NDK 29.0.14206865, fresh BDIR
  `/home/flux/build-rc5/bionic-r30-dev-115-pc-final2`, 4 jobs, under the build lock): OK,
  29900304 bytes, sha256
  `de4c21fdcf197260b4865398f2ac1b493ce928053eb84c2e814f17146bf1b593`. Platforms android and
  x11 (Wayland is not compiled in this profile, so 0003's one-line Wayland hunk is
  untested). Warnings: the same nine as the 114 build (csf/panvk_vX_gpu_queue.c:859 C23
  label x5, jm/panvk_vX_gpu_queue.c:182 indentation x3, BacktraceMap.h:74), none in touched
  code. This is not a release and has not been run on a device.
- Status: P-C phase 1 complete. Seven patches, all on by default (0001 has no behaviour
  change). C8, C7 and C10 are not done (reasons below), M8 and two of the three
  correctness items are documented, not changed.

## Patches

| # | Title | Finding | Arch / path | Knob (default; 114 value) |
|---|---|---|---|---|
| 0001 | panvk/wsi: name the extra_xwayland_image device option | wsi fix (README) | all | none (same value, now named) |
| 0002 | panvk: use INTERSECT ZS preload on v9 and v10 too | C6 | v9 (JM), v10 (CSF) | `PANVK_ZS_PRELOAD_ALWAYS` (0; 1) |
| 0003 | wsi: read sw frames back through cached memory, invalidate first | C2 (a) | X11 sw WSI (PanPlay/DXVK), Wayland SHM | `PANVK_WSI_SW_CACHED` (1; 0) |
| 0004 | wsi/x11: collect sw MIT-SHM present replies on the event thread | C2 (b) | X11 sw WSI with MIT-SHM | `PANVK_WSI_SW_DEFER_REPLY` (1; 0) |
| 0005 | panvk/wsi: Android present waits for the image's fence, not the device | K1 | PanVK's own Android swapchain, all arches | `PANVK_ANDROID_PRESENT_FENCE` (1; 0) |
| 0006 | panvk/wsi: Android swapchain presents on its own thread | D13 | PanVK's own Android swapchain, all arches | `PANVK_ANDROID_PRESENT_THREAD` (1; 0) |
| 0007 | panvk/csf: skip the preload drain when no fragment job wrote what the pass loads | C4 | v10+ (CSF) | `PANVK_PRELOAD_BARRIER_TRACK` (1; 0) |

All-114 A/B set: `PANVK_ZS_PRELOAD_ALWAYS=1 PANVK_WSI_SW_CACHED=0 PANVK_WSI_SW_DEFER_REPLY=0
PANVK_ANDROID_PRESENT_FENCE=0 PANVK_ANDROID_PRESENT_THREAD=0 PANVK_PRELOAD_BARRIER_TRACK=0`.
(`PANVK_ZS_PRELOAD_ALWAYS=1` also turns v11's 061 INTERSECT off; for a pure 114 v11, leave
it unset on v11 devices.)

Expected gains (estimates, not measured; nothing here has run on a device):

- **PanPlay DXVK cubes (X11 sw present, G615):** C2 is the one that matters. The review
  estimates 3.5 ms per present at 1280x705 (10 ms at 2400x1080) for the uncached read plus
  the blocking round trip; 0003 turns the read into a cached one (plus one invalidate of
  the image, roughly 0.3-0.6 ms at 1280x705), 0004 takes the round trip off the present
  thread. Estimate: cube 47 fps to about 55-60+ fps; low confidence, because the Winlator X
  server's own cost per ShmPutImage is unknown and may become the limit. C4 and C6 do
  nothing for the cube (one pass, no depth load).
- **Native Android Vulkan apps on PanVK's own swapchain (PanProbe, cube APKs):** 0005 drops
  the DeviceWaitIdle per present, 0006 overlaps the CPU copy (about 8 MB per 1080p frame)
  and the window's dequeue with the next frame's recording and GPU work. Gain: up to the
  copy time per frame (several ms at 1080p).
- **Multi-pass D3D11 frames (Fallout 4):** C4 saves up to one fragment drain (10-50 us
  each) per LOAD pass whose targets were not rendered since the last drain, so between 0
  and the review's 0.3-1.5 ms per 30-pass frame; C6 saves up to 13 MB of Z/S reads per
  depth-loading pass at 2400x1080 on v9/v10 (G78, G710/G610), nothing on the G615.

### 0001 wsi fix: named `extra_xwayland_image`
- **What:** `wsi_device_init()`'s options in `panvk_wsi_init()` had a positional value after
  the designated ones; it now names `.extra_xwayland_image = kbase_dmabuf && kbase_raw_dri3`
  (the field it already initialized). Same value, no behaviour change, so no knob.

### 0002 C6: INTERSECT Z/S preload on v9 and v10
- **What:** `cmd_preload_zs_attachments()` (`panvk_vX_cmd_frame_shaders.c`) used
  EARLY_ZS_ALWAYS on v9/v10, which reads Z/S for every tile of a pass that loads them,
  including tiles no primitive touches. csf-v11/061 had switched only v11 to INTERSECT. Now
  v9-v11 use INTERSECT unless `always_load()` (a target that has to be written for empty
  tiles too, e.g. an in-bounds CLEAR with a border load). v12 keeps EARLY_ZS_ALWAYS, v13+
  PREPASS_ALWAYS.
- **Why it is safe:** a load target that is not `always` is not written back for tiles
  without primitives, so not preloading those tiles changes nothing in memory. The colour
  preload already uses INTERSECT on every arch.
- **Risk:** low. EARLY_ZS_ALWAYS preloaded tiles ahead; INTERSECT can start the ZS test of a
  tile a little later (not benchmarked upstream either).
- **Validate:** PanProbe depth_stencil, depth_bounds and any partial-render-area depth LOAD
  test on oriole (v9) and panther (v10), with `PANVK_ZS_PRELOAD_ALWAYS=1` as the A/B.

### 0003 C2 (a): sw readback through cached memory
- **What:** on the sw WSI path the present copies every frame on the CPU from the blit
  buffer (or the linear CPU image). With `wsi_device::sw_cached_noncoherent` set (only PanVK
  sets it), `wsi_select_cpu_readback_memory_type()` prefers HOST_VISIBLE | HOST_CACHED
  (HOST_COHERENT too when the device has such a type) for images that are not allocated in
  shared memory, and `wsi_image_invalidate_cpu_map()` invalidates the whole mapping
  (`vkInvalidateMappedMemoryRanges`, VK_WHOLE_SIZE) after the present fence and before the
  CPU reads: in `x11_present_to_x11_sw()` (after the geometry request is sent, so the
  invalidate overlaps the round trip) and in the Wayland SHM copy. On kbase without IO
  coherency PanVK's only coherent host-visible type is uncached write-combined memory,
  which the CPU reads at a fraction of the cached speed; that read was most of C2's 3.5 ms.
- **Other drivers:** unchanged (the flag defaults to false, the old selection is used).
- **Risk:** low-medium: cache maintenance. The invalidate runs after the fence wait, so the
  GPU writes are done and cleaned to memory; `panvk_InvalidateMappedMemoryRanges()` does
  the userland DC CIVAC and then `pan_kmod_flush_bo_map_syncs()`. Note for the kmod owners:
  that function's DSB is taken by whichever thread first clears
  `pending_bo_syncs.user_cache_ops_pending` (`p_atomic_xchg`), so with the present thread
  invalidating while the application submits, the DSB can run on another CPU. The copy
  then relies on the same-line ordering of DC CIVAC and later loads on the present
  thread's CPU. If stale or torn frames show up only with `PANVK_WSI_SW_CACHED=1`, check
  this first (a DSB in the invalidate path regardless of the flag would close it).
- **Validate:** PanPlay cube fps A/B (`PANVK_WSI_SW_CACHED=0`), visual check for stale or
  torn frames, PanProbe swapchain_lifecycle.

### 0004 C2 (b): MIT-SHM replies collected on the event thread
- **What:** with 117's MIT-SHM AttachFd segments (`chain->sw_shm`), every present sent
  ShmPutImage plus GetGeometry and blocked on the reply on the present thread (under
  `thread_state_lock`). With `x11.sw_defer_reply` (only PanVK sets it) the present thread
  records the cookie and returns; the sw path's otherwise idle event thread collects the
  replies in order (`x11_sw_collect_reply()`), completes the present IDs and reports errors
  through `x11_swapchain_result()`. An image is not copied into again while its reply is
  pending (the server may still read the segment), and at most two replies are
  outstanding (`X11_SW_MAX_PENDING_REPLIES`). Destroy discards replies still pending after
  the threads are joined.
- **Risk:** medium-low: present-ID completion and error reporting now happen on the event
  thread; out-of-date/surface-lost errors arrive one present later.
- **Validate:** cube fps A/B (`PANVK_WSI_SW_DEFER_REPLY=0`), resize/minimise/close of the
  X11 window (OUT_OF_DATE path), VK_KHR_present_wait users if any.

### 0005 K1: Android present waits for the image's fence
- **What:** `panvk_android_swapchain_queue_present()` (PanVK's own ANativeWindow
  swapchain, used when PanVK is not the system driver: PanProbe, the cube APKs) called
  `vkDeviceWaitIdle()` before copying. It now waits for the image's present fence (submitted
  by `wsi_common_queue_present()`, which on kbase stays on the real queue, so the fence
  covers the frame on JM and CSF). On a sw WSI device that fence was already waited for, so
  the present no longer waits at all.
- **Risk:** low.

### 0006 D13: Android present thread
- **What:** the Android swapchain gets a present thread like the X11 one:
  `vkQueuePresentKHR()` records the present ID and queues the image; the thread waits for
  the fence, copies into the window and posts (`panvk_android_swapchain_post()`, the old
  copy code), completes the present ID and gives the image back. Acquire takes images from
  that queue (blocks, or VK_NOT_READY / VK_TIMEOUT, only while every image is queued or
  being copied); `vkReleaseSwapchainImagesKHR()` returns images to it;
  `vkWaitForPresent2KHR()` waits on a condition variable, and `vkWaitForPresentKHR()`, which
  the device exposes but the swapchain had no callback for, uses the same function. Destroy
  queues a stop marker behind pending presents and joins. If a queue, the mutex, the
  condition variable or the thread cannot be created, the swapchain presents synchronously
  as before.
- **Risk:** medium: threading in the swapchain (destroy while presents are pending, acquire
  timeouts). `PANVK_ANDROID_PRESENT_THREAD=0` restores the synchronous present (with 0005's
  fence wait unless `PANVK_ANDROID_PRESENT_FENCE=0` too).
- **Validate:** PanProbe swapchain_lifecycle (create/destroy/recreate, rotate, background
  and foreground), present_wait if covered, the cube APKs' fps.

### 0007 C4: preload barrier only waits when needed
- **What:** a pass that loads an attachment records, in `cmd_init_render_state()`, a barrier
  attachment writes -> fragment-shader sampled reads; on CSF a fragment-subqueue wait for all
  iterators plus a texture-cache invalidate, even when no earlier fragment job wrote what
  the pass loads. The command buffer now tracks what the fragment jobs issued since the
  fragment subqueue last waited for all its iterators may have written
  (`struct panvk_frag_written`, `panvk_cmd_draw.h`):
  - `issue_fragment_jobs()` adds every plane of the pass's attachments, their
    multisampled-render-to-single-sampled images, their resolve targets and every
    framebuffer store and spill target (a direct MSAA resolve stores straight to the
    resolve image, which is in none of the attachment views) as page-aligned address
    ranges, so aliased memory counts; at most 16 ranges, and an overflow or an unknown
    range (sparse, unbound, a secondary that continues the primary's pass) makes the set
    unusable;
  - any barrier that waits for all fragment iterators (`emit_barrier_csf()`) empties the set
    and makes it valid;
  - the set is not valid until such a wait in this command buffer, and executing
    secondaries clears it (`panvk_cmd_invalidate_state()` clears the graphics state).
  When the set is valid and none of the loaded targets (in-bounds or border loads: the view
  the preload reads, the attachment and its preload view) overlaps it, the barrier keeps
  its destination scope but
  has no source stages: only the fragment-side texture-cache invalidate is emitted. A LOAD
  pass before the command buffer's first full fragment wait still waits, so earlier
  submissions are always covered. JM is unchanged (there the barrier is a batch split,
  P-D's area).
- **Risk:** medium (synchronization): a missed write gives stale preloads, intermittently.
  Application barriers are not changed, and anything the tracking cannot prove falls back
  to the 114 barrier. Flip `PANVK_PRELOAD_BARRIER_TRACK=0` first if any stale-tile or
  flicker artefact appears on v10/v11.
- **Validate:** PanProbe 39/39 and CTS `dEQP-VK.renderpass2.*`, `dEQP-VK.dynamic_rendering.*`
  with both knob values; Fallout 4 menu and in-game visual check; u_trace fragment job gaps.

## Not done in phase 1, and why

- **C8 (lazy compute wait for vertex-stage barriers), skipped.** The finding is right
  (`add_execution_dependency()` adds COMPUTE as a destination for draw-indirect, vertex-input
  and pre-rasterization stages on every DXVK device), but the gain is small (review: under
  0.5 ms/frame, only in frames with D3D11 compute after such barriers, nothing for the cube)
  and a correct lazy wait is not a small change:
  - gpu_prerast emits COMPUTE-subqueue work from more than a dozen places
    (`gpu_prerast_draw()` and its chunk loop, `gpu_prerast_split()`, `gpu_prerast_tess()`,
    the XFB byte-count draw, `patch_vs_attribs()` on COMPUTE, the sync32 take/give pairs);
    the pending wait would have to be emitted before each of them and not before regular
    dispatches;
  - the source subqueues must still self-wait, flush and signal at barrier time although
    no destination waits then (`emit_barrier_csf()` only signals when someone waits), and
    the recorded `relative_sync_point` is only meaningful inside the command buffer, so the
    pending wait must be flushed at `EndCommandBuffer()` and before `CmdExecuteCommands()`;
  - events (`cmd_event.c`), queries and secondaries would stay eager.
  A missed site gives stale vertex data in geometry/XFB/tessellation draws. Phase 2 design:
  in `CmdPipelineBarrier2()` only (primaries), when COMPUTE was added only by the gpu_prerast
  rule, drop it from the destinations, force the sources' signal, record the per-source
  sync point in `cmdbuf->state`, and emit the pending waits from one helper called at the
  draw entry when `prerast_wanted || prim == PATCHES || PANVK_DEBUG(GPU_PRERAST)`, at the
  XFB byte-count entry, at `EndCommandBuffer()` and before executing secondaries. Knob
  `PANVK_PRERAST_LAZY_DEP=0`.
- **C7 (CRC), skipped: needs Q4 first.** The review's own first step is the
  `PANVK_DEBUG=no_crc` A/B (Q4). Allowing MUTABLE targets while 019 still invalidates on
  every CLEAR/DONT_CARE would add CRC traffic to more targets for no hit; keeping CRCs on
  CLEAR is the part that touches 019, which fixed a real G615 failure (IDVS matrix cases
  reading zeros after a CRC-skipped store). Do it once Q4 says whether CRC is a net win.
- **C10 (blend-shader counter), not done.** Optional in the brief; the review wants a
  count first. A per-key `vk_perf` line or a field in the panvk-mem line is a phase-2 item.
- **M8 (imported sync files waited on the submit thread), documented.** Not on PanPlay's
  frame path: DXVK there presents through the X11 sw WSI (no sync-file import), and PanVK's
  own Android WSI imports nothing at acquire. Only `vk_common_AcquireImageANDROID()` (PanVK
  as the system driver, behind the platform swapchain) imports a sync file per frame. Fix
  when that path matters: KCPU FENCE_WAIT + CQS_SET on a per-queue CQS the CS waits on
  (`PANVK_KBASE_GPU_FENCE_WAITS=0` knob).
- **Flush ID forced to 0 on own-subqueue waits, kept (documented).** In the kbase ring
  emission (`csf/panvk_vX_gpu_queue.c`, the wait loop around line 578) `flush_id = 0` is set
  before the `i == subqueue` skip, so an entry that waits on its own subqueue gets a full
  flush at its start. That flush is what makes the previous entry's writes visible to the
  texture and other read-only caches: the end-of-CS flush only cleans L2/LSC and does not
  invalidate the others. With the flush ID read on the CPU at submit time, the hardware
  would skip the start flush as soon as any flush (the previous entry's clean-only one)
  happened after that read, and the texture caches would keep stale lines. Changing it
  needs an explicit "others" invalidate at the end of each entry first; not worth it for
  one flush per submission.
- **v10 ZSD vs DCD early-ZS disagreement, documented.** On v10 `prepare_ds()` gets the
  `earlyzs` that `prepare_dcd()` filled only when the DCD is rebuilt; when only ZSD state is
  dirty it gets `{0}`, so the ZSD's `shader_read_only_z_s` is false while the DCD keeps the
  early-ZS mode it computed earlier. That is the conservative direction (full Z/S
  dependency tracking, no wrong pixels; it can only cost the read-only optimisation), and
  the opposite case cannot happen because a rebuilt DCD always passes its fresh state.
  Fix sketch (phase 2): cache the last `earlyzs` in the command buffer's graphics state and
  pass it when the DCD is clean. v10 only.

## Integration notes

- **233 (fps log):** it appends a block at the very end of `panvk_wsi.c` and hooks
  `QueuePresentKHR`, `DestroySwapchainKHR` and `QueueSignalReleaseImageANDROID`. No P-C
  hunk touches the end of `panvk_wsi.c` (the last one is in `panvk_wsi_init()`, about 15
  lines before `panvk_wsi_finish()`), and P-C renames or removes none of those entry points
  or call sites: PanVK's Android swapchain is still reached through
  `wsi_common_queue_present()` from `vkQueuePresentKHR()`. With 0006 the window post happens
  later on the present thread, so 233 counts the application's present rate, which is
  also the posted rate in steady state (acquire blocks when the thread falls behind).
  0004 does the same for X11 sw: present IDs complete on the event thread.
- **P-A:** P-C uses the registry bits 8-12 after P-A's 0-7; any later package should
  continue at bit 13. P-A's M5 (dsb per submit) was left for the ring-emission code that
  the flush-ID item touches; P-C does not change that code (see above), so M5 is free.
- **P-B:** no overlap (P-B touches the kbase kmod, pools, CS chunks and heaps). 0003 uses
  `vkInvalidateMappedMemoryRanges()` on WSI images, which goes through the kmod BO sync
  path (see the DSB note under 0003).
- **P-D (JM):** 0002 changes v9 (JM) Z/S preload mode; 0005/0006 apply to JM devices that
  use PanVK's own Android swapchain; 0007 is CSF only.

## Phase 2 ideas (not in this package)

- Zero-copy X11 sw present: let the GPU render (or blit) straight into the MIT-SHM memfd
  segment through VK_EXT_external_memory_host or a dma-buf import, removing the CPU copy
  that 0003 only made cheaper.
- Android: present through AHardwareBuffer-backed images (or ASurfaceControl) instead of
  the CPU copy into the window.
- C8, C7, C10 and the v10 ZSD fix as described above.
