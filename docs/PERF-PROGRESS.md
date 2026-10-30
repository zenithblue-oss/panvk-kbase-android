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


## Build 122c/122d/122e gate (2026-10-30): rc4 vs 116 vs 122c, TB336FU v9

Release gate: the final build must beat rc4 and 116 in speed and memory on every PanProbe and PanPlay benchmark.
Builds after 122b:
- 122c adds 0167, which turns 0161 off on v14, where it was a release blocker.
- 122d adds 0168: sw WSI readback cached on v13+.
- 122e adds 0169: cached on every arch except v11.

Neither 0168 nor 0169 changes PanProbe on v9, and 0167 is CSF-only. So the v9 PanProbe numbers below hold for 122c, 122d and 122e.
PanPlay on v9 changes with 0169 (cached readback again), see "0160 on v9" below.

How the runs were made:
- One session on the TB336FU (battery 28-29 C), using the same app build (main bbb22c94).
- Only the .so differs: panprobe-116c and panplay-rc4c/116c are built from the same tree as 122c.
- PanProbe: two cold and two warm runs per build, in alternating order (122c round 1, rc4, 116, then 122c, 116, rc4).
- PanPlay: one full 1-9 pass each for rc4c and 116c, and two for 122c.
- The value shown is the mean. "±" is half the spread between runs.
- Flag rule: the candidate mean is worse than a baseline mean by more than 5 % and by more than both spreads.
- Raw data: `worklogs/g615-dxvk/device-runs/2026-10-30-local-122c-gate/`. The table script is `table.py` there.

rc4's PanProbe test times are ~2-4 s higher for every test: its slot runs each test with a longer fixed setup. So per-test
rc4 times are not comparable, but its `workload_perf` cases and memory are.

### PanProbe TB336FU, cold cache (ms per test, `workload_perf` us per op)

| metric | rc4 | 116 | 122c | flag |
|---|---|---|---|---|
| gpu_prerast_slice ms | 4081.0 ±3.0 | 164.0 ±24.0 | 173.0 ±24.0 |  |
| clip_cull ms | 5131.5 ±4.5 | 1156.0 ±11.0 | 1163.0 ±16.0 |  |
| multi_viewport ms | 5085.5 ±0.5 | 1100.0 ±15.0 | 1094.0 ±1.0 |  |
| fill_mode ms | 5399.0 ±3.0 | 1241.0 ±41.0 | 1240.0 ±35.0 |  |
| bc_decode ms | 4617.0 ±15.0 | 620.0 ±4.0 | 1617.5 ±7.5 | worse than 116 |
| bc_perf ms | 53637.5 ±43.5 | 46401.5 ±9.5 | 45087.0 ±98.0 |  |
| tess_perf ms | - | 33138.0 ±379.0 | 33279.0 ±1377.0 |  |
| workload_perf ms | 22016.0 ±9.0 | 11530.5 ±15.5 | 9467.5 ±5.5 |  |
| geometry ms | 5498.5 ±10.5 | 1399.5 ±19.5 | 1362.5 ±4.5 |  |
| tessellation ms | 7822.5 ±48.5 | 2273.0 ±4.0 | 2105.5 ±14.5 |  |
| xfb ms | 4335.5 ±2.5 | 300.5 ±2.5 | 289.5 ±3.5 |  |
| pipeline_stats ms | 5209.0 ±7.0 | 1165.0 ±13.0 | 1151.0 ±0.0 |  |
| vertex_stores ms | 4140.0 ±2.0 | 155.5 ±6.5 | 150.0 ±3.0 |  |
| gs_viewport_depth ms | 4243.5 ±2.5 | 284.0 ±37.0 | 302.5 ±4.5 |  |
| vs_viewport_index ms | 4241.0 ±15.0 | 242.5 ±5.5 | 251.0 ±12.0 |  |
| depth_bounds ms | 4313.0 ±2.0 | 197.5 ±3.5 | 185.5 ±1.5 |  |
| large_draw ms | - | 10231.5 ±157.5 | 7419.0 ±6.0 |  |
| vmr_secondary ms | 4091.5 ±2.5 | 101.5 ±2.5 | 103.0 ±9.0 |  |
| tess_cond_state ms | 4168.0 ±5.0 | 233.5 ±25.5 | 265.5 ±19.5 |  |
| bachata_reqs ms | 4177.5 ±17.5 | 107.0 ±13.0 | 92.0 ±8.0 |  |
| bachata_exec ms | 4100.5 ±0.5 | 120.0 ±5.0 | 128.0 ±3.0 |  |
| robustness2 ms | 4098.5 ±4.5 | 141.5 ±3.5 | 135.5 ±11.5 |  |
| robust_image_access2 ms | 10535.0 ±1.0 | 6573.0 ±4.0 | 6564.0 ±27.0 |  |
| blend ms | 4099.0 ±5.0 | 103.5 ±0.5 | 120.0 ±18.0 |  |
| occlusion_query ms | 4099.5 ±5.5 | 125.0 ±21.0 | 117.0 ±3.0 |  |
| descriptor_model ms | 4106.5 ±6.5 | 129.0 ±5.0 | 135.0 ±1.0 |  |
| shader_arith ms | 4113.0 ±23.0 | 197.0 ±16.0 | 171.5 ±7.5 |  |
| depth_stencil ms | 4131.0 ±4.0 | 140.0 ±19.0 | 141.0 ±2.0 |  |
| sampler ms | 4126.0 ±4.0 | 204.0 ±40.0 | 155.5 ±10.5 |  |
| draw_params ms | 4136.5 ±1.5 | 166.0 ±12.0 | 185.5 ±29.5 |  |
| submit_stress ms | 34087.5 ±1.5 | 30107.5 ±0.5 | 30117.0 ±4.0 |  |
| csf_event ms | 4516.0 ±3.0 | 311.5 ±20.5 | 299.0 ±12.0 |  |
| gs_tess_primitive_id ms | 4306.5 ±13.5 | 258.5 ±16.5 | 241.5 ±11.5 |  |
| bachata_storage_fmtless ms | 4107.5 ±3.5 | 99.0 ±5.0 | 109.0 ±9.0 |  |
| bachata_dynamic_render ms | 4112.0 ±11.0 | 128.0 ±5.0 | 140.0 ±6.0 | worse than 116 |
| dxvk_reqs ms | 2070.5 ±3.5 | 84.5 ±0.5 | 83.0 ±1.0 |  |
| vkd3d_reqs ms | 2065.0 ±1.0 | 82.0 ±3.0 | 87.5 ±3.5 |  |
| d3d_fl ms | 2055.5 ±1.5 | 88.0 ±4.0 | 84.5 ±0.5 |  |
| vkd3d_heap ms | 10365.0 ±1.0 | 372.0 ±16.0 | 364.0 ±26.0 |  |
| vkd3d_timeline ms | 4241.5 ±2.5 | 225.0 ±9.0 | 215.5 ±0.5 |  |
| swapchain_lifecycle ms | 10379.0 ±10.0 | 6384.0 ±7.0 | 6429.0 ±12.0 |  |
| wl bc_level us | 107140.0 ±614.0 | 101939.0 ±1587.0 | 256352.0 ±940.0 | worse than rc4 worse than 116 |
| wl ref us | 12.9 ±2.0 | 10.8 ±1.0 | 13.2 ±0.9 | worse than 116 |
| wl gs_pass us | 53.2 ±2.4 | 41.7 ±0.2 | 14.4 ±0.9 |  |
| wl gs_layer us | 53.9 ±3.2 | 42.8 ±0.0 | 34.1 ±0.8 |  |
| wl xfb_vs us | 464.2 ±0.6 | 461.5 ±0.2 | 87.9 ±2.5 |  |
| wl xfb_gs us | 862.0 ±1.4 | 514.2 ±1.4 | 228.2 ±1.6 |  |
| wl strip_ref us | 17.4 ±1.3 | 18.5 ±0.4 | 17.1 ±1.5 |  |
| wl strip_gs us | 1928.8 ±3.8 | 1186.0 ±0.3 | 17.8 ±1.3 |  |
| wl cond_true us | 18.1 ±0.6 | 16.7 ±1.5 | 18.6 ±0.2 | worse than 116 |
| wl cond_false us | 8.4 ±0.1 | 8.1 ±0.0 | 8.1 ±0.0 |  |
| wl pg_query us | 21.3 ±0.1 | 20.5 ±0.1 | 11.9 ±1.0 |  |
| wl ts_ref us | 722.9 ±3.3 | 729.4 ±9.1 | 700.8 ±32.3 |  |
| wl ts_pair us | 1027.0 ±3.6 | 964.7 ±9.6 | 961.8 ±8.2 |  |
| wl msaa_ref us | 255.7 ±3.8 | 256.1 ±4.2 | 253.0 ±1.3 |  |
| wl msaa_sshade us | 1999.7 ±825.1 | 1177.3 ±1.7 | 1195.2 ±3.7 |  |
| wl tex_rgba us | 260.9 ±9.2 | 248.2 ±1.1 | 247.1 ±3.3 |  |
| wl tex_bgra us | 270.0 ±0.3 | 275.2 ±5.3 | 274.4 ±2.8 |  |
| PSS max MB | 228.5 ±0.7 | 230.1 ±0.7 | 226.4 ±1.9 |  |
| Graphics max MB | 130.7 ±0.0 | 128.8 ±0.1 | 126.7 ±0.0 |  |

### PanProbe TB336FU, warm cache

| metric | rc4 | 116 | 122c | flag |
|---|---|---|---|---|
| gpu_prerast_slice ms | 4079.0 ±3.0 | 145.5 ±23.5 | 146.0 ±25.0 |  |
| clip_cull ms | 5127.5 ±0.5 | 1127.5 ±10.5 | 1119.5 ±3.5 |  |
| multi_viewport ms | 5092.5 ±0.5 | 1091.0 ±4.0 | 1098.5 ±7.5 |  |
| fill_mode ms | 5372.5 ±6.5 | 1200.0 ±32.0 | 1174.0 ±2.0 |  |
| bc_decode ms | 4630.0 ±1.0 | 133.5 ±8.5 | 120.0 ±4.0 |  |
| bc_perf ms | 53849.0 ±214.0 | 46392.0 ±113.0 | 45050.0 ±274.0 |  |
| tess_perf ms | - | 32814.0 ±1326.0 | 34380.0 ±131.0 |  |
| workload_perf ms | 22016.5 ±4.5 | 11425.5 ±101.5 | 9316.5 ±64.5 |  |
| geometry ms | 5516.5 ±8.5 | 1195.0 ±4.0 | 1176.0 ±6.0 |  |
| tessellation ms | 7846.0 ±42.0 | 1836.0 ±3.0 | 1847.0 ±8.0 |  |
| xfb ms | 4325.0 ±6.0 | 200.0 ±4.0 | 210.0 ±10.0 |  |
| pipeline_stats ms | 5198.0 ±9.0 | 1171.5 ±19.5 | 1151.5 ±10.5 |  |
| vertex_stores ms | 4148.5 ±10.5 | 144.0 ±10.0 | 144.0 ±0.0 |  |
| gs_viewport_depth ms | 4245.0 ±3.0 | 309.0 ±1.0 | 283.0 ±15.0 |  |
| vs_viewport_index ms | 4235.0 ±15.0 | 208.5 ±1.5 | 229.0 ±17.0 | worse than 116 |
| depth_bounds ms | 4336.0 ±13.0 | 204.5 ±11.5 | 188.0 ±10.0 |  |
| large_draw ms | - | 10184.5 ±63.5 | 7177.5 ±42.5 |  |
| vmr_secondary ms | 4105.0 ±5.0 | 95.0 ±7.0 | 84.5 ±2.5 |  |
| tess_cond_state ms | 4172.0 ±2.0 | 215.5 ±27.5 | 198.5 ±7.5 |  |
| bachata_reqs ms | 4176.5 ±7.5 | 117.0 ±15.0 | 122.5 ±0.5 |  |
| bachata_exec ms | 4093.5 ±10.5 | 107.0 ±3.0 | 114.5 ±10.5 |  |
| robustness2 ms | 4104.5 ±11.5 | 109.0 ±6.0 | 109.5 ±7.5 |  |
| robust_image_access2 ms | 10515.5 ±19.5 | 6514.0 ±5.0 | 6493.5 ±4.5 |  |
| blend ms | 4101.5 ±1.5 | 100.5 ±8.5 | 100.0 ±7.0 |  |
| occlusion_query ms | 4101.5 ±2.5 | 120.0 ±4.0 | 115.5 ±8.5 |  |
| descriptor_model ms | 4098.0 ±1.0 | 123.5 ±0.5 | 123.5 ±2.5 |  |
| shader_arith ms | 4112.5 ±15.5 | 124.0 ±4.0 | 131.5 ±9.5 |  |
| depth_stencil ms | 4120.0 ±8.0 | 134.5 ±7.5 | 137.0 ±2.0 |  |
| sampler ms | 4117.5 ±15.5 | 125.5 ±0.5 | 123.5 ±2.5 |  |
| draw_params ms | 4138.0 ±17.0 | 179.0 ±8.0 | 192.5 ±2.5 | worse than 116 |
| submit_stress ms | 34090.0 ±1.0 | 30116.0 ±7.0 | 30110.0 ±5.0 |  |
| csf_event ms | 4561.5 ±4.5 | 274.5 ±2.5 | 277.0 ±1.0 |  |
| gs_tess_primitive_id ms | 4314.5 ±2.5 | 213.0 ±3.0 | 220.5 ±15.5 |  |
| bachata_storage_fmtless ms | 4099.0 ±4.0 | 123.0 ±8.0 | 116.0 ±12.0 |  |
| bachata_dynamic_render ms | 4103.0 ±1.0 | 118.0 ±3.0 | 122.0 ±3.0 |  |
| dxvk_reqs ms | 2068.0 ±14.0 | 75.5 ±0.5 | 88.0 ±5.0 | worse than 116 |
| vkd3d_reqs ms | 2070.0 ±4.0 | 83.5 ±4.5 | 90.0 ±2.0 |  |
| d3d_fl ms | 2057.0 ±0.0 | 82.0 ±3.0 | 91.0 ±3.0 | worse than 116 |
| vkd3d_heap ms | 10348.5 ±13.5 | 375.5 ±3.5 | 345.5 ±17.5 |  |
| vkd3d_timeline ms | 4230.0 ±2.0 | 221.5 ±4.5 | 226.0 ±0.0 |  |
| swapchain_lifecycle ms | 10336.5 ±34.5 | 6362.5 ±13.5 | 6357.5 ±26.5 |  |
| wl bc_level us | 108437.5 ±333.5 | 104681.5 ±3394.5 | 257520.5 ±2352.5 | worse than rc4 worse than 116 |
| wl ref us | 13.2 ±0.2 | 12.0 ±3.6 | 13.7 ±2.0 |  |
| wl gs_pass us | 54.8 ±0.1 | 39.8 ±4.2 | 13.9 ±2.0 |  |
| wl gs_layer us | 50.0 ±0.9 | 38.8 ±4.8 | 33.8 ±1.6 |  |
| wl xfb_vs us | 463.9 ±0.4 | 462.8 ±0.4 | 90.4 ±0.1 |  |
| wl xfb_gs us | 864.7 ±0.2 | 511.1 ±3.7 | 236.5 ±1.6 |  |
| wl strip_ref us | 18.5 ±0.2 | 16.6 ±0.2 | 19.0 ±0.1 | worse than 116 |
| wl strip_gs us | 1927.9 ±3.7 | 1183.7 ±0.2 | 18.9 ±0.9 |  |
| wl cond_true us | 18.5 ±0.3 | 17.7 ±0.1 | 19.2 ±0.0 | worse than 116 |
| wl cond_false us | 8.3 ±0.2 | 7.9 ±0.1 | 8.2 ±0.1 |  |
| wl pg_query us | 21.7 ±0.2 | 20.7 ±0.2 | 15.4 ±0.3 |  |
| wl ts_ref us | 736.2 ±3.9 | 731.6 ±14.5 | 723.5 ±4.9 |  |
| wl ts_pair us | 1010.7 ±16.4 | 960.6 ±13.2 | 965.5 ±8.2 |  |
| wl msaa_ref us | 251.7 ±0.4 | 254.3 ±2.7 | 253.0 ±1.1 |  |
| wl msaa_sshade us | 1176.6 ±6.3 | 1190.3 ±1.2 | 1169.4 ±4.0 |  |
| wl tex_rgba us | 254.3 ±7.7 | 247.1 ±0.1 | 245.9 ±1.4 |  |
| wl tex_bgra us | 275.5 ±5.4 | 274.5 ±2.0 | 272.8 ±0.8 |  |
| PSS max MB | 228.4 ±2.4 | 224.3 ±5.5 | 232.6 ±4.3 |  |
| Graphics max MB | 129.7 ±1.0 | 125.0 ±1.8 | 129.0 ±0.3 |  |

### PanPlay TB336FU (fps, 1 % low, max PSS / Graphics MB per scenario), 122c = 0160 uncached readback

| metric | rc4 | 116 | 122c | flag |
|---|---|---|---|---|
| s1 01 d3d11 default fps | 155.3 | 152.9 | 159.7 ±0.4 |  |
| s1 01 d3d11 default 1%low | 80.7 | 84.8 | 81.6 ±2.0 |  |
| s2 01 d3d9 default fps | 90.0 | 89.9 | 90.0 ±0.0 |  |
| s2 01 d3d9 default 1%low | 50.9 | 47.1 | 50.3 ±1.1 |  |
| s3 01 d3d11 default fps | 157.4 | 159.5 | 161.2 ±4.1 |  |
| s3 01 d3d11 default 1%low | 86.9 | 92.9 | 90.6 ±0.4 |  |
| s3 02 d3d9 default fps | 90.0 | 90.0 | 90.0 ±0.0 |  |
| s3 02 d3d9 default 1%low | 52.7 | 48.2 | 52.2 ±0.4 |  |
| s4 01 d3d11 old fps | 144.1 | 159.5 | 161.3 ±2.9 |  |
| s4 01 d3d11 old 1%low | 39.7 | 88.4 | 84.8 ±0.4 |  |
| s4 02 d3d11 default fps | 157.3 | 161.1 | 161.2 ±2.1 |  |
| s4 02 d3d11 default 1%low | 83.2 | 91.9 | 85.3 ±1.1 | worse than 116 |
| s4 03 d3d9 old fps | 89.9 | 89.9 | 89.9 ±0.0 |  |
| s4 03 d3d9 old 1%low | 49.8 | 50.4 | 50.3 ±0.4 |  |
| s4 04 d3d9 default fps | 90.0 | 89.9 | 90.0 ±0.0 |  |
| s4 04 d3d9 default 1%low | 49.3 | 46.4 | 50.3 ±1.7 |  |
| s8 01 d3d11fl_10_0 default fps | 194.4 | 208.4 | 199.8 ±0.3 |  |
| s8 01 d3d11fl_10_0 default 1%low | 97.4 | 107.8 | 100.8 ±0.1 | worse than 116 |
| s8 02 d3d11fl_10_1 default fps | 201.9 | 212.9 | 201.8 ±0.3 | worse than 116 |
| s8 02 d3d11fl_10_1 default 1%low | 102.7 | 104.7 | 103.3 ±1.1 |  |
| s8 03 d3d11fl_11_0 default fps | 194.4 | 212.8 | 200.9 ±0.2 | worse than 116 |
| s8 03 d3d11fl_11_0 default 1%low | 99.8 | 109.4 | 99.2 ±2.6 | worse than 116 |
| s8 04 d3d11fl_11_1 default fps | 194.0 | 202.2 | 206.1 ±1.5 |  |
| s8 04 d3d11fl_11_1 default 1%low | 98.2 | 104.8 | 94.3 ±2.1 | worse than 116 |
| s8 05 d3d10 default fps | 158.7 | 162.4 | 162.5 ±2.8 |  |
| s8 05 d3d10 default 1%low | 80.2 | 96.1 | 87.0 ±6.2 | worse than 116 |
| s8 06 d3d9 default fps | 89.9 | 90.0 | 89.8 ±0.1 |  |
| s8 06 d3d9 default 1%low | 50.3 | 47.6 | 48.0 ±1.1 |  |
| s9 02 d3d11fl_all default fps | 197.1 | 212.3 | 200.0 ±1.3 | worse than 116 |
| s9 02 d3d11fl_all default 1%low | 101.3 | 109.5 | 97.8 ±2.2 | worse than 116 |
| s1 PSS MB | 317.4 | 339.1 | 345.6 ±9.3 | worse than rc4 |
| s1 Graphics MB | 174.4 | 187.1 | 195.1 ±8.0 | worse than rc4 |
| s2 PSS MB | 350.6 | 389.6 | 380.7 ±8.4 | worse than rc4 |
| s2 Graphics MB | 196.8 | 212.2 | 213.2 ±0.9 | worse than rc4 |
| s3 PSS MB | 358.7 | 366.9 | 371.9 ±7.9 |  |
| s3 Graphics MB | 193.0 | 205.3 | 204.3 ±1.0 | worse than rc4 |
| s4 PSS MB | 368.2 | 414.8 | 389.7 ±16.3 | worse than rc4 |
| s4 Graphics MB | 209.0 | 218.1 | 213.1 ±1.1 |  |
| s7 PSS MB | 323.3 | 371.8 | 371.8 ±0.8 | worse than rc4 |
| s7 Graphics MB | 168.7 | 209.0 | 208.9 ±0.1 | worse than rc4 |
| s8 PSS MB | 365.6 | 387.0 | 389.0 ±5.2 | worse than rc4 |
| s8 Graphics MB | 193.0 | 214.3 | 210.5 ±1.6 | worse than rc4 |
| s9 PSS MB | 324.3 | 377.0 | 374.7 ±0.6 | worse than rc4 |
| s9 Graphics MB | 168.7 | 209.0 | 208.9 ±0.1 | worse than rc4 |
| s5 01 d3d12 default fps | - | 22.9 | 23.9 ±1.0 |  |
| s5 01 d3d12 default 1%low | - | 22.4 | 23.3 ±1.0 |  |
| s6 01 d3d12gears default fps | - | 261.0 | 221.1 ±1.9 | worse than 116 |
| s6 01 d3d12gears default 1%low | - | 244.2 | 208.3 ±4.5 | worse than 116 |
| s7 01 d3d12fl_11_0 default fps | - | 426.4 | 483.1 ±23.2 |  |
| s7 01 d3d12fl_11_0 default 1%low | - | 402.9 | 413.7 ±16.5 |  |
| s7 02 d3d12fl_11_1 default fps | - | 434.9 | 503.8 ±3.9 |  |
| s7 02 d3d12fl_11_1 default 1%low | - | 407.5 | 419.4 ±19.5 |  |
| s7 03 d3d12fl_12_0 default fps | - | 429.6 | 483.1 ±0.4 |  |
| s7 03 d3d12fl_12_0 default 1%low | - | 410.5 | 397.3 ±20.5 |  |
| s7 04 d3d12fl_12_1 default fps | - | 424.5 | 495.1 ±8.2 |  |
| s7 04 d3d12fl_12_1 default 1%low | - | 391.6 | 426.5 ±1.7 |  |
| s7 05 d3d12fl_12_2 default fps | - | 432.7 | 481.0 ±24.3 |  |
| s7 05 d3d12fl_12_2 default 1%low | - | 416.2 | 417.4 ±20.0 |  |
| s9 01 d3d12fl_all default fps | - | 436.1 | 485.7 ±11.9 |  |
| s9 01 d3d12fl_all default 1%low | - | 418.2 | 407.8 ±16.8 |  |
| s5 PSS MB | - | 371.3 | 378.1 ±10.4 |  |
| s5 Graphics MB | - | 212.3 | 212.3 ±0.0 |  |
| s6 PSS MB | - | 361.0 | 359.8 ±0.8 |  |
| s6 Graphics MB | - | 205.3 | 205.3 ±0.0 |  |

rc4 has no D3D12 results (s5-s7, s9 D3D12 do not run on rc4).

### 0160 on v9: the TB336FU .so bisect

Method: PanPlay `installDrivers` with one env set per build and no env, no APK reinstall.
Kit: `worklogs/g615-dxvk/device-runs/2026-10-30-v14-so-bisect-kit/`, with `CFG=bisect-v9.json SC=6`.

| build | 116 | 117 | 118 | 119 | 120 | 121b | 122c | 122d |
|---|---|---|---|---|---|---|---|---|
| D3D12 gears fps | 248.0 | 250.8 | 257.4 | 259.2 | 219.6 | 219.7 | 222.9 | 218.4 |
| D3D11 FL 11_0 fps | 212.3 | 210.5 | 211.0 | 214.0 | 196.2 | 199.0 | 200.0 | 196.5 |

On v9, 120 adds only 0160, the uncached sw readback.
- That explains every "worse than 116" PanPlay fps and 1 % low row above: gears -15 %, and the D3D11 FL / D3D10 / s4 rows -5 to -8 %.
- PANVK_GPU_STATS is not the cause: 122d with and without it gave gears 221 / 222 and FL 11_0 201 / 195.
- 0169 makes v9 read cached again, which is the 116-119 behaviour.
- The direct A/B of 122d default vs `PANVK_WSI_SW_CACHED=1` was cut off when the TB336FU dropped off USB, so the 122e PanPlay rerun on v9 is still open.

### Regressions vs rc4 / 116 on v9, cause and status

1. **PanPlay fps / 1 % low vs 116** (gears, D3D11 FL, D3D10).
   - Cause: 0160 (bisect above).
   - Fixed by 0169 in 122e. Not yet re-measured on 122e.
2. **`bc_level` 256 ms per format vs rc4 107 / 116 102.** Cold BC level load, shader cache disabled by the test.
   - The time is all recording (`record_ms` ~1450 of ~1530 ms): the recording waits for the BC transcoder pipeline compiles.
   - Cause: 0153 (one-block-per-invocation decoders) or 0155/0162 (prewarm + XC_BATCH pipeline); not separated yet.
   - The `PANVK_BC_PREWARM=0` / `PANVK_BC_LM_BATCH=0` A/B was interrupted by the USB drop: only the 122d default run finished (306 ms per op).
   - Open.
3. **`bc_decode` cold 1618 ms vs 116 620 ms** (warm 120 vs 134, so warm is better).
   - Same compile cost as item 2. Open.
4. **PanPlay memory vs rc4: PSS +20..+50 MB, Graphics +13..+40 MB** (s1, s2, s4, s7, s8, s9).
   - 116 has the same memory as 122c (within run noise), so this predates 116. It is not from 0117-0169.
   - Not investigated in this round. Open.
5. Small flags vs 116 under 20 ms on 100-250 ms PanProbe tests:
   - cold: `bachata_dynamic_render`
   - warm: `vs_viewport_index`, `draw_params`, `dxvk_reqs`, `d3d_fl`
   - `workload_perf`: `ref`, `strip_ref`, `cond_true`, each about 1-2 us
   - One extra run each would settle whether these are noise. Not fixed.

PanProbe memory (max PSS / Graphics over the run) is equal to rc4 and 116 within run noise, cold and warm.

### X6 Pro v11 and the WSI A/B

The X6 Pro dropped off wireless adb at about 00:15, during the WSI A/B. Its gate runs (rc4 / 116 / 122e PanProbe +
PanPlay, FO4 smoke test) did not run.

Round 1 of the A/B on a 122d-dev build (scenario s1 cube, s8 D3D11 FL):

| mode | cube fps | FL 10_0 | FL 10_1 | FL 11_0 | FL 11_1 | D3D10 |
|---|---|---|---|---|---|---|
| uncached (`=0`) | 270.5 | 814.6 | 793.4 | 813.5 | 771.7 | 273.9 |
| cached (`=1`) | 339.0 | - | - | - | - | - |
| timing-based auto (dropped) | 288.1 | 466.9 | 473.5 | 453.9 | 492.5 | 332.4 |

So on v11 the cube is faster cached and FL is faster uncached. 0169 keeps v11 uncached.

The cached FL loss is not in the CPU read. A timing of an invalidate + frame read at the first swapchain chose cached here, which is the wrong pick for FL.

### MangoHud VRAM on JM (0166, from 122b)

TB336FU PanPlay s1-s9 `vram` now reads 56-395 MiB. With 122 it read 1349-1684 MiB, because it counted the full size of the grow-on-fault BOs.
PanPlay 1-9 on 122b and 122c pass with no DEVICE_LOST. PanProbe 41/41 on 122b and 122c, cold and warm.

## Build 122e gate (2026-10-30): rc4 vs 116 vs 122e, X6 Pro v11, TB336FU v9, RTL v14

Raw data and full tables: `worklogs/g615-dxvk/device-runs/2026-10-30-local-122e-gate/`.
- X6 Pro: PanProbe 2 cold + 2 warm runs per build, alternating. PanPlay one full 1-9 pass per build.
- TB336FU: PanPlay rc4c and 116c once each, 122e twice.
- Battery 28-31 C before every run. The same flag rule applies as above.

### Summary

| check | X6 Pro v11 | TB336FU v9 | RTL v14 SM-X946B |
|---|---|---|---|
| PanProbe pass | 41/41 x4 | (122c run holds, see above) | 41/41 |
| D3D11 cube fps vs rc4 / 116 | 276 vs 279-294 / 312-333 (worse) | 156-160 vs 162-163 / 156-162 (same) | 707-823, same as 116 (747-771) in one session |
| D3D11 FL fps vs rc4 / 116 | 771-801 vs 459-620 / 404-443 (better) | 214-218 vs 197-202 / 212-218 (same) | - |
| D3D12 vs 116 | better or same (gears 236 vs 181) | same (s5 23 vs 25) | - |
| D3D9 | same | same | - |
| PanPlay PSS / Graphics vs rc4 | +0..+20 MB s1/s8 | +10..+50 MB / +13..+40 MB | - |
| PanPlay PSS / Graphics vs 116 | same | same | - |
| PanProbe memory | same or lower | same | - |
| FO4 smoke (sound) | in game, 34 fps, audio pk -12..-23 dB, no DEVICE_LOST | - | - |

On v9, 0169 restores the 116 PanPlay fps. The 0160 rows (gears -15 %, FL -5..-8 %) are gone.

Firebase PanProbe 122e: starlte v6, a31 v7 and panther v10 all 41/41, no DEVICE_LOST (`2026-10-30-ftl-v6-v7-v10-122e-panprobe`).

### X6 Pro v11: the sw WSI readback is a per-content tradeoff

Measured on 122e, 2 rounds, alternating modes, with `PANVK_WSI_SW_CACHED`:

| mode | cube s1 | FL 10_0 | FL 10_1 | FL 11_0 | FL 11_1 | D3D10 |
|---|---|---|---|---|---|---|
| auto (122e default = uncached on v11) | 274 | 797 | 781 | 775 | 735 | 271 |
| uncached `=0` | 271 | 775 | 754 | 751 | 751 | 276 |
| cached `=1` | 354 | 504 | 501 | 480 | 474 | 367 |

- Cached reads make the cube and D3D10 30 % faster, but FL 35 % slower. Neither default beats 116 (cached) and 122e on both.
- To fix it, the choice has to be made per swapchain or per content, not per arch. It is not fixed in 122e.

### Regressions left, cause and status

1. **v11 cube / D3D10 vs 116**: the WSI readback tradeoff above. Open.
2. **v14 cube vs 116** (RTL, cached everywhere: 116 863, 120 765, 122e 798):
   - The cause is in 0152-0159.
   - Code read: 0152 is v9 only, 0153/0155 are BC only, 0154 is the string only, 0158 is inert without MangoHud, and 0159 removes a flush.
   - Suspect .so builds 119r0155..119r0158 were made for the bisect kit.
   - **RTL session 3 (same SM-X946B), result: no regression; the gap is run-to-run noise.**
     - Control run: by accident, all 10 passes ran the bundled 122e (on a fresh install the app could not read the shell-created `bisect/` dir). Same driver: 678-884 fps.
     - Real round (`bisect-v14t.json`, CACHED=1):

       | 116 | 118 | 119 | 119r0156 | 119r0158 | 119r0157 | 119 GS_PT_SKIP=0 | 119r0155 | 119 BC_PREWARM=0 | 116 again |
       |---|---|---|---|---|---|---|---|---|---|
       | 771 | 767 | 762 | 730 | 721 | 823 | 707 | 730 | 763 | 747 |

     - 116 and 119 are within 1 %. Pass 6 is the fastest pass in both runs, whichever driver it runs, so run order matters more than the driver.
     - The earlier single-pass bisect (116 863 / 120 765) was inside this noise.
     - The reversed-order round 2 and the full PanPlay did not run: the RTL session ended.
   - Raw data: `worklogs/g615-dxvk/device-runs/2026-10-30-rtl-v14-sm-x946b-suspects/`.
3. **`bc_level` cold and `bc_decode` cold** (shader cache disabled by the test):
   - TB v9: 122e 273-295 ms per op vs 116 138.
   - `PANVK_BC_PREWARM=0` gives 287-295, `PANVK_BC_LM_BATCH=0` 283, and both off 281.
   - So the cost is 0153 (bigger one-block-per-invocation decoders), not prewarm or batching.
   - Tradeoff: runtime decode is 2.7-5x faster, and warm `bc_decode` is better than 116.
   - X6 v11: 122e 450 ms vs 116 790 and rc4 46. The rc4 gap predates 116.
4. **PanPlay memory vs rc4** (TB +10..+50 MB PSS, +13..+40 MB Graphics): 116 is the same as 122e, so the cause predates 116. Open.
5. **`workload_perf` `xfb_vs` on v11**:
   - Gate runs: 217 us vs 116 170 and rc4 130.
   - Isolated reruns: 180-192 us. With `PANVK_KBASE_TILER_IR=0` it was 228-275, so 0167 is not the cause.
   - Noisy, under 0.1 ms per op. Open.
6. Small flags under 20 ms on 75-140 ms PanProbe tests vs 116 (warm `vmr_secondary`, `robustness2`, `blend`, `sampler`). These are within the spread of other runs.
