#!/usr/bin/env python3
"""Turn a panvk BC decoder shader into C++ for glsl_shim.hpp.
usage: glsl2cpp.py shader.comp namespace [DEFINE ...] > out.cpp
Spec constants become namespace globals, buffer_reference blocks become
bufref<T> typedefs, the push constant block a global struct, main()
shader_main()."""
import re, sys

src, ns, defs = sys.argv[1], sys.argv[2], sys.argv[3:]
s = open(src).read()

def match_close(s, i, o='(', c=')'):
    d = 0
    for j in range(i, len(s)):
        if s[j] == o: d += 1
        elif s[j] == c:
            d -= 1
            if d == 0: return j
    raise SystemExit('unbalanced at %d' % i)

out = []
i = 0
while True:
    m = re.compile(r'^layout\s*\(', re.M).search(s, i)
    if not m:
        out.append(s[i:]); break
    out.append(s[i:m.start()])
    pe = match_close(s, m.end() - 1)
    q = s[m.start():pe + 1]
    # statement end: ';' at brace depth 0
    j, d = pe + 1, 0
    while True:
        if s[j] == '{': d += 1
        elif s[j] == '}': d -= 1
        elif s[j] == ';' and d == 0: break
        j += 1
    body = s[pe + 1:j]
    if 'constant_id' in q:
        out.append(re.sub(r'^\s*const\s+', '', body) + ';')
    elif 'buffer_reference' in q:
        mm = re.search(r'buffer\s+(\w+)\s*\{\s*(\w+)\s+\w+\[\]', body)
        out.append('typedef bufref<%s> %s;' % (mm.group(2), mm.group(1)))
    elif 'push_constant' in q:
        mm = re.search(r'uniform\s+(\w+)\s*(\{.*\})\s*(\w+)\s*$', body, re.S)
        out.append('struct %s %s; %s %s;' % (mm.group(1), mm.group(2), mm.group(1), mm.group(3)))
    # local_size / outputs: dropped
    i = j + 1
s = ''.join(out)

s = re.sub(r'^#version.*$|^#extension.*$', '', s, flags=re.M)
# T[](...) array initialisers
while True:
    m = re.search(r'\b\w+\[\]\(', s)
    if not m: break
    e = match_close(s, m.end() - 1)
    s = s[:m.start()] + '{' + s[m.end():e] + '}' + s[e + 1:]
s = re.sub(r'\b(?:out|inout)\s+(\w+)\s+(\w+)', r'\1 &\2', s)
sw = {'rgb': '.rgb_()', 'agbr': '.sw_(3, 1, 2, 0)', 'rabg': '.sw_(0, 3, 2, 1)',
      'rgab': '.sw_(0, 1, 3, 2)', 'zw': '.zw_()'}
s = re.sub(r'\.(rgb|agbr|rabg|rgab|zw)\b', lambda m: sw[m.group(1)], s)
comp = {'x': 0, 'y': 1, 'z': 2, 'w': 3, 'r': 0, 'g': 1, 'b': 2, 'a': 3}
s = re.sub(r'(?<=[\w\)\]])\.([xyzwrgba])\b', lambda m: '[%d]' % comp[m.group(1)], s)
s = s.replace('rgba_result / 255.0', 'vec4(rgba_result) / 255.0f')
s = re.sub(r'\bvoid\s+main\s*\(\)', 'void shader_main()', s)

print('#include "glsl_shim.hpp"')
print('namespace %s {' % ns)
for d in defs:
    k, _, v = d.partition('=')
    print('#define %s %s' % (k, v or '1'))
print('#undef BITEXTRACT_H_')
print(s)
for d in defs:
    print('#undef %s' % d.partition('=')[0])
print('} /* namespace %s */' % ns)
