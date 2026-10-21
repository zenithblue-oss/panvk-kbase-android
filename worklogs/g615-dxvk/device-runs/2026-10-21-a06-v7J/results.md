# A06 (Mali-G52) v7J results, 2026-10-21

Package dev.zenithblue.panvktest.v7j, driver rc5 (PanVK-kbase RC5-dev-v7J), so sha256 c8ea84592f13600b1a22ba15e682c8c6012e516d320e19da44faca53ff9d9dcf (verified).

## Total
autorun all: 37 pass / 0 fail / 1 skip (total 38). v7I was 35 pass / 2 fail: robustness2 and vkd3d_heap, both now PASS.
Only SKIP: swapchain_lifecycle (same as before; skip, not a failure).

Targeted run (robustness2,vkd3d_heap,robust_image_access2,vkd3d_reqs,dxvk_reqs): 5/5 PASS, run=20261021-194856. Step 3 (vkd3d_heap rerun with PANVK_TEX_WATERFALL=0) not needed.

## Failure lines
None.

## Perf (fps / ms per test, autorun all)
Only tests with fps>0: clip_cull 2425.3 fps, multi_viewport 1580.8, fill_mode 1702.6, geometry 1861.5, tessellation 1532.7, pipeline_stats 617.6.
robust_image_access2: ria2=236.55ms ria1=175.86ms delta=+34.5% (targeted run 235.94/171.31, +37.7%).

Per-test status / ms:

- gpu_prerast_slice PASS mismatch=0 fps=0 ms=4271
- clip_cull PASS mismatch=0 fps=2425.3 ms=5443
- multi_viewport PASS mismatch=0 fps=1580.8 ms=5222
- fill_mode PASS mismatch=0 fps=1702.6 ms=8528
- bc_decode PASS mismatch=0 fps=0 ms=6736
- bc_perf PASS mismatch=0 fps=0 ms=62389
- geometry PASS mismatch=0 fps=1861.5 ms=8279
- tessellation PASS mismatch=0 fps=1532.7 ms=12850
- xfb PASS mismatch=0 fps=0 ms=7139
- pipeline_stats PASS mismatch=0 fps=617.6 ms=5564
- vertex_stores PASS mismatch=0 fps=0 ms=4277
- gs_viewport_depth PASS mismatch=0 fps=0 ms=4321
- vs_viewport_index PASS mismatch=0 fps=0 ms=5422
- depth_bounds PASS mismatch=0 fps=0 ms=4559
- large_draw PASS mismatch=0 fps=0 ms=22388
- vmr_secondary PASS mismatch=0 fps=0 ms=4188
- tess_cond_state PASS mismatch=0 fps=0 ms=4455
- bachata_reqs PASS mismatch=0 fps=0 ms=4161
- bachata_exec PASS mismatch=0 fps=0 ms=4157
- robustness2 PASS mismatch=0 fps=0 ms=4181
- robust_image_access2 PASS mismatch=0 fps=0 ms=8764 extra=4096x4096x16 rgba8 loads ria2=236.55ms ria1=175.86ms delta=+34.5% stats(ria2/ria1): Instructions=409/210 Tuples=325/161 Clauses=75/28 Preloads=0/0 Threads=2/2 Code size=4576/2272 Loops=0/0 Spills=0/0 Fills=0/0 Spill cost=0/0 Registers used=18/13 Uniforms used=6/6
- blend PASS mismatch=0 fps=0 ms=4342
- occlusion_query PASS mismatch=0 fps=0 ms=4285
- descriptor_model PASS mismatch=0 fps=0 ms=4291
- shader_arith PASS mismatch=0 fps=0 ms=4266
- depth_stencil PASS mismatch=0 fps=0 ms=4247
- sampler PASS mismatch=0 fps=0 ms=4253
- draw_params PASS mismatch=0 fps=0 ms=4200
- submit_stress PASS mismatch=0 fps=0 ms=34140
- csf_event PASS mismatch=0 fps=0 ms=4664
- gs_tess_primitive_id PASS mismatch=0 fps=0 ms=6160
- bachata_storage_fmtless PASS mismatch=0 fps=0 ms=4277
- bachata_dynamic_render PASS mismatch=0 fps=0 ms=4143
- dxvk_reqs PASS mismatch=0 fps=0 ms=2085
- vkd3d_reqs PASS mismatch=0 fps=0 ms=2092
- vkd3d_heap PASS mismatch=0 fps=0 ms=10490
- vkd3d_timeline PASS mismatch=0 fps=0 ms=4249
- swapchain_lifecycle SKIP mismatch=0 fps=0 ms=5043
