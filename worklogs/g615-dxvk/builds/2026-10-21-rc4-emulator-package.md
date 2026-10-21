# 2026-10-21 RC4 EMULATOR and adpkg packages

Problem: RC4 and RC4-lowmem shipped without the RC3-style `-EMULATOR.zip` (and adpkg), so ArmSX2 users (issues #22, #17) had nothing in the RC3 layout.

Decision: do not rebuild. The RC4 patch series is not reproducible from the repo, so a rebuilt .so would not be RC4. Instead the released Android .so was wrapped in the RC3 layout with `scripts/package-prebuilt.sh`. SOURCE.json says the binary is from a release asset and patchSeriesId is "not-reproducible". minApi=29. No glibc .so was produced or uploaded; the EMULATOR zip carries the bionic driver.

Uploaded (no clobber):
- rc4: `PanVK-Kbase-v0.1.0-rc4-5a07217f-EMULATOR.zip` sha256 e7691c11902c2e29beab028d0da2ca7cecf3f05125e483f3061ee662b49d5c33
- rc4: `PanVK-Kbase-Android-v0.1.0-rc4-5a07217f.adpkg.zip` sha256 00d180657d542a0bd88cbaf178d4c5e7f7a4413d27986db7ee59c2ed47aa6d89
- rc4-lowmem: `PanVK-Kbase-v0.1.0-rc4-lowmem-54376028-EMULATOR.zip` sha256 0c7d6b75ae03308025b8069d26a332a9dc80c5fd1bda90f7828e5e0378607095
- rc4-lowmem: `PanVK-Kbase-Android-v0.1.0-rc4-lowmem-54376028.adpkg.zip` sha256 84b788a47e38852bf4e260c12fa7082c98371599274066a42f5f12bc2b8ee320

Wrapped binaries: rc4 977c0c4c0803b9940c453d691f50f14c9f8f732fe945d93bdde637880befd791, lowmem c395db09ccf160d0419b3f25fa99b6b7fe0e87d61c3780b03ea37cce4930c8a5 (both match the release SHA256SUMS). Release SHA256SUMS assets updated, release notes appended.

Repo changes (fb49ca3): `scripts/package-launcher-zips.sh` (reproduces the six launcher zips byte-for-byte in content: verified by diffing against RC4's; gamehub renames to libvulkan_freedreno.so), `scripts/package-prebuilt.sh`, `release-local.sh` (launcher zips; optional WITH_GLIBC=1 glibc build), `release.yml` (rc/beta must have EMULATOR, adpkg, glibc .so and the launcher zips).

Note: validate-package.sh was not run; it rejects minApi<35 and a non-reproducible patchSeriesId by design.

Comments: issue #22 (6310189271), #17 (6310189709).

## Rename (g615 removed, 2026-10-21)
The driver supports all Mali GPUs, so g615 was dropped from every user-facing name. The EMULATOR/adpkg zips above were
regenerated with the new names (same released .so files), and the six launcher zips on both releases were regenerated
by scripts/package-launcher-zips.sh with meta name `PanVK-kbase-RC4` / `PanVK-kbase-RC4-LM` (no G615). Old g615-named assets
were deleted after the new ones were uploaded; SHA256SUMS and the release notes were updated. The internal build profile id
`g615-v11-csf` and the patch dir names are unchanged. New launcher zip sha256 values are in each release's SHA256SUMS.
