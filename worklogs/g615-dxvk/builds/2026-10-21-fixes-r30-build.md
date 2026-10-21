# 2026-10-21 fixes build with NDK r30, RC5-dev-fixes-r30 (PanProbe v9)

- Source: same tree and series as 2026-10-21-fixes-build.md (/home/flux/src/mesa-fixes, fixes-work, profile g615-v11-csf, 194 patches incl. csf-v11/198 and 199). Only the toolchain and release string differ.
- Why r30: RC4 reproduces only with NDK r30.
- Toolchain: NDK 30.0.16248370 (linux-arm64) bin first on PATH, API 29, HOST_TOOLS=/home/flux/repos/panvk-kbase-android/build/host-tools/bin.
- Build: `MESA=/home/flux/src/mesa-fixes BDIR=/home/flux/build-fixes/bionic-r30 DDIR=/home/flux/build-out/2026-10-21-fixes-r30 PANVK_JOBS=4 PANVK_KBASE_RELEASE=RC5-dev-fixes-r30 flock /home/flux/build-cache/build.lock ./scripts/build-android.sh --profile g615-v11-csf --api 29 --ndk /opt/android-sdk/ndk/30.0.16248370`. Thermal NONE (telemetry), 4 jobs, fresh BDIR, exit 0.
- Artifact: /home/flux/build-out/2026-10-21-fixes-r30/libvulkan_panfrost.so (stripped by the script), 23875336 bytes
- sha256: 82f67f5edfebc5e63ddfeecae98e02cdd653a2068d47f0a728873069e323724c
- driverInfo string RC5-dev-fixes-r30 present (1 occurrence).
- PanProbe APK: `cd apps/panvk-test && flock /home/flux/build-cache/build.lock ./gradlew assembleDebug -PpanvkSoRc4=/home/flux/build-out/rc4/libvulkan_panfrost-android-aarch64.so -PpanvkSoRc5=/home/flux/build-out/2026-10-21-fixes-r30/libvulkan_panfrost.so -PpanprobeSuffix=v9`. BUILD SUCCESSFUL.
  - /home/flux/build-out/apks/panprobe-v9.apk (+ .sha256), 42114821 bytes, sha256 fc0dc40f01505202f3598073fbb0220b31fd6725139d6074e09269376e80f0ee, application id dev.zenithblue.panvktest.v9
  - Bundled (unzipped, verified): lib/arm64-v8a/libvulkan_panfrost_rc4.so = 977c0c4c0803b9940c453d691f50f14c9f8f732fe945d93bdde637880befd791 (matches rc4 reference); libvulkan_panfrost_rc5.so = 82f67f5e... (matches the r30 fixes .so).
- Device test: not run.

## v9 device test steps (Mali-G57/G68, jm-v9)

Safe adb only: `install --no-streaming`, `am start`, `am force-stop`, `logcat`, `pull`. Settings go through `--es env "KEY=VAL;..."` (PANVK_/MESA_/PAN_ keys only). Force-stop before each run, since driver env is read once per process.

1. `adb install --no-streaming -r panprobe-v9.apk`
2. `adb logcat -c`
3. Full run on rc5: `adb shell am start -n dev.zenithblue.panvktest.v9/dev.zenithblue.panvktest.MainActivity --es driver rc5 --es autorun all`. Wait for `AUTORUN DONE` / `PANPROBE_RESULT`, then `adb logcat -d -s panvk:* PanProbe:* > v9-rc5.log`. Check driverInfo is RC5-dev-fixes-r30.
4. Baseline: `am force-stop dev.zenithblue.panvktest.v9`, `logcat -c`, same start with `--es driver rc4`, save `v9-rc4.log`.
5. ETC2/ASTC check: in the logs look at the vkinfo/format section for ETC2 and ASTC LDR support (sampled image, blit src/dst, transfer) on rc5 and compare with rc4. Both should report the same support or rc5 more; no format dropped.
6. VMR samples: `--es autorun vmr_secondary` on rc5 and rc4 (force-stop and `logcat -c` between runs); compare sample counts/pass lines, rc5 must not regress.
7. 128-bit BC blit: `--es autorun bc_decode` on rc5 (BC1-BC7 block copy/blit incl. the 128-bit formats BC2/3/5/6H/7) and rc4; expect 16/16 on rc5. Optionally `--es autorun bc_perf` with `--es env "PANVK_DEBUG=bc_timing"`.
8. `adb pull` any run output path that PanProbe prints. Report pass counts per run (expect no regression vs. rc4), plus any `panvk:` error or fault lines.
