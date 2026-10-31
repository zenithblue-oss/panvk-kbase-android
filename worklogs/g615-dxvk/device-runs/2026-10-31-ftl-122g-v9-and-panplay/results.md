# FTL 122g (p0173): v9 PanProbe + PanPlay game-loop, one matrix per device, --async, no retries

driverInfo on all runs: Mesa 26.3.0-devel (git-a0ae06e7ac, p0173, 2026-10-31). DEVICE_LOST in logcat: 0 on all.
Quota: 5 matrices (panvk-device-test 3, phonefinder-hub 2); all fit.

## PanProbe 122g (game-loop, scenario 2)
| device | GPU arch | project | matrix | result |
|---|---|---|---|---|
| bluejay (Pixel 6a, API 32, Mali-G78) | v9 | panvk-device-test | matrix-31j3i6nxhfpv2 | 41/41 pass, 0 fail |

## PanPlay 122g (game-loop, scenario 3 = D3D11 cube + D3D9 cube, 20 s warm-up + 60 s measured)
Command: `gcloud firebase test android run --type game-loop --app panplay-122g.apk --device model=<m>,version=<v>,locale=en,orientation=portrait --scenario-numbers=3 --timeout 1800s --directories-to-pull=/sdcard/Android/data/dev.zenithblue.panvklauncher/files/gameloop --async`
No earlier PanPlay Firebase run existed; flags chosen per GameLoop.kt.

| device | arch | matrix | D3D11 fps / 1% low / | D3D9 fps / 1% low | test lines |
|---|---|---|---|---|---|
| bluejay (Pixel 6a, API 32) | v9 | matrix-3cjzuned5x1k2 | 170.3 / 93.2 | 60.0 / 35.0 | CUBE: PASS x2 |
| panther (Pixel 7, API 33) | v10 | matrix-2y8tjtzt3deqo | 501.4 / 199.7 | 90.0 / 50.4 | CUBE: PASS x2 |
| a31 (SM-A315N, API 31) | v7 | matrix-2kfkzwlma3fmb | 54.3 / 35.5 | 27.0 / 22.0 | CUBE: PASS x2 |
| starlte (SM-G960F, API 29) | v6 | matrix-32iknc08sa4fc | 95.8 / 60.2 | 60.0 / 43.3 | CUBE: PASS x2 |

Notes: D3D9 fps looks vsync/frame-cap bound (60/90) on v9/v10/v6. First starlte submit hung during the 415 MB upload and created no matrix; panther/a31 first attempt failed 403 (wrong gcloud account), no matrix created. Each matrix submitted once.
