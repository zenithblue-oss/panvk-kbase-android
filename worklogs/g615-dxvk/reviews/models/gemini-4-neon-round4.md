# Gemini 4 Neon — D3D12 FL 12_0 Architecture Council Review (Round 4)

**Verdict: 8.2 / 10 — Conditional GO.** The Round 4 Plan of Record (evaluated from the consolidated plan and seven resolution items provided here) is architecturally sound enough to execute **Phase 0**, **Phase 1**, and **Phase 2** immediately, as decoupling non-sparse FL 12_0 prerequisites from hardware-gated sparse residency eliminates the biggest schedule and stability risk in the project. However, **Path 3B** still carries four unresolved driver-level hazards—NULL-tile fault handling after banning `GROW_ON_GPF`, submit-timeline vs. record-time dirty tracking, heap-aliasing shadow coherency, and `kbase` region lock contention—that must be specified before a single line of shadow-tile runtime code ships.

---

## Item-by-Item Audit Summary

| Plan Item | Round 4 Status | Residual Risk | Sub-Score | Primary Gap to Close |
| --- | --- | --- | --- | --- |
| **1. Test A & `GROW_ON_GPF` Gate** | Closed | High (in Path 3B) | 8.0 / 10 | Unspecified NULL-tile fault avoidance once `GROW_ON_GPF` is banned |
| **2. Phase 0 (Bifrost v6/v7 FL 12_0)** | Integrated | Medium | 8.5 / 10 | Dynamic sampler heap interaction with NIR min/max emulation |
| **3. Bounded Caps (256 MiB / 4096)** | Aligned | High | 7.5 / 10 | `kbase` `reg_lock` scaling, cap-overflow policy, alias double-counting |
| **4. Census Hardening** | Hardened | Low–Medium | 8.8 / 10 | Paired A/B run for B1 vs. B2; tile-remap churn rate telemetry |
| **5. Dirty Tracking & Transfers** | Specified | High | 7.2 / 10 | Whole-resource UAV writeback cliff, heap aliasing, record/submit race |
| **6. Reclaim & Worker Drain** | Specified | Medium | 8.2 / 10 | Memory-pressure bypass for idle hysteresis; `DEVICE_LOST` deadlock |
| **7. Document Hygiene** | Cleaned | Low | 9.5 / 10 | Ensure single canonical spec doc for implementers |
| **Phased Execution & Gating** | Consensus | Low | 8.8 / 10 | Add Path 3B performance-viability gate alongside functional gate |

---

## Detailed Review of Key Resolutions

### 1. Test A Contradiction Closed (`GROW_ON_GPF` vs. Path 3A Kill Switch)

Separating expected ~60 MiB kernel prefix growth from fatal CSF context death fixes a flawed Round 3 kill switch. On stock Android `kbase`, `BASE_MEM_GROW_ON_GPF` commits physical pages linearly from offset 0 up to the faulting VA—a single high-offset tile touch inflates physical backing across the entire prefix. Banning `GROW_ON_GPF` in Path 3B is the right architectural response rather than declaring sparse dead on CSF.

* **Unresolved NULL-Tile Hazard:** D3D12 Tiled Resources Tier 2 strictly mandates that reads from unmapped (NULL) tiles return `0` and writes are silently discarded without faulting. Without `panthor`-style `VM_BIND` zero-page aliasing and with `GROW_ON_GPF` banned, an unmapped shader access on CSF v10/v11/v14 triggers an MMU translation fault that terminates the CSG context. Path 3B must explicitly define how unmapped tiles stay fault-free (e.g., shader-level software page-table / residency check before memory ops, or shadow-backing only committed sub-extents with clamped descriptors) without silently recreating an eager allocation arena.
* **Fragile Kill-Switch Metric:** Triggering Path 3A on "fewer than 2 Bucket B2 titles" is vulnerable to corpus bias. If one B2 title represents a major engine family (such as Unreal Engine 5 Virtual Shadow Maps or RE Engine streaming), abandoning Path 3B based on raw title count understates ecosystem impact; conversely, two obscure outlier titles do not justify maintaining a complex shadow-tile subsystem. Weight the gate by **engine family coverage** rather than raw title count alone.

---

### 2. Phase 0 Completeness on Bifrost v6/v7

Integrating min/max filter emulation, 18-format typed UAV loads, and 64 storage images per stage into Phase 0—while cleanly deferring FL 12_1 (Conservative Rasterization and ROVs)—is the highest-leverage engineering decision in the plan. It unlocks non-sparse FL 12_0 and Resource Binding Tier 2 workloads across Bifrost and Valhall without touching sparse memory paths.

* **Min/Max Filter Emulation (`VK_EXT_sampler_filter_minmax`) Under Bindless Samplers:** Bifrost v6/v7 lacks native texture-unit min/max reduction, requiring NIR lowering (multi-tap gather/texel-fetch plus `fmin`/`fmax`). Two notorious pitfalls must be addressed in the Phase 0 design:
* **Dynamic Sampler Indexing:** D3D12 Resource Binding Tier 2 supports dynamically indexed sampler heaps. The compiler does not know at PSO creation time whether a `tex` instruction binds a standard linear sampler or a `MINIMUM`/`MAXIMUM` reduction sampler. Decide up front between sampler-sideband metadata branching (register pressure and warp divergence cost on Bifrost) vs. specialized pipeline variants.
* **Zero-Weight Texel Exclusion:** Both D3D12 and `VK_EXT_sampler_filter_minmax` require excluding texels with zero bilinear/trilinear filter weight from the min/max reduction. Naive 2×2 `textureGather` reductions fail CTS edge cases when sampling exact texel centers or clamped borders, and trilinear (`LINEAR_MIPMAP_LINEAR`) requires 8-tap reduction across two mip levels.
* **18-Format Typed UAV Loads:** On Bifrost v6/v7, `shaderStorageImageReadWithoutFormat` across the 18 additional D3D12 formats (`R16G16B16A16_*`, `R10G10B10A2_*`, `R11G11B10_FLOAT`, `R8G8B8A8_*`, etc.) requires confirming that AFBC is unconditionally disabled on `STORAGE`-usage images (including mutable-format views created by `vkd3d-proton`) and that software unpack paths match D3D12 UNORM/SNORM/packed-float rounding semantics.
* **64 Storage Images Per Stage:** Raising `maxPerStageDescriptorStorageImages` to 64 on Bifrost requires auditing `panvk`'s attribute/image descriptor table limits and ensuring `VK_EXT_descriptor_indexing` non-uniform indexing does not spill excessively or breach descriptor-set layout bounds. Also verify whether all remaining Resource Binding Tier 2 limits (128 SRVs, 14 CBVs, 16 samplers per stage, `descriptorBindingPartiallyBound`) are already green on v6/v7 so Phase 0 truly reaches non-sparse FL 12_0 parity.

---

### 3. Bounded Context Caps (`256 MiB` and `PANVK_SPARSE_MAX_REGIONS=4096`)

Aligning the memory cap with the region cap (`4096 × 64 KiB = 256 MiB`) eliminates Round 1's Android LMK risk. However, the mechanics of enforcing these caps on stock Android `kbase` expose three operational weak spots:

* **Kernel `reg_lock` Contention at 4096 Regions:** Each distinct `kbase` allocation inserts a `kbase_va_region` into the context RB-tree guarded by `kctx->reg_lock`. Allowing up to 4096 discrete 64 KiB allocations per context risks severe ioctl latency spikes during burst `vkQueueBindSparse` calls and kernel shrinker overhead on vendor Android kernels. Path 3B should require **contiguous range coalescing** (binding adjacent tiles as a single region) and measure `kbase` allocation/free latency at 1024–4096 regions during Phase 2 (S0-GPU).
* **Undefined Cap-Overflow Semantics:** 256 MiB is tight for real AAA Bucket B2 workloads (a single virtual texture cache or VSM pool frequently demands 256–512 MiB on 8–16 GiB phones). The plan does not specify what happens at tile 4097 or MiB 257: returning `VK_ERROR_OUT_OF_DEVICE_MEMORY` from `vkQueueBindSparse` triggers device-lost crashes in `vkd3d-proton`, while silently dropping the bind corrupts rendering. Define both an explicit overflow error contract and a device-RAM-scaled tunable ceiling rather than a hard-coded 256 MiB wall.
* **Heap Aliasing Double-Counting:** When multiple tiled resources map tiles from the same `ID3D12Heap`, clarify whether the 256 MiB cap meters **unique physical backing pages** or **virtual shadow-tile bindings**. Counting virtual bindings independently will exhaust the 256 MiB cap prematurely in aliasing-heavy engines.

---

### 4. Census Hardening

Running Phase 1 with the FL 12_0 + Tier 2 probe shim active is the right methodology—without it, engines silently fall back to FL 11_0/11_1 or non-tiled paths and invalidate the data. Several small instrumentation upgrades will prevent misleading results:

* **Paired A/B Testing for B1 vs. B2:** Running *only* with the probe shim shows what engines opt into when Tier 2 is advertised, not whether they strictly require it. Splitting Bucket B into B1 (opportunistic) and B2 (hard-required) requires a paired run: **(A)** FL 12_0 + Tier 2 shim ON vs. **(B)** non-sparse FL 12_0 ON + Tiled Resources Tier 0. Titles that boot and render accurately in (B) belong in B1; only titles that refuse to launch, crash, or visibly break belong in B2.
* **Static vs. Dynamic `CheckAccessFullyMapped` Logging:** Because `CheckAccessFullyMapped` is a shader intrinsic (`OpImageSparseTexelsResident`) rather than a CPU API call, logging it solely at PSO compile time will produce false positives from unused uber-shader permutations. Log both **compile-time presence** and **runtime dispatch/draw execution** of pipelines containing sparse residency ops.
* **API Terminology Precision:** In D3D12, placed resources are not created *on* reserved resources; rather, a **Placed Resource** (`CreatePlacedResource`) and a **Reserved Resource** (`CreateReservedResource`) alias onto the **same `ID3D12Heap`**. Ensure the probe hooks track shared `ID3D12Heap` handle intersections across both APIs so aliasing is not missed.
* **Missing Viability Telemetry:** Boolean feature flags are not enough to greenlight Path 3B. Add four quantitative metrics to the Census:
1. `UpdateTileMappings` **churn rate** (tiles remapped per frame—high churn kills shadow-tile copy-back).
2. **Mip-tail packing** usage (`NumTilesForPackedMips`).
3. **Resource types** tiled (2D color vs. 3D/Tier 3, texture arrays, depth-stencil, and MSAA).
4. **Simultaneous resident footprint** (peak MiB and active tile count) to validate whether the 256 MiB / 4096-region cap is realistic.

---

### 5. Dirty Tracking & Transfer Bounds

Distinguishing CPU-bounded transfers (`CopyTextureRegion`/`CopyTiles`) from opaque GPU shader writes—and never skipping host-visible copy-in—closes major Round 3 correctness holes, yet this remains the riskiest subsystem in Path 3B.

* **The Whole-Resource UAV/RT Writeback Cliff:** Marking an entire sparse resource dirty whenever a shader writes to it via UAV or Render Target creates a severe bandwidth trap for streaming atlases (e.g., Virtual Shadow Maps or terrain clipmaps that are rendered to every frame while streaming tiles every frame). Two refinements are essential:
* **Remap-Subset Writeback:** On `vkQueueBindSparse`, write back *only* the dirty tiles actually being unbound or remapped, never the entire resource.
* **Render-Pass Bounding:** For Render Target writes, the CPU already knows the conservative tile footprint from the render pass `renderArea` and scissor rect—use that bounding box to avoid dirtying untouched tiles.
* **Heap Aliasing Coherency Bug:** If Reserved Resource A and Resource B map the same physical heap tile, and a shader dirties A's shadow copy, B will read stale data unless `vkCmdPipelineBarrier` (translating D3D12 `ALIASING` barriers) triggers a shadow-to-heap flush and peer invalidation—even when no `UpdateTileMappings` rebind occurs. Without reverse-mapping (`heap_tile → bound shadow tiles`), aliased tiles silently corrupt.
* **CPU Record-Time vs. GPU Submit-Time Race:** Command buffers can be recorded out-of-order, across worker threads, or re-submitted multiple times, while `vkQueueBindSparse` executes asynchronously on the queue timeline via semaphores. Recording dirty bits strictly during `vkCmd*` recording introduces timeline races. Dirty state transitions and writeback blits must be resolved at **queue submission time** (`vkQueueSubmit` / `vkQueueBindSparse`) ordered by queue wait/signal semaphores.
* **UMA `HOST_VISIBLE` Copy-In & Tile Swizzle Mismatch:** On Android UMA devices, heaps may be allocated from `HOST_VISIBLE` memory types. "Never skipping host-visible copy-in" is safe, but triggering a full copy-in on every submit for every `HOST_VISIBLE` heap will crush memory bandwidth unless gated to heaps that are actively `vkMapMemory`'d. Additionally, writing back Mali U-interleaved shadow tiles into canonical D3D12 linear/standard-swizzle heap memory requires layout conversion blits—raw `memcpy`/buffer copies will scramble pixel data whenever a heap is CPU-mapped or aliased under a different format.

---

### 6. Reclaim & Worker Drain

Draining the deferred uncommit worker before resource/memory destruction cleanly eliminates `kbase` use-after-free races, and gating reclaim on signaled submit fences prevents unmapping live GPU buffers.

* **Frame-Count Hysteresis (`N` at least 60 frames) Fragility:** Tying reclaim strictly to `vkQueuePresentKHR` frame counts behaves erratically across workloads: 60 frames is 0.5 seconds on a 120 Hz display, 6 seconds on a 10 FPS loading screen, and infinite in offscreen compute/benchmark suites. More importantly, during level transitions when the context approaches the 256 MiB / 4096-region ceiling, waiting 60 frames to reclaim zero-tile allocations will cause avoidable OOM failures. Pair lazy idle reclaim (`N` at least 60 frames or wall-clock threshold) with a **synchronous pressure-driven reclaim path** triggered at ~80% of either cap.
* **Deadlock Risk Under `VK_ERROR_DEVICE_LOST`:** If the deferred uncommit worker waits on a GPU fence while holding a sparse/device lock—or if `vkDestroyImage` blocks waiting for the worker to drain after a GPU hang—the process will deadlock permanently. Enforce strict lock-free fence waits inside the worker and an immediate abort/bypass path when `device_lost` is flagged.
* **Multi-Queue Fence Tracking:** On CSF hardware supporting concurrent graphics, compute, and transfer queues, checking only the last submit fence on a single queue is insufficient. Track the latest signaled sequence/fence across *all* queues that referenced the sparse resource before permitting uncommit.

---

### 7. Document Hygiene

Tombstoning dead sink path (b) and striking superseded Round 1 bullets shows strong engineering governance. As execution begins, freeze the Round 4 Plan of Record into a single standalone specification file (rather than relying on cross-referenced Round 1–4 diffs) so implementers cannot accidentally pull stale assumptions from earlier passes.

---

## Phased Execution & Gate Validation

* **Phase 0 (S-Pre) — GO Immediately:** Uncontested win across Bifrost v6/v7 and Valhall/5th-Gen CSF. Gate exit on passing Vulkan CTS for `VK_EXT_sampler_filter_minmax`, `shaderStorageImageReadWithoutFormat` (all 18 formats), and 64-UAV descriptor indexing on Bifrost hardware.
* **Phase 1 (Census) — GO Immediately:** Add paired A/B (Tier 2 ON vs. Tier 0) runs, runtime `OpImageSparseTexelsResident` execution tracking, and `UpdateTileMappings` churn/footprint metrics before finalizing Bucket B2 counts.
* **Phase 2 (S0-GPU) — GO on CSF v10/v11 and v14:** Expand S0-GPU test probes to benchmark (1) NULL-tile fault behavior with `GROW_ON_GPF` disabled, (2) `kbase` allocation/uncommit latency at 4096 regions, and (3) vendor Android kernel divergence across OEM `kbase` forks (MediaTek, Samsung, Google Tensor) where ioctl flags or SELinux policies can differ.
* **Path 3A vs. 3B Gate — Add Performance Viability Criterion:** Gate Path 3B not only on functional survival and Bucket B2 demand, but also on a **bandwidth/frametime budget** derived from Phase 1 churn rates and Phase 2 blit latencies. A functionally compliant shadow-tile layer that drops target Bucket B2 games below playable framerates due to UAV writeback storms should still route to Path 3A.

---

## Council Round 4 Score Breakdown

| Evaluation Dimension | Weight | Score | Rationale |
| --- | --- | --- | --- |
| **Architectural Strategy & Risk Decoupling** | 25% | 9.2 / 10 | Decoupling Phase 0 from hardware-gated sparse is exemplary |
| **Hardware & `kbase` Realism (Bifrost + CSF)** | 20% | 7.8 / 10 | `GROW_ON_GPF` fix is right; NULL-tile and `reg_lock` scaling remain open |
| **Correctness & Memory Coherency** | 20% | 7.3 / 10 | Aliasing barriers, submit-timeline ordering, and swizzle conversion need spec |
| **Measurement & Gating Rigor (Census / S0)** | 20% | 8.6 / 10 | Probe shim closes bias; needs paired A/B and churn metrics |
| **Operational Safety (Caps, Reclaim, Hygiene)** | 15% | 8.1 / 10 | Worker drain is solid; needs pressure reclaim and `DEVICE_LOST` guard |
| **Weighted Total** | **100%** | **8.2 / 10** | **Slightly below 8.44 council mean; reflects Path 3B coherency gaps** |

This score places our assessment squarely alongside the more conservative Round 4 delegates (8.0–8.4) and confirms the council's unanimous execution decision: start Phase 0, Phase 1, and Phase 2 without delay, and treat the four Path 3B technical gaps above as mandatory design sign-off items before committing engineering time to shadow-tile implementation.
