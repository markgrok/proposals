---
title: "Making a memory segment durable: native, wasmtime and the browser"
tags: [webcpp, persistence, durability, segment, wasm, browser, opfs, indexeddb]
status: active
created: 2026-10-12
---

# Making a memory segment durable

The owner's question (2026-10-12): open a file that holds the application's whole memory segment,
write to it, and find out exactly how each platform behaves and how to guarantee, or emulate, that
the state on disk is synchronised. Three tracks ran one protocol
(`segment-persistence/PROTOCOL.md`) on one workload (`segment-persistence/core/segment_core.hpp`, a
Boost.Interprocess `managed_external_buffer` with a multi_index, identical digests on native, wasip2
and Emscripten). Each track's report, with its code and raw data:

- native, macOS and Linux: `segment-persistence/native/REPORT.md`
- wasmtime, wasip2 and wasip3: `segment-persistence/wasmtime/REPORT.md`
- browser, Chromium, WebKit and Firefox, OPFS and IndexedDB: `segment-persistence/browser/REPORT.md`

All on this Mac (macOS 27, APFS, internal SSD); Linux in a Docker VM.

## The answer in one paragraph

Every platform converged on the same protocol: keep the live segment in private memory, and make
each turn durable by appending the pages it changed (found by comparing with the last committed copy)
and a commit record (generation and digest) to a log, then one sync; now and then write the pages
into the image, sync, and reset the log. Recovery replays the valid, consecutive commits. This
strategy, S2, was never torn and never lost a reported commit: 1,200 kills natively (three variants
on macOS and Linux), 600 under wasmtime and 1,800 in the browsers. Writing the image in place (S0), or mapping it and letting the kernel
write it (S3, `managed_mapped_file`), tore under every kind of kill, and some torn heaps passed the
logical check and crashed the next process.

## How a sync maps on each platform

| Platform | Call | What reaches the disk | Cost per commit (1 MB, S2, median) |
| --- | --- | --- | --- |
| macOS native | fsync / fdatasync | the drive, not its cache, possibly reordered | 0.05 ms |
| macOS native | F_BARRIERFSYNC | the drive, in order, not its cache | 0.4 ms |
| macOS native | F_FULLFSYNC | the media | 7.4 ms |
| Linux native | fdatasync / fsync / msync(MS_SYNC) | the media, when the device honours flushes | 0.1 ms (VM) |
| wasmtime on macOS | fsync / fdatasync (wasi:filesystem sync, sync-data) | both are F_FULLFSYNC in the host (Rust std), no lighter option | 4.3-6.2 ms |
| wasmtime on Linux | the same | fsync / fdatasync in the host (from source; not measured) | not measured |
| Chromium | OPFS flush() | F_BARRIERFSYNC: in order, not the cache | 0.3 ms |
| WebKit, Firefox | OPFS flush() | fsync: the drive, not its cache | 0.14-0.26 ms |
| every browser | IndexedDB 'strict' | the engine's own sync (LevelDB, SQLite); never F_FULLFSYNC | 0.4-1.1 ms |

No browser API reaches F_FULLFSYNC on macOS, so in the browser a power loss can lose the last
commits on any engine. Above 16 MB the cost is the dirty-page comparison (0.07 to 0.27 ms per MB),
which WebAssembly cannot avoid: it has no page-fault tracking.

## What must never be used

| Mechanism | Why |
| --- | --- |
| wasi-libc's `mmap` with `MAP_SHARED` | accepted, never written back: a silent loss; `msync` does not even link |
| Boost.Interprocess `managed_mapped_file::flush()` | it is `msync(MS_ASYNC)`: durable nowhere, and on Linux it writes nothing |
| the durable image changed in place (S0, S3) | torn under every kill (3% to 44% in browsers), and torn heaps crash the next writer |
| a check that is only logical | in-place heaps passed `equals_replay` and then crashed: recovery must check bytes (a digest) |
| IndexedDB as the primary store | Chromium destroys the whole origin's IndexedDB after a crash tears LevelDB's log (58 of 200 SIGKILLs, even 'strict'); WebKit's wedges after `worker.terminate()` until the browser restarts |
| `worker.terminate()` to stop a writer | Chromium keeps the worker writing for 1-2 s; WebKit's IndexedDB wedges |
| `wasmtime serve` holding the authoritative segment | a fresh instance per request (p2), unpredictable reuse (p3), several instances at once, and no file locks in WASI |
| Emscripten's WasmFS OPFS backend | the same flush with more cost, and it needs pthreads |
| a host that backs wasm memory with a file | possible for an embedder, but the process is not resumable and the OS writes pages back whenever it wants (S0's tearing) |

## Other facts the design must carry

- Linux splits a single large `pwrite` under SIGKILL (147 of 200): a commit's validity comes from
  its checksum, never from write atomicity; short writes are handled.
- A log that grows costs up to 2.5 times a preallocated one on Linux (fdatasync then commits the
  size): preallocate the log with zeros actually written.
- On macOS the S2 checkpoint needs "image pages durable before the log reset": F_FULLFSYNC (or
  F_BARRIERFSYNC for order alone). In the browser, without a barrier on WebKit and Firefox, keep two
  logs alternately and put the image's digest in the checkpoint's header, so no reset ever leaves
  the only copy of a commit in a cache.
- WASI has no file locks: one writer per file is enforced outside WASI.
- An fsync error on Linux is reported once and the pages are then marked clean: treat it as fatal.
- In the browser: call `navigator.storage.persist()` and handle a refusal (headless Chromium and
  WebKit refused; WebKit's ITP deletes script-written storage after 7 days without interaction);
  stop a writer cooperatively (a message, then `close()`).
- Ephemeral browser contexts are not a test bed: Chromium's flush is a no-op there, WebKit's has no
  OPFS.

## Recommendation for webcpp

One protocol everywhere, S2, behind one interface, with a backend per platform:

| Platform | Store | Sync |
| --- | --- | --- |
| Linux native | a file, pwrite | fdatasync |
| macOS native | a file, pwrite | F_FULLFSYNC for what must survive power loss; F_BARRIERFSYNC where order suffices, with group commit |
| wasmtime | a file through wasi:filesystem | fdatasync (F_FULLFSYNC on a macOS host, about 5 ms: group commits when that is too slow) |
| browser | OPFS `FileSystemSyncAccessHandle` in a dedicated worker | flush(), two alternating logs |

This is part 2 of the allocators stage (the commit), and it changes part 2's design: the root-offset
swap was to commit inside the shared segment; persistence adds the log beneath it.

## Found along the way, outside the question

- A wasip3 program that keeps writing to stdout after the pipe's reader closes spins at 100% CPU
  forever under wasmtime 47 (`segment-persistence/wasmtime/probes/epipe2.c`); p2 gets its error at
  once.
- Emscripten's `aligned_alloc` returns null when the size is not a multiple of the alignment, and a
  write through null does not trap in wasm: it corrupts the heap silently.
- Firefox started from a shell on macOS needs `MOZ_APP_DATA` and `MOZ_LOCAL_APP_DATA` pointed away
  from `~/Library/Application Support/Firefox` (app-data protection).
