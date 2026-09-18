// panvk-kbase-probe: pre-Vulkan hardware/Kbase probe (Gate B input).
// Captures: SoC/GPU model+ID, pan arch, /dev/mali0 existence + O_RDONLY/O_RDWR
// + errno (ROOT/SHELL and target-app UID), Kbase UAPI, DDK/module version if
// safely readable, JM/CSF, shader-core mask, coherency, memory, CSF props.
// Usage (NDK): $CC -o panvk-kbase-probe probe.c && adb push + run.
// TODO P2: implement ioctl discovery against observed UAPI; never guess structs.
#include <stdio.h>
int main(void) {
  printf("panvk-kbase-probe: STUB (P2 implements full probe)\n");
  return 2;
}
