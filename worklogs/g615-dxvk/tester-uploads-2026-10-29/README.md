# Tester uploads, rc4 era (2026-10-29)

Source: `panvk-upload` Cloudflare Worker, D1 `panvk-uploads`, blobs in KV `BLOBS`. Read-only (SELECT and blob GET). Raw ZIPs are not in git (kept in `/var/tmp/panvk/cloud-uploads-2026-10-29/`).

## Cutoff and counts

- Previous cutoff: D1 id 884 (2026-10-16), the rc3 / rc3-lowmem report in [PROGRESS.md](../PROGRESS.md). DEVICES.md was last refreshed 2026-10-06.
- Range queried: ids 885 to 1402 (2026-10-16 06:05Z to 2026-10-28 18:24Z): 518 new rows (PanProbe 1.2.7 322, PanPlay 1.2.7 128, others older/newer app builds).
- Driver split by `driver_so_sha256`: rc3 and rc3-lowmem 169, rc4 family 191, newer or unrelated 158 (of which 58 are developer RC5-dev / `26.3.0-devel` Firebase Test Lab runs, the rest vendor drivers and older imports).
- Analysed: 249 rows (rc4 family 191 + 58 newer dev builds). Downloaded and sha256-verified 241; 8 not fetchable (no KV or Catbox link, Gofile only). 183 rc4-family records read: 77 device models.
- rc4 identification: tag `panvk-kbase-v0.1.0-rc4`, Mesa `5a07217f03`, `.so` sha256 `977c0c4c` (BuildID `6654fada`), driverInfo `PanVK-kbase RC4`. rc4-lowmem: tag `panvk-kbase-v0.1.0-rc4-lowmem`, original `.so` `595cf591`, binary-patched default-lowmem `.so` `c395db09` (BuildID `aff7a07f`). Counts: rc4 23, rc4-lowmem patched 160, rc4-lowmem original 8. Some rc4 `.so` uploads carry a stale `driverVersion` string ("RC3 lowmem ...") from the launcher manifest; the sha is what was matched.
- No gpu_id is missing from the model table: all 20 reported ids resolve to a model name in the logs (G72, G76, G52 r1, G77, G57, G78, G68, G710, G610, G715, G615, G720, G725/G925, G1-Ultra). Still never seen: G510, G310, G620, G625, plain G725, G1-Premium, G1-Pro.

## Per-device results (rc4 family)

Arch totals (models): see [DEVICES.md](../../../docs/universal/DEVICES.md#rc4-and-rc4-lowmem-uploads-snapshot-2026-10-29), which has the full table with kernel and kbase. Summary:

- v10 to v14 CSF, rc4 (non-lowmem): PanProbe 38/38 on all 16 full runs (V2413 G925: 17/17 on a shorter test list); all 17 `bc_decode` pass.
- v10 to v14 CSF, rc4-lowmem (patched): 37/38 on nearly all runs, the single failure is `bc_decode` (F1).
- v9 JM (G57, G68, G78, G77): 38/38 on all complete runs except `swapchain_lifecycle` ("no surface") on two stock devices.
- v6 (G72) and v7 (G76, G52): 11-16 of 38, feature gaps by design plus JM job faults (F4).

| Model | gpu_id | Arch | rc4 runs (probe/play) | Builds | PanProbe best | Failing tests (count) | PanPlay |
|---|---|---|---|---|---|---|---|
| SM-A515F | `0x62210010` | v6 | 2/6 | rc4-lowmem, rc4-lowmem-orig | 11/38 | clip_cull (2), multi_viewport (2), fill_mode (2), bc_perf (2) | 6 sessions, exit {'None': 3, '5': 1, '3': 1, '1': 1}: Cube · D3D11 · ARM64EC, Cube · D3D8 · x86, Cube · D3D9 · ARM64EC |
| SM-P615 | `0x62210010` | v6 | 2/0 | rc4-lowmem | 12/38 | clip_cull (2), multi_viewport (2), fill_mode (2), bc_perf (2) | - |
| 2207117BPG | `0x72110000` | v7 | 1/0 | rc4-lowmem | 14/38 | clip_cull (1), multi_viewport (1), fill_mode (1), bc_perf (1) | - |
| ANA-NX9 | `0x72110000` | v7 | 1/0 | rc4-lowmem | 13/38 | clip_cull (1), multi_viewport (1), fill_mode (1), bc_perf (1) | - |
| CPH2579 | `0x74021000` | v7 | 1/0 | rc4 | 14/38 | clip_cull (1), multi_viewport (1), fill_mode (1), bc_perf (1) | - |
| HiPad-Air | `0x74021000` | v7 | 7/0 | rc4-lowmem | 14/38 | clip_cull (7), multi_viewport (7), fill_mode (7), bc_perf (7) | - |
| NLA-LX3 | `0x74021000` | v7 | 1/0 | rc4-lowmem | 14/38 | clip_cull (1), multi_viewport (1), fill_mode (1), bc_perf (1) | - |
| SM-A065F | `0x74021000` | v7 | 1/0 | rc4 | 14/38 | clip_cull (1), multi_viewport (1), fill_mode (1), bc_perf (1) | - |
| 21081111RG | `0x90800011` | v9 | 1/1 | rc4-lowmem-orig | 38/38 | - | 1 sessions, exit {'1': 1}: Cube · D3D8 · x86 |
| 2201117PG | `0x90930010` | v9 | 0/6 | rc4, rc4-lowmem | No run | - | 6 sessions, exit {'0': 5, '137': 1}: Cube · D3D11 · ARM64EC, Cube · D3D11 · x64, Cube · D3D8 · ARM64EC |
| 22101316G | `0x92041010` | v9 | 0/2 | rc4-lowmem | No run | - | 2 sessions, exit {'None': 2}: EnderLilies |
| 22101316UP | `0x92041010` | v9 | 0/1 | rc4-lowmem | No run | - | 1 sessions, exit {'137': 1}: DARKSOULS |
| 23117RA68G | `0x90930010` | v9 | 1/0 | rc4-lowmem | 37/38 | swapchain_lifecycle (1) | - |
| 2312FPCA6G | `0x90930010` | v9 | 1/0 | rc4-lowmem | 37/38 | swapchain_lifecycle (1) | - |
| 2510DRA23E | `0x90930010` | v9 | 1/0 | rc4-lowmem | 38/38 | - | - |
| CPH2481 | `0x90930010` | v9 | 0/1 | rc4-lowmem | No run | - | 1 sessions, exit {'137': 1}: steamsetup.exe |
| Infinix-X6882 | `0x9093` | v0 | 1/0 | rc4-lowmem | 37/38 | swapchain_lifecycle (1) | - |
| LineageOS GSI on ARM64 | `0x90910010` | v9 | 3/0 | rc4-lowmem | 38/38 | - | - |
| M2103K19C | `0x90930010` | v9 | 1/0 | rc4-lowmem | 38/38 | - | - |
| Pixel 6 | `0x92020010` | v9 | 3/1 | rc4-lowmem | 38/38 | - | 1 sessions, exit {'137': 1}: PEAK |
| RG Slide | `0x90910010` | v9 | 2/0 | rc4-lowmem | 38/38 | - | - |
| Retroid Pocket 4 Pro | `0x9080` | v0 | 2/0 | rc4-lowmem | 38/38 | - | - |
| SM-A075F | `0x90930010` | v9 | 0/1 | rc4-lowmem | No run | - | 1 sessions, exit {'137': 1}: Crash |
| SM-A155F | `0x90930010` | v9 | 2/0 | rc4-lowmem | 38/38 | - | - |
| SM-A155M | `0x90930010` | v9 | 1/0 | rc4-lowmem | 38/38 | - | - |
| SM-A166M | `0x92041010` | v9 | 1/0 | rc4-lowmem | 38/38 | - | - |
| SM-S901B | `0x92020010` | v9 | 1/0 | rc4-lowmem | 38/38 | - | - |
| TECNO KL7 | `0x90930010` | v9 | 1/0 | rc4-lowmem | 38/38 | - | - |
| TrebleDroid with GApps | `0x90800011` | v9 | 1/0 | rc4-lowmem | 38/38 | - | - |
| Z2357N | `0x90910010` | v9 | 1/0 | rc4-lowmem | 38/38 | - | - |
| fleur | `0x90930010` | v9 | 2/0 | rc4-lowmem | 38/38 | large_draw (1) | - |
| moto g - 2025 | `0x90930010` | v9 | 1/0 | rc4-lowmem | 38/38 | - | - |
| 22041216I | `0xa8670000` | v10 | 1/0 | rc4-lowmem | 37/38 | bc_decode (1) | - |
| 2306EPN60G | `0xa8670004` | v10 | 4/0 | rc4, rc4-lowmem | 38/38 | bc_decode (2) | - |
| A142P | `0xa8670004` | v10 | 1/0 | rc4-lowmem | 37/38 | bc_decode (1) | - |
| I2301 | `0xa8670000` | v10 | 2/0 | rc4-lowmem | 37/38 | bc_decode (2) | - |
| Infinix X6861 | `0xa8670000` | v10 | 1/0 | rc4-lowmem | 37/38 | bc_decode (1) | - |
| Infinix X6871 | `0xa8670000` | v10 | 0/1 | rc4-lowmem | No run | - | 1 sessions, exit {'137': 1}: Castlevania - Lords of S |
| 2311DRK48G | `0xb8a31030` | v11 | 6/0 | rc4-lowmem | 37/38 | bc_decode (6), swapchain_lifecycle (1) | - |
| 2311DRK48I | `0xb8a31030` | v11 | 7/0 | rc4, rc4-lowmem, rc4-lowmem-orig | 38/38 | bc_decode (4) | - |
| 24090RA29I | `0xb8a31030` | v11 | 2/0 | rc4-lowmem | 37/38 | bc_decode (2) | - |
| 25080RABDG | `0xb8a31030` | v11 | 1/0 | rc4-lowmem | 37/38 | bc_decode (1) | - |
| A001 | `0xb8a31030` | v11 | 1/0 | rc4-lowmem | 37/38 | bc_decode (1) | - |
| A015 | `0xb8a31030` | v11 | 0/1 | rc4-lowmem | No run | - | 1 sessions, exit {'None': 1}: NFS14 |
| CPH2625 | `0xb8a31030` | v11 | 0/1 | rc4-lowmem | No run | - | 1 sessions, exit {'1': 1}: pes2017 |
| CPH2689 | `0xb8a31030` | v11 | 1/0 | rc4-lowmem | 36/38 | bc_decode (1), depth_bounds (1) | - |
| Infinix X6870 | `0xb8a31030` | v11 | 2/2 | rc4-lowmem | 37/38 | bc_decode (2) | 2 sessions, exit {'None': 2}: GTAIV |
| Infinix X6873 | `0xb8a31030` | v11 | 4/7 | rc4, rc4-lowmem | 38/38 | bc_decode (3) | 7 sessions, exit {'137': 7}: NFS Underground |
| Pixel 9 Pro XL | `0xb8a21030` | v11 | 2/0 | rc4-lowmem | 37/38 | bc_decode (2) | - |
| RG 477V | `0xb8a31030` | v11 | 1/0 | rc4-lowmem | 37/38 | bc_decode (1) | - |
| TB373FU | `0xb8a31030` | v11 | 1/0 | rc4-lowmem | 37/38 | bc_decode (1) | - |
| TB375FC | `0xb8a31030` | v11 | 2/0 | rc4-lowmem | 37/38 | bc_decode (2) | - |
| TECNO LJ9 | `0xb8a31030` | v11 | 0/1 | rc4-lowmem | No run | - | 1 sessions, exit {'0': 1}: Cube · D3D9 · x64 |
| motorola edge 60 pro | `0xb8a31030` | v11 | 1/0 | rc4-lowmem | 37/38 | bc_decode (1) | - |
| 2407FPN8EG | `0xc8700000` | v12 | 4/0 | rc4-lowmem | 37/38 | bc_decode (4) | - |
| 24129RT7CC | `0xc8700010` | v12 | 0/2 | rc4-lowmem | No run | - | 2 sessions, exit {'None': 1, '3': 1}: NFS11, Sonic Forces |
| 2412DPC0AG | `0xc8700010` | v12 | 4/1 | rc4-lowmem | 37/38 | bc_decode (4) | 1 sessions, exit {'0': 1}: Cube · D3D11 · ARM64EC |
| 2412DPC0AI | `0xc8700010` | v12 | 1/0 | rc4 | 38/38 | - | - |
| 2511FPC34G | `0xc8700010` | v12 | 6/3 | rc4, rc4-lowmem | 38/38 | bc_decode (4) | 3 sessions, exit {'None': 1, '10': 1, '84': 1}: FCPrimal, bf1, re7 |
| 2606FRT34I | `0xc8700010` | v12 | 1/0 | rc4-lowmem | single test only | - | - |
| CPH2739 | `0xc8700010` | v12 | 1/0 | rc4-lowmem | 35/38 | bc_decode (1), large_draw (1), vkd3d_heap (1) | - |
| CPH2761 | `0xc8700010` | v12 | 0/4 | rc4-lowmem | No run | - | 4 sessions, exit {'0': 1, '137': 3}: Cube · D3D11 · ARM64EC, Cube · D3D11 · x64, Cube · D3D11 · x86 |
| RMX5060 | `0xc8700000` | v12 | 3/0 | rc4, rc4-lowmem | 38/38 | bc_decode (1) | - |
| SM-X820 | `0xc8700000` | v12 | 2/1 | rc4, rc4-lowmem | 38/38 | bc_decode (1) | 1 sessions, exit {'1': 1}: Deep Sleep 2 |
| motorola edge 70 pro | `0xc8700010` | v12 | 2/0 | rc4-lowmem | 37/38 | bc_decode (2) | - |
| 25032RP42C | `0xd8300015` | v13 | 1/0 | rc4-lowmem | 38/38 | - | - |
| 2506BPN68G | `0xd8300015` | v13 | 2/0 | rc4 | 38/38 | - | - |
| 25079RPDCG | `0xd8300015` | v13 | 1/0 | rc4-lowmem | 37/38 | bc_decode (1) | - |
| 2602BPC18G | `0xd8300015` | v13 | 6/1 | rc4, rc4-lowmem | 38/38 | bc_decode (5) | 1 sessions, exit {'137': 1}: SteamSetup |
| SM-X736B | `0xd8300015` | v13 | 4/0 | rc4, rc4-lowmem | 38/38 | bc_decode (3) | - |
| SM-X736N | `0xd8300015` | v13 | 0/1 | rc4-lowmem | No run | - | 1 sessions, exit {'0': 1}: Cube · D3D11 · ARM64EC |
| SM-X930 | `0xd8300015` | v13 | 1/0 | rc4 | 38/38 | - | - |
| V2413 | `0xd8300015` | v13 | 2/0 | rc4 | single test only | - | - |
| V2419A | `0xd8300015` | v13 | 0/1 | rc4-lowmem | No run | - | 1 sessions, exit {'137': 1}: SPEED2 |
| 2602EPTC0G | `0xe8800010` | v14 | 6/0 | rc4-lowmem | 38/38 | bc_decode (5) | - |
| CPH2791 | `0xe8800010` | v14 | 1/0 | rc4-lowmem | 37/38 | bc_decode (1) | - |
| V2515 | `0xe8800010` | v14 | 2/4 | rc4-lowmem | 36/38 | bc_decode (1), swapchain_lifecycle (1) | 4 sessions, exit {'0': 4}: Cube · D3D11 · x64, Cube · D3D11 · x86, Cube · D3D8 · x64 |

## Findings and issues for rc4

Evidence uses the first 8 characters of the upload sha256. "115c" is the Build 115b/115c series (jm-v7-wip 0132-0139, [README](../../../patches/jm-v7-wip/README.md)).

1. **rc4-lowmem `bc_decode` fails raw and copy on every CSF device.** Symptom: `CASE <fmt> raw=FAIL copy=FAIL blit=PASS`, `BC_DEVICE_FAILS=32` (all 16 BC formats, both paths). Devices/arch: v10 (7), v11 (27), v12 (18), v13 (9), v14 (7) of 68 failing runs, 20+ models (G610, G615, G720, G925, G1-Ultra). Evidence: `f7a7da47` (22041216I), `908684cb`, `7dfd0c04` (2606FRT34I), log header `panvk: BC emulation on, lowmem (native compressed mask 0xc7fe001e)`. The only passes under `c395db09` ran with BC `on` or `off` (not lowmem: `2602EPTC0G` ... 3 runs). rc4 (`977c0c4c`) passes 17/17, rc4-lowmem-original `595cf591` passes (only 1 run). Area: lowmem BC path (csf-v11/185 AFRC 8 bpp shadow) raw/copy readback, or the binary patch that makes lowmem the default (`cbz x0` at `0xe91c68`). Was this present in rc3-lowmem: not checked. 115c: not covered (115b/115c do not touch BC). Impact: DXVK BC textures use the blit path (passes), so games may be fine; Vulkan CTS/BC copy users would see wrong data. Priority: high, it is the whole PanProbe score of the lowmem release.
2. **bf1 (Battlefield 1, D3D11) on G720 MC8 (2511FPC34G): CSF fatal exception, DEVICE_LOST.** 2511FPC34G (v12, rc4-lowmem), `908684cb`: `kbase: CSF group 1 fatal error: status 0x010003c1 (exception 0xc1)`, then `timeout snapshot subqueue 1/2 ... seqno 50499, target 50513`, DXVK `vkWaitForPresentKHR failed: VK_ERROR_DEVICE_LOST` after 426 s. Exception 0xc1 reads as a translation fault (not confirmed). Area: CSF memory (a freed or unmapped BO still referenced, tiler/heap or sparse). 115c: not covered (0136 is a tiler OOM of a different kind and exception code). Needs a repro on a G720 with bf1; ask for `subqueue` context.
3. **G77 (POCO 21061110AG family, MT6891, 4.14): D3D8 x86 cube fails with `VK_ERROR_MEMORY_MAP_FAILED`.** `eaf4c5ae` (21081111RG, v9, rc4-lowmem-original): logcat `kbase: placed map of a non-dma-buf BO is not supported`, DXVK `Failed to map Vulkan memory`. Same device passes PanProbe 38/38 (G77 `0x90800011` now enumerates, the jm-v9/006 row works), so only the WoW64 32-bit placed map breaks. 115c: likely yes, 0133 (gralloc BLOB host-visible BOs without a dma-heap on MediaTek 4.14) targets exactly this; needs a G77 retest on a post-115b build.
4. **v6 and v7 (G72, G76, G52) on rc4: JM job faults and missing features.** SM-A515F/SM-P615 (v6) 11-12/38, G52/G76 (v7) 13-16/38. Failing tests: CreateDevice `r=-8`/`-7` (`geometry`, `clip_cull`, `multi_viewport`, `fill_mode`, `pipeline_stats`, `tess_cond_state`), `tessellationShader not exposed`, `VK_EXT_transform_feedback not exposed`, `required feature missing`, plus real faults: `occlusion_query` and `bc_perf` `JM atom failed: event=0x4` and `0x42` (v7 12/12 runs, v6 4/4), `shader_arith` and `sampler` `event=0x51` (INSTR_INVALID_ENC, v6 4/4). Evidence: `2fe6aad7` (SM-A515F), `cd5415a6` (2207117BPG v7). 115c: events `0x4` is 0135 (draw without vertex buffers wrote the descriptor copy to address 0), `0x51` is 0139 (ST_CVT descriptor in the FMA slot), `0x42` plausibly 0132/0135. Support: the developer's RC5-dev builds pass 39/39 on the same GPU families (SM-G960F v6 10 of 13 runs, SM-A315N v7 8 of 9 runs). The missing-feature rows are not bugs in rc4, those features arrive with jm-v7-wip. Action: v6/v7 testers need an RC5 build, rc4 is a feature-limited preview there.
5. **PanProbe `large_draw` TIMEOUT and `vkd3d_heap` FAIL on a G720 (CPH2739, v12, rc4-lowmem).** `69e0e000`: 35/38, `large_draw` runs 62 s after one passing case, the log is full of shader disassembly (`MOV.i32 r7, r5^ ...`), so Mesa shader tracing was on (same harness issue as the old stock G615 item 30). `vkd3d_heap` FAIL after 38 s. The same `vkd3d_heap` failure appears on developer G615 dev builds (`ICD watchdog timeout (12 s)` for `image_heap` and `mutable_heap` at 1,000,000 heaps). Area: harness under logging, plus slow 1M-descriptor heap setup. 115c: not covered.
6. **`depth_bounds` perf gate fails on one G615 phone (CPH2689, v11, rc4-lowmem).** `03d656c8`: `FAIL case I_perf_dyn_off plain=2.188ms dyn_off=6.375ms ratio=2.91`; every correctness case passes. Area: depth-bounds software path timing, device-load dependent (one run). 115c: not covered; low priority.
7. **`swapchain_lifecycle` "no surface" still fails on stock ROMs.** 2311DRK48G (v11, `9a5b7846`), 23117RA68G and 2312FPCA6G (v9), V2515 (v14, `971d3e04`). This is the known gralloc mapper item (android/014 + wsi/017), not new. Also v7 ANA-NX9 (G76): `vkCreateSwapchainKHR res=-1000072003 phase=create_swapchain` (`VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR`).
8. **fleur (v9, G57, rc4-lowmem): `large_draw` ends as a CRASH.** `6da83fcc`: 37/38, `large_draw` status CRASH after `points_indirect` PASS, last warning `v9 indirect gpu_prerast draws ... are drawn in at most 8 chunks of 65536 vertices; the rest of a larger draw is dropped`. Area: v9 indirect GS/XFB prerast chunking (patch 114 family) or the next case (`instanced_direct`). Single report, no tombstone. 115c: not covered.
9. **PanPlay sessions that end in a Wine kill with no GPU error.** Infinix X6873 (G615, v11, rc4-lowmem): NFS Underground 7 of 7 sessions end "Wine killed by SIGKILL (exit 137)" after 18-122 s (`c30d7ed9`: `RtlpWaitForCriticalSection ... heap.c wait timed out`). DXVK was rendering (frame limiter engaged at ~63 fps, FIFO, 800x600) and the Mesa log has no CSF/JM error, so a driver stall is not shown. Area: Wine/FEX or user kill. Not covered, not driver evidence.
10. **PanProbe reports a truncated gpu_id on two JM devices.** Retroid Pocket 4 Pro (G77 MC9, `0x9080`, `d219c9c0`) and Infinix-X6882 (G57 MC2, `0x9093`, `e60f4280`): app/D1 arch becomes `v0`. The driver works (38/38 and 37/38, only `swapchain_lifecycle` "no surface" on the Infinix). Area: PanProbe gpuId read from the JM GPU properties (product id instead of the 32-bit gpu_id). Fix is in the app (next PanProbe), no Mesa change.
11. **Non-driver PanPlay exits.** pes2017 on CPH2625 (exit 1, `setupapi do_file_copyW Unsupported style`), FCPrimal on 2511FPC34G (exit 10, launcher NPE in ActivityThread), re7 (exit 84, `MFMediaSource` 0x8007000e, `517ec70b`), Sonic Forces on 24129RT7CC (exit 3, `steamclient_init unable to load native steamclient`, `54af080c`), Deep Sleep 2 on SM-X820 (exit 1 after 1 s, `2fbf8429`), SM-A515F cubes on v6 (DXVK `Failed to initialize DXVK`, CreateDevice r=-8: v6 feature gaps, F4). All have a Wine, game or launcher cause in the logs.

Positive results to carry forward: G77 `0x90800011` enumerates and passes 38/38 (21081111RG, TrebleDroid GSI, Retroid, 4 runs); G52 r1 (`0x74021000` variant 0x2/0x3) resolves to "Mali-G52 r1"; G1-Ultra (v14, 13 records, 4 models) works through the `MEM_ALLOC_EX` fallback, 37/38 on rc4-lowmem; G925 38/38 (V2413 17/17 on a short build); v9 G57/G68/G78 38/38.

Not read: 8 rows (ids 1057, 1058, 1067, 1068, 1071, 1075, 1232 have only a Gofile link, 1110 has no link at all).
