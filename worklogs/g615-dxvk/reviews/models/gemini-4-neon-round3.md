# Gemini 4 Neon — D3D12 FL 12_0 Architecture Council Review (Round 3)

The Round 3 Adopted Plan of Record earns a **Conditional Approve (Independent Council Score: 7.6/10)**: Phases 0, 1, and 2 are well-structured and ready to execute, while driver stages S1–S6 remain firmly blocked by unresolved D3D12 Tier 2 semantics and contiguous-prefix memory economics.

Round 3 successfully kills the catastrophic architectural liabilities of Round 1 (global spec spoofing, 16 GiB eager arenas, JM/CSF fault divergence, fake per-tile dirty bits, and mid-frame `MEM_COMMIT` shrink stalls). However, the 8.5/10 scores in the council table overrate the feasibility of the sparse emulation path itself. Even if S0-GPU passes without CSG faults, three deep structural gaps—**mip-tail-first memory explosion**, **heap-aliased coherency**, and **unaddressed `CheckAccessFullyMapped` / NULL-tile semantics**—make Path 3A (archiving sparse and shipping decoupled FL 12_0 for Bucket A) the statistically dominant outcome.

---

## Round 3 Item-by-Item Audit Scorecard

| Plan Item | Verdict | Residual Risk | Primary Unresolved Blind Spot |
| --- | --- | --- | --- |
| **1. Decoupled FL 12_0 Enablement** | **Approve** | Medium | False Bucket A classification when engines dynamically enable tiled paths upon seeing FL 12_0; non-sparse FL 12_0 gaps in `panvk`. |
| **2. Narrowed Hardware Scope (CSF v10–v14)** | **Approve** | Medium | Vendor `mali_kbase` fork divergence (MediaTek vs. Exynos vs. Tensor) and v10/v11 Valhall vs. v12–v14 5th Gen differences. |
| **3. Contiguous-Prefix Accounting** | **Revise** | **Critical** | Real engines bind the mip tail on creation, turning sparse textures into 100% resident allocations unless mip layout is inverted. |
| **4. Bindless-Safe Dirty Tracking** | **Revise** | **High** | Heap aliasing (`VkDeviceMemory` writes via staging buffer/CPU map) breaks read-only SRV assumptions; full-surface swizzles blow mobile bandwidth. |
| **5. Concrete Layout Contract** | **Revise** | **High** | BC1–BC7 tile geometry and software-transcoding interaction on Mali-G615; 2x shadow memory overhead; non-POT edge tiles and depth/stencil. |
| **6. Banned Mid-Frame Reclaim** | **Approve w/ Note** | Medium | Structural futility of end-of-frame `MEM_COMMIT` shrink while mip tails stay pinned; unmeasured latency of `MEM_COMMIT` *grow* under Android reclaim. |
| **7. Default-to-Kill Gate (Path 3A)** | **Revise** | Medium | Census bias if probed without FL 12_0 advertised; missing split between opportunistic vs. hard-required tiled titles; soft S0-GPU thresholds. |
| **Final Recommendation (Phases 0–2)** | **Approve** | Low | Sound gating discipline, provided quantitative S0-GPU and census exit criteria are locked before execution. |

---

## Deep Technical Review of All 7 Round 3 Pillars

### 1. Decoupled FL 12_0 Enablement (Phase 0 / S-Pre + Per-Title Overrides)

**Assessment:** This is the highest-ROI decision in the entire plan. Separating immediate FL 12_0 delivery for non-tiled titles (Bucket A) from the multi-month sparse emulation risk guarantees tangible user value even if sparse is killed at Path 3A.

**Critical Risks & Required Controls:**

* **Dynamic Engine Branching (The Bucket A Trap):** Advertising FL 12_0 changes engine behavior at init time. Engines like Unreal, RE Engine, and Unity query `D3D12_FEATURE_DATA_D3D12_OPTIONS` and may only activate Virtual Texturing or Reserved Resources once FL 12_0 succeeds. A title that looked like Bucket A under FL 11_1 can silently mutate into Bucket B once the per-title override is flipped on.
* **Mandatory Fail-Fast Trap:** Per-title Bucket A overrides must never silently no-op `vkQueueBindSparse` or sparse image creation. If a Bucket A title hits a cold sparse path (e.g., high-texture preset, DLC, or streaming cutscene), `panvk` or `vkd3d-proton` must log a loud diagnostic marker and either return a clean `E_OUTOFMEMORY` / `VK_ERROR_FEATURE_NOT_PRESENT` or hard-abort in debug builds rather than causing silent GPU hangs or memory corruption.
* **Non-Sparse FL 12_0 Prerequisites in S-Pre:** FL 12_0 does not fail on sparse alone. S-Pre must explicitly audit and lock down every *other* FL 12_0 and `vkd3d-proton` requirement on Mali CSF:
* **Resource Binding Tier 2:** Descriptor indexing limits (`maxPerStageDescriptorUpdateAfterBindSampledImages`, `runtimeDescriptorArray`, non-uniform indexing across 64+ UAVs and large SRV/CBV tables).
* **Shader Resource Min-LOD (`shaderResourceMinLod`):** Verifying that `panvk` lowers SPIR-V `MinLod` operands accurately into Mali Valhall/5th Gen `TEX` descriptor/instruction fields.
* **Adjacent Expectations:** `TypedUAVLoadAdditionalFormats`, `depthBoundsTest`, `independentBlend`, and `dualSrcBlend`, which FL 12_0 titles routinely assume without fallback.

---

### 2. Narrowed Hardware Scope (Drop JM v6/v7/v9; CSF v10–v14 Only)

**Assessment:** Dropping Job Manager architectures (Bifrost v6/v7, Valhall v9) is strictly necessary. JM job-chain fault recovery (`JS_FAULT`) and CSF Command Stream Group (`CSG`) fatal fault handling have zero shared recovery mechanics; maintaining both would double driver complexity for legacy silicon with tight memory ceilings.

**Critical Risks & Required Controls:**

* **CSF v10–v14 Is Not One Uniform Target:** Grouping v10/v11 (Valhall 3rd/4th Gen: Mali-G610, G615, G715) with v12–v14 (Arm 5th Gen with Deferred Vertex Shading: Immortalis-G720, G925) hides meaningful differences in CSF firmware (`mali_csffw.bin`), tiler heap management, and MMU behavior. S0-GPU testing on both v10/v11 and v14 (as noted in the final recommendation) is the right move, but firmware version strings and kbase driver revisions must be logged as first-class test variables.
* **Stock Android Vendor `mali_kbase` Fragmentation:** On stock Android, `panvk` does not control the kernel driver. OEM forks (MediaTek Dimensity, Samsung Exynos, Google Tensor) carry different SELinux policies, ioctl whitelists, and `KBASE_IOCTL_MEM_COMMIT` / `BASE_MEM_GROW_ON_GPF` quirks—and some hardened Android kernels restrict `MEM_ALIAS` or cap per-context VA/commit behavior differently. S0-GPU must validate the exact ioctl path on unrooted stock Android kernels across at least two SoC vendors if broad CSF support is claimed.

---

### 3. Honest Contiguous-Prefix Accounting (`PANVK_SPARSE_CAP_MB`)

**Assessment:** Charging the full virtual image size on first mip-tail bind against `PANVK_SPARSE_CAP_MB` is intellectually honest and prevents Android's Low Memory Killer Daemon (`lmkd`) from blindsiding the process. However, this honesty exposes a fatal economic flaw in the underlying `MEM_COMMIT` backing design.

**Critical Risks & Architectural Opportunity:**

* **The "Mip-Tail-First" Death Spiral:** Nearly every D3D12 streaming engine binds the 64 KiB mip tail *immediately upon resource creation* so the texture has a valid fallback before higher mips stream in. Under standard Mali layout (mip 0 at offset 0, mip tail at the highest VA offset), committing up to the mip tail immediately consumes **100% of the virtual texture's footprint** on creation.
* **Virtual Overcommit vs. Streaming Pool Ratio:** D3D12 games commonly allocate a single 64–256 MiB physical `ID3D12Heap` streaming pool and alias tiles from it into gigabytes of virtual `CreateReservedResource` textures. Under contiguous-prefix accounting per virtual image, a 128 MiB streaming pool spread across twenty 4K virtual textures immediately bills several GiB against `PANVK_SPARSE_CAP_MB` and trips the cap—meaning the emulator fails on the exact workload sparse resources were invented for.
* **Unexplored Mitigation — Reverse-Mip Layout or Split Mip-Tail BO:** Because Item 5 already disables AFBC and routes tile population through a compute swizzle, `panvk` controls the surface layout. On Mali Valhall/5th Gen (v10–v14), Texture Descriptors point to an array of **Plane Descriptors** where each mip level carries an independent GPU VA pointer. Placing the mip tail (and smaller mips) either at offset 0 in reverse mip order or in a **separate tiny committed BO** decouples mip-tail residency from mip 0 commitment. While binding the last tile of mip 0 will still commit all of mip 0, separating the mip tail prevents an unused 4K texture from eating 20+ MiB at creation time. Round 3 should evaluate this before accepting the contiguous-prefix penalty as unavoidable.

---

### 4. Bindless-Safe Dirty Tracking

**Assessment:** Abandoning speculative per-tile CPU dirty tracking is mandatory for correctness under `vkd3d-proton`'s bindless descriptor model. However, the replacement strategy ("read-only SRV scoping or whole-resource dirty tracking") has severe coherence and performance loopholes.

**Critical Risks & Required Controls:**

* **Heap Aliasing Breaks "Read-Only SRV" Scoping:** In D3D12 and Vulkan, tiled textures are backed by `VkDeviceMemory` heaps (`ID3D12Heap`). A game does not have to bind the `VkImage` as a UAV or RTV to mutate its tile contents. It can:
1. Map the backing `VkDeviceMemory` on the CPU and write texels directly,
2. Bind a `VkBuffer` to the same `VkDeviceMemory` offset and write via `vkCmdCopyBuffer` / compute shader, or
3. Alias two `VkImage` resources to the same physical tile in the heap.

If `panvk` assumes a `VkImage` with only `SAMPLED_BIT` usage is immutable after `vkQueueBindSparse`, aliased heap writes will read stale u-interleaved data unless dirty tracking is anchored to the **backing `VkDeviceMemory` ranges**, not just the `VkImage` handle.

* **Writable Sparse Bandwidth Cliff:** Workloads like UE5 Virtual Shadow Maps or GPU-written sparse terrain caches bind tiled textures as render targets or UAVs. Whole-resource dirty tracking forces a full-surface compute swizzle/unswizzle on synchronization boundaries. On a mobile LPDDR5 thermal budget (such as Mali-G615), multi-megabyte full-surface compute copies per frame will tank frame rates and trigger thermal throttling within minutes.
* **Async Queue Ordering:** `vkQueueBindSparse` operates via timeline/binary semaphore dependencies independently of command buffer recording. Injecting compute swizzle dispatches inside `panvk_QueueBindSparse` must strictly preserve semaphore wait/signal ordering without introducing pipeline bubbles across unrelated graphics work.

---

### 5. Concrete Layout Contract (AFBC Disable + Compute-Shader 2D Tile Swizzle)

**Assessment:** Disabling AFBC for sparse images and normalizing onto Mali's fixed-size 16x16 u-interleaved layout is the only viable path on hardware without sparse AFBC header tables. Even so, the contract currently underspecifies the hardest real-world texture formats.

**Critical Risks & Required Controls:**

* **The BC1–BC7 Collision (Crucial for PC Games):** Over 90% of D3D12 tiled textures in Windows games use block-compressed formats (BC1–BC7). Two hazards collide here:
1. **Block-Geometry Complexity:** D3D12 64 KiB tile dimensions for BCn are defined in 4x4 texel blocks (e.g., 128x64 blocks for 8-byte BC1/BC4; 64x64 blocks for 16-byte BC3/BC5/BC6H/BC7), while Mali u-interleaved groups 4x4 compressed blocks into 16x16-texel superblocks. The swizzle shader must handle per-BPP D3D12 standard tile shapes across all block sizes.
2. **Hardware BCn Availability on Mali-G615:** If target Android SoCs lack fused hardware BC1–BC7 support and rely on `panvk` / `vkd3d-proton` compute transcoding (BCn → ASTC/RGBA), the physical texel footprint diverges from D3D12's 64 KiB BCn tile contract. Round 3 must explicitly verify whether target CSF v10–v14 silicon has native BCn hardware sampling and define the behavior when it does not.

* **2x Shadow Memory Tax:** Swizzling from a D3D12-tiled backing `VkDeviceMemory` heap into a separate u-interleaved `MEM_COMMIT` image buffer doubles physical RAM consumption for every resident tile—compounding Item 3's contiguous-prefix overhead on 8–12 GB Android devices.
* **Edge Cases Missing from the Contract:** The layout spec must explicitly cover:
* **Non-tile-aligned edge tiles** (images whose width/height is not a multiple of the 64 KiB tile dimensions).
* **2D Texture Arrays** (required by Tiled Resources Tier 2) and packed mip tails.
* **Depth/Stencil formats** (Mali separates D24/D32 and S8 into distinct planes) and **MSAA** tiled surfaces if exposed.

* **AFBC Bandwidth Penalty:** Disabling AFBC on primary scene texture atlases increases LPDDR5 read bandwidth by 30–60%. Scope AFBC disable strictly to `VK_IMAGE_CREATE_SPARSE_BINDING_BIT` images—never globally across Bucket A or non-sparse allocations.

---

### 6. Banned Mid-Frame Reclaim (No Mid-Frame `MEM_COMMIT` Shrink)

**Assessment:** Banning mid-frame `MEM_COMMIT` shrink is the right call. `mali_kbase` shrink paths contend on `reg_lock` / `mmap_lock` and trigger AS-wide MMU/L2/LSC cache flushes that guarantee severe multi-millisecond frame-time spikes.

**Critical Risks & Required Controls:**

* **Reclaim Futility Under Contiguous Prefix:** Because `MEM_COMMIT` can only shrink linearly from the *top* of the VA range, banning mid-frame shrink masks a deeper truth: **even end-of-frame shrink reclaims 0 bytes** for any sparse texture whose highest-offset page (the mip tail) remains bound. In practice, physical pages will only be reclaimed on `vkDestroyImage` (or if mip layout is inverted so mip 0 sits at the top and gets fully unbound). Do not budget S4–S6 engineering time for an elaborate end-of-frame shrink daemon until S0-GPU proves any real bytes are shrinkable.
* **Unmeasured `MEM_COMMIT` *Grow* Latency:** Eliminating shrink stalls does not eliminate grow stalls. Under Android memory pressure, growing `MEM_COMMIT` mid-frame can trigger kernel direct reclaim (`kswapd` / page compaction) inside the ioctl, stalling the submission thread for tens of milliseconds. S0-GPU must benchmark `MEM_COMMIT` grow p95/p99 latency while the device is under synthetic memory pressure (e.g., 75% RAM occupied).

---

### 7. Default-to-Kill Gate Structure (Path 3A)

**Assessment:** Making "kill and archive sparse" the default outcome unless proven necessary by real games and hardware benchmarks is exemplary systems discipline. The gate triggers, however, need tighter definition to avoid false negatives or ambiguous pass/fail debates.

**Critical Risks & Required Controls:**

* **Census Measurement Bias:** Phase 1 (Title Census) cannot simply trace games running on current FL 11_1 `panvk`—games will never invoke tiled APIs when Tier 0 is advertised. The census must run with a **probe shim** that advertises FL 12_0 + Tier 2 to capture actual `CreateReservedResource` and `UpdateTileMappings` call frequencies, virtual-to-physical ratios, and mip-tail-first patterns.
* **Split Bucket B into B1 (Opportunistic) vs. B2 (Hard-Required):**
* **Bucket B1:** Checks `TiledResourcesTier` separately from FL 12_0 and falls back gracefully to committed resources if Tier 0 is reported (under a targeted override).
* **Bucket B2:** Unconditionally calls `CreateReservedResource` / `UpdateTileMappings` or refuses to boot without Tier 2.

Only **Bucket B2** titles justify building S1–S6 sparse emulation.

* **Refine the "Fewer Than 2 Games" Threshold:** A raw count treats two minor titles as more important than one flagship AAA game. Weight the gate by target-title priority *and* whether those titles are even mathematically fit inside `PANVK_SPARSE_CAP_MB` (e.g., if the only 2 Bucket B2 games allocate 6 GiB of virtual textures with mip-tail-first binding, Path 3A must still fire).

---

## The Missing Pillar: D3D12 Tiled Resources Tier 2 Shader & Fault Semantics

None of the seven Round 3 synthesis points explicitly addresses the single hardest specification barrier of **D3D12 Tiled Resources Tier 2**: hardware shader residency semantics.

D3D12 FL 12_0 mandates Tier 2, which requires four runtime guarantees that stock `mali_kbase` + `MEM_COMMIT` cannot provide natively:

1. **NULL-Tile Reads Return Zero:** Sampling or loading an unmapped tile must deterministically return `(0, 0, 0, 0)` (or `alpha = 1` for formats lacking an alpha channel) without faulting the CSG. Under contiguous-prefix backing, uncommitted pages beyond the prefix will trigger a fatal CSG MMU fault, while unmapped holes *inside* the committed prefix must be explicitly zero-cleared on commit and re-cleared on unbind.
2. **NULL-Tile Writes Are Silently Dropped:** Writing via UAV/RTV to an unmapped tile inside the committed prefix must not fault—and must not corrupt data if that page is later mapped or read.
3. **`CheckAccessFullyMapped` (`OpImageSparse*` / `shaderResourceResidency`):** Shaders in streaming engines use `Sample(..., status)` + `CheckAccessFullyMapped(status)` to detect unmapped tiles and request streaming or fall back to coarser mips. Mali CSF hardware has no native residency return bit for software-emulated sparse buffers. `panvk` must either:
* Lower `OpImageSparse*` instructions in NIR to sample a **sideband residency bitmask texture** (1 bit per 64 KiB tile) alongside every sparse texture fetch—adding register pressure, texture descriptor slots, and extra memory reads—or
* Spoof "always resident," which silently breaks feedback-driven virtual texturing loops (causing permanent black/blurry tiles or infinite streaming requests).
4. **Min-LOD Clamping (`shaderResourceMinLod`):** Must interact cleanly with NULL-tile clamping rules across mip boundaries.

Before S1 begins, the council must require a one-page design addendum specifying how `panvk` handles `OpImageSparse*` lowering and NULL-tile zeroing/drop-write behavior—or explicitly confirming whether Bucket B2 games even invoke `CheckAccessFullyMapped`.

---

## Audit of the Final Council Recommendation & Mandatory Gate Exit Criteria

Approving **Phase 0 (S-Pre)**, **Phase 1 (Title Census)**, and **Phase 2 (S0-GPU)** while freezing S1–S6 is the exact right decision. To make the Path 3A kill gate objective and immune to scope creep, lock these quantitative exit criteria before Phase 0 starts:

| Phase / Gate | Concrete Deliverable | Hard Pass / Kill Criteria |
| --- | --- | --- |
| **Phase 0: S-Pre** | Non-sparse FL 12_0 compliance audit + per-title Bucket A override + fail-fast sparse trap. | Zero regressions on existing DX11/DX12 FL 11_1 titles; verified `ResourceBindingTier >= 2` and `shaderResourceMinLod` on CSF v10/v11 and v14; loud abort/error on unexpected `CreateReservedResource`. |
| **Phase 1: Title Census** | Instrumented probe run across target D3D12 game catalog with FL 12_0 + Tier 2 advertised. | Log exact counts of Bucket A, B1 (opportunistic), and B2 (hard-required) titles; capture virtual footprint, mip-tail-first behavior, BCn formats, UAV usage, and `CheckAccessFullyMapped` shader usage. **Trigger Path 3A if fewer than 2 priority Bucket B2 titles exist.** |
| **Phase 2: S0-GPU Reality Gate** | Standalone Vulkan/`mali_kbase` microbenchmark on real stock Android CSF v10/v11 (e.g., G615) and v14 devices. | **1.** Zero CSG faults across 10,000 bind/unbind/sample cycles.<br><br>**2.** `MEM_COMMIT` grow p99 latency under 2.0 ms at 75% system RAM pressure.<br><br>**3.** Physical-to-virtual overhead stays within `PANVK_SPARSE_CAP_MB` on replayed Bucket B2 traces (testing reverse-mip/split mip-tail if needed).<br><br>**4.** Measured compute-swizzle throughput for BC7/RGBA8 64 KiB tiles stays within mobile frame budget (under 1.0 ms for 64 tiles/frame). |

---

## Council Calibration & Score Spread Analysis

The progression from **4.72 (R1) → 7.11 (R2) → 7.83 (R3)** accurately reflects a plan that matured from an unshippable hack into a disciplined engineering roadmap. However, the **1.1-point spread in Round 3** (7.4–7.6 from Claude Opus 5.5, Claude Sonnet 4.6, Grok 4.7 High, and Gemini 4 Neon vs. 8.0–8.5 from Beam Free, DeepSeek, GPT-6.2 Astra, Hy4, and MiMo) reveals a classic architecture-review split:

* The **8.0–8.5 cluster** is scoring the *governance and risk containment* of the plan (decoupled Phase 0 win + Path 3A kill switch).
* The **7.4–7.6 cluster** is discounting the *technical viability of S1–S6* under stock Android `mali_kbase` constraints—specifically the high likelihood that mip-tail-first prefix inflation, heap-aliased dirty tracking, and Tier 2 residency semantics make software sparse emulation unviable even after Round 3's cleanups.

An independent score of **7.6/10** aligns with the cautious cluster: Phase 0 delivers immediate, clean FL 12_0 value, and Phases 1–2 will cheaply prove whether S1–S6 should be killed before a single line of sparse emulation code lands in `panvk`.
