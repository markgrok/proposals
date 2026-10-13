## Purpose

webcpp::wasi is a header-only C++20 library of webcpp's own for programs built as WebAssembly
components on WASI. Its first part, `webcpp::wasi::http`, is the HTTP handler a component exports
on wasip2 (WASI 0.2.12) and wasip3 (WASI 0.3.0): a program writes one function, its main, that
turns a request's method and target into a response, and wasi reads the request, calls the main
and writes the answer, sending it as written or refusing it as an internal error. It imposes no
exceptions, no RTTI and no dependency, and is proven by components that wasmtime serves. These
requirements describe 0.1.0 as built (`reference/source/`).

## ADDED Requirements

### Requirement: Library: Header-only library in its namespace
The library SHALL be header-only C++20, in namespace `webcpp::wasi::http` (the response, the main and the handler's public names), with what is not public in `webcpp::wasi::http::detail`, public headers under `<webcpp/wasi/...>` (one per responsibility) and `<webcpp/wasi.hpp>`, macros prefixed `WEBCPP_WASI_` (`WEBCPP_WASI_HTTP_` for the handler's) and include guards `WEBCPP_WASI_<PATH>_HPP` (`WEBCPP_WASI_HPP`, `WEBCPP_WASI_CONFIG_HPP`, `WEBCPP_WASI_HTTP_ENTRYPOINT_HPP`, `WEBCPP_WASI_HTTP_RESPONSE_HPP`). It is webcpp's own and ports nothing; it MUST NOT name a program or a library it was written for, and the lint's rule on the banned word holds that (guarantee 8 of the page). The files carry the notice "Copyright (c) 2026 WebCpp.org" and the Boost Software License 1.0 (`LICENSE_1_0.txt`).

#### Scenario: A program includes the whole library
- **WHEN** a component's source includes `<webcpp/wasi.hpp>` on wasip2 or wasip3 with the build's bindings and macro
- **THEN** it compiles, `webcpp::wasi::http::response` and the main's macros are declared, and `test/Jamfile`'s headers-alone test of that header passes

#### Scenario: The library stands alone
- **WHEN** the headers, the comments and the examples are read
- **THEN** they include only the standard library and the bindings wit-bindgen writes (the response and the configuration include only the standard library), and no header, comment or example names another webcpp library

### Requirement: Library: Public headers and the convenience header
The library SHALL ship these public headers: `include/webcpp/wasi/config.hpp`, `include/webcpp/wasi/http/response.hpp`, `include/webcpp/wasi/http/entrypoint.hpp` and the convenience header `include/webcpp/wasi.hpp`, which MUST include `<webcpp/wasi/config.hpp>` and `<webcpp/wasi/http/entrypoint.hpp>` and nothing else. Every public header but `config.hpp` itself includes `<webcpp/wasi/config.hpp>` first (`wasi.hpp`, `entrypoint.hpp` and `response.hpp` do); `config.hpp` includes nothing. `response.hpp` MUST include nothing of the component ABI. Each header compiles alone, as the only include of its translation unit.

#### Scenario: Each header compiles alone
- **WHEN** `test/Jamfile` compiles `wasi.hpp` and `wasi/http/entrypoint.hpp` on wasip2 and wasip3 with `/webcpp/wasi//http`, and `wasi/config.hpp` and `wasi/http/response.hpp` natively, on wasip2 and on wasip3, each as the only include of its translation unit
- **THEN** every one of the compiles passes

#### Scenario: The convenience header brings the entry point
- **WHEN** `test/http_two_units_answer.cpp` includes `<webcpp/wasi.hpp>` and uses `webcpp::wasi::http::response`
- **THEN** it compiles on wasip2 and wasip3

### Requirement: Library: The response
The library SHALL declare `struct response` in `<webcpp/wasi/http/response.hpp>`, namespace `webcpp::wasi::http`, an aggregate of exactly three members, in this order, each with a default so that a designated initializer names only what differs: `unsigned status = 200`, `std::string_view content_type{}` and `std::string body{}`. The members MUST be declared in that order, which is the order of an HTTP response and of the designated initializer that builds one, and the reference MUST list them in it (`sort-members: false`).

#### Scenario: A response left at its defaults
- **WHEN** `http::response answer;` is default-initialized
- **THEN** `answer.status == 200U`, `answer.content_type.empty()` and `answer.body.empty()` (`response_test`'s `defaults`)

#### Scenario: A designated initializer names one member
- **WHEN** `http::response answer{.content_type = "text/plain"};` is initialized
- **THEN** `answer.status == 200U`, `answer.content_type == "text/plain"` and `answer.body.empty()` (`response_test`'s `designated`)

### Requirement: Response: The body is a copy, the content type a view
The `body` of a response SHALL be a `std::string` that owns its text, and the `content_type` a `std::string_view` that does not. The body is sent whole, however long, after the main has returned, so it MUST NOT point into a string of the main's own; the content type is read after the main has returned too, so it views what outlives the main: a string literal such as `"application/json"`, or the method or the target the main was given, never a string of the main's own. A view is used because a content type is nearly always a literal and costs no allocation, where a `std::string` of 16 characters exceeds the 10 characters libc++ keeps in the string itself on wasm32.

#### Scenario: The body is a copy
- **WHEN** `std::string text = "hello"; const http::response answer{.status = 404, .body = text};` is followed by `text.assign("other")`
- **THEN** `answer.status == 404U` and `answer.body == "hello"` (`response_test`'s `owns_its_body`)

#### Scenario: A content type viewing the target is sent as it reads
- **WHEN** `http_content_type_of_target`'s main answers `GET /text/plain` with `.content_type = target.substr(1)`
- **THEN** the answer is `200 OK` with `content-type: text/plain` and the body `the content type of /text/plain`, and `GET /application/json` answers `content-type: application/json`, on wasip2 and wasip3

### Requirement: Response: Compiles natively and carries no ABI
`<webcpp/wasi/http/response.hpp>` SHALL compile everywhere, natively included, because it includes only `<webcpp/wasi/config.hpp>`, `<string>` and `<string_view>`, so that the rest of a program builds responses, holds them and passes them around (as messages between actors, for example) and is tested natively, without a WASI toolchain. `<webcpp/wasi/config.hpp>` compiles everywhere too.

#### Scenario: The response test runs natively and on both targets
- **WHEN** `b2 libs/wasi/test` is run natively, and with `toolset=clang-wasip2` and `toolset=clang-wasip3` with `testing.launcher=wasmtime`
- **THEN** `response` (a `webcpp.run` test with `native wasip2 wasip3`) runs and passes in each, `response_test.cpp` ending in `boost::report_errors()`

#### Scenario: A header alone compiles natively
- **WHEN** `response.hpp` and `config.hpp` are each compiled alone for the native toolset
- **THEN** both compile

### Requirement: Entry point: One version of WASI, chosen by a macro
The entry point SHALL compile only for WASI and for one version: `<webcpp/wasi/http/entrypoint.hpp>` MUST stop with `#error "define exactly one of WEBCPP_WASI_HTTP_P2 and WEBCPP_WASI_HTTP_P3"` when `defined(WEBCPP_WASI_HTTP_P2) == defined(WEBCPP_WASI_HTTP_P3)`, that is under neither or both, before it includes `webcpp_wasi_http.h`; then it includes `"webcpp_wasi_http.h"` inside `extern "C" { }` and `<webcpp/wasi/http/response.hpp>`. `WEBCPP_WASI_HTTP_P2` selects the wasip2 branch (`wasi:http/incoming-handler`), `WEBCPP_WASI_HTTP_P3` the wasip3 branch (`wasi:http/handler`). This is guarantee 1 of the page.

#### Scenario: Neither macro is defined
- **WHEN** `neither_target_refused.cpp` includes `<webcpp/wasi/http/entrypoint.hpp>` with the bindings of the target and neither macro
- **THEN** the compile fails with exactly the guard's error, which clang's `-verify` finds as `expected-error@webcpp/wasi/http/entrypoint.hpp:* {{define exactly one of WEBCPP_WASI_HTTP_P2 and WEBCPP_WASI_HTTP_P3}}`, and with no other, on wasip2 and wasip3

#### Scenario: Both macros are defined
- **WHEN** `both_targets_refused.cpp` includes the entry point with `<define>WEBCPP_WASI_HTTP_P2` and `<define>WEBCPP_WASI_HTTP_P3`
- **THEN** the compile fails with exactly the same guard error and no other, on wasip2 and wasip3

### Requirement: Entry point: The main and its two macros
The entry point SHALL declare `[[nodiscard]] response http_main(std::string_view method, std::string_view target);` in `webcpp::wasi::http`, which a program defines once by writing its body between `WEBCPP_WASI_HTTP_MAIN_BEGIN(method, target)` and `WEBCPP_WASI_HTTP_MAIN_END()`. `WEBCPP_WASI_HTTP_MAIN_BEGIN(method, target)` MUST expand to `WEBCPP_WASI_HTTP_EXPORTS()` followed by the head `webcpp::wasi::http::response webcpp::wasi::http::http_main(std::string_view method, std::string_view target) {`, naming the two parameters as the macro's arguments; `WEBCPP_WASI_HTTP_MAIN_END()` MUST expand to `}`. The body is therefore inside a function of `webcpp::wasi::http`: it names the program's own code with its full namespace, or with a namespace alias declared at its top. The pair is written once in a program, in one source file.

#### Scenario: The complete example
- **WHEN** `example/http_hello.cpp` is served and sent `GET /v1/greeting?name=ana` and `POST /x`
- **THEN** the transcript is `HTTP/1.1 200 OK` with `content-type: text/plain` and the body `hello from /v1/greeting?name=ana`, then `HTTP/1.1 405 Method Not Allowed` with `content-type: text/plain` and the body `only GET`, byte for byte `example/http_hello.expected`

#### Scenario: The main calls a function of another translation unit
- **WHEN** `http_two_units.cpp`'s main returns `two_units::answer(method, target)`, defined in `http_two_units_answer.cpp`, and `GET /two?units=1` is sent
- **THEN** the answer is `200 OK`, `content-type: text/plain` and the body `from the other unit: GET /two?units=1`

### Requirement: Entry point: The handler's exports
`WEBCPP_WASI_HTTP_EXPORTS()` SHALL define the `extern "C"` function or functions the target's world exports, under the names wit-bindgen generates, which the component's glue `webcpp_wasi_http.c` calls: on wasip2 `extern "C" void exports_wasi_http_incoming_handler_handle(exports_wasi_http_incoming_handler_own_incoming_request_t request, exports_wasi_http_incoming_handler_own_response_outparam_t out)`, which calls `webcpp::wasi::http::detail::handle(request, out)`; on wasip3 `extern "C" webcpp_wasi_http_callback_code_t exports_wasi_http_handler_handle(exports_wasi_http_handler_own_request_t request)`, which returns `webcpp::wasi::http::detail::handle(request)`, and `exports_wasi_http_handler_handle_callback(webcpp_wasi_http_event_t*)`, which returns `WEBCPP_WASI_HTTP_CALLBACK_CODE_EXIT` and is never reached. A program that writes its main between the two macros never names the macro.

#### Scenario: A served component answers on both versions
- **WHEN** any served component of `test/` or `example/` is built for wasip2 and for wasip3 and served by `wasmtime serve`
- **THEN** the host finds the exported handler of its version and the component answers

### Requirement: Entry point: The header defines nothing a program links
The entry point SHALL define no function of its own that a program links: the `extern "C"` functions are defined by the expansion of `WEBCPP_WASI_HTTP_MAIN_BEGIN` in the program, and the header holds only inline functions, macros and the declaration of `http_main`, so that every translation unit of a program may include it and the program writes the pair of macros once. This is guarantee 2.

#### Scenario: Two translation units include the entry point
- **WHEN** `http_two_units` is built from `http_two_units.cpp` (which includes `<webcpp/wasi/http/entrypoint.hpp>` and writes the main) and `http_two_units_answer.cpp` (which includes `<webcpp/wasi.hpp>`), and `GET /two?units=1` is sent
- **THEN** the component links with no duplicate symbol and answers `200 OK`, `content-type: text/plain`, `from the other unit: GET /two?units=1`

### Requirement: Entry point: The method
The handler SHALL give the main the request's method as it travels on the wire, by `detail::method_name`: `"GET"`, `"HEAD"`, `"POST"`, `"PUT"`, `"DELETE"`, `"CONNECT"`, `"OPTIONS"`, `"TRACE"` and `"PATCH"` for the nine enumerators of `wasi:http`'s method, and, for any other, the string of the `other` case as it was sent, without copying it (`detail::view_of`).

#### Scenario: Each method reaches the main by name
- **WHEN** `http_methods` is sent `GET /a?b=c%20d`, `HEAD /a?b=c%20d`, `POST /`, `PUT /items/7`, `DELETE /items/7`, `OPTIONS *`, `TRACE /trace`, `PATCH /items/7?x=1&y=%2F` and `PURGE /cache/key`
- **THEN** each answer is `200 OK` with `content-type: text/plain; method=<METHOD>` and, but for `HEAD`, whose answer carries no body, a body `<METHOD> <target>` echoing the request, `PURGE` included as an extension method; `CONNECT` is left out of the test because the test client cannot send it to an origin server

### Requirement: Entry point: The target
The handler SHALL give the main the request target, its path and its query, as it was sent and with nothing decoded, obtained from the request's `path-with-query`, and `"/"` when the request carried none; `OPTIONS *` reaches the main as `"*"`. The handler MUST NOT parse the target: a main that needs the path and the query apart parses it with a URL library, such as Boost.URL's `parse_origin_form`.

#### Scenario: A target with a query and an escape arrives verbatim
- **WHEN** `http_methods` is sent `GET /a?b=c%20d`, `PATCH /items/7?x=1&y=%2F` and `OPTIONS *`
- **THEN** the bodies are `GET /a?b=c%20d`, `PATCH /items/7?x=1&y=%2F` and `OPTIONS *`

### Requirement: Entry point: The request outlives the response
The handler SHALL free the method, the target (`wasi_http_types_method_free`, `webcpp_wasi_http_string_free`) and the request (`wasi_http_types_incoming_request_drop_own` on wasip2, `wasi_http_types_request_drop_own` on wasip3) only after the response has been written, so that a `content_type` that views the method or the target is still valid when it is sent; the main's two parameters view the request, and the main copies what it keeps beyond the request. This is guarantee 3. The response owns its body, so no body views the request.

#### Scenario: A content type viewing the target survives
- **WHEN** `http_content_type_of_target` is sent `GET /text/plain` and `GET /application/json`
- **THEN** the answers carry `content-type: text/plain` and `content-type: application/json`, which a handler that freed the target first would fail every time, since the free overwrites its first bytes with the allocator's links

### Requirement: Entry point: A status outside 100 to 599 is refused
The handler SHALL send a response's `status` as it is when `detail::sendable` holds, that is `100 <= status && status <= 599`, and MUST otherwise send no response of the main's and answer an internal error instead (`refuse`), before anything is committed, on both versions. `wasi:http`'s `set-status-code` refuses some statuses and leaves the response at 200, and the status being 16 bits wide, 65736 would read as 200. wasmtime 47 sends the internal error as its own `500 Internal Server Error`. This is part of guarantee 9.

#### Scenario: Statuses at and beyond the range
- **WHEN** `http_bad_status`, whose main answers the status its target names, is sent `GET /0`, `GET /99`, `GET /600`, `GET /65736`, `GET /599` and `GET /200`
- **THEN** the first four answer `HTTP/1.1 500 Internal Server Error` with `content-length: 205`, `content-type: text/html; charset=UTF-8` and wasmtime's HTML page naming `500 Internal Server Error`, `GET /599` answers `HTTP/1.1 599 <none>` with the body `the status 599`, and `GET /200` answers `HTTP/1.1 200 OK` with the body `the status 200`, on wasip2 and wasip3

### Requirement: Entry point: The content-type header, and no other
The handler SHALL send a `content-type` header if and only if the response's `content_type` is not empty, with that value as a field value, and MUST send no other header (`detail::headers_of`). An empty content type sends no header, with a body or without one. This is guarantee 7.

#### Scenario: A set and an empty content type
- **WHEN** `http_no_content_type` is sent `GET /none`, `GET /empty` and `GET /plain`
- **THEN** `/none` answers `HTTP/1.1 200 OK` with no header and the body `no content type`, `/empty` answers `HTTP/1.1 200 OK` with no header and no body, and `/plain` answers `HTTP/1.1 200 OK` with exactly one header, `content-type: text/plain`, and the body `a content type`, on wasip2 and wasip3

### Requirement: Entry point: A content type the host refuses is refused, not dropped
When the host refuses the content type as a field value (`wasi_http_types_method_fields_append` fails, for a value that holds a CR, an LF or a NUL for example), the handler SHALL drop the headers it built and answer an internal error instead of the response, before anything is committed, on both versions; it MUST NOT send the response without the header. A tab is a field value's own and is sent as it is. This is part of guarantee 9.

#### Scenario: CR and LF, NUL, and a tab
- **WHEN** `http_bad_content_type` is sent `GET /crlf` (content type `text/plain\r\nx-injected: 1`), `GET /nul` (`text/plain\0x`) and `GET /tab` (`text/plain;\tcharset=utf-8`)
- **THEN** the first two answer `HTTP/1.1 500 Internal Server Error` with wasmtime's HTML page, and the third answers `HTTP/1.1 200 OK` with `content-type: text/plain;<tab>charset=utf-8` and the body `a tab`, on wasip2 and wasip3

### Requirement: Entry point: wasip2 writes every byte of the body in chunks
On wasip2 the handler SHALL answer in this order: refuse a status or a content type it cannot send; build the headers; construct the outgoing response and set its status; take its body; set the outparam with the response; write the body to the body's stream in chunks of at most 4096 bytes, the most `blocking-write-and-flush` accepts per call (`detail::write_all`), dropping the stream afterwards; and finish the body with no trailers. When the response's body cannot be taken, the handler MUST drop the response and answer an internal error. Once the outparam is set nothing can be taken back: a body not written whole is dropped without `finish`, which `wasi:http` reports as a failed response. This is guarantees 4 and 6 on wasip2.

#### Scenario: A body of three chunks and some bytes arrives whole
- **WHEN** `http_long_body` is sent `GET /long`, its main answering a body of 192 lines `line <n> ` padded with `.` to 63 characters and a newline, then the line `end of the body.`, 3 x 4096 + 17 bytes in all
- **THEN** the answer is `HTTP/1.1 200 OK` with `content-type: text/plain` and the body whole, every numbered line of it, equal to `test/http_long_body.expected`, on wasip2

### Requirement: Entry point: wasip3 writes until the stream has taken everything
On wasip3 the handler SHALL answer by returning the response, with its status and headers, from the handler (`exports_wasi_http_handler_handle_return`), then streaming its body through a `stream<u8>` writer and resolving the trailers future. `detail::write_all` MUST write until the stream has taken every byte, however many writes that takes, waiting inside the task with `detail::wait_for` (a waitable set joined to the writer, `webcpp_wasi_http_waitable_set_wait`, the join undone, the set dropped) when a write reports `WEBCPP_WASI_HTTP_WAITABLE_STATUS_BLOCKED`, consuming the count of each completed write, and stopping when the state is not `COMPLETED`; it answers false when the reader went away before the bytes were taken. The trailers future is resolved with no trailers when the body was written whole, and with an internal error when it was not, which is how `wasi:http` 0.3 tells the host the body is incomplete (guarantee 6). The future of whether the host sent the response is dropped unread.

#### Scenario: The long body arrives whole on wasip3
- **WHEN** `http_long_body` is served on wasip3 and sent `GET /long`
- **THEN** the body arrives whole, equal to `test/http_long_body.expected`; wasmtime 47 takes the whole body in one write on wasip3, a body of 1 MiB too, so the test holds that the body arrives whole, not that the loop takes a second turn

### Requirement: Entry point: wasip3 never resumes in its callback
The wasip3 handler SHALL be lifted asynchronously, MUST wait inside the task for each write that blocks and MUST always end by returning `WEBCPP_WASI_HTTP_CALLBACK_CODE_EXIT`, both from `detail::handle` and from the exported callback, because context slot 0 of the component model belongs to wasi-libc on wasip3 and a handler that resumed in its callback would need it (using it trapped, measured). This is guarantee 5.

#### Scenario: Every component served on wasip3 answers
- **WHEN** `http_long_body` and every other served component are built for `wasm32-wasip3` and served with `-S cli,p3 -W component-model-async`
- **THEN** they answer without trapping

### Requirement: Entry point: Failures are reported the way the version defines
A failure found before the response is committed SHALL answer an internal error (`refuse` with `WASI_HTTP_TYPES_ERROR_CODE_INTERNAL_ERROR` and no message), on wasip2 through the outparam and on wasip3 through the handler's return; one found after it MUST be reported the way each version defines: on wasip2 the body is dropped without `finish`, on wasip3 the trailers future is resolved with an internal error. No test can make a write fail on demand: this is held by review (guarantee 6).

#### Scenario: A refusal before the commit
- **WHEN** a main returns `{.status = 600}`
- **THEN** no part of the main's response is sent and the host answers its own `500 Internal Server Error`

### Requirement: Entry point: The handler catches nothing
The handler SHALL throw, try and catch nothing, and wasi MUST need neither exceptions nor RTTI and impose neither. An exception that escapes the main traps the instance, since webcpp builds every program with exceptions on wasip2 and wasip3: wasmtime 47 prints the trap and its backtrace on its standard error (`thrown Wasm exception` among them), answers the client with its own `500 Internal Server Error` HTML page, and keeps serving the requests after it. A main that may throw catches what it can answer itself.

#### Scenario: The documentation says what an escaping exception does
- **WHEN** the page's "Your main" chapter, `AGENTS.md`, the Doc Comment of `http_main` and the README are read
- **THEN** each states that the handler catches nothing and that an exception escaping the main traps the instance, wasmtime 47 answering the client with its own `500 Internal Server Error`

### Requirement: Entry point: Allocation
The handler's own code SHALL allocate nothing: it views the request's strings where they are, and the response's content type where the main put it. Two things allocate, each through its language's default: the request's strings (the target, and an extension method's name), which the bindings copy into the component's memory through the `cabi_realloc` they define (wasi-libc's `realloc`) and the handler frees once it has written the response; and the response's `body`, a `std::string`, through `std::allocator` once it outgrows the 10 characters it holds in itself on wasm32. A program cannot give wasi an allocator of its own.

#### Scenario: The page states what allocates
- **WHEN** the "What it allocates" section of the page's "Limits" chapter is read
- **THEN** it names exactly those two allocations and says the main's own are the program's

### Requirement: Entry point: Limits
The handler SHALL hand the main only the method and the target; it MUST NOT read any request header or request body, and it sends no response header but `content-type`. A main that needs a request's body or its headers, or to set other headers, cannot be written with wasi today. It does not parse the target, route, decode, stream a body or send trailers of the main's.

#### Scenario: The limits are documented
- **WHEN** the page's "Limits" chapter and the README's "Limits" section are read
- **THEN** both state that the handler reads no request header and no request body and sends no response header but `content-type`

### Requirement: Guarantees: Nine numbered promises, each held
The page SHALL state nine guarantees, each keeping its number and the anchor `wasi-invariant-<n>` so that a comment of the code or of a test cites it as `(doc: #wasi-invariant-<n>)`, each with the test or the review that holds it: (1) the entry point compiles only for WASI and for one version; (2) the header defines no function of its own that a program links; (3) nothing written back points into freed memory; (4) every byte of the body is written; (5) on wasip3 the handler never resumes in its callback; (6) a failure is reported the way the version defines; (7) the response carries a `content-type` header only when `content_type` is not empty, and no other header; (8) wasi names nothing of the programs that use it; (9) a response that cannot be sent as the main wrote it is refused, not altered. `b2 libs/wasi/doc`, which builds the page, MUST fail on a citation that names no anchor of the page.

#### Scenario: A citation of the code resolves
- **WHEN** `entrypoint.hpp` cites `(doc: #wasi-invariant-3)` and `(doc: #wasi-invariant-9)`
- **THEN** the page has both anchors, and the build of the page fails if either is removed

#### Scenario: Each guarantee names its holder
- **WHEN** the guarantees chapter is read
- **THEN** guarantee 1 is held by `neither_target_refused`, `both_targets_refused`, the headers compiled alone and `response`; 2 by `http_two_units`; 3 by `http_content_type_of_target` and `response`; 4 by `http_long_body`; 5 by `http_long_body` on wasip3, every component served there, and review; 6 by review; 7 by `http_no_content_type`; 8 by the lint's rule on the banned word and review; 9 by `http_bad_status` and `http_bad_content_type`

### Requirement: Config: WEBCPP_WASI_NO_EXCEPTIONS
`<webcpp/wasi/config.hpp>` SHALL define `WEBCPP_WASI_NO_EXCEPTIONS` when it is not already defined and any of these holds: `WEBCPP_NO_EXCEPTIONS` is defined (the developer disables the exceptions of every webcpp library), `BOOST_NO_EXCEPTIONS` is defined (by the build or by Boost.Config included before), or neither `__cpp_exceptions` (Clang and GCC) nor `_CPPUNWIND` (MSVC) is defined; and it MUST keep a definition the developer made for wasi alone. It includes nothing, Boost.Config and `<webcpp/config.hpp>` included, and so reads `WEBCPP_NO_EXCEPTIONS` itself. wasi raises no exception of its own and its handler catches none, so the macro changes nothing in it; it exists so a program can set it for every webcpp library alike, and an exception wasi raised would go through `boost::throw_exception`. A public header MUST NOT contain a bare `throw`, even in a template nothing instantiates (the lint refuses it). For MrDocs the macro is defined and undefined at once under `__MRDOCS__`, so the reference lists it.

#### Scenario: Defined exactly when the build has no exceptions
- **WHEN** `config_test.cpp`, built as the `config` test with no definition, includes `<webcpp/wasi/config.hpp>` and then `<boost/config.hpp>`
- **THEN** it compiles natively, on wasip2 and on wasip3 if and only if `BOOST_NO_EXCEPTIONS` and `WEBCPP_WASI_NO_EXCEPTIONS` are both defined or both undefined, and `config.hpp` has not included Boost.Config (`BOOST_CONFIG_HPP` is not yet defined after it)

#### Scenario: A developer defines it for wasi alone
- **WHEN** `config_developer_defined` builds `config_test.cpp` with `<define>WEBCPP_WASI_NO_EXCEPTIONS` and `<define>WEBCPP_TEST_WASI_DEVELOPER_DEFINED`, in a build that has exceptions
- **THEN** it compiles, `WEBCPP_WASI_NO_EXCEPTIONS` still defined (`#error "config.hpp drops WEBCPP_WASI_NO_EXCEPTIONS, which the developer defined"` otherwise), natively and on both targets

#### Scenario: A developer disables every webcpp library's exceptions
- **WHEN** `config_all_off` builds `config_test.cpp` with `<define>WEBCPP_NO_EXCEPTIONS` and `<define>WEBCPP_TEST_WASI_ALL_OFF`, in a build that has exceptions
- **THEN** it compiles, `WEBCPP_WASI_NO_EXCEPTIONS` defined (`#error "WEBCPP_NO_EXCEPTIONS does not define WEBCPP_WASI_NO_EXCEPTIONS"` otherwise), natively and on both targets

### Requirement: Build: The b2 targets
`build.jam` SHALL declare `project /webcpp/wasi` and these targets: the alias `wasi`, whose usage requirements are `<include>include` and `<library>/boost//headers`, the headers alone; `http-bindings-p2` and `http-bindings-p3`, declared with `webcpp.wit-bindings` from `wit/http-p2.wit` and `wit/http-p3.wit`, world `http`, renamed `webcpp_wasi_http`, for the version `p2` and `p3` (the second with `--async "wasi:http/handler@0.3.0#handle"`); the alias `http-bindings` of the two, which brings the bindings of the target a program is built for and nothing natively; and the alias `http`, whose usage requirements are `<library>wasi`, `<library>http-bindings`, `<toolset>clang-wasip2:<define>WEBCPP_WASI_HTTP_P2` and `<toolset>clang-wasip3:<define>WEBCPP_WASI_HTTP_P3`. A program that exports the HTTP handler and nothing else links `/webcpp/wasi//http` and is done; it needs no compiled library of wasi's, since there is none.

#### Scenario: The example's Jamfile
- **WHEN** `example/Jamfile` declares `webcpp.targets wasip2 wasip3 ;` and `webcpp.serve http_hello.cpp : <library>/webcpp/wasi//http ;`
- **THEN** the component builds for both targets, with the version's bindings generated and linked and its macro defined, and is served and compared with `http_hello.expected`

#### Scenario: A native build needs no WASI tool
- **WHEN** `b2 libs/wasi/test libs/wasi/example` is run natively
- **THEN** only the tests that need no WASI toolchain are built and run (`response`, `config`, `config_developer_defined`, `config_all_off`, the native headers-alone tests), and none of wit-bindgen, the WIT or wasmtime is looked for

### Requirement: Build: The standard worlds
`wit/http-p2.wit` SHALL declare `package webcpp:wasi-http;` and `world http { export wasi:http/incoming-handler@0.2.12; }`, and `wit/http-p3.wit` the same package and `world http { export wasi:http/handler@0.3.0; }`: each exports the HTTP handler and nothing else. The bindings are generated into the build directory, never committed and never edited. A program whose world exports more than the handler uses `/webcpp/wasi//wasi` (the headers alone), generates its own world's bindings with `webcpp.wit-bindings` renamed `webcpp_wasi_http` too, and defines the macro of its target itself.

#### Scenario: The world renamed
- **WHEN** wit-bindgen's C generator is run on a world with `--rename-world webcpp_wasi_http`
- **THEN** the header it writes is `webcpp_wasi_http.h`, and the names the entry point uses (`webcpp_wasi_http_string_t`, `webcpp_wasi_http_list_u8_t`, `WEBCPP_WASI_HTTP_WAITABLE_STATUS_BLOCKED`, `WEBCPP_WASI_HTTP_CALLBACK_CODE_EXIT`) are the same whatever the world is called

### Requirement: Build: What a build provides the header
A build SHALL provide the entry point with these, listed in the page's "What your build provides": exactly one of `WEBCPP_WASI_HTTP_P2` and `WEBCPP_WASI_HTTP_P3`; the bindings generated with `--rename-world webcpp_wasi_http`; `webcpp_wasi_http.c` compiled as C for the target and `webcpp_wasi_http_component_type.o`, both linked (the glue that calls the handler and the world's type, which the component linker reads); the directory of the bindings on the include path; and `-mexec-model=reactor` on the link, since the program exports a handler and has no C `main` (wasmtime 47 also serves the same program linked as a command, but webcpp builds and tests reactors; `webcpp.serve` adds the flag itself). The host has its part: `wasmtime serve` runs a component of wasi with `-S cli`, since wasi-libc imports parts of `wasi:cli`, and on wasip3 with `-S cli,p3 -W component-model-async`.

#### Scenario: A component of wasip3 is served with its flags
- **WHEN** a component built for `wasm32-wasip3` is served
- **THEN** the command is `wasmtime serve -S cli,p3 -W component-model-async` and, for wasip2, `wasmtime serve -S cli`

### Requirement: Build: The tools and where b2 looks for them
A build that needs a tool SHALL look for it only when a target needs it, and stop naming the tool and every place it looked when it is not there: wit-bindgen 0.62.0 at `-sWIT_BINDGEN=<path>`, else `.local/wit-bindgen/wit-bindgen` in the superproject, else on `PATH`; the `wasi:http` WIT of WASI 0.2.12 and 0.3.0, the `wit/deps` directories of the Rust crates `wasip2` 1.0.4 and `wasip3` 0.9.0, at `-sWASI_WIT_P2=<dir>` and `-sWASI_WIT_P3=<dir>`, else `.local/wasi-wit/p2` and `.local/wasi-wit/p3`; wasmtime 47.0.3 at `-sWASMTIME=<path>`, else on `PATH`, looked for only when a served test runs; and wasi-sdk 34, registered as the toolsets `clang-wasip2` and `clang-wasip3`. A `user-config.jam` may set any of them as `-s` does with `modules.poke : WIT_BINDGEN : <path> ;`. A native build needs none of them.

#### Scenario: A missing tool is named
- **WHEN** a WASI build needs wit-bindgen and none is at the three places
- **THEN** the build stops, naming wit-bindgen and each of the three places it looked

### Requirement: Build: Exceptions on wasip2 and wasip3
webcpp SHALL build every program for wasip2 and wasip3 with exceptions on, encoded as WebAssembly's exception handling: `-fwasm-exceptions -mllvm -wasm-use-legacy-eh=false` on the compile, `-fwasm-exceptions -lunwind` on the link; wasi-sdk 34's Clang emits the legacy encoding by default, which wasmtime 47 refuses with `legacy_exceptions feature required`, and the flag makes it emit the standard `exnref`. A program that uses exceptions needs a host that runs it: wasmtime 47 or later, wasmtime 37 to 46 given `-W exceptions=y`, or jco's `jco transpile` given `--bindgen-enable-wasm-exnref`. A build without exceptions (`-fno-exceptions`, under which Boost.Config defines `BOOST_NO_EXCEPTIONS`) needs the program to define `boost::throw_exception`, which webcpp's `tools/throw_exception.cpp` does by printing and aborting, linked into every program built with `exception-handling=off`; a program built by hand without exceptions compiles with `-fno-exceptions -DBOOST_NO_EXCEPTIONS`. Built with `-fno-exceptions` in place of the exception flags, a component answers the same: measured once, by hand, on wasip3.

#### Scenario: The by-hand commands carry the flags
- **WHEN** the regions `p2` and `p3` of `example/by_hand/build.sh` are read
- **THEN** both compile the program with `-fwasm-exceptions -mllvm -wasm-use-legacy-eh=false -mexec-model=reactor` and link with `-fwasm-exceptions -lunwind`

### Requirement: Build: By hand with wasi-sdk
The complete example SHALL build with wasi-sdk and wit-bindgen alone, without b2, from a directory that holds `main.cpp` (`example/http_hello.cpp`) and `wit/world.wit` (`package example:hello; world service { export wasi:http/incoming-handler@0.2.12; }` for wasip2, `wasi:http/handler@0.3.0` for wasip3) with `wit/deps/` linked to the `wasi:http` WIT. On wasip2: `"$WIT_BINDGEN" c --world service --rename-world webcpp_wasi_http --out-dir gen wit`; `"$WASI_SDK/bin/clang" --target=wasm32-wasip2 -c gen/webcpp_wasi_http.c -o gen/webcpp_wasi_http.o -Igen`; then `"$WASI_SDK/bin/clang++" --target=wasm32-wasip2 -std=c++20 -fwasm-exceptions -mllvm -wasm-use-legacy-eh=false -mexec-model=reactor -DWEBCPP_WASI_HTTP_P2 -I"$WASI/include" -Igen main.cpp gen/webcpp_wasi_http.o gen/webcpp_wasi_http_component_type.o -fwasm-exceptions -lunwind -o hello.wasm`. On wasip3 the bindings are generated with `--async 'wasi:http/handler@0.3.0#handle'` beside `--rename-world`, everything is compiled for `--target=wasm32-wasip3`, the bindings' `.c` with `-pthread`, and the program with `-DWEBCPP_WASI_HTTP_P3`.

#### Scenario: The script builds a component that answers as the example does
- **WHEN** the test `by_hand` runs `example/by_hand/build.sh <p2|p3> <component>` with `WASI_SDK`, `WIT_BINDGEN` and `WASI_WIT` in its environment, in the lane `http`, and serves what it builds
- **THEN** the component answers the requests of `http_hello` with its transcript, and the script exits 2 with `build.sh: <version> is neither p2 nor p3` for any other version

### Requirement: Tests: The served components
Every component that answers HTTP SHALL be tested with `webcpp.serve <source> : <library>/webcpp/wasi//http ;`, which builds it for wasip2 and wasip3, serves it with `wasmtime serve` (`-S cli`, and `-S cli,p3 -W component-model-async` on wasip3) on a port the system chooses, sends it each line `<METHOD> <target>` of `<stem>.requests`, and passes when the transcript equals `<stem>.expected` byte for byte. The transcript writes each request as the curl command that sends it (`$ curl -i -X <METHOD> http://localhost:8080<target>`, each word quoted where a shell would read it otherwise, `curl -I` for `HEAD`, `--request-target '*'` before the bare origin for a target that is not a path), then the status line, the response's headers in lower case and sorted but for `date` and `transfer-encoding`, an empty line and the body as it came; two requests are separated by an empty line. `test/Jamfile` declares seven of them: `http_methods`, `http_long_body`, `http_no_content_type`, `http_two_units` (with `<source>http_two_units_answer.cpp`), `http_content_type_of_target`, `http_bad_status` and `http_bad_content_type`.

#### Scenario: A transcript mismatch fails with the difference
- **WHEN** a served component answers other than its `.expected` file holds
- **THEN** the test fails and shows the difference, and `--dump-tests` and `--out-xml` record it for the test matrix

### Requirement: Tests: The lane http
The served programs, in `test/` and in `example/`, SHALL be named by the lane `http` of their Jamfile (`webcpp.lane http : ... : wasip2 wasip3 ;`), which leaves them out of the ordinary lanes and makes them explicit targets. In `test/`: `http_methods http_long_body http_no_content_type http_two_units http_content_type_of_target http_bad_status http_bad_content_type`; in `example/`: `http_hello by_hand boost_json_http`. A new served program joins its Jamfile's lane on both targets: `b2 declared-lanes`, and the CI's plan with it, stops naming one that no lane runs on a target it is served on. The ordinary lanes run the rest: `b2 -a toolset=clang-wasip2 testing.launcher=wasmtime libs/wasi/test libs/wasi/example`; the served ones: `b2 -a toolset=clang-wasip2 testing.launcher=wasmtime libs/wasi/test//http libs/wasi/example//http`; `toolset=clang-wasip3` likewise.

#### Scenario: The CI runs the lanes as one job per target
- **WHEN** the CI runs
- **THEN** it runs the lanes `http` of both directories as one job on each target, `Own lane (wasi, http, wasip2)` and the same for wasip3, and the test matrix shows the column `wasip2.wasi.http` (and `wasip3.wasi.http`)

### Requirement: Tests: Compile-time refusals and headers alone
`test/Jamfile` SHALL declare `webcpp.targets wasip2 wasip3 ;`; the headers-alone tests `webcpp.headers-alone wasi : ../include : wasi.hpp wasi/http/entrypoint.hpp : <library>/webcpp/wasi//http ;` and `webcpp.headers-alone wasi : ../include : wasi/config.hpp wasi/http/response.hpp : : native wasip2 wasip3 ;`; and the compile-diagnostic tests `neither_target_refused` (with `<library>/webcpp/wasi//http-bindings`) and `both_targets_refused` (with the bindings and `<define>WEBCPP_WASI_HTTP_P2 <define>WEBCPP_WASI_HTTP_P3`), which pass only on the guard's own error and fail on any other, with the bindings of the target and without `/webcpp/wasi//http`, which would define its macro. The tests' own `.clang-tidy` switches off `bugprone-exception-escape.CheckMain`, so that the response test's `main` may let a `std::bad_alloc` end the process, and keeps the check on for everything else (a `noexcept` function, a destructor, a move or a swap).

#### Scenario: A guard error other than the expected one fails the test
- **WHEN** `neither_target_refused.cpp` fails to compile with an error other than the guard's
- **THEN** the compile-diagnostic test fails

### Requirement: Docs: The page and its chapters
The library SHALL have a documentation page `doc/wasi.adoc`, built by `b2 libs/wasi/doc` (`webcpp.doc wasi : wasi.adoc ;`) into `doc/html/index.html` and published at `https://webcpporg.github.io/libs/wasi/`, that includes in this order `introduction.adoc` (introduction, a complete example, requirements, reading this page), `main.adoc` ("Your main": the parameters table, its rules, formatting it with `MacroBlockBegin: "^WEBCPP_WASI_HTTP_MAIN_BEGIN$"` and `MacroBlockEnd: "^WEBCPP_WASI_HTTP_MAIN_END$"`), `response.adoc`, `build.adoc` ("What your build provides", with b2 and by hand), `wasip3.adoc`, `flow.adoc` ("How a request flows"), `limits.adoc`, `guarantees.adoc`, `boost.adoc` ("Building for WASI with Boost"), `testing.adoc`, the reference and the appendix `changelog.adoc`. Every program the page shows was compiled for wasm32-wasip2 and wasm32-wasip3 and run by wasmtime, every answer it shows is what that program answered, and its listings are included from the files the tests run (tags `main`, `p2`, `p3`, `world`, `hello`, `lane`, `by-hand`, `boost`, `includes`, `text`, `catalog`).

#### Scenario: The page builds
- **WHEN** `b2 libs/wasi/doc` is run
- **THEN** it builds the page with the `attribute-missing: warn` setting and no warning, and fails on a citation of a guarantee that names no anchor, and on a `{webcpp-root}` tag that names no tagged region of a file the superproject tracks

### Requirement: Docs: The reference
The reference SHALL list every name the headers declare, in the namespace `webcpp::wasi::http`, and its macros, from the Doc Comments of the headers, each with its declaration, what it does and what it asks of its caller; every public symbol (the response and its three members, `http_main`, `WEBCPP_WASI_HTTP_EXPORTS`, `WEBCPP_WASI_HTTP_MAIN_BEGIN`, `WEBCPP_WASI_HTTP_MAIN_END`, `WEBCPP_WASI_NO_EXCEPTIONS`) MUST have a Doc Comment, and MrDocs builds the reference strict. `webcpp.reference wasi` parses the entry point natively with the bindings of wasip2 and `WEBCPP_WASI_HTTP_P2` (so `WEBCPP_WASI_HTTP_EXPORTS` shows the handler of wasip2), and checks the Doc Comments of its wasip3 branch too, with the bindings and the macro of wasip3. `doc/mrdocs.yml` sets `sort-members: false`.

#### Scenario: The reference is read with the bindings of wasip2
- **WHEN** the reference is built
- **THEN** it is parsed with `/webcpp/wasi//http-bindings-p2-headers` and `<define>WEBCPP_WASI_HTTP_P2`, and the wasip3 branch is checked with `/webcpp/wasi//http-bindings-p3-headers` and `<define>WEBCPP_WASI_HTTP_P3`

### Requirement: Docs: The README's example is a test
The README SHALL open with `webcpp::wasi::http`'s description and a table of its four headers (`<webcpp/wasi.hpp>`, `<webcpp/wasi/http/entrypoint.hpp>`, `<webcpp/wasi/http/response.hpp>`, `<webcpp/wasi/config.hpp>`), its requirements (C++20, wasi-sdk 34, wit-bindgen 0.62.0 measured, the `wasi:http` WIT, a host such as wasmtime 47.0.3 measured), and the sections "A complete example", "Building with b2", "Your main", "The response", "What your build provides", "wasip3", "How a request flows", "Limits", "Building and testing", "Documentation" and "License". Its `main.cpp` and its answers MUST be copies of the region `main` of `example/http_hello.cpp` and of `example/http_hello.expected`, opened with `<!-- include::... -->` comments that doc-check compares; its world and its commands MUST be copies of `example/by_hand/p2/wit/world.wit` and of the region `p2` of `example/by_hand/build.sh`. A change to either changes the README in the same commit.

#### Scenario: The README drifts from the example
- **WHEN** `example/http_hello.expected` changes and the README's copy does not
- **THEN** doc-check fails

### Requirement: Docs: Boost on WASI
The page SHALL carry the chapter "Building for WASI with Boost", which shows the build's own configuration, included by tag through `{webcpp-root}` from `tools/ci/wasi-sdk.jam` (the regions `wasi-sdk`, `wasip2`, `wasip3`) and the Jamroot (`wasi-os`, `wasi-target`, `wasi-exceptions`), and two programs webcpp compiles and runs on both targets: `example/boost_json_command.cpp`, a WASI command that parses a literal JSON text with Boost.JSON with an `error_code` and prints what it read, and `example/boost_json_http.cpp`, a component that answers a catalog's items as JSON. The chapter MUST state that Boost.JSON, Boost.System, Boost.Decimal and Boost.MSM build and run on both targets in webcpp's lanes, that Boost.JSON's definitions are compiled once as `/webcpp//boost_json` and linked with `<library>/webcpp//boost_json`, and that Boost.Asio 1.92 does not compile for wasm32-wasip2 or wasm32-wasip3 with wasi-sdk 34 (wasi-libc defines no `ESHUTDOWN` and no `::pause`, and its `signal.h` stops with `#error`).

#### Scenario: The Boost.JSON command prints the same on every target
- **WHEN** `example/boost_json_command.cpp` is run by wasmtime on wasip2 and on wasip3
- **THEN** it prints `name: "greeter"`, `route: "/v1/greeting"`, `route: "/v1/health"`, `limits: {"body":65536,"debug":true}` and `a broken text: syntax error, read as null`, equal to `boost_json_command.expected`

#### Scenario: The catalog component answers JSON
- **WHEN** `boost_json_http` is sent `GET /v1/items`, `GET /v1/items/2`, `GET /v1/items/9`, `GET /v1/items/2x` and `DELETE /v1/items/2`
- **THEN** it answers `200 OK` `{"items":[{"id":1,"name":"apple"},{"id":2,"name":"pear"}]}`, `200 OK` `{"id":2,"name":"pear"}`, `404 Not Found` `{"error":"no such item","target":"/v1/items/9"}`, `404 Not Found` `{"error":"no such item","target":"/v1/items/2x"}` and `405 Method Not Allowed` `{"error":"only GET"}`, each with `content-type: application/json`

### Requirement: Docs: The changelog
The page SHALL end with an appendix "Changelog", `doc/changelog.adoc`, newest first and one line per change that says what a user sees, with an "Unreleased" section between `// tag::unreleased[]` and `// end::unreleased[]` that the release notes include. Its lines at 0.1.0: a status outside 100-599 answering `200 OK` fixed (it now answers an internal error); a content type viewing the request's method or target not being sent fixed; a content type the host refused being dropped silently fixed (it now answers an internal error); `WEBCPP_WASI_NO_EXCEPTIONS` added, defined automatically or by the developer; the documentation updated (wasmtime's versions, the by-hand build, escaping exceptions and a changelog); and `WEBCPP_NO_EXCEPTIONS` added, which disables wasi's exceptions with every webcpp library's.

#### Scenario: The unreleased region is tagged
- **WHEN** the release notes of a version are built
- **THEN** they include the lines between `tag::unreleased[]` and `end::unreleased[]`

### Requirement: Project: Metadata, repository files and CI
The repository SHALL carry `meta/libraries.json` with `"key": "wasi"`, `"name": "wasi"`, `"authors": ["R"]`, the description "Header-only helpers for C++ built as WebAssembly components: the HTTP handler a component exports on wasip2 and wasip3, and the macros a program writes its main between.", `"category": ["System"]`, `"maintainers": ["R <github.overboard312@passmail.net>"]`, `"cxxstd": "20"` and `"port-of": null`; `LICENSE_1_0.txt`; `.gitattributes` (`* text=auto eol=lf`, `*.bat text eol=crlf`); `.clang-format` inheriting the superproject's (`BasedOnStyle: InheritParentConfig`) and naming the two macros of the main as a block's begin and end; and `.github/workflows/ci.yml`, which on push to `main`, on pull requests and on `workflow_dispatch` calls the superproject's reusable workflow `webcpporg/webcpp/.github/workflows/library.yml@main` with `library: wasi`, with `permissions: contents: read` and a concurrency group that never cancels a run on `main`. The `AGENTS.md` holds only what is specific to wasi and defers to the superproject's.

#### Scenario: The CI is the superproject's
- **WHEN** a commit is pushed to `main`
- **THEN** `ci.yml` builds it inside a checkout of webcpp on every lane wasi declares and on its own lanes, with the page, the lint and the test matrix

### Requirement: Project: Rules of the repository
The repository's `AGENTS.md` SHALL state the rules that bind a change: wasi stands alone and its headers include only the standard library and the bindings; the entry point compiles only for WASI and `response.hpp` compiles everywhere; the standard world exports the HTTP handler and nothing else and its bindings are generated into the build directory; one main per program, with the pair of macros written once; wasip3's handler never resumes in its callback; the headers throw, try and catch nothing, and the handler catches nothing by the owner's decision; `config.hpp` comes first in every public header; every served program is declared with `webcpp.serve` or `webcpp.serve-script` and named by the lane `http`; the README's example is a test; the page's configuration is included by tag and doc-check accepts only a tagged region of a tracked file; and clang-format names the two macros of the main.

#### Scenario: A served program outside its lane is refused
- **WHEN** a program declared with `webcpp.serve` is added to a Jamfile without joining the lane `http` on both targets
- **THEN** `b2 declared-lanes` and the CI's plan stop naming a lane that does not run it
