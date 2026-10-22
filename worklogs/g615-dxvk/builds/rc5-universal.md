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

## RC5-universal-108 (2026-10-22): FTL a31 (v7) fill_mode and GPU timestamps
- RC5-universal-107 on the FTL Galaxy A31 (Mali-G52 MC2, v7) gave 38/39. fill_mode failed because a LINE pipeline reused the FILL pipeline's IDVS VS binary from the shader cache. bc_perf GPU timestamps were 2x the wall time. See `device-runs/2026-10-22-ftl-v7-a31-rc5u-107/results.md`.
- Fixes:
  - `patches/jm-v7-wip/0081` (jm-v7 108): the v6/v7 IDVS polygon-mode predicate is hashed into the shader key, and the knob tag is now 0x76370108.
  - `patches/jm-v7-wip/0082` (jm-v7 109): kbase JM timestamp frequency calibration. `PANVK_KBASE_TS_CALIBRATE=0` = old.
  - mesa-rc5g HEAD `03b39a69`. To rebuild the tree: `prepare-rc5-tree.sh --csf-max 219 --wip-max 0082`.
- .so `/home/flux/build-out/2026-10-22-rc5-universal-108/libvulkan_panfrost.so`, 30520184 bytes, sha256 `276db559f1cf3274387e3fd614e34fb9870a07af206a26bff810a5a02d17e131` (`PanVK-kbase RC5-universal-108 (Mesa 26.3.0-devel (git-03b39a69c0))`). BDIR `/home/flux/build-rc5/bionic-r30-RC5-universal-108`, NDK r30, 4 jobs, under the build lock. The BDIR has 8 `libpanvk_v*.a`.
- APK `/home/flux/build-out/apks/panprobe-rc5u-108.apk`, sha256 `05ca59df4ad0692f46cefdd795326d934ffa6328d8aaa93eea5022c645a1a3bd`, package `dev.zenithblue.panvktest.rc5u` (1.2.7, vc 15), rc5 276db559 (= driver), rc4 c395db09. Built with `assembleDebug -PpanvkSoRc4=/home/flux/build-out/rc4-lowmem/... -PpanvkSoRc5=<so> -PpanprobeSuffix=rc5u`.
- Not device-tested. On a v7 device, check:
  - fill_mode passes 17/17 with a warm cache (run it twice).
  - bc_perf `upload_gpu_ms` is close to `upload_wall_ms`.
  - The logcat has the `kbase: GPU timestamp counter measured` line (A31) or the `keeping cntfrq` line (A06, S9).

## RC5-universal-109 (2026-10-22): FTL starlte timestamp period and the JM warm-up sleep
- RC5-universal-108 on the FTL S9 gave 39/39, but `ts_period=0.000`: cntfrq_el0 reads 0 and 0082 returned without measuring. See `device-runs/2026-10-22-ftl-v6-starlte-rc5u-108/results.md`. The A31 108 run showed a 2 s sleep on every JM device open (`device-runs/2026-10-22-ftl-v7-a31-rc5u-108/results.md`).
- Fixes:
  - `patches/jm-v7-wip/0083` (jm-v7 110): with cntfrq 0 the GPU counter rate is measured (one retry with a 4x window), snapped to a common SoC rate within 3 % and logged. If no stable rate is found, timestamps are not advertised (no period 0). Every early return logs. `PANVK_KBASE_TS_CALIBRATE=0` = old.
  - `patches/jm-v7-wip/0084` (jm-v7 111): the 2 s JM warm-up runs only on the first JM open per process. `PANVK_KBASE_JM_WARMUP=2` = old (every open), `=0` = never; `PANVK_KBASE_JM_WARMUP_MS` = length.
  - mesa-rc5g HEAD `1bdf1e0c`. To rebuild the tree: `prepare-rc5-tree.sh --csf-max 219 --wip-max 0084`.
- .so `/home/flux/build-out/2026-10-22-rc5-universal-109/libvulkan_panfrost.so`, 30522632 bytes, sha256 `e15568d0d7325a6bc49bb7536bd9eb2fb701fad3b5faf36a8e7ff5063d7f2e95` (`PanVK-kbase RC5-universal-109 (Mesa 26.3.0-devel (git-1bdf1e0c1a))`). BDIR `/home/flux/build-rc5/bionic-r30-RC5-universal-109`, NDK r30, 4 jobs, under the build lock. The BDIR has 8 `libpanvk_v*.a`, and the log has no kbase_kmod.c warnings.
- APK `/home/flux/build-out/apks/panprobe-rc5u-109.apk`, sha256 `b4c74216fb9ff4dc56719a6294b0631569e92abfa8e06ed91b4b553c8125105f`, package `dev.zenithblue.panvktest.rc5u` (1.2.7, vc 15), rc5 e15568d0 (= driver), rc4 c395db09. Built like 108 (`assembleDebug -PpanvkSoRc4=/home/flux/build-out/rc4-lowmem/libvulkan_panfrost-android-aarch64.so -PpanvkSoRc5=<so> -PpanprobeSuffix=rc5u`, stale `app/.cxx` removed first).
- PanPlay was not rebuilt.
- Not device-tested. Check:
  - S9: logcat has `kbase: cntfrq_el0 is 0; GPU timestamp counter measured at ... using 26000000 Hz` (or `timestamps not advertised`), bc_perf `ts_period=38.462`, and `upload_gpu_ms` about 0.9-1.0x `upload_wall_ms`.
  - A31: the 0082 line is unchanged (26 MHz).
  - All JM devices: one `kbase: JM warm-up 2000 ms (first JM open in this process)` per process, then `JM warm-up skipped`. 39/39 still pass, with no DEVICE_LOST and no job faults on the second open. The run should be about 1.5 min shorter on the A31.
