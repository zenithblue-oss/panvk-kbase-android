# Claude Opus 5.5 — D3D12 FL 12_0 Architecture Council Review (Round 4)

**Score: 8.4 / 10**

## 1. Verdict on Round 3 risks and contradictions
Mostly resolved. The text is consistent, and the main Round 3 contradiction is gone.
- Test A rule fixed: prefix growth is expected kernel behaviour and only changes the 3B mechanism. Path 3A fires only on context death or <2 B2 titles.
- Tombstone, struck Round 1 bullets, and section precedence: clean.
- Region cap: 4,096 × 64 KiB = 256 MiB is consistent with byte cap.

## 2. Key Technical Strengths
- FL 12_0 decoupled from sparse work; Bucket A titles ship without sparse risk.
- Tiered claim is honest (Tier 1-minus-Aliased, override opt-in per title).
- v6/v7 Phase 0 list is complete: minmax emulation, 18 typed UAV formats, and 64 storage images.
- Census shim catches engines branching on reported tier; B1/B2 split prevents artificial 3B triggering.
- Dual-cap bounds memory and kbase region tracking.
- Destruction drain closes deferred-worker UAF window.

## 3. Remaining Open Items
1. Unbound tile reads once `GROW_ON_GPF` is banned: verify whether GPU hole reads cause CSG termination.
2. Test D conditionality: make Test D and mitigations conditional on Test A single-tile growth.
3. Fallback for CSF uAPI < 1.9: note whole-resource eager fallback for older kernels.
4. Write-back on unmap: ensure unmapping dirty tiles triggers write-back before freeing memory.

## 4. Final Recommendation & Readiness
- Phase 0: GO now on all arches.
- Census: GO, with vkd3d audit run first.
- S0-GPU: GO after adding hole read test and uAPI < 1.9 fallback note.
