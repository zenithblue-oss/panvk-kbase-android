#!/bin/sh
# Build the PanPlay D3D12 gears test: vkd3d-proton demos/gears.c at 2230755878b01993b4b82d0ac0c0624f3ab4d333
# (LGPL-2.1-or-later, glxgears-derived parts MIT; shaders are the Microsoft-fxc DXBC arrays in
# gears_vs.h / gears_ps_*.h) + patches/vkd3d-gears-panplay.patch, compiled standalone with llvm-mingw
# against mingw-w64 d3d12/dxgi headers (shim-gears/ replaces vkd3d-proton's widl-generated headers).
# usage: build-vkd3d-gears.sh <patched demos dir> <out dir> [x86_64|arm64ec|aarch64|i686 ...]
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
SRC=$1
OUT=$2
shift 2
ARCHES=${*:-x86_64}
MINGW=${MINGW:-/home/flux/build-cache/dx12-test/toolchain/llvm-mingw-20260922-ucrt-ubuntu-22.04-aarch64}
export SOURCE_DATE_EPOCH=${SOURCE_DATE_EPOCH:-1}
mkdir -p "$OUT"
for arch in $ARCHES; do
    CC=$MINGW/bin/$arch-w64-mingw32-clang
    "$CC" -std=gnu11 -O2 -DNDEBUG -D_WIN32_WINNT=0x0A00 -DWINVER=0x0A00 -Wno-unused-command-line-argument \
        -I"$HERE/shim-gears" -I"$HERE/common" -I"$SRC" "$SRC/gears.c" -static -s -mconsole \
        -Wl,--no-insert-timestamp -o "$OUT/vkd3d-gears-$arch.exe" -ld3d12 -ldxgi -luser32 -lshell32 -lgdi32 -lm
    echo "built $OUT/vkd3d-gears-$arch.exe"
done
