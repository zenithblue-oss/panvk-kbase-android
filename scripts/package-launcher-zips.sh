#!/bin/sh
# package-launcher-zips.sh — build the six Android launcher driver zips from a validated .so
# (adrenotools, winlator, bannerlator, cmod, ludashi, gamehub). Layouts reproduce the RC4 zips.
# Usage: ./scripts/package-launcher-zips.sh --so path/libvulkan_panfrost.so [--version v0.1.0-rc4]
#        [--label RC4] [--tag g615-v11-csf] [--mesa-sha 5a07217f] [--outdir dist]
set -eu
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SO=""; VER="v0.0.0-dev"; LABEL=""; TAG="g615-v11-csf"; OUTDIR="$ROOT/dist"; MESA8=""
while [ $# -gt 0 ]; do case "$1" in
  --so) SO="$2"; shift 2;; --version) VER="$2"; shift 2;; --label) LABEL="$2"; shift 2;;
  --tag) TAG="$2"; shift 2;; --outdir) OUTDIR="$2"; shift 2;; --mesa-sha) MESA8="$2"; shift 2;;
  *) echo "unknown $1" >&2; exit 2;; esac; done
[ -f "$SO" ] || { echo "--so <file> required" >&2; exit 2; }
if [ -z "$MESA8" ]; then
  MESA8="$(python3 -c "import json;print(json.load(open('$ROOT/sources.lock'))['mesaCommit'][:8])")"
fi
if [ -z "$LABEL" ]; then
  LABEL="$(echo "${VER#v0.1.0-}" | tr '[:lower:]' '[:upper:]')"
fi
MESA10="$(python3 -c "import json;print(json.load(open('$ROOT/sources.lock'))['mesaCommit'][:10])")"; export MESA10
mkdir -p "$OUTDIR"
python3 - "$SO" "$VER" "$LABEL" "$TAG" "$MESA8" "$OUTDIR" <<'EOF'
import sys, json, hashlib, zipfile, pathlib
so, ver, label, tag, mesa8, outdir = sys.argv[1:]
import os
mesa10 = os.environ.get("MESA10") or mesa8
so = pathlib.Path(so); outdir = pathlib.Path(outdir)
data = so.read_bytes()
sha = hashlib.sha256(data).hexdigest()
pkgver = ver.removeprefix("v")
name = f"PanVK-kbase-{label}"
drvver = f"PanVK-kbase {label} (Mesa 26.3.0-devel git-{mesa10})"
base = f"PanVK (Mesa) Vulkan driver for Mali v10+ CSF via kbase, {label} build."
# launcher -> (package id for abs path, minApi, description tail, libname, wrapper-style extras)
L = {
 "adrenotools": dict(minApi=29, desc="Generic adrenotools package.", lib="libvulkan_panfrost.so", app=None),
 "winlator":    dict(minApi=29, desc="Winlator official package (adrenotools layout).", lib="libvulkan_panfrost.so", app=None),
 "bannerlator": dict(minApi=28, app="com.winlator.banner", lib="libvulkan_panfrost.so"),
 "cmod":        dict(minApi=28, app="com.winlator.cmod", lib="libvulkan_panfrost.so"),
 "ludashi":     dict(minApi=28, app="com.winlator.cmod", lib="libvulkan_panfrost.so"),
 "gamehub":     dict(minApi=29, app=None, lib="libvulkan_freedreno.so",
                     desc="GameHub/BannerHub package: the PanVK library is shipped as libvulkan_freedreno.so (name hard-coded by the loader)."),
}
def icd(path):
    return json.dumps({"file_format_version":"1.0.0","ICD":{"library_path":path,"api_version":"1.4.363"}}, indent=2) + "\n"
for key, c in L.items():
    app = c.get("app")
    if app:
        desc = f"{base} Package for {key} ({app}); see README-{key}.txt for the direct-ICD (no wrapper) setting."
    else:
        desc = f"{base} {c['desc']}"
    meta = {"schemaVersion":1,"name":name,"description":desc,"author":"panvk-kbase-android",
            "packageVersion":pkgver,"vendor":"Mesa","driverVersion":drvver,"minApi":c["minApi"],
            "libraryName":c["lib"]}
    files = {c["lib"]: data,
             "meta.json": (json.dumps(meta, indent=2) + "\n").encode(),
             "SHA256SUMS.txt": f"{sha}  {c['lib']}\n".encode()}
    if app:
        root = f"/data/data/{app}/files/contents/adrenotools/{name}"
        files["panvk_icd.aarch64.json"] = icd("./libvulkan_panfrost.so").encode()
        files["panvk_icd_abs.aarch64.json"] = icd(f"{root}/libvulkan_panfrost.so").encode()
        files[f"README-{key}.txt"] = (
f"""PanVK-kbase {label} for {key} ({app})
Install: Adrenotools GPU Drivers > Install > pick this zip. It installs to
  {root}/
Direct load (no wrapper ICD): container > Environment, add
  VK_ICD_FILENAMES={root}/panvk_icd_abs.aarch64.json
(container env vars are applied after the app sets the wrapper ICD, so this replaces it).
If the X11 present path fails, also add MESA_VK_WSI_DEBUG=sw.
Fallback (wrapper in path): leave VK_ICD_FILENAMES unset and pick "{name}" as Graphics Driver Version.
Check: wine log / logcat shows driverInfo "PanVK-kbase {label}"; .so sha256 {sha}
""").encode()
    out = outdir / f"PanVK-Kbase-Android-{ver}-{mesa8}-{key}.zip"
    if out.exists(): out.unlink()
    with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
        for n in sorted(files):
            zi = zipfile.ZipInfo(n, (2026, 10, 17, 0, 0, 0)); zi.compress_type = zipfile.ZIP_DEFLATED
            zi.external_attr = 0o644 << 16
            z.writestr(zi, files[n])
    print("OK", out)
EOF
echo "LAUNCHER-ZIPS OK sha256=$(sha256sum "$SO" | cut -d' ' -f1)"
