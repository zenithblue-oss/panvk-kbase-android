# 122 robustBufferAccess2 on v10

Patch: `patches/csf-v11/122-advertise-robustbufferaccess2-on-v10.patch`. This is the first beta.18 item (PROGRESS item 33).

Status: in the beta.18 series.
- Proven on a pre-v11 proxy: Mali-G57 (v9), with a local build that forces the feature on.
- The G615 regression check was clean on the gate-only build.
- v10 (G610 MC3, manaus, Firebase): PanProbe 36/36 on the dev build, then 37/37 on beta.18-rc1.

## What

1. `robustBufferAccess2` is now advertised for `PAN_ARCH >= 10` instead of `>= 11` (`src/panfrost/vulkan/panvk_vX_physical_device.c:651` in the beta.18 tree). `VK_EXT_robustness2` and `nullDescriptor` were already on at `>= 10`. `robustImageAccess2` stays false. `VK_KHR_robustness2` stays hidden by `kbase-common/017`.
2. Texel buffer accesses on v9/v10 get software bounds checks when robust buffer access is on. Without them the feature would be wrong on v10 (see the proxy results below). This step adds:
   - `bi_lower_robust_texel_buf` in `bifrost_nir.c`, run at the start of `bifrost_postprocess_nir` for arch 9 and 10 only. It wraps texel buffer `image_load`, `image_store`, `image_atomic`, `image_atomic_swap` and `txf` in `if (index < size / stride)`, using the Buffer descriptor words 1 and 4.
     - Storage texel buffers follow `robust_modes & nir_var_mem_ssbo`, and uniform texel buffers follow `nir_var_mem_ubo`. This matches the Vulkan pipeline robustness grouping.
     - Out-of-bounds loads return 0, with alpha 1 when the view format has no alpha. Atomics return 0. Stores are dropped.
   - `PAN_VA_BUF_CVT_NO_ALPHA` (bit 31 of descriptor dword 7, in `pan_compiler.h`). The format is not known at compile time, so `pan_buffer_texture_emit` sets this bit for v9/v10 when `!util_format_has_alpha`. Dword 7 holds the conversion for software only (the genxml says the hardware does not interpret it).
   - `pan_nir_load_va_buf_cvt` takes the arch and masks the bit off before the conversion reaches `LD_CVT`/`ST_CVT`. Both callers pass the arch.
   - Null descriptors have size 0, so their accesses take the out-of-bounds path and read 0.

v11+ is unchanged. The new pass is gated to arch 9/10, the mask to arch < 11, and the descriptor bit to `PAN_ARCH < 11`. v9 (JM) still lacks `VK_EXT_robustness2` in the shipped driver, so on v9 the checks only run with core `robustBufferAccess`. That is harmless, and it also makes core texel buffer robustness correct there.

## Why

Bachata S4 requires `robustBufferAccess2`. It was the only hard Bachata requirement that v10 missed ([requirements](../../docs/bachata-s4-vulkan-requirements.md)). DXVK also enables it when present.

## Where v11 matters, and where it does not

The gate comes from upstream Mesa at our pin. The shallow clone has no history, so the upstream reason is not recorded.

The following parts do not depend on v11:
- SSBO and UBO loads compile to `LD_PKA` on Valhall (`bi_load_ubo_to` and the `load_ssbo` case in `bifrost_compile.c`). The hardware checks them against the Buffer descriptor. Its `Size` field is the same in the v9, v10 and v11 genxml (v11 adds `Size hi` for buffers above 4 GiB).
- SSBO stores and atomics are lowered to global stores. They get software checks from `nir_lower_robust_access` (`panvk_vX_shader.c:2801`), which has no arch gate.
- The UBO descriptor index is clamped. The robust size alignments (4 for storage buffers, 16 for uniform buffers) are the same on every arch.

Texel buffers do depend on v11. On Valhall they go through `LEA_BUF` (`lower_buf_image_access` and `va_lower_txf_buf`) and then `LD_CVT`/`ST_CVT` on the computed address. There is no software check. On v9 the hardware does not bound the index either: the proxy run reads garbage out of bounds. This is the likely reason upstream gated the feature to v11.

## Proxy test on v9 (Mali-G57, 2026-10-07)

The tablet has a Mali-G57 MC2, which is v9 JM. It has the same `LD_PKA`/`LEA_PKA`/`LEA_BUF` ISA and Buffer descriptor as v10.

A local test-only build also turns on `VK_EXT_robustness2` and `robustBufferAccess2` for v9. That flip is never committed. The build tree is `/var/tmp/panvk/wt-beta18-v9test`, and the output is `/var/tmp/panvk/dist-beta18-v9test/libvulkan_panfrost.so`.

Tests ran headless (bionic `deqp-vk` through a shim that loads that ICD, plus the PanProbe `robustness2` native binary). Nothing on screen was touched.

Case lists are in `/var/tmp/panvk/r2test/lists/`:
- `r2buf-v9sub.txt`: 7,823 robustness2 buffer cases, every non-image descriptor type, bind and push, without null descriptors since JM has no `nullDescriptor`.
- `buffer_access.txt`: 1,212 cases.

| Build | robustness2 buffer subset (7,823) | `buffer_access` (1,212) |
|---|---|---|
| gate only (first 122) | 1,982 pass / 342 fail / 5,499 NotSupported | 808 pass / 0 fail / 404 NotSupported |
| gate + texel checks (final 122) | 2,324 pass / 0 fail / 5,499 NotSupported | 808 pass / 0 fail / 404 NotSupported |

With the gate only:
- Every `storage_buffer`, `uniform_buffer` and dynamic case passed.
- Every texel buffer case failed: `storage_texel_buffer` 228 and `uniform_texel_buffer` 114, across r32, rg32 and rgba32 in f, i and ui.
- The native `robustness2` binary passed:
  - `oob_ssbo_load` and `oob_ubo_load`. The backing memory is filled with `0xABABABAB`, so a pass means that the OOB words read 0.
  - `oob_ssbo_store` (an OOB `atomicAdd` returns 0).

With the texel checks:
- All 342 texel cases pass (1,356-case texel-only list: 342 pass / 1,014 NotSupported).
- The full subset and `buffer_access` have no failures.
- The native `robustness2` binary still passes `oob_ssbo_load`, `oob_ubo_load` and `oob_ssbo_store`. The null descriptor cases are skipped, because JM has no `nullDescriptor`.

Results:
- `/var/tmp/panvk/r2test/g57/r2sub-status.txt`: gate only.
- `/var/tmp/panvk/r2test/g57/r2sub2-status.txt`: final.
- `/var/tmp/panvk/r2test/g57/ba*-status.txt`.

## G615 regression (Poco X6 Pro, Mali-G615 MC6, v11, 2026-10-07)

This run used the gate-only beta.18-dev build: Mesa 5a07217f plus the full series (128 patches) with 122 included. The final 122 does not change v11 code (see above).

- PanProbe `robustness2` native binary, beta.17 and beta.18-dev: identical results.
  - `robustBufferAccess2=1`, `nullDescriptor=1`, alignments 4/16.
  - `null_descriptors`, `oob_ssbo_load`, `oob_ubo_load`, `oob_ssbo_store` and `null_vertex_buffer` pass.
- PanProbe 1.2.3 single-test autoruns: `robustness2`, `bachata_reqs` and `bachata_exec` pass on both builds. Bachata S4 compliance passes (114 items, no hard missing).
- PanProbe 1.2.3 `autorun all` with beta.18-dev imported: 36/36. The upload ran in dry-run mode, so nothing was sent. The imported driver and driver choice were restored afterwards.
- CTS (chroot, glibc ICD), 48,567 cases: robustness2 buffer cases (including null descriptors), `buffer_access.*`, `pipeline_robustness.*` and `pipeline_robustness_buffer_access.*`.

| Build | Pass | Fail | NotSupported |
|---|---|---|---|
| beta.17 | 23,824 | 12 | 24,731 |
| beta.18-dev (gate only) | 23,824 | 12 | 24,731 |

Per-case results are identical. Both builds have the same 12 failures: `pipeline_robustness.image_robustness.{bind,push}.notemplate.{r32ui,rgba32f}.*.sampled_image.*.{1d,2d,2d_array}.frag_fast_gpl`. They are image robustness cases with fast-linked pipeline libraries, and they predate this patch. Status files: `/var/tmp/panvk/r2test/g615/{b18rob,b17drob}-status.txt`.

The G615 was not re-run on the final build: the user was using the phone. The v11 shader and descriptor output does not change, because every new path is gated below v11.

Final beta.18-dev builds (both pass `validate-binary.sh`; driverInfo still says beta.17, and 099 is bumped at release):

| Build | SHA256 | BuildID |
|---|---|---|
| Android ICD `/var/tmp/panvk/dist-beta18-dev/libvulkan_panfrost.so` | `d4d04985e1030c03ffa466ab318a9ab204eed016752712a47ae6f006a2dc6e94` | `3457127554d6c62629244b732a32c583b5b181f7` |
| glibc ICD `/var/tmp/panvk/dist-beta18-dev/glibc/libvulkan_panfrost.so` | `979e909d53ae802fad7eaf690f4ba5390ef02d6146d8079edbed532c8cb76f18` | `5bac3a50f3e2c812d14923fe0b977b8b723dcdc2` |

## Open

- **G610 hardware confirmation.** This needs a v10 tester:
  - Run PanProbe `robustness2` and look for `robustBufferAccess2=1` and the `oob_*` cases.
  - If the tester can run CTS, also run the `dEQP-VK.robustness.robustness2.*` buffer and texel buffer cases.
- G615 re-run on the final build (CTS buffer list and PanProbe), when the phone is free.
- Performance: with robustness on, every texel buffer access on v9/v10 now costs two descriptor loads and an integer divide. Atomics also cost a branch. DXVK uses texel buffers for typed UAVs and SRVs, so watch v10 game numbers.
- 64-bit image atomics on texel buffers go through `pan_nir_lower_image_64bit` first. The check does not cover any form that pass rewrites. This was not tested.
- Image robustness: the 12 `frag_fast_gpl` sampled-image failures on v11 are a separate, pre-existing issue.
