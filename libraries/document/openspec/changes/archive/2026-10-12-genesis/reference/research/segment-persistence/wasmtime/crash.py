#!/usr/bin/env python3
"""Crash consistency: SIGKILL the wasmtime process running `seg write` at a random moment, then run
`seg check` in a fresh wasmtime and classify the recovered state.

    crash.py <p2|p3> <S0|S1|S2> <kills> <size-MB> <results.csv>

A run's file carries over to the next run (the writer recovers and continues), so recovery is
exercised by the writer too; the directory is wiped every 25 runs to bound the replay, and after
any torn or lost result so that no run starts from a damaged file.

Classification of the check's "R <g> <ok|torn>" against the highest generation the writer printed
as committed (the "C <g>" lines, printed after the commit's last sync returned):
    consistent  ok and g >= reported (or no file and nothing reported)
    lost        ok and g < reported (or no file although something was reported)
    torn        no valid generation, or one that does not replay
"""
import os
import random
import shutil
import signal
import subprocess
import sys
import threading
import time

here = os.path.dirname(os.path.abspath(__file__))
target, strategy, kills, size, out = sys.argv[1], sys.argv[2], int(sys.argv[3]), sys.argv[4], sys.argv[5]
work = os.path.join(here, ".scratch", f"crash_{target}_{strategy}")
module = os.path.join(here, f"seg_{target}.cwasm")


def wasmtime(*args):
    return ["wasmtime", "run", "--allow-precompiled", "-W", "exceptions=y", "--dir", f"{work}::/data",
            module, *args]


def wipe():
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)


rng = random.Random(f"{target}-{strategy}")
counts = {"consistent": 0, "torn": 0, "lost": 0}
reported = 0
wipe()
with open(out, "w") as csv:
    csv.write("run,delay_s,started,commits_this_run,reported,recovered,verdict,why\n")
    for run in range(kills):
        if run % 25 == 0 and run > 0:
            wipe()
            reported = 0
        lines = []
        proc = subprocess.Popen(wasmtime("write", strategy, "/data", size, "data"), stdout=subprocess.PIPE,
                                stderr=subprocess.DEVNULL, bufsize=0)
        reader = threading.Thread(target=lambda: lines.extend(proc.stdout.read().decode().split("\n")))
        reader.start()
        delay = rng.uniform(0.05, 1.5)
        time.sleep(delay)
        os.kill(proc.pid, signal.SIGKILL)
        proc.wait()
        reader.join()
        started = any(l.startswith("S ") for l in lines)
        committed = [int(l.split()[1]) for l in lines if l.startswith("C ")]
        if committed:
            reported = max(reported, committed[-1])
        check = subprocess.run(wasmtime("check", strategy, "/data", size), capture_output=True, text=True)
        parts = check.stdout.strip().split(" ", 3)
        if len(parts) < 3 or parts[0] != "R":
            print("check failed:", check.stdout, check.stderr, file=sys.stderr)
            sys.exit(1)
        g, flag, why = parts[1], parts[2], parts[3] if len(parts) > 3 else ""
        if flag == "none":
            verdict = "consistent" if reported == 0 else "lost"
        elif flag == "ok":
            verdict = "consistent" if int(g) >= reported else "lost"
        else:
            verdict = "torn"
        counts[verdict] += 1
        csv.write(f"{run},{delay:.3f},{int(started)},{len(committed)},{reported},{g},{verdict},{why}\n")
        csv.flush()
        if verdict != "consistent":
            wipe()
            reported = 0
print(target, strategy, f"{kills} kills:", " ".join(f"{k} {v}" for k, v in counts.items()))
