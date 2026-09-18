/* MEM_SYNC probe: dma_heap alloc -> kbase import -> MEM_SYNC flush.
 * If MEM_SYNC fails on this DDK, driver cache flushes silently break and
 * the GPU reads stale descriptors/streams (hang without fault).
 */
#include <errno.h>
#include <fcntl.h>
#include <linux/dma-heap.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "drm-uapi/mali_kbase_ioctl.h"
#ifndef KBASE_MEM_SYNC_FLUSH
#define KBASE_MEM_SYNC_FLUSH 0
#define KBASE_MEM_SYNC_INV 1
#define KBASE_MEM_SYNC_CLEAN 2
#endif

int
main(void)
{
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

   int dfd = fcntl(ad.fd, F_DUPFD_CLOEXEC, 3);
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
