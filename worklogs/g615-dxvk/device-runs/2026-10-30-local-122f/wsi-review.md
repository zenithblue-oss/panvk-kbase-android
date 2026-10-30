# WSI Review — 2026-10-30 local 122f
**Session:** 122f  **Owner:** WSI / patch 0171  **Date:** 2026-10-30

---

## Scope

Review patch 0171 (per-swapchain sw readback memory type auto-selection) as applied
in the live 122f development tree. No build execution. No device test.

---

## Paths

| Role | Path |
|------|------|
| Live Mesa tree | `/var/tmp/panvk/mesa-122f-dev` |
| Build dir | `/var/tmp/panvk/bionic-p122f` |
| Dist | `/var/tmp/panvk/bionic-p122f-dist/libvulkan_panfrost.so` |
| Patch source | `/home/abhaybyte/repos/panvk/patches/jm-v7-wip/0171-wsi-x11-pick-the-sw-readback-memory-type-per-swapcha.patch` |

---

## Build status

Patch is applied as HEAD of `mesa-122f-dev`:

```
3e17670cf5c wsi/x11: pick the sw readback memory type per swapchain from its frame rate
```

All object files are **newer** than their source files — the build is current
(no rebuild required before device test):

| Object | Src mtime | Obj mtime |
|--------|-----------|-----------|
| `wsi_common.c.o` | 1793331730 | 1793331810 ✓ |
| `wsi_common_x11.c.o` | 1793334285 | 1793334396 ✓ |
| `panvk_wsi.c.o` | 1793333554 | 1793336025 ✓ |
| `libvulkan_panfrost.so` (dist) | — | 1793336025 ✓ |

Build command (do not run until coordinated):
```
ninja -C /var/tmp/panvk/bionic-p122f
# dist copy already up to date at /var/tmp/panvk/bionic-p122f-dist/libvulkan_panfrost.so
```

---

## Files changed by patch 0171

| File | Change |
|------|--------|
| `src/panfrost/vulkan/panvk_wsi.c` | Adds `sw_cached_auto = (PANVK_WSI_SW_CACHED == NULL)` |
| `src/vulkan/wsi/wsi_common.h` | Adds `bool sw_cached_auto` to `wsi_device` |
| `src/vulkan/wsi/wsi_common_private.h` | Declares `wsi_image_set_cpu_readback_cached()` |
| `src/vulkan/wsi/wsi_common.c` | Refactors type selector; adds `wsi_image_free_blit_cmd_buffers()` and `wsi_image_set_cpu_readback_cached()` |
| `src/vulkan/wsi/wsi_common_x11.c` | Adds `sw_ab*` fields to `x11_swapchain`/`x11_image`; adds `x11_sw_ab_{set_mode,window,frame}()`; hooks into `x11_present_to_x11_sw`; initialises on swapchain create |

---

## Correctness analysis

### `wsi_image_set_cpu_readback_cached` (wsi_common.c)

**Guard preconditions (correct):**  
Rejects non-BUFFER_BLIT, alloc_shm, no cpu_map, null buffer, and PROTECTED swapchains
with `VK_ERROR_FEATURE_NOT_PRESENT`. All are runtime-invariant for a given swapchain.

**Resource swap sequence (correct):**  
1. Save old {buffer, memory, cmd_buffers, cpu_map, noncoherent}.  
2. Zero image fields (prevents double-free if early fail).  
3. CreateBuffer → GetBufferMemoryRequirements → AllocateMemory (new type) →
   BindBufferMemory → MapMemory → `wsi_finish_create_blit_context` (re-records
   cmd buffers against new buffer).  
4. On success: UnmapMemory(old), free old cmd_buffers, DestroyBuffer(old),
   FreeMemory(old).

**Fail paths exhaustively verified:**

| Failure point | Leaked? | Notes |
|---|---|---|
| CreateBuffer | No | old restored; no new resources |
| AllocateMemory | No | new buffer destroyed; `FreeMemory(VK_NULL_HANDLE)` is spec no-op |
| BindBufferMemory | No | new buffer + memory both freed; cpu_map still NULL → UnmapMemory skipped |
| MapMemory | No | same as above |
| `wsi_finish_create_blit_context` (OOM) | No | partial cmd_buffers freed; cpu_map set → UnmapMemory called; buffer+memory freed |

**No resource leaks on any fail path.**

### `wsi_image_free_blit_cmd_buffers` (wsi_common.c)

Uses same `cmd_buffer_count` formula as `wsi_finish_create_blit_context`
(`blit.queue != NULL ? 1 : queue_family_count`). Consistent.  
NULL input guard: early-returns cleanly.

### `wsi_cmd_blit_image_to_buffer` baking (wsi_common.c)

`image->blit.buffer` is baked into the command buffer at record time
(`CmdCopyImageToBuffer` + `CmdPipelineBarrier` both reference it directly).
`wsi_image_set_cpu_readback_cached` calls `wsi_finish_create_blit_context` with the
**new** buffer already set, so re-recorded cmd buffers reference the new buffer.
Old cmd buffers (referencing old buffer) are freed on success. **Correct.**

### `x11_sw_ab_window` — frame counting (wsi_common_x11.c)

`sw_ab_n` is post-incremented at entry (old value checked for `==0`).

- **Settle phase** (`!measuring`):  
  n starts 0 after `set_mode`; first call: old=0 → set `t0`; n becomes 1.  
  Settle exit fires when `n >= 30 AND elapsed >= 250 ms` (both required). Correct.  
  On exit: n reset to 1, t0 = now (first measurement frame).

- **Measure phase** (`measuring`):  
  Check `n <= 120 || elapsed < 500ms` → skip.  
  At n=121 and elapsed≥500ms: `ns = elapsed / (121-1) = elapsed / 120`.  
  t0 was set at frame 1 of measurement; 120 inter-frame intervals measured. **No divide-by-zero** (n−1 ≥ 120 guaranteed).

### ABA phase machine (wsi_common_x11.c)

| Phase | Action |
|---|---|
| 0 | Measure A (starting mode); store `sw_ab_ns[cur]`; switch to B |
| 1 | Measure B; store `sw_ab_ns[!cur]`; switch back to A |
| 2 | Measure A again; `a = (sw_ab_ns[A] + ns) / 2`; `b = sw_ab_ns[B]`; pick best if B < a×(1−3%) |
| 3 (hold) | Monitor drift; reprobe if `|drift| > 25%` and `age > 10s`; stores current A, goes to phase 1 |

`sw_ab_ns[2]` is indexed by `bool` (0=uncached, 1=cached). The starting mode
initializes `chain->sw_cached = wsi_device->sw_cached_noncoherent`; images are
initialized to match. Index semantics are consistent throughout. **ABA logic correct.**

**Reprobe:** stores one A window then goes to phase 1 (not phase 0). Phase 2 will
use that stored A + one fresh A — still a valid ABA triple. **Correct.**

### `sw_ab_pending` counter

`set_mode` recomputes `sw_ab_pending` from scratch by counting images whose
`sw_cached != target`. Not an accumulated counter — no drift. **Correct.**

On `wsi_image_set_cpu_readback_cached` failure: `sw_ab = false` disables all further
calls to `x11_sw_ab_frame`. Images remain in whatever mixed state they held; they
continue to work correctly (each has its own valid buffer+memory+cmd_buffers). The
`mesa_logw` is emitted. **Correct.**

### `sw_cached_auto` gate (panvk_wsi.c)

```c
physical_device->wsi_device.sw_cached_auto =
    os_get_option("PANVK_WSI_SW_CACHED") == NULL;
```

`PANVK_WSI_SW_CACHED` set → `sw_cached_auto = false` → `sw_ab` never enabled.  
`PANVK_WSI_SW_CACHED` unset → `sw_cached_auto = true` → `sw_ab` enabled when
`sw_defer_reply && BUFFER_BLIT && !alloc_shm`.  
`sw_cached_noncoherent` (the arch default, still used as starting mode) is set by
`debug_get_bool_option` which also reads `PANVK_WSI_SW_CACHED`. **Consistent.**

---

## Issues / gaps found

**None.** Patch 0171 is logically correct as applied. No source changes required.

---

## No-change verdict

Patch 0171 source in `mesa-122f-dev` matches the patch file exactly. Build artifacts
are current. No correctness gaps, no resource leaks, no counter drift, no
divide-by-zero, no ABA indexing errors. **Patch 0171 requires no update.**

---

## Runnable check

The following self-check covers the non-trivial frame-counting arithmetic (no test
framework needed, no device required):

```c
/* ponytail: wsi_review_check.c — compile and run standalone to verify
 * x11_sw_ab_window settle+measure frame-count invariants.
 * Build: cc -o /tmp/wsi_review_check wsi_review_check.c && /tmp/wsi_review_check
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#define X11_SW_AB_SETTLE     30
#define X11_SW_AB_SETTLE_NS  (250ull * 1000 * 1000)
#define X11_SW_AB_WINDOW     120
#define X11_SW_AB_WINDOW_NS  (500ull * 1000 * 1000)

typedef struct {
    uint32_t sw_ab_n;
    uint64_t sw_ab_t0;
    bool     sw_ab_measuring;
} chain_t;

static void set_mode(chain_t *c, uint64_t now) {
    c->sw_ab_n = 0;
    c->sw_ab_measuring = false;
    c->sw_ab_t0 = now;
}

/* Faithful copy of x11_sw_ab_window logic */
static uint64_t ab_window(chain_t *c, uint64_t now) {
    if (c->sw_ab_n++ == 0)
        c->sw_ab_t0 = now;
    if (!c->sw_ab_measuring) {
        if (c->sw_ab_n < X11_SW_AB_SETTLE ||
            now - c->sw_ab_t0 < X11_SW_AB_SETTLE_NS)
            return 0;
        c->sw_ab_measuring = true;
        c->sw_ab_n = 1;
        c->sw_ab_t0 = now;
        return 0;
    }
    if (c->sw_ab_n <= X11_SW_AB_WINDOW ||
        now - c->sw_ab_t0 < X11_SW_AB_WINDOW_NS)
        return 0;
    uint64_t ns = (now - c->sw_ab_t0) / (c->sw_ab_n - 1);
    c->sw_ab_n = 0;
    c->sw_ab_measuring = false;
    return ns;
}

int main(void) {
    chain_t c = {0};
    uint64_t t = 0;
    const uint64_t frame_ns = 2 * 1000 * 1000; /* 2ms = 500fps */
    uint64_t result = 0;
    int settle_frame = -1, measure_frame = -1;

    set_mode(&c, t);
    for (int i = 0; i < 10000 && !result; i++) {
        t += frame_ns;
        uint64_t r = ab_window(&c, t);
        if (r) {
            result = r;
            measure_frame = i;
        }
        if (!c.sw_ab_measuring && c.sw_ab_n >= X11_SW_AB_SETTLE && settle_frame < 0)
            settle_frame = i; /* last settle frame before measuring starts */
    }

    /* Settle must need >= 30 frames */
    assert(settle_frame >= X11_SW_AB_SETTLE - 1);
    /* Result must be close to frame_ns (within 5%) */
    assert(result > frame_ns * 95 / 100 && result < frame_ns * 105 / 100);
    /* n-1 was never 0 (no divide-by-zero) — implied by result being sane */
    printf("PASS: settle_frame=%d measure_frame=%d result_ns=%llu (expected ~%llu)\n",
           settle_frame, measure_frame, (unsigned long long)result,
           (unsigned long long)frame_ns);
    return 0;
}
```

Save as `/tmp/wsi_review_check.c`; build and run:
```
cc -o /tmp/wsi_review_check /tmp/wsi_review_check.c && /tmp/wsi_review_check
```
Expected output: `PASS: ...`

---

## Blockers

None. Ready for coordinated shared build and device test when scheduled.

---

*Written by WSI agent (122f session), 2026-10-30.*
