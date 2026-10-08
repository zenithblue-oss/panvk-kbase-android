# PanPlay games on G615 (POCO X6 Pro): reach-gameplay worklog

Device: adb 192.168.1.34:37597, PanPlay 1.2.4 (beta.18-rc1) -> dev build 1.2.5-dev (versionCode 16, then 17 for the Burnout HLSL fix; installed with `adb install -r`).
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
| Burnout Paradise | yes (Downtown Paradise, driving Hunter Cavalry, ~60 fps) | Burnout/gameplay.png, Burnout/gameplay2.png | Endless load fixed in app: nested duplicate exe, `ShortcutStore.preferInner` (d4b5820, b1d29f8). Black/garbage world = bad runtime-compiled HLSL from Proton 11.0-2's vkd3d-shader 1.18 (not PanVK, not FEX): app bundles Wine 11.19 d3dcompiler_43+wined3d as native for BurnoutParadise.exe (details below) |
| NFS Undercover | no: BLOCKED by DRM | nfsuc/n3.png | game dir has `paul.dll` (SecuROM 7 component) next to a 10.5 MB nfs.exe whose `.data` (0x91f000, 0x6531d4 virt, 0x64000 raw) is executed: crash at `00D4AB4C` = `.data`+0x2BB4C holding encrypted-looking bytes (`d3 61 43 72 1e ff 80 ff`), SEH CONTINUE_SEARCH; exe also ships steam_api.dll/steam_api.ini and a 2.2 MB dinput8.dll (modified install). Not bypassing DRM/cracks: stopped. |
| AoE2 | yes (Standard Game, village in-game) | AoE2/gameplay.png | `undetectable problem in loading the specified device driver` came from startup/intro path; app passes `nostartup` for age2_x1/age2_x2/empires2.exe. cnc-ddraw works (auto and gdi) |


## Burnout Paradise 3D: root cause and fix
- Game is D3D9 and compiles its HLSL at runtime (d3dx9_37 -> d3dcompiler_43 -> vkd3d-shader inside wined3d.dll), caching SM3 bytecode in `AppData/Local/Criterion Games/Burnout Paradise/ShaderCache` (449-494 files).
- Split: game pulled to host (/var/tmp/panvk/burnout-host), host wine 11.19 + the device's own DXVK 3.1.1 x86 dlls on RADV in a headless sway: world correct (Burnout/host-radv-wine1119-ok.png). Host + the device's ShaderCache: world black/green garbage on RADV too (Burnout/host-radv-proton-shaders-broken.png). Device + host-compiled ShaderCache: correct on PanVK (Burnout/device-hostcompiled-shaders-ok.png). So driver, DXVK and FEX are fine; the bytecode is wrong.
- Device compile is deterministic: fresh compile under FEX Stability-like + `FEX_X87REDUCEDPRECISION=0` is byte-identical to the old cache (449/449), so not FEX precision. Proton 11.0-2 wined3d = "vkd3d-shader 1.18 (Wine bundled)", host = "vkd3d-shader 2.1"; every cache file differs (CTAB layout differs too).
- Wine builtin PE dlls (tag "Wine builtin DLL" at 0x40) are always redirected to Proton's lib/wine copy (app dir, syswow64 and WINEDLLPATH all lose: WINEDLLPATH is searched after the default dll dir). Renaming that tag makes them native: with `d3dcompiler_43,wined3d=n` the device produced 494/494 shaders byte-identical to the host.
- Fix (PanPlay vc17): assets/deps/x86/{d3dcompiler_43,wined3d}.dll (Wine 11.19, Arch wine 11.19-1, tag renamed, NOTICE updated), DEPS_REV 3 copies them into syswow64 (idle under default builtin-first order), launch adds `;d3dcompiler_43,wined3d=n` for BurnoutParadise.exe only. Verified from a clean ShaderCache with Proton's files restored: app reinstalls them, env shows the override, world + driving correct.
- No driver change. NFS MW still launches/renders after the app update. Device ShaderCache backup of the bad cache: Burnout/device-ShaderCache-backup.tar.
- Earlier ruled-out toggles (no change, because the cached bytecode was the problem): PANVK_DEBUG noafbc+sync (linear crashes vkCreateImage), FEX safe settings, and the list below.

## Still blocked (evidence)
- Burnout Paradise 3D (historical, now fixed above): Env toggles that did NOT change the picture (screens in /var/tmp/panvk/panplay-games/Burnout/<name>_d.png): PANVK_DEBUG=bc_compute, bc_wide, no_crc, no_user_mmap_sync+no_wb_mmap, hsr_prepass+wsi_no_afbc, force_simultaneous; PANVK_BC_AFRC=0 and 16 (BC emulation path is not the cause; menus with DXT textures render correctly; Vidmem changes but picture does not); DXVK d3d9.forceSwvp, floatEmulation=Strict+longMad, invariantPosition+forceSamplerTypeSpecConstants, useD32forD24+supportDFFormats=False, dxvk.enableGraphicsPipelineLibrary=False, deferSurfaceCreation+allowDiscard=False. no_gs makes DXVK fail (`Device does not support required feature geometryShader`); no_bc_emul crashes DXVK start. DXVK log shows no unsupported format or warning; wine +file shows no failed game-data opens (only missing config.ini). Symptom: after Paradise City loads, only car silhouette (yellow trim) + blue sky render, city and ground missing; 2D/menus fine. Next: capture the frame with a Vulkan layer or RenderDoc-like dump per draw (draw calls ~1500, 13 render passes) to see which pass loses geometry.
- NFS Undercover: DRM (SecuROM `paul.dll` + code-in-.data). Per project rule no cracks or bypasses.

## NFS Undercover FEX experiments (2026-10-08, still no gameplay)
- User note: same exe runs on box64 + x86 Wine, so this is an FEX/arm64ec emulation gap, not a hard block. No crack/bypass used (game dir dinput8.dll/steam_api.ini untouched).
- Shortcut `nfs` (g2b1762ae): fex=Stability + `FEX_SMCCHECKS=full` or `mtrack` => the `00D4AB4C` illegal-instruction crash is GONE (it is a self-modifying-code/stale-JIT issue). Game then reaches "Compiling shaders" and stays there >11 min (frames every 2.7-4.4 s, HUD: 3 draws, DXVK shaders constant 589).
- Cause of the stall (not solved): one worker thread (created after D3D device init, 6 voluntary ctx switches, ~125% CPU, user mode only, no file I/O/writes for 10 s) spins; main thread nearly idle (293 ticks in 6 min) => it waits on that thread. Candidates: protection/decrypt loop slowed by SMC tracking, or an unmet spin-wait. simpleperf blocked (needs security.perf_harden=0, a system setting: not changed).
- No effect: `FEX_HIDEHYPERVISORBIT=1`; Wine 11.19 `d3dcompiler_43,wined3d=n` override (nfs.exe imports d3dx9_34, so the same HLSL path exists) - not made permanent.
- Next: PanPlay already has a FEXCore content type (Containers.applyFex copies profile.json files into the prefix, libwow64fex.dll/libarm64ecfex.dll); a Wowbox64 (box64, MIT) wcp of the same shape could be a per-game emulator choice. Needs downloading Wowbox64-0.4.4.wcp (GameNative): needs user approval.
- Lesson: hold device.lock for the whole run (nfsuc/sess.sh); releasing it after launch let another agent force-stop the game.

### NFS UC FEX matrix (2026-10-08, later run; scripts nfsuc/exp.sh + queue2.sh, 4-5 min each, Enter pressed every 30 s)
- CORRECTION: `FEX_SMCCHECKS=mtrack` does NOT remove the crash: wine log still shows `Unhandled illegal instruction at address 00D4AB4C` and winedbg start (runs d0, a1). Only `FEX_SMCCHECKS=full` avoids it. Shortcut `nfs` now uses `full`.
- Stability preset already sets TSOENABLED=1, VECTORTSOENABLED=1, MEMCPYSETTSOENABLED=1, HALFBARRIERTSOENABLED=1, X87REDUCEDPRECISION=0, MULTIBLOCK=0, so the requested TSO/MULTIBLOCK=0/X87=0 cases were the baseline (stall).
- All with SMCCHECKS=full, all end in the same stall (HUD: 3 draws, 1 render pass, 589 shaders, FPS 1-3, black screen, no menu/gameplay after 4 min, Enter pressed): X87REDUCEDPRECISION=1 (a2), PARANOIDTSO=1 (b2), HOSTFEATURES=disableavx,disableavx2 (c1), +disablebmi1,disablebmi2 (c2), WINEDLLOVERRIDES +dinput8=b (e1), FEX_MAXINST=8 (h1). mtrack + X87REDUCEDPRECISION=1 (a1): crash at 00D4AB4C again.
- Spinner location (full mode): main thread (tid==pid, ~100% of one core, 24-25k ticks per 240 s) plus wine_dinput_worker (~50%) and RenderThread (~45%) busy. kstkeip is hidden, so debuggerd -b sampled the main thread: mostly aarch64-unix ntdll.so NtWaitForSingleObject -> NtWaitForMultipleObjects -> ntsync_wait_any -> pthread_sigmask, otherwise in FEX JIT code (anon 6ffded0000/6fffe40000) or NtProtectVirtualMemory -> mprotect.
- WINEDEBUG=+sync (h0, 150 s, 78 MB log, 1.39 M lines): main thread 0024 calls `RtlWakeAddressAll` on addresses inside libwow64fex.dll data (6FFE23C5E0, 6FFE23C6A0) about 250 k times (plus kernel32 7FC00A0530), i.e. FEX itself takes/releases its internal lock constantly: continuous JIT recompile/invalidation churn of self-modifying code, not a game-level wait or TSO issue. Another thread polls `NtWaitForSingleObject(handle, timeout 0)`. Log also ends with an Android ART OOM message (`Failed to allocate 157286416 byte allocation`, launcher process).
- Thread 0120 (post-D3D init) loops longjmp unwinds (RtlUnwindEx 80000026) with wow64 context get/set, then goes silent.
- Not tried: simpleperf (needs perf_harden change, forbidden), Wowbox64/box64 (needs downloading a third-party binary).
- Conclusion: no env/preset fixes it, so no per-exe preset added. Needs FEX-side work in libwow64fex (code invalidation under SMC). Screenshot of the stall: /var/tmp/panvk/shots/nfsuc-stall-hud.png.
