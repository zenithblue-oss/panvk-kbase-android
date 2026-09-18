#!/bin/sh
# apply-patches.sh — apply qualified patch series for a profile, fail hard on drift
# Usage: ./scripts/apply-patches.sh --profile g615-v11-csf [--mesa work/mesa]
set -eu
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PROFILE=""; MESA="$ROOT/work/mesa"
while [ $# -gt 0 ]; do case "$1" in
  --profile) PROFILE="$2"; shift 2;;
  --mesa) MESA="$2"; shift 2;;
  *) echo "unknown arg $1" >&2; exit 2;;
esac; done
[ -n "$PROFILE" ] || { echo "--profile required" >&2; exit 2; }
ARCH="$(python3 -c "import json;print(json.load(open('$ROOT/profiles/$PROFILE.json'))['panArch'])")"
FRONTEND="$(python3 -c "import json;print(json.load(open('$ROOT/profiles/$PROFILE.json'))['frontend'])")"
SERIES="common android kbase-common app-loader wsi"
if [ "$FRONTEND" = "CSF" ]; then SERIES="$SERIES csf csf-v$ARCH"; else SERIES="$SERIES jm-v$ARCH"; fi
echo "profile=$PROFILE arch=$ARCH frontend=$FRONTEND series: $SERIES"
git -C "$MESA" status --porcelain | head -n 5
APPLIED=0
for fam in $SERIES; do
  for p in "$ROOT"/patches/"$fam"/*.patch; do
    [ -e "$p" ] || continue
    echo "APPLY [$fam] $(basename "$p")"
    git -C "$MESA" apply --check "$p" || { echo "PATCH-DRIFT: $p does not apply" >&2; exit 1; }
    git -C "$MESA" apply "$p"
    APPLIED=$((APPLIED+1))
  done
done
echo "OK applied=$APPLIED profile=$PROFILE"
