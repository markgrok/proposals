## Why

A web application speaks JSON with JavaScript, and JavaScript decides what that JSON means: the
order in which an object's keys come out, what `JSON.stringify` writes for a number, what `splice`
removes. A C++ program that must agree with a JavaScript peer byte for byte needs a JSON document
that makes the same decisions. Boost.JSON, the JSON library C++ has, makes other choices (measured
on Boost 1.92): it keeps keys in insertion order, its `object::erase` moves the last member into
the gap, its numbers are doubles (`0.1000000000000000000000000000000001` writes back as `1E-1`),
its object index hashes keys (a crafted 4 MB input took 9.23 s to parse), and a `boost::json::value`
holds pointers, so it can neither live in a memory segment nor travel as bytes.

webcpp needs a JSON document that is JavaScript's in behaviour and C++'s in memory: exact in
decimal, in one relocatable binary image that can be copied, sent, opened in place, shared between
threads and processes, and handed to a Worker without a parse. It is for the programs that sit
between a JavaScript peer and a native or WebAssembly peer (a server, a Worker, a runtime), and
other webcpp libraries (xstate, trystero, json_schema) are to move from Boost.JSON to it.

## What Changes

- **New library `webcpp::document`**, webcpp's own (not a port), header-only C++20: a JSON
  document whose behaviour is JavaScript's. What `JSON.parse`, property access, assignment,
  `delete`, the array methods and `JSON.stringify` do in JavaScript, document does in C++, with the
  same key order and the same text out. Node is its oracle: the same cases run in Node and in C++
  and the bytes must agree.
- **The core never throws.** Every operation that can fail returns a `boost::system::result`;
  every operation finds its room in the image before it allocates, so a full heap is an error value
  and the document is unchanged. The sugar that throws as JavaScript does (`v = x`, `as<T>()`,
  `parse_or_throw`) exists only in a build with exceptions, behind the constraint
  `detail::exceptions_enabled`.
- **Traits choose what is stored**: `default_traits` (`std::int64_t`, exact `decimal128_t`
  numbers, UTF-8, base64 bytes, JavaScript's key order) and `js_traits` (JavaScript's own doubles,
  UTF-16 strings, bytes written as a `Uint8Array`).
- **One relocatable image per document**: a 64-byte header and a Boost.Interprocess
  `rbtree_best_fit` heap holding a Boost.MultiIndex tree, every pointer an offset, the same bytes
  on 64-bit native and wasm32, a format version of the library's own, an optional CRC-32C seal,
  `reserve`, `shrink_to_fit` and `compact` (byte-identical images for equal content).
- **Streaming in both directions**: an incremental parser fed in chunks of any size with JSONPath
  selectors that deliver values as they complete (bounded memory), a sans-IO stream parser for
  concatenated JSON, NDJSON and JSON text sequences, `stringify_to` in pieces, and Boost.Asio
  readers and writers (documents and images) in `asio.hpp`, which builds natively only.
- **Binary transmission and sharing**: pools of blocks that images are read into and opened in
  place (`image_pool`, `leased_document`, `image_reader`), and reference-counted, copy-on-write
  `shared_document`s in a lock-free `shared_pool` over the process heap or shared memory.
- **The standard containers' interface** over objects and arrays (`as_object`, `as_array`,
  `by_key` with `lower_bound`, `upper_bound` and `equal_range`).
- **A JavaScript module** (`js/webcpp-document.mjs`, built with Emscripten): `Document` and
  `StreamParser`, exact decimals, key ranges, images opened in place and transferred to Workers;
  a second build whose memory is a `SharedArrayBuffer`; and the npm package `@webcpp/document`
  with TypeScript declarations, published with provenance from a tag.
- **A page** (a guide of 23 sections plus the reference and a changelog), 20 examples with their
  expected output, benchmarks against Boost.JSON and JavaScript, and a test suite on native,
  wasip2, wasip3 and Emscripten.

## Capabilities

### New Capabilities

- `document`: webcpp::document 0.1.0, a JSON document for C++20 that behaves as JavaScript does,
  exact in decimal, in one relocatable binary image, with a JavaScript module and an npm package.

### Modified Capabilities

None.

## Impact

- **Kind:** a library of webcpp's own (`"port-of": null`).
- **Implementation:** https://github.com/webcpporg/document at `ac29296`, the submodule
  `libs/document` of the superproject; `reference/source/` is that tree.
- **Dependencies:** Boost 1.92 or newer (Interprocess, MultiIndex, Decimal, System, CRC, and the
  parser of JSON), through `/boost//headers`; webcpp's configuration (`/webcpp/config//config`);
  the compiled parts of `/webcpp//boost_json` and `/webcpp//boost_charconv`. Optional: Boost.Asio
  (`asio.hpp`, native only), Boost.Cobalt and Boost.Beast (examples), xactor (one example),
  Emscripten and Node (the JavaScript module and its lane), nativejson-benchmark (the benchmarks).
- **Status:** implemented and published on 2026-10-12, in release 0.1.0 (`reference/STATUS.md`).
- **What follows:** other webcpp libraries moving from Boost.JSON to document are their own
  proposals; a document inside a shared segment of several documents, durability, and recovery of
  the references of a dead process were left out of 0.1.0 (see `design.md`, Non-Goals).
