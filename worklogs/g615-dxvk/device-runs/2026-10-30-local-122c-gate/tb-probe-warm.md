| metric | rc4 | 116 | 122c | flag |
|---|---|---|---|---|
| gpu_prerast_slice ms | 4079.0 ±3.0 | 145.5 ±23.5 | 146.0 ±25.0 |  |
| clip_cull ms | 5127.5 ±0.5 | 1127.5 ±10.5 | 1119.5 ±3.5 |  |
| multi_viewport ms | 5092.5 ±0.5 | 1091.0 ±4.0 | 1098.5 ±7.5 |  |
| fill_mode ms | 5372.5 ±6.5 | 1200.0 ±32.0 | 1174.0 ±2.0 |  |
| bc_decode ms | 4630.0 ±1.0 | 133.5 ±8.5 | 120.0 ±4.0 |  |
| bc_perf ms | 53849.0 ±214.0 | 46392.0 ±113.0 | 45050.0 ±274.0 |  |
| tess_perf ms | - | 32814.0 ±1326.0 | 34380.0 ±131.0 |  |
| workload_perf ms | 22016.5 ±4.5 | 11425.5 ±101.5 | 9316.5 ±64.5 |  |
| geometry ms | 5516.5 ±8.5 | 1195.0 ±4.0 | 1176.0 ±6.0 |  |
| tessellation ms | 7846.0 ±42.0 | 1836.0 ±3.0 | 1847.0 ±8.0 |  |
| xfb ms | 4325.0 ±6.0 | 200.0 ±4.0 | 210.0 ±10.0 |  |
| pipeline_stats ms | 5198.0 ±9.0 | 1171.5 ±19.5 | 1151.5 ±10.5 |  |
| vertex_stores ms | 4148.5 ±10.5 | 144.0 ±10.0 | 144.0 ±0.0 |  |
| gs_viewport_depth ms | 4245.0 ±3.0 | 309.0 ±1.0 | 283.0 ±15.0 |  |
| vs_viewport_index ms | 4235.0 ±15.0 | 208.5 ±1.5 | 229.0 ±17.0 | worse than 116 |
| depth_bounds ms | 4336.0 ±13.0 | 204.5 ±11.5 | 188.0 ±10.0 |  |
| large_draw ms | - | 10184.5 ±63.5 | 7177.5 ±42.5 |  |
| vmr_secondary ms | 4105.0 ±5.0 | 95.0 ±7.0 | 84.5 ±2.5 |  |
| tess_cond_state ms | 4172.0 ±2.0 | 215.5 ±27.5 | 198.5 ±7.5 |  |
| bachata_reqs ms | 4176.5 ±7.5 | 117.0 ±15.0 | 122.5 ±0.5 |  |
| bachata_exec ms | 4093.5 ±10.5 | 107.0 ±3.0 | 114.5 ±10.5 |  |
| robustness2 ms | 4104.5 ±11.5 | 109.0 ±6.0 | 109.5 ±7.5 |  |
| robust_image_access2 ms | 10515.5 ±19.5 | 6514.0 ±5.0 | 6493.5 ±4.5 |  |
| blend ms | 4101.5 ±1.5 | 100.5 ±8.5 | 100.0 ±7.0 |  |
| occlusion_query ms | 4101.5 ±2.5 | 120.0 ±4.0 | 115.5 ±8.5 |  |
| descriptor_model ms | 4098.0 ±1.0 | 123.5 ±0.5 | 123.5 ±2.5 |  |
| shader_arith ms | 4112.5 ±15.5 | 124.0 ±4.0 | 131.5 ±9.5 |  |
| depth_stencil ms | 4120.0 ±8.0 | 134.5 ±7.5 | 137.0 ±2.0 |  |
| sampler ms | 4117.5 ±15.5 | 125.5 ±0.5 | 123.5 ±2.5 |  |
| draw_params ms | 4138.0 ±17.0 | 179.0 ±8.0 | 192.5 ±2.5 | worse than 116 |
| submit_stress ms | 34090.0 ±1.0 | 30116.0 ±7.0 | 30110.0 ±5.0 |  |
| csf_event ms | 4561.5 ±4.5 | 274.5 ±2.5 | 277.0 ±1.0 |  |
| gs_tess_primitive_id ms | 4314.5 ±2.5 | 213.0 ±3.0 | 220.5 ±15.5 |  |
| bachata_storage_fmtless ms | 4099.0 ±4.0 | 123.0 ±8.0 | 116.0 ±12.0 |  |
| bachata_dynamic_render ms | 4103.0 ±1.0 | 118.0 ±3.0 | 122.0 ±3.0 |  |
| dxvk_reqs ms | 2068.0 ±14.0 | 75.5 ±0.5 | 88.0 ±5.0 | worse than 116 |
| vkd3d_reqs ms | 2070.0 ±4.0 | 83.5 ±4.5 | 90.0 ±2.0 |  |
| d3d_fl ms | 2057.0 ±0.0 | 82.0 ±3.0 | 91.0 ±3.0 | worse than 116 |
| vkd3d_heap ms | 10348.5 ±13.5 | 375.5 ±3.5 | 345.5 ±17.5 |  |
| vkd3d_timeline ms | 4230.0 ±2.0 | 221.5 ±4.5 | 226.0 ±0.0 |  |
| swapchain_lifecycle ms | 10336.5 ±34.5 | 6362.5 ±13.5 | 6357.5 ±26.5 |  |
| wl bc_level us | 108437.5 ±333.5 | 104681.5 ±3394.5 | 257520.5 ±2352.5 | worse than rc4 worse than 116 |
| wl ref us | 13.2 ±0.2 | 12.0 ±3.6 | 13.7 ±2.0 |  |
| wl gs_pass us | 54.8 ±0.1 | 39.8 ±4.2 | 13.9 ±2.0 |  |
| wl gs_layer us | 50.0 ±0.9 | 38.8 ±4.8 | 33.8 ±1.6 |  |
| wl xfb_vs us | 463.9 ±0.4 | 462.8 ±0.4 | 90.4 ±0.1 |  |
| wl xfb_gs us | 864.7 ±0.2 | 511.1 ±3.7 | 236.5 ±1.6 |  |
| wl strip_ref us | 18.5 ±0.2 | 16.6 ±0.2 | 19.0 ±0.1 | worse than 116 |
| wl strip_gs us | 1927.9 ±3.7 | 1183.7 ±0.2 | 18.9 ±0.9 |  |
| wl cond_true us | 18.5 ±0.3 | 17.7 ±0.1 | 19.2 ±0.0 | worse than 116 |
| wl cond_false us | 8.3 ±0.2 | 7.9 ±0.1 | 8.2 ±0.1 |  |
| wl pg_query us | 21.7 ±0.2 | 20.7 ±0.2 | 15.4 ±0.3 |  |
| wl ts_ref us | 736.2 ±3.9 | 731.6 ±14.5 | 723.5 ±4.9 |  |
| wl ts_pair us | 1010.7 ±16.4 | 960.6 ±13.2 | 965.5 ±8.2 |  |
| wl msaa_ref us | 251.7 ±0.4 | 254.3 ±2.7 | 253.0 ±1.1 |  |
| wl msaa_sshade us | 1176.6 ±6.3 | 1190.3 ±1.2 | 1169.4 ±4.0 |  |
| wl tex_rgba us | 254.3 ±7.7 | 247.1 ±0.1 | 245.9 ±1.4 |  |
| wl tex_bgra us | 275.5 ±5.4 | 274.5 ±2.0 | 272.8 ±0.8 |  |
| PSS max MB | 228.4 ±2.4 | 224.3 ±5.5 | 232.6 ±4.3 |  |
| Graphics max MB | 129.7 ±1.0 | 125.0 ±1.8 | 129.0 ±0.3 |  |

FLAGS:
- vs_viewport_index ms: worse than 116
- draw_params ms: worse than 116
- dxvk_reqs ms: worse than 116
- d3d_fl ms: worse than 116
- wl bc_level us: worse than rc4 worse than 116
- wl strip_ref us: worse than 116
- wl cond_true us: worse than 116
