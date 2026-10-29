# RTL: SM-X846B (Mali-G1-Ultra, v14), build 115e PanPlay: partial (s3, s6, s7 ran; s8 lost, s5/s9 never ran)

Device Galaxy Tab S12+ SM-X846B, MT6993, API 37, Mesa 26.3.0-devel (e7d836d5a6), PanPlay 1.3.0 (44). Override: warm-up 20 s, measure 60 s.
Runs collected by an orphan script after transport recovered (`raw/progress.txt`). Per-file basenames collide when pulled, so for s3 only the
last pass (d3d9) has `perf.csv`/`perf.json`; the d3d11 pass is covered by `results.txt` and `panvk-fps-27655.log`.

## Per scenario

| Scn | Test | Result | fps |
|---|---|---|---|
| 3 | Cube D3D11 (ARM64EC) | PASS | 781.8 drv (1s min 615, max 930), IMMEDIATE present, uncapped |
| 3 | Cube D3D9 (i686) | **FAIL, VK_ERROR_DEVICE_LOST** after ~2 s (exit 3) | 120 fps vsync until hang, then 1.2 fps |
| 6 | D3D12 Gears (vkd3d) | PASS | 306.2 |
| 7 | D3D12 feature levels | 11_0 PASS (357), 11_1 FAIL logic_op (356, expected unsupported on Mali), 12_0 UNSUPPORTED tiled_tier2 (371), 12_1 UNSUPPORTED rov (359), 12_2 not finished (poll cap, pass 5 of 5 was running) | ~350-370 |
| 8 | D3D12 (second set) | NOT COLLECTED: adb transport dropped (`device not found`), `raw/s8` holds only an empty logcat | - |
| 9, 5, 3-cool | - | NOT RUN, see below | - |

Why s7 has no `done:`: the script polled 90 x 5 s = 450 s, s7 needs 5 passes of ~100 s plus startup; pass 5 was still running at the cap. Not a crash.
Why s8 is empty: transport went offline during s8 (logcat pull said `device 'localhost:35395' not found`).
Why s9/s5/s3-cool and the s7/s8 reruns (`progress2.txt`) got NO_NEW_DIR: at rerun time the RTL device had been reprovisioned. `dev.zenithblue.panvklauncher` is no
longer installed (only `dev.zenithblue.panvktest`), `/sdcard/Android/data/dev.zenithblue.panvklauncher` and `/sdcard/Download/gameloop-override-115e.json` are gone, so
`am start TEST_LOOP` has nothing to launch. Reinstall (not allowed this session) is needed to finish s5/s7-pass5/s8/s9.

## Driver errors
- s3 D3D9: first `vkWaitForPresentKHR failed: VK_ERROR_DEVICE_LOST` ~1.6 s after the first frame, then `DxvkSubmissionQueue` submission failures until exit. This is the only
  device-lost; no kbase/CSF message reached the pulled logcat (apps cannot read dmesg). D3D11 cube and all D3D12 passes were clean.
  Likely the same CSF timeout family as the PanProbe tessellation timeouts; needs dmesg/kbase fault capture. D3D9 path = DXVK d3d9 + 32-bit WoW64 FEX.
- No OOM, no page faults seen in app logcat. The mesa `MEM_ALLOC_EX ENOTTY` warning (falls back to MEM_ALLOC) appears as known.

## Frame pacing, s3 cube (what the user sees)
D3D11 cube, 89 one-second intervals (`panvk-fps-27655.log`):
- avg 781.8 fps, median second 778, worst second 615. DXVK stats: p50 1.0 ms, p95 2.6 ms, p99 4.4 ms, 1% low 72.8 fps (about 13.7 ms), max 25.1 ms.
- **Every second has a spike: max_ms per interval 20-27 ms in 88 of 89 intervals (median 21.7), i.e. one ~21 ms frame per second (about 20x the median).** Strictly 1 Hz periodic.
  Matches the 1 Hz samplers running in the app (`perf-sys.csv` is sampled every 1.0 s, plus the panvk-fps 1000 ms interval). The spike is probably the sampler/HUD stalling
  the render or present thread (CPU side), not the GPU. Second suspect: HUD text update (`DXVK_HUD=full`).
- Present mode IMMEDIATE, 4 swapchain images, 1280x705: no vsync pacing, so micro-stutter here is raw frame-time variance; at 780 fps it is invisible to eyes except through
  the 1 Hz hitch.

D3D9 cube (the only pass with per-frame data, `perf.csv`, 190 frames over 1.6 s): vsync/FIFO ran at 120.1 fps (`present_ms 8.04 avg / 12.46 max`).
- avg 118.7 fps, p50 8.25 ms, p95 11.5, p99 15.4, max 36.5 ms (first frame), 1% low 38.5 fps. 5 of 190 frames at 12.0-13.2 ms = exactly one 80 Hz period (12.5 ms), the
  rest at 8.3 ms (120 Hz). That is the display switching between 120 Hz and 80 Hz (LTPO adaptive refresh, `display.txt`: supported 120/80/60, active modeId 1 = 120 Hz with
  80 Hz as alternative refresh), so frames that miss a 120 Hz vsync land on 12.5 ms. Startup also showed present max 399 ms (first present, not steady state).
- Shader compiles: DXVK d3d11 log shows 8 compiler threads; `compilingShare` 0.33 in the d3d9 sample (short window, startup compile), not measurable in steady state for d3d11.
- Thermal: `dumpsys thermalservice` before/after: status 0 (none), AP 29.7 to 27.8 C, SKIN 25.8 to 24.4 C. No throttling. CPU freqs seen scaling 3400 to 1100 MHz (idle governor).

Best-judgment cause of user micro-stutters: (1) 120 Hz vs 80 Hz adaptive refresh switching under FIFO (12.5 ms frames among 8.3 ms ones) and (2) a 1 Hz ~21 ms hitch from the
launcher's perf sampler/HUD. Not thermal, not shader compile, not driver submission in the D3D11 pass. Fix tests (need device): force fixed 120 Hz (Samsung "high" refresh,
disable adaptive) and rerun s3 with sampler/HUD off; confirm hitch vanishes.

Limits: single run, d3d11 pass lacks per-frame csv; d3d9 pass died after 2 s so its pacing sample is only 190 frames.

Raw (small files): `raw/s3`, `raw/s6`, `raw/s7`, `raw/display.txt`, `raw/thermal-*.txt`, `raw/progress*.txt`. Big logs (logcat, wine-run, launcher, mesa-panvk) are not committed.
