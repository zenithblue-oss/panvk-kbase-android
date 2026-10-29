# RTL: Galaxy Tab S12+ SM-X846B (Mali-G1-Ultra, v14 CSF), v14fix driver PanProbe

- Samsung Remote Test Lab, adb `localhost:35395` (RDB). rc5 slot driverInfo `Mesa 26.3.0-devel (git-fbbac61b35)`
  = series through jm-v7-wip 0145 (0143 tessellator in registers, 0144 CSF logic-op dirty bits, 0145 TLS/WLS inside 4 GiB).
- `gpu_id=0xe8800010 arch=v14 va_bits=48`, CSF uAPI 1.38.
- Raw: `raw/autorun.txt`, `raw/runs/20261029-024417/` (device clock).

## Result: 39/40 PASS, 1 FAIL (tessellation mismatch=320), no DEVICE_LOST

Compared to 115e (`../2026-10-29-rtl-v14-sm-x846b-115e-panprobe/`, 38/40):

| test | 115e | v14fix |
|---|---|---|
| `tess_perf` | FAIL DEVICE_LOST (tiler heap OOM, CSF timeout) | PASS, all hashes match across opts / gs=0/1 |
| `tessellation` | FAIL DEVICE_LOST after `instanced2` (exception 0xc1) | FAIL mismatch=320, every CASE runs |
| others (38), 4 COMPLIANCE lines (0 missingHard) | PASS | PASS |

Failing CASEs (all other 22 PASS, incl. quad/tri/frac odd+even/isolines/point/winding, `grid_inst3_indirect_first1`):

```
CASE grid_indexed_inst2_direct_first1   ref_px=960 px=813 mismatch=160 first=(35,16) got=0 0 255 want=255 44 32 FAIL
CASE grid_indexed_inst2_indirect_first1 ref_px=960 px=811 mismatch=160 first=(35,16) got=0 0 255 want=255 44 32 FAIL
```

115e died before reaching these two cases, so it is unknown whether they failed there. Same driver passes them on v9
(TB336FU) and v11 (X6 Pro). Common factor: indexed + instanceCount=2 + firstInstance=1 (non-indexed instanced passes).

## sparse_kbase (informational): PASS pass=12 fail=0 skip=2 (T10 MEM_ALLOC_EX ENOTTY, unchanged).
