#!/bin/bash
# usage: cmp.sh <rundir>...: summary, sum of test ms, and workload_perf cases (us_op) per run.
for d in "$@"; do f=$d/raw/autorun.txt; [ -f $f ] || { echo "$d: none"; continue; }
 sum=$(grep -o 'RESULT [a-z0-9_]* [A-Z]* .*' $f | grep -o ' ms=[0-9]*' | awk -F= '{s+=$2} END{print s}')
 wl=$(grep -m1 'RESULT workload_perf' $f | tr '|' '\n' | sed -nE 's/.*case=([a-z_0-9]+) ms=([0-9.]+) us_op=([0-9.]+).*/\1=\3/p' | tr '\n' ' ')
 echo "$(basename $d): $(sed 's/drv=driverInfo=Mesa 26.3.0-devel //' $d/summary.txt) sum_ms=$sum"
 echo "   $wl"
 grep -o 'RESULT [a-z0-9_]* FAIL' $f
 mx=$(grep -o "TOTAL PSS: [0-9]*" $d/mem.txt 2>/dev/null | grep -o "[0-9]*" | sort -n | tail -1); gx=$(grep -o "Graphics: [0-9]*" $d/mem.txt 2>/dev/null | grep -o "[0-9]*" | sort -n | tail -1)
 echo "   pss_max_kb=$mx graphics_max_kb=$gx"
done
