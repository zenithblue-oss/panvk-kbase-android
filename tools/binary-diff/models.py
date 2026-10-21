import sys, struct
sys.path.insert(0, '.')
from parse import elf_sections

def relas(d, secs):
    out = {}
    for n in ('.rela.dyn',):
        a, o, sz = secs[n]
        for i in range(sz // 24):
            off, info, add = struct.unpack_from('<QQq', d, o + i * 24)
            if (info & 0xffffffff) == 1027:  # R_AARCH64_RELATIVE
                out[off] = add
    return out

def va2off(secs, va):
    for n, (a, o, sz) in secs.items():
        if a and a <= va < a + sz and n not in ('.bss', '.tbss'):
            return o + va - a
    return None

def cstr(d, secs, va):
    o = va2off(secs, va)
    if o is None: return None
    return d[o:d.index(b'\0', o)].decode(errors='replace')

for path in sys.argv[1:]:
    d, secs = elf_sections(path)
    R = {}
    for n in ('.data.rel.ro', '.data'):
        a, o, sz = secs[n]
        for i in range(0, sz - 8, 8):
            v, = struct.unpack_from('<Q', d, o + i)
            if secs['.rodata'][0] <= v < secs['.rodata'][0] + secs['.rodata'][2]:
                R[a + i] = v
    # find struct entries: pointer at +8 to "Mali-..." string
    ents = []
    for off, add in R.items():
        s = cstr(d, secs, add)
        if s and s.startswith('Mali-') and (off - 8) not in [e[0] for e in ents]:
            base = off - 8
            fo = va2off(secs, base)
            prod, var = struct.unpack_from('<II', d, fo)
            if True:
                ents.append((base, prod, var, s))
    ents.sort()
    print(path)
    for base, prod, var, s in ents:
        fo = va2off(secs, base)
        rest = struct.unpack_from('<I II IIIIII BB', d, fo + 24)
        print(' %#x prod=%#x var=%d %-14s cnt=%s minrev=%#x tb=%d/%d rates=%s quirks=%s' % (base, prod, var, s, cstr(d, secs, R.get(base + 16, 0)), rest[0], rest[1], rest[2], rest[3:9], rest[9:]))
