# Tasks

Every task of 0.1.0. The first group is the plan's (`reference/plan.md`, Tasks 0 to 15), each
checked task pointing to its task in the plan and to the commits of the library's repository that
did it; the other groups are what the published library added to the plan, each pointing to its
commits. A checked task is done in `reference/source/`. Nothing remains for 0.1.0; what follows is
in `design.md`, Open Questions.

## 1. The plan's tasks

- [x] 1.1 The plan's worktree of the superproject, the licences checked (plan: Task 0, the controller's; no commit of the library)
- [x] 1.2 The library: its repository, `build.jam`, `AGENTS.md`, `README.md`, CI workflow, configuration, errors and kinds, and the declared interface of every header (plan: Task 1; commits `81b45cd`, `3bcdd23`)
- [x] 1.3 Numbers: exact text in, JavaScript's text out (plan: Task 2; commit `c2aa05c`)
- [x] 1.4 Traits: scalar codecs, encodings, bytes, dates and policies (plan: Task 3; commit `c8b6f75`)
- [x] 1.5 The heap, its allocator, its strings, the regions and the image's header (plan: Task 4; commit `65fd1d7`)
- [x] 1.6 The tree: nodes, indices, key order, and operations that find their room first (plan: Task 5; commit `cccc777`)
- [x] 1.7 The document, its handles, literals and images: literals, every settable type into the tree and every readable type out of it, `create`, `open`, `format`, `from_image`, `clone` (plan: Task 6; commits `bc1d700`, `6cc1290`, `517a240`)
- [x] 1.8 JavaScript's array methods (plan: Task 7; commit `0603af0`)
- [x] 1.9 `stringify` and `stringify_to` (plan: Task 8; commit `c9a80e0`)
- [x] 1.10 Parsing, whole and in chunks (plan: Task 9; commit `048f4b5`)
- [x] 1.11 Selectors: values delivered as they complete, the grammar and matcher first (plan: Task 10; commits `cfc474a`, `048f4b5`)
- [x] 1.12 The sugar, with exceptions only, and its refusal in a build without them (plan: Task 11; commit `621abdd`)
- [x] 1.13 Node as the oracle: the cases and Node's expected results, then the test that runs them through the document, byte for byte (plan: Task 12; commits `535b03c`, `5ff99d2`)
- [x] 1.14 The page, the examples, the README (plan: Task 13; commit `bc3a3ea`)
- [x] 1.15 Into the superproject, unpublished: the submodule, the README row, and `nativejson-benchmark` as a third-party checkout (plan: Task 14; the superproject's commits `25e332b`, `a1cad2d`, `1aaa098`)
- [x] 1.16 Published as `webcpporg/document` and in the superproject's release notes of 0.1.0 (plan: Task 15, the controller's with the owner's go; repository head `ac29296`)

## 2. Beyond the plan: review findings, limits, lint

- [x] 2.1 Text after the text is extra data, images refuse a lying header, and the review's other findings (commit `85c057b`)
- [x] 2.2 `parse_options::max_document_bytes` defaults to 64 MiB, measured against 2 MB of `[0,0,...]` (commit `c4b99da`)
- [x] 2.3 The lint's findings: tests bind their handles, every public symbol documented, long functions split, Jam comments within their width (commits `ede31ab`, `e24579b`, `fa883bb`, `c6d4abc`)

## 3. Beyond the plan: streams of documents and Boost.Asio

- [x] 3.1 `stream.hpp`: `framing`, `on_error`, `stream_options`, `basic_stream_parser` with `write`, `next`, the push form, `finish`, `reset` and positions; `asio.hpp`: `async_read_document`; `stream_test` and `stream_asio_test` (commit `d66446d`)
- [x] 3.2 Both of Asio's dynamic-buffer models, a buffer of several pieces read without joining, and the Beast example (commit `cabe78e`)
- [x] 3.3 A stream that ended stays ended without another read (`finished()`), for Linux's epoll reactor (commit `53ff964`)

## 4. Beyond the plan: images on the wire, sharing, compaction, containers

- [x] 4.1 Sealed images (CRC-32C, `errc::damaged_document`, `verify`), `image_pool`, `leased_document`, `basic_image_reader`, `shared_pool`, `shared_document` with copy on write, `compact`, `compact_into`, `compact_size`, `reserve`, `capacity`, `containers.hpp` with `by_key`, `async_read_image` and `async_write_image` (commit `c45e07d`)
- [x] 4.2 Several images in one gathered write, `async_write_images` (commit `cabe78e`)
- [x] 4.3 The image's format version is the library's own constant; the header's Boost version becomes reserved bytes; golden images that watch Boost (commit `54f1ea0`)
- [x] 4.4 Examples: streams read by hand, with Asio, Cobalt, Beast and an actor system; images with Asio and Interprocess; threads; what an image refuses; the containers; the comparison with Boost.JSON (commit `8c9d97d`, with `cabe78e`)

## 5. Beyond the plan: portability

- [x] 5.1 Doubles read and written through Boost.Charconv, for Xcode 16's libc++ (commit `bb83062`)
- [x] 5.2 MSVC and Clang before 20: literals nested through `wd::literal`, UTF-8 sources, key extractors and comparators that carry a zeroed byte (commit `5ecacfe`)

## 6. Beyond the plan: benchmarks

- [x] 6.1 Benchmarks of 14 operations against Boost.JSON and JavaScript, natively and in Node and Chrome, over nativejson-benchmark's files and synthetic objects (commit `d1d4a8f`)
- [x] 6.2 Binary transmission (rows 15 to 17: send and receive, sealed, to a Worker) and the sizes on the wire, raw, gzip and brotli (commit `25e3545`)
- [x] 6.3 The benchmarks' page as valid HTML (commit `bf4ed5c`, the superproject's `93d0d4b`)

## 7. Beyond the plan: the JavaScript module and the npm package

- [x] 7.1 `js/binding.cpp` and `js/webcpp-document.mjs`: `Document` and `StreamParser` over embind, tested in Node against the oracle's cases and in headless Chrome (commit `ac9dc92`)
- [x] 7.2 `js/shared/`: a second module with Wasm Workers on a `SharedArrayBuffer`, tokens, `makeMutable`, `join` and `leave`, an example in headless Chrome; the JavaScript API as the own lane `js` (commit `cf8e494`; the superproject's `7b103aa`, `65d5a6f`)
- [x] 7.3 `js/npm/`: the package `@webcpp/document` staged from the release build, its declarations and README, tested from its tarball on Node 22, 24 and 26 with TypeScript; `.github/workflows/npm.yml` (commit `778c52d`)

## 8. Beyond the plan: the page

- [x] 8.1 The page leads with what C++ gives JavaScript: new sections on the JavaScript module, the containers, images on the wire, sharing and the comparison with Boost.JSON; the losses stated beside the gains (commit `1e31865`)
- [x] 8.2 A page title within the site's 60 characters (commit `7827350`)

## 9. Publication

- [x] 9.1 The maintainer's public address in `meta/libraries.json` (commit `ac29296`)
