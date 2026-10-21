# BC lowmem v2: full saving under DXVK, BC6H as ASTC HDR, faster transcoder

Patches on top of csf-v11/197 (BC lowmem ETC2/EAC transcode):

| patch | what |
|---|---|
| csf-v11/200 | transcoder 1.84x less work per block (host), bit-identical output; workgroup size as a spec constant (`PANVK_BC_LM_WG`); `PANVK_DEBUG=bc_timing` GPU timing log |
| csf-v11/201 | raw store: readable (TRANSFER_SRC) lowmem images keep no raw plane in the image memory; raw blocks live in a driver BO only while command buffers use them, and in a spill file otherwise |
| csf-v11/202 | BC6H UF16 shadow as ASTC 4x4 HDR (8 bpp) instead of B10G11R11 (32 bpp) |

Host harness: `tests/bc-host/` (shader bodies compiled as C; see its
README). Mesa trees: `/home/flux/src/mesa-bc` (clean: series through 199
+ 200..202, one commit per patch), `/home/flux/src/mesa-lowmem` (build
tree: the same + jm-v9 + jm-v7, applied strictly by
`apply-patches.sh`-style checks, 36/36 with no offsets).

## Targets (RC4-lowmem, from the coordinator)

2048x2048 with mips: BC7 10.9 -> 5.5 MB, BC3 16.4 -> 5.5, BC1 8.2 -> 2.7,
BC6H only -11..-20%; fullscreen sampling 0.435 ms vs 0.49 exact; peak
process RAM ~100 MB higher (270 vs 172 MB in bc_perf). RC4-lowmem also
returned garbage for every raw readback and BC-source copy.

## 1. Full saving under DXVK: the raw store (csf-v11/201)

### Problem

Vulkan returns the raw BC blocks from image -> buffer copies and
BC-source image copies, and a BC -> BC copy (D3D11 CopySubresourceRegion,
texture streaming) needs the original blocks, not a re-encode of the
shadow. 197 therefore kept the raw plane in the image memory for every
TRANSFER_SRC image; DXVK sets TRANSFER_SRC on every texture
(`d3d11_texture.cpp`), so under DXVK BC1-BC5 were 2x the BC size and
BC6H/BC7 did not shrink at all.

### Options considered

| design | memory under DXVK | correct per spec | cost |
|---|---|---|---|
| (a) raw plane in image memory (197) | raw + shadow | yes | none |
| (b) no raw blocks at all (RC4-lowmem) | shadow | no: readback and copies return garbage | none |
| (c) re-encode BC from the shadow on readback | shadow | no: blocks are not bit-exact, BC -> BC copies degrade every generation | BC encoder on the GPU |
| (d) CPU shadow captured at vkQueueSubmit from the host-visible upload buffer | shadow + BC size in RAM | no: the buffer may be written by the GPU or after the submit (wait-before-signal), so the capture can be stale | memcpy per upload |
| (e) raw blocks in a separate driver BO for the image's life | same total RAM as (a), only moved out of the image | yes | an allocation per image |
| (f) driver BO only while command buffers use it, spill file otherwise (chosen) | shadow + transient BO for in-flight uploads; BC size on flash | yes | flash write per upload, pread on a later raw read |
| (g) (f) with an LZ4/zstd CPU heap instead of a file | shadow + ~0.8-0.95x BC size in RAM (BC data barely compresses) | yes | CPU compression |

Only (f) removes the raw blocks from RAM while keeping every copy exact.
(g) would keep ~85-95% of the bytes resident (BC blocks are already
entropy-dense), and on UMA a GPU BO and a malloc are the same RAM, so (e)
saves nothing in reality even though vkGetImageMemoryRequirements gets
smaller.

### Design

- Gate: lowmem image with TRANSFER_SRC, raw blocks >= `PANVK_BC_RAW_MIN`
  (64 KiB; smaller images keep the in-image plane), `PANVK_BC_RAW=store`
  (default; `plane` = 197). The image memory is the shadow only.
- `struct panvk_bc_raw` per image: BO (CPU-cached), command-buffer
  reference count, generation count, spill-file offset.
- Every raw access is recorded through `panvk_bc_raw_acquire()`:
  buffer -> image uploads (raw write; the transcode still reads the copy
  source), image -> buffer copies (read), both sides of `vkCmdCopyImage`.
  It creates the BO if needed (zeroed, or `pread` from the spill file),
  points `planes[0].plane.base` at it and takes a reference for the command
  buffer. ZERO_INITIALIZED decodes read a zero block row instead.
- References are dropped when the command buffer is reset or freed
  (`cmd_bc_release`, csf `panvk_reset_cmdbuf` / `panvk_destroy_cmdbuf`).
  Vulkan forbids both while it is pending, so no queued GPU work can still
  use the BO. At zero, a low-priority worker thread writes the blocks to
  the per-device unlinked spill file (`O_TMPFILE` in `$PANVK_BC_RAW_DIR`,
  `$TMPDIR`, `/data/local/tmp`, `/tmp`), runs `sync_file_range` +
  `POSIX_FADV_DONTNEED`, and frees the BO. An acquire during the write bumps
  the generation and the spill is discarded (the BO stays).
- One BO per image at any time, so GPU reads and writes of the raw blocks
  keep normal Vulkan ordering whatever the submission order.
- A store read back from the file twice is pinned resident (no ping-pong
  for images that are copied from or updated every frame). No spill
  directory, or a write error: stores stay resident (= 197's memory).
- Image destroy punches the file range (`FALLOC_FL_PUNCH_HOLE`).

### Trade-offs

- Flash: every TRANSFER_SRC BC upload is written once (BC size of the
  uploaded textures, e.g. 0.5-2 GB for a large game session); space is
  released with the image. `PANVK_BC_RAW=plane` avoids it.
- Record-time `pread` on the first raw access after a spill (texture
  streaming copies, readback): BC size per image, from page cache or flash.
- Peak RAM: the raw BO lives from the upload until DXVK resets that command
  buffer (a few frames), i.e. only the textures in flight, not all of them.
- No host staging copy anywhere (RC4-lowmem's ~100 MB extra peak in
  bc_perf looked like host staging; this design has none).

## 2. BC6H / BC7 memory (csf-v11/202)

| option | bpp | vs current shadow | verdict |
|---|---|---|---|
| BC7 -> ASTC 4x4 LDR | 8 | same size as the 8 bpp AFRC shadow (csf-v11/185) | no memory gain; ASTC samples 0.84 ms vs 0.26 ms (mode E, random payload). Rejected |
| BC7 -> ETC2 RGBA8 | 8 | same size | no gain, and ETC2 is ~3-5 dB below BC7 (BC1 -> ETC2 already loses 1 dB vs source). Rejected |
| BC7 -> ETC2 RGB8 for opaque images | 4 | -50% | the shadow format is fixed at vkGetImageMemoryRequirements, before any data; DXGI BC7 has no opaque variant. Not possible without lazily placed shadows |
| BC6H UF -> ASTC 4x4 HDR | 8 | 22.4 -> 5.6 MiB (-75%) | implemented |
| BC6H SF -> anything | | ASTC HDR has no negative values; RGBA16F has no AFRC mode | stays RGBA16F |

The G615 reports ASTC LDR and HDR native (TEXTURE_FEATURES_0 0xc7fe001e,
`PIPE_FORMAT_ASTC_4x4_FLOAT` -> `ASTC_2D_HDR`). The gate checks the
format's texture feature bit; `PANVK_BC_BC6H=exact` keeps B10G11R11.

Encoder (`bc/panvk_bc_astc_enc.h`, GLSL/C subset): one partition, CEM 11
(HDR RGB direct; all 8 sub-modes, both endpoint orders and the majcomp-3
layout tried, best decoded endpoints kept), QUANT_256 endpoints, 4x4
weights at QUANT_12 (trit ISE, 58 bits; QUANT_8 was 2 dB worse), endpoints
from the principal axis in the decoder's LNS domain, up to 2
least-squares refinements, weights chosen on the decoded fp16 values.

Host results (`tests/bc-host/test_astc`, 131072 blocks, BC6H from a
single-region 10-bit encoder over synthetic HDR pictures, and random
UF16 blocks of every mode):

- Bitstream: 0 mismatches between astcenc 4.8.0's decode and the
  encoder's model, every texel and channel.
- Quality vs the exact BC6H decode (tone-mapped x/(1+x) PSNR, 8-bit scale;
  log2 RMSE; max relative error), next to the B10G11R11 rounding the exact
  path already applies:

| content | ASTC 4x4 HDR (8 bpp) | B10G11R11 (32 bpp) |
|---|---|---|
| sky + sun, 0..60 | 63.56 dB, 0.0045, 0.050 | 59.60 dB, 0.0065, 0.015 |
| lightmap, 0..8 | 52.06 dB, 0.0161, 0.105 | 60.59 dB, 0.0067, 0.015 |
| noisy texture, 0..2 | 52.53 dB, 0.0138, 0.076 | 59.07 dB, 0.0065, 0.015 |
| emissive panel, 0.02..60 | 80.15 dB, 0.0049, 0.005 | 80.49 dB, 0.0043, 0.004 |
| random BC6H blocks | 23.58 dB, 1.04 | 68.25 dB, 0.0060 |

Random blocks (noise spanning 1e-5..6e4 inside one block, two regions)
are the worst case of a one-partition format; real BC6H content (sky,
light probes, lightmaps) lands at 52-64 dB, the class of the BC1 -> ETC2
transcode (38.7 dB on LDR) or better. Host encode time 3.1 us/block.

## 3. Speed (csf-v11/200)

All changes keep the 197 output bit-identical (`bench_etc2`: 0 mismatches
over 4 x 65536 blocks x 8 modes, random + 3 picture sets):

- one packed word per decoded texel instead of five int[16] arrays
  (16 live values instead of 80: register pressure / TLS spills);
- BC1 palette (4) and RGTC palette (8) decoded once per block;
- candidate colours / EAC values clamped once per table;
- early-outs: a candidate stops when its partial error reaches the best,
  the search stops at error 0 (strict comparisons: same winner).

Host time per 1000 blocks (aarch64 gcc -O2, opts 0x30000, 3 reps):

| mode | 197 us | 200 us | speedup |
|---|---|---|---|
| BC1 | 2186 | 1302 | 1.68x |
| BC1A | 2143 | 1189 | 1.80x |
| BC2 | 5193 | 3807 | 1.36x |
| BC3 | 5281 | 3717 | 1.42x |
| BC4 U/S | 3077 / 3289 | 1325 / 1408 | 2.32x / 2.34x |
| BC5 U/S | 6126 / 6382 | 2747 / 2794 | 2.23x / 2.28x |
| all | | | 1.84x |

The host does not show the GPU register-pressure effect (likely the larger
win on Mali); `PANVK_DEBUG=bc_timing` measures it on the device. Workgroup
size: `PANVK_BC_LM_WG=XxY` (spec constant, part of the pipeline key) for a
device sweep (8x8 default, try 16x4, 32x2, 4x4).

`PANVK_DEBUG=bc_timing`: ALL_COMMANDS timestamps around every BC upload
and BC image copy, read back at command-buffer reset, logged as
`panvk: bc_timing <fmt> <w>x<h> regions=N layers=N xcode|decode: X us`
and a total every 64 operations. It serializes the queue: measurement
only.

The BC6H -> ASTC transcoder does an exact BC6H decode and a search per
block: expect it to cost more than the 3-6 ms fragment decode per 2048^2
chain. Measure; `PANVK_BC_BC6H=exact` is the fallback.

## Comparison with RC4-lowmem

Image memory per 2048x2048 chain with mips (MiB), usage
SAMPLED|TRANSFER_SRC|TRANSFER_DST (what DXVK asks for). v2 numbers are
computed from the formats (shadow bpp x 349526 blocks, plus U-interleaved
tile padding of the small mips); device confirmation is in the plan.

| format | exact (180/185) | 197 | RC4-lowmem | v2 (200-202) | v2 vs RC4-lowmem |
|---|---|---|---|---|---|
| BC1 | 8.2 | 5.5 | 2.7 | 2.7 (ETC2 RGB8) | equal (4 bpp: the ETC2 floor) |
| BC3 | 16.4 | 11 | 5.5 | 5.4 (ETC2 RGBA8) | equal (8 bpp floor) |
| BC4 | 8.5 | 5.5 | ~2.7 | 2.7 (EAC R11) | equal |
| BC5 | 16.6 | 11 | ~5.5 | 5.4 (EAC RG11) | equal |
| BC7 | 10.9 | 10.9 | 5.5 | 5.4 (AFRC 8 bpp) | equal |
| BC6H UF | 28.1 | 28.1 | 22.6 | 5.6 (ASTC 4x4 HDR) | 4x smaller |
| BC6H SF | 48.5 | 48.5 | ~43 | ~43 (RGBA16F) | equal |
| raw readback / BC copies | exact | exact | garbage | exact | fixed |
| host staging (bc_perf peak RAM) | none | none | ~+100 MB | none (transient raw BO per upload in flight) | lower (to measure) |

BC1-BC5 and BC7 are at the floor of what the GPU samples natively (4 bpp
ETC2/EAC for 1-channel and opaque RGB, 8 bpp for RGBA and BC7); going
below needs a lossier format (ASTC 5x5/6x6), which would cost more
quality than the transcode already does. The win over RC4-lowmem there is
correctness at the same size; BC6H UF is 4x smaller.

Sampling: BC1-BC5 shadows are the RC4-lowmem formats (same texture
units, same ~0.435 ms fullscreen figure is expected); BC7 keeps the AFRC
shadow (0.26 ms mode E, at the tap floor). BC6H UF changes from B10G11R11
(0.33 ms mode E) to ASTC HDR, whose bandwidth-bound cost is unknown for
single-partition blocks (0.84 ms for random ASTC payloads): the main
device question.

Upload: transcoder 1.84x faster than 197 on the host proxy (RC4-lowmem's
kernels are precompiled Mali binaries in the libpan blobs, so they can not
be run in the host harness; an apples-to-apples speed comparison needs
the device: same bc_perf upload column with the RC4-lowmem .so and with
this build).

Quality: the ETC2/EAC encoder is the 197 one (BC1 38.7 dB, BC3 alpha
49.1, BC4 49.3 vs exact on the synthetic picture; RC4-lowmem measured
39-50 dB on device).

## Device test plan (G615, needs the user's phone)

Build: see "Build" below. Load the .so directly by path like the
bc-emul-perf runs (`/data/local/tmp/bcperf/`), serialized with the device
lock. Default env unless noted.

1. Log: `panvk: BC emulation on, lowmem`; first spill logs
   `panvk: BC raw store spills to <dir>` (or the "stay resident" warning).
2. bc_decode + bc_verify (`tests/dxvk/vulkan/bc/`), all 16 formats:
   - `PANVK_BC_MODE=exact`: 16/16 (regression gate).
   - default: raw readback and image -> image copies bit-exact for all 16
     (these images have TRANSFER_SRC, so they now use the raw store);
     sampled BC6H UF within the ASTC error (expect ~50-60 dB on real data,
     not the 2 LSB gate), BC1-BC5 within the ETC2/EAC gate, BC7 and BC6H SF
     unchanged.
   - `PANVK_BC_RAW=plane` and `PANVK_BC_BC6H=exact`: same as 197.
3. Raw store specifically (extend bc_decode or a small test):
   upload, submit, wait, reset the upload command buffer, wait ~100 ms
   (spill), then image -> buffer readback and BC -> BC copy: bit-exact;
   repeat the readback 3 times (pinning after 2 read-backs); partial 8x8
   update after a spill (read-back + merge); ZERO_INITIALIZED raw-store
   image read back = zeros; destroy with a recorded-but-unreset command
   buffer (no leak / crash).
4. vkCmdCopyImage BC matrix from the 197 plan (raw-store src/dst,
   no-raw dst, R32G32(B32A32)_UINT <-> BC).
5. bc_perf (`device/bc-perf.c`): image memory per format (expect the v2
   column), upload ms with `PANVK_DEBUG=bc_timing` (per-format xcode us
   in logcat) for 197 vs 200, and with `PANVK_BC_LM_WG=16x4`, `32x2`,
   `4x4`; sampling modes A-E, especially BC6H UF ASTC vs
   `PANVK_BC_BC6H=exact`. Peak RSS (`/proc/<pid>/status` VmHWM) vs the
   exact path and vs the RC4-lowmem .so (target: below 270 MB, ideally at
   or below 172 MB).
6. Spill file: size of the unlinked file (`ls -l /proc/<pid>/fd`) and
   disk usage during a game; RSS of a DXVK game (MiSide / NFS) default vs
   `PANVK_BC_RAW=plane`, DXVK_HUD memory, and no visual corruption.
7. CTS: `dEQP-VK.texture.compressed.*bc*`,
   `dEQP-VK.api.copy_and_blit.*bc*`,
   `dEQP-VK.memory.zero_initialize_device_memory.image_transition.bc*`.

## Build
