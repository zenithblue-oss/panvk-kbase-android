# G615 BC lowmem v2 device checklist (RC5-dev-bc-r30)

One device session. Plan and expectations: `lowmem-v2.md` ("Device test plan").
Build record: `worklogs/g615-dxvk/builds/2026-10-21-bc-r30-build.md`.

Allowed adb commands only: `install --no-streaming`, `am start`, `logcat`,
`pull` (plus `am force-stop <pkg>`). No adb push, no adb shell harness, no run-as, setprop, settings
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

## B. Env-var A/B runs (PanProbe intent extra, no harness)

PanProbe takes `--es env "KEY=VAL;KEY2=VAL2"`. Only `PANVK_*`, `MESA_*` and
`PAN_*` keys are accepted (others are logged as `PANPROBE_ENV rejected`). Each
pair is logged as `PANPROBE_ENV key=... val=...` and is placed in the
environment of every PanProbe child process that loads the driver .so, so it
is set before the .so is loaded. Driver env is read once per process, so start
fresh for each env set: `adb shell am force-stop dev.zenithblue.panvktest.bc`
before every run (`am` command, allowed). Use `adb logcat -c` before and
`adb logcat -d -s panvk:* PanProbe:* > <name>.log` after each run.

BC test: `--es autorun bc_perf` (image memory per format, upload, sampling,
copy; with `PANVK_DEBUG=bc_timing` it emits the `bc_timing` lines) and
`--es autorun bc_decode` (bc_decode 16/16 correctness). `autorun` takes `all`
or one test name.

Per run (replace ENV and TEST; wait for `AUTORUN DONE` / `PANPROBE_RESULT` in logcat):

1. `adb shell am force-stop dev.zenithblue.panvktest.bc`
2. `adb logcat -c`
3. `adb shell am start -n dev.zenithblue.panvktest.bc/dev.zenithblue.panvktest.MainActivity --es driver rc5 --es env "ENV" --es autorun TEST`
4. `adb logcat -d -s panvk:* PanProbe:* > RUN.log`

Run each configuration on rc5 (BC r30); the baseline is `--es driver rc4`
(no env) with the same TEST.

| run | ENV | TEST |
|---|---|---|
| default | (omit `--es env`) | bc_perf, then bc_decode |
| timing | `PANVK_DEBUG=bc_timing` (log lines `panvk: bc_timing <fmt> <w>x<h> regions=N layers=N xcode\|decode: X us`) | bc_perf |
| raw plane (= 197) | `PANVK_BC_RAW=plane` | bc_perf |
| BC6H exact | `PANVK_BC_BC6H=exact` | bc_perf, bc_decode |
| full exact | `PANVK_BC_MODE=exact` (regression gate, bc_decode 16/16) | bc_decode |
| workgroup sweep | `PANVK_DEBUG=bc_timing;PANVK_BC_LM_WG=16x4`, then `32x2`, then `4x4` (default 8x8) | bc_perf |

Example: `--es env "PANVK_DEBUG=bc_timing;PANVK_BC_LM_WG=16x4" --es autorun bc_perf`.
Quote the whole value; `;` separates pairs, so one pair cannot hold a `;`.

## C. Other tests

Plan items 2-5 and 7 of `lowmem-v2.md` map to PanProbe tests: bc_decode
(bc_decode + bc_verify, 16 formats), bc_perf (image memory per format, upload
ms, sampling modes A-E). The raw-store test, the copy matrix and CTS `*bc*`
filters need the bcperf harness binaries and are skipped in this
safe-adb-only session.

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
