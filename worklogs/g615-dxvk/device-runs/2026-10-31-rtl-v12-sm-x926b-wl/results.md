# v12 SM-X926B PanProbe workload_perf, 122g vs 116c (us/op, lower is better)

Runs alternate 122g,116c x3 (r1-r6), then 122g with one env each (r7, r8). APK install per run, autorun=workload_perf, scenario 2.

```
case | 116c | 122g | 122g-cached0 | 122g-tilerir1
gs_layer | 78.79/78.25/71.78 med 78.25 | 83.75/81.73/64.25 med 81.73 | 68.61 med 68.61 | 53.01 med 53.01
tex_rgba | 23.4/22.68/23.11 med 23.11 | 24.57/31.65/30.38 med 30.38 | 30.95 med 30.95 | 27.26 med 27.26
msaa_sshade | 71.91/73.4/70.25 med 71.91 | 70.82/71.62/70.83 med 70.83 | 70.18 med 70.18 | 63.46 med 63.46
msaa_a2one | 119.16/118.44/120.79 med 119.16 | 120.06/117.96/117.79 med 117.96 | 119.44 med 119.44 | 105.48 med 105.48
pg_query | 2.74/2.83/2.89 med 2.83 | 2.21/1.94/2.2 med 2.2 | 2.19 med 2.19 | 1.79 med 1.79
ref | 1.96/2.38/2.3 med 2.3 | 1.93/1.91/1.81 med 1.91 | 1.77 med 1.77 | 1.65 med 1.65
msaa_ref | 23.18/27.66/26.8 med 26.8 | 26.74/23.72/27.43 med 26.74 | 26.65 med 26.65 | 23.82 med 23.82
```

Confirmed lag: tex_rgba only. 122g 24.6/31.7/30.4 vs 116c 23.4/22.7/23.1 (median +31 %, ranges do not overlap).
gs_layer: ranges overlap (122g 64-84, 116c 72-79), noise. msaa_sshade, msaa_a2one: equal. pg_query: 122g faster (2.2 vs 2.8).
PANVK_WSI_SW_CACHED=0 does not fix tex_rgba (31.0). PANVK_KBASE_TILER_IR=1 partly (27.3, still above 116c); it also lowers every case about 10 %, single run.
