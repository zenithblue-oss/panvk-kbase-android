# Perf review C (2026-10-26): GPU-side efficiency, CSF v10/v11 (with JM v6/v7/v9 notes)

- **Scope.** Bandwidth, stalls and wasted GPU work in PanVK over kbase. The main targets are Mali-G615 MC6 (X6 Pro, v11), G710 (v10) and G715 (v11). The other reviews in this directory cover CSF CPU record/submit (A), memory footprint (B) and the JM submission path (D).
- **Source.** Worktree `/home/flux/src/mesa-rc5g`, tag `rc5g-114` (HEAD d324a269). The review is read-only. Paths are relative to `src/panfrost/` unless they say otherwise.
- **"Introduced by".** Taken from `git blame` / `git log` in the worktree. `^5a07217f` is the upstream base.
- **Data used:**
  - `device-runs/2026-10-24-g615-rc5u-112`: FO4 and PanProbe on the X6.
  - `device-runs/2026-10-22-g615-rc5u/raw/cube/d3d11`: the D3D11 cube.
  - The 2026-10-25 FTL 114 runs: panther (v10) and shiba (v11).
  - `fo4-memory-analysis-2026-10-23.md`, `perf-review-2026-10-22.md` and `builds/rc5-universal.md`.
- **Gains.** Estimates unless they quote a measurement.
- **Knobs.** They follow the series' convention: the new behaviour is the default and `=0` restores the old one. Where an existing knob already does the job, it is named instead.
- **Patch numbers.** The next free csf-v11 number is 233.

## 0. What limits the numbers today

- **FO4 in game is swap-bound.** On 112 it ran at 3.8-7.9 fps with MemAvailable at 265 MB, SwapFree down to 156 MB and a 2008 ms frametime max.
  - No GPU-side change will show there until the memory items land, or until the game's texture budget is lowered (`iTexMipMapSkip=1`, rank 2 in fo4-memory-analysis).
  - Until then, use the FO4 menus (63-65 fps), the cubes and PanProbe for GPU A/B.
- **The cubes are not GPU-bound.** The D3D11 cube setup:
  - 1280x705 window, B8G8R8A8_UNORM, IMMEDIATE, about 96 draws.
  - 47.1 fps (p50 21.7 ms) with a DXVK GPU load of 48.8 %.
  - The game process uses 0.6 cores, and the CPU clusters mostly run at 480-1900 MHz (`perf-...-sys.csv`).
- **The cube's GPU work is small.** Clears fold into loadOp CLEAR, there is one DXVK swapchain blit and one WSI image-to-buffer copy. That is about 15 MB of GPU traffic per present at 1280x705, well under 1 ms.
  - None of that explains 46-49 fps. The serialized present chain (C2) does.

## 1. Ranked findings

| rank | id | finding | archs | class | expected gain | risk | confidence |
|---|---|---|---|---|---|---|---|
| 1 | C1 | Depth-bounds emulation turns off FPK, the early ZS update and FS skipping for every DXVK fast-linked fragment shader | v9, v10, v11 (v6/v7 fixed by jm-v7 076) | project (A/B now) | Rough: 0.5-3 ms per full-screen opaque pass at 2400x1080 on affected draws. Depth-only passes with a bound PS lose their whole FS cost | low-medium | high (cost per draw), medium (share of draws) |
| 2 | C2 | X11 sw present: the CPU reads each frame from write-combined memory, then blocks on an X server round trip | all (kbase without DRI3) | quick | Estimate: about 3.5 ms per present at 1280x705 and 10 ms at 2400x1080 come off the present thread. Cube likely 47 → 60+ fps | low | high (code), medium (cube limiter) |
| 3 | C3 | Tessellation emulation runs about 8 dependent dispatches, with full drains, per tess draw | v10, v11 | project (measure FO4 first) | Measured fixed cost per tess draw: 157-468 µs, against 1.5-4.5 µs untessellated. Halving the steps saves about 80-230 µs per tess draw | medium-high | high (cost), low (FO4 share) |
| 4 | C4 | The preload barrier drains the fragment subqueue before every pass that loads an attachment | all (JM: a batch split) | project | Estimate: a 10-50 µs bubble per LOAD pass, so 0.3-1.5 ms per 30-pass DXVK frame | medium | high (code), medium (gain) |
| 5 | C5 | An FO4 shader needs more than 4 KiB of stack per thread (dev_tls 336 MB = 8 KiB per thread) | all | measure first | Unknown until the shader is found. The footprint side is reviewer B's | low (diagnostic) | medium |
| 6 | C6 | v9/v10 preload Z/S on every tile (EARLY_ZS_ALWAYS). 061 changed v11 only | v9, v10 | quick | Up to 10.4 MB of Z plus 2.6 MB of S reads per ZS-load pass at 2400x1080, for tiles no primitive touches | low | high (code), low-medium (gain) |
| 7 | C7 | CRC mostly costs on DXVK: 019 invalidates it on every CLEAR/DONT_CARE, MUTABLE and >4 B/px targets get none, and v10 drains to mark it valid | v10, v11 | measure, then project | Static frames: up to 3.6-10.4 MB of writeback per frame. Today possibly a net cost | medium | medium |
| 8 | C8 | On DXVK devices, every barrier with vertex-stage destinations also makes the COMPUTE subqueue wait | v10, v11 | project | Estimate: under 0.5 ms/frame, only with D3D11 compute | medium | medium (exists), low (gain) |
| 9 | C9 | Renewed tiler heaps start at 4 chunks, so big passes grow them through kernel OOM round trips | v10, v11 (kbase) | A/B now, then project | Unmeasured: one firmware → kernel round trip per extra 1 MiB chunk | low-medium | low-medium |
| 10 | C10 | Blend shaders for float-RT factors other than ZERO/ONE/SRC_ALPHA/INV_SRC_ALPHA, and for MIN/MAX | all | measure first | Unknown: a blend shader per fragment, and per sample with MSAA | medium | low |

- **Quick safe wins (section 2):** C2, C6, and the A/B experiments in 2.3, which need no code or only a one-line knob.
- **Bigger projects (section 3):** C1, C3, C4, C5 (after diagnosis), C7, C8, C9 and C10.

## 2. Quick safe wins

### C2. X11 software present: CPU read from write-combined memory and a blocking round trip per present

- **Where:**
  - **sw path is used.** `vulkan/panvk_wsi.c:518-535`: on kbase without the DRI3 dma-buf path, the WSI device is `sw_device`. `PANVK_KBASE_DRI3` is unset in the cube run, and PanPlay's built-in X server has MIT-SHM but no DRI3.
  - **Memory type choice.**
    - `src/vulkan/wsi/wsi_common.c:3637` selects the blit destination type with `wsi_select_host_memory_type()` (`:3084-3093`), which asks for HOST_COHERENT|HOST_CACHED.
    - PanVK has no such type on a non-IO-coherent GPU (`vulkan/panvk_physical_device.c:536-566`). It only has type 1 (DL|HV|HC) and type 2 (DL|HV|HCACHED).
    - So the fallback at `wsi_common.c:3066-3070` picks type 1, which is the uncached (write-combined) kbase mapping.
  - **Present function.** `src/vulkan/wsi/wsi_common_x11.c:1894-2010`, `x11_present_to_x11_sw()`, does three things in order:
    1. `memcpy(image->sw_shm_map, image->base.cpu_map, ...)` (`:1930`, `:1934`) reads the GPU-written blit buffer through that mapping.
    2. `xcb_shm_put_image` (`:1938`) sends the frame.
    3. `xcb_get_geometry` (`:1946`) is followed by a blocking reply (`:2001`) before the present finishes.
- **Introduced by:** csf-v11/117 (0b43ea13: MIT-SHM AttachFd and FIFO pacing) and 106 (351d52a4: fence wait on the present thread). The memory-type choice is upstream WSI.
- **What is wasted, per present, on one thread:**
  1. The fence wait.
  2. A full-frame CPU read from uncached memory, with no line fills and no prefetch: 3.6 MB at 1280x705, 10.4 MB at 2400x1080.
  3. The X server's processing of the put, waited for synchronously through the reply.

  The GPU side is cheap: a compute copy of the swapchain image to the linear buffer. The chain is latency-bound, which fits the cube data: 49 % GPU load, 0.6 CPU cores and low clocks. Serialization keeps utilisation low, and the governor then lowers clocks, which makes every step slower.
- **Expected gain:**
  - Assuming about 1 GB/s for uncached reads (typical for these cores, unmeasured), the memcpy costs about 3.5 ms per present at 1280x705 and 10 ms at 2400x1080.
  - From cached memory, plus an invalidate (DC CIVAC or MEM_SYNC over the range), it would be under 1 ms.
  - Deferring the reply takes the server's processing off the critical path.
  - Cube: plausibly 47 → 60+ fps (low confidence, because the Java X server side is unknown).
  - FO4 menus may gain. FO4 in game will not, because it is swap-bound.
- **Risk:** low.
  - A missing invalidate shows up as stale frames right away.
  - With the deferred reply, each image's reply must still be collected before the next memcpy into that image's segment. That keeps 117's no-race guarantee. OUT_OF_DATE is then reported one present later.
- **Fix:**
  - **(a) `PANVK_WSI_SW_CACHED`** (default 1; `=0` keeps today's write-combined buffer).
    - PanVK sets a new `wsi_device` flag when the GPU is not IO-coherent and memory type 2 exists.
    - `wsi_select_host_memory_type()` then picks HOST_CACHED without COHERENT for the blit buffer.
    - Add `WSI_CB(InvalidateMappedMemoryRanges)` to `wsi_common.h` and invalidate the range after the fence wait, before the memcpy. `panvk_InvalidateMappedMemoryRanges()` (`vulkan/panvk_device_memory.c:441`) already does the kbase sync.
  - **(b) `PANVK_WSI_SW_DEFER_REPLY`** (default 1; `=0` keeps 117's synchronous reply). Keep the geometry cookie per image and collect it right before the next memcpy into that image's segment, and at the next present for OUT_OF_DATE.
  - **(c) Later, zero copy.** Either let the X server read the blit buffer directly (ShmAttachFd on an fd for the kbase BO, if kbase can export one), or use the DRI3 raw path (`PANVK_KBASE_DRI3=raw` with Termux:X11). This also removes the WSI GPU copy.
- **How to measure:**
  - D3D11/D3D9 cube fps and DXVK HUD `frametimes,gpuload` for each knob.
  - An env-gated timing line in `x11_present_to_x11_sw()`: fence wait, memcpy and reply wait, averaged over 120 presents.
  - The cube at a second window size: fps that scales with pixel count while GPU load stays around 50 % confirms the copy chain is the limiter.
  - An A/B against `PANVK_KBASE_DRI3=raw` with Termux:X11.
- **Confidence:** high that the read is from write-combined memory and that the reply blocks (code). Medium that this is the cube's limiter.

### C6. v9/v10 still preload Z/S on every tile; 061 changed only v11

- **Where:** `vulkan/panvk_vX_cmd_frame_shaders.c:825-843`.
  - `#elif PAN_ARCH == 11` uses INTERSECT unless `always_load()` or `PANVK_ZS_PRELOAD_ALWAYS=1` (`:799`).
  - `#elif PAN_ARCH >= 9`, which covers v9 and v10, always uses `EARLY_ZS_ALWAYS` (upstream).
  - The colour preload in the same file (`:893-895`) is already INTERSECT.
- **Introduced by:** csf-v11/061 (6e9fbf54), for v11 only. The v9/v10 line is upstream.
- **What is wasted:** on G710/G610 (v10) and G78 (v9), each pass that loads depth/stencil reads Z/S for every tile, including tiles with no primitive in that pass. That is up to 10.4 MB (Z24 in 32 bits) plus 2.6 MB (S8) per pass at 2400x1080. The upstream comment itself calls this an unbenchmarked trade-off: bandwidth against having ZS preloaded earlier.
- **Expected gain:** proportional to the empty-tile fraction times the number of ZS-load passes. It matters for DXVK passes that redraw part of the screen with depth bound: decals, particles, light volumes and UI. It is small for full-screen passes and zero on G615.
- **Risk:** low. This is the mode v11 has used since 061, and every v11 FTL run passes with it.
- **Fix:** change `PAN_ARCH == 11` to `PAN_ARCH >= 10`, and test v9 separately before including it. The same knob restores the old mode: `PANVK_ZS_PRELOAD_ALWAYS=1`.
- **How to measure:**
  - PanProbe 39/39 on FTL panther (v10) and oriole (v9), including depth_stencil, depth_bounds and tessellation.
  - A small PanProbe bench: LOAD depth, draw one small quad, measure ms/frame.
- **Confidence:** high on the code. Low-medium on the size of the gain.

### 2.3 A/B experiments that need no code (or only a one-line knob)

Run them on the X6. For FO4, first bring the game out of swap (Q0), or the numbers are noise.

| exp | for | what to run | what it tells |
|---|---|---|---|
| Q0 | all FO4 A/B | `iTexMipMapSkip=1` (fo4-memory-analysis rank 2) | gives an FO4 interior that is not swap-bound, to measure the rest |
| Q1 | C1 | `dxvk.enableGraphicsPipelineLibrary = False` in dxvk.conf versus the default, on the FO4 menu and interior | If fps rises with GPL off, the lowered fast-link FS matter. If DXVK also declares the enable dynamic in monolithic pipelines, nothing changes, and the C1 counter decides instead |
| Q2 | C2 | the cube at two window sizes, and `PANVK_KBASE_DRI3=raw` with Termux:X11 | Whether the present chain is the limiter |
| Q3 | C3 | the FO4 interior with god rays on and off (Fallout4Prefs.ini, for example `bVolumetricLightingEnable=0` under [Display]; check the key) | The fps delta bounds what the tessellation work can win |
| Q4 | C7 | `PANVK_DEBUG=no_crc` (exists) on the FO4 menu and the cube | Whether CRC is a net win under 019 |
| Q5 | C9 | `PANVK_KBASE_HEAP_CHUNKS=10,400` and `16,400` against the default 4 | The fps delta is the cost of heap growth stalls. The memory cost is in reviewer B's area |
| Q6 | C5 | `BIFROST_MESA_DEBUG=statsfull` for an FO4 session (stderr; check that it reaches `wine-run.log`) | Which shader has a "Stack size" above 4096, and its spills/fills |

## 3. Bigger projects

### C1. Depth-bounds emulation keeps DXVK fragment shaders off FPK, the early ZS update and FS skipping (v9/v10/v11)

- **Where:**
  - **Lowering selection.** `vulkan/panvk_vX_shader.c:2427-2438`, `panvk_depth_bounds_lowered()`. It returns true when depthBounds is enabled and the DBT enable is dynamic or statically on, or when there is no pipeline state: GPL libraries compiled without DS state, and shader objects.
  - **The lowering.** `vulkan/panvk_vX_shader.c:2446-2532`, `panvk_lower_depth_bounds()`. It always stores `gl_SampleMask = app_mask & pass` and sets `outputs_written |= SAMPLE_MASK` at the end. The LD_TILE loop only runs when the `ds.enable` sysval is set.
  - **Where it is applied.** `vulkan/panvk_vX_shader.c:5345-5348`, to every FS that the predicate selects.
  - **The existing fix is v6/v7 only.** `panvk_v7_wants_alt()` and the alternate compile are `#if PAN_ARCH < 9` (`:5492-5528`, `:5619-5657`), and so is their serialization (`:6056-6061`).
  - **Consequences on CSF:**
    - `compiler/pan_compiler.c:292`, `:313-315`: the SampleMask write sets `writes_coverage`, which makes `can_fpk = false`.
    - `lib/pan_earlyzs.c:61-76`: `late_coverage && writes_zs_or_oq` forces the ZS update to FORCE_LATE.
    - `vulkan/csf/panvk_vX_cmd_draw.c:2675-2679`: FPK is off. `:2694-2696`: the early-ZS LUT.
    - `vulkan/panvk_cmd_draw.h:555-559`: `fs_required()` is true for any FS that writes SampleMask. So the FS runs even when every colour write is masked.
  - **Feature exposure.** `.depthBounds = true` is set on every arch (`vulkan/panvk_vX_physical_device.c:339`).
- **Introduced by:**
  - csf-v11/092 (4297cc05): depthBounds through the FS on v10/v11.
  - 116: it first kept FPK while the test is off, then dropped that ("correctness over the FPK win").
  - v9 (JM) has the same lowering and no alternate compile.
- **What is wasted:**
  - **Why every DXVK fast-linked FS is lowered.**
    - On the X6, DXVK reports `depthBounds : 1` and `graphicsPipelineLibrary : 1` (cube DXVK log).
    - Its fragment-shader libraries are compiled with the DBT enable dynamic (076's own analysis: "almost always under DXVK (the enable is dynamic)").
    - D3D9/D3D11 never turn the test on. DXVK only uses it for the NVAPI depth-bounds extension.
  - **What every draw on a fast-linked pipeline pays:**
    1. **No FPK.** A later opaque fragment cannot kill queued earlier fragments at the same pixel.
    2. **Late ZS update** with depth writes or an active occlusion query. Early kill still happens, but fragments in flight do not see each other's depth, so more of the overdraw is shaded, and fragments at the same pixel serialize on the late update.
    3. **The FS runs even when nothing is written.** In depth-only passes that keep a PS bound with colour writes masked, the whole PS runs per fragment instead of no FS at all.
    4. **A small fixed cost in every FS:** the uniform `ds.enable` branch and the SampleMask store.
  - **Which draws are affected.**
    - DXVK compiles optimized monolithic pipelines in the background. If those keep the enable static, which is likely but not verifiable here (there is no DXVK source on this machine), they are not lowered.
    - The cost then sits on draws that still use fast-linked pipelines.
    - With 2265 graphics libraries in FO4 and this phone's CPU, that phase is long.
- **Expected gain:**
  - **Overdraw.** In unsorted opaque passes, FPK plus the early update typically remove a large share of the hidden fragments (rough: 20-40 % of shaded fragments). At 2400x1080 and FO4-class FS cost, that is 0.5-3 ms per full-screen opaque pass on affected draws.
  - **Depth-only passes with a bound PS:** the whole FS cost goes away.
  - **Cube:** about zero (no overdraw).
  - The v7 port (076) has no device A/B yet either.
- **Risk:** low-medium.
  - The per-draw selection must use exactly the `db_on` predicate of `build_dcd_flags()` (`csf/panvk_vX_cmd_draw.c:2632-2635`), so that every draw with the test on gets the lowered FS.
  - Switching between the two compiles must dirty the FS descriptors and push uniforms. jm-v7 098 (rebind) is the precedent.
  - Un-guarding the serialization changes the shader cache format on v9+, so the binary version must be bumped.
  - Compile cost: such FS are compiled twice (perf-review-2026-10-22 item 8). Use the late-clone variant described there to share the front end.
- **Fix:** port jm-v7 076 to v9+.
  1. Enable `panvk_v7_wants_alt()`'s FS case on every arch, and keep its VS/xfb case v6/v7-only.
  2. Pick the main or the alt FS per draw in CSF `prepare_draw()`, before `fs_required()` and `build_dcd_flags()`, and set FS dirty when it changes.
  3. Un-guard the serialization.
  - **Knob:** `PANVK_DB_ALT=0` means always the lowered FS (the 092/116 behaviour).
  - **Interim A/B knob, no port needed:** `PANVK_DEPTH_BOUNDS=0` hides `depthBounds`. That is the pre-092 behaviour on v10/v11: DXVK then declares no depth-bounds state at all.
- **How to measure:**
  - **Correctness:** PanProbe depth_bounds, depth_stencil, occlusion_query and blend, with both knob values.
  - **New PanProbe overdraw bench:** N full-screen opaque quads drawn back to front with depth writes, through a pipeline library whose DBT enable is dynamic. Measure ms/frame for each knob value.
  - **Q1** (GPL off in dxvk.conf).
  - **FO4** menu and interior fps (out of swap) with DXVK HUD `frametimes,gpuload,pipelines`.
  - **A `db_lowered_off` counter** in the panvk-mem line (draws with `fs->fs.depth_bounds && !db_on`), to get the share of affected draws per frame.
- **Confidence:** high that each affected draw loses FPK, the early update and FS skipping (verified in code). Medium on the share of draws in steady state.

### C3. Tessellation emulation: about 8 dependent dispatches and full drains per tess draw

- **Where:** `vulkan/csf/panvk_vX_cmd_draw.c:5248-5468`, the `gpu_prerast_tess()` loop.
  - **Compute subqueue, per iteration:**
    1. `gpu_prerast_sync32_take(TESS_FREE)`.
    2. `panlib_tess_setup_step` (one workgroup), then a wait.
    3. VS over the control points, then a wait.
    4. TCS, then a wait.
    5. `panlib_tess_count_draw` (one workgroup), then a wait.
    6. `panlib_tess_gen`, then a wait.
    7. TES as the gpu_prerast VS (plus GS and XFB), then a wait.
    8. `panlib_vp_runs_launch_struct` (one workgroup), then a wait.
    9. `give(TESS_READY)`.
  - **Vertex/tiler side (`:5414-5460`):** take(TESS_READY), draw, wait for everything, load `more`, then give(TESS_FREE).
  - **The waits.** Each one is `cs_wait_slots(dev->csf.sb.all_iters_mask)`, a full drain of the subqueue.
- **Introduced by:**
  - csf-v11/065 (682eef65).
  - The tess_opt series (`vulkan/panvk_vX_device.c:872-880`): 213 fuse, setup-in-step, 2 arenas, inline winding.
  - 219 keeps the fused workgroup at 4 KiB of stack per thread, because the tessellator's dynamically indexed arrays live on the stack.
- **What is wasted:**
  - **The chain.** Each tess draw is a chain of small dependent dispatches. The one-workgroup steps leave the GPU nearly idle, and every boundary costs a drain plus a launch.
  - **At L=1 the chain is nearly all fixed cost.** PanProbe tess_perf, 256 draws × 16 patches:

    | device | build | µs per tess draw (L=1) | µs per draw, no tessellation (L=ref) |
    |---|---|---|---|
    | X6 | 112, on battery | 467.9 | 4.5 |
    | panther (v10) | 114 | 156.6 | 2.4 |
    | shiba (v11) | 114 | 178.8 | 1.5 |

  - **At L=16:** 2.3-2.8 ms per draw. The stack-resident tessellator arrays (219) add load/store traffic for every generated vertex.
  - **FO4 does tessellate in game,** contrary to fo4-memory-analysis 2.6. The 112 run committed the tessellation arena (`gpu_prerast tessellation arena committed (96 MiB)` in `mesa-fo4.log`). The likely source is the god rays: NVIDIA Volumetric Lighting builds its light volumes with tessellation.
  - **Per-frame cost.** At 0.2-0.5 ms per draw, 10 tess draws per frame are 2-5 ms of compute-subqueue latency, and the vertex/tiler subqueue waits for each draw's chain.
- **Expected gain:** halving the dependent steps saves about 80-230 µs per tess draw (tess_perf L=1/L=4). The FO4 gain is that saving times the tess draws per frame. Q3 bounds it.
- **Risk:** medium-high. The work touches the 24 tessellation cases, gs_tess_primitive_id, tess_cond_state, XFB and pipeline-statistics queries.
- **Fix (project):** `PANVK_TESS_BATCH=0` restores the per-draw chains.
  1. **Batch consecutive tess draws** with the same shaders into one chain: one setup/count/gen/vp_runs per batch, with per-draw ranges in the tess draw table.
  2. **Merge steps.** Run VS and TCS in one dispatch for patches that fit a workgroup (like a merged LS-HS), and fold `vp_runs` into `tess_gen` or the TES pass.
  3. **Move the tessellator's indexed arrays** off the stack, into workgroup shared memory or bcsel trees. This addresses the L=16 cost.
- **How to measure:**
  - First Q3, plus a `tess_draws` rate in the panvk-mem line.
  - Then tess_perf `us_draw` at L=1 (fixed cost) and L=16, with the hashes unchanged.
  - Correctness: PanProbe tessellation, gs_tess_primitive_id, tess_cond_state and xfb.
- **Confidence:** high for the per-draw cost (measured). Low for its share of an FO4 frame until it is counted.

### C4. The preload barrier drains the fragment subqueue at every pass that loads an attachment

- **Where:** `vulkan/panvk_vX_cmd_draw.c:709-733`, `cmd_init_render_state()` (upstream `^5a07217f`).
  - **When it fires.** The pass is not resuming, and `pan_fb_has_image_load()` (`lib/pan_fb.h:294`) is true: an in-bounds LOAD on any colour, Z or S attachment, or border loads when the render area is partial (`pan_fb.h:176`).
  - **What it records.** A full `CmdPipelineBarrier2`:
    - src: EARLY_FRAGMENT_TESTS|LATE_FRAGMENT_TESTS|COLOR_ATTACHMENT_OUTPUT, with COLOR_ATTACHMENT_WRITE|DEPTH_STENCIL_ATTACHMENT_WRITE;
    - dst: FRAGMENT_SHADER, with SHADER_SAMPLED_READ.
  - **What it becomes on CSF.** A fragment-subqueue self-wait on all iterators (`csf/panvk_vX_cmd_buffer.c:727`, `:843`), plus an "others" (texture) invalidate (`:731-781`).
- **What is wasted:**
  - **The barrier's purpose.** The preload reads through the texture unit, so it has to see earlier attachment writes.
  - **The waste.** It waits for all earlier fragment jobs even when none of them wrote the attachments this pass loads.
  - **Why DXVK hits it so often.** DXVK records a D3D11 frame as many render passes, and most of them use LOAD: every render-target rebind without a clear, and every split for a copy, a UAV barrier or a compute pass.
  - **Effect.** Consecutive passes on different targets cannot overlap their fragment jobs. Each pass waits for the previous pass's last tiles.
- **Expected gain:** each LOAD pass that follows fragment work without a drain has a bubble of about one tile per core plus the RUN_FRAGMENT start (estimate: 10-50 µs). That is 0.3-1.5 ms per frame with 30 such passes. Nothing for the cube, which has one pass.
- **Risk:** medium. A missed real read-after-write gives stale preloads: wrong pixels, intermittently.
- **Fix:** track, per command buffer, the images (or BO ranges) that fragment jobs have rendered since the last fragment self-wait.
  - In `cmd_init_render_state()`, keep the full barrier when a loaded attachment is in that set, or when the state is unknown: the first LOAD pass of a command buffer, and secondaries.
  - Otherwise, emit only the texture-cache invalidate on the fragment subqueue.
  - **Knob:** `PANVK_PRELOAD_BARRIER_TRACK=0` restores the unconditional upstream barrier.
  - **JM:** the same code there is a batch split (reviewer D's area).
- **How to measure:**
  - PanProbe 39/39 and CTS `dEQP-VK.renderpass2.*` / `dEQP-VK.dynamic_rendering.*`, with both knob values.
  - A PanProbe "pass chain" bench: N passes alternating two render targets with LOAD and small draws. Measure ms/frame.
  - Fragment job start/end gaps in u_trace or perfetto.
- **Confidence:** high that the drain is emitted for every LOAD pass. Medium on the size of the gain.

### C5. An FO4 shader needs more than 4 KiB of stack per thread

- **Where:**
  - Device TLS sizing in `csf/panvk_vX_cmd_buffer.c` `get_device_tls()` (097, 227, 232).
  - `compiler/bifrost/bifrost_nir.c:1159-1192`: function-temp arrays that are indexed indirectly and have more than 8 elements stay in memory and become TLS. The passes are `nir_lower_indirect_derefs_to_if_else_trees(..., 8)` at `:1167`, then `nir_lower_vars_to_explicit_types` and `nir_lower_explicit_io`.
  - Register-allocation spills.
- **What is wasted:**
  - **The evidence.** In the 112 FO4 run, a single device TLS BO of 336 MB appears when gameplay starts. Spread over 21 core IDs × 2048 threads, that is 8 KiB per thread, so some shader asked for more than 4 KiB.
  - **Which shader.** Since 219, no libpan kernel needs more than 4 KiB, so this is a game (DXVK) shader. It is either an indexable temp (`dcl_indexableTemp`, which DXVK emits as a Function-storage array) or heavy spilling.
  - **The cost.** Every access to such an array, and every spill or fill, is a load/store round trip through L2 instead of a register access.
- **Expected gain:**
  - GPU: unknown until the shader is identified. A hot full-screen shader would cost milliseconds.
  - Memory: dev_tls would drop from 336 to 168 MB if the shader fits in 4 KiB (reviewer B).
- **Risk:** low for the diagnostic. Medium for a threshold change, because of code-size growth.
- **Fix:**
  1. **Diagnose.** Use Q6, or add a one-time `mesa_logi` in PanVK for shaders with `tls_size > 4096` (hash, stage, spills/fills), so it lands in `MESA_LOG_FILE`.
  2. **If it is a small vec4 indexable temp:** raise the if-else-tree threshold under a knob. `PANVK_INDIRECT_TEMP_MAX=8` restores today's threshold.
  3. **If it is register-allocation spilling:** hand the shader to the RA work (152-164) as a test case.
- **How to measure:** the log line, then FO4 interior fps (out of swap) and `dev_tls` in the panvk-mem line.
- **Confidence:** medium. The TLS size is measured, but its cause is not identified.

### C7. CRC (transaction elimination) mostly costs on DXVK

- **Where:**
  - **Eligibility.** `vulkan/panvk_image.c:440-480`, `panvk_should_checksum()` (upstream). CRC requires 2D, one layer, colour-attachment usage, no MUTABLE (`:465-466`), no STORAGE, and at most 4 B per pixel (`:476-479`).
  - **Invalidation.** `vulkan/csf/panvk_vX_cmd_draw.c:6526-6574`, `invalidate_initial_attachment_crcs()`. csf-v11/019 (4ba693b1) invalidates the CRCs at every non-resuming pass with loadOp CLEAR or DONT_CARE (`:6542-6547`).
  - **v10.** `mark_crc_valid_after_fragment()` (`:1764-1790`, upstream) waits for all fragment iterators (`:1780`) after a pass whose CRC was invalid, before it marks the CRC valid.
- **What is wasted:**
  - **What CRC saves.** Transaction elimination skips the writeback of tiles that did not change since the last frame.
  - **Why it never fires on DXVK.** D3D games clear their targets every frame, and DXVK turns those clears into loadOp CLEAR, so 019 invalidates the CRCs every frame. The hardware then writes every tile, and the CRC buffers (8 B per tile) are pure overhead.
  - **v10 extra cost.** Each such pass also drains the fragment subqueue once.
  - **Targets with no CRC at all.** RGBA8 targets that DXVK creates MUTABLE (UNORM/SRGB view lists), and every 8 B/px target such as RGBA16F.
- **Expected gain:**
  - Now: removing the overhead, which is one fragment drain per CRC-tracked target per frame on v10, plus the CRC traffic.
  - With a proper fix for 019: static frames such as menus would save up to 3.6 MB (1280x705) or 10.4 MB (2400x1080) of writeback per target per frame.
- **Risk:** medium. 019 fixed a real G615 failure: IDVS matrix cases read back zeros after a store was CRC-skipped.
- **Fix:**
  1. Run Q4 first.
  2. **If CRC helps:** invalidate only when the attachment content is undefined: an UNDEFINED initial layout (via `RENDERING_ATTACHMENT_INITIAL_LAYOUT_INFO_MESA`), a memory rebind or aliasing. Do not invalidate on every CLEAR. `PANVK_CRC_KEEP_ON_CLEAR=0` restores the 019 behaviour.
  3. **Allow MUTABLE** when the view-format list only holds bit-identical formats, such as UNORM/SRGB pairs. `PANVK_CRC_MUTABLE=0` restores the exclusion.
  4. **If `no_crc` is equal or better:** turn CRC off for targets that are cleared every frame.
- **How to measure:** PanProbe 39/39 plus 019's test (`tests/dxvk/vulkan/test_crc_invalidate_undefined_clear.py`); menu fps; a log of CRC-eligible images, to confirm DXVK's MUTABLE usage.
- **Confidence:** medium.

### C8. On DXVK devices, every vertex-stage barrier also stalls the COMPUTE subqueue

- **Where:**
  - `vulkan/csf/panvk_vX_cmd_buffer.c:670-684`. When `gpu_prerast` is true, any barrier whose destination stages include draw-indirect, vertex input, any pre-rasterization stage or ALL_GRAPHICS also makes COMPUTE wait for the source subqueues.
  - `:805-806` passes `dev->gpu_prerast_wanted || dev->gpu_prerast_tess_wanted`.
  - `vulkan/panvk_vX_device.c:862-869` sets those flags as soon as any of geometryShader, transformFeedback, fillModeNonSolid, vertexPipelineStoresAndAtomics or tessellationShader is enabled.
  - DXVK enables all of them, so this is on for every DXVK device, even in frames without a single gpu_prerast draw.
- **Introduced by:** csf-v11/020 (09497257), 045 (522ccc0e) and 065 (682eef65), with the device flag from 205 (497140cc).
- **What is wasted:**
  - A barrier such as "colour writes → vertex and fragment shader reads" also puts a compute-waits-for-fragment wait into the compute stream.
  - A later D3D11 Dispatch that does not depend on that barrier is then serialized behind the fragment work.
  - Each such barrier also costs one more syncobj wait.
- **Expected gain:** small. It only matters in frames with compute work after such barriers (FO4 has compute passes). Estimate: under 0.5 ms per frame.
- **Risk:** medium (synchronization).
- **Fix:** make the dependency lazy.
  - Record the pending source mask on the command buffer.
  - Emit the compute-side wait at the next gpu_prerast launch (next to `gpu_prerast_wait_compute()`, `csf/panvk_vX_cmd_draw.c:3691`), or at the end of the command buffer, because the barrier's scope continues into later submissions.
  - **Knob:** `PANVK_PRERAST_LAZY_DEP=0` restores the eager 020/045 wait.
- **How to measure:** PanProbe gpu_prerast_slice, geometry, xfb, vertex_stores and tessellation for correctness; a bench with a compute dispatch after render-target → VS barriers.
- **Confidence:** medium that the cost exists (code). Low on the size of the gain.

### C9. Renewed tiler heaps start at 4 chunks and grow through kernel round trips

- **Where:**
  - `vulkan/csf/panvk_vX_gpu_queue.c:2112-2150`, `kbase_heap_chunks()` (csf-v11/224, e0ad1f6a). The default is 4 initial and 400 max chunks of 1 MiB; `10,400` was the old behaviour.
  - Heaps are renewed after about every big submit (`KBASE_TILER_HEAP_RENEW_WORK`, `:81`), because recycling has been disarmed on kbase since 098.
- **What is wasted:**
  - The tiler heap holds the polygon lists and the IDVS vertex packets (varyings) of a pass.
  - A heap that outgrows its initial chunks gets each extra 1 MiB chunk through a tiler OOM event, which the kernel serves by allocating and zeroing a chunk. The tiler waits meanwhile.
  - Renewal throws the grown size away, so the next heap grows again.
  - FO4 112 renewed 4680 times in 402 s: about 11.6 per second, or 1-3 per frame at 4-8 fps.
- **Expected gain:** unmeasured. It depends on the chunks needed per frame and on the kernel's service time.
- **Risk:** low-medium, on the memory side.
- **Fix:**
  - Run Q5 first, on the FO4 interior out of swap, the menus and PanProbe large_draw.
  - **If it helps:** create renewed heaps at a per-queue high-water mark, clamped and decaying. `PANVK_KBASE_HEAP_HWM=0` restores the fixed initial count.
  - Agree the footprint trade-off with reviewer B.
- **How to measure:** Q5, then the same runs with the high-water mark.
- **Confidence:** low-medium.

### C10. Blend shaders for common float render-target blends and MIN/MAX

- **Where:**
  - `lib/pan_blend.c:40-46` and `:63-86` (upstream, all archs). On float formats, only ZERO/ONE/SRC_ALPHA/INV_SRC_ALPHA (plus two ADD-with-zero cases) are fixed function. MIN/MAX never are.
  - `vulkan/panvk_vX_blend.c:229-283`, `blend_needs_shader()`. A blend shader is also used for logic op, alpha-to-one, unblendable formats and non-uniform blend constants.
  - `vulkan/csf/panvk_vX_cmd_draw.c:2718-2720`: with MSAA, a blend shader forces per-sample shading.
- **What is wasted:**
  - HDR passes on RGBA16F or R11G11B10F that use DST_COLOR, SRC_COLOR, DST_ALPHA or their INV_ forms (multiplicative decals, some particles) run a blend shader per fragment. So do MIN/MAX blends.
  - With MSAA, the whole FS runs per sample.
- **Expected gain:** unknown. Count first.
- **Risk:** medium: blend precision, and CTS `dEQP-VK.pipeline.*blend*`.
- **Fix:**
  1. Count blend-shader draws per frame: one more field in the panvk-mem line, or a `vk_perf` line per blend-shader key.
  2. If the count is significant, check whether v10/v11 hardware supports more fixed-function factors for float formats. The rule is upstream's, for all archs, and the tree does not document it per arch.
  3. If the hardware allows more factors, widen `factor_is_supported_for_float()` per arch, behind `PANVK_FF_FLOAT_BLEND=0` to restore.
- **How to measure:** the counter on FO4 and the menus, then PanProbe blend and CTS blend.
- **Confidence:** low.

## 4. Checked, and not a finding (or already done)

- **AFBC on v10/v11 DXVK targets** is generally available:
  - `panvk_image_can_use_mod()` (`vulkan/panvk_image.c:166-300`) only refuses MUTABLE on arch ≤ 7 (`:232-235`).
  - `lib/pan_afbc.h:506-610` maps formats through `util_format_any_to_unorm` on v9+, so RGBA16F and R11G11B10F are AFBC-capable.
  - D24S8 is planar Z24+S8 on v9+ (`panvk_image.c:101-108`), so ZS AFBC survives `separateDepthStencilLayouts` (`:241-247`).
  - What still blocks AFBC: STORAGE or HOST_TRANSFER usage (`:200-203`), which means UAV render targets; and MSAA, because `pan_mod_afbc_get_format_caps()` has no MSAA cap (`lib/pan_mod.c:180-217`, upstream/hardware).
- **In-pass BY_REGION barriers** become a full-screen primitive-barrier DCD (`csf/panvk_vX_cmd_buffer.c:808-816`, then `cmd_fb_barrier()` in `csf/panvk_vX_cmd_draw.c:6912-6950`). There is no tile store or reload.
- **Tiler hierarchy mask** is upstream (`vulkan/panvk_cmd_draw.h:505-518`, then `lib/pan_desc.c:1770-1830`). At 2400x1080, all bin pointers (about 110 KB) fit the `chunk_size - 64` budget with 1 MiB chunks, so no level is dropped.
- **Resolves** fold into the store when possible (`vulkan/panvk_vX_cmd_draw.c:704-706`). Colour preloads use INTERSECT unless `always` is set (`panvk_vX_cmd_frame_shaders.c:893-895`).
- **ClearAttachments** always draws through vk_meta (`panvk_vX_cmd_meta.c:342-382`). DXVK folds most clears into loadOp CLEAR, so this is not ranked.
- **Varyings** use compact packing (`compiler/pan_nir_collect_varyings.c:382-417`) and fp16 only for mediump (`vulkan/panvk_vX_shader.c:3486`). DXVK emits highp, and there is nothing safe to change.
- **Large constant arrays** go to constant memory (`nir_opt_large_constants`, 32 B threshold, `vulkan/panvk_vX_shader.c:3309`).
- **XFB** is only lowered while transform feedback is active (`csf/panvk_vX_cmd_draw.c:3989-3997`). A VS only goes through gpu_prerast when it writes memory (`:5489-5495`).
- **BC uploads:**
  - The default path decodes or transcodes straight from the copy source, with no internal barrier (`panvk_vX_cmd_meta.c:1821-1827`).
  - ALL_COMMANDS barriers remain only on image-to-image copies and on the zero-initialized and compute-decode paths (`:1638-1665`, `:1763`, `:2233`, `:2387`, `:2508-2552`).
  - bc_perf (45-69 s) is an upload-time cost. The lowmem ETC2/EAC shadows sample at native ETC2 cost.
- **Tessellation optimisations already done:** 2 arenas (the compute side of draw N+1 overlaps the draw of N), setup in the step, fusion and inline winding. C3 is what remains.
- **Occlusion queries** cost one L2/LSC clean per pass that ends queries (`csf/panvk_vX_cmd_draw.c:7442-7480`). That is upstream and cheap next to the fragment job.
- **Tile size selection** is upstream (`vulkan/panvk_vX_cmd_draw.c:736-760`).

## 5. Still open from the 2026-10-22 review (GPU side)

- **K1, the Android-native present path** (`vulkan/panvk_wsi.c:262-290`). Every present runs `DeviceWaitIdle` (`:273`) and a CPU row copy into an AHB locked CPU_READ_OFTEN (`:282-288`).
  - This drains the GPU every frame and removes CPU/GPU overlap.
  - It only affects native Android swapchains (PanProbe swapchain_lifecycle, native apps). DXVK under PanPlay uses the X11 path (C2).

## 6. Side note

- **Positional initializer in `vulkan/panvk_wsi.c:533-536`** (kbase-common/006, 774619c4). The second initializer of `struct wsi_device_options` has no field name, so `kbase_dmabuf && kbase_raw_dri3` sets `extra_xwayland_image`.
  - If that is intended, name the field.
  - If it is not, it adds a swapchain image (memory and latency) on the raw DRI3 path.

## 7. Suggested order

1. Run Q0-Q6 on the X6. No build is needed.
2. Do C2 (a) and (b), plus C6, as one small patch set (csf-v11 233+).
3. Port C1 (076 to v9+), together with the late-clone compile mitigation (2026-10-22 item 8).
4. Do C4 (preload tracking). Do C3 (tess batching and merged steps) if Q3 shows the god rays matter.
5. Do C5, C7, C9 and C10 once their measurements are in.
