# D3D feature levels: PanProbe report and PanPlay tests (2026-10-27)

Driver: build 115, `/home/flux/build-out/2026-10-26-115/libvulkan_panfrost.so` (sha256 `73709644...`,
`Mesa 26.3.0-devel (git-9dde9e7a18)`), unchanged. Nothing here has run on a device yet: the Firebase commands below
are for the next device session.

## What was added

**PanProbe 1.3.0 (versionCode 17), checker `d3d_fl`** ("D3D feature levels" on the Info page, in `compliance.json`
and the result upload, run label `FL`). Sources: `tests/vkd3d/d3d_fl.h` (the level logic, no Vulkan),
`tests/vkd3d/d3d_fl.c` (queries the driver), `tests/vkd3d/d3d_fl_test.c` (host assert test against the G615 numbers
in `docs/vkd3d-vulkan-requirements.md` and the build 115 FTL caps: `cc -O1 -o /tmp/t tests/vkd3d/d3d_fl_test.c && /tmp/t`).

- D3D12 11_0, 11_1, 12_0, 12_1, 12_2 decided as vkd3d-proton 2230755 does (`d3d12_device_caps_init_feature_level` and
  the tier functions in `libs/vkd3d/device.c`), plus HighestShaderModel, ResourceBindingTier, TiledResourcesTier,
  ConservativeRasterizationTier, ROVsSupported and TypedUAVLoadAdditionalFormats (all 18 formats).
- D3D11 9_1 to 12_1 decided as DXVK 3.1.1 does (`D3D11Device::GetMaxFeatureLevel`).
- One SOFT line per missing level and blocking cap: `D3D12 FL12_0 needs <cap> -- <Vulkan feature/extension> | vN: <reach>`,
  where `<reach>` is the per-arch verdict of the table below (by GPU_ID arch major).
- HARD items: `D3D12 FL11_0` and `D3D11 FL11_0`. RESULT PASS means both floors; 11_1 and up are SOFT, so a device is
  not FAIL for known hardware gaps (making a level HARD is one line in `d3d_fl.c`).

**PanPlay 1.3.0 (versionCode 42), `d3d12fl` and `d3d11fl`** (`apps/panvk-launcher/tests/dfl`, own code, MIT):
x86_64 and ARM64EC exes in `assets/builtin`, shortcuts "D3D12 · Feature levels" and "D3D11 · Feature levels".
Shaders are precompiled DXBC (`dfl.hlsl`, `dfl4.hlsl` -> `dfl_shaders.h` with vkd3d-compiler 1.19); `build-dfl.sh`
rebuilds everything. Each exe takes a level (argument or `D3DFL_LEVEL`, default all), creates the device at that level,
runs the level's features with readback checks, then renders a turning triangle for `-secs N` (`D3DFL_SECS`; the
background is green, amber or red for PASS, UNSUPPORTED, FAIL). Output, one line each:

```
D3DFL: feature level=<l> <name> PASS|FAIL|UNSUPPORTED|REPORTED <detail>
D3DFL: level <l> PASS|FAIL|UNSUPPORTED pass=.. fail=.. unsupported=.. reported=.. <why>
D3DFL: PASS|FAIL|UNSUPPORTED api=.. level=<l> created|not_created <name:status,...> frames= secs= fps= startup=
D3DFL: DONE|FAIL api=.. level=all <l>=<verdict> ... frames= secs= fps= startup=      (all levels)
```

UNSUPPORTED (level) = the device cannot be created at that level; its features still run on the highest device (D3D12)
or are skipped (D3D11). FAIL = a feature the device claims gave a wrong result, or a feature of a level the device was
created at is missing. REPORTED = informational, not counted. The game loop treats UNSUPPORTED and DONE as a pass.

Game loop scenarios (1-6 unchanged): **7** `d3d12fl` = one pass per D3D12 level 11_0 .. 12_2; **8** `d3d11fl` = D3D11
10_0, 10_1, 11_0, 11_1, then the D3D10 and D3D9 cubes (D3D9/D3D10 smoke = the existing `dxcube`); **9** `fl-quick` =
both exes, all levels, one pass each.

## What each PanPlay test checks

| level | feature | check | PASS when |
|---|---|---|---|
| D3D12 11_0 | tess | tri patch with zero area (on y=0), factor 4, the DS lifts interior points by 8uvw | >= 20 px covered (0 without tessellation) |
| | gs | 1 point in, full-screen strip out of the GS | >= 90 % of 64x64 covered |
| | compute | 64 threads write i*3+1 to a structured UAV | all 64 values |
| | mrt | 2 render targets, red and blue | both centre pixels exact |
| | msaa4x | 4x RGBA8 triangle, diagonal through pixel centres, ResolveSubresource | >= 16 edge px partly covered, >= 1000 full |
| D3D12 11_1 | uav_vs | InterlockedAdd on u1 from the VS | count >= 3 |
| | logic_op | R8G8B8A8_UINT: plain write of (ff,0f,f0,aa), then logic op INVERT | 0x550ff000 (skipped as UNSUPPORTED if OutputMergerLogicOp=0) |
| | tir | UAV-only band between pixel centres, ForcedSampleCount 4 vs 0 | REPORTED: vkd3d-proton 2230755 ignores ForcedSampleCount (`libs/vkd3d/state.c` FIXME), so TIR is tested in D3D11 |
| D3D12 12_0 | typed_uav_load | RWTexture2D<float4> on R8G8B8A8_UNORM: load*2 + bias | all 64 texels within 2 of (160,192,144,128) |
| | binding_tier2 | 1,000,000-descriptor heap, t0 at index 999999, u0..u63 table at 999900 (62 null UAVs) | u0 = 42, u63 = 43 |
| | tiled_tier2 | 128 KiB reserved buffer, tile 0 mapped, tile 1 not; raw stores to both | mapped 0x1234, unmapped reads 0 |
| D3D12 12_1 | rov | RasterizerOrderedTexture2D<uint>, 3 full-screen instances, v = v*3 + inst + 1 | every pixel 18 (only in primitive order) |
| | conservative | sub-pixel triangle inside pixel (10,10) that misses its centre, PS invocation count | >= 1 conservative, 0 normal |
| D3D12 12_2 | sm6_5, mesh_shader, dxr1_1, vrs_tier2, sampler_feedback | caps only | REPORTED if present, else UNSUPPORTED |
| D3D11 10_0 | gs, stream_out | ps_4_0/gs_4_0: 1 point to a full-screen strip, rasterized and streamed out | >= 90 % covered; first streamed triangle exact |
| D3D11 10_1 | cube_array | ps_4_1: TextureCubeArray (2 cubes), cube 1 +X face | green |
| | gather4 | ps_4_1: Gather of a 2x2 R8 texture | (30,40,20,10) +-1 |
| D3D11 11_0 | tess, compute | as D3D12 (SM 5.0) | as D3D12 |
| D3D11 11_1 | uav_vs | VS InterlockedAdd on OM UAV slot 1 (OMSetRenderTargetsAndUnorderedAccessViews) | count >= 3 |
| | logic_op | ID3D11Device1::CreateBlendState1, as D3D12 | 0x550ff000 |
| | tir | CreateRasterizerState1 ForcedSampleCount 4 vs 0, UAV-only band between pixel centres | >= 32 invocations with 4, more than with 0 |

Every level of every test is one game-loop pass: about 110-120 s (Wine start, 20 s warm-up, 60 s measured, 8 s tail).

## Expected results (build 115)

Measured caps: PanProbe `vkd3d_reqs`/`dxvk_reqs` logs of the build 115 FTL runs (2026-10-26 device-runs). vkd3d-proton
has no hard gap on v6-v11, typed UAV loads are 18/18 everywhere, ROVs are missing everywhere. v12-v14: no device;
predicted from the 115 tree.

| arch (GPU, FTL device) | PanProbe d3d_fl: D3D12 / D3D11 highest | d3d12fl 11_0 / 11_1 / 12_0 / 12_1 / 12_2 | d3d11fl 10_0 .. 11_1 |
|---|---|---|---|
| v6 (Mali-G72, starlte) | 11_0 (SM 6.5, tiled 0, cons 0) / 11_1 | PASS / UNSUPPORTED (uav_vs, logic_op PASS) / UNSUPPORTED (typed_uav_load, binding_tier2 PASS; tiled_tier2 UNSUPPORTED) / UNSUPPORTED (rov, conservative UNSUPPORTED) / UNSUPPORTED (sm6_5 REPORTED) | all PASS |
| v7 (Mali-G52, a31) | as v6 | as v6 | all PASS |
| v9 (Mali-G78, oriole) | 11_1 (SM 6.8, tiled 0, cons 0) / 11_1 | PASS / PASS / UNSUPPORTED (tiled) / UNSUPPORTED (rov, conservative) / UNSUPPORTED | all PASS |
| v10 (Mali-G710, panther) | as v9 | as v9; but build 115 PanPlay hangs the GPU queue on panther (D3D11, D3D9 cubes and gears, `device-runs/2026-10-26-ftl-v10-panther-115-panplay`), so expect device-lost FAILs there until that is fixed | as v9, same hang risk |
| v11 (Mali-G715, shiba) | 11_1 (SM 6.8, tiled 0, cons 1) / 11_1 | PASS / PASS / UNSUPPORTED (tiled) / UNSUPPORTED (rov UNSUPPORTED, **conservative PASS**) / UNSUPPORTED | all PASS |
| v12, v13 (G720/G620, G725/G625/Immortalis-G925) | predicted 11_1 (cons 1) / 11_1 | predicted as v11 | predicted all PASS |
| v14 (Mali-G1) | predicted 11_1 (cons 2: degenerateTrianglesRasterized) / 11_1 | predicted as v11 | predicted all PASS |

TIR (D3D12) is REPORTED "not applied" everywhere (vkd3d-proton). A FAIL anywhere is a driver (or test) bug to look at.

## Reachability v6-v14

What blocks each level, per arch. Verdicts: **driver work** = possible in PanVK; **kernel** = needs a kbase uAPI the
kernel does not have; **HW** = hardware cannot. The same text is printed by PanProbe (`d3d_fl_reach`).

| level | blocking cap | v6, v7 | v9 | v10 | v11 | v12, v13 | v14 |
|---|---|---|---|---|---|---|---|
| D3D12 11_1 | 64 storage images per stage | **driver work**: Bifrost PanVK caps them at 32 (`panvk_shader.h:50` MAX_PER_STAGE_STORAGE_IMAGES, a table-copy cost choice) | met | met | met | met | met |
| 12_0 (D3D12, D3D11) | tiled resources tier 2 (sparse residency) | **kernel**: kbase has no bind/unbind-at-GPU-VA ioctl (`docs/KBASE-SPARSE-FEASIBILITY.md`, NO-GO), then driver work: PanVK sparse is `PAN_ARCH >= 10 && !kbase` only (`panvk_vX_physical_device.c:310`), no sparseResidencyAliased (panfrost/mesa#237), residencyNonResidentStrict, shaderResourceResidency/MinLod | kernel | kernel | kernel | kernel | kernel |
| 12_0 | typed UAV loads (18 formats) | met | met | met | met | met | met |
| 12_1 (D3D12, D3D11) | ROVs (fragmentShaderPixelInterlock) | **driver work, unproven**: no Mali driver has it, but the ordering primitive exists (eldest-colour slot 7: `bifrost.h:98` BIFROST_SLOT_ELDEST_COLOUR; Valhall `wait` on slots 0,1,2,6,7) | same | same | same | same | same |
| 12_1 (D3D12, D3D11) | conservative rasterization (tier 1) | **HW**: no "Conservative Rast Mode" before v11 (first in `genxml/v11.xml`); only triangle-dilation emulation | HW | HW | met (tier 1; `EXT_conservative_rasterization = PAN_ARCH >= 11`, `panvk_vX_physical_device.c:151`) | met (tier 1) | met (tier 2: `degenerateTrianglesRasterized = PAN_ARCH >= 14`, `:1363`) |
| 12_2 | SM 6.5 | met (6.5) | met (6.8) | met | met | met | met |
| 12_2 | VS/DS viewport and layer output | **driver work** (shaderOutputLayer is `PAN_ARCH >= 10`, `:463`) | driver work | met | met | met | met |
| 12_2 | conservative tier 3, tiled tier 3 | HW, kernel | HW, kernel | HW, kernel | unknown (no inner-coverage input on any arch), kernel | unknown, kernel | unknown, kernel |
| 12_2 | DXR 1.1 | **HW**: no ray tracing unit | HW | HW | **HW on Mali-G715/G615** (Pixel 8 is Mali-G715); Immortalis-G715 has an RT unit, then very large driver work (no VK_KHR_ray_tracing_pipeline/ray_query in PanVK) | Immortalis-G720/G925 only | Mali-G1-Ultra only |
| 12_2 | VRS tier 2 | **HW** | HW | HW | **driver work**: VRS hardware since Mali-G715/G615; PanVK has no VK_KHR_fragment_shading_rate | driver work | driver work |
| 12_2 | mesh shaders | **HW** natively (no mesh/task stage known on v6-v14); compute emulation = very large driver work | same | same | same | same | same |
| 12_2 | sampler feedback | **driver work**: shaderImageInt64Atomics | same | same | same | same | same |

So: D3D12 11_1 is reachable on Bifrost with driver work; 12_0 is blocked on every Mali by the kbase kernel (sparse);
12_1 additionally needs ROVs (driver work, unproven) and, on v6-v10, conservative rasterization that the hardware does
not have; 12_2 needs ray tracing hardware (Immortalis only) plus very large driver work, and mesh shaders that no Mali
has. D3D11 tops out at 11_1 for the same 12_0 and 12_1 reasons.

Sources: vkd3d-proton 2230755 `libs/vkd3d/device.c`, `libs/vkd3d/state.c`; DXVK 3.1.1 `src/d3d11/d3d11_device.cpp`;
Mesa 115 tree `/home/flux/src/mesa-115-rel/src/panfrost` (files above); `docs/KBASE-SPARSE-FEASIBILITY.md`;
`docs/vkd3d-vulkan-requirements.md`; Arm Mali-G715/G615 and Immortalis-G715 launch (VRS on G715/G615, ray tracing only on
Immortalis): https://www.computerbase.de/2022-06/arm-immortalis-g715-mali-g715-g615/,
https://www.cnx-software.com/2022/06/29/arm-immortalis-g715-gpu-supports-hardware-based-ray-tracing/.

## Firebase (not run)

One device per run, under the lock (scenario 7: 5 passes, about 10 min; 8: 6 passes, about 12 min; 9: 2 passes, about
5 min; the first pass on a fresh device also installs the components):

```sh
flock /home/flux/build-cache/firebase.lock gcloud firebase test android run --type game-loop \
  --app /home/flux/build-out/apks/panplay-115fl.apk --scenario-numbers 7 \
  --device model=shiba,version=34 --timeout 30m --results-dir panvk-v11-shiba-115fl \
  --directories-to-pull /sdcard/Android/data/dev.zenithblue.panvklauncher/files/gameloop
```

Then `--scenario-numbers 8`, or `9` for the quick check. Devices: starlte/29 (v6), a31/31 (v7), oriole/33 (v9),
panther/33 (v10), shiba/34 (v11). PanProbe: the usual run of `/home/flux/build-out/apks/panprobe-115fl.apk`
(`scripts/firebase-panprobe.sh`); the new section is `d3d_fl` in its results.

## APKs

| APK | sha256 |
|---|---|
| `/home/flux/build-out/apks/panprobe-115fl.apk` (debug-signed, PanProbe 1.3.0 vc 17, 44452608 bytes) | `b26069c1dda3c891033abba954b242ffea6e91976985f4614a0d36f939dd4373` |
| `/home/flux/build-out/apks/panplay-115fl.apk` (debug-signed, PanPlay 1.3.0 vc 42, 414775760 bytes) | `8bc6a66f8ed6a87517e596416d29ed4891be83351c77da01015856379718349e` |
| `/home/flux/build-out/release-115/PanProbe-1.3.0-unsigned.apk` (release, unsigned, 36599870 bytes) | `03e4b8f48e89ff55af4dc3512dd00b93e0440f1db4e0803f0863acc14121ad3c` |
| `/home/flux/build-out/release-115/PanPlay-1.3.0-unsigned.apk` (release, unsigned, 394311736 bytes) | `c443dbf97a6c107a3983f3bd0ed99ffb65adc7bc6fe9977738ecca5bcb2637e3` |

All four embed the build 115 driver (`73709644...`); PanPlay has all components and the four `d3d1*fl-*.exe`. Built
with `assembleDebug assembleRelease --offline --no-daemon` (`-PpanvkSoRc5=` / `-PpanvkSo=` the 115 .so) under
`build.lock`; the release builds skip the lint-vital tasks (`-x lintVitalAnalyzeRelease -x lintVitalReportRelease
-x lintVitalRelease`: the lint jar is not in the offline cache; they only check, the APK is the same). Both release
APKs pass `zipalign -c -p 4`; `release-115/SHA256SUMS` lists them. PanPlay JVM unit tests 38/38 (new
`featureLevelPatterns`). Signing: `docs/SIGNING.md`, `scripts/sign-release.sh`.
