# RTL SM-X926B v12 build 120 - NOT RUN, device unresponsive

2026-10-29 15:48-15:51 IST. `timeout 20 adb devices -l` against localhost:35395 timed out (rc=124, no output) on 3 tries, 60 s apart. The rdb listener (port 35395) and ./rdb process are up, but the adb server does not answer even `adb devices`, so no device serial and no shell check was possible. No adb connect/disconnect/kill-server, no sysfs read. Nothing installed.

APKs found: /var/tmp/panvk/build-out/apks/panprobe-120.apk, panplay-120.apk (pin match not checked, device never reached).

No wedge logs survived from build 119 scenario 8 (nothing pulled). Needs a fresh RTL session.
