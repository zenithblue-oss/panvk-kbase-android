#!/usr/bin/env python3
"""post115.py probe|play <arch> <model> [baseline run dir ...]: file one build 115 FTL run into the repo.

Copies the pulled data (artifacts, game_loop_results, logcat, video) from /home/flux/ftl-dl/115-<arch>-<model>-<kind>/pull
into device-runs/2026-10-26-ftl-<arch>-<model>-115[-panplay]/raw, makes screenshots (shots115.py), writes a short
results.md with the an115f.py summary, and appends a line to summary115.jsonl. Run with python3 -I (downloaded data).
"""
import glob
import json
import os
import re
import shutil
import subprocess
import sys
from collections import Counter

SP = "/tmp/claude-1000/-home-flux-repos-panvk-kbase-android/db381bd6-e816-4c4b-b498-8633cd891ac4/scratchpad"
sys.path.insert(0, SP)
import an115f  # noqa: E402

DR = "/home/flux/repos/panvk-kbase-android/worklogs/g615-dxvk/device-runs"
NAMES = {"starlte": "Galaxy S9, Mali-G72 MC18", "a31": "Galaxy A31, Mali-G52 r1 MC2", "oriole": "Pixel 6, Mali-G78 MC20",
         "panther": "Pixel 7, Mali-G710", "shiba": "Pixel 8, Mali-G715 MC7"}
kind, arch, model, bases = sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4:]
dl = f"/home/flux/ftl-dl/115-{arch}-{model}-{'probe' if kind == 'probe' else 'panplay'}"
done = json.load(open(dl + "/DONE"))
run = f"{DR}/2026-10-26-ftl-{arch}-{model}-115" + ("-panplay" if kind == "play" else "")
rdir = f"panvk-{arch}-{model}-115-{'probe' if kind == 'probe' else 'panplay'}"
# the results dir also holds the uploaded --other-files (data/, sdcard/): keep the device dir only
src = [d for d in glob.glob(f"{dl}/pull/{rdir}/*/") if os.path.basename(d.rstrip("/")).startswith(model + "-")]
assert len(src) == 1, src
dev = os.path.basename(src[0].rstrip("/"))
dst = f"{run}/raw/{dev}"
os.makedirs(dst, exist_ok=True)
for item in ("artifacts", "game_loop_results", "logcat", "video.mp4"):
    s = os.path.join(src[0], item)
    if os.path.isdir(s):
        shutil.copytree(s, os.path.join(dst, item), dirs_exist_ok=True)
    elif os.path.isfile(s):
        shutil.copyfile(s, os.path.join(dst, item))

summary = subprocess.run([sys.executable, "-I", SP + "/an115f.py", kind, run] + bases, capture_output=True, text=True)
shots = subprocess.run([sys.executable, "-I", SP + "/shots115.py", kind, run], capture_output=True, text=True)
rec = {"kind": kind, "arch": arch, "model": model, "name": NAMES.get(model, model), "run": os.path.basename(run),
       "matrix": (done.get("matrix") or [None])[0], "outcome": done.get("outcome"), "shots": shots.stdout.splitlines(),
       "shots_err": shots.stderr[-400:]}
mx = rec["matrix"]
# gcloud's outcome rows carry ANSI colours and box-drawing bars: keep the words only
oc = "; ".join(" ".join(re.sub(r"\x1b\[[0-9;]*m", "", x).replace("│", " ").replace("|", " ").split())
               for x in done.get("outcome") or [])
bk, pj = done.get("bucket") or "gs://test-lab-hrb456s6qt79m-kz4jkut67musi", done.get("project") or "phonefinder-hub"
rec["project"], rec["bucket"] = pj, bk
lines = []
if kind == "probe":
    auto = an115f.one(run + "/raw/*/artifacts/**/autorun.txt")
    P = an115f.passes_of(auto) if auto else {}
    rec["passes"] = [{"name": n, "counts": dict(Counter(v[0] for v in t.values())), "tests": len(t),
                      "total": round(sum(v[1] for v in t.values()) / 1000, 1),
                      "nonpass": [k for k, v in t.items() if v[0] != "PASS"]} for n, t in P.items()]
    head = (f"# FTL {model} ({NAMES.get(model, '')}, {arch}) build 115: PanProbe driver env A/B (scenario 3)\n\n"
            f"- Matrix `{mx}`; outcome `{oc}`; results `{bk}/{rdir}/` (project `{pj}`).\n"
            f"- APK `/home/flux/build-out/apks/panprobe-115.apk` (sha256 `0b774c6a...`), `dev.zenithblue.panvktest` 1.3.0 (vc 16);"
            f" rc5 slot = build 115 (`73709644...`, driverInfo `Mesa 26.3.0-devel (git-9dde9e7a18)`).\n"
            f"- Passes from `assets/env-ab.json` (`bbb3c2ed...`): `defaults`, then `114-knobs` (27 keys, the 114 value of every"
            f" default-on 115 change) in the same session.\n"
            f"- Baselines: {', '.join('`' + os.path.basename(b) + '`' for b in bases) or 'none'}.\n\n"
            "| pass | result | total time |\n|---|---|---|\n")
    for p in rec["passes"]:
        res = "/".join(f"{v} {k}" for k, v in p["counts"].items())
        head += f"| `{p['name']}` | {res} of {p['tests']}{' (' + ', '.join(p['nonpass']) + ')' if p['nonpass'] else ''} | {p['total']} s |\n"
else:
    files = sorted(glob.glob(run + "/raw/*/artifacts/**/gameloop/*/results.json", recursive=True))
    tasks = []
    for f in files:
        j = json.load(open(f))
        for ps in j.get("passes", []):
            te = ps.get("test") or {}
            tasks.append({"scenario": j.get("scenario"), "task": ps.get("task"), "status": ps.get("status"),
                          "reason": ps.get("reason"), "fps": ps.get("fps"), "testFps": te.get("fps"),
                          "line": (te.get("line") or "")[:120]})
    rec["tasks"] = tasks
    head = (f"# FTL {model} ({NAMES.get(model, '')}, {arch}) build 115: PanPlay game loop (scenarios 3, 5, 6)\n\n"
            f"- Matrix `{mx}`; outcome `{oc}`; results `{bk}/{rdir}/` (project `{pj}`).\n"
            f"- APK `/home/flux/build-out/apks/panplay-115.apk` (sha256 `09a9aba6...`), `dev.zenithblue.panvklauncher` 1.3.0 (vc 41),"
            f" bundled driver build 115 (`73709644...`). Default window: 20 s warm-up, 60 s measured, 8 s tail.\n"
            f"- `raw/` has everything but `video.mp4` (not committed, about 45 MB).\n\n"
            "| scenario | task | result | fps | test line |\n|---|---|---|---|---|\n")
    for t in tasks:
        fps = t["testFps"] if t["testFps"] is not None else t["fps"]
        head += (f"| {t['scenario']} | {t['task']} | {t['status']}{' (' + str(t['reason']) + ')' if t['reason'] else ''} |"
                 f" {an115f.f1(fps)} | `{t['line'] or '-'}` |\n")
    if not files:
        head += "| - | - | no results.json pulled | - | - |\n"
body = head + "\n## Summary (`an115f.py`)\n\n```text\n" + summary.stdout.rstrip() + "\n```\n\n## Screenshots\n\n"
body += "".join(f"- {s}\n" for s in rec["shots"]) + "\n"
open(run + "/results.md", "w").write(body)
open(SP + "/summary115.jsonl", "a").write(json.dumps(rec) + "\n")
print(body[:3000])
if summary.stderr.strip() or shots.stderr.strip():
    print("STDERR", summary.stderr[-800:], shots.stderr[-800:])
