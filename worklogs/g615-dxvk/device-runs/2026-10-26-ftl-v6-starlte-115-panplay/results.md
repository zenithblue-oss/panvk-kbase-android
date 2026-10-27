# FTL starlte (Galaxy S9, Mali-G72 MC18, v6) build 115: PanPlay game loop (scenarios 3, 5, 6)

- Matrix `matrix-311lmai7pr6q2`; outcome `Passed starlte-29-en-portrait -- (Testing API outcomeSummary SUCCESS; gcloud polling lost DNS at 10:25, matrix polled via the API)`; results `gs://test-lab-7jybm598jy36m-hdr2td6as0u1q/panvk-v6-starlte-115-panplay/` (project `phone-finder-hub`).
- APK `/home/flux/build-out/apks/panplay-115.apk` (sha256 `09a9aba6...`), `dev.zenithblue.panvklauncher` 1.3.0 (vc 41), bundled driver build 115 (`73709644...`). Default window: 20 s warm-up, 60 s measured, 8 s tail.
- `gameloop-override.json`: the config override pushed with `--other-files` (D3D12 diagnostics env).
- `raw/` has everything but `video.mp4` (not committed, about 45 MB).

| scenario | task | result | fps | test line |
|---|---|---|---|---|
| 3 | d3d11, d3d9 | **not run**: Wine prefix setup failed (`could not load kernel32.dll, status c0000135`) | - | - |
| 5, 6 | d3d12, d3d12gears | **not run** (scenario 3 never ended) | - | - |

## Result

**No D3D test ran: PanPlay's Wine prefix setup fails on this Android 10 phone.** No Vulkan driver was loaded (no `MESA`
line in logcat), so this run says nothing about the driver. FTL's outcome Passed only means FTL did not see a crash.

**Run details:**
- Project `phone-finder-hub`.
- The gcloud client lost DNS (`EAI_AGAIN`) while polling. The matrix ran to the end on FTL, and its outcome
  (`outcomeSummary SUCCESS`) was read from the Testing API afterwards. The results were then pulled as usual. Not rerun.
- `gameloop-override.json` (D3D12 diagnostics env) was pushed, but the run never reached a D3D12 pass.

**Timeline** (scenario 3, `harness.log` and logcat):
1. 03:21:25 - 03:22:38: PanPlay installs the bundled imagefs, Proton 11.0-2 arm64ec, FEX, DXVK, vkd3d-proton, Box64,
   PulseAudio, ALSA and Mesa Zink normally.
2. 03:22:48: `setup: $ .../wine wineboot -u`.
3. 03:22:49: SELinux denies Wine's directory ioctl on the prefix:
   `avc: denied { ioctl } for comm="wine" / "wineboot.exe" path=".../.wine/drive_c/windows/system32" ioctlcmd=0x7201
   scontext=u:r:untrusted_app_27 tcontext=u:object_r:app_data_file tclass=dir permissive=0` (15 denials). 0x7201 is
   VFAT_IOCTL_READDIR_BOTH, which Wine's ntdll uses to read directories.
4. 03:27:49: `0024:err:environ:run_wineboot boot event wait timed out`, then
   `wine: could not load kernel32.dll, status c0000135` (STATUS_DLL_NOT_FOUND).
5. After that the game loop logs nothing until FTL's 45 min timeout. Scenarios 5 and 6 never started.

The first error is a Wine startup failure on Android 10 (API 29, `untrusted_app_27`), before any Direct3D or Vulkan call.
The SELinux ioctl denial on the prefix directories is the only error before it. The D3D11, D3D9 and D3D12 questions for
v6 stay open: none of these apps ran. The PanProbe run of this build on the same model (`../2026-10-26-ftl-v6-starlte-115/`)
passed 39/39 in both passes; PanProbe does not use Wine.

### D3D12 feature level (vkd3d-proton) on this device

No D3D12 test ran, so there is no `vkd3d.log`. vkd3d-proton 2230755 (PanPlay's `d3d12core.dll`) logs its max feature level only at TRACE, and the bundled release
build has TRACE compiled out (the `Max feature level` string is not in the DLL). The level below is vkd3d-proton's own
`d3d12_device_caps_init_feature_level` (libs/vkd3d/device.c at 2230755) applied to this device's Vulkan caps from the
build 115 PanProbe run (`../2026-10-26-ftl-v6-starlte-115/`, `device_info.json` and `vkd3d_reqs`, whose own
vkd3d-proton model gives the same level). The test apps create their device at FL 11_0.

| | value |
|---|---|
| max feature level | **11_0** |
| max shader model | 6.5 (vkd3d.log: not run) |
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
results.json files: 1
## 20261027-032125-s3 scenario 3 both ok=None error=None
   driverInfo None | driver None  None | app {'package': 'dev.zenithblue.panvklauncher', 'versionName': '1.3.0', 'versionCode': 41}
   device SM-G960F 29 None 60.000003814697266
   harness.log 53 lines; last: [03:22:38] setting up the Wine prefix (wineboot -u)
logcat faults: none
logcat counts: {}
driver strings: {}
```

## Screenshots

- video-start/mid/end.png

