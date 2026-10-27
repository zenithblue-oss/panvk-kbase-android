#!/bin/sh
# prepare-rc5-tree.sh — build the universal Mesa PanVK tree (v6/v7/v9-v14 in one ICD)
#
# Mesa 5a07217 + the g615-v11-csf series in apply-patches.sh order (common android
# kbase-common app-loader wsi csf csf-v11[<=CSF_MAX] jm-v9 jm-v7 001-005), one commit
# per patch, then `git am` of patches/jm-v7-wip/0006..WIP_MAX in file order.
# Strict: any offset, fuzz or conflict stops the script (same rule as apply-patches.sh).
# The defaults (csf-v11 <= 233, jm-v7-wip <= 0131) give the build 115 release tree; older
# builds name their limits in worklogs/g615-dxvk/builds/rc5-universal.md.
#
# Usage:
#   git -C /home/flux/src/mesa worktree add -b p115-rel /home/flux/src/mesa-115-rel 5a07217f
#   scripts/prepare-rc5-tree.sh --mesa /home/flux/src/mesa-115-rel [--csf-max 233] [--wip-max 0131] [--base-tag <tag>]
set -eu
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MESA=""; CSF_MAX=233; WIP_MAX=0131; BASE_TAG=""
while [ $# -gt 0 ]; do case "$1" in
  --mesa) MESA="$2"; shift 2;;
  --csf-max) CSF_MAX="$2"; shift 2;;
  --wip-max) WIP_MAX="$2"; shift 2;;
  --base-tag) BASE_TAG="$2"; shift 2;;
  *) echo "unknown arg $1" >&2; exit 2;;
esac; done
[ -n "$MESA" ] || { echo "--mesa required" >&2; exit 2; }
[ "$(git -C "$MESA" rev-parse HEAD)" = "$(git -C "$MESA" rev-parse 5a07217f)" ] || {
  echo "$MESA must be at Mesa 5a07217f" >&2; exit 1; }
[ -z "$(git -C "$MESA" status --porcelain)" ] || { echo "$MESA is not clean" >&2; exit 1; }

strict_check() {
  # prints the --recount flag if needed; exits on drift/offset/fuzz
  p="$1"; RC=""
  git -C "$MESA" apply --check "$p" 2>/dev/null || RC="--recount"
  if ! VOUT="$(git -C "$MESA" apply $RC --check -v "$p" 2>&1)"; then
    echo "$VOUT" >&2; echo "PATCH-DRIFT: $p does not apply" >&2; exit 1
  fi
  if echo "$VOUT" | grep -qiE 'offset|fuzz'; then
    echo "$VOUT" | grep -iE 'offset|fuzz|Checking' >&2
    echo "PATCH-DRIFT: $p applies only with offset/fuzz (refresh it)" >&2; exit 1
  fi
  echo "$RC"
}

N=0
for fam in common android kbase-common app-loader wsi csf csf-v11 jm-v9 jm-v7; do
  for p in "$ROOT"/patches/"$fam"/*.patch; do
    [ -e "$p" ] || continue
    b="$(basename "$p")"
    if [ "$fam" = csf-v11 ] && [ "${b%%-*}" -gt "$CSF_MAX" ]; then continue; fi
    RC="$(strict_check "$p")"
    git -C "$MESA" apply $RC "$p"
    git -C "$MESA" add -A
    git -C "$MESA" commit -q --no-verify -m "series: $fam/$b"
    N=$((N+1))
  done
  if [ -d "$ROOT/patches/$fam/files" ]; then
    (cd "$ROOT/patches/$fam/files" && find . -type f ! -name README -print0 | tar --null -cf - -T -) | (cd "$MESA" && tar -xf -)
    git -C "$MESA" add -A
    git -C "$MESA" commit -q --no-verify --allow-empty -m "series: $fam/files"
  fi
done
echo "BASE-OK applied=$N csf-v11<=$CSF_MAX head=$(git -C "$MESA" rev-parse --short HEAD)"
[ -z "$BASE_TAG" ] || git -C "$MESA" tag -f "$BASE_TAG" >/dev/null

M=0
for p in "$ROOT"/patches/jm-v7-wip/0*.patch; do
  b="$(basename "$p")"
  [ "${b%%-*}" -le "$WIP_MAX" ] || continue
  if ! VOUT="$(git -C "$MESA" apply --check -v "$p" 2>&1)"; then
    echo "$VOUT" >&2; echo "WIP-CONFLICT: $b (resolve with git am -3, re-export)" >&2; exit 1
  fi
  if echo "$VOUT" | grep -qiE 'offset|fuzz'; then
    echo "$VOUT" | grep -iE 'offset|fuzz|Checking' >&2
    echo "WIP-DRIFT: $b applies only with offset/fuzz (refresh it)" >&2; exit 1
  fi
  git -C "$MESA" am -q "$p"
  M=$((M+1))
done
echo "OK base=$N jm-v7-wip=$M head=$(git -C "$MESA" rev-parse --short HEAD)"
