import sys, subprocess, json
nm = sys.argv[1]; so = sys.argv[2]; out = sys.argv[3]
res = {}
for l in subprocess.run([nm, '-S', '--defined-only', so], capture_output=True, text=True).stdout.splitlines():
    p = l.split()
    if len(p) >= 4 and p[2] in 'tTW' and int(p[1], 16) > 0:
        a = int(p[0], 16); s = int(p[1], 16)
        if a not in res or res[a] < s: res[a] = s
json.dump([{'start': a, 'size': s} for a, s in sorted(res.items())], open(out, 'w'))
print(len(res))
