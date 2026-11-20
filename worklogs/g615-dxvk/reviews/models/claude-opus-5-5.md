# Claude Opus 5.5 — D3D12 FL 12_0 Architecture Council Review (Round 2)

**Score: 7.0 / 10**

### 1. Verdict on Round 1 blockers

| Round 1 blocker | Status |
|---|---|
| Spec integrity: no fake Tier 2 | **Resolved.** README calls the result "Tier 1-minus-Aliased". The plan keeps vkd3d's tier 0 derivation and makes the FL override opt-in per title. |
| JM/v14 16 GiB arena | **Resolved.** Each resource gets its own SAME_VA region. There is a 256 MiB cap and a heap-budget cap; over-cap fails with `VK_ERROR_OUT_OF_DEVICE_MEMORY`. |
| CSF unbound-read faults | **Gated, not solved.** S0 GPU gate is the right control. Path (b) is close to dead: an alias cannot be FIXED, so a per-tile sink is effectively eager. The choice is (a) GPF growth or eager. |
| Bind sync and hot path | **Mostly resolved.** Fence-gated worker, generation counter, and frees waiting on fence close use-after-free. |
| Order and prerequisites | **Resolved.** S-Pre, then census, then S0 gate, then S1-S6. A gate failure stops the plan. |

### 2. Remaining risks

1. **Census can miss the tiled path the override turns on.** With the override, vkd3d-proton reports tier 2. Titles pick their streaming path from `CheckFeatureSupport(D3D12_OPTIONS).TiledResourcesTier`. Run the census with the override on, log the query, and fail closed if `CreateReservedResource` is called.
2. **The override also hides non-tiled 12_0 gaps.** FL 12_0 needs Resource Binding Tier 2 and typed UAV loads. Gate the override per arch where those caps pass.
3. **The CSF cap bounds backing, not region count.** Each 64 KiB tile is its own kbase region. 4 GiB = 65,536 regions. Cap reserved VA or region count per context too.
4. **"Never written, skip copy-in" is unsafe for host-visible memory.** Limit skip to non-mappable device-local memory.
5. **Dirty tracking drops to "all dirty" for bindless titles.** UAVs in descriptor heaps cannot be enumerated per submit. Do not budget savings from it.
6. **Path (a) details.** Set `GROW_ON_GPF` extension to 16 pages so one fault backs the whole tile. Test fault latency in S0.
7. **JM nits.** Plain fully committed allocation is simpler than `GROW_ON_GPF`. If census finds no small-reservation user, cut JM sparse.

### 3. Final recommendation
Approve the gated plan with conditions: run census first with override on; gate per-arch on Resource Binding Tier 2 and typed UAV loads; S-Pre in parallel; S0 GPU gate tests region creation rate and fault latency; start S1 only after S0 passes.
