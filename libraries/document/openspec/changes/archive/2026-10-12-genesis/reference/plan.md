---
title: "webcpp::document: implementation plan"
tags: [webcpp, document, json, javascript, plan]
status: draft
created: 2026-10-12
---

# webcpp::document Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add `webcpp::document`, a header-only C++20 library whose JSON document behaves as
JavaScript's (key order, `JSON.stringify`'s bytes, the array methods), stores exact numbers, lives in
one relocatable binary image, parses in a stream with selectors, and never throws in its core.

**Architecture:**
- **One image per document.** A 64-byte header, then a Boost.Interprocess `rbtree_best_fit` heap
  used directly, holding a Boost.MultiIndex container of nodes (three ordered indices) and webcpp's
  own strings; every pointer an `offset_ptr<..., std::int64_t, std::uint64_t, 8>`, so the image works
  at any address and has the same bytes on 64-bit native and wasm32.
- **Two layers.** The core (`set`, `get<T>`, the array methods, the parser, `stringify`) returns
  `result` and never throws, because every operation finds its room before it allocates; the
  JavaScript-style sugar (`v = x`, `as<T>()`, `parse_or_throw`) throws and exists only with
  exceptions, guarded by a named constraint.
- **Traits choose the stored types** (`boolean`, `integer`, `number`, `string`, `date`, `bytes`) and
  the object and array policies; `default_traits` is exact (`std::int64_t`, `decimal128_t`), and
  `js_traits` restores JavaScript's doubles. Node is the oracle: a corpus run in both must give the
  same `JSON.stringify` bytes.

**Tech Stack:**
- C++20, b2 5.5.3, Boost 1.92 (Homebrew): Interprocess (`offset_ptr`, `rbtree_best_fit`,
  `null_mutex`), MultiIndex, Decimal, JSON (`basic_parser` only), System;
- wasi-sdk 34 (clang 23), wasmtime 47.0.3, Emscripten 6.0.11, Node 26.7.0;
- MrDocs 2026.9.29, Asciidoctor.js; GitHub Actions through `library.yml`.

**Spec:** `~/.buzz/PLANS/WEBCPP_DOCUMENT_SPEC.md` (approved by the owner on
2026-10-12), binding. Also binding: the 2026-10-12 entries of `~/.buzz/PLANS/WEBCPP_DESIGN.md`
from "a new library, webcpp::json" to "numbers by value; no BigInt, no 256 bits". Research the tasks
cite: `~/.buzz/RESEARCH/WEBCPP_INTERPROCESS_WITHOUT_EXCEPTIONS.md` (the heap, measured),
`~/.buzz/RESEARCH/JAVASCRIPT_OBJECT_KEY_ORDER.md` (the key order, measured),
`~/.buzz/RESEARCH/WEBCPP_DOCUMENT_SPIKE.cpp` (a working heap with a zeroing allocator and in-place
nodes, identical digests on four toolchains), `~/.buzz/RESEARCH/WEBCPP_JSON_SAX_SPIKE.cpp`
(`basic_parser` with a handler).

**Departures from the spec's letter** (for the owner; each found while planning):
1. **No Emscripten infrastructure change.** Section 3 says the Jamroot must add
   `-fwasm-exceptions` for Emscripten. b2's own Emscripten toolset already adds it whenever
   `<exception-handling>on` (`boost-build/1.92.0/share/b2/src/tools/emscripten.jam:163-164`), in the
   legacy encoding Emscripten 6.0.11 defaults to, which Node 26 runs. Nothing changes in the
   superproject's build.
2. **A lone surrogate is refused with both encodings** (the owner, 2026-10-12: Boost.JSON's parser
   stays, and the limit is documented on the page). Section 6.8 says `utf16` keeps
   `"\ud800"`. Boost.JSON's `basic_parser` cannot deliver one: it refuses it, or with
   `allow_invalid_utf16` replaces it with U+FFFD, which would change the text silently. The document
   refuses it (`errc::syntax`) with either encoding; `set` of a `std::u16string_view` holding one, with
   `utf16`, keeps it, and `stringify` writes it as `\uXXXX` as section 8 says.
3. **The node's block is probed with a bound, not its exact size.** Section 12 says "the container's
   node size, known per build". Boost.MultiIndex does not name its internal node type publicly, so
   the probe asks for `node_probe_bytes = sizeof(node) + 128`, an upper bound that a test pins (a
   counting allocator records every block the container asks for and checks each is at most the
   bound). A probe that succeeds at the bound guarantees the smaller real block.

## Global Constraints

**Repositories, worktrees and paths:**
- `M` is `~/dev/webcpp`, the superproject, on `main`. `W` is `~/dev/webcpp-document`, this plan's
  worktree of it, on branch `document`, made in Task 0 from `main`. Nothing of this plan is written
  in `M` but the ledger, until Task 15.
- `W/libs/document` is the library's own repository, `git init -b main` in Task 1; the other
  libraries are checked out in `W` as submodules and never changed by this plan. `W/.local` is a
  symbolic link to `M/.local`, read only, excluded in `W`'s `info/exclude`.
- The repository `webcpporg/document` is created only with the owner's go, in Task 15, by the
  controller; until then the library is the local repository and `.gitmodules` does not name it.

**Commits and text:**
- Commits: `git -c user.name=R -c user.email=github.overboard312@passmail.net commit`. No trailer of any
  kind, never an agent co-author. A body paragraph never starts with `word:`.
- Files are staged by path: `git add <path> ...`. Never `git add -A`, never `git add libs`.
- Never push and never touch GitHub: the controller publishes, in Task 15, with the owner's go.
- English only. Never the em dash (U+2014): a plain dash. Nothing anywhere matches the
  case-insensitive pattern `veru[s]`. No "stage N", "Task N", plan, review or probe named in a
  tracked file or a commit message: library code cites none of them.
- Every new source file (`.hpp .cpp .mjs .py .jam .yml .adoc`, `Jamfile`, `build.jam`) opens with the
  licence notice, BSL-1.0, "Copyright (c) 2026 WebCpp.org", `//` for C++, JavaScript and AsciiDoc,
  `#` for Jam, Python and YAML (AGENTS.md chapter 11). Below, "(the notice)" stands for it.
- Subject lines name the part touched: `document: ...`, `AGENTS.md: ...`.

**Running b2** (from `W`, after `source .local/env.sh`):
- `B2` is `env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH b2 --user-config=.local/user-config.jam`.
  A verdict is built from scratch: `B2 -a`.
- The five lanes, in parallel, each with its own build directory:
  `B2 -a --dump-tests --build-dir=bin/lane-clang --out-xml=bin/native.xml toolset=clang libs/document/test libs/document/example`,
  the same with `lane-gcc-15`/`gcc-15.xml`/`toolset=gcc-15`, `lane-wasip2`/`wasip2.xml`/`toolset=clang-wasip2 testing.launcher=wasmtime`,
  `lane-wasip3`/`wasip3.xml`/`toolset=clang-wasip3 testing.launcher=wasmtime`, and
  `lane-emscripten`/`emscripten.xml`/`toolset=emscripten` (never a `testing.launcher` there).
  A lane is green when `python3 tools/report/report.py --lane native=bin/native.xml --lane gcc-15=bin/gcc-15.xml --lane wasip2=bin/wasip2.xml --lane wasip3=bin/wasip3.xml --lane emscripten=bin/emscripten.xml --out bin/report-document`
  exits 0 (`--out-xml` makes b2 exit 0 on a failure: the report is the verdict).
- One test while iterating: `B2 toolset=clang libs/document/test//<name>`.
- The lint: `tools/lint/lint.sh --clang-format .local/wasi-sdk/bin/clang-format --clang-tidy .local/wasi-sdk/bin/clang-tidy`
  ends with `lint: clean`. The page: `B2 libs/document/doc`, at most one doc build at a time.
- Every test is also built without exceptions (the lint and the lanes build the twin with
  `exception-handling=off`); a test that needs exceptions declares `<exception-handling>on`.

**Library rules (AGENTS.md chapters 3, 6, 9; the spec):**
- Namespace `webcpp::document`, `detail` for what is not public; macros `WEBCPP_DOCUMENT_*`; test-only
  macros `WEBCPP_TEST_DOCUMENT_*`; include guards `WEBCPP_DOCUMENT_<HEADER>_HPP`.
- Every public header includes `<webcpp/document/config.hpp>` first.
- No `throw` in `include/`: an exception is raised through `boost::throw_exception`, inside
  `#ifndef WEBCPP_DOCUMENT_NO_EXCEPTIONS`.
- No `float` or `double` anywhere but `js_traits` and its codec.
- Never `boost/interprocess/managed_external_buffer.hpp`, `segment_manager.hpp`,
  `allocators/allocator.hpp` or `sync/mutex_family.hpp`: they throw bare. Allowed from Interprocess:
  `offset_ptr.hpp`, `mem_algo/rbtree_best_fit.hpp`, `sync/null_mutex.hpp`.
- No hashed container anywhere: keys come from untrusted text.
- No global or static mutable state.
- Every public symbol carries a Doc Comment (MrDocs strict: a brief, `@param`, `@tparam`, `@return`,
  one per enumerator and data member); each task documents what it adds.

**Changelog** (`libs/document/doc/changelog.adoc`, under "Unreleased"): the lines each task names, in
its commit; at most ten in all; `* ` then Added, Changed, Fixed, Improved, Removed, Deprecated or
Updated, one sentence of about ten words; no cross-reference. The superproject's
`doc/releases/0.1.0.adoc` names the library only in Task 15, once it is a submodule.

**Outward actions:** subagents never push, merge, create repositories, start or cancel CI runs, or
change GitHub settings, and never touch SSH keys or credentials. Tasks 0 and 15 are the
controller's; Task 15 needs the owner's go.

## Review Focus

1. **A handle kept across growth.** A program holds `doc["a"]`, then inserts enough to make an owned
   document double several times (its image moves), then reads and writes through the handle: it must
   work, and a `std::string_view` taken before the change is the one thing documented as stale.
   Pinned in Task 6 (`handle_survives_growth`).
2. **A full external buffer in the middle of a compound operation.** `splice` inserting many items,
   `concat`, `assign_from`, a deep copy, and a parse into an external buffer, each run in a buffer
   filled to every size from empty to full: every failure returns `errc::capacity` and leaves the
   image's content as it was. Pinned in Task 7 (`splice_in_every_size`), Task 6
   (`copy_in_every_size`) and Task 9 (`parse_into_every_size`).
3. **A chunk boundary inside a token.** A multi-byte UTF-8 character split across chunks, an escape
   split (`"\u12` then `34"`), a number split (`12` then `.5e3`), a literal split (`tr` then `ue`), a
   key split: the streamed document equals the one-shot one. Pinned in Task 9 (every input split at
   every byte, and one byte at a time) and Task 10 (the selectors' deliveries under every split).
4. **Keys that look like array indices.** `"01"`, `"-0"`, `"1e2"`, `"4294967295"`, `"4294967294"`,
   `""`, `"0"`, and a key holding `\u0000`: the order, `contains` and `erase` must match JavaScript's,
   and an index key erased and re-created goes back among the indices, not last. Pinned in Task 5
   (`index_lookalikes`), Task 6 (`order_test`) and the oracle (Task 12).
5. **Numbers at the edges of their text.** `-0`, `0e0`, `1E400`, `1e-400`, `0.0000001`, `1e21`,
   `123456789012345678901234567890123456789` (39 digits), `1.000000000000000000000000000000000000000`
   (one significant digit, 40 written), `9223372036854775807`, `9223372036854775808`,
   `-9223372036854775809`: each stored, refused and written exactly as section 7 says. Pinned in
   Task 2 (`number_edges`), Task 9 (the parse rows) and the oracle (Task 12).
6. **Depth and size at their limits.** Nesting at `max_depth` and one deeper; a 64 MB string; an
   object of 100,000 keys; a selector on a million elements: the limit is exact, the error is
   `capacity` or `syntax` with an offset, and memory under `on` stays bounded. Pinned in Task 9
   (limits, 100,000 keys) and Task 10 (a million elements).

---

## File structure

```
~/dev/webcpp-document/  (W, branch document of the superproject)
  libs/document/                         the library's repository (webcpporg/document from Task 15)
    build.jam README.md AGENTS.md LICENSE_1_0.txt .gitignore .gitattributes
    .github/workflows/ci.yml
    meta/libraries.json
    include/webcpp/document.hpp          every public header
    include/webcpp/document/
      config.hpp                         WEBCPP_DOCUMENT_NO_EXCEPTIONS, detail::exceptions_enabled
      errors.hpp                         errc, the category, result, the exception types
      kind.hpp                           kind, undefined, null
      number.hpp                         decimal digits, JavaScript's Number::toString, number text
      traits.hpp                         scalar codecs, encodings, bytes policies, object and array policies
      literal.hpp                        object, array
      image.hpp                          the header, layout tags, open, format, from_image
      document.hpp                       basic_document, owned, external, document
      value.hpp                          basic_value, basic_const_value, value, const_value
      parse.hpp                          parse_options, parse, parse_or_throw
      parser.hpp                         basic_parser, parser, path_view, the selectors
      stringify.hpp                      stringify, stringify_to
      detail/heap.hpp                    the algorithm, the allocator, byte_string
      detail/tree.hpp                    the node, the container, the tree and its room
      detail/utf.hpp                     UTF-8 validation, UTF-16 counting and transcoding
      detail/handler.hpp                 the basic_parser handler that builds a tree
      detail/selector.hpp                the selector grammar and matcher
      detail/writer.hpp                  the JSON writer stringify and stringify_to share
    test/Jamfile .clang-tidy require.hpp
    test/config_test.cpp errors_test.cpp number_test.cpp traits_test.cpp heap_test.cpp
    test/tree_test.cpp value_test.cpp order_test.cpp array_test.cpp stringify_test.cpp
    test/parse_test.cpp streaming_test.cpp selector_test.cpp image_test.cpp sugar_test.cpp
    test/no_exceptions_test.cpp oracle_test.cpp
    test/oracle/Jamfile package.json oracle.mjs
    test/fixtures/cases/*.json test/fixtures/expected/*.json
    example/Jamfile .clang-tidy document_*.cpp document_*.expected
    doc/Jamfile mrdocs.yml document.adoc and its sections, changelog.adoc
  AGENTS.md README.md                    the library rows (Task 14)
  doc/releases/0.1.0.adoc                New Libraries (Task 15)
  .gitmodules                            libs/document (Task 15)
```

Nothing else of the superproject changes: the Jamroot globs `libs/*/build.jam`, the CI derives its
lanes from `b2 declared-targets` and `declared-lanes`, the lint reads every repository under `libs/`,
and the index and catalogue read `meta/libraries.json`.

## Order and parallel work

Task 0, then Task 1. Then Tasks 2, 3 and 4 depend only on Task 1 and may run in parallel, each on its
own branch of `libs/document` in its own worktree (`git -C W/libs/document worktree add ...`), merged
by the controller into `main` in the order 2, 3, 4 (Task 3 consumes Task 2's `number.hpp`: it starts
from Task 2's interface, and its `number_codec` tests run once Task 2 is merged). Task 5 needs 3 and
4; Task 6 needs 2 and 5; Tasks 7 to 11 follow in order; Task 12 (the oracle) needs 7, 8 and 9 and may
run beside 10 and 11; Task 13 needs every library task; Task 14 needs every other; Task 15 is the
owner's.

---
### Task 0: The plan's worktree (controller)

**Files:** none tracked; `W` and its ledger.

- [ ] **Step 1: The worktree.**

```bash
git -C ~/dev/webcpp worktree add ~/dev/webcpp-document -b document main
cd ~/dev/webcpp-document
git submodule update --init --reference ~/dev/webcpp
ln -s ~/dev/webcpp/.local .local
echo ".local" >> "$(git rev-parse --git-dir)/info/exclude"
```

- [ ] **Step 2: Check the tree builds as `M` does.** `source .local/env.sh`, then
  `B2 -a toolset=clang libs/pratt/test//errors` (any small test): green.
- [ ] **Step 3: The ledger.** `bash <skill>/scripts/sdd-workspace ~/.buzz/PLANS/WEBCPP_DOCUMENT_PLAN.md`
  in `M`, and its first line `# SDD ledger - plan: ~/.buzz/PLANS/WEBCPP_DOCUMENT_PLAN.md`.

### Task 1: The library: its repository, configuration, errors and kinds

**Files:**
- Create in `W/libs/document` (a new repository, `git init -b main`): `build.jam`, `README.md`,
  `AGENTS.md`, `LICENSE_1_0.txt` (copied from `libs/pratt`), `.gitignore`, `.gitattributes`,
  `.github/workflows/ci.yml`, `meta/libraries.json`, `include/webcpp/document.hpp`,
  `include/webcpp/document/config.hpp`, `include/webcpp/document/errors.hpp`,
  `include/webcpp/document/kind.hpp`, `test/Jamfile`, `test/.clang-tidy`, `test/require.hpp`,
  `test/config_test.cpp`, `test/errors_test.cpp`, `doc/Jamfile`, `doc/mrdocs.yml`,
  `doc/document.adoc`, `doc/testing.adoc`, `doc/changelog.adoc`.

**Interfaces:**
- Produces (every later task uses these exact names):
  - `WEBCPP_DOCUMENT_NO_EXCEPTIONS`; `webcpp::document::detail::exceptions_on` (a `constexpr bool`)
    and `template <class T> concept exceptions_enabled`.
  - `enum class errc : int { syntax = 1, type = 2, range = 3, capacity = 4, not_a_document = 5, incompatible_document = 6 };`
  - `const boost::system::error_category& error_category() noexcept;`,
    `boost::system::error_code make_error_code(errc) noexcept;`,
    `template <class T> using result = boost::system::result<T>;`
  - with exceptions only: `class error : public std::runtime_error` (`code()`), `syntax_error`
    (`offset()`), `type_error`, `range_error`, `capacity_error`, `image_error`; and
    `[[noreturn]] void detail::raise(boost::system::error_code, std::uint64_t offset = 0)`, which
    throws the type of the code's `errc` through `boost::throw_exception`.
  - `enum class kind : std::uint8_t { undefined, null, boolean, number, string, date, bytes, array, object };`
    `struct undefined_t {}; inline constexpr undefined_t undefined{};`
    `struct null_t {}; inline constexpr null_t null{};`

- [ ] **Step 1: The repository and its files.**

```bash
cd ~/dev/webcpp-document/libs && mkdir document && cd document && git init -b main
cp ../pratt/LICENSE_1_0.txt ../pratt/.gitattributes .
```

`.gitignore`: `doc/html/`, `node_modules`, `.node-modules/` (one per line). `build.jam`:

```
(the notice)
#
# document, a JSON document that behaves as JavaScript does, in one relocatable image. A program
# uses it through /webcpp/document//document, which adds its headers, Boost's, webcpp's
# configuration and Boost.JSON's definitions, whose parser it uses.

project /webcpp/document ;

alias document : : : : <include>include <library>/boost//headers
  <library>/webcpp/config//config <library>/webcpp//boost_json ;
```

`meta/libraries.json`:

```json
{
    "key": "document",
    "name": "document",
    "authors": ["R"],
    "description": "A JSON document for C++20 that behaves as JavaScript does, in one relocatable image.",
    "category": ["Data structures", "Parsing"],
    "maintainers": ["R <github.overboard312@passmail.net>"],
    "cxxstd": "20",
    "port-of": null
}
```

`.github/workflows/ci.yml`: pratt's, with `document` for `pratt` in its comment and its `library:`.
`test/.clang-tidy`: pratt's, as it is. `test/require.hpp`: pratt's, with the guard
`WEBCPP_TEST_DOCUMENT_REQUIRE_HPP`. `doc/Jamfile`: `import webcpp ;`,
`webcpp.doc document : document.adoc ;`, `webcpp.reference document ;`. `doc/mrdocs.yml`: pratt's
(`show-enum-constants: true`, with its comment naming `errc` and `kind`).

`AGENTS.md`:

```markdown
# Working on document

webcpp's rules apply here: read the superproject's [AGENTS.md](../../AGENTS.md)
(<https://github.com/webcpporg/webcpp/blob/main/AGENTS.md>) first. This file holds only what is
specific to document.

## What document is

- **webcpp's own library** (`"port-of": null`): a JSON document whose behaviour is JavaScript's. Node
  is its oracle: `test/oracle` runs the same cases in Node and in C++, and the bytes must agree.
- **Its core never throws.** Every operation that can fail returns `result`; it finds its room in
  the heap before it allocates, so a full heap is an error value. Only the sugar (`v = x`, `as<T>()`,
  `as_string()`, `as_bool()`, `parse_or_throw`) throws, and each carries the constraint
  `detail::exceptions_enabled`, so a build without exceptions refuses a call to it.
- **One image.** A 64-byte header, then Boost.Interprocess's `rbtree_best_fit` heap, used directly;
  every pointer in it an `offset_ptr<..., std::int64_t, std::uint64_t, 8>`.

## Cautions

- Never include `managed_external_buffer.hpp`, `segment_manager.hpp`, `allocators/allocator.hpp` or
  `sync/mutex_family.hpp` from Boost.Interprocess: they throw bare and do not compile without
  exceptions.
- No floating point outside `js_traits` and its codec; no hashed container; nodes are built in place
  (`emplace`), never copied from a temporary; the allocator zeroes every block it hands out and
  frees.
- The values of `errc` and `kind`, the category's identity `0x646f63756d656e74` ("document") and the
  image's header layout never change; a layout change raises the image's format version.
- Boost.MultiIndex erases an element whose `modify` throws: a `modify` only swaps prepared values.

## The page

`doc/document.adoc` and its sections; `B2 libs/document/doc` builds it with the strict reference.
```

`README.md` (its example and commands grow in Task 13):

```markdown
# document

A JSON document for C++20 that behaves as JavaScript does: the same key order, the same
`JSON.stringify` bytes, the array methods, exact numbers, streaming parse with selectors, and one
relocatable binary image that works at any address and is the same on 64-bit native and wasm32. Its
core never throws. Header-only; it needs Boost 1.92 (Interprocess, MultiIndex, Decimal, JSON's parser).
Targets: native, wasm32-wasip2, wasm32-wasip3 and Emscripten.

## Building and testing

    b2 libs/document/test libs/document/example

## Documentation

https://webcpporg.github.io/libs/document/

## License

Distributed under the [Boost Software License, Version 1.0](https://www.boost.org/LICENSE_1_0.txt).
```

- [ ] **Step 2: Write the failing tests.**

`test/Jamfile`:

```
(the notice)

project : requirements <library>/webcpp/document//document ;

import webcpp ;

webcpp.targets native emscripten wasip2 wasip3 ;

webcpp.headers-alone document : ../include ;

# config.hpp defines WEBCPP_DOCUMENT_NO_EXCEPTIONS exactly when webcpp has no exceptions, and keeps
# it when a developer defines it in a build that has them.
webcpp.compile config : config_test.cpp ;
webcpp.compile config_developer_defined : config_test.cpp
  : <define>WEBCPP_DOCUMENT_NO_EXCEPTIONS <define>WEBCPP_TEST_DOCUMENT_DEVELOPER_DEFINED ;
webcpp.compile config_all_off : config_test.cpp
  : <define>WEBCPP_NO_EXCEPTIONS <define>WEBCPP_TEST_DOCUMENT_ALL_OFF ;

webcpp.run errors : errors_test.cpp ;
```

`test/config_test.cpp`: pratt's, with `DOCUMENT` for `PRATT` and `document` for `pratt`, and one
check more, after the `#endif` of the macro's checks, that `detail::exceptions_on` and
`detail::exceptions_enabled` follow the macro:

```cpp
#ifdef WEBCPP_DOCUMENT_NO_EXCEPTIONS
static_assert(!webcpp::document::detail::exceptions_on);
static_assert(!webcpp::document::detail::exceptions_enabled<int>);
#else
static_assert(webcpp::document::detail::exceptions_on);
static_assert(webcpp::document::detail::exceptions_enabled<int>);
#endif
```

`test/errors_test.cpp`: pratt's three cases for `errc`, with:

```cpp
void values_are_fixed() {
    BOOST_TEST_EQ(static_cast<int>(wd::errc::syntax), 1);
    BOOST_TEST_EQ(static_cast<int>(wd::errc::type), 2);
    BOOST_TEST_EQ(static_cast<int>(wd::errc::range), 3);
    BOOST_TEST_EQ(static_cast<int>(wd::errc::capacity), 4);
    BOOST_TEST_EQ(static_cast<int>(wd::errc::not_a_document), 5);
    BOOST_TEST_EQ(static_cast<int>(wd::errc::incompatible_document), 6);
}
```

the category named `"webcpp.document"`, the messages `"malformed JSON text"`,
`"a value of another kind"`, `"a number or index out of range"`, `"no room left"`,
`"not a document's image"`, `"an image of another format, Boost, layout, byte order or traits"`,
`"unknown webcpp.document error"` for 0 and 7, the buffer truncation; and the kinds:

```cpp
void kinds_are_fixed() {
    BOOST_TEST_EQ(static_cast<int>(wd::kind::undefined), 0);
    BOOST_TEST_EQ(static_cast<int>(wd::kind::null), 1);
    BOOST_TEST_EQ(static_cast<int>(wd::kind::boolean), 2);
    BOOST_TEST_EQ(static_cast<int>(wd::kind::number), 3);
    BOOST_TEST_EQ(static_cast<int>(wd::kind::string), 4);
    BOOST_TEST_EQ(static_cast<int>(wd::kind::date), 5);
    BOOST_TEST_EQ(static_cast<int>(wd::kind::bytes), 6);
    BOOST_TEST_EQ(static_cast<int>(wd::kind::array), 7);
    BOOST_TEST_EQ(static_cast<int>(wd::kind::object), 8);
}

#ifndef WEBCPP_DOCUMENT_NO_EXCEPTIONS
// detail::raise throws the exception of each errc, carrying the code and, for a syntax error, the
// byte offset.
void raise_throws_the_type_of_its_code() {
    auto thrown = [](wd::errc e) -> std::string {
        try {
            wd::detail::raise(e, 17);
        } catch (const wd::syntax_error& x) {
            return "syntax " + std::to_string(x.offset()) + " " + x.code().message();
        } catch (const wd::type_error& x) {
            return "type " + x.code().message();
        } catch (const wd::range_error& x) {
            return "range " + x.code().message();
        } catch (const wd::capacity_error& x) {
            return "capacity " + x.code().message();
        } catch (const wd::image_error& x) {
            return "image " + x.code().message();
        }
        return "none";
    };
    BOOST_TEST_EQ(thrown(wd::errc::syntax), "syntax 17 malformed JSON text");
    BOOST_TEST_EQ(thrown(wd::errc::type), "type a value of another kind");
    BOOST_TEST_EQ(thrown(wd::errc::range), "range a number or index out of range");
    BOOST_TEST_EQ(thrown(wd::errc::capacity), "capacity no room left");
    BOOST_TEST_EQ(thrown(wd::errc::not_a_document), "image not a document's image");
}
#endif
```

with `raise_throws_the_type_of_its_code` declared `<exception-handling>on` by its own target:
`webcpp.run errors_raise : errors_test.cpp : <exception-handling>on <define>WEBCPP_TEST_DOCUMENT_RAISE ;`
and the case run in `main` only under `WEBCPP_TEST_DOCUMENT_RAISE`.

- [ ] **Step 3: Run them and watch them fail.** `B2 toolset=clang libs/document/test`. Expected:
  `fatal error: 'webcpp/document/config.hpp' file not found`.

- [ ] **Step 4: Implement.**

`include/webcpp/document/config.hpp`:

```cpp
(the notice)

/**
 The configuration of document, which each of its headers includes first: whether document is
 built without exceptions, and the constraint its throwing members carry.

 @see "Exceptions", in the guide.
*/

#ifndef WEBCPP_DOCUMENT_CONFIG_HPP
#define WEBCPP_DOCUMENT_CONFIG_HPP

#include <webcpp/config.hpp>

#ifdef __MRDOCS__
/**
 Defined when document is built without exceptions: when webcpp has none (the compiler has none,
 or the developer defined `WEBCPP_NO_EXCEPTIONS` for every webcpp library), or when the developer
 defines it alone.

 document's core never throws, so the macro changes none of it. It removes the members that throw
 (`value::operator=`, `as`, `as_string`, `as_bool`, `parse_or_throw`) and the exception types: a
 call to one of those members does not compile, and the compiler names `detail::exceptions_enabled`.

 @see "Exceptions", in the guide.
*/
#define WEBCPP_DOCUMENT_NO_EXCEPTIONS
#undef WEBCPP_DOCUMENT_NO_EXCEPTIONS
#endif

#if !defined(WEBCPP_HAS_EXCEPTIONS) && !defined(WEBCPP_DOCUMENT_NO_EXCEPTIONS)
#define WEBCPP_DOCUMENT_NO_EXCEPTIONS
#endif

namespace webcpp::document::detail {

/** Whether this build has document's throwing members. */
#ifdef WEBCPP_DOCUMENT_NO_EXCEPTIONS
inline constexpr bool exceptions_on = false;
#else
inline constexpr bool exceptions_on = true;
#endif

/**
 The constraint of every member that throws: it depends on `T`, so it is checked where the member
 is called, and a build without exceptions reports a call by this name.
*/
template <class T>
concept exceptions_enabled = exceptions_on && sizeof(T) != 0;

}  // namespace webcpp::document::detail

#endif  // WEBCPP_DOCUMENT_CONFIG_HPP
```

`include/webcpp/document/errors.hpp`: pratt's `errors.hpp` with these changes. The enum is `errc`,
with the six values above, each with a Doc Comment naming JavaScript's counterpart where there is
one (`SyntaxError`, `TypeError`, `RangeError`). `detail::message_of` returns the messages of Step 2.
The category's identity is `0x646f63756d656e74ULL`, its name `"webcpp.document"`. `result` says
that document's core returns it for every operation that can fail. Then, inside
`#ifndef WEBCPP_DOCUMENT_NO_EXCEPTIONS`:

```cpp
/** What every exception of document derives from: a `std::runtime_error` holding its code. */
class error : public std::runtime_error {
public:
    /** Constructs the exception of `code`, whose message is the code's. */
    explicit error(boost::system::error_code code)
        : std::runtime_error(code.message()), code_(code) {}
    /** The code of the failure. @return The code given at construction. */
    boost::system::error_code code() const noexcept { return code_; }

private:
    boost::system::error_code code_;
};

/** JavaScript's `SyntaxError`: malformed JSON text, at a byte offset. */
class syntax_error : public error {
public:
    /** Constructs the exception of `code` at byte `offset` of the text. */
    syntax_error(boost::system::error_code code, std::uint64_t offset)
        : error(code), offset_(offset) {}
    /** The byte offset of the failure in the text. @return The offset. */
    std::uint64_t offset() const noexcept { return offset_; }

private:
    std::uint64_t offset_;
};

/** JavaScript's `TypeError`: a value of another kind, or a write through nothing. */
class type_error : public error { public: using error::error; };
/** JavaScript's `RangeError`: a number that does not fit or an index out of range. */
class range_error : public error { public: using error::error; };
/** No room left: a full external buffer, a document at its limit, a parser's limit. */
class capacity_error : public error { public: using error::error; };
/** Bytes that are not a document's image, or an image of another format. */
class image_error : public error { public: using error::error; };

namespace detail {
/**
 Throws the exception of `code`'s @ref errc through `boost::throw_exception`; a code of another
 category throws @ref error.
*/
[[noreturn]] inline void raise(boost::system::error_code code, std::uint64_t offset = 0) {
    if (code.category() != error_category()) {
        boost::throw_exception(error(code));
    }
    switch (static_cast<errc>(code.value())) {
        case errc::syntax: boost::throw_exception(syntax_error(code, offset));
        case errc::type: boost::throw_exception(type_error(code));
        case errc::range: boost::throw_exception(range_error(code));
        case errc::capacity: boost::throw_exception(capacity_error(code));
        case errc::not_a_document:
        case errc::incompatible_document: boost::throw_exception(image_error(code));
    }
    boost::throw_exception(error(code));
}
}  // namespace detail
```

(`raise(errc e, ...)` takes the `errc` through its implicit conversion.) `kind.hpp` holds `kind`,
`undefined_t`/`undefined`, `null_t`/`null`, each documented: `kind` as JavaScript's `typeof` with
`null`, arrays, dates and bytes told apart. `include/webcpp/document.hpp` includes `config.hpp`,
`errors.hpp` and `kind.hpp` (each later task adds its headers).

`doc/document.adoc`: pratt's page header with `= document: a JSON document that behaves as
JavaScript does, for {cpp}`, two paragraphs from the spec's section 1, `include::testing.adoc[]`,
the reference block and `include::changelog.adoc[]`. `doc/testing.adoc`: `[#document-testing]`,
`== Testing`, the five lanes and the counts by attribute (`{n-tests}`), as pratt's says.
`doc/changelog.adoc`: pratt's, for document, with one line:

```
* Added the library: a JSON document that behaves as JavaScript does.
```

- [ ] **Step 5: Run everything and watch it pass.** The five lanes (Global Constraints), the report
  exits 0; `B2 libs/document/doc`; the lint `lint: clean`.

- [ ] **Step 6: Commit** (in `W/libs/document`).

```bash
git add build.jam README.md AGENTS.md LICENSE_1_0.txt .gitignore .gitattributes \
  .github/workflows/ci.yml meta/libraries.json include test doc
git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "document: the library, its configuration, errors and kinds"
```

### Task 2: Numbers: exact text in, JavaScript's text out

**Files:**
- Create: `include/webcpp/document/number.hpp`, `test/number_test.cpp`
- Modify: `test/Jamfile` (one line), `include/webcpp/document.hpp` (one include)

**Interfaces:**
- Consumes: `result`, `errc` (Task 1).
- Produces:

```cpp
namespace webcpp::document {
/** A finite decimal number as JavaScript's Number::toString sees it: value = 0.d1...dk x 10^point. */
struct decimal_digits {
    bool negative = false;              // never true for zero
    std::array<char, 34> digits{};      // '0'..'9'; the first and the last are not '0'; empty for zero
    std::uint32_t count = 0;            // k; 0 for zero
    std::int32_t point = 0;             // n: the decimal point after this many digits
};
/** Text of at most 64 characters, held by value. */
struct number_text {
    std::array<char, 64> chars{};
    std::uint32_t size = 0;
    std::string_view view() const noexcept { return {chars.data(), size}; }
};
result<decimal_digits> scan_number(std::string_view json_number) noexcept;  // exact, at most 34 significant digits
number_text write_javascript(const decimal_digits& d) noexcept;              // ECMA-262 Number::toString, radix 10
std::optional<std::int64_t> to_int64(const decimal_digits& d) noexcept;      // when integral and it fits
decimal_digits from_int64(std::int64_t v) noexcept;
result<boost::decimal::decimal128_t> to_decimal128(const decimal_digits& d) noexcept;
decimal_digits from_decimal128(boost::decimal::decimal128_t v) noexcept;     // finite only; normalised
}
```

- [ ] **Step 1: Write the failing test** `test/number_test.cpp`, every case below. The expected
  text of `write_javascript` is what Node 26 prints for the same number where a double holds it
  exactly (measured while planning); the exact rows that a double cannot hold follow the same rule.

```cpp
(the notice)

#include <webcpp/document/number.hpp>

#include <boost/core/lightweight_test.hpp>

#include <string>
#include <string_view>

namespace wd = webcpp::document;

namespace {

wd::decimal_digits digits_of(std::string_view d, std::int32_t point, bool negative = false) {
    wd::decimal_digits out;
    out.negative = negative;
    for (char c : d) {
        out.digits[out.count++] = c;
    }
    out.point = point;
    return out;
}

std::string javascript(std::string_view d, std::int32_t point, bool negative = false) {
    return std::string(wd::write_javascript(digits_of(d, point, negative)).view());
}

// Each row of the formatting table, with Node's text.
void writes_as_javascript_does() {
    BOOST_TEST_EQ(javascript("", 0), "0");
    BOOST_TEST_EQ(javascript("1", 1), "1");
    BOOST_TEST_EQ(javascript("1", 1, true), "-1");
    BOOST_TEST_EQ(javascript("15", 1), "1.5");
    BOOST_TEST_EQ(javascript("1", 3), "100");
    BOOST_TEST_EQ(javascript("12345", 3), "123.45");
    BOOST_TEST_EQ(javascript("1", 21), "100000000000000000000");
    BOOST_TEST_EQ(javascript("1", 22), "1e+21");
    BOOST_TEST_EQ(javascript("15", 22, true), "-1.5e+21");
    BOOST_TEST_EQ(javascript("1", -5), "0.000001");
    BOOST_TEST_EQ(javascript("1", -6), "1e-7");
    BOOST_TEST_EQ(javascript("1234", -6), "1.234e-7");
    BOOST_TEST_EQ(javascript("5", 0), "0.5");
    BOOST_TEST_EQ(javascript("1", 0), "0.1");
    BOOST_TEST_EQ(javascript("12", 0), "0.12");
    BOOST_TEST_EQ(javascript("1", 2), "10");
    BOOST_TEST_EQ(javascript("1234567", 7), "1234567");
    BOOST_TEST_EQ(javascript("123", 1), "1.23");
    BOOST_TEST_EQ(javascript("17976931348623157", 309), "1.7976931348623157e+308");
    // Exact where a double rounds: 18 digits, and below a double's range.
    BOOST_TEST_EQ(javascript("123456789012345678", 21), "123456789012345678000");
    BOOST_TEST_EQ(javascript("1", -323), "1e-324");
    BOOST_TEST_EQ(javascript("1234567890123456789012345678901234", 6144),
                  "1.234567890123456789012345678901234e+6143");
}

std::string rescan(std::string_view text) {
    auto d = wd::scan_number(text);
    if (!d) {
        return "error " + d.error().message();
    }
    return std::string(wd::write_javascript(*d).view());
}

// The edges of the text: signs, zeros, exponents, the 34-digit limit.
void number_edges() {
    BOOST_TEST_EQ(rescan("-0"), "0");
    BOOST_TEST_EQ(rescan("0e0"), "0");
    BOOST_TEST_EQ(rescan("-0.0e-5"), "0");
    BOOST_TEST_EQ(rescan("0.0000001"), "1e-7");
    BOOST_TEST_EQ(rescan("1e21"), "1e+21");
    BOOST_TEST_EQ(rescan("1E+2"), "100");
    BOOST_TEST_EQ(rescan("1.50"), "1.5");
    BOOST_TEST_EQ(rescan("1.000000000000000000000000000000000000000"), "1");
    BOOST_TEST_EQ(rescan("1234567890123456789012345678901234"), "1.234567890123456789012345678901234e+33");
    BOOST_TEST_EQ(rescan("12345678901234567890123456789012345"), "error a number or index out of range");
    BOOST_TEST_EQ(rescan("123456789012345678901234567890123456789"), "error a number or index out of range");
    BOOST_TEST_EQ(rescan("1E400"), "1e+400");
    BOOST_TEST_EQ(rescan("1e-400"), "1e-400");
    BOOST_TEST_EQ(rescan("1e2147483648"), "error a number or index out of range");
}

void integers() {
    BOOST_TEST(wd::to_int64(*wd::scan_number("9223372036854775807")) == INT64_MAX);
    BOOST_TEST(!wd::to_int64(*wd::scan_number("9223372036854775808")));
    BOOST_TEST(wd::to_int64(*wd::scan_number("-9223372036854775808")) == INT64_MIN);
    BOOST_TEST(!wd::to_int64(*wd::scan_number("-9223372036854775809")));
    BOOST_TEST(wd::to_int64(*wd::scan_number("1.0")) == 1);
    BOOST_TEST(wd::to_int64(*wd::scan_number("1e0")) == 1);
    BOOST_TEST(wd::to_int64(*wd::scan_number("1.5e1")) == 15);
    BOOST_TEST(!wd::to_int64(*wd::scan_number("1.5")));
    BOOST_TEST(!wd::to_int64(*wd::scan_number("1e20")));
    BOOST_TEST_EQ(std::string(wd::write_javascript(wd::from_int64(INT64_MIN)).view()), "-9223372036854775808");
    BOOST_TEST_EQ(std::string(wd::write_javascript(wd::from_int64(0)).view()), "0");
}

void decimal128_round_trips() {
    for (std::string_view text : {"0.1", "-1.5", "1e-400", "1e+400", "1234567890123456789012345678901234",
                                  "18446744073709551615", "0.000001"}) {
        auto d = wd::scan_number(text);
        if (!BOOST_TEST(d.has_value())) {
            continue;
        }
        auto v = wd::to_decimal128(*d);
        if (!BOOST_TEST(v.has_value())) {
            continue;
        }
        BOOST_TEST_EQ(std::string(wd::write_javascript(wd::from_decimal128(*v)).view()),
                      std::string(wd::write_javascript(*d).view()));
    }
    // Normalised: the cohorts of 1.5 are one set of digits.
    auto a = wd::from_decimal128(*wd::to_decimal128(*wd::scan_number("1.50")));
    auto b = wd::from_decimal128(*wd::to_decimal128(*wd::scan_number("1.5")));
    BOOST_TEST_EQ(a.count, 2U);
    BOOST_TEST_EQ(a.count, b.count);
    BOOST_TEST_EQ(a.point, b.point);
    // decimal128's exponent range: 1e6145 does not fit.
    BOOST_TEST(!wd::to_decimal128(*wd::scan_number("1e6145")));
}

}  // namespace

int main() {
    writes_as_javascript_does();
    number_edges();
    integers();
    decimal128_round_trips();
    return boost::report_errors();
}
```

`test/Jamfile`: `webcpp.run number : number_test.cpp ;`.

- [ ] **Step 2: Run it and watch it fail.** `B2 toolset=clang libs/document/test//number`: no
  `number.hpp`.

- [ ] **Step 3: Implement** `number.hpp`. `write_javascript` is ECMA-262 Number::toString, radix 10,
  steps 6 to 10, on `k = count` and `n = point`:

```cpp
inline number_text write_javascript(const decimal_digits& d) noexcept {
    number_text t;
    auto put = [&t](char c) { t.chars[t.size++] = c; };
    if (d.count == 0) {
        put('0');
        return t;
    }
    if (d.negative) {
        put('-');
    }
    const std::int64_t k = d.count;
    const std::int64_t n = d.point;
    if (k <= n && n <= 21) {                 // the digits, then n - k zeros
        for (std::int64_t i = 0; i < k; ++i) put(d.digits[i]);
        for (std::int64_t i = k; i < n; ++i) put('0');
    } else if (0 < n && n <= 21) {           // a point after n digits
        for (std::int64_t i = 0; i < n; ++i) put(d.digits[i]);
        put('.');
        for (std::int64_t i = n; i < k; ++i) put(d.digits[i]);
    } else if (-6 < n && n <= 0) {           // 0., -n zeros, the digits
        put('0');
        put('.');
        for (std::int64_t i = n; i < 0; ++i) put('0');
        for (std::int64_t i = 0; i < k; ++i) put(d.digits[i]);
    } else {                                 // exponential: d1[.d2...]e+/-(n - 1)
        put(d.digits[0]);
        if (k > 1) {
            put('.');
            for (std::int64_t i = 1; i < k; ++i) put(d.digits[i]);
        }
        put('e');
        const std::int64_t e = n - 1;
        put(e < 0 ? '-' : '+');
        char buffer[24];
        auto [end, ec] = std::to_chars(buffer, buffer + sizeof buffer, e < 0 ? -e : e);
        for (char* p = buffer; p != end; ++p) put(*p);
    }
    return t;
}
```

  The longest text is 21 digits with zeros, or 34 digits, a point, `e`, a sign and 10 exponent
  digits, plus a sign: under 64. `scan_number` reads the JSON grammar
  (`-? (0 | [1-9][0-9]*) (. [0-9]+)? ([eE] [+-]? [0-9]+)?`, already validated by the parser when it
  comes from one, so it returns `errc::syntax` for anything else): it skips leading zeros, collects
  the significant digits (refusing a 35th non-zero significant digit with `errc::range`, while
  trailing zeros past the last non-zero digit are only counted, not stored), drops the trailing zeros,
  and sets `point` to the number of integer-part digits after the first significant one, plus the
  exponent; an exponent whose value or sum leaves `std::int32_t` is `errc::range`; zero ends with
  `count == 0`, `negative == false`. `to_int64` checks `point >= count` and `point <= 19`, builds the
  value with overflow checks against `INT64_MAX` (or `INT64_MAX + 1` when negative), else
  `std::nullopt`. `to_decimal128` builds the coefficient as `boost::int128::uint128_t` from the
  digits and constructs `decimal128_t(coefficient, point - count, negative)`, then checks
  `isfinite` (an exponent past decimal128's range gives infinity or zero: both are `errc::range`
  unless the digits were zero); check the constructor's exact signature in
  `boost/decimal/decimal128_t.hpp` before writing it. `from_decimal128` uses
  `boost::decimal::frexp10` for the coefficient and exponent, writes the coefficient's digits by
  division by 10, and drops trailing zeros (normalising the cohort). Every function is `noexcept`
  and allocates nothing. Each carries its Doc Comment, with the formatting table.

- [ ] **Step 4: Run it and watch it pass**, then the five lanes, the report exits 0, `B2
  libs/document/doc//reference`, the lint.

- [ ] **Step 5: Commit.**

```bash
git add include/webcpp/document/number.hpp include/webcpp/document.hpp test/number_test.cpp test/Jamfile
git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "document: exact numbers from JSON text, written as JavaScript writes them"
```

### Task 3: Traits: scalar codecs, encodings, bytes, dates and policies

**Files:**
- Create: `include/webcpp/document/traits.hpp`, `include/webcpp/document/detail/utf.hpp`,
  `test/traits_test.cpp`
- Modify: `test/Jamfile` (one line), `include/webcpp/document.hpp` (one include)

**Interfaces:**
- Consumes: `result`, `errc` (Task 1); `decimal_digits`, `number_text`, `scan_number`,
  `write_javascript`, `to_int64`, `from_int64`, `to_decimal128`, `from_decimal128` (Task 2).
- Produces:

```cpp
namespace webcpp::document {
// Encodings, bytes policies, object and array policies: empty tags, each with `name`.
struct utf8 { static constexpr std::string_view name = "utf8"; };
struct utf16 { static constexpr std::string_view name = "utf16"; };
struct base64 { static constexpr std::string_view name = "base64"; };
struct base64url { static constexpr std::string_view name = "base64url"; };
struct javascript_bytes { static constexpr std::string_view name = "javascript"; };
struct javascript_order { static constexpr std::string_view name = "javascript"; };
struct insertion_order { static constexpr std::string_view name = "insertion"; };
struct sorted { static constexpr std::string_view name = "sorted"; };
struct dense { static constexpr std::string_view name = "dense"; };

using date_ms = std::chrono::sys_time<std::chrono::milliseconds>;

template <class T> struct scalar_codec;   // name, size, store(const T&, std::byte*), load(const std::byte*)
template <> struct scalar_codec<bool>;                           // "bool", 1
template <> struct scalar_codec<std::int64_t>;                   // "int64", 8
template <> struct scalar_codec<boost::decimal::decimal128_t>;   // "decimal128", 16
template <> struct scalar_codec<double>;                         // "binary64", 8
template <> struct scalar_codec<date_ms>;                        // "date-ms", 8

template <class T> concept stored_scalar = /* trivially copyable, codec with size 1, 2, 4, 8 or 16 */;

template <class T> struct number_codec;   // for the traits' number type
//   static result<T> parse(std::string_view json_number) noexcept;
//   static number_text write(const T& v) noexcept;              // JavaScript's text
//   static std::optional<std::int64_t> integral(const T& v) noexcept;  // its value, if an integer that fits
//   static T from_int64(std::int64_t v) noexcept;
//   static bool finite(const T& v) noexcept;

struct default_traits {
    using boolean = bool;
    using integer = std::int64_t;
    using number = boost::decimal::decimal128_t;
    using string = utf8;
    using date = date_ms;
    using bytes = base64;
    using object = javascript_order;
    using array = dense;
};
struct js_traits : default_traits {
    using number = double;
    using string = utf16;
    using bytes = javascript_bytes;
};

template <class Traits> constexpr std::uint32_t traits_tag() noexcept;   // FNV-1a over the names and sizes

namespace detail {
std::optional<std::uint32_t> array_index(std::string_view key) noexcept;   // canonical 0..4294967294
number_text write_date(date_ms t) noexcept;                     // Date.prototype.toJSON's text, unquoted
std::optional<date_ms> read_date(std::string_view text) noexcept;   // exactly that format
inline constexpr std::int64_t date_limit_ms = 8'640'000'000'000'000;
std::uint64_t base64_size(std::uint64_t bytes, bool padded) noexcept;
void write_base64(std::span<const std::byte> in, char* out, bool url) noexcept;   // url: "-_", no padding
result<std::uint64_t> read_base64(std::string_view text, std::byte* out, bool url) noexcept;  // the size read
}
namespace detail {   // detail/utf.hpp
bool valid_utf8(std::string_view text) noexcept;                 // RFC 3629: no surrogates, no overlongs
std::uint64_t utf16_length(std::string_view utf8) noexcept;      // JavaScript's String length
std::uint64_t to_utf16(std::string_view utf8, char16_t* out) noexcept;   // the units written
bool valid_utf16(std::u16string_view text) noexcept;             // no lone surrogate
}
}
```

- [ ] **Step 1: Write the failing test** `test/traits_test.cpp`. Cases (the date rows are Node 26's
  `JSON.stringify(new Date(ms))`, measured):

```cpp
(the notice)

#include <webcpp/document/traits.hpp>

#include <boost/core/lightweight_test.hpp>

#include <array>
#include <cstring>
#include <string>
#include <string_view>

namespace wd = webcpp::document;

namespace {

void array_indices() {
    BOOST_TEST(wd::detail::array_index("0") == 0U);
    BOOST_TEST(wd::detail::array_index("7") == 7U);
    BOOST_TEST(wd::detail::array_index("10") == 10U);
    BOOST_TEST(wd::detail::array_index("4294967294") == 4294967294U);
    for (std::string_view no : {"4294967295", "01", "-0", "-1", "1.0", " 1", "1e2", "", "a", "00",
                                "99999999999999999999"}) {
        BOOST_TEST(!wd::detail::array_index(no));
    }
}

std::string date_text(std::int64_t ms) {
    return std::string(wd::detail::write_date(wd::date_ms(std::chrono::milliseconds(ms))).view());
}

void dates_as_javascript_writes_them() {
    BOOST_TEST_EQ(date_text(0), "1970-01-01T00:00:00.000Z");
    BOOST_TEST_EQ(date_text(-1), "1969-12-31T23:59:59.999Z");
    BOOST_TEST_EQ(date_text(1791774245678), "2026-10-12T03:04:05.678Z");
    BOOST_TEST_EQ(date_text(8640000000000000), "+275760-09-13T00:00:00.000Z");
    BOOST_TEST_EQ(date_text(-8640000000000000), "-271821-04-20T00:00:00.000Z");
    BOOST_TEST_EQ(date_text(-62198755200000), "-000001-01-01T00:00:00.000Z");
    BOOST_TEST_EQ(date_text(253402300800000), "+010000-01-01T00:00:00.000Z");
    BOOST_TEST_EQ(date_text(253402300799999), "9999-12-31T23:59:59.999Z");
    BOOST_TEST_EQ(date_text(951782400000), "2000-02-29T00:00:00.000Z");
    for (std::int64_t ms : {0LL, -1LL, 1791774245678LL, 8640000000000000LL, -8640000000000000LL,
                            -62198755200000LL, 253402300800000LL, 951782400000LL}) {
        auto back = wd::detail::read_date(date_text(ms));
        BOOST_TEST(back && back->time_since_epoch().count() == ms);
    }
    for (std::string_view no : {"2026-10-12", "2026-10-12T03:04:05Z", "2026-10-12T03:04:05.678+00:00",
                                "2026-13-01T00:00:00.000Z", "2026-02-30T00:00:00.000Z",
                                "+275760-09-13T00:00:00.001Z", "10000-01-01T00:00:00.000Z"}) {
        BOOST_TEST(!wd::detail::read_date(no));
    }
}

std::string base64_of(std::string_view raw, bool url) {
    std::string out(wd::detail::base64_size(raw.size(), !url), '?');
    wd::detail::write_base64(std::as_bytes(std::span(raw.data(), raw.size())), out.data(), url);
    return out;
}

// RFC 4648 section 10's vectors, and the URL alphabet.
void base64_vectors() {
    BOOST_TEST_EQ(base64_of("", false), "");
    BOOST_TEST_EQ(base64_of("f", false), "Zg==");
    BOOST_TEST_EQ(base64_of("fo", false), "Zm8=");
    BOOST_TEST_EQ(base64_of("foo", false), "Zm9v");
    BOOST_TEST_EQ(base64_of("foob", false), "Zm9vYg==");
    BOOST_TEST_EQ(base64_of("fooba", false), "Zm9vYmE=");
    BOOST_TEST_EQ(base64_of("foobar", false), "Zm9vYmFy");
    BOOST_TEST_EQ(base64_of("\xfb\xff", true), "-_8");
    BOOST_TEST_EQ(base64_of("\xfb\xff", false), "+/8=");
    std::array<std::byte, 8> out{};
    BOOST_TEST(wd::detail::read_base64("Zm9vYmFy", out.data(), false) == 6U);
    BOOST_TEST(wd::detail::read_base64("-_8", out.data(), true) == 2U);
    for (std::string_view bad : {"Zg=", "Z", "Zm9v!", "Zg==Zg==", "Zm=v"}) {
        BOOST_TEST(!wd::detail::read_base64(bad, out.data(), false));
    }
}

void utf() {
    BOOST_TEST(wd::detail::valid_utf8("a\xc3\xa9\xe2\x82\xac\xf0\x9f\x98\x80"));
    BOOST_TEST_EQ(wd::detail::utf16_length("a\xc3\xa9\xe2\x82\xac\xf0\x9f\x98\x80"), 5U);  // Node: 5
    for (std::string_view bad : {"\xc0\xaf", "\xed\xa0\x80", "\xf4\x90\x80\x80", "\xe2\x82", "\xff"}) {
        BOOST_TEST(!wd::detail::valid_utf8(bad));
    }
    BOOST_TEST(wd::detail::valid_utf16(u"a\xd83d\xde00"));
    BOOST_TEST(!wd::detail::valid_utf16(u"\xd800"));
}

void codecs() {
    std::array<std::byte, 16> raw{};
    wd::scalar_codec<std::int64_t>::store(-2, raw.data());
    BOOST_TEST_EQ(wd::scalar_codec<std::int64_t>::load(raw.data()), -2);
    wd::scalar_codec<bool>::store(true, raw.data());
    BOOST_TEST(raw[0] == std::byte{1});
    using dec = boost::decimal::decimal128_t;
    auto n = wd::number_codec<dec>::parse("1.50");
    BOOST_TEST(n && std::string(wd::number_codec<dec>::write(*n).view()) == "1.5");
    BOOST_TEST(wd::number_codec<dec>::integral(*wd::number_codec<dec>::parse("1.0")) == 1);
    BOOST_TEST(!wd::number_codec<dec>::parse("1e6145"));
    auto d = wd::number_codec<double>::parse("123456789012345678901234567890");
    BOOST_TEST(d && std::string(wd::number_codec<double>::write(*d).view()) == "1.2345678901234568e+29");
    BOOST_TEST(std::string(wd::number_codec<double>::write(0.1).view()) == "0.1");
    static_assert(wd::stored_scalar<bool> && wd::stored_scalar<std::int64_t> && wd::stored_scalar<dec> &&
                  wd::stored_scalar<double> && wd::stored_scalar<wd::date_ms>);
    static_assert(!wd::stored_scalar<std::string>);
}

void tags_tell_traits_apart() {
    static_assert(wd::traits_tag<wd::default_traits>() != wd::traits_tag<wd::js_traits>());
    struct sorted_traits : wd::default_traits { using object = wd::sorted; };
    static_assert(wd::traits_tag<wd::default_traits>() != wd::traits_tag<sorted_traits>());
    static_assert(wd::traits_tag<wd::default_traits>() == wd::traits_tag<wd::default_traits>());
}

}  // namespace

int main() {
    array_indices();
    dates_as_javascript_writes_them();
    base64_vectors();
    utf();
    codecs();
    tags_tell_traits_apart();
    return boost::report_errors();
}
```

`test/Jamfile`: `webcpp.run traits : traits_test.cpp ;`.

- [ ] **Step 2: Run it and watch it fail.** `B2 toolset=clang libs/document/test//traits`: no
  `traits.hpp`.

- [ ] **Step 3: Implement.**
  - `scalar_codec<T>`: `store` copies the value's bytes in little-endian order into `size` bytes
    (`bool` as 0 or 1; `decimal128_t` and `double` through `std::bit_cast` to their bit patterns,
    written byte by byte); `load` reverses it. No `memcpy` of the object itself: the stored bytes are
    the same on every target by construction.
  - `stored_scalar<T>`: `std::is_trivially_copyable_v<T> && requires { scalar_codec<T>::size; }`
    and the size is 1, 2, 4, 8 or 16.
  - `number_codec<decimal128_t>`: `parse` is `to_decimal128(scan_number(text))`; `write` is
    `write_javascript(from_decimal128(v))`; `integral` is `to_int64(from_decimal128(v))`.
    `number_codec<double>`: `parse` is `std::from_chars` (a value past a double's range is
    `errc::range`, as `JSON.parse` would make it `Infinity`); `write` takes `std::to_chars(buf, v,
    std::chars_format::scientific)` (the shortest round-trip digits), reads its digits and exponent
    into a `decimal_digits`, and calls `write_javascript`; NaN and the infinities write `null`.
  - `traits_tag<Traits>()`: a `constexpr` FNV-1a (32-bit) over, in order, each scalar codec's
    `name` and `size` (`boolean`, `integer`, `number`, `date`), then the `string`, `bytes`, `object`
    and `array` policies' `name`s, each followed by a zero byte.
  - `detail::array_index`: `""` or a leading `'0'` with more digits is none; at most 10 digits, all
    `'0'`..`'9'`; the value at most 4294967294.
  - `detail::write_date`: `std::chrono::year_month_day` of the day, the time of day from the
    remainder (floor division for negative values); a year 0 to 9999 as four digits, else a sign and
    six digits; `T`, `HH:mm:ss.sss`, `Z`. `read_date` parses exactly that grammar, checks each field's
    range (the day against the month and the leap year), rebuilds the milliseconds, refuses a value
    beyond `date_limit_ms`, and refuses `-000000` (JavaScript's spec forbids it).
  - `detail::write_base64` / `read_base64`: RFC 4648; `url` uses `-` and `_` and no padding; the
    standard alphabet requires padding to a multiple of 4; any other character, misplaced `=`, or
    non-zero trailing bits in the last character are `errc::syntax`.
  - `detail/utf.hpp`: the four functions, each a single pass, `noexcept`, no allocation.
  - Every symbol documented; `default_traits` and `js_traits` say what each member chooses.

- [ ] **Step 4: Run it and watch it pass**, the five lanes, the report, the reference, the lint.

- [ ] **Step 5: Commit**, with the changelog line
  `* Added traits that choose the stored types, dates and bytes included.`

```bash
git add include/webcpp/document/traits.hpp include/webcpp/document/detail/utf.hpp \
  include/webcpp/document.hpp test/traits_test.cpp test/Jamfile doc/changelog.adoc
git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "document: traits choose the stored types, encodings and policies"
```

### Task 4: The heap, its allocator, its strings, the regions and the image's header

**Files:**
- Create: `include/webcpp/document/detail/heap.hpp`, `include/webcpp/document/image.hpp` (the header
  and its check; `open`, `format` and `from_image` come in Task 6), `test/heap_test.cpp`
- Modify: `test/Jamfile` (one line), `include/webcpp/document.hpp` (one include)

**Interfaces:**
- Consumes: `result`, `errc` (Task 1). Measured base: `~/.buzz/RESEARCH/WEBCPP_DOCUMENT_SPIKE.cpp`.
- Produces:

```cpp
namespace webcpp::document::detail {
template <class T> using pointer = boost::interprocess::offset_ptr<T, std::int64_t, std::uint64_t, 8>;
struct no_mutexes {
    using mutex_type = boost::interprocess::null_mutex;
    using recursive_mutex_type = boost::interprocess::null_mutex;
};
using heap = boost::interprocess::rbtree_best_fit<no_mutexes, pointer<void>, 16>;

template <class T> class allocator;   // value_type T, pointer pointer<T>, size_type std::uint64_t;
                                      // allocate zeroes; deallocate zeroes then frees; a failed
                                      // allocation is boost::throw_exception(std::bad_alloc())
struct byte_string {                  // a string or bytes value in the heap
    pointer<std::byte> data;
    std::uint64_t size = 0;
    std::span<const std::byte> view() const noexcept;
};
result<byte_string> make_string(heap& h, std::span<const std::byte> bytes) noexcept;  // exact size; capacity on no room
void free_string(heap& h, byte_string& s) noexcept;                                   // zeroes, frees, empties
bool probe(heap& h, std::span<const std::uint64_t> sizes) noexcept;                    // all fit at once?

inline constexpr std::uint64_t header_bytes = 64;

class region {                        // the image's bytes: owned (grows) or external (fixed)
public:
    static result<region> owned(std::uint64_t size, std::uint64_t max_bytes) noexcept;
    static region external(std::span<std::byte> buffer) noexcept;
    std::byte* data() const noexcept;
    std::uint64_t size() const noexcept;
    bool is_owned() const noexcept;
    bool grow(std::uint64_t at_least) noexcept;   // owned: doubles (or more) up to max_bytes, copies,
                                                  // zeroes the rest; false when it cannot
    // movable, not copyable; an owned region frees its buffer
};
heap& heap_of(const region& r) noexcept;          // the heap at offset header_bytes
}
namespace webcpp::document {
inline constexpr std::uint32_t format_version = 1;
inline constexpr std::array<char, 8> image_magic{'W','C','P','P','D','O','C','\0'};
struct image_header {          // 64 bytes at offset 0 of every image, in the machine's byte order
    std::array<char, 8> magic;
    std::uint32_t version;
    std::uint32_t byte_order;  // 0x01020304
    std::uint32_t boost_version;
    std::uint32_t layout;
    std::uint64_t size;
    std::uint64_t container;
    std::uint64_t next_id;
    std::array<std::byte, 16> reserved;
};
result<void> check_image(std::span<const std::byte> bytes, std::uint32_t layout) noexcept;
}
```

- [ ] **Step 1: Write the failing test** `test/heap_test.cpp`:

```cpp
(the notice)

#include <webcpp/document/detail/heap.hpp>
#include <webcpp/document/image.hpp>

#include <boost/core/lightweight_test.hpp>

#include <algorithm>
#include <cstring>
#include <new>
#include <string_view>
#include <vector>

namespace wd = webcpp::document;
namespace detail = webcpp::document::detail;

namespace {

std::span<const std::byte> bytes_of(std::string_view s) { return std::as_bytes(std::span(s.data(), s.size())); }

bool contains(std::span<const std::byte> image, std::string_view needle) {
    auto n = bytes_of(needle);
    return std::search(image.begin(), image.end(), n.begin(), n.end()) != image.end();
}

void strings_are_exact_and_zeroed_when_freed() {
    auto r = detail::region::owned(4096, 1 << 20);
    if (!BOOST_TEST(r.has_value())) { return; }
    auto& h = detail::heap_of(*r);
    auto s = detail::make_string(h, bytes_of("a secret of 32 bytes, exactly...."));
    if (!BOOST_TEST(s.has_value())) { return; }
    BOOST_TEST_EQ(s->size, 33U);
    BOOST_TEST(contains({r->data(), r->size()}, "a secret of 32 bytes"));
    detail::free_string(h, *s);
    BOOST_TEST_EQ(s->size, 0U);
    BOOST_TEST(!contains({r->data(), r->size()}, "a secret of 32 bytes"));
}

void a_full_heap_says_so_and_grows() {
    auto r = detail::region::owned(4096, 1 << 20);
    if (!BOOST_TEST(r.has_value())) { return; }
    std::vector<detail::byte_string> kept;
    std::string_view block = "0123456789abcdef0123456789abcdef";
    for (;;) {
        auto s = detail::make_string(detail::heap_of(*r), bytes_of(block));
        if (!s) {
            BOOST_TEST(s.error() == wd::errc::capacity);
            break;
        }
        kept.push_back(*s);
    }
    BOOST_TEST(!kept.empty());
    const std::uint64_t before = r->size();
    BOOST_TEST(r->grow(before + 1));
    BOOST_TEST_GE(r->size(), 2 * before);
    // What was there moved with the image and reads the same through its offsets.
    BOOST_TEST(std::memcmp(kept.front().data.get(), block.data(), block.size()) == 0);
    BOOST_TEST(detail::make_string(detail::heap_of(*r), bytes_of(block)).has_value());
}

void an_owned_region_stops_at_its_limit() {
    auto r = detail::region::owned(4096, 8192);
    if (!BOOST_TEST(r.has_value())) { return; }
    BOOST_TEST(r->grow(8192));
    BOOST_TEST(!r->grow(8193));
    BOOST_TEST_EQ(r->size(), 8192U);
}

void an_external_region_never_grows() {
    alignas(16) std::array<std::byte, 4096> buffer{};
    auto r = detail::region::external(buffer);
    BOOST_TEST(!r.is_owned());
    BOOST_TEST(!r.grow(4097));
    BOOST_TEST(r.data() == buffer.data());
}

void probe_answers_without_allocating() {
    auto r = detail::region::owned(4096, 4096);
    if (!BOOST_TEST(r.has_value())) { return; }
    auto& h = detail::heap_of(*r);
    const auto free_before = h.get_free_memory();
    const std::array<std::uint64_t, 2> small{100, 200};
    const std::array<std::uint64_t, 1> large{8192};
    BOOST_TEST(detail::probe(h, small));
    BOOST_TEST(!detail::probe(h, large));
    BOOST_TEST_EQ(h.get_free_memory(), free_before);
}

void the_same_operations_give_the_same_bytes_at_any_address() {
    auto a = detail::region::owned(4096, 1 << 20);
    auto b = detail::region::owned(4096, 1 << 20);
    if (!BOOST_TEST(a.has_value() && b.has_value())) { return; }
    for (auto* r : {&*a, &*b}) {
        auto& h = detail::heap_of(*r);
        auto x = detail::make_string(h, bytes_of("first"));
        auto y = detail::make_string(h, bytes_of("second, longer"));
        detail::free_string(h, *x);
        BOOST_TEST(detail::make_string(h, bytes_of("third")).has_value());
        static_cast<void>(y);
    }
    BOOST_TEST(a->data() != b->data());
    BOOST_TEST(std::memcmp(a->data() + detail::header_bytes, b->data() + detail::header_bytes,
                           a->size() - detail::header_bytes) == 0);
}

// Every defect of a header is refused before anything else is read.
void headers_are_checked() {
    alignas(16) std::array<std::byte, 256> image{};
    auto write = [&](auto&& change) {
        wd::image_header h{};
        h.magic = wd::image_magic;
        h.version = wd::format_version;
        h.byte_order = 0x01020304;
        h.boost_version = BOOST_VERSION;
        h.layout = 42;
        h.size = image.size();
        change(h);
        std::memcpy(image.data(), &h, sizeof h);
    };
    write([](wd::image_header&) {});
    BOOST_TEST(wd::check_image(image, 42).has_value());
    BOOST_TEST(wd::check_image(std::span(image).first(63), 42).error() == wd::errc::not_a_document);
    write([](wd::image_header& h) { h.magic[0] = 'X'; });
    BOOST_TEST(wd::check_image(image, 42).error() == wd::errc::not_a_document);
    write([&](wd::image_header& h) { h.size = image.size() + 1; });
    BOOST_TEST(wd::check_image(image, 42).error() == wd::errc::not_a_document);
    write([](wd::image_header& h) { h.size = 63; });
    BOOST_TEST(wd::check_image(image, 42).error() == wd::errc::not_a_document);
    write([](wd::image_header& h) { h.version = 2; });
    BOOST_TEST(wd::check_image(image, 42).error() == wd::errc::incompatible_document);
    write([](wd::image_header& h) { h.byte_order = 0x04030201; });
    BOOST_TEST(wd::check_image(image, 42).error() == wd::errc::incompatible_document);
    write([](wd::image_header& h) { h.boost_version = BOOST_VERSION + 1; });
    BOOST_TEST(wd::check_image(image, 42).error() == wd::errc::incompatible_document);
    write([](wd::image_header&) {});
    BOOST_TEST(wd::check_image(image, 43).error() == wd::errc::incompatible_document);
}

}  // namespace

int main() {
    static_assert(sizeof(wd::image_header) == 64);
    strings_are_exact_and_zeroed_when_freed();
    a_full_heap_says_so_and_grows();
    an_owned_region_stops_at_its_limit();
    an_external_region_never_grows();
    probe_answers_without_allocating();
    the_same_operations_give_the_same_bytes_at_any_address();
    headers_are_checked();
    return boost::report_errors();
}
```

`test/Jamfile`: `webcpp.run heap : heap_test.cpp ;`.

- [ ] **Step 2: Run it and watch it fail.** `B2 toolset=clang libs/document/test//heap`: no
  `detail/heap.hpp`.

- [ ] **Step 3: Implement** from the measured spike:
  - `region::owned`: `::operator new(size, std::align_val_t{16}, std::nothrow)`, `errc::capacity` on
    null or when `size > max_bytes` or `size < header_bytes + sizeof(heap) + 64`; zero the buffer;
    construct the heap with `new (data + header_bytes) heap(size - header_bytes, sizeof(heap))`.
    `region::external`: the span as it is (the caller formats or opens it, Task 6). `grow(at_least)`:
    the new size is the larger of `2 * size` and `at_least`, rounded up to 16, at most `max_bytes`
    (false past it, or when the allocation fails); allocate, copy the old bytes, zero the rest, free
    the old buffer, then `heap_of(*this).grow(new_size - old_size)` (the heap's offsets are relative,
    so it works in the copy). The destructor of an owned region frees with
    `::operator delete(p, std::align_val_t{16})`.
  - `allocator<T>`: as the spike's, rebinding, `operator==` comparing heaps; `allocate(n)` calls
    `allocate_aligned(n * sizeof(T), alignof(T))`, and a null result goes to
    `boost::throw_exception(std::bad_alloc())`: the owner's ruling (an allocation the document did not
    find room for first is its bug).
  - `make_string`: `heap.allocate(size, std::nothrow)`, `errc::capacity` on null (a size of 0
    allocates nothing and returns an empty string); copy; `free_string` zeroes and deallocates.
  - `probe`: allocate each size with `std::nothrow`, holding them, then free them all; true when all
    succeeded.
  - `check_image` in the order the test checks, with no read beyond the first 64 bytes before the
    size is known.
  - Only `offset_ptr.hpp`, `mem_algo/rbtree_best_fit.hpp` and `sync/null_mutex.hpp` from Interprocess.

- [ ] **Step 4: Run it and watch it pass**, the five lanes (each also without exceptions: this test
  passes there too, since nothing in it reaches the allocator's failure), the report, the reference,
  the lint.

- [ ] **Step 5: Commit**, with the changelog line
  `* Added one relocatable binary image per document, the same on every target.`

```bash
git add include/webcpp/document/detail/heap.hpp include/webcpp/document/image.hpp \
  include/webcpp/document.hpp test/heap_test.cpp test/Jamfile doc/changelog.adoc
git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "document: the heap, its allocator and strings, and the image's header"
```

### Task 5: The tree: nodes, indices, key order, and operations that find their room first

**Files:**
- Create: `include/webcpp/document/detail/tree.hpp`, `test/tree_test.cpp`
- Modify: `include/webcpp/document/image.hpp` (`layout_tag`), `test/Jamfile` (one line)

**Interfaces:**
- Consumes: Tasks 1 to 4 (`kind`, `errc`, `result`, the codecs and policies, `traits_tag`,
  `detail::array_index`, `detail::heap`, `detail::allocator`, `detail::byte_string`,
  `detail::make_string`, `detail::free_string`, `detail::probe`, `detail::region`,
  `detail::heap_of`, `image_header`, `check_image`).
- Produces:

```cpp
namespace webcpp::document::detail {
template <class Traits> inline constexpr std::size_t scalar_bytes = /* max(16, every scalar codec's size), rounded up to 8 */;
enum class repr : std::uint8_t { none = 0, integer = 1, number = 2 };

template <class Traits> struct node {
    std::uint64_t id;
    std::uint64_t parent;
    std::uint8_t group;          // 0: an element, or an array-index key under javascript_order; 1: any other key
    std::uint64_t ordinal;       // the element's index, the index key's value, the insertion number,
                                 // or (sorted) the node's id
    byte_string key;             // the key's code units in the traits' encoding; empty for an element
    document::kind kind;
    repr number;
    std::array<std::byte, scalar_bytes<Traits>> scalar;   // the value; an array's length at [0, 8);
                                                          // an object's next insertion number at [0, 8)
                                                          // and its key count at [8, 16)
    byte_string text;            // a string's code units or a bytes value's bytes
    node(/* every field, built in place by emplace */);
};
template <class Traits> using container = boost::multi_index_container<node<Traits>, /* by_id, by_order, by_key */,
                                                                      allocator<node<Traits>>>;
template <class Traits> inline constexpr std::uint64_t node_probe_bytes = sizeof(node<Traits>) + 128;

// A value about to be stored: what set, push and the parser hand the tree.
template <class Traits> struct item {
    document::kind kind = document::kind::null;
    repr number = repr::none;
    std::array<std::byte, scalar_bytes<Traits>> scalar{};
    std::span<const std::byte> text;                // a string (already in the traits' encoding) or bytes
    const literal_entry<Traits>* entries = nullptr; // a literal object or array (Task 6 makes them)
    std::size_t entry_count = 0;
    const tree<Traits>* from = nullptr;             // a subtree to copy deeply
    std::uint64_t from_id = 0;
};

template <class Traits> class tree {
public:
    static result<tree> format(region& r) noexcept;      // a new image: header, container, root null
    static result<tree> attach(region& r) noexcept;      // an image already there (header checked)
    std::uint64_t root() const noexcept;                 // 1
    const node<Traits>* find(std::uint64_t id) const noexcept;
    const node<Traits>* member(std::uint64_t object, std::string_view key) const noexcept;   // key in UTF-8
    const node<Traits>* element(std::uint64_t array, std::uint64_t index) const noexcept;
    template <class F> void for_each_child(std::uint64_t parent, F&& f) const;  // in the policy's order
    std::uint64_t count(std::uint64_t container_id) const noexcept;             // elements or keys
    std::uint64_t node_count() const noexcept;

    // Every operation below either completes or changes nothing; errc::capacity when there is no room.
    result<void> assign(std::uint64_t id, const item<Traits>& v) noexcept;                     // the value replaced
    result<std::uint64_t> set_member(std::uint64_t object, std::string_view key, const item<Traits>& v) noexcept;
    result<bool> erase_member(std::uint64_t object, std::string_view key) noexcept;            // whether it was there
    result<void> insert_elements(std::uint64_t array, std::uint64_t at, std::span<const item<Traits>> v) noexcept;
    result<void> erase_elements(std::uint64_t array, std::uint64_t at, std::uint64_t n) noexcept;
    result<void> reverse(std::uint64_t array) noexcept;
    result<void> set_length(std::uint64_t array, std::uint64_t n) noexcept;                   // truncates or appends null

    region& storage() const noexcept;
};
}
namespace webcpp::document {
template <class Traits> constexpr std::uint32_t layout_tag() noexcept;   // traits_tag, the node's, the container's
                                                                         // and the heap's sizes and alignments
}
```

  (`literal_entry<Traits>` is declared here, empty of meaning until Task 6: a key, an `item`, and for a
  nested literal its own entries; `item` holds a pointer to an array of them.)

- [ ] **Step 1: Write the failing test** `test/tree_test.cpp`. Its helper `dump(tree)` walks the tree
  from the root in the policy's order and writes a compact text (`{"b":1,...}`-like, with kinds),
  used to compare content before and after an operation.

```cpp
(the notice)

#include <webcpp/document/detail/tree.hpp>

#include <boost/core/lightweight_test.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace wd = webcpp::document;
namespace detail = webcpp::document::detail;
using traits = wd::default_traits;
using tree = detail::tree<traits>;

namespace {

detail::item<traits> integer(std::int64_t v);                 // kind number, repr integer, stored by the codec
detail::item<traits> string(std::string_view utf8);          // kind string
detail::item<traits> object();                               // kind object, no entries
detail::item<traits> array();                                // kind array
std::string dump(const tree& t);                             // as described above
std::vector<std::string> keys(const tree& t, std::uint64_t object);

struct fixture {
    detail::region r = *detail::region::owned(4096, 1 << 24);
    tree t = *tree::format(r);
};

void a_new_tree_holds_null() {
    fixture f;
    BOOST_TEST_EQ(f.t.root(), 1U);
    BOOST_TEST(f.t.find(1)->kind == wd::kind::null);
    BOOST_TEST_EQ(f.t.node_count(), 1U);
}

// JavaScript's OrdinaryOwnPropertyKeys, measured in Node 26.
void javascript_key_order() {
    fixture f;
    BOOST_TEST(f.t.assign(1, object()).has_value());
    for (std::string_view k : {"b", "2", "a", "1"}) {
        BOOST_TEST(f.t.set_member(1, k, integer(0)).has_value());
    }
    BOOST_TEST((keys(f.t, 1) == std::vector<std::string>{"1", "2", "b", "a"}));
    BOOST_TEST(f.t.set_member(1, "b", integer(9)).has_value());          // assigned: keeps its place
    BOOST_TEST((keys(f.t, 1) == std::vector<std::string>{"1", "2", "b", "a"}));
    BOOST_TEST(*f.t.erase_member(1, "b"));
    BOOST_TEST(f.t.set_member(1, "b", integer(1)).has_value());          // re-created: last
    BOOST_TEST((keys(f.t, 1) == std::vector<std::string>{"1", "2", "a", "b"}));
    BOOST_TEST(*f.t.erase_member(1, "1"));
    BOOST_TEST(f.t.set_member(1, "1", integer(1)).has_value());          // an index: back among the indices
    BOOST_TEST((keys(f.t, 1) == std::vector<std::string>{"1", "2", "a", "b"}));
}

void index_lookalikes() {
    fixture f;
    BOOST_TEST(f.t.assign(1, object()).has_value());
    for (std::string_view k : {"x", "01", "4294967295", "-0", "1e2", "", "4294967294", "0",
                               std::string_view("\0k", 2)}) {
        BOOST_TEST(f.t.set_member(1, k, integer(0)).has_value());
    }
    BOOST_TEST((keys(f.t, 1) == std::vector<std::string>{"0", "4294967294", "x", "01", "4294967295", "-0",
                                                           "1e2", "", std::string("\0k", 2)}));
    BOOST_TEST(f.t.member(1, std::string_view("\0k", 2)) != nullptr);
    BOOST_TEST(f.t.member(1, "k") == nullptr);
}

void insertion_and_sorted_orders();       // the same keys under insertion_order and sorted traits

void elements_are_renumbered() {
    fixture f;
    BOOST_TEST(f.t.assign(1, array()).has_value());
    std::vector<detail::item<traits>> four{integer(0), integer(1), integer(2), integer(3)};
    BOOST_TEST(f.t.insert_elements(1, 0, four).has_value());
    std::vector<detail::item<traits>> two{integer(8), integer(9)};
    BOOST_TEST(f.t.insert_elements(1, 1, two).has_value());
    BOOST_TEST_EQ(dump(f.t), "[0,8,9,1,2,3]");
    BOOST_TEST(f.t.erase_elements(1, 2, 3).has_value());
    BOOST_TEST_EQ(dump(f.t), "[0,8,3]");
    BOOST_TEST(f.t.reverse(1).has_value());
    BOOST_TEST_EQ(dump(f.t), "[3,8,0]");
    BOOST_TEST(f.t.set_length(1, 5).has_value());
    BOOST_TEST_EQ(dump(f.t), "[3,8,0,null,null]");
    BOOST_TEST(f.t.set_length(1, 1).has_value());
    BOOST_TEST_EQ(dump(f.t), "[3]");
    BOOST_TEST_EQ(f.t.count(1), 1U);
}

void replacing_a_container_frees_its_subtree() {
    fixture f;
    BOOST_TEST(f.t.assign(1, object()).has_value());
    auto inner = f.t.set_member(1, "a", object());
    BOOST_TEST(f.t.set_member(*inner, "b", string("a long string that is not short")).has_value());
    BOOST_TEST_EQ(f.t.node_count(), 3U);
    BOOST_TEST(f.t.assign(1, integer(5)).has_value());
    BOOST_TEST_EQ(f.t.node_count(), 1U);
}

// In an external buffer too small for what is asked, every operation fails with capacity and the
// content stays as it was; a buffer of every size from the smallest that holds an empty tree up.
void nothing_changes_when_there_is_no_room() {
    for (std::size_t size = 1024; size <= 16384; size += 16) {
        std::vector<std::byte> raw(size + 16);
        auto* base = reinterpret_cast<std::byte*>((reinterpret_cast<std::uintptr_t>(raw.data()) + 15) & ~std::uintptr_t{15});
        detail::region r = detail::region::external({base, size});
        auto t = tree::format(r);
        if (!t) {
            BOOST_TEST(t.error() == wd::errc::capacity);
            continue;
        }
        if (!t->assign(1, array())) {
            continue;   // too small even for an empty array: nothing changed, as the next check would show
        }
        for (int i = 0; i < 200; ++i) {
            const std::string before = dump(*t);
            std::vector<detail::item<traits>> items{string("a string long enough to take a block"), object(), integer(i)};
            auto done = t->insert_elements(1, t->count(1) / 2, items);
            if (!done) {
                BOOST_TEST(done.error() == wd::errc::capacity);
                BOOST_TEST_EQ(dump(*t), before);
                break;
            }
        }
    }
}

// The bound a node's probe asks for covers the block the container really takes.
void the_node_probe_covers_a_node() {
    fixture f;
    BOOST_TEST(f.t.assign(1, array()).has_value());
    auto& h = detail::heap_of(f.r);
    const auto before = h.get_free_memory();
    std::vector<detail::item<traits>> one{integer(1)};
    BOOST_TEST(f.t.insert_elements(1, 0, one).has_value());
    BOOST_TEST_LE(before - h.get_free_memory(), detail::node_probe_bytes<traits>);
}

void a_tree_reopens_after_its_image_moves() {
    fixture f;
    BOOST_TEST(f.t.assign(1, object()).has_value());
    BOOST_TEST(f.t.set_member(1, "k", string("value")).has_value());
    std::vector<std::byte> copy(f.r.data(), f.r.data() + f.r.size());
    std::vector<std::byte> aligned(copy.size() + 16);
    auto* base = reinterpret_cast<std::byte*>((reinterpret_cast<std::uintptr_t>(aligned.data()) + 15) & ~std::uintptr_t{15});
    std::memcpy(base, copy.data(), copy.size());
    detail::region moved = detail::region::external({base, copy.size()});
    auto t = tree::attach(moved);
    if (!BOOST_TEST(t.has_value())) { return; }
    BOOST_TEST_EQ(dump(*t), dump(f.t));
}

void a_deep_copy_between_trees();   // item with from/from_id: a subtree of one tree copied into another

}  // namespace

int main() {
    a_new_tree_holds_null();
    javascript_key_order();
    index_lookalikes();
    insertion_and_sorted_orders();
    elements_are_renumbered();
    replacing_a_container_frees_its_subtree();
    nothing_changes_when_there_is_no_room();
    the_node_probe_covers_a_node();
    a_tree_reopens_after_its_image_moves();
    a_deep_copy_between_trees();
    return boost::report_errors();
}
```

  The helpers and the two cases declared without a body are written in full by the implementer, with
  the expectations their comments state: `insertion_and_sorted_orders` checks that the keys `b, 2, a,
  1` come out `b, 2, a, 1` under `insertion_order` and `1, 2, a, b` under `sorted`;
  `a_deep_copy_between_trees` builds `{"a":[1,"x",{"b":true}]}` in one tree, copies the node of
  `"a"` into another tree's root, and compares the dumps, then changes the source and checks the copy
  is unchanged. `test/Jamfile`: `webcpp.run tree : tree_test.cpp ;`.

- [ ] **Step 2: Run it and watch it fail.** `B2 toolset=clang libs/document/test//tree`: no
  `detail/tree.hpp`.

- [ ] **Step 3: Implement.**
  - The container: `by_id` `ordered_unique<member<id>>`; `by_order`
    `ordered_unique<composite_key<parent, group, ordinal>>`; `by_key`
    `ordered_non_unique<composite_key<parent, key_view>, composite_key_compare<std::less<>, std::less<>>>`
    where `key_view` is a key extractor returning the key's bytes as a `std::string_view`.
  - The container is the heap's first allocation (placement-new on `allocate_aligned`); its offset
    from the image's start and the next id are written in the header, with `layout_tag<Traits>()`,
    `format_version`, `0x01020304` and `BOOST_VERSION`. `attach` runs `check_image` with
    `layout_tag<Traits>()`, then the heap's `check_sanity()` (`errc::not_a_document` when false).
  - Keys arrive in UTF-8; with `utf16` they are transcoded before they are stored and before a lookup
    (a key a lookup cannot transcode finds nothing).
  - A new member's place: `javascript_order`: `array_index(key)` gives group 0 and that ordinal, else
    group 1 and the object's next insertion number (then incremented); `insertion_order`: always the
    latter; `sorted`: group 1 and the node's own id, and `for_each_child` walks `by_key` instead.
  - Every mutation runs in three phases. **Room:** list the blocks it will take
    (`node_probe_bytes` per node to create, counting a literal's entries and a copied subtree's
    nodes, and each string's exact size); while `probe` fails, `region::grow(size + 2 * total)`, and
    when that fails return `errc::capacity` before anything changed. Growth moves the image: no node
    pointer is held across this phase. **Allocate:** the strings (`make_string`), then each node by
    `emplace` with its fields. **Arrange:** what does not allocate: `modify` that swaps a prepared
    `byte_string` or scalar into a node (never a lambda that can throw), ordinal shifts (an insertion
    at `at` shifts the later elements from the last one down, so the unique index never collides),
    erasing a subtree (children first, their strings freed), the length and the next insertion number
    in the container's node.
  - `assign` on a node that holds a container erases the old children after the new value is in
    place; the root may be assigned any kind.
  - `layout_tag<Traits>()` (in `image.hpp`, defined here where the node's type is known): FNV-1a over
    `traits_tag<Traits>()`, `sizeof` and `alignof` of `node<Traits>`, of `container<Traits>` and of
    `heap`.

- [ ] **Step 4: Run it and watch it pass**, the five lanes (each without exceptions too), the
  report, the reference, the lint.

- [ ] **Step 5: Commit**, with the changelog line
  `* Added JavaScript's key order, kept by every change to an object.`

```bash
git add include/webcpp/document/detail/tree.hpp include/webcpp/document/image.hpp \
  test/tree_test.cpp test/Jamfile doc/changelog.adoc
git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "document: the tree, its key order, and changes that find their room first"
```

### Task 6: The document, its handles, literals and images

**Files:**
- Create: `include/webcpp/document/document.hpp`, `include/webcpp/document/value.hpp`,
  `include/webcpp/document/literal.hpp`, `test/value_test.cpp`, `test/order_test.cpp`,
  `test/image_test.cpp`
- Modify: `include/webcpp/document/image.hpp` (`format`, `open`, `from_image`), `test/Jamfile` (three
  lines), `include/webcpp/document.hpp` (three includes)

**Interfaces:**
- Consumes: Tasks 1 to 5, in particular `detail::tree<Traits>` and its operations, `detail::item`,
  `detail::literal_entry`, `detail::region`, `number_codec`, `scalar_codec`, `detail::utf16_length`,
  `detail::read_date`, `detail::read_base64`.
- Produces (Tasks 7 to 12 use exactly these):

```cpp
namespace webcpp::document {
struct owned {};      // storage tags
struct external {};

template <class Traits = default_traits, class Storage = owned> class basic_document;
template <class Traits = default_traits, class Storage = owned> class basic_value;        // reads and writes
template <class Traits = default_traits, class Storage = owned> class basic_const_value;  // reads
using document = basic_document<>;
using value = basic_value<>;
using const_value = basic_const_value<>;

// Literals: what JavaScript writes as {...} and [...]; plain C++ values, converted when stored.
class object;   // object{{"key", v}, ...}; object() is {}
class array;    // array{v, ...}; array() is []
// v: bool, any integer, decimal32/64/128, double (only into js_traits), const char*, std::string_view,
//    std::string, std::u16string_view, date_ms, std::span<const std::byte>, std::nullptr_t, null_t,
//    object, array.

template <class Traits, class Storage> class basic_document {
public:
    static result<basic_document> create(std::uint64_t max_bytes = 1ULL << 30) noexcept
        requires std::same_as<Storage, owned>;
    basic_document(basic_document&&) noexcept;
    basic_document& operator=(basic_document&&) noexcept;
    result<basic_document<Traits, owned>> clone() const noexcept;           // an owned copy of the image

    basic_value<Traits, Storage> root() noexcept;
    basic_const_value<Traits, Storage> root() const noexcept;
    basic_value<Traits, Storage> operator[](std::string_view key) noexcept;      // root()[key]
    basic_value<Traits, Storage> operator[](std::uint64_t index) noexcept;
    basic_const_value<Traits, Storage> operator[](std::string_view key) const noexcept;
    basic_const_value<Traits, Storage> operator[](std::uint64_t index) const noexcept;

    std::span<const std::byte> image() const noexcept;
    result<void> shrink_to_fit() noexcept requires std::same_as<Storage, owned>;
    friend bool operator==(const basic_document& a, const basic_document& b) noexcept;   // root() == root()
};

template <class Traits> result<basic_document<Traits, external>> format(std::span<std::byte> buffer) noexcept;
template <class Traits> result<basic_document<Traits, external>> open(std::span<std::byte> buffer) noexcept;
template <class Traits> result<basic_document<Traits, owned>> from_image(std::span<const std::byte> bytes,
                                                                         std::uint64_t max_bytes = 1ULL << 30) noexcept;
// (Traits defaults to default_traits in each.)

template <class Traits, class Storage> class basic_const_value {
public:
    kind kind() const noexcept;
    bool is_undefined() const noexcept;  // and is_null, is_boolean, is_number, is_integer, is_string,
                                         // is_date, is_bytes, is_array, is_object
    template <class T> result<T> get() const noexcept;
    template <class T> T value_or(T fallback) const noexcept;
    bool contains(std::string_view key) const noexcept;
    std::uint64_t size() const noexcept;
    result<std::uint64_t> length() const noexcept;
    basic_const_value operator[](std::string_view key) const noexcept;
    basic_const_value operator[](std::uint64_t index) const noexcept;
    basic_const_value at(std::int64_t index) const noexcept;      // negative from the end
    /* range */ entries() const noexcept;   // pairs of key and basic_const_value, the policy's order
    /* range */ keys() const noexcept;
    /* range */ values() const noexcept;
    /* begin(), end() */                    // the elements of an array; empty for any other kind
    friend bool operator==(const basic_const_value&, const basic_const_value&) noexcept;
};
// Keys come out as std::string_view with utf8 and std::u16string_view with utf16.

template <class Traits, class Storage> class basic_value : public basic_const_value<Traits, Storage> {
public:
    basic_value operator[](std::string_view key) noexcept;
    basic_value operator[](std::uint64_t index) noexcept;
    basic_value at(std::int64_t index) noexcept;
    template <class T> [[nodiscard]] result<void> set(const T& v) noexcept;   // every settable type,
                                                                              // undefined, a const value
    [[nodiscard]] result<bool> erase(std::string_view key) noexcept;
    template <class S> [[nodiscard]] result<void> assign_from(basic_const_value<Traits, S> source) noexcept;
    operator basic_const_value<Traits, Storage>() const noexcept;
};
}
```

- [ ] **Step 1: Write the failing tests.** Every case below, each a function in its file.

`test/value_test.cpp`:
- `a_new_document_holds_null`: `create()`; `root().kind() == kind::null`; `doc["x"].is_undefined()`.
- `navigation_never_fails`: on `{"a":{"b":[1,2]}}` built with `set` and literals:
  `doc["a"]["b"][1].get<std::int64_t>() == 2`; `doc["a"]["b"][9].is_undefined()`;
  `doc["a"]["b"][1]["x"].is_undefined()`; `doc["zz"]["y"]["w"].is_undefined()`;
  `doc["a"]["b"].at(-1).get<std::int64_t>() == 2`; `doc["a"]["b"].at(-3).is_undefined()`.
- `set_creates_and_replaces`: `doc["user"].set(wd::object())`, then `doc["user"]["age"].set(30)`
  creates; a second `set(31)` replaces; `doc["missing"]["x"].set(1).error() == errc::type` and
  changes nothing; `doc["user"]["age"].set(wd::undefined)` erases (`contains("age")` false);
  `doc["arr"].set(wd::array{1, 2})` then `doc["arr"][4].set(5)` gives length 5 with `null` at 2 and 3.
- `numbers_by_value`: `set(1)`, `set(decimal128 "1.0")`, `set(decimal128 "1e0")`, `set(decimal128
  "1.5e1")` read `is_integer()` and `get<std::int64_t>()` 1, 1, 1, 15; `set(decimal128 "1.50")` is
  not integral and `get<std::int64_t>().error() == errc::range`; `set(std::uint64_t max)` is a number
  read back exactly by `get<std::uint64_t>()`; `get<std::int8_t>()` of 300 is `errc::range`; a NaN or
  infinite decimal is `errc::range`; `get<boost::decimal::decimal64_t>()` of a 34-digit number is
  `errc::range`.
- `strings_dates_bytes`: `set("text")` and `get<std::string_view>()`; `length()` of `"aé€😀"`
  (UTF-8) is 5; invalid UTF-8 is `errc::type`; `set(date_ms(1791774245678ms))` is `kind::date`;
  `get<date_ms>()` of the string `"2026-10-12T03:04:05.678Z"` converts, of `"x"` is `errc::type`;
  `set(std::span<const std::byte>)` is `kind::bytes`; `get<std::vector<std::byte>>()` of the string
  `"Zm9v"` decodes (`base64`).
- `js_traits_strings`: with `wd::basic_document<wd::js_traits>`, the same string's `length()` is 5,
  `entries()` yields `std::u16string_view` keys, and `set(1.5)` (a `double`) reads back `1.5`.
- `value_or_is_nullish_coalescing`: `doc["absent"].value_or(18) == 18`; `null` gives the fallback;
  a present `30` gives 30; a present string asked as an integer gives the fallback.
- `erase_and_assign_from`: `erase("age")` true then false; `assign_from` of `{"a":1,"b":2}` into
  `{"b":0,"c":3}` gives `{"b":2,"c":3,"a":1}` in that order (`Object.assign`).
- `deep_copies_never_alias`: `doc["copy"].set(other["user"])`, then change `other`: `doc` unchanged;
  `doc["a"].set(doc["b"])` within one document copies.
- `handle_survives_growth`: `auto h = doc["k"]; h.set(wd::array())`; push 20,000 strings of 100 bytes
  through `doc["k"][i].set(...)` (the image doubles many times); `h.length() == 20000`;
  `h[19999].get<std::string_view>()` is the last string.
- `equality`: `{"a":1,"b":[1.0,"x"]}` equals `{"b":[1,"x"],"a":1}`; not equal to `{"a":1}`;
  `1` equals `1.0`; dates and bytes by value.
- `iteration`: `entries()`, `keys()`, `values()` in the order of `{"b":1,"2":2,"a":3,"1":4}`
  (`1, 2, b, a`); `for (auto v : arr)` over `[1, "x", null]`.
- `copy_in_every_size`: for each external buffer size from the smallest that formats up to the size
  that fits all, a document holding `{"a":[...100 strings...]}` copied into it with
  `root().set(source.root())` either succeeds wholly or fails with `errc::capacity` leaving `null`.

`test/order_test.cpp`: Review Focus 4 through the public API: the keys `"x"`, `"01"`,
`"4294967295"`, `"-0"`, `"1e2"`, `""`, `"4294967294"`, `"0"` and a key holding `\0` set in that
order come out `0, 4294967294, x, 01, 4294967295, -0, 1e2, "", "\0k"`; assignment keeps a place;
erase and re-create puts a non-index key last and an index key back among the indices; the same set
under `insertion_order` and `sorted` traits.

`test/image_test.cpp`:
- `an_image_moves`: `create()`, fill `{"k":"value","n":[1,2,3]}`, copy `image()` into another
  16-aligned buffer, `open<wd::default_traits>(copy)` equals the original; `from_image` gives an owned
  copy that grows.
- `format_and_open`: `format` into a 64 KB buffer gives `null`; `open` of a buffer that was never
  formatted is `errc::not_a_document`; `open<wd::js_traits>` of a `default_traits` image is
  `errc::incompatible_document`.
- `the_same_operations_give_the_same_image`: two documents built by the same calls at different
  addresses have identical `image()` bytes.
- `an_erased_secret_leaves_no_byte`: set a 64-byte secret string, erase it, search `image()`.
- `shrink_to_fit`: a document grown to 1 MB and emptied shrinks; its content is unchanged.
- `an_external_document_says_full`: in a 4 KB buffer, `set` of a 10 KB string is `errc::capacity`
  and the document is unchanged.

`test/Jamfile`: `webcpp.run value : value_test.cpp ;`, `webcpp.run order : order_test.cpp ;`,
`webcpp.run image : image_test.cpp ;`.

- [ ] **Step 2: Run them and watch them fail.** `B2 toolset=clang libs/document/test//value`: no
  `value.hpp`.

- [ ] **Step 3: Implement.**
  - A handle holds `basic_document*`, and one of: a node id; a missing member (the parent's id and the
    key, a `std::string`); a missing element (the parent's id and the index); nothing. Navigation
    resolves through `tree::member` and `tree::element` each time, never caching a node pointer.
  - `set(v)` turns `v` into a `detail::item` (a literal through `literal_entry`s it builds in a
    `std::vector`), then: a node is `tree::assign`ed; a missing member `tree::set_member`; a missing
    element past the end `tree::set_length` then `tree::insert_elements`, done as one operation (both
    phases' rooms are probed together); nothing is `errc::type`; `undefined` erases a member and stores
    `null` in an element.
  - Numbers by value: an integer type that fits `Traits::integer` is `repr::integer`; any other number
    becomes the traits' `number` through `number_codec<number>::parse` of its exact text
    (`write_javascript` of its digits), and a `number` whose `integral()` fits `integer` is stored as
    an integer. Non-finite is `errc::range`.
  - `get<T>`: the conversions of the spec's section 6.3; a string asked as `date_ms` goes through
    `detail::read_date`; as bytes, through the traits' bytes policy (`read_base64`, or the
    `javascript_bytes` object shape).
  - `length()`: an array's count; a string's UTF-16 units (`utf16_length` with `utf8`, the size in
    units with `utf16`); else `errc::type`.
  - `operator==` walks both trees; objects compare by key lookups, so order does not matter.
  - The ranges iterate `tree::for_each_child` lazily (an iterator holding the parent and the current
    child's `(group, ordinal)` or key, finding the next by `upper_bound` on the index each step).
  - `format`, `open`, `from_image`, `clone`, `shrink_to_fit` on `detail::region` and `tree::format`
    / `tree::attach`.
  - Every public symbol documented; `basic_value::set`'s comment lists the settable types.

- [ ] **Step 4: Run them and watch them pass**, the five lanes (each without exceptions too: these
  tests use only the core), the report, the reference, the lint.

- [ ] **Step 5: Commit**, with the changelog line
  `* Added handles that read and write a document as JavaScript does.`

```bash
git add include/webcpp/document/document.hpp include/webcpp/document/value.hpp \
  include/webcpp/document/literal.hpp include/webcpp/document/image.hpp include/webcpp/document.hpp \
  test/value_test.cpp test/order_test.cpp test/image_test.cpp test/Jamfile doc/changelog.adoc
git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "document: the document, its handles, literals and images"
```

### Task 7: JavaScript's array methods

**Files:**
- Create: `test/array_test.cpp`
- Modify: `include/webcpp/document/value.hpp` (the methods), `test/Jamfile` (one line)

**Interfaces:**
- Consumes: Task 6's handles and `tree::insert_elements`, `erase_elements`, `reverse`, `set_length`.
- Produces, on `basic_value<Traits, Storage>` (`slice`, `concat` also on `basic_const_value`):

```cpp
template <class... T> [[nodiscard]] result<std::uint64_t> push(const T&... v) noexcept;      // the new length
[[nodiscard]] result<basic_document<Traits, owned>> pop() noexcept;                          // removed, or undefined
[[nodiscard]] result<basic_document<Traits, owned>> shift() noexcept;
template <class... T> [[nodiscard]] result<std::uint64_t> unshift(const T&... v) noexcept;
template <class... T> [[nodiscard]] result<basic_document<Traits, owned>> splice(std::int64_t start, std::int64_t count,
                                                                                const T&... v) noexcept;
[[nodiscard]] result<basic_document<Traits, owned>> splice(std::int64_t start) noexcept;
[[nodiscard]] result<void> reverse() noexcept;
[[nodiscard]] result<void> set_length(std::uint64_t n) noexcept;
result<basic_document<Traits, owned>> slice(std::int64_t begin = 0,
                                            std::int64_t end = std::numeric_limits<std::int64_t>::max()) const noexcept;
template <class... T> result<basic_document<Traits, owned>> concat(const T&... v) const noexcept;
```

  A document "holding undefined" is a document whose root reads `undefined`: `basic_document` gains
  that one state for `pop` and `shift` of an empty array (`root().is_undefined()`, and `set` on its
  root makes it a value).

- [ ] **Step 1: Write the failing test** `test/array_test.cpp`. Each case starts from `[0,1,2,3,4]`
  and checks the returned value and the array, both through `wd::stringify` once Task 8 exists; until
  then through a local `text(const_value)` helper that writes the same compact JSON for the kinds the
  test uses. The expected results are Node 26's, measured:

| Call | Returns | Array after |
| --- | --- | --- |
| `splice(1, 2)` | `[1,2]` | `[0,3,4]` |
| `splice(1, 0, "x", "y")` | `[]` | `[0,"x","y",1,2,3,4]` |
| `splice(-2)` | `[3,4]` | `[0,1,2]` |
| `splice(-9, 1)` | `[0]` | `[1,2,3,4]` |
| `splice(9, 1, "z")` | `[]` | `[0,1,2,3,4,"z"]` |
| `splice(2, -1, "z")` | `[]` | `[0,1,"z",2,3,4]` |
| `splice(2)` | `[2,3,4]` | `[0,1]` |
| `splice(1, 99)` | `[1,2,3,4]` | `[0]` |
| `slice(1, 3)` | `[1,2]` | unchanged |
| `slice(-2)` | `[3,4]` | unchanged |
| `slice(3, 1)` | `[]` | unchanged |
| `slice(-9, -4)` | `[0]` | unchanged |
| `push(5, 6)` | `7` | `[0,1,2,3,4,5,6]` |
| `pop()` | `4` | `[0,1,2,3]` |
| `shift()` | `0` | `[1,2,3,4]` |
| `unshift("a", "b")` | `7` | `["a","b",0,1,2,3,4]` |
| `reverse()` | | `[4,3,2,1,0]` |
| `concat(wd::array{5, wd::array{6}}, 7)` | `[0,1,2,3,4,5,[6],7]` | unchanged |
| `at(-1)`, `at(5)`, `at(-6)` | `4`, undefined, undefined | unchanged |
| `set_length(2)` | | `[0,1]` |
| `set_length(7)` | | `[0,1,2,3,4,null,null]` |
| `[7].set(9)` | | `[0,1,2,3,4,null,null,9]` |

  Also: on `[]`, `pop()` and `shift()` return a document whose root is undefined, and `splice(0, 1)`
  returns `[]`; every method on a non-array is `errc::type` and changes nothing; `concat` of a
  `const_value` holding an array spreads its elements, of one holding an object appends it; and
  `splice_in_every_size` (Review Focus 2): in an external buffer of every size from the smallest that
  holds `[0,...,99]` upward, `splice(50, 10, <20 strings of 64 bytes>)` either completes or returns
  `errc::capacity` with the array unchanged. `test/Jamfile`: `webcpp.run array : array_test.cpp ;`.

- [ ] **Step 2: Run it and watch it fail.** `B2 toolset=clang libs/document/test//array`: no member
  `push`.

- [ ] **Step 3: Implement** on Task 5's operations. JavaScript's index rules, written once in a
  `detail` function used by `splice`, `slice` and `at`:

```cpp
// ECMA-262 ToIntegerOrInfinity then relative indexing: a negative index counts from the end,
// clamped to [0, length].
constexpr std::uint64_t relative_index(std::int64_t i, std::uint64_t length) noexcept {
    if (i < 0) {
        const std::uint64_t back = static_cast<std::uint64_t>(-(i + 1)) + 1;   // no overflow at INT64_MIN
        return back >= length ? 0 : length - back;
    }
    return static_cast<std::uint64_t>(i) > length ? length : static_cast<std::uint64_t>(i);
}
```

  `splice(start, count, v...)`: `s = relative_index(start)`, `n = clamp(count, 0, length - s)`;
  copy elements `[s, s + n)` into the returned document first (an owned document, its own image), then
  one tree operation removing `n` at `s` and inserting the new items, probed as one room, so a failure
  leaves both unchanged. `pop`, `shift` and `splice(start)` are this with fixed arguments. `slice` and
  `concat` build a new owned document only. Each method's Doc Comment names its JavaScript
  counterpart and the index rule.

- [ ] **Step 4: Run it and watch it pass**, the lanes (also without exceptions), the report, the
  reference, the lint.

- [ ] **Step 5: Commit**, with the changelog line
  `* Added JavaScript's array methods, splice included, with its index rules.`

```bash
git add include/webcpp/document/value.hpp test/array_test.cpp test/Jamfile doc/changelog.adoc
git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "document: JavaScript's array methods"
```

### Task 8: `stringify` and `stringify_to`

**Files:**
- Create: `include/webcpp/document/stringify.hpp`, `include/webcpp/document/detail/writer.hpp`,
  `test/stringify_test.cpp`
- Modify: `include/webcpp/document/value.hpp`, `document.hpp` (the `stringify` members),
  `test/array_test.cpp` (its local `text` helper replaced by `wd::stringify`), `test/Jamfile`,
  `include/webcpp/document.hpp`

**Interfaces:**
- Consumes: Task 6's handles and ranges; `number_codec<number>::write`,
  `write_javascript(from_int64(...))`, `detail::write_date`, `detail::write_base64`.
- Produces:

```cpp
template <class Traits, class Storage>
std::string stringify(basic_const_value<Traits, Storage> v, std::uint32_t indent = 0);
template <class Traits, class Storage>
std::string stringify(const basic_document<Traits, Storage>& d, std::uint32_t indent = 0);
template <class Traits, class Storage, class Sink>   // Sink: void(std::string_view) or result<void>(std::string_view)
result<void> stringify_to(basic_const_value<Traits, Storage> v, Sink&& sink, std::uint32_t indent = 0,
                          std::uint64_t chunk = 4096);
// members: basic_const_value::stringify(std::uint32_t indent = 0), basic_document::stringify(...)
```

  `indent` is `JSON.stringify`'s third argument as a number: clamped to 10, and 0 writes compactly.
  A value that reads `undefined` writes nothing (an empty string): `JSON.stringify(undefined)` returns
  `undefined`, not text.

- [ ] **Step 1: Write the failing test** `test/stringify_test.cpp`. The document is built with
  `set` and literals as
  `object{{"b", array{1, object{{"c", array()}}, object()}}, {"a", s}, {"e", nullptr}, {"t", true}}`
  where `s` is the UTF-8 text `x"\<BS><FF><LF><CR><TAB><U+0001><U+001F>/é<U+2028>😀`. The expected
  texts are Node 26's `JSON.stringify(v, null, n)`, measured:

```cpp
const std::string escaped = R"(x\"\\\b\f\n\r\t\u0001\u001f/)" "\xc3\xa9" "\xe2\x80\xa8" "\xf0\x9f\x98\x80";
BOOST_TEST_EQ(wd::stringify(doc),
    R"({"b":[1,{"c":[]},{}],"a":")" + escaped + R"(","e":null,"t":true})");
BOOST_TEST_EQ(wd::stringify(doc, 2),
    "{\n  \"b\": [\n    1,\n    {\n      \"c\": []\n    },\n    {}\n  ],\n  \"a\": \"" + escaped +
    "\",\n  \"e\": null,\n  \"t\": true\n}");
BOOST_TEST_EQ(wd::stringify(doc, 1).substr(0, 11), "{\n \"b\": [\n  ");
BOOST_TEST_EQ(wd::stringify(doc, 12), wd::stringify(doc, 10));   // clamped, as JavaScript does
```

  And: `[]` and `{}` write `[]` and `{}` at any indent; `[[]]` at indent 2 writes `"[\n  []\n]"`;
  numbers write through the codec (`1.5`, `1e+21`, `-9223372036854775808`); a date writes
  `"2026-10-12T03:04:05.678Z"`; bytes `{1, 255}` write `"Af8="` with `base64`, `"Af8"` with
  `base64url`, and `{"0":1,"1":255}` with `js_traits` (Node's, measured); with `js_traits` (`utf16`), a
  string holding the lone unit 0xD800 set through `std::u16string_view` writes `"\ud800"` (Node:
  `JSON.stringify("\ud800")` is `"\ud800"` with the escape); a document whose root is undefined writes
  `""`. `stringify_to`: for every chunk size from 1 to 64, the pieces concatenated equal `stringify`'s
  text and no piece is longer than the chunk; a sink returning `errc::capacity` after its third piece
  makes `stringify_to` return `errc::capacity` and the document reads as before.
  `test/Jamfile`: `webcpp.run stringify : stringify_test.cpp ;`.

- [ ] **Step 2: Run it and watch it fail.** `B2 toolset=clang libs/document/test//stringify`: no
  `stringify.hpp`.

- [ ] **Step 3: Implement** one writer, `detail::writer<Traits, Out>`, that walks a `const_value`
  depth-first with an explicit stack (no recursion: depth is unbounded by the API) and emits through
  `Out`, a buffer that flushes to the sink every `chunk` bytes; `stringify` is the writer with a
  `std::string` out. JavaScript's rules (ECMA-262 SerializeJSONObject, SerializeJSONArray,
  QuoteJSONString): with an indent, a non-empty container opens, each entry on a new line at the
  current depth's indent, `"key": value` with one space after the colon, entries joined by `,`, the
  closer on its own line at the parent's indent; an empty container writes `{}` or `[]`;
  QuoteJSONString escapes `"`, `\`, `\b \f \n \r \t`, other units below 0x20 as `\u00` and two
  lowercase hex digits, and a lone surrogate (`utf16`) as `\u` and its four lowercase hex digits;
  everything else is copied (UTF-8 as it is; `utf16` transcoded to UTF-8).

- [ ] **Step 4: Run it and watch it pass**, the lanes (also without exceptions), the report, the
  reference, the lint.

- [ ] **Step 5: Commit**, with the changelog line
  `* Added stringify and stringify_to, writing JSON.stringify's exact bytes.`

```bash
git add include/webcpp/document/stringify.hpp include/webcpp/document/detail/writer.hpp \
  include/webcpp/document/value.hpp include/webcpp/document/document.hpp include/webcpp/document.hpp \
  test/stringify_test.cpp test/array_test.cpp test/Jamfile doc/changelog.adoc
git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "document: stringify and stringify_to, as JSON.stringify writes"
```

### Task 9: Parsing, whole and in chunks

**Files:**
- Create: `include/webcpp/document/parse.hpp`, `include/webcpp/document/parser.hpp` (the parser
  without selectors), `include/webcpp/document/detail/handler.hpp`, `test/parse_test.cpp`,
  `test/streaming_test.cpp`
- Modify: `test/Jamfile` (two lines), `include/webcpp/document.hpp`

**Interfaces:**
- Consumes: Tasks 1 to 8; Boost.JSON's `basic_parser` (`boost/json/basic_parser_impl.hpp`, with
  `/webcpp//boost_json` linked by the alias); `~/.buzz/RESEARCH/WEBCPP_JSON_SAX_SPIKE.cpp` (a working
  handler with `number_precision::none`).
- Produces:

```cpp
struct parse_options {
    std::uint64_t max_depth = 256;
    std::uint64_t max_bytes = std::numeric_limits<std::uint64_t>::max();           // every chunk's total
    std::uint64_t max_document_bytes = 1ULL << 30;                                // an owned document's image
    bool revive_dates = false;
    bool revive_bytes = false;
};
template <class Traits = default_traits>
result<basic_document<Traits, owned>> parse(std::string_view text, const parse_options& o = {}) noexcept;
template <class Traits = default_traits>
result<basic_document<Traits, external>> parse_into(std::span<std::byte> buffer, std::string_view text,
                                                    const parse_options& o = {}) noexcept;

template <class Traits = default_traits, class Storage = owned> class basic_parser {
public:
    explicit basic_parser(const parse_options& o = {}) noexcept requires std::same_as<Storage, owned>;
    basic_parser(std::span<std::byte> buffer, const parse_options& o = {}) noexcept
        requires std::same_as<Storage, external>;
    result<void> write(std::string_view chunk) noexcept;
    result<basic_document<Traits, Storage>> finish() noexcept;
    void reset() noexcept;
    std::uint64_t offset() const noexcept;                 // bytes consumed; where a syntax error was found
    boost::system::error_code cause() const noexcept;      // Boost.JSON's code behind errc::syntax
};
using parser = basic_parser<>;
```

- [ ] **Step 1: Write the failing tests.**

`test/parse_test.cpp`, each row a check of `parse` (accepted: `stringify` of the result; refused:
the `errc`):
- accepted as `JSON.parse` accepts, written as Node writes: `" [1 , 2] "` gives `[1,2]`;
  `"{\"b\":1,\"2\":2,\"a\":3,\"1\":4}"` gives `{"1":4,"2":2,"b":1,"a":3}`;
  `"{\"a\":1,\"b\":2,\"a\":3}"` gives `{"a":3,"b":2}` (a repeated key: the last value, the first place);
  `"{\"a\":[1],\"a\":{\"x\":1}}"` gives `{"a":{"x":1}}`; `"\"\\u00e9\\ud83d\\ude00\""` gives the UTF-8
  text; `"-0"` gives `0`; `"1.0"` gives `1` and `is_integer()`; `"1e400"` gives `1e+400` (default
  traits); `"\"\\u2028\""` gives the raw U+2028; whitespace `" \t\n\r"` only.
- refused with `errc::syntax`: `""`, `"[1,]"`, `"{\"a\":1,}"`, `"// c\n1"`, `"[1] 2"`, `"01"`,
  `"+1"`, `".5"`, `"1."`, `"NaN"`, `"'a'"`, `"\"\\ud800\""` (a lone surrogate: the documented limit),
  `"\"\\x41\""`, `"\"a\nb\""` (a raw control character), `"\xff"`, `"[\"\xc0\xaf\"]"` (overlong UTF-8);
  `parser.offset()` of `"[1,]"` is 3.
- refused with `errc::range`: `"1e999999999999"`, a 35-digit number; accepted with `js_traits`: the
  35-digit number, written as Node writes it (`1.2345678901234568e+34` for
  `12345678901234567890123456789012345`).
- limits: `max_depth = 4`: `"[[[[1]]]]"` is accepted, `"[[[[[1]]]]]"` is `errc::capacity`;
  `max_bytes = 10`: an 11-byte text is `errc::capacity`; an object of 100,000 distinct keys parses and
  each key is found.
- revivers: with `revive_dates`, `"[\"2026-10-12T03:04:05.678Z\",\"2026-10-12\"]"` gives a date and a
  string; with `revive_bytes`, `"\"Zm9v\""` gives bytes `foo` and `"\"Zm9v!\""` stays a string.
- `parse_into_every_size` (Review Focus 2): `parse_into` a buffer of every size from 1 KB up to the
  size that holds `{"a":[...100 strings...]}` either succeeds and equals the owned parse, or returns
  `errc::capacity`.

`test/streaming_test.cpp`:
- `every_split_gives_the_same_document`: for each text of a corpus of 40 (every accepted row above,
  plus texts holding multi-byte characters, escapes `"\u1234"`, numbers `12.5e3`, literals `true`,
  `false`, `null`, keys of every length from 0 to 40), and for every position `p`, `write(text[0, p))`
  then `write(text[p, end))` then `finish()` equals `parse(text)`; and one byte at a time likewise.
- `a_syntax_error_has_the_same_offset_however_split`: for `"[1,2,}"` split at every position, the
  error is `errc::syntax` and `offset()` is the same as one-shot.
- `a_failed_parser_stays_failed`: after a failing `write`, `write` and `finish` return the same error
  until `reset()`, after which a new text parses.
- `chunk_limits`: `max_bytes = 100` spread over many chunks fails at the chunk that passes it.

`test/Jamfile`: `webcpp.run parse : parse_test.cpp ;`, `webcpp.run streaming : streaming_test.cpp ;`.

- [ ] **Step 2: Run them and watch them fail.** `B2 toolset=clang libs/document/test//parse`: no
  `parse.hpp`.

- [ ] **Step 3: Implement** the handler (`detail/handler.hpp`), a Boost.JSON `basic_parser` handler
  with the members of the spike (`max_object_size` and the rest, every `on_*` callback). It keeps a
  stack of frames (the container's node id, for an object the pending key, for an array the text
  index), and buffers of the pending key, string and number text (`on_key_part`, `on_string_part`,
  `on_number_part` append; the final callback completes). Each completed value becomes a
  `detail::item` stored with Task 5's operations: under an object, `tree::set_member` (a repeated key
  replaces the value in place: JavaScript's duplicate rule); under an array, `tree::insert_elements`
  at the end; at the top, `tree::assign` of the root. A container's begin stores an empty container and
  pushes its frame; its end pops. Numbers: `parse_options::numbers = number_precision::none`, and the
  completed text goes through Task 6's numbers-by-value rule. A failure of a tree operation is returned
  from the callback through its `error_code&` (Boost.JSON then stops), keeping the `errc`. `write` calls
  `write_some(true, ...)`; `finish` calls `write_some(false, nullptr, 0)`; `max_bytes` is counted before
  each write; `max_depth` is Boost.JSON's own (its `too_deep` mapped to `errc::capacity`); any other
  Boost.JSON error is `errc::syntax`, its code kept in `cause()` and its count in `offset()`. Boost.JSON
  validates UTF-8 and refuses a lone surrogate by default: no option changes that.

- [ ] **Step 4: Run them and watch them pass**, the lanes (also without exceptions), the report, the
  reference, the lint.

- [ ] **Step 5: Commit**, with the changelog line
  `* Added parse and an incremental parser, with JSON.parse's rules.`

```bash
git add include/webcpp/document/parse.hpp include/webcpp/document/parser.hpp \
  include/webcpp/document/detail/handler.hpp include/webcpp/document.hpp \
  test/parse_test.cpp test/streaming_test.cpp test/Jamfile doc/changelog.adoc
git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "document: parse, whole or in chunks, as JSON.parse does"
```

### Task 10: Selectors: values delivered as they complete

**Files:**
- Create: `include/webcpp/document/detail/selector.hpp`, `test/selector_test.cpp`
- Modify: `include/webcpp/document/parser.hpp` (`on`, `observe`, `path_view`),
  `include/webcpp/document/detail/handler.hpp` (the text path, delivery), `test/Jamfile`

**Interfaces:**
- Consumes: Task 9's parser and handler.
- Produces, on `basic_parser`:

```cpp
// F: void(basic_const_value<Traits, Storage>, path_view) or result<void>(...); the selector grammar:
// $ then .name | ['name'] (\' and \\ escapes) | [n] | [*] | .*
template <class F> result<void> on(std::string_view selector, F&& f);        // delivered, then dropped
template <class F> result<void> observe(std::string_view selector, F&& f);   // delivered, then kept

class path_view {                       // the text's path of a delivered value
public:
    std::size_t size() const noexcept;
    bool is_index(std::size_t i) const noexcept;
    std::string_view key(std::size_t i) const noexcept;    // UTF-8
    std::uint64_t index(std::size_t i) const noexcept;
    std::string to_string() const;      // normalised JSONPath: $['items'][3]
};
```

- [ ] **Step 1: Write the failing test** `test/selector_test.cpp`:
- `grammar`: `$`, `$.a`, `$['a b']`, `$['it\'s']`, `$[0]`, `$[*]`, `$.*`, `$.items[*].id` accepted;
  `a`, `$.`, `$[`, `$[-1]`, `$['a`, `$..a` refused with `errc::type`.
- `delivery`: on `{"meta":{"n":2},"items":[{"id":1},{"id":2}]}`, `on("$.items[*]")` receives two
  values with paths `$['items'][0]` and `$['items'][1]`, and `finish()` gives `{"meta":{"n":2},"items":[]}`;
  `observe("$.meta")` receives `{"n":2}` and the document keeps it; `on("$.*")` receives `meta` and
  `items` and the document is `{}`.
- `nested_first`: `observe("$.a.b")` and `observe("$.a")` on `{"a":{"b":1}}`: `$.a.b` first, then `$.a`
  holding `b`; with `on("$.a.b")` instead, `$.a` arrives without `b`.
- `scalars_when_their_token_ends`: `on("$[*]")` over `[1,"x",true,null]` delivers four values, the
  last when `null` ends even before the closing `]` is written.
- `a_failing_callback_fails_the_parser`: a callback returning `errc::range` makes `write` return it.
- `a_callback_cannot_change_the_document`: the callback's parameter is a `basic_const_value`, which
  has no `set`; the test only checks the parameter type with `static_assert(std::is_same_v<...>)`.
- `a_million_elements_in_bounded_memory` (Review Focus 6): `on("$[*]")` over `[{"i":0,"s":"..."}, ...]`
  of 1,000,000 elements written in 4 KB chunks generated on the fly; after `finish()`, the document's
  image is at most 64 KB and the callback counted 1,000,000. If a lane takes over 60 s, the count is
  lowered for `__wasm__` only, with a comment giving the time measured.
- `selectors_and_splits`: the delivery sequence is the same for the texts of Task 9's corpus split at
  every position.

`test/Jamfile`: `webcpp.run selector : selector_test.cpp ;`.

- [ ] **Step 2: Run it and watch it fail.** `B2 toolset=clang libs/document/test//selector`: no member
  `on`.

- [ ] **Step 3: Implement.** `detail/selector.hpp` parses a selector into a vector of steps
  (key, index, any element, any member) and matches a path of the same length step by step. The
  handler keeps the text path in its frames (keys and text indices, independent of the document's
  positions, which `on` changes). When a value completes (a scalar at its callback, a container at its
  end), every selector is matched against the path; each match calls its callback with a
  `basic_const_value` of the node and the `path_view`; then, if any `on` matched, the node is erased
  from its parent (`tree::erase_member`, or `tree::erase_elements` of that one element), which frees and
  zeroes its blocks. A callback's failure or exception leaves the parser failed (an exception is caught
  inside `write` only to mark the failure, then rethrown, in a build with exceptions).

- [ ] **Step 4: Run it and watch it pass**, the lanes (also without exceptions), the report, the
  reference, the lint.

- [ ] **Step 5: Commit**, with the changelog line
  `* Added selectors that deliver values while they stream, in bounded memory.`

```bash
git add include/webcpp/document/detail/selector.hpp include/webcpp/document/parser.hpp \
  include/webcpp/document/detail/handler.hpp test/selector_test.cpp test/Jamfile doc/changelog.adoc
git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "document: selectors that deliver values as they complete"
```

### Task 11: The sugar, with exceptions only

**Files:**
- Create: `test/sugar_test.cpp`, `test/no_exceptions_test.cpp`
- Modify: `include/webcpp/document/value.hpp`, `document.hpp`, `parse.hpp`, `test/Jamfile`

**Interfaces:**
- Consumes: every core function; `detail::raise`, `detail::exceptions_enabled` (Task 1).
- Produces (each `requires detail::exceptions_enabled<...>` on a template parameter, so a call is
  refused in a build without exceptions):

```cpp
template <class T> requires detail::exceptions_enabled<T>
basic_value& basic_value::operator=(const T& v);                   // set(v) or raise
template <class T> requires detail::exceptions_enabled<T>
T basic_const_value::as() const;                                   // get<T>() or raise
template <class C = char> requires detail::exceptions_enabled<C>
std::string_view basic_const_value::as_string() const;
template <class B = bool> requires detail::exceptions_enabled<B>
bool basic_const_value::as_bool() const;
template <class Traits = default_traits> requires detail::exceptions_enabled<Traits>
basic_document<Traits, owned> parse_or_throw(std::string_view text, const parse_options& o = {});
basic_document() requires detail::exceptions_enabled<Traits>;        // create() or raise
basic_document(const basic_document&) requires detail::exceptions_enabled<Traits>;   // clone() or raise
```

- [ ] **Step 1: Write the failing tests.** `test/sugar_test.cpp` (built with
  `<exception-handling>on`): `doc["user"]["age"] = 30;` then `as<int>() == 30`; `doc["x"]["y"] = 1`
  throws `type_error`; `doc["n"] = 300; doc["n"].as<std::int8_t>()` throws `range_error`;
  `as_string()` of a number throws `type_error`; `parse_or_throw("[1,]")` throws `syntax_error` with
  `offset() == 3`; a default-constructed document holds null; a copy is equal and independent; an
  external document's `= <10 KB string>` throws `capacity_error`; every exception's `code()` is the
  `errc` the core would have returned.
  `test/no_exceptions_test.cpp`, a `webcpp.compile-diagnostic` built with `<exception-handling>off`:
  each sugar member called once, each call preceded by the `// expected-error@+1 {{...}}` and
  `// expected-note@...` lines clang 23 prints for an unsatisfied constraint naming
  `exceptions_enabled` (written from the diagnostics measured when it is first compiled, as
  `libs/config/test` does). `test/Jamfile`:

```
webcpp.run sugar : sugar_test.cpp : <exception-handling>on ;
webcpp.compile-diagnostic no_exceptions : no_exceptions_test.cpp : <exception-handling>off ;
```

- [ ] **Step 2: Run them and watch them fail.** `sugar` does not compile (no `operator=`);
  `no_exceptions` fails its `-verify` (no diagnostic yet).
- [ ] **Step 3: Implement** each member as its core function followed by `detail::raise(code)` on
  failure (`syntax_error` with `parser.offset()`); nothing else.
- [ ] **Step 4: Run them and watch them pass**, the lanes (`no_exceptions` runs on the clang toolsets
  only, by its rule), the report, the reference, the lint.
- [ ] **Step 5: Commit**, with the changelog line
  `* Added JavaScript-style sugar that throws, compiled only with exceptions.`

```bash
git add include/webcpp/document/value.hpp include/webcpp/document/document.hpp \
  include/webcpp/document/parse.hpp test/sugar_test.cpp test/no_exceptions_test.cpp test/Jamfile doc/changelog.adoc
git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "document: the sugar that throws, refused without exceptions"
```

### Task 12: Node as the oracle

**Files:**
- Create: `test/oracle/Jamfile`, `test/oracle/package.json`, `test/oracle/package-lock.json`,
  `test/oracle/oracle.mjs`, `test/oracle/generate.mjs`, `test/fixtures/cases/*.json`,
  `test/fixtures/expected/*.json`, `test/oracle_test.cpp`
- Modify: `test/Jamfile`

**Interfaces:**
- Consumes: the whole core; xstate's oracle as the model (`libs/xstate/test/oracle/Jamfile`,
  `tools/oracle/oracle.jam`: `webcpp.original`, `webcpp.cases`, `webcpp.lane`; `update-expected`).
- Produces: the lane `oracle` (`b2 -a libs/document/test/oracle//oracle`), which the CI runs from
  `b2 declared-lanes`.

  A case file: `{"text": "<JSON text>", "indent": 0, "js_only": false, "operations": [...]}`, an
  operation one of `["set", path, value]`, `["erase", path, key]`, `["push", path, [values]]`,
  `["pop", path]`, `["shift", path]`, `["unshift", path, [values]]`, `["splice", path, start, count,
  [values]]`, `["reverse", path]`, `["set_length", path, n]`, `["assign_from", path, value]`, a `path`
  a list of keys and indices from the root. Its expected file:
  `{"text": "<JSON.stringify(result, null, indent)>", "returns": [<each operation's return, as JSON>]}`.

- [ ] **Step 1: Write the oracle and the cases.** `oracle.mjs <cases> <out>`: for each case,
  `JSON.parse(text)`, apply each operation in JavaScript, write the expected file. `generate.mjs`:
  writes `random-000.json` to `random-199.json` from a seeded generator (a fixed seed, an xorshift),
  documents of depth up to 5 with keys drawn from a set that includes index look-alikes, numbers
  drawn as doubles (`js_only` false) and, in a quarter of the cases, 18 to 30 digit numbers
  (`js_only` true), and 1 to 10 random operations; run once, its output committed. Hand-written cases
  `order-*.json`, `numbers-*.json`, `arrays-*.json`, `strings-*.json`, `indent-*.json` cover each
  earlier test's JavaScript facts. `test/oracle/Jamfile`:

```
(the notice)

import webcpp ;

webcpp.original node ;
webcpp.cases document : oracle.mjs : ../fixtures/cases : ../fixtures/expected ;
webcpp.lane oracle : cases-document ;
```

- [ ] **Step 2: Write the failing C++ side** `test/oracle_test.cpp`: for each case, parse `text`
  with `default_traits` (skipping `js_only` cases) and with `js_traits`, apply the operations through
  the core API, and compare `stringify` and the returns with the expected file; the fixtures reach the
  program as xstate's do (`path-constant`, a quoted `WEBCPP_TEST_DOCUMENT_FIXTURES` define,
  `<dependency>` on the glob, `[ webcpp.data ... ]` for the WASI targets). Run before the expected
  files exist: it fails naming the first missing file.
- [ ] **Step 3: Generate the expected files** with `B2 libs/document/test/oracle//update-expected`,
  read a sample of them against the earlier tests' tables, and run `B2 -a libs/document/test/oracle//oracle`
  (the expected files match) and the C++ side on the five lanes.
- [ ] **Step 4: Commit**, no changelog line (tests only).

```bash
git add test/oracle test/fixtures test/oracle_test.cpp test/Jamfile
git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "document: Node as the oracle of parse, the operations and stringify"
```

### Task 13: The page, the examples, the README

**Files:**
- Create: `example/Jamfile`, `example/.clang-tidy`, and with each its `.expected`:
  `example/document_quick_start.cpp`, `document_javascript.cpp`, `document_arrays.cpp`,
  `document_numbers.cpp`, `document_streaming.cpp`, `document_selectors.cpp`,
  `document_image.cpp`, `document_traits.cpp`, `document_no_exceptions.cpp`; `doc/introduction.adoc`,
  `doc/javascript.adoc`, `doc/handles.adoc`, `doc/numbers.adoc`, `doc/strings.adoc`,
  `doc/order.adoc`, `doc/arrays.adoc`, `doc/stringify.adoc`, `doc/parsing.adoc`,
  `doc/selectors.adoc`, `doc/traits.adoc`, `doc/images.adoc`, `doc/exceptions.adoc`,
  `doc/errors.adoc`
- Modify: `doc/document.adoc` (the includes), `doc/testing.adoc`, `README.md` (its example),
  `AGENTS.md` (the page's sections)

- [ ] **Step 1: The examples.** Each a small program of the topic its name gives, its output in its
  `.expected`, its listings between `// tag::<name>[]` and `// end::<name>[]`; `example/Jamfile`:
  `webcpp.targets native emscripten wasip2 wasip3 ;` and one `webcpp.example document_<topic>.cpp ;`
  each (`document_no_exceptions.cpp` with `<exception-handling>off`). `document_quick_start.cpp`
  parses a JSON text, reads, writes, pushes, and stringifies with indent 2.
- [ ] **Step 2: The page.** One section per file above, in that order, each with its anchor
  `[#document-<file>]`: why (the owner's rationale of the spec's section 1, with its measurements),
  the JavaScript-to-C++ table of section 6 and the differences of 6.8, the lone surrogate stated in
  `parsing.adoc` as the owner decided (Boost.JSON's parser does not deliver one; `JSON.parse` does),
  each listing an `include::{examples}/...[tag=...,indent=0]`, every count by attribute.
- [ ] **Step 3: The README** opens its "A minimal example" with
  `<!-- include::example/document_quick_start.cpp[tag=program] -->` and the region's text.
- [ ] **Step 4: Run** the five lanes with the examples, `B2 libs/document/doc` (strict reference, every
  anchor and `@see` resolved), and the lint.
- [ ] **Step 5: Commit**, no changelog line (the page).

```bash
git add example doc README.md AGENTS.md
git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "document: its page, examples and README"
```

### Task 14: Into the superproject, unpublished

**Files:** in `W` (branch `document`): `AGENTS.md` (chapter 1's table of libraries and its layout
line), `README.md` (its table of libraries).

- [ ] **Step 1: The rows.** AGENTS.md chapter 1: `| document | a JSON document that behaves as
  JavaScript does, in one relocatable image | native, wasip2, wasip3, emscripten | a local repository
  at libs/document, until webcpporg/document exists |`, and `document` in the layout line's list of
  libraries. README: `| [document](https://github.com/webcpporg/document) | a JSON document that
  behaves as JavaScript does | native, wasip2, wasip3, emscripten |`.
- [ ] **Step 2: The whole check, from scratch.** The five lanes of `libs/document`, the oracle lane,
  `B2 -a doc` (every page), the lint `lint: clean`, every `tools/**/*_test.py`; the report exits 0.
- [ ] **Step 3: Commit** in `W`:
  `git add AGENTS.md README.md` and `git commit -m "AGENTS.md, README.md: the document library"`
  (with the identity of the Global Constraints). The library is not yet a submodule: `libs/document`
  stays untracked in `W` until Task 15.

### Task 15: Publish (controller, with the owner's go)

- [ ] **Step 1:** with the owner's go, create `webcpporg/document` (public, empty), add it as the
  library's `origin`, check every commit's `%an %ae %cn %ce`, and push `main`.
- [ ] **Step 2:** in `W`, move the local repository aside, `git submodule add ../document.git
  libs/document`, check it out at the pushed commit, add the release notes' line under New Libraries,
  between config and pratt: `* link:{library-pages}document/{library-page}[document]: a JSON document
  that behaves as JavaScript does, for {cpp}20, from R.`, and commit `.gitmodules`,
  `libs/document` and `doc/releases/0.1.0.adoc`; the AGENTS.md row's last column becomes `a submodule
  at libs/document`.
- [ ] **Step 3:** fast-forward `M`'s `main` to `W`'s `document`, check the outgoing commits' identity,
  push `M`'s `main`, and watch the library's CI and the superproject's CI and Pages to green.

