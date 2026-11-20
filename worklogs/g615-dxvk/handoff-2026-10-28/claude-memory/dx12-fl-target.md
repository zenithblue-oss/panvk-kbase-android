---
name: dx12-fl-target
description: "Target: D3D12 FL 12_0 and 12_1 working on all Mali arches incl. v6/v7; only FL 12_2 out of scope"
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

Update 2026-10-27: full sparse implementation is a TODO (user: "make sparse full impl as todo with research"); research in worklogs/g615-dxvk/sparse-kbase-2026-10-27/README.md, TODO note at top of docs/KBASE-SPARSE-FEASIBILITY.md. Only the S0 PanProbe `sparse_kbase` gate test is being added now.
