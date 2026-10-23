# g615 RC5-universal-110 regression run (2026-10-23)

Device: Poco X6 Pro (Mali-G615 MC6, arch v11), wifi adb 10.11.176.130:44087. **On battery (not charging), 71% -> 62%**, battery 31-40 C. The 108 run was at 100% and charging.
APK /home/flux/build-out/apks/panprobe-rc5u-110.apk (sha256 73bbebec...), package dev.zenithblue.panvktest.rc5u (versionCode 15). `adb install -r` OK.
The rc5 .so in the APK is `libvulkan_panfrost_rc5.so` sha256 5f217f70aa28f47a1eda50200428c92dd8e7af5c717f07d638e8ea9c6357289f (matches the build record).
DRIVER line on every 110 run: `PanVK-kbase RC5-universal-110 (Mesa 26.3.0-devel (git-6cae57f893))`.
The phone clock is IST, so the run ids are dated 2026-10-24.

Baseline: RC5-universal-108, [../2026-10-22-g615-rc5u-108/results.md](../2026-10-22-g615-rc5u-108/results.md) (autorun run=20261022-214210), plus the cube results in [../2026-10-22-g615-rc5u/results.md](../2026-10-22-g615-rc5u/results.md).
Changes since 108: csf-v11 220 (command-pool BO release on reset/trim), 221 (transient flush only up to transient_offset) and 222 (log levels), plus JM-only patches.

Method (same as 108): `am force-stop dev.zenithblue.panvktest.rc5u`, then
`am start -n dev.zenithblue.panvktest.rc5u/dev.zenithblue.panvktest.MainActivity --es driver <rc5|rc4> --es autorun <all|test>`,
wait for `EXTERNAL READY run=<id>` in `/sdcard/Android/data/dev.zenithblue.panvktest.rc5u/files/autorun.txt`, check the DRIVER line, pull `files/runs/<id>`.

**logd is stopped on the phone** (`getprop init.svc.logd` = stopped; every `adb logcat` gives "Logcat read failure: No such file or directory"). There is no logcat for this run. The driver string was checked from the PanProbe DRIVER line and, for PanPlay, from the DXVK HUD `Version:` line in the screenshots.

## Summary
- **autorun all (rc5, run=20261024-010625): 39 pass / 0 fail / 0 skip, total 39.** Same as 108. No reruns were needed.
- **The phone was about 2x faster today than on 10-22, for every driver.** The 1 s fps loops are 2.3-2.9x the 108 numbers, tess_perf per-draw times are 1.7-5x lower, and swapchain_lifecycle presents at 121 fps (120 Hz) instead of 63. The unchanged RC4 .so also gave 5686 fps on tessellation (2608 on 10-22). So the absolute numbers cannot be compared with the 108 log. The cause is the device state (on battery and not charging, 120 Hz display). It is not the driver.
- **So I ran an interleaved A/B on the same phone state:** I reinstalled panprobe-rc5u-108.apk with `install -r`, ran tessellation, bc_perf and tess_perf, then reinstalled 110 and checked its DRIVER line again. **110 is within noise of 108 everywhere:**
  - tess_perf: all 18 hashes are identical to 108. Per-draw times are within -6%..+2% of the 108 A/B (mean of 3 runs of 110 vs 1 run of 108).
  - bc_perf (warm): 110 24404 / 24806 ms (mean 24605). 108 A/B 24741 / 24705 / 24424 ms (mean 24623). That is -0.1%. Large formats are within -3..+3%.
  - tessellation: 110 5291 / 5283 standalone (6850 inside autorun). 108 A/B 5518 / 5915. RC4 control 5686. 110 is 7% below the 108 mean on 2 samples each. This 1-second loop swings 10-15% run to run (see 107/108), so this is not conclusive.
- **Memory: no regression.** tess_perf peak RSS is 860 / 890 / 877 MB on 110 vs 896 MB on the 108 A/B (and 849 MB on the 10-22 108 log). Min MemAvailable is 2.29 / 1.94 / 2.18 GB vs 2.00 GB on the 108 A/B. After teardown RSS returns to 25-28 MB on both.
- GPU timestamps are unchanged: timestampPeriod 76.9231, validBits 64. In bc_perf, upload_gpu_ms < upload_wall_ms on 26/26 BC PERF lines.
- **PanPlay 110 (1.2.9-rc5, vc 35) installed with `adb install -r` over 1.2.7 (vc 33): Success**, with no uninstall. The Drivers screen shows "PanVK Kbase — RC5-universal-110". **All 9 components show "Installed ... (bundled)".**
- **Cubes: D3D11 ARM64EC 45.9 fps, D3D9 i686 48.0 / 48.9 / 45.8 fps** (DXVK HUD). Both PASS (exit 0) and both show `Version: PanVK-kbase RC5-universal-110 (Mesa 26.3.0-devel (git-6cae57f893))`. The 10-22 RC5-universal runs gave 47.1 / 48.4, so these are the same.
- **Fallout 4: 1.7-5.4 fps in the opening scene (Sanctuary house).** It ran under 10 fps on the old RC5 too, and 110 is not better. The cause is system memory exhaustion, not FEX CPU time and not a shader stall. See the Fallout 4 section.

## autorun all (run=20261024-010625)
Full output: [raw/all/files/autorun.txt](raw/all/files/autorun.txt). Per-test logs and json: [raw/all/files/runs/20261024-010625/](raw/all/files/runs/20261024-010625/).

| test | 110 (today) | 108 (10-22, charging) |
|---|---|---|
| clip_cull | PASS 5548.8 fps | PASS 2380.6 fps |
| multi_viewport | PASS 6004.3 fps | PASS 2637.8 fps |
| fill_mode | PASS 6549.5 fps | PASS 2287.5 fps |
| geometry | PASS 6447.8 fps | PASS 2607.4 fps |
| tessellation | PASS 6850.0 fps | PASS 2569.7 fps |
| pipeline_stats | PASS 5416.5 fps | PASS 2330.1 fps |
| bc_decode | PASS 15653 ms | PASS 15737 ms |
| bc_perf | PASS 39943 ms (cold, first run after install) | PASS 43527 ms (cold) |
| tess_perf | PASS 27848 ms | PASS 47534 ms |
| large_draw | PASS 4569 ms | PASS 8219 ms |
| submit_stress | PASS 30065 ms | PASS 30105 ms |
| swapchain_lifecycle | PASS 121.0 fps (resized 120.7) | PASS 63.2 fps |
| all other 27 | PASS | PASS |

The large differences come from the phone state (see the Summary and the RC4 control). bc_decode and submit_stress are fixed-work or fixed-time and match.

## Same-state A/B: 110 vs 108 (interleaved, 2026-10-24 01:07-01:12 phone time)
Order: 110 tessellation x2, RC4 tessellation, 110 bc_perf x2, 110 tess_perf, then `install -r` 108, 108 tessellation x2, bc_perf x3, tess_perf, then `install -r` 110 and 110 tess_perf again. Every run's DRIVER line was checked.

### Tessellation (fps, 1 s loop)
| run | driver | fps |
|---|---|---|
| 010625 (inside autorun all) | 110 | 6850.0 |
| 010717 | 110 | 5291.4 |
| 010723 | RC4 control | 5685.6 |
| 010728 | 110 | 5283.3 |
| 010928 | 108 | 5517.5 |
| 010933 | 108 | 5914.6 |

### tess_perf us_draw (256 draws, 4096 patches)
110 runs: autorun 010625, standalone 010905 (s1) and 011215 (s2, after reinstalling 110). 108 A/B: 011126. The last column but one is the 10-22 108 log, recorded in the slower phone state.

| opt | gs | L | 110 autorun | 110 s1 | 110 s2 | 108 A/B today | 110 mean vs 108 A/B | 108 (10-22) | hash same |
|---|---|---|---|---|---|---|---|---|---|
| default | 0 | ref | 0.9 | 1.1 | 1.1 | 1.1 | -6% | 3.8 | yes (4807f3f5) |
| default | 0 | 1 | 95.6 | 96.5 | 96.2 | 94.0 | +2% | 491.3 | yes (a03404c5) |
| default | 0 | 4 | 304.2 | 301.2 | 311.4 | 304.1 | +0% | 541.9 | yes (cec55885) |
| default | 0 | 16 | 1422.1 | 1424.8 | 1426.6 | 1406.3 | +1% | 2552.6 | yes (d4463b35) |
| default | 0 | mix | 1468.8 | 1465.3 | 1454.2 | 1448.1 | +1% | 2521.3 | yes (b34a251c) |
| default | 1 | 1 | 133.9 | 132.3 | 128.6 | 139.0 | -5% | 665.5 | yes (a03404c5) |
| default | 1 | 4 | 433.9 | 438.0 | 441.7 | 432.2 | +1% | 912.6 | yes (cec55885) |
| default | 1 | 16 | 2262.4 | 2229.2 | 2252.5 | 2232.7 | +1% | 4486.3 | yes (d4463b35) |
| default | 1 | mix | 1880.1 | 1880.4 | 1883.5 | 1860.6 | +1% | 3505.5 | yes (b34a251c) |
| legacy | 0 | ref | 0.9 | 1.0 | 1.0 | 1.0 | -3% | 3.0 | yes (4807f3f5) |
| legacy | 0 | 1 | 107.1 | 111.5 | 111.9 | 113.2 | -3% | 549.4 | yes (a03404c5) |
| legacy | 0 | 4 | 339.5 | 355.9 | 339.9 | 342.1 | +1% | 605.1 | yes (cec55885) |
| legacy | 0 | 16 | 1643.4 | 1665.9 | 1658.1 | 1678.8 | -1% | 2962.7 | yes (d4463b35) |
| legacy | 0 | mix | 1668.1 | 1679.8 | 1680.2 | 1658.9 | +1% | 2928.3 | yes (b34a251c) |
| legacy | 1 | 1 | 134.8 | 142.1 | 143.6 | 148.8 | -6% | 699.7 | yes (a03404c5) |
| legacy | 1 | 4 | 455.5 | 468.5 | 476.6 | 458.6 | +2% | 875.6 | yes (cec55885) |
| legacy | 1 | 16 | 2447.9 | 2511.3 | 2488.6 | 2483.5 | -0% | 5012.1 | yes (d4463b35) |
| legacy | 1 | mix | 2053.2 | 2079.6 | 2032.9 | 2099.7 | -2% | 3896.6 | yes (b34a251c) |

"legacy" = `PANVK_TESS_FUSE=0,PANVK_TESS_SETUP_STEP=0,PANVK_TESS_ARENAS=1,PANVK_TESS_WINDING_INLINE=0`. The largest differences (gs=1 L=1, -5%/-6%) favour 110. The L=ref rows are about 1 us and not meaningful.

tess_perf MEM (peak rss / min MemAvailable): 110 autorun 860 MB / 2.29 GB, s1 890 MB / 1.94 GB, s2 877 MB / 2.18 GB. 108 A/B 896 MB / 2.00 GB. No memory regression from 220/221.

### bc_perf upload_gpu_ms (warm standalone)
110: 010809 / 010836 (totals 24404 / 24806 ms). 108: 011002 / 011029 / 011056 (totals 24741 / 24705 / 24424 ms).

| format | 110 mean (range) | 108 mean | diff |
|---|---|---|---|
| BC1_RGB_UNORM | 0.87 (0.86-0.88) | 0.88 | -2% |
| BC1_RGBA_SRGB | 0.87 (0.84-0.90) | 0.86 | +1% |
| BC2_UNORM | 1.63 (1.58-1.67) | 1.61 | +1% |
| BC3_UNORM | 0.99 (0.96-1.03) | 0.99 | +1% |
| BC3_SRGB | 1.00 (0.97-1.03) | 0.97 | +3% |
| BC4_UNORM | 0.32 (0.29-0.35) | 0.29 | +9% (sub-0.4 ms) |
| BC4_SNORM | 0.32 (0.29-0.34) | 0.30 | +4% |
| BC5_UNORM | 0.43 (0.41-0.45) | 0.41 | +5% |
| BC5_SNORM | 0.43 (0.41-0.46) | 0.41 | +6% |
| BC6H_UFLOAT | 1.33 (1.32-1.34) | 1.29 | +3% |
| BC6H_SFLOAT | 1.66 (1.64-1.69) | 1.68 | -1% |
| BC7_UNORM | 1.93 (1.92-1.93) | 1.99 | -3% |
| BC7_SRGB | 1.94 (1.94-1.94) | 1.93 | +1% |

The totals match (-0.1%). The BC4/BC5 differences are 0.02-0.03 ms on sub-0.5 ms formats, which is noise.

## PanPlay 110
APK /home/flux/build-out/apks/panplay-rc5u-110.apk (sha256 ccbf9109..., 389 MB, `lib/arm64-v8a/libvulkan_panfrost.so` 30522256 bytes, `assets/components/` with all 9 archives).
- `adb install -r`: **Success**. Before: 1.2.7 / vc 33 (installed 2026-10-22 19:08). After: **1.2.9-rc5 / vc 35**, firstInstallTime unchanged, so this was an in-place upgrade with app data kept. No uninstall.
- First open: a "What's new in 1.2.9-rc5" dialog (`screenshots/panplay-01-open.png`), dismissed with OK.
- **Drivers tab** (`screenshots/panplay-02-drivers.png`): installed driver "**PanVK Kbase — RC5-universal-110**", "RC5-universal-110 (Mesa 5a07217f, csf-v11 018-222 + jm-v9 + jm-v7 + jm...)", tags "Default for games" and "Bundled with app". "Probe active driver" gives `Mali-G615 MC6 api=1.4.363 driver=0x6802063` (`panplay-03-probe.png`).
  - Cosmetic: the update line says "Up to date: rc5 installed, newest release rc4". It is correct, but it reads oddly.
- **Components** (`panplay-04..08-*.png`): Rootfs bionic, Wine (Proton) 11.0-2-arm64ec, FEX 2609.1, Box64 0.4.5-2f47bdf, DXVK 3.1.1-4-arm64ec, VKD3D 3.0.1-452-g22307558, PulseAudio 17.0-2, ALSA 1.2.16.1-2 and OpenGL (Mesa Zink) 26.2.4-1 all show "**Installed v... (bundled)**". No manual .wcp step was needed.
  - Caveat: `files/contents/*` still has the directories from the manual install on 10-22 (timestamps 2026-10-22 19:13-19:57), and the versions are the same. The app recognised them as the bundled versions and did not reinstall them. The bundled-install path on a fresh install (no existing contents) was not exercised, because that needs an uninstall.
- The 10-22 "RC3 lowmem" cosmetic label is gone: the HUD and Drivers tab both say RC5-universal-110.
- 1.2.9-rc5 did not write a `files/sessions/<id>` or `files/perf/<id>` record for the cube or Fallout 4 runs (the newest records are still from 10-22). The fps numbers below are therefore taken from the DXVK HUD in the screenshots. This is a launcher regression or a setting change worth checking.

### Cubes
`am force-stop dev.zenithblue.panvklauncher`, then `am start -n dev.zenithblue.panvklauncher/.MainActivity --es dev.zenithblue.panvklauncher.LAUNCH_SHORTCUT <id>`.

| cube | result | fps (DXVK HUD) | HUD driver line | 10-22 RC5-universal |
|---|---|---|---|---|
| builtin-d3d11-arm64ec | **PASS**, `CUBE: PASS api=d3d11 frames=1198`, exit=0 | **45.9** (12 s) | `PanVK-kbase RC5-universal-110 (Mesa 26.3.0-devel (git-6cae57f893))`, Mali-G615 MC6, DXVK v3.1.1+ | 47.1 avg |
| builtin-d3d9-i686 | **PASS**, `CUBE: PASS api=d3d9 frames=1152`, exit=0 | **48.0 / 48.9 / 45.8** (9 / 14 / 19 s) | same | 48.4 avg |

The four lit cubes render correctly in both. The frame counts (1198 / 1152 over the 25 s run) match about 46-48 fps. Screenshots: `cube-d3d11-01-run12s.png` (HUD), `cube-d3d11-02-run25s.png` (back to the games list after exit), `cube-d3d9-01..03` (HUD), `cube-d3d9-04-end.png`. Logs: raw/cube/d3d11/ and raw/cube/d3d9/ (launcher run log, DXVK logs). logd was stopped, so there is no logcat.

## Fallout 4
Shortcut `gb11a0666` "Fallout4" (`/storage/emulated/0/Download/1DM/Compressed/Fallout4.exe`, x86_64, FEX "Extreme", env `DXVK_HUD=full`, 1280x720, DXVK D3D11). It survived the PanPlay upgrade. Launched with `--es dev.zenithblue.panvklauncher.LAUNCH_SHORTCUT gb11a0666` after `am force-stop`.
The game goes straight from the Vault-Tec loading screen into the saved game (the Sanctuary house, the pre-war intro). No menu presses were needed.

Timeline (UTC): launch 19:57:18. Loading screen at about 19:57:50 (103 fps, `fo4-01-start.png`). In gameplay by about 19:58:15. Sampled until 20:07. Stopped at 20:07:25 with the overlay EXIT and then `am force-stop dev.zenithblue.panvklauncher`. `pidof Fallout4.exe` was empty afterwards.
The coordinator's thermal limit arrived mid-run. The run took about 9 min in game, longer than the 1-2 min asked. It was stopped right after the first thermalservice check.

**Temperatures:** battery 37.9 C at the loading screen (PanPlay overlay). At 20:07, `dumpsys thermalservice` gave **Thermal Status 0 (NONE)**, battery 40.5 C, mtktsAP 49.7 C, gpu1 56.5 C, CPU cores 70-89 C (medium core5 88.8 C). After the stop, mtktsAP was 45.6 C. The limit is MODERATE or 42 C battery, and it was not reached. Raw: raw/fallout4/thermal-1.txt.

### FPS
| sample | scene | DXVK HUD fps | frametime min / max (HUD graph) | draws | GPU |
|---|---|---|---|---|---|
| fo4-01-start | loading screen | 103.0 | 8.1 / 47.3 ms | 28 | 100% |
| fo4-02 | house, first gameplay frame | 1.7 | 8.0 / 1776 ms | 1427 | 100% |
| fo4-03-gameplay | same view | 3.4 | 8.0 / 1151 ms | 1431 | 100% |
| fo4-04-gameplay | intro walk, hallway | 3.0 | 111 / 403 ms | 2212 | 100% |
| fo4-05-gameplay | kitchen (Nora close to camera) | 3.7 | 111 / 403 ms | 2302 | 100% |
| fo4-06-gameplay | kitchen | 5.2 | 109 / 451 ms | 1765 | 100% |
| fo4-07-exit | kitchen | 5.4 | 110 / 418 ms | 1666 | 100% |

**Gameplay fps is 1.7-5.4, typically 3-5, with frame times of at least 109 ms.** On 10-22 the old RC5-universal (soSha256 ab7eaa92) ran this game at **9.3-12.2 fps** in gameplay (30 s buckets 240-540 s of perf 20261022-231331, saved in raw/fallout4/old-rc5u-20261022/; menus and loading ran at 55-85 fps). **110 is not better. It is lower in this session, but the memory state was worse, so the comparison is not clean** (see below: MemAvailable was 150-250 MB on 10-22 and 84 MB today, with swap 100% full).

### Bottleneck: system memory exhaustion (GPU memory + game heap > RAM), not CPU/FEX
Evidence, captured during gameplay:
1. **RAM and swap are exhausted.** `top`: 7.25 of 7.5 GB used, 253 MB free, **swap 3.74 / 3.75 GB used**. `/proc/meminfo`: **MemAvailable 84 MB** (raw/fallout4/meminfo-1.txt, top-threads-1.txt). Ten seconds later swap had 2.7 MB free (top-threads-2.txt). After the game stopped, MemAvailable rose to **4.22 GB**, so the game held about 4.1 GB.
2. **The game process is mostly swapped out.** Fallout4.exe `/proc/5284/status`: VmRSS 3.07 GB, of which **RssFile 2.85 GB** (kbase GPU buffer mappings) and **RssAnon only 210 MB, with VmSwap 2.43 GB**. The game's own heap (FEX-translated x86 code and data) lives in zram (raw/fallout4/fo4-proc-status.txt).
3. **Constant thrashing.** Over about 15 s (vmstat-a/b, stat-a/b): the game took **5148 major faults** and 48.7k minor faults. The system swapped in 17.9k pages (70 MB) and refaulted 140k file pages (about 550 MB re-read from flash). kswapd stole 200k pages (about 780 MB). **35% of the game's CPU time is kernel time** (stime +470 ticks vs utime +863), and kswapd0 runs at 8% CPU.
4. **The GPU footprint is far larger than DXVK's allocations.** `dumpsys gpu`: Fallout4.exe **2.75 GB** of GPU memory (global 3.03 GB) (raw/fallout4/dumpsys-gpu.txt). The DXVK HUD at the same time shows device-local chunks of 1408 MB plus host chunks of 64 MB, which is 1.47 GB (heap 0 at 1600 MB, 93-103% of budget). **About 1.3 GB of kbase memory is not in any DXVK allocation.** Driver-internal candidates: BC shadows and raw stores (csf-v11 150/197/201/207: BC6H/BC7 raw stores are separate driver BOs), tiler heap chunks, command or descriptor pool BOs, and shader/meta BOs. A pure GPU-memory leak is unlikely, because the size stayed flat (1449-1454 MB DXVK-visible) across the samples. The kbase total was sampled only once.
5. **It is not CPU-bound (FEX).** The CPU was 72% idle (578% idle of 800%). The hottest game threads were Fallout4.exe main at 10.8-15.7%, FAudio 10-16%, dxvk-cs at 8% with CS load 8-16%, and dxvk-queue at 5%. No thread is near 100%.
6. **The GPU reports 100% busy with frame times of 109 ms or more, but at only 740 MHz.** GED `current_freqency` read OPP index 40 / 740000 kHz and `gpu_utilization` 100. CPU cores were at 70-89 C. A G615 at 100% utilisation would normally clock higher, so DVFS appears capped (thermal or power). With 1.4-2.3k draws and 60-90 render passes at 720p, 110 ms of GPU time is too much. GPU work is probably also stalled by memory: the tiler heap grows on demand, and CPU-cached BOs are flushed under reclaim.
7. **The tiler-heap thread is busy.** `Fa:panvk_heap0` (csf-v11 104/192: KBASE_IOCTL_CS_TILER_HEAP_INIT and TERM on a worker thread) used **41.5 s of CPU in about 9 min (about 8% of a core continuously)**. Every heap renewal commits and zeroes 10 x 1 MiB chunks. Under this memory pressure, each commit has to reclaim pages first.
8. No DEVICE_LOST, fault or shader-compile stall was visible. Graphics pipelines settled at 232, the shader cache hit rate was 78-100%, and the frame-time graph shows no compile spikes after the first frames. logd was stopped, so I could not capture logcat. The DXVK logs (raw/fallout4/dxvk-*.log) have only the usual QueryInterface and EDID warnings. The launcher log has no panvk warnings.

**Conclusion: the low fps is memory-bound.** The kbase GPU allocations (2.75 GB, of which about 1.3 GB is not DXVK's) plus the game's 2.6 GB heap do not fit next to the rest of the phone's 7.5 GB. The game's working set is swapped to zram and file pages are re-read constantly. The GPU also runs at a low DVFS step.

### Graphics glitches
- **Nora's head renders black / missing** (`screenshots/fo4-07-exit.png`, woman on the right): the head and hair are a flat black shape. This is probably a FaceGen texture (BC-compressed) that has not been streamed in or decoded under memory pressure, or a BC shadow transcode that is missing for that image. To tell which, rerun with more free RAM, or with `PANVK_BC_MODE=exact`.
- **Dithered (stippled) Nora close to the camera** (`fo4-05-gameplay.png`): the screen-door pattern over the right half of the frame is Fallout 4's own near-camera dither fade for actors. It is expected and not a driver bug.
- **Very bright overhead lamp and countertop highlight** (`fo4-02.png`, `fo4-04-gameplay.png`): the bloom on the fluorescent lamp and the specular highlight on the counter look strong, but they are plausible for the game's lighting. Not counted as a glitch without a reference capture.
- The rest of the geometry, textures, shadows and UI text (VDSG loading text, "GROGNAK COMIC / E) ACTIVATE" prompt) render correctly.

### Driver knob A/B
Not done. The logs did not point at a single driver knob. The bottleneck is total memory, and none of the per-game PANVK knobs is a clear memory reducer: BC_RAW=lazy and BC lowmem are already the defaults. The run also had to stop for the thermal limit.

### Suggestions for driver fixes
1. **Account for the 1.3 GB of kbase memory that is not in DXVK's allocations.** Add a `PANVK_DEBUG=mem`-style summary of BO bytes by kind (image, BC shadow, BC raw store, tiler heap, cmd pool, desc pool, shader), printed every N seconds or on SIGUSR. Run Fallout 4 with it. The two most likely culprits are the BC6H/BC7 raw stores (201/207 keep them for every readable BC6H/BC7 image, and DXVK sets TRANSFER_SRC on every texture) and the BC upload BOs.
2. **Extend `PANVK_BC_RAW=lazy` to BC6H/BC7 and 3D images** (raw blocks only after the first readback). Fallout 4 never reads textures back, so this should save the whole raw-store size.
3. **Report a smaller VkPhysicalDeviceMemoryBudget on low-RAM devices** (from MemAvailable and the kbase usage) so that DXVK caps heap 0 below 1600 MB. The HUD shows 103% of budget, so DXVK went over it.
4. **Reuse tiler heaps instead of creating and terminating one per renewal**, or lower the initial chunk count from 10 x 1 MiB. `panvk_heap0` used about 8% of a core continuously, and each HEAP_INIT commits and zeroes 10 MiB under reclaim.
5. **Mark idle driver BOs (BC raw stores, staging, trimmed cmd-pool BOs) as kbase evictable / DONTNEED** so the kernel can reclaim them instead of swapping the game's heap.
6. Outside the driver: test Fallout 4 with low texture settings (or with other apps closed) to confirm that fps scales with free RAM. Check why the GPU stays at 740 MHz while 100% busy (a thermal cap with CPU cores at 89 C?).

Raw: raw/fallout4/ (top-threads-1/2, meminfo-1, fo4-proc-status, stat-a/b, vmstat-a/b, dumpsys-gpu, thermal-1, launcher-run.log, dxvk-*.log), raw/fallout4/old-rc5u-20261022/ (perf json/csv of the 10-22 run, labelled "RC3 lowmem" by the old app but with the RC5-universal .so).

## Screenshots
screenshots/:
- PanProbe: all-during (autorun at 4 tests), all-during2 (bc_decode), all (end screen, 39/39).
- PanPlay: panplay-01-open (what's new), panplay-02-drivers (RC5-universal-110 label), panplay-03-probe, panplay-04..08 (components, all "Installed (bundled)").
- Cubes: cube-d3d11-01/02, cube-d3d9-01..04.
- Fallout 4: fo4-01-start (loading), fo4-02 / 03 (first gameplay), fo4-04..07 (intro walk; 07 shows the black head).

## Raw
- [raw/all/](raw/all/): autorun.txt and runs/20261024-010625. The autorun uploaded as id=4321e882.
- [raw/standalone/runs/](raw/standalone/runs/): 110 runs 010717 / 010728 (tessellation), 010723 (rc4 tessellation), 010809 / 010836 (bc_perf), 010905 / 011215 (tess_perf).
- [raw/ab-108/runs/](raw/ab-108/runs/): 108 runs 010928 / 010933 (tessellation), 011002 / 011029 / 011056 (bc_perf), 011126 (tess_perf).
- raw/cube/, raw/fallout4/: see above. There is no logcat (logd stopped on the phone).
