---
title: "Segment persistence: the wasmtime track"
tags: [webcpp, persistence, wasmtime, wasi]
status: active
created: 2026-10-12
---

# Segment persistence: the wasmtime track

wasi-sdk 34 (wasi-libc 2e6fb9d8ee0c), wasmtime 47.0.3, macOS 27 on APFS on an internal SSD. Code:
`seg.cpp` (S0/S1/S2 over pwrite/pread/fdatasync/fsync), `build.sh`, `bench.sh`, `bench.txt`,
`crash.py`, `results/`, `probes/`; `.sources/` (wasmtime v47.0.3, cap-std 4.0.2, wasi-libc, 146 MB)
may be deleted.

## 1. Semantics

| Guest call | WASI | wasmtime 47 host | Rust std, macOS | Rust std, Linux |
| --- | --- | --- | --- | --- |
| fsync | descriptor.sync | File::sync_all | fcntl(F_FULLFSYNC) | fsync |
| fdatasync | descriptor.sync-data | File::sync_data | fcntl(F_FULLFSYNC) | fdatasync |

Sources: wasi-libc `cloudlibc/src/libc/unistd/fsync.c`, `fdatasync.c`; wasmtime
`crates/wasi/src/filesystem.rs:502-528, 598-624`; cap-std `src/fs/file.rs:57-68`; Rust 1.95
`library/std/src/sys/fs/unix.rs:1381-1432` (apple: both F_FULLFSYNC). WIT wasi:filesystem 0.2.12
and 0.3.0 have no file locks.

4 KB pwrite + sync, 200 runs, median / p99 ms: native fsync 0.027 / 0.063, fdatasync 0.022 / 0.038,
F_FULLFSYNC 4.31 / 11.8; wasm p2 fsync 6.03 / 12.2, fdatasync 5.03 / 10.6; p3 5.63 / 9.3,
4.66 / 8.4. On macOS a guest's every sync reaches the drive's cache, with no lighter barrier.
Linux not measured (from source and documentation: fsync/fdatasync flush the device cache with
barriers on). No sync orders two files, or a directory entry unless the directory is synced.

mmap under WASI: wasi-libc `libc-bottom-half/mman/mman.c` is malloc + pread; `MAP_SHARED |
PROT_WRITE` is accepted and never written back (the file stayed zero on p2 and p3, even after
munmap); msync is declared but undefined (link error). managed_mapped_file cannot work; no S3.

## 2. Cost

200 commits of one turn each after 1000 warm-up turns, fdatasync, about 4 dirty pages per turn;
median / p99 ms.

| Strategy | p2 1 MB | p2 16 MB | p2 64 MB | p3 1 MB | p3 16 MB | p3 64 MB | Bytes per commit |
| --- | --- | --- | --- | --- | --- | --- | --- |
| S0 in place | 5.8 / 10.1 | 9.8 / 15.2 | 21.7 / 31.1 | 5.1 / 23.9 | 9.0 / 17.9 | 21.0 / 23.2 | 16,411 |
| S1 two slots | 11.5 / 24.7 | 27.1 / 33.3 | 90.8 / 113.9 | 9.7 / 38.5 | 27.2 / 43.3 | 77.8 / 94.3 | size + 48 |
| S2 log | 6.2 / 13.8 | 8.3 / 17.9 | 28.4 / 52.9 | 4.3 / 12.5 | 8.3 / 16.9 | 23.0 / 32.8 | 17,148 average |

About 4-6 ms per commit is F_FULLFSYNC (S1 does two); the rest grows with the image: the page diff
(memcmp of the whole image, about 17 ms at 64 MB, 0.27 ms per MB) in S0 and S2, the whole write and
digest in S1. S2 checkpoints every 64 commits or 8 MB of log.

## 3. Crash consistency

SIGKILL of wasmtime at a uniform time in 0.05-1.5 s, then a check in a fresh wasmtime (recover,
equals_replay); the writer resumes from the recovered file.

| Run | Kills | Consistent | Torn | Lost |
| --- | --- | --- | --- | --- |
| p2 S0 1 MB | 200 | 200 | 0 | 0 |
| p2 S1 1 MB | 200 | 200 | 0 | 0 |
| p2 S2 1 MB | 200 | 200 | 0 | 0 |
| p3 S0 1 MB | 200 | 198 | 2 | 0 |
| p3 S1 1 MB | 200 | 200 | 0 | 0 |
| p3 S2 1 MB | 200 | 200 | 0 | 0 |
| p2 S1 16 MB | 200 | 200 | 0 | 0 |
| p2 S2 16 MB | 200 | 200 | 0 | 0 |

S0's two tears: the header said g, the image held pages of g+1. Under SIGKILL completed pwrites
survive in the page cache, so S0 tears rarely; under power loss it has no protection. Lost = 0 under
SIGKILL checks the recovery logic, not the disk.

## 4. wasmtime serve

`src/commands/serve.rs:49-51, 138-165, 836-842`: p2 reuses an instance once (fresh memory per
request); p3 reuses up to 128 requests, 16 at once per instance, idle instances dropped after 1 s;
up to 1000 concurrent requests, each instance with its own copy. No file locks in WASI, so one
writer must be enforced outside it. A durable segment needs one long-lived owner, not serve.

## 5. File-backed linear memory (source only)

`Config::with_host_memory(Arc<dyn MemoryCreator>)` (config.rs:1661-1669; C API
`wasmtime_config_host_memory_creator_set`) applies to components (component/instance.rs:838,
store.rs:2173), but only to the on-demand allocator (config.rs:2808-2834), not the pooling one serve
prefers; no CLI flag. It cannot make a resumable image (zero-filled start, data segments re-applied,
globals outside memory, the runtime restarts), and a MAP_SHARED file behind it writes back whenever
the OS wants, like S0. A possible trap: vm/memory.rs:569 `unreachable!` on the copy-on-write init
path with a raw memory (likely needs memory_init_cow(false)); not built.

## 6. Surprises

- The emulated mmap loses writes silently; msync does not link.
- On macOS both WASI syncs are F_FULLFSYNC, about 200 times a native fsync.
- p3 stdout: once the pipe's reader closes, a p3 guest that keeps writing spins at 100% CPU forever
  (`probes/epipe2.c`; p2 gets its error at once). Not investigated further.
- wasmtime 47 runs p3 commands with no extra flag.

## 7. Recommendation

S2: an append-only log of dirty 4 KB pages and a commit record with a digest, one pwrite and one
fdatasync per commit (about 5 ms on macOS), checkpoint every N commits and truncate; recovery
replays valid consecutive commits. Group commits when 5 ms is too slow. The page diff (0.27 ms per
MB) dominates large segments: keep them modest or track dirty pages above wasm. S1 only for a few
MB. Never mmap under WASI. fdatasync over fsync. One writer per file, enforced outside WASI; never
the authoritative segment only in memory under serve.
