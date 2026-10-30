# bc-review.md — patch 0170 review, 2026-10-30, 122f

## Status: COMPLETE — no changes needed

Patch 0170 (`bc8e0d072d9`) is **committed and built** in the live tree.

---

## Paths

| Purpose | Path |
|---|---|
| Live tree | `/var/tmp/panvk/mesa-122f-dev` |
| Build | `/var/tmp/panvk/bionic-p122f` |
| Dist | `/var/tmp/panvk/bionic-p122f-dist/libvulkan_panfrost.so` |
| Patch file | `patches/jm-v7-wip/0170-panvk-bc-decode-with-per-texel-pipelines-until-the-b.patch` |

Build timestamp: `Oct 30 10:23` (matches commit). Tree is clean (`git status` empty).

---

## Patch 0170 — what it does

Ships both the 0153 block decoder (one invocation per 4×4 block, fast GPU
decode, slow compile ~2.5×) and the 116 per-texel decoder (one invocation per
texel, slow GPU decode, fast compile ~1×).

**First upload of a format**: compiles per-texel pipeline on the recording
thread (as fast as 116), enqueues block decoder on a 2-thread minimum-priority
background queue (`util_queue "panvk_bcdec"`).

**Later uploads**: `vk_meta_lookup_pipeline` finds the block pipeline (meta
cache key `shader` 0–3) → returns it directly. Per-texel pipeline uses
distinct key `shader+4` (4–7).

**Quiet window**: background thread polls `dec_texel_busy` and `dec_texel_ns`
before starting the block compile. Only after no per-texel compile has run for
100 ms does it proceed. A cache-hit texel compile does NOT update `dec_texel_ns`
→ block compile starts immediately after cache-hit (correct: no NIR compile
cost to hold it back).

**Env knob**: `PANVK_BC_DECODE_TEXEL=0` restores 0153 (block-only).

---

## Correctness review

### Race / lifetime

| Concern | Verdict |
|---|---|
| `dec_started` read outside lock in fast-path | **OK** — intentional TOCTOU; worst case two threads both see `first=true`, both call `create_bc_decode_pipeline(texel)` (second is a meta cache hit), both call `bc_decode_start_block` (second is a no-op under lock). Perf cost only, no correctness bug. |
| `bc_dec_job` dev pointer lifetime | **OK** — `device_bc_finish` calls `util_queue_finish` before `util_queue_destroy`, draining all jobs before device is freed. |
| `dec_texel_ns` uint64\_t via `p_atomic_*` without `_Atomic` typedef | **OK** — `p_atomic_read/set` use `__atomic_load/store` with `__ATOMIC_SEQ_CST`; works on uint64\_t on arm64. |
| `dec_fence[4]` bounds vs `bc_decode_shader_index()` range | **OK** — index returns 0–3, array size 4. No OOB. |
| `dec_texel_ns=0` at init: first `bc_dec_run` call computes `idle = now - 0` ≈ seconds >> 100 ms | **OK** — block compile starts immediately on first format when no texel compile active. Correct. |
| `bc_dec_run` ignores return value of `create_bc_decode_pipeline` | **OK by design** — background compile is a speed-up only; failure means per-texel pipeline continues to be used (block key absent from meta cache). Silent degradation, no crash. |

### Error fallback

- `get_bc_decode_pipeline` propagates `VK_ERROR_*` from `create_bc_decode_pipeline` to `bc_decode_region`, which calls `vk_command_buffer_set_error`. Clean.
- `malloc` failure for `bc_dec_job`: `job=NULL` → block compile silently skipped; `bc_decode_start_block` returns false. Per-texel used forever. Acceptable.
- `util_queue_init` failure: `dec_queue_ok=false` → same: block compile never queued. Per-texel forever. Acceptable.

### Dispatch size

- Block decoder: `wg_texels=32`, `local_size=8×8` (= 64 invocations/wg, each covers one 4×4 block → 32×32 texels/wg). `DIV_ROUND_UP(w,32) × DIV_ROUND_UP(h,32)`. Correct.
- Per-texel: `wg_texels=8`, `local_size=8×8` (64 invocations, one texel each → 8×8 texels/wg). Correct.

### Meta cache key separation

Block pipeline: key `.shader = shader` (0–3).  
Per-texel pipeline: `create_bc_decode_pipeline(block=false)` does `shader += 4` → key `.shader = shader+4` (4–7).  
No collision. `get_bc_decode_pipeline`'s initial `vk_meta_lookup_pipeline` uses the block key (0–3), so once the block compile lands it is used from the next call onward without `dec_started` check.

---

## Device results (parsed from bc/*.txt, bcw/*.txt)

| Build | bc\_level cold (ms) | bc\_decode cold (ms) | bc\_decode warm (ms) |
|---|---|---|---|
| dev (0170) | 730–1068 | 820–890 | — |
| dev9/10 (0170) | 774–869 | 877–937 | 177–251 |
| 116 baseline | 778–791 | 919 | 219 |
| 122e | 1750–1830 | 1830 | 1777 |

0170 cold bc\_decode matches or beats 116 (849 vs 919 ms in bc7.txt).
Warm bc\_decode 177–251 ms vs 116's 219 ms — competitive.
122e was the regression baseline (1830 ms cold, 1777 warm). Fixed.

bc\_level (full level load including all formats) settled at 774–936 ms across
dev/dev2-6, consistent with commit message's 849 ms figure.

---

## Pending diffs / blockers

None. Tree clean, build current, dist `.so` current (Oct 30 10:23).

Patch 0170 is complete. No further changes required to BC source or the patch
file. Patch file context will not apply to upstream mesa (different tree state)
— that is expected; the patch applies to the `mesa-122f-dev` series only.

---

## Focused check

`worklogs/g615-dxvk/device-runs/2026-10-30-local-122f/bc-check-0170.c`

Build: `cc -O0 -g -o bc-check-0170 bc-check-0170.c && ./bc-check-0170`

Checks: quiet-window invariants (fresh init, busy=1 blocks, just-finished blocks,
elapsed allows, cache-hit allows), dec\_fence bounds.

Result: **PASS** (verified Oct 30 2026).
