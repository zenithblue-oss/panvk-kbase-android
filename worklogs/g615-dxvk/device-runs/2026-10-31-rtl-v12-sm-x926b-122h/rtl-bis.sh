#!/bin/bash
# after rtl-h2: tex_rgba bisect, 3 runs per .so, hard stop 09:40
W=/home/abhaybyte/repos/panvk/worklogs/g615-dxvk/device-runs/2026-10-31-rtl-v12-sm-x926b-122h
while pgrep -f rtl-h2.sh >/dev/null; do sleep 3; done
left=$(( $(date -d 09:40 +%s) - $(date +%s) ))
[ $left -gt 150 ] && N=3 timeout $left bash /var/tmp/panvk/b122/bis.sh 2026-10-29-116 2026-10-29-120 2026-10-30-122e 2026-10-29-121b 2026-10-29-118
timeout 20 adb -s localhost:50909 shell run-as dev.zenithblue.panvktest rm -rf files/imported
timeout 20 adb -s localhost:50909 shell rm -f /data/local/tmp/panvk-bis.so
timeout 20 adb -s localhost:50909 shell am force-stop dev.zenithblue.panvktest
echo "$(date +%T) bisect wrapper end, cleanup done" >> $W/progress.txt
