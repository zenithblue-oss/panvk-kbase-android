# jm-v7 WIP (item 19, Bifrost DXVK feature port)

Not applied by scripts/apply-patches.sh. These come from worktree /var/tmp/panvk/wt-v7-port (on top of jm-v7 001-005). The agent was stopped on 2026-10-16 before it ran a full build or any device test.

- 006 Bifrost draw prerequisites (no IDVS, MDI, position buffer)
- 007 Bifrost geometry shader emulation via compute
- 008 Bifrost transform feedback from the geometry emulation

## Build status (2026-10-16, cloud session)

Builds, NOT device-tested. On Mesa 5a07217f with the full series (179 patches, `apply-patches.sh --profile g615-v11-csf`), 006-008 apply in order with `git am` (they are format-patch files and depend on each other; plain `git apply` of 007/008 alone fails). `scripts/build-android.sh` (API 29, NDK r30 30.0.16248370) builds the universal ICD with no errors and no warnings in the 13 touched files; libpanvk_v6/v7/v9 are all built. `libvulkan_panfrost.so` BuildID `b16ed9aef49d117dfba068f3438eed83fb251511` (unstripped, 25.9 MB), reproducible on a clean rebuild. NEEDED: liblog, libnativewindow, libsync, libm, libz, libdl, libc; no `call_once` import.

Review notes from the build pass:
- v6/v7 now advertise `transformFeedback`, but capture only happens in the geometry shader emulation (008). A vertex/tess shader with XFB outputs and no GS captures nothing on v6/v7. DXVK routes D3D11 stream output through a GS, so DXVK should not hit it, but CTS `transform_feedback` VS cases will fail. Either add a VS capture path or document the gap.
- `shaderClipDistance`/`shaderCullDistance` on v6/v7 use the arch-independent VS store + FS discard lowering (`panvk_lower_clip_cull_vs/fs`), so they are not GS-only.
- `transformFeedbackDraw` drops to v9+ only.

Next: RTL A06/A14 PanProbe with this build (baseline 14/38). Then add tessellation, robustness2 and BC, and move the series into patches/jm-v7 once A06/A14 gets past 14/38.
