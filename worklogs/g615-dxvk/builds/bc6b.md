# RC5-dev-bc6b build (2026-10-22)

bc6b = bc6 (csf-v11 through 212) with the AMENDED patch 209 (commit 7b46f90):
util_get_cpu_caps is primed before the queue workers start and in the fork
prepare handler, which fixes the bionic pthread_once hang in forked children
(vkd3d_heap first case). The APK also has the PanProbe buildEnv() fix
(874033e: `--es env` overrides the built-in defaults). Not run on a device yet.

## Tree
- `/home/flux/src/mesa-bc6b-strict`: worktree at base 5a07217f plus
  `scripts/apply-patches.sh --profile g615-v11-csf`, `OK applied=207`, clean.
- `src/util/u_queue.c` contains the util_get_cpu_caps priming (lines 117, 197).

## Build
- `PANVK_KBASE_RELEASE=RC5-dev-bc6b PANVK_JOBS=4 MESA=/home/flux/src/mesa-bc6b-strict
  BDIR=/home/flux/build-bc6b/bionic-r30-RC5-dev-bc6b DDIR=/home/flux/build-out/2026-10-22-bc6b
  HOST_TOOLS=/home/flux/build-cache/host-tools/5a07217f... flock build.lock sh scripts/build-android.sh
  --profile g615-v11-csf --api 29 --ndk /opt/android-sdk/ndk/30.0.16248370` (thermal NONE).
- Driver: `/home/flux/build-out/2026-10-22-bc6b/libvulkan_panfrost.so`, 27924088 bytes,
  sha256 `1791bdd6ffbb53caaa1bf1cd774b2c0f73b70fb38c0dc2df220c8bbe4478ca27`.
  strings: `PanVK-kbase RC5-dev-bc6b (Mesa 26.3.0-devel (git-5a07217f03))`.
- APK: `./gradlew assembleDebug -PpanvkSoRc4=<rc4-lowmem> -PpanvkSoRc5=<so above> -PpanprobeSuffix=bc6b`
  under the build lock. Output `/home/flux/build-out/apks/panprobe-bc6b.apk`, sha256
  `dba068e964b69fad904f234389c2892ce296ca1789e1f8527535f2368f7d539a`.
  - aapt2: `dev.zenithblue.panvktest.bc6b`, versionCode 15, versionName 1.2.7.
  - Inside: rc5 .so 1791bdd6... (matches), rc4 .so c395db09... (RC4-lowmem, matches).

## Device plan
Same as bc6 (see bc6.md), plus: the vkd3d_heap first case must no longer hang.
