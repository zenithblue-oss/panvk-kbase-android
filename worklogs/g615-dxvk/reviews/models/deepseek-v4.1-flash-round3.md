# DeepSeek V4.1 Flash Free — D3D12 FL 12_0 Architecture Council Review (Round 3)

**Score: 8.0 / 10**

## 1. Verdict
Conditional approve Phase 0/1/2 + Path 3A; hold Path 3B pending the vkd3d aliased-flag, `CopyTiles`, fallback-resource, and non-resident-read audit. Round 3 resolves the core structural contradictions by decoupling FL 12_0 and establishing clean kill gates.

## 2. Key Technical Strengths
- Decoupling FL 12_0 from sparse allows shipping immediate value for Bucket A titles.
- CSF v10-v14 focus avoids complex driver divergence on legacy Bifrost/Valhall JM architectures.
- Contiguous-prefix commit budget rule (full image virtual size on mip-tail bind) prevents silent OOMs.
- Dropping per-tile dirty bits is honest to bindless descriptor architecture.
- Reclaim restrictions avoid `mmap_lock` stalls during command submission.

## 3. Remaining Risks, Edge Cases & Recommendations
1. **vkd3d Aliased Flag Audit**: If vkd3d-proton unconditionally requires `sparseResidencyAliased` to allow `CreateReservedResource`, Path 3B requires a vkd3d patch. Audit this early in Phase 1.
2. **CopyTiles Path**: D3D12 `CopyTiles` uses linear buffer semantics. Ensure compute swizzle handles pitch and non-standard row strides cleanly.
3. **Region Count Limits**: CSF still allocates one region per 64 KiB tile. Enforce a hard cap on total regions per context (e.g., 4096 regions).
4. **Non-Resident Read Semantics**: Verify in S0-GPU whether GPU read faults on unbacked pages result in page allocation or fatal CSG fault.

## 4. Final Recommendation
Proceed with Phase 0, Phase 1, and Phase 2. Path 3A should remain the expected outcome unless multiple high-value titles land in Bucket B and pass all gate criteria.
