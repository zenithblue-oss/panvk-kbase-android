# 2026-10-22 v7N: v7M + v6 ballot without WMASK (RC5-dev-v7N)

Tree: `/home/flux/src/mesa-v7J`, branch `v7N`, HEAD `c68c685`, on top of v7M `bd6d38d` (0006-0075). Build only; no device run yet.

## Root cause of the 3 remaining FTL G72 failures (v7M 35/38)

All three were compute atoms ending with `event=0x51 core_req=0x16` (INSTR_INVALID_ENC). The cause is the instruction **WMASK** (`+WMASK.subgroup4 rN, src, fill:0`), which the Bifrost backend emitted for every `nir_intrinsic_ballot` on all Bifrost. Mali-G72 (v6) has no WMASK. It is a v7 instruction, like the full CLPER, and it shares CLPER's subgroup2/4/8 modifier.

Method: a host panvk build (`/home/flux/build-v7feat/host-v7N-shim`: `-Dvulkan-drivers=panfrost -Dpanfrost-kmds=panfrost,kbase -Dtools=drm-shim`, debugoptimized so asserts are on) runs under the panfrost noop drm-shim with `PAN_GPU_ID=6221`. The harness `tools/bifrost-host-compile/hc.c` creates the PanProbe pipelines from the test SPIR-V headers, and `BIFROST_MESA_DEBUG=shaders` dumps the disassembly. `tools/bifrost-host-compile/ops.py` lists the unique opcodes.

Unique opcodes per shader, v7M (0x6221):
| shader | where WMASK comes from | opcodes not in passing shaders |
|---|---|---|
| shader_arith subgroup_size_control (`subgroupAdd(1u)`, required size 4, full subgroups) | active-lane mask (`ballot(true)`) in nir_lower_subgroups reduce/scan, plus the nir_lower_shuffle waterfall (read_first_invocation -> ballot + find_lsb + CLPER_OLD) | WMASK.subgroup4 |
| sampler gather_ext (gather_offsets) | waterfall over the dynamic `textureGatherOffset` offset (ballot + CLZ + CLPER_OLD), before the TEXC | WMASK.subgroup4 |
| vkd3d_heap image_heap | the 085 texture waterfall over the NonUniform heap index | WMASK.subgroup4 |
| vkd3d_heap texel_heap (PASSES on G72) | none (texel buffer path, no waterfall) | none |

The other opcodes in the failing shaders are valid on v6. CLPER_OLD.i32 is the G71/G72/G31 encoding (BIFROST_LIMITED_CLPER, also used for derivatives). CLZ, MUX, ICMP, CSEL, LSHIFT_*, TEXC, U8/U16_TO_U32, F32_TO_S32 and LOAD/STORE all occur in shaders that pass. The 0068 read_invocation fix (CLPER_OLD) and the 0075 offset pack were correct but not enough. The 0068 README note that "Vote/ballot are WMASK (on all Bifrost)" was wrong.

## Patch relative to v7M

- 0076 (jm-v7 103, v6, compiler): on arch 6, `nir_intrinsic_ballot`/`ballot_relaxed` is built from CLPER_OLD. Each lane computes `src & (1 << lane_id)` and the ballot is the OR of `CLPER_OLD(x, 0..3)`. CLPER_OLD returns zero for an inactive lane, so only active lanes contribute. This covers ballot, vote any/all, the reduce/scan active mask, first_invocation, read_first_invocation, elect and every waterfall. `bi_pack_clause` asserts that no WMASK reaches a v6 clause.
- v7 and v9+ are unchanged. G31 is v7 with LIMITED_CLPER and keeps WMASK; it is untested.

Host verification after the fix:
- 0x6221: subgroup_size_control, gather_ext and image_heap contain no WMASK. Each ballot is now 4x `+CLPER_OLD.i32` + 3x `*LSHIFT_OR.i32`. No packer assert fired (asserts are enabled).
- 0x7212 (G52, v7): image_heap still `+WMASK.subgroup8` + `+CLPER.i32.subgroup8.zero`, the same as before.
- Not checked: subgroup_size_control on v7. Required size 4 is below the v7 subgroup size of 8, so vk_set_subgroup_size asserts on the host; the device test skips that size there.
- Unit tests: the bifrost gtests (`bifrost/test`) cover packing, the scheduler and optimizer passes. They do not cover NIR intrinsic emission (the emit functions are static in bifrost_compile.c), so no v6 case was added. The harness above is the regression check.

## Build

- Thermal NONE, 4 jobs, fresh BDIR, exit 0:
  ```
  flock /home/flux/build-cache/build.lock env MESA=/home/flux/src/mesa-v7J BDIR=/home/flux/build-v7feat/bionic-r30-v7N HOST_TOOLS=/home/flux/repos/panvk-kbase-android/build/host-tools/bin PANVK_KBASE_RELEASE=RC5-dev-v7N PANVK_JOBS=4 scripts/build-android.sh --profile g615-v11-csf --api 29 --ndk /opt/android-sdk/ndk/30.0.16248370
  ```
- .so: `/home/flux/build-out/2026-10-22-v7N/libvulkan_panfrost.so`, sha256 `5e5726514f83b1d9676e2b25020b93a74658086529e64f4818da5713e5e60663`. `strings`: `PanVK-kbase RC5-dev-v7N (Mesa 26.3.0-devel (git-c68c685980))`.
- APK:
  ```
  apps/panvk-test/gradlew -p apps/panvk-test assembleDebug -PpanvkSoRc4=/home/flux/build-out/rc4/libvulkan_panfrost-android-aarch64.so -PpanvkSoRc5=/home/flux/build-out/2026-10-22-v7N/libvulkan_panfrost.so -PpanprobeSuffix=v7n
  ```
  - `/home/flux/build-out/apks/panprobe-v7N.apk`, sha256 `0bf8f3bbcde52fb1e67447f60e0a8878846e8187af4588b70daced6e395fa9ef`.
  - Package `dev.zenithblue.panvktest.v7n` (aapt2 badging, versionCode 15, versionName 1.2.7).
  - The sha of `lib/arm64-v8a/libvulkan_panfrost_rc5.so` matches the .so (5e572651...).
  - The first assemble's output was overwritten by a concurrent `bc4` gradle build in the same app dir (package bc4, wrong rc5 sha). That copy was deleted, the APK was rebuilt and re-verified.

## Firebase (not yet run)

```
gcloud firebase test android run --type game-loop --app /home/flux/build-out/apks/panprobe-v7N.apk --scenario-numbers 2 --device model=starlte,version=29 --timeout 30m --directories-to-pull /sdcard/Android/data/dev.zenithblue.panvktest.v7n/files --results-dir panvk-v6-starlte-v7N
```

Check on G72: shader_arith `subgroup_size_control`, sampler `gather_offsets`, vkd3d_heap `image_heap`. All 35 v7M passes should hold. Expected: 38/38.
