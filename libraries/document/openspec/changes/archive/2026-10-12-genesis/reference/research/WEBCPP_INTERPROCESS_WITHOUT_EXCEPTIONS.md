---
title: "Boost.Interprocess without exceptions: what an external buffer can use (spike of 2026-10-12)"
tags: [webcpp, xactor, document, interprocess, exceptions, wasm]
status: active
created: 2026-10-12
---

# Boost.Interprocess without exceptions

Question (owner, 2026-10-12): can xactor::document use Boost.Interprocess without exceptions on
wasm, where the browser hands over the memory and nothing is allocated but inside an external
buffer, while native, which needs concurrency, requires exceptions?

Boost 1.92 (Homebrew, /opt/homebrew/opt/boost/include). Spike sources:
`~/.buzz/RESEARCH/WEBCPP_DOCUMENT_SPIKE.cpp` (the final one).

## What does not compile without exceptions

Interprocess has its own macros for a build without exceptions (`BOOST_INTERPROCESS_TRY`,
`_CATCH`, `_RETHROW`, `_CATCH_END` in `detail/workaround.hpp`), used by `segment_manager.hpp` and
`managed_open_or_create_impl.hpp`, but 163 bare `throw` remain in 39 headers. On the external
buffer's path, clang and GCC reject these at parse time with `-fno-exceptions`, so including the
header is enough to fail:

| Header | Line | Bare throw |
| --- | --- | --- |
| `managed_external_buffer.hpp` | 76, 90 | `interprocess_exception` when create or open fails |
| `segment_manager.hpp` | 152, 273 | `bad_alloc` in `allocate_aligned`, `allocation_command` |
| `allocators/allocator.hpp` | 162 | `bad_alloc` on size overflow |
| `sync/spin/recursive_mutex.hpp` | 96, 115 | through `sync/mutex_family.hpp`, even for `null_mutex_family` |

With exceptions on in the compiler but none at link (wasi-sdk's default): `undefined symbol:
__cxa_throw`.

## What does compile without exceptions

`offset_ptr.hpp`, `mem_algo/rbtree_best_fit.hpp` (Interprocess's best-fit memory algorithm, with
coalescing; `allocate` returns null when it cannot), `sync/null_mutex.hpp`, given a mutex family of
our own (`struct { using mutex_type = null_mutex; using recursive_mutex_type = null_mutex; }`)
instead of `mutex_family.hpp`. On top of them a 15-line allocator of our own (allocate_aligned from
the algorithm, `boost::throw_exception(std::bad_alloc())` on null) serves Boost.MultiIndex and
`boost::container::basic_string`.

Measured, all without exceptions: native clang (arm64), gcc-15, wasip2 (wasmtime), emscripten
(node). Each builds 100 nodes, erases 50, copies the buffer with memcpy, destroys the original,
finds a node in the copy, and allocates in the copy: the same result on every lane (51 nodes,
57,888 bytes free, an oversized allocation answered with null).

## The image is the same bytes on 64-bit native and wasm32, with two rules

`offset_ptr<T, std::int64_t, std::uint64_t, 8>` and `rbtree_best_fit<..., 16>` give the same layout
on every lane. The first digests differed in 10 bytes only: the unused part of a short string's
internal buffer, copied from a temporary on the stack (they held stack addresses, which also leak
the process's memory to whoever reads the image). With two rules the 64 KB image's FNV-1a digest is
`8d80cfaf86a669cb` on native clang (twice), gcc-15, wasip2 and emscripten:

1. the allocator zeroes what it allocates and what it frees;
2. a node is built in place (`emplace` with a constructor), never copied from a temporary.

## Consequences for the design

- Without exceptions (wasm): `rbtree_best_fit` in the external buffer, our allocator over it. Not
  `managed_external_buffer`, `segment_manager`, `interprocess::allocator` or `mutex_family.hpp`.
- With exceptions (native, concurrent): the whole of Interprocess is available
  (`managed_shared_memory`, its mutexes).
- One document type on every target is possible: a document is a contiguous block, header plus an
  `rbtree_best_fit` heap, which a native shared segment holds as one more block.
- `boost::throw_exception` on a full heap aborts a program without exceptions, so a document must
  find room before it inserts (probe with the algorithm's null-returning allocate, grow the buffer
  if needed) and report a full fixed buffer as an error value.

## basic_managed_external_buffer with WebAssembly's exceptions (same day)

`basic_managed_external_buffer<char, AllocationAlgorithm, IndexType>` is a layer over a memory
algorithm, not an alternative to one: it places a `segment_manager` at the start of the user's
buffer, and the segment manager allocates through `AllocationAlgorithm`, whose default is
`rbtree_best_fit` (the other one Interprocess ships is `simple_seq_fit`). What the layer adds:
`interprocess::allocator` ready to use, named and anonymous construction, `grow(extra)` (the buffer
was enlarged by its owner beforehand), `shrink_to_fit`, `zero_free_memory`, `check_sanity`,
`get_free_memory`, `all_memory_deallocated`. It needs exceptions (the table above).

WebAssembly now has exceptions, and every toolchain webcpp pins supports them:

- wasi-sdk 34 ships an `eh` sysroot (libunwind, libc++abi) beside `noeh`:
  `-fwasm-exceptions -mllvm -wasm-use-legacy-eh=false ... -lunwind`. Without the `-mllvm` flag
  clang emits the legacy encoding, which wasmtime 47 refuses ("legacy_exceptions feature required
  for try instruction"); wasmtime runs the new one with `-W exceptions=y`.
- Emscripten 6.0.11: `-fwasm-exceptions`; node 26 runs it with no flag.

Spike `~/.buzz/RESEARCH/WEBCPP_DOCUMENT_SPIKE_MANAGED.cpp`: a 16 KB external buffer, a
multi_index of nodes through `interprocess::allocator`, 1000 inserts; each `bad_alloc` is caught, a
buffer twice the size receives a memcpy of the image, the heap is reopened on it (`open_only`) and
`grow`n. Same result on native clang, wasip2, wasip3 and emscripten: "nodes 1000, grown 4 times to
262144 bytes, k0777 = 7.77, sane 1".

Gotcha: `construct<T>(anonymous_instance)` does not compile with `null_index` in 1.92 (the named
path is instantiated and asks the index for `insert_check`); allocate and placement-new instead.

## A file mapping under wasmtime (same day)

Question (owner): on WASI under wasmtime, can a file mapping hold the process's memory, so that
everything written lands in the file?

- Inside the guest there is no mapping. wasi-libc's `mmap` is an emulation
  (`-D_WASI_EMULATED_MMAN`, `-lwasi-emulated-mman`): it copies the file into memory it allocates.
  `MAP_SHARED` with `PROT_WRITE` is accepted without an error, yet nothing written reaches the
  file (spike `~/.buzz/RESEARCH/WEBCPP_WASI_MMAP_SPIKE.c`, wasi-sdk 34, wasmtime 47: the file stays
  empty after the write and `munmap`); `msync` does not exist (link error). A silent loss, so
  webcpp must never rely on it.
- A linear memory is one array the engine owns. wasmtime's CLI (`run`, `serve`) has no option to
  back it with a file (`-W help`: `max-memory-size`, `shared-memory`, `multi-memory`,
  `memory64`, nothing about files).
- An embedder can: wasmtime as a library lets the host supply the linear memory (its memory-creator
  hook), which could be a `MAP_SHARED` mapping of a file in the reserved address range. That needs
  a host program of our own instead of the wasmtime CLI. To verify before relying on it: the hook's
  current API and whether it covers components.
- Mapping the whole memory still does not make a process resumable: the stack pointer and the other
  globals and the call stack live outside linear memory, and a new instance re-applies its data
  segments and re-initialises the C runtime's heap over the mapped bytes. Two instances on one file
  would overwrite each other's stacks and heaps, and a crash mid-write leaves a torn file.
- `wasmtime serve` instantiates the component afresh for each request, so nothing in memory outlives
  a request anyway: state that must last is written out explicitly.
- Multi-memory (the segment as memory 1, backed by the file) is not reachable from C++ with ordinary
  pointers today.

Recommendation, proposed to the owner and not yet decided: persist the segment, not the process. The document's image
is relocatable bytes, so a turn's commit writes what changed (pwrite, then wasi:filesystem's
sync-data) and a start reads the file back into a buffer. The same works in the browser (OPFS's
synchronous access handle, in a worker) and natively beside the real mapping (`managed_mapped_file`).
