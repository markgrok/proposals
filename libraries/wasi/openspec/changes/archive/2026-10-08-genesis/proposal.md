## Why

A C++ program built as a WebAssembly component that answers HTTP has to export the handler its
host calls: `wasi:http/incoming-handler` in WASI 0.2 (wasip2) or `wasi:http/handler` in WASI 0.3
(wasip3). The handler's signature and every type it reads and writes are the C functions and
structs that wit-bindgen generates from the WIT, they differ between the two versions, and
writing one correctly means reading the request, freeing it at the right moment, sending the
status and the headers, and writing every byte of the body through a stream that takes at most
so much per call. C++ had no small library that does it once for both versions, so that a
program writes one function and a host such as `wasmtime serve` runs it. webcpp needs one for
its own components, and for the libraries it builds for WASI.

## What Changes

- **New library `webcpp::wasi`**, webcpp's own (not a port), header-only C++20. Its first part,
  `webcpp::wasi::http`, is the HTTP handler a component exports: a program writes its main, a
  function of a request's method and target returning a `response` (a status, a content type and
  a body), between the macros `WEBCPP_WASI_HTTP_MAIN_BEGIN` and `WEBCPP_WASI_HTTP_MAIN_END`; wasi
  exports the handler the program's world needs, reads the request, calls the main and writes
  the answer.
- One header, `<webcpp/wasi/http/entrypoint.hpp>`, serves wasip2 and wasip3: the build defines
  exactly one of `WEBCPP_WASI_HTTP_P2` and `WEBCPP_WASI_HTTP_P3`, and generates the world's C
  bindings with the world renamed `webcpp_wasi_http`. `<webcpp/wasi/http/response.hpp>` includes
  nothing of the component ABI and compiles natively too; `<webcpp/wasi/config.hpp>` defines
  `WEBCPP_WASI_NO_EXCEPTIONS`.
- A response that cannot be sent as the main wrote it (a status outside 100 to 599, a content
  type the host refuses) is refused as an internal error, never altered; the request is freed
  only once the response is written; the body is written whole, however long.
- The build: `build.jam` declares `/webcpp/wasi//wasi` (the headers) and `/webcpp/wasi//http`
  (the headers, the bindings of the standard world of the program's target, `wit/http-p2.wit` or
  `wit/http-p3.wit`, and the macro of that target).
- Proven by a served test per guarantee (`wasmtime serve` runs the component, a transcript of
  curl commands and answers is compared), on wasip2 and wasip3, by refusals that compile only on
  the guard's own error, and by the complete example, also built by hand with wasi-sdk.
- The page teaches the library concept by concept, states nine numbered guarantees each with the
  test that holds it, and carries a chapter "Building for WASI with Boost" (toolsets,
  exceptions, Boost.JSON in a command and in a component).

## Capabilities

### New Capabilities

- `wasi`: webcpp::wasi 0.1.0, the HTTP handler a WebAssembly component exports, for C++20 on
  wasip2 and wasip3.

### Modified Capabilities

None.

## Impact

- **Kind:** a library of webcpp's own (`"port-of": null`), written first as part of an earlier
  private project and moved into webcpp on 2026-10-08.
- **Implementation:** https://github.com/webcpporg/wasi at `6e3c529`, the submodule `libs/wasi`
  of the superproject; `reference/source/` is that tree.
- **Dependencies:** none in the headers but the standard library and the bindings wit-bindgen
  writes (Boost's headers are in the target's usage requirements as every webcpp library's are,
  and the tests and examples use Boost.Core, Boost.Config and Boost.JSON). The build uses the
  superproject's b2 modules (`webcpp.wit-bindings`, `webcpp.serve`, `webcpp.serve-script`,
  `webcpp.lane`, `webcpp.headers-alone`, `webcpp.compile-diagnostic`, `webcpp.doc`,
  `webcpp.reference`) and, for a WASI build, wasi-sdk 34, wit-bindgen 0.62.0, the `wasi:http` WIT
  of WASI 0.2.12 and 0.3.0, and wasmtime 47.0.3.
- **Status:** implemented and published in release 0.1.0 (`reference/STATUS.md`).
