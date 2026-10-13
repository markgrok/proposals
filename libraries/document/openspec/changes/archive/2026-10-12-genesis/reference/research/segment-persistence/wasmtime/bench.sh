#!/bin/sh
# Commit cost: 200 commits per strategy, image size and target, fdatasync (sync-data); S2 at 1 MB
# also with fsync (sync). One run at a time, so that no two compete for the disk.
cd "$(dirname "$0")"
mkdir -p .scratch/bench
for t in p2 p3; do
  for mb in 1 16 64; do
    for s in S0 S1 S2; do
      rm -f .scratch/bench/*
      printf "%s %s %2s MB data: " $t $s $mb
      wasmtime run --allow-precompiled -W exceptions=y --dir "$PWD/.scratch/bench::/data" seg_$t.cwasm bench $s /data $mb data 200
    done
  done
  rm -f .scratch/bench/*
  printf "%s S2  1 MB sync: " $t
  wasmtime run --allow-precompiled -W exceptions=y --dir "$PWD/.scratch/bench::/data" seg_$t.cwasm bench S2 /data 1 sync 200
done
rm -f .scratch/bench/*
