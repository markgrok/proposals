---
title: "wasi redesign: one entry point, p2 and p3, target features, run scripts"
tags: [webcpp, wasi, spike]
status: active
created: 2026-10-08
---

# wasi redesign: one entry point, p2 and p3, target features, run scripts

A measurement spike for the redesign of webcpp's `wasi` library
(`~/dev/webcpp/libs/wasi`). Everything built is throwaway, under
a scratch directory, `$S`. Nothing in
`~/dev/webcpp` was written; its `tools/component/serve.py` and its test
transcripts were only run, read-only.

## Summary

- **(a, b) One user entry point, two namespaces: yes, measured.** A front
  header picks `webcpp::wasi::p2::http` or `webcpp::wasi::p3::http`, each
  its own implementation in its own header, and the two macros define the
  one user function `webcpp::wasi::http::http_main` after the `extern "C"`
  export of the chosen implementation. All six of webcpp's served tests pass
  unchanged on that layout (section 2.7).
- **One component that exports both `wasi:http/incoming-handler@0.2.12` and
  `wasi:http/handler@0.3.0`: possible, but not useful. Do not do it.**
  - It builds: one WIT world with both exports, one wit-bindgen run, one
    module linked against one libc (either wasip2's or wasip3's). It
    validates, and `wasmtime serve` 47 serves it.
  - wasmtime always picks the p3 export. With p3 turned off it refuses the
    component, because the component imports `wasi:http/types@0.3.0`, so the
    p2 export can never be reached as a fallback.
  - Spin 4.2.2 refuses it: "component exports multiple different handlers
    but it's expected to export only one".
  - It costs +16.5 KB stripped (+24%) over the p2 component.
  - What is impossible is one *module* with both libcs (wasip2's and
    wasip3's): their ABIs differ (thread context in globals vs libcalls),
    and wasm-ld refuses the mix (section 2.5).
- **Recommendation for (b):** one source built twice, one component per
  target, each with exactly one export, both macros calling the same
  `http_main`. That is today's model, restructured into the two namespaces.
- **(c) Features beyond HTTP: wasip2 and wasip3 offer the same things through
  wasi-libc in wasi-sdk 34,** in commands and in served components alike:
  - filesystem, sockets (TCP client and server, UDP, DNS), clocks, random,
    environment, arguments and stdio all work, behind wasmtime's
    capability flags (`--dir`, `--env`, `-S inherit-network`,
    `-S allow-ip-name-lookup`);
  - threads work on neither. wasip3 even predefines `__STDCPP_THREADS__`
    and links `std::thread`, which then fails at run time with ENOTSUP;
  - wasip1 has no sockets: no `<netdb.h>`.
- **(d) Run scripts: a b2 `make <program>.sh : <program> : @generator`
  writes a POSIX sh script beside the `.wasm`, measured in a b2 prototype.**
  It passes the right `-S`/`-W` flags for the target and the exception
  setting, finds wasmtime in `$WASMTIME` or on PATH, and forwards `"$@"`.
  shellcheck reports nothing, and it runs under dash and with a space in its
  path.

## 1. Toolchain

```
$ ~/dev/webcpp/.local/wasi-sdk/bin/clang --version | head -1; cat ~/dev/webcpp/.local/wasi-sdk/VERSION
clang version 23.1.0-wasi-sdk (https://github.com/llvm/llvm-project 895aa2c896ada719451be2e3673c83da8ddf1141)
34.0
wasi-libc: 2e6fb9d8ee0c
$ wasmtime --version
wasmtime 47.0.3 (5554cc1a6 2026-07-31)
$ ~/dev/webcpp/.local/wit-bindgen/wit-bindgen --version
wit-bindgen-cli 0.62.0 (c604ee01c 2026-09-10)
$ command -v wasm-tools        # nothing: not on PATH
```

wasm-tools is not on PATH. The one used here is an earlier private project's pinned copy,
`.../001-runtime-scaffold/.build/third_party/wasm-tools/wasm-tools`,
`wasm-tools 1.259.0 (7fc33f279 2026-09-10)`, linked as `$S/wasm-tools`.
Also used: Spin 4.2.2 and jco 1.37.0 from the exceptions spike
(`~/.buzz/RESEARCH/WASIP2_EXCEPTIONS_SPIKE.md`), and b2 5.5.3
(`/opt/homebrew/bin/b2`) for the run-script prototype.

## 2. One component for p2 and p3

### 2.1 A world with both exports

`$S/worlds/both/wit/world.wit`:

```wit
package spike:both;

world service {
  export wasi:http/incoming-handler@0.2.12;
  export wasi:http/handler@0.3.0;
}
```

wit-bindgen resolves two versions of one package when they sit in `deps/`
under different file names. The WIT of each version was copied there with a
suffix (`http-0.2.wit`, `http-0.3.wit`, `cli-0.2.wit`, `cli-0.3.wit`, ...,
`io-0.2.wit`):

```
$ wit-bindgen c --world both --rename-world webcpp_wasi_http \
    --async 'wasi:http/handler@0.3.0#handle' --out-dir gen wit; echo rc=$?
Generating "gen/webcpp_wasi_http.c"
Generating "gen/webcpp_wasi_http.h"
Generating "gen/webcpp_wasi_http_component_type.o"
rc=0
```

When a world holds two versions of a package, wit-bindgen puts the version
into every C name:

```
$ grep -n 'handle(' gen/webcpp_wasi_http.h
1699:void exports_wasi_http_0_2_12_incoming_handler_handle(exports_wasi_http_0_2_12_incoming_handler_own_incoming_request_t request, ...);
1702:webcpp_wasi_http_callback_code_t exports_wasi_http_0_3_0_handler_handle(exports_wasi_http_0_3_0_handler_own_request_t request);
... wasi_http_0_2_12_types_constructor_fields, wasi_http_0_3_0_types_constructor_fields, ...
```

A world with one version gets the plain names (`wasi_http_types_*`) instead,
so code written against one form does not compile against the other.
wit-bindgen's `--rename <K=V>` ("Rename the interface `K` to `V` in the
generated source code") gives each version a fixed prefix in both cases:

```
R2="--rename wasi:http/types@0.2.12=wasi_p2_http_types --rename wasi:http/incoming-handler@0.2.12=wasi_p2_http_incoming_handler \
    --rename wasi:io/streams@0.2.12=wasi_p2_io_streams --rename wasi:io/error@0.2.12=wasi_p2_io_error --rename wasi:io/poll@0.2.12=wasi_p2_io_poll"
R3="--rename wasi:http/types@0.3.0=wasi_p3_http_types --rename wasi:http/handler@0.3.0=wasi_p3_http_handler"
$ wit-bindgen c --world both --rename-world webcpp_wasi_http $R2 $R3 --async 'wasi:http/handler@0.3.0#handle' --out-dir gen-renamed wit
rc=0
$ grep -n 'handle(' gen-renamed/webcpp_wasi_http.h
1699:void exports_wasi_p2_http_incoming_handler_handle(exports_wasi_p2_http_incoming_handler_own_incoming_request_t request, ...);
1702:webcpp_wasi_http_callback_code_t exports_wasi_p3_http_handler_handle(exports_wasi_p3_http_handler_own_request_t request);
$ grep -c 'wasi_http_0_' gen-renamed/webcpp_wasi_http.h
0
```

Constants follow (`WASI_P2_HTTP_TYPES_METHOD_GET`,
`WASI_P3_HTTP_TYPES_ERROR_CODE_INTERNAL_ERROR`). The world-prefixed helpers
(`webcpp_wasi_http_string_t`, `webcpp_wasi_http_waitable_set_new`,
`WEBCPP_WASI_HTTP_CALLBACK_CODE_EXIT`) are shared by both. With the same
renames, a p2-only world and a p3-only world produce the same names as the
world with both, so one p2 header and one p3 header serve all three.

### 2.2 The layout and the build

`$S/lib/include/webcpp/wasi/`:

| Header | Holds |
| --- | --- |
| `config.hpp` | the target, from clang's macros: `WEBCPP_WASI_TARGET_P2`/`_P3`, and feature macros (section 3.4) |
| `http/response.hpp` | `webcpp::wasi::http::response`, unchanged |
| `http/main.hpp` | the declaration of the one entry point, `webcpp::wasi::http::response webcpp::wasi::http::http_main(std::string_view method, std::string_view target)`; no ABI |
| `p2/http.hpp` | `namespace webcpp::wasi::p2::http::detail`: today's p2 code (`refuse`, `write_all`, `respond`, `handle`), against the `wasi_p2_*` names, and `WEBCPP_WASI_P2_HTTP_EXPORTS()`, the `extern "C"` export calling `detail::handle` |
| `p3/http.hpp` | `namespace webcpp::wasi::p3::http::detail`: today's p3 code (`wait_for`, `write_all`, `resolve_trailers`, `respond`, `handle`), against the `wasi_p3_*` names, and `WEBCPP_WASI_P3_HTTP_EXPORTS()`, the export and its callback |
| `http/entrypoint.hpp` | the front: chooses p2, p3 or both, includes their headers, and defines `WEBCPP_WASI_HTTP_MAIN_BEGIN/END` |

Both `p2/http.hpp` and `p3/http.hpp` were generated from today's
`entrypoint.hpp` by cutting its `#ifdef` branches apart and renaming. Each
`handle` calls `webcpp::wasi::http::http_main`. The front, whole:

```cpp
#include <webcpp/wasi/config.hpp>
#include <webcpp/wasi/http/main.hpp>

// The world's handler follows the target unless the build names it: wasip2
// exports wasi:http/incoming-handler, wasip3 wasi:http/handler.
#if !defined(WEBCPP_WASI_HTTP_P2) && !defined(WEBCPP_WASI_HTTP_P3)
#if defined(WEBCPP_WASI_TARGET_P3)
#define WEBCPP_WASI_HTTP_P3 1
#elif defined(WEBCPP_WASI_TARGET_P2)
#define WEBCPP_WASI_HTTP_P2 1
#else
#error "webcpp/wasi/http/entrypoint.hpp: build for wasm32-wasip2 or wasm32-wasip3, or define WEBCPP_WASI_HTTP_P2 or WEBCPP_WASI_HTTP_P3"
#endif
#endif

#ifdef WEBCPP_WASI_HTTP_P2
#include <webcpp/wasi/p2/http.hpp>
#define WEBCPP_WASI_HTTP_DETAIL_P2_EXPORTS() WEBCPP_WASI_P2_HTTP_EXPORTS()
#else
#define WEBCPP_WASI_HTTP_DETAIL_P2_EXPORTS()
#endif

#ifdef WEBCPP_WASI_HTTP_P3
#include <webcpp/wasi/p3/http.hpp>
#define WEBCPP_WASI_HTTP_DETAIL_P3_EXPORTS() WEBCPP_WASI_P3_HTTP_EXPORTS()
#else
#define WEBCPP_WASI_HTTP_DETAIL_P3_EXPORTS()
#endif

#define WEBCPP_WASI_HTTP_EXPORTS()      \
    WEBCPP_WASI_HTTP_DETAIL_P2_EXPORTS() \
    WEBCPP_WASI_HTTP_DETAIL_P3_EXPORTS()

#define WEBCPP_WASI_HTTP_MAIN_BEGIN(method, target)                                     \
    WEBCPP_WASI_HTTP_EXPORTS()                                                          \
    webcpp::wasi::http::response webcpp::wasi::http::http_main(std::string_view method, \
                                                               std::string_view target) {
#define WEBCPP_WASI_HTTP_MAIN_END() }
```

The user's program (`$S/app/main.cpp`) is today's `http_hello` with the
target and the export that answered in its body. That needed a spike-only
`inline const char* spike_handler`, which each `handle` sets:

```cpp
#include <webcpp/wasi/http/entrypoint.hpp>

WEBCPP_WASI_HTTP_MAIN_BEGIN(method, target)
    if (method != "GET") {
        return {.status = 405, .content_type = "text/plain", .body = "only GET\n"};
    }
    ...
    return {.status = 200, .content_type = "text/plain",
            .body = "hello from " + std::string(target) + " (" + built + ", " + webcpp::wasi::http::spike_handler + ")\n"};
WEBCPP_WASI_HTTP_MAIN_END()
```

`$S/build.sh <world: p2|p3|both> <target: wasip2|wasip3> <out.wasm>`
generates the bindings with the renames, compiles their `.c` (with `-pthread`
on wasip3, as webcpp does), and compiles and links the program as webcpp does
today:

```
"$WB" c --world service --rename-world webcpp_wasi_http $renames $async --out-dir "$gen" "$S/worlds/$world/wit"
"$SDK/bin/clang" --target=wasm32-$target $pthread -c "$gen/webcpp_wasi_http.c" -o "$gen/webcpp_wasi_http.o" -I"$gen"
"$SDK/bin/clang++" --target=wasm32-$target -std=c++20 -O2 -Wall -Wextra \
    -fwasm-exceptions -mllvm -wasm-use-legacy-eh=false \
    -mexec-model=reactor $defs -I"$S/lib/include" -I"$gen" \
    app/main.cpp "$gen/webcpp_wasi_http.o" "$gen/webcpp_wasi_http_component_type.o" \
    -fwasm-exceptions -lunwind -o "$out"
```

`$defs` is `-DWEBCPP_WASI_HTTP_P2`, `-DWEBCPP_WASI_HTTP_P3`, or both. Every
combination of world and target built with rc=0, with no warning. The world
is the HTTP export; the target is the libc and its imports.

### 2.3 What each combination imports and exports

```
$ for f in p2-wasip2 p2-wasip3 p3-wasip2 p3-wasip3 both-wasip2 both-wasip3; do wasm-tools validate $f.wasm && echo valid; wasm-tools component wit $f.wasm | grep -E '^\s*(import|export) '; done
=== p2-wasip2.wasm 943727
valid
  import wasi:io/poll@0.2.12; import wasi:clocks/monotonic-clock@0.2.12; import wasi:io/error@0.2.12;
  import wasi:io/streams@0.2.12; import wasi:http/types@0.2.12; import wasi:cli/environment@0.2.12;
  import wasi:cli/exit@0.2.12; import wasi:cli/stdin@0.2.12; ... import wasi:clocks/wall-clock@0.2.12;
  export wasi:http/incoming-handler@0.2.12;
=== p2-wasip3.wasm 997257
valid
  import wasi:io/error@0.2.12; import wasi:io/streams@0.2.12; import wasi:http/types@0.2.12;
  import wasi:cli/environment@0.3.0; ... import wasi:clocks/system-clock@0.3.0;
  export wasi:http/incoming-handler@0.2.12;
=== p3-wasip2.wasm 941867
valid
  import wasi:http/types@0.3.0; import wasi:cli/environment@0.2.12; ... import wasi:clocks/wall-clock@0.2.12;
  export wasi:http/handler@0.3.0;
=== p3-wasip3.wasm 994344
valid
  import wasi:clocks/types@0.3.0; import wasi:http/types@0.3.0; import wasi:cli/environment@0.3.0; ...
  export wasi:http/handler@0.3.0;
=== both-wasip2.wasm 964296
valid
  import wasi:io/poll@0.2.12; ... import wasi:http/types@0.2.12; import wasi:http/types@0.3.0;
  import wasi:cli/environment@0.2.12; ... import wasi:clocks/wall-clock@0.2.12;
  export wasi:http/incoming-handler@0.2.12;
  export wasi:http/handler@0.3.0;
=== both-wasip3.wasm 1017388
valid
  import wasi:io/error@0.2.12; import wasi:io/streams@0.2.12; import wasi:http/types@0.2.12;
  import wasi:clocks/types@0.3.0; import wasi:http/types@0.3.0; import wasi:cli/environment@0.3.0; ...
  export wasi:http/incoming-handler@0.2.12;
  export wasi:http/handler@0.3.0;
```

(Lines joined for brevity; `wasm-tools component wit <file>.wasm` in
`$S/out/` prints the full lists.) No adapter is involved: wasi-sdk 34 links wasip2 and
wasip3 with `wasm-component-ld`, and no component imports
`wasi_snapshot_preview1`:

```
$ clang++ --target=wasm32-wasip2 -### ... | grep -o wasm-component-ld
".../wasi-sdk/bin/wasm-component-ld"
$ wasm-tools print both-wasip3.wasm | grep -c wasi_snapshot_preview1
0
```

So p2 and p3 do not conflict inside one component: the p3 export uses the
canonical async built-ins and `wasi:http/types@0.3.0`, whatever libc the
module was linked with. The two libcs do conflict, and only one can be in
the module (section 2.5).

### 2.4 Which export the hosts pick

`$S/serve_probe.sh <component> [flags]` runs
`wasmtime serve <flags> --addr 127.0.0.1:0 <component>`, waits for
`Serving HTTP on`, sends `GET /hello` and `POST /x`, then stops it. Headers
`date`, `content-type` and `transfer-encoding` and blank lines are elided
below; the 405 answers were all `only GET`.

| Component | `-S cli` | `-S cli,p3 -W component-model-async` | `-S cli,p3=n` | `-S cli -W component-model-async=n` | no flags |
| --- | --- | --- | --- | --- | --- |
| p2-wasip2 | 200 `(wasm32-wasip2, wasi:http/incoming-handler@0.2.12)` | same | (not run) | (not run) | refused: imports `wasi:cli/environment@0.2.12` |
| p3-wasip3 | 200 `(wasm32-wasip3, wasi:http/handler@0.3.0)` | same | refused (1) | refused (2) | 200, also with `-S cli=n` |
| p2-wasip3 | 200 `(wasm32-wasip3, wasi:http/incoming-handler@0.2.12)` | same | refused: imports `wasi:cli/environment@0.3.0` | refused (2) | 200 |
| p3-wasip2 | 200 `(wasm32-wasip2, wasi:http/handler@0.3.0)` | same | refused (1) | refused (2) | refused: imports `wasi:cli/environment@0.2.12` |
| **both-wasip2** | 200 **`(wasm32-wasip2, wasi:http/handler@0.3.0)`** | same | refused (1) | refused (2) | refused: imports `wasi:cli/environment@0.2.12` |
| **both-wasip3** | 200 **`(wasm32-wasip3, wasi:http/handler@0.3.0)`** | same | refused (1) | refused (2) | 200 `(..., wasi:http/handler@0.3.0)` |

(1) and (2), verbatim:

```
$ wasmtime serve -S cli,p3=n --addr 127.0.0.1:0 both-wasip2.wasm
Error: component imports instance `wasi:http/types@0.3.0`, but a matching implementation was not found in the linker
Caused by:
    0: instance export `fields` has the wrong type
    1: resource implementation is missing

$ wasmtime serve -S cli -W component-model-async=n --addr 127.0.0.1:0 both-wasip2.wasm
Error: failed to parse WebAssembly module
Caused by:
    `stream` requires the component model async feature (at offset 0xbab)

$ wasmtime serve --addr 127.0.0.1:0 p2-wasip2.wasm
Error: component imports instance `wasi:cli/environment@0.2.12`, but a matching implementation was not found in the linker
Caused by:
    0: instance export `get-environment` has the wrong type
    1: function implementation is missing
```

What this shows:

- **wasmtime serve 47 picks `wasi:http/handler@0.3.0` whenever a component
  exports it.** The p2 export of a component with both is never called.
- **There is no fallback.** A host without p3 cannot even link the
  component, since the p3 export brings the `wasi:http/types@0.3.0` import
  and the `stream` type. So the p2 export can never be the one in use.
- **wasmtime 47 needs no flag for WASI 0.3.** WASI 0.3.0 and
  `component-model-async` are on by default since wasmtime 46 (release
  notes 46.0.0: "Wasmtime now supports WASI 0.3.0 by default and the
  `component-model-async` wasm feature is now enabled by default", #13612).
  `wasmtime serve` provides `wasi:cli@0.3.0` whatever `-S cli` says, and
  needs `-S cli` only for `wasi:cli@0.2.x`, which the wasip2 libc imports.
  The p3 lane's `-S cli,p3 -W component-model-async` is redundant on 47 but
  harmless.
- **The libc target and the HTTP version are independent.** A p3 handler on
  the wasip2 libc and a p2 handler on the wasip3 libc both serve.

Spin 4.2.2 (wasmtime 49), with a manifest per component
(`[[trigger.http]] route = "/..."`, `source = "../../out/<f>.wasm"`) and
`spin up --listen 127.0.0.1:18460`:

```
######## spin up: p2-wasip2.wasm
HTTP/1.1 200 OK
hello from /hello (wasm32-wasip2, wasi:http/incoming-handler@0.2.12)
######## spin up: p3-wasip3.wasm
HTTP/1.1 200 OK
hello from /hello (wasm32-wasip3, wasi:http/handler@0.3.0)
######## spin up: both-wasip2.wasm
ERROR spin_trigger::cli: Trigger executor failed
Error: component exports multiple different handlers but it's expected to export only one
######## spin up: both-wasip3.wasm
Error: component exports multiple different handlers but it's expected to export only one
```

Spin also logs `worker failed: worker panicked` for the p3 component, at
shutdown only. The backtrace is in `wasmtime_wasi_http::handler::Worker`'s
drop under `OwnedTasks::close_and_shutdown_all`; both requests before it
answered 200.

jco 1.37.0 `transpile --bindgen-enable-wasm-exnref` accepts p2-wasip2,
both-wasip2 and p3-wasip3, writing `.core.wasm`, `.js` and `.d.ts`. Serving
them through preview2-shim was not tried.

### 2.5 One module for both libcs: impossible

```
$ clang++ --target=wasm32-wasip2 -std=c++20 -O2 -c clocks.cpp -o clocks-p2.o
$ clang++ --target=wasm32-wasip3 clocks-p2.o -o mixed.wasm      # a wasip2 object on the wasip3 sysroot
wasm-ld: error: clocks-p2.o: object file uses globals for thread context, but --cooperative-threading was specified
wasm-ld: error: clocks-p2.o: undefined symbol: __stack_pointer
...
$ clang++ --target=wasm32-wasip2 clocks-p2.o .../lib/wasm32-wasip3/libc.a -o two-libcs.wasm
wasm-ld: error: 'atomics' feature must be used in order to use thread-local storage
wasm-ld: error: .../wasm32-wasip3/libc.a(clock_gettime.c.obj): undefined symbol: __wasm_get_stack_pointer
wasm-ld: error: .../wasm32-wasip3/libc.a(clock_gettime.c.obj): undefined symbol: __wasm_set_stack_pointer
...
```

wasm32-wasip3 is a different ABI (`__wasm_libcall_thread_context__`, section
3.1): its objects reach the stack pointer and TLS through libcalls for
cooperative threads, and wasip2's through globals. Every object of a module,
and its libc, is compiled for one target. That is "one module compiled for
both": it does not exist.

### 2.6 Size

Release-like (`-O2`), with exceptions, `wasm-tools strip -a`, `gzip -9`:

| Component | raw | stripped | gzip of stripped |
| --- | ---: | ---: | ---: |
| p2-wasip2 | 944,362 | 70,109 | 27,009 |
| p3-wasip3 | 994,991 | 77,097 | 29,152 |
| p2-wasip3 | 997,904 | 79,204 | 29,822 |
| p3-wasip2 | 942,518 | 68,594 | 26,700 |
| both-wasip2 | 964,947 | 86,625 | 31,428 |
| both-wasip3 | 1,018,035 | 95,530 | 34,162 |

The component with both is +16.5 KB stripped (+24%) over p2-wasip2, for an
export no host calls.

### 2.7 webcpp's own transcripts pass on the spike layout

Each served test of `libs/wasi/test` was built with `$S/build.sh` (sources
unchanged, `APP=<test>.cpp`) and checked with webcpp's own
`tools/component/serve.py` against its committed `.requests` and
`.expected`:

```
$ python3 ~/dev/webcpp/tools/component/serve.py --flags="$flags" --component out/$t-$w-$tg.wasm \
    --requests libs/wasi/test/$t.requests --expected libs/wasi/test/$t.expected --output transcripts/$t-$w-$tg.txt
http_methods world=p2 target=wasip2 flags='-S cli': serve.py rc=0
http_methods world=p3 target=wasip3 flags='-S cli,p3 -W component-model-async': serve.py rc=0
http_methods world=both target=wasip2 flags='-S cli': serve.py rc=0
http_methods world=both target=wasip3 flags='-S cli': serve.py rc=0
(the same four lines, all rc=0, for http_long_body, http_no_content_type,
 http_content_type_of_target, http_bad_status, http_bad_content_type)
```

24 of 24 pass. `http_two_units`, which needs a second source, was not
built. Built without `-D` (`DEFS=`), the front chose the export from the
target, and both served:

```
hello from /hello (wasm32-wasip2, wasi:http/incoming-handler@0.2.12)
hello from /hello (wasm32-wasip3, wasi:http/handler@0.3.0)
$ echo '#include <webcpp/wasi/http/entrypoint.hpp>' | clang++ --target=wasm32-wasip1 -fsyntax-only ...
error: "webcpp/wasi/http/entrypoint.hpp: build for wasm32-wasip2 or wasm32-wasip3, or define WEBCPP_WASI_HTTP_P2 or WEBCPP_WASI_HTTP_P3"
```

### 2.8 Recommendation

- **One component per target, one export each.** Today's model stays: one
  source, built twice, through the same `http_main`. Drop the idea of a
  component with both exports. No host gains from it: wasmtime ignores its
  p2 half, a p2-only host cannot load it, and Spin refuses it.
- **Layout:**

  ```
  include/webcpp/wasi.hpp                  everything
  include/webcpp/wasi/config.hpp           target and features (section 3.4)
  include/webcpp/wasi/http/response.hpp    the response; no ABI, native too
  include/webcpp/wasi/http/main.hpp        declares webcpp::wasi::http::http_main, the one entry point; no ABI
  include/webcpp/wasi/p2/http.hpp          namespace webcpp::wasi::p2::http: the wasi:http 0.2 handler, WEBCPP_WASI_P2_HTTP_EXPORTS()
  include/webcpp/wasi/p3/http.hpp          namespace webcpp::wasi::p3::http: the wasi:http 0.3 handler, WEBCPP_WASI_P3_HTTP_EXPORTS()
  include/webcpp/wasi/http/entrypoint.hpp  the front: includes the target's implementation, WEBCPP_WASI_HTTP_MAIN_BEGIN/END
  ```

  - Each implementation is a clean header with no `#ifdef` between versions.
    The only branch is the front's choice.
  - The front should accept exactly one, as today's guard does. Default it
    from `__wasip2__`/`__wasip3__`, so that a build by hand needs no `-D`.
    Keep `WEBCPP_WASI_HTTP_P2`/`_P3` as an override for a world that does
    not follow the target.
  - `http_main` is a plain function declared in `main.hpp`. A program may
    also define it without the macros and expand `WEBCPP_WASI_HTTP_EXPORTS()`
    once, which keeps clang-format and IDEs happy.
- **Binding names:**
  - With exactly one export per component, the plain wit-bindgen names
    (`wasi_http_types_*`) work, and the commands shown for a build by hand
    stay short.
  - `--rename` (section 2.1) is the tool if a world with both versions is
    ever wanted. It costs five renames on p2 and two on p3, in `build.jam`
    and in the commands for a build by hand.

## 3. What each target offers beyond HTTP

### 3.1 Predefined macros

```
$ for t in wasm32-wasip1 wasm32-wasip1-threads wasm32-wasip2 wasm32-wasip3; do echo | clang --target=$t -dM -E - | grep -E '__wasi|__wasm_atomics|__wasm_libcall' ; done
== wasm32-wasip1:          #define __wasi__ 1   #define __wasip1__ 1
== wasm32-wasip1-threads:  #define __wasi__ 1   #define __wasip1__ 1   #define __wasm_atomics__ 1
== wasm32-wasip2:          #define __wasi__ 1   #define __wasip2__ 1
== wasm32-wasip3:          #define __wasi__ 1   #define __wasip3__ 1   #define __wasm_libcall_thread_context__ 1
```

(All four also define `__wasm__`, `__wasm32__`, `__wasm_bulk_memory__`,
`__wasm_multivalue__`, `__wasm_mutable_globals__`,
`__wasm_nontrapping_fptoint__`, `__wasm_reference_types__` and
`__wasm_sign_ext__`.) In C++ (`clang++ -x c++ -std=c++20 -dM -E`):

| Flags | wasip2 | wasip3 |
| --- | --- | --- |
| none | `__cpp_exceptions`, `__EXCEPTIONS` | `__cpp_exceptions`, `__EXCEPTIONS`, **`__STDCPP_THREADS__`**, `__wasm_libcall_thread_context__` |
| `-pthread` | adds `__STDCPP_THREADS__`, `__wasm_atomics__`, `_REENTRANT` | adds `_REENTRANT` |
| `-fwasm-exceptions` | adds `__wasm_exception_handling__` | adds `__wasm_exception_handling__` |

- `__cpp_exceptions` is defined even without `-fwasm-exceptions`, a build
  that then fails to link (`__cxa_throw` undefined, per the exceptions
  spike). `__wasm_exception_handling__` is the macro that says wasm
  exceptions are on.
- `<wasi/version.h>` restates `__wasip2__`/`__wasip3__`, and adds
  `__wasi_sdk_major__ 34`, `__wasi_sdk_version__ "34.0"`, and
  `__wasi_cooperative_threads__`, which is **undefined** on wasip3 in
  wasi-sdk 34 ("used to test whether cooperative threads are enabled for
  this build of wasi-libc").

### 3.2 Programs and results

`$S/feat/`: `fs.cpp`, `clocks.cpp`, `rnd.cpp`, `envargs.cpp`, `stdio.cpp`,
`threads.cpp`, `tcp_client.cpp`, `tcp_server.cpp`, `udp.cpp`, each built as
webcpp builds a command:

```
clang++ --target=wasm32-$t -std=c++20 -O2 -fwasm-exceptions -mllvm -wasm-use-legacy-eh=false $p.cpp -fwasm-exceptions -lunwind -o $p-$t.wasm
```

Every one compiled and linked on both targets, except
`CLOCK_PROCESS_CPUTIME_ID` (below):

```
wasip2 clocks build rc=1
clocks.cpp:20:80: error: use of undeclared identifier 'CLOCK_PROCESS_CPUTIME_ID'
wasip3 clocks build rc=1   (the same)
```

The outputs below were identical on wasip2 and wasip3 unless shown
otherwise.

**Filesystem**

```
$ wasmtime run fs-wasip2.wasm /sandbox
fstream: FAILED
filesystem::create_directories: FAILED Invalid argument
filesystem::directory_iterator: FAILED No such file or directory
filesystem::file_size: FAILED No such file or directory
filesystem::current_path: ok, /
filesystem::remove_all: ok
open/read: FAILED No such file or directory
opendir/readdir: FAILED No such file or directory
(exit 1)
$ wasmtime run --dir $S/feat/sandbox::/sandbox fs-wasip2.wasm /sandbox
fstream: ok: written by fstream
filesystem::create_directories: ok
filesystem::directory_iterator: ok, 2 entries
filesystem::file_size: ok, 19
filesystem::current_path: ok, /
filesystem::remove_all: ok
open/read: ok, 19 bytes
opendir/readdir: ok, 3 entries
exit=0
$ cd sandbox && wasmtime run --dir . ../fs-wasip2.wasm .      # the same eight ok lines, exit=0
```

**Clocks**

```
$ wasmtime run clocks-wasip2.wasm
steady_clock + sleep_for(50ms): 52 ms
system_clock: 1791515836 (Fri Oct  9 03:17:16 2026)
high_resolution_clock is_steady: 1
clock_gettime(CLOCK_MONOTONIC): 0
clock_gettime(CLOCK_REALTIME): 0
CLOCK_PROCESS_CPUTIME_ID: not declared
time(): 1791515836
exit=0
```

`clock()` fails to link without emulation (`wasm-ld: error: ...: undefined
symbol: clock`). With `-D_WASI_EMULATED_PROCESS_CLOCKS` and
`-lwasi-emulated-process-clocks` it works
(`clock(): 36849834 CLOCKS_PER_SEC 1000000000`), but
`CLOCK_PROCESS_CPUTIME_ID` stays undeclared: `time.h` defines it only under
`__wasilibc_unmodified_upstream`.

**Random**

```
$ wasmtime run rnd-wasip2.wasm
getentropy: 0, first bytes d75fb753
random_device: 2955888472 2435737141, entropy() 0
arc4random: 1489195063
exit=0
```

**Environment and arguments**

```
$ wasmtime run envargs-wasip2.wasm one 'two words'
argc: 3
argv[0]: envargs-wasip2.wasm
argv[1]: one
argv[2]: two words
getenv(SPIKE_VAR): (unset)
environ entries: 0
$ SPIKE_VAR=host wasmtime run --env SPIKE_VAR envargs-wasip2.wasm     -> getenv(SPIKE_VAR): host, environ entries: 1
$ wasmtime run --env SPIKE_VAR=given envargs-wasip2.wasm              -> getenv(SPIKE_VAR): given, environ entries: 1
$ SPIKE_VAR=host wasmtime run -S inherit-env envargs-wasip2.wasm      -> getenv(SPIKE_VAR): host, environ entries: 113
```

**Standard streams**

```
$ printf 'abc\n' | wasmtime run stdio-wasip2.wasm
stdin: 4 bytes: abc
stderr: written
isatty(0,1,2): 000
exit=0
```

**Exit status**

```
wasip2: wasmtime run exit42 -> 1          (main returns 42)
wasip2: wasmtime run -S cli-exit-with-code exit42 -> 1
wasip2: std::exit(42) -> 42
wasip3: (the same three)
wasip1: wasmtime run exit42 -> 42
```

On wasip2 and wasip3, a status returned from `main` reaches the host only as
success or failure (1); `std::exit(n)` keeps it.

**Threads**

```
== wasip2  build rc=0
std::system_error: thread constructor failed: Not supported (code 58)
run exit=1
== wasip2 -pthread build rc=1
wasm-ld: error: --shared-memory is disallowed by Unwind-wasm.c.o because it was not compiled with 'atomics' or 'bulk-memory' features.
wasm-ld: error: .../threads-bc7e1b.o: relocation R_WASM_MEMORY_ADDR_TLS_SLEB cannot be used against non-TLS symbol `__wasm_lpad_context`
== wasip2 -pthread -fno-exceptions build rc=1
wasm-ld: error: --shared-memory is disallowed by memory.cpp.o because it was not compiled with 'atomics' or 'bulk-memory' features.
== wasip3  build rc=0
std::system_error: thread constructor failed: Not supported (code 58)
== wasip3 -pthread build rc=0
std::system_error: thread constructor failed: Not supported (code 58)
$ wasmtime run -W component-model-threading threads-wasip3.wasm
std::system_error: thread constructor failed: Not supported (code 58)
$ wasmtime run -W threads,shared-memory -S threads threads-pthread-noeh-wasip3.wasm
Error: support for wasi-threads has been removed from Wasmtime
$ clang++ --target=wasm32-wasip1-threads -pthread ... ; wasmtime run -W threads,shared-memory -S threads threads-p1threads.wasm
Error: the `-Sthreads` flag is no longer supported
```

wasi-sdk 34 ships no `wasm32-wasip2-threads` sysroot (`lib/` holds
`wasm32-wasip1`, `wasm32-wasip1-threads`, `wasm32-wasip2`,
`wasm32-wasip3`), and wasmtime 47 has dropped wasi-threads.

**Sockets.** The host side was a `python3 -m http.server 18472 --bind
127.0.0.1` and a Python UDP echo on 18471.

```
$ wasmtime run tcp_client-wasip2.wasm 127.0.0.1 18472
getaddrinfo(127.0.0.1): 0 ok
socket: 3 ok
connect: FAILED Permission denied
$ wasmtime run tcp_client-wasip2.wasm localhost 18472
getaddrinfo(localhost): -4 Non-recoverable error
$ wasmtime run -S inherit-network tcp_client-wasip2.wasm 127.0.0.1 18472
getaddrinfo(127.0.0.1): 0 ok
socket: 3 ok
connect: ok
recv: 255 bytes, status line: HTTP/1.0 200 OK
$ wasmtime run -S inherit-network tcp_client-wasip2.wasm localhost 18472
getaddrinfo(localhost): -4 Non-recoverable error
$ wasmtime run -S inherit-network,allow-ip-name-lookup tcp_client-wasip2.wasm localhost 18472
getaddrinfo(localhost): 0 ok ... connect: ok ... status line: HTTP/1.0 200 OK
$ wasmtime run -S inherit-network,allow-ip-name-lookup tcp_client-wasip2.wasm example.com 80
getaddrinfo(example.com): 0 ok ... connect: ok ... status line: HTTP/1.1 200 OK
$ wasmtime run -S tcp=n,inherit-network,allow-ip-name-lookup tcp_client-wasip2.wasm 127.0.0.1 18472
getaddrinfo(127.0.0.1): 0 ok
socket: -1 Permission denied
$ wasmtime run udp-wasip2.wasm 18471
socket: 3 ok
sendto: -1 Permission denied
$ wasmtime run -S inherit-network udp-wasip2.wasm 18471
socket: 3 ok
sendto: 4 ok
recvfrom: 9 ok ping-pong
$ wasmtime run -S inherit-network,udp=n udp-wasip2.wasm 18471
socket: -1 Permission denied
$ wasmtime run tcp_server-wasip2.wasm 18473          (then curl -sS http://127.0.0.1:18473/)
  curl: (7) Failed to connect to 127.0.0.1 port 18473 after 0 ms: Couldn't connect to server
  wasm: bind: FAILED Permission denied
$ wasmtime run -S inherit-network tcp_server-wasip2.wasm 18473
  curl: from a wasm socket
  wasm: listening on 18473
  wasm: accept: ok, received 78 bytes
```

The same on wasip3, line for line. On wasip1:

```
$ clang++ --target=wasm32-wasip1 -std=c++20 -O2 tcp_client.cpp -o tcp_client-wasip1.wasm
tcp_client.cpp:2:10: fatal error: 'netdb.h' file not found
```

**The same inside a served component.** `$S/probeapp/main.cpp` reads
`/data/hello.txt`, reads `SPIKE_VAR`, calls `getentropy`, reads the system
clock and connects over TCP, all inside `http_main`:

```
$ wasmtime serve -S cli --addr 127.0.0.1:0 probe-p2.wasm ; curl /hello
file: FAILED No such file or directory
env SPIKE_VAR: (unset)
getentropy: 0
system_clock > 2026: 1
tcp: connect FAILED Permission denied
$ wasmtime serve -S cli,inherit-network --dir $S/data::/data --env SPIKE_VAR=given --addr 127.0.0.1:0 probe-p2.wasm ; curl /hello
file: hello from a preopened dir
env SPIKE_VAR: given
getentropy: 0
system_clock > 2026: 1
tcp: HTTP/1.0 200 OK
```

The same for probe-p3.wasm (p3 world, wasip3 target). `wasmtime serve`
accepts `--dir` and `--env` as `run` does.

One more difference for served components (`wasmtime serve --help`):
`--max-instance-reuse-count` "defaults to 1 for WASIp2 components and 128
for WASIp3 components", and `--max-instance-concurrent-reuse-count` 1 and
16. A p3 instance, and so its globals, outlives one request.

### 3.3 Feature table

wasi-sdk 34 and wasmtime 47.0.3. "compiles/links" is the same on both
targets except where shown. "runs" names the wasmtime options it needs
under `wasmtime run`; `wasmtime serve` takes the same ones.

| Feature | wasip2 | wasip3 | wasmtime options to run | Notes |
| --- | --- | --- | --- | --- |
| `<fstream>`, `std::filesystem`, `open`, `opendir` | yes | yes | `--dir HOST[::GUEST]` | without one, every path fails (ENOENT or EINVAL); `current_path` is `/` |
| `std::chrono` steady, system, high_resolution; `sleep_for`; `clock_gettime` MONOTONIC/REALTIME | yes | yes | none | |
| `clock()`, `times()` | link only with `-D_WASI_EMULATED_PROCESS_CLOCKS -lwasi-emulated-process-clocks` | same | none | `CLOCK_PROCESS_CPUTIME_ID` undeclared even then |
| `getentropy`, `std::random_device`, `arc4random` | yes | yes | none | `random_device::entropy()` is 0 |
| `argv` | yes | yes | arguments after the `.wasm` | `argv[0]` is the path as given |
| `getenv`, `environ` | yes | yes | `--env NAME[=VAL]`, or `-S inherit-env` | empty by default |
| stdin, stdout, stderr | yes | yes | none (inherited by default) | `isatty` is 0 |
| exit status | `std::exit(n)` keeps n; `return n` from `main` gives 1 | same | none | `-S cli-exit-with-code` changes nothing |
| TCP client (`socket`, `connect`, `send`, `recv`) | yes | yes | `-S inherit-network` | `-S tcp=n` refuses `socket` (EACCES) |
| TCP server (`bind`, `listen`, `accept`) | yes | yes | `-S inherit-network` | without it, `bind` EACCES |
| UDP (`sendto`, `recvfrom`) | yes | yes | `-S inherit-network` | `-S udp=n` refuses `socket` |
| `getaddrinfo` of a name | yes | yes | `-S inherit-network,allow-ip-name-lookup` | a numeric address needs no lookup |
| `std::thread`, `std::async` | compiles, links; throws ENOTSUP | same | none helps | |
| `-pthread` | does not link | compiles, links; still ENOTSUP | wasi-threads removed from wasmtime | wasip3 predefines `__STDCPP_THREADS__` anyway |
| C++ exceptions | with `-fwasm-exceptions -mllvm -wasm-use-legacy-eh=false`, link `-fwasm-exceptions -lunwind` | same | none on 47 (`-W exceptions=y` on 37-46) | exceptions spike |
| inside a served component | all of the above | all of the above | `serve` takes `--dir`, `--env`, `-S inherit-network` | p3 instances are reused across requests |
| wasip1, for contrast | | | | no `<netdb.h>`, no sockets |

### 3.4 What a config header can expose

| Macro | Defined when | Why |
| --- | --- | --- |
| `WEBCPP_WASI_TARGET_P2` / `WEBCPP_WASI_TARGET_P3` (`_P1`) | `__wasip2__` / `__wasip3__` / `__wasip1__` | clang predefines them; `<wasi/version.h>` restates them |
| `WEBCPP_WASI_HTTP_P2` / `WEBCPP_WASI_HTTP_P3` | the build's world, else the target's (section 2.8) | which handler the front includes |
| `WEBCPP_WASI_HAS_FILESYSTEM` | `__wasi__` | every target has it; the host decides with `--dir` |
| `WEBCPP_WASI_HAS_SOCKETS` | `__wasip2__ \|\| __wasip3__` | wasip1 has no `<netdb.h>`; the host decides with `-S inherit-network` |
| `WEBCPP_WASI_HAS_THREADS` | `__wasi_cooperative_threads__` (from `<wasi/version.h>`), never `__STDCPP_THREADS__` | undefined on both in wasi-sdk 34; wasip3 predefines `__STDCPP_THREADS__` and still cannot start a thread |
| `WEBCPP_WASI_HAS_EXCEPTIONS` | `__wasm_exception_handling__` | `__cpp_exceptions` is defined on wasm without `-fwasm-exceptions` |
| `WEBCPP_WASI_HAS_PROCESS_CLOCKS` | `_WASI_EMULATED_PROCESS_CLOCKS` | `clock()`/`times()` need the emulation library |

Clocks, random, environment, arguments and stdio need no macro: every WASI
target has them. The `HAS_*` macros say what compiles. What runs is the
host's grant, and that belongs in the run script (section 4) rather than in
a macro. `$S/lib/include/webcpp/wasi/config.hpp` holds a first cut of the
target and sockets macros.

## 4. Run scripts

### 4.1 The wasmtime command line of each artifact

Measured on wasmtime 47.0.3. "minimal" is what 47 needs; "explicit" also
states what 47 turns on by default, so the line runs on the oldest wasmtime
that can run the artifact (wasmtime 37-46 need `-W exceptions=y` for a
program built with exceptions; WASI 0.3.0 needs wasmtime 46 or later).

| Artifact | minimal (47) | explicit |
| --- | --- | --- |
| wasip2 command, exceptions | `wasmtime run prog.wasm ARGS` | `wasmtime run -W exceptions=y prog.wasm ARGS` |
| wasip2 command, no exceptions | `wasmtime run prog.wasm ARGS` | same |
| wasip3 command, exceptions | `wasmtime run prog.wasm ARGS` | `wasmtime run -S p3 -W component-model-async -W exceptions=y prog.wasm ARGS` |
| wasip3 command, no exceptions | `wasmtime run prog.wasm ARGS` | `wasmtime run -S p3 -W component-model-async prog.wasm ARGS` |
| wasip2 HTTP component, exceptions | `wasmtime serve -S cli --addr A comp.wasm` | `wasmtime serve -S cli -W exceptions=y --addr A comp.wasm` |
| wasip2 HTTP component, no exceptions | `wasmtime serve -S cli --addr A comp.wasm` | same |
| wasip3 HTTP component, exceptions | `wasmtime serve --addr A comp.wasm` | `wasmtime serve -S p3 -W component-model-async -W exceptions=y --addr A comp.wasm` (serve.py today: `-S cli,p3 -W component-model-async`) |
| wasip3 HTTP component, no exceptions | `wasmtime serve --addr A comp.wasm` | `wasmtime serve -S p3 -W component-model-async --addr A comp.wasm` |

Evidence:

- `wasmtime run` of every program of 3.2, with no flags, on both targets.
- `-S p3=n` refuses a wasip3 command ("failed to run main module").
- `-W exceptions=n` refuses a program built with exceptions ("failed to
  parse WebAssembly module") and runs one built with `-fno-exceptions`.
- `-S cli=n` refuses every command.
- `wasmtime serve`: section 2.4.
- `testing.launcher=wasmtime` runs `wasmtime prog.wasm ARGS`, which is
  `wasmtime run`. `wasmtime run -- prog.wasm --flag x` passes `--flag x` to
  the program.

What a program that uses the host needs on top: `--dir HOST[::GUEST]` for
files; `--env NAME[=VAL]` or `-S inherit-env` for the environment;
`-S inherit-network` for any socket; `allow-ip-name-lookup` for names
(`-S inherit-network,allow-ip-name-lookup`). wasmtime grants none of them
by default. TCP and UDP are on by default and only gated by the network
grant.

### 4.2 The script

A prototype b2 project, `$S/b2proto/`, with webcpp's two wasi toolsets in
its `user-config.jam` and webcpp's exception flags in its `Jamroot`:

```
exe hello : hello.cpp ;
make hello.sh : hello : @run-script.command ;
exe http_hello : main.cpp <bindings .c and _component_type.o> : ... <linkflags>-mexec-model=reactor ;
make http_hello.sh : http_hello : @run-script.serve ;
```

The scripts land beside the programs, in the same build directory, with no
`<location>`. b2 gives a `make` with the same properties the same directory
as the `exe`:

```
$ b2 --user-config=user-config.jam toolset=clang-wasip{2,3} exception-handling={on,off} hello.sh hello http_hello.sh
$ find bin -name 'hello*' -not -name '*.o' | sort
bin/clang-darwin-wasip2/debug/exception-handling-off/link-static/target-os-wasi/hello.sh
bin/clang-darwin-wasip2/debug/exception-handling-off/link-static/target-os-wasi/hello.wasm
bin/clang-darwin-wasip2/debug/link-static/target-os-wasi/hello.sh
bin/clang-darwin-wasip2/debug/link-static/target-os-wasi/hello.wasm
bin/clang-darwin-wasip3/debug/exception-handling-off/link-static/target-os-wasi/hello.sh
bin/clang-darwin-wasip3/debug/exception-handling-off/link-static/target-os-wasi/hello.wasm
bin/clang-darwin-wasip3/debug/link-static/target-os-wasi/hello.sh
bin/clang-darwin-wasip3/debug/link-static/target-os-wasi/hello.wasm
```

A command's script (wasip3, exceptions), as generated:

```sh
#!/bin/sh
# Generated by b2: runs hello.wasm, a wasm32-wasip3 command, with wasmtime.
# WASMTIME names wasmtime (default: wasmtime on PATH); WASMTIME_FLAGS adds
# host options, such as --dir=. or -S inherit-network,allow-ip-name-lookup.
here=$(dirname -- "$0")
wasmtime=${WASMTIME:-wasmtime}
if ! command -v -- "$wasmtime" >/dev/null 2>&1; then
    echo "$0: wasmtime not found: install it, or set WASMTIME" >&2
    exit 127
fi
# Usage: hello.sh [argument...]: the arguments go to the program.
# shellcheck disable=SC2086 # WASMTIME_FLAGS is a list of words
exec "$wasmtime" run -S p3 -W component-model-async -W exceptions=y $WASMTIME_FLAGS -- "$here/hello.wasm" "$@"
```

A served component's script (wasip2, exceptions):

```sh
#!/bin/sh
# Generated by b2: runs http_hello.wasm, a wasm32-wasip2 HTTP component, with wasmtime.
# WASMTIME names wasmtime (default: wasmtime on PATH); WASMTIME_FLAGS adds
# host options, such as --dir=. or -S inherit-network,allow-ip-name-lookup.
here=$(dirname -- "$0")
wasmtime=${WASMTIME:-wasmtime}
if ! command -v -- "$wasmtime" >/dev/null 2>&1; then
    echo "$0: wasmtime not found: install it, or set WASMTIME" >&2
    exit 127
fi
# Usage: http_hello.sh [address] [wasmtime-option...]: serves on address,
# 127.0.0.1:8080 by default; the options go to wasmtime serve.
address=127.0.0.1:8080
case ${1-} in
    ''|-*) ;;
    *) address=$1; shift ;;
esac
# shellcheck disable=SC2086 # WASMTIME_FLAGS is a list of words
exec "$wasmtime" serve -S cli -W exceptions=y $WASMTIME_FLAGS "$@" --addr "$address" -- "$here/http_hello.wasm"
```

The four exec lines that vary:

```
wasip2, exceptions on:   exec "$wasmtime" run -W exceptions=y $WASMTIME_FLAGS -- "$here/hello.wasm" "$@"
wasip2, exceptions off:  exec "$wasmtime" run $WASMTIME_FLAGS -- "$here/hello.wasm" "$@"
wasip2 serve, off:       exec "$wasmtime" serve -S cli $WASMTIME_FLAGS "$@" --addr "$address" -- "$here/http_hello.wasm"
wasip3, off:             exec "$wasmtime" run -S p3 -W component-model-async $WASMTIME_FLAGS -- "$here/hello.wasm" "$@"
```

Checks:

```
$ for f in $(find bin -name '*.sh'); do shellcheck -s sh $f && echo "shellcheck ok: $f"; done
shellcheck ok: (all six)
$ .../wasip2/.../hello.sh one 'two words' --flag
argc: 4 / argv[0]: hello.wasm / argv[1]: one / argv[2]: two words / argv[3]: --flag
$ dash .../wasip3/.../hello.sh x                      -> argc: 2, exit=0
$ SPIKE_VAR=v WASMTIME_FLAGS='--env SPIKE_VAR' .../wasip3/.../hello.sh  -> getenv(SPIKE_VAR): v
$ WASMTIME=/nowhere/wasmtime .../hello.sh
.../hello.sh: wasmtime not found: install it, or set WASMTIME
exit=127
$ PATH=/usr/bin:/bin .../hello.sh                     -> the same message, exit=127
$ WASMTIME=/opt/homebrew/bin/wasmtime PATH=/usr/bin:/bin .../hello.sh   -> argc: 1
$ "$S/with space/hello.sh" a                          -> argc: 2 (script and .wasm copied together)
$ .../http_hello.sh &  ; curl -sS http://127.0.0.1:8080/default
hello from /default (wasm32-wasip2, wasi:http/incoming-handler@0.2.12)
Serving HTTP on http://127.0.0.1:8080/
$ .../http_hello.sh 127.0.0.1:18480 --env SPIKE_VAR=x & ; curl http://127.0.0.1:18480/given
hello from /given (wasm32-wasip2, wasi:http/incoming-handler@0.2.12)
$ .../http_hello.sh -S inherit-network & ; curl http://127.0.0.1:8080/opt     (an option first keeps the default address)
hello from /opt (wasm32-wasip2, wasi:http/incoming-handler@0.2.12)
$ WASMTIME=<wasmtime 46.0.0> .../wasip2/.../hello.sh a     -> argc: 2
$ WASMTIME=<wasmtime 46.0.0> .../wasip3/.../hello.sh a     -> argc: 2
```

`exec` replaces the shell, so a signal to the script reaches wasmtime, which
serve.py's keeper relies on. The script finds the `.wasm` relative to
itself, so the pair can be copied anywhere together.

### 4.3 The generator

`$S/b2proto/run-script.jam`, whole in the spike directory. The parts that
matter:

```jam
.nl = "
" ;
# A dollar for the script's own $(...): b2 would expand one written in a string.
.dollar = "$" ;

local rule flags ( kind : properties * )
{
    local version = [ feature.get-values <toolset-clang:version> : $(properties) ] ;
    local eh = [ feature.get-values <exception-handling> : $(properties) ] ;
    local result ;
    if $(kind) = serve && $(version) = wasip2 { result += -S cli ; }
    if $(version) = wasip3 { result += -S p3 -W component-model-async ; }
    if $(eh) = "on" { result += -W exceptions=y ; }
    return $(result) ;
}

rule command ( targets * : sources * : properties * )
{
    local wasm = $(sources[1]:G=:D=) ;
    local FLAGS = [ flags command : $(properties) ] ;
    ...
    TEXT on $(targets) = [ header $(wasm) : command : $(version) ] ...
      [ words exec "\"$wasmtime\"" run $(FLAGS) $WASMTIME_FLAGS -- "\"$here/$(wasm)\"" "\"$@\"" ]
      "" ;
}

actions command
{
    echo "@($(<):E=$(TEXT:J=$(.nl)))" >/dev/null && chmod +x "$(<)"
}
```

Three b2 facts the prototype hit, all fixed in the version above:

- `on` is a jam keyword: `if $(eh) = on {` is "syntax error at keyword on";
  it has to be quoted.
- A string that holds an empty list, such as `"... $(FLAGS:J= ) ..."` with
  no flags, vanishes whole: the wasip2 no-exceptions script lost its `exec`
  line. Each line is built as a list of words and joined (`words`).
- `$(` inside a string is b2's (the "malformed subscript" error, and
  AGENTS.md chapter 12). The script's `$(dirname ...)` is written through
  `$(.dollar)`, which avoids shellcheck's SC2006 for backquotes.

Also, b2 tracks no action text: a change to the generator needs `-a`, which
webcpp's verdicts already pass.

How webcpp could adopt it:

- **Where:** the rules live in `tools/component/component.jam`, beside
  `.wasmtime-flags.p2/p3`, which they replace as the one place that knows
  wasmtime's flags.
- **Which programs:** `tools/webcpp.jam`'s `program` helper declares, for
  every program it builds for wasip2 or wasip3, a
  `make <program>.sh : <program> : @webcpp-component.command-script` with
  the program's requirements. That covers `webcpp.run`, `run-fail`,
  `example`, `link`'s `<name>-program` and `drive`. `webcpp.serve` declares
  `make <stem>-component.sh : <stem>-component : @webcpp-component.serve-script`.
  A conditional gives `<build>no` on every other toolset, read from
  `<toolset>` and its version only, as chapter 12 requires.
- **Name:** `<program>.sh`, the program's name with `.sh` for `.wasm`.
- **The tests run the scripts, so every lane checks them:**
  - `serve.py --script <component>.sh` runs
    `<component>.sh 127.0.0.1:0` instead of composing `wasmtime serve`
    itself. The keeper still sees wasmtime, through `exec`.
  - `run_example.py` gets `--launcher sh` and the script in place of the
    program.
  - b2's own `run` tests may keep `testing.launcher=wasmtime`, whose minimal
    line is the same on 47.
- **Host grants:** let a program declare them as a free feature, for
  example `<wasi-grant>filesystem` (`--dir=.`), `<wasi-grant>network`
  (`-S inherit-network,allow-ip-name-lookup`) or `<wasi-grant>environment`
  (`-S inherit-env`). The generator writes them into the script, so a test
  that reads files or opens sockets passes with the same grants a user gets.
  Grants rather than raw wasmtime flags keep the configuration
  host-neutral, for a later jco or Spin launcher. Everything else goes
  through `WASMTIME_FLAGS`.
- **Install:** an `install` of a WASI program lists both the `.wasm` and its
  `.sh`, since the script finds the `.wasm` by its own directory.

**Windows.** No WASI lane runs on Windows; the MSVC lanes are native. Emit
the `.sh` only, for now. If a Windows WASI lane comes, emit
`<program>.cmd` instead on a Windows build host (`[ os.name ] = NT`),
since the script is for the machine that built it. A sketch, not run:

```bat
@echo off
rem Generated by b2: runs hello.wasm, a wasm32-wasip2 command, with wasmtime.
setlocal
if not defined WASMTIME set "WASMTIME=wasmtime"
"%WASMTIME%" run -W exceptions=y %WASMTIME_FLAGS% -- "%~dp0hello.wasm" %*
exit /b %ERRORLEVEL%
```

## 5. Recommendations

1. **(a) Namespaces.** `webcpp::wasi::p2::http` in `webcpp/wasi/p2/http.hpp`
   and `webcpp::wasi::p3::http` in `webcpp/wasi/p3/http.hpp`, each with its
   own clean implementation and export macro. A front,
   `webcpp/wasi/http/entrypoint.hpp`, includes the one the configuration
   picks, by default from `__wasip2__`/`__wasip3__`, with
   `WEBCPP_WASI_HTTP_P2`/`_P3` as an override; exactly one.
2. **(b) One entry point.** `webcpp::wasi::http::http_main`, declared in an
   ABI-free `webcpp/wasi/http/main.hpp`. Both export macros call it, and
   `WEBCPP_WASI_HTTP_MAIN_BEGIN/END` keep their form.
3. **One component for both versions: no.** One source built twice. The
   component with both exports works on wasmtime but always answers through
   p3, cannot load on a host without p3, is refused by Spin, and costs 24%
   in size. One module with both libcs does not link.
4. **(c) Configuration.**
   - `WEBCPP_WASI_TARGET_P2/P3`, and `WEBCPP_WASI_HAS_SOCKETS` for p2 and
     p3.
   - `WEBCPP_WASI_HAS_THREADS` from `__wasi_cooperative_threads__`:
     undefined today on both targets.
   - `WEBCPP_WASI_HAS_EXCEPTIONS` from `__wasm_exception_handling__`.
   - Filesystem, clocks, random, environment and stdio are always there.
   - Whether the host grants files, environment or network goes in the run
     script, not in a macro.
5. **(d) Run scripts.** `make <program>.sh : <program> :
   @webcpp-component.<command|serve>-script` beside every WASI program. It
   holds the explicit flags of 4.1 and the program's declared grants, and
   honours `WASMTIME` and `WASMTIME_FLAGS`. It forwards `"$@"` to the
   program, or to `wasmtime serve` after an optional address (default
   `127.0.0.1:8080`). serve.py and run_example.py should run the scripts,
   so CI exercises them. No `.cmd` until a Windows WASI lane exists.

## 6. Files

Under the scratch directory `$S`
(throwaway; nothing committed anywhere):

- `lib/include/webcpp/wasi/`: the layout of 2.2 (`config.hpp`,
  `http/{response,main,entrypoint}.hpp`, `p2/http.hpp`, `p3/http.hpp`);
- `app/main.cpp`, `probeapp/main.cpp`; `worlds/{p2,p3,both}/wit/`;
  `both/` (the first, unrenamed generation);
- `build.sh`, `serve_probe.sh`, `out/` (every component and its generated
  bindings), `transcripts/` (serve.py outputs);
- `feat/`: the feature programs and their `.wasm` per target, `udp_echo.py`;
- `spin/` (Spin manifests and logs); `b2proto/` (the run-script generator,
  `Jamroot`, `user-config.jam`, and its `bin/` with the generated scripts);
- `wasm-tools`: a link to an earlier private project's pinned wasm-tools 1.259.0.
