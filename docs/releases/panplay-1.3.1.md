![PanProbe 1.3.2 on build 122i: Lenovo TB336FU, Mali-G57 (v9), 41/41 pass](https://github.com/zenithblue-oss/panplay/releases/download/panplay-v1.3.1/panprobe.png)

![PanPlay 1.3.1 on build 122i: Poco X6 Pro, Mali-G615 MC6 (v11), DXVK D3D11 FL11_0 cube with HUD, 763 fps](https://github.com/zenithblue-oss/panplay/releases/download/panplay-v1.3.1/panplay.png)

# PanPlay 1.3.1

Windows games on Android through Wine, DXVK and vkd3d-proton, on the Mesa PanVK driver for Mali GPUs.

Bundled driver: [Mesa PanVK 26.3.1](https://github.com/zenithblue-oss/panvk-kbase-android/releases/tag/panvk-kbase-v26.3.1), `26.3.0-devel (git-795ca7ff3e, p0175, 2026-10-31)`, build 122i, one universal driver for Mali v6-v14 (v15 not fully supported).
Package `dev.zenithblue.panvklauncher`, versionName 1.3.1, versionCode 45. Install it over 1.2.7 or 1.3.0: games and containers stay.

## What is new since 1.2.7

### Added

- MangoHud 0.8.4 overlay, built from source. Install it in Components. It is on by default with every item (full).
- MangoHud page (Settings > Graphics, and per game in Edit game): on/off and every MangoHud option, global or per game. GPU load and VRAM work through the driver (`PANVK_GPU_STATS`).
- DXVK HUD on/off switch and HUD items in Settings > Graphics, plus a DXVK HUD choice per game.
- Mesa PanVK 26.3.0-devel driver, now also for Mali Bifrost GPUs (v6/v7).
- OpenGL games through Mesa Zink 26.2.4.
- Render API per game: Auto, DXVK, VKD3D, WineD3D, OpenGL (Zink), Vulkan or cnc-ddraw.
- The launch sheet shows the detected graphics API (with Godot, Unity and Unreal hints) and the API of the last run.
- Built-in Direct3D feature level tests: D3D12 11_0 to 12_2 and D3D11 10_0 to 11_1.
- Present path: Default or EGL (frames in AHardwareBuffers, no GL upload), in Settings > Display and per game.
- WineD3D-Zink Render API: D3D9-11 through OpenGL, Zink and PanVK (D3D12 stays on VKD3D).
- Built-in Fallout 4 controller key config ("Fallout 4 (keyboard)" template, with Enter and T). Fallout 4 itself uses the Xbox gamepad by default.
- Firebase game-loop test scenarios for the DXVK cubes, D3D12 (SquidRoom, gears) and the feature level tests.

### Fixed

- The DXVK HUD was always on with every item: the game's DXVK_HUD setting was ignored.
- Fallout 4: launch fixes, less memory, no black heads.
- Game audio in WMA (xWMA): Fallout 4 music, voices and sound effects no longer go silent once gameplay starts.
- The launch sheet no longer saves stale values.
- Browse for .exe: files picked from Downloads (`msf:` picker URIs) now resolve to the real path.
- Built-in X server: MIT-SHM requests check the segment owner and the image bounds.
- Fallout 4: the Xbox gamepad (on-screen or physical) works again. Wine services stay on unless the GOG Galaxy64.dll is present.

## Performance headline (bundled driver 26.3.1)

Average fps in PanPlay's built-in tests. RC4 = last public driver, 116 = internal build, 26.3.1 = this release. Details and PanProbe numbers are in the [driver release](https://github.com/zenithblue-oss/panvk-kbase-android/releases/tag/panvk-kbase-v26.3.1).

**POCO X6 Pro, Mali-G615 (v11)**

| test | RC4 | 116 | 26.3.1 | vs RC4 | vs 116 |
|---|---|---|---|---|---|
| D3D11 cube | 285.0 | 504.7 | 683.3 | +140 % | +35 % |
| D3D11 FL 10_0 | 704.7 | 767.9 | 1294 | +84 % | +69 % |
| D3D11 FL 10_1 | 686.2 | 776.5 | 1045 | +52 % | +35 % |
| D3D11 FL 11_0 | 668.3 | 734.2 | 1048 | +57 % | +43 % |
| D3D11 FL 11_1 | 680.8 | 762.8 | 1041 | +53 % | +36 % |
| D3D10 cube | 256.0 | 429.6 | 689.7 | +169 % | +61 % |
| D3D9 cube (vsync) | 120.0 | 119.9 | 119.9 | 0 % | 0 % |
| D3D12 SquidRoom (s5) (122g) | fails | 128.0 | 159.3 | – | +24 % |
| D3D12 gears (s6) (122g) | fails | 360.7 | 542.6 | – | +50 % |
| D3D12 FL 11_0 (s7) (122g) | fails | 568.4 | 774.0 | – | +36 % |
| D3D12 FL 11_1 (s7) (122g) | fails | 587.8 | 808.3 | – | +38 % |
| D3D12 FL all (s9) (122g) | fails | 558.9 | 793.7 | – | +42 % |

**Lenovo TB336FU, Mali-G57 (v9)**

| test | RC4 | 116 | 26.3.1 | vs RC4 | vs 116 |
|---|---|---|---|---|---|
| D3D11 cube | 163.0 | 160.2 | 159.5 | -2 % | 0 % |
| D3D11 FL 10_0 | 199.6 | 209.5 | 204.5 | +2 % | -2 % |
| D3D11 FL 10_1 | 197.0 | 218.4 | 206.8 | +5 % | -5 % |
| D3D11 FL 11_0 | 197.9 | 219.3 | 211.6 | +7 % | -4 % |
| D3D11 FL 11_1 | 202.3 | 216.9 | 220.5 | +9 % | +2 % |
| D3D10 cube | 162.0 | 161.6 | 160.8 | -1 % | 0 % |
| D3D9 cube (vsync) | 89.9 | 89.9 | 89.8 | 0 % | 0 % |
| D3D12 SquidRoom (s5) | fails | 23.0 | 24.6 | – | +7 % |
| D3D12 gears (s6) | fails | 260.0 | 257.5 | – | -1 % |
| D3D12 FL 11_0 (s7) | fails | 412.6 | 462.7 | – | +12 % |
| D3D12 FL 11_1 (s7) | fails | 429.9 | 500.2 | – | +16 % |
| D3D12 FL all (s9) | fails | 432.0 | 489.6 | – | +13 % |

On RC4, D3D12 failed in these runs ("fails"); it now runs. On v9 the PanPlay scenes are fragment-bound, so fps equals RC4 and 116 within noise.

## Tested

| arch | device | result |
|---|---|---|
| v9 | Lenovo TB336FU, Mali-G57 | scenarios 1-9 ok (D3D9/10/11 cubes, D3D11 FL tests, D3D12 SquidRoom, gears and FL tests), no DEVICE_LOST |
| v11 | POCO X6 Pro, Mali-G615 | scenarios 1-9 ok on 122g, scenarios 3 and 8 on 122i, no DEVICE_LOST |
| v14 | Galaxy Tab S12 SM-X940, Mali-G1-Ultra | scenarios 3, 5-8 ok on 122i |
| v12, v13 | Galaxy Tab S10 Ultra SM-X926B, Tab S11 Ultra SM-X930 | cube, D3D12 and FL tests ok on 122h |
| v6, v7, v9, v10 | Firebase: Galaxy S9, Galaxy A31, Pixel 6a, Pixel 7 | cubes (scenario 3) ok on 122g |

## Known issues

- POCO X6 Pro (v11): D3D11 and D3D10 cubes have a lower 1 % low fps than RC4 and 116 (about 47 vs 103 on RC4), while the average is much higher.
- Fallout 4 on DXVK: the face is drawn black.
- D3D12 feature level 12_x is not supported (reported UNSUPPORTED, runs at the 11_1 fallback).
- D3D12 shows no fps in the DXVK HUD (vkd3d-proton has no DXVK stats). Use MangoHud.
- Galaxy A31 (v7) and Pixel 7 (v10), Firebase: one run each showed 11-20 % less cube fps than 116. Not confirmed.
- TB336FU: launcher PSS in D3D12 and FL scenes is 25-65 MB above RC4 (the same as 116).

## Install

Install `PanPlay-v1.3.1.apk` over 1.2.7 or 1.3.0 (Android 10 or newer, arm64). Games and containers stay. A What's new dialog shows once after the update.

## Files and checksums (sha256)

Signing certificate SHA-256: `8b10fde8c12eff4a29e99ae669ba88854e6003178d9c3ec299254fe87c76f3bf`.

| file | sha256 |
|---|---|
| `PanPlay-v1.3.1.apk` | `71f8a5ae171ff4e91268ef87eb662983bb66ede50ad192f2ed42ac7a4d1638ee` |
| bundled `libvulkan_panfrost.so` | `62c2b3f98ca987d5ab50b6f01e2d2c1c5b3ee995fe93666aa5da9415f65c3348` |

## More screenshots

![tb336fu components](https://github.com/zenithblue-oss/panplay/releases/download/panplay-v1.3.1/tb336fu-components.png)

![tb336fu controls editor](https://github.com/zenithblue-oss/panplay/releases/download/panplay-v1.3.1/tb336fu-controls-editor.png)

![tb336fu cube d3d11](https://github.com/zenithblue-oss/panplay/releases/download/panplay-v1.3.1/tb336fu-cube-d3d11.png)

![tb336fu ingame menu](https://github.com/zenithblue-oss/panplay/releases/download/panplay-v1.3.1/tb336fu-ingame-menu.png)

![tb336fu whatsnew](https://github.com/zenithblue-oss/panplay/releases/download/panplay-v1.3.1/tb336fu-whatsnew.png)

