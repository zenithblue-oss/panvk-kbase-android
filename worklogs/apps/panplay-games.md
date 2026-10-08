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
| Need for Speed MW | yes (Quick Play > circuit City Perimeter, racing 3rd of 3, 90 fps, HUD+speedo) | speedmw/gameplay.png | legal splash was nsiproxy hang; fixed. Nav by `--longpress` keys, race start by Enter |
| Dark Souls PtD | yes (in-world, 3D visible) | DarkSouls/gameplay.png | black 3D = Blur/Antialiasing (MSAA) filter on panvk: app forces `Blur=0 Antialiasing=0 ForceDisableAA=1` in DarkSouls.ini; pad preset darksouls.json |
| Silksong | yes (in-game) | Silksong/gameplay.png | `Crash!!!` = Unity over-budgets VRAM (DXGI 5+ GB vs 2.4 GB heap): app caps `dxgi.maxDeviceMemory=2048; dxgi.maxSharedMemory=1024` for UnityPlayer.dll games |
| Burnout Paradise | partial: main menu, license, in-world (pause menu, minimap, tutorial subtitle); 3D scene renders black/garbled, car does not move | Burnout/ingame_corrupt3d.png, Burnout/menu.png | Endless loading root cause found: shortcut pointed at the outer nested copy of BurnoutParadise.exe, which has no VEHICLES/ dir (log: `CreateFileW ... VEHICLES\\VEHICLELIST.BUNDLE not found (c000003a)`, main thread spins). Pointing it at `.../Burnout Paradise/Burnout Paradise/BurnoutParadise.exe` (the one next to VEHICLES/) loads fine. Not CPU/audio/topology: WINE_CPU_TOPOLOGY=4 had no effect. Remaining: 3D corruption, same with d3d9.forceSwvp and floatEmulation=Strict |
| NFS Undercover | no: BLOCKED by DRM | nfsuc/n3.png | game dir has `paul.dll` (SecuROM 7 component) next to a 10.5 MB nfs.exe whose `.data` (0x91f000, 0x6531d4 virt, 0x64000 raw) is executed: crash at `00D4AB4C` = `.data`+0x2BB4C holding encrypted-looking bytes (`d3 61 43 72 1e ff 80 ff`), SEH CONTINUE_SEARCH; exe also ships steam_api.dll/steam_api.ini and a 2.2 MB dinput8.dll (modified install). Not bypassing DRM/cracks: stopped. |
| AoE2 | yes (Standard Game, village in-game) | AoE2/gameplay.png | `undetectable problem in loading the specified device driver` came from startup/intro path; app passes `nostartup` for age2_x1/age2_x2/empires2.exe. cnc-ddraw works (auto and gdi) |


## Still blocked (evidence)
- Burnout Paradise: 3D scene black/garbled after reaching the world (see table). Next idea: AA/postfx targets like Dark Souls, or panvk D24S8 path (log: `D3D9: VK_FORMAT_D16_UNORM_S8_UINT -> VK_FORMAT_D24_UNORM_S8_UINT`).
- NFS Undercover: DRM (SecuROM `paul.dll` + code-in-.data). Per project rule no cracks or bypasses.
