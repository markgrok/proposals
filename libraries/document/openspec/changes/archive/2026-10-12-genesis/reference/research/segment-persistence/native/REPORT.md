---
title: "Segment persistence: the native track"
tags: [webcpp, persistence, native, macos, linux]
status: active
created: 2026-10-12
---

# Segment persistence: the native track

macOS (APFS, internal Apple SSD, M5 Max, Apple clang 21) and Linux (ext4 on a virtio disk with
write-back cache, Docker named volume `segpersist`, Fedora 42 arm64, kernel 6.12.76-linuxkit,
clang 20). Code: `seg.cpp`, `crash.py`, `run_crash.sh`, `run_crash_inplace.sh`, `bench.sh`;
every raw number in `results/*.jsonl`. `docker volume rm segpersist` removes the Linux volume.

Strategies: S0 (dirty pages in place, header, one sync); S1 (whole image into the free slot) and
S1d (only pages that differ from that slot); S2 (WAL of dirty pages, append and truncate, checkpoint
every 64 commits), S2p (a preallocated log of written zeros, reused from 0), S2mmap (image through
MAP_SHARED, msync at checkpoint); S3raw (turns in a raw MAP_SHARED mapping), S3bip (the heap inside
Boost's basic_managed_mapped_file); tear (one 64 MB pwrite per step). Digests are a page-hash tree
(3 ms at 64 MB against FNV's 64 ms).

## 1. Semantics

- macOS fsync: to the drive, not to permanent storage, possibly reordered (fsync(2); XNU
  vfs_syscalls.c fsync_common, VNOP_FSYNC(MNT_WAIT)). fdatasync: exported by libsystem_kernel
  (SYS_fdatasync 187), not in the SDK; MNT_DWAIT, no drive flush.
- F_FULLFSYNC: fsync, then the drive flushes its cache; also a barrier (fcntl(2); kern_descrip.c).
  The only macOS call that reaches the media. F_BARRIERFSYNC: ordering, not durability.
- macOS msync MS_SYNC: pages written back synchronously (kern_mman.c), no drive flush:
  F_FULLFSYNC must follow. APFS is closed: whether fsync alone pushes dirty mmap pages is not
  verifiable, so msync first.
- Linux fsync flushes the disk cache (ext4_sync_file, blkdev_issue_flush); fdatasync skips
  metadata except what reading needs, a size change included; msync MS_SYNC is
  vfs_fsync_range(..., 1); MS_ASYNC does nothing since 2.6.19 (mm/msync.c).
- Boost managed_mapped_file::flush() (managed_mapped_file.hpp:268) is mapped_region::flush(0, 0,
  async=true), msync(MS_ASYNC) (mapped_region.hpp:746-753): durable on neither OS, nothing at all
  on Linux (0 bytes measured). Boost has no fsync or F_FULLFSYNC path.
- Docker: whether the hypervisor passes the VM's flushes to the Mac's SSD is not verified; Linux
  timings describe this VM, not a server.

## 2. Cost

Median / p99 commit latency in microseconds, one turn per commit, 200 commits after 20 warm-up,
about 3.8 dirty 4 KB pages per commit.

macOS:

| Config | 1 MB | 16 MB | 64 MB |
| --- | --- | --- | --- |
| s0 fsync | 102/5457 | 471/7602 | 3409/6118 |
| s0 F_FULLFSYNC | 7908/13908 | 9460/14352 | 9725/14768 |
| s0 F_BARRIERFSYNC | 404/6555 | 849/10322 | 5140/10904 |
| s1 fsync | 188/10589 | 4571/12445 | 11672/16939 |
| s1 full | 14956/24847 | 19452/29028 | 23217/30799 |
| s1d fsync | 138/9701 | 1237/12959 | 8322/12605 |
| s1d full | 14796/25390 | 18090/29427 | 20772/26593 |
| s2 fsync | 68/22713 | 424/10662 | 2725/7133 |
| s2 full | 8539/21250 | 9686/29164 | 10844/27672 |
| s2p fsync | 49/11604 | 398/8547 | 2297/5897 |
| s2p full | 7393/15593 | 8541/17582 | 8900/15026 |
| s2p barrier | 383/8485 | 790/9848 | 1828/7431 |
| s2mmap full | 8503/24620 | 10146/32000 | 5865/16852 |
| s3raw msync | 60/422 | 69/534 | 130/356 |
| s3raw msync+full | 7446/15235 | 7520/12775 | 4327/10442 |
| s3bip flush() | 11/24 | 18/2080 | 24/42 |
| s3bip msync+full | 7362/9784 | 7466/9752 | 4334/10637 |

Linux (fdatasync, VM):

| Config | 1 MB | 16 MB | 64 MB |
| --- | --- | --- | --- |
| s0 | 145/167 | 528/611 | 1623/1802 |
| s1 | 384/412 | 3884/4620 | 13771/20412 |
| s1d | 226/265 | 1354/1418 | 4712/4887 |
| s2 (append) | 250/1450 | 643/963 | 1728/2079 |
| s2p | 97/224 | 440/578 | 1558/1751 |
| s2mmap | 253/605 | 666/2475 | 1742/2150 |
| s3raw msync | 118/142 | 117/155 | 117/154 |
| s3bip flush() | 0/0 (writes nothing) | | |
| s3bip msync | 103/244 | 104/260 | 103/255 |

The page compare alone: 25, 380 and 1430 us at 1, 16 and 64 MB. Bytes per commit: 18 KB written by
s0, s1d and s2; the OS reports 63 KB on macOS (16 KB VM pages) and 22 KB on Linux.

## 3. Crash consistency

SIGKILL 2-60 ms into each run, 1 MB, fsync; each trial continues from the last one's files.
Verdicts consistent / torn / latent (equals_replay passes, bytes differ from a byte-exact replay) /
lost.

| Strategy | macOS | Linux |
| --- | --- | --- |
| s0 | 181/19/0/0 (200); 271/29/0/0 (300) | 199/1/0/0; 297/3/0/0 |
| s1, s1d, s2, s2p, s2mmap | 200/0/0/0 each | 200/0/0/0 each |
| s3raw | 192/0/8/0; 275/7/18/0 | 199/0/1/0; 280/6/13/0 |
| s3bip fsync | 184/3/13/0; 258/7/35/0 | 199/0/1/0; 286/2/12/0 |
| s3bip flush() | 163/8/29/0; 236/11/52/0 | 196/1/3/0; 295/2/2/0 |
| tear: one 64 MB pwrite | 200 whole | 53 whole, 147 split |

Of the bad states, many crashed the next writer (macOS s0: 12 of 29; s3bip: 15 of 35, with flush()
27 of 52; Linux s3raw: 9 of 13 latent ones, SIGSEGV); the rest ran on from a wrong state. A kill
proves recovery at any instruction boundary and that Linux cuts a single write short; it proves
nothing about power loss (writes survive in the page cache).

## 4. Power loss

Not testable here. macOS: only F_FULLFSYNC is durable; S1 with fsync may persist a header before its
slot (the digest then falls back to the older generation: recent commits lost, nothing corrupt); S2's
checkpoint needs image-before-log-reset ordering, which fsync does not give on macOS. Linux:
fsync, fdatasync and msync(MS_SYNC) flush when the device honours flushes; a writeback error is
reported once, so an fsync error is fatal. Next step would be dm-log-writes replay; not done.

## 5. Surprises

- Linux splits a single pwrite under SIGKILL (147 of 200; mm/filemap.c:4369); macOS never did.
- equals_replay accepts in-place heaps that crash the next turn: verify byte-exact.
- managed_mapped_file::flush() is MS_ASYNC.
- An appending log costs up to 2.5x a preallocated one on Linux (fdatasync commits the size).
- macOS p99 is 5-30 ms even with plain fsync; F_FULLFSYNC 7-10 ms per call.

## 6. Recommendation

S2p: turns in private memory; each commit appends its dirty pages and a commit record (digest over
its pages) as one buffer to a preallocated log of written zeros; replay only a consecutive chain
from the image's generation; checkpoint with pwrite, sync the image, then reset the log. Sync:
Linux fdatasync; macOS F_FULLFSYNC for commits that must survive power loss, F_BARRIERFSYNC where
ordering suffices (with periodic F_FULLFSYNC or group commit), and F_FULLFSYNC before a log reset.
Never mutate the durable image in place (S0, S3, managed_mapped_file), never rely on
managed_mapped_file::flush(); handle short writes; checksums decide validity; verify recovery
byte-exact. Kernel dirty tracking (mprotect, soft-dirty) could replace the compare; not measured.
