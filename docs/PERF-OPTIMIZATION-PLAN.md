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
| Conditional rendering | JM v6/v7/v9: a 1-thread `panlib_v9_cond_render` compute job before every predicated draw/dispatch rewrites its job headers to NULL (jm-v9 030, jm-v7 040); CSF: native `cs_if` on the predicate, plus in the tess loop the first compute iteration zeroes the copied draw and both loops still run to release the arenas (csf-v11 087) | estimate: JM one extra job + barrier per predicated draw (~20-50 us on v9, serialises the chain); CSF tess: a skipped tess draw still pays the full loop fixed cost (~370 us v11) | CSF predicate native (ARM exposes it v10+); JM none | JM: one predicate job per run of consecutive predicated draws with the same predicate (patch all their headers); CSF tess: branch over the whole loop when the predicate fails and only release the arenas. Measure first (no PanProbe test isolates it; `tess_cond_state` is correctness only) |
| VK_EXT_primitives_generated_query | v10+: CS counter adds per draw, GS primitives added by the gpu_prerast GS kernel (csf-v11 073); v9: `panlib_update_pstats_draw` GPU job per API draw (jm-v9 016) | v10+ near zero (CS adds); v9 one compute job per draw while the query is active | ARM exposes it v10+ (counter based) | v9: skip the per-draw kernel when only the IA count is needed and the draw is direct (CPU-known count, one WRITE_VALUE add). Measure first |
| Occlusion / timestamp queries, availability | JM: occlusion EndQuery availability deferred to batch close (jm-v7 100, `PANVK_V7_QUERY_END_DEFER`); timestamps = WRITE_VALUE SYSTEM_TIMESTAMP job with BASE_JD_REQ_PERMON (jm-v9 030, jm-v7 021); availability words in the cached pool on v6/v7 (jm-v7 021/071); CopyQueryPoolResults WAIT closes the batch | estimate: each timestamp is its own job and a PERMON atom (counter start/stop in kbase per atom); a WAIT copy splits the batch (one extra frag/tiler pass boundary). `occlusion_query` test is fixed-cost only | ARM: native timestamps everywhere (CSF has native `cs_store_timestamp`) | JM: batch consecutive timestamps of one batch into one job; keep PERMON only on atoms that carry a timestamp job (check it already is). Measure first (DXVK writes a timestamp pair per frame with the HUD) |
| customBorderColorWithoutFormat (v7) | when enabled, R/B-swapped formats (BGRA8, ...) stop using AFBC on v7 (jm-v7 044) | AFBC off for BGRA8 textures/RTs on v7 when an app enables the feature (DXVK does): bandwidth, estimate 10-30 % on texture-heavy BGRA draws | v7 AFBC has no R/B order; v9+ unaffected | keep AFBC and pre-swizzle the border colour per view (the sampler sees the view format at descriptor write time on v7) when the view is known; measure first on a v7 device |
| v9 per-sample shading, alpha-to-coverage, VMR; alphaToOne v10+ | v9 DCD `Evaluate per-sample` for sample shading or a blend shader with MSAA; VMR closes the batch on a sample-count change (jm-v9 018); alphaToOne v10+ forces alpha 1 in the blend shader (csf-v11 070) | per-sample: 4x FS cost at 4x MSAA when a blend shader is active (also for blend shaders that are not per-sample); alphaToOne: a blend shader for every draw that enables it (no FPK) | per-sample shading native; A2C native (DCD bit); alphaToOne: ARM true v10+ (likely FS output fixup) | only evaluate per-sample for blend shaders that read the destination per sample; alphaToOne: write alpha 1 in the FS epilogue (`nir_lower_alpha_to_one`) instead of forcing a blend shader. Measure first (no PanProbe MSAA perf test) |
| Dual-source blend | `maxFragmentDualSrcAttachments` = 1 (csf-v11 188); RT0 SRC1 through the blend shader / fixed-function per `blend_src1_type` | none beyond the limit (DXVK/vkd3d only use RT0) | 1 everywhere (ARM reports 1) | none planned |
| WSI present copies | Android: every present `ANativeWindow_lock` + one `memcpy` per row of the whole frame into the window buffer (`panvk_wsi.c`, only 4 Bpp buffers since 0146); X11 sw (launcher X server): copy into a MIT-SHM memfd + ShmPutImage, FIFO paced on the present thread (csf-v11 117, damage-rect clipped) | estimate (measure first, ms per present): 1080x2400x4 = 10 MiB per frame, about 2-4 ms of a CPU core at 3-5 GB/s; X11 one more copy in the server. Paid every frame in PanPlay | Android: AHardwareBuffer-backed swapchain images (zero copy) on every arch; X11 DRI3 is not available on the launcher X server | 1) log per-present copy time (`PANVK_FPS_LOG` line) and measure in PanPlay s3/s8; 2) Android: render into AHardwareBuffer/gralloc-imported images and queue them directly (no copy); 3) copy only damage rects on Android too |
| Tiler heap OOM and incremental rendering | kbase CSF: only VERTEX_TILER_STARTED heap ops (no FINISH_FRAGMENT), heap recycled by renewal with two TILER_HEAP slots and a worker-created next heap (csf-v11 043/098/102/104/107), the upstream incremental-rendering pass (`panvk_incremental_rendering_pass`) never triggers on kbase (single kbase group, no OOM handler fragment flush); JM: no incremental rendering, v9 2 x 512 MiB GROW_ON_GPF heaps | structural: a frame whose polygon lists exceed the heap is DEVICE_LOST instead of a partial flush; heap memory is sized for the worst frame (1 GiB of VA on v9, grown in 2 MiB steps) and renewal costs a heap context per 64k work units | ARM: incremental rendering on every arch (OOM event -> partial fragment pass) | CSF: handle the kbase tiler OOM event with an incremental fragment pass (needs the queue-group OOM event plumbing); JM: split a batch when the tiler heap high-water mark passes a threshold (CPU-side vertex estimate). Stability first, then memory; measure heap peak per PanPlay scene first |
| Other fallbacks from the patch subjects | v6/v7 int64 atomics as a CAS loop (jm-v7 047, row above); v9+/v10 robustImageAccess2 alpha fix-up (row above); TLS/WLS pool skips the kbase CPU clear (already a gain); query results block instead of spinning (already a gain); `minStorageBufferOffsetAlignment` 4 (no cost) | - | - | nothing new beyond the rows above |

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
| 16 | Android present: measure per-present copy, then zero-copy AHardwareBuffer swapchain | all | every frame, est. 2-4 ms CPU per present | med (WSI, gralloc variety) | measure small, fix large |
| 17 | Tiler OOM incremental rendering on kbase CSF; JM batch split on heap high-water | all | stability (DEVICE_LOST on huge frames), heap memory | high | large |
| 18 | alphaToOne in the FS epilogue instead of a blend shader; per-sample evaluation only when needed | v9+ | FPK back for alphaToOne draws; up to 4x FS at 4x MSAA | low-med | small, measure first |
| 19 | JM conditional rendering: one predicate job per run of draws; CSF tess: skip the whole loop when predicated off | all | low-med (predicated draws are rare outside D3D predication) | med (sync) | small-med, measure first |
| 20 | JM timestamps batched per batch, PERMON only where needed | v6/v7/v9 | low (DXVK HUD timestamp pairs) | low | small, measure first |
| 21 | v9 primitives-generated: CPU-known count for direct draws | v9 | low | low | small, measure first |
| 22 | v7 customBorderColorWithoutFormat keeps AFBC for BGRA | v7 | AFBC bandwidth on BGRA textures | med | small, measure first on v7 |
| 23 | GPU stats for MangoHud on Android (kbase/devfreq nodes are blocked for apps): the driver writes GPU load % (busy time from its per-submit GPU timestamps over a 500 ms window) and VRAM used (its own allocations) to a small file, gated on an env var the launcher sets only with MangoHud on; the launcher's MangoHud reads it | all | MangoHud GPU load / VRAM on Android (no perf gain; zero cost when off) | low | small |

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
1b. Build 118 = 0152 + 0153 (`git-a0412457ab`): X6 Pro 40/40 x2; v9 gate on Firebase oriole (G78) 40/40, so 118
   is the default bundled driver (**done**). Not done this round: CSF tess drains (rank 6) and the lowmem
   background precompile (rank 7; note it would cost every v10+ app start a cold compile thread even when it never
   uses BC, so gate it on the first BC image creation rather than device creation).
2. Firebase/RTL follow-up for 116 on v6/v7/v10/v14 (fills the empty PERF-PROGRESS columns; 0149 and 0151 touch
   every arch, 0150 only v9).
3. BC compute decode per block on v6/v7/v9 (rank 4), checked with the `bc_decode` hashes and `bc_perf`.
   **Done, 0153**: one invocation per 4x4 block (palette / subset endpoints once per block, BC7 via the fast
   decoders), 16-byte row stores; bit-identical. TB336FU upload GPU ms: BC1 38.4 to 13.9, BC3 56.5 to 15.6,
   BC6H 99-117 to 37-39, BC7 77.5 to 15.2 (RGBA8 6.4). v6/v7 use the same shaders, untested there.
4. CSF tess drains (rank 6), then the single-CS tess design (rank 5) as its own build.
5. BC lowmem background precompile (rank 7).
6. Pass-through GS detection (rank 8) and the parallel strip table (rank 13).
7. Longer term: parallel tessellator, v14 scissor array, v15 native BC once Mesa has v15.

## 5. Item status, round 2 (2026-10-29, ranks 5-23)

Measured with the new PanProbe perf test `workload_perf` (`device/workload-perf.c`, real-workload frames: DXVK
stream output and pass-through GS, emulator strips with restart, D3D predication, DXVK HUD timestamps,
primitives-generated queries, 4x MSAA alphaToOne, BGRA textures with a custom border colour, a cold BC level
load) plus `tess_perf`, `bc_perf` and the `swapchain_lifecycle` present timing. Numbers: `PERF-PROGRESS.md`,
section "Round 2". X6 Pro (v11) only: TB336FU (v9) is disconnected, RTL/Firebase belong to other agents.

| Rank | Item | Status |
|---|---|---|
| 5 | CSF tess on one CS | deferred: large, high risk (IDVS state re-emit inside the compute loop, scoreboards); `tess_perf` covers it. Next step: prototype the loop on the vertex/tiler CS behind a knob |
| 6 | CSF tess drains | closed, not worth it: every wait in the loop is a true dependency (each dispatch reads the previous one's output), a narrower `cs_wait_slot` waits for the same work; the second wait in `gpu_prerast_sync32_give` hits drained scoreboards (one CS instruction). The ~370 us/draw is the cross-CSG hand-off (item 5) |
| 7 | BC transcoder compile at the first BC image | **done, 0155**: `workload_perf bc_level` (cold cache, 6 BC formats 1024^2 + mips) 4.93 s to 2.57 s, device creation unchanged |
| 8 | Pass-through GS detection | **done, 0157** (v9+): a GS that copies every input vertex in order (no layer/viewport/primitive ID, no XFB, no memory writes, VS and GS output layouts equal) is skipped at draw time and the VS runs on the hardware; kept for pipeline statistics queries. `gs_pass` 111 to 4.5 us/draw, `strip_gs` 421 to 7.5 us/draw on v11. v9 (JM) path not measured: no v9 device |
| 16 | Android present copy | measured with the final build's PanPlay runs (0156 `post_*` fields), see PERF-PROGRESS Task C |
| 18 | alphaToOne in the FS epilogue | closed, not worth it: DXVK and vkd3d never enable alphaToOne (D3D has no alpha-to-one); on Mali ATEST takes the same RT0 alpha for alpha-to-coverage, so an FS-side alpha = 1 breaks alphaToCoverage + alphaToOne unless the FS gets a variant keyed on dynamic state; the per-sample cost (`msaa_a2one` 3.3x `msaa_ref`, = `msaa_sshade`) is the blend shader's single-sample ST_TILE design, a per-sample loop in the blend shader is a large change for a feature no target uses |
| 5, 9, 10, 11, 12, 14, 15, 17 | | deferred: 5 see above; 9 large (tessellator rewrite); 10 and 14 need a v9 / v6-v7 device (TB336FU disconnected, RTL/Firebase owned by other agents); 11 and 15 need a v14 device; 12 blocked (no v15 in Mesa); 17 large and high risk |
| 23 | MangoHud GPU stats | **done, 0158** (CSF v10+): per ring entry GPU timestamps (after cross-subqueue waits, after retire), union over the subqueues, file rewritten at most every 500 ms (`load=`, `vram=` = device memory + private BOs). No GPU clock: kbase has no cheap current-frequency query for apps. JM (v6/v7/v9) not done: needs timestamp write jobs in the JM chains; MangoHud shows 0 there. MangoHud reader in `runtimes/mangohud/android-bionic.patch`, PanPlay sets `PANVK_GPU_STATS` with MangoHud on and un-greys GPU load / VRAM |
| - | 0147 (v14 per-chunk prerast cache flush) | reverted by 0159 at the end of the series: the v14 A/B on 116 (bd2ac303) passed `tessellation` 3/3 without it, 0148 (sparse TLS) was the fix |
| 13 | Restart strip table as a parallel scan | closed: `panlib_gs_strip_table` already is a CHUNK_WG-thread segmented scan (one workgroup); `strip_gs` cost is the GS emulation itself (item 8) |
| 19 | Conditional rendering | CSF measured free (`cond_true` = `ref`, `cond_false` skips); JM part (v6/v7/v9) not measurable without a JM device |
| 20 | JM timestamps | CSF native (`ts_pair` +0.1 ms/frame incl. the readback); JM part needs a JM device |
| 21 | v9 primitives-generated | CSF free (`pg_query` = `ref`); v9 part needs a JM device |
| 22 | v7 BGRA custom border colour AFBC | v11 no cost (`tex_bgra` = `tex_rgba` within noise); the v7 case needs a v7 device |
