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

## 122g (p0173) vs 116c baseline, PanPlay s3 (whole-run perfSummary: avgFps / 1% low fps; ram = peak MB)
116c = panplay-116c.apk / panprobe-116c.apk (newest 116 build, git-f7e78953ac). Runs: abhay-byte and phone-finder-hub, one matrix each, all FINISHED SUCCESS.

| device (arch) | test | 116c fps / 1% low / ram | 122g fps / 1% low / ram | fps delta |
|---|---|---|---|---|
| bluejay (v9) | D3D11 | 169.4 / 97.0 / 241 | 169.3 / 93.2 / 242 | 0 % |
| bluejay (v9) | D3D9 | 60.0 / 35.9 / 433 | 60.0 / 35.0 / 440 | 0 % |
| panther (v10) | D3D11 | 576.3 / 230.1 / 246 | 511.1 / 179.9 / 248 | -11 % |
| panther (v10) | D3D9 | 90.0 / 48.6 / 323 | 90.0 / 47.9 / 299 | 0 % |
| a31 (v7) | D3D11 | 67.8 / 39.2 / 246 | 54.2 / 33.0 / 244 | -20 % |
| a31 (v7) | D3D9 | 36.2 / 22.7 / 244 | 26.9 / 17.7 / 249 | -26 % |
| starlte (v6) | D3D11 | 100.3 / 64.6 / 257 | 96.0 / 57.6 / 257 | -4 % |
| starlte (v6) | D3D9 | 59.9 / 41.3 / 292 | 59.8 / 39.8 / 293 | 0 % |

PanProbe 41/41 pass, 0 DEVICE_LOST: bluejay 116c 155.9 s total vs 122g 145.3 s; panther 116c 139.4 s (122g panther not re-run on bluejay-style timing, ms sums only in the new runs).
Single runs, no repeats: a31 (-20/-26 %) and panther D3D11 (-11 %) are worth an RTL/local A/B before treating as real regressions.
