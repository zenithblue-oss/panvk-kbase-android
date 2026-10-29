# RTL: SM-X846B (Mali-G1-Ultra, v14), build 115e PanPlay: NOT RUN (device transport lost)

- `panplay-115e.apk` installed OK (417 MB over the RDB tunnel took about 6 min). Scenario 1 was launched
  (`TEST_LOOP --ei scenario 1`, override warm-up 10 s / measure 30 s); first frame `CUBE: d3d11 Present frame=0 hr=0x0` at 7 s,
  screenshot at 50 s. No results collected: right after that the adb transport to `localhost:35395` went `offline`
  (rdb process alive and listening, `rdb.log` flapping "ADB connected/disconnected"); `adb disconnect/connect` did not bring it
  back for 4+ min. Scenarios 2-9 never ran; no frame-pacing capture (requested by the coordinator) could be taken.
- Nothing to conclude for PanPlay on v14. PanProbe result is in `../2026-10-29-rtl-v14-sm-x846b-115e-panprobe/results.md`.
- To resume on a new RTL session: install `panplay-115e.apk`, push `gameloop-override-115e.json` (warm-up 10, measure 30, shot 20)
  to `/sdcard/Download/`, run scenarios 3 first; per-frame data is in `<run>/01-d3d11-default/perf.csv` and `perf.json`
  (frametimeP50/P95/P99, low1Fps), `panvk-fps-*.log`.
