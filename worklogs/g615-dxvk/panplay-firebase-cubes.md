# PanPlay cubes on Firebase Test Lab (game loop) and driver env A/B

Status 2026-10-26 (115fix): the three 115pre problems are fixed and confirmed on shiba (matrix-33ltrfa55a2v0, `device-runs/2026-10-26-ftl-v11-shiba-panplay-115fix/results.md`; diagnosis in `device-runs/2026-10-26-ftl-v11-shiba-panplay-115fix-diag/results.md`):
- The 47-50 fps cap was the cube's own message pump. With the PanPlay 03cd94c pump, d3d11 runs at 427.6 fps and d3d9 at 118.8 fps (vsync).
- The D3D9 HUD text now uses the system-uncached heap (p115-fix 0002). 114 draws it clean too.
- The D3D12 fault was a RandR request to PanPlay's X server, which has no RANDR. With p115-fix 0004, SquidRoom and gears both PASS.

Earlier (115pre): harness and driver patch built and validated on the host. The first Firebase runs use the 115pre build (p115-int `6a5d4e2756`, staging-115 pa + pb + pc on the 233 base, driverInfo `Mesa 26.3.0-devel (git-6a5d4e2756)`) on shiba (Pixel 8, Mali-G715 MC7): scenario 4 (A/B `old` vs `default`) and the D3D12 scenarios 5 and 6. Results (matrix-3mzs6fl64ex6k, `device-runs/2026-10-26-ftl-v11-shiba-panplay-115pre/results.md`): scenario 4 is 4/4 PASS. Cube driver fps, old vs default: d3d11 46.8 vs 47.5 (+1.4%), d3d9 49.7 vs 48.9 (-1.7%). In both d3d9 passes the DXVK HUD text is garbled. Scenarios 5 and 6 (D3D12) FAIL: the first vkCreateSwapchainKHR raises `Exception 0xc0000005 in Unix call`, then exit 3. The rerun plan is in that results.md. The 114 baseline runs were cancelled.

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
| `/home/flux/build-out/apks/panplay-115pre.apk` | 115pre `652af311` (`Mesa 26.3.0-devel (git-6a5d4e2756)`, 233 included), vc 40, 6 scenarios, `old` = 21 knobs, D3D12 tests + SquidRoom.bin | `dc469ee731a89740a0c236e143afd37b95db0a642d4acb672fdcfe8772ab92a0` |
| `/home/flux/build-out/apks/panprobe-115pre.apk` | rc5 = 115pre `652af311`, rc4 `c395db09`, env-ab `114-knobs` = 21 knobs | `aa9b9ec44e341a0018980d12b826121f6b9d3b4a8b73f44eacd2afead504ae96` |
| `/home/flux/build-out/apks/panplay-115fix-diag.apk` | 115fix-diag `1194cfe8` (`Mesa 26.3.0-devel (git-e519b567f0)`, fix knobs opt-in), PanPlay 03cd94c | `0c39f21665550d3a2b71817a08300fec92da2347fc5fdb26bfddc8ac5eeb5696` |
| `/home/flux/build-out/apks/panplay-115fix.apk` | 115fix `42d91d26` (`Mesa 26.3.0-devel (git-de6b5b2d23)`), PanPlay 871368a, vc 40 | `10161ee3d5b493ece069d515aa2bef4ca381aadf8b05149d14cb333b3c4b767f` |

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
| 5 | d3d12 | D3D12 Multithreading sample, SquidRoom scene (`builtin-d3d12-arm64ec`, vkd3d-proton), `default` |
| 6 | d3d12gears | vkd3d-proton gears demo (`builtin-d3d12gears-arm64ec`), `default` |

The config also has a `d3d12x64` task (SquidRoom as an x86_64 exe through FEX) that no scenario names; use it through an override file. D3D12 sources, licenses and build scripts: `apps/panvk-launcher/tests/dx12` (`INTEGRATION.md`). Both D3D12 tests print `DX12: Present frame=` (first frame), `DX12: fps=` once per second and `DX12: PASS test= frames= secs= fps= startup=` and exit after `DX12TEST_SECS`; the harness stores the PASS line's `fps` and `startup` in `test.fps` and `test.startupSec`. vkd3d-proton has no DXVK HUD or DXVK stats file, so the pass `fps` is the driver fps log (233). Tasks set `VKD3D_DEBUG=warn` and `VKD3D_SHADER_DEBUG=warn`; vkd3d-proton logs to `vkd3d.log` in the session folder.

A scenario lists `tasks` and optionally `envSets` (default `["default"]`). The passes are each task in order, with one pass per env set in order. Scenario numbers must be 1..N without gaps. Gradle reads N from the file into the manifest (`com.google.test.loops`, placeholder `gameLoopScenarios`). `GameLoopMathTest` checks the shipped file.

To add a test (the D3D12 tasks are examples), add a task: `shortcut` (a PanPlay shortcut id; a built-in one needs an entry in `BuiltinTests`), `title`, optional `env`, `secsEnv` (the env var that sets the test's own run time, like the cube's `DXCUBE_SECS`; the harness sets it to warm-up + measure + tail; leave it empty and the harness stops the test after the window), `firstFramePattern` / `okPattern` / `failPattern` (stdout regexes; the harness keeps the first 50,000 output lines of a pass plus every line that matches `okPattern` or `failPattern`, so a chatty `WINEDEBUG` cannot hide the verdict). Then name the task in a new scenario, or in an existing one if Firebase commands may change. Without `firstFramePattern`, the first frame is the moment the driver fps log appears, which needs a driver with 233.

## A/B env sets (115)

`envSets.old.env` holds every default-on knob of staging-115 pa (8), pb (8) and pc (5) at its 114 value (21 keys, list in `patches/staging-115/int/NOTES.md`); `envSets.default` stays empty (driver defaults). Opt-in knobs (`PANVK_KBASE_COHERENT_POOLS`, `PANVK_KBASE_EVICTABLE`, `PANVK_KBASE_HEAP_SLOTS`) are unset in both. `PANVK_ZS_PRELOAD_ALWAYS` is not in `old`: on v11, 114 already used INTERSECT (csf-v11 061); on a v9/v10 device add `PANVK_ZS_PRELOAD_ALWAYS=1` for the full 114 set. With both sets empty, scenario 4 would run the same thing twice (a run-to-run noise figure). Every pass gets common `env`, then the task's env, then the env set's, then the harness keys (`PANVK_FPS_LOG`, `PANVK_FPS_LOG_MS`, `DXCUBE_SECS`). `DXVK_HUD` stays `full`, because ContainerManager owns it. The pass env is in `results.json` (`passes[].env`).

Ways to set the knobs:
1. Edit `assets/gameloop.json` and rebuild. This is deterministic, and the config is inside the APK.
2. Override file, deep-merged over the asset. The first one found wins: `--es gameloop_config`, `<external files>/gameloop.json`, `/sdcard/panplay-gameloop.json`, `/data/local/tmp/panplay-gameloop.json` (Firebase `--other-files /data/local/tmp/panplay-gameloop.json=<file>`; it is used only if the app can read it). A tiny file like `{"envSets":{"old":{"env":{"PANVK_X":"0"}}}}` is enough. `results.json` `configSource` records which file was used (and its sha256), and `config.json` holds the merged config.

Diagnostic keys (2026-10-26, `GameLoopMathTest.passDriverInstallsAndShortPass`):
- `installDrivers {id: path}`: before the passes, each `.so` (absolute, or relative to the external files dir, for example pushed with `--other-files /sdcard/Android/data/dev.zenithblue.panvklauncher/files/x.so=<file>` and named `"x.so"`) is copied to `files/drivers/<id>/` with a `meta.json`, like an imported driver. A missing file is logged and skipped. Push it next to `gameloop.json`: on shiba (Android 14) the app could not see a file in a subdirectory that the push created (`files/drivers/x.so`, 115fix-diag run).
- `envSets.<id>.driver`: a driver id for that env set's passes only. The pass's `driver` (id, path, sha256) is in its `result.json`; an unknown id falls back to the run's driver (logged).
- `mapsSnapshotSec` (0 = off): every N s, `/proc/<pid>/maps` of the app's processes that mapped a `libvulkan_*` driver goes to `maps-<pid>.txt` in the pass dir (the last one before the process exits stays), to turn crash addresses into module offsets.
- An env set that sets the task's `secsEnv` (for example `DXCUBE_SECS`) keeps its value: a short diagnostic pass.
- `PANPLAY_XTIMING=1` in a pass env: X server stage timing (`com.winlator.core.XTiming`), one `XT` line per second in `session/xserver.log` and logcat (requests and their time with the 3 costliest kinds, MIT-SHM PutImage lock wait and copy, GL frame time, DRAWABLE_MANAGER wait, texture upload, wake latency, frame gap; ms mean/max).
- Driver diagnostics (p115-fix, off by default): `MESA_VK_X11_SW_TIMING=1` (sw present stages: queue, fence, pace, slot, copy, put, reply, acquire), `PANVK_KBASE_WAIT_STATS=1` (CSF event waits per second and the ones that slept a full 20 ms slice), `MESA_VK_WSI_CREATE_LOG=1` (swapchain create info and pNext sTypes).

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
- Up to 2026-10-26 the cube waited 10 ms per frame in its message pump, timed with `GetTickCount` in `Sleep(5)` steps. Wine's tick count moves only when the wineserver updates it (about every 16 ms when idle), so every frame waited for the next update: 47-50 fps on any device (G615 and Pixel 8 both 21.7 ms p50). The cube now only drains the message queue; `DXCUBE_PUMP=old` restores the old pump. It prints `CUBE: timing fps= render_ms= present_ms= pump_ms= pump=` (mean/max per frame) once a second. On Pixel 8 the d3d11 cube went from 46.8 to 418.6 fps (DXVK GPU load 22% -> 77%). The d3d9 cube keeps the D3D9 default present interval (vsync): DXVK uses FIFO, and the X11 sw present paces it at `MESA_VK_X11_SW_REFRESH_HZ` (the display refresh PanPlay passes, 120 Hz on Pixel 8), so it runs at about 120 fps. The d3d11/d3d10 cubes present with sync interval 0 (`device-runs/2026-10-26-ftl-v11-shiba-panplay-115fix-diag/results.md`).
- Software X11 present copies each frame on the CPU (1280x720). Present-path cost is part of the number, just as it is for real games in PanPlay.

## Driver fps log (csf-v11 233)

`PANVK_FPS_LOG=<path>` (appended; `%p` = pid) and `PANVK_FPS_LOG_MS` (default 1000, minimum 100). Both are read once per process, at the first present. After that, when unset, the present path reads one int and calls wsi_common as before. When set, each present costs one clock read and one mutex. Lines:

```
# panvk-fps Mesa 26.3.0-devel (git-6a5d4e2756) pid=1234 interval_ms=1000 first_present_unix_ms=...
fps t_ms=1016 frames=61 fps=60.0 avg_ms=16.67 max_ms=16.67 total=62
fps-summary t_ms=5099 frames=241 fps=47.3 avg_ms=21.16 max_ms=100.00 total=242 reason=swapchain
```

The header carries the driverInfo: `Mesa <version> (git-<sha>)` from p115-int on (staging-115/int 0001), `PanVK-kbase <release> (Mesa ...)` on older builds; `GameLoopMath.parseFpsLog` reads both. `t_ms` is the time since the first present, frames/avg/max are over the frame times of the interval, and `total` counts every present so far. `fps-summary` covers the whole run: it is written at `vkDestroySwapchainKHR` and at exit / driver unload (atexit), each time only if there were new presents, and after the interval in progress has been flushed (the `fps` lines add up to the summary). The file is opened at the first present, so processes that never present write nothing. The hooks are strong `panvk_QueuePresentKHR` / `panvk_DestroySwapchainKHR` / `panvk_QueueSignalReleaseImageANDROID`. They take the device dispatch slots ahead of wsi_common and vk_common. The patch only appends to `panvk_wsi.c` (its own includes too), so no line of the file moves, and 115 changes to that file made without 233 still apply on top of it without an offset. Device destroy is not hooked (that would touch `panvk_vX_device.c`, which later patches change); the exit summary covers it. Details and validation: `patches/jm-v7-wip/README.md`, section csf-v11 233.

## PanProbe: driver env A/B (scenario 3)

`dev.zenithblue.panvktest.rc5u` (and every PanProbe build) gets scenario 3. On the rc5 driver, it does one Run all per pass of `assets/env-ab.json`:

```json
{"passes": [{"name": "defaults", "env": {}},
            {"name": "114-knobs", "env": {"PANVK_X": "0"}, "tests": "tess_perf,large_draw"}]}
```

- A pass's env goes into every test and vkinfo child process (`buildEnv`, after `--es env`). Each test is a fresh process, so knobs that are read once at instance or device init (or once per process) take effect. The one in-process test, `swapchain_lifecycle` (it needs the app's Surface), gets the pass env from the app's own env, which is set between passes and restored after the last. That test loads the driver once per process (`g_icd` in `swapchain.c`), so there only knobs read at instance or device init switch between passes, and a value the driver caches per process keeps its first-pass value. Only `PANVK_*`, `MESA_*` and `PAN_*` keys are accepted (the same rule as `--es env`); others are logged and dropped. `tests` (optional) limits a pass to some tests (comma list) for bisecting; the default is all. A subset of knobs in a pass bisects too.
- Tagging: `autorun.txt` gets `GAMELOOP pass=<i>/<n> name=<name> tests=<..> env=<..>` before each pass, and every `RESULT` line ends in ` pass=<name>` (scenario 3 only). The run folder is `runs/<time>-<pass>`, `summary.json` and the Firebase results file have `envPass` (name, index, count, env, tests, source), and `previousPhases` holds the passes already done. Logcat: `PANPROBE_ENVPASS`.
- The shipped file has `defaults` (empty env) and `114-knobs` (the same 21 knobs as PanPlay's `old` set). Overrides (first found wins, whole file): `--es env_ab <path>`, `<external files>/env-ab.json`, `/data/local/tmp/panprobe-env-ab.json`. A missing, bad or empty file gives one `defaults` pass without env, which is exactly scenario 2.
- Scenarios 1 (rc5 then rc4) and 2 (rc5 only) never read the file and are unchanged, so existing runs stay comparable. The manifest now says `com.google.test.loops` = 3. Firebase: `--scenario-numbers 3`, pulling `/sdcard/Android/data/dev.zenithblue.panvktest.rc5u/files` as before.
