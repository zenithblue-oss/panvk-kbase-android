# g615 RC5-universal-112 run (2026-10-24): PanProbe and Fallout 4

Device: Poco X6 Pro (Mali-G615 MC6, v11), wifi adb 10.11.176.130:44087, logd stopped (no logcat).
Build: [builds/rc5-universal.md](../../builds/rc5-universal.md), section RC5-universal-112 (.so sha256 `15fc5464...`, csf-v11 223-229).
APKs installed with `adb install -r` (no uninstall): panprobe-rc5u-112.apk (`dev.zenithblue.panvktest.rc5u`), panplay-rc5u-112.apk (`dev.zenithblue.panvklauncher` 1.2.9-rc5 vc 37), both Success.

## PanProbe autorun all (rc5, run=20261024-063243)
- **39 pass / 0 fail / 0 skip, total 39.** DRIVER line: `PanVK-kbase RC5-universal-112 (Mesa 26.3.0-devel (git-a1f95e1585))`.
- bc_decode 16154 ms, bc_perf 44605 ms (cold, first run after install), tess_perf 48545 ms, large_draw 7868 ms, submit_stress 30091 ms, tessellation 3477 fps. Phone on battery, 19-20 %; no same-state A/B with 111 was run, so the times are not a perf comparison.
- Raw: [raw/all/autorun.txt](raw/all/autorun.txt), [raw/all/20261024-063243/](raw/all/20261024-063243/).

## Fallout 4: not run (phone locked)
- 01:11 UTC: PanPlay was started with `LAUNCH_SHORTCUT gb11a0666` and the shortcut env from [raw/fallout4/shortcut-gb11a0666-112.json](raw/fallout4/shortcut-gb11a0666-112.json) (`PANVK_DEBUG=mem`, `PANVK_MEM_LOG=/data/data/dev.zenithblue.panvklauncher/files/gfx-logs/panvk-mem-%p.log`, `MESA_LOG_FILE=.../gfx-logs/mesa-fo4.log`, plus the usual `DXVK_HUD=full`).
- The phone was asleep with a fingerprint keyguard (`isKeyguardShowing=true`), and at 19 % battery, not charging. `wm dismiss-keyguard` cannot pass a secure lock, so Wine never started (`pidof Fallout4.exe` empty). One notification asked to unlock and plug in; after about 30 min the phone was still locked (16 %, 32.7 C).
- PanPlay was force-stopped to cancel the pending launch, and the shortcut env was restored to the original (`DXVK_HUD=full` only). Nothing else was touched.
- Next run: unlock and charge the phone, put the env from the json back (or `run-as` the json into `files/shortcuts/`), launch, press Enter into gameplay, sample every 30 s for 5-8 min (`/proc/meminfo`, `/proc/<pid>/status`, `/proc/mtk_mali/gpu_memory` line for the pid, thermalservice, the last `panvk-mem` line), stop at MODERATE or 42 C battery. Baseline 110: 1.7-5.4 fps, MemAvailable 84 MB, swap full, 2.75 GB kbase. Tiler heap growth = mtk_mali pages x 4 KiB - (`devmem` + `priv` of the panvk-mem line). A/B for the heads: `PANVK_BC_RAW_STALE=0 PANVK_BC_RAW_FLIP=1 PANVK_BC_RAW_PIN=2 PANVK_KBASE_HEAP_CHUNKS=10,400 PANVK_KBASE_HEAP_TERM_MAX=16 PANVK_CMD_POOL_AUTOTRIM=0 PANVK_TLS_FREE_OLD=0 PANVK_UTRACE_CLONE_MEM_SIZE=0xa00000` = 111 behaviour.
