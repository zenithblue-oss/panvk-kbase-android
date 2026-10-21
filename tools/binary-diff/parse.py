import sys, re, struct, json, pickle

def elf_sections(path):
    d = open(path, 'rb').read()
    shoff, = struct.unpack_from('<Q', d, 0x28)
    shentsize, shnum, shstrndx = struct.unpack_from('<HHH', d, 0x3a)
    secs = []
    for i in range(shnum):
        name, typ, flags, addr, off, size = struct.unpack_from('<IIQQQQ', d, shoff + i * shentsize)
        secs.append([name, typ, addr, off, size])
    so = secs[shstrndx][3]
    out = {}
    for s in secs:
        n = d[so + s[0]: d.index(b'\0', so + s[0])].decode()
        out[n] = (s[2], s[3], s[4])
    return d, out

def read_cstr(d, secs, va):
    for n in ('.rodata',):
        if n not in secs: continue
        a, o, sz = secs[n]
        if a <= va < a + sz:
            off = o + va - a
            e = d.find(b'\0', off, off + 400)
            if e < 0: return None
            s = d[off:e]
            if len(s) >= 3 and all(32 <= c < 127 or c in (9, 10) for c in s):
                return s.decode()
    return None

line_re = re.compile(r'^\s*([0-9a-f]+):\s+(\S+)\s*(.*)$')
lab_re = re.compile(r'^([0-9a-f]+) <(.*)>:$')

def parse(sfile, elf, funcsjson, out):
    d, secs = elf_sections(elf)
    ins = []
    labels = {}
    for line in open(sfile):
        m = line_re.match(line)
        if m:
            ops = m.group(3)
            ins.append((int(m.group(1), 16), m.group(2), ops))
            continue
        m = lab_re.match(line.strip())
        if m:
            labels[int(m.group(1), 16)] = m.group(2)
    funcs = json.load(open(funcsjson))
    starts = sorted(set(f['start'] for f in funcs))
    # add symbol labels as boundaries too (for unstripped)
    idx = {a: i for i, (a, _, _) in enumerate(ins)}
    res = []
    fsizes = {f['start']: f['size'] for f in funcs}
    for st in starts:
        if st not in idx: continue
        i = idx[st]
        end = st + fsizes[st]
        body = []
        while i < len(ins) and ins[i][0] < end:
            body.append(ins[i]); i += 1
        # resolve adrp/add string refs
        strs = []
        pages = {}
        for a, mn, ops in body:
            if mn == 'adrp':
                r, v = ops.split(',')[0], ops.split(',')[1].strip().split()[0]
                pages[r] = int(v, 16)
            elif mn in ('add', 'ldr') and pages:
                p = [x.strip() for x in ops.split(',')]
                if mn == 'add' and len(p) >= 3 and p[1] in pages and p[2].startswith('#'):
                    try:
                        va = pages[p[1]] + int(p[2][1:], 0)
                    except ValueError:
                        continue
                    s = read_cstr(d, secs, va)
                    if s: strs.append(s)
        res.append({'start': st, 'size': fsizes[st], 'name': labels.get(st), 'ins': body, 'strs': strs})
    pickle.dump(res, open(out, 'wb'))
    print(len(res))

if __name__ == '__main__':
    parse(*sys.argv[1:5])
