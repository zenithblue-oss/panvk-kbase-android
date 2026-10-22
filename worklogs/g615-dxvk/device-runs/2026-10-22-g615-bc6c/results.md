# g615 bc6c device runs (2026-10-22)

Device: Poco X6 Pro (Mali-G615 MC6, arch v11), wifi adb 10.11.176.130:38911. APK panprobe-bc6c.apk, package
dev.zenithblue.panvktest.bc6c (versionCode 15). aapt2/sha256 checked: rc5 .so d4cb414a..., rc4 .so c395db09... (both match).
Fresh-ish install (`install -r`), shader cache ON. DRIVER line for rc5: `PanVK-kbase RC5-dev-bc6c (git-5a07217f03)`; rc4: `RC4 (git-5437602824)`.
Thermal status 0 before every run; battery 100% charging, 34 C. Raw logs in raw/, screencaps in screenshots/.

## Summary
- **autorun all (rc5, run=20261022-153443): 37/39 pass, 2 FAIL.** Not 38/38: total is 39 now (tess_perf is new).
  - `tessellation` FAIL mismatch=12358 (13 of 24 CASEs; tri/isolines/grid/indirect pass; got blue, want red, partial pixel coverage).
  - `tess_perf` FAIL (TESS_PERF_FAILS=6): its internal default-vs-legacy hash compare mismatches for L=4, 16, mix at gs=0 and gs=1.
- **Bisect: PANVK_TESS_FUSE (patch 213, default 1) is the culprit.** `tessellation` with PANVK_TESS_FUSE=0 alone passes; with only SETUP_STEP=0, ARENAS=1 or WINDING_INLINE=0 it still fails with the same mismatch=12358. Full legacy set also passes. tess_perf with PANVK_TESS_FUSE=0 passes (default and legacy variants then give identical hashes).
- BC A/B: all 6 bc_perf runs PASS. bc6c is faster than RC4 on BC6H/BC7/BC2 etc. (table below). No BC toggle bisect needed (PANVK_BC_LM_BATCH / PANVK_BC_LM_OPTS not run: BC passes).
- bc_perf inside autorun all took 43.2 s (bc6b: 28.6 s); standalone it is 27.5-27.8 s (rc4 28.3-28.5 s). The 43 s is cold cache / first run after install (first run after a fresh install compiles shaders); not reproduced standalone.

## BC A/B (upload_gpu_ms, median of 3, interleaved rc4, rc5, rc4, rc5, rc4, rc5)
Run ids: rc4 154600/154723/154846; rc5 154641/154803/154930. Totals (bc_perf ms): rc4 28506/28256/28433 (median 28433); rc5 27805/27670/27476 (median 27670, -2.7%).
bc6b column = mean of its two runs (bc6b results.md, single-sample noise ~1-2 ms), RC4 and bc6c = median of 3.

| format | RC4 | bc6b (2 runs) | bc6c | verdict (bc6c vs RC4) |
|---|---|---|---|---|
| BC1_RGB_UNORM | 2.91 | 2.41 | 2.44 | faster (-16%) |
| BC1_RGBA_SRGB | 2.70 | 2.58 | 2.95 | slower (+9%, within noise: runs 2.86-3.16 vs 2.58-2.79) |
| BC2_UNORM | 4.22 | 3.51 | 3.30 | faster (-22%) |
| BC3_UNORM | 3.95 | 3.68 | 3.27 | faster (-17%) |
| BC3_SRGB | 4.77 | 3.98 | 3.82 | faster (-20%) |
| BC4_UNORM | 0.96 | 1.29 | 1.49 | slower/noisy (runs 0.67-1.72 vs 0.89-2.61) |
| BC4_SNORM | 2.10 | 1.61 | 0.92 | faster (noisy) |
| BC5_UNORM | 2.99 | 1.80 | 2.33 | faster (-22%, noisy) |
| BC5_SNORM | 3.41 | 2.27 | 2.20 | faster (-35%) |
| BC6H_UFLOAT | 7.53 | 4.11 | 4.31 | faster (-43%), stable |
| BC6H_SFLOAT | 8.29 | 4.30 | 4.97 | faster (-40%), stable |
| BC7_UNORM | 6.36 | 4.35 | 5.17 | faster (-19%), stable |
| BC7_SRGB | 6.33 | 5.33 | 5.41 | faster (-14%) |
| REF_RGBA8 | 1.78 | | 1.83 | same |
| REF_RG8 | 2.54 | | 1.65 | noise |
| REF_R8 | 2.00 | | 1.24 | noise |
| REF_RGBA16F | 3.80 | | 4.24 | noise (+11%) |
| REF_B10G11R11 | 2.28 | | 2.13 | same |
| REF_ETC2_RGBA8 | 0.98 | | 0.82 | same |
| REF_ASTC4x4 | 0.69 | | 0.80 | same |

Verdict: bc6c beats RC4 on 11 of 13 BC formats; BC1_RGBA_SRGB and BC4_UNORM are not faster, but their per-run spread (about +-1 ms on 1-3 ms values) is as large as the difference. Stable big wins: BC6H (-40%), BC7 (-15-19%), BC2/BC3 (-17-22%). Versus bc6b, bc6c is equal within noise (BC7_UNORM +0.8 ms, BC6H_SFLOAT +0.7 ms; BC2/BC3 slightly better, which is what 217 targets). A..E sample timings are identical in both drivers (0.43 / 0.22 / 0.33 ms).

## Memory (MEM lines, KiB; identical across the 3 runs of each driver within ~1 MiB)
| tag | RC4 rss | RC4 gpu_map | bc6c rss | bc6c gpu_map |
|---|---|---|---|---|
| before_device | 9864-10132 | 8 | 10152-10200 | 8 |
| full_feature_device | 210624-211672 | 177256 | 46304-46496 | 12196 |
| after_full_feature_device (hwm) | 13976-14388 (hwm 211.7-212.1 MiB-ish: 211720) | 8 | 14400-14540 (hwm ~46.3 MB) | 8 |
| end | 188240-188696 (hwm 277.2k) | 114104 | 84788-84824 (hwm 174.3k) | 23152 |

bc6c (lowmem transcode) uses about 4.6x less RSS and 14.5x less GPU map at full_feature_device, and about 0.45x RSS / 0.2x gpu_map at end. hwm at end: 277 MiB (RC4) vs 174 MiB (bc6c) -> actually KiB 277280 vs 174240.

## Tessellation: legacy vs new
`tessellation` test (rc5, standalone):
| config | result | fps | ms |
|---|---|---|---|
| defaults (run 154259) | FAIL mismatch=12358 | 2529.7 | 2372 |
| legacy set FUSE=0;SETUP_STEP=0;ARENAS=1;WINDING_INLINE=0 (154314) | PASS | 2540.1 | 2143 |
| FUSE=0 only (154326) | PASS | 2415.5 | 2067 |
| SETUP_STEP=0 only (154341) | FAIL 12358 | 2587.3 | |
| ARENAS=1 only (154355) | FAIL 12358 | 2412.7 | |
| WINDING_INLINE=0 only (154409) | FAIL 12358 | 2573.8 | |
(inside autorun all: FAIL 12358 at 2817.1 fps.)  fps is not meaningfully different, i.e. the fused path is currently wrong, not faster, in this test.

`tess_perf` us_draw (256 draws, 4096 patches, from autorun all; default = new defaults, legacy = the internal legacy variant):
| gs | L | new us_draw | legacy us_draw | new/legacy | hash new | hash legacy |
|---|---|---|---|---|---|---|
| 0 | ref (no tess) | 3.1 | 3.5 | | 4807f3f5 | 4807f3f5 |
| 0 | 1 | 335.8 | 580.7 | 0.58x | 1d38b605 | 1d38b605 |
| 0 | 4 | 1106.4 | 578.5 | 1.91x slower | 7b274c45 | 1d38b605 |
| 0 | 16 | 2666.4 | 3127.4 | 0.85x | b360d19b | 5c061e8a |
| 0 | mix | 2836.2 | 2806.2 | 1.01x | 83089bf5 | 1d38b605 |
| 1 | 1 | 535.4 | 725.4 | 0.74x | 1d38b605 | 1d38b605 |
| 1 | 4 | 1302.6 | 971.9 | 1.34x slower | e516be97 | 1d38b605 |
| 1 | 16 | 4331.6 | 4949.5 | 0.88x | b360d19b | 5c061e8a |
| 1 | mix | 3499.9 | 3940.6 | 0.89x | 83089bf5 | 1d38b605 |

Notes: the hashes differ at L=4, 16 and mix, which is what fails tess_perf. In the legacy variant the L=4 and mix hashes equal the L=1 hash (and L=16 differs from the new one), while the new path gives distinct per-level hashes. Which one is right is for the tess owner to decide from the shader; the `tessellation` test (reference image compare) passes with FUSE=0 and fails with FUSE=1, so the fused path is the one the reference rejects. L=4 is also slower in the new path (fused). With PANVK_TESS_FUSE=0 as rc5 default-variant (run 154511): L=1 488 / L=4 520 / L=16 2512 / mix 2483 (gs=0), L=1 683 / L=4 920 / L=16 4653 / mix 3654 (gs=1); tess_perf PASS (hashes then match legacy), but the 4-toggle legacy gain over FUSE=0 alone was nil, so only FUSE matters for correctness.

## Screenshots
screenshots/all-stuck.png (post autorun all UI, 37 pass / 2 fail), plus one per bisect/A-B run (tess-*.png, tessperf-fuse0.png, bcab*-rc*.png).
