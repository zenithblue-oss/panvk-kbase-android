# Patched Samsung RTL Remote Debug Bridge (rdb)

`bin.js.fixed` is a patched copy of Samsung's `bin.js` (from rdb.zip, ARM64 launcher extracted from the pkg payload).
`bin.js.patch` is the unified diff against Samsung's original `bin.js.orig` (the original is not redistributed here).

Changes:
- Per-connection state: a ws `close` only runs `adb disconnect` when the closing ws is the current session
  (a stale tab closing no longer kills the live transport). Data from stale ws is not written to adb.
- ws ping every 20 s to keep the tunnel alive.
- Env knobs: `RDB_WS_PORT` (default 8888), `RDB_TCP_PORT` (default random 30000-59999),
  `RDB_LOOPBACK=1` (listen on 127.0.0.1 only; default is all interfaces as in the original),
  `RDB_NO_ADB=1` (test mode: log instead of calling adb).

Install: copy to `/opt/samsung-rdb/current/bin.js.fixed` and run `node bin.js.fixed` (the `rdb-daemon` script prefers it).
Test: `RDB_WS_PORT=18888 RDB_TCP_PORT=18889 RDB_NO_ADB=1 RDB_LOOPBACK=1 timeout 20 node bin.js.fixed`, then connect a
ws client and a TCP peer; bytes flow both ways and closing an older ws logs "Stale ws closed" with no disconnect.
