# jm-v7 WIP (item 19, Bifrost DXVK feature port)

Not applied by scripts/apply-patches.sh. These come from worktree /var/tmp/panvk/wt-v7-port (on top of jm-v7 001-005). The agent was stopped on 2026-10-16 before it ran a full build or any device test.

- 006 Bifrost draw prerequisites (no IDVS, MDI, position buffer)
- 007 Bifrost geometry shader emulation via compute
- 008 Bifrost transform feedback from the geometry emulation

Next: build them (scripts/build-android.sh, universal profile) and fix any errors. Then add tessellation, robustness2 and BC, and move the series into patches/jm-v7 once it builds and A06/A14 PanProbe gets past 14/38.
