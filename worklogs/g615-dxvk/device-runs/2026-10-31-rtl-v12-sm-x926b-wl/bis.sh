#!/bin/bash
# usage: bis.sh <build dir name>... ; PanProbe imported slot, workload_perf, APK already installed (122g)
S=localhost:50909; L=/var/tmp/panvk/rtl.lock; P=dev.zenithblue.panvktest
W=/home/abhaybyte/repos/panvk/worklogs/g615-dxvk/device-runs/2026-10-31-rtl-v12-sm-x926b-wl
O=/var/tmp/panvk/rtl-wl; p(){ echo "$(date +%T) $*" | tee -a $W/progress.txt; }
for b in "$@"; do
  so=/var/tmp/panvk/build-out/$b/libvulkan_panfrost.so
  timeout 120 adb -s $S shell "run-as $P sh -c 'mkdir -p files/imported && cat > files/imported/libimported.so'" < $so
  sz=$(timeout 10 adb -s $S shell run-as $P stat -c %s files/imported/libimported.so | tr -d '\r')
  [ "$sz" = "$(stat -c %s $so)" ] || { p "bis $b push size mismatch $sz"; continue; }
  AR=workload_perf timeout 300 bash /var/tmp/panvk/b122/probe-wl.sh $S $L - imported $O/bis-$b /dev/null >/dev/null
  cp $O/bis-$b/raw/autorun.txt $W/bis-$b.txt
  p "bis $b tex_rgba=$(grep -a -o 'case=tex_rgba ms=[0-9.]* us_op=[0-9.]*' $W/bis-$b.txt | grep -o 'us_op=.*') $(head -1 $O/bis-$b/summary.txt)"
done
timeout 20 adb -s $S shell run-as $P rm -rf files/imported
p "bis DONE $*"
