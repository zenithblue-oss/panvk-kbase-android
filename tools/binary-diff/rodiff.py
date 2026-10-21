import sys, subprocess, struct
sys.path.insert(0, '.')
from parse import elf_sections
NM = '/opt/android-sdk/ndk/29.0.14206865/toolchains/llvm/prebuilt/linux-aarch64/bin/llvm-nm'
da, sa = elf_sections('/home/flux/build-out/rc4/libvulkan_panfrost-android-aarch64.so')
db, sb = elf_sections(sys.argv[1])
ra = da[sa['.rodata'][1]: sa['.rodata'][1] + sa['.rodata'][2]]
syms = []
for l in subprocess.run([NM, '-S', '--defined-only', sys.argv[1]], capture_output=True, text=True).stdout.splitlines():
    p = l.split()
    if len(p) >= 4 and p[2] in 'rRdD' and int(p[1], 16) >= 8:
        syms.append((int(p[0], 16), int(p[1], 16), p[3]))
miss = 0; tot = 0
for sec in ('.rodata',):
    a0, o0, sz = sb[sec]
    for va, size, name in sorted(set(syms)):
        if not (a0 <= va < a0 + sz): continue
        tot += 1
        blob = db[o0 + va - a0: o0 + va - a0 + size]
        if ra.find(blob) < 0:
            miss += 1
            # find best: does a prefix exist?
            print('DIFF %#x %6d %s' % (va, size, name))
print('total', tot, 'diff', miss)
