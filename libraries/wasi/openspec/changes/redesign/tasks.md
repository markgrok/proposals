# Tasks

None is built. The design asks for no plan of its own: `reference/design.md` section 4 orders the
work (after webcpp::config, before webcpp's first release) and `reference/spike.md` holds the
reference code, so this list is the plan. Group 0 is what precedes the work; groups 1 to 4 build
`design.md` D1 to D8; group 5 releases it. Every test is written and seen to fail before the code
that passes it.

## 0. Before: what the redesign needs

- [x] 0.1 The owner approved the design on 2026-10-09 (`reference/design.md`, section 4)
- [x] 0.2 The spike measured the entry point, the namespaces, the target features and the run scripts with wasi-sdk 34 and wasmtime 47.0.3 (`reference/spike.md`)
- [x] 0.3 webcpp::config 0.1.0 exists (implemented, https://github.com/webcpporg/config/tree/2fad162): `<webcpp/config.hpp>`, `WEBCPP_HAS_EXCEPTIONS`, the target macros
- [ ] 0.4 Decide O1 (the native front), O2 (the shared headers' paths), O3 (grants and install) and O4 (where the script rule is specified) with the owner
- [ ] 0.5 A superproject worktree from `main` with `libs/wasi` at 6e3c529 and `libs/config`; run wasi's lanes (native, wasip2, wasip3, the page, the lint) green before touching anything, and record the result in `reference/STATUS.md`

## 1. Configuration and shared types (design D1, D3, D6, D8)

- [ ] 1.1 Tests first, red: `target_namespace`, `target_override_p2`, `target_override_p3`, `target_refused`, `capabilities`, `request`
- [ ] 1.2 `config.hpp`: include `<webcpp/config.hpp>` first; `WEBCPP_WASI_NO_EXCEPTIONS` by config's two lines; `config_test` adapted (scenarios of "Config: WEBCPP_WASI_NO_EXCEPTIONS")
- [ ] 1.3 `config.hpp`: `WEBCPP_WASI_TARGET_P2`/`_P3` from the compiler, the `WEBCPP_WASI_TARGET` override and its refusal, the macro that makes the configured namespace `inline`
- [ ] 1.4 `config.hpp`: `WEBCPP_WASI_HAS_SOCKETS`, `WEBCPP_WASI_HAS_THREADS`, `WEBCPP_WASI_HAS_EXCEPTIONS` against the measured table (`reference/spike.md` sections 3.3 and 3.4)
- [ ] 1.5 `detail/response.hpp` (the response moved, unchanged) and `detail/request.hpp`; `response_test` and `request_test` natively and on both targets
- [ ] 1.6 `build.jam`: the alias `wasi` brings `/webcpp/config//config`; the alias `http` drops the toolset defines

## 2. The two handlers and the entry point (design D2, D4, D5)

- [ ] 2.1 The shared internals in `webcpp::wasi::detail` (`view_of`, `method_name`, `sendable`, `headers_of`)
- [ ] 2.2 `p2/http.hpp`: today's p2 code (`refuse`, `write_all`, `respond`, `handle`) in `webcpp::wasi::p2::http::detail`, the `using` of the shared names, `WEBCPP_WASI_P2_HTTP_ENTRY_POINT`
- [ ] 2.3 `p3/http.hpp`: today's p3 code (`wait_for`, `write_all`, `resolve_trailers`, `respond`, `handle`) in `webcpp::wasi::p3::http::detail`, the shared names, `WEBCPP_WASI_P3_HTTP_ENTRY_POINT` and its callback
- [ ] 2.4 `http.hpp`, the front: the one conditional, `WEBCPP_WASI_HTTP_ENTRY_POINT`, and the native case (O1); `wasi.hpp`; delete `http/entrypoint.hpp` and `http/response.hpp`
- [ ] 2.5 Port the seven served tests and `http_hello`, `boost_json_http` to the plain function, their `.requests` and `.expected` untouched; add `http_explicit` and `function_native`; join `http_explicit` to the lane
- [ ] 2.6 `test/Jamfile`: the headers-alone tests of the new headers, `target_refused` in place of `neither_target_refused` and `both_targets_refused`
- [ ] 2.7 Every served test passes on wasip2 and wasip3 (the spike ran six of them, 24 of 24, on its layout)

## 3. Run scripts (design D7)

- [ ] 3.1 `tools/component/component.jam`: `command-script` and `serve-script`, from the generator of `reference/spike.md` section 4.3 (`"on"` quoted, each line built as words, the script's `$(` written through `.dollar`), replacing `.wasmtime-flags.p2/p3`
- [ ] 3.2 `tools/webcpp.jam`: the helper that declares a program declares its `.sh` for the WASI toolsets, `<build>no` elsewhere; `webcpp.serve` declares `<stem>-component.sh`
- [ ] 3.3 `serve.py --script` and `run_example.py --launcher sh`; the served tests and the examples run through the scripts
- [ ] 3.4 The check of the scripts: generated, shellcheck-clean, dash, a path with a space, exit 127 without wasmtime
- [ ] 3.5 Measure a script against wasmtime 46 and 47 as the spike did, and record it

## 4. Documentation and project files

- [ ] 4.1 The page: `introduction.adoc` (Boost and config in the requirements), `function.adoc` in place of `main.adoc`, `targets.adoc` with the host table, `running.adoc`, `build.adoc` (by hand with the new include paths), `guarantees.adoc` (guarantee 1 and its holders)
- [ ] 4.2 Doc Comments of every public symbol, the reference's parse flags (`WEBCPP_WASI_TARGET=p2`, `=p3`), `doc/mrdocs.yml`
- [ ] 4.3 `example/by_hand/build.sh` regions `p2` and `p3`; the README (sections, header table, copies of the example); `AGENTS.md`; `.clang-format`; `meta/libraries.json`'s description
- [ ] 4.4 `doc/changelog.adoc`: the Unreleased lines of the redesign, the breaking ones marked

## 5. Release

- [ ] 5.1 the superproject's lint, the page without warning, every lane green on native, wasip2 and wasip3, CI green, in webcpp's first release; set this proposal's status, release and implementation
