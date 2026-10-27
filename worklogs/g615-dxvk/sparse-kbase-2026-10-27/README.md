# Real sparse residency on stock kbase (Mali v6-v14) - feasibility and design

2026-10-27. Research + design only. No kernel changes (stock phones, no root).
Goal: real Vulkan sparse residency on the Android ARM DDK (`/dev/mali0`, the
`kbase` UAPI) so vkd3d-proton can expose D3D12 tiled resources tier 2 and PanVK
can reach feature level 12_0. Scope: JM v6 (G72), v7 (G52), v9 (G78); CSF v10
(G710), v11 (G715/G615), v12-v14.

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
| v6/v7/v9 (JM), v14 (no ALLOC_EX) | **Yes, eager** | one SAME_VA `GROW_ON_GPF` arena, sub-allocated, committed up-front | No |
| all | residency / MinLod / strict-non-resident | device-global software page-table bitmap + shader lowering | - |

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
caps at 11_1 regardless of how much of the rest we implement. Reaching 12_0
therefore needs the existing `VKD3D_FEATURE_LEVEL=12_0` override
(`device.c:10035-10040`, forces tier 2) wired into a PanPlay profile, or a
small opt-in vkd3d patch. The honest tier is 1-minus-Aliased.

**Chosen design: "shadow-tile sparse."** Every sparse resource owns stable
physical backing at a stable GPU VA. `vkQueueBindSparse` is turned into GPU copy
work on the bind queue (copy-in on bind, write-back + zero on unbind) plus a
commit/uncommit of the tile's backing. We never move a VA and never rely on a
page-table remap. Residency/MinLod/non-resident-strict come from a device-global
software page-table (SPT) bitmap plus NIR lowering of sparse opcodes.

**Effort: ~8-11 weeks** across 7 stages (S0-S6 below). The first device-provable
milestone (PanProbe `sparse_kbase`) is **2-3 days** and gates the rest.

**Biggest risks:** (1) JM/v14 arena is eager, so VRAM = sum of live reserved
resource sizes - a 1 GiB reserved tiled texture costs ~1 GiB even mostly
unbound; (2) CSF per-tile regions multiply kernel region count and fault-handler
pressure; (3) vendor variance - v14 G1-Ultra rejects `ALLOC_EX` with ENOTTY
(`kbase_kmod.c:2870-2883` fallback), so it drops to the JM arena path; (4) the
store-filter adds 3-4 instructions per storage write on arches lacking a null
PTE.

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
free. Stray GPU access to an uncommitted tile faults; on CSF a fault on
non-backed memory is fatal to the group, so the shader-side store filter (below)
must prevent it, and the SPT makes residency queries honest. Memory cost ~=
bound tiles + per-region overhead.

FIXED_VA zone math (verified `reg_track.h:96,102,109`): EXEC_VA base = 2^47,
EXEC_FIXED_VA = 4 GiB above it, FIXED_VA = `[2^47 + 8 GiB, 2^48)`. Base =
`0x8002_0000_0000`. The prototype uses R = `0x8004_0000_0000`. Requires
`va_bits >= 48` (from `RAW_MMU_FEATURES & 0xff`).

**JM v6/v7/v9 and v14 (no `MEM_ALLOC_EX`):** no FIXED, so one big SAME_VA
`GROW_ON_GPF` "arena" region (e.g. 16 GiB VA) whose CPU=GPU VA is fixed at mmap
time. Sparse resources are sub-allocated inside it at stable offsets. Because
kbase grows a GROW_ON_GPF region only from the backed end upward (`mmu.c`,
`MEM_COMMIT` shrinks/grows from the end only, `mem_linux.c:2217-2260`), we cannot
punch holes - so JM commits **eagerly**: a reserved resource's tiles are backed
up front. This keeps the VA stable and correct but spends real memory. v14 uses
this path too (ALLOC_EX ENOTTY).

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
- **bind:** commit the backing tile(s); if the bound `VkDeviceMemory` already
  holds data, GPU-copy it into the tile; set the SPT bit.
- **unbind:** GPU-copy the tile back to its memory object if needed, zero the
  tile, uncommit (CSF) or leave committed (JM arena), clear the SPT bit.
- images without `INTERLEAVED_64K` (v6-v9, the modifier doesn't exist pre-v10,
  `panvk_async_bind.c:279-287`) copy tiles with image<->buffer region copies.

Bind is ordered on the queue like any other submit, so no extra sync model is
needed. Cost estimate: a 64 KiB GPU copy ~3-10 us, `MEM_COMMIT` ~15-40 us (to be
measured by the prototype T10f), a realistic bind batch ~50-150 us.

### Residency, MinLod, non-resident-strict (the SPT)

A device-global **sparse page-table bitmap** over the window: 64 GiB / 64 KiB =
1 M bits = 128 KiB. One bit per tile = resident/not. It drives three things:
1. **`shaderResourceResidency`** - the `OpImageSparse*` residency code is
   computed from the SPT, not from HW (Mali has no residency feedback).
2. **`shaderResourceMinLod`** - lower via LOD query -> max -> explicit LOD /
   gradient scale. NIR already clamps `min_lod` (`nir_lower_tex.c:281-283`,
   `:685-687`), so this is a small extension.
3. **strict non-resident (`sparseResidencyNonResidentStrict`,
   `panvk_vX_physical_device.c:1109`)** - a store/atomic window check that
   discards writes to unbound tiles (read-0 comes free from zeroed pages).
   Fixed-function RT/ZS/clear/resolve writes can't be filtered in-shader, so they
   get a post-pass zero-fill of non-resident tiles.

### What becomes true, per feature bit

After all stages: `sparseBinding`, `sparseResidencyBuffer`,
`sparseResidencyImage2D`, `residencyStandard2DBlockShape` (already true,
`:1111`), `sparseResidencyNonResidentStrict`, `shaderResourceResidency`,
`shaderResourceMinLod` -> all true. **`sparseResidencyAliased` stays false.**
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
- **(d-JM)** one SAME_VA GROW_ON_GPF arena; commit grows from the end only
  (`mem_linux.c:2217`), so holes aren't possible -> eager backing. Prototype T9
  proves the end-growth/shrink behaviour; T8 proves why munmap+recreate is not a
  safe substitute.

**Non-resident semantics without a null PTE.** Mali has no null/sparse PTE
(Panthor's upstream sparse support maps a dummy BO, Collabora LKML 2026). So:
read-0 comes from zeroed fresh pages (`mem_pool.c:313`); write-discard needs the
in-shader store window check; fixed-function writes need the post-pass
zero-fill. On CSF, an *unfiltered* stray access to an uncommitted tile is a fatal
group fault - the filter is mandatory there, not just for strictness.

---

## Q3 - Per-arch verdict, cost, and what PanProbe must prove

### Per-arch verdict

- **v10/v11 (CSF, G710/G715/G615), v12/v13 (CSF r49p1):** full design. FIXED
  per-tile backing is cheap. These are the primary targets. v10 also carries the
  current GPU-hang work - the sparse bind engine must not collide with it
  (sequence sparse work behind the hang fix).
- **v6 (G72), v7 (G52), v9 (G78) (JM):** design works via the eager SAME_VA
  arena. Extra 12_0 gaps unrelated to sparse: no HW min/max filter (emulate),
  and v6/v7 need 64 storage images per stage for 11_1 (reachability table,
  `worklogs/.../d3d-feature-levels-2026-10-27`). No INTERLEAVED_64K -> image
  tiles via buffer copies.
- **v14 (G1-Ultra, MT6993):** rejects `ALLOC_EX` (ENOTTY,
  `kbase_kmod.c:2870`), so no FIXED -> falls back to the JM eager arena. Treat as
  JM for sparse. Highest vendor-variance risk.

### Cost estimates (to be confirmed by the prototype on-device)

| Operation | Estimate |
|-----------|----------|
| 64 KiB GPU tile copy | 3-10 us |
| `MEM_COMMIT` one tile | 15-40 us (T10f measures) |
| realistic bind batch | 50-150 us |
| in-shader store filter | +3-4 instr / storage write |
| SPT | 128 KiB for a 64 GiB window |

### Staged plan (~8-11 weeks)

| Stage | What | Effort |
|-------|------|--------|
| **S0** | PanProbe `sparse_kbase` test (proves the primitives on real HW) | 2-3 d |
| **S1** | bind engine + arena backing, all arches | 2-3 wk |
| **S2** | CSF FIXED per-tile backing | ~1 wk |
| **S3** | SPT + residency + MinLod lowering | 1.5-2 wk |
| **S4** | strict write filter + fixed-function zero-fill | 1.5-2 wk |
| **S5** | vkd3d tier policy + PanPlay 12_0 profile | 0.5-1 d |
| **S6** | v6-v9 extras (min/max emulation, storage-image count) | 1-2 wk |

### What the PanProbe `sparse_kbase` test must prove (device, S0 gate)

1. Caps: CSF uAPI >= 1.9 and `va_bits >= 48` where FIXED is claimed.
2. GPU store+load at a FIXED VA, plus CPU readback (prototype T10a).
3. GPU load from an uncommitted GROW_ON_GPF FIXED tile returns 0; store-then-load
   returns the store; the context survives (T10d is the CPU-side half).
4. Per-tile uncommit isolation under concurrent access to neighbours.
5. Arena copy-in/out is bit-exact (JM path).
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

**Not covered by S0 here.** The plan's S0 items 2-5 need GPU access (GPU
store/load at a FIXED VA, read-0 from an uncommitted tile, neighbour isolation,
arena copy bit-exactness). They wait for a driver-side test once the S1 bind
engine exists. Item 9 (16384 tiles, 10k cycles) runs bounded here: 256 tiles x 8
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
