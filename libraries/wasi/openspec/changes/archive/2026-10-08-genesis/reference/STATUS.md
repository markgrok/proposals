# webcpp::wasi: where the implementation stands, and how to resume

Written 2026-10-13, when this proposal took wasi in as a retroactive genesis. There is no
`spec.md` or `plan.md` of the library's own: its specification is `source/README.md`,
`source/AGENTS.md` and the page under `source/doc/` (the nine guarantees of
`source/doc/guarantees.adoc` are its invariants); `../tasks.md` is the checklist; `source/` is the
library's tree at `6e3c529`.

## Where it stands

The library was developed from 2026-10-08 to 2026-10-12 as the superproject's stage 3 (moving the
HTTP helper of an earlier private project into webcpp as `webcpp::wasi::http`), its repository's
history 30 commits, all as `R`, from `640d6ae` ("wasi: moved as webcpp::wasi::http, and the
README's example served on wasip2 and wasip3") to `6e3c529` ("meta: the maintainer's public
address"). It is release 0.1.0 and the superproject's submodule `libs/wasi`:

| What | Where |
| --- | --- |
| The library | `github.com/webcpporg/wasi`, branch `main` at `6e3c529` |
| The superproject | `libs/wasi` at `6e3c529` in the local clone of `github.com/webcpporg/webcpp` whose `HEAD` is `c53f3f9` |
| The page | built by the library's CI through the superproject's `library.yml`, published at `webcpporg.github.io/libs/wasi/` |

`source/` is exactly `git archive 6e3c529` of the library (checked by comparing the two trees:
no difference).

### What was measured

Nothing was run for this proposal: it was written by reading the tree, the tests' expected
files and the history. What the sources themselves state as measured:

- The toolchain: wasi-sdk 34, wit-bindgen 0.62.0, the `wasi:http` WIT of WASI 0.2.12 (crate
  `wasip2` 1.0.4) and 0.3.0 (crate `wasip3` 0.9.0), wasmtime 47.0.3 (`README.md`,
  `doc/introduction.adoc`, `doc/testing.adoc`).
- On wasmtime 47: the whole body of 1 MiB is taken in one write on wasip3 (`doc/guarantees.adoc`,
  guarantee 4); an out-of-range status or a refused content type is answered as the host's own
  `500 Internal Server Error` with `content-length: 205` (`test/http_bad_status.expected`);
  using context slot 0 on wasip3 trapped (`entrypoint.hpp`, `doc/wasip3.adoc`); a freed
  `"/text/plain"` reads `f0 0f 01 00 f0 0f 01 00` on wasip2 (`test/http_content_type_of_target.cpp`);
  a component built with `-fno-exceptions` instead of the exception flags answered the same, once,
  by hand, on wasip3 (`doc/wasip3.adoc`).
- The counts the page prints (`{n-tests-wasip2}`, `{n-served-wasip3}` and the like) are
  attributes the superproject computes; they are not in this tree and were not checked.

## Open items of 0.1.0

1. **Held by review only.** Guarantee 6 (a failure after the response is committed) and the second
   turn of wasip3's write loop cannot be made to happen on demand with wasmtime 47; no test covers
   them.
2. **`config.hpp` reads the developer's macros itself** "until wasi's redesign includes
   `<webcpp/config.hpp>`" (its own comment): the redesign is not planned in this tree.
3. **Not supported, by design:** request headers and body, response headers other than
   `content-type`, an allocator of the program's own (the page's Limits chapter): each would be a
   change of its own.
4. **No run of the matrix was seen** by this proposal; the CI's verdict for `6e3c529` is to be read
   from the test matrix (`webcpporg.github.io/report/`).

## How to resume

1. Work in a worktree of the superproject (`git worktree add ../webcpp-wasi-<slug> -b
   lib/wasi-<slug> main`, then `git submodule update --init --recursive`), following the
   superproject's `AGENTS.md`, chapter "Working on a library". `libs/wasi` is the published
   repository; `source/` is the same tree, for reading without a checkout.
2. Install the tools (see `source/AGENTS.md`, "The tools"): wit-bindgen 0.62.0, the two `wasi:http`
   WIT directories, wasmtime 47.0.3 and wasi-sdk 34, and register the toolsets `clang-wasip2` and
   `clang-wasip3`.
3. Follow the library's own rules (`source/AGENTS.md`): a test first (a served component with its
   `.requests` and `.expected` for a behaviour of the handler, in the lane `http` on both targets),
   the guarantees keep their numbers, the README's example and the page's copies change with
   `example/http_hello.*` and `example/by_hand/*` in the same commit, and the handler catches
   nothing.
4. Before saying a task is done: `b2 -a toolset=clang-wasip2 testing.launcher=wasmtime libs/wasi/test
   libs/wasi/example libs/wasi/test//http libs/wasi/example//http`, the same with
   `toolset=clang-wasip3`, the native `b2 -a libs/wasi/test libs/wasi/example`, `b2 libs/wasi/doc`
   and the superproject's lint; then the CI on every lane.
