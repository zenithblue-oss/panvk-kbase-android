# G615 BC lowmem v2 device checklist (RC5-dev-bc-r30)

One device session. Plan and expectations: `lowmem-v2.md` ("Device test plan").
Build record: `worklogs/g615-dxvk/builds/2026-10-21-bc-r30-build.md`.

Allowed adb commands only: `install --no-streaming`, `am start`, `logcat`,
`pull` (plus `push` of the harness files in section C if the operator already
uses the `/data/local/tmp/bcperf/` workflow). No run-as, setprop, settings
put, svc or input.

## Artifacts (all under /home/flux/build-out)

- `apks/panprobe-bc.apk`: PanProbe, slot rc4 = RC4-lowmem release .so, slot
  rc5 = r30 BC .so (RC5-dev-bc-r30). Application id
  `dev.zenithblue.panvktest.bc` (installs next to the other PanProbe builds).
- `2026-10-21-bc-r30/libvulkan_panfrost.so`: stripped BC driver.
- `2026-10-21-bc-r30/*.zip`: launcher/adrenotools zips for the game test
  (adrenotools, winlator, bannerlator, cmod, ludashi, gamehub).
- `rc4-lowmem/libvulkan_panfrost-android-aarch64.so`: RC4-lowmem reference.

## A. Install and PanProbe run

1. `adb install --no-streaming -r panprobe-bc.apk`
2. `adb logcat -c`
3. Driver slot rc5 (BC r30): `adb shell am start -n dev.zenithblue.panvktest.bc/dev.zenithblue.panvktest.MainActivity --es driver rc5 --es autorun all`
4. When done, slot rc4 (RC4-lowmem): same command with `--es driver rc4`.
5. `adb logcat -d -s panvk:* PanProbe:* > bc-probe.log` (or full `adb logcat -d`).
6. `adb pull` the run output PanProbe prints/shares (path shown in the app's log / Share button).

Check: driverInfo is `RC5-dev-bc-r30` for rc5; log has
`panvk: BC emulation on, lowmem`; first spill logs
`panvk: BC raw store spills to <dir>` (else the "stay resident" warning).

## B. Env-var A/B runs

An APK cannot take env vars without setprop/run-as, so these use the
bcperf harness (section C) with the env set on the command line of the
test process. Run each configuration for the r30 BC .so; keep the RC4-lowmem
.so as the baseline.

| run | env |
|---|---|
| default | (none) |
| timing | `PANVK_DEBUG=bc_timing` (log lines `panvk: bc_timing <fmt> <w>x<h> regions=N layers=N xcode\|decode: X us`) |
| raw plane (= 197) | `PANVK_BC_RAW=plane` |
| BC6H exact | `PANVK_BC_BC6H=exact` |
| full exact | `PANVK_BC_MODE=exact` (regression gate, bc_decode 16/16) |
| workgroup sweep | `PANVK_DEBUG=bc_timing PANVK_BC_LM_WG=16x4`, then `32x2`, then `4x4` (default 8x8) |

## C. Harness (existing workflow)

Binaries `bc_decode`, `bc-perf` etc. and the test ICD json live in
`/data/local/tmp/bcperf/`; load the .so by path through the ICD json
(`VK_DRIVER_FILES`), as in `PROGRESS.md`. Plan items 2-5 and 7 of
`lowmem-v2.md`: bc_decode + bc_verify (16 formats), raw-store test, copy
matrix, bc_perf (image memory per format, upload ms, sampling modes A-E),
CTS filters `*bc*`.

## D. Game test (MiSide / NFS)

1. Import the BC r30 zip into the launcher (adrenotools/winlator zip), keep
   RC4-lowmem as the comparison driver.
2. Per driver: play ~5 minutes, `DXVK_HUD=memory,fps` visible.
3. `adb logcat -d` after the run.
4. Repeat the BC r30 game run with `PANVK_BC_RAW=plane` and
   `PANVK_BC_BC6H=exact` set in the launcher's environment variables.

## What to capture

- logcat for every run (`panvk:` lines: BC emulation on, raw store spill dir,
  bc_timing, any warning/ERROR/crash/tombstone).
- bc_decode/bc_verify result per format (16 formats, each mode).
- bc_perf table: image MiB per format (expect v2 column of lowmem-v2.md:
  BC1 2.7, BC3 5.4, BC7 5.4, BC6H UF 5.6), upload ms, sampling A-E ms,
  BC6H UF ASTC vs exact (0.84 vs 0.33 ms mode E is the open question).
- Peak RSS (VmHWM) vs RC4-lowmem (270 MB) and exact path (172 MB).
- Game: DXVK HUD memory and fps for each driver, spill-file size, any
  visual corruption (BC6H skies/lightmaps especially), screenshots via the
  app, not via input injection.
- PanProbe results for both slots (pull the shared output).
