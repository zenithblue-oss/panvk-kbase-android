# FTL oriole (Pixel 6, Mali-G78 MC20, v9) build 115: PanPlay game loop (scenarios 3, 5, 6)

- Matrix `matrix-xpkhx2x86kuqa`; outcome `Passed oriole-33-en-portrait --`; results `gs://test-lab-7jybm598jy36m-hdr2td6as0u1q/panvk-v9-oriole-115-panplay/` (project `phone-finder-hub`).
- APK `/home/flux/build-out/apks/panplay-115.apk` (sha256 `09a9aba6...`), `dev.zenithblue.panvklauncher` 1.3.0 (vc 41), bundled driver build 115 (`73709644...`). Default window: 20 s warm-up, 60 s measured, 8 s tail.
- `raw/` has everything but `video.mp4` (not committed, about 45 MB).

| scenario | task | result | fps | test line |
|---|---|---|---|---|
| 3 | d3d11 | ok | 168.1 | `CUBE: PASS api=d3d11 frames=14883 (check pixels)` |
| 3 | d3d9 | ok | 90.0 | `CUBE: PASS api=d3d9 frames=7855 (check pixels)` |
| 5 | d3d12 | ok | 27.0 | `DX12: PASS test=d3d12-multithreading frames=2376 secs=88.0 fps=27.0 startup=3.5` |
| 6 | d3d12gears | ok | 232.7 | `DX12: PASS test=vkd3d-gears frames=20481 secs=88.0 fps=232.7 startup=1.4` |

## Result

All four passes `ok`. FTL outcome Passed. logcat has no DEVICE_LOST, `timeout on subqueue`, `event=0x`, GPU/MMU fault,
ANR, FATAL EXCEPTION or lowmemorykiller kill. Driver `Mesa 26.3.0-devel (git-9dde9e7a18)` (`.so` sha256 `73709644...`) in
every pass. Run on project `phone-finder-hub`, because `panvk-device-test` refused it with TEST_QUOTA_EXCEEDED
(`matrix-1ve11rl6trvou`, validation only, no device time).

| test | result | fps (window 20-80 s) | notes |
|---|---|---|---|
| D3D11 cube (DXVK) | `CUBE: PASS`, 14883 frames | **168.1** | DXVK HUD: `D3D11 FL11_0`, `Mali-G78 MC20`, `Version: Mesa 26.3.0-devel (git-9dde9e7a18)`; GPU load 87.7 %, p50/p95/p99 5.8/7.3/8.0 ms |
| D3D9 cube (DXVK, i686) | `CUBE: PASS`, 7855 frames | **90.0** | vsync at 90 Hz, GPU 36.7 %. **HUD clean** (`d3d9-default-hud.png`): `D3D9`, `Version: Mesa 26.3.0-devel (git-9dde9e7a18)`, FPS 89.6 |
| D3D12 SquidRoom (vkd3d-proton) | `DX12: PASS ... frames=2376 secs=88.0 fps=27.0 startup=3.5` | **27.0** (test: 27.0) | renders the full scene (`d3d12-squidroom-harness.png`) |
| D3D12 gears (vkd3d-proton) | `DX12: PASS ... frames=20481 secs=88.0 fps=232.7 startup=1.4` | **208.7** (test: 232.7, whole run) | `d3d12gears-harness.png` |

**Baseline:** none. This is the first PanPlay run on an FTL Pixel 6; the 110-115pre-d oriole runs are PanProbe only. The
PanProbe run of this build on the same model (`../2026-10-26-ftl-v9-oriole-115/`) passed 39/39 in both passes.

The driver prints the build 115 JM defaults in every pass:
- `coherency mode 31 (none), shaderDeviceClock off (strict, PANVK_KBASE_STRICT_DEVICE_CLOCK=0 = 114), coherent pools off`
- `JM warm-up off (PANVK_KBASE_JM_WARMUP=0 ...; =1 = 2 s once per process as in 114)`
- `stock JM atom layout (stride 72 refused), stride 56`
- `v9 tiler heaps 2 x 512 MiB (GROW_ON_GPF)`
- `dma-buf imports are not CPU-coherent; cleaning them before each submit`
- `BC emulation on`

**D3D12 detail.** Both tests create their device at FL 11_0 and it succeeds:
- vkd3d-proton logs `build: 2230755878b0199`, `Enabling support for SM 6.6`, `6.7` and `6.8`, then
  `dxgi_vk_swap_chain_init: Creating swapchain (1280 x 720), BufferCount = 3`.
- No vkd3d-proton `err:` line. Warnings only: optional `VK_KHR_surface_maintenance1` not supported,
  `Mapping VK_FORMAT_A8_UNORM_KHR to VK_FORMAT_R8_UNORM`, `No device info available for TotalLaneCount = 512`, DXBC
  checksum/unknown-DWORD notes.
- DXGI adapter `Mali-G78 MC20 (panvk 26.2.99)`.
- SquidRoom's process hits `Unhandled page fault on read access to 0000000000000070` at exit, after its PASS line, as on
  shiba and panther (exit code 0).

### D3D12 feature level (vkd3d-proton) on this device

vkd3d-proton 2230755 (PanPlay's `d3d12core.dll`) logs its max feature level only at TRACE, and the bundled release
build has TRACE compiled out (the `Max feature level` string is not in the DLL). The level below is vkd3d-proton's own
`d3d12_device_caps_init_feature_level` (libs/vkd3d/device.c at 2230755) applied to this device's Vulkan caps from the
build 115 PanProbe run (`../2026-10-26-ftl-v9-oriole-115/`, `device_info.json` and `vkd3d_reqs`, whose own
vkd3d-proton model gives the same level). The test apps create their device at FL 11_0.

| | value |
|---|---|
| max feature level | **11_1** |
| max shader model | 6.8 (vkd3d.log: `Enabling support for SM 6.6`, `6.7`, `6.8`) |
| resource binding tier | 3 (fixed at 3 in this vkd3d-proton) |
| tiled resources / conservative rasterization tier | 0 / 0 |
| typed UAV loads (18 formats) | yes |
| missing for 11_1 | nothing |
| missing for 12_0 | TiledResourcesTier >= 2 (is 0): missing sparseBinding, sparseResidencyAliased, sparseResidencyBuffer, sparseResidencyImage2D, a queue family with VK_QUEUE_SPARSE_BINDING_BIT, shaderResourceResidency, shaderResourceMinLod, residencyNonResidentStrict |
| missing for 12_1 | ROVsSupported: VK_EXT_fragment_shader_interlock fragmentShaderPixelInterlock + fragmentShaderSampleInterlock (extension not exposed); ConservativeRasterizationTier >= 1: VK_EXT_conservative_rasterization (not exposed); (and everything missing for 12_0) |
| | tiled tier 2 also needs `filterMinmaxSingleComponentFormats` (Vulkan 1.2), not in PanProbe's dump |
| 12_2 (out of scope) | shaderOutputViewportIndex+Layer; VK_EXT_mesh_shader; VK_KHR_ray_query + acceleration_structure |

## Summary (`an115f.py`)

```text
results.json files: 3
## 20261027-023234-s3 scenario 3 both ok=True error=None
   driverInfo Mesa 26.3.0-devel (git-9dde9e7a18) | driver mesa-panvk-26.3.0-devel-git-9dde9e7a18 73709644d8f01783 29909088 | app {'package': 'dev.zenithblue.panvklauncher', 'versionName': '1.3.0', 'versionCode': 41}
   device Pixel 6 33 Tensor 90
 - 1 d3d11/default: ok reason=None exit=0 wall=102.0 first=13.3 fps=168.1(driver) drv=168.1 maxms=61.4 dxvk=168.2 p50/95/99=5.8/7.3/8.0 gpu=87.7 drawn=90.1
     test ok=True fps=None startup=None line=CUBE: PASS api=d3d11 frames=14883 (check pixels)
     fpslog=Mesa 26.3.0-devel (git-9dde9e7a18) shot=screen.png
 - 2 d3d9/default: ok reason=None exit=0 wall=92.8 first=4.1 fps=90.0(driver) drv=90.0 maxms=44.0 dxvk=90.0 p50/95/99=11.1/13.0/15.5 gpu=36.7 drawn=90.0
     test ok=True fps=None startup=None line=CUBE: PASS api=d3d9 frames=7855 (check pixels)
     fpslog=Mesa 26.3.0-devel (git-9dde9e7a18) shot=screen.png
   harness.log 65 lines; last: [02:37:25] done: ok=true
## 20261027-023727-s5 scenario 5 d3d12 ok=True error=None
   driverInfo Mesa 26.3.0-devel (git-9dde9e7a18) | driver mesa-panvk-26.3.0-devel-git-9dde9e7a18 73709644d8f01783 29909088 | app {'package': 'dev.zenithblue.panvklauncher', 'versionName': '1.3.0', 'versionCode': 41}
   device Pixel 6 33 Tensor 90
 - 1 d3d12/default: ok reason=None exit=0 wall=95.0 first=5.7 fps=27.0(driver) drv=27.0 maxms=59.1 dxvk=- p50/95/99=-/-/- gpu=- drawn=26.9
     test ok=True fps=27 startup=3.5 line=DX12: PASS test=d3d12-multithreading frames=2376 secs=88.0 fps=27.0 startup=3.5
     fpslog=Mesa 26.3.0-devel (git-9dde9e7a18) shot=screen.png
   harness.log 7 lines; last: [02:39:02] done: ok=true
## 20261027-023903-s6 scenario 6 d3d12gears ok=True error=None
   driverInfo Mesa 26.3.0-devel (git-9dde9e7a18) | driver mesa-panvk-26.3.0-devel-git-9dde9e7a18 73709644d8f01783 29909088 | app {'package': 'dev.zenithblue.panvklauncher', 'versionName': '1.3.0', 'versionCode': 41}
   device Pixel 6 33 Tensor 90
 - 1 d3d12gears/default: ok reason=None exit=0 wall=92.4 first=3.7 fps=208.7(driver) drv=208.7 maxms=33.0 dxvk=- p50/95/99=-/-/- gpu=- drawn=90.2
     test ok=True fps=232.7 startup=1.4 line=DX12: PASS test=vkd3d-gears frames=20481 secs=88.0 fps=232.7 startup=1.4
     fpslog=Mesa 26.3.0-devel (git-9dde9e7a18) shot=screen.png
   harness.log 7 lines; last: [02:40:36] done: ok=true
logcat faults: none
logcat counts: {'kbase: coherency mode': 25, 'CUBE: PASS': 2, 'DX12: PASS': 2}
driver strings: {}
```

## Screenshots

- video-start/mid/end.png
- d3d11-default-harness.png <- raw/oriole-33-en-portrait/artifacts/sdcard/Android/data/dev.zenithblue.panvklauncher/files/gameloop/20261027-023234-s3/01-d3d11-default/screen.png
- d3d9-default-harness.png <- raw/oriole-33-en-portrait/artifacts/sdcard/Android/data/dev.zenithblue.panvklauncher/files/gameloop/20261027-023234-s3/02-d3d9-default/screen.png
- d3d9-default-hud.png crop=861:1044:246:36 of d3d9-default-harness.png
- d3d12-squidroom-harness.png <- raw/oriole-33-en-portrait/artifacts/sdcard/Android/data/dev.zenithblue.panvklauncher/files/gameloop/20261027-023727-s5/01-d3d12-default/screen.png
- d3d12gears-harness.png <- raw/oriole-33-en-portrait/artifacts/sdcard/Android/data/dev.zenithblue.panvklauncher/files/gameloop/20261027-023903-s6/01-d3d12gears-default/screen.png

