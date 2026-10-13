# webcpp::config: where the implementation stands, and how to resume

Written 2026-10-13, when this proposal took config in as a retroactive genesis. `spec.md` and
`plan.md` are the specification (approved by the owner on 2026-10-09) and the plan of that date;
`../tasks.md` is the checklist; `source/` is the library's tree at `2fad162`.

## Where it stands

The library was developed on 2026-10-09 in a worktree of the superproject, and its repository's
history is 8 commits, all as `R`, from 2026-10-09 to 2026-10-12, from `f603665` ("config:
webcpp's configuration, its version and the target, with their tests") to `2fad162` ("meta: the
maintainer's public address"):

| Commit | Date | What |
| --- | --- | --- |
| `f603665` | 2026-10-09 | The headers, the five tests, the example, the repository's files (plan: Task 1) |
| `9c86fd0` | 2026-10-09 | A predefined macro stops at one message; the target block's order pinned |
| `0465bf8` | 2026-10-09 | The page, with the reference of the macros (plan: Task 2) |
| `630aa63` | 2026-10-09 | Predefined macros keep their underscores in the reference; the page's counts say what they count |
| `2d02f91` | 2026-10-09 | The changelog, the page's last appendix (plan: Task 3, config's part) |
| `ac96479` | 2026-10-09 | The changelog's first line |
| `413a882` | 2026-10-09 | The page names `test` among the words no library's name may be |
| `2fad162` | 2026-10-12 | The maintainer's public address in `meta/libraries.json` |

| What | Where |
| --- | --- |
| The library | `github.com/webcpporg/config`, branch `main` at `2fad162` |
| The superproject | `github.com/webcpporg/webcpp`, with `libs/config` as a submodule (`../config.git`) added by its commit `f11986d` of 2026-10-11, "libs/config: the submodule", and at `2fad162` in the checkout read for this proposal (superproject commit `c53f3f9`, 2026-10-13) |
| The page | `webcpporg.github.io/webcpp/libs/config/`, built by the superproject's CI (not opened for this proposal) |

`source/` is exactly `git archive 2fad162` of the library: 27 files, the root files, `include/`,
`test/`, `example/`, `doc/`, `meta/` and `.github/`.

The version the headers name is 0.1.0 (`WEBCPP_VERSION 100`, `WEBCPP_LIB_VERSION "0_1"`). The plan
states that no release is made until the owner asks; no tag was listed in the superproject
checkout read (`git tag` printed nothing), so this proposal records the release as 0.1.0, the
version in preparation, and does not claim a tag.

### What was measured

Only what was seen for this proposal, on 2026-10-13, on macOS with Apple clang 21.0.0 (also used as
`g++`) and Homebrew's Boost 1.92, compiling each test file of `source/test/` directly with
`-std=c++20 -I source/include` (not through b2, not on any wasm target):

- `version_test.cpp`, `target_test.cpp` and `exceptions_test.cpp` compile (`-fsyntax-only`);
  `exceptions_test.cpp` also compiles with `-fno-exceptions -DWEBCPP_TEST_CONFIG_OFF` and with
  `-DWEBCPP_NO_EXCEPTIONS -DWEBCPP_TEST_CONFIG_DEVELOPER_OFF`; `target_test.cpp` also compiles
  through the `g++` driver, which on this machine is the same Apple clang.
- `predefined_test.cpp` with each of `-DWEBCPP_TARGET_NATIVE`, `-DWEBCPP_TARGET_WASIP2`,
  `-DWEBCPP_TARGET_WASIP3`, `-DWEBCPP_TARGET_EMSCRIPTEN` and `-DWEBCPP_HAS_EXCEPTIONS`: exactly one
  error, "WEBCPP_TARGET_* and WEBCPP_HAS_EXCEPTIONS are defined by <webcpp/config.hpp> alone"
  (the text was read for `-DWEBCPP_TARGET_WASIP2`; the five runs printed the same number of
  error lines), and `-Xclang -verify` passes for that case.
- `unsupported_test.cpp` with `-D__wasm__`: `-verify` passes; with `--target=wasm32-unknown-unknown`
  on the same compiler: the error "unsupported target: ..." from `config.hpp`.
- `example/config_quick_start.cpp` built and run natively: it printed `my_library: exceptions on`,
  the content of `config_quick_start.expected`.

No b2 run, no wasip2, wasip3 or Emscripten build, no lint and no documentation build were seen.
The plan's statements that those lanes pass (wasi-sdk 34 measurements, Emscripten 6.0.11) are the
plan's, not verified here.

## Open items

1. **MSVC.** Closed on 2026-10-13: the superproject's MSVC lanes failed before building any
   library (a `path.exists` given a path in b2's own form, in `tools/generated/generated.jam`),
   which `1e33cbc` and `c53f3f9` fixed; CI run 38365413330 at `c53f3f9` saw MSVC 14.3 and 14.5
   green, config's tests among them.
2. **The release.** No `webcpp-0.1.0` tag was seen; the version stays in preparation until the
   owner asks for the release (plan: Task 15, "no release is made"). `version.hpp` moves only by
   the release procedure.
3. **The superproject's part of the approved specification** is not in this library: the
   changelog rule, the release notes, `check-release`, `whats-new`, `dist` and every library's
   move onto `<webcpp/config.hpp>` (plan: Tasks 3 to 14). `wasi` kept its own `config.hpp`
   (it does not include Boost, so it honours `WEBCPP_NO_EXCEPTIONS` directly) until its redesign
   (plan, departure 4). Their proposals belong to the organization root and to those libraries.
4. **A new target** (any other WebAssembly target) needs a macro, a branch of the target block and
   a test of its own; today it is an `#error`.

## How to resume

1. Work in a worktree of the superproject (`git worktree add ../webcpp-config-<slug> -b
   lib/config-<slug> main`, then `git submodule update --init --recursive`), following the
   superproject's `AGENTS.md`, chapter "Working on a library". `libs/config` is the published
   repository; `source/` is the same tree, for reading without a checkout.
2. Follow the library's own rules (`source/AGENTS.md`): macros only, `version.hpp` outside
   `webcpp/config/`, the version written once and moved only by the release procedure, the
   target from the compiler with its block before Boost.Config. Write the failing test first;
   every guard clause of `config.hpp` has a test.
3. Add each change's line to "Unreleased" in `doc/changelog.adoc` in the same commit, and keep the
   reference comments in the headers in step with the page.
4. Before saying a task is done: `b2 -a libs/config/test libs/config/example` for native,
   `toolset=clang-wasip2 testing.launcher=wasmtime`, `toolset=clang-wasip3
   testing.launcher=wasmtime` and `toolset=emscripten`, the superproject's lint, `b2 -a
   libs/config/doc`, and then CI on every lane.
