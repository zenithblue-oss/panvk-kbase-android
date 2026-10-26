# Staging 115, fix: PanPlay on Pixel 8 (v11, coherency mode 31): D3D9 HUD text, D3D12 swapchain fault, diagnostics

Source: the PanPlay problems seen on FTL shiba in `worklogs/g615-dxvk/device-runs/2026-10-26-ftl-v11-shiba-panplay-115pre/`,
diagnosed in `../../../worklogs/g615-dxvk/device-runs/2026-10-26-ftl-v11-shiba-panplay-115fix-diag/results.md` and confirmed in
`../../../worklogs/g615-dxvk/device-runs/2026-10-26-ftl-v11-shiba-panplay-115fix/results.md`.

- Base: branch `p115-pd` (`8a4d9f8e`, see `../pd/NOTES.md`), so P-A .. P-D and the upstream-style driverInfo are below.
- Work tree: `/home/flux/src/mesa-115-fix`, branch `p115-fix` (head `de6b5b2d`).
- Apply: `git am patches/staging-115/fix/*.patch` on `p115-pd` (4 patches).
- Export: `git -C /home/flux/src/mesa-115-fix format-patch p115-pd..p115-fix -o patches/staging-115/fix/`.
- Strict check (2026-10-26): on a fresh 5a07217f worktree (`/home/flux/src/mesa-115-fixv`),
  `scripts/prepare-rc5-tree.sh --mesa /home/flux/src/mesa-115-fixv --csf-max 233 --wip-max 0092` printed
  `BASE-OK applied=228 csf-v11<=233` and `OK base=228 jm-v7-wip=87`, with no PATCH-DRIFT, WIP-DRIFT, offset or fuzz (only the old
  jm-v9/002 whitespace warning); tree `dee163e5` = p115-int-base. Then `git apply --check -v` + `git am` of pa, pb, pc, int, pd and
  fix, one patch at a time: no offset, fuzz, warning or conflict. Final tree `213079bc` = `p115-fix`.
- Build: `/home/flux/build-out/2026-10-26-115fix/libvulkan_panfrost.so`, driverInfo `Mesa 26.3.0-devel (git-de6b5b2d23)`, sha256 `42d91d26...`
  (details in `worklogs/g615-dxvk/builds/rc5-universal.md`, 115fix).

## Patches

| # | Title | Problem | Knob (default; 115 value) |
|---|---|---|---|
| 0001 | wsi/x11, kbase: opt-in present stage timing and CSF wait stats | diagnostics (P1) | `MESA_VK_X11_SW_TIMING`, `PANVK_KBASE_WAIT_STATS` (off; off) |
| 0002 | kbase: HOST_COHERENT dma-heap memory from system-uncached on a non-coherent GPU | P2 D3D9 HUD text | `PANVK_KBASE_UNCACHED_HEAP` (1; 0), `PANVK_KBASE_IMPORT_SYNC` on CSF (1 only without the uncached heap; 0) |
| 0003 | wsi: opt-in swapchain create log (MESA_VK_WSI_CREATE_LOG) | diagnostics (P3) | `MESA_VK_WSI_CREATE_LOG` (off; off) |
| 0004 | wsi/x11: no RandR on a server without RANDR, no present timing on the CPU copy path | P3 D3D12 fault | none (bug fix) |

0001 and 0003 only log when their variable is set. On a GPU with IO coherency (G615 and every coherency mode other than 31),
0002 changes nothing. 0004 changes behaviour only on an X server without RANDR (PanPlay) and on the CPU copy present path.

### 0001: present stage timing and wait stats (diagnostics)
- `MESA_VK_X11_SW_TIMING=1`: one `wsi/x11 sw timing WxH: N fps queue= fence= pace= slot= copy= put= reply= acquire=` line per
  second (ms mean/max of each stage of the sw present: vkQueuePresentKHR to the present thread, blit fence wait, FIFO pacing,
  MIT-SHM slot wait, CPU copy, PutImage, server reply, acquire wait).
- `PANVK_KBASE_WAIT_STATS=1`: one `kbase: wait-stats waits=N full_20ms_slices=M avg_ms= max_ms= kcpu=` line per second for the
  CSF event waits (a full slice is a ppoll that slept its whole 20 ms).
- Atomics only when enabled; nothing is read in a hot path when off (the flag is a chain or device field).

### 0002: system-uncached heap for HOST_COHERENT memory on a non-coherent CSF GPU
- **Problem:** 32-bit WoW64 winevulkan places its host-visible maps (091), so that memory is a shareable BO from
  `/dev/dma_heap/system`, whose CPU mapping is cached. On coherency mode 31 kbase drops `BASE_MEM_COHERENT_SYSTEM` from the
  import and the GPU does not snoop: CPU writes to HOST_COHERENT memory can still be in the CPU cache when the GPU reads.
  The i686 D3D9 cube's DXVK HUD text was missing or garbled on Pixel 8 with 115 (114 drew it clean, see the confirm run
  below). The G615's imports keep COHERENT_SYSTEM.
- **What:** on CSF with coherency mode 31, dma-heap BOs without WB_MMAP come from `/dev/dma_heap/system-uncached`
  (write-combined CPU mapping, as kbase maps its own HOST_COHERENT memory). HOST_CACHED (WB_MMAP) BOs stay on the system heap
  with explicit flush/invalidate. If the heap cannot be opened, CPU-mapped non-coherent imports are cleaned before each
  submit (the v9 JM path, now also on CSF). One info line says which.
- **Evidence (diag run):** HUD clean with the uncached heap (119.5 fps vs 120.0) and with the clean (119.8 fps); missing with the
  cached heap. The existing `PANVK_KBASE_DMA_HEAP=/dev/dma_heap/system-uncached` also fixes it but makes HOST_CACHED memory
  uncached as well (p95 17.8 ms vs 12.6 ms).
- **Risk:** low. CPU reads of HOST_COHERENT dma-heap memory become uncached reads, as they already are for kbase-native
  HOST_COHERENT memory. The clean fallback costs O(BO size) per submit, so it is only the default when the heap is missing.

### 0003: swapchain create log (diagnostics)
- `MESA_VK_WSI_CREATE_LOG=1`: `wsi: CreateSwapchain enter platform= minImages= format= extent= usage= mode= flags= old= pNext: <sTypes>`
  before the platform create and `wsi: CreateSwapchain platform create result=` after it. It placed the D3D12 fault after
  Mesa's X11 create.

### 0004: RandR and present timing on the PanPlay X server
- **Problem:** vkd3d-proton creates its swapchain with `VK_SWAPCHAIN_CREATE_PRESENT_TIMING_BIT_EXT` (flags 0x2c0). After the
  X11 create, `wsi_CreateSwapchainKHR` polls the refresh rate (`x11_poll_early_refresh`), which sends RandR requests. PanPlay's
  X server has no RANDR: libxcb shuts the connection down (`XCB_CONN_CLOSED_EXT_NOTSUPPORTED`), the reply is NULL and
  `loader_update_screen_resources` reads `reply->num_crtcs`. SIGSEGV in `xcb_randr_get_screen_resources_current_crtcs_end+0x10`,
  fault address 0x10; Wine reports `vkCreateSwapchainKHR Exception 0xc0000005 in Unix call` and both D3D12 tests exit 3.
- **What:**
  - the X11 connection records whether the server has RANDR; without it a swapchain gets no screen resources, so the refresh
    estimate is 0 and no RandR request is sent (also for `VK_GOOGLE_display_timing`);
  - `loader_update_screen_resources` keeps its state when the screen resources reply is NULL;
  - surfaces whose swapchain would use the CPU copy present (`x11_present_to_x11_sw`: sw device, no MIT-SHM pixmaps) report
    no present timing, because that path never reports a present stage: timing requests would never complete and fill the
    timing queue. vkd3d-proton then creates the swapchain with id2/wait2 only (flags 0xc0).
- **Risk:** low. A normal X server has RANDR and DRI3, so neither branch is taken there.

## Confirm run (FTL shiba, matrix-33ltrfa55a2v0)

This build was bundled in PanPlay 871368a; details are in the confirm `results.md`.

- **Cubes (with the PanPlay drain pump):**
  - d3d11: 427.6 fps, DXVK GPU 78.5%;
  - d3d9: 118.8 fps (vsync, 120 Hz), GPU 33.0%.
- **0002:**
  - The D3D9 HUD is clean by default.
  - With `PANVK_KBASE_UNCACHED_HEAP=0` it is garbled from the first frame.
  - 114 (RC5-universal-114-fps `51f6fdbe`) draws it clean too, although its dma-heap code is the same. The 115pre `old` set (all 21
    knobs at 114) was still garbled, so a 115 change that has no knob changes the timing that let 114 pass. That change is not
    identified. The uncached heap does not depend on it.
- **0004:**
  - Both D3D12 tests render and pass: SquidRoom `DX12: PASS ... fps=78.2`, gears `DX12: PASS ... fps=182.2`.
  - The create log shows flags 0xc0, with no present timing.
  - On 114, gears still faults (`vkCreateSwapchainKHR Exception 0xc0000005`, exit 3).
