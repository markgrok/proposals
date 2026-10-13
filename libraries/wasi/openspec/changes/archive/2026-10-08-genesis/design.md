## Context

webcpp::wasi began as the HTTP helper of an earlier private project: a header that exported the
`wasi:http` handler for wasip2 and wasip3, with a specification of eight invariants. It was moved
into webcpp on 2026-10-08 as `webcpp::wasi::http` (stage 3 of the superproject's plan), renamed
(namespace, macros, headers, the world's rename), given a Boost-style `build.jam`, tests that
serve each component with wasmtime, a page and a CI of its own, and published as 0.1.0. Its
repository's history is 30 commits, all as `R`, from `640d6ae` (2026-10-08) to `6e3c529`
(2026-10-12). The b2 modules it stands on (`webcpp.wit-bindings`, `webcpp.serve` and the rest)
were written in the superproject's stage 3 and belong to the organization; this library uses
them.

This document records, retroactively, the decisions 0.1.0 is built on.

## Goals / Non-Goals

**Goals:**

- Let a C++20 program that is a WebAssembly component answer HTTP by writing one function, on
  wasip2 and on wasip3, with one header and one pair of macros.
- Send a response exactly as the main wrote it, or refuse it as an internal error: never send an
  altered response, never read freed memory, never lose a byte of the body.
- Impose nothing: no exceptions, no RTTI, no Boost in the headers, no name of another webcpp
  library; a header that includes only the standard library and the bindings.
- Prove every guarantee with a test that fails when it breaks, served by a real host.

**Non-Goals:**

- Request headers, a request body, response headers other than `content-type`, trailers,
  streaming a body, a caller-supplied allocator: the handler hands the main only the method and
  the target ("Limits" in the page).
- Routing, URL parsing, JSON: a program uses Boost.URL, Boost.JSON and the like.
- Other worlds than the HTTP handler; the library's header-only target serves a program whose
  world exports more, but does not generate its bindings.
- Catching an exception that escapes the main: the host answers.
- Supporting a host that is not `wasi:http` 0.2.12 or 0.3.0.

## Decisions

### D1. A library of webcpp's own, with one handler for two versions

wasi ports nothing from outside. The handler of wasip2 (an `outparam`, `blocking-write-and-flush`
in chunks, `outgoing-body.finish`) and the handler of wasip3 (a returned response, a stream of
bytes, a trailers future, an async-lifted export) are two branches of one header, chosen by
`WEBCPP_WASI_HTTP_P2` or `WEBCPP_WASI_HTTP_P3`; the program's main is the same on both.
Alternative rejected: one header per version, which a program would have to choose between.

### D2. The main is a function between two macros

`WEBCPP_WASI_HTTP_MAIN_BEGIN(method, target)` expands `WEBCPP_WASI_HTTP_EXPORTS()` (the
`extern "C"` handler the world exports, whose name wit-bindgen fixes) and the head of the
definition of `webcpp::wasi::http::http_main`; `WEBCPP_WASI_HTTP_MAIN_END()` is `}`. The handler
is therefore defined by the program, once, and the header defines only inline functions and
macros, so every translation unit may include it. The cost: the main's body is inside a function
of `webcpp::wasi::http`, so it names its own code with a full namespace or an alias, and
clang-format needs `MacroBlockBegin`/`MacroBlockEnd` to indent it. Alternative rejected: a
function the program registers, which needs a global.

### D3. The world is renamed, and the bindings are generated

The header includes `webcpp_wasi_http.h`; the build generates the world's C bindings with
`--rename-world webcpp_wasi_http`, so every generated name (`webcpp_wasi_http_string_t`,
`WEBCPP_WASI_HTTP_WAITABLE_STATUS_BLOCKED`, ...) is the same whatever the program's world is
called. The bindings are written into the build directory, never committed, never edited. The
standard worlds `wit/http-p2.wit` and `wit/http-p3.wit` export the handler and nothing else; a
program with a world of its own links `/webcpp/wasi//wasi` and generates its own bindings.

### D4. A response is refused or sent as written

A status outside 100 to 599 is not sent: `wasi:http`'s `set-status-code` refuses some and leaves
the response at 200, and the 16 bits of the status make 65736 read as 200, so a main's bug would
reach the client as a success. A content type the host refuses as a field value (a CR, an LF, a
NUL) is not dropped silently. Both answer an internal error, before anything is committed, on
both versions; wasmtime 47 sends it as its own `500 Internal Server Error`. Alternative
rejected: clamp the status or drop the header, which alter the response.

### D5. The request outlives the response

The handler frees the method, the target and the request only after the response is written, so
a `content_type` that views the method or the target is still valid when it is sent. A first
version freed them earlier and the host refused the overwritten content type; the test
`http_content_type_of_target` fails every time on that handler. `content_type` is a
`std::string_view` because it is nearly always a literal and a `std::string` of 16 characters
allocates on wasm32 (libc++ keeps 10); `body` is a `std::string`, a copy.

### D6. wasip3 never resumes in its callback

The handler is lifted asynchronously, waits for each blocked write inside the task (a waitable
set, `waitable-set.wait`), and always ends with `EXIT`: context slot 0 belongs to wasi-libc on
wasip3 and using it trapped (measured). The `_callback` export exists because the glue calls
it, and returns `EXIT`.

### D7. Failures after the commit are reported the way each version defines

A failure found before the response is committed answers an internal error. After it, a body not
written whole is dropped without `finish` on wasip2 (which the host reports as a failed
response), and on wasip3 the trailers future is resolved with an internal error. No test can
make a write fail on demand: this is held by review (guarantee 6).

### D8. The handler catches nothing

An exception that escapes the main traps the instance, and wasmtime 47 prints the trap and
answers its own HTML 500 and keeps serving. This is the owner's decision of 2026-10-08. wasi
itself throws, tries and catches nothing, needs neither exceptions nor RTTI and imposes neither;
the superproject builds every program with exceptions on wasip2 and wasip3 (`-fwasm-exceptions
-mllvm -wasm-use-legacy-eh=false`, `-lunwind`; wasip2 was moved from `-fno-exceptions` to
exceptions in `fe9ea92`).

### D9. A configuration header that includes nothing

`config.hpp` defines `WEBCPP_WASI_NO_EXCEPTIONS` when `WEBCPP_NO_EXCEPTIONS` or
`BOOST_NO_EXCEPTIONS` is defined or the compiler has no exceptions (`__cpp_exceptions`,
`_CPPUNWIND`), and keeps a developer's own definition. It reads those macros itself because wasi
includes neither Boost nor `<webcpp/config.hpp>` ("until its redesign"). The macro changes
nothing in wasi, which raises no exception; it exists so a program can set it for every webcpp
library alike. An exception wasi raised would go through `boost::throw_exception`.

### D10. Served tests in an own lane

Each guarantee is held by a component that `wasmtime serve` runs: `webcpp.serve` builds it for
wasip2 and wasip3, sends each line `<METHOD> <target>` of `<stem>.requests`, and compares the
transcript (each request as the curl command that sends it, then status line, sorted lower-case
headers but `date` and `transfer-encoding`, an empty line and the body) with `<stem>.expected`.
Served programs run in the lane `http` (`webcpp.lane http : ... : wasip2 wasip3`), which leaves
them out of the ordinary lanes, and CI runs it as the own lane `Own lane (wasi, http, wasip2)` and
likewise wasip3 (owner decision C11, 2026-10-08, since stage 4 needs the same for a browser lane).
Compile-only guards are `webcpp.compile-diagnostic` tests that pass only on the guard's own error.

### D11. The README's example and the by-hand build are tests

The README's `main.cpp` and answers are copies of `example/http_hello.cpp` (region `main`) and
`example/http_hello.expected`, compared by doc-check; its world and commands are copies of
`example/by_hand/*/wit/world.wit` and of the regions `p2` and `p3` of `example/by_hand/build.sh`,
which the test `by_hand` (`webcpp.serve-script`) runs on both targets and whose component it
serves against `http_hello`'s transcript. A page that shows a command that stopped working fails
the lane.

## Risks / Trade-offs

- [wasmtime changes how it answers an internal error] -> the expected files of `http_bad_status`
  and `http_bad_content_type` hold wasmtime 47's HTML 500 verbatim and fail on a change; they are
  regenerated knowingly.
- [A host takes less per write on wasip3] -> wasmtime 47 takes the whole body, even 1 MiB, in one
  write, so the loop's second turn is never exercised; `http_long_body` holds that the body
  arrives whole, not that the loop goes on (guarantee 4 says so).
- [wasip3 is young] -> the handler uses wit-bindgen 0.62.0 and WASI 0.3.0 as measured; a toolchain
  update may move names the glue calls.
- [Failures after the commit are untested] -> guarantee 6 is held by review alone.
- [The doc cites configuration of the superproject by tag] -> doc-check accepts only a tagged
  region of a tracked file, so a region renamed there fails this page's build.

## Migration Plan

None: a new library in webcpp. It moved from the earlier private project with its names changed
(`webcpp::wasi::http`, `<webcpp/wasi/http/...>`, `WEBCPP_WASI_HTTP_*`,
`--rename-world webcpp_wasi_http`).

## Open Questions

None for 0.1.0. What the page's Limits chapter names as missing (request headers and body, other
response headers, an allocator) is not planned.
