#!/usr/bin/env python3
"""chain115.py <plan.json> <not-before HH:MM UTC> <give-up HH:MM UTC>: the build 115 FTL runs, one at a time.

Each plan entry: {"name", "app", "model", "api", "scen", "timeout", "rdir", "pull", "log", "dl"}.
A run whose <dl>/DONE exists is skipped. TEST_QUOTA_EXCEEDED: wait 30 min and try the same run again (the matrix
fails at validation, no device time is used), until the give-up time. Any other gcloud failure without a test
outcome stops the chain. After a run with an outcome: pull into <dl>/pull (a new empty dir) and write <dl>/DONE.
Prints one line per event (for the caller's notifications).
"""
import datetime
import json
import os
import subprocess
import sys
import time

SP = os.path.dirname(os.path.abspath(__file__))
FTL = os.path.join(SP, "ftl115.py")


def now():
    return datetime.datetime.now(datetime.timezone.utc)


def at(hhmm):
    if hhmm == "now":
        return now()
    h, m = map(int, hhmm.split(":"))
    t = now().replace(hour=h, minute=m, second=0, microsecond=0)
    return t if t > now() - datetime.timedelta(hours=12) else t + datetime.timedelta(days=1)


def say(*a):
    print(now().strftime("%H:%M:%S"), *a, flush=True)


plan = json.load(open(sys.argv[1]))
start, stop = at(sys.argv[2]), at(sys.argv[3])
if stop < start:
    stop += datetime.timedelta(days=1)
while now() < start:
    time.sleep(30)
for r in plan:
    done = os.path.join(r["dl"], "DONE")
    if os.path.exists(done):
        say("SKIP", r["name"])
        continue
    while True:
        say("START", r["name"])
        size0 = os.path.getsize(r["log"]) if os.path.exists(r["log"]) else 0
        rc = subprocess.run([sys.executable, FTL, "run", r["app"], r["model"], r["api"], r["scen"], r["timeout"], r["rdir"],
                             r["pull"], r["log"]], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode
        with open(r["log"], errors="replace") as f:
            f.seek(size0)
            out = f.read()
        if "TEST_QUOTA_EXCEEDED" in out:
            if now() + datetime.timedelta(minutes=15) > stop:
                say("QUOTA-GIVEUP", r["name"])
                sys.exit(3)
            say("QUOTA", r["name"], "retry in 15 min")
            time.sleep(900)
            continue
        break
    outcome = [l.strip() for l in out.splitlines() if l.strip().startswith(("|", "│")) and ("Passed" in l or "Failed" in l
                                                                                      or "Inconclusive" in l or "Skipped" in l)]
    matrix = [w for l in out.splitlines() for w in l.replace("[", " ").replace("]", " ").split() if w.startswith("matrix-")]
    say("END", r["name"], "rc", rc, matrix[:1], outcome[:1])
    if not outcome:
        say("STOP", r["name"], "no test outcome, see", r["log"])
        sys.exit(2)
    # gcloud prints "Raw results will be stored in your GCS bucket at [https://console.developers.google.com/storage/browser/<bucket>/<rdir>/]"
    bucket = [l.split("/storage/browser/")[1].split("/")[0] for l in out.splitlines() if "/storage/browser/" in l]
    env = dict(os.environ, FTL_BUCKET="gs://" + bucket[-1]) if bucket else os.environ
    os.makedirs(os.path.join(r["dl"], "pull"), exist_ok=False)
    prc = subprocess.run([sys.executable, FTL, "pull", r["rdir"], os.path.join(r["dl"], "pull")],
                         stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, env=env).returncode
    with open(done, "w") as f:
        f.write(json.dumps({"rc": rc, "pull_rc": prc, "matrix": matrix[:1], "outcome": outcome, "end": now().isoformat(),
                            "project": os.environ.get("FTL_PROJECT", "phonefinder-hub"),
                            "bucket": env.get("FTL_BUCKET")}) + "\n")
    say("PULLED", r["name"], "rc", prc)
say("ALL-DONE")
