#!/bin/bash
# per run dir: pass/fail, bc_decode ms, workload cases gs_pass/strip_gs us_op, bc_level ms, sum ms
for d in "$@"; do f=$d/raw/autorun.txt; [ -f $f ] || { echo "$d: none"; continue; }
 sum=$(grep -o 'RESULT [a-z0-9_]* [A-Z]* .*' $f | grep -o ' ms=[0-9]*' | head -41 | awk -F= '{s+=$2} END{print s}')
 wl=$(grep -m1 'RESULT workload_perf' $f | tr '|' '\n' | grep -E 'case=(gs_pass|strip_gs|bc_level) ' | sed -E 's/.*case=([a-z_]+) ms=([0-9.]+) us_op=([0-9.]+).*/\1 ms=\2 us=\3/' | tr '\n' ';')
 echo "$(basename $d): $(cat $d/summary.txt 2>/dev/null | sed 's/drv=driverInfo=//') bc_decode=$(grep -o 'RESULT bc_decode [A-Z]*.* ms=[0-9]*' $f | grep -o 'ms=[0-9]*$') geometry=$(grep -o 'RESULT geometry [A-Z]*' $f | cut -d' ' -f3) sum=$sum $wl"
 grep -o 'RESULT [a-z0-9_]* FAIL' $f
done
