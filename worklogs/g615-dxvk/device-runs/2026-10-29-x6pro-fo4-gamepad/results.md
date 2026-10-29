# Fallout 4: Xbox gamepad not seen (X6 Pro, v11, 2026-10-29)

Device: Poco X6 Pro Y5WWBMJVOZSK4HU8 (Mali-G615, v11). PanPlay debug build from main + this fix,
driver 121b (`/var/tmp/panvk/build-out/2026-10-29-121b/libvulkan_panfrost.so`), installed with `adb install -r`.
Game: `/storage/emulated/0/Download/1DM/Compressed/Fallout4.exe` (Steam build with Goldberg `steam_api64.dll`,
no `Galaxy64.dll`), shortcut "Fallout" (`gce0d4e3d`), controller config set to the default "Gamepad" preset.

## Root cause

`ContainerManager.runExe` (Containers.kt) appended `services.exe=d` to `WINEDLLOVERRIDES` for every `Fallout4.exe` /
`Fallout4Launcher.exe` launch. That was a workaround for the GOG build: its `Galaxy64.dll` calls `OpenSCManagerW`,
which crashes in the arm64ec `rpcrt4` (`worklogs/driver-remaining/fo4-hang-before-driver.md`).

With services.exe disabled, Wine never starts `winedevice.exe`, so `winebus.sys` and `winexinput.sys` never load.
The padshim waits for `winebus.so`, so the virtual Xbox 360 pad is never created, and XInput (`xinput1_3`) reports
no controller. Evidence from the FO4 runs before the fix (`files/logs/run-20261029-185446.log`, `-191151.log`):
`err:wineboot:start_services_process Couldn't start services.exe: error 126`, and no `padshim` line.

The pad path itself was fine: `xinput_probe.exe` (tests/xinput_probe.c, x86_64) in the same prefix with the
default config saw the pad on `xinput1_4` and got the A press (`xinput-probe-run-excerpt.txt`):
`connected 0`, `pad0 pkt=1 btn=0x1000`.

Ruled out: `bGamepadEnable=1` was already set in the prefix `Fallout4Prefs.ini`, so no ini change was needed. No
physical pad was connected (`dumpsys input`).

## Fix

`Containers.kt`: add `services.exe=d` only when the game folder has `Galaxy64.dll` (the GOG build that crashes).
Other Fallout 4 builds keep Wine services, so winebus and the XInput pad work. The general XInput path is unchanged,
because it already worked for every game that keeps services.

## Verification (on device)

FO4 run log after the fix (`fo4-run-excerpt.txt`): no services error, and
`padshim: virtual joystick attached idx=0 inst=0 gamecontroller=1`. The game ran with services on, with no
OpenSCManager crash.

- `t1-90.jpg`: 90 s after launch, character creation. The prompts show Xbox glyphs (LT/RT, A FACE, X EXTRAS,
  Y, B BODY, Back DONE), so FO4 sees an XInput controller.
- `t2-0-before.jpg`, `t2-1-after-padB.jpg`, `w2.jpg`: gameplay in the house. The overlay sticks are deflected, and
  the camera and position change between shots. During this time someone was using the on-screen overlay on the
  device (`getevent`: 610 touch events in 6 s), so these shots show overlay pad input moving the player. My own
  `input gamepad keyevent --longpress KEYCODE_BUTTON_B` fell in the same window, so the before/after pair cannot
  be credited to that press alone. No more input was injected, so the live session was not disturbed.

## Limits

- GOG Fallout 4 (with `Galaxy64.dll`) still gets `services.exe=d` and therefore has no XInput pad until the arm64ec
  `rpcrt4` OpenSCManager crash is fixed in Wine/Proton. Use a keyboard config there.
- The main menu was not captured: the game went straight into a running session. The pad press there was not
  isolated from the live overlay input.
- Commit 57e40c74 adds a Fallout 4 keyboard preset with `"exe": ["Fallout4.exe"]`, so a new FO4 shortcut on
  "Automatic" now gets the keyboard preset, not the Gamepad one. The pad then does nothing, by design. Choose
  "Gamepad (template)" in the in-game menu or Edit game. Shortcut `gce0d4e3d` on the X6 Pro was left on
  `preset:default.json` (Gamepad), as the user asked.
- Very short `adb shell input` key presses (down/up within ~1 ms) can fall between polls: only 1 of 3 quick A taps
  reached the probe. Real presses and `--longpress` are long enough.
