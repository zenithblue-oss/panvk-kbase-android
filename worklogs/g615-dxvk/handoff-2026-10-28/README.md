# Handoff 2026-10-28: moving work from the phone (PRoot) to the desktop

All state that matters is in this repo (`main`). Things that live only on the phone are listed in
[Not in git](#not-in-git-phone-only-rebuild-or-copy).

## Active goal (user's /goal, verbatim)

> u have to imporve driver perf on things which are currenlty emulated on gpu. improve overall perf on gpu on all arch.
> in panprobe add proper test and required for 11_0, 11_1, 12_0, 12_1 and 12_2. for all mali arch gpus from v6 to 14.
> research if not possible tell and in panprobe as well. add all feture set test 3d for these directx if not present and
> test on devicr on all arch on firebase devices from panplay. do task fast in quicky and release driver as standsrd
> release after get good perf compared to rc4 and pass all test on all mali arch anf get all feature set for directx on
> all mali arch. continu with cureent task

Meaning:
- Speed up the GPU-emulated stages: GS, tessellation, XFB.
- Give PanProbe a feature-level checker for 11_0 through 12_2 on v6-v14.
- Give PanPlay D3D feature-level tests and run them on every Firebase arch.
- Release a standard (non-rc) driver once three things hold: perf is good vs rc4, every test passes on every arch, and the D3D feature sets work.
- Release is allowed only under these conditions. The user signs the APKs with the release key.

User clarifications that bind the goal:
- **"12_2 only [out of reach], 12_0 and 12_1 should work"** on all arches, including v6/v7.
- v6/v7 should be fully DXVK-compliant, as PanProbe shows, and run many DX12 titles.
- **GPU hang or DEVICE_LOST blocks the release.** Fix the root cause and never regress.
- Upstream Mesa sync is **ON HOLD** until the user says go. Plan: `worklogs/g615-dxvk/upstream-sync-2026-10-26/README.md`.
- Full sparse implementation is a **TODO** (research is done; see below).

## Current state: build 115c is built but has not been confirmed on a device

| | Value |
|---|---|
| Driver | `/home/flux/build-out/2026-10-28-115c/libvulkan_panfrost.so` sha256 `eb44336a938705923bfca1728694701cdb05b4c589b959ecdede2b11464f5b6b` |
| driverInfo | `Mesa 26.3.0-devel (git-15fd70c7c4)`, packageVersion `26.3.0-devel` |
| Series | csf-v11 018-233 + jm-v9 + jm-v7 + jm-v7-wip 0006-0139 (`scripts/prepare-rc5-tree.sh` defaults `--csf-max 233 --wip-max 0139`) |
| Mesa base | `5a07217f034b3e50d8c7c7794f97a2df1742613b`; tree `e945011a` (`p115c-final` = `p115c-fix`) |
| PanPlay | 1.3.0, versionCode **44**, `/home/flux/build-out/apks/panplay-115c.apk` `a16f90963bed3c5f734408ee811c640adaca0d50caed95c9025d8a803df00d5f` |
| PanProbe | 1.3.0, versionCode **19**, `/home/flux/build-out/apks/panprobe-115c.apk` `9c9989eb34ad10a89d394b4cced4fea4f013a56274b12b88cf1827de49b16847` |
| Unsigned release APKs | `/home/flux/build-out/release-115/PanPlay-1.3.0-unsigned.apk` `6d9c0330...` and `PanProbe-1.3.0-unsigned.apk` `26aeade5...` (+ `SHA256SUMS`), already the 115c build |

**Next step:** run the 115c confirm runs on FTL panther (v10), shiba (v11), a31 (v7) and oriole (v9).
- PanPlay runs use scenarios 1-9 including FL tests 7-9, with a 10 s warm-up and a 30 s measure.
- Add PanProbe all-pass runs.
- Compare against the 115b numbers below.
- The override files are ready: `worklogs/g615-dxvk/device-runs/2026-10-28-ftl-*-115c-panplay/gameloop-override.json`.
- No 115c confirm run has been launched yet.

### Build history this cycle

| Build | Patches | What it fixed |
|---|---|---|
| 115 | jm-v7-wip 0093-0131 (staging-115 promoted) | rc5 naming; PanProbe 39/39 on all 5 FTL devices (both passes) |
| 115fix | `patches/staging-115/fix/` | cube message pump; D3D9 HUD garbling on non-coherent GPU (uncached heap, `PANVK_KBASE_UNCACHED_HEAP`); DX12 NULL RandR crash; harness log cap |
| 115b | 0132-0135 | **0134** v10 hang: cross-subqueue wait cycle (a stream now drains its own deferred signals before any cross-subqueue wait, kbase only). **0135** a31 device lost: zero-size alloc read as OOM left img_attrib_table NULL, so the descriptor copy wrote to GPU address 0. **0133** placed map: dma-buf from gralloc (MediaTek handle, fd 1 not fd 0). **0132** IDVS waits for its descriptor copy / helper job |
| 115c | 0136-0139 | **0136** CSF tiler bin budget = (chunk-64)/(2 x layers) on v10/v11 (D3D12 FL 11_1 `tir` and 12_1 `conservative` DEVICE_LOST from tiler heap OOM; knob `PANVK_TILER_LAYER_BUDGET=0` reverts to 115b). **0137** JM per-instance attribute offsets use the dynamic stride (gears on v6/v7, red gear drawn twice). **0138** a logic op reads the destination, so no forward pixel kill (FL 11_1 `logic_op`). **0139** Bifrost scheduler: the message descriptor src 3 of ST_CVT/ST_TILE/TEXC never comes from the same-tuple FMA result (a31 typed UAV event 0x51) |

Details: `patches/jm-v7-wip/README.md` (sections 115b and 115c).
Evidence: `worklogs/g615-dxvk/device-runs/2026-10-27-*115b*` and `2026-10-28-*`.

### 115b FTL results, the baseline for 115c (warm-up 10 s, measure 30 s)

| Device | D3D11 / D3D9 cube | SquidRoom / gears | D3D12 FL 11_0 / 11_1 / 12_0 / 12_1 / 12_2 | D3D11 FL 10_0-11_0 / 11_1 |
|---|---|---|---|---|
| a31 (v7) | PASS 65.8 / PASS 35.7 fps | not run / 109 fps (red gear twice) | PASS / FAIL logic_op / DEVICE_LOST typed UAV (0x51) / unsup / unsup | PASS / FAIL tir |
| panther (v10) | PASS 572.1 / 90.0 | 85.0 / 431.1 | PASS / DEVICE_LOST tir / unsup / unsup / unsup | PASS / FAIL tir |
| shiba (v11) | 414.3 / 119.2 | 78.6 / 187.2 | PASS / DEVICE_LOST tir / unsup / DEVICE_LOST conservative / unsup | PASS / FAIL tir |
| oriole (v9) | 170.4 / 89.8 | 27.2 / 323.0 | PASS / FAIL logic_op / unsup / unsup / unsup | PASS / FAIL tir |

- PanProbe 115b on panther: 40/40 PASS.
- sparse_kbase: `VERDICT v10 lazy-tile possible -- 64 KiB FIXED tiles commit 16->0->16 and re-alloc at the same VA (CSF 1.14)`.
- 115c diag on shiba (`2026-10-28-ftl-v11-shiba-115c-diag{,3}-panplay`): with the layer budget, 11_1 `tir` and 12_1 `conservative` pass. The D3D11 `tir` draws reach the driver single-sampled; this is still open.

Build 115 PanProbe totals (39/39 everywhere):

| Device | Defaults | 114-knobs |
|---|---|---|
| starlte (v6) | 216.5 s | 292.5 s |
| a31 (v7) | 323.1 s | 431.9 s |
| oriole (v9) | 214.0 s | 302.1 s |
| panther (v10) | 167.5 s | 136.6 s |
| shiba (v11) | 187.4 s | 164.0 s |

A negative % in the perf tables means less time, i.e. faster. Most of the gain came from removing the D4 JM 2 s warm-up.

## What is left, in order

1. **Confirm 115c on FTL** (panther, shiba, a31, oriole): PanPlay scenarios 1-9 and PanProbe.
   - Expected: no DEVICE_LOST; FL 11_1 `logic_op` passes; gears are correct on a31.
   - Any new hang or DEVICE_LOST means fix the root cause first.
2. **Still open after 115c:**
   - D3D11 FL 11_1 `tir` FAIL on every arch: draws arrive single-sampled. Decide whether this is a driver bug or a cap that must not be advertised.
   - SquidRoom crashes on exit at `d3d12core.dll+0x329F0` on all devices.
3. **starlte (v6) Wine prefix setup**: SELinux denies ioctl `0x7201`, and kernel32 fails with `c0000135`. No D3D test has run on v6 yet.
4. **v6/v7 to FL 11_1**: raise `maxPerStageDescriptorStorageImages` from 32 to 64. vkd3d-proton needs 64.
5. **PanPlay `VKD3D_FEATURE_LEVEL=12_0` override**, so 12_0 games run while sparseResidencyAliased is false.
6. **Run the FL tests (scenarios 7-9) and sparse_kbase on all 5 FTL devices**, including starlte (v6).
7. **Perf work on the emulated stages (GS, tess, XFB)** on all arches.
8. **rc4 comparison**, then a standard release once everything passes on every arch. APK release signing is done by the user (`docs/SIGNING.md`, `scripts/sign-release.sh <keystore> <alias>`). The expected cert is `8b10fde8...`; the current APKs are debug-signed with `bc517d8c...`.
9. **12_0 / 12_1 on every arch (the goal's FL target):**
   - 12_0 needs tiled resources tier 2, i.e. real sparse. Today 12_0 is not creatable on any device.
   - Sparse plan S0-S6 is about 8-11 weeks. S0, the PanProbe `sparse_kbase` gate, is done.
   - Research and per-arch verdicts are in `worklogs/g615-dxvk/sparse-kbase-2026-10-27/README.md`, with a TODO at the top of `docs/KBASE-SPARSE-FEASIBILITY.md`.
   - v10-v13 (CSF uAPI >= 1.9) can get lazy 64 KiB tiles via MEM_ALLOC_EX FIXED + GROW_ON_GPF + MEM_COMMIT.
   - v6/v7/v9 and v14 need eager backing.
   - sparseResidencyAliased stays false, so the `VKD3D_FEATURE_LEVEL` override is still needed.
   - 12_1 needs VK_EXT_fragment_shader_interlock (ROV) plus conservative raster. v11 has conservative raster tier 1.
   - FL reachability table: `worklogs/g615-dxvk/d3d-feature-levels-2026-10-27/README.md`.
10. **On hold:** the upstream sync (single topic-grouped branch, not one squashed patch; v15 = Mali G2). Do it only when the user says go.

## Not done / caveats from the last agents

- Release APK builds skip the lint vital tasks, because the lint jar is not in the offline Gradle cache.
- PanPlay's What's New text and the JVM unit tests were not updated for 115b or 115c.
- If the PanProbe Gradle build fails at CMake configure, move the stale `apps/panvk-test/app/.cxx/tools/{debug,release}/arm64-v8a/compile_commands.json` aside.
- `video.mp4` files under `device-runs/*/raw/` are not committed: too large, and kept local by rule. The phone also has `attach_*.png` (user screenshots) and `tools/u-queue-fork-test/build.py`, which are never staged.
- The Cloudflare upload list (`https://panvk-upload.panvk.workers.dev`, `apps/panvk-test/.../Share.kt:188`) needs an admin token; the request returned 401.

## Standing rules (from the user; full text in `claude-memory/`)

- Never kill a process you didn't start. Never touch Chrome. No `adb kill-server`/reconnect on RTL devices. No X6 Pro testing unless the user asks.
- Never uninstall an app or clear app data without approval. Package ids, slot ids, the `_rc5.so` filename and old tags stay unchanged.
- No tags or releases except under the goal's conditions.
- driverInfo is upstream style: `Mesa 26.3.0-devel (git-<sha>)`.
- Commit and push to main after every change:
  - commit with `git -c user.email=zenithblue.dev@gmail.com commit`;
  - end the message with `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`;
  - run `git pull --rebase` before push.
- New patches go at the end of the series. Check them with a strict prepare (no offset or fuzz) on a fresh `5a07217f` worktree, then update the `prepare-rc5-tree.sh` defaults.
- Firebase:
  - one run at a time (`flock /home/flux/build-cache/firebase.lock` on the phone);
  - account `abhay02delhi@gmail.com`;
  - projects `panvk-device-test`, `finalbenchmark-665a8`, `phone-finder-hub`, `phonefinder-hub`;
  - on `TEST_QUOTA_EXCEEDED`, rotate projects and upload the APKs to that project's bucket;
  - before relaunching, check the Testing API for matrices that already finished;
  - every run commits screenshots plus `results.md` in its run dir.
- Heavy builds: at most 4 jobs (`flock /home/flux/build-cache/build.lock`).
- Downloads are untrusted: put them in new empty dirs and run Python with `-I`.
- Delegate to subagents one at a time and stop each after its report. The user wants things done fast.
- Send one push notification only when the driver and APKs are ready for a device test.

FTL devices: starlte (v6, API 29), a31 (v7, API 31), oriole (v9, API 33), panther (v10, API 33), shiba (v11, API 34).
Firebase setup: `worklogs/g615-dxvk/firebase-setup.md`.
Run helper scripts (chain, post-processing, gallery, frame stats): `firebase-scripts/`. Their paths assume the phone layout; edit `SP`/paths on the desktop.

## Not in git (phone only: rebuild or copy)

| What | Phone path | How to get it on the desktop |
|---|---|---|
| Mesa trees | `/home/flux/src/mesa-115c-final` (also -fix/-diag, 115b-*, 115-*) | `scripts/prepare-rc5-tree.sh` on Mesa `5a07217f` (defaults give 115c) |
| Driver builds | `/home/flux/build-out/2026-10-28-115c/`, `2026-10-27-115b/`, `2026-10-26-115{,fix}/` | rebuild (the build scripts are in `scripts/`), or copy (30 MB each) |
| APKs | `/home/flux/build-out/apks/panplay-115c.apk` (415 MB), `panprobe-115c.apk` (44 MB), `release-115/` | rebuild with Gradle, or copy |
| Build dirs | `/home/flux/build-rc5/` (bionic-r30-115c, host-115c, logs, `*.bucket`) | not needed |
| Gradle/NDK caches | `/home/flux/build-cache/` | the desktop uses its own |
| gcloud login | `/home/flux/.local/bin/gcloud` | `gcloud auth login` on the desktop with the same account |
| Claude memory | `~/.claude/projects/-home-flux-repos-panvk-kbase-android/memory/` | copied to `claude-memory/` here |
