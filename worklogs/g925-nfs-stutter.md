# G925 (Tab S11 Ultra) NFS Most Wanted stutter

Date: 2026-10-09. Device: Galaxy Tab S11 Ultra SM-X930, Immortalis-G925 (v13), Samsung RTL via `~/rtl-rdb/rdb`
(adb serial `localhost:<port>`, default adb server). Work dir: `/var/tmp/panvk/rtl/g925-nfs/`.

## Inputs

- Driver: no `patches/` commits after tag `panvk-kbase-v0.1.0-rc2`, so the rc2 .so is the newest:
  `/var/tmp/panvk/release-rc2/driver-assets/libvulkan_panfrost-android-aarch64.so`,
  sha256 `725b13c7...`, BuildID `2c4da3d9`.
- PanPlay 1.2.5 (versionCode 29, perf recording) built with `-PpanvkSo=<rc2 .so>`:
  `/var/tmp/panvk/rtl/g925-nfs/panplay-vc29-rc2.apk`. APK .so sha256 matches rc2.
- Game: `/home/abhaybyte/games/Need.for.Speed.Most.Wanted.zip` (2.27 GB), unzipped to
  `/sdcard/Games/Need.for.Speed.Most.Wanted`, shortcut `gf5eb79d4` (DXVK_HUD=full, FEX Extreme, i386).

## Session 1 (2026-10-09, cut short)

- Push 2.27 GB over RDB: 427 s (5.1 MB/s). `install -r -g`: OK (first install, PanPlay was absent).
- Cold run launched 09:35:53 (host time); `speed.exe`, wineserver, winedevice running.
- Screen went to sleep 5 min after the last injected input (`mLastSleepReason=timeout`), the
  game was paused behind the keyguard. The keyguard has a 5 s timeout
  (`mUserActivityTimeoutOverrideFromWindowManager=5000`), so every wake needs
  `input keyevent 224` + an unlock swipe at once. Fix in the scripts: poke `keyevent 224` every
  60 s during runs. (`XServerActivity` does not set keep-screen-on; with a real controller or touch
  this never shows.)
- RTL device disconnected from RDB (`Disconnected device: SM-X930` in `~/rtl-rdb/rdb.log`)
  before any gameplay frames were captured. No perf data yet.

## Session 2 (2026-10-09 10:07, dropped)

- Same serial `localhost:56562`, unit `R32Y5008VJW`, but PanPlay and the game were gone (wiped).
- Re-install OK (10:08:46). Re-push OK, 432 s (10:14:43). `nfsmw.zip` is not unzipped yet.
- RDB showed `Disconnected device: SM-X930` right after the push. No run happened.
- Both drops came after a backgrounded on-device loop (`adb shell '(while ...; done) &'`), but
  there is not enough data to say that caused it. Use the host-side `keepalive.sh` instead
  (`touch STOP_KA` stops it).

## Session 3 (2026-10-09 17:48-18:33, PanPlay vc30 + rc2)

- Run 2: career menu, then `VK_ERROR_DEVICE_LOST` (DxvkSubmissionQueue, Presenter recreating
  swapchain in a loop). Before it: `RtlpWaitForCriticalSection ... main process heap section wait
  timed out`. Not reproduced in runs 3-4. kbase fault detail is not in logcat (no dmesg access).
- Run 3, race, uncapped, last 60 s: avg 10.6 ms (94 FPS), p99 20 ms, max over 25 ms only 1x,
  compiler_busy 0, 36 pipelines (no shader stutter), GPU 60-70 %.
  20 % of frames are over 16.7 ms. Frametimes alternate in pairs: 17, 17, 4, 4 ms.
- **Cause: uneven frame pacing (micro-stutter), not GPU load or shader compiles.** Frames are
  delivered in bursts: two slow, two fast. The average FPS looks high but motion judders.
- Run 4, same race, PanPlay `fpsLimit` 60: avg 16.67 ms, p99 19.9 ms, 0.75 % frames over 20 ms,
  2 frames over 25 ms in 40 s, GPU 44 %. Pacing is flat at 14-18 ms. The stutter is gone.
- Fix: use a 60 FPS cap for NFS MW on G925. Candidate: PanPlay default `fpsLimit` 60 for d3d9 games.

## Scripts

- `run.sh start <tag> [secs]`: wake/unlock, force-stop, logcat -c, on-device cpu/gpu freq +
  thermal recorder every 2 s, launch shortcut.
- `run.sh stop <tag>`: screencap, logcat, recorder, `files/perf` + `files/logs`, cache stats.
- `cap.sh <name>`: screencap + 900 px jpg in `shots/`.
- Known: in zsh, `A="adb -s X"; $A ...` does not split; scripts use a function `A()`.

## Next session

1. `adb devices` shows `localhost:<port>` -> update serial in `run.sh`/`cap.sh` if the port changed.
2. Game files and PanPlay vc29 are on the device if the same RTL device is assigned (else re-push, ~7 min).
3. Cold run: menus by `input keyevent --longpress` (Enter/arrows), Quick Play > circuit race, drive
   2-3 min, `run.sh stop cold`. Warm run the same. Analyze `files/perf/<id>.csv` F/S rows.
