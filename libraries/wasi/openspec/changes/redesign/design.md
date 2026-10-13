## Context

wasi 0.1.0 (the change `genesis`, archived) exports the HTTP handler of a WebAssembly component
on wasip2 and wasip3. Reviewing it on 2026-10-08, the owner asked for the redesign recorded in
`reference/design.md`: wasi's purpose is to hide the `wasi:http` bindings, which are resource
handles with a different API per target, behind a function that returns a plain `response`, while
keeping the guarantees of its page; the owner confirmed that purpose. He approved the design on
2026-10-09. `reference/spike.md` measured the design with wasi-sdk 34, wasmtime 47.0.3 and
wit-bindgen 0.62.0. This document records the decisions the specification delta states; where the
spike and the design differ, the design wins (`reference/STATUS.md` lists the differences).

## Goals / Non-Goals

**Goals:**

- Each version's handler readable alone, with no `#if` inside its logic.
- The user's function testable natively, with no ABI in its type.
- The compiler chooses the version; a developer may override it.
- A program can ask what the target offers.
- A run script beside every WASI program, tested by the tests that serve them.
- Every guarantee of the page kept; every served test of 0.1.0 passing on the new layout.

**Non-Goals:**

- A request that carries headers or a body: a later extension.
- A component that exports both handlers, or one module linked against both libcs.
- A namespace for features whose implementation does not differ between versions (files, sockets,
  clocks, environment): no `p2::filesystem`.
- The macros `WEBCPP_WASI_HAS_FILESYSTEM` and `WEBCPP_WASI_HAS_PROCESS_CLOCKS` that the spike
  listed: the design keeps three.
- A `.cmd` script, until a Windows WASI lane exists.
- Changing a verdict of 0.1.0's handler (status range, content type, chunks, trailers).

## Decisions

### D1. Boost and webcpp::config

wasi depends on Boost like every webcpp library. Its `config.hpp` includes `<webcpp/config.hpp>`
(webcpp::config 0.1.0), which includes `<boost/config.hpp>`, and adds wasi's own defines, following
config's pattern for a library's exceptions macro. The claim "wasi needs no Boost" is removed; the
page's by-hand build names Boost's include path and config's.

### D2. One namespace per WASI version, the configured one inline

`webcpp::wasi::p2` and `p3`, as Boost.Process's `v1` and `v2`. Each holds only the feature
namespaces it provides, as Boost.Asio's `posix` and `windows` exist where the platform has them;
only `http` differs. What both share lives in `webcpp::wasi::detail` and appears in each `http` by
`using`; there is no public `wasi::http` outside the versions on a WASI build, which would be
ambiguous against the inline version's. Each handler is in its own header with no `#if` in its
logic; the one choice is made in `<webcpp/wasi/http.hpp>`.

### D3. Target selection by the compiler, overridden by WEBCPP_WASI_TARGET

`config.hpp` defines `WEBCPP_WASI_TARGET_P2` or `_P3` from `__wasip2__` or `__wasip3__`;
`WEBCPP_WASI_TARGET` (`p2` or `p3`) overrides it, and any other value stops with one error. The
build defines nothing. The `inline` of the configured namespace comes from these macros, in the
way Boost.Process's does. The spike measured that a p3 handler on the wasip2 libc and a p2 handler
on the wasip3 libc both serve, so the override is a real choice.

### D4. One entry point, a plain function

```cpp
#include <webcpp/wasi/http.hpp>

webcpp::wasi::http::response hello(webcpp::wasi::http::request const& request) {
    return {.status = 200, .content_type = "text/plain", .body = "hello"};
}

WEBCPP_WASI_HTTP_ENTRY_POINT(hello)   // at global scope, once per program
```

`WEBCPP_WASI_P2_HTTP_ENTRY_POINT(fn)` defines p2's export, `WEBCPP_WASI_P3_HTTP_ENTRY_POINT(fn)`
p3's export and callback, and `WEBCPP_WASI_HTTP_ENTRY_POINT(fn)` forwards to the configured one.
Macro names mirror the namespace path, as Boost's do, since the preprocessor has no namespaces.
The function has no ABI, so a native test calls it. `request` carries the method and the target as
views valid during the call. This replaces the MAIN_BEGIN/END pair, `WEBCPP_WASI_HTTP_EXPORTS` and
`http_main`: a breaking change before webcpp's first release.

### D5. One source, two components

Measured: one component exporting both handlers builds and validates, but wasmtime always answers
through the p3 export, a host without p3 cannot load it, Spin 4.2.2 refuses two handlers, it grows
24%, and one module linked against both libcs fails in wasm-ld. A program is built once per
target, one export per component, both macros calling the same function.

### D6. Capability macros

`WEBCPP_WASI_HAS_SOCKETS` (`__wasip2__ || __wasip3__`), `WEBCPP_WASI_HAS_THREADS`
(`__wasi_cooperative_threads__`, never `__STDCPP_THREADS__`: wasip3 predefines it and cannot start
a thread) and `WEBCPP_WASI_HAS_EXCEPTIONS` (`__wasm_exception_handling__`, next to
`WEBCPP_WASI_NO_EXCEPTIONS`). The macros say what compiles; what runs is the host's grant, and the
page tables it (`--dir`, `-S inherit-network`, `allow-ip-name-lookup`, `--env`).

### D7. A run script from the build

b2 writes `<program>.sh` beside each `.wasm`: a POSIX `sh` script that finds wasmtime (`$WASMTIME`,
else `PATH`), passes the flags of the artifact's kind, target and exception setting (the table of
`reference/spike.md` section 4.1), adds `$WASMTIME_FLAGS` and forwards `"$@"` (a served component
takes an optional address, `127.0.0.1:8080` by default). It is a rule of the build system, so
every library's WASI programs get one, and `serve.py` and `run_example.py` run the programs through
their scripts, so the CI tests the scripts. The prototype's three b2 facts (`on` is a jam keyword,
an empty list vanishes a string, `$(` is b2's) are tasks 3.1 and 3.2.

### D8. Derived by this proposal, to be confirmed (O1, O2)

The design fixes the namespaces and the macros, and leaves the shared headers and the native case
to the implementation. This proposal states them: the ABI-free `response` and `request` live in
`<webcpp/wasi/detail/response.hpp>` and `<webcpp/wasi/detail/request.hpp>`, and on a build with no
WASI version `<webcpp/wasi/http.hpp>` declares `webcpp::wasi::http` with the shared names alone and
defines the entry-point macro as nothing, so that the user's function compiles and runs natively.

## Risks / Trade-offs

- [A breaking change for every program] -> made before webcpp's first release, with a mechanical
  migration listed in the changelog; wasi 0.1.0 has no users outside webcpp.
- [`using` of shared names under an inline namespace can make lookups ambiguous] ->
  `target_namespace` pins that `webcpp::wasi::http::request` is unambiguous on both targets and
  names the shared type.
- [The shared internals that touch the bindings (`method_name`, `headers_of`) need a header that
  exists only on WASI] -> they are included by the version headers alone; the ABI-free headers
  stay free of them, which `response`'s native test holds.
- [wasmtime flags differ by version: `-W exceptions=y` on 37 to 46, WASI 0.3 needs 46 or later] ->
  the script states the explicit flags, which run on the oldest wasmtime that can run the
  artifact; the minimal ones are the same on 47.
- [A p3 instance and its globals outlive one request: wasmtime reuses it 128 times by default] ->
  the handler keeps no state, as 0.1.0 did.
- [The run scripts are a rule of tools shared by all libraries] -> O4; the scripts are written for
  every WASI program, so a regression there stops every library's lane.

## Migration Plan

For a program of 0.1.0, mechanical, listed in the changelog:

1. `#include <webcpp/wasi/http/entrypoint.hpp>` becomes `#include <webcpp/wasi/http.hpp>`.
2. `WEBCPP_WASI_HTTP_MAIN_BEGIN(method, target) ... WEBCPP_WASI_HTTP_MAIN_END()` becomes
   `webcpp::wasi::http::response fn(webcpp::wasi::http::request const& request) { ... }` with
   `request.method` and `request.target`, then `WEBCPP_WASI_HTTP_ENTRY_POINT(fn)` at global scope.
3. A build removes `<define>WEBCPP_WASI_HTTP_P2` and `_P3`; the compiler chooses the version.
4. A by-hand build drops `-DWEBCPP_WASI_HTTP_P2` or `_P3` and adds the include directories of
   webcpp::config and Boost.
5. `.clang-format` no longer names the macros of the main.

## Open Questions

- **O1.** What does `<webcpp/wasi/http.hpp>` do on a build with no WASI version (native,
  Emscripten)? Recommended and specified: it declares `webcpp::wasi::http::response` and `request`
  and defines `WEBCPP_WASI_HTTP_ENTRY_POINT` as nothing, so the function compiles and a native test
  calls it (D8).
- **O2.** Are the shared types public headers under `detail/`
  (`<webcpp/wasi/detail/response.hpp>`)? Recommended: yes, since they are the ABI-free headers; the
  alternative is `<webcpp/wasi/http/response.hpp>`, which suggests a public `wasi::http` outside the
  versions that the design refuses.
- **O3.** Host grants as a feature of the build (`<wasi-grant>filesystem`, `network`,
  `environment`) and `install` listing both the `.wasm` and its `.sh`, as `reference/spike.md`
  section 4.3 outlines? Recommended: later; the design asks for the flags of the kind and target,
  and wasi's tests need no grant. `WASMTIME_FLAGS` serves meanwhile.
- **O4.** Where is the run-script rule specified? It is code of the superproject's tools, shared by
  every library; this change specifies its behaviour in wasi's specification because the design
  places it in the redesign and wasi's tests exercise it. Recommended: the organization's
  specification records the rule, and wasi's refers to it.
