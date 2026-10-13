#!/bin/sh
# usage: bench.sh <seg binary> <work dir> <output jsonl> <macos|linux>
# Commit latency, 200 commits after 20 warm-up ones, one turn per commit, at 1, 16 and 64 MB.
seg=$1; work=$2; out=$3; os=$4
mkdir -p "$work"; : > "$out"
if [ "$os" = macos ]; then fdm="fsync fdatasync full barrier"; mm="msync fsync full barrier"; s2m="fsync full"; bip="bipflush msync full"
else fdm="fsync fdatasync"; mm="msync fdatasync"; s2m="fdatasync"; bip="bipflush msync fdatasync"; fi
run() { r=`$seg bench $1 $2 "$work" $3 200` || r='{"error":true}'; echo "{\"strategy\":\"$1\",\"sync\":\"$2\",\"size_mb\":$3,\"r\":$r}" | tee -a "$out"; }
for size in 1 16 64; do
    for s in s0 s1 s1d s2 s2p; do for m in $fdm; do run $s $m $size; done; done
    for m in $s2m; do run s2mmap $m $size; done
    for m in $mm; do run s3raw $m $size; done
    for m in $bip; do run s3bip $m $size; done
done
