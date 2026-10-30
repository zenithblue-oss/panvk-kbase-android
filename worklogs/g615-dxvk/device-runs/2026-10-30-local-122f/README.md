# Build 122f (2026-10-30): local gate, X6 Pro v11 and TB336FU v9

Build 122f = 0006-0171 (`prepare-rc5-tree.sh --wip-max 0171`, strict: `OK base=228 jm-v7-wip=166 head=e1d5a21c4a9`).
- driverInfo `Mesa 26.3.0-devel (git-e1d5a21c4a, p0171, 2026-10-30)`
- .so `/var/tmp/panvk/build-out/2026-10-30-122f/libvulkan_panfrost.so`, sha256
  `6ca4bb85e0395ccd363415910d4ab9d7c04d54229ee7ac06d4ec71f8faa9041b`, BuildID `5dff0d019d295b5cc0d915ba6cc01feb8506f681`
- APKs `/var/tmp/panvk/build-out/apks/panprobe-122f.apk`, `/var/tmp/panvk/build-out/apks/panplay-122f.apk`
- **Not the default.** The TB336FU PanPlay gate did not finish (the device dropped off USB and Wi-Fi), so the pins stay on 122e.

New patches: 0170 (BC decode per-texel fallback, `PANVK_BC_DECODE_TEXEL=0` = 0153), 0171 (sw WSI readback mode per
swapchain, `PANVK_WSI_SW_CACHED=0/1` forces a mode). See `patches/jm-v7-wip/README.md`.

## Files

- `x6f-probe-cold.md`, `x6f-probe-warm.md`, `x6f-play.md`: X6 Pro gate tables, rc4 vs 116 vs 122f (`b122/table.py`).
- `tbf-probe-cold.md`, `tbf-probe-warm.md`: TB336FU PanProbe tables. `tb336fu/play-rc4c` done, `play-116c` partial, no 122f.
- `x6pro/`, `tb336fu/`: per-run summaries, PanPlay `mem/` with launcher and game process (`EXE <pid>`) meminfo per pass.
- `tbmem2/` (`mem-exe-tb.json`): TB336FU rc4 / 116 / 122e / dev8 in one APK and session, game process memory.
- `x6auto1/` (dev8, frame-count windows), `x6auto9/` (dev9 = final 0171, `wsi-x6-auto9.json`): 0171 vs 122e cached / uncached.
- `bc/`: TB336FU imported-slot BC A/B (`bcab.sh`, `bcwarm.sh`): `bc7.txt` dev6 cold, `bcw1.txt` dev9, `bcw2.txt` dev10 = final 0170.
- `scripts/`: the runners (`gate.sh` PanProbe part, `gate-play.sh` PanPlay part, `play-cfg.sh` with game process meminfo).

## Task 1: v11 sw readback (0171)

Cube and FL both present 640x360 on the X6 Pro, so the frame size does not decide it. Cached + invalidate helps the
present-thread-bound cube (uncached CPU reads are slow) and hurts the CPU-bound FL (the DC CIVAC over the frame costs
the game thread). 0171 measures both modes per swapchain (A, B, A windows) and keeps the faster.

`x6auto9`, fps:

| task | 0171 auto | 122e cached | 122e uncached | 116 (earlier gates) |
|---|---|---|---|---|
| D3D11 cube | 352, 383 | 370 | 271 | 312-333 |
| D3D11 FL 11_1 | 753, 709 | 562 | 759 | 404-443 |
| D3D9 | 118, 118 | 118 | 118 | - |

## Task 2: BC cold compile on v9 (0170)

TB336FU, imported slot, alternating (`bcw2.txt`):

| | 0170 (dev10) | 116 | 122e |
|---|---|---|---|
| `bc_decode` cold ms | 877, 886 | 937, 900 | 1830, 1847 |
| `bc_decode` warm ms | 246, 189 | 182, 177 | 234, 674 |
| `bc_level` cold ms (6 ops) | 818, 870 | 775, 823 | 1748 |

- dev9 created the block decoder synchronously after a per-texel cache hit: warm `bc_decode` was 1777 ms. The block
  decoder is not always in the cache (the test process is killed before the background compile ends). Fixed in dev10.
- In these short tests the block decoder never takes over (`bc_level` GPU wait 135 ms = per-texel, 122e 65 ms). Games
  get it once the background compile ends.

## Task 3: PanPlay memory vs rc4

- `dumpsys meminfo <launcher>` (the gate's PSS / Graphics until 122e) measures the launcher process. Its Graphics is the
  in-process X server's EGL mtrack (~162 MB) + GL mtrack (~48 MB), the same with every driver (tbmem1: rc4 and 122e
  both 210224 KB). The PSS/Graphics differences in the earlier gates were session and APK noise, not the driver.
- The game process (`EXE` lines) holds the driver's memory. TB336FU `tbmem2`, mean of 2 passes:

  | task | rc4 PSS / GL MB | 116 | 122e | dev8 |
  |---|---|---|---|---|
  | D3D11 cube | 299 / 217 | 139 / 57 | 139 / 57 | 142 / 57 |
  | D3D9 | 299 / 225 | 139 / 65 | 139 / 65 | 183 / 70 (one pass with an 81 MB native heap spike) |
  | D3D11 FL 11_0 | 250 / 185 | 90 / 24 | 90 / 24 | 92 / 24 |

- So the driver uses ~160 MB less than rc4 since 116. No driver change was needed.

## X6 Pro gate (122f vs rc4 vs 116, one session, battery 28-35 C)

- PanProbe: 41/41 cold and warm x2 for 122f, 116 and rc4.
- PanPlay 1-9: 23/23 ok on 116 and 122f; rc4 15 ok with 2 DEVICE_LOST.
- Better: FL 10_0-11_1 605-747 fps vs 116 434-468 and rc4 581-655; D3D10 357 vs 298 / 290; D3D12 gears 198 vs 161,
  D3D12 FL 261-307 vs 200-231; D3D9 same.
- Same as 116: D3D11 cube s1 333 vs 353, s3 326 vs 337, s4 354 vs 320 (single passes; 116 ran above its earlier range).
- Worse: D3D11 `old` s4 267 vs rc4 290 (116 265); 1 % lows on D3D11 / FL below rc4's (rc4 sync present path).
- Memory: game process GL 63-74 MB vs rc4 259-344 on D3D9/D3D11; PSS max (both processes) 309-399 MB vs rc4 348-423.
- PanProbe: cold `bc_decode` 5184 ms vs rc4 554 and `bc_level` 449 ms/op vs rc4 46 (v11 BC transcoder compile cost, since
  116); warm `bc_decode` 148 vs 116 196 / rc4 576.

## TB336FU (v9)

- PanProbe 41/41 cold and warm x2 (rc4 39/41). Cold `bc_decode` 654 vs 116 653 vs rc4 4608; `bc_perf` 45.4 s vs 46.7 / 53.8;
  `large_draw` 7.6 s vs 10.3; memory same.
- Small flags vs 116 (tess_cond_state cold 315 vs 231 ms, robustness2, shader_arith, draw_params): 10-80 ms on short tests,
  similar spread to the 122c gate.
- PanPlay: not finished (device lost at 116c scenario 2). `tbmem2` (dev8 = 0170 first version + 0171 frame-count version):
  cube 155-161 vs 116 159-161 vs rc4 158, FL 11_0 199-215 vs 216-220 / 208-212, D3D9 90 for all.
