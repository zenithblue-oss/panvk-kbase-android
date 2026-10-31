| metric | rc4 | 116 | 122g | flag |
|---|---|---|---|---|
| gpu_prerast_slice ms | 93.0 | 115.0 | 103.0 | worse than rc4 |
| clip_cull ms | 1112.0 | 1127.0 | 1091.0 |  |
| multi_viewport ms | 1060.0 | 1076.0 | 1070.0 |  |
| fill_mode ms | 1304.0 | 1215.0 | 1178.0 |  |
| bc_decode ms | 464.0 | 5027.0 | 540.0 | worse than rc4 |
| bc_perf ms | 24927.0 | 28990.0 | 24751.0 |  |
| tess_perf ms | 28368.0 | 18179.0 | 17826.0 |  |
| workload_perf ms | 8098.0 | 12247.0 | 7596.0 |  |
| geometry ms | 1376.0 | 1282.0 | 1265.0 |  |
| tessellation ms | 1749.0 | 1492.0 | 1482.0 |  |
| xfb ms | 282.0 | 170.0 | 161.0 |  |
| pipeline_stats ms | 1130.0 | 1113.0 | 1094.0 |  |
| vertex_stores ms | 90.0 | 100.0 | 99.0 | worse than rc4 |
| gs_viewport_depth ms | 141.0 | 135.0 | 129.0 |  |
| vs_viewport_index ms | 189.0 | 168.0 | 159.0 |  |
| depth_bounds ms | 90.0 | 98.0 | 87.0 |  |
| large_draw ms | 4557.0 | 4449.0 | 4508.0 |  |
| vmr_secondary ms | 70.0 | 70.0 | 60.0 |  |
| tess_cond_state ms | 143.0 | 133.0 | 141.0 | worse than 116 |
| bachata_reqs ms | 126.0 | 66.0 | 59.0 |  |
| bachata_exec ms | 71.0 | 66.0 | 67.0 |  |
| robustness2 ms | 69.0 | 62.0 | 103.0 | worse than rc4 worse than 116 |
| robust_image_access2 ms | 384.0 | 416.0 | 379.0 |  |
| blend ms | 81.0 | 70.0 | 75.0 | worse than 116 |
| occlusion_query ms | 70.0 | 70.0 | 69.0 |  |
| descriptor_model ms | 84.0 | 60.0 | 71.0 | worse than 116 |
| shader_arith ms | 83.0 | 121.0 | 119.0 | worse than rc4 |
| depth_stencil ms | 119.0 | 118.0 | 74.0 |  |
| sampler ms | 86.0 | 124.0 | 120.0 | worse than rc4 |
| draw_params ms | 113.0 | 91.0 | 79.0 |  |
| submit_stress ms | 30074.0 | 30085.0 | 30063.0 |  |
| csf_event ms | 233.0 | 221.0 | 190.0 |  |
| gs_tess_primitive_id ms | 202.0 | 155.0 | 134.0 |  |
| bachata_storage_fmtless ms | 77.0 | 50.0 | 70.0 | worse than 116 |
| bachata_dynamic_render ms | 67.0 | 60.0 | 64.0 | worse than 116 |
| dxvk_reqs ms | 55.0 | 61.0 | 57.0 |  |
| vkd3d_reqs ms | 87.0 | 87.0 | 84.0 |  |
| d3d_fl ms | 67.0 | 78.0 | 72.0 | worse than rc4 |
| vkd3d_heap ms | 230.0 | 188.0 | 180.0 |  |
| vkd3d_timeline ms | 219.0 | 197.0 | 190.0 |  |
| swapchain_lifecycle ms | 8872.0 | 5079.0 | 4920.0 |  |
| wl bc_level us | 41951.0 | 743182.0 | 35245.0 |  |
| wl ref us | 1.2 | 1.1 | 1.1 |  |
| wl gs_pass us | 45.0 | 50.6 | 1.0 |  |
| wl gs_layer us | 52.0 | 54.8 | 35.7 |  |
| wl xfb_vs us | 67.4 | 70.5 | 53.8 |  |
| wl xfb_gs us | 111.1 | 111.0 | 93.4 |  |
| wl strip_ref us | 2.0 | 1.7 | 1.6 |  |
| wl strip_gs us | 155.8 | 164.1 | 1.6 |  |
| wl cond_true us | 1.2 | 1.0 | 1.0 |  |
| wl cond_false us | 0.9 | 0.7 | 0.6 |  |
| wl pg_query us | 1.5 | 1.3 | 1.0 |  |
| wl ts_ref us | 151.6 | 148.3 | 104.7 |  |
| wl ts_pair us | 202.1 | 200.5 | 149.4 |  |
| wl msaa_ref us | 16.1 | 17.9 | 15.2 |  |
| wl msaa_a2one us | 143.2 | 143.3 | 153.6 | worse than rc4 worse than 116 |
| wl msaa_sshade us | 138.4 | 139.8 | 138.2 |  |
| wl tex_rgba us | 15.8 | 18.5 | 14.5 |  |
| wl tex_bgra us | 15.7 | 19.6 | 15.5 |  |
| PSS max MB | 200.9 | 182.7 | 187.2 |  |
| Graphics max MB | 96.5 | 50.9 | 64.5 | worse than 116 |

FLAGS:
- gpu_prerast_slice ms: worse than rc4
- bc_decode ms: worse than rc4
- vertex_stores ms: worse than rc4
- tess_cond_state ms: worse than 116
- robustness2 ms: worse than rc4 worse than 116
- blend ms: worse than 116
- descriptor_model ms: worse than 116
- shader_arith ms: worse than rc4
- sampler ms: worse than rc4
- bachata_storage_fmtless ms: worse than 116
- bachata_dynamic_render ms: worse than 116
- d3d_fl ms: worse than rc4
- wl msaa_a2one us: worse than rc4 worse than 116
- Graphics max MB: worse than 116
