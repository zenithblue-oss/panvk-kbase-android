#!/usr/bin/env python3
"""Self-check: scripts/validate-package.sh beta+ gates. No test framework."""
import hashlib, json, subprocess, sys, tempfile, zipfile, pathlib

ROOT = pathlib.Path(__file__).resolve().parents[1]
VALIDATE = ROOT / "scripts" / "validate-package.sh"
PIN = json.loads((ROOT / "sources.lock").read_text())["mesaCommit"]
PSID = "sha256:" + "ab" * 32
SO = b"fake-so-bytes\n"

def digest(b):
    return hashlib.sha256(b).hexdigest()

def write_zip(path, files):
    with zipfile.ZipFile(path, "w") as z:
        for name, data in files.items():
            z.writestr(name, data)

def run_val(zpath):
    p = subprocess.run([str(VALIDATE), str(zpath)], capture_output=True, text=True)
    return p.returncode, p.stdout + p.stderr

def android_ok(**overrides):
    sha = digest(SO)
    meta = {
        "schemaVersion": 1, "name": "PanVK", "libraryName": "libvulkan_panfrost.so",
        "abi": "android-aarch64-bionic", "backend": "panvk-kbase",
        "profile": "g615-v11-csf", "sourceCommit": PIN, "patchSeriesId": PSID,
        "packageVersion": "0.1.0-beta.3", "minApi": 35, "prerelease": True,
        "published": False, "bcCompatibility": {"included": False},
    }
    meta.update(overrides.get("meta", {}))
    man = {"profile": "g615-v11-csf", "abi": "android-aarch64-bionic", "binarySha256": sha}
    man.update(overrides.get("man", {}))
    valid = {
        "android": {
            "enumeration": "pass", "deviceCreate": "pass", "compute10": "pass",
            "offscreen": "pass", "ahb": "pass", "vulkanSurface": "pass",
            "swapchain300": "pass", "coldSecondLaunch": "pass",
        }
    }
    valid.update(overrides.get("valid", {}))
    files = {
        "libvulkan_panfrost.so": SO,
        "meta.json": json.dumps(meta),
        "MANIFEST.json": json.dumps(man),
        "SOURCE.json": json.dumps({"mesaCommit": PIN, "patchSeriesId": PSID}),
        "VALIDATION.json": json.dumps(valid),
        "SHA256SUMS.txt": f"{sha}  libvulkan_panfrost.so\n",
        "runtime-feature-matrix.json": json.dumps({"gpuId": "0xb8a31030"}),
        "extension-gap.json": json.dumps({"summary": {"runtimeExposed": 194}}),
    }
    files.update(overrides.get("files", {}))
    for k in overrides.get("drop", ()):
        files.pop(k, None)
    return files

def glibc_ok():
    sha = digest(SO)
    return {
        "libvulkan_panfrost.so": SO,
        "panfrost_icd.aarch64.json": json.dumps({"ICD": {"library_path": "libvulkan_panfrost.so"}}),
        "meta.json": json.dumps({
            "schemaVersion": 1, "name": "PanVK", "libraryName": "libvulkan_panfrost.so",
            "abi": "linux-aarch64-glibc", "backend": "panvk-kbase",
            "profile": "g615-v11-csf", "sourceCommit": PIN, "patchSeriesId": PSID,
            "packageVersion": "0.1.0-beta.3", "prerelease": True,
            "published": False, "bcCompatibility": {"included": False},
        }),
        "MANIFEST.json": json.dumps({"profile": "g615-v11-csf", "abi": "linux-aarch64-glibc", "binarySha256": sha}),
        "SOURCE.json": json.dumps({"mesaCommit": PIN, "patchSeriesId": PSID}),
        "VALIDATION.json": json.dumps({"glibc": {"enumeration": "pass", "compute10": "pass", "offscreen": "pass"}}),
        "SHA256SUMS.txt": f"{sha}  libvulkan_panfrost.so\n",
        "runtime-feature-matrix.json": json.dumps({"gpuId": "0xb8a31030"}),
        "extension-gap.json": json.dumps({"summary": {"runtimeExposed": 194}}),
    }

def expect(name, files, want_ok):
    with tempfile.TemporaryDirectory() as td:
        z = pathlib.Path(td) / "p.zip"
        write_zip(z, files)
        rc, out = run_val(z)
        ok = rc == 0
        if ok != want_ok:
            print(f"FAIL {name}: rc={rc} want_ok={want_ok}\n{out}")
            return 1
        print(f"OK {name} rc={rc}")
        return 0

def main():
    fails = 0
    fails += expect("android-beta-pass", android_ok(), True)
    fails += expect("glibc-beta-pass", glibc_ok(), True)
    fails += expect("missing-validation", android_ok(drop=("VALIDATION.json",)), False)
    fails += expect("missing-matrix", android_ok(drop=("runtime-feature-matrix.json",)), False)
    fails += expect("pending-psid", android_ok(meta={"patchSeriesId": "PENDING-P3-REBASE"}), False)
    fails += expect("wrong-commit", android_ok(meta={"sourceCommit": "deadbeef" * 5}), False)
    fails += expect("untested-gate", android_ok(valid={"android": {
        "enumeration": "untested", "deviceCreate": "pass", "compute10": "pass",
        "offscreen": "pass", "ahb": "pass", "vulkanSurface": "pass",
        "swapchain300": "pass", "coldSecondLaunch": "pass",
    }}), False)
    fails += expect("minapi-29", android_ok(meta={"minApi": 29}), False)
    bad_sha = android_ok()
    bad_sha["SHA256SUMS.txt"] = ("0" * 64) + "  libvulkan_panfrost.so\n"
    fails += expect("sha-mismatch", bad_sha, False)
    if fails:
        print(f"SELFCHECK FAIL {fails}")
        return 1
    print("SELFCHECK PASS")
    return 0

if __name__ == "__main__":
    sys.exit(main())
