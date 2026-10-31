# Signing the release APKs

PanPlay 1.3.1 and PanProbe 1.3.2 (build 122i) are built as **unsigned release APKs** and signed only by the holder of the release key.
The key never enters this repo or the build output. Android installs an update only when the new APK is signed with
the same certificate as the installed one. The shipped 1.2.x releases use the certificate with SHA-256
`8b10fde8c12eff4a29e99ae669ba88854e6003178d9c3ec299254fe87c76f3bf` ([RELEASE.md](RELEASE.md)), so 1.3.1/1.3.2 must be
signed with that key. An APK signed with another key installs only after an uninstall, which deletes the app's data
(PanPlay: containers and games).

## Inputs

`/var/tmp/panvk/release-122i/` (build 122i driver, `Mesa 26.3.0-devel (git-795ca7ff3e, p0175, 2026-10-31)`):

| file | package | version |
|---|---|---|
| `PanPlay-1.3.1-unsigned.apk` | `dev.zenithblue.panvklauncher` | 1.3.1, versionCode 45 |
| `PanProbe-1.3.2-unsigned.apk` | `dev.zenithblue.panvktest` | 1.3.2, versionCode 21 |
| `Mesa-PanVK-Android-v26.3.1-795ca7ff.adpkg.zip` | driver package (not signed) | |
| `SHA256SUMS` | sha256 of the APKs, the adpkg and the screenshots | |

They are `assembleRelease` outputs: release build type with the projects' release settings (neither app has a
`release` block, so AGP defaults: no R8 minify, no resource shrinking, not debuggable), native libraries stored
compressed (`useLegacyPackaging`), zipaligned (`zipalign -c -p 4` passes). Not being debuggable means `run-as` and the
debug-only intent extras (PanPlay's launch-shortcut extra, PanProbe's upload dry run, the driver-version override) do
not work in these builds; the Firebase game loop does not use them.
In a clean worktree, PanProbe needs the Vulkan headers at `work/mesa-dxint/include` (untracked; link it from the main
checkout), or NDK headers that lack `VK_ATTACHMENT_LOAD_OP_NONE_KHR` fail the build.

## Sign and verify

apksigner is `/opt/android-sdk/build-tools/36.1.0/apksigner`. It asks for the keystore and key passwords.

```sh
cd /var/tmp/panvk/release-122i
sha256sum -c SHA256SUMS
apksigner sign --ks <keystore> --ks-key-alias <alias> --out PanPlay-1.3.1.apk PanPlay-1.3.1-unsigned.apk
apksigner sign --ks <keystore> --ks-key-alias <alias> --out PanProbe-1.3.2.apk PanProbe-1.3.2-unsigned.apk
apksigner verify --print-certs PanPlay-1.3.1.apk
apksigner verify --print-certs PanProbe-1.3.2.apk
```

Each `verify` must print

```
Signer #1 certificate SHA-256 digest: 8b10fde8c12eff4a29e99ae669ba88854e6003178d9c3ec299254fe87c76f3bf
```

Any other digest means the wrong key: do not publish that APK.

The same in one step: `scripts/sign-release.sh <keystore> <alias> [dir]`. It checks `SHA256SUMS`, signs both APKs,
checks the certificate digest (and deletes an APK signed with another key), and writes `SHA256SUMS-signed`.

Do not zipalign or otherwise rewrite an APK after signing: that breaks the v2/v3 signature. apksigner may also write
a `.apk.idsig` (v4 signature) next to each output; it is not needed for sideloading or GitHub releases.
