---
name: dx12-fl-target
description: "Target: D3D12 FL 12_0 on all Mali arches incl. v6/v7, decoupled from sparse (Round 3 plan of record, 7.83/10); sparse CSF v10-v14 only; FL 12_1 (ROV, conservative raster) deferred to separate branch; FL 12_2 out of scope"
metadata:
  type: project
---

User (2026-10-27): v6/v7 should be fully DXVK-capable (per PanProbe) and run many DX12 titles; "12_2 only [out of reach], 12_0 and 12_1 should work".

**Why:** the user's goal is DX8-DX12 on every arch.

**How to apply:** treat any D3D12 failure at FL 11_x-12_0 as a bug, not a hardware limit. Exception, stated honestly: stock kbase UAPI cannot give `sparseResidencyAliased` (MEM_ALIAS is immutable and never FIXED), so the honest tiled tier is **Tier 1-minus-Aliased**, not conformant Tier 2. Never claim Tier 2. FL 12_1 (ROV via `fragmentShaderPixelInterlock`, Conservative Rasterization Tier 1) is deferred to a separate investigation/branch and does not block FL 12_0.

Update 2026-10-27: full sparse implementation is a TODO (user: "make sparse full impl as todo with research"); TODO note at top of docs/KBASE-SPARSE-FEASIBILITY.md. Only the S0 PanProbe `sparse_kbase` CPU gate test exists.

## Plan of record: Council Round 3 (2026-11-20, 7.83/10 across 9 models)

Gemini 4 Neon proposal plus Round 3 consensus findings. This is the only governing plan. Full detail: worklogs/g615-dxvk/sparse-kbase-2026-10-27/README.md, section "Round 3 authoritative plan".

- **Decoupled.** FL 12_0 does not wait on sparse. Non-tiled titles get native FL 12_0 via Phase 0 + per-title overrides. Sparse work is CSF v10-v14 only; JM v6/v7/v9 get Phase 0 + overrides only.
- **Phase 0 (all arches):** robustness2 (`robustImageAccess2` + `nullDescriptor`; v10 RIA2 GPU-emulated, patch 184, do not re-gate); `VK_EXT_mutable_descriptor_type` + binding tier 2/3 limits; SM 6.2+ (float16/int16, `VK_KHR_shader_float_controls`, subgroups); opt-in per-title override profiles, never global (Wine vkd3d 1.18 `VKD3D_CAPS_OVERRIDE=feature_level=12.0`, vkd3d-proton `VKD3D_FEATURE_LEVEL=12_0`).
- **Phase 0 (v6/v7, required before Phase 0 is complete there):** min/max sampler filter emulation (`samplerFilterMinmax` / `filterMinmaxSingleComponentFormats`, native only at `PAN_ARCH >= 10`, so v9 too); typed UAV loads for all 18 FL 12_0 formats (`shaderStorageImageReadWithoutFormat`); storage images per stage 32->64.
- **Phase 1 census:** run with the FL 12_0 + Tier 2 probe shim active (census-only, never shipped) to catch dynamic engine branching. Log reserved-resource and tile-mapping calls, `CheckAccessFullyMapped` / DXIL residency feedback, `CreatePlacedResource` on reserved heaps, heap aliasing. Buckets: A non-tiled; B1 opportunistic tiled use with working fallback (ships like A, does not count for 3B); B2 hard-required read-only SRV streaming; C true Tier 2/aliasing (out of scope); D non-sparse failures. Audit whether vkd3d allows `CreateReservedResource` with `sparseResidencyAliased=false`.
- **Phase 2 S0-GPU (CSF v10/v11 + v14):** Test A = separate GPU READ and GPU WRITE at 60 MiB in a fresh 64 MiB `GROW_ON_GPF` region; record `used_pages` delta, fault latency, context survival. Test B recycled-pool zero leak. Test C shrink latency under 60 FPS load. Test D mip-tail footprint, plus evaluation of reverse-mip layout (mip tail at offset 0) and split mip-tail BO (separate committed BO) to decouple tail residency from mip 0.
- **Decision tree.** kbase contiguous-prefix GPF growth is expected kernel behaviour. Path 3A (archive S1-S6, ship Phase 0 + overrides) triggers **only** on (1) CSG fatal fault / context death in Test A, or (2) census finds <2 Bucket B2 titles. Prefix growth (Test A) or pool zero leak (Test B) does not kill 3B: it bans `GROW_ON_GPF` and switches 3B to explicit eager pre-commit or explicit GPU zeroing.
- **Path 3B (bounded SRV-first shadow sparse):** context caps of 256 MiB committed bytes (`PANVK_SPARSE_CAP_MB`, and heap budget) **and** 4,096 kbase regions (`PANVK_SPARSE_MAX_REGIONS`); over either -> `VK_ERROR_OUT_OF_DEVICE_MEMORY`. Without a mip-tail mitigation, first mip-tail bind charges full virtual size. Transfer writes (`CopyTextureRegion` / `CopyTiles`, CPU-known bounds) use per-tile dirty bits; shader UAV/RT/DS writes use whole-resource dirty. Never skip copy-in for host-visible memory. Rebind of a dirty tile to new memory writes back to old memory before copy-in. AFBC off; compute swizzle D3D12 64 KiB standard tile <-> Mali 16x16 u-interleaved, BC1-7 and RGBA8 first; investigate strided copy where standard swizzle is reported false. Timeline-safe async bind queue, no mid-frame `MEM_COMMIT` shrink.
- **Reclaim:** only at destroy or zero-tile idle = zero bound tiles for N >= 60 consecutive frames AND last submit fence signaled. Resource / memory destruction drains the deferred uncommit worker before freeing kbase regions.
- **Withdrawn (tombstone):** dummy zero sink, path (b). `MEM_ALIAS` cannot be FIXED, so a sink never sits at a tile VA. Do not plan around it.

## Struck: superseded history

- ~~2026-10-27 order: v6/v7 storage images 32->64; FL 12_0 override; sparse via MEM_ALIAS + sink page on v6-v11; PanProbe/PanPlay FL tests.~~ Sink approach withdrawn; storage images moved into Phase 0.
- ~~Round 1 (2026-11-20, 4.7/10) bullets: JM v6/v7/v9 eager sparse regions; GPF growth or dummy sink chosen by S0; per-tile dirty bits for all writes; fence-gated uncommit worker; order S-Pre -> census -> S0 -> S1-S6.~~ Replaced in full by the Round 3 plan above.
