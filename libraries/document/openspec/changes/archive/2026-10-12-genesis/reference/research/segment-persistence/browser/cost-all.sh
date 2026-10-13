#!/bin/sh
# Every strategy in its own origin (its own port), so that one IndexedDB backing store (one
# LevelDB per origin in Chromium) never carries another strategy's data or compaction.
cd "$(dirname "$0")"
port=8660
for e in chromium webkit firefox; do
    for s in opfs-s0 opfs-s1 opfs-s2 idb-strict idb-relaxed idb-default; do
        node run.mjs cost $e port=$port strategies=$s
        port=$((port + 1))
    done
done
