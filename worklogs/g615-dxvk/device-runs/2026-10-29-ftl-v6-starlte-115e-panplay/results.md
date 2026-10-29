# FTL starlte (Galaxy S9 SM-G960F, Mali-G72 MP18, v6 JM, Android 10) build 115e: PanPlay scenarios 1-9

- Matrix `matrix-2sigec4tjxbed`, project `phone-finder-hub`, results
  `gs://test-lab-7jybm598jy36m-hdr2td6as0u1q/panvk-115d-starlte-play/` (the results dir name says 115d; the APK is 115e);
  FTL outcome `Passed starlte-29-en-portrait`, test time 2700 s (the 45 min limit).
- APK `panplay-115e.apk` (417 MB, full bundle), `dev.zenithblue.panvklauncher` 1.3.0 (vc 44).
  An earlier launch in `finalbenchmark-665a8` failed at validation with TEST_QUOTA_EXCEEDED (no device time used).
  `gameloop-override.json`: warm-up 10 s, measure 30 s. `raw/` has everything but `video.mp4`.
- An earlier 115d launch was not made for starlte; this is the first starlte PanPlay run of the day.

## Result: no scenario ran, the Wine prefix cannot be created

Only scenarios 1 and 2 produced results; each spent 5 min in `wine wineboot -u` and failed; the rest of the 45 min
went the same way (no results files for 3-9).

```
[17:13:09] setting up the Wine prefix (wineboot -u)
PanPlayGameLoop: setup: $ /data/data/dev.zenithblue.panvklauncher/files/contents/Proton/11.0-2-arm64ec/lib/wine/aarch64-unix/wine wineboot -u
[17:18:20] game loop error: java.lang.IllegalStateException: container setup failed: wine wineboot -u failed (exitCode=53, system32=false, systemReg=false)
```

Wine side (logcat `PanPlayGameLoop`):

```
PanPlayGameLoop: setup: wine: could not load kernel32.dll, status c0000135
```

SELinux (the same denial for `wine`, `wineboot.exe` and `start.exe`, on `drive_c/windows`, `windows/system`, `windows/system32`,
`Program Files (x86)` and `container`):

```
E audit : type=1400 audit(1793232800.135:90): avc:  denied  { ioctl } for  pid=16793 comm="wine" path="/data/data/dev.zenithblue.panvklauncher/files/container/.wine/drive_c/windows/system32" dev="dm-3" ino=852372 ioctlcmd=0x7201 scontext=u:r:untrusted_app_27:s0:c5,c257,c512,c768 tcontext=u:object_r:app_data_file:s0:c5,c257,c512,c768 tclass=dir permissive=0
```

Both scenarios show it (scenario 1 at 17:13:20 and 17:18:20, scenario 2 at 17:18:31 and 17:23:31); `untrusted_app_27` (targetSdk 27 domain),
Samsung SELinux build `SEPF_SM-G960F_10_0030`. `ioctlcmd=0x7201` is `FIGETBSZ`-class (`'r'` ioctl range, `0x72`), a directory ioctl Wine
issues while opening the prefix directories; the prefix never gets `system32` or `system.reg` (`system32=false, systemReg=false`).

No driver code ran: no fps, no DEVICE_LOST, no `timeout on subqueue`, no GPU fault in logcat. So there is no 115e result for
starlte D3D11/D3D12 (FL 11_1 on v6) from this run; needs a fix for the dir ioctl denial (for example a Wine build that
tolerates EACCES on that ioctl, or avoiding the ioctl) and a rerun.

## Screenshots

- `screenshots/video-*.png`: FTL video frames.
