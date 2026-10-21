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

## Group E (v7G-E build 3, RC5-dev-v7G-E, so sha efc78275, pkg v7ge; runs in E/runs)

Run 20261021-195352: pass 3 fail 5.

| test | v7E baseline | v7G-E |
|---|---|---|
| vmr_secondary | n/a | PASS |
| tess_cond_state | n/a | PASS |
| bachata_reqs | n/a | PASS |
| vertex_stores | n/a | FAIL: CASE indexed_dup FAIL hit[4] want 0 got 1 total=6 |
| bachata_exec | n/a | FAIL: int64_atomics counter=0x100000001000 (exp same) max=21882858378215 (exp 5095) |
| robustness2 | n/a | FAIL: null_descriptors bad image read a_img=1.0 (samp/txb alpha 0.0) |
| draw_params | n/a | FAIL: base_vertex_instance, draw_nonindexed, maintenance5, maintenance6, load_store_op_none; counts match, first_missing=(100,7,458852,1007) / (5,2,131077,1002) |
| gs_tess_primitive_id | n/a | FAIL: all 7 modes, ids bad (direct 24, chunk 3600) |

robustness2 with PANVK_NULLIMG_INBOUNDS=1 (run 20261021-195519): PASS (all cases; null_descriptors alpha sampler=0 image=0 texel_buf=0). Confirms null-image alpha cause.

robust_image_access2 with PANVK_DEBUG=ria2 (run 20261021-195645): FAIL. INFO robustImageAccess2=1. Fails: *.load_x/y/layer/neg_x (r32ui got a=0 want 1; rgba8 got a=1.0 want 0), *_alias65537/65536 loads return aliased in-bounds data, tbuf_oob/tbuf_alias (same alpha), store_oob and r32ui.atomic_cas_alias write in-bounds (got 0x9 / 0xffffffff, want unchanged). Passes: fetch*, tbuf, store, atomics in-bounds. BENCH ria2 169.24ms vs ria1 169.21ms (no perf delta).

Group A check: v7ga not installed (pm list shows only v7ge); PANVK_QUERY_UNCACHED A/B skipped.
