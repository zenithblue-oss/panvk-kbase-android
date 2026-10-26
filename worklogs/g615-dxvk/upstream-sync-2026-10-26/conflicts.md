# Rebase conflict estimate onto upstream main (2026-10-26)

Base `5a07217f` (2026-09-18) -> upstream main `f7dd61f7` (2026-10-26 15:35 UTC, VERSION 27.0.0-devel): 2538 commits, 229 in `src/panfrost`, 53 in `src/panfrost/vulkan`, 12 in `src/panfrost/lib/kmod`, 147 in `src/panfrost/compiler`.

**Method (estimate, not a trial apply).** For every patch, the files it changes are intersected with the files upstream changed between the base and main (`git diff --numstat 5a07217f f7dd61f7`). Risk is the upstream churn (added+removed lines) in the overlapping files: none = 0, low <= 30, medium <= 300, high > 300. This over-counts (upstream churn in a big file may not touch our hunks) and under-counts semantic breakage (the kmod API changed, so the kbase backend can apply cleanly and still not build). A real `git am -3` trial was skipped on request.

## Per family

| family | patches | none | low | medium | high | classes |
|---|---|---|---|---|---|---|
| common | 1 | 0 | 1 | 0 | 0 | fix 1 |
| android | 5 | 3 | 0 | 0 | 2 | android-wsi 5 |
| kbase-common | 10 | 0 | 2 | 5 | 3 | kbase 8, superseded 1, android-wsi 1 |
| app-loader | 1 | 0 | 1 | 0 | 0 | android-wsi 1 |
| wsi | 2 | 1 | 0 | 1 | 0 | android-wsi 2 |
| csf | 2 | 0 | 0 | 2 | 0 | kbase 2 |
| csf-v11 | 171 | 12 | 29 | 86 | 44 | feature 54, workaround 24, perf 22, fix 20, kbase 17, upstream 12, android-wsi 12, debug 8, superseded 2 |
| jm-v9 | 31 | 4 | 5 | 18 | 4 | feature 13, kbase 7, fix 6, workaround 2, perf 2, debug 1 |
| jm-v7 | 5 | 3 | 1 | 0 | 1 | kbase 3, fix 1, policy 1 |
| jm-v7-wip | 87 | 11 | 4 | 65 | 7 | feature 33, fix 21, perf 13, workaround 11, kbase 8, debug 1 |
| bcn-layer | 1 | 1 | 0 | 0 | 0 | android-wsi 1 |
| staging-115/pa | 10 | 2 | 1 | 7 | 0 | fix 3, perf 3, feature 2, debug 1, kbase 1 |
| staging-115/pb | 10 | 0 | 0 | 3 | 7 | perf 5, kbase 3, fix 1, feature 1 |
| staging-115/pc | 7 | 1 | 3 | 2 | 1 | android-wsi 5, fix 1, perf 1 |
| staging-115/int | 1 | 0 | 0 | 1 | 0 | debug 1 |
| staging-115/pd | 7 | 1 | 0 | 3 | 3 | perf 5, kbase 2 |

Total: 351 patches; risk none 39, low 47, medium 193, high 72, n/a 0.

Classes overall: feature 103, fix 54, kbase 51, perf 51, workaround 37, android-wsi 27, upstream 12, debug 12, superseded 3, policy 1.

## Hotspot files (touched by many of our patches and changed upstream)

| our patches touching it | upstream churn (lines) | file |
|---|---|---|
| 68 | 45 | `src/panfrost/vulkan/panvk_vX_shader.c` |
| 62 | 42 | `src/panfrost/vulkan/panvk_vX_physical_device.c` |
| 61 | 68 | `src/panfrost/vulkan/jm/panvk_vX_cmd_draw.c` |
| 58 | 29 | `src/panfrost/vulkan/csf/panvk_vX_cmd_draw.c` |
| 41 | 203 | `src/panfrost/vulkan/panvk_vX_device.c` |
| 33 | 2 | `src/panfrost/vulkan/panvk_cmd_draw.h` |
| 30 | 102 | `src/panfrost/vulkan/panvk_device.h` |
| 30 | 34 | `src/panfrost/libpan/draw_helper.cl` |
| 27 | 11 | `src/panfrost/vulkan/panvk_instance.c` |
| 25 | 480 | `src/panfrost/vulkan/panvk_physical_device.c` |
| 25 | 62 | `src/panfrost/vulkan/csf/panvk_vX_gpu_queue.c` |
| 23 | 47 | `src/panfrost/vulkan/csf/panvk_vX_cmd_buffer.c` |
| 23 | 229 | `src/panfrost/vulkan/panvk_image.c` |
| 23 | 8 | `src/panfrost/vulkan/panvk_vX_cmd_meta.c` |
| 16 | 22 | `src/panfrost/vulkan/jm/panvk_vX_gpu_queue.c` |
| 15 | 3 | `src/panfrost/vulkan/csf/panvk_cmd_buffer.h` |
| 15 | 38 | `src/panfrost/vulkan/panvk_vX_cmd_draw.c` |
| 13 | 2 | `src/panfrost/vulkan/panvk_instance.h` |
| 13 | 38 | `src/panfrost/vulkan/jm/panvk_vX_cmd_buffer.c` |
| 12 | 6 | `src/panfrost/vulkan/jm/panvk_cmd_buffer.h` |
| 11 | 24 | `src/panfrost/vulkan/panvk_image.h` |
| 9 | 71 | `src/panfrost/vulkan/panvk_vX_nir_lower_descriptors.c` |
| 8 | 12 | `src/panfrost/vulkan/panvk_physical_device.h` |
| 8 | 253 | `src/vulkan/wsi/wsi_common_x11.c` |
| 7 | 6 | `src/panfrost/libpan/draw_helper.h` |

