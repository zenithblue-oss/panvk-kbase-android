# RC4 and RC4 lowmem binary analysis (2026-10-21)

Goal: find out from the recovered binaries what source RC4 and RC4 lowmem were built from, compared with the patch series on `main`, so the lost patches can be recreated.

## Inputs

| | RC4 | RC4 lowmem |
|---|---|---|
| Tag | `panvk-kbase-v0.1.0-rc4` | `panvk-kbase-v0.1.0-rc4-lowmem` |
| Asset SHA-256 | `977c0c4c…befd791`, matches the release notes | `c395db09…4930c8a5`, matches the notes (binary-patched build; the original `595cf591…` is not in the release) |
| BuildID | `6654fada10d55c914e99d3f28c60f22687158656` | `aff7a07faea2695cbf21f44ea80d0d603e34360b` |
| driverInfo | `PanVK-kbase RC4 (Mesa 26.3.0-devel (git-5a07217f03))` | `PanVK-kbase RC4 (Mesa 26.3.0-devel (git-5437602824))` |
| `.note.android.ident` | NDK `r30`, build `16248370`, API 29 (`0x1d`) | same |
| `.comment` | Android clang 22.0.1 (r584948b) for the driver, LLD 21.0.0, plus two clang 21.0.0 lines (the NDK r29 prebuilts and `HomuHomu833/llvm-custom`) that come from linked static archives | same |
| Source path prefix | `../../work/mesa/src/panfrost/...` (built in `work/mesa`) | same |
| `.rodata` / `.text` | 0xc98590 / 0x86f5e0 | 0xdd6fc0 (+1.30 MB) / 0x8775d0 (+32 KB) |

The files are in `/home/flux/rc4-analysis/{rc4,rc4-lowmem}/`. The intermediate files (string dumps, per-patch literal hits, attribution) are in `/home/flux/rc4-analysis/work/`. `git-5437602824` is not an object in the `/home/flux/src/mesa` clone. It is a local commit of the lowmem work tree and was never pushed.

## Method

1. `strings -n 6` of both `.so` files (22,880 strings each).
2. For each patch in `common`, `android`, `kbase-common` (including `files/`), `app-loader`, `wsi`, `csf`, `csf-v11`, `jm-v9`, `jm-v7`, `jm-v7-wip` and `bcn-layer`, I took the string literals on the added lines. I split them at printf conversions, kept the fragments that are not in upstream Mesa `5a07217` (`src/panfrost`, `src/vulkan`, `src/util`, `src/compiler`), and looked for each one in the RC4 bytes.
3. The other direction: I attributed every RC4 string that mentions `panvk`/`kbase`/`PANVK`/`gpu_prerast`/`Bifrost`. Literal concatenation and `PRIx64` were normalised. Each string was matched to a patch on main, to upstream, or marked unknown.
4. Strings that the patches remove: a removed literal that is still in RC4 (and not re-added by another patch) would mean the removing patch was not applied.
5. RC4 against lowmem: diff of the string dumps, a 64-byte block diff of `.rodata`, ADRP/ADD cross-references to the new strings, and disassembly (NDK r29 `llvm-objdump`) of the functions that use them.

The scripts are kept out of the repo, in the session scratchpad: `scan.py`, `check2.py`, `removed_lits.py`, `xref.py`, `ldrscan.py`, `immscan.py`, `rodiff.py`.

## Result 1: RC4 matches main's full series, plus nothing else that a string can show

- **No lost-patch strings.** Every `panvk`/`kbase`/`PANVK_*` string in RC4 (186 patch strings, 12 upstream) comes from a patch now on main or from upstream `5a07217`. The only "unknown" lines are `__FILE__` paths (`../../work/mesa/...`), genxml field names and the driverInfo override. No env var, debug option or log message in RC4 is missing from main.
- **No removed string is still there.** The only removed literal that is still in RC4 (`Failed to choose spill node`, csf-v11/153) also exists in upstream Midgard RA, so it proves nothing.
- **RC4 has patches newer than the PROGRESS note says.** PROGRESS.md said RC4 was probably "csf-v11 up to 186, jm-v9 up to 030". The binary has strings that only these later patches add:
  - csf-v11/187 (`G725/G925`)
  - csf-v11/192 (`PANVK_KBASE_ASYNC_HEAP_TERM`)
  - csf-v11/193 (`cull_flag0/1`)
  - csf-v11/196 (`PANVK_FAN_CHUNKS`)

  So RC4 was built from (close to) the full current csf-v11 series, 018-196. jm-v9 001-030 is also confirmed. jm-v9/031 and android/016 add no strings, so the binary can't show whether they are in.
- **Patch count.** The strings fit `scripts/apply-patches.sh --profile g615-v11-csf`: common 1 + android 5 + kbase-common 10 + app-loader 1 + wsi 2 + csf 2 + csf-v11 134 + jm-v9 31 + jm-v7 5 = **191 patches**. The user remembers about 195-200. That would only fit if a few string-less patches existed that are not on main, or if the count included the 8 jm-v7-wip files, which the binary shows were not applied (see Result 2).

### Patches on main with strings that are absent from RC4, and why

| Patch | Missing literal | Explanation |
|---|---|---|
| csf/007 (8 strings) | `kbase: bound/emitted/completed subqueue …`, `tiler heap desc …` | `mesa_logd`, compiled out in release |
| jm-v9/004 | `KBASE_IOCTL_MEM_EXEC_INIT: %s (zone exists or unsupported…)` | `mesa_logd`; the `mesa_logw` sibling *is* present |
| csf-v11/117 | `wsi/x11: shm memfd seal failed` | `mesa_logd` |
| jm-v9/001 | `kbase: host memory import failed`, `… GPU mmap failed` | `kbase_kmod_bo_import_host()` has no caller, so LTO removed it |
| jm-v9/014, 020 | `v9 tessellated draw needs …`, `… drawn in at most …` | replaced by jm-v9/028, whose wording RC4 has |
| csf-v11/095 | `secondary command buffer with several …` | removed again by csf-v11/114 |
| kbase-common/004 | `/dev/sw_sync`, `pthread_create failed` | removed by csf-v11/075 |
| android/013 | `/vendor/lib64/hw/mapper.mediatek.so` | generalised by android/014 (`%s/mapper.%s.so`, present) |
| csf-v11/099 | `RC3-lowmem` | driverInfo overridden at build time (`RC4`) |
| 025/042/046/065/066/076/082/085/107/120, jm-v9/011/014 | `static_assert`/`UNREACHABLE` text, `cntfrq_el0` asm, the 083 asm thunk text | not emitted as data |
| bcn-layer/001 | `VK_LAYER_BCN_BCnLayer` | a separate layer, not part of the driver `.so` |

All of these are explained. None of them shows that a main patch was left out of RC4.

### Patches confirmed in RC4 by unique strings

- android 013, 014
- kbase-common 003, 004, 005, 006, files/
- csf 007
- csf-v11 018, 022, 025, 031, 033, 039, 041, 042, 046, 051, 059, 061, 062, 065, 066, 075, 076, 083, 085, 088, 089, 090, 092, 099, 103, 104, 110, 117, 118, 119, 120, 131, 150, 151, 180, 185, 186, 187, 192, 193, 196
- jm-v9 001, 002, 004, 011, 014/020 via 028, 015, 022, 024, 025, 028
- jm-v7 002, 003, 004, 005

jm-v7/001 (the GPU variant fix) has no string, but it was verified on the G52 r1 `0x74021000`. Every other patch adds no distinctive literal, so the strings can't show whether it is in RC4.

## Result 2: v7 (Bifrost) in RC4 is jm-v7 001-005 only

RC4 has all the jm-v7 001-005 strings:
- `panvk: v%u (Bifrost) support is experimental`
- the three `Bifrost JM atom stride …` lines
- `KBASE_IOCTL_MEM_JIT_INIT skipped; tiler heap is a GROW_ON_GPF region`
- `v%u growable BO, GROW_ON_GPF, …`
- `/dev/ion unavailable`, `ION_IOC_ALLOC failed`, `using ION system heap, mask`, `no usable ION system heap`

Not one jm-v7-wip string is present, including those that would survive into a release build:

| WIP patch | Evidence that would be in the binary | In RC4? |
|---|---|---|
| 0006 draw prerequisites (no IDVS, MDI, position buffer) | no strings | can't tell |
| 0007 GS emulation | NIR variable names `gs_strip_i`, `gs_fill_i`, `gs_prims_written` | **absent** |
| 0008 XFB from GS | no strings | can't tell (needs 0007) |
| 0009 XFB passthrough | `panvk xfb passthrough`, `in_pos`/`out_pos` | **absent** |
| 0010 robustness2 / nullDescriptor | only `static_assert` text | can't tell |
| 0011, 0012 | no strings | can't tell (they fix 0007-0009) |
| 0013 tessellation | `PANVK_DEBUG` option `no_tess`, four `mesa_logw_once` messages | **absent** |

This matches the release notes ("GS, transform feedback, tessellation, robustness2 and multi-draw indirect are not in RC4 on v7"). It also matches the device result: 14/38 on A06/A14 is the same as the jm-v7-wip README's "baseline before 006". **The binary has no v7 G52 code beyond jm-v7 001-005 that a string can show.** Any lost v7 fix would have to be log-free, for example a register value, a descriptor field or a gate on the PAN_ARCH. Such a fix can only be found by diffing machine code against a rebuild (see the plan).

## Result 3: what the RC4 lowmem patch does

### Differences between the strings

The lowmem binary has only these new strings:
- `PANVK_BC_MODE`
- `lowmem`
- `, lowmem`
- `PANVK_BC_LM_OPTS`
- the log format changed from `panvk: BC emulation %s (native compressed mask %#x)` to `panvk: BC emulation %s%s (native compressed mask %#x)`
- the driverInfo `git-5437602824`

Nothing else changed in the strings. The base is the RC4 series, and the lowmem work is one patch, or a few, on top of it.

`.rodata` grows by 1.30 MB. The block diff finds about 1.48 MB of new or changed data, almost all of it in 5 repeated regions of about 250 KB. These are the per-arch precompiled libpan shader blobs for v10-v14, which hold new or changed BC kernels. `.text` grows by 32 KB.

### Functions (addresses are VAs in the lowmem `.so`; `.text` VA = file offset + 0x4000)

**`panvk_bc_lowmem_enabled(pdev)` @ 0xe95ba0** (common code, next to `panvk_bc_emul_enabled` @ 0xe95a58). Logic:
1. If the GPU product ID (`gpu_id >> 16`) is one of 0x600, 0x620, 0x720, 0x750, 0x820, 0x830, 0x860 or 0x880, return false. These are the same exclusions as the function before it.
2. Require arch >= 10 (`gpu_id >> 29 >= 5`, so this is CSF only). This matches the `bc_gfx` gate of csf-v11/150.
3. Require `panvk_bc_emul_enabled(pdev)`.
4. If either of two `PANVK_DEBUG` bits is set (halfword +2 of the debug flags, mask 0x480), return false.
5. Read `getenv("PANVK_BC_MODE")`. In the original build, NULL returns false. In the released binary-patched build, NULL goes to step 6 (`cbz x0` at 0xe95c68). Any value that `strcmp` does not match to `"lowmem"` returns false.
6. Return true only if all 10 ETC2/EAC formats are natively supported. It checks the native-format bitmask of `pdev->formats` (`+0x2ec … +0x310`).

**Device creation, per arch (v10-v14 copies, e.g. @ 0xf3ece4):** if lowmem is enabled:
- Allocate a device-private BO of `0x121000 + 0x10000` bytes (1.13 MB + 64 KB) and store it at `dev+0x1a90`.
- Fill the last 64 KB (offset 0x121000) with zeros and flush it. This is a "zero BC blocks" source.
- Read `PANVK_BC_LM_OPTS` with `strtoul(s, NULL, 0)`, default `0x30000`, and store it at `dev+0x1ae8`. The first 0x121000 bytes are not written by the CPU near this code, so they are GPU scratch or tables that a kernel fills.

**Image creation, BC setup in `panvk_image.c` @ 0xe8c09c:** after the RC4 BC shadow format is chosen (`image+0xd9c`):
- `image->bc_lowmem` (`+0x11e8`) is set to 0. If lowmem is enabled and all of the following hold:
  - `tiling == OPTIMAL`;
  - none of SPARSE_BINDING, SPARSE_RESIDENCY, SPARSE_ALIASED, BLOCK_TEXEL_VIEW_COMPATIBLE, DISJOINT or ALIAS (mask 0x687) is set;
  - there is no `VkExternalMemoryImageCreateInfo` in pNext;

  then the BC format is mapped through a 12-entry table at rodata 0x5b538:

  | BC VkFormat | Lowmem shadow VkFormat |
  |---|---|
  | BC1_RGB_UNORM/SRGB | ETC2_R8G8B8_UNORM/SRGB (147/148) |
  | BC1_RGBA_UNORM/SRGB | ETC2_R8G8B8A1_UNORM/SRGB (149/150) |
  | BC2_UNORM/SRGB, BC3_UNORM/SRGB | ETC2_R8G8B8A8_UNORM/SRGB (151/152) |
  | BC4_UNORM/SNORM | EAC_R11_UNORM/SNORM (153/154) |
  | BC5_UNORM/SNORM | EAC_R11G11_UNORM/SNORM (155/156) |

- If the table has an entry: `bc_lowmem = 1`, the shadow format becomes the ETC2/EAC format, and the byte at `image+0xd9a` is cleared (probably the `bc_gfx`/AFRC fragment-shadow flag, so the ETC2 shadow does not use the 150/151 fragment path). Otherwise (BC6H, BC7, or a rejected image): `bc_lowmem = 2`.
- When `bc_lowmem` is non-zero, the image size computation (@ 0xe8c1d8) takes a separate branch. Inferred from that branch and the measured sizes (not fully traced): it no longer allocates the raw block plane plus the shadow. This is where the memory drop comes from:
  - BC1 8.2 → 2.7 MB and BC3 16.4 → 5.5 MB: only the ETC2 data, the same size as the BC data.
  - BC7 10.9 → 5.5 MB and BC6H 28.1 → 22.6 MB: the raw plane (5.5 MB at 2048² with mips) is gone, and the RC4 shadow is kept. BC6H and BC7 still decode bit-identically.

**Upload and decode, per arch (e.g. @ 0xf35300 to 0xf35a70):**
- BC VkFormat 131-146 maps to a decoder class 0-10.
- The decode dispatch (`0xfe68f0`) is called with libpan kernel IDs 0x49-0x4d. Its push constants now include the lowmem BO VA (`dev->bc_lm_bo->va`) and `PANVK_BC_LM_OPTS`.
- For `bc_lowmem` images, the raw-plane destination of the upload copy is replaced by `lm_bo + 0x121000`. The raw blocks are therefore never kept. This is why `bc_decode` raw readback and `vkCmdCopyImage` give wrong results for all 16 formats: the copy reads blocks that don't exist.

**Zero-initialised decode (csf-v11/040 path, @ 0xf34d98):** for `bc_lowmem == 1` images, the decoder's source is the zeroed 64 KB at `lm_bo + 0x121000` instead of the raw plane.

**Log:** `panvk: BC emulation %s%s (native compressed mask %#x)`, with `"on"`/`"off"` and `", lowmem"`/`""` (@ 0xe9541c in physical-device init).

### Behaviour to keep, and the known bug to fix

- BC1-BC5 are transcoded to ETC2/EAC on the GPU (lossy, 39-50 dB against RC4's decode). BC6H and BC7 keep RC4's exact shadow and drop only the raw plane.
- Raw blocks are not kept, so these break:
  - `vkCmdCopyImage` from or to a BC image;
  - image-to-buffer copies from a BC image;
  - host image copy.

  A recreated patch should do one of these:
  - (a) keep the raw plane when the image has `TRANSFER_SRC` usage, or when it is a copy destination whose contents are later read back raw. This costs memory only for those images;
  - (b) on a BC→BC copy, transcode or decode from the source's shadow into the destination's shadow;
  - (c) do not enable lowmem for images with `TRANSFER_SRC` usage. This is the simplest and keeps CTS behaviour.
- Peak RAM is about 100 MB higher in `bc_perf`. Look for host staging buffers in the transcode path.

## Plan to recreate the patches (in priority order)

1. **Pin down RC4 by rebuilding it (no new code).** Build Mesa `5a07217` with the current main series through `apply-patches.sh --profile g615-v11-csf` (csf-v11 018-196, jm-v9 001-031, jm-v7 001-005), using NDK r30, API 29, and the driverInfo override `RC4`. Then compare:
   - `strings -n 6` against `/home/flux/rc4-analysis/work/rc4.str`. Only build paths should differ.
   - the sizes of `.rodata`, `.text` and `.data.rel.ro` (RC4: 0xc98590 / 0x86f5e0 / 0x70fb0);
   - the function-level diff of the v7 per-arch code (`objdump` of the `panvk_v7_*` text range, matched by order).

   If the sizes and strings match, RC4 is just main, and the "lost v7 fixes" don't exist beyond 001-005. If v7 `.text` differs, diff that range to find any log-free v7 change. The build agent is already building in `/home/flux/src/mesa`, so its output can be compared straight away.
2. **v7 G52.** The binary has nothing to recover beyond jm-v7 001-005. The next v7 work is jm-v7-wip 0006-0013 (unbuilt on device). Order: 0006 (MDI, no IDVS), 0010 (robustness2/nullDescriptor), then 0007+0011+0012 (GS), 0008/0009 (XFB), 0013 (tess). Test on A06/A14 against the 14/38 baseline after each group.
3. **Recreate the lowmem BC patch on main** as `csf-v11/197-bc-lowmem-etc2-transcode.patch`, following the spec above:
   - `panvk_bc_lowmem_enabled()`, with `PANVK_BC_MODE=lowmem|exact`. Make lowmem the default as in the released binary, or keep it opt-in. This is a product decision.
   - `PANVK_BC_LM_OPTS` (default 0x30000; its bit meanings are unknown, so define them anew).
   - A device lowmem BO with a 64 KB zero tail.
   - The BC→ETC2/EAC format table, `image->bc_lowmem` with values 0/1/2, and image sizing without the raw plane.
   - libpan kernels for BC1-5 → ETC2/EAC transcoding. Write them new; the precompiled blobs can't be decompiled practically.
   - Upload and zero-init decode paths that use the lowmem BO.
   - A fix for the `vkCmdCopyImage`/readback bug, option (c) above or better.
   - The log line `panvk: BC emulation %s%s …`.

   Validate with PanProbe `bc_decode` (target 16/16/16), `bc_perf` (memory near the lowmem numbers), and CTS `texture.compressed.*bc*` and `api.copy_and_blit.*bc*`.
4. **Process.** Record the BuildID and the exact `apply-patches.sh` output for every release build in `validation/`, so a release can never again be built from an unrecorded tree.
