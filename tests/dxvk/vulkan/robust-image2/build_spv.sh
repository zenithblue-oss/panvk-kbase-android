#!/bin/sh
# Regenerate robust_image2_spv.h from GLSL shaders (glslangValidator).
set -e
cd "$(dirname "$0")"
T=$(mktemp -d)
G="glslangValidator -V --target-env vulkan1.3"

$G -DR32UI --vn ri2_r32ui_spv -o "$T/ri2_r32ui.h" ri2.comp >/dev/null
$G -DRGBA8 --vn ri2_rgba8_spv -o "$T/ri2_rgba8.h" ri2.comp >/dev/null
$G -DR32UI -DNOFMT --vn ri2_r32ui_nofmt_spv -o "$T/ri2_r32ui_nofmt.h" ri2.comp >/dev/null
$G -DRGBA8 -DNOFMT --vn ri2_rgba8_nofmt_spv -o "$T/ri2_rgba8_nofmt.h" ri2.comp >/dev/null
$G --vn ri2_bench_spv -o "$T/ri2_bench.h" ri2_bench.comp >/dev/null

cat "$T"/ri2_r32ui.h "$T"/ri2_rgba8.h "$T"/ri2_r32ui_nofmt.h "$T"/ri2_rgba8_nofmt.h "$T"/ri2_bench.h |
   sed 's/^const uint32_t/static const uint32_t/' > robust_image2_spv.h
rm -rf "$T"
