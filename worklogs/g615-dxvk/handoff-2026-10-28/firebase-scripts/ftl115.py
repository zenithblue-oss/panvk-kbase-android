#!/usr/bin/env python3
"""ftl115.py: Firebase Test Lab game-loop runs for build 115, one device at a time, every gcloud/gsutil call under the lock.

  ftl115.py run <app gs://...> <model> <api> <scenarios> <timeout> <results-dir> <pull dir on device> <log>
  ftl115.py pull <results-dir> <new empty local dir>

`run` blocks until the matrix finishes and writes the full gcloud output to <log>.
"""
import os
import subprocess
import sys

LOCK = "/home/flux/build-cache/firebase.lock"
GCLOUD = "/home/flux/.local/bin/gcloud"
GSUTIL = "/home/flux/.local/bin/gsutil"
# FTL_PROJECT / FTL_BUCKET: the Test Lab project and its results bucket (115 runs rotate projects on quota)
PROJECT = os.environ.get("FTL_PROJECT", "phonefinder-hub")
BUCKET = os.environ.get("FTL_BUCKET", "gs://test-lab-hrb456s6qt79m-kz4jkut67musi")


def locked(cmd, log=None):
    print("+", " ".join(cmd), flush=True)
    out = open(log, "a") if log else None
    r = subprocess.run(["flock", LOCK] + cmd, stdout=out or None, stderr=subprocess.STDOUT if out else None)
    if out:
        out.close()
    return r.returncode


def main(a):
    if a[0] == "run":
        app, model, api, scen, timeout, rdir, pull, log = a[1:9]
        assert app.startswith("gs://") or os.path.isfile(app), app  # a local APK: gcloud uploads it to the results dir
        rc = locked([GCLOUD, "firebase", "test", "android", "run", "--project", PROJECT, "--type", "game-loop",
                     "--app", app, "--scenario-numbers", scen, "--device", f"model={model},version={api}",
                     "--timeout", timeout, "--results-dir", rdir, "--directories-to-pull", pull,
                     "--num-flaky-test-attempts", "0"]
                    # FTL_OTHER_FILES: "<device path>=<local file>,..." (PanPlay gameloop.json override)
                    + (["--other-files", os.environ["FTL_OTHER_FILES"]] if os.environ.get("FTL_OTHER_FILES") else []), log)
        print("gcloud exit", rc)
        for line in open(log, errors="replace"):
            s = line.strip()
            if any(k in s for k in ("matrix-", "Passed", "Failed", "Inconclusive", "Skipped", "ERROR", "Error", "Quota", "quota",
                                    "timed out", "OUTCOME", "TEST_AXIS_VALUE", "TEST_DETAILS", "|")):
                print("  ", s[:220])
        return rc
    if a[0] == "pull":
        rdir, dest = a[1], a[2]
        assert os.path.isdir(dest) and not os.listdir(dest), "pull wants a new empty dir: " + dest
        return locked([GSUTIL, "-m", "-q", "cp", "-r", f"{BUCKET}/{rdir}", dest])
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
