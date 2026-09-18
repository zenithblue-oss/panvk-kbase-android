/* MEM_SYNC probe: dma_heap alloc -> kbase import -> MEM_SYNC flush.
 * If MEM_SYNC fails on this DDK, driver cache flushes silently break and
 * the GPU reads stale descriptors/streams (hang without fault).
 * With argv[1]=="ahb": import the fd of a real MediaTek AHB instead and
 * dump req.out (flags/va_pages/gpu_va) to compare against dma_heap.
 */
#include <errno.h>
#include <fcntl.h>
#include <linux/dma-heap.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
typedef struct { int version, numFds, numInts, data[0]; } native_handle_t;
typedef struct AHardwareBuffer AHardwareBuffer;
const native_handle_t *AHardwareBuffer_getNativeHandle(const AHardwareBuffer *b);

#include "drm-uapi/mali_kbase_ioctl.h"
#include <android/hardware_buffer.h>
#ifndef KBASE_MEM_SYNC_FLUSH
#define KBASE_MEM_SYNC_FLUSH 0
#define KBASE_MEM_SYNC_INV 1
#define KBASE_MEM_SYNC_CLEAN 2
#endif

int
main(int argc, char **argv)
{
   setvbuf(stdout, NULL, _IONBF, 0);
   int src_fd = -1;
   AHardwareBuffer *ahb = NULL;
   if (argc > 1 && !strcmp(argv[1], "ahb")) {
      uint64_t uw = AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT |
                    AHARDWAREBUFFER_USAGE_CPU_READ_RARELY;
      if (argc > 2 && !strcmp(argv[2], "cached"))
         uw |= AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN;
      AHardwareBuffer_Desc d = {.width = 64,
                                .height = 64,
                                .layers = 1,
                                .format =
                                   AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM,
                                .usage = uw};
      if (AHardwareBuffer_allocate(&d, &ahb) || !ahb) {
         printf("S-FAIL AHB allocate\n");
         return 1;
      }
      const native_handle_t *h = AHardwareBuffer_getNativeHandle(ahb);
      printf("S-AHB numFds=%d numInts=%d fd0=%d\n", h->numFds, h->numInts,
             h->data[0]);
      src_fd = h->data[0];
   } else {
      int dh = open("/dev/dma_heap/system", O_RDONLY | O_CLOEXEC);
      if (dh < 0) {
         printf("S-FAIL dma_heap open: %s\n", strerror(errno));
         return 1;
      }
      struct dma_heap_allocation_data ad = {.len = 4096,
                                            .fd_flags = O_RDWR | O_CLOEXEC};
      if (ioctl(dh, DMA_HEAP_IOCTL_ALLOC, &ad)) {
         printf("S-FAIL dma_heap alloc: %s\n", strerror(errno));
         return 1;
      }
      printf("S-OK dma_heap alloc fd=%d\n", ad.fd);
      src_fd = ad.fd;
   }

   int kfd = open("/dev/mali0", O_RDWR | O_CLOEXEC);
   struct kbase_ioctl_version_check ver;
   memset(&ver, 0, sizeof(ver));
   if (ioctl(kfd, KBASE_IOCTL_VERSION_CHECK_CSF, &ver)) {
      printf("S-FAIL version: %s\n", strerror(errno));
      return 1;
   }
   struct kbase_ioctl_set_flags sf;
   memset(&sf, 0, sizeof(sf));
   if (ioctl(kfd, KBASE_IOCTL_SET_FLAGS, &sf)) {
      printf("S-FAIL flags: %s\n", strerror(errno));
      return 1;
   }

   int dfd = fcntl(src_fd, F_DUPFD_CLOEXEC, 3);
   if (ahb) {
      /* touch-first variant: fault pages in via dmabuf mmap BEFORE import,
       * testing whether kbase snapshots an empty SGT for lazy heaps */
      const native_handle_t *hh0 = AHardwareBuffer_getNativeHandle(ahb);
      int tfd = fcntl(hh0->data[1], F_DUPFD_CLOEXEC, 3);
      void *pre = mmap(NULL, 16384, PROT_READ | PROT_WRITE, MAP_SHARED, tfd,
                       0);
      if (pre != MAP_FAILED) {
         for (int i = 0; i < 16384; i += 4096)
            ((volatile uint8_t *)pre)[i] = (uint8_t)(i >> 12);
         printf("S-touched %p\n", pre);
         union kbase_ioctl_mem_import tq;
         memset(&tq, 0, sizeof(tq));
         tq.in.flags = BASE_MEM_PROT_CPU_RD | BASE_MEM_PROT_CPU_WR |
                       BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_GPU_WR |
                       BASE_MEM_IMPORT_SHARED | BASE_MEM_COHERENT_SYSTEM;
         tq.in.phandle = (uintptr_t)&tfd;
         tq.in.type = BASE_MEM_IMPORT_TYPE_UMM;
         int tr = ioctl(kfd, KBASE_IOCTL_MEM_IMPORT, &tq);
         printf("S-touch-import r=%d %s va=0x%llx pages=%llu\n", tr,
                tr ? strerror(errno) : "ok",
                (unsigned long long)tq.out.gpu_va,
                (unsigned long long)tq.out.va_pages);
         if (!tr) {
            struct kbase_ioctl_mem_commit cm;
            memset(&cm, 0, sizeof(cm));
            cm.gpu_addr = tq.out.gpu_va;
            cm.pages = tq.out.va_pages;
            int cr = ioctl(kfd, KBASE_IOCTL_MEM_COMMIT, &cm);
            printf("S-touch commit r=%d %s\n", cr,
                   cr ? strerror(errno) : "ok");
            void *gm = mmap(NULL, tq.out.va_pages * 4096,
                            PROT_READ | PROT_WRITE, MAP_SHARED, kfd,
                            tq.out.gpu_va);
            printf("S-touch kbase-mmap %s\n",
                   gm == MAP_FAILED ? strerror(errno) : "OK");
            if (gm != MAP_FAILED) {
               /* NOTE: writing here SIGBUSes for MTK imports (no pages);
                * writability is the signal under test, attempted last */
               munmap(gm, tq.out.va_pages * 4096);
            }
            struct kbase_ioctl_mem_free fr = {.gpu_addr = tq.out.gpu_va};
            ioctl(kfd, KBASE_IOCTL_MEM_FREE, &fr);
         }
         munmap(pre, 16384);
      }
      close(dfd);
      dfd = fcntl(hh0->data[1], F_DUPFD_CLOEXEC, 3);
   }
   if (ahb) {
      /* the data buffer is fd index 1 on this gralloc (fd0/fd2 are not
       * importable); use it for the query section below */
      const native_handle_t *hh0 = AHardwareBuffer_getNativeHandle(ahb);
      close(dfd);
      dfd = fcntl(hh0->data[1], F_DUPFD_CLOEXEC, 3);
      /* try import flag variants for the uncached heap */
      static const struct { const char *n; uint64_t clear; uint64_t set; } vs[] = {
         {"cached-cpu", 0, (1ull << 12)},
         {"base", 0, 0},
         {"no-coherent", BASE_MEM_COHERENT_SYSTEM, 0},
         {"uncached-gpu", BASE_MEM_COHERENT_SYSTEM, (1ull << 21)},
      };
      for (unsigned vi = 0; vi < 4; vi++) {
         int tfd = fcntl(hh0->data[1], F_DUPFD_CLOEXEC, 3);
         union kbase_ioctl_mem_import tq;
         memset(&tq, 0, sizeof(tq));
         uint64_t fl = BASE_MEM_PROT_CPU_RD | BASE_MEM_PROT_CPU_WR |
                       BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_GPU_WR |
                       BASE_MEM_IMPORT_SHARED | BASE_MEM_COHERENT_SYSTEM;
         fl &= ~vs[vi].clear;
         fl |= vs[vi].set;
         tq.in.flags = fl;
         tq.in.phandle = (uintptr_t)&tfd;
         tq.in.type = BASE_MEM_IMPORT_TYPE_UMM;
         int tr = ioctl(kfd, KBASE_IOCTL_MEM_IMPORT, &tq);
         printf("S-var %s r=%d %s va=0x%llx pages=%llu flags=0x%llx\n",
                vs[vi].n, tr, tr ? strerror(errno) : "ok",
                (unsigned long long)tq.out.gpu_va,
                (unsigned long long)tq.out.va_pages,
                (unsigned long long)tq.out.flags);
         if (!tr) {
            void *gm = mmap(NULL, tq.out.va_pages * 4096, PROT_READ | PROT_WRITE,
                            MAP_SHARED, kfd, tq.out.gpu_va);
            int mok = (gm != MAP_FAILED);
            union kbase_ioctl_mem_query q;
            memset(&q, 0, sizeof(q));
            q.in.gpu_addr = tq.out.gpu_va;
            q.in.query = 2;
            int qr = ioctl(kfd, KBASE_IOCTL_MEM_QUERY, &q);
            printf("S-var %s mmap=%s query-va-size r=%d value=0x%llx\n",
                   vs[vi].n, mok ? "OK" : strerror(errno), qr,
                   (unsigned long long)q.out.value);
            if (mok) {
               /* writability probe LAST (may SIGBUS) */
               *(volatile uint32_t *)gm = 0x12345678u;
               printf("S-var %s kbase-write OK\n", vs[vi].n);
               munmap(gm, tq.out.va_pages * 4096);
            }
            struct kbase_ioctl_mem_free fr = {.gpu_addr = tq.out.gpu_va};
            ioctl(kfd, KBASE_IOCTL_MEM_FREE, &fr);
         }
         close(tfd);
      }
      /* import every fd to find the GPU-mappable one */
      const native_handle_t *hh = AHardwareBuffer_getNativeHandle(ahb);
      for (int fi = 0; fi < hh->numFds; fi++) {
         int tfd = fcntl(hh->data[fi], F_DUPFD_CLOEXEC, 3);
         union kbase_ioctl_mem_import tq;
         memset(&tq, 0, sizeof(tq));
         tq.in.flags = BASE_MEM_PROT_CPU_RD | BASE_MEM_PROT_CPU_WR |
                       BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_GPU_WR |
                       BASE_MEM_IMPORT_SHARED | BASE_MEM_COHERENT_SYSTEM;
         tq.in.phandle = (uintptr_t)&tfd;
         tq.in.type = BASE_MEM_IMPORT_TYPE_UMM;
         int tr = ioctl(kfd, KBASE_IOCTL_MEM_IMPORT, &tq);
         printf("S-fd[%d]=%d import r=%d errno=%s va=0x%llx pages=%llu flags=0x%llx\n",
                fi, hh->data[fi], tr,
                tr ? strerror(errno) : "ok",
                (unsigned long long)tq.out.gpu_va,
                (unsigned long long)tq.out.va_pages,
                (unsigned long long)tq.out.flags);
         if (!tr) {
            struct kbase_ioctl_mem_free fr = {.gpu_addr = tq.out.gpu_va};
            ioctl(kfd, KBASE_IOCTL_MEM_FREE, &fr);
         }
         close(tfd);
      }
   }
   union kbase_ioctl_mem_import req;
   memset(&req, 0, sizeof(req));
   req.in.flags = BASE_MEM_PROT_CPU_RD | BASE_MEM_PROT_CPU_WR |
                  BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_GPU_WR |
                  BASE_MEM_IMPORT_SHARED | BASE_MEM_COHERENT_SYSTEM;
   req.in.phandle = (uintptr_t)&dfd;
   req.in.type = BASE_MEM_IMPORT_TYPE_UMM;
   if (ioctl(kfd, KBASE_IOCTL_MEM_IMPORT, &req)) {
      printf("S-FAIL import: %s\n", strerror(errno));
      return 1;
   }
   printf("S-OK import gpu_va=0x%llx pages=%llu flags=0x%llx\n",
          (unsigned long long)req.out.gpu_va,
          (unsigned long long)req.out.va_pages,
          (unsigned long long)req.out.flags);
   {
      struct kbase_ioctl_mem_free fr;
      memset(&fr, 0, sizeof(fr));
      fr.gpu_addr = req.out.gpu_va;
      int fr2 = ioctl(kfd, KBASE_IOCTL_MEM_FREE, &fr);
      printf("S-probe free r=%d %s\n", fr2, fr2 ? strerror(errno) : "ok");
   }
   for (int qi = 1; qi <= 3; qi++) {
      union kbase_ioctl_mem_query q;
      memset(&q, 0, sizeof(q));
      q.in.gpu_addr = req.out.gpu_va;
      q.in.query = qi;
      int qr = ioctl(kfd, KBASE_IOCTL_MEM_QUERY, &q);
      printf("S-query(%d) r=%d errno=%s value=0x%llx\n", qi, qr,
             qr ? strerror(errno) : "ok", (unsigned long long)q.out.value);
   }

   void *cpu = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, dfd, 0);
   if (cpu == MAP_FAILED) {
      printf("S-FAIL cpu mmap: %s\n", strerror(errno));
      return 1;
   }
   printf("S-OK cpu map %p\n", cpu);
   *(volatile uint32_t *)cpu = 0xdeadbeefu;

   struct kbase_ioctl_mem_sync ms;
   memset(&ms, 0, sizeof(ms));
   ms.handle = req.out.gpu_va;
   ms.user_addr = (uintptr_t)cpu;
   ms.size = 4096;
   ms.type = KBASE_MEM_SYNC_FLUSH;
   if (ioctl(kfd, KBASE_IOCTL_MEM_SYNC, &ms))
      printf("S-FAIL mem_sync(FLUSH, gpu_va handle): %s\n", strerror(errno));
   else
      printf("S-OK mem_sync FLUSH\n");

   /* try with SAME_VA-style: mmap kbase fd at gpu_va, use that addr */
   void *gm = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, kfd,
                   req.out.gpu_va);
   if (gm == MAP_FAILED)
      printf("S-INFO kbase mmap at gpu_va: %s\n", strerror(errno));
   else {
      printf("S-OK kbase mmap %p (gpu_va was 0x%llx)\n", gm,
             (unsigned long long)req.out.gpu_va);
      *(volatile uint32_t *)gm = 0xcafef00du;
      memset(&ms, 0, sizeof(ms));
      ms.handle = req.out.gpu_va;
      ms.user_addr = (uintptr_t)gm;
      ms.size = 4096;
      ms.type = KBASE_MEM_SYNC_FLUSH;
      if (ioctl(kfd, KBASE_IOCTL_MEM_SYNC, &ms))
         printf("S-FAIL mem_sync via kbase mapping: %s\n", strerror(errno));
      else
         printf("S-OK mem_sync via kbase mapping\n");
      munmap(gm, 4096);
   }
   munmap(cpu, 4096);
   printf("S-PASS\n");
   return 0;
}
