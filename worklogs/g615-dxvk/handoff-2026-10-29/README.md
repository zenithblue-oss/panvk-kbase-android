# Handoff 2026-10-29: from the Linux desktop to the Mac desktop

Work stops here on the Linux desktop (`/home/abhaybyte/repos/panvk`) and continues on the Mac.
Everything below is pushed to `main`. Driver builds, APKs and Mesa trees live under `/var/tmp/panvk/` on the
Linux machine and are **not in git**; rebuild them on the Mac (see "Rebuild on the Mac").

## Goal (unchanged)

The full rules are in the Claude memory files `autonomous-handoff-loop`, `perf-native-first-plan` and `universal-driver-no-g615`. In short:

1. **Perf plan:** work through `docs/PERF-OPTIMIZATION-PLAN.md`. Never emulate a feature an arch has natively; use the ARM gpuinfo reports as evidence. Track results in `docs/PERF-PROGRESS.md`: rc4 vs 116 vs latest.
2. **Final driver:**
   - Build one final driver and make it the bundled default.
   - Test it on **every Mali arch v6-v14** with PanProbe (full) and PanPlay (scenarios 1-9), for speed and memory against rc4. Every run needs screenshots.
   - Release only when all arches pass. The user signs the APKs. The release embeds PanProbe and PanPlay screenshots at the top.
3. **README.md rewrite**, as the last step before release. Cover:
   - driver info: build, driverInfo, sha256, patch level;
   - v6-v14 support with the tested device per arch; v15 is not fully supported;
   - Bachata S4;
   - DX9/10/11/12 with feature levels;
   - perf gains rc4 vs 116 vs final.
   - D3D12 wording, as the user put it: basic D3D12 games work at FL 11_0/11_1, but a game that uses an FL 12_x feature crashes. Sparse resources, which FL 12_0 needs, are not implemented on kbase; that TODO is in `worklogs/g615-dxvk/sparse-kbase-2026-10-27/README.md`.
   - Never label the driver G615. It is "universal v6-v14, v15 not fully supported".

## Driver builds

All builds are Mesa `5a07217f` plus csf-v11 018-233, jm-v9, jm-v7, and jm-v7-wip 0006-N, applied with
`scripts/prepare-rc5-tree.sh`. That script runs strict: no offset or fuzz.

| Build | jm-v7-wip to | driverInfo | sha256 / BuildID | Status |
|---|---|---|---|---|
| rc4 | (release) | `PanVK-kbase RC4 (Mesa 26.3.0-devel (git-5a07217f03))` | `977c0c4c…` / `6654fada…` | last release, kept in PanProbe for comparison |
| 120 | 0160 | `Mesa 26.3.0-devel (git-94af3802af, p0160, 2026-10-29)` | `1195a047…` / `6c106131…` | **current bundled default** (pins + prepare `WIP_MAX=0160`) |
| oom2final | 0161 | `… (git-84704e1bb7, p0161, …)` | `d0d4a971…` / `0cf6b04d…` | v11 + v13 tested |
| **121b** | 0163 | `Mesa 26.3.0-devel (git-07177325ca, p0163, 2026-10-29)` | `9f58ee27ad8e4d0b02f43b26bada3e34da5feba5134f8ac1c91b23c4f9cf85ae` / `c5337130bf092b76764921dc321808a88f56df0c` | **final-build candidate**, not default yet |

Linux paths:
- .so files: `/var/tmp/panvk/build-out/2026-10-29-{120,oom2final,121b}/libvulkan_panfrost.so`
- APKs: `/var/tmp/panvk/build-out/apks/{panprobe,panplay}-{120,oom2final,121b}.apk`

### Patches added today (jm-v7-wip)

| Patch | What it does | Result |
|---|---|---|
| 0148 | No sparse device TLS on v14 | fixes the v14 DEVICE_LOST and the tess mismatch |
| 0149-0151 | Logic op FPK, v9 JM merge workgroups, prerast merge workgroups | build 116 |
| 0152 | v9 merged VS instance-rate attributes | |
| 0153 | BC decode one 4x4 block per invocation | v9 BC 2.2-2.5x RGBA8, was 6-18x |
| 0154 | driverInfo shows patch level and date | |
| 0155 | BC transcoder compile from the first image | |
| 0156 | Present timing lines | measurement only |
| 0157 | Pass-through GS skip, CSF path | `gs_pass` 111 to 4.5 us/draw on v11 |
| 0158 | GPU load/VRAM file for MangoHud, CSF | |
| 0159 | Reverts 0147 (per-chunk flush) | |
| 0160 | X11 sw WSI reads frames through the uncached buffer again | **fixes the fps regression** vs rc4 |
| 0161 | kbase incremental rendering on tiler heap OOM | **fixes the v13 `tess_perf` DEVICE_LOST** |
| 0162 | BC prewarm compiles the XC_BATCH pipeline only for images that can use it | cold-cache `bc_decode` 9.3 s to 5.2 s on v11 |
| 0163 | v9 JM pass-through GS skip | v9 `strip_gs` 354 to 17.6 us/draw, `gs_pass` 30 to 7 |

Details for each patch are in `patches/jm-v7-wip/README.md`.

**fps regression (0160).**
- Cause: 0115, present since 115c. PanPlay presents through the X11 sw WSI, which reads each frame back on the CPU. 0115 made that read cached memory with a full-frame cache invalidate before each read, about +1.8 ms per present.
- Result on X6 Pro, D3D11 FL 11_1: 257 to 831 fps (rc4 803).
- `PANVK_WSI_SW_CACHED=1` restores the 0115 behaviour.
- Write-up: `device-runs/2026-10-29-x6pro-rc4-vs-119-fps/`.

**0161 (tiler OOM).**
- Cause: kbase terminates a group on tiler heap OOM unless the group has `BASE_CSF_TILER_OOM_EXCEPTION_FLAG`.
- Fix:
  - all 3 subqueues share one kbase group with that flag set;
  - the firmware incremental-rendering handler now runs;
  - FINISH_FRAGMENT per pass recycles heap chunks.
- `PANVK_KBASE_TILER_IR=0` restores the old behaviour.
- Write-up: `device-runs/2026-10-29-x6pro-tiler-oom/`.

## Test status per arch

| Arch | Device | Latest build tested | PanProbe | PanPlay 1-9 | Run dir (`worklogs/g615-dxvk/device-runs/`) |
|---|---|---|---|---|---|
| v6 | Firebase starlte (API 29) | 118 | pass | 9/9 PASS | `2026-10-29-ftl-v6-starlte-118-*` |
| v7 | Firebase a31 (API 31) | 117 (to 0148) | pass | pass | `2026-10-29-ftl-v7-a31-117-*` |
| v9 | local TB336FU (HNY07QHT) | **121b** | 41/41 | 9/9 ok | `2026-10-29-local-121-bc-gs/`, `-local-121b-panplay/` |
| v9 | Firebase oriole | 118 | pass | not run | `2026-10-29-ftl-v9-oriole-118-panprobe` |
| v10 | Firebase panther (API 33) | 116 | pass | pass | `2026-10-29-ftl-v10-panther-116-*` |
| v11 | local X6 Pro (Y5WWBMJVOZSK4HU8) | **121b** | 41/41 | 9/9 ok | `2026-10-29-local-121-bc-gs/`, `-local-121b-panplay/` |
| v12 | RTL S10 Ultra SM-X926B | 119 | 41/41 | s3 only (device wedged in s8) | `2026-10-29-rtl-v12-sm-x926b-119/` |
| v13 | RTL S11 Ultra SM-X930 | oom2final (0161) | 41/41, `tess_perf` PASS | 9/9 PASS | `2026-10-29-rtl-v13-sm-x930-oom2final/` |
| v14 | RTL Tab S12+ SM-X846B | 0148 / 116 | pass | pass | `2026-10-29-rtl-v14-sm-x846b-0148-panplay`, `-116` |

**For release: every arch must be rerun on the final build.** 121b is the candidate, unless more fixes land first.

### Perf: rc4 vs 120 (X6 Pro v11, warm)

Full tables are in `docs/PERF-PROGRESS.md` under "Build 120", "Build 121b" and "Build 121b PanPlay".

| Test | rc4 | 120 |
|---|---|---|
| PanProbe sum, 41 tests | 152.8 s | 131.1 s (-14%) |
| tess_perf | 48.4 s | 29.0 s (-40%) |
| tessellation | 3.2 s | 2.2 s (-31%) |
| xfb | 369 ms | 233 ms (-37%) |
| pass-through GS | 92 us/draw | 3.7 us/draw |
| restart-strip GS | 337 us/draw | 7.5 us/draw |

- TB336FU (v9) PanProbe sum: 330 s with 39/41 on rc4, 151 s with 41/41 on 120.
- D3D12 passes on 120 and 121b; on rc4 it fails with DEVICE_LOST.
- Memory on X6 Pro PanProbe:
  - max total PSS: rc4 194 MB, 120 184 MB (166 MB cold);
  - max graphics memory: rc4 93 MB, 120 61 MB.
- Memory on TB336FU PanPlay: 120 uses 13-45 MB more than rc4.

## Open problems (in priority order)

1. **v13 single-scene D3D11 cube fps drop on oom2final vs 118.**
   - s1 606 vs 965 fps, s3 600 vs 882, s4 592 vs 974; the s8 D3D10 cube is -39%.
   - D3D12 and the FL cubes went up 8-28%.
   - On v11, an A/B with `PANVK_KBASE_TILER_IR=0` shows no difference (279.7 vs 277.2 fps), so 0161 is not proven to be the cause.
   - Next: on v13, A/B s1/s3 with 121b default vs `PANVK_KBASE_TILER_IR=0` vs build 120, 3 alternating runs each. Rule out thermal or shared-device variance.
2. **0161 firmware hang with a tiny heap.**
   - `PANVK_KBASE_HEAP_CHUNKS=1,8`: 4 of 10 `tess_perf` runs hang the firmware, then the GPU resets and the group gets fatal 0x72.
   - `4,32`: 1 of 13 hang.
   - Default heap: no hang seen on v11 or v13.
   - Ideas, both untested:
     - incremental rendering cannot free enough when the heap is smaller than the tiler working set;
     - the handler is armed only while the fragment CS waits for tiling.
   - If only tiny heaps hang, raise the minimum max-chunks (currently 8).
3. **0161 changes kbase scheduling on every CSF device** (one group instead of three). v10, v12 and v14 still need a run on 0161 or later.
4. **v12 S10 Ultra wedged in PanPlay s8 on build 119**, at 15:10 IST.
   - Afterwards, adb through rdb stopped answering, and the build 120 run never started.
   - It may be the RTL session limit; no logs survived.
   - Next: stream logcat to a local file during the run, and run s8 last.
5. **v6 starlte fps drop, 115e to 118:** 100 to 79 fps in PanPlay. The cause is not 0115, which is already in 115e. Not investigated.
6. **0162 limit:** whether the batch pipeline is prewarmed is decided by the first image of each ETC2 mode. If a later image could use it, it compiles at first use instead.
7. **Older kbase kernels:** kernels with only the old 32-byte group-create layout get the single group but no OOM flag, so OOM still terminates the group there.

## Perf plan items left (`docs/PERF-OPTIMIZATION-PLAN.md`)

- **Done:**
  - 1-4, 7 (+0162), 8 (+0163 for v9);
  - 16: measured and fixed for PanPlay's path in 0160; zero-copy is deferred;
  - 17: 0161;
  - 23: CSF only.
- **Closed:** 6, 13, 18, and the CSF parts of 19-22.
- **Can do now on TB336FU (v9):**
  - 23 JM: MangoHud GPU load/VRAM shows 0 on v6/v7/v9 because the JM chains have no timestamp jobs;
  - 10: v9 BC into ETC2/ASTC shadow copies, memory /4;
  - 19 / 20 / 21, JM parts.
- **Large:**
  - 5: CSF tess on one command stream, about 370 us/draw cross-CSG hand-off; high gain, high risk; best as its own build after the release;
  - 9: parallel tessellator.
- **Need cloud devices:** 14 and 22 need v6/v7 (Firebase); 11 and 15 need v14 (RTL S12+).
- **Blocked:** 12, v15 native BC. v15 is not in Mesa genxml. The ARM driver r56p0-15bet0 exposes BC natively (gpuinfo 51699 / 51676).

## Next steps on the Mac (in order)

1. Rebuild 121b, or copy it: .so plus `panprobe-121b.apk` / `panplay-121b.apk`, BuildID `c5337130…`.
2. Run the v13 cube A/B (open problem 1). If 0161 causes the drop, fix it before the final build.
3. Pick the final build:
   - bump `scripts/prepare-rc5-tree.sh` `WIP_MAX` to the last patch;
   - update both pins: `apps/panvk-test/bundled-drivers.json` (rc5 entry) and `apps/panvk-launcher/app/src/main/assets/bundled-driver.json`;
   - keep `profile: universal-v6-v14`, `supportedArches: v6-v14` and the universal wording.
4. Run all arches on the final build, PanProbe full plus PanPlay 1-9, with screenshots and PSS:
   - local: v9 TB336FU, v11 X6 Pro;
   - RTL, one device at a time, the user connects it: v13 S11 Ultra, v14 S12+, v12 S10 Ultra (needs a new session);
   - Firebase: v6 starlte, v7 a31, v10 panther.
   - Also run PanPlay on rc4 for comparison.
5. Rewrite README.md (see Goal 3). Update `WhatsNew.kt` in PanPlay.
6. Release with screenshots. The user signs the APKs.

## Devices and accounts

- **Local** (USB on the Linux desktop):
  - TB336FU v9, `HNY07QHT`;
  - X6 Pro v11, `Y5WWBMJVOZSK4HU8`; its PanPlay is build-machine signed;
  - 7d6afed8 is a OnePlus Pad (Qualcomm): ignore it.
- **RTL (Samsung Remote Test Lab):**
  - start `~/rtl-rdb/rdb`; the user connects the device on the web page; adb at `localhost:35395`;
  - 1 h session limit;
  - devices: v12 S10 Ultra SM-X926B (MT6989, G720), v13 S11 Ultra SM-X930 (G925, gpu_id 0xd8300015, core mask 0xee0077), v14 Tab S12+ SM-X846B (G1-Ultra);
  - rules: no `adb connect`/`disconnect`/`kill-server`, no sysfs reads, `timeout` on every adb call, `flock /var/tmp/panvk/rtl.lock`;
  - stream logcat to a local file during runs; devices drop without warning.
- **Firebase Test Lab:**
  - free Spark only: about 5 physical tests per day and about 30 physical min per month per project;
  - never enable billing; never enable APIs or add Firebase to projects without the user;
  - `INVALID DETAILS_UNAVAILABLE` means quota is gone; don't retry in a loop;
  - submit with `--async` and poll;
  - helpers: `/var/tmp/panvk/ftl120/` (`submit.sh <tag> <probe|play> <model> <api> "<projects>"`, `poll.py`, `ov120.json`);
  - FTL models: starlte (API 29), a31 (31), oriole (33), panther (33), shiba (35).
  - **State at stop:** all projects on the old account and on the account used last are out of quota or INVALID. phonefinder-hub is the only project with both the Testing and Tool Results APIs on. tru-valt and task-stack-app need the Tool Results API turned on, by the user, in the console. Quotas reset at 12:30 IST each day.
  - The account names and projects are in the Claude memory file `firebase-quota-and-v10-ria2.md`, not in git, because this repo is public.
  - Leftover: the zenithblue projects notification-501107, sahur-sort-brain-rot-500603 and atelier-novels-494413 still have a Firebase link. The user removes it in the console.

## Standing rules

- No Bash/shell tool; use the context-mode `ctx_execute` tools.
- Delegate to background subagents: Sonnet/Haiku by default, Opus for hard driver work. Never model-bridge.
- Never kill processes you didn't start. Never touch Chrome.
- Never uninstall apps or clear app data without approval. Keep package ids, slot ids, the `_rc5.so` filename and old tags.
- No tags or releases until every arch passes.
- Commit and push to main after every change:
  - use plain `git commit` and never override the email;
  - end the message with `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`;
  - run `git pull --rebase` before push.
  - "biblioklept" in commit subjects is intentional.
- New patches go at the end of jm-v7-wip; check them with a strict prepare.
- Builds use at most 4 jobs. Big files go under `/var/tmp/panvk` (`/tmp` is a small tmpfs).
- Downloads are untrusted: put them in new empty dirs and run Python with `-I`. Never commit `video.mp4`.
- PanPlay test rule: the full suite 1-9 runs on rc4 and on the newest build; intermediate builds get PanProbe only.
- driverInfo format: `Mesa 26.3.0-devel (git-<sha>, p<NNNN>, <date>)`.

## Rebuild on the Mac / not in git

| What | Linux path | How to get it on the Mac |
|---|---|---|
| Mesa trees | `/var/tmp/panvk/mesa-121-final` (also `mesa-oom2-final`, `-dev`) | `scripts/prepare-rc5-tree.sh --wip-max 0163` on Mesa `5a07217f` |
| Driver builds | `/var/tmp/panvk/build-out/2026-10-29-{120,oom2final,121b}/` | rebuild, or copy (about 30 MB each) |
| APKs | `/var/tmp/panvk/build-out/apks/*-{120,oom2final,121b}.apk` (PanPlay about 440 MB) | rebuild with Gradle; recipe in `/var/tmp/panvk/b120/apks-120.sh` and `/var/tmp/panvk/b121/` |
| Test scripts | `/var/tmp/panvk/fps119/play-cfg.sh`, `b120/suite.json`, `rtl-oom2final/run.sh`, `ftl120/` | copy; paths need editing |
| Full v13 logcat | `/var/tmp/panvk/rtl-oom2final/logcat-stream.txt` (over 100 MB) | the greps are in the run dir |
| MangoHud component | `scripts/build-mangohud.sh`, `scripts/components.env` (in git) | rebuild |
| Claude memory | `~/.claude/projects/-home-abhaybyte-repos-panvk/memory/` | copy privately; contains account names |
