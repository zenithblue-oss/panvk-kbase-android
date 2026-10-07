# PanPlay games on G615 (POCO X6 Pro): reach-gameplay worklog

Device: adb 192.168.1.34:37597, PanPlay 1.2.4 (beta.18-rc1) -> dev build 1.2.5-dev (versionCode 16, installed with `adb install -r`).
Proof screenshots: /var/tmp/panvk/panplay-games/<game>/gameplay.png (DXVK HUD visible).
Helper scripts (host): /var/tmp/panvk/panplay-games/{go.sh,in.sh,sc.py,launch.sh}.

## App-level changes (this session)
| Change | Why |
|---|---|
| Tap/Play on a game card launches it directly with saved settings; "Launch options" moved to the card menu | Was: tap only opened the launch card, nothing started without a second tap |
| Bundled MFC 14 / vcomp140 (x86 + x64) in `assets/deps`, copied into the prefix by `ensureRuntimeDeps` (rev marker `.deps-rev`) | Cat Quest (Unity) died in `Chroma.Awake` with `mfc140u.dll not found` (Razer Chroma plugin) |
| Game folder accepted as exe (`ShortcutStore.findExeIn`: shortcut exe, intent `run_exe`, new "Pick game folder" button via OpenDocumentTree) | Containers/launch with a folder path failed with "File not found" |
| cnc-ddraw bundled; auto-used (WINEPATH + `ddraw=n,b`) for exes that import ddraw.dll and no d3d* | Wine ddraw needs OpenGL (absent on Android): AoE2 etc. show black |

## Per-game status
(filled in below as games are tested)
