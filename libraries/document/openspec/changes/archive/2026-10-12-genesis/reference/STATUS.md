# webcpp::document: where the implementation stands, and how to resume

Written 2026-10-13, when this proposal took document in as a retroactive genesis of a published
library. `spec.md` and `plan.md` are the specification and the plan of 2026-10-12, written before
the library was built; the published library went beyond them (`../design.md`, Context, lists
how); `../tasks.md` is the checklist; `source/` is the library's tree at `ac29296`.

## Where it stands

The library was developed on 2026-10-12, in a worktree of the superproject, its repository's
history 40 commits, all as `R`, from `81b45cd` ("the library, its configuration, errors and
kinds") to `ac29296` ("meta: the maintainer's public address"). It is published:

| What | Where |
| --- | --- |
| The library | `github.com/webcpporg/document`, branch `main` at `ac29296` |
| The superproject | `github.com/webcpporg/webcpp`, whose submodule `libs/document` (`url = ../document.git`) is at `ac29296`; its `doc/releases/0.1.0.adoc` names document among the new libraries ("a header-only JSON document for C++20 that behaves as JavaScript does, exact in decimal, in one relocatable binary image, with a JavaScript module"), and states that no tag names that version yet |
| The page | built by the library's CI (`webcpporg/webcpp/.github/workflows/library.yml`) and published at `webcpporg.github.io/libs/document/`, as the README states |
| The npm package | `@webcpp/document`, built and published by `.github/workflows/npm.yml` on a tag `v<version>`; the very first publication is the owner's, by hand, and nothing in the sources says it was made |

`source/` is exactly `git archive ac29296` of the library: `diff -r` of the two trees, 711 files,
prints nothing.

### What the sources state (not run here)

- The image is the same on every target: a digest `8d80cfaf86a669cb` on clang, gcc-15, wasip2 and
  Emscripten for the plan's heap (`spec.md` section 11), and `example/document_image.expected`
  prints `4096 bytes, sha-256 ed85084394e7bf7f...` on every target of the CI.
- `doc/benchmark-results.adoc` (generated) holds the results measured on 2026-10-12 on an Apple
  M5 Max (macOS 27.0, Apple clang 21.0.0, Boost 1.92, Emscripten 6.0.11, Node.js v26.7.0, Chrome
  147): `canada.json` parses in 209 ms against Boost.JSON's 2.19 ms and Node's 3.58 ms; its image
  is 32,099,376 bytes against 2,090,234 of text; a deep copy takes 441 us.
- The superproject's commits after the library's own (`2ef8a53`, `6572bfc`, `93d0d4b`) carry its
  MSVC and Clang-before-20 work, the lint's last findings and the benchmarks page as valid HTML;
  `doc/testing.adoc` states which targets run what. The sources hold no CI run number for document.

### What was run here (2026-10-13)

On the reference tree, outside b2 and the superproject's CI, with Apple clang 21.0.0
(`-std=c++20 -O1`), Boost 1.92 (Homebrew, with its compiled `libboost_json` and
`libboost_charconv` in place of `/webcpp//boost_json` and `/webcpp//boost_charconv`), the
superproject's `libs/config` for `<webcpp/config.hpp>`, and Node v26.7.0:

- `test/oracle/oracle.mjs` run on all 254 cases of `test/fixtures/cases` into a scratch directory:
  `diff -r` against the committed `test/fixtures/expected` printed nothing, so Node reproduces
  every expected file.
- 25 test programs built and run natively, each exit 0 and "No errors detected.": `errors`,
  `number`, `traits`, `heap`, `selector_grammar`, `literal`, `tree`, `convert`, `value`, `order`,
  `array`, `stringify`, `image`, `seal`, `compact`, `containers`, `parse`, `streaming`,
  `selector`, `stream`, `image_stream`, `shared`, `oracle_cases` (as `oracle_test.cpp`), `format`
  (against the golden images) and `sugar` (with exceptions).
- 12 examples built and run natively, each output identical to its `.expected`:
  `document_quick_start`, `document_javascript`, `document_numbers`, `document_arrays`,
  `document_streaming`, `document_selectors`, `document_traits`, `document_image` (digest
  included), `document_stream_sync`, `document_image_errors`, `document_containers` and
  `document_vs_boost_json`.

Not run here: GCC, MSVC, wasip2, wasip3 and Emscripten; the tests and examples that need
Boost.Asio, Cobalt, Beast, Interprocess, threads or xactor (`stream_asio`, `image_asio`,
`shared_interprocess`, `document_stream_asio`, `document_stream_cobalt`, `document_stream_beast`,
`document_image_asio`, `document_image_interprocess`, `document_threads`,
`document_stream_xactor`); `config` and its variants, `errors_raise`, `parse_sugar`,
`no_exceptions` (a compile-diagnostic test; `document_no_exceptions` compiled here but did not
link without webcpp's `throw_exception`), the headers-alone targets and the `bench` smoke pass;
the JavaScript lane, the npm package, the page and the lint; and any CI.

## Where the code differs from the specification and the plan (the code wins)

- `build.jam` also links `/webcpp//boost_charconv` (doubles through Boost.Charconv); the plan
  names `/webcpp//boost_json` alone.
- The header's Boost version of the plan is four reserved bytes written zero; the format version
  is the library's constant (`config.hpp`), with golden images that watch Boost.
- `parse_options::max_document_bytes` defaults to 64 MiB, not "the storage's" limit.
- The stream parser has `error_line()`, `error_offset()`, `error_cause()` and `finished()` beside
  `line()`, `offset()` and `documents()`; `async_read_document` takes both of Asio's dynamic-buffer
  models; `async_write_images`, `reserve` and `capacity` as API, the containers' interface
  (`containers.hpp`, `by_key`), the Beast example and both JavaScript modules are not in the
  specification or the plan; `push` and `unshift` return `result<std::uint64_t>`, not
  `result<std::size_t>`.
- The specification's example `document_stream_js.cpp` and its `.mjs` were not made: the JavaScript
  module and its tests cover feeding the C++ parser from JavaScript. The examples shipped are 20
  (see `../specs/document/spec.md`, "Examples: The programs and where they build").
- The plan's departure 1 held: no Emscripten infrastructure change for `-fwasm-exceptions`;
  departure 2 held (a lone surrogate is refused by the parser with either encoding); departure 3
  held (the node's block is probed with `node_probe_bytes`, a bound).

## Open items

1. **The page lags on two points.** `doc/js-module.adoc` ("Memory") says `reserve`,
   `shrink_to_fit` and `compact` are not in the module, while `Document` has `reserve`,
   `capacity` and `byteLength` (`webcpp-document.mjs`, `webcpp-document.d.mts`, tested in
   `api.test.mjs`); and the README and `doc/introduction.adoc` list Boost's parts without
   Charconv, which `build.jam` links.
2. **Not in 0.1.0** (`../design.md`, Open Questions): a document inside a shared segment with
   other documents, a commit protocol and durability, the recovery of the references a dead
   process held, and the move of xstate, trystero and json_schema from Boost.JSON to document.
3. **Performance.** document is slower and larger than Boost.JSON by design and by measurement;
   the benchmarks' profile names where to begin (the lookup of a node by id behind every handle,
   the renumbering of positions on a splice, the image `pop` allocates).
4. **The shared JavaScript module** reads internals of Emscripten 6.0.11 and is tied to that
   version; browsers need cross-origin isolation for it.
5. **The first npm publication** is the owner's, by hand, before the workflow's trusted
   publishing can take over (`.github/workflows/npm.yml`, header comment).

## How to resume

1. Work in a worktree of the superproject (its `AGENTS.md`, chapter "Working on a library"):
   `libs/document` is the published repository (`webcpporg/document`); `source/` is the same tree
   for reading without a checkout. The prerequisites are the superproject's: Boost 1.92, b2, a
   C++20 compiler, wasi-sdk and wasmtime for the WASI toolsets, Emscripten 6.0.11 and Node 26 for
   Emscripten and the JavaScript lane, MrDocs and Asciidoctor for the page, and a
   `user-config.jam` that registers the toolsets.
2. Build and test, from scratch (`-a`, since b2 compares timestamps): `b2 -a libs/document/test
   libs/document/example` natively, the same with `toolset=emscripten`,
   `toolset=clang-wasip2 testing.launcher=wasmtime` and `toolset=clang-wasip3
   testing.launcher=wasmtime`; the oracle lane `b2 libs/document/test/oracle//oracle`; the
   JavaScript lane `b2 toolset=emscripten libs/document/test/js//js`; the page `b2
   libs/document/doc`; the benchmarks `b2 libs/document/bench//results` (explicit) and
   `bench//smoke`; the npm package `b2 toolset=emscripten libs/document/js/npm//npm-package` and
   `libs/document/test/js//npm-package-test`; one test while iterating:
   `b2 toolset=clang libs/document/test//<name>`.
3. Follow the library's rules (`source/AGENTS.md`): the core never throws and every operation finds
   its room first; the values of `errc` and `kind`, the category's identity and the image's header
   layout never change, and a layout change raises `format_version` with a migration (never
   regenerate the golden images in `test/fixtures/format` to make `format_test` pass); no floating
   point outside `js_traits` and its codec, no hashed container, nodes built in place; a new
   example is `example/document_<topic>.cpp` with its `.expected` checked against Node, and a
   change of its output checks the `lines=` ranges of the sections that show it; renaming a
   section renames its `@see` lines; a case that holds what the two sides do differently by design
   does not belong in the oracle's corpus; after a change to the oracle's cases run
   `test/oracle//update-expected`.
4. A proposal that changes document is a change under `../../` (this directory's parent), adding
   or modifying requirements of the capability `document`; it does not edit `reference/`.
5. Before saying a task is done: the lint, the tests and examples on every target, the lanes
   `oracle` and `js`, the page, and then the CI on every lane.
