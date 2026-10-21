import pickle, sys, difflib, re
A = pickle.load(open('rc4.pkl', 'rb')); B = pickle.load(open(sys.argv[1], 'rb'))
a0 = int(sys.argv[2], 16); b0 = int(sys.argv[3], 16)
fa = next(f for f in A if f['start'] == a0); fb = next(f for f in B if f['start'] == b0)
def norm(f):
    out = []
    for a, mn, ops in f['ins']:
        o = re.sub(r'0x[0-9a-f]{5,}( <[^>]*>)?', 'ADDR', ops)
        out.append(mn + ' ' + o)
    return out
na, nb = norm(fa), norm(fb)
ctx = int(sys.argv[4]) if len(sys.argv) > 4 else 3
sm = difflib.SequenceMatcher(None, na, nb, autojunk=False)
for tag, i1, i2, j1, j2 in sm.get_opcodes():
    if tag == 'equal': continue
    print('---- %s A[%#x..] B[%#x..]' % (tag, fa['ins'][i1][0] if i1 < len(fa['ins']) else 0, fb['ins'][j1][0] if j1 < len(fb['ins']) else 0))
    for k in range(max(0, i1 - ctx), i1): print('   ', hex(fa['ins'][k][0]), fa['ins'][k][1], fa['ins'][k][2])
    for k in range(i1, i2): print(' A-', hex(fa['ins'][k][0]), fa['ins'][k][1], fa['ins'][k][2])
    for k in range(j1, j2): print(' B+', hex(fb['ins'][k][0]), fb['ins'][k][1], fb['ins'][k][2])
