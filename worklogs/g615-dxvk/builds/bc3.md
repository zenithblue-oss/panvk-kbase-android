# RC5-dev-bc3 build (2026-10-22): BC lowmem upload cost (csf-v11/203)

Goal (coordinator, after the G615 bc2 run in
`device-runs/2026-10-22-g615-bc2/results.md`): bc2 passes 38/38 but its BC
uploads are slower on the GPU than RC4-lowmem (BC1 2.9 -> 5.0 ms, BC3
3.2 -> 6.2, BC4S 1.2 -> 3.0, BC5U 2.5 -> 5.2, BC7 5.9 -> 6.9, BC6H 8.4 -> 8.8),
and bc_perf takes 76 s against 37 s. bc3 has to be faster and no bigger,
keep bc_decode passing, and recover quality.

## Where the extra upload time goes (rc4-lowmem vs rc5 bc2)

1. **Transcode ALU (BC1-BC5, the main cost).** I wrote a new
   `tools/spvstat` that prints Valhall shader-db stats for the shader the
   driver embeds. The bc2 transcoder takes 79 cycles per block for BC1,
   107 for BC2, 102 for BC3, 28.5 for BC4 and 52.8 for BC5. It is bound by
   integer work (CVT, and SFU for 382 32-bit IMULs) while the FMA unit sits
   idle. It uses 53-59 registers, which allows only 1 thread. A linear fit
   to the device numbers gives upload ≈ 1.9 ms fixed + 0.039 ms per
   cycle per 2048² chain. RC4's precompiled kernels have no source, so the
   comparison is only by time.
2. **The separate raw-store copy (201).** In an upload into a raw-store
   image, a vk_meta copy wrote the source into the raw plane before the
   transcode. That is one extra source read and two dispatches per mip
   region. It is part of the fixed ~1.9 ms, and it is the ~1 ms
   BC6H/BC7 delta: those formats have no transcode, but they do have the
   copy. RC4-lowmem sends the copy to a 64 KB scratch (a cache-hot dummy),
   so it pays little for it, and that is also why its raw/copy access fails.
3. **No disk shader cache in Android app processes.** The log shows
   "Failed to create /data/.cache for shader cache". Every process
   recompiles the per-mode transcode pipelines. The fully unrolled shaders
   take 0.8-5.8 s each to compile on the host compiler, and 8 modes per
   bc_perf process explain most of the +39 s wall time. RC4 has no
   compile step because its kernels are precompiled.
4. Barriers: the xcode upload path adds none (the ALL_COMMANDS barrier of
   perf review #9 is on the v9/JM `bc_emul` compute path). There is no
   second transcode pass and no re-upload.

## What changed (csf-v11/203, `patches/csf-v11/203-cut-bc-lowmem-upload-cost-on-the-gpu.patch`)

- The ETC1 and H error sums now run in fp32. This is exact, since
  16 * 3 * 255² < 2^24, so the bitstream is identical to bc2 and PSNR does
  not change. Valhall cycles per block:
  - BC1: 79 -> 58
  - BC1A: 80 -> 59
  - BC2: 107 -> 86
  - BC3: 102 -> 81
  - BC4: 28.5 -> 30
  - BC5: 52.8 -> 54.3

  BC4 and BC5 gain about 1.5 cycles from the raw-store branch and store.
- Fused raw store: the ETC2/EAC transcoder writes the raw BC blocks itself.
  New push constants are raw / raw_row_B / raw_slice_B, set to 0 when the
  image keeps no raw blocks. The source is read once and the vk_meta copy
  pass is gone. The BC6H ASTC transcoder and 3D images keep the separate
  copy. bc_decode keeps working: the raw blocks are still stored, in the
  same place.
- util/disk_cache: on Android, when neither MESA_SHADER_CACHE_DIR nor
  XDG_CACHE_HOME is set, the cache goes in the app cache dir
  `/data/user/<user>/<package>/cache`, tried before $HOME. Compiles then
  happen once per install rather than once per process. The fallback sits
  in util rather than in panvk_physical_device.c, so jm-v9 002 and 030
  apply without offset.
- Model estimate of upload time (shader only, before the fused-copy saving
  on the fixed part): BC1 ~4.2 ms, BC2/BC3 ~5.3 ms. bc2 measured 5.0, 6.2
  and 6.3 ms. **Not measured on a device.** Whether the fused copy brings
  BC1-BC5 under RC4 (2.9/3.2/5.1/3.2 ms) needs the G615 run below.

Memory: image memory (`mem_KiB`) is unchanged. The raw store BO is still
allocated only while command buffers reference it, then spilled (201).
Possible further wins, noted but not done:

- Suballocate raw-store BOs from a pool (perf review #14). One BO is
  created per full upload, and its pages are memset on the CPU with
  MEM_SYNC in kbase_kmod. This is a CPU and wall-time cost, not GPU time.
- Skip the raw store entirely for images that never use TRANSFER_SRC.
  DXVK sets it on nearly all images, so the gain is small.
- Make the G615 gpu_prerast arenas lazy (perf review #11: 160 MiB
  committed and memset per VkDevice). This is the largest memory gap to
  RC4-lowmem, and it is outside BC.

Quality: bc3 equals bc2 on every row. I wrote an exact encoder (EAC with
exact levels, 4 LUT candidates, LS base and ±1; ETC1 with exact scoring of
clamping tables; exact H and planar). It reaches ≥ lowmem PSNR on every
row at 2.1-3.2x lowmem speed on the host. But it compiles to 581-8590
Valhall cycles with heavy spills, which would be slower than lowmem on the
GPU, so it is kept only as a reference in `tools/bc-bench/hq/`. Quality
opt-ins that already exist: `PANVK_BC_LM_OPTS` bits 20-21 (EAC candidates
- 1; 0x330000 gives BC4U random 35.20 dB, lowmem 35.62, bc2 default
33.26), 0x40000 and 0x80000. Tables are in
`worklogs/bc-emul-perf/host-bench-2026-10-22.md` (bc3 column + GPU cost).

## Series checks

- Patch work tree `/home/flux/src/mesa-bc` (branch bc2), commit 203 on top
  of d2f5472 (202). SPIR-V regenerated by `bc/gen_spv.sh`. Only
  `panvk_bc_etc2_spv.h` is kept; the other headers differ only because
  glslang versions differ, so they were reverted.
- Build tree `/home/flux/src/mesa-bc3-strict` = fresh `git worktree add
  --detach` of 5a07217 from /home/flux/src/mesa +
  `scripts/apply-patches.sh --profile g615-v11-csf`: `OK applied=198`, no
  offsets.

## Build

- NDK r30 30.0.16248370, `PANVK_KBASE_RELEASE=RC5-dev-bc3 PANVK_JOBS=2
  MESA=/home/flux/src/mesa-bc3-strict BDIR=/home/flux/build-bc3/bionic-r30-RC5-dev-bc3
  DDIR=/home/flux/build-out/2026-10-22-bc3
  HOST_TOOLS=/home/flux/repos/panvk-kbase-android/build/host-tools/bin
  flock /home/flux/build-cache/build.lock sh scripts/build-android.sh
  --profile g615-v11-csf --api 29 --ndk /opt/android-sdk/ndk/30.0.16248370`.
  Thermal SEVERE, so 2 jobs. Fresh BDIR, 1168 ninja steps, exit 0, no
  warnings in the files 203 touches.
- so `/home/flux/build-out/2026-10-22-bc3/libvulkan_panfrost.so`, 25830360
  bytes, sha256
  `41340bf6ca09bfbb3f8fdcbdd34dcae83ec9d43d99f1599cb3616300651c98cf`
  (release string RC5-dev-bc3 present).
- APK: `apps/panvk-test` `./gradlew assembleDebug
  -PpanvkSoRc4=/home/flux/build-out/rc4-lowmem/libvulkan_panfrost-android-aarch64.so
  -PpanvkSoRc5=<so above> -PpanprobeSuffix=bc3` (current MainActivity,
  swapchain fix included), under the build lock. Output
  `/home/flux/build-out/apks/panprobe-bc3.apk`, sha256
  `989207aa0390a073ea6cc6f937fa148a4b95d43331671a19eb218f1d1a7c055b`,
  package `dev.zenithblue.panvktest.bc3`. Inside: rc5 = 41340bf6... (matches),
  rc4 = c395db09... (RC4-lowmem).

## Device plan (G615, not run)

`--es driver rc5 --es autorun all` (target 38/38, bc_decode PASS). Then
alternate rc4/rc5 `bc_perf` like the bc2 run, and check upload_gpu_ms
against RC4-lowmem and bc2. Run once with `PANVK_DEBUG=bc_timing`. In
logcat, check for "shader cache" / no "/data/.cache" failure, and that the
second bc_perf process is faster in wall time (cache hit). If BC1-BC5 are
still above RC4, the next lever is register pressure: 1 thread at 53-59
registers, 2 threads needs ≤ 32.
