#!/bin/bash
# usage: bcab.sh <outfile> <rounds> <so...>: workload_perf, bc_decode, bc_perf per .so on the TB336FU (imported slot), alternating.
S=HNY07QHT; L=/var/tmp/panvk/tb336fu.lock; OUT=$1; R=$2; shift 2
for r in $(seq 1 $R); do
  for so in "$@"; do
    for t in ${TESTS:-workload_perf bc_decode bc_perf}; do
      CLEAR=1 bash /var/tmp/panvk/b122f/imp.sh $S $L /data/local/tmp/p122f/$so $t $OUT "$ENV"
    done
  done
done
echo DONE >> $OUT.done
