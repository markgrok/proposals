#!/bin/sh
# usage: run_crash_inplace.sh <seg binary> <work dir> <output jsonl> <kills>
# The strategies that mutate the image in place (S0, S3), with what a writer does on a bad state.
seg=$1; work=$2; out=$3; kills=$4
: > "$out"
for s in s0 s3raw s3bip; do
    python3 "$(dirname "$0")/crash.py" "$seg" $s fsync "$work/$s" 1 "$kills" 2 60 >> "$out"
done
python3 "$(dirname "$0")/crash.py" "$seg" s3bip bipflush "$work/s3bip-flush" 1 "$kills" 2 60 >> "$out"
