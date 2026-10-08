# beta.18-rc1 tester logs (PanProbe / PanPlay 1.2.4)

Source: D1 `panvk-uploads` rows 254-330 (2026-10-07 17:56 to 2026-10-08 ~09:30 UTC), zips read from the upload blob store (read-only). Local copies: `tmp/rc1-logs/` (gitignored), beta.17 comparison zips in `tmp/rc1-logs/b17/`. No tester identifiers are recorded here; records are named by device model, gpu_id and the first 8 hex digits of the zip sha256.

rc1 identity: `.so` sha256 `d000493e...`, BuildID `f01a87a4...`, driverInfo `PanVK-kbase beta.18-rc1 (Mesa 26.3.0-devel (git-8d4fa734a1))`.

## Record counts

| Group | Records | Notes |
|---|---|---|
| rc1 bundled, app 1.2.4 | 41 zips (30 PanProbe, 11 PanPlay) + 9 D1-only duplicates (external host only, same device/time as a retrieved zip) | the real rc1 population |
| rc1 `.so` imported into app 1.2.3 | 11 zips (10 PanProbe, 1 PanPlay) | same binary, older harness (36 tests, not 37) |
| app 1.2.4/1.2.2 with a different `.so` (flagged separately) | 12 zips | TECNO CM7 x9 (zip file imported as `.so`, see P1), 1 stock-vendor-driver run pair, 1 PanPlay 1.2.2 with a non-bundled dev `.so` `4603b00f` |
| beta.17 bundled (reference) | 4 in this id range | used only for comparison |

Devices on rc1 (model, gpu_id):
- v10: 23090RA98I `0xa8670000` (G610 MC6, 5.15), 23054RA19C `0xa8670000` (5.10), 2306EPN60G and TECNO CL8 `0xa8670004` (6.12).
- v11: 2311DRK48C/G/I, 24090RA29G/I, LXX525, RG557 (all `0xb8a31030`, G615).
- v12: 2412DPC0AG, 2511FPC34G (`0xc8700010`).
- v13: 2506BPN68G, 2602BPC18G, PLC110, V2507A (`0xd8300015`).
- v9: 24116RACCG (G57, `0x90930010`), RMX3944 (G57), Infinix X6739 (G77, `0x90800011`).
- v7: HiPad-Air (G52, `0x74021000`).
- Non-Mali: PHK110 (Adreno, SM8475).

## PanProbe results on rc1

- v10, v11, v12, v13 bundled 1.2.4: **every record 37/37 PASS, 0 fail, 0 skip** (30 zips minus the v9/v7 ones, i.e. 26 records across 17 device models; Android 14 to 17, kernels 5.10 to 6.12).
- rc1 `.so` imported into 1.2.3: 36/36 on all 10 records (v10-v13).
- v9 G57 (24116RACCG): 12 pass / 19 fail / 6 skip. Decisive line: `MESA: error: kbase: JM atom 35 failed: event=0x58 core_req=0x16` followed by `FAIL Wait r=-4` (gpu_prerast_slice); `FAIL VK_EXT_transform_feedback not exposed` (large_draw); `hard 47/56 available` (dxvk_reqs).
- v9 G77 (Infinix X6739): 0/36. `panvk: gpu_id 0x90800011 ... model unknown` then `Unknown gpu_id (0x90800011) or variant (0)` and `FAIL no Mali`.
- v7 G52 (HiPad-Air): 0/36. `Unknown gpu_id (0x74021000) or variant (0x3)`, also `KBASE_IOCTL_MEM_JIT_INIT failed: Out of memory`.

## Problems

| # | Class | NEW/KNOWN | Devices | Evidence |
|---|---|---|---|---|
| P1 | PANPROBE app | NEW | TECNO CM7 x9 zips (app 1.2.4) and 2412DPC0AG (app 1.2.3) | Imported "driver" is a zip, not an ELF: `dlopen failed: ".../imported/libimported.so" has bad ELF magic: 504b0304` (every test FAIL, 0/37). `MainActivity.kt` DriverTab import copies the picked file with no ELF check (only `DriverUpdate.kt` has `isAarch64So`). Also the run still uploads as a "result", polluting stats. |
| P2 | PANPLAY app | NEW | 2506BPN68G (PanPlay 1.2.2, record `e5c8eef5`) | `FATAL EXCEPTION: main java.nio.BufferOverflowException at XOutputStream.writeInt <- XInput2Extension$RawMotion.send <- XServer.injectPointerMoveDelta`. Likely cause (not proven): `XOutputStream.flush()` leaves the buffer flipped if `clientSocket.write` throws (Wine gone); `ensureSpaceIsAvailable` checks `capacity - position`, not `limit`, so the next `put` overflows. Code is unchanged in the tree. |
| P3 | PANPLAY app | NEW | 2506BPN68G (run 300 after Silksong run 297) | Cube D3D11 zip contains `unity-Player-Hollow Knight Silksong.log` and `-prev` from the earlier game; summary reports 37 errors incl. `Crash!!!` that belong to the other game. Stale game logs are not cleared or filtered per run. |
| P4 | DEVICE / PANPLAY | NEW | PHK110 (Snapdragon, Adreno) | `vulkan:init_physical_devices Failed to enumerate physical devices, res -3` then `DxvkInstance::createInstance: Failed to create Vulkan instance`, exit 1 in 4 s. PanVK on a non-Mali SoC; the launcher does not pre-check and the upload has no gpu_id. |
| P5 | WINE/PROTON | NEW (low) | 2506BPN68G (Silksong, 137 s) | Unity log: `mono_os_sem_timedwait: mono_win32_wait_for_single_object_ex failed with error 87`, `Crash!!!`, `IOException ... EventWaitHandle.Reset`. Wine sync primitive error 87 (invalid parameter); game kept running until user exit; no device lost, no tombstone. Same `error 87` appears in svchost/rpc noise on every run. |
| P6 | WINE/DXVK teardown | NEW (benign) | 2506BPN68G D3D8 x86 | `Presenter: Failed to get surface capabilities: VK_ERROR_SURFACE_LOST_KHR` then `Failed to create Vulkan surface: VK_ERROR_OUT_OF_HOST_MEMORY` right before exit 137, after the user pressed EXIT (window destroyed first). Not seen in the 11 other cube runs. |
| P7 | DRIVER / feature gap | KNOWN (item 9, v9 Vulkan 1.3) | RMX3944 (G57) | DXVK: `Skipping: Device does not support Vulkan 1.3`, `No adapters found`. Cube then dies with `Unhandled page fault on read access to 0x0 at 0x140001BA1` (builtin dxcube does not handle DXVK init failure; exit 5). Also DXVK shows driver string ` 26.2.99` with empty name on JM. |
| P8 | DRIVER | KNOWN (JM compute pre-raster, PROGRESS line ~121) | 24116RACCG (G57) | 3402 `JM atom N failed: event=0x58` lines in one run. 12/19/6 vs 12/18/6 on beta.17: the extra fail is `bc_perf`, a test new in 1.2.4 (`FAIL Query r=-4`), not a driver regression. |
| P9 | DRIVER | KNOWN (item 19, fix in uncommitted jm-v9/006) | Infinix X6739 (G77) | `Unknown gpu_id (0x90800011)`. rc1 predates the model-row patch. |
| P10 | DEVICE (not a target) | KNOWN | HiPad-Air (G52 v7) | `Unknown gpu_id (0x74021000)`. PanProbe reports 36 FAIL instead of "unsupported GPU"; a harness N/A would be clearer. |
| P11 | DEVICE / ROM | KNOWN class | TECNO CM7 x2 (stock vendor Vulkan, Mali-G615 MC2) | System driver 16/16/5: `FAIL CreateDevice r=-8`, `hard 44/56` in dxvk_reqs. Vendor driver, not PanVK. |
| P12 | PANPLAY harness | NEW (cosmetic) | all cube runs | Summary counts normal Wine/DXVK noise as errors (`winebth`, `libGL.so.1`, `EDID`, `rpcrt4 ... error 87`, `BadImplementation` opcode -103 on xserver). 17 to 53 "errors" per clean run hides real ones. |
| P13 | PANPLAY | not a bug | 2311DRK48C | Exit 137 on cube runs is `User exit (overlay EXIT or Stop)`, not a crash. |

Not seen anywhere in rc1: DeviceLost, fence timeout, `VK_ERROR_OUT_OF_DEVICE_MEMORY`, "Failed to allocate", box64/FEX crash, hang before driver load, ANR, tombstone (apps cannot read them, so absence is weak evidence).

## PanPlay runs on rc1 (bundled, 1.2.4 unless noted)

| Device | gpu_id | Workload | DXVK | Result |
|---|---|---|---|---|
| 2311DRK48C | `0xb8a31030` | Cube D3D11 ARM64EC | v3.1.1+, FL11 | 19 s, ~900 logged frames, user exit |
| 2311DRK48G | `0xb8a31030` | Cuphead D3D11 x2 (118 s, 196 s), Cube D3D8 x86 | v3.1.1+ | no DXVK/MESA errors, user exit; D3D8 exit 0 |
| 2412DPC0AG | `0xc8700010` | Cube D3D8 x86/x64, D3D9 x86 (5 runs) | v3.1.1+ | all exit 0, ~1140 to 1320 frames in 27 to 29 s |
| 2506BPN68G | `0xd8300015` | Silksong D3D11 (137 s), Cube D3D8 x86, D3D11 ARM64EC | v3.1.1+ | no driver errors; P5, P6, P3 |
| 23054RA19C (app 1.2.3, rc1 `.so` imported) | `0xa8670000` | METAL GEAR RISING | n/a | 18 s user exit before any DXVK log was written (inconclusive) |
| RMX3944 | `0x90930010` | Cube D3D8 x64 | v3.1.1+ | P7 |
| PHK110 | none | Cube D3D11 ARM64EC | n/a | P4 |

Known-item checks:
- Item 29 (v13 D3D8/D3D9 about 1/3 the frames of D3D10/D3D11): not reproduced on rc1. G925 D3D8 x86 ~990 frames in 25 s (about 40/s) against D3D11 ARM64EC 540 in 14 s (about 39/s). One sample each, so not conclusive.
- Item 40 (`gs_viewport_depth`): beta.17 `2412DPC0AG` (record `28ea7f24`) and `2602BPC18G` failed it; all rc1 v12/v13 records pass it. Consistent with the csf-v11/132 fix.
- Items 15/22/20 not exercised: no rc1 swapchain failure, no v14 device in rc1 uploads.

## Regressions vs beta.17 (same device)

None found. Improvements: 2311DRK48G `large_draw` TIMEOUT and `vkd3d_heap` FAIL (34/36) now 37/37; 23090RA98I `bachata_reqs`/`dxvk_reqs` FAIL (34/36) now 37/37; 2506BPN68G `vertex_stores`/`draw_params` FAIL now 37/37; 2602BPC18G 33/36 now 37/37; 2511FPC34G `gs_viewport_depth` now PASS.

## Suggested next actions

1. PanProbe: validate the imported file (ELF64 aarch64 magic, or unzip a `.so` inside a zip) in the DriverTab picker and refuse to start or upload a run otherwise (P1).
2. PanPlay: fix `XOutputStream` (clear the buffer in a `finally` around the flush, or check `limit` in `ensureSpaceIsAvailable`), clear per-game logs before each run, and filter normal Wine/DXVK noise from the error count; pre-check for a Mali kbase device before launching DXVK (P2, P3, P4, P12).
3. Driver: ship rc1 as is for v10 to v13 (37/37 on 26 records); land the G77 row (jm-v9/006) and decide whether v9 gets a Vulkan 1.3 audit (P7, P9) before the next beta. Retest item 29 with a D3D8 vs D3D11 pair on the same G925 run.
