# RC4 missing-patch recovery (2026-10-21)

Goal: find the code that RC4 (`977c0c4c…`, BuildID `6654fada`, NDK r30) reportedly has and main lacks, and recreate it as
`patches/jm-v7/006+`. RC4 passes PanProbe 38/38 on the Galaxy A06 (G52 MC2 r1, v7, kbase JM). RC5-dev-v7 (`a1e68b59…`,
NDK r29) gets about 14/38. Method: treat the RC4 binary as ground truth and diff it, function by function, against rebuilds
of main.

## Result

**No patch can be recovered. With the same toolchain and build layout, main's 191-patch series rebuilds RC4. The v7 path
matches instruction for instruction.**

The earlier comparison (rc4-binary-analysis.md, Result 4) diffed against an NDK **r29** build. That left about 1,600
functions differing from compiler noise alone, which hides any real change. I rebuilt main with the **same toolchain as
RC4**, the HomuHomu833 `android-ndk-custom` r30 (`/opt/android-sdk/ndk/30.0.16248370`, build 16248370, which is the
number in RC4's `.note.android.ident`). I used the **same build layout**: a Mesa tree at `…/work/mesa` with HEAD
`5a07217` and the patches uncommitted, so `__FILE__` is `../../work/mesa/…` and driverInfo is `git-5a07217f03`. I also set
`PANVK_KBASE_RELEASE=RC4`. With noise this low, a real source change would show up at once.

| Build | `.text` | `.rodata` | `.data.rel.ro` | Functions not instruction-identical to RC4 |
|---|---|---|---|---|
| RC4 release | 0x86f5e0 | 0xc98590 | 0x70fb0 | – |
| main, NDK r29 (RC5-dev-v7 `a1e68b59`) | 0x882b44 | 0xc9b254 | 0x73d80 | ~1,580 of 10,439 (codegen noise) |
| main, r30, `src/mesa-recover` path (`RC5-dev-v7-r30`) | 0x86f5c8 | 0xc98790 | 0x70fb0 | 50 of 10,442, all path, driverInfo, page-offset or veneer noise except the item below |
| **main, r30, RC4 layout (`RC4repro`, unstripped sha256 `158bf8c9…`)** | 0x86f600 | **0xc98590** | **0x70fb0** | **27 of 10,442**, see below |

For RC4repro against RC4:
- **`.rodata` is byte-identical** except 13 bytes in one `.text`-relative offset table, which moves because `.text` is 32
  bytes longer.
- This includes the per-model table: `pan_model_list` has 36 entries, with the same G52 / G52 r1 rows (`0x7212` /
  `0x7402`), tile buffer sizes, rates and quirks.
- It also includes every format, opcode and disassembler table, and the precompiled libpan shader blobs for every arch.
- **`.data.rel.ro` and `.data`:** every word is equal or is a pointer. Every `.text` pointer maps to the matching function
  and every string pointer points to the same string. So the vtables, entrypoint tables and dispatch tables are identical.
- **Functions:** 10,415 of 10,442 are instruction-identical once registers and page offsets are normalised. Every function
  below 0xe064e0 sits at the same address as in RC4. That range covers all of v6, v7 and v9 panvk, the vk runtime and the
  start of v10.

### The 27 remaining functions

| Function(s) (RC4 VA) | Difference | Real? |
|---|---|---|
| `panvk_compile_shader` v10 0xe064e0, v11 0xee6074, v12 0xfaccbc, v13 0x1073be8, v14 0x113a824 | Register allocation and block placement only. The calls are the same (509; 505 on v14), the immediates are the same (RC4repro rematerialises `mov #0x86` once), and the struct offsets, strings and branch conditions are the same. RC4 tail-duplicates one NULL-entrypoint block in the inlined `panvk_gpu_prerast_emit_generated_records()` (the `nir_shader_get_entrypoint` → `nir_load_global_invocation_id` → `load_global` sequence). That accounts for the +2 branches and -16 bytes. | Not a semantic change, and **CSF (v10+) only**. It cannot touch the v7 path: the v6, v7 and v9 `panvk_compile_shader` are identical. I can't tell whether it is an equivalent source edit or a toolchain-build difference (see the next row). There is nothing to transcribe. |
| `__init_cpu_features`, `__aarch64_swp{1,4}_acq_rel`, `unw_get_proc_info` | RC4 has a trailing `udf #0` padding word, and the Cortex-A53 843419 veneers sit after `unw_get_proc_info` | Runtime library objects. RC4repro's `.comment` has one extra line, `Android (dev, r563880c) clang 21.0.0`, so one static runtime archive in our r30 install differs from the one RC4 linked. This is not driver code. |
| v9 `cmd_draw_layer`/`emit_malloc_vertex_job`, `kbase_kmod_{page_size,dev_create,bo_alloc}`, `u_trace_state_init_once`, x11/libdrm helpers, `nir_swizzle`, `pandecode_cs_trace_v14`, `panvk_v12_device_draw_context_init`, `panvk_v13_CmdExecuteCommands`, `panvk_v14_get_physical_device_extensions` | The `:lo12:` offset of a `.data`/`.bss` global (which moves by 0x20), or an erratum veneer branch | Layout noise |

**v7 path, function by function:** `panvk_v7_*`, the v7 `cmd_*`/`prepare_*`/descriptor/meta/blend/shader functions, the
shared Bifrost compiler (`bi_*`, `va_*`, `bifrost_*`, `bi_pack_*`), the NIR passes, `pan_*_v7` (libpan/genxml), kbase
kmod and JM submission (`kbase_kmod_*`, `panvk_jm_kbase_*`, the jm-v7/003-005 code), the vk runtime and WSI are all
instruction-identical to RC4. **RC4's v7 driver is exactly main's jm-v7 001-005.**

## Other sources searched (subagent, read-only), all empty

- Every local and remote ref, the tags and the stash: all are ancestors of origin/main. Patch files deleted after the rc4
  tag: none.
- The broken clone `…git-broken-2026-10-21`: it was cloned 2026-10-20 at bc29d09. It has 5 unreachable commits, all
  autostashes or worklogs from 10-21.
- Every worktree of `/home/flux/src/mesa`, `/home/flux/repos/*` and the build dirs. No `work/mesa` tree from 10-17 exists
  anywhere.
- GitHub: branches, PRs (#1 only), issues #2-#24, releases (the rc4 and rc4-lowmem assets are `.so` files, ICD json,
  launcher zips and SHA256SUMS only, with no source or patches), CI artifacts, gists (none), and forks (none).
- There are no commits on any ref between 2026-10-16 13:22 and 2026-10-20. The RC4 notes themselves say the exact patch
  set was not saved. The binary now shows it was main's series as of 10-16 (csf-v11 018-196, jm-v9 001-031, jm-v7
  001-005).

## What this means for the A06 38/38 vs 14/38

- Driver source cannot explain the gap; it is ruled out.
- There are two remaining causes, and v7R decides between them:
  1. **The toolchain.** RC5-dev-v7 was built with **NDK r29**, which produces about 1,600 functions of different code,
     including the Bifrost compiler (`bi_emit_load_ubo`, `bi_pack_*`, `va_pack_instr` …) and v7 `cmd_emit_dcd`/`prepare_draw`.
     A miscompile, or UB that r29 exposes, would be specific to that build.
  2. **The run itself** (app slot, device or firmware unit, misread). See `rc5-vs-rc4-a06.md`.
- **RC5-dev-v7R** is this same series built the RC4 way (r30, RC4 layout). Its code is RC4's code, with only the
  driverInfo string changed. Run RC4 and v7R on the A06 in the same session:
  - If v7R scores 38 and RC5-dev-v7 scores 14 → the r29 toolchain is the cause. Build releases with r30 only, and bisect
    the r29 code differences if it matters.
  - If v7R scores 14 → RC4 does too when run the same way. The 38/38 came from another slot, device or run.
- Either way, nothing in RC4 is missing from main.

## Other findings

- `scripts/build-android.sh`: changing `PANVK_KBASE_RELEASE` on an existing `BDIR` has no effect. The re-generated cross
  file is not re-read on `meson setup --reconfigure`, so the old driverInfo is kept (seen here: `RC4` stayed after a
  rebuild with `RC5-dev-v7R`). Use a fresh `BDIR` for every release string.
- The tools are in `tools/binary-diff/`:
  - `parse.py`: objdump → per-function pickles with string xrefs.
  - `match.py`: anchors on unique string sets and exact token hashes, propagates through call graphs, LIS-aligns, and
    fills gaps with Needleman-Wunsch.
  - `cmp.py`: per-pair constants, callees, strings and branch counts.
  - `tdiff.py`/`fdiff.py`: register-normalised and raw diffs of one pair.
  - `mset.py`: instruction multiset.
  - `rodiff.py`/`rosync.py`/`rohex.py`: `.rodata` objects and resync diff.
  - `models.py`: dumps `pan_model_list`.
  - `nmfuncs.py`: function bounds from symbols.

  RC4 function bounds come from `.eh_frame_hdr` (`/home/flux/rc4-analysis/work/rc4.funcs.json`).
