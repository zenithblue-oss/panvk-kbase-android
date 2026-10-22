# 2026-10-22 v7M: v7L + v6 FTL fixes (RC5-dev-v7M)

Tree: `/home/flux/src/mesa-v7J`, branch `v7M`, HEAD `bd6d38d`, on top of v7K `d160802` (0006-0062). Build only; no device run yet.

## Patches relative to v7K

- 0063-0073: as in `builds/v7L.md` (VS image table redraw, shader cache key knobs, v6 IDVS/clock/import sync/CLPER_OLD, min/max helper, descriptor dirty tracking, alt rebind, split tiler heap, deferred occlusion EndQuery).
- 0074 (jm-v7 101, v6): robustImageAccess2 out-of-bounds alpha taken from the view's hardware format (format-less storage image loads, uniform texel buffer fetches). Targets the 10 FTL G72 r32ui/r32ui_nofmt alpha 0 vs 1 failures. v7 unchanged.
- 0075 (jm-v7 102, v6, compiler): Bifrost texture offset/MS-index staging word packed with 32-bit AND/shift/OR on v6 instead of 8-bit vector ops. Targets FTL G72 sampler `gather_offsets` (INSTR_INVALID_ENC). v7+ unchanged. Cause to be confirmed on the device.

Patch files in `patches/jm-v7-wip` match `git format-patch` of the Mesa commits (only the `[PATCH n/m]` subject numbering differs). README lists both.

## Build

- Command (thermal NONE, 4 jobs, fresh BDIR), exit 0:
  ```
  flock /home/flux/build-cache/build.lock env MESA=/home/flux/src/mesa-v7J BDIR=/home/flux/build-v7feat/bionic-r30-v7M HOST_TOOLS=/home/flux/repos/panvk-kbase-android/build/host-tools/bin PANVK_KBASE_RELEASE=RC5-dev-v7M PANVK_JOBS=4 scripts/build-android.sh --profile g615-v11-csf --api 29 --ndk /opt/android-sdk/ndk/30.0.16248370
  ```
- .so: `/home/flux/build-out/2026-10-22-v7M/libvulkan_panfrost.so`, sha256 `7632e6a6151340499a1fb2baa2c286ad5bcb5fee577e11cbd9f007f6c53ffaae`. `strings`: `PanVK-kbase RC5-dev-v7M (Mesa 26.3.0-devel (git-bd6d38d376))`.
- APK:
  ```
  apps/panvk-test/gradlew -p apps/panvk-test assembleDebug -PpanvkSoRc4=/home/flux/build-out/rc4/libvulkan_panfrost-android-aarch64.so -PpanvkSoRc5=/home/flux/build-out/2026-10-22-v7M/libvulkan_panfrost.so -PpanprobeSuffix=v7m
  ```
  - `/home/flux/build-out/apks/panprobe-v7M.apk`, sha256 `17864a28af9502303c90b27aadd557f172a6f5d77a4a523e2c1f9f1e8d5d8ae5`. Includes the Firebase game-loop MainActivity.
  - Package `dev.zenithblue.panvktest.v7m` (aapt2 badging, versionCode 15, versionName 1.2.7).
  - `lib/arm64-v8a/libvulkan_panfrost_rc5.so` sha matches the .so (7632e6a6...).

## Firebase (not yet run)

```
gcloud firebase test android run --type game-loop --app /home/flux/build-out/apks/panprobe-v7M.apk --scenario-numbers 2 --device model=starlte,version=29 --timeout 30m --directories-to-pull /sdcard/Android/data/dev.zenithblue.panvktest.v7m/files --results-dir panvk-v6-starlte-v7M
```

Check on G72: sampler `gather_offsets`, robust_image_access2 r32ui cases, plus the v7L items (vertex_stores, shader_arith, vkd3d_heap).
