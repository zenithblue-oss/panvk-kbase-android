# PanVK kbase performance progress

Tracks the gains of the perf work in `docs/PERF-OPTIMIZATION-PLAN.md` against the RC4 release.

Drivers:
- **rc4**: PanVK-kbase 0.1.0-rc4 (Mesa 5a07217f release build), the PanProbe rc4 slot.
- **latest**: the series before the perf work (jm-v7-wip through 0146, `git-d66d93471c`; PanPlay numbers from the
  115e/115g runs).
- **116**: build 116 (perf), jm-v7-wip through 0151 on top of 0148, `Mesa 26.3.0-devel (git-f7e78953ac)`,
  .so sha256 `d5817e51...`. Default bundled driver since 2026-10-29.
- **118**: build 118 (perf), jm-v7-wip through 0153 (0152 v9 merged-VS attribute fix, 0153 BC compute decode per
  block), `Mesa 26.3.0-devel (git-a0412457ab)`, .so sha256 `7a09e925...`. Not default yet: the TB336FU (v9) run is
  pending. ("117" is the Firebase build through 0148.)

Devices: TB336FU = Lenovo, Mali-G57 (v9 JM). X6 Pro = POCO X6 Pro, Mali-G615 (v11 CSF). The v6, v7, v10 and v14
columns are empty until 116 runs there (Firebase / RTL).

Percentages are against rc4. Lower is better for every PanProbe number (ms or us per draw); higher is better for fps.

## PanProbe (2026-10-29, warm shader cache)

Each column is one full 40-test run (`--ei scenario 2`); 116 is the second of two runs (the first run of a new
driver compiles the shader cache: X6 Pro `bc_decode` is 5.0 s cold, 0.18 s warm). Raw data:
`/var/tmp/panvk/perf116/{base-*,f2-*}/raw/`.

Notes:
- rc4 on v9 JM has a ~2 s warm-up in every test (fixed after rc4), so most v9 rc4 test times are about 4 s.
- rc4 fails `tess_perf` and `large_draw` on TB336FU, so those rows have no %; compare 116 with latest instead.

### tess_perf (us per draw, default options)

| Config | TB336FU v9 rc4 | TB336FU v9 latest | TB336FU v9 116 | X6 Pro v11 rc4 | X6 Pro v11 latest | X6 Pro v11 116 | v6 | v7 | v10 | v14 |
|---|---|---|---|---|---|---|---|---|---|---|
| gs=0 L=1 | 192.8 | 107.6 (-44 %) | 116.4 (-40 %) | 556.6 | 374.2 (-33 %) | 382.1 (-31 %) | | | | |
| gs=0 L=4 | 642.4 | 284.6 (-56 %) | 286.9 (-55 %) | 587.3 | 286.4 (-51 %) | 304.8 (-48 %) | | | | |
| gs=0 L=16 | 3143.3 | 1500.6 (-52 %) | 1493.1 (-52 %) | 2787.4 | 1150.7 (-59 %) | 1186.6 (-57 %) | | | | |
| gs=0 L=mix | 2860.5 | 1314.3 (-54 %) | 1314.3 (-54 %) | 2734.6 | 1064.3 (-61 %) | 1069 (-61 %) | | | | |
| gs=1 L=1 | 511.3 | 305 (-40 %) | 288.2 (-44 %) | 691.8 | 568.1 (-18 %) | 556 (-20 %) | | | | |
| gs=1 L=4 | 2030.3 | 1445.5 (-29 %) | 876.2 (-57 %) | 872.1 | 561.5 (-36 %) | 708.3 (-19 %) | | | | |
| gs=1 L=16 | FAIL | 10574.8 | 3137.1 (-70 % vs latest) | 4681.8 | 2961.1 (-37 %) | 2387 (-49 %) | | | | |
| gs=1 L=mix | FAIL | 4882.9 | 2553.6 (-48 % vs latest) | 3757.2 | 1892.9 (-50 %) | 1751.4 (-53 %) | | | | |
| test time (ms) | FAIL 20599 | 58889 | 32318 (-45 % vs latest) | 48637 | 30172 (-38 %) | 28715 (-41 %) | | | | |

### Feature tests (ms per test)

| Test | TB336FU v9 rc4 | TB336FU v9 latest | TB336FU v9 116 | X6 Pro v11 rc4 | X6 Pro v11 latest | X6 Pro v11 116 | v6 | v7 | v10 | v14 |
|---|---|---|---|---|---|---|---|---|---|---|
| tessellation | 7562 | 3144 (-58 %) | 1841 (-76 %) | 3183 | 2347 (-26 %) | 2196 (-31 %) | | | | |
| geometry | 5451 | 1252 (-77 %) | 1242 (-77 %) | 2216 | 1571 (-29 %) | 1602 (-28 %) | | | | |
| xfb | 4341 | 213 (-95 %) | 195 (-96 %) | 463 | 322 (-30 %) | 223 (-52 %) | | | | |
| gs_tess_primitive_id | 4310 | 224 (-95 %) | 207 (-95 %) | 299 | 219 (-27 %) | 234 (-22 %) | | | | |
| gpu_prerast_slice | 4077 | 138 (-97 %) | 167 (-96 %) | 93 | 80 (-14 %) | 89 (-4 %) | | | | |
| fill_mode | 5373 | 1162 (-78 %) | 1166 (-78 %) | 2350 | 1605 (-32 %) | 1620 (-31 %) | | | | |
| vertex_stores | 4160 | 123 (-97 %) | 154 (-96 %) | 187 | 229 (+22 %) | 183 (-2 %) | | | | |
| multi_viewport | 5093 | 1088 (-79 %) | 1086 (-79 %) | 1204 | 1165 (-3 %) | 1147 (-5 %) | | | | |
| gs_viewport_depth | 4235 | 212 (-95 %) | 248 (-94 %) | 287 | 299 (+4 %) | 207 (-28 %) | | | | |
| vs_viewport_index | 4223 | 201 (-95 %) | 211 (-95 %) | 327 | 307 (-6 %) | 239 (-27 %) | | | | |
| clip_cull | 5121 | 1125 (-78 %) | 1106 (-78 %) | 1266 | 1212 (-4 %) | 1188 (-6 %) | | | | |
| depth_bounds | 4310 | 176 (-96 %) | 173 (-96 %) | 222 | 204 (-8 %) | 194 (-13 %) | | | | |
| pipeline_stats | 5187 | 1152 (-78 %) | 1137 (-78 %) | 1412 | 1314 (-7 %) | 1254 (-11 %) | | | | |
| large_draw | FAIL 28387 | 24607 | 10178 (-59 % vs latest) | 9842 | 9555 (-3 %) | 9392 (-5 %) | | | | |
| bc_decode | 4614 | 113 (-98 %) | 114 (-98 %) | 601 | 228 (-62 %) | 180 (-70 %) | | | | |
| bc_perf | 53249 | 46656 (-12 %) | 46204 (-13 %) | 28737 | 28346 (-1 %) | 28218 (-2 %) | | | | |
| robust_image_access2 | 10533 | 6483 (-38 %) | 6504 (-38 %) | 709 | 642 (-9 %) | 649 (-8 %) | | | | |
| blend | 4104 | 98 (-98 %) | 119 (-97 %) | 156 | 113 (-28 %) | 114 (-27 %) | | | | |
| draw_params | 4141 | 126 (-97 %) | 123 (-97 %) | 181 | 151 (-17 %) | 117 (-35 %) | | | | |
| **all 40 tests, sum** | 308197 (38/40) | 185862 (40/40, -40 %) | 143167 (40/40, -54 %) | 144996 (40/40) | 122206 (40/40, -16 %) | 119413 (40/40, -18 %) | | | | |

Differences under about 15 % on sub-300 ms tests are run-to-run noise (single runs).

### bc_perf upload (ms per 2048x2048 upload, GPU time)

Unchanged by 116 (no BC work yet). On v9 the BC compute decode is 6-18x an RGBA8 upload; on v11 it is near parity.

| Format | TB336FU v9 rc4 | TB336FU v9 latest | TB336FU v9 116 | X6 Pro v11 rc4 | X6 Pro v11 latest | X6 Pro v11 116 | v6 | v7 | v10 | v14 |
|---|---|---|---|---|---|---|---|---|---|---|
| BC1_RGB_UNORM | 38.7 | 38.8 | 38.7 | 2.9 | 2.3 | 2.7 | | | | |
| BC3_UNORM | 56.5 | 56.8 | 56.7 | 5.3 | 2.0 | 3.6 | | | | |
| BC4_UNORM | 41.6 | 42.1 | 42.1 | 5.0 | 0.8 | 1.2 | | | | |
| BC5_UNORM | 48.9 | 49.0 | 48.7 | 5.1 | 1.1 | 2.3 | | | | |
| BC6H_UFLOAT | 98.6 | 98.1 | 99.0 | 8.3 | 4.4 | 4.3 | | | | |
| BC6H_SFLOAT | 116.1 | 116.8 | 117.1 | 8.4 | 4.7 | 5.0 | | | | |
| BC7_UNORM | 77.5 | 77.5 | 77.8 | 5.7 | 5.6 | 5.5 | | | | |
| REF_RGBA8 | 6.2 | 6.3 | 6.3 | 2.7 | 2.4 | 2.3 | | | | |

## PanPlay (TB336FU only, fps, 48 s per pass)

The X6 Pro has the old signed PanPlay 1.2.9 and is not run. rc4 = PanPlay APK with the RC4 .so in place of the
bundled driver; rc4 has no `PANVK_FPS_LOG`, so all columns use the DXVK/vkd3d fps. rc4 predates the D3D12 work:
s6 and s7 fail on rc4. Latest s7: 11_0 from 115g run 1, 11_1 from run 2 (run 1 11_1 306.6, run 2 11_0 flaked).
116 run: `/var/tmp/panvk/perf116/play-f2/`, all passes ok, no deviceLost.

| Scenario / pass | rc4 | latest | 116 | v6 | v7 | v10 | v14 |
|---|---|---|---|---|---|---|---|
| s3 d3d11 cube | 160.8 | 155.7 (-3 %) | 161.5 (+0 %) | | | | |
| s3 d3d9 cube | 90.0 (vsync) | 90.0 | 90.0 | | | | |
| s5 d3d12 multithreading | - | 23.1 | 22.9 | | | | |
| s6 vkd3d gears | FAIL | 247.1 | 257.5 | | | | |
| s7 d3d12 FL 11_0 | FAIL | 394.2 | 403.8 | | | | |
| s7 d3d12 FL 11_1 | FAIL | 408.9 | 425.1 | | | | |
| s8 d3d11 FL 10_0 | 192.8 | 211.4 (+10 %) | 214.9 (+11 %) | | | | |
| s8 d3d11 FL 10_1 | 198.2 | 212.4 (+7 %) | 212.5 (+7 %) | | | | |
| s8 d3d11 FL 11_0 | 193.7 | 213.1 (+10 %) | 214.8 (+11 %) | | | | |
| s8 d3d11 FL 11_1 | 198.1 | 233.8 (+18 %) | 203.0 (+2 %) | | | | |
| s8 d3d10 cube | 164.1 | 160.6 (-2 %) | 158.2 (-4 %) | | | | |
| s8 d3d9 cube | 90.0 (vsync) | 89.9 | 89.9 | | | | |

The PanPlay scenes are fragment-bound and use no GS/tess at these sizes, so 116 is within noise of latest
(s8 FL 11_1: an earlier 116 candidate build measured 213.2; one run each). The 116 gains are in the emulated
geometry paths measured by PanProbe.

## Build 118 (2026-10-29)

X6 Pro: final build, two full runs, 40/40 both; numbers are run 2 (warm). TB336FU: **pending** for the final
build (device disconnected); the v9 numbers below are from the dev build with the same tree (0152 full run cold
cache; 0153 `bc_perf`, warm) and are marked "dev". Raw: `/var/tmp/panvk/perf117/{p117-x6-*,full-0152-tb-1,bcp-tb-*}`.

### PanProbe, X6 Pro v11 (ms per test unless noted)

| Test | rc4 | 116 | 118 |
|---|---|---|---|
| tess_perf gs=0 L=16 (us/draw) | 2787.4 | 1186.6 | 1157.4 (-58 %) |
| tess_perf gs=1 L=16 (us/draw) | 4681.8 | 2387 | 2348.6 (-50 %) |
| tessellation | 3183 | 2196 | 2195 (-31 %) |
| geometry | 2216 | 1602 | 1564 (-29 %) |
| xfb | 463 | 223 | 216 (-53 %) |
| large_draw | 9842 | 9392 | 9286 (-6 %) |
| bc_decode | 601 | 180 | 165 (-73 %) |
| draw_params | 181 | 117 | 112 (-38 %) |
| **all 40 tests, sum** | 144996 (40/40) | 119413 (40/40) | 119229 (40/40, -18 %) |

v11 is within noise of 116, as expected: 0152 is v9-only and v11 uses the fragment BC decode for 2D images
(`bc_perf` upload on v11 unchanged, e.g. BC7 5.5 ms in both).

### TB336FU v9 (dev build, final run pending)

| Measure | rc4 | latest | 116 | 118 dev |
|---|---|---|---|---|
| large_draw (ms) | FAIL 28387 | 24607 | 10178 | 7198 (-71 % vs latest, cold cache) |
| draw_params (ms) | 4141 | 126 | 123 | 144 (PASS with merging) |
| bc_perf BC1_RGB_UNORM upload (ms) | 38.7 | 38.8 | 38.7 | 13.9 (-64 %) |
| bc_perf BC3_UNORM | 56.5 | 56.8 | 56.7 | 15.6 (-72 %) |
| bc_perf BC4_UNORM | 41.6 | 42.1 | 42.1 | 15.5 (-63 %) |
| bc_perf BC5_UNORM | 48.9 | 49.0 | 48.7 | 15.8 (-68 %) |
| bc_perf BC6H_UFLOAT | 98.6 | 98.1 | 99.0 | 37.2 (-62 %) |
| bc_perf BC6H_SFLOAT | 116.1 | 116.8 | 117.1 | 38.5 (-67 %) |
| bc_perf BC7_UNORM | 77.5 | 77.5 | 77.8 | 15.2 (-80 %) |
| bc_perf REF_RGBA8 | 6.2 | 6.3 | 6.3 | 6.4 |
| PanProbe full, final build | | | 40/40 | **pending** |

BC is now 2.2-2.5x an RGBA8 upload on v9 (was 6-18x); BC6H stays about 6x (RGBA16F output, 10.8 ms reference).
