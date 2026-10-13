# webcpp::xstate: where the implementation stands, and how to resume

Written 2026-10-13, when this proposal took xstate in as a retroactive genesis of a published
library. There is no `spec.md` or `plan.md` in this directory: the library's documentation
(`source/doc/*.adoc`, `source/README.md`, `source/AGENTS.md` and `source/test/CONVERSION.md`) is
its specification, `../specs/xstate/spec.md` states it as requirements, `../tasks.md` is the
checklist and `source/` is the library's tree at `5299db2`.

## Where it stands

The library has 27 commits, all as `R`, from `631e737` (the move of the code of an earlier private project
into webcpp as `webcpp::xstate`, 2026-10-07) to `5299db2` ("meta: the maintainer's public
address", 2026-10-12). It is in release 0.1.0 of webcpp:

| What | Where |
| --- | --- |
| The library | `github.com/webcpporg/xstate`, `main` at `5299db2` |
| The superproject | `github.com/webcpporg/webcpp`, which holds it as the submodule `libs/xstate` |
| The page | `webcpporg.github.io/libs/xstate/`, built by `b2 libs/xstate/doc` |

`source/` is exactly the 680 files of `git ls-tree -r 5299db2` of the library. The library was not
changed by this proposal.

### What was measured

From the sources (not re-run here):

- `test/CONVERSION.md`, 2026-10-07: the object sections of each actor test program counted with
  `-ffunction-sections -fdata-sections` by clang 18 and g++ 14 on Linux against Boost 1.92 and
  libstdc++ (`actors_lifecycle` 37,043 and 39,806, `actors_sending` 37,161 and 39,936,
  `actors_children` 36,753 and 39,441, `actors_time` 38,138 and 40,985, `actors_host` 38,507 and
  41,395, `actors_cases` 41,017 and 43,970, `cases` 31,121 and 33,316), which is why the five actor
  programs and `actors_cases` take `/bigobj` on MSVC; MSVC itself was not available.
- The page states the lanes of the continuous integration (GCC 14 and 15, Clang 18 and 22, Apple
  Clang, MSVC 14.3 and 14.5, wasm32-wasip2 and wasm32-wasip3, the own lane `xstate, oracle`, the
  documentation and the lint); this proposal did not see a run of them.

By this proposal, on 2026-10-13, on macOS with Apple clang 21.0.0, Boost 1.92 (Homebrew), Node
26.7.0 and npm 11.19.0, in scratch directories outside the repositories, with compile lines of its
own (`clang++ -std=c++20 -O0 -isystem <boost> -I source/include -I <config>/include -I
<xactor>/include`, `boost_json.cpp` compiled once and linked), not b2, against the working trees of
the publishing checkout's `libs/config` (`2fad162`) and `libs/xactor` (`62f3e77`):

- Examples: all 146 programs (89 in `example/xstate/`, 57 in `example/actors/`) compiled and run
  natively, and every output equal to its `.expected` file (146 of 146).
- Tests: `values`, `machine`, `cursor`, `actors_lifecycle`, `actors_sending`, `actors_children`,
  `actors_time` and `actors_host` built and run: exit 0 (8 of 8). `core_alone` and
  `user_aggregates`: built and run, exit 0. `config_test.cpp` compiled three ways (no definition;
  `WEBCPP_XSTATE_NO_EXCEPTIONS` with `WEBCPP_TEST_XSTATE_DEVELOPER_DEFINED`; `WEBCPP_NO_EXCEPTIONS`
  with `WEBCPP_TEST_XSTATE_ALL_OFF`): all three compile.
- Case suites: `cases_test.cpp` and `actors_cases_test.cpp`, with Boost.Test's header-only
  framework (`tools/boost_test_runner.cpp` compiled with `BOOST_TEST_MODULE`) and
  `WEBCPP_TEST_XSTATE_FIXTURES` set to `source/test/fixtures`: "Running 508 test cases... No errors
  detected" and "Running 231 test cases... No errors detected".
- Oracle: `npm ci --ignore-scripts` from `source/test/oracle/package-lock.json` installed `xstate`
  5.33.2; `node --conditions=development oracle.mjs` over `test/fixtures/cases` and `actors.mjs`
  over `test/fixtures/actors/cases` wrote trees that the superproject's `tools/oracle/compare.py`
  found byte-identical to `test/fixtures/expected` and `test/fixtures/actors/expected`.
- Twins: the superproject's `tools/oracle/twins.py` (suffix `.mjs`, the words `node
  --conditions=development --test-reporter=spec --test-reporter-destination=stderr`) over
  `source/example` and a copy of `source/test/oracle/twins` placed beside the installed
  `node_modules`: exit 0, and `--list` ends `total 146 114 22 10` (114 agree, 22 diverge, 10 have
  no twin).
- `source/doc/counts.py source`: the 17 counts quoted in the requirement "Docs: Counts on the page
  come from the data".

## Not established here

- No run of `b2`, so no confirmation of the declared target lists, `webcpp.headers-alone` (24
  public headers), the lint, the documentation build (Asciidoctor.js, MrDocs strict, the
  `(doc: #...)` citations check) or the link check into xactor's page.
- No wasm32-wasip2 or wasm32-wasip3 run (no wasi-sdk or wasmtime was used) and no GCC or MSVC
  build; the headers were compiled with Apple clang alone.
- No `-fno-exceptions` compile of the headers, tests and examples (the lint's check).
- The result of the continuous integration of `5299db2`.
- The publication date of release 0.1.0 and of the page.

## Open items

1. The unported parts of XState 5.33.2 (the overview's list): `enqueueActions`, callback and
   observable actors, `fromTransition`, persistence of actors, `toPromise`/`waitFor`,
   `syncSnapshot` and so `onSnapshot`, `spawn` inside `assign`, `machine.provide()`.
2. The owner's direction on allocators and a document type of webcpp's own in place of
   Boost.JSON (design, Open Questions).
3. MSVC: the `/bigobj` decision rests on section counts made with other compilers.

## How to resume

1. Work in a worktree of the superproject (`git worktree add ../webcpp-xstate-<slug> -b
   lib/xstate-<slug> main`, then `git submodule update --init --recursive`), following the
   superproject's `AGENTS.md`, chapter "Working on a library". `libs/xstate` is the published
   repository; `source/` is the same tree, for reading without a checkout.
2. Every change follows the library's own rules (`source/AGENTS.md`): errors are values, a header
   never throws, `config.hpp` first, the machine core never includes xactor or the actor layer, a
   behaviour that XState also has is proven by a case or a twin on XState 5.33.2 and not by an
   expectation written by hand, the expected results written only by
   `b2 libs/xstate/test/oracle//update-expected`, every count on the page computed, every
   guarantee cited by its anchor, `example/Jamfile` naming every program.
3. A new machine behaviour starts with a case file under `test/fixtures/cases/` or
   `test/fixtures/actors/cases/` and the oracle's expected result; a new difference from XState
   starts with a `diff_` example, its twin and its entry in the appendix.
4. Before saying a task is done: the lint, `b2 -a libs/xstate/test libs/xstate/example`, the same
   with `toolset=clang-wasip2 testing.launcher=wasmtime` and `toolset=clang-wasip3
   testing.launcher=wasmtime`, `b2 -a libs/xstate/test/oracle//oracle`, `b2 libs/xstate/doc` and
   the site, locally; then CI on every lane.
