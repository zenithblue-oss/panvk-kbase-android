# Bifrost v6/v7 support research (2026-10-16)

Scope: whether and how to add Mali v6 (Bifrost gen 1: G71, G72) and v7 (Bifrost gen 2: G31, G51, G52, G52 r1, G76) to our kbase PanVK, with enough features for DXVK, by reusing existing work. Inputs: the "Mali v7 (Bifrost) support" section of [PROGRESS.md](PROGRESS.md), item 12 (v9 JM) and `patches/jm-v9` 001-030, upstream Mesa main (`b27cf4e6f6`, 2026-10-16), GitHub and freedesktop GitLab APIs, and existing reference clones under `/var/tmp/panvk/refs/` (no new clones, nothing built or run).

## Summary

- **Upstream Mesa** still gates v6/v7 behind `PAN_I_WANT_A_BROKEN_VULKAN_DRIVER` (main, today). It reports **Vulkan 1.3** on Bifrost since !43245 (merged 2026-07-30, so our pin 5a07217f has it; PROGRESS.md saying "about Vulkan 1.1" is out of date). No geometry shaders, tessellation, transform feedback, multiViewport, clip/cull distance, fillModeNonSolid or robustness2 on Bifrost upstream. No Khronos Vulkan conformance for any Bifrost GPU (Panfrost GL ES 3.1 on G52 is the only Bifrost conformance).
- **One fork already did nearly all of the DXVK work on Bifrost over kbase:** [JimVulkan/mali-panvk](https://github.com/JimVulkan/mali-panvk) (MIT, Mesa 26.3.0-devel, r4 released 2026-09-27; newer PanVK code including partial BCn is in [JimVulkan/mesa-unified-drivers](https://github.com/JimVulkan/mesa-unified-drivers), 2026-09-30). It removes the gate, forces the variant to 0, avoids JIT entirely (GROW_ON_GPF heaps), and emulates GS, tessellation and XFB with compute jobs on JM. It runs DXVK D3D9-D3D11 on G76 (S10e), G72 (Tab S6 Lite) and G52 MC2 (Galaxy A31). Not conformant, AI-assisted, one squashed commit.
- **It solves all three of our v7 blockers** in its own kbase code: variant (sets `gpu_variant = 0` in kbase props), `MEM_JIT_INIT` ENOMEM (never calls JIT_INIT; tiler heap, varying and tess heaps use `BASE_MEM_GROW_ON_GPF`), and device-creation gate (warns and continues on v6/v7). Its atom layout is a fixed 64-byte `base_jd_atom` (with `seq_nr`), unlike our probe (56 accepted, 72 refused), so the stride needs a check on device.
- **Our v9 JM work does not transfer directly.** `jm-v9` 011-019 build on the CSF `gpu_prerast` framework and are gated `PAN_ARCH >= 9` / `== 9` (Valhall resource tables, Valhall compiler). Bifrost needs attribute/varying descriptors and the Bifrost compiler. What does transfer: our kbase JM layer (001, 004/005, 022 atom probe and fault propagation, 024 dmabuf cache clean), shared JM queue fixes (007/008-style), and possibly the BC compute decode.

**Recommendation: worth doing for v7, as a port of JimVulkan's Bifrost pieces onto our tree, not a rebase onto JimVulkan and not an extension of `gpu_prerast` to Bifrost.** Keep our kbase/JM transport (it carries the v9 fixes and v10+ CSF), take JimVulkan's arch-independent GS NIR lowering and its `PAN_ARCH < 9` JM draw paths, robustness2 emulation and the kbase memory policy for arch < 9. Start with a 1-2 day unblock so a G52 MC2 can create a device and give a PanProbe baseline. v6 comes almost free after v7 (same backend), but its devices are old and mostly Android 9-11, so it is low priority. A test device (G52 MC2) is the real gate; none is in hand.

## Upstream Mesa state (main, 2026-10-16)

| Item | State |
|---|---|
| Gate | `panvk_physical_device.c`: arch 6, 7, 11, 14 fail with `VK_ERROR_INCOMPATIBLE_DRIVER` unless `PAN_I_WANT_A_BROKEN_VULKAN_DRIVER` is set. No MR to lift it found. |
| API version | 1.3 on v6-v9 (!43245, kusma, merged 2026-07-30); 1.4 on v10+. Vulkan 1.1 + subgroups on Bifrost: !41981 (merged 2026-07-25). |
| Features off on Bifrost | geometryShader, tessellationShader, fillModeNonSolid, multiViewport, shaderClipDistance/CullDistance, transformFeedback, depthBounds, pipelineStatisticsQuery, descriptorIndexing (`>= 9`), multiDrawIndirect (`>= 10`), robustBufferAccess2 (`>= 11`), nullDescriptor (`>= 10`), shaderFloat64. |
| On | imageCubeArray (!32612), dualSrcBlend, logicOp, sampleRateShading, depthClamp, customBorderColors, timeline, sync2, maintenance4, shaderInt64, fragmentStoresAndAtomics (!39207), draw indirect (!35724), multiview (!34832), ycbcr (!38145). BC only if the GPU has it natively (Bifrost texture features usually do not). |
| Relevant open/new MRs | !37969 poly_heap on JM, replacing the JM indirect varying buffer (olivia, merged 2026-10-08, after our pin): the base upstream needs for libpoly GS/tess on JM. !38401 "Very Draft! Panvk Geometry Shaders" (gfxstrand, last update 2026-08-10). !43359 transform feedback (v10 only). !43648 shader_atomic_int64 on Bifrost (draft, 2026-10-15). !44378/!44382 v6-7 descriptor-set and host-copy fixes (merged 2026-09-15). |

Upstream will likely get libpoly-based GS/tess on JM eventually (poly_heap landed), but there is no date and no Bifrost-specific GS MR yet.

## Third-party projects (verified to exist via GitHub API on 2026-10-16)

| Repo | Last push | License | Arch / devices | Kernel path | Vulkan / DXVK | Notes |
|---|---|---|---|---|---|---|
| [JimVulkan/mali-panvk](https://github.com/JimVulkan/mali-panvk) | 2026-09-27 (r4 = v1.3.0) | MIT (changes), Mesa per-file | v6 + v7: G76 MC12 (S10e, Exynos 9820), G72 MP3 (Tab S6 Lite, Exynos 9611, r1-r3), G52 MC2 (A31, Helio P65) | kbase JM, own `kbase_kmod.c` (1606 lines, "Copyright 2026 XclipseTools"), 64-byte atom, no JIT, GROW_ON_GPF heaps | Vulkan 1.3; DXVK D3D9/10/11 (version not stated); D3D12 no | GS/tess/XFB/streams via compute jobs (`panvk_nir_lower_gs.c` 1273 lines, arch-independent NIR; `jm/panvk_vX_cmd_draw.c` 3388 lines); robustness2 + nullDescriptor emulated; fillModeNonSolid, multiViewport, clip/cull, MDI on `PAN_ARCH < 9`; MediaTek gralloc slot + AFBC AHB import. Android 12+ (API 31). Games: Minecraft Java, ARMSX2 (PCSX2) mentioned; DXVK titles not named. Local clone `/var/tmp/panvk/refs/jimvulkan` is at the latest commit `fb3b49d`. |
| [JimVulkan/mesa-unified-drivers](https://github.com/JimVulkan/mesa-unified-drivers) | 2026-09-30 | MIT | same + Panfrost GL 4.6 on Bifrost kbase | same | same + "partial BCn on Bifrost" (`a5010bb4`: `panvk_bc_emu.c` 464 lines, meta 279 lines) | Newer than mali-panvk. Not cloned yet. |
| [LukeValen/panvk-mali-g52](https://github.com/LukeValen/panvk-mali-g52) | 2026-09-30 (v0.0.2-alpha) | **none** (README says Mesa is MIT; repo has no license) | v7 G52 MC2 (Redmi 13C, kbase UAPI 11.38, Android 14) | kbase JM on funnymdzz Mesa | DXVK 1.10.3: D3D9-11 basic rendering, D3D11 GS test pass | v0.0.1 force-advertises GS/tess/XFB/BC/multiViewport/clip without implementing them; v0.0.2 adds real libpoly GS (VS -> GS -> indexed raster). Reference only (no license). Already in `sources.lock` (`dd2d0ee`, older). |
| [0x8055/panvk-g52-oppo-a38](https://github.com/0x8055/panvk-g52-oppo-a38) | 2026-09-21 | none | G52 MC2, MT6768, kernel 6.6 (UAPI 11.46) | kbase JM, funnymdzz | none (bring-up notes) | Useful finding: `JOB_CONFIG_FAULT` root cause was the wrong `JOB_SUBMIT` stride; MediaTek AHB fd index. |
| FristOneRR ([source](https://github.com/FristOneRR-Admin/FristOneRR-Panvk-Source), [driver](https://github.com/FristOneRR/FristOneRR-Panvk-Driver)) | 2026-09-30 / 2026-10-10 | Mesa MIT; driver repo MIT | v9 G57 main; G52 r1 (A38) per our plan doc, "atom stride 56" | kbase JM, funnymdzz | DXVK 1.10.3 on G57 | Fakes features via `PANVK_DXVK_COMPAT`. Our 001/002 are based on it. Little Bifrost value. |
| [TheoLore/panvkJava](https://github.com/TheoLore/panvkJava) | 2026-10-15 (created same day) | Mesa per-file | "kbase and Job Manager" (arch not stated) | kbase JM | no release | Unverified content; check later. |
| [Koadzba/PanVK-Droid](https://github.com/Koadzba/PanVK-Droid) | 2026-06-04 | none | "Bifrost" | Mesa 24.0.5 | none | Empty/README only; ignore. |
| [kamnevdima1220006-tech/realme8-vulkan-driver-build](https://github.com/kamnevdima1220006-tech/realme8-vulkan-driver-build) | 2026-09-07 | none | G76 MC4 | plan only | none | Build guide, no driver. Ignore. |
| [bmdhacks/malisx2](https://github.com/bmdhacks/malisx2), [Noysz/panvk-g99-jm](https://github.com/Noysz/panvk-g99-jm) | 2026-10-14 / 2026-10-11 | MIT / none | v9 (malisx2 README now targets G615 v11) | kbase | | No Bifrost. Already mined for jm-v9 022/023. |

Not found: any Linux panfrost-DRM fork running DXVK on Bifrost (RK3566/RK3568/S922X G52 boards only have upstream PanVK behind the gate). Winlator/GameHub/Mobox driver lists only redistribute the zips above (mali-panvk, LukeValen PanVk V7) as far as found; not checked in detail.

## kbase JM on Bifrost: who solved what

| Problem | Ours (v9 JM) | JimVulkan | LukeValen / 0x8055 |
|---|---|---|---|
| Variant 0x2 on G52 r1 (`0x74021000`) | lookup fails (`Unknown gpu_id ... variant (0x2)`) | `props->gpu_variant = 0` in kbase props | not stated |
| `MEM_JIT_INIT` ENOMEM | JIT_INIT called (after EXEC_INIT, jm-v9/004); tiler heap incremental commit (023, v9 only) | no JIT at all; tiler/varying/tess heaps are GROW_ON_GPF (256 pages initial, 512-page steps) | not stated |
| Atom layout | probe: 72 refused, 56 used (022) | fixed 64-byte `base_jd_atom` "captured from the vendor driver" | wrong stride = `JOB_CONFIG_FAULT` (0x8055 fix) |
| Gate | upstream gate | warn and continue on v6/v7 | removed |
| Android 10 (`call_once`) | API 31+ build | API 31 build, Android 12+ only | Android 14 |

The 56 vs 64 stride difference must be checked on a real Bifrost kernel before trusting either; our probe in 022 is the safer mechanism, but it may need 64 added as a candidate.

## Feature gap

Effort is for porting into our tree, given a G52 MC2 test device. "JV" = JimVulkan.

| Feature | Upstream v7 | Best fork (JV r4 / unified) | Ours v9 JM | Port effort for v7 |
|---|---|---|---|---|
| Device creation (gate, variant, JIT) | gated, variant strict, JIT | done | n/a (blocks v7) | 1-2 days (3 small patches + atom stride check) |
| Vulkan 1.3 | yes | yes | yes (003) | 0 |
| robustness2 / nullDescriptor | no | emulated (`null_descs`) | v9 via 010/030 | 2-4 days |
| geometryShader + streams | no | compute-job emulation, NIR lowering | gpu_prerast (Valhall only) | 5-10 days (lowering is arch-independent; JM draw glue conflicts with our v9 edits in `jm/panvk_vX_cmd_draw.c`) |
| tessellation | no | compute emulation | gpu_prerast | 3-6 days (with GS) |
| transform feedback | no | compute emulation (no XFB queries, no xfbDraw) | gpu_prerast | 2-4 days (with GS) |
| multiViewport, clip/cull | no | yes on `< 9` | yes (013, 015) | 1-3 days |
| fillModeNonSolid | no | yes on `< 9` | yes (019) | 1-2 days |
| multiDrawIndirect | no (`>= 10`) | yes | yes (009) | 1-2 days |
| BC1-BC7 | native only | partial (`panvk_bc_emu`, unified repo) | CSF compute decode; JM blit still failing | 3-7 days (take JV or adapt ours) |
| pipelineStatistics, depthBounds, VMR | no | no | yes on v9 | later; DXVK FL11 can live without depthBounds |
| descriptorIndexing | no (`>= 9`) | no | yes on v9 | not needed for DXVK D3D9-11; skip |
| AHB / WSI on MediaTek | generic | MediaTek gralloc slot, AFBC import | ours (v9/v10+) | 1-2 days if our AHB path fails on MTK Bifrost |
| Android 10 (API 29) | n/a | no | no | item 5 of the refreshed list; separate |

## Step plan

1. **Unblock (1-2 days).** In our tree: (a) ignore the variant for arch < 9 in `pan_get_model` (or zero it in kbase props, as JV does); (b) allow v6/v7 without the env var on kbase builds, keeping the warning; (c) on arch < 9 skip `MEM_JIT_INIT` or treat its failure as non-fatal and allocate the tiler heap with GROW_ON_GPF like JV; (d) add 64 to the atom stride probe. Ship as an experimental build; ask a G52 MC2 tester (Helio G85/G88, Redmi 9/10/13C) for PanProbe and logs.
2. **Extract JV's diff (1 day).** Clone `JimVulkan/mesa-unified-drivers` into `/var/tmp/panvk/research/`, find the closest upstream base commit for `src/panfrost/`, and produce a clean per-feature diff (kbase, GS/tess/XFB, robustness2, BC, WSI). Treat as untrusted code; review, do not run.
3. **Port features as `csf-v11`-style numbered patches in a new `jm-v7` series**, gated `PAN_ARCH < 9`, in this order: robustness2/nullDescriptor, MDI, GS + streams, XFB, tessellation, multiViewport/clip/cull, fillModeNonSolid, BC. Keep v9 behaviour unchanged (G615 regression 38/38 and the v9 build must not move).
4. **Validate on device**: PanProbe, CTS subsets (geometry, tessellation, transform_feedback, robustness2, draw), then DXVK D3D9 and D3D11 in PanPlay. Without a device this step is blocked; cloud farms rarely have Bifrost.
5. **v6** after v7 passes: same backend; check G71/G72 rows (TB sizes, no subgroup votes on G71/G72 per JV), and the Android 10 build, since most G71/G72 phones stop at Android 9-11.
6. Apps: show "Bifrost (v6/v7): experimental" instead of INCOMPATIBLE_DRIVER once step 1 lands.

**Estimate:** v7 to DXVK D3D9-D3D11 on a G52 MC2: about 4-6 weeks with a device (1 week unblock and extraction, 2-3 weeks porting, 1-2 weeks device debugging). v6 on top: about 1-2 weeks, plus the API 29 build if Android 9/10 devices are wanted.

## Devices that matter

| Priority | GPU | SoCs / phones | Why |
|---|---|---|---|
| 1 | G52 MC2 (incl. G52 r1 `0x7402`) | MediaTek Helio G80/G85/G88, P65/P70-class MT6768/MT6769 (Redmi 9/10/13C, Galaxy A31/A32 4G, TECNO, Oppo A38) | Largest installed base; three of our cloud/tester logs already hit this GPU. Weak GPU, so D3D9-era games are the realistic target. |
| 2 | G76 MC4/MC10/MC12 | Helio G90T/G95 (Redmi Note 8 Pro), Exynos 9820/9825 (S10/Note 10), Kirin 980/990 | Strongest Bifrost; JV's main test GPU; many are stuck on Android 10/11 (our Note 8 Pro and P40 logs fail on `call_once`). |
| 3 | G52 MC1/MP1 | Exynos 850 (Galaxy A12/M12/A13/A21s) | Common, but very slow. Only needs the gate fix (variant 0). |
| 4 | G72 | Exynos 9611 (A50/A51/Tab S6 Lite), Helio P60/P70, Kirin 970, Exynos 9810 | v6; old, mostly Android 9-11. |
| 5 | G71, G31, G51 | Exynos 8895, low-end TV/boxes | Ignore unless asked. |

## Risks

- **No device in hand.** All fork claims are self-reported; JV and LukeValen state the work was heavily AI-assisted and is not conformant.
- **Code provenance.** JV is MIT and safe to port with attribution (keep the "XclipseTools" headers and Mesa notices). LukeValen, 0x8055 and Noysz have no license: reference only, no copying.
- **Two JM draw paths.** JV's Bifrost GS emulation and our v9 `gpu_prerast` will both live in `jm/panvk_vX_cmd_draw.c`; merge conflicts and divergent behaviour across v7 and v9 are likely. Keep them strictly separated by `PAN_ARCH`.
- **Atom stride and kbase variants.** 56 (ours), 64 (JV), 72 (MediaTek frame_nr) all exist; a wrong value faults every job.
- **Memory.** GROW_ON_GPF heaps avoid the 300+ MB commit JV measured, but 3-4 GB phones are still tight with Wine + DXVK.
- **Performance.** G52 MC2 is far below the G57; compute-emulated GS/tess will be slow. Expect D3D9 and light D3D11 only.
- **Upstream drift.** Upstream poly_heap on JM (!37969) and gfxstrand's GS draft may land a different design; a later rebase may replace the ported code.
