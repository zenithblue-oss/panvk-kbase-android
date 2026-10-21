# Host tests of the panvk BC transcoder shaders

The lowmem transcoders (`src/panfrost/vulkan/bc/panvk_bc_etc2.comp`,
`panvk_bc_astc_enc.h` in the patched Mesa tree) are written in the GLSL
subset that is also C, so their bodies compile and run on the host.

- `test_etc2`: ETC2/EAC bitstream check against Mesa's ETC2 decoder
  (`ref/ref_etc*.h`, verbatim from `src/mesa/main/texcompress_etc.c`) for
  every mode, plus PSNR of the transcode against the exact BC decode.
- `bench_etc2`: the csf-v11/197 transcoder (`ref/panvk_bc_etc2_197.comp`)
  against the current one: bit-identical output on random and picture
  blocks, and host time per block. Args: `[opts] [reps]`.
- `test_astc` (csf-v11/202): BC6H UF16 -> ASTC 4x4 HDR. Every block is
  decoded by the ARM reference codec and must match the encoder's own
  model bit for bit; quality against the exact BC6H decode (Mesa's BPTC
  decoder) next to the B10G11R11 rounding of the exact path.

```
make MESA=/path/to/patched/mesa run
make MESA=... ASTCENC=/path/to/astc-encoder test_astc && ./test_astc
```

`ASTCENC` is a checkout of https://github.com/ARM-software/astc-encoder
(tested with tag 4.8.0); only its core codec is built (`libastcenc.a`).
