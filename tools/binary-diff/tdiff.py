import pickle, sys, difflib, re, os
exec(open('match.py').read().split("def sig(f)")[0].replace("open(os.environ.get('BPKL','rc5.pkl'), 'rb')","open(sys.argv[1],'rb')"))
a0 = int(sys.argv[2], 16); b0 = int(sys.argv[3], 16)
fa = next(f for f in A if f['start'] == a0); fb = next(f for f in B if f['start'] == b0)
sm = difflib.SequenceMatcher(None, fa['toks'], fb['toks'], autojunk=False)
for tag, i1, i2, j1, j2 in sm.get_opcodes():
    if tag == 'equal': continue
    print('----', tag)
    for k in range(i1, i2): print(' A-', hex(fa['ins'][k][0]), fa['ins'][k][1], fa['ins'][k][2])
    for k in range(j1, j2): print(' B+', hex(fb['ins'][k][0]), fb['ins'][k][1], fb['ins'][k][2])
