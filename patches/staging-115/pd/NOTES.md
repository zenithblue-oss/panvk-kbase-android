# Staging 115, package P-D: JM path (v6 G72, v7 G52, v9 G78)

Source: `worklogs/g615-dxvk/perf-review-2026-10-26/d-jm.md` (findings D1-D14, anomalies A1/A2) and the
consolidated plan `README.md` in the same directory.

- Base: branch `p115-int` (`6a5d4e27`, worktree `/home/flux/src/mesa-115-int`, see `../int/NOTES.md`), so
  P-A + P-B + P-C + the upstream-style driverInfo are below these patches.
- Work tree: `/home/flux/src/mesa-115-pd`, branch `p115-pd` (head `8a4d9f8e`).
- Apply: `git am patches/staging-115/pd/*.patch` on `p115-int` (7 patches, no conflicts: the rebases from
  `rc5g-114` onto `p115-pc` and then onto `p115-int` were clean).
- Export: `git -C /home/flux/src/mesa-115-pd format-patch p115-int..p115-pd -o patches/staging-115/pd/`.
- Every behaviour change has an env knob that restores 114, read once per VkDevice (`panvk_device::jm_opt`,
  set in `panvk_per_arch(create_device)()` next to P-B's `cmdbuf_opt`) or once per process
  (`kbase_kmod_dev_create()`). No getenv in a hot path. P-D does not use the P-A knob registry
  (`panvk_perf_knobs.h`); its knobs are JM-only and do not log a "114 behaviour restored" line.

## Patches

| # | Title | Finding | Arch | Knob (default; 114 value) |
|---|---|---|---|---|
| 0001 | panvk/jm: v6/v7 FS descriptor copy job only when the FS tables change | D3 | v6, v7 | `PANVK_V7_FS_COPY_ONCE` (1; 0) |
| 0002 | panvk/jm: clean the descriptor pool once per resubmitted command buffer | D5 | v6, v7, v9 | `PANVK_JM_RESET_FLUSH` (once; `batch` or `0`) |
| 0003 | panvk/jm: v6/v7 tess winding option read once per device | D14 | v6, v7 | none (same option, read once) |
| 0004 | panvk/jm: v6/v7 tess draws share a batch: per-draw charge capped | D1 step 1 | v6, v7 | `PANVK_V7_TESS_BATCH_DRAWS` (4; 0) |
| 0005 | panvk/jm: v9 gpu_prerast arenas committed at first use, as on CSF | D7 | v9 | `PANVK_V9_PRERAST_ARENA_LAZY` (1; 0); `PANVK_GPU_PRERAST_ARENA=eager` still forces eager on every arch |
| 0006 | kbase: JM warm-up sleep off by default | D4 | v6, v7, v9 | `PANVK_KBASE_JM_WARMUP` (0; 1) |
| 0007 | panvk/jm: the TLS/WLS pool skips the kbase CPU clear | D2 quick part | v6, v7, v9 | P-B's `PANVK_KBASE_ZERO_ALL` (unset; 1) |

114 A/B set for these (all JM-only, ignored on CSF; in PanProbe `assets/env-ab.json` pass `114-knobs`):

```
PANVK_V7_FS_COPY_ONCE=0 PANVK_JM_RESET_FLUSH=batch PANVK_V7_TESS_BATCH_DRAWS=0
PANVK_V9_PRERAST_ARENA_LAZY=0 PANVK_KBASE_JM_WARMUP=1
```

0007 is covered by `PANVK_KBASE_ZERO_ALL=1`, which the pa-pc set already has.

### 0001 D3: FS descriptor copy job only when the FS tables change
- **What:** the per-layer FS copy job built in `panvk_draw_prepare_tiler_job()` was never chained (the chained
  one is `draw->jobs.frag_copy_desc` from `prepare_draw()`, added before the layer loop), so it is no longer
  built. This also fixes the uninitialized `ptr` read on FS-less draws (upstream UB: a stack value was appended
  to `batch->jobs`, and a resubmit memset 16 bytes at it).
- **Note:** the review assumed the per-layer job ran on the GPU. It did not, so the GPU job chain is unchanged
  and the gain is CPU only.
- **Gain:** 1-3 us of CPU and about 0.5 KiB of descriptor pool per draw and layer, fewer header resets per
  resubmit, less pool to clean (D5).
- **Risk:** low (the GPU chain is identical; the knob rebuilds the dead job).

### 0002 D5: one descriptor pool clean per resubmitted command buffer
- **What:** `gpu_queue_submit()` resets every issued batch first, and each reset cleaned the whole descriptor
  pool (DC CVAC). Now the pool is cleaned once after the last reset, still before the first submit.
- **Gain:** 10-50 ms per resubmitted tess_perf frame on v6/v7 (256 batches), less with 0004. It makes
  resubmit-based PanProbe timings trustworthy.
- **Risk:** very low (the same memory is written in the same order: all resets, one clean, then the submits).

### 0003 D14: tess winding option read once
- `PANVK_TESS_WINDING_INLINE` was looked up per tessellated draw on v6/v7. It is now read at device creation,
  as on v9+. About 1 us per tess draw, no risk.
- Not done from D14: dropping the second `poll(0)` in `kbase_drain_events()`. It is not redundant: another
  thread can drain between the caller's poll and the read lock, and the fd is blocking. The point-size bytes of
  the tess budget also stay (`t->varying_bytes` bounds the GPU allocation in the prefix clamp).

### 0004 D1 step 1: tess draws share a batch
- **What:** each tessellated draw is charged at most `budget / PANVK_V7_TESS_BATCH_DRAWS` (default 4) of the
  32 MiB tess varying budget, instead of its level-64 worst case (21.5 MiB for 16 triangle patches). So four
  worst-case draws share a batch instead of one.
- Correctness does not depend on the charge: `panlib_v7_tess_prefix()` keeps only the whole patches whose
  varyings fit the batch's indirect varying buffer.
- **Gain:** tess_perf has 64 instead of 256 batches per frame on v6/v7. Each avoided split saves a framebuffer
  store and reload (15-33 MB at 1080p, 2-4 ms on G52/G72) and two atoms.
- **Risk:** low-medium. A batch may now touch up to 4 x 21.5 MiB of the GROW_ON_GPF varying buffer per half
  when every patch really reaches level 64 (it was 32 MiB). If a level-64 scene ever exceeds the per-half VA,
  the GPU clamp drops whole patches (visible holes, no fault). The "tess batch budget" log line prints the
  charge and N.

### 0005 D7: v9 gpu_prerast arenas at first use
- **What:** the two v9 JM readers (`v9_prerast_draw_one()`, `v9_tess_draw()`) now use
  `panvk_per_arch(device_gpu_prerast_arena)()`. It commits the arena (fully, under `gpu_prerast_lock`) the
  first time a draw records such a pass, as csf-v11 205 does on CSF. Recording comes before any submit that can
  use it, so the arena is still never grown by GPU faults.
- **Gain:** 160 MiB and 50-100 ms of CPU clear per VkDevice that never records a gpu_prerast or tessellation
  pass. Every DXVK device enables the features.
- **Risk:** low-medium: the first such draw now pays the commit and clear (one hitch).
- **Knobs:** `PANVK_V9_PRERAST_ARENA_LAZY=0` is the 114 behaviour on v9 only (CSF unchanged, so it can sit in
  an all-arch A/B set). `PANVK_GPU_PRERAST_ARENA=eager` still forces the creation-time commit everywhere.

### 0006 D4: JM warm-up sleep off by default
- **IMPORTANT:** no justification for the 2 s sleep was found. It came in with jm-v9 001 (from
  FristOneRR-Panvk-Source efd07ba, no comment); jm-v7 111 kept it once per process because the reason was
  unknown; the worklogs give none. The default is now **0 (no sleep)**, and
  **`PANVK_KBASE_JM_WARMUP=1` restores the 114 behaviour** (2 s once per process; 2 = every open).
- The first vtc and frag chains of each queue are already submitted synchronously with
  `kbase_kmod_job_submit_retry()` (5 attempts, 0.4-1.6 s back-off).
- **Gain:** 2 s per process: about 78 s (21-26 %) of each FTL PanProbe run on starlte, a31 and oriole, and
  2 s per game start.
- **Risk:** low-medium (the original reason is unknown). Watch the first submit of each process: any
  "JM atom ... failed", `event=0x` or DEVICE_LOST right after the "kbase: JM warm-up off" line. If one is
  seen, rerun with `PANVK_KBASE_JM_WARMUP=1`.

### 0007 D2 quick part: TLS/WLS pool without the kbase CPU clear
- **What:** the JM command buffer TLS pool (only the per-half TLS and WLS BOs of jm-v7 107) now asks for
  P-B's `PAN_KMOD_BO_FLAG_NO_ZERO`, as the CSF TLS and WLS pools do. Shaders never read either before writing
  it, and pooled BOs were already handed out again uncleared. kbase still hands out zeroed pages, except this
  context's own freed ones.
- **Gain:** 40.5 + 6.75 MiB of memset and cache sync per half on the S9 (92 MiB per half on the Pixel 6), so
  10-30 ms of CPU on the first use of each half by a command buffer with a tessellated draw.
- **Risk:** low. The other half of D2's quick part (command pool auto-trim on JM) is P-B 0005.

## Build (115pre-d)

- Fresh BDIR `/home/flux/build-rc5/bionic-r30-115pre-d`, `--profile g615-v11-csf --api 29`, NDK 29.0.14206865,
  `HOST_TOOLS=/home/flux/repos/panvk-kbase-android/build/host-tools/bin`, 4 jobs, under the build lock.
- `/home/flux/build-out/2026-10-26-115pre-d/libvulkan_panfrost.so`, 29904352 bytes, sha256
  `7f2260f0598e9f062eb08d96a6a3698c4b1d1c6d8ae90bb9854a36dde2b9d544`, build-id `c4f0d9c9...`, driverInfo
  `Mesa 26.3.0-devel (git-8a4d9f8e50)`. Only the nine known warnings (`jm/panvk_vX_gpu_queue.c:182` is now
  `:185`, moved by 0002). All 26 A/B knob names are in the .so.
- PanProbe `/home/flux/build-out/apks/panprobe-115pre-d.apk`, sha256
  `ec4a6453aede73588a3fda35f77689e647f4dfa27ffee10988baebe477459e38` (rc5 = the driver, rc4 `c395db09`). Record:
  `worklogs/g615-dxvk/builds/rc5-universal.md`, section 115pre-d.

## Device results (115pre-d, Firebase, PanProbe scenario 3: `defaults` then `114-knobs`)

Runs: `worklogs/g615-dxvk/device-runs/2026-10-26-ftl-{v6-starlte,v7-a31,v9-oriole}-115pre-d/results.md`, baselines the
2026-10-25 113 runs (there is no JM 114 run). Pass 2 has a warm shader cache.

| device | arch | defaults | 114-knobs | tess_perf hashes | faults | total s (defaults / 114-knobs / 113) |
|---|---|---|---|---|---|---|
| starlte (Galaxy S9, Mali-G72 MC18) | v6 | 39/39 | 39/39 | = 113 (18/18) | none | 215.0 / 292.1 / 295.9 (-26 %) |
| a31 (Galaxy A31, Mali-G52 r1 MC2) | v7 | 39/39 | 39/39 | = 113 (18/18) | none | 286.9 / 362.9 / 364.3 (-21 %) |
| oriole (Pixel 6, Mali-G78 MC20) | v9 | 39/39 | 39/39 | = 113 (18/18) | none | 219.5 / 297.8 / 299.9 (-26 %) |

Faults means DEVICE_LOST, `event=0x`, GPU/MMU fault, Fatal signal or lmkd kills. driverInfo is
`Mesa 26.3.0-devel (git-8a4d9f8e50)` in all six passes. `114-knobs` is within 0.4-1.3 % of 113 on all three devices.

- **0006 (warm-up off):** 92 device opens per pass on each device with no submit retries, "JM atom ... failed" lines or event
  errors.
  - The first test of a run (gpu_prerast_slice) takes 0.08-0.35 s instead of about 2.2 s.
  - Timestamp calibration is unaffected: within 83 ppm on the a31 and 103 ppm on oriole. starlte advertises no timestamps,
    as on 113.
  - Gain: 76-78 s per PanProbe run, nearly all of the total time difference.
  - With 2 s subtracted from every `114-knobs` test, the totals are within ±0.7 %.
- **0004 + 0002 (+ 0001), v6/v7 tess_perf GPU time per frame:**
  - gs=0 L=1: -11 % (starlte) and -17 % (a31);
  - gs=0 L=4: -6.6 % / -3.1 %;
  - gs=1 L=1: -4.2 % / -2.5 %;
  - L=16 and mix: within ±1.5 %.
  Images are identical (hashes). On oriole (0002 only) L=4 is -3 % and the rest within noise.
- **0004 cost:** tess_perf peak RSS is +38 MiB in the gs=1 cases (starlte +18 %, a31 +33 %) and +6 MiB at gs=0 L=16.
  It comes from the growable varying and GS-scratch buffers shared by 4 draws per batch, and it is freed at device
  teardown. `PANVK_V7_TESS_BATCH_DRAWS=2` would halve the extra (not measured). **Integration should decide whether 4 is
  the right default for 3-4 GB v6/v7 phones.**
- **0005 (oriole):** a full-feature VkDevice with no draws (bc_perf `full_feature_device`) maps exactly 160 MiB less GPU
  memory and has 162 MiB less RSS (21632 against 187356 KiB). tess_perf commits the tess arena at the first tessellated
  draw and the gpu_prerast arena at the first GS draw; after that the footprint equals 113.
- **0001 / pa-pc CPU knobs:** in the PanProbe 1 s draw loops (FPS lines), the defaults are faster on starlte (+31-71 %) and
  in the first three oriole loops (+30-38 %). These loops are confounded by the pass order, because pass 2 starts during
  the pass-1 upload.
  - The first a31 run showed a 42-60 % dip in its first three loops with the defaults (start-of-run system load).
  - A second a31 run (alternating order, 4 passes) shows the defaults as fast or faster in both pairs. There is no
    CPU-throughput regression.
- **0007:** not separately visible in PanProbe (a one-time clear per TLS/WLS BO).
- **v9 JM caveat:** P-C C6 (Z/S preload) is active in both oriole passes, because `PANVK_ZS_PRELOAD_ALWAYS=1` is not in
  the shared `114-knobs` set.
- **A1 and A2 are unchanged:** the a31 gs=0 L=16 frame is 3.0x its gs=1 frame, and on oriole gs=1 L=16 is 4.3x gs=0.
  They are phase-2 measurements (below).

## Phase 2 (not in this package; design notes)

The larger items of d-jm.md need device timing or a new draw/submit shape. None is started except D6a, which
is saved as an uncompiled WIP.

- **D6 (dispatch batching, barrier jobs; upstream):**
  - WIP branch `p115-pd-wip` (`2e0e6c2c`, on the old `p115-pd` `0a9e5295` over `p115-pc`; not compiled, not
    tested): "back-to-back dispatches share a batch", `PANVK_JM_DISPATCH_BATCH`. A `batch->dispatch` flag, up
    to 4 extra LOCAL_STORAGE descriptors (`wls_descs[4]`) so each dispatch keeps its own WLS layout,
    `dispatch_can_join()` (same TLS/WLS needs, under 4096 jobs), the job barrier bit on the first job of a
    joining dispatch, WLS precomp kernels with the barrier bit in dispatch batches, and the batch kept open
    after a dispatch.
  - Why it was not taken: the 114 batch TLS keeps only the last job's layout. The indirect-dispatch kernel
    throttles indirect WLS dispatches to 1 instance per core. That is a latent perf bug that only the WIP fixes,
    and the shared-batch WLS rules need CTS compute and synchronization groups on a device.
  - Gallium panfrost already serializes compute jobs with `barrier=true` and per-grid LOCAL_STORAGE
    descriptors. That is the model for the rest of D6.
  - D6b (lower vertex/compute/transfer-only barriers to a `MALI_JOB_TYPE_CACHE_FLUSH` job with the job barrier
    bit in the current vtc chain, `PANVK_JM_BARRIER_JOB=0` = old): not started. Texture-cache coherency
    within one chain on Bifrost needs the CTS memory-model groups.
  - Measure: FTL a31 and starlte: bc_decode, bc_perf, csf_event, submit_stress, robustness2, and the atoms per
    submit.
- **D8 (5K-job split mid-pass; upstream):** after 0001, raise the `vtc_jc.job_index > 5 * 1024` split in
  `panvk_cmd_draw_indirect()` to about 12K jobs (16-bit index), or count `vary_infos` bytes instead.
  `PANVK_V7_SPLIT_JOBS=5120` = old. Rerun dEQP `many_indirect_draws_on_secondary` (soft/hard-stop risk), and
  add a 3000-draw pass bench at 1280x720.
- **D11 (kbase JM submission):**
  1. `BASE_JD_REQ_EVENT_ONLY_ON_FAILURE` on `no_waiter` vtc atoms (`PANVK_KBASE_VTC_SILENT=0`).
  2. One JOB_SUBMIT per `vkQueueSubmit` (K8; mid-array failure handling needs a device;
     `PANVK_KBASE_MULTI_ATOM=0`).
  3. `SKIP_CACHE_END` on such vtc atoms and `SKIP_CACHE_START` on their frag atom (`PANVK_KBASE_SKIP_CACHE=0`;
     check kbase errata gating).
  Measure: the event count per frame, submit_stress, csf_event, occlusion_query, vkd3d_timeline.
- **D12 (v9 tiler heaps never shrink):** when idle (after a half's last atom) and the recent high-water is
  below 1/4 of the committed size, shrink with `KBASE_IOCTL_MEM_COMMIT` to 2x the high-water
  (`PANVK_V9_HEAP_TRIM=0`). First check on a device that kbase shrinks a GROW_ON_GPF region. The same applies
  to the v6/v7 growable buffers (varying x2, GS scratch x2, tess arena).
- **D2b (device-level JM TLS/WLS, sparse TLS):** per-device, per-half, grow-only TLS/WLS freed once the atoms
  that used the old BO are done (the CSF analogue is csf-v11 230). The halves already order their batches.
  `PANVK_JM_DEVICE_TLS=0`. Then sparse TLS for absent core IDs through MEM_ALIAS (csf-v11 230/232, mind the
  232 stride fix; `PANVK_JM_TLS_SPARSE=0`): the core-ID range is 27 for the 18-core G72 and 23 for the
  20-core G78. Measure with a PanProbe variant that records the tess frame into 4 command buffers from 4 pools
  (as DXVK does) and compares RSS and MemAvailable.
- **D9 (v6/v7 TES per index):** an indexed TES draw over the domain points (count from the prefix kernel)
  with a tiler job reading the tessellator's index buffer (check `poly_heap_alloc_points` rebasing).
  `PANVK_V7_TES_SHARED=0`. tess_perf hashes and pipeline_stats TES counts must hold.
- **D10 (v6/v7 tess arena serialization):** 2-4 arena regions in turn for CPU-known draws, and a job
  dependency on the previous user of the region instead of the JM barrier on COUNT
  (`PANVK_V7_TESS_ARENAS=1`). Worth it now that 0004 lets tess draws share a batch. Longer term, a per-output
  tessellator also shrinks the D2 stack.
- **D1 step 2:** charge the real tess use (count from the prefix kernel, read back per batch) instead of a
  fixed share. Needs per-batch GPU timing first.
- **A1 (A31 gs=0 3x slower than gs=1 at L=16) and A2 (Pixel 6 gs=1 4.2x slower than gs=0):** measure
  first, with the `rasterizerDiscardEnable` and 1x1-scissor tess_perf configs and per-batch GPU timestamps.
- **Found while doing P-D, not in d-jm.md:**
  - v9 JM rebuilds the VS/FS driver descriptor sets and the resource tables on every draw (no dirty gating;
    P-A gated CSF only).
  - P-A's note: shaderSubgroupClock on JM kbase reads a counter that runs only for PERMON atoms
    (`batch->timestamps`), so a batch whose shaders read the clock should also get PERMON.
  - P-B's note: the JM arenas (v6/v7 tess arena, v9 gpu_prerast arenas) still get the csf-v11 041 CPU
    clear. Check whether anything in them relies on zeroed memory before giving them `NO_ZERO`.
  - The JM varying pool still takes the kbase CPU clear; it could use `PAN_KMOD_BO_FLAG_NO_ZERO` too, after
    a check that no varying is read before it is written (point size, XFB).
  - P-B's device slab caches (`PANVK_CMD_SLAB_CACHE`) are v10+ only (`cmdbuf_opt.slab_cache`). The JM
    command pools (desc and TLS BO pools) could use them too.
  - v9 JM tess arena: `v9_tess_layout()` still uses the fixed 32 MiB VS region (`PANVK_GPU_PRERAST_*`
    constants, 96 MiB arena). P-B's `PANVK_TESS_VS_REGION_MB` (8 MiB default) applies to v10+ only. Moving v9
    JM to `tess_opt.vs_region_end` would make its tess arena 72 MiB (24 MiB less per device that tessellates).
