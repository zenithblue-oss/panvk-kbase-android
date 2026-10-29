// SPDX-License-Identifier: MIT
/*
 * memfdshim: LD_PRELOAD shim for Android < 11. Bionic only exports memfd_create from API 30, so
 * imagefs libxshmfence.so.1 fails to dlopen on API 29 ("cannot locate symbol memfd_create"), the
 * Mesa X11 WSI then reports VK_ERROR_OUT_OF_HOST_MEMORY from surface queries. The kernel (>= 3.17)
 * has the syscall; export the libc wrapper ourselves.
 */
#define _GNU_SOURCE
#include <sys/syscall.h>
#include <unistd.h>

__attribute__((visibility("default")))
int memfd_create(const char *name, unsigned int flags)
{
   return (int)syscall(__NR_memfd_create, name, flags);
}
