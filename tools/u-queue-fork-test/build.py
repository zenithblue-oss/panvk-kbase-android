#!/usr/bin/env python3
"""Build tools/u-queue-fork-test/test.c and fork_cost.c against a Mesa tree (see test.c).

usage: build.py <host Mesa build dir B> <Mesa tree M> <out dir>
Recompiles u_queue.c, disk_cache.c and disk_cache_os.c from M with B's flags
(struct util_queue is embedded in struct disk_cache) and links the rest of
libmesa_util from B.
"""
import json, os, shlex, subprocess, sys

b, m, out = (os.path.abspath(p) for p in sys.argv[1:4])
os.makedirs(out, exist_ok=True)
cmds = json.load(open(os.path.join(b, "compile_commands.json")))
objs = []
defs = []  # u_queue.c's -D flags, so u_queue.h inlines match (futex fences)
for f in ("u_queue", "disk_cache", "disk_cache_os"):
    ent = next(e for e in cmds if e["file"].endswith("/src/util/%s.c" % f))
    src_root = ent["file"][: -len("/src/util/%s.c" % f)]
    args = shlex.split(ent["command"])
    keep, skip = [], 0
    for a in args:
        if skip:
            skip -= 1
            continue
        if a in ("-MQ", "-MF", "-o", "-c"):
            skip = 1
            continue
        if a == "-MD":
            continue
        keep.append(a.replace(src_root, m))
        if f == "u_queue" and a.startswith("-D"):
            defs.append(a)
    obj = os.path.join(out, f + ".o")
    subprocess.check_call(keep + ["-c", os.path.join(m, "src/util/%s.c" % f), "-o", obj], cwd=b)
    objs.append(obj)
here = os.path.dirname(os.path.abspath(__file__))
for prog in ("test", "fork_cost"):
    subprocess.check_call(
        ["cc", "-O1", "-g", "-DENABLE_SHADER_CACHE", "-D_GNU_SOURCE"] + defs + ["-I" + m + "/src", "-I" + m + "/include", "-I" + m + "/src/util",
         "-I" + b + "/src", os.path.join(here, prog + ".c")] + objs +
        [b + "/src/util/libmesa_util.a", b + "/src/util/libmesa_util_simd.a",
         b + "/src/util/blake3/libblake3.a", b + "/src/c11/impl/libmesa_util_c11.a",
         "-lz", "-lm", "-pthread", "-o", os.path.join(out, prog)])
    print("built", os.path.join(out, prog))
