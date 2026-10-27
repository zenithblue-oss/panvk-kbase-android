# FTL a31 (Galaxy A31, Mali-G52 r1 MC2, v7) build 115: PanPlay game loop (scenarios 3, 5, 6)

- Matrix `matrix-3vebm0s2e6jih`; outcome `Passed a31-31-en-portrait --`; results `gs://test-lab-7jybm598jy36m-hdr2td6as0u1q/panvk-v7-a31-115-panplay/` (project `phone-finder-hub`).
- APK `/home/flux/build-out/apks/panplay-115.apk` (sha256 `09a9aba6...`), `dev.zenithblue.panvklauncher` 1.3.0 (vc 41), bundled driver build 115 (`73709644...`). Default window: 20 s warm-up, 60 s measured, 8 s tail.
- `gameloop-override.json`: the config override pushed with `--other-files` (D3D12 diagnostics env).
- `raw/` has everything but `video.mp4` (not committed, about 45 MB).

| scenario | task | result | fps | test line |
|---|---|---|---|---|
| 3 | d3d11 | fail (test failed: CUBE: FAIL Present hr=0x80004005) | - | `CUBE: FAIL Present hr=0x80004005` |
| 3 | d3d9 | fail (test failed: CUBE: FAIL VB hr=0x8876086c) | - | `CUBE: FAIL VB hr=0x8876086c` |
| 5 | d3d12 | ok (0.9 fps, see Result) | 0.9 | `DX12: PASS test=d3d12-multithreading frames=82 secs=89.0 fps=0.9 startup=7.3` |
| 6 | d3d12gears | ok by the harness, **picture wrong** (see Result) | 101.9 | `DX12: PASS test=vkd3d-gears frames=8972 secs=88.0 fps=101.9 startup=2.6` |

## Result

**These are the first PanPlay (DXVK and vkd3d-proton) runs on v7.** The 107-115 a31 runs are PanProbe only, so there is
no baseline. In both DXVK cubes the device comes up and then the first frame fails. Both D3D12 tests create their device,
bring up a swapchain and print PASS, but neither works fully: SquidRoom runs at 0.9 fps and gears draws the wrong picture.
The FTL outcome Passed only means the app exited normally. The run used project `phone-finder-hub`.

Galaxy A31 (SM-A315N, MT6768, Mali-G52 r1 MC2 `0x74021000` variant 0x2), Android 12 (API 31), kernel 4.14.186, kbase
JM uAPI 11.27. Driver `Mesa 26.3.0-devel (git-9dde9e7a18)` (`.so` sha256 `73709644...`).

**Diagnostics:** `gameloop-override.json` was pushed with `--other-files`, and every pass's `configSource` shows that PanPlay
read it (`/storage/emulated/0/Android/data/dev.zenithblue.panvklauncher/files/gameloop.json sha256=181f64ce...`). Both D3D12
passes ran with `VKD3D_DEBUG=warn`, `VKD3D_SHADER_DEBUG=warn` and `WINEDEBUG=+seh,+loaddll`. Each pass's `session/` has
`wine-run.log`, `vkd3d.log`, the DXVK logs and the driver log `mesa-panvk.txt`.

| test | result | fps | first error and cause |
|---|---|---|---|
| D3D11 cube (DXVK, ARM64EC) | **FAIL** `CUBE: FAIL Present hr=0x80004005` | - | Device created at FL 11_0 (`CUBE: d3d11 create hr=0x00000000 fl=0xb000`). The first frame's job chains are killed at 03:12:11.134: `kbase: JM atom 14 failed: event=0x4 core_req=0x16`, `JM atom 15 failed: event=0x4 core_req=0x1`, `JM atom 16 failed: event=0x4 core_req=0x16` (event 0x4 = BASE_JD_EVENT_TERMINATED; core_req 0x16 = vertex/tiler chain, 0x1 = fragment). DXVK then logs `Command submission failed: VK_ERROR_DEVICE_LOST` and Present returns E_FAIL. No frame reached the screen; the X window stays white. FTL's logcat has no kernel log, so the kbase reason for the termination is not in this data |
| D3D9 cube (DXVK, i686 via WoW64) | **FAIL** `CUBE: FAIL VB hr=0x8876086c` (D3DERR_INVALIDCALL) | - | `CUBE: d3d9 CreateDevice hr=0x00000000`, then the first vertex buffer fails to map: `0024:err:vulkan:win32u_vkMapMemory2KHR vkMapMemory2EXT failed: -5` and DXVK `Failed to map Vulkan memory: VK_ERROR_MEMORY_MAP_FAILED`, `size: 4194304 bytes`. The failing call is the placed map (VK_EXT_map_memory_placed) that Wine's WoW64 uses to map device memory below 4 GB for a 32-bit app. On v7 it fails for a 4 MiB allocation |
| D3D12 SquidRoom (vkd3d-proton, ARM64EC) | `DX12: PASS test=d3d12-multithreading frames=82 secs=89.0 fps=0.9 startup=7.3` | **0.9** | The picture is correct (`d3d12-squidroom-harness.png`), but every frame takes about 1.1 s. The driver's fps log has 81 frames, avg 1098 ms, max 2226 ms. Game CPU is 4.5 % (0.36 cores), so the wait is on the GPU side. No vkd3d-proton `err:`, no driver warning or error, no `event=0x` line |
| D3D12 gears (vkd3d-proton, ARM64EC) | `DX12: PASS test=vkd3d-gears frames=8972 secs=88.0 fps=101.9 startup=2.6` | **101.8** (driver) | **Wrong picture** (`d3d12gears-harness.png`). Only the red gear is drawn, twice, offset, and not in its place; the green and blue gears are missing. oriole and shiba draw all three. Nothing is logged: no vkd3d-proton `err:`, no driver error, no fault. The test's PASS does not check pixels |

**D3D12 bring-up on v7 (both tests):**
- `D3D12CreateDevice(..., D3D_FEATURE_LEVEL_11_0)` succeeds. vkd3d-proton logs `build: 2230755878b0199`, with no `Enabling
  support for SM 6.6` line, so shader model 6.5. Then `dxgi_vk_swap_chain_init: Creating swapchain (1280 x 720),
  BufferCount = 3`.
- DXGI adapter: `Mali-G52 r1 MC2 (panvk 26.2.99)`.
- Warnings: optional `VK_KHR_surface_maintenance1` not supported, `TotalLaneCount = 256` fixme.
- `+loaddll`: `d3d12.dll`, `dxgi.dll` and `d3d12core.dll` load native and `winevulkan.dll` builtin.
- `+seh`: SquidRoom's process hits `EXCEPTION_ACCESS_VIOLATION` (read of 0x70) at `d3d12core.dll + 0x329F0`, called from
  `d3d12-multithreading-arm64ec.exe + 0x7368`. This happens after its PASS line, at teardown, with exit code 0. v9, v10 and
  v11 hit the same address (`0x6FFBDE29F0`), so it is vkd3d-proton or test teardown, not v7.
- gears has no exception.

**Which failure belongs where, from these logs:**
1. D3D11: JM atom termination on the first frame (`event=0x4`).
2. D3D9 (32-bit): placed memory map, `vkMapMemory2EXT` returning VK_ERROR_MEMORY_MAP_FAILED.
3. D3D12: no missing feature, extension or format is reported. Device creation, swapchain and presentation work. The
   problems are SquidRoom's GPU time per frame and gears' wrong draws, and neither logs an error.

**Knobs.** Every pass prints the build 115 v7 JM defaults:
- `JM warm-up off (PANVK_KBASE_JM_WARMUP=0 ...)`
- `v7 JM, KBASE_IOCTL_MEM_JIT_INIT skipped; tiler heap is a GROW_ON_GPF region`
- `Bifrost JM atom stride 56`
- `coherency mode 31 (none), shaderDeviceClock off`
- `GPU timestamp counter measured at 26 MHz, cntfrq 13000000 Hz: using 26000000 Hz (PANVK_KBASE_TS_CALIBRATE=0 = cntfrq)`
- `no usable ION system heap`
- `BC emulation on`

This run has no knob A/B and there is no earlier v7 PanPlay run. The a31 PanProbe run of this build
(`../2026-10-26-ftl-v7-a31-115/`) passed 39/39 in both `defaults` and `114-knobs`.

### D3D12 feature level (vkd3d-proton) on this device

vkd3d-proton 2230755 (PanPlay's `d3d12core.dll`) logs its max feature level only at TRACE, and the bundled release
build has TRACE compiled out (the `Max feature level` string is not in the DLL). The level below is vkd3d-proton's own
`d3d12_device_caps_init_feature_level` (libs/vkd3d/device.c at 2230755) applied to this device's Vulkan caps from the
build 115 PanProbe run (`../2026-10-26-ftl-v7-a31-115/`, `device_info.json` and `vkd3d_reqs`, whose own
vkd3d-proton model gives the same level). The test apps create their device at FL 11_0.

| | value |
|---|---|
| max feature level | **11_0** |
| max shader model | 6.5 (vkd3d.log: no `Enabling support for SM` line, so below 6.6) |
| resource binding tier | 3 (fixed at 3 in this vkd3d-proton) |
| tiled resources / conservative rasterization tier | 0 / 0 |
| typed UAV loads (18 formats) | yes |
| missing for 11_1 | maxPerStageDescriptorStorageImages >= 64 (is 32) |
| missing for 12_0 | TiledResourcesTier >= 2 (is 0): missing sparseBinding, sparseResidencyAliased, sparseResidencyBuffer, sparseResidencyImage2D, a queue family with VK_QUEUE_SPARSE_BINDING_BIT, shaderResourceResidency, shaderResourceMinLod, residencyNonResidentStrict; (and everything missing for 11_1) |
| missing for 12_1 | ROVsSupported: VK_EXT_fragment_shader_interlock fragmentShaderPixelInterlock + fragmentShaderSampleInterlock (extension not exposed); ConservativeRasterizationTier >= 1: VK_EXT_conservative_rasterization (not exposed); (and everything missing for 12_0) |
| | tiled tier 2 also needs `filterMinmaxSingleComponentFormats` (Vulkan 1.2), not in PanProbe's dump |
| shader model, next step | SM 6.6: VK_KHR_compute_shader_derivatives computeDerivativeGroupLinear; SM 6.7: VK_KHR_shader_maximal_reconvergence, VK_KHR_shader_quad_control |
| 12_2 (out of scope) | shaderOutputViewportIndex+Layer; VK_EXT_mesh_shader; VK_KHR_ray_query + acceleration_structure |

## Summary (`an115f.py`)

```text
results.json files: 3
## 20261027-030754-s3 scenario 3 both ok=False error=None
   driverInfo Mesa 26.3.0-devel (git-9dde9e7a18) | driver mesa-panvk-26.3.0-devel-git-9dde9e7a18 73709644d8f01783 29909088 | app {'package': 'dev.zenithblue.panvklauncher', 'versionName': '1.3.0', 'versionCode': 41}
   device SM-A315N 31 MT6768V/CA 60
 - 1 d3d11/default: fail reason=test failed: CUBE: FAIL Present hr=0x80004005 exit=1 wall=25.1 first=24.0 fps=-(None) drv=- maxms=- dxvk=- p50/95/99=-/-/- gpu=- drawn=-
     test ok=False fps=None startup=None line=CUBE: FAIL Present hr=0x80004005
     fpslog=Mesa 26.3.0-devel (git-9dde9e7a18) shot=None
 - 2 d3d9/default: fail reason=test failed: CUBE: FAIL VB hr=0x8876086c exit=1 wall=8.7 first=- fps=-(None) drv=- maxms=- dxvk=- p50/95/99=-/-/- gpu=- drawn=-
     test ok=False fps=None startup=None line=CUBE: FAIL VB hr=0x8876086c
     fpslog=None shot=None
   harness.log 62 lines; last: [03:12:27] done: ok=false
     ! [03:12:11] CUBE: FAIL Present hr=0x80004005
     ! [03:12:13] pass 1: fail test failed: CUBE: FAIL Present hr=0x80004005 fps=n/a
     ! [03:12:26] CUBE: FAIL VB hr=0x8876086c
     ! [03:12:27] pass 2: fail test failed: CUBE: FAIL VB hr=0x8876086c fps=n/a
## 20261027-031230-s5 scenario 5 d3d12 ok=True error=None
   driverInfo Mesa 26.3.0-devel (git-9dde9e7a18) | driver mesa-panvk-26.3.0-devel-git-9dde9e7a18 73709644d8f01783 29909088 | app {'package': 'dev.zenithblue.panvklauncher', 'versionName': '1.3.0', 'versionCode': 41}
   device SM-A315N 31 MT6768V/CA 60
 - 1 d3d12/default: ok reason=None exit=0 wall=104.8 first=11.5 fps=0.9(driver) drv=0.9 maxms=1138.3 dxvk=- p50/95/99=-/-/- gpu=- drawn=0.9
     test ok=True fps=0.9 startup=7.3 line=DX12: PASS test=d3d12-multithreading frames=82 secs=89.0 fps=0.9 startup=7.3
     fpslog=Mesa 26.3.0-devel (git-9dde9e7a18) shot=screen.png
   harness.log 7 lines; last: [03:14:17] done: ok=true
## 20261027-031420-s6 scenario 6 d3d12gears ok=True error=None
   driverInfo Mesa 26.3.0-devel (git-9dde9e7a18) | driver mesa-panvk-26.3.0-devel-git-9dde9e7a18 73709644d8f01783 29909088 | app {'package': 'dev.zenithblue.panvklauncher', 'versionName': '1.3.0', 'versionCode': 41}
   device SM-A315N 31 MT6768V/CA 60
 - 1 d3d12gears/default: ok reason=None exit=0 wall=96.1 first=6.4 fps=101.8(driver) drv=101.8 maxms=32.6 dxvk=- p50/95/99=-/-/- gpu=- drawn=45.0
     test ok=True fps=101.9 startup=2.6 line=DX12: PASS test=vkd3d-gears frames=8972 secs=88.0 fps=101.9 startup=2.6
     fpslog=Mesa 26.3.0-devel (git-9dde9e7a18) shot=screen.png
   harness.log 7 lines; last: [03:16:00] done: ok=true
logcat faults: {'event=0x': 3}
logcat counts: {'kbase: coherency mode': 25, 'CUBE: FAIL': 4, 'DX12: PASS': 2}
driver strings: {}
```

## Screenshots

- video-start/mid/end.png
- d3d12-squidroom-harness.png <- raw/a31-31-en-portrait/artifacts/sdcard/Android/data/dev.zenithblue.panvklauncher/files/gameloop/20261027-031230-s5/01-d3d12-default/screen.png
- d3d12gears-harness.png <- raw/a31-31-en-portrait/artifacts/sdcard/Android/data/dev.zenithblue.panvklauncher/files/gameloop/20261027-031420-s6/01-d3d12gears-default/screen.png

