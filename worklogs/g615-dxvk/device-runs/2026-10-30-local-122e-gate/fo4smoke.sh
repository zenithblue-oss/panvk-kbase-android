#!/bin/zsh
# usage: fo4smoke.sh <outdir>: launch the FO4 shortcut as it is (no shortcut or save changes), Continue,
# record 30 s of device output audio, screenshot, pull the run log, kill Fallout4.exe.
function dev { timeout ${T:-60} adb -s 192.168.1.35:42767 "$@"; }
O=$1; mkdir -p $O
exec 9>/var/tmp/panvk/x6pro.lock; flock 9
dev shell 'run-as dev.zenithblue.panvklauncher cat files/shortcuts/gce0d4e3d.json' > $O/shortcut.json
dev shell 'pidof Fallout4.exe' && { echo "FO4 already running, not mine"; exit 1; }
dev shell am start -n dev.zenithblue.panvklauncher/.MainActivity --es dev.zenithblue.panvklauncher.LAUNCH_SHORTCUT gce0d4e3d
sleep 48; dev shell input gamepad keyevent --longpress KEYCODE_BUTTON_A; sleep 6
dev shell input gamepad keyevent --longpress KEYCODE_BUTTON_A; sleep 4
dev shell input gamepad keyevent --longpress KEYCODE_BUTTON_A
sleep 40
timeout 60 scrcpy -s 192.168.1.35:42767 --no-window --no-video --no-playback --audio-source=output --audio-codec=raw --record=$O/audio.wav --time-limit=30 > $O/scrcpy.log 2>&1
dev exec-out screencap -p > $O/fo4.png
python3 /var/tmp/panvk/fo4-audio-face/lev.py $O/audio.wav > $O/levels.txt
L=$(dev shell 'run-as dev.zenithblue.panvklauncher ls -t files/logs' | grep run- | head -1 | tr -d '\r')
dev exec-out run-as dev.zenithblue.panvklauncher cat files/logs/$L > $O/run.log
dev shell 'run-as dev.zenithblue.panvklauncher sh -c "kill \$(pidof Fallout4.exe) 2>/dev/null"'
echo "log=$L libav=$(grep -c gstavcodecmap $O/run.log) wgfail=$(grep -c 'wg_transform_create: Failed' $O/run.log) devlost=$(grep -ci device_lost $O/run.log)"
cat $O/levels.txt
