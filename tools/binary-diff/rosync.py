import sys
sys.path.insert(0, '.')
from parse import elf_sections
da, sa = elf_sections(sys.argv[1]); db, sb = elf_sections(sys.argv[2])
sec = sys.argv[3] if len(sys.argv) > 3 else '.rodata'
A = da[sa[sec][1]: sa[sec][1] + sa[sec][2]]
B = db[sb[sec][1]: sb[sec][1] + sb[sec][2]]
C = 32
i = 0; d = 0; bad = []
while i + C <= len(B):
    j = i + d
    if A[j:j + C] == B[i:i + C]:
        i += C; continue
    # find resync
    found = None
    for k in range(i, min(len(B) - C, i + 8192), 4):
        w = B[k:k + C]
        if w.count(w[:1]) == C: continue
        p = A.find(w, max(0, k + d - 8192), k + d + 8192)
        if p >= 0:
            found = (k, p - k); break
    if not found:
        bad.append((i, 8192, d)); i += 8192; continue
    k, nd = found
    if k > i or nd != d:
        bad.append((i, k - i, nd - d))
    i = k; d = nd
va = sb[sec][0]
for off, ln, dd in bad:
    s = B[off:off + ln]
    pr = ''.join(chr(c) if 32 <= c < 127 else '.' for c in s[:80])
    print('%#x len=%d dshift=%d  %s' % (va + off, ln, dd, pr))
print('regions', len(bad), file=sys.stderr)
