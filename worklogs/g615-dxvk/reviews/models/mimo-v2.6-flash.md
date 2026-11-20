# MiMo V2.6 Flash Free — D3D12 FL 12_0 Architecture Council Review (Round 2)

**Score: 8.0 / 10**

### Verdict on Round 1 blockers

Yes — all five Round 1 blockers are adequately resolved:
1. **Specification integrity** — Fixed. Concedes "Tier 1-minus-Aliased", never claims conformant Tier 2, keeps `sparseResidencyAliased = false`, refuses to patch vkd3d derivation, and scopes override to per-title opt-in.
2. **Memory budget** — Fixed. 16 GiB arena gone; per-resource SAME_VA regions, 256 MiB context cap (`PANVK_SPARSE_CAP_MB`) + heap budget, hard `VK_ERROR_OUT_OF_DEVICE_MEMORY` on over-cap.
3. **CSF faults / unbound reads** — Fixed in structure. S0-GPU kill-switch gate tests read-0, boundary taps, and context survival before S1-S6; redundant GPU zeroing on CSF uncommit removed.
4. **Bind sync / hot path** — Fixed. Per-tile dirty bits with write-target marking rule, deferred fence-gated worker with generation counter, and frees waiting on fence.
5. **Prerequisites & order** — Fixed. S-Pre → census → S0-CPU → S0-GPU gate → S1-S6 gated on both gate and census.

### Remaining risks & nuances

1. **Cap accounting vs. GPF growth (most important).** The 256 MiB counter tracks driver-initiated commits. Under path (a), kernel-grown pages on unbound tiles are not re-uncommitted if no unbind entry runs. Need counter to track kernel-grown bytes and a standing reclaim pass.
2. **Byte cap != region-count cap on CSF.** A large sparse reservation under the byte cap can still create tens of thousands of 64 KiB regions. Add explicit region-count limit.
3. **Sink path (b) is likely dead.** `MEM_ALIAS` cannot be FIXED (`mem_linux.c:1759`). State (b) as unavailable up front, leaving GPF-growth vs eager.
4. **Override can be self-defeating.** Reporting FL 12_0 can induce titles to take tiled paths they didn't take at 11_1. Validate census with override active.
5. **Per-arch gate, not global kill-switch.** JM (v9) is eager and doesn't depend on GPF growth; CSF-only failure should stop S2, not S1/S3-S6.
6. **GPF-grown page zeroing.** Verify that growth allocations provide zeroed pages, not just recommitted pages.

### Final Recommendation
Approve to proceed to S-Pre + census + S0-GPU gate, with amendments before S1: extend 256 MiB cap to cover GPF growth, add region-count cap on CSF, demote sink path (b), and validate census with override active.
