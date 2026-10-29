# PanVK kbase performance progress

Tracks the gains of the perf work in `docs/PERF-OPTIMIZATION-PLAN.md` against the RC4 release.

Drivers:
- **rc4**: PanVK-kbase 0.1.0-rc4 (Mesa 5a07217f release build), the PanProbe rc4 slot.
- **latest**: the series before the perf work (jm-v7-wip through 0146, `git-d66d93471c`; PanPlay numbers from the
  115e/115g runs).
- **116**: build 116 (perf), jm-v7-wip through 0151 on top of 0148, `Mesa 26.3.0-devel (git-f7e78953ac)`,
  .so sha256 `d5817e51...`. Default bundled driver since 2026-10-29.
- **118**: build 118 (perf), jm-v7-wip through 0153 (0152 v9 merged-VS attribute fix, 0153 BC compute decode per
  block), `Mesa 26.3.0-devel (git-a0412457ab)`, .so sha256 `7a09e925...`. Default bundled driver since 2026-10-29
  (v9 gate passed on Firebase oriole, Pixel 6 G78, in place of the disconnected TB336FU). ("117" is the Firebase
  build through 0148.)

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

### v9 gate: Firebase oriole (Pixel 6, Mali-G78 MC20, v9 JM), final 118

One FTL run (`worklogs/g615-dxvk/device-runs/2026-10-29-ftl-v9-oriole-118-panprobe/`): **PanProbe 40/40**,
`draw_params` PASS (0152), `bc_decode` PASS (0153), no DEVICE_LOST. 118 is the default bundled driver from here.
Per-test wall-clock seconds (log deltas, so a different method from the ms tables above; earlier oriole run =
115pre-d, `git-8a4d9f8e50`, another unit):

| Test | oriole 115pre-d (s) | oriole 118 (s) |
|---|---|---|
| tess_perf | 91.8 | 34.8 (-62 %) |
| large_draw | 32.2 | 9.6 (-70 %) |
| bc_perf | 36.0 | 32.4 (-10 %) |
| robust_image_access2 | | 8.1 |
| tessellation | | 2.4 |
| geometry | | 1.4 |
| bc_decode | 0.3 | 0.9 |

## Round 2 (2026-10-29): plan items 5-23, X6 Pro (v11)

New PanProbe perf test `workload_perf` (PanProbe 1.3.1, versionCode 20; `device/workload-perf.c`): one command
buffer per frame of a real pattern, resubmitted for 400 ms; `ms` per frame, `us/op` per draw (per frame for the
timestamps, per texture for `bc_level`). Cases with an emulated feature have a `ref` case without it; the
images of pass-through GS, XFB, predicate-true and query cases must equal `ref`. `swapchain_lifecycle` also
prints `BENCH present` (CPU ms inside `vkQueuePresentKHR`; the Android copy runs on the driver's present thread,
item 16 times that with `PANVK_FPS_LOG`). Raw: `/var/tmp/panvk/perf119/{b116,b118,c118,d*}/`.

Drivers: 116, 118 (default), and the dev builds on top of 118: `+0154` driverInfo, `+0155` item 7,
`+0156` present timing, `+0157` item 8, `+0158` item 23, `+0159` revert of 0147.

| workload_perf case | what | 116 | 118 | dev |
|---|---|---|---|---|
| ref | 512 draws x 48 tris, VS only (us/draw) | 4.00 | 4.19 || 3.12 |
| gs_pass | + pass-through GS (DXVK/emulator) | 107.4 | 97.7 || **4.47** (+0157) |
| gs_layer | + GS writing gl_Layer | 108.4 | 104.8 || 109.5 |
| xfb_vs | VS + transform feedback | 182.9 | 202.4 || 145-317 (noise, 3 runs) |
| xfb_gs | GS + transform feedback (D3D11 SO) | 583.5 | 571.3 || 579.2 |
| strip_ref | 256 indexed strips with restart (us/draw) | 7.61 | 7.69 || 7.73 |
| strip_gs | + pass-through GS | 308.6 | 302.8 || **7.52** (+0157) |
| cond_true | 512 draws, each predicated, predicate 1 | 3.95 | 2.90 || 2.86 |
| cond_false | predicate 0 | 2.76 | 2.76 || 2.68 |
| pg_query | 512 draws in a primitives-generated query | 3.15 | 3.82 || 4.34 |
| ts_ref / ts_pair | 64 draws, + timestamp pair read back (ms/frame) | 0.33 / 0.45 | 0.38 / 0.48 || 0.31 / 0.48 |
| msaa_ref | 4x MSAA 512^2, 16 blended full-screen draws (us/draw) | 75.6 | 79.1 || 54-103 (noise) |
| msaa_a2one | + alphaToOne | 255.9 | 252.2 || 254.4 |
| msaa_sshade | + sample shading 1.0 | 249.3 | 256.7 || 251.2 |
| tex_rgba / tex_bgra | 16 full-screen draws, 512^2 texture, custom border colour | 68.4 / 80.7 | 77.0 / 73.1 || 92.0 / 90.2 |
| bc_level | cold device, 6 BC textures 1024^2 + mips (ms, total) | 4937 | 4927 | 2567 (+0155) |

`bc_level` with `+0155`: device creation 6.3 ms (8.4 on 116), the recording that used to compile the 5
transcoder pipelines (4.9 s) takes 2.55 s; `PANVK_BC_PREWARM=0` on the same build: 4.47 s.
driverInfo with 0154: `Mesa 26.3.0-devel (git-33ae94dd8c, p0154, 2026-10-29)` (dev tree HEAD).

Item 8 (0157), dev column = 118 + 0154-0157 (`d157`, same session as the 118 re-run `c118`): a pass-through GS
(every input vertex copied to the output in order, no layer/viewport/primitive ID/XFB/memory writes) is dropped
at draw time and the VS runs on the hardware: `gs_pass` 111.2 to 4.47 us/draw (25x, = `ref` + 1.3 us),
`strip_gs` 421.5 to 7.52 (56x, = `strip_ref`). `gs_layer` and `xfb_gs` keep the emulation (layer write, XFB).
Pipeline statistics queries keep the GS (its invocation counters). Images equal `ref`. Full PanProbe on
`+0157` (`full-d157`): 41/41, no DEVICE_LOST. The other cases did not change; `xfb_vs`, `msaa_ref`, `tex_*`
move by up to 2x between runs of the same build (400 ms windows, thermal state).

Item 23 (0158): with `PANVK_GPU_STATS=<file>` the file read `load=83` / `vram=165822464` after `workload_perf`;
`workload_perf` with and without the knob is the same within the noise above.

0159 reverts 0147 (v14 per-chunk prerast cache flush; the v14 A/B on 116 passed `tessellation` 3/3 with
`PANVK_PRERAST_CHUNK_FLUSH=0`, 0148 was the fix). X6 Pro `tessellation`, `geometry`, `workload_perf` PASS on
`+0159`.

## Build 120 (2026-10-29): final perf driver, rc4 vs 116 vs 118 vs 120

120 = jm-v7-wip 0006-0160 (`git-94af3802af`, driverInfo `Mesa 26.3.0-devel (git-94af3802af, p0160, 2026-10-29)`).
That is 119 (0006-0159, `git-0146a719f6`) plus **0160**, which fixes the rc4 vs 115c+ PanPlay fps gap.
The gap came from 0115's cached sw readback. Details:
`worklogs/g615-dxvk/device-runs/2026-10-29-x6pro-rc4-vs-119-fps/`. The local regression (screenshots, raw
results, memory samples) is in `worklogs/g615-dxvk/device-runs/2026-10-29-local-120-regression/`.

- **PanProbe:** 41/41 on both devices, no DEVICE_LOST. X6 Pro cold and warm; TB336FU twice. rc4 on TB336FU:
  39/41.
- **PanPlay 1-9:** every task passes on 120, no DEVICE_LOST.
  - On rc4, every D3D12 task fails on both devices.

The 116 runs fail only `workload_perf`, in its `bc_level` case. That test is newer than 116; it passes on 118+.

### PanProbe, X6 Pro v11 (ms per test, warm cache, same session)

| Test | rc4 | 116 | 118 | 120 |
|---|---|---|---|---|
| tess_perf | 48411 | 28987 | 29075 | 28978 (-40 %) |
| tessellation | 3199 | 2067 | 2097 | 2219 (-31 %) |
| geometry | 2049 | 1617 | 1610 | 1543 (-25 %) |
| xfb | 369 | 218 | 219 | 233 (-37 %) |
| large_draw | 9613 | 9078 | 9376 | 9366 |
| bc_decode | 595 | 160 | 161 | 181 |
| draw_params | 121 | 95 | 107 | 93 |
| workload_perf `gs_pass` (us/draw) | 91.6 | 110.9 | 99.1 | **3.7** |
| workload_perf `strip_gs` (us/draw) | 337 | 313 | 376 | **7.5** |
| workload_perf `bc_level` (ms, cold BC compile) | 285 | 4476 | 4917 | 2592 |
| **all 41 tests, sum** | 152839 | 132911 (40/41) | 134200 | **131056 (-14 %)** |

`bc_level` on rc4 is fast because rc4 has no lowmem BC transcoder, so it has nothing to compile. With a cold
shader cache, 120 `bc_decode` takes 9.3 s; on 118 cold it took 5.0 s, and warm it is 0.18 s. The cause: 0155 also
compiled the unused XC_BATCH pipeline per ETC2 mode. **Fixed in 0162** (build 121b, below).

### PanProbe, TB336FU v9 (ms per test)

| Test | rc4 | 120 |
|---|---|---|
| tess_perf | 20705 **FAIL** | 34080 |
| tessellation | 7749 | 1863 (-76 %) |
| geometry | 5508 | 1185 (-78 %) |
| xfb | 4312 | 205 (-95 %) |
| large_draw | 28365 **FAIL** | 7537 |
| bc_decode | 4651 | 112 |
| bc_perf | 53329 | 44839 |
| draw_params | 4125 | 135 |
| workload_perf | 21967 | 9536 |
| **sum** | 330333 (39/41) | 150608 (41/41, -54 %) |

On v9, `workload_perf` `gs_pass` is 24 us/draw (rc4: 53) and `strip_gs` is 353 (rc4: 1927). The pass-through
skip in 0157 does not fire on the v9 JM path at all, because 0157 hooked only the CSF draw. **Fixed in 0163**
(build 121b, below).

### PanPlay fps, rc4 vs 120 (full suite, 10 s warm-up, 30 s measured)

Each cell is fps / 1 %-low / drawn. fps is the DXVK HUD rate; for D3D12 it is the driver fps log, because rc4
has no driver fps log. The X6 Pro display composites at about 60 Hz, the TB336FU at 90 Hz. Screenshots are in
the regression worklog under `shots/<device>-<rc4|120>-panplay-s<N>[-k].png`.

| s | task | X6 rc4 | X6 120 | TB rc4 | TB 120 |
|---|---|---|---|---|---|
| 1 | d3d11 cube | 278.5 / 80.8 / 56 | 287.9 / 78.6 / 53 | 160.2 / 71.2 / 90 | 159.8 / 75.4 / 90 |
| 2 | d3d9 cube | 116.9 / 32.4 / 55 | 115.5 / 31.8 / 53 | 89.9 / 49.0 / 90 | 90.0 / 50.3 / 90 |
| 3 | d3d11 | 268.0 / 80.2 / 56 | 276.2 / 84.4 / 55 | 160.8 / 82.4 / 90 | 165.9 / 94.5 / 90 |
| 3 | d3d9 | 116.8 / 33.9 / 55 | 115.2 / 31.8 / 53 | 89.9 / 47.4 / 90 | 90.0 / 51.8 / 90 |
| 4 | d3d11 old / default | 270.1 / 277.8 | 276.7 / 279.8 | 164.3 / 161.4 | 164.8 / 162.9 |
| 4 | d3d9 old / default | 117.0 / 115.6 | 116.3 / 117.9 | 89.9 / 90.0 | 90.0 / 90.0 |
| 5 | d3d12 SquidRoom | fail | 59.0 / - / 55 | fail | 24.8 / - / 25 |
| 6 | d3d12 gears | fail | 174.0 / - / 60 | fail | 226.4 / - / 90 |
| 7 | d3d12 FL 11_0 / 11_1 / 12_0 / 12_1 / 12_2 | fail | 198 / 207 / 210 / 166 / 179 | fail | 477 / 504 / 480 / 472 / 527 |
| 8 | d3d11 FL 10_0 | 794.9 / 137.2 / 57 | 746.1 / 147.5 / 44 | 199.7 / 99.0 / 90 | 217.1 / 100.1 / 90 |
| 8 | d3d11 FL 10_1 | 798.1 / 135.1 / 54 | 739.2 / 132.0 / 49 | 200.1 / 98.2 / 90 | 214.0 / 105.4 / 90 |
| 8 | d3d11 FL 11_0 | 797.9 / 128.2 / 57 | 752.3 / 140.7 / 44 | 199.7 / 97.6 / 90 | 204.8 / 100.4 / 90 |
| 8 | d3d11 FL 11_1 | 820.0 / 150.3 / 52 | 707.9 / 105.6 / 51 | 200.4 / 102.9 / 90 | 198.6 / 97.7 / 90 |
| 8 | d3d10 cube | 271.2 / 83.0 / 56 | 271.7 / 73.4 / 55 | 162.0 / 74.4 / 90 | 157.0 / 87.8 / 90 |
| 8 | d3d9 cube | 117.5 / 34.7 / 55 | 117.1 / 35.6 / 54 | 90.0 / 49.0 / 90 | 89.9 / 49.4 / 90 |
| 9 | d3d12 FL all | fail | 197.5 / - / 60 | fail | 498.4 / - / 90 |
| 9 | d3d11 FL all | 825.9 / 156.6 / 53 | 772.5 / 158.4 / 41 | 195.7 / 100.5 / 90 | 192.9 / 100.3 / 90 |

119 (before 0160) on the X6 Pro, same window: FL 10_0 to 11_1 ran at 272-359 fps and FL all at 397. With 0160,
120 reaches 92-94 % of rc4 on the tiny FL frames. The 120 runs came after the rc4 runs, so the device was
warmer. The cubes, D3D9 and D3D10 are equal on both devices.

### Memory

**PanProbe:** maximum over 5 s `dumpsys meminfo` samples, full run, X6 Pro warm. The `-` cells were not sampled.

| | rc4 | 116 | 118 | 120 |
|---|---|---|---|---|
| X6 Pro TOTAL PSS (MB) | 194 | 162 | 163 | 184 (cold run: 166) |
| X6 Pro Graphics (MB) | 93 | 61 | 61 | 61 |
| TB336FU TOTAL PSS / Graphics (MB) | 229 / 128 | - | - | 226 / 126 |

**PanPlay:** one sample per test process, about 20 s into the pass; maximum per scenario. The driver VRAM
counter comes from 0158 (`PANVK_GPU_STATS`, CSF only, so none on v9). The `-` cells are scenarios that fail on
rc4.

| Scenario | X6 PSS rc4 / 120 (MB) | X6 Graphics rc4 / 120 | X6 driver VRAM 120 (MiB) | TB PSS rc4 / 120 | TB Graphics rc4 / 120 |
|---|---|---|---|---|---|
| s1 d3d11 | 262 / 262 | 111 / 111 | 51 | 334 / 356 | 190 / 206 |
| s3 d3d11 + d3d9 | 298 / 302 | 125 / 125 | 54 | 359 / 389 | 192 / 212 |
| s5 SquidRoom | - / 306 | - / 132 | 154 | - / 365 | - / 212 |
| s7 D3D12 FL | - / 286 | - / 118 | 166 | 327 / 372 | 168 / 208 |
| s8 D3D11 FL | 326 / 326 | 121 / 120 | 54 | 366 / 379 | 192 / 208 |
| s9 FL all | 304 / 243 | 104 / 104 | 167 | 329 / 371 | 168 / 208 |

- **X6 Pro (v11):** 120 uses the same memory as rc4 in PanPlay. PanProbe Graphics is 32 MB lower (116 and 118 too;
  not split by patch).
- **TB336FU (v9):** 120 is +13-45 MB PSS and +16-40 MB Graphics in PanPlay. Its PanProbe totals equal rc4.

## Build 121b (2026-10-29): 0162 cold-cache BC, 0163 v9 pass-through GS (not the default)

121b = jm-v7-wip 0006-0163, `Mesa 26.3.0-devel (git-07177325ca, p0163, 2026-10-29)`. It includes the tiler-OOM
0161.
- .so: `/var/tmp/panvk/build-out/2026-10-29-121b/libvulkan_panfrost.so`, sha256 `9f58ee27...`, BuildID
  `c5337130...`.
- Details and raw results: `worklogs/g615-dxvk/device-runs/2026-10-29-local-121-bc-gs/`.

PanProbe 41/41 on both devices, cold and warm, with no DEVICE_LOST. Cold means only PanProbe's
`cache/mesa_shader_cache` was removed.

| | X6 120 cold | X6 121b cold / warm | TB 120 cold | TB 121b cold / warm |
|---|---|---|---|---|
| bc_decode (ms) | 9346 | **5155** / 180 | 1639 | 1610 / 130 |
| workload `bc_level` (ms) | 2674 | 2994 / 2544 | 1513 | 1512 / 1517 |
| `gs_pass` (us/draw) | 4.4 | 3.1 / 2.4 | 30.0 | **7.0 / 10.8** |
| `strip_gs` (us/draw) | 9.2 | 6.3 / 4.3 | 354 | **17.6 / 15.8** |

- **0162:** the XC_BATCH transcoder pipeline is prewarmed only for one-layer 2D images with mips; other images
  compile it at first use. A first try (121) dropped the batch prewarm entirely, and `bc_level` rose to
  3.3-3.5 s.
- **0163:** the 0157 pass-through GS swap is added to the v9 JM `v9_cmd_draw()`.
- PanPlay and the RTL/Firebase arches have not been run on 121b.

## Build 121b PanPlay (2026-10-29)

Full PanPlay 1-9 on 121b (`git-07177325ca`, p0163, BuildID `c5337130...`) on X6 Pro (v11) and TB336FU (v9), same
short-window config as the build 120 run. All tasks pass, no DEVICE_LOST, no GPU reset lines in logcat/dmesg. fps is
within noise of 120 (X6 Pro s7 D3D12 FL tasks swing -27 %..+39 %, other tasks within about 6 %; TB336FU worst -13 %).
Extra: X6 Pro D3D11 cube s1/s3, 3 alternating runs, default vs `PANVK_KBASE_TILER_IR=0`: 279.7 vs 277.2 and 277.2 vs
278.3 fps, no difference, so the v13 cube drop is not the tiler IR path on v11. Details, screenshots and raw:
`worklogs/g615-dxvk/device-runs/2026-10-29-local-121b-panplay/`.

## Build 122 (2026-10-29): final perf build, rc4 vs 116 vs 121b vs 122

122 = jm-v7-wip 0006-0165, `Mesa 26.3.0-devel (git-3506474d17, p0165, 2026-10-29)`: 121b plus **0164** (MangoHud
GPU load on JM, item 23) and **0165** (v9 primitives-generated counted on the CPU, item 21). Strict prepare
`OK base=228 jm-v7-wip=160 head=3506474d173`.
- .so: `/var/tmp/panvk/build-out/2026-10-29-122/libvulkan_panfrost.so`, sha256
  `acb4a0116987fd807163f438424cec9c6c3fbd0d0712d47d1a3115197cebc81e`, 29464696 bytes, BuildID
  `2f61168ad05aa9204824eb58cd9a50de43e8c785`.
- APKs: `build-out/apks/panprobe-122.apk` (`180f61a1...`, rc4 + 122), `panplay-122.apk` (`269fc98d...`, from main
  3d7f9de6, includes the FO4 audio fix 0ae08d6a, `libglxstub.so`). Both APK labels are derived from the .so.
- **Default bundled driver** (both pins, `prepare-rc5-tree.sh` `WIP_MAX=0165`).
- Raw results, screenshots, memory samples: `worklogs/g615-dxvk/device-runs/2026-10-29-local-122/`.

Local only (X6 Pro v11 on wireless adb, battery 36.5 C at the start; TB336FU v9). Firebase and RTL not run.

**PanProbe** (sum of the 41 tests, ms; max TOTAL PSS / Graphics over 5 s samples, MB):

| | rc4 | 116 | 121b | 122 cold | 122 warm |
|---|---|---|---|---|---|
| X6 Pro pass | 41/41 | 40/41 | 41/41 | 41/41 | 41/41 |
| X6 Pro sum | 156188 | 132911 | 133946 | 140599 | **128997 (-17 % vs rc4)** |
| X6 Pro PSS / Graphics | 198 / 65 | 162 / 61 | 178 / 61 | 198 / 65 | 198 / 65 |
| TB336FU pass | 39/41 | - | 41/41 | 41/41 | 41/41 |
| TB336FU sum | 331346 | - | 157912 | 160358 | **157737 (-52 % vs rc4)** |
| TB336FU PSS / Graphics | 236 / 129 | - | 234 / 131 | 231 / 127 | 233 / 129 |

116 is the earlier table (X6 Pro only). 0164/0165 are JM-only, so the X6 Pro differences against 121b are run
noise (PSS swings 166-198 MB between runs of the same build, see Build 120).

TB336FU `workload_perf` (us/draw, 3 alternating runs of 121b and 122a = 122 code): `pg_query` 18.6 to **12.2**
(`ref` 6.2); `cond_true` 15 (item 19 not done), `ts_pair` - `ts_ref` about 0.1 ms per pair per frame (item 20
closed). `bc_level` (cold BC level load) is 254 ms on 122 vs 109 ms on rc4 on v9: the BC transcoder compile cost of
0155+, as on v11.

**MangoHud GPU stats on JM (0164):** TB336FU under `workload_perf` `load=97..99`; PanPlay s1-s9 `load` 66-99.
`vram` on JM reads about 1.35-1.68 GiB, against 54 MiB on CSF: it counts the full size of the JM growable private
BOs (tess arena, varying and GS buffers), not the committed pages. Known overestimate, not fixed.

**PanPlay 1-9 (122):** every task passes on both devices, no DEVICE_LOST, no GPU reset in dmesg. fps / 1 %-low,
compared with the build 120 run above:

| s | task | X6 120 | X6 122 | TB 120 | TB 122 |
|---|---|---|---|---|---|
| 1 | d3d11 cube | 287.9 | 272.0 / 237.3 | 159.8 | 160.8 / 110.0 |
| 2 | d3d9 cube | 115.5 | 115.6 / 108.6 | 90.0 | 89.9 / 87.9 |
| 5 | d3d12 SquidRoom | 59.0 | **78.8** / 61.6 | 24.8 | 23.0 / 22.4 |
| 6 | d3d12 gears | 174.0 | **214.3** / 181.0 | 226.4 | 222.3 / 210.9 |
| 7 | d3d12 FL 11_0 / 11_1 / 12_0 / 12_1 / 12_2 | 198 / 207 / 210 / 166 / 179 | 227 / 219 / 238 / 215 / 216 | 477 / 504 / 480 / 472 / 527 | 485 / 481 / 468 / 471 / 479 |
| 8 | d3d11 FL 10_0 / 10_1 / 11_0 / 11_1 | 746 / 739 / 752 / 708 | 766 / 725 / 796 / 752 | 217 / 214 / 205 / 199 | 200 / 203 / 203 / 197 |
| 8 | d3d10 cube | 271.7 | 274.0 | 157.0 | 165.5 |
| 9 | d3d12 FL all / d3d11 FL all | 197.5 / 772.5 | 224.9 / 741.8 | 498.4 / 192.9 | 489.1 / 197.6 |

PanPlay memory (max TOTAL PSS / Graphics, MB): X6 Pro s1 295 / 135, s3 367 / 123, s5 365 / 127, s7 344 / 121,
s8 342 / 131, s9 301 / 121; TB336FU s1 340 / 190, s3 391 / 212, s5 385 / 212, s7 373 / 208, s8 376 / 214, s9
369 / 208 (120: TB s1 356 / 206, s3 389 / 212). The X6 Pro `gpustats` sample is the same in every scenario
(load 74, vram 54 MiB): the harness read a stale `gpustats.txt`, not checked further.

**v13 single-scene cube** (RTL S11 Ultra, other agent, `2026-10-29-rtl-v13-sm-x930-cube-ab/`): 118 964 fps / 1 %-low
116 vs 121b about 600 / 290, so 121b is smoother with a lower average; `PANVK_WSI_SW_CACHED=1` and
`PANVK_KBASE_TILER_IR=0` change nothing; the 119 bisect is open.
