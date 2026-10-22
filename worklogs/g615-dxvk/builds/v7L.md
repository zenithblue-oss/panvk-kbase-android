# 2026-10-22 v7L: perf-review items + v6 safety (RC5-dev-v7L)

Tree: `/home/flux/src/mesa-v7J`, branch `v7L` on top of v7K `d160802` (0006-0062), plus 0063-0073 below (Mesa HEAD `a724b88`). Source: `worklogs/g615-dxvk/perf-review-2026-10-22.md`. Build only; no device run yet.

## Patches

| Patch | jm-v7 | Item | Change | Toggle (old behaviour) |
|---|---|---|---|---|
| 0063 | 090 | #6 | VS storage-image table: the VS copy-desc job also runs when the VS has storage images, so redraws without rebind no longer see a stale table. New PanProbe case `vertex_stores/vs_img_redraw` (64x1 R32_UINT, 4 draws, no rebind; runs in `autorun all`). | fix, none |
| 0064 | 091 | cache key | v7 compile knobs (tex waterfall, IDVS, desc reorder) resolved once (`panvk_v7_knobs.h`) and hashed into the shader cache key. Shader toggles no longer need `MESA_SHADER_CACHE_DISABLE`. | fix, none |
| 0065 | 092 | v6 | IDVS quirk on all of arch 6 (was G72/G76 only, so G71 kept IDVS). | `PANVK_V7_IDVS=1` |
| 0066 | 093 | v6 | kbase atom wait deadlines on CLOCK_MONOTONIC (vk_sync clock). | `PANVK_V7_RAW_CLOCK=1` |
| 0067 | 094 | v6 | dma-buf import clean available on v6/v7 but **opt-in** (a full per-submit DMA_BUF_IOCTL_SYNC is not cheap); v9 default unchanged. | `PANVK_KBASE_IMPORT_SYNC=1` enables on v6/v7 |
| 0068 | 095 | v6 | `read_invocation` emits CLPER_OLD.i32 under BIFROST_LIMITED_CLPER (G71/G72/G31). Plain CLPER.i32 is invalid there; likely cause of FTL G72 `shader_arith subgroup_size_control` and `vkd3d_heap image_heap` DEVICE_LOST (waterfall uses read_invocation). | fix, none |
| 0069 | 096 | #2 | min/max helper: strided search kernel with a local-memory tree reduction (up to 64 WGs x 64 threads), no WRITE_VALUE job when one WG suffices; for small direct indexed draws (<= 4096 indices, VS without global writes) the search is folded into the indirect helper. | `PANVK_V7_MINMAX=nofold` (no fold) / `=0` (old) |
| 0070 | 097 | #4/#13 | per-set descriptor dirty tracking (bind/push/template report changed sets; identical rebinds skip), push uniforms re-emitted only when dirty, attribute table cached on (vertex count, instance base/count, offset, attrib count). | `PANVK_V7_DESC_DIRTY=0`, `PANVK_V7_PUSH_CACHE=0`, `PANVK_V7_ATTR_CACHE=0` |
| 0071 | 098 | #12 | rebinding a shader whose alternate is bound is not a change (no VS/FS dirty flip). | `PANVK_V7_ALT_REBIND_DIRTY=1` |
| 0072 | 099 | #7 (K2) | with PANVK_OVERLAP, one 64 MiB GROW_ON_GPF tiler heap BO per half. | `PANVK_V7_SPLIT_HEAP=0` |
| 0073 | 100 | #10 | occlusion EndQuery in an open batch no longer splits it; availability writes (<= 64) deferred to an after-frag job at batch close. | `PANVK_V7_QUERY_END_DEFER=0` |

All toggles are read once per process.

## v6 review (FTL G71/G72 priority)

- **Runtime gate:** `panvk_physical_device_init_kbase` already accepts v6/v7 without any env var (jm-v7 002); only the DRM path is gated. No change.
- **48-byte atom fallback (0018):** reviewed, correct; the FTL G72 run logs "stride 48" and runs 33 tests.
- **Subgroups:** vote/ballot use WMASK, present on all Bifrost; no gating needed. The real bug was CLPER.i32 (0068). Known gap: G31 (v7, subgroup 8) still gets subgroup4-only CLPER_OLD.
- **G71 FP32 transcendentals:** BIFROST_NO_FP32_TRANSCENDENTALS lowering covers frcp/frsq/exp2/log2/pow; libpan kernels are compiled for gpu_id 6<<28 (G71 quirks). OK.

## FTL G72 (v7K) failures and expected v7L effect

- vertex_stores `ImgWait r=-4` (vs_img_redraw DEVICE_LOST) -> 0063.
- shader_arith subgroup_size_control DEVICE_LOST -> 0068.
- vkd3d_heap image_heap `vkWaitForFences r=-4` -> 0068 (waterfall read_invocation).
- sampler: `compute wait r=-4` plus `gather_offsets got (-255,-255) exp (18,41)`. The DEVICE_LOST may be 0068; gather_offsets (lower_tg4_offsets path) is **not addressed**, open v6 item.
- robust_image_access2: 10 r32ui / r32ui_nofmt cases alpha 0 vs 1. **Not addressed**, open v6 item.

## Skipped

- #1, #3 (batch/barrier architecture): too large for this pass; later.
- #5 (present wait): JM exposes a single GPU queue, so a per-image present fence wait equals a queue idle; no gain. Real gains need AHB queue/dequeue work.

## Build

- **Mesa tree:** `/home/flux/src/mesa-v7J` branch `v7L`, HEAD `a724b88`. Patches 0006-0073 in file order are in `patches/jm-v7-wip`.
- **Command** (thermal NONE, 2 jobs, fresh BDIR):
  ```
  flock /home/flux/build-cache/build.lock env MESA=/home/flux/src/mesa-v7J BDIR=/home/flux/build-v7feat/bionic-r30-v7L HOST_TOOLS=/home/flux/repos/panvk-kbase-android/build/host-tools/bin PANVK_KBASE_RELEASE=RC5-dev-v7L PANVK_JOBS=2 scripts/build-android.sh --profile g615-v11-csf --api 29 --ndk /opt/android-sdk/ndk/30.0.16248370
  ```
  Exit code 0.
- **.so:** `/home/flux/build-out/2026-10-22-v7L/libvulkan_panfrost.so`, sha256 `3c582fb7d9397098de6ef245548c24d3b44db4957a54b5b02f1ac923d04ff013`. It contains the strings `RC5-dev-v7L` and `a724b88`.
- **APK:** built from repo `55d76bb`, which includes the Firebase game-loop MainActivity (6faf253).
  ```
  apps/panvk-test/gradlew -p apps/panvk-test assembleDebug -PpanvkSoRc4=/home/flux/build-out/rc4/libvulkan_panfrost-android-aarch64.so -PpanvkSoRc5=/home/flux/build-out/2026-10-22-v7L/libvulkan_panfrost.so -PpanprobeSuffix=v7l
  ```
  - Built under the lock: BUILD SUCCESSFUL.
  - `/home/flux/build-out/apks/panprobe-v7L.apk`, sha256 `6f527a2d3330cfe528364201ad78e6ff1eed83d9ceb10350451b3b3c550fe904`.
  - Package `dev.zenithblue.panvktest.v7l`.
  - Its `lib/arm64-v8a/libvulkan_panfrost_rc5.so` hash was checked and matches the .so (3c582fb7...).

## Device test plan

- **A06 (G52, v7):** full `--es driver rc5 --es autorun all`, expect 38 tests (37 + vs_img_redraw case inside vertex_stores) with no regression vs v7K. Then targeted A/B: `vertex_stores,occlusion_query,pipeline_stats,draw_indirect,vkd3d_heap,shader_arith` and DXVK perf with each toggle flipped (`PANVK_V7_MINMAX=0`, `PANVK_V7_DESC_DIRTY=0`, `PANVK_V7_PUSH_CACHE=0`, `PANVK_V7_ATTR_CACHE=0`, `PANVK_V7_SPLIT_HEAP=0`, `PANVK_V7_QUERY_END_DEFER=0`). Any failure: rerun with that toggle to bisect.
- **v6 FTL (Galaxy S9 G72; a G71 device if available):** game-loop scenario 2 (rc5). Check vertex_stores (vs_img_redraw), shader_arith, vkd3d_heap, sampler, robust_image_access2. Extra runs: `PANVK_V7_IDVS=1` (IDVS opt-in), `PANVK_KBASE_IMPORT_SYNC=1` (swapchain/import clean).
