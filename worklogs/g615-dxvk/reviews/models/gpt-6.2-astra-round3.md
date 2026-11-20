# GPT-6.2 Astra — D3D12 FL 12_0 Architecture Council Review (Round 3)

**Score: 8.0 / 10**

## 1. Verdict
Round 3 resolves the main architectural concerns. Approve Phase 0 and the title census, and keep Phase 3A as the default. Phase 3B still needs a consistent decision rule, a budget that applies before any backing grows, and a GPU test that exercises the texture sampler.

## 2. Key Technical Strengths
- Decoupling per-title FL overrides from sparse work gives Bucket A a release path even if the sparse prototype fails.
- Audit of `CreateReservedResource` in both vkd3d implementations is an important early check.
- CSF-only scope, explicit prefix accounting, and ban on mid-frame shrink address three distinct costs.
- Test D asks the right practical question for streaming workloads: physical memory consumed when the application requests only the smallest mips.
- Dropping inferred per-tile dirty bits removes a correctness assumption the driver cannot uphold for arbitrary shader writes. Disabling AFBC provides a verifiable layout contract.

## 3. Remaining Risks, Edge Cases & Recommendations
1. **Make the S0 decision unambiguous**: Test A defines success as context survival, so a surviving context with 60 MiB of prefix growth can pass A and still mandate 3A. CSG faults or prefix explosion select 3A; stale-byte results disable `GROW_ON_GPF`.
2. **Enforce cap before growth, including non-tail binds**: Reserve additional page-rounded prefixes before growth operations or reserve full image on first bind.
3. **Restore sampler and rebind tests to gate**: GPU access at offset 60 MiB does not prove filtered texture reads across resident/unbound boundaries. Run bilinear boundary sampling and neighbor-isolation readbacks.
4. **Define read-only SRV population**: Trace `CopyTiles` and `CopyTextureRegion` alongside mapping calls. D3D12 `CopyTiles` uses linear buffer data within each 64 KiB tile. Verify partial edge tiles, block-compressed formats, and packed mip uploads by texel readback.
5. **Describe Phase 0 as per-title FL 12_0 compatibility**: Both documents say "native FL 12_0", but Microsoft requires Tiled Resources Tier 2 for native FL 12_0. Keep capability reporting accurate.
