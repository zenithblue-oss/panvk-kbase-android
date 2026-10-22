# v6 (Bifrost G71/G72) readiness for RC5 v7K, 2026-10-22

## Verdict

The v7K build already covers v6. No new patch (no 0063) and no rebuild were needed. Nothing was run on any device; no v6 device has run anything yet.

## Evidence

- Build flags: `scripts/build-android.sh` passes `-Dvulkan-drivers=panfrost -Dpanfrost-kmds=kbase` with no arch list. `src/panfrost/vulkan/meson.build` (mesa-v7J) always builds per-arch libs `foreach arch : [6, 7, 9, 10, ...]`, with `bifrost_archs = [6, 7]` and `jm_archs = [6, 7, 9]`. v7K was built with `--profile g615-v11-csf --api 29` (worklogs/g615-dxvk/builds/v7K.md), so v6 is compiled in.
- Device table: `src/panfrost/model/pan_model.c` has G71 `PAN_PROD_ID(6,0,0)` (0x6000) and G72 `PAN_PROD_ID(6,2,1)` (0x6221).
- Gating: in `panvk_physical_device.c` the kbase path (`case 6: case 7:`) only logs "v%u (Bifrost) support is experimental" and continues; there is no env-var gate. (The DRM path still has the `PAN_I_WANT_A_BROKEN_VULKAN_DRIVER` gate but the Android build uses kbase.) All jm-v7-wip paths are `PAN_ARCH < 9` or `== 6 || == 7`, never `== 7` (patches/jm-v7-wip/README.md, "v6 (G71/G72) parity").
- Binary: `strings` on the stripped .so finds "G72" (8 hits). Per-arch `panvk_v6_*` symbols are stripped, so they cannot be checked this way.
- Atom stride: patch 0018 adds the 48-byte `base_jd_atom_v2` fallback for pre-uAPI-11.22 kernels, which is what G71/G72-era devices usually run.
- Compiler quirks come from the real gpu_id: G71 gets `NO_FP32_TRANSCENDENTALS | LIMITED_CLPER`, G72 gets `LIMITED_CLPER`. Precompiled libpan kernels for v6 are built with gpu_id 0x60000000 (G71 quirk set), safe on both.

## Risks / v6-specific assumptions (unverified on hardware)

- Subgroups: `panvk_vX_physical_device.c` advertises the full subgroup op set (vote, ballot, shuffle, quad, clustered, rotate) for all Bifrost. JimVulkan reports no subgroup votes on G71/G72. Likely first thing to break on G71/G72 (LIMITED_CLPER); watch the subgroup tests.
- G71 has no FP32 transcendentals in hardware (compiler workaround exists, untested).
- Robustness (patch 0010, 0035) relies on hardware bounds of UBO/attribute/texel-buffer descriptors, measured only on G52 so far (A06). Unverified on G72.
- v6 varying swizzle and blend-constant paths are upstream PanVK code, never run by us.
- G72 `MODEL_TB_SIZES` row is 8192/4096, G71 is 4096/4096, never validated.
- Android 10 (API 29): earlier ICDs imported `call_once` and failed to dlopen on API 29 (group G in PROGRESS.md). v7K was built `--api 29` with NDK r30; not confirmed that this fixes group G, so check dlopen first on an Android 10 device.
- Memory: G71/G72 kernels may lack dma_heap/ion differences; the kbase memory policy for arch < 9 (GROW_ON_GPF, no JIT) applies but is unproven on v6.

## Artifacts (reused, nothing new built)

| Artifact | Path | sha256 |
|---|---|---|
| RC5 v7K driver | /home/flux/build-out/2026-10-21-v7K/libvulkan_panfrost.so | badc28eb31786c33495245395f62edcbbfb45d86f73f6e88edb9c8e57b36de4b |
| PanProbe APK (pkg dev.zenithblue.panvktest.v7k) | /home/flux/build-out/apks/panprobe-v7K.apk | c8231e008e36794aea7d339c78af1448a63a273c2d03a1d41e557d57c6349af6 |

The APK bundles rc4 (`libvulkan_panfrost_rc4.so`) and rc5 (`libvulkan_panfrost_rc5.so` = badc28eb...). rc4 has no v6/v7 Bifrost support, so expect it to refuse the device (INCOMPATIBLE_DRIVER); that is a useful negative control, not a failure.

## Devices

RTL: per TEST_DEVICES.md (snapshot 2026-10-16, 1324 devices), there is no G71/G72 device on RTL, and the oldest Android is 12. That table says "Missing on RTL: G71, G72 (v6)". The Exynos 7885/9610/9611/9810 phones (A8 2018, A7 2018, A50, A51, M30s, M31, M21, S9) were not in the signed-in list, so no RTL candidate can be recommended. Re-check the live RTL list before giving up (stock may have changed; not re-verified today, no RTL login from this agent).

Candidates ranked:

1. Firebase Test Lab `xcoverpro` (Galaxy XCover Pro, Exynos 9611, Mali-G72 MP3, Android 13 / API 33). Best: v6 and no Android 10 dlopen issue. Limit: Firebase needs an instrumentation/game-loop entry point and 5 runs/day on Spark.
2. Firebase `starlte` (Galaxy S9, Exynos 9810, G72 MP18, API 29): covers the big G72 and the API 29 dlopen question.
3. Firebase `a10` (Galaxy A10, Exynos 7884, G71 MP2, API 29): only G71 option.

If an RTL device with Exynos 9610/9611/7885/9810 appears, prefer, in order: Galaxy A50/A51 (G72 MP3, Android 11+), M31/M21 (G72 MP3), A8 2018 (G71 MP2, Android 9-10 at most, likely API 29 at best).

Recommended: `xcoverpro` on Firebase for the first v6 run; an RTL G72 phone (A50/A51) if one is listed.

## Test plan

1. Confirm the device GPU: gpu_id 0x6000 (G71) or 0x6221 (G72, may carry a revision in the low bits).
2. Install `panprobe-v7K.apk`.
3. rc5: `adb shell am start -S -n dev.zenithblue.panvktest.v7k/.MainActivity --es driver bundled-rc5 --es autorun all` (use the same `--es driver` value as the A06 v7K runs in worklogs/g615-dxvk/device-runs/2026-10-22-a06-v7K-ab/).
4. rc4: same command with the rc4 driver selection. Expect INCOMPATIBLE_DRIVER or a refusal on v6.
5. Pull `files/autorun.txt` via `run-as` (ends with `AUTORUN DONE`) and logcat. Check for: "v6 (Bifrost) support is experimental", the atom stride chosen (48/56/64), DEVICE_LOST, GPU faults, dlopen errors (call_once on API 29).
6. Triage failures first among: subgroup tests, robustness, swapchain, tess/GS/xfb emulation. Baseline reference is A06 v7K 38/0/0.
7. Record results in worklogs/g615-dxvk/device-runs/<date>-<device>-v7K/.
