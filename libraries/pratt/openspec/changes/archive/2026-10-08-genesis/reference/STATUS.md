# webcpp::pratt: where the implementation stands, and how to resume

Written 2026-10-13, when this proposal took pratt in retroactively as its genesis. `design-source.md`
and `draft.md` are the design (approved 2026-10-08) and the owner's draft prompt with its reference
implementation (Appendices A to D); `../tasks.md` is the checklist; `source/` is the library's
tree at `d78451b`.

## Where it stands

The library was developed from 2026-10-08 to 2026-10-12 in a worktree of the superproject, its
repository's history 42 commits, all as `R`, from `3305002` ("the library's skeleton, its errors
and their category") to `d78451b` ("meta: the maintainer's public address").

| What | Where |
| --- | --- |
| The library | `github.com/webcpporg/pratt`, `origin` of the local repository, at `d78451b` |
| The superproject | its checkout seen on 2026-10-13 records `libs/pratt` at `d78451b` |
| The page | built by that commit's CI at `webcpporg.github.io/libs/pratt/` (the address the README names; not opened) |

`source/` is exactly `git archive d78451b` of the library (compared with `diff -r`, no
difference). Metadata of this proposal: status implemented, release 0.1.0.

### What was measured

Only what could be seen is stated here.

- By this proposal, 2026-10-13, outside b2, with Apple clang 21.0.0, `-std=c++20 -O0`, Boost
  from Homebrew and the superproject's `config` headers on the include path: the eight test
  programs (`errors`, `engine`, `sexpr`, `lexer`, `calculator`, `allocation`, `depth`, `openui`)
  compiled and each printed `No errors detected.`; the eleven example programs compiled and ran,
  and the output of each (the REPLs fed their `.input`) was identical to its `.expected` (`diff`
  silent). Not run: any wasm lane, Emscripten, GCC, MSVC, the lint, the documentation build.
- The depth measurement (wasi-sdk 34, clang 23.1.0, wasmtime 47.0.3) is the table in
  `include/webcpp/pratt/parser.hpp` and in `doc/limits.adoc`; it was taken by the library's author
  on 2026-10-08 and again after the frames shrank (commits `edd1e73`, `3f02808`), and was not
  repeated by this proposal.
- The library's CI results are not recorded in the sources; no run id is known to this
  proposal.

## Open items

1. **Counts on the page.** `doc/testing.adoc` takes the counts of tests and examples per target
   from attributes (`{n-tests-native}`, `{n-examples}`, ...) that the superproject's doc tool
   supplies; their values were not checked here.
2. **The publication date** of `webcpporg/pratt` and the release in which `libs/pratt` entered
   the superproject are the controller's metadata (release 0.1.0); the sources show only the last
   commit, 2026-10-12.
3. **The parser of openui-lang.** The micro-subset in `example/openui/grammar.hpp` is its seed;
   the parser itself is not specified anywhere yet.

## How to resume

1. Work in a worktree of the superproject (`git worktree add ../webcpp-pratt-<slug> -b
   lib/pratt-<slug> main`, then `git submodule update --init --recursive`), following the
   superproject's `AGENTS.md`, chapter "Working on a library". `libs/pratt` is the published
   repository; `source/` is the same tree, for reading without a checkout.
2. Follow the library's own rules (`source/AGENTS.md`): tests first; the error values 1 to 9 and
   the category's name and identity never change, a new error takes the next value; every recursion
   goes through `expression()`; the headers contain no `throw`, `try` or `catch`;
   `<webcpp/pratt/config.hpp>` first in every public header; a test or example never states how
   many allocations a standard library makes.
3. A change to the frames of `expression`, the helpers, the calculator's nud and led or the
   micro-subset's may move `default_max_depth`: measure again on wasip2, wasip3 and Emscripten in
   their debug build, and update the table above the constant in `parser.hpp`, the value, and the
   "Limits" chapter together. `depth_test` fails on wasm, as a trap, when the default no longer
   fits.
4. A change to a program's output checks the `lines=` ranges of `calculator.adoc` and
   `openui.adoc`; line 30 of `example/calculator_repl.input` (ten thousand `(` and a `1`) stays
   where it is, or the range `lines=1..29;31..33` moves with it. The README copies
   `example/pratt_first_grammar.cpp[tag=program]` and its output, and doc-check compares them.
5. Before saying a task is done: the lint, `b2 -a libs/pratt/test libs/pratt/example` on native,
   `toolset=emscripten`, `toolset=clang-wasip2 testing.launcher=wasmtime` and
   `toolset=clang-wasip3 testing.launcher=wasmtime`, `b2 libs/pratt/doc`, and CI on every lane.
