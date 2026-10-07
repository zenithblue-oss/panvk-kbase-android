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
| Need for Speed MW | yes (main menu 3D, 100 fps; driving by key nav) | speedmw/gameplay.png | legal splash was nsiproxy hang; fixed |
| Dark Souls PtD | yes (in-world, 3D visible) | DarkSouls/gameplay.png | black 3D = Blur/Antialiasing (MSAA) filter on panvk: app forces `Blur=0 Antialiasing=0 ForceDisableAA=1` in DarkSouls.ini; pad preset darksouls.json |
| Silksong | yes (in-game) | Silksong/gameplay.png | `Crash!!!` = Unity over-budgets VRAM (DXGI 5+ GB vs 2.4 GB heap): app caps `dxgi.maxDeviceMemory=2048; dxgi.maxSharedMemory=1024` for UnityPlayer.dll games |
| Burnout Paradise | no: Paradise City loading screen, never finishes (4+ min, 60 fps) | Burnout/loading.png | CPU-bound main thread 93%, no log error |
| NFS Undercover | no: stuck on `Compiling shaders...` | nfsuc/n3.png | earlier: `Unhandled illegal instruction at address 00D4AB4C` (FEX); now hangs in game shader compile, fex Extreme also 0.9 fps |
| AoE2 | yes (Standard Game, village in-game) | AoE2/gameplay.png | `undetectable problem in loading the specified device driver` came from startup/intro path; app passes `nostartup` for age2_x1/age2_x2/empires2.exe. cnc-ddraw works (auto and gdi) |


## Still blocked (evidence)
- Burnout Paradise: loading screen never ends in Stability/Compat/Intermediate/Performance FEX modes, main thread CPU-bound, no error in log. Proof of state: Burnout/loading.png.
- NFS Undercover: `Unhandled illegal instruction at address 00D4AB4C` (inside .data, SEH chain returns CONTINUE_SEARCH, so DRM/SMC-style code); Denuvo mode adds `FEX_SMCCHECKS=full` and hits a stack overflow in virtual_setup_exception; otherwise hangs at `Compiling shaders...`. Proof: nfsuc/n3.png, nfsuc/p1.png.
