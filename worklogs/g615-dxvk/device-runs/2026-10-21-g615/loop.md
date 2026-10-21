# G615 fix loop 2026-10-21 (RC5 BC build to 38/38)

Device: Poco X6 Pro (G615, v10 CSF), adb `10.11.176.130:33991`. APK
`/home/flux/build-out/apks/panprobe-bc.apk` (`dev.zenithblue.panvktest.bc`),
rc4 slot = RC4-lowmem `c395db09…`, rc5 slot = the build of each iteration.

Scope (coordinator, 2026-10-21): PanProbe pass count only. The BC upload
speed / memory work (problem 2) is dropped for now; the BC transcode path is
unchanged.

Start: rc4 37/1 (bc_decode), rc5 RC5-dev-bc-r30 37/1 (gs_viewport_depth).

## Iteration 1: gs_viewport_depth (csf-v11/193)

Root cause (code reading, then device check below). With a GS that writes
`gl_ViewportIndex` and multiple viewports, the per-viewport scissor is done
in the FS: the GS passthrough stores the viewport's four scissor planes in
the first free clip slot (`panvk_prerast_scissor_slot()`: CLIP_DIST0 when the
GS writes no clip distances) and the FS kills on any plane < 0.
csf-v11/193 added `panvk_fixup_gs_passthrough()`, which rewrites every
CLIP_DIST0/1 store of the passthrough and sets the lanes past the GS clip
array size to +1 (to stop stale arena data from killing pixels). With
`clip_distance_array_size == 0` that turned all four scissor planes into
+1, so GS-selected viewports were no longer scissored at all. RC4 (series
up to 186) has no 193, hence the RC4 pass.

Why only case A fails: each GS primitive is a triangle
(-1,-1),(3,-1),(-1,3) in its viewport, so without the scissor the vp0
triangle covers a 256-pixel triangle of vp1 (16x32 half, hypotenuse
x+y=2 in NDC). Depth test is ALWAYS, last write wins. In A the last
primitive is vp0 (z=1.5 -> 0.25), so it overwrites that part of vp1:
`depthR=[0.25..0.5] badDepth=256 badColor=256`. In B and C the last two
primitives are clipped and the last surviving one is vp1, which repaints
its own half, hiding the leak. Depth clamp itself was fine.

Fix: skip the store whose location is `gs->scissor_clip_loc` in
`panvk_fixup_gs_passthrough()` (one line, no offsets for later hunks).
Commit 7c3321c (patch file), tree `/home/flux/src/mesa-lowmem` (uncommitted
working-tree change on top of 3925d56 + jm-v9 + jm-v7, like the jm patches).

Build: `PANVK_KBASE_RELEASE=RC5-dev-bc-r30-vp`, fresh BDIR
`/home/flux/build-lowmem/bionic-r30-vp`, DDIR
`/home/flux/build-out/2026-10-21-bc-r30-vp`, NDK r30, 4 jobs (thermal NONE).
