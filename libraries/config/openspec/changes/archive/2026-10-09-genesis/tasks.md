# Tasks

Every task of the library, each pointing to its task in `reference/plan.md` or to its commits in
the library's repository; a checked task is done in `reference/source/` (the tree at `2fad162`).
The plan's other tasks (3 to 14) are the superproject's: the changelog rule, the release notes,
the version read by the Jamroot, `check-release`, `whats-new`, `dist`, `AGENTS.md` and the other
libraries' move onto `<webcpp/config.hpp>`; they are outside this capability, and the
superproject's own proposals carry them.

## 1. The library: headers, tests, repository (plan: Task 1)

- [x] 1.1 The repository and its files: `build.jam` (`project /webcpp/config`, the alias `config`), `meta/libraries.json`, `README.md`, `AGENTS.md`, `LICENSE_1_0.txt`, `.gitignore`, `.gitattributes`, `.github/workflows/ci.yml` (plan: Task 1, Steps 1 and 7; commit `f603665`)
- [x] 1.2 The tests written first, each failing for its own reason: `version`, `target`, `exceptions` (three builds), `predefined_*` (five) and `unsupported_*` (three), with the headers-alone test (plan: Task 1, Steps 1, 2 and 4; commit `f603665`)
- [x] 1.3 The order of `--target` and the macros each target defines measured with wasi-sdk 34 and Emscripten 6.0.11 (plan: Task 1, Step 3)
- [x] 1.4 `include/webcpp/version.hpp`: `WEBCPP_VERSION 100` and `WEBCPP_LIB_VERSION "0_1"` (plan: Task 1, Step 5; commit `f603665`)
- [x] 1.5 `include/webcpp/config.hpp`: the target block before `<boost/config.hpp>`, the `#error` for an unsupported WebAssembly build, `WEBCPP_HAS_EXCEPTIONS` (plan: Task 1, Step 5; commit `f603665`)
- [x] 1.6 The example `config_quick_start` and its `.expected` file (plan: Task 1, Step 6; commit `f603665`)
- [x] 1.7 A predefined macro stops the build at one message: the `#else` of the guard, a test for every guard clause, the order of the target block pinned by a wasm32-unknown-unknown build, and `exceptions_test.cpp` refusing a `WEBCPP_NO_EXCEPTIONS` that config defines itself (plan: Task 1, Step 9, planted defects; commit `9c86fd0`)
- [x] 1.8 The four target lanes (native, wasip2, wasip3, Emscripten) and the lint, as the plan runs them (plan: Task 1, Steps 8 and 10; what was seen to run is in `reference/STATUS.md`)

## 2. The page and its reference (plan: Task 2)

- [x] 2.1 The page: `doc/Jamfile`, `doc/config.adoc`, `introduction.adoc`, `targets.adoc`, `exceptions.adoc`, `version.adoc`, `testing.adoc`, and the README's link to it (plan: Task 2, Steps 5 and 6; commit `0465bf8`)
- [x] 2.2 The reference of the macros: the `/** */` comments in the headers, the `__MRDOCS__` block, `\__wasip2__` spelled so Asciidoctor keeps its underscores (plan: Task 2, Steps 3 and 4; commits `f603665`, `630aa63`)
- [x] 2.3 The page's counts say what they count: tests declared per target, with the clang-only diagnostic tests (commit `630aa63`)
- [x] 2.4 The words no library's name may be, named on the page, `test` included (commit `413a882`; the check itself is the superproject's, `tools/doc/libraries.py`)

## 3. The changelog (plan: Task 3, config's part)

- [x] 3.1 `doc/changelog.adoc`, the page's last appendix, with its "Unreleased" section (plan: Task 3; commit `2d02f91`)
- [x] 3.2 Its first line, "Added the library, with webcpp's target, exceptions and version macros." (commit `ac96479`)

## 4. Publication (plan: Task 15, config's part)

- [x] 4.1 The repository `webcpporg/config` created and pushed, and the submodule `libs/config` added to the superproject (superproject commit `f11986d`, "libs/config: the submodule"; plan: Task 15, Steps 1 and 2)
- [x] 4.2 The maintainer's public address in `meta/libraries.json` (commit `2fad162`)
- [x] 4.3 The library's tests seen green on MSVC, which the target block and `webcpp.compile` lanes cover: seen green on 2026-10-13: MSVC 14.3 and 14.5 in the superproject CI run 38365413330 at `c53f3f9`.
