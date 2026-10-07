# Shader compile stalls (multi-second hitches in DXVK games)

Date: 2026-10-07. Patches: `csf-v11/152`, `153`, `154` (all in
`src/panfrost/compiler/bifrost/bi_ra.c`).

## Evidence

- NFS Most Wanted perf capture (`/var/tmp/panvk/nfs-stutter/runs/nfs-23640`):
  three bursts of 3.6, 3.7 and 4.1 s on the `dxvk-cs` thread. All of them are
  in `vk_common_CreateGraphicsPipelines -> vk_compile_shaders ->
  panvk_compile_shaders`, and nearly all of that time is in
  `bi_register_allocate`. The DXVK log shows "Compiling shader ... cache miss"
  for the three slow hashes right when they are first used.
- The cause is the LCRA spill loop in `bi_register_allocate()`. When
  allocation fails, it spills one node and then redoes liveness, interference
  and solve for the whole shader. A shader that needs N LCRA spills takes N
  passes, up to 2000.
- `bi_spill_ssa()` runs first and is meant to make the LCRA spill loop rare.
  It gets k = 64. `bi_calc_register_demand()` then adds 8 reserved registers
  for the spiller's parallel copies, and vector widths are rounded up. So
  demand after SSA spilling was still 77-87 (measured), and LCRA had to spill
  the rest one pass at a time.
- Per pass, about 85-90% of the time is interference building. This is about
  70 ns per (destination, live node) pair, and around 100k pairs per pass on
  a 2700-instruction pixel shader.

## Harness

- Capture hook in `pan_shader_compile()`: `PAN_SHADER_CAPTURE=dir` writes
  each compile's inputs and serialized NIR. `pan_shader_replay` deserializes
  and times `pan_shader_compile`.
- Both live only in the scratch tree `/var/tmp/panvk/wt-shc` (commit
  `98498d5`). They are not part of the series.
- Scripts and data are in `/var/tmp/panvk/shc`:
  - `eval.sh`: host replay of a list.
  - `ana.py`: compares two modes.
  - `ch-replay.sh` and `ch-shc-cts.sh`: device chroot.
  - `gen.py`: synthetic shaders.
- Corpus, 67539 shaders:
  - 67492 captured from the CTS shader subset (`spirv_assembly`, `glsl`,
    `graphicsfuzz`, 96129 cases). 451 of them spill.
  - 47 synthetic D3D9-style shaders: constant-buffer `c[]`, 16-48 live vec4
    temporaries, texture taps and loops. They are compiled through a real
    `vkCreateGraphicsPipelines`. They cover the NFS-like range (1-4 s on
    device) and far beyond it (up to 157 s).
- The 3 NFS shaders themselves were not captured. That needs a run of the
  game, which the user does themselves (see "What to test").

## Changes

- **152, lossless:** interference loops only over the nodes that are live.
  `lcra_solve` builds a 64-bit forbidden mask in one row pass instead of one
  `lcra_test_linear` row scan per candidate register.
  - Output is identical to before on all 67539 shaders: regs, spills, fills,
    instruction counts and sizes.
  - Host replay of the whole corpus: 773 s before, 319 s after.
- **153, batch spilling:** the 64-register attempt keeps solving past
  failures and records every failed node. The spiller then picks one
  candidate per failed node and spills all of them before the next pass.
  - The candidate choice is the same cost/benefit choice as before, and a
    chosen node is not picked again.
  - Constraint counts are cached per round.
  - Passes on the worst shaders drop from 88-290 to 2-26.
- **154, SSA spill target:** `bi_spill_ssa(ctx, MAX2(regs_to_use - 8, 16))`
  leaves room for the reservation.
  - More spilling happens in the SSA spiller, which places spill code better.
  - With 153, total fills drop below the old code.

### Policies evaluated (host, full corpus, before 154 unless noted)

| mode | sum | max | fills | notes |
|---|---|---|---|---|
| base (one spill per pass) | 773 s | 62.8 s | 80721 | |
| one spill per pass + 152 | 319-340 s | 26 s | 80443 | |
| batch all failed | 261 s* | 3.3 s | 102455 | +27% fills |
| batch, cap doubles per pass (GEO=1) | 139 s* | 2.6 s | 92174 | |
| batch, cap x4 per pass (GEO=2) | 104 s* | 2.0 s | 95693 | |
| **batch all + k = 56 (152+153+154)** | **86-93 s** | **0.95-1.07 s** | **79843** | chosen |
| GEO=2 + k = 56 | 90 s | 1.4 s | 78883 | slightly better fills, needs a cap |

*Wall-clock numbers are from 8 parallel replays and are noisy. The sums
are only good for ranking.

Rejected:

- Lower k alone (60/56/52/48 with one spill per pass): passes 31-93, still
  slow. LCRA keeps failing after SSA spilling because of the out-of-SSA phi
  copies and lowered vectors, not because of total demand. Even
  k = 40 (demand 53) needs 4-9 LCRA passes.
- Avoiding nodes with short live ranges as spill candidates: no effect.
- Squeezing indices every pass: no effect.
- "Independent" batch (skip failed nodes already covered by a spill): too
  many passes.
- Faster interference containers:
  - Batching unsorted constraints with a sort-merge flush: identical
    output, no speedup.
  - Set-bit constraint computation: no speedup.
- Skipping the 32-register attempt when SSA spilling happened: saves about
  10%. But 2 of 378 SSA-spilled shaders still fit in 32 registers, so it
  would cost occupancy. Kept the attempt.

## Results

Device (G615, glibc chroot replay, single thread, `pan_shader_compile` time):

| set | before | after |
|---|---|---|
| CTS corpus p50 / p99 / max | 0.39 / 7.55 / 1686 ms | 0.38 / 6.90 / 300 ms |
| CTS corpus shaders > 100 ms / > 250 ms | 28 / 7 | 11 / 1 |
| CTS spilling shaders (451) p50 / p99 / max | 4.9 / 39.0 / 108 ms | 4.2 / 22.7 / 112 ms |
| synthetic (47) p50 / max / sum | 2401 ms / 156.9 s / 1523 s | 117 ms / 1.34 s / 14.1 s |
| synthetic shaders in the NFS range (base 0.3-4.0 s, 16 shaders) | 293-3989 ms | 28-249 ms |

- The one CTS shader still over 250 ms is a 30k-instruction compute shader
  with no spilling. It is plain RA and scheduling cost: 1686 -> 300 ms.
- Named synthetic shaders, device:

  | shader | before | after |
  |---|---|---|
  | FS-1786 | 293 ms | 28 ms |
  | FS-5a7c | 984 ms | 56 ms |
  | FS-5c51 | 1182 ms | 46 ms |
  | FS-c14f | 2224 ms | 152 ms |

- NFS's three pipelines (3.6-4.1 s on device) correspond to the 2.0-4.0 s
  base synthetic shaders, which now take 49-249 ms. NFS itself was not run.
- Code quality, host, all 494 shaders that spill:
  - fills 80721 -> 79843
  - instructions +0.02% in total, +0.9% geomean on spilling shaders (max
    +10.7%, min -8.8%)
  - 32-register (full occupancy) shaders unchanged: 57908

Targets: "worst < 100 ms, no shader > 250 ms" is met for the CTS spilling
shaders (max 112 ms) and for the NFS-range synthetic shaders (max 249 ms).
It is not met for:

- the 30k-instruction CS (300 ms, no spilling)
- the extreme synthetic shaders (demand 500-1000, 0.4-1.3 s)

Going further needs a cheaper interference build per pass or an SSA-based
allocator. Interference is now about 85% of the remaining time.

## Correctness

- CTS shader subset (`spirv_assembly`, `glsl`, `graphicsfuzz`; 96129 cases)
  in the glibc chroot, with the same glibc ICD build before and after:
  - Before: 68227 Pass / 634 Fail / 27268 NotSupported.
  - After: 68227 Pass / 633 Fail / 27268 NotSupported / 1 DeviceLost.
  - Every case has the same status except
    `glsl.loops.special.do_while_dynamic_iterations.dowhile_trap_vertex`.
    That test already failed before, after running 4.7 s near the GPU
    timeout. Rerun alone 2x before and 2x after, it gives Fail in about
    4.8 s every time, so the DeviceLost was a timeout flake.
  - 0 regressions.
- PanProbe (panvk-test APK, side-by-side `dev.zenithblue.panvktest.shc`,
  bundled Android ICD from a clean series tree + 152-154, BuildID
  `7f0299197d33c9c00a7e2d76e5d5b6e1dfbc5f0f`): Run all 37/37 pass.

## GPL finding (why compiles land on dxvk-cs)

- panvk exposes `VK_EXT_graphics_pipeline_library`, plus EDS3
  `DepthClipEnable`, `RasterizationSamples`, `SampleMask` and
  `AlphaToCoverage`. The NFS DXVK log shows all of them, and GPL is used.
- Mesa's `vk_pipeline` fast-link without `LINK_TIME_OPTIMIZATION` reuses the
  library shaders and does not recompile. So the dxvk-cs compiles come from
  DXVK itself, in one of two ways:
  1. `createBasePipeline` -> `acquirePipelineHandle()` compiles a shader
     library synchronously when the dxvk-shader workers have not reached it
     yet. New shaders hit this: the slow hashes are compiled right at first
     use.
  2. `canCreateBasePipeline()` is false for state-dependent reasons, and
     DXVK compiles the optimized pipeline synchronously. Those reasons are:
     - flat shading inputs
     - a non-identity render target swizzle
     - FS/VS interface mismatch
     - dual-source blend
     - line or point fill
     - conservative raster
     - sample-rate shading
- None of these are driver gates that panvk fails. Both paths call the same
  compiler, so making compiles fast is the fix. In the perf capture, 12318
  compile samples were on dxvk-cs and 2517 on the dxvk-shader workers.

## What to test in-game

- NFS Most Wanted with `DXVK_HUD=full`. The 3-4 s freezes on new effects or
  areas should drop to short hitches, about 0.25 s or less.
- To get exact numbers for the NFS shaders, run the game once with an ICD
  built from `/var/tmp/panvk/wt-shc`, which has the capture hook, and set
  `PAN_SHADER_CAPTURE=<dir>`. Then replay the `.psc` files with
  `pan_shader_replay`.
- Any other DXVK title with first-use stutter.

## Round 2: compile cost per pipeline (csf-v11/160-164)

After 152-154, each of the 3 slow NFS pipelines still took about 0.65 s
on the device, 0.42 s of it in register allocation. Device perf profile of
the 152-154 driver in NFS (`nfs-23197`), RA samples: SSA spiller 290 (SSA
repair 159), RA liveness 260, interference 260. NIR: `bi_optimize_loop`
638, `nir_opt_algebraic` 251, `nir_opt_copy_prop_vars` 140.

Harness: scratch tree `/var/tmp/panvk/wt-cc` (not in the series). It adds
phase timers (`PAN_PHT=1`: RA phases, backend phases, every `NIR_PASS`),
captures before `pan_preprocess_nir` and `pan_postprocess_nir`, and an
output hash. Scripts: `ch-cc2.sh` (device: pipeline time plus per-phase
replays), `pc.py`, `pht.py`, `cmp.py` in `/var/tmp/panvk/shc`.

All five patches are lossless. The binary is identical on all 67539
corpus shaders, and NIR is identical on the 65 synthetic pre/post
captures. Code quality is unchanged: instructions, spills, fills, register
count.

- **160:** one LCRA constraint build is shared by the 32 and 64 register
  attempts. Only affinities differ, so both masks are kept. Block liveness
  tracks only "global" nodes (read before written in some block).
  Constraints are queued and sorted into rows at the end with two
  counting sorts. Before, each one was inserted into a sorted sparse row,
  which moved about 56 entries per insert.
- **161:** after a spill round the constraint state is updated, not
  rebuilt. Spilling only changes the spilled nodes and adds fill
  temporaries, so only constraints with a spilled or new node are redone.
  All nodes of a round are spilled from one scan of the shader. The fill
  temporaries are numbered as before.
- **162:** SSA repair keeps one flat (block, variable) map, instead of a
  `hash_table_u64` per block with a ralloc copy per definition.
- **163:** `bi_optimize_loop` uses `NIR_LOOP_PASS_NOT_IDEMPOTENT`. A pass is
  skipped only while no other pass made progress since it last ran without
  progress.
- **164:** constraint rows stay sparse up to a quarter of the nodes. A dense
  row (more than 256 entries before) cost a full node scan per solver use.

Host per phase (x86, `pan_shader_compile`, 18 synthetic shaders in the
NFS range, ms, before = 152-154):

| phase | before | after |
|---|---|---|
| total | 955 | 309 |
| RA | 832 | 190 |
| interference build (113 / 25 builds) | 697 | 26 + 40 sort |
| solve, 32-register attempt (incl. rebuild before) | 161 | 1 |
| solve, 64-register passes (incl. rebuild before) | 579 | 16 |
| spill rounds (spill + constraint update after) | 35 | 49 |
| SSA spiller | 41 | 32 |
| `nir_opt_algebraic` (backend) | 14.7 | 9.7 |

65 very large synthetic shaders (syn2): 23.7 s -> 5.1 s, RA 22.3 -> 3.8 s.
By patch, NFS-range / syn2: 160 955 -> 472 ms / 23.7 -> 13.8 s, 161 ->
345 / 8.0 s, 162 -> 335 / 7.4 s, 163 -> 325 / 7.2 s, 164 -> 309 / 5.1 s.
NIR, 65 syn2 captures with 163: preprocess 1198 -> 1144 ms, postprocess
334 -> 307 ms.

Device (G615, glibc chroot, full `vkCreateGraphicsPipelines` through
`shc_pipe`, one run each, 63 synthetic pipelines, before = 152-154):

| pipelines by "before" time | n | before | after | after > 150 ms |
|---|---|---|---|---|
| < 300 ms | 35 | 19-295 (median 115) | 14-118 (median 63) | 0 |
| 300-1000 ms | 11 | 322-933 (median 751) | 142-318 (median 298) | 10 |
| 1-4.4 s | 17 | 1060-4394 (median 1604) | 282-915 (median 509) | 17 |
| all | 63 | sum 44.1 s, max 4394 | sum 14.0 s, max 915 | 27 |

Device CTS corpus replay (67492 shaders, `pan_shader_compile`, minimum of
2 runs each, same session):

| set | before | after |
|---|---|---|
| all: p50 / p99 / max | 0.36 / 6.49 / 294 ms | 0.36 / 5.12 / 262 ms |
| shaders > 100 ms | 11 | 5 |
| spilling (451): p50 / p99 / max | 4.06 / 23.3 / 111 ms | 2.83 / 15.9 / 70 ms |

What is left in the pipelines that took 0.6-0.8 s before (the NFS cost
class, now 0.28-0.32 s on the device):

- preprocess NIR, about 100 ms. `nir_opt_algebraic` is 80 ms of it. It
  drops the float range analysis cache after every replacement, then
  recomputes it. On the 152-154 NFS profile this was not visible.
- backend 115-140 ms, of which RA is 70-90 ms.
- about 50 ms in SPIR-V to NIR and the pass-through stages.

So the "< 150 ms" target for the 3 NFS pipelines is probably not met.
Expect about 0.3 s, down from 0.65 s. The "< 100 ms" target for synthetic
shaders is met for the ones that took under 300 ms before (max 118 ms
pipeline, single run). It is not met for the bigger ones. Next steps,
neither lossless:

- range analysis that survives `nir_opt_algebraic` replacements
- fewer LCRA passes after SSA spilling (fixed cost per pass: liveness and
  constraint update)

Correctness:

- PanProbe (panvk-test APK, `dev.zenithblue.panvktest`, bundled Android
  ICD from a clean series tree with 160-164, BuildID
  `4a93096816dee3df54a7c201a6a9afa26df47795`): Run all 37/37 pass.
- CTS subset (`ch-shc-cts.sh`, same 96129 cases as the v3 run, clean series
  tree with 160-164): 68227 Pass / 634 Fail / 27268 NotSupported, v3 was
  68227 / 633 / 27268 / 1 DeviceLost. Only diff:
  `glsl.loops.special.do_while_dynamic_iterations.dowhile_trap_vertex`
  DeviceLost to Fail (known flaky trap test). No new failures.
