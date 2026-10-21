#!/bin/sh
# package-prebuilt.sh — wrap an EXISTING released libvulkan_panfrost.so in the RC3-style
# EMULATOR zip and .adpkg.zip layouts, without rebuilding.
# SOURCE.json states honestly that the binary comes from a release asset and that the
# source series is not reproducible. The EMULATOR zip carries the Android (bionic) driver.
# Usage: ./scripts/package-prebuilt.sh --so <libvulkan_panfrost.so> --icd <panfrost_icd.aarch64.json>
#        --version v0.1.0-rc4 [--suffix lowmem] [--mesa8 5a07217f] [--profile g615-v11-csf]
#        [--outdir dist] [--minapi 29]
set -eu
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SO=""; ICD=""; VER=""; SUFFIX=""; MESA8=""; PROFILE="g615-v11-csf"; OUTDIR="$ROOT/dist"; MINAPI=29
while [ $# -gt 0 ]; do case "$1" in
  --so) SO="$2"; shift 2;; --icd) ICD="$2"; shift 2;; --version) VER="$2"; shift 2;;
  --suffix) SUFFIX="$2"; shift 2;; --mesa8) MESA8="$2"; shift 2;; --profile) PROFILE="$2"; shift 2;;
  --outdir) OUTDIR="$2"; shift 2;; --minapi) MINAPI="$2"; shift 2;;
  *) echo "unknown $1" >&2; exit 2;; esac; done
[ -f "$SO" ] && [ -f "$ICD" ] && [ -n "$VER" ] || { echo "--so --icd --version required" >&2; exit 2; }
mkdir -p "$OUTDIR"
python3 - "$SO" "$ICD" "$VER" "$SUFFIX" "$MESA8" "$PROFILE" "$OUTDIR" "$MINAPI" "$ROOT" <<'EOF'
import sys, json, hashlib, zipfile, pathlib
so, icd, ver, suffix, mesa8, profile, outdir, minapi, root = sys.argv[1:]
so = pathlib.Path(so); outdir = pathlib.Path(outdir); root = pathlib.Path(root)
data = so.read_bytes(); sha = hashlib.sha256(data).hexdigest()
lock = json.loads((root / "sources.lock").read_text())
mesa = lock["mesaCommit"]; mesa8 = mesa8 or mesa[:8]
prof = json.loads((root / "profiles" / f"{profile}.json").read_text())
label = (ver.removeprefix("v").split("-",1)[1].upper() if "-" in ver else ver.removeprefix("v")) + (("-" + {"lowmem":"LM"}.get(suffix, suffix)) if suffix else "")
metaname = f"PanVK-kbase-{label}"
profile = profile.replace("g615-", "")  # user-facing id; internal build profile is only used to load profiles/*.json above
pkgver = ver.removeprefix("v") + (f"-{suffix}" if suffix else "")
tag = f"{ver}" + (f"-{suffix}" if suffix else "")
note = ("Binary taken verbatim from the published release asset libvulkan_panfrost-android-aarch64.so "
        f"(sha256 {sha}). The exact patch series used to build it is not reproducible from this repository, "
        "so patchSeriesId is not claimed. This is the Android (bionic) driver repackaged in the RC3 package layout.")
def meta(abi, desc, with_minapi):
    m = {"schemaVersion":1,"name":metaname,"author":"panvk-kbase-android","packageVersion":pkgver,
         "vendor":"Mesa","driverVersion":"Mesa 26.3.0-devel"}
    if with_minapi: m["minApi"] = int(minapi)
    m.update({"description":desc,"libraryName":"libvulkan_panfrost.so","abi":abi,"backend":"panvk-kbase",
         "kernelInterface":"mali_kbase","profile":profile,"gpuId":prof.get("gpuId","0xb8a31030"),
         "panArch":prof.get("panArch",11),"frontend":prof.get("frontend","CSF"),
         "kbaseUapi":prof.get("kbaseUapi","1.21"),"sourceCommit":mesa,
         "patchSeriesId":"not-reproducible","prerelease":True,"published":True,
         "bcCompatibility":{"included":False,"reason":"not part of this package"}})
    return m
manifest = {"profile":profile,"abi":"android-aarch64-bionic","mesaCommit":mesa,"mesaVersion":lock.get("mesaVersion"),
            "patchSeriesId":"not-reproducible","binarySha256":sha,"binarySource":"release asset, not rebuilt"}
source = {"mesaRepo":lock.get("mesaRepo"),"mesaCommit":mesa,"sourceCommit":mesa,"patchSeriesId":"not-reproducible",
          "binaryProvenance":"release-asset","note":note,"releaseStatus":{"prerelease":True,"published":True},
          "bcCompatibilityIncluded":False}
validation = {"profile":profile,"mesaCommit":mesa,"binarySha256":sha,
              "status":"not-revalidated-by-packaging","note":"This package was assembled around an already released binary; "
              "no validation gates were run as part of packaging. See the release notes and worklogs for device results."}
def jd(o): return (json.dumps(o, indent=2) + "\n").encode()
def write(out, files):
    out = pathlib.Path(out)
    if out.exists(): out.unlink()
    with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
        for n, b in files.items():
            zi = zipfile.ZipInfo(n, (2026,10,21,0,0,0)); zi.compress_type = zipfile.ZIP_DEFLATED
            zi.external_attr = 0o644 << 16; z.writestr(zi, b)
    print("OK", out, hashlib.sha256(out.read_bytes()).hexdigest())
common = {"libvulkan_panfrost.so": data}
emu = dict(common)
emu["panfrost_icd.aarch64.json"] = pathlib.Path(icd).read_bytes()
emu["meta.json"] = jd(meta("android-aarch64-bionic",
    "PanVK Kbase driver in the EMULATOR package layout. Carries the Android (bionic) build of this release, not a glibc build.", True))
for n, o in (("MANIFEST.json", manifest), ("SOURCE.json", source), ("VALIDATION.json", validation)):
    emu[n] = jd(o)
emu["SHA256SUMS.txt"] = f"{sha}  libvulkan_panfrost.so\n".encode()
base = f"PanVK-Kbase-{tag}-{mesa8}" if not suffix else f"PanVK-Kbase-{tag}-{mesa8}"
write(outdir / f"PanVK-Kbase-{tag}-{mesa8}-EMULATOR.zip", emu)
ad = dict(common)
ad["meta.json"] = jd(meta("android-aarch64-bionic",
    "Experimental PanVK Kbase driver for supported Mali devices.", True))
for n, o in (("MANIFEST.json", manifest), ("SOURCE.json", source), ("VALIDATION.json", validation)):
    ad[n] = jd(o)
ad["SHA256SUMS.txt"] = f"{sha}  libvulkan_panfrost.so\n".encode()
write(outdir / f"PanVK-Kbase-Android-{tag}-{mesa8}.adpkg.zip", ad)
EOF
