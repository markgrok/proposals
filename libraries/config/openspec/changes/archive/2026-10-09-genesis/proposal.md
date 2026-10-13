## Why

Every webcpp library needs to know the same three things before it compiles a line of its own:
which target the program is built for (native, wasm32-wasip2, wasm32-wasip3 or Emscripten),
whether webcpp has exceptions, and which version of webcpp it is part of. Without one place that
answers them, each library would read the compiler's macros its own way, name its target
differently, treat "no exceptions" differently, and a program could not turn exceptions off for
the whole collection with one switch. Boost solved this with Boost.Config and `<boost/version.hpp>`;
webcpp needs its counterpart, built on them, and needs it to be the first thing every library
includes.

## What Changes

- **New library `webcpp::config`**, webcpp's own (not a port), header-only and macros only,
  declaring no namespace, no type and no function:
  - `<webcpp/config.hpp>` defines exactly one target macro, from the compiler's own macros
    (`WEBCPP_TARGET_WASIP2`, `WEBCPP_TARGET_WASIP3`, `WEBCPP_TARGET_EMSCRIPTEN` or
    `WEBCPP_TARGET_NATIVE`), stops any other WebAssembly build with `#error "unsupported
    target ..."`, includes `<boost/config.hpp>` and `<webcpp/version.hpp>`, and defines
    `WEBCPP_HAS_EXCEPTIONS` unless `BOOST_NO_EXCEPTIONS` or the developer's `WEBCPP_NO_EXCEPTIONS`
    is defined.
  - `<webcpp/version.hpp>` defines `WEBCPP_VERSION` (`100`, version 0.1.0, numbered as
    `BOOST_VERSION` is) and `WEBCPP_LIB_VERSION` (`"0_1"`, written as `BOOST_LIB_VERSION` is).
  - A target macro or `WEBCPP_HAS_EXCEPTIONS` defined before the header stops the build with one
    message, and a library builds its own `WEBCPP_<LIBRARY>_NO_EXCEPTIONS` on them in two lines.
- The library's tests, all compile-time (`#error`, `static_assert` and clang `-verify`), its
  example, its documentation page with a reference of its macros, its changelog, and its CI
  through the superproject's reusable workflow.
- Outside the library, in the superproject, and therefore not requirements of this capability:
  the changelog rule of `AGENTS.md`, the release notes, and the release tooling
  (`check-release`, `whats-new`, `dist`) that the approved specification and plan also cover;
  `reference/STATUS.md` says where each stands.

## Capabilities

### New Capabilities

- `config`: webcpp::config 0.1.0, webcpp's configuration (target, exceptions, version) for C++20.

### Modified Capabilities

None.

## Impact

- **Kind:** a library of webcpp's own (`"port-of": null`), the counterpart of Boost.Config.
- **Implementation:** https://github.com/webcpporg/config at `2fad162`, the submodule
  `libs/config` of the superproject; `reference/source/` is that tree.
- **Dependencies:** Boost's headers only (`/boost//headers`, for `<boost/config.hpp>`); nothing
  to build or link. Tests need clang for the diagnostic tests; the wasm lanes need wasi-sdk and
  wasmtime, the Emscripten lane Emscripten and node.
- **Status:** implemented and published, version 0.1.0 (`WEBCPP_VERSION 100`); the decision was
  taken on 2026-10-09 (`reference/STATUS.md`).
- **Other proposals:** every other library's proposal depends on this one: each library's
  `config.hpp` includes `<webcpp/config.hpp>` first and defines its
  `WEBCPP_<LIBRARY>_NO_EXCEPTIONS` as this library's example shows.
