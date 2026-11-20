# DeepSeek V4.1 Flash Free — D3D12 FL 12_0 Architecture Council Review (Round 4)

**Score: 8.0 / 10**

## 1. Verdict: Round 3 risks are mostly resolved, but not fully

**Resolved cleanly:**
- Phase 0 v6/v7 completeness is concrete: min/max emulation, 18-format typed UAV loads, storage images 32→64, FL 12_1 deferred.
- Test A no longer conflates prefix growth with context death. Kill switch triggers only on CSG fatal fault/DEVICE_LOST or <2 B2 titles.
- Mip-tail mitigation is framed as a Test D measurement (reverse-mip or split tail BO).
- 4,096-region cap plus 256 MiB byte cap protects kbase VMA/tracking tables.
- Census hardening: shim active, DXIL `CheckAccessFullyMapped` scan, placed-resource/aliasing logging, B1/B2 split.
- Two-granularity dirty tracking, host-visible copy-in, rebind write-back ordering, and worker drain are coherent.
- Sink path (b) properly tombstoned and Round 1 bullets struck.

## 2. Material issues still open (for Path 3B)
1. **Eager fallback contradicts non-resident read semantics**: If `GROW_ON_GPF` is banned, committing only the bound tile leaves unbound tiles unbacked; a TMU read crossing into them faults on CSF. The fallback is only sound if eager means committing and zeroing every tile of the resource at creation.
2. **B2 shipping path conflicts with override policy**: The override is authorized only for Bucket A titles that never call `UpdateTileMappings`, while Path 3B exists for B2 titles that do. B2 reachability in vkd3d must be a hard gate.
3. **Budget enforcement at bind is not expressible through D3D12**: `UpdateTileMappings` returns `void`; reservation must charge worst-case committed bytes/regions so that binds never fail for budget.

## 3. Key Technical Strengths
- Strategic decoupling: FL 12_0 ships natively via Phase 0 for non-tiled titles.
- Test A READ/WRITE split and decision table are honest about kbase prefix growth.
- Two-cap budget with region cap protects the kernel.
- Census design is strong: shim-driven, B1/B2 separation, DXIL residency scan, vkd3d audit.

## 4. Final Recommendation & Readiness
- Phase 0: GO.
- Census: GO with conditions (no-shim baseline pass, log virtual footprints, vkd3d audit gate).
- S0-GPU: GO with conditions (eager-fallback read-0 and zeroing tests).
- Path 3B: NO-GO at present pending eager fallback semantics, B2 reachability in vkd3d, and reservation-time worst-case budget enforcement.
