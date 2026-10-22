#!/bin/sh
# Valhall (v11, G615) shader-db stats of panvk_bc_etc2.comp per transcode mode.
#   sh tools/spvstat/all.sh file.spv [opts]
D=$(cd "$(dirname "$0")" && pwd)
for m in 0 1 2 3 4 5 6 7; do
  printf "mode %s: " $m
  BIFROST_MESA_DEBUG=shaderdb "$D/spvstat" "$1" 11 $m ${2:-0x30000} 2>&1 |
    sed -n 's/.*shader: \([0-9]*\) instrs, \([0-9.]*\) cycles, \([0-9.]*\) fma, \([0-9.]*\) cvt, \([0-9.]*\) sfu.*, \([0-9.]*\) ls,.* \([0-9]*:[0-9]*\) spills:fills.* \([0-9]*\) registers used.*/instrs=\1 cyc=\2 fma=\3 cvt=\4 sfu=\5 ls=\6 spills:fills=\7 regs=\8/p; s/.*compile_ms=/ compile_ms=/p' | tr '\n' ' '
  echo
done
