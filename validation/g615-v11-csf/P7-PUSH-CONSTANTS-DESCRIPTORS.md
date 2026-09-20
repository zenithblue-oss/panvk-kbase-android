# P7 Push Constants And Push Descriptors

## Result

`PASS` for valid functionality. No PanVK defect was found; no driver capability
or implementation code changed.

- `maxPushConstantsSize`: `256` (`PASS`)
- `VK_KHR_push_descriptor`: advertised and callable (`PASS`)
- `maxPushDescriptors`: `32` (`PASS`)
- Graphics and compute value verification: `PASS`
- Above-256 validation: `BLOCKED` (Khronos validation layer unavailable)
- Vulkan CTS: `BLOCKED` (`deqp-vk` and CTS test list unavailable)

## Source Audit

Pinned Mesa `5a07217f034b3e50d8c7c7794f97a2df1742613b` and audited upstream
`e1f3f372c4a661cd0e71a0cdafc4d469d56ecf35` contain the same P7 paths:

- `panvk_vX_physical_device.c`: advertises `maxPushConstantsSize=256`,
  `VK_KHR_push_descriptor`, and `maxPushDescriptors=MAX_PUSH_DESCS` (32).
- `panvk_vX_shader.c`: reserves 32 64-bit FAUs (256 bytes), tracks every used
  push-constant range, packs direct loads, and lowers dynamic loads through the
  push-uniform backing address.
- `panvk_vX_cmd_push_constant.c`: stores 256 command-buffer bytes and packs the
  exact per-stage used FAUs.
- `panvk_vX_cmd_desc_state.c`: maintains distinct graphics/compute push sets,
  copies descriptor records into command-buffer state, supports partial
  replacement, and uploads descriptor state before execution.

No newer upstream implementation or applicable backport exists.

## Focused Workload

`tests/dxvk-vkd3d/push-constants-descriptors.c` directly loads the tested ICD.
Its shaders verify actual values rather than pipeline creation alone.

| Case | Result |
|---|---|
| Push first 128 bytes | PASS |
| Extend values through 192 bytes | PASS |
| Extend through byte 251, then verify byte 255 with aligned 4-byte update | PASS |
| Push/read all 256 bytes | PASS |
| Vertex, fragment, and compute stage values | PASS |
| Push descriptor array boundary at 32 storage buffers | PASS |
| Graphics push descriptor update | PASS |
| Compute push descriptor update | PASS |
| Partial replacement of array element 31 | PASS |
| Earlier elements survive partial replacement | PASS |
| Descriptor-info source storage overwritten after recording | PASS |

The Vulkan API requires push-constant `offset` and `size` to be multiples of
four. A literal 255-byte update is therefore invalid; the workload reaches and
checks byte 255 using the valid aligned `[252, 256)` range. A 260-byte range is
above the reported limit and invalid. It was not submitted directly to the ICD,
because behavior after violating valid usage is undefined and cannot prove
rejection. Validation-layer proof remains explicitly blocked.

## Device Evidence

Device: Poco X6 Pro (`duchamp`), Mali-G615 MC6, serial
`Y5WWBMJVOZSK4HU8`.

```text
device=Mali-G615 MC6 maxPushConstantsSize=256 VK_KHR_push_descriptor=true maxPushDescriptors=32
push-constant boundaries: 128, 192, byte-255, 256 PASS
push-descriptor compute/graphics 32, replacement, record-lifetime PASS
invalid boundaries: size=255 requires 4-byte alignment; size=260 exceeds reported limit
```

Commands:

```sh
./tests/dxvk-vkd3d/build-push-constants-descriptors.sh
adb -s Y5WWBMJVOZSK4HU8 push build/push-constants-descriptors/push-constants-descriptors-android /data/local/tmp/panvk-p7
adb -s Y5WWBMJVOZSK4HU8 push dist/android-g615-v11-csf/libvulkan_panfrost.so /data/local/tmp/libvulkan_panfrost.so
adb -s Y5WWBMJVOZSK4HU8 shell 'timeout 60 /data/local/tmp/panvk-p7 /data/local/tmp/libvulkan_panfrost.so'
```

The build validates all three shaders with `spirv-val`, then compiles host
x86-64 and Android AArch64 binaries with `-Wall -Wextra -Werror`.

## Blockers

- No `VK_LAYER_KHRONOS_validation` binary/manifest exists in the workspace,
  host Vulkan layer directories, or target device. Therefore VUID validation
  for a 255-byte unaligned size and a 260-byte over-limit range is `BLOCKED`.
- No `deqp-vk` binary or CTS test list exists on host or device. CTS version,
  test-list hash, pass/fail/skip/crash/device-lost counts are
  `NOT_TESTED/BLOCKED`.

Relevant CTS, when supplied, should cover push-constant limits/stages and
`dEQP-VK.api.*push_descriptor*`; exact case names must come from that supplied
CTS test list rather than be guessed.

## Exit

P7 exits `PASS_WITH_BLOCKED_VALIDATION_CTS`: all valid required G615 behavior
is proven at 256 bytes and 32 push descriptors. No fake exposure or reporting
change was made. P8 was not started.
