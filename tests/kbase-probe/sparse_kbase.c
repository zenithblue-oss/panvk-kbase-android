/* sparse_kbase - PanProbe S0 gate of real sparse residency on stock kbase.
 *
 * Plan, per-step expectations and how to read the log: worklogs/g615-dxvk/sparse-kbase-2026-10-27/README.md
 * ("S0 PanProbe test"). Port of tools/sparse-kbase-proto/sparse_kbase_proto.c T1-T11, made safe to run
 * unattended inside PanProbe:
 *  - own /dev/mali0 context (this process never loads the Vulkan driver) and CPU-side kbase ioctls only:
 *    no GPU job is submitted, so no GPU fault can be provoked;
 *  - bounded sizes (at most 16 MiB committed at once), everything allocated is freed (close() frees the rest);
 *  - every CPU access to kbase memory is SIGSEGV/SIGBUS guarded, every step runs under a SIGALRM watchdog;
 *  - an ioctl this kernel lacks (ENOTTY, ...) is a SKIP with the reason, not a FAIL.
 * Output, one line each:
 *   PASS|FAIL|SKIP <step> <what>       per check
 *   INFO <text>
 *   BENCH sparse_<op>=<us>us ...       latencies (PanProbe shows them in the result's extra)
 *   VERDICT v<arch> <lazy-tile possible|eager-only|not possible> -- <why>
 *   SUMMARY pass=N fail=N skip=N
 * Exits 0: informational, PanProbe reports it apart from the suite. RESULT SKIP when /dev/mali0 is unusable.
 * Usage: sparse_kbase [driver.so, ignored]
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
#include "sparse_kbase.h"

/* Minimal kbase UAPI subset, as tools/sparse-kbase-proto (cross-checked against Mesa's mali_kbase_ioctl.h). */
#define KB_TYPE 0x80
struct kb_version_check {
   uint16_t major, minor;
};
struct kb_set_flags {
   uint32_t create_flags;
};
struct kb_get_gpuprops {
   uint64_t buffer;
   uint32_t size, flags;
};
union kb_mem_alloc {
   struct {
      uint64_t va_pages, commit_pages, extension, flags;
   } in;
   struct {
      uint64_t flags, gpu_va;
   } out;
};
union kb_mem_alloc_ex {
   struct {
      uint64_t va_pages, commit_pages, extension, flags, fixed_address, extra[3];
   } in;
   struct {
      uint64_t flags, gpu_va;
   } out;
};
union kb_mem_query {
   struct {
      uint64_t gpu_addr, query;
   } in;
   struct {
      uint64_t value;
   } out;
};
struct kb_mem_free {
   uint64_t gpu_addr;
};
struct kb_mem_commit {
   uint64_t gpu_addr, pages;
};
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
#define KBASE_IOCTL_VERSION_CHECK_JM _IOWR(KB_TYPE, 0, struct kb_version_check)
#define KBASE_IOCTL_VERSION_CHECK_CSF _IOWR(KB_TYPE, 52, struct kb_version_check)
#define KBASE_IOCTL_SET_FLAGS _IOW(KB_TYPE, 1, struct kb_set_flags)
#define KBASE_IOCTL_GET_GPUPROPS _IOW(KB_TYPE, 3, struct kb_get_gpuprops)
#define KBASE_IOCTL_MEM_ALLOC _IOWR(KB_TYPE, 5, union kb_mem_alloc)
#define KBASE_IOCTL_MEM_QUERY _IOWR(KB_TYPE, 6, union kb_mem_query)
#define KBASE_IOCTL_MEM_FREE _IOW(KB_TYPE, 7, struct kb_mem_free)
#define KBASE_IOCTL_MEM_COMMIT _IOW(KB_TYPE, 20, struct kb_mem_commit)
#define KBASE_IOCTL_MEM_ALIAS _IOWR(KB_TYPE, 21, union kb_mem_alias)
#define KBASE_IOCTL_MEM_ALLOC_EX _IOWR(KB_TYPE, 59, union kb_mem_alloc_ex)
#define KBASE_MEM_QUERY_COMMIT_SIZE 1ull

#define BASE_MEM_PROT_CPU_RD (1ull << 0)
#define BASE_MEM_PROT_CPU_WR (1ull << 1)
#define BASE_MEM_PROT_GPU_RD (1ull << 2)
#define BASE_MEM_PROT_GPU_WR (1ull << 3)
#define BASE_MEM_FIXED (1ull << 8) /* CSF */
#define BASE_MEM_GROW_ON_GPF (1ull << 9)
#define BASE_MEM_COHERENT_LOCAL (1ull << 11)
#define BASE_MEM_SAME_VA (1ull << 13)
#define BASE_MEM_FIXABLE (1ull << 29) /* CSF */
#define BASE_MEM_MAP_TRACKING_HANDLE (3ull << 12)
#define BASEP_MEM_WRITE_ALLOC_PAGES_HANDLE (4ull << 12) /* the per-context sink page */
#define GPUPROP_PRODUCT_ID 1
#define GPUPROP_RAW_MMU_FEATURES 32
#define GPUPROP_RAW_GPU_ID 55

#define PAGE 4096ull
#define TILE_PAGES 16ull /* one 64 KiB sparse tile */
#define TILE (TILE_PAGES * PAGE)
#define H1_PAGES (2 * TILE_PAGES)
/* The Mesa kbase backend's BO flags. No CACHED_CPU: CPU views are write-combined, so an alias of a BO reads
 * what the BO's own mapping wrote. NEED_MMAP is an output-only flag (as input: EINVAL). */
#define BO_FLAGS                                                                                                 \
   (BASE_MEM_PROT_CPU_RD | BASE_MEM_PROT_CPU_WR | BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_GPU_WR |               \
    BASE_MEM_COHERENT_LOCAL)
#define STEP_SECONDS 6 /* watchdog per step; the loops stop on their own deadlines well before */

typedef unsigned long long ull;

static int g_pass, g_fail, g_skip;
static struct sk_facts g_facts;
static const char *g_no_lazy = "FIXED-VA tiles not tested"; /* why lazy-tile is not proven, for VERDICT */
static char g_no_ctx[160] = "setup did not finish";
static unsigned g_tile_floor; /* commit of an unbound tile: 0, or 1 if this kernel refuses commit 0 */
static uint32_t *g_h0, *g_h1; /* T1: H0 plain BO; H1 growable BO, so T6 can test MEM_COMMIT on an alias source */

static void
out(const char *tag, int *count, const char *fmt, va_list ap)
{
   printf("%s ", tag);
   vprintf(fmt, ap);
   printf("\n");
   if (count)
      (*count)++;
}
#define LOGFN(name, tag, counter)                                                                                \
   __attribute__((format(printf, 1, 2))) static void name(const char *fmt, ...)                                \
   {                                                                                                             \
      va_list ap;                                                                                                \
      va_start(ap, fmt);                                                                                         \
      out(tag, counter, fmt, ap);                                                                                \
      va_end(ap);                                                                                                \
   }
LOGFN(P, "PASS", &g_pass)
LOGFN(F, "FAIL", &g_fail)
LOGFN(S, "SKIP", &g_skip)
LOGFN(I, "INFO", NULL)

/* ---- guards: SIGSEGV/SIGBUS around CPU accesses, SIGALRM watchdog per step ---- */
static sigjmp_buf g_step_jmp, g_guard_jmp;
static volatile sig_atomic_t g_in_step, g_in_guard, g_sig;
static bool g_aborted;

static void
on_signal(int sig)
{
   g_sig = sig;
   if (sig != SIGALRM && g_in_guard)
      siglongjmp(g_guard_jmp, 1);
   if (g_in_step)
      siglongjmp(g_step_jmp, 1);
   _exit(128 + sig);
}

/* CPU accesses to kbase memory that may SIGBUS (unbacked page): false if it faulted. */
static bool
peek(const volatile uint32_t *p, uint32_t *v)
{
   volatile bool ok = false;
   g_in_guard = 1;
   if (!sigsetjmp(g_guard_jmp, 1)) {
      *v = *p;
      ok = true;
   }
   g_in_guard = 0;
   return ok;
}

static bool
poke(volatile uint32_t *p, uint32_t v)
{
   volatile bool ok = false;
   g_in_guard = 1;
   if (!sigsetjmp(g_guard_jmp, 1)) {
      *p = v;
      ok = true;
   }
   g_in_guard = 0;
   return ok;
}

/* Writes (or checks) p[i] = seed + i over n words; false on a fault or a mismatch. */
static bool
pattern(volatile uint32_t *p, size_t n, uint32_t seed, bool write)
{
   volatile bool ok = false;
   g_in_guard = 1;
   if (!sigsetjmp(g_guard_jmp, 1)) {
      size_t i;
      for (i = 0; i < n; i++) {
         if (write)
            p[i] = seed + (uint32_t)i;
         else if (p[i] != seed + (uint32_t)i)
            break;
      }
      ok = i == n;
   }
   g_in_guard = 0;
   return ok;
}

/* Write-combined CPU stores reach memory before another mapping of the same pages reads them. */
static void
wc_flush(void)
{
#if defined(__aarch64__)
   __asm__ __volatile__("dsb sy" ::: "memory");
#else
   __sync_synchronize();
#endif
}

static uint64_t
now_ns(void)
{
   struct timespec ts;
   clock_gettime(CLOCK_MONOTONIC, &ts);
   return (uint64_t)ts.tv_sec * 1000000000ull + ts.tv_nsec;
}

static double
us_per(uint64_t ns, unsigned n)
{
   return n ? ns / 1000.0 / n : 0.0;
}

/* ---- kbase helpers ---- */
struct ctx {
   int fd;
   bool csf;
   unsigned major, minor, arch, va_bits;
   uint32_t gpu_id;
   void *track;
};

static bool
unavailable(int e)
{
   return e == ENOTTY || e == ENOSYS || e == EOPNOTSUPP;
}

/* An ioctl failed: SKIP when this kernel lacks it, FAIL otherwise. */
static void
ioctl_fail(const char *what)
{
   if (unavailable(errno))
      S("%s: ioctl unavailable on this kernel (%s)", what, strerror(errno));
   else
      F("%s: %s", what, strerror(errno));
}

static void
mem_free(struct ctx *c, uint64_t va)
{
   struct kb_mem_free fr = {.gpu_addr = va};
   ioctl(c->fd, KBASE_IOCTL_MEM_FREE, &fr);
}

static bool
commit(struct ctx *c, uint64_t va, uint64_t pages)
{
   struct kb_mem_commit cm = {.gpu_addr = va, .pages = pages};
   return ioctl(c->fd, KBASE_IOCTL_MEM_COMMIT, &cm) == 0;
}

/* Backed pages of the region at va, UINT64_MAX on error. */
static uint64_t
committed(struct ctx *c, uint64_t va)
{
   union kb_mem_query q = {.in = {.gpu_addr = va, .query = KBASE_MEM_QUERY_COMMIT_SIZE}};
   return ioctl(c->fd, KBASE_IOCTL_MEM_QUERY, &q) ? UINT64_MAX : q.out.value;
}

/* SAME_VA BO: mmap of its cookie fixes CPU VA == GPU VA, munmap frees it. NULL + errno on failure. */
static uint32_t *
same_va_bo(struct ctx *c, uint64_t va_pages, uint64_t commit_pages, uint64_t extra)
{
   union kb_mem_alloc req = {.in = {.va_pages = va_pages,
                                    .commit_pages = commit_pages,
                                    .extension = (extra & BASE_MEM_GROW_ON_GPF) ? TILE_PAGES : 0,
                                    .flags = BO_FLAGS | BASE_MEM_SAME_VA | extra}};
   if (ioctl(c->fd, KBASE_IOCTL_MEM_ALLOC, &req))
      return NULL;
   void *p = mmap(NULL, va_pages * PAGE, PROT_READ | PROT_WRITE, MAP_SHARED, c->fd, req.out.gpu_va);
   if (p == MAP_FAILED) {
      int e = errno;
      mem_free(c, req.out.gpu_va); /* an unmapped cookie stays pending until freed */
      errno = e;
      return NULL;
   }
   return p;
}

/* One-tile MEM_ALLOC_EX FIXED region at va (growable: GROW_ON_GPF, extension one tile). 0 + errno on failure. */
static uint64_t
fixed_alloc(struct ctx *c, uint64_t va, uint64_t commit_pages, bool growable)
{
   union kb_mem_alloc_ex req = {.in = {.va_pages = TILE_PAGES,
                                       .commit_pages = commit_pages,
                                       .extension = growable ? TILE_PAGES : 0,
                                       .flags = BO_FLAGS | BASE_MEM_FIXED | (growable ? BASE_MEM_GROW_ON_GPF : 0),
                                       .fixed_address = va}};
   return ioctl(c->fd, KBASE_IOCTL_MEM_ALLOC_EX, &req) ? 0 : req.out.gpu_va;
}

/* ---- steps ---- */

/* Same handshake as the backend (kbase_kmod_dev_create) minus EXEC_INIT/JIT_INIT, which no step needs. */
static void
t0_setup(struct ctx *c)
{
   c->fd = open("/dev/mali0", O_RDWR | O_CLOEXEC);
   if (c->fd < 0) {
      snprintf(g_no_ctx, sizeof(g_no_ctx), "open /dev/mali0: %s", strerror(errno));
      S("setup %s", g_no_ctx);
      return;
   }
   struct kb_version_check ver = {0};
   if (ioctl(c->fd, KBASE_IOCTL_VERSION_CHECK_CSF, &ver) == 0) {
      c->csf = true;
   } else {
      memset(&ver, 0, sizeof(ver));
      if (ioctl(c->fd, KBASE_IOCTL_VERSION_CHECK_JM, &ver)) {
         snprintf(g_no_ctx, sizeof(g_no_ctx), "VERSION_CHECK rejected: %s", strerror(errno));
         S("setup %s", g_no_ctx);
         return;
      }
   }
   c->major = ver.major;
   c->minor = ver.minor;
   if (!c->csf && c->major < 11) {
      snprintf(g_no_ctx, sizeof(g_no_ctx), "legacy JM uAPI %u.%u (< 11, not supported by the backend)", c->major,
               c->minor);
      S("setup %s", g_no_ctx);
      return;
   }
   struct kb_set_flags sf = {0};
   if (ioctl(c->fd, KBASE_IOCTL_SET_FLAGS, &sf)) {
      snprintf(g_no_ctx, sizeof(g_no_ctx), "SET_FLAGS: %s", strerror(errno));
      S("setup %s", g_no_ctx);
      return;
   }
   /* allocations are refused until the tracking page is mapped (kbase_mem_allow_alloc) */
   c->track = mmap(NULL, PAGE, PROT_NONE, MAP_SHARED, c->fd, BASE_MEM_MAP_TRACKING_HANDLE);
   if (c->track == MAP_FAILED) {
      c->track = NULL;
      snprintf(g_no_ctx, sizeof(g_no_ctx), "tracking page mmap: %s", strerror(errno));
      S("setup %s", g_no_ctx);
      return;
   }
   struct kb_get_gpuprops gp = {0};
   int sz = ioctl(c->fd, KBASE_IOCTL_GET_GPUPROPS, &gp);
   uint8_t *props = sz > 0 && sz <= (1 << 20) ? calloc(1, (size_t)sz) : NULL;
   if (props) {
      gp.buffer = (uintptr_t)props;
      gp.size = (uint32_t)sz;
      if (ioctl(c->fd, KBASE_IOCTL_GET_GPUPROPS, &gp) > 0) {
         c->gpu_id = (uint32_t)sk_prop_get(props, (size_t)sz, GPUPROP_RAW_GPU_ID, 0);
         if (!c->gpu_id)
            c->gpu_id = (uint32_t)(sk_prop_get(props, (size_t)sz, GPUPROP_PRODUCT_ID, 0) << 16);
         c->va_bits = sk_prop_get(props, (size_t)sz, GPUPROP_RAW_MMU_FEATURES, 0) & 0xff;
      }
      free(props);
   }
   if (!c->gpu_id)
      I("setup GET_GPUPROPS gave no gpu_id (arch unknown, va_bits not checked)");
   c->arch = (c->gpu_id >> 28) & 0xf;
   g_facts.ctx = 1;
   P("setup %s uAPI %u.%u gpu_id=0x%08x arch=v%u va_bits=%u: own kbase context", c->csf ? "CSF" : "JM", c->major,
     c->minor, c->gpu_id, c->arch, c->va_bits);
}

/* T1: native SAME_VA BOs, CPU write + readback. */
static void
t1_bos(struct ctx *c)
{
   const size_t words = TILE / 4;
   g_h0 = same_va_bo(c, TILE_PAGES, TILE_PAGES, 0);
   if (!g_h0) {
      ioctl_fail("T1 SAME_VA BO");
   } else {
      bool ok = pattern(g_h0, words, 0xA5A50000u, true);
      wc_flush();
      if (ok && pattern(g_h0, words, 0xA5A50000u, false)) {
         g_facts.same_va = 1;
         P("T1 SAME_VA BO H0 at 0x%llx (64 KiB), CPU write/readback ok", (ull)(uintptr_t)g_h0);
      } else {
         F("T1 SAME_VA BO at 0x%llx: CPU write/readback faulted or mismatched", (ull)(uintptr_t)g_h0);
      }
   }
   g_h1 = same_va_bo(c, H1_PAGES, TILE_PAGES, BASE_MEM_GROW_ON_GPF);
   if (!g_h1)
      ioctl_fail("T1 growable SAME_VA BO (GROW_ON_GPF)");
   else if (!pattern(g_h1, words, 0x5A5A0000u, true))
      F("T1 growable BO H1: CPU write faulted");
   else
      P("T1 growable SAME_VA BO H1 at 0x%llx (64 of 128 KiB committed)", (ull)(uintptr_t)g_h1);
   wc_flush();
}

/* T2-T6: MEM_ALIAS [H0, sink, H1, sink]; CPU views of entries; sink not CPU-mappable; alias pins its sources. */
static void
t2_alias(struct ctx *c)
{
   if (!g_h0 || !g_h1) {
      S("T2-T6 alias: T1 BOs missing");
      return;
   }
   struct kb_aliasing_info ai[4] = {
      {.handle = (uintptr_t)g_h0, .length = TILE_PAGES},
      {.handle = BASEP_MEM_WRITE_ALLOC_PAGES_HANDLE, .length = TILE_PAGES},
      {.handle = (uintptr_t)g_h1, .length = TILE_PAGES},
      {.handle = BASEP_MEM_WRITE_ALLOC_PAGES_HANDLE, .length = TILE_PAGES},
   };
   union kb_mem_alias req = {.in = {.flags = BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_GPU_WR | BASE_MEM_PROT_CPU_RD,
                                    .stride = TILE_PAGES,
                                    .nents = 4,
                                    .aliasing_info = (uintptr_t)ai}};
   if (ioctl(c->fd, KBASE_IOCTL_MEM_ALIAS, &req)) {
      ioctl_fail("T2 MEM_ALIAS [H0,sink,H1,sink]");
      return;
   }
   P("T2 MEM_ALIAS [H0,sink,H1,sink] created (%llu pages)", (ull)req.out.va_pages);

   /* The cookie mmap fixes the alias VA; its CPU faults resolve against entry 0 only. */
   uint32_t *whole = mmap(NULL, 4 * TILE, PROT_READ, MAP_SHARED, c->fd, req.out.gpu_va);
   if (whole == MAP_FAILED) {
      F("T3 mmap of the alias cookie: %s", strerror(errno));
      mem_free(c, req.out.gpu_va);
      S("T4-T6 not run: alias not mapped");
      return;
   }
   const uint64_t va = (uintptr_t)whole;
   uint32_t v = 0;
   if (!peek(&whole[1], &v))
      F("T3 alias entry 0 read faulted");
   else if (v != 0xA5A50001u)
      F("T3 alias entry 0 reads 0x%08x, H0 holds 0xa5a50001", v);
   else
      P("T3 alias at 0x%llx: entry 0 reads H0's pages", (ull)va);

   /* Per-entry ("dumping") CPU map by VA: allowed for a real entry, refused for a sink entry. */
   uint32_t *e2 = mmap(NULL, TILE, PROT_READ, MAP_SHARED, c->fd, va + 2 * TILE);
   if (e2 == MAP_FAILED) {
      F("T4 per-entry mmap of alias entry 2: %s", strerror(errno));
   } else {
      v = 0;
      if (!peek(&e2[1], &v))
         F("T4 alias entry 2 read faulted");
      else if (v != 0x5A5A0001u)
         F("T4 alias entry 2 reads 0x%08x, H1 holds 0x5a5a0001", v);
      else
         P("T4 per-entry mmap of alias entry 2 reads H1's pages");
      munmap(e2, TILE);
   }
   uint32_t *es = mmap(NULL, TILE, PROT_READ, MAP_SHARED, c->fd, va + TILE);
   if (es == MAP_FAILED) {
      P("T5 sink entry CPU mmap refused (%s): sink contents are GPU-only", strerror(errno));
   } else {
      F("T5 sink entry CPU mmap unexpectedly allowed");
      munmap(es, TILE);
   }

   /* T6: an alias pins its sources (gpu_mappings > 1), so MEM_COMMIT on H1 is refused until the alias goes. */
   bool resized = commit(c, (uintptr_t)g_h1, H1_PAGES);
   int pin_err = errno;
   munmap(whole, 4 * TILE); /* SAME_VA free-on-close: frees the alias */
   if (resized) {
      F("T6 MEM_COMMIT on an aliased source succeeded (the alias did not pin it)");
      commit(c, (uintptr_t)g_h1, TILE_PAGES);
   } else if (commit(c, (uintptr_t)g_h1, H1_PAGES)) {
      P("T6 MEM_COMMIT on an alias source refused while aliased (%s), accepted once the alias is freed",
        strerror(pin_err));
      commit(c, (uintptr_t)g_h1, TILE_PAGES);
   } else {
      S("T6 inconclusive: MEM_COMMIT on H1 refused even without the alias (%s)", strerror(errno));
   }
}

/* T7: MEM_ALIAS masks BASE_MEM_FIXED, so an alias can never sit at a fixed VA. */
static void
t7_alias_fixed(struct ctx *c)
{
   if (!c->csf) {
      S("T7 alias+FIXED: CSF only (BASE_MEM_FIXED is a CSF flag)");
      return;
   }
   if (!g_h0) {
      S("T7 alias+FIXED: T1 BO missing");
      return;
   }
   struct kb_aliasing_info ai = {.handle = (uintptr_t)g_h0, .length = TILE_PAGES};
   union kb_mem_alias req = {.in = {.flags = BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_GPU_WR | BASE_MEM_FIXED,
                                    .stride = TILE_PAGES,
                                    .nents = 1,
                                    .aliasing_info = (uintptr_t)&ai}};
   if (ioctl(c->fd, KBASE_IOCTL_MEM_ALIAS, &req)) {
      P("T7 alias with BASE_MEM_FIXED rejected (%s): no alias at a fixed VA", strerror(errno));
      return;
   }
   if (req.out.flags & BASE_MEM_FIXED)
      F("T7 alias kept BASE_MEM_FIXED (gpu_va 0x%llx)", (ull)req.out.gpu_va);
   else
      P("T7 alias dropped BASE_MEM_FIXED (SAME_VA cookie 0x%llx): no alias at a fixed VA", (ull)req.out.gpu_va);
   mem_free(c, req.out.gpu_va);
}

/* T8: SAME_VA alloc + munmap cycles, how often the same VA comes back (why munmap+recreate is not a design). */
static void
t8_same_va_reuse(struct ctx *c)
{
   enum { N = 64 };
   const uint64_t deadline = now_ns() + 1500000000ull;
   uint64_t first = 0;
   unsigned ok = 0, same = 0;
   int err = 0;
   for (unsigned i = 0; i < N && now_ns() < deadline; i++) {
      uint32_t *p = same_va_bo(c, TILE_PAGES, TILE_PAGES, 0);
      if (!p) {
         err = errno;
         break;
      }
      if (!ok)
         first = (uintptr_t)p;
      else
         same += (uintptr_t)p == first;
      ok++;
      munmap(p, TILE);
   }
   if (err || !ok)
      F("T8 SAME_VA alloc/munmap cycle stopped after %u: %s", ok, strerror(err));
   else
      P("T8 SAME_VA alloc/munmap x%u: first VA came back %u/%u times (quiet process; Wine/ART can take it)", ok,
        same, ok - 1);
}

/* T9: the JM/v14 eager arena: SAME_VA GROW_ON_GPF region, commit grows and shrinks one 64 KiB tile at a time. */
static void
t9_arena(struct ctx *c)
{
   enum { VA_PAGES = 4096, MAX_PAGES = 1024 }; /* 16 MiB VA, at most 4 MiB committed */
   uint32_t *a = same_va_bo(c, VA_PAGES, TILE_PAGES, BASE_MEM_GROW_ON_GPF);
   if (!a) {
      ioctl_fail("T9 SAME_VA GROW_ON_GPF arena");
      return;
   }
   const uint64_t va = (uintptr_t)a, deadline = now_ns() + 2000000000ull;
   const uint64_t c0 = committed(c, va);
   unsigned grows = 0, shrinks = 0;
   uint64_t n, t0 = now_ns();
   for (n = 2 * TILE_PAGES; n <= MAX_PAGES && now_ns() < deadline && commit(c, va, n); n += TILE_PAGES)
      grows++;
   const int grow_err = errno;
   const uint64_t t_grow = now_ns() - t0, c1 = committed(c, va);
   /* the newest tile is CPU-usable (pages fault in up to the commit) */
   bool top = c1 != UINT64_MAX && c1 > TILE_PAGES && poke(&a[c1 * PAGE / 4 - 1], 0x600DF00Du);
   t0 = now_ns();
   if (c1 != UINT64_MAX)
      for (n = c1 - TILE_PAGES; n >= TILE_PAGES && n < c1 && now_ns() < deadline && commit(c, va, n);
           n -= TILE_PAGES)
         shrinks++;
   const uint64_t t_shrink = now_ns() - t0, c2 = committed(c, va);
   munmap(a, VA_PAGES * PAGE);
   if (c0 == TILE_PAGES && grows && c1 == TILE_PAGES * (1 + grows) && c2 == TILE_PAGES && shrinks == grows) {
      g_facts.arena_grow = 1;
      P("T9 arena commit grew 16->%llu pages and shrank back from the end%s", (ull)c1,
        top ? ", newest tile CPU-writable" : "");
      printf("BENCH sparse_arena_commit=%.1fus sparse_arena_uncommit=%.1fus (per 64KiB tile, n=%u)\n",
             us_per(t_grow, grows), us_per(t_shrink, shrinks), grows);
   } else if (!grows && unavailable(grow_err)) {
      S("T9 MEM_COMMIT unavailable on this kernel (%s)", strerror(grow_err));
   } else {
      F("T9 arena commit: MEM_QUERY %llu -> %llu -> %llu pages after %u grows / %u shrinks (errno after grow: %s)",
        (ull)c0, (ull)c1, (ull)c2, grows, shrinks, strerror(grow_err));
   }
}

/* T10 (CSF uAPI >= 1.9, va_bits >= 48): FIXED-VA tiles, the lazy-tile design's primitive. */
static void
t10_fixed(struct ctx *c)
{
   if (!c->csf || !(c->major > 1 || (c->major == 1 && c->minor >= 9))) {
      g_no_lazy = c->csf ? "CSF uAPI < 1.9 has no MEM_ALLOC_EX" : "JM kbase has no FIXED-VA allocations";
      S("T10 FIXED-VA tiles: %s (%s %u.%u)", g_no_lazy, c->csf ? "CSF" : "JM", c->major, c->minor);
      return;
   }
   if (c->va_bits && c->va_bits < 48) {
      g_no_lazy = "GPU va_bits < 48, FIXED_VA zone unreachable";
      S("T10 FIXED-VA tiles: va_bits=%u < 48", c->va_bits);
      return;
   }

   /* (a) FIXED alloc at R; CPU view through mmap(offset = VA) */
   uint64_t va = fixed_alloc(c, SK_FIXED_R, TILE_PAGES, false);
   if (!va) {
      g_no_lazy = "MEM_ALLOC_EX FIXED refused";
      if (unavailable(errno) || errno == EINVAL)
         S("T10 MEM_ALLOC_EX FIXED at 0x%llx unavailable on this kernel (%s)", (ull)SK_FIXED_R, strerror(errno));
      else
         F("T10a MEM_ALLOC_EX FIXED at 0x%llx: %s", (ull)SK_FIXED_R, strerror(errno));
      return;
   }
   if (va != SK_FIXED_R) {
      g_no_lazy = "FIXED alloc not placed at the requested VA";
      F("T10a FIXED alloc landed at 0x%llx, asked 0x%llx", (ull)va, (ull)SK_FIXED_R);
      mem_free(c, va);
      return;
   }
   g_facts.fixed_va = 1;
   P("T10a MEM_ALLOC_EX FIXED landed at the requested VA 0x%llx", (ull)va);
   uint32_t *m = mmap(NULL, TILE, PROT_READ | PROT_WRITE, MAP_SHARED, c->fd, va);
   if (m == MAP_FAILED) {
      I("T10a CPU mmap of the FIXED region refused (%s); GPU-only tiles do not need it", strerror(errno));
   } else {
      uint32_t v = 0;
      bool rw = poke(&m[1], 0xDEADBEEFu);
      wc_flush();
      if (rw && peek(&m[1], &v) && v == 0xDEADBEEFu)
         I("T10a CPU write/readback through mmap(offset=VA) ok");
      else
         F("T10a CPU write/readback of the FIXED region faulted or mismatched (0x%08x)", v);
      munmap(m, TILE);
   }

   /* (b) a second FIXED alloc over a live one is refused */
   uint64_t dup = fixed_alloc(c, va, TILE_PAGES, false);
   if (!dup) {
      P("T10b overlapping FIXED alloc refused (%s)", strerror(errno));
   } else {
      F("T10b overlapping FIXED alloc accepted (0x%llx)", (ull)dup);
      mem_free(c, dup);
   }

   /* (c) rebind: free, then allocate again at the same VA */
   mem_free(c, va);
   uint64_t t0 = now_ns();
   uint64_t va2 = fixed_alloc(c, SK_FIXED_R, TILE_PAGES, false);
   const uint64_t t_realloc = now_ns() - t0;
   if (va2 == SK_FIXED_R) {
      g_facts.fixed_rebind = 1;
      P("T10c freed and re-allocated at the same FIXED VA (%.1f us)", t_realloc / 1000.0);
   } else {
      g_no_lazy = "re-alloc at a freed FIXED VA failed";
      F("T10c re-alloc at 0x%llx after free: %s", (ull)SK_FIXED_R, va2 ? "landed elsewhere" : strerror(errno));
   }
   if (va2)
      mem_free(c, va2);

   /* (d) one growable 64 KiB tile: commit 16 -> 0 -> 16 must take effect; fresh pages should read 0 */
   const uint64_t tva = SK_FIXED_R + (2ull << 20);
   uint64_t t = fixed_alloc(c, tva, 0, true);
   if (!t) {
      I("T10d growable FIXED tile with commit 0 refused (%s); retrying with commit 1", strerror(errno));
      g_tile_floor = 1;
      t = fixed_alloc(c, tva, 1, true);
   }
   if (!t) {
      g_no_lazy = "growable (GROW_ON_GPF) FIXED tile refused";
      F("T10d growable FIXED tile at 0x%llx: %s", (ull)tva, strerror(errno));
   } else {
      const uint64_t q0 = committed(c, t);
      const bool ok1 = commit(c, t, TILE_PAGES);
      const uint64_t q1 = committed(c, t);
      uint32_t *tm = mmap(NULL, TILE, PROT_READ | PROT_WRITE, MAP_SHARED, c->fd, t);
      /* marker in the last page: the one an uncommit to 0 or 1 page always drops */
      volatile uint32_t *last = tm != MAP_FAILED ? &tm[TILE / 4 - 1] : NULL;
      const bool marked = last && poke(last, 0xCAFEBABEu);
      wc_flush();
      const bool ok2 = commit(c, t, g_tile_floor);
      const uint64_t q2 = committed(c, t);
      const bool ok3 = commit(c, t, TILE_PAGES);
      const uint64_t q3 = committed(c, t);
      uint32_t z = 1;
      const bool zeroed = marked && peek(last, &z) && z == 0;
      if (tm != MAP_FAILED)
         munmap(tm, TILE);
      mem_free(c, t);
      if (ok1 && ok2 && ok3 && q1 == TILE_PAGES && q2 == g_tile_floor && q3 == TILE_PAGES) {
         g_facts.tile_cycle = 1;
         P("T10d per-tile commit %llu->16->%u->16 took effect (MEM_QUERY)%s", (ull)q0, g_tile_floor,
           zeroed   ? "; re-committed page reads 0"
           : marked ? "; re-committed page NOT zero (unbind must zero it)"
                    : "");
      } else {
         g_no_lazy = q2 == TILE_PAGES ? "tile uncommit ignored (fully-backed kernel?)"
                                      : "per-tile commit cycle did not take effect";
         F("T10d per-tile commit cycle: commit ok=%d/%d/%d, MEM_QUERY %llu/%llu/%llu, want 16/%u/16", ok1, ok2, ok3,
           (ull)q1, (ull)q2, (ull)q3, g_tile_floor);
      }
   }

   /* (e) FIXABLE and FIXED are exclusive per context */
   uint64_t keep = fixed_alloc(c, SK_FIXED_R, TILE_PAGES, false);
   if (keep) {
      union kb_mem_alloc_ex fx = {
         .in = {.va_pages = TILE_PAGES, .commit_pages = TILE_PAGES, .flags = BO_FLAGS | BASE_MEM_FIXABLE}};
      if (ioctl(c->fd, KBASE_IOCTL_MEM_ALLOC_EX, &fx)) {
         P("T10e FIXABLE refused while a FIXED alloc is live (%s)", strerror(errno));
      } else {
         I("T10e FIXABLE accepted alongside FIXED (vendor kernel differs)");
         mem_free(c, fx.out.gpu_va);
      }
      mem_free(c, keep);
   }
}

/* T10f: bind/unbind latency over contiguous growable FIXED tiles (reserve, commit, uncommit, free) + leak check. */
static void
t10f_tile_timing(struct ctx *c)
{
   if (!g_facts.fixed_va || !g_facts.tile_cycle) {
      S("T10f tile bind/unbind timing: needs T10a and T10d");
      return;
   }
   enum { NT = 256, CYCLES = 8 }; /* 16 MiB committed at most */
   const uint64_t base = SK_FIXED_R + (4ull << 20), deadline = now_ns() + 3000000000ull;
   unsigned made = 0, cycles = 0, bad = 0, leaked = 0;
   uint64_t t0 = now_ns();
   while (made < NT && now_ns() < deadline && fixed_alloc(c, base + made * TILE, g_tile_floor, true))
      made++;
   const uint64_t t_reserve = now_ns() - t0;
   uint64_t t_bind = 0, t_unbind = 0;
   for (; made && cycles < CYCLES && now_ns() < deadline; cycles++) {
      t0 = now_ns();
      for (unsigned i = 0; i < made; i++)
         bad += !commit(c, base + i * TILE, TILE_PAGES);
      const uint64_t t1 = now_ns();
      for (unsigned i = 0; i < made; i++)
         bad += !commit(c, base + i * TILE, g_tile_floor);
      t_bind += t1 - t0;
      t_unbind += now_ns() - t1;
   }
   for (unsigned i = 0; i < made; i++)
      leaked += committed(c, base + i * TILE) != g_tile_floor;
   t0 = now_ns();
   for (unsigned i = 0; i < made; i++)
      mem_free(c, base + i * TILE);
   const uint64_t t_free = now_ns() - t0;
   if (made)
      printf("BENCH sparse_tile_bind=%.1fus sparse_tile_unbind=%.1fus sparse_tile_reserve=%.1fus "
             "sparse_tile_free=%.1fus (64KiB FIXED tiles, n=%u x %u cycles)\n",
             us_per(t_bind, made * cycles), us_per(t_unbind, made * cycles), us_per(t_reserve, made),
             us_per(t_free, made), made, cycles);
   if (!made || !cycles)
      F("T10f no tiles reserved (%s)", strerror(errno));
   else if (bad || leaked)
      F("T10f %u commit errors, %u of %u tiles not back at %u pages after %u cycles", bad, leaked, made,
        g_tile_floor, cycles);
   else
      P("T10f %u tiles x %u bind/unbind cycles: no commit errors, every tile back at %u pages", made, cycles,
        g_tile_floor);
}

/* T11: cost of rebuilding a 24-entry alias (the rejected "alias rebuild + re-point" design). */
static void
t11_alias_rebuild(struct ctx *c)
{
   if (!g_h0) {
      S("T11 alias rebuild: T1 BO missing");
      return;
   }
   enum { ENTS = 24, ITERS = 256 };
   struct kb_aliasing_info ai[ENTS];
   for (int i = 0; i < ENTS; i++)
      ai[i] = (struct kb_aliasing_info){.handle = (uintptr_t)g_h0, .length = TILE_PAGES};
   const uint64_t t0 = now_ns(), deadline = t0 + 1500000000ull;
   unsigned ok = 0;
   int err = 0;
   for (; ok < ITERS && now_ns() < deadline; ok++) {
      union kb_mem_alias req = {.in = {.flags = BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_GPU_WR,
                                       .stride = TILE_PAGES,
                                       .nents = ENTS,
                                       .aliasing_info = (uintptr_t)ai}};
      if (ioctl(c->fd, KBASE_IOCTL_MEM_ALIAS, &req)) {
         err = errno;
         break;
      }
      mem_free(c, req.out.gpu_va);
   }
   if (ok)
      printf("BENCH sparse_alias_rebuild24=%.1fus (n=%u)\n", us_per(now_ns() - t0, ok), ok);
   if (err) {
      errno = err;
      ioctl_fail("T11 MEM_ALIAS rebuild");
   } else {
      P("T11 rebuilt a 24-entry alias %u times", ok);
   }
}

static void
run_step(const char *name, void (*fn)(struct ctx *), struct ctx *c)
{
   if (g_aborted) {
      S("%s not run: an earlier step was aborted", name);
      return;
   }
   g_in_step = 1;
   if (!sigsetjmp(g_step_jmp, 1)) {
      alarm(STEP_SECONDS);
      fn(c);
   } else {
      g_aborted = true;
      if (g_sig == SIGALRM)
         F("%s watchdog: step ran over %d s; later steps skipped", name, STEP_SECONDS);
      else
         F("%s unguarded signal %d; later steps skipped", name, (int)g_sig);
   }
   alarm(0);
   g_in_step = 0;
   g_in_guard = 0;
}

static void
verdict(const struct ctx *c)
{
   const enum sk_verdict v = sk_verdict(&g_facts);
   char arch[16] = "v?";
   if (c->arch)
      snprintf(arch, sizeof(arch), "v%u", c->arch);
   printf("VERDICT %s %s -- ", arch, sk_verdict_name(v));
   if (v == SK_LAZY_TILE)
      printf("64 KiB FIXED tiles commit 16->%u->16 and re-alloc at the same VA (CSF %u.%u)\n", g_tile_floor,
             c->major, c->minor);
   else if (v == SK_EAGER_ONLY)
      printf("%s; SAME_VA arena, grow-from-end %s\n", g_no_lazy,
             g_facts.arena_grow ? "ok" : "not proven (fixed-size arenas)");
   else
      printf("%s\n", g_facts.ctx ? "no SAME_VA allocation" : g_no_ctx);
}

int
main(int argc, char **argv)
{
   (void)argc;
   (void)argv;
   struct sigaction sa;
   memset(&sa, 0, sizeof(sa));
   sa.sa_handler = on_signal;
   sa.sa_flags = SA_ONSTACK; /* libpt_runner.so installed an alternate signal stack */
   sigemptyset(&sa.sa_mask);
   sigaction(SIGSEGV, &sa, NULL);
   sigaction(SIGBUS, &sa, NULL);
   sigaction(SIGALRM, &sa, NULL);

   struct ctx c = {.fd = -1};
   I("sparse_kbase S0: own /dev/mali0 context, CPU-side kbase ioctls only, no GPU jobs");
   run_step("setup", t0_setup, &c);
   if (g_facts.ctx) {
      run_step("T1", t1_bos, &c);
      run_step("T2-T6", t2_alias, &c);
      run_step("T7", t7_alias_fixed, &c);
      run_step("T8", t8_same_va_reuse, &c);
      run_step("T9", t9_arena, &c);
      run_step("T10", t10_fixed, &c);
      run_step("T10f", t10f_tile_timing, &c);
      run_step("T11", t11_alias_rebuild, &c);
   }
   if (g_h0)
      munmap(g_h0, TILE);
   if (g_h1)
      munmap(g_h1, H1_PAGES * PAGE);
   if (c.track)
      munmap(c.track, PAGE);
   if (c.fd >= 0)
      close(c.fd);
   verdict(&c);
   printf("SUMMARY pass=%d fail=%d skip=%d\n", g_pass, g_fail, g_skip);
   if (!g_facts.ctx)
      printf("RESULT SKIP no usable kbase context: %s\n", g_no_ctx);
   return 0;
}
