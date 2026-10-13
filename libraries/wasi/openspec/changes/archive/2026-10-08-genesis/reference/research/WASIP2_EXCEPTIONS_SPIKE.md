---
title: "C++ exceptions on wasip2 with wasmtime"
tags: [webcpp, wasi, exceptions, spike]
status: active
created: 2026-10-08
---

# C++ exceptions on wasip2 with wasmtime

## Summary

- **wasmtime 47.0.3 runs wasip2 programs built with C++ exceptions, as a
  command (`wasmtime run`) and as an HTTP component (`wasmtime serve -S cli`),
  with its default flags.** No `-W exceptions=y` is needed: wasmtime 47 is
  the first release with the exception-handling proposal on by default
  (release notes, #13603). `-W exceptions=n` makes it refuse the program.
- **The flags are the ones the wasip3 lane already uses:** compile with
  `-fwasm-exceptions -mllvm -wasm-use-legacy-eh=false`, link with
  `-fwasm-exceptions -lunwind`. wasi-sdk 34 ships an `eh` build of libc++,
  libc++abi and libunwind for wasm32-wasip2 (`share/wasi-sysroot/lib/wasm32-wasip2/eh/`),
  encoded with exnref. Both link flags are required.
- **The legacy encoding does not work with wasmtime 47 and cannot be made
  to work:** wasmtime has no legacy-exceptions support (no `-W` option, no
  `Config` method), and the shipped `eh` libraries are exnref anyway, so a
  legacy build is a mix of both encodings.
- **An exception escaping the main** traps with `thrown Wasm exception`:
  exit code 1 for a command (native would print the `what()` and abort with
  134), and for a component, wasmtime's own HTML 500 page, after which the
  server keeps serving. That is the same outcome as the `-fno-exceptions`
  build's `abort()`, which also answers that 500, but it says less: no type,
  no message.
- **Portability:** Spin 4.2.2 (wasmtime 49) serves the component unchanged.
  wasmCloud 2.10 (wasmtime 48, `gc` feature) enables it by default, per its
  source code (not run). jco 1.37.0 transpiles and runs it on Node 22, 24 and 26
  and on Bun, but only with `jco transpile --bindgen-enable-wasm-exnref`;
  `jco run` and `jco serve` lack that option and refuse the component. All
  current browsers ship exnref (Chrome 137, Firefox 131, Safari 18.4).
  wasmtime 37 to 46 need `-W exceptions=y`. Wasmer, WAMR, wazero and
  WasmEdge do not run wasip2 components at all, with or without exceptions.
- **Cost:** +21% stripped size for http_hello (+10.6 KB, +4.2 KB gzipped),
  +18% for the Boost.JSON component (+20.5 KB, +6.0 KB gzipped). Cold start
  of `wasmtime serve` is unchanged with the compilation cache (16.1 ms for both)
  and within 0.5 to 3 ms without it.
- **Verdict:** wasmtime does not need the restriction. Every host that runs a
  wasip2 component today also runs one with exnref exceptions, except jco's
  `run` and `serve` commands (they need a one-line option) and wasmtime
  older than 47 (it needs a flag). The restriction costs webcpp Boost.Locale
  (17 of 28 sources do not compile), a second Boost configuration, the
  `throw_exception` handler and a wasip2-only code path in every header. It
  should be lifted, unless webcpp wants to support jco's CLI or wasmtime
  older than 47 without flags.

## 1. Why the restriction exists

- `~/dev/webcpp/Jamroot` lines 249-256: "A library that declares wasip2
  builds there without exceptions, a portability policy: some wasm hosts lack
  exception handling." Then `<toolset>clang-wasip2:<exception-handling>off`
  and `<define>BOOST_NO_EXCEPTIONS`. Lines 257-264 give wasip3
  `-fwasm-exceptions -mllvm -wasm-use-legacy-eh=false` and
  `-fwasm-exceptions -lunwind`.
- `~/.buzz/PLANS/WEBCPP_DESIGN.md` section 4, "Exceptions": "This is a
  portability policy, not a toolchain limit. Measured on 2026-10-07, wasi-sdk
  34 and wasmtime 47.0.3 run exceptions and RTTI on both wasip2 and wasip3.
  [...] The limit is in hosts that lack wasm exception handling."
- Nothing in the design, the work logs (`WEBCPP_STAGE1_LOG.md`,
  `WEBCPP_STAGE2_LOG.md`, `WEBCPP_STAGE3_LOG.md`) or the Jamroot names which
  hosts. The owner decision of 2026-10-08 (`WEBCPP_STAGE2_LOG.md` line 115)
  kept "without exceptions" on wasip2 only, as the one place a portability
  concern was assumed. This spike tests that assumption.
- `~/.buzz/RESEARCH/JSON_SCHEMA_ENGINE_SPIKE.md` section 5: Boost.Locale does
  not compile for wasip2 under `-fno-exceptions` (17 of 28 sources fail), and
  compiles and runs on wasip2 with wasm exceptions. That spike ran wasmtime
  with `-W exceptions=y`, which wasmtime 47 does not need (section 2 below).

## 2. A wasip2 command with exceptions (`wasmtime run`)

Spike directory: `S=~/.claude/jobs/fcf3a902/tmp/spike-wasip2-eh`,
`SDK=~/dev/webcpp/.local/wasi-sdk` (wasi-sdk 34, clang 23.1.0, wasi-libc
2e6fb9d8ee0c), wasmtime 47.0.3 (5554cc1a6 2026-07-31), wasm-tools 1.259.0.

`$S/cmd/eh_command.cpp` runs five cases:

1. throws and catches a `std::runtime_error` in one frame;
2. throws from `depth3` through `depth2` and `depth1`, each with a local that
   has a destructor (`std::string`, `std::vector`), caught as
   `std::exception` in main;
3. `std::vector::at` out of range, caught as `std::out_of_range`;
4. `throw 42`, `catch (...)`, `throw;`, caught as `int`;
5. `throw std::logic_error("escapes main")`, uncaught.

### 2.1 Build

```
$SDK/bin/clang++ --target=wasm32-wasip2 -std=c++20 -O2 \
    -fwasm-exceptions -mllvm -wasm-use-legacy-eh=false eh_command.cpp -lunwind -o eh_exnref.wasm
# rc=0, 941382 bytes

$SDK/bin/clang++ --target=wasm32-wasip2 -std=c++20 -O2 \
    -fwasm-exceptions eh_command.cpp -lunwind -o eh_legacy.wasm
# rc=0, 941165 bytes (clang's default encoding is the legacy one)

$SDK/bin/clang++ --target=wasm32-wasip2 -std=c++20 -O2 \
    -fwasm-exceptions -mllvm -wasm-use-legacy-eh=false eh_command.cpp -o eh_nounwind.wasm
wasm-ld: error: .../eh_command-e8bda1.o: undefined symbol: __cpp_exception
wasm-ld: error: .../eh_command-e8bda1.o: undefined symbol: __wasm_lpad_context
wasm-ld: error: .../eh_command-e8bda1.o: undefined symbol: _Unwind_CallPersonality
...
# rc=1: -lunwind is required

$SDK/bin/clang++ --target=wasm32-wasip2 eh.o -lunwind -o x.wasm      # object built with EH
wasm-ld: error: unable to find library -lunwind
# rc=1: -fwasm-exceptions is required at link too; it adds -L.../lib/wasm32-wasip2/eh

$SDK/bin/clang++ --target=wasm32-wasip2 -std=c++20 -O2 eh_command.cpp -o eh_default.wasm
wasm-ld: error: .../eh_command-5528dd.o: undefined symbol: __cxa_allocate_exception
wasm-ld: error: .../eh_command-5528dd.o: undefined symbol: __cxa_throw
# rc=1: with no flag, clang still accepts throw and try (it predefines
# __cpp_exceptions and __EXCEPTIONS), but links the noeh libc++abi
```

The link line `-fwasm-exceptions -lunwind` selects the `eh` libraries:

```
$SDK/bin/clang++ ... -fwasm-exceptions ... -lunwind -o eh_exnref.wasm -### | grep wasm-ld
"-L.../wasi-sdk/bin/../share/wasi-sysroot/lib/wasm32-wasip2/eh"
"-L.../wasi-sdk/bin/../share/wasi-sysroot/lib/wasm32-wasip2"
... "-lunwind" "-lc++" "-lc++abi"

ls $SDK/share/wasi-sysroot/lib/wasm32-wasip2/eh $SDK/share/wasi-sysroot/lib/wasm32-wasip2/noeh
eh:   libc++.a libc++.modules.json libc++abi.a libc++experimental.a libunwind.a llvm-lto
noeh: libc++.a libc++.modules.json libc++.so libc++abi.a libc++abi.so libc++experimental.a llvm-lto
```

### 2.2 Encoding

```
wasm-tools validate eh_exnref.wasm            # valid
wasm-tools print eh_exnref.wasm | grep -c try_table    # 38
wasm-tools print eh_exnref.wasm | grep -c throw_ref    # 20
legacy try/delegate/rethrow                            # 0

wasm-tools validate eh_legacy.wasm
error: func 27 failed to validate
Caused by:
    0: legacy_exceptions feature required for try instruction (at offset 0xe6e)
# 34 legacy try/rethrow/delegate in user code, plus 20 try_table and 6 throw_ref
# from the eh libraries: a mix of both encodings
```

The legacy build is a mix because wasi-sdk 34 ships the `eh` libraries in
the exnref encoding only. A pure legacy program would need libc++abi and
libunwind rebuilt with `-mllvm -wasm-use-legacy-eh=true`, and no host in
section 4 needs that.

### 2.3 Run

| Binary | `wasmtime run` (default) | `-W exceptions=y` | `-W exceptions=n` |
| --- | --- | --- | --- |
| `eh_exnref.wasm` | cases 1-4 pass, case 5 traps, exit 1 | same | refused: "exception refs not supported without the exception handling feature (at offset 0x723)" |
| `eh_legacy.wasm` | refused: "legacy_exceptions feature required for try instruction" | same | refused: "exceptions proposal not enabled" |

There is no option for the legacy form:
`wasmtime run -W exceptions=y,legacy-exceptions=y` gives
`error: unknown -W / --wasm option: legacy-exceptions` (exit 2), and
`wasmtime run -W help` lists only `exceptions[=y|n]`.

```
$ wasmtime run eh_exnref.wasm; echo exit=$?
case 1 caught: plain
case 2 caught: from depth3 n=1
case 3 caught std::out_of_range: vector
case 4 inner caught, rethrowing
case 4 outer caught int 42
Error: failed to run main module `eh_exnref.wasm`

Caused by:
    0: failed to invoke `run` function
    1: error while executing at wasm backtrace:
    0:   0x1619 - eh_exnref.wasm!_Unwind_RaiseException
    1:   0x342a - eh_exnref.wasm!__cxa_throw
    2:   0x12b4 - eh_exnref.wasm!main
    3:   0x423c - eh_exnref.wasm!__main_void
    4:    0xe48 - eh_exnref.wasm!_start
note: using the `WASMTIME_BACKTRACE_DETAILS=1` environment variable may show more debugging information
    2: thrown Wasm exception

exit=1
```

The same at `-O0 -g` (`eh_exnref_O0.wasm`): identical output, exit 1.

**The escaping exception.** Native clang prints
`libc++abi: terminating due to uncaught exception of type std::logic_error: escapes main`
and aborts with 134. On wasm, libunwind's `_Unwind_RaiseException` throws a
wasm exception that nothing catches, so `std::terminate` never runs: the host
reports `thrown Wasm exception` and exits 1, without the type or the
`what()`. For comparison, the `-fno-exceptions` build of a failing
`vector::at` (`at_noexc.wasm`) prints libc++'s
`out_of_range was thrown in -fno-exceptions mode with message "vector"`, traps
on `unreachable` in `abort`, and exits 134.

## 3. A wasip2 component with exceptions (`wasmtime serve`)

### 3.1 The b2 commands

```
cd ~/dev/webcpp && env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH \
  b2 --user-config=.local/user-config.jam -n -a --build-dir=$S/bd toolset=clang-wasip2 libs/wasi/example
```

That builds only `boost_json_command`: the served examples are in the lane
`http`, so `libs/wasi/example//http_hello` was dry-run instead (debug, then
`variant=release`), and built for real into `$S/bd` to obtain the generated
bindings and the baseline. The wasip2 lane's commands (debug):

```
clang++ -std=c++20 -fno-exceptions --target=wasm32-wasip2 -O0 -fno-inline -Wall -Wextra -Werror -g \
  --system-header-prefix="boost/" -DBOOST_ALL_NO_LIB -DBOOST_NO_EXCEPTIONS -DWEBCPP_WASI_HTTP_P2 \
  -I$S/bd/webcpp/bin/generated/wasi/http-bindings-p2 -I/opt/homebrew/opt/boost/include -I libs/wasi/include \
  -c -o http_hello.o libs/wasi/example/http_hello.cpp
clang++ -o http_hello.wasm http_hello.o .../webcpp_wasi_http_component_type.o .../throw_exception.o \
  .../http-bindings-p2-c.o -std=c++20 -g --target=wasm32-wasip2 -mexec-model=reactor
python3 tools/component/serve.py ... --flags="-S cli" --component .../http_hello.wasm ...
```

Release is the same with `-O3 -Wno-inline -DNDEBUG` and no `-O0 -fno-inline -g`.

### 3.2 The edit

`-fno-exceptions` and `-DBOOST_NO_EXCEPTIONS` replaced with
`-fwasm-exceptions -mllvm -wasm-use-legacy-eh=false`, `throw_exception.o`
dropped, and `-fwasm-exceptions -lunwind` added to the link. The C bindings
object is reused as b2 built it (C, no exceptions involved).

```
G=$S/bd/webcpp/bin/generated/wasi/http-bindings-p2
BC=$S/bd/webcpp/bin/libs/wasi/clang-darwin-wasip2/debug/exception-handling-off/link-static/target-os-wasi/http-bindings-p2-c.o
$SDK/bin/clang++ -std=c++20 -fwasm-exceptions -mllvm -wasm-use-legacy-eh=false --target=wasm32-wasip2 \
  -O0 -fno-inline -Wall -Wextra -Werror -g --system-header-prefix="boost/" -DBOOST_ALL_NO_LIB \
  -DWEBCPP_WASI_HTTP_P2 -I$G -I/opt/homebrew/opt/boost/include -I$HOME/dev/webcpp/libs/wasi/include \
  -c -o eh_hello.o eh_hello.cpp
$SDK/bin/clang++ -o eh_hello.wasm eh_hello.o $G/webcpp_wasi_http_component_type.o $BC \
  -std=c++20 -g --target=wasm32-wasip2 -mexec-model=reactor -fwasm-exceptions -lunwind
```

`$S/comp/eh_hello.cpp` is http_hello with two more routes: `/catch` calls a
function that does `std::vector::at` out of range and catches
`std::out_of_range` in the main, answering 500 with its `what()`; `/escape`
throws `std::runtime_error` out of the main. It compiles with
`-Wall -Wextra -Werror`, as does `libs/wasi/example/http_hello.cpp` itself
with the same flags (`http_hello_eh.wasm`).

### 3.3 Validation

```
wasm-tools validate eh_hello.wasm             # valid
wasm-tools validate http_hello_eh.wasm        # valid
wasm-tools validate http_hello_noeh.wasm      # valid

wasm-tools validate --features=wasm2,component-model eh_hello.wasm
error: exception refs not supported without the exception handling feature (at offset 0xfd8)
wasm-tools validate --features=wasm3,component-model eh_hello.wasm          # valid
wasm-tools validate --features=wasm2,component-model http_hello_noeh.wasm   # valid
```

So the component with exceptions is a WebAssembly 3.0 component, and the
one without is a 2.0 one. That is the whole portability difference.

### 3.4 Serving

`$S/serve_probe.sh` starts `wasmtime serve -S cli [flags] --addr 127.0.0.1:0`,
as `tools/component/serve.py` does for p2, curls four routes and stops it.

```
$ serve_probe.sh eh_hello.wasm          # default flags
$ curl -sS -i http://127.0.0.1:PORT/hello
HTTP/1.1 200 OK
content-type: text/plain
transfer-encoding: chunked

hello from /hello

$ curl -sS -i http://127.0.0.1:PORT/catch
HTTP/1.1 500 Internal Server Error
content-type: text/plain
transfer-encoding: chunked

caught std::out_of_range: vector

$ curl -sS -i http://127.0.0.1:PORT/escape
HTTP/1.1 500 Internal Server Error
content-type: text/html; charset=UTF-8
content-length: 205

<!doctype html>
<html>
<head>
    <title>500 Internal Server Error</title>
</head>
<body>
    <center>
        <h1>500 Internal Server Error</h1>
        <hr>
        wasmtime
    </center>
</body>
</html>

$ curl -sS -i http://127.0.0.1:PORT/hello
HTTP/1.1 200 OK
content-type: text/plain
transfer-encoding: chunked

hello from /hello

--- wasmtime output:
Serving HTTP on http://127.0.0.1:49285/
worker failed: error while executing at wasm backtrace:
    0:   0x82be - eh_hello.wasm!_Unwind_RaiseException
    1:   0x9cf3 - eh_hello.wasm!__cxa_throw
    2:   0x2347 - eh_hello.wasm!webcpp::wasi::http::http_main(std::__2::basic_string_view<char, ...>, ...)
    3:   0x1c65 - eh_hello.wasm!webcpp::wasi::http::detail::handle(wasi_http_types_own_incoming_request_t, wasi_http_types_own_response_outparam_t)
    4:   0x1b43 - eh_hello.wasm!exports_wasi_http_incoming_handler_handle
    5:   0x8185 - eh_hello.wasm!__wasm_export_exports_wasi_http_incoming_handler_handle
note: using the `WASMTIME_BACKTRACE_DETAILS=1` environment variable may show more debugging information

Caused by:
    thrown Wasm exception

error: guest never invoked `response-outparam::set` method
```

- `-W exceptions=y`: identical transcript.
- `-W exceptions=n`: wasmtime does not start ("exception refs not supported
  without the exception handling feature (at offset 0xfd8)").

The `-fno-exceptions` counterpart (`$S/comp/noeh_abort.cpp`, where `/escape`
does the same `vector::at`, which libc++ turns into `abort()`) answers
exactly the same HTML 500 and keeps serving. The only difference is
stderr: there, wasmtime prints `out_of_range was thrown in -fno-exceptions
mode with message "vector"` and `wasm trap: wasm 'unreachable' instruction
executed`.

webcpp's own transcript test passes on the components built with exceptions
(release, stripped):

```
python3 tools/component/serve.py --flags="-S cli" --component http_hello_eh_rel.stripped.wasm \
  --requests libs/wasi/example/http_hello.requests --expected libs/wasi/example/http_hello.expected --output ...
# rc=0
python3 tools/component/serve.py --flags="-S cli" --component bjh_eh.stripped.wasm \
  --requests libs/wasi/example/boost_json_http.requests --expected libs/wasi/example/boost_json_http.expected --output ...
# rc=0  (boost_json_http with tools/boost_json.cpp, both built with wasm exceptions)
```

## 4. Portability

### 4.1 Standardization

- Exception handling with exnref is a **finished proposal (phase 5)**,
  approved by the WG on 2025-07-23, and part of **WebAssembly 3.0**
  ([finished-proposals.md](https://github.com/WebAssembly/proposals/blob/main/finished-proposals.md)).
- The legacy form (`try`/`catch`/`delegate`) is listed as phase "inactive"
  ([features.json](https://webassembly.org/features.json), behind
  [webassembly.org/features](https://webassembly.org/features/)).
- **wasmtime:** the proposal is Tier 1
  ([stability-wasm-proposals](https://docs.wasmtime.dev/stability-wasm-proposals.html);
  Winch excepted). It was implemented, off by default, in 37.0.0 ("Support is
  still disabled by default but is ready for testing", #11326). **It is on by
  default since 47.0.0** ("Wasmtime now has the WebAssembly
  exception-handling proposal enabled by default", #13603). 47.0.0 also turns
  uncaught wasm exceptions at component boundaries into traps (#13613). Notes:
  [release-37.0.0](https://github.com/bytecodealliance/wasmtime/blob/release-37.0.0/RELEASES.md),
  [release-47.0.0](https://github.com/bytecodealliance/wasmtime/blob/release-47.0.0/RELEASES.md).
  `Config::wasm_exceptions` is "`true` by default, except when using
  `Strategy::Winch`"; there is no legacy-exceptions method
  ([Config](https://docs.wasmtime.dev/api/wasmtime/struct.Config.html)).
- Measured with wasmtime 46.0.0, downloaded into `$S/wt46`:
  `wasmtime run eh_exnref.wasm` refuses it ("exception refs not supported
  without the exception handling feature"); `-W exceptions=y` runs cases 1-4
  and traps on 5, as 47 does.

### 4.2 Hosts

"exnref" is the encoding wasi-sdk 34 produces with
`-wasm-use-legacy-eh=false`, the only one wasmtime accepts. "Runs a p2
component" is the precondition: without it, exceptions do not matter.

| Host | Runs a wasip2 component? | exnref | legacy EH | Evidence |
| --- | --- | --- | --- | --- |
| wasmtime 47.0.3 | yes | **yes, default** | no | measured, sections 2 and 3 |
| wasmtime 37-46 | yes | yes, with `-W exceptions=y` | no | measured 46.0.0; release notes 37, 47 |
| Spin 4.2.2 (wasmtime 49.0.2) | yes | **yes, default** | no | measured, 4.3 |
| wasmCloud 2.10.3 (wasmtime 48.0.5) | yes | yes, default | no | not run; `Cargo.toml` pins wasmtime 48.0.5; `crates/wash-runtime` enables wasmtime's `gc` feature, and `engine/mod.rs` says "Exception handling. Enabled by default in wasmtime >= 47" |
| jco 1.37.0 `transpile` + Node / Bun | yes | yes, with `--bindgen-enable-wasm-exnref` | not tried | measured, 4.4; option added by [jco #1806](https://github.com/bytecodealliance/jco/pull/1806), merged 2026-08-03 |
| jco 1.37.0 `run`, `serve` | yes | **no**: no option, refused | not tried | measured, 4.4 |
| Node 26.7.0 (V8 14.6), 24.21.0 (V8 13.6), 22.23.3 (V8 12.4) | through jco | yes, default | yes (V8 flag on by default) | measured; features.json says Node 24.15, but 22.23.3 compiled and ran the module |
| Node 20.20.2 (V8 11.3, EOL) | through jco | no ("invalid value type 'noexternref'") | 17.0+ | measured; the no-exceptions command runs there |
| Bun 1.4.2 (JavaScriptCore) | through jco | yes | not tried | measured |
| Chrome / Edge | through jco | 137+ | 95+ | [features.json](https://webassembly.org/features.json) |
| Firefox | through jco | 131+ | 100+ | features.json |
| Safari | through jco | 18.4+ | 15.2+ | features.json |
| Deno | through jco | 2.3.2+ | 1.16+ | features.json (not run) |
| Wasmer 7.1.0 | **no** ("encoded as a component but the WebAssembly component model feature is not enabled", for the no-exceptions command too) | yes, core modules (6.0+; measured: a wasip1 build runs cases 1-4) | no | measured; features.json |
| WAMR 2.4.5 | no component model | no | yes, classic interpreter only, `WAMR_BUILD_EXCE_HANDLING=1`, off by default | [build_wamr.md](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/build_wamr.md): "supports only Legacy Wasm exception handling proposal" |
| wazero 1.12.0 | no component model | behind a flag | no | features.json |
| WasmEdge 0.18.0 | not yet: "Component Model completion" and "WASI preview2 as plugins" are on its Q4 2026 roadmap | 0.14.0+ | no | [WasmEdge #5362](https://github.com/WasmEdge/WasmEdge/issues/5362); features.json |

### 4.3 Spin, measured

Spin 4.2.2 (`gh release download v4.2.2 -R spinframework/spin`, in `$S/spin`),
with a manifest per component (`[[trigger.http]] route = "/..."`,
`[component.main] source = "../eh_hello.wasm"`), `spin up --listen 127.0.0.1:18440`:

| Route | `eh_hello.wasm` (exceptions) | `noeh_abort.wasm` (no exceptions) |
| --- | --- | --- |
| `/hello` | 200 `hello from /hello` | 200 `hello from /hello` |
| `/catch` | 500 `caught std::out_of_range: vector` | (no such route) 200 `hello from /catch` |
| `/escape` | 500, `content-length: 0`; log `ERROR spin_trigger_http::server: Error processing request: guest invocation failed` | the same 500 and log line, after libc++'s `out_of_range was thrown in -fno-exceptions mode` |
| `/hello` again | 200 | 200 |

### 4.4 jco, measured

jco 1.37.0, installed locally in `$S/jco` (`npm i @bytecodealliance/jco @bytecodealliance/preview2-shim`).

```
$ npx jco transpile eh_exnref.wasm -o out_eh
    exception refs not supported without the exception handling feature (at offset 0x723)
$ npx jco run eh_exnref.wasm
  [cause]: ComponentError: failed to translate component
      exception refs not supported without the exception handling feature (at offset 0x723)
$ npx jco serve --port 18431 eh_hello.wasm
(jco serve) Error: Unable to transpile command for execution
      exception refs not supported without the exception handling feature (at offset 0xfd8)

$ npx jco transpile eh_exnref.wasm --bindgen-enable-wasm-exnref -o out_eh   # succeeds
$ node run_eh.mjs        # imports out_eh/eh_exnref.js and calls run.run()
case 1 caught: plain
case 2 caught: from depth3 n=1
case 3 caught std::out_of_range: vector
case 4 inner caught, rethrowing
case 4 outer caught int 42
run threw: Exception [object WebAssembly.Exception]
```

The same output on Node 26.7.0, 24.21.0, 22.23.3 and Bun 1.4.2. Node 20.20.2:
`CompileError: WebAssembly.compile(): invalid value type 'noexternref'`.
The no-exceptions command (`noeh_cmd.wasm`) transpiles without the option and
runs on all of them, Node 20 included. `jco serve` serves the no-exceptions
`http_hello` (200).

The HTTP component, transpiled with `--bindgen-enable-wasm-exnref` and served by
preview2-shim's `HTTPServer` on one shared instance (`$S/jco/serve_eh.mjs`):
`/hello` 200, `/catch` 500 with its body, as wasmtime. `/escape` ends
the Node process (`Exception [WebAssembly.Exception] {}` thrown from
`node:internal/event_target`), so later requests are refused. The
no-exceptions component, served the same way (`serve_ab.mjs`), dies the same
way on its `abort()` (`RuntimeError: wasm trap: wasm 'unreachable' instruction
executed`). That is the shim running one instance without isolation, not
something exceptions add.

## 5. Cost

### 5.1 Size

All on wasip2, linked as the b2 lane links. "stripped" is
`wasm-tools strip -a`; gzip is `gzip -9`.

| Component | Build | raw | stripped | gzip of stripped |
| --- | --- | ---: | ---: | ---: |
| http_hello | debug (`-O0 -g`), no exceptions (b2's own output) | 886,148 | 63,578 | 23,628 |
| http_hello | debug, wasm exceptions | 1,060,212 | 74,240 | 27,889 |
| http_hello | release (`-O3 -DNDEBUG`), no exceptions (b2's own output) | 743,052 | 49,685 | 19,596 |
| http_hello | release, wasm exceptions | 934,568 | 60,321 | 23,828 |
| boost_json_http | release, no exceptions | 3,220,366 | 113,202 | 48,166 |
| boost_json_http | release, wasm exceptions | 3,416,855 | 133,688 | 54,170 |

Release, stripped: **+10.6 KB (+21%)** for http_hello and **+20.5 KB
(+18%)** for boost_json_http. Gzipped: +4.2 KB and +6.0 KB. The fixed part is
libunwind plus the libc++abi personality and the exnref libc++. The rest
grows with landing pads, so roughly with the amount of code that has
destructors on a throwing path.

### 5.2 Cold start

`$S/coldstart.py 10 <component> [flags]` starts `wasmtime serve -S cli
--addr 127.0.0.1:0`, waits for `Serving HTTP on`, sends one GET, and stops it.
Ten runs, median, in ms. The first number is "listening", the second is "first
response". The default flags use wasmtime's compilation cache;
`-C cache=n` compiles each time.

| Component (release, stripped) | default flags | `-C cache=n` |
| --- | --- | --- |
| http_hello, no exceptions | 15.4 / 16.1 | 24.3 / 25.0 |
| http_hello, wasm exceptions | 15.4 / 16.1 | 24.6 / 25.5 |
| boost_json_http (`GET /v1/items`), no exceptions | 16.9 / 17.7 | 26.0 / 26.8 |
| boost_json_http, wasm exceptions | 16.9 / 17.7 | 28.9 / 29.9 |

Raw runs, http_hello, first response, default flags: no exceptions
`38.1 16.5 16.0 16.3 16.3 15.9 15.7 15.9 16.3 15.9`; exceptions
`26.6 16.5 15.9 16.5 16.0 16.1 16.0 15.9 15.8 16.1`. The cost with a warm
cache is nothing measurable. Without the cache it is 0.5 ms for http_hello
and 3 ms for boost_json_http, which is Cranelift compiling the larger
module. Throughput per request was not measured. With exnref, a `try_table`
that does not throw does no work at run time.

## 6. Verdict

**Is "no exceptions on wasip2" necessary for wasmtime?** No. wasmtime 47
runs wasip2 commands and components built with exnref exceptions with its
default flags, and the flags are the ones the wasip3 lane already uses.
Only wasmtime 37 to 46 need `-W exceptions=y`. No wasmtime runs the legacy
form, and wasi-sdk 34 does not ship it.

**What does lifting it cost in portability?** Very little, because the hosts
without exnref cannot run a wasip2 component anyway:

- Every host that runs wasip2 components today runs this one: wasmtime 47+,
  Spin (measured), wasmCloud (from its source), and jco on Node 22+, Bun and
  current browsers.
- Two gaps remain:
  - jco needs `--bindgen-enable-wasm-exnref` at transpile, and its
    `run`/`serve` shortcuts cannot pass it in 1.37.0.
  - Embedders of wasmtime older than 47 must turn the proposal on.
- Wasmer, WAMR, wazero and WasmEdge run no wasip2 component, with or without
  exceptions. Of them, only WAMR would ever need the legacy form.
- Node 20, which fails on exnref, is end of life.
- The binary becomes a WebAssembly 3.0 component instead of a 2.0 one
  (section 3.3). That is the precise statement of the remaining risk: a
  future wasip2 host that implements the component model on a Wasm 2.0
  engine.

**What webcpp would gain:**

- One exception policy on wasip2 and wasip3. That removes:
  - `<toolset>clang-wasip2:<exception-handling>off`, `BOOST_NO_EXCEPTIONS`
    and the `/webcpp//throw_exception` handler on wasip2;
  - the lint's extra no-exceptions compile database;
  - every `#ifndef BOOST_NO_EXCEPTIONS` branch that only wasip2 tests.
- Boost libraries that throw unguarded (Boost.Locale, Boost.Test's runner)
  build on wasip2.
- Errors that the `-fno-exceptions` build turns into `abort()` (for example
  `vector::at`, `bad_alloc`, Boost.JSON's throwing accessors) become
  catchable. That is a better guarantee in a server that answers many
  requests.

**What webcpp would lose:**

- About 10-20 KB stripped (about 20%) per component, and up to 3 ms of
  uncached compile.
- Wasm 2.0 compatibility of wasip2 artifacts.
- The `-fno-exceptions` diagnostic on an escaping error. An uncaught
  exception now traps with only `thrown Wasm exception`, without libc++'s
  message. Under `__cpp_exceptions`, `detail::handle` could catch
  `std::exception` and print `what()` to stderr before answering 500.
  That was already the open item M1 of `WEBCPP_STAGE3_LOG.md` for wasip3, and
  it would then apply to both.

**Recommendation:** lift the restriction. Build wasip2 exactly like wasip3
(`-fwasm-exceptions -mllvm -wasm-use-legacy-eh=false`, link
`-fwasm-exceptions -lunwind`), record the requirement:

- wasmtime 47 or later (`-W exceptions=y` before 47);
- jco transpile `--bindgen-enable-wasm-exnref`;
- a Wasm 3.0 engine.

Settle M1 (catch and report in the handler) for both targets at once. The
`throw_exception` handler stays for a user who builds with
`exception-handling=off` on purpose, as on every other target.

## 7. Files

Everything is under `~/.claude/jobs/fcf3a902/tmp/spike-wasip2-eh/`
(throwaway; nothing was committed anywhere):

- `cmd/eh_command.cpp` and its builds `eh_exnref.wasm`, `eh_legacy.wasm`,
  `eh_exnref_O0.wasm`, `eh_p1.wasm`; `cmd/at_noexc.cpp`;
- `comp/eh_hello.cpp`, `comp/noeh_abort.cpp`, and every `*.wasm` of the
  tables;
- `dryrun-p2.txt`, `dry.http_hello.txt`, `build-p2-baseline.txt`,
  `build-p2-release.txt`;
- `serve_probe.sh`, `coldstart.py`;
- `jco/` (local jco, `run_eh.mjs`, `serve_eh.mjs`, `serve_ab.mjs`,
  `compile_core.mjs`);
- `nodes/` (Node 20, 22, 24), `spin/` (Spin 4.2.2), `wt46/` (wasmtime 46);
- `features.json`, `wt-rel/*.md` (wasmtime release notes 35-50).
