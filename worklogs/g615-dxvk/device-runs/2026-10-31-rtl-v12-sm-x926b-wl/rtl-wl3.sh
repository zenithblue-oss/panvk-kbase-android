#!/bin/bash
for n in 9 11; do
RUNS="$n:122g" ENVX=PANVK_KBASE_TILER_IR=1 TAG=-tilerir1 bash /var/tmp/panvk/b122/rtl-wl.sh
RUNS="$((n+1)):122g" bash /var/tmp/panvk/b122/rtl-wl.sh
done
