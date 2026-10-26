# FTL shiba (Pixel 8, v11, Mali-G715 MC7) 115fix: confirm run for the cube fps cap, D3D9 HUD text and D3D12 swapchain fault

This run checks the fixes from the diag run (`../2026-10-26-ftl-v11-shiba-panplay-115fix-diag/results.md`): PanPlay 03cd94c + 871368a and
the driver series `patches/staging-115/fix/` (p115-fix `de6b5b2d`). It ran the same tests on the 114 driver for comparison.

| problem | before | this run (115fix, defaults) | 114 driver, same run |
|---|---|---|---|
| P1 cube fps cap | d3d11 46.8 fps, DXVK GPU 21.9%; d3d9 49.2 fps, GPU 28.0% (diag, old pump) | **d3d11 427.6 fps, GPU 78.5%**; **d3d9 118.8 fps (vsync at 120 Hz), GPU 33.0%** | d3d9 120.0 fps, GPU 33.0% |
| P2 D3D9 (i686) HUD text | missing or garbled | **clean** in every sample of the pass | clean |
| P3 D3D12 swapchain fault | both tests: `vkCreateSwapchainKHR Exception 0xc0000005`, exit 3 | **SquidRoom `DX12: PASS ... fps=78.2`, gears `DX12: PASS ... fps=182.2`**, both render | gears: the same exception, exit 3 |

The same `.so` with `PANVK_KBASE_UNCACHED_HEAP=0` (the 115 cached heap) still garbles the D3D9 HUD.

## Run details

- Matrix `matrix-33ltrfa55a2v0`, 2026-10-26 19:41-19:51 UTC (12:42-12:49 device time), test time 406 s. FTL outcome Passed
  (the game loop only reports that the app finished; the verdicts below come from the harness).
- Raw results: `gs://test-lab-hrb456s6qt79m-kz4jkut67musi/panvk-v11-shiba-115fix-panplay/`, pulled into `raw/`.
  Not committed:
  - `video.mp4` (47 MB, in GCS);
  - the pushed 114 driver `files/rc5u-114fps.so`. Its sha256 is `51f6fdbe...`, the same as
    `/home/flux/build-out/2026-10-26-rc5-universal-114-fps-private/libvulkan_panfrost.so`.
- Command (under `flock /home/flux/build-cache/firebase.lock`):

  ```
  gcloud firebase test android run --project phonefinder-hub --type game-loop \
    --app gs://test-lab-hrb456s6qt79m-kz4jkut67musi/panvk-apks/panplay-115fix.apk \
    --scenario-numbers 1,2,3,4 --device model=shiba,version=34 --timeout 30m \
    --other-files /sdcard/Android/data/dev.zenithblue.panvklauncher/files/gameloop.json=gameloop-override.json,\
  /sdcard/Android/data/dev.zenithblue.panvklauncher/files/rc5u-114fps.so=<114-fps .so> \
    --directories-to-pull /sdcard/Android/data/dev.zenithblue.panvklauncher/files --results-dir panvk-v11-shiba-115fix-panplay
  ```

- APK: `/home/flux/build-out/apks/panplay-115fix.apk`, sha256 `10161ee3d5b493ece069d515aa2bef4ca381aadf8b05149d14cb333b3c4b767f`.
  It is PanPlay 871368a (1.2.9-rc5, vc 40) and bundles the 115fix driver.
- **Driver: the bundled 115fix `.so`.**
  - sha256 `42d91d26f6ca4ec975c60c3c8e26c1f2b68b97ee96742d2994255c808538e3ca`, GNU build-id `3995975f...`.
  - driverInfo `Mesa 26.3.0-devel (git-de6b5b2d23)` in `results.json` and on the HUD `Version:` line.
  - `bundled-driver.json` still names 115pre (`git-6a5d4e2756`), so the harness `Driver:` line and `perfSummary.driver*` show that label.
- **114 driver: RC5-universal-114-fps** (114 + csf-v11 233 fps log), pushed next to `gameloop.json`.
  - Each scenario logs `installDrivers r114: 30674016 bytes, sha256 51f6fdbe...`.
  - In the `r114` passes:
    - the ICD json points at `files/drivers/r114/libvulkan_panfrost.so`;
    - `results.json` has `driver.sha256 51f6fdbe...` and driverInfo `PanVK-kbase RC5-universal-114-fps (Mesa 26.3.0-devel (git-3071c6dfa2))`;
    - the Wine processes log no `kbase: coherency mode` line, which only 115 prints.
- Config: `gameloop-override.json` (this directory, sha256 `9943fd55...`, equal to the pulled `files/gameloop.json`).
  - Window: 8 s warm-up, 20 s measured, 4 s tail, screenshot at +22 s.
  - Env sets: `createlog` = `MESA_VK_WSI_CREATE_LOG=1`; `heap115` = `PANVK_KBASE_UNCACHED_HEAP=0`; `r114` = driver `r114`.

## Passes

Column meanings:
- `fps`: the driver fps log over the measured window.
- DXVK columns: from `DXVK_STATS_FILE`.
- `drawn`: frames per second the X server composited.
- `pump`: the cube's mean pump time per frame.
- HUD: the harness capture at +22 s.

| s | pass | status | fps | DXVK avg | p50 / p95 / p99 ms | DXVK GPU % | max frame ms | drawn | pump ms | HUD | test line |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | 01-d3d11-default | ok | **427.6** | 427.2 | 2.1 / 3.7 / 6.3 | **78.5** | 24.8 | 59.2 | 0.10 | clean, `FPS: 435.5`, `GPU: 80%` | `CUBE: PASS api=d3d11 frames=13792` |
| 1 | 02-d3d9-default | ok | **118.8** | 118.8 | 8.1 / 13.1 / 24.6 | **33.0** | 43.3 | 60.2 | 0.41 | **clean**, `FPS: 119.3`, `GPU: 32%` | `CUBE: PASS api=d3d9 frames=3718` |
| 2 | 01-d3d12-createlog (SquidRoom) | ok | 79.3 | | | | 35.0 | 60.2 | | | `DX12: PASS test=d3d12-multithreading frames=2505 secs=32.0 fps=78.2 startup=4.2` |
| 2 | 02-d3d12gears-createlog | ok | 183.9 | | | | 39.8 | 60.2 | | | `DX12: PASS test=vkd3d-gears frames=5832 secs=32.0 fps=182.2 startup=3.2` |
| 3 | 01-d3d9-heap115 | ok | 119.8 | 119.8 | 8.3 / 12.0 / 16.8 | 33.1 | 47.0 | 60.2 | 0.21 | **garbled** | `CUBE: PASS api=d3d9 frames=3407` |
| 3 | 02-d3d9-r114 | ok | 120.0 | 120.0 | 8.3 / 12.2 / 18.0 | 33.0 | 31.4 | 60.2 | 0.33 | clean, `FPS: 119.8`, `GPU: 33%` | `CUBE: PASS api=d3d9 frames=3270` |
| 4 | 01-d3d12gears-r114 | fail | - | | | | | | | | `vkCreateSwapchainKHR Exception 0xc0000005 in Unix call`, exit 3 after 9 s |

## P1: cube fps

| cube | before (diag, old pump) | after (this run) |
|---|---|---|
| d3d11 (ARM64EC) | 46.8 fps, pump 20.40 ms per frame, DXVK GPU 21.9% | **427.6 fps**, pump 0.10 ms, DXVK GPU **78.5%** (HUD: `FPS: 435.5`, `GPU: 80%`) |
| d3d9 (i686, WoW64) | 49.2 fps, pump 19.31 ms, GPU 28.0% | **118.8 fps**, pump 0.41 ms, GPU **33.0%** (HUD: `FPS: 119.3`, `GPU: 32%`) |

- The 115pre run showed the same cap with the HUD at `GPU: 24-25%`.
- The d3d9 cube keeps the D3D9 default present interval (vsync). DXVK presents FIFO, paced at the 120 Hz refresh, so 120 fps is
  that cube's ceiling. The d3d11 cube presents with sync interval 0.
- The 20 ms CSF wait slice was ruled out in the diag run with `kbase: wait-stats`:
  - `kcpu=0`;
  - the present thread's fence wait stays at or under 11.75 ms.

## P2: D3D9 HUD text

- **115fix default (system-uncached heap):** the HUD is clean in the +22 s capture (`screenshots/d3d9-default-hud.png`). It is also
  clean in all 11 video samples, taken every 3 s across the pass (`screenshots/video-hud-d3d9-default.png`).
- **The same `.so` with `PANVK_KBASE_UNCACHED_HEAP=0` (the 115 cached system heap):**
  - the glyphs are broken from the first frame to the end, and look the same in every sample;
  - the cubes, the frame-time graph and the memory bars are drawn correctly;
  - captures: `screenshots/d3d9-heap115-hud.png`, `screenshots/video-hud-d3d9-heap115.png`.
- **114: the HUD is clean** in the capture and in all 11 samples (`screenshots/d3d9-r114-hud.png`,
  `screenshots/video-hud-d3d9-r114.png`). So 114 does not show it on Pixel 8.
- **Why 114 is clean is not identified.**
  - The code these BOs take is the same in 114 and 115:
    - 091 placed maps;
    - `kbase_kmod_bo_alloc_dmabuf()` and `kbase_kmod_import_dmabuf()`, untouched by staging-115. p115-int-base is the 114-fps tree.
  - DXVK sees the same memory types: its two logs differ only in the budget line.
  - The 115pre run's `old` set (all 21 default-on staging-115 knobs at their 114 values) was still garbled.
  - So 114 has the same missing cache clean, and a 115 change that has no knob changes whether the CPU's lines reach memory
    before the GPU reads them.
  - Only the glyphs break, and the breakage stays the same for the whole run. That fits the font texture: DXVK uploads it once,
    from a staging buffer, when it creates the HUD. A stale copy then stays for the run. This is an inference; no log shows it.
  - The uncached heap does not depend on that timing.
- The `Mapped:` line overlapping the graph's `min`/`max` labels is DXVK's D3D9 HUD layout. It looks the same on 114.

## P3: D3D12 swapchain

- **Create log** (`MESA_VK_WSI_CREATE_LOG=1`), for both tests and both creates:
  - `wsi: CreateSwapchain enter platform=4 minImages=3 format=44 extent=1280x705 usage=0x12 mode=2 flags=0xc0 old=0x0 pNext: 1000275002`,
    then `platform create result=0` (gears: extent 1280x707).
  - The flags are now 0xc0 (`PRESENT_ID_2 | PRESENT_WAIT_2`) instead of 0x2c0: the CPU copy present path no longer reports
    present timing, so vkd3d-proton does not ask for it. No RandR request is sent.
- **SquidRoom** (D3D12 Multithreading sample): `DX12: PASS ... fps=78.2`, driver fps 79.3, exit 0. It renders the scene
  (`screenshots/d3d12-squidroom-harness.png`).
- **gears:** `DX12: PASS ... fps=182.2`, driver fps 183.9, exit 0 (`screenshots/d3d12gears-harness.png`).
  - The diag run's 215.3 fps came from `g-nowait`, which ran without present id/wait and with `+seh`. The two numbers are not
    comparable.
- **114:** gears fails the same way (`vkCreateSwapchainKHR Exception 0xc0000005 in Unix call`, exit 3 after 9 s). The fault is in
  114 too, as expected from the code: present timing on X11 is already in p115-int-base.

## Screenshots

| file | what |
|---|---|
| `screenshots/d3d11-default-harness.png` | d3d11 cube: `FPS: 435.5`, `GPU: 80%`, `Version: Mesa 26.3.0-devel (git-de6b5b2d23)` |
| `screenshots/d3d9-default-harness.png` | d3d9 cube with the 115fix defaults: HUD clean, `FPS: 119.3`, `GPU: 32%` |
| `screenshots/d3d9-default-hud.png` | close-up of that HUD (crop 860x1000 of the 2400x1080 capture) |
| `screenshots/d3d9-heap115-harness.png`, `-hud.png` | `PANVK_KBASE_UNCACHED_HEAP=0`: HUD glyphs garbled |
| `screenshots/d3d9-r114-harness.png`, `-hud.png` | 114 driver: HUD clean, `Version: PanVK-kbase RC5-universal-114-fps (...)`, `FPS: 119.8` |
| `screenshots/video-hud-d3d9-{default,heap115,r114}.png` | HUD crop from the FTL video every 3 s across each d3d9 pass (11 samples) |
| `screenshots/d3d12-squidroom-harness.png` | SquidRoom rendering (D3D12) |
| `screenshots/d3d12gears-harness.png` | gears rendering (D3D12) |
| `screenshots/video-{start,mid,end}.png` | FTL video at 30 s, the middle and the end |
