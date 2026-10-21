# RC5-dev-v7 vs RC4 on the Galaxy A06 (G52 MC2 r1, v7): investigation (2026-10-21, no device)

Question: the user reports the released RC4 (`977c0c4c…`, BuildID `6654fada`, NDK r30) at PanProbe 38/38 on the A06.
Our RC5-dev-v7 (`a1e68b59…`, BuildID `728f33ee`, NDK r29), bundled in `panprobe-debug.apk`, scored the "older recorded"
result (about 14/38). The app also showed "RC3".

## Short answer

1. **The "RC3" in the app is a stale label, not an RC3 driver.** `panprobe-debug.apk` (05:14, `c68ac7e4…`) contains
   `lib/arm64-v8a/libvulkan_panfrost.so` with sha256 **`a1e68b59…` = RC5-dev-v7**. Its `assets/bundled-driver.json`,
   however, is the launcher's pinned **RC3-lowmem** manifest (`"name": "PanVK Kbase — RC3 lowmem"`, sha256 pin
   `f550a006…`, profile `g615-v11-csf`). At that commit (cf62533), `build.gradle.kts` copied
   `apps/panvk-launcher/.../bundled-driver.json` and only *warned* when `-PpanvkSo` did not match the pin, so the UI and
   the upload record named RC3-lowmem. The tests themselves ran the RC5-dev-v7 binary. Nothing in the app reads
   `profile` or other manifest fields to change behaviour; they are display and record metadata only. A real
   RC3/RC3-lowmem binary could not have scored 14: it has no jm-v7, so the v7 env gate refuses the G52 and no tests run
   (0 pass, "Mali Bifrost (v6/v7) is not supported yet").
   Fixed already in 8909683 (v2 APK: per-slot metadata derived from the actual `.so`).
2. **RC5's ~14/38 is the expected v7 result, and it matches RC4's own recorded result.** The README and PROGRESS RC4 row
   say "PanProbe 14/38 on Galaxy A06 (G52 MC2) and A14 (G52 MC1)". The 2026-10-16 baseline (a065, v9fix
   `de4827a8`, same jm-v7 001-005) was 14 pass / 19 fail / 5 skip. The failures are missing DXVK features (GS,
   XFB, tessellation, robustness2, MDI), which are only in `patches/jm-v7-wip`, not in RC4 and not in RC5-dev-v7.
3. **A genuine 38/38 from the RC4 binary on PanVK v7 is not possible from the code.** `rc4-binary-analysis.md`
   Result 4 shows `panvk_v7_get_physical_device_features` and `panvk_v7_get_physical_device_extensions`
   **instruction-identical** between RC4 and this rebuild (and RC4 has none of the jm-v7-wip strings: no
   `gs_strip_i`, no `panvk xfb passthrough`, no `no_tess`). RC4 therefore reports the same features as RC5 on the G52.
   The geometry, tessellation, xfb, gs_* and robustness2 tests fail or SKIP on both. **The 38/38 is not a PanVK-v7
   result from this binary.** No artifact of it exists: `test-results/2026-10-21-a06/rc4-ref/` is empty and
   `summary.txt` has only a header. Most likely it is one of:
   - a different device or run (RC4 is 39/39 on the G615; 38/38 is the v9 to v13 figure);
   - the **System Vulkan** slot (vendor Mali). The vendor-ICD guard does not apply to SYSTEM
     (`vendorIcd = driverTypeState != SYSTEM && …`), so vendor passes count as PASS there;
   - a misread of the screen.
4. **The build configuration is the same apart from the NDK** (candidate 1). RC4 and `a1e68b59` have identical
   NEEDED (`liblog libnativewindow libsync libm libz libdl libc`), SONAME `libvulkan_panfrost.so`, RUNPATH
   `$ORIGIN/../../android_stub`, FLAGS `SYMBOLIC BIND_NOW`, API 29 in `.note.android.ident` (r30 `16248370` vs
   r29 `14206865`), and the same exported symbols. They have the same driver string set (so assert text, ndebug and
   buildtype match), and the same 191-patch series (`builds/2026-10-21-v7-build.md`; RC4 is `-Dbuildtype=release` by
   the same `scripts/build-android.sh`). The only differences are r29's statically linked libc++abi demangler and
   libunwind (+79 KB `.text`, imports `__register_atfork` and `pthread_rwlock_*`) and RC4's import of
   `__stack_chk_fail`. Neither changes which features are reported.
5. **The test app is unchanged where it matters** (candidate 3). The 38 TestCases are the same since the release-era app
   (b97ae37, 2026-10-16). The app changes since then: driver slots and metadata (8909683), the large_draw ×3 timeout
   *only with Mesa debug on* (a06bb31), build-only fixes (5dce7b9, 54f3294, cf62533), and the large_draw test source
   (csf-v11/191, 195 era). None of these can move 24 tests.

## Other facts found

- **The host-side install of `panprobe-debug.apk` failed** (`/home/flux/build-logs/rtl-a06.log`: "adb: failed to install
  … install failed"). So the copy the user ran was installed another way. Its exact identity is not recorded on the
  host. A 14/38 score still proves it was a jm-v7 driver (RC4 or RC5), because RC3 and RC3-lowmem give 0 on v7.
- **Two different A06 units or firmwares were used today:** `rtl-a06.log` has kernel `4.19.191-29401052-abA065FXXS3AYB1`
  (Feb 2025). `test-results/2026-10-21-a06/device.txt` has `…abA065FXXS4AYE2` (May 2025), with Mali GLES
  `r32p1-01eac0`. If RC4 and RC5 ran on different RTL units, the kbase driver may differ too. Record the firmware in
  every run.
- v2 APK (`panprobe-debug-v2.apk`, `b99e66b7…`) is correct: `libvulkan_panfrost_rc4.so` = `977c0c4c…` (RC4 release, pin
  matches), `libvulkan_panfrost_rc5.so` = `a1e68b59…` (RC5-dev-v7), and per-slot `bundled-driver.json` with the real
  BuildIDs and driverInfo.

## NDK r30 (candidate 4): staged, build not run

- lzhiyong/termux-ndk publishes only r29 for aarch64. RC4's `.comment` points to the HomuHomu833 toolchain. Its
  `android-ndk-custom` release `r30` has `android-ndk-r30-aarch64-linux-musl.tar.xz`, 177,989,192 bytes, sha256
  `632120d0571ee32f108e7abad877370e05bb4e4260f2b22a239991957658539a`. **This matches GitHub's asset digest.** It was
  downloaded to `/home/flux/downloads/ndk-r30/` and extracted to **`/opt/android-sdk/ndk/30.0.16248370`**.
  `source.properties` says `Pkg.Revision = 30.0.16248370`, **the exact build number in RC4's `.note.android.ident`**.
- Its host dir is `toolchains/llvm/prebuilt/linux-arm64` (not `linux-aarch64`). `scripts/build-android.sh` now also
  looks for `linux-arm64` on aarch64 hosts.
- **The r30 build did not run.** Running the downloaded toolchain was refused by the session's permission classifier
  ("code from external"), so the user must approve it. The Mesa tree `/home/flux/src/mesa` is clean at `d495f50`
  ("v7-base: 191-patch g615-v11-csf series applied"), the same tree as `a1e68b59`. The command (one line, check
  `nativecode-agent telemetry` first; use `PANVK_JOBS=2` if thermal is SEVERE or worse):

  ```sh
  flock /home/flux/build-cache/build.lock env MESA=/home/flux/src/mesa PANVK_JOBS=4 PANVK_KBASE_RELEASE=RC5-dev-v7-r30 BDIR=/home/flux/repos/panvk-kbase-android/build/android-bionic-r30 DDIR=/home/flux/build-out/2026-10-21-v7-r30 /home/flux/repos/panvk-kbase-android/scripts/build-android.sh --profile g615-v11-csf --api 29 --ndk /opt/android-sdk/ndk/30.0.16248370
  ```

  A fresh `BDIR` keeps the r29 meson dir untouched. The static deps in `work/android-deps` (libdrm and others) were
  built with r29. RC4 shows the same thing: its `.comment` has the r29 clang 21 lines from linked static archives.
  Expected result: the same strings as `a1e68b59` minus the demangler and libunwind text, close to RC4's section
  sizes (`.text` 0x86f5e0). Then bundle it as a third slot or a separate `-PpanprobeSuffix` APK.
  Given points 3 and 4, an r30 build is **not expected to change the A06 count**.

## What the next device session should run (A06, record firmware `ro.build.PDA` / `uname -a`)

1. Install **`/home/flux/build-out/apks/panprobe-debug-v2.apk`** (`b99e66b7…`). Uninstall any other
   `dev.zenithblue.panvktest` first, because the debug and release signatures differ and an update install fails.
   Confirm with `pm path` and `dumpsys package dev.zenithblue.panvktest | grep versionName`.
2. In that same APK, run **RC4** then **RC5**: `--es driver rc4 --es autorun all`, then `--es driver rc5 --es autorun all`.
   Capture each from logcat `PANPROBE_RESULT driver=<driverInfo> pass= fail= skip=` and `PANPROBE_TEST` lines, and
   pull `Android/data/dev.zenithblue.panvktest/files/runs/` (no run-as). The `DRIVER slot=… driverInfo=…` line proves
   which binary ran.
3. If RC4 really gives 38/38 while RC5 gives 14, keep both runs' per-test logs. Check that the 38 runs show
   `MESA: info: panvk:` lines and `ICD device=Mali-G52 …` with `geometryShader=1`. Without those lines, it was not
   PanVK. Also run the **System Vulkan** slot once to see what the vendor driver scores.
4. Only then is an r29 vs r30 comparison useful: the RC5-dev-v7-r30 build (above), bundled next to `a1e68b59`.
5. Expected on both RC4 and RC5: about 14 pass / 19 fail / 5 skip (the 2026-10-16 set). Real v7 progress needs
   `patches/jm-v7-wip` 0006-0013 (rc4-binary-analysis plan step 2).
