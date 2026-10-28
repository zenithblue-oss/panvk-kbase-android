"""an115play.py <run dir>: PanPlay game-loop (scenarios 4, 5, 6) analysis for 115pre on FTL."""
import glob
import json
import os
import re
import sys
from collections import Counter

run = sys.argv[1]
files = sorted(glob.glob(run + "/raw/*/artifacts/**/gameloop/*/results.json", recursive=True))
print("== results.json files:", len(files))


def g(o, *ks):
    for k in ks:
        if not isinstance(o, dict):
            return None
        o = o.get(k)
    return o


def f1(v):
    return "-" if v is None else (f"{v:.1f}" if isinstance(v, (int, float)) else str(v))


for p in files:
    j = json.load(open(p))
    d = os.path.dirname(p)
    print("\n###", os.path.relpath(d, run), "scenario", j.get("scenario"), j.get("scenarioName"),
          "ok", j.get("ok"), "error", j.get("error"))
    print("driverInfo:", j.get("driverInfo"), "| driver:", g(j, "driver", "name"), g(j, "driver", "version"),
          "sha", (g(j, "driver", "sha256") or "")[:16], "size", g(j, "driver", "size"))
    print("app:", j.get("app"), "| device:", g(j, "device", "model"), g(j, "device", "sdk"), g(j, "device", "socModel"))
    print("configSource:", j.get("configSource"))
    for ps in j.get("passes", []):
        print(f"- pass {ps.get('index')} {ps.get('task')} {ps.get('envSet')}: status={ps.get('status')} reason={ps.get('reason')}"
              f" exit={ps.get('exit')} wall={f1(ps.get('wallSec'))}s firstFrame={f1(ps.get('firstFrameSec'))}s"
              f" fps={f1(ps.get('fps'))} ({ps.get('fpsSource')}) presents={ps.get('driverPresents')}")
        df, dx, co, te = ps.get("driverFps") or {}, ps.get("dxvkFps") or {}, ps.get("compositor") or {}, ps.get("test") or {}
        print(f"    driverFps fps={f1(df.get('fps'))} frames={df.get('frames')} 1s min/max={f1(df.get('minIntervalFps'))}/{f1(df.get('maxIntervalFps'))}"
              f" maxFrameMs={f1(df.get('maxFrameMs'))} intervals={df.get('intervals')}")
        print(f"    dxvkFps avg={f1(dx.get('avgFps'))} 1%low={f1(dx.get('low1Fps'))} 0.1%low={f1(dx.get('low01Fps'))} p50={f1(dx.get('p50Ms'))}"
              f" p95={f1(dx.get('p95Ms'))} p99={f1(dx.get('p99Ms'))} stutters={dx.get('stutters')} gpuLoad={f1(dx.get('gpuLoadAvg'))}")
        print(f"    compositor drawn={f1(co.get('drawnFps'))} content={f1(co.get('contentFps'))} | test ok={te.get('ok')} fps={te.get('fps')}"
              f" startup={te.get('startupSec')} frames={te.get('frames')} line={te.get('line')}")
        print(f"    driverInfo(fps log)={ps.get('driverInfo')} screenshot={ps.get('screenshot')} sessionReason={ps.get('sessionReason')}")
        envs = ps.get("env") or {}
        print(f"    env keys={len(envs)} PANVK_*={sum(1 for k in envs if k.startswith('PANVK_') and k not in ('PANVK_FPS_LOG', 'PANVK_FPS_LOG_MS'))}")
    for a in j.get("ab", []):
        print("A/B:", a)
    t = os.path.join(d, "results.txt")
    if os.path.isfile(t):
        print(open(t).read())
    h = os.path.join(d, "harness.log")
    if os.path.isfile(h):
        print("-- harness.log")
        for line in open(h, errors="replace"):
            print("   ", line.rstrip()[:220])

print("\n== per-pass logs")
for pdir in sorted(glob.glob(run + "/raw/*/artifacts/**/gameloop/*/[0-9][0-9]-*", recursive=True)):
    if not os.path.isdir(pdir):
        continue
    name = os.path.relpath(pdir, run)
    errs = Counter()
    for lf in glob.glob(pdir + "/session/**/*", recursive=True):
        if not os.path.isfile(lf) or os.path.getsize(lf) > 50_000_000:
            continue
        base = os.path.basename(lf)
        if not (base.endswith(".log") or base.endswith(".txt")):
            continue
        for line in open(lf, errors="replace"):
            s = line.strip()
            if re.search(r"\b(err|ERR|error|ERROR|FIXME|fixme)\b", s) and ("vkd3d" in base or "vkd3d" in s or base.startswith("wine")):
                errs[(base, re.sub(r"^[0-9a-f]+:|^\d+:|^[\d:. -]+", "", s)[:160])] += 1
            for k in ("DEVICE_LOST", "event=0x", "Fatal signal", "VK_ERROR", "GPU fault", "CS_FATAL"):
                if k in s:
                    errs[(base, "FAULT " + k + ": " + s[:140])] += 1
    print("##", name, "files", len(os.listdir(pdir)), sorted(os.listdir(pdir))[:20])
    for (b, s), c in errs.most_common(25):
        print("   ", c, b, s)

L = sorted(glob.glob(run + "/raw/*/logcat"))
if L:
    c = Counter()
    coh = Counter()
    dx = []
    for line in open(L[0], errors="replace"):
        for k in ("DEVICE_LOST", "event=0x", "Fatal signal", "GPU fault", "CS_FATAL", "ANR in", "FATAL EXCEPTION", "lowmemorykiller", "Killing "):
            if k in line and ("panvklauncher" in line or k in ("DEVICE_LOST", "event=0x", "GPU fault", "CS_FATAL") or "Fatal signal" in k):
                c[k] += 1
        if "kbase: coherency mode" in line:
            coh[re.sub(r"^.*?kbase: ", "kbase: ", line.strip())] += 1
        if "DX12:" in line and len(dx) < 400:
            dx.append(line.strip()[:200])
    print("\n== logcat faults:", dict(c) or "none")
    for k, v in coh.most_common():
        print("coherency:", v, k[:220])
    print("DX12 lines in logcat:", len(dx))
    for x in dx[:12] + (["..."] if len(dx) > 24 else []) + dx[-12:]:
        print("   ", x)
