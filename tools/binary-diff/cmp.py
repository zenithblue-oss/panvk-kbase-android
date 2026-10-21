import pickle, collections, difflib, sys, json
exec(open('match.py').read().split("def sig(f)")[0])  # reuse feats
P = pickle.load(open(os.environ.get('PAIRS','pairs.pkl'), 'rb'))
mp = dict(P)
rows = []
for a, b in P:
    fa, fb = A[a], B[b]
    exact = fa['toks'] == fb['toks']
    ca = collections.Counter(mp.get(c, ('?', c)) if isinstance(c, int) else c for c in fa['calls'])
    cb = collections.Counter(c for c in fb['calls'])
    cdiff = (ca - cb) + (cb - ca)
    kA = set(k for k in fa['consts'] if abs(k) > 8); kB = set(k for k in fb['consts'] if abs(k) > 8)
    oA = set(fa['offs']); oB = set(fb['offs'])
    sA = collections.Counter(fa['strs']); sB = collections.Counter(fb['strs'])
    if exact:
        r = 1.0
    else:
        sm = difflib.SequenceMatcher(None, fa['toks'], fb['toks'], autojunk=False) if len(fa['toks']) * len(fb['toks']) < 4e7 else None
        r = sm.ratio() if sm else -1
    rows.append(dict(a=fa['start'], b=fb['start'], name=fb['name'], sa=fa['size'], sb=fb['size'], r=round(r, 4),
                     nbrA=fa['nbr'], nbrB=fb['nbr'],
                     kOnlyA=sorted(kA - kB), kOnlyB=sorted(kB - kA),
                     oOnlyA=sorted(oA - oB), oOnlyB=sorted(oB - oA),
                     callsOnlyA=[str(k) for k in (ca - cb)], callsOnlyB=[str(B[k]['name'] if isinstance(k, int) else k) for k in (cb - ca)],
                     strOnlyA=list((sA - sB)), strOnlyB=list((sB - sA))))
json.dump(rows, open(os.environ.get('CMP','cmp.json'), 'w'))
print(len(rows), sum(1 for r in rows if r['r'] == 1.0))
