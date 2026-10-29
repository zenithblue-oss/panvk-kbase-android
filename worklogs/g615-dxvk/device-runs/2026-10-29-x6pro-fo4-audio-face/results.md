# Fallout 4: no sound in gameplay, black Nora face (X6 Pro, v11, 2026-10-29)

Device: Poco X6 Pro Y5WWBMJVOZSK4HU8 (Mali-G615, v11). Driver 121b (git-07177325ca, p0163), DXVK 3.1.1-5-arm64ec,
Proton 11.0-2-arm64ec, ALSA audio (default). Game: `/storage/emulated/0/Download/1DM/Compressed/Fallout4.exe`,
shortcut "Fallout" (`gce0d4e3d`). The user played parts of these sessions with an Xbox pad, so some runs
include their input. Saves, the INIs and the shortcut JSON were backed up to `/var/tmp/panvk/fo4-audio-face/backup`
before the runs. The shortcut env was only changed for the A/B runs and was restored to `{}` at the end.

## 1. Sound: root cause and fix

Fallout 4 plays most of its audio (music, voices, most effects) as xWMA through XAudio2 2.7 (Wine builtin, FAudio).
FAudio decodes xWMA through `wmadmod` -> `winegstreamer`. The intro and menu UI sounds are PCM, so they still played.
Once WMA was needed, every xWMA voice failed. There were two faults, one after the other:

1. **`winegstreamer.so` did not load.** Its NEEDED `libgstgl-1.0.so` (imagefs) has NEEDED `libGL.so.1`, and no
   component ships a `libGL.so.1` (Mesa-Zink only has EGL/GLES). `winegstreamer.dll` loaded (`+loaddll`), but no
   GStreamer library was mapped into `Fallout4.exe` (`/proc/<pid>/maps`). `gst-inspect-1.0` with the same
   `LD_LIBRARY_PATH` shows the error directly: `libgstopengl.so: dlopen failed: library "libGL.so.1" not found`.
2. **With 1 fixed, Proton's media converter won the decoder search.** Proton's winegstreamer registers
   `protonaudioconverterbin` for `audio/x-wma` at a higher rank than gst-libav's `avdec_wmav2` (rank marginal). It
   needs Steam's transcoded-media fozdb (`MEDIACONV_*`). Without that it fails, and every WMA transform fails
   (`run-excerpt.txt`):
   `protonmediaconverter ... dump_fozdb_open: Env MEDIACONV_AUDIO_DUMP_FILE not set` ->
   `wg_transform_create: Failed to create winegstreamer transform`. If it had been configured, it would only play
   blank audio for media Steam has not transcoded.

The audio path itself was fine. The AAudio track of the game stayed active and consumed frames the whole time
(`dumpsys media.audio_flinger`, 0 underruns). It was only fed silence. There was no OOM or audio-server death.

Fix (launcher, `Containers.kt` + `cpp/glxstub.c`):
- `libglxstub.so` (soname `libGL.so.1`): the 15 GLX entry points libgstgl imports, all returning "no GLX". At every
  launch it is linked as `files/glxstub/libGL.so.1` (relinked when the APK's lib dir moves) and appended **last**
  to `LD_LIBRARY_PATH`, so a real libGL always wins. winex11's GLX init and GStreamer's GL elements see no GLX and
  disable themselves.
- `GST_PLUGIN_FEATURE_RANK=avdec_wmav1:MAX,avdec_wmav2:MAX,avdec_wmapro:MAX` in the base env (a shortcut env can
  override it), so the real WMA decoders are picked.

Evidence. Output audio was captured with `scrcpy --audio-source=output` (levels per 5 s):
- Before (`audio-levels-before.txt`, run 2, 121b APK): sound at 10-18 s (PCM intro), then silence (-90 dB), then from 45 s a flat
  -49 dB peak / -63 dB rms floor through the whole Vault-Tec gameplay scene (`audio-before-gameplay.jpg`: subtitles,
  but no voice).
- Stub only (run 3): winegstreamer now loads (52 gst mappings), but the transform fails as described in 2.
- Stub + rank (run 4, `audio-levels-after.txt`): menu music at 25-60 s (rms -25 dB), then gameplay from 85 s to the
  end at 290 s (3.4 min, Vault 111 elevator with dialogue, `audio-after-gameplay.jpg`) with peaks of -9 to -20 dB and rms
  -30 to -40 dB, never back to the floor. The log shows libav decoding (`gstavcodecmap ... channel layout`).
  The user also reported that sound works in their next session.

Run 4 used `panplay-121b-audio.apk` (stub) with `GST_PLUGIN_FEATURE_RANK` set in the shortcut env.
`panplay-121b-audio2.apk` (sha256 87fc2dfc..., `/var/tmp/panvk/build-out/apks/`) has both in the launcher and no
shortcut env. Its run 5 recorded 240 s with menu music and then gameplay at rms -29 to -42 dB the whole time.

## 2. Black Nora face

Reproduced on 121b. In the Vault 111 entrance cutscene and in the elevator after loading the Exitsave, Nora's head
(face and hair) is a flat black silhouette, while the male characters' faces render (`face-before-121b-cutscene.jpg`,
`face-before-121b-elevator.jpg`). It is black right after a save load, not only after minutes under memory pressure
as in the 110 run. With `MESA_LOG_FILE`, the driver logged no `BC image without raw blocks skipped` line, so the
lazy-raw skip (black-head cause 1 in `fo4-memory-analysis-2026-10-23.md`) did not fire. The 225 stale guard is in
this tree (csf-v11 <= 233).

### A/B on the user's elevator save (Quicksave 20:47, facing Nora), wireless adb, battery <= 39 C

Each run loaded the same save with the audio2 APK. Only the shortcut env, driver or render API changed:

| Run | Change | Nora's face | Shot |
|---|---|---|---|
| base | 121b, DXVK, launcher defaults | black | `face-before-121b-elevator.jpg`, `nora-face-user-aligned-204729.png` |
| exact | `PANVK_BC_MODE=exact` (BC lowmem/ETC2 transcode off) | black | `face-exact2-n.jpg` |
| rc4 | driver ab-rc4 (Mesa 5a07217f rc4) | black | `face-rc4-3.jpg` |
| mip0 | `iTexMipMapSkip=0` in Fallout4Prefs.ini (restored to 2) | black | `face-mip0-3.jpg` |
| nocap | `DXVK_CONFIG` without `dxgi.maxDeviceMemory/maxSharedMemory` | black | `face-nocap-3.jpg` |
| nan | `+ d3d11.enableRtOutputNanFixup = True` | black | `face-nan-3.jpg` |
| amd | `+ dxgi.customVendorId = 1002, customDeviceId = 73bf` | black | `face-amd-3.jpg` |
| **wd3d** | **Render API WineD3D-Zink** (`PANPLAY_RENDER_API=wined3d`), same 121b driver | **correct** | `face-wd3d-3.jpg` |

Findings:
- **This is not a regression.** rc4 shows it too, so the earlier "no black heads" WhatsNew entry (csf-v11 225) fixed
  a different, later-in-session cause. 0153/0155/0162 are not the cause either.
- **This is not the BC lowmem transcode**: exact mode is still black. The driver logged no lazy-raw skip.
- It is not FO4's texture budget (memory cap removed), the mip skip, the vendor id, or NaN render-target outputs.
- The same driver and the same BC emulation render the face correctly through WineD3D -> Zink. So the cause is in
  something DXVK does that WineD3D does not, on the FaceGen face material only (hair, clothes, the baby and male NPC
  faces render). Male NPCs use pre-baked FaceGen textures. Nora and the player have runtime-built FaceGen data.
  Candidates still open: a DXVK-specific shader pattern in the face/skin pixel shader compiled by PanVK (float
  controls, `SampleCmp`/`Gather`, min-LOD clamp), or a resource path DXVK uses for the runtime face textures (UAV /
  typeless or mutable views, a `CopyResource` between compatible formats). The next step is a frame capture of the
  face draw (DXVK shader dump `DXVK_SHADER_DUMP_PATH` + RenderDoc-style replay on desktop RADV vs PanVK).

### UI flicker report (coordinator, 21:00)

The "face fixed" session the user saw was the **wd3d A/B run** (WineD3D-Zink render API set through the shortcut
env). It is not a shipped change. WineD3D-Zink is known to have Scaleform UI artifacts in FO4: subtitles garble
in `face-wd3d-3.jpg`. The shortcut was restored to its original `{}` env and default driver (DXVK), so the flicker is
gone and the black face is back. Nothing in the committed launcher changes affects rendering.

## Limits

- The glxstub makes winegstreamer load for every game, so Media Foundation / DirectShow video and audio now go
  through GStreamer where it used to fail at once. Video still hits Proton's converter, which plays a blank video
  without Steam's transcodes, as before.
- `scrcpy --audio-source=output` mutes the device speaker while it records. The user reported that sound works.
