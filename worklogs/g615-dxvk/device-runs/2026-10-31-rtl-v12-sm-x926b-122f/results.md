# RTL v12 Galaxy Tab S10 Ultra SM-X926B, 2026-10-31: 122g (p0173) vs 116, same device, .so kit

Fresh device: PanPlay 122f installed for the kit (the .so files replace its driver per env set).

| test | 122g | 116 | 122g vs 116 |
|---|---|---|---|
| s3 cube, no env (alternating 116, 122g, 116, 122g) | 471 / 491 | 502 / 469 | -1 % (noise) |
| s3 cube, `PANVK_WSI_SW_CACHED=1` on 122f vs 116 | 122f 478 / 495 | 448 / 461 | +7 % |
| s6 gears 122g, no env | 227 fps (1 s min 158, max 264) | - | one readback decision (cached) |
| s8 FL 10_0 / 10_1 / 11_0 / 11_1 fps | 720 / 694 / 742 / 639 | 669 / 696 / 647 / 655 | mean +5 % |
| s8 FL 1 % low | 152 / 125 / 157 / 139 | 139 / 128 / 128 / 127 | mean +10 % |
| s8 D3D10 fps / 1 % low | 490 / 64 | 476 / 64 | +3 % |
| s8 D3D9 fps | 120 (vsync) | 120 (vsync) | same |
| s8 PSS max / mean MB | 296 / 265 | 333 / 286 | lower |
| s8 Graphics max / mean MB | 120 / 102 | 120 / 107 | same or lower |
| PanProbe full | 41/41, tess_perf PASS | 41/41, tess_perf PASS | total test time 101.0 s vs 109.6 s |

- 0173 decide-once: one `wsi/x11 sw readback` decision per swapchain (cube: cached 489/439 vs uncached 440 -> cached; gears: cached 237/214 vs uncached 215 -> cached). The 122f gears flip-flop (5 decisions per pass on SM-X920) is gone.
- PanProbe 122g vs 116, per test (>10 % and >30 ms): `bc_decode` 447 vs 3039 ms, `workload_perf` 7926 vs 10958, `bc_perf` 22108 vs 25301, `vkd3d_timeline` 214 vs 272, `csf_event` 211 vs 242. No test is slower by more than that.
- `workload_perf` cases slower on 122g (single run each): `gs_layer` 86 vs 62 us, `tex_rgba` 29.6 vs 21.7, `msaa_sshade` 73 vs 62, `msaa_a2one` 120 vs 108, `pg_query` 2.9 vs 2.1. Faster: `bc_level` 18.8k vs 459k us, `gs_pass` 1.8 vs 64.6, `strip_gs` 2.8 vs 211, `msaa_ref` 23.6 vs 27.9.
- s5 D3D12 122g vs 116 was not run: the time ran out. Cleanup at the end: rc=0.
