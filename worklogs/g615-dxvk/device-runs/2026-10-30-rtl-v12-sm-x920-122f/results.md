# RTL v12 SM-X920 (Galaxy Tab S10 Ultra), build 122f (p0171) smoke test

- PanProbe 122f: 40/41, no DEVICE_LOST. `tess_perf` FAIL: every captured image hash matches between options and gs=0/1, but the RESULT line is cut after 16 segments, so the failing check is not in the saved output. Open.
- PanPlay 122f, no env (fps / 1 % low, max PSS / Graphics MB):

| scen | test | fps | 1 % low | PSS | Graphics |
|---|---|---|---|---|---|
| s3 | D3D11 cube | 603 | 73 | 259 | 93 |
| s3 | D3D9 | 120 (vsync) | 55 | | |
| s8 | D3D11 FL 10_0 / 10_1 / 11_0 / 11_1 | 914 / 820 / 765 / 837 | 139 / 109 / 147 / 150 | 238 | 74 |
| s8 | D3D10 | 623 | 77 | | |
| s5 | D3D12 | 180 | - | 227 | 93 |
| s6 | D3D12 gears | 292 | - | see s6.txt | |

- s8 pass 6 (D3D9) did not run: my 480 s cap for s8 was too short for 6 passes of ~95 s. This was not a hang; passes 1-5 were saved by the live pull.
- 0171 readback A/B (logcat `wsi/x11 sw readback`): the D3D11 cube picked cached (cached 745-762 vs uncached 412-416 fps), and D3D9 picked cached (120 vs 120, vsync). The D3D12 s5 picked uncached (182 vs 199). The s3 cube average includes the A/B windows.
- Cleanup at start and end: rc=0.
