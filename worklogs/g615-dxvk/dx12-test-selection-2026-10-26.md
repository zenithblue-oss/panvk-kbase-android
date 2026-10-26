# D3D12 test for PanPlay: selection, feasibility and staging (2026-10-26)

Goal: a Direct3D 12 3D test that PanPlay can run next to its DXVK cubes. It runs on vkd3d-proton 22307558
(the VKD3D component), DXVK 3.1.1 dxgi and Proton 11 arm64ec, with FEX for x86_64.

This round covered research, building and staging only. Nothing ran on a device or on Firebase, and PanPlay
(`apps/panvk-launcher`) was not changed. The staged files and integration notes are outside the repo, in
`/home/flux/build-cache/dx12-test/`.

**Result**

- **Primary test:** the Microsoft D3D12 Multithreading sample (SquidRoom scene), MIT.
- **Fallback test:** vkd3d-proton's `demos/gears`, LGPL-2.1-or-later.
- Both are built for x86_64 and arm64ec with llvm-mingw.
- Both need only FL 11_0 and SM 5.0 DXBC, and neither needs d3dcompiler at runtime.
- Both print `DX12:` stdout lines (per-second fps, PASS or FAIL) and exit by themselves.
- Per PanProbe RC5 113/114, vkd3d-proton device creation has **no blocking gap** on any arch (v6 to v11).

## 1. Candidates

The criteria were:

- a real 3D scene;
- bundling allowed in an APK;
- runs standalone;
- FL 11_x with SM 5.x DXBC, and no ray tracing, mesh shaders or sampler feedback;
- prints fps or exits after a fixed time.

The web search found no redistributable prebuilt D3D12 benchmark exe. Benchmarks such as 3DMark are proprietary,
and open demos ship as source, so every candidate had to be built from source.

| Candidate | License | Scene | D3D12 needs | Verdict |
|---|---|---|---|---|
| DirectX-Graphics-Samples D3D12Multithreading at `0f86e460` (2021-07-26) | MIT, code and the SquidRoom asset | Textured, normal-mapped room; 74 textures (72 BC1); 3 animated shadowing lights; 3 recording threads | FL 11_0, root signature 1.1 with 1.0 fallback, SM 5.0 (D3DCompile at runtime upstream, precompiled here) | **Chosen** |
| vkd3d-proton `demos/gears` at `22307558` | LGPL-2.1-or-later; glxgears parts MIT | 3 gears, 2 PSOs, instanced | FL 11_0; fxc DXBC already in the tree | **Fallback** (tiny, no assets) |
| DirectX-Graphics-Samples master | MIT | Same samples | Agility SDK (D3D12Core redistributable), DXC SM 6.x DXIL, enhanced barriers | Rejected; `0f86e460` is the last revision before that port |
| Wine vkd3d 1.19 demos (teapot, gears) | LGPL-2.1-or-later | Teapot, gears | Compile HLSL at runtime through D3DCompile; teapot uses tessellation and a GS | Not chosen; needs a runtime compiler |
| bgfx examples | BSD-2-Clause | Many | D3D12 backend now DXIL; GENie build; asset provenance mixed | Rejected |
| Godot 4 D3D12 renderer | MIT | Engine | FL 12_0-class hardware, DXIL; about 100 MB | Rejected |
| Diligent Engine samples | Apache-2.0 | Many | MSVC/CMake-centric, no prebuilt binaries | Rejected; too much build work |
| Wicked Engine | MIT | Engine | SM 6+ DXIL | Rejected |
| Xbox-ATG-Samples, DirectXTK12 | MIT | Many | Shaders built by the Windows-only `CompileShaders.cmd` | Rejected |

**Why D3D12Multithreading**

- It is a real scene (reference screenshot: `d3d12-multithreading/source/reference-windows.png`).
- It is MIT, including the asset.
- It has the lowest feature needs among the scene candidates.
- It covers the paths most likely to break on PanVK under D3D12:
  - BC1 textures through the BC emulation;
  - depth (shadow map) render-to-texture with `R32_TYPELESS` aliasing;
  - several command lists recorded in parallel and submitted on one queue.

**Why gears as the fallback**

- It is 38 to 60 KB with no data files.
- Its DXBC comes from real fxc, so it is the control for the vkd3d-compiler DXBC used in the primary.
- It isolates device, PSO and swapchain problems from scene problems.

## 2. Build (summary; full provenance in each `SOURCES.txt`)

- **Toolchain:** llvm-mingw 20260922 UCRT aarch64. The tarball's sha256
  `07d21263c56bfe9a713db6fdb3f7434bf4c121a005e40397d3b4c0170fb06769` matches the GitHub release asset digest.
- **Shaders:** Wine vkd3d 1.19's `vkd3d-compiler`, built from the release tarball (sha256
  `034613605baab8ba84674f8d272cf22b5e86bc6bc03fc5728ef9bce07308baa6`; the .sign was not checked because gpg is
  not installed), compiles `shaders.hlsl` to `vs_5_0`/`ps_5_0` DXBC. The output is bit-identical when rebuilt.
- **Reproducibility:** a second build of the Multithreading exes from the final patch was also bit-identical.
- **Multithreading patch:** about 200 lines, and the rendering code is unchanged. It:
  - loads the `.cso` files instead of calling D3DCompileFromFile;
  - adds Present sync and quit hooks;
  - adds mingw-w64 build fixes (`FileHandle`, `sprintf_s`, `L#x`, `DXGIDeclareAdapterRemovalSupport` through
    GetProcAddress);
  - makes `-warp` match exactly;
  - catches exceptions and prints them as `DX12: FAIL`.
- **Gears patch:** about 80 lines. It adds the same hooks, turns `assert()` into a FAIL line plus exit, and sets
  the window to 1280x720 (was 300x300).
- **Imports:** KERNEL32, USER32, SHELL32, GDI32 (gears only), `api-ms-win-crt-*`, `d3d12.dll` and `dxgi.dll`.
  There is no d3dcompiler, vcruntime or libc++ DLL; libc++ and libunwind are statically linked.
- **Harness hooks** (`PanPlayHarness.h`, MIT):
  - lines: `DX12: start`, `DX12: first frame startup=`, `DX12: Present frame=`, `DX12: fps=` (per second),
    `DX12: PASS test= frames= secs= fps= startup=` and `DX12: FAIL <what> hr=`;
  - options: `DX12TEST_SECS` (default 30, counted from the first frame) and `DX12TEST_SYNC` (default 0),
    or `-secs N -sync N`.

## 3. Staged files (`/home/flux/build-cache/dx12-test/`)

| File | sha256 |
|---|---|
| `d3d12-multithreading/d3d12-multithreading-x86_64.exe` | `1c79bae4bd3148d5e908d00873b1346154cdbf237b6af33d60a0f87e8441c430` |
| `d3d12-multithreading/d3d12-multithreading-arm64ec.exe` | `d4dde5b49e6200493933886157809ea589e60a7bd617f89a80201fb42cf402d9` |
| `d3d12-multithreading/SquidRoom.bin` (43 MB; upstream blob `91a24636`) | `02c3d5421b4d4714971bd34e5416e9de619f993510da4c66192b15653858e0aa` |
| `d3d12-multithreading/shaders_VSMain.cso` | `8226faa84def7a71dbabc1ce37aca9d64205dbe181aef62c68521d5e5a069128` |
| `d3d12-multithreading/shaders_PSMain.cso` | `e1e9f4abf97683e51810fd87c7b786d3e70585053d698865c54e3a51ff4c4765` |
| `vkd3d-gears/vkd3d-gears-x86_64.exe` | `3e6f5a7c50c671ffeca824255522a47e1879a96651aad49409aeb0e87ee2822e` |
| `vkd3d-gears/vkd3d-gears-arm64ec.exe` | `cd2a23189c24fb716f61a7745cca4a197b0efb32856abe5b6f77e611f8d8d3df` |

Each test directory also holds:

- the license files and `SOURCES.txt`;
- `source/`, with the patch, harness header, shims, build script and shaders. For gears this is the LGPL
  corresponding source.

`INTEGRATION.md` lists:

- the files to bundle: flat in `assets/builtin`, because a subdirectory breaks `BuiltinTests.extract`;
- shortcut ids `builtin-d3d12-{x86_64,arm64ec}` and `builtin-d3d12gears-{x86_64,arm64ec}`;
- environment variables, required DLLs and overrides;
- proposed `components-NOTICE.txt` entries;
- harness scenarios: `gameloop.json` tasks with `^DX12: Present frame=`, `^DX12: PASS` and `^DX12: FAIL`
  patterns, plus a 3d-suite case.

APK cost is about 24 MB, nearly all of it `SquidRoom.bin` (about 23.2 MB deflated).

## 4. Feasibility: vkd3d-proton 22307558 against PanVK (rc5g-114)

### 4.1 Hard device-creation checks (vkd3d-proton)

Missing any of these makes `D3D12CreateDevice` fail.

| Check | vkd3d-proton | PanVK status |
|---|---|---|
| Vulkan 1.3 instance and device | `device.c:675`, `:2745` | ok, all arches |
| Vertex attribute divisor, including zero divisor | `device.c:2495-2500` | ok |
| Transform feedback queries | `device.c:2502-2506` | ok |
| Single-texel alignment, storage and uniform texel buffers | `device.c:2508-2520` | ok |
| samplerMirrorClampToEdge | `device.c:2633-2636` | ok |
| robustBufferAccess2 + robustImageAccess2 | `device.c:2639-2643` | ok. v6/v7 RIA2 comes through `PANVK_V7_RIA2` (default on, `panvk_vX_physical_device.c:691`) |
| nullDescriptor | `device.c:2646-2649` | ok (`:693`) |
| shaderDrawParameters, push descriptors, maintenance5/6 | `device.c:2655-2671` | ok |
| Descriptor indexing: storage texel and storage image non-uniform indexing, variable count | `state.c:8351-8359` | ok |
| 1M per-stage update-after-bind sampled images, storage images and SSBOs | `state.c:8208-8213` | ok, all arches |
| Requested minimum FL <= max FL | `device.c:10942` | ok for FL 11_0 requests |

Source: PanProbe `compliance-vkd3d_reqs`, a Vulkan-level re-check of these vkd3d-proton conditions, not a
vkd3d-proton run. Each log reports `proton_device_create ok (0 hard gaps)`. Logs are under
`worklogs/g615-dxvk/device-runs/2026-10-25-ftl-*/raw/*/…/compliance-vkd3d_reqs.log`:

| Arch | Device | Build | Max FL | Max SM | Binding / tiled / conservative tier |
|---|---|---|---|---|---|
| v11 | shiba (G715) | rc5u-114 | 11_1 | 6_8 | 3 / 0 / 1 |
| v10 | panther (G710) | rc5u-114 | 11_1 | 6_8 | 3 / 0 / 0 |
| v9 | oriole (G78) | rc5u-113 | 11_1 | 6_8 | 3 / 0 / 0 |
| v7 | a31 (G52) | rc5u-113 | 11_0 | 6_5 | 3 / 0 / 0 |
| v6 | starlte (G72) | rc5u-113 | 11_0 | 6_5 | 3 / 0 / 0 |

Feature levels follow vkd3d-proton's ladder (`device.c:9644-9683`):

- **11_1** needs logicOp, vertexPipelineStoresAndAtomics, and 64 SSBOs and 64 storage images per stage.
- **12_0** needs tiled resources tier 2 or higher, binding tier 2 or higher, and typed UAV loads.
- **12_1** adds ROVs and conservative rasterization.

### 4.2 Blocking gaps for the chosen tests

**None.** Both tests request FL 11_0, which every arch reports. They use nothing beyond FL 11_0: no UAVs, no
ExecuteIndirect, no tessellation and no geometry shaders.

One trap for anyone probing feature levels: vkd3d-proton returns an existing device singleton without the FL
check (`device.c:10900-10916`). A process that keeps an FL 11_0 device alive and then "creates" at FL 12_x gets
`S_OK`. The old d3d12probe "hr=0 at 12_x" result was this artifact.

### 4.3 Non-blocking gaps

These limit which other D3D12 titles can run, not these tests. Code references are in
`src/panfrost/vulkan/panvk_vX_physical_device.c` at rc5g-114.

| Gap | Effect in vkd3d-proton | PanVK location | Arches |
|---|---|---|---|
| Sparse residency / tiled tier 2: `has_sparse` is false on kbase, `sparseResidencyAliased` false, `shaderResourceResidency`/`MinLod` false | Tiled tier 0, so **FL 12_0 is blocked**. This is the main gap for D3D12 games, many of which require 12_0. | `:313`, `:381-391`; tier logic `device.c:8938-8960` | all |
| ROVs: no `VK_EXT_fragment_shader_interlock` | FL 12_1 blocked | not implemented | all |
| Conservative rasterization | FL 12_1 blocked (v11 has tier 1, `:154`) | `:154` (`PAN_ARCH >= 11`) | v6-v10 |
| 32 storage images per stage | Capped at FL 11_0 (11_1 needs 64) | `panvk_shader.h:50` | v6, v7 |
| `VK_KHR_draw_indirect_count` | ExecuteIndirect count buffer ignored | `:67` (`PAN_ARCH >= 9`) | v6, v7 |
| SM 6.6/6.7 features: compute derivatives, int64 buffer atomics, int8, maximal reconvergence, quad control | Max SM 6.5 | various | v6, v7 |
| `shaderFloat64` | `DoublePrecisionFloatShaderOps` false | `:378` | all |
| `shaderOutputLayer` | VS RT array index unsupported (FL 12_2 gate) | `:463` (`PAN_ARCH >= 10`) | v6-v9 |
| `VK_EXT_descriptor_buffer` | Descriptor-set heap path, with slower descriptor copies | not implemented | all |
| VRS, mesh shaders, ray query | FL 12_2 and DXR blocked | not implemented | all |
| `viewportSubPixelBits = 0` | Warning only | `:1058` | all |
| `VK_EXT_calibrated_timestamps` | `GetClockCalibration` returns 0 | not exposed | v6 |

Escape hatch for apps that only probe FL 12_0: `VKD3D_FEATURE_LEVEL=12_0` (`device.c:9956-10012`). It
misreports caps (tiled resources) and is not meant for tests.

### 4.4 Risks not covered by the requirement checks

1. **First D3D12 present in PanPlay.** DXVK dxgi hands the swapchain to vkd3d-proton
   (`IDXGIVkSwapChainFactory`), which presents through the PanVK X11 software WSI. This path is shared with the
   cube's present cost (perf review C2).
2. BC1 sampling through the BC emulation. The G715 log shows "BC emulation on, lowmem".
3. Parallel command-list recording on 3 threads, plus multiple command lists per ExecuteCommandLists.
4. vkd3d-compiler DXBC instead of fxc DXBC. Gears is the fxc control.
5. FEX overhead for the x86_64 builds. The Multithreading window title shows CPU ms per frame, and
   `startup=` shows load and PSO time.

## 5. Next steps (not done here)

1. Integrate into PanPlay:
   - copy the files flat into `assets/builtin`;
   - add the `APIS` entries and names;
   - add the NOTICE entries;
   - commit the patches and scripts next to `tests/dxcube.c`.
2. Add the game-loop tasks.
3. Run gears, then the SquidRoom test, arm64ec first, on one device. Recommended first-run env:
   `VKD3D_DEBUG=warn` and `VKD3D_SHADER_DEBUG=warn`. Collect `vkd3d.log` and the run log.
