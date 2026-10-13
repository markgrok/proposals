#!/bin/sh
# usage: run_crash.sh <seg binary> <work dir> <output jsonl> <kills>
# Every strategy under SIGKILL, 1 MB segments; tear with 64 MB single pwrites.
seg=$1; work=$2; out=$3; kills=$4
: > "$out"
for s in s0 s1 s1d s2 s2p s2mmap s3raw s3bip; do
    python3 "$(dirname "$0")/crash.py" "$seg" $s fsync "$work/$s" 1 "$kills" 2 60 >> "$out"
done
python3 "$(dirname "$0")/crash.py" "$seg" s3bip bipflush "$work/s3bip-flush" 1 "$kills" 2 60 >> "$out"
python3 "$(dirname "$0")/crash.py" "$seg" tear fsync "$work/tear" 64 "$kills" 2 120 >> "$out"
