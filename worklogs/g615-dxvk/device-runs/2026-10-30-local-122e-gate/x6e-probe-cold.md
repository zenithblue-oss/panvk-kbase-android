| metric | rc4 | 116 | 122e | flag |
|---|---|---|---|---|
| gpu_prerast_slice ms | 86.5 ±16.5 | 88.0 ±8.0 | 70.0 ±3.0 |  |
| clip_cull ms | 1267.0 ±7.0 | 1202.0 ±70.0 | 1245.5 ±6.5 |  |
| multi_viewport ms | 1171.5 ±16.5 | 1133.5 ±9.5 | 1138.0 ±6.0 |  |
| fill_mode ms | 1759.5 ±120.5 | 1554.0 ±44.0 | 1479.5 ±1.5 |  |
| bc_decode ms | 533.5 ±36.5 | 5070.0 ±21.0 | 5250.5 ±132.5 | worse than rc4 |
| bc_perf ms | 27996.5 ±36.5 | 32873.0 ±252.0 | 32538.0 ±433.0 | worse than rc4 |
| tess_perf ms | 49132.0 ±377.0 | 28909.5 ±10.5 | 27159.5 ±114.5 |  |
| workload_perf ms | 10070.0 ±173.0 | 14679.0 ±81.0 | 11000.5 ±2.5 | worse than rc4 |
| geometry ms | 1972.5 ±9.5 | 1717.0 ±84.0 | 1564.0 ±1.0 |  |
| tessellation ms | 2773.5 ±120.5 | 2295.5 ±164.5 | 2065.5 ±58.5 |  |
| xfb ms | 320.0 ±18.0 | 290.0 ±39.0 | 287.5 ±19.5 |  |
| pipeline_stats ms | 1346.0 ±12.0 | 1245.5 ±20.5 | 1219.0 ±1.0 |  |
| vertex_stores ms | 173.5 ±7.5 | 129.5 ±6.5 | 135.0 ±1.0 |  |
| gs_viewport_depth ms | 175.0 ±13.0 | 169.5 ±15.5 | 162.0 ±9.0 |  |
| vs_viewport_index ms | 215.0 ±21.0 | 171.5 ±1.5 | 190.0 ±24.0 |  |
| depth_bounds ms | 168.5 ±34.5 | 133.5 ±2.5 | 132.0 ±11.0 |  |
| large_draw ms | 8434.0 ±322.0 | 8348.5 ±144.5 | 7890.5 ±116.5 |  |
| vmr_secondary ms | 100.5 ±17.5 | 93.5 ±9.5 | 84.0 ±9.0 |  |
| tess_cond_state ms | 191.5 ±7.5 | 195.0 ±11.0 | 184.5 ±30.5 |  |
| bachata_reqs ms | 141.5 ±19.5 | 87.5 ±2.5 | 56.5 ±6.5 |  |
| bachata_exec ms | 92.0 ±3.0 | 76.0 ±1.0 | 70.5 ±7.5 |  |
| robustness2 ms | 74.0 ±12.0 | 75.5 ±14.5 | 84.0 ±10.0 |  |
| robust_image_access2 ms | 666.5 ±5.5 | 631.0 ±5.0 | 623.5 ±5.5 |  |
| blend ms | 139.5 ±1.5 | 131.0 ±23.0 | 107.0 ±21.0 |  |
| occlusion_query ms | 155.0 ±56.0 | 80.5 ±5.5 | 79.5 ±8.5 |  |
| descriptor_model ms | 85.5 ±8.5 | 81.5 ±3.5 | 72.0 ±1.0 |  |
| shader_arith ms | 108.5 ±3.5 | 87.5 ±6.5 | 103.5 ±4.5 | worse than 116 |
| depth_stencil ms | 115.5 ±12.5 | 95.5 ±1.5 | 91.5 ±6.5 |  |
| sampler ms | 96.5 ±11.5 | 88.0 ±4.0 | 77.0 ±7.0 |  |
| draw_params ms | 129.0 ±3.0 | 106.5 ±6.5 | 109.5 ±0.5 |  |
| submit_stress ms | 30090.0 ±4.0 | 30079.0 ±5.0 | 30079.5 ±11.5 |  |
| csf_event ms | 389.0 ±2.0 | 396.0 ±21.0 | 311.5 ±13.5 |  |
| gs_tess_primitive_id ms | 276.5 ±24.5 | 179.5 ±27.5 | 200.5 ±44.5 |  |
| bachata_storage_fmtless ms | 136.0 ±10.0 | 85.5 ±1.5 | 80.5 ±10.5 |  |
| bachata_dynamic_render ms | 96.5 ±5.5 | 66.5 ±7.5 | 80.0 ±7.0 |  |
| dxvk_reqs ms | 56.5 ±5.5 | 68.5 ±7.5 | 74.0 ±13.0 |  |
| vkd3d_reqs ms | 60.0 ±0.0 | 58.5 ±2.5 | 61.0 ±5.0 |  |
| d3d_fl ms | 53.5 ±1.5 | 56.0 ±6.0 | 74.0 ±14.0 | worse than rc4 |
| vkd3d_heap ms | 231.0 ±3.0 | 265.5 ±0.5 | 232.5 ±9.5 |  |
| vkd3d_timeline ms | 243.0 ±0.0 | 257.5 ±5.5 | 246.0 ±26.0 |  |
| swapchain_lifecycle ms | 5713.0 ±689.0 | 6230.0 ±297.0 | 5220.5 ±122.5 |  |
| wl bc_level us | 46221.5 ±297.5 | 790430.5 ±32097.5 | 450415.0 ±15880.0 | worse than rc4 |
| wl ref us | 3.6 ±0.3 | 3.2 ±0.0 | 2.7 ±0.2 |  |
| wl gs_pass us | 109.4 ±3.8 | 99.7 ±0.3 | 2.7 ±0.1 |  |
| wl gs_layer us | 100.9 ±1.9 | 113.4 ±9.5 | 67.2 ±1.6 |  |
| wl xfb_vs us | 130.5 ±2.4 | 170.5 ±7.8 | 216.6 ±1.9 | worse than rc4 worse than 116 |
| wl xfb_gs us | 521.1 ±56.8 | 528.0 ±22.9 | 196.6 ±0.6 |  |
| wl strip_ref us | 7.8 ±0.4 | 7.2 ±0.2 | 5.6 ±0.9 |  |
| wl strip_gs us | 373.4 ±60.7 | 448.4 ±142.9 | 4.3 ±0.1 |  |
| wl cond_true us | 3.8 ±0.8 | 3.0 ±0.1 | 3.0 ±0.2 |  |
| wl cond_false us | 3.0 ±0.1 | 3.1 ±0.2 | 2.2 ±0.0 |  |
| wl pg_query us | 3.7 ±0.1 | 4.2 ±0.1 | 2.6 ±0.0 |  |
| wl ts_ref us | 449.1 ±22.6 | 474.8 ±12.1 | 295.6 ±11.1 |  |
| wl ts_pair us | 530.9 ±12.8 | 462.1 ±37.9 | 375.7 ±45.7 |  |
| wl msaa_ref us | 64.6 ±12.7 | 93.3 ±12.3 | 55.8 ±2.1 |  |
| wl msaa_a2one us | 264.7 ±0.0 | 266.7 ±6.1 | 256.6 ±3.5 |  |
| wl msaa_sshade us | 243.7 ±5.8 | 252.1 ±1.4 | 247.5 ±3.7 |  |
| wl tex_rgba us | 59.8 ±3.4 | 91.6 ±8.9 | 59.9 ±15.5 |  |
| wl tex_bgra us | 83.6 ±16.2 | 72.0 ±17.5 | 57.9 ±13.6 |  |
| PSS max MB | 185.5 ±18.2 | 184.9 ±0.7 | 182.0 ±3.4 |  |
| Graphics max MB | 74.0 ±23.0 | 64.6 ±0.1 | 64.7 ±0.0 |  |

FLAGS:
- bc_decode ms: worse than rc4
- bc_perf ms: worse than rc4
- workload_perf ms: worse than rc4
- shader_arith ms: worse than 116
- d3d_fl ms: worse than rc4
- wl bc_level us: worse than rc4
- wl xfb_vs us: worse than rc4 worse than 116
