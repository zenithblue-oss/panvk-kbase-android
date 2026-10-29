# PanVK kbase: emulated-feature performance plan (native-first)

Goal: make every emulated Vulkan feature in the PanVK kbase driver cheaper, use the hardware wherever an arch
has a native path, and track the gains against RC4 in [PERF-PROGRESS.md](PERF-PROGRESS.md).

Inputs:
- ARM driver feature tables per arch: [MALI-VULKAN-NATIVE-FEATURES.md](MALI-VULKAN-NATIVE-FEATURES.md)
  (section 8: native vs emulated).
- Source: the build 116 tree (`/var/tmp/panvk/mesa-116-final`, Mesa 5a07217f + series through jm-v7-wip 0151).
  `vk/` below is `src/panfrost/vulkan/`.
- Earlier reviews: `worklogs/g615-dxvk/perf-review-2026-10-26/` (ids A*, B*, C*, D*, Q* below), the jm-v7-wip
  README build sections 115f/115g (tessellation cost analysis).
- Measurements: PanProbe on TB336FU (Mali-G57, v9 JM) and POCO X6 Pro (Mali-G615, v11 CSF), 2026-10-29.

Arch map: JM = v6 (G72), v7 (G52/G76), v9 (G57/G68/G78, Valhall JM). CSF = v10 (G610/G710), v11 (G615/G715),
v12 (G720/G620), v13 (G725/G925), v14 (G1). v15 (G2) is not in Mesa yet (no `genxml/v15.xml`).

## 1. Where the time goes today (measured)

| Workload | TB336FU v9 | X6 Pro v11 | Source |
|---|---|---|---|
| Tess draw, 16 quad patches, L=1 (fixed cost) | 108 us/draw | 374 us/draw | tess_perf, latest |
| Tess draw, L=16 | 1.50 ms | 1.15 ms | tess_perf, latest |
| Tess + passthrough GS, L=16 | 10.6 ms (7x the tess-only draw) | 2.96 ms (2.6x) | tess_perf gs=1 |
| BC1 upload+decode, 2048^2 + mips | 38.8 ms (RGBA8 copy: 6.3 ms) | 2.3 ms (RGBA8: 2.4 ms) | bc_perf upload_gpu_ms |
| BC6H / BC7 upload+decode | 98-117 / 78 ms | 4.4-4.7 / 5.6-5.8 ms | bc_perf |
| BC lowmem transcoder pipelines, cold shader cache | n/a (no lowmem on v9) | 5.0 s (rc4 0.7 s) | 115d README |
| CSF tess hand-off (compute <-> vertex/tiler CSGs) | n/a | about 370 us/draw | 115f README |

Rule of thumb from the numbers: on v9 the BC compute decode and the GS emulation are the largest emulation costs;
on CSF the fixed per-draw cost of tessellation (hand-offs and drains between subqueues) dominates small draws.

## 2. Feature inventory

Cost column: measured where a PanProbe test isolates it, otherwise an estimate with its basis. Native column:
what the hardware or the ARM driver offers per arch (ARM = ARM proprietary driver on gpuinfo, see the native
doc). "None" means no fixed-function path on any Mali; ARM also emulates or does not expose the feature.

| Feature | Current mechanism (arch) | Cost | Native availability | Candidate optimisations |
|---|---|---|---|---|
| Geometry shaders | v6/v7: `panvk_nir_lower_gs` (GS as compute, `vk/panvk_vX_shader.c:5251`); v9+: gpu_prerast, sw VS + GS kernel compute then a passthrough draw (`vk/csf/panvk_vX_cmd_draw.c:3894`, v9 JM shares `csf/panvk_gpu_prerast.c`) | gs=1 L=16 tess: +9.1 ms/draw v9, +1.8 ms v11 over gs=0; `geometry` test 1.25 s v9 | None (ARM also emulates on v6-v15) | 1) let the 1x1x1-workgroup prerast kernels merge workgroups (**done, 0151**); 2) detect pass-through GS (one output prim per input, identity copy) and skip the GS kernel; 3) one GS invocation per thread group of primitives with a single index-append atomic per warp |
| GS viewport index / multiViewport | v6/v7: union scissor + FS discard on `vp_scissor`, per-viewport split when depth ranges differ (jm draw:1268, 1361); v9+: `vp_runs` compute pass builds ordered runs (csf draw:4510) | `multi_viewport` 1.09 s v9, 1.17 s v11 (test time, mostly fixed) | v14/v15: ARM maxViewports 16, likely native; genxml v10-v14 have `Scissor array enable` (never set, layout undocumented) | v14: decode `Scissor array enable` + viewport index attribute and drop vp_runs (needs RE, no device); v9+: skip vp_runs when every primitive selects viewport 0 (GPU flag) |
| Tessellation | v6/v7: TCS compute + `panlib_v7_tessellate` + TES as VS (`panvk_tess_draw`); v9 JM: `v9_tess_draw` unrolled iterations (0142); CSF: `gpu_prerast_tess` loop on compute, TESS_READY/FREE syncs to the vertex/tiler subqueue (csf draw:4994-5556) | v9 L=1 108 us, L=16 1.5 ms; v11 L=1 374 us, L=16 1.15 ms; 0142/0143 already -50 % | None (ARM also emulates; no tess fixed function) | 1) libpan kernels merge workgroups on v9 JM (**done, 0150**); 2) CSF: run the compute steps on the vertex/tiler CS (one CSG, no cross-CSG hand-off, about 300 us/draw); 3) CSF: narrow the 4-5 full drains per iteration to the iteration slot (`cs_wait_slot(SB_ITER)`), drop the drain right before `give` (13 `all_iters_mask` waits in the loop); 4) parallel tessellator (one thread per ring/row, L=16 is 1 us per index); 5) v6/v7 TES once per domain point (D9), rotating arenas (D10) |
| Transform feedback | gpu_prerast XFB pass (v9+, csf draw:4082); v6/v7 inside the GS emulation (`xfb_table`, scan/copy jobs) | `xfb` 0.21 s v9, 0.32 s v11 | None (ARM 1 stream, emulated) | covered by the prerast merge (0151); skip the passthrough draw when rasterizerDiscard |
| fillModeNonSolid | v6/v7 passthrough GS; v9+ polygon kernel (`panvk_gpu_prerast_polygon_nir`) | `fill_mode` 1.16 s v9 | None (ARM false everywhere) | polygon kernel merges workgroups (0151); points/lines via tiler primitive type for simple cases (no clip) |
| vertexPipelineStoresAndAtomics | v9+: a VS that writes memory runs through gpu_prerast (csf draw:5574); v6/v7 vertex job per index | `vertex_stores` 0.12 s | None (ARM false everywhere) | prerast merge (0151); v13+: IDVS with a store-safe variant if the speculation hazard is absent there (stale drirc text says v13+; needs proof) |
| Primitive restart under GS | v6/v7: serial `panlib_v7_gs_restart_list`; v9+: in-kernel restart with `panlib_gs_strip_table` (one workgroup) | estimate: serial pass, O(indices) single thread | None | parallel prefix strip table (scan) instead of one thread |
| Pipeline statistics | GPU counters by kernels (`panlib_update_pstats_draw_struct`), v6/v7 per-core sums | `pipeline_stats` 1.15 s v9 (fixed) | ARM false everywhere | skip the counter kernels when no pstats query is active (check that the gate already covers all paths) |
| multiDrawIndirect / drawIndirectCount | JM: CPU loop, `drawCount` jobs, v9 unrolled NULL jobs past the GPU count; CSF: native `cs_while` loop (csf draw:3480) | JM: one job set per draw | CSF native (ARM true v10+), JM none | nothing on CSF; JM: cap unrolled jobs by maxDrawCount hints |
| depthBounds | FS lowering (LD_TILE depth, SampleMask), unlowered `alt` variant when the test is off (C1, 115), device stand-in FS for depth-only pipelines | 0.5-3 ms per full-screen pass before C1 (review C1) | ARM false everywhere | done (115 C1, A1); remaining: early-ZS variant when bounds and depth test are both static |
| Clip / cull distance | FS discard on interpolated distances (`panvk_lower_clip_cull_fs`) | `clip_cull` 1.13 s; disables early-ZS when used | ARM false everywhere | cull whole primitives in gpu_prerast when it runs anyway; leave FS discard for the rest |
| BC1-BC7 | v6/v7/v9 and 3D images: compute decode to RGBA8 (8x8 workgroups, `vk/bc/*.comp`); v10+ 2D: fragment decode to AFRC/compact shadows, lowmem transcode to ETC2/EAC (BC1-5) and ASTC/BC6H paths | v9 BC1 38.8 ms vs RGBA8 6.3 ms (6x), BC6H 98-117 ms, BC7 78 ms per 2048^2 upload; v11 at par with RGBA8; v11 lowmem cold compile 5.0 s | v6-v14 none; **v15: ARM exposes 16/16 BC formats** (gpuinfo 51699/51676) | 1) v15: native BC (below); 2) v6/v7/v9: decode one block per thread (16 texels) instead of one texel per thread re-decoding its block; 3) v9: decode to ETC2/ASTC instead of RGBA8 (v9 has native ETC2/ASTC) to cut the 4x memory; 4) v10+ lowmem: compile the transcoder pipelines on a background thread at device creation, or ship them precompiled like libpan |
| robustImageAccess2 | v9/v10: `bi_lower_robust_image_alpha_v10` + texel-buffer lowering; v6/v7: `panvk_v7_lower_robust_image2` | `robust_image_access2` 6.5 s v9 (test time incl. many pipelines) | ARM: none on v9/v10, Y on v14/v15 | v11+ have no alpha fix (already "native"); v14: check whether the HW returns (0,0,0,1) and the v14 path needs nothing (no device) |
| Null descriptors | v6/v7 zero BO + dummy descriptors; v9+ compiler `robust_descriptors` | negligible | ARM Y on v9/v10/v14/v15 | none needed |
| Bindless (v6/v7) | update-after-bind and mutable bindings bindless, `PANVK_V7_UAB_*` | per-access indirection | none (Bifrost tables) | none planned |
| int64 atomics (v6/v7) | `nir_lower_atomics` to a 64-bit CAS loop | contention-dependent | v9+ native | none planned |
| Logic op | blend shader for every logic op; `any_dest_read` turned FPK off for every logic op (0138) | FPK lost on every logicOp draw (D3D11.1 / D3D12 FL11_1) | ARM: logicOp native-looking on v7, v9, v10, v12+ | 1) keep FPK for CLEAR/COPY/COPY_INVERTED/SET (**done, 0149**); 2) fixed-function logic op if the blend descriptor has it (RE needed) |
| Blend shader fallbacks | float RT with DST/SRC colour factors, MIN/MAX, unblendable formats (review C10) | unknown, count first | n/a | count blend-shader draws per frame first (C10) |
| Sparse | v10+ non-kbase only | n/a on kbase | ARM none | none |
| Device TLS (scratch) on v14 | jm-v7-wip 0148: no sparse alias of the device TLS on v14 (one full BO), fixes the v14 DEVICE_LOST and tessellation mismatch | 176 MiB TLS on the G1-Ultra instead of 96 MiB with the sparse alias | n/a | a correct sparse alias for the v14 core-ID layout (core mask 0x3300ff: the core IDs are not contiguous, so the alias must cover the IDs the hardware uses, not the core count); needs the RTL v14 device |
| Compute dispatch on v9 JM | "Allow merging workgroups" never set on JM | small workgroups waste lanes | v9 hardware has the bit (`Compute Payload`) | set it from the shader (**done, 0150**) |

### v15 (Mali-G2) native BC

gpuinfo 51699 and 51676 (Mali-G2-Ultra-NX MC16, ARM r56p0-15bet0) report `textureCompressionBC` and all 16 BC
formats with sampled, linear-filter, blit-src and transfer bits; 52414 (r56p0-18eac0) exposes none, so exposure
depends on the ARM build, but the hardware decodes BC. v6-v14 have no BC in any report.

Mesa state: the BC pixel-format encodings exist (`genxml/v10.xml` BC1..BC7 values 7-16, `lib/pan_format.c:243-258`
`FMTC(...BC*...)`), and `panvk_bc_emul_enabled()` (`vk/panvk_physical_device.c:1968`) already turns BC emulation
off when the GPU's texture_features report every BC format native. What is missing is v15 itself: no
`genxml/v15.xml`, no v15 GPU IDs, no `PAN_ARCH == 15` build. Item: once v15 support exists, check that the G2
texture_features words report BC and that the v10-v14 encodings are unchanged, then BC goes native with no
emulation code involved. **Blocked on v15 arch support; untested (no v15 device).**

## 3. Ranked candidates

Gain = expected effect on the measured workloads; risk = chance of a correctness regression; effort = rough size.

| Rank | Item | Arch | Gain | Risk | Effort |
|---|---|---|---|---|---|
| 1 | gpu_prerast kernels merge workgroups (GS, XFB, polygon, VS-stores, TES-as-VS) | v9+ | high for GS/XFB/tess+GS | low-med (divergence options) | small, **0151** |
| 2 | v9 JM compute jobs set "Allow merging workgroups" | v9 | med (libpan tess kernels, small-group dispatches) | low (CSF already does it with the same binaries) | small, **0150** |
| 3 | Logic ops that ignore the destination keep FPK | all | low-med (FL11_1 / D3D11.1 logic op users) | low | tiny, **0149** |
| 4 | BC compute decode one block per thread | v6/v7/v9 | high for load times (v9 BC1 6x RGBA8) | med (decode correctness, bc_decode hashes) | med |
| 5 | CSF tess compute on the vertex/tiler CS | v10+ | high for small tess draws (~300 us/draw) | high (IDVS state re-emit, scoreboards) | large |
| 6 | CSF tess: narrow drains, drop duplicate drain before give | v10+ | low-med | med (sync) | small |
| 7 | BC lowmem transcoder: background precompile or shipped binaries | v10+ | removes a 5 s first-use stall | low (background) / med (shipped) | med |
| 8 | Pass-through GS detection | v9+ | high for the GS-only-for-XFB/layer cases | med | med |
| 9 | Parallel tessellator (per ring/row) | all | med at L>=16 | med | large |
| 10 | v9: BC decode into ETC2/ASTC shadows | v9 | memory /4, bandwidth | med | med |
| 11 | v14 native scissor array / viewport index | v14 | removes vp_runs on v14 | high (undocumented, no device) | large |
| 12 | v15 native BC | v15 | removes all BC emulation | blocked (no v15 in Mesa) | small once v15 exists |
| 13 | Primitive-restart strip table as a parallel scan | v9+ | low-med with restart + GS | low | small |
| 14 | v6/v7 TES once per domain point (D9), rotating arenas (D10) | v6/v7 | med-high at high levels | med-high | med |
| 15 | v14 sparse device TLS alias for core mask 0x3300ff (memory, after 0148) | v14 | 80 MiB less TLS | med (the alias bug 0148 removed) | small-med |

## 4. Ordered work list

1. Build 116 (**done**, default bundled driver): 0149 (logic op FPK), 0150 (v9 JM merge), 0151 (prerast merge).
   PanProbe 40/40 on TB336FU and X6 Pro, PanPlay s3/s5/s6/s7/s8 clean. Results in `docs/PERF-PROGRESS.md`.
   Each has a knob (`PANVK_LOGICOP_FPK`, `PANVK_V9_JM_MERGE_WG`, `PANVK_PRERAST_MERGE_WG`, `=0` restores the
   old behaviour) for A/B and bisection.
   Exception in 0151: on v9 a lowered vertex shader that reads vertex inputs does not merge. With merging,
   `draw_params` case `divisor` (1 vertex x 6 instances, divisor 2) read attribute 1004 instead of 1005 for
   instances 6 and 7 on TB336FU; CSF passed. Root cause not found (likely the v9 JM instance/attribute path
   under merged warps). Cost of the exception on v9: `large_draw` 10.2 s instead of 7.3 s with full merging.
1a. Root-cause the v9 merged-VS instance-divisor bug and drop the exception (v9 `large_draw` about -30 %).
   **Done, 0152**: on the G57 an instance-frequency `LD_ATTR` with merged workgroups returns the element of the
   first lane of each 4-lane group (hardware divisor or not); the v9 lowered VS now computes instance / divisor
   itself and fetches with vertex frequency. `large_draw` 10.2 s to 7.2 s on TB336FU.
2. Firebase/RTL follow-up for 116 on v6/v7/v10/v14 (fills the empty PERF-PROGRESS columns; 0149 and 0151 touch
   every arch, 0150 only v9).
3. BC compute decode per block on v6/v7/v9 (rank 4), checked with the `bc_decode` hashes and `bc_perf`.
4. CSF tess drains (rank 6), then the single-CS tess design (rank 5) as its own build.
5. BC lowmem background precompile (rank 7).
6. Pass-through GS detection (rank 8) and the parallel strip table (rank 13).
7. Longer term: parallel tessellator, v14 scissor array, v15 native BC once Mesa has v15.
