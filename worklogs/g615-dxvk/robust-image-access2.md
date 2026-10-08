# robustImageAccess2 (csf-v11/181)

Date: 2026-10-08. Goal: vkd3d-proton D3D12CreateDevice on PanVK. It failed with `Robustness2 features not supported. This is required.` (vkd3d-proton 22307558 `libs/vkd3d/device.c:2639`), because `robustImageAccess2` was hard-coded `false`.

## Upstream

There was nothing to backport. Mesa main `1375dc60642` (fetched 2026-10-08) still has `.robustImageAccess2 = false` in `src/panfrost/vulkan/panvk_vX_physical_device.c:634`. `git log -S robustImageAccess2 -- src/panfrost` finds no PanVK change. The GitLab MR search found no PanVK MR (only kk, v3dv and dzn).

## Hardware analysis (no shader emulation)

The Valhall image path already bounds-checks every access in hardware. Nothing in the shader changes:
- Storage image loads use `LD_TEX`. Stores, atomics (`nir_lower_image_atomics_to_global`) and R64 accesses use `LEA_TEX` plus a global access (`pan_nir_lower_image.c`, `pan_nir_lower_image_64bit.c`). Both instructions check every coordinate against the texture descriptor. `pack_image_coords` folds the upper 16 coordinate bits into an unused coordinate, so aliased coordinates (65536+) also fail the check. This is upstream code that runs on every arch.
- `texelFetch` uses `TEX_FETCH` on the same descriptor, with the x/y/layer/LOD check.
- Out-of-bounds reads return zero in the format's components, and the descriptor swizzle fills the rest: R32 reads `(0,0,0,1)` and RGBA8 reads `(0,0,0,0)`. That is the robustImageAccess2 rule. Out-of-bounds stores and atomics are dropped, and the atomics return 0.
- Texel buffers count as buffer robustness. v11 checks them in hardware. v10 has the 122 software checks (robustBufferAccess2).

Proof: the beta.18-rc1 .so (feature still off, only `robustImageAccess` on) already passed every case of the new probe on G615 (45/45, with only "advertised=0" failing). So 181 is only the feature bit.

Arch gate: `PAN_ARCH >= 10`, the same as `EXT_robustness2` and `nullDescriptor`. v10 uses the same `LD_TEX`/`LEA_TEX`/`TEX_FETCH` path, but has no hardware run yet (no G610 device). v12/v13 have not been tested either. `defaultRobustnessImages` stays `ROBUST_IMAGE_ACCESS`.

The previous agent's partial work: a clean applied tree (`/var/tmp/panvk/wt-ria2`) and `tests/dxvk/vulkan/robust-image2/ri2.comp`. It had no shader-emulation edits. The shader was kept. `work/mesa` has an unrelated uncommitted `kbase_kmod.c` diff from another task, which was left alone.

## PanProbe

- New test `robust_image_access2` (`tests/dxvk/vulkan/robust-image2/robust_image2.c`, `ri2.comp`, `build_spv.sh`). It runs on R32_UINT and RGBA8_UNORM and checks:
  - imageLoad out of bounds in x, y and layer, with negative x, and with x and layer aliased at 65536.
  - texelFetch out of bounds in LOD, in x at LOD 1, in layer, with negative y, and at LOD 31.
  - Texel buffer out of bounds, including an aliased index.
  - imageStore out of bounds: the image is read back and every texel must be unchanged.
  - imageAtomicAdd/Exchange/CompSwap/Max out of bounds: each returns 0 and nothing is written.
  - An in-bounds control for each group, plus a sanity check of the output buffer.
  - It FAILs when `robustImageAccess2` is not advertised.
- `vkd3d_reqs` passed while the bit was off, because every vkd3d-proton requirement was reported as soft ("PanPlay does not ship vkd3d-proton", which stopped being true at 4c4f039). The vkd3d-proton device creation checks are now HARD. They are the `device.c` 2495-2672 init checks plus the bindless init at `state.c:8351`. The "used, unchecked" items stay soft. With rc1 the result is now `FAIL hard proton: robustImageAccess2` and `RESULT FAIL`. With 181 the result is hard 24/24 and `proton_device_create ok`.
- PanProbe 1.2.4 (versionCode 12, debug cert 8b10fde8), bundled .so sha256 `dee2a3c5…`, BuildID `2ac2df3a`. `autorun all` on G615: **38/38 pass**, DXVK, S4 and vkd3d compliance all PASS. Screenshot: `/var/tmp/panvk/shots/panprobe-robust2.png`.

## CTS (G615, chroot glibc ICD, same tree)

List `/var/tmp/panvk/ria2/cts/ria2.txt`, 12,024 cases: every `robustness2` image case except null descriptors, plus `pipeline_robustness.image_robustness.*`.

| Subset | Pass | Fail | NotSupported |
|---|---|---|---|
| robustness2 image (11,880) | 4,332 | 0 | 7,548 (rgen, r64 no_fmt_qual, ...) |
| pipeline_robustness.image_robustness (144) | 72 | 12 | 60 |

- The 12 failures are `*.sampled_image.*.{1d,2d,2d_array}.frag_fast_gpl`. They are the same pre-existing set that beta.17 and beta.18-dev also fail (worklog 122).
- R64 `fmt_qual` cases pass, which covers the 64-bit image path.
- Status file: `/var/tmp/panvk/ria2/cts/status.txt`.

## D3D12 (PanPlay)

- Main was merged with `box64-option` (f457392) as instructed. PanPlay versionCode 22 bundles the 181 .so and is installed with `adb install -r`.
- `d3d12probe-x86_64.exe` (`run_exe` intent, default vkd3d-proton log level, which includes warn): `CreateDevice` returns hr=0 at FL11_0, 12_0, 12_1 and 12_2. The queue, allocator, command list and fence work, and the fence completed. The log has no `err:` lines.
- Logs: `/var/tmp/panvk/shots/d3d12probe-ria2-vkd3d.log`, `/var/tmp/panvk/shots/d3d12probe-ria2.png`.
- NFS UC shortcut `nfs` still launches on the merged build: the WowBox64 banner is in the run log, and the game reached the menu at about 50 FPS (`/var/tmp/panvk/shots/nfsuc-box64-merged.png`).

## Open

- v10 (G610), v12 and v13 hardware runs for `robust_image_access2`.
- driverInfo still says beta.18-rc1. Bump 099 at release.
