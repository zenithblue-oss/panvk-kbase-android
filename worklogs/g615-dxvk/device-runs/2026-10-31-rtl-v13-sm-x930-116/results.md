# v13 SM-X930 (S11 Ultra, RTL): 122h (p0174) vs 116, no env

Same kit and short window (10 s warm-up, 30 s measured) for both builds. 122h data: ../2026-10-31-rtl-v13-sm-x930-122h (09:30-09:54); 116 data: this dir (09:56-10:16). Only cube (s3) and gears (s6) ran both builds in one session.
All kit runs rc=0, cleanup rc=0, no DEVICE_LOST, no hang.

| test | 122h | 116 | 122h vs 116 |
|---|---|---|---|
| PanProbe full | 41/41, 97.2 s | 41/41, 106.5 s | -9 % time |
| PanProbe bc_decode / workload_perf / csf_event ms | 299 / 7831 / 223 | 2638 / 10527 / 274 | faster |
| workload_perf gs_layer / tex_rgba / msaa_sshade / msaa_a2one / pg_query us | 85.2 / 40.7 / 98.3 / 147.8 / 1.9 | 74.2 / 57.4 / 157.3 / 237.7 / 5.7 | gs_layer +15 % (one run), rest faster |
| s3 cube fps (same session, alternating) | 1119 / 1133 | 939 / 948 | +19 % |
| s5 D3D12 fps (1 s min) | 235.3 (214.9) | 208.6 (190.4) | +13 % |
| s6 gears fps (1 s min, same session) | 465.8 (328.0) | 493.1 (360.4) | -5.5 % (one pass each) |
| s7 D3D12 FL 11_0 / 11_1 / 12_0 / 12_1 / 12_2 fps | 592 / 603 / 632 / 632 / 650 | 612 / 612 / 600 / 617 / 617 | equal (-3 % to +5 %) |
| s7 1 s min | 403 / 450 / 410 / 432 / 378 | 490 / 465 / 433 / 413 / 411 | 122h lower on 11_0 |
| s8 FL 10_0 / 10_1 / 11_0 / 11_1 fps | 1587 / 1702 / 1655 / 1582 | 1431 / 1459 / 1419 / 1429 | +11 to +17 % |
| s8 FL 1 % low | 188 / 246 / 204 / 178 | 255 / 240 / 220 / 259 | 122h lower on 10_0, 11_1 |
| s8 D3D10 fps | 1069 | 956 | +12 % |
| s8 D3D9 fps | 120 (vsync) | 120 (vsync) | same |
| s8 PSS mean / max MB | 262 / 284 | 258 / 282 | +1.5 % |
| s8 Graphics mean / max MB | 147 / 160 | 146 / 161 | same |

Flags (single runs, need confirmation): gears -5.5 %, s8 FL 10_0/11_1 1 % low and s7 FL 11_0 1 s min lower on 122h, workload_perf gs_layer +15 %. TILER_IR (0174) is v12-only, so v13 runs the default tiler path.
