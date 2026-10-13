## ADDED Requirements

### Requirement: Library: Header-only library in one namespace per WASI version
The library SHALL be header-only C++20, with its public names in `webcpp::wasi::p2` and `webcpp::wasi::p3` (one namespace per version of WASI, as Boost.Process has `boost::process::v1` and `v2`) and what the two versions share in `webcpp::wasi::detail`, public headers under `<webcpp/wasi/...>` (one per responsibility) and `<webcpp/wasi.hpp>`, macros prefixed `WEBCPP_WASI_` (`WEBCPP_WASI_HTTP_`, `WEBCPP_WASI_P2_HTTP_` and `WEBCPP_WASI_P3_HTTP_` for the handler's, since macro names mirror the namespace path as Boost's do) and include guards `WEBCPP_WASI_<PATH>_HPP` (`WEBCPP_WASI_HPP`, `WEBCPP_WASI_CONFIG_HPP`, `WEBCPP_WASI_HTTP_HPP`, `WEBCPP_WASI_P2_HTTP_HPP`, `WEBCPP_WASI_P3_HTTP_HPP`, `WEBCPP_WASI_DETAIL_RESPONSE_HPP`, `WEBCPP_WASI_DETAIL_REQUEST_HPP`). It is webcpp's own and ports nothing; it MUST NOT name a program or a library it was written for, and the lint's rule on the banned word holds that (guarantee 8 of the page). The files carry the notice "Copyright (c) 2026 WebCpp.org" and the Boost Software License 1.0 (`LICENSE_1_0.txt`).

#### Scenario: A program includes the whole library
- **WHEN** a component's source includes `<webcpp/wasi.hpp>` on wasip2 or wasip3 with the build's bindings
- **THEN** it compiles, `webcpp::wasi::http::response`, `webcpp::wasi::http::request` and `WEBCPP_WASI_HTTP_ENTRY_POINT` are declared, and `test/Jamfile`'s headers-alone test of that header passes

#### Scenario: The library names no other webcpp library but config
- **WHEN** the headers, the comments and the examples are read
- **THEN** they include only the standard library, Boost through `<webcpp/config.hpp>`, `<webcpp/config.hpp>` itself and the bindings wit-bindgen writes (the response and the request include no bindings), and no header, comment or example names another webcpp library than config

### Requirement: Library: Depends on webcpp::config and Boost
The library SHALL depend on Boost like every webcpp library: `<webcpp/wasi/config.hpp>` MUST include `<webcpp/config.hpp>` first, webcpp's configuration library, which includes `<boost/config.hpp>`, and adds wasi's own defines after it, and the build's alias `wasi` MUST bring `/webcpp/config//config` and `/boost//headers` to whoever uses it. The earlier statement that wasi needs no Boost, inherited from the README of the project it was written for, is removed from the page, the README and `AGENTS.md`, and the page's by-hand build names Boost's include path and config's.

#### Scenario: config.hpp brings Boost.Config and the version
- **WHEN** a translation unit includes only `<webcpp/wasi/config.hpp>`
- **THEN** `BOOST_CONFIG_HPP`, `WEBCPP_VERSION` and `WEBCPP_LIB_VERSION` are defined, because `<webcpp/config.hpp>` came first

#### Scenario: The by-hand build names the include paths
- **WHEN** the regions `p2` and `p3` of `example/by_hand/build.sh` are read
- **THEN** each puts the include directories of wasi, of webcpp::config and of Boost on the compile line, and neither names `WEBCPP_WASI_HTTP_P2` or `WEBCPP_WASI_HTTP_P3`

### Requirement: Versions: The configured version's namespace is inline
`webcpp::wasi::p2` and `webcpp::wasi::p3` SHALL be the namespaces of the two versions, in `<webcpp/wasi/p2/http.hpp>` and `<webcpp/wasi/p3/http.hpp>` (directories `wasi/p2/` and `wasi/p3/`), and the namespace of the configured version MUST be `inline`, so that `webcpp::wasi::http::...` names the configured target's names. The other version's namespace is never inline. This is the technique of Boost.Process, where the configured one of `v1` and `v2` is `inline`.

#### Scenario: On wasip2 the inline version is p2
- **WHEN** `target_namespace.cpp` is built for wasip2 with the bindings of wasip2
- **THEN** `static_assert(std::is_same_v<decltype(&webcpp::wasi::http::detail::handle), decltype(&webcpp::wasi::p2::http::detail::handle)>)` holds

#### Scenario: On wasip3 the inline version is p3
- **WHEN** `target_namespace.cpp` is built for wasip3 with the bindings of wasip3
- **THEN** the same `static_assert` holds with `webcpp::wasi::p3::http::detail::handle`

### Requirement: Versions: A version holds only the features it provides
Each version's namespace SHALL hold only the feature namespaces that version provides, as Boost.Asio's `posix` and `windows` exist only when the platform has them: `webcpp::wasi::p2::http` is the handler of `wasi:http/incoming-handler@0.2.12` and `webcpp::wasi::p3::http` the asynchronous handler of `wasi:http/handler@0.3.0`. A feature namespace MUST exist per version only where the implementation differs; the files, the sockets, the clocks and the environment go through the same wasi-libc on both versions, so the library declares no `filesystem` or `sockets` namespace in either.

#### Scenario: Only http differs today
- **WHEN** the headers under `include/webcpp/wasi/p2/` and `include/webcpp/wasi/p3/` are listed
- **THEN** each holds `http.hpp` and nothing else, and no namespace `filesystem`, `sockets`, `clocks` or `environment` is declared in `webcpp::wasi`

### Requirement: Versions: What both share is in detail and appears by using
What both versions share SHALL live in `webcpp::wasi::detail`: `response`, `request` and the internals the handlers have in common, such as `sendable`; each version's `http` namespace MUST bring the public ones by `using`, so that `response` and `request` are named `webcpp::wasi::p2::http::response` and `webcpp::wasi::p3::http::response` and are one type. There is no public `webcpp::wasi::http` outside the versions on a WASI build, which would be ambiguous against the inline version's `http`.

#### Scenario: The version's response is the shared one
- **WHEN** `target_namespace.cpp` is built for wasip2 and for wasip3
- **THEN** `std::is_same_v<webcpp::wasi::http::response, webcpp::wasi::detail::response>` and the same for `request` hold in both

#### Scenario: A WASI build has one http
- **WHEN** `target_namespace.cpp` names `webcpp::wasi::http::request` on wasip2 or wasip3
- **THEN** the lookup is not ambiguous, since only the inline version's `http` is visible there

### Requirement: Versions: Each handler is in its own header, and the choice is made once
Each version's handler SHALL be in a header of its own, readable alone: `<webcpp/wasi/p2/http.hpp>` holds the code of wasip2 and `<webcpp/wasi/p3/http.hpp>` the code of wasip3, with no `#if` inside the logic of either. The only choice between them MUST be made once, in the front header `<webcpp/wasi/http.hpp>`, which includes the configured version's header.

#### Scenario: The headers hold no choice
- **WHEN** `p2/http.hpp` and `p3/http.hpp` are read
- **THEN** neither contains a conditional directive other than its include guard, and neither names a symbol of the other version's bindings (`incoming_handler` in `p3/http.hpp`, `handler_handle_return` in `p2/http.hpp`)

#### Scenario: The front makes the one choice
- **WHEN** `http.hpp` is read
- **THEN** it holds the conditional that includes `p2/http.hpp` or `p3/http.hpp`, from `WEBCPP_WASI_TARGET_P2` and `WEBCPP_WASI_TARGET_P3`, and no other

### Requirement: Target: The version follows the compiler
`<webcpp/wasi/config.hpp>` SHALL define exactly one of `WEBCPP_WASI_TARGET_P2` and `WEBCPP_WASI_TARGET_P3` when a version is configured, picked from the compiler's own `__wasip2__` and `__wasip3__`, and none when the build is not for WebAssembly. wasip1 and the other WebAssembly targets MUST stop at webcpp::config's error `unsupported target`, which `<webcpp/config.hpp>` raises before wasi's block is read. The macros come from the compiler alone, never from the build system, so a program compiled by hand gets the version b2 would give it.

#### Scenario: A wasm32-wasip2 build
- **WHEN** `target_namespace.cpp` is built with `toolset=clang-wasip2`
- **THEN** `WEBCPP_WASI_TARGET_P2` is defined and `WEBCPP_WASI_TARGET_P3` is not

#### Scenario: A wasm32-wasip3 build
- **WHEN** `target_namespace.cpp` is built with `toolset=clang-wasip3`
- **THEN** `WEBCPP_WASI_TARGET_P3` is defined and `WEBCPP_WASI_TARGET_P2` is not

#### Scenario: A native build and wasip1
- **WHEN** `<webcpp/wasi/config.hpp>` is compiled natively, and with `--target=wasm32-wasip1`
- **THEN** natively neither macro is defined, and for wasip1 clang `-verify` sees webcpp::config's `unsupported target` error and no other

### Requirement: Target: WEBCPP_WASI_TARGET overrides the compiler
`WEBCPP_WASI_TARGET`, defined by the developer as `p2` or `p3`, SHALL override the compiler's choice, as `BOOST_PROCESS_VERSION` does for Boost.Process; `<webcpp/wasi/config.hpp>` MUST then define the matching `WEBCPP_WASI_TARGET_P2` or `WEBCPP_WASI_TARGET_P3` and make that version's namespace the inline one. Any other value MUST stop with the single error `WEBCPP_WASI_TARGET is p2 or p3`. config never defines `WEBCPP_WASI_TARGET`, and the build does not define it: the compiler decides unless a developer says otherwise.

#### Scenario: The override names the other version
- **WHEN** `target_override_p3` builds `target_namespace.cpp` for wasip2 with `<define>WEBCPP_WASI_TARGET=p3` and the bindings of wasip3, and `target_override_p2` the reverse
- **THEN** `webcpp::wasi::http::detail::handle` is that of the version the override names, `WEBCPP_WASI_TARGET_P3` (respectively `_P2`) is defined and the other is not

#### Scenario: A value that is neither version
- **WHEN** `target_refused.cpp` includes `<webcpp/wasi/config.hpp>` with `<define>WEBCPP_WASI_TARGET=p4`
- **THEN** the compile fails with exactly the error `WEBCPP_WASI_TARGET is p2 or p3`, which clang's `-verify` finds, and with no other

### Requirement: Entry point: A plain function and one macro
A program SHALL write an ordinary C++ function that takes a `webcpp::wasi::http::request const&` and returns a `webcpp::wasi::http::response`, and name it once, at global scope, with `WEBCPP_WASI_HTTP_ENTRY_POINT(fn)` from `<webcpp/wasi/http.hpp>`, which forwards to the configured version's macro (`WEBCPP_WASI_P2_HTTP_ENTRY_POINT` or `WEBCPP_WASI_P3_HTTP_ENTRY_POINT`). The function has no ABI and no name the library fixes, may live in any namespace (the macro takes its qualified name), and may be defined in another translation unit than the macro's. The pair `WEBCPP_WASI_HTTP_MAIN_BEGIN`/`_END`, `WEBCPP_WASI_HTTP_EXPORTS` and `http_main` of 0.1.0 do not exist.

#### Scenario: The complete example
- **WHEN** `example/http_hello.cpp`, which includes `<webcpp/wasi/http.hpp>`, defines `webcpp::wasi::http::response hello(webcpp::wasi::http::request const& request)` answering 405 `only GET` for any method but `GET` and otherwise `hello from ` and `request.target`, and ends with `WEBCPP_WASI_HTTP_ENTRY_POINT(hello)`, is served and sent `GET /v1/greeting?name=ana` and `POST /x`
- **THEN** the transcript is `HTTP/1.1 200 OK` with `content-type: text/plain` and the body `hello from /v1/greeting?name=ana`, then `HTTP/1.1 405 Method Not Allowed` with `content-type: text/plain` and the body `only GET`, byte for byte `example/http_hello.expected`

#### Scenario: The function is defined in another translation unit
- **WHEN** `http_two_units.cpp` declares the entry point with `WEBCPP_WASI_HTTP_ENTRY_POINT(two_units::answer)`, `two_units::answer(request)` is defined in `http_two_units_answer.cpp`, and `GET /two?units=1` is sent
- **THEN** the answer is `200 OK`, `content-type: text/plain` and the body `from the other unit: GET /two?units=1`

### Requirement: Entry point: The explicit macro of each version
`<webcpp/wasi/p2/http.hpp>` SHALL define `WEBCPP_WASI_P2_HTTP_ENTRY_POINT(fn)` and `<webcpp/wasi/p3/http.hpp>` `WEBCPP_WASI_P3_HTTP_ENTRY_POINT(fn)`, each the export of its own version, so that a program names a version whatever the configuration says. `WEBCPP_WASI_HTTP_ENTRY_POINT(fn)` MUST only forward to the configured version's macro. A program uses an explicit macro with the bindings of that version.

#### Scenario: A program names its version
- **WHEN** `http_explicit.cpp` includes `<webcpp/wasi/http.hpp>` and ends with `WEBCPP_WASI_P2_HTTP_ENTRY_POINT(hello)` on wasip2 and `WEBCPP_WASI_P3_HTTP_ENTRY_POINT(hello)` on wasip3, chosen by `WEBCPP_WASI_TARGET_P2`, and is served on each target
- **THEN** each component answers the transcript of `http_hello`

#### Scenario: The front forwards
- **WHEN** `WEBCPP_WASI_HTTP_ENTRY_POINT(hello)` is expanded on wasip2 and on wasip3
- **THEN** it expands to `WEBCPP_WASI_P2_HTTP_ENTRY_POINT(hello)` and to `WEBCPP_WASI_P3_HTTP_ENTRY_POINT(hello)`

### Requirement: Entry point: The request
`webcpp::wasi::detail::request` SHALL be an aggregate of exactly two members, in this order, `std::string_view method` and `std::string_view target`, declared in `<webcpp/wasi/detail/request.hpp>` and named `webcpp::wasi::http::request` in the configured version; the handler builds one for each call and passes it by `const&`. Both views point into the request, valid during the call: the function copies what it keeps beyond it. The request carries nothing else: the headers and the body are a later extension, not part of this change.

#### Scenario: A request is built by designated initializers
- **WHEN** `request_test` builds `webcpp::wasi::detail::request{.method = "GET", .target = "/a?b=c"}`
- **THEN** `method == "GET"` and `target == "/a?b=c"`, natively and on both targets

### Requirement: Entry point: The program's function runs natively
The function a program writes SHALL compile and run natively, with no WASI toolchain, so that a native test calls it directly: `<webcpp/wasi/http.hpp>` compiles natively and, there, declares `webcpp::wasi::http::response` and `webcpp::wasi::http::request` and defines `WEBCPP_WASI_HTTP_ENTRY_POINT(fn)` as nothing. The macro's expansion is the only code of a program that needs the component ABI.

#### Scenario: A native test calls the function
- **WHEN** `function_native.cpp` defines a function answering `200`, `text/plain` and `hello from /x` for `GET /x`, writes `WEBCPP_WASI_HTTP_ENTRY_POINT` for it, and calls it with `request{.method = "GET", .target = "/x"}`
- **THEN** it compiles and passes natively, and on wasip2 and wasip3 under wasmtime

### Requirement: Entry point: One source, one component per target, one export each
A program SHALL be built once per target from one source, and each component MUST export the handler of one version only: the entry-point macros define the export of one version, and wasi never defines both. A component that exports both `wasi:http/incoming-handler@0.2.12` and `wasi:http/handler@0.3.0` is not built, because measured with wasi-sdk 34 and wasmtime 47.0.3: wasmtime always answers through the p3 export, a host without p3 cannot load it (it imports `wasi:http/types@0.3.0`), Spin 4.2.2 refuses two handlers, it grows 24%, and one module linked against both libcs fails in wasm-ld.

#### Scenario: One source, two components
- **WHEN** `http_hello.cpp` is built with `webcpp.targets wasip2 wasip3 ;`
- **THEN** b2 builds one component for each target, each with the bindings of its own world (`wit/http-p2.wit` or `wit/http-p3.wit`), and both answer `http_hello.expected`

### Requirement: Config: WEBCPP_WASI_HAS_SOCKETS
`<webcpp/wasi/config.hpp>` SHALL define `WEBCPP_WASI_HAS_SOCKETS` when the compiler defines `__wasip2__` or `__wasip3__`, and MUST NOT define it natively or on wasip1, which has no `<netdb.h>`. It says that sockets compile; the host decides whether they run (`-S inherit-network`).

#### Scenario: Sockets compile on wasip2 and wasip3
- **WHEN** `capabilities.cpp` is built natively, for wasip2 and for wasip3
- **THEN** `WEBCPP_WASI_HAS_SOCKETS` is defined on wasip2 and wasip3 and not natively

### Requirement: Config: WEBCPP_WASI_HAS_THREADS
`<webcpp/wasi/config.hpp>` SHALL define `WEBCPP_WASI_HAS_THREADS` when the compiler defines `__wasi_cooperative_threads__` (from `<wasi/version.h>`), and MUST NOT read `__STDCPP_THREADS__`. Measured with wasi-sdk 34 and wasmtime 47.0.3: no thread runs on either target; `__wasi_cooperative_threads__` is undefined on both, and wasip3 predefines `__STDCPP_THREADS__` and links `std::thread`, which then fails at run time with ENOTSUP.

#### Scenario: No threads on either target today
- **WHEN** `capabilities.cpp` is built for wasip2 and for wasip3 with wasi-sdk 34
- **THEN** `WEBCPP_WASI_HAS_THREADS` is not defined on either, though `__STDCPP_THREADS__` is defined on wasip3

### Requirement: Config: WEBCPP_WASI_HAS_EXCEPTIONS
`<webcpp/wasi/config.hpp>` SHALL define `WEBCPP_WASI_HAS_EXCEPTIONS` when the compiler defines `__wasm_exception_handling__`, and MUST NOT read `__cpp_exceptions`, which wasm defines without `-fwasm-exceptions`. It stands next to `WEBCPP_WASI_NO_EXCEPTIONS` and says that the component carries WebAssembly's exception handling, which a host must run.

#### Scenario: Built with and without the exception flags
- **WHEN** `capabilities.cpp` is built for wasip2 and for wasip3, with the build's exception flags and with `exception-handling=off`
- **THEN** `WEBCPP_WASI_HAS_EXCEPTIONS` is defined in the first and not in the second

### Requirement: Build: A run script beside every WASI program
For every program it builds for wasip2 or wasip3 the build system SHALL write a POSIX `sh` script, `<program>.sh`, beside the `.wasm` in the same build directory, that runs the program with wasmtime: `wasmtime run` for a command and `wasmtime serve` for an HTTP component. The script MUST find wasmtime in `$WASMTIME`, else on `PATH`, and when it is in neither stop with exit status 127 and the message `<script>: wasmtime not found: install it, or set WASMTIME`; MUST add `$WASMTIME_FLAGS` as a list of words; MUST find the `.wasm` relative to itself, so that the pair is copied anywhere together; and MUST `exec` wasmtime, so that a signal sent to the script reaches it. A command's script forwards `"$@"` to the program, after `--`. An HTTP component's script takes an optional first argument that does not start with `-` as the address, `127.0.0.1:8080` by default, and passes the other arguments to `wasmtime serve`.

#### Scenario: The command line of each artifact
- **WHEN** the scripts of a program built with exceptions and of one built with `exception-handling=off` are read, for a command and for an HTTP component, on wasip2 and on wasip3
- **THEN** their `exec` lines run `run -W exceptions=y`, `run`, `run -S p3 -W component-model-async -W exceptions=y`, `run -S p3 -W component-model-async`, `serve -S cli -W exceptions=y`, `serve -S cli`, `serve -S p3 -W component-model-async -W exceptions=y` and `serve -S p3 -W component-model-async` respectively, each followed by `$WASMTIME_FLAGS`, and the serve ones by `"$@" --addr "$address"`

#### Scenario: wasmtime is missing
- **WHEN** a script runs with `WASMTIME=/nowhere/wasmtime`, and with `PATH=/usr/bin:/bin` and no `WASMTIME`
- **THEN** each prints `<script>: wasmtime not found: install it, or set WASMTIME` on standard error and exits 127

#### Scenario: A served script takes an address and options
- **WHEN** `http_hello.sh` runs with no argument, with `127.0.0.1:18480 --env X=1`, and with `-S inherit-network`
- **THEN** the first serves on `127.0.0.1:8080`, the second on `127.0.0.1:18480` with the option passed to wasmtime, and the third on `127.0.0.1:8080` with the option passed, since an argument that starts with `-` is no address

### Requirement: Build: The run scripts are a rule of the build system
The run scripts SHALL be written by a rule of the build system, in `tools/component/component.jam` beside the one place that knows wasmtime's flags and in the helper of `tools/webcpp.jam` that declares a program, as `make <program>.sh : <program> : @webcpp-component.command-script` and, for a served component, `make <stem>-component.sh : <stem>-component : @webcpp-component.serve-script`, so that every library's WASI programs get one with their requirements and the exception setting decides the flags. They are written for every toolset of a WASI target and `<build>no` on every other, read from the toolset and its version only. The rule of 0.1.0 that composed `wasmtime serve` flags in `serve.py` is replaced by it. No `.cmd` is written until a Windows WASI lane exists.

#### Scenario: A program gets its script
- **WHEN** `b2 toolset=clang-wasip3 hello.sh` is run for a command `hello` and `http_hello.sh` for a component
- **THEN** `hello.sh` and `http_hello.sh` are written beside `hello.wasm` and `http_hello.wasm`, executable, in the build directory b2 gave the program

#### Scenario: A native build writes none
- **WHEN** the native toolset builds the tests of `libs/wasi`
- **THEN** no `.sh` is declared, and none of wasmtime is looked for

### Requirement: Tests: The served tests and the examples run through the scripts
`serve.py` and `run_example.py` SHALL run a program through its script, so that the CI tests the scripts: `serve.py --script <component>.sh` runs `<component>.sh 127.0.0.1:0` instead of composing `wasmtime serve` itself, its keeper still seeing wasmtime through `exec`; `run_example.py` takes `--launcher sh` and the script in place of the program. b2's own `webcpp.run` tests may keep `testing.launcher=wasmtime`, whose minimal line is the same on wasmtime 47.

#### Scenario: A served test runs through its script
- **WHEN** `http_methods` is served on wasip3
- **THEN** the harness starts `http_methods-component.sh 127.0.0.1:0`, reads the address wasmtime prints, and compares the transcript with `http_methods.expected`

### Requirement: Tests: The run scripts are tested
`test/Jamfile` SHALL declare a check of the run scripts of a command and of a component on both targets: that they are generated, that they pass `shellcheck -s sh` with no finding, that they run under `dash` and from a path with a space in it, and that the exit status is 127 when wasmtime is missing. shellcheck and dash are looked for only by this check, and when one is missing the check stops naming it and every place it looked.

#### Scenario: The scripts pass the check
- **WHEN** the check runs on the scripts of `response` (a command) and `http_hello` (a component) for wasip2 and wasip3
- **THEN** shellcheck reports nothing, `dash <script> one 'two words'` runs the command with two arguments, and a copy of the script and its `.wasm` in `with space/` runs

### Requirement: Tests: The target tests
`test/Jamfile` SHALL declare the tests of the target: `target_namespace` (the configured version's namespace is the inline one, on wasip2 and wasip3, and the shared names are the version's), `target_override_p2` and `target_override_p3` (the override names the other version's namespace, with that version's bindings), `target_refused` (a compile diagnostic: `WEBCPP_WASI_TARGET=p4`), `capabilities` (the three capability macros against the measured table, natively and on both targets), `function_native`, `request` and `http_explicit` (served on both targets).

#### Scenario: The capability table
- **WHEN** `capabilities.cpp` is built natively, for wasip2 and for wasip3 with wasi-sdk 34
- **THEN** it holds `WEBCPP_WASI_HAS_SOCKETS` defined on wasip2 and wasip3 and not natively, `WEBCPP_WASI_HAS_THREADS` defined nowhere, and `WEBCPP_WASI_HAS_EXCEPTIONS` defined when the build uses WebAssembly's exception handling

### Requirement: Docs: Versions, targets and what the host grants
The page SHALL carry the chapter "Versions and targets" (`doc/targets.adoc`), which says what each of `webcpp::wasi::p2` and `p3` holds, how the configured one is chosen (the compiler's `__wasip2__` and `__wasip3__`, `WEBCPP_WASI_TARGET` as the override), that one source is built once per target and why a component with both exports is not built (the measurements of the requirement above), and the capability macros. It MUST table what each feature needs from wasmtime, as measured with wasi-sdk 34 and wasmtime 47.0.3: `--dir HOST[::GUEST]` for files, `-S inherit-network` for sockets, `-S inherit-network,allow-ip-name-lookup` for names, `--env NAME[=VAL]` or `-S inherit-env` for the environment, and that `std::thread` compiles and fails with ENOTSUP on both targets. A chapter "Running a program" (`doc/running.adoc`) shows the run scripts, `WASMTIME` and `WASMTIME_FLAGS`.

#### Scenario: The table names the grants
- **WHEN** the chapter "Versions and targets" is read
- **THEN** its table lists files, sockets, name lookup, the environment and threads with the wasmtime option each needs and the capability macro that says it compiles

### Requirement: Docs: The changelog lists the redesign
The "Unreleased" section of `doc/changelog.adoc` SHALL list the redesign, one line per change a user sees, with the breaking ones marked: **BREAKING** `WEBCPP_WASI_HTTP_MAIN_BEGIN`, `WEBCPP_WASI_HTTP_MAIN_END` and `WEBCPP_WASI_HTTP_EXPORTS` replaced by a plain function and `WEBCPP_WASI_HTTP_ENTRY_POINT`; **BREAKING** `webcpp::wasi::http::http_main` removed, the function takes a `request`; **BREAKING** `<webcpp/wasi/http/entrypoint.hpp>` and `<webcpp/wasi/http/response.hpp>` replaced by `<webcpp/wasi/http.hpp>`; **BREAKING** the build no longer defines `WEBCPP_WASI_HTTP_P2` and `WEBCPP_WASI_HTTP_P3`, the compiler decides and `WEBCPP_WASI_TARGET` overrides; wasi depends on webcpp::config and Boost; the namespaces `webcpp::wasi::p2` and `p3`; the capability macros; and the run scripts. The lines are between `// tag::unreleased[]` and `// end::unreleased[]`, and the change is listed in webcpp's first release notes, made before that release.

#### Scenario: The breaking lines are in the release notes
- **WHEN** the release notes of webcpp's first release are built
- **THEN** they include the lines between `tag::unreleased[]` and `end::unreleased[]`, the breaking ones marked

## MODIFIED Requirements

### Requirement: Library: Public headers and the convenience header
The library SHALL ship these public headers: `include/webcpp/wasi/config.hpp`, the front `include/webcpp/wasi/http.hpp`, the handlers of the two versions `include/webcpp/wasi/p2/http.hpp` and `include/webcpp/wasi/p3/http.hpp`, the ABI-free `include/webcpp/wasi/detail/response.hpp` and `include/webcpp/wasi/detail/request.hpp`, and the convenience header `include/webcpp/wasi.hpp`, which MUST include `<webcpp/wasi/config.hpp>` and `<webcpp/wasi/http.hpp>` and nothing else. Every public header but `config.hpp` itself includes `<webcpp/wasi/config.hpp>` first (`wasi.hpp`, `http.hpp`, the two handlers, `response.hpp` and `request.hpp` do); `config.hpp` includes `<webcpp/config.hpp>` first and nothing else. `response.hpp` and `request.hpp` MUST include nothing of the component ABI. `http.hpp` includes the header of the configured version, `p2/http.hpp` or `p3/http.hpp`, and neither on a build that is not for WebAssembly. Each header compiles alone, as the only include of its translation unit.

#### Scenario: Each header compiles alone
- **WHEN** `test/Jamfile` compiles `wasi.hpp` and `wasi/http.hpp` on wasip2 and wasip3 with `/webcpp/wasi//http`, `wasi/p2/http.hpp` on wasip2 with `/webcpp/wasi//http-bindings-p2`, `wasi/p3/http.hpp` on wasip3 with `/webcpp/wasi//http-bindings-p3`, and `wasi/config.hpp`, `wasi/detail/response.hpp` and `wasi/detail/request.hpp` natively, on wasip2 and on wasip3, each as the only include of its translation unit
- **THEN** every one of the compiles passes

#### Scenario: The convenience header brings the entry point
- **WHEN** `test/http_two_units_answer.cpp` includes `<webcpp/wasi.hpp>` and uses `webcpp::wasi::http::response` and `webcpp::wasi::http::request`
- **THEN** it compiles on wasip2 and wasip3

### Requirement: Library: The response
The library SHALL declare `struct response` in `<webcpp/wasi/detail/response.hpp>`, namespace `webcpp::wasi::detail` (named `webcpp::wasi::http::response` in the configured version, by `using`), an aggregate of exactly three members, in this order, each with a default so that a designated initializer names only what differs: `unsigned status = 200`, `std::string_view content_type{}` and `std::string body{}`. The members MUST be declared in that order, which is the order of an HTTP response and of the designated initializer that builds one, and the reference MUST list them in it (`sort-members: false`).

#### Scenario: A response left at its defaults
- **WHEN** `detail::response answer;` is default-initialized
- **THEN** `answer.status == 200U`, `answer.content_type.empty()` and `answer.body.empty()` (`response_test`'s `defaults`)

#### Scenario: A designated initializer names one member
- **WHEN** `detail::response answer{.content_type = "text/plain"};` is initialized
- **THEN** `answer.status == 200U`, `answer.content_type == "text/plain"` and `answer.body.empty()` (`response_test`'s `designated`)

### Requirement: Response: The body is a copy, the content type a view
The `body` of a response SHALL be a `std::string` that owns its text, and the `content_type` a `std::string_view` that does not. The body is sent whole, however long, after the function has returned, so it MUST NOT point into a string of the function's own; the content type is read after the function has returned too, so it views what outlives the function: a string literal such as `"application/json"`, or the `method` or the `target` of the `request` the function was given, never a string of the function's own. A view is used because a content type is nearly always a literal and costs no allocation, where a `std::string` of 16 characters exceeds the 10 characters libc++ keeps in the string itself on wasm32.

#### Scenario: The body is a copy
- **WHEN** `std::string text = "hello"; const detail::response answer{.status = 404, .body = text};` is followed by `text.assign("other")`
- **THEN** `answer.status == 404U` and `answer.body == "hello"` (`response_test`'s `owns_its_body`)

#### Scenario: A content type viewing the target is sent as it reads
- **WHEN** `http_content_type_of_target`'s function answers `GET /text/plain` with `.content_type = request.target.substr(1)`
- **THEN** the answer is `200 OK` with `content-type: text/plain` and the body `the content type of /text/plain`, and `GET /application/json` answers `content-type: application/json`, on wasip2 and wasip3

### Requirement: Response: Compiles natively and carries no ABI
`<webcpp/wasi/detail/response.hpp>` and `<webcpp/wasi/detail/request.hpp>` SHALL compile everywhere, natively included, because they include only `<webcpp/wasi/config.hpp>` (which includes `<webcpp/config.hpp>`, and so Boost.Config), `<string>` and `<string_view>`, so that the rest of a program builds responses, holds them and passes them around (as messages between actors, for example) and is tested natively, without a WASI toolchain. `<webcpp/wasi/config.hpp>` compiles everywhere too.

#### Scenario: The response test runs natively and on both targets
- **WHEN** `b2 libs/wasi/test` is run natively, and with `toolset=clang-wasip2` and `toolset=clang-wasip3` with `testing.launcher=wasmtime`
- **THEN** `response` and `request` (`webcpp.run` tests with `native wasip2 wasip3`) run and pass in each, `response_test.cpp` and `request_test.cpp` ending in `boost::report_errors()`

#### Scenario: A header alone compiles natively
- **WHEN** `detail/response.hpp`, `detail/request.hpp` and `config.hpp` are each compiled alone for the native toolset
- **THEN** all three compile

### Requirement: Entry point: The handler's exports
`WEBCPP_WASI_P2_HTTP_ENTRY_POINT(fn)` and `WEBCPP_WASI_P3_HTTP_ENTRY_POINT(fn)` SHALL define the `extern "C"` function or functions the version's world exports, under the names wit-bindgen generates for a world of one version, which the component's glue `webcpp_wasi_http.c` calls and which convert the request and call `fn`: on wasip2 `extern "C" void exports_wasi_http_incoming_handler_handle(exports_wasi_http_incoming_handler_own_incoming_request_t request, exports_wasi_http_incoming_handler_own_response_outparam_t out)`, which hands the request and `fn` to `webcpp::wasi::p2::http::detail::handle`; on wasip3 `extern "C" webcpp_wasi_http_callback_code_t exports_wasi_http_handler_handle(exports_wasi_http_handler_own_request_t request)`, which returns `webcpp::wasi::p3::http::detail::handle` for it and `fn`, and `exports_wasi_http_handler_handle_callback(webcpp_wasi_http_event_t*)`, which returns `WEBCPP_WASI_HTTP_CALLBACK_CODE_EXIT` and is never reached. A program names `WEBCPP_WASI_HTTP_ENTRY_POINT`, which forwards to the configured version's macro, and never writes these names.

#### Scenario: A served component answers on both versions
- **WHEN** any served component of `test/` or `example/` is built for wasip2 and for wasip3 and served by `wasmtime serve`
- **THEN** the host finds the exported handler of its version and the component answers

### Requirement: Entry point: The header defines nothing a program links
The entry point SHALL define no function of its own that a program links: the `extern "C"` functions are defined by the expansion of `WEBCPP_WASI_HTTP_ENTRY_POINT` in the program, and the headers hold only inline functions, macros and declarations, so that every translation unit of a program may include them and the program writes the macro once. This is guarantee 2.

#### Scenario: Two translation units include the entry point
- **WHEN** `http_two_units` is built from `http_two_units.cpp` (which includes `<webcpp/wasi/http.hpp>` and writes the macro) and `http_two_units_answer.cpp` (which includes `<webcpp/wasi.hpp>`), and `GET /two?units=1` is sent
- **THEN** the component links with no duplicate symbol and answers `200 OK`, `content-type: text/plain`, `from the other unit: GET /two?units=1`

### Requirement: Entry point: The method
The handler SHALL give the function the request's method, in `request.method`, as it travels on the wire, by `webcpp::wasi::detail::method_name`: `"GET"`, `"HEAD"`, `"POST"`, `"PUT"`, `"DELETE"`, `"CONNECT"`, `"OPTIONS"`, `"TRACE"` and `"PATCH"` for the nine enumerators of `wasi:http`'s method, and, for any other, the string of the `other` case as it was sent, without copying it (`webcpp::wasi::detail::view_of`).

#### Scenario: Each method reaches the main by name
- **WHEN** `http_methods` is sent `GET /a?b=c%20d`, `HEAD /a?b=c%20d`, `POST /`, `PUT /items/7`, `DELETE /items/7`, `OPTIONS *`, `TRACE /trace`, `PATCH /items/7?x=1&y=%2F` and `PURGE /cache/key`
- **THEN** each answer is `200 OK` with `content-type: text/plain; method=<METHOD>` and, but for `HEAD`, whose answer carries no body, a body `<METHOD> <target>` echoing the request, `PURGE` included as an extension method; `CONNECT` is left out of the test because the test client cannot send it to an origin server

### Requirement: Entry point: The target
The handler SHALL give the function the request target, in `request.target`, its path and its query, as it was sent and with nothing decoded, obtained from the request's `path-with-query`, and `"/"` when the request carried none; `OPTIONS *` reaches the main as `"*"`. The handler MUST NOT parse the target: a function that needs the path and the query apart parses it with a URL library, such as Boost.URL's `parse_origin_form`.

#### Scenario: A target with a query and an escape arrives verbatim
- **WHEN** `http_methods` is sent `GET /a?b=c%20d`, `PATCH /items/7?x=1&y=%2F` and `OPTIONS *`
- **THEN** the bodies are `GET /a?b=c%20d`, `PATCH /items/7?x=1&y=%2F` and `OPTIONS *`

### Requirement: Entry point: The request outlives the response
The handler SHALL free the method, the target (`wasi_http_types_method_free`, `webcpp_wasi_http_string_free`) and the request (`wasi_http_types_incoming_request_drop_own` on wasip2, `wasi_http_types_request_drop_own` on wasip3) only after the response has been written, so that a `content_type` that views the method or the target is still valid when it is sent; the two views of the `request` the function receives look into the request of the component ABI, and the function copies what it keeps beyond the call. This is guarantee 3. The response owns its body, so no body views the request.

#### Scenario: A content type viewing the target survives
- **WHEN** `http_content_type_of_target` is sent `GET /text/plain` and `GET /application/json`
- **THEN** the answers carry `content-type: text/plain` and `content-type: application/json`, which a handler that freed the target first would fail every time, since the free overwrites its first bytes with the allocator's links

### Requirement: Entry point: A status outside 100 to 599 is refused
The handler SHALL send a response's `status` as it is when `webcpp::wasi::detail::sendable` holds, that is `100 <= status && status <= 599`, and MUST otherwise send no response of the function's and answer an internal error instead (`refuse`), before anything is committed, on both versions. `wasi:http`'s `set-status-code` refuses some statuses and leaves the response at 200, and the status being 16 bits wide, 65736 would read as 200. wasmtime 47 sends the internal error as its own `500 Internal Server Error`. This is part of guarantee 9.

#### Scenario: Statuses at and beyond the range
- **WHEN** `http_bad_status`, whose function answers the status its target names, is sent `GET /0`, `GET /99`, `GET /600`, `GET /65736`, `GET /599` and `GET /200`
- **THEN** the first four answer `HTTP/1.1 500 Internal Server Error` with `content-length: 205`, `content-type: text/html; charset=UTF-8` and wasmtime's HTML page naming `500 Internal Server Error`, `GET /599` answers `HTTP/1.1 599 <none>` with the body `the status 599`, and `GET /200` answers `HTTP/1.1 200 OK` with the body `the status 200`, on wasip2 and wasip3

### Requirement: Entry point: The content-type header, and no other
The handler SHALL send a `content-type` header if and only if the response's `content_type` is not empty, with that value as a field value, and MUST send no other header (`webcpp::wasi::detail::headers_of`). An empty content type sends no header, with a body or without one. This is guarantee 7.

#### Scenario: A set and an empty content type
- **WHEN** `http_no_content_type` is sent `GET /none`, `GET /empty` and `GET /plain`
- **THEN** `/none` answers `HTTP/1.1 200 OK` with no header and the body `no content type`, `/empty` answers `HTTP/1.1 200 OK` with no header and no body, and `/plain` answers `HTTP/1.1 200 OK` with exactly one header, `content-type: text/plain`, and the body `a content type`, on wasip2 and wasip3

### Requirement: Entry point: wasip2 writes every byte of the body in chunks
On wasip2 the handler SHALL answer in this order: refuse a status or a content type it cannot send; build the headers; construct the outgoing response and set its status; take its body; set the outparam with the response; write the body to the body's stream in chunks of at most 4096 bytes, the most `blocking-write-and-flush` accepts per call (`webcpp::wasi::p2::http::detail::write_all`), dropping the stream afterwards; and finish the body with no trailers. When the response's body cannot be taken, the handler MUST drop the response and answer an internal error. Once the outparam is set nothing can be taken back: a body not written whole is dropped without `finish`, which `wasi:http` reports as a failed response. This is guarantees 4 and 6 on wasip2.

#### Scenario: A body of three chunks and some bytes arrives whole
- **WHEN** `http_long_body` is sent `GET /long`, its function answering a body of 192 lines `line <n> ` padded with `.` to 63 characters and a newline, then the line `end of the body.`, 3 x 4096 + 17 bytes in all
- **THEN** the answer is `HTTP/1.1 200 OK` with `content-type: text/plain` and the body whole, every numbered line of it, equal to `test/http_long_body.expected`, on wasip2

### Requirement: Entry point: wasip3 writes until the stream has taken everything
On wasip3 the handler SHALL answer by returning the response, with its status and headers, from the handler (`exports_wasi_http_handler_handle_return`), then streaming its body through a `stream<u8>` writer and resolving the trailers future. `webcpp::wasi::p3::http::detail::write_all` MUST write until the stream has taken every byte, however many writes that takes, waiting inside the task with `webcpp::wasi::p3::http::detail::wait_for` (a waitable set joined to the writer, `webcpp_wasi_http_waitable_set_wait`, the join undone, the set dropped) when a write reports `WEBCPP_WASI_HTTP_WAITABLE_STATUS_BLOCKED`, consuming the count of each completed write, and stopping when the state is not `COMPLETED`; it answers false when the reader went away before the bytes were taken. The trailers future is resolved with no trailers when the body was written whole, and with an internal error when it was not, which is how `wasi:http` 0.3 tells the host the body is incomplete (guarantee 6). The future of whether the host sent the response is dropped unread.

#### Scenario: The long body arrives whole on wasip3
- **WHEN** `http_long_body` is served on wasip3 and sent `GET /long`
- **THEN** the body arrives whole, equal to `test/http_long_body.expected`; wasmtime 47 takes the whole body in one write on wasip3, a body of 1 MiB too, so the test holds that the body arrives whole, not that the loop takes a second turn

### Requirement: Entry point: wasip3 never resumes in its callback
The wasip3 handler SHALL be lifted asynchronously, MUST wait inside the task for each write that blocks and MUST always end by returning `WEBCPP_WASI_HTTP_CALLBACK_CODE_EXIT`, both from `webcpp::wasi::p3::http::detail::handle` and from the exported callback, because context slot 0 of the component model belongs to wasi-libc on wasip3 and a handler that resumed in its callback would need it (using it trapped, measured). This is guarantee 5.

#### Scenario: Every component served on wasip3 answers
- **WHEN** `http_long_body` and every other served component are built for `wasm32-wasip3` and served through their run scripts, whose wasmtime line carries `-S p3 -W component-model-async`
- **THEN** they answer without trapping

### Requirement: Entry point: Failures are reported the way the version defines
A failure found before the response is committed SHALL answer an internal error (`refuse` with `WASI_HTTP_TYPES_ERROR_CODE_INTERNAL_ERROR` and no message), on wasip2 through the outparam and on wasip3 through the handler's return; one found after it MUST be reported the way each version defines: on wasip2 the body is dropped without `finish`, on wasip3 the trailers future is resolved with an internal error. No test can make a write fail on demand: this is held by review (guarantee 6).

#### Scenario: A refusal before the commit
- **WHEN** the program's function returns `{.status = 600}`
- **THEN** no part of the function's response is sent and the host answers its own `500 Internal Server Error`

### Requirement: Entry point: The handler catches nothing
The handler SHALL throw, try and catch nothing, and wasi MUST need neither exceptions nor RTTI and impose neither. An exception that escapes the function traps the instance, since webcpp builds every program with exceptions on wasip2 and wasip3: wasmtime 47 prints the trap and its backtrace on its standard error (`thrown Wasm exception` among them), answers the client with its own `500 Internal Server Error` HTML page, and keeps serving the requests after it. A function that may throw catches what it can answer itself.

#### Scenario: The documentation says what an escaping exception does
- **WHEN** the page's "Your function" chapter, `AGENTS.md`, the Doc Comment of `WEBCPP_WASI_HTTP_ENTRY_POINT` and the README are read
- **THEN** each states that the handler catches nothing and that an exception escaping the function traps the instance, wasmtime 47 answering the client with its own `500 Internal Server Error`

### Requirement: Entry point: Allocation
The handler's own code SHALL allocate nothing: it views the request's strings where they are, and the response's content type where the function put it. Two things allocate, each through its language's default: the request's strings (the target, and an extension method's name), which the bindings copy into the component's memory through the `cabi_realloc` they define (wasi-libc's `realloc`) and the handler frees once it has written the response; and the response's `body`, a `std::string`, through `std::allocator` once it outgrows the 10 characters it holds in itself on wasm32. A program cannot give wasi an allocator of its own.

#### Scenario: The page states what allocates
- **WHEN** the "What it allocates" section of the page's "Limits" chapter is read
- **THEN** it names exactly those two allocations and says the function's own are the program's

### Requirement: Entry point: Limits
The handler SHALL hand the function only a `request`, the method and the target; it MUST NOT read any request header or request body, and it sends no response header but `content-type`. A function that needs a request's body or its headers, or to set other headers, cannot be written with wasi today. It does not parse the target, route, decode, stream a body or send trailers of the main's.

#### Scenario: The limits are documented
- **WHEN** the page's "Limits" chapter and the README's "Limits" section are read
- **THEN** both state that the handler reads no request header and no request body and sends no response header but `content-type`

### Requirement: Guarantees: Nine numbered promises, each held
The page SHALL state nine guarantees, each keeping its number and the anchor `wasi-invariant-<n>` so that a comment of the code or of a test cites it as `(doc: #wasi-invariant-<n>)`, each with the test or the review that holds it: (1) the entry point compiles only for WASI and a component exports the handler of exactly one version; (2) the header defines no function of its own that a program links; (3) nothing written back points into freed memory; (4) every byte of the body is written; (5) on wasip3 the handler never resumes in its callback; (6) a failure is reported the way the version defines; (7) the response carries a `content-type` header only when `content_type` is not empty, and no other header; (8) wasi names nothing of the programs that use it; (9) a response that cannot be sent as the function wrote it is refused, not altered. `b2 libs/wasi/doc`, which builds the page, MUST fail on a citation that names no anchor of the page.

#### Scenario: A citation of the code resolves
- **WHEN** `p2/http.hpp` cites `(doc: #wasi-invariant-3)` and `(doc: #wasi-invariant-9)`, and `p3/http.hpp` the same
- **THEN** the page has both anchors, and the build of the page fails if either is removed

#### Scenario: Each guarantee names its holder
- **WHEN** the guarantees chapter is read
- **THEN** guarantee 1 is held by `target_namespace`, `target_override_p2`, `target_override_p3`, `target_refused`, the headers compiled alone and every served component; 2 by `http_two_units`; 3 by `http_content_type_of_target` and `response`; 4 by `http_long_body`; 5 by `http_long_body` on wasip3, every component served there, and review; 6 by review; 7 by `http_no_content_type`; 8 by the lint's rule on the banned word and review; 9 by `http_bad_status` and `http_bad_content_type`

### Requirement: Config: WEBCPP_WASI_NO_EXCEPTIONS
`<webcpp/wasi/config.hpp>` SHALL define `WEBCPP_WASI_NO_EXCEPTIONS` by the two lines webcpp::config prescribes for a library, `#if !defined(WEBCPP_HAS_EXCEPTIONS) && !defined(WEBCPP_WASI_NO_EXCEPTIONS)` then `#define WEBCPP_WASI_NO_EXCEPTIONS`, so that it is defined when it is not already and webcpp has no exceptions, and it MUST keep a definition the developer made for wasi alone. `WEBCPP_HAS_EXCEPTIONS` is not defined when `WEBCPP_NO_EXCEPTIONS` is (the developer disables the exceptions of every webcpp library), when `BOOST_NO_EXCEPTIONS` is (by the build or by Boost.Config), or when the compiler has no exceptions: `<webcpp/config.hpp>`, included first, tells it, and wasi no longer reads the compiler's macros itself. wasi raises no exception of its own and its handler catches none, so the macro changes nothing in it; it exists so a program can set it for every webcpp library alike, and an exception wasi raised would go through `boost::throw_exception`. A public header MUST NOT contain a bare `throw`, even in a template nothing instantiates (the lint refuses it). For MrDocs the macro is defined and undefined at once under `__MRDOCS__`, so the reference lists it.

#### Scenario: Defined exactly when the build has no exceptions
- **WHEN** `config_test.cpp`, built as the `config` test with no definition, includes `<webcpp/wasi/config.hpp>`
- **THEN** it compiles natively, on wasip2 and on wasip3, `WEBCPP_WASI_NO_EXCEPTIONS` is defined if and only if `WEBCPP_HAS_EXCEPTIONS` is not, and `BOOST_NO_EXCEPTIONS` and `WEBCPP_WASI_NO_EXCEPTIONS` are both defined or both undefined

#### Scenario: A developer defines it for wasi alone
- **WHEN** `config_developer_defined` builds `config_test.cpp` with `<define>WEBCPP_WASI_NO_EXCEPTIONS` and `<define>WEBCPP_TEST_WASI_DEVELOPER_DEFINED`, in a build that has exceptions
- **THEN** it compiles, `WEBCPP_WASI_NO_EXCEPTIONS` still defined (`#error "config.hpp drops WEBCPP_WASI_NO_EXCEPTIONS, which the developer defined"` otherwise), natively and on both targets

#### Scenario: A developer disables every webcpp library's exceptions
- **WHEN** `config_all_off` builds `config_test.cpp` with `<define>WEBCPP_NO_EXCEPTIONS` and `<define>WEBCPP_TEST_WASI_ALL_OFF`, in a build that has exceptions
- **THEN** it compiles, `WEBCPP_WASI_NO_EXCEPTIONS` defined (`#error "WEBCPP_NO_EXCEPTIONS does not define WEBCPP_WASI_NO_EXCEPTIONS"` otherwise), natively and on both targets

### Requirement: Build: The b2 targets
`build.jam` SHALL declare `project /webcpp/wasi` and these targets: the alias `wasi`, whose usage requirements are `<include>include`, `<library>/webcpp/config//config` and `<library>/boost//headers`, the headers alone; `http-bindings-p2` and `http-bindings-p3`, declared with `webcpp.wit-bindings` from `wit/http-p2.wit` and `wit/http-p3.wit`, world `http`, renamed `webcpp_wasi_http`, for the version `p2` and `p3` (the second with `--async "wasi:http/handler@0.3.0#handle"`); the alias `http-bindings` of the two, which brings the bindings of the target a program is built for and nothing natively; and the alias `http`, whose usage requirements are `<library>wasi` and `<library>http-bindings` and no define: the compiler chooses the version, and `WEBCPP_WASI_TARGET` overrides it. A program that exports the HTTP handler and nothing else links `/webcpp/wasi//http` and is done; it needs no compiled library of wasi's, since there is none.

#### Scenario: The example's Jamfile
- **WHEN** `example/Jamfile` declares `webcpp.targets wasip2 wasip3 ;` and `webcpp.serve http_hello.cpp : <library>/webcpp/wasi//http ;`
- **THEN** the component builds for both targets, with the version's bindings generated and linked and no version macro defined, and is served through its run script and compared with `http_hello.expected`

#### Scenario: A native build needs no WASI tool
- **WHEN** `b2 libs/wasi/test libs/wasi/example` is run natively
- **THEN** only the tests that need no WASI toolchain are built and run (`response`, `request`, `config`, `config_developer_defined`, `config_all_off`, `function_native`, `capabilities`, the native headers-alone tests), and none of wit-bindgen, the WIT or wasmtime is looked for

### Requirement: Build: The standard worlds
`wit/http-p2.wit` SHALL declare `package webcpp:wasi-http;` and `world http { export wasi:http/incoming-handler@0.2.12; }`, and `wit/http-p3.wit` the same package and `world http { export wasi:http/handler@0.3.0; }`: each exports the HTTP handler and nothing else. The bindings are generated into the build directory, never committed and never edited. A program whose world exports more than the handler uses `/webcpp/wasi//wasi` (the headers alone), generates its own world's bindings with `webcpp.wit-bindings` renamed `webcpp_wasi_http` too, and takes its version from the compiler, or from `WEBCPP_WASI_TARGET`.

#### Scenario: The world renamed
- **WHEN** wit-bindgen's C generator is run on a world with `--rename-world webcpp_wasi_http`
- **THEN** the header it writes is `webcpp_wasi_http.h`, and the names the entry point uses (`webcpp_wasi_http_string_t`, `webcpp_wasi_http_list_u8_t`, `WEBCPP_WASI_HTTP_WAITABLE_STATUS_BLOCKED`, `WEBCPP_WASI_HTTP_CALLBACK_CODE_EXIT`) are the same whatever the world is called

### Requirement: Build: What a build provides the header
A build SHALL provide the entry point with these, listed in the page's "What your build provides": the bindings generated with `--rename-world webcpp_wasi_http`; `webcpp_wasi_http.c` compiled as C for the target and `webcpp_wasi_http_component_type.o`, both linked (the glue that calls the handler and the world's type, which the component linker reads); the directory of the bindings on the include path, with the include directories of wasi, of webcpp::config and of Boost; and `-mexec-model=reactor` on the link, since the program exports a handler and has no C `main` (wasmtime 47 also serves the same program linked as a command, but webcpp builds and tests reactors; `webcpp.serve` adds the flag itself). It provides no macro that names the version: the compiler's `__wasip2__` or `__wasip3__` chooses it, and a build that wants the other defines `WEBCPP_WASI_TARGET`. The host has its part, which the run script of the component holds: `wasmtime serve` with `-S cli` on wasip2 (wasi-libc imports parts of `wasi:cli`), with `-S p3 -W component-model-async` on wasip3, and with `-W exceptions=y` for a component built with exceptions.

#### Scenario: A component of wasip3 is served with its flags
- **WHEN** a component built for `wasm32-wasip3` with exceptions is served through its script
- **THEN** the script's last line is `exec "$wasmtime" serve -S p3 -W component-model-async -W exceptions=y $WASMTIME_FLAGS "$@" --addr "$address" -- "$here/<component>.wasm"` and, for wasip2, `exec "$wasmtime" serve -S cli -W exceptions=y $WASMTIME_FLAGS "$@" --addr "$address" -- "$here/<component>.wasm"`

### Requirement: Build: By hand with wasi-sdk
The complete example SHALL build with wasi-sdk and wit-bindgen alone, without b2, from a directory that holds `main.cpp` (`example/http_hello.cpp`) and `wit/world.wit` (`package example:hello; world service { export wasi:http/incoming-handler@0.2.12; }` for wasip2, `wasi:http/handler@0.3.0` for wasip3) with `wit/deps/` linked to the `wasi:http` WIT. On wasip2: `"$WIT_BINDGEN" c --world service --rename-world webcpp_wasi_http --out-dir gen wit`; `"$WASI_SDK/bin/clang" --target=wasm32-wasip2 -c gen/webcpp_wasi_http.c -o gen/webcpp_wasi_http.o -Igen`; then `"$WASI_SDK/bin/clang++" --target=wasm32-wasip2 -std=c++20 -fwasm-exceptions -mllvm -wasm-use-legacy-eh=false -mexec-model=reactor -I"$WASI/include" -I"$CONFIG/include" -I"$BOOST_ROOT" -Igen main.cpp gen/webcpp_wasi_http.o gen/webcpp_wasi_http_component_type.o -fwasm-exceptions -lunwind -o hello.wasm`. On wasip3 the bindings are generated with `--async 'wasi:http/handler@0.3.0#handle'` beside `--rename-world`, everything is compiled for `--target=wasm32-wasip3`, the bindings' `.c` with `-pthread`, and the program with the same include directories; neither names the version, which the compiler chooses. `$CONFIG` is the directory of webcpp::config and `$BOOST_ROOT` the directory of Boost's headers.

#### Scenario: The script builds a component that answers as the example does
- **WHEN** the test `by_hand` runs `example/by_hand/build.sh <p2|p3> <component>` with `WASI_SDK`, `WIT_BINDGEN`, `WASI_WIT` and `BOOST_ROOT` in its environment, in the lane `http`, and serves what it builds
- **THEN** the component answers the requests of `http_hello` with its transcript, and the script exits 2 with `build.sh: <version> is neither p2 nor p3` for any other version

### Requirement: Tests: The served components
Every component that answers HTTP SHALL be tested with `webcpp.serve <source> : <library>/webcpp/wasi//http ;`, which builds it for wasip2 and wasip3, serves it through its run script, which runs `wasmtime serve` with the flags of the artifact, on a port the system chooses, sends it each line `<METHOD> <target>` of `<stem>.requests`, and passes when the transcript equals `<stem>.expected` byte for byte. The transcript writes each request as the curl command that sends it (`$ curl -i -X <METHOD> http://localhost:8080<target>`, each word quoted where a shell would read it otherwise, `curl -I` for `HEAD`, `--request-target '*'` before the bare origin for a target that is not a path), then the status line, the response's headers in lower case and sorted but for `date` and `transfer-encoding`, an empty line and the body as it came; two requests are separated by an empty line. Their sources are functions named by `WEBCPP_WASI_HTTP_ENTRY_POINT`, and their `.requests` and `.expected` are those of 0.1.0. `test/Jamfile` declares eight of them: `http_methods`, `http_long_body`, `http_no_content_type`, `http_two_units` (with `<source>http_two_units_answer.cpp`), `http_content_type_of_target`, `http_bad_status`, `http_bad_content_type` and `http_explicit`.

#### Scenario: A transcript mismatch fails with the difference
- **WHEN** a served component answers other than its `.expected` file holds
- **THEN** the test fails and shows the difference, and `--dump-tests` and `--out-xml` record it for the test matrix

### Requirement: Tests: The lane http
The served programs, in `test/` and in `example/`, SHALL be named by the lane `http` of their Jamfile (`webcpp.lane http : ... : wasip2 wasip3 ;`), which leaves them out of the ordinary lanes and makes them explicit targets. In `test/`: `http_methods http_long_body http_no_content_type http_two_units http_content_type_of_target http_bad_status http_bad_content_type http_explicit`; in `example/`: `http_hello by_hand boost_json_http`. A new served program joins its Jamfile's lane on both targets: `b2 declared-lanes`, and the CI's plan with it, stops naming one that no lane runs on a target it is served on. The ordinary lanes run the rest: `b2 -a toolset=clang-wasip2 testing.launcher=wasmtime libs/wasi/test libs/wasi/example`; the served ones: `b2 -a toolset=clang-wasip2 testing.launcher=wasmtime libs/wasi/test//http libs/wasi/example//http`; `toolset=clang-wasip3` likewise.

#### Scenario: The CI runs the lanes as one job per target
- **WHEN** the CI runs
- **THEN** it runs the lanes `http` of both directories as one job on each target, `Own lane (wasi, http, wasip2)` and the same for wasip3, and the test matrix shows the column `wasip2.wasi.http` (and `wasip3.wasi.http`)

### Requirement: Tests: Compile-time refusals and headers alone
`test/Jamfile` SHALL declare `webcpp.targets wasip2 wasip3 ;`; the headers-alone tests `webcpp.headers-alone wasi : ../include : wasi.hpp wasi/http.hpp : <library>/webcpp/wasi//http ;`, the same for `wasi/p2/http.hpp` with `<library>/webcpp/wasi//http-bindings-p2` on wasip2 and for `wasi/p3/http.hpp` with `<library>/webcpp/wasi//http-bindings-p3` on wasip3, and `webcpp.headers-alone wasi : ../include : wasi/config.hpp wasi/detail/response.hpp wasi/detail/request.hpp : : native wasip2 wasip3 ;`; and the compile-diagnostic test `target_refused` (`<define>WEBCPP_WASI_TARGET=p4`), which passes only on the guard's own error and fails on any other. The tests' own `.clang-tidy` switches off `bugprone-exception-escape.CheckMain`, so that the response test's `main` may let a `std::bad_alloc` end the process, and keeps the check on for everything else (a `noexcept` function, a destructor, a move or a swap).

#### Scenario: A guard error other than the expected one fails the test
- **WHEN** `target_refused.cpp` fails to compile with an error other than the guard's
- **THEN** the compile-diagnostic test fails

### Requirement: Docs: The page and its chapters
The library SHALL have a documentation page `doc/wasi.adoc`, built by `b2 libs/wasi/doc` (`webcpp.doc wasi : wasi.adoc ;`) into `doc/html/index.html` and published at `https://webcpporg.github.io/libs/wasi/`, that includes in this order `introduction.adoc` (introduction, a complete example, requirements, which name Boost and webcpp::config, reading this page), `function.adoc` ("Your function": the request it takes, the response it returns, its rules and the entry-point macro), `targets.adoc` ("Versions and targets"), `response.adoc`, `build.adoc` ("What your build provides", with b2 and by hand), `running.adoc` ("Running a program": the run scripts), `wasip3.adoc`, `flow.adoc` ("How a request flows"), `limits.adoc`, `guarantees.adoc`, `boost.adoc` ("Building for WASI with Boost"), `testing.adoc`, the reference and the appendix `changelog.adoc`. Every program the page shows was compiled for wasm32-wasip2 and wasm32-wasip3 and run by wasmtime, every answer it shows is what that program answered, and its listings are included from the files the tests run (tags `function`, `p2`, `p3`, `world`, `hello`, `lane`, `by-hand`, `boost`, `includes`, `text`, `catalog`). The chapter "Formatting it" of 0.1.0, which named the two macros of the main for clang-format, is gone with them.

#### Scenario: The page builds
- **WHEN** `b2 libs/wasi/doc` is run
- **THEN** it builds the page with the `attribute-missing: warn` setting and no warning, and fails on a citation of a guarantee that names no anchor, and on a `{webcpp-root}` tag that names no tagged region of a file the superproject tracks

### Requirement: Docs: The reference
The reference SHALL list every name the headers declare, in the namespaces `webcpp::wasi::p2::http`, `webcpp::wasi::p3::http` and `webcpp::wasi::detail`, and its macros, from the Doc Comments of the headers, each with its declaration, what it does and what it asks of its caller; every public symbol (`response` and its three members, `request` and its two, `WEBCPP_WASI_HTTP_ENTRY_POINT`, `WEBCPP_WASI_P2_HTTP_ENTRY_POINT`, `WEBCPP_WASI_P3_HTTP_ENTRY_POINT`, `WEBCPP_WASI_TARGET`, `WEBCPP_WASI_TARGET_P2`, `WEBCPP_WASI_TARGET_P3`, `WEBCPP_WASI_HAS_SOCKETS`, `WEBCPP_WASI_HAS_THREADS`, `WEBCPP_WASI_HAS_EXCEPTIONS`, `WEBCPP_WASI_NO_EXCEPTIONS`) MUST have a Doc Comment, and MrDocs builds the reference strict. `webcpp.reference wasi` parses the front natively with the bindings of wasip2 and `WEBCPP_WASI_TARGET=p2` (so `webcpp::wasi::http` shows wasip2's), and checks the Doc Comments of wasip3's header too, with the bindings of wasip3 and `WEBCPP_WASI_TARGET=p3`. `doc/mrdocs.yml` sets `sort-members: false`.

#### Scenario: The reference is read with the bindings of wasip2
- **WHEN** the reference is built
- **THEN** it is parsed with `/webcpp/wasi//http-bindings-p2-headers` and `<define>WEBCPP_WASI_TARGET=p2`, and the wasip3 header is checked with `/webcpp/wasi//http-bindings-p3-headers` and `<define>WEBCPP_WASI_TARGET=p3`

### Requirement: Docs: The README's example is a test
The README SHALL open with `webcpp::wasi`'s description and a table of its headers (`<webcpp/wasi.hpp>`, `<webcpp/wasi/http.hpp>`, `<webcpp/wasi/p2/http.hpp>`, `<webcpp/wasi/p3/http.hpp>`, `<webcpp/wasi/detail/response.hpp>`, `<webcpp/wasi/detail/request.hpp>`, `<webcpp/wasi/config.hpp>`), its requirements (C++20, Boost, webcpp::config, wasi-sdk 34, wit-bindgen 0.62.0 measured, the `wasi:http` WIT, a host such as wasmtime 47.0.3 measured), and the sections "A complete example", "Building with b2", "Your function", "The response", "Versions and targets", "What your build provides", "Running a program", "wasip3", "How a request flows", "Limits", "Building and testing", "Documentation" and "License". Its `main.cpp` and its answers MUST be copies of the region `function` of `example/http_hello.cpp` and of `example/http_hello.expected`, opened with `<!-- include::... -->` comments that doc-check compares; its world and its commands MUST be copies of `example/by_hand/p2/wit/world.wit` and of the region `p2` of `example/by_hand/build.sh`. A change to either changes the README in the same commit.

#### Scenario: The README drifts from the example
- **WHEN** `example/http_hello.expected` changes and the README's copy does not
- **THEN** doc-check fails

### Requirement: Project: Metadata, repository files and CI
The repository SHALL carry `meta/libraries.json` with `"key": "wasi"`, `"name": "wasi"`, `"authors": ["R"]`, the description "Header-only helpers for C++ built as WebAssembly components: the HTTP handler a component exports on wasip2 and wasip3, and the one macro that names the program's function.", `"category": ["System"]`, `"maintainers": ["R <github.overboard312@passmail.net>"]`, `"cxxstd": "20"` and `"port-of": null`; `LICENSE_1_0.txt`; `.gitattributes` (`* text=auto eol=lf`, `*.bat text eol=crlf`); `.clang-format` inheriting the superproject's (`BasedOnStyle: InheritParentConfig`) and nothing more, since no macro opens a function's body any more; and `.github/workflows/ci.yml`, which on push to `main`, on pull requests and on `workflow_dispatch` calls the superproject's reusable workflow `webcpporg/webcpp/.github/workflows/library.yml@main` with `library: wasi`, with `permissions: contents: read` and a concurrency group that never cancels a run on `main`. The `AGENTS.md` holds only what is specific to wasi and defers to the superproject's.

#### Scenario: The CI is the superproject's
- **WHEN** a commit is pushed to `main`
- **THEN** `ci.yml` builds it inside a checkout of webcpp on every lane wasi declares and on its own lanes, with the page, the lint and the test matrix

### Requirement: Project: Rules of the repository
The repository's `AGENTS.md` SHALL state the rules that bind a change: wasi's headers include only the standard library, Boost, webcpp::config and the bindings; the front and the shared names compile everywhere, a version's handler only for WASI, and `detail/response.hpp` and `detail/request.hpp` include nothing of the component ABI; the standard world exports the HTTP handler and nothing else and its bindings are generated into the build directory; a program names its function once with `WEBCPP_WASI_HTTP_ENTRY_POINT`; each version's handler is in a header of its own with no `#if` inside its logic, and the one choice between them is made in `http.hpp`; a namespace exists per version only where the implementation differs; wasip3's handler never resumes in its callback; the headers throw, try and catch nothing, and the handler catches nothing by the owner's decision; `config.hpp` comes first in every public header; every served program is declared with `webcpp.serve` or `webcpp.serve-script`, named by the lane `http`, and run through its run script; the README's example is a test; and the page's configuration is included by tag and doc-check accepts only a tagged region of a tracked file.

#### Scenario: A served program outside its lane is refused
- **WHEN** a program declared with `webcpp.serve` is added to a Jamfile without joining the lane `http` on both targets
- **THEN** `b2 declared-lanes` and the CI's plan stop naming a lane that does not run it

## REMOVED Requirements

### Requirement: Library: Header-only library in its namespace
**Reason**: The library no longer has one namespace, `webcpp::wasi::http`: its public names are in one namespace per WASI version, `webcpp::wasi::p2` and `webcpp::wasi::p3`, with what both share in `webcpp::wasi::detail`, its headers are other and its include guards too, and it no longer stands alone, since it includes webcpp::config and Boost. The scenario "The library stands alone" is false.
**Migration**: Replaced by "Library: Header-only library in one namespace per WASI version", "Library: Depends on webcpp::config and Boost" and "Versions: The configured version's namespace is inline". `webcpp::wasi::http::response` keeps its name in the configured version, by the inline namespace; `<webcpp/wasi/http/entrypoint.hpp>` and `<webcpp/wasi/http/response.hpp>` become `<webcpp/wasi/http.hpp>`.

### Requirement: Entry point: One version of WASI, chosen by a macro
**Reason**: The build no longer defines `WEBCPP_WASI_HTTP_P2` or `WEBCPP_WASI_HTTP_P3` and the header no longer stops under neither or both: the version follows the compiler's `__wasip2__` and `__wasip3__`, and `WEBCPP_WASI_TARGET` overrides it, as `BOOST_PROCESS_VERSION` does. Both scenarios, "Neither macro is defined" and "Both macros are defined", are false.
**Migration**: Replaced by "Target: The version follows the compiler" and "Target: WEBCPP_WASI_TARGET overrides the compiler". A build deletes its `<define>WEBCPP_WASI_HTTP_P2` and `<define>WEBCPP_WASI_HTTP_P3` and defines `WEBCPP_WASI_TARGET=p2` or `=p3` only to choose a version the compiler does not name.

### Requirement: Entry point: The main and its two macros
**Reason**: The program no longer defines `webcpp::wasi::http::http_main` between `WEBCPP_WASI_HTTP_MAIN_BEGIN` and `WEBCPP_WASI_HTTP_MAIN_END`: it writes an ordinary function of a `request` and names it once with `WEBCPP_WASI_HTTP_ENTRY_POINT(fn)`, which has no ABI and compiles natively. This is a breaking change, made before webcpp's first release and listed in its release notes.
**Migration**: Replaced by "Entry point: A plain function and one macro", "Entry point: The explicit macro of each version" and "Entry point: The request". `WEBCPP_WASI_HTTP_MAIN_BEGIN(method, target) body WEBCPP_WASI_HTTP_MAIN_END()` becomes `response fn(request const& request) { body }` with `request.method` and `request.target` for the two parameters, followed by `WEBCPP_WASI_HTTP_ENTRY_POINT(fn)` at global scope; `WEBCPP_WASI_HTTP_EXPORTS` is gone.
