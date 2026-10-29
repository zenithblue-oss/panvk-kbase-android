#!/bin/bash
# usage: playmem.sh <playdir>: per scenario max TOTAL PSS / Graphics (MB) and driver vram (MiB), GPU load.
for s in 1 2 3 4 5 6 7 8 9; do
  fs=$(ls $1/mem/s$s.txt $1/mem/s$s-*.txt 2>/dev/null); [ -z "$fs" ] && continue
  p=$(cat $fs | grep -o 'TOTAL PSS: *[0-9]*' | grep -o '[0-9]*$' | sort -n | tail -1)
  g=$(cat $fs | grep -o 'Graphics: *[0-9]*' | grep -o '[0-9]*$' | sort -n | tail -1)
  v=$(cat $fs | grep -o 'vram=[0-9]*' | grep -o '[0-9]*$' | sort -n | tail -1)
  l=$(cat $fs | grep -o 'load=[0-9]*' | grep -o '[0-9]*$' | tr '\n' ',')
  echo "s$s pss=$((p/1024)) gfx=$((g/1024)) vram=$(( ${v:-0}/1048576 )) load=$l"
done
