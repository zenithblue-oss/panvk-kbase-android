# Staging 115, package P-A: CSF CPU record/submit, depth bounds FS, kbase

Source: `worklogs/g615-dxvk/perf-review-2026-10-26/a-csf-cpu.md` (A*, M*, B*) and
`c-gpu.md` (C1), with the consolidated plan `README.md` in the same directory.

- Base: tag `rc5g-114` (worktree `/home/flux/src/mesa-rc5g`, untouched).
- Work tree: `/home/flux/src/mesa-115-pa`, branch `p115-pa`.
- Export: `git -C /home/flux/src/mesa-115-pa format-patch rc5g-114..p115-pa -o patches/staging-115/pa/`.
- Every patch has an env knob; `<knob>=0` restores the 114 behaviour unless noted. The
  driver-side knobs live in `panvk_perf_knobs.h`: resolved once, at the first
  vkCreateInstance, with `os_get_option()` (so `debug.mesa.*` properties work on Android).
  When any of them is 0, the log has one line `panvk: 114 behaviour restored for: ...`.
- Private build: `/home/flux/build-out/dev-115-pa/libvulkan_panfrost.so` (profile
  g615-v11-csf, API 29). Not a release, not device-tested.

Status: work in progress (A1, C1 done; A2, A4, A5, M1, B1, B2, B4 and the
timestamp_device_coherent fix being added).

## Patches

| # | Title | Finding | Arch | Knob (default; 114 value) |
|---|---|---|---|---|
| 0001 | panvk/csf: leave the FS state clean across draws that do not run the FS | A1 | v10+ | `PANVK_CSF_FS_IDLE_CLEAN` (1; 0) |
| 0002 | panvk: v9+ unlowered fragment shader while the depth bounds test is off | C1 | v9+ | `PANVK_DB_ALT` (1; 0) |
| 0003 | panvk: PANVK_DEPTH_BOUNDS=0 hides the depthBounds feature | C1 (A/B knob) | all | `PANVK_DEPTH_BOUNDS` (1; 1) |

### 0001 A1: FS state stays clean across draws that do not run the FS
- **What:** `clear_dirty_after_draw()` re-dirtied FS after every draw whose bound FS was not
  required. Since csf-v11 092 a depth-only pipeline binds the device depth bounds FS, so
  every depth-only draw rebuilt FRAGMENT_SRT/SPD/FAU, the FS driver set and resource table,
  blend descriptors, ZSD, the three DCD words and the tiler flags. Now those draws leave the
  state clean; `fs.unused` makes the first draw that requires the FS mark FS and
  FS_PUSH_UNIFORMS dirty (a superset of what 114 had dirty at that draw). Every input of
  `fs_required()` is in `fs_user_dirty()`, so the "no FS" state emitted at the first such
  draw stays valid.
- **Gain:** review estimate 0.2-0.5 us CPU per depth-only draw (shadow maps, depth
  prepasses, stencil-only passes), about 10 CS instructions (80 B) and 1-2 descriptor
  allocations per such draw, so less to flush. Measure with a draw-overhead bench (5000
  no-FS draws into D32) or simpleperf: `build_zsd`, `build_dcd_flags`, `build_blend`,
  `set_tiler_idvs_flags` should leave the depth-only profile.
- **Risk:** low. The only new state is `fs.unused`; meta, reset and secondaries dirty FS
  themselves.
- **Regression signal:** PanProbe depth_bounds (depth-only pipeline toggled with a colour
  pipeline), depth_stencil, occlusion_query (depth-only draws with OQ), blend (colour after
  depth-only), descriptor_model; PanPlay cube (depth prepass on). A missed re-emit shows as
  a draw rendered with no FS (missing colour) right after a depth-only draw.
- **v6/v7/v9 JM:** unchanged (JM clear macro kept); the JM path rebuilds FS jobs per draw.

### 0002 C1: v9+ unlowered FS while the depth bounds test is off
- **What:** port of jm-v7 076 (FS half) to v9+: fragment shaders that get the depth bounds
  lowering while the enable may be dynamic (or without pipeline state) also get an unlowered
  compile (`panvk_shader::alt`). `cmd_select_db_alt_fs()` picks the compile at the start of
  every CSF draw and v9 JM draw layer, before `fs.required`: lowered while
  `DBT enable && depth attachment` (same predicate as `build_dcd_flags()`'s `db_on`, the v9
  JM DCD and the `ds.enable` sysval), unlowered otherwise, with FS/FS_PUSH_UNIFORMS dirty on
  a change. Nested [de]serialization un-guarded, `panvk_hash_state()` hashes whether the FS
  carries the alternate. jm-v7 098 rebind rule on every arch.
- **Gain (GPU):** every draw of a DXVK fast-linked pipeline gets FPK and the early ZS update
  back, and an FS with all colour writes masked is skipped again. Review estimate 0.5-3 ms
  per full-screen opaque pass with overdraw on affected draws (FO4-class FS); cube about 0.
- **Cost:** those FS are compiled twice (2x FS compile time for DXVK libraries with a dynamic
  DBT enable) and their binaries take twice the memory (rough: ~10 MB for FO4's ~2265
  libraries, not measured). The late clone (perf-review-2026-10-22 item 8) is phase 2.
- **Risk:** low-medium: the selection must match `db_on` exactly (it does, same three
  terms), and switching compiles must re-dirty FS state (FS + FS_PUSH_UNIFORMS, as a bind).
  Cache format changes on v9+ (UUIDs include the build-id, so no 114 blob is read).
- **Regression signal:** PanProbe depth_bounds (dynamic enable on/off, with and without a
  depth attachment), depth_stencil, occlusion_query, blend; compare with `PANVK_DB_ALT=0`.
  A wrong selection shows as depth bounds not applied (enable on) or as fragments killed
  while the test is off.
- **Integration:** one call added at the top of `v9_cmd_draw_layer()` in
  `jm/panvk_vX_cmd_draw.c` (P-D's file; no other JM change).

### 0003 C1 A/B knob: PANVK_DEPTH_BOUNDS=0
- **What:** hides `depthBounds` (all archs), so DXVK declares no depth bounds state and no FS
  is lowered: the pre-092 state. Measures the whole emulation cost against 0002, and is a
  fallback if 0002 misbehaves on a device. D3D12 depth bounds (vkd3d-proton) and DXVK's NVAPI
  extension are lost while it is 0.
- **Risk:** none at the default. **Regression signal:** depth_bounds test skips with the knob 0.
