#!/bin/sh
# bcperf-run.sh <tag> [extra env, e.g. "PANVK_DEBUG=bc_compute"] [bc-perf filter]
# Pushes dist-<tag> ICD, runs bc_decode + verify + bc-perf on G615 under device lock.
T="$1"; ENVS="$2"; FILT="$3"
export ANDROID_SERIAL=192.168.1.34:35419
B=/var/tmp/panvk/bcperf-bin; O=/var/tmp/panvk/res-$T${4:+-$4}; D=/data/local/tmp/bcperf
mkdir -p $O
exec 9>>/var/tmp/panvk/device.lock
flock -w 1800 9 || { echo LOCK_TIMEOUT; exit 1; }
adb shell "dumpsys activity top | grep -E '^  ACTIVITY' | head -3" > $O/top.txt
adb shell mkdir -p $D/$T $D/dump-$T
adb push /var/tmp/panvk/dist-$T/libvulkan_panfrost.so $D/$T/ >/dev/null
adb push $B/bc-perf $B/bc_decode $D/ >/dev/null
adb shell "cd $D && chmod 755 bc-perf bc_decode && rm -f dump-$T/* && env $ENVS ./bc_decode $D/$T/libvulkan_panfrost.so dump-$T" > $O/bcdec.txt 2>&1
rm -rf $O/dump; adb pull $D/dump-$T $O/dump >/dev/null
python3 /home/abhaybyte/repos/panvk/.claude/worktrees/agent-a3c08a5bb85d0d417/tests/dxvk/vulkan/bc/bc_verify.py $O/dump $B/bc_ref > $O/verify.txt 2>&1
if [ "$FILT" != "none" ]; then
adb shell "cd $D && env $ENVS ./bc-perf $D/$T/libvulkan_panfrost.so $FILT" > $O/perf.txt 2>&1
fi
grep -E 'FAIL|BC_DEVICE' $O/bcdec.txt | head; grep -E 'FAIL|BC_VERIFY|BC6' $O/verify.txt
grep -E '^PERF|RESULT|rror|ssert' $O/perf.txt 2>/dev/null | sed 's/upload_wall_ms=[0-9.]* //; s/wallA=.*//'
cat $O/top.txt
