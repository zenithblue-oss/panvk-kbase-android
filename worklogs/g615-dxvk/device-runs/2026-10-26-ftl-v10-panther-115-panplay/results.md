# FTL panther (Pixel 7, Mali-G710, v10) build 115: PanPlay game loop (scenarios 3, 5, 6)

- Matrix `matrix-3mq4u6b3hjyk7`; outcome `Passed panther-33-en-portrait --`; results `gs://test-lab-r9iqhnhjf96wc-h6in9q007ukk4/panvk-v10-panther-115-panplay/` (project `panvk-device-test`).
- APK `/home/flux/build-out/apks/panplay-115.apk` (sha256 `09a9aba6...`), `dev.zenithblue.panvklauncher` 1.3.0 (vc 41), bundled driver build 115 (`73709644...`). Default window: 20 s warm-up, 60 s measured, 8 s tail.
- `raw/` has everything but `video.mp4` (not committed, about 45 MB).

| scenario | task | result | fps | test line |
|---|---|---|---|---|
| 3 | d3d11 | fail (test failed: CUBE: FAIL Present hr=0x887a0007) | - | `CUBE: FAIL Present hr=0x887a0007` |
| 3 | d3d9 | fail (no pass line (^CUBE: PASS)) | - | `-` |
| 5 | d3d12 | ok | 85.8 | `DX12: PASS test=d3d12-multithreading frames=7548 secs=88.0 fps=85.8 startup=3.9` |
| 6 | d3d12gears | fail (no fps data in the window (no frames 20-80 s after the first)) | - | `DX12: PASS test=vkd3d-gears frames=81815 secs=88.0 fps=929.7 startup=1.3` (presents on a lost device, see Result) |

## Result

**3 of 4 passes fail: the GPU queue hangs** (FTL's outcome Passed only means the app exited). D3D11 cube, D3D9 cube and
D3D12 gears all hit the same driver timeout; SquidRoom passes. This is the first PanPlay run on an FTL Pixel 7 since the
beta.18-dev build (CHANGELOG beta.18 validation: PanProbe and the D3D11 cube passed on the Pixel 7), and the 110-114
panther runs are PanProbe only, so there is no recent PanPlay baseline on this device. Driver
`Mesa 26.3.0-devel (git-9dde9e7a18)` (`.so` sha256 `73709644...`) in every pass.

| test | result | fps | notes |
|---|---|---|---|
| D3D11 cube (DXVK) | **FAIL** `CUBE: FAIL Present hr=0x887a0007` (DXGI_ERROR_DEVICE_RESET) | - (driver: 561 fps for 1.5 s) | first frames correct (`video-d3d11-cube.png`), then the GPU hangs |
| D3D9 cube (DXVK, i686) | **FAIL** no pass line, exit 3 | - (driver: 90.1 fps, vsync 90 Hz, for 17.7 s) | renders with the HUD (`video-d3d9-cube.png`, video frame, HUD text too small to read), then the GPU hangs |
| D3D12 SquidRoom (vkd3d-proton) | `DX12: PASS ... frames=7548 secs=88.0 fps=85.8 startup=3.9` | **85.8** (driver 85.8) | full scene (`d3d12-squidroom-harness.png`), no hang in 88 s |
| D3D12 gears (vkd3d-proton) | **FAIL** (harness: no fps data in the window) | - (driver: 364 fps for 1.5 s) | gears correct (`video-d3d12gears.png`), then the GPU hangs. The app's `DX12: PASS ... frames=81815 fps=929.7` counts presents on a lost device |

**The hang, the same in all three** (`session/mesa-panvk.txt`, logcat clock):

| pass | first present | driver counted | `kbase: timeout on subqueue 0` | seqno / target |
|---|---|---|---|---|
| D3D11 cube | 02:09:19.660 | 867 frames in 1.544 s | 02:09:31.204 | 4337 / 4344 |
| D3D9 cube | 02:09:48.217 | 1597 frames in 17.715 s | 02:10:15.913 | 7983 / 7995 |
| D3D12 gears | 02:12:38.968 | 536 frames in 1.473 s | 02:12:50.441 | 2149 / 2153 |

Each timeout fires 10 s after the last frame the driver counted. The dump shows all three subqueues (0, 1, 2) stopped at
the same seqno, with `active 1, error 0x0`, `marks pre/post-call/post-wait 0x0/0x0/0x0` and `stream progress 0x0`. Each
extract pointer sits on the ring's `CALL` (`0x20005c5e00000000`) into the submitted command stream (subqueue 0 stream
`0x5fffd39000/184`, 1 `0x5fffd37000/216`, 2 `0x5fffd38000/936` in the D3D11 pass). So the command streams are waiting on
something that never signals. There is no CS fault, no MMU/GPU fault and no `event=0x` line, so this is a wait that never
completes, not a crash. The application errors follow from it:
- D3D11: one Present blocks for 10002.5 ms, then DXGI_ERROR_DEVICE_RESET.
- D3D9: DXVK logs `vkWaitForPresentKHR failed: VK_ERROR_DEVICE_LOST` and `Command submission failed: VK_ERROR_DEVICE_LOST`.
  Presents then run unthrottled to frame 271080, until `err:virtual:allocate_virtual_memory out of memory for allocation,
  base 0x0 size 01000000`, `win32u_vkMapMemory2KHR NtAllocateVirtualMemory failed`,
  `terminating due to uncaught exception of type dxvk::DxvkError`, exit 3.
- Gears: vkd3d-proton logs `Failed to wait for Vulkan timeline semaphore, vr -4` and
  `d3d12_command_queue_execute: Failed to submit queue(s), vr -4` (22361 `err:` lines), while Present keeps returning S_OK.

Uncapped present rates hang within 1.5 s (D3D11 561 fps, gears 364 fps). The vsync-bound D3D9 cube lasts 17.7 s. SquidRoom
at 85.8 fps runs its full 88 s. `screenshot failed (no display view)` in the D3D9 and gears passes comes after the hang.

**Knob-level evidence (these runs only).** This PanPlay run has one env set (`default`), so it has no in-run knob A/B.
Every pass prints the build 115 defaults, the same as on shiba:
- `kbase: coherency mode 31 (none), shaderDeviceClock off (strict, PANVK_KBASE_STRICT_DEVICE_CLOCK=0 = 114), coherent pools off`
- `HOST_COHERENT dma-heap BOs from /dev/dma_heap/system-uncached (PANVK_KBASE_UNCACHED_HEAP=0 = 115)`
- `timestamp calibration: CSF, keeping cntfrq 24576000 Hz`
- `BC emulation on, lowmem`
- `tiler heaps 4 initial / 400 max chunks of 1024 KiB`

The same lines appear in the shiba (v11 CSF) PanPlay run of this build, where all four tests ran 88 s with no
`timeout on subqueue`. The panther PanProbe run of this build (`../2026-10-26-ftl-v10-panther-115/`) passed 39/39 in both
`defaults` and `114-knobs` (`PANVK_KBASE_UNCACHED_HEAP=0` plus the 27 other 114 values) and has 0 `timeout on subqueue`
lines. So these runs do not tie the hang to any 115 knob. The next step is to rerun PanPlay on panther with the
`114-knobs` env set. **Regression against beta.18's D3D11 cube on the Pixel 7. The build that introduced it cannot be
identified from FTL data, because 110-114 have no panther PanPlay runs.**

**D3D12 detail.** Both tests create their device at FL 11_0 and it succeeds: vkd3d-proton `build: 2230755878b0199`,
`Enabling support for SM 6.6`, `6.7`, `6.8`, `dxgi_vk_swap_chain_init: Creating swapchain (1280 x 720), BufferCount = 3`
(both tests). Neither test logs a vkd3d-proton `err:` line before the hang. The only warnings are optional
`VK_KHR_surface_maintenance1` not supported and `No device info available for TotalLaneCount = 512`. DXGI adapter
`Mali-G710 MC7 (panvk 26.2.99)`. After its PASS line, SquidRoom hits
`Unhandled page fault on read access to 0000000000000070` at exit, as on shiba (exit code 0).

The only lowmemorykiller kill in logcat is a Chrome sandboxed process at 02:07:41, before the first pass.

### D3D12 feature level (vkd3d-proton) on this device

vkd3d-proton 2230755 (PanPlay's `d3d12core.dll`) logs its max feature level only at TRACE, and the bundled release
build has TRACE compiled out (the `Max feature level` string is not in the DLL). The level below is vkd3d-proton's own
`d3d12_device_caps_init_feature_level` (libs/vkd3d/device.c at 2230755) applied to this device's Vulkan caps from the
build 115 PanProbe run (`../2026-10-26-ftl-v10-panther-115/`, `device_info.json` and `vkd3d_reqs`, whose own
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
| 12_2 (out of scope) | VK_EXT_mesh_shader; VK_KHR_ray_query + acceleration_structure |

## Summary (`an115f.py`)

```text
results.json files: 3
## 20261027-020732-s3 scenario 3 both ok=False error=None
   driverInfo Mesa 26.3.0-devel (git-9dde9e7a18) | driver mesa-panvk-26.3.0-devel-git-9dde9e7a18 73709644d8f01783 29909088 | app {'package': 'dev.zenithblue.panvklauncher', 'versionName': '1.3.0', 'versionCode': 41}
   device Pixel 7 33 GS201 90
 - 1 d3d11/default: fail reason=test failed: CUBE: FAIL Present hr=0x887a0007 exit=1 wall=33.7 first=15.9 fps=-(None) drv=- maxms=- dxvk=- p50/95/99=-/-/- gpu=- drawn=-
     test ok=False fps=None startup=None line=CUBE: FAIL Present hr=0x887a0007
     fpslog=Mesa 26.3.0-devel (git-9dde9e7a18) shot=None
 - 2 d3d9/default: fail reason=no pass line (^CUBE: PASS) exit=3 wall=70.4 first=4.7 fps=-(None) drv=- maxms=- dxvk=- p50/95/99=-/-/- gpu=- drawn=-
     test ok=None fps=None startup=None line=
     fpslog=Mesa 26.3.0-devel (git-9dde9e7a18) shot=None
   harness.log 63 lines; last: [02:10:54] done: ok=false
     ! [02:09:31] CUBE: FAIL Present hr=0x887a0007
     ! [02:09:37] pass 1: fail test failed: CUBE: FAIL Present hr=0x887a0007 fps=n/a
     ! [02:10:38] screenshot failed (no display view)
     ! [02:10:54] pass 2: fail no pass line (^CUBE: PASS) fps=n/a
## 20261027-021056-s5 scenario 5 d3d12 ok=True error=None
   driverInfo Mesa 26.3.0-devel (git-9dde9e7a18) | driver mesa-panvk-26.3.0-devel-git-9dde9e7a18 73709644d8f01783 29909088 | app {'package': 'dev.zenithblue.panvklauncher', 'versionName': '1.3.0', 'versionCode': 41}
   device Pixel 7 33 GS201 90
 - 1 d3d12/default: ok reason=None exit=0 wall=96.7 first=7.7 fps=85.8(driver) drv=85.8 maxms=26.8 dxvk=- p50/95/99=-/-/- gpu=- drawn=85.8
     test ok=True fps=85.8 startup=3.9 line=DX12: PASS test=d3d12-multithreading frames=7548 secs=88.0 fps=85.8 startup=3.9
     fpslog=Mesa 26.3.0-devel (git-9dde9e7a18) shot=screen.png
   harness.log 7 lines; last: [02:12:33] done: ok=true
## 20261027-021234-s6 scenario 6 d3d12gears ok=False error=None
   driverInfo Mesa 26.3.0-devel (git-9dde9e7a18) | driver mesa-panvk-26.3.0-devel-git-9dde9e7a18 73709644d8f01783 29909088 | app {'package': 'dev.zenithblue.panvklauncher', 'versionName': '1.3.0', 'versionCode': 41}
   device Pixel 7 33 GS201 90
 - 1 d3d12gears/default: fail reason=no fps data in the window (no frames 20-80 s after the first) exit=0 wall=121.0 first=3.9 fps=-(None) drv=- maxms=- dxvk=- p50/95/99=-/-/- gpu=- drawn=0.0
     test ok=True fps=929.7 startup=1.3 line=DX12: PASS test=vkd3d-gears frames=81815 secs=88.0 fps=929.7 startup=1.3
     fpslog=Mesa 26.3.0-devel (git-9dde9e7a18) shot=None
   harness.log 7 lines; last: [02:14:36] done: ok=false
     ! [02:13:29] screenshot failed (no display view)
     ! [02:14:36] pass 1: fail no fps data in the window (no frames 20-80 s after the first) fps=n/a
logcat faults: {'lmk kill': 1}
logcat counts: {'kbase: coherency mode': 25, 'system-uncached': 25, 'CUBE: FAIL': 2, 'CUBE: PASS': 1, 'DX12: PASS': 2}
driver strings: {}
```

## Screenshots

- video-start/mid/end.png
- d3d12-squidroom-harness.png <- raw/panther-33-en-portrait/artifacts/sdcard/Android/data/dev.zenithblue.panvklauncher/files/gameloop/20261027-021056-s5/01-d3d12-default/screen.png
- video-d3d11-cube.png: video frame (51.9 s), the D3D11 cube's first frames before the hang
- video-d3d9-cube.png: video frame (72 s), the D3D9 cube with HUD before the hang
- video-d3d12gears.png: video frame (188.3 s), gears before the hang (the harness screenshot failed: no display view)
