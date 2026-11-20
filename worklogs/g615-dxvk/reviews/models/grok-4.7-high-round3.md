# Grok 4.7 High — D3D12 FL 12_0 Architecture Council Review (Round 3)

**Score: 7.5 / 10**

## 1. Verdict
Round 3 resolves the Round 2 technical contradictions. It does not close the FL 12_0 catch-22, and the two plan documents still disagree with each other.

## 2. Key Technical Strengths
- FL 12_0 no longer waits on sparse. Phase 0 + Bucket A overrides ship first. Path 3A is the default kill switch.
- JM v6/v7/v9 dropped from sparse. CSF v10–v14 only. Removes the fault/sync split Round 2 still carried.
- Prefix commit is now a budget rule: first mip-tail bind charges full virtual size against `PANVK_SPARSE_CAP_MB`. `GROW_ON_GPF` banned if Test A shows prefix explosion or Test B shows zero leaks.
- Per-tile dirty bits gone. Bindless cannot see them. SRV-only, else whole-resource dirty.
- AFBC off + compute swizzle for D3D12 64 KiB ↔ Mali 16×16 u-interleaved is the right copy path.
- Mid-frame `MEM_COMMIT` shrink banned. Reclaim only at destroy or zero-tile idle.
- Phase order is correct: 0 → census A–D + vkd3d audit → S0-GPU A–D → 3A or 3B.

## 3. Remaining Risks, Edge Cases & Recommendations
1. **Catch-22 remains**: `sparseResidencyAliased=false` still makes vkd3d report tiled tier 0 and cap FL at 11_1. "Native FL 12_0" is still an override. Phase 1 must record the exact `CreateReservedResource` reject path. If vkd3d rejects reserved resources, Bucket B dies and 3B is dead code.
2. **Path 3B is still not sparse**: Full-size charge on first mip-tail bind means four 64 MiB reserved textures exhaust 256 MiB after binding ~256 KiB of tail. Treat 3B as a compatibility shim, not a streaming feature.
3. **Test A underspecified**: Touch must be separate GPU read and GPU write at 60 MiB. Record `used_pages` delta and whether growth is page, 64 KiB tile, or ~60 MiB prefix.
4. **Layout hole**: Compute swizzle does not fix 3D standard tiles, packed mip tails, or block-compressed tiles (BC/ASTC). Zero pages do not decode as black. Census must flag compressed and 3D reserved resources as Bucket C.
5. **Phase 0 scope on v6/v7**: Typed UAV loads and 64 storage images are needed for FL 12_0 on Bifrost; ensure Phase 0 covers them.

## 4. Final Recommendation
Phase 1 exit: census with override active, plus vkd3d reserved-resource audit. If Bucket B < 2 or vkd3d rejects reserved resources, stop at 3A. Phase 2 exit: Tests A–D on CSF v10, v11, and v14. Do not schedule S1 until both exits pass.
