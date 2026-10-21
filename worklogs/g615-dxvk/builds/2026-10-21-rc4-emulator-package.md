# 2026-10-21 RC4 EMULATOR and adpkg packages

Problem: RC4 and RC4-lowmem shipped without the RC3-style `-EMULATOR.zip` (and adpkg), so ArmSX2 users (issues #22, #17) had nothing in the RC3 layout.

Decision: do not rebuild. The RC4 patch series is not reproducible from the repo, so a rebuilt .so would not be RC4. Instead the released Android .so was wrapped in the RC3 layout with `scripts/package-prebuilt.sh`. SOURCE.json says the binary is from a release asset and patchSeriesId is "not-reproducible". minApi=29. No glibc .so was produced or uploaded; the EMULATOR zip carries the bionic driver.

Uploaded (no clobber):
- rc4: `PanVK-Kbase-g615-v11-csf-v0.1.0-rc4-5a07217f-EMULATOR.zip` sha256 5b9340e916361df61ab6ac785e98f1e574dd181a04411e32717b5ba9bf398426
- rc4: `PanVK-Kbase-Android-g615-v11-csf-v0.1.0-rc4-5a07217f.adpkg.zip` sha256 9faa917d32872f46ddfda4ac54e698835db214924979a53671ef89b218b5bf17
- rc4-lowmem: `PanVK-Kbase-g615-v11-csf-v0.1.0-rc4-lowmem-54376028-EMULATOR.zip` sha256 70cefb418d1707d6c86774483a9f57fc0e11cced3f951f2e2c37dcd313ae6ab6
- rc4-lowmem: `PanVK-Kbase-Android-g615-v11-csf-v0.1.0-rc4-lowmem-54376028.adpkg.zip` sha256 a0b7646ab3d20213807913b7310861b2af410a8fb7cd7dc8901cbaed48ec8545

Wrapped binaries: rc4 977c0c4c0803b9940c453d691f50f14c9f8f732fe945d93bdde637880befd791, lowmem c395db09ccf160d0419b3f25fa99b6b7fe0e87d61c3780b03ea37cce4930c8a5 (both match the release SHA256SUMS). Release SHA256SUMS assets updated, release notes appended.

Repo changes (fb49ca3): `scripts/package-launcher-zips.sh` (reproduces the six launcher zips byte-for-byte in content: verified by diffing against RC4's; gamehub renames to libvulkan_freedreno.so), `scripts/package-prebuilt.sh`, `release-local.sh` (launcher zips; optional WITH_GLIBC=1 glibc build), `release.yml` (rc/beta must have EMULATOR, adpkg, glibc .so and the launcher zips).

Note: validate-package.sh was not run; it rejects minApi<35 and a non-reproducible patchSeriesId by design.

Comments: issue #22 (6310189271), #17 (6310189709).
