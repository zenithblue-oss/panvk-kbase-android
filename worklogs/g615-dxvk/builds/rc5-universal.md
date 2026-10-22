# RC5-universal build (v6, v7, v9, v10, v11 in one .so) — 2026-10-22

## Tree
- Mesa 5a07217f + `g615-v11-csf` series in apply-patches.sh order, one commit per patch:
  `common android kbase-common app-loader wsi csf csf-v11 jm-v9 jm-v7` = 214 commits
  (csf-v11 018-219, which includes the amended 209 (7b46f90), bc6 210-212, tess 213-216,
  bc6c 217-218 and the tess fix 219), then `git am` of `patches/jm-v7-wip` 0006-0077 = 72 commits.
- Worktree `/home/flux/src/mesa-rc5g` (branch `rc5g`, base tag `rc5g-base`).
  **HEAD = `3093e6a58e`**. `prepare-rc5-tree.sh --csf-max 219 --wip-max 0077` printed
  `OK base=214 jm-v7-wip=72 head=3093e6a5`, with no PATCH-DRIFT, WIP-DRIFT or offset lines.
- csf-v11 210-219 moved context in six jm-v7-wip patches. They were refreshed with `git am -3`
  (no conflicts, hunk headers only): 0010, 0013, 0030, 0035, 0052, 0072 (commit 05a4908).

## Driver
- Recipe: NDK r30.0.16248370, `--api 29`, `PANVK_KBASE_RELEASE=RC5-universal`, `PANVK_JOBS=4`
  (thermal NONE), fresh BDIR `/home/flux/build-rc5/bionic-r30-RC5-universal-f`,
  HOST_TOOLS `/home/flux/repos/panvk-kbase-android/build/host-tools/bin` (absolute), under the build lock.
- `/home/flux/build-out/2026-10-22-rc5-universal/libvulkan_panfrost.so`,
  30512184 bytes, sha256 `ab7eaa920f5d98c0390c99f4dabe0c2eb96649789fe7b2396564ffacedc4a5c4`.
- `strings`: `PanVK-kbase RC5-universal (Mesa 26.3.0-devel (git-3093e6a58e))`, `PANVK_GPU_PRERAST_ARENA`,
  `astc_sf`, `ria2`, `no_tess`, `panvk: v%u (Bifrost) support is experimental`.
  8 `libpanvk_v*.a` in the BDIR (v6, v7, v9-v14).

## APKs (aapt2 and embedded sha checked)
| APK | package | sha256 | embedded .so |
|---|---|---|---|
| `/home/flux/build-out/apks/panprobe-rc5u.apk` | `dev.zenithblue.panvktest.rc5u` (1.2.7, vc 15) | `3d757e99…4c46` | rc5 `ab7eaa92…a5c4` (= driver); rc4 `c395db09…c8a5` (RC4-lowmem) |
| `/home/flux/build-out/apks/panplay-rc5u.apk` | `dev.zenithblue.panvklauncher` (1.2.7, vc 33) | `377f7ef3…6b50f` | `ab7eaa92…a5c4` (= driver) |

- PanProbe has no MESA_SHADER_CACHE_DISABLE default (ebd14ee) and contains the Firebase game loop
  (`GAMELOOP scenario=N`; scenario 2 = rc5 only).
- PanPlay: built with `-PpanvkSo=<driver>`. Package name is unchanged, so it replaces an installed PanPlay.
  `bundled-driver.json` still pins and labels RC3 lowmem (the pin needs a published release sha), so
  the app's driver card shows RC3 text while the bundled .so is RC5-universal. Check the .so sha, not the label.

## Test plan
a. **X6 Pro (v11), wifi adb.**
   - PanProbe autorun all: expect 39/39 (bc6d level), vkd3d_heap with the cache ON.
   - PanPlay cube:
     `am start -n dev.zenithblue.panvklauncher/.MainActivity --es dev.zenithblue.panvklauncher.LAUNCH_SHORTCUT builtin-d3d11-arm64ec`
     and then `builtin-d3d9-i686` (debug build; Wine must not be running).
     `apps/panvk-launcher/tools/launch-shortcut.py --launch <id> --wait 25 --screenshot out.png --logs dir/` wraps it.
b. **Firebase starlte (Galaxy S9, G72, v6):** one run, rc5 only, `--scenario-numbers=2`.
c. **Firebase v7 (Mali-G52):** model `a31` (Galaxy A31, Helio P65 = Mali-G52 MC2), API 31
   (its only listed version). One run, scenario 2. Alternatives: `a06` (API 35, the RTL A06 model), `a12`, `a21`.
   The SoC can vary by region, so check the GPU line in the log.
d. **v9, v10:** no device.

## RC5-universal-105 (2026-10-22): FTL starlte fix
- FTL starlte RC5-universal (matrix-2kxz6gv4kwzhv) was LMK-killed in tess_perf: the v6/v7 nested-tess GS emulation allocated 768 BOs (768 MiB committed). See `device-runs/2026-10-22-ftl-v6-starlte-rc5u/results.md`.
- Fix: `patches/jm-v7-wip/0078` (jm-v7 105), mesa-rc5g HEAD `710db1f9`.
- .so `/home/flux/build-out/2026-10-22-rc5-universal-105/libvulkan_panfrost.so`, sha256 `3d1a588cf54856002acce5a2dca8d6c79f76b8a972bf8440073c3ae33aa5d356` (`PanVK-kbase RC5-universal-105 (Mesa 26.3.0-devel (git-710db1f998))`), BDIR `/home/flux/build-rc5/bionic-r30-RC5-universal-105`.
- APK `/home/flux/build-out/apks/panprobe-rc5u-105.apk`, sha256 `23dd2d23fcbaa48febdea8fa64bb312d94be076eccb85c6c5897049991be55e6`, package `dev.zenithblue.panvktest.rc5u` (same package as panprobe-rc5u), rc5 3d1a588c, rc4 c395db09.
- To rebuild the tree: `prepare-rc5-tree.sh --csf-max 219 --wip-max 0078`.
- PanPlay was not rebuilt.

## RC5-universal-106 (2026-10-22): second FTL starlte fix
- RC5-universal-105 (matrix 5002760495733809791) was still LMK-killed in tess_perf, with 0078 in effect. See `device-runs/2026-10-22-ftl-v6-starlte-rc5u-105/results.md`.
- Fix: `patches/jm-v7-wip/0079` (jm-v7 106), mesa-rc5g HEAD `f68abd4b`.
- .so `/home/flux/build-out/2026-10-22-rc5-universal-106/libvulkan_panfrost.so`, sha256 `1cb2c074bb1f234ca4fc3a5f1eb12eb048ba7ca74d93710a45351ffbea51c7e5`. The version string is `PanVK-kbase RC5-universal-106 (Mesa 26.3.0-devel (git-710db1f998))`; the build was made before the commit. BDIR `/home/flux/build-rc5/bionic-r30-RC5-universal-106`.
- APK `/home/flux/build-out/apks/panprobe-rc5u-106.apk`, sha256 `7356674b2481f39b9297a7c45e4b6f41e448e7a85f723339bdd84c6614b88010`, package `dev.zenithblue.panvktest.rc5u`, rc5 1cb2c074, rc4 c395db09. tess_perf now logs memory (`PanProbeTess`) and tears down each variant's device.
- To rebuild the tree: `prepare-rc5-tree.sh --csf-max 219 --wip-max 0079`.

## RC5-universal-107 (2026-10-22): third FTL starlte fix
- RC5-universal-106 (matrix 5926634178382505063) was LMK-killed while recording tess_perf gs=0 L=1: per-batch TLS/WLS times the 0079 batch splits. See `device-runs/2026-10-22-ftl-v6-starlte-rc5u-106/results.md`.
- Fix: `patches/jm-v7-wip/0080` (jm-v7 107), mesa-rc5g HEAD `28b0346c`. tess_perf now also has a LOWMEM guard (FAIL instead of a kill).
- .so `/home/flux/build-out/2026-10-22-rc5-universal-107/libvulkan_panfrost.so`, sha256 `9a6308c8be00f5ea58c59f9a399f329790fc7e2ee55a49a7544becc594eabbbd` (`PanVK-kbase RC5-universal-107 (Mesa 26.3.0-devel (git-28b0346c01))`), BDIR `/home/flux/build-rc5/bionic-r30-RC5-universal-107`.
- APK `/home/flux/build-out/apks/panprobe-rc5u-107.apk`, sha256 `0c3a8f5192301c30404d5f07aa2c19b3b784f099d19994b8cc302d185e84702d`, package `dev.zenithblue.panvktest.rc5u`, rc5 9a6308c8, rc4 c395db09.
- To rebuild the tree: `prepare-rc5-tree.sh --csf-max 219 --wip-max 0080`.
