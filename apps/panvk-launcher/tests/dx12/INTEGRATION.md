# PanPlay D3D12 test: integration notes

Staged 2026-10-26. Nothing here is in PanPlay yet and nothing has run on a device; the device behaviour
described below is expected, not measured. Selection and feasibility notes are in the repo at
`worklogs/g615-dxvk/dx12-test-selection-2026-10-26.md`.

| Staging dir | Test | Role | License |
|---|---|---|---|
| `d3d12-multithreading/` | Microsoft D3D12 Multithreading sample (SquidRoom scene), 2021 revision | primary | MIT (Microsoft); DirectXMath MIT |
| `vkd3d-gears/` | vkd3d-proton `demos/gears` (glxgears port) | fallback and quick smoke | LGPL-2.1-or-later; glxgears parts MIT |

Each directory has `SOURCES.txt` (upstream URLs, commits, git blob ids, tool tarball sha256s, build
steps, modifications and file sha256s), the license texts, and `source/` (patch, harness header, build
script, shaders and the reference screenshot).

## 1. What the tests render

- **d3d12-multithreading** is the textured "SquidRoom" level from the Microsoft sample:
  - 74 textures, 72 of them BC1_UNORM, so PanVK's BC emulation is exercised.
  - Normal mapping, 3 animated spot lights each with a shadow map, and 2x2 PCF.
  - 3 worker threads record the shadow and scene command lists in parallel, with 3 frame resources.
  - Device: FL 11_0. Root signature 1.1, falling back to 1.0. VS/PS are SM 5.0 DXBC.
  - Rendering is 1280x720 windowed; the window title shows CPU ms per frame.
  - Upstream Windows screenshot for comparison: `d3d12-multithreading/source/reference-windows.png`.
- **vkd3d-gears** draws 3 instanced gears with 2 PSOs, using Microsoft-fxc DXBC from the vkd3d-proton tree.
  - Device: FL 11_0. Rendering is 1280x720; swapchain of 3 buffers with a frame-latency waitable object.
  - It has no data files and starts in well under a second on a PC, so it is the cheap smoke test for
    device, queue, PSO and swapchain.

## 2. Files to bundle

Copy the files flat into `apps/panvk-launcher/app/src/main/assets/builtin/`, with no subdirectory.
`BuiltinTests.extract()` (Shortcuts.kt:446-457) calls `assets.open("builtin/$n")` for every name in the
listing. A directory makes that call throw, and the `catch` sits outside the loop, so every file listed
after it would not be extracted either.

| File | Size | sha256 |
|---|---|---|
| `d3d12-multithreading-x86_64.exe` | 432640 | see `d3d12-multithreading/SOURCES.txt` |
| `d3d12-multithreading-arm64ec.exe` | 450048 | " |
| `SquidRoom.bin` | 43020292 (about 23.2 MB deflated) | `02c3d5421b4d4714971bd34e5416e9de619f993510da4c66192b15653858e0aa` |
| `shaders_VSMain.cso` | 4596 | `8226faa84def7a71dbabc1ce37aca9d64205dbe181aef62c68521d5e5a069128` |
| `shaders_PSMain.cso` | 7332 | `e1e9f4abf97683e51810fd87c7b786d3e70585053d698865c54e3a51ff4c4765` |
| `vkd3d-gears-x86_64.exe` | 37888 | see `vkd3d-gears/SOURCES.txt` |
| `vkd3d-gears-arm64ec.exe` | 60416 | " |

- The Multithreading exe finds `SquidRoom.bin` and the two `.cso` files through `GetModuleFileName`,
  so they must sit next to the exe. One copy serves both arches.
- APK size grows by about 24 MB, nearly all of it `SquidRoom.bin`.
- `extract()` copies every asset again after each app update, so the 43 MB file is rewritten once per update.
  To ship a smaller APK, bundle only gears and offer the SquidRoom test as a download.
- There is no i686 build, because D3D12 titles are 64-bit. The build scripts accept `i686` as one more arch argument.
- vkd3d-proton writes `vkd3d-proton.cache` (its pipeline cache) to the working directory. PanPlay uses the exe
  directory (Containers.kt:783), which is `files/games/builtin`. Delete that file for cold-start measurements,
  or set `VKD3D_SHADER_CACHE_PATH`. The Mesa disk cache in `files/mesa_shader_cache` also stays warm.

Commit the patches, `PanPlayHarness.h`, the shims and the build scripts to the repo during integration, for
example next to `tests/dxcube.c`. That covers the gears LGPL source obligation and makes the exes
reproducible from the repo.

## 3. Shortcut ids

`BuiltinTests.list()` makes `builtin-$api-$file` from `$exe-$file.exe`, where `$file` is one of i686, x86_64 or arm64ec:

| APIS entry (api, label, exe prefix, arg) | Ids | arch field |
|---|---|---|
| `("d3d12", "D3D12", "d3d12-multithreading", "")` | `builtin-d3d12-x86_64`, `builtin-d3d12-arm64ec` | x86_64 (runs under FEX), arm64ec (native) |
| `("d3d12gears", "D3D12 gears", "vkd3d-gears", "")` | `builtin-d3d12gears-x86_64`, `builtin-d3d12gears-arm64ec` | same |

`mapNotNull` skips the missing i686 files on its own. The display name is currently hard-coded as
`"Cube · $label · $short"`, so these need their own names, for example `D3D12 · SquidRoom · x64` and
`D3D12 · Gears · ARM64EC`. Both can reuse the `res:ic_directx_cube` icon.

## 4. Running

```
wine <files/games/builtin>/d3d12-multithreading-arm64ec.exe [-secs N] [-sync N]
wine <files/games/builtin>/vkd3d-gears-x86_64.exe [-secs N] [-sync N]
```

Each option can be set in the environment or as an argument; the argument wins.

| Option | Default | Meaning |
|---|---|---|
| `DX12TEST_SECS` / `-secs N` | 30 | Seconds to render, counted from the first Present. Total run time is startup plus N. 0 means run until the window is closed. |
| `DX12TEST_SYNC` / `-sync N` | 0 | Sync interval passed to Present, 0-4. With 0, vkd3d-proton switches to an unlocked present mode (IMMEDIATE or MAILBOX) if the surface offers one, otherwise it uses FIFO. |

- **Stopping.** After N seconds the test prints the PASS line and posts WM_QUIT. Normal teardown follows
  (GPU wait, object release) and the exit code is 0.
- **Closing early.** Closing the window also exits with 0, but without a PASS line. In gears, Escape quits
  too and `a` toggles the animation. In Multithreading, the arrow keys move the camera and Space toggles
  the light animation.
- **Arguments.** Unknown arguments are ignored. `-warp` (or `/warp`) sends the Multithreading sample down its
  WARP path. DXVK's dxgi has no WARP and returns the hardware adapter with a log warning, and the window title
  gains "(WARP)". Do not pass it. Upstream treated an empty argument or any prefix of `-warp` (such as `-`) as
  `-warp`; this build only accepts the exact words.
- **Startup.** For Multithreading, startup covers reading the 43 MB asset, uploading 74 textures and
  creating 2 PSOs. It will be slower under FEX (x86_64) and on a cold pipeline cache. The `startup=` field
  reports it.

## 5. Output (fps, pass, fail)

PanPlay starts `wine <exe> <args>` with `redirectErrorStream(true)` and writes every line to
`files/logs/run-<timestamp>.log` and the in-app console (Containers.kt:536-596). The tests write unbuffered
to stdout:

```
DX12: start test=d3d12-multithreading secs=30 sync=0 1280x720
DX12: first frame startup=<s>                  seconds from process start to the first Present
DX12: Present frame=1 hr=0x00000000            frames 1-10, then every 60th
DX12: fps=<f> frames=<n> t=<s>                 about once a second; t counts from the first frame
DX12: PASS test=<name> frames=<n> secs=<s> fps=<avg> startup=<s>     exit code 0
DX12: FAIL <what> hr=0x<hresult>               exit code 1
```

- `fps=` in the PASS line is `(frames - 1) / secs`, measured from the first frame. Asset loading and PSO
  compilation are therefore excluded and reported as `startup=`.
- FAIL is printed when:
  - Multithreading throws on any failing HRESULT. Examples: D3D12CreateDevice, CreateGraphicsPipelineState,
    Present, or a missing `SquidRoom.bin` or `.cso`, which shows as `FAIL std::exception`.
  - gears fails one of its `assert()` checks, which prints `FAIL gears.c:<line> <expr>`.
- If Multithreading gets DXGI_ERROR_DEVICE_REMOVED, the sample's own recovery path runs (RestoreD3DResources).
- **There is no HUD.** PanPlay sets `DXVK_HUD=full` whenever DXVK is on, but that has no effect here. DXVK's
  dxgi hands a D3D12 swapchain to vkd3d-proton (`IDXGIVkSwapChainFactory`), and vkd3d-proton has no HUD.
  Use the `DX12:` lines, or a driver-side fps line such as PANVK_FPS_LOG_MS (csf-v11 patch 233, in progress),
  which counts presents whatever the API.

## 6. Required DLLs

- **d3d12.dll and d3d12core.dll** come from vkd3d-proton 22307558, the VKD3D component. Its ARM64EC build
  serves both the arm64ec exe and the x86_64 exe under FEX. PanPlay copies them into the prefix when
  `vkd3d_enabled` is on, which is the default (Containers.kt:1172, 1233).
- **dxgi.dll** comes from DXVK 3.1.1. The Multithreading sample calls `DXGIDeclareAdapterRemovalSupport`
  if the export exists; DXVK's dxgi has it. The exe looks it up with GetProcAddress, so it does not depend on it.
- **WINEDLLOVERRIDES** must contain `dxgi=n,b` and `d3d12,d3d12core=n,b`. PanPlay's `defaultDllOverrides`
  produces both when DXVK and VKD3D are enabled (Containers.kt:1155-1158).
  - With DXVK off, Wine's builtin dxgi and d3d12 load instead. That path uses Wine's vkd3d and is not what
    these tests are meant to cover.
  - The adb harness script `dxenv-synchronized.sh` hard-codes overrides without d3d12 (see section 8).
- **Nothing else.** No d3dcompiler_47, DXC, d3dx12 DLL, vcruntime or msvcp. Imports are limited to
  KERNEL32, USER32, SHELL32, GDI32 (gears only), `api-ms-win-crt-*` (Wine's ucrtbase), d3d12 and dxgi.
  libc++ and libunwind are statically linked.

## 7. Environment

PanPlay already sets:

- `VKD3D_LOG_FILE=Z:<gfxLogs>/vkd3d.log`. SessionLogs.collect later moves it into the session folder.
- `VKD3D_FRAME_RATE=<cap>` when the shortcut has an fps limit. Leave this unset for fps runs.

Recommended for the first device runs, set in the shortcut env or the harness:

| Variable | Value | Why |
|---|---|---|
| `VKD3D_DEBUG` | `warn` | The default level is `fixme` (err + info + fixme). `warn` adds WARN lines, including `Feature level %#x is not supported.` when device creation fails. `trace` is very slow. |
| `VKD3D_SHADER_DEBUG` | `warn` | DXBC to SPIR-V translation problems. |
| `VKD3D_CONFIG` | empty for normal runs | Toggles for diagnosis: `single_queue` (rules out cross-queue sync), `skip_driver_workarounds`, `force_host_cached`, `no_upload_hvv`, `log_memory_budget`. `breadcrumbs` and `fault` need vendor extensions PanVK does not expose. |
| `VKD3D_FEATURE_LEVEL` | do not set | Both tests ask for 11_0, which every arch reports. Forcing 12_x only misreports caps to apps. |
| `VKD3D_SWAPCHAIN_PRESENT_MODE` | `IMMEDIATE`, `MAILBOX`, `FIFO`, `FIFO_RELAXED` or `FIFO_LATEST_READY` (uppercase) | Pins the present mode, which helps separate present-path problems from rendering problems. |
| `VKD3D_SWAPCHAIN_LATENCY_FRAMES`, `VKD3D_SWAPCHAIN_IMAGES` | numbers | Latency and queue-depth experiments. |
| `VKD3D_QUEUE_PROFILE`, `VKD3D_TIMESTAMP_PROFILE` | `Z:` file paths | Per-submission CPU timeline and GPU timestamp profiles. |
| `VKD3D_SHADER_CACHE_PATH` | a directory, or leave unset | Moves `vkd3d-proton.cache` out of `files/games/builtin`. |
| `DX12TEST_SECS`, `DX12TEST_SYNC` | see section 4 | Test length and vsync. |

## 8. Harness scenarios

### 8a. Firebase game loop (`assets/gameloop.json` / `GameLoop.kt`, in progress)

The tasks fit the existing cube task schema:

```json
"d3d12gears": { "title": "D3D12 gears (ARM64EC)", "shortcut": "builtin-d3d12gears-arm64ec",
  "secsEnv": "DX12TEST_SECS", "firstFramePattern": "^DX12: Present frame=",
  "okPattern": "^DX12: PASS", "failPattern": "^DX12: FAIL" },
"d3d12": { "title": "D3D12 SquidRoom (ARM64EC)", "shortcut": "builtin-d3d12-arm64ec",
  "secsEnv": "DX12TEST_SECS", "firstFramePattern": "^DX12: Present frame=",
  "okPattern": "^DX12: PASS", "failPattern": "^DX12: FAIL" },
"d3d12x64": { "title": "D3D12 SquidRoom (x86_64, FEX)", "shortcut": "builtin-d3d12-x86_64",
  "secsEnv": "DX12TEST_SECS", "firstFramePattern": "^DX12: Present frame=",
  "okPattern": "^DX12: PASS", "failPattern": "^DX12: FAIL" }
```

Add a scenario `"5": { "name": "d3d12", "tasks": ["d3d12gears", "d3d12", "d3d12x64"] }`. Gears runs first:
if it fails, the problem is device or swapchain creation, not the scene.

- `DX12TEST_SECS` counts from the first frame, the same as `secsEnv` is meant to
  (warmupSec + measureSec + tailSec after the first frame).
- The game loop's `startTimeoutSec` (300) covers the Multithreading startup.
- Read the average fps from the PASS line, or from the per-second `DX12: fps=` lines inside the measured window.

### 8b. adb 3d-suite (`apps/panvk-launcher/tests/3d-suite.py`)

One case per arch, next to the cube cases:

```python
DX12 = pathlib.Path(os.environ.get('DX12_DIR', '/home/flux/build-cache/dx12-test/d3d12-multithreading'))
yield dict(test='dx12mt', arch=arch, api='d3d12', exe=DX12 / ('d3d12-multithreading-%s.exe' % arch),
           data=[DX12 / 'SquidRoom.bin', DX12 / 'shaders_VSMain.cso', DX12 / 'shaders_PSMain.cso'],
           marker='DX12: Present frame=5', env='DX12TEST_SECS=%d' % CUBE_SECS, tmo=CUBE_SECS + 120)
```

Differences from the cube cases:

1. Push `data` into `drive_c` next to `3dt.exe`; the exe loads the files from its own directory.
2. `dxenv-synchronized.sh` exports `WINEDLLOVERRIDES="mscoree,mshtml=d;d3d8,d3d9,d3d10core,d3d11,dxgi=n,b"`,
   with no d3d12. A D3D12 case needs a variant that appends `;d3d12,d3d12core=n,b`, with the VKD3D
   component's DLLs in `system32`. Alternatively, launch the PanPlay shortcut itself.
3. The first-frame wait in the polling loop is capped at 50 s, which is too short for a cold Multithreading
   start under FEX. Allow at least 120 s.
4. Pass when `'DX12: PASS' in lg and 'exit=0' in lg`. For fps, use
   `re.search(r'DX12: PASS test=\S+ frames=(\d+) secs=([\d.]+) fps=([\d.]+) startup=([\d.]+)', lg)`.
   The existing `frames=(\d+)` regex would match the first per-second line.
5. Pixel check: the scene is never black, so the existing `nonblack >= 0.05` rule applies. Compare the
   screenshot by eye with `source/reference-windows.png`. The camera starts at the same pose; the lighting
   differs because the lights animate.

### 8c. Expected outcome per arch

These expectations come from the PanProbe `compliance-vkd3d_reqs` RC5 113/114 runs, not from running these exes.

- vkd3d-proton reports 0 hard gaps on v6, v7, v9, v10 and v11.
- The maximum feature level is 11_1 on v9, v10 and v11, and 11_0 on v6 and v7. Both tests request 11_0,
  so device creation should succeed everywhere.

Still unproven on a device:

- D3D12 present through DXVK dxgi and the vkd3d-proton swapchain on the PanVK X11 WSI in PanPlay.
- BC1 sampling through the BC emulation.
- Recording command lists from several threads on one queue.
- vkd3d-compiler DXBC in Multithreading versus fxc DXBC in gears; gears is the control.

## 9. Proposed components-NOTICE.txt entries

```
D3D12 Multithreading sample (built-in test: assets/builtin/d3d12-multithreading-*.exe, SquidRoom.bin, shaders_*.cso)
  License: MIT, Copyright (c) Microsoft Corporation. Source: https://github.com/microsoft/DirectX-Graphics-Samples
  commit 0f86e460bec17ed5231549748599a91ca5093132 (Samples/Desktop/D3D12Multithreading), with DirectXMath
  (MIT, https://github.com/microsoft/DirectXMath tag jun2026) and the PanPlay harness patch <repo path>.
  Shaders precompiled with vkd3d-compiler 1.19. Built with llvm-mingw 20260922.
  <MIT license text from LICENSE-DirectX-Graphics-Samples.txt and LICENSE-DirectXMath.txt>

vkd3d-proton gears demo (built-in test: assets/builtin/vkd3d-gears-*.exe)
  License: LGPL-2.1-or-later, Copyright 2016 Henri Verbeet for CodeWeavers; glxgears-derived parts MIT,
  Copyright (C) 1999-2001 Brian Paul. Source: https://github.com/HansKristian-Work/vkd3d-proton
  commit 2230755878b01993b4b82d0ac0c0624f3ab4d333 (demos/), plus the PanPlay patch <repo path>.
  Built with llvm-mingw 20260922.
  <LGPL-2.1 text, or a pointer to the copy already shipped for Wine/vkd3d-proton; MIT text from LICENSE-glxgears-MIT.txt>
```

## 10. Rebuilding

The scripts are in `/home/flux/build-cache/dx12-test/scripts/`. Run them under `flock /home/flux/build-cache/build.lock`.

- `build-vkd3d-compiler.sh`: needs `VULKAN_INC=<dir containing vulkan/vulkan.h>`, for example
  `/home/flux/src/mesa-rc5g/include`.
- `build-d3d12multithreading.sh <patched src> <DirectXMath/Inc> <out> x86_64 arm64ec`
- `build-vkd3d-gears.sh <patched demos> <out> x86_64 arm64ec`

The patched trees are in `build/mt/src` and `build/gears/src`, next to the pristine `orig/` copies. The
patches are `scripts/patches/*.patch` (`diff -ru orig src`; apply with `patch -p1 -d <dir>`).
