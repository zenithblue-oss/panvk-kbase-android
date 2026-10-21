import pickle, re, sys, bisect, difflib, collections, json, os

A = pickle.load(open('rc4.pkl', 'rb'))
B = pickle.load(open(os.environ.get('BPKL','rc5.pkl'), 'rb'))

BR = {'b.eq','b.ne','b.lt','b.le','b.gt','b.ge','b.lo','b.ls','b.hi','b.hs','b.mi','b.pl','b.vs','b.vc','b.cc','b.cs','cbz','cbnz','tbz','tbnz'}
imm_re = re.compile(r'#(-?0x[0-9a-f]+|-?\d+)')
reg_re = re.compile(r'\b([xwvqdsbh]\d+|[xw]zr|sp)\b')

def feats(F):
    starts = [f['start'] for f in F]
    for i, f in enumerate(F):
        f['i'] = i
        calls = []; consts = collections.Counter(); offs = collections.Counter(); nbr = 0
        toks = []
        pregs = set()
        for a, mn, ops in f['ins']:
            o0 = ops.split(',')[0].strip()
            if mn == 'adrp':
                pregs.add(o0.replace('w', 'x')); toks.append('adrp'); continue
            parts = [x.strip(' []!') for x in ops.split(',')]
            if len(parts) >= 2 and parts[1].replace('w','x') in pregs and (mn == 'add' or mn.startswith(('ld', 'st'))):
                toks.append(mn + ' PO')
                if mn == 'add' or mn.startswith('ld'):
                    if o0.replace('w','x') != parts[1].replace('w','x'): pregs.discard(o0.replace('w','x'))
                continue
            pregs.discard(o0.replace('w', 'x'))
            if mn in ('bl', 'b') and not ops.startswith('x'):
                t = int(ops.split()[0], 16)
                if mn == 'b' and f['start'] <= t < f['start'] + f['size']:
                    toks.append('b'); continue
                j = bisect.bisect_right(starts, t) - 1
                calls.append(j if j >= 0 and starts[j] == t else ('plt', ops.split('<')[-1] if '<' in ops else t))
                toks.append(mn + ' CALL'); continue
            if mn in BR:
                nbr += 1
                o = ops.split(',')
                if mn in ('tbz', 'tbnz'): toks.append(mn + o[1].strip())
                else: toks.append(mn)
                continue
            if mn == 'adrp':
                toks.append('adrp'); continue
            ims = imm_re.findall(ops)
            if mn.startswith(('ld', 'st')):
                if '[sp' not in ops and '[x29' not in ops:
                    for v in ims: offs[int(v, 0)] += 1
                toks.append(mn + ' ' + ' '.join(ims) if '[sp' not in ops and '[x29' not in ops else mn + ' S')
                continue
            if mn in ('add', 'sub') and ('sp' in ops.split(',')[0] or ', sp' in ops):
                toks.append(mn + ' S'); continue
            for v in ims:
                consts[int(v, 0)] += 1
            toks.append(mn + ' ' + ' '.join(ims))
        f['calls'] = calls; f['consts'] = consts; f['offs'] = offs; f['nbr'] = nbr; f['toks'] = toks
feats(A); feats(B)

def sig(f):
    return tuple(sorted(set(f['strs']))) if f['strs'] else None
ca = collections.Counter(sig(f) for f in A); cb = collections.Counter(sig(f) for f in B)
ia = {sig(f): f['i'] for f in A if sig(f) and ca[sig(f)] == 1}
ib = {sig(f): f['i'] for f in B if sig(f) and cb[sig(f)] == 1}

def lis(anch):
    tails = []; tidx = []; prev = [None] * len(anch)
    for k, (x, y) in enumerate(anch):
        p = bs.bisect_left(tails, y)
        if p == len(tails): tails.append(y); tidx.append(k)
        else: tails[p] = y; tidx[p] = k
        prev[k] = tidx[p - 1] if p > 0 else None
    seq = []; k = tidx[-1] if tidx else None
    while k is not None: seq.append(anch[k]); k = prev[k]
    seq.reverse(); return seq
import bisect as bs

def fsim(a, b):
    if a['nbr'] == b['nbr'] and abs(a['size'] - b['size']) <= max(16, a['size'] // 50): return 1.0
    return 1 - abs(a['size'] - b['size']) / max(a['size'], b['size'])

def fill(seq):
    pairs = list(seq)
    bounds = [(-1, -1)] + seq + [(len(A), len(B))]
    for (x0, y0), (x1, y1) in zip(bounds, bounds[1:]):
        ga = list(range(x0 + 1, x1)); gb = list(range(y0 + 1, y1))
        if not ga or not gb: continue
        if len(ga) == len(gb):
            pairs += list(zip(ga, gb)); continue
        if len(ga) * len(gb) > 12_000_000: continue
        n, m = len(ga), len(gb)
        S = [[0.0] * (m + 1) for _ in range(n + 1)]
        for i in range(1, n + 1):
            for j in range(1, m + 1):
                s = fsim(A[ga[i-1]], B[gb[j-1]])
                S[i][j] = max(S[i-1][j], S[i][j-1], S[i-1][j-1] + (s if s > 0.6 else -1))
        i, j = n, m
        while i > 0 and j > 0:
            s = fsim(A[ga[i-1]], B[gb[j-1]])
            if s > 0.6 and S[i][j] == S[i-1][j-1] + s:
                pairs.append((ga[i-1], gb[j-1])); i -= 1; j -= 1
            elif S[i][j] == S[i-1][j]: i -= 1
            else: j -= 1
    return sorted(pairs)

anch = set((ia[s], ib[s]) for s in ia if s in ib)
def hsig(f):
    if len(f['toks']) < 12: return None
    return hash(tuple(f['toks']))
ha = collections.Counter(hsig(f) for f in A); hb = collections.Counter(hsig(f) for f in B)
ja = {hsig(f): f['i'] for f in A if hsig(f) and ha[hsig(f)] == 1}
jb = {hsig(f): f['i'] for f in B if hsig(f) and hb[hsig(f)] == 1}
anch |= set((ja[s], jb[s]) for s in ja if s in jb)
print('exact anchors', sum(1 for s in ja if s in jb), file=sys.stderr)
for it in range(4):
    seq = lis(sorted(anch))
    # propagate via callees of confident anchors
    new = set(seq)
    for x, y in seq:
        cx = [c for c in A[x]['calls'] if isinstance(c, int)]
        cy = [c for c in B[y]['calls'] if isinstance(c, int)]
        if len(cx) == len(cy):
            new |= set(zip(cx, cy))
    # keep only consistent (one-to-one)
    cxa = collections.Counter(a for a, b in new); cxb = collections.Counter(b for a, b in new)
    anch = set(p for p in new if cxa[p[0]] == 1 and cxb[p[1]] == 1)
    print('iter', it, len(seq), len(anch), file=sys.stderr)
seq = lis(sorted(anch))
pairs = fill(seq)
print('anchors', len(seq), 'pairs', len(pairs), file=sys.stderr)
pickle.dump(pairs, open(os.environ.get('PAIRS','pairs.pkl'), 'wb'))
