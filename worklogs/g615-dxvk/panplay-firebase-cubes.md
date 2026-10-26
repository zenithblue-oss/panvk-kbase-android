# PanPlay cubes on Firebase Test Lab (game loop) and driver env A/B

Status 2026-10-26: harness and driver patch built and validated on the host only. **No Firebase run yet.** The first runs are planned with the perf-patched driver build (115), as an A/B in one execution (scenario 4). The 114 baseline runs were cancelled.

Code:
- PanPlay harness: `apps/panvk-launcher/app/src/main/java/dev/zenithblue/panvklauncher/GameLoop.kt` (Android part) and `GameLoopMath.kt` (config, scenarios, fps math; JVM tests in `app/src/test/.../GameLoopMathTest.kt`). Config: `app/src/main/assets/gameloop.json`.
- Driver fps log: `patches/csf-v11/233-panvk-fps-log-present-counter.patch` (only `panvk_wsi.c`).
- PanProbe env A/B: `apps/panvk-test/app/src/main/java/dev/zenithblue/panvktest/EnvPasses.kt`, `app/src/main/assets/env-ab.json`, scenario 3 (see the last section).

## APKs (private test builds, not releases)

| APK | driver | sha256 |
|---|---|---|
| `/home/flux/build-out/apks/panplay-rc5u-114-gameloop.apk` | RC5-universal-114 `57018130` (no fps log: driver fps fields stay empty, DXVK and compositor numbers are there) | `d2eb06500a8ce47e22ff597d3588987754de226053b49cfc6edbc5dcab069f8b` |
| `/home/flux/build-out/apks/panplay-rc5u-114fps-gameloop.apk` | private RC5-universal-114-fps `51f6fdbe` (114 + csf-v11 233) | `3fd54e8cb138172be6d3a3d4a94f7302da2b020afc63874e9eb5b6b745792ab2` |
| `/home/flux/build-out/apks/panprobe-rc5u-114-ab.apk` | rc5 = RC5-universal-114 `57018130`, rc4 `c395db09` | `80150702e08f686d61266f07ae3d94e12ddef31c472ea752f91885625803d45a` |

Both PanPlay APKs are `dev.zenithblue.panvklauncher` 1.2.9-rc5 (vc 39), debug-signed, all 9 components bundled, built with `-PpanvkSo=<so>`. Their `bundled-driver.json` still names 114 (the label is not rewritten). The 114-fps APK's real driver shows in `results.json` (`driverInfo` from the fps log header, `driver.sha256` of the loaded .so). For 115, build with `-PpanvkSo=<115 .so>`. If the `old` env set is filled in (below), rebuild after editing `gameloop.json`.

## Run on Firebase (later, 115 only)

One execution at a time, under the Firebase lock (`firebase-setup.md`):

```
flock /home/flux/build-cache/firebase.lock gcloud firebase test android run --type game-loop \
  --app /home/flux/build-out/apks/<panplay-115-gameloop>.apk --scenario-numbers 4 \
  --device model=<m>,version=<v> --timeout 30m \
  --directories-to-pull /sdcard/Android/data/dev.zenithblue.panvklauncher/files/gameloop
```

Timing: on a fresh device the first pass also installs the bundled components and runs `wineboot -u` (a few minutes). After that, a pass takes about 110-120 s: Wine start, then 20 s warm-up, 60 s measured and 8 s tail, then teardown, log collection and a 5 s gap. Scenario 4 (4 passes) takes about 10-12 minutes, so `--timeout 30m` is enough.

Local (adb, own phone only): `adb shell am start -a com.google.intent.action.TEST_LOOP -n dev.zenithblue.panvklauncher/.MainActivity --ei scenario 1`. Optional: `--ei warmup_sec 5 --ei measure_sec 10` and `--es gameloop_config <path to an override json>`. Without `-d <uri>` there is no Firebase results file, and everything still goes to the external files dir.

## Scenarios (assets/gameloop.json)

| # | name | passes (in order) |
|---|---|---|
| 1 | d3d11 | D3D11 cube (`builtin-d3d11-arm64ec`), env set `default` |
| 2 | d3d9 | D3D9 cube (`builtin-d3d9-i686`, WoW64 through FEX), `default` |
| 3 | both | d3d11 `default`, d3d9 `default` |
| 4 | ab | d3d11 `old`, d3d11 `default`, d3d9 `old`, d3d9 `default` |

A scenario lists `tasks` and optionally `envSets` (default `["default"]`). The passes are each task in order, with one pass per env set in order. Scenario numbers must be 1..N without gaps. Gradle reads N from the file into the manifest (`com.google.test.loops`, placeholder `gameLoopScenarios`). `GameLoopMathTest` checks the shipped file.

To add a test (for example the DX12 3D test), add a task: `shortcut` (a PanPlay shortcut id; a built-in one needs an entry in `BuiltinTests`), `title`, optional `env`, `secsEnv` (the env var that sets the test's own run time, like the cube's `DXCUBE_SECS`; the harness sets it to warm-up + measure + tail; leave it empty and the harness stops the test after the window), `firstFramePattern` / `okPattern` / `failPattern` (stdout regexes). Then name the task in a new scenario, or in an existing one if Firebase commands may change. Without `firstFramePattern`, the first frame is the moment the driver fps log appears, which needs a driver with 233.

## A/B env sets (115)

`envSets.old.env` is the place for the 115 perf knobs at their old-behaviour values (for example `"PANVK_X": "0"`). `envSets.default` stays empty (driver defaults). Both are empty today, so scenario 4 runs the same thing twice, which still gives a run-to-run noise figure. Every pass gets common `env`, then the task's env, then the env set's, then the harness keys (`PANVK_FPS_LOG`, `PANVK_FPS_LOG_MS`, `DXCUBE_SECS`). `DXVK_HUD` stays `full`, because ContainerManager owns it. The pass env is in `results.json` (`passes[].env`).

Ways to set the knobs:
1. Edit `assets/gameloop.json` and rebuild. This is deterministic, and the config is inside the APK.
2. Override file, deep-merged over the asset. The first one found wins: `--es gameloop_config`, `<external files>/gameloop.json`, `/sdcard/panplay-gameloop.json`, `/data/local/tmp/panplay-gameloop.json` (Firebase `--other-files /data/local/tmp/panplay-gameloop.json=<file>`; it is used only if the app can read it). A tiny file like `{"envSets":{"old":{"env":{"PANVK_X":"0"}}}}` is enough. `results.json` `configSource` records which file was used (and its sha256), and `config.json` holds the merged config.

Order bias: in scenario 4, `old` always runs before `default`, so the `default` pass runs on a warmer device. If that matters, add a scenario with `"envSets": ["default", "old"]` and run both orders.

## What a pass does

1. Same launch path as the Games screen (`ContainerManager.runExe` with the shortcut's LaunchOptions + the pass env, built-in X server, the APK's bundled driver unless `"driver"` says otherwise). In game-loop mode, the launcher UI is not shown (no storage / What's new dialogs, no update check), and the touch overlay is collapsed.
2. First frame: the cube's `CUBE: <api> Present frame=0` line, or the driver fps log appearing. If no first frame comes after `startTimeoutSec` (300), the pass is stopped and marked failed.
3. Measured window: 20-80 s after the first frame. Compositor counters are sampled at both ends. A screenshot (PixelCopy of the display view: game + DXVK HUD) is taken at 50 s.
4. The cube runs `DXCUBE_SECS` = 88 s and exits by itself (`CUBE: PASS api=... frames=N`, exit 0). After 45 s more, the harness stops it and marks it failed.
5. Logs: the launcher's session folder (`SessionLogs.collect`: wine-run.log, dxvk-*.log, xserver.log, logcat.txt, mesa-panvk.txt, session.json, device/config), the perf recorder files, the fps log(s), result.json.

Verdict `ok`: no harness stop, test PASS line seen, exit 0, and fps data in the window. Otherwise `fail` with `reason`.

## Outputs

`/sdcard/Android/data/dev.zenithblue.panvklauncher/files/gameloop/<yyyyMMdd-HHmmss>-s<N>/`:
- `results.json` (also written to the Firebase results file, `intent.data`, after every pass, so a timeout keeps the finished passes): app, device (model, SoC, SDK, display, refresh), driver (id, path, size, sha256), `driverInfo`, window, `configSource`, `passes[]`, `ab[]`, `ok`.
- `results.txt`: one table row per pass plus the A/B lines (`A/B d3d11: old 45.2 fps, default 47.9 fps (+6.0%) (change vs old)`).
- `config.json` (merged config), `harness.log` (also logcat tag `PanPlayGameLoop`).
- `<NN>-<task>-<envSet>/`: `result.json`, `panvk-fps-<pid>.log`, `perf.csv` (DXVK stats: `F,timestamp_us,frametime_us` per present, `S,...` every 500 ms; `dxvk-stats.csv` instead when the launcher's perf recording is off), `perf-sys.csv` (CPU/GPU load and clocks, temperatures, RAM), `perf.json`, `screen.png`, `session/`.

Per pass in `results.json`: `status`, `reason`, `exit`, `wallSec`, `firstFrameSec`, `test` (ok, line, frames), `fps` + `fpsSource`, `driverFps` (fps, frames, spanMs, intervals, minIntervalFps, maxIntervalFps, maxFrameMs), `dxvkFps` (avgFps, low1Fps, low01Fps, p50Ms, p95Ms, p99Ms, stutters, gpuLoadAvg, drawCallsAvg), `compositor` (drawnFps, contentFps), `perfSummary` (whole run, perf recorder: CPU/GPU/temperature/RAM), `driverPresents`, `env`.

## fps method

Three independent counts over the same window, each measured from its own first frame:
- **Driver (primary, `driverFps`)**: csf-v11 233 counts the presents that queued an image. It works for any API layer (DXVK, VKD3D-Proton, native) and with the HUD on or off. `PANVK_FPS_LOG_MS=1000` gives one line per second, and the window uses the 1 s intervals that lie fully inside 20-80 s: fps = their frames / their time. It also gives the slowest and fastest second and the longest frame. In PanPlay, the presents reach the driver as `vkQueuePresentKHR` through wsi_common's X11 swapchain (software present: PutImage / MIT-SHM into the built-in X server). `vkQueueSignalReleaseImageANDROID` (the Android loader's swapchain) is counted too.
- **DXVK (`dxvkFps`)**: the patched DXVK stats file (`runtimes/dxvk/panplay-stats.patch`) with frame times per present. It gives the 1% / 0.1% lows and the percentiles that the driver's 1 s lines cannot. It exists only under DXVK (both cubes).
- **Compositor (`compositor`)**: GLRenderer counters in the built-in X server: frames composited (`drawnFps`, what reaches the screen, vsync-capped) and window content updates (`contentFps`). A present that the X server dropped or merged shows up here.

`fps` (the headline, and what A/B compares) is the driver value, or the DXVK value with a driver that has no 233 (for example plain 114). The DXVK HUD stays on (it is drawn into the frame, so it costs the same in both A/B passes).

Caveats:
- The cube sleeps in a message pump for at least 10 ms per frame (`pump(10)` in `tests/dxcube.c`), so it never goes above about 70-100 fps. On the G615 (2026-10-22 run), the frame time was 21.7 ms p50 at 47 fps, so the real work (about 11 ms) still shows in the A/B. On a fast device, a pinned fps near the cap means the knobs are not visible in that test.
- Software X11 present copies each frame on the CPU (1280x720). Present-path cost is part of the number, just as it is for real games in PanPlay.

## Driver fps log (csf-v11 233)

`PANVK_FPS_LOG=<path>` (appended; `%p` = pid) and `PANVK_FPS_LOG_MS` (default 1000, minimum 100). Both are read once per process, at the first present. After that, when unset, the present path reads one int and calls wsi_common as before. When set, each present costs one clock read and one mutex. Lines:

```
# panvk-fps PanVK-kbase RC5-universal-114-fps (Mesa 26.3.0-devel (git-3071c6dfa2)) pid=1234 interval_ms=1000 first_present_unix_ms=...
fps t_ms=1016 frames=61 fps=60.0 avg_ms=16.67 max_ms=16.67 total=62
fps-summary t_ms=5099 frames=241 fps=47.3 avg_ms=21.16 max_ms=100.00 total=242 reason=swapchain
```

`t_ms` is the time since the first present, frames/avg/max are over the frame times of the interval, and `total` counts every present so far. `fps-summary` covers the whole run: it is written at `vkDestroySwapchainKHR` and at exit / driver unload (atexit), each time only if there were new presents, and after the interval in progress has been flushed (the `fps` lines add up to the summary). The file is opened at the first present, so processes that never present write nothing. The hooks are strong `panvk_QueuePresentKHR` / `panvk_DestroySwapchainKHR` / `panvk_QueueSignalReleaseImageANDROID`. They take the device dispatch slots ahead of wsi_common and vk_common. The patch only appends to `panvk_wsi.c` (its own includes too), so no line of the file moves, and 115 changes to that file made without 233 still apply on top of it without an offset. Device destroy is not hooked (that would touch `panvk_vX_device.c`, which later patches change); the exit summary covers it. Details and validation: `patches/jm-v7-wip/README.md`, section csf-v11 233.

## PanProbe: driver env A/B (scenario 3)

`dev.zenithblue.panvktest.rc5u` (and every PanProbe build) gets scenario 3. On the rc5 driver, it does one Run all per pass of `assets/env-ab.json`:

```json
{"passes": [{"name": "defaults", "env": {}},
            {"name": "114-knobs", "env": {"PANVK_X": "0"}, "tests": "tess_perf,large_draw"}]}
```

- A pass's env goes into every test and vkinfo child process (`buildEnv`, after `--es env`). Each test is a fresh process, so knobs that are read once at instance or device init (or once per process) take effect. The one in-process test, `swapchain_lifecycle` (it needs the app's Surface), gets the pass env from the app's own env, which is set between passes and restored after the last. That test loads the driver once per process (`g_icd` in `swapchain.c`), so there only knobs read at instance or device init switch between passes, and a value the driver caches per process keeps its first-pass value. Only `PANVK_*`, `MESA_*` and `PAN_*` keys are accepted (the same rule as `--es env`); others are logged and dropped. `tests` (optional) limits a pass to some tests (comma list) for bisecting; the default is all. A subset of knobs in a pass bisects too.
- Tagging: `autorun.txt` gets `GAMELOOP pass=<i>/<n> name=<name> tests=<..> env=<..>` before each pass, and every `RESULT` line ends in ` pass=<name>` (scenario 3 only). The run folder is `runs/<time>-<pass>`, `summary.json` and the Firebase results file have `envPass` (name, index, count, env, tests, source), and `previousPhases` holds the passes already done. Logcat: `PANPROBE_ENVPASS`.
- The shipped file has `defaults` and `114-knobs`, both with empty env (to fill in for 115). Overrides (first found wins, whole file): `--es env_ab <path>`, `<external files>/env-ab.json`, `/data/local/tmp/panprobe-env-ab.json`. A missing, bad or empty file gives one `defaults` pass without env, which is exactly scenario 2.
- Scenarios 1 (rc5 then rc4) and 2 (rc5 only) never read the file and are unchanged, so existing runs stay comparable. The manifest now says `com.google.test.loops` = 3. Firebase: `--scenario-numbers 3`, pulling `/sdcard/Android/data/dev.zenithblue.panvktest.rc5u/files` as before.
