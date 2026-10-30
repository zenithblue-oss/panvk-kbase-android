| metric | rc4 | 116 | 122e | flag |
|---|---|---|---|---|
| gpu_prerast_slice ms | 86.5 ±15.5 | 82.0 ±9.0 | 75.5 ±5.5 |  |
| clip_cull ms | 1284.5 ±10.5 | 1172.0 ±6.0 | 1168.5 ±9.5 |  |
| multi_viewport ms | 1153.5 ±18.5 | 1131.0 ±17.0 | 1126.5 ±4.5 |  |
| fill_mode ms | 1755.0 ±19.0 | 1504.5 ±20.5 | 1458.0 ±39.0 |  |
| bc_decode ms | 506.0 ±22.0 | 178.5 ±8.5 | 129.5 ±28.5 |  |
| bc_perf ms | 28759.5 ±110.5 | 28094.5 ±142.5 | 27996.0 ±546.0 |  |
| tess_perf ms | 48953.5 ±15.5 | 28977.5 ±242.5 | 27389.0 ±237.0 |  |
| workload_perf ms | 10033.0 ±117.0 | 14320.0 ±145.0 | 10880.5 ±46.5 | worse than rc4 |
| geometry ms | 1865.0 ±79.0 | 1425.0 ±11.0 | 1402.0 ±32.0 |  |
| tessellation ms | 2783.5 ±22.5 | 1963.0 ±11.0 | 1979.5 ±58.5 |  |
| xfb ms | 331.5 ±58.5 | 173.0 ±21.0 | 175.5 ±19.5 |  |
| pipeline_stats ms | 1298.0 ±20.0 | 1202.5 ±2.5 | 1193.0 ±3.0 |  |
| vertex_stores ms | 148.5 ±5.5 | 124.5 ±8.5 | 152.5 ±21.5 |  |
| gs_viewport_depth ms | 166.5 ±3.5 | 202.5 ±0.5 | 150.0 ±7.0 |  |
| vs_viewport_index ms | 209.0 ±14.0 | 159.0 ±10.0 | 144.5 ±3.5 |  |
| depth_bounds ms | 155.0 ±18.0 | 113.5 ±10.5 | 120.0 ±20.0 |  |
| large_draw ms | 8729.0 ±136.0 | 8070.5 ±121.5 | 7821.5 ±185.5 |  |
| vmr_secondary ms | 124.5 ±0.5 | 95.0 ±2.0 | 113.5 ±14.5 | worse than 116 |
| tess_cond_state ms | 171.5 ±4.5 | 157.5 ±19.5 | 130.5 ±5.5 |  |
| bachata_reqs ms | 114.0 ±12.0 | 77.0 ±7.0 | 56.5 ±3.5 |  |
| bachata_exec ms | 76.5 ±25.5 | 60.5 ±0.5 | 65.0 ±5.0 |  |
| robustness2 ms | 89.0 ±20.0 | 65.0 ±2.0 | 76.5 ±5.5 | worse than 116 |
| robust_image_access2 ms | 651.5 ±4.5 | 619.5 ±2.5 | 607.0 ±11.0 |  |
| blend ms | 107.5 ±8.5 | 82.5 ±0.5 | 103.0 ±10.0 | worse than 116 |
| occlusion_query ms | 90.5 ±19.5 | 99.5 ±13.5 | 102.0 ±20.0 |  |
| descriptor_model ms | 81.5 ±9.5 | 76.0 ±13.0 | 92.0 ±3.0 |  |
| shader_arith ms | 93.0 ±1.0 | 80.5 ±9.5 | 83.5 ±13.5 |  |
| depth_stencil ms | 105.0 ±0.0 | 99.5 ±10.5 | 86.0 ±5.0 |  |
| sampler ms | 97.0 ±6.0 | 74.5 ±4.5 | 80.0 ±0.0 | worse than 116 |
| draw_params ms | 117.5 ±8.5 | 85.5 ±15.5 | 90.5 ±0.5 |  |
| submit_stress ms | 30089.5 ±8.5 | 30079.5 ±4.5 | 30078.5 ±4.5 |  |
| csf_event ms | 421.0 ±21.0 | 379.5 ±5.5 | 284.5 ±9.5 |  |
| gs_tess_primitive_id ms | 235.0 ±6.0 | 175.0 ±26.0 | 157.0 ±26.0 |  |
| bachata_storage_fmtless ms | 97.0 ±4.0 | 83.5 ±7.5 | 65.5 ±5.5 |  |
| bachata_dynamic_render ms | 73.5 ±2.5 | 77.5 ±4.5 | 63.5 ±1.5 |  |
| dxvk_reqs ms | 65.5 ±0.5 | 55.5 ±0.5 | 61.5 ±8.5 |  |
| vkd3d_reqs ms | 58.0 ±1.0 | 67.0 ±15.0 | 73.5 ±7.5 | worse than rc4 |
| d3d_fl ms | 52.0 ±9.0 | 69.5 ±2.5 | 52.5 ±2.5 |  |
| vkd3d_heap ms | 242.0 ±9.0 | 238.5 ±17.5 | 204.5 ±14.5 |  |
| vkd3d_timeline ms | 260.0 ±28.0 | 246.0 ±1.0 | 246.0 ±24.0 |  |
| swapchain_lifecycle ms | 5826.5 ±661.5 | 5863.5 ±644.5 | 5399.0 ±262.0 |  |
| wl bc_level us | 46640.5 ±1457.5 | 771714.0 ±12782.0 | 450751.0 ±8889.0 | worse than rc4 |
| wl ref us | 3.2 ±0.0 | 3.7 ±0.8 | 2.5 ±0.1 |  |
| wl gs_pass us | 98.0 ±4.5 | 105.1 ±5.6 | 2.7 ±0.3 |  |
| wl gs_layer us | 98.2 ±0.3 | 104.0 ±3.4 | 73.9 ±4.9 |  |
| wl xfb_vs us | 132.7 ±4.9 | 173.1 ±2.1 | 217.7 ±1.3 | worse than rc4 worse than 116 |
| wl xfb_gs us | 547.1 ±5.9 | 566.1 ±2.8 | 198.5 ±30.2 |  |
| wl strip_ref us | 8.6 ±0.4 | 7.8 ±0.3 | 5.4 ±0.3 |  |
| wl strip_gs us | 285.2 ±7.3 | 321.3 ±2.1 | 4.2 ±0.3 |  |
| wl cond_true us | 3.1 ±0.4 | 3.0 ±0.1 | 2.9 ±0.2 |  |
| wl cond_false us | 2.3 ±0.4 | 3.0 ±0.0 | 2.3 ±0.1 |  |
| wl pg_query us | 5.3 ±0.5 | 4.7 ±0.5 | 2.7 ±0.4 |  |
| wl ts_ref us | 416.5 ±77.4 | 327.9 ±1.8 | 313.4 ±11.9 |  |
| wl ts_pair us | 482.8 ±9.7 | 464.3 ±62.2 | 334.0 ±34.1 |  |
| wl msaa_ref us | 76.5 ±15.5 | 69.1 ±5.8 | 51.4 ±0.7 |  |
| wl msaa_a2one us | 274.1 ±5.6 | 276.4 ±7.8 | 247.6 ±0.8 |  |
| wl msaa_sshade us | 248.9 ±3.6 | 252.2 ±3.9 | 267.1 ±19.4 |  |
| wl tex_rgba us | 55.5 ±4.7 | 71.8 ±1.1 | 51.3 ±2.2 |  |
| wl tex_bgra us | 74.4 ±24.3 | 73.5 ±3.8 | 53.9 ±2.5 |  |
| PSS max MB | 214.1 ±0.8 | 182.8 ±1.7 | 189.0 ±4.1 |  |
| Graphics max MB | 96.9 ±0.1 | 64.5 ±0.0 | 64.5 ±0.0 |  |

FLAGS:
- workload_perf ms: worse than rc4
- vmr_secondary ms: worse than 116
- robustness2 ms: worse than 116
- blend ms: worse than 116
- sampler ms: worse than 116
- vkd3d_reqs ms: worse than rc4
- wl bc_level us: worse than rc4
- wl xfb_vs us: worse than rc4 worse than 116
