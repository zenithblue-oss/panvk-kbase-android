# Staging 115, package P-B: memory footprint, budget, command pools, CS chunks

> **Promoted (2026-10-26):** these 10 patches (0001-0010) are now `patches/jm-v7-wip/0103-0112` (the same files, renumbered in apply order: patch 000N here is jm-v7-wip 102 + N). `scripts/prepare-rc5-tree.sh` applies them (default `--wip-max 0131`, the build 115 tree); see `patches/jm-v7-wip/README.md`, section "Build 115". The `patches/staging-115/...` paths and the apply/export commands below are from before the move.

Source: `worklogs/g615-dxvk/perf-review-2026-10-26/b-memory.md` (Q*, B*, J1), A3 from
`a-csf-cpu.md` and the JM auto-trim half of D2's quick part from `d-jm.md`, with the consolidated plan
`README.md` in the same directory.

- **Applies on top of P-A.** Apply `patches/staging-115/pa/` 0001-0010 first (branch
  `p115-pa`, head 3e959a5e, on tag `rc5g-114`), then these ten. They do not apply to a bare
  `rc5g-114`.
- Work tree: `/home/flux/src/mesa-115-pb`, branch `p115-pb` (10 commits on `p115-pa`).
- Export: `git -C /home/flux/src/mesa-115-pb format-patch p115-pa..p115-pb -o patches/staging-115/pb/`.
- Every patch has an env knob, read once at kmod device, physical device, device or queue
  creation with `debug_get_*_option()` / `os_get_option()` (so `debug.mesa.*` properties work
  on Android; nothing reads the environment on a hot path). The values given in the table
  restore 114.
  They are not in P-A's `panvk_perf_knobs.h` registry (several are numbers, not switches).
- Private build: `/home/flux/build-out/dev-115-pb/libvulkan_panfrost.so` (profile
  g615-v11-csf, API 29, NDK 29.0.14206865, fresh BDIR
  `/home/flux/build-rc5/dev-115-pb-final`, 4 jobs): OK, 29867744 bytes stripped, sha256
  `7ea90cbd7603fef5b0f0a0892778191abe38ef694748e231b679fef460c4a412`. Warnings are the same
  three as P-A's final build (csf/panvk_vX_gpu_queue.c:859 C23 label, jm/panvk_vX_gpu_queue.c:182
  indentation, BacktraceMap.h:74), none in touched code. This is not a release and has not
  been run on a device.

Status: P-B phase 1 complete. Ten patches: eight on by default, two opt-in (B4, B6).

## Patches

| # | Title | Finding | Arch | Knob (default; 114 value) |
|---|---|---|---|---|
| 0001 | panvk/csf: destroy completed retired tiler heaps at every submission | Q2 | v10+ kbase | `PANVK_KBASE_HEAP_EAGER_TERM` (1; 0) |
| 0002 | panvk/csf: tessellation arenas with an 8 MiB VS region | Q3 | v10+ | `PANVK_TESS_VS_REGION_MB` (8; 32) |
| 0003 | panfrost/kmod, panvk: kbase: zero only memory that has to be zero | Q4 | kbase: VkDeviceMemory v9+, TLS v10+ | `PANVK_KBASE_ZERO_ALL` (0; 1) |
| 0004 | panvk/csf: flush only the written part of CS chunks at EndCommandBuffer | A3 | v10+ | `PANVK_CS_FLUSH_USED` (1; 0) |
| 0005 | panvk: command pool auto-trim on JM too | J1 (D2 quick part) | v6/v7/v9 | `PANVK_JM_CMD_POOL_AUTOTRIM` (1; 0) |
| 0006 | panvk: memory budget that shrinks under memory pressure | Q1 | all | `PANVK_MEM_BUDGET_PRESSURE` (1; 0), `PANVK_MEM_BUDGET_RESERVE_MB` (256 up to 8 GiB of RAM, else 0) |
| 0007 | panfrost/cs_builder, panvk/csf: CS chunks that grow from 4 KiB | B2 | v10+ | `PANVK_CS_CHUNK_KB` (4; 64) |
| 0008 | panvk: device-wide caches for command buffer CS, descriptor and TLS slabs | B1 | v10+ | `PANVK_CMD_SLAB_CACHE` (1; 0), `PANVK_CMD_SLAB_CACHE_MS` (2000) |
| 0009 | kbase, panvk: opt-in evictable command buffer slabs (BASE_MEM_DONT_NEED) | B4 | v10+ kbase | `PANVK_KBASE_EVICTABLE` (0 = off = 114; 1-64 windows) |
| 0010 | panvk/csf: PANVK_KBASE_HEAP_SLOTS, opt-in fewer live tiler heaps | B6 (a) | v10+ kbase | `PANVK_KBASE_HEAP_SLOTS` (4 = 114; 2-3) |

All-114 A/B set: `PANVK_KBASE_HEAP_EAGER_TERM=0 PANVK_TESS_VS_REGION_MB=32
PANVK_KBASE_ZERO_ALL=1 PANVK_CS_FLUSH_USED=0 PANVK_JM_CMD_POOL_AUTOTRIM=0
PANVK_MEM_BUDGET_PRESSURE=0 PANVK_CS_CHUNK_KB=64 PANVK_CMD_SLAB_CACHE=0` (and leave
`PANVK_KBASE_EVICTABLE` and `PANVK_KBASE_HEAP_SLOTS` unset).

Expected saving for Fallout 4 on the G615 (review estimates against 112, not measured):
about 190-320 MB with the defaults. Q1 100-150 MB of DXVK slack (indirect: DXVK decides what
to give back; 307 MB was idle in 112's end state), Q2 15-50 MB on average and more at
peaks, Q3 24 MiB per committed tessellation arena (48 with the second), B1 + B2 50-100 MB
of the 125-155 MiB of cmd_cs + cmd_desc (the review caps Q5 + B1 + B2 + B4 together at
about 100-120 MiB). Q4 and A3 save CPU time, not memory; J1 is for JM devices; B4 and B6
are off by default (30-100 MB under pressure and one heap of 35-60 MiB at peaks, if
enabled).

### 0001 Q2: retired tiler heaps destroyed at every submission
- **What:** `kbase_try_destroy_retired_heap()` (the csf/007, 107 completion test: two
  cache-line invalidates and two seqno loads) ran only at renewal time, about once per frame
  in Fallout 4 and up to 128 submissions apart in light games. It now also runs after every
  submission that emitted work while retired heaps are pending; TERM still goes to the heap
  thread (224's backlog bound).
- **Saving:** 15-50 MB on average (35-60 MiB per heap, 0.3-1 heap earlier), more at peaks.
- **Risk:** low: the test and its safety argument are unchanged, only run more often.
- **Regression signal:** PanProbe tess_perf, large_draw, submit_stress; a CSG fatal 0xc0 at a
  wild sideband address would mean a heap went too early. PANVK_DEBUG=mem: `heap live` should
  drop between renewals, `term_sync` should not rise.

### 0002 Q3: tessellation arenas with an 8 MiB VS region
- **What:** the CSF tessellation arenas end their VS records at `tess_opt.vs_region_end`
  (8 MiB) instead of the gpu_prerast 32 MiB, so each arena is 72 MiB instead of 96.
  max_patches also takes the VS capacity, so patches with more than 16 KiB of VS records get
  fewer patches per chunk (more GPU loop iterations, never a failure). The lowered VS checks
  the record capacity of the arena it writes.
- **Saving:** 24 MiB per committed arena (Fallout 4 commits one at its first godrays draw).
- **Risk:** low.
- **Regression signal:** PanProbe tessellation, tess_perf, gs_tess_primitive_id,
  tess_cond_state; `prerast=72.0` instead of 96 in the PANVK_DEBUG=mem line. A layout bug
  shows as missing or corrupt tessellated geometry.
- **JM:** v9 JM computes its own layout from the 32 MiB constants, unchanged (P-D's area).

### 0003 Q4: zero only what has to be zero
- **What:** new kmod flag `PAN_KMOD_BO_FLAG_NO_ZERO` (kbase only). The 041 memset (and its
  MEM_SYNC) is skipped for VkDeviceMemory without `VK_MEMORY_ALLOCATE_ZERO_INITIALIZE_BIT_EXT`
  on v9+, the CSF device TLS and the CSF command buffer TLS (WLS) pool. kbase hands out
  zeroed pages except the context's own freed ones (cleaned from the CPU caches at free), so
  only this process's own data can show, which Vulkan allows. v6/v7 keep the clear (an
  unwritten AFBC header there is an arbitrary payload offset); everything internal keeps it.
  The null-descriptor image memory asks for ZERO_INITIALIZE explicitly.
- **Saving:** CPU, not memory: 0.2-0.4 s during the Fallout 4 load, 40-80 ms per 256 MiB
  chunk allocated in game (stutter on the allocating thread), and no CPU page-table
  population for GPU-only BOs.
- **Risk:** low-medium: an app that relies on fresh memory being zero without asking (invalid
  usage) would now see old data of its own.
- **Regression signal:** dEQP `zero_initialize_device_memory` (asks for the bit, must still
  pass), PanProbe bc_decode (zero-initialized images) and dxvk_reqs, PanPlay cubes; garbage
  in a never-written texture or buffer.
  `PANVK_KBASE_ZERO_ALL=1` for the A/B.
- **P-A 0009:** checked: the coherent-pool retry path recomputes the allocation flags from the
  same `kmod_flags`, the clear decision reads `kmod_flags` on both paths, and coherent BOs
  skip the MEM_SYNC after the clear. The two compose.

### 0004 A3: flush only the written part of CS chunks
- **What:** each CS chunk is recorded at allocation; before each `cs_end()` the chunk the
  builder ends in is trimmed to what it wrote (cache-line aligned). EndCommandBuffer cleans
  those ranges instead of every CS pool BO whole (221's transient trim never applied to the
  CS pool). Wrapped chunks are flushed whole.
- **Saving:** CPU: at least 3072 `dc cvac` lines per primary in 114 (3 x 64 KiB), now only
  written lines; with 0007 the root chunks are 4 KiB.
- **Risk:** low: only never-written lines are skipped.
- **Regression signal:** PanProbe submit_stress, vmr_secondary, csf_event, large_draw; a
  missed flush shows as a GPU fault or garbage CS instructions (CS_FATAL) on non-coherent
  parts. `PANVK_CS_FLUSH_USED=0` for the A/B.
- **P-A 0009:** with coherent pools on, `pan_kmod_queue_bo_map_sync()` returns early for the
  CS BOs, so this patch only saves the walk there.

### 0005 J1: command pool auto-trim on JM
- **What:** csf-v11 226/231 auto-trim now also runs on v6/v7/v9. The JM command buffers give
  their pool BOs back to the same free lists at reset and clear their only pointers into them
  (tls_half), so the free lists hold only BOs no pending command buffer uses.
- **Saving:** 0 on the G615; tens of MB on JM devices (each pool's free lists stayed at its
  largest command list, loading screens included).
- **Risk:** low (same trim as CSF).
- **Regression signal:** PanProbe submit_stress and the JM suites on v6/v7/v9; PANVK_DEBUG=mem
  `trim bos=` rising on JM. `PANVK_JM_CMD_POOL_AUTOTRIM=0` for the A/B.

### 0006 Q1: memory budget that shrinks under pressure
- **What:** with `PANVK_MEM_BUDGET_PRESSURE` the budget is
  `min(heap, used + 0.9 x max(0, MemAvailable - R))`, never below heapUsage, with R =
  `PANVK_MEM_BUDGET_RESERVE_MB` (256 up to 8 GiB of RAM) plus 256 MiB while SwapFree is below
  SwapTotal / 8 (one /proc/meminfo read, cached 100 ms). heapUsage is unchanged. 229
  "honest" stays opt-in (it can never lower DXVK's budget).
- **Saving:** 100-150 MB of DXVK slack (indirect). DXVK 3.1.1 re-reads the budget every
  500 ms, frees empty chunks when over it and defragments one chunk at a time when the other
  chunks have room; on UMA nothing is evicted and no allocation is refused.
- **Risk:** low-medium: defragmentation copies cost GPU time and may stutter while DXVK is
  over budget. Above 8 GiB without swap pressure nothing changes.
- **Regression signal:** DXVK HUD `memory` (allocated vs used, budget), PANVK_DEBUG=mem
  `devmem`, fps and stutter in the A/B; vulkaninfo budget values.

### 0007 B2: CS chunks that grow from 4 KiB
- **What:** `cs_builder_conf::alloc_buffer_sized(b, cookie, min_instrs)` (optional, used for
  the lazy root chunk and every next chunk; min_instrs includes the jump sequence). panvk
  starts each stream (and VMR segment) at `PANVK_CS_CHUNK_KB` and doubles up to 64 KiB, or
  bigger when a sequence needs it; chunks are suballocated from the CS pool's 64 KiB slabs.
  Other builders (Gallium panfrost, kraid, the tests, the other panvk builders) are unchanged.
- **Saving:** about 9 of the menu's 10.9 MB; 10-40 MB in game (shared with 0008).
- **Risk:** medium: cs_builder is shared code; a stream over 64 KiB costs four more JUMPs.
- **Regression signal:** PanProbe submit_stress, large_draw, vmr_secondary, csf_event,
  pipeline_stats, occlusion_query; a broken chunk link is a CS_FATAL or a hang.
  PANVK_DEBUG=mem `cmd_cs` bytes/count. `PANVK_CS_CHUNK_KB=64` for the A/B.

### 0008 B1: device-wide slab caches
- **What:** one cache per kind (CS, descriptor, TLS 64 KiB slabs) for the device. After a
  flags = 0 vkResetCommandPool a pool keeps 2 free slabs per kind and hands the rest to the
  caches (vkDestroyCommandPool hands them all); a recording takes from its pool's list, then
  the cache (one mutex per slab), then allocates. Each cache gives out its newest slab and,
  at the first hand-off after each `PANVK_CMD_SLAB_CACHE_MS` window, frees its oldest slabs
  down to a quarter of the window's swing (at least 2): those slabs were not used during the
  whole window. 231 still trims the other lists (big TLS BOs, JM). RELEASE_RESOURCES and
  vkTrimCommandPool free the pool's own lists as before.
- **Saving:** 40-100 MB (review; shared with 0007): the sum of ~20 per-pool peaks becomes
  one device-wide peak plus 25 %.
- **Risk:** medium: slabs now cross pools (a stale reference to a reset command buffer's
  memory would now hit another pool's recording), and one lock per slab on the allocation
  path.
- **Regression signal:** PanProbe submit_stress, vmr_secondary, csf_event, large_draw,
  occlusion_query; PanPlay cubes (DXVK recycles its command pools). PANVK_DEBUG=mem:
  `slab_cache=<MiB>/<slabs> hits=<n> freed=<slabs> <MiB>`; `cmd_cs` + `cmd_desc` should
  fall well below the 231 level and stay flat. `PANVK_CMD_SLAB_CACHE=0` for the A/B.

### 0009 B4 (opt-in): evictable slabs
- **What:** `kbase_kmod_bo_set_evictable()`: FLAGS_CHANGE(BASE_MEM_DONT_NEED) to mark;
  to unmark, clear the flag (its result is EINVAL on native regions in the kbase sources, so
  it is ignored) and then MEM_COMMIT every page, which kbase refuses for DONT_NEED regions:
  only a successful commit makes the BO usable again. With `PANVK_KBASE_EVICTABLE=N` the
  slabs 0008's window trim would free are marked and kept on an evict list for N more
  windows; a take uses them only when the cache is otherwise empty, and frees one that does
  not verify. The first refusal to mark turns it off for the cache (one warning). Off with
  pandecode.
- **Saving:** 0 by default; 30-100 MB under pressure if enabled (review, overlaps 0008).
- **Risk:** high until tested: the clear semantics come from kbase sources and are untested
  on MTK kbase 1.21. Needed before anyone enables it: a PanProbe check that marks slabs,
  applies memory pressure, takes them back, writes them from the CPU and reads them on the
  GPU.
- **Regression signal:** PANVK_DEBUG=mem `evictable=<MiB>/<slabs> reused=<n> lost=<n>`
  (`lost` = pages could not be brought back); SIGBUS on the recording thread or a GPU fault
  in a command buffer would mean a slab was used while still evictable.

### 0010 B6 (opt-in): PANVK_KBASE_HEAP_SLOTS
- **What:** the 107 descriptor-slot count is read per queue (2-4, default 4): at most
  slots - 1 retired heaps. Rotation and completion test unchanged.
- **Saving:** 0 by default; about one heap (35-60 MiB) at peaks with 3.
- **Risk:** high: a renewal is skipped while every retired slot is in use, and the current
  heap then keeps growing; 107 measured submit stalls with 1 retired heap.
- **Regression signal:** PANVK_DEBUG=mem `heap live`, `renew`; fps and stalls in NFS Most
  Wanted and Fallout 4; DEVICE_LOST.

## Not done, and why

- **Q5** (finish only the subqueues a command buffer used): not in P-B's list; it changes
  EndCommandBuffer sync points and query barriers. 0007 already takes most of its memory
  (root chunks are 4 KiB).
- **Q6** extras (per-kind in-use vs free bytes, CS bytes written, TLS culprit, tessellation
  high-water): only the new caches are accounted (0008, 0009).
- **Q7**, **B3**, **B5**, **B7**, **B8**: not in P-B's list.
- **B4 phase 2:** idle tessellation arenas and the device TLS. The TLS backing is aliased
  (csf-v11 230), which kbase refuses for DONT_NEED; arenas need tracking of the submissions
  that use them before they can be marked, and a clear at record time.
- **B6 (b), (c):** a lower max_chunks on 8 GB parts (earlier incremental rendering, costs
  fps) and chunk recycling with correct heap statistics (the real fix for 098's DEVICE_LOST).
- 229 "honest" stays opt-in, as the review recommends.

## Notes for the other packages

- **P-A:** 0003 composes with 0009 (see above); 0004 is mostly moot where 0009 holds. The
  rebase onto `p115-pa` was clean. In `csf/panvk_vX_cmd_draw.c` only 0002 changes anything,
  and only the gpu_prerast tessellation layout (VS record capacity of the arena, max_patches);
  0003, 0004 and 0007 change TLS BO flags, CS chunk allocation and the EndCommandBuffer
  flush in `csf/panvk_vX_cmd_buffer.c`. Nothing in descriptor dirty tracking, the depth
  bounds FS, KCPU waits or cache-maintenance deferral.
- **P-C:** nothing in its areas (present/WSI, fb preload, barriers, CRC, flush IDs).
- **P-D:** 0005 turns 226/231 auto-trim on for the JM pools (shared `panvk_cmd_pool.c`; the
  knob is `PANVK_JM_CMD_POOL_AUTOTRIM`): the first half of D2's quick part. Its second half,
  no CPU clear for the JM TLS/WLS pool (`jm/panvk_vX_cmd_buffer.c`), can now use 0003's
  `PAN_KMOD_BO_FLAG_NO_ZERO` (kbase only, masked elsewhere by
  `panvk_device_adjust_bo_flags()`) as the CSF TLS pool does; it was left to P-D because it
  is JM TLS. JM arenas still get the 041 clear, and v9 JM could use `tess_opt.vs_region_end`
  for its tessellation layout (Q3, J2).
  The slab caches (0008) are attached only on v10+; JM pools could attach them in
  `panvk_CreateCommandPool()` once the JM command buffers are checked for references to
  pool slabs after reset.
