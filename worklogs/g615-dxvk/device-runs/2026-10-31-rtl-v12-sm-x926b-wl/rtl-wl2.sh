#!/bin/bash
W=/home/abhaybyte/repos/panvk/worklogs/g615-dxvk/device-runs/2026-10-31-rtl-v12-sm-x926b-wl
RUNS="7:122g" ENVX=PANVK_WSI_SW_CACHED=0 TAG=-cached0 bash /var/tmp/panvk/b122/rtl-wl.sh
RUNS="8:122g" ENVX=PANVK_KBASE_TILER_IR=1 TAG=-tilerir1 bash /var/tmp/panvk/b122/rtl-wl.sh
