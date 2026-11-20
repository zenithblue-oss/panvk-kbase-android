# Hy4 Flash Free — D3D12 FL 12_0 Architecture Council Review (Round 2)

**Score: 7.0 / 10**

### Verdict on Round 1 blockers

| # | Round 1 blocker | Status | Why |
|---|---|---|---|
| 1 | Spec fraud / silent corruption (Tier 2 lie) | **Adequately resolved** | Plan concedes "Tier 1-minus-Aliased", refuses to patch vkd3d derivation, and makes override opt-in per-title. |
| 2 | Inverted memory economics (eager JM/v14) | **Adequately resolved** | 16 GiB arena dropped; per-resource SAME_VA regions; 256 MiB cap + heap budget with `VK_ERROR_OUT_OF_DEVICE_MEMORY`. |
| 3 | CSF unbound-tile group faults | **Conditionally resolved** | S0-GPU kill-switch gate added before S1 with two paths and eager fallback; GPU zeroing on uncommit removed. |
| 4 | Region exhaustion / hot-path latency | **Partially resolved** | Dirty bits + fence-gated worker fix sync hazard and cut write-backs; commit cost on bind remains and 16k-region ceiling untested. |
| 5 | Runtime target discrepancy | **Adequately resolved** | Both overrides documented; S-Pre puts `robustImageAccess2` + float controls first. |

### Remaining Risks & Nuances

1. **Zero-on-recommit is contradicted by device data.** Plan assumes kernel-zeroed recommit, but a tested v12 device returned non-zero recommitted data. S0-GPU must settle this before relying on it for read-0.
2. **The sink is not read-only.** kbase's sink is WRITE_ALLOC and shareable; writes to one unbound tile land in a shared page visible at every sink-mapped VA.
3. **S1–S6 may be moot.** If no title uses `UpdateTileMappings`, S1-S6 are deprioritized; if a title uses them, no override can be set without aliasing breakage.
4. **Uncached SPT lookups unmodeled.** 128 KiB global bitmap read per sparse op can stall Mali warps.
5. **Region ceiling untested at scale.** 16,384 regions/1 GiB remains a kbase context pressure risk.
6. **"Never written" copy-in skip under-specified** for CPU host-visible mappings.

### Final Recommendation
Conditional GO, staged. Proceed immediately with zero-driver-work path: S-Pre, census, and per-title override for probe-only titles. Run S0-GPU gate on CSF v10/v11 and JM v9 before driver commitment. Hold S1-S6 until S0-GPU passes and zero-on-recommit contradiction is resolved on hardware.
