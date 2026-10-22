# FTL run: Galaxy S9 (starlte, API 29), RC5 only, PanProbe v7N, game-loop scenario 2

- Project: phonefinder-hub; matrix matrix-3k3zzap4w7x4x; outcome Passed (harness level), test time 313 s
- Bucket: gs://test-lab-hrb456s6qt79m-kz4jkut67musi/panvk-v6-starlte-v7N/
- Console: https://console.firebase.google.com/project/phonefinder-hub/testlab/histories/bh.803f29bfb04d2984/matrices/5952454857278691520
- Driver: `PanVK-kbase RC5-dev-v7N (Mesa 26.3.0-devel (git-c68c685980))`
- gpu_id 0x62210010, variant 0, arch v6, Mali-G72 (Exynos 9810)
- Files: panvk-v6-starlte-v7N/starlte-29-en-portrait/{artifacts (autorun.txt, runs/20261022-000307/), logcat, game_loop_results, video.mp4}. The APK was not kept.

## Totals
`AUTORUN DONE pass=38 fail=0 skip=0 total=38 (38 pass / 0 fail / 0 skip)`

v7M was 35/38 (3 fail), so net +3, a full pass. Compliance (DXVK items=94, Bachata S4 items=114, vkd3d items=100) all PASS, missingHard=[].

## Previously failing tests
| test | v7M | v7N |
|---|---|---|
| shader_arith | FAIL (subgroup_size_control DEVICE_LOST, atom 9 event=0x51) | PASS, 8/8 cases incl. subgroup_size_control and zero_init_wg |
| sampler | FAIL (gather_offsets, atom 46 event=0x51) | PASS, 10/10 cases incl. `gather_offsets all components exact` |
| vkd3d_heap | FAIL (image_heap, atom 5 event=0x51) | PASS, ssbo_heap, texel_heap, image_heap, mutable_heap |

Other autorun lines of note:
- `RESULT shader_arith PASS mismatch=0 fps=0 ms=4248`
- `RESULT sampler PASS mismatch=0 fps=0 ms=4274`
- `RESULT vkd3d_heap PASS mismatch=0 fps=0 ms=10687`
- `RESULT robust_image_access2 PASS ... ria2=34.46ms ria1=33.39ms delta=+3.2%`
- `RESULT swapchain_lifecycle PASS ... fps=59.5` (resized FPS 60.1)
- vertex_stores PASS (kept from v7M).

All 38 tests PASS.

## FAIL lines
None at the test level (fail=0).

## SKIP lines
Test-level skip=0. Three per-case SKIP lines inside passing tests (capability-based, same as expected-unsupported features):
- large_draw: `SKIP fan chunk loop cases: PANVK_FAN_CHUNKS unset`
- large_draw: `SKIP indirect_count: drawIndirectCount not exposed`
- depth_stencil: `SKIP case d16s8_all format unsupported`

No `FAIL case` lines in any per-case log under runs/20261022-000307/.

## Logcat (logcat, 64123 lines)
- `event=0x51`: 0 occurrences
- `DEVICE_LOST`: 0 occurrences
- `MESA: error` / kbase atom failed: 0
- GPU/job/data/MMU/page faults: none (the only "fault" hits are `default` strings); SIGSEGV / fatal signal: 0
- One linker E line remains (same as v7M): libvulkan_panfrost_rc5.so needs /system/lib64/libvndksupport.so, "not accessible for the namespace classloader-namespace", followed by `[P0A-V19-FULLPLANE] mapper load failed (1 candidate names, first=mediatek)` (swapchain/gralloc path; swapchain_lifecycle still PASS).

## Screenshots
No stills from Test Lab. ffmpeg unavailable, so no frames extracted. Video (1.6 MB): [video.mp4](panvk-v6-starlte-v7N/starlte-29-en-portrait/video.mp4).
