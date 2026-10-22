# g615 RC5-universal device runs (2026-10-22)

Device: Poco X6 Pro (Mali-G615 MC6, arch v11), wifi adb 10.11.176.130:42737. Battery 100% charging, 33-34 C.
APK /home/flux/build-out/apks/panprobe-rc5u.apk, package dev.zenithblue.panvktest.rc5u (versionCode 15).
The rc5 .so in the APK is `libvulkan_panfrost_rc5.so` sha256 ab7eaa920f5d98c0390c99f4dabe0c2eb96649789fe7b2396564ffacedc4a5c4 (matches the build record). The rc4 .so is c395db09... (unchanged).
DRIVER line on every rc5 run: `PanVK-kbase RC5-universal (Mesa 26.3.0-devel (git-3093e6a58e))`. On the rc4 control run: `RC4 (git-5437602824)`.

## Summary
- **autorun all (rc5, run=20261022-185328): 39 pass / 0 fail / 0 skip, total 39.** No reruns were needed. swapchain_lifecycle passed inside the full run this time (63.1 fps, resized 60.6 fps); bc6d needed a standalone rerun for it.
- Baseline bc6d was 38+1 skip, with swapchain passing on its rerun, so effectively 39/39. **RC5-universal matches it with no regression in pass count.**
- tess_perf hashes are identical to bc6d in every config (default and legacy, gs=0/1, all L). The new path is still 7-15% faster than legacy.
- bc_perf (warm, standalone): 27676 / 27517 ms (mean 27597). bc6d rc5 was 28559 / 27799 ms (mean 28179), so this run is 2% faster, which is within noise.
- tessellation: 2629.5 fps inside autorun, and 2376 / 2432 / 2258 fps standalone. bc6d's single sample was 2877 fps. The rc5 history on this phone (bc6b/bc6c/bc6d) runs from 2409 to 2877 fps, and an rc4 control run now gave 2608 fps (bc6d's rc4 run gave 2587). The rc5u numbers are at the low end of the rc5 range. It is a 1-second fps loop and its spread is large, so this is not conclusive. See the note below.
- **PanPlay cube tests (after the user-approved uninstall and a fresh panplay-rc5u.apk install): D3D11 ARM64EC PASS 47.1 fps, D3D9 i686 PASS 48.4 fps**, both on `PanVK-kbase RC5-universal (Mesa 26.3.0-devel (git-3093e6a58e))` with soSha256 ab7eaa92. That is the same 46-47 fps as earlier cube runs on this phone. panplay-rc5u.apk is a download-only build (no bundled components), so FEX, DXVK and the other components had to be installed by hand first. See "PanPlay cube tests" below.
- The filtered logcat for the full run has no FATAL lines or crashes.

## autorun all (run=20261022-185328)
Full output: [raw/all/files/autorun.txt](raw/all/files/autorun.txt). Per-test logs and json: [raw/all/files/runs/20261022-185328/](raw/all/files/runs/20261022-185328/). Logcat (driver and app lines only): [raw/all-logcat-filtered.txt](raw/all-logcat-filtered.txt).

| test | rc5u | bc6d (172406) |
|---|---|---|
| clip_cull | PASS 2457.5 fps | PASS 2624.3 fps |
| multi_viewport | PASS 2770.3 fps | PASS 2592.6 fps |
| fill_mode | PASS 2349.6 fps | PASS 2452.6 fps |
| geometry | PASS 2657.6 fps | PASS 3048.7 fps |
| tessellation | PASS 2629.5 fps | PASS 2877.3 fps |
| pipeline_stats | PASS 2335.6 fps | PASS 2328.7 fps |
| bc_decode | PASS 15617 ms | PASS 15607 ms |
| bc_perf | PASS 43968 ms (cold cache, first run after install) | PASS |
| tess_perf | PASS 48331 ms | PASS 49257 ms |
| large_draw | PASS 8305 ms | PASS 89489 ms |
| submit_stress | PASS 30142 ms | PASS 30093 ms |
| swapchain_lifecycle | **PASS** 63.1 fps (resized 60.6) | SKIP (no surface); PASS on rerun 120.3 fps |
| all other 27 | PASS | PASS |

The bc_perf time inside autorun all (44 s) is the known cold-cache-after-install effect, seen before in bc6b (43.2 s). It does not show up in the standalone runs (27.5-27.7 s).

## Tessellation (fps, 1 s loop)
| run | driver | fps |
|---|---|---|
| 185328 (inside autorun all) | rc5u | 2629.5 |
| 185406 standalone | rc5u | 2376.4 |
| 185422 standalone | rc5u | 2432.3 |
| 185435 standalone (control) | rc4 | 2608.0 |
| 185441 standalone | rc5u | 2257.8 |
| bc6d 172406 (inside autorun all) | rc5 bc6d | 2877.3 |
| bc6d 172811 | rc4 | 2587.3 |

Earlier rc5 dev builds on this phone gave 2408.8 (bc6b), 2412.7-2817.1 (bc6c) and 2877.3 (bc6d). The rc5u mean is about 2424 fps (4 samples), about 16% below bc6d's single sample and about 7% below the rc4 control. This is not a correctness issue. If tessellation throughput matters for release, it is worth an interleaved bc6d vs rc5u A/B. Per-draw tess_perf (below) shows no regression in the tessellation path itself.

## tess_perf us_draw (256 draws, 4096 patches). rc5u: autorun 185328 / standalone 185555. bc6d: mean of its two runs.
| gs | L | new rc5u (a / s) | new bc6d | legacy rc5u (a / s) | legacy bc6d | new/legacy rc5u | hash = bc6d |
|---|---|---|---|---|---|---|---|
| 0 | ref | 2.9 / 2.8 | 3.05 | 2.4 / 3.0 | 2.2 | | yes (4807f3f5) |
| 0 | 1 | 476.7 / 475.8 | 471.0 | 552.6 / 552.9 | 534.6 | 0.86x | yes (a03404c5) |
| 0 | 4 | 541.6 / 534.3 | 512.5 | 591.9 / 601.5 | 570.5 | 0.90x | yes (cec55885) |
| 0 | 16 | 2645.5 / 2596.0 | 2588.6 | 2952.2 / 3115.0 | 3007.7 | 0.86x | yes (d4463b35) |
| 0 | mix | 2516.1 / 2537.5 | 2482.8 | 2880.4 / 2889.5 | 2824.7 | 0.88x | yes (b34a251c) |
| 1 | 1 | 671.4 / 659.1 | 710.5 | 689.6 / 696.0 | 726.5 | 0.96x | yes |
| 1 | 4 | 861.5 / 919.6 | 864.0 | 885.6 / 901.8 | 855.8 | 1.00x | yes |
| 1 | 16 | 4640.0 / 4572.2 | 4534.2 | 5256.9 / 5259.9 | 5056.1 | 0.88x | yes |
| 1 | mix | 3573.9 / 3618.8 | 3583.6 | 4112.4 / 3865.9 | 3927.8 | 0.90x | yes |

rc5u per-draw times are within plus or minus 5% of bc6d in every config (worst: gs=0 L=4 new +5%; gs=1 L=1 new is 6% faster). Hashes are identical to bc6d and between new and legacy.

## bc_perf (upload_gpu_ms, mean of standalone 185628 / 185701; range in parentheses)
Totals: rc5u 27676 / 27517 ms (mean 27597). bc6d rc5 mean 28179, rc4 mean 28364.

| format | RC4 (bc6d run) | RC5 bc6d | RC5u | rc5u vs bc6d |
|---|---|---|---|---|
| BC1_RGB_UNORM | 2.96 | 2.76 | 2.20 (1.99-2.41) | -20% |
| BC1_RGBA_SRGB | 3.42 | 3.02 | 2.72 (2.68-2.76) | -10% |
| BC2_UNORM | 4.13 | 3.39 | 3.21 (3.20-3.23) | -5% |
| BC3_UNORM | 3.02 | 3.05 | 2.49 (2.37-2.62) | -18% |
| BC3_SRGB | 4.89 | 3.12 | 3.93 (3.89-3.98) | +26% (bc6d range 2.60-3.63; noise-level) |
| BC4_UNORM | 1.55 | 0.97 | 1.60 (1.48-1.71) | noise (still at RC4 level) |
| BC4_SNORM | 1.68 | 0.83 | 1.17 (0.68-1.65) | noise |
| BC5_UNORM | 2.24 | 1.73 | 1.54 (0.97-2.11) | noise |
| BC5_SNORM | 3.08 | 1.57 | 1.70 (1.06-2.35) | noise |
| BC6H_UFLOAT | 7.54 | 4.38 | 4.22 (4.13-4.31) | -4% |
| BC6H_SFLOAT | 8.26 | 4.79 | 4.86 (4.84-4.88) | +1% |
| BC7_UNORM | 6.47 | 5.20 | 5.47 (5.21-5.73) | +5% |
| BC7_SRGB | 6.01 | 5.15 | 5.17 (5.12-5.21) | 0% |

The stable large wins over RC4 hold: BC6H at -41..-44% and BC7 at -14..-15%. The sub-2 ms formats (BC4/BC5) swing by about 2x from run to run on this phone, so their changes are noise. REF_* formats are within noise of bc6d.

## PanPlay cube tests
APK: /home/flux/build-out/apks/panplay-rc5u.apk, `dev.zenithblue.panvklauncher` 1.2.7 / vc 33, debug, installed 19:08 after the user-approved uninstall of the old 1.2.8 (see "Earlier attempt" below).
The installed `lib/arm64/libvulkan_panfrost.so` sha256 is ab7eaa92...a5c4, and both sessions record `soSha256: ab7eaa92...`.
Launched with `am start -n dev.zenithblue.panvklauncher/.MainActivity --es dev.zenithblue.panvklauncher.LAUNCH_SHORTCUT <id>` after `am force-stop`.

| cube | session | result | avg fps | 1% low | p50 frametime | GPU load avg | HUD driver line |
|---|---|---|---|---|---|---|---|
| builtin-d3d11-arm64ec (D3D11 FL11_0, ARM64EC + FEX arm64ec) | 20261022-195905 | **PASS**, rendered, exit 0 after 27 s | **47.1** (1141 frames / 24.2 s) | 24.7 | 21.67 ms | 49% | `PanVK-kbase RC5-universal (Mesa 26.3.0-devel (git-3093e6a58e))`, Mali-G615 MC6, DXVK v3.1.1+ |
| builtin-d3d9-i686 (D3D9, i686 via FEX WoW64) | 20261022-200113 | **PASS**, rendered, exit 0 after 28 s | **48.4** (1134 frames / 23.4 s) | 23.9 | 21.66 ms | 48% | same |

- The baseline for these cubes on this phone is 46-47 fps for every arch and API (091-placed-dma-heap, PROGRESS.md; Tab S10 Ultra about 47 fps). RC5-universal shows no regression. The flat 21.7 ms p50 at half GPU load points to presentation pacing, not to GPU limits.
- Geometry and shading are correct in both (four lit cubes, full DXVK HUD): screenshots `cube-d3d11-10-run25s.png` and `cube-d3d9-02-run25s.png`.
- DXVK log: `Found device: Mali-G615 MC6 (panvk 26.2.99)`. That is the Vulkan driverVersion encoding; the full RC5-universal string is on the HUD.
- Wine errors in both runs are the usual harmless ones: nsiproxy/winebth ZwLoadDriver, rpcrt4 wait error 87, and `X11DRV_OpenGLInit ... libGL.so.1 not found` (wgl only; the cubes use DXVK). The filtered logcat has no FATAL, SIGSEGV, GPU fault or DEVICE_LOST.
- Cosmetic: `device.txt` and the perf json label the driver "RC3 lowmem" because `bundled-driver.json` still pins RC3 (known, see builds/rc5-universal.md). The .so sha is RC5-universal.

### Dependencies (what was missing and how it was fixed)
panplay-rc5u.apk is 94 MB. It was built **without bundled components** (no `assets/components/`), and in `Contents.kt` only the rootfs and Proton have a download URL. FEX, DXVK, vkd3d, Box64, PulseAudio, ALSA and Mesa-Zink have `url = ""`, so their Download buttons only set "<X> ships only inside the full PanPlay APK" and do nothing.

| component | version | sha256 (= pin in app/bundled-components.json?) | source | installed via |
|---|---|---|---|---|
| Rootfs imagefs_bionic | bionic | 368db62b... (yes) | in-app download (downloads.gamenative.app) | Components > Download |
| Wine (Proton) | 11.0-2-arm64ec | fffa4672... (yes) | in-app download (GameNative/proton-wine GitHub release) | Components > Download |
| FEXCore | 2609.1 | 1701380b... (yes) | extracted from the PanPlay v1.2.7 release APK (zenithblue-oss/panplay, 383 MB) | Install local .wcp |
| DXVK | 3.1.1-4-arm64ec | c812e000... (yes) | same | Install local .wcp |
| VKD3D | 3.0.1-452-g22307558 | dd00eac3... (yes) | same | Install local .wcp |
| Box64 | 0.4.5-2f47bdf | d71d5a21... (yes) | same | Install local .wcp |
| PulseAudio | 17.0-2 | 15604aa0... (yes) | same | Install local .wcp |
| ALSA | 1.2.16.1-2 | 723aaf3c... (yes) | same | Install local .wcp |
| Mesa-Zink (OpenGL) | 26.2.4-1 | 1f1eb714... (**no**: pin is 46707d28...) | **built here** with `apps/panvk-launcher/scripts/build-mesa-zink.sh` (NDK 29.0.14206865, mesa-26.2.4.tar.xz sha OK), because the v1.2.7 APK predates it | Install local .wcp |

- All archives are in /var/tmp/panvk/components, the default `-PcomponentsDir`. All except Zink match the pins byte for byte. Zink is a local rebuild and is not bit-reproducible against the pin. It is not used by the D3D cubes.
- They were pushed to `/sdcard/Download/panplay-components/` (sha256 checked on the phone) and installed one by one with Components > Install local .wcp (SAF picker, adb taps). Afterwards Components shows every entry Installed, and `files/contents` = ALSA Box64 DXVK FEXCore Mesa-Zink Proton PulseAudio VKD3D imagefs. See `screenshots/cube-components-installed.png`.
- To make a self-contained PanPlay rc5u for testers: rebuild with `-PpanvkSo=<rc5u .so>` and the default componentsDir (now filled). The Zink pin would need updating to 1f1eb714, or use the published Zink build.

### First attempt (setup ordering, launcher issue, not the driver)
- First launch: "Allow access to all files" dialog (tapped Not now; built-in cubes do not need it), then the "What's new in 1.2.7" dialog (OK). Then nothing launched, because the shortcut waits for container setup, which needs Proton and the rootfs installed.
- As soon as Proton finished installing, container setup ran and the pending LAUNCH_SHORTCUT fired before FEX and DXVK existed. That gave session 20261022-193201: **exit 53**, `err:module:load_arm64ec_module could not load L"C:\\windows\\system32\\libarm64ecfex.dll", status c0000135` (raw/cube/d3d11-attempt1-exit53/). **Cause: launcher setup** (FEX missing). The driver was never loaded.
- Launcher follow-ups: (1) The shortcut and container setup gate only on Proton and imagefs, so a game can start without FEX or DXVK. It should also check FEXCore (and DXVK for D3D). (2) A download-only APK should either give FEX/DXVK/ALSA a URL or hide those Download buttons. The current message is easy to miss.

### Files
- screenshots/: cube-d3d11-01-start (all-files dialog), -02 (what's new), -03 (games list, nothing launched), -04-components (all not installed), -05-exit53 (first attempt), cube-components-installed, cube-d3d11-10-run25s / -11-run40s / -12-run60s (rendering, then session summary), cube-d3d9-01-run15s / -02-run25s (rendering) / -03-session.
- raw/cube/d3d11/ and raw/cube/d3d9/: session files (summary, config, device, session.json, wine-run.log, launcher.log, mesa-panvk.txt, DXVK logs), perf json/csv, and logcat-filtered.txt.

### Earlier attempt (before the uninstall)
`adb install -r panplay-rc5u.apk` failed: `INSTALL_FAILED_VERSION_DOWNGRADE: Update version code 33 is older than current 34`.
`adb install -r -d` failed: `INSTALL_FAILED_DUPLICATE_PERMISSION ... DYNAMIC_RECEIVER_NOT_EXPORTED_PERMISSION already owned by dev.zenithblue.panvklauncher`.
Cause: the installed PanPlay is versionName 1.2.8 / versionCode 34 (lastUpdate 2026-10-13, about 387 MB, signer cert SHA-256 8b10fde8...). panplay-rc5u.apk is 1.2.7 / vc 33 (the repo's apps/panvk-launcher value) and is **signed with a different key** (cert bc517d8c...). Android cannot update across different signing keys, so replacing it needs `adb uninstall dev.zenithblue.panvklauncher`, which wipes the installed Wine container and imagefs. I did not do that without the user's go-ahead.
To unblock, either uninstall the installed PanPlay with the user's OK, or rebuild PanPlay rc5u with versionCode >= 35, signed with the key that signed the installed 1.2.8.
For reference, the launcher's shortcut extra is `--es dev.zenithblue.panvklauncher.LAUNCH_SHORTCUT <id|name>` (MainActivity.kt:50, Shortcuts.kt:117; debug builds; waits for container setup and an idle Wine).

## Screenshots
screenshots/: all-during (autorun at 6 tests), all (end screen, 39/39), tess / tess2 / tess3 (rc5u standalone), tess-rc4 (control), tessperf1, bcperf1, bcperf2.
Raw: [raw/all/](raw/all/) holds the full autorun pull. [raw/standalone/files/runs/](raw/standalone/files/runs/) holds the standalone runs 185406, 185422, 185435 (rc4), 185441, 185555 (tess_perf), 185628 and 185701 (bc_perf); its autorun.txt is from the last bc_perf run.
