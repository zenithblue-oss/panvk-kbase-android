# RC5-dev-bc4 build (2026-10-22): cheaper BC encoders, lazy raw store, lazy prerast arenas, working shader cache (csf-v11/204-208)

Goal (user, via the coordinator): bc4 has to beat RC4-lowmem on both memory
and speed for every BC format, keep 38/38, and keep bc_decode passing. The
targets are upload_gpu_ms BC1 2.89, BC3 3.22, BC4S 1.17, BC4U 2.48,
BC5U 2.50, BC5S 2.80, and bc_perf wall time of about 37 s or less. Process
memory has to be below RC4. The fork research
(`worklogs/bc-emul-perf/fork-research-2026-10-22.md`) asked for these
changes: port the fork's cheap encoders as the default; write raw blocks
only after the first readback; keep BC7 on AFRC; make signed BC6H to ASTC
HDR an opt-in.

## Patches

1. **204 `util/disk_cache`: find the Android app cache dir for execve()d
   helpers.**
   - **Problem.** bc3 still logged "Failed to create /data/.cache".
     PanProbe fork()+execve()s `libpt_runner.so`, so `/proc/self/cmdline`
     is a `/data/app/...` path and HOME=/data. 203 skipped any cmdline that
     starts with '/'.
   - **Fix.** The new `src/util/u_android_app_dir.h` finds the package in
     several places, in this order:
     - the own cmdline (as a process name, then as a path: /data/app/[~~x/]pkg-y,
       /data/user[_de]/N/pkg, /data/data/pkg);
     - the parent's cmdline;
     - TMPDIR, cwd, then HOME.

     It then uses the first writable dir of
     `/data/user/<uid/100000>/<pkg>/cache` and `/data/data/<pkg>/cache`.
     On Android, an unwritable XDG_CACHE_HOME now falls through instead of
     disabling the cache. The bc raw-store spill dir uses the same resolver.
   - **Test.** The host harness `tools/android-cache-dir-test/test.c` runs
     21 cases and all pass.
2. **205 panvk/csf: commit the gpu_prerast arenas at their first draw.**
   - **Problem.** The 2 x 80 MiB arenas (GS/XFB and tess) were allocated
     and memset at every VkDevice creation.
   - **Fix.** On v10+ they are now allocated on the first draw that needs
     them, under `gpu_prerast_lock`, using
     `panvk_per_arch(device_gpu_prerast_arena)`. On failure the draw sets
     OUT_OF_DEVICE_MEMORY. Each arena is still committed in full when it is
     created, because kbase growth-on-fault loses the first draw (patch 100).
   - **Unchanged.** v9 JM stays eager, since it reads the arenas directly.
   - **Toggle.** `PANVK_GPU_PRERAST_ARENA=eager` restores the old
     behaviour.
3. **206 panvk: transcode BC3 alpha, BC4 and BC5 through an endpoint pair
   table.**
   - **Source.** Port of the fork's EAC LUT (GunaCharanTeja csf-v11/213 +
     219, MIT, credited in the files). The offline generator
     `bc/panvk_bc_eac_lutgen.c` finds the best EAC header for each (e0, e1)
     and each kind: alpha, R11 and signed R11. A palette that reaches 0 or
     255 (or a mask's ends) only takes fits that have that end level.
   - **Shader.** It does one 2-word table load per block and remaps the
     16 indices. The table data lives in panvk_image.c. The device holds a
     512 KiB BO per kind, created lazily at the first upload of that kind.
   - **BC1 palette fit.** The fork's palette-fit idea is now the default
     opt set: `PANVK_BC_LM_OPTS` = 0x20000, which was 0x30000. H mode runs
     only when the best ETC1 error is above 1024.
   - **Opt-ins.** 0x400000 restores the EAC search, 0x800000 tries H mode
     on every block, and 0xc30000 is the bc3 encoder, bit-identical.
4. **207 panvk: keep BC raw blocks only after the device reads one back**
   (`PANVK_BC_RAW=lazy`, the default).
   - **Default behaviour.** Uploads of BC1-BC5 images (lowmem 1, not 3D)
     write no raw blocks, and there is no raw store BO.
   - **BC to BC copies.** These copy the transcoded shadow blocks with the
     new `bc/panvk_bc_scopy.comp`. That is exact for sampling.
   - **Readback.** A raw readback (CopyImageToBuffer/Memory, or a BC copy
     that cannot use the shadow) is skipped and warns once. It then sets
     `dev->bc_raw.needed`, so images created after that keep raw blocks.
   - **Toggles.** `PANVK_BC_RAW=store` restores 201-206 and `plane` keeps a
     raw plane. bc_decode sets `PANVK_BC_RAW=store` before it loads the
     driver, so its bit-exact raw checks stay valid.
   - **Fix in this build.** `bc_raw_lazy_hit` was used before its
     definition. The first Android build failed on that, it is now fixed,
     and 207 has been re-exported.
5. **208 panvk: transcode signed BC6H to ASTC 4x4 HDR on request**
   (`PANVK_BC_BC6H=astc_sf`).
   - **What it does.** The ASTC transcoder gets spec constant 2 (8 = UF,
     9 = SF). SF clamps negative values to 0 and scales by 31/32.
   - **Status.** Opt-in only, because it loses negatives. The default stays
     the exact shadow.
   - **Unsigned BC6H.** The default is the exact B10G11R11 shadow, which
     filters at full rate. ASTC HDR (`astc`) filters at half rate (fp16),
     so it stays opt-in. BC7 stays on AFRC.

Not done: pooling raw-store BOs (perf review #14). With 207, BC1-BC5 make
no raw BOs by default, so pooling would only help BC6H/BC7 and store mode.

## Shader cost (tools/spvstat, v11, cycles per block / registers)

| mode | bc3 (0x30000) | bc4 default (0x20000) | bc4 at 0xc30000 (= bc3) |
|---|---|---|---|
| BC1 | 58.25 / 59 | 50.81 / 58 (17 spills) | 58.25 / 59 |
| BC1A | 58.75 / 58 | 51.0 / 58 | 58.75 / 58 |
| BC2 | 85.75 / 59 | 78.19 / 57 | 85.75 / 59 |
| BC3 | 81.1 / 59 | 54.72 / 57 | 81.1 / 59 |
| BC4U / BC4S | 30.0 / 53, 29.8 / 55 | **10.25 / 16** | 30.0 / 53 |
| BC5U / BC5S | 54.3 / 57, 53.75 / 57 | **14.69 / 19** | 54.3 / 57 |

- BC4 and BC5 are now at or below 32 registers (2 threads). The fork's
  BC4 is 18-19 cycles / 41 registers and its BC5 is 35 / 64.
- BC1-BC3 stay at 57-59 registers. Going under 32 there needs opts 0:
  BC1 35.3 / 56, BC3 39.9 / 56, which costs about 6 dB.
- Host compile time per mode drops from 1.6-3.4 s to about 0.5 s (BC2
  about 1.5 s).
- The ASTC transcoder is unchanged at 645 cycles / 59 registers.

## Quality (tools/bc-bench, 1024², PSNR dB, random / gradient)

| format | bc3 | bc4 |
|---|---|---|
| BC1 | 20.19 / 41.17 | 20.19 / 40.46 |
| BC3 | 25.80 / 42.30 | 25.88 / 41.60 |
| BC4U | 33.26 / 52.19 | 35.40 / 51.71 |
| BC4S | 33.21 / 40.28 | 35.42 / 40.82 |
| BC5U | 33.26 / 51.92 | 35.42 / 51.21 |
| BC5S | 33.22 / 40.37 | 35.39 / 42.47 |

Every row is within 1 dB of bc3 or better. 0xc30000 matches bc3 exactly on
every row.

## Projection against RC4-lowmem (not measured)

Model: bc3 device upload times scaled by the change in cycles, at about
0.0385 ms per cycle (the BC1 to BC3 slope of the bc3 run). This does not
include the saving from dropping the raw write (207, estimated 0.2-0.4 ms)
or the gain from 2 threads on BC4/BC5.

| format | RC4-lowmem | bc3 measured | bc4 projected | vs RC4 |
|---|---|---|---|---|
| BC1_RGB | 2.89 | 3.45 | ~3.15, ~2.8-2.95 without raw write | borderline |
| BC3 | 3.22 | 4.33 | ~3.31, ~2.9-3.1 without raw write | borderline / likely |
| BC4U | 2.48 | 2.56 | ~1.8 or less | beats |
| BC4S | 1.17 | 2.66 | ~1.9, ~1.5-1.7 without raw write | **likely misses** |
| BC5U | 2.50 | 3.96 | ~2.4, ~2.0-2.2 without raw write | beats |
| BC5S | 2.80 | 3.60 | ~2.1 or less | beats |
| BC6H / BC7 | 8.42 / 5.91 | 7.93 / 5.67 | unchanged | already beats |

- **Upload time.** BC4S is the format most at risk. RC4's 1.17 ms is far
  below its own BC4U 2.48 ms. The fixed cost of our pipeline (about 1.5 ms
  per 2048² chain) is already above 1.17 ms.
- **bc_perf wall time.** bc3 took 57 s against RC4's 37 s, and most of the
  gap was per-process shader compiles with the cache disabled. With 204 the
  second process should hit the disk cache, and compiles are also about
  3-6x shorter (BC4/BC5). Expect the first start to be well under 57 s and
  the second near RC4. This is not measured.
- **Memory.** Without GS/XFB/tess draws, a VkDevice no longer commits the
  160 MiB of prerast arenas. BC1-BC5 images make no raw store BOs. Added:
  up to 3 x 512 KiB EAC table BOs per device (only for the kinds used).
  `mem_KiB` (image memory) is unchanged; I found no safe reduction.
- **Measuring memory.** bc_perf creates a device with texture compression
  features only, so it never had the arenas. That is why it now creates
  one extra full-feature device and prints `MEM` lines for both. Each line
  gives heapUsage, VmRSS/VmHWM, and the /dev/mali mapping Size/Rss, at
  before_device, full_feature_device, after_full_feature_device and end.
  Compare these between rc4 and rc5.

## Toggles

- `PANVK_BC_LM_OPTS`:
  - 0x20000: default
  - 0xc30000: bc3 encoder
  - 0x400000: EAC search
  - 0x800000: H mode on every block
- `PANVK_BC_RAW`: `lazy` (default), `store` (201-206 behaviour), `plane`
- `PANVK_GPU_PRERAST_ARENA=eager`: eager arenas on v10+ (the v9 behaviour)
- `PANVK_BC_BC6H`: `exact` (default), `astc` (UF16 to ASTC HDR),
  `astc_sf` (also SF16)

## Series checks

- Patch work tree: `/home/flux/src/mesa-bc` (branch bc2). Patches 204-208
  sit on 2a99b4a (203). The old state is kept as branch bc4-pre.
- Strict apply: a fresh worktree of 5a07217 plus
  `scripts/apply-patches.sh --profile g615-v11-csf` gives
  `OK applied=203`, with no offsets. For jm-v9 011 and 023 only the hunk
  headers were refreshed.

## Build

- **Driver build.** NDK r30 30.0.16248370, with
  `PANVK_KBASE_RELEASE=RC5-dev-bc4 PANVK_JOBS=2
  MESA=/home/flux/src/mesa-bc4-strict
  BDIR=/home/flux/build-bc4/bionic-r30-RC5-dev-bc4
  DDIR=/home/flux/build-out/2026-10-22-bc4
  HOST_TOOLS=/home/flux/repos/panvk-kbase-android/build/host-tools/bin
  flock /home/flux/build-cache/build.lock sh scripts/build-android.sh
  --profile g615-v11-csf --api 29 --ndk /opt/android-sdk/ndk/30.0.16248370`.
  - Thermal was SEVERE, so it ran with 2 jobs and a fresh BDIR.
  - The first build stopped on the 207 implicit declaration. After the fix
    and 208, an incremental rebuild finished with exit 0 and 0 warnings.
- **Driver .so.** `/home/flux/build-out/2026-10-22-bc4/libvulkan_panfrost.so`,
  26330112 bytes, sha256
  `a7b7c58e79ea61fb8f8fac6873edff8a8ad2d4eb7f3d1c5e7c4a3f8b803f1993`.
  - It contains the release string RC5-dev-bc4.
  - The strings for 205, 207 and 208 are present:
    PANVK_GPU_PRERAST_ARENA, "without raw blocks skipped", astc_sf.
- **APK.** Built in `apps/panvk-test` with `./gradlew assembleDebug
  -PpanvkSoRc4=/home/flux/build-out/rc4-lowmem/libvulkan_panfrost-android-aarch64.so
  -PpanvkSoRc5=<so above> -PpanprobeSuffix=bc4`, under the build lock.
  - Output: `/home/flux/build-out/apks/panprobe-bc4.apk`, sha256
    `68332b0898da6d2b7b94f7236bab2406d8d9759f78981b66c3b6b063561d101d`.
  - Package `dev.zenithblue.panvktest.bc4` (versionName 1.2.7).
  - The rc5 .so inside is a7b7c58e... (matches) and the rc4 .so is
    c395db09... (RC4-lowmem).

## Device plan (G615, not run)

1. Run `--es driver rc5 --es autorun all`. The target is 38/38 with
   bc_decode PASS; bc_decode forces `PANVK_BC_RAW=store`.
2. Run rc4/rc5 `bc_perf` alternately. Compare upload_gpu_ms against the
   table above, and the `MEM` lines (full_feature_device delta and /dev/mali
   Rss).
3. Start rc5 `bc_perf` twice. The log must not show "/data/.cache". A cache
   dir should appear under `/data/user/0/dev.zenithblue.panvktest.bc4/cache`.
   The second start should be faster in wall time.
4. Optional: run once with `PANVK_DEBUG=bc_timing`. Run once with
   `PANVK_BC_LM_OPTS=0xc30000` to get the bc3 encoder speed, which isolates
   the gain from the lazy raw store. Run once with `PANVK_BC_BC6H=astc_sf`.
5. If BC4S still misses RC4, the remaining cost is the fixed part. In that
   case, the next steps are to merge the per-level dispatches into one or
   to skip the pre-transcode barrier.
