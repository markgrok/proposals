---
title: "Segment persistence: the browser track"
tags: [webcpp, persistence, browser, opfs, indexeddb]
status: active
created: 2026-10-12
---

# Segment persistence: the browser track

macOS 27.0, APFS on an internal SSD. Emscripten 6.0.11 with `-fwasm-exceptions`; each strategy in
its own dedicated worker, handing HEAPU8 ranges to storage. Playwright 1.63.0 (installed under
this directory); Chromium 153.0.8010.12 (headless shell), WebKit 26.6 (build 2359), Firefox 155.0
(firefox-1543, about 104 MB, downloaded with `npx playwright install firefox`). Pages served from
127.0.0.1 with COOP/COEP, every run in `launchPersistentContext` (ephemeral contexts are a trap,
surprise 6). Raw data in `results/*.jsonl` and `logs/`; `node summarize.mjs` prints the tables.

## Semantics

| Primitive | macOS call | Writes happen in | Source |
| --- | --- | --- | --- |
| Chromium OPFS flush() | F_BARRIERFSYNC, fsync as fallback; never F_FULLFSYNC | renderer, fd from the browser | file_system_sync_access_handle.cc:53; file_system_access_regular_file_delegate.cc:127; base/files/file_posix.cc:674-706 |
| WebKit OPFS flush() | fsync | WebContent, fd from the network process | FileSystemSyncAccessHandle.cpp:97; WTF/posix/FileHandlePOSIX.cpp:73 |
| Gecko OPFS flush() | PR_Sync (fsync) | content process, fd from the parent | dom/fs/api/FileSystemSyncAccessHandle.cpp:443; nsFileStreams.cpp:227; nspr unix.c:413 |
| Chromium IDB (LevelDB) | strict: sync=true (F_BARRIERFSYNC); relaxed: write only; default: the bucket's, relaxed since M124 | browser process | backing_store.cc:1461; quota_database.cc:143; env_chromium.cc:375 |
| WebKit IDB (SQLite, WAL) | strict adds a full WAL checkpoint; relaxed and default alike | network process | SQLiteIDBBackingStore.cpp:1164 |
| Gecko IDB (SQLite, WAL) | strict EXTRA, relaxed OFF, default NORMAL | parent process | ActorsParent.cpp:6875 |

No engine sets SQLite fullfsync or reaches F_FULLFSYNC: no browser primitive empties the drive's
cache; only Chromium gets the ordering barrier. Native reference, 1 MB pwrite plus sync, median /
p99: write 0.02 / 0.05 ms, fsync 0.11 / 0.13, F_BARRIERFSYNC 0.27 / 0.9, F_FULLFSYNC 4.3 / 6.5.

Persistence: Chromium grants `persist()` by heuristic without a prompt; WebKit only to exempt
domains, and ITP deletes script-written storage after 7 days without interaction
(ResourceLoadStatisticsStore.cpp:74); Firefox prompts. Headless: persist() false in Chromium and
WebKit, Firefox's prompt unanswered. Quota: 10 GB (Chromium, Firefox), 19.2 GB (WebKit) measured.

APIs: all three have createSyncAccessHandle, createWritable, the IDB durability option and
tx.commit(); only Chromium has 'readwrite-unsafe' and storageBuckets; WebKit's ephemeral context has
no OPFS. Emscripten's WasmFS OPFS backend maps fsync to accessHandle.flush() on a proxy thread
(needs pthreads, COOP/COEP); without pthreads its flush is a no-op. It adds cost, not strength.

## Cost

Median / p99 ms over 200 commits, about 4 dirty pages per turn; in brackets the CPU part (S1 digest,
S2 and IDB dirty scan). Bytes per commit: S0 and S1 image + 4 KB, S2 about 17.8 KB, IDB about 16.4 KB.

| Engine, MB | opfs-s0 | opfs-s1 | opfs-s2 | idb-strict | idb-relaxed | idb-default |
| --- | --- | --- | --- | --- | --- | --- |
| Chromium 1 | 0.36/0.52 | 1.72/2.07 (0.95) | 0.29/1.06 (0.07) | 0.62/2.18 | 0.24/0.50 | 0.24/0.46 |
| Chromium 16 | 1.38/1.88 | 16.9/18.1 (14.7) | 1.38/3.42 (1.1) | 1.78/2.77 | 1.30/1.61 | 1.31/1.57 |
| Chromium 64 | 4.95/8.02 | 64.9/68.1 (58.9) | 4.97/6.34 (4.4) | 5.17/6.98 | 4.66/5.00 | 4.65/4.97 |
| WebKit 1 | 0.14/0.20 | 1.14/1.26 | 0.14/0.44 | 1.10/2.36 | 0.26/0.84 | 0.28/0.90 |
| WebKit 16 | 1.10/1.30 | 17.0/17.6 | 1.16/1.50 | 2.14/2.48 | 1.32/1.76 | 1.30/2.04 |
| WebKit 64 | 3.98/4.22 | 67.0/69.8 | 4.44/4.84 | 5.72/6.56 | 4.66/6.14 | 4.52/5.28 |
| Firefox 1 | 0.18/0.28 | 1.50/7.22 | 0.26/10.8 | 0.40/7.94 | 0.34/5.32 | 0.32/5.64 |
| Firefox 16 | 1.36/1.80 | 20.4/27.6 | 2.08/7.04 | 3.06/7.06 | 2.26/9.02 | 3.00/6.00 |
| Firefox 64 | 8.08/14.8 | 71.0/80.0 | 7.30/12.7 | 7.34/9.74 | 7.44/10.5 | 7.40/10.1 |

The durability hint is honoured (strict costs about 0.4 ms more in Chromium, 0.8 ms in WebKit; in
Firefox barely visible). Above 16 MB the cost is the dirty tracking (memcmp against a shadow copy,
about 0.07 ms per MB) and S1's digest. Chromium's first cost run, under the other tracks' load, is
kept in `results/cost-chromium-run1.jsonl`. WasmFS S1 / S2 medians at 1, 16, 64 MB: Chromium 5.5,
27.5, 89.1 / 0.61, 1.71, 8.27; WebKit 1.3, 20.2, 72.3 / 0.20, 3.8, 8.4; Firefox 4.9, 21.5, 73.6 /
3.0, 4.3, 8.4.

## Crash consistency

Consistent / torn / lost over 200 kills, 1 MB, the six writers at once in one page, each kill 0-200 ms
after every writer has recovered and committed; every recovery checked with equals_replay.

| Kill | Engine | opfs-s0 | opfs-s1 | opfs-s2 | idb strict / relaxed / default |
| --- | --- | --- | --- | --- | --- |
| terminate | Chromium | 113/87/0 | 200/0/0 | 200/0/0 | 200/0/0 each |
| terminate | WebKit | 178/22/0 | 200/0/0 | 200/0/0 | strict, relaxed 200/0/0; default 118, then 67 hangs (wedge) |
| terminate | Firefox | 140/60/0 | 200/0/0 | 200/0/0 | 200/0/0 each |
| reload | Chromium | 139/61/0 | 200/0/0 | 200/0/0 | 200/0/0 each |
| reload | WebKit | 149/51/0 | 200/0/0 | 200/0/0 | 200/0/0 each |
| reload | Firefox | 172/28/0 | 200/0/0 | 200/0/0 | 200/0/0 each |
| SIGKILL | Chromium | 194/6/0 | 200/0/0 | 200/0/0 | 142/0/58 each, the same 58 kills |
| SIGKILL | WebKit | 157/43/0 | 200/0/0 | 200/0/0 | 200/0/0 each |
| SIGKILL | Firefox | 129/71/0 | 200/0/0 | 200/0/0 | 200/0/0 each |

A separate Chromium SIGKILL run with idb-strict alone: 40/0/0 of 40.

## Surprises

1. **Chromium's IndexedDB loses the whole origin after SIGKILL, even with 'strict'.** LevelDB is
   opened with paranoid_checks (backing_store.cc:155); a torn tail record is "Corruption: checksum
   mismatch", and Chromium destroys the store (DestroyDB, :314; IDBDataLoss::Total, :1498): every
   database of the origin at once (`logs/idbloss7-chromium.log`).
2. **WebKit's IndexedDB wedges after worker.terminate() mid-transaction**, in every durability
   (onset after 6 to 119 kills): every later readonly transaction hangs. A reload or a new page does
   not clear it; relaunching the browser on the same profile does, with the data intact.
3. Chromium's worker.terminate() does not stop a busy worker at once: it commits for 1-2 s more, the
   lock is free after about 1.1 s. WebKit and Firefox stop at once. No write after another worker
   took the lock, in any engine.
4. Firefox could not start from this shell (macOS app-data protection on ~/Library/Application
   Support/Firefox); MOZ_APP_DATA and MOZ_LOCAL_APP_DATA pointed elsewhere work around it.
5. Emscripten's aligned_alloc returns null for a size not a multiple of the alignment, and writes
   through that null do not trap in wasm: they silently corrupted the heap (fixed by rounding up).
6. Ephemeral contexts: Chromium's are incognito (IDB in memory, OPFS flush a no-op); WebKit's have
   no OPFS.
7. S0 tears under every kind of kill, 3% to 44%.

## Power loss (not testable here)

OPFS flush in WebKit and Firefox and SQLite's fsync reach the drive, whose cache may reorder or drop
them; Chromium's F_BARRIERFSYNC keeps the order but may lose the last commits; IndexedDB relaxed may
lose recent transactions. No browser API reaches F_FULLFSYNC.

## Recommendation

A dedicated worker with an OPFS FileSystemSyncAccessHandle and S2 (a log of dirty pages, a commit
record with the generation and a digest, flush; checkpoint, then truncate the log): 0 torn and 0
lost in 1,800 kills, about 0.3 ms plus the dirty scan per commit. S1 only for small images, never S0.
Without a barrier, do not truncate the only log: alternate two logs and put the image digest in the
checkpoint header. IndexedDB not as the primary store (Chromium may wipe the origin, WebKit may
wedge). Stop writers cooperatively (a message, then close()), never with terminate(). Call
navigator.storage.persist() and handle a refusal. Skip WasmFS.

The old chrome-headless-shell processes (pids 43052, from Fri Oct 9, and 80250, from Mon Oct 5)
are not this track's; none of its processes are left; its test profiles (1.3 GB) are deleted.
