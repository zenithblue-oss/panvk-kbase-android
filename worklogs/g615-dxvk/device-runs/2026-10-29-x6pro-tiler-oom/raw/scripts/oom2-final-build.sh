#!/bin/bash
sh /var/tmp/panvk/oom2-build.sh /var/tmp/panvk/mesa-oom2-final oom2final
O=/var/tmp/panvk/build-out/2026-10-29-oom2final
mkdir -p $O && cp /var/tmp/panvk/bionic-oom2final-dist/libvulkan_panfrost.so $O/
bash /var/tmp/panvk/oom2-apk.sh $O/libvulkan_panfrost.so oom2final play
echo FINAL-DONE
