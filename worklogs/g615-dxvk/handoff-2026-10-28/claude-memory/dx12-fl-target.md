---
name: dx12-fl-target
description: "Target: D3D12 FL 12_0 and 12_1 on all Mali arches incl. v6/v7; FL 12_0 decoupled from sparse (Round 3 plan); sparse CSF v10-v14 only; FL 12_2 out of scope"
metadata:
  type: project
---

User (2026-10-27): v6/v7 should be fully DXVK-capable (per PanProbe) and run many DX12 titles; "12_2 only [out of reach], 12_0 and 12_1 should work".

**Why:** the user's goal is DX8-DX12 on every arch.

**How to apply:** treat any D3D12 failure at FL 11_x-12_1 as a bug. Known gaps to close: 12_0 needs tiled resources tier 2, resource binding tier 2+, typed UAV loads; 12_1 needs ROVs + conservative rasterization tier 1. Don't call these hardware limits. Exception, stated honestly: stock kbase UAPI cannot give `sparseResidencyAliased` (MEM_ALIAS is immutable and never FIXED), so the honest tiled tier is **Tier 1-minus-Aliased**, not conformant Tier 2. Never claim Tier 2.

Plan agreed 2026-10-27 (user: "do all steps fast after hang fix"): after the v10 hang fix, (1) v6/v7 11_1: raise per-stage storage images 32->64; (2) FL 12_0 override for 12_0 games; (3) sparse feasibility on kbase via MEM_ALIAS + sink page, PanProbe test v6-v11; (4) PanProbe FL report + PanPlay FL tests on all 5 Firebase devices.

Revised 2026-11-20 after Review Council Round 1 (4.7/10, 9 models). Full detail in worklogs/g615-dxvk/sparse-kbase-2026-10-27/README.md, section "Council Round 1 corrections".
- Override is opt-in, per title, never a global PanPlay default: Wine vkd3d 1.18 `VKD3D_CAPS_OVERRIDE=feature_level=12.0`, vkd3d-proton `VKD3D_FEATURE_LEVEL=12_0`. Only for titles that probe FL and never call `UpdateTileMappings` or alias tiles.
- JM v6/v7/v9 and v14: no 16 GiB eager arena. One SAME_VA GROW_ON_GPF region per sparse resource, context cap 256 MiB (`PANVK_SPARSE_CAP_MB`) and heap budget; over-cap reservations fail with `VK_ERROR_OUT_OF_DEVICE_MEMORY`.
- CSF lazy tiles: TMU filter/border taps on unbound tiles can fault the group. S0 GPU gate picks GPF growth or dummy zero sink (sink cannot be FIXED); else eager. No GPU zero fill on CSF uncommit (kernel zeroes on recommit).
- Bind engine: per-tile dirty bits (write back dirty tiles only); deferred uncommit worker gated on the bind fence, generation counter, frees wait on fence.
- Order: S-Pre (`robustImageAccess2` all arches + SM 6.2+ float controls) -> census of tiled-resource calls per target title -> S0 GPU kill-switch gate (read-0, boundary sampling, context survival on HW) -> S1-S6 only if the gate passes and the census shows a title needs it.

Plan of record since 2026-11-20: Council Round 3 (Gemini 4 Neon proposal). Overrides the Round 1 bullets above where they conflict. Full detail: README.md section "Round 3 authoritative plan".
- FL 12_0 is decoupled from sparse. Non-tiled titles get native FL 12_0 now via Phase 0 + per-title overrides.
- Sparse work is CSF v10-v14 only. JM v6/v7/v9 dropped from sparse scope.
- Phase 0 (S-Pre): robustness2 (`robustImageAccess2` + `nullDescriptor`), mutable descriptors, binding tier 2/3 limits; SM 6.2+ (float16/int16, float_controls, subgroups); per-title override profiles for Bucket A.
- Phase 1: census buckets A (non-tiled), B (narrow read-only SRV streaming), C (true Tier 2/aliasing, out of scope on kbase), D (non-sparse failures). Audit whether vkd3d allows `CreateReservedResource` with `sparseResidencyAliased=false`.
- Phase 2 S0-GPU on CSF v10/v11 + v14: A prefix commit/fault survival at 60 MiB in a 64 MiB region (`used_pages` +1 page or ~60 MiB?); B recycled-pool zero leak; C shrink latency under 60 FPS load; D mip-tail footprint on one `kbase_va_region`.
- Phase 3A (default): <2 Bucket B titles, or prefix explosion / CSG faults -> archive S1-S6, ship Phase 0 + overrides only.
- Phase 3B (bounded SRV-first shadow sparse): charge full virtual size on first mip-tail bind against `PANVK_SPARSE_CAP_MB`; explicit zero init if Test A/B fail; read-only SRV scope, whole-resource dirty tracking if writable (no per-tile bits); AFBC off + compute swizzle D3D12 64 KiB tile <-> Mali 16x16 u-interleaved; timeline-safe async queue, no mid-frame `MEM_COMMIT` shrink (reclaim only at destroy or zero-tile idle).

Update 2026-10-27: full sparse implementation is a TODO (user: "make sparse full impl as todo with research"); research in worklogs/g615-dxvk/sparse-kbase-2026-10-27/README.md, TODO note at top of docs/KBASE-SPARSE-FEASIBILITY.md. Only the S0 PanProbe `sparse_kbase` gate test is being added now.
