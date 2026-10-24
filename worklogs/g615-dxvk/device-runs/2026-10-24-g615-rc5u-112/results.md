# g615 RC5-universal-112 run (2026-10-24): PanProbe and Fallout 4

Device: Poco X6 Pro (Mali-G615 MC6, v11), wifi adb 10.11.176.130:44087 (46755 for FO4), logd stopped (no logcat).
Build: [builds/rc5-universal.md](../../builds/rc5-universal.md), section RC5-universal-112 (.so sha256 `15fc5464...`, csf-v11 223-229).
APKs installed with `adb install -r` (no uninstall): panprobe-rc5u-112.apk (`dev.zenithblue.panvktest.rc5u`), panplay-rc5u-112.apk (`dev.zenithblue.panvklauncher` 1.2.9-rc5 vc 37), both Success.

## PanProbe autorun all (rc5, run=20261024-063243)
- **39 pass / 0 fail / 0 skip, total 39.** DRIVER line: `PanVK-kbase RC5-universal-112 (Mesa 26.3.0-devel (git-a1f95e1585))`.
- bc_decode 16154 ms, bc_perf 44605 ms (cold, first run after install), tess_perf 48545 ms, large_draw 7868 ms, submit_stress 30091 ms, tessellation 3477 fps. Phone on battery, 19-20 %; no same-state A/B with 111 was run, so the times are not a perf comparison.
- Raw: [raw/all/autorun.txt](raw/all/autorun.txt), [raw/all/20261024-063243/](raw/all/20261024-063243/).

## Fallout 4 (gb11a0666, DXVK 3.1.1, build 112 defaults), 2026-10-24 03:41-03:48 UTC
Phone unlocked and charging (57 -> 63 %), adb port 46755. The shortcut env was set from [raw/fallout4/shortcut-gb11a0666-112.json](raw/fallout4/shortcut-gb11a0666-112.json): `DXVK_HUD=full`, `PANVK_DEBUG=mem`, `PANVK_MEM_LOG=.../gfx-logs/panvk-mem-%p.log` and `MESA_LOG_FILE=.../gfx-logs/mesa-fo4.log`. I launched it (pid 27889), got to the main menu and pressed CONTINUE to load the pre-war Sanctuary house save. One sample every 30 s ([raw/fallout4/mon/](raw/fallout4/mon/)), and the in-app log is in [raw/fallout4/panvk-mem-27889.log](raw/fallout4/panvk-mem-27889.log) and [raw/fallout4/mesa-fo4.log](raw/fallout4/mesa-fo4.log).

**Stopped after 6.8 min (03:48:38).** The battery had reached 41.5-41.6 C while charging, close to the 42 C limit, and swap was almost full. Thermal status stayed at 0 throughout and mtktsAP was 49 C. I force-stopped PanPlay, restored the shortcut env to the original (`DXVK_HUD=full` only) and reverted `svc power stayon`. I skipped the 111-knob A/B because the battery was still at 41.6 C after the stop.

| t (panvk-mem) | phase | MemAvailable | SwapFree (of 3666 MB) | kbase (mtk_mali, pid) | devmem | priv | dev_tls | tiler renewals |
|---|---|---|---|---|---|---|---|---|
| 0 s | load | 2265 MB | 2743 MB | 714 MB | 448 | 1.8 | 0 | 0 |
| 32-242 s | title/main menu, ~63-65 fps | 1191-1606 MB | 3000-3043 MB | 752-755 MB | 716 | 22-25 | 0 | 214 -> 2954 |
| 273 s | CONTINUE, loading | 1107 MB | 3044 MB | 816 MB | 780 | 25 | 0 | 3500 |
| 310 s | in game, living room, **7.9 fps** | **265 MB** | 2799 MB | 1775 MB | 1356 | 392 | **336** | 4146 |
| 341 s | kitchen, Codsworth, **4.7 fps** | 416 MB | 872 MB | 2322 MB | 1516 | 603 | 336 | 4430 |
| 371 s | Codsworth close-up, **3.8 fps** | 298 MB | **156 MB** | 2515 MB | 1772 | 581 | 336 | 4572 |

All sizes are in MB. kbase = mtk_mali pages x 4 KiB; devmem, priv and dev_tls come from the `panvk-mem` line.

### Compared with 110 (1.7-5.4 fps, MemAvailable 84 MB, swap full, 2.75 GB kbase)
- **fps: 3.8-7.9 in game**, against 1.7-5.4 on 110. That is somewhat better, but still far from playable. The fps fell as swap filled: 7.9 at the first in-game frame, 3.8 by the third sample. The frametime max was 2008 ms (stalls).
- **MemAvailable low point: 265 MB**, against 84 MB on 110. kbase at the end was **2.51 GB**, against 2.75 GB on 110. Swap was still nearly full: FO4's own VmSwap reached 2.24 GB, and system SwapFree fell to 156 MB.
- **Tiler heaps are now bounded** (patch 224). They start at 4 chunks; on 110 they started at 10. Up to 5 heaps were live. TERM backlog max was 2 and there were 20 synchronous terms. kbase minus (devmem + priv) was 39-60 MB in the menu and **~160 MB in game**. The `init` counter (18.7 GB) adds up every renewal allocation (4680 x 4 MiB), so it is not a resident figure.
- **New finding: `dev_tls` = 336 MB.** It is a single device TLS BO that appears when gameplay starts (peak 341 MB). It is the largest driver-private item, more than half of `priv`. The FO4 gameplay shaders spill heavily, and the TLS size is per-thread stack x all cores x thread slots. Patch 227 frees only the outgrown old BOs (5.2 MB freed), so this one stays. **Next fix candidate:** clamp TLS to the real per-shader need, or drop it on idle.
- Command buffers: `cmd_cs` 88-106 MB in 1400-1700 BOs, `cmd_desc` 37-41 MB, peak 170 MB. Pool auto-trim (226) is active and has freed **656k BOs / 41.9 GB in total** in 6 min. That much churn is wasted CPU work in the submit path. For the A/B, try `PANVK_CMD_POOL_AUTOTRIM=0`, or make the trim keep a larger free list.
- Other categories: `prerast` 96 MB (tessellation arena), shader 7 MB, desc_pool 1.5 MB, utrace 0.1 MB (228). bc_raw stayed at 0 resident with one 2 MB spill. Lazy mode never needed raw stores (`needed=0`, `lazy_skips=0`).
- **Black-head guard (225):** 647 stale marks and 4294 stale reads redirected. Nora was not in view during the 1.6 min of gameplay; Codsworth and the house rendered correctly. **The black-head check is still open.**
- `VK_EXT_memory_budget` was left at the default (opt-in `PANVK_MEM_BUDGET=honest` was not tested).

Screenshots:
- [fo4-01-start.png](screenshots/fo4-01-start.png): title screen.
- [fo4-02-menu.png](screenshots/fo4-02-menu.png): main menu, 65 fps.
- [fo4-03-gameplay.png](screenshots/fo4-03-gameplay.png): living room, 7.9 fps, vidmem 1488 MB.
- [fo4-04-kitchen-codsworth.png](screenshots/fo4-04-kitchen-codsworth.png): kitchen, 4.7 fps.
- [fo4-05-gameplay.png](screenshots/fo4-05-gameplay.png): Codsworth, 3.8 fps, vidmem 1744 MB / 1437 used.

An earlier attempt the same morning did not run because the phone was locked at 19 % battery (commit b631334). For the A/B, the 111 behaviour is `PANVK_BC_RAW_STALE=0 PANVK_BC_RAW_FLIP=1 PANVK_BC_RAW_PIN=2 PANVK_KBASE_HEAP_CHUNKS=10,400 PANVK_KBASE_HEAP_TERM_MAX=16 PANVK_CMD_POOL_AUTOTRIM=0 PANVK_TLS_FREE_OLD=0 PANVK_UTRACE_CLONE_MEM_SIZE=0xa00000`.
