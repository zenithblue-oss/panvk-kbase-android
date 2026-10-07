# NFS Most Wanted stutter follow-up (shader cache, GPL path, loading stall)

Date: 2026-10-07. Patches: `csf-v11/170`, `171`. Launcher: `Containers.kt`
(game env). Driver build `beta.18-dev+stutter`, BuildID
`73947988e06f91b054c6c1507ed8d4ca87a1d44d`, PanPlay `1.2.3+stutter`
(versionCode 13, not committed).

Input: perf capture `/var/tmp/panvk/nfs-stutter/runs/nfs-23197` (driver
with 152-154) and the DXVK debug log of session `20261007-141718`.
Remaining freezes after 152-154: 1.07 s and 1.57 s on `dxvk-cs`
(0.65 s per pipeline, same 3 pipelines every session), plus a 1.3 s
freeze while loading (17-19.6 s) on the game thread.

## 1. Shader disk cache

Findings:

- The Mesa disk cache is built in (`-DENABLE_SHADER_CACHE`, zlib).
  panvk creates it in `init_shader_caches()`. The key is the driver
  BuildID (`driver_build_sha` from the ELF build-id note) plus the GPU
  id, so a driver update starts a fresh cache.
- It was never active: `disk_cache_enabled()` defaults to *disabled* on
  Android (`DETECT_OS_ANDROID`), because upstream expects the EGL blob
  cache. PanPlay also did not set `MESA_SHADER_CACHE_DIR`. The wine env
  has `HOME=<container dir>`, so the path would have worked; only the
  Android default blocked it.
- DXVK 3.1.1 has its own cache, but it only stores DXBC -> SPIR-V
  results (`C:\users\xuser\AppData\Local\dxvk\*.dxvk.bin`, inside the
  prefix, so already persistent). The panvk compile (NIR -> Mali ISA,
  where all the time goes) was redone every session.

Changes:

- `csf-v11/170`: keep the disk cache on by default on Android. If no
  writable `$MESA_SHADER_CACHE_DIR`, `$XDG_CACHE_HOME` or `$HOME/.cache`
  exists (a plain Android app), path init fails and the cache stays
  inert, as before. `MESA_SHADER_CACHE_DISABLE=true` still turns it off.
  This also covers other launchers that use this driver and set `HOME`.
- PanPlay sets for every game: `MESA_SHADER_CACHE_DIR=<filesDir>`
  (cache in `files/mesa_shader_cache`), `MESA_SHADER_CACHE_DISABLE=false`,
  `MESA_SHADER_CACHE_MAX_SIZE=1G`. Per-game env can still override them.
- No "Clear shader cache" button: deleting `files/mesa_shader_cache` is
  enough, and a driver update invalidates the cache on its own.

## 2. Why DXVK compiles on dxvk-cs instead of fast-linking

DXVK 3.1.1 source (`/var/tmp/panvk/components-build/dxvk-src`). panvk
passes every check DXVK makes for GPL: `graphicsPipelineLibrary`,
`graphicsPipelineLibraryIndependentInterpolationDecoration`, EDS3
`DepthClipEnable`, `RasterizationSamples`, `SampleMask`,
`AlphaToCoverageEnable` are all 1, and the log says "Graphics pipeline
libraries supported". No missing feature or property forces full
compiles, so there is no driver-side GPL fix.

The cause is DXVK's 32-bit pipeline lifetime tracking:

- NFS is a 32-bit game, so `env::is32BitHostPlatform()` is true and
  `DxvkDevice::mustTrackPipelineLifetime()` returns true (it is only
  turned off for RADV).
- With tracking on, `DxvkShaderPipelineLibrary::compilePipeline()` (the
  background `dxvk-shader` workers) compiles each VS/FS library and then
  destroys it right away to save address space. The comment says "We
  should hit the driver's disk cache once we need to recreate the
  pipeline".
- At first draw, `getPipelineHandle -> createInstance -> getBasePipeline
  -> acquirePipelineHandle()` recreates the library synchronously on
  `dxvk-cs`.
- panvk's in-memory pipeline cache holds only weak references
  (`weak_ref = true` in `panvk_vX_device.c`), so the shader was freed
  with the library, and the disk cache was off. The re-create was a full
  compile on the render thread. In the capture the workers spent 1.3 s
  compiling and `dxvk-cs` another 2.2 s, on the same shaders.

Fixes (two independent layers):

- With 170 the re-create becomes a disk cache lookup, in the first
  session too as long as the worker's disk write (async queue) has
  finished before the draw. Not measured in NFS; the second layer below
  makes this moot for PanPlay.
- PanPlay sets `DXVK_CONFIG="dxvk.trackPipelineLifetime = False"` for DXVK
  games. Address space is not a concern here: the driver is a 64-bit
  native library under wow64 (PanPlay has no 32-bit ARM driver), and
  pipelines never live in the 32-bit guest address space. DXVK then keeps
  the libraries and only fast-links at draw time. Verified in the DXVK
  log: "Found config env: dxvk.trackPipelineLifetime = False". A per-game
  `DXVK_CONFIG` replaces it.

What is left on `dxvk-cs` after this: shaders the game creates right
before their first draw (no time for a worker to compile them first) and
D3D9 fixed-function shaders, which DXVK generates on first use. Those
compile once per driver build and then come from the disk cache.

Side finding (DXVK, not fixed): DXVK's own SPIR-V cache writer flushes
in batches of 32 and otherwise only in the destructor of a static
singleton. NFS adds 31 new shaders per session and does not exit
cleanly, so those 31 are never saved ("Cache: 64 shaders" in every
session). Cost is only the DXBC -> SPIR-V step, small next to the panvk
compile.

## 3. 1.3 s loading freeze (18.29-19.63 s)

On-CPU time of the game thread (`speed.exe`) in that window: 890 ms.

| what | ms | where |
|---|---|---|
| `panvk_CreateImage` | 168 | driver |
| - of which `os_get_option` (property lookups) | 128 | driver |
| `panvk_AllocateMemory` | 107 | driver |
| - `KBASE_IOCTL_MEM_ALLOC` (kernel page alloc + clear) | 69 | kernel |
| - `memset` of new BO pages (`csf-v11/041`) | 35 (+13 faults) | driver |
| page faults from game/FEX code, file mappings (f2fs) | ~120 | game/wine |
| FEX icache maintenance (`caches_clean_inval_pou`) | ~21 | FEX JIT |
| `munmap` (wine `NtFreeVirtualMemory`) | 63 | wine |

- `os_get_option()` on Android falls back to three
  `__system_property_get()` calls per miss (debug./vendor./plain
  `mesa.*`), and for an app context each denied lookup is logged by
  libc ("Access denied finding property"). That is about 1 ms per call.
  `panvk_image_init()` (`PANVK_BC_AFRC`) and `panvk_bc_shadow_info()`
  read an option on every `vkCreateImage`; `spirv_to_nir` and
  `bifrost_dump_shader` read one per shader. Session total 335 ms.
  `csf-v11/171` caches the property result per name. The environment is
  still read on every call.
- The BO `memset` from `csf-v11/041` is not redundant: kbase recycles
  freed pages through the per-context pool without clearing them, which
  is why the zero-init CTS test needed it. Kept.
- The rest is the game streaming data (file faults, wine memory
  management, FEX). Nothing to change in the driver.

## Side finding: tiler heap renew on dxvk-submit

The same capture shows `kbase_renew_tiler_heap ->
KBASE_IOCTL_CS_TILER_HEAP_TERM` on `dxvk-submit`: 4988 calls in 159 s,
1.5 ms p50, 4.8 ms p90, 23 ms max (kernel `kbase_mem_pool_add_array`,
page table teardown). This is per-submit cost on the submit thread, not
a freeze, but worth a look (renew interval, csf-v11/107). Not changed
here.

## Verification (G615)

Test: `/storage/emulated/0/Games/dxcube/dxcube-i686.exe d3d9` (32-bit,
D3D9 fixed function, DXVK HUD full, 8 s), started through a temporary
PanPlay shortcut. Recorder and analysis:
`/var/tmp/panvk/nfs-stutter/cachetest/` (`run-ct.sh`, `rect.sh`,
`ana_ct.py`). simpleperf, on-CPU ms inside `vk_compile_shaders`:

| run | driver | cache | compile total | on dxvk-cs | disk_cache_get | frames in 8 s |
|---|---|---|---|---|---|---|
| base | beta.18-dev+shc (dd7f378c) | off | 800 ms | 737 ms | 0 | 352 |
| cold | +stutter (73947988) | empty | 783 ms | 721 ms | 3 ms | 332 |
| warm | +stutter (73947988) | 57 files, 2.5 MB | **0 ms** | **0 ms** | 13 ms | 390 |

- The warm run wrote no new cache files (57 before and after).
- In this test the `dxvk-cs` compiles are D3D9 fixed-function and HUD
  shaders made on first use, so `trackPipelineLifetime` does not change
  the cold run; the disk cache removes them from the second run on.
- Property lookups (171): the base run logged the same denied property
  up to 39 times per process (`vendor.mesa.bifrost.mesa.dump.dir`, once
  per shader); with 171 every name is looked up at most once per process
  (221 -> 101 "Access denied" lines per run).
- On-device driver sha256 `aadf6776...` matches the build
  (BuildID `73947988`). Install kept app data (shortcuts, containers).

## NFS results (beta.18-dev+combined, BuildID 4eab4565)

Two user sessions on the combined build (152-154, 160-164, 170-171, 180):
`nfs-29088` (cold cache) and `nfs-10098` (warm cache). `ana_nfs.py` and
`cgp.py` in `/var/tmp/panvk/nfs-stutter`.

| | 23640 old | 23197 RA fix | 29088 cold | 10098 warm |
|---|---|---|---|---|
| gaps >150/250/1000 ms | 7/6/4 | 7/4/3 | 4/3/1 | 4/2/0 |
| freeze sum / max | 14.8 / 4.50 s | 4.85 / 1.57 s | 2.43 / 1.06 s | 1.62 / 0.99 s |
| dxvk-cs compile | 11861 ms | 2183 ms | 491 ms | 0 (11 ms cache reads) |
| worker compile | 1458 ms | 1296 ms | 1245 ms | 0 (118 ms cache reads) |
| race start (~30 s) | 4.5 + 4.0 s | 1.57 s | 0.93 s | none >250 ms |
| ~110 s spot | 4.07 s | 1.07 s | none | none |
| loading freeze | 1.30 s | 1.34 s | 1.06 s | 0.99 s |

- Cache: 57 files before session 1, 540 files / 5.4 MB after, unchanged
  after session 2 (all hits).
- The cold race-start freeze is one 419 ms compile on dxvk-cs plus the
  disk cache writer compressing new entries (zlib `longest_match` on the
  `disk$` threads, about 360 ms of CPU). First session only.
- Loading freeze in the warm session: `os_get_option` 128 -> 0 ms
  (session total 335 -> 3 ms), `panvk_CreateImage` 168 -> 24 ms. Left:
  `panvk_AllocateMemory` 103 ms (kbase alloc ioctl 62, BO zeroing 35),
  wine `munmap` 72 ms, file-backed page faults and FEX/game code.
- Other warm stalls >250 ms: 36.1 s, 283 ms, game/FEX code on the game
  thread, no driver compile.
- Hardware keyboard connected 12.5 s into session 1: config change, no
  activity relaunch, the game ran 177 s more. `VK_ERROR_SURFACE_LOST_KHR`
  appears only at the user exit (window teardown). No crash.

## What to test in NFS (user)

- Two sessions of the same race or the same stretch of the map. The
  first session after this update compiles and fills the cache; the
  second one should not freeze where the first one did (the 1-1.6 s
  freezes at the first race start and at 110 s).
- The loading freeze at about 18 s should be shorter in both sessions.
- `files/mesa_shader_cache` should exist and grow after the first
  session.
