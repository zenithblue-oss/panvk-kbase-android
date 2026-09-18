#!/usr/bin/env python3
"""generate-release-manifest.py — MANIFEST.json + SOURCE.json + VALIDATION.json skeletons.

Usage: generate-release-manifest.py --profile g615-v11-csf --abi android-aarch64-bionic --stage <dir>
Fills build provenance; runtime VALIDATION fields are updated by device gates (P6-P13).
"""
import argparse, json, hashlib, pathlib, subprocess, datetime

ap = argparse.ArgumentParser()
ap.add_argument('--profile', required=True)
ap.add_argument('--abi', required=True)
ap.add_argument('--stage', required=True)
a = ap.parse_args()
stage = pathlib.Path(a.stage)
root = pathlib.Path(__file__).resolve().parent.parent

def sh(*cmd):
    try:
        return subprocess.check_output(cmd, text=True).strip()
    except Exception:
        return 'unknown'

mesa_dir = root / 'work' / 'mesa'
mesa_sha = sh('git', '-C', str(mesa_dir), 'rev-parse', 'HEAD')
lock = json.loads((root / 'sources.lock').read_text())
so = stage / 'libvulkan_panfrost.so'
sha = hashlib.sha256(so.read_bytes()).hexdigest() if so.exists() else 'missing'

manifest = {
    'profile': a.profile, 'abi': a.abi,
    'mesaCommit': mesa_sha, 'mesaVersion': lock.get('mesaVersion'),
    'patchSeriesId': lock.get('patchSeriesId'),
    'buildHost': sh('uname', '-a'), 'date': datetime.datetime.now(datetime.timezone.utc).isoformat(),
    'ndk': sh('ls', '/opt/android-sdk/ndk'),
    'binarySha256': sha,
}
(stage / 'MANIFEST.json').write_text(json.dumps(manifest, indent=2))
(stage / 'SOURCE.json').write_text(json.dumps(
    {'mesaRepo': lock.get('mesaRepo'), 'mesaCommit': mesa_sha,
     'referenceRepos': lock.get('referenceRepos')}, indent=2))
valid = {k: 'untested' for k in (
    'enumeration', 'deviceCreate', 'compute10', 'offscreen', 'ahb',
    'androidSurface300', 'secondRun', 'loaderApk', 'glibcVulkaninfo',
    'glibcCompute', 'glibcOffscreen')}
(stage / 'VALIDATION.json').write_text(json.dumps(valid, indent=2))
print(f"OK stage={stage} sha={sha[:16]}")
