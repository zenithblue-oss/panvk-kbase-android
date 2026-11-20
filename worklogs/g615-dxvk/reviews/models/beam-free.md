# Beam Free — D3D12 FL 12_0 Architecture Council Review (Round 2)

**Score: 7.0 / 10**

### Round 1 Blocker Resolution Verdict

**Adequately resolved in design; none yet proven on hardware.**

| Round 1 Blocker | Status | Notes |
|---|---|---|
| Spec fraud (Tier 2 claim) | Resolved | Concedes Tier 1-minus-Aliased; override is per-title, opt-in, census-gated. |
| 16 GiB eager arena | Resolved | Per-resource SAME_VA GROW_ON_GPF regions; 256 MiB context cap (`PANVK_SPARSE_CAP_MB`); `VK_ERROR_OUT_OF_DEVICE_MEMORY` on over-cap. |
| CSF uncommitted tile faults | Partially resolved | S0-GPU kill-switch gate added to choose GPF growth vs sink; GPU zero fill removed on CSF uncommit. Gate unbuilt. |
| Bind sync / hot-path | Resolved | Per-tile dirty bits; deferred fence-gated worker with generation counter; frees wait on fence. |
| Prerequisites / ordering | Resolved | S-Pre → census → S0-GPU gate → S1-S6. |

### Remaining Risks & Nuances

1. **S0-GPU gate is the critical unproven path.** If neither GPF growth nor dummy sink passes, CSF falls back to eager backing, largely defeating the sparse story on primary targets.
2. **Dummy zero sink may be unavailable for CSF FIXED tiles.** `MEM_ALIAS` cannot be `FIXED` (`mem_linux.c:1759`).
3. **Dirty-tracking granularity is coarse.** "All bound tiles otherwise" on full-resource writes re-introduces write-back cost for large streamed textures.
4. **Single worker thread per device vs. multi-queue.** Needs explicit multi-queue fence handling.
5. **Generation-counter cancellation** needs rigorous locking for concurrent unbind/rebind/free.
6. **v14 vendor variance.** G1-Ultra rejects `ALLOC_EX`, forcing JM eager path.
7. **SPT bitmap lookup overhead unaddressed.** Needs microbenchmark before NIR lowering.
8. **Census is a binary gate for S5.** Fallback must be defined per title.

### Final Recommendation
Proceed, but gate aggressively. Single most important next step is building and running the S0-GPU kill-switch gate on real hardware before any driver stage begins. Run the census in parallel.
