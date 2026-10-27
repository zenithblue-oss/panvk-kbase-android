# FTL shiba (Pixel 8, Mali-G715 MC7, v11) build 115: PanPlay game loop (scenarios 3, 5, 6)

- Matrix `matrix-1kkgygdq2v8mm`; outcome `Passed shiba-34-en-portrait --`; results `gs://test-lab-r9iqhnhjf96wc-h6in9q007ukk4/panvk-v11-shiba-115-panplay/` (project `panvk-device-test`).
- APK `/home/flux/build-out/apks/panplay-115.apk` (sha256 `09a9aba6...`), `dev.zenithblue.panvklauncher` 1.3.0 (vc 41), bundled driver build 115 (`73709644...`). Default window: 20 s warm-up, 60 s measured, 8 s tail.
- `raw/` has everything but `video.mp4` (not committed, about 45 MB).

| scenario | task | result | fps | test line |
|---|---|---|---|---|
| 3 | d3d11 | ok | 395.0 | `CUBE: PASS api=d3d11 frames=34714 (check pixels)` |
| 3 | d3d9 | ok | 117.9 | `CUBE: PASS api=d3d9 frames=10330 (check pixels)` |
| 5 | d3d12 | ok | 74.3 | `DX12: PASS test=d3d12-multithreading frames=6536 secs=88.0 fps=74.3 startup=5.2` |
| 6 | d3d12gears | ok | 166.4 | `DX12: PASS test=vkd3d-gears frames=14649 secs=88.0 fps=166.4 startup=2.6` |

## Result

All four passes `ok`, FTL outcome Passed, logcat without DEVICE_LOST, `event=0x`, GPU/MMU fault, CS_FATAL, ANR or FATAL
EXCEPTION. Driver `Mesa 26.3.0-devel (git-9dde9e7a18)` (`.so` sha256 `73709644...`) in every pass.

| test | result | fps (window 20-80 s) | notes |
|---|---|---|---|
| D3D11 cube (DXVK) | `CUBE: PASS`, 34714 frames | **395.0** | DXVK GPU load 71.7 %, p50/p95/p99 2.2/4.3/8.6 ms |
| D3D9 cube (DXVK, i686) | `CUBE: PASS`, 10330 frames | **117.9** | vsync at 120 Hz, GPU 33.0 %. **HUD clean** (`d3d9-default-hud.png`): `Version: Mesa 26.3.0-devel (git-9dde9e7a18)`, FPS 119.7 |
| D3D12 SquidRoom (vkd3d-proton) | `DX12: PASS ... frames=6536 secs=88.0 fps=74.3 startup=5.2` | **74.8** (test: 74.3) | renders the full scene (`d3d12-squidroom-harness.png`) |
| D3D12 gears (vkd3d-proton) | `DX12: PASS ... frames=14649 secs=88.0 fps=166.4 startup=2.6` | **166.0** (test: 166.4) | |

**Against 115fix** (`../2026-10-26-ftl-v11-shiba-panplay-115fix/`, the same driver source: build 115's tree `213079bc` is
the p115-fix tree). 115fix measured 8-28 s after the first frame and ran the D3D12 tests for 32 s, so the same window from
this run's fps logs: D3D11 394.2 vs 427.9 fps (-7.9 %), D3D9 118.2 vs 118.8 (vsync), SquidRoom 75.3 vs 78.9 (-4.6 %),
gears 168.5 vs 184.6 (-8.7 %). Not a build change: the driver prints the same configuration in both runs (`kbase: coherency
mode 31 (none), shaderDeviceClock off`, `HOST_COHERENT dma-heap BOs from /dev/dma_heap/system-uncached`, `BC emulation on,
lowmem`, same counts), and PanPlay's code changed only in `WhatsNew.kt` since 115fix. Everything that is not vsync-bound is
5-9 % slower, with lower DXVK GPU load (71.7 vs 78.5 %): another Pixel 8 or a warmer one. The D3D11 cube varies 377-421 fps
between 10 s blocks within this run. **No regression** that the logs can tie to the driver.

**D3D12 detail.** Both tests create their device at FL 11_0 (`D3D12CreateDevice(..., D3D_FEATURE_LEVEL_11_0, ...)`) and
it succeeds: vkd3d-proton `build: 2230755878b0199`, `Enabling support for SM 6.6`, `6.7`, `6.8`, then `dxgi_vk_swap_chain_init:
Creating swapchain (1280 x 720), BufferCount = 3`. No vkd3d-proton `err:` line. Warnings only: optional
`VK_KHR_surface_maintenance1` not supported, `Missing sufficient features to expose WMMA`, fixme `TotalLaneCount = 512`.
SquidRoom's process hits `Unhandled page fault on read access to 0000000000000070` right after its PASS line, at exit,
exactly as in 115fix (exit code 0).

### D3D12 feature level (vkd3d-proton) on this device

vkd3d-proton 2230755 (PanPlay's `d3d12core.dll`) logs its max feature level only at TRACE, and the bundled release
build has TRACE compiled out (the `Max feature level` string is not in the DLL). The level below is vkd3d-proton's own
`d3d12_device_caps_init_feature_level` (libs/vkd3d/device.c at 2230755) applied to this device's Vulkan caps from the
build 115 PanProbe run (`../2026-10-26-ftl-v11-shiba-115/`, `device_info.json` and `vkd3d_reqs`, whose own
vkd3d-proton model gives the same level). The test apps create their device at FL 11_0.

| | value |
|---|---|
| max feature level | **11_1** |
| max shader model | 6.8 (vkd3d.log: `Enabling support for SM 6.6`, `6.7`, `6.8`) |
| resource binding tier | 3 (fixed at 3 in this vkd3d-proton) |
| tiled resources / conservative rasterization tier | 0 / 1 |
| typed UAV loads (18 formats) | yes |
| missing for 11_1 | nothing |
| missing for 12_0 | TiledResourcesTier >= 2 (is 0): missing sparseBinding, sparseResidencyAliased, sparseResidencyBuffer, sparseResidencyImage2D, a queue family with VK_QUEUE_SPARSE_BINDING_BIT, shaderResourceResidency, shaderResourceMinLod, residencyNonResidentStrict |
| missing for 12_1 | ROVsSupported: VK_EXT_fragment_shader_interlock fragmentShaderPixelInterlock + fragmentShaderSampleInterlock (extension not exposed); (and everything missing for 12_0) |
| | tiled tier 2 also needs `filterMinmaxSingleComponentFormats` (Vulkan 1.2), not in PanProbe's dump |
| 12_2 (out of scope) | VK_EXT_mesh_shader; VK_KHR_ray_query + acceleration_structure |

## Summary (`an115f.py`)

```text
results.json files: 3
## 20261027-015317-s3 scenario 3 both ok=True error=None
   driverInfo Mesa 26.3.0-devel (git-9dde9e7a18) | driver mesa-panvk-26.3.0-devel-git-9dde9e7a18 73709644d8f01783 29909088 | app {'package': 'dev.zenithblue.panvklauncher', 'versionName': '1.3.0', 'versionCode': 41}
   device Pixel 8 34 Tensor G3 60
 - 1 d3d11/default: ok reason=None exit=0 wall=113.0 first=23.3 fps=395.0(driver) drv=395.0 maxms=31.9 dxvk=395.3 p50/95/99=2.2/4.3/8.6 gpu=71.7 drawn=59.6
     test ok=True fps=None startup=None line=CUBE: PASS api=d3d11 frames=34714 (check pixels)
     fpslog=Mesa 26.3.0-devel (git-9dde9e7a18) shot=screen.png
 - 2 d3d9/default: ok reason=None exit=0 wall=96.7 first=7.7 fps=117.9(driver) drv=117.9 maxms=53.2 dxvk=117.9 p50/95/99=8.1/15.8/27.5 gpu=33.0 drawn=60.0
     test ok=True fps=None startup=None line=CUBE: PASS api=d3d9 frames=10330 (check pixels)
     fpslog=Mesa 26.3.0-devel (git-9dde9e7a18) shot=screen.png
   harness.log 65 lines; last: [01:59:26] done: ok=true
## 20261027-015926-s5 scenario 5 d3d12 ok=True error=None
   driverInfo Mesa 26.3.0-devel (git-9dde9e7a18) | driver mesa-panvk-26.3.0-devel-git-9dde9e7a18 73709644d8f01783 29909088 | app {'package': 'dev.zenithblue.panvklauncher', 'versionName': '1.3.0', 'versionCode': 41}
   device Pixel 8 34 Tensor G3 60
 - 1 d3d12/default: ok reason=None exit=0 wall=98.1 first=8.4 fps=74.8(driver) drv=74.8 maxms=41.0 dxvk=- p50/95/99=-/-/- gpu=- drawn=60.0
     test ok=True fps=74.3 startup=5.2 line=DX12: PASS test=d3d12-multithreading frames=6536 secs=88.0 fps=74.3 startup=5.2
     fpslog=Mesa 26.3.0-devel (git-9dde9e7a18) shot=screen.png
   harness.log 7 lines; last: [02:01:05] done: ok=true
## 20261027-020106-s6 scenario 6 d3d12gears ok=True error=None
   driverInfo Mesa 26.3.0-devel (git-9dde9e7a18) | driver mesa-panvk-26.3.0-devel-git-9dde9e7a18 73709644d8f01783 29909088 | app {'package': 'dev.zenithblue.panvklauncher', 'versionName': '1.3.0', 'versionCode': 41}
   device Pixel 8 34 Tensor G3 60
 - 1 d3d12gears/default: ok reason=None exit=0 wall=94.6 first=5.8 fps=166.0(driver) drv=166.0 maxms=56.5 dxvk=- p50/95/99=-/-/- gpu=- drawn=60.1
     test ok=True fps=166.4 startup=2.6 line=DX12: PASS test=vkd3d-gears frames=14649 secs=88.0 fps=166.4 startup=2.6
     fpslog=Mesa 26.3.0-devel (git-9dde9e7a18) shot=screen.png
   harness.log 7 lines; last: [02:02:41] done: ok=true
logcat faults: none
logcat counts: {'kbase: coherency mode': 25, 'system-uncached': 25, 'CUBE: PASS': 2, 'DX12: PASS': 2}
driver strings: {}
```

## Screenshots

- video-start/mid/end.png
- d3d11-default-harness.png <- raw/shiba-34-en-portrait/artifacts/sdcard/Android/data/dev.zenithblue.panvklauncher/files/gameloop/20261027-015317-s3/01-d3d11-default/screen.png
- d3d9-default-harness.png <- raw/shiba-34-en-portrait/artifacts/sdcard/Android/data/dev.zenithblue.panvklauncher/files/gameloop/20261027-015317-s3/02-d3d9-default/screen.png
- d3d9-default-hud.png crop=861:1044:246:36 of d3d9-default-harness.png
- d3d12-squidroom-harness.png <- raw/shiba-34-en-portrait/artifacts/sdcard/Android/data/dev.zenithblue.panvklauncher/files/gameloop/20261027-015926-s5/01-d3d12-default/screen.png
- d3d12gears-harness.png <- raw/shiba-34-en-portrait/artifacts/sdcard/Android/data/dev.zenithblue.panvklauncher/files/gameloop/20261027-020106-s6/01-d3d12gears-default/screen.png

