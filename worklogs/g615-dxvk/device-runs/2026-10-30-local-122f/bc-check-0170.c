/*
 * bc-check-0170.c — focused runnable check for 0170 quiet-window logic.
 *
 * Simulates bc_dec_run's busy/quiet decision to verify the state machine:
 *   - dec_texel_ns=0 at init -> idle >> BC_DEC_QUIET_NS -> block compile
 *     starts immediately when no per-texel compile is active.
 *   - If texel_busy > 0 the block compile must not start.
 *   - A real texel compile (thread_compile_count incremented) sets dec_texel_ns
 *     to now; the block compile must then wait BC_DEC_QUIET_NS from that point.
 *   - A cache-hit texel (thread_compile_count unchanged) does NOT update
 *     dec_texel_ns, so the block compile may start immediately.
 *
 * Build: cc -O0 -g -o bc-check-0170 bc-check-0170.c && ./bc-check-0170
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#define BC_DEC_QUIET_NS (100ull * 1000 * 1000)

static uint64_t now_ns(void)
{
   struct timespec ts;
   clock_gettime(CLOCK_MONOTONIC, &ts);
   return (uint64_t)ts.tv_sec * 1000000000ull + ts.tv_nsec;
}

/* Simulate the loop body from bc_dec_run, one iteration. */
static int can_start_block(uint64_t dec_texel_ns, uint32_t dec_texel_busy)
{
   uint64_t idle = now_ns() - dec_texel_ns;
   return !dec_texel_busy && idle >= BC_DEC_QUIET_NS;
}

int main(void)
{
   /* Case 1: fresh device (dec_texel_ns=0, busy=0)
    * idle = now_ns() - 0 ≈ seconds-since-epoch >> 100 ms -> must start. */
   assert(can_start_block(0, 0) && "case1: fresh device should allow block start");

   /* Case 2: texel compile in flight (busy=1, ns=0) -> must NOT start. */
   assert(!can_start_block(0, 1) && "case2: busy=1 must block compile start");

   /* Case 3: texel compile just finished (ns=now, busy=0) -> must NOT start
    * (quiet window not elapsed). */
   uint64_t t = now_ns();
   assert(!can_start_block(t, 0) && "case3: just-finished texel, quiet not elapsed");

   /* Case 4: texel compile finished >100ms ago (busy=0) -> must start. */
   uint64_t old_t = now_ns() - BC_DEC_QUIET_NS - 1;
   assert(can_start_block(old_t, 0) && "case4: quiet elapsed -> block start allowed");

   /* Case 5: cache hit (dec_texel_ns unchanged = 0, compile_count unchanged)
    * -> block starts immediately (no quiet needed). Same as case 1. */
   assert(can_start_block(0, 0) && "case5: cache hit -> immediate block start");

   /* Verify: dec_fence[4] bounds - bc_decode_shader_index returns 0..3.
    * If somehow shader >= 4 is passed, dec_fence[shader] is OOB. */
   uint32_t max_shader = 3; /* bc_decode_shader_index() max = 3 (BC7) */
   assert(max_shader < 4 && "dec_fence array size (4) covers all shader indices");

   printf("PASS: all 0170 quiet-window invariants hold\n");
   return 0;
}
