# Claude Sonnet 4.6 — D3D12 FL 12_0 Architecture Council Review (Round 4)

**Score: 8.9 / 10**

## 1. Verdict: Round 3 Risks Resolved

| Round 3 risk / contradiction | Status |
|---|---|
| Test A kill-condition contradiction (prefix growth killing 3B) | **RESOLVED** — now a mechanism-swap, not a kill |
| B bucket ambiguity (opportunistic vs. hard-required conflated) | **RESOLVED** — B1/B2 split with explicit 3B threshold rule |
| Phase 0 v6/v7 incompleteness (min/max, typed UAV, storage image count) | **RESOLVED** — all three gated as 12_0 prerequisites |
| Mip-tail memory bomb (no mitigation path) | **ADDRESSED** — reverse-mip and split-mip-BO evaluated in Test D |
| Context region exhaustion (256 MiB cap only) | **ADDRESSED** — dual caps: 256 MiB bytes + 4,096 regions |
| Dirty tracking scope (per-tile vs. whole-resource conflated) | **RESOLVED** — transfer writes per-tile, shader writes whole-resource |
| Reclaim policy imprecision (when to uncommit) | **RESOLVED** — N>=60 consecutive zero-tile frames + fence signaled + worker drain before region free |
| Sink path (b) confusion (references to dead mechanism) | **RESOLVED** — tombstoned in plan body and struck in history |
| vkd3d `CreateReservedResource` with `sparseResidencyAliased=false` | **OPEN ACTION ITEM** — documented as "investigate in Phase 1," resolution path underspecified |

## 2. Key Technical Strengths

1. **Test A decision tree now sound**: Prefix growth and pool leak now only ban `GROW_ON_GPF` as a residency mechanism and switch 3B to explicit eager pre-commit or explicit GPU zeroing. Only CSG fatal fault / context death OR <2 B2 titles routes to 3A.
2. **B1/B2 split is architecturally clean**: B1 (opportunistic) shipping as a Phase 0 title with an override profile costs nothing on Path 3A and provides census data.
3. **Dual context cap protects both subsystems**: 256 MiB committed bytes guards physical memory; 4,096 regions (`PANVK_SPARSE_MAX_REGIONS`) guards kbase's region tracking tables.
4. **Dirty tracking scopes match D3D12 semantics**: Transfer writes have CPU-known bounds; shader writes use whole-resource dirty tracking; rebind writeback rule closes double-bind aliasing hazards.
5. **Reclaim policy is conservative and correct**: Zero-tile idle requires both tile count and fence signal; worker drain before region free prevents use-after-free on kbase VA.
6. **Mip-tail mitigation properly deferred to measurement**: Evaluates reverse-mip and split-BO, falling back gracefully to full virtual size if needed.

## 3. Remaining Open Items

1. **Bind-queue zeroing ordering invariant**: Specify synchronization between bind queue zeroing and draw queue submission.
2. **Swizzle fallback for formats outside BC1-7 / RGBA8**: Census should log formats to establish whether other swizzles or strided copies are required.
3. **Reverse-mip vs. split-mip-BO selection criteria**: Prefer split-mip-BO as default if both pass with equal footprint.
4. **vkd3d `CreateReservedResource` with `sparseResidencyAliased=false`**: File a vkd3d-proton issue before Phase 1 begins.

## 4. Final Recommendation & Readiness

- **Phase 0 (all arches)**: EXECUTE NOW.
- **Phase 1 Census**: READY.
- **Phase 2 S0-GPU**: READY.
- **Path 3B S1-S2**: Hold until S0-GPU passes and bind-queue ordering is specified.
