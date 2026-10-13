# webcpp::xactor: where the implementation stands, and how to resume

Written 2026-10-13, when this proposal took xactor in as a retroactive genesis. xactor has no
separate specification or plan: its own documents (`source/doc/*.adoc`, the Doc Comments of its
headers, `source/AGENTS.md`, `source/README.md`, `source/test/CONVERSION.md`) are its specification,
`../specs/xactor/spec.md` states them as requirements, `../tasks.md` is the checklist reconstructed
from the commit history, and `source/` is the library's tree at `62f3e77`.

## Where it stands

The library was developed from 2026-10-07 to 2026-10-12 (the first commit `284b68a`, 24 commits in
all, the last `62f3e77`, "meta: the maintainer's public address"). It is the code of an earlier
private project's actor layer, moved into a repository of its own and renamed `webcpp::xactor`.

| What | Where |
| --- | --- |
| The library | `github.com/webcpporg/xactor`, branch `main`, commit `62f3e77` (release 0.1.0) |
| The superproject | `github.com/webcpporg/webcpp`, whose submodule `libs/xactor` holds it; the superproject supplies `webcpp.jam`, the lint, the toolsets and the reusable CI workflow `library.yml` |
| The page | built from `doc/xactor.adoc` by the superproject's documentation toolchain (Asciidoctor.js and MrDocs); published at `webcpporg.github.io/libs/xactor/` according to `README.md` |

`source/` is exactly `git archive 62f3e77` of the library: a recursive diff of the two directories
printed no difference (checked 2026-10-13).

### What was measured

Seen while writing this proposal, 2026-10-13, on macOS with Apple clang 21.0.0 and Homebrew's
Boost 1.92, compiling `source/` directly with `clang++ -std=c++20` (not through b2), outside the
proposal tree, against the `libs/config/include` of a local checkout of the superproject:

- `scheduler_test.cpp`, `lifecycle_test.cpp`, `fuel_test.cpp` and `create_actor_test.cpp`: each
  compiled and ran, printed `No errors detected.` and exited 0.
- The eight examples (`xactor_quick_start`, `lifecycle`, `fuel`, `timers`, `drivers`, `testing`,
  `msm`, `asio`): each compiled, exited 0, and its standard output equalled its `.expected` file
  (the `testing` one is empty).
- `config_test.cpp` compiled in the three ways `test/Jamfile` declares (as built; with
  `WEBCPP_XACTOR_NO_EXCEPTIONS` and `WEBCPP_TEST_XACTOR_DEVELOPER_DEFINED`; with `WEBCPP_NO_EXCEPTIONS`
  and `WEBCPP_TEST_XACTOR_ALL_OFF`).
- Each of the ten headers under `include/webcpp/xactor/` and `xactor.hpp`, as the only include of a
  translation unit, passed `-fsyntax-only -fno-exceptions -DBOOST_NO_EXCEPTIONS`, and so did
  `xactor_quick_start.cpp`.
- A scratch program (not kept) confirmed the category's `name()`, `message()` for 0, 1, 2 and 99, the
  cut `message(value, buffer, size)`, the defaults of `budgets` and of a new `scheduler`, the first
  addresses, the sequence numbers and the log entries of a two-actor delivery, which the scenarios of
  `../specs/xactor/spec.md` state.

Stated by the sources and not seen by me: `test/CONVERSION.md` records 55 cases, 295 assertions in
the source and 636 at run time (old and new equal), and three planted defects that fail the same
cases in the old and the new suites; the page states that every program it shows was compiled and run
natively and, but for the Asio one, on wasm32-wasip2 and wasm32-wasip3 under wasmtime. I did not run
b2, the WASI toolsets, wasmtime, MrDocs, Asciidoctor, the superproject's lint, the libstdc++ lane or
CI, and I know no CI run number for `62f3e77`.

## Open items found while reading the tree

1. **Tests that the specification states and no test checks.** Nothing in `test/` calls
   `error_category()`, `name()` or `message()` (the category's name, the strings, the truncating
   overload), `sequence_number::next` or `envelope_log`'s `operator==` on unequal logs; they are
   covered by the examples (`==` on equal logs) and, for the rest, true by the source alone.
2. **The page's table of tests** (`doc/testing.adoc`, "How xactor is tested") lists the four
   lightweight_test programs and leaves out the three `config` compile tests, which the text after
   the table describes.
3. **Boost's version** is stated by the page (1.92 or newer) and enforced by the superproject's
   Jamroot; `build.jam` itself checks no version.
4. **`run_one` returns a `result<bool>` that never holds an error** today; the drivers forward a
   failure that cannot happen. Kept on purpose (Doc Comment of `scheduler::run_one`).
5. **`test/CONVERSION.md` names the earlier private project's commits** (as `main` and `cc11cec`) and
   the old target names; it is a record of the conversion and stays as it was made.
6. **Later work is not specified here.** Placing messages and actors' state in shared or relocatable
   memory, and the integration of asynchronous libraries with Boost.Asio or Boost.Cobalt on top of
   the actor system, have been discussed for later stages; none of it is part of 0.1.0 and no change
   exists for it yet.

## How to resume

1. Work in a worktree of the superproject (`git worktree add ../webcpp-xactor-<slug> -b
   lib/xactor-<slug> main`, then `git submodule update --init --recursive`), following the
   superproject's `AGENTS.md`, chapter "Working on a library". `libs/xactor` is the published
   repository; `source/` is the same tree, for reading without a checkout.
2. Every change follows `source/AGENTS.md`: determinism is the library's point (an order that
   depends on anything but the enqueue order, `clock_tick` and `release_next` is a bug); errors are
   values; `config.hpp` comes first in every public header; the values of `errc` and `status` are
   never changed or reused; Boost.Asio stays out of WASI builds; a driver drains with `poll`.
3. Test first: a case in the program of its subject (`scheduler`, `lifecycle`, `fuel` or
   `create_actor`), named after the rule it checks, citing the guarantee as `(doc: #xactor-invariant-<n>)`.
   A new guarantee takes the next number and keeps it.
4. A new public symbol gets a Doc Comment (the reference is built strict), and the page and its
   example change in the same commit; a section renamed changes every `@see` and `(doc: #...)` that
   names it.
5. Before saying a task is done: the superproject's lint, `b2 -a libs/xactor/test libs/xactor/example`
   natively and for `toolset=clang-wasip2` and `toolset=clang-wasip3` with `testing.launcher=wasmtime`,
   `b2 -a libs/xactor/doc`, then CI on every lane.
