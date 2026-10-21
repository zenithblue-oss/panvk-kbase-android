# G615 device run 2026-10-21 (Poco X6 Pro, Mali-G615 MC6, v10/CSF)

- Device: `POCO/duchamp_global/duchamp:16/BP2A.250605.031.A3/OS3.0.6.0.WNLMIXM:user/release-keys`
- APK: `/home/flux/build-out/apks/panprobe-bc.apk`, package `dev.zenithblue.panvktest.bc` (v1.2.7, code 15)
- rc4 = `PanVK-kbase RC4 (Mesa 26.3.0-devel (git-5437602824))` (RC4-lowmem)
- rc5 = `PanVK-kbase RC5-dev-bc-r30 (Mesa 26.3.0-devel (git-3925d562d7))`
- Note: the "DRIVER slot=" line is not emitted by this PanProbe build; the driver is identified by `PANPROBE_RESULT driver=` and `summary.json` driverInfo.
- Memory: PanProbe does not report VmHWM; only per-format image memory (`mem_KiB`) is available. Peak RSS was not captured.
- Files: `ab-runs/` = pulled `files/runs/` (all runs), `ab-*.log` = logcat dumps per A/B run, `rc4-all.log` = logcat of the rc4 full run. The logcat of the rc5 full run was lost (buffer came back empty on dump); its results are in `ab-runs/20261021-150601/summary.json` and the per-test logs, plus the PANPROBE lines quoted below.

## 1. Full suite (`--es autorun all`)

| driver | PANPROBE_RESULT |
|---|---|
| rc4 (RC4-lowmem) | pass=37 fail=1 skip=0 (fail: bc_decode, BC_DEVICE_FAILS=32) |
| rc5 (BC r30) | pass=37 fail=1 skip=0 (fail: gs_viewport_depth) |

Per-test diff (38 tests, everything else PASS on both):

| test | rc4 | rc5 |
|---|---|---|
| bc_decode | FAIL (`BC_DEVICE_FAILS=32`: every one of the 16 formats has raw=FAIL copy=FAIL, blit=PASS) | PASS (`BC_DEVICE_FAILS=0`) |
| gs_viewport_depth | PASS | FAIL (reproduced when run alone: `ab-rc5-gs_viewport_depth-rerun.log`) |

All other tests PASS on both drivers: gpu_prerast_slice, clip_cull, multi_viewport, fill_mode, bc_perf, geometry, tessellation, xfb, pipeline_stats, vertex_stores, vs_viewport_index, depth_bounds, large_draw, vmr_secondary, tess_cond_state, bachata_reqs, bachata_exec, robustness2, robust_image_access2, blend, occlusion_query, descriptor_model, shader_arith, depth_stencil, sampler, draw_params, submit_stress, csf_event, gs_tess_primitive_id, bachata_storage_fmtless, bachata_dynamic_render, dxvk_reqs, vkd3d_reqs, vkd3d_heap, vkd3d_timeline, swapchain_lifecycle.

### RC5 failure: gs_viewport_depth

```
PANPROBE_TEST name=gs_viewport_depth status=FAIL msg=RESULT FAIL
FEATURE geom=1 multiVp=1 depthClamp=1 clipExt=1 depthClipEnable=1
PASS case B_clamp_clip depthL=[0.125..0.125] expL=0.125 depthR=[0.75..0.75] expR=0.75 badDepth=0 badColor=0
PASS case C_noclamp    depthL=[0.125..0.125] expL=0.125 depthR=[0.75..0.75] expR=0.75 badDepth=0 badColor=0
FAIL case A_clamp      depthL=[0.25..0.25] expL=0.25 depthR=[0.25..0.5] expR=0.5 badDepth=256 badColor=256
```

rc4 on the same case: `PASS case A_clamp ... depthR=[0.5..0.5] expR=0.5 badDepth=0`. So in the depth-clamp-only case with a geometry shader writing the viewport index, the right viewport (index 1) gets depth 0.25 on part of the image (256 bad pixels) instead of 0.5, i.e. per-viewport depth clamp range for viewport 1 is partially applied wrongly. B and C pass. There are no MESA warnings or errors in that run (log only shows the normal kbase/panvk init lines), so it is a silent rendering difference. Full log: `ab-runs/20261021-150601/20261021-150508-gs_viewport_depth.log`. The rc5 build is a different Mesa commit (3925d562d7 vs 5437602824), so the regression comes from the Mesa tree delta, not necessarily from the BC work.

## 2. BC A/B on rc5 (all PASS in PanProbe; env was accepted, see `PANPROBE_ENV` lines)

| config (run dir under ab-runs) | test | result |
|---|---|---|
| default (151052 / 151124) | bc_perf / bc_decode | PASS / PASS, `BC raw store spills to .../cache` logged |
| `PANVK_DEBUG=bc_timing` (151218) | bc_perf | PASS, 40 `bc_timing` lines |
| `PANVK_BC_RAW=plane` (151307) | bc_perf | PASS, no spill line, memory doubled |
| `PANVK_BC_BC6H=exact` (151424 / 151450) | bc_perf / bc_decode | PASS / PASS (0 fails) |
| `PANVK_BC_MODE=exact` (151511) | bc_decode | PASS (0 fails, 16/16) |
| `bc_timing;PANVK_BC_LM_WG=16x4` (151817) | bc_perf | PASS |
| `bc_timing;PANVK_BC_LM_WG=32x2` (151916) | bc_perf | PASS |
| `bc_timing;PANVK_BC_LM_WG=4x4` (152007) | bc_perf | PASS |

The `;` in env values needs the remote command quoted: `adb shell "am start ... --es env 'A=b;C=d' ..."`. An unquoted first attempt started the app without extras and was discarded.

### Image memory (KiB, 2048x2048 mip chain), columns: rc4 | rc5 default | rc5 timing | rc5 raw=plane | rc5 BC6H=exact | rc5 WG16x4 | WG32x2 | WG4x4

| fmt | rc4 | rc5 def | timing | raw=plane | bc6h=exact | 16x4 | 32x2 | 4x4 |
|---|---|---|---|---|---|---|---|---|
| BC1 (RGB, RGBA) | 2732 | 2732 | 2732 | 5464 | 2732 | 2732 | 2732 | 2732 |
| BC2/BC3/BC5 | 5464 | 5464 | 5464 | 10928 | 5464 | 5464 | 5464 | 5464 |
| BC4 | 2732 | 2732 | 2732 | 5464 | 2732 | 2732 | 2732 | 2732 |
| BC6H_UFLOAT | 22644 | 5464 | 5464 | 10928 | 22644 | 5464 | 5464 | 5464 |
| BC6H_SFLOAT | 44244 | 44244 | 44244 | 49708 | 44244 | 44244 | 44244 | 44244 |
| BC7 | 5468 | 5468 | 5468 | 10932 | 5468 | 5468 | 5468 | 5468 |
| REF_RGBA8 | 22644 | 22644 | 22644 | 22644 | 22644 | 22644 | 22644 | 22644 |

Findings: the plan's v2 column is met for BC1 (2.7 MiB), BC3 (5.4), BC7 (5.4) and BC6H UFLOAT (5.6 expected, 5.46 measured) on rc5 default. rc4 already had BC1-5 and BC7 at the same size; the saving of rc5 vs rc4 is BC6H_UFLOAT (22644 -> 5464). BC6H_SFLOAT is still 44 MiB on both (not reduced). `raw=plane` doubles the BC memory. `BC6H=exact` returns BC6H_UFLOAT to 22644.

### Upload GPU ms (same column order)

| fmt | rc4 | rc5 def | timing | raw=plane | bc6h=exact | 16x4 | 32x2 | 4x4 |
|---|---|---|---|---|---|---|---|---|
| BC1_RGB | 3.5 | 23.3 | 24.8 | 25.7 | 25.9 | 25.2 | 26.5 | 27.0 |
| BC1_RGBA_SRGB | 3.2 | 16.2 | 18.1 | 18.5 | 19.0 | 18.3 | 19.4 | 18.8 |
| BC2 | 4.1 | 38.7 | 38.0 | 40.9 | 39.6 | 36.8 | 35.5 | 38.9 |
| BC3_UNORM | 3.0 | 34.8 | 42.2 | 37.4 | 41.2 | 42.0 | 39.0 | 38.6 |
| BC3_SRGB | 3.3 | 32.5 | 35.3 | 43.1 | 35.6 | 39.3 | 39.8 | 39.3 |
| BC4_UNORM | 1.4 | 17.2 | 19.4 | 19.5 | 19.7 | 19.9 | 20.8 | 20.6 |
| BC4_SNORM | 2.1 | 17.3 | 19.2 | 19.1 | 19.2 | 19.9 | 19.8 | 19.9 |
| BC5_UNORM | 2.3 | 33.9 | 39.0 | 39.2 | 38.3 | 39.4 | 39.4 | 39.2 |
| BC5_SNORM | 2.5 | 33.8 | 38.3 | 38.4 | 37.8 | 39.4 | 38.9 | 38.9 |
| BC6H_UFLOAT | 7.4 | 65.9 | 71.2 | 80.3 | 8.5 | 87.3 | 81.9 | 80.7 |
| BC6H_SFLOAT | 8.2 | 8.4 | 9.8 | 9.5 | 9.3 | 9.8 | 9.9 | 9.2 |
| BC7_UNORM | 5.1 | 5.4 | 5.9 | 6.0 | 5.9 | 6.9 | 6.2 | 5.9 |
| BC7_SRGB | 5.2 | 5.3 | 6.6 | 6.0 | 5.8 | 6.0 | 6.4 | 5.9 |
| REF_RGBA8 | 2.7 | 2.9 | 2.6 | 3.4 | 2.1 | 2.1 | 2.5 | 2.7 |

Key regression: on rc5, uploads of BC1-BC5 and BC6H_UFLOAT cost 5-10x more than on rc4 (17-40 ms, BC6H_UFLOAT 66-87 ms vs 1.4-8 ms). The `bc_timing` lines show these are the GPU ETC2/EAC (BC1-BC5) and ASTC HDR (BC6H_UFLOAT) compute transcodes (`xcode:`: BC1 18-29 ms, BC2/3/5 35-44 ms, BC4 19 ms, BC6H_UFLOAT 71-85 ms, per 2048x2048 12-region upload) and BC6H_SFLOAT/BC7 use the GPU `decode:` path (BC6H_S 9.7 ms, BC7 5.9-6.6 ms). With `PANVK_BC_BC6H=exact` BC6H_UFLOAT upload drops to 8.5 ms (GPU path), at 22644 KiB. `PANVK_BC_LM_WG` (16x4, 32x2, 4x4) and raw=plane made no measurable difference to the xcode upload times (correction 2026-10-21, perf review: the workgroup size applies to the transcoder, not to the decode formats, so the transcode cost is ALU-bound per block, not occupancy-bound; the BC7/BC6H_S numbers are within noise). The csf-v11/200 palette-encoder rewrite (RC5-dev-bc2) targets that ALU cost.

### Sampling mode E ms (sampling cost, same column order)

| fmt | rc4 | rc5 def | timing | raw=plane | bc6h=exact | 16x4 | 32x2 | 4x4 |
|---|---|---|---|---|---|---|---|---|
| BC1/2/3/5 (typical) | 0.32-0.36 | 0.32-0.33 | 0.37 | 0.37 | 0.37 | 0.37 | 0.37-0.41 | 0.37 |
| BC4 | 0.22-0.30 | 0.22 | 0.25 | 0.25 | 0.25 | 0.25 | 0.25 | 0.25 |
| BC6H_UFLOAT | 0.285 | 0.733 | 0.838 | 0.843 | 0.433 | 0.841 | 0.836 | 0.835 |
| BC6H_SFLOAT | 0.641 | 0.638 | 0.724 | 0.704 | 0.702 | 0.721 | 0.715 | 0.723 |
| BC7 | 0.22 | 0.23 | 0.26 | 0.26 | 0.26 | 0.27 | 0.26 | 0.26 |

The later runs show a uniform +12 percent shift on all formats including REF_RGBA8 (0.329 -> 0.35-0.36), i.e. device clock/thermal drift, not a driver effect. Modes A-D are flat at 0.43 / 0.215 across formats (see per-run `bc_perf` logs). The open question from the plan (BC6H UF ASTC vs exact, mode E): ASTC path 0.73-0.84 ms vs exact 0.43 ms (rc4 lowmem 0.285 ms with 22644 KiB). So the ASTC BC6H path is about 2x slower to sample than exact on this device.

## 3. Not covered

- Peak RSS / VmHWM (not available through PanProbe).
- Game test (section D of the checklist), raw-store test, copy matrix, CTS filters.
- rc4 baseline for the env variants: the rc4 `all` run provides the default bc_perf/bc_decode baseline (env keys do not apply to RC4-lowmem as a control).
