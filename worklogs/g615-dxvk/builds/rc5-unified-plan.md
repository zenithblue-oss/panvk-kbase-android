# RC5 unified tree and build recipe (v6, v7, v9, v10, v11 in one .so) — 2026-10-22

Status: the tree is prepared; nothing is built. The bc5 fix (csf-v11/209+) is
still in progress and holds the build lock, so no build was run.

Arch map: v6 = Mali-G72 (Galaxy S9 / starlte, FTL), v7 = Mali-G52 (A06),
v9 = Mali-G57/G77/G78 (JM Valhall), v10 = Mali-G610/G710 (CSF), and
**v11 = Mali-G615 MC6 (Poco X6 Pro, `panArch: 11` in `profiles/g615-v11-csf.json`)**.
The csf-v11 series is named after the G615, and its behaviour is verified on v11.

## 1. How the two series relate

- **They are one stack, not two parallel series.** jm-v7-wip is not an
  independent series on bare Mesa 5a07217. It applies with `git am` **on
  top of** the full `g615-v11-csf` series (see `patches/jm-v7-wip/README.md`).
- **What the base series contains.** The base is what
  `scripts/apply-patches.sh --profile g615-v11-csf` applies, in this order:
  `common android kbase-common app-loader wsi csf csf-v11 jm-v9 jm-v7`. The
  `jm-v7` dir holds jm-v7 001-005. The profile special case
  (`[ "$PROFILE" = "g615-v11-csf" ]`) is what adds jm-v9 and jm-v7. That
  makes this profile the universal ICD.
- **The profile does not choose patches at build time.**
  `scripts/build-android.sh` never applies patches. `--profile` only names
  the default DDIR, and the .so is whatever tree `MESA=` points at. So
  "built with `--profile g615-v11-csf`" says nothing about patch content. The
  tree does.
- **The v7K/v7N builds contained the csf-v11 BC patches, up to 202.**
  `/home/flux/src/mesa-v7J` branches v7K and v7N sit on `ab7d630
  base-series`. That is a single squashed commit on 5a07217 holding the
  197-patch series: csf-v11 018-202, jm-v9 001-031 and jm-v7 001-005. This
  covers the BC lowmem series through 202 (ETC2/EAC transcode, raw store,
  BC6H to ASTC HDR, etc.). It **lacked csf-v11 203-208**:
  - 203: BC upload cost on the GPU
  - 204: Android cache dir for execve()d helpers
  - 205: lazy gpu_prerast arenas
  - 206: EAC endpoint-pair LUT
  - 207: lazy BC raw store
  - 208: BC6H SF to ASTC opt-in

  v7K (`d160802`, jm-v7-wip 0006-0062) and v7N (`c68c685`, 0006-0076)
  differ only in jm-v7-wip.
- **bc4 had no jm-v7-wip at all.** It is the 203-patch series (csf-v11
  through 208 + jm-v9 + jm-v7 001-005), with no 0006+. It was built from
  `/home/flux/src/mesa-bc4-strict`. That tree is byte-identical to
  `rc5-base` below, which was checked with a full tree compare that includes
  untracked files.
- **Which .so served which device:**

  | device | arch | build | .so sha256 | tree |
  |---|---|---|---|---|
  | Poco X6 Pro (G615 MC6) | **v11** CSF | RC5-dev-bc4 | `a7b7c58e…1993` | 203-patch series, no jm-v7-wip |
  | A06 (G52 MC2) | v7 JM | RC5-dev-v7K (38/38) | `badc28eb…de4b` | ab7d630 (csf-v11 ≤202) + 0006-0062 |
  | S9 / starlte (G72) | v6 JM | RC5-dev-v7N (38/38) | `5e572651…0663` | ab7d630 (csf-v11 ≤202) + 0006-0076 |

  These were two different .so files. No single binary has passed on both
  CSF and JM yet.

## 2. Unified tree: `/home/flux/src/mesa-rc5` (branch `rc5`)

- **Worktree.** It was made with
  `git -C /home/flux/src/mesa worktree add -b rc5 /home/flux/src/mesa-rc5 5a07217f`.
- **Patches.** `scripts/prepare-rc5-tree.sh --mesa /home/flux/src/mesa-rc5 --base-tag rc5-base`
  adds one commit per base-series patch, using the apply-patches.sh order
  and strictness, with csf-v11 capped at 208. It then runs `git am` on
  jm-v7-wip 0006-0076.
- **The csf-v11 patches are not all `git am`-able.** Older ones (018…) are
  plain diffs with a `Purpose:` header, and some need `--recount`. So the
  base is committed per patch with `git apply`. Only jm-v7-wip, which is
  `format-patch` output, goes through `git am`.
- **Refs:**
  - `rc5-base` = `d2fb9ab751adb8c408bdc86bed6f14bee351c08e` (203 patches
    + 2 `files/` commits). This is the bc4 tree.
  - **HEAD = `4554612f25fd486e1b11b0a03273829e2ad02e74`** (`jm-v7 103: v6
    ballot without WMASK`). That is rc5-base + 71 jm-v7-wip commits.

### Conflicts and refreshes

The only cause was csf-v11/205, which is new relative to the v7 base. It adds
`util/log.h` and `util/os_misc.h` to the includes of `panvk_vX_device.c`, and
it adds the lazy-arena code in `panvk_vX_device.c` and `panvk_device.h`. The
other base files 203-208 touch (bc/*, panvk_image.c, cmd_meta, disk_cache)
caused no conflict.

- **Real conflict: 0030 (jm-v7 043).** Its include hunk wanted to add
  `util/u_debug.h` between `os_file.h` and `u_printf.h`. The resolution
  keeps both, in this order: `util/log.h`, `util/os_misc.h`,
  `util/u_debug.h`, `util/u_printf.h`. No code change.
- **Offset only, with hunk headers re-exported:**
  - 0010 (panvk_device.h, panvk_vX_device.c)
  - 0013 (panvk_vX_device.c)
  - 0035 (panvk_vX_device.c)
  - 0043, 0045 (jm/panvk_vX_cmd_draw.c)
  - 0046, 0047, 0048, 0050 (panvk_vX_shader.c)
  - 0051 (libpan/draw_helper.cl, panvk_vX_shader.c)
  - 0052 (jm/panvk_vX_cmd_draw.c, panvk_device.h, panvk_vX_device.c,
    panvk_vX_shader.c)
  - 0072 (panvk_vX_device.c)
- **Export.** The 13 refreshed files were re-exported with
  `git format-patch -N --start-number 6 rc5-base..rc5`. The filenames are
  unchanged, and the diffs only touch `@@` lines, `index` lines, the From
  sha and the Subject wrapping (127+/128-). The other 58 jm-v7-wip files and
  every base patch are unchanged.
- **Side effect.** jm-v7-wip now applies strictly on the 203-patch base
  (csf-v11 through 208). It no longer applies with zero offsets on the old
  197-patch `ab7d630`. Use the old file versions from git history for that.

### Behaviour checks (no build, no device)

- **v6/v7 are the same as v7N.** The normalized diffs `ab7d630..rc5-base`
  and `c68c685..rc5` match, with only hunk headers and index lines ignored.
  The one difference is a single context line around the resolved
  `u_debug.h` include. So rc5 = v7N + exactly the csf-v11 203-208 delta, and
  the jm-v7-wip delta is identical on both bases.
- **v11 (and v10) are the same as bc4.** `rc5-base` == the bc4 build tree.
  An independent audit covered `rc5-base..rc5` outside `jm/`. It resolved
  `#if PAN_ARCH` at 9, 10 and 11, and read the shared files and the
  compiler by hand. It found **no default change on v10/v11**:
  - **Features, limits and properties.** The `PAN_ARCH >= 9 ? x : 0` to
    `x` rewrites give the same values on v9+.
  - **v10/v11 shader code and cache key.** No change: jm-v7 091's knob hash
    is under `PAN_ARCH < 9`.
  - **Compiler.** The bifrost_compile/bi_pack/pan_nir_lower_tex changes are
    gated on `arch == 6`, `BIFROST_LIMITED_CLPER` (G71/G72/G31 only) or
    `arch < 9`.
  - **kbase_kmod.** The changes are JM-only.
  - **Remaining items reached on v10/v11, all harmless:**
    - about 130 bytes of new `panvk_device` fields;
    - an empty `gs_bo_pool`;
    - a discarded "changed sets" mask;
    - a reorder of the libpan `vp_runs.cl` include;
    - `panvk_nir_lower_gs.c` linked but not called.
  - **PANVK_DEBUG bits.** No collisions: `no_tess` = bit 28 and `ria2` =
    bit 30 fall between `BC_WIDE` (27) and `BC_TIMING` (29).
- **One v9 change outside jm/.** `PANVK_JM_MAX_VERTICES_INDIRECT` goes from
  2M to 4M (jm-v7 030). That doubles v9's ALLOC_ON_FAULT
  `indirect_varying_buffer` VA from about 144 to about 288 MB (physical
  memory only as touched). v9 also compiles all of `jm/`, which the jm-v7
  patches change. **v9 has not run any jm-v7-wip build**, so retest the G57.

### Per-arch coverage in the .so

- `src/panfrost/vulkan/meson.build` builds `panvk_v6`, `v7`, `v9`, `v10`,
  `v11`, `v12`, `v13` and `v14` unconditionally:
  - `jm_archs = [6, 7, 9]`
  - `csf_archs = [10, 11, 12, 13, 14]`
  - `valhall_archs = [9, 10, 11]`
- libpan builds `4, 5, 6, 7, 9, 10, 11, 12, 13, 14`.
- At runtime, `panvk_physical_device.c` accepts v6/v7 (warns
  "experimental", no env gate, jm-v7 002) and v9-v14.
- After the build, confirm all of `panvk_v6`…`panvk_v14` linked (section 3,
  step 4).

## 3. Build recipe (single RC5 .so)

Run this only after the bc5 agent releases `/home/flux/build-cache/build.lock`.

1. **Optional: rebase on bc5.** If bc5 lands csf-v11/209+ first, recreate
   the tree with `--csf-max <new top>`:
   ```
   git -C /home/flux/src/mesa worktree add -b rc5b /home/flux/src/mesa-rc5b 5a07217f
   sh scripts/prepare-rc5-tree.sh --mesa /home/flux/src/mesa-rc5b --csf-max 209 --base-tag rc5b-base
   ```
   On `WIP-CONFLICT` or `WIP-DRIFT`:
   - run `git -C <tree> am -3 <patch>`, resolve the conflict and
     `am --continue`;
   - finish the remaining patches;
   - run
     `git -C <tree> format-patch -N --start-number 6 -o <scratch> rc5b-base..HEAD`;
   - copy back only the changed files, then rerun the script on a fresh
     worktree. It must print `OK base=… jm-v7-wip=71` with no offsets.
   - 209 itself (`src/util/u_queue.c`) touches no file that jm-v7-wip
     touches. Its check is expected to be clean, but run it anyway.
2. **Strict apply check.** Run it on a throwaway 5a07217 worktree. It must
   print `OK base=203 jm-v7-wip=71`, with no `PATCH-DRIFT`, `WIP-DRIFT` or
   offset lines. That was done on 2026-10-22, and the result equals `rc5`
   HEAD 4554612f.
3. **Build.** NDK r30, fresh BDIR, absolute HOST_TOOLS, one build at a time
   under the lock:
   ```
   flock /home/flux/build-cache/build.lock env \
     MESA=/home/flux/src/mesa-rc5 \
     BDIR=/home/flux/build-rc5/bionic-r30-RC5-universal \
     DDIR=/home/flux/build-out/2026-10-22-rc5-universal \
     HOST_TOOLS=/home/flux/repos/panvk-kbase-android/build/host-tools/bin \
     PANVK_KBASE_RELEASE=RC5-universal PANVK_JOBS=4 \
     sh scripts/build-android.sh --profile g615-v11-csf --api 29 --ndk /opt/android-sdk/ndk/30.0.16248370
   ```
   - **Fresh BDIR.** `/home/flux/build-rc5/...` must not exist yet; if it
     does, use a new name. Never reuse a v7*/bc* BDIR.
   - **Profile.** `--profile` only picks the default DDIR (passed
     explicitly here). The tree decides the content, and
     `g615-v11-csf` (v11) is the universal profile name.
   - **Thermals.** Run `nativecode-agent telemetry` first. Use
     `PANVK_JOBS=2` if thermal is SEVERE.
   - **HOST_TOOLS.** The tools are `mesa_clc`, `vtn_bindgen2` and
     `panfrost_compile`, built on Oct 21 from `/home/flux/src/mesa`.
     Both v7N and bc4 were built with this exact HOST_TOOLS, so keeping it
     reproduces the tested libpan precompiled kernels.
   - **Host-tools follow-up (optional).** `panfrost_compile` carries its
     own Bifrost compiler, so the libpan kernels are not compiled with the
     tree's jm-v7 compiler fixes (0068 CLPER_OLD, 0075, 0076 v6 ballot).
     Same as v7N, which passed 38/38 on v6. To build tools matched to the
     tree, run
     `scripts/bootstrap-host-tools.sh --mesa /home/flux/src/mesa-rc5 --out /home/flux/repos/panvk-kbase-android/build/host-tools-rc5`
     and point HOST_TOOLS at `.../host-tools-rc5/bin`. That is a behaviour
     change to test separately.
4. **Verify the .so** at
   `/home/flux/build-out/2026-10-22-rc5-universal/libvulkan_panfrost.so`:
   - **Release string.** `strings` must show
     `PanVK-kbase RC5-universal (Mesa 26.3.0-devel (git-4554612f25))`.
   - **Per-arch libs.** `ls <BDIR>/src/panfrost/vulkan/libpanvk_v*.a` must
     show v6, v7, v9, v10, v11, v12, v13 and v14.
   - **Patch markers present:**
     - `PANVK_GPU_PRERAST_ARENA`, `without raw blocks skipped` and
       `astc_sf` (csf-v11 205/207/208);
     - `ria2` and `no_tess` (jm-v7-wip);
     - `v%u (Bifrost) support is experimental`.
   - Record the sha256 and size.
5. **APK.** Build it under the lock, alone, because concurrent gradle runs
   in `apps/panvk-test` overwrite each other (see v7N.md):
   ```
   flock /home/flux/build-cache/build.lock apps/panvk-test/gradlew -p apps/panvk-test assembleDebug \
     -PpanvkSoRc4=/home/flux/build-out/rc4-lowmem/libvulkan_panfrost-android-aarch64.so \
     -PpanvkSoRc5=/home/flux/build-out/2026-10-22-rc5-universal/libvulkan_panfrost.so \
     -PpanprobeSuffix=rc5u
   ```
   Check that the sha of `lib/arm64-v8a/libvulkan_panfrost_rc5.so` matches
   step 4 and that the package is `dev.zenithblue.panvktest.rc5u`.
6. **Device matrix**, with the same APK on every device (memory rules:
   X6 Pro over wifi adb, one Firebase device per run, rc5 only):
   - **X6 Pro, G615, v11:** must equal bc4. That is 38/38, bc_decode PASS,
     and bc_perf and MEM numbers at bc4 levels. If 209 is not in, the bc4
     vkd3d_heap shader-cache regression is expected.
   - **A06, G52, v7 (RTL):** 38/38 (v7K/v7N level).
   - **S9 / starlte, G72, v6 (Firebase):** 38/38 (v7N level). Use the v7N
     Firebase command with the rc5u APK and package.
   - **v9 G57 and v10 G610/G710:** run when a device is available. v9 is
     the only arch with a new default change (jm/ code + the 4M indirect
     vertex cap).
