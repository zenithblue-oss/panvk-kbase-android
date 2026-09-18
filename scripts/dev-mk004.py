#!/usr/bin/env python3
"""Assemble patch 004 from header + full .h diff + filtered .c hunks."""
import subprocess

M = 'work/mesa'
B = 'work/ref-g720-beta'
def udiff(a, b):
    p = subprocess.run(['diff', '-U3', a, b], capture_output=True, text=True)
    assert p.returncode in (0, 1), p.stderr
    return p.stdout.splitlines(keepends=True)

h = udiff(f'{M}/src/panfrost/vulkan/panvk_physical_device.h',
          f'{B}/src/panfrost/vulkan/panvk_physical_device.h')
# rewrite labels to a/ b/
h = [('--- a/src/panfrost/vulkan/panvk_physical_device.h\n'
      if l.startswith('--- ') else
      ('+++ b/src/panfrost/vulkan/panvk_physical_device.h\n'
       if l.startswith('+++ ') else l)) for l in h]
c = open('/tmp/pdev-kbase.diff').read().splitlines(keepends=True)
c = [('--- a/src/panfrost/vulkan/panvk_physical_device.c\n'
      if l.startswith('--- ') else
      ('+++ b/src/panfrost/vulkan/panvk_physical_device.c\n'
       if l.startswith('+++ ') else l)) for l in c]
header = """From: panvk-kbase-android worker <panvk-worker@localhost>
Subject: [PATCH kbase-common 004] panvk: kbase physical-device init path

Purpose: Add kbase_node_path to panvk_physical_device plus the
 panvk_physical_device_init_kbase() bring-up (sysfs helpers, kmod
 creation, GPU model/arch gating incl. v11, kbase sync helpers).
 Caller (instance enumeration) comes in 005.
Source/Reference: https://github.com/wonderkast02/panvk-g720-kbase-csf
 branch android-candidate-beta-1.9.4
 (src/panfrost/vulkan/panvk_physical_device.[hc]).
 .h taken whole (pure addition). .c keeps only hunks containing kbase:
 pure additions, no removals, no arch-list changes (v11 kept).
Tested GPU: none yet (compile-level).
Tested Kbase UAPI: n/a.
Mesa base range: 26.3.0-devel (pinned 5a07217f).
Dependencies: 003 (kbase_kmod backend + create_with_driver).
Validation test: build with kbase; DRM enumeration unchanged.

"""
open('patches/kbase-common/004-physical-device-kbase.patch', 'w').write(
    header + ''.join(h) + ''.join(c))
print('wrote 004')
