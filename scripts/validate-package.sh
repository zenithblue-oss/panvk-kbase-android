#!/bin/sh
# validate-package.sh — check canonical ZIP layout (flat, no ABI mixing)
# Usage: ./scripts/validate-package.sh <package.zip>
set -eu
Z="$1"; [ -f "$Z" ] || { echo "missing $Z" >&2; exit 1; }
unzip -l "$Z"
python3 - "$Z" <<'EOF'
import sys, zipfile, json
z = zipfile.ZipFile(sys.argv[1])
names = z.namelist()
assert 'libvulkan_panfrost.so' in names, 'missing libvulkan_panfrost.so'
assert 'meta.json' in names, 'missing meta.json'
assert 'MANIFEST.json' in names, 'missing MANIFEST.json'
meta = json.loads(z.read('meta.json'))
for k in ('schemaVersion','name','libraryName','abi','backend','profile','sourceCommit'):
    assert k in meta, f'meta.json missing {k}'
assert meta['libraryName'] == 'libvulkan_panfrost.so', 'canonical binary renamed!'
assert meta['abi'] in ('android-aarch64-bionic','linux-aarch64-glibc'), meta['abi']
print('PACKAGE-VALIDATION: PASS', meta['abi'], meta['profile'])
EOF
