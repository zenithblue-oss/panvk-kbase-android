#!/bin/sh
# package-glibc.sh — Bachata-compatible flat ZIP: .so + ICD json + meta.json + MANIFEST.json
# Usage: ./scripts/package-glibc.sh --profile g615-v11-csf --version v0.1.0-beta.1
set -eu
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PROFILE=""; VER="v0.0.0-dev"
while [ $# -gt 0 ]; do case "$1" in --profile) PROFILE="$2"; shift 2;; --version) VER="$2"; shift 2;; *) echo "unknown $1" >&2; exit 2;; esac; done
[ -n "$PROFILE" ] || { echo "--profile required" >&2; exit 2; }
SRC="$ROOT/dist/glibc-$PROFILE/libvulkan_panfrost.so"
[ -f "$SRC" ] || { echo "build first: $SRC missing" >&2; exit 1; }
MESA_SHA="$(git -C "$ROOT/work/mesa" rev-parse HEAD 2>/dev/null || echo unknown)"
OUT="$ROOT/dist/PanVK-Kbase-$PROFILE-$VER-${MESA_SHA:0:8}-EMULATOR.zip"
STAGE="$(mktemp -d)"; cp "$SRC" "$STAGE/libvulkan_panfrost.so"
cp "$ROOT/dist/glibc-$PROFILE/panfrost_icd.aarch64.json" "$STAGE/" 2>/dev/null || \
  printf '{"file_format_version":"1.0.0","ICD":{"library_path":"libvulkan_panfrost.so","api_version":"1.4.0"}}\n' > "$STAGE/panfrost_icd.aarch64.json"
python3 - "$STAGE/meta.json" "$PROFILE" "$MESA_SHA" <<'EOF'
import sys, json
out, profile, sha = sys.argv[1], sys.argv[2], sys.argv[3]
meta = {"schemaVersion":1,"name":f"PanVK Kbase {profile.upper()} (glibc)","author":"panvk-kbase-android",
 "packageVersion":"1","vendor":"Mesa","driverVersion":"Mesa 26.3.0-devel / Vulkan 1.4.x","minApi":29,
 "description":"Experimental PanVK Kbase glibc ICD for Bachata/NativeCode",
 "libraryName":"libvulkan_panfrost.so","abi":"linux-aarch64-glibc","backend":"panvk-kbase",
 "kernelInterface":"mali_kbase","profile":profile,"sourceCommit":sha,"patchSeriesId":"PENDING-P3-REBASE"}
json.dump(meta, open(out,'w'), indent=2)
EOF
(cd "$STAGE" && sha256sum libvulkan_panfrost.so > SHA256SUMS.txt)
python3 "$ROOT/scripts/generate-release-manifest.py" --profile "$PROFILE" --abi linux-aarch64-glibc --stage "$STAGE"
(cd "$STAGE" && zip -q "$OUT" libvulkan_panfrost.so panfrost_icd.aarch64.json meta.json MANIFEST.json SHA256SUMS.txt SOURCE.json VALIDATION.json)
"$ROOT/scripts/validate-package.sh" "$OUT"
echo "OK $OUT"
