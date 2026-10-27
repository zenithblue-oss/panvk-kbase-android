#!/usr/bin/env python3
"""driver_identity.py: the identity a libvulkan_panfrost.so reports, for packaging and release names.

Trees with patches/jm-v7-wip 0120 (was staging-115/int 0001; p115-int and later) report driverInfo like an upstream Mesa
development build, "Mesa <VERSION> (git-<sha10>)", e.g. "Mesa 26.3.0-devel (git-6a5d4e2756)"
(PACKAGE_VERSION of the Mesa base, MESA_GIT_SHA1 of the patched tree). RC4 up to rc5g-114 reported
"PanVK-kbase <release> (Mesa <VERSION> (git-<sha10>))" (csf-v11/099, PANVK_KBASE_RELEASE).

  upstream: displayName "Mesa PanVK 26.3.0-devel", packageName "Mesa-PanVK-26.3.0-devel-6a5d4e27"
            (meta.json "name": also the launcher install directory, so no spaces), sha8 "6a5d4e27".
  legacy:   displayName "PanVK-kbase RC4", packageName "PanVK-kbase-RC4"; packaging scripts keep their old
            names for these builds so released zips can be rebuilt unchanged.

Usage: driver_identity.py <libvulkan_panfrost.so>             one JSON object ("null" if no driverInfo found)
       driver_identity.py --field <key> <libvulkan_panfrost.so> one field, empty if absent
"""
import json
import re
import sys

# Whole NUL-terminated string: the "# panvk-fps Mesa ..." fps-log header contains the same text but does not
# start at a NUL.
UPSTREAM = re.compile(rb"\x00(Mesa (\d+\.\d+\.\d+[-.0-9A-Za-z]*) \(git-([0-9a-f]{7,40})\))\x00")
LEGACY = re.compile(rb"PanVK-kbase ([^\x00]*?) \(Mesa (\d+\.\d+\.\d+[-.0-9A-Za-z]*) \(git-([0-9a-f]{7,40})\)\)\x00")


def identity(data):
    m = UPSTREAM.search(data)
    if m:
        info, ver, sha = (g.decode() for g in m.groups())
        return {"style": "upstream", "driverInfo": info, "mesaVersion": ver, "gitSha": sha, "sha8": sha[:8],
                "release": None, "displayName": f"Mesa PanVK {ver}",
                "packageName": f"Mesa-PanVK-{ver}-{sha[:8]}"}
    m = LEGACY.search(data)
    if m:
        rel, ver, sha = (g.decode() for g in m.groups())
        return {"style": "legacy", "driverInfo": f"PanVK-kbase {rel} (Mesa {ver} (git-{sha}))",
                "mesaVersion": ver, "gitSha": sha, "sha8": sha[:8], "release": rel,
                "displayName": f"PanVK-kbase {rel}", "packageName": f"PanVK-kbase-{rel}"}
    return None


def identity_of(path):
    with open(path, "rb") as f:
        return identity(f.read())


def main(argv):
    field = None
    if len(argv) == 3 and argv[0] == "--field":
        field, path = argv[1], argv[2]
    elif len(argv) == 1:
        path = argv[0]
    else:
        print(__doc__.strip().splitlines()[-2].strip(), file=sys.stderr)
        return 2
    ident = identity_of(path)
    if field is None:
        print(json.dumps(ident, indent=2))
    else:
        v = (ident or {}).get(field)
        print("" if v is None else v)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
