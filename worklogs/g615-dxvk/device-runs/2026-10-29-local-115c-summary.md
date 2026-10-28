# 2026-10-29 local USB confirm of build 115c (PanProbe + PanPlay)

APKs: `panprobe-115c.apk` (1.3.0 vc19), `panplay-115c.apk` (1.3.0 vc44), debug-signed, from `/var/tmp/panvk/build-out/apks/`. Driver `Mesa 26.3.0-devel (git-30c29c0175)` (desktop build of tree e945011a; the phone build says git-15fd70c7c4).

| Device | GPU | Install | PanProbe rc5 (115c) | PanProbe rc4 | PanPlay |
|---|---|---|---|---|---|
| Lenovo TB336FU `HNY07QHT` (MT8755) | Mali-G57 MC2, `0x90930010`, v9 | both OK | 40/40, 206.7 s | 38/40 (tess_perf, large_draw: old rc4 bugs), 304.5 s | s3/5/6/7/8/9 done, see below |
| Poco X6 Pro `Y5WWBMJVOZSK4HU8` (MT6897) | Mali-G615 MC6, `0xb8a31030`, v11 | PanProbe OK; **PanPlay blocked** | 40/40, 168.3 s | 40/40, 145.1 s | not run |

PanPlay blocked on X6 Pro: `INSTALL_FAILED_DUPLICATE_PERMISSION` (installed `dev.zenithblue.panvklauncher` 1.2.9-rc5 vc37 has another signature; install needs an uninstall). Not done per rules. User decision needed.

PanPlay TB336FU (details in `2026-10-29-local-v9-tb336fu-115c-panplay/results.md`): D3D11 153.7, D3D9 90.0, gears 254.9; SquidRoom FAIL (not bundled, expected); D3D12 FL 11_0 and 11_1 PASS (logic_op fixed), 12_0/12_1/12_2 unsupported; D3D11 FL 10_0 to 11_0 PASS, FL 11_1 FAIL on `tir` only (known open); D3D10, D3D9 PASS. No DEVICE_LOST or kbase fault.

Versus 115b: no regression. 115b D3D12 FL 11_1 `logic_op` FAIL (oriole v9) now PASS; DEVICE_LOST on v11 `tir` and `conservative` (patch 0136) not exercised here since PanPlay did not run on v11; PanProbe v11 40/40 incl. d3d_fl.

Notes: X6 Pro logcat is unreadable over adb. X6 Pro PanProbe shows `bc_decode` 15.5 s on 115c vs 0.7 s on rc4 (known bc_decode lowmem regression), `bc_perf` 43 s vs 29 s.
