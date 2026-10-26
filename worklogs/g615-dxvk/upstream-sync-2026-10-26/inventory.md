# Patch inventory (generated 2026-10-26)

One row per patch in `patches/`. Columns: number, subject, +added/-removed lines, class, estimated rebase risk onto upstream main `f7dd61f7` (from file overlap, see conflicts.md), note.

Classes: `upstream` = same subject already in upstream main since 5a07217f; `superseded` = upstream solved it differently, drop or re-port; `kbase` = kbase backend (ours only, upstream has DRM panthor/panfrost); `android-wsi`; `perf`; `workaround` (BC emulation and other HW/DDK workarounds); `debug`; `feature` (Vulkan features upstream panvk lacks); `fix`; `policy`.

The class is a first pass from the subject and paths, with hand checks for the `upstream`/`superseded` rows. Treat `perf`/`fix`/`feature` on a single row as approximate.

## common (1 patches)

| # | subject | +/- | class | risk | note |
|---|---|---|---|---|---|
| 019 | nir: load_attr_pan io offset is src 2 | +1/-0 | fix | low (4) |  |

## android (5 patches)

| # | subject | +/- | class | risk | note |
|---|---|---|---|---|---|
| 008 | panvk: robust dma-buf fd discovery and allocationSize query for AHB | +110/-8 | android-wsi | high (360) |  |
| 013 | gralloc: vendor mapper metadata, refuse guesses | +275/-0 | android-wsi | none |  |
| 014 | gralloc: vendor-neutral mapper discovery, HIDL mapper4 backend | +379/-51 | android-wsi | high (360) |  |
| 015 | 015-android-stub-static-hardware.patch | +15/-3 | android-wsi | none |  |
| 016 | gralloc: compile the HIDL mapper4 backend out by default | +24/-5 | android-wsi | none |  |

## kbase-common (10 patches)

| # | subject | +/- | class | risk | note |
|---|---|---|---|---|---|
| 001 | build: add panfrost-kmds option and HAVE_PAN_KMOD_* defines | +24/-0 | kbase | medium (205) |  |
| 002 | kmod: compile kbase_kmod.c when kbase KMD selected | +4/-0 | kbase | low (22) |  |
| 003 | kmod: backend ops hooks for non-DRM backends | +180/-24 | superseded | medium (245) | re-port: upstream kmod now has backend selection (ab1dc701), sync ops (712afcde), submit_job (b3843729), bo_map/bo_export (3395246b/3f05703b), import delegation (37a1dcfb) |
| 004 | panvk: kbase physical-device init path | +936/-0 | kbase | high (972) |  |
| 005 | panvk: enumerate /dev/mali* kbase nodes | +73/-2 | kbase | low (13) | re-port onto upstream backend selection (ab1dc701) and kmod compat checks (9e872e24) |
| 006 | panvk: kbase queue/device/WSI integration | +368/-7 | android-wsi | high (414) |  |
| 010 | panvk: unmap BOs via backend hook | +14/-8 | kbase | high (356) | re-port onto upstream bo_map op (3395246b) |
| 012 | panvk: kbase-aware csif in device init | +1/-1 | kbase | medium (203) |  |
| 014 | panvk: query fd properties without GPU import | +32/-0 | kbase | medium (56) |  |
| 017 | panvk: hide unvalidated extensions | +3/-3 | kbase | medium (42) |  |

## app-loader (1 patches)

| # | subject | +/- | class | risk | note |
|---|---|---|---|---|---|
| 001 | android: export ICD symbols alongside HMI | +4/-2 | android-wsi | low (3) |  |

## wsi (2 patches)

| # | subject | +/- | class | risk | note |
|---|---|---|---|---|---|
| 016 | panvk: Android VK_KHR_android_surface + AHB swapchain | +472/-1 | android-wsi | medium (119) |  |
| 017 | panvk: mark Android WSI swapchain AHBs as driver-owned | +5/-0 | android-wsi | none |  |

## csf (2 patches)

| # | subject | +/- | class | risk | note |
|---|---|---|---|---|---|
| 007 | panvk: kbase CSF queue submit path | +1795/-70 | kbase | medium (62) |  |
| 009 | panvk: route csif/flush-id sites via kbase helpers | +8/-8 | kbase | medium (202) |  |

## csf-v11 (171 patches)

| # | subject | +/- | class | risk | note |
|---|---|---|---|---|---|
| 018 | panvk: GPU prerast VS records to IDVS/FS slice | +1004/-24 | fix | high (707) |  |
| 019 | panvk: invalidate CRC on CLEAR/DONT_CARE | +5/-1 | fix | low (29) |  |
| 020 | panvk: gpu_prerast TLS descriptor, record IO, compute barrier | +127/-16 | feature | medium (121) |  |
| 021 | panvk: gate gpu_prerast VS variants, fix passthrough output base | +23/-4 | feature | medium (45) |  |
| 022 | panvk: BC1-BC7 via GPU compute decode (PANVK_DEBUG=bc_emul) | +7712/-5 | workaround | high (820) | BC / emulation |
| 023 | panvk: shaderClipDistance/shaderCullDistance via FS discard | +212/-7 | feature | medium (87) |  |
| 024 | panvk: expose multiViewport (maxViewports = 16) | +2/-2 | feature | medium (42) |  |
| 025 | panvk: fillModeNonSolid via GPU polygon kernel on gpu_prerast | +371/-19 | feature | high (319) |  |
| 026 | panvk/csf: flush the FAU stores before indirect draws read them | +2/-0 | upstream | low (29) | upstream fbb4993c |
| 027 | panfrost: add the missing v11 cases to the arch switches | +9/-0 | upstream | low (29) | upstream c9c207bf |
| 028 | panvk: Advertise VK_KHR_incremental_present | +1/-0 | upstream | medium (42) | upstream fb385376 |
| 029 | panvk: Advertise VK_EXT_swapchain_colorspace | +1/-0 | upstream | low (11) | upstream eef8db80 |
| 030 | pan/afbc: Support X-component format variants in AFBC and blend tables | +9/-0 | upstream | low (13) | upstream 6c89f9d2 |
| 031 | pan/mod: Introduce modifier layout capabilities query | +190/-122 | upstream | high (320) | upstream 2042227e |
| 032 | panvk: Query modifier capabilities for format property advertisement | +121/-142 | upstream | high (733) | upstream 98ed8110 |
| 033 | panvk: Enable AFBC for WSI images by default | +4/-4 | upstream | high (493) | upstream 50bda0b8 |
| 034 | panvk: Zero images built for device-level queries | +2/-2 | upstream | medium (229) | upstream 90be04d4 |
| 035 | panvk: Report image compression properties | +77/-17 | upstream | high (733) | upstream 19f5cf79 |
| 036 | panvk: Honor VkImageCompressionControlEXT | +27/-3 | upstream | high (709) | upstream e3a315d6 |
| 037 | panvk: Advertise VK_EXT_image_compression_control | +4/-0 | upstream | medium (42) | upstream 6409a5b1 |
| 038 | panvk: unmap CRC init mapping through pan_kmod_bo_munmap | +2/-1 | fix | medium (229) |  |
| 039 | panvk: BC GPU decode on by default when HW lacks BC (no_bc_emul opts out) | +3/-3 | workaround | high (493) | BC / emulation |
| 040 | panvk: decode BC emulated images leaving ZERO_INITIALIZED layout | +53/-0 | workaround | medium (55) | BC / emulation |
| 041 | panfrost/kmod: zero BO pages at allocation | +14/-0 | kbase | none |  |
| 042 | panvk: geometry shaders on gpu_prerast | +1012/-44 | feature | high (413) |  |
| 043 | panvk/csf: account tiler work for heap renewal | +30/-0 | fix | medium (138) |  |
| 044 | panvk: fix gpu_prerast VS pointer qualifier | +1/-1 | feature | low (29) |  |
| 045 | panvk: order compute after gpu_prerast | +7/-3 | feature | medium (47) |  |
| 046 | panvk: handle GS split and indirect draws | +164/-20 | fix | medium (76) |  |
| 047 | panvk: route non-fill polygon draws through GPU prerast | +20/-4 | fix | medium (74) |  |
| 048 | panvk: enable geometry shaders by default | +3/-3 | feature | medium (55) |  |
| 049 | panvk: block instead of busy-spinning for query results | +19/-1 | feature | none |  |
| 050 | panvk: implement pipeline statistics queries on CSF | +649/-4 | feature | medium (133) |  |
| 051 | panvk: count GS and clipping statistics in gpu_prerast | +59/-0 | feature | medium (74) |  |
| 052 | panvk: expose pipelineStatisticsQuery on v10+ | +1/-1 | feature | medium (42) |  |
| 053 | panvk: synchronize and read complete pipeline statistics reports | +7/-6 | feature | low (14) |  |
| 054 | panvk: order pipeline statistics copy before reset | +8/-0 | feature | none |  |
| 055 | panvk: use 2D dimension for layered cube attachment and storage views | +5/-0 | fix | medium (34) |  |
| 056 | panvk: cache the memory budget query for 100 ms | +24/-2 | perf | high (492) | overlaps upstream ba3dacb9 track heap budget across instances; re-check |
| 057 | panfrost/kbase: report the timestamp frequency from cntfrq | +13/-8 | kbase | medium (62) |  |
| 058 | panvk/csf: skip texture cache invalidation for non-texel buffer barriers | +5/-0 | perf | medium (47) |  |
| 059 | panvk/csf: add opt-in GPU waits for same-queue kbase semaphores | +85/-7 | kbase | high (554) |  |
| 060 | panvk: lower minStorageBufferOffsetAlignment to 4 on v9+ | +2/-3 | fix | medium (87) |  |
| 061 | panvk: use INTERSECT ZS preload on v11 | +11/-0 | fix | low (22) |  |
| 062 | nir,panvk: add a robust SSBO load vectorizer behind PANVK_DEBUG=robust_ssbo_vec | +413/-0 | feature | medium (162) |  |
| 063 | nir/opt_load_store_vectorize: allow robust merges that cannot overflow | +9/-0 | fix | low (6) | generic NIR change, upstreamable |
| 064 | panvk/csf: select all layers of full tiler descriptors in dynamic fullscreen runs | +2/-1 | fix | low (29) |  |
| 065 | panvk: implement tessellation on the gpu_prerast path | +1199/-107 | feature | high (756) |  |
| 066 | panvk: implement transform feedback on the gpu_prerast path | +854/-25 | feature | high (316) |  |
| 067 | panvk/csf: pad the tiler geometry buffer allocation by one page | +1/-1 | feature | medium (62) |  |
| 068 | panvk: expose VK_EXT_transform_feedback on v10+ | +17/-0 | feature | medium (42) |  |
| 069 | panvk: expose VK_EXT_memory_priority and VK_EXT_pageable_device_local_memory on v10+ | +10/-0 | feature | medium (98) |  |
| 070 | panvk: expose alphaToOne on v10+ | +2/-2 | feature | medium (42) |  |
| 071 | panvk: raise maxGeometryShaderInvocations to 64 on v10+ | +1/-1 | feature | medium (42) |  |
| 072 | panvk: implement VK_EXT_multi_draw on v10+ | +97/-5 | feature | medium (71) |  |
| 073 | panvk: implement VK_EXT_primitives_generated_query on v10+ | +34/-26 | feature | medium (116) |  |
| 074 | panvk: expose variableMultisampleRate on v10+ | +75/-15 | feature | medium (71) |  |
| 075 | panvk: export kbase sync_file fences through a KCPU queue | +171/-87 | kbase | high (554) |  |
| 076 | panvk: honour the geometry shader viewport index on v10+ | +548/-136 | feature | high (352) |  |
| 077 | panvk: use system scope for subqueue sync signals on kbase | +19/-6 | kbase | low (29) |  |
| 078 | panvk: expose vertexPipelineStoresAndAtomics on v10-v12 via gpu_prerast | +69/-25 | feature | medium (116) |  |
| 079 | panvk: emit a real gl_PointSize output for point-mode TES before poly lowering | +16/-2 | fix | medium (45) |  |
| 080 | panvk: allocate the gpu_prerast arena when vertexPipelineStoresAndAtomics is enabled | +2/-1 | feature | medium (203) |  |
| 081 | panvk: derive IDVS flags from the bound VS variant for gpu_prerast draws | +2/-1 | feature | low (29) |  |
| 082 | panvk: keep gpu_prerast cross-subqueue syncs in CSF event memory | +110/-41 | feature | high (399) |  |
| 083 | build: dlopen the X11/XCB libraries for the Android X11 WSI | +221/-1 | android-wsi | high (417) |  |
| 084 | panvk: complete present ids on the X11 software present path | +10/-1 | android-wsi | medium (253) |  |
| 085 | panvk: keep the descriptor ring and VkEvent syncs in CSF event memory | +188/-38 | kbase | high (339) |  |
| 087 | panvk: honour conditional rendering in the tessellation compute loop | +143/-22 | feature | low (29) |  |
| 088 | panvk: use TES primitive ID in tess-fed geometry shaders | +61/-28 | feature | medium (45) |  |
| 089 | panvk: draw GS viewport selections as ordered runs | +957/-35 | feature | medium (65) |  |
| 090 | panvk: select viewports for VS and TES ViewportIndex writes | +350/-142 | feature | high (359) |  |
| 091 | panvk: allocate placeable host memory from the dma-heap | +8/-2 | android-wsi | medium (56) |  |
| 092 | panvk: emulate the depth bounds test in the fragment shader | +204/-7 | workaround | high (461) | BC / emulation |
| 093 | panvk: do not predicate tessellation state emission on conditional rendering | +8/-15 | feature | low (29) |  |
| 094 | panvk: chunk large and indirect gpu_prerast draws on the GPU | +554/-46 | feature | medium (63) |  |
| 095 | panvk: give attachment-less secondaries a context with their sample count | +45/-7 | fix | medium (31) |  |
| 096 | panvk: plan restart strip chunks with the whole workgroup | +91/-35 | fix | medium (34) |  |
| 097 | panvk: share TLS across command buffers and fault-back gpu_prerast arenas | +98/-18 | kbase | high (352) |  |
| 098 | panvk: emit only VERTEX_TILER_STARTED heap ops on kbase | +23/-4 | kbase | low (29) |  |
| 099 | panvk: report the PanVK-kbase release in driverInfo | +6/-2 | debug | medium (42) | driverInfo branding; replaced by staging-115/int 0001 |
| 100 | panvk: commit gpu_prerast arenas up front on kbase | +7/-8 | kbase | medium (203) |  |
| 101 | panvk: map the CSF trace buffer at a kbase-assigned VA | +19/-13 | debug | medium (62) |  |
| 102 | panvk: renew the kbase tiler heap without a graphics drain | +82/-52 | kbase | medium (62) |  |
| 103 | panvk: wait for same-queue kbase semaphores on the GPU by default | +9/-4 | kbase | medium (62) |  |
| 104 | panvk: pre-create the next kbase tiler heap on a worker thread | +90/-1 | kbase | medium (62) |  |
| 105 | panvk: reset the CS trace buffers before each kbase submit | +32/-17 | debug | medium (62) |  |
| 106 | wsi/x11: wait for sw present fences on the present thread | +12/-2 | android-wsi | high (363) |  |
| 107 | panvk: keep up to three retired kbase tiler heaps in flight | +68/-38 | kbase | medium (62) |  |
| 108 | panvk: emulate BC unless every BC format is native | +19/-7 | workaround | high (480) | BC / emulation |
| 109 | panvk: port viewport depth runs to v12+ and lift depthBounds/shaderOutputViewportIndex gates | +17/-23 | feature | medium (71) |  |
| 110 | kbase: select CSF uAPI layouts by version, support 16 KiB pages, fall back from MEM_ALLOC_EX | +264/-98 | kbase | medium (62) |  |
| 111 | panvk: never return VK_ERROR_DEVICE_LOST from SetEvent/ResetEvent | +11/-11 | fix | none |  |
| 112 | panvk: split prerast triangle fans, cap fan/tess instance grids, skip conditional chunk draws | +121/-5 | perf | medium (63) |  |
| 114 | panvk: cut attachment-less secondaries per sample count, end contexts on suspend | +179/-32 | perf | medium (81) |  |
| 115 | panvk: keep the FS primitive ID across VS viewport runs | +98/-16 | feature | medium (74) |  |
| 116 | panvk: update ZS after the FS for EarlyFragmentTests with depth bounds on | +27/-8 | feature | medium (74) |  |
| 117 | wsi/x11: sw present via MIT-SHM AttachFd, pace sw FIFO | +152/-4 | android-wsi | medium (253) |  |
| 118 | panvk/kbase: log device decisions at INFO for tester uploads | +19/-0 | debug | high (480) |  |
| 119 | panvk: enable vertexPipelineStoresAndAtomics on v13 and v14 by default | +8/-7 | feature | medium (42) |  |
| 120 | panvk: build a strip table for gpu_prerast GS restart draws | +129/-54 | feature | medium (108) |  |
| 121 | panvk: keep the depth flat for minDepth == maxDepth viewports | +6/-2 | feature | low (2) |  |
| 122 | panvk: advertise robustBufferAccess2 on v10 | +127/-5 | feature | high (830) |  |
| 130 | panvk: run v13 vertex stores through gpu_prerast | +3/-4 | feature | medium (74) |  |
| 131 | panvk: name Immortalis and small GPUs by core count | +26/-2 | superseded | high (480) | upstream 7c648edb correctly report immortalis + 5cc8e63b pan_model_device_name |
| 132 | panvk: wait for the prerast producer before viewport runs | +8/-1 | feature | low (29) |  |
| 140 | panfrost: add the Mali-G710 model row | +3/-0 | fix | low (27) |  |
| 150 | panvk: decode BC through a fragment pass into a compact tiled shadow | +9955/-4236 | workaround | medium (286) | BC / emulation |
| 151 | panvk: store RGBA8 BC shadows as AFRC where the GPU has it | +37/-5 | workaround | high (453) | BC / emulation |
| 152 | pan/bi: walk only live nodes in LCRA interference and solve each node in one pass | +104/-15 | perf | low (21) |  |
| 153 | pan/bi: spill for every node that fails in an LCRA pass, not just the first | +83/-32 | perf | low (21) |  |
| 154 | pan/bi: leave room for the spill reservation in the SSA spill target | +6/-1 | perf | low (21) |  |
| 160 | pan/bi: build LCRA constraints once for both register file sizes, with global-only liveness and a sorted queue | +355/-77 | perf | low (21) |  |
| 161 | pan/bi: update LCRA constraints after spilling instead of rebuilding them, spill in one scan | +270/-44 | perf | low (21) |  |
| 162 | pan/bi: repair SSA with one flat definition map instead of a hash table per block | +76/-18 | perf | none |  |
| 163 | pan/bi: skip optimization loop passes that cannot make progress | +42/-31 | perf | medium (212) |  |
| 164 | pan/bi: keep LCRA constraint rows sparse up to a quarter of the nodes | +8/-6 | perf | low (21) |  |
| 170 | util/disk_cache: keep the shader disk cache on by default on Android | +6/-7 | android-wsi | medium (109) |  |
| 171 | util/os_misc: look each Android property option up once | +37/-1 | android-wsi | low (12) |  |
| 180 | panvk: store BC1/BC2 AFRC shadows at 12 bpp | +14/-5 | workaround | medium (229) | BC / emulation |
| 181 | panvk: advertise robustImageAccess2 on v10+ | +1/-1 | feature | medium (42) |  |
| 184 | pan/bi: emulated robustImageAccess2 alpha fix-up on GPU, v10 only | +84/-1 | workaround | medium (257) | BC / emulation |
| 185 | panvk: store BC1 RGB and BC7 AFRC shadows at 8 bpp | +7/-1 | workaround | medium (229) | BC / emulation |
| 186 | kbase: retry dma-buf MEM_IMPORT with the MTK SLC layout | +58/-1 | kbase | none |  |
| 187 | panfrost: name the (13, 8, 0) model row G725/G925 | +5/-2 | superseded | high (507) | upstream 7c648edb adds G925-Immortalis / G725 rows |
| 188 | panvk: report maxFragmentDualSrcAttachments = 1 | +5/-1 | fix | medium (42) |  |
| 189 | panvk: report 32_BIT_ONLY denorm independence on Valhall | +13/-6 | perf | medium (264) |  |
| 190 | panvk: append the u_trace of earlier secondary VMR segments | +10/-0 | debug | medium (47) |  |
| 191 | panvk: keep firstInstance on gpu_prerast CPU instance splits | +57/-13 | feature | medium (32) |  |
| 192 | panvk: destroy retired kbase tiler heaps on the heap thread | +113/-5 | kbase | medium (62) |  |
| 193 | panvk: carry clip/cull distances through the GS and ViewportIndex to the FS on v10+ | +336/-13 | feature | medium (45) |  |
| 194 | panvk: pass a valid robustness state to the depth-bounds FS compile | +20/-2 | feature | medium (45) |  |
| 195 | panvk: clamp indirect gpu_prerast fans to the arena on v10+ | +63/-0 | feature | medium (31) |  |
| 196 | panvk: run gpu_prerast triangle fans in the GPU chunk loop, opt-in, on v10+ | +384/-2 | feature | low (29) |  |
| 197 | panvk: transcode BC1-BC5 to ETC2/EAC shadows and drop unreadable raw planes | +3543/-22 | workaround | high (753) | BC / emulation |
| 198 | wsi/x11: create the surface when the window is already gone | +15/-8 | android-wsi | medium (253) |  |
| 199 | wsi/x11: clip present damage rects to the image instead of skipping them | +25/-9 | android-wsi | medium (253) |  |
| 200 | panvk: speed up the BC lowmem transcoder and add PANVK_DEBUG=bc_timing | +6112/-2622 | workaround | high (376) | BC / emulation |
| 201 | panvk: keep the raw blocks of readable BC lowmem images in a spillable store | +591/-8 | workaround | high (413) | BC / emulation |
| 202 | panvk: transcode BC6H UF16 lowmem shadows to ASTC 4x4 HDR | +6281/-20 | workaround | medium (273) | BC / emulation |
| 203 | panvk: cut the BC lowmem upload cost on the GPU | +5101/-4808 | workaround | medium (117) | BC / emulation |
| 204 | util/disk_cache: find the Android app cache dir for execve()d helpers | +225/-37 | android-wsi | high (338) |  |
| 205 | panvk/csf: commit the gpu_prerast arenas at their first draw | +101/-32 | perf | high (381) |  |
| 206 | panvk: transcode BC3 alpha, BC4 and BC5 through an endpoint pair table | +17574/-4763 | workaround | high (843) | BC / emulation |
| 207 | panvk: keep BC raw blocks only after the device reads one back | +513/-7 | workaround | high (363) | BC / emulation |
| 208 | panvk: transcode signed BC6H to ASTC 4x4 HDR on request | +3832/-3807 | workaround | medium (237) | BC / emulation |
| 209 | util/u_queue: restart the workers in a fork() child | +123/-1 | fix | medium (138) | generic util fix, upstreamable |
| 210 | panvk: decode BC6H once per block into the shadow with a compute shader | +3578/-16 | workaround | medium (261) | BC / emulation |
| 211 | panvk: decode BC7 with constant bit offsets and keep its raw blocks in the decode | +1984/-16 | workaround | low (8) | BC / emulation |
| 212 | panvk: transcode BC1 through a palette fit without spills | +6665/-4873 | workaround | high (480) | BC / emulation |
| 213 | panvk/csf: count, prefix sum and draw a tess chunk in one workgroup | +209/-10 | fix | high (336) |  |
| 214 | panvk/csf: set up a tess draw in its first step dispatch | +124/-8 | fix | high (334) |  |
| 215 | panvk/csf: alternate tess draws over two tessellation arenas | +99/-6 | feature | high (399) |  |
| 216 | panvk/csf: write counter-clockwise tess triangles in the tessellator | +33/-7 | feature | high (336) |  |
| 217 | panvk: transcode BC2 / BC3 colour through the BC1 palette fit | +4822/-4806 | workaround | high (480) | BC / emulation |
| 218 | panvk: transcode the small levels of a BC upload in one dispatch | +2179/-1838 | workaround | low (8) | BC / emulation |
| 219 | panvk/csf: keep the fused tess workgroup's stack at 4 KiB per thread | +24/-4 | perf | low (29) |  |
| 220 | panvk: release command pool BOs on reset with RELEASE_RESOURCES and on trim | +66/-4 | perf | none |  |
| 221 | panvk: flush the transient pool BO only up to transient_offset | +24/-5 | perf | none |  |
| 222 | panvk/kbase: per-import and per-queue-wait log lines at debug level | +3/-4 | debug | medium (62) |  |
| 223 | panvk: PANVK_DEBUG=mem driver memory accounting and report | +391/-24 | debug | high (357) |  |
| 224 | panvk/kbase: smaller tiler heap initial size and a bounded TERM backlog | +71/-11 | kbase | medium (62) |  |
| 225 | panvk: BC raw stores: no device-wide flip, no pinning, stale raw blocks after a shadow-only copy | +90/-11 | workaround | medium (261) | BC / emulation |
| 226 | panvk: command pool auto-trim on vkResetCommandPool(flags = 0) | +67/-1 | perf | none |  |
| 227 | panvk/csf: free outgrown device TLS BOs once no command buffer uses them | +52/-2 | perf | medium (50) |  |
| 228 | panvk/csf: 64 KiB u_trace heap on kbase without MESA_GPU_TRACES | +11/-4 | debug | none |  |
| 229 | panvk: opt-in honest memory budget and budget headroom knobs | +32/-1 | perf | high (480) |  |
| 230 | panvk/csf: back the device TLS only for present shader cores, shrink it after idle | +227/-11 | android-wsi | medium (56) |  |
| 231 | panvk: command pool auto-trim keeps the recent high-water mark | +40/-3 | perf | none |  |
| 232 | panvk/csf: sparse device TLS: every job uses the stride the BO was built for | +50/-19 | perf | medium (103) |  |
| 233 | panvk: PANVK_FPS_LOG present counter (fps per interval, summary at swapchain destroy and exit) | +277/-1 | android-wsi | none |  |

## jm-v9 (31 patches)

| # | subject | +/- | class | risk | note |
|---|---|---|---|---|---|
| 001 | kbase: JM job atom submission | +592/-0 | kbase | none |  |
| 002 | panvk: Valhall v9 JM backend | +1482/-228 | feature | high (729) |  |
| 003 | panvk/jm: v9 multiview and layered rendering, report Vulkan 1.3 | +76/-12 | feature | medium (191) |  |
| 004 | kbase: init EXEC_VA before JIT, full 4G zone | +19/-13 | kbase | none |  |
| 005 | kbase: size the EXEC_VA/JIT zones and host imports in kernel pages | +10/-8 | kbase | none |  |
| 006 | panfrost: add the Mali-G77 model row | +3/-1 | fix | low (27) |  |
| 007 | panvk/jm: reset the MALLOC_VERTEX vertex array when a command buffer is resubmitted | +29/-0 | fix | medium (134) |  |
| 008 | panvk/jm: apply vkCmdSetEvent when the GPU work before it completes | +31/-6 | fix | low (22) |  |
| 009 | panvk/jm: v9 multiDrawIndirect, drawIndirectCount and gl_DrawID | +151/-39 | feature | medium (227) |  |
| 010 | panvk: v9 robustBufferAccess2 and nullDescriptor | +5/-3 | feature | medium (42) |  |
| 011 | panvk: v9 transform feedback through the gpu_prerast path | +950/-84 | feature | high (446) |  |
| 012 | panvk: v9 geometry shaders and vertex streams through gpu_prerast | +244/-34 | feature | medium (155) |  |
| 013 | panvk: v9 clip/cull distances and vertex pipeline stores | +32/-9 | feature | medium (110) |  |
| 014 | panvk: v9 tessellation via gpu_prerast | +405/-14 | feature | medium (152) |  |
| 015 | panvk: v9 multiViewport and gl_ViewportIndex | +283/-5 | feature | medium (146) |  |
| 016 | panvk: v9 pipeline statistics, primitives generated and XFB queries | +302/-22 | feature | medium (114) |  |
| 017 | panvk: v9 split indirect gpu_prerast draws over the arena | +126/-20 | feature | medium (102) |  |
| 018 | panvk: v9 depth bounds emulation, per-sample shading, alpha-to-coverage and variableMultisampleRate | +69/-7 | workaround | medium (193) | BC / emulation |
| 019 | panvk: v9 fillModeNonSolid via the gpu_prerast polygon-mode kernel | +116/-12 | feature | medium (110) |  |
| 020 | panvk: v9 warn once when indirect prerast/tess draws exceed the chunk caps | +12/-0 | perf | medium (68) |  |
| 021 | panfrost: add the Mali-G78 and G78AE model rows, match v9 ids by product | +20/-0 | fix | low (27) |  |
| 022 | panvk/kbase: JM atom layout probe, fault propagation within a submit, teardown wait | +121/-31 | kbase | low (22) |  |
| 023 | panvk/jm: v9 tiler heap committed in 2 MiB steps, one heap per overlap half | +60/-2 | kbase | high (365) |  |
| 024 | panvk/kbase: v9 clean non-coherent dma-buf imports before submit | +70/-0 | kbase | low (22) |  |
| 025 | panvk: v9 PANVK_V9_RT128 bisect knob for 128-bit render targets | +29/-0 | debug | medium (44) |  |
| 026 | panvk: v9 instance-rate attributes add firstInstance for every divisor | +13/-13 | fix | medium (102) |  |
| 027 | panvk: v9 count compute invocations before the empty-shader skip | +53/-38 | perf | none |  |
| 028 | panvk: v9 chunk direct restart strips, size tess chunks on the GPU | +233/-54 | fix | medium (104) |  |
| 029 | panvk: v9 enable and override the primitive index when shaders use it | +10/-0 | feature | medium (68) |  |
| 030 | panvk: v9 robustImageAccess2, conditional rendering and GPU timestamps | +360/-30 | kbase | high (921) |  |
| 031 | panvk: v9 JM: decode BC images leaving ZERO_INITIALIZED | +7/-0 | workaround | medium (38) | BC / emulation |

## jm-v7 (5 patches)

| # | subject | +/- | class | risk | note |
|---|---|---|---|---|---|
| 001 | panfrost: ignore the GPU variant on Bifrost (v6/v7) in pan_get_model | +10/-0 | fix | low (27) |  |
| 002 | panvk: allow Bifrost v6/v7 on kbase without PAN_I_WANT_A_BROKEN_VULKAN_DRIVER | +2/-7 | policy | high (480) | upstream still gates v6/v7 behind PAN_I_WANT_A_BROKEN_VULKAN_DRIVER |
| 003 | kbase: skip MEM_JIT_INIT on Bifrost JM, GROW_ON_GPF tiler heap with 1 MiB commit | +23/-1 | kbase | none |  |
| 004 | kbase: check the JM atom stride on Bifrost, add stride 64 (seq_nr) candidate | +71/-2 | kbase | none |  |
| 005 | 005-kbase-ion-fallback-for-placed-map-dma-buf.patch | +73/-1 | kbase | none |  |

## jm-v7-wip (87 patches)

| # | subject | +/- | class | risk | note |
|---|---|---|---|---|---|
| 0006 | jm-v7 006: Bifrost draw prerequisites (no IDVS, MDI, position buffer) | +59/-39 | fix | medium (155) |  |
| 0007 | jm-v7 007: Bifrost geometry shader emulation via compute | +2161/-36 | workaround | medium (157) | BC / emulation |
| 0008 | jm-v7 008: Bifrost transform feedback from the geometry emulation | +425/-10 | workaround | medium (235) | BC / emulation |
| 0009 | jm-v7 009: Bifrost transform feedback from vertex shaders (passthrough GS) | +433/-17 | feature | medium (191) |  |
| 0010 | jm-v7 010: Bifrost robustBufferAccess2 and nullDescriptor | +309/-19 | feature | high (503) |  |
| 0011 | jm-v7 011: fix geometry emulation NIR and the GS tiler DCD | +16/-8 | workaround | medium (68) | BC / emulation |
| 0012 | jm-v7 012: keep the emulated geometry shader across meta operations | +13/-1 | workaround | low (8) | BC / emulation |
| 0013 | jm-v7 014: Bifrost tessellation | +1020/-89 | feature | high (523) |  |
| 0014 | jm-v7 015: Bifrost depthBounds through the FS depth bounds emulation | +24/-13 | workaround | medium (193) | BC / emulation |
| 0015 | jm-v7 016: Bifrost pipeline statistics and transform feedback queries | +426/-27 | feature | medium (156) |  |
| 0016 | jm-v7 017: Bifrost multiViewport through the geometry emulation | +189/-27 | workaround | medium (155) | BC / emulation |
| 0017 | jm-v7 018: Bifrost fillModeNonSolid through a polygon-mode passthrough GS | +174/-11 | feature | medium (155) |  |
| 0018 | jm-v7 019: kbase: try the 48-byte JM atom on Bifrost (pre-11.22 kbase) | +11/-2 | kbase | none |  |
| 0019 | jm-v7 020: PANVK_DEBUG=ria2 exposes robustImageAccess2 on Bifrost for measurement | +13/-3 | feature | medium (55) |  |
| 0020 | jm-v7 021: Bifrost queries: cached availability words and timestamp queries | +38/-24 | perf | high (554) |  |
| 0021 | jm-v7 022: Bifrost depth bounds: give the depth LD_TILE its conversion | +32/-2 | feature | medium (83) |  |
| 0022 | jm-v7 030: Bifrost restart in the GS emulation, per-viewport depth, VS viewport index, large tessellation, tes | +656/-52 | workaround | high (337) | BC / emulation |
| 0023 | jm-v7 031: Bifrost FS gl_PrimitiveID behind the geometry shader emulation | +26/-2 | workaround | medium (70) | BC / emulation |
| 0024 | jm-v7 023: Bifrost query ordering, large indirect xfb, strip-adjacency order, xfb cull capture | +83/-16 | feature | medium (141) |  |
| 0025 | jm-v7 024: Bifrost instanced attributes undo the DCD offset_start | +17/-1 | fix | medium (108) |  |
| 0026 | jm-v7 025: draw a geometry shader behind an empty vertex shader | +32/-10 | feature | medium (68) |  |
| 0027 | jm-v7 040: VK_EXT_conditional_rendering on v6/v7 | +193/-172 | fix | medium (180) |  |
| 0028 | jm-v7 041: shaderBufferInt64Atomics and shaderSharedInt64Atomics on v6/v7 | +4/-4 | perf | medium (42) |  |
| 0029 | jm-v7 042: variableMultisampleRate on v6/v7 | +17/-2 | feature | medium (110) |  |
| 0030 | jm-v7 043: null storage images read (0,0,0,0) on v6/v7 | +34/-4 | feature | medium (203) |  |
| 0031 | jm-v7 044: customBorderColorWithoutFormat on v7 | +32/-6 | feature | medium (283) |  |
| 0032 | jm-v7 045: vertexPipelineStoresAndAtomics on v6/v7 | +4/-3 | feature | medium (42) |  |
| 0033 | jm-v7 046: skip vertex shader side effects of unreferenced vertices on v6/v7 | +216/-1 | perf | medium (147) |  |
| 0034 | jm-v7 047: lower 64-bit arithmetic atomics to an ACMPXCHG loop on Bifrost | +26/-1 | fix | medium (212) |  |
| 0035 | jm-v7 048: robust storage images on v6/v7: in-bounds null image, robustImageAccess2 bounds check | +122/-7 | feature | medium (248) |  |
| 0036 | jm-v7 049: gl_PrimitiveIDIn of a tess-fed geometry shader is the patch ID on v6/v7 | +44/-2 | feature | medium (68) |  |
| 0037 | jm-v7 035: guarded VS atomics return their result again on v6/v7 | +22/-1 | fix | medium (45) |  |
| 0038 | jm-v7 036: take the jm-v7 049 tess patch ID path in the GS emulation | +17/-3 | workaround | none | BC / emulation |
| 0039 | jm-v7 026: capture xfb gl_CullDistance from a raw copy on v6/v7 | +58/-4 | feature | medium (45) |  |
| 0040 | jm-v7 027: tessellate up to 2^18 patches per draw on v6/v7 | +84/-14 | feature | medium (102) |  |
| 0041 | jm-v7 070: rebuild the FS RSD when the depth bounds test enable changes | +4/-1 | feature | medium (68) |  |
| 0042 | jm-v7 071: keep query availability words out of the WB-mapped pool on v6/v7 | +22/-4 | feature | none |  |
| 0043 | jm-v7 072: reserve 32 job indices per layer for a v6/v7 draw | +27/-5 | fix | medium (68) |  |
| 0044 | jm-v7 073: size the v6/v7 geometry emulation buffers in 64 bits | +40/-12 | workaround | medium (68) | BC / emulation |
| 0045 | jm-v7 074: keep the xfb scan job of a conditional draw unpredicated | +52/-3 | feature | medium (114) |  |
| 0046 | jm-v7 075: use IDVS again for Bifrost vertex shaders that need no vertex job | +40/-5 | fix | medium (45) |  |
| 0047 | jm-v7 076: unlowered fragment shader while the depth bounds test is off | +154/-6 | feature | medium (113) |  |
| 0048 | jm-v7 077: screen-space vertex shader for xfb VS draws without transform feedback | +36/-4 | feature | medium (113) |  |
| 0049 | jm-v7 078: GS emulation, xfb and restart buffers from the GPU-only pool | +43/-21 | workaround | medium (68) | BC / emulation |
| 0050 | jm-v7 079: clip-distance discard behind a v6/v7 GS only for slots it writes | +21/-5 | perf | medium (45) |  |
| 0051 | jm-v7 080: cheaper vertex store dedup on v6/v7 | +68/-16 | perf | medium (191) |  |
| 0052 | jm-v7 050: bindless update-after-bind and mutable bindings on v6/v7 | +637/-20 | feature | high (527) |  |
| 0053 | jm-v7 051: descriptorIndexing, mutableDescriptorType and robustImageAccess2 on v6/v7 | +52/-37 | feature | medium (42) |  |
| 0054 | jm-v7 081: build if-phi constants before the if on v6/v7 | +37/-34 | fix | medium (116) |  |
| 0055 | jm-v7 082: zero the first output vertex of each v6/v7 GS invocation | +33/-1 | fix | none |  |
| 0056 | jm-v7 083: robustImageAccess2 for uniform texel buffers on v6/v7 | +64/-2 | feature | medium (45) |  |
| 0057 | jm-v7 084: null uniform texel buffers read (0,0,0,0) under robustImageAccess2 on v6/v7 | +6/-2 | feature | medium (45) |  |
| 0058 | jm-v7 085: waterfall divergent texture indices with every lane active on v6/v7 | +138/-1 | fix | medium (45) |  |
| 0059 | jm-v7 086: reorderable v6/v7 descriptor loads at a constant set offset | +32/-2 | perf | medium (71) |  |
| 0060 | jm-v7 087: waterfall only NonUniform texture indices on v6/v7, clean up after | +36/-8 | perf | medium (45) |  |
| 0061 | jm-v7 088: keep PANVK_OVERLAP for v6/v7 render passes that begin a query | +41/-3 | feature | none |  |
| 0062 | jm-v7 089: keep AFBC for unsampled R/B-swapped v7 images with customBorderColorWithoutFormat | +23/-2 | feature | medium (229) |  |
| 0063 | jm-v7 090: fill the v6/v7 VS storage-image table on every draw | +11/-2 | fix | medium (68) |  |
| 0064 | jm-v7 091: hash the v6/v7 compile knobs into the shader cache key | +77/-15 | perf | medium (116) |  |
| 0065 | jm-v7 092: no IDVS by default on any v6 GPU | +6/-3 | fix | medium (45) |  |
| 0066 | jm-v7 093: v6/v7 kbase atom waits use CLOCK_MONOTONIC deadlines | +10/-3 | kbase | low (22) |  |
| 0067 | jm-v7 094: optional dma-buf import clean on v6/v7 (PANVK_KBASE_IMPORT_SYNC=1) | +13/-5 | kbase | low (22) |  |
| 0068 | jm-v7 095: read_invocation uses the old CLPER encoding on G71/G72/G31 | +10/-1 | fix | medium (269) |  |
| 0069 | jm-v7 096: strided index min/max search, folded into small direct draws | +245/-69 | fix | medium (102) |  |
| 0070 | jm-v7 097: per-set descriptor dirty tracking, cached push uniforms and attribute tables on v6/v7 | +205/-18 | perf | medium (70) |  |
| 0071 | jm-v7 098: rebinding a v6/v7 shader is not a change while its alternate is in use | +31/-1 | fix | medium (45) |  |
| 0072 | jm-v7 099: one tiler heap BO per PANVK_OVERLAP half on v6/v7 | +49/-4 | fix | medium (263) |  |
| 0073 | panvk/jm: defer occlusion EndQuery availability writes to batch close (jm-v7 100) | +81/-1 | feature | medium (40) |  |
| 0074 | jm-v7 101: v6 robustImageAccess2 out-of-bounds alpha from the view format | +54/-4 | feature | medium (116) |  |
| 0075 | jm-v7 102: v6 texture offsets packed with 32-bit shifts | +23/-3 | fix | high (512) |  |
| 0076 | jm-v7 103: v6 ballot without WMASK | +32/-1 | fix | medium (269) |  |
| 0077 | jm-v7 104: v6/v7 counter-clockwise tess triangles from the tessellator | +9/-4 | feature | medium (102) |  |
| 0078 | jm-v7 105: v6/v7 nested tess draws: tight GS emulation size, small grow steps | +62/-2 | kbase | medium (68) |  |
| 0079 | jm-v7 106: v6/v7 tessellation memory reused per batch, bounded per batch | +208/-11 | perf | high (441) |  |
| 0080 | jm-v7 107: one TLS/WLS buffer per heap half per command buffer; 0079 review fixes | +86/-9 | fix | medium (112) |  |
| 0081 | jm-v7 108: v6/v7 polygon mode in the shader cache key | +29/-6 | perf | medium (45) |  |
| 0082 | jm-v7 109: kbase JM GPU timestamp frequency calibrated against the kernel clock | +125/-1 | kbase | none |  |
| 0083 | jm-v7 110: kbase timestamp frequency measured when cntfrq_el0 is 0 | +110/-34 | kbase | none |  |
| 0084 | jm-v7 111: kbase JM 2 s warm-up sleep only on the first open per process | +59/-4 | kbase | none |  |
| 0085 | jm-v7 112: release the gs_bo_pool free list on pool reset and trim | +2/-1 | perf | none |  |
| 0086 | jm-v7 113: drop the dead per-batch BO list on JM submit | +7/-46 | fix | low (22) |  |
| 0087 | jm-v7 114: kbase JM: read up to 16 job events per read() | +21/-5 | kbase | none |  |
| 0088 | jm-v7 115: Bifrost growable BO log line at info level only 8 times | +12/-4 | debug | none |  |
| 0089 | jm-v7 116: v6/v7 bindless texel buffer bounds without a divide | +40/-5 | perf | medium (118) |  |
| 0090 | jm-v7 117: v9 JM tiler heaps 512 MiB each (tess_perf gs=1 OUT_OF_MEMORY) | +18/-3 | fix | medium (203) |  |
| 0091 | jm-v7 118: v9 gpu_prerast CPU instance splits keep firstInstance | +58/-7 | feature | medium (74) |  |
| 0092 | jm-v7 119: v9 indirect gpu_prerast fans clamped to the arena | +100/-1 | feature | medium (68) |  |

## bcn-layer (1 patches)

| # | subject | +/- | class | risk | note |
|---|---|---|---|---|---|
| 001 | export Android layer enumeration entrypoints | +48/-0 | android-wsi | none |  |

## staging-115/pa (10 patches)

| # | subject | +/- | class | risk | note |
|---|---|---|---|---|---|
| 0001 | panvk/csf: leave the FS state clean across draws that do not run the FS | +122/-2 | fix | medium (42) |  |
| 0002 | panvk: v9+ unlowered fragment shader while the depth bounds test is off | +136/-40 | feature | medium (193) |  |
| 0003 | panvk: PANVK_DEPTH_BOUNDS=0 hides the depthBounds feature | +9/-3 | feature | medium (53) |  |
| 0004 | panvk/csf: rebuild the VS tables and push uniforms after a repeated indirect draw | +20/-1 | fix | medium (40) |  |
| 0005 | panvk/csf: descriptor binds and pushes dirty only the stages that use the sets | +84/-5 | perf | medium (42) |  |
| 0006 | panvk: vkCmdBindVertexBuffers2 dirties VB only when a binding changes | +34/-11 | perf | medium (49) |  |
| 0007 | panvk/csf: evaluate the gpu_prerast triggers once for a plain draw | +23/-7 | perf | medium (40) |  |
| 0008 | kbase: decouple shaderDeviceClock from the coherency mode, log the mode | +66/-7 | debug | none |  |
| 0009 | kbase: opt-in system-coherent CPU-cached BOs on ACE-Lite/ACE (CSF) | +95/-7 | kbase | none |  |
| 0010 | panvk: descriptor pools search for a free set from a hint, reset visits used sets | +44/-4 | fix | low (13) |  |

## staging-115/pb (10 patches)

| # | subject | +/- | class | risk | note |
|---|---|---|---|---|---|
| 0001 | panvk/csf: destroy completed retired tiler heaps at every submission | +18/-1 | fix | medium (62) |  |
| 0002 | panvk/csf: tessellation arenas with an 8 MiB VS region | +84/-23 | feature | high (334) |  |
| 0003 | panfrost/kmod, panvk: kbase: zero only memory that has to be zero | +43/-6 | kbase | high (556) |  |
| 0004 | panvk/csf: flush only the written part of CS chunks at EndCommandBuffer | +112/-5 | perf | high (355) |  |
| 0005 | panvk: command pool auto-trim on JM too | +15/-7 | perf | high (305) |  |
| 0006 | panvk: memory budget that shrinks under memory pressure | +99/-8 | perf | high (492) |  |
| 0007 | panfrost/cs_builder, panvk/csf: CS chunks that grow from 4 KiB | +62/-7 | perf | high (359) |  |
| 0008 | panvk: device-wide caches for command buffer CS, descriptor and TLS slabs | +222/-3 | perf | high (314) |  |
| 0009 | kbase, panvk: opt-in evictable command buffer slabs (BASE_MEM_DONT_NEED) | +209/-13 | kbase | medium (212) |  |
| 0010 | panvk/csf: PANVK_KBASE_HEAP_SLOTS, opt-in fewer live tiler heaps | +18/-8 | kbase | medium (62) |  |

## staging-115/pc (7 patches)

| # | subject | +/- | class | risk | note |
|---|---|---|---|---|---|
| 0001 | panvk/wsi: name the extra_xwayland_image device option | +3/-1 | android-wsi | none |  |
| 0002 | panvk: use INTERSECT ZS preload on v9 and v10 too | +8/-9 | fix | low (22) |  |
| 0003 | wsi: read sw frames back through cached memory, invalidate first (PANVK_WSI_SW_CACHED) | +103/-4 | android-wsi | high (378) |  |
| 0004 | wsi/x11: collect sw MIT-SHM present replies on the event thread (PANVK_WSI_SW_DEFER_REPLY) | +150/-17 | android-wsi | medium (268) |  |
| 0005 | panvk/wsi: Android present waits for the image's fence, not the device (PANVK_ANDROID_PRESENT_FENCE) | +16/-3 | android-wsi | low (11) |  |
| 0006 | panvk/wsi: Android swapchain presents on its own thread (PANVK_ANDROID_PRESENT_THREAD) | +181/-22 | android-wsi | low (11) |  |
| 0007 | panvk/csf: skip the preload drain when no fragment job wrote what the pass loads (PANVK_PRELOAD_BARRIER_TRACK) | +296/-3 | perf | medium (127) |  |

## staging-115/int (1 patches)

| # | subject | +/- | class | risk | note |
|---|---|---|---|---|---|
| 0001 | panvk: report driverInfo like upstream Mesa ("Mesa <version> (git-<sha>)") | +3/-13 | debug | medium (42) | driverInfo in upstream format; keep |

## staging-115/pd (7 patches)

| # | subject | +/- | class | risk | note |
|---|---|---|---|---|---|
| 0001 | panvk/jm: v6/v7 FS descriptor copy job only when the FS tables change | +37/-17 | perf | high (373) |  |
| 0002 | panvk/jm: clean the descriptor pool once per resubmitted command buffer | +30/-8 | perf | high (327) |  |
| 0003 | panvk/jm: v6/v7 tess winding option read once per device | +10/-6 | perf | medium (271) |  |
| 0004 | panvk/jm: v6/v7 tess draws share a batch: per-draw charge capped | +23/-3 | perf | high (373) |  |
| 0005 | panvk/jm: v9 gpu_prerast arenas committed at first use, as on CSF | +18/-6 | perf | medium (271) |  |
| 0006 | kbase: JM warm-up sleep off by default | +21/-7 | kbase | none |  |
| 0007 | panvk/jm: the TLS/WLS pool skips the kbase CPU clear | +8/-3 | kbase | medium (38) |  |

