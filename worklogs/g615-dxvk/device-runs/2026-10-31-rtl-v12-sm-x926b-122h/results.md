# v12 SM-X926B (RTL), build 122h (p0174, TILER_IR on by default on v12)

All runs no env. 122g and 116 values for s8 come from the 2026-10-31 122g/116 session on the same device (../2026-10-31-rtl-v12-sm-x926b-122f), same window (20 s warm-up, 60 s measured).

| test | 122h | 122g | 116 |
|---|---|---|---|
| PanProbe full | 41/41, tess_perf PASS, 0 DEVICE_LOST | 41/41 | 41/41 |
| PanProbe total (sum of test ms) | 100.3 s | 101.0 s | 109.6 s |
| workload_perf gs_layer / tex_rgba / msaa_sshade / msaa_a2one / pg_query (us/op, one run) | 52.5 / 24.3 / 61.3 / 109.2 / 1.6 | 86.0 / 29.6 / 73.2 / 119.8 / 2.9 | 62.1 / 21.7 / 62.1 / 107.8 / 2.1 |
| s3 cube fps (122h, 116, 122h, same session) | 484.8 / 483.0 | - | 488.6 |
| s5 D3D12 fps (same session, 1 % low) | 187.1 (160.6) | - | 174.3 (137.2) |
| s8 FL 10_0 / 10_1 / 11_0 / 11_1 fps | 636 / 671 / 733 / 641 | 720 / 694 / 742 / 639 | 669 / 696 / 647 / 655 |
| s8 FL 1 % low | 125 / 126 / 159 / 113 | 152 / 125 / 157 / 139 | 139 / 128 / 128 / 127 |
| s8 D3D10 fps | 514 | 490 | 476 |
| s8 D3D9 fps | 120 (vsync) | 120 | 120 |

All PanPlay passes PASS, deviceLost false. s8 memory was not captured (the mem/ dir is empty).
s8 FL 10_0 and 11_1 on 122h are lower than 122g (single runs, different session); the 116 numbers are in the same range. Needs a same-session A/B to confirm.
tex_rgba bisect not run: the device left rdb at about 09:26, right after s8.
