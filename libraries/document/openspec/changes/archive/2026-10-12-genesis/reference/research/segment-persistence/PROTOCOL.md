---
title: "Segment persistence: the protocol every track follows"
tags: [webcpp, xactor, persistence, durability, wasm, browser]
status: active
created: 2026-10-12
---

# Segment persistence: the protocol

Owner's question (2026-10-12): open a file that holds the application's whole memory segment,
write to it, and find out exactly how each platform behaves and how to guarantee (or emulate) that
the state on disk is synchronised. Three tracks, one protocol, comparable results:

| Track | Directory | Storage |
| --- | --- | --- |
| native | `native/` | a file: mmap `MAP_SHARED` (Boost.Interprocess `managed_mapped_file` and raw mmap), and pwrite plus fsync / fdatasync / `F_FULLFSYNC` |
| wasmtime | `wasmtime/` | a file through wasi:filesystem (wasi-libc's POSIX calls), wasip2 and wasip3 |
| browser | `browser/` | OPFS (`FileSystemSyncAccessHandle` in a worker) and IndexedDB, in Chromium, WebKit and Firefox |

## The workload

`core/segment_core.hpp` (checked on native, wasip2 and Emscripten: identical digests). A segment
is a `managed_external_buffer` over a buffer; `create` builds it, `turn(g)` applies turn g,
`equals_replay(g)` says whether a buffer holds exactly the state after g turns. Do not change it;
copy nothing of it, include it. WebAssembly builds need exceptions: wasi-sdk
`-fwasm-exceptions -mllvm -wasm-use-legacy-eh=false ... -lunwind`, wasmtime `-W exceptions=y`;
Emscripten `-fwasm-exceptions`. Boost: `-I/opt/homebrew/opt/boost/include` (1.92). Toolchains:
`~/dev/webcpp/.local/wasi-sdk`, `~/dev/webcpp/.local/emscripten/em++` (6.0.11),
`~/dev/webcpp/.local/emscripten/node`, wasmtime 47.

## The commit strategies

The file holds a page 0 header and the image. A commit makes generation g durable.

- **S0, no protocol:** write the image in place (or let mmap write it), then sync. Expected to tear;
  it is the baseline that shows why a protocol is needed.
- **S1, two slots (shadow copy):** file = header A, header B, slot A, slot B. Write the whole image
  into the slot not in use, sync, then write that slot's header (generation, size, digest of the
  slot), sync. Recovery takes the valid header (digest matches its slot) with the highest generation.
- **S2, write-ahead log of dirty pages:** find the 4 KB pages that changed since the last commit
  (compare with a copy of the last committed image: the only dirty tracking WebAssembly has), append
  records (page number, bytes) and a commit record (generation, digest of the records) to a log,
  sync the log; apply to the main image and sync later (checkpoint), then truncate the log. Recovery
  replays every complete, valid commit of the log over the image.
- On native also: **S3, mmap `MAP_SHARED` in place with msync(`MS_SYNC`)**, alone (it is S0) and
  under S2 (the log written with pwrite, the image through the mapping).
- On IndexedDB: the pages are records, and S1/S2 become one readwrite transaction that writes the
  dirty pages and the header; compare `durability: 'strict'` with `'relaxed'` and the default.

## What each track measures

1. **Semantics:** what each sync primitive guarantees, from the platform's source or documentation,
   cited (man pages, wasmtime's and Rust std's source for `sync_data` / `sync_all` on macOS and
   Linux, Chromium's / WebKit's / Gecko's OPFS `flush()` and IndexedDB durability). Which ones reach
   the disk's own cache (macOS `fsync` does not; `F_FULLFSYNC` does).
2. **Cost:** commit latency (median and 99th percentile over at least 200 commits) for images of
   1 MB, 16 MB and 64 MB, with the workload's few dirty pages per turn; bytes written per commit.
3. **Crash consistency:** kill the writer at random moments during a loop of turns and commits (at
   least 200 kills per strategy: `SIGKILL` of the process natively and of wasmtime; on the browser,
   `worker.terminate()`, a page reload, and `SIGKILL` of the browser process with a persistent
   profile), reopen, recover, and check with `equals_replay` that the state is exactly some
   generation g with g at least the last generation the writer reported as committed. Report the
   counts: consistent, torn (no valid generation, or one that does not replay), lost (an older
   generation than a reported commit).
4. **Power loss** cannot be tested here: state what each primitive promises and what it does not.
5. Anything surprising, with the evidence (a silent loss like wasi-libc's emulated `MAP_SHARED`,
   found earlier: accepted, never written back).

## Rules

- Work only under your track's directory; read `core/` and include it. Nothing goes into any
  repository; nothing is pushed or published.
- Everything written to files is in English; no em dash character.
- Never print environment variables. Downloads only of the tooling your track needs (for instance
  Playwright's browser builds), named in your report.
- A result you did not run is not a result: say "not measured" and why.
- Return your report as your final message (plain text, under 120 lines): the semantics table with
  sources, the cost table, the crash table, the surprises, and your recommendation for how webcpp
  should make a segment durable on your platform.
