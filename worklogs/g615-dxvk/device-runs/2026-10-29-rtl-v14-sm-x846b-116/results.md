# RTL SM-X846B (Mali-G1-Ultra, v14) - release gate build 116 (2026-10-29)

Driver git-f7e78953ac (build 116), panprobe-116.apk / panplay-116.apk. Device online 11:53 IST, session limit ~12:50.
Scripts: /var/tmp/panvk/run-116-probe.sh, run-116-rtl.sh (under rtl.lock).

## PanProbe autorun `all` (11:55-11:58 IST): 40/40 PASS
All 40 tests PASS, incl. tessellation (mismatch=0, 3520.9 fps), tess_perf, submit_stress, csf_event, swapchain_lifecycle, sparse_kbase (pass=12 fail=0 skip=2).
Compliance: DXVK PASS (94), Bachata S4 PASS (114), vkd3d PASS (100), D3D FL PASS (20); only soft misses (FL12_x as before).
Logcat grep (kbase/mali/panvk/csf/fault/DEVICE_LOST/timeout snapshot): 0 faults, 0 timeout snapshots, 0 DEVICE_LOST.
Raw: raw/probe-all/ (autorun.txt, logcat-grep.txt).

(PanPlay and 0147 A/B sections appended below.)
