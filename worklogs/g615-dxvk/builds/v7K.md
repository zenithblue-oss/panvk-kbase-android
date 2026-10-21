# 2026-10-21 v7K: perf pass (RC5-dev-v7K)

Tree: `/home/flux/src/mesa-v7J`, new branch `v7K` on top of v7J `f69289a` (0006-0058), plus 0059-0062 below. Goal: faster v7 without regressing the A06 v7J result of 37 pass / 0 fail / 1 skip. Nothing was run on a device; this is a build only.

## Patches

- **0059 (jm-v7 086): descriptor loads at a constant set offset are reorderable on v6/v7** (`panvk_vX_nir_lower_descriptors.c`, `load_resource_deref_desc`).
  - **Problem:** on v6/v7 every descriptor field (image size, texture size, levels, samples, texel buffer size) is a `load_global` from set memory with no access flags. CSE and GCM therefore kept every copy.
  - **Why RIA2 was slow:** the 048 RIA2 pass (patch 0035) calls `image_deref_size` before each access. So the A06 bench, with 16 imageLoads, did 16 descriptor loads, each in its own clause. Shaderdb: 409 vs 210 instructions, 75 vs 28 clauses, 18 vs 13 registers. ria2 took 236 ms vs 176 ms for ria1.
  - **Fix:** these loads now carry `ACCESS_NON_WRITEABLE | ACCESS_CAN_REORDER`, so the 16 size queries collapse into one, hoisted. Set memory is constant while a shader runs, and a constant offset into a bound set is valid memory even when hoisted.
  - **Not changed:** loads with a dynamic array index stay pinned, because a hoisted load could read past the array.
  - **Side benefit:** textureSize, imageSize and textureQueryLevels also get CSE'd.
  - **Expected effect:** about 15 fewer clauses and about 120 fewer instructions in the bench. The ria2-ria1 gap should shrink to the compare/select cost: per load 2 ult, 1 iand, 1 coord csel and 1 alpha csel (the x compare is shared). Measure it on the device.
- **0060 (jm-v7 087): the waterfall now fires only on NonUniform indices.**
  - **What 085 (0058) did:** it waterfalled every tex whose texture or sampler offset divergence analysis could not prove uniform.
  - **Why that was too broad:** that analysis is conservative. A non-NON_WRITEABLE `load_global`/SSBO index "may tear", and phis in divergent control flow also count as divergent. Without NonUniform, the spec requires the index to be dynamically uniform. Before 085 such texs were not lowered at all, since `nir_lower_non_uniform_access` only takes flagged ones. So those shaders paid for ballots, read_invocation and a loop for nothing.
  - **New rule:** a source is waterfalled only if it is divergent **and** has `texture_non_uniform` or `sampler_non_uniform` set. vtn sets these flags from NonUniform; vkd3d_heap image_heap uses `nonuniformEXT`, so it still takes the waterfall.
  - **Cleanup:** after the loops, `copy_prop`, `algebraic`, `constant_folding` and `dce` now run.
- **0061 (jm-v7 088, review M9): BeginQuery no longer marks its batch `after_frag`.**
  - **Problem:** that flag turned PANVK_OVERLAP off for every render pass that begins an occlusion or pipeline-statistics query, which DXVK does constantly.
  - **Why it is safe:** BeginQuery only zeroes its own reports, plus a fresh pstats scratch. Between uses the query was reset, either by CmdResetQueryPool or on the host:
    - CmdResetQueryPool runs in its own `after_frag` batch, and this batch's vtc chain follows it through `last_vtc_atom`.
    - A host reset requires the previous use to have completed.
  - **Not changed:** End, WriteTimestamp, Copy and Reset keep `after_frag`. v9 is unchanged.
- **0062 (jm-v7 089, review M11): unsampled R/B-swapped images keep AFBC on v7 when `customBorderColorWithoutFormat` is enabled.**
  - **Rule:** images without SAMPLED or INPUT_ATTACHMENT usage keep AFBC; sampled images are unchanged.
  - **Why it is safe:** a border color can only be seen through a sampled view. The texture views of these images (meta blit and copy) already get the AFBC reswizzle from the image modifier, as 044 does.

csf-v11 gets no new patches. None of these items apply to v10:
- v9+ descriptor reads are `load_ubo`, which is already reorderable.
- The waterfall, M9 and M11 are v6/v7 JM code.

## Env toggles for device A/B

Every toggle is read once per process. **Use `MESA_SHADER_CACHE_DISABLE=true`** for the shader toggles; otherwise the disk cache can return shaders compiled with the other setting.

| Toggle | Effect |
|---|---|
| `PANVK_V7_DESC_REORDER=0` | 0059 off (old pinned descriptor loads) |
| `PANVK_TEX_WATERFALL=all` | 085 selection (every divergent index) |
| `PANVK_TEX_WATERFALL=0` | no waterfall (pre-085) |
| `PANVK_V7_QUERY_BEGIN_SERIAL=1` | 0061 off (BeginQuery marks after_frag) |
| `PANVK_V7_CBC_AFBC=0` | 0062 off (044 rule: no AFBC for any R/B-swapped image) |
| `PANVK_V7_RIA2=0` | (existing, 0053) do not expose robustImageAccess2 |

Suggested device checks:
- **Full autorun:** expect 37/0/1.
- **RIA2 cost:** the `robust_image_access2` BENCH line (ria2 vs ria1 ms and stats). Run it once with `PANVK_V7_DESC_REORDER=0` for the old number.
- **Waterfall:** `vkd3d_heap` must still pass. If it fails, rerun with `PANVK_TEX_WATERFALL=all`.
- **Queries:** `occlusion_query` and `pipeline_stats`. If either fails, rerun with `PANVK_V7_QUERY_BEGIN_SERIAL=1`.
- **AFBC:** `blend`, `bachata_reqs` and `sampler`. If any fails, rerun with `PANVK_V7_CBC_AFBC=0`.

## Review items not done (and why)

- **H5** (availability visible before the report data): it is a correctness item and needs a CACHE_FLUSH job, which costs time. The data race only matters for non-WAIT reads while the job is running. occlusion_query and pipeline_stats pass. It needs a device.
- **M8** (native 64-bit atomics): fixing `bi_emit_atomic_i64_to` staging and dropping the CAS-loop fallback is compiler work, with a hang risk if wrong. It needs a device test.
- **M12** (RIA1 OOB store/atomic guard): it is a correctness fix that adds cost to stores. RIA2 is the default since 0053, so only apps or pipelines that ask for RIA1 are affected. It needs a device test.
- **M13** (null storage image stores at large coordinates): this is a device test item, not a code change.
- **M15** (BC raw store I/O, csf-v11/201): this concerns memory and record-time I/O policy (spill directory, skipping the pread on full-coverage writes). It is not a GPU perf item, and it needs a device to see whether it spills at all.
- **M16** (GS/TCS compute job without a chain barrier): this needs a device. The barrier also orders against earlier jobs in the chain, such as the xfb offset chain and gs_pool reuse, and dropping it could cause a hang or wrong output.
- **RIA2, beyond 0059:**
  - Clamping by hardware alone is not enough. Coordinates wrap at 16 bits, so a bounds compare is still needed, and the OOB alpha differs from the hardware's (0 vs 1 for rgba8). The coordinate redirect and alpha select are already one csel each.
  - The pass already skips bindless images, MS and subpass images, and pipelines whose image robustness is not RIA2.
  - No host shaderdb was available: there is no host panvk or drm-shim build. The instruction and clause counts above are expected values, not measured ones.

## Build

- **Mesa tree:** HEAD `d160802` on branch `v7K`. Patches 0006-0062 in file order are in `patches/jm-v7-wip`.
- **Command:**
  ```
  flock /home/flux/build-cache/build.lock env MESA=/home/flux/src/mesa-v7J BDIR=/home/flux/build-v7feat/bionic-r30-v7K HOST_TOOLS=/home/flux/repos/panvk-kbase-android/build/host-tools/bin PANVK_KBASE_RELEASE=RC5-dev-v7K PANVK_JOBS=4 scripts/build-android.sh --profile g615-v11-csf --api 29 --ndk /opt/android-sdk/ndk/30.0.16248370
  ```
  - BDIR was fresh. 4 jobs were used because thermal status was NONE.
  - Exit code 0. There are no warnings in the changed files; the only warnings are the old csf/jm gpu_queue ones.
  - Log: `/home/flux/build-v7feat/bionic-r30-v7K.log`.
- **.so:** `/home/flux/build-out/2026-10-21-v7K/libvulkan_panfrost.so`
  - sha256 `badc28eb31786c33495245395f62edcbbfb45d86f73f6e88edb9c8e57b36de4b`
  - Contains the RC5-dev-v7K string.
- APK: `apps/panvk-test/gradlew -p apps/panvk-test assembleDebug -PpanvkSoRc4=/home/flux/build-out/rc4/libvulkan_panfrost-android-aarch64.so -PpanvkSoRc5=/home/flux/build-out/2026-10-21-v7K/libvulkan_panfrost.so -PpanprobeSuffix=v7k` (under the lock, current main MainActivity with the v7J2 swapchain harness fix: Tests tab, keep screen on, turnScreenOn/showWhenLocked): BUILD SUCCESSFUL. `/home/flux/build-out/apks/panprobe-v7K.apk` sha256 `c8231e008e36794aea7d339c78af1448a63a273c2d03a1d41e557d57c6349af6`, package `dev.zenithblue.panvktest.v7k`; its `lib/arm64-v8a/libvulkan_panfrost_rc5.so` = badc28eb... (verified, matches).
- Autorun: `--es driver rc5 --es autorun all`; targeted: `robust_image_access2,vkd3d_heap,occlusion_query,pipeline_stats,blend,sampler,bachata_reqs`.
