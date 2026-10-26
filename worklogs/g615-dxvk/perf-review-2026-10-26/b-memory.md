# Perf review B (2026-10-26): memory footprint and allocation efficiency

Scope: memory the driver allocates on kbase/Android. The focus is CSF v10/v11 (Mali-G615 MC6 in the Poco X6 Pro: 8 GB shared, Fallout 4 under DXVK 3.1.1, memory-bound), with notes on JM (v6/v7/v9). This was a read-only review of `/home/flux/src/mesa-rc5g` at tag `rc5g-114` (HEAD `d324a269`). Every `file:line` below is in that worktree, under `src/` unless a path says otherwise. Patch attribution comes from `git blame`/`git log` in that tree, where each series commit is `series: <dir>/<NNN>-...`. "Upstream" means the Mesa base `5a07217f`.

Data used: the only Fallout 4 memory log from the G615 is RC5-universal-112: `device-runs/2026-10-24-g615-rc5u-112/raw/fallout4/` (`panvk-mem-27889.log`, `mon/all.txt`, `mesa-fo4.log`), plus the DXVK HUD in screenshot fo4-05. Fallout 4 was never run with 113 (230, 231) or 114 (232), so all 113/114 numbers below are projections.

Not verified here:
- DXVK internals. There is no DXVK source on this machine. The DXVK option names quoted below were checked with `strings` on `system32/d3d11.dll` from the bundled `/var/tmp/panvk/components/dxvk-3.1.1-4-arm64ec.wcp`. They exist: `dxvk.enableMemoryDefrag`, `dxvk.maxMemoryBudget`, `dxvk.tilerMode`, `dxgi.maxDeviceMemory`, `dxgi.maxFrameLatency`. `dxvk.maxChunkSize` is gone in 3.1.1. What DXVK does internally comes from knowledge of DXVK 2.5+ and is marked as such.
- kbase kernel behaviour. It comes from knowledge of the Arm kbase source. MediaTek's tree is not available here.
- Fallout 4 INI keys.

## 0. Where the memory goes (Fallout 4, G615, 112, in game, t = 341-402 s)

| item | MiB (112) | source | 114 projection |
|---|---|---|---|
| kbase pages of the Fallout4.exe context | 2515 (t=371), 2652 (t=402) | `mon/all.txt`: kctx 643,938 and 678,791 pages | |
| VkDeviceMemory (`devmem`) | 1516-1772 | panvk-mem | same |
| ... of which DXVK allocated / used | 1744 / 1437, so **307 MB of DXVK slack** (type 0: 1504 / 1327 in 8 chunks; type 1: 112 / 35 in 5 chunks; about 128 dedicated) | HUD fo4-05, `Vidmem heap 0: 1744 MB (89%) 1437 MB used` | same |
| driver private BOs (`priv`) | 581-611 | panvk-mem | about 340-370 |
| ... `dev_tls` | 336 = 21 core IDs x 2048 threads x 8 KiB | panvk-mem | about 96 (230: 6 present cores x 16 MiB) |
| ... `cmd_cs` | 87.5-110.3 (1400-1765 BOs of 64 KiB) | panvk-mem | at least 112 (231 keeps a windowed high-water) |
| ... `cmd_desc` | 37.1-44.8 (594-716 BOs of 64 KiB) | panvk-mem | at least 112 |
| ... `prerast` (tessellation arena) | 96 | panvk-mem, `gpu_prerast tessellation arena committed (96 MiB)` | 96 |
| ... `cmd_tls` (only WLS uses it on CSF) | 13.5-13.8 | panvk-mem | same |
| ... shader, desc_pool, dev_pool, misc, utrace, bc_raw | 7.0, 1.5, 1.2, 1.1, 0.1, 0 | panvk-mem | same |
| not seen by panvk: tiler heaps (up to 5 live, 1 MiB chunks) + kbase CSF internals | 163 (t=371) - 285 (t=402) | kctx - devmem - priv | same |
| **driver-side total** (priv + heaps) | **about 745-895** | | **about 500-650** |

System state at t=402: MemAvailable 365 MB, SwapFree **23 MB** (2.81 GB at the start), VmSwap 2.33 GB, RssAnon 291 MB, RssFile 2.70 GB. The RssFile figure is the kbase mappings: they are shared mappings of the device file.

Why every MiB counts:
- Every kbase BO is committed in full when it is allocated: `commit_pages = va_pages` (`panfrost/lib/kmod/kbase_kmod.c:2525`, kbase-common), unless it has ALLOC_ON_FAULT.
- Every BO is CPU-mapped for its whole lifetime (`kbase_kmod.c:2680`), because SAME_VA needs the mmap.
- Every BO is zeroed when it is allocated (`kbase_kmod.c:2697-2711`, csf-v11 041).
- kbase pages are not on the LRU and are never swapped. So the kernel makes room by compressing Fallout4.exe's own anonymous memory into zram. By the end of the 112 run, 2.33 GB of it was in swap and zram was full. The next steps are reclaim stalls or an LMK kill.

Memory that the driver holds but is not using is the cheapest to give back: free lists, completed retired heaps, worst-case arena space and DXVK's slack.

## 1. Ranked findings

The ranking is (expected MiB x confidence) / risk. **Quick** means small, contained, guarded by a knob, and fit for the next RC. **Project** means design work or a change in shared code. Savings are for Fallout 4 on the G615, measured against 112. Findings Q5, B1, B2 and B4 all draw on the same 125-155 MiB of cmd_cs + cmd_desc, so together they cannot save more than about 100-120 MiB.

| # | id | finding | class | saving (FO4, G615) | risk | confidence |
|---|---|---|---|---|---|---|
| 1 | Q1 | VK_EXT_memory_budget that reacts to memory pressure (and 229 honest is not a default) | quick | 100-150 MB of DXVK slack (indirect) | low-medium | medium-low |
| 2 | P1 | Fallout 4 profile: godrays off/low, iTexMipMapSkip, dxvk.maxMemoryBudget | quick (profile) | 96 MiB arena; 0.6-1.0 GB textures | none (image quality) | high (arena) / medium |
| 3 | B1 | one device-wide cache for command CS/desc slabs instead of per-pool free lists | project | 40-100 MB | medium | medium-low |
| 4 | B3 | keep the device TLS stack at 4 KiB/thread or less for Fallout 4's worst shader | project | 48 MiB (114: 96 to 48) | medium | medium |
| 5 | Q2 | destroy completed retired tiler heaps at every graphics submit | quick | 15-50 MB | low | low |
| 6 | Q3 | tessellation arena: 8 MiB VS region instead of 32 MiB | quick | 24 MiB | low | high |
| 7 | B2 | CS chunks that grow (4 KiB first, then up to 64 KiB) via a size hint in cs_builder | project | 10-40 MB (menu: 9) | medium | medium-low |
| 8 | B4 | kbase evictable memory (BASE_MEM_DONT_NEED) for idle driver caches | project | 30-100 MB under pressure | medium-high | low-medium |
| 9 | B6 | fewer live tiler heaps, or chunk recycling again | project | 50-150 MB | high | low |
| 10 | Q4 | zero only what has to be zero (the 041 memset) | quick | 0 MiB; 0.2-0.4 s of CPU at load, 40-80 ms per 256 MiB chunk | low-medium | high (cost) |
| 11 | Q5 | finish only the subqueues a command buffer used | quick | 4-8 MB | low-medium | medium |
| 12 | Q6 | PANVK_DEBUG=mem: in-use vs free bytes, CS bytes, TLS culprit, tess high-water | quick | 0 (needed to size B1-B3, B5, B6) | none | high |
| 13 | Q7 | commit the second tessellation arena only when memory allows | quick | 0 in this run, 96 MiB latent | low | high |
| 14 | B5 | tessellation arena sized at run time, or grown explicitly | project | 24-64 MiB beyond Q3 | medium | medium |
| 15 | B7 | WLS is allocated per dispatch, sized for 21 core IDs, never reused | project | 5-10 MB | medium | medium |
| 16 | B8 | the VS driver set always holds 16 attribute descriptors | project | 5-8 MB | medium | low-medium |
| J1 | JM | command pool auto-trim is v10+ only | quick | (JM devices) tens of MB | low | medium |
| J2 | JM | v9 JM commits 64 + 96 MiB of arenas in every DXVK VkDevice | quick-medium | (v9) 64-160 MiB | medium | high |

## 2. Quick safe wins

### Q1. VK_EXT_memory_budget that reacts to memory pressure (do not make 229 "honest" the default)

- **Location:** `panfrost/vulkan/panvk_physical_device.c:1741-1797`.
  - The default (upstream) sets `heapUsage = heap_used` (VkDeviceMemory only) and `heapBudget = vk_physical_device_heap_budget(MemAvailable, 0.9, heap, used)` (lines 1754-1760), which is `min(heap, used + 0.9 x MemAvailable)` (`vulkan/runtime/vk_physical_device.h:169`).
  - The csf-v11 229 block is at lines 1762-1792.
  - `panvk_available_system_memory()` (lines 1701-1716) reads only MemAvailable, cached for 100 ms.
  - The heap is 75 % of MemTotal = 5498 MiB (`get_device_heaps()` at line 509, the `os_misc.h` heuristic).
- **What is wasteful:** The budget follows whatever is already allocated. On Android, zram and LMK hold MemAvailable at 200-400 MB until zram is full. So DXVK always sees 0.2-0.4 GB of headroom (89 % on fo4-05) and keeps 307 MB of allocated but unused chunk memory. Meanwhile the kernel is already swapping the game out (SwapFree 23 MB).
- **Fix:** add `PANVK_MEM_BUDGET_PRESSURE` (default 1 after the A/B; 0 restores today's formula):
  - `heapBudget = min(heap, used + 0.9 x max(0, MemAvailable - R))`.
  - R = `PANVK_MEM_BUDGET_RESERVE_MB` (default 256 when MemTotal is 8 GiB or less, else 0), plus 256 MiB while SwapFree is below SwapTotal / 8 (read in the same `/proc/meminfo` pass).
  - Keep `heapUsage = heap_used`, and never report a budget below `used`.

  In 112's end state (MemAvailable 365 MB, SwapFree 23 MB) the budget would equal `used`, so DXVK would sit at about 100 %. From DXVK knowledge, DXVK then frees empty chunks and defragments. `dxvk.enableMemoryDefrag` exists in 3.1.1 and defaults to Auto.
- **Saving:** DXVK's slack is 307 MB. The 111 run (fo4-memory-analysis, section 0) was at 95-103 % of budget and had only about 151 MB of slack (1600 MB allocated, 1449 MB used). That is a different scene and weak evidence, so expect 100-150 MB. The effect is indirect: DXVK decides what to release.
- **Risk:** low to medium. While DXVK is over budget, its defragmentation copies cost GPU time and can stutter. Engines that read DXGI QueryVideoMemoryInfo would lower their quality, which is what we want here. Fallout 4 does not read it (D3D11, Windows 7 era).
- **Measure:** A/B with the knob, same save. Read the DXVK HUD `memory` lines (heap allocated vs used, per-type chunk totals), panvk-mem `devmem`, kctx pages from `/proc/mtk_mali/gpu_memory`, SwapFree and fps.
- **Confidence:** medium-low, because DXVK's reaction is inferred. See section 4 for why 229 "honest" cannot do this.

### P1. Fallout 4 profile (no driver change)

- **Location:** `apps/panvk-launcher/app/src/main/java/dev/zenithblue/panvklauncher/Containers.kt:797-801`. Fallout 4 already gets `dxgi.maxDeviceMemory = 2048; dxgi.maxSharedMemory = 1024; dxvk.trackPipelineLifetime = False; dxvk.numCompilerThreads = 2`. Game settings live in Fallout4Prefs.ini.
- **(a) Godrays.** Fallout 4's volumetric lighting (NVIDIA GameWorks) is the game's tessellation user. The tessellation arena is committed at the first tessellation draw (205, `panvk_vX_device.c:183-205`) and stays for the life of the VkDevice. `[Display] bVolumetricLightingEnable=0` (key from game-config knowledge) means the 96 MiB arena is never committed. It also removes the tessellated light-volume geometry from the tiler heaps and the GPU tessellation loop from every frame (fps too). To check: the info line `gpu_prerast tessellation arena committed` disappears and `prerast=0.0`.
- **(b) `iTexMipMapSkip=1`.** This is fo4-memory-analysis fix 2, still open: -0.6 to -1.0 GB of DXVK memory.
- **(c) `dxvk.maxMemoryBudget = 1536`.** The option exists in 3.1.1. From upstream docs it caps DXVK's own budget in MiB. With that cap, DXVK compacts under its own limit whatever the driver reports. A/B it against Q1. Do not stack both blindly.
- **(d) `dxgi.maxFrameLatency = 1`.** The option exists. It means fewer frames in flight, so fewer live DXVK command lists and retired tiler heaps. It can cost fps.
- **Risk:** image quality and possibly fps.
- **Confidence:** high for (a)'s 96 MiB (205's lazy commit), medium for (b) to (d).

### Q2. Destroy completed retired tiler heaps at every graphics submit

- **Location:** `panfrost/vulkan/csf/panvk_vX_gpu_queue.c:2507-2545`, `kbase_try_destroy_retired_heap()` (csf/007, 107).
  - It is only called from `kbase_renew_tiler_heap()` (line 2562).
  - That only runs when a renewal is due (lines 3212-3242: every 128 submits or 65536 units of tiler work). In Fallout 4 that is about once per frame: 4680 renewals, and 158 between t=361 and t=402.
  - `panfrost/vulkan/csf/panvk_queue.h:155` sets 4 slots (107). Current + 3 retired + the spare is the `heap live=5` in the log.
- **What is wasteful:** A retired heap whose work has finished keeps every chunk it grew until the next renewal, which can be about one frame (260 ms at 3.8 fps). 098 disabled chunk recycling, so a heap holds the geometry of everything it served.
- **Fix:** run the same completion test (two cache-line invalidates and two loads) at every graphics submit, and on the heap thread when it is idle. The TERM itself already goes to the heap thread, with the backlog bounded by 224. The test is the one that prevents the 0xc0 fault described at lines 2499-2505, so the safety argument does not change. Knob: `PANVK_KBASE_HEAP_EAGER_TERM=0` restores the current behaviour.
- **Saving:** heaps plus kbase internals are 163-285 MiB for up to 5 heaps, about 35-60 MiB per heap. On average 0.3-1 heap goes earlier, so 15-50 MB, and more at peaks. Peaks are what trigger LMK.
- **Risk:** low.
- **Confidence:** low, because nobody measures bytes per heap (see Q6e).
- **Measure:** kctx - devmem - priv over time, `heap live`. Correctness: tess_perf, large_draw and submit_stress; renewal stalls in NFS Most Wanted (the scenario behind 107).

### Q3. Tessellation arena: 8 MiB VS region instead of 32 MiB

- **Location:** `panfrost/vulkan/panvk_gpu_prerast.h`:
  - line 20: `PANVK_GPU_PRERAST_VS_REGION_END` 32 MiB (csf-v11 042)
  - lines 310-318: record capacity
  - lines 324-325: `TESS_ARENA_SIZE` 96 MiB, with `TESS_DATA_OFFSET = VS_REGION_END` (csf-v11 065)
  - lines 353-362: `max_patches = min(tess data / patch bytes, VS capacity / patch_size)`
  - lines 365-385: layout

  Users: `csf/panvk_vX_cmd_draw.c:5028`, and `jm/panvk_vX_cmd_draw.c:6258-6290` (v9 JM).
- **What is wasteful:** The 64 MiB data region holds about 490 patches at the maximum level of 64. Per patch: 4225 points x 8 B + 24576 indices x 4 B, about 132 KiB, plus the TCS outputs. The VS region only has to hold 490 x patch_size x VS record stride:
  - about 0.25 MiB for 4 control points x 128 B;
  - 8 MiB only for 32-control-point patches with 512 B records.

  Yet all 32 MiB are committed (and zeroed by 041).
- **Fix:** tessellation-only layout constants: VS region 8 MiB, data offset 8 MiB, arena 72 MiB (the same for arena2).
  - The tessellation bound on max_patches stays the same.
  - Only patches with patch_size x stride above 16 KiB get fewer patches per chunk. That means more GPU loop iterations, not failures.
  - The GPU takes every address from the layout, so this is a CPU-side change.
  - Knob: `PANVK_TESS_VS_REGION_MB` (default 8; 32 is the old layout).
- **Saving:** 24 MiB per committed tessellation arena (48 MiB if arena2 is committed).
- **Risk:** low. Check the nested path (`prerast_arena` next to `csf/panvk_vX_cmd_draw.c:5025`) and the v9 JM layout. Tests: tess_perf hashes, tessellation, gs_tess_primitive_id.
- **Confidence:** high.

### Q4. Zero only what has to be zero (the 041 memset)

- **Location:** `kbase_kmod.c:2697-2711` (csf-v11 041, "zero BO pages at allocation"). It memsets every committed page and calls MEM_SYNC when the mapping is CACHED_CPU. It runs for every BO, NO_MMAP ones included, because all of them are mapped (line 2680).
  - VK_EXT_zero_initialize_device_memory is exposed (`panvk_vX_physical_device.c:242`, `:791`).
  - `panvk_AllocateMemory()` (`panvk_device_memory.c`) does not look at `VK_MEMORY_ALLOCATE_ZERO_INITIALIZE_BIT_EXT`, because the kmod zeroes everything anyway.
- **What is wasteful:** 041 was needed for dEQP zero_initialize. kbase reuses a context's own freed pages without zeroing them; pages from the device pool or the kernel arrive zeroed (kbase knowledge). The zeroing costs CPU, not footprint, because MEM_ALLOC commits the pages anyway:
  - The Fallout 4 load grew devmem from 716 to 1772 MiB. That is about 1 GiB of memset, mostly through write-combined mappings for memory type 0.
  - Each DXVK chunk allocated mid-game (up to 256 MiB) costs a 40-80 ms memset plus sync on the allocating thread.
  - 112's 336 MiB device TLS and the 96 MiB arena are zeroed in the same way.
  - The memset also faults in CPU PTEs for every GPU-only BO (about 5 MiB of page tables for 2.6 GB).
- **Fix:** add a kmod flag (for example `PAN_KMOD_BO_FLAG_NO_ZERO`).
  - Use it for VkDeviceMemory unless `ZERO_INITIALIZE_BIT_EXT` is set.
  - Use it for the GPU-only scratch BOs: dev_tls, the WLS in cmd_tls, and the tessellation arena data past its header (the header is cleared explicitly at `panvk_vX_device.c:176-178`).
  - Keep 041 for every other private BO until each kind is audited (CS/desc slabs and query pools may assume zero).
  - Knob: `PANVK_KBASE_ZERO_ALL=1` restores 041 for everything.
- **Saving:** 0 MiB of footprint. Roughly 0.2-0.4 s less CPU during the Fallout 4 load, and 40-80 ms less stutter per mid-game chunk.
- **Risk:** low to medium. Stale data can only come from the same process, which Vulkan allows; this depends on kbase zeroing device-pool pages (check on MTK). dEQP-VK.memory.zero_initialize_device_memory.* and memory.* must still pass.
- **Confidence:** high on the cost, medium on the safety.

### Q5. Finish only the subqueues a command buffer used

- **Location:** `csf/panvk_vX_cmd_buffer.c:617-651`, `EndCommandBuffer` (all of this code is upstream):
  - It calls `finish_queries()` for every primary (lines 630-631). That function puts a barrier on all subqueues (lines 572-615).
  - It calls `finish_cs()` on all 3 subqueues (lines 637-646). `finish_cs()` (lines 474-560) always emits slot waits, the error store and a cache CLEAN.
  - `alloc_cs_buffer()` (lines 1149-1162) hands out one 64 KiB chunk, which is one 64 KiB slab (pool properties at lines 1385-1392).
  - Submit already skips empty streams (`panvk_vX_gpu_queue.c:2685`, `:2911`), but after `finish_cs()` no stream is ever empty.
- **What is wasteful:** every primary costs at least 3 x 64 KiB of CS and three cache cleans, even when only one subqueue has work (upload and init command buffers). In the menu at 65 fps, 175 CS BOs (10.9 MB) is what about 58 command buffers x 3 root chunks gives. The menu records very little, so most of that is minimum-chunk waste.
- **Fix:** track per subqueue whether anything was emitted, and skip `finish_cs()` for untouched ones.
  - Run `finish_queries()` only when `contains_timestamp_queries` is set. Today only `panvk_vX_cmd_query.c:592` sets it, so it must also be set from ExecuteCommands for secondaries that contain timestamp queries.
  - Treat a subqueue as used whenever `flush_sync_points()` (lines 445-470) has to update its progress seqnos.
  - Knob: `PANVK_CS_SKIP_IDLE_SUBQUEUES=0`.
- **Saving:** 4-8 MB (about 7 of the menu's 10.9 MB), plus fewer queue kicks and cache cleans per submit. Those are CPU/GPU costs and belong to reviewers A and C.
- **Risk:** low to medium (sync-point and error semantics). Tests: submit_stress, csf_event, occlusion_query, pipeline_stats, vmr_secondary.
- **Confidence:** medium.

### Q6. PANVK_DEBUG=mem additions (sizing data for everything else)

- **Location:** `panvk_priv_bo.c:59-100` (223 kinds), 230's TLS info line (`csf/panvk_vX_cmd_buffer.c`, `dev_tls_create()`), and the panvk-mem log line.
- **Add:**
  - (a) Per kind: bytes in pool free lists vs bytes attached to command buffers. This sizes B1 and B4.
  - (b) At EndCommandBuffer: CS bytes and chunk count per subqueue, and descriptor bytes, as a histogram per log period. This sizes Q5 and B2. 1400-1765 CS chunks over about 600 draws per submission would mean about 7-9 KB of CS per draw if it were all content (assuming about 20 DXVK command lists that each hold one submission). The free lists and minimum chunks are the more likely explanation, but nothing measures it.
  - (c) When the device TLS grows: the hash, stage, `tls_size` and spill bytes of the shader that caused it (for B3).
  - (d) Tessellation: the most patches per chunk and the highest tessellation level seen (for Q3 and B5).
  - (e) Heaps: per-heap growth if the heap context exposes it. If not, keep `kctx - devmem - priv` from `/proc/mtk_mali/gpu_memory` in the monitor script (adb only today).
- **Saving:** 0 MiB. **Risk:** none.

### Q7. Commit the second tessellation arena only when memory allows

- **Location:** `panvk_vX_device.c:208-226` (215). The second arena is committed at the second tessellation draw of a command buffer (`csf/panvk_vX_cmd_draw.c:5014`: odd tessellation draws use arena2). `panvk_vX_device.c:877-878` sets `PANVK_TESS_ARENAS` to 2 by default.
- **What is wasteful:** in 112, Fallout 4 never had two tessellation draws in one command buffer (prerast stayed at 96). Any scene or game that does commits another 96 MiB (72 MiB with Q3) for life. It only buys overlap between the compute and vertex/tiler queues.
- **Fix:** commit arena2 only when MemTotal is above 8 GiB, or MemAvailable is above 1 GiB at commit time; otherwise use one arena. `PANVK_TESS_ARENAS=2` forces the old behaviour.
- **Saving:** 0 in this run; 96 MiB latent.
- **Risk:** low (215 is a perf optimisation).
- **Confidence:** high.

## 3. Bigger projects

### B1. One device-wide cache for command CS/desc slabs

- **Location:**
  - Per-VkCommandPool BO pools: `csf/panvk_vX_cmd_buffer.c:1385-1418` (CS/desc/TLS, 64 KiB slabs).
  - `panvk_mempool.c:38-115`: `panvk_pool_alloc_backing()` takes slab-sized BOs from the pool's free list.
  - `panvk_mempool.c:217-232`: `panvk_pool_reset()` splices them back.
  - `panvk_cmd_pool.c:219-238` (226, 231): each free list is trimmed to `max(current window, previous window) + 25 %`, at least 2.
- **What is wasteful:** From DXVK knowledge, DXVK has one VkCommandPool per command list (a recycled set of about 16, plus those in flight) and resets it with flags = 0 after the GPU finishes.
  - Every pool keeps free slabs for its largest recording in its last 64-128 resets, plus 25 %. At about 11 submits/s spread over about 20 lists, that is minutes.
  - So the footprint is the sum of the per-pool high-waters, although only one list records at a time.
  - In 112, cmd_cs + cmd_desc were 125-155 MiB (2000-2480 slabs). 231 (113+) deliberately keeps more than 112's 226.
- **Fix:** a device-level slab cache under a mutex for the 64 KiB CS/desc slabs, and the small TLS slabs:
  - After a reset, a pool keeps at most a few slabs (for example 4) and gives the rest to the device cache.
  - The cache trims to its own windowed high-water of concurrent demand + 25 %, freeing the oldest first. anv's device `batch_bo_pool` works the same way.
  - Cache coherency is the same as reuse within a pool: 221 `flush_maps` for CPU writes, `finish_cs` CLEAN for GPU-dirty lines.
  - Knob: `PANVK_CMD_SLAB_CACHE=0`.
- **Saving:** 40-100 MB, if about 60-75 % of slabs sit idle in recycled pools. Q6a gives the real share.
- **Risk:** medium: one lock per 64 KiB slab on the allocation path, and slabs now cross pools. Tests: submit_stress, plus Fallout 4 trim counters.
- **Confidence:** medium-low.

### B2. CS chunks that grow (size hint in cs_builder)

- **Location:**
  - `alloc_cs_buffer()` at `csf/panvk_vX_cmd_buffer.c:1149-1162`: fixed 64 KiB capacity (upstream).
  - `panfrost/genxml/cs_builder.h:106`: `alloc_buffer(cookie)` has no size parameter.
  - `cs_builder.h:541-551`: lazy root chunk, and an assert that each instruction sequence fits in one chunk. Release builds do not check it.
  - `cs_builder.h:553-600`: chunk switch.
- **What is wasteful:** every chunk is a 64 KiB slab, even for a stream of 1-2 KiB. After Q5, only touched subqueues pay for a chunk, but each still pays 64 KiB.
- **Fix:**
  - Add `alloc_buffer2(cookie, min_instrs)` (or a capacity hint) to `cs_builder_conf`, so a sequence never gets a chunk smaller than itself.
  - In panvk, start with a 4 KiB root chunk and double up to 64 KiB. The pool already suballocates small chunks out of 64 KiB slabs (alignment 64).
  - Knob: `PANVK_CS_CHUNK_KB=64` restores today's behaviour.
- **Saving:** about 9 of the menu's 10.9 MB; 10-40 MB in game, depending on what Q6b finds.
- **Risk:** medium: cs_builder is shared code (Gallium panfrost uses it too), and there are more JUMPs.
- **Confidence:** medium-low.

### B3. Keep the device TLS stack at 4 KiB/thread or less

- **Location:**
  - `csf/panvk_vX_cmd_buffer.c:375-415`: `emit_tls()` calls `pan_get_total_stack_size()` (`panfrost/lib/pan_scratch.c:57-62`, per-thread size rounded up to a power of two, a hardware encoding requirement) x THREAD_TLS_ALLOC x core-ID range.
  - 230's sparse backing is at lines 129-300.
  - Compiler: `panfrost/compiler/bifrost/bifrost_nir.c:1160-1190` sends indirectly indexed function_temp arrays above 8 elements to scratch; line 1654 sets `tls_size = scratch_size`; `bi_ra.c:2015` adds spills.
- **What is wasteful:** 336 MiB = 21 x 2048 x 8 KiB, so some Fallout 4 shader needs between 4 and 8 KiB per thread. The libpan kernels stay at 4 KiB or less (219). With 230, the device TLS is 6 x 16 = 96 MiB backed; at 4 KiB it would be 48 MiB. The power-of-two rounding means a shader needing 4.1 KiB costs 8 KiB.
- **Fix:** first name the shader with Q6c. Then fix by cause:
  - DXBC indexable temps that became NIR arrays: raise the if/else-tree threshold for small read-mostly arrays, use `nir_opt_large_constants` for constant-initialised arrays, or 16-bit elements where precision allows.
  - Spills: work on register pressure.
  - Give each fix its own knob.
- **Saving:** 48 MiB on 114.
- **Risk:** medium (compiler changes).
- **Confidence:** medium. The 8 KiB bucket is certain; which shader puts it there is not.

### B4. kbase evictable memory for idle driver caches

- **Location:** the uAPI is there but nothing uses it.
  - `panfrost/lib/kmod/mali_base_common_kernel.h:117`: `BASE_MEM_DONT_NEED`.
  - `mali_base_kernel.h:75-77`: `BASE_MEM_FLAGS_MODIFIABLE` includes DONT_NEED.
  - `kbase_uapi.h:376`: `KBASE_IOCTL_MEM_COMMIT`.
  - `kbase_uapi.h:449`: `KBASE_IOCTL_MEM_FLAGS_CHANGE`.
  - `git grep` finds only these definitions.
- **What is wasteful:** all idle driver memory is pinned: free-list slabs, an idle tessellation arena, device TLS between uses. From kbase knowledge, FLAGS_CHANGE(DONT_NEED) puts a native region on the context's evict list, and kbase's shrinker frees its pages under memory pressure, which the X6 is under all the time in Fallout 4. Clearing the flag makes the region needed again; the backing is re-allocated if it was evicted, and the contents are lost.
- **Fix:**
  - Mark slabs DONT_NEED when they enter a free list beyond a small reserve, and clear the flag when they are taken. That is 2 ioctls per slab transition; batch them at reset.
  - Mark arenas and TLS after N seconds idle, as 230's shrink does, and clear the flag at record time before the next use. Recording happens before submit.
  - Knob: `PANVK_KBASE_EVICTABLE=0`.
- **Saving:** 30-100 MB under pressure. This overlaps B1; B1 is deterministic, B4 adapts to the kernel's pressure.
- **Risk:** medium-high.
  - A region must never be DONT_NEED while in-flight GPU work references it.
  - CPU access to an evicted region faults (SIGBUS), so every CPU write must come after the flag is cleared.
  - kbase refuses aliased or imported regions. 230's TLS backing is aliased, so it cannot be made evictable while its alias exists.
  - Behaviour on MTK uAPI 1.21 is untested. Build a PanProbe-style unit test first.
- **Confidence:** low-medium.

### B5. Tessellation arena sized at run time, or grown explicitly (beyond Q3)

- **Location:**
  - `panvk_gpu_prerast.h:329-348`: the data region assumes level 64 for every patch.
  - `panvk_vX_device.c:165-180`: the arena is WB, committed and zeroed.
  - `kbase_kmod.c:2528-2537`: the ALLOC_ON_FAULT path.
- **What is wasteful:** draws with few patches, or low tessellation factors, never touch most of the 64 MiB data region.
- **Fix:**
  - (a) Reserve 96 MiB of VA and commit, with `KBASE_IOCTL_MEM_COMMIT`, only the header + VS region + max_patches x patch bytes of the largest draw recorded so far. Grow on the CPU at record time, which is always before submit.
  - (b) Clamp the per-patch reservation to the device's actual maximum level when it is known.
  - Do not use GROW_ON_GPF. The comment at `panvk_vX_device.c:142-146` records lost gpu_prerast output with GPU-fault growth on MTK kbase CSF 1.21.
  - Knob: `PANVK_TESS_ARENA_COMMIT=0`.
- **Saving:** up to 64 MiB for apps with small tessellation draws. For Fallout 4 it is unknown until Q6d.
- **Risk:** medium (tess_perf and gs_tess_primitive_id cover it).
- **Confidence:** medium.

### B6. Tiler heaps: fewer live heaps, or chunk recycling again

- **Location:**
  - `csf/panvk_vX_gpu_queue.c:2111-2150` (224): 4 initial / 400 maximum chunks of 1 MiB.
  - `csf/panvk_vX_gpu_queue.c:2384-2402` (csf/007): heap init with `target_in_flight` 65535.
  - `panvk_queue.h:155` (107): 4 slots.
  - csf-v11 098: only VERTEX_TILER_STARTED heap ops, so no chunk recycling between renewals. Recycling had caused "Invalid Heap statistics" DEVICE_LOST in NFS Most Wanted.
- **What is wasteful:** each heap grows by the geometry of what it served and lives 1-3 frames longer. That is 163-285 MiB that panvk does not see.
- **Options:**
  - (a) A run-time `PANVK_KBASE_HEAP_SLOTS` (3 on 8 GB parts) keeps one fewer retired heap. 107 measured submit stalls with only 1 retired heap; 2 may be enough.
  - (b) A lower per-heap max_chunks on 8 GB parts: incremental rendering starts earlier, which costs perf.
  - (c) Turn recycling back on with correct heap statistics, so kbase's frag_end > vt_end check passes. This is the real fix.
- **Saving:** 50-150 MB.
- **Risk:** high.
- **Confidence:** low.
- **Measure:** kctx - devmem - priv, renewal counts, DEVICE_LOST in NFS Most Wanted and Fallout 4.

### B7. WLS: per dispatch, sized for 21 core IDs, never reused

- **Location:** `csf/panvk_vX_cmd_dispatch.c:155-174` (upstream, with a TODO at line 165). `panfrost/lib/pan_desc.h:265-330`: total = pow2(max(wls, 128)) x instances x core-ID range.
- **What is wasteful:** cmd_tls (13.5 MB in Fallout 4) is all WLS; it is the only user of the command buffer TLS pool on CSF. 15 of every 21 per-core slots are for cores that do not exist, and every dispatch allocates fresh.
- **Fix:** reuse within a command buffer, per size bucket, when a barrier orders the dispatches; dispatches that can run concurrently must not share. Alternatively, a pool-level WLS BO with a 230-style alias. Knob: `PANVK_WLS_REUSE=0`.
- **Saving:** 5-10 MB. **Risk:** medium. **Confidence:** medium.

### B8. The VS driver set always holds 16 attribute descriptors

- **Location:** `csf/panvk_vX_cmd_draw.c:347-419`, `prepare_vs_driver_set()` (upstream).
  - It writes MAX_VS_ATTRIBS = 16 attribute descriptors every time, then the dummy sampler, the dynamic buffers and the vertex buffers (`vb_offset = dyn + 16 + 1`).
  - It is rebuilt whenever VI, VB, VS or DESC_STATE is dirty (lines 430-438), which is most DXVK draws.
- **What is wasteful:** (16 - used) x 32 B, about 256-384 B per draw, or about 0.5-0.7 MB of descriptor memory per frame.
- **Fix:** size the attribute table per shader (highest used location + 1) and move the sampler and dynamic buffers after it. This changes the shader ABI. Knob: `PANVK_VS_ATTRIB_TRIM=0`.
- **Saving:** 5-8 MB of cmd_desc, because pools cache it. **Risk:** medium. **Confidence:** low-medium.

Considered and not recommended: lazy (fault-grown) commit for device-local VkDeviceMemory, to avoid committing DXVK's 177 MB of slack inside type-0 chunks. kbase only grows a region from the bottom up on GPU faults, that path lost output on MTK (`panvk_vX_device.c:142-146`), and explicit growth would need DXVK's cooperation.

## 4. VK_EXT_memory_budget: what 229 does, and the recommended default

Let U = heap_used (VkDeviceMemory), P = private BOs (223 counters), A = MemAvailable and H = heap size.

- Default: heapUsage = U and heapBudget = min(H, U + 0.9 A).
- 229 `PANVK_MEM_BUDGET=honest`: heapUsage = U + P and heapBudget = min(H, U + P + 0.9 A).

How DXVK turns this into its own limit is not verified here. Both plausible readings lead to the same conclusion:
- **(a) DXVK compares its own allocation D (about U) with heapBudget.** Honest mode then gives DXVK P more headroom: about 600 MB in 112, about 360 MB on 114. DXVK grows instead of compacting. **Harmful.**
- **(b) DXVK subtracts external usage (heapUsage - D) from heapBudget.** Both modes then give exactly 0.9 A of headroom. **No effect.**

So honest mode can never make DXVK more conservative. The fo4-05 HUD reading (89 % = 1744 / about 1960) fits U + 0.9 A in both readings, so an honest A/B would show either nothing or harm. `PANVK_MEM_BUDGET_HEADROOM_MB` raises the budget near OOM, which is the wrong direction on 8 GB parts; keep it at 0.

Note that 229 is not fo4-memory-analysis fix 9. Fix 9 also capped the budget at X % of MemTotal minus private bytes, which would have lowered it.

**Recommendation:**
- Do not make honest the default. Remove it, or leave it opt-in.
- Keep heapUsage = U, because DXVK's own accounting matches it.
- Make the budget pressure-aware (Q1): `min(H, U + 0.9 x max(0, A - R))`, with a swap-aware reserve R. It becomes the default on kbase builds after one Fallout 4 A/B on the X6 Pro, with `PANVK_MEM_BUDGET_PRESSURE=0` as the escape hatch.
- Optional: lower the heap from 75 % to 50 % of MemTotal on Android (`heap_memory_percent`, already a driconf option read in `get_device_heaps()`) for Vulkan-native engines that size from the heap. DXVK games are already capped by PanPlay's dxgi.maxDeviceMemory.

## 5. JM (v6/v7/v9)

- **J1. Command pool auto-trim is v10+ only.** `panvk_cmd_pool.c:124-131`: 226's gate is `pan_arch >= 10`, and 231 inherits it. On v6/v7/v9, DXVK's pools keep their all-time high-water, because 220/0085 only free on RELEASE_RESOURCES or vkTrimCommandPool, and DXVK calls neither. jm-v7 112 releases only `gs_bo_pool` on reset and trim.

  Enable 226/231 on JM after checking the JM-only pools (`varying_bo_pool`, `gs_bo_pool`, growable GS buffers). The 4 GB parts (S9 starlte, A31) benefit most. `PANVK_CMD_POOL_AUTOTRIM=0` still restores the old behaviour.
- **J2. v9 JM commits both gpu_prerast arenas (64 + 96 MiB) when the VkDevice is created.** See `panvk_vX_device.c:142-162` (`PAN_ARCH < 10 || eager`) and lines 884-894. That covers every device that enables GS, XFB, tessellation, fillModeNonSolid, viewport index or vertex stores, which is every DXVK device. On oriole it is 160 MiB per DXVK device, used or not.

  Make the v9 JM draw paths call the lazy getter at record time, as 205 did for CSF. Knob: `PANVK_GPU_PRERAST_ARENA=eager` restores it. Saving: 64-160 MiB on v9. Risk: medium, because the v9 JM paths read the arenas directly.
- **Shared with JM:** Q1 (budget), Q3 (v9 JM uses the same tessellation constants, `jm/panvk_vX_cmd_draw.c:6258-6290`), Q4 (`kbase_kmod.c` is shared), B3 (compiler) and B4 (kmod).
- **CSF only:** Q2, Q5, Q7, B1, B2, B6, B7. JM has its own batch and TLS sharing (jm-v7 107).

## 6. Checked: not worth it now, or already done

- **Descriptor pools:** 1.2-1.5 MiB for DXVK's 9 pools / 2507 sets. `panvk_vX_descriptor_set.c:433-520` sizes the BO at create time.
- **Shaders:** 7 MiB of shader BOs for 2265 graphics shaders. dev_pool and misc are 2.3 MiB. utrace is 0.1 MiB (228). BC raw stores are 0 (225).
- **Image padding:**
  - Alignment is 4096 (`panvk_image.c:1476-1496`), slices are aligned to 64 B, and each plane is rounded up to 4096.
  - A U-interleaved compressed tile is 4 x 4 blocks (16 x 16 texels), not the 64 x 64 that the 10-23 analysis assumed.
  - So the padding is at most 4 KiB per image, about 1-6 MB for thousands of textures.
- **Buffers:** 64 B alignment (`panvk_buffer.c:46-55`).
- **Small objects:** query pools, events and syncobjs are under 5 MiB together.
- **Per-region overhead:** kernel bookkeeping per BO, and the GPU and CPU page tables, are about 5 MiB each for 2.6 GB.
- **CPU mappings:** they stay mapped because SAME_VA needs them. That costs VA only, plus the PTEs from Q4's memset.
- **Already done:** 220/221/226/231 (trim), 222, 223 (accounting), 224 (4 initial chunks, TERM backlog 2), 225 (BC raw), 227/230/232 (TLS), 228 (utrace). 229 is assessed in section 4.

## 7. Measuring this in one Fallout 4 session (X6 Pro, 114 or later)

1. **Baseline:** 114, the same save, DXVK HUD `fps,memory`, `PANVK_DEBUG=mem PANVK_MEM_LOG=10`, plus the monitor script (kctx pages, MemAvailable, SwapFree, RssFile/RssAnon/VmSwap). Confirm `dev_tls` reads about 96 and the 230 TLS info line reads 8 KiB/thread. Record cmd_cs and cmd_desc under 231: this decides B1's size.
2. **P1(a) godrays off:** `prerast` should read 0. Watch the heap remainder and fps.
3. **Q1 A/B** (once built): DXVK HUD allocated vs used, devmem, SwapFree, fps, defragmentation stutter.
4. **Each quick fix with its knob at 0** for the A/B. PanProbe regressions: tess_perf (hashes), tessellation, gs_tess_primitive_id, large_draw, submit_stress, csf_event, occlusion_query, vmr_secondary, swapchain_lifecycle.
