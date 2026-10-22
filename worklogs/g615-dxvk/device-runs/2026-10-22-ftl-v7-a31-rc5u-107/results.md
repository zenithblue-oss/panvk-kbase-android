# FTL run: Galaxy A31 (a31, Mali-G52 r1 MC2, Bifrost v7, API 31), PanProbe RC5-universal-107, game-loop scenario 2

- Project phonefinder-hub. gcloud exit 0. That only means the game loop finished; the real results are below.
- Bucket `gs://test-lab-hrb456s6qt79m-kz4jkut67musi/panvk-v7-a31-rc5u-107/`, copied to `raw/a31-31-en-portrait/`: logcat (70638 lines), video.mp4 (348 s), game_loop_results/results_scenario_2.json, artifacts/.../autorun.txt and runs/20261022-084025/ (per-test logs and JSON). The APK in the bucket was not kept; it is `/home/flux/build-out/apks/panprobe-rc5u-107.apk` (rc5 .so `9a6308c8`).
- **GPU confirmed:** `panvk: gpu_id 0x74021000 variant 0x2 arch v7 model Mali-G52 r1`, `ICD device=Mali-G52 r1 MC2`. So this unit is the MediaTek (Helio P65) variant, not a Qualcomm one. kbase is JM, uAPI 11.27 (the A06 has 11.38), atom stride 56, no ION system heap.
- Driver line: `DRIVER slot=RC5 driverInfo=PanVK-kbase RC5-universal-107 (Mesa 26.3.0-devel (git-28b0346c01))`. This is the same tree as the S9 run (0078 + 0079 + 0080).
- GAMELOOP at 08:32:42.6, AUTORUN DONE at 08:40:25.5 (463 s). Auto-upload OK, id d50c6c0f.

## Totals
`AUTORUN DONE pass=38 fail=1 skip=0 total=39 (38 pass / 1 fail / 0 skip)`. results_scenario_2.json agrees: pass 38, fail 1, skip 0.

- **FAIL: fill_mode** (`mismatch=2303`, 13 of 17 cases). This is a real driver bug, a shader-cache key collision on v7. See "Failure" below.
- The other 38 tests PASS with mismatch=0. tess_perf PASS, with no LOWMEM.
- Compliance: DXVK items=94, Bachata S4 items=114 and vkd3d items=100 all PASS, missingHard=[].
- Per-case SKIP lines inside passing tests are the same as on the S9: large_draw `PANVK_FAN_CHUNKS unset`, large_draw `drawIndirectCount not exposed`, depth_stencil `d16s8_all format unsupported`. Outside fill_mode there are no failing CASE lines. tessellation has 24/24 cases PASS.
- Logcat: 0 `DEVICE_LOST`, 0 `event=0x51`, 0 `Fatal signal`. The 36 lmkd/lowmemorykiller lines are for other processes; none of them names PanProbe or the testloop companion.

| test | result | ms | fps |
|---|---|---|---|
| gpu_prerast_slice | PASS | 4288 | |
| clip_cull | PASS | 5353 | 1867.5 |
| multi_viewport | PASS | 5146 | 2392.4 |
| **fill_mode** | **FAIL mismatch=2303** | 6250 | 2630.6 |
| bc_decode | PASS | 5225 | |
| bc_perf | PASS | 67921 | |
| tess_perf | PASS | 139298 | 0.4 |
| geometry | PASS | 6004 | 2445.5 |
| tessellation | PASS | 13004 | 2714.2 |
| xfb | PASS | 5295 | |
| pipeline_stats | PASS | 5368 | 1775.9 |
| vertex_stores, gs_viewport_depth, vs_viewport_index, depth_bounds | PASS | 4154, 4198, 4380, 4356 | |
| large_draw | PASS | 24970 | |
| vmr_secondary, tess_cond_state, bachata_reqs, bachata_exec, robustness2 | PASS | 4113, 4152, 4123, 4161, 4158 | |
| robust_image_access2 | PASS | 7656 | ria2 205.32 ms, ria1 204.54 ms (+0.4 %) |
| blend, occlusion_query, descriptor_model, shader_arith, depth_stencil, sampler, draw_params | PASS | 4193, 4106, 4153, 4198, 4157, 4199, 4154 | |
| submit_stress | PASS | 34097 | |
| csf_event, gs_tess_primitive_id, bachata_storage_fmtless, bachata_dynamic_render | PASS | 4611, 4657, 4139, 4103 | |
| dxvk_reqs, vkd3d_reqs, vkd3d_heap, vkd3d_timeline | PASS | 2104, 2078, 10320, 4239 | |
| swapchain_lifecycle | PASS | 15910 | 47.8 (resized 45.7) |

## Failure: fill_mode, LINE polygon mode draws nothing on v7

### What the log shows
```
CASE list_line_cull_none tris=3 kept=3 ref_red=266 red=0 mismatch=266 first=(21,7) got=0 0 255 want=255 0 0 FAIL
... every *_line* case: red=0 (nothing drawn; the cull-test fill draws and the LINE_LIST reference are correct)
CASE strip_restart_line ... ref_red=204 red=0 mismatch=271 got=0 0 0 want=255 0 0 FAIL      (black garbage pixels too)
CASE fan_restart_line_cull_back ... kept=0 ref_red=0 red=0 mismatch=47 got=0 0 0 want=0 0 255 FAIL
CASE list_line_cull_both / list_point_cull_none / fan_point_cull_back / strip_point_flipy_cull_front  PASS
FILL_MODE_FAILS=13
```
- Every VK_POLYGON_MODE_LINE case fails, and every VK_POLYGON_MODE_POINT case passes. The one line case that passes, `cull_both`, expects nothing to be drawn.
- The same test passes on the A06 with v7K (17/17 cases, same Mali-G52 r1 MC2). It also passes on the S9 (v6) with this same RC5-universal-107 .so.
- The `extra=Bifrost (v7): feature not implemented yet` text is not a driver message. PanProbe appends `UploadLogs.BIFROST_TAG` to any FAIL or SKIP on a Bifrost GPU (`MainActivity.kt:1036`), so it is misleading here.

### Root cause: polygon mode changes the v7 VS binary but is not in the cache key
1. jm-v7 075 (`patches/jm-v7-wip/0046`) compiles a v7 vertex shader with IDVS unless it may be drawn in LINE or POINT mode. In that case `polygon_mode` is true and `no_idvs` stays set, because the polygon passthrough GS (jm-v7 018, `0017`) needs the separate vertex job.
2. `panvk_hash_state()` (mesa-rc5g `src/panfrost/vulkan/panvk_vX_shader.c:2528`) hashes `non_solid` only under `#if PAN_ARCH >= 9`. jm-v7 091 (`0064`) added the v7 knobs to the hash, but not this pipeline-state input. So on v7, a FILL pipeline and a LINE pipeline with the same VS get the same cache key.
3. fill_mode first builds a FILL triangle pipeline with `vs_none` (for the cull test), which compiles it with IDVS. The LINE pipeline then gets that IDVS binary from the disk cache. The polygon passthrough reads the outputs of a vertex job that the IDVS binary never writes, so nothing (or garbage, in the restart cases) is drawn.
4. Why points pass: the point cases use `vs_psize1`, and `bi_should_idvs()` never uses IDVS for a Bifrost VS that writes gl_PointSize (`compiler/bifrost/bifrost_nir.c:1375`). Both binaries are therefore no-IDVS and identical.
5. Why the A06 v7K run passed: that tree had no Android disk-cache fallback. Its log shows `Failed to create /data/.cache for shader cache (Permission denied)---disabling`. RC5-universal uses the app cache directory (`util/disk_cache_os.c`, `u_android_app_cache_dir`), and the A31 log has no such line. The device in-memory cache is `weak_ref` (`panvk_vX_device.c:1052`), and the test destroys each pipeline before creating the next one, so only the disk cache can produce the hit.
6. Why the S9 (v6) passes: jm-v7 092 (`0065`) turns IDVS off on every v6 GPU, so the FILL and LINE binaries are identical there. The G615 (v11) hashes `non_solid`.

**Impact:** any v7 app that uses the same VS in both a FILL pipeline and a LINE pipeline gets no wireframe once the cache is warm. DXVK D3D wireframe (`D3DFILL_WIREFRAME` / `D3D11_FILL_WIREFRAME`) is the common case. POINT mode is affected too whenever the VS does not write PointSize.

### Proposed fix (jm-v7-wip, not applied: another agent owns patches/jm-v7-wip and mesa-rc5g)
Hash the same predicate that sets `no_idvs`, on PAN_ARCH < 9. Ideally factor it into one helper that both `panvk_compile_shader()` and `panvk_hash_state()` call:
```c
/* panvk_hash_state(), inside `if (state != NULL)` */
#if PAN_ARCH < 9
      /* jm-v7 075 keeps no_idvs for a VS that may be drawn in LINE/POINT
       * mode (polygon passthrough). Same predicate, or a FILL pipeline's
       * IDVS binary is reused for a LINE pipeline from the disk cache. */
      bool v7_nonfill = (stages & VK_SHADER_STAGE_VERTEX_BIT) &&
         enabled_features->fillModeNonSolid &&
         (!state->rs ||
          BITSET_TEST(state->dynamic, MESA_VK_DYNAMIC_RS_POLYGON_MODE) ||
          state->rs->polygon_mode != VK_POLYGON_MODE_FILL);
      _mesa_blake3_update(&blake3_ctx, &v7_nonfill, sizeof(v7_nonfill));
#endif
```
- Also bump the v7 knob-set tag (`0x76370091`) so that existing caches holding the bad entries are not hit.
- **Check without a rebuild:** rerun fill_mode on a v7 device with `PANVK_V7_IDVS=0` or `MESA_SHADER_CACHE_DISABLE=true`. Both should give 17/17.
- **Check with the fix:** on the A31 or A06, run fill_mode twice in one autorun (warm cache), plus a DXVK wireframe title.
- **Test-side follow-up (optional):** add a fill_mode case that creates the FILL and LINE pipelines in reverse order, and one that keeps the FILL pipeline alive, so the bug also shows with only the in-memory cache.

## tess_perf: PASS, no LOWMEM
`RESULT tess_perf PASS mismatch=0 fps=0.4 ms=139298`. `TESS_PERF_FAILS=0`, with no `FAIL LOWMEM` and no `MEM LOWMEM` line. It ran from 08:34:28 to 08:36:47 in pid 11910; both option variants ran in that one process.

### Memory curve (PanProbeTess MEM lines)
| point | avail_kb | rss_kb |
|---|---|---|
| device (default pass) | 1410272 | 19344 |
| gs=0 L=ref done | 1402464 | 24956 |
| gs=0 L=1 recorded / first frame | 1401556 / 1400984 | 44484 / 48588 |
| gs=0 L=4 / L=16 / L=mix done | 1402364 / 1414928 / 1419744 | 48592 / 52568 / 52568 |
| gs=1 L=1 recorded / first frame | 1417304 / 1364248 | 61232 / 118704 |
| gs=1 L=4 / L=16 / L=mix done | 1385124 / 1376804 / 1363896 | 118640 / 118608 / 118540 |
| teardown (default) | 1365632 | 41956 |
| device (opt-off pass) | 1384492 | 52140 |
| opt-off gs=0 L=1 first frame | 1361456 | 74704 |
| opt-off, lowest point (gs=1 L=1 first frame) | 1280240 | 144756 |
| teardown (opt-off) | 1291684 | 68472 |

- Peak RSS is 119 MB (default pass) and 145 MB (opt-off pass). MemAvailable never drops below 1.28 GB, which is far above the 300 MB LOWMEM floor.
- Memory is flat within each config.
- **Watch item:** teardown does not return to the starting RSS. It ends at 42 MB after the first pass (from 19 MB) and 68 MB after the second (from 52 MB), about 23-26 MB more per pass. The S9 went back to 15 MB both times. This is harmless at this size, but worth one check, for example in the batch TLS/WLS free path on v7 or in disk-cache mappings.

### Batch TLS/WLS (0080)
```
08:34:33.476 panvk: v7 tess batch budget 32 MiB: new batch (varyings 43 MiB worst case, GS scratch 0 MiB)
08:34:33.482 panvk: v7 batch TLS 8192 KiB (stack 2324 B/thread), half 1, shared per half
08:34:33.483 panvk: v7 batch WLS 32 KiB, half 1, shared per half
08:34:33.486 panvk: v7 batch TLS 8192 KiB (stack 2324 B/thread), half 0, shared per half
08:34:33.486 panvk: v7 batch WLS 32 KiB, half 0, shared per half
08:34:34.476 / 34.477  same pair again for half 1 and half 0 (next command buffer, L=4)
```
- There is one TLS and one WLS buffer per heap half per command buffer, as 0080 intended.
- On the MC2 the tess stack (2324 B/thread) needs **8 MiB of TLS per half (16 MiB per command buffer)**. On the S9's MC18 the same stack needed 40.5 MiB per half.
  - This matches the RSS step from 25 MB to 44 MB at gs=0 L=1.
- Plain draws use 1024 KiB of TLS (304 B/thread) and 4 KiB of WLS; one fill_mode case used 296 B/thread.

### Timings (ms per frame of 256 draws / 4096 patches; us_draw)
| L | gs=0 default | gs=1 default | gs=0 opt-off | gs=1 opt-off | S9 gs=0 / gs=1 default |
|---|---|---|---|---|---|
| ref (no tess) | 1.87 (7.3) | - | 2.07 (8.1) | - | 1.75 / - |
| 1 | 121.38 (474.2) | 391.78 (1530.4) | 124.38 (485.9) | 389.20 (1520.3) | 154.86 / 235.30 |
| 4 | 643.09 (2512.1) | 688.21 (2688.3) | 650.34 (2540.4) | 699.03 (2730.6) | 463.61 / 519.27 |
| 16 | 5119.38 (19997.6) | 1693.53 (6615.4) | 5250.26 (20508.8) | 1838.32 (7180.9) | 2265.30 / 1799.69 |
| mix | 2460.39 (9610.9) | 1503.95 (5874.8) | 2549.32 (9958.3) | 1563.00 (6105.5) | 1857.62 / 1745.18 |

- Hashes: ref `4807f3f5`, L=1 `a03404c5`, L=4 `cec55885`, L=16 `d4463b35`, L=mix `b34a251c`.
  - They are identical across gs and option variants, and the same as on the S9 and G615, so the output is correct.
- The default options are 1-8 % faster than opt-off in 7 of 8 configs. The largest gain is gs=1 L=16, at 8 %. The exception is gs=1 L=1, where opt-off is 0.7 % faster, which is noise.
- gs=0 L=16 costs 5.1 s per frame on the MC2, against 2.3 s on the S9 (MC18). At high levels the cost is bound by shader cores. At L=1 the A31 is faster (121 vs 155 ms), because there per-batch overhead dominates.

## Perf numbers, rough comparison with A06 v7K (different device, older driver)
The A31 is a Helio P65, whose G52 MC2 runs at a lower clock than the A06's Helio G85. So GPU-bound numbers are expected to be roughly 15-20 % slower.

| test | A06 v7K | A31 rc5u-107 | delta |
|---|---|---|---|
| bc_perf total ms | 63982 | 67921 | +6 % |
| bc_perf wallA, 13 BC formats | 2.50-2.73 | 2.99-3.03 | about +18 % (clock) |
| bc_perf BC6H / BC7 upload_wall_ms | 94.5-96.5 / 49.6-50.2 | 112.8-113.5 / 59.7-60.3 | about +19 % |
| robust_image_access2 ria2 / ria1 | 169.49 / 169.02 | 205.32 / 204.54 | +21 % (clock); ria2-ria1 gap still 0.4 % |
| tessellation fps | 2419.8 | 2714.2 | +12 % |
| geometry fps | 1942.5 | 2445.5 | +26 % |
| clip_cull fps | 2372.7 | 1867.5 | **-21 %** |
| multi_viewport fps | 2705.1 | 2392.4 | -12 % |
| pipeline_stats fps | 1343.0 | 1775.9 | +32 % |
| swapchain_lifecycle fps | 45.6 | 47.8 | flat |
| fill_mode fps | 790.8 | 2630.6 | not comparable (LINE draws are empty) |

- Geometry, tessellation and pipeline_stats are faster despite the slower GPU. Those are small-draw, CPU/submit-bound loops, so the gain likely comes from the post-v7K CPU work (0069-0071: index min/max, per-set descriptor dirty, shader rebind).
- clip_cull and multi_viewport are slower, by about the clock ratio. A single sample on a different device cannot separate the two causes.
- **bc_perf GPU timestamps are 2x too high on this device.** `upload_gpu_ms` is 1.97-2.0x `upload_wall_ms`, and `A_ms` (5.95) is 2.0x `wallA` (3.00). On the A06 they agree.
  - The kbase backend sets `timestamp_frequency` from the CPU's `cntfrq_el0` (13 MHz, `ts_period=76.923`) in `lib/kmod/kbase_kmod.c:507-515`. The MT6768 GPU timestamp counter evidently runs at twice that rate.
  - Only wall numbers are valid for this device. A possible fix: calibrate the GPU frequency once against `KBASE_IOCTL_GET_CPU_GPU_TIMEINFO` (two samples), instead of assuming the CPU's system counter rate. This affects `timestampPeriod` for apps on such SoCs.

## Screenshots
- `screenshots/video-060s-fill_mode-fail.png`: 08:33, 4 pass / 1 fail; fill_mode shows FAIL mismatch=2303.
- `screenshots/video-100s-tess_perf.png`: 08:35, 5 pass / 1 fail, tess_perf running.
- `screenshots/video-347s-38pass-1fail.png`: final `38 pass / 1 fail / 0 skip`, uploaded id d50c6c0f.

Frames were taken with a static ffmpeg 7.0.2 arm64 build, kept in a scratch directory and not committed.
