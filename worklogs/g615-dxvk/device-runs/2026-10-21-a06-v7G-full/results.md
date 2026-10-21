# A06 v7G full PanProbe run (RC5-dev-v7G, all A06 fix groups), 2026-10-21

- Device: Galaxy A06 SM-A065F (G52 MC2 r1, v7), adb `localhost:54520`. Logcat: `kbase: Bifrost JM atom stride 56 (base_jd_atom_v2)`, `panvk: gpu_id 0x74021000 ... arch v7`.
- APK `panprobe-v7G.apk` (sha256 `8442ffe1...06ca`), package `dev.zenithblue.panvktest.v7g`, `--es driver rc5`.
- driverInfo: `PanVK-kbase RC5-dev-v7G (Mesa 26.3.0-devel (git-121f9b16f0))`. .so sha256 `086424e4d9f3f89f99d4759f7feb2a8ecd8fb84d244d01420674e009f5703da9`. Build: `builds/2026-10-21-v7G-build.md`.

## Total: 29 pass / 8 fail / 1 skip (38 tests)

RC4 14/19/5, v7E 16/17/5. draw_params is counted as a FAIL (hang).

Runs:
- run1 `--es autorun all`: 27 results, then **draw_params hung** (started 20:55:48, no result for more than 10 min; device-init processes stopped at 21:06:30). Force-stopped. No runs/ dir was written (autorun stops before export), so only `run1/files/autorun.txt` and `run1/logcat-hang.txt` (PanProbe/MESA tags).
- run2 (run id 20261021-211057): the 10 tests after draw_params, 6 pass / 4 fail. `run2/`.
- run3 (run id 20261021-211312): xfb, vertex_stores, large_draw again to get their per-case logs (run1 had none); same results as run1. `run3/`.

## Per-test

| test | RC4 | v7E | v7G |
|---|---|---|---|
| gpu_prerast_slice | PASS | PASS | PASS |
| clip_cull | FAIL | PASS | PASS |
| multi_viewport | FAIL | PASS | PASS |
| fill_mode | FAIL | FAIL | **PASS** |
| bc_decode | PASS | PASS | PASS |
| bc_perf | FAIL | FAIL | **PASS** (65190 ms) |
| geometry | FAIL | FAIL | **PASS** |
| tessellation | FAIL | FAIL | **PASS** |
| xfb | FAIL | FAIL | FAIL (1 case) |
| pipeline_stats | FAIL | FAIL | **PASS** |
| vertex_stores | SKIP | SKIP | FAIL (newly exposed) |
| gs_viewport_depth | FAIL | FAIL | **PASS** |
| vs_viewport_index | FAIL | FAIL | **PASS** |
| depth_bounds | FAIL | FAIL | **PASS** |
| large_draw | FAIL | FAIL | FAIL (5 tess xfb cases) |
| vmr_secondary | SKIP | SKIP | **PASS** |
| tess_cond_state | FAIL | FAIL | **PASS** |
| bachata_reqs | FAIL | FAIL | **PASS** |
| bachata_exec | PASS | PASS | PASS |
| robustness2 | SKIP | FAIL | **PASS** |
| robust_image_access2 | FAIL | SKIP | SKIP (not advertised) |
| blend | PASS | PASS | PASS |
| occlusion_query | FAIL | FAIL | **PASS** |
| descriptor_model | PASS | PASS | PASS |
| shader_arith | PASS | PASS | PASS |
| depth_stencil | PASS | PASS | PASS |
| sampler | PASS | PASS | PASS |
| draw_params | SKIP | SKIP | FAIL: hang (newly exposed) |
| submit_stress | PASS | PASS | PASS |
| csf_event | PASS | PASS | PASS |
| gs_tess_primitive_id | SKIP | SKIP | FAIL (newly exposed) |
| bachata_storage_fmtless | PASS | PASS | PASS |
| bachata_dynamic_render | PASS | PASS | PASS |
| dxvk_reqs | FAIL | FAIL | FAIL |
| vkd3d_reqs | FAIL | FAIL | FAIL |
| vkd3d_heap | FAIL | FAIL | FAIL |
| vkd3d_timeline | PASS | PASS | PASS |
| swapchain_lifecycle | PASS | PASS | PASS (40.5 fps) |

No regressions vs v7E: every v7E PASS still passes.

## Failure lines

- **xfb** `RESULT xfb FAIL mismatch=4`: `CASE vs_clip_cull_capture mismatch=4 FAIL`, `XFB_FAILS=1` (same as A3).
- **vertex_stores** `RESULT vertex_stores FAIL` (`VERTEX_STORES_FAILS=6`): `CASE iter0..iter5 FAIL outv[0,16] inconsistent got (1000,1000) want (1000,1001) or (1017,1000) total=32`. This is different from v7G-E (`indexed_dup hit[4]`), which now passes.
- **large_draw** `RESULT large_draw FAIL` (`XFB_FAILS=5`; logcat `panvk: tessellated draw of 70000 patches, only 8192 are drawn`):
  - `FAIL case tess_direct bad=185426 bytes=262144 written=16384 generated=-1`
  - `FAIL case tess_indirect bad=185426 bytes=262144 written=16384 generated=-1`
  - `FAIL case tess_instanced bad=2 bytes=262144 written=16384 generated=-1`
  - `FAIL case tess_tri_direct bad=247234 bytes=393216 written=8192 generated=-1`
  - `FAIL case tess_tri_indirect bad=247234 bytes=393216 written=8192 generated=-1`
- **draw_params**: hang. No result line. The app was force-stopped after more than 10 min.
- **gs_tess_primitive_id** `RESULT FAIL` (unchanged from v7G-E, so 0049 did not fix it on the device):
  - `FAIL direct ids bad=24 first=16 17 18 19`
  - `FAIL direct-replay ids bad=24 first=0 1 2 3`
  - `FAIL indirect ids bad=24 first=16 17 18 19`
  - `FAIL indirect-replay ids bad=24 first=16 17 18 19`
  - `FAIL chunk ids bad=3600 first=640 641 520 521`
  - `FAIL chunk-replay ids bad=3600 first=138 139 8 9`
  - `FAIL indirect-chunk ids bad=3600 first=649 650 520 521`
- **dxvk_reqs**: `FAIL hard descriptorIndexing`, `descriptorBindingSampledImageUpdateAfterBind`, `descriptorBindingUpdateUnusedWhilePending`, `descriptorBindingPartiallyBound`.
- **vkd3d_reqs**: `FAIL hard proton: robustImageAccess2`, `proton: descriptorIndexing`, `proton: 1M per-stage UAB sampled/storage images + SSBOs (or descriptor buffer)`.
- **vkd3d_heap**: `FAIL case ssbo_heap/texel_heap/image_heap missing descriptorIndexing`.
- **robust_image_access2** SKIP: `robustImageAccess2 not supported` (needs `PANVK_DEBUG=ria2`).

Not in v7G: jm-v7-wip 0070-0072 (H3, H4, M4), which landed on main after the merge.
