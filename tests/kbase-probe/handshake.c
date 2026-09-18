/* Kbase handshake probe: VERSION_CHECK (CSF nr52 then JM nr0),
 * SET_FLAGS, tracking-page mmap, GPUPROPS size probe.
 * Prints each step with errno. No GPU submission. Bounded and safe.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "drm-uapi/mali_kbase_ioctl.h"

int
main(void)
{
   int fd = open("/dev/mali0", O_RDWR | O_CLOEXEC);
   if (fd < 0) {
      printf("H-FAIL open: %s\n", strerror(errno));
      return 1;
   }
   printf("H-OK open fd=%d\n", fd);

   struct kbase_ioctl_version_check ver;
   memset(&ver, 0, sizeof(ver));
   if (ioctl(fd, KBASE_IOCTL_VERSION_CHECK_CSF, &ver) == 0) {
      printf("H-OK VERSION_CHECK_CSF flavor=CSF major=%u minor=%u\n",
             ver.major, ver.minor);
   } else {
      printf("H-INFO VERSION_CHECK_CSF failed: %s\n", strerror(errno));
      memset(&ver, 0, sizeof(ver));
      if (ioctl(fd, KBASE_IOCTL_VERSION_CHECK_JM, &ver) == 0) {
         printf("H-OK VERSION_CHECK_JM flavor=JM major=%u minor=%u\n",
                ver.major, ver.minor);
      } else {
         printf("H-FAIL VERSION_CHECK_JM: %s\n", strerror(errno));
         return 1;
      }
   }

   struct kbase_ioctl_set_flags set_flags;
   memset(&set_flags, 0, sizeof(set_flags));
   if (ioctl(fd, KBASE_IOCTL_SET_FLAGS, &set_flags))
      printf("H-FAIL SET_FLAGS: %s\n", strerror(errno));
   else
      printf("H-OK SET_FLAGS\n");

   void *tp = mmap(NULL, 4096, PROT_NONE, MAP_SHARED, fd,
                   BASE_MEM_MAP_TRACKING_HANDLE);
   if (tp == MAP_FAILED)
      printf("H-FAIL tracking mmap: %s\n", strerror(errno));
   else {
      printf("H-OK tracking mmap\n");
      munmap(tp, 4096);
   }

   close(fd);
   printf("H-PASS\n");
   return 0;
}
