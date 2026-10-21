# A06 v7G results (partial: device dropped)

Group A (v7G-A b1, RC5-dev-v7G-A, run 20261021-193735, A1/runs): pass 3 fail 4.

| test | v7E | v7G-A |
|---|---|---|
| occlusion_query | FAIL Wait r=-4 | PASS |
| bc_perf | FAIL Wait r=-4 | PASS |
| depth_bounds | FAIL | PASS |
| tessellation | FAIL 4 cases + Wait r=-4 | FAIL 4 cases (grid_64x48_l16, indexed_inst2 direct/indirect first1, grid_tes_to_gs_l16), no fault |
| xfb | FAIL Wait r=-4 | FAIL gs_nopos, vs_adj_tri_strip, vs_clip_cull_capture mismatch |
| pipeline_stats | FAIL fs=0 | FAIL fs=0 in all 9 cases (gs_inv=0 for non-GS ok) |
| large_draw | FAIL GPU faults | FAIL xfb written=4096 generated=-1, 4 fence r=-4 (restart cases) |

Not done: occlusion_query PANVK_QUERY_UNCACHED=1 A/B, Group E (device vanished from adb after A1; v7gE not installed). Note another agent ran v7G-B on the device concurrently.
