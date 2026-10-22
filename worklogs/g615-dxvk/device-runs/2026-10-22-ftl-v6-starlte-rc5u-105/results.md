# FTL run: Galaxy S9 (starlte, G72 v6, API 29), PanProbe RC5-universal-105, game-loop scenario 2

- Matrix 5002760495733809791. Outcome: "Test failed to run" (same as matrix-2kxz6gv4kwzhv).
- Bucket `gs://test-lab-hrb456s6qt79m-kz4jkut67musi/panvk-v6-starlte-rc5u-105/`, copied to `raw/starlte-29-en-portrait/`: logcat (7.4 MB), video.mp4 (68 s), results_scenario_2.json, autorun.txt.
- APK `panprobe-rc5u-105.apk` (23dd2d23), .so 3d1a588c. The driver line confirms 0078 is loaded: `DRIVER slot=RC5 driverInfo=PanVK-kbase RC5-universal-105 (Mesa 26.3.0-devel (git-710db1f998))`.

## Answers
1. **Same low-memory kill? Yes.** The install and game loop were fine (`PACKAGE_ADDED`, then `06:54:50.939 GAMELOOP scenario=2`). There is no Fatal signal or DEVICE_LOST. lmkd reclaimed memory, then most of userspace was SIGKILLed together with PanProbe:
   ```
   06:56:28.666   513 I lmkd    : Reclaim Mem 'com.sec.spp.push' (12326), uid 10112, oom_adj 999, state 11 to free 102032kB
   06:56:31.260   513 I lmkd    : Reclaim Mem 'com.samsung.android.mobileservice' (9689), ... to free 64532kB
   06:56:31.150 - 06:56:31.280  Zygote: Process 16095/16840/16123/16203/14680/11162/13003/16223/16150 exited due to signal 9 (Killed)
   06:56:33.653  1056 I ActivityManager: Process dev.zenithblue.panvktest.rc5u (pid 12824) has died: fore TOP (1001,244)
   ```
2. **Growable allocations in tess_perf (pid 17809): 772.** That is 4 device BOs, 512 x 1632 pages and 256 x 512 pages, the same count as before: 256 draws x 3 emulation buffers.
3. **Did 0078 take effect? Yes, both parts.** The VA per buffer dropped from 24576/24576/6144 pages (96/96/24 MiB) to 1632/1632/512 pages (6.4/6.4/2 MiB). The up-front commit dropped from 256 pages to 25/25/16 pages. So the up-front commit for the 256 draws went from 768 MiB to about 66 MiB:
   ```
   06:56:28.529 17809 I MESA    : kbase: v6 growable BO, GROW_ON_GPF, 25 of 1632 pages committed
   06:56:28.529 17809 I MESA    : kbase: v6 growable BO, GROW_ON_GPF, 25 of 1632 pages committed
   06:56:28.530 17809 I MESA    : kbase: v6 growable BO, GROW_ON_GPF, 16 of 512 pages committed
   ```
4. **Which test? tess_perf again**, the 7th test (6 PASS before it: gpu_prerast_slice ... bc_perf 42379 ms). The process started at 06:56:10 and its device at 06:56:14. Its gs=0 configs ran for 14 s, then the gs=1 command buffer was recorded at 06:56:28.53-28.67, and lmkd started reclaiming at 06:56:28.666. The app was killed 5 s later, during the first gs=1 frames.

## Cause: more than the GS buffers
Even with only about 66 MiB committed up front, the S9 ran out of memory at the first tess_perf gs=1 frame. What the device already held after the gs=0 configs did not show in the log, because only GPU page-fault growth adds to it. The JM tessellation path has three memory sinks that grow and never shrink:

1. **Indirect varying buffer (288 MB growable): its offset is never reset when a command buffer is resubmitted.** The allocator (`libpan_draw_helper_varying_buf_info.offset`) is set to 0 by the CPU at record time and advanced by GPU atomics. `panvk_reset_issued_batch()` restores the job headers and tiler descriptors, but not this offset. tess_perf records one command buffer per config and submits it dozens of times in 700 ms. At L=16 each frame's tess eval varyings take about 0.9 MiB per draw, about 220 MiB per frame. By the second frame the offset reaches the end of the buffer, so all 288 MB are GPU-committed (kbase GROW_ON_GPF commits every page below the faulting one). After that the tessellator's room check drops patches. This is a pre-existing driver bug, latent in every test that submits once.
2. **The tess arena (160 MiB growable)** was laid out for PANVK_TESS_MAX_INDICES and 2^18 patches. The index buffer sits at about 90+ MiB, so the first tessellated draw commits about 90+ MiB.
3. **Nested GS emulation buffers** (0078 shrank them): one set per draw, alive until the command buffer is reset. At L=16 that is about 0.9 MiB per draw, so about 230 MiB per frame.

Together with the two 64 MiB tiler heaps that 256 draws per batch fill, and the app itself, this runs past what the S9 has free. The FTL S9 was already shedding cached apps before PanProbe started (ActivityManager "Killing ... empty #25" bursts from 06:54:47). These numbers are estimates from code reading. jm-v7 106 adds logging (below) so that the next run measures them.

## Fix: patches/jm-v7-wip/0079 (jm-v7 106)
mesa-rc5g commit `f68abd4b` (on 0078 `710db1f9`):
- **Reset the indirect varying allocator on re-issue.** Each batch records the allocators it created (`batch->vary_infos`), and `panvk_reset_issued_batch()` sets their offsets back to 0 before the desc pool flush.
- **Reuse memory per batch, bounded per batch.** `PANVK_V7_TESS_BATCH_MB` defaults to 32; 0 disables it.
  - Before each tessellated draw, `panvk_tess_draw()` starts a new batch when the batch's worst-case tess eval varyings (`max_indices x layers x (20 + generic)`) would pass the budget, or when the GS scratch used so far is past it. A batch split keeps the render pass contents.
  - It also splits before a draw that would cross the ~5k-job split, so that split can no longer fall between the phases of one tess draw.
  - With per-half buffers, each new batch gets its own varying allocator starting at 0.
- **Per heap half buffers.** PANVK_OVERLAP lets batch N+1's vertex jobs run during batch N's fragment job, but never during the previous batch of the same half. So the device gets a second indirect varying buffer and two 256 MiB growable `tess_gs_scratch` regions, one of each per `heap_half`.
  - Nested tess draws bump-allocate their GS emulation buffers from the batch's half, starting at 0 in every batch. A draw that does not fit falls back to the gs_pool.
  - The tiler context, and so the heap half, is now set up before the varying allocator is made.
- **Tight tess arena.** A draw the CPU knows sizes `max_indices` (patches x 8704 x vpp) and `max_patches` (its patch count) from its own patches. That applies to all such draws, not only nested ones, so the arena only commits what the draw can touch.
- Expected bound for tess_perf on S9: per half, varyings up to about 32 MiB plus one draw, GS scratch up to about 32 + 14 MiB, and the arena about 5 MiB. About 160 MiB in total, instead of 288 + 90 + 230 MiB or more.
- Cost: tess_perf gs=1 splits roughly every 2 draws (about 15 MiB worst case of varyings per 16-patch draw), so a frame becomes about 128 batches. That costs tess_perf us_draw on v6/v7, in exchange for not being killed.

## PanProbe changes (same APK)
- tess_perf logs `MEM <stage> avail_kb=.. rss_kb=..` to stdout and to logcat (tag `PanProbeTess`) after device creation, after each record, after each config's first frame, after each config, and after teardown. The next FTL logcat will show the memory curve even if the app is killed.
- tess_perf destroys the device and instance at the end of each variant. Before, it kept the first variant's device alive while the second ran, so two devices' tess memory would have been resident at once.
- Test libraries link liblog.

## Build (not device-tested; no Firebase run launched)
- Driver: fresh BDIR `/home/flux/build-rc5/bionic-r30-RC5-universal-106`, under the build lock, 4 jobs, exit 0 (1169/1169):
  `flock /home/flux/build-cache/build.lock env MESA=/home/flux/src/mesa-rc5g BDIR=/home/flux/build-rc5/bionic-r30-RC5-universal-106 HOST_TOOLS=/home/flux/repos/panvk-kbase-android/build/host-tools/bin PANVK_KBASE_RELEASE=RC5-universal-106 PANVK_JOBS=4 scripts/build-android.sh --profile g615-v11-csf --api 29 --ndk /opt/android-sdk/ndk/30.0.16248370`
- .so `/home/flux/build-out/2026-10-22-rc5-universal-106/libvulkan_panfrost.so`, 30516152 bytes, sha256 `1cb2c074bb1f234ca4fc3a5f1eb12eb048ba7ca74d93710a45351ffbea51c7e5`.
  - Version string: `PanVK-kbase RC5-universal-106 (Mesa 26.3.0-devel (git-710db1f998))`. The git id is the HEAD at build time; the 106 change was built from the worktree before it was committed as `f68abd4b`, and the release label is what identifies it.
  - It contains `PANVK_V7_TESS_BATCH_MB` and the "tess batch budget" log string.
- APK: `apps/panvk-test/gradlew assembleDebug -PpanvkSoRc4=/home/flux/build-out/rc4-lowmem/libvulkan_panfrost-android-aarch64.so -PpanvkSoRc5=<so above> -PpanprobeSuffix=rc5u` (a stale `app/.cxx` was removed first).
  - Output: `/home/flux/build-out/apks/panprobe-rc5u-106.apk`, sha256 `7356674b2481f39b9297a7c45e4b6f41e448e7a85f723339bdd84c6614b88010`.
  - aapt2: `dev.zenithblue.panvktest.rc5u`, versionCode 15, 1.2.7, with the TEST_LOOP intent filter and `com.google.test.loops`.
  - Embedded .so files: rc5 `1cb2c074` (= the .so above), rc4 `c395db09` (RC4-lowmem).
  - `libt_tess_perf.so` (`0ceaf1f1`) needs liblog and contains `PanProbeTess`, `MEM %s` and `vkDestroyDevice`.
- What to look for in the next logcat:
  - `PanProbeTess: MEM ...` lines, which give the memory curve per tess_perf stage.
  - `panvk: v6 tess batch budget 32 MiB: new batch` (the split is active).
  - Very few `growable BO` lines in tess_perf: 4 device BOs, plus 3 for the new per-half buffers. The nested GS buffers come from the scratch now.
- A/B knobs if it still dies:
  - `PANVK_V7_TESS_BATCH_MB=16` (smaller batches) or `=0` (off).
  - `PANVK_V7_TESS_GS_TIGHT=0` and `PANVK_V7_GROW_STEP=0` (0078).

## Screenshots
`screenshots/video-001s.png` (FTL device details) and `video-040s.png` (PanProbe running). `video-068s-end.png` is the last frame: a black screen after the mass kill.
