# DeepSeek V4.1 Flash Free — D3D12 FL 12_0 Architecture Council Review (Round 2)

**Score: 7.5 / 10**

### 1. Round 1 closure

| Round 1 blocker | Round 2 assessment | Verdict |
|---|---|---|
| Specification integrity | No conformant Tier 2 claim; `sparseResidencyAliased` stays false; override is per-title, opt-in, and explicitly non-conformant. | **Resolved** |
| Memory budget | 16 GiB arena replaced by per-resource regions, 256 MiB context cap, heap-budget check, and OOM on over-cap reservation. | **Resolved at plan level** |
| CSF faults / unbound reads | S0-GPU kill-switch gate before S1; no GPU zero-fill on CSF uncommit; eager fallback defined. | **Resolved in principle**; sink path (b) is dead and should be deleted |
| Bind sync / hot path | Dirty-bit write-back and deferred fence-gated uncommit worker replace the naive immediate path. | **Partially resolved**; lifetime and rebind semantics are underspecified |
| Prerequisites / order | S-Pre, census, then S0 gate; S1-S6 conditional. | **Resolved**, except S5 placement is unsafe |

### 2. Remaining risks & nuances

1. **S5 (override) before S1/S3 is unsafe.** The census only logs API calls; it cannot prove a title won't call `UpdateTileMappings` on later levels or use sparse shader ops. With the override active, vkd3d-proton compiles sparse residency shaders, which the driver won't support until S3. Ship S5 only after S1 and S3.
2. **The sparse-queue premise is wrong in the current tree.** In the tree reviewed, `has_sparse = PAN_ARCH >= 10 && !device->kbase_node_path[0]`. S1 must enable the feature and bind queue, not merely replace a submit stub. Also set `sparseResidencyImage3D = false` so 3D sparse residency is not falsely advertised.
3. **Sink path (b) is dead.** `MEM_ALIAS` cannot be FIXED (`mem_linux.c:1759`). Delete (b) so the outcome space is cleanly `{GPF growth, eager}`.
4. **Core shadow-tile semantics need specification before S1.** Rebind is an implicit unbind (must write back old dirty tile before copy-in). Frees must drain the uncommit worker. Rebind/uncommit race needs atomic per-region locking.
5. **GPF growth needs a performance gate.** S0 should measure fault rate/latency across a 50% unbound texture over multiple frames to detect fault storms.
6. **FL 12_1 scope gap.** Explicitly defer FL 12_1 in `dx12-fl-target.md` or add ROV/CR to S-Pre/S6.

### 3. Final recommendation
Approve the revised direction and census-first / S0-gate-first order. Do not commit S1-S6 effort until census shows a target title needs tiled residency and S0-GPU passes. Fix plan-level items (remove sink path b, correct sparse-queue/Image3D, specify rebind write-back and worker drain) before S1 starts.
