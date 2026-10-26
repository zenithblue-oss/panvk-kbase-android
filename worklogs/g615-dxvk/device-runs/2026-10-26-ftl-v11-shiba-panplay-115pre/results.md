# FTL shiba (Pixel 8, v11, Mali-G715 MC7) 115pre: PanPlay cubes A/B (scenario 4) and D3D12 (scenarios 5, 6)

All three scenarios ran.

| scenario | passes | result |
|---|---|---|
| 4 `ab` | d3d11 `old`, d3d11 `default`, d3d9 `old`, d3d9 `default` | **4/4 ok**: each `CUBE: PASS` with exit 0. No fps regression. |
| 5 `d3d12` | SquidRoom (D3D12 Multithreading sample), ARM64EC, `default` | **FAIL**: `vkCreateSwapchainKHR Exception 0xc0000005 in Unix call.`, exit 3 after 7 s |
| 6 `d3d12gears` | vkd3d-proton gears, ARM64EC, `default` | **FAIL**: the same exception, exit 3 after 6 s |

- The FTL outcome is "Passed", because the game loop only reports that the app finished. The verdicts above are the harness's own.
- logcat has no DEVICE_LOST, `event=0x`, GPU fault, CS_FATAL, Fatal signal, ANR or FATAL EXCEPTION lines.

Run details:

- Matrix `matrix-3mzs6fl64ex6k`, 2026-10-26 14:03-14:13 UTC (07:03-07:12 device time). Raw results are in
  `gs://test-lab-hrb456s6qt79m-kz4jkut67musi/panvk-v11-shiba-115pre-panplay/`.
- Command, run under the Firebase lock:

  ```
  gcloud firebase test android run --type game-loop --app gs://test-lab-hrb456s6qt79m-kz4jkut67musi/panvk-apks/panplay-115pre.apk \
    --scenario-numbers 4,5,6 --device model=shiba,version=34 --timeout 30m \
    --directories-to-pull /sdcard/Android/data/dev.zenithblue.panvklauncher/files --results-dir panvk-v11-shiba-115pre-panplay
  ```

  The APK was uploaded with gsutil (parallel composite upload). The first `gcloud` attempt crashed with a ReadTimeout
  while uploading the 414 MB APK, before any matrix was created.
- APK: `/home/flux/build-out/apks/panplay-115pre.apk`, sha256 `dc469ee7...`. Package `dev.zenithblue.panvklauncher`
  1.2.9-rc5 (vc 40). Config is `assets/gameloop.json` only, with no override.
- Driver: the bundled `.so`, sha256 `652af311e78c3846...`, 29903696 bytes
  (= `/home/flux/build-out/2026-10-26-115pre/libvulkan_panfrost.so`).
  - fps-log header and DXVK HUD `Version:` both read `Mesa 26.3.0-devel (git-6a5d4e2756)`.
  - DXVK reports `Mali-G715 MC7 (panvk 26.2.99)`. 26.2.99 is Mesa's usual `driverVersion` encoding for a 26.3.0-devel build.
- Device: Pixel 8 (shiba), Tensor G3, Android 14 (SDK 34), kbase CSF uAPI 1.24.
- Components: Proton 11.0-2 arm64ec, FEX 2609.1, DXVK 3.1.1, vkd3d-proton 3.0.1-452-g22307558, Box64 0.4.5 (WoW64).
  The Wine prefix was created fresh (wineboot 58 s).

## Cubes: old vs default (scenario 4)

- **Window:** 20 s warm-up, then 60 s measured, both counted from the first frame.
- **Primary fps (`drv-fps`):** the driver fps log (csf-v11 233, `PANVK_FPS_LOG_MS=1000`).
- **DXVK stats:** taken from the `DXVK_STATS_FILE` `perf.csv`. Frame-time percentiles are in ms.
- **`drawn`:** the SurfaceFlinger compositor rate.

| # | task | env | drv-fps | 1 s min / max | max frame ms | DXVK avg | 1% low | 0.1% low | p50 / p95 / p99 ms | stutters | drawn | frames | first frame |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | d3d11 | old | **46.8** | 44.3 / 51.2 | 42.9 | 46.8 | 32.4 | 27.1 | 21.7 / 27.0 / 29.0 | 0 | 46.8 | 4128 | 17.2 s (cold: first run in the new prefix) |
| 2 | d3d11 | default | **47.5** | 43.9 / 50.8 | 56.5 | 47.5 | 32.7 | 28.0 | 21.9 / 27.3 / 28.9 | 0 | 47.5 | 4171 | 4.3 s |
| 3 | d3d9 | old | **49.7** | 45.8 / 52.9 | 108.8 | 49.7 | 27.0 | 12.5 | 22.0 / 28.2 / 30.7 | 2 | 49.6 | 4351 | 6.2 s |
| 4 | d3d9 | default | **48.9** | 41.5 / 51.9 | 185.8 | 48.9 | 26.5 | 11.3 | 22.0 / 27.9 / 29.8 | 1 | 48.8 | 4129 | 6.3 s |

The A/B lines below are from `results.txt`:

- **A/B d3d11:** old 46.8 fps, default 47.5 fps (**+1.4%**).
- **A/B d3d9:** old 49.7 fps, default 48.9 fps (**-1.7%**).

Both deltas are inside the 1-second spread of a single pass (about 44-52 fps), so the 21 default-on 115 knobs neither help
nor hurt the cube.

- The cube is paced by its own 10 ms message pump. It runs at about 47-50 fps at a p50 of about 22 ms, with roughly 11 ms of real work per frame.
- The d3d9 max frame times (109 and 186 ms) are 1-2 single hitches, which DXVK counts as stutters. They appear in both env sets.
- `old` always runs before `default`, so `default` runs on a warmer device.
- The DXVK HUD (`DXVK_HUD=full`) stays on in all four passes.
- DXVK GPU load: 31% / 23% / 27% / 27%.
- In the harness screenshots, taken 50 s after the first frame, the HUD shows `FPS: 47.8` (d3d11 old) and `FPS: 45.5` (d3d11 default).
  These are instantaneous HUD values, not the window averages.

**The env sets reached the driver.**

- **P-A registry line.** Only the two `old` passes log `panvk: 114 behaviour restored for: PANVK_CSF_FS_IDLE_CLEAN ... PANVK_PRELOAD_BARRIER_TRACK`:
  - 4 lines in total, at 07:05:16 and 07:05:18 (d3d11) and at 07:08:36 and 07:08:37 (d3d9);
  - each line names 12 knobs;
  - the pb knobs do not log; they are in the pass env (`result.json` `env`, 21 `PANVK_*` keys).
- **P-A 0008 coherency line.** The `old` passes print `shaderDeviceClock on (114 rule, coherency mode != 0)` (8 lines). Every
  `default` process, including the D3D12 ones, prints `shaderDeviceClock off (strict, PANVK_KBASE_STRICT_DEVICE_CLOCK=0 = 114)` (27 lines).
  The coherency mode is 31 (none), as in the PanProbe run.

### Finding: the DXVK HUD text is garbled in both d3d9 passes

In `panplay-d3d9-old-harness.png` the HUD glyphs are partly broken. In `panplay-d3d9-default-harness.png` most of them are missing.

Everything else on screen is correct:

- the four lit cubes and the pixel check (`CUBE: PASS ... (check pixels)`);
- the HUD frame-time graph;
- both D3D11 HUDs.

The video frames show the same thing, so the harness capture is not the cause. The earlier G615 (v10) runs drew a clean D3D9 HUD:

- `2026-10-22-g615-rc5u`;
- `2026-10-23-g615-rc5u-110`.

The `old` set has the garbling too, so it is not one of the 21 knobs. There is no 114 PanPlay run on shiba to compare with, so
it is open whether this is new in 115pre or specific to v11 with the i686 DXVK (WoW64 through FEX).

The d3d9 fps numbers above come from the driver log and DXVK stats, not from the HUD.

To decide, run scenario 2 on shiba with `/home/flux/build-out/apks/panplay-rc5u-114fps-gameloop.apk` (114 + 233) and compare the HUD.

## D3D12 (scenarios 5 and 6): FAIL, crash in the first vkCreateSwapchainKHR

Both tests have the same `wine-run.log` tail: SquidRoom (s5) shows the lines below, and gears (s6) shows the same exception
on thread `0134`, with no `Present` line.

```
DX12: start test=d3d12-multithreading secs=88 sync=0 1280x720
...
DX12: first frame startup=3.8
DX12: Present frame=1 hr=0x00000000
DX12: Present frame=2 hr=0x00000000
0138:err:vulkan:vkCreateSwapchainKHR Exception 0xc0000005 in Unix call.
0144:err:sync:RtlLeaveCriticalSection section 0000006FFF9069B8 "dlls/msvcrt/lock.c: LOCKTABLEENTRY.crit" is not acquired
exit=3
```

| | s5 SquidRoom | s6 gears |
|---|---|---|
| harness verdict | fail: no pass line (`^DX12: PASS`), exit 3 | same |
| wall | 7.1 s (the app's first Present at 6.3 s) | 5.9 s, no first frame |
| fps | none: the driver fps log was never created, so the driver presented nothing | none |
| screen | black X window under the PanPlay overlay (`panplay-d3d12-*.png`) | black, then the harness log |

What works up to the crash:

- **vkd3d-proton device setup.** Its log has only warn, fixme and info lines:
  - SM 6.6-6.8 enabled, `VK_EXT_mutable_descriptor_type`, UMA topology;
  - `Optional user instance extension "VK_KHR_surface_maintenance1" is not supported` (Wine exposes the EXT version);
  - `TotalLaneCount = 512, may be inaccurate`;
  - no WMMA.
- **DXGI swapchain object.** `dxgi_vk_swap_chain_init: Creating swapchain (1280 x 720), BufferCount = 3` and
  `Ensure maximum latency of 3 frames with KHR_present_wait`.
- **Asset load.** SquidRoom loaded its 43 MB asset, uploaded the BC1 textures and built its PSOs (startup 3.8 s).
- **No errors elsewhere.** Mesa logged no WSI errors, and nothing in the `vkd3d.log`, `mesa-panvk.txt` or `logcat.txt` session logs points at the fault.

Analysis (no new run was made):

1. **Where it faults.** vkd3d-proton creates the `VkSwapchainKHR` lazily, in its present task on the queue thread, the first time
   it presents (`libs/vkd3d/swapchain.c:2243` at 22307558). The two app-side Presents only queued work. The very first
   `vkCreateSwapchainKHR` raised SIGSEGV on the Unix side of the call, which can be any of:
   - winevulkan/win32u;
   - winex11's surface update;
   - PanVK's X11 sw WSI (`x11_surface_create_swapchain`).

   Wine turned it into status `c0000005` and the process aborted with exit 3. That is msvcrt's `abort()` code, most
   likely from the winevulkan loader's check on the Unix-call status.
2. **How this differs from DXVK, which works in the same container.**

   | | DXVK (D3D11 cube, works) | vkd3d-proton (fails) |
   |---|---|---|
   | where the swapchain is created | app thread | vkd3d's queue thread |
   | images | 4 | 3 (`max(3, minImageCount)`) |
   | usage | | `COLOR_ATTACHMENT \| TRANSFER_DST` |
   | present mode | `IMMEDIATE (dynamic: yes)` | FIFO first, with `VkSwapchainPresentModesCreateInfoEXT` {FIFO, IMMEDIATE/MAILBOX} when the surface reports a compatible unlocked mode |

   Present id2/wait2 and present timing are not in play: Wine reports `presentId2 = presentWait2 = 0`.
3. **No baseline.** This is the first D3D12 presentation test on any driver. The 2026-10-08 `d3d12probe` only covered
   device, queue, command list and fence, without a swapchain. The 114 PanPlay APKs (vc 39) have no D3D12 tasks.
   So this cannot be called a 115 regression yet.
4. **The suspect 115 change was not A/B'd.** P-C 0003 and 0004 changed the X11 sw WSI (`PANVK_WSI_SW_CACHED`, `PANVK_WSI_SW_DEFER_REPLY`),
   but the D3D12 passes ran only with `default`.

What a rerun needs (not launched; to be rerun later):

- **Override file.** Use the APK as it is, plus an override file deep-merged over `assets/gameloop.json`. Push it with
  `--other-files /sdcard/Android/data/dev.zenithblue.panvklauncher/files/gameloop.json=<file>`. That is the app's own
  external dir, which it can read on API 34. Under scoped storage the app may not be able to read `/data/local/tmp` or the
  `/sdcard` root.
- **Scenarios.** The APK declares `com.google.test.loops = 6`, so redefine scenarios 5 and 6. Gears fails in 6 s, so
  several env sets fit in one scenario.
- **Env sets to add:**
  - `old`: the 114 knobs, which covers P-C's X11 changes;
  - `seh`: `WINEDEBUG=fixme-all,+seh,+loaddll`, to get the faulting PC and the module;
  - `nomaint1`: `VKD3D_DISABLE_EXTENSIONS=VK_KHR_swapchain_maintenance1,VK_EXT_swapchain_maintenance1`;
  - `nowait`: `VKD3D_DISABLE_EXTENSIONS=VK_KHR_present_wait,VK_KHR_present_id`;
  - optionally `VKD3D_SWAPCHAIN_IMAGES=4`.
- **Example:** `{"envSets": {"seh": {"note": "...", "env": {...}}, ...}, "scenarios": {"6": {"name": "d3d12gears-diag", "tasks": ["d3d12gears"], "envSets": ["old", "seh", "nomaint1", "nowait", "default"]}}}`.
  Run `--scenario-numbers 6`, then `5` if gears passes in any set.
- **114 baseline.** It needs a PanPlay build with the 114 `.so` and the current assets (`-PpanvkSo=<114 so>`), because the
  old 114 APKs do not have the D3D12 tests.

## Files

- **`raw/shiba-34-en-portrait/`.** It holds:
  - `logcat` (8.7 MB);
  - `game_loop_results/results_scenario_{4,5,6}.json`;
  - `artifacts/sdcard/Android/data/dev.zenithblue.panvklauncher/files/gameloop/` with `20261026-070315-s4/`,
    `20261026-071149-s5/` and `20261026-071158-s6/`. Each pass folder has `result.json`, `panvk-fps-<pid>.log`,
    `perf.csv`, `perf-sys.csv` and `screen.png`, plus the `session/` logs (`wine-run.log`, `dxvk-*.log`, `vkd3d.log`,
    `xserver.log`, `logcat.txt`, `mesa-panvk.txt`, `config.txt`).
- **The FTL video is not committed.** It is 77.7 MB (earlier runs' videos were 0.3-7 MB). Copies:
  - `gs://test-lab-hrb456s6qt79m-kz4jkut67musi/panvk-v11-shiba-115pre-panplay/shiba-34-en-portrait/video.mp4`;
  - `/home/flux/build-out/ftl-video/2026-10-26-ftl-v11-shiba-panplay-115pre.mp4`.

  Video time 0 is the first `TEST_LOOP` START in logcat (07:03:15.069), and it runs for 531 s.

## Screenshots (`screenshots/`)

- **Video frames** are 540x1200 portrait, with the landscape game in a band across the middle. The HUD is drawn but is too
  small to read at that size. Video frames are at offset = logcat time - 07:03:15.069.
- **`*-harness.png`** are the app's own 2400x1080 captures, taken 50 s after the first frame, and the HUD is readable in them.

| file | what it shows |
|---|---|
| `video-start.png` | 07:03:45: game-loop log screen, scenario 4 (ab), 4 passes, installing the bundled imagefs rootfs (50%) |
| `video-mid.png` | 07:07:40: d3d11 `default` pass, four lit cubes and the DXVK HUD |
| `video-end.png` | about 07:12:03: d3d12gears pass, black X window under the overlay buttons (nothing presented before the crash) |
| `panplay-d3d11-old-mid.png` | 07:06:00 (first frame + 42 s): d3d11 `old`, cubes and HUD (pass average 46.8 fps) |
| `panplay-d3d11-default-mid.png` | 07:07:40 (+43 s): d3d11 `default`, cubes and HUD (47.5 fps) |
| `panplay-d3d9-old-mid.png` | 07:09:20 (+42 s): d3d9 `old`, cubes, HUD text partly garbled (49.7 fps) |
| `panplay-d3d9-default-mid.png` | 07:11:00 (+41 s): d3d9 `default`, cubes, HUD text mostly missing (48.9 fps) |
| `panplay-d3d11-old-harness.png` | harness capture at +50 s: HUD `Version: Mesa 26.3.0-devel (git-6a5d4e2756)`, `FPS: 47.8`, GPU 25% |
| `panplay-d3d11-default-harness.png` | harness capture at +50 s: same HUD, `FPS: 45.5`, GPU 24% |
| `panplay-d3d9-old-harness.png` | harness capture at +50 s: cubes fine, HUD glyphs partly corrupted, fps not legible |
| `panplay-d3d9-default-harness.png` | harness capture at +50 s: cubes fine, HUD text mostly missing (graph intact), fps not legible |
| `panplay-d3d12-squidroom-black.png` | 07:11:55.6: SquidRoom just after its first app-side Present, black window (no VkSwapchain yet) |
| `panplay-d3d12-squidroom-after-crash.png` | 07:11:57.6: right after the vkCreateSwapchainKHR crash (exit 3), still black |
| `panplay-d3d12gears-black.png` | 07:12:03.8: gears, black window, nothing presented |
| `panplay-d3d12gears-fail-harness-log.png` | 07:12:05.8: harness log view after the gears failure (`pass 1/1: d3d12gears ... env set default`) |
