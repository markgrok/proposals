## Why

wasi 0.1.0 is one `entrypoint.hpp` of about 500 lines in which the wasip2 and the wasip3 handlers
are interleaved with `#if`, and a program writes its main between two macros that open and close
a function the library declares. That layout has five problems, all found when the owner reviewed
it on 2026-10-08:

- A program cannot be tested natively: its main is a definition of `http_main` inside macros that
  need the component ABI, so only a served component runs it.
- The two versions of WASI cannot be read apart: every function of the handler has an `#ifdef`
  inside it, and the build must say which version it wants (`WEBCPP_WASI_HTTP_P2` or `_P3`) though
  the compiler already knows.
- The library says "wasi needs no Boost", inherited from the project it was written for, while
  every webcpp library depends on Boost and on webcpp's configuration library.
- A program cannot ask what the target offers (sockets, threads, WebAssembly exceptions); the
  spike measured that wasip2 and wasip3 offer the same through wasi-libc, that no thread runs on
  either, and that what runs is the host's grant.
- A WASI program is run by a wasmtime command line that the test harness composes and nobody else
  can copy: the flags differ by kind (command or HTTP component), target and exception setting.

The owner approved the redesign on 2026-10-09 (`reference/design.md`), and the spike
(`reference/spike.md`) measured it with wasi-sdk 34 and wasmtime 47.0.3: one entry point, one
namespace per version, target features and run scripts. It is made before webcpp's first release,
so that the breaking change costs no user.

## What Changes

- **One namespace per WASI version, as Boost.Process has `v1` and `v2`:** `webcpp::wasi::p2` and
  `webcpp::wasi::p3`, in `wasi/p2/` and `wasi/p3/`; the configured version's is `inline`, so
  `webcpp::wasi::http::...` names the configured target's; a version holds only the feature
  namespaces it provides (today only `http`); what both share (`response`, `request`, the
  internals) lives in `webcpp::wasi::detail` and appears in each `http` by `using`.
- **Each handler in its own header,** `<webcpp/wasi/p2/http.hpp>` and `<webcpp/wasi/p3/http.hpp>`,
  readable alone with no `#if` in its logic; the one choice is made in the front,
  `<webcpp/wasi/http.hpp>`.
- **Target selection:** the compiler's `__wasip2__` and `__wasip3__` choose the version;
  `WEBCPP_WASI_TARGET` (`p2` or `p3`) overrides it, as `BOOST_PROCESS_VERSION` does.
- **BREAKING: one entry point, a plain function.** The program writes
  `response fn(request const&)` and names it once with `WEBCPP_WASI_HTTP_ENTRY_POINT(fn)`;
  `WEBCPP_WASI_P2_HTTP_ENTRY_POINT` and `WEBCPP_WASI_P3_HTTP_ENTRY_POINT` name a version. The
  function has no ABI, so it compiles natively. `WEBCPP_WASI_HTTP_MAIN_BEGIN`, `_END` and
  `WEBCPP_WASI_HTTP_EXPORTS`, and `http_main`, are removed. **BREAKING:** the build no longer
  defines `WEBCPP_WASI_HTTP_P2` or `_P3`, and the headers move to `<webcpp/wasi/http.hpp>`.
- **A `request`** (`method`, `target`, views valid during the call) replaces the main's two
  parameters.
- **Boost and webcpp::config:** `config.hpp` includes `<webcpp/config.hpp>` first, which includes
  `<boost/config.hpp>`; `WEBCPP_WASI_NO_EXCEPTIONS` follows `WEBCPP_HAS_EXCEPTIONS`; "wasi needs
  no Boost" is removed from the page.
- **One source, two components; no dual component,** measured and recorded.
- **Capability macros, as `BOOST_ASIO_HAS_*`:** `WEBCPP_WASI_HAS_SOCKETS`,
  `WEBCPP_WASI_HAS_THREADS`, `WEBCPP_WASI_HAS_EXCEPTIONS`, and a table of what each needs from
  wasmtime.
- **A run script from the build for every WASI program:** `<program>.sh` beside the `.wasm`,
  written by a rule of the build system; the served tests and the examples run through it.
- **Tests, documentation and project files** follow: the seven served tests ported, new tests of
  the target, the capabilities, the native call and the scripts; the page's chapters "Your
  function", "Versions and targets" and "Running a program".

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `wasi`: 32 requirements modified, 23 added and 3 removed (replaced under new names).

## Impact

- **Kind:** a change to a library of webcpp's own.
- **Implementation:** not started. `reference/design.md` is the owner's design, approved on
  2026-10-09; `reference/spike.md` is the spike that measured it, whose code blocks are the
  reference code (`reference/STATUS.md` names them). The published library is wasi 0.1.0
  (https://github.com/webcpporg/wasi/tree/6e3c529), whose archived genesis holds its source.
- **Status:** accepted on 2026-10-09; nothing is built.
- **Dependencies:** webcpp::config 0.1.0 (implemented) and Boost, for every header; shellcheck and
  dash for the check of the run scripts alone.
- **Other proposals:** the run scripts are a rule of the superproject's build tools
  (`tools/webcpp.jam`, `tools/component/component.jam`, `serve.py`, `run_example.py`), shared by
  every library; the organization's specification may need a matching change (design.md, O4).
  The change is made before webcpp's first release and listed in its release notes.
