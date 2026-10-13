# Working on json_schema

webcpp's rules apply here: read the superproject's
[AGENTS.md](../../AGENTS.md)
(<https://github.com/webcpporg/webcpp/blob/main/AGENTS.md>) first. This file
holds only what is specific to json_schema.

## What json_schema is

A library of webcpp's own, not a port: `meta/libraries.json` has
`"port-of": null`, and json_schema has no oracle and no twins. Its evidence is
the official JSON Schema Test Suite, and its page, with the Doc Comments of
its headers, is what a change is checked against.

## Cautions

- **The suite and its pin.** The JSON Schema Test Suite is the superproject's
  submodule `third_party/libs/JSON-Schema-Test-Suite`, pinned at a commit;
  moving the pin is a change of its own, with the known failures it moves. A
  test reaches it through `/webcpp/third_party//json-schema-test-suite`, which
  defines `WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE`, its root, and gives it to a
  WASI test through wasmtime's `--dir`. The suite is MIT: it is test data read
  at run time, never copied into this repository, so no corpus, fixture or
  expected output here holds its text.
- **Known failures never lie.** A runner's known failures are
  `test/known_failures/<runner>.json`, each entry with its reason. The runner
  fails on a failing test that is not listed, on a listed test that passes and
  on a listed test it cannot find. A list is recorded from a native run, never
  edited by hand.
- **The contract files.** The IR headers (`ir/ids.hpp`, `ir/number.hpp`,
  `ir/keyword.hpp`, `ir/node.hpp`, `ir/document.hpp`), the sink and its events
  (`detail/vm/event.hpp`, `detail/vm/sink.hpp`), the output record
  (`detail/output/record.hpp`), `output/error.hpp`, `output/outcome.hpp` and
  `output/message_hook.hpp` are the contract between the loader, the
  compiler, the VM, the emitter and the output. Once frozen, a change to one
  is proposed to the owner and applied in one change to every open branch.
  Appending a value to `keyword_kind`, `errc` or `error_kind` is not a change
  of the contract; reusing or reordering one is forbidden.
- **The errors are fixed.** `errc`'s values 1 to 20 never change and are
  never reused; a new error takes the next value. The category's name,
  `webcpp.json_schema`, and its identity, `0x6a736368656d6100`, never change
  either. `test/errors_test.cpp` pins them.
- **SRELL and its mirror.** The regex engine is SRELL, from the mirror
  `webcpporg/srell`: upstream's release imported as it is, and the patch that
  lets it build without exceptions as a commit of its own. It is the
  superproject's submodule `third_party/libs/srell`, linked through
  `/webcpp/third_party//srell`, which defines `SRELL_NO_UBMOD`, and
  `SRELL_NO_THROW` in a build without exceptions. Only the regex policy's
  header includes `<srell.hpp>`; `meta/include-boundaries.json` keeps every
  other header from it, and the lint enforces it. Boost.Locale, Boost.URL and
  webcpp::lexical are kept behind their own headers the same way.
- **Two allocators.** The schema's allocator holds everything a compiled
  schema owns; the validation's holds everything a validation makes. Nothing
  allocates outside them, a policy included: it allocates only through the
  allocator it is given. Both are `std::pmr::polymorphic_allocator<std::byte>`
  by default; any other allocator gets an adapter
  (`detail/memory/resource.hpp`). `is_valid` allocates nothing on the common
  path: its stacks start inline (`detail/memory/inline_stack.hpp`).
- **Deep input.** json_schema never recurses on a JSON value: every walk over
  a schema, an instance, an IR or an output is iterative, on an explicit
  stack. A value is copied only by an iterative copy, never by Boost.JSON's
  copy constructor, and a value json_schema owns lives where its destruction
  does not recurse. Parsing bounds Boost.JSON's recursive parser with
  `parse_options::max_depth`, at a depth measured safe on every target, so
  text deeper than that is an error, a value.
- **No exceptions in a header.** The headers compile without exceptions,
  which the lint checks by compiling every public header, and every test, a
  second time without them. A header raises only through
  `boost::throw_exception`, and an API that throws sits behind
  `#ifndef WEBCPP_JSON_SCHEMA_NO_EXCEPTIONS`.
- **`config.hpp` comes first.** Every public header includes
  `<webcpp/json_schema/config.hpp>` first, which defines
  `WEBCPP_JSON_SCHEMA_NO_EXCEPTIONS` without exceptions or when a developer
  does.
- **Targets.** Every test declares native, wasip2, wasip3 and emscripten,
  unless its Jamfile says why not.
- **The benchmark's machine.** The benchmark is run on demand on the owner's
  machine, never in CI, whose machines are not stable enough to time. The
  owner commits its results; the page's table and chart are generated from
  them, never typed by hand.
