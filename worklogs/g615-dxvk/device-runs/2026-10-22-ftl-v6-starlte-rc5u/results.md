# FTL run: Galaxy S9 (starlte, G72 v6, API 29), PanProbe RC5-universal, game-loop scenario 2

- Project phonefinder-hub, matrix `matrix-2kxz6gv4kwzhv`. Outcome: "Test failed to run", "A non-retryable error occurred" after 101 s.
- Bucket `gs://test-lab-hrb456s6qt79m-kz4jkut67musi/panvk-v6-starlte-rc5u/`, copied to `raw/starlte-29-en-portrait/`: logcat (64502 lines), video.mp4 (83 s), game_loop_results/results_scenario_2.json, artifacts/.../files/autorun.txt. No runs/ dir was pulled because the app died before it wrote one.
- APK `/home/flux/build-out/apks/panprobe-rc5u.apk`, package `dev.zenithblue.panvktest.rc5u` 1.2.7, rc5 slot .so `ab7eaa92`.
- Driver line: `DRIVER slot=RC5 driverInfo=PanVK-kbase RC5-universal (Mesa 26.3.0-devel (git-3093e6a58e))`, `gpu_id 0x62210010 variant 0 arch v6 model Mali-G72`.

## Result
6 PASS, 0 FAIL, then the app was SIGKILLed. `results_scenario_2.json`: `"pass": 6, "fail": 0, "complete": false`.

| test | result | ms |
|---|---|---|
| gpu_prerast_slice | PASS | 4167 |
| clip_cull | PASS | 5225 |
| multi_viewport | PASS | 5152 |
| fill_mode | PASS | 5833 |
| bc_decode | PASS | 4176 |
| bc_perf | PASS | 41277 |
| **tess_perf** | killed (no result) | - |

DXVK, Bachata S4 and vkd3d compliance all PASS (items 94/114/100, missingHard=[]).

## What failed: no install, manifest or game-loop problem
- The install worked: `PackageManager ... destCodeFile /data/app/dev.zenithblue.panvktest.rc5u-...` and `PACKAGE_ADDED` at 06:19:15.
- The game-loop intent worked: `06:19:26.466 3582 6220 I PanVKTest: GAMELOOP scenario=2`. The rc5 slot loaded, and six tests passed and logged `PANPROBE_TEST ... status=PASS`.
- The app did not hang, and there was no native crash or GPU fault: no `Fatal signal`, tombstone, `DEVICE_LOST`, `event=0x` or MMU fault lines.

## Cause: the new tess_perf test ran the device out of memory (v6/v7 GS-emulation sizing)
In this APK, the test after bc_perf is **tess_perf**. It was added in PanProbe commit 6162649, after the v7N APK was built: `libt_tess_perf.so` is in panprobe-rc5u.apk but not in panprobe-v7N.apk. tess_perf records 256 tessellated draws (16 quad patches each) into one command buffer, with the GS off and then on.

Each test runs in its own process. The tess_perf process is pid 8676. It started at 06:20:43, and after its configs ran for 14 s it logged 768 growable BOs in 2 s:
```
06:20:43.352  8676  8676 I MESA    : kbase: JM driver, uAPI version 11.16, page_size=4096
06:20:47.358  8676  8676 I MESA    : kbase: v6 growable BO, GROW_ON_GPF, 256 of 40960 pages committed
06:21:01.681  8676  8676 I MESA    : kbase: v6 growable BO, GROW_ON_GPF, 256 of 24576 pages committed
06:21:01.682  8676  8676 I MESA    : kbase: v6 growable BO, GROW_ON_GPF, 256 of 24576 pages committed
06:21:01.683  8676  8676 I MESA    : kbase: v6 growable BO, GROW_ON_GPF, 256 of 6144 pages committed
   ... the same triple repeated 256 times ...
06:21:03.407  8676  8676 I MESA    : kbase: v6 growable BO, GROW_ON_GPF, 256 of 6144 pages committed
```
Per-process MESA line counts (`grow` = growable-BO lines):

| run | process | test | MESA lines | grow |
|---|---|---|---|---|
| v7N (passed 38/38) | 13267 | geometry (the test after bc_perf in v7N) | 30 | 14 |
| v7N | 13807 | large_draw (largest in v7N) | 128 | 112 |
| rc5u | 8676 | tess_perf | 788 | **772** (512 x 24576 pages + 256 x 6144 pages) |

Next, lmkd reclaimed memory, and then the kernel/LMK killed most of userspace at once, including Chrome, the launcher, Play Store, the FTL testloop companion and PanProbe:
```
06:20:53.768   510   510 I lmkd    : Reclaim Mem 'com.samsung.android.bixby.service' (344) ... to free 142068kB
06:21:01.977   510   510 I lmkd    : Reclaim Mem 'com.sec.android.widgetapp.samsungapps' (6492) ... to free 90400kB
06:21:05.153   764   764 I Zygote  : Process 3582 exited due to signal 9 (Killed)
06:21:05.606   510   510 W lmkd    : Failed to open /proc/3582/oom_score_adj; errno=2: process 3582 might have been killed
06:21:05.621  1045  8818 I ActivityManager: Process dev.zenithblue.panvktest.rc5u (pid 3582) has died: vis TOP (1447,194)
06:21:05.719  1045  8932 I ActivityManager: Failure reporting to instrumentation watcher: comp=ComponentInfo{com.google.testing.platform.android.driver.testloop.companion/...
```
The testloop companion died along with the app, so FTL reports "non-retryable error" rather than a test failure.

### Driver mechanism (v6/v7 JM, not new in RC5)
On v6/v7 a tessellated draw with a GS after the TES is "nested" (`panvk_tess_draw`): the TES feeds the geometry emulation. Each nested draw gets its own emulation buffers, sized for `PANVK_TESS_GS_MAX_PRIMS` = 2M input primitives (jm-v7 033/014). That is 2M x 3 verts x 16 B = 96 MiB position, 96 MiB general and 24 MiB index, which are the 24576/24576/6144-page BOs above. They come from the growable `gs_pool` (jm-v7 030/078). Each is larger than the pool's 2 MiB slab, so it becomes a dedicated BO. On Bifrost every GPU-only growable BO commits 1 MiB up front (jm-v7 003) and grows in 2 MiB steps. So 256 draws make 768 BOs, at least 768 MiB committed and 54 GiB of VA in one command buffer, before the GPU touches anything.

- Not a csf-v11 204-219 regression. The JM GS/tess sizing (`panvk_vX_cmd_draw.c` jm path) is unchanged between v7N (`c68c685`) and RC5u (`3093e6a5`) except for 0077's winding flag. The csf-v11 changes there (shader cache dir, u_queue fork handlers, BC) did not show up: bc_decode and bc_perf passed, and there are no cache or fork errors. The v7N driver is expected to fail the same way on tess_perf. v7N passed only because its APK had no tess_perf.
- The X6 (v11 CSF) does not use this path. Its tess is fused (csf-v11 213-219), which is why bc6d ran tess_perf PASS on the G615.

## Fix: patches/jm-v7-wip/0078 (jm-v7 105)
`0078-jm-v7-105-v6-v7-nested-tess-draws-tight-GS-emulation.patch` (mesa-rc5g commit `710db1f9` on top of `3093e6a5`):
1. `panvk_tess_draw`: for a nested draw whose counts the CPU knows, `max_indices` is capped at patches x `PANVK_TESS_MAX_PRIMS_PER_PATCH` (8704, the maximum at tess level 64) x vpp. The tessellator keeps only the whole patches that fit in `max_indices`, and `gs_max_prims = max_indices / vpp` sizes the emulation, so the GPU cannot overrun it. For tess_perf the buffers drop from 216 MiB VA to 6.4 + 6.4 + 1.6 MiB per draw. `PANVK_V7_TESS_GS_TIGHT=0` restores the old behaviour.
2. `kbase_kmod_bo_alloc`: Bifrost JM GPU-only growable BOs under 64 MiB commit and grow in steps of 1/64 of their size, 64 KiB to 1 MiB, instead of a 1 MiB commit and 2 MiB steps. This covers the GS-emulation buffers and the 2 MiB gs_pool slabs. The tiler heaps (64/128 MiB), tess arena (160 MiB) and indirect varying buffer (288 MB) are unchanged. `PANVK_V7_GROW_STEP=0` restores the old behaviour.

Estimated tess_perf gs=1 peak: about 1.1 MiB per draw at L=16 and 0.2 MiB at L=1, so under 300 MiB per command buffer. Before the fix it was at least 3 MiB per draw, 768 MiB committed. This is not device-tested.

## Build (fix not yet device-tested, no Firebase rerun launched)
- Tree: `/home/flux/src/mesa-rc5g` branch `rc5g`, HEAD `710db1f9` (= RC5-universal `3093e6a5` + jm-v7-wip 0078).
- Built under the build lock with a fresh BDIR, 4 jobs and thermal NONE. Exit 0, 1169/1169:
  `flock /home/flux/build-cache/build.lock env MESA=/home/flux/src/mesa-rc5g BDIR=/home/flux/build-rc5/bionic-r30-RC5-universal-105 HOST_TOOLS=/home/flux/repos/panvk-kbase-android/build/host-tools/bin PANVK_KBASE_RELEASE=RC5-universal-105 PANVK_JOBS=4 scripts/build-android.sh --profile g615-v11-csf --api 29 --ndk /opt/android-sdk/ndk/30.0.16248370`
- .so: `/home/flux/build-out/2026-10-22-rc5-universal-105/libvulkan_panfrost.so`, 30512960 bytes, sha256 `3d1a588cf54856002acce5a2dca8d6c79f76b8a972bf8440073c3ae33aa5d356`. `strings` shows `PanVK-kbase RC5-universal-105 (Mesa 26.3.0-devel (git-710db1f998))`, `PANVK_V7_TESS_GS_TIGHT` and `PANVK_V7_GROW_STEP`.
- APK: `apps/panvk-test/gradlew assembleDebug -PpanvkSoRc4=/home/flux/build-out/rc4-lowmem/libvulkan_panfrost-android-aarch64.so -PpanvkSoRc5=<so above> -PpanprobeSuffix=rc5u`
  produced `/home/flux/build-out/apks/panprobe-rc5u-105.apk`, sha256 `23dd2d23fcbaa48febdea8fa64bb312d94be076eccb85c6c5897049991be55e6`.
  aapt2: `dev.zenithblue.panvktest.rc5u`, versionCode 15, 1.2.7, with the TEST_LOOP intent filter and `com.google.test.loops` metadata. The embedded `libvulkan_panfrost_rc5.so` is 3d1a588c (the .so above) and `libvulkan_panfrost_rc4.so` is c395db09 (RC4-lowmem). It has the same file list as panprobe-rc5u.apk, including `libt_tess_perf.so`.
- Rerun (user's call): `gcloud firebase test android run --type game-loop --app /home/flux/build-out/apks/panprobe-rc5u-105.apk --scenario-numbers 2 --device model=starlte,version=29 --timeout 30m --directories-to-pull /sdcard/Android/data/dev.zenithblue.panvktest.rc5u/files --results-dir panvk-v6-starlte-rc5u-105`.
  Expect tess_perf to log far fewer than 768 growable BOs, with no LMK kill, and 39/39.
- If tess_perf still OOMs: A/B with `PANVK_V7_TESS_GS_TIGHT=0` / `PANVK_V7_GROW_STEP=0`. The longer-term fix is a GPU-side bump allocation of nested-tess GS output (one per-batch heap, like the libpoly tess heap) instead of per-draw buffers.

## Screenshots
Frames from video.mp4 (PyAV, 540x1110): `screenshots/video-001s.png` (FTL device details, SM-G960F, API 29), `video-040s.png`, `video-075s.png` and `video-082s.png`. The last frame shows PanProbe "Running", 6 pass / 0 fail / 0 skip, the state at the kill.
