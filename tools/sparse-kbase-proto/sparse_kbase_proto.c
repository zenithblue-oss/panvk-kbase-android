/*
 * sparse_kbase_proto.c - standalone probe for Mali kbase sparse-residency
 * building blocks (MEM_ALIAS + sink, FIXED-VA alloc, per-tile commit, rebind).
 *
 * This is a research instrument for the "sparse-kbase" design, NOT part of
 * PanVK.  It opens /dev/mali0, performs the same version-check / SET_FLAGS /
 * tracking-page / gpuprops / EXEC_INIT / JIT_INIT handshake the Mesa kbase
 * backend does, then exercises the candidate ioctls and prints PASS / FAIL /
 * SKIP / INFO per step.  Exit code == number of FAILs.
 *
 * It is deliberately self-contained: the minimal kbase UAPI is declared inline
 * (values cross-checked against mesa-115-rel include/drm-uapi/mali_kbase_ioctl.h
 * and the Pixel kbase r38p1 / r52p0 sources), so it builds with a bare NDK
 * clang and no Mesa headers.
 *
 * Build:   tools/sparse-kbase-proto/build.sh
 * Run:     adb push out/sparse_kbase_proto /data/local/tmp && run on device.
 *          --selftest runs the pure-host parser/VA self-checks only (no ioctls).
 *
 * DO NOT run this on a user device as part of an automated job: it pins pages,
 * provokes GPU page faults on purpose, and in the SAME_VA race test it maps
 * foreign memory.  Run it by hand, read the log.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

/* ---------------------------------------------------------------------------
 * Minimal kbase UAPI (subset).  _IOC encoding matches asm-generic (the ABI
 * kbase is compiled with on arm64 Android).
 * ------------------------------------------------------------------------- */
#define KB_IOC_NRBITS 8
#define KB_IOC_TYPEBITS 8
#define KB_IOC_SIZEBITS 14
#define KB_IOC_NRSHIFT 0
#define KB_IOC_TYPESHIFT (KB_IOC_NRSHIFT + KB_IOC_NRBITS)
#define KB_IOC_SIZESHIFT (KB_IOC_TYPESHIFT + KB_IOC_TYPEBITS)
#define KB_IOC_DIRSHIFT (KB_IOC_SIZESHIFT + KB_IOC_SIZEBITS)
#define KB_IOC_NONE 0U
#define KB_IOC_WRITE 1U
#define KB_IOC_READ 2U
#define KB_IOC(dir, type, nr, size)                                            \
   (((dir) << KB_IOC_DIRSHIFT) | ((type) << KB_IOC_TYPESHIFT) |                \
    ((nr) << KB_IOC_NRSHIFT) | ((size) << KB_IOC_SIZESHIFT))
#define KB_IOW(type, nr, t) KB_IOC(KB_IOC_WRITE, (type), (nr), sizeof(t))
#define KB_IOWR(type, nr, t) KB_IOC(KB_IOC_READ | KB_IOC_WRITE, (type), (nr), sizeof(t))

#define KBASE_IOCTL_TYPE 0x80

struct kb_version_check {
   uint16_t major;
   uint16_t minor;
};
#define KBASE_IOCTL_VERSION_CHECK_JM KB_IOWR(KBASE_IOCTL_TYPE, 0, struct kb_version_check)
#define KBASE_IOCTL_VERSION_CHECK_CSF KB_IOWR(KBASE_IOCTL_TYPE, 52, struct kb_version_check)

struct kb_set_flags {
   uint32_t create_flags;
};
#define KBASE_IOCTL_SET_FLAGS KB_IOW(KBASE_IOCTL_TYPE, 1, struct kb_set_flags)

struct kb_get_gpuprops {
   uint64_t buffer;
   uint32_t size;
   uint32_t flags;
};
#define KBASE_IOCTL_GET_GPUPROPS KB_IOW(KBASE_IOCTL_TYPE, 3, struct kb_get_gpuprops)

union kb_mem_alloc {
   struct {
      uint64_t va_pages, commit_pages, extension, flags;
   } in;
   struct {
      uint64_t flags, gpu_va;
   } out;
};
#define KBASE_IOCTL_MEM_ALLOC KB_IOWR(KBASE_IOCTL_TYPE, 5, union kb_mem_alloc)

union kb_mem_alloc_ex {
   struct {
      uint64_t va_pages, commit_pages, extension, flags, fixed_address, extra[3];
   } in;
   struct {
      uint64_t flags, gpu_va;
   } out;
};
#define KBASE_IOCTL_MEM_ALLOC_EX KB_IOWR(KBASE_IOCTL_TYPE, 59, union kb_mem_alloc_ex)

union kb_mem_query {
   struct {
      uint64_t gpu_addr, query;
   } in;
   struct {
      uint64_t value;
   } out;
};
#define KBASE_IOCTL_MEM_QUERY KB_IOWR(KBASE_IOCTL_TYPE, 6, union kb_mem_query)
#define KBASE_MEM_QUERY_COMMIT_SIZE 1ull
#define KBASE_MEM_QUERY_VA_SIZE 2ull
#define KBASE_MEM_QUERY_FLAGS 3ull

struct kb_mem_free {
   uint64_t gpu_addr;
};
#define KBASE_IOCTL_MEM_FREE KB_IOW(KBASE_IOCTL_TYPE, 7, struct kb_mem_free)

struct kb_mem_jit_init {
   uint64_t va_pages;
   uint8_t max_allocations, trim_level, group_id, padding[5];
   uint64_t phys_pages;
};
#define KBASE_IOCTL_MEM_JIT_INIT KB_IOW(KBASE_IOCTL_TYPE, 14, struct kb_mem_jit_init)

struct kb_mem_commit {
   uint64_t gpu_addr, pages;
};
#define KBASE_IOCTL_MEM_COMMIT KB_IOW(KBASE_IOCTL_TYPE, 20, struct kb_mem_commit)

struct kb_aliasing_info {
   uint64_t handle, offset, length;
};
union kb_mem_alias {
   struct {
      uint64_t flags, stride, nents, aliasing_info;
   } in;
   struct {
      uint64_t flags, gpu_va, va_pages;
   } out;
};
#define KBASE_IOCTL_MEM_ALIAS KB_IOWR(KBASE_IOCTL_TYPE, 21, union kb_mem_alias)

struct kb_mem_flags_change {
   uint64_t gpu_va, flags, mask;
};
#define KBASE_IOCTL_MEM_FLAGS_CHANGE KB_IOW(KBASE_IOCTL_TYPE, 23, struct kb_mem_flags_change)

struct kb_mem_exec_init {
   uint64_t va_pages;
};
#define KBASE_IOCTL_MEM_EXEC_INIT KB_IOW(KBASE_IOCTL_TYPE, 38, struct kb_mem_exec_init)

/* base_mem_alloc_flags */
#define BASE_MEM_PROT_CPU_RD (1ull << 0)
#define BASE_MEM_PROT_CPU_WR (1ull << 1)
#define BASE_MEM_PROT_GPU_RD (1ull << 2)
#define BASE_MEM_PROT_GPU_WR (1ull << 3)
#define BASE_MEM_PROT_GPU_EX (1ull << 4)
#define BASE_MEM_GROW_ON_GPF (1ull << 9)
#define BASE_MEM_COHERENT_SYSTEM (1ull << 10)
#define BASE_MEM_COHERENT_LOCAL (1ull << 11)
#define BASE_MEM_CACHED_CPU (1ull << 12)
#define BASE_MEM_SAME_VA (1ull << 13)
#define BASE_MEM_NEED_MMAP (1ull << 14)
#define BASE_MEM_DONT_NEED (1ull << 17)
#define BASE_MEM_FIXED (1ull << 8)   /* CSF */
#define BASE_MEM_FIXABLE (1ull << 29) /* CSF */

/* special mmap "handles" (4K-page based, see backend KBASE_MMAP_HANDLE) */
#define BASE_MEM_MAP_TRACKING_HANDLE (3ull << 12)
#define BASEP_MEM_WRITE_ALLOC_PAGES_HANDLE (4ull << 12)
#define BASE_MEM_COOKIE_BASE (64ull << 12)

/* gpuprops keys */
#define GPUPROP_PRODUCT_ID 1
#define GPUPROP_RAW_MMU_FEATURES 32
#define GPUPROP_RAW_GPU_ID 55

/* ---------------------------------------------------------------------------
 * harness state + logging
 * ------------------------------------------------------------------------- */
static int g_pass, g_fail, g_skip;
static sigjmp_buf g_jmp;
static volatile sig_atomic_t g_in_guard;

static void P(const char *fmt, ...)
{
   va_list ap;
   printf("PASS  ");
   va_start(ap, fmt);
   vprintf(fmt, ap);
   va_end(ap);
   printf("\n");
   g_pass++;
}
static void F(const char *fmt, ...)
{
   va_list ap;
   printf("FAIL  ");
   va_start(ap, fmt);
   vprintf(fmt, ap);
   va_end(ap);
   printf("\n");
   g_fail++;
}
static void S(const char *fmt, ...)
{
   va_list ap;
   printf("SKIP  ");
   va_start(ap, fmt);
   vprintf(fmt, ap);
   va_end(ap);
   printf("\n");
   g_skip++;
}
static void I(const char *fmt, ...)
{
   va_list ap;
   printf("INFO  ");
   va_start(ap, fmt);
   vprintf(fmt, ap);
   va_end(ap);
   printf("\n");
}

static void on_fault(int sig)
{
   if (g_in_guard)
      siglongjmp(g_jmp, sig);
   _exit(128 + sig);
}

static uint64_t now_ns(void)
{
   struct timespec ts;
   clock_gettime(CLOCK_MONOTONIC, &ts);
   return (uint64_t)ts.tv_sec * 1000000000ull + ts.tv_nsec;
}

/* gpuprops blob walker, same encoding as the backend: hdr=(key<<2)|size_code */
static uint64_t prop_get(const uint8_t *buf, size_t len, uint32_t key, uint64_t dflt)
{
   size_t off = 0;
   while (off + 4 <= len) {
      uint32_t hdr;
      memcpy(&hdr, buf + off, 4);
      off += 4;
      uint32_t k = hdr >> 2, vsz = 1u << (hdr & 3);
      if (off + vsz > len)
         break;
      if (k == key) {
         uint64_t v = 0;
         memcpy(&v, buf + off, vsz);
         return v;
      }
      off += vsz;
   }
   return dflt;
}

#define PAGE 4096ull
#define KBASE_MMAP_OFF(h) (((h) >> 12) * PAGE)

/* ---------------------------------------------------------------------------
 * device context
 * ------------------------------------------------------------------------- */
struct ctx {
   int fd;
   bool is_csf;
   uint16_t ver_major, ver_minor;
   uint64_t gpu_id;
   unsigned arch;
   unsigned va_bits;
};

static bool ver_at_least(const struct ctx *c, uint16_t maj, uint16_t min)
{
   return c->ver_major > maj || (c->ver_major == maj && c->ver_minor >= min);
}

/* native SAME_VA heap BO; returns gpu/cpu VA (equal) via *va, cookie via *handle */
static void *heap_bo(struct ctx *c, uint64_t pages, uint64_t extra_flags,
                     uint64_t *out_va)
{
   union kb_mem_alloc req = {.in = {.va_pages = pages,
                                    .commit_pages = pages,
                                    .extension = 0,
                                    .flags = BASE_MEM_PROT_CPU_RD | BASE_MEM_PROT_CPU_WR |
                                             BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_GPU_WR |
                                             BASE_MEM_SAME_VA | BASE_MEM_NEED_MMAP |
                                             BASE_MEM_CACHED_CPU | extra_flags}};
   if (ioctl(c->fd, KBASE_IOCTL_MEM_ALLOC, &req))
      return NULL;
   void *p = mmap(NULL, pages * PAGE, PROT_READ | PROT_WRITE, MAP_SHARED, c->fd,
                  req.out.gpu_va);
   if (p == MAP_FAILED)
      return NULL;
   if (out_va)
      *out_va = (uint64_t)(uintptr_t)p;
   return p;
}

/* ---------------------------------------------------------------------------
 * setup: mirrors kbase_kmod_dev_create
 * ------------------------------------------------------------------------- */
static bool setup(struct ctx *c, const char *path)
{
   c->fd = open(path, O_RDWR | O_CLOEXEC);
   if (c->fd < 0) {
      F("open(%s): %s", path, strerror(errno));
      return false;
   }

   struct kb_version_check ver = {0};
   if (ioctl(c->fd, KBASE_IOCTL_VERSION_CHECK_CSF, &ver) == 0) {
      c->is_csf = true;
   } else if (ioctl(c->fd, KBASE_IOCTL_VERSION_CHECK_JM, &ver) == 0) {
      c->is_csf = false;
      if (ver.major < 11) {
         F("legacy JM %u.%u (<11) unsupported", ver.major, ver.minor);
         return false;
      }
   } else {
      F("version handshake rejected: %s", strerror(errno));
      return false;
   }
   c->ver_major = ver.major;
   c->ver_minor = ver.minor;
   P("version check: %s uAPI %u.%u", c->is_csf ? "CSF" : "JM", ver.major, ver.minor);

   struct kb_set_flags sf = {.create_flags = 0};
   if (ioctl(c->fd, KBASE_IOCTL_SET_FLAGS, &sf)) {
      F("SET_FLAGS: %s", strerror(errno));
      return false;
   }

   void *track = mmap(NULL, PAGE, PROT_NONE, MAP_SHARED, c->fd,
                      KBASE_MMAP_OFF(BASE_MEM_MAP_TRACKING_HANDLE));
   if (track == MAP_FAILED) {
      F("mmap tracking page: %s", strerror(errno));
      return false;
   }

   /* gpuprops: probe size, then fill */
   struct kb_get_gpuprops gp = {0};
   int sz = ioctl(c->fd, KBASE_IOCTL_GET_GPUPROPS, &gp);
   if (sz <= 0) {
      F("GET_GPUPROPS probe: %s", strerror(errno));
      return false;
   }
   uint8_t *props = calloc(1, (size_t)sz);
   gp.buffer = (uint64_t)(uintptr_t)props;
   gp.size = (uint32_t)sz;
   if (ioctl(c->fd, KBASE_IOCTL_GET_GPUPROPS, &gp) < 0) {
      F("GET_GPUPROPS fill: %s", strerror(errno));
      return false;
   }
   c->gpu_id = prop_get(props, sz, GPUPROP_RAW_GPU_ID, 0);
   if (!c->gpu_id)
      c->gpu_id = prop_get(props, sz, GPUPROP_PRODUCT_ID, 0) << 16;
   uint64_t mmu = prop_get(props, sz, GPUPROP_RAW_MMU_FEATURES, 0);
   c->va_bits = mmu & 0xff;
   c->arch = (unsigned)((c->gpu_id >> 28) & 0xf);
   free(props);
   I("gpu_id=0x%llx arch(product major)=v%u va_bits=%u",
     (unsigned long long)c->gpu_id, c->arch, c->va_bits);

   struct kb_mem_exec_init ei = {.va_pages = (4ull << 30) / PAGE};
   if (ioctl(c->fd, KBASE_IOCTL_MEM_EXEC_INIT, &ei))
      I("EXEC_INIT: %s (zone exists or unsupported)", strerror(errno));

   if (!(!c->is_csf && c->arch < 9)) {
      struct kb_mem_jit_init ji = {.va_pages = (128ull << 30) / PAGE,
                                   .max_allocations = 255,
                                   .phys_pages = (128ull << 30) / PAGE};
      if (ioctl(c->fd, KBASE_IOCTL_MEM_JIT_INIT, &ji))
         I("JIT_INIT: %s (non-fatal here)", strerror(errno));
   } else {
      I("JIT_INIT skipped (JM arch<9)");
   }
   return true;
}

/* ---------------------------------------------------------------------------
 * T1: two native heap BOs with distinct patterns
 * ------------------------------------------------------------------------- */
static uint32_t *g_h0, *g_h1;
static uint64_t g_h0_va, g_h1_va;
#define HEAP_PAGES 16ull

static void t1_heaps(struct ctx *c)
{
   g_h0 = heap_bo(c, HEAP_PAGES, 0, &g_h0_va);
   g_h1 = heap_bo(c, HEAP_PAGES, 0, &g_h1_va);
   if (!g_h0 || !g_h1) {
      F("T1 heap BOs: %s", strerror(errno));
      return;
   }
   for (uint64_t i = 0; i < HEAP_PAGES * PAGE / 4; i++) {
      g_h0[i] = 0xA5A50000u | (uint32_t)(i & 0xffff);
      g_h1[i] = 0x5A5A0000u | (uint32_t)(i & 0xffff);
   }
   P("T1 native SAME_VA heaps H0=0x%llx H1=0x%llx (%llu pages each)",
     (unsigned long long)g_h0_va, (unsigned long long)g_h1_va, HEAP_PAGES);
}

/* ---------------------------------------------------------------------------
 * T2-T6: MEM_ALIAS with the sink page.  stride=HEAP_PAGES, nents=4:
 *   entry 0 = H0, entry 1 = sink, entry 2 = H1, entry 3 = sink.
 * Verify: alias created, entry0 readable through whole-mapping mmap,
 *   entry2 readable through a per-entry dumping mmap, sink mmap refused,
 *   MEM_COMMIT on an alias source refused.
 * ------------------------------------------------------------------------- */
static void t2_alias_sink(struct ctx *c)
{
   if (!g_h0 || !g_h1) {
      S("T2 alias: no heaps");
      return;
   }
   struct kb_aliasing_info ai[4] = {
      {.handle = g_h0_va, .offset = 0, .length = HEAP_PAGES},
      {.handle = BASEP_MEM_WRITE_ALLOC_PAGES_HANDLE, .offset = 0, .length = HEAP_PAGES},
      {.handle = g_h1_va, .offset = 0, .length = HEAP_PAGES},
      {.handle = BASEP_MEM_WRITE_ALLOC_PAGES_HANDLE, .offset = 0, .length = HEAP_PAGES},
   };
   union kb_mem_alias req = {.in = {.flags = BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_GPU_WR |
                                             BASE_MEM_PROT_CPU_RD,
                                    .stride = HEAP_PAGES,
                                    .nents = 4,
                                    .aliasing_info = (uint64_t)(uintptr_t)ai}};
   if (ioctl(c->fd, KBASE_IOCTL_MEM_ALIAS, &req)) {
      F("T2 MEM_ALIAS [H0,sink,H1,sink]: %s", strerror(errno));
      return;
   }
   uint64_t cookie = req.out.gpu_va;
   uint64_t total = 4 * HEAP_PAGES * PAGE;
   P("T2 MEM_ALIAS created, va_pages=%llu cookie=0x%llx",
     (unsigned long long)req.out.va_pages, (unsigned long long)cookie);

   /* whole-alias mmap: only entry 0 faults in; reading entry 0 returns H0 */
   uint32_t *whole = mmap(NULL, total, PROT_READ, MAP_SHARED, c->fd, cookie);
   if (whole == MAP_FAILED) {
      F("T3 mmap whole alias: %s", strerror(errno));
   } else {
      g_in_guard = 1;
      if (sigsetjmp(g_jmp, 1) == 0) {
         if (whole[0] == g_h0[0])
            P("T3 whole-alias entry0 reads H0 (0x%08x)", whole[0]);
         else
            F("T3 whole-alias entry0 = 0x%08x, expected 0x%08x", whole[0], g_h0[0]);
      } else {
         F("T3 whole-alias entry0 SIGBUS/SEGV");
      }
      g_in_guard = 0;
      munmap(whole, total);
   }

   /* per-entry dumping mmap at offset of entry 2 -> H1 */
   uint32_t *e2 = mmap(NULL, HEAP_PAGES * PAGE, PROT_READ, MAP_SHARED, c->fd,
                       cookie + 2 * HEAP_PAGES * PAGE);
   if (e2 == MAP_FAILED) {
      F("T4 dumping mmap entry2: %s", strerror(errno));
   } else {
      g_in_guard = 1;
      if (sigsetjmp(g_jmp, 1) == 0) {
         if (e2[0] == g_h1[0])
            P("T4 dumping mmap entry2 reads H1 (0x%08x)", e2[0]);
         else
            F("T4 dumping mmap entry2 = 0x%08x, expected 0x%08x", e2[0], g_h1[0]);
      } else {
         F("T4 dumping mmap entry2 SIGBUS/SEGV");
      }
      g_in_guard = 0;
      munmap(e2, HEAP_PAGES * PAGE);
   }

   /* sink entry dumping mmap must be refused (aliased->alloc == NULL) */
   uint32_t *es = mmap(NULL, HEAP_PAGES * PAGE, PROT_READ, MAP_SHARED, c->fd,
                       cookie + 1 * HEAP_PAGES * PAGE);
   if (es == MAP_FAILED)
      P("T5 sink-entry dumping mmap refused as expected (%s)", strerror(errno));
   else {
      F("T5 sink-entry dumping mmap unexpectedly succeeded");
      munmap(es, HEAP_PAGES * PAGE);
   }

   /* MEM_COMMIT on an alias source must be refused: it is still native, but the
    * alias pinned it via gpu_mapped so gpu_mappings>1 => shrink/grow refused. */
   struct kb_mem_commit cm = {.gpu_addr = g_h0_va, .pages = HEAP_PAGES + 1};
   if (ioctl(c->fd, KBASE_IOCTL_MEM_COMMIT, &cm))
      P("T6 MEM_COMMIT on aliased source refused as expected (%s)", strerror(errno));
   else
      F("T6 MEM_COMMIT on aliased source unexpectedly succeeded");

   struct kb_mem_free fr = {.gpu_addr = cookie};
   ioctl(c->fd, KBASE_IOCTL_MEM_FREE, &fr);
}

/* ---------------------------------------------------------------------------
 * T7: an alias requested with the FIXED flag lands in SAME_VA (FIXED is masked
 * off by the kernel), proving aliases cannot keep a stable fixed VA.
 * ------------------------------------------------------------------------- */
static void t7_alias_fixed_masked(struct ctx *c)
{
   if (!c->is_csf) {
      S("T7 alias+FIXED: CSF only");
      return;
   }
   if (!g_h0) {
      S("T7 alias+FIXED: no heap");
      return;
   }
   struct kb_aliasing_info ai = {.handle = g_h0_va, .offset = 0, .length = HEAP_PAGES};
   union kb_mem_alias req = {.in = {.flags = BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_GPU_WR |
                                             BASE_MEM_FIXED,
                                    .stride = HEAP_PAGES,
                                    .nents = 1,
                                    .aliasing_info = (uint64_t)(uintptr_t)&ai}};
   if (ioctl(c->fd, KBASE_IOCTL_MEM_ALIAS, &req)) {
      I("T7 alias+FIXED rejected outright (%s) - also proves no fixed alias",
        strerror(errno));
      P("T7 alias cannot be FIXED");
      return;
   }
   if (req.out.flags & BASE_MEM_FIXED)
      F("T7 alias kept BASE_MEM_FIXED (unexpected)");
   else
      P("T7 alias dropped BASE_MEM_FIXED -> SAME_VA cookie 0x%llx (as kernel masks it)",
        (unsigned long long)req.out.gpu_va);
   struct kb_mem_free fr = {.gpu_addr = req.out.gpu_va};
   ioctl(c->fd, KBASE_IOCTL_MEM_FREE, &fr);
}

/* ---------------------------------------------------------------------------
 * T8: SAME_VA re-placement reuse rate.  Allocate+mmap+free N times, see how
 * often the kernel's top-down allocator hands back the same VA.  This is the
 * "design (d1)" risk: if reuse is not 100%, a sparse scheme built on munmap+
 * recreate loses the VA.
 * ------------------------------------------------------------------------- */
static void t8_samevacovers(struct ctx *c)
{
   enum { N = 64 };
   uint64_t first = 0;
   unsigned same = 0, ok = 0;
   for (int i = 0; i < N; i++) {
      uint64_t va = 0;
      void *p = heap_bo(c, HEAP_PAGES, 0, &va);
      if (!p)
         continue;
      ok++;
      if (i == 0)
         first = va;
      else if (va == first)
         same++;
      munmap(p, HEAP_PAGES * PAGE);
   }
   I("T8 SAME_VA re-placement: %u/%d allocs returned the first VA 0x%llx",
     same, N - 1, (unsigned long long)first);
   if (ok == N)
      P("T8 SAME_VA alloc/free cycle stable (%u/%u ok, reuse=%u)", ok, N, same);
   else
      F("T8 SAME_VA alloc/free cycle: only %u/%u ok", ok, N);
}

/* ---------------------------------------------------------------------------
 * T9: SAME_VA GROW_ON_GPF commit grow/shrink from the end.
 * ------------------------------------------------------------------------- */
static void t9_grow_commit(struct ctx *c)
{
   uint64_t va = 0;
   union kb_mem_alloc req = {.in = {.va_pages = 256,
                                    .commit_pages = 16,
                                    .extension = 16,
                                    .flags = BASE_MEM_PROT_CPU_RD | BASE_MEM_PROT_CPU_WR |
                                             BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_GPU_WR |
                                             BASE_MEM_SAME_VA | BASE_MEM_NEED_MMAP |
                                             BASE_MEM_CACHED_CPU | BASE_MEM_GROW_ON_GPF}};
   if (ioctl(c->fd, KBASE_IOCTL_MEM_ALLOC, &req)) {
      F("T9 GROW_ON_GPF alloc: %s", strerror(errno));
      return;
   }
   va = req.out.gpu_va;
   union kb_mem_query q = {.in = {.gpu_addr = va, .query = KBASE_MEM_QUERY_COMMIT_SIZE}};
   ioctl(c->fd, KBASE_IOCTL_MEM_QUERY, &q);
   uint64_t c0 = q.out.value;

   struct kb_mem_commit cm = {.gpu_addr = va, .pages = 64};
   bool grow = ioctl(c->fd, KBASE_IOCTL_MEM_COMMIT, &cm) == 0;
   q.in.query = KBASE_MEM_QUERY_COMMIT_SIZE;
   ioctl(c->fd, KBASE_IOCTL_MEM_QUERY, &q);
   uint64_t c1 = q.out.value;

   cm.pages = 16;
   bool shrink = ioctl(c->fd, KBASE_IOCTL_MEM_COMMIT, &cm) == 0;
   ioctl(c->fd, KBASE_IOCTL_MEM_QUERY, &q);
   uint64_t c2 = q.out.value;

   if (grow && shrink && c1 > c0 && c2 < c1)
      P("T9 GROW_ON_GPF commit grow/shrink from end: %llu->%llu->%llu pages",
        (unsigned long long)c0, (unsigned long long)c1, (unsigned long long)c2);
   else
      F("T9 GROW_ON_GPF commit: grow=%d shrink=%d sizes %llu/%llu/%llu", grow, shrink,
        (unsigned long long)c0, (unsigned long long)c1, (unsigned long long)c2);

   struct kb_mem_free fr = {.gpu_addr = va};
   ioctl(c->fd, KBASE_IOCTL_MEM_FREE, &fr);
}

/* ---------------------------------------------------------------------------
 * T10 (CSF >= 1.9, va_bits >= 48): FIXED-VA allocation at a driver-chosen VA
 * in the FIXED_VA zone, plus per-tile growable commit and rebind-at-same-VA.
 *   FIXED_VA zone base (64-bit) = EXEC_VA(2^47) + EXEC_FIXED_VA(4G) = 0x800200000000.
 *   We use R = 0x800400000000 (base + 8G), 2MiB-aligned, well inside [.., 2^48).
 * NB: FIXED allocations are CPU-mappable via their real gpu_va as the mmap
 * offset, so we can read back through the CPU.
 * ------------------------------------------------------------------------- */
#define FIXED_R 0x800400000000ull

static uint32_t *map_fixed(struct ctx *c, uint64_t va, uint64_t bytes, int prot)
{
   void *p = mmap(NULL, bytes, prot, MAP_SHARED, c->fd, va);
   return p == MAP_FAILED ? NULL : p;
}

static uint64_t alloc_ex_fixed(struct ctx *c, uint64_t va, uint64_t va_pages,
                               uint64_t commit, uint64_t ext, uint64_t extra_flags)
{
   union kb_mem_alloc_ex req = {.in = {.va_pages = va_pages,
                                       .commit_pages = commit,
                                       .extension = ext,
                                       .flags = BASE_MEM_PROT_CPU_RD | BASE_MEM_PROT_CPU_WR |
                                                BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_GPU_WR |
                                                BASE_MEM_CACHED_CPU | BASE_MEM_FIXED | extra_flags,
                                       .fixed_address = va}};
   if (ioctl(c->fd, KBASE_IOCTL_MEM_ALLOC_EX, &req))
      return 0;
   return req.out.gpu_va;
}

static void t10_fixed_va(struct ctx *c)
{
   if (!c->is_csf || !ver_at_least(c, 1, 9)) {
      S("T10 FIXED-VA: needs CSF uAPI >= 1.9 (have %s %u.%u)",
        c->is_csf ? "CSF" : "JM", c->ver_major, c->ver_minor);
      return;
   }
   if (c->va_bits && c->va_bits < 48) {
      S("T10 FIXED-VA: va_bits=%u < 48, FIXED_VA zone unreachable", c->va_bits);
      return;
   }

   /* (a) plain FIXED alloc at R with RW readback */
   uint64_t va = alloc_ex_fixed(c, FIXED_R, 16, 16, 0, 0);
   if (!va) {
      if (errno == ENOTTY || errno == EINVAL) {
         S("T10 ALLOC_EX FIXED unsupported (%s) - no FIXED-VA on this kernel",
           strerror(errno));
         return;
      }
      F("T10 ALLOC_EX FIXED at R: %s", strerror(errno));
      return;
   }
   if (va != FIXED_R)
      I("T10 FIXED alloc returned 0x%llx (asked 0x%llx)", (unsigned long long)va,
        (unsigned long long)FIXED_R);
   uint32_t *m = map_fixed(c, va, 16 * PAGE, PROT_READ | PROT_WRITE);
   if (m) {
      m[0] = 0xDEADBEEFu;
      m[1] = 0x1234u;
      if (m[0] == 0xDEADBEEFu && m[1] == 0x1234u)
         P("T10a FIXED alloc at 0x%llx, CPU RW readback ok", (unsigned long long)va);
      else
         F("T10a FIXED readback mismatch");
      munmap(m, 16 * PAGE);
   } else {
      F("T10a mmap FIXED region: %s", strerror(errno));
   }

   /* (b) overlapping FIXED alloc at the same VA must be refused */
   uint64_t dup = alloc_ex_fixed(c, va, 16, 16, 0, 0);
   if (!dup)
      P("T10b overlapping FIXED alloc refused as expected (%s)", strerror(errno));
   else {
      F("T10b overlapping FIXED alloc unexpectedly succeeded (0x%llx)",
        (unsigned long long)dup);
      struct kb_mem_free fr = {.gpu_addr = dup};
      ioctl(c->fd, KBASE_IOCTL_MEM_FREE, &fr);
   }

   /* (c) free and re-alloc at the same VA (rebind) */
   struct kb_mem_free fr = {.gpu_addr = va};
   ioctl(c->fd, KBASE_IOCTL_MEM_FREE, &fr);
   uint64_t va2 = alloc_ex_fixed(c, FIXED_R, 16, 16, 0, 0);
   if (va2 == FIXED_R) {
      P("T10c re-alloc at same FIXED VA 0x%llx after free (rebind works)",
        (unsigned long long)FIXED_R);
      fr.gpu_addr = va2;
      ioctl(c->fd, KBASE_IOCTL_MEM_FREE, &fr);
   } else {
      F("T10c re-alloc at same FIXED VA: got 0x%llx (%s)", (unsigned long long)va2,
        va2 ? "wrong VA" : strerror(errno));
      if (va2) {
         fr.gpu_addr = va2;
         ioctl(c->fd, KBASE_IOCTL_MEM_FREE, &fr);
      }
   }

   /* (d) per-tile growable FIXED region: commit 0->16->0->16, zero on recommit.
    * A 64KiB tile = 16 x 4KiB pages. extension=16 so GROW_ON_GPF rounds to it. */
   uint64_t tva = alloc_ex_fixed(c, FIXED_R + (2ull << 20), 16, 0, 16, BASE_MEM_GROW_ON_GPF);
   if (!tva) {
      I("T10d growable FIXED (commit=0) rejected (%s); retry commit=1", strerror(errno));
      tva = alloc_ex_fixed(c, FIXED_R + (2ull << 20), 16, 1, 16, BASE_MEM_GROW_ON_GPF);
   }
   if (!tva) {
      F("T10d growable FIXED tile alloc: %s", strerror(errno));
   } else {
      uint32_t *tm = map_fixed(c, tva, 16 * PAGE, PROT_READ | PROT_WRITE);
      bool ok = true;
      if (tm) {
         struct kb_mem_commit cm = {.gpu_addr = tva, .pages = 16};
         if (ioctl(c->fd, KBASE_IOCTL_MEM_COMMIT, &cm)) {
            I("T10d commit 16: %s", strerror(errno));
            ok = false;
         }
         tm[0] = 0xCAFEBABEu;
         cm.pages = 0;
         if (ioctl(c->fd, KBASE_IOCTL_MEM_COMMIT, &cm))
            I("T10d uncommit 0: %s", strerror(errno));
         cm.pages = 16;
         if (ioctl(c->fd, KBASE_IOCTL_MEM_COMMIT, &cm))
            I("T10d recommit 16: %s", strerror(errno));
         /* fresh pages must be zero (kbase pools are __GFP_ZERO) */
         if (ok && tm[0] == 0)
            P("T10d per-tile commit cycle 16->0->16 gives zeroed pages on recommit");
         else if (ok)
            I("T10d recommit returned 0x%08x (page reuse, not necessarily zeroed)", tm[0]);
         munmap(tm, 16 * PAGE);
      }
      struct kb_mem_free tf = {.gpu_addr = tva};
      ioctl(c->fd, KBASE_IOCTL_MEM_FREE, &tf);
   }

   /* (e) FIXABLE refused while a FIXED allocation is live */
   uint64_t keep = alloc_ex_fixed(c, FIXED_R, 16, 16, 0, 0);
   if (keep) {
      union kb_mem_alloc_ex fx = {.in = {.va_pages = 16,
                                         .commit_pages = 16,
                                         .flags = BASE_MEM_PROT_CPU_RD | BASE_MEM_PROT_GPU_RD |
                                                  BASE_MEM_PROT_GPU_WR | BASE_MEM_FIXABLE}};
      if (ioctl(c->fd, KBASE_IOCTL_MEM_ALLOC_EX, &fx))
         P("T10e FIXABLE refused while FIXED live, as expected (%s)", strerror(errno));
      else {
         I("T10e FIXABLE accepted alongside FIXED (vendor kernel differs)");
         struct kb_mem_free ff = {.gpu_addr = fx.out.gpu_va};
         ioctl(c->fd, KBASE_IOCTL_MEM_FREE, &ff);
      }
      struct kb_mem_free kf = {.gpu_addr = keep};
      ioctl(c->fd, KBASE_IOCTL_MEM_FREE, &kf);
   }

   /* (f) timing: alloc+commit+uncommit+free over many tiles */
   enum { NT = 1024 };
   uint64_t *tiles = calloc(NT, sizeof(*tiles));
   uint64_t t_alloc = now_ns();
   unsigned made = 0;
   for (int i = 0; i < NT; i++) {
      uint64_t a = alloc_ex_fixed(c, FIXED_R + ((uint64_t)(i + 4) << 20), 16, 16, 0, 0);
      if (!a)
         break;
      tiles[made++] = a;
   }
   uint64_t t_done = now_ns();
   if (made) {
      double us = (t_done - t_alloc) / 1000.0 / made;
      I("T10f FIXED alloc+map-less: %.2f us/tile over %u tiles", us, made);
   }
   uint64_t t_free = now_ns();
   for (unsigned i = 0; i < made; i++) {
      struct kb_mem_free f = {.gpu_addr = tiles[i]};
      ioctl(c->fd, KBASE_IOCTL_MEM_FREE, &f);
   }
   if (made)
      I("T10f FIXED free: %.2f us/tile", (now_ns() - t_free) / 1000.0 / made);
   free(tiles);
   P("T10 FIXED-VA suite complete");
}

/* ---------------------------------------------------------------------------
 * T11: alias-rebuild timing (design (b) cost): rebuild a 24-entry alias many
 * times and measure, to bound the "re-point via alias rebuild" approach.
 * ------------------------------------------------------------------------- */
static void t11_alias_rebuild_timing(struct ctx *c)
{
   if (!g_h0) {
      S("T11 alias rebuild: no heap");
      return;
   }
   enum { ENTS = 24, ITERS = 256 };
   struct kb_aliasing_info *ai = calloc(ENTS, sizeof(*ai));
   for (int i = 0; i < ENTS; i++) {
      ai[i].handle = g_h0_va;
      ai[i].length = HEAP_PAGES;
   }
   uint64_t t0 = now_ns();
   unsigned ok = 0;
   for (int it = 0; it < ITERS; it++) {
      union kb_mem_alias req = {.in = {.flags = BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_GPU_WR,
                                       .stride = HEAP_PAGES,
                                       .nents = ENTS,
                                       .aliasing_info = (uint64_t)(uintptr_t)ai}};
      if (ioctl(c->fd, KBASE_IOCTL_MEM_ALIAS, &req))
         break;
      ok++;
      struct kb_mem_free fr = {.gpu_addr = req.out.gpu_va};
      ioctl(c->fd, KBASE_IOCTL_MEM_FREE, &fr);
   }
   free(ai);
   if (ok)
      I("T11 alias rebuild (%d ents): %.2f us/rebuild over %u iters", ENTS,
        (now_ns() - t0) / 1000.0 / ok, ok);
   P("T11 alias rebuild timing done (%u/%d ok)", ok, ITERS);
}

/* ---------------------------------------------------------------------------
 * self-test: pure-host checks of the props parser and VA math (no device).
 * ------------------------------------------------------------------------- */
static int selftest(void)
{
   /* build a tiny blob: key RAW_GPU_ID (u64) and RAW_MMU_FEATURES (u32) */
   uint8_t buf[64];
   size_t o = 0;
   uint32_t hdr = (GPUPROP_RAW_GPU_ID << 2) | 3; /* u64 */
   memcpy(buf + o, &hdr, 4);
   o += 4;
   uint64_t gid = 0xa8670000ull;
   memcpy(buf + o, &gid, 8);
   o += 8;
   hdr = (GPUPROP_RAW_MMU_FEATURES << 2) | 2; /* u32 */
   memcpy(buf + o, &hdr, 4);
   o += 4;
   uint32_t mmu = 0x2830; /* va_bits = 0x30 = 48 */
   memcpy(buf + o, &mmu, 4);
   o += 4;

   int fails = 0;
   if (prop_get(buf, o, GPUPROP_RAW_GPU_ID, 0) != gid) {
      F("selftest prop_get gpu_id");
      fails++;
   } else
      P("selftest prop_get gpu_id");
   if ((prop_get(buf, o, GPUPROP_RAW_MMU_FEATURES, 0) & 0xff) != 48) {
      F("selftest va_bits parse");
      fails++;
   } else
      P("selftest va_bits=48 parse");
   /* VA zone math: FIXED_VA base must be 2^47 + 4GiB and R must sit above it */
   uint64_t fixed_base = (1ull << 47) + (4ull << 30);
   if (fixed_base != 0x800100000000ull) {
      F("selftest fixed_base math (0x%llx)", (unsigned long long)fixed_base);
      fails++;
   } else
      P("selftest EXEC_FIXED_VA end = 0x800100000000");
   if (FIXED_R <= fixed_base || FIXED_R >= (1ull << 48)) {
      F("selftest R out of FIXED_VA zone");
      fails++;
   } else
      P("selftest R=0x%llx inside FIXED_VA zone", (unsigned long long)FIXED_R);
   if (KBASE_MMAP_OFF(BASE_MEM_COOKIE_BASE) != 64 * PAGE) {
      F("selftest mmap-handle math");
      fails++;
   } else
      P("selftest cookie-base mmap offset");
   return fails;
}

int main(int argc, char **argv)
{
   const char *path = "/dev/mali0";
   bool self = false;
   for (int i = 1; i < argc; i++) {
      if (!strcmp(argv[i], "--selftest"))
         self = true;
      else if (!strcmp(argv[i], "--dev") && i + 1 < argc)
         path = argv[++i];
      else if (!strncmp(argv[i], "--dev=", 6))
         path = argv[i] + 6;
   }

   if (self) {
      int f = selftest();
      printf("\n--- selftest: %d pass, %d fail ---\n", g_pass, g_fail);
      return f;
   }

   signal(SIGBUS, on_fault);
   signal(SIGSEGV, on_fault);

   struct ctx c = {.fd = -1};
   if (!setup(&c, path)) {
      printf("\n--- setup failed: %d pass, %d fail, %d skip ---\n", g_pass, g_fail, g_skip);
      return g_fail ? g_fail : 1;
   }

   t1_heaps(&c);
   t2_alias_sink(&c);
   t7_alias_fixed_masked(&c);
   t8_samevacovers(&c);
   t9_grow_commit(&c);
   t10_fixed_va(&c);
   t11_alias_rebuild_timing(&c);

   if (g_h0)
      munmap(g_h0, HEAP_PAGES * PAGE);
   if (g_h1)
      munmap(g_h1, HEAP_PAGES * PAGE);
   if (c.fd >= 0)
      close(c.fd);

   printf("\n--- %d pass, %d fail, %d skip ---\n", g_pass, g_fail, g_skip);
   return g_fail;
}
