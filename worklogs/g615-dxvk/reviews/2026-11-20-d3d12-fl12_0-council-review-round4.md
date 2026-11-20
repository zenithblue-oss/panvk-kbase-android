# D3D12 Feature Level 12_0 Architecture Council Review — Round 4

Date: 2026-11-20  
Branch: `feature/dx12-12_0-implementation`  
Subject: Round 4 Architecture Council Review of finalized D3D12 FL 12_0 Plan of Record

Detailed Per-Model Round 4 Reports:
- [Claude Sonnet 4.6 (8.9/10)](models/claude-sonnet-4-6-round4.md)
- [Hy4 Flash Free (8.8/10)](models/hy4-flash-round4.md)
- [Beam Free (8.5/10)](models/beam-free-round4.md)
- [MiMo V2.6 Flash Free (8.5/10)](models/mimo-v2.6-flash-round4.md)
- [Claude Opus 5.5 (8.4/10)](models/claude-opus-5-5-round4.md)
- [Grok 4.7 High (8.4/10)](models/grok-4.7-high-round4.md)
- [Gemini 4 Neon (8.2/10)](models/gemini-4-neon-round4.md)
- [DeepSeek V4.1 Flash Free (8.0/10)](models/deepseek-v4.1-flash-round4.md)
- [GPT-6.2 Astra (8.0/10)](models/gpt-6.2-astra-round4.md)

---

## 1. Council Score Progression Across All 4 Rounds

| Delegate | Provider | Round 1 | Round 2 | Round 3 | Round 4 | Total Delta (R1 ➔ R4) | Detailed Round 4 Report |
|---|---|:---:|:---:|:---:|:---:|:---:|---|
| Claude Sonnet 4.6 | Claude | 6.0/10 | 7.5/10 | 7.5/10 | **8.9/10** | +2.9 | [`claude-sonnet-4-6-round4.md`](models/claude-sonnet-4-6-round4.md) |
| Hy4 Flash Free | OpenCode | 4.0/10 | 7.0/10 | 8.0/10 | **8.8/10** | +4.8 | [`hy4-flash-round4.md`](models/hy4-flash-round4.md) |
| Beam Free | OpenCode | 6.0/10 | 7.0/10 | 8.5/10 | **8.5/10** | +2.5 | [`beam-free-round4.md`](models/beam-free-round4.md) |
| MiMo V2.6 Flash Free | OpenCode | 6.0/10 | 8.0/10 | 8.0/10 | **8.5/10** | +2.5 | [`mimo-v2.6-flash-round4.md`](models/mimo-v2.6-flash-round4.md) |
| Claude Opus 5.5 | Claude | 5.5/10 | 7.0/10 | 7.4/10 | **8.4/10** | +2.9 | [`claude-opus-5-5-round4.md`](models/claude-opus-5-5-round4.md) |
| Grok 4.7 High | OpenCode | 4.0/10 | 7.0/10 | 7.5/10 | **8.4/10** | +4.4 | [`grok-4.7-high-round4.md`](models/grok-4.7-high-round4.md) |
| Gemini 4 Neon | Google | — | 6.5/10 | 7.6/10 | **8.2/10** | +1.7 | [`gemini-4-neon-round4.md`](models/gemini-4-neon-round4.md) |
| DeepSeek V4.1 Flash Free | OpenCode | 4.0/10 | 7.5/10 | 8.0/10 | **8.0/10** | +4.0 | [`deepseek-v4.1-flash-round4.md`](models/deepseek-v4.1-flash-round4.md) |
| GPT-6.2 Astra | Codex | 4.0/10 | 6.5/10 | 8.0/10 | **8.0/10** | +4.0 | [`gpt-6.2-astra-round4.md`](models/gpt-6.2-astra-round4.md) |

**Council Average: 4.72 / 10 (R1) ➔ 7.11 / 10 (R2) ➔ 7.83 / 10 (R3) ➔ 8.41 / 10 (R4)** (+3.69 total improvement across council passes)

---

## 2. Verdict on Round 4 Plan of Record

**UNANIMOUS APPROVAL TO EXECUTE.** All 9 delegates approve the execution of Phase 0, Phase 1 (Title Census), and Phase 2 (S0-GPU Hardware Reality Gate). The internal contradictions from earlier rounds are fully closed.

### Key Resolution Summary

1. **Test A Contradiction Closed**: Expected kernel prefix growth (~60 MiB) is separated from context failure; it routes to mechanism changes (banning `GROW_ON_GPF` magic in Path 3B) rather than triggering the Path 3A kill switch. Path 3A triggers strictly on CSG fatal fault / context death or <2 Bucket B2 titles.
2. **Phase 0 Completeness on Bifrost v6/v7**: All three missing non-sparse FL 12_0 prerequisites (min/max filter emulation, 18-format typed UAV loads, and 64 storage images per stage) are formally integrated into Phase 0. FL 12_1 is cleanly deferred.
3. **Bounded Context Caps**: Memory cap (256 MiB) and region-tracking cap (`PANVK_SPARSE_MAX_REGIONS=4096`) are aligned (4096 × 64 KiB = 256 MiB) and dual-enforced.
4. **Census Hardening**: Runs with the FL 12_0 + Tier 2 probe shim active to capture dynamic engine branching; logs `CheckAccessFullyMapped`, placed-resource-on-reserved, and heap aliasing; splits Bucket B into B1 (opportunistic) and B2 (hard-required).
5. **Dirty Tracking & Transfer Bounds**: Per-tile dirty tracking for transfer writes (`CopyTextureRegion`/`CopyTiles`) where CPU knows bounds; whole-resource dirty tracking for shader UAV/RT writes. Host-visible copy-in is never skipped; dirty tiles are written back before rebind.
6. **Reclaim & Worker Drain**: Zero-tile idle requires N>=60 consecutive frames and submit fence signaled; resource/memory destruction drains the deferred uncommit worker before destroying kbase regions.
7. **Document Hygiene**: Dead sink path (b) tombstoned; superseded Round 1 mechanism bullets struck.

---

## 3. Final Execution Orders

| Stage | Action | Status |
|---|---|:---:|
| **Phase 0 (S-Pre)** | Execute immediately on all architectures (CSF v10–v14 and JM v6/v7/v9). | **GO** |
| **Phase 1 (Census)** | Run title census with probe shim active; audit vkd3d `CreateReservedResource`. | **GO** |
| **Phase 2 (S0-GPU)** | Run Tests A–D on CSF v10/v11 and v14 hardware. | **GO** |
| **Path 3A vs 3B** | Branch conditionally based on Census Bucket B2 count and S0-GPU survival. | **HOLD (Gated)** |
