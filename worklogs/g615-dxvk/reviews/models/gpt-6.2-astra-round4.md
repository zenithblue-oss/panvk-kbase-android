# GPT-6.2 Astra — D3D12 FL 12_0 Architecture Council Review (Round 4)

**Score: 8.0 / 10**

## 1. Verdict
Approve Phase 0 implementation, census instrumentation, and S0-GPU experiments. The Round 3 contradiction over prefix growth is resolved. The remaining issue is that the plan can select Path 3B without proving that its chosen backing mechanism serves correct reads from unbound tiles.

## 2. Key Technical Strengths
- Decoupling per-title FL overrides from sparse work gives Bucket A a release path even if the sparse prototype fails.
- Fresh regions for A-READ and A-WRITE prevent the first access from masking the second fault.
- Independent region-count cap bounds kernel tracking overhead even when committed bytes are low.
- Mandatory host-visible copy-in, dirty write-back before rebind, and draining worker before region free each address a concrete data-loss or lifetime failure.

## 3. Remaining Findings
1. **Test backing mechanism that actually ships**: After expected prefix growth, the plan disables `GROW_ON_GPF` and commits per-tile FIXED regions at bind time. Add an S0 run with GPF disabled and the proposed fallback active. Verify a direct read from a never-bound tile and a filtered sample across a bound/unbound edge.
2. **Correct Phase 0 shipping claim in both files**: "Native FL 12_0" implies meeting the feature-level contract, which includes Tiled Resources Tier 2. The deliverable is per-title FL 12_0 compatibility through an opt-in override.
3. **Make B2 count reproducible**: Run each candidate twice through the same streaming scene at forced FL 12_0 (with Tier 2 reported and with tiled resources disabled).
4. **Footprint and reclaim correctness criteria**: For Test D, sample distinct values across mips and tail boundaries before choosing layout. For Test C, reconcile context-wide shrink prohibition with resource-local idle rule.

## 4. Final Recommendation
Start all three workstreams. Approve Phase 0 profiles per verified title, count only demonstrated B2 cases, and hold S1–S6 authorization until the selected backing layout passes unbound-read and filtering tests on target hardware.
