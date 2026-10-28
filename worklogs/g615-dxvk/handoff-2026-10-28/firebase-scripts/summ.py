#!/usr/bin/env python3
"""Summarize a PanPlay FTL raw dir: per pass D3DFL/DX12 result lines, kbase errors, diag tiler lines."""
import os
import re
import sys

root = sys.argv[1]
pat_keep = re.compile(r"D3DFL: (feature|level|caps|FAIL|PASS|UNSUP|DONE)|DX12: (PASS|FAIL)|CUBE: |kbase: (CSF group|timeout on|JM atom)|"
                      r"panvk diag|tiler heaps|DEVICE_LOST|device lost|installDrivers|panvk: gpu_id")
for dirpath, dirs, files in sorted(os.walk(root)):
    dirs.sort()
    if os.path.basename(dirpath) != "session":
        continue
    pas = os.path.relpath(os.path.dirname(dirpath), root)
    print("==", pas)
    seen = set()
    for f in ("wine-run.log", "mesa-panvk.txt"):
        p = os.path.join(dirpath, f)
        if not os.path.exists(p):
            continue
        with open(p, errors="replace") as fh:
            for line in fh:
                if pat_keep.search(line):
                    s = re.sub(r"^\d\d-\d\d \d\d:\d\d:\d\d\.\d+\s+\d+\s+\d+ [A-Z] ", "", line.strip())
                    s = s[:300]
                    if s in seen:
                        continue
                    seen.add(s)
                    print("  ", s)
