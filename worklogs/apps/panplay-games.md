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
| Game | Gameplay | Proof (/var/tmp/panvk/panplay-games/) | Cause / fix |
|---|---|---|---|
| Cat Quest | yes (overworld, 121 fps) | CatQuest/gameplay.png | mfc140u.dll missing: bundled MFC; nsiproxy hang fixed |
| MiSide | yes (new game, bedroom, 59 fps) | MiSide/gameplay.png | works after nsiproxy.sys=d |
| Skyrim TESV | yes (intro carriage 3D, 38 fps) | TESV/gameplay.png | works after nsiproxy.sys=d |
| Need for Speed MW | partial: main menu 3D, 100 fps | speedmw/gameplay.png | legal splash was nsiproxy hang; driving not reached (menu nav by key slow) |
| Dark Souls PtD | partial: character creation, 30 fps | none (menu only: DarkSouls/charcreate.png) | needs pad: new preset darksouls.json (output both); load into world not reached |
| Silksong | partial: Unity loads, DXVK D3D11, black/loading | Silksong/menu.png | log: `mono_os_sem_timedwait ... error 87` then `Crash!!!` (earlier run); Steam init fails; not reached title |
| Burnout Paradise | no: Paradise City loading screen, never finishes (4+ min, 60 fps) | Burnout/loading.png | CPU-bound main thread 93%, no log error |
| NFS Undercover | no: stuck on `Compiling shaders...` | nfsuc/n3.png | earlier: `Unhandled illegal instruction at address 00D4AB4C` (FEX); now hangs in game shader compile, fex Extreme also 0.9 fps |
| AoE2 | no | n/a | ddraw: `undetectable problem in loading the specified device driver` with builtin and cnc-ddraw |
