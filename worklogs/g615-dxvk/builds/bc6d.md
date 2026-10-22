# RC5-dev-bc6d build (2026-10-22): fix PANVK_TESS_FUSE on quads (csf-v11 219), tess_perf level check

bc6d = bc6c (common .. csf-v11 218, jm-v9, jm-v7) plus **csf-v11 219**, and
a tess_perf fix in PanProbe. This answers the bc6c device run
(`device-runs/2026-10-22-g615-bc6c/results.md`), which found:

- `tessellation` failing in 13 of 24 cases
- `tess_perf` hash mismatches
- `PANVK_TESS_FUSE` (213) as the culprit
- L=4 slower than legacy

**Not run on a device yet.** The root cause comes from the device symptoms
plus the compiled kernels' stack sizes. The fix is checked on the host.

## 1. The 213 FUSE bug: quads only, caused by an 8 KiB stack

**Which cases fail.** Every failing `tessellation` case draws a quad-domain
patch, and is a direct draw of no more than `PANVK_TESS_FUSE_GEN_MAX` (64)
patches. Those are the draws that `panlib_tess_count_draw()` also
tessellates (WITH_COUNTS) inside the fused workgroup. These pass:

- `tri_equal_l3`, the winding cases and `isolines_4x4`: also fused-gen,
  but not quads.
- The grids, `indirect` and `indexed_indirect`: quads, but they take the
  fused COUNT + scan + draw and then tessellate in `panlib_tess_gen()`.

So the scan, the prefix sums, the draw bookkeeping and the indirect, instancing
and base-vertex handling of 213 are fine. They run on all of those passing
cases. The gen_max threshold and the 64-thread boundary are not the cause either.

**Point mode shows what breaks.** `point_mode_l4` drew 16 of 25 points. An L4
quad has 16 outer-ring points and 9 interior points. The outer ring comes
from fully unrolled loops with registers only. The interior points come from
`insideTessFactorCtx[odd]` and other dynamically indexed tessellator arrays,
which are on the **stack**. In fill mode the interior triangles then use
garbage (u,v). That is the blue where red was wanted, the partial pixels, and
`quad_frac_odd_l3.5` px=0. At L=1 the quad takes the all-ones fast path,
which uses no arrays, and its hash matched. That is why tess_perf L=1 passed
and L=4/16/mix did not.

**Stack sizes.** The `pan_shader_info.tls_size` of the libpan v11 kernels in
the bc6c build:

| kernel | stack |
|---|---|
| `panlib_tess_count_draw` quads (213) | **5648 B, rounded up to 8 KiB per thread** |
| `panlib_tess_count_draw` triangles (213) | 3616 B (4 KiB), works |
| `panlib_tess_gen` / `panlib_tessellate` quads | 2096 B (4 KiB), works |
| every other libpan kernel | at most 1360 B |

The fused kernel inlined the tessellator twice: once with mode=COUNT and once
with mode=WITH_COUNTS. Each copy has its own CHWTessellator context on the
stack. For quads, the WITH_COUNTS copy sits above 4 KiB. It is the only
kernel in the driver with a stack over 4 KiB per thread, and the only
broken one.

**Not pinned down.** I could not find the exact hardware or TLS mechanism. The
kbase TLS is device-wide (csf-v11 097) and is sized from the command
buffer's largest stack. Shader immediates are 16-bit, so they are fine.
The 8 KiB stack also doubles the device-wide TLS: about 84 MiB at 4 KiB
per thread on G615 MC6 becomes about 168 MiB, which counts against the
lowmem goals in any case.

**Fix: csf-v11 219** (`219-keep-the-fused-tess-workgroup-stack-at-4-kib-per-thread`,
mesa-bc6d `eca5ab1e`):

- `gen` is now a **variant** of `panlib_tess_count_draw` (`gen__2`), not a
  run-time argument. The COUNT-only variant inlines one tessellator.
- The driver uses the gen variant only when its `tls_size` is 4096 or less.
  It checks this through the precomp cache in `gpu_prerast_tess()`.
- **Quads:** COUNT, scan and draw run fused (2208 B), then
  `panlib_tess_gen()` runs (2096 B). That is 2 dispatches per chunk instead
  of 3.
- **Triangles and isolines:** still fully fused, 1 dispatch.

v11 libpan stacks after 219 (variant index = domain*2 + gen):

| variant | stack | used |
|---|---|---|
| tri count | 0 | when gen is not fused |
| tri count+gen | 3632 | yes (same as bc6c, passed) |
| quad count | 2208 | yes |
| quad count+gen | 5648 | **no, rejected by the 4096 guard** |
| iso count, iso count+gen | 0, 0 | yes |

The largest stack any tess path can run is 3632 B, so 4 KiB per thread.

**Tried first:** one tessellator call site with a run-time mode, a pass loop
bounded by `gen`. It still needed 5296 B for quads, so it was dropped.

**Not done:** amending 213. 214 and 216 edit `tess_fast.cl` after it, so a
separate fix-up patch is safer.

## 2. Legacy L=4 hash == L=1: a test artifact, not a driver bug

The tess_perf TES colours with (u, v, primID). On the axis-aligned flat
quads of tess_perf:

- position is affine in (u, v), so (u, v) is affine in screen position
- the rasterizer's linear interpolation gives the same colour at every
  pixel whatever the tessellation

So L=1, L=4 and mix give the same image when the tessellator is correct.
Legacy L=16 differs only by rounding. The level does reach the shaders: it
comes from a per-draw **push constant** (`pc.p.x`) that the TCS reads. The
new path's L=4 and L=16 hashes differed only because 213 broke its interior
points.

The fix is in PanProbe (`a7a1558`):

- The TES colour is now `(u*u, v*v, 0.5*primID/15 + inner/64)`. It is not
  affine and it encodes the level.
- tess_perf FAILs with `LEVEL_IGNORED` when L=1, 4 and 16 give equal hashes
  within a variant.

Every tess_perf hash changes with this, so do not compare them to bc6c.

## 3. Why L=4 was slower, and the expected tess_perf numbers

bc6c's fused L=4 (1106 us, against 520 with FUSE=0) was not a cost of fusing.
At L=4, 16 and mix, the broken interior points produce garbage triangles.
They spread over the render target, and the extra time is fragment and
overdraw work. At L=1 no garbage was made, and fusing saved 152 us per draw
(488 -> 336, two dispatches and drains fewer). Generating inside the
64-thread group is not serial per patch either: patch i is thread i, the
same as `panlib_tessellate`.

tess_perf draws only quads, so with 219 every tess_perf draw takes the
2-dispatch path, one dispatch fewer than FUSE=0. The estimate:

- start from bc6c's PANVK_TESS_FUSE=0 numbers (214-216 on)
- subtract one dispatch, about half of the 152 us measured for two, so
  about 75 us

| variant | L=1 | L=4 | L=16 | mix |
|---|---|---|---|---|
| bc6c legacy, gs=0 (measured) | 581 | 579 | 3127 | 2806 |
| bc6c FUSE=0, gs=0 (measured) | 488 | 520 | 2512 | 2483 |
| **bc6d default, gs=0 (expected)** | **~410** | **~445** | **~2440** | **~2410** |
| bc6c legacy, gs=1 (measured) | 725 | 972 | 4950 | 3941 |
| bc6c FUSE=0, gs=1 (measured) | 683 | 920 | 4653 | 3654 |
| **bc6d default, gs=1 (expected)** | **~610** | **~845** | **~4580** | **~3580** |

Against legacy, that is about -30% at L=1, **-23% at L=4**, -22% at L=16 and
-14% at mix (gs=0). L=1 will be about 75 us slower than bc6c's 336, because
quads lost the in-kernel gen. Triangle-domain draws keep the full fusion.
The default and legacy hashes must match, and L=1/4/16 must differ.

**Still possible:**

- Get the quad count+gen variant under 4 KiB, so quads also take one
  dispatch. That needs the two tessellator contexts to share stack slots,
  for example by restructuring `poly_tess_quad_process` so that COUNT does
  not inline the array setup.
- Generate in parallel per ring or row (review #6).

## Toggles

- `PANVK_TESS_FUSE=1` (default): fused count/scan/draw, plus gen for
  triangles and isolines.
- `PANVK_TESS_FUSE_GEN_MAX=0` turns the fused gen off for every domain.
- `PANVK_TESS_FUSE=0` gives the three bc6b passes.
- `PANVK_TESS_SETUP_STEP`, `PANVK_TESS_ARENAS` and
  `PANVK_TESS_WINDING_INLINE` (214-216) are unchanged and on. They passed
  on bc6c.

## Strict apply

- **Full series:** a fresh worktree `/home/flux/src/mesa-bc6d-strict` at
  5a07217f, then `scripts/apply-patches.sh --profile g615-v11-csf`. It
  printed `OK applied=214`, with no offset or fuzz.
- **jm-v7-wip:** `git am` of jm-v7-wip 0006-0077 (72 patches) on top applies
  cleanly. The top patch is 0077, jm-v7 104.
- **Dev branch:** `bc6d` in `/home/flux/src/mesa-bc` (worktree
  `/home/flux/src/mesa-bc6d`). Its base is c57d7d26 (common .. csf-v11 218)
  and 219 is eca5ab1e.

## Build

- Thermal was NONE, so 4 jobs. The command:

  ```
  PANVK_KBASE_RELEASE=RC5-dev-bc6d PANVK_JOBS=4 MESA=/home/flux/src/mesa-bc6d-strict
  BDIR=/home/flux/build-bc6d/bionic-r30-RC5-dev-bc6d DDIR=/home/flux/build-out/2026-10-22-bc6d
  HOST_TOOLS=/home/flux/build-cache/host-tools/5a07217f... flock /home/flux/build-cache/build.lock
  sh scripts/build-android.sh --profile g615-v11-csf --api 29 --ndk /opt/android-sdk/ndk/30.0.16248370
  ```

  It printed `OK profile=g615-v11-csf`.
- **Driver:** `/home/flux/build-out/2026-10-22-bc6d/libvulkan_panfrost.so`,
  29441528 bytes, sha256
  `bd6ae709095adc8c6c1bd3997aa85b079f89c8771faa8fd850d6ba6460f35a11`.
  - Its version string is `PanVK-kbase RC5-dev-bc6d (Mesa 26.3.0-devel (git-5a07217f03))`.
  - It contains the PANVK_TESS_FUSE and PANVK_TESS_FUSE_GEN_MAX strings.
- **APK:** built under the build lock with:

  ```
  ./gradlew assembleDebug -PpanvkSoRc4=/home/flux/build-out/rc4-lowmem/libvulkan_panfrost-android-aarch64.so
  -PpanvkSoRc5=<so above> -PpanprobeSuffix=bc6d
  ```

  - BUILD SUCCESSFUL, with no stale `.cxx` this time.
  - Output: `/home/flux/build-out/apks/panprobe-bc6d.apk`, 44602894 bytes,
    sha256 `f7b8049745ad2e0e9ac279f66a55757ba869a5813ad7e84226f8635a3c42e54d`.
  - aapt2: `dev.zenithblue.panvktest.bc6d`, versionCode 15, versionName 1.2.7.
  - The .so files inside both match: `libvulkan_panfrost_rc5.so` is
    bd6ae709... (bc6d) and `libvulkan_panfrost_rc4.so` is c395db09...
    (RC4-lowmem).
  - `libt_tess_perf.so` contains the new LEVEL_IGNORED check (a7a1558).

## Device plan (G615)

1. Run `--es driver rc5 --es autorun all`. The target is 39/39, with
   `tessellation` PASS and `tess_perf` PASS (no MISMATCH, no LEVEL_IGNORED).
2. Read the tess_perf us_draw numbers against the table above.
3. If `tessellation` still fails:
   - with `PANVK_TESS_FUSE_GEN_MAX=0`: if it passes, the triangle fused gen
     at 3632 B is the problem
   - with `PANVK_TESS_FUSE=0`: if it passes, the scan or count is the
     problem
