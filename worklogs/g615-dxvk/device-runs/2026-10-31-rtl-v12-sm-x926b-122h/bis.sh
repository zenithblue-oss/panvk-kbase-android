#!/bin/bash
# usage: N=3 bis.sh <build dir name>... ; PanProbe imported slot, workload_perf, PanProbe APK already installed
S=localhost:50909; L=/var/tmp/panvk/rtl.lock; P=dev.zenithblue.panvktest; N=${N:-3}
W=/home/abhaybyte/repos/panvk/worklogs/g615-dxvk/device-runs/2026-10-31-rtl-v12-sm-x926b-122h
O=/var/tmp/panvk/rtl-h; p(){ echo "$(date +%T) $*" | tee -a $W/progress.txt; }
T=/data/local/tmp/panvk-bis.so
for b in "$@"; do
  so=/var/tmp/panvk/build-out/$b/libvulkan_panfrost.so
  timeout 120 adb -s $S push $so $T >/dev/null 2>&1
  timeout 30 adb -s $S shell "run-as $P sh -c 'mkdir -p files/imported && cp $T files/imported/libimported.so'" 2>/dev/null
  sz=$(timeout 10 adb -s $S shell run-as $P stat -c %s files/imported/libimported.so | tr -d '\r')
  if [ "$sz" != "$(stat -c %s $so)" ]; then
    timeout 120 adb -s $S shell "run-as $P sh -c 'mkdir -p files/imported && cat > files/imported/libimported.so'" < $so
    sz=$(timeout 10 adb -s $S shell run-as $P stat -c %s files/imported/libimported.so | tr -d '\r')
  fi
  [ "$sz" = "$(stat -c %s $so)" ] || { p "bis $b push size mismatch $sz"; continue; }
  v=""
  for i in $(seq 1 $N); do
    AR=workload_perf timeout 300 bash /var/tmp/panvk/b122/probe-wl.sh $S $L - imported $O/bis-$b-$i /dev/null >/dev/null
    cp $O/bis-$b-$i/raw/autorun.txt $W/bis-$b-$i.txt
    v="$v $(grep -a -o 'case=tex_rgba ms=[0-9.]* us_op=[0-9.]*' $W/bis-$b-$i.txt | grep -o 'us_op=.*' | cut -d= -f2)"
  done
  p "bis $b tex_rgba us_op:$v $(grep -m1 -a -o 'driverInfo=[^,]*,[^,]*' $W/bis-$b-1.txt)"
done
timeout 20 adb -s $S shell run-as $P rm -rf files/imported; timeout 20 adb -s $S shell rm -f $T
p "bis DONE $*"
