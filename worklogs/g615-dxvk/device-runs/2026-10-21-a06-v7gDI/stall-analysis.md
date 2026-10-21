# v7G-DI A06 stall: which test it was

Reported as "bachata_dynamic_render stalls >10 min, twice". From the pulled logs it was **draw_params**, not bachata_dynamic_render.

- Autorun runs the selected tests in suite order (`testCases.filter { it.name in wanted }`, MainActivity.kt), so a list's order doesn't matter.
- part1 (`part1_autorun.txt`) ends at `sampler` (test 27). The next test in the suite is `draw_params`. In the logcat, pid 26845 creates its device at 18:12:24 and nothing is logged until 18:21:13, when the runner restarts.
- part2 covers the 11 tests after sampler: draw_params, submit_stress, csf_event, gs_tess_primitive_id, bachata_storage_fmtless, bachata_dynamic_render, dxvk_reqs, vkd3d_reqs, vkd3d_heap, vkd3d_timeline, swapchain_lifecycle. After vkinfo and compliance, the first test process (pid 31251) creates its device at 18:21:28 and stalls the same way. That test is draw_params again.
- This is the known v7G-DI draw_params stall. Patch jm-v7-wip 0054 (jm-v7 081) explains it: jm-v7 080 changed the skipped-atomic value to `nir_imm_zero()`, which undid jm-v7 035, so every guarded VS atomic returned 0 and draw_params hung with `rec[atomicAdd(count)]` all writing to rec[0]. 0054 fixed it, and RC5-dev-v7I passes draw_params on the A06 (`../2026-10-21-a06-v7I/pulled-4tests/autorun.txt`, 4158 ms).
- bachata_dynamic_render never ran on v7G-DI. It passed on v7G-full in all 3 runs.

Why the 60 s harness timeout didn't fire: there is no TIMEOUT line, so the app itself stopped (not just the child). The likely cause is a wedged GPU blocking the app's UI/HWUI thread, which `updateTestStatus` waits on through `withContext(Dispatchers.Main)`. This is not confirmed: no kernel log was pulled.

Result: no driver patch for this. The v7I full run decides whether bachata_dynamic_render or the tests after it have a real problem.
