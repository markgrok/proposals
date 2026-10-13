## Context

webcpp::config was specified and approved by the owner on 2026-10-09 (`reference/spec.md`), and
planned task by task the same day (`reference/plan.md`); its first commit is of that date and its
last of 2026-10-12. It is webcpp's counterpart of Boost.Config: one header-only library that every
webcpp library includes first, holding what is common to all of them. The approved specification
and plan also cover work that lives in the superproject rather than in this library (the
changelog rule in `AGENTS.md`, the release notes, the release tooling and the move of the other
libraries onto `<webcpp/config.hpp>`); this document records, retroactively, the decisions the
library itself, `reference/source/`, is built on.

## Goals / Non-Goals

**Goals:**

- Give every webcpp library, and every program using them, one answer to three questions: which
  target is this built for, does webcpp have exceptions, which version of webcpp is this.
- Compute the answers from the compiler's own macros, so that a program compiled by hand gets
  what b2 would give it.
- Stop a build for a WebAssembly target webcpp does not support, with a message that names the
  supported ones, rather than read it as native.
- Let a developer turn exceptions off for the whole collection with one macro.
- Cost nothing: macros only, no namespace, nothing to build or link beyond Boost's headers, and
  the same on native, wasip2, wasip3 and Emscripten.

**Non-Goals:**

- Anything Boost.Config already answers (compilers, standard library, `BOOST_NO_*`): config
  includes `<boost/config.hpp>` and adds only what is webcpp's.
- Targets beyond the four (wasm32-wasip1, wasm32-unknown-unknown, other WebAssembly): an
  `#error` names them; a new target will get a macro of its own.
- Turning off Boost's own exceptions: `WEBCPP_NO_EXCEPTIONS` is webcpp's alone.
- The changelog rule, the release notes and the release tooling: the superproject's, specified by
  its own proposals; config holds only its own `doc/changelog.adoc`.

## Decisions

### D1. A library of webcpp's own, on Boost.Config

config ports nothing and is a library like the others: a repository of its own, the submodule
`libs/config`, `build.jam`, `meta/libraries.json`, a page, a CI file. `<webcpp/config.hpp>`
includes `<boost/config.hpp>`, so a program that includes it has Boost.Config's macros too.
Alternative rejected: each library reading `__wasip2__` and `BOOST_NO_EXCEPTIONS` itself, which
repeats the logic nine times and lets the answers drift apart.

### D2. Macros only, in the collection's own prefix

config declares no namespace, no type and no function, as Boost.Config does. Its macros are
`WEBCPP_*` (the collection's), where every other library's are `WEBCPP_<LIBRARY>_*`; so no
library is named `target`, `has`, `no`, `lib`, `version` or `test`, alone or followed by an
underscore and more, since its macros would meet config's or the tests' `WEBCPP_TEST_*`. Include
guards are `WEBCPP_CONFIG_HPP` and `WEBCPP_VERSION_HPP`. The superproject's reference tooling is
taught that this one library's prefix is `WEBCPP_` and that it declares no namespace.

### D3. The target comes from the compiler, before Boost.Config

Exactly one of `WEBCPP_TARGET_WASIP2` (`__wasip2__`), `WEBCPP_TARGET_WASIP3` (`__wasip3__`),
`WEBCPP_TARGET_EMSCRIPTEN` (`__EMSCRIPTEN__`) and `WEBCPP_TARGET_NATIVE` (none of the three, and
not `__wasm__`) is defined, tested in that order. The macros come from the compiler alone, never
from the build system, so a plain `clang++` or wasi-sdk's gets what b2 would give. The block
comes before `#include <boost/config.hpp>`, because wasm32-unknown-unknown has no C++ library and
Boost.Config stops there at a header that is not there: the target message must come first. Measured
with wasi-sdk 34 on 2026-10-09: `--target=wasm32-wasip2` defines `__wasi__`, `__wasm__` and
`__wasip2__`, wasm32-wasip3 likewise with `__wasip3__`, wasm32-wasip1 defines `__wasip1__` and
compiles `<boost/config.hpp>`; Emscripten 6.0.11 defines `__EMSCRIPTEN__`.

### D4. An unsupported WebAssembly build is an error

Any `__wasm__` build that is not one of the three is `#error "unsupported target: webcpp builds
for wasm32-wasip2, wasm32-wasip3, Emscripten and natively"` (owner, 2026-10-09, the amendment on
unknown wasm targets). Alternative rejected: reading it as native, which would compile code for
a system the program does not run on.

### D5. Exceptions: one developer switch, Boost's left alone

`WEBCPP_HAS_EXCEPTIONS` is defined unless `BOOST_NO_EXCEPTIONS` (the compiler has none) or
`WEBCPP_NO_EXCEPTIONS` (the developer's, on the command line or in a b2 requirement) is defined.
config never defines `WEBCPP_NO_EXCEPTIONS`, and the developer's does not define
`BOOST_NO_EXCEPTIONS`: Boost's code keeps its exceptions in a build that has them (owner,
2026-10-09, option (a)). Each library's own macro follows:
`#if !defined(WEBCPP_HAS_EXCEPTIONS) && !defined(WEBCPP_<LIBRARY>_NO_EXCEPTIONS)` then
`#define WEBCPP_<LIBRARY>_NO_EXCEPTIONS`, so a developer may also define one library's alone.
Alternatives rejected: a per-library switch only (a program would have to know which libraries
throw), and `WEBCPP_NO_EXCEPTIONS` also defining `BOOST_NO_EXCEPTIONS` (Boost's headers would
change behaviour in a build whose compiler supports exceptions).

### D6. A macro defined before the header stops the build at one message

A target macro or `WEBCPP_HAS_EXCEPTIONS` defined before `<webcpp/config.hpp>` is an `#error
"WEBCPP_TARGET_* and WEBCPP_HAS_EXCEPTIONS are defined by <webcpp/config.hpp> alone"`, and the
rest of the header is the `#else`, so that no macro is defined a second time and the message is
the only one. The tests pin both this and the order of the target block.

### D7. The version: numbered and written as Boost's, in one place

`WEBCPP_VERSION` is `major * 100000 + minor * 100 + patch` (`100` is 0.1.0, `100203` is 1.2.3)
and `WEBCPP_LIB_VERSION` is `"<major>_<minor>"` with `"_<patch>"` only when the patch is not 0
(`"0_1"`, `"1_2_3"`), as `BOOST_VERSION` and `BOOST_LIB_VERSION` are. The version is a three-number
one (tag `webcpp-0.1.0`), as Boost's tags are. `<webcpp/version.hpp>` is the one place it is
written, a macro so that `#if` compares it; the superproject's build and release tooling read the
`#define` line, and it moves only by the release procedure, right after a tag. It lies outside
`webcpp/config/`, as `<boost/version.hpp>` does, and so has a test of its own.

### D8. Tests that run when compiled

What config does, it does when a program is compiled, so every test is compiled and never run:
`webcpp.headers-alone`, `webcpp.compile` (a translation unit of `#error` and `static_assert`) and
`webcpp.compile-diagnostic` (clang `-verify` against the message a source states). The
diagnostic tests are clang's alone, so GCC, MSVC and Emscripten skip them. The exceptions test is
built three times, to see the macro in a build with exceptions, with `exception-handling=off` and
with `WEBCPP_NO_EXCEPTIONS` defined. Every test and the example are declared for the four targets
with `webcpp.targets native wasip2 wasip3 emscripten`.

### D9. A page and a reference of eight macros

The page (`doc/`) has an introduction, targets, exceptions, version and testing sections, a
reference, and a changelog appendix. The reference is generated by MrDocs from `/** */` comments;
because MrDocs parses natively and with exceptions, an `#ifdef __MRDOCS__` block defines and
undefines each macro it would otherwise not see (`WEBCPP_TARGET_WASIP2`, `WEBCPP_TARGET_WASIP3`,
`WEBCPP_TARGET_EMSCRIPTEN`, `WEBCPP_NO_EXCEPTIONS`). The page's test counts are attributes the
superproject's tooling computes from `test/Jamfile`, never typed.

### D10. Scope of this library against the superproject's release machinery

The approved specification names, beside the headers, a changelog rule for every library
(`doc/changelog.adoc`, "Unreleased" on top, one line per user-visible change) and release tooling.
Those are the superproject's and live in `AGENTS.md`, `tools/` and `doc/releases/`; they are
outside this capability. What config itself has is its `doc/changelog.adoc` with its first line,
"Added the library, with webcpp's target, exceptions and version macros.", and the statement that
`version.hpp` is moved only by the release procedure.

## Risks / Trade-offs

- [A new target is needed] → a new target gets its own macro and its own test; until then the
  `#error` names the four.
- [`WEBCPP_NO_EXCEPTIONS` does not affect Boost] → by design (D5); documented on the page and
  pinned by the `exceptions_developer_off` test.
- [The diagnostic tests run on clang alone] → GCC and MSVC lanes skip them (shown as grey cells in
  the test matrix); the macros they pin are the same source.
- [The macro names reserve library names] → the superproject's lint refuses a library named
  `target`, `has`, `no`, `lib`, `version` or `test` (D2).
- [MSVC not yet seen green] → the superproject's MSVC lanes failed to load a Jamfile for an
  unrelated reason before building any library (`reference/STATUS.md`).

## Migration Plan

None: a new library. Each other library moves onto it in a change of its own, by including
`<webcpp/config.hpp>` first in its `config.hpp` and keeping its own `WEBCPP_<LIBRARY>_NO_EXCEPTIONS`
as D5 shows.

## Open Questions

None for 0.1.0.
