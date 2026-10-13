# webcpp::wasi redesign: where the implementation stands, and how to resume

Written 2026-10-13, when the proposal was written from the owner's design and the spike. Nothing
of the redesign is built: no header, test, rule or page was written, and `implementation` is
null. The published library is wasi 0.1.0, https://github.com/webcpporg/wasi/tree/6e3c529, whose
tree is also the archived genesis's `reference/source/`
(`../../archive/2026-10-08-genesis/reference/source/`); that is the baseline every task changes.

## What exists

| Item | State |
| --- | --- |
| `design.md` | The owner's design of 2026-10-08, marked active, ending "Approved by the owner on 2026-10-09". The specification delta follows it. |
| `spike.md` | The spike that measured the design, with wasi-sdk 34, wasmtime 47.0.3, wit-bindgen 0.62.0 and b2 5.5.3. Its code blocks are the reference code (below). Its scratch directory was throwaway and is not kept. |
| webcpp::config | Implemented in 0.1.0 (https://github.com/webcpporg/config/tree/2fad162): the redesign needs its `<webcpp/config.hpp>` and `WEBCPP_HAS_EXCEPTIONS`. |
| The change | `proposal.md`, `design.md`, `tasks.md` and `specs/wasi/spec.md` here: 23 requirements added, 32 modified, 3 removed. |

## What the spike measured

Everything below is as `spike.md` states it; none of it was re-run when this proposal was
written.

- **One source, two components:** all six served tests it built (a seventh, `http_two_units`, was
  not built) pass on the new layout on wasip2, on wasip3 and in the dual component, 24 of 24
  transcripts by webcpp's own `serve.py`; the front chose the export from the target when built
  with no `-D`, and stopped on wasip1 with its own error (section 2.7).
- **The dual component is not worth building** (section 2.4 to 2.6): wasmtime 47 always answers
  through the p3 export and a host without p3 cannot load the component; Spin 4.2.2 refuses two
  handlers; +16.5 KB stripped (+24%); one module linked against both libcs fails in wasm-ld. A p3
  handler on the wasip2 libc and a p2 handler on the wasip3 libc both serve.
- **Features (section 3.3):** wasip2 and wasip3 offer the same through wasi-libc (files, sockets,
  clocks, random, environment, arguments, stdio), behind `--dir`, `-S inherit-network`,
  `allow-ip-name-lookup` and `--env`; no thread runs on either (wasip3 predefines
  `__STDCPP_THREADS__` and `std::thread` fails with ENOTSUP); wasip1 has no `<netdb.h>`.
- **Run scripts (section 4):** a b2 prototype wrote `<program>.sh` beside the `.wasm`; shellcheck
  reported nothing on all six, dash ran them, a path with a space worked, a missing wasmtime
  exited 127, and the scripts ran under wasmtime 46 and 47.
- **Not run:** the `.cmd` sketch (section 4.3), serving the jco output, and `http_two_units`.

## The reference code

The spike's code blocks are the reference an implementer starts from:

| Block | Where | Use |
| --- | --- | --- |
| The layout table and the front header | `spike.md` 2.2 | Task 2.4. Adapt: the design replaced its `WEBCPP_WASI_HTTP_P2`/`_P3` override by `WEBCPP_WASI_TARGET` and its `http_main` by the entry-point macros. |
| The p2 and p3 code to cut apart | `spike.md` 2.2 (generated from 0.1.0's `entrypoint.hpp`) | Tasks 2.2 and 2.3. The source is the archived `entrypoint.hpp`. |
| The wit-bindgen commands | `spike.md` 2.1 and 2.2 | Not needed: the `--rename` of interfaces only served the dual world, which is rejected. One world per target keeps today's plain names and `wit/http-p2.wit`, `wit/http-p3.wit`. |
| The macro table | `spike.md` 3.4 | Task 1.3 and 1.4. The design keeps three `HAS_*` macros and `WEBCPP_WASI_TARGET_P2`/`_P3`. |
| The feature table | `spike.md` 3.3 | Task 4.1, the host table of the page. |
| The command-line table | `spike.md` 4.1 | Task 3.1, the explicit flags of each script. |
| The sample scripts and the four `exec` lines | `spike.md` 4.2 | Task 3.4's expected text. |
| The generator and its three b2 facts | `spike.md` 4.3 | Tasks 3.1 and 3.2. |

## Where the design and the spike differ

The design is the owner's decision; the spike's code follows an earlier shape in these places.

- The spike keeps `webcpp::wasi::http::http_main` and `WEBCPP_WASI_HTTP_MAIN_BEGIN/END`; the design
  replaces them by a plain function of a `request` and `WEBCPP_WASI_HTTP_ENTRY_POINT(fn)`.
- The spike chooses the export by `WEBCPP_WASI_HTTP_P2`/`_P3`; the design by the compiler and
  `WEBCPP_WASI_TARGET`, and the build defines nothing.
- The spike keeps `response` in `webcpp::wasi::http` (`http/response.hpp`); the design puts what
  both versions share in `webcpp::wasi::detail`, brought into each `http` by `using`.
- The spike's `p2/http.hpp` and `p3/http.hpp` use the renamed `wasi_p2_*` and `wasi_p3_*` names of
  the dual world; with one world per target the names are today's `wasi_http_*` and
  `exports_wasi_http_incoming_handler_handle` and `exports_wasi_http_handler_handle`.
- The spike lists `WEBCPP_WASI_HAS_FILESYSTEM` and `WEBCPP_WASI_HAS_PROCESS_CLOCKS` and the host
  grants as a build feature; the design keeps three macros and leaves the grants out
  (`design.md` O3).

## Open

`design.md` O1 to O4: the native case of the front, the path of the shared headers, grants and
install of the script, and where the script rule is specified. Task 0.4 decides them with the
owner.

## How to resume

1. Read `design.md` (the owner's), then this proposal's `design.md` and `specs/wasi/spec.md`.
2. Task 0.5: a superproject worktree from `main`, with `libs/wasi` at 6e3c529 and `libs/config`,
   the toolchain of the superproject's `AGENTS.md` (Boost 1.92, b2 5.5.3, wasi-sdk 34, wasmtime
   47, wit-bindgen 0.62.0), and wasi's lanes run green before any change.
3. Work the tasks in order: tests first, seen red; group 1 (configuration and types), group 2 (the
   handlers and the entry point), group 3 (the run scripts, which touch the superproject's
   `tools/`), group 4 (page and project files).
4. Record in this file what was built and what was run, with dates, and set the proposal's status
   to `implementing`.
