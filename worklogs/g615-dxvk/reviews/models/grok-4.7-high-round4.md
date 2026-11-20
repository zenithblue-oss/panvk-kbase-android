# Grok 4.7 High — D3D12 FL 12_0 Architecture Council Review (Round 4)

**Score: 8.4 / 10**

## 1. Verdict
Round 3 risks mostly closed. Prefix-growth vs Path 3A contradiction is fixed. Sink path (b) tombstoned. Caps, dirty split, host-visible copy-in, reclaim drain, B1/B2, census shim all consistent between README and `dx12-fl-target.md`.

## 2. Key Technical Strengths
- Decision tree now matches kbase prefix growth (`MEM_COMMIT` / GPF from backed end). ~60 MiB delta bans `GROW_ON_GPF`, does not kill 3B.
- Test A splits GPU READ and WRITE at 60 MiB. Path 3A is only CSG fatal / `DEVICE_LOST` or <2 Bucket B2.
- Phase 0 on v6/v7 names the real FL blockers: min/max filter, 18 typed UAV load formats, storage images 32→64.
- Dual cap (256 MiB and `PANVK_SPARSE_MAX_REGIONS=4096`) bounds rbtree / `reg_track`.
- Reverse-mip and split mip-tail BO are measured in Test D, not assumed.
- Transfer writes per-tile dirty; shader writes whole-resource. Host-visible never skips copy-in. Write-back before copy-in. Worker drain before region free.

## 3. Leftover (not blockers)
- v9 still needs min/max emulation (`PAN_ARCH >= 10`).
- Name the 18 formats explicitly before Phase 0 coding.
- `CheckAccessFullyMapped` forces B2, but SPT / `OpImageSparse*` lowering is S3.
- Storage-image 32→64 on v6/v7 reduces sampled images 256→176 to fit the 240-entry attribute table (patch 0141).

## 4. Final Recommendation & Readiness
- Phase 0: ready to execute. Non-tiled FL 12_0 does not wait on sparse.
- Census: ready once the FL 12_0 + Tier 2 shim is census-only and never shipped.
- S0-GPU: ready on CSF v10/v11 and v14. Do not start S1–S6 until Test A context survival and the >=2 B2 rule both pass.
