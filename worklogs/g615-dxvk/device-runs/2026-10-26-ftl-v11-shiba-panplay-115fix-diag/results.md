# FTL shiba (Pixel 8, v11, Mali-G715 MC7) 115fix-diag: PanPlay cube fps cap, D3D9 HUD text, D3D12 swapchain fault

One diagnostic run for the three PanPlay problems. Each one has a root cause:

| problem | root cause | fix |
|---|---|---|
| P1 cubes at 47 / 49 fps, HUD GPU about 24% | the cube's own message pump: `pump(10)` waited 10 ms by `GetTickCount` in `Sleep(5)` steps. Wine's tick moves about every 16 ms when idle, so the pump took **20.4 ms of a 21.4 ms frame** (render 0.2 ms, Present 0.6 ms) | PanPlay 03cd94c: the pump only drains the queue. **d3d11 46.8 -> 418.6 fps, DXVK GPU 22% -> 77%**. d3d9 49.2 -> 120.0 fps: there the D3D9 default present interval (vsync, FIFO paced at the 120 Hz refresh) is the limit |
| P2 D3D9 (i686, WoW64) HUD text garbled or missing | 32-bit winevulkan maps host-visible memory with `VK_EXT_map_memory_placed` ("Using placed map with alignment 4096"). Under 091 that memory is a dma-buf from `/dev/dma_heap/system`, mapped cached. The GPU has coherency mode 31 (none) and kbase drops `COHERENT_SYSTEM` from the import, so the CPU's HUD vertex writes sit in the CPU cache | driver: HOST_COHERENT dma-heap BOs from `/dev/dma_heap/system-uncached` (write-combined). Both A/B candidates gave clean text at the same fps |
| P3 D3D12 (SquidRoom, gears) `vkCreateSwapchainKHR Exception 0xc0000005 in Unix call`, exit 3 | vkd3d-proton asks for `VK_SWAPCHAIN_CREATE_PRESENT_TIMING_BIT_EXT`. After Mesa's X11 create succeeded, `wsi_CreateSwapchainKHR` polled the refresh rate through RandR. PanPlay's X server has no RANDR, so libxcb closed the connection and the NULL reply was dereferenced: **SIGSEGV at `libxcb-randr.so` `xcb_randr_get_screen_resources_current_crtcs_end+0x10`, fault address 0x10** | driver: no RandR without RANDR, and no present timing on the CPU copy present path. Only the env set that removed the timing flag rendered (gears 215 fps, PASS) |

The fixes are in `patches/staging-115/fix/` (p115-fix) and PanPlay 03cd94c. They are checked in the confirm run
`../2026-10-26-ftl-v11-shiba-panplay-115fix/`.

## Run details

- Matrix `matrix-p5cojiyld9oaa`, 2026-10-26 18:36-18:50 UTC (11:36-11:48 device time), test time 751 s. FTL outcome Passed
  (the game loop only reports that the app finished; the verdicts here are the harness's).
- Raw results: `gs://test-lab-hrb456s6qt79m-kz4jkut67musi/panvk-v11-shiba-115fix-diag-panplay/`, pulled into `raw/`.
  Not committed: `video.mp4` (86 MB, in GCS), the pushed 114 driver copy, and the 15.7 MB `launcher.log` of gears `g-nowait`
  (158,739 lines, mostly `+seh` traces; its `wine-run.log` is committed).
- Command (under `flock /home/flux/build-cache/firebase.lock`):

  ```
  gcloud firebase test android run --project phonefinder-hub --type game-loop \
    --app gs://test-lab-hrb456s6qt79m-kz4jkut67musi/panvk-apks/panplay-115fix-diag.apk \
    --scenario-numbers 1,2,3,4 --device model=shiba,version=34 --timeout 45m \
    --other-files /sdcard/Android/data/dev.zenithblue.panvklauncher/files/gameloop.json=gameloop-override.json,\
  /data/local/tmp/panplay-gameloop.json=gameloop-override.json,\
  /sdcard/Android/data/dev.zenithblue.panvklauncher/files/drivers/rc5u-114fps.so=<114-fps .so> \
    --directories-to-pull /sdcard/Android/data/dev.zenithblue.panvklauncher/files --results-dir panvk-v11-shiba-115fix-diag-panplay
  ```

- APK `/home/flux/build-out/apks/panplay-115fix-diag.apk`, sha256 `0c39f21665550d3a2b71817a08300fec92da2347fc5fdb26bfddc8ac5eeb5696`
  (PanPlay 03cd94c: drain pump, `DXCUBE_PUMP=old`, X server timing, `installDrivers`, `mapsSnapshotSec`).
- Driver: the bundled diagnostic `.so`, sha256 `1194cfe80defd44e84d699dbac46bba4fbc5f6f88904ad2abf192748bd5c6560`,
  driverInfo `Mesa 26.3.0-devel (git-e519b567f0)` (p115-pd + the opt-in timing, wait-stats, uncached-heap and create-log commits).
  `bundled-driver.json` still names 115pre (`git-6a5d4e2756`); `results.json` records the real sha256.
- Config: `gameloop-override.json` (this directory), read from the external files dir (`configSource` sha256 `b6f1d21c...`).
  Window 8 s warm-up + 20 s measured + 4 s tail, screenshot at +22 s, `/proc/<pid>/maps` every second.
- **The 114 driver never loaded.** The pushed `files/drivers/rc5u-114fps.so` exists on the device (it came back in the pull), but
  the app saw no file there (`installDrivers r114: ... not found`), so `drv114` and `g-114` ran the bundled diagnostic driver.
  A subdirectory created by the push is not visible to the app; the confirm run pushes the `.so` next to `gameloop.json`.

## Passes

`fps` is the driver fps log (csf-v11 233) over the measured window, DXVK columns from `DXVK_STATS_FILE`, `drawn` is the
X server's composited frames per second, `pump` the cube's mean pump time per frame (`CUBE: timing`).

| s | pass | status | fps | DXVK avg | p50 / p95 / p99 ms | DXVK GPU % | max frame ms | drawn | pump ms | test line |
|---|---|---|---|---|---|---|---|---|---|---|
| 1 | 01-d3d11-c-old-t | ok | 46.8 | 46.8 | 21.9 / 27.9 / 38.0 | 21.9 | 72.8 | 46.8 | 20.40 | CUBE: PASS api=d3d11 frames=1494 |
| 1 | 02-d3d11-c-drain-t | ok | 430.1 | 428.9 | 2.0 / 3.9 / 7.5 | 79.1 | 41.2 | 57.5 | 0.11 | CUBE: PASS api=d3d11 frames=13740 |
| 1 | 03-d3d11-c-drain-swold-t | ok | 125.3 | 124.7 | 7.8 / 10.8 / 17.9 | 30.4 | 51.2 | 60.1 | 0.18 | CUBE: PASS api=d3d11 frames=4015 |
| 1 | 04-d3d11-c-drain | ok | **418.6** | 419.9 | 2.1 / 3.9 / 8.1 | **76.9** | 26.0 | 58.7 | 0.11 | CUBE: PASS api=d3d11 frames=13516 |
| 2 | 01-d3d9-c-old-t | ok | 49.2 | 49.2 | 22.1 / 28.7 / 37.7 | 28.0 | 55.9 | 49.1 | 19.31 | CUBE: PASS api=d3d9 frames=1541 |
| 2 | 02-d3d9-c-drain-t | ok | **120.0** | 120.0 | 8.2 / 12.6 / 21.5 | **31.6** | 31.2 | 60.1 | 0.24 | CUBE: PASS api=d3d9 frames=3421 |
| 2 | 03-d3d9-nc-uncached | ok | 119.5 | 119.6 | 8.3 / 12.9 / 21.7 | 32.3 | 59.2 | 60.2 | 0.36 | CUBE: PASS api=d3d9 frames=3795 |
| 2 | 04-d3d9-nc-heapenv | ok | 116.9 | 117.1 | 7.8 / 17.8 / 25.8 | 31.2 | 75.3 | 60.1 | 0.33 | CUBE: PASS api=d3d9 frames=3729 |
| 2 | 05-d3d9-nc-clean | ok | 119.8 | 119.8 | 8.2 / 12.1 / 19.4 | 31.9 | 44.3 | 60.2 | 0.22 | CUBE: PASS api=d3d9 frames=3813 |
| 2 | 06-d3d9-drv114 (ran the diag driver) | ok | 119.6 | 119.6 | 8.3 / 11.9 / 18.8 | 32.5 | 43.2 | 60.2 | 0.23 | CUBE: PASS api=d3d9 frames=3414 |
| 2 | 07-d3d9-vulkan9 (12 s, `+vulkan`) | ok | 120.7 | 119.6 | 8.3 / 11.7 / 17.7 | 33.3 | 29.1 | - | 0.16 | CUBE: PASS api=d3d9 frames=1016 |
| 3 | 01..09 gears, every env set but `g-nowait` | fail | - | | | | | | | `vkCreateSwapchainKHR Exception 0xc0000005`, exit 3 after 5-9 s |
| 3 | 04-d3d12gears-g-nowait | fail (harness, see below) | 213.9 | | | | 48.6 | 60.1 | | `DX12: PASS test=vkd3d-gears frames=6891 secs=32.0 fps=215.3 startup=2.7`, exit 0 |
| 4 | 01-d3d12-g-seh, 02-d3d12-g-immediate (SquidRoom) | fail | - | | | | | | | the same exception, exit 3 after 8-9 s |

`c-*-t` passes have all timing on (`PANPLAY_XTIMING`, `MESA_VK_X11_SW_TIMING`, `PANVK_KBASE_WAIT_STATS`); `c-drain` is the same
pump without timing (418.6 vs 430.1 fps).

## P1: the cube's pump was the cap

- **Before (old pump, `DXCUBE_PUMP=old`):** per frame, render 0.20 ms, Present 0.60 ms, pump 20.40 ms (d3d11). The pump loop
  waits until `GetTickCount()` has moved 10 ms, sleeping 5 ms per step. Wine's tick count is the wineserver's shared-data
  time, updated about every 16 ms when idle, so the loop exits on the second update: 47-50 fps on any GPU (the G615 showed the
  same 21.7 ms p50).
- **After (drain pump, the new default):** pump 0.11 ms; d3d11 **418.6 fps** (430.1 with timing on), DXVK GPU load 77-79%,
  p50 2.1 ms. The X server keeps up: 399 MIT-SHM PutImage/s, its request thread busy 451 ms/s (`XT` line: shm lock 0.14 ms,
  copy 0.57 ms), and it composites at 57-59 fps (vsync).
- **d3d9** goes from 49.2 to **120.0 fps** (GPU 28% -> 32%). The D3D9 cube keeps the D3D9 default present interval (vsync),
  so DXVK uses FIFO and Mesa's X11 sw present paces at `MESA_VK_X11_SW_REFRESH_HZ` (120 Hz, set by PanPlay): `pace=6.76 ms`
  per frame in the sw timing line. That is the intended vsync pacing. The D3D11 cube presents with sync interval 0.
- **The P-C sw knobs matter once the pump is gone:** `PANVK_WSI_SW_CACHED=0 PANVK_WSI_SW_DEFER_REPLY=0` (114 behaviour) gives
  125.3 fps instead of 430.1 (copy into the MIT-SHM segment 5.73 ms instead of 0.57 ms, queue 22 ms). With the old pump both
  ran at about 47 fps, which is why P-C "changed nothing".
- **The 20 ms CSF wait slice is not the cap (log lines `kbase: wait-stats`):**
  - `kcpu=0`: the KCPU path (`PANVK_KBASE_KCPU_SYNC`) is off by default.
  - Event waits that slept a full 20 ms slice: 160 in 31 s with the old pump, 72 in 32 s with the drain pump (1-5 per second
    out of 6000-8500 waits), 0 with the 114 sw knobs. None of them is in the present thread: its per-second maximum fence wait
    stays at or under 11.75 ms (`wsi/x11 sw timing ... fence=avg/max`). They are other threads' waits, off the frame path.
    Frame p99 stays at 7.5-8.1 ms.

## P2: the 32-bit HUD text and the cached dma-heap

- `vulkan9` (`WINEDEBUG=+vulkan`): the 32-bit winevulkan logs `Using placed map with alignment 4096`, so the i686 D3D9
  host-visible memory is placed-mapped. Under 091 that makes it a shareable BO from `/dev/dma_heap/system`, imported into kbase.
- Every process logs `kbase: coherency mode 31 (none)`. kbase then drops `BASE_MEM_COHERENT_SYSTEM` from the import, and the
  system heap's CPU mapping is cached: the HUD's vertex writes can stay in the CPU cache when the GPU reads them.
- Default (`c-drain-t`): HUD text missing, only the frame-time graph and the memory bars are drawn
  (`screenshots/d3d9-c-drain-t-harness.png`). The 115pre run had it partly garbled or missing.
- The 64-bit cubes do not use placed maps; their HUD is clean.
- Candidates, all HUD-clean at the same fps:
  - `nc-uncached` (`PANVK_KBASE_UNCACHED_HEAP=1`, new): HOST_COHERENT dma-heap BOs from `/dev/dma_heap/system-uncached`, 119.5 fps
    (`screenshots/d3d9-nc-uncached-harness.png`). The heap exists on Pixel 8 (`GPU not IO-coherent: HOST_COHERENT dma-heap BOs from
    /dev/dma_heap/system-uncached`).
  - `nc-heapenv` (existing `PANVK_KBASE_DMA_HEAP=/dev/dma_heap/system-uncached`, all dma-heap BOs): 116.9 fps, p95 17.8 ms
    (HOST_CACHED memory uncached too).
  - `nc-clean` (`PANVK_KBASE_IMPORT_SYNC=1`, clean before each submit): 119.8 fps (`screenshots/d3d9-nc-clean-harness.png`).
    The clean is O(BO size) per submit; DXVK chunks reach 256 MiB in games.
- Chosen: the uncached heap by default on CSF with coherency mode 31, the per-submit clean only when the kernel has no
  `system-uncached` heap.
- 114: this run could not load it (see above). Its dma-heap path is the same code (`kbase_kmod_bo_alloc_dmabuf` and the import are
  unchanged from p115-int-base to p115-pd); the confirm run runs the d3d9 cube on 114.

## P3: the D3D12 swapchain fault

- **Create log** (`MESA_VK_WSI_CREATE_LOG=1`, gears and SquidRoom alike):
  `wsi: CreateSwapchain enter platform=4 minImages=3 format=44 extent=1280x707 usage=0x12 mode=2 flags=0x2c0 old=0x0 pNext: 1000275002`,
  then `wsi: CreateSwapchain platform create result=0`. Flags 0x2c0 = `PRESENT_ID_2 | PRESENT_WAIT_2 | PRESENT_TIMING_EXT`; the pNext
  is `VkSwapchainPresentModesCreateInfoEXT`. Mesa's X11 swapchain was created; the fault came after it.
- **Fault (`WINEDEBUG=+seh`, thread 0134):** `handle_syscall_fault code=c0000005 addr=0x0 pc=0x785e30e818`, `info[1]=0x10` (read of
  address 0x10), `x0=0`, `lr=0x785e30e8c8`, `x17=0x785e30e808` (PLT target).
  `maps-2319.txt` puts the pc in `imagefs/bionic/usr/lib/libxcb-randr.so.0.1.0` at offset 0xe818. The symbols of that file
  (from `imagefs_bionic.txz`): pc = `xcb_randr_get_screen_resources_current_crtcs_end+0x10`, lr =
  `xcb_randr_get_screen_resources_current_outputs_end+0x18`. Offset 0x10 is `num_crtcs` of the reply, so the reply pointer was NULL.
- **Path:** `wsi_CreateSwapchainKHR` with `PRESENT_TIMING` calls `poll_early_refresh` -> `x11_update_present_timing_xrandr_estimate`
  -> `loader_update_screen_resources` -> `xcb_randr_get_screen_resources_current_unchecked` + `_reply`, then
  `xcb_randr_get_screen_resources_current_modes(reply)` with `reply == NULL`.
- **Why NULL:** PanPlay's X server (`XServer.java`) implements BIG-REQUESTS, MIT-SHM, Present, SYNC and XInputExtension, no RANDR.
  libxcb answers a request to a missing extension by shutting the connection down (`XCB_CONN_CLOSED_EXT_NOTSUPPORTED`), which is
  also Wine's Xlib connection.
- **Env sets:**
  - Only `g-nowait` (`VKD3D_DISABLE_EXTENSIONS=VK_KHR_present_wait,VK_KHR_present_id,VK_KHR_present_wait2,VK_KHR_present_id2`)
    rendered. vkd3d-proton enables present timing only together with id2/wait2, so the create flags became 0x0: gears 215.3 fps,
    `DX12: PASS`, exit 0 (`screenshots/d3d12gears-g-nowait-harness.png`).
  - Everything else crashed in the same place: maintenance1 off (no pNext, mode IMMEDIATE), `VKD3D_SWAPCHAIN_PRESENT_MODE=IMMEDIATE`,
    the P-C sw knobs at 114, `MESA_VK_WSI_DEBUG=noshm`, all 21 knobs at 114, `+vulkan`.
  - 114 has the same code: present timing on X11 and `EXT_present_timing` are already in p115-int-base.
- **Harness bug found here:** `g-nowait` printed its PASS line, but the pass is marked "no pass line". The harness stores only the
  first 50,000 output lines, and `+seh` produced 158,739 before the PASS line. Fixed in PanPlay (verdict lines are always kept).

## Screenshots

| file | what |
|---|---|
| `screenshots/d3d11-c-old-t-harness.png` | d3d11, old pump: 47 fps |
| `screenshots/d3d11-c-drain-harness.png` | d3d11, drain pump: about 420 fps, HUD clean |
| `screenshots/d3d9-c-drain-t-harness.png` | d3d9, default heap: HUD text missing |
| `screenshots/d3d9-nc-uncached-harness.png` | d3d9, `PANVK_KBASE_UNCACHED_HEAP=1`: HUD clean, `FPS: 120.2`, `GPU: 34%` |
| `screenshots/d3d9-nc-clean-harness.png` | d3d9, `PANVK_KBASE_IMPORT_SYNC=1`: HUD clean, `FPS: 120.3` |
| `screenshots/d3d12gears-g-nowait-harness.png` | gears rendering with the timing flag removed |
| `screenshots/video-<s>-<pass>.png` | FTL video frames at the screenshot times (175, 304, 389, 433, 664 s) |
| `screenshots/video-{start,mid,end}.png` | FTL video at 30 s, the middle and the end |
