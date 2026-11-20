# D3D12 Feature Level 12_0 Architecture Council Review

Date: 2026-11-20  
Branch: `feature/dx12-12_0-implementation`  
Subject: Architecture & feasibility review of stock-kbase sparse residency ("shadow-tile sparse") for D3D12 FL 12_0  
Documents Reviewed:
- `worklogs/g615-dxvk/handoff-2026-10-28/claude-memory/dx12-fl-target.md`
- `worklogs/g615-dxvk/sparse-kbase-2026-10-27/README.md`
- `docs/KBASE-SPARSE-FEASIBILITY.md`
- `docs/vkd3d-vulkan-requirements.md`

---

## 1. Council Scorecard

| Delegate | Provider | Model / Options | Score |
|---|---|---|---|
| Claude Opus 5.5 | Claude | `claude-opus-5-5` (medium effort) | 5.5/10 |
| Claude Sonnet 4.6 | Claude | `claude-sonnet-4-6` (high effort) | 6.0/10 |
| Beam Free | OpenCode | `opencode/beam-free` (high variant) | 6.0/10 |
| MiMo V2.6 Flash Free | OpenCode | `opencode/mimo-v2.6-flash-free` | 6.0/10 |
| GPT-6.2 Astra | Codex | `gpt-6.2-astra` (medium effort) | 4.0/10 |
| Grok 4.7 High | OpenCode | `9router/gcli/grok-4.7-high` | 4.0/10 |
| DeepSeek V4.1 Flash Free | OpenCode | `opencode/deepseek-v4.1-flash-free` (max variant) | 4.0/10 |
| Hy4 Flash Free | OpenCode | `opencode/hy4-flash-free` (high variant) | 4.0/10 |
| Gemini 3.8 Flash | OpenCode | `9router/ag/gemini-3.8-flash-high` | 3.0/10 |

**Council Average: 4.72 / 10**

---

## 2. Core Consensus & Architectural Flaws

1. **Specification Fraud & Silent Corruption**:
   - `vkd3d` computes tiled tier strictly from `sparseResidencyAliased`.
   - Bypassing this via `VKD3D_FEATURE_LEVEL=12_0` (or `VKD3D_CAPS_OVERRIDE`) tells games that physical memory sharing / aliasing between tiles/resources works. On kbase, shadow tiles isolate writes, causing silent corruption for any title relying on D3D12 tile-pool sharing, placed-resource aliasing, or `CopyTileMappings`.
2. **Inverted Memory Economics on Eager Arches (JM v6-v9, v14)**:
   - Non-CSF kbase `MEM_COMMIT` grows/shrinks from the end only, meaning the entire virtual arena must be committed upfront.
   - A 1 GiB virtual tiled texture consumes 1 GiB of physical DRAM plus the tile pool heap, triggering Android Low Memory Killer (`lmkd`) on 4–8 GB devices.
3. **CSF Uncommitted Tile Group Faults**:
   - Uncommitted 64 KiB tiles have no physical pages. TMU filtering across tile boundaries, anisotropic footprints, and fixed-function RT/depth clears will trigger fatal GPU group faults rather than reading zero.
4. **Kernel Region Exhaustion & Hot-Path Latency**:
   - CSF per-tile regions (16,384 regions per 1 GiB) risk kbase context tracking caps and VMA exhaustion.
   - Serialized `MEM_COMMIT` ioctls at 15–40 µs/tile exceed frame budgets (15–40 ms for a 1,000-tile streaming batch).
5. **Runtime Target Discrepancy**:
   - PanPlay runs Wine vkd3d 1.18 (`VKD3D_CAPS_OVERRIDE=feature_level=12.0`), not vkd3d-proton.
   - For vkd3d-proton, device creation fails independently on `robustImageAccess2`.

---

## 3. Detailed Individual Delegate Reviews

### Claude Opus 5.5 (5.5/10)
- **Key Strengths**: Cited kernel UAPI primitive analysis; honest that `sparseResidencyAliased` stays false; S0 gate exists and has initial device data; stable VA model prevents BDA/root descriptor invalidation.
- **Critical Flaws**:
  - Wrong override target: PanPlay runs Wine vkd3d 1.18 (`VKD3D_CAPS_OVERRIDE`), while `VKD3D_FEATURE_LEVEL` is proton-only.
  - Shadow copies create a 2x memory multiplier (backing tile + heap copy).
  - CPU/GPU sync hazard: `MEM_COMMIT` uncommit is a CPU ioctl not ordered on the GPU queue; uncommitting without a fence wait causes use-after-free or faults.
  - Eager JM/v14 arena causes permanent fragmentation and LMK kills on 8–12 GB devices.
  - Contradictory fault behavior: If CSF uncommitted tiles fault fatally, TMU sampling fails; if they grow on GPF, read-0 is lost and memory silently inflates.
- **Recommendations**:
  - Test Wine `VKD3D_CAPS_OVERRIDE=feature_level=12.0` first with zero driver work.
  - Run title census logging `CreateReservedResource` and `UpdateTileMappings` before building driver sparse.
  - Build GPU-side test checking whether sampling an uncommitted tile faults or grows.
  - Add deferred uncommit worker and dirty tile tracking.

### Claude Sonnet 4.6 (6.0/10)
- **Key Strengths**: CSF `MEM_ALLOC_EX FIXED` + `MEM_COMMIT` is real UAPI; S0 gate stops implementation before validation; thorough kbase archaeology (`gpu_mappings > 1`, end-only growth).
- **Critical Flaws**:
  - Specification fraud: Forcing Tier 2 without aliasing produces undefined semantics for aliased reserved resources.
  - JM eager arena commits 1 GB upfront for 1 GB virtual textures with no memory savings.
  - SPT shader overhead: Uncached device-global bitmap lookups on every sparse texture op stall Mali warps.
  - 16,384 kernel regions per 1 GiB texture risks exceeding kbase context limits.
- **Recommendations**:
  - Audit target games for aliased reserved-resource usage before wiring overrides.
  - Cap JM eager arena at 256 MB with driver warning.
  - Run SPT microbenchmark on G615 before writing NIR lowering passes.
  - Audit kbase context region limits in r38p1 source.
  - Eliminate redundant GPU zero-before-uncommit on CSF.

### Beam Free (6.0/10)
- **Key Strengths**: Realistic per-arch split; avoids faking aliased bits; keeps GPU VAs stable; prototype with S0 device gate.
- **Critical Flaws**: Eager JM backing defeats streaming; Tier 2 override with `Aliased=false` causes silent corruption; fixed-function RT/ZS clears cannot be filtered in-shader; shadow copies lack dirty tracking.
- **Recommendations**: Run S0 on all 5 arches first; implement dirty tracking; make override opt-in per title; cap eager arena; stress-test 16k tiles.

### MiMo V2.6 Flash Free (6.0/10)
- **Key Strengths**: Evidence-grade feasibility citing exact kbase kernel lines; honest `Aliased=false` conclusion; S0 before 8–11 week commitment.
- **Critical Flaws**: Write-back on unbind creates divergent views between heap and shadow tiles; fixed-function writes vs fatal CSF group faults unresolved; full-texture bind of 1 GiB takes ~0.4–0.8s; SPT cache coherency between CPU updates and shader reads unmodeled.
- **Recommendations**: Resolve fixed-function handling before S1 design freeze; drop write-back on unbind to match D3D12 discard semantics; gate override per-title behind PanPlay profile; constrain SPT lowering strictly to sparse-flagged pipelines.

### GPT-6.2 Astra (4.0/10)
- **Key Strengths**: Preserves stable GPU addresses; empirical basis from device runs; pinpoints exact vkd3d-proton capability gate.
- **Critical Flaws**:
  - Shadow copies lose heap identity with `REUSE_SINGLE_TILE`.
  - Zeroing on unbind cannot implement strict non-residency; tested v12 device returned non-zero recommitted data.
  - Filtering cannot be reproduced by a residency bitmap: bilinear taps crossing resident/NULL boundaries return invalid colors.
- **Recommendations**: Make alias visibility the primary go/no-go test; prove strict access on GPU before building bind engine; specify bind ordering with full queue tracing; keep launcher override opt-in.

### Grok 4.7 High (4.0/10)
- **Key Strengths**: Rejects unsafe munmap/recreate and sink page alternatives; acknowledges S0 currently lacks GPU-side validation; accurate SPT sizing.
- **Critical Flaws**:
  - Shadow-tile is not sparse residency: memory is 2x bound set, and binds are full copies.
  - CSF region count exhaustion at 16k regions.
  - TMU cannot consult bitmap: texture op rewrite plus dependent load destroys warp occupancy.
  - Fixed-function clear/resolve cannot be filtered.
- **Recommendations**: Do not flip `has_sparse` until GPU load of uncommitted FIXED tile returns 0 without crashing; measure region ceiling at 4k/16k tiles; report honest Tier 0 with override instead of Tier 2.

### DeepSeek V4.1 Flash Free (4.0/10)
- **Key Strengths**: Kernel constraint analysis grounded in Pixel kernel source; separate gates for residency features; bounded S0 harness.
- **Critical Flaws**:
  - Override lies about the core memory model: pool tiles mapped to multiple resources, UAV writes followed by pool reads, and copies from pools fail silently.
  - Lazy CSF tiles contradict non-resident read semantics (faults fatal vs allocating on read).
  - Bind path requires an entire synthesized queue/synchronization subsystem with layout transitions.
  - Costs understated: 1,024 streaming tiles = 15–40 ms of ioctl time alone.
- **Recommendations**: Resolve constraint triangle (no kernel changes vs no aliasing vs honest Tier 2); run title census on `UpdateTileMappings` before driver work; replace S0 with GPU fault kill-switch gate; drop lazy CSF from critical path initially.

### Hy4 Flash Free (4.0/10)
- **Key Strengths**: Rigorous kbase UAPI disassembly; upfront admission of hard limits; stable VA preservation.
- **Critical Flaws**: CSF read path unsound due to TMU filtering touching unbound texels; kbase BIND queue is a stub that marks queues lost; eager arena VRAM costs cause immediate OOM on mobile; contradicts existing NO-GO policy.
- **Recommendations**: Fix vkd3d target before driver work; extend S0 to real GPU jobs; resolve CSF filtered sampling; benchmark at scale (16k tiles); split SPT plumbing into separate spike.

### Gemini 3.8 Flash (3.0/10)
- **Key Strengths**: Accurate disassembly of CSF 1.9 `MEM_ALLOC_EX FIXED_VA` zone; honest aliasing assessment; isolated S0 harness.
- **Critical Flaws**: LPDDR bus bandwidth starvation from blit copies; de-tile/re-tile overhead on JM non-interleaved textures; unbind write-back spec violation; Android LMK kill from eager arenas; VMA exhaustion.
- **Recommendations**: Cancel S1–S6; retain S0 probe only; implement `robustImageAccess2` and float-controls denorm reporting first; use launcher-level overrides for titles that only check feature levels.

---

## 4. Immediate Action Items

1. **Reconcile Runtime**: Verify Wine vkd3d 1.18 behavior with `VKD3D_CAPS_OVERRIDE=feature_level=12.0` on target games.
2. **GPU Kill-Switch Gate**: Run S0 device test verifying GPU load/store on uncommitted `FIXED` tiles to determine whether hardware faults or grows.
3. **Census Check**: Instrument PanPlay to check whether target D3D12 games ever call `UpdateTileMappings` or `CreateReservedResource`.
4. **Prerequisites First**: Implement `robustImageAccess2` and SM 6.2+ float controls before committing to driver-side sparse emulation.
