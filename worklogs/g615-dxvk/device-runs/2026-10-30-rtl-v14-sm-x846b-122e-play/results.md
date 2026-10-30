# RTL v14 Galaxy Tab S12+ SM-X846B, PanPlay 122e (p0169) no env, scenarios 1-6 (session ended before 7-9 and PanProbe)

Comparison: no 116 or rc4 run exists on this model. The SM-X946B (also v14) single passes are 122e s1 787 / s3 882, 116 s1 ~847 / s3 967. The same-session SM-X946B cube A/B gave 116 747-771, 122e 678-884 (noise).

| metric | 122e | flag |
|---|---|---|
| s1 01 d3d11 default fps | 771.0 |  |
| s1 01 d3d11 default 1%low | 69.6 |  |
| s1 PSS MB | 265.8 |  |
| s1 Graphics MB | 104.5 |  |
| s2 01 d3d9 default fps | 120.0 |  |
| s2 01 d3d9 default 1%low | 64.4 |  |
| s2 PSS MB | 212.1 |  |
| s2 Graphics MB | 105.1 |  |
| s3 01 d3d11 default fps | 797.7 |  |
| s3 01 d3d11 default 1%low | 77.2 |  |
| s3 02 d3d9 default fps | 120.0 |  |
| s3 02 d3d9 default 1%low | 62.3 |  |
| s3 PSS MB | 225.1 |  |
| s3 Graphics MB | 103.9 |  |
| s4 01 d3d11 old fps | 260.8 |  |
| s4 01 d3d11 old 1%low | 120.1 |  |
| s4 02 d3d11 default fps | 824.1 |  |
| s4 02 d3d11 default 1%low | 86.3 |  |
| s4 03 d3d9 old fps | 120.0 |  |
| s4 03 d3d9 old 1%low | 64.8 |  |
| s4 04 d3d9 default fps | 120.0 |  |
| s4 04 d3d9 default 1%low | 61.1 |  |
| s4 PSS MB | 249.9 |  |
| s4 Graphics MB | 107.8 |  |
| s5 01 d3d12 default fps | 286.8 |  |
| s5 01 d3d12 default 1%low | 249.1 |  |
| s5 PSS MB | 265.7 |  |
| s5 Graphics MB | 144.6 |  |
| s6 01 d3d12gears default fps | 316.9 |  |
| s6 01 d3d12gears default 1%low | 266.5 |  |
| s6 PSS MB | 218.3 |  |
| s6 Graphics MB | 110.5 |  |


## Session 5 (same SM-X846B): same-device cube A/B, s3 D3D11 cube, one 20 s pass each, alternating

| WSI | 122e | 116 |
|---|---|---|
| default, round a | 740 | 685 |
| default, round b | 748 | 747 |
| `PANVK_WSI_SW_CACHED=1`, round c | 768 | 756 |
| `PANVK_WSI_SW_CACHED=1`, round d | 777 | 719 |
| mean default / cached | 744 / 772 | 716 / 737 |

122e is at or above 116 in every pair. 122e default (cached on v14 by 0169) is +4 %; with CACHED=1 on both it is +5 %.

## Session 5: s7 D3D12 FL (122e, no env)

FL 11_0 388, 11_1 379, 12_0 393, 12_1 376, 12_2 378 fps, all ok (`play-s7-partial.txt`). s8, s9 and PanProbe were not run: the session was stopped at 25 min.

## Session 6 (same SM-X846B): PanProbe and s8/s9 (122e, no env)

- PanProbe 122e: 41/41, no DEVICE_LOST, driverInfo p0169.
- s8: D3D11 FL 10_0 1230, 10_1 1224, 11_0 1177, 11_1 1209, D3D10 829, D3D9 120 (vsync) fps, all ok.
- s9: D3D12 FL all 379, D3D11 FL all 1193 fps, all ok.
- PanPlay s1-s9 are now complete on v14 for 122e. Pushed configs were removed at the end (cleanup rc=0).
