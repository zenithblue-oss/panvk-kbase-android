#!/bin/sh
# Build the Wine vkd3d HLSL compiler (vkd3d-compiler) for the aarch64 build host, without running the
# tarball's configure script: our own config.h + direct gcc calls. Used only to precompile the test's
# HLSL to DXBC SM5.0 (fxc-style), so the shipped test needs no d3dcompiler_47.dll at runtime.
# usage: VULKAN_INC=<dir with vulkan/vulkan.h> build-vkd3d-compiler.sh <vkd3d-1.19 source dir> <build dir>
set -eu
SRC=$1
B=$2
mkdir -p "$B/gen" "$B/obj"

cat > "$B/gen/config.h" <<'EOF'
#define PACKAGE_VERSION "1.19"
#define HAVE_ATOMIC_EXCHANGE_N 1
#define HAVE_BUILTIN_ADD_OVERFLOW 1
#define HAVE_BUILTIN_CLZ 1
#define HAVE_BUILTIN_CTZ 1
#define HAVE_BUILTIN_POPCOUNT 1
#define HAVE_DLFCN_H 1
#define HAVE_GETTID 1
#define HAVE_PTHREAD_H 1
#define HAVE_PTHREAD_SETNAME_NP_2 1
#define HAVE_SYNC_ADD_AND_FETCH 1
#define HAVE_SYNC_BOOL_COMPARE_AND_SWAP 1
#define HAVE_SPIRV_UNIFIED1_SPIRV_H 1
#define HAVE_SPIRV_UNIFIED1_GLSL_STD_450_H 1
EOF
printf '#define VKD3D_VCS_ID " (vkd3d-1.19 release tarball)"\n' > "$B/gen/vkd3d_version.h"

flex -o "$B/gen/preproc.yy.c" "$SRC/libs/vkd3d-shader/preproc.l"
bison -d -o "$B/gen/preproc.tab.c" "$SRC/libs/vkd3d-shader/preproc.y"
flex -o "$B/gen/hlsl.yy.c" "$SRC/libs/vkd3d-shader/hlsl.l"
bison -d -o "$B/gen/hlsl.tab.c" "$SRC/libs/vkd3d-shader/hlsl.y"

CFLAGS="-O2 -pipe -std=c99 -D_GNU_SOURCE -D_WIN32_WINNT=0x0600 -w"
INC="-I$B/gen -I$SRC/include -I$SRC/include/private -I$SRC/libs/vkd3d-shader -I${VULKAN_INC:?set VULKAN_INC to a Vulkan-Headers include dir}"
objs=
for f in "$SRC"/libs/vkd3d-common/blob.c "$SRC"/libs/vkd3d-common/debug.c "$SRC"/libs/vkd3d-common/error.c \
         "$SRC"/libs/vkd3d-common/memory.c "$SRC"/libs/vkd3d-common/utf8.c; do
    o=$B/obj/common-$(basename "$f" .c).o
    gcc $CFLAGS $INC -c "$f" -o "$o"
    objs="$objs $o"
done
for f in "$SRC"/libs/vkd3d-shader/*.c "$B"/gen/preproc.yy.c "$B"/gen/preproc.tab.c "$B"/gen/hlsl.yy.c "$B"/gen/hlsl.tab.c; do
    o=$B/obj/shader-$(basename "$f" .c).o
    gcc $CFLAGS $INC -DLIBVKD3D_SHADER_SOURCE -DVKD3D_DEBUG_ENV=VKD3D_SHADER_DEBUG -c "$f" -o "$o"
    objs="$objs $o"
done
gcc $CFLAGS $INC -c "$SRC/programs/vkd3d-compiler/main.c" -o "$B/obj/main.o"
gcc -o "$B/vkd3d-compiler" "$B/obj/main.o" $objs -lm -lpthread -ldl
"$B/vkd3d-compiler" --version
