# v13 SM-X930 repeats, 122h vs 116, one session, alternating, short window (10 s warm-up / 30 s measured)

| test | 122h | 116 | verdict |
|---|---|---|---|
| s6 gears fps (1 s min), order 122h/116/122h/116 | 474.5 (349) / 578.5 (488) | 471.0 (332) / 475.6 (n/a) | no regression; 122h equal or faster |
| s8 FL 10_0 fps, order 122h/116/122h | 1706 / 1709 | 1446 | 122h +18 % |
| s8 FL 10_0 1 % low | 249 / 247 | 274 | 122h -10 %, repeatable |
| s8 FL 10_0 p99 frame ms | 1.8 / 1.9 | 2.1 | 122h better |
| s8 FL 11_1 fps | 1666 / 1718 | 1434 | 122h +18 % |
| s8 FL 11_1 1 % low | 225 / 270 | 236 | equal (spread covers 116) |
| s8 FL 11_1 p99 frame ms | 1.9 / 1.8 | 2.1 | 122h better |

All passes PASS, kit rc=0, cleanup rc=0, no DEVICE_LOST.
The earlier gears -5.5 % (single pass) does not reproduce. FL 10_0 1 % low stays about 10 % below 116 at about 18 % higher average fps and better p99; 1 % low at 1700 fps is about 4 ms worst frames, likely occasional stalls rather than throughput.
