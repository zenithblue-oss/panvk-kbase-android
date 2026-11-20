# Hy4 Flash Free — D3D12 FL 12_0 Architecture Council Review (Round 3)

**Score: 8.0 / 10**

## 1. Verdict
Round 3 resolves all five Round 1 blockers and all four of Gemini 4 Neon's Round 2 fundamental contradictions at the architectural level — more honestly than either prior round, by making sparse optional instead of trying to salvage it.

## 2. Key Technical Strengths
- **Decoupling + Path 3A default**: Delivers the real user goal (FL 12_0 for non-tiled titles) with zero sparse code.
- **Honest memory accounting**: Charges full virtual image size on first mip-tail bind against `PANVK_SPARSE_CAP_MB`; converts silent corruption into deterministic OOM.
- **Correct dirty-tracking fix**: Drops fake per-tile bits (unobservable under bindless descriptor indexing); replaces with read-only SRV scope / whole-resource dirty.
- **Correct layout handling**: Mandatory AFBC disable + compute 2D swizzle between D3D12 64 KiB standard tiles and Mali 16×16 u-interleaved.
- **Correct reclaim discipline**: Bans mid-frame `MEM_COMMIT` shrink (mmap_lock + AS-wide MMU/L2/LSC flush); reclaim only at destroy / zero-tile idle.
- **Scope discipline**: CSF v10–v14 only; JM v6/v7/v9 dropped.
- **Evidence-driven gates**: Tests A–D measure `used_pages` prefix growth, zero-leak, shrink stalls, and mip-tail footprint on HW before S1.

## 3. Remaining Risks, Edge Cases & Recommendations
1. **Region-count ceiling regression (high)**: Round 3 budgets bytes only. Reinstate a context-wide region cap (~4096 regions / 256 MiB); fail closed on region-alloc ENOSPC.
2. **Budget economics make Path 3B low-value (high)**: Full-virtual-size charging means four 64 MiB textures exhaust 256 MiB cap. Add a Path 3B precondition: live virtual size must fit under cap.
3. **Swizzle integration/correctness (med-high)**: Must integrate with vkd3d tile-copy commands. Gate on format-specific correctness tests.
4. **Read-vs-write fault in Test A (med)**: Require separate GPU READ and WRITE; a read-fault CSG kill fails path (a) even if writes grow.
5. **Census under active override (med)**: Run census with override active to catch titles that enable tiled paths only at FL 12_0.

## 4. Final Recommendation
Approve Round 3 as plan of record. Proceed with Phase 0, Phase 1 census/vkd3d audit, and Phase 2 S0-GPU immediately. Keep Path 3B strictly opt-in and gated on Tests A–D.
