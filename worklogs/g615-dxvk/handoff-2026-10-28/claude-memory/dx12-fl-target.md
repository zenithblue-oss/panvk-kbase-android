---
name: dx12-fl-target
description: "Target: D3D12 FL 12_0 and 12_1 working on all Mali arches incl. v6/v7; only FL 12_2 out of scope"
metadata:
  type: project
---

User (2026-10-27): v6/v7 should be fully DXVK-capable (per PanProbe) and run many DX12 titles; "12_2 only [out of reach], 12_0 and 12_1 should work".

**Why:** the user's goal is DX8-DX12 on every arch.

**How to apply:** treat any D3D12 failure at FL 11_x-12_1 as a bug. Known gaps to close: 12_0 needs sparse / tiled resources tier 2 (blocked on kbase per upstream-sync report), resource binding tier 2+, typed UAV loads; 12_1 needs ROVs + conservative rasterization tier 1. Don't call these hardware limits.

Plan agreed 2026-10-27 (user: "do all steps fast after hang fix"): after the v10 hang fix, (1) v6/v7 11_1: raise per-stage storage images 32->64; (2) PanPlay sets VKD3D_FEATURE_LEVEL=12_0 so 12_0 games run on all arches; (3) sparse feasibility on kbase via MEM_ALIAS + sink page, PanProbe test v6-v11; (4) PanProbe FL report + PanPlay FL tests on all 5 Firebase devices.

Update 2026-10-27: full sparse implementation is a TODO (user: "make sparse full impl as todo with research"); research in worklogs/g615-dxvk/sparse-kbase-2026-10-27/README.md, TODO note at top of docs/KBASE-SPARSE-FEASIBILITY.md. Only the S0 PanProbe `sparse_kbase` gate test is being added now.
