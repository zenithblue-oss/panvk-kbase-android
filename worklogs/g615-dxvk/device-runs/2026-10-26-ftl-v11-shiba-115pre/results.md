# FTL shiba (Pixel 8, v11, Mali-G715 MC7) 115pre: PanProbe driver env A/B (scenario 3)

Both passes: **39/39** (`AUTORUN DONE pass=39 fail=0 skip=0` twice). FTL outcome: Passed. No DEVICE_LOST, `event=0x`,
GPU fault or Fatal signal lines in logcat.

- Matrix `matrix-2q4uiwh6f4fvx` (2026-10-26 13:11 UTC), results `gs://test-lab-hrb456s6qt79m-kz4jkut67musi/panvk-v11-shiba-115pre-probe/`.
  `gcloud firebase test android run --type game-loop --app panprobe-115pre.apk --scenario-numbers 3 --device model=shiba,version=34 --timeout 30m --directories-to-pull /sdcard/Android/data/dev.zenithblue.panvktest.rc5u/files`,
  under the Firebase lock.
- APK `/home/flux/build-out/apks/panprobe-115pre.apk` (sha256 `aa9b9ec4...`), `dev.zenithblue.panvktest.rc5u` 1.2.7 (vc 15).
- Device: Pixel 8 (shiba), Tensor G3, Android 14 (SDK 34), kernel 5.15.137, kbase CSF uAPI 1.24, gpu_id `0xb8a21020`, arch v11.
- Driver: the bundled rc5 slot, `.so` sha256 `652af311e78c3846018923d4838e02bbed638771facb5aea5625ec1f98108f93`
  (= `/home/flux/build-out/2026-10-26-115pre/libvulkan_panfrost.so`), build-id `7d2a26a2...`.
- Passes (`assets/env-ab.json`, sha256 `c057731a...`): 1 `defaults` (no env), 2 `114-knobs` (the 21 default-on knobs of
  staging-115 pa/pb/pc at their 114 values, `patches/staging-115/int/NOTES.md`). Pass 2 ran in the same app session,
  right after pass 1.

## Driver identity (upstream style)

| | value |
|---|---|
| `PANPROBE_RESULT` | `driver=Mesa 26.3.0-devel (git-6a5d4e2756) pass=39 fail=0 skip=0` (both passes) |
| vkinfo `driverName` | `panvk` |
| vkinfo `driverInfo` | `Mesa 26.3.0-devel (git-6a5d4e2756)` |
| vkinfo `deviceName` | `Mali-G715 MC7` (same as 114 and as upstream main) |
| app slot label (summary.json `driverType`) | `Bundled Mesa PanVK` |
| app metadata | name `Mesa PanVK`, version `26.3.0-devel (git-6a5d4e2756)` |

No `PanVK-kbase` string anywhere in the logcat.

## P-A 0008: coherency mode line

Once per device open, every test process:

```
defaults:  kbase: coherency mode 31 (none), shaderDeviceClock off (strict, PANVK_KBASE_STRICT_DEVICE_CLOCK=0 = 114), coherent pools off (PANVK_KBASE_COHERENT_POOLS=1 enables)
114-knobs: kbase: coherency mode 31 (none), shaderDeviceClock on (114 rule, coherency mode != 0), coherent pools off (PANVK_KBASE_COHERENT_POOLS=1 enables)
```

92 + 92 lines (plus one each echoed in the results JSON). shiba reports coherency mode 31 (COHERENCY_NONE), so the
opt-in coherent pools (0009) would have nothing to work with on this device. vkinfo `shaderDeviceClock`: `false` with
defaults, `true` with `114-knobs` and on 114, as intended by 0008.

The P-A registry confirms the env reached the driver: in pass 2 every test process logs
`panvk: 114 behaviour restored for: PANVK_CSF_FS_IDLE_CLEAN PANVK_DB_ALT PANVK_CSF_INDIRECT_REEMIT PANVK_CSF_DESC_DIRTY PANVK_VB_DIRTY_CMP PANVK_PRERAST_FAST_SKIP PANVK_DESC_POOL_HINT PANVK_WSI_SW_CACHED PANVK_WSI_SW_DEFER_REPLY PANVK_ANDROID_PRESENT_FENCE PANVK_ANDROID_PRESENT_THREAD PANVK_PRELOAD_BARRIER_TRACK`
(46 processes); the pb knobs do not log, they are in the pass env (`GAMELOOP pass=2/2 ... env=`).

## Correctness

- tess_perf: all 18 BENCH hashes equal to 114 in both passes (`4807f3f5`, `a03404c5`, `cec55885`, `d4463b35`, `b34a251c`).
- gs_tess_primitive_id, large_draw (`65534 of 149999` / `65534 of 139999` clamps as before), robustness2, swapchain_lifecycle
  (61.3 fps, vsync) all PASS in both passes.
- Device TLS lines unchanged from 114 (sparse TLS, 4096 B/thread: 200 MiB of VA, 56 MiB backed).

## Timing (s, from the RESULT lines)

114 = `device-runs/2026-10-25-ftl-v11-shiba-rc5u-114` (scenario 2, one cold pass on another shiba).

| test | defaults | 114-knobs | 114 | defaults vs 114 | 114-knobs vs 114 | defaults vs 114-knobs |
|---|---|---|---|---|---|---|
| gpu_prerast_slice | 0.1 | 0.1 | 0.1 | -37% | +23% | -48% |
| clip_cull | 1.2 | 1.2 | 1.2 | +0% | -1% | +1% |
| multi_viewport | 1.1 | 1.1 | 1.1 | +0% | +4% | -3% |
| fill_mode | 1.2 | 1.4 | 1.3 | -4% | +8% | -11% |
| bc_decode | 13.3 | 0.2 | 12.3 | +7% | -99% | +8691% |
| bc_perf | 67.1 | 58.6 | 68.8 | -3% | -15% | +14% |
| tess_perf | 46.7 | 47.5 | 47.3 | -1% | +0% | -2% |
| geometry | 1.6 | 1.4 | 1.6 | +1% | -11% | +13% |
| tessellation | 2.5 | 2.0 | 2.5 | -0% | -19% | +23% |
| xfb | 0.4 | 0.3 | 0.4 | -6% | -28% | +30% |
| pipeline_stats | 1.2 | 1.2 | 1.2 | +1% | +2% | -1% |
| vertex_stores | 0.1 | 0.1 | 0.2 | -29% | -36% | +11% |
| gs_viewport_depth | 0.3 | 0.2 | 0.2 | +24% | +10% | +13% |
| vs_viewport_index | 0.3 | 0.2 | 0.3 | -2% | -11% | +11% |
| depth_bounds | 0.2 | 0.2 | 0.2 | +11% | +27% | -12% |
| large_draw | 6.3 | 6.1 | 6.4 | -2% | -4% | +3% |
| vmr_secondary | 0.1 | 0.1 | 0.1 | +51% | +17% | +30% |
| tess_cond_state | 0.2 | 0.2 | 0.2 | -4% | -13% | +10% |
| bachata_reqs | 0.1 | 0.1 | 0.1 | +29% | +31% | -2% |
| bachata_exec | 0.1 | 0.1 | 0.1 | +41% | +55% | -9% |
| robustness2 | 0.1 | 0.1 | 0.1 | -29% | -4% | -26% |
| robust_image_access2 | 0.6 | 0.6 | 0.7 | -9% | -7% | -2% |
| blend | 0.1 | 0.1 | 0.1 | +34% | +28% | +5% |
| occlusion_query | 0.1 | 0.1 | 0.1 | -7% | +15% | -19% |
| descriptor_model | 0.1 | 0.1 | 0.1 | +4% | -35% | +61% |
| shader_arith | 0.1 | 0.1 | 0.2 | -39% | -48% | +18% |
| depth_stencil | 0.1 | 0.1 | 0.1 | -33% | -10% | -26% |
| sampler | 0.1 | 0.1 | 0.1 | -32% | -36% | +6% |
| draw_params | 0.1 | 0.1 | 0.1 | -21% | -5% | -17% |
| submit_stress | 30.1 | 30.1 | 30.1 | -0% | +0% | -0% |
| csf_event | 0.3 | 0.3 | 0.3 | -2% | -8% | +7% |
| gs_tess_primitive_id | 0.3 | 0.3 | 0.2 | +33% | +16% | +15% |
| bachata_storage_fmtless | 0.1 | 0.1 | 0.1 | +17% | -6% | +25% |
| bachata_dynamic_render | 0.1 | 0.1 | 0.1 | -35% | -34% | -2% |
| dxvk_reqs | 0.1 | 0.1 | 0.1 | -32% | -20% | -15% |
| vkd3d_reqs | 0.1 | 0.1 | 0.0 | +16% | +53% | -24% |
| vkd3d_heap | 0.3 | 0.3 | 0.3 | +4% | -11% | +17% |
| vkd3d_timeline | 0.3 | 0.3 | 0.3 | -1% | -6% | +6% |
| swapchain_lifecycle | 9.2 | 9.2 | 9.2 | -0% | -0% | +0% |
| total (tests in all) | 186.0 | 164.4 | 187.8 | -0.9% | -12.4% | +13.1% |

**Read this table with care.** Pass 2 (`114-knobs`) runs with a warm shader disk cache: csf-v11 170/204 keep the Mesa disk
cache on in the app cache directory, and pass 1 filled it. That is why `bc_decode` drops from 13.3 s to 0.15 s with the
same log output, and why compile-heavy tests (tessellation, geometry, xfb, bc_perf) are 10-30 % faster in pass 2. The
knobs do not change shader keys. The cold-cache comparison is `defaults` against 114: total 186.0 s against 187.8 s
(-0.9 %), every larger test within a few percent (bc_perf -3 %, tess_perf -1 %, large_draw -2 %, bc_decode +7 %),
submit_stress and swapchain_lifecycle are fixed-length. **No regression.** PanProbe does not exercise the CPU-side draw
paths the 115 knobs target; the cube A/B in PanPlay does.

GPU-timed numbers, which a warm shader cache does not change:

- tess_perf (default opts): gs=0 L=16 604 ms (defaults) / 598 ms (114-knobs) / 592 ms (114); gs=1 L=16 1035 / 1137 / 1105 ms;
  the other cases within 2 % of each other.
- bc_perf, sum of the 20 formats' GPU sampling passes A-E (ms): defaults A 11.6, B 10.8, C 10.2, D 5.3, E 9.1;
  114-knobs 12.2, 11.5, 11.4, 6.3, 9.6; 114 13.6, 12.4, 11.8, 5.8, 10.0. Upload (GPU) 51.8 / 47.9 / 55.7 ms.
  Small absolute values on different physical devices; at most a hint that defaults is not slower.

## Screenshots

`screenshots/`:
- `video-start.png`: video at 30 s, pass 1 (defaults): 5 pass, bc_perf running, bc_decode 13275 ms.
- `video-mid.png`: video middle (06:15:37), end of pass 1: 39 pass / 0 fail, uploading results.
- `video-end.png`: last frame, end of pass 2: 39 pass / 0 fail, bc_decode 151 ms, bc_perf 58591 ms.
- `panprobe-defaults-mid.png`: pass 1 (defaults) at 06:14:00, 6 pass, tess_perf running below.
- `panprobe-defaults-swapchain.png`: pass 1, swapchain_lifecycle live surface (red/blue triangle, 38 pass so far).
- `panprobe-defaults-done.png`: pass 1 done, 39 pass / 0 fail / 0 skip.
- `panprobe-114knobs-mid.png`: pass 2 (114-knobs) at 06:17:20, bc_decode 151 ms (warm cache), the app's result upload
  from pass 1 failed once (network) and was retried.
- `panprobe-114knobs-swapchain.png`: pass 2, swapchain_lifecycle live surface.
- `panprobe-114knobs-done.png`: pass 2 done, 39 pass / 0 fail / 0 skip, results uploaded (id abd5f4f7).

## Files

`raw/shiba-34-en-portrait/`: `logcat`, `video.mp4`, `game_loop_results/results_scenario_3.json` (pass 2 with
`previousPhases` = pass 1), `artifacts/.../files/autorun.txt` and `runs/20261026-061523-defaults`,
`runs/20261026-061841-114-knobs` (per-test logs, `summary.json`, `device_info.json`).
