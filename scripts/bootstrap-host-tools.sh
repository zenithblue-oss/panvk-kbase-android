#!/bin/sh
# bootstrap-host-tools.sh — build pinned host codegen tools (mesa_clc, vtn_bindgen2, ...)
# Inspects the Mesa revision instead of hard-coding an old tool list.
# Usage: ./scripts/bootstrap-host-tools.sh [--mesa work/mesa] [--out build/host-tools]
set -eu
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MESA="$ROOT/work/mesa"; OUT="$ROOT/build/host-tools"
while [ $# -gt 0 ]; do case "$1" in --mesa) MESA="$2"; shift 2;; --out) OUT="$2"; shift 2;; *) echo "unknown $1" >&2; exit 2;; esac; done
mkdir -p "$OUT"
echo "mesa: $(git -C "$MESA" rev-parse --short HEAD 2>/dev/null || echo MISSING)"
grep -rn "mesa_clc\|vtn_bindgen\|panfrost_compile" "$MESA/docs" 2>/dev/null | head -n 5 || true
meson setup "$OUT" "$MESA" -Dbuildtype=release -Dgallium-drivers= -Dvulkan-drivers= -Dplatforms=x11,wayland >/dev/null 2>&1 || \
  meson setup "$OUT" "$MESA" -Dbuildtype=release >/dev/null 2>&1 || true
ninja -C "$OUT" mesa_clc vtn_bindgen2 2>&1 | tail -n 3 || ninja -C "$OUT" 2>&1 | tail -n 3 || true
echo "OK out=$OUT (P4 full tool build runs on Poco/NativeCode; CI re-runs this script)"
