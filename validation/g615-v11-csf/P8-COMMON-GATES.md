# P8 Common DXVK/vkd3d Gates

## Result

`PASS_WITH_BLOCKED_CTS`

All eleven P8 gates are exposed by the pinned Mesa and passed a focused direct-ICD workload on the G615. No PanVK defect was found. No driver patch or feature-reporting change was made.

Tested device: Poco X6 Pro (`duchamp`), Mali-G615 MC6, serial `Y5WWBMJVOZSK4HU8`. Mesa remains pinned at `5a07217f034b3e50d8c7c7794f97a2df1742613b`. The P3 audit found no relevant post-pin upstream change through `e1f3f372c4a661cd0e71a0cdafc4d469d56ecf35`.

## Gate Matrix

| Gate | Upstream PanVK status | Runtime bit/extension | Focused executable semantics | CTS subset status |
|---|---|---|---|---|
| `samplerMirrorClampToEdge` | Implemented by `c45d05b1e4f`; present pinned/current | Vulkan 1.2 bit `true`; `VK_KHR_sampler_mirror_clamp_to_edge` advertised | Sampled a two-texel image outside both U boundaries with `VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE`; expected edge texel returned | `BLOCKED`: sampler addressing subset unavailable |
| `shaderDrawParameters` | Implemented by `963e9feb8a8`; present pinned/current | Vulkan 1.1 bit `true`; `VK_KHR_shader_draw_parameters` advertised | Vertex shader stored `gl_BaseInstanceARB`; `vkCmdDraw(... firstInstance=37)` produced `37` | `BLOCKED`: draw-parameters subset unavailable |
| `storageBuffer8BitAccess` | Implemented with `2cac1389e57`; present pinned/current | Vulkan 1.2 bit `true`; `VK_KHR_8bit_storage` advertised | Compute loaded signed 8-bit SSBO value `7`, performed arithmetic, stored `12` through an 8-bit SSBO element | `BLOCKED`: 8-bit storage subset unavailable |
| `shaderInt8` | Implemented with `2cac1389e57`; present pinned/current | Vulkan 1.2 bit `true`; `VK_KHR_shader_float16_int8` advertised | Same compute shader executed native SPIR-V `Int8` arithmetic and contributed the value to a 32-bit result | `BLOCKED`: shader int8 subset unavailable |
| `storageBuffer16BitAccess` | Implemented by `eb84f1e61fd`; present pinned/current | Vulkan 1.1 bit `true`; `VK_KHR_16bit_storage` advertised | Compute loaded signed 16-bit SSBO value `300`, performed arithmetic, stored `309` through a 16-bit SSBO element | `BLOCKED`: 16-bit storage subset unavailable |
| `fragmentStoresAndAtomics` | Implemented by `9c9f5f69fd1`; present pinned/current | Core feature `true` | Fragment shader `atomicAdd` updated an SSBO only for the unclipped draw | `BLOCKED`: fragment stores/atomics subset unavailable |
| `depthClipEnable` | Implemented by `b63dab29f09`; present pinned/current | `VK_EXT_depth_clip_enable` advertised; feature `true` | Pipeline used `depthClipEnable=VK_TRUE`; triangle at Z=2 generated zero fragment atomics, then Z=0 generated fragments | `BLOCKED`: depth-clip subset unavailable |
| `maintenance5` | Upstream Vulkan runtime/PanVK support present pinned/current | `VK_KHR_maintenance5` advertised; Vulkan 1.4 bit `true` | Compute and graphics pipelines used `VkPipelineCreateFlags2CreateInfo` | `BLOCKED`: maintenance5 subset unavailable |
| `maintenance6` | Upstream Vulkan runtime/PanVK support present pinned/current | `VK_KHR_maintenance6` advertised; Vulkan 1.4 bit `true` | Real dispatch/draw descriptor binding used `vkCmdBindDescriptorSets2`; compute push data used `vkCmdPushConstants2` | `BLOCKED`: maintenance6 subset unavailable |
| `load_store_op_none` | Implemented by `b69dcd8d33f`; present pinned/current | `VK_KHR_load_store_op_none` advertised | Dynamic rendering used `VK_ATTACHMENT_LOAD_OP_NONE` and `VK_ATTACHMENT_STORE_OP_NONE` for an actual draw | `BLOCKED`: load/store-op-none subset unavailable |
| `dynamicRenderingUnusedAttachments` | Implemented by `aed60946a11`; present pinned/current | `VK_EXT_dynamic_rendering_unused_attachments` advertised; feature `true` | Pipeline declared two color formats; rendering supplied one valid view plus one `VK_NULL_HANDLE` unused attachment while fragment work completed | `BLOCKED`: dynamic-rendering-unused-attachments subset unavailable |

The `maintenance5`/`maintenance6` history predates or is folded into the pinned snapshot; no newer coherent backport exists. Pinned and audited current upstream expose identical relevant paths in `panvk_vX_physical_device.c` and Mesa Vulkan runtime.

## Workload

`tests/dxvk-vkd3d/p8-common-gates.c` loads `libvulkan_panfrost.so` through `vk_icdGetInstanceProcAddr`. Its four shaders and command streams combine related gates without reducing them to property queries:

- Compute: 8-bit and 16-bit SSBO load/arithmetic/store, `shaderInt8`, maintenance5 pipeline flags, maintenance6 descriptor/push commands.
- Sampling: mirror-clamp edge selection from real image data.
- Graphics: base-instance readback, fragment SSBO atomic, positive depth-clip control, load/store-op-none, and a null unused dynamic-rendering attachment.

Device output is stored in `p8-common-gates-2026-09-20.txt`.

## Verification

- `build-p8-common-gates.sh`: all shaders compiled; `spirv-val --target-env vulkan1.2` passed; host x86-64 and Android AArch64 C builds passed with `-Wall -Wextra -Werror`.
- Focused P8 workload: all eleven gates passed against the rebuilt Android ICD.
- Android PanVK build: passed.
- Clean glibc PanVK build: passed with `ANDROID_SERIAL=Y5WWBMJVOZSK4HU8`.
- P5 descriptor stress build: passed.
- P6 robustness2 build and G615 workload: passed.
- P7 push constants/descriptors build and G615 workload: passed.
- Consumer/runtime Python tests, BC validation, package self-check, patch-series verification, and `git diff --check`: passed.
- Patch series remains `sha256:c0bbdeef591b206a2f3ae36dc6191c08854399039075f8d69f103e33c0eca2f8`.

## CTS Blocker

No `deqp-vk` executable or matching CTS test list exists on the host or G615 device. Therefore the CTS version, test-list hash, pass/fail/skip/crash/device-lost counts, and each relevant subset are `NOT_TESTED/BLOCKED`. No Vulkan conformance claim is made.

## Exit

P8 exits `PASS_WITH_BLOCKED_CTS`. Runtime exposure and focused semantics pass for every requested gate. No fake exposure, PanVK patch, or P9 work was added.
