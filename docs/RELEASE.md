# Release

Tiers: `dev` < `alpha` < `beta` < `rc` < `stable`. First Poco build is never
stable. Tags like `g615-v11-csf-v0.1.0-beta.1`. Immutable: patch change = new
release even if Mesa SHA unchanged.

Android asset: `Mesa-PanVK-Android-<version>-<sha8>.adpkg.zip`
(flat: `libvulkan_panfrost.so + meta.json + MANIFEST.json + SOURCE.json +
VALIDATION.json + SHA256SUMS.txt + runtime-feature-matrix.json`).
glibc asset: `Mesa-PanVK-<version>-<sha8>-EMULATOR.zip`
(+ `panfrost_icd.aarch64.json` and the same provenance files).
Launcher zips: `Mesa-PanVK-Android-<version>-<sha8>-<launcher>.zip`.
`<sha8>` is the git sha the driver reports in `driverInfo`
(`Mesa 26.3.0-devel (git-<sha>)`, like an upstream Mesa PanVK development
build); meta.json `name` is `Mesa-PanVK-<Mesa version>-<sha8>` and
`driverVersion` the exact `driverInfo` (`scripts/driver_identity.py`).
Releases up to RC4 used `PanVK-Kbase-Android-<version>-<mesa8>...` with
`driverInfo` `PanVK-kbase <release> (Mesa ...)`. Release title:
`Mesa PanVK <tag>`; tags keep the `panvk-kbase-v` prefix.
`minApi` is 35 until an API 29 Bionic rebuild is proven. Do not advertise 29.

App update check: PanPlay and PanProbe compare a release's tag version (after
`panvk-kbase-v`) with the bundled driver's `packageVersion`, which is the Mesa
version from build 115 on (`26.3.0-devel`, was `0.1.0-rc5`). `DriverUpdate.isNewer`
compares major.minor.patch, then beta/rc, so only a tag above 26.3.0 (for example
`panvk-kbase-v26.3.1`) is offered as an update to these builds; a `0.1.x` tag is not.

App versions: PanPlay 1.3.0 (versionCode 41) and PanProbe 1.3.0 (versionCode 16), tagged
`panplay-v<version>` and `panprobe-v<version>` in their repos. A new versionCode must be above
every APK handed out: PanPlay 33 (1.2.7), 34 (1.2.8, sideloaded) and 35-40 (1.2.9-rc5 test
builds); PanProbe 15 (1.2.7). The `-PpanprobeSuffix=rc5u` test APKs are a separate package
(`dev.zenithblue.panvktest.rc5u`). Android installs an update only when it is signed with the
same key as the installed APK. The releases up to 1.2.7 are signed with the certificate
SHA-256 `8b10fde8c12eff4a29e99ae669ba88854e6003178d9c3ec299254fe87c76f3bf`, so 1.3.0 must be
signed with that key too (`apksigner verify --print-certs`). An APK signed with another key
installs only after an uninstall, which deletes the app's data (PanPlay: containers and games).

Every release: `SHA256SUMS.txt + SOURCE.json + VALIDATION.json` with Mesa
commit, patch hash, compiler/NDK/Meson, host, API, Build ID, SHAs, device,
GPU ID, UAPI, all gate results. CI builds `dev` only; beta/rc/stable require
Poco (or declared device) hardware validation evidence. CI success is never a
substitute for `/dev/mali0` runtime testing.

Fault safety: hard timeouts, small allocations, tiny images, single queue,
bounded frames, one change per run. Never spam CSF kicks after fault, fake
DRM nodes/features, guess Kbase structs across DDKs, or mix JM/CSF ioctls.
After severe fault: save logs + kernel evidence, stop, reboot if needed.

## Release checklist

- Every release attaches, and embeds at the very top of the release notes
  (before any text section), the latest panvk-test screenshots and the latest
  panvk-launcher 3D (DXVK cube, with HUD) screenshots. Give each launcher image
  its arch and API.
- Embed only committed files, via
  `https://raw.githubusercontent.com/zenithblue-oss/panvk-kbase-android/<tag-or-sha>/<path>`,
  and check each URL returns 200 (`curl -sI`) after publishing. Upload the same
  images as release assets.
- Build assets from a clean worktree of the release commit, not the dirty
  working tree; confirm the patch series applies on the pinned Mesa base.
  The universal driver's tree is `scripts/prepare-rc5-tree.sh --mesa <fresh
  5a07217f worktree>` (strict; its defaults, csf-v11 <= 233 and jm-v7-wip
  <= 0131, are build 115), built with `MESA=<that tree>
  scripts/build-android.sh --profile g615-v11-csf --api 29`.
