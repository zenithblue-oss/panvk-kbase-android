# Beam Free — D3D12 FL 12_0 Architecture Council Review (Round 3)

**Score: 8.5 / 10**

## 1. Verdict
Round 3 substantially resolves the prior Round 1 concerns (4.7/10). It decouples FL 12_0 from sparse (shipping value immediately), narrows sparse to CSF v10-v14 (avoiding JM fault/sync divergence), replaces the impossible per-tile dirty bits with honest read-only SRV scoping / whole-resource tracking, bans mid-frame `MEM_COMMIT` shrink, and adds a concrete S0-GPU kill-switch with explicit pass/fail criteria. The plan is now implementable and internally consistent.

## 2. Key Technical Strengths
- Decoupling FL 12_0 from sparse is the single biggest win: Phase 0 + per-title overrides deliver FL 12_0 for non-tiled titles now, with no sparse dependency.
- Honest memory economics: acknowledges contiguous-prefix commit, charges full virtual size on first mip-tail bind against `PANVK_SPARSE_CAP_MB`, and bans `GROW_ON_GPF` on prefix explosion.
- Dirty tracking corrected: drops fake per-tile bits (impossible under bindless descriptor indexing) in favor of read-only SRV scoping or whole-resource dirty tracking.
- Tile transfer specified concretely: mandatory AFBC disable + compute-shader swizzle between D3D12 64 KiB standard blocks and Mali 16x16 u-interleaved.
- Reclaim discipline: no mid-frame shrink; reclaim only at destroy or zero-tile idle, preventing `mmap_lock`/AS-wide cache flush stalls.
- Clear phase gating (0 → 1 → 2 → 3A/3B) with an explicit default archive path (3A) if the census or S0-GPU fails.

## 3. Remaining Risks, Edge Cases & Recommendations
- **vkd3d external dependency**: Bucket B viability hinges on whether vkd3d-proton allows `CreateReservedResource` with `sparseResidencyAliased=false`. Record the exact code path during Phase 1.
- **v14 labeling ambiguity**: Annotate v14 as eager-path in Phase 2/3 docs to avoid implementation confusion.
- **AFBC disable cost**: Benchmark perf in Phase 2 Test D.
- **Swizzle format coverage**: Provide an explicit supported-format list before Phase 3B.
- **"Zero-tile idle" boundary**: Define precisely as zero bound tiles AND last submit fence signaled AND no pending bind-queue entries.
- **Phase 0 audit**: Audit descriptor limits and subgroup sufficiency before Phase 0 ships.
