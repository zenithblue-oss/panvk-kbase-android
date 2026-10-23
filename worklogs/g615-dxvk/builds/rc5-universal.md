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

## RC5-universal-110 (2026-10-23): low-risk perf round from perf-review-2026-10-22
- Changes (none device-tested; details in `patches/jm-v7-wip/README.md` and the review's status list):

  | patch | item | expected gain | knob (old behaviour) |
  |---|---|---|---|
  | csf-v11 220 + jm-v7-wip 0085 | K6: command pool BOs freed on reset(RELEASE_RESOURCES) and vkTrimCommandPool | memory after loading screens / big frames | `PANVK_CMD_POOL_TRIM=0` |
  | csf-v11 221 | flush the transient pool BO only up to transient_offset | up to ~60 KiB less cache clean per pool per EndCommandBuffer/re-submit | `PANVK_POOL_FLUSH_FULL=1` |
  | csf-v11 222 + jm-v7-wip 0088 | hot-path log lines at debug (dma-buf import, CSF queue wait, v6/v7 growable BO after 8) | fewer logd writes per import / wait / GS buffer | `MESA_LOG_LEVEL=debug` |
  | jm-v7-wip 0086 | K7: no per-batch BO list on JM submit | CPU per submit, O(n^2) in pool BOs | none (list had no reader) |
  | jm-v7-wip 0087 | K8 (events): up to 16 job events per read() | about 2 syscalls per atom saved | `PANVK_KBASE_EVENT_BATCH=0` |
  | jm-v7-wip 0089 | item 14 (divide): texel-buffer width in the descriptor pad word | ~20-40 instructions per v6/v7 bindless texel-buffer access | `PANVK_V7_TEXEL_WIDTH=0` (hashed, tag 0x76370116) |

- Skipped: item 8, K4, the multi-atom half of K8, the index-clamp half of item 14 (reasons in the review).
- Tree: mesa-rc5g (branch `rc5g`) reset to the new series: HEAD `6cae57f8` (tag `rc5g-110`), base tag `rc5g-base` = `3916f976` (217 series commits). `prepare-rc5-tree.sh --csf-max 222 --wip-max 0089` on a fresh 5a07217f worktree (`/home/flux/src/mesa-rc5-110v`) printed `BASE-OK applied=217` and `OK base=217 jm-v7-wip=84 head=199d42af`, with no PATCH-DRIFT, WIP-DRIFT or offset lines; its tree `e24adbd0` equals mesa-rc5g HEAD's tree. Refreshed for it: `jm-v9/024` (two context lines) and `jm-v7-wip/0022` (hunk headers).
- .so `/home/flux/build-out/2026-10-22-rc5-universal-110/libvulkan_panfrost.so`, 30522256 bytes, sha256 `5f217f70aa28f47a1eda50200428c92dd8e7af5c717f07d638e8ea9c6357289f` (`PanVK-kbase RC5-universal-110 (Mesa 26.3.0-devel (git-6cae57f893))`; the new knob strings are present). Fresh BDIR `/home/flux/build-rc5/bionic-r30-RC5-universal-110`, NDK r30, `--api 29`, 4 jobs (thermal NONE), under the build lock; 8 `libpanvk_v*.a`. No new warnings in the touched files.
- PanProbe `/home/flux/build-out/apks/panprobe-rc5u-110.apk`, sha256 `73bbebec1f6cae26e3a0ba978b3e1b9680e36df2c3c1e37de3a5e9e622c65b88`, `dev.zenithblue.panvktest.rc5u` (1.2.7, vc 15), rc5 `5f217f70` (= driver), rc4 `c395db09`. Built like 109 (`--offline`, stale `app/.cxx` removed).
- PanPlay `/home/flux/build-out/apks/panplay-rc5u-110.apk`, sha256 `ccbf9109300631b3efd72e578d39519f77fa2781870944ff77761b0ad9d05352`, `dev.zenithblue.panvklauncher` **1.2.9-rc5, vc 35**, debug-signed (cert `bc517d8c...`, so it still cannot update the 1.2.8 install signed `8b10fde8...`; that needs an uninstall). `-PpanvkSo=<110 .so>`, all components bundled (stored) from /var/tmp/panvk/components: imagefs_bionic, Proton 11.0-2, FEXCore 2609.1, DXVK 3.1.1-4, VKD3D 3.0.1-452, Box64 0.4.5, PulseAudio 17.0-2, ALSA 1.2.16.1-2, Mesa-Zink 26.2.4-1. The Zink pin in `app/bundled-components.json` is now the local build `1f1eb714...` (was 46707d28). `bundled-driver.json` now labels the bundled driver "PanVK Kbase — RC5-universal-110" with the .so sha and size (`published: false`, tag `panvk-kbase-v0.1.0-rc5-universal-110`, not a release), so the driver card and session logs match the .so.
- Not device-tested. Check on JM devices: 39/39, no DEVICE_LOST, `kbase: v7 growable BO` lines stop after 8; on v6/v7 the bindless texel-buffer tests (vkd3d_heap, robustness) still pass; PSS drops after vkTrimCommandPool. On G615: 39/39 and the cubes.

## RC5-universal-111 (2026-10-23): FTL oriole (v9, Mali-G78) tess_perf and large_draw
- RC5-universal-110 on the FTL Pixel 6 (Mali-G78, v9 JM, API 33) gave 37/39. tess_perf gs=1 L=16 hit `JM atom 25 failed: event=0x60` (BASE_JD_EVENT_OUT_OF_MEMORY, tiler heap exhausted), then DEVICE_LOST. large_draw failed the base_inst_* CPU split cases and the two indirect fan cases. Diagnosis: `device-runs/2026-10-23-ftl-v9-oriole-rc5u-110/results.md`.
- Fixes (all v9 only, `PAN_ARCH == 9`; v6/v7/v10/v11 code unchanged):
  - `patches/jm-v7-wip/0090` (jm-v7 117): v9 tiler heaps 512 MiB each, up from 128 (GROW_ON_GPF, so VA, not memory). `PANVK_V9_TILER_HEAP_MB` (128 = old).
  - `patches/jm-v7-wip/0091` (jm-v7 118): v9 gpu_prerast CPU instance splits and fan windows keep firstInstance; the offset goes in `params.chunk_instance` (csf-v11 191 for v9). `PANVK_V9_SPLIT_BASE_INST=0` = old.
  - `patches/jm-v7-wip/0092` (jm-v7 119): v9 indirect fans clamped to the arena through a clamped copy of the command (`panlib_v9_fan_clamp_cmd`, csf-v11 195 for v9). `PANVK_V9_FAN_CLAMP=0` = old.
- Tree: mesa-rc5g (branch `rc5g`) HEAD `0f470d21` (tag `rc5g-111`), with commits bc83c08a / 7dd18a82 / 0f470d21 on 6cae57f8. `prepare-rc5-tree.sh --mesa /home/flux/src/mesa-rc5-111v --csf-max 222 --wip-max 0092` on a fresh 5a07217f worktree printed `BASE-OK applied=217 csf-v11<=222 head=a59989b6` and `OK base=217 jm-v7-wip=87 head=55b11c67`, with no PATCH-DRIFT, WIP-DRIFT, offset or fuzz lines. Its tree `ffa36b12` equals mesa-rc5g HEAD's tree.
- .so `/home/flux/build-out/2026-10-23-rc5-universal-111/libvulkan_panfrost.so`, 30525648 bytes, sha256 `a06030fafa51a72f148cc14f8f94e3b26b434fb2864203181f674788fe88fae5` (`PanVK-kbase RC5-universal-111 (Mesa 26.3.0-devel (git-0f470d21cf))`; it contains the strings `panvk: v9 tiler heaps 2 x %u MiB` and the three knobs). Fresh BDIR `/home/flux/build-rc5/bionic-r30-RC5-universal-111`, NDK r30, `--api 29`, `HOST_TOOLS=/home/flux/repos/panvk-kbase-android/build/host-tools/bin`, 4 jobs (thermal NONE), under the build lock. No warnings in the touched files.
- PanProbe `/home/flux/build-out/apks/panprobe-rc5u-111.apk`, sha256 `fc1649aaddd208e4fa4d60a57a9d1b33c115b061b1b58dd9821f94b017a8b028`, `dev.zenithblue.panvktest.rc5u` (1.2.7, vc 15). It embeds rc5 `a06030fa` (= driver) and rc4 `c395db09`. Built like 110 (`--offline`, stale `app/.cxx` removed).
- PanPlay `/home/flux/build-out/apks/panplay-rc5u-111.apk`, sha256 `92e1b8da2f0a312a5b4fd004eb4fc27e7c3cf6bf72cffefc29018a317be5ff74`, `dev.zenithblue.panvklauncher` **1.2.9-rc5, vc 36**, debug-signed, `-PpanvkSo=<111 .so>`. All components are bundled (stored), as in 110. It embeds `a06030fa`. `bundled-driver.json` labels "PanVK Kbase — RC5-universal-111" with the .so sha and size (`published: false`).
- Not device-tested. On v9 (Pixel 6 FTL): 39/39, the logcat line `panvk: v9 tiler heaps 2 x 512 MiB`, tess_perf gs=1 L=16 / L=mix hashes `d4463b35` / `b34a251c`, large_draw `65534 of 149999` / `65534 of 139999`, and every base_inst case bad=0. On v6/v7/v10/v11 a 39/39 rerun is optional, since only v9 code changed.
