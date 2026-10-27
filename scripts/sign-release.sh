#!/bin/sh
# Sign the unsigned PanPlay and PanProbe 1.3.0 release APKs with the release key and check the certificate.
# usage: scripts/sign-release.sh <keystore> <key alias> [dir with *-1.3.0-unsigned.apk, default /home/flux/build-out/release-115]
# apksigner asks for the keystore and key passwords. Writes <App>-1.3.0.apk and SHA256SUMS-signed next to the inputs.
# Nothing here creates or stores a key. Steps and background: docs/SIGNING.md.
set -eu
usage="usage: $0 <keystore> <key alias> [dir]"
KS=${1:?$usage}
ALIAS=${2:?$usage}
DIR=${3:-/home/flux/build-out/release-115}
VER=1.3.0
APKSIGNER=${APKSIGNER:-/opt/android-sdk/build-tools/36.1.0/apksigner}
# SHA-256 of the certificate that signed the shipped 1.2.x releases (docs/RELEASE.md): updates install only with it.
WANT=8b10fde8c12eff4a29e99ae669ba88854e6003178d9c3ec299254fe87c76f3bf
[ -f "$KS" ] || { echo "keystore not found: $KS" >&2; exit 2; }
cd "$DIR"
sha256sum -c SHA256SUMS
for app in PanPlay PanProbe; do
    "$APKSIGNER" sign --ks "$KS" --ks-key-alias "$ALIAS" --out "$app-$VER.apk" "$app-$VER-unsigned.apk"
    if "$APKSIGNER" verify --print-certs "$app-$VER.apk" | grep -q "certificate SHA-256 digest: $WANT"; then
        echo "ok: $DIR/$app-$VER.apk is signed with the release certificate $WANT"
    else
        "$APKSIGNER" verify --print-certs "$app-$VER.apk" >&2 || true
        echo "WRONG KEY: $app-$VER.apk is not signed with $WANT; do not publish it" >&2
        rm -f "$app-$VER.apk" "$app-$VER.apk.idsig"
        exit 1
    fi
done
sha256sum "PanPlay-$VER.apk" "PanProbe-$VER.apk" > SHA256SUMS-signed
cat SHA256SUMS-signed
