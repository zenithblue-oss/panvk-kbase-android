#!/bin/sh
# Regenerate tess-perf-spv.h from the tess-perf.* GLSL sources (glslangValidator).
set -e
cd "$(dirname "$0")"
T=$(mktemp -d)
G="glslangValidator -V --target-env vulkan1.1"
$G --vn tp_vs -o "$T/a.h" tess-perf.vert >/dev/null
$G -DREF --vn tp_vs_ref -o "$T/b.h" tess-perf.vert >/dev/null
$G --vn tp_tcs -o "$T/c.h" tess-perf.tesc >/dev/null
$G --vn tp_tes -o "$T/d.h" tess-perf.tese >/dev/null
$G --vn tp_gs -o "$T/e.h" tess-perf.geom >/dev/null
$G --vn tp_fs -o "$T/f.h" tess-perf.frag >/dev/null
cat "$T"/*.h | sed 's/^const uint32_t/static const uint32_t/' > tess-perf-spv.h
rm -rf "$T"
