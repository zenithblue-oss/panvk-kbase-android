# Tessellation performance review: CSF v10/v11 (G615), 2026-10-22

Trigger: Bachata games run very slowly with tessellation on (X6 Pro, Mali-G615, v11).
Scope: PanVK tessellation emulation on CSF (`gpu_prerast_tess()`), with notes for v6/v7 (jm-v7-wip).
Tree read: `/home/flux/src/mesa-bc5-strict` (bc5-strict, common..csf-v11 209). Line numbers refer to that tree.
No device run backs this yet. Every time below is an estimate, and the first device run of `tess_perf` (below) is the A/B that settles them.

## How a tess draw runs today

`csf/panvk_vX_cmd_draw.c:4908 gpu_prerast_tess()` records two CS loops. One runs on the compute queue and one on the vertex/tiler (VT) queue. They hand off chunks of patches through queue syncobjs (`TESS_FREE` / `TESS_READY`, :5198 / :5335 / :5350 / :5376). Both loops share one device-global 96 MiB tess arena.

Compute side, per chunk of at most `max_patches` (:4938, about 507 patches):

1. take `TESS_FREE`, then a CS once-block (predicate drop, 3 LS waits)
2. `panlib_tess_step` (draw_helper.cl:188, 1 thread) (:5239)
3. VS as lowered compute
4. TCS (:5260): one workgroup of `out_patch_size` threads per patch
5. `panlib_tessellate` COUNT (:5264, draw_helper.cl:252)
6. `panlib_tess_draw` (:5267, draw_helper.cl:289): a **single-thread serial prefix sum** over all patches of the chunk
7. `panlib_tessellate` WITH_COUNTS (:5270), plus a **serial ccw swap post-pass per patch** (draw_helper.cl:268-276)
8. give `TESS_READY`

Every step is followed by `cs_wait_slots(all_iters)`, a full drain. Before the loop, `panlib_tess_setup` (:5187) runs and is waited on.

VT side, per chunk: take `TESS_READY`, then the IDVS draw (TES as VS), drain, give `TESS_FREE`. On the nested GS path, TES runs as lowered compute per index and then GS, in ranges of at most 65536 vertices, so a chunk can take many handshakes.

What it does **not** do: no CPU waits, no cache flushes or invalidates per draw, no draw-time shader recompiles, no memsets of the arena, and no CPU fallback for the fractional modes. The cost is GPU latency and serialization, not CPU stalls.

## Cost model at DXVK usage (many small draws, factors 1-16)

| Item | Per draw (small direct draw, 1 chunk) |
|---|---|
| Dependent compute dispatches, each a full drain | 7 (setup + step + VS + TCS + COUNT + draw + GEN), about 5-15 us each, so 40-100 us |
| Cross-queue hand-offs (READY, FREE) | 2, about 5-10 us each |
| VT draw + drain | 5-20 us |
| CPU record (allocs, descriptors, full VT state re-emit) | about 15-40 us (CPU side, overlaps the GPU) |
| **GPU critical path** | **about 60-150 us per draw**, and compute never overlaps VT (one arena) |

Per patch: COUNT, prefix and GEN are dominated by the tessellator, which runs one thread per patch and generates serially. At L16 a quad patch is 289 points and 1536 indices for one thread. The serial ccw pass adds about 30% of GEN time at high factors. The single-thread prefix sum adds about 1 us per 10-20 patches.

Frame: 200-300 tess draws per frame gives **12-40 ms of tess overhead per frame** before any real shading. That alone caps a frame at 25-80 fps, and it gets worse when draws use factors above 1. This matches "super slow with tess on".

## Ranked findings

| # | Where | Issue | Impact | Fix |
|---|---|---|---|---|
| 1 | `panvk_vX_device.c` tess arena, `cmd_draw.c:5196-5376` | A single tess arena per device: VT draw N must give `TESS_FREE` before compute can start draw N+1. Compute and VT strictly alternate and never overlap. | Highest: the whole compute chain is exposed on every draw | **csf-v11 215**: two arenas, odd and even tess draws alternate, so draw N+1's compute overlaps draw N's VT |
| 2 | `cmd_draw.c:5239-5272` | 7 dependent dispatches per chunk, each followed by a full `cs_wait_slots` | 40-100 us per draw | **213** fuses COUNT, prefix and draw (and GEN for at most 64 direct patches) into one dispatch, so 3 dispatches become 1 or 2. **214** folds setup into step, so 2 become 1 |
| 3 | `draw_helper.cl:289-322 panlib_tess_draw` | Serial single-thread prefix sum over up to about 507 patches | 1 thread walking up to 507 entries | **213**: `panlib_tess_count_draw`, a 64-thread workgroup with a Hillis-Steele scan |
| 4 | `cmd_draw.c:5187` + once-block | Separate setup dispatch + wait; the CS once-block costs 3 LS waits | about 10-20 us per draw | **214**: `panlib_tess_setup_step` runs setup in the first step |
| 5 | `draw_helper.cl:268-276` (v9/v10/v11), `panlib_v7_tessellate` post-pass (jm-v7-wip) | Serial ccw post-pass: re-reads and re-writes every index of the patch after generation | about 30% of GEN time at L16 | **216** (CSF) + **jm-v7 104** (v6/v7): CHWTessellator writes the swapped triangle directly (`POLY_TESS_CCW_INLINE`) |
| 6 | `src/poly/cl/tessellator.h` (shared) | Tessellator runs one thread per patch with serial generation. Small draws (16 patches) occupy 16 threads of the GPU | Low occupancy; GEN time scales with factor squared on one thread | Not done: per-ring or per-row parallel generation (bigger rewrite) |
| 7 | `cmd_draw.c:5260` TCS launch | One tiny workgroup (3-4 threads) per patch | Low occupancy on small draws | Not done: pack several patches per workgroup (needs TCS lowering changes) |
| 8 | Nested GS path (`cmd_draw.c` nested branch) | TES runs per index, not per unique point (about 5x the work at L16). Ranges of at most 65536 vertices mean many READY/FREE handshakes per chunk | GS-on tess draws several times slower than GS-off | Not done: run TES over unique points (`coord_allocs`) and index the GS input. Raise or remove the 65536 range cap |
| 9 | `gpu_prerast_tess_max_patches` (:4938) | Chunks are sized for the worst case (132 KB per patch at factor 64), so a chunk caps at about 507 patches even at L1 | Large draws split into many chunks, each paying the full handshake | Not done: size chunks from the COUNT results, as v9 `idx_limit` does |
| 10 | CPU recording per tess draw | Allocs, descriptor upload and a full VT state re-emit, about 15-40 us | CPU-bound frames with many draws | Not done: cache the VT state across consecutive tess draws |

## Patches (csf-v11, plus jm-v7-wip)

Every patch has an env toggle. The driver reads these at `vkCreateDevice`, except `jm-v7 104`, which reads its toggle per draw.

| Patch | Toggle (default) | Change |
|---|---|---|
| `csf-v11/213-count-prefix-sum-and-draw-a-tess-chunk-in-one-workgroup` | `PANVK_TESS_FUSE` (1), `PANVK_TESS_FUSE_GEN_MAX` (64) | New `libpan/tess_fast.cl`. `panlib_tess_count_draw` runs COUNT for every patch, a parallel scan and the `panlib_tess_draw` bookkeeping in one 64-thread workgroup. For direct draws of at most `FUSE_GEN_MAX` patches it also generates (WITH_COUNTS) in the same dispatch. Otherwise it runs `panlib_tess_gen` |
| `csf-v11/214-set-up-a-tess-draw-in-its-first-step-dispatch` | `PANVK_TESS_SETUP_STEP` (1) | `panlib_tess_setup_step` runs setup and the per-draw zeroing on the first iteration. It drops the pre-loop setup dispatch and wait, and the CS once-block |
| `csf-v11/215-alternate-tess-draws-over-two-tessellation-arenas` | `PANVK_TESS_ARENAS` (2) | A second 96 MiB arena (lazy; falls back to 1 if allocation fails) with its own READY2/FREE2 syncobjs. Odd tess draws use it, so compute for draw N+1 overlaps VT for draw N |
| `csf-v11/216-write-counter-clockwise-tess-triangles-in-the-tessellator` | `PANVK_TESS_WINDING_INLINE` (1) | `POLY_TESS_CCW_INLINE` (bit 1 of `ccw`) makes CHWTessellator swap winding in `DefineTriangle`. The serial post-pass runs only for bit 0. v9 still passes 0 or 1 (unchanged) |
| `jm-v7-wip/0077-jm-v7-104-v6-v7-counter-clockwise-tess-triangles-fro` | `PANVK_TESS_WINDING_INLINE` (1) | Same as 216 for `panlib_v7_tessellate` (v6/v7). Depends on 216 |

All defaults are on. The old behaviour is: `PANVK_TESS_FUSE=0 PANVK_TESS_SETUP_STEP=0 PANVK_TESS_ARENAS=1 PANVK_TESS_WINDING_INLINE=0`.

Memory: with tessellationShader enabled, 215 adds 96 MiB of virtual space per VkDevice, committed on the first odd tess draw. `PANVK_TESS_ARENAS=1` returns to a single arena.

### Checks done (host only, no .so built)

- The full g615-v11-csf series (common .. csf-v11 1-216, jm-v9, jm-v7) applies strictly on `mesa-bc5-strict` 5a07217f (211 patches). jm-v9 011 and 023 needed hunk-header-only refreshes, because 213-215 add lines to `panvk_vX_device.c`. They are committed with this change.
- `git am` of jm-v7-wip 0006-0076 and the new 0077 applies on top.
- libpan `mesa_clc`, `vtn_bindgen2` and `panfrost_compile` pass for v6, v7, v9, v10 and v11.
- With NDK clang `-fsyntax-only`, using the bc5 compile db: `csf/panvk_vX_cmd_draw.c` (v10, v11), `csf/panvk_vX_gpu_queue.c` (v10, v11), `panvk_vX_device.c` (v7, v9, v10, v11), and `jm/panvk_vX_cmd_draw.c` (v7, v9) all pass.

## Expected speedup (estimate, to be measured)

- Small direct draws (16 patches): the compute chain goes from 7 dispatches to 4 (step+setup, VS, TCS, count+draw+gen). Of 2 handshakes per draw, about half are now hidden behind the other arena. Per-draw constant cost drops roughly 45-60%.
- Higher factors (L16): no serial ccw pass, and the scan is parallel. GEN time drops about 30%. The tessellator itself is still serial per patch (#6).
- Many-small-draw tess frames (Bachata-like): about **1.6-2.2x** on the tess part of the frame. Frames where GS-on draws dominate gain less (#8 remains).

## Bench: PanProbe `tess_perf` (device/tess-perf.c)

Each frame is 256 draws × 16 quad patches, with a push constant per draw. Configs: `L=ref` (same draws without tess), `L=1`, `L=4`, `L=16`, and `L=mix` (per-patch factors 1-16). Each runs with the GS off and on.

It runs two variants in-process, each on a fresh VkDevice: `default` and the legacy env set. Each prints one line per config:

```
BENCH tess_perf opt=<variant> gs=<0|1> L=<cfg> draws=256 patches=4096 ms=<frame ms> us_draw=<us> frames=<n> hash=<readback>
```

At the end it prints `FPS` (default, gs=0, L=mix) and `PASS` or `FAIL`. It FAILs if any config's readback hash differs between variants. Use `PT_TESS_AB` to choose the variants, separated by `;`, each a list of `NAME=VAL` joined by `,` (`default` means no overrides). `PT_TESS_MS` sets the time budget per config (default 700 ms).

What the bench should show on G615:

- `us_draw` for L=1 is far above L=ref: the fixed per-draw chain.
- L=16 and mix are slower than L=1: serial generation.
- gs=1 is slower than gs=0: the nested path.
- `opt=default` beats the legacy variant mainly on L=1 and mix.

Per-toggle bisect: run with `PT_TESS_AB="default;PANVK_TESS_ARENAS=1;PANVK_TESS_FUSE=0;PANVK_TESS_SETUP_STEP=0"`.

## Next steps (not done here)

1. Device A/B with `tess_perf`, then a Bachata capture with and without the toggles.
2. #8: run nested TES over unique points. This is likely the next biggest win for DXVK titles that use a GS with tessellation.
3. #9: size chunks from the counts.
4. #6 and #7: parallel tessellator, and packing several patches per TCS workgroup.
