## Context

webcpp::document was specified (`reference/spec.md`) and planned task by task
(`reference/plan.md`) on 2026-10-12, as the JSON document the other webcpp libraries are to move
to from Boost.JSON, and was built and published the same day: 40 commits of its repository, all
as `R`, from `81b45cd` ("the library, its configuration, errors and kinds") to `ac29296` ("the
maintainer's public address"). The plan's 16 tasks (0 to 15) produced the core: the model, the
tree, the handles, the array methods, `stringify`, `parse`, the selectors, the sugar, the Node
oracle, the page.

The published library went well beyond the plan. This document records, retroactively, the
decisions 0.1.0 is built on, and says in each which part the plan did not have:

- streams of documents (`stream.hpp`: NDJSON, JSON text sequences, concatenated JSON) and Boost.Asio
  readers over them, with Cobalt, Beast and xactor examples (`d66446d`, `8c9d97d`, `cabe78e`);
- images on the wire: sealed images (CRC-32C), pools of blocks, leased documents, an image reader,
  `async_read_image`, `async_write_image`, `async_write_images` (`c45e07d`, `cabe78e`);
- shared documents between threads and processes (`shared_pool`, `shared_document`), with a
  Boost.Interprocess example and a threads example (`c45e07d`, `8c9d97d`);
- compact images (`compact`, `compact_into`, `compact_size`), `reserve` and `capacity`, and the
  standard containers' interface (`containers.hpp`, `by_key`) (`c45e07d`);
- the image's format version as the library's own constant, with golden images that watch Boost
  (`54f1ea0`);
- a default of 64 MiB for a parse's image (`c4b99da`) and the review's other findings (`85c057b`);
- doubles read and written through Boost.Charconv (`bb83062`);
- benchmarks against Boost.JSON and JavaScript, natively, in Node and in Chrome, and sizes on the
  wire (`d1d4a8f`, `25e3545`);
- a JavaScript module over WebAssembly (`ac9dc92`), a second module whose memory is a
  `SharedArrayBuffer` shared by the programmer's own Workers (`cf8e494`), and the npm package
  `@webcpp/document` with TypeScript declarations and a publishing workflow (`778c52d`);
- the page rewritten around "what C++ gives JavaScript" (`1e31865`), and portability work for MSVC,
  Clang before 20 and Xcode 16's libc++ (`5ecacfe`, `bb83062`).

What the owner asked for next is not part of 0.1.0 (see Non-Goals).

## Goals / Non-Goals

**Goals:**

- A JSON document whose behaviour is JavaScript's: the same key order, the same text out of
  `JSON.stringify`, the same array methods and index rules, proven by running the same cases in
  Node and in C++ and comparing bytes.
- Exact numbers by default (what is stored is what was sent), written as JavaScript writes
  numbers; JavaScript's own doubles when asked.
- A core that never throws and never leaves a half-made change: every operation completes or
  changes nothing, a full heap included, in a build with exceptions and in one without.
- One relocatable image per document: copied, sent, opened in place, shared, made durable by its
  owner, the same bytes on every target.
- Streaming in both directions, in bounded memory.
- Native, wasip2, wasip3 and Emscripten, and a JavaScript module and npm package over the
  WebAssembly build.
- Honesty about cost: the page says where document loses to Boost.JSON and to JavaScript, with
  measurements.

**Non-Goals:**

- Speed of parse, stringify or lookup against Boost.JSON or V8: document is not a fast JSON tree,
  and 0.1.0 does not tune it. The benchmarks name where to begin.
- `toJSON`, replacers and revivers (other than `revive_dates` and `revive_bytes`), Map, Set,
  RegExp, BigInt and fixed 256-bit integers, prototypes, getters and symbols.
- JSON Pointer and JSONPath beyond the selectors' subset; schemas (json_schema's).
- Authentication of an image: a seal is a checksum against accidental damage, and an image is
  trusted input.
- A document inside a shared segment together with other documents, a commit protocol and
  durability, and the recovery of the references a dead process held.
- A lone surrogate in JSON text: Boost.JSON's parser cannot deliver one, and the limit is
  documented rather than worked around.
- Moving xstate, trystero and json_schema from Boost.JSON to document (their own proposals).

## Decisions

### D1. A library of webcpp's own, with Boost.JSON's SAX parser and nothing else of it

document ports nothing: its evidence is JavaScript itself (Node 26), run as an oracle. It uses
Boost.JSON's `basic_parser` (a SAX handler builds the tree) and none of its value types, since the
reasons it exists are properties of `boost::json::value` (key order, `erase`, numbers as doubles,
hashed objects, pointers). Boost.JSON's parser is not header-only, so the build links
`/webcpp//boost_json`. Alternative rejected: wrapping `boost::json::value`, which cannot be made
relocatable or exact.

### D2. JavaScript's behaviour is the contract, and Node is the oracle

A case (`test/fixtures/cases/<name>.json`) is a JSON text, an indent and a list of operations
(`set`, `erase`, `push`, `pop`, `shift`, `unshift`, `splice`, `reverse`, `set_length`,
`assign_from`); `test/oracle/oracle.mjs` runs it with `JSON.parse`, the operations in JavaScript
and `JSON.stringify` and writes `test/fixtures/expected/<name>.json`, which is committed; the test
`oracle_cases` runs each case through the document and must give Node's bytes and return values.
54 cases are written by hand and 200 generated from a fixed seed. Where document differs on
purpose (a copy instead of an alias, `undefined` erased, dense arrays, exact numbers, a lone
surrogate, `__proto__`) no case holds the difference, and the page lists each in a table.

### D3. One image: a header, a heap, a tree, every pointer an offset

An image is a 64-byte header and a Boost.Interprocess `rbtree_best_fit` heap used directly, with an
allocator and strings of webcpp's own, because `managed_external_buffer`, `segment_manager`,
`allocators/allocator.hpp` and `sync/mutex_family.hpp` throw bare and do not compile without
exceptions. The heap holds a Boost.MultiIndex container of nodes with three ordered indices (by id,
by parent-group-ordinal, by parent-key), never hashed. Every pointer is an
`offset_ptr<..., std::int64_t, std::uint64_t, 8>` and every size a `std::uint64_t`, so the image
works at any 16-aligned address and has the same bytes on 64-bit native and wasm32. The allocator
zeroes each block it hands out and each block it frees, and nodes are built in place, so that an
erased value leaves none of its bytes and the image is a function of the operations alone.

### D4. The format version is the library's own, and a test watches Boost

The header does not carry the Boost version. Boost releases often and the structures the image
holds rarely change; changing the format is document's own decision, `format_version` (1), which
the library raises, with a migration from the previous version, when the layout changes. A Boost
upgrade does not change it. `test/format_test.cpp` compares the compact image of a fixed document,
under each traits, with golden images (`test/fixtures/format/*.image`) on every toolset and fails
when a release changes a byte; writing the goldens again is right only with a raised version.
(The plan put a Boost version in the header; `54f1ea0` replaced it by reserved bytes.)

### D5. The core never throws: every operation finds its room first

An operation computes what it will allocate (each node's block, bounded by
`node_probe_bytes = sizeof(node) + 128` because Boost.MultiIndex does not name its internal node
type; each string's exact size), probes the heap for each block with the non-throwing `allocate`
and frees the probes at once; if a probe fails, an owned document grows and probes again and an
external one returns `errc::capacity`; then it allocates for real (the algorithm is deterministic,
so the blocks just probed are found again) and rearranges with steps that cannot allocate.
Boost.MultiIndex erases an element whose `modify` throws, so a `modify` only swaps prepared values.
The allocator reports a failure that survives the probe through `boost::throw_exception` (a bug,
never a run-time event). The text `stringify` returns, the key a handle of a missing member holds
and the parser's frames live on the process heap, whose `std::bad_alloc` is the one failure that
is not a value.

### D6. The sugar throws, and exists only with exceptions

`h = x`, `h = other`, `as<T>()`, `as_string()`, `as_bool()`, `parse_or_throw`, the document's
default constructor and copy constructor are each a core function followed by `detail::raise`,
each constrained by `detail::exceptions_enabled<T>` (false when `WEBCPP_DOCUMENT_NO_EXCEPTIONS`
is defined), so that a call in a build without exceptions does not compile and the diagnostic
names the constraint. A test (`no_exceptions`) holds the compiler's diagnostics. The exception
types (`error`, `syntax_error` with the byte offset, `type_error`, `range_error`,
`capacity_error`, `image_error`) exist only with exceptions. Assignment to a handle writes to its
place and never rebinds it, so a handle is not a regular value (`std::swap` of two handles writes
through both).

### D7. Traits choose the stored types, and are part of the image

`default_traits` stores `bool`, `std::int64_t`, `decimal128_t`, UTF-8, `date_ms`, base64 bytes,
JavaScript's key order and dense arrays; `js_traits` derives from it and changes `number` to
`double`, `string` to UTF-16 and `bytes` to `javascript_bytes`. The concept `document_traits` fixes
`boolean`, `integer` and `date` and lets `number` be any `stored_scalar` with a `scalar_codec` and a
`number_codec`. An FNV-1a hash of each codec's name and size and of the encoding and policy names
is part of the header's layout tag, so an image written with other traits is refused
(`incompatible_document`), never read wrongly.

### D8. Numbers are stored by value, exactly, and written as JavaScript writes them

A number is an `integer` when it is an integer that fits `std::int64_t`, else a normalised
`number` (no trailing zeros), so `1`, `1.0` and `1e0` are one stored value. Parsing reads the text
into digits, never through a double; more than 34 significant digits or an exponent outside
`decimal128_t`'s range is `errc::range`, and `1e999` is stored exactly. Writing follows ECMA-262's
Number::toString on the stored digits (four rows of a table on `k` and `n`). `js_traits` round as
`JSON.parse` does and write NaN and infinities `null`. A double is read and written through
Boost.Charconv (`bb83062`), since Xcode 16's libc++ deletes the floating-point `std::from_chars`.

### D9. Key order is a policy of the traits; JavaScript's is the default

`javascript_order` puts array-index keys (the canonical decimal spelling of 0 to 4,294,967,294)
first, ascending, then the other keys in creation order; assigning keeps a key's place and erasing
and re-creating puts it where a new key goes. `insertion_order` and `sorted` (by UTF-16 code
units, as `sort()` and RFC 8785 order them) are the other two. A repeated key in parsed text keeps
the last value in the first one's place.

### D10. A handle names a place and resolves it at every call

A handle (`value`, `const_value`) holds the document's address and node ids, never a pointer into
the tree, and is one of three things (a node; a missing member or element of a container that
exists; nothing), so navigation never fails and a handle stays valid when the image grows and
moves. It is invalid once its document is moved from or destroyed, as a container's iterators
would be; a `std::string_view` read from a value is valid until the document's next change.

### D11. Copies, never aliases

`a.x = b.y` in JavaScript makes one object of two; a document is a tree, so `set` of a handle or a
document deep-copies (reading the source before it changes anything, so a value may be copied into
its own subtree), `splice` inserts a copy of an array spliced into itself, `undefined` erases a
member and stores `null` in an element, arrays are dense, and `-0` is `0`.

### D12. Streaming: a parser with selectors, a sans-IO stream parser, sinks, Asio

`basic_parser` takes text in chunks of any size (Boost.JSON's `write_some` with `more`), builds
the document node by node by ids so that it may move between chunks, and delivers values that a
JSONPath-subset selector matches when they complete: `on` removes the value (blocks zeroed and
freed: a million elements need the memory of one), `observe` keeps it. `basic_stream_parser` frames
documents (`concatenated`, `lines` for NDJSON, `rfc7464`), with an error policy (`stop`, `skip`)
and positions (`line`, `offset`, `error_line`, `error_offset`, `error_cause`), sans-IO: a
`write`/`next` form and a push form. `stringify_to` writes the same bytes as `stringify` in
pieces to a sink. `asio.hpp` is the one header that includes Boost.Asio and builds natively only;
it takes both of Asio's dynamic-buffer models. (Streams, Asio and the examples are beyond the
plan.)

### D13. Images travel: seal, pools, leases, shared documents

A seal is CRC-32C (Boost.CRC) of the image with the seal's eight bytes zero, stored in header
offset 48 with the high bit set; any change clears it; opening, `from_image`, the reader and
`shared_document` check it first (`errc::damaged_document`) unless `verify::none`. It is a
checksum against accidental damage, not authentication; an image is trusted input and the page
says so wherever images are discussed. `image_pool` allocates `blocks` blocks of `block_bytes`
once and leases them (backpressure is `errc::capacity`); `leased_document` is an external document
that owns its lease; `basic_image_reader` is sans-IO and exposes `prepare`/`commit` so a socket
reads straight into the block. `shared_pool` formats a region (the process heap or a shared-memory
segment) as a header plus blocks, each with a 64-byte header holding an address-free lock-free
`std::atomic<std::uint64_t>` count, works through offsets, and `shared_document` is
`std::shared_ptr` for a document with `make_mutable` (copy on write), `release`/`adopt` across
processes and `shared_pool::drop` by offset. The count lives in the block header, never in the
image, so the image stays the document's content. Writers are not synchronised by the library.

### D14. Compact images

`compact()` walks the tree once in document order and builds a fresh image in which ids and
insertion numbers are renumbered from the walk, blocks lie one after another with no hole, and the
size is exactly that of the blocks; two documents of equal content, whatever their histories,
compact to byte-identical images, and a compact image compacts to itself. `compact_size()` is exact
and allocates nothing (the sizes come from `rbtree_best_fit`'s arithmetic and a scratch heap on the
stack, so the figure follows the Boost release); `compact_into(buffer)` serves documents that
cannot reallocate. The copy is built before the swap, which is what makes the guarantee strong.

### D15. The standard containers' interface over the same tree

`as_object()` gives a map-like view (`find`, `contains`, `count`, `insert`, `insert_or_assign`,
`try_emplace`, `erase`, `clear`, bidirectional iteration) in the traits' order, and `by_key()` the
same members in key order by UTF-16 code units with `lower_bound`, `upper_bound` and `equal_range`
(a prefix query is two bounds, with no sort); `as_array()` a vector-like view. The views are
`std::ranges::view`s and borrowed ranges whose iterators hold ids, so an insertion and any growth
of the image leave them valid.

### D16. A JavaScript module over the WebAssembly build

`js/binding.cpp` (embind) registers `NativeDocument`, `NativeStream`, `parseText`, `openBytes` and
`heapBytesInUse`; every operation returns an `Outcome` (a code, a message, a value) and nothing
throws into JavaScript. `js/webcpp-document.mjs` wraps it in `Document` (addressed by paths: arrays
of string keys and non-negative integer indices) and `StreamParser`, turns `errc` into JavaScript's
`SyntaxError`, `RangeError` or `TypeError`, frees through `FinalizationRegistry` as a safety net
and `free()` to release a document at once. `js/shared/` is a second build (Wasm Workers on pthreads, 128 MiB of
fixed shared memory, a pool of 64 blocks of 512 KiB, at most 32 joined Workers and 1,024 tokens in
flight) whose documents are `shared_document`s that Workers adopt through tokens. `js/npm/` stages
`@webcpp/document` (release `-O3`, hand-written `webcpp-document.d.mts`, Node 22 and newer) and
tests the tarball as a user receives it; `.github/workflows/npm.yml` builds it on a tag, tests it
on Node 22, 24 and 26 and publishes with provenance by npm trusted publishing (no token).

### D17. Tests, examples and the page are the specification's evidence

Tests are `lightweight_test` programs (29 files), each built without exceptions as well unless it
needs them, on native (Clang, GCC), wasip2, wasip3 and Emscripten; every public header is compiled
alone; allocation-counting tests replace the global `operator new`; the page's programs are
compiled, run and compared with their `.expected`; the JavaScript API is the own lane `js`
(Node's runner, the npm tarball, two headless-Chromium examples). The page shows each example by
tag and each output by line range, and every `@see "<title>"` of a Doc Comment names a section.

## Risks / Trade-offs

- [A Boost release changes the layout of `rbtree_best_fit` or of MultiIndex's nodes] → the golden
  images of `format_test` fail on every toolset; the version is raised with a migration (D4).
- [An image from an untrusted source crashes a reader] → the header, the heap's own check and the
  container offset are verified, but not every offset inside the tree: an image is documented as
  trusted input, a seal is not authentication, untrusted data enters as JSON text (D13).
- [A tree is slower and larger than Boost.JSON's] → measured and published: parse 95 times
  Boost.JSON's time on `canada.json` (209 ms against 2.19 ms), an image 8 to 17 times the text,
  192 bytes a node; hence `max_document_bytes` defaults to 64 MiB and the page states where
  document loses (D17).
- [A process that dies holding references leaves their blocks counted] → documented, as
  Boost.Interprocess's `shared_ptr` leaves them; recovery is the program's.
- [The shared JavaScript module reads internals of Emscripten 6.0.11 (`createWasm`, `wasmMemory`,
  `updateMemoryViews`)] → the module is tied to that version until checked against another.
- [A compiler applies CWG 1467 to every class (MSVC, Clang before 20)] → `wd::array{wd::array{1}}`
  is `[1]` there and `[[1]]` elsewhere; the Doc Comment of `array` states the rule, the tests nest
  through `wd::literal`.
- [Boost.Asio builds natively only] → `asio.hpp` is excluded from the convenience header, compiled
  alone natively, and every program that includes it is declared native.

## Migration Plan

None: a new library. Programs that use Boost.JSON are not migrated by this proposal; the page
compares the two (`doc/boost-json.adoc`, `example/document_vs_boost_json.cpp`).

## Open Questions

- O1. Moving xstate, trystero and json_schema from Boost.JSON to document: each is its own
  proposal.
- O2. A third storage, a document inside a shared segment of several documents, and the commit and
  durability that go with it: not in 0.1.0.
- O3. Recovery of the references a dead process held in a shared pool.
- O4. Tuning: the benchmarks' profile names the lookup of a node by id behind every handle, the
  renumbering of positions on a splice and the image that `pop` allocates as the places to begin.
- O5. The page's "Memory" section of the JavaScript module says `reserve`, `shrink_to_fit` and
  `compact` are not in the module, while `Document` has `reserve`, `capacity` and `byteLength`
  (`reference/STATUS.md`, open items).
