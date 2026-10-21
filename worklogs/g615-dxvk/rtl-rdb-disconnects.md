# RTL / RDB adb disconnects: analysis (research only, nothing restarted)

Date: 2026-10-21. Device: SM-A065F via Samsung RTL, RDB at /opt/samsung-rdb/current/bin.js.

## 1. How the tunnel works (from bin.js)
- bin.js is byte-identical to bin.js.orig right now (cmp). bin.js.loopback is the loopback-only variant; the
  rdb log shows the loopback variant ran until 05:37, the original (all interfaces, no "listening on 127.0.0.1" lines) since 05:53.
  The protocol is the same in both.
- Two servers: WebSocket :8888 (RTL web page) and a TCP server on a random port N (30000-59999). No framing is
  added: every TCP chunk from adb becomes one ws message (`websocket.send(chunk)`), and every ws message from the
  page is written verbatim to the adb socket (`adbsocket.write(data)`). ws message boundaries therefore are not
  adb packet boundaries; the RTL side must reassemble the stream. The RTL side is effectively a remote
  USB/adbd transport.
- The very first ws message is JSON ({name: ...}) used only for the log line "Connected device: X"; the same code
  path also writes it to adbsocket if adb is already connected (harmless only if adb is not yet connected).
- adb auth is NOT handled by rdb. adb host speaks CNXN/AUTH end-to-end with the phone's adbd through the pipe:
  adbd sends AUTH TOKEN, adb replies send_auth_response (signature with ~/.android/adbkey), and if adbd does not
  know the key, send_auth_publickey (needs on-device "Allow USB debugging" accept). Auth is per transport/connection,
  so every new connection repeats it. The RSA key is stable (fingerprint 1512F0A7...; adbkey exists, only
  generated once at 23:54 on Oct 20).
- State is global, not per connection: single `websocket` and `adbsocket` variables. A new web client overwrites
  `websocket`; and each ws 'close' runs `adb disconnect localhost:N` for the shared port, even if a newer
  ws session is live. A stale/old tab closing kills the new session's transport.
- "Disconnected device: X" is printed only by ws.on('close') (web page/tab closed, RTL session dropped, or RTL
  server closed the socket), and it immediately runs `adb disconnect`. "ADB disconnected from RDB" is the adb TCP
  socket 'end' (adb server closed it: after `adb disconnect`, transport kick, or adb server crash).
  rdb itself never initiates a drop and has no keepalive/ping.

## 2. Evidence in the logs
- /tmp/rdb.log: pattern "Web client connected -> Connected device -> ADB connected -> Disconnected device ->
  ADB disconnected". The ws closes (from the RTL side or page), then rdb calls adb disconnect. Several cycles also
  show "ADB disconnected" with no "Disconnected device" (adb side dropped while ws stayed up, then adb reconnects:
  "ADB connected" again).
- /tmp/adb.1000.log: transports go "offline" at send_auth_response / send_auth_publickey, then "kicking transport"
  (= `adb disconnect` from rdb, or reconnect replacing it) 40-90 s later, repeated. Many are "read failed: Success"
  = EOF from the ws tunnel (peer closed), not an adb error.
- Crash: `08:35:29 F fdevent_epoll.cpp:138 epoll_wait failed: Function not implemented` (pid 28362), 2 min after
  first transport; server died, next command restarted it (pid 28987). One occurrence in the log; netlink error -19
  (ENODEV) at startup is PRoot noise and harmless.
- User observation: device dropped right after `adb shell svc power stayon true` in 3/3 sessions, while getprop,
  pm path and a 35 MB install were fine.

## 3. Root causes (ranked)
1. RTL-side session drop triggered by specific shell commands (most likely for the stayon case). `svc power stayon true`
   changes a global setting / power state, and RTL's page states terminal activity is monitored and the
   connection may drop when a command is blocked. RTL devices are shared, reset per session and have a
   controlled environment; the RTL proxy appears to close the ws, which yields "error: closed" then
   "Disconnected device". Evidence is the 3/3 correlation with that one command while heavy commands work; the
   tunnel bytes are untouched by rdb. (The command also can restart/reconfigure adbd on some Samsung builds, which
   would produce the same symptom, but either way it is a command-triggered drop.) Not proven which one; the
   differentiator is whether the page shows a session message or the device reappears after a few seconds.
2. Idle/unauthenticated flapping (the ~40 s cycles): transports stuck at offline after auth (key not accepted
   on RTL device, or the web client re-opening the session); then multiple web clients/tabs racing on the global
   websocket/adbsocket variables, and `adb disconnect` from an old ws close killing the live transport. Samsung
   forum threads report the same connect/disconnect cycle on RDB for several devices (Android 14 included) after RTL
   updates, i.e. partly server-side and out of our control.
3. adb server crash: adb 35.0.2 uses an epoll fdevent backend only (fdevent_epoll.cpp); PRoot sometimes returns ENOSYS
   for epoll_wait/epoll_pwait while ptrace-intercepting, and adb treats it as fatal (LOG(FATAL)). One crash seen. When
   the server dies, all transports go "closed" and RDB sees "ADB disconnected". It does not explain the 3/3
   stayon drops (no crash in those windows) but contributes to dropouts.
   Web search found no documented env var: current adb has no poll backend, and ADB_LIBUSB / ADB_MDNS_* affect USB
   and mDNS only (they reduce, not remove, fd churn, and there is no USB here).

## 4. Fixes without touching anything running
Commands to avoid on RTL (use alternatives):
- `svc power stayon *`, `settings put global stay_on_while_plugged_in`, `svc usb`, `setprop persist.*`,
  `setprop sys.usb.*`, `svc wifi/data`, `reboot`, `adb root/usb/tcpip`, `settings put global adb_enabled|development_settings_enabled`.
- Instead, keep the screen awake with `input keyevent KEYCODE_WAKEUP` and `adb shell input keyevent 224` before tests,
  or hold a wake lock from the app being tested (FLAG_KEEP_SCREEN_ON); `settings put system screen_off_timeout 1800000`
  is a lighter change but should also be tried only once and treated as suspect.
- Prefer one short command per adb call; avoid streaming install of big files (--no-streaming already used, keep it).
- Bisect safely: after each new risky command check `adb devices` and the RTL page for a message.

Proposed (NOT applied) ~/.local/bin/adb wrapper for the next adb server start:
```sh
#!/bin/sh
export ADB_MDNS=0 ADB_MDNS_OPENSCREEN=0 ADB_LIBUSB=0   # drop mDNS and USB threads (fewer fds / netlink noise)
export ADB_TRACE=            # leave empty; logging stays in /tmp/adb.1000.log
# keep the server alive: restart on crash is already implicit (next client call spawns it)
exec /opt/android-sdk/platform-tools/adb "$@"
```
Expect only a small effect on the epoll crash. Real options for it: a retry wrapper (`for i in 1 2 3; do adb "$@" && break; adb start-server; done`
for idempotent commands only), or an older/different adb build with a poll-based fdevent (e.g. Alpine/Debian
android-tools build or platform-tools r30-style) to test, or run adb with `proot --kill-on-exit` off/`-L` variants
(untested). No env var found that switches 35.0.2 to poll.

Keepalive / session settings:
- adb host: `ADB_TRACE=` none needed; no real adb keepalive exists on TCP transports. Run a lightweight
  heartbeat from the host, `while :; do adb -s localhost:N shell true; sleep 20; done` (only if sessions idle-drop;
  not applied; note it may itself race with disconnects).
- rdb: keep a single RTL web tab per device; do not reconnect the page while a transport is active (global state race).
  Use the patched bin.js.loopback only after adding per-connection state if desired (e.g. close the previous ws and keep
  the adbsocket tied to its ws; add ws ping every 20 s to prevent idle timeouts).
- Optional rdb patch idea: ws.ping() timer + only run `adb disconnect` if `ws === websocket`.

## Sources
- Samsung forum RDB threads: https://forum.developer.samsung.com/t/remote-test-lab-rdb-problem/24907 ,
  https://forum.developer.samsung.com/t/remotetestlab-android-14-deivce-remotedebug-bridge-not-working/27622
- Samsung RTL troubleshooting: https://developer.samsung.com/sdp/blog/en/2022/08/23/troubleshooting-common-issues-while-using-the-remote-test-lab-service
- epoll ENOSYS under sandboxes (analogous): https://gitlab.torproject.org/legacy/trac/-/issues/23449
- No source found for the exact RTL "Terminal activity is monitored" text or an adb poll env var.
