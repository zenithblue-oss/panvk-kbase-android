# Staging 115, integration (p115-int): P-A + P-B + P-C on the 233 base, upstream-style driver name

> **Promoted (2026-10-26):** these 1 patch (0001) are now `patches/jm-v7-wip/0120` (the same files, renumbered in apply order: patch 000N here is jm-v7-wip 119 + N). `scripts/prepare-rc5-tree.sh` applies them (default `--wip-max 0131`, the build 115 tree); see `patches/jm-v7-wip/README.md`, section "Build 115". The `patches/staging-115/...` paths and the apply/export commands below are from before the move.

The build-115 integration branch. It carries no package of its own: it is the series with csf-v11 233,
then the three finished packages unchanged, then the commits in this directory.

## Application order (full)

1. Mesa `5a07217f` (26.3.0-devel), fresh worktree:
   `git -C /home/flux/src/mesa worktree add -b p115-int /home/flux/src/mesa-115-int 5a07217f`
2. Series: `scripts/prepare-rc5-tree.sh --mesa /home/flux/src/mesa-115-int --csf-max 233 --wip-max 0092`
   (`common android kbase-common app-loader wsi csf csf-v11<=233 jm-v9 jm-v7`, one commit per patch, then
   `git am` of `jm-v7-wip` 0006-0092).
   Printed `BASE-OK applied=228 csf-v11<=233 head=d48019c7` and `OK base=228 jm-v7-wip=87 head=a4858f73`,
   with no PATCH-DRIFT, WIP-DRIFT, offset or fuzz lines (only the old jm-v9/002 trailing-whitespace warning).
   Tree `dee163e5` = the fps-log tree (rc5g-114 + 233). Tag `p115-int-base` (`a4858f73`).
3. `git am patches/staging-115/pa/*.patch` (10), then `pb/*.patch` (10), then `pc/*.patch` (7).
   Each patch was checked with `git apply --check -v` first: all 27 applied with no offset, fuzz or
   conflict, so nothing was resolved by hand. `git diff p115-pc p115-int-pc` is exactly 233
   (`panvk_wsi.c`, +277 lines). Tag `p115-int-pc` (`20364bac`).
4. `git am patches/staging-115/int/*.patch` (this directory). Branch head `p115-int`.

Export: `git -C /home/flux/src/mesa-115-int format-patch p115-int-pc..p115-int -o patches/staging-115/int/`.

P-D (JM v6/v7/v9, paused) should rebase onto `p115-int` (or apply after step 4).

## Commits

| # | Title | Knob |
|---|---|---|
| 0001 | panvk: report driverInfo like upstream Mesa ("Mesa <version> (git-<sha>)") | none (no functional change) |

### 0001 upstream-style driver identity

csf-v11/099 reported `driverInfo` = `PanVK-kbase <release> (Mesa 26.3.0-devel (git-<sha>))`, with the release
name from the `PANVK_KBASE_RELEASE` build define (`scripts/build-android.sh`), and csf-v11/233 copied that
string into the `PANVK_FPS_LOG` header. For the first GitHub release the driver identifies itself exactly like
an upstream Mesa PanVK development build:

| Field | Before (114) | Now | Upstream main |
|---|---|---|---|
| `driverName` | `panvk` | `panvk` | `panvk` |
| `driverInfo` | `PanVK-kbase RC5-universal-114 (Mesa 26.3.0-devel (git-d324a2699e))` | `Mesa 26.3.0-devel (git-<sha10>)` | `"Mesa " PACKAGE_VERSION MESA_GIT_SHA1` |
| `deviceName` | `Mali-G715 MC7` (shiba) | unchanged | `pan_model_device_name()`: `"%s MC%u"`, Immortalis names by core count |
| fps log header | `# panvk-fps PanVK-kbase <release> (Mesa ...) pid=` | `# panvk-fps Mesa 26.3.0-devel (git-<sha10>) pid=` | n/a |

- `PACKAGE_VERSION` comes from the Mesa base's `VERSION` file (26.3.0-devel) and `MESA_GIT_SHA1` from
  `bin/git_sha1_gen.py` (the tree's HEAD, 10 hex digits), as upstream. The 115pre build reports
  `Mesa 26.3.0-devel (git-6a5d4e2756)`.
- deviceName: the series (csf-v11/131, 187) names Immortalis and small-core parts by core count; upstream main
  now does the same for G715/G720/G925 Immortalis (`pan_model.c` rows with `min_core_count = 10`). For every GPU
  tested so far (G615 MC6, G715 MC7, G710 MC7, G610 MC3, G78, G57, G52, G72) the string is identical to
  upstream's.
- `PANVK_KBASE_RELEASE` is no longer read. `build-android.sh` still passes it when set (so trees up to
  rc5g-114 rebuild with their old strings); here it has no effect. Build identity: the git sha in driverInfo,
  the .so sha256, and the worklogs (`worklogs/g615-dxvk/builds/rc5-universal.md`).
- PanPlay's fps-log parser (`# panvk-fps (.*?) pid=`) reads both header forms.

## Build (115pre)

- Fresh BDIR `/home/flux/build-rc5/bionic-r30-115pre`, `--profile g615-v11-csf --api 29`,
  NDK 29.0.14206865 (clang r563880c), `HOST_TOOLS=/home/flux/repos/panvk-kbase-android/build/host-tools/bin`,
  4 jobs, under the build lock, no `PANVK_KBASE_RELEASE`. 10:22-11:58 UTC.
- `/home/flux/build-out/2026-10-26-115pre/libvulkan_panfrost.so`, 29903696 bytes, sha256
  `652af311e78c3846018923d4838e02bbed638771facb5aea5625ec1f98108f93`, driverInfo
  `Mesa 26.3.0-devel (git-6a5d4e2756)`. 8 `libpanvk_v*.a`. Warnings: the nine known ones
  (`csf/panvk_vX_gpu_queue.c:859` x5, `jm/panvk_vX_gpu_queue.c:182` x3, `BacktraceMap.h:74` x1).
- All 21 A/B knob names of pa/pb/pc are in the .so (see the env sets below).

## A/B env sets (114 behaviour)

Every default-on knob of pa (8), pb (8) and pc (5) at its 114 value; opt-in knobs
(`PANVK_KBASE_COHERENT_POOLS`, `PANVK_KBASE_EVICTABLE`, `PANVK_KBASE_HEAP_SLOTS`) unset in both sets:

```
PANVK_CSF_FS_IDLE_CLEAN=0 PANVK_DB_ALT=0 PANVK_CSF_INDIRECT_REEMIT=0 PANVK_CSF_DESC_DIRTY=0
PANVK_VB_DIRTY_CMP=0 PANVK_PRERAST_FAST_SKIP=0 PANVK_KBASE_STRICT_DEVICE_CLOCK=0 PANVK_DESC_POOL_HINT=0
PANVK_KBASE_HEAP_EAGER_TERM=0 PANVK_TESS_VS_REGION_MB=32 PANVK_KBASE_ZERO_ALL=1 PANVK_CS_FLUSH_USED=0
PANVK_JM_CMD_POOL_AUTOTRIM=0 PANVK_MEM_BUDGET_PRESSURE=0 PANVK_CS_CHUNK_KB=64 PANVK_CMD_SLAB_CACHE=0
PANVK_WSI_SW_CACHED=0 PANVK_WSI_SW_DEFER_REPLY=0 PANVK_ANDROID_PRESENT_FENCE=0
PANVK_ANDROID_PRESENT_THREAD=0 PANVK_PRELOAD_BARRIER_TRACK=0
```

`PANVK_ZS_PRELOAD_ALWAYS` is left out: on v11 the 114 behaviour is the unset default (csf-v11/061 already used
INTERSECT there), and `=1` would select the pre-061 mode. On v9/v10 add `PANVK_ZS_PRELOAD_ALWAYS=1`.
These are PanProbe `assets/env-ab.json` pass `114-knobs` and PanPlay `assets/gameloop.json` env set `old`.

## Device results (115pre, Firebase shiba: Pixel 8, Mali-G715 MC7, v11)

- **PanProbe env A/B** (`device-runs/2026-10-26-ftl-v11-shiba-115pre`):
  - `defaults` and `114-knobs` both 39/39, with no faults;
  - driverInfo `Mesa 26.3.0-devel (git-6a5d4e2756)`, driverName `panvk`;
  - tess_perf hashes equal to 114;
  - defaults total time -0.9% vs 114.
- **PanPlay cubes A/B** (`device-runs/2026-10-26-ftl-v11-shiba-panplay-115pre`, scenario 4): all 4 passes `CUBE: PASS`.
  Driver fps by pass:

  | cube | old | default | change |
  |---|---|---|---|
  | d3d11 | 46.8 | 47.5 | +1.4% |
  | d3d9 | 49.7 | 48.9 | -1.7% |

  Both changes are within the noise, so there is no regression.
- **Open: d3d9 HUD.** The DXVK HUD text is garbled in both d3d9 passes. The cubes and the pixel check are fine.
  The `old` set shows it too, and there is no shiba 114 PanPlay baseline yet.
- **PanPlay D3D12** (scenarios 5, 6): both FAIL. vkd3d-proton's first `vkCreateSwapchainKHR` raises
  `Exception 0xc0000005 in Unix call`, then exit 3. Nothing was presented.
  - These are the first D3D12 presentation tests on any driver. There is no baseline, and they ran only with `default`.
  - The rerun plan (override file with `old`, `+seh` and vkd3d extension A/B sets) is in that run's results.md.
