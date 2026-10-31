![PanProbe 1.3.2 on build 122i: Lenovo TB336FU, Mali-G57 (v9), 41/41 pass](https://github.com/zenithblue-oss/panprobe/releases/download/panprobe-v1.3.2/panprobe.png)

![PanPlay 1.3.1 on build 122i: Poco X6 Pro, Mali-G615 MC6 (v11), DXVK D3D11 FL11_0 cube with HUD, 763 fps](https://github.com/zenithblue-oss/panprobe/releases/download/panprobe-v1.3.2/panplay.png)

# PanProbe 1.3.2

Vulkan conformance and performance probe for the Mesa PanVK driver on Mali GPUs (mali_kbase). It loads the bundled driver, the system Vulkan driver or an imported `.so`, shows what the driver reports, and runs **41** on-device tests.

Bundled drivers:
- `rc5` slot (default): [Mesa PanVK 26.3.1](https://github.com/zenithblue-oss/panvk-kbase-android/releases/tag/panvk-kbase-v26.3.1), `26.3.0-devel (git-795ca7ff3e, p0175, 2026-10-31)`, build 122i, universal Mali v6-v14 (v15 not fully supported).
- `rc4` slot: the RC4 driver (`panvk-kbase-v0.1.0-rc4`), for A/B runs.

Package `dev.zenithblue.panvktest`, versionName 1.3.2, versionCode 21. It updates 1.2.7 and 1.3.x in place.

## What is new since 1.2.7

- Bundles build 122i (26.3.1) in the default slot; the `rc4` slot keeps RC4 for A/B runs.
- 41 tests (38 in 1.2.7). New and extended tests:
  - `workload_perf`: one command buffer per frame of a real workload (DXVK GS layer and pass-through GS, transform feedback, strips with restart, conditional rendering, primitives-generated query, timestamps, MSAA, custom border colour textures, cold BC level load), with `ref` cases for comparison.
  - `tess_perf`: DXVK-like tessellation benchmark with in-process A/B of options.
  - `bc_perf`, `bc_decode`, `large_draw`, `draw_params`, `vkd3d_heap`, `d3d_fl` (D3D feature level report), `sparse_kbase` S0 probe, `swapchain_lifecycle` with present timing.
- When a test fails, the lines that made it fail come first in its result. Long benchmark output no longer hides them.
- Reads the driver patch level and build date from driverInfo (`(git-<sha>, p<NNNN>, <date>)`).
- Autorun: takes a comma list of tests, wakes the display, selects the Tests tab and keeps the screen on; `--es env "KEY=VAL;..."` sets driver environment variables (PANVK_, MESA_, PAN_ only) for A/B runs; output goes to the external dir with `PANPROBE_*` logcat lines.
- Firebase game-loop scenarios (1 = rc5 + rc4, 2 = rc5) for PanProbe, with env A/B passes.
- Safer: refuses a glibc driver, optional vendor ICD flag, swapchain test skips cleanly.

## Results on 26.3.1 (builds 122g to 122i)

| arch | device | GPU | build | result |
|---|---|---|---|---|
| v6 | Galaxy S9 SM-G960F (Firebase) | Mali-G72 | 122g | 41/41, 0 DEVICE_LOST |
| v7 | Galaxy A31 SM-A315N (Firebase) | Mali-G52 | 122g | 41/41, 0 DEVICE_LOST |
| v9 | Lenovo TB336FU | Mali-G57 | 122i | 41/41 cold and warm shader cache, 0 DEVICE_LOST |
| v9 | Pixel 6a (Firebase) | Mali-G78 | 122g, 122h | 41/41, 0 DEVICE_LOST |
| v10 | Pixel 7 (Firebase) | Mali-G710 | 122g | 41/41, 0 DEVICE_LOST |
| v11 | POCO X6 Pro | Mali-G615 | 122i | 41/41 cold and warm shader cache, 0 DEVICE_LOST |
| v12 | Galaxy Tab S10 Ultra SM-X926B | Immortalis-G720 | 122h | 41/41, 0 DEVICE_LOST |
| v13 | Galaxy Tab S11 Ultra SM-X930 | Immortalis-G925 | 122h | 41/41, 0 DEVICE_LOST |
| v14 | Galaxy Tab S12 SM-X940 | Mali-G1-Ultra | 122i | 41/41, 0 DEVICE_LOST |

122h = 122g + 0174 (incremental rendering on v12). 122i = 122h + 0175 (software X11 WSI probe only).

## Speed of the bundled driver (PanProbe test times, warm cache)

Full table and the RC4 / 116 comparison are in the [driver release](https://github.com/zenithblue-oss/panvk-kbase-android/releases/tag/panvk-kbase-v26.3.1). Key results:

**POCO X6 Pro (v11)**, build 122g, same session:

| test | RC4 | 116 | 26.3.1 | vs RC4 | vs 116 |
|---|---|---|---|---|---|
| total test time, s | 117.4 | 108.5 | 103.5 | -12 % | -5 % |
| tess_perf, ms | 28263 | 18226 | 17790 | -37 % | -2 % |
| workload_perf, ms | 8056 | 12309 | 7532 | -7 % | -39 % |
| pass-through GS draw (gs_pass), us | 48.2 | 53.7 | 1.1 | -98 % | -98 % |
| GS strips with restart (strip_gs), us | 157.0 | 163.2 | 1.6 | -99 % | -99 % |

**Lenovo TB336FU (v9)**, build 122f:

| test | RC4 | 116 | 26.3.1 | vs RC4 | vs 116 |
|---|---|---|---|---|---|
| total test time, s (see note) | 331.3 | 156.7 | 149.5 | -55 % | -5 % |
| tess_perf, ms | fails | 32713 | 32205 | – | -2 % |
| large_draw, ms | fails | 10189 | 7390 | – | -27 % |
| workload_perf, ms | 22039 | 11429 | 9200 | -58 % | -20 % |
| bc_perf, ms | 54008 | 46732 | 45228 | -16 % | -3 % |
| pass-through GS draw (gs_pass), us | 52.0 | 34.5 | 6.7 | -87 % | -81 % |
| GS strips with restart (strip_gs), us | 1935 | 1187 | 17.2 | -99 % | -99 % |

Cold shader cache `bc_decode`: X6 Pro 5027 ms (116) to 540 ms (RC4 464); TB336FU 4608 ms (RC4) to 653 ms.

## Known issues

- No new test failures. v12 and v13 were run on 122h, not 122i.
- Fallout 4 black face on DXVK and D3D12 FL 12_x (unsupported) are driver issues, not PanProbe issues.
- Single-run drops on Firebase (Galaxy A31, Pixel 7) are unconfirmed; see the driver release.

## Install

Install `PanProbe-v1.3.2.apk` (Android 10 or newer, arm64). It updates 1.2.x and 1.3.0/1.3.1 in place; run "Run all" and send the result when you test a GPU not in the table.

## Files and checksums (sha256)

Signing certificate SHA-256: `8b10fde8c12eff4a29e99ae669ba88854e6003178d9c3ec299254fe87c76f3bf`.

| file | sha256 |
|---|---|
| `PanProbe-1.3.2.apk` | `104801b565fd60fff3297e4698191124f61eb765f2d5c8965b7d78446e95bc73` |
| `PanProbe-v1.3.2.apk` | `104801b565fd60fff3297e4698191124f61eb765f2d5c8965b7d78446e95bc73` |
| bundled `libvulkan_panfrost_rc5.so` (build 122i) | `62c2b3f98ca987d5ab50b6f01e2d2c1c5b3ee995fe93666aa5da9415f65c3348` |
| bundled `libvulkan_panfrost_rc4.so` | `977c0c4c0803b9940c453d691f50f14c9f8f732fe945d93bdde637880befd791` |


