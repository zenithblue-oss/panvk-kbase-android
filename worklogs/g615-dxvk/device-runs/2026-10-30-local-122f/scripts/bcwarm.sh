#!/bin/bash
# usage: bcwarm.sh <outfile> <rounds> <so...>: per .so on the TB336FU: each test cold (cache cleared) then warm (cache kept).
S=HNY07QHT; L=/var/tmp/panvk/tb336fu.lock; OUT=$1; R=$2; shift 2
for r in $(seq 1 $R); do
  for so in "$@"; do
    for t in ${TESTS:-bc_decode workload_perf}; do
      CLEAR=1 bash /var/tmp/panvk/b122f/imp.sh $S $L /data/local/tmp/p122f/$so $t $OUT "$ENV"
      CLEAR=0 bash /var/tmp/panvk/b122f/imp.sh $S $L /data/local/tmp/p122f/$so $t $OUT "$ENV"
    done
  done
done
echo DONE >> $OUT.done
