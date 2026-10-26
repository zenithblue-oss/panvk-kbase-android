# Perf review 2026-10-26, part D: the JM path (v6 G72, v7 G52, v9 G78)

This is a read-only review: no builds, no device, no code edits.

- **Scope:** CPU overhead on kbase JM (job-chain building, submission, events, per-batch allocation) and JM-specific GPU waste (chain structure, batch splits, tiler heap, varyings, GS/tess emulation, TLS/WLS) on Mali v6 (Galaxy S9 `starlte`, G72 MP18), v7 (Galaxy A31 `a31`, G52 MC2) and v9 (Pixel 6 `oriole`, G78 MP20).
- **Tree:** `/home/flux/src/mesa-rc5g`, tag rc5g-114 (HEAD d324a269). The v6/v7/v9 code has not changed since RC5-universal-111 (jm-v7-wip 0090-0092).
- **Paths:** relative to `src/panfrost/` unless noted. Each finding names the series patch and commit that introduced the code (`git blame`), or says *(upstream)* for Mesa 5a07217f code.
- **Evidence:** the FTL RC5-universal-113 runs of 2026-10-25 (`device-runs/2026-10-25-ftl-{v6-starlte,v7-a31,v9-oriole}-rc5u-113`): `results.md`, logcat, and the tess_perf log under `raw/*/artifacts/sdcard`.
- **Not repeated:** items that the 2026-10-22 review covered and that are done. Its open items are referenced where they still apply (items 1 and 2, K1, K3, K8).

## Measured baseline (113)

tess_perf times one frame: 256 draws of 16 quad patches into a 64x64 RT, fence-timed resubmits of one command buffer. Values are ms per frame (`BENCH ... opt=default`).

| device | ref | L=1 gs0 | L=1 gs1 | L=4 gs0 | L=4 gs1 | L=16 gs0 | L=16 gs1 | mix gs0 | mix gs1 |
|---|---|---|---|---|---|---|---|---|---|
| S9 (v6) | 2.93 | 153.7 | 234.2 | 463.1 | 519.8 | 2266.3 | 1789.1 | 1857.1 | 1748.1 |
| A31 (v7) | 1.91 | 123.5 | 390.8 | 640.5 | 704.6 | 5121.0 | 1694.8 | 2456.5 | 1507.7 |
| Pixel 6 (v9) | 2.14 | 70.3 | 161.0 | 199.2 | 529.9 | 894.2 | 3791.2 | 896.3 | 2006.3 |

Driver lines from the 113 logcats:
- **S9:**
  - `panvk: v6 tess batch budget 32 MiB: new batch (varyings 43 MiB worst case, GS scratch 0 MiB)`
  - `v6 batch TLS 41472 KiB (stack 2324 B/thread), half 0/1, shared per half`
  - `v6 batch WLS 6912 KiB`
  - `kbase: JM warm-up 2000 ms (first JM open in this process)`
- **A31:**
  - the same budget line for v7
  - `v7 batch TLS 8192 KiB (stack 2324 B/thread)`
- **Pixel 6:**
  - `v9 batch TLS 94208 KiB (stack 2336 B/thread)`
  - `v9 batch WLS 368 KiB`

## Ranked findings

| # | Finding | Arch | Expected gain | Risk | Kind | Conf. |
|---|---|---|---|---|---|---|
| D1 | The 0079 tess budget charges the level-64 worst case, so every tessellated draw starts a new batch and spills the framebuffer | v6 v7 | High for tessellating games: one full store+reload per tess draw, about 2-4 ms each at 1080p | low-med (quick cap), med (adaptive) | quick + bigger | high (mechanism), med (game size) |
| D2 | TLS/WLS are kept per command buffer per half, sized for the sparse core-ID range, CPU-cleared, and never trimmed on JM | v6 v7 v9 | 95 MiB (S9) / 185 MiB (Pixel 6) per extra command list with tess or spilling shaders, plus first-use hitches | low (trim, no-clear), med (device TLS) | quick + bigger | high (sizes), med (DXVK count) |
| D3 | An FS descriptor-copy job is built and chained for every draw (the dirty-driven one is dead), and `ptr` is uninitialized on FS-less draws *(upstream)* | v6 v7 | 1-3 us CPU and one GPU job per draw; removes a UB path hit by every depth-only draw | low | quick | high |
| D4 | 2 s JM warm-up sleep in every process (0084) | v6 v7 v9 | 2 s per app start; about 78 s (21-26 %) of each FTL run | low-med | quick A/B | high |
| D5 | A resubmitted command buffer cleans the whole descriptor pool once per batch | v6 v7 v9 | Estimated 10-50 ms per tess_perf frame on v6/v7; makes the PanProbe benches honest | very low | quick | high (mechanism), med (size) |
| D6 | Every dispatch is its own atom and every barrier splits the batch (10-22 items 1 and 2, still open) | all JM | Tens of us to about 0.1 ms per avoided atom; one full spill per in-pass barrier | med | bigger | med |
| D7 | gpu_prerast arenas (64 + 96 MiB) are committed and CPU-cleared in every VkDevice (K3, still open on v9) | v9 | 160 MiB and about 50-100 ms per VkDevice | low-med | quick-ish | high |
| D8 | The upstream 5K-job split lands mid-pass, and every v6/v7 indexed draw costs 3-5 jobs | v6 v7 | One spill per 1000-2500 indexed draws in a pass | med | medium | med |
| D9 | The TES is shaded once per index, not per domain point | v6 v7 | TES invocations and varying bytes /5 at L=16; D1 worst case /6 | med-high | bigger | med |
| D10 | Tess draws serialize through one arena and barrier kernels; a 16-patch draw tessellates on one core | v6 v7 | Up to Kx on the tessellator phase of many small draws (after D1) | med | bigger | med |
| D11 | Per batch: 2 JOB_SUBMIT ioctls, 2 events, and an L2 clean at the end of every vtc atom (K8 multi-atom still open) | all JM | CPU 10-30 us per batch; one L2 clean per batch | med | bigger | med |
| D12 | The v9 2 x 512 MiB GROW_ON_GPF tiler heaps (MALLOC_VERTEX) never give pages back | v9 | 100s of MiB after one heavy scene on 4-6 GB phones | med | bigger | med |
| D13 | Android present waits for device idle and CPU-copies every frame (K1, open; JM has no sync_file) | all (worst on JM) | CPU/GPU overlap; up to about 2x FPS when CPU and GPU times are similar | med | bigger | high |
| D14 | Minor CPU items | | small | none | quick | high |

Open anomalies, with a plan to measure them, are A1 and A2 after the findings.

---

## Quick, safe wins

### D3. An FS descriptor-copy job for every draw; the dirty-driven one is dead; `ptr` is uninitialized on FS-less draws *(upstream)*
- **Arch:** v6, v7. Bifrost descriptor copy jobs; v9 has none.
- **Where:** all upstream 5a07217f.
  - `vulkan/jm/panvk_vX_cmd_draw.c:1743-1764`, `panvk_draw_prepare_tiler_job()`:
    ```c
    struct pan_ptr ptr;
    if (cmdbuf->state.gfx.fs.required) { ... meta_get_copy_desc_job(..., &ptr); }
    if (ptr.cpu)
       util_dynarray_append(&batch->jobs, ptr.cpu);
    draw->jobs.frag_copy_desc = ptr;
    ```
  - The dirty-driven copy, `if (fs_desc_dirty)` at `:2214-2233`, calls `panvk_draw_prepare_fs_copy_desc_job()` (`:1921-1941`). That sets `draw->jobs.frag_copy_desc` first. The tiler-job prep, called at `:2351`, then overwrites it.
  - Only the last value is chained (`:3179-3186`, `:3289-3296`).
  - The builder is `vulkan/bifrost/panvk_vX_meta_desc_copy.c:388-482`. It allocates three descriptors (the push-uniform `copy_info`, COMPUTE_JOB and LOCAL_STORAGE) and looks up the copy RSD.
- **What is wasted:**
  - Every draw that runs the FS gets a COMPUTE copy job. That job copies unchanged FS descriptors into the same tables, because the tables are reallocated only when `fs_desc_dirty` is set (`vulkan/panvk_vX_cmd_desc_state.c:225-246`).
  - This undoes the FS half of the 10-22 item 4 fix (the per-set dirty mask, now at `:2161-2172`).
  - When the sets are dirty, the first copy job is built and appended to `batch->jobs`, but it is never chained.
  - Every chained copy job adds to the 5K split (D8).
- **Correctness (UB):**
  - A draw with no FS bound (every DXVK depth-only or shadow draw), or with an FS that is not required, reaches `:1761` with `ptr` uninitialized.
  - The release build (`scripts/build-android.sh:69`, buildtype=release, no `-ftrivial-auto-var-init`) does not zero it.
  - Whatever is on the stack is appended to `batch->jobs`. A resubmit then memsets 16 bytes at that address (`jm/panvk_vX_gpu_queue.c:116-117`). If `.gpu != 0`, the value is also chained as a job (`:3179`).
  - The tests pass, so the slot is probably zero in practice. That is luck, not a guarantee.
- **Gain:**
  - CPU: one copy-job build less per draw, about 1-3 us on A55/A75/M3, so 1.5-4.5 ms per 1500-draw frame on CPU-bound v6/v7 phones.
  - GPU: one compute job less per draw, about 20-25 % of the jobs of a v6 indexed draw.
  - Fewer 5K splits (D8).
- **Fix:**
  - Write `struct pan_ptr ptr = {0};` (no knob needed).
  - Drop the tiler-job copy and keep the dirty-driven one.
  - So that a required-ness flip (for example color write enables) still rebuilds the FS tables, compute `fs_desc_dirty` with `fs_user_dirty(cmdbuf)` (`vulkan/panvk_cmd_draw.h:590-598`) instead of `gfx_state_dirty(cmdbuf, FS)` at `:2170-2172`. Today the `fs == NULL` branch at `:2215-2218` zeroes the tables, and nothing rebuilds them when the same FS later becomes required.
  - Knob: `PANVK_V7_FS_COPY_ONCE=0` = old (a copy per draw).
- **Risk:** low.
- **Measure:**
  - FTL a31 and starlte: draw_params, descriptor_model, depth_stencil, blend, multi_viewport and dxvk_reqs for correctness.
  - For CPU cost, a PanProbe CPU bench recording 2000 indexed draws that rebind one FS set every 8 draws: us per `vkCmdDrawIndexed` (clock_gettime around recording), and jobs per batch (a one-shot log of `vtc_jc.job_index` at batch close).
- **Confidence:** high on the code. Medium on the us figure.

### D5. A resubmitted command buffer cleans the whole descriptor pool once per batch
- **Arch:** v6, v7, v9.
- **Where:**
  - `vulkan/jm/panvk_vX_gpu_queue.c:149-151` *(upstream)*, inside `panvk_reset_issued_batch()`:
    ```c
    /* We don't keep track of BO <-> job relationship, so let's just flush the
     * whole desc pool for now. */
    panvk_pool_flush_maps(&cmdbuf->desc_pool);
    ```
  - It is called for every issued batch by the pre-submit reset loop at `:464-473`. That loop comes from series jm-v9/030 (05ccf10f) and runs before any batch is submitted. `:169-171` is the same call on the submit path.
  - `vulkan/panvk_mempool.c:277-305` walks every desc BO. csf-v11 221 trims only the transient BO's tail.
  - With the default userspace cache ops (`lib/kmod/kbase_kmod.c:1930-1939`, `lib/kmod/pan_kmod.c:334-350`), each call runs `DC CVAC` over every 64-byte line of the pool.
- **What is wasted:**
  - Each resubmit costs batches x pool bytes of cache maintenance.
  - tess_perf on v6/v7 has 256 batches per frame (D1). With an estimated 1-1.5 MiB pool (about 4-6 KiB of descriptors per tess draw), that is about 0.25-0.4 GiB of `DC CVAC` (4-6 M lines) per frame, all before the first atom, while the GPU is idle.
  - At 2-10 ns per line that is about 10-50 ms of a 123 ms (A31) or 154 ms (S9) L=1 frame.
  - Lines are clean after the first pass, so the repeats only burn time. `DC CVAC` writes back dirty lines only, so flushing once, or flushing only the lines the reset wrote, gives identical memory contents.
- **Who pays:**
  - Every PanProbe bench that resubmits a recorded command buffer (tess_perf; check bc_perf, large_draw and submit_stress). This biases the numbers used to judge every other change.
  - Real apps pay only when they resubmit. DXVK and vkd3d re-record, so `issued` stays false.
- **Fix:**
  - Flush once per command buffer after the reset loop. On v<=9 every batch is reset there, so one flush covers them all.
  - Better: flush only the ranges the reset rewrote (job headers, tiler heap descriptor and contexts, `vary_infos`, MALLOC_VERTEX arrays). With userland cache ops that needs no BO lookup (`util_flush_range_no_fence` plus the existing pending-fence flag).
  - Knob: `PANVK_JM_RESET_FLUSH=batch` = old; default `ranges`; `once` in between.
- **Risk:** very low. The same lines are cleaned, the ordering is unchanged, and all resets still precede all submits.
- **Measure:**
  - FTL starlte, a31 and oriole: tess_perf L=1 and L=4, gs=0/1, before and after.
  - Add a CPU-time print around `vkQueueSubmit` to tess_perf, so CPU submit time and GPU time can be told apart.
  - Add a one-shot log of desc-pool bytes at `EndCommandBuffer` to turn the estimate into a number.
- **Confidence:** high on the mechanism. Medium on the ms.

### D4. A 2 s JM warm-up sleep in every process (jm-v7-wip 0084)
- **Arch:** v6, v7, v9.
- **Where:** `lib/kmod/kbase_kmod.c:1732-1789` (`kbase_jm_warmup`), called from `dev_create` at `:1838` after the atom-stride probe. Introduced by jm-v7 111 (f783235e). The first JM open is `vkEnumeratePhysicalDevices` (`vulkan/panvk_physical_device.c:382`).
- **What is wasted:**
  - 2.0 s of `nanosleep` once per process. The code comment says the reason is not known: "the probe atoms are dependency-only (jc 0), the kernel completes them without the GPU".
  - Every PanProbe test is a process. The sub-second tests take about 2.1-2.5 s on all three devices (A31 and oriole tables in the 113 `results.md`), so the warm-up is about 39 x 2 s = 78 s of a 364 s (A31) or 300 s (oriole) run.
  - Every game start pays 2 s too.
  - Smaller, in the same open: the jm-v7 109/110 timestamp calibration (`:633-720`) runs on both device opens of a process. That is 2 x 5 ms windows per open where TIMEINFO works (A31). It fails at once on the S9.
- **Fix:**
  - One FTL A/B run per arch with `PANVK_KBASE_JM_WARMUP=0`, checking 39/39 and no DEVICE_LOST, `event=0x` or fault lines.
  - If clean, make 0 the default and keep `=1` for the old behaviour. The knob is read with `getenv`, so PanProbe must set it before the first `vkCreateInstance`.
  - Cache the calibration result per process (static plus mutex, like the warm-up).
- **Risk:** low-medium, since the original reason is unknown. The knob reverts it.
- **Measure:** per-test `durationMs` in the FTL results on starlte, a31 and oriole; the `kbase: JM warm-up` line.
- **Confidence:** high.

### D7. v9: gpu_prerast arenas committed and CPU-cleared in every VkDevice (K3, still open on v9)
- **Arch:** v9.
- **Where:**
  - `vulkan/panvk_vX_device.c:155-162`: `gpu_prerast_arena_eager()` returns `PAN_ARCH < 10 || ...`. The CSF lazy path came in with csf-v11 205 (497140cc).
  - Eager creation at `:884-893`. Sizes in `vulkan/panvk_gpu_prerast.h:26` (64 MiB) and `:324` (96 MiB).
  - The two v9 readers use the fields directly: `vulkan/jm/panvk_vX_cmd_draw.c:5802` and `:6305`.
- **What is wasted:**
  - Every VkDevice that enables GS, XFB, tess, non-solid fill, viewport index or vertex stores commits 160 MiB. That is every DXVK device.
  - The arena is WB-mapped, so `kbase_kmod_bo_alloc` memsets all 160 MiB and runs MEM_SYNC on it (`lib/kmod/kbase_kmod.c:2697-2709`). This happens whether or not a prerast path ever runs.
- **Fix:**
  - Route the two readers through `panvk_per_arch(device_gpu_prerast_arena)(dev, tess)`, which commits fully at first use at record time, before any submit. That is why the CSF fault-growth problem does not return.
  - Then let `gpu_prerast_arena_eager()` return false on v9. `PANVK_GPU_PRERAST_ARENA=eager` = old.
- **Gain:** 160 MiB committed and about 50-100 ms of CPU clear per VkDevice on G77/G78/G68 phones, many of which have 4-6 GB.
- **Risk:** low-medium. Every v9 reader must use the accessor. `gpu_prerast_wanted` gating is unchanged.
- **Measure:** FTL oriole: the `gpu_prerast ... arena committed` lines appear only in tests that use the paths (geometry, xfb, tessellation, gpu_prerast_slice, gs_viewport_depth); tess_perf MEM lines; all 39 PASS.
- **Confidence:** high.

### D2 (quick part). TLS/WLS: trim and no-clear
See D2 below for the full picture. These parts are cheap:
- **Command-pool autotrim on JM:**
  - `vulkan/panvk_cmd_pool.c:124-132` gates csf-v11 226/231 autotrim to `pan_arch >= 10`.
  - DXVK resets pools without RELEASE_RESOURCES, so a JM pool keeps every TLS, desc and varying BO it ever held.
  - Fix: lift the gate. `PANVK_JM_CMD_POOL_AUTOTRIM=0` = old.
- **No CPU clear for TLS/WLS BOs:**
  - They are GPU scratch with undefined contents and are never VkDeviceMemory. K4 kept the generic memset for VK zero-init, and that reason does not apply here.
  - Fix: add a kmod "no clear" flag for the `tls` pool (`vulkan/jm/panvk_vX_cmd_buffer.c:672-682`). `PANVK_JM_TLS_CLEAR=1` = old.
  - Without it, the first use of each per-half TLS BO runs a 40.5 MiB (S9) or 92 MiB (Pixel 6) CPU memset through the mapping. That is a 10-30 ms hitch.
- **Diagnostics:** log `shader_present` and `core_id_range` once, as csf-v11 230 does for CSF, and add the shader name to the `batch TLS` line. The core-ID range below is inferred.

### D14. Minor CPU items
- **Uncached option read per tess draw:**
  - `vulkan/jm/panvk_vX_cmd_draw.c:3899` calls `debug_get_num_option("PANVK_TESS_WINDING_INLINE", 1)` on every v6/v7 tess draw (jm-v7 104, f304d35f).
  - The device already caches it at `vulkan/panvk_vX_device.c:879-880` (`tess_opt.winding_inline`). Each call is a `getenv` plus a mutex and hash lookup (`util/os_misc.c:212-246`), about 1 us.
- **Redundant `poll(0)`:**
  - `lib/kmod/kbase_kmod.c:3157-3166` polls again before the first `read()` right after `poll(timeout)` reported POLLIN.
  - That is one syscall per drain (0087 left it in). Skip the poll on the first iteration.
- **Point size in the budget:** the `+ 4` in the D1 `vary_w` and in `t->varying_bytes` (`:3915-3916`) counts point size for every TES. It is 7 % of the charge when the TES writes no PointSize and the draw is not points mode.

---

## Bigger projects

### D1. v6/v7: the tess batch budget charges the level-64 worst case, so every tessellated draw starts a new batch, and every batch after the first spills the framebuffer
- **Arch:** v6, v7.
- **Where:**
  - `vulkan/jm/panvk_vX_cmd_draw.c:3819-3864`, jm-v7 106 = 0079 (40ee4b65):
    ```c
    const uint64_t vary_w = (uint64_t)max_indices * layers *
          (16 + 4 + tes_v->info.varyings.formats.generic_size_B) + layers * 3 * 64;
    ...
    const bool over = used && (b->tess_vary_w + vary_w > tess_budget || ...);
    if (over || jobs) { cmd_close_batch(); cmd_open_batch(); ... }
    ```
  - The bound at `:3745-3757` (0079, with 0078 = 6fcdb883): `max_indices = MAX2(cpu_patches, 1) * PANVK_TESS_MAX_PRIMS_PER_PATCH * vpp`. `PANVK_TESS_MAX_PRIMS_PER_PATCH` is 8704 (`vulkan/panvk_gs.h:335`).
  - Knob: `PANVK_V7_TESS_BATCH_MB`, default 32 (`vulkan/panvk_v7_knobs.h:156-166`).
  - The spill: `vulkan/jm/panvk_vX_cmd_buffer.c:250-253` (`.load = needs_load ? &fb.load : &fb.spill.load`, the same for store) and `:312-313` ("Everything from now on should spill").
- **What is wasted:**
  - A 16-patch draw is charged 16 x 8704 x 3 = 417,792 indices x about 54 B = 21.5 MiB. The logs show "varyings 43 MiB worst case" when a second draw would join, so each batch holds exactly one tess draw.
  - tess_perf is therefore 256 batches (512 atoms) per frame, against 1 batch for L=ref.
  - The real TES output at L=16 (fractional_odd 16 gives 578 triangles per patch, per the 0090 diagnosis) is 27,744 indices, about 1.4 MiB, 15 times less. At L=1 it is 96 indices, about 5 KiB.
  - Each batch after the first in a render pass adds a new FBD and fragment job, two atoms, and the spill path: every attachment, color and ZS, is stored and then reloaded by the preload frame shaders.
  - At 1280x720 RGBA8 + D24S8 that is about 15 MB of traffic per split; at 1080p about 33 MB, roughly 2-4 ms on G52/G72-class memory.
  - tess_perf's 64x64 RT (16 tiles) hides this cost, so PanProbe understates it.
- **Gain:**
  - High for D3D11 titles that tessellate on v6/v7. N tess draws in a pass cost N-1 full spills today.
  - In tess_perf it is low-medium: the batch and atom overhead is part of the 482-600 us per draw at L=1.
- **Fix, staged:**
  1. **Quick:** cap the per-draw charge at `vary_w = MIN2(vary_w, tess_budget / N)`, with N from `PANVK_V7_TESS_BATCH_DRAWS` (default 4; `0` = old, the full worst case).
     - Correctness does not change. The GPU prefix (`libpan/draw_helper.cl:1904-1912`) already clamps the kept patches to the per-half indirect varying buffer, which is 4M x 72 B = 288 MB of VA (`vulkan/panvk_device.h:44-45`, `vulkan/panvk_vX_device.c:979-983` and `:1004-1011`).
     - N x 21.5 MiB stays under that for N <= 12, so no patch is dropped even with level 64 everywhere.
     - Cost: the worst-case high-water per half becomes N x 21.5 MiB (86 MiB at N=4) instead of 32 MiB. Typical content stays at about N x 1.4 MiB.
  2. **Adaptive:** charge what draws really use.
     - When a tess draw's batch has completed (or at command buffer reset), read `t->draw[0]`, the kept index count written by `panlib_v7_tess_prefix` (`libpan/draw_helper.cl:1967`).
     - Keep a per-TES maximum of bytes per patch and charge 1.5 times it, capped at the worst case. The first use keeps the worst case.
     - `PANVK_V7_TESS_BUDGET_ADAPT=0` = old.
  3. **Structural:** D9 (TES per domain point) divides the worst case by about 6.
- **Risk:**
  - (1) low-medium. The level-64 high-water rises, and JM never gives GROW_ON_GPF pages back (see D12), so keep N small on 4 GB phones.
  - (2) medium.
- **Measure:**
  - FTL starlte and a31: tess_perf unchanged, plus a new config with a 1280x720 RT, loadOp LOAD, storeOp STORE and 32 tess draws per pass. Compare the default with `PANVK_V7_TESS_BATCH_DRAWS=0`.
  - The knobs are process-static (static cache in `panvk_v7_knobs.h`), so the per-VkDevice `PT_TESS_AB` variants cannot A/B them. Use separate runs.
  - Add a one-shot "batches per submit" log line.
  - Use the tess_perf MEM lines for the memory side.
- **Confidence:** high on the mechanism (code and log). Medium on the game-side size.

### D2. JM TLS/WLS: per command buffer per half, sized for the sparse core-ID range, CPU-cleared, never trimmed
- **Arch:** v6, v7, v9.
- **Where:**
  - `vulkan/jm/panvk_vX_cmd_buffer.c:176-232`, jm-v7 107 = 0080 (a2d90281). The size is `pan_get_total_stack_size(tls.size, thread_tls_alloc, core_id_range)`. Only `cmdbuf->tls_half[h]` is reused. Otherwise a new BO comes from the command buffer's TLS pool (`:672-682`; big BOs come from the VkCommandPool's `tls_big_bo_pool`).
  - `lib/pan_props.c:48-55`: `core_id_range = util_last_bit64(shader_present)`.
  - `lib/pan_scratch.c:57-65`: the pow2 per-thread size x threads per core x core_id_range.
  - `lib/kmod/kbase_kmod.c:2697-2709`: the CPU memset of every committed page.
  - `vulkan/panvk_cmd_pool.c:124-132`: autotrim only for arch >= 10.
- **Measured sizes:**
  - S9: TLS 41472 KiB per half (stack 2324 B, pow2 4 KiB); WLS 6912 KiB per half.
  - A31: TLS 8192 KiB.
  - Pixel 6: TLS 94208 KiB per half (stack 2336 B).
- **Core-ID range (inferred):**
  - Every S9 WLS size is 27 x 2^k (54, 432, 864, 6912 KiB). Every Pixel 6 size is 23 x 2^k (92, 368 KiB). The A31's are 2 x 2^k.
  - So the range is 27 for the 18-core G72 and 23 for the 20-core G78: 33 % and 13 % of TLS/WLS is for core IDs that do not exist.
- **Which shader (inferred):** the about 2.3 KB/thread stack matches the poly tessellator kernels. Every JM arch uses them; the libpan `tess_fast.cl` passes are CSF-only (`libpan/tess_fast.cl:6-9`).
- **What is wasted:**
  - On the S9, one command buffer with a tess draw holds 2 x 40.5 + 2 x 6.75 = 94.5 MiB. When the stack grows, `:190-195` replaces the half's pointer but keeps the smaller BOs until reset.
  - DXVK keeps several command lists, each with its own pool, so a tessellating title on a 4 GB S9 holds K x 94.5 MiB. On the Pixel 6 it is K x 185 MiB.
  - JM has no autotrim, so the pools' free lists keep these BOs after `vkResetCommandPool(0)`.
  - The first use of each per-half BO costs the CPU clear (D2 quick part).
- **Fix:**
  - The quick parts are above (autotrim on JM, no clear, logs).
  - Device-level per-half JM TLS/WLS, grow-only. Free the old BO once the atoms that used it are done; the CSF analogue is csf-v11 230.
    - The halves already order the batches of one half (`half_frag_atom` dependencies, `vulkan/jm/panvk_vX_gpu_queue.c:195-208`).
    - Memory then becomes 2 x the maximum size per device, whatever the number of command lists.
    - `PANVK_JM_DEVICE_TLS=0` = old.
  - Sparse TLS for absent core IDs through MEM_ALIAS, as csf-v11 230/232 do. Mind the 232 stride fix. `PANVK_JM_TLS_SPARSE=0` = old.
  - Cut the tessellator's private-memory stack: below 2 KiB halves the TLS, below 1 KiB quarters it. This overlaps with D10 (a per-output tessellator).
- **Gain:**
  - S9: (K-1) x 94.5 MiB for tess content, and 33 % of the rest with sparse TLS.
  - Pixel 6: (K-1) x 185 MiB.
  - No 10-30 ms first-use hitches.
- **Risk:** trim and no-clear are low; device TLS is medium (lifetime and overlap rules); sparse is medium (the 232 history).
- **Measure:**
  - FTL starlte and oriole: the tess_perf MEM lines.
  - A new PanProbe variant that records the tess frame into 4 command buffers from 4 pools and submits them round-robin, as DXVK does, comparing RSS and MemAvailable.
  - Regressions: tessellation, geometry, gs_tess_primitive_id, submit_stress.
- **Confidence:** high on the sizes. Medium on how many command lists DXVK keeps hot.

### D6. Every dispatch is its own atom; every barrier splits the batch (10-22 items 1 and 2, still open) *(upstream)*
- **Arch:** v6, v7, v9.
- **Where:**
  - `vulkan/jm/panvk_vX_cmd_dispatch.c:178-179` (close and open around every dispatch) and `:362`.
  - `vulkan/jm/panvk_vX_cmd_buffer.c:539-556` (barrier = close/open; the upstream FIXME suggests a CacheFlush job).
- **What is wasted:**
  - Each dispatch is one vtc atom: one JOB_SUBMIT, one event, an atom dependency, and kbase's end-of-atom L2 clean (no `BASE_JD_REQ_SKIP_CACHE_*` is set, `lib/kmod/kbase_kmod.c:3302` and `:3349`).
  - Between dependent atoms the GPU idles for the IRQ, kbase's dependency resolution and the flush.
  - A barrier inside a render pass (DXVK UAV or feedback barriers) is a full spill, as in D1.
- **Fix:**
  - Put back-to-back dispatches in one compute batch.
  - Lower a barrier whose scopes are only vertex/compute/transfer to a `MALI_JOB_TYPE_CACHE_FLUSH` job with the job barrier bit in the current vtc chain. Split only for fragment-to-earlier-stage or attachment dependencies.
  - Knobs: `PANVK_JM_DISPATCH_BATCH=0` and `PANVK_JM_BARRIER_JOB=0` = old.
- **Gain:** tens of us to about 0.1 ms per avoided atom pair, and one spill per in-pass barrier.
- **Risk:** medium. Texture-cache coherency inside one chain on Bifrost needs the CTS synchronization and memory-model groups on a device.
- **Measure:** FTL a31 and starlte: bc_decode, bc_perf (compute), csf_event, submit_stress, robustness2; atoms per submit (one-shot log).
- **Confidence:** medium.

### D8. The upstream 5K-job split lands mid-pass; every v6/v7 indexed draw costs 3-5 jobs *(upstream)*
- **Arch:** v6, v7.
- **Where:**
  - `vulkan/jm/panvk_vX_cmd_draw.c:3613-3627`: `if (batch->vtc_jc.job_index > (5 * 1024))` closes and opens the batch.
  - This sits in `panvk_cmd_draw_indirect()`. Every v6/v7 `vkCmdDrawIndexed` goes through it (`:4060-4104` builds a `VkDrawIndexedIndirectCommand`).
- **What is wasted:**
  - A draw is the helper job (min/max folded since 0069), then vertex + tiler (or one IDVS job), then the FS copy (D3), plus the VS copy when dirty. That is 3-5 jobs, so the split comes every 1000-1700 indexed draws in a pass, as a full spill.
  - The comment's reasons have mostly gone:
    - The varying buffer is now per half, reset per batch and clamped on the GPU.
    - TLS growth was fixed by 0080.
    - Only the dEQP `many_indirect_draws_on_secondary` timeout remains.
- **Fix:**
  - After D3, raise the limit, for example to 12K jobs (the job index is 16 bits), or count bytes in `vary_infos` instead of jobs.
  - `PANVK_V7_SPLIT_JOBS=5120` = old.
- **Gain:** passes of 1000-2500 indexed draws (open-world D3D11) avoid 1-2 spills.
- **Risk:** medium (long chains: kbase soft or hard stop). Rerun the dEQP case.
- **Measure:** FTL a31: large_draw, draw_params, plus a 3000-draw pass bench with a 1280x720 RT.
- **Confidence:** medium.

### D9. v6/v7 TES is shaded once per index, not per domain point
- **Arch:** v6, v7.
- **Where:** jm-v7 014 (71d5597d).
  - `vulkan/jm/panvk_vX_cmd_draw.c:3972-3990`: the TES is the VS of a non-indexed indirect draw whose `vertexCount` is `t->draw[0]`, the kept index count (`libpan/draw_helper.cl:1967`).
- **What is wasted:**
  - At L=16, each 16-patch draw runs the TES 27,744 times for about 5,200 domain points, about 5.4x, and stores that many varyings (`t->varying_bytes`).
  - v9 already shares them. The 0090 diagnosis: "gs=0 L=16 shades the shared tessellator vertices (about 1.3M)", against about 7.1M per frame on v6/v7.
  - This also makes D1's worst case 6x larger than it needs to be.
- **Fix:**
  - Use an indexed TES draw: a vertex job over the draw's domain points (count from the prefix kernel), and a tiler job reading the tessellator's index buffer.
  - The indices may need rebasing if they are relative to each patch's point allocation; check `poly_heap_alloc_points` in `poly/cl/tessellator.h:186-198`.
  - IDVS stays off for the TES (0065/0075).
  - `PANVK_V7_TES_SHARED=0` = old.
- **Gain:** TES ALU and varying traffic /5 at high levels; D1's worst case /6.
- **Risk:** medium-high (new draw shape on v6/v7).
- **Measure:** FTL starlte and a31: tess_perf hashes must match (4807f3f5, a03404c5, cec55885, d4463b35, b34a251c); tessellation, gs_tess_primitive_id, pipeline_stats (TES invocation counts change meaning, so the query must still report per-index counts or follow the spec's "at least" rule).
- **Confidence:** medium. The TES share of frame time is not measured (see A1).

### D10. v6/v7 tess draws serialize through one arena and barrier kernels; a 16-patch draw tessellates on one core
- **Arch:** v6, v7.
- **Where:**
  - `vulkan/jm/panvk_vX_cmd_draw.c:3788-3801`: every draw lays out `dev->tess_arena` from offset 0.
  - `:3964-3969`: COUNT, prefix and WITH_COUNTS are all `PANLIB_BARRIER_JM_BARRIER`.
  - The grid is `DIV_ROUND_UP(patches, 64)` workgroups of a one-thread-per-patch tessellator (`libpan/draw_helper.cl:1840-1872`).
- **What is wasted:**
  - Once D1 lets several tess draws share a batch, each draw's tessellator waits for everything before it.
  - The tessellator itself is 16 threads, one workgroup, on an 18-core G72.
  - CSF already alternates two arenas (`PANVK_TESS_ARENAS`, `vulkan/panvk_vX_device.c:877-878`).
- **Fix:**
  - Use 2-4 arena regions in turn for CPU-known draws (a 16-patch draw needs only a few MiB of the 160 MiB arena).
  - Replace the JM barrier on COUNT with a job dependency on the previous user of the same region.
  - `PANVK_V7_TESS_ARENAS=1` = old.
  - Longer term: a per-output tessellator, which also shrinks the D2 stack.
- **Gain:** up to Kx on the tessellator phase for many small draws on wide GPUs (S9); little on the 2-core A31.
- **Risk:** medium.
- **Measure:** tess_perf L=4/L=16 on starlte after D1.
- **Confidence:** medium.

### D11. kbase JM submission: 2 ioctls and 2 events per batch, an L2 clean after every vtc atom (K8 multi-atom still open)
- **Arch:** v6, v7, v9.
- **Where:**
  - `lib/kmod/kbase_kmod.c:3280-3327` and `3330-3380`: `nr_atoms = 1`, and `core_req` has no `SKIP_CACHE` or `EVENT_ONLY_ON_FAILURE`.
  - `vulkan/jm/panvk_vX_gpu_queue.c:195-208` and `264-277`: one vtc atom and one frag atom per batch.
  - Event drain: `lib/kmod/kbase_kmod.c:3133-3190`.
- **What is wasted:**
  - A vtc atom with a dependent frag atom (submitted with `no_waiter`) still produces an event that has to be read.
  - Its end-of-atom L2 clean writes back polygon lists and varyings that the frag atom then reads back.
  - Two ioctls per batch, 512 per tess_perf frame today.
- **Fix:**
  1. `BASE_JD_REQ_EVENT_ONLY_ON_FAILURE` on `no_waiter` vtc atoms. The frag atom's DATA dependency implies their success; reclaim the slot on the frag event. `PANVK_KBASE_VTC_SILENT=0` = old.
  2. One JOB_SUBMIT per `vkQueueSubmit` (K8; mid-array failure handling needs a device). `PANVK_KBASE_MULTI_ATOM=0`.
  3. `SKIP_CACHE_END` on such vtc atoms and `SKIP_CACHE_START` on their frag atom. The frag atom's end clean still makes everything CPU-visible. `PANVK_KBASE_SKIP_CACHE=0`.
- **Gain:** CPU 10-30 us per batch; one L2 clean per batch (GPU time grows with dirty data).
- **Risk:** (1) and (2) medium; (3) medium-high (coherency; check kbase errata gating).
- **Measure:** FTL starlte, a31 and oriole: submit_stress, csf_event, occlusion_query, vkd3d_timeline, tess_perf; the event count per frame (one-shot log).
- **Confidence:** medium.

### D12. v9: the 2 x 512 MiB tiler heaps never give pages back
- **Arch:** v9.
- **Where:** `vulkan/panvk_vX_device.c:936-967`, jm-v7 117 = 0090 (bc83c08a). MALLOC_VERTEX takes every vertex packet of a batch from the heap.
- **What is wasted:**
  - GROW_ON_GPF commits pages as the heap is touched and keeps them for the life of the device.
  - After one heavy scene (tess_perf gs=1 L=16 is "several hundred MiB per frame"), that memory stays committed for the session on 4-6 GB G68/G77 phones.
  - The same applies to the v6/v7 growable buffers: varying x2, GS scratch x2, tess arena.
- **Fix:**
  - While idle (after the last atom of a half), when the recent high-water is below 1/4 of the committed size, shrink with `KBASE_IOCTL_MEM_COMMIT` to 2x the recent high-water.
  - Whether kbase shrinks a GROW_ON_GPF region needs a device check.
  - `PANVK_V9_HEAP_TRIM=0` = old.
- **Gain:** 100s of MiB after spikes.
- **Risk:** medium (shrinking under an in-flight atom faults).
- **Measure:** FTL oriole: tess_perf gs=1 L=16 followed by an idle loop, then MemAvailable.
- **Confidence:** medium.

### D13. Android present waits for device idle and CPU-copies every frame (K1, still open)
- **Arch:** all; worst on JM.
- **Where:** `vulkan/panvk_wsi.c:262-299`: `DeviceWaitIdle` at `:273`, then a row-by-row memcpy from the AHB into the window.
- **What is wasted:**
  - CPU and GPU never overlap across frames.
  - About 8 MB of CPU reads per 1080p frame.
  - JM has no sync_file export, so the fence path K1 proposed for CSF is not available.
- **Fix:**
  - A present thread that waits on the image's last-render kbase atom (`kbase_kmod_wait_atom`) and then copies and posts. `vkQueuePresentKHR` returns at once, and acquire blocks only when every image is in flight.
  - `PANVK_ANDROID_PRESENT_THREAD=0` = old.
- **Gain:** up to about 2x FPS when CPU and GPU frame times are similar.
- **Risk:** medium (WSI threading).
- **Measure:** FTL starlte, a31 and oriole: swapchain_lifecycle, plus CPU ms per `vkQueuePresentKHR` and FPS from the FTL video frames.
- **Confidence:** high on the mechanism.
- **Overlap:** the shared WSI path also belongs to the CSF CPU review (A).

---

## Open anomalies (measure before acting)

### A1. A31: gs=0 is 3x slower than gs=1 at L=16
The numbers: 5121 against 1695 ms per frame. On the S9 the ratio is 1.27x.
- Both v6/v7 modes share:
  - the tessellator and the TES per index;
  - the per-draw batch split: `vary_w` does not depend on the GS, so `over` is true on the second tess draw of a batch either way.
- Per extra triangle, A31 gs=0 costs 2.07 us and gs=1 0.46 us. The difference must be in the part only gs=0 has: the TES is the vertex job of a tiler draw, writing varyings into the indirect varying buffer.
- IDVS is ruled out: `vulkan/panvk_vX_shader.c:4627-4699` never gives the TES IDVS on v6/v7.
- Plan:
  1. a tess_perf config with `rasterizerDiscardEnable`, which keeps the TES vertex job and drops tiler and fragment;
  2. the same with a 1x1 scissor, for fragment cost;
  3. per-batch GPU timestamps on the A31 (calibrated since 0081/0082). Each tess draw is its own batch today, so this gives per-draw GPU time;
  4. one run with `PANVK_V7_TESS_BATCH_MB=0`.

### A2. Pixel 6: gs=1 is 4.2x slower than gs=0 at L=16
The numbers: 3791 against 894 ms per frame.
- The 0090 diagnosis is that the pass-through GS emits about 7M unshared vertex packets per frame into the MALLOC_VERTEX heap.
- At 48 B each that is about 0.7 GB of heap traffic per frame, roughly 40 ms at Tensor bandwidth. That is far below the 2.9 s gap.
- So something serial in the v9 GS path dominates. Profile it with the same discard and scissor configs, and use the GPU-side review (C).

## Checked, nothing to change
- **0087 event batching:** correct; only the redundant first `poll(0)` (D14).
- **No steady-state CPU waits on submit:** the only waits are the warm-up paths (`vulkan/jm/panvk_vX_gpu_queue.c:182-192`, `242-262`) and host-set VkEvent waits (`usleep(100)`, `:311-319`). More than `PANVK_KBASE_SYNC_TARGET_COUNT` batches release, not wait (`:496-505`).
- **0069 folded min/max:** a workgroup reduction (`libpan/draw_helper.cl:1635-1668`), not serial.
- **0091/0092 (v9):**
  - 0091 is a push-uniform field, no extra job.
  - 0092 adds one single-thread barrier job per indirect fan draw (`vulkan/jm/panvk_vX_cmd_draw.c:6765-6800`). Indirect fans are rare.
- **IDVS rules:** TES and v6 have no IDVS by design (0065/0075); no change proposed.
- **Android option lookups:** cached (`util/os_misc.c:212-246`).
- **Userspace cache maintenance:** the default on kbase, so the desc-pool flushes are no longer MEM_SYNC ioctls (but see D5 for their volume).

## Overlaps with the other reviews
- **Memory (B):**
  - D2 is the JM counterpart of csf-v11 230 (device TLS) and of 226/231 (autotrim).
  - D7 is K3 on v9.
  - D12 is heap trimming.
  - AFBC eligibility is not covered here; it also scales the D1, D6 and D8 spill cost.
- **GPU efficiency (C):** the spill and preload cost per split (D1, D6, D8), and anomalies A1 and A2.
- **CSF CPU (A):** D13 (shared WSI). D3 and D5 are Bifrost/JM-only and have no CSF counterpart.

## Suggested order
1. D3 (UB fix plus one copy per dirty bind) and D5 (one flush per resubmit). Both are safe without a device, and D5 makes the PanProbe numbers trustworthy for everything after it.
2. D14 and the D2 quick parts (JM autotrim, no TLS clear, core-ID log), then D7.
3. FTL A/B runs: D4 (`PANVK_KBASE_JM_WARMUP=0`), and D1 step 1 (`PANVK_V7_TESS_BATCH_DRAWS`) with a 1280x720 tess_perf config.
4. Device work: D1 step 2, D2 device TLS, D6, D8, D11.
5. Larger rewrites: D9 and D10 (v6/v7 tessellator shape), D12, D13.
