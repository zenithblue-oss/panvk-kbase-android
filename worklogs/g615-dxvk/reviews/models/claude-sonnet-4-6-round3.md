# Claude Sonnet 4.6 — D3D12 FL 12_0 Architecture Council Review (Round 3)

**Score: 7.5 / 10**

## 1. Verdict
Round 3 resolves the core architectural problems from Round 1 (4.7/10). Decoupling FL 12_0 from sparse is the decisive structural fix. The hardware scope reduction, the kbase-honest memory model, and the dirty-tracking replacement are all correct. What remains are execution risks — combinatorial correctness in the swizzle kernel, budget arithmetic for large textures, and a few undefined boundaries — not architectural flaws. The plan is now viable.

## 2. Key Technical Strengths
1. **FL 12_0 decoupling (Phase 0 + per-title overrides)**: Delivers real user value immediately without blocking on a 10-14 week sparse implementation.
2. **CSF v10-v14 scope reduction**: Dropping JM avoids diverging fault/sync models at the hardware interrupt level.
3. **kbase-honest memory model**: Charging full virtual size on first mip-tail bind against `PANVK_SPARSE_CAP_MB` internalizes prefix-commit constraints.
4. **Fake per-tile dirty bits dropped**: Restricting 3B to read-only SRV streaming and whole-resource dirty tracking is the only bindless-safe model.
5. **Mandatory AFBC disable**: Non-sparsely mappable headers are avoided.
6. **Mid-frame reclaim ban**: Eliminates `mmap_lock` and AS-wide cache-flush stalls during GPU submission.
7. **Risk-ordered phase gate**: Default-to-3A kill switch prevents sunk-cost traps.

## 3. Remaining Risks, Edge Cases & Recommendations
1. **Compute swizzle kernel correctness (high)**: Standard 64 KiB tile shape is format-dependent (BC1 is 256x128 blocks, R8 is 256x256, RGBA32F is 64x32). Mali 16x16 u-interleaved is also format-dependent. Prototype swizzle for BC1-7 and RGBA8 first.
2. **Mip-tail budget explosion (high)**: An 8K RGBA32F texture is ~256 MiB virtual alone, immediately exhausting `PANVK_SPARSE_CAP_MB`. Add per-resource charge semantics or census check for aggregate virtual budget.
3. **v14 scope in Phase 3B (medium)**: v14 has no `MEM_ALLOC_EX` and falls back to eager. State explicitly that Phase 3B per-tile FIXED applies to CSF v10-v13 only.
4. **Zero-tile idle boundary undefined (medium)**: Define idle as zero active tile binds for N >= 60 consecutive frames (`PANVK_SPARSE_IDLE_FRAMES`).
5. **Phase 2 Test A/B interaction for zero-init gate (medium)**: If Test B fails, block 3B entirely for that device rather than relying on CPU zero-init.
6. **Per-title override governance (low-medium)**: Version-pin override entries to specific title builds verified by census.

## 4. Final Recommendation
Approve Round 3 as plan of record. Proceed with Phase 0, Phase 1 census/vkd3d audit, and Phase 2 S0-GPU immediately. Keep Path 3B strictly opt-in and gated on Tests A–D.
