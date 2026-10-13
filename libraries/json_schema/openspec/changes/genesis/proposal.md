## Why

A web program in C++ validates JSON against a schema at every boundary it does not control. C++ has
no validator that covers every JSON Schema dialect, runs on WebAssembly (WASI and Emscripten)
as well as natively, works without exceptions, allocates only through the program's allocators,
and has a licence a library can ship under: the best engine, Blaze, is AGPL. webcpp needs one of
its own, and webcpp::zod, later, is built on its IR, loader, emitter and validator.

The library was designed, specified and planned (2026-10-08 to 2026-10-09), then implemented
through nine of its 58 tasks before the owner shelved it with SRELL and webcpp::regex
(2026-10-12). Since then webcpp::lexical carries an ECMA-262 translator over Boost.Regex that
passes every regex-dependent test of the official suite on every target, which removes the
reason the library depended on SRELL. This change revives json_schema as an open proposal, so
complete that an implementer resumes exactly where the work stopped.

## What Changes

- **New library `webcpp::json_schema`**, webcpp's own (not a port), header-only C++20: a JSON
  Schema validator for drafts 2020-12, 2019-09, 7, 6 and 4, and OpenAPI 3.0's Schema Object,
  on native, wasip2, wasip3 and Emscripten, proven by the official JSON-Schema-Test-Suite.
- A schema is loaded into a dialect-neutral IR, compiled once into an immutable flat plan, and
  validated by a small iterative VM in two modes: `is_valid` (a boolean, short-circuiting,
  allocation-free on the common path) and `validate` (an outcome with the four output formats
  of 2020-12, a message hook, default English messages, an optional Boost.Locale header).
- References (`$ref`, `$dynamicRef`, `$recursiveRef`, anchors, embedded resources, a local
  registry, a user resolver, bundled meta-schemas), annotations and `unevaluated*`, exact
  numbers (`decimal64_t` by default, `multipleOf` without floating point), formats through
  webcpp::lexical, content assertion on request, user vocabularies, and an emitter that writes
  the IR back in any dialect.
- **Regular expressions change engine:** `pattern` and `patternProperties` run on Boost.Regex
  1.92, header-only, through webcpp::lexical's ECMA-262 translator, behind json_schema's regex
  policy. The done task over webcpp::regex (E1.4) is re-specified and marked to redo; SRELL and
  webcpp::regex leave the library's dependencies.
- **The snapshot is brought up to the superproject's current rules first:** `config.hpp`
  includes `<webcpp/config.hpp>`, `build.jam` adds `<library>/webcpp/config//config`, the
  configuration test is built three times, the changelog file exists.
- Errors as values everywhere (`result<T>` with a `diagnostic` naming the place); a throwing
  form only when exceptions are enabled.

## Capabilities

### New Capabilities

- `libraries/json-schema`: webcpp::json_schema, a JSON Schema validator for C++20 over every
  published dialect from draft 4 to 2020-12 and OpenAPI 3.0, with its IR, loader, compiler,
  VM, registry, output, formats, emitter, exact numbers, allocation and exception guarantees,
  tests, benchmark and documentation.

### Modified Capabilities

None.

## Impact

- **Kind:** a library of webcpp's own (`"port-of": null`), repository `webcpporg/json_schema`
  (not created; the owner creates it), checked out as the submodule `libs/json_schema`,
  BSL-1.0, "Copyright (c) 2026 WebCpp.org".
- **Dependencies:** Boost.JSON (its parser, through `/webcpp//boost_json`); Boost.URL (the
  default URL policy, through `/webcpp//boost_url`); webcpp::lexical (the default format set,
  and the ECMA-262 translator the regex policy consumes); Boost.Regex 1.92, header-only,
  reached only through formats; webcpp::config (`<webcpp/config.hpp>`); Boost's headers
  (System, Decimal, StaticString, Container, Compat, Core); Boost.Locale only for the optional
  locale header. Test data: the JSON-Schema-Test-Suite submodule at `5b0ee16` (MIT).
- **Implementation:** a local repository, unpublished (below); `reference/source/` is its tree.
- **Status:** draft: the specification is being completed, and 9 of its 68 tasks are built.
- **Where the implementation stands:** tasks E1.1 to E1.9 are done (18 commits): the skeleton,
  exact numbers, JSON equality and copies, the regex policy (to redo), the error and event
  types with the two sinks, traits and dialects, exact JSON text, the IR, the options, and the
  2020-12 loader. E1.10, the compiler, is written and passes its test natively and on wasip2,
  uncommitted. Everything from E1.11 (the VM) on is not started.
- **The implementation repository is not published.** `reference/source/` is the library's
  tree at the last commit of its branch `e1` (`80c3b69`), as files; `reference/source-wip/` is
  the uncommitted E1.10; `reference/STATUS.md` says how to recreate the repository from them,
  build it and resume. `reference/plan.md` is the implementation plan with each task's code,
  `reference/spec.md` and `reference/design-source.md` the full specification and design.
- **Other webcpp libraries:** this change depends on the change `strategies` of lexical
  (`libraries/lexical`): lexical must publish the public matching interface its group 3
  specifies (compile a pattern, search with a step budget) for task E1.4's redo; the superproject gains the suite's
  submodule (already present) and, later, the libraries' index entry.
- **Breaking changes:** none; nothing depends on json_schema yet.
