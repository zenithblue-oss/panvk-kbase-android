#!/bin/sh
set -e
cd /tmp/tests
echo "=== GLIBC ENUMERATE ==="
gcc -O2 -o glibc-enum enumerate.c -ldl
./glibc-enum /tmp/build-glibc/src/panfrost/vulkan/libvulkan_panfrost.so

echo "=== GLIBC COMPUTE ==="
gcc -O2 -o glibc-compute compute.c -ldl -lm
./glibc-compute /tmp/build-glibc/src/panfrost/vulkan/libvulkan_panfrost.so

echo "=== GLIBC TRIANGLE ==="
gcc -O2 -o glibc-triangle triangle.c -ldl -lm
./glibc-triangle /tmp/build-glibc/src/panfrost/vulkan/libvulkan_panfrost.so
