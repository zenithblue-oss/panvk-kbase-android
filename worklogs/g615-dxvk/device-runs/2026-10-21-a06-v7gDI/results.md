# A06 (Mali-G52 v7) runs 2026-10-21: v7gDI and v7I

Device drops (RTL) interrupted the runs several times. v7H perf A/B was NOT run.

## v7gDI (rc5, driverInfo RC5-dev-v7G-DI)
- APK rc5 .so sha256 (stripped copy in APK): e166fe762316b4e54f7a90b6845c23af996b14ca1012400d7691df4b8ce802a6; embedded manifest records 259b6ff9...
- Part 1 (`autorun all`): 27 tests completed before the run stalled after `sampler`: 24 PASS / 3 FAIL.
  - fill_mode FAIL mismatch=2: "Bifrost (v7): feature not implemented yet" (FILL_MODE_FAILS=1)
  - vertex_stores FAIL: "Bifrost (v7): feature not implemented yet" (VERTEX_STORES_FAILS=6)
  - robust_image_access2 FAIL: "Bifrost (v7): feature not implemented yet" (ria2=231.80ms ria1=169.13ms)
- Stalled test: next test after `sampler` in the all-order (name unknown; the app never logged a result). Two resume attempts with the remaining 10-11 tests as a comma list produced no result for 10+ minutes (compliance probes only).
- Not completed on v7gDI: bachata_dynamic_render, bachata_storage_fmtless, csf_event, draw_params, dxvk_reqs, gs_tess_primitive_id, submit_stress, vkd3d_heap, vkd3d_reqs, vkd3d_timeline, vkinfo (all but vkinfo later PASSED on v7I, except vkd3d_heap).
- Files: pulled/part1_autorun.txt, pulled/logcat_PanProbe_MESA.txt.

## v7I (rc5 sha e191f62ef001456cc2793d9568ca57bd0a594d9f0da194697d472418f14ba151, verified)
- 4-test check (fill_mode, vertex_stores, robust_image_access2, draw_params): 4/0/0, all PASS.
- Full `all`: 37 results = 35 PASS / 2 FAIL / 0 skip (device dropped after vkd3d_timeline; vkinfo not reported).
  - robustness2 FAIL: "Bifrost (v7): feature not implemented yet" (PASS on v7gDI; regression in v7I)
  - vkd3d_heap FAIL: "Bifrost (v7): feature not implemented yet" ms=10298 (PANVK_DI_TEX_MAX=65535 rerun not done)
- Files: ../2026-10-21-a06-v7I/pulled-4tests, ../2026-10-21-a06-v7I/pulled-all.

## Comparison
Baselines quoted by coordinator: v7G 29/8/1, RC4 14/19/5, v7E 16/17/5. v7I 35/2/0 (37 results) is the best so far; per-test baseline tables were not available to this agent.

## Perf (v7I default, ms / fps from autorun)
pipeline_stats 1377.9 fps; geometry 1963.9 fps; tessellation 1925.2 fps; large_draw 21592 ms; bc_perf 65334 ms.
(v7gDI part 1: pipeline_stats 1145.2 fps; geometry 2739.0; tessellation 2560.6; large_draw 20772 ms; bc_perf 61825 ms.)
v7H A/B (PANVK_V7_IDVS=0, PANVK_V7_GS_DESC=1): not run.
