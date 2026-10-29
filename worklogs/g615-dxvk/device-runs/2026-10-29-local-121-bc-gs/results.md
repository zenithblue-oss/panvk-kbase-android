# Build 121b (2026-10-29): cold-cache BC fix (0162) and v9 pass-through GS skip (0163)

Not the default. The bundled pins stay on build 120. 121b = jm-v7-wip 0006-0163, so it includes the tiler-OOM
0161 (CSF kbase only).

| Build | Patches | driverInfo | .so sha256 | BuildID |
|---|---|---|---|---|
| 120 | 0006-0160 | `git-94af3802af, p0160` | `1195a047...` | `6c106131...` |
| 121 (first try, superseded) | 0006-0163, 0162 v1 | `git-d1653642e2, p0163` | `0cee572e...` | `47b235a0...` |
| **121b** | 0006-0163 | `Mesa 26.3.0-devel (git-07177325ca, p0163, 2026-10-29)` | `9f58ee27ad8e4d0b02f43b26bada3e34da5feba5134f8ac1c91b23c4f9cf85ae` | `c5337130bf092b76764921dc321808a88f56df0c` |

- .so: `/var/tmp/panvk/build-out/2026-10-29-121b/libvulkan_panfrost.so`, 29458152 bytes.
- APK: `/var/tmp/panvk/build-out/apks/panprobe-121b.apk` (`3c09cc87...`, rc4 + 121b slots).
- Strict prepare `--wip-max 0163`: `OK base=228 jm-v7-wip=158 head=07177325cad`. The tree equals the dev worktree
  `/var/tmp/panvk/mesa-121-dev`.

"Cold" means only PanProbe's `cache/mesa_shader_cache` was removed, via `run-as`, right before the run
(`scripts/probe.sh`, `CLEAR=1`). There was no `pm clear`.

## Causes

- **Cold BC (0155):** the prewarm job compiled the per-region pipeline and then the XC_BATCH pipeline of every
  ETC2 mode. The upload waited on that one fence. `bc_decode` images have 2 layers, so they are never batchable
  and the second compile was wasted. 0162 prewarms the batch pipeline only for one-layer 2D images with mips.
  Other images compile it at first use.
  - The first try (121) dropped the batch prewarm entirely. That fixed `bc_decode` but `bc_level` regressed.
- **v9 GS:** 0157 hooked only the CSF `panvk_cmd_draw()`. On v9 the JM path (`v9_cmd_draw()`) never checked
  `pt_skip`. 0163 adds the same swap there.

## PanProbe (full, scenario 2)

All runs were 41/41 with no DEVICE_LOST.

| Device | Run | bc_decode ms | workload `bc_level` ms | `gs_pass` us/draw | `strip_gs` us/draw |
|---|---|---|---|---|---|
| X6 Pro v11 | 120 cold | **9346** | 2674 | 4.42 | 9.22 |
| X6 Pro v11 | 121 cold / warm | 4966 / 121 | 3256 / 3464 | 2.78 / 2.94 | 3.81 / 3.63 |
| X6 Pro v11 | **121b cold / warm** | **5155** / 180 | **2994 / 2544** | 3.10 / 2.36 | 6.31 / 4.31 |
| TB336FU v9 | 120 cold | 1639 | 1513 | 29.95 | **354.03** |
| TB336FU v9 | 121 cold / warm | 1629 / 125 | 1534 / 1507 | 6.09 / 6.02 | 16.38 / 18.77 |
| TB336FU v9 | **121b cold / warm** | 1610 / 130 | 1512 / 1517 | 6.95 / 10.75 | **17.61 / 15.81** |

Notes:
- `bc_level` creates its own device with `MESA_SHADER_CACHE_DISABLE`, so it always compiles. For reference,
  118 was 4917 ms.
- The v9 `bc_decode` cold time is small because the TB336FU decodes most BC formats natively or through compute,
  not through the ETC2 transcoder.

Raw results are in `raw/<device>-<build>-<cold|warm>/` (autorun, summary, meminfo). Screenshots are in `shots/`.
Scripts are in `scripts/`.

## Not done

- PanPlay was not run on 121b. 0163 changes v9 draws with a pass-through GS, which matters for DXVK/emulator
  games.
- There was no RTL or Firebase run: v10, v12-v14 are untested. 0162 is shared code; 0163 is v9 JM only.
