# FTL run: Galaxy S9 (starlte, API 29), RC5 only, game-loop scenario 2

- Project: phonefinder-hub; matrix matrix-2aw7f2bu1e85n; outcome Passed (harness level), test time 303 s
- Bucket: gs://test-lab-hrb456s6qt79m-kz4jkut67musi/panvk-v6-starlte-1/
- Console: https://console.firebase.google.com/project/phonefinder-hub/testlab/histories/bh.a3fc709fae5a67f/matrices/7838356796179209085
- Driver: PanVK-kbase RC5-dev-v7K (Mesa 26.3.0-devel git-d160802965)
- gpu_id 0x62210010, arch v6, Mali-G72 (Exynos 9810)
- Files: panvk-v6-starlte-1/starlte-29-en-portrait/{artifacts (autorun.txt, runs/), logcat, game_loop_results, video.mp4}

## Totals
AUTORUN DONE pass=33 fail=5 skip=0 total=38. Compliance (DXVK, Bachata S4, vkd3d) all PASS, missingHard=[].

## FAIL lines
- vertex_stores: Bifrost (v7): feature not implemented yet
- robust_image_access2: not implemented yet; 10 failed cases (r32ui.tbuf_oob, r32ui.tbuf_alias65537, r32ui_nofmt.load_x/load_y/load_layer/load_neg_x/load_x_alias65537/load_layer_alias65536/tbuf_oob/tbuf_alias65537), all got alpha 0, want 1
- shader_arith: not implemented yet (logcat: case subgroup_size_control DEVICE_LOST)
- sampler: not implemented yet
- vkd3d_heap: not implemented yet
SKIP: none.

## Logcat
- dlopen failures: none
- GPU/data/job faults, fatal signals: none
- DEVICE_LOST: only in shader_arith case subgroup_size_control (2 lines, TAIL + JSON echo)
- atom stride: every process logs "kbase: Bifrost JM atom strides 56 and 64 refused, stride 48 (base_jd_atom_v2 without renderpass_id)" (informational, driver falls back to 48)
