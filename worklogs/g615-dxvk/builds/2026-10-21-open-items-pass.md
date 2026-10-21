# 2026-10-21 non-device open-items pass

## Strict apply of the default series
- Fresh worktree `/home/flux/src/mesa-strict2` at Mesa `5a07217`, `scripts/apply-patches.sh --profile g615-v11-csf`: `OK applied=197`, rc 0, no offset, no fuzz (PATCH-DRIFT check active). Includes csf-v11 up to 202, jm-v9 up to 031 and jm-v7 001-005.

## jm-v7-wip on the current series
- Committing the 197-patch result as one base commit and running `git apply --check -v` plus `git am` per patch over 0006-0019 found drift against the current series:
  - 0010 applied with a 19-line offset.
  - 0013 (tessellation) failed: csf-v11/200 added `PANVK_DEBUG_BC_TIMING` (`1 << 29`, `bc_timing`) in the same hunks in `panvk_instance.[ch]` where 0013 adds `PANVK_DEBUG_NO_TESS`.
- Fix: `git am -3` for 0013 (trivial both-sides conflict in the debug flag table and enum, kept both) and for 0014-0019 (clean), then `git format-patch` over the result. The 14 patches in `patches/jm-v7-wip/` were regenerated (same names, same subjects, same content apart from context and offsets).
- Verified: the regenerated set applies strictly (`git apply --check -v` reports no offset or fuzz, then `git am`) in order on a fresh 197-patch tree.
- The RC5-dev-v7E/v7F binaries were built from the earlier base (192 patches); the regenerated patches differ only by context, so the driver code is the same. A rebuild on the 197 base is a device-session decision.
- Debug flag values on the combined tree: `NO_TESS = 1 << 28`, `BC_TIMING = 1 << 29` (no collision).

## Docs
- PROGRESS.md: new section "State 2026-10-21".
- `2026-10-21-rc4-emulator-package.md`: note that RC4 (not RC4-lowmem) is reproducible.
