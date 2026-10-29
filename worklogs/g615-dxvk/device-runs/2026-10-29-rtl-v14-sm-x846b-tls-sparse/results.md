# RTL SM-X846B (Mali-G1-Ultra, v14) - D3D9 DEVICE_LOST = sparse device TLS (2026-10-29)

Device: Galaxy Tab S12+ SM-X846B (MT6993), gpu_id 0xe8800010, shader_present 0x3300ff (12 cores, 22 core IDs),
kbase CSF uAPI 1.38, coherency ACE-Lite. PanPlay game loop scenario 10 with an override (`raw/gl-*.json`: d3d9 cube only,
warm-up 3 s, measure 6-20 s; each envSet is one pass). Fault lines per pass: `raw/gameloop-*-faults.txt`.

## Not a deadlock
Every DEVICE_LOST starts with a GPU MMU fault reported to all 3 groups (`kbase: CSF group N fatal error: status
0x....02c1`, exception 0xc0/0xc1 translation fault, READ; one 0x03c1 WRITE), 1-7 s after the first frame. The
subqueue timeouts 10 s later are the consequence. Fault addresses: 0x0, 0x3f800000, 0x3f67f100, 0x3e7ace00,
0x144009d7c00, 0x144000ae5c0, 0x144001dad80, 0x5f00000000 (write): pointers reloaded from memory that holds other data.

## A/B (115g driver git-2f2a20a728 unless noted)
| env | passes | faulted |
|---|---|---|
| default / `old` / `PANVK_KBASE_IMPORT_SYNC=1` / DXVK GPL off / `PANVK_DEBUG=no_wb_mmap` / `d3d9.presentInterval = 0` / `PANVK_TLS_SPARSE=1` | 17 | 13 |
| `PANVK_TLS_SPARSE=0` | 6 | 0 |
| `PANVK_DEBUG=sync` (decode ctx, also no alias) | 2 | 0 |
| 0148 build git-41aa2dafce, default (full TLS BO) | 7 | 0 |
| 0148 build, `PANVK_TLS_SPARSE=1` | 1 | 1 |

PanProbe `tessellation` (`raw/panprobe-tess.txt`): installed git-2732f11e42 with `PANVK_TLS_SPARSE=1` FAIL mismatch
437/64/510, `=0` PASS 0 (4 of 4); 0148 build default PASS 0 (3 of 3). The v14 tessellation mismatch blamed on stale
chunk data (0147) is this bug.

Fix: jm-v7-wip 0148, `PANVK_TLS_SPARSE` defaults to 0 on v14.
