# Gemini 4 Neon — D3D12 FL 12_0 Architecture Council Review (Round 2)

**Verdict: Conditional Approve (S-Pre, Title Census, S0-GPU Gate) / Hold (S1–S6) — Score: 6.5 / 10**

*(Scope note: GitHub returned empty client-side shells for the two linked `.md` blobs, so this review evaluates the Round 2 architecture summary above directly against `vkd3d-proton` capability gating, upstream Mesa PanVK, and Android `mali_kbase` kernel internals.)*

Round 2 is a major governance upgrade over Round 1: moving `robustImageAccess2` and SM 6.2+ float controls into **S-Pre**, eliminating the 16 GiB eager arena, dropping the false Tier 2 claim, and gating S1–S6 behind both a **Title Census** and an **S0-GPU hardware kill-switch** are the right engineering calls. However, while Round 2 fixed Round 1's *process and honesty* failures, **four fundamental technical contradictions remain inside the S1–S6 shadow-tile design** that the council's 7.19 average appears to under-weight.

---

## Critical Findings Remaining in Round 2

### 1. The FL 12_0 Catch-22: Dropping `sparseResidencyAliased` Hollows Out S1–S6's ROI

Conceding "Tier 1-minus-Aliased" (`sparseResidencyAliased = false`) fixes the spec-fraud blocker, but it breaks the causal link between S1–S6 and native FL 12_0:

* In `vkd3d-proton`, `d3d12_device_caps_init_feature_level()` requires **`TiledResourcesTier >= D3D12_TILED_RESOURCES_TIER_2`** (plus `ResourceBindingTier >= 2`) to expose `D3D_FEATURE_LEVEL_12_0` natively.
* `d3d12_device_determine_tiled_resources_tier()` drops all the way to **`D3D12_TILED_RESOURCES_TIER_NOT_SUPPORTED`** unless *all* of `sparseBinding`, `sparseResidencyBuffer`, `sparseResidencyImage2D`, `sparseResidencyAliased`, `residencyStandard2DBlockShape`, and a sparse-binding queue are present—and Tier 2 additionally requires `residencyNonResidentStrict` (zero-read / silent-drop-write on unbound tiles) and `shaderResourceResidency`.
* **Consequence:** Even after shipping all six driver stages (S1–S6), `vkd3d-proton` will **still report `TILED_RESOURCES_TIER_NOT_SUPPORTED` and cap native FL at 11_1**. Per-title `VKD3D_FEATURE_LEVEL=12_0` / `VKD3D_CAPS_OVERRIDE` overrides remain mandatory either way.
* That collapses S1–S6's addressable universe to a razor-thin sliver: titles that (1) require an FL 12_0 override, (2) actually invoke `CreateReservedResource` / `UpdateTileMappings`, (3) never alias two virtual tiles to one heap offset (ruling out fallback/zero-tile aliasing and heap repacking), (4) never rely on Tier 2 residency feedback (`CheckAccessFullyMapped`), and (5) fit inside a 256 MiB sparse cap. Titles that only check FL 12_0 at `D3D12CreateDevice` are already solved by **S-Pre + per-title override** with zero sparse driver code.

### 2. `mali_kbase` Prefix-Only Commits Break Per-Resource `SAME_VA` Memory Economics

Replacing the 16 GiB arena with per-resource `SAME_VA` regions capped at 256 MiB (`PANVK_SPARSE_CAP_MB`) does **not** actually achieve sparse memory consumption on stock `mali_kbase`:

* Every `kbase_va_region` is backed by a single dense prefix array (`reg->gpu_alloc->pages[0 .. nents - 1]`). Both `KBASE_IOCTL_MEM_COMMIT` and `kbase_mmu_page_fault_worker()` (`reg_grow_calc_extra_pages(kbdev, region, fault_rel_pfn)`) only grow or shrink a **contiguous prefix `[0 .. high_water_mark)`**—they cannot commit page 4000 while leaving pages 0..3999 unbacked.
* **The Mip-Tail Memory Bomb:** In Mali's standard hardware mipmap layout, mip 0 occupies offset `0` (largest) while the **packed mip tail** sits at the **highest offset** of the `VkImage`. The first operation of virtually every D3D12 texture streamer after `CreateReservedResource` is binding the 64 KiB mip tail so the texture is immediately sampleable. On `mali_kbase`, committing that single 64 KiB tail tile forces physical allocation of **100% of the texture's VA range** (all mips below it).
* Four 64 MiB streaming textures with *only* their mip tails bound will immediately commit 256 MiB and exhaust `PANVK_SPARSE_CAP_MB`, triggering `VK_ERROR_OUT_OF_DEVICE_MEMORY` on a workload that only bound 256 KiB of real tile data.
* *(For the council record: `KBASE_IOCTL_MEM_ALIAS` also cannot rescue this, because `kbase_mem_alias` creates an immutable VA region with fixed stride at creation time and cannot rebind tiles in-place at a stable GPU VA as required by `vkQueueBindSparse` and bindless descriptors—unlike upstream `panthor`'s `DRM_PANTHOR_VM_BIND`.)*

### 3. Shadow-Tile Dirty Tracking & Layout Costs Under VKD3D-Proton Bindless

Round 2's Blocker #4 resolution (*"per-tile dirty bits; deferred CPU uncommit worker gated on bind fence with generation counter"*) has four unresolved correctness/performance holes:

* **Bindless defeats per-tile dirty tracking:** `vkd3d-proton` accesses virtually all D3D12 UAVs and SRVs through `VK_EXT_descriptor_indexing` descriptor heaps. The driver cannot observe which 64 KiB tiles—or even which sparse resources—a compute or fragment shader wrote to dynamically. Without hardware dirty-page bits, any UAV/storage write forces conservative full-resource (or all-writable-sparse-resources) dirtying, bringing back heavy writebacks on unbind.
* **Backing-heap coherency without unbind:** In Vulkan/D3D12, the backing `VkDeviceMemory` can be CPU-mapped (`HOST_VISIBLE`, common on Android UMA) or aliased by another placed resource with a barrier *without* unbinding the sparse tile. Writing GPU results into a shadow BO leaves the backing `VkDeviceMemory` silently stale whenever the app reads it directly.
* **Tile layout swizzle & AFBC:** AFBC must be strictly disabled for sparse images (header blocks are not tile-independent). Furthermore, `vkd3d-proton` requires `residencyStandard2DBlockShape` (64 KiB D3D12 standard 2D tile blocks); mapping those onto Mali's 16×16 u-interleaved layout (`DRM_FORMAT_MOD_ARM_16X16_BLOCK_U_INTERLEAVED`) means tile bind/writeback is not always a flat 64 KiB `memcpy`—it can require 2D strided/swizzled copies depending on format and pitch alignment.
* **Timeline semaphore ordering:** `vkQueueBindSparse` in `vkd3d-proton` uses timeline semaphores (`pWaitSemaphores` / `pSignalSemaphores`). Any CPU-side shadow copy or uncommit worker waiting on GPU fences must be completely off the calling thread and strictly ordered against subsequent binds to the same VA range to prevent ABA races and self-deadlock on batched submits.

### 4. Hidden `mali_kbase` Kernel Hazards for S0-GPU & Deferred Uncommit

Even before S1, four `mali_kbase` kernel invariants directly impact the S0-GPU gate and deferred uncommit worker:

* **`GROWABLE_FLAGS_REQUIRED` enforces GPU write permission:** In `mali_kbase`, `kbase_mmu_page_fault_worker()` checks `(region->flags & (KBASE_REG_PF_GROW | KBASE_REG_GPU_WR))`. Any growable region *must* be allocated with `BASE_MEM_PROT_GPU_WR`—and OEM CSF kernels vary on whether a read-only translation fault to an unbacked page grows the region or terminates the CSG (`KBASE_CSF_GROUP_FAULT_TERMINATED` -> `VK_ERROR_DEVICE_LOST`).
* **GPF growth does NOT guarantee zeroed pages:** Pages recycled from the same context's `kbase_mem_pool` after a prior `kbase_mem_shrink` / free are not guaranteed to be re-zeroed on re-allocation to the same `kctx`. Relying on GPF growth to simulate unbound zero-reads risks leaking stale bytes from previously freed buffers in the same process.
* **`KBASE_IOCTL_MEM_COMMIT` shrink stalls the GPU & takes `mmap_lock`:** Shrinking a `SAME_VA` region from a background CPU worker zaps CPU VMA PTEs (`mmap_lock`) and executes an address-space-wide GPU MMU + L2/LSC cache flush (`mmu_flush_invalidate`, switching to the `GPU_CONTROL` path on `arch_maj > 11` / v14). Doing this asynchronously while active rendering is in flight risks frame-time spikes and kernel lock contention on `kctx->reg_lock`.
* **JM v9 scope creep:** Upstream PanVK only supports sparse on v10+ (`panthor`). Dragging JM v9 into S1–S6 doubles the fault/sync testing matrix for hardware with fundamentally different fault isolation (job-chain fault vs. CSG fatal fault) and lower modern D3D12 viability.

---

## Required Additions to S-Pre, Title Census & S0-GPU Before Unlocking S1

Before any S1 code is written, harden the three approved gates with these specific pass/fail checks:

| Gate | Mandatory Check to Add | Kill / Pass Criterion |
| --- | --- | --- |
| **S-Pre** | Verify `ResourceBindingTier >= 2`, `VK_EXT_robustness2` (`robustImageAccess2` + `nullDescriptor`), `VK_EXT_mutable_descriptor_type`, and SM 6.2+ float controls (`shaderFloat16`/`Int16` + `VK_KHR_shader_float_controls`). | Must boot baseline FL 11_1 and override-gated FL 12_0 titles (that don't call tiled APIs) cleanly before touching sparse. |
| **Title Census** | Log **all four** calls per target game: (1) `D3D12CreateDevice` FL requested, (2) `CreateReservedResource` count & total virtual MiB, (3) `UpdateTileMappings` aliasing (same heap offset mapped to multiple tiles), and (4) DXIL `CheckAccessFullyMapped` usage. | **Kill S1–S6** if target games either (a) never call `CreateReservedResource`, or (b) rely on tile aliasing / residency feedback that "Tier 1-minus-Aliased" cannot satisfy. |
| **S0-GPU (Test A)** | Issue a GPU **read** and **write** to an unbacked page at `offset = 60 MiB` inside a `64 MiB` `BASE_MEM_SAME_VA | BASE_MEM_GROW_ON_GPF | BASE_MEM_PROT_GPU_WR` region across CSF v10/v11, CSF v14, and JM v9. | Measure: (1) Does CSF survive without CSG fatal? (2) Does `used_pages` jump by 1 page or by **~60 MiB** (confirming prefix-only commit)? |
| **S0-GPU (Test B)** | Fill a region with `0xDEADBEEF`, shrink via `KBASE_IOCTL_MEM_COMMIT(0)`, then re-grow via GPF on another region in the same `kctx` and read back on GPU. | Verify whether recycled `kbase_mem_pool` pages leak non-zero data (breaks unbound-tile zero semantics). |
| **S0-GPU (Test C)** | Run continuous 60 FPS GPU rendering while a worker thread performs `KBASE_IOCTL_MEM_COMMIT` shrinks on a separate `SAME_VA` BO. | Measure p99 frame-time stall from AS-wide MMU/L2/LSC invalidation and `kctx->reg_lock` contention. |

---

## Council Scorecard Entry

* **Delegate:** Gemini 4 Neon
* **Score:** **6.5 / 10**
* **Vote:** **Approve S-Pre, Title Census, and S0-GPU** (with Tests A–C added above). **Hold S1–S6**—and recommend explicitly scoping JM v9 out of S1–S6 entirely. Given the `sparseResidencyAliased = false` VKD3D-Proton gate and `mali_kbase`'s prefix-only commit semantics, S-Pre + per-title FL 12_0 overrides will likely deliver 90%+ of the real-world game compatibility gains without inheriting the memory cliff and bindless coherency hazards of shadow-tile sparse.
