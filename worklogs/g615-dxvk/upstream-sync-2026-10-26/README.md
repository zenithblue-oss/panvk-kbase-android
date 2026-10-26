# Upstream sync: assessment and plan (2026-10-26)

Report and plan only. No driver code, Firebase run, device run or app change was made. Supporting files in this
directory:

- `inventory.md`: all 351 patches, one row each, with class and estimated rebase risk.
- `conflicts.md`: the conflict estimate per family, the method, and the hotspot files.
- `upstream-panvk-commits.txt`: the 64 upstream commits since our base that touch panvk, kmod or the GPU model table.

## Executive summary

**The idea.** Combine all patches into one patch, take every upstream change, and have DX8 to DX12 work on Mali v6
to v14.

**Short answer.** Taking upstream changes regularly is the right goal. A single combined patch is the wrong tool for
it. "Everything works from DX8 to DX12 on v6 to v14" is also not reachable as stated, by us or by upstream. Details:

1. **One big patch makes rebasing harder.** Our changes are about 121,000 added lines in 351 patches (plus the
   7,900-line kbase backend). Squashed into one patch, any upstream edit near any of our hunks makes the whole patch
   fail to apply. Every rebase then becomes one huge conflict session. We could no longer drop the pieces upstream
   already merged (12 patches today), bisect a regression to a change, or send small pieces upstream. Use a
   **Mesa git branch with about 10 ordered topic groups**, rebased with `git rebase` (or `git am -3`) plus
   `git rerere`. Only the commits that really collide stop, and each fix is recorded once and reused. A single
   flattened diff is still useful as a release artifact. Generate it from the branch for each release; do not
   maintain it by hand.
2. **Upstream has moved, but not far.** Upstream `main` is 2,538 commits past our base `5a07217f` (229 of them in
   `src/panfrost`). It is now `27.0.0-devel`, so the 26.3 branch has been cut. panvk itself changed moderately
   (+1,118/-599 lines). The hard parts are elsewhere:
   - the **kmod backend layer was refactored** (backend selection, sync ops, `submit_job`, `bo_map`/`bo_export`,
     import delegation, VDRM backends), so our kbase backend must be re-ported onto the new interface;
   - the **compiler changed a lot** (+6,579/-2,353 lines, including the new Rust "Kraid" work);
   - X11 WSI and `panvk_physical_device.c` were reworked.
3. **What upstream gives us.** 8 new device extensions since our base:
   - `VK_KHR_extended_flags`, `VK_EXT_vertex_attribute_robustness`, `VK_KHR_maintenance10`, `VK_KHR_maintenance11`;
   - `VK_INTEL_shader_integer_functions2`, `VK_ANDROID_external_format_resolve` (v10+);
   - `VK_KHR_incremental_present`, `VK_EXT_image_compression_control`.

   It also brings fixes, including two v14 fixes (`8e3e1a8e` SPD register allocation, `ec7b2f32` incremental
   rendering), the Immortalis model rows, and the start of `poly_heap` use in panvk.
4. **What upstream still lacks.** Upstream panvk still has no geometry shaders, no tessellation and no transform
   feedback on any arch: `geometryShader=false`, `tessellationShader=false` and no `VK_EXT_transform_feedback` at
   `f7dd61f7`. Upstream is therefore not enough for D3D10/11 or D3D12 (DXVK FL 10_0 needs GS and stream output,
   FL 11_0 needs tessellation). Our `gpu_prerast` emulation (csf-v11, jm-v9, jm-v7) is the part with the most value,
   and it has no upstream equivalent yet. It must be carried, and later converged with upstream's own work: draft
   GS MR !38401 and the open XFB MR !43359.
5. **DX12 limits that nobody has solved.**
   - FL 12_0 needs tiled resources tier 2 (sparse residency). That gap is on every arch for us. Upstream has sparse
     only on panthor (DRM), not on kbase, and without aliased residency.
   - FL 12_1 needs ROVs and conservative rasterization.
   - FL 12_2, DXR and mesh shaders are missing everywhere.
   - v6/v7 stop at FL 11_0.

   So "DX12" means **FL 11_x DX12 titles** for the foreseeable future. Games that require FL 12_0 or higher will
   not run.
6. **Arch facts (corrected).**
   - v14 exists: Mali-G1-Ultra/Premium/Pro. Upstream supports it behind `PAN_I_WANT_A_BROKEN_VULKAN_DRIVER`.
   - **v15 exists only as unmerged upstream drafts.** It is the Mali-G2 family ("G2-Ultra", "G2-Premium", "G2-Pro",
     product ids 15.8.x) in draft MR !41366. It needs new panthor uAPI that is not in mainline Linux.
   - Upstream panvk builds v6, v7 and v10-v14:
     - v10, v12 and v13 are enabled without a gate;
     - v6, v7, v11 and v14 need the env gate;
     - **v9 is not built by upstream panvk at all**, so our jm-v9 backend is ours only.
   - Only v10 reports Vulkan conformance.
7. **Recommended plan.**
   1. Finish and release the 115 work on the current base first.
   2. Turn the series into a topic-ordered branch. This is pure reordering and squashing, and the release tree hash
      must not change.
   3. Re-port the kbase backend onto upstream's new kmod ops.
   4. Rebase onto upstream main.
   5. Gate with PanProbe and PanPlay on Firebase (v6/v7/v9/v10/v11), the X6 Pro, and testers for v12-v14.
   6. Then rebase every 2-4 weeks.

   First pass rough estimate: 2-3 weeks including device validation. After that, about 1-2 days per rebase.

## 1. Facts gathered

### 1.1 Base and upstream

- Our base: Mesa `5a07217f034b3e50d8c7c7794f97a2df1742613b` (2026-09-18, "radv: slightly rework initializing the DCC
  predicate"), VERSION `26.3.0-devel` (`sources.lock`). The tree is built by `scripts/prepare-rc5-tree.sh`:
  - strict apply, one commit per patch, family order
    `common android kbase-common app-loader wsi csf csf-v11 jm-v9 jm-v7` (plus the `files/` trees);
  - then `git am` of `jm-v7-wip`;
  - `staging-115` is applied with `git am` on branch `p115-int` / `p115-pd`.
- Upstream clone: `/home/flux/src/mesa-upstream` (new dir, `--shallow-since=2026-09-01`, single branch `main`). It is
  checked out detached at **`f7dd61f78f83` (2026-10-26 15:35 UTC, "docs: Fix new_features.txt after rebase
  issue"), VERSION `27.0.0-devel`**. The base is an ancestor of main.
- Since the base: 2,538 commits in total:
  - 229 in `src/panfrost`;
  - 53 in `src/panfrost/vulkan` (+1,118/-599 lines);
  - 12 in `src/panfrost/lib/kmod`;
  - 147 in `src/panfrost/compiler` (+6,579/-2,353 lines).

### 1.2 Arch support, v6 to v15

Sources:
- Upstream `src/panfrost/model/pan_model.c` and `src/panfrost/vulkan/meson.build`, and
  `panvk_physical_device.c:454-476` at `f7dd61f7`.
- Linux master `drivers/gpu/drm/panthor/panthor_hw.c`, fetched 2026-10-26 (a tool summary; treat it as secondary).
- GitLab MRs !41081, !41366, !44172 and !44720, read through the API.

| Arch | GPUs (upstream names) | Mesa product ids | Upstream kernel driver | Upstream panvk at `f7dd61f7` | Ours (kbase) |
|---|---|---|---|---|---|
| v6 Bifrost JM | G71, G72 | 6.0.0, 6.2.1 | panfrost | built, env gate, Vulkan 1.3 | built, no gate (jm-v7 002), GS/tess/XFB emulation |
| v7 Bifrost JM | G51, G31, G76, G52, G52 r1 | 7.0.0, 7.0.3, 7.2.1, 7.2.2, 7.4.2 | panfrost | built, env gate, Vulkan 1.3 | same as v6 |
| v9 Valhall JM | G57, G68 (Mesa rows); we add G77, G78, G78AE (jm-v9 006, 021) | 9.0.1, 9.0.3, 9.2.4 | panfrost (Gallium only) | **not built** (arch list `[6, 7, 10, 11, 12, 13, 14]`) | our v9 JM backend (jm-v9 002), Vulkan 1.3 |
| v10 Valhall CSF | G610, G710, G310 v1-v5 (kernel also G510) | 10.8.7, 10.12.4 | panthor | enabled, Vulkan 1.4, **the only conformant arch** (1.4.5.2) | built |
| v11 Valhall CSF | G715, G715-Immortalis, G615 | 11.8.2, 11.8.3 | panthor | env gate | built (main target: X6 Pro G615) |
| v12 5th gen | G720, G720-Immortalis (kernel also names G620 = 12/1; Mesa has no row) | 12.8.0 | panthor | enabled, not conformant | built |
| v13 5th gen | G725, G925-Immortalis (kernel also G625 = 13/1; no Mesa row) | 13.8.0 | panthor | enabled, not conformant | built (csf-v11 109, 119, 130 v12+/v13 paths) |
| v14 5th gen | **Mali-G1**: G1-Ultra, G1-Premium, G1-Pro | 14.8.0, 14.8.1, 14.8.3 | panthor (arch 14 has the new PWR control block) | env gate (MR !41081, merged 2026-05-13) | built; kbase fixes csf-v11 110 (ALLOC_EX fallback) and 186 (MTK MEM_IMPORT layout) |
| v15 5th gen | **Mali-G2**: G2-Ultra, G2-Premium, G2-Pro (**draft MR only**) | 15.8.0, 15.8.1, 15.8.3 (draft) | panthor uAPI still in development (panfrost/linux MR 65); not in Linux master | **not in main.** Drafts: !41366 panfrost v15 (Arm, updated 2026-10-23), !44172 Kraid v15, !44720 panvk syncobj buffers (v15+) | nothing yet |

What v15 changes, per the upstream drafts:
- Shaders can use up to 128 registers. The instruction encoding changes, and preloads move from r48-r63 to r0-r15.
- GPU_ID becomes 64-bit (`PAN_ID64_*` helpers).
- A new syncobj design: syncobjs are mapped in firmware space on a heap the kernel driver controls.
- Vulkan needs `PAN_I_WANT_A_BROKEN_VULKAN_DRIVER=1`.

No v15 kbase interface is known to us. Section 5.4 covers what our backend would need.

### 1.3 Test evidence per arch (PanProbe)

| Arch | Evidence in this repo | Source |
|---|---|---|
| v6 | FTL starlte (G72), 115pre-d: 39/39 in both A/B passes (2026-10-26) | `device-runs/2026-10-26-ftl-v6-starlte-115pre-d/results.md` |
| v7 | FTL a31 (G52 MC2), 115pre-d: 39/39 x2 | `device-runs/2026-10-26-ftl-v7-a31-115pre-d/results.md` |
| v9 | FTL oriole (G78), 115pre-d: 39/39 x2 | `device-runs/2026-10-26-ftl-v9-oriole-115pre-d/results.md` |
| v10 | FTL panther (G710 `0xa8620004`), RC5-universal-114: 39/39 | `device-runs/2026-10-25-ftl-v10-panther-rc5u-114/results.md` |
| v11 | FTL shiba (G715 `0xb8a21020`), 114: 39/39; 115pre: 39/39 x2; X6 Pro G615 is the dev device | `device-runs/2026-10-25-ftl-v11-shiba-rc5u-114`, `2026-10-26-ftl-v11-shiba-115pre` |
| v12 | Tester uploads, G720 `0xc8700010` (2412DPC0AG, 2511FPC34G): beta.18-rc1 37/37 (2026-10-07/08); rc3 "38/38 on v9-v13" (2026-10-16) | `worklogs/driver-remaining/rc1-tester-logs.md`; `PROGRESS.md` line 5 |
| v13 | Tester uploads, Immortalis-G925 `0xd8300015`, kbase CSF uAPI 1.30 (2506BPN68G, 2602BPC18G, PLC110, V2507A): rc1 37/37; rc3 38/38 | same; `driver-remaining/130-132-g925.md` |
| v14 | Tester uploads, G1-Ultra MC12 `0xe8800010`, MT6993, CSF uAPI 1.38: rc3/rc3-lowmem **37/38** (2026-10-16). `swapchain_lifecycle` failed with `KBASE_IOCTL_MEM_IMPORT` ENOTTY. Fix csf-v11/186 was built but untested in the repo record. | `PROGRESS.md` "Mali v14 support (status 2026-10-16)" |
| v15 | none | - |

**v14 full pass: the user's statement (source: user), not verified here.** The user says v14 now passes. The repo's
last record is 37/38 on 2026-10-16.

The upload store could not be read:
- `GET https://panvk-upload.panvk.workers.dev/admin/uploads` returns **401** without the admin token.
- This environment has no Cloudflare API token, no wrangler login and no `.dev.vars`.
- The worker's only open routes are uploads and `GET /blob/<sha256>`, which needs a known hash.

The v12/v13 rows above come from earlier read-only D1/KV passes recorded in the repo. They are not newer than
2026-10-16. To confirm v12-v14 on RC5, read D1 `panvk-uploads` with the admin token (`/admin/uploads?app=panprobe&since=2026-10-16`).

## 2. Verdict on "one combined patch"

| Question | Combined single patch | Topic-ordered git branch (recommended) |
|---|---|---|
| Rebase onto new upstream | All-or-nothing apply; every rebase is one conflict session over ~121k lines | `git rebase`/`am -3` stops only at colliding commits; `rerere` replays earlier resolutions |
| Pieces upstream already merged (12 now) | Hunks must be found and cut by hand | Rebase drops patch-id-equal commits itself; superseded ones are dropped explicitly |
| Regression hunting (115 A/B knobs, FTL runs) | No bisect | `git bisect` across topic commits |
| Provenance (`Purpose/Tested GPU/Validation` headers, `patchSeriesId`) | Lost or merged into one blob | Kept per commit; `patches/` regenerated by `git format-patch` |
| Sending work upstream | Impossible as is | Topic commits can go upstream almost as they are |
| Release reproducibility | Good (one file) | Same: generate `git diff <upstream-base>..<release>` as a release asset |

So keep many patches but **fewer, cleaner ones**. Squash the long fixup chains (for example BC 150-218, the tess
chunk series, the "review fixes" patches and the knob scaffolding once validated). Order them by topic so that
upstream-facing changes sit at the bottom and the most volatile code sits at the top.

## 3. Inventory summary (details in `inventory.md`)

| Family | Patches | What it is | Main classes |
|---|---|---|---|
| common | 1 | NIR `load_attr_pan` offset src | fix |
| android | 5 | gralloc mapper (vendor-neutral, HIDL mapper4 off by default), AHB dma-buf finder, static hardware stub | android-wsi |
| kbase-common | 10 + `files/` (kbase_kmod.c 2,033 lines + Arm uAPI headers, ~7.9k lines) | meson KMD option, kmod backend hooks, kbase physical device, /dev/mali enumeration, queue/WSI integration | kbase (003 is superseded in shape by the new upstream kmod ops) |
| app-loader | 1 + files | ICD exports next to HMI | android-wsi |
| wsi | 2 | Android surface + AHB swapchain | android-wsi |
| csf | 2 | kbase CSF queue submit path (1,795 lines of call-site branches in `panvk_vX_gpu_queue.c`) | kbase |
| csf-v11 | 171 (018-233) | gpu_prerast emulation (GS, tess, XFB, multiViewport, statistics, fill mode, depth bounds, clip/cull), BC emulation and transcoders, kbase CSF fixes, lowmem/perf, X11 sw present, pan/bi RA speedups, debug | feature 54, workaround 24, perf 22, fix 20, kbase 17, **upstream 12**, android-wsi 12, debug 8, superseded 2 |
| jm-v9 | 31 | Valhall v9 JM backend and its features | feature 13, kbase 7, fix 6 |
| jm-v7 | 5 | Bifrost on kbase (no env gate, JIT skip, atom stride, ion fallback) | kbase 3, policy 1 |
| jm-v7-wip | 87 | Bifrost features (GS/tess/XFB emulation, robustness, bindless, conditional rendering), v6/v7 memory, timestamps, v9 fixes | feature 33, fix 21, perf 13 |
| bcn-layer | 1 | layer enumeration exports (separate component) | android-wsi |
| staging-115 pa/pb/pc/int/pd | 10/10/7/1/7 | 115 perf round (CSF state, memory, WSI threads, JM), driverInfo in upstream format | perf, kbase, android-wsi |

**Already upstream (drop on rebase):** csf-v11 026, 027, 028, 029, 030, 031, 032, 033, 034, 035, 036 and 037. These
are the FAU flush, the v11 arch switches, incremental_present, swapchain_colorspace, the AFBC X-component formats,
the modifier capability query, AFBC for WSI by default, zeroed query images, image compression control (x3), and
the extension advertisement.

**Superseded:**
- csf-v11 131 and 187: model names. Upstream now has Immortalis rows (`7c648edb`) and `pan_model_device_name`
  (`5cc8e63b`).
- kbase-common 003: kmod hooks. Re-port onto upstream's new ops; 005 and 010 follow it.
- csf-v11 099: driverInfo branding, already replaced by staging-115/int 0001.

**Overlaps to re-check:** csf-v11 056 (budget cache) against upstream `ba3dacb9` (heap budget across instances).

**Upstream kmod vs our kbase backend.**
- Our `kbase_kmod.c` is already a `pan_kmod` backend, taken from the wonderkast02 G720 reference. The rest of panvk
  reaches kbase through hooks (kbase-common 003/010) and through large `if (kbase)` branches at panvk call sites
  (csf 007, kbase-common 004/006).
- Since our base, upstream added:
  - backend selection (`ab1dc701`) and compatibility checks inside kmod (`9e872e24`);
  - abstract sync ops (`712afcde`) with panvk on `u_sync_provider` (`09eccf7f`);
  - `submit_job` (`b3843729`), `bo_map`/`bo_export` (`3395246b`/`3f05703b`), `get_param` and `prime_fd_to_handle`;
  - backend-side import (`37a1dcfb`), executables in one 4 GiB VA range (`fa679747`), and VDRM backends (`64133541`).
- These are the seams we patched by hand, so the re-port should make the kbase layer *smaller* and more
  self-contained. Not checked: whether CSF group/queue creation is abstracted too. If it is still direct panthor
  ioctls, the clean fix is new kmod ops, which could be proposed upstream.
- An upstream kbase backend is unlikely, because the vendor kbase uAPI is not upstream kernel uAPI (my
  assessment). Plan for it to stay out of tree.

## 4. Rebase conflict estimate (details in `conflicts.md`)

No trial apply was run: the user asked for a fast wrap-up. The estimate below comes from file overlap.

- **351 patches:** 39 touch no file upstream changed, 47 low, 193 medium and 72 high. This over-counts textual
  conflicts and under-counts API breakage.
- **Hot files** (touched by many of our patches and changed upstream):
  - `panvk_vX_shader.c` (68 patches, upstream +/-45 lines), `panvk_vX_physical_device.c` (62, 42),
    `jm/panvk_vX_cmd_draw.c` (61, 68), `csf/panvk_vX_cmd_draw.c` (58, 29): small upstream churn, so expect few
    real conflicts;
  - `panvk_vX_device.c` (41, 203), `panvk_physical_device.c` (25, **480**), `panvk_image.c` (23, 229) and
    `panvk_device.h` (30, 102): these are where the conflicts will be.
- **Kmod layer.** `pan_kmod.h` (+112/-36), `pan_kmod_backend.h` (+98/-2), `panfrost_kmod.c` (+701/-86) and
  `panthor_kmod.c` (+150/-27) changed. Our backend files may apply cleanly and still not build, so this is the
  largest single work item.
- **Compiler.** `bifrost_compile.c` changed +62/-207, but `bi_ra.c` only +13/-8, so our RA patches 152-164 are
  medium risk. Check whether Kraid replaces the Valhall backend for some archs; if so, those patches matter less there.
- **X11 WSI.** `wsi_common_x11.c` changed +237/-16. Our X11 sw-present patches (csf-v11 084, 106, 117, 198, 199,
  staging-115/pc 0003/0004) will conflict.
- **Within the series.** In order, csf-v11 210-219 have already been seen to move context for jm-v7-wip patches
  (`rc5-universal.md`); `git am -3` handled those. Expect the same pattern after each upstream move.
- **Generated code.** The BC patches carry very large generated tables (206 is +17,574/-4,763). They do not conflict
  with upstream, but they make every refresh noisy. Generate them at build time, or keep them as single files and
  not as patch churn.

## 5. Feature gaps for DX8-DX12, v6 to v15

### 5.1 What each API layer needs, and where we stand

| Layer | Needs (Vulkan side) | Upstream panvk alone | With our series |
|---|---|---|---|
| DX8/DX9 (DXVK d3d8/d3d9), DX1-7/Glide (dgVoodoo -> D3D11) | Vulkan 1.3, BC textures (or emulation), basic robustness | Vulkan 1.3 on v6/v7, 1.4 on v10-v14. Native BC only where the HW has it; no emulation | Works v6-v14: BC emulation and transcode, cubes run on v6-v13 per uploads and FTL. v14 x86 cubes needed the 186 import fix (status: user) |
| DX10/DX11 (DXVK d3d11) | FL 10_0: `geometryShader` + `VK_EXT_transform_feedback`; FL 11_0: `tessellationShader`; FL 11_1: `logicOp`, `vertexPipelineStoresAndAtomics`, 64 UAV slots | **No GS, no tess, no XFB on any arch**, and depthBounds, fillModeNonSolid, multiViewport, pipeline statistics, clip/cull distance and robustImageAccess2 all false. DXVK cannot reach FL 10_0 | GS/tess/XFB/multiViewport/statistics/fill mode/depth bounds/clip-cull through gpu_prerast on v6-v14. FL 11_1 on v9-v11; FL 11_0 on v6/v7 (32 storage images per stage) |
| DX12 (vkd3d-proton) FL 11_x | Vulkan 1.3, robustness2 + RIA2, nullDescriptor, mutable descriptors, 1M update-after-bind descriptors, push descriptors, maintenance5/6, XFB queries, GS/tess | Fails on GS/tess/XFB, and on RIA2 (false upstream) | **0 hard gaps** on v6-v11 (PanProbe `compliance-vkd3d_reqs`, `dx12-test-selection-2026-10-26.md` 4.1). v12-v14 not measured with that test. |
| DX12 FL 12_0 | Tiled resources tier 2 (sparse residency + residency queries), binding tier 2, typed UAV loads | Sparse binding/residency on v10+ through panthor VM_BIND, but `sparseResidencyAliased=false` and no residency/MinLod, so tier 0 | **Blocked on every arch.** `has_sparse` is false on kbase |
| DX12 FL 12_1 | ROVs (`VK_EXT_fragment_shader_interlock`), conservative rasterization | Neither | v11 has conservative tier 1; no ROVs anywhere |
| DX12 FL 12_2 / DXR / SM 6.5+ extras | Mesh shaders, VRS (`KHR_fragment_shading_rate`), ray query/pipelines, sampler feedback; `shaderFloat64` for double ops | None (no FP64 on Mali) | None |

### 5.2 Per-arch gaps that remain with our series

| Arch | Max FL (vkd3d) | Remaining gaps |
|---|---|---|
| v6, v7 | 11_0, SM 6.5 | 32 storage images per stage (11_1 needs 64); no `KHR_draw_indirect_count` (ExecuteIndirect count); no int64 buffer atomics, maximal reconvergence or quad control (SM 6.6/6.7); no `shaderOutputLayer`; sparse; all FL 12_x items. Upstream keeps v6/v7 gated and does not plan these features. |
| v9 | 11_1, SM 6.8 | Upstream does not support it at all, so the whole backend is ours to maintain. No `shaderOutputLayer`; sparse; FL 12_x. |
| v10, v11 | 11_1, SM 6.8 | Sparse on kbase (FL 12_0); ROVs; conservative raster on v10; FL 12_2 items. |
| v12, v13 | 11_1 expected (not measured with `compliance-vkd3d_reqs`) | Same as v10/v11. v13+ VPSA uses the upstream IDVS path (csf-v11 119/130). |
| v14 | expected as v12/v13 (not measured) | Upstream v14 fixes after our base (`8e3e1a8e`, `ec7b2f32`) are missing until we rebase. Vendor uAPI 1.38 changed structs (MEM_ALLOC_EX, MEM_IMPORT): handled by 110/186, more may come. |
| v15 | n/a | Not in upstream main yet (see 5.4). |

### 5.3 Upstream features since our base that matter for DX titles

- `VK_EXT_vertex_attribute_robustness`: DXVK and vkd3d-proton use it for out-of-range vertex fetches.
- `VK_KHR_maintenance10`/`11`, `VK_KHR_extended_flags`: newer DXVK/vkd3d-proton versions tend to require recent
  maintenance extensions, so falling behind upstream eventually blocks component updates. This is an inference;
  check each DXVK/vkd3d-proton release's requirement list.
- `VK_EXT_image_compression_control` (we already carried it), `VK_KHR_incremental_present`,
  `VK_ANDROID_external_format_resolve` (v10+).
- Fixes: v14 SPD register allocation and incremental rendering, full texture handles on v9+ (`c3e210cb`),
  executables in one 4 GiB VA range, descriptor-count validation, the GetInstanceProcAddr NULL dereference.

### 5.4 What our kbase backend would need for v12+ and v15

- **v12-v14 (done or partly done).**
  - CSF uAPI layout selection by version: 112-byte group create on about 1.30, 16-byte heap init, the ALLOC_EX
    fallback, 16 KiB pages (csf-v11 110).
  - 128 CS registers on v12+.
  - The MTK SLC `MEM_IMPORT` layout on v14 (186).
  - Keep logging the uAPI version and every ENOTTY ioctl once, so each new vendor drop shows all ABI mismatches in
    one upload.
- **v15 (to bring up).**
  - Parse the 64-bit GPU_ID that kbase reports: the `PAN_ID64_*` decode, plus the kbase gpuprops path.
  - Take the v15 compiler and genxml (`v15.xml`, 128-register encoding, preloads at r0-r15) from upstream once
    !41366 and !44172 merge. Do not port them early.
  - The new syncobj model (syncobjs in firmware space on a KMD-controlled heap, !44720) changes how CSF sync and
    queue wait/signal work. Our kbase CSF path (csf 007, csf-v11 059, 077, 082, 085) would need the kbase equivalent.
  - The kbase v15 ioctls are unknown. We need a v15 kbase source (an OEM GPL kernel drop for a G2-class SoC, as was
    done for MT6993) and a tester device.
  - Readiness gate: upstream v15 merged in main, plus a kbase source, plus one device. Until then v15 stays
    "planned"; nothing can be tested.

## 6. Plan

### 6.1 Maintenance model

- **Repository.** Keep a Mesa fork with branch `panvk-kbase/main`, the source of truth for code. In this repo,
  `patches/` becomes an export (`git format-patch`) for provenance. `sources.lock` keeps the upstream commit and the
  `patchSeriesId`. `prepare-rc5-tree.sh` stays as the strict reproducer of the export.
- **Topic groups**, bottom to top, so the upstream-facing and stable parts sit lowest:
  1. **upstreamable generic:**
     - common 019;
     - csf-v11 063 (NIR robust vectorizer merge), 209 (`u_queue` fork restart), 171 (Android property lookup once);
     - model rows (jm-v9 006/021, csf-v11 140);
     - pan/bi RA speedups (152-164).
  2. **kbase kmod backend:** kbase-common + `files/`, re-ported to the upstream ops.
  3. **kbase JM submit (v6/v7/v9)** and **kbase CSF submit (v10+)**: csf 007/009, the kbase parts of csf-v11,
     jm-v9 and jm-v7.
  4. **Android:** android/, app-loader/, wsi/, Android disk cache dir (170/204), dma-heap placement (091).
  5. **X11 sw present** (Wine X server path).
  6. **v9 JM backend** (jm-v9 002 and its features).
  7. **gpu_prerast emulation:** GS, tess, XFB, viewports, statistics, fill mode, depth bounds, clip/cull, shared by
     v6-v14.
  8. **Bifrost v6/v7 features** (jm-v7 + jm-v7-wip).
  9. **BC emulation and transcoders**, squashed. Move the generated tables to build-time generation.
  10. **memory/perf tuning** (lowmem, pools, TLS, tiler heaps, 115 perf), each change with its knob.
  11. **debug/identity** (`PANVK_DEBUG=mem`, FPS log, driverInfo in upstream format).
- **Tools.**
  - `git rerere` on.
  - `git range-diff old..new` reviewed at every rebase.
  - Use `git am -3` / `git rebase` with no fuzz.
  - Keep the strict no-offset rule for the exported series.
- **Cadence.** Rebase onto upstream `main` every 2-4 weeks, and always shortly after a Mesa branchpoint. Release
  builds come from a gated snapshot recorded in `sources.lock`. driverInfo follows upstream, so it becomes
  `Mesa 27.0.0-devel (git-<sha>)` after the first rebase.
- **CI gate per rebase** (one Firebase device at a time under `firebase.lock`). A rebase is promoted only on a full
  pass with no regression against the last release:
  - build all arch libraries (v6, v7, v9-v14, plus v15 once upstream adds it), NDK r30 API 29 and glibc;
  - PanProbe 39/39 plus the env A/B on Firebase v6 starlte, v7 a31, v9 oriole, v10 panther and v11 shiba;
  - PanPlay cubes D3D8/9/10/11 and the D3D12 Multithreading/gears tests;
  - X6 Pro games;
  - tester uploads for v12-v14 (read D1 with the admin token);
  - keep tess_perf hashes, FPS within noise, and no DEVICE_LOST.

### 6.2 Phases

| Phase | Work | Effort (rough) | Risk |
|---|---|---|---|
| 0 | Finish 115 on the current base; the other agent owns `mesa-115-pd`, so do not disturb it. Release RC5/115. Add the coming `fix/` family as fixups in its topic group. | as planned | - |
| 1 | Import the released tree as commits (`rc5g`/`p115-*` already are), drop the 12 upstream patches, regroup and squash into the topics above. **Check: the final tree hash equals the release tree.** | 1-2 days | low (zero behaviour change, checked by tree equality) |
| 2 | Rebase onto `f7dd61f7` (or the newest main) in two steps. First onto the commit just before the kmod refactor (`ab1dc701` and its neighbours), then across it. Re-port the kbase backend onto backend selection, sync ops, `submit_job`, `bo_map`/`bo_export` and import. | 3-5 days | medium-high (kbase JM and CSF lifecycle, sync) |
| 3 | Rebase the remaining topics and resolve the hot files (`panvk_physical_device.c`, `panvk_vX_device.c`, `panvk_image.c`, `wsi_common_x11.c`, compiler). Build all archs. Check that each upstream feature gained (section 5.3) is not shadowed by our gates. | 3-5 days | medium |
| 4 | Full gate (6.1) on Firebase plus the X6 Pro; compare with the RC5 baselines. | 1-2 days of device time | medium |
| 5 | Upstream contributions (6.3) and the rebase cadence. v15 bring-up only after its readiness gate (5.4). | ongoing, 1-2 days per rebase | low if the cadence holds; cost grows fast with longer gaps |

### 6.3 What to send upstream

- Generic pieces: csf-v11 063 (NIR), 209 (`u_queue` fork), common 019, possibly 171/170/204 (Android
  `os_misc`/disk cache), and the model rows for G77/G78/G78AE/G710 (jm-v9 006/021, csf-v11 140).
- pan/bi RA compile-time work (152-164), if still relevant next to Kraid.
- gpu_prerast GS/tess/XFB: talk to upstream about !38401 (draft GS) and !43359 (XFB, open since 2026-07-31) before
  porting ours again. Upstream now hooks up `poly_heap` (`83504d84`, `a1ab82b2`) and our path is built on `src/poly`,
  so converging would remove our largest feature delta.
- kmod ops for CSF group/queue/tiler heap, if upstream has not abstracted them. That makes the kbase backend a
  drop-in backend without call-site branches.
- Not upstreamable: the kbase backend itself (vendor uAPI), Android vendor workarounds (MTK layouts), BC emulation
  (policy question, so ask first), and the debug/knob scaffolding.

### 6.4 Where the 115 work fits

staging-115 pa/pb/pc/int/pd (35 patches, passing on FTL v6/v7/v9/v11) stays on the current base until it is
released. Do not mix it with the rebase. After release it is folded into topics 4 (WSI threads/present), 10
(memory/perf) and 3 (JM/CSF submit), and int 0001 into topic 11. Its env knobs stay, because they are the A/B tool
for the first post-rebase gate.

## 7. Not verified, and why

- The conflict numbers are a file-overlap estimate. No `git am -3` trial and no meson configure were run (skipped on
  request).
- Patch classes are a first pass from subjects and paths. The `upstream` and `superseded` rows were checked by hand
  against upstream subjects, `pan_model.c` and the kmod log; the other classes are approximate.
- v12, v13 and v14 on current RC5 builds: the upload store needs the admin token (401) and no Cloudflare
  credentials are available here. The v14 full pass is the user's statement.
- The v15 names, ids and features come from draft MRs and may change before merge. The kernel GPU names (G510, G620,
  G625) come from a fetched summary of `panthor_hw.c`.
- That sparse residency can be built on kbase (the FL 12_0 path) has not been investigated.
- The DXVK FL to feature mapping is from knowledge of DXVK, not re-read from its source in this pass. The vkd3d-proton
  limits come from this repo's `dx12-test-selection-2026-10-26.md` (vkd3d-proton 22307558).
