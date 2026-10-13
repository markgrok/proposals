---
title: "webcpp::wasi redesign: design"
tags: [webcpp, wasi, design]
status: active
created: 2026-10-08
---

# webcpp::wasi redesign

The owner's requests and decisions of 2026-10-08. Investigation:
`~/.buzz/RESEARCH/WASI_REDESIGN_SPIKE.md` (one component for both targets,
what each target offers, run scripts), measured with wasi-sdk 34 and
wasmtime 47.0.3. The library today: `libs/wasi`, one `entrypoint.hpp` of
about 500 lines in which the p2 and p3 handlers are interleaved with `#if`,
and a `response.hpp`.

## 1. What wasi is, and why `response` exists

wasi-sdk provides wasi-libc, the POSIX part (files, sockets, clocks, stdio),
the same on wasip2 and wasip3. It provides nothing for `wasi:http`: the only
way to speak HTTP in a component is the C bindings wit-bindgen generates from
the WIT, which are resource handles, with a different API per target (p2:
fields, outgoing-response, outgoing-body, output-stream written in chunks of
at most 4096 bytes, response-outparam, every handle dropped by hand; p3: an
async API of streams, futures and a callback). wasi is the helper that hides
this: the program's function returns a plain `response` (status, content
type, body), and each target's handler does the rest, keeping the guarantees
of wasi's page (a body arrives whole, a refused status or content type
answers 500, nothing written back points into freed memory). The owner
confirmed this purpose.

## 2. Decisions

1. **Boost.** wasi depends on Boost like every webcpp library. Its
   `config.hpp` includes `<webcpp/config.hpp>`, the configuration library of
   webcpp (its own design: `~/.buzz/PLANS/WEBCPP_RELEASE_DESIGN.md`), which
   includes `<boost/config.hpp>`; wasi adds its own defines. The earlier "wasi
   needs no Boost" was inherited from the source project's README and is
   removed; the page's by-hand build names Boost's include path.
2. **One namespace per WASI version, as Boost.Process does**
   (`boost::process::v1`/`v2`, the configured one `inline`):
   - `webcpp::wasi::p2` and `webcpp::wasi::p3`, in `wasi/p2/` and `wasi/p3/`;
     the configured target's namespace is `inline`, so
     `webcpp::wasi::http::...` names the current target's;
   - each version holds only the feature namespaces it provides, as
     Boost.Asio's `posix` and `windows` exist only when the platform has
     them; today only `http` differs (`p2::http`: incoming-handler@0.2;
     `p3::http`: handler@0.3, async), and a namespace exists per version only
     where its implementation differs (files and sockets go through the same
     wasi-libc on both: no `p2::filesystem`);
   - what both share (`response`, `request`, the internals they have in
     common) lives in `webcpp::wasi::detail` and appears in each version's
     `http` by `using`; there is no public `wasi::http` outside the versions,
     which would be ambiguous against an inline version's `http`;
   - each target's handler is in its own header, readable on its own, with no
     `#if` inside its logic; the only choice is made once, in the front
     header.
3. **Target selection.** `config.hpp` picks the target from the compiler's
   `__wasip2__`/`__wasip3__`; `WEBCPP_WASI_TARGET` (`p2` or `p3`) overrides
   it, as `BOOST_PROCESS_VERSION` does.
4. **One entry point, a plain function.** The program writes an ordinary C++
   function and names it once:

   ```cpp
   #include <webcpp/wasi/http.hpp>

   webcpp::wasi::http::response hello(webcpp::wasi::http::request const& request) {
       return {.status = 200, .content_type = "text/plain", .body = "hello"};
   }

   WEBCPP_WASI_HTTP_ENTRY_POINT(hello)   // at global scope, once per program
   ```

   - `WEBCPP_WASI_P2_HTTP_ENTRY_POINT(fn)` in `<webcpp/wasi/p2/http.hpp>`
     defines p2's export (`exports_wasi_http_incoming_handler_handle`), which
     converts the request and calls `fn`;
   - `WEBCPP_WASI_P3_HTTP_ENTRY_POINT(fn)` in `<webcpp/wasi/p3/http.hpp>`
     defines p3's exports (`exports_wasi_http_handler_handle` and the async
     callback), which call `fn`;
   - `WEBCPP_WASI_HTTP_ENTRY_POINT(fn)` in `<webcpp/wasi/http.hpp>` forwards
     to the configured target's macro;
   - macro names mirror the namespace path, the Boost convention
     (`BOOST_ASIO_...`, `BOOST_PROCESS_V2_...`), since the preprocessor has
     no namespaces;
   - the function has no ABI, so it compiles natively and a native test calls
     it directly;
   - `request` carries what the main receives today, the method and the
     target as views valid during the call; anything more (headers, body) is
     a later extension, not part of this redesign.
   - This replaces `WEBCPP_WASI_HTTP_MAIN_BEGIN`/`_END` and
     `WEBCPP_WASI_HTTP_EXPORTS`: a breaking change, made before webcpp's
     first release (0.1), and listed in its release notes.
5. **One source, two components; no dual component.** Measured: one component
   exporting both handlers builds and validates, but wasmtime always answers
   through the p3 export, a host without p3 cannot load it (it imports
   `wasi:http/types@0.3.0`), Spin refuses two handlers, and it grows 24%; one
   module linked against both libcs fails in wasm-ld. So a program is built
   once per target, one export per component, both macros calling the same
   function.
6. **Capability macros, as `BOOST_ASIO_HAS_*`.** `config.hpp` defines what
   the target offers, from the compiler's macros, so a program can ask:
   `WEBCPP_WASI_HAS_SOCKETS` (`__wasip2__ || __wasip3__`; wasip1 has none),
   `WEBCPP_WASI_HAS_THREADS` (`__wasi_cooperative_threads__`; measured: no
   thread runs on either target today, and wasip3's `std::thread` compiles
   then fails with ENOTSUP), `WEBCPP_WASI_HAS_EXCEPTIONS`
   (`__wasm_exception_handling__`), next to `WEBCPP_WASI_NO_EXCEPTIONS`.
   The page tables what each needs from wasmtime (`--dir` for files,
   `-S inherit-network` for sockets, `allow-ip-name-lookup` for names,
   `--env` for the environment).
7. **A run script from the build, for every WASI program.** Beside each
   `.wasm`, b2 writes `<program>.sh`, a POSIX `sh` script that finds wasmtime
   (`$WASMTIME`, else PATH), passes the right `-S`/`-W` flags for the
   artifact's kind and target (a command: `wasmtime run`; an HTTP component:
   `wasmtime serve`, with an optional address, `127.0.0.1:8080` by default),
   adds `$WASMTIME_FLAGS`, and forwards `"$@"`. It is a rule of the build
   system (`tools/webcpp.jam` / `tools/component/component.jam`), so every
   library's WASI programs get one; `serve.py` and `run_example.py` run the
   programs through their scripts, so CI tests the scripts. No `.cmd` until
   a Windows WASI lane exists.

## 3. Testing

- Every served test of today, ported to the new layout, on both targets (the
  spike ran six of them on it, 24 of 24).
- A native test of a user function, called directly.
- A test that the configured target's namespace is the inline one, that
  `WEBCPP_WASI_TARGET` overrides it, and that both explicit macros exist.
- The run scripts: generated, shellcheck-clean, run by the served tests and
  examples, working under `dash` and with a space in the path.
- The capability macros against the measured table.

## 4. Order

It needs `webcpp::config` (the configuration library), so it comes after
that library exists; it replaces wasi's API before 0.1, so it comes before
webcpp's first release. Stage 4 (trystero) does not depend on it.

Approved by the owner on 2026-10-09.
