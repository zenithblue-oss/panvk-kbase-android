/* panvk-kbase-probe — pre-Vulkan hardware/Kbase probe (Gate B input).
 *
 * Purpose: capture SoC/GPU/ID/arch/node/open-results/errno/UAPI hints safely.
 * Source/Reference: plan PANVK_KBASE_ANDROID_DRIVER_WORKER.md sec.7; no
 *   Kbase structs are guessed — only open() + sysfs/getprop reads here.
 * Tested GPU: Mali-G615 MC6 (Poco X6 Pro, duchamp).
 * Validation test: run as shell/root AND as app UID; both results recorded.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void read_first_line(const char *path, char *out, int outlen) {
  int fd = open(path, O_RDONLY);
  if (fd < 0) {
    snprintf(out, outlen, "OPEN-FAIL errno=%d(%s)", errno, strerror(errno));
    return;
  }
  ssize_t n = read(fd, out, outlen - 1);
  close(fd);
  if (n <= 0) {
    snprintf(out, outlen, "READ-FAIL errno=%d(%s)", errno, strerror(errno));
    return;
  }
  out[n] = '\0';
  char *nl = strchr(out, '\n');
  if (nl) *nl = '\0';
}

static void try_open(const char *node, int flags, const char *label) {
  int fd = open(node, flags);
  if (fd >= 0) {
    printf("OPEN %s %s: OK fd=%d\n", node, label, fd);
    close(fd);
  } else {
    printf("OPEN %s %s: FAIL errno=%d(%s)\n", node, label, errno,
           strerror(errno));
  }
}

int main(void) {
  char buf[512];
  printf("PROBE-BEGIN uid=%d\n", (int)getuid());
  printf("NODE-EXISTS /dev/mali0: %s\n",
         access("/dev/mali0", F_OK) == 0 ? "yes" : "no");
  try_open("/dev/mali0", O_RDONLY, "O_RDONLY");
  try_open("/dev/mali0", O_RDWR, "O_RDWR");
  read_first_line("/sys/class/misc/mali0/device/gpuinfo", buf, sizeof buf);
  printf("GPUINFO: %s\n", buf);
  read_first_line("/sys/class/misc/mali0/device/core_mask", buf, sizeof buf);
  printf("CORE_MASK: %s\n", buf);
  read_first_line("/proc/version", buf, sizeof buf);
  printf("KERNEL: %s\n", buf);
  printf("PROBE-END\n");
  return 0;
}
