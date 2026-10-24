# g615 RC5-universal-112 run (2026-10-24): PanProbe and Fallout 4

Device: Poco X6 Pro (Mali-G615 MC6, v11), wifi adb 10.11.176.130:44087, logd stopped (no logcat).
Build: [builds/rc5-universal.md](../../builds/rc5-universal.md), section RC5-universal-112 (.so sha256 `15fc5464...`, csf-v11 223-229).
APKs installed with `adb install -r` (no uninstall): panprobe-rc5u-112.apk (`dev.zenithblue.panvktest.rc5u`), panplay-rc5u-112.apk (`dev.zenithblue.panvklauncher` 1.2.9-rc5 vc 37), both Success.

## PanProbe autorun all (rc5, run=20261024-063243)
- **39 pass / 0 fail / 0 skip, total 39.** DRIVER line: `PanVK-kbase RC5-universal-112 (Mesa 26.3.0-devel (git-a1f95e1585))`.
- bc_decode 16154 ms, bc_perf 44605 ms (cold, first run after install), tess_perf 48545 ms, large_draw 7868 ms, submit_stress 30091 ms, tessellation 3477 fps. Phone on battery, 19-20 %; no same-state A/B with 111 was run, so the times are not a perf comparison.
- Raw: [raw/all/autorun.txt](raw/all/autorun.txt), [raw/all/20261024-063243/](raw/all/20261024-063243/).
