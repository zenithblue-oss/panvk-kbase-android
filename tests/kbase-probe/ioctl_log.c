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
      /* MEM_IMPORT (nr 22): union overlays in/out; log OUT fields */
      unsigned nr = (unsigned)req & 0xff;
      if (nr == 22 && r == 0 && arg) {
         uint64_t ofl = 0, ova = 0, opg = 0;
         memcpy(&ofl, (char *)arg + 0, 8);
         memcpy(&ova, (char *)arg + 8, 8);
         memcpy(&opg, (char *)arg + 16, 8);
         int n = snprintf(buf, sizeof(buf),
                          "IOCTL fd=%d IMPORT r=%d in_flags=0x%llx va=0x%llx pages=%llu flags=0x%llx\n",
                          fd, r, (unsigned long long)a0,
                          (unsigned long long)ova, (unsigned long long)opg,
                          (unsigned long long)ofl);
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
