# Signing the release APKs

PanPlay and PanProbe 1.3.0 are built as **unsigned release APKs** and signed only by the holder of the release key.
The key never enters this repo or the build output. Android installs an update only when the new APK is signed with
the same certificate as the installed one. The shipped 1.2.x releases use the certificate with SHA-256
`8b10fde8c12eff4a29e99ae669ba88854e6003178d9c3ec299254fe87c76f3bf` ([RELEASE.md](RELEASE.md)), so 1.3.0 must be
signed with that key. An APK signed with another key installs only after an uninstall, which deletes the app's data
(PanPlay: containers and games).

## Inputs

`/home/flux/build-out/release-115/` (build 115b driver, `Mesa 26.3.0-devel (git-ad5cd69141)`):

| file | package | version |
|---|---|---|
| `PanPlay-1.3.0-unsigned.apk` | `dev.zenithblue.panvklauncher` | 1.3.0, versionCode 43 |
| `PanProbe-1.3.0-unsigned.apk` | `dev.zenithblue.panvktest` | 1.3.0, versionCode 18 |
| `SHA256SUMS` | sha256 of both unsigned APKs | |

They are `assembleRelease` outputs: release build type with the projects' release settings (neither app has a
`release` block, so AGP defaults: no R8 minify, no resource shrinking, not debuggable), native libraries stored
compressed (`useLegacyPackaging`), zipaligned (`zipalign -c -p 4` passes). Not being debuggable means `run-as` and the
debug-only intent extras (PanPlay's launch-shortcut extra, PanProbe's upload dry run, the driver-version override) do
not work in these builds; the Firebase game loop does not use them.

## Sign and verify

apksigner is `/opt/android-sdk/build-tools/36.1.0/apksigner`. It asks for the keystore and key passwords.

```sh
cd /home/flux/build-out/release-115
sha256sum -c SHA256SUMS
apksigner sign --ks <keystore> --ks-key-alias <alias> --out PanPlay-1.3.0.apk PanPlay-1.3.0-unsigned.apk
apksigner sign --ks <keystore> --ks-key-alias <alias> --out PanProbe-1.3.0.apk PanProbe-1.3.0-unsigned.apk
apksigner verify --print-certs PanPlay-1.3.0.apk
apksigner verify --print-certs PanProbe-1.3.0.apk
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
