# Fallout 4 controller key preset, X6 Pro, 2026-10-29

APK: /var/tmp/panvk/build-out/apks/panplay-121b-fo4.apk (SO 2026-10-29-121b), installed with `adb install -r`.
Preset: apps/panvk-launcher/app/src/main/assets/controller-presets/fallout4.json (`exe: Fallout4.exe`, output keyboard).

## Binding table

| Pad input | Key / mouse | FO4 action |
|---|---|---|
| Left stick | W A S D | Move |
| Right stick | mouse | Look |
| A | E | Activate |
| B | Tab | Pip-Boy |
| X | R | Ready / reload (hold = holster, hold unverified on PC) |
| Y | Space | Jump |
| LB | Q | VATS |
| RB | Left Alt | Bash / power attack / grenade (hold) |
| LT | mouse right | Aim / block |
| RT | mouse left | Attack |
| L3 | Left Shift | Sprint |
| R3 | Left Ctrl | Sneak |
| Back | V | Toggle POV |
| Start | Escape | Pause |
| D-pad up | F | Favorites menu |
| D-pad down | Return (Enter) | Confirm in menus, dialogue, Pip-Boy |
| D-pad left | T | Wait |
| D-pad right | G | G (requested; no source) |
| Guide | F5 | Quicksave |

Enter / T / G choice: all 17 pad inputs are used. Every face, shoulder, trigger, stick and Back/Start input maps to a core FO4 pad action, so Enter, T and G go on the D-pad. On the Xbox layout the D-pad only opens favorites (any direction). Up keeps the F menu, which also reaches the number hotkeys 1-0 (so losing direct 1/2 costs least). Return is on down, T on left, G on right.

Not reachable (no spare input): F9 quickload, direct hotkeys 1-0 (use F menu), X toggle always run, C auto move, Caps Lock always run. Not bound, no source: Pip-Boy light, workshop/build keys.

UNVERIFIED: T = Wait and G (user-supplied, not in the pages read), hold R to holster on PC, Pip-Boy light, workshop keys.

## Sources
- https://www.magicgameworld.com/fallout-4-controls-guide-pc-xbox-and-playstation/ (PC + Xbox lists)
- https://fallout4.wiki.fextralife.com/Controls (Xbox list; its "LB - Aim/Block" is a typo for LT)
- https://strategywiki.org/wiki/Fallout_4/Controls (search snippet only, page blocked)
- fallout.fandom.com blocked (HTTP 403/402)

## Device checks (Y5WWBMJVOZSK4HU8)
- Add game, path typed /storage/emulated/0/Download/1DM/Compressed/Fallout4.exe: detected x86_64. Name field saved as "Fallout" (space/4 dropped by `input text`).
- Controller config dropdown lists "Fallout 4 (template)" (1-dropdown). Selected and saved (2, 5).
- Launch sheet: Controller "Fallout 4", output keyboard (3).
- Controls overlay preview (4): V, ESC, RMB, Q, ALT, LMB, SPC, R, TAB, E captions shown, WASD and MOUSE sticks. D-pad is drawn as arrows without key captions, and L3/R3/Guide are not drawn, so Enter, T, G, Shift, Ctrl, F5 work but are not labelled. Return is in the editor key list as "Enter" (ControllerKeys name `Return`).
- Not tested: in-game key delivery, the final Enter/T/G build was not launched in a game.
