#!/bin/sh
# Regenerate workload-perf-spv.h from workload-perf.glsl (glslangValidator).
set -e
cd "$(dirname "$0")"
T=$(mktemp -d)
G="glslangValidator -V --target-env vulkan1.1"
n=0
s() { n=$((n+1)); $G -S "$1" -D"$2" --vn "$3" -o "$T/$(printf %02d $n).h" workload-perf.glsl >/dev/null; }
s vert VS wp_vs
s vert VS_XFB wp_vs_xfb
s vert VS_FULL wp_vs_full
s geom GS wp_gs
s geom GS_LAYER wp_gs_layer
s geom GS_XFB wp_gs_xfb
s frag FS wp_fs
s frag FS_TEX wp_fs_tex
cat "$T"/*.h | sed 's/^const uint32_t/static const uint32_t/' > workload-perf-spv.h
rm -rf "$T"
