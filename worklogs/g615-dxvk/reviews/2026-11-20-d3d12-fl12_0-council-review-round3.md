# D3D12 Feature Level 12_0 Architecture Council Review — Round 3

Date: 2026-11-20  
Branch: `feature/dx12-12_0-implementation`  
Subject: Round 3 Architecture Council Review of adopted plan of record (Gemini 4 Neon Round 3 Decoupled Proposal)

Detailed Per-Model Round 3 Reports:
- [Gemini 4 Neon (7.6/10)](models/gemini-4-neon-round3.md)
- [Beam Free (8.5/10)](models/beam-free-round3.md)
- [DeepSeek V4.1 Flash Free (8.0/10)](models/deepseek-v4.1-flash-round3.md)
- [GPT-6.2 Astra (8.0/10)](models/gpt-6.2-astra-round3.md)
- [Hy4 Flash Free (8.0/10)](models/hy4-flash-round3.md)
- [MiMo V2.6 Flash Free (8.0/10)](models/mimo-v2.6-flash-round3.md)
- [Claude Sonnet 4.6 (7.5/10)](models/claude-sonnet-4-6-round3.md)
- [Grok 4.7 High (7.5/10)](models/grok-4.7-high-round3.md)
- [Claude Opus 5.5 (7.4/10)](models/claude-opus-5-5-round3.md)

---

## 1. Council Score Progression Across Rounds

| Delegate | Provider | Round 1 | Round 2 | Round 3 | Delta (R1 ➔ R3) | Detailed Round 3 Report |
|---|---|:---:|:---:|:---:|:---:|---|
| Beam Free | OpenCode | 6.0/10 | 7.0/10 | **8.5/10** | +2.5 | [`beam-free-round3.md`](models/beam-free-round3.md) |
| DeepSeek V4.1 Flash Free | OpenCode | 4.0/10 | 7.5/10 | **8.0/10** | +4.0 | [`deepseek-v4.1-flash-round3.md`](models/deepseek-v4.1-flash-round3.md) |
| GPT-6.2 Astra | Codex | 4.0/10 | 6.5/10 | **8.0/10** | +4.0 | [`gpt-6.2-astra-round3.md`](models/gpt-6.2-astra-round3.md) |
| Hy4 Flash Free | OpenCode | 4.0/10 | 7.0/10 | **8.0/10** | +4.0 | [`hy4-flash-round3.md`](models/hy4-flash-round3.md) |
| MiMo V2.6 Flash Free | OpenCode | 6.0/10 | 8.0/10 | **8.0/10** | +2.0 | [`mimo-v2.6-flash-round3.md`](models/mimo-v2.6-flash-round3.md) |
| Gemini 4 Neon | Google | — | 6.5/10 | **7.6/10** | +1.1 | [`gemini-4-neon-round3.md`](models/gemini-4-neon-round3.md) |
| Claude Sonnet 4.6 | Claude | 6.0/10 | 7.5/10 | **7.5/10** | +1.5 | [`claude-sonnet-4-6-round3.md`](models/claude-sonnet-4-6-round3.md) |
| Grok 4.7 High | OpenCode | 4.0/10 | 7.0/10 | **7.5/10** | +3.5 | [`grok-4.7-high-round3.md`](models/grok-4.7-high-round3.md) |
| Claude Opus 5.5 | Claude | 5.5/10 | 7.0/10 | **7.4/10** | +1.9 | [`claude-opus-5-5-round3.md`](models/claude-opus-5-5-round3.md) |

**Council Average: 4.72 / 10 (Round 1) ➔ 7.11 / 10 (Round 2) ➔ 7.83 / 10 (Round 3)** (+3.11 total improvement)

---

## 2. Verdict on Round 3 Architecture

Round 3 represents a complete architectural consensus. All 9 council models approve proceeding with Phase 0 (S-Pre), Phase 1 (Title Census & vkd3d audit), and Phase 2 (S0-GPU Hardware Reality Gate).

### Key Architectural Strengths of Round 3
1. **Decoupled FL 12_0 Enablement**: Phase 0 (S-Pre) + per-title overrides deliver FL 12_0 for Bucket A non-tiled titles immediately without waiting on sparse emulation.
2. **Narrowed Hardware Scope**: Dropped JM v6/v7/v9 from sparse scope; CSF v10–v14 only. Prevents divergence between JM job-chain faults and CSF CSG fatal faults.
3. **Honest Contiguous-Prefix Accounting**: Charges full virtual image size on first mip-tail bind against `PANVK_SPARSE_CAP_MB`, eliminating the mip-tail memory bomb.
4. **Bindless-Safe Dirty Tracking**: Dropped fake per-tile dirty bits; replaced with read-only SRV scoping or whole-resource dirty tracking.
5. **Concrete Layout Contract**: Mandatory AFBC disable + compute-shader 2D tile swizzle between D3D12 64 KiB blocks and Mali 16×16 u-interleaved layout.
6. **Banned Mid-Frame Reclaim**: Prohibits mid-frame `MEM_COMMIT` shrink, avoiding `mmap_lock` and AS-wide MMU/L2/LSC flush stalls.
7. **Default-to-Kill Gate Structure**: Path 3A archives sparse work cleanly if the title census finds <2 Bucket B games or S0-GPU shows prefix explosion or CSG faults.

---

## 3. Actionable Consensus Recommendations for Implementation

1. **Phase 0 Completeness**: Ensure Phase 0 encompasses typed UAV loads and SM 6.2+ float controls before shipping overrides.
2. **Context Region Cap on CSF**: Supplement the 256 MiB byte cap with a context-wide region limit (4,096 regions max).
3. **Census under Active Override**: Run census with override active to catch titles that dynamically branch into tiled paths upon detecting FL 12_0.
4. **S0-GPU Hardware Gate Execution**: Run Tests A–D on CSF v10/v11 and v14 before any S1–S6 driver code is written.
