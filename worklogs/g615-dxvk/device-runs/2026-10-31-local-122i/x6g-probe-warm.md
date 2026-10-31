| metric | rc4 | 116 | 122g | flag |
|---|---|---|---|---|
| gpu_prerast_slice ms | 87.0 | 117.0 | 87.0 |  |
| clip_cull ms | 1157.0 | 1091.0 | 1084.0 |  |
| multi_viewport ms | 1094.0 | 1054.0 | 1062.0 |  |
| fill_mode ms | 1293.0 | 1175.0 | 1181.0 |  |
| bc_decode ms | 455.0 | 91.0 | 111.0 | worse than 116 |
| bc_perf ms | 25698.0 | 24331.0 | 24460.0 |  |
| tess_perf ms | 28263.0 | 18226.0 | 17790.0 |  |
| workload_perf ms | 8056.0 | 12309.0 | 7532.0 |  |
| geometry ms | 1407.0 | 1162.0 | 1175.0 |  |
| tessellation ms | 1745.0 | 1348.0 | 1358.0 |  |
| xfb ms | 253.0 | 130.0 | 151.0 | worse than 116 |
| pipeline_stats ms | 1141.0 | 1079.0 | 1110.0 |  |
| vertex_stores ms | 111.0 | 110.0 | 111.0 |  |
| gs_viewport_depth ms | 139.0 | 184.0 | 141.0 |  |
| vs_viewport_index ms | 194.0 | 121.0 | 130.0 | worse than 116 |
| depth_bounds ms | 90.0 | 90.0 | 80.0 |  |
| large_draw ms | 4594.0 | 4411.0 | 4407.0 |  |
| vmr_secondary ms | 70.0 | 40.0 | 50.0 | worse than 116 |
| tess_cond_state ms | 142.0 | 112.0 | 114.0 |  |
| bachata_reqs ms | 116.0 | 67.0 | 61.0 |  |
| bachata_exec ms | 66.0 | 96.0 | 81.0 | worse than rc4 |
| robustness2 ms | 76.0 | 92.0 | 71.0 |  |
| robust_image_access2 ms | 400.0 | 383.0 | 400.0 |  |
| blend ms | 70.0 | 61.0 | 70.0 | worse than 116 |
| occlusion_query ms | 80.0 | 62.0 | 68.0 | worse than 116 |
| descriptor_model ms | 75.0 | 71.0 | 64.0 |  |
| shader_arith ms | 127.0 | 102.0 | 112.0 | worse than 116 |
| depth_stencil ms | 114.0 | 112.0 | 116.0 |  |
| sampler ms | 92.0 | 107.0 | 86.0 |  |
| draw_params ms | 101.0 | 108.0 | 123.0 | worse than rc4 worse than 116 |
| submit_stress ms | 30072.0 | 30072.0 | 30068.0 |  |
| csf_event ms | 221.0 | 221.0 | 201.0 |  |
| gs_tess_primitive_id ms | 181.0 | 117.0 | 130.0 | worse than 116 |
| bachata_storage_fmtless ms | 78.0 | 60.0 | 75.0 | worse than 116 |
| bachata_dynamic_render ms | 68.0 | 49.0 | 66.0 | worse than 116 |
| dxvk_reqs ms | 57.0 | 85.0 | 50.0 |  |
| vkd3d_reqs ms | 67.0 | 46.0 | 83.0 | worse than rc4 worse than 116 |
| d3d_fl ms | 57.0 | 78.0 | 87.0 | worse than rc4 worse than 116 |
| vkd3d_heap ms | 231.0 | 221.0 | 208.0 |  |
| vkd3d_timeline ms | 201.0 | 189.0 | 186.0 |  |
| swapchain_lifecycle ms | 8890.0 | 8952.0 | 8937.0 |  |
| wl bc_level us | 43930.0 | 767130.0 | 33361.0 |  |
| wl ref us | 1.4 | 1.1 | 1.0 |  |
| wl gs_pass us | 48.2 | 53.7 | 1.1 |  |
| wl gs_layer us | 51.0 | 56.0 | 35.9 |  |
| wl xfb_vs us | 68.8 | 68.8 | 52.2 |  |
| wl xfb_gs us | 111.8 | 111.3 | 93.7 |  |
| wl strip_ref us | 1.9 | 1.8 | 1.6 |  |
| wl strip_gs us | 157.0 | 163.2 | 1.6 |  |
| wl cond_true us | 1.3 | 1.0 | 1.0 |  |
| wl cond_false us | 0.8 | 0.7 | 0.6 |  |
| wl pg_query us | 1.4 | 1.2 | 1.0 |  |
| wl ts_ref us | 134.3 | 155.4 | 113.6 |  |
| wl ts_pair us | 194.6 | 194.5 | 141.7 |  |
| wl msaa_ref us | 15.7 | 20.5 | 15.8 |  |
| wl msaa_a2one us | 145.9 | 145.3 | 142.9 |  |
| wl msaa_sshade us | 139.6 | 141.5 | 136.8 |  |
| wl tex_rgba us | 16.4 | 17.5 | 15.4 |  |
| wl tex_bgra us | 18.8 | 21.2 | 15.5 |  |
| PSS max MB | 201.0 | 192.7 | 183.9 |  |
| Graphics max MB | 96.5 | 64.3 | 64.3 |  |

FLAGS:
- bc_decode ms: worse than 116
- xfb ms: worse than 116
- vs_viewport_index ms: worse than 116
- vmr_secondary ms: worse than 116
- bachata_exec ms: worse than rc4
- blend ms: worse than 116
- occlusion_query ms: worse than 116
- shader_arith ms: worse than 116
- draw_params ms: worse than rc4 worse than 116
- gs_tess_primitive_id ms: worse than 116
- bachata_storage_fmtless ms: worse than 116
- bachata_dynamic_render ms: worse than 116
- vkd3d_reqs ms: worse than rc4 worse than 116
- d3d_fl ms: worse than rc4 worse than 116
