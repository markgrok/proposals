#!/usr/bin/env python3
"""Crash driver: launches `seg write`, SIGKILLs it at a random moment, runs `seg check`, tallies.

usage: crash.py <seg binary> <strategy> <sync> <work dir> <size_mb> <kills> <min_ms> <max_ms>

Each trial continues from the files the previous one left (the writer recovers and goes on), so
recovery itself is under test. A torn, lost or ahead verdict resets the files; the trial after it
starts from a fresh segment. Prints one JSON line with the counts.
"""
import collections
import json
import os
import random
import shutil
import signal
import subprocess
import sys
import time


def next_writer(seg, strat, sync, work, size):
    """What a writer does when it opens a bad state and runs for 0.5 s: crashes, or goes on (and
    then whether the state it leaves is still bad)."""
    probe = work + "-probe"
    shutil.rmtree(probe, ignore_errors=True)
    shutil.copytree(work, probe)
    p = subprocess.Popen([seg, "write", strat, sync, probe, size], stdout=subprocess.DEVNULL,
                         stderr=subprocess.DEVNULL)
    time.sleep(0.5)
    if p.poll() is not None:
        return f"next writer died ({p.returncode})"
    p.send_signal(signal.SIGKILL)
    p.wait()
    out = subprocess.run([seg, "check", strat, probe, size], capture_output=True, text=True, check=True).stdout
    return "next writer ran, then " + json.loads(out)["verdict"]


def main():
    seg, strat, sync, work, size, kills, lo, hi = sys.argv[1:9]
    kills, lo, hi = int(kills), float(lo), float(hi)
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    verdicts = collections.Counter()
    phases = collections.Counter()
    bad_phases = collections.Counter()
    examples = []
    after = collections.Counter()
    commits = 0
    rng = random.Random(int(os.environ.get("SEED", "12345")))
    for trial in range(kills):
        p = subprocess.Popen([seg, "write", strat, sync, work, size], stdout=subprocess.DEVNULL)
        time.sleep(rng.uniform(lo, hi) / 1000.0)
        if p.poll() is not None:
            print(f"writer exited early with {p.returncode}", file=sys.stderr)
            sys.exit(1)
        p.send_signal(signal.SIGKILL)
        p.wait()
        out = subprocess.run([seg, "check", strat, work, size], capture_output=True, text=True, check=True).stdout
        r = json.loads(out)
        verdicts[r["verdict"]] += 1
        phases[r["phase"]] += 1
        if r["verdict"] not in ("consistent", "none"):  # torn, latent, lost, ahead
            bad_phases[r["phase"]] += 1
            if len(examples) < 4:
                examples.append(r)
            if r["verdict"] in ("latent", "torn"):
                after[r["verdict"] + ": " + next_writer(seg, strat, sync, work, size)] += 1
            shutil.rmtree(work)
            os.makedirs(work)
        commits = max(commits, r.get("reported", 0))
    print(json.dumps({"strategy": strat, "sync": sync, "size_mb": int(size), "kills": kills,
                      "verdicts": dict(verdicts), "kill_phases": dict(phases),
                      "bad_phases": dict(bad_phases), "after_bad_state": dict(after),
                      "examples": examples}))


if __name__ == "__main__":
    main()
