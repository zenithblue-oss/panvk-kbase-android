# FTL 122h (p0174) vs 116c, v6-v10: partial campaign (INVALID matrices)

Every matrix submitted once, no retries. Account/projects: the second Google account's projects with testing + toolresults enabled.
116c = newest 116 build (git-f7e78953ac); 122h driverInfo p0174 confirmed on bluejay. Scenario groups per matrix: A = s1-4, B = s5-7, C = s8-9. Each scenario gets its own run dir; one matrix = one execution.

## PanProbe (scenario 2)
| device (arch) | 116c | 122h |
|---|---|---|
| bluejay (v9) | 41/41, 155.9 s, 0 DEVICE_LOST | 41/41, 146.7 s, 0 DEVICE_LOST |
| panther (v10) | 41/41, 139.4 s, 0 DEVICE_LOST | INVALID (not run) |
| a31 (v7), starlte (v6) | not run | INVALID / not submitted |

## PanPlay bluejay (v9): fps / 1% low / ram MB (whole-run)
| scenario / task | 116c | 122h |
|---|---|---|
| s1 D3D11 | 170.6 / 96.9 / 239 | group A INVALID |
| s2 D3D9 | 60.0 / 48.8 / 437 | group A INVALID |
| s3 D3D11 (122g p0173) | 169.4 / 97.0 / 241 | 169.3 / 93.2 / 242 (122g) |
| s3 D3D9 (122g p0173) | 60.0 / 35.9 / 433 | 60.0 / 35.0 / 440 (122g) |
| s4 D3D11 default / old | 165.9 / 96.2 / 237 ; 124.9 / 75.9 / 238 | group A INVALID |
| s4 D3D9 default / old | 60.0 / 35.7 / 267 ; 60.0 / 38.5 / 264 | group A INVALID |
| s5-s7 D3D12 (SquidRoom, gears, FL 11_0..12_2) | group B INVALID | all 7 passes status ok, no fps (vkd3d has no DXVK stats), ram 214-618 MB |
| s8 D3D10 | group C INVALID | 145.6 / 87.2 / 243 |
| s8 D3D11 FL 10_0 / 10_1 / 11_0 / 11_1 | group C INVALID | 221.7 / 223.6 / 218.3 / 220.1 fps, 1% low 98-102, ram 186-189 |
| s8 D3D9 | group C INVALID | 60.0 / 37.2 / 436 |
| s9 D3D11 FL all | group C INVALID | 270.6 / 147.9 / 275 |
| s9 D3D12 FL all | group C INVALID | ok, no fps, ram 462 |

Graphics memory (vidmem peak) is 20 MB on D3D10/D3D9/D3D11 cubes and 8 MB on FL tests in every run; sysmem 0.

## PanPlay s3 on 116c vs 122g: see ../2026-10-31-ftl-122g-v9-and-panplay/results.md (all four devices).

## Matrix status
OK (FINISHED SUCCESS): 116c s3 x4 devices, 116c PanProbe bluejay+panther, 116c bluejay s124, 122h PanProbe bluejay, 122h bluejay s567 and s89.
INVALID (DETAILS_UNAVAILABLE, API gives no reason), not retried: 116c bluejay s567, 116c bluejay s89, 122h bluejay s1234, 122h panther probe, panther: 116c s124 / s567 / s89 and 122h s1234 / s567 / s89, 122h a31 PanProbe, 122h a31 s1234.
Not submitted (halted after INVALID in the first a31 wave): a31 and starlte 122h/116c remaining matrices (PanProbe, A/B/C groups), and all other a31/starlte combinations. 122h-vs-116 comparison therefore exists only for bluejay (partial) and the s3 cubes.
No pattern found by project or device (same projects had both valid and INVALID matrices); likely a new-project or throttling limit on Test Lab, worth checking in the console.
