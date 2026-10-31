# v14 SM-X940 (S12, RTL): 122i (p0175) vs 116, no env

PanProbe and PanPlay in one rdb session 10:32-11:25. PanPlay default window (20 s warm-up, 60 s measured), cube 5 s / 20 s. Kit rc=0, cleanup rc=0 every run, no DEVICE_LOST, no hang.
First s5-s9 attempt failed at once: PanPlay 1.3.0 rejects a config with scenario keys outside 1..15 ("scenario numbers must be 1..15"); renumbered and reran.

| test | 122i | 116 | 122i vs 116 |
|---|---|---|---|
| PanProbe full | 41/41, tess_perf PASS, 107.8 s | 41/41, 117.3 s | -8 % time |
| PanProbe PSS max / mean MB | 169 / 124 | 167 / 118 | +1 % / +5 % |
| PanProbe faster tests ms | bc_decode 460, workload_perf 7994, bachata_exec 191, depth_stencil 242 | 2679, 10610, 2756, 804 | faster |
| PanProbe slower tests ms | shader_arith 214, csf_event 456, vertex_stores 294, gpu_prerast_slice 220, robust_image_access2 403, blend 207, bachata_storage_fmtless 204 | 132, 325, 205, 174, 344, 177, 153 | slower (one run each) |
| workload_perf gs_layer / tex_rgba / msaa_sshade / msaa_a2one us | 94.3 / 28.7 / 58.6 / 100.9 | 68.2 / 25.6 / 54.4 / 88.7 | slower (one run) |
| s3 cube fps, order 122i/116/122i/116 | 466 (cold first pass) / 712 | 711 / 655 | equal (warm) |
| s5 D3D12 fps (1 s min) | 265.0 (249.9) | 263.1 (241.8) | equal |
| s6 gears fps (1 s min) | 316.0 (258.0) | 318.4 (251.3) | equal |
| s7 D3D12 FL 11_0 / 11_1 / 12_0 / 12_1 / 12_2 fps | 379 / 475 / 380 / 372 / 382 | not run | - |
| s8 FL 10_0 / 10_1 / 11_0 / 11_1 fps | 1295 / 1187 / 1264 / 1261 | 1262 / 1227 / 1223 / 1235 | +3 / -3 / +3 / +2 % |
| s8 FL 1 % low | 184 / 155 / 170 / 166 | 168 / 166 / 178 / 161 | +10 / -7 / -5 / +3 % |
| s8 D3D10 / D3D9 fps | 876 / 120 | not run (time cap) | - |
| s8 FL passes PSS mean / max MB | 261 / 286 | 266 / 287 | equal |
| s8 FL passes Graphics mean / max MB | 144 / 159 | 145 / 162 | equal |

s9 and FL 10_0 repeats skipped (time). Single runs for 122i vs 116 except cube.
