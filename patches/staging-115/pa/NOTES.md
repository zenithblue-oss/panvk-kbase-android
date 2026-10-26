# Staging 115, package P-A: CSF CPU record/submit, depth bounds FS, kbase

Source: `worklogs/g615-dxvk/perf-review-2026-10-26/a-csf-cpu.md` (A*, M*, B*) and
`c-gpu.md` (C1), with the consolidated plan `README.md` in the same directory.

- Base: tag `rc5g-114` (worktree `/home/flux/src/mesa-rc5g`, untouched).
- Work tree: `/home/flux/src/mesa-115-pa`, branch `p115-pa` (10 commits).
- Export: `git -C /home/flux/src/mesa-115-pa format-patch rc5g-114..p115-pa -o patches/staging-115/pa/`.
- Every patch has an env knob. For the driver-side ones, `<knob>=0` restores the 114
  behaviour. They live in `panvk_perf_knobs.h`, are resolved once at the first
  vkCreateInstance with `os_get_option()` (so `debug.mesa.*` properties work on Android), and
  hot paths only test a bit. When any of them is 0, the log has one line
  `panvk: 114 behaviour restored for: ...`. The two kbase knobs are read once per kbase
  device open, with `debug_get_bool_option()`.
- Private build: `/home/flux/build-out/dev-115-pa/libvulkan_panfrost.so` (profile
  g615-v11-csf, API 29, fresh BDIR `/home/flux/build-rc5/bionic-r30-dev-115-pa-final`). This
  is not a release and has not been run on a device.

Status: P-A complete for phase 1. Ten patches (8 on by default, 1 opt-in, 1 A/B-only knob).
M1, B1, B3 and B4 are not implemented; see "Not done" for the reasons and the phase-2 designs.

## Patches

| # | Title | Finding | Arch | Knob (default; 114 value) |
|---|---|---|---|---|
| 0001 | panvk/csf: leave the FS state clean across draws that do not run the FS | A1 | v10+ | `PANVK_CSF_FS_IDLE_CLEAN` (1; 0) |
| 0002 | panvk: v9+ unlowered fragment shader while the depth bounds test is off | C1 | v9+ | `PANVK_DB_ALT` (1; 0) |
| 0003 | panvk: PANVK_DEPTH_BOUNDS=0 hides the depthBounds feature | C1 (A/B knob) | all | `PANVK_DEPTH_BOUNDS` (1; 1) |
| 0004 | panvk/csf: rebuild the VS tables and push uniforms after a repeated indirect draw | latent bug found under A2 | v10+ | `PANVK_CSF_INDIRECT_REEMIT` (1; 0) |
| 0005 | panvk/csf: descriptor binds and pushes dirty only the stages that use the sets | A2 | v10+ | `PANVK_CSF_DESC_DIRTY` (1; 0) |
| 0006 | panvk: vkCmdBindVertexBuffers2 dirties VB only when a binding changes | A4 | all | `PANVK_VB_DIRTY_CMP` (1; 0) |
| 0007 | panvk/csf: evaluate the gpu_prerast triggers once for a plain draw | A5 | v10+ | `PANVK_PRERAST_FAST_SKIP` (1; 0) |
| 0008 | kbase: decouple shaderDeviceClock from the coherency mode, log the mode | timestamp_device_coherent (B2 overlap) | kbase, all | `PANVK_KBASE_STRICT_DEVICE_CLOCK` (1; 0) |
| 0009 | kbase: opt-in system-coherent CPU-cached BOs on ACE-Lite/ACE (CSF) | B2 | kbase CSF | `PANVK_KBASE_COHERENT_POOLS` (0 = 114; 1 enables) |
| 0010 | panvk: descriptor pools search for a free set from a hint, reset visits used sets | M3 | all | `PANVK_DESC_POOL_HINT` (1; 0) |

All-114 A/B set: `PANVK_CSF_FS_IDLE_CLEAN=0 PANVK_DB_ALT=0 PANVK_CSF_INDIRECT_REEMIT=0
PANVK_CSF_DESC_DIRTY=0 PANVK_VB_DIRTY_CMP=0 PANVK_PRERAST_FAST_SKIP=0
PANVK_KBASE_STRICT_DEVICE_CLOCK=0 PANVK_DESC_POOL_HINT=0` (and leave
`PANVK_KBASE_COHERENT_POOLS` unset).

### 0001 A1: FS state stays clean across draws that do not run the FS
- **What:** `clear_dirty_after_draw()` re-dirtied FS after every draw whose bound FS was not
  required. Since csf-v11 092 a depth-only pipeline binds the device depth bounds FS, so
  every depth-only draw rebuilt FRAGMENT_SRT/SPD/FAU, the FS driver set and resource table,
  blend descriptors, ZSD, the three DCD words and the tiler flags. Now those draws leave the
  state clean; `fs.unused` makes the first draw that requires the FS mark FS and
  FS_PUSH_UNIFORMS dirty (a superset of what 114 had dirty at that draw). Every input of
  `fs_required()` is in `fs_user_dirty()`, so the "no FS" state emitted at the first such
  draw stays valid.
- **Gain:** review estimate 0.2-0.5 us CPU per depth-only draw (shadow maps, depth
  prepasses, stencil-only passes), about 10 CS instructions (80 B) and 1-2 descriptor
  allocations per such draw, so less to flush. Measure with a draw-overhead bench (5000
  no-FS draws into D32) or simpleperf: `build_zsd`, `build_dcd_flags`, `build_blend`,
  `set_tiler_idvs_flags` should leave the depth-only profile.
- **Risk:** low. The only new state is `fs.unused`; meta, reset and secondaries dirty FS
  themselves.
- **Regression signal:** PanProbe depth_bounds (depth-only pipeline toggled with a colour
  pipeline), depth_stencil, occlusion_query (depth-only draws with OQ), blend (colour after
  depth-only), descriptor_model; PanPlay cubes (depth prepass on). A missed re-emit shows as
  a draw rendered with no FS (missing colour) right after a depth-only draw.
- **v6/v7/v9 JM:** unchanged (JM clear macro kept); the JM path rebuilds FS jobs per draw.

### 0002 C1: v9+ unlowered FS while the depth bounds test is off
- **What:** port of jm-v7 076 (FS half) to v9+: fragment shaders that get the depth bounds
  lowering while the enable may be dynamic (or without pipeline state) also get an unlowered
  compile (`panvk_shader::alt`). `cmd_select_db_alt_fs()` picks the compile at the start of
  every CSF draw and v9 JM draw layer, before `fs.required`: lowered while
  `DBT enable && depth attachment` (same predicate as `build_dcd_flags()`'s `db_on`, the v9
  JM DCD and the `ds.enable` sysval), unlowered otherwise, with FS/FS_PUSH_UNIFORMS dirty on
  a change. Nested [de]serialization un-guarded, `panvk_hash_state()` hashes whether the FS
  carries the alternate. jm-v7 098 rebind rule on every arch.
- **Gain (GPU):** every draw of a DXVK fast-linked pipeline gets FPK and the early ZS update
  back, and an FS with all colour writes masked is skipped again. Review estimate 0.5-3 ms
  per full-screen opaque pass with overdraw on affected draws (FO4-class FS); cube about 0.
- **Cost:** those FS are compiled twice (2x FS compile time for DXVK libraries with a dynamic
  DBT enable) and their binaries take twice the memory (rough: ~10 MB for FO4's ~2265
  libraries, not measured). The late clone (perf-review-2026-10-22 item 8) is phase 2.
- **Risk:** low-medium: the selection must match `db_on` exactly (it does, same three
  terms), and switching compiles must re-dirty FS state (FS + FS_PUSH_UNIFORMS, as a bind).
  Cache format changes on v9+ (UUIDs include the build-id, so no 114 blob is read).
- **Regression signal:** PanProbe depth_bounds (dynamic enable on/off, with and without a
  depth attachment), depth_stencil, occlusion_query, blend; compare with `PANVK_DB_ALT=0`.
  A wrong selection shows as depth bounds not applied (enable on) or as fragments killed
  while the test is off.
- **Integration:** one call added at the top of `v9_cmd_draw_layer()` in
  `jm/panvk_vX_cmd_draw.c` (P-D's file; no other JM change).

### 0003 C1 A/B knob: PANVK_DEPTH_BOUNDS=0
- **What:** hides `depthBounds` (all archs), so DXVK declares no depth bounds state and no FS
  is lowered: the pre-092 state. Measures the whole emulation cost against 0002, and is a
  fallback if 0002 misbehaves on a device. D3D12 depth bounds (vkd3d-proton) and DXVK's NVAPI
  extension are lost while it is 0.
- **Risk:** none at the default. **Regression signal:** depth_bounds test skips with the knob 0.

### 0004 Latent bug: VS tables and push uniforms after a repeated indirect draw
- **What:** found while checking A2's register assumptions. An indirect draw whose VS has
  per-instance attributes gets one VS resource table and driver set per draw
  (`desc_repeat_count`), and one whose VS reads firstVertex/firstInstance gets one push
  uniform block per draw; the draw loop advances VERTEX_SRT and VERTEX_FAU after every draw,
  so they end one past the last copy. The next direct draw with the same state and zero
  bases dirtied neither and drew with those registers. In 114 any descriptor bind in between
  hid it; 0005 removes that, so this goes first. After the loop the draw now marks
  BASE_INSTANCE dirty when the tables were repeated and VS_PUSH_UNIFORMS when the push
  uniforms were (set after `clear_dirty_after_draw()`, so they reach the next draw only).
- **Gain:** none (correctness). Cost: one VS table and FAU rebuild on the draw after such an
  indirect draw.
- **Risk:** low (only adds dirty bits).
- **Regression signal:** PanProbe draw_params (indirect draws with firstInstance), large_draw,
  gpu_prerast_slice (indirect slicing), vertex_stores. The bug itself shows as a direct draw
  after a multi-draw indirect using wrong instance attributes or base values.

### 0005 A2: descriptor binds and pushes dirty only the stages that use the sets
- **What:** on v10+, CmdBindDescriptorSets2KHR and both push paths go through
  `csf_gfx_desc_dirty_sets()` (CSF form of jm-v7 097's `v7_gfx_desc_dirty_sets()`):
  DESC_STATE plus a per-set mask, exact while every DESC_STATE dirtying since the last draw
  came through it (`csf_dirty_sets_valid`). An identical rebind dirties nothing.
  `vs_desc_dirty()`/`fs_desc_dirty()` test DESC_STATE only through the sets their shader uses
  (`used_set_mask`, which covers the dynamic-buffer sets too); an FS that does not run has no
  tables. DESC_STATE from meta restore, secondaries or segment splits means every set. The two
  driver-set builders no longer set DESC_STATE (prepare_vs/prepare_fs evaluate the same
  predicates, so VERTEX_SRT/FRAGMENT_SRT are emitted exactly when a table is rebuilt).
- **Gain:** review: med-high. A draw where only FS sets changed no longer rebuilds the VS
  driver set (16 ATTRIBUTE descriptors with a format lookup each, the dummy sampler, dynamic
  buffers, one BUFFER per vertex buffer: 18-27 descriptors, 0.6-0.9 KiB) and VS resource
  table, and vice versa; a VS-only rebuild (vertex buffers, base instance) no longer rebuilds
  the FS tables.
- **Risk:** low-medium. A set a stage uses but `used_set_mask` misses would draw with a stale
  table.
- **Regression signal:** PanProbe descriptor_model, vkd3d_heap, robustness2, sampler,
  bachata_exec, draw_params; PanPlay D3D11 cube. Symptom: wrong texture/UBO/SSBO in one stage
  after a bind (compare with `PANVK_CSF_DESC_DIRTY=0`).
- **v9 JM:** unchanged (keeps DESC_STATE on every bind; its draws rebuild the tables anyway).
  v6/v7 keep 097.

### 0006 A4: vkCmdBindVertexBuffers2 dirties VB only when a binding changes
- **What:** the bind computes each binding's address and size as before and sets VB dirty
  only if one of them, or the binding count, changed. Strides already compare in the runtime.
  Meta still restores binding 0 and dirties VB itself.
- **Gain:** review: low-med. An identical rebind no longer rebuilds the CSF VS driver set and
  resource table; on v6/v7 it no longer defeats the jm-v7 097 attribute table cache.
- **Risk:** very low (every VB consumer only reads address and size).
- **Regression signal:** PanProbe draw_params, large_draw, vertex_stores, clip_cull; PanPlay
  cubes. Symptom: wrong vertices after a rebind of a different buffer.

### 0007 A5: gpu_prerast triggers evaluated once for a plain draw
- **What:** a CSF draw keeps the result of the first `gpu_prerast_wanted()` and, when it is
  false and `PANVK_DEBUG=gpu_prerast` is off, skips `gpu_prerast_draw()` (chunk_wanted,
  lowers, can_lower, poly_mode again) and the second `gpu_prerast_needed()`.
  `gpu_prerast_can_lower()` is false unless one of the conditions `gpu_prerast_wanted()`
  covers holds, and nothing between the checks changes their inputs.
- **Gain:** review: low, about 50-150 ns CPU per plain draw.
- **Risk:** low.
- **Regression signal:** PanProbe gpu_prerast_slice, geometry, tessellation, xfb,
  multi_viewport, vs_viewport_index, gs_viewport_depth, fill_mode, vertex_stores,
  tess_cond_state, gs_tess_primitive_id. Symptom: a draw that needed the pre-rasterization
  path rendering through the plain path (missing xfb output, wrong polygon mode or viewport).

### 0008 timestamp_device_coherent: shaderDeviceClock no longer follows the coherency mode
- **What:** `KBASE_GPUPROP_RAW_COHERENCY_MODE` is the *selected* mode in the COHERENCY_ENABLE
  encoding (ACE-Lite 0, ACE 1, none 31), but 114 read it as "0 = none" and set
  `timestamp_device_coherent = (mode != 0)`: shaderDeviceClock on for COHERENCY_NONE (the
  G615 MC2 phones) and off for ACE-Lite or an unreported mode (the MC6 dev device). The
  review called it inverted, but inverting it would still be wrong: the flag means the GPU
  timestamp reaches the shader cores (panfrost: cycle-count job requirement; panthor 1.6:
  GLB_COUNTER_EN), and nothing in the kbase backend arranges that (JM runs the counter only
  for PERMON atoms; kbase does not report whether CSF firmware has GLB_COUNTER_EN). So the
  device scope is no longer advertised. shaderSubgroupClock and timestamp queries are
  unchanged. The raw mode is stored in `kbase_kmod_dev::coherency_mode` and logged once per
  device open: `kbase: coherency mode 0 (ACE-Lite), shaderDeviceClock off (strict, ...),
  coherent pools ...`.
- **Gain:** none (correctness; the log line is what B2 needs from a device run).
- **Risk:** low. vulkaninfo changes on stock MC2-class kernels (shaderDeviceClock true ->
  false); no known DXVK, vkd3d-proton or PanProbe user (the compliance lists treat
  VK_KHR_shader_clock as optional, and the extension stays exposed).
- **Regression signal:** vulkaninfo `shaderDeviceClock = false` on every kbase device; CTS
  `dEQP-VK.glsl.shader_clock.*` device-scope cases become NotSupported. The log line must
  appear once per device open.
- **Arch:** all kbase devices, JM included (see the P-D notes below).

### 0009 B2 (opt-in): system-coherent CPU-cached BOs on ACE-Lite/ACE
- **What:** with `PANVK_KBASE_COHERENT_POOLS=1` on a CSF device whose coherency mode is
  ACE-Lite or ACE, a WB_MMAP allocation that is not executable, growable, GPU-uncached,
  NO_MMAP or CSF event memory asks for BASE_MEM_COHERENT_SYSTEM instead of
  BASE_MEM_COHERENT_LOCAL (kbase rejects both together). If the returned flags keep it, the BO
  is `cpu_gpu_coherent` (no MEM_SYNC after zeroing or in the kernel path) and gets
  PAN_KMOD_BO_FLAG_IO_COHERENT, so `pan_kmod_queue_bo_map_sync()` returns early for it: no
  `dc cvac`/`civac` for command buffer CS/descriptor pools, descriptor pools, device pools,
  meta/BC uploads and HOST_CACHED memory. No caller changes. If kbase drops the flag or the
  allocation fails, the region is freed, the BO is allocated again with the 114 flags and
  the knob turns itself off for the device (one warning). GPU caches are still flushed and
  invalidated by the CS exactly as before.
- **Logs:** the device-open line ends with `coherent pools requested|off (...)`; the first
  coherent allocation logs `kbase: coherent pools on: the kernel kept
  BASE_MEM_COHERENT_SYSTEM (flags 0x...)`, or a warning `kbase: coherent pools off: ...`.
- **Gain:** potentially the largest CPU item on the X6 Pro (review: every dc op on the
  recording path goes; A3 alone is at least 192 KiB = 3072 lines of `dc cvac` per primary
  command buffer, plus a clean per vkUpdateDescriptorSets*). Not measured.
  `PANVK_DEBUG=no_wb_mmap` gives an upper bound today.
- **Risk:** high if the platform is not really coherent for these mappings (stale CS or
  descriptors: GPU faults, corruption), hence default off. GPU-side snoop cost for CS and
  descriptor fetches is expected to be small.
- **Device A/B (one device, X6 Pro first):** check the coherency line (needs mode 0 or 1),
  then run the 39-test PanProbe suite with `PANVK_KBASE_COHERENT_POOLS=1` (submit_stress,
  csf_event, descriptor_model, large_draw, bc_decode, vkd3d_timeline, swapchain_lifecycle
  matter most), the PanPlay cubes, and CTS `dEQP-VK.memory.*` and `dEQP-VK.synchronization.*`
  if time allows. CPU: simpleperf share of `util_flush_range_no_fence` /
  `util_flush_inval_range_no_fence`, submit_stress time, FO4 menu frame time. Regression
  shows as kbase CS/MMU faults in the log or corrupted frames.
- **Not covered (phase 2):** dma-heap imports (they already ask for COHERENT_SYSTEM but keep
  the CPU maintenance), the direct dc ops in `csf/panvk_vX_gpu_queue.c` (rings are not
  CPU-cached and seqno cells are CSF event memory, so they are unaffected), a
  HOST_CACHED|HOST_COHERENT memory type for DXVK when coherent, the executable pool, JM.

### 0010 M3: descriptor pool free-set hint and reset
- **What:** `panvk_desc_pool_allocate_set()` searched the free-set bitmap from word 0
  (O(sets/32) per allocation when filling a pool). `free_sets_hint` is a word index below
  which every word is 0: lowered by every free, reset to 0 on reset, moved to the found word
  on allocation. The search starts there and returns the same set as the scan from 0 (same
  allocation order). `ResetDescriptorPool()` visits the allocated sets (0 bits) a word at a
  time instead of testing every slot. Pools are externally synchronized, so no lock.
- **Gain:** low: O(1) amortized set allocation, cheaper resets of large pools. FO4 shows
  only ~1.5 MB of descriptor pools (112 run), so expect little there.
- **Risk:** very low (same results by construction).
- **Regression signal:** PanProbe descriptor_model, vkd3d_heap, bachata_exec. A hint bug
  would show as VK_ERROR_OUT_OF_POOL_MEMORY where `PANVK_DESC_POOL_HINT=0` succeeds.

## Not done, and phase 2

- **M1 (register shadow), skipped.** Gain 20-40 ns and up to 40 B of CS per draw, but
  medium risk: the IDVS and COMPUTE staging registers overlap (r0-r39) and precomp /
  gpu_prerast kernels run on the COMPUTE subqueue; the indirect loop advances
  VERTEX_SRT/VERTEX_FAU and loads INDEX_COUNT and four more registers from memory; the
  fullscreen and meta paths write the same registers; VARY_SIZE depends on the per-draw
  generated flag. Phase-2 design (`PANVK_CSF_SR_SHADOW=0`): a per-command-buffer shadow of
  GLOBAL_ATTRIBUTE_OFFSET, INSTANCE_OFFSET, VARY_SIZE, INDEX_BUFFER and INDEX_BUFFER_SIZE,
  invalidated at Begin, after every CALL into a secondary, around vk_meta and meta draws,
  after `split_render_ctx`, after each indirect draw, after any precomp/gpu_prerast dispatch
  on the same subqueue, and wherever other code writes those registers.
- **B1 (KCPU fence wait), skipped.** The KCPU path is opt-in (`PANVK_KBASE_KCPU_SYNC=1`, the
  queue is only created then), so the mutex-across-poll and one-wait-per-device costs do not
  exist by default. The default wait (`kbase_kmod_csf_wait_event()`) is a lock-free ppoll on
  the device fd plus one read; the r-series CSF `read()` returns an EVENT record at once
  even when nothing is pending, so the two-waiter read race the review raised should not
  happen (to confirm on the vendor kernel). Phase 2, only if KCPU becomes the default:
  poll without the lock on a dup()ed fence fd, one CQS_WAIT for all pending subqueues
  (the export path already builds that shape), a small KCPU queue pool, knob
  `PANVK_KBASE_KCPU_MULTI=0`.
- **B4 (deferred userland cache ops), not implemented.** It moves the dc ops from the
  recording thread to the submit thread rather than removing them, the deferred path pays a
  mutex and the O(n) merge scan of `pending_bo_syncs` per request (more than the 1-50 lines a
  descriptor update cleans), its main producer (`panvk_pool_flush_maps` at End, A3) is
  P-B's, and B2 removes the same work where coherency holds. Phase-2 design
  (`PANVK_DEFER_CACHE_OPS=0`): defer only CPU_CACHE_FLUSH (clean) requests flagged
  deferrable by their caller (pool flush at End, descriptor set flush), never invalidates
  or user-memory flushes; queue them merged per BO into `pending_bo_syncs`; run them as
  userland dc ops plus one fence in `kbase_kmod_flush_bo_map_syncs()` (already called before
  ring emission at submit, at the queue init stream and at JM submit;
  `pan_kmod_bo_cleanup()` already flushes a BO's pending syncs before it is freed). Audit
  first: sparse binds, every direct `kbase_cache_clean_range()` user.
- **B3 (native kbase timelines), design only (high risk).** A kbase timeline vk_sync type
  with TIMELINE | CPU_WAIT | GPU_WAIT | GPU_MULTI_WAIT | WAIT_PENDING | CPU_SIGNAL: a small
  ring of (value -> per-subqueue seqno targets), CPU waits on those targets, GPU waits
  through `kbase_gpu_waits`. Without WAIT_BEFORE_SIGNAL the runtime then picks ASSISTED
  (THREADED_ON_DEMAND) instead of EMULATED/DEFERRED, so submission is immediate unless a
  wait-before-signal happens. Knob `PANVK_KBASE_NATIVE_TIMELINE=0`. Gain low-medium (us per
  submit, fewer lock hops); needs CTS timeline_semaphore/synchronization2, sync_file
  import/export and WSI coverage.
- **Small items skipped:** M2 (push constant memcmp; shader binds already dirty the push
  uniforms, so DXVK's re-push after a pipeline change saves nothing, and vkd3d root
  constants change), M4 (WAIT_ANY spin, rare in DXVK), M5 (dsb per submit, in the ring
  emission code that P-C's flush-ID work touches), M6/M7 (low, upstream), M8 (needs a
  frame-path sync-file import first). A3 and B5 are P-B's.
- **C1 phase 2:** the late clone (compile the unlowered FS only when a draw first needs it)
  to drop the 2x FS compile cost.

## Integration notes for the other packages

- **P-B (memory, pools, CS flushing, BO zeroing):** 0009 edits `kbase_kmod_bo_alloc()` in
  `src/panfrost/lib/kmod/kbase_kmod.c` around the ALLOC_EX/ALLOC calls, just above the zeroing
  memset + MEM_SYNC block; a P-B change there will conflict textually (coherent BOs already
  skip that MEM_SYNC). With 0009 on, `panvk_pool_flush_maps()` still walks its ranges but
  every call returns early in `pan_kmod_queue_bo_map_sync()`; P-B could skip the walk for
  IO_COHERENT BOs. A3 (whole-chunk flushes) is moot where 0009 holds.
- **P-C (present, barriers, flush IDs):** 0009 changes no GPU cache operation. CPU-written
  data still needs the CS-side L2 invalidate between the CPU write and the GPU read, which is
  what the flush-ID logic protects; nothing in P-A depends on P-C.
- **P-D (JM):** 0002 adds one call at the top of `v9_cmd_draw_layer()` in
  `jm/panvk_vX_cmd_draw.c`. 0005 is a no-op on v9 JM. 0006 and 0010 apply to v6/v7/v9 too
  (0006 helps the jm-v7 097 attribute cache). 0008 turns shaderDeviceClock off on JM kbase
  devices as well. Related, for P-D: shaderSubgroupClock on JM kbase reads a counter that
  only runs for PERMON atoms (`batch->timestamps`), so a batch whose shaders read the clock
  should also get PERMON.

## Build

- Incremental builds after each patch (BDIR `/home/flux/build-rc5/bionic-r30-dev-115-pa`):
  all OK, no new warnings in touched files.
- Final fresh build of `p115-pa` HEAD `3e959a5e` (BDIR
  `/home/flux/build-rc5/bionic-r30-dev-115-pa-final`, NDK 29.0.14206865, `--profile
  g615-v11-csf --api 29`, 4 jobs): OK. `libvulkan_panfrost.so` 30694568 bytes, sha256
  `95f3d81abf7fb4aea7b516a82028b1577c8ccea5edc247509eb514ba6ed483cd`.
- Warnings in that build: 9, all outside the touched files and all present before
  (`csf/panvk_vX_gpu_queue.c:859` C23 label x5, `jm/panvk_vX_gpu_queue.c:182` misleading
  indentation x3, `android_stub/backtrace/BacktraceMap.h:74` deprecated iterator x1).
