#!/usr/bin/env python3
"""usage: table.py probe|play label=dir1,dir2 ... : markdown table, last label is the candidate.
Flags (!) where the candidate mean is worse than a baseline mean by > 5 % and by more than the
run-to-run spread of both."""
import re, sys, os, glob, statistics as st

def probe(d):
    f = open(os.path.join(d, 'raw/autorun.txt'), errors='replace').read()
    r = {}
    for m in re.finditer(r'RESULT (\w+) (PASS|FAIL)[^\n]*?\bms=([0-9.]+)', f):
        r[m.group(1) + ' ms'] = float(m.group(3)) if m.group(2) == 'PASS' else None
    wl = re.search(r'RESULT workload_perf [^\n]*', f)
    if wl:
        for m in re.finditer(r'case=(\w+) ms=[0-9.]+ us_op=([0-9.]+)', wl.group(0)):
            r['wl ' + m.group(1) + ' us'] = float(m.group(2))
    mem = open(os.path.join(d, 'mem.txt')).read()
    p = [int(x) for x in re.findall(r'TOTAL PSS: (\d+)', mem)]
    g = [int(x) for x in re.findall(r'Graphics: (\d+)', mem)]
    r['PSS max MB'] = max(p) / 1024 if p else None
    r['Graphics max MB'] = max(g) / 1024 if g else None
    return r

def play(d):
    r = {}
    for res in sorted(glob.glob(os.path.join(d, 'raw/*-s*/results.txt'))):
        s = re.search(r'-s(\d+)$', os.path.dirname(res)).group(1)
        for line in open(res):
            c = line.split()
            if len(c) > 10 and re.match(r'^\d\d$', c[0]) and c[3] == 'ok':
                k = f's{s} {c[0]} {c[1]} {c[2]}'
                # rc4 has no driver fps log: DXVK fps then; D3D12: no DXVK, 1 s min as low.
                fps = c[4] if c[4] != '-' else c[8]
                low = c[9] if c[9] != '-' else c[5]
                if fps != '-': r[k + ' fps'] = float(fps)
                if low != '-': r[k + ' 1%low'] = float(low)
    for mf in glob.glob(os.path.join(d, 'mem/s*.txt')):
        s = re.match(r's(\d+)', os.path.basename(mf)).group(1)
        t = open(mf).read()
        for name, rx in (('PSS MB', r'TOTAL PSS: *(\d+)'), ('Graphics MB', r'Graphics: *(\d+)')):
            v = [int(x) for x in re.findall(rx, t)]
            if v:
                k = f's{s} {name}'
                r[k] = max(r.get(k, 0), max(v) / 1024)
    return r

kind = sys.argv[1]; fn = probe if kind == 'probe' else play
cols = []
for a in sys.argv[2:]:
    lab, dirs = a.split('=')
    cols.append((lab, [fn(x) for x in dirs.split(',') if os.path.isdir(x)]))
keys = []
for _, runs in cols:
    for r in runs:
        for k in r:
            if k not in keys: keys.append(k)
higher_better = lambda k: k.endswith('fps') or k.endswith('1%low')
print('| metric | ' + ' | '.join(l for l, _ in cols) + ' | flag |')
print('|---' * (len(cols) + 2) + '|')
flags = []
for k in keys:
    vals = []
    for _, runs in cols:
        v = [r[k] for r in runs if r.get(k) is not None]
        vals.append(v)
    cell = lambda v: (f'{st.mean(v):.1f}' + (f' ±{(max(v)-min(v))/2:.1f}' if len(v) > 1 else '')) if v else '-'
    flag = ''
    c = vals[-1]
    if c:
        cm = st.mean(c); cs = (max(c) - min(c)) / 2
        for (lab, _), b in zip(cols[:-1], vals[:-1]):
            if not b: continue
            bm = st.mean(b); bs = (max(b) - min(b)) / 2
            worse = (bm - cm) if higher_better(k) else (cm - bm)
            if worse > 0.05 * abs(bm) and worse > cs + bs:
                flag += f'worse than {lab} '
    if flag: flags.append(f'{k}: {flag.strip()}')
    print(f'| {k} | ' + ' | '.join(cell(v) for v in vals) + f' | {flag.strip()} |')
print('\nFLAGS:' if flags else '\nFLAGS: none')
for f in flags: print('- ' + f)
