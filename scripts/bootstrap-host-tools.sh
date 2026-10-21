#!/bin/sh
# bootstrap-host-tools.sh — build pinned host codegen tools (mesa_clc, vtn_bindgen2, ...)
# Inspects the Mesa revision instead of hard-coding an old tool list.
# Usage: ./scripts/bootstrap-host-tools.sh [--mesa work/mesa] [--out build/host-tools]
set -eu
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MESA="$ROOT/work/mesa"; OUT="$ROOT/build/host-tools"
while [ $# -gt 0 ]; do case "$1" in --mesa) MESA="$2"; shift 2;; --out) OUT="$2"; shift 2;; *) echo "unknown $1" >&2; exit 2;; esac; done
mkdir -p "$OUT"
# Opt-in: PANVK_HOST_TOOLS_CACHE=<dir> reuses host tools built for the same Mesa sha.
if [ -n "${PANVK_HOST_TOOLS_CACHE:-}" ]; then
  SHA="$(git -C "$MESA" rev-parse HEAD 2>/dev/null || echo unknown)"
  CACHE="$PANVK_HOST_TOOLS_CACHE/$SHA"
  if [ -x "$CACHE/mesa_clc" ] && [ -x "$CACHE/vtn_bindgen2" ] && [ -x "$CACHE/panfrost_compile" ]; then
    mkdir -p "$OUT/bin"; cp -f "$CACHE"/mesa_clc "$CACHE"/vtn_bindgen2 "$CACHE"/panfrost_compile "$OUT/bin/"
    echo "OK host tools restored from cache $CACHE"; exit 0
  fi
fi
echo "mesa: $(git -C "$MESA" rev-parse --short HEAD 2>/dev/null || echo MISSING)"
if [ ! -f "$OUT/build.ninja" ]; then
  meson setup "$OUT" "$MESA" -Dbuildtype=release -Dgallium-drivers= -Dvulkan-drivers=panfrost -Dplatforms= -Dglx=disabled -Degl=disabled -Dgbm=disabled -Dopengl=false -Dgles1=disabled -Dgles2=disabled -Dmesa-clc=enabled -Dinstall-mesa-clc=true -Dprecomp-compiler=enabled -Dinstall-precomp-compiler=true -Dllvm=enabled -Dzstd=disabled
fi
ninja -j"${PANVK_JOBS:-$(nproc)}" -C "$OUT" src/compiler/clc/mesa_clc src/compiler/spirv/vtn_bindgen2 src/panfrost/clc/panfrost_compile
mkdir -p "$OUT/bin"
for p in src/compiler/clc/mesa_clc src/compiler/spirv/vtn_bindgen2 src/panfrost/clc/panfrost_compile; do
  b="$(basename "$p")"
  rm -f "$OUT/bin/$b"
  ln -sf "../$p" "$OUT/bin/$b"
done
if [ -n "${CACHE:-}" ]; then
  mkdir -p "$CACHE"
  for b in mesa_clc vtn_bindgen2 panfrost_compile; do cp -fL "$OUT/bin/$b" "$CACHE/$b"; done
fi
echo "OK out=$OUT (P4 full tool build runs on Poco/NativeCode; CI re-runs this script)"
