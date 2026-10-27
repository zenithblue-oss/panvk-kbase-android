/* Host unit test for sparse_kbase.h (no device):
 *   cc -O1 -o /tmp/sparse_kbase_test tests/kbase-probe/sparse_kbase_test.c && /tmp/sparse_kbase_test
 * Cases: the per-arch outcomes of worklogs/g615-dxvk/sparse-kbase-2026-10-27/README.md Q3 (CSF v10-v13
 * lazy-tile, JM v6/v7/v9 and v14 with ALLOC_EX ENOTTY eager-only, no kbase not possible), the gpuprops parser
 * and the FIXED window math. */
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include "sparse_kbase.h"

static size_t
put(uint8_t *b, size_t o, uint32_t key, unsigned log2sz, uint64_t v)
{
   uint32_t hdr = key << 2 | log2sz;
   memcpy(b + o, &hdr, 4);
   memcpy(b + o + 4, &v, 1u << log2sz);
   return o + 4 + (1u << log2sz);
}

int
main(void)
{
   const struct sk_facts csf = {.ctx = 1, .same_va = 1, .arena_grow = 1, .fixed_va = 1, .fixed_rebind = 1, .tile_cycle = 1};
   const struct sk_facts jm = {.ctx = 1, .same_va = 1, .arena_grow = 1};
   const struct sk_facts none = {0};
   struct sk_facts f;

   assert(sk_verdict(&csf) == SK_LAZY_TILE);
   f = csf, f.same_va = 0, f.arena_grow = 0;
   assert(sk_verdict(&f) == SK_LAZY_TILE); /* FIXED tiles do not need the SAME_VA arena */
   f = csf, f.tile_cycle = 0;              /* fully-backed kernel: uncommit ignored */
   assert(sk_verdict(&f) == SK_EAGER_ONLY);
   f = csf, f.fixed_rebind = 0;
   assert(sk_verdict(&f) == SK_EAGER_ONLY);
   f = csf, f.fixed_va = 0; /* v14: ALLOC_EX ENOTTY */
   assert(sk_verdict(&f) == SK_EAGER_ONLY);
   assert(sk_verdict(&jm) == SK_EAGER_ONLY);
   f = jm, f.arena_grow = 0; /* arena growth is reported, not decisive */
   assert(sk_verdict(&f) == SK_EAGER_ONLY);
   f = jm, f.same_va = 0;
   assert(sk_verdict(&f) == SK_NOT_POSSIBLE);
   f = csf, f.ctx = 0; /* no context: nothing else counts */
   assert(sk_verdict(&f) == SK_NOT_POSSIBLE);
   assert(sk_verdict(&none) == SK_NOT_POSSIBLE);
   assert(!strcmp(sk_verdict_name(SK_LAZY_TILE), "lazy-tile possible"));
   assert(!strcmp(sk_verdict_name(SK_EAGER_ONLY), "eager-only"));
   assert(!strcmp(sk_verdict_name(SK_NOT_POSSIBLE), "not possible"));

   /* gpuprops: u8, u32 and u64 values, missing key, truncated value */
   uint8_t b[64];
   size_t o = put(b, 0, 7, 0, 0x5a);
   o = put(b, o, 32, 2, 0x2830); /* RAW_MMU_FEATURES: va_bits 0x30 = 48 */
   o = put(b, o, 55, 3, 0xa8670000ull);
   assert(sk_prop_get(b, o, 7, 0) == 0x5a);
   assert((sk_prop_get(b, o, 32, 0) & 0xff) == 48);
   assert(sk_prop_get(b, o, 55, 0) == 0xa8670000ull);
   assert(((sk_prop_get(b, o, 55, 0) >> 28) & 0xf) == 10); /* G710 arch v10 */
   assert(sk_prop_get(b, o, 99, 123) == 123);
   assert(sk_prop_get(b, o - 1, 55, 77) == 77); /* last value cut short */
   assert(sk_prop_get(b, 0, 7, 9) == 9);

   /* FIXED window inside the 64-bit FIXED_VA zone, 2 MiB aligned */
   assert(SK_FIXED_VA_BASE == 0x800200000000ull);
   assert(SK_FIXED_R >= SK_FIXED_VA_BASE && SK_FIXED_R + SK_FIXED_SPAN <= (1ull << 48));
   assert(!(SK_FIXED_R & ((2ull << 20) - 1)));

   printf("sparse_kbase_test: all checks passed\n");
   return 0;
}
