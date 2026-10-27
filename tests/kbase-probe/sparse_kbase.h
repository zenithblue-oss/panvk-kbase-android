/* Sparse residency on stock kbase, S0 gate: the per-arch verdict and the gpuprops parser, pure (no ioctls).
 * Used by sparse_kbase.c (PanProbe test) and sparse_kbase_test.c (host unit test:
 * cc -O1 -o /tmp/sparse_kbase_test tests/kbase-probe/sparse_kbase_test.c && /tmp/sparse_kbase_test).
 * Plan and step meanings: worklogs/g615-dxvk/sparse-kbase-2026-10-27/README.md ("S0 PanProbe test"). */
#ifndef SPARSE_KBASE_H
#define SPARSE_KBASE_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* FIXED_VA zone of a 64-bit CSF context: [2^47 + 8 GiB, 2^48) (EXEC_VA 4 GiB + EXEC_FIXED_VA 4 GiB above 2^47). */
#define SK_FIXED_VA_BASE ((1ull << 47) + (8ull << 30))
#define SK_FIXED_R 0x800400000000ull /* the probe's FIXED window, 2 MiB aligned */
#define SK_FIXED_SPAN (32ull << 20)  /* what the probe uses above R: T10 at R, R+2M, tiles from R+4M */

/* What the probe proved on this kbase (1 = proven). */
struct sk_facts {
   int ctx;          /* /dev/mali0 handshake, SET_FLAGS and tracking page ok */
   int same_va;      /* T1: SAME_VA BO allocated and CPU write/readback ok */
   int arena_grow;   /* T9: SAME_VA GROW_ON_GPF commit grows and shrinks from the end (reported, not decisive) */
   int fixed_va;     /* T10a: MEM_ALLOC_EX FIXED landed at the requested VA */
   int fixed_rebind; /* T10c: free + re-alloc at the same FIXED VA */
   int tile_cycle;   /* T10d: growable FIXED 64 KiB tile commit 16->0->16 took effect (MEM_QUERY) */
};

enum sk_verdict { SK_LAZY_TILE, SK_EAGER_ONLY, SK_NOT_POSSIBLE };

/* lazy-tile possible: tiles (un)backed one by one at a stable FIXED VA (CSF design).
 * eager-only: a stable SAME_VA arena with every reserved tile backed up front (JM / v14 design).
 * not possible: no kbase context or no SAME_VA allocation at all. */
static inline enum sk_verdict
sk_verdict(const struct sk_facts *f)
{
   if (!f->ctx)
      return SK_NOT_POSSIBLE;
   if (f->fixed_va && f->fixed_rebind && f->tile_cycle)
      return SK_LAZY_TILE;
   return f->same_va ? SK_EAGER_ONLY : SK_NOT_POSSIBLE;
}

static inline const char *
sk_verdict_name(enum sk_verdict v)
{
   return v == SK_LAZY_TILE ? "lazy-tile possible" : v == SK_EAGER_ONLY ? "eager-only" : "not possible";
}

/* gpuprops blob lookup, encoding as the Mesa kbase backend: u32 header = key << 2 | log2(value size). */
static inline uint64_t
sk_prop_get(const uint8_t *buf, size_t len, uint32_t key, uint64_t dflt)
{
   size_t off = 0;
   while (off + 4 <= len) {
      uint32_t hdr;
      memcpy(&hdr, buf + off, 4);
      off += 4;
      uint32_t vsz = 1u << (hdr & 3);
      if (off + vsz > len)
         break;
      if (hdr >> 2 == key) {
         uint64_t v = 0;
         memcpy(&v, buf + off, vsz);
         return v;
      }
      off += vsz;
   }
   return dflt;
}

#endif
