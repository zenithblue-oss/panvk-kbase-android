import re,sys,glob,collections,statistics as st
W=sys.argv[1]; C=['gs_layer','tex_rgba','msaa_sshade','msaa_a2one','pg_query','ref','msaa_ref']
d=collections.defaultdict(lambda: collections.defaultdict(list))
for f in sorted(glob.glob(W+'/wl-r*.txt')):
    k=re.sub(r'wl-r\d+-','',f.split('/')[-1])[:-4]
    t=open(f,errors='ignore').read()
    for c,v in re.findall(r'workload_perf case=(\w+) ms=[\d.]+ us_op=([\d.]+)',t):
        d[c][k].append(float(v))
keys=sorted({k for c in d for k in d[c]})
print('case | '+' | '.join(keys))
for c in C:
    print(c+' | '+' | '.join(('/'.join('%g'%x for x in d[c][k])+' med %g'%st.median(d[c][k])) if d[c][k] else '-' for k in keys))
