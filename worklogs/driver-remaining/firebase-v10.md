# Firebase Test Lab: PanProbe on Mali v10 (2026-10-07)

Goal: PanProbe Run all passes on every Mali v10 device in Firebase Test Lab.

## Setup

- PanProbe 1.2.4-dev (debug) gained a game-loop mode. `com.google.intent.action.TEST_LOOP` (category default, `application/javascript`) on `MainActivity` starts Run all, auto-uploads as usual, writes a JSON summary to the intent's data URI and finishes. The summary holds per-test status, ms, `extra`, the last 30 log lines of each non-PASS test, the upload id, `driverLoad` (from the upload zip's `driver-load.json`) and `driver`/`device`/`gpu`/`android`/`deviceFacts` (from `manifest.json`). It is rewritten after every test (`"complete": false` until the end), so a Test Lab timeout still leaves partial results. Non-PASS tails are also logged to logcat (tag `PanProbeGameLoop`).
- Command: `gcloud firebase test android run --type game-loop --app <apk> --device model=manaus,version=34 --device model=panther,version=33 --timeout 30m`. Results come back as `logcat`, `video.mp4` and `game_loop_results/results_scenario_0.json` per device. Local copies are in `/var/tmp/panvk/firebase/<run>/`.
- The game loop was first checked on a free virtual device (`MediumPhone.arm`, API 34): 0/35/1 (no Mali, as expected), with the results file, logcat and upload all present.
- Quota: Spark plan, 5 physical device runs per day. Every device in a matrix counts. Used 5/5 on 2026-10-07.

## Runs

| Run | Driver | manaus (G610 MC3) | panther (G710 MC7) |
|---|---|---|---|
| r1 | beta.17 .so | no results: hang in `gpu_prerast_slice`, then PanProbe blocked for the full 15 min | 0 pass / 35 fail / 1 skip: `Unknown gpu_id (0xa8620004)` |
| r2 | main (122) + csf-v11/140 | no results: activity recreated 3 s after start, Run all lost (PanProbe bug) | **36/36 PASS** (upload `99a194b3`) |
| r3 | main (122) + csf-v11/130-132 + 140 | **36/36 PASS** (upload `411b7306`) | not rerun (quota) |

| r4 | unified beta.18 (main 5df0941, sha `ffcf87c2…`) | not run: `TEST_QUOTA_EXCEEDED` (Insufficient testing quota), matrix failed at validation | same |

Latest per-device result: manaus 36/36 (r3), panther 36/36 (r2). DXVK, Bachata S4 and vkd3d compliance: all PASS on manaus r3 (no hard item missing; `robustBufferAccess2` comes from 122).

## Devices

| Model id | Device | SoC | GPU | gpu_id -> model | kbase uAPI | Kernel | Android |
|---|---|---|---|---|---|---|---|
| manaus | motorola edge 40 neo | MT6879 | Mali-G610 MC3 | `0xa8670000` -> G610 | CSF 1.18, 4 KiB pages | 5.10.218 android12 | 14 (API 34) |
| panther | Pixel 7 | GS201 (Tensor G2) | Mali-G710 MC7 | `0xa8620004` -> G710 (new row, 140) | CSF 1.14, 4 KiB pages | 5.10.157 android13 | 13 (API 33) |

Both: BC emulation on (native masks `0xc1fe039e` and `0xc1fe001e`), `KBASE_IOCTL_MEM_ALLOC_EX` ENOTTY with fallback to `MEM_ALLOC`, mapper4 HIDL (`swapchain_lifecycle` passes on both, so no Pixel AHB import problem showed up). `submit_stress` takes 30 s on both (its own time budget).

## Fixes

1. **csf-v11/140 Mali-G710 model row** (driver). panther dropped the device: `panvk: Unknown gpu_id (0xa8620004) or variant (0)`. `0xa862` is kbase TODX (`GPU_ID2_MODEL_MAKE(10, 2)`, "Mali-G710"), the same Valhall v10 core as G610 (LODX, `0xa867`). The row reuses the G610 tile buffer sizes and rates. The device name comes out as "Mali-G710 MC7". G510 (TGRX, product major 3) is not added because its arch minor has never been seen and pan_model matches the full id.
2. **PanProbe watchdog** (`jni.c`). After the 60 s timeout, `Native.run` sent SIGKILL and then called a blocking `waitpid(pid, 0)`. A child stuck in an uninterruptible kbase wait never dies, so the whole run hung (r1 manaus). The reap now polls `WNOHANG` for at most 5 s and then returns `timeout`.
3. **PanProbe activity recreation**. The launched activity is recreated a few seconds after the TEST_LOOP start on manaus (seen in r2 and r3; the trigger is not in logcat). `lifecycleScope` cancelled Run all, and the new instance skipped autorun because `savedInstanceState != null`. `MainActivity` now handles the usual config changes itself (`android:configChanges`), and in game-loop mode a recreated activity starts Run all again. In r3 the recreation still happened (not one of the listed changes): the first run was cancelled after 2 tests and the restarted run finished 36/36.
4. Carried in from the main checkout unchanged: csf-v11/130 (v13 vertex stores through gpu_prerast), 131 (marketing names by core count) and 132 (wait for the prerast producer before viewport runs).

## Open items

- **r1 `gpu_prerast_slice` hang on G610 (beta.17).** One child did `queue_group_create`, `tiler_heap_init` and one successful queue wait, then nothing for 15 min, and could not be killed. There is no test log (PanProbe hung before it could upload). It did not reproduce in r3 (main + 122 + 130-132 + 140, `gpu_prerast_slice` PASS in 303 ms). 132 fixes a compute-to-compute ordering race in the same path and is a plausible fix, but one passing run does not prove it. A G610 rerun of beta.17 alone would tell.
- **Unified beta.18 rerun (r4)**: blocked by the daily quota on 2026-10-07. Rerun on the next quota day with `/var/tmp/panvk/apk-beta18u/panprobe-beta18u.apk`, in this order: manaus, panther, cheetah, lynx, tangorpro (felix after that).
- **Remaining v10 models**: cheetah (Pixel 7 Pro), lynx (Pixel 7a), felix (Pixel Fold, API 33/34/36), tangorpro (Pixel Tablet, API 33/36). All are G710 and should hit the same 140 row. They need the next quota day.
- The recreation trigger on manaus is unknown. The restart covers it, but the cancelled first run overlaps the new one for about one test.
- The bundled-driver label still says beta.17 (the dev APK uses `-PpanvkSo`, so `bundled-driver.json` is not updated).
- The dev uploads are on the project endpoint: virtual devices `b8237c88`, `3af5ef43`, `247121d4` (no Mali, emulator rows); physical `99a194b3` (panther) and `411b7306` (manaus).
