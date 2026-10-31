# v13 SM-X930 (S11 Ultra, RTL), build 122h (p0174) vs 116, no env

Hardened kit (per-run timeout, PanPlay-only force-stop, live per-pass pulls, cleanup before and after). No hang this time; every kit run rc=0, cleanup rc=0.

| test | 122h | 116 |
|---|---|---|
| PanProbe full | 41/41, tess_perf PASS, 0 DEVICE_LOST, total 97.2 s | not run (122e p0169 on 2026-10-30: 41/41, 103.0 s) |
| s3 cube fps, alternating 122h/116/122h/116 | 1119.3 / 1132.9 | 939.1 / 948.0 (122h +19 %) |
| s5 D3D12 fps (1 % low) | 235.3 (214.9) | not run (time) |
| s7 D3D12 FL 11_0 / 11_1 / 12_0 / 12_1 / 12_2 fps | 592 / 603 / 632 / 632 / 650 | not run |
| s8 FL 10_0 / 10_1 / 11_0 / 11_1 fps | 1587 / 1702 / 1655 / 1582 | not run |
| s8 FL 1 % low | 188 / 246 / 204 / 178 | - |
| s8 D3D10 / D3D9 fps | 1069 / 120 (vsync) | - |
| s8 PSS mean / max MB | 262 / 284 | - |
| s8 Graphics mean / max MB | 147 / 160 | - |

s5/s7/s8 use a shortened window (10 s warm-up, 30 s measured). s7 FL 12_x report UNSUPPORTED (no FL12 on PanVK; deferred), runs at the 11_1 fallback, not a regression.
deviceLost false in every PanPlay results.json.
