| metric | rc4 | 116 | 122f | flag |
|---|---|---|---|---|
| gpu_prerast_slice ms | 86.0 ±4.0 | 97.0 ±4.0 | 80.5 ±9.5 |  |
| clip_cull ms | 1291.0 ±5.0 | 1188.0 ±2.0 | 1170.0 ±0.0 |  |
| multi_viewport ms | 1164.0 ±1.0 | 1127.5 ±3.5 | 1130.5 ±23.5 |  |
| fill_mode ms | 1734.0 ±2.0 | 1465.5 ±12.5 | 1475.0 ±4.0 |  |
| bc_decode ms | 576.0 ±13.0 | 196.0 ±12.0 | 147.5 ±6.5 |  |
| bc_perf ms | 28233.5 ±178.5 | 28234.5 ±2.5 | 28208.0 ±775.0 |  |
| tess_perf ms | 49431.5 ±314.5 | 28588.5 ±1.5 | 27477.0 ±34.0 |  |
| workload_perf ms | 10129.0 ±185.0 | 14461.0 ±73.0 | 10660.5 ±57.5 | worse than rc4 |
| geometry ms | 1891.0 ±37.0 | 1494.5 ±6.5 | 1454.0 ±23.0 |  |
| tessellation ms | 2795.0 ±16.0 | 1919.0 ±3.0 | 1928.5 ±21.5 |  |
| xfb ms | 299.0 ±28.0 | 198.5 ±21.5 | 185.0 ±30.0 |  |
| pipeline_stats ms | 1333.5 ±50.5 | 1201.0 ±10.0 | 1205.0 ±8.0 |  |
| vertex_stores ms | 171.5 ±13.5 | 157.5 ±14.5 | 146.0 ±30.0 |  |
| gs_viewport_depth ms | 161.5 ±1.5 | 154.0 ±16.0 | 143.0 ±3.0 |  |
| vs_viewport_index ms | 199.0 ±7.0 | 141.0 ±5.0 | 130.0 ±6.0 |  |
| depth_bounds ms | 132.0 ±2.0 | 106.0 ±5.0 | 121.0 ±21.0 |  |
| large_draw ms | 8618.0 ±42.0 | 8086.5 ±60.5 | 8062.0 ±98.0 |  |
| vmr_secondary ms | 76.5 ±7.5 | 102.0 ±18.0 | 88.0 ±13.0 |  |
| tess_cond_state ms | 157.5 ±4.5 | 163.0 ±1.0 | 173.0 ±1.0 | worse than rc4 worse than 116 |
| bachata_reqs ms | 125.0 ±5.0 | 77.0 ±4.0 | 74.0 ±7.0 |  |
| bachata_exec ms | 70.5 ±0.5 | 66.0 ±4.0 | 69.5 ±4.5 |  |
| robustness2 ms | 79.0 ±7.0 | 72.5 ±12.5 | 74.5 ±10.5 |  |
| robust_image_access2 ms | 652.0 ±14.0 | 637.5 ±9.5 | 607.5 ±3.5 |  |
| blend ms | 115.5 ±0.5 | 86.5 ±1.5 | 121.0 ±5.0 | worse than 116 |
| occlusion_query ms | 93.0 ±9.0 | 76.5 ±6.5 | 83.5 ±4.5 |  |
| descriptor_model ms | 83.5 ±2.5 | 65.5 ±3.5 | 71.0 ±11.0 |  |
| shader_arith ms | 98.0 ±21.0 | 79.5 ±7.5 | 99.0 ±5.0 | worse than 116 |
| depth_stencil ms | 104.0 ±7.0 | 105.0 ±13.0 | 86.0 ±2.0 |  |
| sampler ms | 89.5 ±9.5 | 72.5 ±0.5 | 78.0 ±7.0 |  |
| draw_params ms | 114.0 ±1.0 | 111.0 ±2.0 | 94.5 ±0.5 |  |
| submit_stress ms | 30099.0 ±11.0 | 30076.0 ±4.0 | 30088.5 ±24.5 |  |
| csf_event ms | 384.5 ±1.5 | 340.5 ±3.5 | 296.5 ±40.5 |  |
| gs_tess_primitive_id ms | 208.0 ±12.0 | 154.5 ±2.5 | 168.5 ±3.5 | worse than 116 |
| bachata_storage_fmtless ms | 100.5 ±30.5 | 77.0 ±5.0 | 78.5 ±0.5 |  |
| bachata_dynamic_render ms | 80.0 ±8.0 | 61.0 ±10.0 | 67.0 ±7.0 |  |
| dxvk_reqs ms | 67.0 ±18.0 | 49.5 ±9.5 | 55.5 ±3.5 |  |
| vkd3d_reqs ms | 66.5 ±5.5 | 82.0 ±10.0 | 80.5 ±10.5 |  |
| d3d_fl ms | 75.0 ±9.0 | 74.0 ±13.0 | 67.0 ±7.0 |  |
| vkd3d_heap ms | 243.5 ±17.5 | 234.0 ±10.0 | 234.5 ±17.5 |  |
| vkd3d_timeline ms | 227.5 ±5.5 | 247.5 ±11.5 | 219.5 ±17.5 |  |
| swapchain_lifecycle ms | 5622.5 ±638.5 | 5118.5 ±127.5 | 5559.0 ±214.0 | worse than 116 |
| wl bc_level us | 45270.5 ±458.5 | 793338.0 ±13845.0 | 418635.5 ±4782.5 | worse than rc4 |
| wl ref us | 3.8 ±0.3 | 3.1 ±0.1 | 2.7 ±0.3 |  |
| wl gs_pass us | 95.1 ±0.8 | 104.7 ±3.3 | 2.8 ±0.0 |  |
| wl gs_layer us | 110.0 ±8.7 | 98.6 ±0.6 | 64.9 ±0.2 |  |
| wl xfb_vs us | 140.0 ±5.3 | 203.8 ±4.2 | 203.7 ±18.5 | worse than rc4 |
| wl xfb_gs us | 504.6 ±52.2 | 554.8 ±32.4 | 186.7 ±18.4 |  |
| wl strip_ref us | 9.1 ±0.5 | 7.3 ±0.1 | 5.2 ±0.5 |  |
| wl strip_gs us | 294.2 ±1.0 | 400.0 ±70.7 | 5.2 ±0.1 |  |
| wl cond_true us | 3.6 ±0.1 | 3.1 ±0.1 | 3.0 ±0.4 |  |
| wl cond_false us | 3.5 ±0.4 | 2.9 ±0.0 | 2.3 ±0.1 |  |
| wl pg_query us | 4.2 ±0.3 | 5.0 ±1.0 | 2.3 ±0.2 |  |
| wl ts_ref us | 371.4 ±22.9 | 395.0 ±57.5 | 262.0 ±27.6 |  |
| wl ts_pair us | 574.2 ±76.4 | 443.6 ±26.2 | 368.1 ±7.6 |  |
| wl msaa_ref us | 82.9 ±2.1 | 71.2 ±14.0 | 51.0 ±3.9 |  |
| wl msaa_a2one us | 263.2 ±4.3 | 259.1 ±2.8 | 258.6 ±1.7 |  |
| wl msaa_sshade us | 260.5 ±10.3 | 244.7 ±4.3 | 246.6 ±1.9 |  |
| wl tex_rgba us | 77.6 ±9.9 | 85.1 ±15.0 | 56.5 ±0.1 |  |
| wl tex_bgra us | 78.4 ±4.8 | 68.6 ±6.4 | 59.3 ±1.2 |  |
| PSS max MB | 223.3 ±5.1 | 209.3 ±1.2 | 207.8 ±1.2 |  |
| Graphics max MB | 88.5 ±8.3 | 64.5 ±0.0 | 64.7 ±0.0 |  |

FLAGS:
- workload_perf ms: worse than rc4
- tess_cond_state ms: worse than rc4 worse than 116
- blend ms: worse than 116
- shader_arith ms: worse than 116
- gs_tess_primitive_id ms: worse than 116
- swapchain_lifecycle ms: worse than 116
- wl bc_level us: worse than rc4
- wl xfb_vs us: worse than rc4
