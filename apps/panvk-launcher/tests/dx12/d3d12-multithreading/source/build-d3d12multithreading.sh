#!/bin/sh
# Build the PanPlay D3D12 Multithreading test: Microsoft DirectX-Graphics-Samples
# Samples/Desktop/D3D12Multithreading at 0f86e460bec17ed5231549748599a91ca5093132 (MIT, last revision
# before the Agility SDK / DXC / enhanced-barriers port) + patches/d3d12multithreading-panplay.patch,
# with llvm-mingw (statically linked libc++/libunwind/winpthread; imports only system DLLs + d3d12 + dxgi).
# usage: build-d3d12multithreading.sh <patched src dir> <DirectXMath Inc dir> <out dir> [x86_64|arm64ec|aarch64|i686 ...]
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
SRC=$1
DXM=$2
OUT=$3
shift 3
ARCHES=${*:-x86_64}
MINGW=${MINGW:-/home/flux/build-cache/dx12-test/toolchain/llvm-mingw-20260922-ucrt-ubuntu-22.04-aarch64}
export SOURCE_DATE_EPOCH=${SOURCE_DATE_EPOCH:-1}
for arch in $ARCHES; do
    CXX=$MINGW/bin/$arch-w64-mingw32-clang++
    B=$OUT/obj-$arch
    mkdir -p "$B"
    objs=
    for f in Camera D3D12Multithreading DXSample FrameResource Main Win32Application; do
        "$CXX" -std=c++17 -O2 -DNDEBUG -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0A00 -DWINVER=0x0A00 \
            -D_uuidof=__uuidof -Wno-address-of-temporary -Wno-unused-command-line-argument \
            -I"$HERE/shim-mt" -I"$HERE/common" -I"$DXM" -I"$SRC" -c "$SRC/$f.cpp" -o "$B/$f.o"
        objs="$objs $B/$f.o"
    done
    "$CXX" -static -s -mconsole -Wl,--no-insert-timestamp -o "$OUT/d3d12-multithreading-$arch.exe" $objs \
        -ld3d12 -ldxgi -ldxguid -luser32 -lshell32 -lkernel32
    echo "built $OUT/d3d12-multithreading-$arch.exe"
done
