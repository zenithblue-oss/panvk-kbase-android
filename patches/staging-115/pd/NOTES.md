# Staging 115, package P-D: JM path (v6 G72, v7 G52, v9 G78)

Source: `worklogs/g615-dxvk/perf-review-2026-10-26/d-jm.md` (findings D1-D14, anomalies A1/A2)
and the consolidated plan `README.md` in the same directory.

- Base: tag `rc5g-114` (worktree `/home/flux/src/mesa-rc5g`, untouched).
- Work tree: `/home/flux/src/mesa-115-pd`, branch `p115-pd`.
- Export: `git -C /home/flux/src/mesa-115-pd format-patch rc5g-114..p115-pd -o patches/staging-115/pd/`.
- Every patch has an env knob that restores the 114 behaviour, read once per VkDevice
  (`panvk_device::jm_opt`) or once per process in `kbase_kmod_dev_create()`.
- Private build: `/home/flux/build-out/dev-115-pd/libvulkan_panfrost.so` (profile g615-v11-csf,
  API 29). Not a release, not device-tested.

Status: work in progress (quick wins done, larger items being added).

## Patches

| # | Title | Finding | Arch | Knob (default; 114 value) |
|---|---|---|---|---|
| 0001 | panvk/jm: v6/v7 FS descriptor copy job only when the FS tables change | D3 | v6, v7 | `PANVK_V7_FS_COPY_ONCE` (1; 0) |
| 0002 | panvk/jm: clean the descriptor pool once per resubmitted command buffer | D5 | v6, v7, v9 | `PANVK_JM_RESET_FLUSH` (once; `batch` or `0`) |
| 0003 | panvk/jm: v6/v7 tess winding option read once per device | D14 | v6, v7 | none (same option, read once) |
| 0004 | panvk/jm: v6/v7 tess draws share a batch: per-draw charge capped | D1 step 1 | v6, v7 | `PANVK_V7_TESS_BATCH_DRAWS` (4; 0) |
| 0005 | panvk/jm: v9 gpu_prerast arenas committed at first use, as on CSF | D7 | v9 | `PANVK_GPU_PRERAST_ARENA` (lazy; `eager`) |
| 0006 | kbase: JM warm-up sleep off by default | D4 | v6, v7, v9 | `PANVK_KBASE_JM_WARMUP` (0; 1) |

### 0001 D3: FS descriptor copy job only when the FS tables change
- **What:** the per-layer FS copy job built in `panvk_draw_prepare_tiler_job()` was never
  chained (the chained one is `draw->jobs.frag_copy_desc` from `prepare_draw()`, added before
  the layer loop), so it is no longer built. Also fixes the uninitialized `ptr` read on
  FS-less draws (upstream UB: a stack value was appended to `batch->jobs` and a resubmit
  memset 16 bytes at it).
- **Note:** the review assumed the per-layer job ran on the GPU. It did not, so the GPU job
  chain is unchanged and the gain is CPU only.
- **Gain:** 1-3 us CPU and about 0.5 KiB of descriptor pool per draw and layer, fewer header
  resets per resubmit, less pool to clean (D5).
- **Risk:** low (the GPU chain is identical; the knob rebuilds the dead job).
- **Regression signal:** none expected; any draw test (large_draw, draw_params, geometry,
  tessellation) would show a missing-descriptor fault.

### 0002 D5: one descriptor pool clean per resubmitted command buffer
- **What:** `gpu_queue_submit()` resets every issued batch first; each reset cleaned the whole
  descriptor pool (DC CVAC). Now the pool is cleaned once after the last reset, still before
  the first submit.
- **Gain:** 10-50 ms per resubmitted tess_perf frame on v6/v7 (256 batches), less with 0004.
  Makes resubmit-based PanProbe timings trustworthy.
- **Risk:** very low (same memory written, same order: all resets, one clean, then submits).
- **Regression signal:** submit_stress, tess_perf (hashes), any test that resubmits a
  recorded command buffer (vkd3d_timeline, occlusion_query).

### 0003 D14: tess winding option read once
- **What:** `PANVK_TESS_WINDING_INLINE` was looked up per tessellated draw on v6/v7; now read
  at device creation as on v9+.
- **Gain:** about 1 us per tess draw. **Risk:** none.
- **Not done from D14:** dropping the second `poll(0)` in `kbase_drain_events()` (not
  redundant: another thread can drain between the caller's poll and the read lock, and the
  fd is blocking); the point-size bytes of the tess budget (`t->varying_bytes` bounds the GPU
  allocation in the prefix clamp).

### 0004 D1 step 1: tess draws share a batch
- **What:** each tessellated draw is charged at most `budget / PANVK_V7_TESS_BATCH_DRAWS`
  (default 4) of the 32 MiB tess varying budget instead of its level-64 worst case
  (21.5 MiB for 16 triangle patches), so four worst-case draws share a batch instead of one.
  Correctness does not depend on the charge: `panlib_v7_tess_prefix()` keeps only the whole
  patches whose varyings fit the batch's indirect varying buffer.
- **Gain:** tess_perf 64 instead of 256 batches per frame on v6/v7; each avoided split saves a
  framebuffer store+reload (15-33 MB at 1080p, 2-4 ms on G52/G72) and two atoms.
- **Risk:** low-medium. A batch may now touch up to 4 x 21.5 MiB of the GROW_ON_GPF varying
  buffer per half when every patch really reaches level 64 (was 32 MiB). If a level-64 scene
  ever exceeds the per-half VA, the GPU clamp drops whole patches (visible holes, no fault).
- **Regression signal:** tess_perf hashes (4807f3f5, a03404c5, cec55885, d4463b35, b34a251c)
  must match; tessellation, gs_tess_primitive_id, geometry. Check the "tess batch budget" log
  line (now prints the charge and N) and the MEM lines.
- **Phase 2 (D1 step 2):** charge the real use (count from the prefix kernel, read back per
  batch) instead of a fixed share; needs device timing first.

### 0005 D7: v9 gpu_prerast arenas at first use
- **What:** the two v9 JM readers now use `panvk_per_arch(device_gpu_prerast_arena)()`, which
  commits the arena (fully, under `gpu_prerast_lock`) the first time a draw records such a
  pass, as csf-v11 205 does on CSF. Recording precedes any submit that can use it, so the
  arena is still never grown by GPU faults.
- **Gain:** 160 MiB and 50-100 ms of CPU clear per VkDevice that never records a gpu_prerast
  or tessellation pass (every DXVK device enables the features).
- **Risk:** low-medium: the first such draw now pays the commit and clear (one hitch).
- **Regression signal (oriole):** geometry, tessellation, gs_tess_primitive_id, xfb tests,
  tess_perf (hashes and MEM lines), large_draw.

### 0006 D4: JM warm-up sleep off by default
- **IMPORTANT:** no justification for the 2 s sleep was found (jm-v9 001 from
  FristOneRR-Panvk-Source efd07ba, no comment; jm-v7 111 kept it once per process because the
  reason was unknown; the worklogs give none). The default is now **0 (no sleep)**;
  **`PANVK_KBASE_JM_WARMUP=1` restores the 114 behaviour** (2 s once per process; 2 = every
  open). The first vtc and frag chains of each queue are already submitted synchronously with
  `kbase_kmod_job_submit_retry()` (5 attempts, 0.4-1.6 s back-off).
- **Gain:** 2 s per process: about 78 s (21-26 %) of each FTL PanProbe run on starlte, a31 and
  oriole, and 2 s per game start.
- **Risk:** low-medium (unknown original reason).
- **Regression signal:** the first submit of each process: PanProbe's first test of each run,
  any "JM atom ... failed", `event=0x` or DEVICE_LOST right after the
  "kbase: JM warm-up off" log line. If seen, rerun with `PANVK_KBASE_JM_WARMUP=1`.

## Notes for the other packages

- **P-B (BO zeroing, Q4):** D2's quick "no TLS clear" part is P-B's `kbase_kmod_bo_alloc()`
  memset (csf-v11 041), which P-D does not touch. Once P-B's no-zero BO flag exists, the JM
  TLS/WLS pools in `jm/panvk_vX_cmd_buffer.c` (and the device TLS of a later P-D patch, if
  taken) only need that flag in their create flags: TLS/WLS contents are undefined to
  shaders.
- **P-B (auto-trim on JM):** not done here (P-B owns `panvk_cmd_pool.c`).
- **P-C (D13):** the Android present `DeviceWaitIdle` is P-C's.
