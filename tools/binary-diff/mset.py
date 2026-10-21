import pickle, sys, collections
exec(open('match.py').read().split("def sig(f)")[0].replace("open(os.environ.get('BPKL','rc5.pkl'), 'rb')","open(sys.argv[1],'rb')"))
fa = next(f for f in A if f['start'] == int(sys.argv[2],16)); fb = next(f for f in B if f['start'] == int(sys.argv[3],16))
def ms(f):
    return collections.Counter(t for t in f['toks'] if not t.endswith(' S') and not t.startswith(('mov ', 'b', 'nop')))
a, b = ms(fa), ms(fb)
print('A-only', dict(a - b)); print('B-only', dict(b - a))
