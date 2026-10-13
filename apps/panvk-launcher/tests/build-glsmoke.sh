#!/bin/sh
# Build the WGL smoke test (tests/glsmoke.c) for ARM64EC and x86_64 into tests/build/.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
MINGW=${MINGW:-/var/tmp/panvk/launcher-tests/llvm-mingw-20260922-ucrt-ubuntu-22.04-x86_64}
mkdir -p "$ROOT/build"
for t in arm64ec x86_64; do
    "$MINGW/bin/$t-w64-mingw32-clang" -O2 -Wall -o "$ROOT/build/glsmoke-$t.exe" "$ROOT/glsmoke.c" -lopengl32 -lgdi32 -luser32
done
