# FTL run: Galaxy S9 (starlte, G72 v6, API 29), PanProbe RC5-universal-106, game-loop scenario 2

- Matrix 5926634178382505063. Outcome: "A non-retryable error occurred", test time 88 s.
- Bucket `gs://test-lab-hrb456s6qt79m-kz4jkut67musi/panvk-v6-starlte-rc5u-106/`, copied to `raw/starlte-29-en-portrait/`: logcat, video.mp4 (64 s), results_scenario_2.json, autorun.txt.
- APK panprobe-rc5u-106 (7356674b), .so 1cb2c074. The driver line confirms it: `DRIVER slot=RC5 driverInfo=PanVK-kbase RC5-universal-106 (Mesa 26.3.0-devel (git-710db1f998))`.
- Result: 6 PASS, 0 FAIL (gpu_prerast_slice ... bc_perf 41217 ms), then the kill. `"pass": 6, "complete": false`.

## Answers
1. **Did 0079 cause an earlier crash?** It caused an earlier kill, not a crash. There is no Fatal signal, tombstone, DEVICE_LOST or GPU fault. The process was SIGKILLed: `07:49:53.388 Zygote: Process 6269 exited due to signal 9 (Killed)`.
   - The run was shorter because the kill came in tess_perf's first tessellated config (gs=0 L=1), during command buffer recording. In the 105 run it came after 14 s of gs=0 configs, at the first gs=1 frame.
   - From GAMELOOP (07:48:27.58) to the kill took 86 s.
2. **Still the low-memory kill? Yes.** The PanProbeTess MEM lines from tess_perf (pid 9169):
   ```
   07:49:48.337 I PanProbeTess: MEM device avail_kb=1582572 rss_kb=19508
   07:49:48.347 I PanProbeTess: MEM recorded opt=default gs=0 L=ref avail_kb=1581860 rss_kb=24196
   07:49:48.349 I PanProbeTess: MEM first frame avail_kb=1581828 rss_kb=24196
   07:49:49.053 I PanProbeTess: MEM opt=default gs=0 L=ref avail_kb=1576616 rss_kb=24196
   07:49:49.074 I MESA    : panvk: v6 tess batch budget 32 MiB: new batch (varyings 43 MiB worst case, GS scratch 0 MiB)
   07:49:49.093 I chatty  : ... libpt_runner.so identical 1 line
   07:49:49.115 I MESA    : panvk: v6 tess batch budget 32 MiB: new batch (varyings 43 MiB worst case, GS scratch 0 MiB)
   ```
   - The test started with **1.58 GB available and 19-24 MB RSS**. The non-tessellated reference config (one batch) cost about 5 MB.
   - L=1 then split into a new batch **on every draw**, about every 20 ms. One 16-patch draw is 21.5 MiB of worst-case varyings, so two never fit the 32 MiB budget. The log is capped at 3 lines.
   - No `recorded opt=default gs=0 L=1` line followed, so recording never finished. By 07:49:49.367 lmkd was reclaiming. From 07:49:51.0 to 07:49:53.6 about 80 processes were killed, including PanProbe (`fore TOP (236,186)`) and the testloop companion.
   - No new growable BO appeared during this. pid 9169 logged only the 7 device BOs: 2 tiler heaps, 2 indirect varying buffers, the tess arena and 2 tess GS scratch regions.
   - The last MEM line is 1.58 GB available and 24 MB RSS, at 07:49:49.053. About 1.5 GB was then consumed within about 4 s of recording, before any GPU work for L=1 had been submitted.
3. **Which test?** tess_perf, the 7th test, with variant `default`, gs=0, L=1, during recording.

## Cause: per-batch TLS/WLS buffers multiplied by the 0079 batch splits
- On JM, `panvk_per_arch(cmd_close_batch)()` gives every batch its own TLS buffer and its own WLS buffer from the command buffer's tls_pool. The TLS buffer is `pan_get_total_stack_size(stack, thread_tls_alloc, core_id_range)`, the stack for every thread of every core. WLS is the workgroup local storage times instances.
- These are NO_MMAP BOs, committed in full, and they are only released when the command buffer is reset. A tessellated draw's batch needs both, because the libpoly tessellator and prefix kernels use stack and shared memory.
- With 0079 a 256-draw frame became about 256 batches, so about 256 sets of TLS and WLS. At about 20 ms per draw (allocating and zero-filling several MB per batch), roughly 200 batches in 4 s used about 1.5 GB. The size per batch is estimated from the timing, not measured; 0080 logs the first TLS and WLS sizes so the next run measures it.
- 0079's offset reset and per-batch reuse worked: there were no growing growable BOs. But its splits are only cheap if the batch's other per-batch resources are shared too. They were not.

## Fix: patches/jm-v7-wip/0080 (jm-v7 107) + PanProbe guard
- **One TLS and one WLS buffer per heap half per command buffer.** Every batch of that half reuses it, and a larger one is allocated only when a batch needs more. `PANVK_V7_TLS_SHARE=0` restores the old behaviour.
  - Same-half batches are ordered: a batch's vertex/tiler atom depends on the previous same-half fragment atom, and the fragment atoms are ordered. Batches without a fragment job, or with overlap off, are fully ordered. So a shared buffer is never used by two running batches.
  - Split-heavy frames now cost at most 2 TLS + 2 WLS buffers per command buffer.
- **0079 self-review** (v6 and v7, for the A31 G52 run next):
  - **Tiler context clobbering, fixed.** Picking the heap half early (the varying allocator and the tess draw) called `cmd_prepare_tiler_context(cmdbuf, 0)`, which overwrites `batch->tiler.ctx` with layer 0's context. For layered or multiview rendering that could tile a draw into the wrong layer. `panvk_v7_batch_pick_half()` now runs only when the batch has no tiler context yet, and keeps `batch->tiler.ctx`.
  - **Transform feedback, fixed.** Nested tess draws with transform feedback active no longer take GS scratch memory: their xfb staging stays in gs_pool. I could not prove that xfb staging is consumed inside the same batch.
  - **Checked, OK:**
    - The per-batch varying allocator is a fresh one with its offset reset on re-issue. It only depends on same-half ordering.
    - The GS scratch only holds buffers the batch's own tiler and fragment jobs read.
    - The tight arena uses the same `max_indices` for the tessellator budget and the emulation size, so it can never be overrun.
    - Arch guards: `indirect_varying_bufs_infos` and the scratch exist only for `PAN_ARCH < 9`; v9 JM builds them out.
  - **Known cost:** one batch per 16-patch tess draw at the 32 MiB budget means 256 small fragment passes per tess_perf frame on v6/v7. That costs tess_perf us_draw, not memory.
- **PanProbe tess_perf low-memory guard (honest).** MemAvailable is checked every 32 recorded draws and after every frame. Below `PT_TESS_MIN_AVAIL_KB`, default 300000 kB, the test stops and **FAILs** with `FAIL LOWMEM`, and logs a `MEM LOWMEM at <where>` line. The rest of the autorun continues instead of the harness being killed. Correctness checks (variant hash MISMATCH, LEVEL_IGNORED) are unchanged for every config that ran.

## Build (not device-tested; no Firebase run launched)
- Tree `/home/flux/src/mesa-rc5g` HEAD `28b0346c`, which is 0078 `710db1f9` + 0079 `f68abd4b` + 0080 `28b0346c`.
- Built with a fresh BDIR `/home/flux/build-rc5/bionic-r30-RC5-universal-107`, under the build lock, 4 jobs, exit 0 (1169/1169):
  `flock /home/flux/build-cache/build.lock env MESA=/home/flux/src/mesa-rc5g BDIR=/home/flux/build-rc5/bionic-r30-RC5-universal-107 HOST_TOOLS=/home/flux/repos/panvk-kbase-android/build/host-tools/bin PANVK_KBASE_RELEASE=RC5-universal-107 PANVK_JOBS=4 scripts/build-android.sh --profile g615-v11-csf --api 29 --ndk /opt/android-sdk/ndk/30.0.16248370`
- .so `/home/flux/build-out/2026-10-22-rc5-universal-107/libvulkan_panfrost.so`, 30518344 bytes, sha256 `9a6308c8be00f5ea58c59f9a399f329790fc7e2ee55a49a7544becc594eabbbd`.
  - Version string: `PanVK-kbase RC5-universal-107 (Mesa 26.3.0-devel (git-28b0346c01))`.
  - It contains `PANVK_V7_TLS_SHARE` and the `batch TLS` / `batch WLS` log strings. These log the first 4 TLS/WLS allocations per process with their sizes.
- APK: `apps/panvk-test/gradlew assembleDebug -PpanvkSoRc4=/home/flux/build-out/rc4-lowmem/libvulkan_panfrost-android-aarch64.so -PpanvkSoRc5=<so above> -PpanprobeSuffix=rc5u` (after removing the stale `app/.cxx`).
  - Output: `/home/flux/build-out/apks/panprobe-rc5u-107.apk`, 53750240 bytes, sha256 `0c3a8f5192301c30404d5f07aa2c19b3b784f099d19994b8cc302d185e84702d`.
  - aapt2: `dev.zenithblue.panvktest.rc5u`, versionCode 15, 1.2.7, with the TEST_LOOP intent filter and `com.google.test.loops`.
  - Embedded .so files: rc5 `9a6308c8` (= the .so above), rc4 `c395db09`.
  - `libt_tess_perf.so` (`4d08b280`) contains the LOWMEM guard and `PT_TESS_MIN_AVAIL_KB`.
- What to look for in the next logcat:
  - `panvk: v6 batch TLS ... KiB` and `batch WLS ... KiB`: at most 2 of each per command buffer.
  - `PanProbeTess: MEM recorded opt=default gs=0 L=1 ...`: recording now finishes.
  - MemAvailable staying far above 300 MB through gs=1.
  - If memory still runs low, `FAIL LOWMEM` and a tess_perf FAIL instead of a harness kill, with the other 32 tests still running.
- A/B knobs: `PANVK_V7_TLS_SHARE=0`, `PANVK_V7_TESS_BATCH_MB=0/16/64`, `PANVK_V7_TESS_GS_TIGHT=0`, `PANVK_V7_GROW_STEP=0`, `PT_TESS_MIN_AVAIL_KB`.

## Screenshots
`screenshots/video-001s.png` (FTL device details), `video-040s.png` (PanProbe running) and `video-063s-end.png` (last frame, after the mass kill).
