# Fallout 4 on the X6 Pro: where the driver-internal GPU memory goes (2026-10-23)

This is a read-only analysis. Nothing was built and no device or Firebase was used.
Code: tag `rc5g-110` in `/home/flux/src/mesa-rc5g`, read with `git show` and `git archive`.
Evidence: [device-runs/2026-10-23-g615-rc5u-110/](device-runs/2026-10-23-g615-rc5u-110/results.md) (raw/fallout4, screenshots fo4-02..07).

## 0. Corrected numbers

| quantity | value | source |
|---|---|---|
| kbase GPU memory, Fallout4.exe (pid 5284) | 2 752 675 840 B = **2625 MiB** | raw/fallout4/dumpsys-gpu.txt |
| DXVK heap 0 **allocated** | **1600 MiB** (1425-1454 MiB used) | HUD line `Vidmem heap 0: 1600 MB (95%) 1449 MB used`, fo4-02..07 |
| of which DXVK chunks | 1408 MiB (type 0) + 64 MiB (type 1) = 1472 MiB | HUD `Mem type` lines |
| of which dedicated allocations (not in chunks) | about 128 MiB (render targets and similar) | 1600 - 1472 |
| **not in any DXVK allocation** | **about 1025 MiB (1.07 GB)** | 2625 - 1600 |

results.md compared the 2.75 GB with the chunk total (1.47 GB) and got "about 1.3 GB". The HUD's heap line is the right comparison (it includes the dedicated allocations), so the gap is **about 1.0 GB**, not 1.3 GB. Up to about 30 MiB of it is the WSI swapchain and present buffers. They are VkDeviceMemory, but the WSI layer allocates them, not DXVK.

Two facts from the code explain why this memory hurts so much:
- **Every kbase BO is fully committed and CPU-mapped.** Without `PAN_KMOD_BO_FLAG_ALLOC_ON_FAULT`, `kbase_kmod_bo_alloc()` (`src/panfrost/lib/kmod/kbase_kmod.c`) commits `va_pages`, mmaps the SAME_VA cookie and memsets every committed page. This is why VmRSS/RssFile (2.85 GB) is about the same as the kbase total. kbase pages are not on the LRU and cannot be swapped. Every MiB of driver memory is pinned, and it pushes one more MiB of the game's FEX/Wine heap into zram (VmSwap 2.43 GB, RssAnon 210 MB).
- **Only VkDeviceMemory counts in `heap_used`.** `panvk_AllocateMemory()` adds to `physical_device->memory.heap_used`. `panvk_priv_bo_create()` and the kbase tiler heap (`KBASE_IOCTL_CS_TILER_HEAP_INIT`) do not.

## 1. What PanVK reports to DXVK (VK_EXT_memory_budget)

- **Heap size**: one DEVICE_LOCAL heap of **5 765 070 848 B = 5498 MiB**, from `raw/all/.../device_info.json`. That is `os_get_gpu_heap_size()` with the default heuristic: 75 % of MemTotal (7.16 GiB) because RAM > 4 GiB (`get_device_heaps()`, `panvk_physical_device.c:510`). DXGI reports this as DedicatedVideoMemory. Memory types: 0 = DEVICE_LOCAL, 1 = DL|HV|HC, 2 = DL|HV|HCACHED.
- **Budget** (`panvk_GetPhysicalDeviceMemoryProperties2`, `panvk_physical_device.c:1741`):
  `heapBudget = min(heap_size, heap_used + 0.9 * MemAvailable)`, rounded down to 1 MiB, with MemAvailable cached for 100 ms. `heapUsage = heap_used`.
  - With MemAvailable = 84 MB: budget is about 1600 + 76 = 1676 MiB, and 1600 / 1676 = **95 %**, which is what the HUD shows. In fo4-02 the HUD says 103 %: the budget was sampled while MemAvailable was about 0, so the budget equalled DXVK's own usage at that moment.
  - So DXVK is told it has **no headroom at all**. The budget follows whatever is already allocated, so it cannot steer DXVK down, and D3D11 cannot evict textures anyway. A budget at or over 100 % also makes DXVK 2.5+ release empty chunks immediately and, if memory defragmentation is enabled in this DXVK build, relocate (copy) resources between chunks. Relocation copies BC images through `vkCmdCopyImage`, which matters for the raw store (section 2.1) and for the black head (section 4).
- **Can DXVK see the BC shadow memory?** **Yes, the shadow itself.** The ETC2/EAC or AFRC shadow is part of the image's `VkMemoryRequirements` (`panvk_image_get_total_size()` adds `bc_decoded_offset + shadow size`), and `bc_shadow` is bound into the parent image's memory (`panvk_image_bind()`, `panvk_image.c:1839-1847`). So the shadows are inside the 1600 MiB and inside `heap_used`.
  **No, the raw stores** (201/207 `struct panvk_bc_raw` BOs from `panvk_priv_bo_create()`). The same goes for tiler heaps, command and descriptor pools, shaders, TLS and the other private BOs. DXVK sees them only indirectly, as a smaller MemAvailable term.

## 2. Candidates, sized for a game with 1-1.5 GB of BC textures

Assumed texture mix (vanilla Fallout 4: diffuse BC1/BC3, normal and specular BC5, some BC7, almost no BC6H), at 1.2 GB of BC data: BC1 30 % (360 MB), BC3 20 % (240), BC5 40 % (480), BC7 8 % (96), BC6H 2 % (24).
"Visible" means the memory is inside DXVK's 1600 MiB. "Hidden" means it is part of the about 1025 MiB gap.

| # | candidate | visible / hidden | estimate | confidence |
|---|---|---|---|---|
| 2.2 | tiler heaps (current + 3 retired + spare + TERM backlog) | hidden | **150-500 MiB** (cap 5 x 400 MiB + queue) | medium: not measured, but the mechanism is clear |
| 2.1 | BC raw stores (201/207) | hidden | **20-150 MiB** typical; **up to about 1.2 GB** if `bc_raw.needed` flipped and stores got pinned | low: depends on log lines nobody captured |
| 2.4 | per-command-buffer pools (CS + desc + TLS slabs) | hidden | **30-100 MiB** | medium |
| 2.7 | device TLS BOs (grow-only list) | hidden | **10-60 MiB** | low-medium |
| 2.5 | shader binaries (exec pool) | hidden | **15-40 MiB** | medium |
| 2.3 | descriptor pools | hidden | **10-40 MiB** | medium |
| 2.6 | gpu_prerast arenas (205) | hidden | **0 or 64 MiB** (tess: +96, +96) | unknown: depends on whether FO4 hits a GS/XFB lowering |
| 2.9 | fixed per-device and per-queue BOs (utrace 10 MiB, syncobjs, OOM handlers, ring buffers, null descs, EAC LUT) | hidden | **15-25 MiB** | high |
| 2.9 | WSI swapchain + present buffers | hidden from DXVK, in `heap_used` | **15-30 MiB** | medium |
| 2.9 | kbase GPU MMU page tables for about 2.6 GiB mapped | hidden | **about 5-6 MiB** | medium |
| 2.8 | query pools | hidden | **< 5 MiB** | high |
| 2.1 | BC shadows (ETC2/EAC/AFRC) | **visible** | 1.0x the BC size for BC1-5/BC7, 4x for BC6H UF, 8x for BC6H SF; **+0-10 % overall** (+0-120 MB) | high |
| 2.8 | AFBC headers / CRC | visible | 10-20 MiB / < 1 MiB | high |

**Sum of hidden midpoints: about 400-600 MiB.** That leaves 400-600 MiB that only the upper ends explain: tiler heaps near their cap (most likely), or pinned or flipped raw stores. **This has to be measured** (fix 1). Without a per-kind counter, these numbers cannot tell the two apart.

### 2.1 BC texture emulation

Code: `panvk_image_init()` (`panvk_image.c:972`), `panvk_bc_shadow_info()` (`:489`), `panvk_bc_lowmem_format()` and `panvk_bc_compact_format()` (`panvk_image.h:197-245`), raw store `panvk_image.c:1880-2265`, lazy switch `panvk_vX_cmd_meta.c:1945`.

**Shadows (visible).**
- BC1-BC5 use the lowmem path (`bc_lowmem = 1`): BC1 RGB to ETC2 RGB8 (4 bpp), BC1 A to ETC2 RGB8A1, BC2/BC3 to ETC2 RGBA8 (8 bpp), BC4/BC5 to EAC R11/RG11. That is the same size as the source, plus U-interleaved padding: each level is padded to 16x16 blocks (64x64 texels), so small mips grow and large ones barely change.
- BC7 (`bc_lowmem = 2`) gets an RGBA8 AFRC shadow with CU 16 = 8 bpp, which is the same size as BC7. Cube-compatible BC7 images stay on AFBC (up to 32 bpp, so up to 4x).
- BC6H gets B10G11R11 (32 bpp, 4x) unless `PANVK_BC_BC6H=astc` (8 bpp). SF16 gets RGBA16F (8x).
- **Exact-path fallbacks**: images that `panvk_bc_lowmem_allowed()` rejects keep the raw plane *and* an uncompressed shadow in the image memory. That is about 3x for BC1/BC5 (raw + RGBA8 AFRC or RG8). This covers MUTABLE BC4/BC5 without a single-format list, BLOCK_TEXEL_VIEW_COMPATIBLE, HOST_TRANSFER, external memory and sparse images. Ordinary FO4 DDS textures are typed and should not hit this. A typeless BC5 would.
- **Small images**: a raw plane under `PANVK_BC_RAW_MIN` (64 KiB, for example BC3 at or below 128x128 with mips) keeps the raw blocks in the image memory, so 2x. These are many images but little memory.
- **Net for the assumed mix: about 1.25-1.3 GB visible for 1.2 GB of BC data.** The shadows are not the leak.

**Raw stores (hidden).**
- DXVK sets TRANSFER_SRC on textures, so BC7 and BC6H always get a raw store (`raw_store`; 207's lazy mode only covers `bc_lowmem == 1`).
- BC1-BC5 at or above 64 KiB are **lazy** (no raw blocks) **until the first raw read of any lazy image** (`bc_raw_lazy_hit()`: copy-to-memory, or a copy to a non-identical shadow). After that, **every BC1-5 image created later gets a raw store** (`dev->bc_raw.needed`). The warning is logged once and is not visible without logcat.
- Lifetime: an upload materializes a BO of the BC size (`bc_raw_materialize()`, a WB-mapped priv BO). The BO is spilled to the spill file and freed only after **every command buffer that referenced it has been reset** (`panvk_bc_raw_release()` from the csf cmdbuf reset), so the BOs from DXVK's loading command lists are resident at the same time.
- A store that is read back from the file twice is **pinned resident forever** (`remat >= 2`). With no spill directory, or after a write error, stores are pinned from the start. DXVK relocating the same BC7 image twice is enough to pin it.
- Steady state: BC7 + BC6H raw (about 120 MB in the assumed mix) is resident only while in flight, so about **20-150 MiB**. Worst case, `needed` flipped early, relocation pins stores, or the spill directory is unusable: **up to about 1.2 GB**. That alone would explain the gap.
- **Staging**: DXVK's own staging buffers are visible. The driver adds only small per-upload `panvk_cmd_alloc_dev_mem()` tables, plus the staging copy for non-BC-to-BC copies. These live in the command-buffer desc pools (2.4), not in separate BOs.

### 2.2 Tiler heaps (the most likely large item)

Code: `panvk_physical_device.c:1445-1451`, `csf/panvk_vX_gpu_queue.c:79-110, 2080-2250, 2412-2500, 3100-3155`, `csf/panvk_vX_cmd_draw.c:59-72, 1371, 6092-6096`, `csf/panvk_queue.h:155-185`.

- On kbase, chunks are **1 MiB, 10 initial, max_chunks = MAX2(400, 200) = 400 MiB per heap**.
- **Chunk recycling is disarmed on kbase.** `cmdbuf_skips_gpu_heap_ops()` emits only VERTEX_TILER_STARTED, because kbase terminated the group on its heap-statistics check (csf-v11/098). So **a heap only grows until it is renewed**. On v10+ the tiler heap holds the **IDVS vertex packets** (position + varyings, `MALI_ATTRIBUTE_TYPE_VERTEX_PACKET`, `packet_stride = generic_size_B + 16`) as well as the polygon lists. Every vertex the frame shades lands in the heap.
- **Renewal**: the default `KBASE_TILER_HEAP_RENEW_WORK` = 65536 "work" units, and one unit is one vertex x instance (`account_tiler_work()`). Every FO4 submit (1.4-2.3k draws per frame) exceeds that, so **the heap is renewed on nearly every submit**, which is 3 per frame.
- **Alive at once**: the current heap, up to 3 retired heaps (`PANVK_KBASE_HEAP_SLOTS - 1`, 107), 1 pre-created spare (10 MiB, 104), and up to `PANVK_KBASE_HEAP_TERM_MAX` = 16 contexts queued for TERM on the single `panvk_heap` thread (192), which also builds the spare.
- **GPU-bound at about 110 ms per frame**, the 3 retired slots stay busy. `kbase_renew_tiler_heap()` then returns VK_INCOMPLETE and the current heap keeps growing until the 4x backpressure wait. So about 4-5 heaps each hold one or two submits' worth of vertex packets.
- **Size**: FO4 at 720p with shadow cascades plausibly shades 1-4 M vertices per frame. At about 40-80 B per vertex (packet + polygon-list share) that is 60-300 MiB per frame. 1.3-2.5 frames in flight gives **150-500 MiB**. The hard cap is 5 x 400 MiB plus the TERM backlog.
- **CPU cost**: about 15 renewals per second, each with HEAP_INIT (commits and zeroes 10 MiB) and a TERM, all under direct reclaim. That matches `Fa:panvk_heap0` at about 8 % of a core. The NFS capture (stutter-followup.md) measured TERM at 1.5 ms p50 and 23 ms max even without memory pressure.

### 2.3 Descriptor pools

Code: `panvk_vX_descriptor_set.c:443-510`.
- The pool BO is `(sum of stride x descriptorCount + maxSets) x 32 B`. The stride is 1 per descriptor, and 5 for COMBINED_IMAGE_SAMPLER (3 planes + 2 samplers). It is allocated **in full at vkCreateDescriptorPool and committed** (a CPU-written BO cannot be grow-on-fault on kbase).
- DXVK's HUD shows 11 pools and 2.0-2.9k live sets. DXVK sizes its pools for thousands of sets with about 10 descriptors each, so about 1-3 MiB per pool: **10-40 MiB**, mostly unused.

### 2.4 Per-command-buffer pools, and whether 220 helps

Code: `csf/panvk_vX_cmd_buffer.c:1088-1125`, `panvk_mempool.c`, `panvk_cmd_pool.c`.
- Each command buffer has a CS pool and a desc pool with 64 KiB WB slabs, plus a TLS pool. On reset the slabs go to the **VkCommandPool's free lists, which never shrink** except through 220.
- A FO4 frame with about 2k draws needs on the order of 2-6 MiB of CS + descriptors + push uniforms.
- DXVK keeps one or more command pools per DxvkCommandList and recycles about 10-16 lists (3 submits per frame x up to 3-4 frames + the initializer context), so the retained high-water marks add up to **30-100 MiB**.
- **220 does not help much here.** It frees only on `vkResetCommandPool(RELEASE_RESOURCES)` or `vkTrimCommandPool`. DXVK resets its pools every submission with flags = 0, and the pools are reused right away, so the steady state is the per-list high-water mark either way. 220's message assumes "DXVK trims command pools". This should be checked against the DXVK 3.1.1 source. If DXVK never trims, 220 only matters for other apps.

### 2.5 Shader binaries

- `panvk_pool_upload_aligned(&dev->mempools.exec, ...)`, with 16 KiB slabs in the WB exec zone.
- PanVK exposes `VK_EXT_graphics_pipeline_library`, so DXVK compiles **every** shader module (HUD: 2265 graphics + 95 compute) as a library, and later the linked or optimized pipelines (232).
- At 4-10 KiB per binary, plus IDVS variants: **15-40 MiB**. It grows with play time, does not churn, and is low priority.

### 2.6 Prerast arenas (205)

- 64 MiB (draw arena) and 96 MiB (+96 MiB second tess arena), fully committed, created on CSF at the first draw that needs a gpu_prerast lowering or tessellation (`panvk_per_arch(device_gpu_prerast_arena)`).
- DXVK enables geometryShader, transformFeedback and tessellationShader, so `gpu_prerast_wanted` is set, but only the first real GS/XFB/viewport-index lowering commits the arena.
- FO4 does not tessellate. Whether it hits a GS/XFB lowering is unknown, so the estimate is **0 or 64 MiB**. The `panvk: gpu_prerast draw arena committed (64 MiB)` info line answers it.

### 2.7 Device TLS (097)

- `get_device_tls()` (`csf/panvk_vX_cmd_buffer.c:91`) appends a new BO whenever a command buffer needs more stack than the current top BO has. **Smaller BOs are never freed** until the device is destroyed.
- Size = `pan_get_total_stack_size(tls_size, thread_tls_alloc, core_id_range)`, about tls x 2048 threads x core-id range. For 512 B to 2 KiB of spill per thread on 6-8 cores that is 8-32 MiB for the top BO, plus the earlier ones (up to 2x in total): **10-60 MiB**.
- The BC transcoders and large DXVK shaders are the likely spillers.

### 2.8 Queries, AFBC, CRC

- **Queries**: reports come from `mempools.rw` and availability words from `rw_nc`. Under 5 MiB for DXVK's occlusion pools.
- **AFBC/CRC**: render-target metadata is inside the image memory, so visible. At 720p it is about 10-20 MiB of AFBC headers and padding, and the CRC buffers are under 1 MiB.

### 2.9 Growable versus fully committed, and fixed costs

- **Only GROW_ON_GPF BOs are partly committed.** On CSF that means only the kbase-side tiler heap chunks (which grow by allocating chunks) and BOs with `ALLOC_ON_FAULT`. Every other BO (VkDeviceMemory, pools, raw stores, arenas, TLS, descriptor pools) is **committed and zeroed in full at allocation**. So "allocated" equals "resident" for everything except the heap.
- **Fixed costs**:
  - utrace `copy_buf_heap_bo`: 10 MiB (`PANVK_UTRACE_CLONE_MEM_SIZE`), allocated even with tracing off.
  - The kbase spare heap (10 MiB).
  - Queue ring buffers (64 KiB per subqueue), the render-desc ring buffer (512 KiB), OOM handlers, null descriptors and the EAC LUT.
  - Total **about 15-25 MiB**.
- **kbase per-context page pools** keep freed pages (raw-store churn, heap TERM) out of the system without showing in `dumpsys gpu`. This is extra RAM pressure that neither DXVK nor dumpsys sees.

## 3. Ranked fixes

The ranking is expected saving x confidence / risk. Fixes 2 and 3 can be tried now with existing knobs. The others need a patch: the next csf-v11 number is 223.

| rank | fix | expected saving | risk | knob | touches |
|---|---|---|---|---|---|
| 1 | **Measure first: per-kind private-BO accounting.** Tag `panvk_priv_bo_create()` callers by kind (bc_raw, desc_pool, cmd_cs, cmd_desc, tls, exec, prerast, wsi, misc). Count the kbase tiler heap bytes as `initial_chunks x chunk_size` per live context, plus the growth read back from the heap context at TERM. Print one line every N seconds and on device destroy. Log `dev->bc_raw.resident_B/spilled_B` and `needed` (already counted, never printed). Run with `MESA_LOG_FILE=<app dir>/mesa.log` because logd is stopped on this phone. | 0 (it tells which of 2-6 is the 1 GB) | none | new `PANVK_DEBUG=mem` | `panvk_priv_bo.c/.h`, `panvk_device.h`, `csf/panvk_vX_gpu_queue.c`, `panvk_instance.c` (new patch) |
| 2 | **Game-side texture budget**: `iTexMipMapSkip=1` (or Medium textures) in `Fallout4Prefs.ini` / `Fallout4.ini` | **-0.6 to -1.0 GB** of visible DXVK memory, which also lowers tiler/heap pressure indirectly | none (quality) | game ini | PanPlay per-game preset only |
| 3 | **Bound the tiler heaps.** A/B now: `PANVK_KBASE_ASYNC_HEAP_TERM=0` (no TERM backlog), `PANVK_KBASE_HEAP_RENEW_WORK=16384`. Read `dumpsys gpu` Proc total and fps for each. Patch: (a) kbase initial chunks 10 -> 2-4 (each renewal commits and zeroes 2-4 MiB instead of 10; about 15 renewals/s); (b) max_chunks 400 -> 128-160 on kbase, so the 4-5 live heaps cap at about 0.6-0.8 GiB instead of 2 GiB (overflow goes through the existing tiler OOM / incremental-render handler); (c) when the TERM queue holds more than 4 contexts, destroy synchronously; (d) longer term: re-arm FRAGMENT_COMPLETED recycling with kbase-valid heap statistics, so one heap serves every in-flight pass and renewals stop. | (a)+(c) -30 to -150 MiB; (b) caps the worst case; (d) -100 to -400 MiB and about 8 % of a CPU core | (a)/(c) low; (b) medium (more OOM-path renders, test with tess_perf / large_draw); (d) high (the 098 DEVICE_LOST history) | `PANVK_KBASE_HEAP_RENEW_WORK`, `_INTERVAL`, `PANVK_KBASE_ASYNC_HEAP_TERM`; new `PANVK_KBASE_HEAP_CHUNKS=init,max` | `panvk_physical_device.c:1445-1451`, `csf/panvk_vX_gpu_queue.c` (renewal, `kbase_heap_term_push`), `csf/panvk_queue.h`; on top of 102/104/107/192 |
| 4 | **BC raw stores: no global flip, no pinning, lazy for BC6H/BC7.** (a) Make 207's lazy mode cover BC7/BC6H (`bc_lowmem` 2/4: decode-once shadows, so a BC to BC copy can copy shadow blocks as for ETC2); (b) replace the device-wide `bc_raw.needed` with a per-image raw store created at the first raw read of *that* image (the read itself still cannot be exact); (c) drop the `remat >= 2` pin, or spill again after M frames; (d) free the raw BO as soon as its upload has completed on the GPU (submit-time fence) instead of waiting for DXVK to reset the command list. | -20 to -150 MiB typical; up to -1 GB if the flip or pinning is what happened here | (a) medium: BC7/BC6H readback becomes "skipped + logged", as BC1-5 already are; (b)-(d) low | `PANVK_BC_RAW=lazy` default; A/B now with `store` and `plane` | `panvk_image.c` (`panvk_image_init`, `bc_raw_*`), `panvk_vX_cmd_meta.c` (`bc_raw_lazy_hit`, `cmd_copy_image`, `bc_copy_shadow_ok`); on 201/207 |
| 5 | **Command-pool auto-trim for apps that never trim.** On `vkResetCommandPool(flags = 0)`, keep at most the slab count the last N resets used (or a cap, for example 1 MiB per pool) and free the rest. Also cap `tls_big_bo_pool`. | -20 to -80 MiB | low | extend `PANVK_CMD_POOL_TRIM` (`=2` for auto) | `panvk_cmd_pool.c`, `panvk_mempool.c` (on 220) |
| 6 | **Free superseded device TLS BOs**: keep only the largest, and free the older ones once the queue seqno passes the last command buffer that used them | -5 to -30 MiB | low | none | `csf/panvk_vX_cmd_buffer.c` `get_device_tls()` (097) |
| 7 | **Descriptor pools: back them in 256 KiB blocks on demand** instead of one BO of the declared maximum | -10 to -30 MiB | medium (the set address stability and free paths change) | none | `panvk_vX_descriptor_set.c` |
| 8 | **Don't commit the utrace clone heap unless u_trace is enabled** | -10 MiB | very low | `PANVK_UTRACE_CLONE_MEM_SIZE` (already) | `csf/panvk_vX_utrace.c` |
| 9 | **Honest budget**: report `heapUsage = heap_used + private BO bytes + tiler heap bytes`; cap `heapBudget` at `min(heap_size, X % of MemTotal - private bytes, heap_used + 0.9 x MemAvailable)`; lower the default heap to about 50 % of RAM on 8 GB phones (`heap_memory_percent`) | 0 MB directly. It makes the DXVK HUD and DXGI tell the truth. It can stop DXVK from relocating forever if that is confirmed | low | driconf/env `heap_memory_percent` (0.0-1.0) | `panvk_physical_device.c:1741` |
| 10 | **BC6H UF as ASTC 4x4 HDR by default on G615** | 0-50 MiB visible (3/4 of the BC6H shadows) | low-medium (quality: 52-64 dB on real content, half-rate filtering) | `PANVK_BC_BC6H=astc` (exists) | `panvk_image.c` default (202/208) |
| 11 | Prerast arena: if the info line shows it is committed in FO4, make the 64 MiB arena 16 MiB plus the chunk loop (196) | 0-48 MiB | medium | `PANVK_GPU_PRERAST_ARENA` | `panvk_vX_device.c:150-200`, `panvk_gpu_prerast.h` |
| 12 | Driver-side mip skip for large BC textures (shadow of BC level 1 and up, LOD-biased views): the driver version of fix 2 | -50 to -70 % of the BC shadow bytes for textures of 2048 and up | high (`textureSize`/`txf` results change, the view and copy maths changes) | new `PANVK_BC_MIP_SKIP` | `panvk_image.c`, `panvk_vX_image_view.c`, `panvk_vX_cmd_meta.c` |

Realistic driver-side total from fixes 3-8: **-150 to -450 MiB**. Fix 4 reaches up to about -1 GB if the measurement in fix 1 shows the raw store flipped. Fix 2 alone frees more than any driver change, so it should be the PanPlay default preset for this game on phones with 8 GB or less.

**Measurement plan for the next device session** (one launch each, same save, read the HUD + `dumpsys gpu` + `/proc/<pid>/status` after 60 s):
1. Default + `MESA_LOG_FILE` (+ `PANVK_DEBUG=mem` once fix 1 exists). Grep for `without raw blocks skipped`, `BC raw store spills to`, `no BC raw store spill directory`, `gpu_prerast ... arena committed`.
2. `PANVK_KBASE_ASYNC_HEAP_TERM=0`.
3. `PANVK_BC_RAW=store` and `PANVK_BC_RAW=plane`. The difference between them is the raw-store share.
4. `PANVK_BC_MODE=exact`. This gives the shadow share of DXVK's 1600 MiB. Expect it to go up.
5. `iTexMipMapSkip=1`.

## 4. The black head (fo4-07, the woman on the right)

What the screenshots show: in fo4-02 (first gameplay frame, 19:58) her head and hair render normally at the left edge. In fo4-07 (about 20:07) the head and hair are a flat black silhouette, while the player's head (fo4-06) is fine. So the head became black **later in the session**, after memory had been exhausted for minutes. That makes a decode error at first upload less likely, and points to a copy, re-creation or allocation event. The intro spouse's face (and the player's) is regenerated at runtime after the mirror scene, so this actor goes through texture copies and re-creation that ordinary NPCs do not.

Likely driver causes, most likely first:

1. **A lazy-raw copy that 207 skips.**
   - Cause: a copy *from* a lazy BC1-5 image (no raw blocks) is dropped and only logged once (`bc_raw_lazy_hit()`) when it goes to a buffer or memory (D3D11 staging `Map(READ)`, which DXVK does as an image-to-buffer copy), to a non-BC image, or to a BC image whose shadow differs (`bc_copy_shadow_ok()` needs `bc_lowmem == 1` and the same `bc_format` on both sides). One example of a differing shadow is a MUTABLE BC4/BC5 destination on the exact path. The destination is never written, so it keeps zero blocks. A zero ETC2 block is black, and a zero BC5/EAC normal gives xy = (-1, -1), so z = sqrt(-1) = NaN, which lights to black.
   - Confirm: the log line `panvk: ... of a BC image without raw blocks skipped` (needs `MESA_LOG_FILE`). Rerun with `PANVK_BC_RAW=store`. If the head stays correct, this is the cause.
2. **207 + `needed` flip + shadow-only copy, then a raw copy from the destination.**
   - Cause: once `bc_raw.needed` is set, new BC1-5 images get raw stores. `bc_copy_image_shadow()` copies only the shadow into such a destination ("the destination's raw store, if it has one, is not written"), so its raw store holds zeros. A later copy *from* that image is no longer lazy and uses the zero raw blocks: `copy_image_raw` + decode, so black. DXVK relocating a texture twice under budget pressure (section 1) produces exactly this pattern for the runtime face textures.
   - Confirm: same log line + `PANVK_BC_RAW=store` (fixes it) versus default. `PANVK_BC_RAW=plane` also avoids it. If this DXVK build has the defragmentation option in dxvk.conf, turning it off should hide the bug.
   - Fix: in `bc_copy_image_shadow()`, mark a destination raw store as invalid ("no exact blocks") so it is treated as lazy, instead of keeping zeros (`panvk_vX_cmd_meta.c:2310`).
3. **Raw-store BO allocation or `pread` failure under memory exhaustion.**
   - Cause: `bc_raw_materialize()` can fail with OOM. Then `bc_raw_use()` sets the command-buffer error and the copy or upload is skipped. A failed `pread` from the spill file gives a zeroed BO, which decodes to black.
   - Confirm: the log lines `BC raw store read-back failed` or `spill failed`, or a DXVK `vkEndCommandBuffer` error in dxvk-*.log. The bug should not reproduce with more free RAM (fix 2), which also tells it apart from causes 1/2.
4. **BC5 normal-map transcode (206 endpoint-pair EAC) or BC5 signedness.**
   - Cause: a wrong RG11 transcode of the face or hair normal or specular map gives NaN or extreme lighting. bc_decode/bc_perf pass, but FO4's facegen maps are not in the test set.
   - Confirm: `PANVK_BC_MODE=exact` (lowmem off, exact RG8 shadow). Then extract the actor's head textures (BA2: `textures\actors\character\...`, FaceGenData) and run them through `tests/dxvk/vulkan/bc/bc_decode.c` + `bc_verify.py`, comparing default against exact. If they match and the head is still black with `exact`, BC is not the cause.
5. **sRGB on the transcoded shadows** (ETC2 stored UNORM, sRGB chosen per view in `panvk_bc_view_pfmt()`), or **AFRC with an sRGB view** for BC7_SRGB/BC1-3 on the exact path.
   - This is unlikely, because a format bug would hit many objects and not one head. Confirm with `PANVK_BC_AFRC=0` (no AFRC on the exact-path shadows) and `PANVK_BC_MODE=exact`.
6. **Not BC: a CRC/transaction-elimination miss on a runtime render target** (the face tint is rendered to a texture).
   - Cause: if a copy or clear into the render target outside a render pass does not invalidate the CRC header, later tiles can be eliminated and keep stale or zero content.
   - Confirm: `PANVK_DEBUG=no_crc`. If the head renders correctly with it, look at `copy_buffer_to_image_touches_crc()` and its counterparts for image-to-image and clear paths.

A cheap single run that splits these: `PANVK_BC_RAW=store` + `MESA_LOG_FILE`. If the head is fine, it is 1 or 2. If it is still black, add `PANVK_BC_MODE=exact` (4, 5), then `PANVK_DEBUG=no_crc` (6), then free RAM (3).
