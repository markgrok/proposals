---
title: "webcpp::config: configuration, version and changelogs - implementation plan"
tags: [webcpp, config, release, changelog, plan]
status: draft
created: 2026-10-09
---

# webcpp::config Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add `webcpporg/config`, webcpp's counterpart of Boost.Config:
`<webcpp/config.hpp>` (the target, exceptions, `WEBCPP_NO_EXCEPTIONS`) and
`<webcpp/version.hpp>` (webcpp 0.1.0), which every library's `config.hpp`
includes first; give every library a `doc/changelog.adoc` and the superproject
the release notes of each version, with the changelog rule in AGENTS.md and a
lint rule that keeps every line in form; and build and test the release
tooling (`check-release`, `whats-new`, `dist`) without ever running a release.

**Architecture:**
- **config is a library like the others,** `libs/config`, a local repository
  until the owner creates `webcpporg/config` (Task 15). It is macros only, as
  Boost.Config is: its macros are the collection's own, `WEBCPP_*`, so the
  reference tooling learns that one library's prefix is `WEBCPP_` and that it
  declares no namespace.
- **Changelogs live in the libraries, once.** Each library's
  `doc/changelog.adoc` is the last appendix of its page, with an "Unreleased"
  section on top; the superproject's `doc/releases/<version>.adoc` holds what
  belongs to the collection and includes, by tagged region, each updated
  library's section. The index page shows every version's notes, newest
  first. `tools/release/changelog.py` is the one reader of both forms, used by
  the lint on every run and by the release check.
- **The release tooling is one Python program,** `tools/release/release.py`
  (`check`, `whats-new`, `dist`), run by three explicit targets of the
  Jamroot, which reads the version from `version.hpp`. It is tested in scratch
  git repositories and never run on the real tree.
- **Built in parallel, integrated after stage 4.** Tasks 1 to 10 are built in
  their own worktree while stage 4 continues on `~/dev/webcpp`. The one
  change of the doc-check stage 4's Task 11 also changes, a library's tracked
  file in the release notes, waits for Task 11 here, written against stage
  4's finished code. The integration into `M`'s
  `main` (Tasks 11 and 12) runs only after stage 4's Task 14 has published;
  from then on nobody pushes `M` or any library until Task 15. The libraries'
  move onto `<webcpp/config.hpp>` (Tasks 13 and 14) is held on branches until
  `webcpporg/config` exists (Task 15), because a library that includes it
  cannot be published before config is.

**Tech Stack:**
- C++20, b2 5.5.3, Boost 1.92 (Homebrew), Boost.Config;
- wasi-sdk 34 (clang 23), wasmtime 47.0.3, Emscripten 6.0.11 and Node 26.7.0;
- Python 3.9+ (`tarfile`, `gzip`, `hashlib`, `subprocess` with git);
- MrDocs 2026.9.29, Asciidoctor.js; GitHub Actions and Pages (unchanged).

**Spec:** `~/.buzz/PLANS/WEBCPP_CONFIG_SPEC.md` (approved by the
owner on 2026-10-09, with the amendment on unknown wasm targets), binding.
Also binding: the owner's decisions in
`~/.buzz/PLANS/WEBCPP_RELEASE_DESIGN.md` and the last entries of
`~/.buzz/PLANS/WEBCPP_DESIGN.md` ("overnight execution", "the config plan's
open points" (a) to (f)). The format follows
`~/.buzz/RESEARCH/BOOST_RELEASE_NOTES.md`, sections 1 and 5. The pending
entries to move are `~/.buzz/PLANS/WEBCPP_CHANGELOG_PENDING.md`. The stage-4
ledger, `~/dev/webcpp/.superpowers/sdd/WEBCPP_STAGE4_PLAN_V2/progress.md`,
says what is changing in parallel. The independent review,
`~/dev/webcpp/.superpowers/sdd/plan-reviews/WEBCPP_CONFIG_PLAN-review.md`, is
answered in "Review disposition" at the end.

**Departures from the spec's letter** (points 1 to 5 accepted by the owner on
2026-10-09, (a) to (e); the rest new, for the owner):
1. `b2 release-check` cannot exist: b2 reads a target named `release-<word>`
   as the variant `release` with a subfeature and stops before it builds
   anything (measured, B2 5.5.3: `error: "release-check" is not a known value
   of feature <variant>`). The target is `b2 check-release`, and AGENTS.md
   chapter 12 records the fact.
2. The libraries' move onto `<webcpp/config.hpp>` waits on a branch until
   `webcpporg/config` exists.
3. The version is named with three numbers, `0.1.0`, as Boost's tags are
   (`boost-1.92.0`): `WEBCPP_VERSION 100`, `WEBCPP_LIB_VERSION "0_1"`, tag
   `webcpp-0.1.0`, archive `webcpp-0.1.0.tar.gz`, checksum
   `webcpp-0.1.0.tar.gz.txt`; the release notes are rendered in the index
   page, in a section "Releases", newest first.
4. wasi keeps not including Boost (its by-hand build names only its own
   `include/`), so its `config.hpp` honours `WEBCPP_NO_EXCEPTIONS` directly;
   it includes `<webcpp/config.hpp>` in its redesign, whose plan's first step
   replaces the two-line condition with the include.
5. A library has changed, for the release check, when its repository changed
   under `include/`, `build.jam`, `wit/`, `meta/libraries.json`, `README.md`,
   `example/`, `doc/` (its changelog apart) or `LICENSE*` (the controller's
   addition to the owner's (e), 2026-10-09); `include/webcpp/version.hpp`
   apart, which only config has and which moves at every release.
6. A changelog line may also start with "Improved" or "Updated", beside the
   spec's Added, Changed, Fixed, Removed and Deprecated: Boost's notes use
   both, and the pending entries already use "Updated".
7. Section 7 says "After stage 4"; this plan builds Tasks 1 to 10 during stage
   4, in a worktree of their own, and integrates only after stage 4's
   publication.
8. A wasm build that is no supported target stops with `#error "unsupported
   target ..."` (the owner's (f)), tested on wasm32-wasip1 and on `__wasm__`
   alone; the target block comes before `<boost/config.hpp>`, so that
   wasm32-unknown-unknown, which has no C++ library, gets the same message
   before Boost's fatal error.

9. The changelog rule (Task 5) is stricter than the spec's letter in two
   measured ways, by the controller's ruling of 2026-10-09: a verb of the
   list follows `*Breaking change:*`, and a line over 30 words fails
   ("about ten words" stays the reviewer's judgement below that ceiling).

## Global Constraints

**Repositories, worktrees and paths:**
- `M` is `~/dev/webcpp`, the superproject, on `main`, where stage 4 goes on.
  `W` is `~/dev/webcpp-config`, this track's worktree of it, on branch
  `config`, made in Task 0 from `main`. Nothing of this plan is written in `M`
  before Task 12, but the ledger.
- In `W`, each library of `M/libs` is a git worktree of that library's own
  repository, on a branch `config` (Task 0); `W/libs/config` is config's own
  repository, `git init -b main` (Task 1). `W/.local` is a symbolic link to
  `M/.local`, read only, and excluded in `W`'s `info/exclude`.
- Never inside `M`: a worktree under `M` is untracked there, and stage 4's
  lint and builds would read it.
- `libs/trystero` enters `W` at Task 11, once stage 4 has published it: its
  changelog moves in with the others at Task 11, and its `config.hpp` move is
  Task 14, held like Task 13.
- From Task 12 until Task 15, `M`'s `main` and the libraries' `main`s carry
  config's unpublished commits: **nobody pushes `M` or any library between
  Task 12 and Task 15.** Both ledgers say so (Task 12).
- The repository `webcpporg/config` is created only with the owner's go, in
  Task 15, by the controller.
- The library-side changes are commits in the libraries' own repositories;
  the superproject records a library's commit only by `git add
  libs/<name>`, never `git add libs`, and only in Task 15.

**Commits and text** (every plan of this project states these):
- Commits are made with `git -c user.name=R -c
  user.email=github.overboard312@passmail.net commit`. No trailer of any kind, never an
  agent co-author. A body paragraph never starts with `word:` (`std::`).
- Files are staged by path: `git add <path> ...`. Never `git add -A`, never
  `git add libs`. (The tests' scratch repositories are no commit of the
  project's: their helpers add all.)
- Never push and never touch GitHub: the controller publishes, in Task 15.
- English only. Never the em dash (U+2014): a plain dash. Nothing anywhere
  matches the case-insensitive pattern `veru[s]`: no file, file name, path in
  a command, or commit. No "stage N" or "Task N" in a tracked file or a commit
  message, test comments included.
- Every new source file (`.hpp .cpp .py .sh .jam .yml .adoc`, `Jamfile`,
  `build.jam`) opens with the licence notice, BSL-1.0, "Copyright (c) 2026
  WebCpp.org", `//` for C++ and AsciiDoc, `#` for Jam, Python, shell and YAML,
  a `#!` line first (AGENTS.md chapter 11). Below, "(the notice)" stands for
  it.
- Subject lines name the part touched: `config: ...`, `tools/doc: ...`,
  `tools/release: ...`, `xactor: ...`, `AGENTS.md: ...`.

**Running b2:**
- `B2` is `env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH b2
  --user-config=.local/user-config.jam`, run from `W` (from `M` in Tasks 12
  and 15), in a shell that has run `source .local/env.sh`. A verdict is built
  from scratch (`B2 -a`).
- Parallel lanes each get their own `--build-dir`: `bin/lane-clang`,
  `bin/lane-gcc-15`, `bin/lane-wasip2`, `bin/lane-wasip3`,
  `bin/lane-emscripten`. Start them in the background, wait for all, read
  each log; at most one doc build at a time per tree. Never
  `testing.launcher` on emscripten.
- A lane is green when `python3 tools/report/report.py --lane
  <lane>=<lane>.xml ... --out <dir>` exits 0, the lanes run as
  `B2 -a --dump-tests --out-xml=<lane>.xml ...` (a native lane is named
  `native` or after its toolset directory, `gcc-15`).
- An action of more than one command opens with `set -e`; `$(` inside an
  action is b2's (backquotes); a `<build>no` is conditioned on `<toolset>`
  alone; no target name starts with `debug-`, `release-` or `profile-`.
- The tools' tests: `python3 tools/<dir>/<name>_test.py`, every one of them
  before a commit that touches `tools/`, `pyright` with `pyrightconfig.json`
  0 errors, and the lint `lint: clean`.

**Changelog** (from now on, every task):
- Every user-visible change adds its line in the same commit as the change:
  a library's in its `doc/changelog.adoc` under "Unreleased", a change of the
  superproject's tools or build in `doc/releases/0.1.0.adoc` under "Tools and
  build". Tasks 1 to 3 run before the notes exist: their reports'
  "Changelog" section carries their superproject lines, which Task 4 places.
- A line: `* ` then Added, Changed, Deprecated, Fixed, Improved, Removed or
  Updated (or `*Breaking change:*` first), one sentence of about ten words,
  what a user sees; no cross-reference (`<<...>>`); at most ten per library
  and version. A library's internal changes (its tests, its CI, a
  refactoring) get none. The superproject's "Tools and build" lists what
  changes for whoever builds, lints or tests with it: its b2 rules and
  targets, the lint's rules, the CI's lanes; the reference tooling's
  internals and AGENTS.md get none.
- By the owner's decision (release design, item 4) agents write webcpp's
  changelogs: for webcpp this replaces the global rule of never editing a
  changelog.

**Names:**
- config's headers are `<webcpp/config.hpp>` and `<webcpp/version.hpp>`, as
  Boost's `<boost/config.hpp>` and `<boost/version.hpp>`; its macros are
  `WEBCPP_*`, never `WEBCPP_CONFIG_*`; its include guards
  `WEBCPP_CONFIG_HPP` and `WEBCPP_VERSION_HPP`; it declares no namespace.
- Since config's macros take `WEBCPP_TARGET_*`, `WEBCPP_HAS_*`,
  `WEBCPP_NO_*`, `WEBCPP_LIB_*` and `WEBCPP_VERSION*`, no library may be
  named `target`, `has`, `no`, `lib` or `version` (Task 2 refuses them).
- A test-only macro is `WEBCPP_TEST_*`.
- idna, formats and json_schema, whose specs already say "a one-line
  migration", are not in the tree: each is born on `<webcpp/config.hpp>`
  when it comes, and nothing of them is in this plan.

**Outward actions:** subagents never push, merge, create repositories, start
or cancel CI runs, or change GitHub settings, and never touch SSH keys or
credentials. Tasks 0, 12 and 15 are the controller's; Task 15 needs the
owner's go.

## Review Focus

1. **A checkout without `libs/config`.** The scratch superprojects of the
   tools' tests, and a clone whose submodule is not checked out, have no
   `version.hpp`: `b2 test`, `b2 declared-targets` and `b2 doc` must work as
   before, and only `check-release`, `whats-new` and `dist` stop, naming the
   header. The index then links a config page the checkout lacks, which
   `tools/ci/assemble.py`'s link check refuses in CI: that is why nothing is
   pushed before config is a submodule (Task 15). Pinned in Task 6
   (`test_jamroot_without_config`).
2. **`WEBCPP_NO_EXCEPTIONS` that misses a library or reaches Boost.** Defined
   by a developer in a build with exceptions, it must undefine
   `WEBCPP_HAS_EXCEPTIONS` and define every library's
   `WEBCPP_<LIBRARY>_NO_EXCEPTIONS`, while `BOOST_NO_EXCEPTIONS` stays as the
   compiler says. Pinned in Task 1 (`exceptions_developer_off`), Task 13
   (`config_all_off` in each library) and Task 14 (trystero's).
3. **A target macro that lies.** Two `WEBCPP_TARGET_*` defined, none, one
   that disagrees with the toolset, one a developer predefined, or a wasm
   build that is no supported target read as native. Pinned in Task 1
   (`target`, `predefined_target`, `unsupported_wasip1`,
   `unsupported_wasm`).
4. **A changelog line in the wrong form, written between releases.** An agent
   writes a line with no verb of the list, a cross-reference, an eleventh
   line, or text outside a bullet, and nobody sees it until release day.
   Wrapped lines must read as one bullet. Pinned in Task 5 (the lint rule
   `changelog`, `changelog_test.py`).
5. **`check-release` passing a release that misses a library.** A library
   changed since the last tag with no line; one with lines but not listed;
   one listed whose region is empty; a submodule added or removed and not
   under New or Removed Libraries; the first release; a tag commit the
   library no longer has; a pointer that is not the library's checked-out
   commit. Each fails by name. Pinned in Task 7.
6. **An archive that is not reproducible or carries what it should not.** Two
   runs, or two checkouts at different paths, giving different bytes; a dot
   file, `bin/` or an untracked file slipping in; a dirty tree archived; a
   built page missing. Pinned in Task 9.

---

## File structure

```
~/dev/webcpp-config/  (W, branch config of the superproject)
  Jamroot                          define-line; webcpp-version; check-release, whats-new, dist
  doc/index.adoc                   + section Releases: each version's notes, newest first
  doc/releases/0.1.0.adoc          the release notes of 0.1.0, in preparation
  tools/release/changelog.py       the form of a changelog and of release notes
  tools/release/changelog_test.py
  tools/release/release.py         check, whats-new, dist
  tools/release/release_test.py
  tools/release/scratch.py         scratch git superprojects for release_test.py
  tools/release/template.adoc      the notes of a version just opened
  tools/lint/rules.py, lint.sh, lint_test.py   + the rule changelog
  tools/doc/reference.py           config's macros are WEBCPP_*; config declares no namespace
  tools/doc/doc_comments.py        check(..., namespace_required)
  tools/doc/libraries.py           the names config's macros reserve
  tools/doc/doc-check.py           {webcpp-root}/libs/<name>/<path>[tag=...] of a library's git
  tools/doc/doc.jam                the index gets {webcpp-root} and --webcpp-root
  tools/doc/*_test.py, tools/test/doc_test.py
  pyrightconfig.json               + tools/release
  AGENTS.md, README.md
  libs/config/                     config's repository (webcpporg/config from Task 15)
    build.jam README.md AGENTS.md LICENSE_1_0.txt .gitignore .gitattributes
    meta/libraries.json
    include/webcpp/config.hpp include/webcpp/version.hpp
    test/Jamfile test/version_test.cpp test/target_test.cpp test/exceptions_test.cpp
    test/predefined_test.cpp test/unsupported_test.cpp
    example/Jamfile example/config_quick_start.cpp example/config_quick_start.expected
    doc/Jamfile doc/config.adoc doc/introduction.adoc doc/targets.adoc doc/exceptions.adoc
    doc/version.adoc doc/testing.adoc doc/changelog.adoc
    .github/workflows/ci.yml
  libs/xactor, libs/xstate, libs/pratt, libs/wasi, libs/trystero
    doc/changelog.adoc, the page's include          (Tasks 3 and 11, branch config)
    include/webcpp/<name>/config.hpp, build.jam, test/Jamfile, test/config_test.cpp,
    the texts that name the macro or a by-hand -I    (Tasks 13 and 14, branch config-adoption)
```

## Conflicts with parallel tracks

Stage 4 edits the same superproject and the same libraries. The files this
plan touches that stage 4 also touches, and how each is handled:

| Shared file | This plan | Stage 4 | Handled by |
| --- | --- | --- | --- |
| `tools/doc/doc-check.py`, `tools/doc/doc.jam`, `tools/doc/doc_check_test.py`, `tools/test/doc_test.py` | the index's `{webcpp-root}`, a library's tracked file (Task 11) | Task 11, certainly: `{library-root}`, "a region of a file the library's own git tracks" | written in this plan's Task 11, after stage 4 has published, against its final code; one helper, `Library.repository_of` and `Library.tracked_in`, serves both rules; nothing of Tasks 1 to 10 touches these files but `doc_test.py`'s appended cases (Task 2) |
| `AGENTS.md` | chapters 1, 2, 3, 6, 7, 8, 10, 11, 12, 13 (Task 10); 2, 3, 6, 13 (Task 13); 1 (Task 15) | Tasks 4, 11, 12, 13 | additions, never a renumbering; rebased once, at Task 11, onto stage 4's finished text |
| `README.md` | one paragraph (Task 10) | Task 12 | the same |
| `Jamroot` | `define-line`, `webcpp-version`, three targets appended before the last `explicit` (Tasks 6, 8, 9) | Task 4's fix round (emscripten headers), done | appended; one call site changed |
| `tools/doc/reference.py`, `doc_comments.py` and their tests | one prefix, one parameter (Task 2) | Task 4 (`-idirafter`), done | two small hunks |
| `tools/lint/rules.py`, `lint.sh`, `lint_test.py` | the rule `changelog` (Task 5) | possibly Task 9 (trystero lint-clean) | a rule appended; rebased at Task 11 |
| `doc/index.adoc` | one section appended (Task 4) | Task 12 (the index's text) | appended |
| `pyrightconfig.json` | one line (Task 5) | Tasks 7 or 12 if a tools directory is added | one line |
| `libs/xactor`, `libs/xstate`, `libs/pratt`, `libs/wasi` | `doc/changelog.adoc` and one include (Task 3); `config.hpp`, `build.jam`, tests, a few sentences (Task 13) | Task 13 (pratt's Jamfiles, README, `testing.adoc`, `introduction.adoc`), and any fix | branches rebased onto each library's `main` at Task 11 |
| `libs/trystero` | changelog (Task 11), `config.hpp` (Task 14) | Tasks 5 to 14 create it | entered only after stage 4's Task 14 |
| `~/.buzz/PLANS/WEBCPP_CHANGELOG_PENDING.md` | read (Task 3), emptied (Task 11), retired (Task 12) | appended to until stage 4 ends, trystero's lines included | Task 11 moves everything not yet moved |
| `.local/` (through `W/.local`) | read only | Task 0 follow-ups | never written by this track |

New files only, which conflict with nothing: `libs/config/**`,
`tools/release/**`, `doc/releases/**`. No CI file changes: the tools job runs
every `tools/**/*_test.py` by itself, and config's own CI file lives in its
repository.

---

### Task 0: The track's worktree (controller)

**Files:**
- Create: `~/dev/webcpp-config` (a worktree of `M`, branch `config`), the
  link `W/.local`, a worktree of each library at `W/libs/<name>` on branch
  `config`, and the ledger
  `M/.superpowers/sdd/WEBCPP_CONFIG_PLAN/progress.md`

- [ ] **Step 1: Choose the base.** Read the stage-4 ledger's last lines: no
  stage-4 task may be committing to the superproject at this moment. Record
  in this plan's ledger (knowledge-file frontmatter, title `"webcpp config
  track: ledger"`, status active) `git -C ~/dev/webcpp rev-parse --short
  HEAD` and each library's `git -C ~/dev/webcpp/libs/<name> rev-parse --short
  main`.
- [ ] **Step 2: The worktrees,** for the four published libraries only
  (trystero, still being built by stage 4, enters at Task 11):

```bash
git -C ~/dev/webcpp worktree add ~/dev/webcpp-config -b config main
ln -s ~/dev/webcpp/.local ~/dev/webcpp-config/.local
printf '/.local\n' >> "$(git -C ~/dev/webcpp-config rev-parse --git-path info/exclude)"
for name in xactor xstate pratt wasi; do
    rmdir ~/dev/webcpp-config/libs/$name
    git -C ~/dev/webcpp/libs/$name worktree add ~/dev/webcpp-config/libs/$name -b config main
done
git -C ~/dev/webcpp-config status --short
```

  Expected: `status` prints nothing (`.gitignore`'s `.local/` does not match
  a symbolic link; the exclude does).
- [ ] **Step 3: See the base green in `W`,** from scratch, in parallel: each
  library's native lane, `B2 -a doc`, `tools/lint/lint.sh --clang-format
  .local/wasi-sdk/bin/clang-format --clang-tidy .local/wasi-sdk/bin/clang-tidy`
  (ends `lint: clean`) and every `tools/**/*_test.py`. Expected: all green, as
  in `M`. A red one is recorded with its cause before any task starts: this
  track inherits it and does not fix stage 4's work.

### Task 1: The config library: headers, tests, repository

**Files** (all in `W/libs/config`, a new repository):
- Create: `build.jam`, `include/webcpp/config.hpp`,
  `include/webcpp/version.hpp`, `test/Jamfile`, `test/version_test.cpp`,
  `test/target_test.cpp`, `test/exceptions_test.cpp`,
  `test/predefined_test.cpp`, `test/unsupported_test.cpp`, `example/Jamfile`,
  `example/config_quick_start.cpp`, `example/config_quick_start.expected`,
  `meta/libraries.json`, `README.md`, `AGENTS.md`, `LICENSE_1_0.txt` (a copy
  of xactor's), `.gitignore` (`doc/html/`), `.gitattributes` (xactor's),
  `.github/workflows/ci.yml`

**Interfaces:**
- Produces the target `/webcpp/config//config` (its headers and Boost's), and
  these macros, which every other task uses by name:

| Macro | Defined when |
| --- | --- |
| `WEBCPP_VERSION` | always: `major * 100000 + minor * 100 + patch`, `100` |
| `WEBCPP_LIB_VERSION` | always: `"<major>_<minor>"`, `"_<patch>"` after it when patch is not 0: `"0_1"` |
| `WEBCPP_TARGET_WASIP2` | `__wasip2__` |
| `WEBCPP_TARGET_WASIP3` | `__wasip3__` |
| `WEBCPP_TARGET_EMSCRIPTEN` | `__EMSCRIPTEN__` |
| `WEBCPP_TARGET_NATIVE` | none of the three, and not `__wasm__`; any other wasm build (wasip1, wasm32-unknown) is an `#error` "unsupported target" (owner, 2026-10-09) |
| `WEBCPP_HAS_EXCEPTIONS` | neither `BOOST_NO_EXCEPTIONS` nor `WEBCPP_NO_EXCEPTIONS` |
| `WEBCPP_NO_EXCEPTIONS` | by the developer only |

  Measured with wasi-sdk 34: `--target=wasm32-wasip2` defines `__wasi__`,
  `__wasm__` and `__wasip2__`, `wasm32-wasip3` likewise with `__wasip3__`,
  `wasm32-wasip1` `__wasi__`, `__wasm__` and `__wasip1__` and compiles
  `<boost/config.hpp>`; em++ 6.0.11 defines `__EMSCRIPTEN__`; native clang
  with `-D__wasm__` compiles `<boost/config.hpp>`; `wasm32-unknown-unknown`
  stops inside `<boost/config.hpp>` (no `<cstddef>`).

- [ ] **Step 1: The repository and the tests.**
  `git init -b main ~/dev/webcpp-config/libs/config`, then `build.jam`:

```
# (the notice)
#
# config, webcpp's configuration, which every webcpp library includes first,
# as Boost.Config is Boost's. A library uses it through /webcpp/config//config,
# which adds its headers and Boost's.

project /webcpp/config ;

alias config : : : : <include>include <library>/boost//headers ;
```

  `test/Jamfile` (after the notice):

```
project : requirements <library>/webcpp/config//config ;

import webcpp ;

webcpp.targets native wasip2 wasip3 emscripten ;

# config.hpp alone. version.hpp, which is <webcpp/version.hpp> as Boost's is
# <boost/version.hpp>, lies outside webcpp/config/, so headers-alone does not
# take it: the test version compiles it alone.
webcpp.headers-alone config : ../include ;

# version.hpp compiles alone, first, and its two macros name one version.
webcpp.compile version : version_test.cpp ;

# Exactly one target macro is defined, the one of the toolset the build is
# for: the toolset tells the test, the compiler tells config.hpp.
webcpp.compile target : target_test.cpp
  : <toolset>clang-wasip2:<define>WEBCPP_TEST_TARGET_WASIP2
    <toolset>clang-wasip3:<define>WEBCPP_TEST_TARGET_WASIP3
    <toolset>emscripten:<define>WEBCPP_TEST_TARGET_EMSCRIPTEN ;

# WEBCPP_HAS_EXCEPTIONS is defined exactly when webcpp has exceptions: as the
# build asks, in a build without them, and never once the developer defines
# WEBCPP_NO_EXCEPTIONS, which leaves Boost's own exceptions as they are.
webcpp.compile exceptions : exceptions_test.cpp ;
webcpp.compile exceptions_off : exceptions_test.cpp
  : <exception-handling>off <define>WEBCPP_TEST_CONFIG_OFF ;
webcpp.compile exceptions_developer_off : exceptions_test.cpp
  : <define>WEBCPP_NO_EXCEPTIONS <define>WEBCPP_TEST_CONFIG_DEVELOPER_OFF ;

# The target and WEBCPP_HAS_EXCEPTIONS are config.hpp's to define: one
# defined before it stops the build, with config.hpp's message.
webcpp.compile-diagnostic predefined_target : predefined_test.cpp
  : <define>WEBCPP_TARGET_NATIVE ;
webcpp.compile-diagnostic predefined_exceptions : predefined_test.cpp
  : <define>WEBCPP_HAS_EXCEPTIONS ;

# A WebAssembly target webcpp does not support stops the build, with
# config.hpp's message: wasm32-wasip1 on wasi-sdk's clang, and __wasm__
# alone natively, which every native clang lane runs.
webcpp.compile-diagnostic unsupported_wasip1 : unsupported_test.cpp
  : <cxxflags>--target=wasm32-wasip1 : wasip2 ;
webcpp.compile-diagnostic unsupported_wasm : unsupported_test.cpp
  : <define>__wasm__ : native ;
```

  `test/version_test.cpp` (after the notice):

```cpp
// <webcpp/version.hpp> compiles alone, first, and its two macros name one version:
// WEBCPP_LIB_VERSION is "<major>_<minor>", and "_<patch>" after it only when the patch level is
// not 0, as BOOST_LIB_VERSION is to BOOST_VERSION.

#include <webcpp/version.hpp>

#include <array>
#include <string_view>

namespace {

// The numbers of a version as WEBCPP_LIB_VERSION writes it, how many it holds, and whether the
// text was digits separated by single underscores.
struct written {
    std::array<int, 3> numbers{};
    int count = 0;
    bool valid = true;
};

constexpr written read(std::string_view text)
{
    written result;
    bool digits = false;
    for (const char c : text) {
        if (c == '_' && digits && result.count < 2) {
            ++result.count;
            digits = false;
        } else if (c >= '0' && c <= '9') {
            result.numbers.at(result.count) = result.numbers.at(result.count) * 10 + (c - '0');
            digits = true;
        } else {
            result.valid = false;
        }
    }
    result.count += digits ? 1 : 0;
    result.valid = result.valid && digits;
    return result;
}

// Not major and minor: glibc's <sys/sysmacros.h> defines both as macros.
constexpr int version_major = WEBCPP_VERSION / 100000;
constexpr int version_minor = WEBCPP_VERSION / 100 % 1000;
constexpr int version_patch = WEBCPP_VERSION % 100;
constexpr written lib = read(WEBCPP_LIB_VERSION);

static_assert(lib.valid, "WEBCPP_LIB_VERSION is not \"<major>_<minor>[_<patch>]\"");
static_assert(lib.numbers[0] == version_major && lib.numbers[1] == version_minor,
              "WEBCPP_LIB_VERSION names another version than WEBCPP_VERSION");
static_assert(lib.count == (version_patch == 0 ? 2 : 3),
              "WEBCPP_LIB_VERSION writes the patch level exactly when it is not 0");
static_assert(lib.count == 2 || lib.numbers[2] == version_patch,
              "WEBCPP_LIB_VERSION names another patch level than WEBCPP_VERSION");

}  // namespace
```

  `test/target_test.cpp` (after the notice):

```cpp
// <webcpp/config.hpp> defines exactly one WEBCPP_TARGET_* macro, the one of the target the
// build is for, which test/Jamfile tells this file by the toolset: WEBCPP_TEST_TARGET_WASIP2,
// WEBCPP_TEST_TARGET_WASIP3, WEBCPP_TEST_TARGET_EMSCRIPTEN, or none natively.

#include <webcpp/config.hpp>

#if defined(WEBCPP_TARGET_NATIVE) + defined(WEBCPP_TARGET_WASIP2) + \
        defined(WEBCPP_TARGET_WASIP3) + defined(WEBCPP_TARGET_EMSCRIPTEN) != 1
#error "config.hpp defines other than exactly one WEBCPP_TARGET_* macro"
#endif

#if defined(WEBCPP_TEST_TARGET_WASIP2)
#ifndef WEBCPP_TARGET_WASIP2
#error "a build for wasm32-wasip2 does not define WEBCPP_TARGET_WASIP2"
#endif
#elif defined(WEBCPP_TEST_TARGET_WASIP3)
#ifndef WEBCPP_TARGET_WASIP3
#error "a build for wasm32-wasip3 does not define WEBCPP_TARGET_WASIP3"
#endif
#elif defined(WEBCPP_TEST_TARGET_EMSCRIPTEN)
#ifndef WEBCPP_TARGET_EMSCRIPTEN
#error "a build for emscripten does not define WEBCPP_TARGET_EMSCRIPTEN"
#endif
#elif !defined(WEBCPP_TARGET_NATIVE)
#error "a native build does not define WEBCPP_TARGET_NATIVE"
#endif
```

  `test/exceptions_test.cpp` (after the notice):

```cpp
// <webcpp/config.hpp> defines WEBCPP_HAS_EXCEPTIONS exactly when webcpp has exceptions.
// test/Jamfile builds this file three times: as the build asks; with exception-handling=off,
// which WEBCPP_TEST_CONFIG_OFF tells this file; and with WEBCPP_NO_EXCEPTIONS defined as a
// developer defines it, which WEBCPP_TEST_CONFIG_DEVELOPER_OFF tells it.

#include <webcpp/config.hpp>

#ifndef BOOST_CONFIG_HPP
#error "<webcpp/config.hpp> does not include <boost/config.hpp>"
#endif

#if defined(WEBCPP_TEST_CONFIG_OFF) && !defined(BOOST_NO_EXCEPTIONS)
#error "exceptions_off is built with exceptions: exception-handling=off was not applied"
#endif

#ifdef WEBCPP_TEST_CONFIG_DEVELOPER_OFF
#ifdef WEBCPP_HAS_EXCEPTIONS
#error "WEBCPP_HAS_EXCEPTIONS is defined though the developer defined WEBCPP_NO_EXCEPTIONS"
#endif
// WEBCPP_NO_EXCEPTIONS is webcpp's alone: Boost keeps the exceptions the compiler has.
#if (defined(__cpp_exceptions) || defined(_CPPUNWIND)) && defined(BOOST_NO_EXCEPTIONS)
#error "WEBCPP_NO_EXCEPTIONS turned Boost's exceptions off too"
#endif
#elif defined(BOOST_NO_EXCEPTIONS) == defined(WEBCPP_HAS_EXCEPTIONS)
#error "WEBCPP_HAS_EXCEPTIONS is not defined exactly when the build has exceptions"
#endif
```

  `test/predefined_test.cpp` (after the notice):

```cpp
// A WEBCPP_TARGET_* macro or WEBCPP_HAS_EXCEPTIONS defined before <webcpp/config.hpp>, which
// test/Jamfile defines on the command line, stops the build with config.hpp's message.

// expected-error@webcpp/config.hpp:* {{are defined by <webcpp/config.hpp> alone}}
#include <webcpp/config.hpp>
```

  `test/unsupported_test.cpp` (after the notice):

```cpp
// A WebAssembly build for none of wasm32-wasip2, wasm32-wasip3 and Emscripten stops with
// config.hpp's message rather than read as native: test/Jamfile builds this file so.

// expected-error@webcpp/config.hpp:* {{unsupported target}}
#include <webcpp/config.hpp>
```

  (`-verify` matches a part of a message, which keeps each comment within
  clang-format's 100 columns: a reflowed `expected-error` line stops
  matching.)
- [ ] **Step 2: Stubs, so that each test fails for its own reason.**
  `include/webcpp/version.hpp` and `include/webcpp/config.hpp` hold only the
  notice and their include guards (`WEBCPP_VERSION_HPP`,
  `WEBCPP_CONFIG_HPP`).
- [ ] **Step 3: Measure the order of `--target`,** before trusting
  `unsupported_wasip1`: `B2 -n toolset=clang-wasip2 testing.launcher=wasmtime
  libs/config/test//unsupported_wasip1`. Expected: the compile line holds
  `--target=wasm32-wasip2` before `--target=wasm32-wasip1`, so clang takes
  wasip1. If the order is the other way round, delete `unsupported_wasip1`
  from the Jamfile, keep `unsupported_wasm`, and record the measurement in
  the ledger.
- [ ] **Step 4: Run them and watch each fail.** `B2 -a -k libs/config/test
  libs/config/example` natively. Expected, test by test:
  - `alone-config` passes (an empty header compiles alone);
  - `version`: `use of undeclared identifier 'WEBCPP_VERSION'`;
  - `target`: `config.hpp defines other than exactly one WEBCPP_TARGET_*
    macro`;
  - `exceptions`, `exceptions_off`, `exceptions_developer_off`:
    `<webcpp/config.hpp> does not include <boost/config.hpp>`;
  - `predefined_target`, `predefined_exceptions`, `unsupported_wasm`:
    `-verify`'s `'expected-error' diagnostics expected but not seen`;
  - `config_quick_start` (Step 6 below, written now too): its output
    `my_library: exceptions off` differs from its `.expected`.
- [ ] **Step 5: The headers.** `include/webcpp/version.hpp` (after the
  notice):

```cpp
/**
 webcpp's version: the one place it is written, which the superproject's build and its release
 tooling read.
*/
#ifndef WEBCPP_VERSION_HPP
#define WEBCPP_VERSION_HPP

/**
 webcpp's version as a number, `major * 100000 + minor * 100 + patch`, as `BOOST_VERSION` is
 Boost's.

 0.1.0 is 100 and 1.2.3 is 100203: `WEBCPP_VERSION % 100` is the patch level,
 `WEBCPP_VERSION / 100 % 1000` the minor version and `WEBCPP_VERSION / 100000` the major one.

 @see "Version", in the guide.
*/
#define WEBCPP_VERSION 100

/**
 webcpp's version as text, `"<major>_<minor>"`, with `"_<patch>"` after it when the patch level
 is not 0, as `BOOST_LIB_VERSION` is Boost's.

 `"0_1"` is 0.1.0 and `"1_2_3"` is 1.2.3.

 @see "Version", in the guide.
*/
#define WEBCPP_LIB_VERSION "0_1"

#endif  // WEBCPP_VERSION_HPP
```

  `include/webcpp/config.hpp` (after the notice):

```cpp
/**
 webcpp's configuration, which every webcpp library's own configuration includes first:
 Boost.Config, webcpp's version, the target a program is built for, and whether webcpp has
 exceptions.
*/
#ifndef WEBCPP_CONFIG_HPP
#define WEBCPP_CONFIG_HPP

// MrDocs parses webcpp natively and with exceptions, where only WEBCPP_TARGET_NATIVE and
// WEBCPP_HAS_EXCEPTIONS are defined: these definitions, each undone at once, are the ones the
// reference lists for the other macros.
#ifdef __MRDOCS__
/**
 Defined when the program is built for wasm32-wasip2, as a component of WASI 0.2.

 The compiler says so with `__wasip2__`, which wasi-sdk's clang defines for the target
 `wasm32-wasip2`.

 @see "Targets", in the guide.
*/
#define WEBCPP_TARGET_WASIP2
#undef WEBCPP_TARGET_WASIP2
/**
 Defined when the program is built for wasm32-wasip3, as a component of WASI 0.3.

 The compiler says so with `__wasip3__`, which wasi-sdk's clang defines for the target
 `wasm32-wasip3`.

 @see "Targets", in the guide.
*/
#define WEBCPP_TARGET_WASIP3
#undef WEBCPP_TARGET_WASIP3
/**
 Defined when the program is built with Emscripten, for a browser or for node.

 The compiler says so with `__EMSCRIPTEN__`.

 @see "Targets", in the guide.
*/
#define WEBCPP_TARGET_EMSCRIPTEN
#undef WEBCPP_TARGET_EMSCRIPTEN
/**
 Defined by the developer to disable the exceptions of every webcpp library at once, even in a
 build that has them.

 With it, `WEBCPP_HAS_EXCEPTIONS` is not defined, and so each library's
 `WEBCPP_<LIBRARY>_NO_EXCEPTIONS` is. Boost's own exceptions are left as the compiler has them:
 `BOOST_NO_EXCEPTIONS` is not defined by it.

 @see "Exceptions", in the guide.
*/
#define WEBCPP_NO_EXCEPTIONS
#undef WEBCPP_NO_EXCEPTIONS
#endif

#if defined(WEBCPP_TARGET_NATIVE) || defined(WEBCPP_TARGET_WASIP2) || \
    defined(WEBCPP_TARGET_WASIP3) || defined(WEBCPP_TARGET_EMSCRIPTEN) || \
    defined(WEBCPP_HAS_EXCEPTIONS)
#error "WEBCPP_TARGET_* and WEBCPP_HAS_EXCEPTIONS are defined by <webcpp/config.hpp> alone"
#endif

// The target, from the compiler's own macros alone, before Boost.Config: a WebAssembly target
// without a C++ library, wasm32-unknown-unknown, stops inside <boost/config.hpp>, and is named
// here first.
#if defined(__wasip2__)
#define WEBCPP_TARGET_WASIP2
#elif defined(__wasip3__)
#define WEBCPP_TARGET_WASIP3
#elif defined(__EMSCRIPTEN__)
#define WEBCPP_TARGET_EMSCRIPTEN
#elif defined(__wasm__)
#error "unsupported target: webcpp builds for wasm32-wasip2, wasm32-wasip3, Emscripten and natively"
#else
/**
 Defined when the program is built natively, for the host's system: for none of wasm32-wasip2,
 wasm32-wasip3 and Emscripten, and not for WebAssembly, which, outside those three targets,
 stops the build.

 @see "Targets", in the guide.
*/
#define WEBCPP_TARGET_NATIVE
#endif

#include <boost/config.hpp>
#include <webcpp/version.hpp>

#if !defined(BOOST_NO_EXCEPTIONS) && !defined(WEBCPP_NO_EXCEPTIONS)
/**
 Defined when webcpp has exceptions: when the compiler has them, so that Boost.Config defines no
 `BOOST_NO_EXCEPTIONS`, and the developer has not defined `WEBCPP_NO_EXCEPTIONS`.

 Each library's `WEBCPP_<LIBRARY>_NO_EXCEPTIONS` is defined when this is not, or when the
 developer defines it alone.

 @see "Exceptions", in the guide.
*/
#define WEBCPP_HAS_EXCEPTIONS
#endif

#endif  // WEBCPP_CONFIG_HPP
```

  (The unsupported-target `#error` line is exactly 100 columns, at
  clang-format's limit: a longer message would need a shorter wording.)
- [ ] **Step 6: The example.** `example/Jamfile` (after the notice):

```
project : requirements <library>/webcpp/config//config ;

import webcpp ;

webcpp.targets native wasip2 wasip3 emscripten ;

webcpp.example config_quick_start.cpp ;
```

  `example/config_quick_start.cpp` (after the notice):

```cpp
// How a library builds its own configuration on webcpp's, as every webcpp library's config.hpp
// does, and how a program chooses code for the target it is built for.

#include <webcpp/config.hpp>

#include <cstdio>
#include <string_view>

// tag::library-config[]
// my_library's own macro: defined when webcpp has no exceptions (the compiler has none, or the
// developer defined WEBCPP_NO_EXCEPTIONS), or when the developer defines it alone.
#if !defined(WEBCPP_HAS_EXCEPTIONS) && !defined(MY_LIBRARY_NO_EXCEPTIONS)
#define MY_LIBRARY_NO_EXCEPTIONS
#endif
// end::library-config[]

// tag::target[]
// Where the program's standard output goes, chosen when it is compiled.
constexpr std::string_view output()
{
#if defined(WEBCPP_TARGET_EMSCRIPTEN)
    return "the JavaScript console";
#elif defined(WEBCPP_TARGET_WASIP2) || defined(WEBCPP_TARGET_WASIP3)
    return "the WASI host's standard output";
#else
    return "the process's standard output";
#endif
}
// end::target[]

int main()
{
    static_assert(!output().empty());
    // tag::main[]
#ifdef MY_LIBRARY_NO_EXCEPTIONS
    std::puts("my_library: exceptions off");
#else
    std::puts("my_library: exceptions on");
#endif
    // end::main[]
}
```

  `example/config_quick_start.expected`: `my_library: exceptions on` and a
  newline (every lane builds with exceptions).
- [ ] **Step 7: The repository's files.** `meta/libraries.json`:

```json
{
    "key": "config",
    "name": "config",
    "authors": ["R"],
    "description": "webcpp's configuration, which every webcpp library includes first: the target a program is built for, whether webcpp has exceptions, and webcpp's version.",
    "category": ["Miscellaneous"],
    "maintainers": ["R <github.overboard312@passmail.net>"],
    "cxxstd": "20",
    "port-of": null
}
```

  `.github/workflows/ci.yml`: xactor's, with `library: config` and its first
  comment "config's CI: the superproject's reusable workflow, which builds
  this commit inside a checkout of webcpp, on every lane config declares,
  with its page, the lint and the test matrix."
  `README.md`, as xactor's is laid out: a first paragraph ("config is
  webcpp's configuration, as Boost.Config is Boost's: `<webcpp/config.hpp>`
  tells a program the target it is built for and whether webcpp has
  exceptions, and `<webcpp/version.hpp>` webcpp's version. Every webcpp
  library's own `config.hpp` includes it first."); a table of the eight
  macros of the Interfaces table; "Building and testing" (`b2
  libs/config/test libs/config/example`, and the WASI and emscripten
  commands of AGENTS.md chapter 1, `toolset=clang-wasip2
  testing.launcher=wasmtime`, `toolset=clang-wasip3
  testing.launcher=wasmtime`, `toolset=emscripten`); "Documentation"
  (`https://webcpporg.github.io/webcpp/libs/config/`); "Licence"
  (BSL-1.0). Its one C++ block is the example's region `library-config`,
  opened by `<!-- include::example/config_quick_start.cpp[tag=library-config,indent=0] -->`
  (doc-check compares them). `AGENTS.md`, opening with "Read
  [webcpp's AGENTS.md](../../AGENTS.md) first; this file holds only what is
  config's own.", then four bullets:
  - "**Macros only.** config declares no namespace; its macros are the
    collection's, `WEBCPP_*`, as Boost.Config's are `BOOST_*`, and its
    include guards `WEBCPP_CONFIG_HPP` and `WEBCPP_VERSION_HPP`."
  - "**`<webcpp/version.hpp>` lies outside `webcpp/config/`,** as Boost's
    `<boost/version.hpp>`: headers-alone takes `config.hpp`, and the test
    `version` compiles `version.hpp` alone."
  - "**The version is written once,** in `version.hpp`, and moves only by
    the release procedure (AGENTS.md chapter 11), right after a tag."
  - "**The target comes from the compiler,** never from b2, so a plain
    compiler gets it too; the target block comes before Boost.Config, which
    a WebAssembly target without a C++ library cannot compile."
- [ ] **Step 8: Run the lanes and watch them pass,** in parallel, from
  scratch, each with `--dump-tests --out-xml`: native clang
  (`bin/lane-clang`), `toolset=gcc-15`, `toolset=clang-wasip2
  testing.launcher=wasmtime`, `toolset=clang-wasip3
  testing.launcher=wasmtime`, `toolset=emscripten`, each over
  `libs/config/test libs/config/example`; then
  `python3 tools/report/report.py --lane native=... --lane gcc-15=... --lane
  wasip2=... --lane wasip3=... --lane emscripten=... --out bin/report-config`.
  Expected: exit 0; `alone-config`, `version`, `target`, `exceptions`,
  `exceptions_off`, `exceptions_developer_off` and `config_quick_start` pass
  on every lane; `predefined_target` and `predefined_exceptions` pass on
  native clang, wasip2 and wasip3 and are skipped on gcc-15 and emscripten;
  `unsupported_wasip1` passes on wasip2 alone; `unsupported_wasm` passes on
  native clang and is skipped on gcc-15 (`-verify` is clang's).
- [ ] **Step 9: Plant each defect, see its test fail, undo it.** One at a
  time, on the lanes named:
  - swap the `__wasip2__` and `__wasip3__` branches: `target` fails on
    wasip2 and wasip3;
  - move `#define WEBCPP_TARGET_NATIVE` out of its `#else`, after `#endif`:
    `target` fails on wasip2, wasip3 and emscripten ("other than exactly
    one");
  - write `__wasm32__x` for `__wasm__`: `unsupported_wasip1` and
    `unsupported_wasm` fail ("expected but not seen");
  - delete the predefined-macros `#error`: `predefined_target` and
    `predefined_exceptions` fail;
  - drop `!defined(BOOST_NO_EXCEPTIONS) &&`: `exceptions_off` fails;
  - drop `&& !defined(WEBCPP_NO_EXCEPTIONS)`: `exceptions_developer_off`
    fails;
  - write `WEBCPP_LIB_VERSION "0_1_0"`: `version` fails.
- [ ] **Step 10: The lint.** Run it in `W`. Expected: `lint: clean`, with
  `exceptions_off` analysed once, as b2 compiles it. If the compilation
  database writes it twice (its twin without exceptions is itself), or
  refuses it, `tools/lint/compile_commands.py` leaves out the twin of a
  program that declares `<exception-handling>off`, as it does for one that
  declares `on`, with a case in `tools/lint/lint_test.py` seen failing first,
  and Task 10 states it in chapter 6. `B2 -a doc` fails now with the index
  naming config's missing page: Task 2 adds it.
- [ ] **Step 11: Commit** in `W/libs/config`: `git add` each file of the list
  above, by path, then commit "config: webcpp's configuration, its version
  and the target, with their tests". The report's Changelog section holds
  the New Libraries line: "config: webcpp's configuration, which every
  library includes first: the target, exceptions and the version, from
  R."

### Task 2: config's page, its reference, and the names it reserves

**Files:**
- Modify (superproject, `W`): `tools/doc/reference.py` (the `MACROS` value,
  about line 373, and the call of `doc_comments.check`, about line 382),
  `tools/doc/doc_comments.py` (`check`, about line 425),
  `tools/doc/libraries.py` (`rows`, about line 121),
  `tools/doc/doc_comments_test.py`, `tools/test/doc_test.py`
- Create (`W/libs/config`): `doc/Jamfile`, `doc/config.adoc`,
  `doc/introduction.adoc`, `doc/targets.adoc`, `doc/exceptions.adoc`,
  `doc/version.adoc`, `doc/testing.adoc`

**Interfaces:**
- Consumes: config's headers and example (Task 1).
- `reference.py`, beside `PRESENTATION`:

```python
# config holds the collection's own macros, WEBCPP_*, as Boost.Config holds BOOST_*: its
# reference lists them all, the include guards apart, and it declares nothing in a namespace.
COLLECTION = 'config'


def macro_prefix(library: str) -> str:
    """The prefix of the macros the library's reference lists: WEBCPP_<NAME>_, or WEBCPP_ for
    config."""
    return 'WEBCPP_' if library == COLLECTION else f'WEBCPP_{library.upper()}_'
```

  `'MACROS': macro_prefix(library),` and
  `doc_comments.check(options.clang, library, include, [...],
  namespace_required=library != COLLECTION)`.
- `doc_comments.py`:

```python
def check(clang: str, library: str, include: Path, arguments: list[str],
          namespace_required: bool = True) -> list[Finding]:
    """The findings of the library's headers under include, read through arguments. A library
    declares its namespace, webcpp::<library>, unless namespace_required is False: config's
    headers hold macros alone."""
    objects = dump(clang, library, arguments)
    checker = Checker(objects, include)
    namespaces = [value for value in objects
                  if value.get('kind') == 'NamespaceDecl' and value.get('name') == library]
    if namespace_required and not any(checker.ours(checker.locations.of(value)[0])
                                      for value in namespaces):
        raise Unreadable(f'no declaration of webcpp::{library} is in a header under {include}')
    for namespace in namespaces:
        checker.visit(namespace, ['webcpp'], False, True)
    return checker.findings()
```

- `libraries.py`, before `rows`, and its first lines in the loop:

```python
# The names no library may take: config's macros are WEBCPP_TARGET_*, WEBCPP_HAS_*, WEBCPP_NO_*,
# WEBCPP_LIB_* and WEBCPP_VERSION*, which a library's WEBCPP_<NAME>_* would meet.
RESERVED = ('has', 'lib', 'no', 'target', 'version')
```

```python
        if library.name in RESERVED:
            raise Invalid(f'{library}: a library is not named {library.name}, which config\'s '
                          f'macros take, WEBCPP_{library.name.upper()}_*; the names taken are '
                          f'{", ".join(RESERVED)}')
```

- The page's section titles, which config's `@see` lines name: "Targets",
  "Exceptions", "Version"; and "How config is tested".

Measured before this plan (a macros-only `config` in a scratch superproject,
`reference.py` with the prefix `WEBCPP_`): MrDocs 2026.9.29 writes the macros
of `config.hpp` and `version.hpp` (it reads `version.hpp` through
`config.hpp`) without an empty-corpus error, and `reference.py` then stops at
`doc_comments.py: no declaration of webcpp::config is in a header under ...`.

- [ ] **Step 1: Write the failing tests.** `doc_comments_test.py`, with
  `import doc_comments` added to its imports, a case appended to its list of
  cases:

```python
def test_macros_only_library_needs_no_namespace(library: Library) -> None:
    # config holds macros alone, as Boost.Config does: with no namespace of its own required,
    # the check passes; required, as for every other library, it fails naming webcpp::config.
    include = library.root / 'macros'
    (include / 'webcpp').mkdir(parents=True)
    (include / 'webcpp/config.hpp').write_text('/** A macro. */\n#define WEBCPP_PLANTED 1\n')
    source = library.root / 'macros.cpp'
    source.write_text('#include <webcpp/config.hpp>\n')
    arguments = ['-std=c++20', f'-I{include}', str(source)]
    assert doc_comments.check(clang(), 'config', include, arguments,
                              namespace_required=False) == []
    try:
        doc_comments.check(clang(), 'config', include, arguments)
    except doc_comments.Unreadable as unreadable:
        assert 'webcpp::config' in str(unreadable), unreadable
    else:
        raise AssertionError('a library without its namespace passed where one is required')
```

  `tools/test/doc_test.py`, each case added to its `CASES`:

```python
CONFIG_HPP = ('#ifndef WEBCPP_CONFIG_HPP\n#define WEBCPP_CONFIG_HPP\n\n'
              '#include <webcpp/version.hpp>\n\n'
              '#ifdef __MRDOCS__\n/** Defined by the developer. */\n#define WEBCPP_NO_EXCEPTIONS\n'
              '#undef WEBCPP_NO_EXCEPTIONS\n#endif\n\n'
              '/** Defined natively. */\n#define WEBCPP_TARGET_NATIVE\n\n'
              '/** Defined with exceptions. */\n#define WEBCPP_HAS_EXCEPTIONS\n\n#endif\n')
VERSION_HPP = ('#ifndef WEBCPP_VERSION_HPP\n#define WEBCPP_VERSION_HPP\n\n'
               '/** The version as a number. */\n#define WEBCPP_VERSION 100\n\n'
               '/** The version as text. */\n#define WEBCPP_LIB_VERSION "0_1"\n\n#endif\n')


def test_config_reference_lists_every_webcpp_macro(root):
    # A library named config, macros only, as config's headers are: its reference lists every
    # WEBCPP_* macro but the include guards, and an undocumented one fails, named.
    prepare(root)
    config = add_paged(root, 'config')
    (config / 'include/webcpp/config.hpp').write_text(CONFIG_HPP)
    (config / 'include/webcpp/version.hpp').write_text(VERSION_HPP)
    harness.expect(harness.run_b2(root, 'libs/config/doc'), True)
    html = (config / 'doc/html/index.html').read_text()
    for macro in ('WEBCPP_VERSION', 'WEBCPP_LIB_VERSION', 'WEBCPP_TARGET_NATIVE',
                  'WEBCPP_NO_EXCEPTIONS', 'WEBCPP_HAS_EXCEPTIONS'):
        assert f'id="{macro}"' in html, (macro, html[-3000:])
    assert 'WEBCPP_CONFIG_HPP' not in html and 'WEBCPP_VERSION_HPP' not in html
    harness.replace(config / 'include/webcpp/version.hpp', '/** The version as a number. */\n', '')
    harness.expect(harness.run_b2(root, 'libs/config/doc'), False, 'WEBCPP_VERSION')


def test_other_libraries_list_only_their_own_macros(root):
    # Any other library's reference lists its own WEBCPP_<NAME>_* alone.
    prepare(root)
    other = add_paged(root, 'other')
    header = other / 'include/webcpp/other.hpp'
    header.write_text(header.read_text().replace(
        '#define WEBCPP_OTHER_HPP\n',
        '#define WEBCPP_OTHER_HPP\n\n/** Its own. */\n#define WEBCPP_OTHER_OWN 1\n\n'
        '/** Not its own. */\n#define WEBCPP_ELSEWHERE 1\n'))
    harness.expect(harness.run_b2(root, 'libs/other/doc'), True)
    html = (other / 'doc/html/index.html').read_text()
    assert 'id="WEBCPP_OTHER_OWN"' in html and 'WEBCPP_ELSEWHERE' not in html, html[-3000:]


def test_a_library_named_as_config_s_macros_is_refused(root):
    # A library named target would meet WEBCPP_TARGET_*: the index refuses it, naming it.
    prepare(root)
    add_paged(root, 'target')
    harness.expect(harness.run_b2(root, 'doc'), False,
                   'a library is not named target, which config\'s macros take')
```

- [ ] **Step 2: Run them and watch them fail.** `python3
  tools/doc/doc_comments_test.py` (`TypeError: check() got an unexpected
  keyword argument 'namespace_required'`); `python3 tools/test/doc_test.py
  test_config_reference_lists_every_webcpp_macro
  test_other_libraries_list_only_their_own_macros
  test_a_library_named_as_config_s_macros_is_refused` (the first fails with
  `no declaration of webcpp::config`; the second passes already, and stays;
  the third builds where it must fail).
- [ ] **Step 3: Implement** the code above, and `reference.py`'s module
  docstring: "the macros of the library, `WEBCPP_<NAME>_*`, or every
  `WEBCPP_*` for config, which holds the collection's".
- [ ] **Step 4: Run them and watch them pass.** Both files green, and every
  other `tools/**/*_test.py`.
- [ ] **Step 5: The page.** `doc/Jamfile` (after the notice): `import webcpp
  ;`, `webcpp.doc config : config.adoc ;`, `webcpp.reference config ;`.
  `doc/config.adoc`:

```
// (the notice)

= config: webcpp's configuration for {cpp}
:toc: left
:toclevels: 1
:idprefix:
:sectanchors:
:example-caption!:
:attribute-missing: warn

include::introduction.adoc[]

include::targets.adoc[]

include::exceptions.adoc[]

include::version.adoc[]

include::testing.adoc[]

[#reference]
== Reference

The macros of `<webcpp/config.hpp>` and `<webcpp/version.hpp>`.

include::{reference}[leveloffset=+1]
```

  The sections, each a file opening with the notice and an anchor that
  starts with `config-`:
  - `[#config-introduction] == Introduction`: config is webcpp's
    configuration, as Boost.Config is Boost's; every webcpp library's
    `config.hpp` includes it first; it includes `<boost/config.hpp>` and
    `<webcpp/version.hpp>`; it is macros only. Shows the example's region
    `library-config` (`include::{examples}/config_quick_start.cpp[tag=library-config,indent=0]`)
    and the program's output (`include::{examples}/config_quick_start.expected[]`).
  - `[#config-targets] == Targets`: the four macros and the compiler macro
    that defines each; exactly one is defined; they come from the compiler,
    so a plain compiler gets them; a WebAssembly build for none of the three
    WebAssembly targets stops with "unsupported target" rather than read as
    native; one defined beforehand stops the build. Shows the region
    `target`.
  - `[#config-exceptions] == Exceptions`: `WEBCPP_HAS_EXCEPTIONS`,
    `WEBCPP_NO_EXCEPTIONS`, and how a library's own
    `WEBCPP_<LIBRARY>_NO_EXCEPTIONS` follows them, as the introduction's
    region shows (`<<config-introduction>>`, not repeated); Boost's own
    exceptions are untouched.
  - `[#config-version] == Version`: the two macros and the formula;
    `version.hpp` is the one place the version is written, and moves to the
    next version right after each release (AGENTS.md chapter 11, linked as
    `https://github.com/webcpporg/webcpp/blob/main/AGENTS.md[AGENTS.md]`).
  - `[#config-testing] == How config is tested`: the lanes, with
    `{n-tests}`, `{n-tests-native}`, `{n-tests-wasip2}`, `{n-tests-wasip3}`,
    `{n-tests-emscripten}` and `{n-examples}`; the commands; the test matrix
    (`https://webcpporg.github.io/webcpp/report/`).
- [ ] **Step 6: Build it.** `B2 -a doc`. Expected: green;
  `doc/html/index.html` lists config, "original"; config's page shows the
  reference's "Macros" with the eight macros. Open the page at 320 and 1440
  px: nothing overflows.
- [ ] **Step 7: Commit.** Superproject (`W`): `git add tools/doc/reference.py
  tools/doc/doc_comments.py tools/doc/libraries.py
  tools/doc/doc_comments_test.py tools/test/doc_test.py`, commit "tools/doc:
  config's reference lists every WEBCPP_ macro, and the names its macros
  take are refused". `W/libs/config`: `git add doc/Jamfile doc/config.adoc
  doc/introduction.adoc doc/targets.adoc doc/exceptions.adoc doc/version.adoc
  doc/testing.adoc`, commit "config: its page, with the reference of its
  macros". The report's Changelog section: Tools and build, "Added the names
  `target`, `has`, `no`, `lib` and `version` to those no library may take."

### Task 3: Every library's changelog

**Files:**
- Create: `doc/changelog.adoc` in `W/libs/config`, `W/libs/xactor`,
  `W/libs/xstate`, `W/libs/pratt`, `W/libs/wasi`
- Modify: each page's entry file, `doc/config.adoc`, `doc/xactor.adoc`,
  `doc/xstate.adoc`, `doc/pratt.adoc`, `doc/wasi.adoc`: one line at its end

**Interfaces:**
- Produces the file every later task writes into, and that Task 5 reads:

```
// (the notice)

[appendix]
[#<library>-changelog]
== Changelog

What changed in <library> in each version of webcpp, newest first, one line
per change. "Unreleased" is what the next version brings. The release notes
of every version are on webcpp's index page.

[#<library>-changelog-unreleased]
=== Unreleased

// tag::unreleased[]
* <a line, which may wrap
onto the next line>
// end::unreleased[]
```

  A released version is a section after "Unreleased",
  `[#<library>-changelog-<a-b-c>]` and `=== webcpp <a.b.c>`, with its region
  `// tag::<a.b.c>[]` to `// end::<a.b.c>[]`. The regions hold bullets only;
  a region with no bullet is empty (config's today).
- Each page includes it last, after the reference and every other appendix:
  `include::changelog.adoc[]`.

- [ ] **Step 1: The failing check.** Add `include::changelog.adoc[]` to
  xactor's page first, then `B2 libs/xactor/doc`. Expected: Asciidoctor's
  warning `include file not found: .../changelog.adoc`, which fails the build.
- [ ] **Step 2: Write each file.** The bullets are the pending file's section
  of the library as it reads when the task runs (below as of 2026-10-09; take
  any line added since), then this task's own line, last: "Added a changelog,
  the page's last appendix, with each version's changes."
  - xactor:

```
* *Breaking change:* removed the native build without exceptions; RTTI is
never restricted in any build.
* Changed wasip2 to build with exceptions, the last target still built
without them.
* Added the `WEBCPP_XACTOR_NO_EXCEPTIONS` configuration macro, defined
automatically or by the developer.
* Updated the page to link xstate, now that it builds on xactor.
* Added a changelog, the page's last appendix, with each version's changes.
```

  - xstate:

```
* Changed wasip2 to build with exceptions, as native and wasip3 already did.
* Added the `WEBCPP_XSTATE_NO_EXCEPTIONS` configuration macro, defined
automatically or by the developer.
* Added a changelog, the page's last appendix, with each version's changes.
```

  - pratt:

```
* Fixed `calc::environment` failing to compile with a stateful allocator
that has no default constructor.
* Changed wasip2 to build with exceptions, as native and wasip3 already did.
* Added the `WEBCPP_PRATT_NO_EXCEPTIONS` configuration macro, defined
automatically or by the developer.
* Added a changelog, the page's last appendix, with each version's changes.
```

  - wasi:

```
* Fixed a status outside 100-599 answering 200 OK; it now answers an
internal error.
* Fixed the handler reading the response's content type after freeing the
request that held it.
* Fixed a content type the host refused being dropped silently; it now
answers an internal error.
* Changed wasip2 to build with exceptions, as wasip3 already did.
* Added the `WEBCPP_WASI_NO_EXCEPTIONS` configuration macro, defined
automatically or by the developer.
* Updated the documentation on wasmtime's version requirements, the by-hand
build, and an exception escaping the main on wasip3.
* Added a changelog, the page's last appendix, with each version's changes.
```

  - config: the region empty (config is new in 0.1.0; the release notes
    announce it).
- [ ] **Step 3: Build every page and watch them pass.** `B2 -a doc` (one doc
  build). Expected: green; each page's table of contents ends with
  "Appendix ?: Changelog", which shows "Unreleased" and its bullets, each
  wrapped line in its bullet, and no `tag::` or `end::` line is visible. Then
  the lint: `lint: clean`.
- [ ] **Step 4: Commit** in each library: `git add doc/changelog.adoc
  doc/<name>.adoc`, commit "<name>: a changelog, the page's last appendix".
  Record the five commits in the ledger.

### Task 4: The release notes, on the index page

**Files:**
- Create: `doc/releases/0.1.0.adoc`, `tools/release/template.adoc`
- Modify: `doc/index.adoc` (a section appended)

**Interfaces:**
- Consumes: the changelog file of Task 3.
- **The notes of a version,** `doc/releases/<a.b.c>.adoc`, with `<anchor>`
  the version with dashes (`0-1-0`):

```
// (the notice)

[#webcpp-<anchor>]
= webcpp <a.b.c>

In preparation: no tag names this version yet, and its libraries' entries
are their changelogs' "Unreleased" sections.

[#webcpp-<anchor>-new-libraries]
== New Libraries

* link:{library-pages}<name>/{library-page}[<name>]: <what it is>, from <authors>.

[#webcpp-<anchor>-updated-libraries]
== Updated Libraries

[#webcpp-<anchor>-<name>]
=== link:{library-pages}<name>/{library-page}[<name>]

include::{webcpp-root}/libs/<name>/doc/changelog.adoc[tag=unreleased]

[#webcpp-<anchor>-removed-libraries]
== Removed Libraries

* <name>: <what went out, and what replaces it>.

[#webcpp-<anchor>-tools-and-build]
== Tools and build

* <a line>

[#webcpp-<anchor>-compilers-tested]
== Compilers tested

* <the name of each CI lane, tools/ci/matrix.py's LANES, in its order>
```

  A section with nothing to list holds the one paragraph `None.`. Libraries
  are in alphabetical order in each section. The paragraph "In preparation:
  ..." is there exactly while the version is in preparation, and its
  includes then read `tag=unreleased`; the release procedure removes it and
  makes them `tag=<a.b.c>` (Task 10's chapter 11).
- **`tools/release/template.adoc`:** that file, with `@VERSION@` and
  `@ANCHOR@` in place of the version and its anchor, the paragraph "In
  preparation: ...", and `None.` in each section. The release procedure
  opens the next version with `sed -e 's/@VERSION@/<a.b.c>/g' -e
  's/@ANCHOR@/<a-b-c>/g' tools/release/template.adoc >
  doc/releases/<a.b.c>.adoc`.
- **The index,** `doc/index.adoc`, appended after "Getting started":

```
[#releases]
== Releases

The release notes of each version of webcpp, newest first: what came into
the collection, what changed in each library, and what went out.

include::releases/0.1.0.adoc[leveloffset=+2]
```

- [ ] **Step 1: The failing check.** Append the index's section Releases first,
  then `B2 doc`. Expected: Asciidoctor's warning `include file not found:
  .../releases/0.1.0.adoc`, which fails the build.
- [ ] **Step 2: The notes of 0.1.0.** `doc/releases/0.1.0.adoc` in the form
  above, in preparation. New Libraries, from the pending file's section and
  Task 1's report:

```
* link:{library-pages}config/{library-page}[config]: webcpp's configuration,
which every library includes first: the target, exceptions and the version,
from R.
* link:{library-pages}pratt/{library-page}[pratt]: a header-only
Pratt-parsing engine for {cpp}20, with a decimal calculator built on it,
from R.
* link:{library-pages}wasi/{library-page}[wasi]: a header-only HTTP handler
for {cpp}20 programs built as WebAssembly components, covering WASI 0.2 and
WASI 0.3, from R.
* link:{library-pages}xactor/{library-page}[xactor]: a header-only,
deterministic actor system for {cpp}20, from R.
* link:{library-pages}xstate/{library-page}[xstate]: a header-only port of
XState's state machines and actors for {cpp}20, from R.
```

  Updated Libraries and Removed Libraries: `None.` (every library is new in
  0.1.0). Tools and build: the pending file's "Tools and build" bullets as
  they read when the task runs (lint and CI lines included: they change what
  a contributor meets), Task 2's line, then this task's, "Added each
  version's release notes to the index page, newest first." Compilers
  tested: one bullet per name of `tools/ci/matrix.py`'s `LANES`, in its order
  (today "GCC 14", "GCC 15", "Clang 18 (libstdc++)", "Clang 22 (libstdc++)",
  "Apple Clang (macOS 15)", "MSVC 14.3 (Visual Studio 2022)", "MSVC 14.5
  (Visual Studio 2026)", "Emscripten 6.0.11 (node)", "wasm32-wasip2
  (wasi-sdk 34, wasmtime 47.0.3)", "wasm32-wasip3 (wasi-sdk 34, wasmtime
  47.0.3)"). `tools/release/template.adoc` as above. The index's section as
  above.
- [ ] **Step 3: Build it.** `B2 -a doc` and `B2 -a doc -sWEBCPP_INDEX=site`
  (one at a time). Expected: green; the index ends with "Releases" and
  "webcpp 0.1.0", whose five sections show, each New Libraries name a link to
  its page; at 320 px nothing overflows. The lint: `lint: clean`.
- [ ] **Step 4: Commit** in `W`: `git add doc/index.adoc
  doc/releases/0.1.0.adoc tools/release/template.adoc`, commit "doc: each
  version's release notes on the index page, with the notes of 0.1.0".
  The 0.1.0 notes include no library's region (every library is new), so
  doc-check needs no change yet: Task 11 teaches it a library's tracked
  file, once stage 4's doc-check is final.

### Task 5: The changelog's form, checked by the lint

**Files:**
- Create: `tools/release/changelog.py`, `tools/release/changelog_test.py`
- Modify: `tools/lint/rules.py` (the docstring's list, the rule, the two
  tables, about lines 579-594), `tools/lint/lint.sh` (rule 17, before the
  final verdict), `tools/lint/lint_test.py` (one case), `pyrightconfig.json`
  (`"tools/release"` in `extraPaths`), `doc/releases/0.1.0.adoc` (this
  task's line)

**Interfaces:**
- Consumes: the forms of Tasks 3 and 4.
- Produces `tools/release/changelog.py`, which Tasks 6 to 9 and the lint
  use: `VERBS`, `BREAKING`, `MOST`, `NONE`, `IN_PREPARATION`, `SECTIONS`,
  `Bullet`, `Region`, `Notes`, `Finding`, `bullets`, `line_findings`,
  `parse_changelog`, `changelog_findings`, `parse_notes`, `notes_findings`,
  `file_findings`.

- [ ] **Step 1: Write the failing tests.** `tools/release/changelog_test.py`:

```python
#!/usr/bin/env python3
# (the notice)

"""Checks tools/release/changelog.py: a library's changelog and a version's release notes in
AGENTS.md chapter 11's form pass, wrapped lines included; a line with no verb of the list, a
cross-reference, an eleventh line, text outside a bullet, a section out of order or without its
region, and notes out of form each fail, at their line. Run with the names of some cases to run
only those."""

from __future__ import annotations

import sys
from collections.abc import Callable
from pathlib import Path

import changelog

ROOT = Path(__file__).resolve().parents[2]


def log(name: str, *sections: tuple[str, list[str]]) -> str:
    """A library's changelog: each section a title, Unreleased or webcpp <a.b.c>, and its lines,
    each line of the file as written."""
    text = f'// The notice.\n\n[appendix]\n[#{name}-changelog]\n== Changelog\n\nWhat changed.\n'
    for title, lines in sections:
        tag = 'unreleased' if title == 'Unreleased' else title.removeprefix('webcpp ')
        body = ''.join(f'{line}\n' for line in lines)
        text += (f'\n[#{name}-changelog-{tag.replace(".", "-")}]\n=== {title}\n\n'
                 f'// tag::{tag}[]\n{body}// end::{tag}[]\n')
    return text


def line_of(text: str, line: str) -> int:
    """The number of the line of text that is line, which must be there once."""
    numbers = [number for number, held in enumerate(text.split('\n'), 1) if held == line]
    assert len(numbers) == 1, (line, numbers)
    return numbers[0]


def test_a_changelog_in_form_passes() -> None:
    text = log('alpha', ('Unreleased', ['* Added a line that wraps', 'onto the next one.',
                                        '* *Breaking change:* removed `x`; use `y`.']),
               ('webcpp 0.1.0', ['* Fixed a crash.']))
    assert changelog.changelog_findings(text, 'alpha') == []
    regions, _ = changelog.parse_changelog(text, 'alpha')
    assert [(region.title, region.tag) for region in regions] == [
        ('Unreleased', 'unreleased'), ('webcpp 0.1.0', '0.1.0')]
    assert regions[0].bullets[0].text == 'Added a line that wraps onto the next one.'


def test_wrapped_lines_are_one_bullet() -> None:
    # A continuation that starts with a verb of the list is no bullet of its own; a bullet that
    # starts with none fails once, at the line it starts on.
    text = log('alpha', ('Unreleased', ['* Added a line whose next line', 'Added nothing.',
                                        '* Made a mistake that wraps', 'onto the next one.']))
    assert changelog.changelog_findings(text, 'alpha') == [
        (line_of(text, '* Made a mistake that wraps'),
         'a line starts with *Breaking change:* or one of Added, Changed, Deprecated, Fixed, '
         'Improved, Removed, Updated: Made a mistake that wraps onto the next one.')]


def test_eleven_wrapped_lines_fail() -> None:
    lines = [part for number in range(11) for part in (f'* Added line {number}', 'wrapped.')]
    text = log('alpha', ('Unreleased', lines))
    found = changelog.changelog_findings(text, 'alpha')
    assert found == [(line_of(text, '=== Unreleased'),
                      'Unreleased holds 11 lines, and a library holds at most 10 per version')]


def test_text_outside_a_bullet_and_a_cross_reference_fail() -> None:
    text = log('alpha', ('Unreleased', ['* Added <<alpha-intro>>.', '', 'A paragraph.']))
    assert changelog.changelog_findings(text, 'alpha') == [
        (line_of(text, '* Added <<alpha-intro>>.'), 'a line holds no cross-reference, <<...>>: '
                                                    'the index page shows it too'),
        (line_of(text, 'A paragraph.'), 'not a line of a changelog, which starts with "* ": '
                                        'A paragraph.')]


def test_sections_in_order_each_with_its_region() -> None:
    late = log('alpha', ('webcpp 0.1.0', []), ('Unreleased', []))
    assert (line_of(late, '[appendix]'), 'a changelog has one section "Unreleased", first') in \
        changelog.changelog_findings(late, 'alpha')
    old = log('alpha', ('Unreleased', []), ('webcpp 0.1.0', []), ('webcpp 0.2.0', []))
    assert (line_of(old, '[appendix]'),
            'the versions of a changelog come newest first, each once') in \
        changelog.changelog_findings(old, 'alpha')
    open_region = log('alpha', ('Unreleased', ['* Added a line.'])).replace(
        '// end::unreleased[]\n', '')
    assert (line_of(open_region, '// tag::unreleased[]'),
            'the region unreleased has no // end::unreleased[]') in \
        changelog.changelog_findings(open_region, 'alpha')
    headless = '[appendix]\n[#other-changelog]\n== Changelog\n'
    assert changelog.changelog_findings(headless, 'alpha') == [
        (1, 'a changelog opens, after its notice, with [appendix], [#alpha-changelog], '
            '== Changelog')]


def notes(*, preparing: bool = True, new: str = 'None.', updated: str = 'None.',
          tools: str = 'None.', compilers: str = 'None.') -> str:
    """Release notes of 0.2.0, each section's body as given."""
    text = '// The notice.\n\n[#webcpp-0-2-0]\n= webcpp 0.2.0\n'
    if preparing:
        text += '\n' + '\n'.join(changelog.IN_PREPARATION) + '\n'
    for slug, title, body in (('new-libraries', 'New Libraries', new),
                              ('updated-libraries', 'Updated Libraries', updated),
                              ('removed-libraries', 'Removed Libraries', 'None.'),
                              ('tools-and-build', 'Tools and build', tools),
                              ('compilers-tested', 'Compilers tested', compilers)):
        text += f'\n[#webcpp-0-2-0-{slug}]\n== {title}\n\n{body}\n'
    return text


UPDATED = ('[#webcpp-0-2-0-alpha]\n=== link:{library-pages}alpha/{library-page}[alpha]\n\n'
           'include::{webcpp-root}/libs/alpha/doc/changelog.adoc[tag=unreleased]')


def test_release_notes_in_form() -> None:
    text = notes(new='* link:{library-pages}gamma/{library-page}[gamma]: a library,\n'
                     'from R.',
                 updated=UPDATED, tools='* Added a target.', compilers='* GCC 14')
    parsed, found = changelog.parse_notes(text, '0.2.0')
    assert found == [], found
    assert parsed.preparing
    assert parsed.new == [('gamma', 'a library, from R.')]
    assert parsed.updated == [('alpha', 'unreleased')]
    assert [bullet.text for bullet in parsed.tools] == ['Added a target.']
    assert parsed.compilers == ['GCC 14']
    frozen, found = changelog.parse_notes(notes(preparing=False), '0.2.0')
    assert found == [] and not frozen.preparing and frozen.compilers is None


def test_release_notes_out_of_form() -> None:
    text = notes(new='* link:{library-pages}gamma/{library-page}[gamma]: a library.')
    _, found = changelog.parse_notes(text, '0.2.0')
    assert found == [(line_of(text, '* link:{library-pages}gamma/{library-page}[gamma]: a '
                                    'library.'),
                      'not a line of new-libraries: link:{library-pages}gamma/{library-page}'
                      '[gamma]: a library.')], found
    unsorted = notes(new='* link:{library-pages}zeta/{library-page}[zeta]: z, from R.\n'
                         '* link:{library-pages}eta/{library-page}[eta]: e, from R.')
    assert any('in alphabetical order' in message
               for _, message in changelog.parse_notes(unsorted, '0.2.0')[1])
    swapped = notes().replace('== Removed Libraries', '== Tools and build', 1)
    assert changelog.parse_notes(swapped, '0.2.0')[1][0][1].startswith(
        'release notes hold five sections, in order')
    assert changelog.notes_findings(notes(tools='* Built a thing.'), '0.2.0') != []


def test_findings_by_path() -> None:
    text = log('alpha', ('Unreleased', ['* Made a mistake.']))
    assert changelog.file_findings('libs/alpha/doc/changelog.adoc', text) != []
    assert changelog.file_findings('libs/alpha/doc/other.adoc', text) == []
    assert changelog.file_findings('doc/releases/0.2.0.adoc', notes(tools='* Made it.')) != []
    assert changelog.file_findings('doc/index.adoc', 'anything') == []


def test_the_tree_s_changelogs_keep_their_form() -> None:
    # The real files, read only: what the lint's rule changelog reads on every run.
    paths = [*ROOT.glob('libs/*/doc/changelog.adoc'), *ROOT.glob('doc/releases/*.adoc')]
    for path in paths:
        relative = path.relative_to(ROOT).as_posix()
        assert changelog.file_findings(relative, path.read_text(encoding='utf-8')) == [], relative


CASES: list[Callable[[], None]] = [
    test_a_changelog_in_form_passes,
    test_wrapped_lines_are_one_bullet,
    test_eleven_wrapped_lines_fail,
    test_text_outside_a_bullet_and_a_cross_reference_fail,
    test_sections_in_order_each_with_its_region,
    test_release_notes_in_form,
    test_release_notes_out_of_form,
    test_findings_by_path,
    test_the_tree_s_changelogs_keep_their_form,
]


def main(argv: list[str]) -> int:
    unknown = set(argv) - {case.__name__ for case in CASES}
    if unknown:
        print(f'changelog_test: no case named {", ".join(sorted(unknown))}', file=sys.stderr)
        return 2
    for case in CASES:
        if argv and case.__name__ not in argv:
            continue
        case()
        print(f'{case.__name__}: ok')
    print('changelog_test: ok')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
```

  `tools/lint/lint_test.py`, a case added to its cases:

```python
CHANGELOG = (CPP + '\n[appendix]\n[#demo-changelog]\n== Changelog\n\nWhat changed.\n\n'
             '[#demo-changelog-unreleased]\n=== Unreleased\n\n// tag::unreleased[]\n'
             '* Added a line that\nwraps.\n{line}\n// end::unreleased[]\n')


def test_changelog(root):
    prepare(root)
    # A library's changelog keeps AGENTS.md chapter 11's form: a wrapped line is one line; one
    # that starts with no verb of the list fails, at its line.
    write(root, 'libs/demo/doc/changelog.adoc', CHANGELOG.format(line='* Fixed a crash.'))
    commit(root / 'libs/demo', 'A changelog')
    expect_clean(lint(root))
    text = CHANGELOG.format(line='* Made a mistake.')
    write(root, 'libs/demo/doc/changelog.adoc', text)
    number = text.split('\n').index('* Made a mistake.') + 1
    expect_alone(lint(root), 'changelog',
                 [f'libs/demo/doc/changelog.adoc:{number}: a line starts with'])
```

- [ ] **Step 2: Run them and watch them fail.** `python3
  tools/release/changelog_test.py`: `ModuleNotFoundError: No module named
  'changelog'`. `python3 tools/lint/lint_test.py test_changelog`: the lint
  passes the planted line (no rule `changelog`).
- [ ] **Step 3: Write `tools/release/changelog.py`:**

```python
#!/usr/bin/env python3
# (the notice)

"""The form of webcpp's changelog files (AGENTS.md chapter 11), which tools/lint/rules.py reads
on every lint run and tools/release/release.py before a release.

A library's changelog, libs/<name>/doc/changelog.adoc, after its notice: [appendix],
[#<name>-changelog] and == Changelog, its paragraphs, then the section Unreleased,
[#<name>-changelog-unreleased] and === Unreleased, its lines between // tag::unreleased[] and
// end::unreleased[], then each released version, newest first, [#<name>-changelog-<a-b-c>],
=== webcpp <a.b.c> and its region <a.b.c>.

A version's release notes, doc/releases/<a.b.c>.adoc: [#webcpp-<a-b-c>] and = webcpp <a.b.c>,
the paragraph IN_PREPARATION while the version is in preparation, then SECTIONS, each
[#webcpp-<a-b-c>-<slug>] and == <title>, each holding its entries in alphabetical order, or the
paragraph None.

A line is a bullet: "* " at the start of a line, then the lines after it up to a blank line,
the next bullet or the region's end, which continue it. It starts with "*Breaking change:* "
or with one of VERBS, and holds no cross-reference, <<...>>, since the index page shows it too.
A library's version holds at most MOST of them.
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field
from pathlib import PurePosixPath

VERBS = ('Added', 'Changed', 'Deprecated', 'Fixed', 'Improved', 'Removed', 'Updated')
BREAKING = '*Breaking change:* '
MOST = 10
NONE = 'None.'
IN_PREPARATION = ["In preparation: no tag names this version yet, and its libraries' entries",
                  'are their changelogs\' "Unreleased" sections.']
SECTIONS = (('new-libraries', 'New Libraries'), ('updated-libraries', 'Updated Libraries'),
            ('removed-libraries', 'Removed Libraries'), ('tools-and-build', 'Tools and build'),
            ('compilers-tested', 'Compilers tested'))

ANCHOR = re.compile(r'\[#([\w-]+)\]')
REGION_START = re.compile(r'// tag::([\w.]+)\[\]')
REGION_END = re.compile(r'// end::([\w.]+)\[\]')
RELEASED = re.compile(r'webcpp (\d+\.\d+\.\d+)')
NAME = r'[a-z][a-z0-9_]*'
NEW = re.compile(r'link:\{library-pages\}(?P<name>' + NAME + r')/\{library-page\}\[(?P=name)\]: '
                 r'(?P<what>.+, from .+\.)')
REMOVED = re.compile(r'(?P<name>' + NAME + r'): (?P<what>.+\.)')
UPDATED = re.compile(r'\[#webcpp-(?P<anchor>[\d-]+)-(?P<name>' + NAME + r')\]\n'
                     r'=== link:\{library-pages\}(?P=name)/\{library-page\}\[(?P=name)\]\n\n'
                     r'include::\{webcpp-root\}/libs/(?P=name)/doc/changelog\.adoc'
                     r'\[tag=(?P<tag>[\w.]+)\]')

Finding = tuple[int, str]


@dataclass
class Bullet:
    """A line of a changelog: the line its bullet starts on, and its text without the "* ", its
    lines joined by single spaces."""

    line: int
    text: str


@dataclass
class Region:
    """A section of a library's changelog, Unreleased or a version: its title, its region's tag,
    the line of its title, and its lines."""

    title: str
    tag: str
    line: int
    bullets: list[Bullet]


@dataclass
class Notes:
    """A version's release notes as their sections list them: whether the version is in
    preparation; each new and removed library with its text; each updated library with the tag
    its include names; the lines of Tools and build; the lanes of Compilers tested, None for
    None."""

    preparing: bool = False
    new: list[tuple[str, str]] = field(default_factory=list)
    updated: list[tuple[str, str]] = field(default_factory=list)
    removed: list[tuple[str, str]] = field(default_factory=list)
    tools: list[Bullet] = field(default_factory=list)
    compilers: list[str] | None = None


def bullets(lines: list[str], first: int) -> tuple[list[Bullet], list[Finding]]:
    """The bullets of lines, the first of which is the file's line first, and each line that
    belongs to none."""
    found: list[Bullet] = []
    findings: list[Finding] = []
    open_bullet = False
    for number, line in enumerate(lines, first):
        if line.startswith('* '):
            found.append(Bullet(number, line[2:].strip()))
            open_bullet = True
        elif not line.strip():
            open_bullet = False
        elif open_bullet:
            found[-1].text += ' ' + line.strip()
        else:
            findings.append((number, f'not a line of a changelog, which starts with "* ": {line}'))
    return found, findings


def line_findings(bullet: Bullet) -> list[Finding]:
    """What breaks a line's form."""
    findings: list[Finding] = []
    if not bullet.text.startswith(BREAKING) and bullet.text.split(' ', 1)[0] not in VERBS:
        findings.append((bullet.line, f'a line starts with {BREAKING.strip()} or one of '
                                      f'{", ".join(VERBS)}: {bullet.text}'))
    if '<<' in bullet.text:
        findings.append((bullet.line, 'a line holds no cross-reference, <<...>>: the index page '
                                      'shows it too'))
    return findings


def read_region(lines: list[str], index: int) -> tuple[str | None, int, int, list[Finding]]:
    """The region that starts at lines[index], after blank lines: its tag, the index of its
    first line and of its end line, and what is wrong with it."""
    while index < len(lines) and not lines[index].strip():
        index += 1
    start = REGION_START.fullmatch(lines[index]) if index < len(lines) else None
    if start is None:
        return None, index, index, [(index + 1, 'a section holds its lines between '
                                                '// tag::<name>[] and // end::<name>[]')]
    tag = start.group(1)
    for end in range(index + 1, len(lines)):
        if REGION_END.fullmatch(lines[end]) or REGION_START.fullmatch(lines[end]):
            if lines[end] == f'// end::{tag}[]':
                return tag, index + 1, end, []
            break
    return tag, index + 1, index + 1, [(index + 1, f'the region {tag} has no // end::{tag}[]')]


def expected(library: str, title: str) -> tuple[str, str] | None:
    """The anchor and the tag of a library's section titled title, or None for no section's
    title."""
    if title == 'Unreleased':
        return f'{library}-changelog-unreleased', 'unreleased'
    released = RELEASED.fullmatch(title)
    if released is None:
        return None
    return f'{library}-changelog-{released.group(1).replace(".", "-")}', released.group(1)


def parse_changelog(text: str, library: str) -> tuple[list[Region], list[Finding]]:
    """The sections of a library's changelog, and what breaks its form."""
    lines = text.split('\n')
    head = ['[appendix]', f'[#{library}-changelog]', '== Changelog']
    start = next((index for index in range(len(lines)) if lines[index:index + 3] == head), None)
    if start is None:
        return [], [(1, 'a changelog opens, after its notice, with ' + ', '.join(head))]
    found: list[Region] = []
    findings: list[Finding] = []
    index = start + 3
    while index < len(lines):
        anchor = ANCHOR.fullmatch(lines[index])
        if anchor is None or index + 1 == len(lines) or not lines[index + 1].startswith('=== '):
            if lines[index].startswith('='):
                findings.append((index + 1, 'a section of a changelog is [#<anchor>] and '
                                            f'=== <title>: {lines[index]}'))
            index += 1
            continue
        title = lines[index + 1][4:]
        want = expected(library, title)
        if want is None:
            findings.append((index + 2, f'a section is Unreleased or webcpp <a.b.c>: {title}'))
        elif anchor.group(1) != want[0]:
            findings.append((index + 1, f'the section {title} is [#{want[0]}]'))
        tag, first, end, faults = read_region(lines, index + 2)
        findings += faults
        if tag is None:
            index += 2
            continue
        if want is not None and tag != want[1]:
            findings.append((first, f'the region of {title} is {want[1]}'))
        body, body_faults = bullets(lines[first:end], first + 1)
        findings += body_faults
        found.append(Region(title, tag, index + 2, body))
        index = end + 1
    titles = [region.title for region in found]
    if not titles or titles[0] != 'Unreleased' or titles.count('Unreleased') != 1:
        findings.append((start + 1, 'a changelog has one section "Unreleased", first'))
    versions = [tuple(int(number) for number in region.tag.split('.')) for region in found
                if RELEASED.fullmatch(region.title) and RELEASED.fullmatch(f'webcpp {region.tag}')]
    if versions != sorted(set(versions), reverse=True):
        findings.append((start + 1, 'the versions of a changelog come newest first, each once'))
    return found, findings


def changelog_findings(text: str, library: str) -> list[Finding]:
    """What breaks the form of a library's changelog and of each of its lines, by line."""
    regions, findings = parse_changelog(text, library)
    for region in regions:
        for bullet in region.bullets:
            findings += line_findings(bullet)
        if len(region.bullets) > MOST:
            findings.append((region.line, f'{region.title} holds {len(region.bullets)} lines, '
                                          f'and a library holds at most {MOST} per version'))
    return sorted(findings)


def parse_notes(text: str, version: str) -> tuple[Notes, list[Finding]]:
    """A version's release notes, and what breaks their form."""
    anchor = version.replace('.', '-')
    lines = text.split('\n')
    notes = Notes()
    title = next((index for index, line in enumerate(lines) if line.startswith('= ')), None)
    if title is None or title == 0 or \
            lines[title - 1:title + 1] != [f'[#webcpp-{anchor}]', f'= webcpp {version}']:
        return notes, [(1 if title is None else title + 1,
                        f'release notes open with [#webcpp-{anchor}] and = webcpp {version}')]
    heads: list[tuple[int, str, str]] = []
    for index in range(title + 1, len(lines) - 1):
        match = ANCHOR.fullmatch(lines[index])
        if match is not None and lines[index + 1].startswith('== '):
            heads.append((index, match.group(1), lines[index + 1][3:]))
    want = [(f'webcpp-{anchor}-{slug}', name) for slug, name in SECTIONS]
    if [(found, name) for _, found, name in heads] != want:
        return notes, [(title + 1, 'release notes hold five sections, in order: '
                                   + ', '.join(f'[#{found}] == {name}' for found, name in want))]
    findings: list[Finding] = []
    preamble = [line for line in lines[title + 1:heads[0][0]] if line.strip()]
    notes.preparing = preamble == IN_PREPARATION
    if preamble and not notes.preparing:
        findings.append((title + 2, 'between the title and New Libraries, release notes hold the '
                                    'paragraph "In preparation: ..." or nothing'))
    bodies: dict[str, tuple[int, list[str]]] = {}
    for position, (index, found, _) in enumerate(heads):
        end = heads[position + 1][0] if position + 1 < len(heads) else len(lines)
        bodies[found.removeprefix(f'webcpp-{anchor}-')] = (index + 3, lines[index + 2:end])

    def is_none(slug: str) -> bool:
        return [line for line in bodies[slug][1] if line.strip()] == [NONE]

    def listed(slug: str, pattern: re.Pattern[str]) -> list[tuple[str, str]]:
        if is_none(slug):
            return []
        items, faults = bullets(bodies[slug][1], bodies[slug][0])
        findings.extend(faults)
        entries = []
        for item in items:
            match = pattern.fullmatch(item.text)
            if match is None:
                findings.append((item.line, f'not a line of {slug}: {item.text}'))
            else:
                entries.append((match.group('name'), match.group('what')))
        return entries

    notes.new = listed('new-libraries', NEW)
    notes.removed = listed('removed-libraries', REMOVED)
    first, body = bodies['updated-libraries']
    block = '\n'.join(body).strip()
    if block != NONE:
        position = 0
        for match in UPDATED.finditer(block):
            if block[position:match.start()].strip() or match.group('anchor') != anchor:
                break
            notes.updated.append((match.group('name'), match.group('tag')))
            position = match.end()
        if block[position:].strip() or not notes.updated:
            findings.append((first, f'Updated Libraries holds, for each library, '
                                    f'[#webcpp-{anchor}-<name>], === link:{{library-pages}}'
                                    '<name>/{library-page}[<name>] and the include of its '
                                    'changelog\'s region, or None.'))
    if not is_none('tools-and-build'):
        notes.tools, faults = bullets(bodies['tools-and-build'][1], bodies['tools-and-build'][0])
        findings += faults
    if not is_none('compilers-tested'):
        lanes, faults = bullets(bodies['compilers-tested'][1], bodies['compilers-tested'][0])
        findings += faults
        notes.compilers = [lane.text for lane in lanes]
    for slug, names in (('new-libraries', [name for name, _ in notes.new]),
                        ('updated-libraries', [name for name, _ in notes.updated]),
                        ('removed-libraries', [name for name, _ in notes.removed])):
        if names != sorted(names):
            findings.append((bodies[slug][0], f'{slug} lists its libraries in alphabetical order'))
    return notes, findings


def notes_findings(text: str, version: str) -> list[Finding]:
    """What breaks the form of a version's release notes and of each line of Tools and build,
    by line."""
    notes, findings = parse_notes(text, version)
    for bullet in notes.tools:
        findings += line_findings(bullet)
    return sorted(findings)


def file_findings(path: str, text: str) -> list[Finding]:
    """What breaks the form of a changelog file, by its path in the superproject: a library's
    changelog, libs/<name>/doc/changelog.adoc, or a version's release notes,
    doc/releases/<a.b.c>.adoc; nothing for any other file."""
    parts = PurePosixPath(path).parts
    if len(parts) == 4 and parts[0] == 'libs' and parts[2:] == ('doc', 'changelog.adoc'):
        return changelog_findings(text, parts[1])
    if len(parts) == 3 and parts[:2] == ('doc', 'releases'):
        release = re.fullmatch(r'(\d+\.\d+\.\d+)\.adoc', parts[2])
        if release is not None:
            return notes_findings(text, release.group(1))
    return []
```

- [ ] **Step 4: The lint rule.** `tools/lint/rules.py`: after its imports,

```python
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'release'))
import changelog  # noqa: E402  (tools/release, put on the path above)
```

  the rule, before `PER_FILE_RULES`,

```python
def changelog_lines(path: str, text: str) -> Iterator[Finding]:
    """A library's changelog and a version's release notes keep AGENTS.md chapter 11's form, each
    line of them a bullet in it (tools/release/changelog.py)."""
    for line, message in changelog.file_findings(path, text):
        yield path, line, message
```

  `'changelog': changelog_lines,` in `PER_FILE_RULES`, and in the docstring's
  list: "`changelog`  a library's changelog and a version's release notes
  keep AGENTS.md chapter 11's form, every line a bullet in it;".
  `tools/lint/lint.sh`, after rule 16:

```bash
# 17. A library's changelog and a version's release notes keep the form of AGENTS.md chapter
#     11: their sections and regions, and every line a bullet that starts with *Breaking
#     change:* or a verb of the list, with no cross-reference, at most ten per library and
#     version (tools/release/changelog.py, through tools/lint/rules.py).
rule 'changelog'
if python3 tools/lint/rules.py changelog < "${work_directory}/files"; then
    printf 'every changelog keeps its form\n'
else
    fail 'a changelog breaks its form (AGENTS.md chapter 11)'
fi
```

  `pyrightconfig.json`: `"tools/release"` in `extraPaths`, in its order.
- [ ] **Step 5: Run them and watch them pass.** `python3
  tools/release/changelog_test.py` and `python3 tools/lint/lint_test.py
  test_changelog`, then every `tools/**/*_test.py`, `pyright` (0), and the
  lint on `W` itself (`lint: clean`: the real changelogs and the notes of
  0.1.0 pass the rule).
- [ ] **Step 6: The line and the commit.** `doc/releases/0.1.0.adoc`, Tools
  and build: "Added the lint rule `changelog`: every changelog line keeps its
  form." Commit in `W`: `git add tools/release/changelog.py
  tools/release/changelog_test.py tools/lint/rules.py tools/lint/lint.sh
  tools/lint/lint_test.py pyrightconfig.json doc/releases/0.1.0.adoc`,
  "tools/release: the changelog's form, which the lint checks on every run".

### Task 6: The version, read by the Jamroot and checked

**Files:**
- Create: `tools/release/release.py`, `tools/release/release_test.py`,
  `tools/release/scratch.py`
- Modify: `Jamroot` (`version-line` becomes `define-line`, and its call in
  `boost-version`; `webcpp-version`; `release-tool`; the target
  `check-release`; the last `explicit`)

**Interfaces:**
- Consumes: `version.hpp` (Task 1); `changelog.py` (Task 5), which
  `scratch.py` uses.
- Produces: `release.Version` (`of_number`, `parse`, `name`, `lib`,
  `anchor`, `tag`, ordered), `read_version`, `version_findings`, `check`
  (the version alone here; Task 7 makes it whole), `COMMANDS`, `main`,
  `Failure`, `Broken`; the Jamroot's `define-line`, `webcpp-version`,
  `release-tool` and `check-release`; `scratch.py`'s `git`, `write`,
  `VERSION_HPP`, `changelog_text`, `notes_text`, `index_text`, `Tree`,
  `make_tree`; `release_test.py`'s `expect`, `git_checkout`, `mrdocs_root`,
  `CASES`, `B2_CASES`.

- [ ] **Step 1: The scratch trees.** `tools/release/scratch.py`:

```python
#!/usr/bin/env python3
# (the notice)

"""Scratch superprojects for the tests of tools/release: git repositories under a directory
whose path holds a space, with libs/config, whose version.hpp names the version, libraries whose
changelogs keep AGENTS.md chapter 11's form, the index's section Releases, the notes of the
version in preparation, and built pages, which git ignores. The files are no source of the
project, and carry no notice."""

from __future__ import annotations

import subprocess
from dataclasses import dataclass
from pathlib import Path

import changelog

GIT = ['git', '-c', 'user.name=webcpp release test', '-c', 'user.email=release@webcpp.invalid',
       '-c', 'commit.gpgsign=false', '-c', 'tag.gpgsign=false', '-c', 'protocol.file.allow=always',
       '-c', 'advice.addEmbeddedRepo=false', '-c', 'init.defaultBranch=main']

VERSION_HPP = ('#ifndef WEBCPP_VERSION_HPP\n#define WEBCPP_VERSION_HPP\n'
               '#define WEBCPP_VERSION {number}\n#define WEBCPP_LIB_VERSION "{lib}"\n#endif\n')


def git(directory: Path, *arguments: str) -> str:
    """Runs git in directory as the test's author, and returns what it printed."""
    completed = subprocess.run([*GIT, '-C', str(directory), *arguments], capture_output=True,
                               text=True, check=False)
    assert completed.returncode == 0, (arguments, completed.stderr)
    return completed.stdout


def write(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding='utf-8')


def changelog_text(name: str, unreleased: tuple[str, ...] = (),
                   released: tuple[tuple[str, tuple[str, ...]], ...] = ()) -> str:
    """A library's changelog: its Unreleased lines, then each released version's, newest first,
    each line of the file as written."""
    sections = [(f'{name}-changelog-unreleased', 'Unreleased', 'unreleased', unreleased)]
    sections += [(f'{name}-changelog-{version.replace(".", "-")}', f'webcpp {version}', version,
                  lines) for version, lines in released]
    text = f'[appendix]\n[#{name}-changelog]\n== Changelog\n\nWhat changed in {name}.\n'
    for anchor, title, tag, lines in sections:
        body = ''.join(f'{line}\n' for line in lines)
        text += f'\n[#{anchor}]\n=== {title}\n\n// tag::{tag}[]\n{body}// end::{tag}[]\n'
    return text


def notes_text(version: str, *, preparing: bool = True, new: tuple[str, ...] = (),
               updated: tuple[tuple[str, str], ...] = (), removed: tuple[str, ...] = (),
               tools: tuple[str, ...] = (), compilers: tuple[str, ...] | None = None) -> str:
    """A version's release notes: each new and removed library named, each updated one with the
    tag its include names, the lines of Tools and build as written, and the lanes."""
    anchor = version.replace('.', '-')

    def section(slug: str, title: str, body: str) -> str:
        return f'\n[#webcpp-{anchor}-{slug}]\n== {title}\n\n{body or changelog.NONE}\n'

    text = f'[#webcpp-{anchor}]\n= webcpp {version}\n'
    if preparing:
        text += '\n' + '\n'.join(changelog.IN_PREPARATION) + '\n'
    text += section('new-libraries', 'New Libraries', '\n'.join(
        f'* link:{{library-pages}}{name}/{{library-page}}[{name}]: {name}, a library, from '
        'R.' for name in new))
    text += section('updated-libraries', 'Updated Libraries', '\n\n'.join(
        f'[#webcpp-{anchor}-{name}]\n=== link:{{library-pages}}{name}/{{library-page}}[{name}]'
        f'\n\ninclude::{{webcpp-root}}/libs/{name}/doc/changelog.adoc[tag={tag}]'
        for name, tag in updated))
    text += section('removed-libraries', 'Removed Libraries',
                    '\n'.join(f'* {name}: {name}, gone.' for name in removed))
    text += section('tools-and-build', 'Tools and build', '\n'.join(tools))
    text += section('compilers-tested', 'Compilers tested',
                    '' if compilers is None else '\n'.join(f'* {lane}' for lane in compilers))
    return text


def index_text(versions: tuple[str, ...]) -> str:
    """The index page, its section Releases including each version's notes, as given."""
    included = '\n\n'.join(f'include::releases/{version}.adoc[leveloffset=+2]'
                           for version in versions)
    return f'= webcpp\n\n[#releases]\n== Releases\n\nEach version.\n\n{included}\n'


@dataclass
class Tree:
    """A scratch superproject: a git checkout whose libraries are gitlinks. Its commits allow
    an unchanged tree, so that a test may write a file again as it was."""

    root: Path

    def library(self, name: str) -> Path:
        return self.root / 'libs' / name

    def add_library(self, name: str) -> None:
        """Adds library name, with what a user sees and what they do not, and a built page git
        ignores, committed in its own repository and recorded by the superproject."""
        directory = self.library(name)
        write(directory / 'build.jam', f'project /webcpp/{name} ;\n')
        write(directory / f'include/webcpp/{name}.hpp', f'// {name}\n')
        write(directory / f'test/{name}_test.cpp', 'int main() {}\n')
        write(directory / 'AGENTS.md', f'# {name}\n')
        write(directory / 'LICENSE_1_0.txt', 'Boost Software License\n')
        write(directory / 'doc/changelog.adoc', changelog_text(name))
        write(directory / '.gitattributes', '* text=auto eol=lf\n')
        write(directory / '.gitignore', 'doc/html/\n')
        write(directory / 'doc/html/index.html', f'<p>{name}</p>\n')
        git(directory, 'init', '-q')
        git(directory, 'add', '.')
        git(directory, 'commit', '-q', '-m', f'{name}: its first commit')
        self.record(name)

    def record(self, name: str) -> None:
        """Records library name's checked-out commit in the superproject."""
        git(self.root, 'add', f'libs/{name}')
        git(self.root, 'commit', '-q', '--allow-empty', '-m', f'libs/{name}: recorded')

    def change(self, name: str, path: str, text: str) -> None:
        """Commits text at path of library name, and records the commit."""
        write(self.library(name) / path, text)
        git(self.library(name), 'add', path)
        git(self.library(name), 'commit', '-q', '--allow-empty', '-m', f'{name}: {path}')
        self.record(name)

    def commit_file(self, path: str, text: str) -> None:
        """Commits text at path of the superproject."""
        write(self.root / path, text)
        git(self.root, 'add', path)
        git(self.root, 'commit', '-q', '--allow-empty', '-m', path)


def make_tree(directory: Path) -> Tree:
    """A superproject preparing 0.1.0, its first version, with config, alpha and beta, which
    every check passes, and its dot files, a script, and pages built for every library."""
    root = directory / 'super project'
    root.mkdir()
    git(root, 'init', '-q')
    write(root / '.gitignore', 'bin/\ndoc/html/\n')
    write(root / '.clang-format', 'BasedOnStyle: Google\n')
    write(root / '.github/workflows/ci.yml', 'name: CI\n')
    write(root / 'README.md', '# webcpp\n')
    write(root / 'doc/index.adoc', index_text(('0.1.0',)))
    write(root / 'doc/releases/0.1.0.adoc', notes_text('0.1.0', new=('alpha', 'beta', 'config')))
    write(root / 'tools/run.sh', '#!/bin/sh\n')
    (root / 'tools/run.sh').chmod(0o755)
    write(root / 'doc/html/index.html', '<p>webcpp</p>\n')
    git(root, 'add', '.')
    git(root, 'commit', '-q', '-m', 'The superproject')
    tree = Tree(root)
    for name in ('alpha', 'beta', 'config'):
        tree.add_library(name)
    tree.change('config', 'include/webcpp/version.hpp', VERSION_HPP.format(number=100, lib='0_1'))
    return tree
```

- [ ] **Step 2: Write the failing tests.** `tools/release/release_test.py`:

```python
#!/usr/bin/env python3
# (the notice)

"""Checks tools/release/release.py on scratch git superprojects (tools/release/scratch.py), and
the Jamroot's targets on scratch copies of this superproject (tools/test/harness.py). Run with
the names of some cases to run only those."""

from __future__ import annotations

import os
import shutil
import sys
import tempfile
from collections.abc import Callable
from pathlib import Path

import release
import scratch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'test'))
import harness  # noqa: E402  (tools/test, put on the path above)

V010 = release.Version.parse('0.1.0')
V020 = release.Version.parse('0.2.0')


def expect(found: list[str], *texts: str) -> None:
    """Asserts that some finding holds each of texts."""
    for text in texts:
        assert any(text in finding for finding in found), (text, found)


def test_versions_are_named(directory: Path) -> None:
    assert release.Version.of_number(100) == V010
    assert (V010.lib, V010.anchor, V010.tag) == ('0_1', '0-1-0', 'webcpp-0.1.0')
    other = release.Version.of_number(100203)
    assert (other.name, other.lib) == ('1.2.3', '1_2_3')
    assert release.Version.parse('0.10.0') > release.Version.parse('0.9.0')


def test_version_must_agree(directory: Path) -> None:
    tree = scratch.make_tree(directory)
    assert release.version_findings(tree.root, V010) == []
    tree.change('config', 'include/webcpp/version.hpp',
                scratch.VERSION_HPP.format(number=101, lib='0_1'))
    expect(release.version_findings(tree.root, release.Version.parse('0.1.1')),
           'WEBCPP_VERSION names 0.1.1, and WEBCPP_LIB_VERSION "0_1" is not "0_1_1"')
    expect(release.version_findings(tree.root, V010),
           'the build read the version 0.1.0, and libs/config/include/webcpp/version.hpp names '
           '0.1.1')


def mrdocs_root() -> Path:
    """The MrDocs of this checkout, as tools/test/doc_test.py links it: $MRDOCS_ROOT, else
    .local/mrdocs."""
    configured = os.environ.get('MRDOCS_ROOT')
    return Path(configured) if configured else harness.ROOT / '.local/mrdocs'


def git_checkout(root: Path) -> None:
    """Makes the harness's scratch superproject root a git checkout whose libraries, config and
    demo, are gitlinks, config holding the version.hpp of 0.1.0, with an index and notes of its
    own that name config and demo: the copy's are this superproject's, which name others."""
    scratch.write(root / 'libs/config/build.jam', 'project /webcpp/config ;\n\nalias config ;\n')
    scratch.write(root / 'libs/config/include/webcpp/version.hpp',
                  scratch.VERSION_HPP.format(number=100, lib='0_1'))
    for name in ('config', 'demo'):
        scratch.write(root / 'libs' / name / 'doc/changelog.adoc', scratch.changelog_text(name))
        scratch.git(root / 'libs' / name, 'init', '-q')
        scratch.git(root / 'libs' / name, 'add', '.')
        scratch.git(root / 'libs' / name, 'commit', '-q', '-m', name)
    scratch.write(root / 'doc/index.adoc', scratch.index_text(('0.1.0',)))
    scratch.write(root / 'doc/releases/0.1.0.adoc',
                  scratch.notes_text('0.1.0', new=('config', 'demo')))
    scratch.git(root, 'init', '-q')
    scratch.git(root, 'add', '.')
    scratch.git(root, 'commit', '-q', '-m', 'The scratch superproject')


def test_jamroot_targets(root: Path) -> None:
    git_checkout(root)
    harness.expect(harness.run_b2(root, '-d0', 'check-release'), True)


def test_jamroot_without_config(root: Path) -> None:
    # The copy holds no libs/config: everything but the release targets builds, the doc included,
    # and those stop, naming the header.
    (root / '.local/mrdocs').symlink_to(mrdocs_root())
    harness.expect(harness.run_b2(root, '-d0', 'declared-targets'), True)
    harness.expect(harness.run_b2(root, 'libs/demo/test'), True)
    harness.expect(harness.run_b2(root, '-d0', 'doc'), True)
    harness.expect(harness.run_b2(root, '-d0', 'check-release'), False,
                   'libs/config/include/webcpp/version.hpp')


CASES: list[Callable[[Path], None]] = [
    test_versions_are_named,
    test_version_must_agree,
]

B2_CASES: list[Callable[[Path], None]] = [
    test_jamroot_targets,
    test_jamroot_without_config,
]


def main(argv: list[str]) -> int:
    unknown = set(argv) - {case.__name__ for case in (*CASES, *B2_CASES)}
    if unknown:
        print(f'release_test: no case named {", ".join(sorted(unknown))}', file=sys.stderr)
        return 2
    for case in CASES:
        if argv and case.__name__ not in argv:
            continue
        with tempfile.TemporaryDirectory(prefix='webcpp release ') as directory:
            case(Path(directory))
        print(f'{case.__name__}: ok')
    for case in B2_CASES:
        if argv and case.__name__ not in argv:
            continue
        root = harness.scratch_superproject('demo')
        try:
            case(root)
        finally:
            shutil.rmtree(root)
        print(f'{case.__name__}: ok')
    print('release_test: ok')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
```

  (If `tools/test/doc_test.py`'s own `mrdocs_root` reads another variable
  than `MRDOCS_ROOT`, this one follows it.)
- [ ] **Step 3: Run them and watch them fail.** `python3
  tools/release/release_test.py`: `ModuleNotFoundError: No module named
  'release'`.
- [ ] **Step 4: `tools/release/release.py`:**

```python
#!/usr/bin/env python3
# (the notice)

"""webcpp's release tooling (AGENTS.md chapter 11), which the Jamroot's explicit targets run.

Usage: release.py check --root <superproject> --version <a.b.c>

check fails when the version disagrees anywhere: WEBCPP_VERSION and WEBCPP_LIB_VERSION of
libs/config/include/webcpp/version.hpp, the one place it is written, name different versions,
or the version the Jamroot read, --version, is not theirs. Exit 0 when nothing is wrong; 1 with
each fault on a line of its own, `<command>: <fault>`; 2 on a usage error, or when what it reads
is not there.
"""

from __future__ import annotations

import argparse
import re
import sys
from collections.abc import Callable
from dataclasses import dataclass
from pathlib import Path

HEADER = 'libs/config/include/webcpp/version.hpp'
DEFINE = re.compile(r'^#define (WEBCPP_VERSION|WEBCPP_LIB_VERSION) +(.+?)\s*$', re.MULTILINE)


class Failure(Exception):
    """Faults of the tree that stop a command, said to the user, one per line; exit 1."""


class Broken(Exception):
    """A usage error, or something the tool reads that is not there; exit 2."""


@dataclass(frozen=True, order=True)
class Version:
    """A version of webcpp, which orders as its numbers do."""

    major: int
    minor: int
    patch: int

    @staticmethod
    def of_number(number: int) -> Version:
        """The version WEBCPP_VERSION's number names: 100 is 0.1.0."""
        return Version(number // 100000, number // 100 % 1000, number % 100)

    @staticmethod
    def parse(name: str) -> Version:
        """The version name writes, <major>.<minor>.<patch>."""
        match = re.fullmatch(r'(\d+)\.(\d+)\.(\d+)', name)
        if match is None:
            raise Broken(f'{name} is no version <major>.<minor>.<patch>')
        return Version(int(match.group(1)), int(match.group(2)), int(match.group(3)))

    @property
    def name(self) -> str:
        return f'{self.major}.{self.minor}.{self.patch}'

    @property
    def lib(self) -> str:
        """The version as WEBCPP_LIB_VERSION writes it: 0_1, 1_2_3."""
        return f'{self.major}_{self.minor}' + (f'_{self.patch}' if self.patch else '')

    @property
    def anchor(self) -> str:
        return self.name.replace('.', '-')

    @property
    def tag(self) -> str:
        return f'webcpp-{self.name}'


def read_version(root: Path) -> tuple[Version, str]:
    """The version WEBCPP_VERSION names, and WEBCPP_LIB_VERSION's text without its quotes."""
    header = root / HEADER
    if not header.is_file():
        raise Broken(f'{HEADER} does not exist: check out libs/config')
    defined = dict(DEFINE.findall(header.read_text(encoding='utf-8')))
    number = defined.get('WEBCPP_VERSION', '')
    lib = defined.get('WEBCPP_LIB_VERSION', '')
    if not number.isdigit() or len(lib) < 2 or lib[0] != '"' or lib[-1] != '"':
        raise Broken(f'{HEADER} has no lines `#define WEBCPP_VERSION <number>` and '
                     '`#define WEBCPP_LIB_VERSION "<text>"`')
    return Version.of_number(int(number)), lib[1:-1]


def version_findings(root: Path, version: Version) -> list[str]:
    """What disagrees about the version: version.hpp's two macros, and the Jamroot's reading."""
    written, lib = read_version(root)
    findings = []
    if lib != written.lib:
        findings.append(f'{HEADER}: WEBCPP_VERSION names {written.name}, and WEBCPP_LIB_VERSION '
                        f'"{lib}" is not "{written.lib}"')
    if version != written:
        findings.append(f'the build read the version {version.name}, and {HEADER} names '
                        f'{written.name}')
    return findings


def check(root: Path, version: Version) -> list[str]:
    """Every fault that keeps the tree from being tagged webcpp-<version>."""
    return version_findings(root, version)


# A command: given the superproject, the version and its arguments, it returns the faults that
# stop it, and prints what it writes.
Command = Callable[[Path, Version, argparse.Namespace], list[str]]

COMMANDS: dict[str, Command] = {
    'check': lambda root, version, _: check(root, version),
}


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(prog='release.py', description="webcpp's release tooling")
    commands = parser.add_subparsers(dest='command', required=True)
    for name in COMMANDS:
        command = commands.add_parser(name)
        command.add_argument('--root', type=Path, required=True)
        command.add_argument('--version', required=True)
        if name == 'dist':
            command.add_argument('--out', type=Path, required=True)
    arguments = parser.parse_args(argv)
    try:
        version = Version.parse(arguments.version)
        findings = COMMANDS[arguments.command](arguments.root.resolve(), version, arguments)
    except Failure as failure:
        findings = str(failure).split('\n')
    except Broken as broken:
        print(f'{arguments.command}: {broken}', file=sys.stderr)
        return 2
    for finding in findings:
        print(f'{arguments.command}: {finding}')
    return 1 if findings else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
```

- [ ] **Step 5: The Jamroot.** `version-line` becomes:

```
# The line of the file header that defines macro, `#define <macro> <value>`:
# Boost's BOOST_VERSION, config's WEBCPP_VERSION. b2 runs a SHELL command with
# cmd.exe on Windows, where findstr does what grep does elsewhere; no Windows
# lane runs the release targets, so WEBCPP_VERSION's findstr is read by none.
local rule define-line ( header : macro )
{
    if [ os.name ] = NT
    {
        local native = [ path.native [ path.make $(header) ] ] ;
        return [ SHELL "findstr /B /C:\"#define $(macro) \" \"$(native)\"" ] ;
    }
    return [ SHELL "grep \"^#define $(macro) \" \"$(header)\"" ] ;
}
```

  and `boost-version` calls `[ define-line $(header) : BOOST_VERSION ]`.
  Before the last `explicit`:

```
# webcpp's version, which libs/config/include/webcpp/version.hpp writes, the
# one place it is written: its WEBCPP_VERSION and its name, 100 0.1.0. Only
# the release targets read it, so that a checkout without libs/config (a
# submodule not checked out, the scratch copies of the tools' tests) builds
# everything else; they stop, naming the header, when it is not there or has
# no line `#define WEBCPP_VERSION <number>`.
rule webcpp-version ( )
{
    local header = [ path.native [ path.join [ path.make $(WEBCPP_ROOT) ]
        libs/config/include/webcpp/version.hpp ] ] ;
    if ! [ CHECK_IF_FILE $(header) ]
    {
        errors.user-error "webcpp's version is read from $(header), which does not"
            "exist: check out libs/config" ;
    }
    local number = [ MATCH "^#define WEBCPP_VERSION +([0-9]+)"
        : [ define-line $(header) : WEBCPP_VERSION ] ] ;
    if ! $(number)
    {
        errors.user-error "$(header) has no line `#define WEBCPP_VERSION <number>`" ;
    }
    # major, minor (three digits) and patch (two), each without its zeros.
    local parts = [ MATCH "^([0-9]*)([0-9][0-9][0-9])([0-9][0-9])$" : 00000$(number) ] ;
    local name ;
    for local part in $(parts)
    {
        name += [ MATCH "^0*([0-9]+)$" : $(part) ] ;
    }
    return $(number) $(name:J=.) ;
}

# The release tooling, tools/release/release.py (AGENTS.md chapter 11), run
# only when named: check-release, what a tag needs; whats-new, the skeleton of
# the version's "What's new"; dist, its archive, once every page is built,
# into bin/dist. None is named release-<word>, which b2 reads as the variant
# release (AGENTS.md chapter 12).
if [ os.name ] = NT { .python = python ; } else { .python = python3 ; }

rule release-tool ( targets * : sources * : properties * )
{
    local version = [ webcpp-version ] ;
    PYTHON on $(targets) = $(.python) ;
    SCRIPT on $(targets) = [ path.native [ path.join [ path.make $(WEBCPP_ROOT) ]
        tools/release/release.py ] ] ;
    ROOT on $(targets) = $(WEBCPP_ROOT) ;
    VERSION on $(targets) = $(version[2]) ;
}

rule check-release ( targets * : sources * : properties * )
{
    release-tool $(targets) : $(sources) : $(properties) ;
}

actions check-release
{
    "$(PYTHON)" "$(SCRIPT)" check --root "$(ROOT)" --version "$(VERSION)"
}

notfile check-release : @check-release : : <webcpp-boost-check>off ;
```

  and `check-release` joins the last `explicit`.
- [ ] **Step 6: Run them and watch them pass.** `python3
  tools/release/release_test.py`, every other `tools/**/*_test.py` (the
  Jamroot changed), `pyright` (0), the lint (`lint: clean`). Nothing of the
  tooling runs on `W` itself: no `check-release` there.
- [ ] **Step 7: Commit** in `W`: `git add Jamroot tools/release/release.py
  tools/release/release_test.py tools/release/scratch.py`, "tools/release:
  the version config writes, read by the Jamroot and checked". No changelog
  line yet: Task 9 writes the tooling's line once its three targets exist.

### Task 7: `check-release`, whole

**Files:**
- Modify: `tools/release/release.py`, `tools/release/release_test.py`

**Interfaces:**
- Consumes: `changelog.parse_changelog`, `changelog_findings`,
  `parse_notes`, `line_findings` (Task 5); `matrix.LANES` (`tools/ci`);
  `Version`, `check` (Task 6); `scratch` (Task 6).
- Produces: `git`, `libraries`, `gitlinks`, `previous_tag`, `has_commit`,
  `changed`, `located`, `lines_of`, `index_findings`, `state_findings`,
  `library_findings`, `USER_VISIBLE`, and `check` whole (Task 8 adds one
  line to it); `release_test.py`'s `released` and `frozen`.

- [ ] **Step 1: Write the failing tests,** added to `release_test.py` (with
  `import matrix` after `import release`, which puts `tools/ci` on the path)
  and to `CASES`:

```python
def released(tree: scratch.Tree) -> None:
    """Tags 0.1.0, then opens 0.2.0 as the release procedure does: config's version moves, and
    the notes of 0.2.0 come first in the index."""
    scratch.git(tree.root, 'tag', 'webcpp-0.1.0')
    tree.change('config', 'include/webcpp/version.hpp',
                scratch.VERSION_HPP.format(number=200, lib='0_2'))
    tree.commit_file('doc/releases/0.2.0.adoc', scratch.notes_text('0.2.0'))
    tree.commit_file('doc/index.adoc', scratch.index_text(('0.2.0', '0.1.0')))


def test_first_release_lists_every_library_as_new(directory: Path) -> None:
    tree = scratch.make_tree(directory)
    assert release.check(tree.root, V010) == []
    tree.commit_file('doc/releases/0.1.0.adoc',
                     scratch.notes_text('0.1.0', new=('alpha', 'config')))
    expect(release.check(tree.root, V010),
           'New Libraries lists alpha, config, and the libraries new since the first version are '
           'alpha, beta, config')


def test_changed_library_needs_a_line_and_a_listing(directory: Path) -> None:
    tree = scratch.make_tree(directory)
    released(tree)
    assert release.check(tree.root, V020) == []
    tree.change('alpha', 'include/webcpp/alpha.hpp', '// alpha, changed\n')
    expect(release.check(tree.root, V020),
           'libs/alpha changed since webcpp-0.1.0 and its Unreleased has no line')
    tree.change('alpha', 'doc/changelog.adoc',
                scratch.changelog_text('alpha', unreleased=('* Changed alpha.',)))
    expect(release.check(tree.root, V020),
           'the release notes miss alpha, whose changelog has lines for this version')
    tree.commit_file('doc/releases/0.2.0.adoc',
                     scratch.notes_text('0.2.0', updated=(('alpha', 'unreleased'),)))
    assert release.check(tree.root, V020) == []


def test_a_change_no_user_sees_needs_no_line(directory: Path) -> None:
    tree = scratch.make_tree(directory)
    released(tree)
    tree.change('alpha', 'test/alpha_test.cpp', 'int main() { return 0; }\n')
    tree.change('alpha', 'AGENTS.md', '# alpha, for agents\n')
    assert release.check(tree.root, V020) == []
    tree.change('alpha', 'LICENSE_1_0.txt', 'Boost Software License, again\n')
    expect(release.check(tree.root, V020), 'libs/alpha changed since webcpp-0.1.0')


def test_a_listed_library_without_a_line_fails(directory: Path) -> None:
    tree = scratch.make_tree(directory)
    released(tree)
    tree.commit_file('doc/releases/0.2.0.adoc',
                     scratch.notes_text('0.2.0', updated=(('beta', 'unreleased'),)))
    expect(release.check(tree.root, V020),
           'beta is under Updated Libraries, and its region unreleased holds no line')


def test_added_and_removed_libraries(directory: Path) -> None:
    tree = scratch.make_tree(directory)
    released(tree)
    tree.add_library('gamma')
    expect(release.check(tree.root, V020),
           'New Libraries lists none, and the libraries new since webcpp-0.1.0 are gamma')
    scratch.git(tree.root, 'rm', '-q', '--cached', 'libs/beta')
    shutil.rmtree(tree.library('beta'))
    scratch.git(tree.root, 'commit', '-q', '-m', 'libs/beta: removed')
    tree.commit_file('doc/releases/0.2.0.adoc',
                     scratch.notes_text('0.2.0', new=('gamma',), removed=('beta',)))
    assert release.check(tree.root, V020) == []
    tree.commit_file('doc/releases/0.2.0.adoc', scratch.notes_text(
        '0.2.0', new=('gamma',), updated=(('beta', 'unreleased'),), removed=('beta',)))
    expect(release.check(tree.root, V020),
           'beta is under Updated Libraries, and is removed since webcpp-0.1.0')


def test_a_missing_commit_and_a_stale_pointer(directory: Path) -> None:
    tree = scratch.make_tree(directory)
    released(tree)
    alpha = tree.library('alpha')
    scratch.git(alpha, 'checkout', '-q', '--orphan', 'rewritten')
    scratch.git(alpha, 'commit', '-q', '-m', 'alpha: rewritten')
    scratch.git(alpha, 'branch', '-q', '-D', 'main')
    scratch.git(alpha, 'reflog', 'expire', '--expire=now', '--all')
    scratch.git(alpha, 'gc', '-q', '--prune=now')
    tree.record('alpha')
    expect(release.check(tree.root, V020), 'is not in its repository; fetch its tags')
    scratch.git(tree.library('beta'), 'commit', '-q', '--allow-empty', '-m', 'beta: unrecorded')
    expect(release.check(tree.root, V020), 'libs/beta is at ', ', and the superproject records ')


def test_lines_out_of_form_are_named(directory: Path) -> None:
    tree = scratch.make_tree(directory)
    tree.change('alpha', 'doc/changelog.adoc',
                scratch.changelog_text('alpha', unreleased=('* Made a mistake.',)))
    tree.commit_file('doc/releases/0.1.0.adoc', scratch.notes_text(
        '0.1.0', new=('alpha', 'beta', 'config'), tools=('* Built a thing.',)))
    expect(release.check(tree.root, V010), 'libs/alpha/doc/changelog.adoc:',
           'doc/releases/0.1.0.adoc:')


def test_compilers_tested_are_the_ci_lanes(directory: Path) -> None:
    lanes = tuple(lane.name for lane in matrix.LANES)
    tree = scratch.make_tree(directory)
    tree.commit_file('doc/releases/0.1.0.adoc', scratch.notes_text(
        '0.1.0', new=('alpha', 'beta', 'config'), compilers=lanes))
    assert release.check(tree.root, V010) == []
    tree.commit_file('doc/releases/0.1.0.adoc', scratch.notes_text(
        '0.1.0', new=('alpha', 'beta', 'config'), compilers=lanes[1:]))
    expect(release.check(tree.root, V010), 'Compilers tested lists')


def frozen(tree: scratch.Tree, lanes: tuple[str, ...]) -> None:
    """Freezes 0.2.0 as the release procedure does: alpha's Unreleased becomes webcpp 0.2.0 under
    a new, empty Unreleased, and the notes include tag=0.2.0, without their paragraph."""
    tree.change('alpha', 'doc/changelog.adoc', scratch.changelog_text(
        'alpha', released=(('0.2.0', ('* Changed alpha.',)),)))
    tree.commit_file('doc/releases/0.2.0.adoc', scratch.notes_text(
        '0.2.0', preparing=False, updated=(('alpha', '0.2.0'),), compilers=lanes))


def test_frozen_state(directory: Path) -> None:
    lanes = tuple(lane.name for lane in matrix.LANES)
    tree = scratch.make_tree(directory)
    released(tree)
    tree.change('alpha', 'include/webcpp/alpha.hpp', '// alpha, changed\n')
    frozen(tree, lanes)
    assert release.check(tree.root, V020) == []
    tree.commit_file('doc/releases/0.2.0.adoc', scratch.notes_text(
        '0.2.0', updated=(('alpha', '0.2.0'),), compilers=lanes))
    expect(release.check(tree.root, V020),
           'alpha is included with tag=0.2.0, and the version is in preparation: '
           'tag=unreleased', 'libs/alpha/doc/changelog.adoc has webcpp 0.2.0')
    frozen(tree, lanes)
    tree.change('alpha', 'doc/changelog.adoc', scratch.changelog_text(
        'alpha', unreleased=('* Fixed alpha.',), released=(('0.2.0', ('* Changed alpha.',)),)))
    expect(release.check(tree.root, V020),
           'libs/alpha/doc/changelog.adoc: Unreleased holds lines, and webcpp 0.2.0 is frozen')
    frozen(tree, lanes)
    scratch.git(tree.root, 'tag', 'webcpp-0.2.0', 'HEAD~1')
    expect(release.check(tree.root, V020), 'the tag webcpp-0.2.0 is at ')
    scratch.git(tree.root, 'tag', '-f', 'webcpp-0.2.0', 'HEAD')
    assert release.check(tree.root, V020) == []


def test_index_includes_every_version_newest_first(directory: Path) -> None:
    tree = scratch.make_tree(directory)
    released(tree)
    tree.commit_file('doc/index.adoc', scratch.index_text(('0.1.0', '0.2.0')))
    expect(release.check(tree.root, V020),
           'doc/index.adoc: section Releases includes 0.1.0, 0.2.0, and it includes the notes of '
           'doc/releases newest first: 0.2.0, 0.1.0')
    tree.commit_file('doc/index.adoc', scratch.index_text(('0.1.0',)))
    expect(release.check(tree.root, V020), 'section Releases includes 0.1.0, and')


def test_template_opens_a_version_that_checks(directory: Path) -> None:
    tree = scratch.make_tree(directory)
    released(tree)
    template = (Path(release.__file__).parent / 'template.adoc').read_text(encoding='utf-8')
    tree.commit_file('doc/releases/0.2.0.adoc',
                     template.replace('@VERSION@', '0.2.0').replace('@ANCHOR@', '0-2-0'))
    assert release.check(tree.root, V020) == []
```

- [ ] **Step 2: Run them and watch them fail.** `python3
  tools/release/release_test.py`. Expected: every new case but
  `test_template_opens_a_version_that_checks` fails on its first
  `expect` (the check reads the version alone); that one passes already
  and stays.
- [ ] **Step 3: Implement.** In `release.py`: the docstring gains the rules
  below, as their findings say them; the imports gain `subprocess` and `from
  pathlib import PurePosixPath`, and after them:

```python
TOOLS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS / 'ci'))
import changelog  # noqa: E402  (beside this file)
import matrix  # noqa: E402  (tools/ci, put on the path above: its LANES are the CI's lanes)

# What a user of a library sees: a change under these paths of its repository needs a line in
# its changelog; its tests, its CI and its AGENTS.md need none. version.hpp, config's alone,
# moves at every release by the procedure, and is no change of config's.
USER_VISIBLE = ('include', 'build.jam', 'wit', 'meta/libraries.json', 'README.md', 'example',
                'doc', 'LICENSE*', ':(exclude)doc/changelog.adoc',
                ':(exclude)include/webcpp/version.hpp')

TAG = re.compile(r'webcpp-(\d+\.\d+\.\d+)')
RELEASES = re.compile(r'^\[#releases\]\n== Releases\n', re.MULTILINE)
INCLUDED = re.compile(r'^include::releases/(\d+\.\d+\.\d+)\.adoc\[leveloffset=\+2\]$',
                      re.MULTILINE)
NOTES_FILE = re.compile(r'(\d+\.\d+\.\d+)\.adoc')


def git(directory: Path, *arguments: str) -> str:
    """What git prints in directory; a git that fails is Broken, naming the command."""
    completed = subprocess.run(['git', '-C', str(directory), *arguments], capture_output=True,
                               text=True, check=False)
    if completed.returncode != 0:
        raise Broken(f'git {" ".join(arguments)} in {directory}: {completed.stderr.strip()}')
    return completed.stdout


def libraries(root: Path) -> list[str]:
    """The libraries the build registers: each directory of libs/ with a build.jam."""
    return sorted(build.parent.name for build in (root / 'libs').glob('*/build.jam'))


def gitlinks(root: Path, revision: str) -> dict[str, str]:
    """The commit the superproject records at revision for each library, libs/<name>."""
    found = {}
    for entry in git(root, 'ls-tree', '-z', revision, 'libs/').split('\0'):
        if not entry:
            continue
        meta, path = entry.split('\t', 1)
        _, kind, commit = meta.split()
        if kind == 'commit':
            found[PurePosixPath(path).name] = commit
    return found


def previous_tag(root: Path, version: Version) -> str | None:
    """The tag of the newest version before version that HEAD holds, or None for a first
    release."""
    earlier = []
    for tag in git(root, 'tag', '--merged', 'HEAD', '--list', 'webcpp-*').split():
        match = TAG.fullmatch(tag)
        if match is not None and Version.parse(match.group(1)) < version:
            earlier.append(Version.parse(match.group(1)))
    return max(earlier).tag if earlier else None


def has_commit(root: Path, library: str, commit: str) -> bool:
    """Whether library's repository holds commit."""
    completed = subprocess.run(['git', '-C', str(root / 'libs' / library), 'cat-file', '-e',
                                f'{commit}^{{commit}}'], capture_output=True, check=False)
    return completed.returncode == 0


def changed(root: Path, library: str, old: str, new: str) -> bool:
    """Whether a user sees a change in library between the commits the superproject records at
    the previous tag, old, and at HEAD, new."""
    completed = subprocess.run(['git', '-C', str(root / 'libs' / library), 'diff', '--quiet',
                                old, new, '--', *USER_VISIBLE], capture_output=True, text=True,
                               check=False)
    if completed.returncode not in (0, 1):
        raise Broken(f'git diff in libs/{library}: {completed.stderr.strip()}')
    return completed.returncode == 1


def located(path: str, faults: list[changelog.Finding]) -> list[str]:
    """Faults of the file path as findings: <path>:<line>: <message>."""
    return [f'{path}:{line}: {message}' for line, message in faults]


def lines_of(regions: list[changelog.Region], tag: str) -> list[changelog.Bullet]:
    """The lines of the region tag, or none when there is no such region."""
    return next((region.bullets for region in regions if region.tag == tag), [])


def index_findings(root: Path, version: Version) -> list[str]:
    """What the index's section Releases gets wrong: it includes every version's notes of
    doc/releases once, newest first, the version's first."""
    text = (root / 'doc/index.adoc').read_text(encoding='utf-8')
    section = RELEASES.search(text)
    if section is None:
        return ['doc/index.adoc has no section [#releases], == Releases']
    body = text[section.end():]
    following = re.search(r'^== ', body, re.MULTILINE)
    included = INCLUDED.findall(body[:following.start()] if following else body)
    names = [path.name for path in (root / 'doc/releases').iterdir()]
    findings = [f'doc/releases/{name} is no version\'s notes, <a.b.c>.adoc' for name in names
                if name.endswith('.adoc') and NOTES_FILE.fullmatch(name) is None]
    files = sorted((Version.parse(match.group(1)) for match in
                    (NOTES_FILE.fullmatch(name) for name in names) if match is not None),
                   reverse=True)
    if included != [each.name for each in files] or not files or files[0] != version:
        findings.append(f'doc/index.adoc: section Releases includes {", ".join(included)}, and '
                        f'it includes the notes of doc/releases newest first: '
                        f'{", ".join(each.name for each in files)}')
    return findings


def state_findings(root: Path, version: Version, notes: changelog.Notes,
                   logs: dict[str, list[changelog.Region]]) -> list[str]:
    """What breaks the state the notes give the version: in preparation, or frozen for its
    tag."""
    relative = f'doc/releases/{version.name}.adoc'
    wanted = 'unreleased' if notes.preparing else version.name
    state = 'in preparation' if notes.preparing else 'frozen'
    findings = [f'{relative}: {name} is included with tag={tag}, and the version is {state}: '
                f'tag={wanted}' for name, tag in notes.updated if tag != wanted]
    for name, regions in logs.items():
        if notes.preparing and version.name in {region.tag for region in regions}:
            findings.append(f'libs/{name}/doc/changelog.adoc has webcpp {version.name}, and '
                            f'{relative} says it is in preparation')
        if not notes.preparing and lines_of(regions, 'unreleased'):
            findings.append(f'libs/{name}/doc/changelog.adoc: Unreleased holds lines, and webcpp '
                            f'{version.name} is frozen')
    lanes = [lane.name for lane in matrix.LANES]
    if notes.compilers is None:
        if not notes.preparing:
            findings.append(f'{relative}: Compilers tested is None., and the version is frozen')
    elif notes.compilers != lanes:
        findings.append(f'{relative}: Compilers tested lists {", ".join(notes.compilers)}, and '
                        f'the CI\'s lanes are {", ".join(lanes)}')
    if not notes.preparing:
        tagged = subprocess.run(['git', '-C', str(root), 'rev-parse', '-q', '--verify',
                                 f'refs/tags/{version.tag}^{{commit}}'], capture_output=True,
                                text=True, check=False)
        head = git(root, 'rev-parse', 'HEAD').strip()
        if tagged.returncode == 0 and tagged.stdout.strip() != head:
            findings.append(f'the tag {version.tag} is at {tagged.stdout.strip()}, and HEAD is '
                            f'{head}')
    return findings


def library_findings(root: Path, version: Version, notes: changelog.Notes,
                     logs: dict[str, list[changelog.Region]], current: list[str]) -> list[str]:
    """What the libraries get wrong: each recorded at its checked-out commit; New and Removed
    Libraries exactly those new or gone since the previous tag; and every library changed since,
    or with lines for this version, under Updated Libraries with its lines."""
    relative = f'doc/releases/{version.name}.adoc'
    wanted = 'unreleased' if notes.preparing else version.name
    region = 'Unreleased' if notes.preparing else f'webcpp {version.name}'
    previous = previous_tag(root, version)
    since = previous or 'the first version'
    before = gitlinks(root, previous) if previous else {}
    head = gitlinks(root, 'HEAD')
    findings = []
    for name in current:
        if name not in head:
            findings.append(f'libs/{name} is no submodule of the superproject at HEAD: a release '
                            'ships only what its tag records')
            continue
        checked_out = git(root / 'libs' / name, 'rev-parse', 'HEAD').strip()
        if checked_out != head[name]:
            findings.append(f'libs/{name} is at {checked_out}, and the superproject records '
                            f'{head[name]}')
    new = sorted(set(current) - set(before))
    removed = sorted(set(before) - set(current))
    listed_new = [name for name, _ in notes.new]
    listed_removed = [name for name, _ in notes.removed]
    if listed_new != new:
        findings.append(f'{relative}: New Libraries lists {", ".join(listed_new) or "none"}, and '
                        f'the libraries new since {since} are {", ".join(new) or "none"}')
    if listed_removed != removed:
        findings.append(f'{relative}: Removed Libraries lists '
                        f'{", ".join(listed_removed) or "none"}, and the libraries gone since '
                        f'{since} are {", ".join(removed) or "none"}')
    updated = [name for name, _ in notes.updated]
    for name in updated:
        if name in new or name in removed:
            findings.append(f'{relative}: {name} is under Updated Libraries, and is '
                            f'{"new" if name in new else "removed"} since {since}')
        elif not lines_of(logs.get(name, []), wanted):
            findings.append(f'{relative}: {name} is under Updated Libraries, and its region '
                            f'{wanted} holds no line')
    for name in sorted(set(current) & set(before)):
        lines = lines_of(logs.get(name, []), wanted)
        if lines and name not in updated:
            findings.append(f'the release notes miss {name}, whose changelog has lines for this '
                            'version')
        if lines or name not in head:
            continue
        if not has_commit(root, name, before[name]):
            findings.append(f'libs/{name}: the commit {previous} records, {before[name]}, is not '
                            'in its repository; fetch its tags')
        elif not has_commit(root, name, head[name]):
            findings.append(f'libs/{name}: the commit HEAD records, {head[name]}, is not in its '
                            'repository')
        elif changed(root, name, before[name], head[name]):
            findings.append(f'libs/{name} changed since {previous} and its {region} has no line')
    return findings


def check(root: Path, version: Version) -> list[str]:
    """Every fault that keeps the tree from being tagged webcpp-<version>."""
    findings = version_findings(root, version)
    relative = f'doc/releases/{version.name}.adoc'
    if not (root / relative).is_file():
        return findings + [f'{relative} does not exist: open it from '
                           'tools/release/template.adoc']
    text = (root / relative).read_text(encoding='utf-8')
    notes, faults = changelog.parse_notes(text, version.name)
    findings += located(relative, sorted(faults + [fault for bullet in notes.tools
                                                   for fault in changelog.line_findings(bullet)]))
    findings += index_findings(root, version)
    current = libraries(root)
    logs: dict[str, list[changelog.Region]] = {}
    for name in current:
        path = f'libs/{name}/doc/changelog.adoc'
        if not (root / path).is_file():
            findings.append(f'{path} does not exist')
            continue
        log = (root / path).read_text(encoding='utf-8')
        findings += located(path, changelog.changelog_findings(log, name))
        logs[name] = changelog.parse_changelog(log, name)[0]
    findings += state_findings(root, version, notes, logs)
    findings += library_findings(root, version, notes, logs, current)
    return findings
```

  (`check` replaces Task 6's.)
- [ ] **Step 4: Run them and watch them pass.** `python3
  tools/release/release_test.py`, every other `tools/**/*_test.py`,
  `pyright` (0), the lint. Plant one defect and undo it: drop `'LICENSE*'`
  from `USER_VISIBLE`, and `test_a_change_no_user_sees_needs_no_line` fails.
- [ ] **Step 5: Commit** in `W`: `git add tools/release/release.py
  tools/release/release_test.py`, "tools/release: check-release, what a
  tag needs".

### Task 8: `whats-new`, and the form of its highlights

**Files:**
- Modify: `tools/release/release.py`, `tools/release/release_test.py`,
  `Jamroot` (the target `whats-new`)

**Interfaces:**
- Consumes: `check`, `lines_of`, `located` (Task 7); `changelog.parse_notes`
  and `parse_changelog` (Task 5).
- Produces: `CATEGORIES`, `HIGHLIGHTS`, `HIGHLIGHT`, `markdown`,
  `whats_new_text`, `whats_new`, `whats_new_findings`; `check` gains
  `findings += whats_new_findings(root, version)`.

- [ ] **Step 1: Write the failing tests,** added to `release_test.py` and
  `CASES` (`import re` joins its imports); `test_jamroot_targets` gains its
  last lines:

```python
def test_whats_new_skeleton(directory: Path) -> None:
    tree = scratch.make_tree(directory)
    tree.commit_file('doc/releases/0.1.0.adoc', scratch.notes_text(
        '0.1.0', new=('alpha', 'beta', 'config'),
        tools=('* Added `b2 dist`, which packages a release, for {cpp}20.',)))
    assert release.whats_new(tree.root, V010) == []
    written = tree.root / 'doc/releases/0.1.0.whats-new.md'
    text = written.read_text(encoding='utf-8')
    assert text.startswith('# webcpp 0.1.0\n\nwebcpp 0.1.0 is out.\n\n## New libraries\n'), text
    assert '- **alpha**: alpha, a library, from R.\n' in text, text
    assert '- Added `b2 dist`, which packages a release, for C++20.\n' in text, text
    assert '<!-- At most five highlights' in text and '{' not in text and 'link:' not in text
    assert 'https://webcpporg.github.io/webcpp/#webcpp-0-1-0' in text
    try:
        release.whats_new(tree.root, V010)
    except release.Failure as failure:
        assert 'doc/releases/0.1.0.whats-new.md exists, and is never written over' in str(failure)
    else:
        raise AssertionError('whats-new wrote over a What\'s new')
    assert written.read_text(encoding='utf-8') == text


def test_whats_new_lists_updated_libraries(directory: Path) -> None:
    tree = scratch.make_tree(directory)
    released(tree)
    tree.change('alpha', 'doc/changelog.adoc', scratch.changelog_text('alpha', unreleased=(
        '* *Breaking change:* removed `x`; use `y`.',
        '* Fixed a crash (https://github.com/webcpporg/alpha/issues/3[#3]).',
        '* Added `z`.', '* Updated the page.')))
    tree.commit_file('doc/releases/0.2.0.adoc',
                     scratch.notes_text('0.2.0', updated=(('alpha', 'unreleased'),)))
    release.whats_new(tree.root, V020)
    text = (tree.root / 'doc/releases/0.2.0.whats-new.md').read_text(encoding='utf-8')
    assert ('- **alpha**: **Breaking change:** removed `x`; use `y`. Fixed a crash '
            '([#3](https://github.com/webcpporg/alpha/issues/3)). Added `z`.\n') in text, text
    assert 'Updated the page' not in text


def test_whats_new_highlights_are_checked(directory: Path) -> None:
    tree = scratch.make_tree(directory)
    release.whats_new(tree.root, V010)
    written = tree.root / 'doc/releases/0.1.0.whats-new.md'
    skeleton = written.read_text(encoding='utf-8')
    assert release.check(tree.root, V010) == []

    def highlights(*lines: str) -> None:
        written.write_text(re.sub(r'<!--.*?-->', '\n'.join(lines), skeleton, flags=re.DOTALL),
                           encoding='utf-8')

    highlights('- **New libraries**: five header-only libraries arrive.',
               '- **Developer experience**: every changelog line is checked.')
    assert release.check(tree.root, V010) == []
    highlights(*(['- **Dependencies**: one.'] * 6))
    expect(release.check(tree.root, V010), '6 highlights, and there are at most 5')
    highlights('- **Dependencies**: ' + ' '.join(['word'] * 21) + '.')
    expect(release.check(tree.root, V010), 'the highlight Dependencies has more than 20 words')
    highlights('- **Speed**: faster.')
    expect(release.check(tree.root, V010), 'a highlight is "- **<category>**: <sentence>"')
    highlights('- **Dependencies**: one.', '- **New libraries**: two.')
    expect(release.check(tree.root, V010), 'the highlights come in the order')
```

```python
    harness.expect(harness.run_b2(root, 'whats-new'), True)
    assert (root / 'doc/releases/0.1.0.whats-new.md').is_file()
```

- [ ] **Step 2: Run them and watch them fail.** `python3
  tools/release/release_test.py`: `AttributeError: module 'release' has no
  attribute 'whats_new'`; `test_jamroot_targets` fails, b2 naming no target
  `whats-new`.
- [ ] **Step 3: Implement,** in `release.py` (the docstring gains `whats-new`
  and the form of the highlights):

```python
# The highlights of a What's new, Boost's: at most five, each one of these categories, in this
# order, and one sentence of at most 20 words.
CATEGORIES = ('New libraries', 'Performance improvements', 'Dependencies',
              'Security & reliability', 'Developer experience')
HIGHLIGHTS = ('<!-- At most five highlights, each "- **<category>**: <one sentence of at most 20 '
              'words>.", in this\norder, a category with nothing to say left out: '
              + ', '.join(CATEGORIES) + '. Written from the\nrelease notes, inventing nothing; '
              'the owner approves them before the tag. -->')
HIGHLIGHT = re.compile(r'- \*\*(?P<category>[^*]+)\*\*: (?P<sentence>.+)')


def markdown(text: str) -> str:
    """A line of AsciiDoc as the What's new's Markdown writes it: bold, links, {cpp}."""
    text = text.replace(changelog.BREAKING.strip(), '**Breaking change:**')
    text = re.sub(r'link:\{library-pages\}[a-z0-9_]+/\{library-page\}\[([^\]]*)\]', r'\1', text)
    text = re.sub(r'(?:link:)?(https?://[^\s\[]+)\[([^\]]*)\]', r'[\2](\1)', text)
    return text.replace('{cpp}', 'C++')


def whats_new_text(root: Path, version: Version) -> str:
    """The skeleton of the version's What's new, from its release notes."""
    relative = f'doc/releases/{version.name}.adoc'
    notes, faults = changelog.parse_notes((root / relative).read_text(encoding='utf-8'),
                                          version.name)
    if faults:
        raise Failure('\n'.join(located(relative, faults)))
    wanted = 'unreleased' if notes.preparing else version.name
    parts = [f'# webcpp {version.name}', f'webcpp {version.name} is out.']
    if notes.new:
        parts.append('## New libraries\n\n' + '\n'.join(
            f'- **{name}**: {markdown(what)}' for name, what in notes.new))
    parts.append('## Highlights\n\n' + HIGHLIGHTS)
    if notes.updated:
        entries = []
        for name, _ in notes.updated:
            log = (root / 'libs' / name / 'doc/changelog.adoc').read_text(encoding='utf-8')
            first = lines_of(changelog.parse_changelog(log, name)[0], wanted)[:3]
            entries.append(f'- **{name}**: ' + ' '.join(markdown(line.text) for line in first))
        parts.append('## Updated libraries\n\n' + '\n'.join(entries))
    if notes.removed:
        parts.append('## Removed libraries\n\n' + '\n'.join(
            f'- **{name}**: {markdown(what)}' for name, what in notes.removed))
    if notes.tools:
        parts.append('## Tools and build\n\n' + '\n'.join(
            f'- {markdown(line.text)}' for line in notes.tools[:3]))
    parts.append(f'Release notes: https://webcpporg.github.io/webcpp/#webcpp-{version.anchor}. '
                 f'Checksum:\n`webcpp-{version.name}.tar.gz.txt`.')
    return '\n\n'.join(parts) + '\n'


def whats_new(root: Path, version: Version) -> list[str]:
    """Writes the skeleton of the version's What's new, never over one that exists."""
    relative = f'doc/releases/{version.name}.whats-new.md'
    if (root / relative).exists():
        raise Failure(f'{relative} exists, and is never written over: remove it to write the '
                      'skeleton again')
    (root / relative).write_text(whats_new_text(root, version), encoding='utf-8')
    print(relative)
    return []


def whats_new_findings(root: Path, version: Version) -> list[str]:
    """What breaks the form of the version's What's new once its highlights are written: at
    most five, each a category of CATEGORIES, in their order, each once, and one sentence of at
    most 20 words."""
    relative = f'doc/releases/{version.name}.whats-new.md'
    if not (root / relative).is_file():
        return []
    text = (root / relative).read_text(encoding='utf-8')
    section = re.search(r'^## Highlights\n(.*?)(?=^## |^Release notes: |\Z)', text,
                        re.MULTILINE | re.DOTALL)
    if section is None:
        return [f'{relative} has no section ## Highlights']
    body = re.sub(r'<!--.*?-->', '', section.group(1), flags=re.DOTALL)
    lines = [line for line in body.split('\n') if line.strip()]
    findings = []
    categories = []
    for line in lines:
        match = HIGHLIGHT.fullmatch(line)
        if match is None or match.group('category') not in CATEGORIES:
            findings.append(f'{relative}: a highlight is "- **<category>**: <sentence>", its '
                            f'category one of {", ".join(CATEGORIES)}: {line}')
            continue
        categories.append(match.group('category'))
        if len(match.group('sentence').split()) > 20:
            findings.append(f'{relative}: the highlight {match.group("category")} has more than '
                            '20 words')
    if len(lines) > 5:
        findings.append(f'{relative}: {len(lines)} highlights, and there are at most 5')
    if categories != sorted(set(categories), key=CATEGORIES.index):
        findings.append(f'{relative}: the highlights come in the order {", ".join(CATEGORIES)}, '
                        'each once')
    return findings
```

  `check` gains, after `index_findings`, `findings += whats_new_findings(root,
  version)`; `COMMANDS` gains `'whats-new': lambda root, version, _:
  whats_new(root, version),`. The Jamroot, after `check-release`'s target:

```
rule whats-new ( targets * : sources * : properties * )
{
    release-tool $(targets) : $(sources) : $(properties) ;
}

actions whats-new
{
    "$(PYTHON)" "$(SCRIPT)" whats-new --root "$(ROOT)" --version "$(VERSION)"
}

notfile whats-new : @whats-new : : <webcpp-boost-check>off ;
```

  and `whats-new` joins the last `explicit`.
- [ ] **Step 4: Run them and watch them pass.** `release_test.py`, every
  `tools/**/*_test.py`, `pyright`, the lint. Nothing runs on `W` itself.
- [ ] **Step 5: Commit** in `W`: `git add Jamroot tools/release/release.py
  tools/release/release_test.py`, "tools/release: whats-new, the skeleton of
  a version's What's new, and the form of its highlights".

### Task 9: `dist`, a reproducible archive

**Files:**
- Modify: `tools/release/release.py`, `tools/release/release_test.py`,
  `Jamroot` (the target `dist`), `doc/releases/0.1.0.adoc` (the tooling's
  line)

**Interfaces:**
- Consumes: `libraries`, `gitlinks`, `git` (Task 7).
- Produces: `blobs`, `contents`, `hidden`, `dist_refusals`, `dist`.

- [ ] **Step 1: Write the failing tests,** added to `release_test.py` and
  `CASES` (`import hashlib`, `import tarfile` and `import time` join its
  imports); `test_jamroot_targets` gains its last lines:

```python
def test_dist_is_reproducible(directory: Path) -> None:
    tree = scratch.make_tree(directory)
    release.dist(tree.root, V010, directory / 'out one')
    first = (directory / 'out one/webcpp-0.1.0.tar.gz').read_bytes()
    time.sleep(1.1)
    release.dist(tree.root, V010, directory / 'out two')
    assert (directory / 'out two/webcpp-0.1.0.tar.gz').read_bytes() == first
    # The same commits and pages at another path, every page's time changed.
    other = directory / 'another place'
    shutil.copytree(tree.root, other, symlinks=True)
    for page in other.rglob('index.html'):
        os.utime(page, (0, 0))
    release.dist(other, V010, directory / 'out three')
    assert (directory / 'out three/webcpp-0.1.0.tar.gz').read_bytes() == first
    digest, name = (directory / 'out one/webcpp-0.1.0.tar.gz.txt').read_text().split()
    assert (digest, name) == (hashlib.sha256(first).hexdigest(), 'webcpp-0.1.0.tar.gz')
    with tarfile.open(directory / 'out one/webcpp-0.1.0.tar.gz') as archive:
        assert all(member.startswith('webcpp-0.1.0/') for member in archive.getnames())


def test_dist_leaves_out_dot_files_and_what_git_does_not_track(directory: Path) -> None:
    tree = scratch.make_tree(directory)
    scratch.write(tree.root / 'bin/dist/old.tar.gz', 'ignored\n')
    release.dist(tree.root, V010, directory / 'out')
    with tarfile.open(directory / 'out/webcpp-0.1.0.tar.gz') as archive:
        members = {member.name.removeprefix('webcpp-0.1.0/'): member
                   for member in archive.getmembers()}
    for absent in ('.gitignore', '.clang-format', '.github/workflows/ci.yml',
                   'libs/alpha/.gitattributes', 'libs/alpha/.gitignore', 'bin/dist/old.tar.gz'):
        assert absent not in members, absent
    for present in ('README.md', 'doc/index.adoc', 'doc/html/index.html',
                    'libs/alpha/doc/html/index.html', 'libs/alpha/include/webcpp/alpha.hpp',
                    'libs/config/include/webcpp/version.hpp'):
        assert present in members, present
    assert members['tools/run.sh'].mode == 0o755 and members['README.md'].mode == 0o644
    assert {(member.uid, member.gid, member.uname, member.gname)
            for member in members.values()} == {(0, 0, '', '')}


def test_dist_refuses_a_dirty_tree_and_missing_pages(directory: Path) -> None:
    tree = scratch.make_tree(directory)
    out = directory / 'out'
    scratch.write(tree.library('alpha') / 'notes.txt', 'untracked\n')
    try:
        release.dist(tree.root, V010, out)
    except release.Failure as failure:
        assert 'libs/alpha/notes.txt is not committed' in str(failure), failure
    else:
        raise AssertionError('dist archived a dirty tree')
    (tree.library('alpha') / 'notes.txt').unlink()
    (tree.library('beta') / 'doc/html/index.html').unlink()
    try:
        release.dist(tree.root, V010, out)
    except release.Failure as failure:
        assert 'libs/beta/doc/html/index.html is not built' in str(failure), failure
    else:
        raise AssertionError('dist archived without a page')
    assert not out.exists()
```

```python
    # b2 dist builds every page, then packs: the index's conversion comes before the archive.
    (root / '.local/mrdocs').symlink_to(mrdocs_root())
    dry = harness.run_b2(root, '-n', 'dist')
    harness.expect(dry, True)
    converted = dry.stdout.find('index.adoc')
    packed = dry.stdout.find('release.py" dist')
    assert 0 <= converted < packed, dry.stdout[-4000:]
```

- [ ] **Step 2: Run them and watch them fail.** `AttributeError: module
  'release' has no attribute 'dist'`; b2 names no target `dist`.
- [ ] **Step 3: Implement,** in `release.py` (`gzip`, `hashlib`, `io`, `os`
  and `tarfile` join the imports; the docstring gains `dist`):

```python
def blobs(directory: Path) -> list[tuple[str, str, str]]:
    """The blobs of the tree at HEAD of the repository directory: each path, git mode and
    object; its gitlinks left out."""
    listed = subprocess.run(['git', '-C', str(directory), 'ls-tree', '-r', '-z', 'HEAD'],
                            capture_output=True, check=False)
    if listed.returncode != 0:
        raise Broken(f'git ls-tree in {directory}: {listed.stderr.decode().strip()}')
    found = []
    for entry in listed.stdout.split(b'\0'):
        if not entry:
            continue
        meta, path = entry.split(b'\t', 1)
        mode, kind, name = meta.decode().split()
        if kind == 'blob':
            found.append((path.decode('utf-8'), mode, name))
    return found


def contents(directory: Path, objects: list[str]) -> dict[str, bytes]:
    """Each object's bytes, read with one git cat-file --batch."""
    read = subprocess.run(['git', '-C', str(directory), 'cat-file', '--batch'],
                          input=''.join(f'{name}\n' for name in objects).encode(),
                          capture_output=True, check=False)
    if read.returncode != 0:
        raise Broken(f'git cat-file in {directory}: {read.stderr.decode().strip()}')
    data = read.stdout
    found: dict[str, bytes] = {}
    position = 0
    while position < len(data):
        end = data.index(b'\n', position)
        name, _, size = data[position:end].decode().split()
        start = end + 1
        found[name] = data[start:start + int(size)]
        position = start + int(size) + 1
    return found


def hidden(path: str) -> bool:
    """Whether a path has a part that starts with a dot, which the archive leaves out."""
    return any(part.startswith('.') for part in PurePosixPath(path).parts)


def dist_refusals(root: Path) -> list[str]:
    """Why the tree cannot be archived: an uncommitted change, a library not at the commit the
    superproject records, a page not built."""
    refusals = []
    repositories = [('', root)] + [(f'libs/{name}/', root / 'libs' / name)
                                   for name in libraries(root)]
    for prefix, directory in repositories:
        arguments = ['status', '--porcelain', '--untracked-files=all']
        if not prefix:
            arguments.append('--ignore-submodules=all')
        for line in git(directory, *arguments).splitlines():
            refusals.append(f'{prefix}{line[3:]} is not committed: an archive holds what a tag '
                            'records')
    head = gitlinks(root, 'HEAD')
    for name in libraries(root):
        at = git(root / 'libs' / name, 'rev-parse', 'HEAD').strip()
        if head.get(name) != at:
            refusals.append(f'libs/{name} is at {at}, and the superproject records '
                            f'{head.get(name, "nothing")}')
    pages = ['doc/html/index.html'] + [f'libs/{name}/doc/html/index.html'
                                       for name in libraries(root)]
    refusals += [f'{page} is not built: b2 dist builds every page first' for page in pages
                 if not (root / page).is_file()]
    return refusals


def dist(root: Path, version: Version, out: Path) -> list[str]:
    """Writes webcpp-<version>.tar.gz and its sha256sum file into out: the blobs of the
    superproject and of every library at the commits recorded, and every built page, under
    webcpp-<version>/, no path with a part that starts with a dot; the same bytes from the same
    commits and pages, wherever and whenever it runs."""
    refusals = dist_refusals(root)
    if refusals:
        raise Failure('\n'.join(refusals))
    prefix = f'webcpp-{version.name}'
    mtime = int(git(root, 'log', '-1', '--format=%ct', 'HEAD').strip())
    members: dict[str, tuple[int, bytes, bool]] = {}
    for base, directory in [('', root)] + [(f'libs/{name}/', root / 'libs' / name)
                                           for name in libraries(root)]:
        listed = [entry for entry in blobs(directory) if not hidden(entry[0])]
        data = contents(directory, [name for _, _, name in listed])
        for path, mode, name in listed:
            members[base + path] = (0o755 if mode == '100755' else 0o644, data[name],
                                    mode == '120000')
    for pages in [root / 'doc/html'] + [root / 'libs' / name / 'doc/html'
                                        for name in libraries(root)]:
        for page in sorted(pages.rglob('*')):
            path = page.relative_to(root).as_posix()
            if page.is_file() and not hidden(path):
                members[path] = (0o755 if os.access(page, os.X_OK) else 0o644,
                                 page.read_bytes(), False)
    packed = io.BytesIO()
    with tarfile.open(fileobj=packed, mode='w', format=tarfile.PAX_FORMAT) as archive:
        for path in sorted(members):
            mode, data, link = members[path]
            info = tarfile.TarInfo(f'{prefix}/{path}')
            info.mtime = mtime
            info.uid = info.gid = 0
            info.uname = info.gname = ''
            if link:
                info.type = tarfile.SYMTYPE
                info.linkname = data.decode('utf-8')
                info.mode = 0o777
                archive.addfile(info)
            else:
                info.size = len(data)
                info.mode = mode
                archive.addfile(info, io.BytesIO(data))
    out.mkdir(parents=True, exist_ok=True)
    target = out / f'{prefix}.tar.gz'
    partial = out / f'{prefix}.tar.gz.partial'
    with partial.open('wb') as file:
        with gzip.GzipFile(filename='', mode='wb', fileobj=file, compresslevel=9,
                           mtime=0) as compressed:
            compressed.write(packed.getvalue())
    partial.replace(target)
    checksum = out / f'{prefix}.tar.gz.txt'
    checksum.write_text(f'{hashlib.sha256(target.read_bytes()).hexdigest()}  {target.name}\n',
                        encoding='utf-8')
    print(target)
    print(checksum)
    return []
```

  `COMMANDS` gains `'dist': lambda root, version, arguments: dist(root,
  version, arguments.out.resolve()),`. The Jamroot, after `whats-new`'s
  target:

```
rule dist ( targets * : sources * : properties * )
{
    release-tool $(targets) : $(sources) : $(properties) ;
}

actions dist
{
    "$(PYTHON)" "$(SCRIPT)" dist --root "$(ROOT)" --version "$(VERSION)" --out "$(ROOT)/bin/dist"
}

notfile dist : @dist : : <dependency>doc ;
```

  and `dist` joins the last `explicit`. Measure, and record in the ledger,
  that `<dependency>doc` orders the conversions before the action (`B2 -n
  dist` in `W`); if b2 refuses the alias there, name it
  `<dependency>/webcpp//doc` and say so in the comment above `release-tool`.
- [ ] **Step 4: Run them and watch them pass.** `release_test.py`, every
  `tools/**/*_test.py`, `pyright`, the lint. Nothing runs on `W` itself.
- [ ] **Step 5: The line and the commit.** `doc/releases/0.1.0.adoc`, Tools
  and build: "Added `b2 check-release`, `b2 whats-new` and `b2 dist`, which
  check, announce and package a release." Commit in `W`: `git add Jamroot
  tools/release/release.py tools/release/release_test.py
  doc/releases/0.1.0.adoc`, "tools/release: dist, a reproducible archive of
  a version".

### Task 10: AGENTS.md and README.md

**Files:**
- Modify: `W/AGENTS.md`, `W/README.md`

**Interfaces:** every name above, as the rulebook states it. What Task 13
adds (the libraries on `<webcpp/config.hpp>`) is not stated here: chapter 13
lists it as to come.

- [ ] **Step 1: The edits.**
  - Chapter 1: the libraries table gains config, first: "| config |
    webcpp's configuration, which every library includes first: the target,
    exceptions and webcpp's version, as Boost.Config is Boost's; webcpp's own
    | native, wasip2, wasip3, emscripten | a local repository at
    `libs/config`, until `webcpporg/config` exists |". The superproject's
    layout gains `doc/releases/` ("each version's release notes, which the
    index page shows") and `tools/release/` ("changelog.py, the form of a
    changelog, which the lint checks; release.py, the release tooling;
    template.adoc; scratch.py and their tests"); a library's layout gains
    `doc/changelog.adoc` ("its changelog, the page's last appendix"). The
    build commands table gains `b2 check-release`, `b2 whats-new` and `b2
    dist` (chapter 11).
  - Chapter 2: "The name" gains "nor `target`, `has`, `no`, `lib` or
    `version`, which config's macros take (`WEBCPP_TARGET_*`,
    `WEBCPP_HAS_EXCEPTIONS`, ...): the index refuses each"; the checklist's
    item 6 lists `doc/changelog.adoc`; item 8 adds "and its line under New
    Libraries in the release notes of the version in preparation".
  - Chapter 3: after the table, "config is the one exception: it holds the
    collection's macros, `WEBCPP_*`, as Boost.Config holds `BOOST_*`, in
    `<webcpp/config.hpp>` and `<webcpp/version.hpp>`, whose include guards
    are `WEBCPP_CONFIG_HPP` and `WEBCPP_VERSION_HPP`, and declares no
    namespace."
  - Chapter 6: the lint table gains the row "| changelog | a library's
    changelog or a version's release notes out of chapter 11's form: a
    section or a region missing or out of order, a line that starts with no
    verb of the list, a cross-reference, more than ten lines for a library
    and a version, text outside a bullet |"; and, only when Task 1, Step 10
    changed `compile_commands.py`, the clang-tidy row's "a program that
    declares `<exception-handling>on` has none" becomes "a program that
    declares `<exception-handling>on` or `off` has none: b2 builds it so
    whatever the build asks".
  - Chapter 7: the MrDocs bullet's "the macros `WEBCPP_<NAME>_*` except the
    include guards" gains ", every `WEBCPP_*` for config".
  - Chapter 8: "What a page holds" gains a last item, "**the appendix
    Changelog,** `include::changelog.adoc[]`, last (chapter 11)"; doc-check's
    `{webcpp-root}` item gains "or under `libs/<library>/` that the library's
    own git tracks, as the release notes include a library's changelog"; the
    index page's section says the index shows each version's release notes,
    newest first, and that the notes name a library only once it is a
    submodule, since the site's link check refuses a page that is not there.
  - Chapter 10: the list of tests gains `tools/release/changelog_test.py`
    and `tools/release/release_test.py`.
  - Chapter 11 gains two sections after "Commits and text"'s bullets, whole:

```
### The changelog

- **Every change a user can see adds its line, in the same commit.** A
  library's goes in its `doc/changelog.adoc`, under "Unreleased"; one of the
  superproject's tools or build in the release notes of the version in
  preparation, `doc/releases/<version>.adoc`, under "Tools and build", which
  lists what changes for whoever builds, lints or tests with it: its b2
  rules and targets, the lint's rules, the CI's lanes. Agents write them:
  for webcpp, this replaces the rule of never editing a changelog.
- **A line** is one bullet: `* `, a past-tense verb (Added, Changed,
  Deprecated, Fixed, Improved, Removed or Updated), then one sentence of
  about ten words, what a user sees rather than how it was done; it may wrap
  onto the next lines. A breaking change starts with `*Breaking change:*`
  and says what the user must change; a deprecation names what replaces it;
  a fixed bug names the symptom, with its issue or pull request when there
  is one, `https://github.com/webcpporg/<library>/issues/<n>[#<n>]`. A line
  holds no cross-reference, `<<...>>`: the index page shows it too, where the
  library's anchors are not.
- **At most ten lines per library and version,** the most important first;
  a version's documentation work is one line. A library's change that no
  user sees (a test, its CI, a refactoring) gets none.
- **The file.** `[appendix]`, `[#<library>-changelog]`, `== Changelog`, a
  paragraph, then `=== Unreleased` and each released version,
  `=== webcpp <version>`, newest first, each with its anchor
  (`[#<library>-changelog-unreleased]`, `[#<library>-changelog-0-1-0]`) and
  its lines between `// tag::unreleased[]` (`// tag::<version>[]`) and its
  `end::`. A released section is never rewritten: a correction is a line of
  the next version. The lint's rule `changelog` checks every changelog and
  release notes on every run (`tools/release/changelog.py`).
- **The release notes,** `doc/releases/<version>.adoc`, hold what belongs to
  the collection, in Boost's sections: New Libraries (one line each, what it
  is, "from <authors>"), Updated Libraries (each library a section that
  includes its changelog's region, alphabetical), Removed Libraries (what
  went out, and what replaces it), Tools and build, Compilers tested (the
  CI's lanes). The index page includes each, newest first. While a version
  is in preparation its notes open with the paragraph "In preparation: ..."
  and include each library's `tag=unreleased`.

### Releases

webcpp has one version, `WEBCPP_VERSION` in `<webcpp/version.hpp>`
(`libs/config`), the one place it is written: 0.1.0 is 100,
`WEBCPP_LIB_VERSION` `"0_1"`. The Jamroot reads it for the release
targets, which run only when named:

| Command | What it does |
| --- | --- |
| `b2 check-release` | fails, naming each fault, when a library changed since the last tag (under `include/`, `build.jam`, `wit/`, `meta/libraries.json`, `README.md`, `example/`, `doc/` or `LICENSE*`) has no line, when the notes miss a library, when a new or removed submodule is not under New or Removed Libraries, when the version disagrees anywhere, when a line or a file breaks the form above, or when the What's new's highlights break theirs |
| `b2 whats-new` | writes `doc/releases/<version>.whats-new.md`, the skeleton of the GitHub Release's body, in Boost's form: at most five highlights, a bold category and at most 20 words each; never over a file that exists |
| `b2 dist` | builds every page, then `bin/dist/webcpp-<version>.tar.gz`, reproducible (sources at the recorded commits, built pages, no dot files), and its `sha256sum` file `.tar.gz.txt` |

Nothing is released until the owner asks. Then:

1. In every library whose "Unreleased" holds lines, it becomes
   `=== webcpp <version>`, with its anchor and region, under a new empty
   "Unreleased"; the notes lose their paragraph "In preparation: ..." and
   include `tag=<version>`; each repository commits, and the superproject
   records each library's commit.
2. `b2 check-release` passes.
3. `b2 whats-new`; an agent writes the highlights from the notes, and the
   owner approves them before the tag.
4. `b2 -a dist`.
5. The tag `webcpp-<version>` goes on the superproject and on every library
   at the commit the superproject records. Whether it is signed is the
   owner's decision then.
6. The GitHub Release `webcpp-<version>` carries the archive, its checksum
   file and the "What's new" as its body:
   `gh release create webcpp-<version> bin/dist/webcpp-<version>.tar.gz
   bin/dist/webcpp-<version>.tar.gz.txt --title "webcpp <version>"
   --notes-file doc/releases/<version>.whats-new.md`.
7. Right after, `version.hpp` names the next version, whose notes are
   opened from `tools/release/template.adoc` (`sed -e 's/@VERSION@/<next>/g'
   -e 's/@ANCHOR@/<next with dashes>/g'`) and included first by the index.
```

  - Chapter 12 gains: "**A target named `release-<word>` is no target.** b2
    reads it as the variant `release` with a subfeature and stops before it
    builds anything (`"release-check" is not a known value of feature
    <variant>`); `debug-` and `profile-` likewise. The release check is
    `check-release`."
  - Chapter 13 gains: "**Every library on `<webcpp/config.hpp>`.** Each
    library's `config.hpp` includes it first, and `WEBCPP_NO_EXCEPTIONS`
    disables every library's exceptions at once (xactor, xstate, pratt and
    trystero; wasi with its redesign, which brings Boost into it)."
  - `README.md`: after the paragraph on exceptions, one paragraph: "webcpp
    has one version, `WEBCPP_VERSION` in `<webcpp/version.hpp>`, part of
    `libs/config`, whose `<webcpp/config.hpp>` every library includes first.
    Each library's page ends with its changelog, and the index page shows
    each version's release notes."
- [ ] **Step 2: Check them.** The lint (`lint: clean`: em dash, banned
  word); `B2 -a doc`; `grep -n 'release-check' AGENTS.md README.md` prints
  only chapter 12's fact.
- [ ] **Step 3: Commit** in `W`: `git add AGENTS.md README.md`, commit
  "AGENTS.md: the config library, the changelog rule and the release
  tooling". No changelog line.

### Task 11: Onto stage 4's published tree: a library's changelog in the notes, and trystero's

**Starts only once stage 4's Task 14 has published** (its ledger shows the
stage complete). trystero is then a submodule of `M`, published, complete
and lint-clean, and doc-check is as stage 4's Task 11 left it, with its
`{library-root}`, "a region of a file the library's own git tracks": this
task's include rule shares one helper with it.

**Files:**
- `W`, branch `config`: rebased onto `M`'s `main`; `doc/releases/0.1.0.adoc`
  (trystero's New Libraries line; Tools and build lines added to the
  pending file since Task 4)
- `W/libs/<name>` for xactor, xstate, pratt, wasi: branch `config` rebased
  onto each library's `main`; `doc/changelog.adoc` (lines added to the
  pending file since Task 3)
- `W/libs/trystero` (new worktree, branch `config`): create
  `doc/changelog.adoc`; modify `doc/trystero.adoc` (its include, last)
- `W`: `tools/doc/doc.jam` (rule `convert`, around line 542, and rule
  `doc-check`, around lines 449-491), `tools/doc/doc-check.py` (class
  `Library`, around line 295; `superproject_fault`, around line 372; the
  module docstring), `tools/doc/doc_check_test.py`, `tools/test/doc_test.py`

**Interfaces:**
- Consumes: Task 3's changelog file; Task 4's notes and New Libraries form.
- **doc-check.py:** `Library.listed` becomes a cache per repository, and two
  methods serve `{webcpp-root}` and stage 4's `{library-root}` alike. If
  stage 4 wrote its own helper for the library's git, it is replaced by these:

```python
    def tracked_in(self, repository: Path) -> set[str] | None:
        """The files the git repository at repository tracks, by their path in it, read once
        for each; None when it is not a git checkout, or git is not there to list them."""
        key = repository.resolve()
        if key not in self.listed:
            try:
                listed = subprocess.run(['git', '-C', str(repository), 'ls-files', '-z'],
                                        capture_output=True, check=False)
            except OSError:
                return None
            if listed.returncode != 0:
                return None
            self.listed[key] = set(listed.stdout.decode('utf-8').split('\0')) - {''}
        return self.listed[key]

    def repository_of(self, relative: str) -> tuple[Path, str]:
        """The repository whose git tracks the superproject's path relative, and the path in it:
        libs/<library>/<rest> is the library's own repository when libs/<library> is one, and any
        other path the superproject's."""
        assert self.webcpp_root is not None
        parts = PurePosixPath(relative).parts
        library = self.webcpp_root / 'libs' / parts[1] if len(parts) > 2 else None
        if parts[0] == 'libs' and library is not None and (library / '.git').exists():
            return library, PurePosixPath(*parts[2:]).as_posix()
        return self.webcpp_root, relative
```

  with `self.listed: dict[Path, set[str]] = {}` in `__init__`, and in
  `superproject_fault` the tracked test becomes:

```python
    repository, inside = library.repository_of(
        path.resolve().relative_to(top.resolve()).as_posix())
    tracked = library.tracked_in(repository)
    if tracked is None:
        return (f'includes a file of the superproject, and {repository} is not a git checkout, '
                'or git is not on PATH: the check reads the files git tracks')
    if inside not in tracked:
        return ('includes a file the superproject does not track' if repository == top else
                'includes a file its repository does not track')
```

  Every other refusal of the rule stays (a region, never a whole file or
  lines by number; the region must be in the file). `from pathlib import
  PurePosixPath` joins the imports if it is not there.
- **doc.jam:** the index is converted with `webcpp-root=<superproject>` and
  checked with `--webcpp-root`, as a library's page is: in `rule convert`,
  `local root = [ path.native $(.root) ] ;` moves before the `if`, and
  `webcpp-root=$(root)` joins the attributes every page gets (the line
  `local attributes = ...`), leaving the library branch; in `rule
  doc-check`, `WEBCPP_ROOT on $(targets) = [ path.native $(.root) ] ;`
  replaces the empty assignment and leaves the library branch, whose comment
  ("Each empty for the index page...") no longer names it.


- [ ] **Step 1: Rebase.** `git -C ~/dev/webcpp-config rebase main`; for each
  of xactor, xstate, pratt and wasi, `git -C ~/dev/webcpp-config/libs/<name>
  rebase main`. On a conflict, `main`'s content is kept and this track's
  change applied on top; stage 4's Task 12 rewrote AGENTS.md chapters 1, 9
  and 13 and the README, where Task 10's additions go back in at their
  places. Then trystero:

```bash
rmdir ~/dev/webcpp-config/libs/trystero
git -C ~/dev/webcpp/libs/trystero worktree add ~/dev/webcpp-config/libs/trystero -b config main
```

- [ ] **Step 2: Write the failing tests.** `doc_check_test.py`, a part added
  to `main()`'s list after `check_superproject`:

```python
def check_library_file(root: Path) -> None:
    """The release notes include a region of a library's changelog through
    {webcpp-root}/libs/<library>/...: a file the library's own git tracks, as libs/<library> is a
    repository of its own. A file it does not track, a whole file and lines by number fail."""
    with tempfile.TemporaryDirectory(prefix='superproject ') as scratch:
        top = Path(scratch)
        subprocess.run(['git', 'init', '-q'], cwd=top, check=True)
        library = top / 'libs/lib'
        changelog = ('// tag::unreleased[]\n* Added a line.\n// end::unreleased[]\n')
        write(library / 'doc/changelog.adoc', changelog)
        write(library / 'doc/untracked.adoc', changelog)
        subprocess.run(['git', 'init', '-q'], cwd=library, check=True)
        subprocess.run(['git', 'add', 'doc/changelog.adoc'], cwd=library, check=True)
        given = ('--webcpp-root', str(top))
        line = NEXT
        write(root / 'doc/page.adoc', PAGE + '\ninclude::{webcpp-root}/libs/lib/doc/'
                                             'changelog.adoc[tag=unreleased]\n')
        expect(check(root, *given), 0, '')
        for included, fault in (
                ('untracked.adoc[tag=unreleased]', 'includes a file its repository does not '
                                                   'track: {webcpp-root}/libs/lib/doc/'
                                                   'untracked.adoc'),
                ('changelog.adoc[]', 'includes a file of the superproject whole; name its region '
                                     'with tag=<name>'),
                ('changelog.adoc[lines=1..2]', 'includes lines of a file of the superproject by '
                                               'number, which drift')):
            write(root / 'doc/page.adoc',
                  PAGE + f'\ninclude::{{webcpp-root}}/libs/lib/doc/{included}\n')
            expect(check(root, *given), 1, f'page.adoc:{line}: {fault}')
    write(root / 'doc/page.adoc', PAGE)
```

  (`NEXT` is the file's existing number of the first line after `PAGE`; if
  the include sits on another line in this form, `line` is `NEXT + 1`, read
  once from the first failing run.) `tools/test/doc_test.py`, added to
  `CASES`:

```python
def test_index_shows_release_notes_with_a_library_s_section(root):
    # The index shows each version's release notes, which include a library's section of its
    # changelog by its region, a file the library's own git tracks, shown without its tag
    # lines. A region renamed, the file untracked, or notes the index does not include fail.
    prepare(root)
    demo = root / 'libs/demo'
    (demo / 'doc/changelog.adoc').write_text(
        '[appendix]\n[#demo-changelog]\n== Changelog\n\nWhat changed.\n\n'
        '[#demo-changelog-unreleased]\n=== Unreleased\n\n// tag::unreleased[]\n'
        '* Added a planted line.\n// end::unreleased[]\n')
    with (demo / 'doc/demo.adoc').open('a') as page:
        page.write('\ninclude::changelog.adoc[]\n')
    subprocess.run(['git', 'add', 'doc/changelog.adoc'], cwd=demo, check=True)
    (root / 'doc/releases/9.0.0.adoc').write_text(
        '[#webcpp-9-0-0]\n= webcpp 9.0.0\n\n[#webcpp-9-0-0-updated-libraries]\n'
        '== Updated Libraries\n\n[#webcpp-9-0-0-demo]\n=== demo\n\n'
        'include::{webcpp-root}/libs/demo/doc/changelog.adoc[tag=unreleased]\n')
    edit(root, 'doc/index.adoc', 'include::releases/0.1.0.adoc[leveloffset=+2]\n',
         'include::releases/9.0.0.adoc[leveloffset=+2]\n\n'
         'include::releases/0.1.0.adoc[leveloffset=+2]\n')
    subprocess.run(['git', 'init', '-q', '-b', 'main'], cwd=root, check=True)
    harness.expect(harness.run_b2(root, 'doc'), True)
    html = (root / 'doc/html/index.html').read_text()
    assert 'Added a planted line.' in html, html[-3000:]
    assert 'tag::' not in html and 'end::' not in html, html[-3000:]
    edit(root, 'libs/demo/doc/changelog.adoc', '// tag::unreleased[]', '// tag::renamed[]')
    edit(root, 'libs/demo/doc/changelog.adoc', '// end::unreleased[]', '// end::renamed[]')
    harness.expect(harness.run_b2(root, 'doc'), False, 'includes a region the file does not hold')
    edit(root, 'libs/demo/doc/changelog.adoc', '// tag::renamed[]', '// tag::unreleased[]')
    edit(root, 'libs/demo/doc/changelog.adoc', '// end::renamed[]', '// end::unreleased[]')
    subprocess.run(['git', 'rm', '-q', '--cached', 'doc/changelog.adoc'], cwd=demo, check=True)
    harness.expect(harness.run_b2(root, 'doc'), False,
                   'includes a file its repository does not track: '
                   '{webcpp-root}/libs/demo/doc/changelog.adoc')
    subprocess.run(['git', 'add', 'doc/changelog.adoc'], cwd=demo, check=True)
    edit(root, 'doc/index.adoc', 'include::releases/9.0.0.adoc[leveloffset=+2]\n\n', '')
    harness.expect(harness.run_b2(root, 'doc'), False, 'releases/9.0.0.adoc: not reached from '
                                                       'the page')
```

  (These scratch files are no source of the project: the lint never reads
  them, so they carry no notice.)
- [ ] **Step 3: Run them and watch them fail.** `python3
  tools/doc/doc_check_test.py`: `check_library_file` fails with "includes a
  file the superproject does not track" for the tracked changelog.
  `python3 tools/test/doc_test.py
  test_index_shows_release_notes_with_a_library_s_section`: fails with
  "includes a file of the superproject, and the check was not given
  --webcpp-root".
- [ ] **Step 4: Implement** doc-check's two methods and its rule as above,
  doc.jam's two changes, and doc-check's docstring: "...a file the
  superproject's git tracks, or one under `libs/<library>/` that the
  library's own git tracks, as the release notes include a library's section
  of its changelog".
- [ ] **Step 5: Run them and watch them pass.** Both files green, and every
  other `tools/**/*_test.py`. Commit in `W`: `git add tools/doc/doc.jam
  tools/doc/doc-check.py tools/doc/doc_check_test.py tools/test/doc_test.py`,
  "tools/doc: the release notes include a library's changelog region, which
  its own git tracks".
- [ ] **Step 6: The failing check.** Add `include::changelog.adoc[]` at the
  end of trystero's page; `B2 libs/trystero/doc` fails with Asciidoctor's
  "include file not found".
- [ ] **Step 7: trystero's changelog,** in Task 3's form: its "Unreleased"
  region holds the pending file's trystero lines as they read now, then
  "Added a changelog, the page's last appendix, with each version's
  changes."; at most ten lines (the lint's rule refuses more). The notes'
  New Libraries gains, in alphabetical order:
  "* link:{library-pages}trystero/{library-page}[trystero]: a port of
  Trystero 0.26.0, serverless WebRTC rooms wire-compatible with the
  original, for {cpp}20, from R."
- [ ] **Step 8: Everything else pending.** Every line of
  `~/.buzz/PLANS/WEBCPP_CHANGELOG_PENDING.md` not yet in a changelog or in
  the notes moves there now: each library's into its "Unreleased" (pratt's
  emscripten lines among them), the superproject's into Tools and build.
  The pending file is then empty of entries; Task 12 retires it.
- [ ] **Step 9: Green in `W`,** from scratch, in parallel: every library's
  native lane (trystero's included), config's five lanes, trystero's
  emscripten lane, `B2 -a doc`, the lint (the rule `changelog` reads
  trystero's file too), every `tools/**/*_test.py`, `pyright`.
- [ ] **Step 10: Commit,** each by path: in `libs/trystero` (branch `config`),
  `git add doc/changelog.adoc doc/trystero.adoc`, "trystero: a changelog, the
  page's last appendix"; in each library that gained lines, "<name>: the
  changelog lines gathered before it existed"; in `W`, `git add
  doc/releases/0.1.0.adoc`, "doc: trystero among the new libraries of 0.1.0,
  and the lines gathered before the notes existed".

### Task 12: Into `M`'s main, unpublished (controller)

- [ ] **Step 1: Pause.** No stage-4 work is running (it is complete); no
  other track pushes `M` or a library from now until Task 15.
- [ ] **Step 2: The libraries' `main`s.** For xactor, xstate, pratt, wasi and
  trystero: in `M/libs/<name>`, `git merge --ff-only config`.
- [ ] **Step 3: `libs/config` into `M`.**

```bash
mv ~/dev/webcpp-config/libs/config ~/dev/webcpp/libs/config
git -C ~/dev/webcpp/libs/config worktree add --detach ~/dev/webcpp-config/libs/config main
```

  In `M` it is a local repository the superproject does not track yet, as
  AGENTS.md chapter 2 says of a library whose repository does not exist.
- [ ] **Step 4: The superproject's `main`.** `git -C ~/dev/webcpp merge
  --ff-only config`. No pointer is recorded: Task 15 records them all.
- [ ] **Step 5: Green in `M`,** from scratch, in parallel: every library's
  native lane, config's five lanes, trystero's emscripten lane, wasi's own
  lanes, xstate's and trystero's oracle lanes, `B2 -a doc`, the lint, every
  `tools/**/*_test.py`, `pyright`. Expected: all green.
- [ ] **Step 6: Retire the pending file and say so.**
  `~/.buzz/PLANS/WEBCPP_CHANGELOG_PENDING.md`'s frontmatter becomes `status:
  superseded`, its first line naming the files that replace it. A line in
  this ledger and one in the stage-4 ledger: "`M`'s `main`, and the `main`s
  of xactor, xstate, pratt, wasi and trystero, carry config's unpublished
  commits: nobody pushes any of them until the config plan's publication. A
  change writes its changelog line into `libs/<name>/doc/changelog.adoc` or
  `doc/releases/0.1.0.adoc` itself, in the same commit."

### Task 13: The libraries on `<webcpp/config.hpp>` (held until Task 15)

Runs after Task 12, on branches `config-adoption` made from each library's
`main` and from `M`'s `main`, in `W`. Nothing of it reaches a `main` before
Task 15.

**Files** (each library's own repository, branch `config-adoption`):
- xactor, xstate, pratt: `include/webcpp/<name>/config.hpp`, `build.jam`,
  `test/Jamfile`, `test/config_test.cpp`, `doc/changelog.adoc`, and every
  text that names the macro or a by-hand `-I`: xactor `AGENTS.md` (22-26),
  `doc/testing.adoc` (165-173), `doc/quick-start.adoc` (82, 95); xstate
  `AGENTS.md` (36), `doc/testing.adoc` (624-632),
  `doc/introduction/quick-start.adoc` (32, 41, 52, 57); pratt `AGENTS.md`
  (74), `doc/testing.adoc` (171-179). `test/CONVERSION.md` records the
  conversion's history and is not edited.
- wasi: `include/webcpp/wasi/config.hpp`, `test/Jamfile`,
  `test/config_test.cpp`, `doc/changelog.adoc`, `doc/boost.adoc` (102, 112,
  121-127), `README.md` (17), `AGENTS.md` (47)
- Superproject (`W`, branch `config-adoption`): `AGENTS.md` chapters 2, 3, 6
  and 13

(Line numbers are those of 2026-10-09; read each in context after Task 11's
rebase.)

**Interfaces:**
- Consumes: `/webcpp/config//config`, `WEBCPP_HAS_EXCEPTIONS`,
  `WEBCPP_NO_EXCEPTIONS` (Task 1).
- xactor's `config.hpp`, the model of xstate's and pratt's: `#include
  <boost/config.hpp>` becomes `#include <webcpp/config.hpp>`; the
  `__MRDOCS__` block stays, its Doc Comment's first paragraph now "Defined
  when xactor is built without exceptions: when webcpp has none (the compiler
  has none, or the developer defined `WEBCPP_NO_EXCEPTIONS` for every webcpp
  library), or when the developer defines it alone, to disable xactor's in a
  build that has them."; the condition becomes the spec's:

```cpp
#if !defined(WEBCPP_HAS_EXCEPTIONS) && !defined(WEBCPP_XACTOR_NO_EXCEPTIONS)
#define WEBCPP_XACTOR_NO_EXCEPTIONS
#endif
```

- `build.jam`: `alias xactor : : : : <include>include <library>/boost//headers
  <library>/webcpp/config//config ;` (xstate and pratt likewise; xstate also
  reaches it through xactor).
- wasi's `config.hpp` keeps including nothing; its condition gains
  `WEBCPP_NO_EXCEPTIONS`, and its comment says that wasi reads it itself
  until it includes `<webcpp/config.hpp>`, with its redesign:

```cpp
#if !defined(WEBCPP_WASI_NO_EXCEPTIONS) &&                              \
    (defined(WEBCPP_NO_EXCEPTIONS) || defined(BOOST_NO_EXCEPTIONS) || \
     !(defined(__cpp_exceptions) || defined(_CPPUNWIND)))
#define WEBCPP_WASI_NO_EXCEPTIONS
#endif
```

- [ ] **Step 1: The branches.**

```bash
for name in xactor xstate pratt wasi; do
    git -C ~/dev/webcpp-config/libs/$name switch -c config-adoption main
done
git -C ~/dev/webcpp-config switch -c config-adoption main
```

- [ ] **Step 2: Write the failing tests.** In each library's `test/Jamfile`,
  after `config_developer_defined`:

```
# WEBCPP_NO_EXCEPTIONS, which a developer defines for every webcpp library at
# once, defines WEBCPP_XACTOR_NO_EXCEPTIONS in a build that has exceptions.
webcpp.compile config_all_off : config_test.cpp
  : <define>WEBCPP_NO_EXCEPTIONS <define>WEBCPP_TEST_XACTOR_ALL_OFF ;
```

  (wasi's with `: : native wasip2 wasip3` as its other config tests). In each
  `config_test.cpp`, its first comment saying "three times", and before the
  existing branches:

```cpp
#ifdef WEBCPP_TEST_XACTOR_ALL_OFF
#ifndef WEBCPP_XACTOR_NO_EXCEPTIONS
#error "WEBCPP_NO_EXCEPTIONS does not define WEBCPP_XACTOR_NO_EXCEPTIONS"
#endif
#elif defined(WEBCPP_TEST_XACTOR_DEVELOPER_DEFINED)
```

  (the existing `#ifdef WEBCPP_TEST_XACTOR_DEVELOPER_DEFINED` becomes this
  `#elif`). xactor's, xstate's and pratt's also check, after the include, in
  place of their check that Boost.Config was included (webcpp's includes it):

```cpp
#ifndef WEBCPP_CONFIG_HPP
#error "config.hpp does not include <webcpp/config.hpp>"
#endif
```

  wasi's keeps checking that nothing of Boost came in.
- [ ] **Step 3: Run them and watch them fail.** `B2
  libs/<name>/test//config_all_off` for each. Expected: `error:
  "WEBCPP_NO_EXCEPTIONS does not define WEBCPP_<NAME>_NO_EXCEPTIONS"`, and
  for xactor, xstate and pratt the plain `config` fails with "config.hpp
  does not include <webcpp/config.hpp>".
- [ ] **Step 4: Implement** the headers and `build.jam` lines above, and the
  texts: each `testing.adoc` paragraph says the macro is defined "when
  webcpp has no exceptions (`WEBCPP_HAS_EXCEPTIONS` is not defined: the
  compiler has none, or the program defined `WEBCPP_NO_EXCEPTIONS`), or by a
  program for <name> alone", and that the tests `config`,
  `config_developer_defined` and `config_all_off` check the three ways;
  xactor's, xstate's and pratt's `AGENTS.md` bullets likewise; each by-hand
  command adds `-I libs/config/include` after `-I libs/<name>/include`
  (xstate's prose at 41 names it too); wasi's `boost.adoc` (121-127), README
  row and `AGENTS.md` say that `WEBCPP_NO_EXCEPTIONS` defines it too, and its
  `boost.adoc` 102 and 112 keep describing Boost's `BOOST_NO_EXCEPTIONS`,
  which is unchanged.
- [ ] **Step 5: The lines.** In each library's "Unreleased", after the
  existing lines, with `<name>` the library's name: for xactor, xstate and
  pratt, "Changed `config.hpp` to include `<webcpp/config.hpp>`, webcpp's
  configuration, which the build adds." and "Added `WEBCPP_NO_EXCEPTIONS`,
  which disables <name>'s exceptions with every webcpp library's."; for wasi,
  the second alone. The lint's rule `changelog` must stay clean (at most ten
  lines each).
- [ ] **Step 6: AGENTS.md** (superproject, `config-adoption`): chapter 3's
  configuration row: "`<webcpp/<name>/config.hpp>`, which every public header
  includes first, which includes `<webcpp/config.hpp>` first, with
  `WEBCPP_<NAME>_NO_EXCEPTIONS` (chapter 6)"; chapter 6's bullet "Each
  library has its own configuration macro" says it is defined when
  `WEBCPP_HAS_EXCEPTIONS` is not (no exceptions, or the developer's
  `WEBCPP_NO_EXCEPTIONS`, which disables every library's at once) or when the
  developer defines it alone, that its test is built three times, and that
  wasi reads `WEBCPP_NO_EXCEPTIONS` itself until its redesign; chapter 2's
  `build.jam` example gains `<library>/webcpp/config//config`; chapter 13's
  item keeps only "trystero (held with config's publication), and wasi with
  its redesign".
- [ ] **Step 7: Run everything and watch it pass,** from scratch, in
  parallel: each of the four libraries' lanes (native clang, gcc-15, wasip2,
  wasip3, and emscripten for pratt), each XML through `report.py` (exit 0);
  wasi's own lanes `http` on wasip2 and wasip3; xstate's oracle lane; `B2 -a
  doc`; the lint; every `tools/**/*_test.py`. Plant one defect and undo it:
  drop `<library>/webcpp/config//config` from xactor's `build.jam` (xactor's
  lanes fail: `webcpp/config.hpp` not found). wasi's by-hand build
  (`B2 toolset=clang-wasip2 testing.launcher=wasmtime libs/wasi/example//http`)
  stays green without `libs/config` on its include path.
- [ ] **Step 8: Commit,** without integrating: in each library, by path,
  "<name>: config.hpp includes <webcpp/config.hpp>, and WEBCPP_NO_EXCEPTIONS
  disables its exceptions" (wasi's: "wasi: WEBCPP_NO_EXCEPTIONS disables its
  exceptions too"); in `W` "AGENTS.md: every library's config.hpp includes
  <webcpp/config.hpp> first". Record each commit in the ledger as held for
  Task 15.

### Task 14: trystero on `<webcpp/config.hpp>` (held until Task 15)

Runs after Task 12, like Task 13, on trystero's branch `config-adoption`
(`git -C ~/dev/webcpp-config/libs/trystero switch -c config-adoption main`).

**Files:** `include/webcpp/trystero/config.hpp`, `build.jam`, `test/Jamfile`,
`test/config_test.cpp`, `doc/changelog.adoc`, and the texts that
`grep -rn 'NO_EXCEPTIONS\|-I libs/' README.md AGENTS.md doc` names.

**Interfaces:** Task 13's for xactor, with `TRYSTERO` for `XACTOR`: the
include, the condition, `<library>/webcpp/config//config` on the alias
`trystero` (which `native` reaches through it), `config_all_off` and its
`#error`; the native headers' `#ifdef ... #error` guards keep naming
libdatachannel, with `WEBCPP_TRYSTERO_NO_EXCEPTIONS` as they read after
stage 4.

- [ ] **Step 1:** the failing `config_all_off` (as Task 13, Step 3).
- [ ] **Step 2:** implement, and the two lines of Task 13, Step 5, for
  trystero; run trystero's lanes (native, emscripten), its own lanes
  `oracle` and `interop`, `B2 -a doc`, the lint.
- [ ] **Step 3: Commit,** held for Task 15: "trystero: config.hpp includes
  <webcpp/config.hpp>, and WEBCPP_NO_EXCEPTIONS disables its exceptions".

### Task 15: Publish (controller, with the owner's go)

Only once the owner says to create `webcpporg/config`. Runners are scarce:
one repository's CI at a time.

- [ ] **Step 1: The repository.** `gh repo create webcpporg/config --public
  --description "webcpp's configuration: the target, exceptions and the
  version every webcpp library includes first"`; `git -C
  ~/dev/webcpp/libs/config remote add origin
  git@github.com:webcpporg/config.git && git -C ~/dev/webcpp/libs/config push
  -u origin main`. Its first CI run, against a superproject without it, is
  cancelled (`gh run list --repo webcpporg/config --limit 1 --json
  databaseId`, `gh run cancel <id> --repo webcpporg/config`).
- [ ] **Step 2: The submodule.** In `M`: `git submodule add ../config.git
  libs/config`, which stages the repository already at that path in place
  (its worktree `W/libs/config` stays valid); check that `.gitmodules` names
  `../config.git` and that `libs/config` is at the pushed commit. AGENTS.md
  chapter 1's config row becomes "a submodule at `libs/config`". Commit by
  path (`.gitmodules libs/config AGENTS.md`): "libs/config: the submodule".
- [ ] **Step 3: The held commits.** Rebase each `config-adoption` branch
  (Tasks 13 and 14, and `W`'s) onto its `main`, fast-forward each `main`,
  and run Task 13's Step 7 and Task 14's Step 2 in `M`, from scratch.
  Expected: green.
- [ ] **Step 4: The pointers.** `git -C ~/dev/webcpp add libs/xactor
  libs/xstate libs/pratt libs/wasi libs/trystero`, commit "libs: every
  library on <webcpp/config.hpp>, with its changelog".
- [ ] **Step 5: Push,** in order: config (done), xactor, xstate, pratt, wasi,
  trystero (cancel each library's run against the old superproject), then
  the superproject. Watch its CI until green: every native lane, wasip2,
  wasip3, emscripten, the docs, the lint shards (the rule `changelog`
  among them), the tools job (`changelog_test.py`, `release_test.py`), every
  own lane, the site (its link check finds config's page) and the report.
  Fix what fails in the right repository, with a test.
- [ ] **Step 6: Each library's CI,** config first, one at a time:
  `gh workflow run ci.yml --repo webcpporg/<name> --ref main`, watched until
  green before the next.
- [ ] **Step 7: Open the published Pages:** the index lists config, its page
  shows the reference's eight macros, every page ends with its changelog, and
  the index ends with "Releases", webcpp 0.1.0, in preparation. Both ledgers:
  the hold on pushing is lifted. Ledger the track complete; no release is
  made.

---

## Review disposition

The independent review (Critical 1, Important 10, Minor 16) and the
controller's rulings of 2026-10-09: **27 accepted, 0 declined.**

| Item | Disposition |
| --- | --- |
| C1 | Accepted: integration (Tasks 11, 12) only after stage 4's Task 14; nobody pushes `M` or a library from Task 12 to Task 15, in both ledgers; Departure 7 |
| I1 | Accepted: `unsupported_wasip1` and `unsupported_wasm`, the `--target` order measured first, the `__wasm32__x` plant, the target block moved before Boost.Config, the Doc Comment, the page and Review Focus 3 |
| I2 | Accepted, one step further than the ruling: the 0.1.0 notes include no library's region, so only the doc-check change waits; it moved from Task 4 into Task 11, which runs after stage 4 has published, against its Task 11's final code; one helper, `repository_of` and `tracked_in`, for both rules; Task 4 says so, and the conflicts table says "certainly" |
| I3, I4, I5 | Accepted as C1 resolves them: Task 0 enters the four published libraries only; trystero enters at Task 11, complete and published, its changelog with the rest; Task 14 is its `config.hpp` move alone, held; the alternative path through stage 4 is gone |
| I6 | Accepted: every test and every tool is real code (`changelog.py`, `release.py`, `scratch.py`, the doc-check methods, the lint rule); `test_jamroot_targets` writes its own index and notes |
| I7 | Accepted: the old Task 5 is now Tasks 5 (form), 6 (version and Jamroot), 7 (check), 8 (whats-new), 9 (dist), each with its own red, green and commit |
| I8 | Accepted: the lint rule `changelog`, `changelog_test.py` with wrapped lines (a continuation, one that starts with a verb, eleven wrapped), a read-only test of the real files, Review Focus 4 |
| I9 | Accepted in its second form: `test_jamroot_without_config` runs `b2 doc` and pins that it builds; a doc-check rule on the index's library links would turn every tools test red, since their scratch copies hold the real notes and no library; assemble.py's link check is the net in CI, and C1's hold keeps an unpublished config out of it |
| I10 | Accepted: stub headers first, each test's own message listed; plants for `exceptions_off`, both `predefined_*`, the exactly-one count and `__wasm__` |
| M1 | Accepted: Departure 6, for the owner |
| M2 | Accepted as a decision: Tools and build lists what changes for whoever builds, lints or tests with the superproject (lint and CI lines kept); a library's internal changes get none |
| M3 | Accepted: Task 13 lists every hit; `test/CONVERSION.md` records history and is not edited |
| M4 | Accepted: `scratch.GIT` sets identity, signing off and file protocol |
| M5 | Accepted: `libraries.py` refuses `has`, `lib`, `no`, `target`, `version`, with a test; AGENTS chapter 2 |
| M6 | Accepted: every commit stages its files by path |
| M7 | Accepted: `worktree add --detach` for `W/libs/config` |
| M8 | Accepted: the doc.jam references name rules `convert` and `doc-check` |
| M9 | Accepted: `path.native [ path.join [ path.make $(WEBCPP_ROOT) ] ... ]` |
| M10 | Accepted: no "Task N" in test text |
| M11 | Accepted: `whats_new_findings` in `check`, with a test |
| M12 | Accepted: `/.local` in `W`'s `info/exclude`, checked in Task 0 |
| M13 | Accepted: `changed` diffs the two recorded gitlinks |
| M14 | Accepted: Task 15, Step 4 commits the pointers |
| M15 | Accepted: chapter 3's sentence names config's include guards |
| M16 | Accepted: the Jamroot comment says no Windows lane runs `define-line` for `WEBCPP_VERSION` |
| (e) | Accepted: `LICENSE*` in `USER_VISIBLE`, with a test |

Found while revising, beyond the review: config's `version.hpp` moves at
every release and would have made config "changed" each time, so
`USER_VISIBLE` leaves `include/webcpp/version.hpp` out (Departure 5).
