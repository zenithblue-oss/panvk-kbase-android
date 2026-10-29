#!/bin/bash
# usage: oom2-batch.sh <apk> <outbase> <n> <tests> <env>
APK=$1; O=$2; N=$3; T=$4; E=$5
for i in $(seq 1 $N); do
  D=$O-$i
  timeout 2400 bash /var/tmp/panvk/oom2-run.sh "$T" "$E" $D $APK
  echo "$i $(grep -h 'DRIVER' $D/autorun.txt | head -1 | grep -o 'git-[0-9a-f]*') $(grep -h 'AUTORUN DONE' $D/autorun.txt) $(grep -hc 'Queue group to be terminated\|Trigger GPU reset' $D/dmesg.txt)" >> $O.summary
done
