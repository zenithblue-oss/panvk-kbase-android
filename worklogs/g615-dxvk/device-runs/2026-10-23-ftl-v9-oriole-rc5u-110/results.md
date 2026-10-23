# FTL oriole (Pixel 6, v9, Mali-G78) RC5-universal-110

AUTORUN DONE pass=37 fail=2 skip=0. Driver PanVK-kbase RC5-universal-110 (git-6cae57f893), so 5f217f70. gpu_id 0x92020010 arch v9 model Mali-G78 (JM), API 33. First RC5 run on v9 hardware.

Failures:
- tess_perf: `FAIL Wait r=-4 line=253` after gs=1 L=4; kbase `JM atom 25/26/27 failed: event=0x60` (core_req 0x16 / 0x1) at 04:04:14.386. gs=0 configs and gs=1 L=1 ran with hashes matching other GPUs.
- large_draw: base_inst_direct / base_inst_windows / base_inst_fan_windows write wrong data (bad=174464 / 140002 / 449997); fan_indirect and fan_indexed_indirect write nothing (written=0).

Raw files in raw/ (the uploaded APK is not kept). Diagnosis below.

Screenshots (frames from the FTL video): ![start](screenshots/video-start.png) ![mid](screenshots/video-mid.png) ![end](screenshots/video-end.png)

## Diagnosis (2026-10-23)

Both failures are v9 JM only. The v6/v7/v10/v11 runs of the same .so pass 39/39 and use other code paths. Neither failure is a regression from rc3: tess_perf was added on 2026-10-22, and the large_draw base_inst/fan cases on 2026-10-16. The G77/G78 38/38 rc3 runs (PROGRESS.md, RC3 findings) never ran them. Fixes are in RC5-universal-111 (jm-v7-wip 0090-0092), and none is device-tested yet.

### tess_perf: tiler heap OUT_OF_MEMORY (fixed by 0090)

- The fault is on the first frame of **gs=1 L=16**, which `PanProbeTess: MEM recorded opt=default gs=1 L=16` logged at 04:04:12.288, not in L=4 (which finished, hash cec55885 matches). 2.1 s later:
  `kbase: JM atom 25 failed: event=0x60 core_req=0x16`, `atom 26 ... core_req=0x1`, `atom 27 ... core_req=0x16`, then `FAIL Wait r=-4 line=253` (VK_ERROR_DEVICE_LOST).
- In the kbase JM event codes, event 0x60 is `BASE_JD_EVENT_OUT_OF_MEMORY`, meaning the tiler heap is exhausted. In the same table, 0x59 is TILE_RANGE_FAULT (`mali_kbase_csf_registers.h:810` in the tree). The core_req bits come from `kbase_kmod.h:27-31`: 0x16 = CS|T|V, the vertex/tiler/compute atom of a batch (`jm/panvk_vX_gpu_queue.c:178`), and 0x1 = FS, its fragment atom. Atom 25 faulted. Atom 26 (its fragment job, DATA dependency on 25) and atom 27 (the next batch's vertex/tiler chain, ordered after `last_vtc_atom`) failed with it.
- Cause: on Valhall JM, MALLOC_VERTEX jobs take every vertex packet (16 B position plus the varyings, `jm/panvk_vX_cmd_draw.c:4783`) from the tiler heap, next to the polygon lists, and JM has no incremental rendering. The v9 heaps were 128 MiB each (`panvk_vX_device.c`, jm-v9 023). gs=1 L=16 is 256 draws x 16 quad patches at fractional_odd 16 (17x17x2 = 578 triangles per patch), about 2.4M triangles per frame. The pass-through GS emits 3 unshared vertices per triangle, so there are about 7M vertex packets (32 B each plus 16 B of varyings), several hundred MiB per frame. gs=0 L=16 shades the shared tessellator vertices (about 1.3M) and fits, as do gs=1 L=1 and L=4 (1/64 and 1/16 of that). v6/v7 keep varyings outside the heap. v10+ have the CSF chunked heap with OOM handling.
- Ruled out: 0078 (kbase grow step is `pan_arch < 9`) and 0079 (`PAN_ARCH < 9`) are not active on v9. 0080 is active on v9 (the log shows `v9 batch TLS 94208 KiB ... shared per half`), but same-half batches are ordered for every JM arch in `gpu_queue_submit`, and a TLS problem would show as a fault or TLS exception, not 0x60. 0084 (warm-up) and 0087 (event batching) only touch device open and event reads. The three events are real kernel events.
- Fix, jm-v7-wip **0090** (jm-v7 117, v9 only): each v9 heap is 512 MiB. They stay GROW_ON_GPF, committed in 2 MiB steps, so the size is a VA reservation and a ceiling, not memory. `PANVK_V9_TILER_HEAP_MB` sets the size (64-1024, 128 = old), and one info line `panvk: v9 tiler heaps 2 x 512 MiB` logs it. The hierarchy mask is unchanged (the bin pointers are far below 128 MiB). Caveat: this raises the ceiling about 4x; a heavier per-batch load (more GS-expanded geometry in one render pass) can still exhaust it. A real fix needs a per-batch heap budget split, which the CPU cannot size for GPU-generated geometry.

### large_draw: CPU split base instance and indirect fans (fixed by 0091 and 0092)

- base_inst_direct / base_inst_windows / base_inst_fan_windows: `rec 65536: vtx 7 inst 16387 attr 16387 base+1 16388, want attr 5464 base+1 4` and `rec 70001: vtx 7 inst 4 attr 4 base+1 5, want attr 3 base+1 4`. gl_InstanceIndex is right, but gl_BaseInstance and the divisor-3 attribute are off by the chunk's instance offset. `v9_prerast_draw()` (instance ranges and per-instance windows) and `v9_prerast_split_fan()` drew each chunk with `instance.base = firstInstance + offset`, which is the bug csf-v11 191 fixed on CSF only. The v9 lowered VS gets its instance ID as invocation + `params.chunk_instance` (`panvk_vX_shader.c:927`), but `v9_prerast_fill_params()` left it at 0 for direct draws. base_inst_indirect passed because the GPU chunk loop already sets `chunk_instance`.
  Fix, **0091** (jm-v7 118, v9 only): the chunks keep the API `instance.base`, and `v9_prerast_draw_split()` passes the offset as `params.chunk_instance` through `cmdbuf->state.gpu_prerast_inst_base`, as csf-v11 191 does. `PANVK_V9_SPLIT_BASE_INST=0` = old.
- fan_indirect / fan_indexed_indirect: `0 of 149999 triangles captured`, `primitives generated 149999, want 0`. v10 gives `65534 of 149999` with generated=65534. Fans are not chunkable on v9, so an indirect fan over the arena goes to `panlib_v9_prerast_indirect()`, which turns a draw over the record capacity into NULL jobs (`draw_helper.cl:2437-2440`). `v9_count_draw_queries()` had already counted the full API draw.
  Fix, **0092** (jm-v7 119, v9 only): for an indirect fan lowered through gpu_prerast, a new `panlib_v9_fan_clamp_cmd` (`fan_chunks.cl`, `PAN_ARCH == 9`) copies the indirect command with vertices x instances clamped to the capacity (as csf-v11 195 `panlib_fan_clamp_draw` does on CSF), and the queries and the lowering read the copy. `PANVK_V9_FAN_CLAMP=0` = old.

Expected on the next v9 run: 39/39, the `v9 tiler heaps 2 x 512 MiB` line, tess_perf gs=1 L=16 / L=mix hashes equal to the other GPUs (d4463b35 / b34a251c), large_draw `65534 of 149999` / `65534 of 139999`, and all base_inst cases bad=0.
