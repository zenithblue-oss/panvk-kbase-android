# 2026-10-29 X6 Pro (v11, Y5WWBMJVOZSK4HU8): PanPlay present path Default vs EGL, WineD3D-Zink

PanPlay 1.3.0 (44), from commit 2bdc7ddf plus the PANPLAY_RENDER_API env key. Driver: the bundled build 119
(git-0146a719f6, p0159, sha256 186278b8). Game loop runs used `--ei warmup_sec 10 --ei measure_sec 30` with the
override config `/var/tmp/panvk/egl-present/gameloop-egl.json`, which adds the env sets `egl`
(PANPLAY_PRESENT=egl), `zink` (PANPLAY_RENDER_API=wined3d) and `zinkegl`. Each pass's launcher.log confirms the path
it used ("present path EGL (AHardwareBuffer + EGLImage)", "Render API: WineD3D-Zink ...").

Runs 13:58 and 14:13 are not counted. The perf agent installed its own APK on the shared device at 13:55, so those
runs did not use this build. Every later run reinstalls this APK while it holds the device lock.

## What "EGL renderer" means in Winlator forks

- Winlator Mali (GunaCharanTeja/WinlatorMali, bionic-mali-1.2) is built as "(egl-renderer)". Its X server
  compositor is a native C++ EGL/GLES renderer (`cpp/winlator/renderer/egl.cpp`) that replaces the Java
  GLSurfaceView renderer. With "Use DRI3" on (the default), client swapchain buffers arrive as AHardwareBuffers
  (DRI3 PixmapFromBuffer(s) -> GPUImage). The compositor imports them with `eglCreateImageKHR(EGL_NATIVE_BUFFER_ANDROID)`
  and samples them without a copy. RGBA AHBs get an R/B swap (`swapRB`). The same chain runs post-FX, the upscaler and
  frame generation; frame generation uses R10G10B10A2 AHBs that are shared with Vulkan. With DRI3 off it sets
  `MESA_VK_WSI_DEBUG=sw`, which is the CPU copy path.
- Winlator-Ludashi / bionic: a per-shortcut or per-container "Renderer" choice of EGL (that same native GLES
  compositor), Vulkan (a Vulkan compositor) or DisplayX.
- Winlator cmod: a Java GLRenderer plus GPUImage EGLImage import for DRI3 buffers. Its "Renderer" spinner is WineD3D's
  own renderer (gl or vulkan), which is a different setting.

So "EGL renderer" is the display compositor and how frames reach it: a GLES compositor that samples the frame as an
EGLImage over an AHardwareBuffer. It is not a DXVK or vkd3d option. DXVK and vkd3d still render through Vulkan and
present through X11 WSI.

## What PanPlay got

The present path is set in Settings > Display (global), on the launch sheet per game, or with env PANPLAY_PRESENT:
- Default: X window pixels live in heap memory. Each frame is uploaded with glTexSubImage2D.
- EGL: window contents live in AHardwareBuffers (RGBA8, CPU write). The compositor binds each one once as an
  EGLImage texture with an R/B swizzle. A frame update is an unlock/lock cache sync, with no GL upload. If the EGLImage
  cannot be created, it falls back to the upload path.

The Render API option is now labelled "WineD3D-Zink (D3D9-11 → OpenGL → Zink → PanVK)". DXVK stays the default
(Auto). If a D3D12 exe is set to WineD3D-Zink, PanPlay warns that D3D12 has no OpenGL path (vkd3d is Vulkan only).

## Results

drv-fps is the driver present rate. drawn is compositor frames per second.

### D3D11 cube (scenario 3, d3d11 only)

| path | run 14:30 drv-fps | run 13:51 drv-fps | drawn | test |
|---|---|---|---|---|
| Default (DXVK) | 245.0 | 289.2 | 52.6 | PASS |
| EGL (DXVK) | 213.8 | 216.9 | 59.5 | PASS |
| WineD3D-Zink | 382.5 | 428.3 | 55.5 | PASS |
| WineD3D-Zink + EGL | 270.0 | 353.9 | 60.2 | PASS |

### D3D12 SquidRoom (scenario 5)

| path | drv-fps | drawn | test |
|---|---|---|---|
| Default | 63.4 | 56.7 | PASS |
| EGL | 58.5 | 54.3 | PASS |

WineD3D-Zink: not applicable (no GL path for D3D12).

### D3D12 feature levels (scenario 7)

| level | Default drv-fps | EGL drv-fps |
|---|---|---|
| 11_0 | 132.5 | 198.3 |
| 11_1 | 177.2 | 190.8 |
| 12_0 | 182.0 | 161.3 (verdict line printed, then the pass hit the 93 s exit watchdog) |
| 12_1 | 168.2 | 192.3 |
| 12_2 | 135.3 | 172.7 |

The 12_0 EGL pass printed its UNSUPPORTED verdict (as expected on v11) and `exit=0`, but Wine took too long to exit.
This is a teardown stall, not a rendering fault.

## Magenta bug

It did not reproduce. Colours are correct on every path: the cube's red, green, blue and yellow, SquidRoom, and the
FL triangles. PanPlay's EGL path takes X11 BGRA pixels into an RGBA8 AHB and swizzles R/B.

The tester's magenta frame (green = 0, UI text visible) comes from Winlator Mali's own compositor chain, not from a
PanVK swapchain format we can see. The likely suspects in that app are:
- its frame-generation or post-FX path, which uses R10G10B10A2 AHBs shared between Vulkan and GLES;
- the DRI3 AHB import of the game's swapchain.

No driver patch was made.

## Files

- Screenshots (`adb exec-out screencap -p`): `/var/tmp/panvk/egl-present/shots/` (s3-*, s5-*, s7-*)
- Grids: [`grid-s3-screencap.png`](/var/tmp/panvk/egl-present/grid-s3-screencap.png),
  [`grid-s3-harness.png`](/var/tmp/panvk/egl-present/grid-s3-harness.png),
  [`grid-s5.png`](/var/tmp/panvk/egl-present/grid-s5.png),
  [`grid-s7.png`](/var/tmp/panvk/egl-present/grid-s7.png)
- Game loop results: `/var/tmp/panvk/egl-present/runs/20261029-135135-s3`, `20261029-143020-s3`,
  `20261029-144704-s5`, `20261029-143452-s7`
- Winlator sources looked at: `/var/tmp/panvk/research/` (wm, coffincolors_winlator, StevenMXZ_Winlator-Ludashi)
