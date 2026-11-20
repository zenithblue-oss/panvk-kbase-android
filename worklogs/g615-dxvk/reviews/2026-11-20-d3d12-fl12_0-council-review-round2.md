# D3D12 Feature Level 12_0 Architecture Council Review — Round 2

Date: 2026-11-20  
Branch: `feature/dx12-12_0-implementation`  
Subject: Round 2 Review of revised D3D12 FL 12_0 plan on stock Android kbase following Council Round 1 corrections.

---

## 1. Council Scorecard (Rounds 1 & 2)

| Delegate | Provider | Round 1 Score | Round 2 Score | Delta |
|---|---|:---:|:---:|:---:|
| MiMo V2.6 Flash Free | OpenCode | 6.0/10 | **8.0/10** | +2.0 |
| Claude Sonnet 4.6 | Claude | 6.0/10 | **7.5/10** | +1.5 |
| DeepSeek V4.1 Flash Free | OpenCode | 4.0/10 | **7.5/10** | +3.5 |
| Claude Opus 5.5 | Claude | 5.5/10 | **7.0/10** | +1.5 |
| Grok 4.7 High | OpenCode | 4.0/10 | **7.0/10** | +3.0 |
| Beam Free | OpenCode | 6.0/10 | **7.0/10** | +1.0 |
| Hy4 Flash Free | OpenCode | 4.0/10 | **7.0/10** | +3.0 |
| GPT-6.2 Astra | Codex | 4.0/10 | **6.5/10** | +2.5 |

**Council Average: 4.72 / 10 (Round 1) ➔ 7.19 / 10 (Round 2)** (+2.47 improvement)

---

## 2. Verdict on Round 1 Blockers

All five Round 1 blockers are adequately resolved at the architectural design level:

| # | Round 1 Blocker | Round 2 Assessment | Status |
|---|---|---|---|
| 1 | Spec fraud / silent corruption (Tier 2 lie) | Plan concedes "Tier 1-minus-Aliased", keeps `sparseResidencyAliased=false`, refuses to patch vkd3d derivation. Override is opt-in per-title (`VKD3D_CAPS_OVERRIDE` / `VKD3D_FEATURE_LEVEL`) only for titles verified by census to never use tile mapping/aliasing. | **RESOLVED** |
| 2 | Inverted memory economics (JM/v14 16 GiB arena) | 16 GiB monolithic arena dropped. Replaced with per-resource SAME_VA `GROW_ON_GPF` regions freed with resource. Enforces 256 MiB context cap (`PANVK_SPARSE_CAP_MB`) and heap budget; over-cap reservations return `VK_ERROR_OUT_OF_DEVICE_MEMORY`. | **RESOLVED** |
| 3 | CSF unbound-tile group faults | S0-GPU kill-switch gate added before S1 to test GPF growth vs dummy zero sink on real HW; redundant GPU zeroing on CSF uncommit removed. | **CONDITIONALLY RESOLVED** (gated on HW test) |
| 4 | Bind sync / hot-path latency | Per-tile dirty bit tracking (writes back only dirty tiles); deferred CPU uncommit worker thread gated on bind fence with generation counter; resource frees wait on fence. | **RESOLVED** |
| 5 | Runtime target discrepancy & prerequisites | S-Pre prioritizes `robustImageAccess2` and SM 6.2+ float controls before sparse work. Both Wine and proton env vars documented. Census added to kill unnecessary driver stages. | **RESOLVED** |

---

## 3. Remaining Nuances & Council Conditions for S1

1. **Delete Dead Sink Path (b)**: `MEM_ALIAS` cannot be `FIXED` (`mem_linux.c:1759`). Sink path (b) cannot sit at a FIXED tile VA. The outcome space for S0-GPU is strictly `{GPF growth, eager}`.
2. **Census under Active Override**: Run title census with the override active, logging `CheckFeatureSupport(D3D12_OPTIONS).TiledResourcesTier` and runtime `UpdateTileMappings` calls to ensure titles don't dynamically enable tiled paths when seeing FL 12_0.
3. **Region Ceiling Cap on CSF**: Cap context-wide reserved regions (e.g. 4,096 regions max) alongside the 256 MiB byte cap to prevent kbase tracking exhaustion.
4. **Host-Visible Memory Exemption**: Do not skip copy-in on unwritten ranges if the memory type is host-visible (CPU writes can occur without GPU knowledge).
5. **Rebind Write-Back**: Rebinding a dirty tile to new memory must write back dirty contents before the new copy-in.
6. **Worker Drain on Resource Free**: Freeing resources or memory must drain the deferred uncommit worker before destroying kbase regions.

---

## 4. Final Council Verdict

**Approve for S-Pre (prerequisites), title census, and S0-GPU hardware kill-switch gate.**  
Hold driver stages S1–S6 until the S0-GPU gate passes on real devices (CSF v10/v11 and JM v9) and the census confirms whether any target titles actually invoke tiled-resource APIs.
