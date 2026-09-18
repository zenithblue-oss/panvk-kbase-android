/* LD_PRELOAD ioctl logger: logs ioctls issued to /dev/mali0.
 * Usage: LD_PRELOAD=./ioctl_log.so TARGET ...
 * Log goes to /data/local/tmp/ioctl.log (append).
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>

static int (*real_ioctl)(int, int, ...) = NULL;
static int log_fd = -1;

static void
ensure(void)
{
   if (!real_ioctl)
      real_ioctl = dlsym(RTLD_NEXT, "ioctl");
   if (log_fd < 0)
      log_fd = open("/data/local/tmp/ioctl.log",
                    O_WRONLY | O_CREAT | O_APPEND, 0644);
}

static int
is_mali(int fd)
{
   char path[64], link[128];
   snprintf(path, sizeof(path), "/proc/self/fd/%d", fd);
   ssize_t n = readlink(path, link, sizeof(link) - 1);
   if (n <= 0)
      return 0;
   link[n] = 0;
   return strstr(link, "mali") != NULL;
}

static void *(*real_mmap)(void *, size_t, int, int, int, off_t) = NULL;
static void *(*real_mmap64)(void *, size_t, int, int, int, off_t) = NULL;

static void
log_mmap(const char *which, int fd, size_t len, int prot, off_t off, void *r)
{
   if (log_fd >= 0 && fd >= 0 && is_mali(fd)) {
      char buf[192];
      int n = snprintf(buf, sizeof(buf),
                       "MMAP-%s fd=%d len=%zu prot=%d off=0x%llx r=%p\n",
                       which, fd, len, prot, (unsigned long long)off, r);
      write(log_fd, buf, n);
   }
}

void *
mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off)
{
   ensure();
   if (!real_mmap)
      real_mmap = dlsym(RTLD_NEXT, "mmap");
   void *r = real_mmap(addr, len, prot, flags, fd, off);
   log_mmap("mmap", fd, len, prot, off, r);
   return r;
}

void *
mmap64(void *addr, size_t len, int prot, int flags, int fd, off_t off)
{
   ensure();
   if (!real_mmap64)
      real_mmap64 = dlsym(RTLD_NEXT, "mmap64");
   void *r = real_mmap64(addr, len, prot, flags, fd, off);
   log_mmap("mmap64", fd, len, prot, off, r);
   return r;
}

int
ioctl(int fd, int req, ...)
{
   va_list ap;
   void *arg = NULL;
   ensure();
   va_start(ap, req);
   arg = va_arg(ap, void *);
   va_end(ap);
   int r;
   /* pre-call snapshot for MEM_IMPORT (union: OUT overwrites IN) */
   unsigned nr = (unsigned)req & 0xff;
   uint64_t pre0 = 0, pre1 = 0, pre2 = 0;
   if (nr == 22 && arg) {
      memcpy(&pre0, arg, 8);
      memcpy(&pre1, (char *)arg + 8, 8);
      memcpy(&pre2, (char *)arg + 16, 8);
   }
   {
      /* forward: ioctl is varargs; call with the pointer */
      r = real_ioctl(fd, req, arg);
   }
   if (log_fd >= 0 && is_mali(fd)) {
      char buf[256];
      uint64_t a0 = 0, a1 = 0;
      if (arg) {
         memcpy(&a0, arg, 8);
         memcpy(&a1, (char *)arg + 8, 8);
      }
      if (nr == 22 && arg) {
         char pre[160];
         int pn = snprintf(pre, sizeof(pre),
                           "IOCTL fd=%d IMPORT-IN flags=0x%llx type=%llu\n",
                           fd, (unsigned long long)pre0,
                           (unsigned long long)pre2);
         write(log_fd, pre, pn);
      }
      if ((nr == 22 || nr == 59) && r == 0 && arg) {
         uint64_t w[6] = {0};
         memcpy(w, arg, sizeof(w));
         int n = snprintf(buf, sizeof(buf),
                          "IOCTL fd=%d %s r=%d w0=0x%llx w1=0x%llx w2=0x%llx w3=0x%llx w4=0x%llx w5=0x%llx\n",
                          fd, nr == 22 ? "IMPORT" : "ALLOC_EX", r,
                          (unsigned long long)w[0], (unsigned long long)w[1],
                          (unsigned long long)w[2], (unsigned long long)w[3],
                          (unsigned long long)w[4], (unsigned long long)w[5]);
         write(log_fd, buf, n);
      } else {
         int n = snprintf(buf, sizeof(buf),
                          "IOCTL fd=%d req=0x%x nr=%u r=%d errno=%d a0=0x%llx a1=0x%llx\n",
                          fd, req, nr, r, r < 0 ? errno : 0,
                          (unsigned long long)a0, (unsigned long long)a1);
         write(log_fd, buf, n);
      }
   }
   return r;
}
