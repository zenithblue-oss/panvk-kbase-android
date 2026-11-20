# Real sparse residency on stock kbase (Mali v6-v14) - feasibility and design

2026-10-27. Research + design only. No kernel changes (stock phones, no root).
Revised 2026-11-20 after Review Council Round 1 (4.7/10, 9 models); see
"Council Round 1 corrections" below. Superseded where it conflicts by the
**Round 3 authoritative plan** (Gemini 4 Neon proposal, adopted 2026-11-20),
which is the next section.
Goal: real Vulkan sparse residency on the Android ARM DDK (`/dev/mali0`, the
`kbase` UAPI). The honest capability this can reach is **Tiled Resources
"Tier 1-minus-Aliased"**, not conformant Tier 2. Feature level 12_0 is reported
only through an opt-in, per-title override (see below). FL 12_0 enablement is
**decoupled** from sparse residency. Sparse scope: **CSF v10-v14 only**
(v10 G710, v11 G715/G615, v12-v14). JM v6 (G72), v7 (G52), v9 (G78) are out of
scope for sparse work; they get FL 12_0 only through Phase 0 + overrides.

---

## Round 3 authoritative plan (2026-11-20)

Source: Gemini 4 Neon, Council Round 3 proposal. This section is the plan of
record. Where any later section (Round 1 corrections, Executive summary,
design, Q3 staged plan) conflicts with it, this section wins. Later sections
stay as design reference for Path 3B only.

### Goals

1. **Decouple FL 12_0 from sparse residency emulation.** FL 12_0 for titles
   that do not use tiled resources does not wait on any sparse stage.
2. **Ship native FL 12_0 for non-tiled titles now**, via Phase 0 (S-Pre) plus
   per-title override profiles. No sparse code is needed for these titles.
3. **Narrow hardware scope for any sparse work to CSF v10-v14.** JM v9 is
   dropped (with v6/v7). JM arches get Phase 0 and overrides only.

### Phase 0 - S-Pre (ships first, no sparse dependency)

- **robustness2:** `robustImageAccess2` + `nullDescriptor` on all arches
  (v10 `robustImageAccess2` is GPU-emulated, patch 184; do not re-gate).
- **Descriptors:** `VK_EXT_mutable_descriptor_type`; descriptor limits that meet
  resource binding tier 2/3 (vkd3d's binding-tier derivation).
- **SM 6.2+ float controls:** `shaderFloat16` / `shaderInt16`,
  `VK_KHR_shader_float_controls` (fp16/fp32 denorm preserve/flush, rounding
  modes), subgroup operations required by SM 6.x wave intrinsics.
- **Per-title override profiles** for Bucket A titles (see Phase 1), opt-in in
  PanPlay, never global:
  - Wine vkd3d 1.18: `VKD3D_CAPS_OVERRIDE=feature_level=12.0`
  - vkd3d-proton: `VKD3D_FEATURE_LEVEL=12_0`

### Phase 1 - Title census and vkd3d audit

Log `CreateReservedResource`, `UpdateTileMappings`, `CopyTileMappings`,
`GetResourceTiling` per target title. Classify each title:

| Bucket | Meaning | Treatment |
|--------|---------|-----------|
| **A** | Non-tiled: probes FL 12_0, never creates reserved resources | Phase 0 + override profile |
| **B** | Narrow read-only SRV streaming: reserved textures sampled as SRV, tiles mapped/unmapped, no aliasing, no UAV/RT writes to reserved resources | Candidate for Path 3B |
| **C** | True Tier 2 / aliasing: `CopyTileMappings` or multiple tiles mapped to the same heap offset, writes to reserved resources that need per-tile semantics | Out of scope on kbase; runs at 11_1 or listed unsupported |
| **D** | Fails for non-sparse reasons | Tracked as ordinary driver bugs, outside this plan |

**vkd3d audit:** check whether vkd3d-proton (and Wine vkd3d) allow
`CreateReservedResource` when `sparseResidencyAliased=false`, with and without
the FL override. If vkd3d rejects it, Bucket B needs either a vkd3d-side
change or is reclassified; record the exact code path.

### Phase 2 - S0-GPU hardware reality gate (CSF v10/v11 and v14)

Run on real hardware. Each test records `used_pages` (kbase region commit /
`mem_profile`) before and after.

- **Test A - prefix commit and fault survival.** One 64 MiB `GROW_ON_GPF`
  region. GPU touches offset 60 MiB only. Pass: the context survives (no group
  fault, no `DEVICE_LOST`, next submit succeeds). Record whether `used_pages`
  grows by one page/tile or by ~60 MiB (prefix explosion).
- **Test B - recycled pool zero-leak.** Free a region filled with a pattern,
  allocate and commit a new one, GPU-read it. Pass: no pattern bytes visible
  (pool pages come back zeroed).
- **Test C - concurrent shrink latency.** Under a 60 FPS GPU load, shrink a
  region with `MEM_COMMIT`. Record frame-time spikes and shrink latency. Fail:
  stalls or faults in the concurrent workload.
- **Test D - mip-tail footprint.** One image with packed mip tail on a single
  `kbase_va_region`. Bind only the mip tail. Record the physical footprint
  (expected: prefix up to the mip-tail offset).

### Phase 3 - Branching execution

**Path 3A (default).** Taken if the census finds fewer than 2 Bucket B titles,
or S0-GPU shows prefix explosion (Test A) or CSG faults. S1-S6 are archived
permanently. FL 12_0 = Phase 0 + per-title overrides. Bucket B/C titles run at
11_1 or are listed unsupported.

**Path 3B (bounded SRV-first shadow sparse).** Taken only if Bucket B justifies
it (2+ titles) and S0-GPU passes. Rules:

- **Budget.** kbase commit is prefix-only, so on the first mip-tail bind charge
  the full virtual image size against `PANVK_SPARSE_CAP_MB` (and the heap
  budget). Over cap: `VK_ERROR_OUT_OF_DEVICE_MEMORY`.
- **Zero init.** If Test A or Test B fails, zero new backing explicitly; do not
  rely on GPF-grown pages or pool zeroing ("GPF magic").
- **No fake per-tile dirty tracking.** Scope is read-only SRV streaming. If a
  reserved resource is ever writable (UAV/RT/DS/copy destination), use
  whole-resource dirty tracking, not per-tile bits.
- **Layout.** AFBC disabled for every sparse image (mandatory). A compute
  shader swizzles between the D3D12 64 KiB standard tile layout and Mali 16x16
  u-interleaved 2D layout on copy-in/copy-out.
- **Queue.** Timeline-semaphore-safe async bind queue. No mid-frame
  `MEM_COMMIT` shrink; reclaim backing only at resource destruction or at an
  idle boundary where the resource has zero bound tiles.

---

## Council Round 1 corrections (2026-11-20)

These rules override anything later in this file that conflicts with them.
The Round 3 authoritative plan above overrides these rules. In particular:
JM v6/v7/v9 are out of sparse scope (item 2 applies to v14 only), per-tile
dirty bits (item 4) are replaced by Path 3B's whole-resource rule, the
deferred uncommit worker becomes reclaim-at-destroy/idle only, and the order in
item 5 becomes Phase 0 -> Phase 1 -> Phase 2 -> Phase 3A/3B.

1. **Specification integrity.** No global override, PanPlay default, or driver
   cap claims conformant Tiled Resources Tier 2. The kbase UAPI cannot give
   `sparseResidencyAliased` (see Q1/Q2), so the honest tier is
   **Tier 1-minus-Aliased**, and vkd3d correctly derives tier 0 from it. The
   feature-level override is a **per-title, opt-in compatibility hack**:
   - Wine vkd3d 1.18: `VKD3D_CAPS_OVERRIDE=feature_level=12.0`
   - vkd3d-proton: `VKD3D_FEATURE_LEVEL=12_0`

   Enable it only for a title that the census (item 5) shows probes the
   feature level but never calls `UpdateTileMappings` / `CopyTileMappings` and
   never aliases tiles. A title that uses tiled resources gets no override; it
   runs at 11_1 or is listed as unsupported.
2. **Memory budget (JM v6/v7/v9, v14).** The 16 GiB monolithic eager arena is
   dropped. Each sparse resource gets its own SAME_VA `GROW_ON_GPF` region
   sized to the resource (rounded to 64 KiB) and freed with it. All sparse
   backing in one kbase context is capped (default **256 MiB**, calibration
   knob `PANVK_SPARSE_CAP_MB`) and also capped by the heap budget
   (`VK_EXT_memory_budget`). A reservation that would exceed either cap fails
   with `VK_ERROR_OUT_OF_DEVICE_MEMORY` (D3D12: `E_OUTOFMEMORY` from
   `CreateReservedResource`). The driver never over-commits silently.
3. **CSF faults and non-resident reads.** The store filter does not cover reads.
   TMU bilinear/aniso footprints and border taps can touch an unbound tile, and
   on CSF a fault on unbacked memory kills the group. The S0 GPU gate (item 5)
   must choose one of two paths on hardware:
   - **(a) GPF growth.** Verify that a read fault on an uncommitted
     `GROW_ON_GPF` FIXED tile is handled by kbase growth (zeroed page, no group
     fault). The SPT stays authoritative for residency. The bind engine
     re-uncommits grown-but-unbound tiles on the deferred worker (item 4).
   - **(b) Dummy zero sink.** Map unbound tiles to a shared, read-only zero
     region. Constraint: `MEM_ALIAS` cannot be FIXED (`mem_linux.c:1759`), so a
     sink cannot sit at a FIXED tile VA. This path works only if the S0 gate
     shows a usable placement. Otherwise (b) is not available.

   If neither (a) nor (b) passes, CSF uses eager backing under the item 2 cap.
   CSF uncommit does **no GPU zero fill**. kbase gives zeroed pages on recommit
   (`mem_pool.c:313`, verified CPU-side by S0 T10d). JM tiles stay committed,
   so JM unbind still zero-fills.
4. **Bind sync and hot-path copies.**
   - **Dirty tracking.** Each bound tile has a dirty bit. Submit-time marks it
     when the resource is reachable as a write target (storage/UAV, RT, ZS,
     copy/clear/resolve destination) in that submit. Read-only use (SRV,
     copy source) leaves it clean. Unbind writes back only dirty tiles. Bind
     copy-in is skipped when the source memory range has never been written.
   - **Deferred CPU uncommit worker.** Unbind never calls `MEM_COMMIT` inline.
     It queues `{region, tile, generation}` tagged with the bind batch's
     signal fence. One worker thread waits on the fence, then uncommits. A
     rebind of the same tile before the worker runs bumps the generation and
     cancels the stale entry. `VkDeviceMemory` and region frees also wait on
     that fence, which prevents use-after-free from in-flight GPU work.
5. **Prerequisites and order.**
   - **S-Pre:** `robustImageAccess2` on all arches (v10 already GPU-emulated,
     patch 184) and SM 6.2+ float controls (`VK_KHR_shader_float_controls`:
     fp16/fp32 denorm preserve/flush, rounding modes). These come before any
     sparse work.
   - **Census:** log D3D12 tiled-resource calls (`CreateReservedResource`,
     `UpdateTileMappings`, `CopyTileMappings`, `GetResourceTiling`) per target
     title. If no target title calls `UpdateTileMappings`, S1-S6 are
     deprioritized and the per-title override is the whole 12_0 story.
   - **S0 GPU kill-switch gate:** on real hardware, prove GPU read-0 from an
     unbound tile, bilinear/border sampling across a bound/unbound tile edge,
     and context survival (no group fault, no `DEVICE_LOST`, next submit
     succeeds) on CSF v10/v11 and JM v9 at least. A failure stops the plan
     before driver stages S1-S6.

Sources are cited as `file:line`. kbase kernel sources are the two Pixel
branches we can read in full: **r38p1** = `android-gs-pantah-5.10-android14`
(JM v9 + CSF v10, uAPI JM 11.x / CSF 1.x), **r52p0** =
`android-gs-shusky-6.1-android16` (CSF uAPI 1.33, `csf_ioctl.h:130`). Backend =
`mesa-115-rel/src/panfrost/lib/kmod/kbase_kmod.c`. vkd3d = master `e9f486a6`.

---

## Executive summary

**Verdict: real HW sparse residency (a stable GPU VA whose 64 KiB tiles are
independently (un)backed by physical pages) is achievable on every target, but
`sparseResidencyAliased` is not.** The stock kbase UAPI gives us no page-table
remap and no null-PTE, so the two capabilities split:

| Arch | Stable-VA tile (un)backing | How | `Aliased` |
|------|----------------------------|-----|-----------|
| v10-v13 (CSF uAPI >= 1.9) | **Yes, cheap** | `MEM_ALLOC_EX` FIXED per-64 KiB-tile `GROW_ON_GPF` region + `MEM_COMMIT` | No |
| v6/v7/v9 (JM), v14 (no ALLOC_EX) | **Yes, eager, capped** | one SAME_VA `GROW_ON_GPF` region per sparse resource, committed up-front, 256 MiB context cap | No |
| all | residency / MinLod / strict-non-resident | software page-table (SPT) bitmap + shader lowering | - |

`sparseResidencyAliased` stays **false** on every arch: it needs two VAs to map
the *same* physical tile, and kbase's only remap primitive (`MEM_ALIAS`) is
immutable after creation, always SAME_VA on 64-bit, and cannot be given a fixed
address (`mem_linux.c:1759` masks flags, `:1794` forces SAME_VA+cookie,
`mmap.c:449` "err on fixed address"). This is the same wall upstream hit
(panvk `sparseResidencyAliased = false`, `panvk_vX_physical_device.c:391`,
issue 237).

**Consequence for D3D12.** vkd3d computes its tiled tier itself from the Vulkan
caps (`device.c:8985-9007`): tier 1 and tier 2 both *require*
`sparseResidencyAliased`, so with Aliased=false vkd3d derives **tier 0** and FL
caps at 11_1 regardless of how much of the rest we implement. That derivation
is correct and we do not patch it. The honest tier is **Tier 1-minus-Aliased**.
FL 12_0 is reported only through an opt-in, per-title compatibility override
(Wine vkd3d 1.18 `VKD3D_CAPS_OVERRIDE=feature_level=12.0`; vkd3d-proton
`VKD3D_FEATURE_LEVEL=12_0`, `device.c:10035-10040`; it makes vkd3d-proton
report tier 2 to the app, and that report is not conformant). It is
set only for titles that probe the feature level and never call
`UpdateTileMappings` or alias tiles. It is never a global PanPlay default.

**Chosen design: "shadow-tile sparse."** Every sparse resource owns stable
physical backing at a stable GPU VA. `vkQueueBindSparse` is turned into GPU copy
work on the bind queue (copy-in on bind, write-back of dirty tiles on unbind,
zero-fill on JM only) plus a deferred, fence-gated commit/uncommit of the tile's
backing. We never move a VA and never rely on a page-table remap.
Residency/MinLod/non-resident-strict come from a software page-table (SPT)
bitmap plus NIR lowering of sparse opcodes.

**Effort: ~10-14 weeks** across S-Pre, census, S0 (CPU + GPU gate) and S1-S6.
The PanProbe `sparse_kbase` CPU test exists. The S0 GPU kill-switch gate must
pass on hardware before any driver stage starts.

**Biggest risks:** (1) JM/v14 backing is eager, so memory = sum of live reserved
resource sizes. The 256 MiB context cap makes large reservations fail
(`VK_ERROR_OUT_OF_DEVICE_MEMORY`) instead of exhausting the device; (2) CSF
non-resident reads (TMU footprints, border taps) can fault the group unless the
S0 GPU gate proves GPF growth or a sink path; (3) CSF per-tile regions multiply
kernel region count and fault-handler pressure; (4) vendor variance - v14
G1-Ultra rejects `ALLOC_EX` with ENOTTY (`kbase_kmod.c:2870-2883` fallback), so
it drops to the JM path; (5) the store filter adds 3-4 instructions per storage
write on arches lacking a null PTE.

---

## The chosen design in detail

### Backing (the "where do the pages live" problem)

**CSF uAPI >= 1.9 (v10, v11, v12, v13, G615):** a driver-owned FIXED window in
the FIXED_VA zone. Each sparse resource reserves a contiguous VA sub-range of
the window. Each 64 KiB tile is a **FIXED + GROW_ON_GPF** region allocated at
its exact tile VA (`MEM_ALLOC_EX`, `fixed_address` = tile VA,
`core_linux.c:1032-1047`). Bind commits the tile (`MEM_COMMIT` to 16 pages,
`mem_linux.c:2176-2262`); unbind uncommits to 0. Fresh pages are zeroed by the
kernel pool (`mem_pool.c:313` `__GFP_ZERO`), giving correct read-0 semantics for
free, so CSF uncommit issues **no GPU zero fill**. A stray GPU access to an
uncommitted tile faults. On CSF a fault on non-backed memory is fatal to the
group unless kbase services it as `GROW_ON_GPF` growth. The store filter (below)
covers writes only. Reads from TMU filter footprints and border taps are
handled by path (a) GPF growth or (b) dummy zero sink, chosen by the S0 GPU gate
(see "Council Round 1 corrections", item 3). The SPT keeps residency queries
honest. Memory cost ~= bound tiles + GPF-grown unbound tiles (until the worker
re-uncommits them) + per-region overhead, all under the context cap.

FIXED_VA zone math (verified `reg_track.h:96,102,109`): EXEC_VA base = 2^47,
EXEC_FIXED_VA = 4 GiB above it, FIXED_VA = `[2^47 + 8 GiB, 2^48)`. Base =
`0x8002_0000_0000`. The prototype uses R = `0x8004_0000_0000`. Requires
`va_bits >= 48` (from `RAW_MMU_FEATURES & 0xff`).

**JM v6/v7/v9 and v14 (no `MEM_ALLOC_EX`):** no FIXED, so each sparse resource
gets **its own SAME_VA `GROW_ON_GPF` region**, sized to the resource rounded to
64 KiB. Its CPU=GPU VA is fixed at mmap time and the region is freed with the
resource (after its last fence). There is no monolithic arena and no up-front
16 GiB VA reservation. Because kbase grows a GROW_ON_GPF region only from the
backed end upward (`mmu.c`, `MEM_COMMIT` shrinks/grows from the end only,
`mem_linux.c:2217-2260`), we cannot punch holes - so JM commits **eagerly**: a
reserved resource's tiles are backed at creation. This keeps the VA stable and
correct but spends real memory, so it is budgeted:
- A context-wide counter tracks committed sparse bytes (both paths).
- Cap = min(`PANVK_SPARSE_CAP_MB`, default 256 MiB, and the sparse share of
  the `VK_EXT_memory_budget` heap budget).
- `vkCreateBuffer` / `vkCreateImage` with sparse flags return
  `VK_ERROR_OUT_OF_DEVICE_MEMORY` when the reservation would exceed the cap.
  The counter is decremented only after the region free completes.

v14 uses this path too (ALLOC_EX ENOTTY). On this path the SPT is a
per-resource bitmap that the shader reaches through the descriptor's sparse
side data, because the regions are not contiguous. On CSF it stays one bitmap
over the FIXED window.

Why not the obvious alternatives, in short (full analysis in Q2):
- **FIXED + re-alias at the fixed VA** - impossible: aliases can't be FIXED
  (`mem_linux.c:1759` masks the flag) and FIXED only ever maps fresh private
  pages, never shared ones.
- **Alias rebuild + re-point** - not viable: a buffer's GPU VA / D3D12 BDA is
  baked into root arguments, indirect-draw buffers and app-side pointers, and
  descriptors are copied, so we cannot change the VA under a live resource.
- **Software page table only** - impossible for HW texture sampling; usable only
  for residency queries and write filtering (which is exactly what the SPT is).
- **SAME_VA munmap + recreate at the same VA** - racy against Wine/Box64
  reservations, ART and the in-process HWUI kbase context; a lost VA is a lost
  device. Rejected. (The prototype's T8 measures the real reuse rate.)

### Binding (turning `vkQueueBindSparse` into work)

The GPU queue family already advertises `SPARSE_BINDING`
(`panvk_physical_device.c:1579`); the kbase bind-queue path is still a stub
(`panvk_vX_device.c:443` "sparse binding queues are not implemented yet"). The
design replaces the stub with a bind engine that, per `VkSparseMemoryBind`:
- **bind:** cancel any pending uncommit of the tile (generation bump); commit
  the backing tile(s) if not committed; if the bound `VkDeviceMemory` range has
  ever been written, GPU-copy it into the tile, else skip the copy; set the SPT
  bit; clear the dirty bit.
- **unbind:** GPU-copy the tile back to its memory object **only if dirty**;
  clear the SPT bit. CSF: queue a deferred uncommit, with no GPU zero fill
  (recommit gives kernel-zeroed pages). JM: leave committed and GPU zero-fill
  the tile so later reads return 0.
- images without `INTERLEAVED_64K` (v6-v9, the modifier doesn't exist pre-v10,
  `panvk_async_bind.c:279-287`) copy tiles with image<->buffer region copies.

**Dirty tracking.** One dirty bit per bound tile. At submit, a command buffer
that can write the resource (storage/UAV, RT, ZS, copy/clear/resolve
destination) marks the reachable tiles dirty: exact tiles for copy regions,
all bound tiles of the resource otherwise. Read-only use (SRV, copy source)
does not mark tiles. A clean tile has no write-back cost on unbind.

**Deferred uncommit worker.** `MEM_COMMIT` and region/memory frees never run
inline in `vkQueueBindSparse`:
1. Unbind appends `{region, tile, generation}` to a list tagged with the bind
   batch's signal fence (syncobj/timeline point).
2. One worker thread per device waits on the fence, then uncommits each entry
   whose generation still matches.
3. A rebind before step 2 bumps the generation, so the stale entry is dropped.
4. `vkFreeMemory` and sparse resource destroy hold their regions until the last
   tagged fence signals. This prevents use-after-free from in-flight GPU work.

Bind copies are ordered on the queue like any other submit. Only the CPU-side
kbase calls move to the worker. Cost estimate: a 64 KiB GPU copy ~3-10 us,
`MEM_COMMIT` ~15-40 us off the submit thread (to be measured by T10f), a
realistic bind batch ~50-150 us before dirty skipping.

### Residency, MinLod, non-resident-strict (the SPT)

A **sparse page-table bitmap**: on CSF one bitmap over the FIXED window
(64 GiB / 64 KiB = 1 M bits = 128 KiB); on JM/v14 one bitmap per sparse
resource. One bit per tile = resident/not. It drives three things:
1. **`shaderResourceResidency`** - the `OpImageSparse*` residency code is
   computed from the SPT, not from HW (Mali has no residency feedback).
2. **`shaderResourceMinLod`** - lower via LOD query -> max -> explicit LOD /
   gradient scale. NIR already clamps `min_lod` (`nir_lower_tex.c:281-283`,
   `:685-687`), so this is a small extension.
3. **strict non-resident (`sparseResidencyNonResidentStrict`,
   `panvk_vX_physical_device.c:1109`)** - a store/atomic window check that
   discards writes to unbound tiles. Read-0 comes from zeroed pages on JM, and
   from GPF growth or the dummy sink on CSF (S0 GPU gate decides).
   Fixed-function RT/ZS/clear/resolve writes can't be filtered in-shader, so they
   get a post-pass zero-fill of non-resident tiles.

### What becomes true, per feature bit

After all stages: `sparseBinding`, `sparseResidencyBuffer`,
`sparseResidencyImage2D`, `residencyStandard2DBlockShape` (already true,
`:1111`), `sparseResidencyNonResidentStrict`, `shaderResourceResidency`,
`shaderResourceMinLod` -> all true. **`sparseResidencyAliased` stays false**,
so the D3D12 result is Tier 1-minus-Aliased, never conformant Tier 2.
`samplerFilterMinmax` / `filterMinmaxSingleComponentFormats` are already
`PAN_ARCH >= 10` (`:445`, `:1229`); v6-v9 need min/max filter emulation for the
12_0 feature set, independent of sparse.

---

## Q1 - Which stock-kbase mechanisms exist, and on which arches

| Mechanism | ioctl / flag | Can give a *stable fixed VA*? | Can *remap* which pages back a VA? | Arches |
|-----------|--------------|-------------------------------|-------------------------------------|--------|
| SAME_VA native alloc | `MEM_ALLOC` nr 5 | VA chosen by kernel at mmap, then fixed | no | all |
| **FIXED alloc** | `MEM_ALLOC_EX` nr 59, `BASE_MEM_FIXED` + `fixed_address` | **yes**, caller picks VA in FIXED_VA zone | no (fresh pages only) | CSF uAPI >= 1.9 (v10-v13); **not** v14 (ENOTTY) |
| FIXABLE alloc | `MEM_ALLOC_EX`, `BASE_MEM_FIXABLE` | VA assigned, then stable | no | CSF >= 1.9; mutually exclusive with FIXED per ctx (`core_linux.c:1020-1023`) |
| **per-tile commit** | `MEM_COMMIT` nr 20 | n/a (changes backing *size* of one region) | grows/shrinks **from the end only** | all (native, growable, `gpu_mappings<=1`, not DONT_NEED/NO_USER_FREE: `mem_linux.c:2183-2215`) |
| **GROW_ON_GPF** | `BASE_MEM_GROW_ON_GPF` (needs `extension != 0`, `mem.c:2271`) | n/a | backs pages lazily on fault, from backed end to fault | all |
| **MEM_ALIAS + sink** | `MEM_ALIAS` nr 21 | **no** - always SAME_VA cookie on 64-bit (`mem_linux.c:1794`), FIXED flag masked off (`:1759`) | **no** - alias contents are immutable after creation | all; max `BASE_MEM_ALIAS_MAX_ENTS` ents (`core_linux.c:1299`) |
| JIT | `MEM_JIT_INIT` nr 14 | kernel-placed | no | arch >= 9; <=255 ids, regions are NO_USER_FREE and **cannot be aliased** (`mem_linux.c:1866` "JIT regions can't be aliased") |
| EXEC_VA | `MEM_EXEC_INIT` nr 38 | zone only, GPU_EX, **not GPU_WR** | no | all 64-bit |
| USER_BUFFER / UMM import | `MEM_IMPORT` nr 22 | no (SAME_VA) | no; **cannot be FIXED** (`mem.c:2211`) nor GROW_ON_GPF (`:2207`/r38 `:3325,3330`) | all |
| FLAGS_CHANGE | `MEM_FLAGS_CHANGE` nr 23 | no | only toggles DONT_NEED / coherency (backend `kbase_kmod.c:1762-1786`) | all |

**The sink page.** `MEM_ALIAS` accepts a magic source handle
`BASEP_MEM_WRITE_ALLOC_PAGES_HANDLE` (= `4 << 12`, `mem_linux.c:1829`): those
alias entries map a single per-context zeroed page
(`context.c:382 aliasing_sink_page`, `mem.c:396`) with WRITE_ALLOC memattr
(`mem.c:363`). GPU reads of a sink entry return 0; GPU writes land in the one
shared page and are visible from *every* sink-mapped VA (not isolated). A CPU
"dumping" mmap of a sink entry is refused (`get_aliased_alloc` returns NULL for
`!aliased->alloc`, `mem_linux.c:2403-2406`). This is what csf-v11 patch 230 uses
for sparse TLS (proven on v10/v11) - but it is read-0 / write-shared, not real
residency.

**Alias source constraints** (`mem_linux.c:1856-1902`): sources must be NATIVE,
GPU-cached, coherency-matched, not shrinkable, not NO_USER_FREE; they get pinned
via `kbase_mem_phy_alloc_gpu_mapped` (`:1902`), which also blocks a later
`MEM_COMMIT` on them (`gpu_mappings > 1`). UMM/dma-buf imports cannot be alias
sources (not NATIVE) - and our non-exclusive BOs are dma-buf imports when
`/dev/dma_heap` exists (`kbase_kmod.c:2199-2226`).

---

## Q2 - Can we hold a stable GPU VA while changing which pages back each tile?

**Yes on CSF (real per-tile), yes-but-eager on JM; never with aliasing.** The
four candidate designs:

**(a) FIXED window + re-alias each tile at its fixed VA.** *Impossible.* To share
physical pages between a resource VA and a backing store you need `MEM_ALIAS`,
but aliases are forced SAME_VA with a kernel cookie on 64-bit
(`mem_linux.c:1794`) and the FIXED flag is masked off before placement
(`:1759`). You cannot alias *into* a FIXED VA. And FIXED by itself only ever maps
fresh private pages - no sharing, no remap.

**(b) Alias rebuild + re-point.** *Not viable for buffers, maybe for images with
a relocation layer.* We could rebuild a resource's `MEM_ALIAS` to point at
different source BOs, but each rebuild yields a *new* kernel-chosen VA. A D3D12
buffer's GPU VA is its BDA, baked into root arguments, indirect buffers and
app-visible pointers, and descriptors are copied at record time - we can't move
it. Prototype T11 bounds the rebuild cost for completeness.

**(c) Software page table only.** *Impossible for sampling* - the Mali TMU reads
real page tables we can't touch. Usable only for residency queries + store
filtering, which is the SPT's actual job.

**(d) Real per-tile (un)backing of one stable region.** *This is the design.*
- **(d-CSF)** FIXED + GROW_ON_GPF per tile, `MEM_COMMIT` to (un)back. Prototype
  T10 proves alloc-at-R, RW readback, overlap-refused, free+realloc-at-same-VA,
  and the commit 16->0->16 cycle with zeroed recommit.
- **(d-JM)** one SAME_VA GROW_ON_GPF region per sparse resource; commit grows
  from the end only (`mem_linux.c:2217`), so holes aren't possible -> eager
  backing under the 256 MiB context cap. Prototype T9 proves the
  end-growth/shrink behaviour; T8 proves why munmap+recreate is not a safe
  substitute.

**Non-resident semantics without a null PTE.** Mali has no null/sparse PTE
(Panthor's upstream sparse support maps a dummy BO, Collabora LKML 2026). So:
read-0 comes from zeroed pages (`mem_pool.c:313`) on JM; write-discard needs the
in-shader store window check; fixed-function writes need the post-pass
zero-fill. On CSF, a stray access to an uncommitted tile is a fatal group fault
unless kbase services it as GPF growth. The store filter covers writes only.
Reads (including TMU filter footprints and border taps that cross into an
unbound tile) need path (a) GPF growth or (b) dummy zero sink, proven by the S0
GPU kill-switch gate. If neither passes, CSF falls back to eager backing.

---

## Q3 - Per-arch verdict, cost, and what PanProbe must prove

### Per-arch verdict

- **v10/v11 (CSF, G710/G715/G615), v12/v13 (CSF r49p1):** full design. FIXED
  per-tile backing is cheap. These are the primary targets. v10 also carries the
  current GPU-hang work - the sparse bind engine must not collide with it
  (sequence sparse work behind the hang fix).
- **v6 (G72), v7 (G52), v9 (G78) (JM):** design works via eager per-resource
  SAME_VA regions under the context cap. Extra 12_0 gaps unrelated to sparse: no HW min/max filter (emulate),
  and v6/v7 need 64 storage images per stage for 11_1 (reachability table,
  `worklogs/.../d3d-feature-levels-2026-10-27`). No INTERLEAVED_64K -> image
  tiles via buffer copies.
- **v14 (G1-Ultra, MT6993):** rejects `ALLOC_EX` (ENOTTY,
  `kbase_kmod.c:2870`), so no FIXED -> falls back to JM eager per-resource
  regions. Treat as JM for sparse. Highest vendor-variance risk.

### Cost estimates (to be confirmed by the prototype on-device)

| Operation | Estimate |
|-----------|----------|
| 64 KiB GPU tile copy | 3-10 us |
| `MEM_COMMIT` one tile | 15-40 us (T10f measures) |
| realistic bind batch | 50-150 us |
| in-shader store filter | +3-4 instr / storage write |
| SPT | 128 KiB for a 64 GiB window |

### Staged plan (~10-14 weeks, in this order)

Superseded by the Round 3 authoritative plan. S1-S6 below run only on Path 3B
(CSF v10-v14, Bucket B titles); on Path 3A they are archived. S-Pre = Phase 0,
Census = Phase 1, S0-GPU = Phase 2. S6 (JM extras) is not sparse work and
moves out of this plan.

| Stage | What | Effort |
|-------|------|--------|
| **S-Pre** | `robustImageAccess2` on all arches (v10 GPU-emulated, patch 184) + SM 6.2+ float controls (`VK_KHR_shader_float_controls`) | 1-2 wk |
| **Census** | log `CreateReservedResource` / `UpdateTileMappings` / `CopyTileMappings` / `GetResourceTiling` per target title; decides whether S1-S6 are needed | 1-2 d |
| **S0-CPU** | PanProbe `sparse_kbase` test (CPU-side kbase primitives, exists) | done |
| **S0-GPU** | **kill-switch gate**: on HW, GPU read-0 from an unbound tile, bilinear/border sampling across a bound/unbound edge, context survival; picks CSF path (a) GPF growth, (b) sink, or eager. Fail = stop before S1 | 3-5 d |
| **S1** | bind engine (dirty tracking, deferred fence-gated uncommit worker) + per-resource SAME_VA backing + 256 MiB context cap, all arches | 2-3 wk |
| **S2** | CSF FIXED per-tile backing, no GPU zero on uncommit, non-resident read path from S0-GPU | ~1 wk |
| **S3** | SPT + residency + MinLod lowering | 1.5-2 wk |
| **S4** | strict write filter + fixed-function zero-fill | 1.5-2 wk |
| **S5** | per-title opt-in FL override in PanPlay profiles (census-listed titles only; Wine vkd3d 1.18 `VKD3D_CAPS_OVERRIDE=feature_level=12.0`, vkd3d-proton `VKD3D_FEATURE_LEVEL=12_0`); no vkd3d tier patch | 0.5-1 d |
| **S6** | v6-v9 extras (min/max emulation, storage-image count) | 1-2 wk |

The per-title override (S5) does not depend on S1-S4 and can ship right after
the census, but only for titles that never call `UpdateTileMappings` or alias
tiles.

### What the PanProbe `sparse_kbase` test must prove (device, S0 gate)

1. Caps: CSF uAPI >= 1.9 and `va_bits >= 48` where FIXED is claimed.
2. GPU store+load at a FIXED VA, plus CPU readback (prototype T10a).
3. GPU load from an uncommitted GROW_ON_GPF FIXED tile returns 0; store-then-load
   returns the store; the context survives (T10d is the CPU-side half).
4. Per-tile uncommit isolation under concurrent access to neighbours.
5. Per-resource region copy-in/out is bit-exact (JM path).
6. Sink reads 0 and a write leaks to other sink entries (T2-T6 CPU half).
7. SAME_VA re-placement reuse rate (T8) - confirms munmap+recreate is unsafe.
8. Timings (bind batch, commit) on each arch.
9. Stress: 16384 tiles, 10k bind/unbind cycles, no leak, no DEVICE_LOST.

The existing PanPlay `d3d12fl` `tiled_tier2` probe (128 KiB reserved buffer, tile
0 bound / tile 1 unbound, raw stores to both; expects bound=0x1234,
unbound-read=0) passes only **after S4** (it needs write-discard). Proposed new
PanPlay cases: `tiled_texture`, `tile_remap`, `reuse_single_tile`.

---

## Prototype

`tools/sparse-kbase-proto/` - a single C file, bare NDK clang, no Gradle/Mesa.
Build: `tools/sparse-kbase-proto/build.sh` (uses NDK
`/opt/android-sdk/ndk/29.0.14206865`, aarch64 Android 29, builds under
`flock /home/flux/build-cache/build.lock`, then runs `--selftest` on the host).
It performs the backend's open/version-check/SET_FLAGS/tracking-page/gpuprops/
EXEC_INIT/JIT_INIT handshake, then runs T1-T11 (native heaps; MEM_ALIAS with the
sink and its dumping-map / sink-refusal / commit-refusal rules; FIXED-flag
masking; SAME_VA re-placement; GROW_ON_GPF commit; FIXED-VA alloc + per-tile
commit cycle + rebind-at-same-VA; alias-rebuild timing), printing PASS/FAIL/
SKIP/INFO with `sigsetjmp` guards around every GPU-backed read. **Do not run it
on a user device inside an automated job** (it provokes GPU faults and maps
foreign memory on purpose); run by hand and read the log. `--selftest` (parser +
VA math, no ioctls) is safe anywhere and is run at build time.

Note: this implementation needs
`tests/dxvk-vkd3d/test_p22_kbase_sparse_feasibility.py` (which currently asserts
the sparse bits stay false) and `docs/KBASE-SPARSE-FEASIBILITY.md` (the earlier
NO-GO) to be updated when S1+ lands.

---

## S0 PanProbe test (`sparse_kbase`)

The prototype's T1-T11 ported into PanProbe as an unattended device test.
Sources: `tests/kbase-probe/sparse_kbase.c` (the test),
`tests/kbase-probe/sparse_kbase.h` (verdict logic and gpuprops parser, pure),
`tests/kbase-probe/sparse_kbase_test.c` (host unit test of the verdict:
`cc -O1 -o /tmp/sparse_kbase_test tests/kbase-probe/sparse_kbase_test.c && /tmp/sparse_kbase_test`).
Built as `libt_sparse_kbase.so` (`add_vk_test` in `apps/panvk-test/app/src/main/cpp/CMakeLists.txt`)
and run out of process through `libpt_runner.so` (`Native.run`), like every suite test.

**Wiring.** It is not a suite test. `runHeadlessAutorun` runs it once per
autorun, after the suite: once in scenario 2 (rc5 only), only in the first env
pass of scenario 3, only in the rc5 phase of scenario 1, and in an adb
`--es autorun all` (or `--es autorun sparse_kbase` alone). It is informational
like the compliance checkers, so its SKIP/FAIL/CRASH/TIMEOUT never counts toward
the run's pass/fail. The 90 s outer timeout is a backstop. The test bounds itself
far below that.

**Safety model.** The test opens `/dev/mali0` itself, so it gets its own kbase
context. The test process never loads the Vulkan driver. It uses CPU-side
ioctls only (VERSION_CHECK, SET_FLAGS, GET_GPUPROPS, MEM_ALLOC, MEM_ALLOC_EX,
MEM_QUERY, MEM_COMMIT, MEM_ALIAS, MEM_FREE). It submits **no GPU job**, so it
cannot provoke a GPU fault. This README marks no GPU step as safe. Other bounds:
- At most 16 MiB is committed at once, and every allocation is freed. `close()`
  frees anything a step left behind.
- Every CPU touch of kbase memory is guarded against SIGSEGV/SIGBUS.
- Each step has a 6 s SIGALRM watchdog. On expiry or an unguarded signal, the step
  FAILs and the later steps SKIP. Loops also stop on their own 1.5-3 s deadlines.
- If an ioctl is missing (ENOTTY/ENOSYS/EOPNOTSUPP), the step SKIPs with the reason.

**Prototype bugs fixed in the port.** The prototype never ran on a device. These
fixes follow the r38p1 source:
- `BASE_MEM_NEED_MMAP` is an output-only flag. Passed as input it fails
  `kbase_check_alloc_flags` with EINVAL, so the port uses the backend's BO flags
  (`CPU_RD|CPU_WR|GPU_RD|GPU_WR|COHERENT_LOCAL|SAME_VA`).
- The prototype's T9 never mmapped its cookie, so its commit/query hit a cookie
  instead of a VA.
- The prototype's T3 munmapped (and so freed) the alias before T4. A per-entry
  ("dumping") map also needs the alias **VA** as the offset, not cookie+N.
  `get_aliased_alloc`, `mem_linux.c:2410`.
- The prototype's T6 resized a non-growable source, which is refused for being
  non-growable. The port aliases a growable H1, and checks that the commit is
  refused while aliased and accepted after the alias is freed.
- The port skips EXEC_INIT and JIT_INIT (no step needs them). It keeps the
  tracking page, because kbase refuses allocations until that page is mapped.
- No `CACHED_CPU`: CPU views are write-combined, so an alias reads what its
  source BO's own mapping wrote.

### What each step checks, and the expected result per arch

The expectations come from the kbase sources and are **not yet device-verified**.

| Step | Checks | v10-v13 (CSF >= 1.9) | v6/v7/v9 (JM) | v14 (CSF, no ALLOC_EX) |
|------|--------|----------------------|---------------|------------------------|
| setup | open, VERSION_CHECK (CSF nr 52, else JM nr 0), SET_FLAGS, tracking page, gpu_id/va_bits | PASS CSF 1.x | PASS JM 11.x | PASS CSF 1.x |
| T1 | SAME_VA BO H0 (64 KiB) CPU write/readback; growable H1 (GROW_ON_GPF, 64 of 128 KiB) | PASS x2 | PASS x2 | PASS x2 |
| T2 | MEM_ALIAS [H0, sink, H1, sink], stride 16 pages | PASS | PASS | PASS |
| T3 | cookie mmap of the alias; entry 0 reads H0's pages | PASS | PASS | PASS |
| T4 | per-entry mmap (offset = alias VA + 2 tiles) reads H1's pages | PASS | PASS | PASS |
| T5 | per-entry mmap of a sink entry is refused (sink is GPU-only) | PASS | PASS | PASS |
| T6 | MEM_COMMIT on an aliased source refused, accepted after the alias is freed | PASS | PASS | PASS |
| T7 | alias with BASE_MEM_FIXED: rejected or FIXED dropped | PASS | SKIP (CSF flag) | PASS |
| T8 | 64x SAME_VA alloc+munmap, VA reuse count | PASS (+count) | PASS (+count) | PASS (+count) |
| T9 | eager arena: SAME_VA GROW_ON_GPF 16 MiB VA, commit grows 16->1024 pages one tile at a time and shrinks back | PASS + BENCH | PASS + BENCH | PASS + BENCH |
| T10a | MEM_ALLOC_EX FIXED at R=0x800400000000 lands at R (INFO: CPU RW via mmap(offset=VA)) | PASS | SKIP (JM) | SKIP (ALLOC_EX ENOTTY) |
| T10b | overlapping FIXED alloc refused | PASS | - | - |
| T10c | free + re-alloc at the same FIXED VA | PASS | - | - |
| T10d | growable FIXED tile commit 0->16->0->16 takes effect (MEM_QUERY), recommitted page reads 0 | PASS | - | - |
| T10e | FIXABLE refused while FIXED is live (INFO if a vendor kernel allows it) | PASS/INFO | - | - |
| T10f | 256 contiguous FIXED tiles x 8 bind/unbind cycles, no commit errors, all back at floor | PASS + BENCH | SKIP | SKIP |
| T11 | 24-entry alias rebuilt 256x (cost of the rejected re-point design) | PASS + BENCH | PASS + BENCH | PASS + BENCH |
| VERDICT | | **lazy-tile possible** | **eager-only** | **eager-only** |

**Verdict rules** (`sk_verdict`, unit-tested):
- *lazy-tile possible* = T10a and T10c and T10d all proven.
- Otherwise *eager-only* = T1 proved a SAME_VA BO.
- Otherwise *not possible*. This includes no `/dev/mali0` and an unusable handshake.

T9 does not decide the verdict. The verdict line only says whether the arena can
grow from the end; if not, eager falls back to fixed-size arenas. If T10d's
commit 0 is refused, the port retries with commit 1, so an unbound tile keeps 1
page (`16->1->16` in the log). If the uncommit is ignored (a
`CONFIG_MALI_MEMORY_FULLY_BACKED` kernel), the result is eager-only.

**Not covered by S0-CPU here.** The plan's S0 items 2-5 need GPU access (GPU
store/load at a FIXED VA, read-0 from an uncommitted tile, neighbour isolation,
copy bit-exactness). They move to the **S0-GPU kill-switch gate**, which runs
before S1, not after it. It adds boundary sampling (bilinear and border taps
across a bound/unbound tile edge) and context survival (no group fault, no
`DEVICE_LOST`, a follow-up submit succeeds). It needs only a minimal test-only
driver hook that binds one FIXED `GROW_ON_GPF` region as a texture/texel buffer,
not the S1 bind engine. Item 9 (16384 tiles, 10k cycles) runs bounded here: 256 tiles x 8
cycles within 3 s.

### How to read the log

- `files/autorun.txt` and logcat tag `PanVKTest` carry every test line, prefixed
  `SPARSE_KBASE `, then `SPARSE_KBASE status=<PASS|FAIL|SKIP|CRASH|TIMEOUT> ms=<n>`.
  Logcat tag `PanProbe` also gets `PANPROBE_SPARSE status=... VERDICT ...`.
- The run folder (in the zip and the upload) has `<time>-sparse_kbase.log`, the
  full test output. `summary.json` gets `"sparse_kbase": {status, verdict, ms,
  bench, log, informational: true}`, and so does the game-loop results JSON
  (`results_scenario_N.json`).
- Line format:
  - `PASS|FAIL|SKIP <step> ...` per check, and `INFO ...`.
  - `BENCH sparse_arena_commit=..us sparse_arena_uncommit=..us`: T9, per 64 KiB tile.
  - `BENCH sparse_tile_bind=..us sparse_tile_unbind=..us sparse_tile_reserve=..us sparse_tile_free=..us`:
    T10f. Bind = commit 0->16 and unbind = commit 16->0 of one FIXED tile.
    Reserve and free are the FIXED alloc and free.
  - `BENCH sparse_alias_rebuild24=..us`: T11.
  - `VERDICT v<arch> <lazy-tile possible|eager-only|not possible> -- <why>`.
  - `SUMMARY pass= fail= skip=`.
- Status `SKIP` means `/dev/mali0` was unusable (`RESULT SKIP <reason>`). `FAIL`
  means at least one check failed. Read the FAIL lines, not just the status: the
  verdict can still be correct, for example a FAIL on T5 does not affect it.
- Compare the BENCH numbers with the cost table above (MEM_COMMIT per tile:
  estimate 15-40 us). A realistic bind batch of 16 tiles is about 16 x `sparse_tile_bind`.
