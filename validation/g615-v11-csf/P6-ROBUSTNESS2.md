# P6 Robustness2

## Result

`PARTIAL`

- `VK_EXT_robustness2`: `PASS` (advertised)
- `robustBufferAccess2`: `PASS`
- `nullDescriptor`: `PASS`
- `robustImageAccess2`: `UNSUPPORTED` (reported `false`)
- Core Vulkan 1.3 `robustImageAccess`: `PASS`
- Vulkan CTS: `BLOCKED` (no `deqp-vk` binary or CTS test list available)

No PanVK patch was required. The focused G615 workload found no defect after
separating null storage-buffer read and write into independent dispatches.
Pinned Mesa already contains upstream commits `9f93ade4572` (PanVK robust SSBO
bounds checks) and `6e3c92cd63c` (discard OOB buffer stores). Current upstream
at `e1f3f372c4a661cd0e71a0cdafc4d469d56ecf35` has the same relevant PanVK
implementation and still reports `robustImageAccess2=false`.

## Proven Cases

All cases ran through direct `vk_icdGetInstanceProcAddr` dispatch against
Android ICD SHA-256
`576e37de9a3b60dda9a972a6f91c3a50e09238bd31a9888e04215c7b554c803a` on
Poco X6 Pro (`duchamp`), Mali-G615 MC6, serial `Y5WWBMJVOZSK4HU8`.

| Case | Result |
|---|---|
| OOB uniform buffer read | PASS: zero |
| OOB storage buffer read | PASS: zero |
| OOB storage buffer write | PASS: discarded; sentinel unchanged |
| OOB uniform texel buffer read | PASS: zero |
| OOB storage texel buffer write/read | PASS: write discarded; read zero; sentinel unchanged |
| OOB storage image write/read | PASS under `robustImageAccess`: write discarded; read zero; sentinel unchanged |
| Null sampled image | PASS: zero |
| Null storage image | PASS: write discarded; read zero |
| Null sampler | NOT_APPLICABLE: Vulkan `nullDescriptor` does not permit null sampler handles; isolated diagnostic returned the valid image's value |
| Null uniform buffer | PASS: zero |
| Null storage buffer read | PASS: zero |
| Null storage buffer write | PASS: discarded; sentinel unchanged |
| Null uniform texel buffer | PASS: zero |
| Null storage texel buffer | PASS: write discarded; read zero |
| Nonuniform descriptor indexing plus null descriptor | PASS: zero |
| UpdateAfterBind plus null descriptor | PASS: zero |

## Commands

```sh
./tests/dxvk-vkd3d/build-robustness2.sh
adb -s Y5WWBMJVOZSK4HU8 push build/robustness2/robustness2-android /data/local/tmp/panvk-robustness2
adb -s Y5WWBMJVOZSK4HU8 push dist/android-g615-v11-csf/libvulkan_panfrost.so /data/local/tmp/libvulkan_panfrost.so
adb -s Y5WWBMJVOZSK4HU8 shell 'timeout 60 /data/local/tmp/panvk-robustness2 /data/local/tmp/libvulkan_panfrost.so'
```

The build runs `glslangValidator --target-env vulkan1.2`,
`spirv-val --target-env vulkan1.2`, host C compilation, and Android AArch64 C
compilation with `-Wall -Wextra -Werror`.

CTS command when a matching Vulkan CTS build is supplied:

```sh
adb -s Y5WWBMJVOZSK4HU8 shell 'timeout 1800 /data/local/tmp/deqp-vk --deqp-case=dEQP-VK.robustness.* --deqp-log-filename=/data/local/tmp/p6-robustness.qpa'
```

CTS version, test-list hash, pass/fail/skip/crash/device-lost counts:
`NOT_TESTED/BLOCKED`, because no CTS binary or test list is installed.
