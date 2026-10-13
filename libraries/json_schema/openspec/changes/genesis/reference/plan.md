---
title: "webcpp::json_schema: implementation plan"
tags: [webcpp, json-schema, plan]
status: active
created: 2026-10-09
---

# webcpp::json_schema Implementation Plan

> **This copy, in the OpenSpec change `json-schema`.** The plan as written
> (2026-10-09, with its revisions up to 2026-10-11), every task with its
> code. What differs from the original:
>
> - the "superseded" banner is removed: this proposal revives the library;
> - **Task E1.4 is rewritten** for Boost.Regex through webcpp::lexical'
>   ECMA-262 translator, since the owner abandoned webcpp::regex and SRELL
>   on 2026-10-12; its first version, done over webcpp::regex, is described
>   at the head of that section and kept in `source/`;
> - the commit identity is the superproject's current one (its AGENTS.md,
>   chapter 11), and paths of the original machine are written under `~/`.
>
> Paths under `~/` describe the layout of the machine the plan was written
> on: `~/dev/webcpp` the superproject, `~/dev/webcpp-repos/json_schema` the
> library's canonical local repository, `~/.buzz/PLANS` and
> `~/.buzz/RESEARCH` the owner's planning notes, which are not published;
> `STATUS.md` says how to recreate what a resumed implementation needs.
> Outside E1.4, where this plan names SRELL, webcpp::regex, the srell mirror
> or `/webcpp/regex//regex` (the Tech Stack, the shared item S3, the
> `superproject_worktree` function, E1.1's `build.jam` and `AGENTS.md`, the
> include boundaries, the risks), `design.md`'s decision "Regular
> expressions" supersedes it; `tasks.md`'s group 0 and task 1.4 say what
> changes.
>
> **Sections and `tasks.md`.** Group 0 of `tasks.md` (bring the snapshot up
> to the superproject's current rules) has no section here; `STATUS.md` holds
> its detail. Then each task of `tasks.md` is the section of the same name:
>
> | `tasks.md` | Section here |
> | --- | --- |
> | 0.9 | Task 0: The repository, the E1 worktree, the licences |
> | 1.1 to 1.17 | Task E1.1 to Task E1.17 |
> | 2.1 to 2.11 | Task E2.1 to Task E2.11 |
> | 3.1 to 3.4 | Task E3.1 to Task E3.4 |
> | 4.1 to 4.4 | Task E4.1 to Task E4.4 |
> | 5.0a, 5.0b, 5.1 to 5.20 | Task E5.0a, Task E5.0b, Task E5.1 to Task E5.20 |
> | 6.1 | Task P: Publication |
>
> The sections "Global Constraints", "Review Focus", "File structure",
> "Shared items", "Parallel map", "Decisions this plan makes for the owner",
> "What the owner must decide" and "Review disposition" apply to every task.

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build `webcpp::json_schema`, a header-only C++20 JSON Schema
validator for draft 4, 6, 7, 2019-09, 2020-12 and OpenAPI 3.0, proven by the
official JSON-Schema-Test-Suite on native, wasip2, wasip3 and emscripten.
- Its canonical repository is the local `~/dev/webcpp-repos/json_schema`.
- Each sub-track's worktree clones it at `libs/json_schema`.
- It is ready to publish as `webcpporg/json_schema`.

**Architecture:**
- The pipeline is `boost::json -> load -> IR -> compile -> plan -> validate ->
  events -> output`, and `IR -> emit(dialect)` (spec 1, S1).
- The interface between the sub-tracks is a set of frozen files, reviewed
  and tagged `contract-1` at the end of E1 (Task E1.17):
  - the IR;
  - the evaluation sink and its protocol;
  - the record the collecting sink fills;
  - the outcome;
  - the message hook.
- **E1** lays down everything under that interface:
  - the skeleton, exact numbers, regex, exact parsing, the IR;
  - the 2020-12 loader without references;
  - the compiler and the VM for the core keywords, with `is_valid` and a
    `validate` that runs the plan at most twice;
  - the English messages and the four output formats, which belong to the
    frozen outcome;
  - the suite runner, the allocation and hostile-input proofs, and the first
    page.
- **E2, E3 and E4** then run in parallel worktrees:
  - E2 completes the compiler and the VM: references, meta-schemas,
    annotations, `unevaluated*`;
  - E3 writes the message catalogue, the Boost.Locale header and the proofs
    of output and messages;
  - E4 writes the emitter and `ir::equivalent`, which agree by construction.
- **E5** starts with a serial base (every dialect's machinery), then:
  - the dialects, formats, user vocabularies, fuzzing and Bowtie, in
    parallel;
  - then content, the optimizations, the benchmark and the conformance
    documentation.

**Tech Stack:**
- C++20, b2 5.5.3, Boost 1.92: JSON through `/webcpp//boost_json`, System,
  Decimal, StaticString, Container, Compat, Unordered, Core lightweight_test;
  Boost.URL through `/webcpp//boost_url` (S4); Boost.Locale and
  Boost.Charconv through `/webcpp//boost_locale` and `/webcpp//boost_charconv`
  (S4b), for the optional header only.
- SRELL 2026.07 from the local mirror (S3), with its no-exceptions and
  allocator commits.
- The JSON-Schema-Test-Suite at `5b0ee1613e45fcc2bddac00e07c19cd49b00d8a8`
  (S2).
- `webcpp::lexical` and `webcpp::idna`, for E5.5 only.
- wasi-sdk 34, wasmtime 47.0.3, Emscripten 6.0.11, MrDocs 2026.9.29,
  Asciidoctor.js, Python 3.9 or newer.
- E5's own lanes: libFuzzer (Linux clang 18), Docker with Bowtie,
  sourcemeta-research/jsonschema-benchmark.

**Spec:** `spec.md` beside this file (originally `~/.buzz/PLANS/WEBCPP_JSON_SCHEMA_SPEC.md`) (approved
2026-10-09, with its section 10 edits of the same day). The plan never edits
it: section "Decisions this plan makes for the owner" holds two proposed
amendments (A1, A2) and the points this plan settles.

Other sources:
- The design record: `design-source.md` beside this file (originally `~/.buzz/PLANS/JSON_SCHEMA_DESIGN.md`).
- The controller's rulings of 2026-10-09: `WEBCPP_DESIGN.md`, the last
  entries of "Owner decisions after approval" (where an unpublished library
  lives; deep input; numbers; `validate`; the freeze; S4b; E5's base).
- The two reviews this revision answers:
  `~/dev/webcpp/.superpowers/sdd/plan-reviews/WEBCPP_JSON_SCHEMA_PLAN-E1-review.md`
  and `.../WEBCPP_JSON_SCHEMA_PLAN-E2E5-review.md`. Section "Review
  disposition" answers each finding.
- The shared items, `~/.buzz/PLANS/WEBCPP_SHARED_INFRA_PLAN.md`,
  its section "Consumer interface". This plan copies those lines as the
  revision of 2026-10-09 04:04 states them (checked again at writing). The
  controller checks them once more before dispatch, if the infra plan
  changes again. Its tables still name this plan's earlier task numbers:
  S4b serves E3.2 here, not E3.4.
- `~/.buzz/PLANS/WEBCPP_LEXICAL_SPEC.md`,
  `WEBCPP_LEXICAL_PLAN.md`, `WEBCPP_IDNA_SPEC.md`.
- The rulebook `~/dev/webcpp/AGENTS.md`, and the models `libs/pratt` (layout,
  errors, config, the allocation test, depth measured) and `libs/xstate`.

## Global Constraints

**Where everything lives**
- **The canonical repository is `~/dev/webcpp-repos/json_schema`**
  (controller ruling, 2026-10-09).
  - Nothing that is not a registered submodule ever lives under
    `~/dev/webcpp/libs/`: the Jamroot's `glob libs/*/build.jam` and the lint
    would read it.
  - json_schema enters `~/dev/webcpp/libs/` only as a submodule, in Task P.
  - `test ! -e ~/dev/webcpp/libs/json_schema && echo absent` prints `absent`
    at every step before Task P.
- **Task 0 creates the canonical repository empty**, with `git init -b
  main`.
  - Every worktree, E1's included, clones it at `libs/json_schema` and works
    on the library branch `e<N>` (a sub-track) or `e<N>-<slug>` (a task that
    runs alongside others).
  - Nobody works in the canonical repository directly. The controller
    fast-forwards its `main` from `e1` at E1's end (Task E1.17), then merges
    each later sub-track, one at a time, each rebased on the last (spec 20.3).
- **A clone's `origin`** points at the canonical repository or a track's
  clone. That is the one kind of remote allowed; no other remote is ever
  added.
- **The superproject branch** of each worktree is based on the infra track's
  `infra/shared` (S1 to S5), never on `main`, until the controller
  integrates the infra track into `main`. Its submodules come from local
  clones only (`superproject_worktree` below).
  - After that integration, the controller sets `SUPERPROJECT_BASE=main` in
    the ledger, and every later rebase uses it.
  - Nothing of S3 reaches `main` before the owner creates `webcpporg/srell`.
- **Outward actions:** none until Task P. No push, no GitHub, no Bowtie pull
  request.

**Commits and text**
- **The machine's global git identity never reaches a webcpp commit, as
  author or as committer** (ruling recorded in WEBCPP_DESIGN.md).
  - Every git command that writes a commit or a tag runs as `git -c
    user.name=R -c user.email=github.overboard312@passmail.net ...`, written `$G`
    below: commit, rebase, cherry-pick, a merge that is not a fast-forward,
    am, amend, and `tag -a`.
  - Before each integration (`merge_task`, `merge_track`, Task E1.17's
    fast-forward, Task P), `ids_ok` checks `git log --format='%an %ae %cn
    %ce'` over the range to be integrated. Every line must read `R
    github.overboard312@passmail.net R github.overboard312@passmail.net`.
- Commit with `$G commit -m "<subject>" -- <paths>`.
  - No trailer, never an agent co-author.
  - A body paragraph never starts with `word:`.
- **Stage by path.** `git add <file> ...` names every file; a directory is
  never staged. Never `git add -A`, never `git add libs`, never `git add -u`
  of a directory. Each task's commit step lists its files.
- English only. Never the em dash (U+2014); use a plain dash. No file, file
  name or commit holds the word `veru[s]` matches, in any case.
- **No "stage N", "Task N", sub-track name (E1 to E5) or S label in a
  tracked file or a commit message.**
  - The one exception: the known-failure reasons spec 16.1 names
    (`E2: references`).
  - Code comments say what is true now ("measured on wasip2 at -O0; see
    Limits"), never which task does what.
- Every new source file (`.hpp .cpp .py .sh .jam Jamfile build.jam .yml .adoc
  .html`, a Dockerfile) opens with the notice, `//` for C++ and AsciiDoc, `#`
  for the rest, a `#!` line kept first:

  ```
  // Copyright (c) 2026 WebCpp.org
  //
  // Distributed under the Boost Software License, Version 1.0. (See
  // accompanying file LICENSE_1_0.txt or copy at
  // https://www.boost.org/LICENSE_1_0.txt)
  ```

  The code blocks below leave the notice out for brevity; every file still
  opens with it.
- Python lines stay within 100 columns, and Jam comments within 80.

**Running b2, and the shell functions every task uses** (defined once per
shell, from a worktree's root):

```bash
B2="env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH b2 --user-config=.local/user-config.jam"
G="git -c user.name=R -c user.email=github.overboard312@passmail.net"
CANONICAL=~/dev/webcpp-repos/json_schema
SUPERPROJECT_BASE=${SUPERPROJECT_BASE:-infra/shared}
export EM_CACHE=~/dev/webcpp/.local/emscripten-cache
FROZEN="include/webcpp/json_schema/ir/ids.hpp include/webcpp/json_schema/ir/number.hpp \
include/webcpp/json_schema/ir/keyword.hpp include/webcpp/json_schema/ir/node.hpp \
include/webcpp/json_schema/ir/document.hpp include/webcpp/json_schema/detail/vm/event.hpp \
include/webcpp/json_schema/detail/vm/sink.hpp include/webcpp/json_schema/detail/output/record.hpp \
include/webcpp/json_schema/output/error.hpp include/webcpp/json_schema/output/outcome.hpp \
include/webcpp/json_schema/output/message_hook.hpp"

ids_ok() {       # ids_ok <repository> <range>: every commit of the range is R's, both identities
  local bad
  bad=$(git -C "$1" log --format='%an %ae %cn %ce' "$2" \
    | grep -vx 'R github.overboard312@passmail.net R github.overboard312@passmail.net')
  [ -z "$bad" ] || { echo "ids_ok: another identity in $2:"; echo "$bad"; return 1; }
}

lint_clean() {   # the lint, its last line checked
  tools/lint/lint.sh --clang-format .local/wasi-sdk/bin/clang-format \
    --clang-tidy .local/wasi-sdk/bin/clang-tidy > /tmp/json_schema-lint.log 2>&1
  tail -1 /tmp/json_schema-lint.log | grep -qx 'lint: clean' && echo 'lint: clean'
}

lanes() {   # lanes <b2 targets...>: the five lanes, from scratch, in parallel
  mkdir -p bin
  local pids=() opts=() lane pid status=0
  for lane in clang gcc-15 wasip2 wasip3 emscripten; do
    case $lane in
      clang) opts=() ;;
      gcc-15) opts=(toolset=gcc-15) ;;
      wasip2) opts=(toolset=clang-wasip2 testing.launcher=wasmtime) ;;
      wasip3) opts=(toolset=clang-wasip3 testing.launcher=wasmtime) ;;
      emscripten) opts=(toolset=emscripten) ;;
    esac
    $B2 -a --build-dir=bin/lane-$lane "${opts[@]}" "$@" > bin/lane-$lane.log 2>&1 &
    pids+=($!)
  done
  for pid in "${pids[@]}"; do wait "$pid" || status=1; done
  if grep -l '^\.\.\.failed' bin/lane-*.log; then status=1; fi
  if [ $status = 0 ]; then echo "lanes: green"; else echo "lanes: red"; fi
  return $status
}

superproject_worktree() {   # superproject_worktree <path> <branch>, the infra plan's lines
  git -C ~/dev/webcpp worktree add "$1" -b "$2" "$SUPERPROJECT_BASE" || return 1
  git -C "$1" -c protocol.file.allow=always \
    -c submodule.third_party/libs/JSON-Schema-Test-Suite.url="$HOME/.buzz/REPOS/JSON-Schema-Test-Suite" \
    -c submodule.third_party/libs/srell.url="$HOME/.buzz/REPOS/srell.git" \
    submodule update --init third_party/libs/JSON-Schema-Test-Suite third_party/libs/srell || return 1
  git -C "$1" submodule status third_party/libs
  ln -s ~/dev/webcpp/.local "$1/.local"
}

track_worktree() {   # track_worktree <N>: the worktree of sub-track e<N>
  superproject_worktree ~/dev/webcpp-json-schema-e"$1" lib/json_schema-e"$1" || return 1
  git clone "$CANONICAL" ~/dev/webcpp-json-schema-e"$1"/libs/json_schema || return 1
  git -C ~/dev/webcpp-json-schema-e"$1"/libs/json_schema checkout -b "e$1"
}

task_worktree() {    # task_worktree <N> <slug>: a task that runs alongside others in e<N>
  local w=~/dev/webcpp-json-schema-e"$1"-"$2"
  superproject_worktree "$w" lib/json_schema-e"$1"-"$2" || return 1
  git clone "$CANONICAL" "$w/libs/json_schema" || return 1
  git -C "$w/libs/json_schema" fetch ~/dev/webcpp-json-schema-e"$1"/libs/json_schema "e$1" || return 1
  git -C "$w/libs/json_schema" checkout -b "e$1-$2" FETCH_HEAD
}

merge_task() {       # merge_task <N> <slug>, from the worktree of e<N>
  local w=~/dev/webcpp-json-schema-e"$1"-"$2"
  git -C "$w/libs/json_schema" fetch "$PWD/libs/json_schema" "e$1" || return 1
  $G -C "$w/libs/json_schema" rebase FETCH_HEAD || return 1
  (cd "$w" && lanes libs/json_schema/test && lint_clean) || return 1
  git -C libs/json_schema fetch "$w/libs/json_schema" "e$1-$2" || return 1
  ids_ok libs/json_schema "HEAD..FETCH_HEAD" || return 1
  $G -C libs/json_schema merge --ff-only FETCH_HEAD || return 1
  git -C ~/dev/webcpp worktree remove "$w" && git -C ~/dev/webcpp branch -D lib/json_schema-e"$1"-"$2"
}

merge_track() {      # merge_track <N>, from the worktree of e<N>: the controller's integration
  git -C libs/json_schema fetch "$CANONICAL" main || return 1
  $G -C libs/json_schema rebase FETCH_HEAD || return 1
  if ! git -C libs/json_schema diff --exit-code --stat contract-1 HEAD -- $FROZEN; then
    [ "$FROZEN_CHANGE_APPROVED" = yes ] || {
      echo "merge_track: a frozen file changed; the ledger must approve it first"; return 1; }
  fi
  $G rebase "$SUPERPROJECT_BASE" || return 1
  lanes libs/json_schema/test libs/json_schema/example || return 1
  $B2 -a --build-dir=bin/doc libs/json_schema/doc > bin/doc.log 2>&1 || return 1
  lint_clean || return 1
  git -C "$CANONICAL" fetch "$PWD/libs/json_schema" "e$1" || return 1
  ids_ok "$CANONICAL" "main..FETCH_HEAD" || return 1
  $G -C "$CANONICAL" merge --ff-only FETCH_HEAD
}

record_known() {     # record_known <runner>: its known-failure list, from a native run
  $B2 -a --build-dir=bin/record "libs/json_schema/test//suite-$1" > /dev/null 2>&1
  local runner
  runner=$(find bin/record -path '*libs/json_schema/test*' -name "suite-$1" -type f -perm -u+x \
    | sort | head -1)
  [ -n "$runner" ] || { echo "record_known: no native suite-$1 under bin/record"; return 1; }
  "$runner" --record > "/tmp/known-$1.json" || return 1
  python3 -m json.tool --indent 4 "/tmp/known-$1.json" > "libs/json_schema/test/known_failures/$1.json"
}
```

- `lanes` prints `lanes: green` and returns 0, or names the red logs and
  returns 1. Never give `testing.launcher` on emscripten. At most one doc
  build at a time.
- `lint_clean` prints `lint: clean` and returns 0, or prints nothing and
  returns 1; the log is `/tmp/json_schema-lint.log`.
- `merge_task` and `merge_track` call `ids_ok` before they fast-forward, and
  write every rebase and merge through `$G`.
- `merge_track` prints `lanes: green`, then `lint: clean`, then git's
  fast-forward. It stops before the fast-forward when a frozen file changed.
  - The controller sets `FROZEN_CHANGE_APPROVED=yes` only after the ledger
    records the change as allowed by spec 5.5: a value appended to
    `keyword_kind`, `errc` or `error_kind`, or a contract change already
    applied to every open branch.
  - Then the controller unsets it.
- `record_known` exits 1, after printing each `not recordable:` test, when a
  failure has no recorded reason. That is a failure the current task fixes.
- **Shared files of parallel tasks:** `test/Jamfile`,
  `meta/include-boundaries.json`, `include/webcpp/json_schema.hpp`, and
  `test/suite/runner.hpp` from E3 on.
  - Each task adds its own lines in its own block:
    - one `webcpp.headers-alone` call per new header directory;
    - one boundary object per new header directory;
    - one block of `#include`s in the convenience header, in alphabetical
      order of directory.
  - A rebase conflict in these files is resolved by keeping both sides. Then
    the task's tests, the lanes and the lint run again.

**Targets.** `native wasip2 wasip3 emscripten`, declared by every test and
example Jamfile. The exceptions, each stated where it applies:
- the threads test is native only;
- the locale header and its test run natively, on wasip2 and on wasip3 (S4b
  refuses emscripten);
- the fuzz lane is Linux clang 18 only;
- the Bowtie lane needs Docker.

**C++ rules** (AGENTS.md chapter 6, spec S11):
- Header-only C++20 in `webcpp::json_schema`, with `detail` for internals.
- Macros are `WEBCPP_JSON_SCHEMA_*`, test macros `WEBCPP_TEST_*`, and
  include guards `WEBCPP_JSON_SCHEMA_<PATH>_HPP`.
- Errors as values: `result<T>` is `boost::system::result<T, diagnostic>`.
  The category is `webcpp.json_schema`, with the fixed identity
  `0x6a736368656d6100`.
- No global or static mutable state, and no clock, file, network, thread,
  environment or entropy in a library header.
- Guards first. Text is held as `std::string_view`. Boost and std come before
  custom code.
- **No bare `throw` in `include/`.** Every raise goes through
  `boost::throw_exception`, and every throwing API sits behind `#ifndef
  WEBCPP_JSON_SCHEMA_NO_EXCEPTIONS`. No `.value()` on a result inside the
  library.
- **Deep input** (ruling 2026-10-09). json_schema never recurses on a JSON
  value:
  - every walk is iterative: loader, compiler, VM, equality, hashing, copies,
    renderers, emitter, `equivalent`, `check`;
  - a value is copied only by `detail::deep_copy`, iteratively, never by
    Boost.JSON's copy constructor, and a keyword's source refers into the
    document's one copy;
  - values json_schema owns live in a trivially deallocating arena, so their
    destruction never recurses;
  - `js::parse` bounds Boost.JSON's recursive parser with
    `parse_options::max_depth`, clamped to `detail::max_parse_depth`, which
    is measured safe on every target. Text deeper than that is
    `boost::json::error::too_deep`, a value.
- **Numbers** (ruling 2026-10-09). Every comparison between an instance
  number and a schema number, and `multipleOf`, is exact:
  - `int64_t` and `uint64_t` are compared exactly;
  - a `double` is never rounded to `Number`: it is read once as the exact
    decimal its shortest round-trip text denotes (17 significant digits at
    most, in `decomposed`'s 128-bit coefficient);
  - the ruling's words "a double by its exact binary value" and the spec's
    words "converted once to `Number`" both differ from this. Section
    "Decisions", amendment A1, gives the evidence and the proposed wording,
    and the item is open.
- **Two allocators** (spec 14): the schema's and the validation's. Nothing
  allocates outside them, policies included, and `is_valid` allocates
  nothing on the common path: an instance nesting at most 32 levels and a
  string of at most 64 bytes against a pattern.
- **clang-tidy, as `.clang-tidy` configures it:**
  - every aggregate of two or more members is initialized with designated
    initializers;
  - arrays are `std::array`;
  - no function goes over a cognitive complexity of 25 (the VM's dispatch is
    one member function per opcode family);
  - a test's helpers live in an anonymous namespace, with every `#include`
    at the top;
  - a replaced `operator new` or `operator delete` follows pratt's
    `allocation_test.cpp` (its `NOLINT`s, `BOOST_NOINLINE`, the
    `align_val_t` forms).

**Fixed values.** These never change and are never reused; appending is
allowed:
- `errc` 1 to 20 (spec 4.1);
- `dialect` 1 to 6 (spec 4.3);
- `error_kind` 1 to 43 (spec 4.6);
- `keyword_kind` as spec 5.2's table;
- the opcodes, from 0 upward, as each task appends them.

**The freeze** (spec 5.5, ruling 2026-10-09). The files of `$FROZEN` above
freeze at Task E1.17:
- `ir/ids.hpp`, `ir/number.hpp`, `ir/keyword.hpp`, `ir/node.hpp`,
  `ir/document.hpp`;
- `detail/vm/event.hpp`, `detail/vm/sink.hpp` (with the protocol P1 to P6);
- `detail/output/record.hpp`;
- `output/error.hpp`, `output/outcome.hpp`;
- `output/message_hook.hpp`.

After the freeze, a change to any of them follows spec 5.5, and
`merge_track` enforces it.

**Compiled libraries.** The URL policy links `/webcpp//boost_url` (S4), which
replaces spec 3's `/boost//url` by the owner's decision of 2026-10-09. The
locale header links `/webcpp//boost_locale` (S4b).

**Licences** (spec 21.7; Task 0 re-checks them):
- the suite (MIT) is test data read at run time, never copied into the
  repository: no corpus, fixture or golden holds suite text;
- the official meta-schemas are "BSD 3-clause *or* AFL-3.0";
- the OpenAPI 3.0 schema is Apache-2.0;
- `sourcemeta-research/jsonschema-benchmark` has no licence file at
  `724607b`;
- Blaze is AGPL, run and never linked or copied (D7); ajv is MIT;
- Bowtie is the owner's pin.

Section "What the owner must decide" lists what waits for the owner.

**Reports.** Every task's report ends with a section `Changelog`, in Boost's
release-notes form. Until json_schema is first published, it holds this one
line:

```
New Libraries:
- json_schema: a header-only JSON Schema validator for C++20, every dialect from draft 4 to
  2020-12 and OpenAPI 3.0, proven by the official test suite, from R.
```

The controller keeps the lines in the json_schema ledger. Once the config
plan's integration has created the changelog files, the controller writes
them where its rule says (spec 19).

## Review Focus

1. **`$schema` as users spell it.** Each official URI is accepted with and
   without its empty fragment:
   - `http://json-schema.org/draft-07/schema#`, and the same without `#`;
   - `https://json-schema.org/draft/2020-12/schema`, and the same with `#`.

   Anything else is refused by name: `unknown_dialect`, with `what` holding
   the URI as written (draft 7 under `https`, a trailing slash, draft 3).
   Pinned in E1.9 (`schema_uri_spellings`), and in E5.0a and E5.0b for the
   other dialects.
2. **Numbers at the edges of their representations.**
   - The values to cover:
     - the double 2^64 against `maximum: 18446744073709551615`;
     - `0.1 + 0.2` against `const: 0.3`, and `[0.30000000000000004, 0.3]`
       against `uniqueItems`;
     - `-0.0` against `minimum: 0`;
     - `1.0` against `const: 1`;
     - `1e308` against `multipleOf: 0.0001`;
     - `1.1` against `exclusiveMinimum: 1.1`;
     - a `maximum` beyond `decimal64_t`'s range.
   - Expected: exact verdicts, and `invalid_number` only for the last.
   - Pinned in E1.2 (`numbers_test`) and E1.11 (`vm_test`).
3. **Property names that are not plain identifiers:** `""`, `"a/b"`, `"~1"`,
   a name with a NUL byte, a non-BMP name. Every location is a well-formed
   JSON Pointer, and lengths count code points. Pinned in E1.3
   (`pointer_test`) and E1.12 (`validate_test`).
4. **Temporaries and lifetimes:**
   - `compile(boost::json::parse(...))` from a temporary;
   - an outcome kept after its `basic_schema` handle;
   - a schema in a monotonic resource released after the last outcome.

   Pinned in E1.12 (`lifetime_test`).
5. **One compiled schema shared between threads.** Eight threads validate
   with `is_valid` and `validate`, each with its own resource, and the
   verdicts are identical. Pinned in E1.12 (`threads_test`, native), and run
   under ThreadSanitizer in E5.7.

---

## File structure

```
~/dev/webcpp-repos/json_schema/            canonical repository; cloned at libs/json_schema
  build.jam  meta/libraries.json  meta/include-boundaries.json
  README.md AGENTS.md LICENSE_1_0.txt .gitignore .gitattributes .github/workflows/ci.yml
  LICENSE-JSON-SCHEMA.txt LICENSE-OPENAPI.txt                       E2.2, E5.0b (decision 1)
  include/webcpp/json_schema.hpp                                    every public header but locale.hpp
  include/webcpp/json_schema/
    config.hpp errors.hpp                                           E1.1
    traits.hpp json/parse.hpp dialects/dialect.hpp                  E1.6
    numbers/number.hpp numbers/decimal.hpp                          E1.2
    regex/regex_policy.hpp regex/webcpp_regex.hpp                   E1.4
    url/url_policy.hpp url/boost_url.hpp                            E2.1
    ir/ids.hpp ir/number.hpp ir/keyword.hpp ir/node.hpp ir/document.hpp   E1.2, E1.7 (frozen)
    ir/equivalent.hpp                                               E4.1
    registry/registry.hpp registry/resolver.hpp                     E1.8, E2.3
    vocabulary/vocabulary.hpp formats/format_set.hpp                E1.8
    formats/default_formats.hpp                                     E5.5
    validator/options.hpp validator/validator.hpp                   E1.8 to E1.10
    validator/schema.hpp                                            E1.11, E1.12, E5.12
    output/error.hpp                                                E1.5 (frozen)
    output/message_hook.hpp                                         E1.8 (frozen)
    output/outcome.hpp                                              E1.12 (frozen)
    output/messages.hpp                                             E1.13
    output/locale.hpp                                               E3.2
    emitter/emit.hpp                                                E4.2
    detail/memory/      resource.hpp inline_stack.hpp                         E1.1
    detail/numbers/     arithmetic.hpp decompose.hpp compare.hpp multiple_of.hpp   E1.2
    detail/json/        json_resource.hpp value_arena.hpp deep_copy.hpp utf8.hpp pointer.hpp
                        equal.hpp exact_handler.hpp (E1.3, E1.6), hash.hpp (E5.12)
    detail/vm/          event.hpp sink.hpp (frozen) flag_sink.hpp collecting_sink.hpp (E1.5),
                        machine.hpp (E1.11)
    detail/output/      record.hpp (E1.5, frozen) render.hpp units.hpp (E1.13)
    detail/dialects/    keyword_table.hpp draft2020_12.hpp (E1.9), rule.hpp draft2019_09.hpp
                        draft7.hpp draft6.hpp (E5.0a), draft4.hpp openapi3_0.hpp (E5.0b)
    detail/loader/      walk.hpp readers.hpp (E1.9), resources.hpp resolve.hpp (E2.3),
                        meta_check.hpp (E2.5)
    detail/meta_schemas/  generated headers (E2.2, E5.0b)
    detail/compiler/    opcode.hpp plan.hpp compile.hpp (E1.10), cycles.hpp (E2.4),
                        evaluated.hpp (E2.9), optimize.hpp explain.hpp (E5.12 to E5.14)
    detail/emitter/     reach.hpp writer.hpp (E4.2), forms.hpp (E4.3)
    detail/content/     base64.hpp (E5.9)
  meta_schemas/  *.json README.md generate.py                       E2.2, E5.0b
  tools/  Jamfile pot.hpp make_pot.cpp                              E3.1
  i18n/webcpp_json_schema.pot                                       E3.1
  test/  Jamfile require.hpp counting_resource.hpp *_test.cpp       every task
  test/suite/  runner.hpp suite_*.cpp                               E1.14 on
  test/known_failures/<runner>.json                                 E1.14 on
  test/measure/  Jamfile *_probe.cpp                                E1.15
  test/openapi/*.json                                               E5.0b
  test/fuzz/  Jamfile bodies.hpp seed.hpp loader_fuzz.cpp validator_fuzz.cpp regressions/  E5.7
  test/bowtie/  Jamfile                                             E5.8
  example/  Jamfile json_schema_*.cpp .expected, bowtie_webcpp.cpp .input .expected
  bowtie/  Dockerfile README.md run.sh compare.py                   E5.8
  benchmark/  Jamfile implementation/runner.cpp implementation/micro.cpp run.py report.py
              report_test.py README.md results.json                 E5.15 to E5.17
  doc/  Jamfile mrdocs.yml json_schema.adoc <sections>.adoc counts.py known_failures.py
        known_failures.adoc benchmark.adoc                          E1.16 on
```

## Shared items, as the infra plan's "Consumer interface" states them

| Item | What a task writes | From | Needed from |
| --- | --- | --- | --- |
| S2 | `<library>/webcpp/third_party//json-schema-test-suite` in a `webcpp.run`'s requirements: the define `WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE` (the suite's root, absolute, forward slashes, no trailing slash, extended by concatenation), a dependency on every suite file, the suite's directory under wasmtime, a refusal by name when not checked out | `infra/shared`, infra Task 1 (and Task 4 for WASI) | E1.14 |
| S3 | regex's `/webcpp/regex//regex`, which brings SRELL | `infra/shared` Task 5, the mirror `~/.buzz/REPOS/srell.git` | E1.1 (`build.jam`) |
| S5 | `[ webcpp.data <directory> + ]` in a `webcpp.run`'s requirements, each directory absolute (a `path-constant`) | `infra/shared` Task 4 | E1.14, and every later test that reads files |
| S1 | `webcpp.generated <name> : <script> : <argument> * : <output> + ;`, its targets `generated-<name>` and `update-generated` | `infra/shared` Task 2 | E2.2 |
| S4 | `<library>/webcpp//boost_url` | `infra/shared` Task 3, with `~/dev/webcpp/.local/boost-source` | E2.1 |
| S4b | `<library>/webcpp//boost_locale` (with `/webcpp//boost_charconv`): refused on emscripten and without exceptions | `infra/shared` Task 3B | E3.2 |
| formats | `<library>/webcpp/lexical//lexical`; `is_iri(s, resource)`, `is_iri_reference(s, resource)`, `is_regex(s, resource)`; `leap_seconds` | the formats track's published result (its F8) | E5.5 only |

`webcpp.data` does nothing for `webcpp.example` and `webcpp.boost-test`, so
every test that reads files is a `webcpp.run`.

## Parallel map

```
Task 0 (controller)
E1:  E1.1 -> E1.2 -> { E1.3 json | E1.4 regex | E1.5 sinks } -> E1.6 -> E1.7 -> E1.8 -> E1.9
     -> E1.10 -> E1.11 -> E1.12 -> E1.13 -> E1.14 -> E1.15 -> E1.16 -> E1.17 FREEZE
E2 | E3 | E4, each in its own worktree from contract-1:
  E2:  { E2.1 URL | E2.2 meta-schemas } -> E2.3 -> E2.4
       -> { E2.5 -> E2.6 -> E2.7 -> E2.10 | E2.8 -> E2.9 } -> E2.11 (merge)
  E3:  { E3.1 pot | E3.2 locale } -> E3.3 (after E2 is merged) -> E3.4 (merge)
  E4:  { E4.1 equivalent | E4.2 emitter } -> E4.3 -> E4.4 (after E2 is merged; merge)
E5 (after E2, E3 and E4 are merged):
       E5.0a -> E5.0b
       -> { E5.1 2019-09 | E5.2 drafts 7, 6 | E5.3 draft 4 | E5.4 OpenAPI | E5.5 formats
            | E5.6 vocabularies | E5.7 fuzz | E5.8 Bowtie }
          merged in that order, each merge followed by record_known of every list
       -> E5.9 content -> E5.10 allocation -> E5.11 round trips -> E5.12 -> E5.13 -> E5.14
       -> E5.15 -> E5.17 Steps 1-2 -> E5.18 -> E5.19 -> E5.20 (merge)
       E5.16 (owner's decisions 2, 3) -> E5.17 Steps 3-4 (owner's run): a later fast-forward
Task P (controller, the owner's go)
```

- `{ a | b }` run at once, each from `task_worktree`, merged into the track
  by `merge_task` in the order written.
- E2.5 to E2.7 and E2.8 to E2.9 are two lanes of E2: they touch different
  files but `test/Jamfile`.
- At most four tracks implement at once across webcpp (spec 20.2).

---

### Task 0: The repository, the E1 worktree, the licences (controller)

**Files:**
- Create:
  - `~/dev/webcpp-repos/json_schema` (empty);
  - `~/dev/webcpp-json-schema-e1` (a worktree; its `libs/json_schema` a
    clone on `e1`);
  - `~/.buzz/WORK_LOGS/WEBCPP_JSON_SCHEMA_LOG.md` (the ledger).

- [ ] **Step 1: What `infra/shared` carries.** E1.1 needs S3; E1.14 needs S2
  and S5. S5 alone may arrive later: E1.14 checks it again at its start.

```bash
git -C ~/dev/webcpp log --oneline -1 infra/shared
git -C ~/dev/webcpp ls-tree infra/shared third_party/libs/JSON-Schema-Test-Suite third_party/libs/srell
git -C ~/dev/webcpp show infra/shared:third_party/Jamfile | grep -n 'alias json-schema-test-suite\|alias srell'
git -C ~/dev/webcpp show infra/shared:tools/webcpp.jam | grep -n 'rule data ( directories + )'
git -C ~/.buzz/REPOS/srell.git log --oneline -3
test ! -e ~/dev/webcpp/libs/json_schema && echo absent
```

  Expected:
  - two `160000 commit` lines: the suite at
    `5b0ee1613e45fcc2bddac00e07c19cd49b00d8a8`, and srell;
  - the two aliases;
  - the `rule data` line;
  - the mirror's three commits (import, no-exceptions, allocator);
  - `absent`.

  If srell or the suite is missing, E1 waits. If only `rule data` is missing,
  E1 starts and E1.14 waits for it.
- [ ] **Step 2: The licences, re-checked.**

```bash
gh api repos/json-schema-org/json-schema-spec/contents/README.md --jq .content | base64 -d | grep -A2 '^## License'
gh api repos/json-schema-org/json-schema-spec/contents/LICENSE --jq .content | base64 -d | head -1
gh api repos/OAI/OpenAPI-Specification/license --jq .license.spdx_id
gh api repos/sourcemeta-research/jsonschema-benchmark/commits/main --jq .sha
gh api repos/sourcemeta-research/jsonschema-benchmark/license --jq .license.spdx_id
gh api repos/sourcemeta/blaze/license --jq .license.spdx_id
gh api repos/ajv-validator/ajv/license --jq .license.spdx_id
gh api repos/bowtie-json-schema/bowtie/license --jq .license.spdx_id
```

  Expected (as found on 2026-10-09):
  - the json-schema-spec README says "licensed under either the BSD 3-clause
    license *or* the Academic Free License v3.0", and its LICENSE opens
    "Copyright (c) 2022 JSON Schema Specification Authors";
  - OpenAPI: `Apache-2.0`;
  - the benchmark's head: `724607bccb6bf0c5d3e8eebbe4b1a5aef75f3bd8`, and
    `gh: Not Found (HTTP 404)` for its licence;
  - Blaze: `AGPL-3.0`; ajv: `MIT`; Bowtie's SPDX id, whatever it is.

  Record each finding in the ledger with the date. A changed finding
  revises decisions 1 to 4 before E2.2, E5.0b, E5.8 and E5.15 start.
- [ ] **Step 3: The repository and the E1 worktree.**

```bash
mkdir -p ~/dev/webcpp-repos
git init -b main ~/dev/webcpp-repos/json_schema
track_worktree 1
cd ~/dev/webcpp-json-schema-e1
$B2 -a --build-dir=bin/check libs/pratt/test 2>&1 | tail -3
```

  Expected:
  - `Initialized empty Git repository in .../webcpp-repos/json_schema/.git/`;
  - git's "You appear to have cloned an empty repository.", then "Switched
    to a new branch 'e1'";
  - the two submodule status lines without `-` or `+`;
  - pratt's tests pass, so the worktree builds. pratt is a submodule of the
    superproject: `git -C ~/dev/webcpp-json-schema-e1 submodule update
    --init libs/pratt` first if its directory is empty.
- [ ] **Step 4: The ledger.** Create it with the knowledge-file frontmatter
  (title `"webcpp::json_schema: ledger"`, status active). Record:
  - `infra/shared`'s commit, and `SUPERPROJECT_BASE=infra/shared`;
  - the licence findings;
  - the worktree;
  - the rule that the Changelog lines wait in the ledger.

---

## E1: runner, IR, loader, core keywords (2020-12)

In `~/dev/webcpp-json-schema-e1`, the library on `e1`. Paths are relative to
`libs/json_schema/` unless they start with `tools/` or `third_party/`.
E1.3, E1.4 and E1.5 run at once, each from `task_worktree 1 json`, `task_worktree 1 regex` and
`task_worktree 1 sinks`, merged into `e1` by `merge_task 1 json`, then
`regex`, then `sinks`.

### Task E1.1: The skeleton, the configuration, the errors, the memory adapters

Runs first, alone.

**Files:**
- Create:
  - `build.jam`, `LICENSE_1_0.txt` (pratt's), `.gitignore` (`doc/html/`),
    `.gitattributes`, `README.md`, `AGENTS.md`;
  - `meta/libraries.json`, `meta/include-boundaries.json`;
  - `.github/workflows/ci.yml`;
  - `include/webcpp/json_schema.hpp`, `include/webcpp/json_schema/config.hpp`,
    `include/webcpp/json_schema/errors.hpp`;
  - `include/webcpp/json_schema/detail/memory/resource.hpp`,
    `include/webcpp/json_schema/detail/memory/inline_stack.hpp`;
  - `test/Jamfile`, `test/require.hpp`, `test/counting_resource.hpp`,
    `test/config_test.cpp`, `test/errors_test.cpp`,
    `test/errors_throw_test.cpp`, `test/memory_test.cpp`.

**Interfaces:**
- Consumes: S3's `/webcpp/third_party//srell` (from `build.jam` on).
- Produces spec 4.1 whole, and:

```cpp
namespace webcpp::json_schema::detail {
/** A diagnostic of code, where and what, each cut to 256 characters with "...", no allocation. */
diagnostic make_diagnostic(boost::system::error_code code, std::string_view where,
                           std::string_view what) noexcept;
/** A std::pmr::memory_resource for any allocator of std::byte: the pmr one's own, or an adapter. */
template <class Allocator> class resource_of;   // not copyable; get() -> std::pmr::memory_resource&
/** A stack whose first N elements live inline, then in a std::pmr resource. */
template <class T, std::size_t N>
using inline_stack = boost::container::small_vector<T, N, std::pmr::polymorphic_allocator<T>>;
}
namespace webcpp::test {
class counting_resource;   // test/counting_resource.hpp: allocations, deallocations, live, peak
}
```

- [ ] **Step 1: Write the failing tests.**

`test/errors_test.cpp`:

```cpp
#include <webcpp/json_schema/errors.hpp>

#include <boost/core/lightweight_test.hpp>

#include <array>
#include <string>

namespace js = webcpp::json_schema;

namespace {

void values_are_fixed() {
    constexpr std::array<js::errc, 20> all{
        js::errc::not_a_schema, js::errc::invalid_keyword_value, js::errc::meta_schema_violation,
        js::errc::unknown_dialect, js::errc::unknown_vocabulary, js::errc::unknown_keyword,
        js::errc::invalid_uri, js::errc::unresolvable_reference, js::errc::resolver_failed,
        js::errc::invalid_json_pointer, js::errc::duplicate_resource, js::errc::duplicate_anchor,
        js::errc::reference_cycle, js::errc::invalid_regex, js::errc::schema_too_deep,
        js::errc::invalid_number, js::errc::invalid_ir, js::errc::unrepresentable,
        js::errc::output_not_collected, js::errc::vocabulary_conflict};
    for (std::size_t i = 0; i < all.size(); ++i) {
        BOOST_TEST_EQ(static_cast<int>(all[i]), static_cast<int>(i) + 1);
    }
}

void the_category_is_webcpp_json_schema() {
    BOOST_TEST_EQ(std::string(js::error_category().name()), "webcpp.json_schema");
    const boost::system::error_code code = js::errc::reference_cycle;
    BOOST_TEST(code.category() == js::error_category());
    BOOST_TEST_EQ(code.message(), "a cycle of references consumes no instance");
    BOOST_TEST_EQ(js::make_error_code(js::errc::not_a_schema).message(), "the value is not a schema");
    BOOST_TEST_EQ(js::make_error_code(js::errc::vocabulary_conflict).message(),
                  "a user keyword's name is taken");
    BOOST_TEST_EQ(js::error_category().message(0), "unknown webcpp.json_schema error");
    BOOST_TEST_EQ(js::error_category().message(21), "unknown webcpp.json_schema error");
    std::array<char, 8> small{};
    BOOST_TEST_EQ(std::string(js::error_category().message(1, small.data(), small.size())),
                  "the value is not a schema");
    BOOST_TEST_EQ(std::string(small.data()), "the val");
}

void a_diagnostic_names_its_place_without_allocating() {
    const std::string long_text(300, 'x');
    const js::diagnostic d =
        js::detail::make_diagnostic(js::errc::unknown_dialect, long_text, "draft-03");
    BOOST_TEST(d.code == js::errc::unknown_dialect);
    BOOST_TEST_EQ(d.where.size(), 256U);
    BOOST_TEST_EQ(std::string(d.where.substr(253)), "...");
    BOOST_TEST_EQ(std::string(d.what), "draft-03");
}

void a_result_holds_a_diagnostic() {
    const js::result<int> r = js::detail::make_diagnostic(js::errc::invalid_uri, "/$id", "a b");
    BOOST_TEST(r.has_error());
    BOOST_TEST(r.error().code == js::errc::invalid_uri);
    BOOST_TEST_EQ(std::string(r.error().where), "/$id");
}

}  // namespace

int main() {
    values_are_fixed();
    the_category_is_webcpp_json_schema();
    a_diagnostic_names_its_place_without_allocating();
    a_result_holds_a_diagnostic();
    return boost::report_errors();
}
```

`test/errors_throw_test.cpp`, built with exceptions:

```cpp
#include <webcpp/json_schema/errors.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/system/system_error.hpp>

#include <string>

namespace js = webcpp::json_schema;

int main() {
    const js::result<int> r = js::detail::make_diagnostic(js::errc::invalid_regex, "/pattern", "(");
    try {
        (void)r.value();
        BOOST_ERROR("value() of an error returned");
    } catch (const boost::system::system_error& e) {
        BOOST_TEST(e.code() == js::errc::invalid_regex);
        BOOST_TEST(std::string(e.what()).find("/pattern: (") != std::string::npos);
    }
    return boost::report_errors();
}
```

`test/counting_resource.hpp`:

```cpp
#ifndef WEBCPP_TEST_JSON_SCHEMA_COUNTING_RESOURCE_HPP
#define WEBCPP_TEST_JSON_SCHEMA_COUNTING_RESOURCE_HPP

#include <cstddef>
#include <memory_resource>

namespace webcpp::test {

// Counts what it allocates, frees and holds at its peak, and passes each request upstream.
class counting_resource final : public std::pmr::memory_resource {
public:
    explicit counting_resource(std::pmr::memory_resource* upstream = std::pmr::new_delete_resource())
        : upstream_(upstream) {}
    std::size_t allocations = 0;
    std::size_t deallocations = 0;
    std::size_t live = 0;
    std::size_t peak = 0;

private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        ++allocations;
        live += bytes;
        peak = live > peak ? live : peak;
        return upstream_->allocate(bytes, alignment);
    }
    void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override {
        ++deallocations;
        live -= bytes;
        upstream_->deallocate(p, bytes, alignment);
    }
    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
    std::pmr::memory_resource* upstream_;
};

}  // namespace webcpp::test

#endif  // WEBCPP_TEST_JSON_SCHEMA_COUNTING_RESOURCE_HPP
```

`test/memory_test.cpp`:

```cpp
#include <webcpp/json_schema/detail/memory/inline_stack.hpp>
#include <webcpp/json_schema/detail/memory/resource.hpp>

#include "counting_resource.hpp"

#include <boost/core/lightweight_test.hpp>

#include <memory>

namespace js = webcpp::json_schema;

namespace {

void a_pmr_allocator_gives_its_own_resource() {
    webcpp::test::counting_resource counted;
    const std::pmr::polymorphic_allocator<std::byte> pmr(&counted);
    js::detail::resource_of<std::pmr::polymorphic_allocator<std::byte>> direct(pmr);
    BOOST_TEST(&direct.get() == &counted);
}

void another_allocator_gets_an_adapter() {
    js::detail::resource_of<std::allocator<std::byte>> adapted{std::allocator<std::byte>{}};
    void* p = adapted.get().allocate(64, 16);
    BOOST_TEST(p != nullptr);
    adapted.get().deallocate(p, 64, 16);
}

void an_inline_stack_spills_into_its_resource() {
    webcpp::test::counting_resource counted;
    js::detail::inline_stack<int, 4> stack{std::pmr::polymorphic_allocator<int>(&counted)};
    for (int i = 0; i < 4; ++i) {
        stack.push_back(i);
    }
    BOOST_TEST_EQ(counted.allocations, 0U);
    stack.push_back(4);
    BOOST_TEST_EQ(counted.allocations, 1U);
}

}  // namespace

int main() {
    a_pmr_allocator_gives_its_own_resource();
    another_allocator_gets_an_adapter();
    an_inline_stack_spills_into_its_resource();
    return boost::report_errors();
}
```

  `test/config_test.cpp` is pratt's with `PRATT` replaced by `JSON_SCHEMA`
  and `pratt` by `json_schema` in its include, its messages and its macros
  (`WEBCPP_TEST_JSON_SCHEMA_DEVELOPER_DEFINED`). `test/require.hpp` is pratt's,
  its guard `WEBCPP_TEST_JSON_SCHEMA_REQUIRE_HPP`.

`test/Jamfile`:

```
project : requirements <library>/webcpp/json_schema//json_schema ;

import webcpp ;

webcpp.targets native wasip2 wasip3 emscripten ;

# Every public header compiles alone, one call per directory.
webcpp.headers-alone json_schema : ../include : json_schema.hpp json_schema/*.hpp ;
webcpp.headers-alone json_schema : ../include : json_schema/detail/memory/*.hpp ;

# config.hpp defines WEBCPP_JSON_SCHEMA_NO_EXCEPTIONS exactly when the build
# has no exceptions, and keeps it when a developer defines it in a build
# that has them.
webcpp.compile config : config_test.cpp ;
webcpp.compile config_developer_defined : config_test.cpp
  : <define>WEBCPP_JSON_SCHEMA_NO_EXCEPTIONS <define>WEBCPP_TEST_JSON_SCHEMA_DEVELOPER_DEFINED ;

webcpp.run errors : errors_test.cpp ;
# result::value() of an error raises it, which this test catches.
webcpp.run errors_throw : errors_throw_test.cpp : <exception-handling>on ;
webcpp.run memory : memory_test.cpp ;
```

- [ ] **Step 2: Run them and watch them fail.** `$B2 libs/json_schema/test`.
  Expected: FAIL, `error: unable to find file or target named
  '/webcpp/json_schema//json_schema'` until `build.jam` exists, then
  `'webcpp/json_schema/errors.hpp' file not found`.
- [ ] **Step 3: Implement.**

`build.jam`:

```
# json_schema, a header-only JSON Schema validator for C++20. A program uses
# it through /webcpp/json_schema//json_schema, which adds its headers,
# Boost's, Boost.JSON's definitions and SRELL.

project /webcpp/json_schema ;

alias json_schema
  : : : :
    <include>include
    <library>/boost//headers
    <library>/webcpp//boost_json
    <library>/webcpp/third_party//srell
  ;
```

`meta/libraries.json`:

```json
{
    "key": "json_schema",
    "name": "json_schema",
    "authors": ["R"],
    "description": "A header-only JSON Schema validator for C++20: every dialect from draft 4 to 2020-12 and OpenAPI 3.0, proven by the official test suite.",
    "category": ["Parsing", "Correctness"],
    "maintainers": ["R <github.overboard312@passmail.net>"],
    "cxxstd": "20",
    "port-of": null
}
```

`meta/include-boundaries.json`. Each later task appends one object per header
directory it creates, in this form, so that parallel tasks never edit the
same lines:

```json
{
    "boundaries": [
        {
            "headers": [
                "include/webcpp/json_schema.hpp",
                "include/webcpp/json_schema/*.hpp",
                "include/webcpp/json_schema/detail/memory/*.hpp"
            ],
            "must-not-include": ["boost/locale", "srell", "boost/url", "webcpp/lexical"],
            "why": "the engines are reached only through their policies: Boost.Locale through output/locale.hpp, SRELL through regex/srell.hpp, Boost.URL through url/boost_url.hpp, formats through formats/default_formats.hpp"
        }
    ]
}
```

`include/webcpp/json_schema/config.hpp` is pratt's `config.hpp` with every
`PRATT` and `pratt` renamed. It includes `<boost/config.hpp>` until
`webcpp::config` exists, with this macro comment:

```cpp
/**
 Defined when json_schema is built without exceptions: automatically when the
 compiler has none (`BOOST_NO_EXCEPTIONS`), or by the developer to disable them
 in a build that has them.

 While it is defined, `basic_validator::compile_or_throw` does not exist and
 `<webcpp/json_schema/output/locale.hpp>` refuses to compile, naming
 Boost.Locale. Every other operation reports its failure as a value, in both
 modes.

 @see "Errors and exceptions", in the guide.
*/
```

`include/webcpp/json_schema/errors.hpp` follows pratt's `errors.hpp`:
- `detail::message_of(int)`;
- the `constexpr` category with identity `0x6a736368656d6100` and name
  `"webcpp.json_schema"`;
- the two `message` overloads;
- `error_category()` and `make_error_code`;
- the `is_error_code_enum` specialization.

The messages, in enumerator order:

```cpp
inline const char* message_of(int value) noexcept {
    switch (static_cast<errc>(value)) {
        case errc::not_a_schema: return "the value is not a schema";
        case errc::invalid_keyword_value: return "a keyword's value has the wrong shape";
        case errc::meta_schema_violation: return "the schema fails its meta-schema";
        case errc::unknown_dialect: return "the dialect is outside the validator's set";
        case errc::unknown_vocabulary: return "a required vocabulary is unknown";
        case errc::unknown_keyword: return "the keyword is refused";
        case errc::invalid_uri: return "the URI is not a URI reference";
        case errc::unresolvable_reference: return "no resource for the reference";
        case errc::resolver_failed: return "the resolver failed";
        case errc::invalid_json_pointer: return "the JSON Pointer is invalid or points at nothing";
        case errc::duplicate_resource: return "two resources have one URI";
        case errc::duplicate_anchor: return "two anchors have one name";
        case errc::reference_cycle: return "a cycle of references consumes no instance";
        case errc::invalid_regex: return "the pattern is not a valid regular expression";
        case errc::schema_too_deep: return "the schema nests too deeply";
        case errc::invalid_number: return "the number cannot be represented";
        case errc::invalid_ir: return "the IR breaks an invariant";
        case errc::unrepresentable: return "the target dialect cannot express the schema";
        case errc::output_not_collected: return "the output format needs more than was collected";
        case errc::vocabulary_conflict: return "a user keyword's name is taken";
    }
    return "unknown webcpp.json_schema error";
}
```

The diagnostic, its maker and its throw:

```cpp
struct diagnostic {
    /** Why it failed: an @ref errc, or the code of the component that refused. */
    boost::system::error_code code;
    /** Where: a keyword location or a URI, cut to 256 characters with "...". */
    boost::static_strings::static_string<256> where;
    /** What: the dialect, keyword or detail named, cut likewise. */
    boost::static_strings::static_string<256> what;
};

namespace detail {

/** Copies `text` into `out`, its last three characters "..." when it does not fit. */
inline void assign_cut(boost::static_strings::static_string<256>& out, std::string_view text) noexcept {
    if (text.size() <= out.max_size()) {
        out.assign(text.data(), text.size());
        return;
    }
    out.assign(text.data(), out.max_size() - 3);
    out.append("...");
}

/** A diagnostic of `code`, `where` and `what`, without allocating. */
inline diagnostic make_diagnostic(boost::system::error_code code, std::string_view where,
                                  std::string_view what) noexcept {
    diagnostic d;
    d.code = code;
    assign_cut(d.where, where);
    assign_cut(d.what, what);
    return d;
}

}  // namespace detail

[[noreturn]] inline void throw_exception_from_error(const diagnostic& d,
                                                    const boost::source_location& loc) {
    std::string text(d.where.data(), d.where.size());
    if (!d.what.empty()) {
        text.append(": ").append(d.what.data(), d.what.size());
    }
    boost::throw_exception(boost::system::system_error(d.code, text), loc);
}
```

  `throw_exception_from_error` stays outside the macro:
  `result<T, diagnostic>::value()` finds it by argument-dependent lookup in
  both modes, and without exceptions `boost::throw_exception` calls the
  program's handler.

`detail/memory/resource.hpp`:

```cpp
template <class Allocator>
class allocator_resource final : public std::pmr::memory_resource {
public:
    explicit allocator_resource(const Allocator& a) noexcept : allocator_(a) {}

private:
    using slots = typename std::allocator_traits<Allocator>::template rebind_alloc<std::max_align_t>;
    static std::size_t count(std::size_t size) noexcept {
        return (size + sizeof(std::max_align_t) - 1) / sizeof(std::max_align_t);
    }
    void* do_allocate(std::size_t size, std::size_t /*alignment*/) override {
        slots a(allocator_);
        return std::allocator_traits<slots>::allocate(a, count(size));
    }
    void do_deallocate(void* p, std::size_t size, std::size_t /*alignment*/) override {
        slots a(allocator_);
        std::allocator_traits<slots>::deallocate(a, static_cast<std::max_align_t*>(p), count(size));
    }
    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
    Allocator allocator_;
};

template <class Allocator>
class resource_of {
public:
    explicit resource_of(const Allocator& a) noexcept : adapter_(a) {}
    resource_of(const resource_of&) = delete;
    resource_of& operator=(const resource_of&) = delete;
    std::pmr::memory_resource& get() noexcept { return adapter_; }

private:
    allocator_resource<Allocator> adapter_;
};

template <class T>
class resource_of<std::pmr::polymorphic_allocator<T>> {
public:
    explicit resource_of(const std::pmr::polymorphic_allocator<T>& a) noexcept
        : resource_(a.resource()) {}
    resource_of(const resource_of&) = delete;
    resource_of& operator=(const resource_of&) = delete;
    std::pmr::memory_resource& get() noexcept { return *resource_; }

private:
    std::pmr::memory_resource* resource_;
};
```

  `allocator_resource` serves alignments up to `alignof(std::max_align_t)`,
  every alignment json_schema and Boost.JSON ask for; its Doc Comment says so.
  `detail/memory/inline_stack.hpp` is the alias of the Interfaces.

  The repository's files:
  - `.gitattributes` is pratt's, plus `*.input -text` and
    `meta_schemas/**/*.json -text`;
  - `README.md` says what json_schema is (spec 1's first paragraph), how to
    build and test it inside the superproject, where its page is, and
    BSL-1.0;
  - `AGENTS.md` links `../../AGENTS.md`, then says what is specific: the
    suite and its pin, the known-failure rule, the frozen files, SRELL and
    its mirror, the two allocators, deep input, the benchmark's machine;
  - `.github/workflows/ci.yml` is xactor's, with `library: json_schema`;
  - `include/webcpp/json_schema.hpp` includes `config.hpp` and `errors.hpp`.
    Each later task adds its block, every public header but
    `output/locale.hpp`, which its comment names with Boost.Locale.

- [ ] **Step 4: Run the lanes and watch them pass.**

```bash
lanes libs/json_schema/test
$B2 -d0 declared-targets | grep json_schema
lint_clean
```

  Expected:
  - `lanes: green`;
  - `json_schema native`, `json_schema wasip2`, `json_schema wasip3`,
    `json_schema emscripten`;
  - `lint: clean`.

  Then prove the boundary: add `#include <srell.hpp>` to `errors.hpp`, see
  the lint name that line with the boundary's `why`, and revert.
- [ ] **Step 5: Commit.**

```bash
cd libs/json_schema
F="build.jam LICENSE_1_0.txt .gitignore .gitattributes README.md AGENTS.md meta/libraries.json \
meta/include-boundaries.json .github/workflows/ci.yml include/webcpp/json_schema.hpp \
include/webcpp/json_schema/config.hpp include/webcpp/json_schema/errors.hpp \
include/webcpp/json_schema/detail/memory/resource.hpp include/webcpp/json_schema/detail/memory/inline_stack.hpp \
test/Jamfile test/require.hpp test/counting_resource.hpp test/config_test.cpp test/errors_test.cpp \
test/errors_throw_test.cpp test/memory_test.cpp"
git add $F
$G commit -m "json_schema: the library's skeleton, its configuration, its errors and its memory adapters" -- $F
```

### Task E1.2: Exact numbers

After E1.1, alone (E1.3 builds on it).

**Files:**
- Create:
  - `include/webcpp/json_schema/numbers/number.hpp`,
    `include/webcpp/json_schema/numbers/decimal.hpp`;
  - `include/webcpp/json_schema/ir/number.hpp`;
  - `include/webcpp/json_schema/detail/numbers/arithmetic.hpp`,
    `detail/numbers/decompose.hpp`, `detail/numbers/compare.hpp`,
    `detail/numbers/multiple_of.hpp`;
  - `test/numbers_test.cpp`.
- Modify: `include/webcpp/json_schema.hpp`, `test/Jamfile`,
  `meta/include-boundaries.json`.

**Interfaces:**
- Produces spec 4.2's number types, spec 5.1's `ir::number`, and:

```cpp
namespace webcpp::json_schema::detail {
using u128 = boost::decimal::uint128_t;                  // Boost.Int128's, through Boost.Decimal
u128 to_u128(const uint128& v) noexcept;
uint128 from_u128(u128 v) noexcept;
// a schema number: int64 and uint64 exactly, a Number by its traits
template <class Number> decomposed decompose(const ir::number<Number>& n) noexcept;
// an instance double: the exact decimal of its shortest round-trip text, at most 17 digits;
// nothing for infinity or NaN
std::optional<decomposed> decompose_double(double d) noexcept;
// an instance value: int64, uint64 exactly, a double as above; nothing for a non-number
std::optional<decomposed> decompose_value(const boost::json::value& v) noexcept;
std::strong_ordering compare(const decomposed& a, const decomposed& b) noexcept;
bool is_multiple_of(const decomposed& x, const decomposed& y) noexcept;   // y > 0
// the loader's: a schema double to ir::number (spec invariant 7): an integral double that fits
// int64_t stays an integer, any other is Number from the shortest text; nothing when Number
// cannot hold it
template <class Number> std::optional<ir::number<Number>> schema_number_of_double(double d) noexcept;
bool is_integer_value(const boost::json::value& v, bool integer_by_value) noexcept;
}
```

  The meaning, settled by section "Decisions", amendment A1:
  - an instance double is never rounded to `Number`;
  - a schema number keeps spec 12's `Number`, whose precision is the
    documented limit (a 17-digit schema literal rounds in `decimal64_t`,
    not in `decimal128_t`).

- [ ] **Step 1: Write the failing test** `test/numbers_test.cpp`:

```cpp
#include <webcpp/json_schema/detail/numbers/compare.hpp>
#include <webcpp/json_schema/detail/numbers/decompose.hpp>
#include <webcpp/json_schema/detail/numbers/multiple_of.hpp>
#include <webcpp/json_schema/numbers/decimal.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/json/value.hpp>

#include <array>
#include <cstdint>
#include <limits>
#include <string_view>

namespace js = webcpp::json_schema;
using d64 = boost::decimal::decimal64_t;
using d128 = boost::decimal::decimal128_t;

static_assert(js::number<d64>);
static_assert(js::number<d128>);
static_assert(!js::number<double>);   // no number_traits<double> ships (S3)

namespace {

using n64 = js::ir::number<d64>;

js::decomposed schema(std::string_view text) {
    return js::detail::decompose(n64{std::in_place_index<2>, *js::number_traits<d64>::from_chars(text)});
}
js::decomposed schema_int(std::int64_t v) { return js::detail::decompose(n64{std::in_place_index<0>, v}); }
js::decomposed schema_uint(std::uint64_t v) { return js::detail::decompose(n64{std::in_place_index<1>, v}); }
js::decomposed instance(double d) { return *js::detail::decompose_double(d); }

void decimal_traits() {
    const js::decomposed tenth = js::number_traits<d64>::decompose(*js::number_traits<d64>::from_chars("0.1"));
    BOOST_TEST(js::detail::compare(tenth, {.negative = false, .coefficient = {.high = 0, .low = 1}, .exponent = -1}) == 0);
    BOOST_TEST(!js::number_traits<d64>::from_chars("1e400"));
    BOOST_TEST(js::number_traits<d128>::from_chars("1e400").has_value());
    BOOST_TEST(!js::number_traits<d64>::from_chars("1.5x"));
    BOOST_TEST(!js::number_traits<d64>::from_chars(""));
    BOOST_TEST(js::number_traits<d64>::is_integer(*js::number_traits<d64>::from_chars("2.0")));
    std::array<char, 32> text{};
    const std::size_t n = js::number_traits<d64>::to_chars(text, *js::number_traits<d64>::from_chars("1048503.5"));
    BOOST_TEST_EQ(std::string_view(text.data(), n), "1048503.5");
}

void instance_doubles_are_never_rounded() {
    const double two_to_64 = 18446744073709551616.0;
    BOOST_TEST(js::detail::compare(schema_uint(std::numeric_limits<std::uint64_t>::max()), instance(two_to_64)) < 0);
    BOOST_TEST(js::detail::compare(instance(0.1 + 0.2), schema("0.3")) > 0);
    BOOST_TEST(js::detail::compare(instance(0.1 + 0.2), instance(0.3)) > 0);
    BOOST_TEST(js::detail::compare(instance(1.1), schema("1.1")) == 0);   // exclusiveMinimum 1.1
    BOOST_TEST(js::detail::compare(instance(-0.0), schema_int(0)) == 0);
    BOOST_TEST(js::detail::compare(instance(1.0), schema_int(1)) == 0);
    BOOST_TEST(js::detail::compare(instance(1e-300), schema_int(0)) > 0);
    BOOST_TEST(js::detail::compare(schema_int(std::numeric_limits<std::int64_t>::min()), instance(-9.3e18)) > 0);
    BOOST_TEST(!js::detail::decompose_double(std::numeric_limits<double>::infinity()));
    BOOST_TEST(!js::detail::decompose_value(boost::json::value("1")));
}

void the_schema_side_keeps_its_documented_limit() {
    // 17 digits round in decimal64_t (16), not in decimal128_t (34)
    const auto rounded = *js::number_traits<d64>::from_chars("0.30000000000000004");
    BOOST_TEST(rounded == *js::number_traits<d64>::from_chars("0.3"));
    const auto wide = *js::number_traits<d128>::from_chars("0.30000000000000004");
    BOOST_TEST(wide != *js::number_traits<d128>::from_chars("0.3"));
}

void multiple_of_is_exact() {
    using js::detail::is_multiple_of;
    // multipleOf.json of draft 2020-12, its required cases
    BOOST_TEST(is_multiple_of(schema_int(10), schema_int(2)));
    BOOST_TEST(!is_multiple_of(schema_int(7), schema_int(2)));
    BOOST_TEST(is_multiple_of(schema_int(0), schema("1.5")));
    BOOST_TEST(is_multiple_of(instance(4.5), schema("1.5")));
    BOOST_TEST(!is_multiple_of(schema_int(35), schema("1.5")));
    BOOST_TEST(is_multiple_of(instance(0.0075), schema("0.0001")));
    BOOST_TEST(!is_multiple_of(instance(0.00751), schema("0.0001")));
    BOOST_TEST(!is_multiple_of(instance(1e308), schema("0.123456789")));
    BOOST_TEST(is_multiple_of(schema_int(12391239123), schema("1e-8")));
    // optional/float-overflow.json, and the review's 1e308 by 0.0001
    BOOST_TEST(is_multiple_of(instance(1e308), schema("0.5")));
    BOOST_TEST(is_multiple_of(instance(1e308), schema("0.0001")));
    // a coefficient of 34 digits in decimal128
    using n128 = js::ir::number<d128>;
    const auto y = js::detail::decompose(n128{std::in_place_index<2>,
        *js::number_traits<d128>::from_chars("1234567890123456789012345678901234e-30")});
    const auto x = js::detail::decompose(n128{std::in_place_index<2>,
        *js::number_traits<d128>::from_chars("3703703670370370367037037036703702e-30")});
    BOOST_TEST(is_multiple_of(x, y));
    BOOST_TEST(!is_multiple_of(y, x));
}

void integers_by_value() {
    BOOST_TEST(js::detail::is_integer_value(boost::json::value(1.0), true));
    BOOST_TEST(!js::detail::is_integer_value(boost::json::value(1.0), false));   // draft 4
    BOOST_TEST(js::detail::is_integer_value(boost::json::value(std::int64_t{1}), false));
    BOOST_TEST(!js::detail::is_integer_value(boost::json::value(1.5), true));
    const auto big = js::detail::schema_number_of_double<d64>(1e20);
    BOOST_TEST(big && big->index() == 2);
    const auto one = js::detail::schema_number_of_double<d64>(1.0);
    BOOST_TEST(one && one->index() == 0);
}

}  // namespace

int main() {
    decimal_traits();
    instance_doubles_are_never_rounded();
    the_schema_side_keeps_its_documented_limit();
    multiple_of_is_exact();
    integers_by_value();
    return boost::report_errors();
}
```

  Append to `test/Jamfile`:

```
webcpp.headers-alone json_schema : ../include
  : json_schema/numbers/*.hpp json_schema/ir/number.hpp json_schema/detail/numbers/*.hpp ;
webcpp.run numbers : numbers_test.cpp ;
```

  and to `meta/include-boundaries.json` one object for
  `include/webcpp/json_schema/numbers/*.hpp` and
  `include/webcpp/json_schema/detail/numbers/*.hpp`, with E1.1's four
  prefixes and `why`, and the IR's own object:

```json
        {
            "headers": ["include/webcpp/json_schema/ir/number.hpp"],
            "must-not-include": [
                "webcpp/json_schema/detail/compiler",
                "webcpp/json_schema/detail/vm",
                "webcpp/json_schema/output",
                "webcpp/json_schema/emitter",
                "boost/locale",
                "srell",
                "boost/url",
                "webcpp/lexical"
            ],
            "why": "the IR is the contract; it knows no engine"
        }
```

  E1.7 widens that object's `headers` to `include/webcpp/json_schema/ir/*.hpp`.
  E4.1 adds `"except": ["include/webcpp/json_schema/ir/equivalent.hpp"]` only
  if `equivalent.hpp` needs one of the prefixes, which spec 5.4 does not ask.
- [ ] **Step 2: Run it and watch it fail.**
  `$B2 libs/json_schema/test//numbers`. Expected: FAIL,
  `'webcpp/json_schema/detail/numbers/compare.hpp' file not found`.
- [ ] **Step 3: Implement.**

`numbers/number.hpp`: spec 4.2's `uint128`, `decomposed` (each member with
a default and `operator==` defaulted), the primary `number_traits` and the
concept.

`numbers/decimal.hpp`, one specialization per type through Boost.Decimal. The
decimal64 one is below; decimal128's differs only in its type and its
significand, a `boost::decimal::uint128_t` split with `>> 64`:

```cpp
template <>
struct number_traits<boost::decimal::decimal64_t> {
    using type = boost::decimal::decimal64_t;

    static boost::system::result<type> from_chars(std::string_view text) noexcept {
        if (text.empty()) {
            return boost::system::errc::make_error_code(boost::system::errc::invalid_argument);
        }
        type value{};
        const char* last = text.data() + text.size();
        const auto parsed = boost::decimal::from_chars(text.data(), last, value,
                                                       boost::decimal::chars_format::general);
        if (parsed.ec != std::errc{} || parsed.ptr != last || !boost::decimal::isfinite(value)) {
            return boost::system::errc::make_error_code(boost::system::errc::result_out_of_range);
        }
        return value;
    }

    static decomposed decompose(type n) noexcept {
        int exponent = 0;
        const auto significand = boost::decimal::frexp10(n, &exponent);
        return {.negative = boost::decimal::signbit(n),
                .coefficient = {.high = 0, .low = static_cast<std::uint64_t>(significand)},
                .exponent = static_cast<std::int32_t>(exponent)};
    }

    static bool is_integer(type n) noexcept {
        return boost::decimal::isfinite(n) && boost::decimal::trunc(n) == n;
    }

    static std::size_t to_chars(std::span<char> out, type n) noexcept {
        const auto written = boost::decimal::to_chars(out.data(), out.data() + out.size(), n);
        return written.ec == std::errc{} ? static_cast<std::size_t>(written.ptr - out.data()) : 0;
    }
};
```

`detail/numbers/arithmetic.hpp` (Boost.Int128's type, through Boost.Decimal):

```cpp
using u128 = boost::decimal::uint128_t;

inline u128 to_u128(const uint128& v) noexcept {
    return (static_cast<u128>(v.high) << 64) | static_cast<u128>(v.low);
}

inline uint128 from_u128(u128 v) noexcept {
    return {.high = static_cast<std::uint64_t>(v >> 64), .low = static_cast<std::uint64_t>(v)};
}

inline std::int64_t digits(u128 c) noexcept {
    std::int64_t count = 0;
    do {
        ++count;
        c /= 10U;
    } while (c != 0U);
    return count;
}

/** c * 10^n, or nothing when it exceeds 128 bits. */
inline std::optional<u128> scale(u128 c, std::int64_t n) noexcept {
    const u128 limit = std::numeric_limits<u128>::max() / 10U;
    for (std::int64_t i = 0; i < n; ++i) {
        if (c > limit) {
            return std::nullopt;
        }
        c *= 10U;
    }
    return c;
}

/** (a * b) mod m by doubling, for m below 2^127, so nothing overflows. */
inline u128 mulmod(u128 a, u128 b, u128 m) noexcept {
    u128 result = 0U;
    a %= m;
    while (b != 0U) {
        if ((b & 1U) != 0U) {
            result += a;
            result = result >= m ? result - m : result;
        }
        a += a;
        a = a >= m ? a - m : a;
        b >>= 1;
    }
    return result;
}

/** 10^n mod m, by squaring. */
inline u128 pow10_mod(std::uint64_t n, u128 m) noexcept {
    u128 result = 1U % m;
    u128 base = 10U % m;
    for (; n != 0; n >>= 1) {
        if ((n & 1U) != 0U) {
            result = mulmod(result, base, m);
        }
        base = mulmod(base, base, m);
    }
    return result;
}

/** |value| = coefficient * 10^exponent, no trailing zero in the coefficient; zero is {0, 0}. */
struct magnitude {
    u128 coefficient;
    std::int64_t exponent;
};

inline magnitude normalize(const decomposed& d) noexcept {
    u128 c = to_u128(d.coefficient);
    std::int64_t e = d.exponent;
    if (c == 0U) {
        return {.coefficient = 0U, .exponent = 0};
    }
    while (c % 10U == 0U) {
        c /= 10U;
        ++e;
    }
    return {.coefficient = c, .exponent = e};
}
```

`detail/numbers/decompose.hpp`:

```cpp
inline decomposed decompose_int64(std::int64_t i) noexcept {
    const bool negative = i < 0;
    const std::uint64_t m = negative ? 0 - static_cast<std::uint64_t>(i) : static_cast<std::uint64_t>(i);
    return {.negative = negative, .coefficient = {.high = 0, .low = m}, .exponent = 0};
}

template <class Number>
decomposed decompose(const ir::number<Number>& n) noexcept {
    if (const auto* i = std::get_if<0>(&n)) {
        return decompose_int64(*i);
    }
    if (const auto* u = std::get_if<1>(&n)) {
        return {.negative = false, .coefficient = {.high = 0, .low = *u}, .exponent = 0};
    }
    return number_traits<Number>::decompose(std::get<2>(n));
}

/** The decimal the shortest round-trip text of `d` denotes, exactly: "1.8446744073709552e+19". */
inline std::optional<decomposed> decompose_double(double d) noexcept {
    if (!std::isfinite(d)) {
        return std::nullopt;
    }
    std::array<char, 32> buffer{};
    const auto written = std::to_chars(buffer.data(), buffer.data() + buffer.size(), d);
    const std::string_view text(buffer.data(), static_cast<std::size_t>(written.ptr - buffer.data()));
    const bool negative = text.front() == '-';
    const std::size_t e = text.find_first_of("eE");
    const std::string_view mantissa = text.substr(negative ? 1 : 0, std::min(e, text.size()) - (negative ? 1 : 0));
    u128 coefficient = 0U;
    std::int64_t exponent = 0;
    bool after_point = false;
    for (const char c : mantissa) {
        if (c == '.') {
            after_point = true;
            continue;
        }
        coefficient = coefficient * 10U + static_cast<unsigned>(c - '0');
        exponent -= after_point ? 1 : 0;
    }
    if (e != std::string_view::npos) {
        const std::string_view written_exponent = text.substr(e + (text[e + 1] == '+' ? 2 : 1));
        int value = 0;
        std::from_chars(written_exponent.data(), written_exponent.data() + written_exponent.size(), value);
        exponent += value;
    }
    return decomposed{.negative = negative, .coefficient = from_u128(coefficient),
                      .exponent = static_cast<std::int32_t>(exponent)};
}

inline std::optional<decomposed> decompose_value(const boost::json::value& v) noexcept {
    if (const auto* i = v.if_int64()) {
        return decompose_int64(*i);
    }
    if (const auto* u = v.if_uint64()) {
        return decomposed{.negative = false, .coefficient = {.high = 0, .low = *u}, .exponent = 0};
    }
    if (const auto* d = v.if_double()) {
        return decompose_double(*d);
    }
    return std::nullopt;
}
```

  The coefficient of a shortest text has at most 17 digits, far inside 128
  bits.

`detail/numbers/compare.hpp`:

```cpp
inline std::strong_ordering order(u128 a, u128 b) noexcept {
    if (a < b) {
        return std::strong_ordering::less;
    }
    return a == b ? std::strong_ordering::equal : std::strong_ordering::greater;
}

/** Same order of magnitude means the scaled coefficient has the other's digit count, at most 34,
    so the scale never overflows. */
inline std::strong_ordering compare_magnitudes(const magnitude& x, const magnitude& y) noexcept {
    if (x.coefficient == 0U || y.coefficient == 0U) {
        return order(x.coefficient == 0U ? 0U : 1U, y.coefficient == 0U ? 0U : 1U);
    }
    const std::int64_t x_order = digits(x.coefficient) + x.exponent;
    const std::int64_t y_order = digits(y.coefficient) + y.exponent;
    if (x_order != y_order) {
        return x_order <=> y_order;
    }
    if (x.exponent >= y.exponent) {
        return order(*scale(x.coefficient, x.exponent - y.exponent), y.coefficient);
    }
    return order(x.coefficient, *scale(y.coefficient, y.exponent - x.exponent));
}

inline std::strong_ordering compare(const decomposed& a, const decomposed& b) noexcept {
    const magnitude x = normalize(a);
    const magnitude y = normalize(b);
    const bool x_negative = a.negative && x.coefficient != 0U;
    const bool y_negative = b.negative && y.coefficient != 0U;
    if (x_negative != y_negative) {
        return x_negative ? std::strong_ordering::less : std::strong_ordering::greater;
    }
    const std::strong_ordering m = compare_magnitudes(x, y);
    return x_negative ? 0 <=> m : m;
}
```

`detail/numbers/multiple_of.hpp` (spec 12):

```cpp
inline bool is_multiple_of(const decomposed& x, const decomposed& y) noexcept {
    const magnitude a = normalize(x);
    const magnitude b = normalize(y);
    if (a.coefficient == 0U) {
        return true;
    }
    if (b.coefficient == 0U) {
        return false;   // y > 0 by [ir-7]; a zero here is no multiple of anything
    }
    if (a.exponent >= b.exponent) {
        const u128 rest = a.coefficient % b.coefficient;
        const auto shift = static_cast<std::uint64_t>(a.exponent - b.exponent);
        return mulmod(rest, pow10_mod(shift, b.coefficient), b.coefficient) == 0U;
    }
    const std::int64_t shift = b.exponent - a.exponent;
    if (digits(b.coefficient) + shift > digits(a.coefficient)) {
        return false;   // cy * 10^shift exceeds cx, which is not zero
    }
    return a.coefficient % *scale(b.coefficient, shift) == 0U;
}
```

  `schema_number_of_double` and `is_integer_value` sit in `decompose.hpp`.
  An integral double in `[-2^63, 2^63)` becomes `int64_t`; any other finite
  double becomes `Number` from its shortest text, through
  `number_traits::from_chars`; a non-finite one or an unrepresentable one
  gives nothing.
- [ ] **Step 4: Run it and watch it pass** on the five lanes:

```bash
lanes libs/json_schema/test
lint_clean
```

  Expected: `lanes: green`, then `lint: clean`.
- [ ] **Step 5: Commit.**

```bash
cd libs/json_schema
F="include/webcpp/json_schema.hpp include/webcpp/json_schema/numbers/number.hpp \
include/webcpp/json_schema/numbers/decimal.hpp include/webcpp/json_schema/ir/number.hpp \
include/webcpp/json_schema/detail/numbers/arithmetic.hpp include/webcpp/json_schema/detail/numbers/decompose.hpp \
include/webcpp/json_schema/detail/numbers/compare.hpp include/webcpp/json_schema/detail/numbers/multiple_of.hpp \
meta/include-boundaries.json test/Jamfile test/numbers_test.cpp"
git add $F
$G commit -m "json_schema: exact numbers, an instance double never rounded, multipleOf without floating point" -- $F
```

### Task E1.3: JSON equality, copies, arenas, pointers and UTF-8

After E1.2; runs alongside E1.4 and E1.5 (slug `json`).

**Files:**
- Create:
  - `include/webcpp/json_schema/detail/json/json_resource.hpp`,
    `detail/json/value_arena.hpp`, `detail/json/deep_copy.hpp`,
    `detail/json/utf8.hpp`, `detail/json/pointer.hpp`,
    `detail/json/equal.hpp`;
  - `test/equality_test.cpp`, `test/deep_copy_test.cpp`,
    `test/pointer_test.cpp`, `test/utf8_test.cpp`, `test/json_memory_test.cpp`.
- Modify: `test/Jamfile` (its own block), `meta/include-boundaries.json` (its
  own object).

**Interfaces:**
- Produces, in `webcpp::json_schema::detail`:

```cpp
// Boost.JSON's resource over a std::pmr one (spec 14)
class json_resource final : public boost::container::pmr::memory_resource {
public:
    explicit json_resource(std::pmr::memory_resource& upstream) noexcept;
};
// values json_schema owns: Boost.JSON's monotonic resource over a json_resource, so that their
// storage deallocates trivially and no destructor recurses (Boost.JSON skips element destruction
// for such storage)
template <class Allocator> class value_arena {   // not copyable, not movable; allocate_shared
public:
    explicit value_arena(const Allocator& a);
    boost::json::storage_ptr storage() noexcept;   // non-owning, trivially deallocating
    std::pmr::memory_resource& resource() noexcept;
};
boost::json::value deep_copy(const boost::json::value& v, boost::json::storage_ptr storage,
                             std::pmr::memory_resource& scratch);   // iterative
bool is_valid_utf8(std::string_view s) noexcept;
std::size_t code_points(std::string_view s) noexcept;
void append_token(std::pmr::string& out, std::string_view name);
void append_index(std::pmr::string& out, std::uint64_t index);
bool is_valid_pointer(std::string_view pointer) noexcept;
bool token_equals(std::string_view token, std::string_view key) noexcept;
const boost::json::value* resolve_pointer(const boost::json::value& root, std::string_view pointer) noexcept;
bool equal(const boost::json::value& a, const boost::json::value& b,
           std::pmr::memory_resource& scratch) noexcept;   // numbers by decompose_value; iterative
}
```

- [ ] **Step 1: Write the failing tests.**

`test/pointer_test.cpp`:

```cpp
#include <webcpp/json_schema/detail/json/pointer.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/json/parse.hpp>

#include <initializer_list>
#include <memory_resource>
#include <string>

namespace js = webcpp::json_schema;

namespace {

std::string tokens(std::initializer_list<std::string_view> names) {
    std::pmr::string out;
    for (const auto name : names) {
        js::detail::append_token(out, name);
    }
    return std::string(out);
}

void tokens_are_escaped() {
    BOOST_TEST_EQ(tokens({"a"}), "/a");
    BOOST_TEST_EQ(tokens({""}), "/");
    BOOST_TEST_EQ(tokens({"a/b"}), "/a~1b");
    BOOST_TEST_EQ(tokens({"~1"}), "/~01");
    BOOST_TEST_EQ(tokens({"~", "/"}), "/~0/~1");
    BOOST_TEST_EQ(tokens({std::string_view("a\0b", 3)}), std::string("/a\0b", 4));
    BOOST_TEST_EQ(tokens({"\xF0\x9F\x90\xB2"}), "/\xF0\x9F\x90\xB2");
    std::pmr::string indexed;
    js::detail::append_index(indexed, 0);
    js::detail::append_index(indexed, 18446744073709551615ULL);
    BOOST_TEST_EQ(std::string(indexed), "/0/18446744073709551615");
}

void pointers_resolve() {
    // RFC 6901 section 5
    const auto doc = boost::json::parse(R"({
        "foo": ["bar", "baz"],
        "": 0,
        "a/b": 1,
        "c%d": 2,
        "e^f": 3,
        "g|h": 4,
        "i\\j": 5,
        "k\"l": 6,
        " ": 7,
        "m~n": 8
    })");
    BOOST_TEST(js::detail::resolve_pointer(doc, "") == &doc);
    BOOST_TEST_EQ(*js::detail::resolve_pointer(doc, "/foo/0"), "bar");
    BOOST_TEST_EQ(*js::detail::resolve_pointer(doc, "/"), 0);
    BOOST_TEST_EQ(*js::detail::resolve_pointer(doc, "/a~1b"), 1);
    BOOST_TEST_EQ(*js::detail::resolve_pointer(doc, "/m~0n"), 8);
    BOOST_TEST(js::detail::resolve_pointer(doc, "/foo/2") == nullptr);
    BOOST_TEST(js::detail::resolve_pointer(doc, "/foo/01") == nullptr);
    BOOST_TEST(js::detail::resolve_pointer(doc, "/foo/-") == nullptr);
    BOOST_TEST(!js::detail::is_valid_pointer("foo"));
    BOOST_TEST(!js::detail::is_valid_pointer("/~2"));
    BOOST_TEST(!js::detail::is_valid_pointer("/~"));
    BOOST_TEST(js::detail::is_valid_pointer("/~0~1"));
}

}  // namespace

int main() {
    tokens_are_escaped();
    pointers_resolve();
    return boost::report_errors();
}
```

`test/utf8_test.cpp`:

```cpp
#include <webcpp/json_schema/detail/json/utf8.hpp>

#include <boost/core/lightweight_test.hpp>

namespace js = webcpp::json_schema;

int main() {
    BOOST_TEST_EQ(js::detail::code_points(""), 0U);
    BOOST_TEST_EQ(js::detail::code_points("abc"), 3U);
    BOOST_TEST_EQ(js::detail::code_points("\xC3\xA9"), 1U);
    BOOST_TEST_EQ(js::detail::code_points("\xF0\x9F\x90\xB2\xF0\x9F\x90\xB2"), 2U);
    BOOST_TEST(js::detail::is_valid_utf8("\xF0\x9F\x90\xB2"));
    BOOST_TEST(!js::detail::is_valid_utf8("\xC0\x80"));          // overlong
    BOOST_TEST(!js::detail::is_valid_utf8("\xED\xA0\x80"));      // a surrogate
    BOOST_TEST(!js::detail::is_valid_utf8("\xF4\x90\x80\x80"));  // past U+10FFFF
    BOOST_TEST(!js::detail::is_valid_utf8("\xE2\x82"));          // truncated
    BOOST_TEST(!js::detail::is_valid_utf8("\x80"));
    return boost::report_errors();
}
```

`test/equality_test.cpp`:

```cpp
#include <webcpp/json_schema/detail/json/equal.hpp>

#include "counting_resource.hpp"

#include <boost/core/lightweight_test.hpp>
#include <boost/json/monotonic_resource.hpp>
#include <boost/json/parse.hpp>

namespace js = webcpp::json_schema;

namespace {

bool eq(std::string_view a, std::string_view b) {
    std::pmr::monotonic_buffer_resource scratch;
    return js::detail::equal(boost::json::parse(a), boost::json::parse(b), scratch);
}

void values_compare_as_json_says() {
    BOOST_TEST(eq("1", "1.0"));
    BOOST_TEST(eq("-0.0", "0"));
    BOOST_TEST(!eq("1", "true"));
    BOOST_TEST(!eq("0", "false"));
    BOOST_TEST(!eq("null", "false"));
    BOOST_TEST(eq(R"({"a": 1, "b": [1, 2]})", R"({"b": [1.0, 2], "a": 1})"));
    BOOST_TEST(!eq("[1, 2]", "[2, 1]"));
    BOOST_TEST(!eq(R"({"a": 1})", R"({"a": 1, "b": 2})"));
    BOOST_TEST(!eq(R"("a")", R"("A")"));
    BOOST_TEST(eq("18446744073709551615", "18446744073709551615"));
    BOOST_TEST(!eq("9007199254740993", "9007199254740992"));
    BOOST_TEST(!eq("0.30000000000000004", "0.3"));
}

// A chain of one-element arrays keeps one pair on the work stack at a time.
boost::json::value chain(boost::json::monotonic_resource& storage, int depth) {
    boost::json::value v = boost::json::array(&storage);
    for (int i = 0; i < depth; ++i) {
        boost::json::array outer(&storage);
        outer.push_back(std::move(v));
        v = std::move(outer);
    }
    return v;
}

void depth_never_reaches_the_stack() {
    boost::json::monotonic_resource storage;
    std::pmr::monotonic_buffer_resource scratch;
    BOOST_TEST(js::detail::equal(chain(storage, 100000), chain(storage, 100000), scratch));
}

void width_spills_into_the_given_resource() {
    boost::json::array wide;
    for (int i = 0; i < 1000; ++i) {
        wide.push_back(boost::json::array{i, i});
    }
    const boost::json::value a = wide;
    const boost::json::value b = wide;
    webcpp::test::counting_resource counted;
    BOOST_TEST(js::detail::equal(a, b, counted));
    BOOST_TEST(counted.allocations > 0);
    BOOST_TEST_EQ(counted.live, 0U);
}

}  // namespace

int main() {
    values_compare_as_json_says();
    depth_never_reaches_the_stack();
    width_spills_into_the_given_resource();
    return boost::report_errors();
}
```

`test/deep_copy_test.cpp`:

```cpp
#include <webcpp/json_schema/detail/json/deep_copy.hpp>
#include <webcpp/json_schema/detail/json/equal.hpp>
#include <webcpp/json_schema/detail/json/value_arena.hpp>

#include "counting_resource.hpp"

#include <boost/core/lightweight_test.hpp>
#include <boost/json/monotonic_resource.hpp>
#include <boost/json/parse.hpp>

#include <memory>

namespace js = webcpp::json_schema;
using alloc = std::pmr::polymorphic_allocator<std::byte>;

namespace {

void a_copy_is_equal_and_lives_in_the_arena() {
    webcpp::test::counting_resource counted;
    std::pmr::monotonic_buffer_resource scratch;
    const auto source = boost::json::parse(R"({"a": [1, 2.5, "text long enough to allocate"], "b": {"c": null}})");
    {
        const auto arena = std::allocate_shared<js::detail::value_arena<alloc>>(alloc(&counted), alloc(&counted));
        const boost::json::value copy = js::detail::deep_copy(source, arena->storage(), scratch);
        BOOST_TEST(js::detail::equal(copy, source, scratch));
        BOOST_TEST(copy.storage().get() == arena->storage().get());
        BOOST_TEST(counted.allocations > 0);
    }
    BOOST_TEST_EQ(counted.live, 0U);
}

void a_deep_value_copies_and_dies_without_recursion() {
    boost::json::monotonic_resource source_storage;
    boost::json::value deep = boost::json::object(&source_storage);
    for (int i = 0; i < 100000; ++i) {
        boost::json::object outer(&source_storage);
        outer["items"] = std::move(deep);
        deep = std::move(outer);
    }
    std::pmr::monotonic_buffer_resource scratch;
    const auto arena = std::allocate_shared<js::detail::value_arena<alloc>>(alloc(), alloc());
    const boost::json::value copy = js::detail::deep_copy(deep, arena->storage(), scratch);
    BOOST_TEST(js::detail::equal(copy, deep, scratch));
}   // copy and arena die here: trivially deallocating storage, no recursive destructor

}  // namespace

int main() {
    a_copy_is_equal_and_lives_in_the_arena();
    a_deep_value_copies_and_dies_without_recursion();
    return boost::report_errors();
}
```

`test/json_memory_test.cpp`:

```cpp
#include <webcpp/json_schema/detail/json/json_resource.hpp>

#include "counting_resource.hpp"

#include <boost/core/lightweight_test.hpp>
#include <boost/json/parse.hpp>

namespace js = webcpp::json_schema;

int main() {
    webcpp::test::counting_resource counted;
    js::detail::json_resource adapter(counted);
    {
        const auto v = boost::json::parse(R"({"a": [1, 2, 3], "b": "a string long enough"})",
                                          boost::json::storage_ptr(&adapter));
        BOOST_TEST(counted.allocations > 0);
    }
    BOOST_TEST_EQ(counted.allocations, counted.deallocations);
    return boost::report_errors();
}
```

  Append to `test/Jamfile`:

```
webcpp.headers-alone json_schema : ../include : json_schema/detail/json/*.hpp ;
webcpp.run equality : equality_test.cpp ;
webcpp.run deep_copy : deep_copy_test.cpp ;
webcpp.run pointer : pointer_test.cpp ;
webcpp.run utf8 : utf8_test.cpp ;
webcpp.run json_memory : json_memory_test.cpp ;
```

  and one boundary object for `include/webcpp/json_schema/detail/json/*.hpp`
  with E1.1's four prefixes.
- [ ] **Step 2: Run them and watch them fail.**
  `$B2 libs/json_schema/test//equality libs/json_schema/test//deep_copy`.
  Expected: FAIL, `'webcpp/json_schema/detail/json/equal.hpp' file not found`
  and `'webcpp/json_schema/detail/json/deep_copy.hpp' file not found`.
- [ ] **Step 3: Implement.**

`detail/json/json_resource.hpp`:

```cpp
class json_resource final : public boost::container::pmr::memory_resource {
public:
    explicit json_resource(std::pmr::memory_resource& upstream) noexcept : upstream_(&upstream) {}

private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        return upstream_->allocate(bytes, alignment);
    }
    void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override {
        upstream_->deallocate(p, bytes, alignment);
    }
    bool do_is_equal(const boost::container::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
    std::pmr::memory_resource* upstream_;
};
```

`detail/json/value_arena.hpp`:

```cpp
template <class Allocator>
class value_arena {
public:
    explicit value_arena(const Allocator& a)
        : upstream_(a), adapter_(upstream_.get()), monotonic_(boost::json::storage_ptr(&adapter_)) {}
    value_arena(const value_arena&) = delete;
    value_arena& operator=(const value_arena&) = delete;
    boost::json::storage_ptr storage() noexcept { return boost::json::storage_ptr(&monotonic_); }
    std::pmr::memory_resource& resource() noexcept { return upstream_.get(); }

private:
    resource_of<Allocator> upstream_;
    json_resource adapter_;
    boost::json::monotonic_resource monotonic_;   // is_deallocate_trivial: no recursive destruction
};
```

  `storage_ptr(&monotonic_)` built from a `monotonic_resource*` carries
  Boost.JSON's trivial-deallocation flag, under which its arrays and objects
  skip their elements' destructors. A value in it dies without recursion.
  The arena is made with `std::allocate_shared` through its allocator, so
  its address never moves.

`detail/json/deep_copy.hpp`, post-order through Boost.JSON's own builder:

```cpp
inline boost::json::value deep_copy(const boost::json::value& v, boost::json::storage_ptr storage,
                                    std::pmr::memory_resource& scratch) {
    struct frame {
        const boost::json::value* value;
        std::size_t next;
    };
    boost::json::value_stack built(storage);
    built.reset(storage);
    inline_stack<frame, 32> work{std::pmr::polymorphic_allocator<frame>(&scratch)};
    work.push_back({.value = &v, .next = 0});
    while (!work.empty()) {
        frame& f = work.back();
        if (const auto* array = f.value->if_array()) {
            if (f.next < array->size()) {
                work.push_back({.value = &(*array)[f.next++], .next = 0});
                continue;
            }
            built.push_array(array->size());
        } else if (const auto* object = f.value->if_object()) {
            if (f.next < object->size()) {
                const auto& member = *(object->begin() + static_cast<std::ptrdiff_t>(f.next++));
                built.push_key(member.key());
                work.push_back({.value = &member.value(), .next = 0});
                continue;
            }
            built.push_object(object->size());
        } else {
            push_scalar(built, *f.value);   // null, bool, int64, uint64, double, string
        }
        work.pop_back();
    }
    return built.release();
}
```

  `push_scalar` is a six-case `switch` on `kind()`. `value_stack` keeps its
  own temporaries in `storage`.

`detail/json/utf8.hpp`, RFC 3629's table, one helper per lead byte range:

```cpp
/** The count of continuation bytes after `lead` and the bounds of the first one, or none. */
struct utf8_lead {
    std::size_t extra;
    unsigned char low;
    unsigned char high;
};

inline std::optional<utf8_lead> lead_of(unsigned char c) noexcept {
    if (c >= 0xC2 && c <= 0xDF) {
        return utf8_lead{.extra = 1, .low = 0x80, .high = 0xBF};
    }
    if (c >= 0xE0 && c <= 0xEF) {
        return utf8_lead{.extra = 2, .low = c == 0xE0 ? 0xA0 : 0x80, .high = c == 0xED ? 0x9F : 0xBF};
    }
    if (c >= 0xF0 && c <= 0xF4) {
        return utf8_lead{.extra = 3, .low = c == 0xF0 ? 0x90 : 0x80, .high = c == 0xF4 ? 0x8F : 0xBF};
    }
    return std::nullopt;
}

inline bool is_valid_utf8(std::string_view s) noexcept {
    std::size_t i = 0;
    while (i < s.size()) {
        const auto c = static_cast<unsigned char>(s[i]);
        if (c < 0x80) {
            ++i;
            continue;
        }
        const auto lead = lead_of(c);
        if (!lead || s.size() - i <= lead->extra) {
            return false;
        }
        for (std::size_t k = 1; k <= lead->extra; ++k) {
            const auto t = static_cast<unsigned char>(s[i + k]);
            if (t < (k == 1 ? lead->low : 0x80) || t > (k == 1 ? lead->high : 0xBF)) {
                return false;
            }
        }
        i += lead->extra + 1;
    }
    return true;
}

inline std::size_t code_points(std::string_view s) noexcept {
    return static_cast<std::size_t>(std::count_if(s.begin(), s.end(), [](char c) {
        const auto b = static_cast<unsigned char>(c);
        return b < 0x80 || b > 0xBF;
    }));
}
```

`detail/json/pointer.hpp`:

```cpp
inline void append_token(std::pmr::string& out, std::string_view name) {
    out.push_back('/');
    for (const char c : name) {
        if (c == '~') {
            out.append("~0");
        } else if (c == '/') {
            out.append("~1");
        } else {
            out.push_back(c);
        }
    }
}

inline void append_index(std::pmr::string& out, std::uint64_t index) {
    std::array<char, 24> digits{};
    const auto written = std::to_chars(digits.data(), digits.data() + digits.size(), index);
    out.push_back('/');
    out.append(digits.data(), written.ptr);
}

inline bool is_valid_pointer(std::string_view pointer) noexcept {
    if (!pointer.empty() && pointer.front() != '/') {
        return false;
    }
    for (std::size_t i = 0; i < pointer.size(); ++i) {
        const bool escape = pointer[i] == '~';
        if (escape && (i + 1 == pointer.size() || (pointer[i + 1] != '0' && pointer[i + 1] != '1'))) {
            return false;
        }
    }
    return true;
}

/** The array index a token spells (RFC 6901 section 4): digits, no leading zero, never "-". */
inline std::optional<std::size_t> index_of(std::string_view token) noexcept {
    if (token.empty() || (token.size() > 1 && token.front() == '0')) {
        return std::nullopt;
    }
    std::size_t value = 0;
    const auto parsed = std::from_chars(token.data(), token.data() + token.size(), value);
    if (parsed.ec != std::errc{} || parsed.ptr != token.data() + token.size()) {
        return std::nullopt;
    }
    return value;
}

/** Compares an escaped token with a key, reading "~0" as "~" and "~1" as "/", copying nothing. */
inline bool token_equals(std::string_view token, std::string_view key) noexcept {
    std::size_t k = 0;
    for (std::size_t t = 0; t < token.size(); ++t, ++k) {
        char c = token[t];
        if (c == '~') {
            c = token[++t] == '0' ? '~' : '/';
        }
        if (k == key.size() || key[k] != c) {
            return false;
        }
    }
    return k == key.size();
}

/** One step: the member of `object` whose key the escaped token spells, or nullptr. */
inline const boost::json::value* member_of(const boost::json::object& object, std::string_view token) noexcept {
    for (const auto& member : object) {
        if (token_equals(token, member.key())) {
            return &member.value();
        }
    }
    return nullptr;
}

inline const boost::json::value* resolve_pointer(const boost::json::value& root,
                                                 std::string_view pointer) noexcept {
    if (!is_valid_pointer(pointer)) {
        return nullptr;
    }
    const boost::json::value* at = &root;
    while (!pointer.empty() && at != nullptr) {
        pointer.remove_prefix(1);
        const std::size_t end = std::min(pointer.find('/'), pointer.size());
        const std::string_view token = pointer.substr(0, end);
        pointer.remove_prefix(end);
        if (const auto* object = at->if_object()) {
            at = member_of(*object, token);
        } else if (const auto* array = at->if_array()) {
            const auto index = index_of(token);
            at = index && *index < array->size() ? &(*array)[*index] : nullptr;
        } else {
            at = nullptr;
        }
    }
    return at;
}
```

`detail/json/equal.hpp`, iterative, one helper per JSON kind (clang-tidy's
cognitive complexity):

```cpp
struct value_pair {
    const boost::json::value* left;
    const boost::json::value* right;
};
using pair_stack = inline_stack<value_pair, 32>;

inline bool equal_arrays(const boost::json::array& l, const boost::json::array& r, pair_stack& work) {
    if (l.size() != r.size()) {
        return false;
    }
    for (std::size_t i = 0; i < l.size(); ++i) {
        work.push_back({.left = &l[i], .right = &r[i]});
    }
    return true;
}

inline bool equal_objects(const boost::json::object& l, const boost::json::object& r, pair_stack& work) {
    if (l.size() != r.size()) {
        return false;
    }
    for (const auto& member : l) {
        const auto* other = r.if_contains(member.key());
        if (other == nullptr) {
            return false;
        }
        work.push_back({.left = &member.value(), .right = other});
    }
    return true;
}

/** One pair: true when equal so far, pushing the pairs of its children. */
inline bool equal_step(const value_pair& p, pair_stack& work) noexcept {
    const auto left = decompose_value(*p.left);
    const auto right = decompose_value(*p.right);
    if (left || right) {
        return left && right && compare(*left, *right) == 0;
    }
    if (p.left->kind() != p.right->kind()) {
        return false;
    }
    switch (p.left->kind()) {
        case boost::json::kind::bool_: return p.left->get_bool() == p.right->get_bool();
        case boost::json::kind::string: return p.left->get_string() == p.right->get_string();
        case boost::json::kind::array: return equal_arrays(p.left->get_array(), p.right->get_array(), work);
        case boost::json::kind::object: return equal_objects(p.left->get_object(), p.right->get_object(), work);
        default: return true;   // null; numbers were decomposed above
    }
}

inline bool equal(const boost::json::value& a, const boost::json::value& b,
                  std::pmr::memory_resource& scratch) noexcept {
    pair_stack work{std::pmr::polymorphic_allocator<value_pair>(&scratch)};
    work.push_back({.left = &a, .right = &b});
    while (!work.empty()) {
        const value_pair p = work.back();
        work.pop_back();
        if (!equal_step(p, work)) {
            return false;
        }
    }
    return true;
}
```

  A container that runs out of memory raises `std::bad_alloc` from its
  resource, which aborts under `-fno-exceptions`. The functions are
  `noexcept` because json_schema reports no allocation failure as a value
  (spec 13).
- [ ] **Step 4: Run them and watch them pass** on the five lanes:

```bash
lanes libs/json_schema/test
lint_clean
```

  Expected: `lanes: green`, `lint: clean`.
- [ ] **Step 5: Commit.**

```bash
cd libs/json_schema
F="include/webcpp/json_schema/detail/json/json_resource.hpp include/webcpp/json_schema/detail/json/value_arena.hpp \
include/webcpp/json_schema/detail/json/deep_copy.hpp include/webcpp/json_schema/detail/json/utf8.hpp \
include/webcpp/json_schema/detail/json/pointer.hpp include/webcpp/json_schema/detail/json/equal.hpp \
meta/include-boundaries.json test/Jamfile test/equality_test.cpp test/deep_copy_test.cpp \
test/pointer_test.cpp test/utf8_test.cpp test/json_memory_test.cpp"
git add $F
$G commit -m "json_schema: JSON equality, iterative copies into a trivially freed arena, pointers, UTF-8" -- $F
```

### Task E1.4: The regex policy over Boost.Regex, through webcpp::lexical's ECMA-262 translator (to redo)

> **Status: to redo.** This task was done once, on 2026-10-10, over
> webcpp::regex, whose default backend was SRELL: the commit "json_schema:
> the regex policy over webcpp::regex, in both allocators, with its step
> budget" of the snapshot, whose files are
> `reference/source/include/webcpp/json_schema/regex/regex_policy.hpp`,
> `reference/source/include/webcpp/json_schema/regex/webcpp_regex.hpp` and
> `reference/source/test/regex_test.cpp`, with `build.jam` naming
> `/webcpp/regex//regex`. On 2026-10-12 the owner abandoned webcpp::regex and
> SRELL. `pattern` and `patternProperties` now run on Boost.Regex 1.92,
> header-only, through the ECMA-262 translator that webcpp::lexical carries
> for its `regex` format (research: `BOOST_REGEX_ECMASCRIPT_FEASIBILITY.md`,
> its prototype `ecma_regex.hpp`). This section is the redo; the done code
> stays in the snapshot as history and is replaced by this task.

After Task 0 of `tasks.md` (the snapshot brought up to the superproject's
current rules). Needs webcpp::lexical with a public matching interface
(below); formats is turning the prototype into production code, so this task
starts when formats publishes it, and stops and reports if formats exposes
only `is_regex` by then. json_schema never includes a header under
`webcpp/lexical/detail/`, and never copies the translator.

**What formats must provide** (coordinated with the formats track; the names
below are this plan's proposal, and the policy adapts to the names formats
publishes, never to its `detail`):

```cpp
namespace webcpp::lexical {
/** Why a pattern does not compile for matching. */
enum class regex_refusal : std::uint8_t {
    invalid,         // not ECMA-262 Pattern[+UnicodeMode]: a SyntaxError in JavaScript
    beyond_bounds,   // nested past max_regex_nesting, or past another bound of the translator
    unsupported,     // valid ECMA-262 that Boost.Regex cannot run as ECMA-262 runs it: an
                     // unbounded lookbehind that is not leading, lookbehind alternatives of
                     // different widths inside a group, a backreference into a quantified group
                     // or inside a lookbehind
};
/** A pattern compiled once: the translation, compiled by Boost.Regex over code points. */
class ecma_regex;
/** The translator's memory from `resource`; Boost.Regex's own from the global operator new. */
boost::system::result<ecma_regex> compile_regex(std::string_view pattern,
                                                std::pmr::memory_resource& resource);
enum class search_result : std::uint8_t { found, not_found, limit };
/** Unanchored search over well-formed UTF-8; the step budget counts iterator steps
    (each *, ++, --, == of the UTF-8 iterator); 0 is the default budget, 10,000,000 steps
    plus 64 per byte of the text; match_results' memory from `resource`. */
search_result search(const ecma_regex& re, std::string_view text, std::size_t step_budget,
                     std::pmr::memory_resource& resource);
}
```

  formats also owns Boost.Regex's configuration, as usage requirements of
  `/webcpp/lexical//lexical` so that every translation unit sees the same
  values: `BOOST_REGEX_MAX_BLOCKS` raised so the backtracking stack ends
  only at the step budget, and, with exceptions, Boost's own
  `error_complexity` and `error_stack` caught inside `search` and answered
  `limit`.

**Files:**
- Create: `include/webcpp/json_schema/regex/boost_regex.hpp`.
- Rewrite: `test/regex_test.cpp`.
- Delete: `include/webcpp/json_schema/regex/webcpp_regex.hpp`.
- Modify:
  - `include/webcpp/json_schema/regex/regex_policy.hpp` (the Doc Comments
    name Boost.Regex; the concept is unchanged);
  - `include/webcpp/json_schema/traits.hpp` (`default_traits::regex_policy`
    is `boost_regex`);
  - `include/webcpp/json_schema/errors.hpp` (`errc::unsupported_regex = 21`,
    appended, and its message);
  - `include/webcpp/json_schema/validator/options.hpp` (the Doc Comment of
    `regex_step_budget`: iterator steps per search, 0 for the default);
  - `include/webcpp/json_schema.hpp` (its regex block);
  - `build.jam` (`<library>/webcpp/regex//regex` becomes
    `<library>/webcpp/lexical//lexical`, and its comment);
  - `meta/include-boundaries.json` (every `webcpp/regex` and `srell` entry
    becomes `boost/regex`; `regex/boost_regex.hpp` is the one header allowed
    `webcpp/lexical/regex.hpp`, alongside `formats/default_formats.hpp`, which
    later reaches the rest of formats);
  - `test/Jamfile` (its regex block), `test/errors_test.cpp` (the new value),
    `README.md` and `AGENTS.md` (the SRELL caution becomes the Boost.Regex
    one: what it allocates outside the allocators, and its known gaps).

**Interfaces:**
- Consumes: E1.1's `resource_of` and `test/counting_resource.hpp`, E1.1's
  `errc`, formats' interface above.
- Produces:

```cpp
namespace webcpp::json_schema {
enum class match_result : std::uint8_t { found, not_found, limit };   // unchanged
template <class R, class SchemaAlloc, class ValidationAlloc>
concept regex_policy = /* unchanged: compile(text, schema_allocator, step_budget),
                          search(re, text, validation_allocator) */;
struct boost_regex {
    template <class Allocator>
    class compiled;   // holds lexical::ecma_regex and the step budget; movable, not copyable
    template <class Allocator>
    static boost::system::result<compiled<Allocator>> compile(std::string_view text,
                                                              const Allocator& allocator,
                                                              std::size_t step_budget);
    template <class Allocator, class ValidationAlloc>
    static match_result search(const compiled<Allocator>& re, std::string_view text,
                               const ValidationAlloc& allocator);
};
}
```

  - `compile` gives the translator the schema's allocator through
    `detail::resource_of<Allocator>`; formats' `invalid` is
    `errc::invalid_regex`; `beyond_bounds` and `unsupported` are
    `errc::unsupported_regex` (the compiler reports either at the keyword's
    location with the pattern as `what`).
  - `search` keeps match_results' state on the stack up to
    `detail::regex_inline_bytes`, then in the validation's allocator, and
    maps formats' `limit` to `match_result::limit`, which the VM reports as
    `error_kind::pattern_limit`.
  - **Allocation, stated:** Boost.Regex allocates outside any allocator: 7
    to 9 global allocations per compiled pattern (its `basic_regex`), and
    its backtracking stack in 4 KiB blocks from the global `operator new`,
    up to 16 kept in a process-wide cache (boostorg/regex#258). So the
    two-allocator guarantee and "`is_valid` allocates nothing on the common
    path" hold for schemas without `pattern` and `patternProperties`, and the
    page states the exception, as it states the user's resolver's and
    hook's. The allocation test counts zero global allocations on the
    suite's schemas without patterns, and measures, not hides, the others.

- [ ] **Step 1: Write the failing test** `test/regex_test.cpp` (replacing the
  one over webcpp::regex):

```cpp
// The regex policy over Boost.Regex, through formats' ECMA-262 translator: ECMAScript's
// semantics in u mode, unanchored, on UTF-8; refusals as values, invalid and unsupported apart;
// the step budget reaching the search; a runaway search a limit, never an abort; the translator's
// memory in the schema's allocator and the search's state in the validation's.

#include <webcpp/json_schema/regex/boost_regex.hpp>

#include "counting_resource.hpp"
#include "recording_resource.hpp"

#include <boost/core/lightweight_test.hpp>

#include <cstddef>
#include <memory_resource>
#include <string>

namespace js = webcpp::json_schema;
using alloc = std::pmr::polymorphic_allocator<std::byte>;

static_assert(js::regex_policy<js::boost_regex, alloc, alloc>);
static_assert(
    js::regex_policy<js::boost_regex, alloc, webcpp::test::recording_allocator<std::byte>>);

namespace {

js::match_result search(std::string_view pattern, std::string_view text,
                        std::size_t budget = 0) {
    std::pmr::monotonic_buffer_resource schema;
    auto re = js::boost_regex::compile(pattern, alloc(&schema), budget);
    if (!BOOST_TEST(re.has_value())) {
        return js::match_result::limit;
    }
    std::pmr::monotonic_buffer_resource validation;
    return js::boost_regex::search(*re, text, alloc(&validation));
}

boost::system::error_code refusal(std::string_view pattern) {
    std::pmr::monotonic_buffer_resource schema;
    const auto re = js::boost_regex::compile(pattern, alloc(&schema), 0);
    return re.has_error() ? re.error() : boost::system::error_code();
}

void ecmascript_semantics() {
    BOOST_TEST(search("^[a-z]+$", "abc") == js::match_result::found);
    BOOST_TEST(search("^[a-z]+$", "ab1") == js::match_result::not_found);
    BOOST_TEST(search("b", "abc") == js::match_result::found);  // unanchored
    BOOST_TEST(search("^abc$", "abc\n") == js::match_result::not_found);
    BOOST_TEST(search("^.$", "\xF0\x9F\x90\xB2") == js::match_result::found);
    BOOST_TEST(search("^\\p{Letter}+$", "\xC3\x89\x63ole") == js::match_result::found);
    BOOST_TEST(search("(?<=a+)b", "aab") == js::match_result::found);  // leftmost quantifier
    BOOST_TEST(search("(?:ab){1,2}", "ab") == js::match_result::found);  // SRELL 2026.06's miss
    BOOST_TEST(search("^(?:[0-9]{3}-){1,2}[0-9]{4}$", "123-4567") == js::match_result::found);
}

void refusals_are_values() {
    BOOST_TEST(refusal("(") == js::errc::invalid_regex);
    BOOST_TEST(refusal("a{,5}") == js::errc::invalid_regex);
    BOOST_TEST(refusal("(?i)a") == js::errc::invalid_regex);
    BOOST_TEST(refusal("\\Z") == js::errc::invalid_regex);
    BOOST_TEST(refusal("(?<=\\$\\d*)x") == js::errc::unsupported_regex);  // unbounded, not leading
    BOOST_TEST(refusal("^(?:(a)|b)+\\1$") == js::errc::unsupported_regex);  // into a repeat
    const std::string nested = std::string(10000, '(') + "a" + std::string(10000, ')');
    BOOST_TEST(refusal(nested) == js::errc::unsupported_regex);  // past formats' nesting
}

void the_step_budget_reaches_the_search() {
    BOOST_TEST(search("(a*)*b", std::string(40, 'a'), 1'000) == js::match_result::limit);
    BOOST_TEST(search("^[a-z]+$", std::string(1'000'000, 'a')) == js::match_result::found);
}

void a_runaway_search_is_a_limit() {
    BOOST_TEST(search("(a*)*b", std::string(40, 'a')) == js::match_result::limit);
    BOOST_TEST(search("(x+x+)+y", std::string(30, 'x')) == js::match_result::limit);
    BOOST_TEST(
        search("^(\\w+\\s?)*$", "A long sentence of words that ends with something refused!") ==
        js::match_result::limit);
    // Boost's backtracking stack, raised by formats' BOOST_REGEX_MAX_BLOCKS: found or a limit,
    // never an abort.
    BOOST_TEST(search("^(a|bc)*$", std::string(100'000, 'a')) != js::match_result::not_found);
}

void storage_goes_where_it_is_told() {
    webcpp::test::counting_resource schema;
    webcpp::test::counting_resource validation;
    webcpp::test::counting_resource elsewhere;
    std::pmr::memory_resource* const previous = std::pmr::set_default_resource(&elsewhere);
    auto re = js::boost_regex::compile("^[a-z]+$", alloc(&schema), 0);
    if (!BOOST_TEST(re.has_value())) {
        std::pmr::set_default_resource(previous);
        return;
    }
    BOOST_TEST(schema.allocations > 0);  // the translator's
    BOOST_TEST(js::boost_regex::search(*re, "abc", alloc(&validation)) ==
               js::match_result::found);
    BOOST_TEST_EQ(validation.live, 0U);
    std::pmr::set_default_resource(previous);
    BOOST_TEST_EQ(elsewhere.allocations, 0U);
}

}  // namespace

int main() {
    ecmascript_semantics();
    refusals_are_values();
    the_step_budget_reaches_the_search();
    a_runaway_search_is_a_limit();
    storage_goes_where_it_is_told();
    return boost::report_errors();
}
```

  The test's regex block in `test/Jamfile` stays:

```
webcpp.headers-alone json_schema : ../include : json_schema/regex/*.hpp ;
webcpp.run regex : regex_test.cpp ;
```

  and `meta/include-boundaries.json` keeps one object per regex header:
  - `regex/regex_policy.hpp` must not include `boost/regex`,
    `webcpp/lexical`, `boost/locale`, `boost/url`;
  - `regex/boost_regex.hpp` must not include `boost/locale`, `boost/url`,
    and includes formats only as `webcpp/lexical/regex.hpp`, with the `why`
    "Boost.Regex is reached only through formats' translator, through this
    policy".
- [ ] **Step 2: Run it and watch it fail.** `$B2 libs/json_schema/test//regex`.
  Expected: FAIL, `'webcpp/json_schema/regex/boost_regex.hpp' file not found`.
- [ ] **Step 3: Implement.**

```cpp
struct boost_regex {
    template <class Allocator>
    class compiled {
    public:
        compiled(lexical::ecma_regex re, std::size_t step_budget) noexcept
            : re_(std::move(re)), step_budget_(step_budget) {}
        const lexical::ecma_regex& regex() const noexcept { return re_; }
        std::size_t step_budget() const noexcept { return step_budget_; }

    private:
        lexical::ecma_regex re_;
        std::size_t step_budget_;
    };

    template <class Allocator>
    static boost::system::result<compiled<Allocator>> compile(std::string_view text,
                                                              const Allocator& allocator,
                                                              std::size_t step_budget) {
        detail::resource_of<Allocator> resource(allocator);
        auto re = lexical::compile_regex(text, resource.get());
        if (!re) {
            return make_error_code(re.error() == lexical::regex_refusal::invalid
                                       ? errc::invalid_regex
                                       : errc::unsupported_regex);
        }
        return compiled<Allocator>(std::move(*re), step_budget);
    }

    template <class Allocator, class ValidationAlloc>
    static match_result search(const compiled<Allocator>& re, std::string_view text,
                               const ValidationAlloc& allocator) {
        detail::resource_of<ValidationAlloc> upstream(allocator);
        // Uninitialized on purpose: the monotonic resource writes before it reads.
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        std::array<std::byte, detail::regex_inline_bytes> buffer;
        std::pmr::monotonic_buffer_resource local(buffer.data(), buffer.size(), &upstream.get());
        switch (lexical::search(re.regex(), text, re.step_budget(), local)) {
            case lexical::search_result::found: return match_result::found;
            case lexical::search_result::not_found: return match_result::not_found;
            case lexical::search_result::limit: return match_result::limit;
        }
        return match_result::limit;
    }
};
```

  - The translator's memory lives in the schema's resource for as long as
    the compiled pattern does: if formats' `ecma_regex` keeps the resource
    it was given, the `compiled` here holds that resource the way the done
    `webcpp_regex` did (an `allocate_shared` resource that never moves),
    which the snapshot's `regex_test` pinned.
  - No Boost.Regex macro is defined here: formats' target carries them.
  - The page's "Regular expressions" section states the engine, the unit of
    `regex_step_budget`, the allocation exception, the refusals as
    `unsupported_regex` and the residual match-time abort on wasm32 (Boost's
    fixed state cap for a non-random-access iterator, reachable by a long
    legitimate search; feasibility section 5.3).
- [ ] **Step 4: Run it and watch it pass** on the five lanes:

```bash
lanes libs/json_schema/test
lint_clean
```

  Expected: `lanes: green`, `lint: clean`. Every other test still passes:
  the compiler (E1.10) reaches the policy only through `regex_policy`.
- [ ] **Step 5: Commit.**

```bash
cd libs/json_schema
F="build.jam README.md AGENTS.md include/webcpp/json_schema.hpp \
include/webcpp/json_schema/errors.hpp include/webcpp/json_schema/traits.hpp \
include/webcpp/json_schema/validator/options.hpp \
include/webcpp/json_schema/regex/regex_policy.hpp include/webcpp/json_schema/regex/boost_regex.hpp \
include/webcpp/json_schema/regex/webcpp_regex.hpp \
meta/include-boundaries.json test/Jamfile test/errors_test.cpp test/regex_test.cpp"
git add $F
$G commit -m "json_schema: patterns on Boost.Regex through formats' ECMA-262 translator" -- $F
```

### Task E1.5: Errors, annotations, events, the record and the two sinks

After E1.2; runs alongside E1.3 and E1.4 (slug `sinks`). `error.hpp`,
`event.hpp`, `sink.hpp` and `record.hpp` freeze at Task E1.17.

**Files:**
- Create:
  - `include/webcpp/json_schema/output/error.hpp`;
  - `include/webcpp/json_schema/detail/vm/event.hpp`, `detail/vm/sink.hpp`,
    `detail/vm/flag_sink.hpp`, `detail/vm/collecting_sink.hpp`;
  - `include/webcpp/json_schema/detail/output/record.hpp`;
  - `test/sink_test.cpp`.
- Modify: `include/webcpp/json_schema.hpp`, `test/Jamfile`,
  `meta/include-boundaries.json`.

**Interfaces:**
- Consumes: E1.1's `inline_stack`.
- Produces `output/error.hpp`:
  - spec 4.6's `error_kind`, `error_details`, `error` and `annotation`, with
    the accessors spec 4.6 lists;
  - `enum class collect : std::uint8_t { errors, errors_and_annotations,
    everything };`. It lives here, not in `output/outcome.hpp` as spec 3
    places it, because the collecting sink needs it first;
    `output/outcome.hpp` includes this header, so a program sees no
    difference;
  - `namespace detail { struct error_access; }`, the one way the engine
    builds an error or an annotation. `error` and `annotation` keep their
    fields private, with `friend struct detail::error_access;`.
- Produces `detail/vm/event.hpp` and `detail/vm/sink.hpp`:

```cpp
namespace webcpp::json_schema::detail {

/** One step of a path: a keyword or a property name, or an array index. */
struct path_token {
    std::string_view name;      // empty with is_index
    std::uint64_t index = 0;
    bool is_index = false;
};

/** Where an evaluation stands; a sink renders pointers from it only when it records. */
struct event_context {
    std::span<const path_token> evaluation_path;  // keywords from the root, $ref segments kept
    std::span<const path_token> instance_path;
    std::string_view resource_uri;                // absolute, without fragment
    std::string_view schema_pointer;              // the node's JSON Pointer in its resource
};

template <class S>
concept evaluation_sink = requires(S& sink, const event_context& at, const error& e,
                                   const annotation& a, typename S::mark m, bool valid) {
    { S::records } -> std::convertible_to<bool>;
    sink.enter(at);
    sink.leave(at, valid);
    sink.fail(e);
    sink.annotate(a);
    { sink.checkpoint() } -> std::same_as<typename S::mark>;
    sink.discard(m);
};

}  // namespace webcpp::json_schema::detail
```

  `sink.hpp`'s comment states the protocol the VM keeps, frozen with the
  concept:
  - **P1.** Every evaluation of a schema node at an instance location is
    bracketed by `enter` and `leave`, properly nested.
  - **P2.** On `leave(at, false)`, every annotation recorded since the
    matching `enter` no longer counts (2020-12 core 7.7.1.2). The sink
    applies this itself.
  - **P3.** `discard(m)`: everything recorded since `m`, errors and
    annotations, no longer counts. The VM calls it in LIFO order, never with
    a mark older than one it already discarded to. A sink that collects
    everything marks what follows `m`, and never removes it.
  - **P4.** The VM calls `discard` after a subschema whose records do not
    count:
    - the condition of `if` when it failed;
    - an item that does not match `contains`;
    - the operand of `not`, whatever its result;
    - an alternative of `anyOf` or `oneOf` that failed, unless that
      applicator is one whose failed alternatives count.

    An applicator's failed alternatives count when it fails with no passing
    alternative. The VM knows which applicators those are without nested
    re-evaluation:
    - `validate` runs the plan a first time, discarding every failed
      alternative and noting, for each `anyOf` and `oneOf` evaluation in
      order, whether it so failed;
    - only when the instance is invalid and one did, it runs the plan once
      more from the top, keeping those applicators' failed alternatives.
    - evaluation is deterministic, so the two runs meet the applicators in
      the same order. `validate` runs the plan at most twice.
  - **P5.** `fail` and `annotate` receive views valid only during the call.
    A sink that keeps them copies them.
  - **P6.** With `records == false`, the VM calls none of the six and
    short-circuits:
    - a block stops at its first failing assertion;
    - `allOf` stops at its first failing subschema;
    - `anyOf` stops at its first passing one, unless the compiler marked it
      exhaustive;
    - `oneOf` stops at its second passing one.
- Produces `detail/output/record.hpp` (frozen):

```cpp
namespace webcpp::json_schema::detail {
inline constexpr std::uint32_t no_unit = 0xFFFF'FFFFu;
/** One entered node: its locations, its parent, its result, and whether it counts. */
struct unit {
    std::string_view keyword_location;
    std::string_view absolute_keyword_location;
    std::string_view instance_location;
    std::uint32_t parent;   // no_unit for a root
    bool valid;
    bool counts;            // false once discarded; kept only under collect::everything
};
/** What a validation recorded, in the validation's resource; neither copyable nor movable. */
class record {
public:
    record(std::pmr::memory_resource& resource, collect what);
    record(const record&) = delete;
    record& operator=(const record&) = delete;
    collect collected() const noexcept;
    // what counts, in evaluation order, complete once finish() has run
    std::span<const error> errors() const noexcept;
    std::span<const std::uint32_t> error_units() const noexcept;
    std::span<const annotation> annotations() const noexcept;
    std::span<const std::uint32_t> annotation_units() const noexcept;
    // everything recorded; under collect::everything the discarded entries stay, flagged 0
    std::span<const unit> units() const noexcept;
    std::span<const error> recorded_errors() const noexcept;
    std::span<const std::uint32_t> recorded_error_units() const noexcept;
    std::span<const std::uint8_t> recorded_error_counts() const noexcept;
    std::span<const annotation> recorded_annotations() const noexcept;
    std::span<const std::uint32_t> recorded_annotation_units() const noexcept;
    std::span<const std::uint8_t> recorded_annotation_counts() const noexcept;
    std::pmr::memory_resource& arena() noexcept;      // monotonic over the given resource
    boost::json::storage_ptr storage() noexcept;      // values the engine computes (annotations)
    // the collecting sink's side
    struct position { std::uint32_t units; std::uint32_t errors; std::uint32_t annotations; };
    position here() const noexcept;
    std::uint32_t open_unit(std::string_view keyword_location, std::string_view absolute,
                            std::string_view instance_location);   // copies into the arena
    void close_unit(bool valid);
    std::uint32_t current_unit() const noexcept;
    void add_error(const error& e);            // copies its strings and details into the arena
    void add_annotation(const annotation& a);  // copies its strings; the value by deep_copy
    void discard_since(position p);            // truncates, or marks under collect::everything
    void discard_annotations_since(std::uint32_t annotations);   // P2, the same way
    void finish();                             // gathers what counts into the four spans above
};
}
```

  - The arena is a `std::pmr::monotonic_buffer_resource` over the given
    resource.
  - `storage()` is a `value_arena`-like Boost.JSON monotonic resource over a
    `json_resource` on the arena, so computed annotation values are freed
    without recursion.
  - The sinks:

```cpp
namespace webcpp::json_schema::detail {
struct flag_sink {
    static constexpr bool records = false;
    struct mark {};
    void enter(const event_context&) noexcept {}
    void leave(const event_context&, bool) noexcept {}
    void fail(const error&) noexcept {}
    void annotate(const annotation&) noexcept {}
    mark checkpoint() noexcept { return {}; }
    void discard(mark) noexcept {}
};
class collecting_sink {
public:
    static constexpr bool records = true;
    using mark = record::position;
    explicit collecting_sink(record& into) noexcept;
    void enter(const event_context& at);
    void leave(const event_context& at, bool valid);
    void fail(const error& e);
    void annotate(const annotation& a);
    mark checkpoint() const noexcept;
    void discard(mark m);
};
}
```

  The collecting sink renders the three locations into scratch strings of
  the record's arena:
  - the evaluation path as a JSON Pointer;
  - the instance path as a JSON Pointer;
  - the absolute location as `resource_uri + "#" + schema_pointer`.

  It keeps a stack of the open units' annotation counts
  (`inline_stack<std::uint32_t, 32>` in the arena) for P2.

- [ ] **Step 1: Write the failing test** `test/sink_test.cpp`:

```cpp
#include <webcpp/json_schema/detail/output/record.hpp>
#include <webcpp/json_schema/detail/vm/collecting_sink.hpp>
#include <webcpp/json_schema/detail/vm/flag_sink.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/json/value.hpp>

#include <array>
#include <memory_resource>
#include <string>

namespace js = webcpp::json_schema;
namespace detail = js::detail;

static_assert(detail::evaluation_sink<detail::flag_sink>);
static_assert(detail::evaluation_sink<detail::collecting_sink>);
static_assert(!detail::flag_sink::records);

namespace {

const boost::json::value schema_value = 3;
const boost::json::value instance_value = "abcd";

js::error make_error(std::string_view keyword_location) {
    return detail::error_access::make_error({.kind = js::error_kind::max_length,
                                             .keyword = "maxLength",
                                             .instance_location = "/name",
                                             .keyword_location = keyword_location,
                                             .absolute_keyword_location = "urn:webcpp:root#/maxLength",
                                             .expected = &schema_value,
                                             .received = &instance_value,
                                             .details = {}});
}

js::annotation make_annotation() {
    return detail::error_access::make_annotation({.keyword = "title",
                                                  .instance_location = "",
                                                  .keyword_location = "/title",
                                                  .absolute_keyword_location = "urn:webcpp:root#/title",
                                                  .value = &schema_value});
}

detail::event_context at(std::span<const detail::path_token> evaluation) {
    return {.evaluation_path = evaluation, .instance_path = {}, .resource_uri = "urn:webcpp:root",
            .schema_pointer = ""};
}

void error_kinds_are_fixed() {
    BOOST_TEST_EQ(static_cast<int>(js::error_kind::false_schema), 1);
    BOOST_TEST_EQ(static_cast<int>(js::error_kind::type), 2);
    BOOST_TEST_EQ(static_cast<int>(js::error_kind::pattern_limit), 13);
    BOOST_TEST_EQ(static_cast<int>(js::error_kind::unevaluated_properties), 33);
    BOOST_TEST_EQ(static_cast<int>(js::error_kind::reference), 39);
    BOOST_TEST_EQ(static_cast<int>(js::error_kind::depth_limit), 43);
}

void errors_are_copied_and_located() {
    std::pmr::monotonic_buffer_resource resource;
    detail::record record(resource, js::collect::errors);
    detail::collecting_sink sink(record);
    const std::array<detail::path_token, 2> path{detail::path_token{.name = "properties"},
                                                 detail::path_token{.name = "a/b"}};
    sink.enter(at(path));
    {
        std::string scratch = "/properties/name/maxLength";   // freed before the record is read
        sink.fail(make_error(scratch));
    }
    sink.leave(at(path), false);
    record.finish();
    if (!BOOST_TEST_EQ(record.errors().size(), 1U)) {
        return;
    }
    BOOST_TEST_EQ(record.errors()[0].keyword_location(), "/properties/name/maxLength");
    BOOST_TEST(&record.errors()[0].received() == &instance_value);
    BOOST_TEST_EQ(record.units()[0].keyword_location, "/properties/a~1b");
    BOOST_TEST(!record.units()[0].valid);
}

void a_failed_node_drops_its_annotations_and_keeps_its_errors() {
    std::pmr::monotonic_buffer_resource resource;
    detail::record record(resource, js::collect::errors_and_annotations);
    detail::collecting_sink sink(record);
    sink.enter(at({}));
    sink.annotate(make_annotation());
    sink.fail(make_error("/maxLength"));
    sink.leave(at({}), false);
    record.finish();
    BOOST_TEST_EQ(record.annotations().size(), 0U);
    BOOST_TEST_EQ(record.errors().size(), 1U);
}

void discard_truncates_unless_everything_is_collected() {
    for (const js::collect what : {js::collect::errors_and_annotations, js::collect::everything}) {
        std::pmr::monotonic_buffer_resource resource;
        detail::record record(resource, what);
        detail::collecting_sink sink(record);
        sink.enter(at({}));
        sink.annotate(make_annotation());
        const auto mark = sink.checkpoint();
        sink.enter(at({}));
        sink.fail(make_error("/anyOf/0/maxLength"));
        sink.leave(at({}), false);
        sink.discard(mark);
        sink.leave(at({}), true);
        record.finish();
        BOOST_TEST_EQ(record.errors().size(), 0U);
        BOOST_TEST_EQ(record.annotations().size(), 1U);
        if (what == js::collect::everything) {
            BOOST_TEST_EQ(record.units().size(), 2U);       // the failed branch stays, marked
            BOOST_TEST(!record.units()[1].counts && !record.units()[1].valid);
            BOOST_TEST_EQ(record.recorded_errors().size(), 1U);
            BOOST_TEST_EQ(record.recorded_error_counts()[0], 0U);
        } else {
            BOOST_TEST_EQ(record.units().size(), 1U);
        }
    }
}

void collect_errors_records_no_annotation() {
    std::pmr::monotonic_buffer_resource resource;
    detail::record record(resource, js::collect::errors);
    detail::collecting_sink sink(record);
    sink.enter(at({}));
    sink.annotate(make_annotation());
    sink.leave(at({}), true);
    record.finish();
    BOOST_TEST_EQ(record.annotations().size(), 0U);
}

}  // namespace

int main() {
    error_kinds_are_fixed();
    errors_are_copied_and_located();
    a_failed_node_drops_its_annotations_and_keeps_its_errors();
    discard_truncates_unless_everything_is_collected();
    collect_errors_records_no_annotation();
    return boost::report_errors();
}
```

  `detail::error_access` takes two aggregates, so that the call sites name
  each field:

```cpp
struct error_fields {
    error_kind kind;
    std::string_view keyword;
    std::string_view instance_location;
    std::string_view keyword_location;
    std::string_view absolute_keyword_location;
    const boost::json::value* expected;
    const boost::json::value* received;
    error_details details;
};
struct annotation_fields {
    std::string_view keyword;
    std::string_view instance_location;
    std::string_view keyword_location;
    std::string_view absolute_keyword_location;
    const boost::json::value* value;
};
struct error_access {
    static error make_error(const error_fields& f) noexcept;
    static annotation make_annotation(const annotation_fields& f) noexcept;
    static error copy_into(const error& e, std::pmr::memory_resource& arena);
    static annotation copy_into(const annotation& a, std::pmr::memory_resource& arena,
                                boost::json::storage_ptr values, std::pmr::memory_resource& scratch);
};
```

  Append to `test/Jamfile`:

```
webcpp.headers-alone json_schema : ../include : json_schema/output/error.hpp ;
webcpp.headers-alone json_schema : ../include : json_schema/detail/vm/*.hpp json_schema/detail/output/*.hpp ;
webcpp.run sink : sink_test.cpp ;
```

  The `output/` directory takes its headers by name, one call each, so that
  `output/locale.hpp`, which needs exceptions, can later have a call of its
  own. Append one boundary object for `output/error.hpp`, `detail/vm/*.hpp`
  and `detail/output/*.hpp`, with E1.1's four prefixes.
- [ ] **Step 2: Run it and watch it fail.** `$B2 libs/json_schema/test//sink`.
  Expected: FAIL, `'webcpp/json_schema/detail/output/record.hpp' file not
  found`.
- [ ] **Step 3: Implement** as the Interfaces say. The heart of the record:

```cpp
inline void record::discard_since(position p) {
    if (what_ != collect::everything) {
        units_.resize(p.units);
        errors_.resize(p.errors);
        error_units_.resize(p.errors);
        error_counts_.resize(p.errors);
        annotations_.resize(p.annotations);
        annotation_units_.resize(p.annotations);
        annotation_counts_.resize(p.annotations);
        return;
    }
    for (std::size_t i = p.units; i < units_.size(); ++i) {
        units_[i].counts = false;
    }
    std::fill(error_counts_.begin() + p.errors, error_counts_.end(), std::uint8_t{0});
    std::fill(annotation_counts_.begin() + p.annotations, annotation_counts_.end(), std::uint8_t{0});
}

inline void record::finish() {
    counting_errors_.clear();
    counting_error_units_.clear();
    for (std::size_t i = 0; i < errors_.size(); ++i) {
        if (error_counts_[i] != 0) {
            counting_errors_.push_back(errors_[i]);
            counting_error_units_.push_back(error_units_[i]);
        }
    }
    counting_annotations_.clear();
    counting_annotation_units_.clear();
    for (std::size_t i = 0; i < annotations_.size(); ++i) {
        if (annotation_counts_[i] != 0) {
            counting_annotations_.push_back(annotations_[i]);
            counting_annotation_units_.push_back(annotation_units_[i]);
        }
    }
}
```

  `discard_annotations_since` truncates or zeroes `annotation_counts_` from
  the given position, by the same rule. The vectors are
  `std::pmr::vector`s in the arena.
- [ ] **Step 4: Run it and watch it pass** on the five lanes:

```bash
lanes libs/json_schema/test
lint_clean
```

  Expected: `lanes: green`, `lint: clean`.
- [ ] **Step 5: Commit.**

```bash
cd libs/json_schema
F="include/webcpp/json_schema.hpp include/webcpp/json_schema/output/error.hpp \
include/webcpp/json_schema/detail/vm/event.hpp include/webcpp/json_schema/detail/vm/sink.hpp \
include/webcpp/json_schema/detail/vm/flag_sink.hpp include/webcpp/json_schema/detail/vm/collecting_sink.hpp \
include/webcpp/json_schema/detail/output/record.hpp meta/include-boundaries.json test/Jamfile test/sink_test.cpp"
git add $F
$G commit -m "json_schema: errors, annotations, the evaluation events, the record and the two sinks" -- $F
```

  Then, from `~/dev/webcpp-json-schema-e1`: `merge_task 1 json`, `merge_task
  1 regex`, `merge_task 1 sinks`. Expected after each: `lanes: green`,
  `lint: clean`, a fast-forward of `e1`. A conflict in a shared file is
  resolved by keeping both sides.

### Task E1.6: Traits, dialects and exact JSON text

After E1.3, E1.4 and E1.5 are merged into `e1`.

**Files:**
- Create:
  - `include/webcpp/json_schema/traits.hpp`;
  - `include/webcpp/json_schema/dialects/dialect.hpp`;
  - `include/webcpp/json_schema/json/parse.hpp`;
  - `include/webcpp/json_schema/detail/json/exact_handler.hpp`;
  - `test/dialect_test.cpp`, `test/parse_test.cpp`.
- Modify: `include/webcpp/json_schema.hpp`, `test/Jamfile`,
  `meta/include-boundaries.json`.

**Interfaces:**
- Produces:

```cpp
namespace webcpp::json_schema {
struct default_traits {   // the URL policy joins with Boost.URL
    using number_type = boost::decimal::decimal64_t;
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    using regex_policy = webcpp_regex;
};
template <class T> concept traits = number<typename T::number_type>
    && regex_policy<typename T::regex_policy, typename T::allocator_type, typename T::allocator_type>
    && std::same_as<typename std::allocator_traits<typename T::allocator_type>::value_type, std::byte>;
template <class Traits> using basic_string = std::basic_string<char, std::char_traits<char>,
    typename std::allocator_traits<typename Traits::allocator_type>::template rebind_alloc<char>>;

enum class dialect : std::uint8_t { draft4 = 1, draft6 = 2, draft7 = 3, draft2019_09 = 4,
                                    draft2020_12 = 5, openapi3_0 = 6 };
struct draft4 { static constexpr dialect id = dialect::draft4; };   // and the five others
template <class D> concept dialect_policy =
    requires { { D::id } -> std::convertible_to<dialect>; };        // the machinery joins with the loader
constexpr std::string_view uri_of(dialect d) noexcept;
constexpr std::optional<dialect> dialect_of_uri(std::string_view uri) noexcept;

template <class Traits = default_traits> struct basic_parsed_json {
    using allocator_type = typename Traits::allocator_type;
    explicit basic_parsed_json(allocator_type allocator = {});
    std::shared_ptr<detail::value_arena<allocator_type>> storage;   // declared first: freed last
    boost::json::value value;
    detail::exact_table<Traits> exact_numbers;
};
using parsed_json = basic_parsed_json<>;
template <class Traits = default_traits>
result<basic_parsed_json<Traits>> parse(std::string_view text,
    typename Traits::allocator_type allocator = {}, boost::json::parse_options options = {});
namespace detail {
inline constexpr std::size_t max_parse_depth = 256;   // measured at its definition
template <class Traits> class exact_table {   // JSON Pointer -> the exact Number of a lexeme
public:
    struct entry {
        typename Traits::number_type value;
        bool representable;
        double read_as;   // the double the value holds there: the entry counts only while it matches
    };
    const entry* find(std::string_view pointer) const noexcept;
    bool empty() const noexcept;
};
}
}
```

  - `parse` clamps `options.max_depth` to `detail::max_parse_depth`: a
    smaller value given is kept, and a larger one is lowered. Text nested
    deeper is `boost::json::error::too_deep`, a value, never a crash.
    Boost.JSON's parser recurses once per level.
  - The table is keyed by JSON Pointer from the document's root. An entry
    counts only while `read_as` equals the double the value holds at that
    pointer. A duplicate key keeps the last, in the value and, through that
    rule, in the table (spec 4.4).
  - `dialect_of_uri` accepts each official URI with and without its empty
    fragment, and nothing else. `uri_of` returns the spelling of spec 4.3,
    or `""` for OpenAPI 3.0.

- [ ] **Step 1: Write the failing tests.**

`test/dialect_test.cpp`:

```cpp
#include <webcpp/json_schema/dialects/dialect.hpp>
#include <webcpp/json_schema/traits.hpp>

#include <boost/core/lightweight_test.hpp>

namespace js = webcpp::json_schema;

static_assert(js::traits<js::default_traits>);
static_assert(js::dialect_policy<js::draft2020_12>);
static_assert(std::is_same_v<js::basic_string<js::default_traits>, std::pmr::string>);

int main() {
    BOOST_TEST_EQ(static_cast<int>(js::dialect::draft4), 1);
    BOOST_TEST_EQ(static_cast<int>(js::dialect::draft6), 2);
    BOOST_TEST_EQ(static_cast<int>(js::dialect::draft7), 3);
    BOOST_TEST_EQ(static_cast<int>(js::dialect::draft2019_09), 4);
    BOOST_TEST_EQ(static_cast<int>(js::dialect::draft2020_12), 5);
    BOOST_TEST_EQ(static_cast<int>(js::dialect::openapi3_0), 6);
    BOOST_TEST_EQ(js::uri_of(js::dialect::draft2020_12), "https://json-schema.org/draft/2020-12/schema");
    BOOST_TEST_EQ(js::uri_of(js::dialect::draft7), "http://json-schema.org/draft-07/schema#");
    BOOST_TEST_EQ(js::uri_of(js::dialect::openapi3_0), "");
    BOOST_TEST(js::dialect_of_uri("https://json-schema.org/draft/2020-12/schema#") == js::dialect::draft2020_12);
    BOOST_TEST(js::dialect_of_uri("http://json-schema.org/draft-07/schema") == js::dialect::draft7);
    BOOST_TEST(!js::dialect_of_uri("https://json-schema.org/draft-07/schema#"));
    BOOST_TEST(!js::dialect_of_uri("https://json-schema.org/draft/2019-09/schema/"));
    BOOST_TEST(!js::dialect_of_uri("http://json-schema.org/draft-03/schema#"));
    BOOST_TEST(!js::dialect_of_uri(""));
    return boost::report_errors();
}
```

`test/parse_test.cpp`:

```cpp
#include <webcpp/json_schema/json/parse.hpp>

#include "counting_resource.hpp"

#include <boost/core/lightweight_test.hpp>
#include <boost/json/error.hpp>

#include <string>

namespace js = webcpp::json_schema;
using d64 = boost::decimal::decimal64_t;
using d128 = boost::decimal::decimal128_t;

namespace {

struct wide_traits : js::default_traits {
    using number_type = d128;
};

d64 dec(std::string_view text) { return *js::number_traits<d64>::from_chars(text); }

void parse_keeps_what_a_double_loses() {
    auto parsed = js::parse(R"({"a": [0.1, 9007199254740993.0], "b": 12, "c": 1e400})");
    if (!BOOST_TEST(parsed.has_value())) {
        return;
    }
    BOOST_TEST(parsed->exact_numbers.find("/a/0") == nullptr);   // the double's text says 0.1
    const auto* exact = parsed->exact_numbers.find("/a/1");      // the double holds ...992
    if (BOOST_TEST(exact != nullptr)) {
        BOOST_TEST(exact->representable);
        BOOST_TEST(exact->value == dec("9007199254740993"));
    }
    BOOST_TEST(parsed->exact_numbers.find("/b") == nullptr);
    const auto* huge = parsed->exact_numbers.find("/c");         // the double is infinite
    BOOST_TEST(huge != nullptr && !huge->representable);
    auto wide = js::parse<wide_traits>("[12345678901234567890123.5]");
    const auto* big = wide->exact_numbers.find("/0");
    BOOST_TEST(big != nullptr && big->value == *js::number_traits<d128>::from_chars("12345678901234567890123.5"));
}

void the_last_duplicate_wins_in_the_table_too() {
    auto parsed = js::parse(R"({"maximum": 9007199254740993.0, "maximum": 5.5})");
    BOOST_TEST_EQ(parsed->value.at("maximum"), 5.5);
    const auto* stale = parsed->exact_numbers.find("/maximum");
    BOOST_TEST(stale == nullptr || stale->read_as != parsed->value.at("maximum").as_double());
}

void refusals_are_values() {
    BOOST_TEST(js::parse("{} x").has_error());
    BOOST_TEST(js::parse("{\"a\": ").has_error());
    const std::string deep(100000, '[');
    const auto too_deep = js::parse(deep);
    BOOST_TEST(too_deep.has_error() && too_deep.error().code == boost::json::error::too_deep);
    boost::json::parse_options asked;
    asked.max_depth = 1000000;
    BOOST_TEST(js::parse(deep, {}, asked).error().code == boost::json::error::too_deep);
}

void parse_allocates_through_the_given_allocator() {
    webcpp::test::counting_resource counted;
    {
        auto parsed = js::parse(R"({"a": [1, 2, 3], "b": "a string long enough to leave SSO"})",
                                std::pmr::polymorphic_allocator<std::byte>(&counted));
        BOOST_TEST(parsed.has_value());
        BOOST_TEST(counted.allocations > 0);
    }
    BOOST_TEST_EQ(counted.live, 0U);
}

}  // namespace

int main() {
    parse_keeps_what_a_double_loses();
    the_last_duplicate_wins_in_the_table_too();
    refusals_are_values();
    parse_allocates_through_the_given_allocator();
    return boost::report_errors();
}
```

  Append to `test/Jamfile`:

```
webcpp.headers-alone json_schema : ../include : json_schema/dialects/*.hpp json_schema/json/*.hpp ;
webcpp.run dialect : dialect_test.cpp ;
webcpp.run parse : parse_test.cpp ;
```

  E1.1's first `headers-alone` call already takes `traits.hpp`, through
  `json_schema/*.hpp`. Append one boundary object for `dialects/*.hpp` and
  `json/*.hpp`.
- [ ] **Step 2: Run them and watch them fail.**
  `$B2 libs/json_schema/test//dialect libs/json_schema/test//parse`.
  Expected: FAIL, `'webcpp/json_schema/dialects/dialect.hpp' file not
  found` and `'webcpp/json_schema/json/parse.hpp' file not found`.
- [ ] **Step 3: Implement.**

`dialects/dialect.hpp`:

```cpp
constexpr std::string_view uri_of(dialect d) noexcept {
    switch (d) {
        case dialect::draft4: return "http://json-schema.org/draft-04/schema#";
        case dialect::draft6: return "http://json-schema.org/draft-06/schema#";
        case dialect::draft7: return "http://json-schema.org/draft-07/schema#";
        case dialect::draft2019_09: return "https://json-schema.org/draft/2019-09/schema";
        case dialect::draft2020_12: return "https://json-schema.org/draft/2020-12/schema";
        case dialect::openapi3_0: return "";
    }
    return "";
}

constexpr std::optional<dialect> dialect_of_uri(std::string_view uri) noexcept {
    if (uri.ends_with('#')) {
        uri.remove_suffix(1);
    }
    for (const dialect d : {dialect::draft4, dialect::draft6, dialect::draft7,
                            dialect::draft2019_09, dialect::draft2020_12}) {
        std::string_view official = uri_of(d);
        if (official.ends_with('#')) {
            official.remove_suffix(1);
        }
        if (uri == official) {
            return d;
        }
    }
    return std::nullopt;
}
```

`detail/json/exact_handler.hpp`: a `boost::json::basic_parser` handler. It
builds the value through Boost.JSON's `value_stack`, which is iterative, and
records each lexeme whose double does not denote the same `Number`:

```cpp
template <class Traits>
class exact_handler {
public:
    static constexpr std::size_t max_object_size = boost::json::object::max_size();
    static constexpr std::size_t max_array_size = boost::json::array::max_size();
    static constexpr std::size_t max_key_size = boost::json::string::max_size();
    static constexpr std::size_t max_string_size = boost::json::string::max_size();

    exact_handler(boost::json::storage_ptr storage, std::pmr::memory_resource& scratch,
                  exact_table<Traits>& table)
        : stack_(storage), path_(&scratch), key_(&scratch), lexeme_(&scratch), table_(&table) {
        stack_.reset(std::move(storage));
    }

    boost::json::value release() { return stack_.release(); }

    bool on_document_begin(boost::system::error_code&) { return true; }
    bool on_document_end(boost::system::error_code&) { return true; }
    bool on_object_begin(boost::system::error_code&) { return open(false); }
    bool on_object_end(std::size_t n, boost::system::error_code&) {
        path_.pop_back();
        stack_.push_object(n);
        return next();
    }
    bool on_array_begin(boost::system::error_code&) { return open(true); }
    bool on_array_end(std::size_t n, boost::system::error_code&) {
        path_.pop_back();
        stack_.push_array(n);
        return next();
    }
    bool on_key_part(boost::json::string_view s, std::size_t, boost::system::error_code&) {
        stack_.push_chars(s);
        key_.append(s.data(), s.size());
        return true;
    }
    bool on_key(boost::json::string_view s, std::size_t, boost::system::error_code&) {
        stack_.push_key(s);
        key_.append(s.data(), s.size());
        path_.back().key.assign(key_);
        key_.clear();
        return true;
    }
    bool on_string_part(boost::json::string_view s, std::size_t, boost::system::error_code&) {
        stack_.push_chars(s);
        return true;
    }
    bool on_string(boost::json::string_view s, std::size_t, boost::system::error_code&) {
        stack_.push_string(s);
        return next();
    }
    bool on_number_part(boost::json::string_view s, boost::system::error_code&) {
        lexeme_.append(s.data(), s.size());
        return true;
    }
    bool on_int64(std::int64_t i, boost::json::string_view, boost::system::error_code&) {
        stack_.push_int64(i);
        lexeme_.clear();
        return next();
    }
    bool on_uint64(std::uint64_t u, boost::json::string_view, boost::system::error_code&) {
        stack_.push_uint64(u);
        lexeme_.clear();
        return next();
    }
    bool on_double(double d, boost::json::string_view s, boost::system::error_code&) {
        lexeme_.append(s.data(), s.size());
        record(d);
        stack_.push_double(d);
        lexeme_.clear();
        return next();
    }
    bool on_bool(bool b, boost::system::error_code&) {
        stack_.push_bool(b);
        return next();
    }
    bool on_null(boost::system::error_code&) {
        stack_.push_null();
        return next();
    }
    bool on_comment_part(boost::json::string_view, boost::system::error_code&) { return true; }
    bool on_comment(boost::json::string_view, boost::system::error_code&) { return true; }

private:
    struct level {
        bool array;
        std::uint64_t index;
        std::pmr::string key;
    };

    bool open(bool array) {
        path_.push_back({.array = array, .index = 0, .key = std::pmr::string(path_.get_allocator())});
        return true;
    }
    bool next() {
        if (!path_.empty() && path_.back().array) {
            ++path_.back().index;
        }
        return true;
    }
    std::pmr::string pointer() const {
        std::pmr::string out(path_.get_allocator());
        for (const level& l : path_) {
            if (l.array) {
                append_index(out, l.index);
            } else {
                append_token(out, l.key);
            }
        }
        return out;
    }
    void record(double d) {
        using number_type = typename Traits::number_type;
        const auto from_lexeme = number_traits<number_type>::from_chars(lexeme_);
        std::array<char, 32> text{};
        const auto written = std::to_chars(text.data(), text.data() + text.size(), d);
        const auto from_double = number_traits<number_type>::from_chars(
            std::string_view(text.data(), static_cast<std::size_t>(written.ptr - text.data())));
        if (from_lexeme && from_double && *from_lexeme == *from_double) {
            return;
        }
        table_->insert(pointer(), {.value = from_lexeme ? *from_lexeme : number_type{},
                                   .representable = from_lexeme.has_value(), .read_as = d});
    }

    boost::json::value_stack stack_;
    std::pmr::vector<level> path_;
    std::pmr::string key_;
    std::pmr::string lexeme_;
    exact_table<Traits>* table_;
};
```

  - `exact_table` is a `boost::unordered_flat_map<std::pmr::string, entry>`
    with transparent hashing, in the parsed document's allocator. Its
    `insert` replaces an existing key.
  - `parse`:
    - forces `options.numbers = boost::json::number_precision::precise`;
    - clamps `options.max_depth`;
    - runs `basic_parser<exact_handler<Traits>>::write_some(false, ...)`;
    - returns `detail::make_diagnostic(ec, "offset <n>", "")` on
      Boost.JSON's error, and `boost::json::error::extra_data` when fewer
      bytes than the text holds were consumed;
    - otherwise moves `release()` into `value`.
  - `basic_parsed_json`'s storage is a `value_arena` made with
    `std::allocate_shared`, so a parsed value dies without recursion.
  - `traits.hpp` holds `default_traits`, the concept and `basic_string`.
- [ ] **Step 4: Run them and watch them pass** on the five lanes:

```bash
lanes libs/json_schema/test
lint_clean
```

  Expected: `lanes: green`, `lint: clean`.
- [ ] **Step 5: Commit.**

```bash
cd libs/json_schema
F="include/webcpp/json_schema.hpp include/webcpp/json_schema/traits.hpp \
include/webcpp/json_schema/dialects/dialect.hpp include/webcpp/json_schema/json/parse.hpp \
include/webcpp/json_schema/detail/json/exact_handler.hpp meta/include-boundaries.json test/Jamfile \
test/dialect_test.cpp test/parse_test.cpp"
git add $F
$G commit -m "json_schema: traits, the dialect tags, and JSON text read exactly, its depth bounded" -- $F
```

### Task E1.7: The IR

After E1.6. Its five headers freeze at Task E1.17.

**Files:**
- Create: `include/webcpp/json_schema/ir/ids.hpp`, `ir/keyword.hpp`,
  `ir/node.hpp`, `ir/document.hpp`, `test/ir_test.cpp`.
- Modify: `include/webcpp/json_schema.hpp`, `test/Jamfile` (its own
  `headers-alone` call `json_schema/ir/*.hpp`; E1.2's call names
  `ir/number.hpp`, so this call names the four new files),
  `meta/include-boundaries.json` (the IR object's `headers` becomes
  `include/webcpp/json_schema/ir/*.hpp`).

**Interfaces:**
- Consumes: E1.2's `ir::number` and `detail::compare`; E1.3's `deep_copy`,
  `value_arena`, `resolve_pointer`, `is_valid_utf8`; E1.6's `dialect` and
  `traits`.
- Produces spec 5.1 to 5.4 with the spec's names. Each departure below is
  part of the contract the freeze reviews:
  - **Qualified number type.** `ir::number<...>` is written qualified inside
    `basic_document`. Spec 5.4's unqualified `number<number_type>` beside a
    member function named `number` does not compile: g++-15 says "changes
    meaning of 'number'".
  - **Keyword sources refer into the document's one copy** (the deep-input
    ruling). `add_value` deep-copies a value into the document's arena, once.
    `add_value_at(base, pointer)` gives a `value_id` that refers into an owned
    value at a JSON Pointer, copying nothing. `value(id)` reads a table of
    resolved addresses that the document rebuilds when it is copied with
    another allocator.
  - **`define` called twice** on one node is recorded and refused by `check`
    as `[ir-1]`.

```cpp
namespace webcpp::json_schema::ir {
enum class keyword_kind : std::uint8_t {
    ref = 1, dynamic_ref = 2, defs = 3, comment = 4,
    all_of = 10, any_of = 11, one_of = 12, not_ = 13, if_ = 14, then_ = 15, else_ = 16,
    dependent_schemas = 17, prefix_items = 18, items = 19, contains = 20, properties = 21,
    pattern_properties = 22, additional_properties = 23, property_names = 24,
    unevaluated_items = 30, unevaluated_properties = 31,
    type = 40, enum_ = 41, const_ = 42, multiple_of = 43, maximum = 44, exclusive_maximum = 45,
    minimum = 46, exclusive_minimum = 47, max_length = 48, min_length = 49, pattern = 50,
    max_items = 51, min_items = 52, unique_items = 53, max_contains = 54, min_contains = 55,
    max_properties = 56, min_properties = 57, required = 58, dependent_required = 59,
    format = 60, content_encoding = 61, content_media_type = 62, content_schema = 63,
    title = 70, description = 71, default_ = 72, deprecated = 73, read_only = 74,
    write_only = 75, examples = 76,
    discriminator = 80, example = 81, xml = 82, external_docs = 83,
    unknown = 90, ignored = 91, user = 92,
};
/** The index, in `payload`, of the alternative a keyword of `kind` holds (spec 5.2's table). */
constexpr std::size_t payload_index(keyword_kind kind) noexcept;
namespace vocabulary_bit {
inline constexpr std::uint16_t core = 1, applicator = 2, unevaluated = 4, validation = 8,
    meta_data = 16, format_annotation = 32, format_assertion = 64, content = 128;
}
template <class Traits> class basic_document {
public:
    // spec 5.4's reading members, with `const ir::number<number_type>& number(number_id) const noexcept`
    // spec 5.4's building members, with `number_id add_number(const ir::number<number_type>& n)`
    value_id add_value(const boost::json::value& v);                    // one deep copy
    value_id add_value_at(value_id base, std::string_view pointer);     // refers into base's copy
    basic_document(const basic_document& other, allocator_type allocator);   // [ir-13]
    basic_document(basic_document&&) noexcept;
    basic_document& operator=(basic_document&&) noexcept;
};
using document = basic_document<default_traits>;
}
```

  `payload_index`:
  - `ref` 4, `dynamic_ref` 5, `defs` 3, `comment` 7;
  - the three lists 2, `not_` to `else_` 1, `dependent_schemas` 3,
    `prefix_items` 2, `items` 1, `contains` 15;
  - `properties` and `pattern_properties` 3, `additional_properties` and
    `property_names` 1, both `unevaluated_*` 1;
  - `type` 6, `enum_` 8, `const_` 7, the five bounds 9;
  - the eight counts 10: `max_length`, `min_length`, `max_items`,
    `min_items`, `max_contains`, `min_contains`, `max_properties`,
    `min_properties`;
  - `pattern` 11, `unique_items` 7, `required` 13, `dependent_required` 14,
    `format` 12, the two content texts 11, `content_schema` 1;
  - the meta-data and OpenAPI annotations 7, `unknown` and `ignored` 7,
    `user` 16.

- [ ] **Step 1: Write the failing test** `test/ir_test.cpp`:

```cpp
#include <webcpp/json_schema/ir/document.hpp>

#include "counting_resource.hpp"

#include <boost/core/lightweight_test.hpp>
#include <boost/json/monotonic_resource.hpp>
#include <boost/json/parse.hpp>

#include <array>
#include <functional>
#include <string>

namespace js = webcpp::json_schema;
namespace ir = js::ir;
using k = ir::keyword_kind;
using n64 = ir::number<boost::decimal::decimal64_t>;

namespace {

ir::keyword make(ir::document& d, k kind, std::string_view name, ir::payload data) {
    return {.kind = kind, .name = d.intern(name), .source = ir::no_value, .data = data};
}

// {"type": "object", "properties": {"a": {"minLength": 2}}, "required": ["a"]}, built in code
ir::document example(std::pmr::memory_resource* resource = std::pmr::get_default_resource()) {
    ir::document doc{std::pmr::polymorphic_allocator<std::byte>(resource)};
    const auto root_resource = doc.add_resource({.uri = doc.intern("urn:webcpp:root"),
                                                 .source_dialect = js::dialect::draft2020_12,
                                                 .root = ir::node_id{0},
                                                 .parent = ir::resource_id{0},
                                                 .vocabularies = 0x00FF,
                                                 .recursive_anchor = false});
    const ir::node_id root = doc.reserve(root_resource, "");
    const ir::node_id a = doc.reserve(root_resource, "/properties/a");
    const std::array<ir::named_subschema, 1> entry{ir::named_subschema{.name = doc.intern("a"), .schema = a}};
    const std::array<ir::string_id, 1> required{doc.intern("a")};
    const std::array<ir::keyword, 3> keywords{
        make(doc, k::type, "type", ir::types{.set = {.bits = ir::type_set::object}, .integer_by_value = true}),
        make(doc, k::properties, "properties", ir::named_subschemas{.entries = doc.add_entries(entry)}),
        make(doc, k::required, "required", ir::names{.strings = doc.add_strings(required)}),
    };
    doc.define(root, ir::node_kind::object, keywords);
    const std::array<ir::keyword, 1> child{make(doc, k::min_length, "minLength", ir::count{.value = 2})};
    doc.define(a, ir::node_kind::object, child);
    doc.set_root(root);
    return doc;
}

void kinds_are_fixed() {
    BOOST_TEST_EQ(static_cast<int>(k::ref), 1);
    BOOST_TEST_EQ(static_cast<int>(k::all_of), 10);
    BOOST_TEST_EQ(static_cast<int>(k::unevaluated_items), 30);
    BOOST_TEST_EQ(static_cast<int>(k::type), 40);
    BOOST_TEST_EQ(static_cast<int>(k::min_properties), 57);
    BOOST_TEST_EQ(static_cast<int>(k::content_schema), 63);
    BOOST_TEST_EQ(static_cast<int>(k::examples), 76);
    BOOST_TEST_EQ(static_cast<int>(k::external_docs), 83);
    BOOST_TEST_EQ(static_cast<int>(k::user), 92);
    BOOST_TEST_EQ(ir::payload_index(k::properties), 3U);
    BOOST_TEST_EQ(ir::payload_index(k::max_contains), 10U);
    BOOST_TEST_EQ(ir::payload_index(k::user), 16U);
}

void a_document_reads_back_what_was_built() {
    const ir::document doc = example();
    BOOST_TEST(doc.check().has_value());
    BOOST_TEST_EQ(doc.size(), 2U);
    const auto* properties = doc.find(doc.root(), k::properties);
    if (!BOOST_TEST(properties != nullptr)) {
        return;
    }
    const auto entries = doc.entries(std::get<ir::named_subschemas>(properties->data).entries);
    BOOST_TEST_EQ(doc.string(entries[0].name), "a");
    BOOST_TEST_EQ(doc.string(doc.at(entries[0].schema).pointer), "/properties/a");
    BOOST_TEST(doc.find(doc.root(), k::pattern) == nullptr);
}

void sources_refer_into_one_copy() {
    webcpp::test::counting_resource counted;
    ir::document doc{std::pmr::polymorphic_allocator<std::byte>(&counted)};
    const auto source = boost::json::parse(R"({"properties": {"a": {"enum": [1, 2, 3]}}})");
    const ir::value_id whole = doc.add_value(source);
    const std::size_t after_copy = counted.peak;
    std::array<ir::value_id, 1000> views{};
    for (auto& v : views) {
        v = doc.add_value_at(whole, "/properties/a/enum");
    }
    BOOST_TEST_EQ(doc.value(views[999]).as_array().size(), 3U);
    BOOST_TEST(counted.peak < after_copy + 64 * 1024);   // indices only, never another copy
}

void a_deep_value_is_owned_without_recursion() {
    boost::json::monotonic_resource storage;
    boost::json::value deep = boost::json::array(&storage);
    for (int i = 0; i < 100000; ++i) {
        boost::json::array outer(&storage);
        outer.push_back(std::move(deep));
        deep = std::move(outer);
    }
    ir::document doc;
    const ir::value_id v = doc.add_value(deep);
    BOOST_TEST(doc.value(v).is_array());
}   // doc dies here: its values are in a trivially freed arena

void refused(int invariant, const std::function<void(ir::document&)>& broken) {
    ir::document doc = example();
    broken(doc);
    const auto checked = doc.check();
    if (!BOOST_TEST(checked.has_error())) {
        return;
    }
    BOOST_TEST(checked.error().code == js::errc::invalid_ir);
    const std::string expected = "[ir-" + std::to_string(invariant) + "]";
    BOOST_TEST_EQ(std::string(checked.error().what.substr(0, expected.size())), expected);
}

void define_one(ir::document& d, std::string_view pointer, const ir::keyword& keyword) {
    const auto n = d.reserve(ir::resource_id{0}, pointer);
    const std::array<ir::keyword, 1> keywords{keyword};
    d.define(n, ir::node_kind::object, keywords);
}

void check_refuses_each_broken_invariant() {
    refused(1, [](ir::document& d) { (void)d.reserve(ir::resource_id{0}, "/never/defined"); });
    refused(1, [](ir::document& d) {
        define_one(d, "/x", make(d, k::not_, "not", ir::subschema{.schema = ir::node_id{99}}));
    });
    refused(1, [](ir::document& d) { define_one(d, "/y", make(d, k::max_length, "maxLength", ir::text{.value = d.intern("2")})); });
    refused(1, [](ir::document& d) {   // defined twice
        const std::array<ir::keyword, 0> none{};
        d.define(d.root(), ir::node_kind::object, none);
    });
    refused(2, [](ir::document& d) {
        define_one(d, "/r", make(d, k::ref, "$ref", ir::reference{.target = ir::no_node, .uri = d.intern("#/x")}));
    });
    refused(4, [](ir::document& d) {
        const auto n = d.reserve(ir::resource_id{0}, "/z");
        const std::array<ir::keyword, 2> twice{make(d, k::min_length, "minLength", ir::count{.value = 1}),
                                               make(d, k::min_length, "minLength", ir::count{.value = 2})};
        d.define(n, ir::node_kind::object, twice);
    });
    refused(6, [](ir::document& d) {
        const auto n = d.reserve(ir::resource_id{0}, "/w");
        const std::array<ir::named_subschema, 2> same{ir::named_subschema{.name = d.intern("a"), .schema = n},
                                                      ir::named_subschema{.name = d.intern("a"), .schema = n}};
        const std::array<ir::keyword, 1> keywords{
            make(d, k::properties, "properties", ir::named_subschemas{.entries = d.add_entries(same)})};
        d.define(n, ir::node_kind::object, keywords);
    });
    refused(7, [](ir::document& d) {
        const auto zero = d.add_number(n64{std::in_place_index<0>, 0});
        define_one(d, "/m", make(d, k::multiple_of, "multipleOf", ir::bound{.value = zero}));
    });
    refused(7, [](ir::document& d) {   // an integer that fits int64_t held as a Number
        const auto two = d.add_number(n64{std::in_place_index<2>, boost::decimal::decimal64_t{2}});
        define_one(d, "/n", make(d, k::maximum, "maximum", ir::bound{.value = two}));
    });
    refused(8, [](ir::document& d) { (void)d.intern("\xC0\x80"); });
    refused(9, [](ir::document& d) {
        (void)d.add_resource({.uri = d.intern("urn:webcpp:root"), .source_dialect = js::dialect::draft2020_12,
                              .root = d.root(), .parent = ir::resource_id{0}, .vocabularies = 0,
                              .recursive_anchor = false});
    });
    refused(10, [](ir::document& d) {
        d.add_anchor({.resource = ir::resource_id{0}, .name = d.intern("x"), .target = d.root(), .dynamic = false});
        d.add_anchor({.resource = ir::resource_id{0}, .name = d.intern("x"), .target = d.root(), .dynamic = false});
    });
}

void a_copy_with_another_allocator_copies_every_table() {
    const ir::document original = example();
    webcpp::test::counting_resource counted;
    const ir::document copy(original, std::pmr::polymorphic_allocator<std::byte>(&counted));
    BOOST_TEST(counted.allocations > 0);
    BOOST_TEST(copy.check().has_value());
    BOOST_TEST_EQ(copy.size(), original.size());
    BOOST_TEST_EQ(copy.string(copy.at(copy.root()).pointer), "");
}

}  // namespace

int main() {
    kinds_are_fixed();
    a_document_reads_back_what_was_built();
    sources_refer_into_one_copy();
    a_deep_value_is_owned_without_recursion();
    check_refuses_each_broken_invariant();
    a_copy_with_another_allocator_copies_every_table();
    return boost::report_errors();
}
```

  Append to `test/Jamfile`: `webcpp.run ir : ir_test.cpp ;` and the
  `headers-alone` call
  `json_schema/ir/ids.hpp json_schema/ir/keyword.hpp json_schema/ir/node.hpp json_schema/ir/document.hpp`.
- [ ] **Step 2: Run it and watch it fail.** `$B2 libs/json_schema/test//ir`.
  Expected: FAIL, `'webcpp/json_schema/ir/document.hpp' file not found`.
- [ ] **Step 3: Implement.**
  - `ir/ids.hpp`, `ir/keyword.hpp` and `ir/node.hpp` hold spec 5.1 and 5.2
    as written, plus the additions above.
  - `ir/document.hpp` stores each table in a vector of the traits'
    allocator, rebound:
    - nodes, a `defined` bit per node, a `redefined` flag;
    - keywords, `node_id`s, named entries, `string_id`s, dependencies,
      `value_id`s, numbers, resources, anchors;
    - interned strings in a `std::pmr::deque<std::pmr::string>` (stable
      views), deduplicated by a `boost::unordered_flat_map<std::string_view,
      string_id>`;
    - values: owned copies in a `value_arena`, and a table of `{value_id
      base; string_id pointer;}` with its resolved addresses beside it.
  - `check()` returns at the first fault
    `detail::make_diagnostic(errc::invalid_ir, <where>, "[ir-N] <what>")`,
    one helper per table (clang-tidy's complexity):

```cpp
template <class Traits>
result<void> basic_document<Traits>::check() const {
    if (root_ == no_node || static_cast<std::size_t>(root_) >= nodes_.size()) {
        return fault(1, "root", "the root is not set");
    }
    if (redefined_) {
        return fault(1, "nodes", "a node is defined twice");
    }
    if (resources_.empty() || resources_[0].root != root_) {
        return fault(9, "resources", "resource 0 is not the root's");
    }
    for (std::uint32_t i = 0; i < nodes_.size(); ++i) {
        if (auto r = check_node(node_id{i}); !r) {
            return r;
        }
    }
    if (auto r = check_strings(); !r) {
        return r;
    }
    if (auto r = check_values(); !r) {
        return r;
    }
    if (auto r = check_resources(); !r) {
        return r;
    }
    return check_anchors();
}

template <class Traits>
result<void> basic_document<Traits>::check_node(node_id id) const {
    const node& n = at(id);
    const std::string_view where = string(n.pointer);
    if (!defined_[static_cast<std::uint32_t>(id)]) {
        return fault(1, where, "a reserved node is never defined");
    }
    if (static_cast<std::size_t>(n.resource) >= resources_.size()) {
        return fault(1, where, "the node's resource does not exist");
    }
    std::array<bool, 256> seen{};
    for (const keyword& k : keywords(id)) {
        if (k.data.index() != payload_index(k.kind)) {
            return fault(1, where, string(k.name));
        }
        const bool repeats = k.kind == keyword_kind::unknown || k.kind == keyword_kind::ignored ||
                             k.kind == keyword_kind::user;
        const auto kind = static_cast<std::size_t>(k.kind);
        if (seen[kind] && !repeats) {
            return fault(4, where, string(k.name));
        }
        seen[kind] = true;
        if (auto r = check_payload(k, where); !r) {
            return r;
        }
    }
    return {};
}
```

  - `fault` formats `"[ir-N] <what>"` into a fixed buffer.
  - `check_payload` visits the payload:
    - every id and range must lie inside its table (1);
    - a `ref`'s target, and a `dynamic_ref`'s initial target, must not be
      `no_node` (2);
    - the entries of one `named_subschemas` have distinct names (6);
    - `multiple_of`'s number is above zero, by `detail::compare` with zero
      (7);
    - a `Number` alternative is never an integer that fits `int64_t` or
      `uint64_t` (7).
  - `check_strings`: every string is UTF-8 (8).
  - `check_values`: each base is owned, and each pointer resolves (1, 11).
  - `check_resources`: each URI is non-empty, holds no `#` and is unique;
    each root has pointer `""` and belongs to its resource (9).
  - `check_anchors`: each `(resource, name, dynamic)` triple is unique, and
    its target belongs to its resource (10).
  - Invariant 13 holds by construction: the tables hold indices only. The
    resolved addresses are a cache that the allocator-taking copy rebuilds.
- [ ] **Step 4: Run it and watch it pass** on the five lanes:

```bash
lanes libs/json_schema/test
lint_clean
```

  Expected: `lanes: green`, `lint: clean`.
- [ ] **Step 5: Commit.**

```bash
cd libs/json_schema
F="include/webcpp/json_schema.hpp include/webcpp/json_schema/ir/ids.hpp include/webcpp/json_schema/ir/keyword.hpp \
include/webcpp/json_schema/ir/node.hpp include/webcpp/json_schema/ir/document.hpp meta/include-boundaries.json \
test/Jamfile test/ir_test.cpp"
git add $F
$G commit -m "json_schema: the IR, one copy of each value, and the check of its invariants" -- $F
```

### Task E1.8: The options, the message hook, and the data types of later features

After E1.7.

**Files:**
- Create:
  - `include/webcpp/json_schema/validator/options.hpp`;
  - `include/webcpp/json_schema/output/message_hook.hpp`;
  - `include/webcpp/json_schema/registry/registry.hpp` (the class declared
    and defined empty of members but its constructor and allocator; E2.3
    adds the rest), `registry/resolver.hpp`;
  - `include/webcpp/json_schema/formats/format_set.hpp`;
  - `include/webcpp/json_schema/vocabulary/vocabulary.hpp`;
  - `test/options_test.cpp`.
- Modify: `include/webcpp/json_schema.hpp`, `test/Jamfile`,
  `meta/include-boundaries.json`.

**Interfaces:**
- Produces:
  - spec 4.5's `unknown_keywords`, `basic_compile_options` and
    `basic_validate_options`, whole and in the spec's member order;
  - the aliases `compile_options` and `validate_options` of
    `default_traits`;
  - `inline constexpr std::size_t default_max_schema_depth = 1024;` and
    `default_max_depth = 1024;`, starting values that Tasks E1.15 and E2.10
    measure;
  - spec 4.7's `resolver_ref`, spec 4.8's types, spec 4.9's
    `format_entry` and `format_set`;
  - `output/message_hook.hpp` (frozen):

```cpp
namespace webcpp::json_schema {
/** What a validation calls to write an error's message, in place of the English templates. */
template <class Traits>
using basic_message_hook = boost::compat::function_ref<void(const error& e, basic_string<Traits>& out)>;
}
```

  Spec 4.11 puts the alias in `output/messages.hpp`; this header holds it
  alone, so that it freezes without freezing the templates.
  `output/messages.hpp` includes it.

  Each option takes effect where its feature arrives; its Doc Comment
  describes the behaviour, not the plan:
  - `registry`, `resolver`, `base_uri`, `compatibility_keywords` and
    `validate_schema`: with references and meta-schemas;
  - `format_assertion`, `formats` and `content_assertion`: with formats and
    content;
  - `regex_step_budget`: with the regex policy (WEBCPP_REGEX_PLAN.md's Task
    E1.4);
  - `vocabularies`: with user vocabularies;
  - `messages`: with the English messages (E1.13).

  Until a feature exists, its option keeps its default, and the loader
  refuses a non-default one with `errc::invalid_keyword_value`, `where`
  `"options"` and `what` the option's name. A program can therefore never
  believe an option worked when it was ignored.

- [ ] **Step 1: Write the failing test** `test/options_test.cpp`:

```cpp
#include <webcpp/json_schema/output/message_hook.hpp>
#include <webcpp/json_schema/validator/options.hpp>

#include <boost/core/lightweight_test.hpp>

namespace js = webcpp::json_schema;

int main() {
    const js::compile_options compile{.default_dialect = js::dialect::draft2020_12, .validate_schema = false};
    BOOST_TEST(compile.registry == nullptr);
    BOOST_TEST(!compile.resolver);
    BOOST_TEST(compile.base_uri.empty());
    BOOST_TEST(!compile.format_assertion);
    BOOST_TEST(compile.formats.empty());
    BOOST_TEST_EQ(compile.regex_step_budget, 0U);
    BOOST_TEST(compile.unknown == js::unknown_keywords::dialect_default);
    BOOST_TEST(compile.compatibility_keywords);
    BOOST_TEST(!compile.validate_schema);
    BOOST_TEST_EQ(compile.max_schema_depth, js::default_max_schema_depth);
    const js::validate_options validate{.what = js::collect::everything};
    BOOST_TEST(!validate.messages);
    BOOST_TEST_EQ(validate.max_depth, js::default_max_depth);
    static_assert(std::is_same_v<js::basic_message_hook<js::default_traits>,
                                 boost::compat::function_ref<void(const js::error&, std::pmr::string&)>>);
    return boost::report_errors();
}
```

  Append to `test/Jamfile`:

```
webcpp.headers-alone json_schema : ../include : json_schema/output/message_hook.hpp ;
webcpp.headers-alone json_schema : ../include
  : json_schema/validator/options.hpp json_schema/registry/*.hpp json_schema/formats/format_set.hpp
    json_schema/vocabulary/*.hpp ;
webcpp.run options : options_test.cpp ;
```

  and one boundary object for those headers.
- [ ] **Step 2: Run it and watch it fail.** `$B2 libs/json_schema/test//options`.
  Expected: FAIL, `'webcpp/json_schema/output/message_hook.hpp' file not
  found`.
- [ ] **Step 3: Implement** spec 4.5, 4.7, 4.8 and 4.9 as written, with
  `basic_registry<Traits>` defined with its constructor and
  `get_allocator` only.
- [ ] **Step 4: Run it and watch it pass** on the five lanes:

```bash
lanes libs/json_schema/test
lint_clean
```

  Expected: `lanes: green`, `lint: clean`.
- [ ] **Step 5: Commit.**

```bash
cd libs/json_schema
F="include/webcpp/json_schema.hpp include/webcpp/json_schema/validator/options.hpp \
include/webcpp/json_schema/output/message_hook.hpp include/webcpp/json_schema/registry/registry.hpp \
include/webcpp/json_schema/registry/resolver.hpp include/webcpp/json_schema/formats/format_set.hpp \
include/webcpp/json_schema/vocabulary/vocabulary.hpp meta/include-boundaries.json test/Jamfile test/options_test.cpp"
git add $F
$G commit -m "json_schema: the compile and validate options, and the message hook's interface" -- $F
```

### Task E1.9: The loader for 2020-12

After E1.8.

**Files:**
- Create:
  - `include/webcpp/json_schema/detail/dialects/keyword_table.hpp`,
    `detail/dialects/draft2020_12.hpp`;
  - `include/webcpp/json_schema/detail/loader/walk.hpp`,
    `detail/loader/readers.hpp`;
  - `include/webcpp/json_schema/validator/validator.hpp`;
  - `test/loader_draft2020_12_test.cpp`, `test/compile_errors_test.cpp`.
- Modify:
  - `include/webcpp/json_schema/dialects/dialect.hpp` (the machinery
    requirement);
  - `include/webcpp/json_schema.hpp`, `test/Jamfile`,
    `meta/include-boundaries.json`.

**Interfaces:**
- Produces:

```cpp
namespace webcpp::json_schema {
template <class D> concept dialect_policy = requires { { D::id } -> std::convertible_to<dialect>; }
    && requires { detail::dialect_machinery<D>::keywords(); };
template <class Traits, class... Dialects>
    requires traits<Traits> && (sizeof...(Dialects) > 0) && (dialect_policy<Dialects> && ...)
class basic_validator {
public:
    using traits_type = Traits;
    using allocator_type = typename Traits::allocator_type;
    using document_type = ir::basic_document<Traits>;
    using compile_options = basic_compile_options<Traits>;
    static constexpr std::array<dialect, sizeof...(Dialects)> dialects{Dialects::id...};
    result<document_type> load(const boost::json::value& schema, const compile_options& options = {},
                               allocator_type allocator = {}) const;
    result<document_type> load(const basic_parsed_json<Traits>& schema, const compile_options& options = {},
                               allocator_type allocator = {}) const;
};
template <class... Dialects> using validator_for = basic_validator<default_traits, Dialects...>;
using validator = validator_for<draft2020_12>;   // grows to spec 4.5's six with every dialect's machinery
namespace detail {
template <class D> struct dialect_machinery;   // specialized in detail/dialects/<d>.hpp
enum class reader : std::uint8_t {
    schema, schema_list, nonempty_schema_list, schema_map, pattern_map, types, constant,
    constants, bound, positive_bound, count, text, pattern, names, dependent_required,
    format, contains, ref, dynamic_ref, annotation
};
struct keyword_rule { std::string_view name; ir::keyword_kind kind; reader read; std::uint16_t vocabulary; };
template <class Traits> class walker;   // spec 6.1, iterative
}
}
```

  **The name `validator` is well-formed at every point** (ruling,
  2026-10-09):
  - from this task on, it is `validator_for<draft2020_12>`, the one dialect
    with its machinery;
  - Task E5.0b, which gives the last dialect its machinery, widens it to
    spec 4.5's six;
  - a test that means one dialect names `validator_for<...>` explicitly, so
    that the widening changes none.

  The 2020-12 table, `detail/dialects/draft2020_12.hpp`:

```cpp
template <>
struct dialect_machinery<draft2020_12> {
    static std::span<const keyword_rule> keywords() noexcept {
        using k = ir::keyword_kind;
        using r = reader;
        namespace v = ir::vocabulary_bit;
        static constexpr std::array<keyword_rule, 52> table{{
            {.name = "$ref", .kind = k::ref, .read = r::ref, .vocabulary = v::core},
            {.name = "$dynamicRef", .kind = k::dynamic_ref, .read = r::dynamic_ref, .vocabulary = v::core},
            {.name = "$defs", .kind = k::defs, .read = r::schema_map, .vocabulary = v::core},
            {.name = "$comment", .kind = k::comment, .read = r::annotation, .vocabulary = v::core},
            {.name = "allOf", .kind = k::all_of, .read = r::nonempty_schema_list, .vocabulary = v::applicator},
            {.name = "anyOf", .kind = k::any_of, .read = r::nonempty_schema_list, .vocabulary = v::applicator},
            {.name = "oneOf", .kind = k::one_of, .read = r::nonempty_schema_list, .vocabulary = v::applicator},
            {.name = "not", .kind = k::not_, .read = r::schema, .vocabulary = v::applicator},
            {.name = "if", .kind = k::if_, .read = r::schema, .vocabulary = v::applicator},
            {.name = "then", .kind = k::then_, .read = r::schema, .vocabulary = v::applicator},
            {.name = "else", .kind = k::else_, .read = r::schema, .vocabulary = v::applicator},
            {.name = "dependentSchemas", .kind = k::dependent_schemas, .read = r::schema_map, .vocabulary = v::applicator},
            {.name = "prefixItems", .kind = k::prefix_items, .read = r::nonempty_schema_list, .vocabulary = v::applicator},
            {.name = "items", .kind = k::items, .read = r::schema, .vocabulary = v::applicator},
            {.name = "contains", .kind = k::contains, .read = r::contains, .vocabulary = v::applicator},
            {.name = "properties", .kind = k::properties, .read = r::schema_map, .vocabulary = v::applicator},
            {.name = "patternProperties", .kind = k::pattern_properties, .read = r::pattern_map, .vocabulary = v::applicator},
            {.name = "additionalProperties", .kind = k::additional_properties, .read = r::schema, .vocabulary = v::applicator},
            {.name = "propertyNames", .kind = k::property_names, .read = r::schema, .vocabulary = v::applicator},
            {.name = "unevaluatedItems", .kind = k::unevaluated_items, .read = r::schema, .vocabulary = v::unevaluated},
            {.name = "unevaluatedProperties", .kind = k::unevaluated_properties, .read = r::schema, .vocabulary = v::unevaluated},
            {.name = "type", .kind = k::type, .read = r::types, .vocabulary = v::validation},
            {.name = "enum", .kind = k::enum_, .read = r::constants, .vocabulary = v::validation},
            {.name = "const", .kind = k::const_, .read = r::constant, .vocabulary = v::validation},
            {.name = "multipleOf", .kind = k::multiple_of, .read = r::positive_bound, .vocabulary = v::validation},
            {.name = "maximum", .kind = k::maximum, .read = r::bound, .vocabulary = v::validation},
            {.name = "exclusiveMaximum", .kind = k::exclusive_maximum, .read = r::bound, .vocabulary = v::validation},
            {.name = "minimum", .kind = k::minimum, .read = r::bound, .vocabulary = v::validation},
            {.name = "exclusiveMinimum", .kind = k::exclusive_minimum, .read = r::bound, .vocabulary = v::validation},
            {.name = "maxLength", .kind = k::max_length, .read = r::count, .vocabulary = v::validation},
            {.name = "minLength", .kind = k::min_length, .read = r::count, .vocabulary = v::validation},
            {.name = "pattern", .kind = k::pattern, .read = r::pattern, .vocabulary = v::validation},
            {.name = "maxItems", .kind = k::max_items, .read = r::count, .vocabulary = v::validation},
            {.name = "minItems", .kind = k::min_items, .read = r::count, .vocabulary = v::validation},
            {.name = "uniqueItems", .kind = k::unique_items, .read = r::constant, .vocabulary = v::validation},
            {.name = "maxContains", .kind = k::max_contains, .read = r::count, .vocabulary = v::validation},
            {.name = "minContains", .kind = k::min_contains, .read = r::count, .vocabulary = v::validation},
            {.name = "maxProperties", .kind = k::max_properties, .read = r::count, .vocabulary = v::validation},
            {.name = "minProperties", .kind = k::min_properties, .read = r::count, .vocabulary = v::validation},
            {.name = "required", .kind = k::required, .read = r::names, .vocabulary = v::validation},
            {.name = "dependentRequired", .kind = k::dependent_required, .read = r::dependent_required, .vocabulary = v::validation},
            {.name = "title", .kind = k::title, .read = r::annotation, .vocabulary = v::meta_data},
            {.name = "description", .kind = k::description, .read = r::annotation, .vocabulary = v::meta_data},
            {.name = "default", .kind = k::default_, .read = r::annotation, .vocabulary = v::meta_data},
            {.name = "deprecated", .kind = k::deprecated, .read = r::annotation, .vocabulary = v::meta_data},
            {.name = "readOnly", .kind = k::read_only, .read = r::annotation, .vocabulary = v::meta_data},
            {.name = "writeOnly", .kind = k::write_only, .read = r::annotation, .vocabulary = v::meta_data},
            {.name = "examples", .kind = k::examples, .read = r::annotation, .vocabulary = v::meta_data},
            {.name = "format", .kind = k::format, .read = r::format, .vocabulary = v::format_annotation},
            {.name = "contentEncoding", .kind = k::content_encoding, .read = r::text, .vocabulary = v::content},
            {.name = "contentMediaType", .kind = k::content_media_type, .read = r::text, .vocabulary = v::content},
            {.name = "contentSchema", .kind = k::content_schema, .read = r::schema, .vocabulary = v::content},
        }};
        return table;
    }
};
```

  The resource keywords are read before the table, never as IR keywords:
  `$schema`, `$id`, `$anchor`, `$dynamicAnchor` and `$vocabulary`.

- [ ] **Step 1: Write the failing tests.**

`test/loader_draft2020_12_test.cpp`:

```cpp
#include <webcpp/json_schema/detail/numbers/compare.hpp>
#include <webcpp/json_schema/detail/numbers/decompose.hpp>
#include <webcpp/json_schema/validator/validator.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/json/parse.hpp>

#include <string>

namespace js = webcpp::json_schema;
namespace ir = js::ir;
using k = ir::keyword_kind;

namespace {

const js::validator_for<js::draft2020_12> validator;

js::result<js::ir::document> load(std::string_view text) {
    return validator.load(boost::json::parse(text));
}

const ir::keyword* find(const js::ir::document& d, k kind) { return d.find(d.root(), kind); }

void boolean_schemas() {
    const auto yes = load("true");
    BOOST_TEST(yes->at(yes->root()).kind == ir::node_kind::always_valid);
    const auto no = load("false");
    BOOST_TEST(no->at(no->root()).kind == ir::node_kind::never_valid);
    BOOST_TEST(load("3").error().code == js::errc::not_a_schema);
}

void applicators_keep_source_order() {
    const auto d = load(R"({"prefixItems": [true, false], "items": false, "properties": {"b": true, "a": {}}})");
    if (!BOOST_TEST(d.has_value())) {
        return;
    }
    const auto prefix = std::get<ir::subschema_list>(find(*d, k::prefix_items)->data);
    BOOST_TEST_EQ(d->nodes(prefix.nodes).size(), 2U);
    const auto entries = d->entries(std::get<ir::named_subschemas>(find(*d, k::properties)->data).entries);
    BOOST_TEST_EQ(d->string(entries[0].name), "b");
    BOOST_TEST_EQ(d->string(d->at(entries[1].schema).pointer), "/properties/a");
    BOOST_TEST(find(*d, k::items) != nullptr);
}

void dialect_fields_are_explicit() {
    const auto d = load(R"({"type": ["integer", "null"], "contains": true, "minContains": 2.0,
                            "format": "email", "exclusiveMaximum": 3, "dependentRequired": {"a": ["b"]}})");
    if (!BOOST_TEST(d.has_value())) {
        return;
    }
    BOOST_TEST(std::get<ir::types>(find(*d, k::type)->data).integer_by_value);
    BOOST_TEST(std::get<ir::contains_payload>(find(*d, k::contains)->data).annotates_items);
    BOOST_TEST_EQ(std::get<ir::count>(find(*d, k::min_contains)->data).value, 2U);
    BOOST_TEST(!std::get<ir::format_payload>(find(*d, k::format)->data).assertion);
    BOOST_TEST(find(*d, k::exclusive_maximum) != nullptr);
    BOOST_TEST(find(*d, k::dependent_required) != nullptr);
}

void sources_are_views_of_the_document() {
    const auto d = load(R"({"enum": [{"$id": "urn:data"}, 2], "title": "t"})");
    BOOST_TEST_EQ(d->resources().size(), 1U);   // an $id inside enum is data
    BOOST_TEST_EQ(d->value(find(*d, k::title)->source), "t");
}

void anchors_and_unknown_keywords() {
    const auto d = load(R"({"$anchor": "a", "$dynamicAnchor": "d", "x-note": {"$id": "urn:no"},
                            "$comment": "c", "definitions": {}})");
    if (!BOOST_TEST(d.has_value())) {
        return;
    }
    BOOST_TEST_EQ(d->anchors().size(), 3U);    // a; d dynamic; d plain
    BOOST_TEST_EQ(d->resources().size(), 1U);  // the $id inside x-note registers nothing
    BOOST_TEST(find(*d, k::unknown) != nullptr);
    BOOST_TEST(find(*d, k::comment) != nullptr);
}

void schema_uri_spellings() {
    BOOST_TEST(load(R"({"$schema": "https://json-schema.org/draft/2020-12/schema#"})").has_value());
    BOOST_TEST(load(R"({"$schema": "https://json-schema.org/draft/2020-12/schema"})").has_value());
    const auto draft7 = load(R"({"$schema": "http://json-schema.org/draft-07/schema#"})");
    BOOST_TEST(draft7.error().code == js::errc::unknown_dialect);
    BOOST_TEST_EQ(std::string(draft7.error().what), "http://json-schema.org/draft-07/schema#");
    const auto https7 = load(R"({"$schema": "https://json-schema.org/draft-07/schema#"})");
    BOOST_TEST(https7.error().code == js::errc::unknown_dialect);
    BOOST_TEST_EQ(std::string(https7.error().what), "https://json-schema.org/draft-07/schema#");
}

void shapes_are_refused_by_place() {
    const auto d = load(R"({"properties": {"a": {"minLength": -1}}})");
    BOOST_TEST(d.error().code == js::errc::invalid_keyword_value);
    BOOST_TEST_EQ(std::string(d.error().where), "/properties/a/minLength");
    BOOST_TEST_EQ(std::string(d.error().what), "minLength");
    BOOST_TEST(load(R"({"multipleOf": 0})").error().code == js::errc::invalid_keyword_value);
    BOOST_TEST(load(R"({"required": ["a", "a"]})").error().code == js::errc::invalid_keyword_value);
    BOOST_TEST(load(R"({"allOf": []})").error().code == js::errc::invalid_keyword_value);
}

void exact_numbers_reach_the_ir() {
    const auto parsed = js::parse(R"({"maximum": 9007199254740993.0})");
    const auto d = validator.load(*parsed);
    const auto& n = d->number(std::get<ir::bound>(find(*d, k::maximum)->data).value);
    BOOST_TEST(js::detail::compare(js::detail::decompose(n), js::detail::decompose_int64(9007199254740993)) == 0);
    BOOST_TEST(load(R"({"maximum": 1e400})").error().code == js::errc::invalid_number);
}

}  // namespace

int main() {
    boolean_schemas();
    applicators_keep_source_order();
    dialect_fields_are_explicit();
    sources_are_views_of_the_document();
    anchors_and_unknown_keywords();
    schema_uri_spellings();
    shapes_are_refused_by_place();
    exact_numbers_reach_the_ir();
    return boost::report_errors();
}
```

`test/compile_errors_test.cpp` (spec 16.3: every `errc` a load or compile
can return, with its `where`). It starts with what a load returns here;
later tasks append their codes:

```cpp
#include <webcpp/json_schema/validator/validator.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/json/parse.hpp>

#include <string>

namespace js = webcpp::json_schema;

namespace {

// load() never runs the meta-schema check, so every shape fault is the loader's own here.
const js::validator_for<js::draft2020_12> validator;

void refused(std::string_view schema, js::errc code, std::string_view where, std::string_view what = {},
             const js::compile_options& options = {}) {
    const auto loaded = validator.load(boost::json::parse(schema), options);
    if (!BOOST_TEST(loaded.has_error())) {
        return;
    }
    BOOST_TEST(loaded.error().code == code);
    BOOST_TEST_EQ(std::string(loaded.error().where), std::string(where));
    if (!what.empty()) {
        BOOST_TEST_EQ(std::string(loaded.error().what), std::string(what));
    }
}

void load_errors() {
    refused(R"({"items": 1})", js::errc::not_a_schema, "/items");   // the child, refused by the walk
    refused(R"({"type": "integr"})", js::errc::invalid_keyword_value, "/type", "type");
    refused(R"({"$schema": "urn:other"})", js::errc::unknown_dialect, "/$schema", "urn:other");
    refused(R"({"maximum": 1e400})", js::errc::invalid_number, "/maximum", "maximum");
    refused(R"({"$ref": "#/x"})", js::errc::unresolvable_reference, "/$ref", "#/x");
    refused("{}", js::errc::unknown_dialect, "options", "http://json-schema.org/draft-07/schema#",
            {.default_dialect = js::dialect::draft7});
    refused(R"({"not": {"not": {"not": {"not": {}}}}})", js::errc::schema_too_deep, "/not/not/not/not", {},
            {.max_schema_depth = 3});
}

}  // namespace

int main() {
    load_errors();
    return boost::report_errors();
}
```

  Append to `test/Jamfile`:

```
webcpp.headers-alone json_schema : ../include : json_schema/validator/validator.hpp ;
webcpp.headers-alone json_schema : ../include : json_schema/detail/dialects/*.hpp json_schema/detail/loader/*.hpp ;
webcpp.run loader_draft2020_12 : loader_draft2020_12_test.cpp ;
webcpp.run compile_errors : compile_errors_test.cpp ;
```

  and one boundary object for those headers.
- [ ] **Step 2: Run them and watch them fail.**
  `$B2 libs/json_schema/test//loader_draft2020_12`. Expected: FAIL,
  `'webcpp/json_schema/validator/validator.hpp' file not found`.
- [ ] **Step 3: Implement.**

The walk, `detail/loader/walk.hpp`. The document is copied once (spec 6.1,
the deep-input ruling) and walked over that copy, so every keyword's source
is a view of it:

```cpp
struct pending {
    const boost::json::value* value;   // inside the document's one copy
    ir::node_id node;
    ir::resource_id resource;
    std::uint32_t depth;
    std::uint32_t pointer;             // index into pointers_: the JSON Pointer from the document's root
};

template <class Traits>
result<ir::basic_document<Traits>> walker<Traits>::run(const boost::json::value& root,
                                                       const exact_table<Traits>* exact) {
    exact_ = exact;
    root_copy_ = doc_.add_value(root);
    auto opened = open_root_resource(doc_.value(root_copy_));   // dialect (D3), base URI, the root node
    if (!opened) {
        return opened.error();
    }
    work_.push_back(*opened);
    while (!work_.empty()) {
        const pending p = work_.back();
        work_.pop_back();
        if (auto r = walk_one(p); !r) {
            return r.error();
        }
    }
    doc_.set_root(root_node_);
    return std::move(doc_);
}

template <class Traits>
result<void> walker<Traits>::walk_one(const pending& p) {
    if (p.depth > options_.max_schema_depth) {
        return make_diagnostic(errc::schema_too_deep, pointer(p), "");
    }
    if (const bool* b = p.value->if_bool()) {
        doc_.define(p.node, *b ? ir::node_kind::always_valid : ir::node_kind::never_valid, {});
        return {};
    }
    const auto* object = p.value->if_object();
    if (object == nullptr) {
        return make_diagnostic(errc::not_a_schema, pointer(p), "");
    }
    if (auto r = read_resource_keywords(p, *object); !r) {   // $schema, $id, $anchor, $dynamicAnchor
        return r;
    }
    keywords_.clear();
    for (const auto& member : *object) {
        if (auto r = read_member(p, member.key(), member.value()); !r) {
            return r;
        }
    }
    doc_.define(p.node, ir::node_kind::object, keywords_);
    return {};
}

template <class Traits>
result<void> walker<Traits>::read_member(const pending& p, std::string_view name, const boost::json::value& v) {
    if (is_resource_keyword(name)) {
        return {};
    }
    const std::uint32_t at = child_pointer(p.pointer, name);   // "<p>/<name>", escaped
    const keyword_rule* rule = rule_of(p.resource, name);       // the resource's table and vocabularies
    if (rule == nullptr) {
        return add_unknown(name, at);   // its subschemas are not loaded (spec 6.2)
    }
    auto data = read_payload(*rule, p, v, at);   // readers.hpp; queues each child as a pending
    if (!data) {
        return data.error();
    }
    keywords_.push_back({.kind = rule->kind, .name = doc_.intern(name),
                         .source = doc_.add_value_at(root_copy_, pointers_[at]), .data = *data});
    return {};
}
```

  - `open_root_resource` picks the dialect, in this order: the root's
    `$schema` through `dialect_of_uri`, then `options.default_dialect`, then
    the validator's first dialect (D3).
    - a dialect outside the set is `unknown_dialect`. Its `where` is
      `/$schema` with `what` the URI as written, or `where` is `"options"`
      with `what` the dialect's `uri_of` (`"openapi3_0"` for OpenAPI 3.0,
      which has none) for a `default_dialect` (spec 4.5).
    - It reserves the root node in resource 0, whose URI is the root's
      absolute `$id`, with its empty fragment dropped, else
      `urn:webcpp:root`.
    - It returns the first `pending`.
  - `read_resource_keywords` records `$anchor` and `$dynamicAnchor` (a
    dynamic anchor and a plain anchor of the same name). A nested `$id` is
    `unresolvable_reference` until references exist.
  - `read_payload`, in `readers.hpp`, is a `switch` on `rule.read` that
    calls one function per reader:

| Reader | Accepts | Builds, else `invalid_keyword_value` (`where` the keyword's pointer, `what` its name) |
| --- | --- | --- |
| `schema` | any value | `subschema`: the child is reserved and queued; the walk refuses a non-schema there with `not_a_schema` at the child's pointer (spec 4.1's meaning of the code) |
| `schema_list`, `nonempty_schema_list` | an array (non-empty for the second) | `subschema_list` of queued children |
| `schema_map`, `pattern_map` | an object | `named_subschemas` in source order; patterns are compiled by the compiler |
| `types` | a type name, or a non-empty array of distinct ones | `types{.integer_by_value = true}` |
| `constant`, `annotation` | anything (`uniqueItems`: a boolean) | `constant`: a view at the keyword's pointer |
| `constants` | an array | `constants`: one view per element |
| `bound`, `positive_bound` | a number (above zero for the second) | `bound`: from `exact`'s entry at the pointer when it counts (`read_as` equal to the double there), else int64, uint64 or `schema_number_of_double`; unrepresentable is `invalid_number` |
| `count` | a non-negative integer, `2.0` included | `count` |
| `text`, `pattern` | a string | `text` |
| `names` | an array of distinct strings | `names` |
| `dependent_required` | an object of arrays of distinct strings | `dependencies` |
| `format` | a string | `format_payload{.assertion = false}` |
| `contains` | any value | `contains_payload{.annotates_items = true}` |
| `ref`, `dynamic_ref` | a string | refused with `unresolvable_reference`, `where` the keyword's pointer and `what` the URI as written, until references exist |

  `definitions` and `dependencies` load as `unknown` until
  `compatibility_keywords` is read.

  `basic_validator::load(parsed)` passes `&parsed.exact_numbers`, and
  `load(value)` passes `nullptr`. Both refuse a non-default option that is
  not read yet, as Task E1.8 says.
- [ ] **Step 4: Run them and watch them pass** on the five lanes:

```bash
lanes libs/json_schema/test
lint_clean
```

  Expected: `lanes: green`, `lint: clean`.
- [ ] **Step 5: Commit.**

```bash
cd libs/json_schema
F="include/webcpp/json_schema.hpp include/webcpp/json_schema/dialects/dialect.hpp \
include/webcpp/json_schema/detail/dialects/keyword_table.hpp include/webcpp/json_schema/detail/dialects/draft2020_12.hpp \
include/webcpp/json_schema/detail/loader/walk.hpp include/webcpp/json_schema/detail/loader/readers.hpp \
include/webcpp/json_schema/validator/validator.hpp meta/include-boundaries.json test/Jamfile \
test/loader_draft2020_12_test.cpp test/compile_errors_test.cpp"
git add $F
$G commit -m "json_schema: the 2020-12 loader, iterative over one copy of the schema" -- $F
```

### Task E1.10: The compiler: the plan, its lowering, and its text

After E1.9.

**Files:**
- Create:
  - `include/webcpp/json_schema/detail/compiler/opcode.hpp`,
    `detail/compiler/plan.hpp`, `detail/compiler/compile.hpp`,
    `detail/compiler/explain.hpp`;
  - `test/compiler_test.cpp`.
- Modify: `test/Jamfile`, `meta/include-boundaries.json`.

**Interfaces:**
- Produces (spec 7.1, 7.2's E1 subset):

```cpp
namespace webcpp::json_schema::detail {
enum class opcode : std::uint8_t {
    ret = 0, type_guard = 1, fail = 2,
    assert_type = 3, assert_const = 4, assert_enum = 5, assert_multiple_of = 6,
    assert_maximum = 7, assert_exclusive_maximum = 8, assert_minimum = 9,
    assert_exclusive_minimum = 10, assert_length = 11, assert_pattern = 12,
    assert_item_count = 13, assert_unique = 14, assert_property_count = 15,
    assert_required = 16, assert_dependent_required = 17,
    all_of = 18, any_of = 19, one_of = 20, not_ = 21, if_then_else = 22,
    loop_properties = 23, loop_property_names = 24, loop_dependent_schemas = 25,
    loop_items = 26, loop_contains = 27,
};
inline constexpr std::uint32_t no_block = 0xFFFF'FFFFu;
inline constexpr std::uint32_t no_location = 0xFFFF'FFFFu;
struct instruction {
    opcode op;
    std::uint8_t flags;
    std::uint16_t small;
    std::uint32_t a;
    std::uint32_t b;
    std::uint32_t location;   // index into plan::locations, or no_location
};
struct block { std::uint32_t first; std::uint32_t count; ir::node_id node; std::uint32_t flags; };
/** Where an instruction's keyword is, as indices only (spec 7.1): rendered when an error is built. */
struct keyword_location { ir::keyword_id keyword; ir::node_id node; };
template <class Traits> struct plan {
    using regex_type = typename Traits::regex_policy::template compiled<typename Traits::allocator_type>;
    std::pmr::vector<instruction> code;
    std::pmr::vector<block> blocks;
    std::pmr::vector<keyword_location> locations;
    std::pmr::vector<std::uint32_t> operands;      // block lists, string ids, (name, block) pairs
    std::pmr::vector<regex_type> regexes;
    std::uint32_t root_block;
};
template <class Traits>
result<plan<Traits>> compile_plan(const ir::basic_document<Traits>& document,
                                  typename Traits::allocator_type allocator);
template <class Traits>
std::pmr::string explain(const ir::basic_document<Traits>& document, const plan<Traits>& plan,
                         std::pmr::memory_resource& resource);
}
```

  **The lowering.** Blocks are numbered in the order a breadth-first walk
  from the root first reaches their nodes, keywords in IR order. Each
  keyword whose instructions apply to one type opens with `type_guard
  <type> <n>`, which skips its `n` instructions when the instance has
  another type:
  - arrays: `loop_items`, `assert_item_count`, `assert_unique`,
    `loop_contains`;
  - objects: the object keywords;
  - strings: `assert_length`, `assert_pattern`;
  - numbers: the numeric assertions.

| IR | Instruction | Operands, and their words in `explain` |
| --- | --- | --- |
| node `never_valid` | `fail` | none |
| node `always_valid` | none | the block is `ret` alone |
| `type` | `assert_type` | `small` the bits; `flags` bit 0 `integer_by_value`. Words: the type names joined by `\|`, in bit order |
| `const_`, `enum_` | `assert_const`, `assert_enum` | `a` the value id, or `a`, `b` first and count in `operands`. Words: the count |
| `multiple_of` and the four bounds | `assert_multiple_of`, `assert_maximum`, ... | `a` the number id. Words: the number |
| `max_length`, `min_length` | `assert_length` | `flags` 1 for max; `a`, `b` the count's low and high words. Words: `max <n>` or `min <n>` |
| `pattern` | `assert_pattern` | `a` the regex index; compiled here through `R::compile(text, allocator, options.regex_step_budget)`, a refusal is `invalid_regex` with `where` the keyword's location and `what` the pattern. Words: the pattern |
| `max_items`, `min_items` | `assert_item_count` | as `assert_length` |
| `unique_items` | `assert_unique`, only when `true` | none |
| `max_properties`, `min_properties` | `assert_property_count` | as `assert_length` |
| `required` | `assert_required` | `a`, `b` the string ids in `operands`. Words: the names joined by `,` |
| `dependent_required` | `assert_dependent_required` | `a`, `b` the dependencies' range. Words: the count |
| `all_of`, `any_of`, `one_of` | the same name | `a`, `b` the child blocks in `operands`. Words: the blocks |
| `not_` | `not_` | `a` the block |
| `if_` (with `then_`, `else_`) | `if_then_else` | `a` the if block, `b` the first of then and else in `operands` (`no_block` when absent). Words: the three, `none` for an absent one |
| `properties`, `pattern_properties`, `additional_properties` | one `loop_properties` | `a` the (name id, block) pairs, `b` the (regex, block) pairs, both in `operands`; `small` and `flags` locate the additional block. Words: `names <n> patterns <n> additional <block\|none>` |
| `property_names` | `loop_property_names` | `a` the block |
| `dependent_schemas` | `loop_dependent_schemas` | the (name id, block) pairs |
| `prefix_items`, `items` | one `loop_items` | `a` the prefix blocks, `b` the rest's block or `no_block`. Words: `prefix <n> rest <block\|none>` |
| `contains`, `min_contains`, `max_contains` | one `loop_contains` | `a` the block, its bounds in `operands`. Words: `<block> min <n> max <n\|none>` |
| annotations, `unknown`, `ignored`, `comment`, `defs`, `format`, `content_*` | nothing yet | |

  A block ends with `ret`.

- [ ] **Step 1: Write the failing test** `test/compiler_test.cpp`:

```cpp
#include <webcpp/json_schema/detail/compiler/compile.hpp>
#include <webcpp/json_schema/detail/compiler/explain.hpp>
#include <webcpp/json_schema/validator/validator.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/json/parse.hpp>

#include <string>

namespace js = webcpp::json_schema;

namespace {

const js::validator_for<js::draft2020_12> validator;

std::string plan(std::string_view schema) {
    const auto document = validator.load(boost::json::parse(schema));
    const auto compiled = js::detail::compile_plan(*document, {});
    if (!compiled) {
        return std::string(compiled.error().code.message()) + " " + std::string(compiled.error().where);
    }
    std::pmr::monotonic_buffer_resource scratch;
    return std::string(js::detail::explain(*document, *compiled, scratch));
}

void assertions_are_guarded_by_type() {
    BOOST_TEST_EQ(plan(R"({"type": "string", "maxLength": 3})"),
                  "block 0 urn:webcpp:root#\n"
                  "  0 assert_type string ; urn:webcpp:root#/type\n"
                  "  1 type_guard string 1 ; urn:webcpp:root#/maxLength\n"
                  "  2 assert_length max 3 ; urn:webcpp:root#/maxLength\n"
                  "  3 ret\n");
}

void blocks_follow_a_breadth_first_walk() {
    BOOST_TEST_EQ(plan(R"({"properties": {"a": {"minLength": 1}}, "required": ["a"], "allOf": [true, {"maximum": 2}]})"),
                  "block 0 urn:webcpp:root#\n"
                  "  0 type_guard object 1 ; urn:webcpp:root#/properties\n"
                  "  1 loop_properties names 1 patterns 0 additional none ; urn:webcpp:root#/properties\n"
                  "  2 type_guard object 1 ; urn:webcpp:root#/required\n"
                  "  3 assert_required a ; urn:webcpp:root#/required\n"
                  "  4 all_of 2 3 ; urn:webcpp:root#/allOf\n"
                  "  5 ret\n"
                  "block 1 urn:webcpp:root#/properties/a\n"
                  "  0 type_guard string 1 ; urn:webcpp:root#/properties/a/minLength\n"
                  "  1 assert_length min 1 ; urn:webcpp:root#/properties/a/minLength\n"
                  "  2 ret\n"
                  "block 2 urn:webcpp:root#/allOf/0\n"
                  "  0 ret\n"
                  "block 3 urn:webcpp:root#/allOf/1\n"
                  "  0 type_guard number 1 ; urn:webcpp:root#/allOf/1/maximum\n"
                  "  1 assert_maximum 2 ; urn:webcpp:root#/allOf/1/maximum\n"
                  "  2 ret\n");
}

void a_bad_pattern_is_refused_by_place() {
    BOOST_TEST_EQ(plan(R"({"properties": {"a": {"pattern": "("}}})"),
                  "the pattern is not a valid regular expression /properties/a/pattern");
    const auto document = validator.load(boost::json::parse(R"({"pattern": "("})"));
    BOOST_TEST_EQ(std::string(js::detail::compile_plan(*document, {}).error().what), "(");
}

}  // namespace

int main() {
    assertions_are_guarded_by_type();
    blocks_follow_a_breadth_first_walk();
    a_bad_pattern_is_refused_by_place();
    return boost::report_errors();
}
```

  Append to `test/Jamfile`:

```
webcpp.headers-alone json_schema : ../include : json_schema/detail/compiler/*.hpp ;
webcpp.run compiler : compiler_test.cpp ;
```

  and one boundary object for `detail/compiler/*.hpp`.
- [ ] **Step 2: Run it and watch it fail.** `$B2 libs/json_schema/test//compiler`.
  Expected: FAIL, `'webcpp/json_schema/detail/compiler/compile.hpp' file
  not found`.
- [ ] **Step 3: Implement.** The walk and the per-keyword lowering:

```cpp
template <class Traits>
result<plan<Traits>> compile_plan(const ir::basic_document<Traits>& document,
                                  typename Traits::allocator_type allocator) {
    lowering<Traits> lower(document, allocator);
    lower.block_of(document.root());   // reserves block 0 and queues the root
    while (auto node = lower.next_queued()) {   // breadth first: a deque in the plan's allocator
        if (auto r = lower.lower_node(*node); !r) {
            return r.error();
        }
    }
    return lower.release();
}

template <class Traits>
result<void> lowering<Traits>::lower_node(ir::node_id node) {
    open_block(node);
    const ir::node& n = document_.at(node);
    if (n.kind == ir::node_kind::never_valid) {
        emit({.op = opcode::fail, .flags = 0, .small = 0, .a = 0, .b = 0, .location = no_location});
    }
    for (std::uint32_t i = 0; i < n.keywords.count; ++i) {
        const ir::keyword_id id{n.keywords.first + i};
        if (auto r = lower_keyword(node, id, document_.keywords(node)[i]); !r) {
            return r;
        }
    }
    emit({.op = opcode::ret, .flags = 0, .small = 0, .a = 0, .b = 0, .location = no_location});
    return close_block();
}
```

  - `lower_keyword` is a `switch` on the kind that calls one helper per
    row of the table: `lower_type`, `lower_constants`, `lower_bound`,
    `lower_count`, `lower_pattern`, `lower_required`, `lower_list`,
    `lower_conditional`, `lower_properties`, `lower_items`,
    `lower_contains`.
  - Each helper:
    - adds a `keyword_location{.keyword = id, .node = node}`;
    - writes the guard with its skip count;
    - writes the instruction;
    - calls `block_of(child)`, which returns a node's block, reserving and
      queuing it on first reach.
  - `explain` renders each block's header from its node's resource URI and
    pointer, and each instruction's words by the table.
- [ ] **Step 4: Run it and watch it pass** on the five lanes:

```bash
lanes libs/json_schema/test
lint_clean
```

  Expected: `lanes: green`, `lint: clean`.
- [ ] **Step 5: Commit.**

```bash
cd libs/json_schema
F="include/webcpp/json_schema/detail/compiler/opcode.hpp include/webcpp/json_schema/detail/compiler/plan.hpp \
include/webcpp/json_schema/detail/compiler/compile.hpp include/webcpp/json_schema/detail/compiler/explain.hpp \
meta/include-boundaries.json test/Jamfile test/compiler_test.cpp"
git add $F
$G commit -m "json_schema: the compiler: a flat plan of indices, and its text" -- $F
```

### Task E1.11: The machine and `is_valid`: every E1 instruction's verdict

After E1.10.

**Files:**
- Create:
  - `include/webcpp/json_schema/detail/vm/machine.hpp`;
  - `include/webcpp/json_schema/validator/schema.hpp`;
  - `test/vm_test.cpp`, `test/compile_throw_test.cpp`.
- Modify:
  - `include/webcpp/json_schema/validator/validator.hpp` (the three
    `compile`s, `compile_or_throw`);
  - `include/webcpp/json_schema.hpp`, `test/Jamfile`,
    `test/compile_errors_test.cpp`.

**Interfaces:**
- Produces:

```cpp
namespace webcpp::json_schema::detail {
template <class Traits> struct compiled_schema {   // what basic_schema shares
    ir::basic_document<Traits> document;
    plan<Traits> code;
};
/** The alternatives of anyOf and oneOf whose failure counts, in evaluation order (sink.hpp, P4). */
using alternative_bits = inline_stack<std::uint64_t, 4>;
struct machine_options {
    std::size_t max_depth;
    alternative_bits* noted;            // first run: written; nullptr otherwise
    const alternative_bits* replayed;   // second run: read; nullptr otherwise
};
template <class Traits, class Sink> requires evaluation_sink<Sink>
class machine {
public:
    using allocator_type = typename Traits::allocator_type;
    machine(const compiled_schema<Traits>& schema, Sink& sink, allocator_type allocator,
            std::pmr::memory_resource& resource, const machine_options& options) noexcept;
    bool run(const boost::json::value& instance);
    std::uint64_t steps() const noexcept;   // instructions executed, for the hostile tests
};
}
namespace webcpp::json_schema {
template <class Traits> class basic_schema {   // spec 4.6; validate and explain join later
public:
    using allocator_type = typename Traits::allocator_type;
    using validate_options = basic_validate_options<Traits>;
    bool is_valid(const boost::json::value& instance, const validate_options& options = {},
                  allocator_type allocator = {}) const;
    const ir::basic_document<Traits>& ir() const noexcept;
    allocator_type get_allocator() const noexcept;
};
using schema = basic_schema<default_traits>;
}
```

  The five loops (spec 7.2):
  - **`loop_properties`** walks the instance's members in order. For each,
    it runs:
    - the named block, when the name is in the name table;
    - every pattern block whose regex `R::search` finds;
    - the additional block, when neither matched.

    It pushes `properties/<name>`, `patternProperties/<pattern>` or
    `additionalProperties` onto the evaluation path, and `<name>` onto the
    instance path.
  - **`loop_property_names`** runs its block on each name, as a string value
    built in the validation's arena, at the member's instance location.
  - **`loop_dependent_schemas`** runs each entry whose name the instance
    has, on the instance itself.
  - **`loop_items`** runs the prefix blocks on the first items, then the
    rest's block on the others.
  - **`loop_contains`** counts matching items and discards each
    non-matching item's records (P4). It then checks `minContains` (1 by
    default) and `maxContains`, with the kinds `contains`, `min_contains`
    and `max_contains`.

  **The recording paths**, behind `if constexpr (Sink::records)`, proven by
  Task E1.12. Every applicator records its own error after its subschemas':
  - `properties`, `pattern_properties`, `additional_properties`,
    `property_names` and `dependent_schemas` name the failing members in
    their details;
  - `prefix_items` and `items` count the failing items as `std::uint64_t`
    details.

  Every count assertion stores what it measured as `std::uint64_t` details:
  - `max_length` and `min_length`: the code points;
  - `max_items`, `min_items`, `max_properties`, `min_properties`: the
    count;
  - `min_contains` and `max_contains`: the matches.

  `required` and `dependent_required` name the missing properties, and
  `unique_items` gives the two equal indices.

  **The depth limit** (spec 7.6). A child that descends into an item, a
  member or a name past `max_depth` is not pushed:
  - the machine fails the keyword with kind `depth_limit` at that child's
    instance location, `expected` the keyword's value and the limit as
    details;
  - it delivers `false` as the child's result.

  The machine gets the validation's `allocator_type` as well as its
  resource, for `R::search(re, text, allocator)`. Its frames, paths and
  bits start in inline buffers (32 frames, 64 tokens, 256 applicator bits),
  so `is_valid` allocates nothing for an instance nesting at most 32
  levels. With `!Sink::records` it pushes no evaluation-path token, which
  only a recording sink renders.

- [ ] **Step 1: Write the failing tests.** `test/vm_test.cpp`, the
  assertions and logical applicators through `is_valid`, Review Focus 2's
  numbers among them:

```cpp
#include <webcpp/json_schema/validator/validator.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/json/parse.hpp>

namespace js = webcpp::json_schema;

namespace {

const js::validator_for<js::draft2020_12> validator;

bool valid(std::string_view schema, const boost::json::value& instance) {
    const auto compiled = validator.compile(boost::json::parse(schema));
    if (!BOOST_TEST(compiled.has_value())) {
        return false;
    }
    return compiled->is_valid(instance);
}

bool valid(std::string_view schema, std::string_view instance) {
    return valid(schema, boost::json::parse(instance));
}

void assertions() {
    BOOST_TEST(valid(R"({"type": "integer"})", "1.0"));
    BOOST_TEST(!valid(R"({"type": "integer"})", "1.5"));
    BOOST_TEST(valid(R"({"type": ["string", "null"]})", "null"));
    BOOST_TEST(valid(R"({"const": 1})", "1.0"));
    BOOST_TEST(!valid(R"({"const": 0.3})", boost::json::value(0.1 + 0.2)));
    BOOST_TEST(valid(R"({"enum": [{"a": [1]}]})", R"({"a": [1.0]})"));
    BOOST_TEST(!valid(R"({"enum": [false]})", "0"));
    BOOST_TEST(valid(R"({"minimum": 0})", "-0.0"));
    BOOST_TEST(!valid(R"({"exclusiveMinimum": 0})", "0"));
    BOOST_TEST(!valid(R"({"exclusiveMinimum": 1.1})", "1.1"));
    BOOST_TEST(valid(R"({"maximum": 18446744073709551615})", "18446744073709551615"));
    BOOST_TEST(!valid(R"({"maximum": 18446744073709551615})", "18446744073709551616"));   // the double 2^64
    BOOST_TEST(valid(R"({"multipleOf": 0.0001})", "1e308"));
    BOOST_TEST(valid(R"({"multipleOf": 0.0001})", "0.0075"));
    BOOST_TEST(valid(R"({"maxLength": 2})", "\"\xF0\x9F\x90\xB2\xF0\x9F\x90\xB2\""));
    BOOST_TEST(!valid(R"({"minLength": 3})", "\"ab\""));
    BOOST_TEST(valid(R"({"minLength": 3})", "1"));   // other types pass
    BOOST_TEST(valid(R"({"pattern": "^a"})", "\"abc\""));
    BOOST_TEST(!valid(R"({"pattern": "^a"})", "\"bc\""));
    BOOST_TEST(!valid(R"({"pattern": "^(\\w+\\s?)*$"})",
                      "\"A long sentence of words that ends with something refused!\""));
    BOOST_TEST(!valid(R"({"uniqueItems": true})", "[1, 1.0]"));
    BOOST_TEST(valid(R"({"uniqueItems": true})", boost::json::array{0.1 + 0.2, 0.3}));
    BOOST_TEST(valid(R"({"uniqueItems": false})", "[1, 1]"));
    BOOST_TEST(!valid(R"({"required": ["a", "b"]})", R"({"a": 1})"));
    BOOST_TEST(!valid(R"({"dependentRequired": {"a": ["b"]}})", R"({"a": 1})"));
    BOOST_TEST(valid(R"({"maxProperties": 1})", R"({"a": 1})"));
    BOOST_TEST(!valid("false", "null"));
}

void logical_applicators() {
    BOOST_TEST(!valid(R"({"allOf": [{"type": "string"}, {"maxLength": 1}]})", "\"ab\""));
    BOOST_TEST(valid(R"({"anyOf": [{"type": "string"}, {"type": "null"}]})", "null"));
    BOOST_TEST(!valid(R"({"anyOf": [{"type": "string"}, {"type": "null"}]})", "1"));
    BOOST_TEST(!valid(R"({"oneOf": [{"type": "integer"}, {"minimum": 0}]})", "1"));
    BOOST_TEST(valid(R"({"oneOf": [{"type": "integer"}, {"minimum": 0}]})", "-1"));
    BOOST_TEST(valid(R"({"not": {"type": "null"}})", "1"));
    BOOST_TEST(valid(R"({"if": {"minimum": 0}, "then": {"maximum": 1}, "else": {"maximum": -5}})", "-9"));
    BOOST_TEST(!valid(R"({"if": {"minimum": 0}, "then": {"maximum": 1}})", "2"));
    BOOST_TEST(valid(R"({"then": false})", "1"));   // then without if compiles to nothing
}

void loops() {
    BOOST_TEST(!valid(R"({"properties": {"a/b": {"type": "string"}}})", R"({"a/b": 1})"));
    BOOST_TEST(!valid(R"({"patternProperties": {"^x": false}})", R"({"xy": 1})"));
    BOOST_TEST(!valid(R"({"properties": {"a": true}, "additionalProperties": false})", R"({"b": 1})"));
    BOOST_TEST(valid(R"({"properties": {"a": true}, "additionalProperties": false})", R"({"a": 1})"));
    BOOST_TEST(!valid(R"({"propertyNames": {"maxLength": 1}})", R"({"ab": 1})"));
    BOOST_TEST(!valid(R"({"dependentSchemas": {"a": {"required": ["b"]}}})", R"({"a": 1})"));
    BOOST_TEST(!valid(R"({"prefixItems": [{"type": "string"}], "items": false})", R"(["a", 1])"));
    BOOST_TEST(valid(R"({"prefixItems": [{"type": "string"}], "items": false})", R"(["a"])"));
    BOOST_TEST(!valid(R"({"contains": {"type": "null"}})", "[1, 2]"));
    BOOST_TEST(!valid(R"({"contains": true, "maxContains": 1})", "[1, 2]"));
    BOOST_TEST(valid(R"({"contains": false, "minContains": 0})", "[1]"));
}

void depth_is_limited() {
    const auto schema = validator.compile(boost::json::parse(R"({"items": {"items": {"items": {"items": {}}}}})"));
    const js::validate_options limited{.max_depth = 3};
    BOOST_TEST(schema->is_valid(boost::json::parse("[[[1]]]"), limited));
    BOOST_TEST(!schema->is_valid(boost::json::parse("[[[[1]]]]"), limited));
}

}  // namespace

int main() {
    assertions();
    logical_applicators();
    loops();
    depth_is_limited();
    return boost::report_errors();
}
```

  `test/compile_throw_test.cpp`, with exceptions. It sets
  `validate_schema = false`, so that the loader's shape check is what
  throws, whatever meta-schema checks come later:

```cpp
#include <webcpp/json_schema/validator/validator.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/json/parse.hpp>
#include <boost/system/system_error.hpp>

namespace js = webcpp::json_schema;

int main() {
    const js::validator_for<js::draft2020_12> validator;
    const js::schema s = validator.compile_or_throw(boost::json::parse(R"({"type": "string"})"));
    BOOST_TEST(s.is_valid(boost::json::parse("\"a\"")));
    try {
        (void)validator.compile_or_throw(boost::json::parse(R"({"minLength": -1})"), {.validate_schema = false});
        BOOST_ERROR("compile_or_throw of a refused schema returned");
    } catch (const boost::system::system_error& e) {
        BOOST_TEST(e.code() == js::errc::invalid_keyword_value);
    }
    return boost::report_errors();
}
```

  Append to `compile_errors_test.cpp`, called from `main`. The meta-check
  that comes with meta-schemas leaves a regex refusal to the compiler, since
  `format: regex` only annotates:

```cpp
void compile_errors() {
    const auto compiled = validator.compile(boost::json::parse(R"({"properties": {"a": {"pattern": "("}}})"));
    if (!BOOST_TEST(compiled.has_error())) {
        return;
    }
    BOOST_TEST(compiled.error().code == js::errc::invalid_regex);
    BOOST_TEST_EQ(std::string(compiled.error().where), "/properties/a/pattern");
    BOOST_TEST_EQ(std::string(compiled.error().what), "(");
    js::ir::document broken;
    (void)broken.add_resource({.uri = broken.intern("urn:webcpp:root"), .source_dialect = js::dialect::draft2020_12,
                               .root = js::ir::node_id{0}, .parent = js::ir::resource_id{0}, .vocabularies = 0,
                               .recursive_anchor = false});
    broken.set_root(broken.reserve(js::ir::resource_id{0}, ""));   // reserved, never defined
    BOOST_TEST(validator.compile(broken).error().code == js::errc::invalid_ir);
}
```

  Append to `test/Jamfile`:

```
webcpp.headers-alone json_schema : ../include : json_schema/validator/schema.hpp ;
webcpp.run vm : vm_test.cpp ;
# compile_or_throw raises, which this test catches.
webcpp.run compile_throw : compile_throw_test.cpp : <exception-handling>on ;
```

  E1.5's call on `json_schema/detail/vm/*.hpp` already takes `machine.hpp`.
- [ ] **Step 2: Run them and watch them fail.** `$B2 libs/json_schema/test//vm`.
  Expected: FAIL, `no member named 'compile' in 'basic_validator'`.
- [ ] **Step 3: Implement.** The machine, iterative on an explicit stack
  (spec 7.3):

```cpp
template <class Traits, class Sink> requires evaluation_sink<Sink>
class machine {
    enum class next : std::uint8_t { advance, wait, stop };
    struct frame {
        std::uint32_t block;
        std::uint32_t pc;
        const boost::json::value* instance;
        std::uint32_t depth;
        std::uint32_t instance_path_size;
        std::uint32_t evaluation_path_size;
        bool valid;
        // the waiting applicator's state, reset when its instruction completes
        std::uint64_t cursor;
        std::uint64_t count;
        std::uint64_t applicator;   // its index among anyOf and oneOf evaluations
        bool keep_failed;           // its failed alternatives count (sink.hpp, P4)
        typename Sink::mark mark;
        std::uint32_t details;      // where its names begin on details_
    };

public:
    bool run(const boost::json::value& instance) {
        push_frame(plan_.root_block, instance, 0);
        std::optional<bool> returned;
        while (!frames_.empty()) {
            ++steps_;
            const std::size_t top = frames_.size() - 1;
            const block& b = plan_.blocks[frames_[top].block];
            if (frames_[top].pc == b.first + b.count) {
                returned = pop_frame();
                continue;
            }
            if (pending_) {   // a child the depth limit refused, delivered as failed
                returned = std::exchange(pending_, std::nullopt);
            }
            const instruction& in = plan_.code[frames_[top].pc];
            switch (step(top, in, std::exchange(returned, std::nullopt))) {
                case next::advance: ++frames_[top].pc; break;
                case next::wait: break;   // step pushed a child; frames_ may have moved
                case next::stop: frames_[top].pc = b.first + b.count; break;   // P6
            }
        }
        return *returned;
    }

private:
    next step(std::size_t top, const instruction& in, std::optional<bool> child) {
        switch (in.op) {
            case opcode::ret: return next::advance;
            case opcode::type_guard: return step_guard(top, in);
            case opcode::all_of: return step_all_of(top, in, child);
            case opcode::any_of:
            case opcode::one_of: return step_alternatives(top, in, child);
            case opcode::not_: return step_not(top, in, child);
            case opcode::if_then_else: return step_if(top, in, child);
            case opcode::loop_properties:
            case opcode::loop_property_names:
            case opcode::loop_dependent_schemas:
            case opcode::loop_items:
            case opcode::loop_contains: return step_loop(top, in, child);
            default: return step_assertion(top, in);
        }
    }
```

  The assertion family is one member function, with one helper per
  opcode returning `true` when the instance passes:

```cpp
    next step_assertion(std::size_t top, const instruction& in) {
        const boost::json::value& v = *frames_[top].instance;
        const bool passed = assertion_holds(in, v);   // a switch over opcodes 2 to 17
        if (passed) {
            return next::advance;
        }
        fail_keyword(top, in, kind_of(in.op), details_of(in, v));
        return Sink::records ? next::advance : next::stop;
    }

    void fail_keyword(std::size_t top, const instruction& in, error_kind kind, const error_details& details) {
        frames_[top].valid = false;
        if constexpr (Sink::records) {
            sink_.fail(error_access::make_error(error_fields_of(top, in, kind, details)));
        }
    }
```

  - `error_fields_of` renders three locations into scratch strings of the
    validation's resource:
    - the instance location, from `instance_path_`;
    - the keyword location, from `evaluation_path_` plus the instruction's
      keyword name;
    - the absolute location: its node's resource URI, `#`, the node's
      pointer and `/` with the keyword name.
  - It points `expected` at the keyword's source in the document and
    `received` at the instance value.
  - The numeric helpers compare `decompose_value(v)` with the bound's
    `decompose`, or call `is_multiple_of`. A value with no decomposition (a
    non-finite double built in code) fails every numeric assertion and
    passes `type: number`.
  - `assert_length` counts `code_points`.
  - `assert_pattern` calls `R::search(regex, text, allocator_)`. `limit` is
    a failure of kind `pattern_limit`.
  - `assert_unique` compares pairs with `detail::equal` in the validation's
    resource.

  The logical family:

```cpp
    next step_all_of(std::size_t top, const instruction& in, std::optional<bool> child) {
        const auto children = operands(in);
        if (child) {
            frames_[top].valid = frames_[top].valid && *child;
            if (!*child && !Sink::records) {
                return finish(top, next::stop);
            }
        }
        if (frames_[top].cursor == children.size()) {
            return finish(top, next::advance);
        }
        const auto i = static_cast<std::uint32_t>(frames_[top].cursor++);
        push_child(children[i], *frames_[top].instance, in, index_token(i), std::nullopt);
        return next::wait;
    }

    // anyOf and oneOf: P4's tentative discard on the first run, the replay on the second
    next step_alternatives(std::size_t top, const instruction& in, std::optional<bool> child) {
        frame& f = frames_[top];
        const auto children = operands(in);
        if (!child) {
            f.applicator = next_applicator_++;
            f.keep_failed = options_.replayed != nullptr && test_bit(*options_.replayed, f.applicator);
        } else if (*child) {
            ++f.count;
        } else if (Sink::records && !f.keep_failed) {
            sink_.discard(f.mark);
        }
        if (const auto decided = decided_early(f, in, child)) {   // P6: anyOf's first pass, oneOf's second
            return finish_alternatives(top, in, *decided);
        }
        if (f.cursor == children.size()) {
            return finish_alternatives(top, in, in.op == opcode::any_of ? f.count > 0 : f.count == 1);
        }
        if constexpr (Sink::records) {
            f.mark = sink_.checkpoint();
        }
        const auto i = static_cast<std::uint32_t>(f.cursor++);
        push_child(children[i], *f.instance, in, index_token(i), std::nullopt);
        return next::wait;
    }

    next finish_alternatives(std::size_t top, const instruction& in, bool passed) {
        frame& f = frames_[top];
        if (!passed && f.count == 0 && options_.noted != nullptr) {
            set_bit(*options_.noted, f.applicator);   // its failed alternatives count
        }
        if (!passed) {
            fail_keyword(top, in, in.op == opcode::any_of ? error_kind::any_of : error_kind::one_of,
                         matching_indices(top));
        }
        return finish(top, passed || Sink::records ? next::advance : next::stop);
    }
```

  - `decided_early` returns nothing with `Sink::records`. Without it:
    - `anyOf` is decided `true` at its first pass, unless its `flags` mark
      it exhaustive;
    - `oneOf` is decided `false` at its second pass.
  - `step_not` runs its child, discards the child's records whatever the
    result (P4), and fails with `not_schema` when the child passed.
  - `step_if` runs the condition, discards its records when it failed,
    then runs `then` or `else` when present, and fails with `if_then_else`
    when the branch failed.
  - `finish(top, n)` resets the frame's applicator state.
  - `push_frame` and `pop_frame`:
    - `push_frame` records the path sizes;
    - with a recording sink, `push_frame` calls `sink_.enter` and
      `pop_frame` calls `sink_.leave`;
    - `pop_frame` truncates the two paths and returns the frame's `valid`.
  - `matching_indices` reads the passing children's indices off
    `details_`, where `step_alternatives` pushes each one.
  The loops are one member function, `step_loop`, dispatching to one
  helper each. The property loop:

```cpp
    next step_properties(std::size_t top, const instruction& in, std::optional<bool> child) {
        frame& f = frames_[top];
        const auto& members = f.instance->get_object();
        if (child && !*child) {
            f.valid = false;
            push_detail(top, (members.begin() + static_cast<std::ptrdiff_t>(f.cursor - 1))->key());
            if (!Sink::records) {
                return finish(top, next::stop);
            }
        }
        while (f.cursor < members.size()) {
            const auto& member = *(members.begin() + static_cast<std::ptrdiff_t>(f.cursor));
            if (auto target = next_target(top, in, member.key())) {   // name, then patterns, then additional
                push_child(target->block, member.value(), in, target->token, name_token(member.key()));
                return next::wait;
            }
            ++f.cursor;   // every target of this member ran
            f.count = 0;
        }
        if (!f.valid) {
            fail_keyword(top, in, kind_of_properties(in), names_since(top));
        }
        return finish(top, f.valid || Sink::records ? next::advance : next::stop);
    }
```

    - `next_target` walks the current member's targets with `f.count` as its
    position among them: the named block, each matching pattern, then the
    additional block when nothing matched.
    - `push_child` with an instance token checks the depth first. Past the
    limit, it calls `fail_keyword` with `depth_limit`, sets `pending_ =
    false`, and returns without pushing, so the loop receives a failed
    child at its next step.

  - `basic_validator::compile(document)` runs `document.check()`, then
    `compile_plan`, and makes the `compiled_schema` with
    `std::allocate_shared` through the schema's allocator.
  - `compile(value)` and `compile(parsed)` are `load`, then
    `compile(document)`.
  - `compile_or_throw` sits behind `#ifndef WEBCPP_JSON_SCHEMA_NO_EXCEPTIONS`
    and returns `compile(...).value()`.
  - `is_valid` builds a `resource_of` from the given allocator, a
    `flag_sink` and a machine with `machine_options{.max_depth =
    options.max_depth, .noted = nullptr, .replayed = nullptr}`, and returns
    `run`.
- [ ] **Step 4: Run them and watch them pass** on the five lanes:

```bash
lanes libs/json_schema/test
lint_clean
```

  Expected: `lanes: green`, `lint: clean`.
- [ ] **Step 5: Commit.**

```bash
cd libs/json_schema
F="include/webcpp/json_schema.hpp include/webcpp/json_schema/detail/vm/machine.hpp \
include/webcpp/json_schema/validator/schema.hpp include/webcpp/json_schema/validator/validator.hpp \
test/Jamfile test/vm_test.cpp test/compile_throw_test.cpp test/compile_errors_test.cpp"
git add $F
$G commit -m "json_schema: the machine, iterative, every core instruction, and the depth limit" -- $F
```

### Task E1.12: `validate`, the outcome, at most two runs, lifetimes and threads

After E1.11. `output/outcome.hpp` freezes at Task E1.17. The machine's
recording paths, written in E1.11 behind `if constexpr (Sink::records)`, are
proven here, and fixed here when a test finds them wrong.

**Files:**
- Create:
  - `include/webcpp/json_schema/output/outcome.hpp`;
  - `test/validate_test.cpp`, `test/lifetime_test.cpp`,
    `test/threads_test.cpp`.
- Modify: `include/webcpp/json_schema/validator/schema.hpp` (`validate`),
  `include/webcpp/json_schema.hpp`, `test/Jamfile`.

**Interfaces:**
- Produces `output/outcome.hpp` (frozen). It holds every member spec 4.6
  lists, so that no later track edits it. `message` and `to_json` join in
  Task E1.13, delegating to `output/messages.hpp` and
  `detail/output/render.hpp`:

```cpp
namespace webcpp::json_schema {
enum class output_format : std::uint8_t { flag, basic, detailed, verbose };
namespace detail {
template <class Traits> struct outcome_state {   // allocate_shared in the validation's allocator
    std::shared_ptr<const compiled_schema<Traits>> schema;   // keeps the schema alive (4.6)
    std::optional<basic_message_hook<Traits>> messages;      // must outlive the outcome
    record events;
    bool valid;
    std::uint64_t steps;                                     // both runs, for the hostile tests
    std::uint8_t runs;                                       // 1 or 2 (sink.hpp, P4)
};
}
template <class Traits> class basic_outcome {
public:
    bool valid() const noexcept;
    std::span<const error> errors() const noexcept;
    std::span<const annotation> annotations() const noexcept;
    collect collected() const noexcept;
    basic_string<Traits> message(const error& e) const;
    result<boost::json::value> to_json(output_format format, boost::json::storage_ptr storage = {}) const;
    const detail::outcome_state<Traits>& state() const noexcept;   // detail: the renderers' input
};
using outcome = basic_outcome<default_traits>;
}
```

  E1.13 adds `message` and `to_json`, and the freeze takes the file whole at
  E1.17.

  `basic_schema::validate`:

```cpp
template <class Traits>
result<basic_outcome<Traits>> basic_schema<Traits>::validate(const boost::json::value& instance,
                                                             const validate_options& options,
                                                             allocator_type allocator) const {
    detail::resource_of<allocator_type> resource(allocator);
    auto state = std::allocate_shared<detail::outcome_state<Traits>>(allocator, schema_, options.messages,
                                                                     resource.get(), options.what);
    detail::alternative_bits noted{std::pmr::polymorphic_allocator<std::uint64_t>(&resource.get())};
    {
        detail::collecting_sink sink(state->events);
        detail::machine<Traits, detail::collecting_sink> first(
            *schema_, sink, allocator, resource.get(),
            {.max_depth = options.max_depth, .noted = &noted, .replayed = nullptr});
        state->valid = first.run(instance);
        state->steps = first.steps();
        state->runs = 1;
    }
    if (!state->valid && detail::any_bit(noted)) {   // P4: once more, from the top, never nested
        state->events.reset();
        detail::collecting_sink sink(state->events);
        detail::machine<Traits, detail::collecting_sink> second(
            *schema_, sink, allocator, resource.get(),
            {.max_depth = options.max_depth, .noted = nullptr, .replayed = &noted});
        state->valid = second.run(instance);
        state->steps += second.steps();
        state->runs = 2;
    }
    state->events.finish();
    return basic_outcome<Traits>(std::move(state));
}
```

  `record::reset()` empties every vector and keeps the arena: it is part of
  the record's sink-side members, which the frozen `record.hpp` lists.
  `validate` fails only with a user keyword's own error, from user
  vocabularies on.

- [ ] **Step 1: Write the failing tests.**

`test/validate_test.cpp`:

```cpp
#include <webcpp/json_schema/validator/validator.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/json/parse.hpp>

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace js = webcpp::json_schema;

namespace {

const js::validator_for<js::draft2020_12> validator;

void errors_carry_their_places_and_details() {
    const auto schema = validator.compile(boost::json::parse(R"({
        "properties": {
            "a/b": {"maxLength": 1},
            "~": {"type": "integer"},
            "": {"minimum": 3}
        },
        "required": ["x", "y"]
    })"));
    const auto instance = boost::json::parse(R"({"a/b": "xyz", "~": "s", "": 1})");
    const auto outcome = schema->validate(instance);
    if (!BOOST_TEST(outcome.has_value()) || !BOOST_TEST_EQ(outcome->errors().size(), 5U)) {
        return;   // three assertions in source order, then properties' own error, then required's
    }
    const js::error& first = outcome->errors()[0];
    BOOST_TEST(first.kind() == js::error_kind::max_length);
    BOOST_TEST_EQ(first.instance_location(), "/a~1b");
    BOOST_TEST_EQ(first.keyword_location(), "/properties/a~1b/maxLength");
    BOOST_TEST_EQ(first.absolute_keyword_location(), "urn:webcpp:root#/properties/a~1b/maxLength");
    BOOST_TEST(&first.received() == &instance.at("a/b"));
    BOOST_TEST_EQ(first.expected(), 1);
    BOOST_TEST_EQ(std::get<std::uint64_t>(first.details().value), 3U);   // the code points measured
    BOOST_TEST_EQ(outcome->errors()[1].instance_location(), "/~0");
    BOOST_TEST_EQ(outcome->errors()[2].instance_location(), "/");
    BOOST_TEST(outcome->errors()[3].kind() == js::error_kind::properties);
    BOOST_TEST_EQ(outcome->errors()[3].keyword_location(), "/properties");
    BOOST_TEST_EQ((std::get<std::span<const std::string_view>>(outcome->errors()[3].details().value).size()), 3U);
    BOOST_TEST_EQ((std::get<std::span<const std::string_view>>(outcome->errors()[4].details().value).size()), 2U);
}

void the_two_modes_agree() {
    const std::array<std::array<std::string_view, 2>, 4> cases{{
        {R"({"anyOf": [{"type": "string"}, {"minimum": 3}]})", "1"},
        {R"({"anyOf": [{"type": "string"}, {"minimum": 3}]})", "4"},
        {R"({"oneOf": [{"minimum": 0}, {"maximum": 5}]})", "3"},
        {R"({"not": {"contains": {"const": 1}}})", "[2, 1]"},
    }};
    for (const auto& c : cases) {
        const auto schema = validator.compile(boost::json::parse(c[0]));
        const auto instance = boost::json::parse(c[1]);
        BOOST_TEST_EQ(schema->is_valid(instance), schema->validate(instance)->valid());
    }
}

void a_failing_any_of_reports_every_alternative() {
    const auto schema = validator.compile(boost::json::parse(R"({"anyOf": [{"type": "string"}, {"minimum": 3}]})"));
    const auto outcome = schema->validate(boost::json::parse("1"));
    BOOST_TEST_EQ(outcome->errors().size(), 3U);   // type, minimum, then anyOf
    BOOST_TEST(outcome->errors()[2].kind() == js::error_kind::any_of);
    BOOST_TEST_EQ(outcome->state().runs, 2U);
    const auto passing = schema->validate(boost::json::parse("4"));
    BOOST_TEST(passing->valid());
    BOOST_TEST_EQ(passing->errors().size(), 0U);
    BOOST_TEST_EQ(passing->state().runs, 1U);
}

void nested_failing_alternatives_run_the_plan_at_most_twice() {
    std::string schema = R"({"type": "string"})";
    for (int i = 0; i < 64; ++i) {
        schema = R"({"anyOf": [)" + schema + "]}";
    }
    boost::json::parse_options deep;
    deep.max_depth = 200;   // Boost.JSON's default is 32 levels
    const auto compiled = validator.compile(boost::json::parse(schema, {}, deep), {.max_schema_depth = 200});
    const auto instance = boost::json::parse("1");
    const auto outcome = compiled->validate(instance);
    BOOST_TEST(!outcome->valid());
    BOOST_TEST_EQ(outcome->errors().size(), 65U);   // the type error, then each anyOf's own
    BOOST_TEST_EQ(outcome->state().runs, 2U);
    BOOST_TEST(outcome->state().steps < 4 * 65 * 3);   // linear, not 2^64
}

void a_depth_error_names_its_place() {
    const auto schema = validator.compile(boost::json::parse(R"({"items": {"items": {"items": {"items": {}}}}})"));
    const auto outcome = schema->validate(boost::json::parse("[[[[1]]]]"), {.max_depth = 3});
    BOOST_TEST(outcome->errors()[0].kind() == js::error_kind::depth_limit);
    BOOST_TEST_EQ(outcome->errors()[0].instance_location(), "/0/0/0/0");
}

}  // namespace

int main() {
    errors_carry_their_places_and_details();
    the_two_modes_agree();
    a_failing_any_of_reports_every_alternative();
    nested_failing_alternatives_run_the_plan_at_most_twice();
    a_depth_error_names_its_place();
    return boost::report_errors();
}
```

`test/lifetime_test.cpp` (Review Focus 4):

```cpp
#include <webcpp/json_schema/validator/validator.hpp>

#include "counting_resource.hpp"

#include <boost/core/lightweight_test.hpp>
#include <boost/json/parse.hpp>

#include <optional>

namespace js = webcpp::json_schema;

int main() {
    const js::validator_for<js::draft2020_12> validator;
    const boost::json::value instance = boost::json::parse(R"({"a": "too long"})");
    std::optional<js::outcome> kept;
    {
        auto schema = validator.compile(boost::json::parse(R"({"properties": {"a": {"maxLength": 2}}})"));
        kept.emplace(*schema->validate(instance));
    }   // the handle is gone; the outcome keeps the compiled schema alive
    BOOST_TEST(!kept->valid());
    BOOST_TEST_EQ(kept->errors()[0].expected(), 2);
    BOOST_TEST_EQ(kept->errors()[0].keyword_location(), "/properties/a/maxLength");

    webcpp::test::counting_resource schema_memory;
    {
        std::pmr::monotonic_buffer_resource arena(&schema_memory);
        std::optional<js::outcome> late;
        {
            auto schema = validator.compile(boost::json::parse(R"({"type": "string"})"), {},
                                            std::pmr::polymorphic_allocator<std::byte>(&arena));
            late.emplace(*schema->validate(boost::json::parse("1")));
        }
        BOOST_TEST(!late->valid());
    }   // the user releases the arena after the last outcome
    BOOST_TEST_EQ(schema_memory.live, 0U);
    return boost::report_errors();
}
```

`test/threads_test.cpp` (Review Focus 5), native only:

```cpp
#include <webcpp/json_schema/validator/validator.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/json/parse.hpp>

#include <atomic>
#include <memory_resource>
#include <thread>
#include <vector>

namespace js = webcpp::json_schema;

int main() {
    const js::validator_for<js::draft2020_12> validator;
    const auto schema = validator.compile(boost::json::parse(R"({
        "type": "array",
        "items": {"anyOf": [{"type": "string", "pattern": "^[a-z]+$"}, {"minimum": 3}]}
    })"));
    const auto good = boost::json::parse(R"(["abc", 4, "z"])");
    const auto bad = boost::json::parse(R"(["abc", 2, "Z"])");
    std::atomic<int> wrong{0};
    std::vector<std::thread> threads;
    for (int t = 0; t < 8; ++t) {
        threads.emplace_back([&, t] {
            std::pmr::monotonic_buffer_resource own;
            for (int i = 0; i < 2000; ++i) {
                const bool g = t % 2 == 0 ? schema->is_valid(good, {}, &own) : schema->validate(good, {}, &own)->valid();
                const bool b = t % 2 == 0 ? schema->is_valid(bad, {}, &own) : schema->validate(bad, {}, &own)->valid();
                wrong += (g && !b) ? 0 : 1;
                own.release();
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
    BOOST_TEST_EQ(wrong.load(), 0);
    return boost::report_errors();
}
```

  Append to `test/Jamfile`:

```
webcpp.headers-alone json_schema : ../include : json_schema/output/outcome.hpp ;
webcpp.run validate : validate_test.cpp ;
webcpp.run lifetime : lifetime_test.cpp ;
# One compiled schema shared by threads. No WASI or emscripten target here
# runs threads, so the test is native.
webcpp.run threads : threads_test.cpp : <threading>multi : native ;
```

- [ ] **Step 2: Run them and watch them fail.**
  `$B2 libs/json_schema/test//validate`. Expected: FAIL, `no member named
  'validate'`.
- [ ] **Step 3: Implement** `outcome.hpp` (without `message` and `to_json`)
  and `validate` as above. `basic_outcome` holds the
  `shared_ptr<const outcome_state>`, and its accessors read the record.
- [ ] **Step 4: Run them and watch them pass** on the five lanes:

```bash
lanes libs/json_schema/test
lint_clean
```

  Expected: `lanes: green`, `lint: clean`.
- [ ] **Step 5: Commit.**

```bash
cd libs/json_schema
F="include/webcpp/json_schema.hpp include/webcpp/json_schema/output/outcome.hpp \
include/webcpp/json_schema/validator/schema.hpp include/webcpp/json_schema/detail/output/record.hpp \
test/Jamfile test/validate_test.cpp test/lifetime_test.cpp test/threads_test.cpp"
git add $F
$G commit -m "json_schema: validate and its outcome, the plan run at most twice, lifetimes and threads" -- $F
```

  `record.hpp` is staged for `reset()`, which this task adds.

### Task E1.13: The English messages and the four output formats

After E1.12. Both are members of the frozen outcome, so they land in E1
(ruling: whatever a later track would change in a frozen file moves into
E1). E3 then writes the catalogue, the translations, and the proofs against
the suite.

**Files:**
- Create:
  - `include/webcpp/json_schema/output/messages.hpp`;
  - `include/webcpp/json_schema/detail/output/units.hpp`,
    `detail/output/render.hpp`;
  - `test/messages_test.cpp`, `test/output_test.cpp`.
- Modify: `include/webcpp/json_schema/output/outcome.hpp` (`message`,
  `to_json`), `include/webcpp/json_schema.hpp`, `test/Jamfile`.

**Interfaces:**
- Produces:

```cpp
namespace webcpp::json_schema::detail {
struct message_template { error_kind kind; std::string_view name; std::string_view text; };
std::span<const message_template> message_table() noexcept;   // 43 rows, in error_kind order
template <class String> void format_message(std::string_view text, const error& e, String& out);
template <class Traits>
result<boost::json::value> render(const outcome_state<Traits>& state, output_format format,
                                  boost::json::storage_ptr storage);
}
```

  The parameters:
  - `{expected}`: the keyword's value as compact JSON, at most 64 bytes, the
    ellipsis `...` included, cut on a UTF-8 boundary. For `type`: the type
    names joined by " or ", unquoted.
  - `{received}`: the received value, cut likewise. For `type`: its JSON
    type name, `integer` for an integral number.
  - `{count}`: the details' number.
  - `{names}`: the details' names, each quoted, joined by ", ".
  - `{indices}`: the details' indices joined by ", ".
  - `{keyword}`: the keyword as written.

  The table keeps spec 9.2's three examples as written (`type`, `required`,
  `max_length`). The other counts are phrased so that no number needs a
  plural:

```cpp
inline std::span<const message_template> message_table() noexcept {
    using k = error_kind;
    static constexpr std::array<message_template, 43> table{{
        {.kind = k::false_schema, .name = "false_schema", .text = "No value is allowed here."},
        {.kind = k::type, .name = "type", .text = "Expected {expected} but found {received}."},
        {.kind = k::enum_value, .name = "enum_value", .text = "Expected one of {expected} but found {received}."},
        {.kind = k::const_value, .name = "const_value", .text = "Expected {expected} but found {received}."},
        {.kind = k::multiple_of, .name = "multiple_of", .text = "Expected a multiple of {expected} but found {received}."},
        {.kind = k::maximum, .name = "maximum", .text = "Expected at most {expected} but found {received}."},
        {.kind = k::exclusive_maximum, .name = "exclusive_maximum", .text = "Expected less than {expected} but found {received}."},
        {.kind = k::minimum, .name = "minimum", .text = "Expected at least {expected} but found {received}."},
        {.kind = k::exclusive_minimum, .name = "exclusive_minimum", .text = "Expected more than {expected} but found {received}."},
        {.kind = k::max_length, .name = "max_length", .text = "Expected at most {expected} characters but found {count}."},
        {.kind = k::min_length, .name = "min_length", .text = "Expected a length of at least {expected}, found {count}."},
        {.kind = k::pattern, .name = "pattern", .text = "Expected a string matching {expected} but found {received}."},
        {.kind = k::pattern_limit, .name = "pattern_limit", .text = "The pattern {expected} is too costly to match against {received}."},
        {.kind = k::format, .name = "format", .text = "Expected a string of format {expected} but found {received}."},
        {.kind = k::max_items, .name = "max_items", .text = "Expected an item count of at most {expected}, found {count}."},
        {.kind = k::min_items, .name = "min_items", .text = "Expected an item count of at least {expected}, found {count}."},
        {.kind = k::unique_items, .name = "unique_items", .text = "Expected unique items but items {indices} are equal."},
        {.kind = k::contains, .name = "contains", .text = "Expected an item matching the contains schema but found none."},
        {.kind = k::min_contains, .name = "min_contains", .text = "Expected a match count of at least {expected}, found {count}."},
        {.kind = k::max_contains, .name = "max_contains", .text = "Expected a match count of at most {expected}, found {count}."},
        {.kind = k::max_properties, .name = "max_properties", .text = "Expected a property count of at most {expected}, found {count}."},
        {.kind = k::min_properties, .name = "min_properties", .text = "Expected a property count of at least {expected}, found {count}."},
        {.kind = k::required, .name = "required", .text = "Required properties {names} are missing."},
        {.kind = k::dependent_required, .name = "dependent_required", .text = "Required properties {names} are missing."},
        {.kind = k::properties, .name = "properties", .text = "Properties {names} do not match their schemas."},
        {.kind = k::pattern_properties, .name = "pattern_properties", .text = "Properties {names} do not match the schemas of their patterns."},
        {.kind = k::additional_properties, .name = "additional_properties", .text = "Additional properties {names} are not allowed."},
        {.kind = k::property_names, .name = "property_names", .text = "Property names {names} do not match the propertyNames schema."},
        {.kind = k::dependent_schemas, .name = "dependent_schemas", .text = "The schemas required by properties {names} do not match."},
        {.kind = k::prefix_items, .name = "prefix_items", .text = "Expected the first items to match their schemas; failing: {count}."},
        {.kind = k::items, .name = "items", .text = "Expected every item to match the items schema; failing: {count}."},
        {.kind = k::unevaluated_items, .name = "unevaluated_items", .text = "Unevaluated items are not allowed; found: {count}."},
        {.kind = k::unevaluated_properties, .name = "unevaluated_properties", .text = "Unevaluated properties {names} are not allowed."},
        {.kind = k::all_of, .name = "all_of", .text = "Expected the value to match every allOf schema."},
        {.kind = k::any_of, .name = "any_of", .text = "Expected the value to match at least one anyOf schema."},
        {.kind = k::one_of, .name = "one_of", .text = "Expected the value to match exactly one oneOf schema; matching: {indices}."},
        {.kind = k::not_schema, .name = "not_schema", .text = "Expected the value not to match the not schema."},
        {.kind = k::if_then_else, .name = "if_then_else", .text = "Expected the value to match the schema its if selects."},
        {.kind = k::reference, .name = "reference", .text = "Expected the value to match the referenced schema {expected}."},
        {.kind = k::content, .name = "content", .text = "Expected content that decodes and matches its schema."},
        {.kind = k::schema, .name = "schema", .text = "Expected the value to match the schema."},
        {.kind = k::user, .name = "user", .text = "Expected the value to satisfy {keyword}."},
        {.kind = k::depth_limit, .name = "depth_limit", .text = "The value nests deeper than the limit of {count}."},
    }};
    return table;
}
```

  The four formats (spec 9.1), each walked iteratively over the record's
  units:
  - `flag` writes `{"valid": bool}`.
  - `basic` writes the counting errors in evaluation order, as units. When
    valid, it writes the annotations if they were collected, else
    `{"valid": true}`.
  - `detailed` writes the hierarchy of failing units:
    - a unit with neither children nor errors is removed;
    - a unit with one child is replaced by it;
    - the root is always kept.
  - `verbose` writes every unit, the failed branches of passing applicators
    included (`counts == false`), each with `valid`, its errors and its
    annotations. It needs `collect::everything`; anything less is
    `output_not_collected`.
  - An output unit holds `valid`, `keywordLocation`,
    `absoluteKeywordLocation`, `instanceLocation`, and `error` (the message)
    or `annotation`, with nested `errors` or `annotations`.
  - `expected` and `received` are copied into the report with
    `detail::deep_copy`, never with Boost.JSON's recursive copy.

- [ ] **Step 1: Write the failing tests.**

`test/messages_test.cpp`:

```cpp
#include <webcpp/json_schema.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/json/parse.hpp>

#include <string>

namespace js = webcpp::json_schema;

namespace {

const js::validator_for<js::draft2020_12> validator;

std::string first_message(std::string_view schema, const boost::json::value& instance,
                          const js::validate_options& options = {}) {
    const auto s = validator.compile(boost::json::parse(schema));
    const auto outcome = s->validate(instance, options);
    return std::string(outcome->message(outcome->errors()[0]));
}

std::string first_message(std::string_view schema, std::string_view instance) {
    return first_message(schema, boost::json::parse(instance));
}

void the_table_is_whole() {
    const auto table = js::detail::message_table();
    BOOST_TEST_EQ(table.size(), 43U);
    for (std::size_t i = 0; i < table.size(); ++i) {
        BOOST_TEST_EQ(static_cast<std::size_t>(table[i].kind), i + 1);   // error_kind order
        BOOST_TEST(table[i].text.ends_with('.'));
        BOOST_TEST(table[i].text.find_first_of("\"\\") == std::string_view::npos);   // the .pot needs no escape
    }
}

void templates_are_filled() {
    BOOST_TEST_EQ(first_message(R"({"type": "string"})", "3"), "Expected string but found integer.");
    BOOST_TEST_EQ(first_message(R"({"type": ["string", "null"]})", "1.5"), "Expected string or null but found number.");
    BOOST_TEST_EQ(first_message(R"({"required": ["a", "b"]})", "{}"), "Required properties \"a\", \"b\" are missing.");
    BOOST_TEST_EQ(first_message(R"({"maxLength": 2})", "\"abcd\""), "Expected at most 2 characters but found 4.");
    BOOST_TEST_EQ(first_message(R"({"maxItems": 1})", "[1, 2]"), "Expected an item count of at most 1, found 2.");
    BOOST_TEST_EQ(first_message(R"({"const": "a very long constant that goes on and on and on and on and on and on"})", "1"),
                  "Expected \"a very long constant that goes on and on and on and on and o... but found 1.");
    BOOST_TEST_EQ(first_message(R"({"uniqueItems": true})", "[1, 2, 1]"), "Expected unique items but items 0, 2 are equal.");
}

void a_cut_never_splits_a_code_point() {
    std::string dragons = "\"";
    for (int i = 0; i < 30; ++i) {
        dragons += "\xF0\x9F\x90\xB2";
    }
    dragons += "\"";
    const std::string message = first_message(R"({"const": 1})", dragons);
    BOOST_TEST(message.find("\xF0\x9F\x90...") == std::string::npos);
    BOOST_TEST(message.find("\xB2...") != std::string::npos);   // the last whole dragon, then the ellipsis
}

void a_hook_replaces_the_templates() {
    auto hook = [](const js::error& e, std::pmr::string& out) {
        out = "kind ";
        out += std::to_string(static_cast<int>(e.kind()));
    };
    const js::validate_options options{.messages = js::basic_message_hook<js::default_traits>(hook)};
    BOOST_TEST_EQ(first_message(R"({"type": "string"})", boost::json::value(3), options), "kind 2");
}

}  // namespace

int main() {
    the_table_is_whole();
    templates_are_filled();
    a_cut_never_splits_a_code_point();
    a_hook_replaces_the_templates();
    return boost::report_errors();
}
```

`test/output_test.cpp`:

```cpp
#include <webcpp/json_schema.hpp>

#include "counting_resource.hpp"

#include <boost/core/lightweight_test.hpp>
#include <boost/json.hpp>

namespace js = webcpp::json_schema;

namespace {

const js::validator_for<js::draft2020_12> validator;
const char* const schema = R"({"properties": {"a": {"type": "string"}}, "required": ["b"]})";

boost::json::value report(std::string_view schema_text, std::string_view instance, js::output_format f,
                          js::collect what = js::collect::errors) {
    const auto value = boost::json::parse(instance);
    const auto outcome = validator.compile(boost::json::parse(schema_text))->validate(value, {.what = what});
    const auto json = outcome->to_json(f);   // copies expected and received: value may die after
    return json ? *json : boost::json::value(json.error().code.message());
}

void flag_and_basic() {
    BOOST_TEST_EQ(report(schema, R"({"a": 1})", js::output_format::flag), boost::json::parse(R"({"valid": false})"));
    BOOST_TEST_EQ(report(schema, R"({"a": 1})", js::output_format::basic), boost::json::parse(R"({
        "valid": false,
        "errors": [
            {
                "valid": false,
                "keywordLocation": "/properties/a/type",
                "absoluteKeywordLocation": "urn:webcpp:root#/properties/a/type",
                "instanceLocation": "/a",
                "error": "Expected string but found integer."
            },
            {
                "valid": false,
                "keywordLocation": "/properties",
                "absoluteKeywordLocation": "urn:webcpp:root#/properties",
                "instanceLocation": "",
                "error": "Properties \"a\" do not match their schemas."
            },
            {
                "valid": false,
                "keywordLocation": "/required",
                "absoluteKeywordLocation": "urn:webcpp:root#/required",
                "instanceLocation": "",
                "error": "Required properties \"b\" are missing."
            }
        ]
    })"));
    BOOST_TEST_EQ(report(schema, R"({"a": "x", "b": 1})", js::output_format::basic),
                  boost::json::parse(R"({"valid": true})"));
}

void detailed_collapses_single_children() {
    BOOST_TEST_EQ(report(schema, R"({"a": 1, "b": 2})", js::output_format::detailed), boost::json::parse(R"({
        "valid": false,
        "keywordLocation": "",
        "absoluteKeywordLocation": "urn:webcpp:root#",
        "instanceLocation": "",
        "errors": [
            {
                "valid": false,
                "keywordLocation": "/properties/a/type",
                "absoluteKeywordLocation": "urn:webcpp:root#/properties/a/type",
                "instanceLocation": "/a",
                "error": "Expected string but found integer."
            },
            {
                "valid": false,
                "keywordLocation": "/properties",
                "absoluteKeywordLocation": "urn:webcpp:root#/properties",
                "instanceLocation": "",
                "error": "Properties \"a\" do not match their schemas."
            }
        ]
    })"));
}

void verbose_keeps_the_failed_branches() {
    BOOST_TEST_EQ(report(schema, "{}", js::output_format::verbose),
                  boost::json::value("the output format needs more than was collected"));
    const auto all = report(R"({"anyOf": [{"type": "string"}, {"type": "integer"}]})", "1",
                            js::output_format::verbose, js::collect::everything);
    BOOST_TEST_EQ(all, boost::json::parse(R"({
        "valid": true,
        "keywordLocation": "",
        "absoluteKeywordLocation": "urn:webcpp:root#",
        "instanceLocation": "",
        "annotations": [
            {
                "valid": false,
                "keywordLocation": "/anyOf/0",
                "absoluteKeywordLocation": "urn:webcpp:root#/anyOf/0",
                "instanceLocation": "",
                "errors": [
                    {
                        "valid": false,
                        "keywordLocation": "/anyOf/0/type",
                        "absoluteKeywordLocation": "urn:webcpp:root#/anyOf/0/type",
                        "instanceLocation": "",
                        "error": "Expected string but found integer."
                    }
                ]
            },
            {
                "valid": true,
                "keywordLocation": "/anyOf/1",
                "absoluteKeywordLocation": "urn:webcpp:root#/anyOf/1",
                "instanceLocation": ""
            }
        ]
    })"));
}

void a_report_lives_in_the_storage_given() {
    const auto value = boost::json::parse(R"({"a": 1})");
    const auto outcome = validator.compile(boost::json::parse(schema))->validate(value);
    webcpp::test::counting_resource counted;
    boost::json::monotonic_resource monotonic;
    const auto json = outcome->to_json(js::output_format::basic, &monotonic);
    BOOST_TEST(json->storage().get() == boost::json::storage_ptr(&monotonic).get());
    (void)counted;
}

}  // namespace

int main() {
    flag_and_basic();
    detailed_collapses_single_children();
    verbose_keeps_the_failed_branches();
    a_report_lives_in_the_storage_given();
    return boost::report_errors();
}
```

  Under the verbose format, a unit's children sit under `"annotations"`
  when the unit is valid and under `"errors"` when it is not (spec 9.1;
  2020-12 core 12.4.4).

  Append to `test/Jamfile`:

```
webcpp.headers-alone json_schema : ../include : json_schema/output/messages.hpp ;
webcpp.run messages : messages_test.cpp ;
webcpp.run output : output_test.cpp ;
```

- [ ] **Step 2: Run them and watch them fail.**
  `$B2 libs/json_schema/test//messages libs/json_schema/test//output`.
  Expected: FAIL, `'webcpp/json_schema/output/messages.hpp' file not found`
  and `no member named 'to_json'`.
- [ ] **Step 3: Implement.**
  - `format_message` scans the template once. It copies the text, and on
    `{name}` appends that parameter's rendering; an unknown name is copied
    as written. It allocates only through `out`'s allocator. The cut keeps
    whole code points: it backs off over continuation bytes (`0x80` to
    `0xBF`) before appending `...`.
  - `basic_outcome::message` calls the hook when one was given, else the
    template of the error's kind.
  - `detail/output/units.hpp` lists each unit's children from the parents,
    an `inline_stack` of `(unit, next child)`.
  - `render` builds the report in the given storage while it walks.
    `detailed`'s collapsing is decided bottom-up, in a second pass over the
    same order reversed.
  - `to_json` calls `render`.
- [ ] **Step 4: Run them and watch them pass** on the five lanes:

```bash
lanes libs/json_schema/test
lint_clean
```

  Expected: `lanes: green`, `lint: clean`.
- [ ] **Step 5: Commit.**

```bash
cd libs/json_schema
F="include/webcpp/json_schema.hpp include/webcpp/json_schema/output/messages.hpp \
include/webcpp/json_schema/output/outcome.hpp include/webcpp/json_schema/detail/output/units.hpp \
include/webcpp/json_schema/detail/output/render.hpp test/Jamfile test/messages_test.cpp test/output_test.cpp"
git add $F
$G commit -m "json_schema: English messages from one table, and the four output formats" -- $F
```

### Task E1.14: The suite runner and its known failures

After E1.13. Needs S2 and S5 on `infra/shared`: check first, with
`grep -n 'rule data ( directories + )' tools/webcpp.jam` (one line) and
`grep -n 'alias json-schema-test-suite' third_party/Jamfile`. If either is
missing, rebase the superproject branch on `infra/shared` once it carries
them:

```bash
$G rebase "$SUPERPROJECT_BASE"
```

**Files:**
- Create:
  - `test/suite/runner.hpp`;
  - `test/suite/suite_draft2020_12.cpp`;
  - `test/known_failures/draft2020-12.json`.
- Modify: `test/Jamfile`, `AGENTS.md` (recording the list).

**Interfaces:**
- Consumes:
  - `<library>/webcpp/third_party//json-schema-test-suite`, whose define
    `WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE` is the suite's root, absolute, with
    no trailing slash;
  - `[ webcpp.data <dir> ]`;
  - the validator's `compile`, `is_valid` and `validate`.
- Produces, in `webcpp::test::suite`, for every later runner:

```cpp
struct test_id { std::string file; std::string case_description; std::string test_description; };
struct run_options {
    std::string_view root;            // WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE
    std::string_view dialect;         // "draft2020-12"
    std::string_view known_failures;  // the list's path
    bool record = false;              // print the failing tests as a list, with reasons
};
std::string read_file(const std::filesystem::path& path);
std::vector<std::string> files_of(std::string_view root, std::string_view dialect);
template <class Visit> void for_each_case(std::string_view root, std::string_view dialect, Visit visit);
std::string reason_for(std::string_view dialect, const test_id& id, const boost::json::value& schema);
template <class Validator> int run(const Validator& validator, const run_options& options);
```

  - `run_options` gains members in later tasks, each appended at the end,
    so that every runner's positional braces still hold.
  - The reason of a failure is decided in this order:
    - **the permanent table**: a known deviation, keyed by dialect, file,
      case and test, empty today;
    - **the file**: `optional/format/` and `optional/format-assertion.json`
      are `E5: formats`; `optional/cross-draft.json` is
      `E5: other dialects`; draft 7's `optional/content.json` is
      `E5: content`;
    - **the case's schema**, walked iteratively:
      - `unevaluatedProperties` or `unevaluatedItems` gives
        `E2: unevaluated`;
      - `$ref`, `$dynamicRef`, `$anchor`, `$dynamicAnchor` or a nested
        `$id` gives `E2: references`;
      - a non-official `$schema` or `$vocabulary` gives `E2: vocabularies`;
      - `definitions` or `dependencies` gives `E2: compatibility keywords`.
  - A failure with no reason is not recordable: E1 fixes it.

- [ ] **Step 1: Write the runner and the program, and see the list fail.**

`test/suite/suite_draft2020_12.cpp`:

```cpp
#include "runner.hpp"

#include <webcpp/json_schema/validator/validator.hpp>

#include <string_view>

namespace js = webcpp::json_schema;

int main(int argc, char** argv) {
    const js::validator_for<js::draft2020_12> validator;
    return webcpp::test::suite::run(
        validator, {.root = WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE, .dialect = "draft2020-12",
                    .known_failures = WEBCPP_TEST_JSON_SCHEMA_KNOWN,
                    .record = argc > 1 && std::string_view(argv[1]) == "--record"});
}
```

`test/suite/runner.hpp`, split so that no function passes clang-tidy's
complexity of 25:

```cpp
#ifndef WEBCPP_TEST_JSON_SCHEMA_SUITE_RUNNER_HPP
#define WEBCPP_TEST_JSON_SCHEMA_SUITE_RUNNER_HPP

#include <webcpp/json_schema/dialects/dialect.hpp>
#include <webcpp/json_schema/errors.hpp>

#include <boost/json.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include <tuple>
#include <vector>

namespace webcpp::test::suite {

struct test_id {
    std::string file;
    std::string case_description;
    std::string test_description;
    friend bool operator<(const test_id& a, const test_id& b) {
        return std::tie(a.file, a.case_description, a.test_description) <
               std::tie(b.file, b.case_description, b.test_description);
    }
};

struct run_options {
    std::string_view root;
    std::string_view dialect;
    std::string_view known_failures;
    bool record = false;
};

inline std::string read_file(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

inline std::vector<std::string> files_of(std::string_view root, std::string_view dialect) {
    const std::filesystem::path base = std::filesystem::path(root) / "tests" / dialect;
    std::vector<std::string> files;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(base)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            files.push_back(std::filesystem::relative(entry.path(), base).generic_string());
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

template <class Visit>
void for_each_case(std::string_view root, std::string_view dialect, Visit visit) {
    const std::filesystem::path base = std::filesystem::path(root) / "tests" / dialect;
    for (const std::string& file : files_of(root, dialect)) {
        const boost::json::value cases = boost::json::parse(read_file(base / file));
        for (const auto& c : cases.as_array()) {
            visit(file, c.as_object());
        }
    }
}

// Known deviations that stay: (dialect, file, case, test) and why. Empty until one is recorded.
inline const std::map<std::array<std::string_view, 4>, std::string_view>& permanent_reasons() {
    static const std::map<std::array<std::string_view, 4>, std::string_view> table;
    return table;
}

// The keywords a case's schema holds anywhere, walked without recursion.
struct schema_traits {
    bool references = false;
    bool unevaluated = false;
    bool vocabularies = false;
    bool compatibility = false;
};

inline void note_member(schema_traits& t, std::string_view key, const boost::json::value& value, bool nested) {
    t.references = t.references || key == "$ref" || key == "$dynamicRef" || key == "$anchor" ||
                   key == "$dynamicAnchor" || (key == "$id" && nested);
    t.unevaluated = t.unevaluated || key == "unevaluatedProperties" || key == "unevaluatedItems";
    t.vocabularies = t.vocabularies || key == "$vocabulary" ||
                     (key == "$schema" && value.is_string() &&
                      !webcpp::json_schema::dialect_of_uri(value.get_string()));
    t.compatibility = t.compatibility || key == "definitions" || key == "dependencies";
}

inline schema_traits traits_of(const boost::json::value& schema) {
    schema_traits t;
    std::vector<std::pair<const boost::json::value*, bool>> work{{&schema, false}};
    while (!work.empty()) {
        const auto [v, nested] = work.back();
        work.pop_back();
        if (const auto* object = v->if_object()) {
            for (const auto& member : *object) {
                note_member(t, member.key(), member.value(), nested);
                work.emplace_back(&member.value(), true);
            }
        } else if (const auto* array = v->if_array()) {
            for (const auto& item : *array) {
                work.emplace_back(&item, true);
            }
        }
    }
    return t;
}

inline std::string reason_for(std::string_view dialect, const test_id& id, const boost::json::value& schema) {
    const auto& permanent = permanent_reasons();
    if (const auto it = permanent.find({dialect, id.file, id.case_description, id.test_description});
        it != permanent.end()) {
        return std::string(it->second);
    }
    if (id.file.starts_with("optional/format/") || id.file == "optional/format-assertion.json") {
        return "E5: formats";
    }
    if (id.file == "optional/cross-draft.json") {
        return "E5: other dialects";
    }
    if (id.file == "optional/content.json") {
        return "E5: content";
    }
    const schema_traits t = traits_of(schema);
    if (t.unevaluated) {
        return "E2: unevaluated";
    }
    if (t.references) {
        return "E2: references";
    }
    if (t.vocabularies) {
        return "E2: vocabularies";
    }
    return t.compatibility ? "E2: compatibility keywords" : "";
}

// The bookkeeping of one run: what passed, what failed, against what was listed.
struct ledger {
    std::map<test_id, std::string> known;
    std::map<test_id, bool> seen;
    boost::json::array recorded;
    int passed = 0;
    int unexpected = 0;
    int unrecordable = 0;
};

inline std::map<test_id, std::string> read_known(std::string_view path) {
    std::map<test_id, std::string> known;
    for (const auto& entry : boost::json::parse(read_file(path)).as_array()) {
        const auto& o = entry.as_object();
        known[{boost::json::value_to<std::string>(o.at("file")), boost::json::value_to<std::string>(o.at("case")),
               boost::json::value_to<std::string>(o.at("test"))}] = boost::json::value_to<std::string>(o.at("reason"));
    }
    return known;
}

inline void note_failure(ledger& l, const run_options& options, const test_id& id, const boost::json::value& schema,
                         const std::string& why) {
    if (!options.record) {
        if (!l.known.contains(id)) {
            ++l.unexpected;
            std::cout << "unexpected failure: " << id.file << " / " << id.case_description << " / "
                      << id.test_description << ": " << why << '\n';
        }
        return;
    }
    const std::string reason = reason_for(options.dialect, id, schema);
    if (reason.empty()) {
        ++l.unrecordable;
        std::cout << "not recordable: " << id.file << " / " << id.case_description << " / "
                  << id.test_description << ": " << why << '\n';
        return;
    }
    l.recorded.push_back({{"file", id.file}, {"case", id.case_description}, {"test", id.test_description},
                          {"reason", reason}});
}

template <class Validator>
void run_case(const Validator& validator, const run_options& options, const std::string& file,
              const boost::json::object& c, ledger& l) {
    typename Validator::compile_options compile;
    compile.format_assertion = file.starts_with("optional/format/") || file == "optional/format-assertion.json";
    const auto schema = validator.compile(c.at("schema"), compile);
    for (const auto& t : c.at("tests").as_array()) {
        const auto& test = t.as_object();
        const test_id id{.file = file, .case_description = boost::json::value_to<std::string>(c.at("description")),
                         .test_description = boost::json::value_to<std::string>(test.at("description"))};
        std::string why;
        if (!schema) {
            why = "schema refused: " + std::string(schema.error().where) + " " + std::string(schema.error().what) +
                  " (" + schema.error().code.message() + ")";
        } else {
            const bool flag = schema->is_valid(test.at("data"));
            const auto outcome = schema->validate(test.at("data"));
            if (!outcome || outcome->valid() != flag) {
                why = "is_valid and validate disagree";
            } else if (flag != test.at("valid").as_bool()) {
                why = test.at("valid").as_bool() ? "expected valid" : "expected invalid";
            }
        }
        l.seen[id] = why.empty();
        if (why.empty()) {
            ++l.passed;
        } else {
            note_failure(l, options, id, c.at("schema"), why);
        }
    }
}

inline int report(const ledger& l, const run_options& options) {
    if (options.record) {
        std::cout << boost::json::serialize(l.recorded) << '\n';
        return l.unrecordable == 0 ? 0 : 1;
    }
    int stale = 0;
    for (const auto& [id, reason] : l.known) {
        const auto it = l.seen.find(id);
        if (it == l.seen.end() || it->second) {
            ++stale;
            std::cout << (it == l.seen.end() ? "listed, not found: " : "listed, passes: ") << id.file << " / "
                      << id.case_description << " / " << id.test_description << '\n';
        }
    }
    std::cout << "suite " << options.dialect << ": " << l.passed << " passed, " << l.known.size()
              << " known failures, " << l.unexpected << " unexpected, " << stale << " stale\n";
    return l.unexpected == 0 && stale == 0 ? 0 : 1;
}

template <class Validator>
int run(const Validator& validator, const run_options& options) {
    ledger l;
    if (!options.record) {
        l.known = read_known(options.known_failures);
    }
    for_each_case(options.root, options.dialect, [&](const std::string& file, const boost::json::object& c) {
        run_case(validator, options, file, c, l);
    });
    return report(l, options);
}

}  // namespace webcpp::test::suite

#endif  // WEBCPP_TEST_JSON_SCHEMA_SUITE_RUNNER_HPP
```

  The record mode prints the list as one line, which `record_known` lays
  out. The list is never hand-edited.

  Append to `test/Jamfile`:

```
import regex ;

# The known failures, a directory a WASI program reaches through webcpp.data;
# the suite comes with its own target.
path-constant KNOWN : known_failures ;
.known = [ regex.replace $(KNOWN) "\\\\" "/" ] ;

rule suite-runner ( name : source )
{
    webcpp.run suite-$(name) : $(source)
      : <library>/webcpp/third_party//json-schema-test-suite
        <define>"\"WEBCPP_TEST_JSON_SCHEMA_KNOWN=\\\"$(.known)/$(name).json\\\"\""
        [ webcpp.data $(KNOWN) ]
        <dependency>known_failures/$(name).json ;
}

suite-runner draft2020-12 : suite/suite_draft2020_12.cpp ;
```

  Create `test/known_failures/draft2020-12.json` as `[]`, then run
  `$B2 libs/json_schema/test//suite-draft2020-12`. Expected: FAIL. It
  prints an `unexpected failure:` line for each test that needs a later
  track, and a last line `suite draft2020-12: <P> passed, 0 known failures,
  <U> unexpected, 0 stale` with `U > 0`.
- [ ] **Step 2: Record the list, and fix what it refuses.**

```bash
record_known draft2020-12
```

  Expected: exit 0, and no `not recordable:` line.
  - A `not recordable:` line is a core test E1 fails: fix it in the loader,
    the compiler or the machine, and record again.
  - Every entry's reason starts with `E2:` or `E5:`.
- [ ] **Step 3: Run the runner on the five lanes.**

```bash
lanes libs/json_schema/test
grep 'suite draft2020-12' bin/lane-*.log
```

  Expected: `lanes: green`, and on each lane the same line
  `suite draft2020-12: <P> passed, <K> known failures, 0 unexpected, 0 stale`.
  - Every test of these files passes, unless its case is listed for a later
    track by its schema: `additionalProperties`, `allOf`, `anyOf`,
    `boolean_schema`, `const`, `contains`, `content`, `default`,
    `dependentRequired`, `dependentSchemas`, `enum`, `exclusiveMaximum`,
    `exclusiveMinimum`, `format`, `if-then-else`, `items`, `maxContains`,
    `maxItems`, `maxLength`, `maxProperties`, `maximum`, `minContains`,
    `minItems`, `minLength`, `minProperties`, `minimum`, `multipleOf`,
    `not`, `oneOf`, `pattern`, `patternProperties`, `prefixItems`,
    `properties`, `propertyNames`, `required`, `type`, `uniqueItems`.
  - The optional `bignum`, `float-overflow`, `ecmascript-regex` and
    `non-bmp-regex` pass whole.
- [ ] **Step 4:** Add to `AGENTS.md` the paragraph "Recording the known
  failures":
  - `record_known`'s commands, written out;
  - the rule that a list never lies and is never hand-edited;
  - the order of the reasons: the permanent table, the file, the schema.

  Then run `lint_clean`. Expected: `lint: clean`.
- [ ] **Step 5: Commit.**

```bash
cd libs/json_schema
F="test/Jamfile test/suite/runner.hpp test/suite/suite_draft2020_12.cpp test/known_failures/draft2020-12.json AGENTS.md"
git add $F
$G commit -m "json_schema: the official suite's runner, its known failures recorded and never hand-edited" -- $F
```

### Task E1.15: The measured limits, allocation, hostile input

After E1.14.

**Files:**
- Create:
  - `test/measure/Jamfile`, `test/measure/parse_depth_probe.cpp`,
    `test/measure/schema_depth_probe.cpp`,
    `test/measure/regex_probe.cpp`;
  - `test/allocation_test.cpp`, `test/hostile_test.cpp`.
- Modify:
  - `include/webcpp/json_schema/json/parse.hpp` (`max_parse_depth`);
  - `include/webcpp/json_schema/validator/options.hpp`
    (`default_max_schema_depth`);
  - `include/webcpp/json_schema/regex/regex_policy.hpp`
    (`max_regex_alternatives`);
  - `include/webcpp/json_schema/regex/webcpp_regex.hpp` (`regex_inline_bytes`);
  - `test/Jamfile`.

**Interfaces:**
- Produces the four measured constants, each with its table in the comment
  above it. `max_regex_nesting` is gone: webcpp::regex bounds nesting
  itself (`max_nesting`).
- `default_max_depth` stays 1024 until E2.10 measures it with the 2020-12
  meta-schema (spec 7.6).

- [ ] **Step 1: The probes**, kept as explicit programs, linked in every lane
  and run only by hand. `test/measure/Jamfile`:

```
project : requirements <library>/webcpp/json_schema//json_schema ;

import webcpp ;

webcpp.targets native wasip2 wasip3 emscripten ;

# Programs that measure json_schema's limits on each target. Linked in every
# lane; run by hand with a depth as their argument (Limits, in the page).
webcpp.link parse-depth-probe : parse_depth_probe.cpp ;
webcpp.link schema-depth-probe : schema_depth_probe.cpp ;
webcpp.link regex-probe : regex_probe.cpp ;
```

`test/measure/parse_depth_probe.cpp`:

```cpp
#include <webcpp/json_schema/json/parse.hpp>

#include <cstdio>
#include <cstdlib>
#include <string>

// Parses {"items": ... } nested <depth> levels with Boost.JSON's parser, as js::parse runs it;
// a trap is the stack running out.
int main(int argc, char** argv) {
    if (argc != 2) {
        return 2;
    }
    const std::size_t depth = std::strtoul(argv[1], nullptr, 10);
    std::string text;
    for (std::size_t i = 0; i < depth; ++i) {
        text += R"({"items": )";
    }
    text += "true";
    text.append(depth, '}');
    boost::json::monotonic_resource storage;
    boost::json::parse_options options;
    options.max_depth = depth + 1;
    boost::system::error_code ec;
    const auto value = boost::json::parse(text, ec, &storage, options);
    std::printf("%zu %s\n", depth, ec ? "refused" : "parsed");
}
```

  `schema_depth_probe.cpp` builds the same nesting in code, and loads,
  compiles, validates `[]`, and destroys it into a counting resource. It
  prints the depth and the peak bytes. `regex_probe.cpp` compiles a pattern
  of `<n>` alternatives and prints the result; nesting is webcpp::regex's
  own bound (`max_nesting`), measured in WEBCPP_REGEX_PLAN.md, not here.

  The measurement, for each target `T` in `clang-wasip2`, `clang-wasip3`,
  `emscripten` and the native debug build:

```bash
$B2 --build-dir=bin/measure-$T toolset=$T libs/json_schema/test/measure//parse-depth-probe-program
probe=$(find bin/measure-$T -name 'parse-depth-probe-program*' -type f \( -name '*.wasm' -o -name '*.js' -o -perm -u+x \) | head -1)
for d in 64 128 256 512 1024 2048 4096; do
  case $T in
    clang-wasip*) wasmtime run "$probe" $d ;;
    emscripten) node "$probe" $d ;;
    *) "$probe" $d ;;
  esac || echo "$d trapped"
done
```

  The constants:
  - `max_parse_depth` is the largest power of two at most half the
    smallest depth that trapped on any target.
  - `default_max_schema_depth` is the largest power of two whose peak stays
    under 1 MiB on wasip2 at `-O0`.
  - `max_regex_alternatives` is half the smallest count that trapped, as a
    power of two: a wide pattern costs SRELL's compile the square of its
    alternatives, and until regex's `max_alternatives` is measured on every
    target (amendment A1) json_schema keeps its own bound; the task that
    adopts the measured value removes it.
  - `regex_inline_bytes` is the smallest power of two at which every
    pattern of the 2020-12 suite's `pattern.json`, `patternProperties.json`
    and `optional/ecmascript-regex.json` searches its strings of up to 64
    bytes without spilling.

  Each constant gets its table in the comment above it, written as what is
  measured ("measured on wasip2 at -O0 with the default stack; see
  Limits"), and in the ledger.
- [ ] **Step 2: Write the failing tests.**

`test/allocation_test.cpp` (spec 14).
- The replaced operators are pratt's `test/allocation_test.cpp` block,
  copied whole:
  - the four `operator new` and `operator delete` forms, `align_val_t`
    included;
  - `aligned_memory` and `free_aligned_memory`;
  - their `NOLINTNEXTLINE`s and `BOOST_NOINLINE`.
- Each `++global_allocations` sits inside `if (counting)`.
- The counting resources' upstream allocates with `aligned_memory`, never
  through `operator new`:

```cpp
#include <webcpp/json_schema.hpp>

#include "counting_resource.hpp"
#include "suite/runner.hpp"

#include <boost/config.hpp>
#include <boost/core/lightweight_test.hpp>

#include <algorithm>
#include <cstdlib>
#include <new>
#include <string>
#include <vector>

#ifdef _MSC_VER
#include <malloc.h>
#endif

namespace js = webcpp::json_schema;

namespace {

// This program's own instruments, the one place the replaced operators can record what they see.
bool counting = false;                // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
std::size_t global_allocations = 0;   // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

}  // namespace

// pratt's replacements, each counting only while `counting` is set (see pratt's comment).
// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
BOOST_NOINLINE void* operator new(std::size_t size) {
    global_allocations += counting ? 1 : 0;
    void* memory = std::malloc(size == 0 ? 1 : size);
    if (memory == nullptr) {
        std::abort();
    }
    return memory;
}
// ... the three other forms and the two helpers, as pratt's file has them

namespace {

// The counting resources' upstream: aligned memory that never passes through operator new.
class malloc_resource final : public std::pmr::memory_resource {
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        return aligned_memory(bytes, std::max(alignment, alignof(std::max_align_t)));
    }
    void do_deallocate(void* p, std::size_t, std::size_t) override { free_aligned_memory(p); }
    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override { return this == &other; }
};

struct sample {
    std::string schema;
    std::vector<std::string> instances;
};

std::vector<sample> read_samples() {   // before counting starts
    std::vector<sample> samples;
    webcpp::test::suite::for_each_case(WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE, "draft2020-12",
        [&](const std::string&, const boost::json::object& c) {
            sample s{.schema = boost::json::serialize(c.at("schema")), .instances = {}};
            for (const auto& t : c.at("tests").as_array()) {
                s.instances.push_back(boost::json::serialize(t.as_object().at("data")));
            }
            samples.push_back(std::move(s));
        });
    return samples;
}

}  // namespace

int main() {
    const std::vector<sample> samples = read_samples();
    const js::validator_for<js::draft2020_12> validator;
    malloc_resource upstream;
    webcpp::test::counting_resource schema_memory(&upstream);
    webcpp::test::counting_resource validation_memory(&upstream);
    const std::pmr::polymorphic_allocator<std::byte> schema_alloc(&schema_memory);
    const std::pmr::polymorphic_allocator<std::byte> validation_alloc(&validation_memory);
    std::size_t compiled = 0;
    counting = true;
    for (const sample& s : samples) {
        const auto text = js::parse(s.schema, schema_alloc);
        const auto schema = validator.compile(*text, {}, schema_alloc);
        if (!schema) {
            continue;   // a known failure until its track
        }
        ++compiled;
        for (const std::string& i : s.instances) {
            const auto instance = js::parse(i, validation_alloc);
            (void)schema->validate(instance->value, {}, validation_alloc);
            const std::size_t before = validation_memory.allocations;
            (void)schema->is_valid(instance->value, {}, validation_alloc);
            if (i.size() <= 64) {
                BOOST_TEST_EQ(validation_memory.allocations, before);   // the common path
            }
        }
    }
    counting = false;
    BOOST_TEST(compiled > 300);
    BOOST_TEST_EQ(global_allocations, 0U);
    BOOST_TEST(schema_memory.allocations > 0);
    return boost::report_errors();
}
```

  The `// ...` line in the block above stands for the remaining lines of
  pratt's replacements, copied as they are. It is the only elision in this
  test, and nothing of the replacements is written anew.

`test/hostile_test.cpp` (spec 16.5, the deep-input ruling; the reviewer's
reproductions). Every deep value is built in code, never parsed from text
deeper than the bound:

```cpp
#include <webcpp/json_schema.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/json.hpp>

#include <string>

namespace js = webcpp::json_schema;

namespace {

const js::validator_for<js::draft2020_12> validator;

boost::json::value nested(boost::json::monotonic_resource& storage, const char* key, std::size_t depth) {
    boost::json::value v = boost::json::object(&storage);
    for (std::size_t i = 0; i < depth; ++i) {
        boost::json::object outer(&storage);
        outer[key] = std::move(v);
        v = std::move(outer);
    }
    return v;
}

void deep_text_is_refused_as_a_value() {
    // reproductions: 100,000 '[' crashed natively, 1,124 levels of {"items": ...} trapped on wasip2
    const std::string brackets(100000, '[');
    BOOST_TEST(js::parse(brackets).error().code == boost::json::error::too_deep);
    std::string items;
    for (int i = 0; i < 1124; ++i) {
        items += R"({"items": )";
    }
    items += "true" + std::string(1124, '}');
    BOOST_TEST(js::parse(items).error().code == boost::json::error::too_deep);
}

void a_deep_schema_is_refused_and_freed() {
    boost::json::monotonic_resource storage;
    const auto schema = nested(storage, "not", 100000);
    BOOST_TEST(validator.load(schema).error().code == js::errc::schema_too_deep);
    const auto four_hundred = nested(storage, "items", 400);   // the recursive copy trapped here
    BOOST_TEST(validator.compile(four_hundred, {.validate_schema = false, .max_schema_depth = 1000}).has_value());
}

void deep_instances_never_reach_the_stack() {
    boost::json::monotonic_resource storage;
    boost::json::array pair(&storage);
    pair.push_back(nested(storage, "a", 100000));
    pair.push_back(nested(storage, "a", 100000));
    const auto unique = validator.compile(boost::json::parse(R"({"uniqueItems": true})"));
    BOOST_TEST(!unique->is_valid(pair));   // the two copies are equal, compared iteratively
    const std::size_t levels = js::default_max_depth + 100;
    // validate_schema off: the meta-schema check would itself stop at the instance depth limit
    const auto descending = validator.compile(nested(storage, "additionalProperties", levels),
                                              {.validate_schema = false, .max_schema_depth = levels + 1});
    const auto outcome = descending->validate(pair.at(0));
    BOOST_TEST(!outcome->valid());
    BOOST_TEST(outcome->errors()[0].kind() == js::error_kind::depth_limit);
    BOOST_TEST_EQ(outcome->errors()[0].instance_location().size(), 2 * (js::default_max_depth + 1));
    const auto report = outcome->to_json(js::output_format::basic, &storage);   // deep received, copied iteratively
    BOOST_TEST(report.has_value());
}

void hostile_patterns_are_values() {
    const auto runaway = validator.compile(boost::json::parse(R"({"pattern": "^(\\w+\\s?)*$"})"));
    const boost::json::value sentence = "A long sentence of words that ends with something refused!";
    BOOST_TEST(!runaway->is_valid(sentence));
    BOOST_TEST(runaway->validate(sentence)->errors()[0].kind() == js::error_kind::pattern_limit);
    const std::string groups = std::string(10000, '(') + "a" + std::string(10000, ')');
    boost::json::object schema;
    schema["pattern"] = groups;
    BOOST_TEST(validator.compile(schema).error().code == js::errc::invalid_regex);
}

}  // namespace

int main() {
    deep_text_is_refused_as_a_value();
    a_deep_schema_is_refused_and_freed();
    deep_instances_never_reach_the_stack();
    hostile_patterns_are_values();
    return boost::report_errors();
}
```

  `nested(storage, "a", n)` builds `{"a": {"a": ...}}`, and the schema of
  nested `additionalProperties` descends one member per level. Each
  `instance_location` adds `/a`, two bytes. The 64 nested failing `anyOf`s
  are pinned by `validate_test`.

  Append to `test/Jamfile`:

```
webcpp.run allocation : allocation_test.cpp : <library>/webcpp/third_party//json-schema-test-suite ;
webcpp.run hostile : hostile_test.cpp ;
```

- [ ] **Step 3: Run them and watch them fail or pass.**
  `$B2 libs/json_schema/test//allocation libs/json_schema/test//hostile`.
  Expected: the allocation test fails while any path allocates globally.
  Find each path with a breakpoint on the replaced `operator new` while
  `counting` is set, fix it, and run again until both pass.
- [ ] **Step 4: Run the lanes.**

```bash
lanes libs/json_schema/test
lint_clean
```

  Expected: `lanes: green`, `lint: clean`.
- [ ] **Step 5: Commit.**

```bash
cd libs/json_schema
F="include/webcpp/json_schema/json/parse.hpp include/webcpp/json_schema/validator/options.hpp \
include/webcpp/json_schema/regex/regex_policy.hpp include/webcpp/json_schema/regex/srell.hpp \
test/Jamfile test/measure/Jamfile test/measure/parse_depth_probe.cpp test/measure/schema_depth_probe.cpp \
test/measure/regex_probe.cpp test/allocation_test.cpp test/hostile_test.cpp"
git add $F
$G commit -m "json_schema: measured limits, nothing allocated outside the given allocators, hostile input" -- $F
```

### Task E1.16: Doc Comments, the reference, the page and its first examples

After E1.15. It runs after the constants are measured, so that its
token-for-token check of "no code changed" meets no later edit.

**Files:**
- Create:
  - `doc/Jamfile`, `doc/mrdocs.yml` (pratt's, `show-enum-constants: true`);
  - `doc/json_schema.adoc`, `doc/introduction.adoc`, `doc/compiling.adoc`,
    `doc/validating.adoc`, `doc/output.adoc`, `doc/numbers.adoc`,
    `doc/limits.adoc`, `doc/errors.adoc`, `doc/testing.adoc`,
    `doc/guarantees.adoc`;
  - `example/Jamfile`;
  - `example/json_schema_first.cpp`, `example/json_schema_errors.cpp`,
    `example/json_schema_numbers.cpp`, `example/json_schema_allocators.cpp`,
    `example/json_schema_output.cpp`, each with its `.expected`.
- Modify: every public header under `include/` (Doc Comments only).

- [ ] **Step 1: The failing checks.**

```bash
$B2 -a --build-dir=bin/doc libs/json_schema/doc//reference
$B2 libs/json_schema/example
```

  Expected: FAIL. The reference names each missing Doc Comment (record the
  count in the ledger), and `libs/json_schema/example` does not exist.
- [ ] **Step 2: Document every public symbol**, xactor's and pratt's
  headers being the model:
  - a one-sentence brief;
  - `@param`, `@tparam` and `@return`;
  - each enumerator and data member with its own comment;
  - the invariants cited as `[ir-N]`;
  - `@see "<title>"` naming only a section this task writes;
  - a brief on each `detail` symbol;
  - no forbidden command, and no task or sub-track name.

  Change no code. Prove it with `g++-15 -fpreprocessed -dD -E -P -x c++` on
  each header before and after, compared token by token.
- [ ] **Step 3: The examples**, each a test of the page. `example/Jamfile`:

```
project : requirements <library>/webcpp/json_schema//json_schema ;

import webcpp ;

webcpp.targets native wasip2 wasip3 emscripten ;

webcpp.example json_schema_first.cpp ;
webcpp.example json_schema_errors.cpp ;
webcpp.example json_schema_numbers.cpp ;
webcpp.example json_schema_allocators.cpp ;
webcpp.example json_schema_output.cpp ;
```

`example/json_schema_first.cpp` (spec 4.12):

```cpp
#include <webcpp/json_schema.hpp>

#include <boost/json/parse.hpp>

#include <iostream>

namespace js = webcpp::json_schema;

int main() {
    // tag::first[]
    const js::validator validator;
    auto schema = validator.compile(boost::json::parse(R"({
        "$schema": "https://json-schema.org/draft/2020-12/schema",
        "type": "object",
        "required": ["name"]
    })"));
    if (!schema) {
        std::cout << "refused at " << schema.error().where << '\n';
        return 1;
    }
    const boost::json::value instance = boost::json::parse(R"({"age": 3})");
    std::cout << "is_valid: " << std::boolalpha << schema->is_valid(instance) << '\n';
    const auto outcome = schema->validate(instance);
    for (const js::error& e : outcome->errors()) {
        std::cout << e.keyword() << " at '" << e.instance_location() << "': " << outcome->message(e) << '\n';
    }
    // end::first[]
}
```

`example/json_schema_first.expected`:

```
is_valid: false
required at '': Required properties "name" are missing.
```

`example/json_schema_errors.cpp`:

```cpp
#include <webcpp/json_schema.hpp>

#include <boost/json/parse.hpp>

#include <iostream>

namespace js = webcpp::json_schema;

int main() {
    // tag::errors[]
    const js::validator validator;
    const auto schema = validator.compile(boost::json::parse(R"({
        "properties": {"a/b": {"maxLength": 1}},
        "required": ["x"]
    })"));
    const auto instance = boost::json::parse(R"({"a/b": "xyz"})");
    const auto outcome = schema->validate(instance);
    for (const js::error& e : outcome->errors()) {
        std::cout << e.keyword() << '\n'
                  << "  instance: '" << e.instance_location() << "'\n"
                  << "  keyword: " << e.keyword_location() << '\n'
                  << "  absolute: " << e.absolute_keyword_location() << '\n';
    }
    // end::errors[]
}
```

`example/json_schema_errors.expected`:

```
maxLength
  instance: '/a~1b'
  keyword: /properties/a~1b/maxLength
  absolute: urn:webcpp:root#/properties/a~1b/maxLength
properties
  instance: ''
  keyword: /properties
  absolute: urn:webcpp:root#/properties
required
  instance: ''
  keyword: /required
  absolute: urn:webcpp:root#/required
```

`example/json_schema_numbers.cpp`:

```cpp
#include <webcpp/json_schema.hpp>

#include <boost/json/parse.hpp>

#include <iostream>

namespace js = webcpp::json_schema;

int main() {
    // tag::numbers[]
    const js::validator validator;
    const auto cents = validator.compile(boost::json::parse(R"({"multipleOf": 0.01})"));
    std::cout << std::boolalpha << cents->is_valid(boost::json::parse("0.07")) << ' '
              << cents->is_valid(boost::json::parse("0.075")) << '\n';
    const char* text = R"({"maximum": 9007199254740993.0})";
    const auto from_double = validator.compile(boost::json::parse(text));
    const auto from_lexeme = validator.compile(*js::parse(text));
    const auto instance = boost::json::parse("9007199254740993");
    std::cout << from_double->is_valid(instance) << ' ' << from_lexeme->is_valid(instance) << '\n';
    // end::numbers[]
}
```

`example/json_schema_numbers.expected`:

```
true false
false true
```

  The second line is what `js::parse` is for: the double the first schema
  holds is 9007199254740992, below the integer, and the lexeme the second
  keeps is not. The Numbers section says that constants (`const`, `enum`)
  compare at the precision their `boost::json::value` holds (spec 12,
  risk 9).

`example/json_schema_allocators.cpp`:

```cpp
#include <webcpp/json_schema.hpp>

#include <boost/json/parse.hpp>

#include <iostream>
#include <memory_resource>

namespace js = webcpp::json_schema;

int main() {
    // tag::allocators[]
    std::pmr::monotonic_buffer_resource schema_memory;
    std::pmr::monotonic_buffer_resource validation_memory;
    const js::validator validator;
    const auto schema = validator.compile(boost::json::parse(R"({"items": {"type": "integer"}})"), {},
                                          &schema_memory);
    const auto outcome = schema->validate(boost::json::parse("[1, 2.5]"), {}, &validation_memory);
    std::cout << "valid: " << std::boolalpha << outcome->valid() << '\n';
    // end::allocators[]
}
```

`example/json_schema_allocators.expected`:

```
valid: false
```

`example/json_schema_output.cpp`:

```cpp
#include <webcpp/json_schema.hpp>

#include <boost/json.hpp>

#include <iostream>

namespace js = webcpp::json_schema;

int main() {
    // tag::output[]
    const js::validator validator;
    const auto schema = validator.compile(boost::json::parse(R"({"properties": {"a": {"type": "string"}}})"));
    const auto instance = boost::json::parse(R"({"a": 1})");
    const auto outcome = schema->validate(instance);
    std::cout << boost::json::serialize(*outcome->to_json(js::output_format::flag)) << '\n';
    std::cout << boost::json::serialize(*outcome->to_json(js::output_format::basic)) << '\n';
    // end::output[]
}
```

`example/json_schema_output.expected`:

```
{"valid":false}
{"valid":false,"errors":[{"valid":false,"keywordLocation":"/properties/a/type","absoluteKeywordLocation":"urn:webcpp:root#/properties/a/type","instanceLocation":"/a","error":"Expected string but found integer."},{"valid":false,"keywordLocation":"/properties","absoluteKeywordLocation":"urn:webcpp:root#/properties","instanceLocation":"","error":"Properties \"a\" do not match their schemas."}]}
```

- [ ] **Step 4: The page.** `doc/Jamfile` declares `webcpp.doc json_schema :
  json_schema.adoc ;` and `webcpp.reference json_schema ;`.
  `json_schema.adoc` opens as AGENTS chapter 8 says, titled `json_schema:
  JSON Schema validation for {cpp}`, and includes these sections, each
  listing an example by its tag:
  - **introduction**, with `json_schema_first`;
  - **compiling** (the options that work today, `load`, `compile`, `ir()`);
  - **validating** (`is_valid`, `validate`, `outcome`, `error`), with
    `json_schema_errors`;
  - **output formats and messages**, with `json_schema_output`;
  - **numbers and exactness**, with `json_schema_numbers`;
  - **limits:**
    - the measured constants and their tables;
    - hostile input;
    - how a program builds and frees a deep instance:
      `boost::json::monotonic_resource`, and `parse_options::max_depth`,
      which `js::parse` clamps;
  - **errors and exceptions**, with `json_schema_allocators` as the
    allocation listing;
  - **testing:** the suite, its runner, the known-failure lists, the lanes;
  - **the guarantees**, numbered with anchors `json_schema-invariant-<n>`:
    1. a compiled schema is immutable and may be shared between threads;
    2. `is_valid` and `validate` agree;
    3. `is_valid` allocates nothing on the common path;
    4. no function recurses on a JSON value's depth, and parsing is bounded;
    5. the library touches no network, disk or clock;
    6. a runaway or oversized pattern is an error, never a hang or an
       abort;
    7. `validate` runs the plan at most twice;
  - **the reference**, under `[#reference]`.

  No section is written before its code; later tracks add theirs.
- [ ] **Step 5: Run the checks and watch them pass.**

```bash
$B2 -a --build-dir=bin/doc libs/json_schema/doc
lanes libs/json_schema/test libs/json_schema/example
lint_clean
```

  Expected:
  - the doc build exits 0, doc-check and MrDocs strict included;
  - `lanes: green`;
  - `lint: clean`.

  Look at the page at 320 and 1280 px.
- [ ] **Step 6: Commit.** The changed headers are listed by
  `git -C libs/json_schema diff --name-only -- include`, and each is
  staged by its path:

```bash
cd libs/json_schema
F="$(git diff --name-only -- include | tr '\n' ' ') doc/Jamfile doc/mrdocs.yml doc/json_schema.adoc \
doc/introduction.adoc doc/compiling.adoc doc/validating.adoc doc/output.adoc doc/numbers.adoc doc/limits.adoc \
doc/errors.adoc doc/testing.adoc doc/guarantees.adoc example/Jamfile example/json_schema_first.cpp \
example/json_schema_first.expected example/json_schema_errors.cpp example/json_schema_errors.expected \
example/json_schema_numbers.cpp example/json_schema_numbers.expected example/json_schema_allocators.cpp \
example/json_schema_allocators.expected example/json_schema_output.cpp example/json_schema_output.expected"
git add $F
$G commit -m "json_schema: Doc Comments, the strict reference, the page and its first examples" -- $F
```

### Task E1.17: E1 complete; the freeze (controller and reviewer)

After E1.16. **This is the freeze point** (spec 5.5).

**Files:**
- Review: the files of `$FROZEN`:
  - `ir/ids.hpp`, `ir/number.hpp`, `ir/keyword.hpp`, `ir/node.hpp`,
    `ir/document.hpp`;
  - `detail/vm/event.hpp`, `detail/vm/sink.hpp`;
  - `detail/output/record.hpp`;
  - `output/error.hpp`, `output/outcome.hpp`;
  - `output/message_hook.hpp`.
- Modify: `~/dev/webcpp-repos/json_schema` (its `main` fast-forwarded to
  `e1`; the tag `contract-1`).
- Create: the worktrees of E2, E3 and E4.

- [ ] **Step 1: Green from scratch.**

```bash
cd ~/dev/webcpp-json-schema-e1
lanes libs/json_schema/test libs/json_schema/example
$B2 -a --build-dir=bin/doc libs/json_schema/doc
lint_clean
for t in $(find tools -name '*_test.py' -not -path '*/node_modules/*' | sort); do
  python3 "$t" > "/tmp/json_schema-$(basename "$t").log" 2>&1 && echo "ok $t" || echo "FAILED $t"
done
test ! -e ~/dev/webcpp/libs/json_schema && echo absent
```

  Expected:
  - `lanes: green`;
  - the doc build exits 0;
  - `lint: clean`;
  - one `ok` line per tools test, no `FAILED` (E1 changes no tool, so this
    proves the worktree sound);
  - `absent`.
- [ ] **Step 2: The freeze review**, by a fresh reviewer on the most
  capable model, the spec open beside the files. Each item is answered
  "holds" or "fails, because":
  1. Every type of spec 5.1 and 5.2 exists with the spec's names, member
     order, member types and variant order, the qualified `ir::number`
     excepted, as E1.7 states.
  2. `keyword_kind`'s values equal spec 5.2's table one by one;
     `payload_index` returns that table's alternative for each, the eight
     counts included; `ir_test` pins them.
  3. `vocabulary_bit`'s eight bits match `resource::vocabularies`' comment.
  4. `basic_document` has:
     - spec 5.4's reading and building members;
     - `add_value_at`;
     - the allocator-taking copy;
     - values in a trivially freed arena;
     - nothing that exposes a pointer into its tables beyond the views spec
       5.4 returns.
  5. `check()` refuses a fault of each invariant 1 to 11 and 13 that can be
     built through the public API, a second `define` included, and
     `ir_test` pins each one.
  6. Invariants 1 to 14 are written, numbered, in `document.hpp`'s Doc
     Comments, each cited where it is enforced.
  7. The frozen headers include nothing their boundary forbids; the lint
     proves it.
  8. `evaluation_sink` and `event_context` are as E1.5 wrote them, and P1
     to P6 are stated in `sink.hpp`. The machine keeps each:
     - `validate_test` pins P4's two runs, never more;
     - `sink_test` pins P2, and P3 under every `collect`.
  9. `error` and `annotation` have exactly spec 4.6's accessors, and
     `error_kind`'s values are 1 to 43 as spec 4.6 lists them.
  10. `record.hpp` gives E2 and E3 what they need without an edit:
      - E2: `storage()` for computed annotations, and the
        annotation-recording calls;
      - E3: units with parents, `counts` and locations; errors and
        annotations by unit, both what counts and what was recorded.
  11. `outcome.hpp` holds every member spec 4.6 lists, and `outcome_state`
      the `messages` hook, `steps` and `runs`.
  12. `message_hook.hpp` is spec 4.11's alias, alone.
  13. What E2 and E4 read is in the IR: `dynamic_reference`'s fields,
      `resource::recursive_anchor`, anchors with `dynamic`, and what the
      emitter's table (spec 11.1) reads.

  A fault is fixed in E1 before the freeze, with its test, and the review
  runs again on the fixed files.
- [ ] **Step 3: Freeze, and fast-forward the canonical repository.**

```bash
cd ~/dev/webcpp-json-schema-e1/libs/json_schema
$G tag -a contract-1 -m "The IR, the evaluation sink, the record, the outcome and the message hook"
git -C ~/dev/webcpp-repos/json_schema fetch ~/dev/webcpp-json-schema-e1/libs/json_schema \
  e1 'refs/tags/contract-1:refs/tags/contract-1'
ids_ok ~/dev/webcpp-repos/json_schema FETCH_HEAD
$G -C ~/dev/webcpp-repos/json_schema merge --ff-only FETCH_HEAD
git -C ~/dev/webcpp-repos/json_schema ls-tree contract-1 -- $FROZEN
test ! -e ~/dev/webcpp/libs/json_schema && echo absent
```

  Expected:
  - `ids_ok` prints nothing;
  - `~/dev/webcpp-repos/json_schema` is on `main` at E1's last commit, with
    the tag;
  - eleven blob lines;
  - `absent`.

  Record in the ledger the eleven blob ids and the runner's slice line.
  From here on, `merge_track` enforces spec 5.5.
- [ ] **Step 4: The worktrees of E2, E3 and E4.**

```bash
track_worktree 2 && track_worktree 3 && track_worktree 4
```

  Expected: three worktrees, each with its two submodule status lines and
  its library at `contract-1`, on `e2`, `e3` and `e4`.
- [ ] **Step 5: The report** ends with the Changelog section of the Global
  Constraints, and the slice's line.

## E2: the compiler and the VM completed; references; annotations

In `~/dev/webcpp-json-schema-e2`, the library on `e2`, from `contract-1`.
E2.1 and E2.2 run at once (`task_worktree 2 url`, `task_worktree 2 meta`),
merged by `merge_task 2 url` then `merge_task 2 meta`. After E2.4, two
lanes:
- E2.5, E2.6, E2.7 and E2.10 in the track's worktree;
- E2.8 then E2.9 in `task_worktree 2 annotations`, merged by `merge_task 2
  annotations` after E2.9.

E2 edits no frozen file; `merge_track 2` checks it.

The E1 tests that compile a schema the meta-schema check (E2.5) would
refuse already set `validate_schema = false`, or use `load` or
`compile(document)`, which never check:
- `compile_throw_test`;
- `compile_errors_test`'s `load_errors` and its `invalid_ir` case;
- `hostile_test`'s deep compiles.

E2.5's Step 2 runs the whole suite of tests to prove that no other E1 test
depends on it.

### Task E2.1: The URL policy over Boost.URL

Runs alongside E2.2 (slug `url`). Needs S4.

**Files:**
- Create: `include/webcpp/json_schema/url/url_policy.hpp`,
  `include/webcpp/json_schema/url/boost_url.hpp`, `test/url_test.cpp`.
- Modify:
  - `build.jam` (`<library>/webcpp//boost_url`);
  - `include/webcpp/json_schema/traits.hpp` (`default_traits` gains `using
    url_policy = boost_url<>;`, and the concept `url_policy<typename
    T::url_policy>`);
  - `include/webcpp/json_schema.hpp`, `test/Jamfile`,
    `meta/include-boundaries.json` (two objects: `url/url_policy.hpp` with
    the four prefixes; `url/boost_url.hpp` without `boost/url`).

**Interfaces:**
- Produces spec 4.2's `url_policy` and `boost_url<Capacity>`:

```cpp
template <std::size_t Capacity = 4096>
struct boost_url {
    static constexpr std::size_t capacity = Capacity;
    static boost::system::result<void> resolve(std::string_view base, std::string_view ref,
                                               boost::static_strings::static_string<Capacity>& out);
    static bool is_absolute(std::string_view ref) noexcept;
};
```

  A URI longer than the capacity is `boost::system::errc::value_too_large`.
  The loader maps every refusal to `invalid_uri`, naming the URI.

- [ ] **Step 1: Write the failing test** `test/url_test.cpp`:

```cpp
#include <webcpp/json_schema/url/boost_url.hpp>

#include <boost/core/lightweight_test.hpp>

#include <string>

namespace js = webcpp::json_schema;
using url = js::boost_url<>;

static_assert(js::url_policy<url>);

namespace {

std::string resolved(std::string_view base, std::string_view ref) {
    boost::static_strings::static_string<url::capacity> out;
    const auto r = url::resolve(base, ref, out);
    return r ? std::string(out.data(), out.size()) : "error: " + r.error().message();
}

void rfc3986_section_5_4() {
    const std::string_view base = "http://a/b/c/d;p?q";
    BOOST_TEST_EQ(resolved(base, "g"), "http://a/b/c/g");
    BOOST_TEST_EQ(resolved(base, "./g"), "http://a/b/c/g");
    BOOST_TEST_EQ(resolved(base, "g/"), "http://a/b/c/g/");
    BOOST_TEST_EQ(resolved(base, "/g"), "http://a/g");
    BOOST_TEST_EQ(resolved(base, "//g"), "http://g");
    BOOST_TEST_EQ(resolved(base, "?y"), "http://a/b/c/d;p?y");
    BOOST_TEST_EQ(resolved(base, "#s"), "http://a/b/c/d;p?q#s");
    BOOST_TEST_EQ(resolved(base, ".."), "http://a/b/");
    BOOST_TEST_EQ(resolved(base, "../../../g"), "http://a/g");
}

void normalized_and_bounded() {
    BOOST_TEST_EQ(resolved("HTTP://Example.COM/a/", "%7euser"), "http://example.com/a/~user");
    BOOST_TEST_EQ(resolved("urn:webcpp:root", "#a"), "urn:webcpp:root#a");
    BOOST_TEST_EQ(resolved("urn:webcpp:root", "http://x/y.json"), "http://x/y.json");
    const std::string tail(5000, 'x');
    boost::static_strings::static_string<url::capacity> out;
    BOOST_TEST(url::resolve("http://a/", tail, out).error() == boost::system::errc::value_too_large);
    BOOST_TEST(resolved("http://a/", "a b").starts_with("error"));
    BOOST_TEST(url::is_absolute("urn:x"));
    BOOST_TEST(!url::is_absolute("x.json#a"));
}

}  // namespace

int main() {
    rfc3986_section_5_4();
    normalized_and_bounded();
    return boost::report_errors();
}
```

  Append to `test/Jamfile`:

```
webcpp.headers-alone json_schema : ../include : json_schema/url/*.hpp ;
webcpp.run url : url_test.cpp ;
```

- [ ] **Step 2: Run it and watch it fail.** `$B2 libs/json_schema/test//url`.
  Expected: FAIL, `'webcpp/json_schema/url/boost_url.hpp' file not found`.
- [ ] **Step 3: Implement.**

```cpp
template <std::size_t Capacity>
boost::system::result<void> boost_url<Capacity>::resolve(
    std::string_view base, std::string_view ref, boost::static_strings::static_string<Capacity>& out) {
    if (base.size() + ref.size() + 1 > Capacity) {
        // a resolved URI is never longer than its base and its reference together, so static_url,
        // which would raise past its capacity, never grows that far
        return boost::system::errc::make_error_code(boost::system::errc::value_too_large);
    }
    const auto parsed_base = boost::urls::parse_uri(base);
    if (!parsed_base) {
        return parsed_base.error();
    }
    const auto parsed_ref = boost::urls::parse_uri_reference(ref);
    if (!parsed_ref) {
        return parsed_ref.error();
    }
    boost::urls::static_url<Capacity> result;
    if (const auto r = boost::urls::resolve(*parsed_base, *parsed_ref, result); !r) {
        return r.error();
    }
    result.normalize();
    out.assign(result.buffer().data(), result.buffer().size());
    return {};
}
```

- [ ] **Step 4: Run it and watch it pass** on the five lanes:

```bash
lanes libs/json_schema/test
lint_clean
```

  Expected: `lanes: green`, `lint: clean`.
- [ ] **Step 5: Commit.**

```bash
cd libs/json_schema
F="build.jam include/webcpp/json_schema.hpp include/webcpp/json_schema/traits.hpp \
include/webcpp/json_schema/url/url_policy.hpp include/webcpp/json_schema/url/boost_url.hpp \
meta/include-boundaries.json test/Jamfile test/url_test.cpp"
git add $F
$G commit -m "json_schema: the URL policy, Boost.URL resolving and normalizing into a stack buffer" -- $F
```

### Task E2.2: The bundled meta-schemas, generated and checked

Runs alongside E2.1 (slug `meta`). Needs S1.
- **Written for decision 1's recommended outcome (a):** the fetched files
  are committed with `LICENSE-JSON-SCHEMA.txt`, and each generated header
  carries webcpp's notice and a line naming its origin and licence.
- **Under (b):** the files are fetched at build time from a pinned upstream
  commit, and the headers are generated into the build directory by the
  same `generate.py`. Then:
  - `LICENSE-JSON-SCHEMA.txt` and `meta_schemas/*.json` are not committed;
  - `build.jam` gains the generated directory as an `<include>`;
  - the `webcpp.generated` check becomes a check of the fetched files'
    SHA-256 against `meta_schemas/README.md`.

  The controller revises this task before dispatch if the owner chooses (b).

**Files:**
- Create:
  - `meta_schemas/README.md`, `meta_schemas/generate.py`;
  - `meta_schemas/draft2020-12/schema.json`;
  - `meta_schemas/draft2020-12/meta/{core,applicator,unevaluated,validation,meta-data,format-annotation,format-assertion,content}.json`;
  - `meta_schemas/draft2019-09/schema.json`;
  - `meta_schemas/draft2019-09/meta/{core,applicator,validation,meta-data,format,content}.json`;
  - `meta_schemas/draft-07/schema.json`, `meta_schemas/draft-06/schema.json`,
    `meta_schemas/draft-04/schema.json`;
  - `LICENSE-JSON-SCHEMA.txt`;
  - `include/webcpp/json_schema/detail/meta_schemas/*.hpp` (19 headers,
    generated) and `detail/meta_schemas/index.hpp` (generated).
- Modify: `test/Jamfile`, `.gitattributes`, `README.md`,
  `meta/include-boundaries.json`.

**Interfaces:**
- Produces:

```cpp
namespace webcpp::json_schema::detail {
struct meta_schema {
    std::string_view uri;                       // its $id (draft 4: id), without "#"
    std::span<const std::string_view> chunks;   // its bytes, cut at 8,000 bytes on UTF-8 boundaries
};
std::span<const meta_schema> bundled_meta_schemas() noexcept;
const meta_schema* find_meta_schema(std::string_view uri) noexcept;
}
```

  The check is S1's `generated-meta-schemas`. It runs the generator into
  the build directory and compares each output, the notice included,
  byte for byte. Its own lane is `meta-schemas`.

- [ ] **Step 1: Fetch the files** (the controller's network step).

```bash
cd libs/json_schema/meta_schemas
for f in schema meta/core meta/applicator meta/unevaluated meta/validation meta/meta-data \
         meta/format-annotation meta/format-assertion meta/content; do
  mkdir -p "draft2020-12/$(dirname "$f")"
  curl -fsSL "https://json-schema.org/draft/2020-12/$f" -o "draft2020-12/$f.json"
done
for f in schema meta/core meta/applicator meta/validation meta/meta-data meta/format meta/content; do
  mkdir -p "draft2019-09/$(dirname "$f")"
  curl -fsSL "https://json-schema.org/draft/2019-09/$f" -o "draft2019-09/$f.json"
done
for d in draft-07 draft-06 draft-04; do
  mkdir -p "$d"
  curl -fsSL "http://json-schema.org/$d/schema" -o "$d/schema.json"
done
shasum -a 256 $(find . -name '*.json' | sort)
gh api repos/json-schema-org/json-schema-spec/contents/LICENSE --jq .content | base64 -d > ../LICENSE-JSON-SCHEMA.txt
```

  Expected: 19 files, each a JSON object whose `$id` (draft 4: `id`) is its
  URL.

  `meta_schemas/README.md` records, for each file, its URL, the date and
  its SHA-256. It says that the files are BSD-3-Clause, chosen of the
  json-schema-spec's two licences, with the notice in
  `LICENSE-JSON-SCHEMA.txt`.

  `.gitattributes` gains
  `include/webcpp/json_schema/detail/meta_schemas/*.hpp -text`, so that the
  byte comparison holds on a CRLF checkout.
- [ ] **Step 2: Write the generator, and the failing check.**

`meta_schemas/generate.py`:

```python
#!/usr/bin/env python3
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)
"""Writes json_schema's bundled meta-schema headers, one per file and an index, at the paths given.

Run by webcpp.generated as `generate.py <header>... <index>`, in test/; the headers are given in
the sorted order of the JSON files under meta_schemas/. Each file's bytes become raw string
literals of at most 8,000 bytes, cut on UTF-8 boundaries (MSVC refuses a literal of 16,380).
"""

import json
import pathlib
import sys

HERE = pathlib.Path(__file__).resolve().parent
NOTICE = """// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//
// Generated by meta_schemas/generate.py from {source}; never edited by hand. The text it holds
// is the JSON Schema authors', BSD-3-Clause (LICENSE-JSON-SCHEMA.txt).
"""
CHUNK = 8000


def identifier(path: pathlib.Path) -> str:
    stem = path.relative_to(HERE).with_suffix("").as_posix()
    return "".join(c if c.isalnum() else "_" for c in stem)


def uri_of(data: bytes) -> str:
    document = json.loads(data)
    return (document.get("$id") or document["id"]).rstrip("#")


def chunks_of(data: bytes) -> list:
    out, start = [], 0
    while start < len(data):
        end = min(start + CHUNK, len(data))
        while end < len(data) and (data[end] & 0xC0) == 0x80:
            end -= 1   # never inside a code point
        out.append(data[start:end].decode("utf-8"))
        start = end
    return out


def header(path: pathlib.Path) -> str:
    data = path.read_bytes()
    name = identifier(path)
    parts = chunks_of(data)
    if any(')json"' in part for part in parts):
        sys.exit(f"{path}: holds the raw literal's delimiter")
    literals = ",\n".join(f'    R"json({part})json"' for part in parts)
    guard = f"WEBCPP_JSON_SCHEMA_DETAIL_META_SCHEMAS_{name.upper()}_HPP"
    source = path.relative_to(HERE.parent).as_posix()
    return (NOTICE.format(source=source) + f"\n#ifndef {guard}\n#define {guard}\n\n"
            + "#include <array>\n#include <string_view>\n\n"
            + "namespace webcpp::json_schema::detail::meta_schemas {\n\n"
            + f"/** The bytes of {uri_of(data)}. */\n"
            + f"inline constexpr std::array<std::string_view, {len(parts)}> {name}{{\n"
            + literals + "\n};\n\n}  // namespace webcpp::json_schema::detail::meta_schemas\n\n"
            + f"#endif  // {guard}\n")


def index(rows: list) -> str:
    includes = "".join(f"#include <webcpp/json_schema/detail/meta_schemas/{n}.hpp>\n"
                       for _, n in rows)
    entries = "".join(f'        meta_schema{{.uri = "{u}", .chunks = meta_schemas::{n}}},\n'
                      for u, n in rows)
    guard = "WEBCPP_JSON_SCHEMA_DETAIL_META_SCHEMAS_INDEX_HPP"
    return (NOTICE.format(source="meta_schemas/") + f"\n#ifndef {guard}\n#define {guard}\n\n"
            + includes + "\n#include <array>\n#include <span>\n#include <string_view>\n\n"
            + "namespace webcpp::json_schema::detail {\n\n"
            + "/** A bundled meta-schema: its URI and its bytes. */\n"
            + "struct meta_schema {\n    /** Its URI, without fragment. */\n"
            + "    std::string_view uri;\n"
            + "    /** Its bytes, in pieces. */\n"
            + "    std::span<const std::string_view> chunks;\n};\n\n"
            + "/** Every bundled meta-schema. */\n"
            + "inline std::span<const meta_schema> bundled_meta_schemas() noexcept {\n"
            + f"    static constexpr std::array<meta_schema, {len(rows)}> all{{{{\n"
            + entries + "    }};\n    return all;\n}\n\n"
            + "/** The bundled meta-schema of `uri`, or nullptr. */\n"
            + "inline const meta_schema* find_meta_schema(std::string_view uri) noexcept {\n"
            + "    if (uri.ends_with('#')) {\n        uri.remove_suffix(1);\n    }\n"
            + "    for (const meta_schema& m : bundled_meta_schemas()) {\n"
            + "        if (m.uri == uri) {\n            return &m;\n        }\n    }\n"
            + "    return nullptr;\n}\n\n"
            + "}  // namespace webcpp::json_schema::detail\n\n" + f"#endif  // {guard}\n")


def main() -> None:
    files = sorted(HERE.rglob("*.json"))
    outputs = [pathlib.Path(a) for a in sys.argv[1:]]
    if len(outputs) != len(files) + 1:
        sys.exit(f"generate.py: {len(files)} meta-schemas and an index, "
                 f"{len(outputs)} outputs given")
    rows = []
    for path, out in zip(files, outputs):
        out.write_text(header(path), encoding="utf-8", newline="\n")
        rows.append((uri_of(path.read_bytes()), identifier(path)))
    outputs[-1].write_text(index(rows), encoding="utf-8", newline="\n")


if __name__ == "__main__":
    main()
```

  Append to `test/Jamfile` the generated outputs, one per line, in the
  sorted order of the JSON files, with the index last:

```
# The bundled meta-schemas: committed, written only by meta_schemas/generate.py,
# checked byte for byte by generated-meta-schemas, in its own lane.
.meta = ../include/webcpp/json_schema/detail/meta_schemas ;
webcpp.generated meta-schemas : ../meta_schemas/generate.py :
  : $(.meta)/draft_04_schema.hpp
    $(.meta)/draft_06_schema.hpp
    $(.meta)/draft_07_schema.hpp
    $(.meta)/draft2019_09_meta_applicator.hpp
    $(.meta)/draft2019_09_meta_content.hpp
    $(.meta)/draft2019_09_meta_core.hpp
    $(.meta)/draft2019_09_meta_format.hpp
    $(.meta)/draft2019_09_meta_meta_data.hpp
    $(.meta)/draft2019_09_meta_validation.hpp
    $(.meta)/draft2019_09_schema.hpp
    $(.meta)/draft2020_12_meta_applicator.hpp
    $(.meta)/draft2020_12_meta_content.hpp
    $(.meta)/draft2020_12_meta_core.hpp
    $(.meta)/draft2020_12_meta_format_annotation.hpp
    $(.meta)/draft2020_12_meta_format_assertion.hpp
    $(.meta)/draft2020_12_meta_meta_data.hpp
    $(.meta)/draft2020_12_meta_unevaluated.hpp
    $(.meta)/draft2020_12_meta_validation.hpp
    $(.meta)/draft2020_12_schema.hpp
    $(.meta)/index.hpp ;
webcpp.lane meta-schemas : generated-meta-schemas ;
webcpp.headers-alone json_schema : ../include : json_schema/detail/meta_schemas/*.hpp ;
```

  The order is Python's sorted order of the paths: `draft-0*` before
  `draft2*`, `meta/` before `schema.json`. Generate.py checks the count;
  the order is checked in Step 3.

  Run `$B2 libs/json_schema/test//generated-meta-schemas`. Expected: FAIL,
  naming each output as missing.
- [ ] **Step 3: Generate, and check.**

```bash
$B2 libs/json_schema/test//update-generated
$B2 -a libs/json_schema/test//generated-meta-schemas
.local/wasi-sdk/bin/clang-format --dry-run --Werror libs/json_schema/include/webcpp/json_schema/detail/meta_schemas/*.hpp
grep -l 'draft2020-12/schema"' libs/json_schema/include/webcpp/json_schema/detail/meta_schemas/draft2020_12_schema.hpp
```

  Expected:
  - the check exits 0;
  - `clang-format` prints nothing. If it wants another layout, the
    generator's text changes until it prints nothing;
  - `grep` names the file, so the outputs' order matches the JSON files'.

  Then change one byte of a generated header, see `generated-meta-schemas`
  fail naming it, and run `update-generated` again.
- [ ] **Step 4: Run the lanes.**

```bash
lanes libs/json_schema/test
lint_clean
```

  Expected: `lanes: green`, `lint: clean` (Pyright included).
- [ ] **Step 5: Commit.**

```bash
cd libs/json_schema
F="$(git ls-files --others --exclude-standard -- meta_schemas include/webcpp/json_schema/detail/meta_schemas \
| tr '\n' ' ') LICENSE-JSON-SCHEMA.txt README.md .gitattributes meta/include-boundaries.json test/Jamfile"
echo "$F" | tr ' ' '\n' | grep -c . # expect 19 json + README + generate.py + 20 headers + 5 = 46
git add $F
$G commit -m "json_schema: the official meta-schemas, bundled by a generator whose output the build checks" -- $F
```

  `git ls-files --others` lists exactly the new files of those two
  directories, each by its path. The count printed is checked against the
  file list above before the commit.

### Task E2.3: Resources, anchors, the registry, the resolver, and `$ref`

After E2.1 and E2.2 are merged.

**Files:**
- Create:
  - `include/webcpp/json_schema/detail/loader/resources.hpp`,
    `detail/loader/resolve.hpp`;
  - `test/references_test.cpp`.
- Modify:
  - `include/webcpp/json_schema/registry/registry.hpp` (its members);
  - `include/webcpp/json_schema/detail/loader/walk.hpp`;
  - `include/webcpp/json_schema/detail/compiler/opcode.hpp` (appends
    `jump = 28`), `detail/compiler/compile.hpp`,
    `detail/compiler/explain.hpp`;
  - `include/webcpp/json_schema/detail/vm/machine.hpp`;
  - `test/compile_errors_test.cpp`, `test/suite/runner.hpp`,
    `test/known_failures/draft2020-12.json`, `test/Jamfile`.

**Interfaces:**
- Produces:
  - spec 4.7's `basic_registry`: `add` of a value and of a parsed document,
    each copied with `deep_copy`; `contains`; `get_allocator`;
  - spec 8.2's lookup order;
  - `opcode::jump` (`a` the target block), which pushes the target's block
    on the same instance with the evaluation token `$ref`. When the target
    fails, it records `reference` (kind 39) after the target's errors.
- The rules:
  - Each `$id` is resolved against the enclosing base with the URL policy
    (spec 8.1). An empty fragment is dropped; a non-empty one is
    `invalid_uri`. `compile_options::base_uri` is the root's base, else
    `urn:webcpp:root`.
  - **A keyword's target is set before its node is defined.**
    `basic_document::define` runs once per node (spec 5.4), so the walker
    keeps, in its own allocator, the keyword buffer of each node that holds
    an open `ref`. It defines that node only after the resolution passes
    have set every target.
  - References are resolved in passes, until none is open:
    - the absolute URI without fragment is looked up in spec 8.2's order:
      loaded resources, then the registry, then the bundled meta-schemas,
      then the resolver, called once per URI;
    - an empty fragment names the resource's root;
    - a fragment starting with `/` is a percent-decoded JSON Pointer from
      the resource's root value: it finds the node there, or reserves one
      for the value there, as a node of that resource, when an unknown
      keyword hid it (spec 6.2), registering no `$id` inside;
    - any other fragment is a plain anchor.
  - A pointer that crosses into an embedded resource follows 2020-12 core
    9.2.1.
  - A document from the registry or the resolver is walked as a resource of
    its own: its dialect is its `$schema`, else the referencing resource's.
  - Whatever is used is copied into the IR (invariant 2).
  - The runner registers every file of the suite's `remotes/`, read from
    `WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE "/remotes"`, under
    `http://localhost:1234/<relative path>`.

- [ ] **Step 1: Write the failing tests.** `test/references_test.cpp`:

```cpp
#include <webcpp/json_schema.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/json/parse.hpp>

#include <string>

namespace js = webcpp::json_schema;

namespace {

const js::validator_for<js::draft2020_12> validator;

bool valid(const js::schema& s, std::string_view instance) { return s.is_valid(boost::json::parse(instance)); }

void local_references() {
    const auto s = validator.compile(boost::json::parse(R"({
        "$defs": {"positive": {"minimum": 0}, "a/b": {"type": "string"}, "named": {"$anchor": "n", "maxLength": 1}},
        "properties": {
            "p": {"$ref": "#/$defs/positive"},
            "q": {"$ref": "#/$defs/a~1b"},
            "r": {"$ref": "#n"},
            "s": {"$ref": "#/$hidden"}
        },
        "$hidden": {"type": "integer"}
    })"));
    if (!BOOST_TEST(s.has_value())) {
        return;
    }
    BOOST_TEST(valid(*s, R"({"p": 1, "q": "x", "r": "y", "s": 2})"));
    BOOST_TEST(!valid(*s, R"({"p": -1})"));
    BOOST_TEST(!valid(*s, R"({"r": "yy"})"));
    BOOST_TEST(!valid(*s, R"({"s": "x"})"));   // a pointer into an unknown keyword loads what it reaches
    BOOST_TEST(s->ir().check().has_value());
}

void a_failing_target_records_reference() {
    const auto s = validator.compile(boost::json::parse(R"({"$defs": {"i": {"type": "integer"}}, "$ref": "#/$defs/i"})"));
    const auto outcome = s->validate(boost::json::parse("\"x\""));
    BOOST_TEST_EQ(outcome->errors().size(), 2U);
    BOOST_TEST(outcome->errors()[1].kind() == js::error_kind::reference);
    BOOST_TEST_EQ(outcome->errors()[0].keyword_location(), "/$ref/type");
}

void recursion_is_a_loop_of_the_plan() {
    const auto tree = validator.compile(boost::json::parse(R"({
        "type": "object",
        "properties": {"children": {"type": "array", "items": {"$ref": "#"}}}
    })"));
    BOOST_TEST(valid(*tree, R"({"children": [{"children": []}, {"children": [{}]}]})"));
    BOOST_TEST(!valid(*tree, R"({"children": [1]})"));
}

void bases_resources_and_the_registry() {
    js::registry registry;
    BOOST_TEST(registry.add("http://example.com/int.json", boost::json::parse(R"({"type": "integer"})")).has_value());
    BOOST_TEST(registry.add("http://example.com/str.json", *js::parse(R"({"type": "string"})")).has_value());
    const js::compile_options options{.registry = &registry, .base_uri = "http://example.com/root.json"};
    const auto s = validator.compile(boost::json::parse(R"({
        "properties": {
            "a": {"$ref": "int.json"},
            "b": {"$ref": "item.json"},
            "c": {"$ref": "str.json"}
        },
        "$defs": {"item": {"$id": "item.json#", "type": "boolean"}}
    })"), options);
    if (!BOOST_TEST(s.has_value())) {
        return;
    }
    BOOST_TEST(valid(*s, R"({"a": 1, "b": true, "c": "x"})"));
    BOOST_TEST(!valid(*s, R"({"a": 1.5})"));
    BOOST_TEST(!valid(*s, R"({"b": 2})"));
}

void the_lookup_order_and_the_resolver() {
    int calls = 0;
    auto resolve = [&](std::string_view uri, const boost::json::storage_ptr& storage)
        -> boost::system::result<boost::json::value> {
        ++calls;
        if (uri == "urn:remote" || uri == "urn:shadowed") {
            return boost::json::parse(R"({"minimum": 10})", storage);
        }
        return boost::system::errc::make_error_code(boost::system::errc::no_such_file_or_directory);
    };
    js::registry registry;
    (void)registry.add("urn:shadowed", boost::json::parse(R"({"maximum": 0})"));
    const js::compile_options options{.registry = &registry, .resolver = js::resolver_ref(resolve)};
    const auto s = validator.compile(boost::json::parse(
        R"({"allOf": [{"$ref": "urn:remote"}, {"$ref": "urn:remote#"}]})"), options);
    BOOST_TEST(s.has_value());
    BOOST_TEST_EQ(calls, 1);
    BOOST_TEST(!valid(*s, "5"));
    const auto shadowed = validator.compile(boost::json::parse(R"({"$ref": "urn:shadowed"})"), options);
    BOOST_TEST(!valid(*shadowed, "5"));   // the registry before the resolver
    BOOST_TEST_EQ(calls, 1);
    const auto failed = validator.compile(boost::json::parse(R"({"$ref": "urn:missing"})"), options);
    BOOST_TEST(failed.error().code == js::errc::resolver_failed);
    BOOST_TEST_EQ(std::string(failed.error().what), "urn:missing");
}

}  // namespace

int main() {
    local_references();
    a_failing_target_records_reference();
    recursion_is_a_loop_of_the_plan();
    bases_resources_and_the_registry();
    the_lookup_order_and_the_resolver();
    return boost::report_errors();
}
```

  Append to `compile_errors_test.cpp`, called from `main`:

```cpp
void reference_errors() {
    const auto code = [](std::string_view schema) {
        return validator.compile(boost::json::parse(schema), {.validate_schema = false}).error().code;
    };
    BOOST_TEST(code(R"({"$id": "http://a/b#frag"})") == js::errc::invalid_uri);
    BOOST_TEST(code(R"({"$ref": "http://a b/"})") == js::errc::invalid_uri);
    BOOST_TEST(code(R"({"$ref": "urn:nowhere"})") == js::errc::unresolvable_reference);
    BOOST_TEST(code(R"({"$ref": "#/$defs/missing"})") == js::errc::invalid_json_pointer);
    BOOST_TEST(code(R"({"$ref": "#/~2"})") == js::errc::invalid_json_pointer);
    BOOST_TEST(code(R"({"$defs": {"a": {"$id": "urn:x"}, "b": {"$id": "urn:x"}}})") == js::errc::duplicate_resource);
    BOOST_TEST(code(R"({"$defs": {"a": {"$anchor": "x"}, "b": {"$anchor": "x"}}})") == js::errc::duplicate_anchor);
}
```

  Append to `test/Jamfile`: `webcpp.run references : references_test.cpp ;`.

  The runner's registry, in `runner.hpp`'s `run`:

```cpp
template <class Validator>
webcpp::json_schema::basic_registry<typename Validator::traits_type> remotes(std::string_view root) {
    webcpp::json_schema::basic_registry<typename Validator::traits_type> registry;
    const std::filesystem::path base = std::string(root) + "/remotes";
    for (const auto& entry : std::filesystem::recursive_directory_iterator(base)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            const std::string uri = "http://localhost:1234/" +
                                    std::filesystem::relative(entry.path(), base).generic_string();
            (void)registry.add(uri, boost::json::parse(read_file(entry.path())));
        }
    }
    return registry;
}
```

  `run` builds it once and `run_case` sets `compile.registry = &registry`.
- [ ] **Step 2: Run them and watch them fail.**
  `$B2 libs/json_schema/test//references libs/json_schema/test//compile_errors`.
  Expected: FAIL, `unresolvable_reference` for every `$ref`.
- [ ] **Step 3: Implement.** The resolution loop:

```cpp
template <class Traits>
result<void> walker<Traits>::resolve_references() {
    while (!open_references_.empty()) {
        const open_reference ref = open_references_.back();
        open_references_.pop_back();
        boost::static_strings::static_string<url_type::capacity> absolute;
        if (!url_type::resolve(base_of(ref.resource), ref.written, absolute)) {
            return make_diagnostic(errc::invalid_uri, ref.where, ref.written);
        }
        const auto [document_uri, fragment] = split_fragment(absolute);
        auto resource = find_or_load(document_uri);   // spec 8.2's four places; walks a new one
        if (!resource) {
            return resource.error();
        }
        auto target = target_of(*resource, fragment);   // root, pointer (reserving hidden values), or anchor
        if (!target) {
            return target.error();
        }
        set_target(ref.keyword_buffer, ref.keyword, *target);   // the walker's buffer, not the document
        if (auto walked = walk_pending(); !walked) {             // a document or a hidden schema just reached
            return walked.error();
        }
    }
    return define_deferred();   // every node held back for its references, now complete
}
```

  `target_of` returns `invalid_json_pointer` for a pointer fragment that
  is invalid or finds nothing, and `unresolvable_reference` for an unknown
  anchor. The compiler lowers `ref` to `jump`, and `explain` writes `jump
  <block>`.
- [ ] **Step 4: Run them and watch them pass**, then record the list:

```bash
lanes libs/json_schema/test
record_known draft2020-12
lanes libs/json_schema/test
lint_clean
```

  Expected:
  - the first run fails with `listed, passes:` for each reference test now
    passing;
  - the record drops the `E2: references` entries of `ref`, `refRemote`,
    `anchor` and `defs`;
  - `lanes: green`, `lint: clean`.
- [ ] **Step 5: Commit.**

```bash
cd libs/json_schema
F="include/webcpp/json_schema/registry/registry.hpp include/webcpp/json_schema/detail/loader/resources.hpp \
include/webcpp/json_schema/detail/loader/resolve.hpp include/webcpp/json_schema/detail/loader/walk.hpp \
include/webcpp/json_schema/detail/compiler/opcode.hpp include/webcpp/json_schema/detail/compiler/compile.hpp \
include/webcpp/json_schema/detail/compiler/explain.hpp include/webcpp/json_schema/detail/vm/machine.hpp \
test/Jamfile test/references_test.cpp test/compile_errors_test.cpp test/suite/runner.hpp \
test/known_failures/draft2020-12.json"
git add $F
$G commit -m "json_schema: resources, anchors, the registry, the resolver, and \$ref as a jump" -- $F
```

### Task E2.4: `$dynamicRef`, the dynamic scope, and cycles refused

After E2.3.

**Files:**
- Create: `include/webcpp/json_schema/detail/compiler/cycles.hpp`,
  `test/dynamic_test.cpp`.
- Modify:
  - `detail/loader/walk.hpp`, `detail/loader/resolve.hpp`
    (`dynamic_reference`);
  - `detail/compiler/opcode.hpp` (appends `dynamic_jump = 29`,
    `enter_resource = 30`, `leave_resource = 31`), `compile.hpp`,
    `explain.hpp`;
  - `detail/vm/machine.hpp`;
  - `test/compile_errors_test.cpp`, `test/hostile_test.cpp`,
    `test/known_failures/draft2020-12.json`, `test/Jamfile`.

**Interfaces:**
- Produces:
  - The loader builds `dynamic_reference{.initial_target, .uri, .anchor,
    .form = dynamic, .bookended}`. `bookended` is true when the fragment is
    a plain name and the initial target carries a `$dynamicAnchor` of that
    name.
  - The compiler emits:
    - `jump` for a reference that is not bookended;
    - `dynamic_jump` (`a` the anchor's string id, `b` the initial block)
      for one that is;
    - and, only when the plan holds a `dynamic_jump`, `enter_resource` and
      `leave_resource` (`a` the resource id) around every block whose node
      roots a resource, and around every jump into a node of another
      resource.
  - The machine keeps the dynamic scope as an `inline_stack<ir::resource_id,
    16>` in the validation's resource. `dynamic_jump` takes the outermost
    resource on it that holds a dynamic anchor of the name, and jumps to
    that anchor's target; with none, it jumps to the initial target (2020-12
    core 8.2.3.2). A `form = recursive` reference is refused with
    `invalid_ir` naming it, until 2019-09's runtime arrives.
  - `template <class Traits> std::optional<std::pmr::vector<ir::keyword_id>>
    detail::find_cycle(const ir::basic_document<Traits>&,
    std::pmr::memory_resource&)` over the in-place edges of spec 7.5:
    - a cycle is `reference_cycle`;
    - its `where` is the first keyword's location;
    - its `what` is the locations around the cycle joined by ` -> `, cut
      with `...`.

- [ ] **Step 1: Write the failing tests.** `test/dynamic_test.cpp`:

```cpp
#include <webcpp/json_schema.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/json/parse.hpp>

namespace js = webcpp::json_schema;

namespace {

const js::validator_for<js::draft2020_12> validator;

void a_dynamic_reference_finds_the_outermost_anchor() {
    // dynamicRef.json, "A $dynamicRef resolves to the first $dynamicAnchor still in scope"
    const auto s = validator.compile(boost::json::parse(R"({
        "$id": "https://test.json-schema.org/typical-dynamic-resolution/root",
        "$ref": "list",
        "$defs": {
            "foo": {"$dynamicAnchor": "items", "type": "string"},
            "list": {
                "$id": "list",
                "type": "array",
                "items": {"$dynamicRef": "#items"},
                "$defs": {"items": {"$comment": "default", "$dynamicAnchor": "items"}}
            }
        }
    })"));
    if (!BOOST_TEST(s.has_value())) {
        return;
    }
    BOOST_TEST(s->is_valid(boost::json::parse(R"(["foo", "bar"])")));
    BOOST_TEST(!s->is_valid(boost::json::parse(R"(["foo", 42])")));
}

void without_a_bookend_it_is_a_plain_jump() {
    const auto s = validator.compile(boost::json::parse(R"({"$defs": {"a": {"type": "integer"}}, "$dynamicRef": "#/$defs/a"})"));
    BOOST_TEST(s->is_valid(boost::json::parse("1")));
    BOOST_TEST(!s->is_valid(boost::json::parse("\"x\"")));
}

}  // namespace

int main() {
    a_dynamic_reference_finds_the_outermost_anchor();
    without_a_bookend_it_is_a_plain_jump();
    return boost::report_errors();
}
```

  Append to `compile_errors_test.cpp`, called from `main`:

```cpp
void cycles() {
    const auto self = validator.compile(boost::json::parse(R"({"$ref": "#"})"));
    BOOST_TEST(self.error().code == js::errc::reference_cycle);
    const auto pair = validator.compile(boost::json::parse(R"({
        "$defs": {"a": {"allOf": [{"$ref": "#/$defs/b"}]}, "b": {"anyOf": [{"$ref": "#/$defs/a"}]}},
        "$ref": "#/$defs/a"
    })"));
    BOOST_TEST(pair.error().code == js::errc::reference_cycle);
    BOOST_TEST(std::string(pair.error().what).find(" -> ") != std::string::npos);
    // infinite-loop-detection.json: the same location twice is no cycle
    const auto twice = validator.compile(boost::json::parse(R"({
        "$defs": {"int": {"type": "integer"}},
        "allOf": [{"properties": {"foo": {"$ref": "#/$defs/int"}}}, {"additionalProperties": {"$ref": "#/$defs/int"}}]
    })"));
    BOOST_TEST(twice.has_value());
}
```

  Append to `hostile_test.cpp`, called from `main`. The chain is built in
  code, and its expected outcome is that it compiles and validates, its
  in-place jumps on the machine's heap stack:

```cpp
void a_long_chain_of_references_compiles_and_runs() {
    boost::json::monotonic_resource storage;
    boost::json::object defs(&storage);
    for (int i = 0; i < 100000; ++i) {
        defs["d" + std::to_string(i)] = boost::json::object{{"$ref", "#/$defs/d" + std::to_string(i + 1)}};
    }
    defs["d100000"] = boost::json::object{{"type", "integer"}};
    boost::json::object schema(&storage);
    schema["$defs"] = std::move(defs);
    schema["$ref"] = "#/$defs/d0";
    const auto chain = validator.compile(schema, {.validate_schema = false});
    if (!BOOST_TEST(chain.has_value())) {
        return;
    }
    BOOST_TEST(chain->is_valid(boost::json::value(1)));
    BOOST_TEST(!chain->is_valid(boost::json::value("x")));
}

void a_recursive_schema_meets_a_deep_instance() {
    boost::json::monotonic_resource storage;
    boost::json::value deep = boost::json::array(&storage);
    for (int i = 0; i < 100000; ++i) {
        boost::json::array outer(&storage);
        outer.push_back(std::move(deep));
        deep = std::move(outer);
    }
    const auto recursive = validator.compile(boost::json::parse(R"({"items": {"$ref": "#"}})"));
    const auto outcome = recursive->validate(deep);
    BOOST_TEST(outcome->errors()[0].kind() == js::error_kind::depth_limit);
}
```

  Append `webcpp.run dynamic : dynamic_test.cpp ;` to `test/Jamfile`.
- [ ] **Step 2: Run them and watch them fail.**
  `$B2 libs/json_schema/test//dynamic libs/json_schema/test//compile_errors`.
  Expected: FAIL. `$dynamicRef` is unresolvable, and `{"$ref": "#"}`
  compiles: it consumes nothing, so without the check the machine would
  push frames until memory ran out.
- [ ] **Step 3: Implement.** The cycle search colours the nodes,
  iteratively:

```cpp
template <class Traits>
std::optional<std::pmr::vector<ir::keyword_id>> find_cycle(const ir::basic_document<Traits>& d,
                                                           std::pmr::memory_resource& scratch) {
    enum class colour : std::uint8_t { white, grey, black };
    std::pmr::vector<colour> state(d.size(), colour::white, &scratch);
    struct step { ir::node_id node; std::uint32_t next_edge; ir::keyword_id via; };
    std::pmr::vector<step> path(&scratch);
    for (std::uint32_t start = 0; start < d.size(); ++start) {
        if (state[start] != colour::white) {
            continue;
        }
        path.push_back({.node = ir::node_id{start}, .next_edge = 0, .via = ir::keyword_id{0}});
        state[start] = colour::grey;
        while (!path.empty()) {
            step& top = path.back();
            const auto edge = in_place_edge(d, top.node, top.next_edge++);
            if (!edge) {
                state[static_cast<std::uint32_t>(top.node)] = colour::black;
                path.pop_back();
                continue;
            }
            const auto target = static_cast<std::uint32_t>(edge->target);
            if (state[target] == colour::grey) {
                return cycle_from(path, edge->target, edge->keyword, scratch);
            }
            if (state[target] == colour::white) {
                state[target] = colour::grey;
                path.push_back({.node = edge->target, .next_edge = 0, .via = edge->keyword});
            }
        }
    }
    return std::nullopt;
}
```

  `in_place_edge(d, node, i)` yields the node's i-th in-place edge:
  - the subschemas of `all_of`, `any_of`, `one_of`, `not_`, `if_`, `then_`,
    `else_` and `dependent_schemas`;
  - the target of `ref`;
  - every candidate of a `dynamic_ref`: the initial target and every
    dynamic anchor of the same name.
- [ ] **Step 4: Run them and watch them pass**, recording the list:

```bash
record_known draft2020-12
lanes libs/json_schema/test
lint_clean
```

  Expected: the `dynamicRef` and `infinite-loop-detection` entries gone,
  `lanes: green`, `lint: clean`.
- [ ] **Step 5: Commit.**

```bash
cd libs/json_schema
F="include/webcpp/json_schema/detail/compiler/cycles.hpp include/webcpp/json_schema/detail/compiler/opcode.hpp \
include/webcpp/json_schema/detail/compiler/compile.hpp include/webcpp/json_schema/detail/compiler/explain.hpp \
include/webcpp/json_schema/detail/loader/walk.hpp include/webcpp/json_schema/detail/loader/resolve.hpp \
include/webcpp/json_schema/detail/vm/machine.hpp test/Jamfile test/dynamic_test.cpp test/compile_errors_test.cpp \
test/hostile_test.cpp test/known_failures/draft2020-12.json"
git add $F
$G commit -m "json_schema: \$dynamicRef in its dynamic scope, and in-place reference cycles refused" -- $F
```

### Task E2.5: Meta-schema validation at compile, and the registry's cache

After E2.4 (and E2.2 merged).

**Files:**
- Create: `include/webcpp/json_schema/detail/loader/meta_check.hpp`,
  `test/meta_validation_test.cpp`.
- Modify:
  - `include/webcpp/json_schema/validator/validator.hpp` (the pipeline);
  - `include/webcpp/json_schema/registry/registry.hpp` (the cache);
  - `include/webcpp/json_schema/detail/loader/walk.hpp` (the walker
    defers its first shape fault);
  - `test/compile_errors_test.cpp`, `test/Jamfile`.

**Interfaces:**
- Produces the pipeline of spec 6.1 step 5 and spec 8.4, written out:
  1. `compile(value)` with `validate_schema` first picks the root's dialect
     (D3) and finds its meta-schema: the registry's cache, else the bundled
     one or a custom one, compiled once per compile call with
     `validate_schema = false`.
  2. It validates the root value against it, before the walk lowers
     anything. A violation is `meta_schema_violation`: its `where` is the
     first error's instance location in the schema, and its `what` is that
     error's keyword.
  3. A document reached later, from the registry or the resolver, is
     checked against its own meta-schema before it is walked.
  4. Only then does the walk run. Its shape faults are reported as before.
     With the check on, a fault the meta-schema should have caught is still
     reported, by the walk.
  5. `load()` never checks: it reports shape faults alone.
- `basic_registry` keeps each compiled meta-schema it was given for later
  compiles, in its own allocator. It is the user's object, never written by
  two threads at once (spec 8.4).
- Each document is checked against its root's meta-schema. An embedded
  resource that names another dialect is checked by that meta-schema too,
  through the 2020-12 meta-schema's `$dynamicRef`, the spec's own
  mechanism.

- [ ] **Step 1: Write the failing test** `test/meta_validation_test.cpp`:

```cpp
#include <webcpp/json_schema.hpp>

#include "counting_resource.hpp"

#include <boost/core/lightweight_test.hpp>
#include <boost/json/parse.hpp>

#include <string>

namespace js = webcpp::json_schema;

namespace {

const js::validator_for<js::draft2020_12> validator;

void a_schema_is_checked_against_its_meta_schema() {
    const auto bad = validator.compile(boost::json::parse(R"({"properties": {"a": {"type": 3}}})"));
    if (!BOOST_TEST(bad.has_error())) {
        return;
    }
    BOOST_TEST(bad.error().code == js::errc::meta_schema_violation);
    BOOST_TEST_EQ(std::string(bad.error().where), "/properties/a/type");
    BOOST_TEST(!bad.error().what.empty());
    const auto off = validator.compile(boost::json::parse(R"({"properties": {"a": {"type": 3}}})"),
                                       {.validate_schema = false});
    BOOST_TEST(off.error().code == js::errc::invalid_keyword_value);
    BOOST_TEST(validator.load(boost::json::parse(R"({"type": 3})")).error().code == js::errc::invalid_keyword_value);
}

void the_registry_keeps_compiled_meta_schemas() {
    webcpp::test::counting_resource memory;
    js::registry registry{std::pmr::polymorphic_allocator<std::byte>(&memory)};
    const js::compile_options options{.registry = &registry};
    (void)validator.compile(boost::json::parse(R"({"type": "string"})"), options);
    const std::size_t after_first = memory.allocations;
    BOOST_TEST(after_first > 0);
    (void)validator.compile(boost::json::parse(R"({"type": "integer"})"), options);
    BOOST_TEST_EQ(memory.allocations, after_first);   // the cache served the second compile
}

}  // namespace

int main() {
    a_schema_is_checked_against_its_meta_schema();
    the_registry_keeps_compiled_meta_schemas();
    return boost::report_errors();
}
```

  Append to `compile_errors_test.cpp` the order of the two checks:

```cpp
void the_meta_check_comes_first() {
    BOOST_TEST(validator.compile(boost::json::parse(R"({"minLength": -1})")).error().code ==
               js::errc::meta_schema_violation);
    BOOST_TEST(validator.compile(boost::json::parse(R"({"minLength": -1})"), {.validate_schema = false})
                   .error().code == js::errc::invalid_keyword_value);
}
```

  Append `webcpp.run meta_validation : meta_validation_test.cpp ;` to
  `test/Jamfile`.
- [ ] **Step 2: Run them and watch them fail,** and run every test to find
  any other E1 test that depends on the old order:

```bash
$B2 libs/json_schema/test
```

  Expected: FAIL in `meta_validation` and `compile_errors` only. Any other
  failing test is fixed by setting `validate_schema = false` where the
  test means the loader's own check, with a comment saying so.
- [ ] **Step 3: Implement** `meta_check.hpp`: `check_against_meta(const
  boost::json::value&, dialect or URI, options, allocator) -> result<void>`.
  `compile` calls it before `load`, and the walker calls it for each
  document it fetches.
- [ ] **Step 4: Run the lanes.**

```bash
lanes libs/json_schema/test
lint_clean
```

  Expected: `lanes: green`, `lint: clean`.
- [ ] **Step 5: Commit.**

```bash
cd libs/json_schema
F="include/webcpp/json_schema/detail/loader/meta_check.hpp include/webcpp/json_schema/detail/loader/walk.hpp \
include/webcpp/json_schema/validator/validator.hpp include/webcpp/json_schema/registry/registry.hpp \
test/Jamfile test/meta_validation_test.cpp test/compile_errors_test.cpp"
git add $F
$G commit -m "json_schema: schemas checked against their meta-schemas first, the registry caching them" -- $F
```

### Task E2.6: `$vocabulary` and custom meta-schemas

After E2.5.

**Files:**
- Modify: `include/webcpp/json_schema/detail/loader/walk.hpp`,
  `detail/loader/meta_check.hpp`, `test/meta_validation_test.cpp`,
  `test/known_failures/draft2020-12.json`.

**Interfaces:**
- Produces spec 8.5, for a `$schema` that names a non-official meta-schema
  reached through the registry or the resolver:
  - the loader reads that meta-schema's `$vocabulary`, and takes its
    `$schema` as the resource's dialect;
  - a required vocabulary unknown to the validator, official or user, is
    `unknown_vocabulary` naming its URI; an optional unknown one is
    ignored;
  - a known vocabulary left out switches its keywords to `unknown` for
    every resource using that meta-schema;
  - `format-assertion` sets `format_payload::assertion`;
  - vocabularies are not inherited through `$ref`.

- [ ] **Step 1: Write the failing test.** Append to `meta_validation_test.cpp`,
  called from `main`:

```cpp
void vocabularies_switch_keywords() {
    js::registry registry;
    (void)registry.add("http://example.com/no-validation", boost::json::parse(R"({
        "$schema": "https://json-schema.org/draft/2020-12/schema",
        "$id": "http://example.com/no-validation",
        "$vocabulary": {
            "https://json-schema.org/draft/2020-12/vocab/core": true,
            "https://json-schema.org/draft/2020-12/vocab/applicator": true
        },
        "$dynamicAnchor": "meta"
    })"));
    const js::compile_options options{.registry = &registry};
    const auto s = validator.compile(boost::json::parse(R"({"$schema": "http://example.com/no-validation", "minimum": 10})"),
                                     options);
    if (!BOOST_TEST(s.has_value())) {
        return;
    }
    BOOST_TEST(s->is_valid(boost::json::parse("1")));   // minimum is unknown here
    (void)registry.add("http://example.com/strange", boost::json::parse(R"({
        "$schema": "https://json-schema.org/draft/2020-12/schema",
        "$id": "http://example.com/strange",
        "$vocabulary": {"urn:unknown:vocabulary": true}
    })"));
    const auto refused = validator.compile(boost::json::parse(R"({"$schema": "http://example.com/strange"})"), options);
    BOOST_TEST(refused.error().code == js::errc::unknown_vocabulary);
    BOOST_TEST_EQ(std::string(refused.error().what), "urn:unknown:vocabulary");
    (void)registry.add("http://example.com/optional", boost::json::parse(R"({
        "$schema": "https://json-schema.org/draft/2020-12/schema",
        "$id": "http://example.com/optional",
        "$vocabulary": {"https://json-schema.org/draft/2020-12/vocab/core": true, "urn:unknown:vocabulary": false}
    })"));
    BOOST_TEST(validator.compile(boost::json::parse(R"({"$schema": "http://example.com/optional"})"), options)
                   .has_value());
}
```

  Run `$B2 libs/json_schema/test//meta_validation`. Expected: FAIL,
  `minimum` loaded as known and the custom `$schema` refused as
  `unknown_dialect`.
- [ ] **Step 2: Implement** as the Interfaces say.
- [ ] **Step 3: Record the list, and run the lanes.**

```bash
record_known draft2020-12
lanes libs/json_schema/test
lint_clean
```

  Expected: `vocabulary` and `optional/no-schema` gone from the list,
  `lanes: green`, `lint: clean`.
- [ ] **Step 4: Commit.**

```bash
cd libs/json_schema
F="include/webcpp/json_schema/detail/loader/walk.hpp include/webcpp/json_schema/detail/loader/meta_check.hpp \
test/meta_validation_test.cpp test/known_failures/draft2020-12.json"
git add $F
$G commit -m "json_schema: \$vocabulary and custom meta-schemas" -- $F
```

### Task E2.7: Compatibility keywords, and what an unknown keyword becomes

After E2.6.

**Files:**
- Modify: `include/webcpp/json_schema/detail/loader/walk.hpp`,
  `detail/loader/readers.hpp`, `test/loader_draft2020_12_test.cpp`,
  `test/meta_validation_test.cpp`,
  `test/known_failures/draft2020-12.json`.

**Interfaces:**
- **Compatibility keywords** (spec 6.2). With `compatibility_keywords`:
  - `definitions` loads as `defs`;
  - `dependencies` loads each member as a `dependent_required` or a
    `dependent_schemas` entry, both named `dependencies`.

  Without it, both are `unknown`.
- **An unknown keyword's kind carries its meaning** (decision PD2 of section
  "Decisions", answering the E2 to E5 review's I4). The IR forbids reading
  the dialect for semantics ([ir-3]), so the loader resolves
  `unknown_keywords` into the kind:
  - **`dialect_default`:**
    - in 2019-09 and 2020-12, the kind is `unknown`, which annotates;
    - in drafts 4 to 7, it is `ignored`, which is inert;
    - in OpenAPI 3.0 (and v1 later), a keyword other than `x-` is
      `unknown_keyword`, and an `x-` one is `unknown`.
  - **`annotate`:** `unknown`. **`ignore`:** `ignored`. **`reject`:**
    `unknown_keyword` for every keyword but an `x-` one.
  - The emitter writes an `ignored` keyword into its own dialect. Into
    another, it writes it only when its name is no keyword of the source
    dialect (an inert unknown keyword, not a `$ref` sibling), which the
    emitter, allowed to read the dialect, can tell.

- [ ] **Step 1: Write the failing tests.** Append to
  `loader_draft2020_12_test.cpp`, called from `main`:

```cpp
void unknown_keywords_follow_the_option() {
    const auto annotate = load(R"({"foo": 1})");
    BOOST_TEST(find(*annotate, k::unknown) != nullptr);
    const auto ignore = validator.load(boost::json::parse(R"({"foo": 1})"), {.unknown = js::unknown_keywords::ignore});
    BOOST_TEST(find(*ignore, k::ignored) != nullptr);
    const auto reject = validator.load(boost::json::parse(R"({"foo": 1})"), {.unknown = js::unknown_keywords::reject});
    BOOST_TEST(reject.error().code == js::errc::unknown_keyword);
    BOOST_TEST_EQ(std::string(reject.error().what), "foo");
    BOOST_TEST(validator.load(boost::json::parse(R"({"x-foo": 1})"), {.unknown = js::unknown_keywords::reject})
                   .has_value());
}
```

  Append to `meta_validation_test.cpp`, called from `main` (the E2 to E5
  review's I7: a test that fails for the bug it targets):

```cpp
void compatibility_keywords() {
    const auto s = validator.compile(boost::json::parse(R"({
        "dependencies": {"a": ["b"], "c": {"required": ["d"]}},
        "definitions": {"x": {"type": "object"}},
        "$ref": "#/definitions/x"
    })"));
    if (!BOOST_TEST(s.has_value())) {
        return;
    }
    BOOST_TEST(s->is_valid(boost::json::parse(R"({"a": 1, "b": 2})")));
    BOOST_TEST(!s->is_valid(boost::json::parse(R"({"a": 1})")));
    BOOST_TEST(!s->is_valid(boost::json::parse(R"({"c": 1})")));
    BOOST_TEST(s->is_valid(boost::json::parse("{}")));
    const auto off = validator.compile(boost::json::parse(R"({"dependencies": {"a": ["b"]}})"),
                                       {.compatibility_keywords = false});
    BOOST_TEST(off->is_valid(boost::json::parse(R"({"a": 1})")));
}
```

  Run `$B2 libs/json_schema/test//loader_draft2020_12
  libs/json_schema/test//meta_validation`. Expected: FAIL, `ignore` and
  `reject` without effect, and `dependencies` unknown.
- [ ] **Step 2: Implement** as the Interfaces say.
- [ ] **Step 3: Record the list, and run the lanes.**

```bash
record_known draft2020-12
lanes libs/json_schema/test
lint_clean
```

  Expected: `optional/dependencies-compatibility`,
  `optional/unknownKeyword`, `optional/refOfUnknownKeyword` and
  `optional/id` gone, `lanes: green`, `lint: clean`.
- [ ] **Step 4: Commit.**

```bash
cd libs/json_schema
F="include/webcpp/json_schema/detail/loader/walk.hpp include/webcpp/json_schema/detail/loader/readers.hpp \
test/loader_draft2020_12_test.cpp test/meta_validation_test.cpp test/known_failures/draft2020-12.json"
git add $F
$G commit -m "json_schema: compatibility keywords, and an unknown keyword's kind decided at load" -- $F
```

### Task E2.8: Annotations

After E2.4, in `task_worktree 2 annotations`, alongside E2.5 to E2.7.

**Files:**
- Create: `test/annotations_test.cpp`.
- Modify:
  - `include/webcpp/json_schema/detail/compiler/opcode.hpp` (appends
    `annotate = 32`), `compile.hpp`, `explain.hpp`;
  - `include/webcpp/json_schema/detail/vm/machine.hpp`;
  - `test/Jamfile`.

**Interfaces:**
- Produces `annotate` (`a` the keyword; `flags` constant or computed). It
  is emitted for:
  - the meta-data keywords, `unknown`, `format` (its name), and the
    `content_*` keywords;
  - the applicators whose annotation 2020-12 core 10.3 defines:
    - `properties`, `patternProperties` and `additionalProperties`: the
      names they evaluated;
    - `prefixItems`: the largest index;
    - `items`: `true`;
    - `contains`, when `annotates_items`: the matching indices.
- Computed values are built in the record's `storage()`, frozen since E1.
  `outcome::annotations()` returns them.
- With `collect::errors`, the collecting sink ignores `annotate`, as E1.5
  wrote it.

- [ ] **Step 1: Write the failing test** `test/annotations_test.cpp`. It runs
  the suite's `annotations/tests/*.json` (spec 16.2):
  - it keeps the cases whose `compatibility` admits 2020-12;
  - it validates each test's instance with
    `collect::errors_and_annotations`;
  - for each assertion `{location, keyword, expected}`, the outcome's
    annotations of that keyword at that instance location must be exactly
    `expected`, keyed by schema location from the document's root.

```cpp
#include <webcpp/json_schema.hpp>

#include "suite/runner.hpp"

#include <boost/core/lightweight_test.hpp>

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace js = webcpp::json_schema;

namespace {

// One part of "compatibility": "=2020" exactly, "<=2019" at most, "2019" at least; a list is
// the conjunction of its parts (the annotation suite's README).
bool admits(std::string_view part, int version) {
    if (part.starts_with("<=")) {
        return version <= std::stoi(std::string(part.substr(2)));
    }
    if (part.starts_with("=")) {
        return version == std::stoi(std::string(part.substr(1)));
    }
    return version >= std::stoi(std::string(part));
}

bool admits_all(std::string_view compatibility, int version) {
    std::size_t start = 0;
    while (start <= compatibility.size()) {
        const std::size_t end = std::min(compatibility.find(',', start), compatibility.size());
        if (!admits(compatibility.substr(start, end - start), version)) {
            return false;
        }
        start = end + 1;
    }
    return true;
}

// Each resource's URI and the JSON Pointer of its root in the document, walked without recursion.
std::map<std::string, std::string> resource_pointers(const boost::json::value& schema) {
    std::map<std::string, std::string> out;
    struct item { const boost::json::value* value; std::string pointer; std::string base; };
    std::vector<item> work{{.value = &schema, .pointer = "", .base = "urn:webcpp:root"}};
    while (!work.empty()) {
        item it = work.back();
        work.pop_back();
        const auto* object = it.value->if_object();
        if (object == nullptr) {
            continue;
        }
        if (const auto* id = object->if_contains("$id"); id != nullptr && id->is_string()) {
            boost::static_strings::static_string<4096> absolute;
            (void)js::boost_url<>::resolve(it.base, id->get_string(), absolute);
            it.base = std::string(absolute.substr(0, absolute.find('#')));
        }
        out.emplace(it.base, it.pointer);
        for (const auto& member : *object) {
            std::pmr::string child(it.pointer.begin(), it.pointer.end());
            js::detail::append_token(child, member.key());
            work.push_back({.value = &member.value(), .pointer = std::string(child), .base = it.base});
        }
    }
    return out;
}

std::string key_of(const js::annotation& a, const std::map<std::string, std::string>& resources) {
    const std::string_view absolute = a.absolute_keyword_location();
    const std::size_t hash = absolute.find('#');
    const std::string uri(absolute.substr(0, hash));
    std::string_view pointer = absolute.substr(hash + 1);
    pointer.remove_suffix(a.keyword().size() + 1);
    const auto it = resources.find(uri);
    return "#" + (it == resources.end() ? std::string() : it->second) + std::string(pointer);
}

}  // namespace

int main() {
    const js::validator_for<js::draft2020_12> validator;
    const std::filesystem::path dir = std::string(WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE) + "/annotations/tests";
    int assertions = 0;
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        const auto file = boost::json::parse(webcpp::test::suite::read_file(entry.path()));
        for (const auto& c : file.as_object().at("suite").as_array()) {
            const auto& kase = c.as_object();
            if (const auto* compat = kase.if_contains("compatibility");
                compat != nullptr && !admits_all(compat->as_string(), 2020)) {
                continue;
            }
            const auto schema = validator.compile(kase.at("schema"));
            if (!BOOST_TEST(schema.has_value())) {
                continue;
            }
            const auto resources = resource_pointers(kase.at("schema"));
            for (const auto& t : kase.at("tests").as_array()) {
                const auto outcome = schema->validate(t.as_object().at("instance"),
                                                      {.what = js::collect::errors_and_annotations});
                for (const auto& a : t.as_object().at("assertions").as_array()) {
                    ++assertions;
                    const auto& assertion = a.as_object();
                    boost::json::object found;
                    for (const js::annotation& n : outcome->annotations()) {
                        if (n.instance_location() == assertion.at("location").as_string() &&
                            n.keyword() == assertion.at("keyword").as_string()) {
                            found[key_of(n, resources)] = n.value();
                        }
                    }
                    BOOST_TEST_EQ(boost::json::value(found), assertion.at("expected"));
                }
            }
        }
    }
    BOOST_TEST(assertions > 50);
    return boost::report_errors();
}
```

  Append to `test/Jamfile`:

```
webcpp.run annotations : annotations_test.cpp : <library>/webcpp/third_party//json-schema-test-suite ;
```

- [ ] **Step 2: Run it and watch it fail.**
  `$B2 libs/json_schema/test//annotations`. Expected: FAIL, each expected
  object non-empty and each found one `{}`.
- [ ] **Step 3: Implement** the instruction, its lowering and the computed
  values. The cases that hold `unevaluated*` pass only with E2.9: until
  then the test skips any case whose schema names `unevaluated`. The skip
  sits in the loop above, as `if (std::string(serialize(kase.at("schema"))).find("unevaluated") != npos) { ++skipped; continue; }`,
  and the test prints `skipped until unevaluated: <n>`. E2.9 removes it.
- [ ] **Step 4: Run the lanes.**

```bash
lanes libs/json_schema/test
lint_clean
```

  Expected: `lanes: green`, every admitted assertion holding but the
  skipped ones; `lint: clean`.
- [ ] **Step 5: Commit.**

```bash
cd libs/json_schema
F="include/webcpp/json_schema/detail/compiler/opcode.hpp include/webcpp/json_schema/detail/compiler/compile.hpp \
include/webcpp/json_schema/detail/compiler/explain.hpp include/webcpp/json_schema/detail/vm/machine.hpp \
test/Jamfile test/annotations_test.cpp"
git add $F
$G commit -m "json_schema: annotations, collected as the specification defines them" -- $F
```

### Task E2.9: `unevaluated*` with marks, checkpoint and rewind

After E2.8, in the same worktree. Then `merge_task 2 annotations`.

**Files:**
- Create: `include/webcpp/json_schema/detail/compiler/evaluated.hpp`,
  `test/unevaluated_test.cpp`.
- Modify:
  - `detail/compiler/opcode.hpp` (appends `mark_evaluated = 33`,
    `loop_unevaluated_properties = 34`, `loop_unevaluated_items = 35`),
    `compile.hpp`, `explain.hpp`;
  - `detail/vm/machine.hpp`;
  - `test/annotations_test.cpp` (the skip removed), `test/Jamfile`,
    `test/known_failures/draft2020-12.json`.

**Interfaces:**
- Produces (spec 7.4):
  - `detail::contributors(document, node)`: the keywords of an
    `unevaluated_*` node's scope that evaluate properties or items.
  - The compiler emits `mark_evaluated` after each contributor reached
    from such a scope:
    - `flags` says names or indices;
    - `a` says one, a range, or all.

    It marks each `anyOf` in that scope exhaustive (P6).
  - The machine keeps a mark stack, an `inline_stack<mark, 64>` in the
    validation's resource. A mark is `{const boost::json::value* container,
    std::uint32_t member_or_index, std::uint32_t through_index}`.
    - A frame that ends invalid rewinds the marks to its entry.
    - So does an alternative that P4 discards.
  - `loop_unevaluated_*` applies its schema to every member or item of its
    instance location that holds no mark, then marks them all.
  - A schema without `unevaluated*` emits no mark and pays nothing.

- [ ] **Step 1: Write the failing test** `test/unevaluated_test.cpp`:

```cpp
#include <webcpp/json_schema.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/json/parse.hpp>

namespace js = webcpp::json_schema;

namespace {

const js::validator_for<js::draft2020_12> validator;

bool valid(std::string_view schema, std::string_view instance) {
    return validator.compile(boost::json::parse(schema))->is_valid(boost::json::parse(instance));
}

void properties() {
    const std::string_view s = R"({
        "properties": {"a": true},
        "anyOf": [{"properties": {"b": true}}, {"properties": {"c": true}}],
        "unevaluatedProperties": false
    })";
    BOOST_TEST(valid(s, R"({"a": 1, "b": 2, "c": 3})"));   // anyOf is exhaustive here
    BOOST_TEST(!valid(s, R"({"a": 1, "d": 4})"));
    const std::string_view failed_branch = R"({
        "anyOf": [{"properties": {"b": true}, "required": ["x"]}, true],
        "unevaluatedProperties": false
    })";
    BOOST_TEST(!valid(failed_branch, R"({"b": 1})"));   // the failed branch's marks are rewound
}

void items() {
    const std::string_view s = R"({"prefixItems": [true], "contains": {"type": "string"}, "unevaluatedItems": false})";
    BOOST_TEST(valid(s, R"([1, "a", "b"])"));
    BOOST_TEST(!valid(s, R"([1, "a", 2])"));
    BOOST_TEST(valid(R"({"$ref": "#/$defs/x", "$defs": {"x": {"items": true}}, "unevaluatedItems": false})", "[1, 2]"));
}

}  // namespace

int main() {
    properties();
    items();
    return boost::report_errors();
}
```

  Append `webcpp.run unevaluated : unevaluated_test.cpp ;` to
  `test/Jamfile`. Run it. Expected: FAIL, `unevaluatedProperties` without
  effect.
- [ ] **Step 2: Implement**, and remove `annotations_test`'s skip.
- [ ] **Step 3: Record the list, and run the lanes.**

```bash
record_known draft2020-12
lanes libs/json_schema/test
lint_clean
```

  Expected: `unevaluatedItems` and `unevaluatedProperties` gone,
  `lanes: green`, no `skipped` line, `lint: clean`.
- [ ] **Step 4: Commit,** then merge the lane into `e2`.

```bash
cd libs/json_schema
F="include/webcpp/json_schema/detail/compiler/evaluated.hpp include/webcpp/json_schema/detail/compiler/opcode.hpp \
include/webcpp/json_schema/detail/compiler/compile.hpp include/webcpp/json_schema/detail/compiler/explain.hpp \
include/webcpp/json_schema/detail/vm/machine.hpp test/Jamfile test/unevaluated_test.cpp test/annotations_test.cpp \
test/known_failures/draft2020-12.json"
git add $F
$G commit -m "json_schema: unevaluatedProperties and unevaluatedItems, by marks rewound with failed branches" -- $F
cd ~/dev/webcpp-json-schema-e2
merge_task 2 annotations
```

  Expected: `lanes: green`, `lint: clean`, a fast-forward of `e2`. The
  conflicts to expect, each resolved by keeping both sides:
  - `opcode.hpp`: E2.5 to E2.7 append no opcode, so none is expected
    there;
  - `test/Jamfile`;
  - `test/known_failures/draft2020-12.json`: the list is recorded again
    after the merge with `record_known draft2020-12`, never merged by hand.

### Task E2.10: The measured instance depth

After E2.7 and E2.9 are both in `e2`.

**Files:**
- Create: `test/measure/meta_depth_probe.cpp`.
- Modify: `test/measure/Jamfile`,
  `include/webcpp/json_schema/validator/options.hpp`
  (`default_max_depth`).

- [ ] **Step 1: The probe.** `meta_depth_probe.cpp` builds in code an
  instance `{"properties": {"properties": ...}}` nested `<depth>` levels. It
  validates it against the bundled 2020-12 meta-schema with
  `collect::errors`, into a counting resource, and prints the depth and
  the peak bytes. Add `webcpp.link meta-depth-probe : meta_depth_probe.cpp
  ;` to `test/measure/Jamfile`.
- [ ] **Step 2: Measure** on wasip2 at `-O0` for depths 256 to 8192, as
  Task E1.15 measures:

```bash
$B2 --build-dir=bin/measure-clang-wasip2 toolset=clang-wasip2 libs/json_schema/test/measure//meta-depth-probe-program
probe=$(find bin/measure-clang-wasip2 -name 'meta-depth-probe-program*.wasm' | head -1)
for d in 256 512 1024 2048 4096 8192; do wasmtime run "$probe" $d; done
```

  `default_max_depth` is the largest power of two whose peak stays at or
  under 1 MiB (spec 7.6). Write the table in the comment above the
  constant and in the ledger.
- [ ] **Step 3: Run the lanes** (the tests that name `default_max_depth`
  follow it).

```bash
lanes libs/json_schema/test
lint_clean
```

  Expected: `lanes: green`, `lint: clean`.
- [ ] **Step 4: Commit.**

```bash
cd libs/json_schema
F="test/measure/Jamfile test/measure/meta_depth_probe.cpp include/webcpp/json_schema/validator/options.hpp"
git add $F
$G commit -m "json_schema: the default instance depth, measured against the 2020-12 meta-schema" -- $F
```

### Task E2.11: E2 complete: 2020-12 whole, the reasons cleaned, the page, the merge

After E2.10.

**Files:**
- Create:
  - `doc/references.adoc`;
  - `example/json_schema_registry.cpp`, `example/json_schema_resolver.cpp`,
    `example/json_schema_annotations.cpp`, each with its `.expected`.
- Modify:
  - `test/suite/runner.hpp` (`reason_for` loses its schema rules, which
    blamed E2);
  - `test/known_failures/draft2020-12.json`;
  - `doc/json_schema.adoc`, `doc/compiling.adoc`, `doc/validating.adoc`;
  - `example/Jamfile`.

- [ ] **Step 1: The reasons cleaned** (the E2 to E5 review's I11):
  - `reason_for` keeps the permanent table and the file rules (`E5:
    formats`, `E5: other dialects`, `E5: content`);
  - the schema rules are removed, so a later failure in a reference or
    `unevaluated*` test is not recordable and must be fixed.

```bash
record_known draft2020-12
python3 -c "import json; l=json.load(open('libs/json_schema/test/known_failures/draft2020-12.json')); \
print(sorted({e['reason'] for e in l}))"
lanes libs/json_schema/test
grep 'suite draft2020-12' bin/lane-*.log
```

  Expected:
  - `['E5: formats', 'E5: other dialects']`;
  - `lanes: green`;
  - each lane's line with `0 unexpected, 0 stale`.

  So 1301 of 1301 required pass, and every optional group but formats and
  cross-draft does too.
- [ ] **Step 2: The examples.**

`example/json_schema_registry.cpp`:

```cpp
#include <webcpp/json_schema.hpp>

#include <boost/json/parse.hpp>

#include <iostream>

namespace js = webcpp::json_schema;

int main() {
    // tag::registry[]
    js::registry registry;
    (void)registry.add("https://example.com/money.json", boost::json::parse(R"({
        "type": "object",
        "required": ["amount", "currency"]
    })"));
    const js::validator validator;
    const auto schema = validator.compile(boost::json::parse(R"({"items": {"$ref": "https://example.com/money.json"}})"),
                                          {.registry = &registry});
    std::cout << std::boolalpha << schema->is_valid(boost::json::parse(R"([{"amount": 3, "currency": "EUR"}])"))
              << ' ' << schema->is_valid(boost::json::parse(R"([{"amount": 3}])")) << '\n';
    // end::registry[]
}
```

  `example/json_schema_registry.expected`: `true false`.

`example/json_schema_resolver.cpp`:

```cpp
#include <webcpp/json_schema.hpp>

#include <boost/json/parse.hpp>

#include <iostream>
#include <map>
#include <string>

namespace js = webcpp::json_schema;

int main() {
    // tag::resolver[]
    const std::map<std::string, std::string, std::less<>> documents{{"urn:example:positive", R"({"minimum": 0})"}};
    auto resolve = [&](std::string_view uri, const boost::json::storage_ptr& storage)
        -> boost::system::result<boost::json::value> {
        std::cout << "asked for " << uri << '\n';
        if (const auto it = documents.find(uri); it != documents.end()) {
            return boost::json::parse(it->second, storage);
        }
        return boost::system::errc::make_error_code(boost::system::errc::no_such_file_or_directory);
    };
    const js::validator validator;
    const auto schema = validator.compile(boost::json::parse(R"({"$ref": "urn:example:positive"})"),
                                          {.resolver = js::resolver_ref(resolve)});
    std::cout << std::boolalpha << schema->is_valid(boost::json::parse("-1")) << '\n';
    const auto missing = validator.compile(boost::json::parse(R"({"$ref": "urn:example:missing"})"),
                                           {.resolver = js::resolver_ref(resolve)});
    std::cout << missing.error().code.message() << ": " << missing.error().what << '\n';
    // end::resolver[]
}
```

  `example/json_schema_resolver.expected`:

```
asked for urn:example:positive
false
asked for urn:example:missing
the resolver failed: urn:example:missing
```

`example/json_schema_annotations.cpp`:

```cpp
#include <webcpp/json_schema.hpp>

#include <boost/json.hpp>

#include <iostream>

namespace js = webcpp::json_schema;

int main() {
    // tag::annotations[]
    const js::validator validator;
    const auto schema = validator.compile(boost::json::parse(R"({
        "title": "A point",
        "properties": {"x": {"type": "number"}, "y": {"type": "number"}}
    })"));
    const auto instance = boost::json::parse(R"({"x": 1, "y": 2})");
    const auto outcome = schema->validate(instance, {.what = js::collect::errors_and_annotations});
    for (const js::annotation& a : outcome->annotations()) {
        std::cout << a.keyword() << " at '" << a.instance_location() << "': " << boost::json::serialize(a.value())
                  << '\n';
    }
    // end::annotations[]
}
```

  `example/json_schema_annotations.expected`:

```
title at '': "A point"
properties at '': ["x","y"]
```

  Each joins `example/Jamfile` as `webcpp.example <name>.cpp ;`.
- [ ] **Step 3: The page.**
  - A section "References, anchors, dynamic references" (spec 18.8),
    `doc/references.adoc`, listing `json_schema_registry` and
    `json_schema_resolver`.
  - Compiling gains the registry, the resolver and the meta-schema check.
  - Validating gains annotations, with `json_schema_annotations`.

  Run:

```bash
$B2 -a --build-dir=bin/doc libs/json_schema/doc
lanes libs/json_schema/test libs/json_schema/example
lint_clean
```

  Expected: exit 0, `lanes: green`, `lint: clean`.
- [ ] **Step 4: Commit,** before the merge:

```bash
cd libs/json_schema
F="test/suite/runner.hpp test/known_failures/draft2020-12.json doc/references.adoc doc/json_schema.adoc \
doc/compiling.adoc doc/validating.adoc example/Jamfile example/json_schema_registry.cpp \
example/json_schema_registry.expected example/json_schema_resolver.cpp example/json_schema_resolver.expected \
example/json_schema_annotations.cpp example/json_schema_annotations.expected"
git add $F
$G commit -m "json_schema: 2020-12 whole: references, vocabularies and annotations on the page" -- $F
```

- [ ] **Step 5: The merge** (controller).

```bash
cd ~/dev/webcpp-json-schema-e2
merge_track 2
```

  Expected: `lanes: green`, `lint: clean`, a fast-forward of the canonical
  `main`, and no frozen file changed.

## E3: the message catalogue, the Boost.Locale header, and the proofs of output

In `~/dev/webcpp-json-schema-e3`, the library on `e3`, from `contract-1`.
E3.1 and E3.2 run at once (`task_worktree 3 pot`, `task_worktree 3 locale`),
merged by `merge_task 3 pot` then `merge_task 3 locale`. E3 edits no frozen
file: the messages and the renderers are E1's (Task E1.13), as the outcome
that holds them is frozen.

### Task E3.1: The `.pot` catalogue, printed from the table

Runs alongside E3.2 (slug `pot`).

**Files:**
- Create: `tools/Jamfile`, `tools/pot.hpp`, `tools/make_pot.cpp`,
  `i18n/webcpp_json_schema.pot` (generated), `test/pot_test.cpp`.
- Modify: `test/Jamfile`.

**Interfaces:**
- Produces `webcpp::tools::write_pot(std::ostream&)` in `tools/pot.hpp`,
  which `make_pot.cpp` and `pot_test.cpp` both include.
- `b2 libs/json_schema/tools//make_pot` builds the printer.
- The check is a C++ test, as the infra plan's point 3 settles: S1 runs
  Python generators only.

- [ ] **Step 1: Write the failing test** `test/pot_test.cpp`:

```cpp
#include "../tools/pot.hpp"

#include "suite/runner.hpp"

#include <boost/core/lightweight_test.hpp>

#include <sstream>
#include <string>

int main() {
    std::ostringstream printed;
    webcpp::tools::write_pot(printed);
    const std::string committed = webcpp::test::suite::read_file(WEBCPP_TEST_JSON_SCHEMA_POT);
    BOOST_TEST(printed.str() == committed);   // regenerate: the make_pot program into i18n/
    BOOST_TEST(committed.find("msgctxt \"max_length\"\nmsgid \"Expected at most {expected} characters but found "
                              "{count}.\"\n") != std::string::npos);
    return boost::report_errors();
}
```

  Append to `test/Jamfile`:

```
path-constant I18N : ../i18n ;
.i18n = [ regex.replace $(I18N) "\\\\" "/" ] ;
webcpp.run pot : pot_test.cpp
  : <define>"\"WEBCPP_TEST_JSON_SCHEMA_POT=\\\"$(.i18n)/webcpp_json_schema.pot\\\"\""
    [ webcpp.data $(I18N) ] <dependency>../i18n/webcpp_json_schema.pot ;
```

  Run `$B2 libs/json_schema/test//pot`. Expected: FAIL, `tools/pot.hpp` not
  found.
- [ ] **Step 2: Implement.**

`tools/pot.hpp`:

```cpp
#ifndef WEBCPP_TOOLS_JSON_SCHEMA_POT_HPP
#define WEBCPP_TOOLS_JSON_SCHEMA_POT_HPP

#include <webcpp/json_schema/output/messages.hpp>

#include <ostream>

namespace webcpp::tools {

// Writes the catalogue of json_schema's messages: the domain webcpp_json_schema, one entry per
// error kind, its context the kind's name and its msgid the English template. The templates hold
// no '"' or '\', which messages_test pins, so nothing needs escaping.
inline void write_pot(std::ostream& out) {
    out << "# Copyright (c) 2026 WebCpp.org\n"
           "#\n"
           "# Distributed under the Boost Software License, Version 1.0. (See\n"
           "# accompanying file LICENSE_1_0.txt or copy at\n"
           "# https://www.boost.org/LICENSE_1_0.txt)\n"
           "#\n"
           "# The messages of webcpp::json_schema, domain webcpp_json_schema. Printed by\n"
           "# tools/make_pot.cpp from output/messages.hpp; never edited by hand.\n"
           "msgid \"\"\n"
           "msgstr \"\"\n"
           "\"Content-Type: text/plain; charset=UTF-8\\n\"\n";
    for (const auto& m : webcpp::json_schema::detail::message_table()) {
        out << "\nmsgctxt \"" << m.name << "\"\nmsgid \"" << m.text << "\"\nmsgstr \"\"\n";
    }
}

}  // namespace webcpp::tools

#endif  // WEBCPP_TOOLS_JSON_SCHEMA_POT_HPP
```

`tools/make_pot.cpp`:

```cpp
#include "pot.hpp"

#include <iostream>

int main() {
    webcpp::tools::write_pot(std::cout);
}
```

`tools/Jamfile`:

```
# The printer of the .pot catalogue: b2 libs/json_schema/tools//make_pot,
# then run it into i18n/webcpp_json_schema.pot.

project : requirements <library>/webcpp/json_schema//json_schema ;

exe make_pot : make_pot.cpp ;
explicit make_pot ;
```

- [ ] **Step 3: Generate, and watch the test pass.**

```bash
$B2 --build-dir=bin/pot libs/json_schema/tools//make_pot
"$(find bin/pot -path '*json_schema/tools*' -name make_pot -type f -perm -u+x | head -1)" \
  > libs/json_schema/i18n/webcpp_json_schema.pot
lanes libs/json_schema/test
lint_clean
```

  Expected: `lanes: green`, `lint: clean`. Change one template's text in a
  scratch copy, see `pot` fail, and revert.
- [ ] **Step 4: Commit.**

```bash
cd libs/json_schema
F="tools/Jamfile tools/pot.hpp tools/make_pot.cpp i18n/webcpp_json_schema.pot test/Jamfile test/pot_test.cpp"
git add $F
$G commit -m "json_schema: the .pot catalogue, printed from the message table and checked by a test" -- $F
```

### Task E3.2: The Boost.Locale header

Runs alongside E3.1 (slug `locale`). Needs S4b (`/webcpp//boost_locale`).

**Files:**
- Create:
  - `include/webcpp/json_schema/output/locale.hpp`;
  - `test/locale_test.cpp`, `test/mo.hpp`;
  - `test/locale_without_exceptions.cpp`.
- Modify: `test/Jamfile`, `meta/include-boundaries.json` (an object for
  `output/locale.hpp` without `boost/locale`).

**Interfaces:**
- Produces spec 4.11's `locale_messages<Traits>`:
  - `explicit locale_messages(const std::locale&)`;
  - `void operator()(const error&, basic_string<Traits>&) const`.
- It translates `boost::locale::translate(context = kind's name, template)`
  through the locale's `boost::locale::message_format` facet. It rewrites
  the six named parameters to Boost.Locale's positional ones
  (`{expected}` to `{1}` ... `{keyword}` to `{6}`), and fills them with
  `boost::locale::format(...) % expected % received % count % names %
  indices % keyword`.
- The header opens with:

```cpp
#ifdef WEBCPP_JSON_SCHEMA_NO_EXCEPTIONS
#error "<webcpp/json_schema/output/locale.hpp> needs exceptions: Boost.Locale reports errors by throwing"
#endif
```

- [ ] **Step 1: Write the failing tests.**

`test/locale_test.cpp`, with a catalogue built in memory, so that no file
and no `msgfmt` is needed:

```cpp
#include <webcpp/json_schema.hpp>
#include <webcpp/json_schema/output/locale.hpp>

#include "mo.hpp"

#include <boost/core/lightweight_test.hpp>
#include <boost/json/parse.hpp>
#include <boost/locale.hpp>

#include <string>

namespace js = webcpp::json_schema;

int main() {
    const std::vector<char> catalogue = webcpp::test::mo({
        {std::string("type") + '\x04' + "Expected {expected} but found {received}.",
         "Esperado {expected}, encontrado {received}."},
    });
    boost::locale::gnu_gettext::messages_info info;
    info.language = "pt";
    info.country = "BR";
    info.encoding = "UTF-8";
    info.paths.push_back("/");
    info.domains.push_back(boost::locale::gnu_gettext::messages_info::domain("webcpp_json_schema"));
    info.callback = [&](const std::string&, const std::string&) { return catalogue; };
    const std::locale locale(std::locale::classic(), boost::locale::gnu_gettext::create_messages_facet<char>(info));
    const js::locale_messages<> translated(locale);
    const js::validator_for<js::draft2020_12> validator;
    const auto instance = boost::json::parse(R"({"a": 1})");
    const auto schema = validator.compile(boost::json::parse(R"({"properties": {"a": {"type": "string"}}, "required": ["b"]})"));
    const auto outcome = schema->validate(instance, {.messages = js::basic_message_hook<js::default_traits>(translated)});
    BOOST_TEST_EQ(outcome->message(outcome->errors()[0]), "Esperado string, encontrado integer.");
    BOOST_TEST_EQ(outcome->message(outcome->errors()[2]), "Required properties \"b\" are missing.");   // untranslated
    return boost::report_errors();
}
```

`test/mo.hpp`, GNU gettext's binary format:

```cpp
#ifndef WEBCPP_TEST_JSON_SCHEMA_MO_HPP
#define WEBCPP_TEST_JSON_SCHEMA_MO_HPP

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace webcpp::test {

// A .mo catalogue: the magic, revision 0, the count, the two string tables and an empty hash
// table, then the sorted originals and their translations, each NUL-terminated. Little-endian, as
// every target here is.
inline std::vector<char> mo(std::vector<std::pair<std::string, std::string>> entries) {
    std::sort(entries.begin(), entries.end());
    const auto n = static_cast<std::uint32_t>(entries.size());
    const std::uint32_t originals = 28;
    const std::uint32_t translations = originals + 8 * n;
    const std::uint32_t text = translations + 8 * n;
    std::vector<std::uint32_t> words{0x950412de, 0, n, originals, translations, 0, text};
    std::string strings;
    std::vector<std::uint32_t> table;
    for (const bool translated : {false, true}) {
        for (const auto& e : entries) {
            const std::string& s = translated ? e.second : e.first;
            table.push_back(static_cast<std::uint32_t>(s.size()));
            table.push_back(text + static_cast<std::uint32_t>(strings.size()));
            strings += s;
            strings.push_back('\0');
        }
    }
    words.insert(words.end(), table.begin(), table.end());
    std::vector<char> out(words.size() * 4);
    std::memcpy(out.data(), words.data(), out.size());
    out.insert(out.end(), strings.begin(), strings.end());
    return out;
}

}  // namespace webcpp::test

#endif  // WEBCPP_TEST_JSON_SCHEMA_MO_HPP
```

`test/locale_without_exceptions.cpp`, spec S3's "refused by name":

```cpp
// expected-error@*:* {{<webcpp/json_schema/output/locale.hpp> needs exceptions: Boost.Locale reports errors by throwing}}
#include <webcpp/json_schema/output/locale.hpp>
```

  Append to `test/Jamfile`:

```
# Boost.Locale reports errors by throwing, so the locale header and its
# test need exceptions; S4b refuses emscripten.
webcpp.headers-alone json_schema : ../include : json_schema/output/locale.hpp
  : <exception-handling>on <library>/webcpp//boost_locale : native wasip2 wasip3 ;
webcpp.run locale : locale_test.cpp
  : <exception-handling>on <library>/webcpp//boost_locale : native wasip2 wasip3 ;
webcpp.compile-diagnostic locale_without_exceptions : locale_without_exceptions.cpp
  : <define>WEBCPP_JSON_SCHEMA_NO_EXCEPTIONS ;
```

  E1's `headers-alone` calls name the `output/` headers one by one, so
  `locale.hpp` has this call alone.
- [ ] **Step 2: Run them and watch them fail.**
  `$B2 libs/json_schema/test//locale libs/json_schema/test//locale_without_exceptions`.
  Expected: FAIL, `locale.hpp` not found.
- [ ] **Step 3: Implement** as the Interfaces say.
- [ ] **Step 4: Run the lanes,** and see the lint name `locale.hpp` among
  the headers left out of the analysis without exceptions:

```bash
lanes libs/json_schema/test
lint_clean
grep -n 'locale.hpp' /tmp/json_schema-lint.log
```

  Expected: `lanes: green` (the locale programs on native, wasip2 and
  wasip3 only), `lint: clean`, and the lint's line naming
  `json_schema/output/locale.hpp`.
- [ ] **Step 5: Commit.**

```bash
cd libs/json_schema
F="include/webcpp/json_schema/output/locale.hpp meta/include-boundaries.json test/Jamfile test/locale_test.cpp \
test/mo.hpp test/locale_without_exceptions.cpp"
git add $F
$G commit -m "json_schema: an optional Boost.Locale header that translates the messages" -- $F
```

### Task E3.3: Every report of the suite checked against the official output schema

After E3.1 and E3.2, and after E2 is merged into the canonical `main`.

**Files:**
- Create:
  - `test/output_suite_test.cpp`;
  - `test/suite/suite_draft2020_12_output.cpp`;
  - `test/known_failures/draft2020-12-output.json`.
- Modify: `test/suite/runner.hpp` (the output mode), `test/Jamfile`.

**Interfaces:**
- `run_options` gains `bool output = false;`, appended.
- In output mode, `run_case` validates each test with
  `collect::everything`, and renders `basic`, `detailed` and `verbose`.
  It then validates each report against the official output schema,
  compiled once, with json_schema itself.
- A report that fails the schema is an unexpected failure, as is a wrong
  verdict.
- E4.4 adds a round-trip mode to the same file: whichever of the two
  merges second re-applies its `runner.hpp` change on top of the first,
  keeping both.

- [ ] **Step 1: Rebase on E2.**

```bash
cd ~/dev/webcpp-json-schema-e3
git -C libs/json_schema fetch "$CANONICAL" main
$G -C libs/json_schema rebase FETCH_HEAD
lanes libs/json_schema/test
```

  Expected: the conflicts are in `test/Jamfile` and
  `meta/include-boundaries.json` only, each resolved by keeping both
  sides; then `lanes: green`.
- [ ] **Step 2: Write the failing tests.**

`test/output_suite_test.cpp`:

```cpp
#include <webcpp/json_schema.hpp>

#include "suite/runner.hpp"

#include <boost/core/lightweight_test.hpp>

#include <filesystem>
#include <string>

namespace js = webcpp::json_schema;

namespace {

js::output_format format_named(std::string_view name) {
    if (name == "basic") {
        return js::output_format::basic;
    }
    if (name == "detailed") {
        return js::output_format::detailed;
    }
    return name == "verbose" ? js::output_format::verbose : js::output_format::flag;
}

}  // namespace

int main() {
    const std::filesystem::path base = std::string(WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE) + "/output-tests/draft2020-12";
    const js::validator_for<js::draft2020_12> validator;
    js::registry registry;
    const auto output_schema = boost::json::parse(webcpp::test::suite::read_file(base / "output-schema.json"));
    (void)registry.add(output_schema.at("$id").as_string(), output_schema);
    const js::compile_options options{.registry = &registry};
    int checked = 0;
    for (const auto& entry : std::filesystem::directory_iterator(base / "content")) {
        for (const auto& c : boost::json::parse(webcpp::test::suite::read_file(entry.path())).as_array()) {
            const auto schema = validator.compile(c.at("schema"));
            if (!BOOST_TEST(schema.has_value())) {
                continue;
            }
            for (const auto& t : c.at("tests").as_array()) {
                const auto outcome = schema->validate(t.at("data"), {.what = js::collect::everything});
                for (const auto& [format, expected] : t.at("output").as_object()) {
                    const auto report = outcome->to_json(format_named(format));
                    const auto check = validator.compile(expected, options);
                    BOOST_TEST(report.has_value() && check.has_value() && check->is_valid(*report));
                    ++checked;
                }
            }
        }
    }
    BOOST_TEST(checked > 0);
    return boost::report_errors();
}
```

  The runner's output mode, in `run_case`, after a test's verdict:

```cpp
if (options.output && schema && why.empty()) {
    const auto outcome = schema->validate(test.at("data"), {.what = webcpp::json_schema::collect::everything});
    for (const auto format : {webcpp::json_schema::output_format::basic, webcpp::json_schema::output_format::detailed,
                              webcpp::json_schema::output_format::verbose}) {
        const auto report = outcome->to_json(format);
        if (!report || !output_schema->is_valid(*report)) {
            why = "a report fails the output schema";
            break;
        }
    }
}
```

  `run` compiles `output_schema` once, from
  `WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE "/output-tests/draft2020-12/output-schema.json"`,
  when `options.output` is set. `suite_draft2020_12_output.cpp` is
  `suite_draft2020_12.cpp` with `.output = true` and the list
  `draft2020-12-output`.

  Append to `test/Jamfile`:

```
webcpp.run output_suite : output_suite_test.cpp : <library>/webcpp/third_party//json-schema-test-suite ;
suite-runner draft2020-12-output : suite/suite_draft2020_12_output.cpp ;
```

  Create the list as `[]`, and run
  `$B2 libs/json_schema/test//output_suite libs/json_schema/test//suite-draft2020-12-output`.
  Expected: FAIL wherever a report breaks the output schema, and an
  unexpected failure for each test that is still `E5:`.
- [ ] **Step 3: Fix the renderers** until no report breaks the schema. A
  renderer change is in `detail/output/render.hpp`, not a frozen file. Then
  record the list:

```bash
record_known draft2020-12-output
python3 -c "import json; print(sorted({e['reason'] for e in json.load(open('libs/json_schema/test/known_failures/draft2020-12-output.json'))}))"
lanes libs/json_schema/test
lint_clean
```

  Expected: `['E5: formats', 'E5: other dialects']`, `lanes: green`,
  `lint: clean`.
- [ ] **Step 4: Commit.**

```bash
cd libs/json_schema
F="test/Jamfile test/output_suite_test.cpp test/suite/runner.hpp test/suite/suite_draft2020_12_output.cpp \
test/known_failures/draft2020-12-output.json include/webcpp/json_schema/detail/output/render.hpp"
git add $F
$G commit -m "json_schema: every report of the suite valid against the official output schema" -- $F
```

### Task E3.4: A golden per message kind, the page, the merge

After E3.3.

**Files:**
- Create: `example/json_schema_messages.cpp` with its `.expected`.
- Modify: `test/messages_test.cpp`, `doc/output.adoc`, `example/Jamfile`.

- [ ] **Step 1: Write the goldens** (spec 20.1's E3 slice). Append to
  `messages_test.cpp`, called from `main`, one row per kind a 2020-12
  schema can raise.
  - The first table checks the outcome's first error.
  - The second checks the applicator's own error, the outcome's last.
  - `format`, `content` and `user` join with formats, content and user
    vocabularies.
  - `schema` (41) is raised by no keyword: decision PD5 records it as
    reserved.

```cpp
struct golden {
    const char* schema;
    const char* instance;
    const char* message;
};

void a_golden_per_kind() {
    constexpr std::array<golden, 21> first{{
        {.schema = "false", .instance = "1", .message = "No value is allowed here."},
        {.schema = R"({"enum": [1, 2]})", .instance = "3", .message = "Expected one of [1,2] but found 3."},
        {.schema = R"({"const": 1})", .instance = "2", .message = "Expected 1 but found 2."},
        {.schema = R"({"multipleOf": 2})", .instance = "3", .message = "Expected a multiple of 2 but found 3."},
        {.schema = R"({"maximum": 2})", .instance = "3", .message = "Expected at most 2 but found 3."},
        {.schema = R"({"exclusiveMaximum": 2})", .instance = "2", .message = "Expected less than 2 but found 2."},
        {.schema = R"({"minimum": 2})", .instance = "1", .message = "Expected at least 2 but found 1."},
        {.schema = R"({"exclusiveMinimum": 2})", .instance = "2", .message = "Expected more than 2 but found 2."},
        {.schema = R"({"minLength": 2})", .instance = "\"a\"", .message = "Expected a length of at least 2, found 1."},
        {.schema = R"({"pattern": "^a"})", .instance = "\"b\"", .message = "Expected a string matching \"^a\" but found \"b\"."},
        {.schema = R"({"pattern": "^(\\w+\\s?)*$"})", .instance = "\"A long sentence of words that ends badly!\"",
         .message = "The pattern \"^(\\\\w+\\\\s?)*$\" is too costly to match against \"A long sentence of words that ends badly!\"."},
        {.schema = R"({"minItems": 2})", .instance = "[1]", .message = "Expected an item count of at least 2, found 1."},
        {.schema = R"({"uniqueItems": true})", .instance = "[1, 1]", .message = "Expected unique items but items 0, 1 are equal."},
        {.schema = R"({"contains": {"const": 1}})", .instance = "[2]",
         .message = "Expected an item matching the contains schema but found none."},
        {.schema = R"({"contains": true, "minContains": 2})", .instance = "[1]",
         .message = "Expected a match count of at least 2, found 1."},
        {.schema = R"({"contains": true, "maxContains": 1})", .instance = "[1, 2]",
         .message = "Expected a match count of at most 1, found 2."},
        {.schema = R"({"maxProperties": 0})", .instance = R"({"a": 1})",
         .message = "Expected a property count of at most 0, found 1."},
        {.schema = R"({"minProperties": 1})", .instance = "{}", .message = "Expected a property count of at least 1, found 0."},
        {.schema = R"({"dependentRequired": {"a": ["b"]}})", .instance = R"({"a": 1})",
         .message = "Required properties \"b\" are missing."},
        {.schema = R"({"not": true})", .instance = "1", .message = "Expected the value not to match the not schema."},
        {.schema = R"({"oneOf": [true, true]})", .instance = "1",
         .message = "Expected the value to match exactly one oneOf schema; matching: 0, 1."},
    }};
    for (const golden& g : first) {
        BOOST_TEST_EQ(first_message(g.schema, g.instance), std::string(g.message));
    }
    constexpr std::array<golden, 13> own{{
        {.schema = R"({"properties": {"a": false}})", .instance = R"({"a": 1})",
         .message = "Properties \"a\" do not match their schemas."},
        {.schema = R"({"patternProperties": {"^a": false}})", .instance = R"({"ab": 1})",
         .message = "Properties \"ab\" do not match the schemas of their patterns."},
        {.schema = R"({"additionalProperties": false})", .instance = R"({"x": 1})",
         .message = "Additional properties \"x\" are not allowed."},
        {.schema = R"({"propertyNames": {"maxLength": 1}})", .instance = R"({"ab": 1})",
         .message = "Property names \"ab\" do not match the propertyNames schema."},
        {.schema = R"({"dependentSchemas": {"a": false}})", .instance = R"({"a": 1})",
         .message = "The schemas required by properties \"a\" do not match."},
        {.schema = R"({"prefixItems": [false]})", .instance = "[1]",
         .message = "Expected the first items to match their schemas; failing: 1."},
        {.schema = R"({"items": false})", .instance = "[1, 2]", .message = "Expected every item to match the items schema; failing: 2."},
        {.schema = R"({"unevaluatedItems": false})", .instance = "[1]", .message = "Unevaluated items are not allowed; found: 1."},
        {.schema = R"({"unevaluatedProperties": false})", .instance = R"({"u": 1})",
         .message = "Unevaluated properties \"u\" are not allowed."},
        {.schema = R"({"allOf": [false]})", .instance = "1", .message = "Expected the value to match every allOf schema."},
        {.schema = R"({"anyOf": [false]})", .instance = "1", .message = "Expected the value to match at least one anyOf schema."},
        {.schema = R"({"if": true, "then": false})", .instance = "1",
         .message = "Expected the value to match the schema its if selects."},
        {.schema = R"({"$defs": {"s": false}, "$ref": "#/$defs/s"})", .instance = "1",
         .message = "Expected the value to match the referenced schema \"#/$defs/s\"."},
    }};
    for (const golden& g : own) {
        const auto outcome = validator.compile(boost::json::parse(g.schema))->validate(boost::json::parse(g.instance));
        BOOST_TEST_EQ(std::string(outcome->message(outcome->errors().back())), std::string(g.message));
    }
    const auto deep = validator.compile(boost::json::parse(R"({"items": {"items": true}})"));
    const auto limited = deep->validate(boost::json::parse("[[1]]"), {.max_depth = 1});
    BOOST_TEST_EQ(std::string(limited->message(limited->errors()[0])), "The value nests deeper than the limit of 1.");
}
```

  The pattern row uses the pattern that `hostile_test` pins as a limit:
  `(a*)*b` may be answered `not_found` quickly by SRELL's required-character
  check.
- [ ] **Step 2: The page and its example.** `example/json_schema_messages.cpp`
  shows the English message, then a hook's, then the locale header's ready
  hook. The last falls back to English when the catalogue has no entry,
  which is what this program shows: it needs no `.mo` file.

```cpp
#include <webcpp/json_schema.hpp>

#include <boost/json/parse.hpp>

#include <iostream>
#include <string>

namespace js = webcpp::json_schema;

int main() {
    // tag::messages[]
    const js::validator validator;
    const auto schema = validator.compile(boost::json::parse(R"({"maxLength": 3})"));
    const auto instance = boost::json::parse(R"("abcdef")");
    const auto english = schema->validate(instance);
    std::cout << english->message(english->errors()[0]) << '\n';
    auto shout = [](const js::error& e, std::pmr::string& out) {
        out = "INVALID ";
        out += e.keyword();
    };
    const auto hooked = schema->validate(instance, {.messages = js::basic_message_hook<js::default_traits>(shout)});
    std::cout << hooked->message(hooked->errors()[0]) << '\n';
    // end::messages[]
}
```

  `example/json_schema_messages.expected`:

```
Expected at most 3 characters but found 6.
INVALID maxLength
```

  The page's "Output formats and messages" section adds:
  - the message parameters' table;
  - this example;
  - the `.pot` catalogue, its domain and contexts;
  - `locale_messages`, with the locale test's catalogue setup as its
    listing, through a `tag::catalogue[]` region in `locale_test.cpp`.
    doc-check accepts that include: the page may include a test file by
    region (`{webcpp-root}` is the superproject; the library's tests are
    reached through `{examples}/../test`, which doc-check's include rule
    allows only inside `doc/`, `{examples}` and `{twins}`).
  - If doc-check refuses the test include, the listing moves to a
    native-only example `json_schema_locale.cpp` with
    `<exception-handling>on <library>/webcpp//boost_locale : native wasip2
    wasip3`, holding the same in-memory catalogue (a copy of `mo.hpp`'s
    function), and printing `Esperado string, encontrado integer.` as its
    `.expected`. The ledger records which.

  Run:

```bash
$B2 -a --build-dir=bin/doc libs/json_schema/doc
lanes libs/json_schema/test libs/json_schema/example
lint_clean
```

  Expected: exit 0, `lanes: green`, `lint: clean`.
- [ ] **Step 3: Commit,** before the merge:

```bash
cd libs/json_schema
F="test/messages_test.cpp doc/output.adoc example/Jamfile example/json_schema_messages.cpp \
example/json_schema_messages.expected"
git add $F
$G commit -m "json_schema: a golden per message kind, and messages and translations on the page" -- $F
```

- [ ] **Step 4: The merge** (controller).

```bash
cd ~/dev/webcpp-json-schema-e3
merge_track 3
```

  Expected: `lanes: green`, `lint: clean`, a fast-forward, and no frozen
  file changed.

## E4: the emitter

In `~/dev/webcpp-json-schema-e4`, the library on `e4`, from `contract-1`.
E4.1 and E4.2 run at once (`task_worktree 4 equivalent`, `task_worktree 4
emitter`), merged by `merge_task 4 equivalent` then `merge_task 4 emitter`.
E4 reads only the IR.

**Equivalence and the emitter agree by construction** (the E2 to E5
review's C2; decision PD3). Spec 16.4 asks that a schema emitted into its
own dialect reload to an `ir::equivalent` IR. The emitter must extract
shared nodes into definitions (spec 11.2), and must wrap a `$ref` that has
evaluated siblings in draft 4 to 7 and OpenAPI 3.0, so equivalence is
defined modulo exactly those rewrites:
1. `defs` keywords are not compared: only the graph reached from the root
   is.
2. A node whose one comparable keyword is a `ref` is transparent: its
   target is compared in its place. A chain of such nodes is followed with
   a visited set, and a chain that loops is compared as itself.
3. An `all_of` of exactly one subschema that is a transparent node compares
   as that node's `ref`.
4. `ignored` keywords are not compared, and neither are `name`, `source`,
   pointers and resource URIs (spec 5.4).

The emitter introduces no other rewrite into the same dialect; any new one
adds its rule here and in E4.1's test in the same change.

### Task E4.1: `ir::equivalent`

Runs alongside E4.2 (slug `equivalent`).

**Files:**
- Create: `include/webcpp/json_schema/ir/equivalent.hpp`,
  `test/equivalent_test.cpp`.
- Modify: `include/webcpp/json_schema.hpp`, `test/Jamfile`.

**Interfaces:**
- Produces (spec 5.4, with the rules above):

```cpp
namespace webcpp::json_schema::ir {
template <class Traits>
bool equivalent(const basic_document<Traits>& a, node_id a_root, const basic_document<Traits>& b, node_id b_root,
                std::pmr::memory_resource& scratch);
template <class Traits>
bool equivalent(const basic_document<Traits>& a, node_id a_root, const basic_document<Traits>& b, node_id b_root);
}
```

  The second form uses the default resource. It is for tests and tools,
  and its Doc Comment says so (spec 14: a program that counts uses the
  first). The IR's boundary object already covers `ir/*.hpp`;
  `equivalent.hpp` needs no exception from it.

- [ ] **Step 1: Write the failing test** `test/equivalent_test.cpp`:

```cpp
#include <webcpp/json_schema.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/json/parse.hpp>

namespace js = webcpp::json_schema;

namespace {

const js::validator_for<js::draft2020_12> validator;

bool same(std::string_view a, std::string_view b) {
    const auto x = validator.load(boost::json::parse(a));
    const auto y = validator.load(boost::json::parse(b));
    return js::ir::equivalent(*x, x->root(), *y, y->root());
}

void structure() {
    BOOST_TEST(same(R"({"maximum": 1})", R"({"maximum": 1.0})"));
    BOOST_TEST(same(R"({"const": {"a": 1, "b": 2}})", R"({"const": {"b": 2, "a": 1.0}})"));
    BOOST_TEST(same(R"({"$id": "urn:a", "type": "string"})", R"({"$id": "urn:b", "type": "string"})"));
    BOOST_TEST(same(R"({"type": "string"})", R"({"type": ["string"]})"));
    BOOST_TEST(same(R"({"type": "string", "minLength": 1})", R"({"minLength": 1, "type": "string"})"));
    BOOST_TEST(!same(R"({"minimum": 1})", R"({"minimum": 2})"));
    BOOST_TEST(!same(R"({"type": "string"})", R"({"type": "string", "title": "x"})"));
    BOOST_TEST(!same(R"({"properties": {"a": true}})", R"({"properties": {"b": true}})"));
    BOOST_TEST(!same(R"({"x-a": 1})", R"({"x-b": 1})"));
}

void modulo_the_emitter_s_rewrites() {
    // a shared node extracted into definitions, and referenced
    BOOST_TEST(same(R"({"properties": {"a": {"$ref": "#/properties/b"}, "b": {"type": "integer"}}})",
                    R"({"properties": {"a": {"$ref": "#/$defs/__schema0"}, "b": {"$ref": "#/$defs/__schema0"}},
                        "$defs": {"__schema0": {"type": "integer"}}})"));
    // a $ref with siblings, wrapped in an allOf of one
    BOOST_TEST(same(R"({"$defs": {"i": {"type": "integer"}}, "$ref": "#/$defs/i", "minimum": 5})",
                    R"({"$defs": {"i": {"type": "integer"}}, "allOf": [{"$ref": "#/$defs/i"}], "minimum": 5})"));
    // cycles end
    BOOST_TEST(same(R"({"properties": {"next": {"$ref": "#"}}, "type": "object"})",
                    R"({"type": "object", "properties": {"next": {"$ref": "#"}}})"));
    BOOST_TEST(same(R"({"$defs": {"a": {"$ref": "#/$defs/b"}, "b": {"$ref": "#/$defs/a"}}})",
                    R"({"$defs": {"a": {"$ref": "#/$defs/b"}, "b": {"$ref": "#/$defs/a"}}})"));
}

}  // namespace

int main() {
    structure();
    modulo_the_emitter_s_rewrites();
    return boost::report_errors();
}
```

  E4 runs from `contract-1`, before E2's references exist, so the cases
  holding `$ref` sit in `modulo_the_emitter_s_rewrites`. It is called only
  `#ifdef WEBCPP_TEST_JSON_SCHEMA_WITH_REFERENCES`, a define this task's
  Jamfile line does not give; E4.4 removes the condition after its rebase
  on E2:

```cpp
int main() {
    structure();
#ifdef WEBCPP_TEST_JSON_SCHEMA_WITH_REFERENCES
    modulo_the_emitter_s_rewrites();
#endif
    return boost::report_errors();
}
```

  (This `main` replaces the one above.) Append `webcpp.run equivalent :
  equivalent_test.cpp ;` and `webcpp.headers-alone json_schema :
  ../include : json_schema/ir/equivalent.hpp ;` to `test/Jamfile`.
- [ ] **Step 2: Run it and watch it fail.**
  `$B2 libs/json_schema/test//equivalent`. Expected: FAIL,
  `'webcpp/json_schema/ir/equivalent.hpp' file not found`.
- [ ] **Step 3: Implement.**

```cpp
template <class Traits>
bool equivalent(const basic_document<Traits>& a, node_id a_root, const basic_document<Traits>& b, node_id b_root,
                std::pmr::memory_resource& scratch) {
    using pair = std::pair<std::uint32_t, std::uint32_t>;
    boost::unordered_flat_set<pair, boost::hash<pair>, std::equal_to<>, std::pmr::polymorphic_allocator<pair>>
        assumed(&scratch);
    detail::inline_stack<pair, 32> work{std::pmr::polymorphic_allocator<pair>(&scratch)};
    work.push_back({static_cast<std::uint32_t>(detail::through_refs(a, a_root, scratch)),
                    static_cast<std::uint32_t>(detail::through_refs(b, b_root, scratch))});
    while (!work.empty()) {
        const pair p = work.back();
        work.pop_back();
        if (!assumed.insert(p).second) {
            continue;   // already being compared: a cycle, or a shared node
        }
        if (!detail::same_node(a, node_id{p.first}, b, node_id{p.second}, work, scratch)) {
            return false;
        }
    }
    return true;
}
```

  - `through_refs` applies rule 2, following transparent nodes to their
    target with a visited set.
  - `same_node` compares kinds, then the comparable keywords:
    - sorted by kind, `defs` and `ignored` left out (rules 1 and 4);
    - an `all_of` of one transparent node read as its `ref` (rule 3);
    - `unknown` and `user` also sorted by name.
  - It compares payloads, pushing each pair of subschemas through
    `through_refs`, constants by `detail::equal`, numbers by
    `detail::compare`.
- [ ] **Step 4: Run it and watch it pass** on the five lanes:

```bash
lanes libs/json_schema/test
lint_clean
```

  Expected: `lanes: green`, `lint: clean`.
- [ ] **Step 5: Commit.**

```bash
cd libs/json_schema
F="include/webcpp/json_schema.hpp include/webcpp/json_schema/ir/equivalent.hpp test/Jamfile test/equivalent_test.cpp"
git add $F
$G commit -m "json_schema: equivalence of two IR graphs, modulo the emitter's own rewrites" -- $F
```

### Task E4.2: The emitter, 2020-12

Runs alongside E4.1 (slug `emitter`).

**Files:**
- Create:
  - `include/webcpp/json_schema/emitter/emit.hpp`;
  - `include/webcpp/json_schema/detail/emitter/reach.hpp`,
    `detail/emitter/writer.hpp`;
  - `test/emitter_test.cpp`.
- Modify: `include/webcpp/json_schema.hpp`, `test/Jamfile`,
  `meta/include-boundaries.json`. Its object:

```json
        {
            "headers": [
                "include/webcpp/json_schema/emitter/*.hpp",
                "include/webcpp/json_schema/detail/emitter/*.hpp"
            ],
            "must-not-include": [
                "webcpp/json_schema/detail/compiler",
                "webcpp/json_schema/detail/vm",
                "boost/locale",
                "srell",
                "boost/url",
                "webcpp/lexical"
            ],
            "why": "emitting needs only the IR"
        }
```

**Interfaces:**
- Produces spec 4.10's `unrepresentable`, `emit_options`, `emitted` and the
  two `emit`s. The rules (spec 11.2, and the section above):
  - **Extraction.** A node is written once, under the target's definitions,
    and referenced by `$ref`, when it is the target of a `ref` (and not the
    root), or reached more than once, or on a cycle.
    - A node that is a `defs` entry keeps its place and its name.
    - Any other is named `__schema<N>`, in order of first reach from 0.
    - A `ref` to the root is written `#`.
  - **Cycles.** `detail/emitter/reach.hpp` colours the graph itself,
    iteratively, as the compiler's cycle search does. The emitter's
    boundary keeps it from including the compiler.
  - **Order.** `$schema` comes first at the root (with
    `write_schema_keyword`), then `$id`, `$anchor`, `$dynamicAnchor`, then
    the IR's keywords in order.
  - **Base URIs.** The default base `urn:webcpp:root` is never written as
    `$id`. A resource with any other URI is written with its `$id`, a
    `base_uri` given at load included: the IR cannot tell one from an
    `$id`, as the page says.
  - **Resources of other documents** are embedded under the definitions as
    `__resource<N>`, each with its `$id`. A `ref` into one is written as
    that resource's URI and the target's pointer inside it.
  - **Unrepresentable.** With `error`, the result is `errc::unrepresentable`,
    `where` the node's emitted pointer and `what` the keyword's name. With
    `drop`, the keyword is left out and its name appended to the node's
    `"x-webcpp-dropped"`, written last.
  - **Values** (`const`, `enum`, annotations, `unknown`) are copied with
    `detail::deep_copy` into `options.storage`.
- The writer walks the nodes with an explicit stack of `(node, next
  keyword, output object)`.

- [ ] **Step 1: Write the failing test** `test/emitter_test.cpp`:

```cpp
#include <webcpp/json_schema.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/json.hpp>

#include <array>
#include <string>

namespace js = webcpp::json_schema;

namespace {

const js::validator_for<js::draft2020_12> validator;

struct cell {
    const char* source;      // a 2020-12 schema
    js::dialect target;
    const char* expected;    // the emitted schema, or "!" for unrepresentable
};

std::string emitted(const char* source, js::dialect target, js::unrepresentable mode = js::unrepresentable::error) {
    const auto document = validator.load(boost::json::parse(source));
    const auto out = js::emit(*document, target, {.on_unrepresentable = mode, .write_schema_keyword = false});
    if (!out) {
        return out.error().code == js::errc::unrepresentable ? "!" : "error";
    }
    return boost::json::serialize(out->schema);
}

constexpr std::array<cell, 9> cells_2020_12{{
    {.source = R"({"prefixItems":[{"type":"string"}],"items":{"type":"integer"}})", .target = js::dialect::draft2020_12,
     .expected = R"({"prefixItems":[{"type":"string"}],"items":{"type":"integer"}})"},
    {.source = R"({"exclusiveMaximum":3})", .target = js::dialect::draft2020_12, .expected = R"({"exclusiveMaximum":3})"},
    {.source = R"({"dependentRequired":{"a":["b"]},"dependentSchemas":{"c":{"required":["d"]}}})",
     .target = js::dialect::draft2020_12,
     .expected = R"({"dependentRequired":{"a":["b"]},"dependentSchemas":{"c":{"required":["d"]}}})"},
    {.source = R"({"type":["string","null"]})", .target = js::dialect::draft2020_12, .expected = R"({"type":["string","null"]})"},
    {.source = R"({"const":1})", .target = js::dialect::draft2020_12, .expected = R"({"const":1})"},
    {.source = R"({"unevaluatedProperties":false})", .target = js::dialect::draft2020_12,
     .expected = R"({"unevaluatedProperties":false})"},
    {.source = R"({"properties":{"a":false,"b":true}})", .target = js::dialect::draft2020_12,
     .expected = R"({"properties":{"a":false,"b":true}})"},
    {.source = R"({"if":{"type":"string"},"then":{"minLength":1}})", .target = js::dialect::draft2020_12,
     .expected = R"({"if":{"type":"string"},"then":{"minLength":1}})"},
    {.source = R"({"title":"T","x-note":1,"foo":2})", .target = js::dialect::draft2020_12,
     .expected = R"({"title":"T","x-note":1,"foo":2})"},
}};

void forms() {
    for (const cell& c : cells_2020_12) {
        BOOST_TEST_EQ(emitted(c.source, c.target), std::string(c.expected));
    }
}

void order_and_determinism() {
    BOOST_TEST_EQ(emitted(R"({"type":"object","$anchor":"a","$id":"urn:x"})", js::dialect::draft2020_12),
                  R"({"$id":"urn:x","$anchor":"a","type":"object"})");
    const auto document = validator.load(boost::json::parse(R"({"type":"string"})"));
    const auto once = js::emit(*document, js::dialect::draft2020_12);
    const auto twice = js::emit(*document, js::dialect::draft2020_12);
    BOOST_TEST_EQ(boost::json::serialize(once->schema),
                  R"({"$schema":"https://json-schema.org/draft/2020-12/schema","type":"string"})");
    BOOST_TEST_EQ(boost::json::serialize(once->schema), boost::json::serialize(twice->schema));
}

void references_sharing_and_cycles() {
    BOOST_TEST_EQ(emitted(R"({"$defs":{"a":{"type":"string"}},"$ref":"#/$defs/a"})", js::dialect::draft2020_12),
                  R"({"$defs":{"a":{"type":"string"}},"$ref":"#/$defs/a"})");
    BOOST_TEST_EQ(emitted(R"({"properties":{"a":{"$ref":"#/properties/b"},"b":{"type":"integer"}}})", js::dialect::draft2020_12),
                  R"({"properties":{"a":{"$ref":"#/$defs/__schema0"},"b":{"$ref":"#/$defs/__schema0"}},"$defs":{"__schema0":{"type":"integer"}}})");
    BOOST_TEST_EQ(emitted(R"({"properties":{"next":{"$ref":"#"}}})", js::dialect::draft2020_12),
                  R"({"properties":{"next":{"$ref":"#"}}})");
    BOOST_TEST_EQ(emitted(R"({"$dynamicAnchor":"n","items":{"$dynamicRef":"#n"}})", js::dialect::draft2020_12),
                  R"({"$dynamicAnchor":"n","items":{"$dynamicRef":"#n"}})");
}

}  // namespace

int main() {
    forms();
    order_and_determinism();
#ifdef WEBCPP_TEST_JSON_SCHEMA_WITH_REFERENCES
    references_sharing_and_cycles();
#endif
    return boost::report_errors();
}
```

  Append `webcpp.run emitter : emitter_test.cpp ;` and the two
  `headers-alone` lines for `emitter/*.hpp` and `detail/emitter/*.hpp` to
  `test/Jamfile`.
- [ ] **Step 2: Run it and watch it fail.** `$B2 libs/json_schema/test//emitter`.
  Expected: FAIL, `'webcpp/json_schema/emitter/emit.hpp' file not found`.
- [ ] **Step 3: Implement.** The reach analysis:

```cpp
template <class Traits>
reach_plan analyse(const ir::basic_document<Traits>& d, ir::node_id root, std::pmr::memory_resource& scratch) {
    reach_plan plan(d.size(), &scratch);   // per node: reaches, ref_target, on_cycle, defs name, generated name
    detail::inline_stack<ir::node_id, 32> work{std::pmr::polymorphic_allocator<ir::node_id>(&scratch)};
    work.push_back(root);
    plan.first_reach_order.push_back(root);
    while (!work.empty()) {
        const ir::node_id n = work.back();
        work.pop_back();
        for (const auto& edge : emitted_edges(d, n)) {   // every subschema and ref target
            auto& info = plan.of(edge.target);
            info.ref_target = info.ref_target || edge.is_ref;
            if (++info.reaches == 1 && !edge.is_defs_entry) {
                plan.first_reach_order.push_back(edge.target);
                work.push_back(edge.target);
            }
        }
    }
    mark_cycles(d, root, plan, scratch);   // this header's own iterative colouring
    std::uint32_t next = 0;
    for (const ir::node_id n : plan.first_reach_order) {
        auto& info = plan.of(n);
        info.extracted = n != root && (info.ref_target || info.reaches > 1 || info.on_cycle);
        if (info.extracted && !info.defs_name) {
            info.generated_name = next++;   // __schema<N>
        }
    }
    return plan;
}
```

  The writer emits a node's keywords in order. Each subschema position
  holds `{"$ref": "#/$defs/<name>"}` for an extracted node, or the node
  written in place. Then come the definitions: the existing `defs`
  entries, followed by the generated ones in order.
- [ ] **Step 4: Run it and watch it pass** on the five lanes:

```bash
lanes libs/json_schema/test
lint_clean
```

  Expected: `lanes: green`, `lint: clean` (its include boundary included).
- [ ] **Step 5: Commit.**

```bash
cd libs/json_schema
F="include/webcpp/json_schema.hpp include/webcpp/json_schema/emitter/emit.hpp \
include/webcpp/json_schema/detail/emitter/reach.hpp include/webcpp/json_schema/detail/emitter/writer.hpp \
meta/include-boundaries.json test/Jamfile test/emitter_test.cpp"
git add $F
$G commit -m "json_schema: the emitter: shared nodes extracted, cycles as references, a fixed key order" -- $F
```

### Task E4.3: Every dialect's forms, and OpenAPI 3.0's emit side

After E4.1 and E4.2 are merged.

**Files:**
- Create: `include/webcpp/json_schema/detail/emitter/forms.hpp` (spec 11.1,
  one writer per row and target).
- Modify: `detail/emitter/writer.hpp`, `test/emitter_test.cpp`.

**Interfaces:**
- Spec 11.1's table, plus:
  - **A `$ref` with evaluated siblings, in draft 4 to 7 and OpenAPI 3.0,**
    is written as `"allOf": [{"$ref": ...}]` in the `ref`'s place.
    "Evaluated" means anything but `ignored`, `defs` and an annotation-only
    keyword. Those dialects ignore `$ref`'s siblings (spec 6.2), so this is
    the one form that keeps the verdicts. Spec 11.1 leaves it implicit; the
    plan settles it (decision PD4).
  - **Resource keywords:**
    - `$id` is written `id` in draft 4;
    - `$anchor` is written `"$id": "#a"` in drafts 6 and 7, and `"id":
      "#a"` in draft 4;
    - `$dynamicAnchor` is unrepresentable outside 2020-12.
  - **`type` with `integer`.** A 2020-12 `integer` is
    `integer_by_value = true`, which draft 4's `integer` is not. So a
    `types` holding `integer` with `integer_by_value` is unrepresentable in
    draft 4, and the reverse holds out of draft 4 (E5.11 covers that
    direction).
  - **OpenAPI 3.0 refuses** what its Schema Object lacks:
    - `contains`, `if`, `propertyNames`;
    - `{"type": "null"}`;
    - `examples`, `$comment`, and every non-`x-` unknown keyword.

- [ ] **Step 1: Write the failing goldens.** Append to `emitter_test.cpp` a
  second table, one row per cell of spec 11.1 for the other targets, plus
  the cells above. Each `"!"` is `unrepresentable` in `error` mode:

```cpp
constexpr std::array<cell, 47> cells_other{{
    // defs
    {.source = R"({"$defs":{"a":{"type":"string"}},"$ref":"#/$defs/a"})", .target = js::dialect::draft2019_09,
     .expected = R"({"$defs":{"a":{"type":"string"}},"$ref":"#/$defs/a"})"},
    {.source = R"({"$defs":{"a":{"type":"string"}},"$ref":"#/$defs/a"})", .target = js::dialect::draft7,
     .expected = R"({"definitions":{"a":{"type":"string"}},"$ref":"#/definitions/a"})"},
    {.source = R"({"$defs":{"a":{"type":"string"}},"$ref":"#/$defs/a","minimum":5})", .target = js::dialect::draft7,
     .expected = R"({"definitions":{"a":{"type":"string"}},"allOf":[{"$ref":"#/definitions/a"}],"minimum":5})"},
    {.source = R"({"$defs":{"a":{"type":"string"}},"$ref":"#/$defs/a"})", .target = js::dialect::draft4,
     .expected = R"({"definitions":{"a":{"type":"string"}},"$ref":"#/definitions/a"})"},
    {.source = R"({"$defs":{"a":{"type":"string"}},"$ref":"#/$defs/a"})", .target = js::dialect::openapi3_0,
     .expected = R"({"$ref":"#/components/schemas/a"})"},
    // prefix_items and items
    {.source = R"({"prefixItems":[{"type":"string"}],"items":{"type":"integer"}})", .target = js::dialect::draft2019_09,
     .expected = R"({"items":[{"type":"string"}],"additionalItems":{"type":"integer"}})"},
    {.source = R"({"prefixItems":[{"type":"string"}],"items":{"type":"integer"}})", .target = js::dialect::draft6,
     .expected = R"({"items":[{"type":"string"}],"additionalItems":{"type":"integer"}})"},
    {.source = R"({"prefixItems":[{"type":"string"}],"items":{"type":"integer"}})", .target = js::dialect::draft4,
     .expected = R"({"items":[{"type":"string"}],"additionalItems":{"type":"integer"}})"},
    {.source = R"({"prefixItems":[{"type":"string"}]})", .target = js::dialect::openapi3_0, .expected = "!"},
    {.source = R"({"items":{"type":"integer"}})", .target = js::dialect::openapi3_0, .expected = R"({"items":{"type":"integer"}})"},
    // exclusive_maximum
    {.source = R"({"exclusiveMaximum":3})", .target = js::dialect::draft7, .expected = R"({"exclusiveMaximum":3})"},
    {.source = R"({"exclusiveMaximum":3})", .target = js::dialect::draft4, .expected = R"({"maximum":3,"exclusiveMaximum":true})"},
    {.source = R"({"exclusiveMaximum":3})", .target = js::dialect::openapi3_0, .expected = R"({"maximum":3,"exclusiveMaximum":true})"},
    // dynamic_ref
    {.source = R"({"$dynamicAnchor":"n","items":{"$dynamicRef":"#n"}})", .target = js::dialect::draft2019_09, .expected = "!"},
    {.source = R"({"$dynamicAnchor":"n","items":{"$dynamicRef":"#n"}})", .target = js::dialect::draft7, .expected = "!"},
    {.source = R"({"$dynamicAnchor":"n","items":{"$dynamicRef":"#n"}})", .target = js::dialect::draft4, .expected = "!"},
    {.source = R"({"$dynamicAnchor":"n","items":{"$dynamicRef":"#n"}})", .target = js::dialect::openapi3_0, .expected = "!"},
    // dependent_*
    {.source = R"({"dependentRequired":{"a":["b"]},"dependentSchemas":{"c":{"required":["d"]}}})",
     .target = js::dialect::draft2019_09,
     .expected = R"({"dependentRequired":{"a":["b"]},"dependentSchemas":{"c":{"required":["d"]}}})"},
    {.source = R"({"dependentRequired":{"a":["b"]},"dependentSchemas":{"c":{"required":["d"]}}})", .target = js::dialect::draft7,
     .expected = R"({"dependencies":{"a":["b"],"c":{"required":["d"]}}})"},
    {.source = R"({"dependentRequired":{"a":["b"]},"dependentSchemas":{"c":{"required":["d"]}}})", .target = js::dialect::draft4,
     .expected = R"({"dependencies":{"a":["b"],"c":{"required":["d"]}}})"},
    {.source = R"({"dependentRequired":{"a":["b"]}})", .target = js::dialect::openapi3_0, .expected = "!"},
    // type
    {.source = R"({"type":["string","null"]})", .target = js::dialect::draft4, .expected = R"({"type":["string","null"]})"},
    {.source = R"({"type":"integer"})", .target = js::dialect::draft4, .expected = "!"},
    {.source = R"({"type":["string","null"]})", .target = js::dialect::openapi3_0, .expected = R"({"type":"string","nullable":true})"},
    {.source = R"({"type":["string","integer"]})", .target = js::dialect::openapi3_0,
     .expected = R"({"anyOf":[{"type":"string"},{"type":"integer"}]})"},
    {.source = R"({"type":"null"})", .target = js::dialect::openapi3_0, .expected = "!"},
    // const
    {.source = R"({"const":1})", .target = js::dialect::draft6, .expected = R"({"const":1})"},
    {.source = R"({"const":1})", .target = js::dialect::draft4, .expected = R"({"enum":[1]})"},
    {.source = R"({"const":1})", .target = js::dialect::openapi3_0, .expected = R"({"enum":[1]})"},
    // unevaluated
    {.source = R"({"unevaluatedProperties":false})", .target = js::dialect::draft2019_09, .expected = R"({"unevaluatedProperties":false})"},
    {.source = R"({"unevaluatedProperties":false})", .target = js::dialect::draft7, .expected = "!"},
    {.source = R"({"unevaluatedProperties":false})", .target = js::dialect::draft4, .expected = "!"},
    {.source = R"({"unevaluatedProperties":false})", .target = js::dialect::openapi3_0, .expected = "!"},
    // boolean nodes
    {.source = R"({"properties":{"a":false,"b":true}})", .target = js::dialect::draft6, .expected = R"({"properties":{"a":false,"b":true}})"},
    {.source = R"({"properties":{"a":false,"b":true}})", .target = js::dialect::draft4, .expected = R"({"properties":{"a":{"not":{}},"b":{}}})"},
    {.source = R"({"properties":{"a":false,"b":true}})", .target = js::dialect::openapi3_0,
     .expected = R"({"properties":{"a":{"not":{}},"b":{}}})"},
    // contains, if, propertyNames
    {.source = R"({"contains":{"type":"string"}})", .target = js::dialect::draft6, .expected = R"({"contains":{"type":"string"}})"},
    {.source = R"({"contains":{"type":"string"}})", .target = js::dialect::draft4, .expected = "!"},
    {.source = R"({"contains":{"type":"string"}})", .target = js::dialect::openapi3_0, .expected = "!"},
    {.source = R"({"if":{"type":"string"},"then":{"minLength":1}})", .target = js::dialect::draft7,
     .expected = R"({"if":{"type":"string"},"then":{"minLength":1}})"},
    {.source = R"({"if":{"type":"string"},"then":{"minLength":1}})", .target = js::dialect::draft6, .expected = "!"},
    {.source = R"({"if":{"type":"string"},"then":{"minLength":1}})", .target = js::dialect::draft4, .expected = "!"},
    {.source = R"({"propertyNames":{"maxLength":3}})", .target = js::dialect::draft4, .expected = "!"},
    {.source = R"({"propertyNames":{"maxLength":3}})", .target = js::dialect::openapi3_0, .expected = "!"},
    // resource keywords
    {.source = R"({"$id":"urn:x","$anchor":"a","type":"string"})", .target = js::dialect::draft7,
     .expected = R"({"$id":"urn:x","definitions":{},"type":"string"})"},
    {.source = R"({"$id":"urn:x","type":"string"})", .target = js::dialect::draft4, .expected = R"({"id":"urn:x","type":"string"})"},
    // annotations and unknown keywords
    {.source = R"({"title":"T","x-note":1,"foo":2})", .target = js::dialect::openapi3_0, .expected = "!"},
}};

void every_form() {
    for (const cell& c : cells_other) {
        BOOST_TEST_EQ(emitted(c.source, c.target), std::string(c.expected));
    }
    BOOST_TEST_EQ(emitted(R"({"title":"T","x-note":1,"foo":2})", js::dialect::openapi3_0, js::unrepresentable::drop),
                  R"({"title":"T","x-note":1,"x-webcpp-dropped":["foo"]})");
    BOOST_TEST_EQ(emitted(R"({"type":"object","unevaluatedProperties":false})", js::dialect::draft7, js::unrepresentable::drop),
                  R"({"type":"object","x-webcpp-dropped":["unevaluatedProperties"]})");
    BOOST_TEST_EQ(emitted(R"({"examples":[1],"$comment":"c"})", js::dialect::openapi3_0, js::unrepresentable::drop),
                  R"({"x-webcpp-dropped":["examples","$comment"]})");
    const auto document = validator.load(boost::json::parse(R"({"$defs":{"a":{"type":"string"}},"$ref":"#/$defs/a"})"));
    const auto components = js::emit(*document, js::dialect::openapi3_0, {.write_schema_keyword = false});
    BOOST_TEST_EQ(boost::json::serialize(components->components), R"({"a":{"type":"string"}})");
}
```

  The `$anchor` row: an anchor in drafts 6 and 7 is a plain-name `$id` on
  its node. When that node is the root, which already carries its `$id`,
  the anchor moves into an empty `definitions` entry. That is why the row
  writes `"definitions":{}`.
  - The precise placement is the writer's to settle in Step 3.
  - The rule the row pins is that the anchor is not lost: the emitted text
    is re-read in Step 3, and the golden is corrected to the writer's
    settled placement in the same commit, with the reason in the ledger.

  Call `every_form()` from `main` under the same
  `#ifdef WEBCPP_TEST_JSON_SCHEMA_WITH_REFERENCES` for its `$ref` rows: the
  rows without `$ref` run unconditionally, so split them into two arrays
  when writing. The `user` row joins with user vocabularies, and the
  recursive `dynamic_ref` row with 2019-09.
- [ ] **Step 2: Run it and watch it fail.** `$B2 libs/json_schema/test//emitter`.
  Expected: FAIL, each target written in 2020-12's forms.
- [ ] **Step 3: Implement** `forms.hpp`, per target dialect: one writer for
  each row of spec 11.1, plus the rules above. Draft 4's boolean nodes
  become `{}` and `{"not": {}}` everywhere, the root included.
- [ ] **Step 4: Run it and watch it pass** on the five lanes:

```bash
lanes libs/json_schema/test
lint_clean
```

  Expected: `lanes: green`, `lint: clean`.
- [ ] **Step 5: Commit.**

```bash
cd libs/json_schema
F="include/webcpp/json_schema/detail/emitter/forms.hpp include/webcpp/json_schema/detail/emitter/writer.hpp \
test/emitter_test.cpp"
git add $F
$G commit -m "json_schema: each dialect's forms in the emitter, OpenAPI 3.0 included, both unrepresentable modes" -- $F
```

### Task E4.4: E4 complete: the round trip, the page, the merge

After E4.3, and after E2 is merged into the canonical `main`.

**Files:**
- Create:
  - `test/suite/suite_draft2020_12_round_trip.cpp`;
  - `test/known_failures/draft2020-12-round-trip.json`;
  - `doc/emitting.adoc`;
  - `example/json_schema_emit.cpp` with its `.expected`.
- Modify:
  - `test/suite/runner.hpp` (the round-trip mode);
  - `test/equivalent_test.cpp`, `test/emitter_test.cpp` (the conditions
    removed);
  - `test/Jamfile`, `example/Jamfile`, `doc/json_schema.adoc`.

- [ ] **Step 1: Rebase on E2, and remove the two conditions.**

```bash
cd ~/dev/webcpp-json-schema-e4
git -C libs/json_schema fetch "$CANONICAL" main
$G -C libs/json_schema rebase FETCH_HEAD
$B2 libs/json_schema/test//equivalent libs/json_schema/test//emitter
```

  Expected: the conflicts are in `test/Jamfile`,
  `meta/include-boundaries.json`, and `test/suite/runner.hpp` when E3
  merged first. Each is resolved by keeping both sides. Then both tests
  pass with their `$ref` cases. A failing `$ref` case is the emitter's to
  fix.
- [ ] **Step 2: The round trip (spec 16.4).** `run_options` gains
  `std::optional<webcpp::json_schema::dialect> round_trip;`, appended. In
  `run_case`, after the schema compiled:

```cpp
std::string why_case;   // set when the case's round trip broke: each of its tests fails with it
std::optional<decltype(validator.compile(c.at("schema"), compile))> reloaded;
if (options.round_trip && schema) {
    const auto out = webcpp::json_schema::emit(schema->ir(), *options.round_trip);
    if (!out) {
        why_case = "emit failed";
    } else {
        auto again = compile;
        again.default_dialect = options.round_trip;
        reloaded.emplace(validator.compile(out->schema, again));
        if (!*reloaded) {
            why_case = "reload refused";
        } else if (!webcpp::json_schema::ir::equivalent(schema->ir(), schema->ir().root(), (*reloaded)->ir(),
                                                        (*reloaded)->ir().root())) {
            why_case = "not equivalent";
        }
    }
}
```

  Each test of a case then:
  - fails with `why_case` when it is set;
  - otherwise validates with `(reloaded && *reloaded) ? **reloaded :
    *schema` and compares with the original's verdict.

  The program:

```cpp
#include "runner.hpp"

#include <webcpp/json_schema.hpp>

#include <string_view>

namespace js = webcpp::json_schema;

int main(int argc, char** argv) {
    const js::validator_for<js::draft2020_12> validator;
    webcpp::test::suite::run_options options{.root = WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE, .dialect = "draft2020-12",
                                             .known_failures = WEBCPP_TEST_JSON_SCHEMA_KNOWN,
                                             .record = argc > 1 && std::string_view(argv[1]) == "--record"};
    options.round_trip = js::dialect::draft2020_12;
    return webcpp::test::suite::run(validator, options);
}
```

  Append `suite-runner draft2020-12-round-trip :
  suite/suite_draft2020_12_round_trip.cpp ;` to `test/Jamfile`, create the
  list as `[]`, and run it. Expected: an unexpected failure for each `E5:`
  test. Every other case round-trips, by construction (section E4's rules);
  a case that does not is the emitter's or `equivalent`'s to fix. Then:

```bash
record_known draft2020-12-round-trip
python3 -c "import json; print(sorted({e['reason'] for e in json.load(open('libs/json_schema/test/known_failures/draft2020-12-round-trip.json'))}))"
```

  Expected: `['E5: formats', 'E5: other dialects']`.
- [ ] **Step 3: The page and its example.** `example/json_schema_emit.cpp`:

```cpp
#include <webcpp/json_schema.hpp>

#include <boost/json.hpp>

#include <iostream>

namespace js = webcpp::json_schema;

int main() {
    // tag::emit[]
    const js::validator validator;
    const auto document = validator.load(boost::json::parse(R"({
        "prefixItems": [{"type": "string"}],
        "items": {"type": "integer"},
        "unevaluatedItems": false
    })"));
    for (const js::dialect target : {js::dialect::draft2020_12, js::dialect::draft7}) {
        const auto out = js::emit(*document, target,
                                  {.on_unrepresentable = js::unrepresentable::drop, .write_schema_keyword = false});
        std::cout << boost::json::serialize(out->schema) << '\n';
    }
    // end::emit[]
}
```

  `example/json_schema_emit.expected`:

```
{"prefixItems":[{"type":"string"}],"items":{"type":"integer"},"unevaluatedItems":false}
{"items":[{"type":"string"}],"additionalItems":{"type":"integer"},"x-webcpp-dropped":["unevaluatedItems"]}
```

  `doc/emitting.adoc` (spec 18.10) holds:
  - `emit`, its options and the unrepresentable modes;
  - the forms table;
  - the rules of section E4, equivalence's included;
  - this example.

  OpenAPI 3.0's half joins with its loader.

```bash
$B2 -a --build-dir=bin/doc libs/json_schema/doc
lanes libs/json_schema/test libs/json_schema/example
lint_clean
```

  Expected: exit 0, `lanes: green`, `lint: clean`.
- [ ] **Step 4: Commit,** before the merge:

```bash
cd libs/json_schema
F="test/Jamfile test/suite/runner.hpp test/suite/suite_draft2020_12_round_trip.cpp \
test/known_failures/draft2020-12-round-trip.json test/equivalent_test.cpp test/emitter_test.cpp \
doc/emitting.adoc doc/json_schema.adoc example/Jamfile example/json_schema_emit.cpp example/json_schema_emit.expected"
git add $F
$G commit -m "json_schema: every 2020-12 suite schema round-trips through the emitter, verdicts unchanged" -- $F
```

- [ ] **Step 5: The merge** (controller).

```bash
cd ~/dev/webcpp-json-schema-e4
merge_track 4
```

  Expected: `lanes: green`, `lint: clean`, a fast-forward, and no frozen
  file changed.

## E5: every other dialect, formats, vocabularies, the proofs and the page

In `~/dev/webcpp-json-schema-e5`, the library on `e5`, created once E2, E3
and E4 are merged into the canonical `main` (the E2 to E5 review's I1):

```bash
git -C "$CANONICAL" log --oneline -3 main   # E4's merge on top
track_worktree 5
cd ~/dev/webcpp-json-schema-e5
lanes libs/json_schema/test libs/json_schema/example
```

  Expected: the three merges in the log, then git's "Switched to a new
  branch 'e5'", then `lanes: green`.

**The serial base** (the E2 to E5 review's C1 and I10; ruling 2026-10-09).
E5.0a and E5.0b give every dialect its machinery, widen `validator` to spec
4.5's six, add `run_options::default_dialect`, and create every dialect's
runner with its first list. Only then do E5.1 to E5.8 run at once, each from
`task_worktree 5 <slug>`. So every program they write compiles in its own
worktree, `js::validator` included.

**Merging the parallel group.** The controller merges E5.1 to E5.8 into
`e5` in that order, with `merge_e5 <slug>`. After each merge, every list is
recorded again and may only shrink: a test that a merge breaks is a bug of
that merge, never a new list entry.

```bash
LISTS="draft2020-12 draft2020-12-output draft2020-12-round-trip draft2019-09 draft7 draft6 draft4 \
openapi3-0 draft2019-09-round-trip draft7-round-trip draft6-round-trip draft4-round-trip openapi3-0-round-trip"

record_all() {   # every existing list recorded again; fails when one gains an entry
  local l
  for l in $LISTS; do
    [ -f "libs/json_schema/test/known_failures/$l.json" ] || continue
    cp "libs/json_schema/test/known_failures/$l.json" "/tmp/before-$l.json"
    record_known "$l" || return 1
    python3 - "$l" <<'EOF' || return 1
import json
import sys

name = sys.argv[1]
def key(e):
    return (e["file"], e["case"], e["test"])
before = {key(e) for e in json.load(open(f"/tmp/before-{name}.json"))}
after = {key(e) for e in json.load(open(f"libs/json_schema/test/known_failures/{name}.json"))}
for gained in sorted(after - before):
    print(f"record_all: {name} gained {gained}")
sys.exit(1 if after - before else 0)
EOF
  done
}

merge_e5() {     # merge_e5 <slug>: a parallel task into e5, then every list recorded again
  merge_task 5 "$1" || return 1
  record_all || return 1
  lanes libs/json_schema/test libs/json_schema/example || return 1
  lint_clean || return 1
  local F
  F=$(git -C libs/json_schema status --porcelain -- test/known_failures | awk '{print $2}' | tr '\n' ' ')
  [ -z "$F" ] && return 0
  (cd libs/json_schema && git add $F && $G commit -m "json_schema: the known-failure lists after $1" -- $F)
}
```

  `merge_e5` prints `lanes: green`, then `lint: clean`, then git's commit
  line when a list shrank. A `record_all: <list> gained` line stops it.

**Opcodes reserved in advance**, so that tasks running at once never take
the same value: `assert_format = 36` (E5.5), `call_user = 37` (E5.6),
`assert_content = 38` (E5.9), `assert_enum_hash = 39` (E5.12),
`assert_type_and_guard = 40` (E5.13). Each task appends its own line to
`opcode.hpp`; a rebase conflict there keeps both lines, in value order.

**Reasons in E5.** E5.0b appends a rule to `reason_for`: a failure in a
runner of another dialect than 2020-12, with no other reason, is
`E5: <dialect>` (`E5: draft2019-09`, `E5: draft7`, `E5: draft6`,
`E5: draft4`, `E5: openapi3-0`). Each dialect's task leaves no entry of its
own reason, and E5.18 removes the rule and every `E5:` reason with it.

### Task E5.0a: The machinery of 2019-09, draft 7 and draft 6

Runs first, alone.

**Files:**
- Create:
  - `include/webcpp/json_schema/detail/dialects/draft2019_09.hpp`,
    `detail/dialects/draft7.hpp`, `detail/dialects/draft6.hpp`,
    `detail/dialects/rule.hpp`;
  - `test/loader_draft2019_09_test.cpp`, `test/loader_draft7_test.cpp`,
    `test/loader_draft6_test.cpp`.
- Modify:
  - `include/webcpp/json_schema/validator/validator.hpp` (`reader` gains
    its new values, appended; `dialect_rules`);
  - `include/webcpp/json_schema/detail/dialects/draft2020_12.hpp` (its
    `rules`);
  - `include/webcpp/json_schema/detail/loader/walk.hpp`,
    `detail/loader/readers.hpp`;
  - `test/Jamfile`.

**Interfaces:**
- Each `dialect_machinery<D>` gains `static constexpr dialect_rules rules`,
  what its resources and readers depend on:

```cpp
namespace webcpp::json_schema::detail {
/** How a dialect spells its resources, and the readings that differ between dialects. */
struct dialect_rules {
    std::string_view id;               // "$id", or "id" in draft 4; empty in OpenAPI 3.0
    bool fragment_id_is_anchor;        // "$id": "#name" is a plain anchor: drafts 4 (id), 6 and 7
    bool anchor_keyword;               // $anchor: 2019-09 and 2020-12
    bool dynamic_anchor_keyword;       // $dynamicAnchor: 2020-12
    bool recursive_anchor_keyword;     // $recursiveAnchor: 2019-09
    bool vocabulary_keyword;           // $vocabulary: 2019-09 and 2020-12
    bool ref_hides_siblings;           // drafts 4, 6, 7 and OpenAPI 3.0 (spec 6.2)
    bool boolean_schemas;              // every dialect but draft 4 and OpenAPI 3.0
    bool integer_by_value;             // type integer admits 1.0: every dialect but draft 4
    bool contains_annotates_items;     // 2020-12 only
};
/** A keyword_rule, written as a call so that a table row stays one line. */
constexpr keyword_rule rule(std::string_view name, ir::keyword_kind kind, reader read,
                            std::uint16_t vocabulary) noexcept {
    return {.name = name, .kind = kind, .read = read, .vocabulary = vocabulary};
}
}
```

  `rule` lives in `detail/dialects/rule.hpp`, with `dialect_rules`.

| Dialect | `id` | fragment anchor | `$anchor` | `$dynamicAnchor` | `$recursiveAnchor` | `$vocabulary` | `$ref` hides | booleans | integer by value | contains annotates |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 2020-12 | `$id` | no | yes | yes | no | yes | no | yes | yes | yes |
| 2019-09 | `$id` | no | yes | no | yes | yes | no | yes | yes | no |
| 7, 6 | `$id` | yes | no | no | no | no | yes | yes | yes | no |
| 4 | `id` | yes | no | no | no | no | yes | no | no | no |
| OpenAPI 3.0 | none | no | no | no | no | no | yes | no | yes | no |

- `reader` gains, appended:
  - `items_2019`: an array is `prefix_items` named `items`, a schema is
    `items`;
  - `additional_items`: beside an array `items` it is `items` named
    `additionalItems`; beside a schema `items`, or alone, it is `ignored`;
  - `recursive_ref`: `$recursiveRef`, read as `dynamic_ref` with `form =
    recursive`, `anchor = ""`, bookended when its initial target is a
    resource root with `"$recursiveAnchor": true`;
  - `bound_draft4`, `exclusive_flag`, `nonempty_names`, `type_single`,
    `nullable`, `boolean_or_schema`, used by E5.0b.
- The walker reads the resource keywords through `rules`. With
  `ref_hides_siblings`, a node holding `$ref` loads `$ref` and every
  sibling as `ignored`, `$id` and `definitions` included; a pointer into a
  sibling still resolves, through the on-demand loading of references.
- A reader may give another kind than its rule's: `read_payload` returns
  `result<read_result>`, `struct read_result { ir::keyword_kind kind;
  ir::payload data; };`.
- `validator` stays `validator_for<draft2020_12>` here; E5.0b widens it.

- [ ] **Step 1: Write the failing tests.**

`test/loader_draft2019_09_test.cpp`:

```cpp
#include <webcpp/json_schema.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/json/parse.hpp>

#include <string>

namespace js = webcpp::json_schema;
namespace ir = js::ir;
using k = ir::keyword_kind;

namespace {

const js::validator_for<js::draft2020_12, js::draft2019_09, js::draft7, js::draft6> validator;

js::result<js::ir::document> load(std::string_view text) {
    return validator.load(boost::json::parse(text), {.default_dialect = js::dialect::draft2019_09});
}

const ir::keyword* find(const js::ir::document& d, k kind) {
    return d.find(d.root(), kind);
}

void items_forms() {
    const auto array = load(R"({"items": [true], "additionalItems": false})");
    BOOST_TEST_EQ(array->string(find(*array, k::prefix_items)->name), "items");
    BOOST_TEST_EQ(array->string(find(*array, k::items)->name), "additionalItems");
    const auto schema = load(R"({"items": {}, "additionalItems": false})");
    BOOST_TEST(find(*schema, k::prefix_items) == nullptr);
    BOOST_TEST(find(*schema, k::ignored) != nullptr);
    BOOST_TEST(find(*load(R"({"additionalItems": false})"), k::ignored) != nullptr);
}

void recursion() {
    const auto d = load(R"({"$recursiveAnchor": true, "items": {"$recursiveRef": "#"}})");
    BOOST_TEST(d->resources()[0].recursive_anchor);
    const auto items = std::get<ir::subschema>(find(*d, k::items)->data);
    const auto& payload = std::get<ir::dynamic_reference>(d->find(items.schema, k::dynamic_ref)->data);
    BOOST_TEST(payload.form == ir::dynamic_form::recursive);
    BOOST_TEST(payload.bookended);
    BOOST_TEST_EQ(d->string(payload.anchor), "");
}

void dialect_fields() {
    const auto d = load(R"({"contains": true, "prefixItems": [true], "$dynamicRef": "#a"})");
    BOOST_TEST(!std::get<ir::contains_payload>(find(*d, k::contains)->data).annotates_items);
    BOOST_TEST(find(*d, k::prefix_items) == nullptr);   // not 2019-09's: unknown, which annotates
    BOOST_TEST(find(*d, k::dynamic_ref) == nullptr);
    std::size_t unknown = 0;
    for (const auto& keyword : d->keywords(d->root())) {
        unknown += keyword.kind == k::unknown ? 1 : 0;
    }
    BOOST_TEST_EQ(unknown, 2U);
    BOOST_TEST_EQ(js::detail::dialect_machinery<js::draft2019_09>::keywords().size(), 52U);
}

void schema_uri_spellings() {
    for (const char* uri : {"https://json-schema.org/draft/2019-09/schema",
                            "https://json-schema.org/draft/2019-09/schema#"}) {
        const auto d = validator.load(boost::json::parse(std::string(R"({"$schema": ")") + uri + "\"}"));
        BOOST_TEST(d.has_value() && d->resources()[0].source_dialect == js::dialect::draft2019_09);
    }
    const auto slash =
        validator.load(boost::json::parse(R"({"$schema": "https://json-schema.org/draft/2019-09/schema/"})"));
    BOOST_TEST(slash.error().code == js::errc::unknown_dialect);
    BOOST_TEST_EQ(std::string(slash.error().what), "https://json-schema.org/draft/2019-09/schema/");
}

}  // namespace

int main() {
    items_forms();
    recursion();
    dialect_fields();
    schema_uri_spellings();
    return boost::report_errors();
}
```

`test/loader_draft7_test.cpp`:

```cpp
#include <webcpp/json_schema.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/json/parse.hpp>

#include <array>
#include <string>
#include <utility>

namespace js = webcpp::json_schema;
namespace ir = js::ir;
using k = ir::keyword_kind;

namespace {

const js::validator_for<js::draft2020_12, js::draft2019_09, js::draft7, js::draft6> validator;

js::result<js::ir::document> load(std::string_view text, js::dialect d = js::dialect::draft7) {
    return validator.load(boost::json::parse(text), {.default_dialect = d});
}

const ir::keyword* find(const js::ir::document& d, k kind) {
    return d.find(d.root(), kind);
}

std::size_t count(const js::ir::document& d, k kind) {
    std::size_t n = 0;
    for (const auto& keyword : d.keywords(d.root())) {
        n += keyword.kind == kind ? 1 : 0;
    }
    return n;
}

void ref_hides_its_siblings() {
    const auto d = load(R"({"$ref": "#/definitions/a", "minimum": 5, "$id": "http://other/",
                            "definitions": {"a": {"type": "integer"}}})");
    BOOST_TEST(find(*d, k::ref) != nullptr);
    BOOST_TEST(find(*d, k::minimum) == nullptr);
    BOOST_TEST_EQ(count(*d, k::ignored), 3U);   // minimum, $id, definitions
    BOOST_TEST_EQ(d->resources().size(), 1U);   // the sibling $id opens no resource
}

void plain_name_ids_are_anchors() {
    const auto d = load(R"({"allOf": [{"$ref": "#foo"}], "definitions": {"a": {"$id": "#foo", "type": "string"}}})");
    BOOST_TEST(d.has_value());
    BOOST_TEST_EQ(d->anchors().size(), 1U);
}

void dependencies_split() {
    const auto d = load(R"({"dependencies": {"a": ["b"], "c": {"required": ["d"]}}})");
    BOOST_TEST_EQ(d->string(find(*d, k::dependent_required)->name), "dependencies");
    BOOST_TEST_EQ(d->string(find(*d, k::dependent_schemas)->name), "dependencies");
    BOOST_TEST_EQ(js::detail::dialect_machinery<js::draft7>::keywords().size(), 44U);
}

void contains_if_and_unknown() {
    BOOST_TEST(!std::get<ir::contains_payload>(find(*load(R"({"contains": true})"), k::contains)->data)
                    .annotates_items);
    BOOST_TEST(find(*load(R"({"if": true, "then": false})"), k::if_) != nullptr);
    BOOST_TEST(find(*load(R"({"foo": 1})"), k::ignored) != nullptr);   // inert in drafts 4 to 7 (PD2)
}

void schema_uri_spellings() {
    const std::array<std::pair<const char*, js::dialect>, 4> official{{
        {"http://json-schema.org/draft-07/schema#", js::dialect::draft7},
        {"http://json-schema.org/draft-07/schema", js::dialect::draft7},
        {"http://json-schema.org/draft-06/schema#", js::dialect::draft6},
        {"http://json-schema.org/draft-06/schema", js::dialect::draft6},
    }};
    for (const auto& [uri, dialect] : official) {
        const auto d = validator.load(boost::json::parse(std::string(R"({"$schema": ")") + uri + "\"}"));
        BOOST_TEST(d.has_value() && d->resources()[0].source_dialect == dialect);
    }
    const auto https = validator.load(boost::json::parse(R"({"$schema": "https://json-schema.org/draft-07/schema#"})"));
    BOOST_TEST(https.error().code == js::errc::unknown_dialect);
    BOOST_TEST_EQ(std::string(https.error().what), "https://json-schema.org/draft-07/schema#");
}

}  // namespace

int main() {
    ref_hides_its_siblings();
    plain_name_ids_are_anchors();
    dependencies_split();
    contains_if_and_unknown();
    schema_uri_spellings();
    return boost::report_errors();
}
```

`test/loader_draft6_test.cpp` is the draft 7 test with `load`'s default
`js::dialect::draft6`, and with `contains_if_and_unknown` replaced by:

```cpp
void what_draft6_lacks() {
    for (const char* keyword : {"if", "then", "else", "$comment", "contentEncoding", "contentMediaType",
                                "readOnly", "writeOnly"}) {
        const auto d = load(std::string(R"({")") + keyword + R"(": "x"})");
        BOOST_TEST(find(*d, k::ignored) != nullptr);   // no keyword of draft 6: inert (PD2)
    }
    BOOST_TEST(find(*load(R"({"const": 1})"), k::const_) != nullptr);
    BOOST_TEST(find(*load(R"({"contains": true})"), k::contains) != nullptr);
    BOOST_TEST(find(*load(R"({"propertyNames": true})"), k::property_names) != nullptr);
    BOOST_TEST(find(*load(R"({"examples": []})"), k::examples) != nullptr);
    BOOST_TEST_EQ(js::detail::dialect_machinery<js::draft6>::keywords().size(), 36U);
}
```

  Its `dependencies_split` drops draft 7's count line; `what_draft6_lacks` holds draft 6's.

  Append to `test/Jamfile`:

```
webcpp.run loader_draft2019_09 : loader_draft2019_09_test.cpp ;
webcpp.run loader_draft7 : loader_draft7_test.cpp ;
webcpp.run loader_draft6 : loader_draft6_test.cpp ;
```

- [ ] **Step 2: Run them and watch them fail.**
  `$B2 libs/json_schema/test//loader_draft2019_09 libs/json_schema/test//loader_draft7 libs/json_schema/test//loader_draft6`.
  Expected: FAIL, `implicit instantiation of undefined template
  'dialect_machinery<webcpp::json_schema::draft2019_09>'`, and the same for
  drafts 7 and 6.
- [ ] **Step 3: Implement.** The three tables, each in its header.

`detail/dialects/draft2019_09.hpp`:

```cpp
template <>
struct dialect_machinery<draft2019_09> {
    static constexpr dialect_rules rules{.id = "$id", .fragment_id_is_anchor = false, .anchor_keyword = true,
                                         .dynamic_anchor_keyword = false, .recursive_anchor_keyword = true,
                                         .vocabulary_keyword = true, .ref_hides_siblings = false,
                                         .boolean_schemas = true, .integer_by_value = true,
                                         .contains_annotates_items = false};
    static std::span<const keyword_rule> keywords() noexcept {
        using k = ir::keyword_kind;
        using r = reader;
        namespace v = ir::vocabulary_bit;
        static constexpr std::array<keyword_rule, 52> table{{
            rule("$ref", k::ref, r::ref, v::core),
            rule("$recursiveRef", k::dynamic_ref, r::recursive_ref, v::core),
            rule("$defs", k::defs, r::schema_map, v::core),
            rule("$comment", k::comment, r::annotation, v::core),
            rule("allOf", k::all_of, r::nonempty_schema_list, v::applicator),
            rule("anyOf", k::any_of, r::nonempty_schema_list, v::applicator),
            rule("oneOf", k::one_of, r::nonempty_schema_list, v::applicator),
            rule("not", k::not_, r::schema, v::applicator),
            rule("if", k::if_, r::schema, v::applicator),
            rule("then", k::then_, r::schema, v::applicator),
            rule("else", k::else_, r::schema, v::applicator),
            rule("dependentSchemas", k::dependent_schemas, r::schema_map, v::applicator),
            rule("items", k::items, r::items_2019, v::applicator),
            rule("additionalItems", k::items, r::additional_items, v::applicator),
            rule("contains", k::contains, r::contains, v::applicator),
            rule("properties", k::properties, r::schema_map, v::applicator),
            rule("patternProperties", k::pattern_properties, r::pattern_map, v::applicator),
            rule("additionalProperties", k::additional_properties, r::schema, v::applicator),
            rule("propertyNames", k::property_names, r::schema, v::applicator),
            rule("unevaluatedItems", k::unevaluated_items, r::schema, v::applicator),
            rule("unevaluatedProperties", k::unevaluated_properties, r::schema, v::applicator),
            rule("type", k::type, r::types, v::validation),
            rule("enum", k::enum_, r::constants, v::validation),
            rule("const", k::const_, r::constant, v::validation),
            rule("multipleOf", k::multiple_of, r::positive_bound, v::validation),
            rule("maximum", k::maximum, r::bound, v::validation),
            rule("exclusiveMaximum", k::exclusive_maximum, r::bound, v::validation),
            rule("minimum", k::minimum, r::bound, v::validation),
            rule("exclusiveMinimum", k::exclusive_minimum, r::bound, v::validation),
            rule("maxLength", k::max_length, r::count, v::validation),
            rule("minLength", k::min_length, r::count, v::validation),
            rule("pattern", k::pattern, r::pattern, v::validation),
            rule("maxItems", k::max_items, r::count, v::validation),
            rule("minItems", k::min_items, r::count, v::validation),
            rule("uniqueItems", k::unique_items, r::constant, v::validation),
            rule("maxContains", k::max_contains, r::count, v::validation),
            rule("minContains", k::min_contains, r::count, v::validation),
            rule("maxProperties", k::max_properties, r::count, v::validation),
            rule("minProperties", k::min_properties, r::count, v::validation),
            rule("required", k::required, r::names, v::validation),
            rule("dependentRequired", k::dependent_required, r::dependent_required, v::validation),
            rule("title", k::title, r::annotation, v::meta_data),
            rule("description", k::description, r::annotation, v::meta_data),
            rule("default", k::default_, r::annotation, v::meta_data),
            rule("deprecated", k::deprecated, r::annotation, v::meta_data),
            rule("readOnly", k::read_only, r::annotation, v::meta_data),
            rule("writeOnly", k::write_only, r::annotation, v::meta_data),
            rule("examples", k::examples, r::annotation, v::meta_data),
            rule("format", k::format, r::format, v::format_annotation),
            rule("contentEncoding", k::content_encoding, r::text, v::content),
            rule("contentMediaType", k::content_media_type, r::text, v::content),
            rule("contentSchema", k::content_schema, r::schema, v::content),
        }};
        return table;
    }
};
```

`detail/dialects/draft7.hpp`. `definitions` and `dependencies` are read as
E2.7 reads them under `compatibility_keywords`, here as the dialect's own:

```cpp
template <>
struct dialect_machinery<draft7> {
    static constexpr dialect_rules rules{.id = "$id", .fragment_id_is_anchor = true, .anchor_keyword = false,
                                         .dynamic_anchor_keyword = false, .recursive_anchor_keyword = false,
                                         .vocabulary_keyword = false, .ref_hides_siblings = true,
                                         .boolean_schemas = true, .integer_by_value = true,
                                         .contains_annotates_items = false};
    static std::span<const keyword_rule> keywords() noexcept {
        using k = ir::keyword_kind;
        using r = reader;
        namespace v = ir::vocabulary_bit;
        static constexpr std::array<keyword_rule, 44> table{{
            rule("$ref", k::ref, r::ref, v::core),
            rule("definitions", k::defs, r::schema_map, v::core),
            rule("$comment", k::comment, r::annotation, v::core),
            rule("allOf", k::all_of, r::nonempty_schema_list, v::applicator),
            rule("anyOf", k::any_of, r::nonempty_schema_list, v::applicator),
            rule("oneOf", k::one_of, r::nonempty_schema_list, v::applicator),
            rule("not", k::not_, r::schema, v::applicator),
            rule("if", k::if_, r::schema, v::applicator),
            rule("then", k::then_, r::schema, v::applicator),
            rule("else", k::else_, r::schema, v::applicator),
            rule("items", k::items, r::items_2019, v::applicator),
            rule("additionalItems", k::items, r::additional_items, v::applicator),
            rule("contains", k::contains, r::contains, v::applicator),
            rule("properties", k::properties, r::schema_map, v::applicator),
            rule("patternProperties", k::pattern_properties, r::pattern_map, v::applicator),
            rule("additionalProperties", k::additional_properties, r::schema, v::applicator),
            rule("propertyNames", k::property_names, r::schema, v::applicator),
            rule("dependencies", k::dependent_schemas, r::dependencies, v::applicator),
            rule("type", k::type, r::types, v::validation),
            rule("enum", k::enum_, r::constants, v::validation),
            rule("const", k::const_, r::constant, v::validation),
            rule("multipleOf", k::multiple_of, r::positive_bound, v::validation),
            rule("maximum", k::maximum, r::bound, v::validation),
            rule("exclusiveMaximum", k::exclusive_maximum, r::bound, v::validation),
            rule("minimum", k::minimum, r::bound, v::validation),
            rule("exclusiveMinimum", k::exclusive_minimum, r::bound, v::validation),
            rule("maxLength", k::max_length, r::count, v::validation),
            rule("minLength", k::min_length, r::count, v::validation),
            rule("pattern", k::pattern, r::pattern, v::validation),
            rule("maxItems", k::max_items, r::count, v::validation),
            rule("minItems", k::min_items, r::count, v::validation),
            rule("uniqueItems", k::unique_items, r::constant, v::validation),
            rule("maxProperties", k::max_properties, r::count, v::validation),
            rule("minProperties", k::min_properties, r::count, v::validation),
            rule("required", k::required, r::names, v::validation),
            rule("format", k::format, r::format, v::format_annotation),
            rule("contentEncoding", k::content_encoding, r::text, v::content),
            rule("contentMediaType", k::content_media_type, r::text, v::content),
            rule("title", k::title, r::annotation, v::meta_data),
            rule("description", k::description, r::annotation, v::meta_data),
            rule("default", k::default_, r::annotation, v::meta_data),
            rule("readOnly", k::read_only, r::annotation, v::meta_data),
            rule("writeOnly", k::write_only, r::annotation, v::meta_data),
            rule("examples", k::examples, r::annotation, v::meta_data),
        }};
        return table;
    }
};
```

  - The tables hold 52 (2019-09), 44 (draft 7) and 36 (draft 6) rows.
    `loader_draft*_test` pins the count of each through
    `dialect_machinery<D>::keywords().size()`, since a `std::array` with
    fewer initializers than its size compiles.
  - `dependencies` goes through E2.7's path for it, which may add two IR
    keywords for one JSON keyword (`dependent_required` and
    `dependent_schemas`, both named `dependencies`); its row only makes it
    the dialect's own keyword.

`detail/dialects/draft6.hpp` is draft 7's header with `draft6` for `draft7`,
less the rows of `$comment`, `if`, `then`, `else`, `contentEncoding`,
`contentMediaType`, `readOnly` and `writeOnly` (36 rows), and the same
`rules`.

  2020-12's header gains its `rules`: `.id = "$id"`, `.anchor_keyword`,
  `.dynamic_anchor_keyword`, `.vocabulary_keyword`, `.boolean_schemas`,
  `.integer_by_value` and `.contains_annotates_items` true, the rest false.

The walker, in `walk.hpp`, reads the resource keywords through `rules` and
honours `ref_hides_siblings` before the table:

```cpp
template <class Traits>
result<void> walker<Traits>::walk_object(const pending& p, const boost::json::object& object) {
    const dialect_rules& r = rules_of(p.resource);   // the resource's dialect
    if (r.ref_hides_siblings && object.contains("$ref")) {
        return walk_ref_alone(p, object);   // $ref read; every sibling, resource keywords included, ignored
    }
    if (auto opened = read_resource_keywords(p, object, r); !opened) {
        return opened;
    }
    keywords_.clear();
    for (const auto& member : object) {
        if (auto read = read_member(p, member.key(), member.value(), r); !read) {
            return read;
        }
    }
    doc_.define(p.node, ir::node_kind::object, keywords_);
    return {};
}

template <class Traits>
result<void> walker<Traits>::walk_ref_alone(const pending& p, const boost::json::object& object) {
    keywords_.clear();
    for (const auto& member : object) {
        const std::uint32_t at = child_pointer(p.pointer, member.key());
        if (member.key() != "$ref") {
            add_ignored(member.key(), at);
            continue;
        }
        auto data = read_payload(*rule_of(p.resource, "$ref"), p, member.value(), at);
        if (!data) {
            return data.error();
        }
        keywords_.push_back({.kind = data->kind, .name = doc_.intern("$ref"),
                             .source = doc_.add_value_at(root_copy_, pointers_[at]), .data = data->data});
    }
    doc_.define(p.node, ir::node_kind::object, keywords_);
    return {};
}
```

  `walk_one` calls `walk_object` after its guards, and refuses a boolean
  with `not_a_schema` when `!rules_of(p.resource).boolean_schemas`.
  `read_resource_keywords` reads `rules.id` (a fragment-only value is an
  anchor when `fragment_id_is_anchor`, and `invalid_keyword_value`
  otherwise), then `$anchor`, `$dynamicAnchor` and `$recursiveAnchor` each
  only when its flag is set; `is_resource_keyword` asks the same flags. A
  resource keyword the dialect lacks is an unknown keyword, handled by
  `add_unknown` (PD2).

  The two new readers that look at a sibling, in `readers.hpp`:

```cpp
template <class Traits>
result<read_result> walker<Traits>::read_additional_items(const pending& p, const boost::json::value& v,
                                                          std::uint32_t at) {
    const boost::json::value* items = p.value->as_object().if_contains("items");
    if (items == nullptr || !items->is_array()) {
        return read_result{.kind = ir::keyword_kind::ignored, .data = ir::payload{}};   // spec 6.2: inert
    }
    auto schema = queue_schema(p, v, at);   // checks the shape, queues the child
    if (!schema) {
        return schema.error();
    }
    return read_result{.kind = ir::keyword_kind::items, .data = ir::subschema{.schema = *schema}};
}

template <class Traits>
result<read_result> walker<Traits>::read_items_2019(const pending& p, const boost::json::value& v,
                                                    std::uint32_t at) {
    if (!v.is_array()) {
        auto schema = queue_schema(p, v, at);
        if (!schema) {
            return schema.error();
        }
        return read_result{.kind = ir::keyword_kind::items, .data = ir::subschema{.schema = *schema}};
    }
    auto list = queue_schema_list(p, v.get_array(), at, /*nonempty=*/false);
    if (!list) {
        return list.error();
    }
    return read_result{.kind = ir::keyword_kind::prefix_items, .data = *list};
}
```

  `recursive_ref` reads a string, resolved as `$ref` is, and records
  `dynamic_reference{.form = dynamic_form::recursive, .anchor = <empty>,
  .bookended = <target is a resource root with recursive_anchor>}` once
  the references are resolved, as `$dynamicRef`'s `bookended` is. The
  compiler already refuses `form = recursive` with `invalid_ir`, until
  2019-09's runtime.
- [ ] **Step 4: Run them and watch them pass,** then the lanes:

```bash
$B2 libs/json_schema/test//loader_draft2019_09 libs/json_schema/test//loader_draft7 \
  libs/json_schema/test//loader_draft6
lanes libs/json_schema/test
lint_clean
```

  Expected: `**passed**` for the three, `lanes: green` (every earlier test
  unchanged: 2020-12 reads the same through its `rules`), `lint: clean`.
- [ ] **Step 5: Commit.**

```bash
cd libs/json_schema
F="include/webcpp/json_schema/validator/validator.hpp include/webcpp/json_schema/detail/dialects/rule.hpp \
include/webcpp/json_schema/detail/dialects/draft2020_12.hpp include/webcpp/json_schema/detail/dialects/draft2019_09.hpp \
include/webcpp/json_schema/detail/dialects/draft7.hpp include/webcpp/json_schema/detail/dialects/draft6.hpp \
include/webcpp/json_schema/detail/loader/walk.hpp include/webcpp/json_schema/detail/loader/readers.hpp \
test/Jamfile test/loader_draft2019_09_test.cpp test/loader_draft7_test.cpp test/loader_draft6_test.cpp"
git add $F
$G commit -m "json_schema: the keyword tables and resource rules of 2019-09, draft 7 and draft 6" -- $F
```

### Task E5.0b: Draft 4, OpenAPI 3.0, the six-dialect `validator`, and every runner

After E5.0a, alone. Waits for the owner's decision 1, which covers the
OpenAPI 3.0 schema (Apache-2.0).
- **Written for decision 1's recommended outcome (a):** the fetched schema
  is committed under `meta_schemas/openapi3_0/` with `LICENSE-OPENAPI.txt`,
  and its generated header carries webcpp's notice and a line naming the
  OpenAPI Initiative and Apache-2.0.
- **Under (b):** the schema is fetched at build time, as E2.2's are under
  (b), and neither the JSON nor `LICENSE-OPENAPI.txt` is committed.

**Files:**
- Create:
  - `include/webcpp/json_schema/detail/dialects/draft4.hpp`,
    `detail/dialects/openapi3_0.hpp`;
  - `meta_schemas/openapi3_0/schema.json` (fetched), `LICENSE-OPENAPI.txt`,
    `include/webcpp/json_schema/detail/meta_schemas/openapi3_0_schema.hpp`
    (generated);
  - `test/loader_draft4_test.cpp`, `test/loader_openapi3_0_test.cpp`;
  - `test/suite/suite_draft2019_09.cpp`, `suite_draft7.cpp`,
    `suite_draft6.cpp`, `suite_draft4.cpp`, `suite_openapi3_0.cpp`;
  - `test/openapi/nullable.json`, `test/openapi/discriminator.json`,
    `test/openapi/example.json`, `test/openapi/refused.json`;
  - `test/known_failures/draft2019-09.json`, `draft7.json`, `draft6.json`,
    `draft4.json`, `openapi3-0.json` (recorded).
- Modify:
  - `include/webcpp/json_schema/validator/validator.hpp` (`validator`
    widened);
  - `include/webcpp/json_schema/dialects/dialect.hpp`
    (`detail::meta_schema_of`);
  - `include/webcpp/json_schema/detail/loader/readers.hpp`;
  - `include/webcpp/json_schema/detail/meta_schemas/index.hpp`
    (generated), `meta_schemas/generate.py`, `meta_schemas/README.md`;
  - `test/suite/runner.hpp` (`default_dialect`, the dialect reason,
    `run_openapi`);
  - `test/meta_schemas_test.cpp` (20 files), `test/Jamfile`.

**Interfaces:**
- `dialect_machinery<draft4>` (spec 6.2, column 4): draft 6's keywords less
  `const`, `contains`, `propertyNames` and `examples` (32 rows), with these
  readers:
  - `maximum` and `minimum` read by `bound_draft4`: beside
    `"exclusiveMaximum": true` (or `exclusiveMinimum`) the kind is
    `exclusive_maximum` (`exclusive_minimum`), named `maximum`
    (`minimum`); otherwise it stays as written;
  - `exclusiveMaximum` and `exclusiveMinimum` read by `exclusive_flag`: a
    boolean, kept as `ignored`; anything else is `invalid_keyword_value`;
  - `required` read by `nonempty_names`: a non-empty array.
  - Its `rules`: `.id = "id"`, `.fragment_id_is_anchor`,
    `.ref_hides_siblings` true; `.boolean_schemas` and `.integer_by_value`
    false.
- `dialect_machinery<openapi3_0>` (spec 6.2, column OA), the Schema Object
  of OpenAPI 3.0.3, 36 rows:
  - `title`, `multipleOf`, `maximum` and `minimum` (`bound_draft4`),
    `exclusiveMaximum` and `exclusiveMinimum` (`exclusive_flag`);
  - `maxLength`, `minLength`, `pattern`, `maxItems`, `minItems`,
    `uniqueItems`, `maxProperties`, `minProperties`, `required`
    (`nonempty_names`), `enum`;
  - `type` (`type_single`, one string), `allOf`, `oneOf`, `anyOf`, `not`,
    `items` (a schema only), `properties`, `additionalProperties`
    (`boolean_or_schema`: a boolean or a schema, though the dialect has no
    boolean schemas elsewhere);
  - `description`, `format`, `default`, `nullable` (`nullable`, kind
    `ignored`), `discriminator`, `readOnly`, `writeOnly`, `xml`,
    `externalDocs`, `example`, `deprecated`;
  - `$ref`, whose siblings are `ignored`.
  - `nullable: true` beside `type` adds `null` to the type set; alone, or
    `false`, it changes nothing.
  - Its `rules`: `.id` empty, `.ref_hides_siblings` and `.integer_by_value`
    true, the rest false.
  - An unknown keyword follows PD2: `unknown_keyword`, but an `x-` one,
    which is `unknown`.
- `detail::meta_schema_of(dialect)` gives each dialect's meta-schema URI;
  OpenAPI 3.0's is
  `https://spec.openapis.org/oas/3.0/schema/2021-09-28#/definitions/Schema`.
  E2.5's pipeline compiles it, with its bundled document, as it does any
  other meta-schema.
- `validator` becomes spec 4.5's six:

```cpp
using validator = validator_for<draft2020_12, draft2019_09, draft7, draft6, draft4, openapi3_0>;
```

- `run_options` gains `std::optional<webcpp::json_schema::dialect>
  default_dialect;`, appended, which `run_case` puts into each case's
  compile options.
- `reason_for` gains the dialect rule, last:

```cpp
    if (dialect != "draft2020-12") {
        return "E5: " + std::string(dialect);   // a failure of a dialect's own semantics
    }
    return "";
```

- `run_openapi(validator, options, cases)` runs the draft 4 required cases
  whose schemas use only the Schema Object's keywords (no `"null"` type),
  with `default_dialect = openapi3_0`. Then it runs every case of the files
  under `cases`, in the suite's format. A case holding `"refused"` names
  the `errc` its compile must return, with `validate_schema` at its
  default, on.

- [ ] **Step 1: Fetch the OpenAPI schema and generate its header.**

```bash
cd libs/json_schema
mkdir -p meta_schemas/openapi3_0
curl -fsSL https://spec.openapis.org/oas/3.0/schema/2021-09-28 -o meta_schemas/openapi3_0/schema.json
shasum -a 256 meta_schemas/openapi3_0/schema.json
gh api repos/OAI/OpenAPI-Specification/contents/LICENSE --jq .content | base64 -d > LICENSE-OPENAPI.txt
head -1 LICENSE-OPENAPI.txt
cd ../..
```

  Expected: the SHA-256, which `meta_schemas/README.md` records beside the
  others, and the licence's first line `Apache License`.

  `generate.py` gains the origin of each file, so that the OpenAPI header
  names its own:

```python
ORIGINS = {
    "openapi3_0": "the OpenAPI Initiative's, Apache-2.0 (LICENSE-OPENAPI.txt)",
}
DEFAULT_ORIGIN = "the JSON Schema authors', BSD-3-Clause (LICENSE-JSON-SCHEMA.txt)"


def origin_of(path: pathlib.Path) -> str:
    return ORIGINS.get(path.relative_to(HERE).parts[0], DEFAULT_ORIGIN)
```

  `NOTICE`'s last line becomes `// is {origin}.`, and `header` formats it
  with `origin=origin_of(path)`; `index` passes the default. The
  `webcpp.generated meta-schemas` call gains
  `$(.meta)/openapi3_0_schema.hpp` after `draft2020_12_schema.hpp`, before
  `index.hpp` (Python's sorted order: `openapi3_0/` after every `draft`).

```bash
$B2 libs/json_schema/test//update-generated
$B2 -a libs/json_schema/test//generated-meta-schemas
head -8 libs/json_schema/include/webcpp/json_schema/detail/meta_schemas/openapi3_0_schema.hpp | tail -2
```

  Expected: the check exits 0, and the two lines name
  `meta_schemas/openapi3_0/schema.json` and "the OpenAPI Initiative's,
  Apache-2.0". Every other header is unchanged but `index.hpp`.
  `meta_schemas_test` expects 20 files.
- [ ] **Step 2: Write the failing tests.**

`test/loader_draft4_test.cpp`:

```cpp
#include <webcpp/json_schema.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/json/parse.hpp>

#include <string>

namespace js = webcpp::json_schema;
namespace ir = js::ir;
using k = ir::keyword_kind;

namespace {

const js::validator validator;

js::result<js::ir::document> load(std::string_view text) {
    return validator.load(boost::json::parse(text), {.default_dialect = js::dialect::draft4});
}

const ir::keyword* find(const js::ir::document& d, k kind) {
    return d.find(d.root(), kind);
}

void boolean_schemas_are_refused() {
    BOOST_TEST(load("true").error().code == js::errc::not_a_schema);
    BOOST_TEST(load(R"({"not": false})").error().code == js::errc::not_a_schema);
}

void exclusive_bounds() {
    const auto exclusive = load(R"({"maximum": 3, "exclusiveMaximum": true})");
    BOOST_TEST_EQ(exclusive->string(find(*exclusive, k::exclusive_maximum)->name), "maximum");
    BOOST_TEST(find(*exclusive, k::maximum) == nullptr);
    BOOST_TEST(find(*exclusive, k::ignored) != nullptr);
    const auto inclusive = load(R"({"minimum": 3, "exclusiveMinimum": false})");
    BOOST_TEST(find(*inclusive, k::minimum) != nullptr);
    BOOST_TEST(find(*inclusive, k::exclusive_minimum) == nullptr);
    BOOST_TEST(find(*load(R"({"exclusiveMaximum": true})"), k::ignored) != nullptr);
    BOOST_TEST(load(R"({"exclusiveMaximum": 3})").error().code == js::errc::invalid_keyword_value);
}

void integers_ids_and_required() {
    BOOST_TEST(!std::get<ir::types>(find(*load(R"({"type": "integer"})"), k::type)->data).integer_by_value);
    const auto d = load(R"({"id": "http://x/s", "definitions": {"a": {"id": "#a"}}})");
    BOOST_TEST_EQ(d->string(d->resources()[0].uri), "http://x/s");
    BOOST_TEST_EQ(d->anchors().size(), 1U);
    BOOST_TEST(load(R"({"required": []})").error().code == js::errc::invalid_keyword_value);
    BOOST_TEST(find(*load(R"({"const": 1})"), k::ignored) != nullptr);   // no keyword of draft 4 (PD2)
    BOOST_TEST_EQ(js::detail::dialect_machinery<js::draft4>::keywords().size(), 32U);
}

void schema_uri_spellings() {
    for (const char* uri : {"http://json-schema.org/draft-04/schema#", "http://json-schema.org/draft-04/schema"}) {
        const auto d = validator.load(boost::json::parse(std::string(R"({"$schema": ")") + uri + "\"}"));
        BOOST_TEST(d.has_value() && d->resources()[0].source_dialect == js::dialect::draft4);
    }
    const auto old = validator.load(boost::json::parse(R"({"$schema": "http://json-schema.org/draft-03/schema#"})"));
    BOOST_TEST(old.error().code == js::errc::unknown_dialect);
    BOOST_TEST_EQ(std::string(old.error().what), "http://json-schema.org/draft-03/schema#");
}

}  // namespace

int main() {
    boolean_schemas_are_refused();
    exclusive_bounds();
    integers_ids_and_required();
    schema_uri_spellings();
    return boost::report_errors();
}
```

`test/loader_openapi3_0_test.cpp` (spec 16.3: one case per OA row of spec
6.2; the E2 to E5 review's I16 and M24):

```cpp
#include <webcpp/json_schema.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/json/parse.hpp>

#include <string>

namespace js = webcpp::json_schema;
namespace ir = js::ir;
using k = ir::keyword_kind;

namespace {

const js::validator validator;

js::result<js::ir::document> load(std::string_view text,
                                  js::unknown_keywords unknown = js::unknown_keywords::dialect_default) {
    return validator.load(boost::json::parse(text), {.default_dialect = js::dialect::openapi3_0, .unknown = unknown});
}

const ir::keyword* find(const js::ir::document& d, k kind) {
    return d.find(d.root(), kind);
}

void nullable() {
    const auto with_type = load(R"({"type": "string", "nullable": true})");
    const auto& set = std::get<ir::types>(find(*with_type, k::type)->data).set;
    BOOST_TEST(set.contains(ir::type_set::null) && set.contains(ir::type_set::string));
    BOOST_TEST(find(*with_type, k::ignored) != nullptr);   // nullable itself
    const auto alone = load(R"({"nullable": true})");
    BOOST_TEST(find(*alone, k::type) == nullptr);
    const auto off = load(R"({"type": "integer", "nullable": false})");
    BOOST_TEST(!std::get<ir::types>(find(*off, k::type)->data).set.contains(ir::type_set::null));
}

void annotations_and_extensions() {
    const auto d = load(R"({"discriminator": {"propertyName": "k"}, "example": 1, "xml": {"name": "a"},
                            "externalDocs": {"url": "https://example.com"}, "x-mine": [1]})");
    BOOST_TEST(find(*d, k::discriminator) != nullptr);
    BOOST_TEST(find(*d, k::example) != nullptr);
    BOOST_TEST(find(*d, k::xml) != nullptr);
    BOOST_TEST(find(*d, k::external_docs) != nullptr);
    BOOST_TEST(find(*d, k::unknown) != nullptr);   // x-mine
}

void refusals() {
    const auto c = load(R"({"const": 1})");
    BOOST_TEST(c.error().code == js::errc::unknown_keyword);
    BOOST_TEST_EQ(std::string(c.error().what), "const");
    BOOST_TEST(load(R"({"type": ["string", "null"]})").error().code == js::errc::invalid_keyword_value);
    BOOST_TEST(load(R"({"not": true})").error().code == js::errc::not_a_schema);
    BOOST_TEST(load(R"({"additionalProperties": false})").has_value());
    BOOST_TEST(find(*load(R"({"const": 1})", js::unknown_keywords::annotate), k::unknown) != nullptr);
    BOOST_TEST(find(*load(R"({"const": 1})", js::unknown_keywords::ignore), k::ignored) != nullptr);
}

void ref_hides_siblings_and_bounds() {
    const auto d = load(R"({"$ref": "#/x", "minimum": 1, "x": {}})");
    BOOST_TEST(d.has_error() || find(*d, k::minimum) == nullptr);   // resolved or not, minimum is inert
    const auto b = load(R"({"maximum": 3, "exclusiveMaximum": true})");
    BOOST_TEST(find(*b, k::exclusive_maximum) != nullptr);
    BOOST_TEST_EQ(js::detail::dialect_machinery<js::openapi3_0>::keywords().size(), 36U);
}

}  // namespace

int main() {
    nullable();
    annotations_and_extensions();
    refusals();
    ref_hides_siblings_and_bounds();
    return boost::report_errors();
}
```

  `compile_errors_test.cpp` gains the two orders of E2.5's pipeline for
  OpenAPI 3.0, called from `main`:

```cpp
void openapi_refusals_in_both_orders() {
    const js::validator all;
    const auto checked = all.compile(boost::json::parse(R"({"const": 1})"), {.default_dialect = js::dialect::openapi3_0});
    BOOST_TEST(checked.error().code == js::errc::meta_schema_violation);   // the meta-schema first
    const auto walked = all.compile(boost::json::parse(R"({"const": 1})"),
                                    {.default_dialect = js::dialect::openapi3_0, .validate_schema = false});
    BOOST_TEST(walked.error().code == js::errc::unknown_keyword);
}
```

  The runner programs. `test/suite/suite_draft2019_09.cpp`:

```cpp
#include "runner.hpp"

#include <webcpp/json_schema.hpp>

#include <string_view>

namespace js = webcpp::json_schema;

int main(int argc, char** argv) {
    const js::validator validator;
    return webcpp::test::suite::run(
        validator, {.root = WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE, .dialect = "draft2019-09",
                    .known_failures = WEBCPP_TEST_JSON_SCHEMA_KNOWN,
                    .record = argc > 1 && std::string_view(argv[1]) == "--record",
                    .default_dialect = js::dialect::draft2019_09});
}
```

  `suite_draft7.cpp`, `suite_draft6.cpp` and `suite_draft4.cpp` differ in
  their two values only: `"draft7"` and `js::dialect::draft7`, `"draft6"`
  and `js::dialect::draft6`, `"draft4"` and `js::dialect::draft4`.
  `suite_openapi3_0.cpp`:

```cpp
#include "runner.hpp"

#include <webcpp/json_schema.hpp>

#include <string_view>

namespace js = webcpp::json_schema;

int main(int argc, char** argv) {
    const js::validator validator;
    return webcpp::test::suite::run_openapi(
        validator,
        {.root = WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE, .dialect = "openapi3-0",
         .known_failures = WEBCPP_TEST_JSON_SCHEMA_KNOWN,
         .record = argc > 1 && std::string_view(argv[1]) == "--record",
         .default_dialect = js::dialect::openapi3_0},
        WEBCPP_TEST_JSON_SCHEMA_OPENAPI);
}
```

  `run_openapi` and its helpers, in `runner.hpp`:

```cpp
inline bool schema_object_member(std::string_view key, const boost::json::value& value) {
    static constexpr std::array<std::string_view, 28> allowed{
        "title", "multipleOf", "maximum", "exclusiveMaximum", "minimum", "exclusiveMinimum",
        "maxLength", "minLength", "pattern", "maxItems", "minItems", "uniqueItems", "maxProperties",
        "minProperties", "required", "enum", "type", "allOf", "oneOf", "anyOf", "not", "items",
        "properties", "additionalProperties", "description", "format", "default", "$ref"};
    if (std::find(allowed.begin(), allowed.end(), key) == allowed.end()) {
        return false;
    }
    if (key == "type") {
        return value.is_string() && value.get_string() != "null";   // OpenAPI 3.0 has no null type
    }
    if (key == "exclusiveMaximum" || key == "exclusiveMinimum") {
        return value.is_bool();
    }
    return key != "items" || value.is_object();   // array items are draft 4's, not OpenAPI's
}

// Whether a draft 4 schema uses only the Schema Object's keywords, walked without recursion.
inline bool schema_object_only(const boost::json::value& schema) {
    std::vector<const boost::json::value*> work{&schema};
    while (!work.empty()) {
        const auto* object = work.back()->if_object();
        work.pop_back();
        if (object == nullptr) {
            return false;   // a boolean schema is no Schema Object
        }
        for (const auto& [key, value] : *object) {
            if (!schema_object_member(key, value)) {
                return false;
            }
            if (key == "properties") {
                for (const auto& member : value.as_object()) {
                    work.push_back(&member.value());
                }
            } else if (key == "allOf" || key == "anyOf" || key == "oneOf") {
                for (const auto& sub : value.as_array()) {
                    work.push_back(&sub);
                }
            } else if (key == "items" || key == "not" || (key == "additionalProperties" && value.is_object())) {
                work.push_back(&value);
            }
        }
    }
    return true;
}

inline std::optional<webcpp::json_schema::errc> errc_named(std::string_view name) {
    using webcpp::json_schema::errc;
    static constexpr std::array<std::pair<std::string_view, errc>, 6> names{{
        {"not_a_schema", errc::not_a_schema},
        {"invalid_keyword_value", errc::invalid_keyword_value},
        {"meta_schema_violation", errc::meta_schema_violation},
        {"unknown_keyword", errc::unknown_keyword},
        {"invalid_uri", errc::invalid_uri},
        {"unresolvable_reference", errc::unresolvable_reference},
    }};
    for (const auto& [n, code] : names) {
        if (n == name) {
            return code;
        }
    }
    return std::nullopt;
}

template <class Validator>
void run_refusal(const Validator& validator, const run_options& options, const std::string& file,
                 const boost::json::object& c, ledger& l) {
    typename Validator::compile_options compile;
    compile.default_dialect = options.default_dialect;
    const auto schema = validator.compile(c.at("schema"), compile);
    const test_id id{.file = file, .case_description = boost::json::value_to<std::string>(c.at("description")),
                     .test_description = "refused"};
    const auto expected = errc_named(c.at("refused").as_string());
    std::string why;
    if (!expected) {
        why = "no such errc";
    } else if (schema) {
        why = "compiled";
    } else if (schema.error().code != *expected) {
        why = "refused with " + schema.error().code.message();
    }
    l.seen[id] = why.empty();
    if (why.empty()) {
        ++l.passed;
    } else {
        note_failure(l, options, id, c.at("schema"), why);
    }
}

template <class Validator>
int run_openapi(const Validator& validator, const run_options& options, std::string_view cases) {
    ledger l;
    if (!options.record) {
        l.known = read_known(options.known_failures);
    }
    for_each_case(options.root, "draft4", [&](const std::string& file, const boost::json::object& c) {
        if (!file.starts_with("optional/") && schema_object_only(c.at("schema"))) {
            run_case(validator, options, "draft4/" + file, c, l);
        }
    });
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(cases)) {
        files.push_back(entry.path());
    }
    std::sort(files.begin(), files.end());
    for (const auto& path : files) {
        const std::string file = "openapi/" + path.filename().generic_string();
        for (const auto& c : boost::json::parse(read_file(path)).as_array()) {
            if (c.as_object().contains("refused")) {
                run_refusal(validator, options, file, c.as_object(), l);
            } else {
                run_case(validator, options, file, c.as_object(), l);
            }
        }
    }
    return report(l, options);
}
```

  The four case files:

`test/openapi/nullable.json`:

```json
[
    {
        "description": "nullable beside type admits null",
        "schema": {"type": "string", "nullable": true},
        "tests": [
            {"description": "a string", "data": "a", "valid": true},
            {"description": "null", "data": null, "valid": true},
            {"description": "a number", "data": 1, "valid": false}
        ]
    },
    {
        "description": "nullable without type changes nothing",
        "schema": {"nullable": true, "minimum": 1},
        "tests": [
            {"description": "null", "data": null, "valid": true},
            {"description": "zero", "data": 0, "valid": false}
        ]
    },
    {
        "description": "nullable false keeps null out",
        "schema": {"type": "integer", "nullable": false},
        "tests": [
            {"description": "null", "data": null, "valid": false}
        ]
    }
]
```

`test/openapi/refused.json`. With the meta-schema check on, each is refused
by OpenAPI 3.0's meta-schema before the walk (E2.5's pipeline):

```json
[
    {"description": "const is refused", "schema": {"const": 1}, "refused": "meta_schema_violation", "tests": []},
    {"description": "patternProperties is refused", "schema": {"patternProperties": {"^a": {}}}, "refused": "meta_schema_violation", "tests": []},
    {"description": "$defs is refused", "schema": {"$defs": {}}, "refused": "meta_schema_violation", "tests": []},
    {"description": "if is refused", "schema": {"if": {}}, "refused": "meta_schema_violation", "tests": []},
    {"description": "a null type is refused", "schema": {"type": "null"}, "refused": "meta_schema_violation", "tests": []},
    {
        "description": "an extension loads",
        "schema": {"x-anything": {"a": 1}, "type": "string"},
        "tests": [
            {"description": "a string", "data": "a", "valid": true}
        ]
    }
]
```

`test/openapi/discriminator.json`:

```json
[
    {
        "description": "discriminator validates nothing",
        "schema": {"type": "object", "discriminator": {"propertyName": "kind"}},
        "tests": [
            {"description": "without the property", "data": {}, "valid": true},
            {"description": "with it", "data": {"kind": "a"}, "valid": true}
        ]
    }
]
```

`test/openapi/example.json`:

```json
[
    {
        "description": "example, xml and externalDocs validate nothing",
        "schema": {
            "type": "object",
            "example": {"a": 1},
            "xml": {"name": "a"},
            "externalDocs": {"url": "https://example.com"}
        },
        "tests": [
            {"description": "an object", "data": {"b": 2}, "valid": true},
            {"description": "a string", "data": "b", "valid": false}
        ]
    }
]
```

  Append to `test/Jamfile`:

```
webcpp.run loader_draft4 : loader_draft4_test.cpp ;
webcpp.run loader_openapi3_0 : loader_openapi3_0_test.cpp ;
suite-runner draft2019-09 : suite/suite_draft2019_09.cpp ;
suite-runner draft7 : suite/suite_draft7.cpp ;
suite-runner draft6 : suite/suite_draft6.cpp ;
suite-runner draft4 : suite/suite_draft4.cpp ;

# OpenAPI 3.0 has no suite of its own: a filtered draft 4 and the cases
# under openapi/, a directory a WASI program reaches through webcpp.data.
path-constant OPENAPI : openapi ;
.openapi = [ regex.replace $(OPENAPI) "\\\\" "/" ] ;
webcpp.run suite-openapi3-0 : suite/suite_openapi3_0.cpp
  : <library>/webcpp/third_party//json-schema-test-suite
    <define>"\"WEBCPP_TEST_JSON_SCHEMA_KNOWN=\\\"$(.known)/openapi3-0.json\\\"\""
    <define>"\"WEBCPP_TEST_JSON_SCHEMA_OPENAPI=\\\"$(.openapi)\\\"\""
    [ webcpp.data $(KNOWN) $(OPENAPI) ]
    <dependency>known_failures/openapi3-0.json ;
```

  Create the five lists as `[]`. Run
  `$B2 libs/json_schema/test//loader_draft4 libs/json_schema/test//loader_openapi3_0 libs/json_schema/test//suite-draft4`.
  Expected: FAIL, `dialect_machinery<webcpp::json_schema::draft4>`
  undefined.
- [ ] **Step 3: Implement** the two tables, the six readers and the
  widening. `bound_draft4`, the one reader that turns on a sibling's value:

```cpp
template <class Traits>
result<read_result> walker<Traits>::read_bound_draft4(const pending& p, const keyword_rule& rule,
                                                      const boost::json::value& v, std::uint32_t at) {
    auto bound = read_bound(p, v, at);   // the number, exactly, through the exact table
    if (!bound) {
        return bound.error();
    }
    const bool maximum = rule.kind == ir::keyword_kind::maximum;
    const boost::json::value* flag = p.value->as_object().if_contains(maximum ? "exclusiveMaximum" : "exclusiveMinimum");
    const bool exclusive = flag != nullptr && flag->is_bool() && flag->get_bool();
    if (!exclusive) {
        return read_result{.kind = rule.kind, .data = *bound};
    }
    return read_result{.kind = maximum ? ir::keyword_kind::exclusive_maximum : ir::keyword_kind::exclusive_minimum,
                       .data = *bound};
}
```

  `nullable` adds `null` to the node's `types` keyword after the node's
  members are read: `walk_object` calls `apply_nullable(keywords_)` before
  `define` when the node held `"nullable": true` and a `type`.
- [ ] **Step 4: Run the tests, then record every new list.**

```bash
$B2 libs/json_schema/test//loader_draft4 libs/json_schema/test//loader_openapi3_0 \
  libs/json_schema/test//compile_errors libs/json_schema/test//meta_schemas
for l in draft2019-09 draft7 draft6 draft4 openapi3-0; do
  record_known $l || break
  python3 -c "import json,sys; l=json.load(open(sys.argv[1])); print(sys.argv[1], sorted({e['reason'] for e in l}))" \
    libs/json_schema/test/known_failures/$l.json
done
lanes libs/json_schema/test
lint_clean
```

  Expected:
  - the four tests `**passed**`;
  - each list's reasons among `E5: formats`, `E5: other dialects`,
    `E5: content` and its own `E5: <dialect>`;
  - `lanes: green`, every runner `0 unexpected, 0 stale`;
  - `lint: clean`.
- [ ] **Step 5: Commit.**

```bash
cd libs/json_schema
F="include/webcpp/json_schema/validator/validator.hpp include/webcpp/json_schema/dialects/dialect.hpp \
include/webcpp/json_schema/detail/dialects/draft4.hpp include/webcpp/json_schema/detail/dialects/openapi3_0.hpp \
include/webcpp/json_schema/detail/loader/walk.hpp include/webcpp/json_schema/detail/loader/readers.hpp \
include/webcpp/json_schema/detail/meta_schemas/openapi3_0_schema.hpp include/webcpp/json_schema/detail/meta_schemas/index.hpp \
meta_schemas/openapi3_0/schema.json meta_schemas/generate.py meta_schemas/README.md LICENSE-OPENAPI.txt \
test/Jamfile test/loader_draft4_test.cpp test/loader_openapi3_0_test.cpp test/compile_errors_test.cpp \
test/meta_schemas_test.cpp test/suite/runner.hpp test/suite/suite_draft2019_09.cpp test/suite/suite_draft7.cpp \
test/suite/suite_draft6.cpp test/suite/suite_draft4.cpp test/suite/suite_openapi3_0.cpp \
test/openapi/nullable.json test/openapi/refused.json test/openapi/discriminator.json test/openapi/example.json \
test/known_failures/draft2019-09.json test/known_failures/draft7.json test/known_failures/draft6.json \
test/known_failures/draft4.json test/known_failures/openapi3-0.json"
git add $F
$G commit -m "json_schema: draft 4 and OpenAPI 3.0 loaded, every dialect in validator, and a runner for each" -- $F
```

### Task E5.1: 2019-09 whole: `$recursiveRef` at run time

Runs alongside E5.2 to E5.8 (slug `draft2019-09`).

**Files:**
- Create: `test/recursive_ref_test.cpp`.
- Modify:
  - `include/webcpp/json_schema/detail/compiler/compile.hpp` (lowers
    `form = recursive` to `dynamic_jump` with its flag);
  - `include/webcpp/json_schema/detail/vm/machine.hpp` (the recursive
    rule);
  - `include/webcpp/json_schema/detail/emitter/forms.hpp` (the recursive
    row);
  - `test/emitter_test.cpp`, `test/Jamfile`,
    `test/known_failures/draft2019-09.json`.

**Interfaces:**
- `dynamic_jump` with its `flags` bit `recursive` (2019-09 core 8.2.4.2):
  - when the initial target's resource has `recursive_anchor`, it jumps to
    the outermost resource of the dynamic scope that also has
    `recursive_anchor`;
  - otherwise it jumps to the initial target.
- The emitter writes `$recursiveRef` and `$recursiveAnchor` into 2019-09,
  and refuses them (`unrepresentable`) into every other target.

- [ ] **Step 1: Write the failing tests.** `test/recursive_ref_test.cpp`,
  2019-09 core's own example:

```cpp
#include <webcpp/json_schema.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/json/parse.hpp>

namespace js = webcpp::json_schema;

namespace {

const js::validator validator;

constexpr const char* tree = R"({
    "$schema": "https://json-schema.org/draft/2019-09/schema",
    "$id": "https://example.com/tree",
    "$recursiveAnchor": true,
    "type": "object",
    "properties": {
        "data": true,
        "children": {"type": "array", "items": {"$recursiveRef": "#"}}
    }
})";

js::result<js::validator::schema_type> strict(bool anchored) {
    js::registry registry;
    (void)registry.add("https://example.com/tree", boost::json::parse(tree));
    boost::json::value s = boost::json::parse(R"({
        "$schema": "https://json-schema.org/draft/2019-09/schema",
        "$id": "https://example.com/strict-tree",
        "$recursiveAnchor": true,
        "$ref": "tree",
        "unevaluatedProperties": false
    })");
    if (!anchored) {
        s.as_object().erase("$recursiveAnchor");
    }
    return validator.compile(s, {.registry = &registry});
}

void the_outermost_anchor_wins() {
    const auto s = strict(true);
    if (!BOOST_TEST(s.has_value())) {
        return;
    }
    BOOST_TEST(s->is_valid(boost::json::parse(R"({"children": [{"data": 1}]})")));
    BOOST_TEST(!s->is_valid(boost::json::parse(R"({"children": [{"daat": 1}]})")));   // strict, recursively
}

void without_the_outer_anchor_it_is_a_plain_reference() {
    const auto s = strict(false);
    if (!BOOST_TEST(s.has_value())) {
        return;
    }
    BOOST_TEST(s->is_valid(boost::json::parse(R"({"children": [{"daat": 1}]})")));   // tree's rules inside
    BOOST_TEST(!s->is_valid(boost::json::parse(R"({"daat": 1})")));                    // strict at the top
}

}  // namespace

int main() {
    the_outermost_anchor_wins();
    without_the_outer_anchor_it_is_a_plain_reference();
    return boost::report_errors();
}
```

  The emitter's recursive row, appended to `emitter_test.cpp` and called from
  `main`:

```cpp
void recursive_references() {
    const char* source = R"({"$schema": "https://json-schema.org/draft/2019-09/schema",
                             "$recursiveAnchor": true, "items": {"$recursiveRef": "#"}})";
    const auto document = js::validator().load(boost::json::parse(source));
    const auto same = js::emit(*document, js::dialect::draft2019_09, {.write_schema_keyword = false});
    BOOST_TEST_EQ(boost::json::serialize(same->schema), R"({"$recursiveAnchor":true,"items":{"$recursiveRef":"#"}})");
    for (const js::dialect target : {js::dialect::draft2020_12, js::dialect::draft7, js::dialect::draft6,
                                     js::dialect::draft4, js::dialect::openapi3_0}) {
        BOOST_TEST(js::emit(*document, target).error().code == js::errc::unrepresentable);
    }
}
```

  Append `webcpp.run recursive_ref : recursive_ref_test.cpp ;` to
  `test/Jamfile`. Run
  `$B2 libs/json_schema/test//recursive_ref libs/json_schema/test//emitter`.
  Expected: FAIL, the compile refused with `invalid_ir` naming
  `$recursiveRef`, and the emitter's row unwritten.
- [ ] **Step 2: Implement** the lowering, the rule and the row.
- [ ] **Step 3: Record the list and check its reasons.**

```bash
record_known draft2019-09
python3 -c "import json; print(sorted({e['reason'] for e in json.load(open('libs/json_schema/test/known_failures/draft2019-09.json'))}))"
lanes libs/json_schema/test
lint_clean
```

  Expected: `['E5: formats', 'E5: other dialects']`, so the 1261 required
  tests pass; `lanes: green`; `lint: clean`.
- [ ] **Step 4: Commit.**

```bash
cd libs/json_schema
F="include/webcpp/json_schema/detail/compiler/compile.hpp include/webcpp/json_schema/detail/vm/machine.hpp \
include/webcpp/json_schema/detail/emitter/forms.hpp test/recursive_ref_test.cpp test/emitter_test.cpp test/Jamfile \
test/known_failures/draft2019-09.json"
git add $F
$G commit -m "json_schema: draft 2019-09 whole, \$recursiveRef included, proven by its suite" -- $F
```

### Task E5.2: Drafts 7 and 6 whole

Runs alongside E5.1 and E5.3 to E5.8 (slug `drafts-7-6`).

**Files:**
- Create: `test/draft7_semantics_test.cpp`.
- Modify: whatever the suites show wrong, among
  `detail/loader/walk.hpp`, `detail/loader/resources.hpp`,
  `detail/compiler/compile.hpp`; `test/Jamfile`,
  `test/known_failures/draft7.json`, `test/known_failures/draft6.json`.

**Interfaces:** none new. The verdicts below are what spec 6.2's columns 6
and 7 mean at run time.

- [ ] **Step 1: Write the failing test** `test/draft7_semantics_test.cpp`:

```cpp
#include <webcpp/json_schema.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/json/parse.hpp>

namespace js = webcpp::json_schema;

namespace {

const js::validator validator;

bool valid(std::string_view schema, std::string_view instance, js::dialect d = js::dialect::draft7) {
    const auto s = validator.compile(boost::json::parse(schema), {.default_dialect = d});
    return s.has_value() && s->is_valid(boost::json::parse(instance));
}

void ref_siblings_are_inert() {
    const char* s = R"({"$ref": "#/definitions/a", "minimum": 5, "definitions": {"a": {"type": "integer"}}})";
    BOOST_TEST(valid(s, "1"));
    BOOST_TEST(!valid(s, "\"a\""));
}

void a_sibling_id_does_not_move_the_base() {
    // the suite's "$ref prevents a sibling $id from changing the base uri", in short
    const char* s = R"({
        "$id": "http://localhost:1234/sibling_id/base/",
        "definitions": {
            "foo": {"$id": "http://localhost:1234/sibling_id/foo.json", "type": "string"},
            "base_foo": {"$id": "foo.json", "type": "number"}
        },
        "allOf": [{"$id": "http://localhost:1234/sibling_id/", "$ref": "foo.json"}]
    })";
    BOOST_TEST(valid(s, "1"));
    BOOST_TEST(!valid(s, "\"a\""));
}

void anchors_dependencies_and_integers() {
    BOOST_TEST(!valid(R"({"allOf": [{"$ref": "#s"}], "definitions": {"a": {"$id": "#s", "type": "string"}}})", "1"));
    const char* d = R"({"dependencies": {"a": ["b"], "c": {"required": ["d"]}}})";
    BOOST_TEST(!valid(d, R"({"a": 1})"));
    BOOST_TEST(!valid(d, R"({"c": 1})"));
    BOOST_TEST(valid(d, R"({"a": 1, "b": 2, "c": 3, "d": 4})"));
    BOOST_TEST(valid(R"({"type": "integer"})", "1.0", js::dialect::draft6));
}

void draft6_has_no_if() {
    const char* s = R"({"if": {"type": "string"}, "then": false})";
    BOOST_TEST(!valid(s, "\"a\""));
    BOOST_TEST(valid(s, "\"a\"", js::dialect::draft6));
}

}  // namespace

int main() {
    ref_siblings_are_inert();
    a_sibling_id_does_not_move_the_base();
    anchors_dependencies_and_integers();
    draft6_has_no_if();
    return boost::report_errors();
}
```

  Append `webcpp.run draft7_semantics : draft7_semantics_test.cpp ;`. Run
  it. Expected: FAIL wherever E5.0a's loading leaves a verdict wrong; if
  it passes at once, the suites below are the failing test.
- [ ] **Step 2: Fix, then record both lists.**

```bash
record_known draft7
record_known draft6
for l in draft7 draft6; do
  python3 -c "import json,sys; print(sys.argv[1], sorted({e['reason'] for e in json.load(open(sys.argv[2]))}))" \
    $l libs/json_schema/test/known_failures/$l.json
done
lanes libs/json_schema/test
lint_clean
```

  Expected: `draft7 ['E5: content', 'E5: formats', 'E5: other dialects']`
  and `draft6 ['E5: formats']`, so 929 of 929 and 841 of 841 required tests
  pass; `lanes: green`; `lint: clean`.
- [ ] **Step 3: Commit.** `git diff --name-only -- include` names each header Step 2
  changed, one path each:

```bash
cd libs/json_schema
git status --porcelain
F="test/draft7_semantics_test.cpp test/Jamfile test/known_failures/draft7.json test/known_failures/draft6.json \
`git diff --name-only -- include | tr '\n' ' '`"
git add $F
$G commit -m "json_schema: drafts 7 and 6 whole, each proven by its suite" -- $F
```

### Task E5.3: Draft 4 whole

Runs alongside E5.1, E5.2 and E5.4 to E5.8 (slug `draft4`).

**Files:**
- Create: `test/draft4_semantics_test.cpp`.
- Modify: `include/webcpp/json_schema/detail/numbers/decompose.hpp`
  (`is_integer_value`'s `false` branch), `test/numbers_test.cpp`, whatever
  else the suite shows wrong; `test/Jamfile`,
  `test/known_failures/draft4.json`.

**Interfaces:**
- `integer` with `integer_by_value = false` (decision PD7):
  - an `int64_t` or `uint64_t` is an integer;
  - a `double` is an integer only when it is integral and outside both
    64-bit ranges, where Boost.JSON stores an integer literal it cannot hold
    in 64 bits;
  - so `1.0` is not an integer, and a 54-digit literal is.
  - The one deviation: a literal written with an exponent beyond 2^64
    (`1e60`) counts as an integer. The conformance chapter states it.

- [ ] **Step 1: Write the failing test** `test/draft4_semantics_test.cpp`:

```cpp
#include <webcpp/json_schema.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/json/parse.hpp>

namespace js = webcpp::json_schema;

namespace {

const js::validator validator;

bool valid(std::string_view schema, std::string_view instance) {
    const auto s = validator.compile(boost::json::parse(schema), {.default_dialect = js::dialect::draft4});
    return s.has_value() && s->is_valid(boost::json::parse(instance));
}

}  // namespace

int main() {
    BOOST_TEST(!valid(R"({"type": "integer"})", "1.0"));
    BOOST_TEST(valid(R"({"type": "integer"})", "1"));
    BOOST_TEST(valid(R"({"type": "integer"})", "12345678910111213141516171819202122232425262728293031"));
    BOOST_TEST(valid(R"({"type": "integer"})", "-12345678910111213141516171819202122232425262728293031"));
    BOOST_TEST(valid(R"({"type": "integer"})", "18446744073709551615"));
    BOOST_TEST(!valid(R"({"type": "integer"})", "1.5"));
    BOOST_TEST(!valid(R"({"maximum": 3, "exclusiveMaximum": true})", "3"));
    BOOST_TEST(valid(R"({"maximum": 3, "exclusiveMaximum": false})", "3"));
    BOOST_TEST(!valid(R"({"maximum": 9.727837981879871e+26, "exclusiveMaximum": true})", "9.727837981879871e+26"));
    BOOST_TEST(valid(R"({"$ref": "#/definitions/a", "minimum": 5, "definitions": {"a": {}}})", "1"));
    return boost::report_errors();
}
```

  `numbers_test.cpp`'s E1.2 case gains, beside its draft 4 lines:

```cpp
    BOOST_TEST(js::detail::is_integer_value(boost::json::value(1.2345678910111213e52), false));
    BOOST_TEST(!js::detail::is_integer_value(boost::json::value(9007199254740992.0), false));   // fits 64 bits
```

  Append `webcpp.run draft4_semantics : draft4_semantics_test.cpp ;`. Run
  both. Expected: FAIL, a bignum not an integer in draft 4.
- [ ] **Step 2: Implement,** then record the list:

```bash
record_known draft4
python3 -c "import json; print(sorted({e['reason'] for e in json.load(open('libs/json_schema/test/known_failures/draft4.json'))}))"
lanes libs/json_schema/test
lint_clean
```

  Expected: `['E5: formats']`, so 618 of 618 required tests pass, and
  `optional/zeroTerminatedFloats` and `optional/bignum` pass whole;
  `lanes: green`; `lint: clean`.
- [ ] **Step 3: Commit.**

```bash
cd libs/json_schema
F="include/webcpp/json_schema/detail/numbers/decompose.hpp test/numbers_test.cpp test/draft4_semantics_test.cpp \
test/Jamfile test/known_failures/draft4.json `git diff --name-only -- include | tr '\n' ' '`"
git add $F
$G commit -m "json_schema: draft 4 whole, integers by their representation, proven by its suite" -- $F
```

### Task E5.4: OpenAPI 3.0 whole, and its page

Runs alongside E5.1 to E5.3 and E5.5 to E5.8 (slug `openapi`). Written for
decision 1's (a), as E5.0b is; under (b) nothing here changes.

**Files:**
- Create: `test/openapi_test.cpp`,
  `example/json_schema_openapi.cpp` with its `.expected`.
- Modify: whatever the runner shows wrong; `doc/emitting.adoc` (OpenAPI
  3.0's half), `example/Jamfile`, `test/Jamfile`,
  `test/known_failures/openapi3-0.json`.

- [ ] **Step 1: Write the failing test** `test/openapi_test.cpp`:

```cpp
#include <webcpp/json_schema.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/json/parse.hpp>

namespace js = webcpp::json_schema;

namespace {

const js::validator validator;

bool valid(std::string_view schema, std::string_view instance) {
    const auto s = validator.compile(boost::json::parse(schema), {.default_dialect = js::dialect::openapi3_0});
    return s.has_value() && s->is_valid(boost::json::parse(instance));
}

}  // namespace

int main() {
    BOOST_TEST(valid(R"({"type": "string", "nullable": true})", "null"));
    BOOST_TEST(!valid(R"({"type": "string"})", "null"));
    BOOST_TEST(valid(R"({"type": "integer"})", "1.0"));   // integers by value, as in JSON Schema 2020-12
    BOOST_TEST(!valid(R"({"maximum": 3, "exclusiveMaximum": true})", "3"));
    BOOST_TEST(valid(R"({"type": "object", "discriminator": {"propertyName": "k"}})", "{}"));
    BOOST_TEST(valid(R"({"additionalProperties": false, "properties": {"a": {}}})", R"({"a": 1})"));
    BOOST_TEST(!valid(R"({"additionalProperties": false, "properties": {"a": {}}})", R"({"b": 1})"));
    BOOST_TEST(valid(R"({"type": "string", "x-note": {"any": "thing"}})", "\"s\""));
    return boost::report_errors();
}
```

  Append `webcpp.run openapi : openapi_test.cpp ;`. Run it. Expected: FAIL
  wherever E5.0b's loading leaves a verdict wrong; the runner's list is the
  failing test otherwise.
- [ ] **Step 2: Fix, then record the list.**

```bash
record_known openapi3-0
python3 -c "import json; print(sorted({e['reason'] for e in json.load(open('libs/json_schema/test/known_failures/openapi3-0.json'))}))"
```

  Expected: `[]` or `['E5: formats']`: every `test/openapi` case passes,
  and the draft 4 subset passes but formats.
- [ ] **Step 3: The page's OpenAPI 3.0 half,** in `doc/emitting.adoc`, and
  its example `example/json_schema_openapi.cpp`:

```cpp
#include <webcpp/json_schema.hpp>

#include <boost/json.hpp>

#include <iostream>

namespace js = webcpp::json_schema;

int main() {
    // tag::openapi[]
    const js::validator validator;
    const auto document = validator.load(boost::json::parse(R"({
        "$defs": {"name": {"type": "string", "minLength": 1}},
        "type": "object",
        "properties": {"first": {"$ref": "#/$defs/name"}, "nick": {"type": ["string", "null"]}}
    })"));
    const auto out = js::emit(*document, js::dialect::openapi3_0, {.write_schema_keyword = false});
    std::cout << boost::json::serialize(out->schema) << '\n';
    std::cout << boost::json::serialize(out->components) << '\n';
    // end::openapi[]
}
```

  `example/json_schema_openapi.expected`:

```
{"type":"object","properties":{"first":{"$ref":"#/components/schemas/name"},"nick":{"type":"string","nullable":true}}}
{"name":{"type":"string","minLength":1}}
```

  The section shows it, says that shared nodes go into `components`, which
  the caller places under its document's `components/schemas`, and lists
  what OpenAPI 3.0 refuses (E4.3's cells).

```bash
$B2 -a --build-dir=bin/doc libs/json_schema/doc
lanes libs/json_schema/test libs/json_schema/example
lint_clean
```

  Expected: exit 0, `lanes: green`, `lint: clean`.
- [ ] **Step 4: Commit.** `git diff --name-only -- include` names each header Step 2
  changed, one path each:

```bash
cd libs/json_schema
git status --porcelain
F="test/openapi_test.cpp test/Jamfile test/known_failures/openapi3-0.json doc/emitting.adoc example/Jamfile \
example/json_schema_openapi.cpp example/json_schema_openapi.expected `git diff --name-only -- include | tr '\n' ' '`"
git add $F
$G commit -m "json_schema: OpenAPI 3.0's Schema Object whole, loaded, validated and emitted" -- $F
```

### Task E5.5: Formats through `webcpp::lexical`

Runs alongside E5.1 to E5.4 and E5.6 to E5.8 (slug `formats`). Needs the
formats track's published result (its F8): check first with
`git -C ~/dev/webcpp ls-tree "$SUPERPROJECT_BASE" libs/lexical libs/idna`
(two `160000 commit` lines). Until they are there, E5.5 waits; the others
do not.

**Files:**
- Create:
  - `include/webcpp/json_schema/formats/default_formats.hpp`;
  - `test/format_test.cpp`.
- Modify:
  - `build.jam` (`<library>/webcpp/lexical//lexical`);
  - `include/webcpp/json_schema/formats/format_set.hpp` (`check_in`);
  - `include/webcpp/json_schema/validator/options.hpp` (`formats`
    defaults to `default_formats()`, as spec 4.5 says);
  - `include/webcpp/json_schema/detail/compiler/opcode.hpp`
    (`assert_format = 36`), `detail/compiler/compile.hpp`;
  - `include/webcpp/json_schema/detail/vm/machine.hpp`;
  - `test/suite/runner.hpp` (`permanent_reasons` rows, if any);
  - `test/messages_test.cpp` (the `format` golden);
  - `meta/include-boundaries.json` (an object for
    `formats/default_formats.hpp`, the one header that may include
    `webcpp/lexical`), `test/Jamfile`.

**Interfaces:**
- `format_entry` gains `check_in`, so the default set can pass the
  validation's resource to `is_iri`, `is_iri_reference` and `is_regex`
  (spec 10 asks for it; spec 4.9's form cannot carry it; decision PD6):

```cpp
struct format_entry {
    std::string_view name;
    boost::compat::function_ref<bool(std::string_view) noexcept> check;
    /** When set, called in place of check, with the validation's resource. */
    std::optional<boost::compat::function_ref<bool(std::string_view, std::pmr::memory_resource&) noexcept>>
        check_in = std::nullopt;
};
```

- `default_formats()` returns a `static constexpr std::array` of 19
  entries: `date-time`, `date`, `time`, `duration`, `email`, `idn-email`,
  `hostname`, `idn-hostname`, `ipv4`, `ipv6`, `uri`, `uri-reference`,
  `iri`, `iri-reference`, `uuid`, `uri-template`, `json-pointer`,
  `relative-json-pointer`, `regex`.
  - Each names `webcpp::lexical::is_<format>` through a `noexcept`
    function of this header where the signature differs (`is_time`'s
    defaulted policy).
  - `iri`, `iri-reference` and `regex` also set `check_in`.
  - **`time` admits leap seconds** (`leap_seconds::allowed`), as RFC 3339
    and every dialect's suite do. The formats plan's flag, that v1's
    refused policy is unreachable from this set, is settled by decision
    PD8: json_schema has no v1 dialect (spec 21, item 12), and a program
    that wants v1's rule today passes its own entry, which `format_test`
    shows.
- `assert_format` (opcode 36) runs only when the payload's `assertion` or
  `format_assertion` is set, and only on strings. An unknown name
  annotates. A failure is kind `format`.

- [ ] **Step 1: Write the failing test** `test/format_test.cpp`:

```cpp
#include <webcpp/lexical/date_time.hpp>
#include <webcpp/json_schema.hpp>
#include <webcpp/json_schema/formats/default_formats.hpp>

#include "counting_resource.hpp"

#include <boost/core/lightweight_test.hpp>
#include <boost/json/parse.hpp>

#include <array>
#include <string>

namespace js = webcpp::json_schema;

namespace {

const js::validator validator;

bool valid(std::string_view schema, std::string_view instance, bool assertion,
           js::format_set formats = js::default_formats()) {
    const auto s = validator.compile(boost::json::parse(schema), {.format_assertion = assertion, .formats = formats});
    return s.has_value() && s->is_valid(boost::json::parse(instance));
}

bool even_length(std::string_view s) noexcept {
    return s.size() % 2 == 0;
}

bool time_without_leap_seconds(std::string_view s) noexcept {
    return webcpp::lexical::is_time(s, webcpp::lexical::leap_seconds::refused);
}

void annotation_or_assertion() {
    BOOST_TEST(valid(R"({"format": "email"})", "\"nope\"", false));   // an annotation by default
    BOOST_TEST(!valid(R"({"format": "email"})", "\"nope\"", true));
    BOOST_TEST(valid(R"({"format": "email"})", "\"a@b.example\"", true));
    BOOST_TEST(valid(R"({"format": "email"})", "12", true));          // strings only
    BOOST_TEST(valid(R"({"format": "no-such-format"})", "\"x\"", true));
    BOOST_TEST(!valid(R"({"format": "iri"})", "\"http://a b\"", true));
    BOOST_TEST(!valid(R"({"format": "regex"})", "\"\\\\A\"", true));
    BOOST_TEST_EQ(js::default_formats().size(), 19U);
}

void a_set_of_ones_own() {
    const std::array<js::format_entry, 1> mine{{{.name = "even", .check = even_length}}};
    BOOST_TEST(!valid(R"({"format": "even"})", "\"abc\"", true, mine));
    BOOST_TEST(valid(R"({"format": "email"})", "\"nope\"", true, mine));   // the default set replaced
}

void leap_seconds() {
    BOOST_TEST(valid(R"({"format": "time"})", "\"23:59:60Z\"", true));
    const std::array<js::format_entry, 1> v1{{{.name = "time", .check = time_without_leap_seconds}}};
    BOOST_TEST(!valid(R"({"format": "time"})", "\"23:59:60Z\"", true, v1));
}

void an_iri_maps_into_the_validation_s_resource() {
    webcpp::test::counting_resource validation;
    const auto s = validator.compile(boost::json::parse(R"({"format": "iri"})"), {.format_assertion = true});
    const std::string broken = "http://example.com/" + std::string(600, '\xC3');   // invalid UTF-8
    std::string valid_iri = "http://example.com/";
    for (int i = 0; i < 600; ++i) {
        valid_iri += "\xC3\xA9";   // 1,200 bytes of non-ASCII: past the 1 KiB inline buffer
    }
    BOOST_TEST(s->is_valid(boost::json::value(valid_iri), {}, std::pmr::polymorphic_allocator<std::byte>(&validation)));
    BOOST_TEST(validation.allocations > 0);
    BOOST_TEST(!s->is_valid(boost::json::value(broken), {}, std::pmr::polymorphic_allocator<std::byte>(&validation)));
}

}  // namespace

int main() {
    annotation_or_assertion();
    a_set_of_ones_own();
    leap_seconds();
    an_iri_maps_into_the_validation_s_resource();
    return boost::report_errors();
}
```

  `messages_test.cpp`'s `a_golden_per_kind` gains a row of the first table:
  `{.schema = R"({"format": "email"})", .instance = "\"nope\"", .message =
  "Expected a string of format \"email\" but found \"nope\"."}`, compiled
  with `format_assertion` on (the table's helper takes the options), and
  its array's size grows by one. Append `webcpp.run format :
  format_test.cpp ;` and the `headers-alone` line for
  `json_schema/formats/default_formats.hpp`. Run
  `$B2 libs/json_schema/test//format`. Expected: FAIL,
  `'webcpp/json_schema/formats/default_formats.hpp' file not found`.
- [ ] **Step 2: Implement** as the Interfaces say.
- [ ] **Step 3: Record every list.** `optional/format/` runs with
  `format_assertion` on since E1.14's runner.

```bash
for l in draft2020-12 draft2019-09 draft7 draft6 draft4 openapi3-0 draft2020-12-output draft2020-12-round-trip; do
  record_known $l || break
done
grep -h '"reason"' libs/json_schema/test/known_failures/*.json | sort | uniq -c
```

  Expected: no `E5: formats` reason left anywhere.
  - A failure the formats track's known-failure list names as a residue of
    the Unicode version (its risk 10) is the one exception.
  - Each such test gets a row in `permanent_reasons()`, written with the
    reason naming both versions, for example
    `{{"draft2020-12", "optional/format/idn-hostname.json", "<case>",
    "<test>"}, "idna's tables are Unicode 15.1; the suite's case needs
    16.0"}`.
  - Then record again.
- [ ] **Step 4: Run the lanes.**

```bash
lanes libs/json_schema/test
lint_clean
```

  Expected: `lanes: green`, `lint: clean`.
- [ ] **Step 5: Commit.**

```bash
cd libs/json_schema
F="build.jam include/webcpp/json_schema/formats/default_formats.hpp include/webcpp/json_schema/formats/format_set.hpp \
include/webcpp/json_schema/validator/options.hpp include/webcpp/json_schema/detail/compiler/opcode.hpp \
include/webcpp/json_schema/detail/compiler/compile.hpp include/webcpp/json_schema/detail/vm/machine.hpp \
meta/include-boundaries.json test/Jamfile test/format_test.cpp test/messages_test.cpp test/suite/runner.hpp \
`git diff --name-only -- test/known_failures | tr '\n' ' '`"
git add $F
$G commit -m "json_schema: formats through webcpp::lexical, as annotations or assertions" -- $F
```

### Task E5.6: User vocabularies

Runs alongside E5.1 to E5.5, E5.7 and E5.8 (slug `vocabularies`).

**Files:**
- Create: `test/vocabulary_test.cpp`.
- Modify:
  - `include/webcpp/json_schema/detail/loader/walk.hpp` (activation,
    `check`, conflicts);
  - `include/webcpp/json_schema/detail/compiler/opcode.hpp`
    (`call_user = 37`), `detail/compiler/compile.hpp`;
  - `include/webcpp/json_schema/detail/vm/machine.hpp`;
  - `include/webcpp/json_schema/validator/schema.hpp` (`validate` returns
    a keyword's own error);
  - `include/webcpp/json_schema/detail/emitter/writer.hpp` (`write`);
  - `test/messages_test.cpp` (the `user` golden), `test/Jamfile`.

**Interfaces** (spec 4.8, declared since E1.8):
- A vocabulary of `compile_options::vocabularies` is active in every
  resource (`always`), or where the resource's meta-schema lists its URI in
  `$vocabulary` (`when_declared`).
- An active user keyword's `check` runs at load; its error refuses the
  load with `invalid_keyword_value`, naming the keyword.
- It loads as `user` with `user_payload{vocabulary, keyword}`, and compiles
  to `call_user`.
- At run time, `validate` returning `false` is an error of kind `user`.
  Returning an error makes `validate()` return it as the result's error,
  `where` the keyword's location, and makes `is_valid` answer `false`.
- With `annotates`, a passing keyword annotates its value.
- The emitter writes it through `write`, else copies the value; into
  OpenAPI 3.0, a copied keyword whose name does not start with `x-` is
  unrepresentable.
- A name that an active official keyword, or another active user keyword,
  already takes is `vocabulary_conflict`.

- [ ] **Step 1: Write the failing test** `test/vocabulary_test.cpp` (spec
  16.3: checked, compiled, failing, erring, annotating, written back,
  conflicting; the E2 to E5 review's I27):

```cpp
#include <webcpp/json_schema.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/json.hpp>

#include <array>
#include <string>

namespace js = webcpp::json_schema;
using keyword = js::basic_user_keyword<js::default_traits>;
using vocabulary = js::basic_vocabulary<js::default_traits>;

namespace {

const js::validator validator;

boost::system::result<void> a_number(const boost::json::value& v) {
    if (v.is_number()) {
        return {};
    }
    return boost::system::errc::make_error_code(boost::system::errc::invalid_argument);
}

boost::system::result<bool> divisible(const js::user_call& call) {
    if (call.instance.as_int64() == 13) {
        return boost::system::errc::make_error_code(boost::system::errc::operation_not_permitted);
    }
    return call.instance.as_int64() % call.value.as_int64() == 0;
}

boost::system::result<boost::json::value> as_multiple_of(const boost::json::value& v, js::dialect,
                                                         const boost::json::storage_ptr& sp) {
    return boost::json::value({{"multipleOf", v}}, sp);   // written as an official keyword's object
}

const std::array<keyword, 1> divisible_by{{
    {.name = "x-divisibleBy", .check = a_number, .validate = divisible,
     .applies_to = js::ir::type_set{js::ir::type_set::integer}, .annotates = true, .write = std::nullopt},
}};

void checked_compiled_failing_erring_annotating() {
    const std::array<vocabulary, 1> v{{{.uri = "urn:example:vocab", .keywords = divisible_by}}};
    const auto s = validator.compile(boost::json::parse(R"({"x-divisibleBy": 3})"), {.vocabularies = v});
    if (!BOOST_TEST(s.has_value())) {
        return;
    }
    BOOST_TEST(s->is_valid(boost::json::parse("9")));
    BOOST_TEST(!s->is_valid(boost::json::parse("10")));
    BOOST_TEST(s->is_valid(boost::json::parse("\"text\"")));   // other types pass
    BOOST_TEST(s->validate(boost::json::parse("10"))->errors()[0].kind() == js::error_kind::user);
    const auto erred = s->validate(boost::json::parse("13"));
    BOOST_TEST(erred.has_error());
    BOOST_TEST_EQ(std::string(erred.error().where), "/x-divisibleBy");
    BOOST_TEST(!s->is_valid(boost::json::parse("13")));
    const auto annotated = s->validate(boost::json::parse("9"), {.what = js::collect::errors_and_annotations});
    BOOST_TEST_EQ(annotated->annotations().size(), 1U);
    BOOST_TEST_EQ(boost::json::serialize(annotated->annotations()[0].value()), "3");
    const auto bad = validator.compile(boost::json::parse(R"({"x-divisibleBy": "3"})"), {.vocabularies = v});
    BOOST_TEST(bad.error().code == js::errc::invalid_keyword_value);
    BOOST_TEST_EQ(std::string(bad.error().what), "x-divisibleBy");
}

void active_only_where_declared() {
    const std::array<vocabulary, 1> v{{{.uri = "urn:example:vocab", .keywords = divisible_by,
                                        .active = js::activation::when_declared}}};
    js::registry registry;
    (void)registry.add("urn:example:meta", boost::json::parse(R"({
        "$schema": "https://json-schema.org/draft/2020-12/schema",
        "$id": "urn:example:meta",
        "$vocabulary": {
            "https://json-schema.org/draft/2020-12/vocab/core": true,
            "https://json-schema.org/draft/2020-12/vocab/validation": true,
            "urn:example:vocab": true
        }
    })"));
    const auto declared = validator.compile(boost::json::parse(R"({"$schema": "urn:example:meta", "x-divisibleBy": 3})"),
                                            {.registry = &registry, .vocabularies = v});
    BOOST_TEST(declared.has_value() && !declared->is_valid(boost::json::parse("10")));
    const auto plain = validator.compile(boost::json::parse(R"({"x-divisibleBy": 3})"), {.vocabularies = v});
    BOOST_TEST(plain.has_value() && plain->is_valid(boost::json::parse("10")));   // not declared: unknown
}

void written_back() {
    const std::array<keyword, 1> renamed{{
        {.name = "x-divisibleBy", .check = a_number, .validate = divisible,
         .applies_to = js::ir::type_set{js::ir::type_set::integer}, .annotates = false, .write = as_multiple_of},
    }};
    const std::array<vocabulary, 1> v{{{.uri = "urn:renamed", .keywords = renamed}}};
    const auto document = validator.load(boost::json::parse(R"({"x-divisibleBy": 3})"), {.vocabularies = v});
    const auto out = js::emit(*document, js::dialect::draft7, {.write_schema_keyword = false});
    BOOST_TEST_EQ(boost::json::serialize(out->schema), R"({"multipleOf":3})");
    const std::array<keyword, 1> plain{{
        {.name = "divisibleBy", .check = a_number, .validate = divisible,
         .applies_to = js::ir::type_set::all(), .annotates = false, .write = std::nullopt},
    }};
    const std::array<vocabulary, 1> copied{{{.uri = "urn:copied", .keywords = plain}}};
    const auto d2 = validator.load(boost::json::parse(R"({"divisibleBy": 3})"), {.vocabularies = copied});
    BOOST_TEST_EQ(boost::json::serialize(js::emit(*d2, js::dialect::draft7, {.write_schema_keyword = false})->schema),
                  R"({"divisibleBy":3})");
    BOOST_TEST(js::emit(*d2, js::dialect::openapi3_0).error().code == js::errc::unrepresentable);
}

void conflicting() {
    const std::array<keyword, 1> clash{{
        {.name = "minimum", .check = a_number, .validate = divisible, .applies_to = js::ir::type_set::all(),
         .annotates = false, .write = std::nullopt},
    }};
    const std::array<vocabulary, 1> official{{{.uri = "urn:clash", .keywords = clash}}};
    BOOST_TEST(validator.compile(boost::json::parse("{}"), {.vocabularies = official}).error().code ==
               js::errc::vocabulary_conflict);
    const std::array<vocabulary, 2> twice{{{.uri = "urn:one", .keywords = divisible_by},
                                           {.uri = "urn:two", .keywords = divisible_by}}};
    BOOST_TEST(validator.compile(boost::json::parse("{}"), {.vocabularies = twice}).error().code ==
               js::errc::vocabulary_conflict);
}

}  // namespace

int main() {
    checked_compiled_failing_erring_annotating();
    active_only_where_declared();
    written_back();
    conflicting();
    return boost::report_errors();
}
```

  `messages_test.cpp`'s first table gains the `user` row (a vocabulary as
  above, the message `"Expected the value to satisfy x-divisibleBy."`, E1.13's
  template for `user`). Append `webcpp.run vocabulary :
  vocabulary_test.cpp ;`. Run it. Expected: FAIL, `x-divisibleBy` loaded
  as `unknown` and every instance valid.
- [ ] **Step 2: Implement** as the Interfaces say.
- [ ] **Step 3: Run the lanes.**

```bash
lanes libs/json_schema/test
lint_clean
```

  Expected: `lanes: green`, `lint: clean`.
- [ ] **Step 4: Commit.**

```bash
cd libs/json_schema
F="include/webcpp/json_schema/detail/loader/walk.hpp include/webcpp/json_schema/detail/compiler/opcode.hpp \
include/webcpp/json_schema/detail/compiler/compile.hpp include/webcpp/json_schema/detail/vm/machine.hpp \
include/webcpp/json_schema/validator/schema.hpp include/webcpp/json_schema/detail/emitter/writer.hpp \
test/vocabulary_test.cpp test/messages_test.cpp test/Jamfile"
git add $F
$G commit -m "json_schema: user vocabularies, checked at load, called by the plan, written by the emitter" -- $F
```

### Task E5.7: The fuzz lane

Runs alongside E5.1 to E5.6 and E5.8 (slug `fuzz`).

**Files:**
- Create:
  - `test/fuzz/Jamfile`, `test/fuzz/bodies.hpp`,
    `test/fuzz/loader_fuzz.cpp`, `test/fuzz/validator_fuzz.cpp`,
    `test/fuzz/seed.hpp`;
  - `test/fuzz/regressions/.gitattributes` (`* -text`);
  - `test/fuzz_regressions_test.cpp`.
- Modify: `test/Jamfile`.

**Interfaces:**
- The own lane `fuzz` (spec 16.6), Linux clang only, about 90 seconds:
  - each fuzzer for 40 seconds, under AddressSanitizer and
    UndefinedBehaviorSanitizer;
  - `threads_test` under ThreadSanitizer (Review Focus 5).
- Nothing of the suite is committed (the licence constraint; the E2 to E5
  review's I20):
  - each fuzzer seeds itself at start, from the suite, into
    `bin/json_schema-fuzz/seed-<name>/` of the superproject, unless that
    directory already holds files;
  - libFuzzer writes what it finds into `bin/json_schema-fuzz/out-<name>/`
    (the first corpus directory, so the seed stays as written) and a crash
    into `bin/json_schema-fuzz/` (`-artifact_prefix`).
- Only `test/fuzz/regressions/` is committed: inputs that once crashed,
  replayed on every target by `fuzz_regressions_test`.

- [ ] **Step 1: Write the regression test first,** with an empty
  `regressions/` (only its `.gitattributes`). `test/fuzz/bodies.hpp`:

```cpp
#ifndef WEBCPP_TEST_JSON_SCHEMA_FUZZ_BODIES_HPP
#define WEBCPP_TEST_JSON_SCHEMA_FUZZ_BODIES_HPP

#include <webcpp/json_schema.hpp>

#include <cstdlib>
#include <string_view>

namespace webcpp::test::fuzz {

// Loads and compiles one schema text; never aborts.
inline void load_one(std::string_view input) {
    const auto parsed = webcpp::json_schema::parse(input);
    if (!parsed) {
        return;
    }
    const webcpp::json_schema::validator validator;
    (void)validator.compile(*parsed);
}

// A schema, a NUL byte, an instance: validates it both ways, and aborts when they disagree (spec 16.7).
inline void validate_one(std::string_view input) {
    const std::size_t nul = input.find('\0');
    if (nul == std::string_view::npos) {
        return;
    }
    const auto schema_text = webcpp::json_schema::parse(input.substr(0, nul));
    const auto instance_text = webcpp::json_schema::parse(input.substr(nul + 1));
    if (!schema_text || !instance_text) {
        return;
    }
    const webcpp::json_schema::validator validator;
    const auto schema = validator.compile(*schema_text);
    if (!schema) {
        return;
    }
    const bool flag = schema->is_valid(instance_text->value);
    const auto outcome = schema->validate(instance_text->value);
    if (outcome && outcome->valid() != flag) {
        std::abort();
    }
}

}  // namespace webcpp::test::fuzz

#endif  // WEBCPP_TEST_JSON_SCHEMA_FUZZ_BODIES_HPP
```

`test/fuzz_regressions_test.cpp`:

```cpp
#include "fuzz/bodies.hpp"
#include "suite/runner.hpp"

#include <boost/core/lightweight_test.hpp>

#include <cstdio>
#include <filesystem>
#include <string>

int main() {
    int replayed = 0;
    for (const auto& entry : std::filesystem::directory_iterator(WEBCPP_TEST_JSON_SCHEMA_REGRESSIONS)) {
        if (entry.path().filename() == ".gitattributes") {
            continue;
        }
        const std::string input = webcpp::test::suite::read_file(entry.path());
        webcpp::test::fuzz::load_one(input);
        webcpp::test::fuzz::validate_one(input);
        ++replayed;
    }
    std::printf("replayed %d regressions\n", replayed);
    return boost::report_errors();
}
```

  Append to `test/Jamfile`:

```
path-constant REGRESSIONS : fuzz/regressions ;
.regressions = [ regex.replace $(REGRESSIONS) "\\\\" "/" ] ;
webcpp.run fuzz_regressions : fuzz_regressions_test.cpp
  : <define>"\"WEBCPP_TEST_JSON_SCHEMA_REGRESSIONS=\\\"$(.regressions)\\\"\""
    [ webcpp.data $(REGRESSIONS) ] ;
```

  Run `$B2 libs/json_schema/test//fuzz_regressions`. Expected: pass,
  printing `replayed 0 regressions`. It is the target the lane's finds
  land in.
- [ ] **Step 2: The fuzzers.** `test/fuzz/seed.hpp` writes the seed once:

```cpp
#ifndef WEBCPP_TEST_JSON_SCHEMA_FUZZ_SEED_HPP
#define WEBCPP_TEST_JSON_SCHEMA_FUZZ_SEED_HPP

#include "../suite/runner.hpp"

#include <filesystem>
#include <fstream>
#include <string>

namespace webcpp::test::fuzz {

// Writes every 2020-12 suite schema (with_instances = false) or each schema, a NUL byte and one of
// its instances, into `directory`, unless it already holds files.
inline void seed(const std::filesystem::path& directory, bool with_instances) {
    std::filesystem::create_directories(directory);
    if (!std::filesystem::is_empty(directory)) {
        return;
    }
    int n = 0;
    suite::for_each_case(WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE, "draft2020-12",
        [&](const std::string&, const boost::json::object& c) {
            const std::string schema = boost::json::serialize(c.at("schema"));
            if (!with_instances) {
                std::ofstream(directory / (std::to_string(n++) + ".json"), std::ios::binary) << schema;
                return;
            }
            for (const auto& t : c.at("tests").as_array()) {
                std::ofstream out(directory / std::to_string(n++), std::ios::binary);
                out << schema << '\0' << boost::json::serialize(t.as_object().at("data"));
            }
        });
}

}  // namespace webcpp::test::fuzz

#endif  // WEBCPP_TEST_JSON_SCHEMA_FUZZ_SEED_HPP
```

`test/fuzz/loader_fuzz.cpp`:

```cpp
#include "bodies.hpp"
#include "seed.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

// libFuzzer reads its options from argv; webcpp.run passes none, so the program gives itself its
// budget, its directories and its seed here. The strings live as long as the program.
extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv) {
    static std::string budget = "-max_total_time=40";
    static std::string artifacts = "-artifact_prefix=" WEBCPP_TEST_JSON_SCHEMA_FUZZ_WORK "/";
    static std::string out = WEBCPP_TEST_JSON_SCHEMA_FUZZ_WORK "/out-loader";
    static std::string seeded = WEBCPP_TEST_JSON_SCHEMA_FUZZ_WORK "/seed-loader";
    std::filesystem::create_directories(out);
    webcpp::test::fuzz::seed(seeded, false);
    static std::array<char*, 6> arguments{(*argv)[0], budget.data(), artifacts.data(), out.data(), seeded.data(),
                                          nullptr};
    *argc = 5;
    *argv = arguments.data();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    webcpp::test::fuzz::load_one(std::string_view(reinterpret_cast<const char*>(data), size));
    return 0;
}
```

  `test/fuzz/validator_fuzz.cpp` is the same with `out-validator`,
  `seed-validator`, `seed(seeded, true)`, and `validate_one` in place of
  `load_one`.

`test/fuzz/Jamfile`:

```
project : requirements <library>/webcpp/json_schema//json_schema ;

import webcpp ;

# libFuzzer with AddressSanitizer and UndefinedBehaviorSanitizer, and the
# threads test under ThreadSanitizer: Linux clang only, in the own lane
# fuzz. Every other platform and toolset skips them; Apple clang has no
# libFuzzer.
path-constant WORK : ../../../../bin/json_schema-fuzz ;
local only-linux-clang = <build>no <target-os>linux,<toolset>clang:<build>yes ;
local fuzz = <cxxflags>-fsanitize=fuzzer,address,undefined <linkflags>-fsanitize=fuzzer,address,undefined
  <library>/webcpp/third_party//json-schema-test-suite
  <define>"\"WEBCPP_TEST_JSON_SCHEMA_FUZZ_WORK=\\\"$(WORK)\\\"\"" ;
webcpp.run loader-fuzz : loader_fuzz.cpp : $(fuzz) $(only-linux-clang) ;
webcpp.run validator-fuzz : validator_fuzz.cpp : $(fuzz) $(only-linux-clang) ;
webcpp.run threads-tsan : ../threads_test.cpp
  : <cxxflags>-fsanitize=thread <linkflags>-fsanitize=thread <threading>multi $(only-linux-clang) ;
webcpp.lane fuzz : loader-fuzz validator-fuzz threads-tsan ;
```

- [ ] **Step 3: The lane, skipped here and run on Linux.** On macOS:

```bash
$B2 -a --build-dir=bin/fuzz libs/json_schema/test/fuzz//fuzz 2>&1 | grep -c 'skipped'
$B2 -d0 declared-lanes | grep json_schema
```

  Expected: `3` (each target skipped, no Apple clang build), and
  `json_schema fuzz libs/json_schema/test/fuzz` among the lanes. Then on
  Linux, in the CI's image, from the task's worktree:

```bash
docker run --rm -v "$PWD:/w" -v "$HOME/dev/webcpp/.local:$HOME/dev/webcpp/.local:ro" -w /w ubuntu:24.04 bash -c '
  apt-get update -qq && apt-get install -y -qq clang-18 libclang-rt-18-dev python3 > /dev/null
  cp -r .local/boost-source/tools/build /tmp/b2 && (cd /tmp/b2 && ./bootstrap.sh > /dev/null)
  echo "using clang : 18 : clang++-18 ;" > /tmp/user-config.jam
  /tmp/b2/b2 --user-config=/tmp/user-config.jam -a --build-dir=bin/fuzz-linux toolset=clang-18 \
    libs/json_schema/test/fuzz//fuzz'
```

  Expected: exit 0 after about two minutes, with each fuzzer's `Done`
  line and the TSan run's `**passed**`. If the superproject's Jamroot needs
  more of a user-config than one `using`, the CI's own-lane job holds what
  it adds: `python3 tools/ci/matrix.py own-lanes --library json_schema`
  prints that job, and the container runs its commands. The ledger records
  the command that ran.
- [ ] **Step 4: A find, if the lane makes one.** A crash leaves
  `bin/json_schema-fuzz/crash-<hash>`. Copy it to
  `test/fuzz/regressions/<short name>`, see `fuzz_regressions` abort on
  every lane, fix the library, see it pass, and commit the input with the
  fix.
- [ ] **Step 5: Run the lanes and commit.**

```bash
lanes libs/json_schema/test
lint_clean
cd libs/json_schema
F="test/fuzz/Jamfile test/fuzz/bodies.hpp test/fuzz/seed.hpp test/fuzz/loader_fuzz.cpp test/fuzz/validator_fuzz.cpp \
test/fuzz/regressions/.gitattributes test/fuzz_regressions_test.cpp test/Jamfile"
git add $F
$G commit -m "json_schema: a fuzz lane over the loader and the validator, with replayed regressions" -- $F
```

  Expected: `lanes: green`, `lint: clean`.

### Task E5.8: The Bowtie harness and its lane

Runs alongside E5.1 to E5.7 (slug `bowtie`). Its lane waits for the owner's
Bowtie pin (decision 4); the harness and its transcript do not.

**Files:**
- Create:
  - `example/bowtie_webcpp.cpp`, `example/bowtie_webcpp.input`,
    `example/bowtie_webcpp.expected`;
  - `bowtie/Dockerfile`, `bowtie/README.md`, `bowtie/run.sh`,
    `bowtie/compare.py`;
  - `test/bowtie/Jamfile`.
- Modify: `example/Jamfile`.

**Interfaces:**
- The IHOP v1 harness (spec 16.8): `start`, `dialect`, `run` with its
  `registry`, `stop`, the flag output; an example, so its transcript is
  checked on every target.
- The own lane `bowtie`, local only: it needs Docker and Bowtie on `PATH`,
  which the CI's own-lane job does not install (AGENTS chapter 9). The
  conformance chapter states it (decision PD9).
- The image builds from a context `run.sh` stages under `bin/`, never from
  the superproject's root, so nothing of `bin/` or `.local` is sent.

- [ ] **Step 1: The transcript first.** `example/bowtie_webcpp.input`:

```
{"cmd": "start", "version": 1}
{"cmd": "dialect", "dialect": "https://json-schema.org/draft/2020-12/schema"}
{"cmd": "run", "seq": 1, "case": {"description": "minimum", "schema": {"minimum": 2}, "tests": [{"description": "1", "instance": 1}, {"description": "3", "instance": 3}]}}
{"cmd": "run", "seq": 2, "case": {"description": "remote", "schema": {"$ref": "http://example.com/int"}, "registry": {"http://example.com/int": {"type": "integer"}}, "tests": [{"description": "a", "instance": "a"}]}}
{"cmd": "run", "seq": 3, "case": {"description": "broken", "schema": {"$ref": "urn:nowhere"}, "tests": [{"description": "x", "instance": 1}]}}
{"cmd": "dialect", "dialect": "http://json-schema.org/draft-04/schema#"}
{"cmd": "run", "seq": 4, "case": {"description": "integer", "schema": {"type": "integer"}, "tests": [{"description": "1.0", "instance": 1.0}]}}
{"cmd": "stop"}
```

  `example/bowtie_webcpp.expected`:

```
{"version":1,"implementation":{"name":"webcpp-json-schema","language":"c++","homepage":"https://webcpporg.github.io/webcpp/libs/json_schema/","issues":"https://github.com/webcpporg/json_schema/issues","source":"https://github.com/webcpporg/json_schema","dialects":["https://json-schema.org/draft/2020-12/schema","https://json-schema.org/draft/2019-09/schema","http://json-schema.org/draft-07/schema#","http://json-schema.org/draft-06/schema#","http://json-schema.org/draft-04/schema#"]}}
{"ok":true}
{"seq":1,"results":[{"valid":false},{"valid":true}]}
{"seq":2,"results":[{"valid":false}]}
{"seq":3,"errored":true,"context":{"message":"no resource for the reference: urn:nowhere"}}
{"ok":true}
{"seq":4,"results":[{"valid":false}]}
```

  Append `webcpp.example bowtie_webcpp.cpp ;` to `example/Jamfile`. Run
  `$B2 libs/json_schema/example//bowtie_webcpp`. Expected: FAIL, the
  source not found.
- [ ] **Step 2: Write the harness** `example/bowtie_webcpp.cpp`:

```cpp
#include <webcpp/json_schema.hpp>

#include <boost/json.hpp>

#include <iostream>
#include <optional>
#include <string>

namespace js = webcpp::json_schema;

namespace {

boost::json::value implementation() {
    boost::json::array dialects;
    for (const js::dialect d : {js::dialect::draft2020_12, js::dialect::draft2019_09, js::dialect::draft7,
                                js::dialect::draft6, js::dialect::draft4}) {
        dialects.emplace_back(js::uri_of(d));
    }
    return {{"name", "webcpp-json-schema"},
            {"language", "c++"},
            {"homepage", "https://webcpporg.github.io/webcpp/libs/json_schema/"},
            {"issues", "https://github.com/webcpporg/json_schema/issues"},
            {"source", "https://github.com/webcpporg/json_schema"},
            {"dialects", std::move(dialects)}};
}

boost::json::value run(const js::validator& validator, std::optional<js::dialect> dialect,
                       const boost::json::object& command) {
    const auto& c = command.at("case").as_object();
    js::registry registry;
    if (const auto* r = c.if_contains("registry")) {
        for (const auto& [uri, schema] : r->as_object()) {
            (void)registry.add(uri, schema);
        }
    }
    const auto schema = validator.compile(c.at("schema"), {.registry = &registry, .default_dialect = dialect});
    if (!schema) {
        std::string message = schema.error().code.message();
        message.append(": ").append(schema.error().what.data(), schema.error().what.size());
        return {{"seq", command.at("seq")}, {"errored", true}, {"context", {{"message", message}}}};
    }
    boost::json::array results;
    for (const auto& t : c.at("tests").as_array()) {
        results.push_back({{"valid", schema->is_valid(t.as_object().at("instance"))}});
    }
    return {{"seq", command.at("seq")}, {"results", std::move(results)}};
}

}  // namespace

int main() {
    // tag::harness[]
    const js::validator validator;
    std::optional<js::dialect> dialect;
    std::string line;
    while (std::getline(std::cin, line)) {
        const boost::json::object command = boost::json::parse(line).as_object();
        const std::string_view cmd = command.at("cmd").as_string();
        boost::json::value answer;
        if (cmd == "start") {
            answer = {{"version", 1}, {"implementation", implementation()}};
        } else if (cmd == "dialect") {
            dialect = js::dialect_of_uri(command.at("dialect").as_string());
            answer = {{"ok", dialect.has_value()}};
        } else if (cmd == "run") {
            answer = run(validator, dialect, command);
        } else {
            return 0;   // stop
        }
        std::cout << boost::json::serialize(answer) << std::endl;   // IHOP reads one line at a time
    }
    // end::harness[]
}
```

  The message is the category's message and the diagnostic's `what`. Run
  the example again. Expected: pass on every lane, its output equal to the
  transcript. The page's testing section (E5.19) shows the `harness` tag.
- [ ] **Step 3: The container, the comparison and the lane.**

`bowtie/Dockerfile`, built from the context `run.sh` stages:

```dockerfile
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)
#
# json_schema's Bowtie harness, built from the context bowtie/run.sh stages.
FROM alpine:3.22 AS build
RUN apk add --no-cache g++ curl
ARG BOOST_SHA256
RUN curl -fsSL https://archives.boost.io/release/1.92.0/source/boost_1_92_0.tar.gz -o /boost.tar.gz \
 && echo "${BOOST_SHA256}  /boost.tar.gz" | sha256sum -c - \
 && tar -xzf /boost.tar.gz -C / && mv /boost_1_92_0 /boost
COPY . /context
WORKDIR /context
RUN g++ -std=c++20 -O2 -DSRELL_NO_UBMOD -I/boost -Iinclude -Isrell \
      bowtie_webcpp.cpp boost_json.cpp /boost/libs/url/src/*.cpp /boost/libs/url/src/*/*.cpp -o /bowtie_webcpp

FROM alpine:3.22
RUN apk add --no-cache libstdc++
COPY --from=build /bowtie_webcpp /usr/local/bin/bowtie_webcpp
CMD ["/usr/local/bin/bowtie_webcpp"]
```

`bowtie/run.sh`:

```bash
#!/bin/sh
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)
#
# Stages the harness's build context, builds its image, and runs Bowtie's
# checks and each dialect's suite against it; fails on any failure that the
# dialect's known-failure list does not name, and on any it names that passes.
set -e
root=$1
here=$root/libs/json_schema/bowtie
pinned=`sed -n 's/^Bowtie: //p' "$here/README.md"`
actual=`bowtie --version | sed 's/^[^0-9]*//'`
[ "$pinned" = "$actual" ] || { echo "run.sh: Bowtie $actual, pinned: $pinned"; exit 1; }
context=$root/bin/json_schema-bowtie
rm -rf "$context" && mkdir -p "$context/include" "$context/srell"
cp -R "$root/libs/json_schema/include/." "$root/libs/lexical/include/." "$root/libs/idna/include/." "$context/include/"
cp -R "$root/third_party/libs/srell/." "$context/srell/"
cp "$root/libs/json_schema/example/bowtie_webcpp.cpp" "$root/tools/boost_json.cpp" "$here/Dockerfile" "$context/"
sha=`sed -n 's/^sha256=//p' "$root/tools/ci/actions/boost/install.sh" | head -1`
docker build -q -t localhost/webcpp-json-schema --build-arg BOOST_SHA256="$sha" "$context"
bowtie smoke -i localhost/webcpp-json-schema
for d in draft2020-12 draft2019-09 draft7 draft6 draft4; do
  bowtie suite -i localhost/webcpp-json-schema "$root/third_party/libs/JSON-Schema-Test-Suite/tests/$d" \
    > "$context/$d.jsonl"
  python3 "$here/compare.py" "$context/$d.jsonl" "$root/libs/json_schema/test/known_failures/$d.json"
done
```

`bowtie/compare.py`:

```python
#!/usr/bin/env python3
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)
"""Compares a Bowtie suite report with a known-failure list: exits 1 on any difference.

A report line that holds a case and its results is read as Bowtie's pinned version writes it
(bowtie/README.md). A test fails when its result differs from the case's expected validity, or
when the case errored. The list is compared by case and test description.
"""

import json
import sys


def failures_of(report_path: str) -> set:
    failed, seen = set(), 0
    with open(report_path, encoding="utf-8") as report:
        for line in report:
            record = json.loads(line)
            case = record.get("case")
            result = record.get("result") or {}
            if not isinstance(case, dict) or "tests" not in case:
                continue
            seen += 1
            results = result.get("results")
            for i, test in enumerate(case["tests"]):
                got = results[i].get("valid") if results and i < len(results) else None
                if got is None or got != test.get("valid"):
                    failed.add((case["description"], test["description"]))
    if seen == 0:
        sys.exit(f"compare.py: no case in {report_path}; has the report's form changed?")
    return failed


def main() -> None:
    failed = failures_of(sys.argv[1])
    with open(sys.argv[2], encoding="utf-8") as known_file:
        known = {(e["case"], e["test"]) for e in json.load(known_file)}
    for case, test in sorted(failed - known):
        print(f"unexpected failure: {case} / {test}")
    for case, test in sorted(known - failed):
        print(f"listed, passes: {case} / {test}")
    sys.exit(0 if failed == known else 1)


if __name__ == "__main__":
    main()
```

`test/bowtie/Jamfile`:

```
import notfile ;
import webcpp ;

# Bowtie's run against the harness's container: Docker, and Bowtie at the
# version bowtie/README.md pins, on PATH. A local lane: the CI's own-lane
# job installs neither.
path-constant ROOT : ../../../.. ;
notfile bowtie-run : @bowtie-run ;
actions bowtie-run
{
    sh "$(ROOT)/libs/json_schema/bowtie/run.sh" "$(ROOT)"
}
webcpp.lane bowtie : bowtie-run ;
```

  `bowtie/README.md` says what the lane runs, how to run it, and holds the
  line `Bowtie: unpinned` until the owner's pin replaces it; it says that a
  pull request to bowtie-json-schema/bowtie waits for the owner's go.
- [ ] **Step 4: Run what can run now.**

```bash
lanes libs/json_schema/test libs/json_schema/example
lint_clean
$B2 -a libs/json_schema/test/bowtie//bowtie 2>&1 | tail -2
```

  Expected: `lanes: green`, `lint: clean` (the Python lint reads
  `compare.py`), and, before the pin, `run.sh: Bowtie <version>, pinned:
  unpinned` with the lane failing. Once the owner pins it, the controller
  writes the version into the README and runs the lane: `bowtie smoke`
  clean, and no `unexpected failure` or `listed, passes` line.
- [ ] **Step 5: Commit.**

```bash
cd libs/json_schema
F="example/Jamfile example/bowtie_webcpp.cpp example/bowtie_webcpp.input example/bowtie_webcpp.expected \
bowtie/Dockerfile bowtie/README.md bowtie/run.sh bowtie/compare.py test/bowtie/Jamfile"
git add $F
$G commit -m "json_schema: a Bowtie harness over IHOP, tested by its transcript, and a local bowtie lane" -- $F
```

**The merges of the parallel group** (controller), in `~/dev/webcpp-json-schema-e5`:

```bash
for slug in draft2019-09 drafts-7-6 draft4 openapi formats vocabularies fuzz bowtie; do
  merge_e5 $slug || break
done
```

  Expected: for each, `lanes: green` and `lint: clean`, and no
  `record_all: ... gained` line. A conflict in a shared file is resolved
  by keeping both sides, then `merge_e5` is run again for that slug.

### Task E5.9: Content

After the merges of the parallel group.

**Files:**
- Create: `include/webcpp/json_schema/detail/content/base64.hpp`,
  `test/content_test.cpp`, `test/base64_test.cpp`.
- Modify:
  - `include/webcpp/json_schema/detail/compiler/opcode.hpp`
    (`assert_content = 38`), `detail/compiler/compile.hpp`;
  - `include/webcpp/json_schema/detail/vm/machine.hpp`;
  - `test/suite/runner.hpp` (`content_assertion` for
    `optional/content.json`);
  - `test/messages_test.cpp` (the `content` golden);
  - `test/known_failures/draft7.json`, `test/Jamfile`.

**Interfaces:**
- `detail::base64_decode(std::string_view, std::pmr::memory_resource&) ->
  std::optional<std::pmr::string>`: RFC 4648 section 4's alphabet, padding
  required, whitespace refused, over a 256-entry `constexpr` table.
- `assert_content`, with `content_assertion` on, on strings only:
  - decodes `base64` into the validation's resource, and fails on bad
    input;
  - parses `application/json` with Boost.JSON into the validation's
    storage, with `parse_options::max_depth` clamped as `js::parse` does;
  - runs `contentSchema`'s block on the decoded value, which lives on the
    machine's stack of decoded values until its frame returns;
  - a failure is kind `content`.
- Without `content_assertion`, the three keywords annotate (spec 10).

- [ ] **Step 1: Write the failing tests.** `test/base64_test.cpp`:

```cpp
#include <webcpp/json_schema/detail/content/base64.hpp>

#include <boost/core/lightweight_test.hpp>

#include <memory_resource>
#include <string>

namespace js = webcpp::json_schema;

namespace {

std::string decoded(std::string_view text) {
    std::pmr::monotonic_buffer_resource scratch;
    const auto out = js::detail::base64_decode(text, scratch);
    return out ? std::string(*out) : std::string("!");
}

}  // namespace

int main() {
    BOOST_TEST_EQ(decoded(""), "");
    BOOST_TEST_EQ(decoded("YQ=="), "a");
    BOOST_TEST_EQ(decoded("YWI="), "ab");
    BOOST_TEST_EQ(decoded("YWJj"), "abc");
    BOOST_TEST_EQ(decoded("YQ"), "!");       // padding required
    BOOST_TEST_EQ(decoded("Y Q=="), "!");    // whitespace refused
    BOOST_TEST_EQ(decoded("YQ=a"), "!");     // padding only at the end
    BOOST_TEST_EQ(decoded("!!!!"), "!");
    BOOST_TEST_EQ(decoded("+/+/"), "\xfb\xff\xbf");
    return boost::report_errors();
}
```

`test/content_test.cpp`:

```cpp
#include <webcpp/json_schema.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/json/parse.hpp>

namespace js = webcpp::json_schema;

namespace {

const js::validator validator;

bool valid(std::string_view schema, std::string_view instance, bool assertion = true) {
    const auto s = validator.compile(boost::json::parse(schema), {.content_assertion = assertion});
    return s.has_value() && s->is_valid(boost::json::parse(instance));
}

}  // namespace

int main() {
    BOOST_TEST(!valid(R"({"contentEncoding": "base64"})", R"("!!")"));
    BOOST_TEST(valid(R"({"contentEncoding": "base64"})", R"("YQ==")"));
    BOOST_TEST(!valid(R"({"contentMediaType": "application/json"})", R"("{")"));
    const char* both = R"({"contentEncoding": "base64", "contentMediaType": "application/json",
                           "contentSchema": {"required": ["a"]}})";
    BOOST_TEST(valid(both, R"("eyJhIjoxfQ==")"));    // {"a":1}
    BOOST_TEST(!valid(both, R"("e30=")"));           // {}
    BOOST_TEST(valid(R"({"contentEncoding": "base64"})", R"("!!")", false));
    BOOST_TEST(valid(both, R"("e30=")", false));
    BOOST_TEST(valid(R"({"contentEncoding": "base64"})", "12"));   // strings only
    std::string deep(300, '[');
    deep += std::string(300, ']');
    BOOST_TEST(!valid(R"({"contentMediaType": "application/json"})", boost::json::serialize(boost::json::value(deep))));
    return boost::report_errors();
}
```

  The runner's `run_case` sets `compile.content_assertion = file ==
  "optional/content.json";`. `messages_test.cpp`'s first table gains the
  row `{.schema = R"({"contentEncoding": "base64"})", .instance =
  "\"!!\"", .message = "Expected content that decodes and matches its schema."}` under
  `content_assertion`. Append `webcpp.run base64 : base64_test.cpp ;` and
  `webcpp.run content : content_test.cpp ;`. Run them. Expected: FAIL,
  `base64.hpp` not found.
- [ ] **Step 2: Implement.**
- [ ] **Step 3: Record, and run the lanes.**

```bash
record_all
python3 -c "import json; print(sorted({e['reason'] for e in json.load(open('libs/json_schema/test/known_failures/draft7.json'))}))"
lanes libs/json_schema/test
lint_clean
```

  Expected: `record_all` gains nothing; draft 7's reasons are
  `['E5: other dialects']` at most (cross-draft, until E5.11), or `[]`;
  `lanes: green`; `lint: clean`.
- [ ] **Step 4: Commit.**

```bash
cd libs/json_schema
F="include/webcpp/json_schema/detail/content/base64.hpp include/webcpp/json_schema/detail/compiler/opcode.hpp \
include/webcpp/json_schema/detail/compiler/compile.hpp include/webcpp/json_schema/detail/vm/machine.hpp \
test/base64_test.cpp test/content_test.cpp test/suite/runner.hpp test/messages_test.cpp test/Jamfile \
`git diff --name-only -- test/known_failures | tr '\n' ' '`"
git add $F
$G commit -m "json_schema: content assertion, base64 and JSON decoded into the validation's allocator" -- $F
```

### Task E5.10: Allocation, over every dialect, formats and patterns

After E5.9.

**Files:** Modify `test/allocation_test.cpp`.

- [ ] **Step 1: Extend the test.** `read_samples` takes the dialect's
  directory and tag, and `main` runs every dialect's required and optional
  suites with `format_assertion` and `content_assertion` on. Then it adds
  the SRELL spike's patterns (`~/.buzz/RESEARCH/SRELL_ALLOCATION_SPIKE.md`,
  section 5, recommendation 4), each compiled into the schema's resource
  and searched on 1 KB and 100 KB subjects. The ninth of the spike's
  patterns, `[\p{L}--[a-z]]+` under the `v` and `i` flags, is not a JSON
  Schema pattern (no `pattern` sets a flag); the mirror's CI keeps it (S3).

```cpp
struct pattern_case {
    std::string pattern;
    std::string subject_1k;
    std::string subject_100k;
};

std::string alternation200() {
    constexpr std::array<std::string_view, 36> words{
        "alpha", "bravo", "charlie", "delta", "echo", "foxtrot", "golf", "hotel", "india", "juliett", "kilo",
        "lima", "mike", "november", "oscar", "papa", "quebec", "romeo", "sierra", "tango", "uniform", "victor",
        "whiskey", "xray", "yankee", "zulu", "amber", "basalt", "cobalt", "dolomite", "emerald", "feldspar",
        "garnet", "hematite", "iolite", "jasper"};
    std::string s = "(?:";
    for (const std::string_view w : words) {
        if (s.size() + w.size() + 2 > 200) {
            break;
        }
        if (s.size() > 3) {
            s += '|';
        }
        s += w;
    }
    return s + ")";
}

std::string ending(std::size_t n, char fill, std::string_view tail) {
    return std::string(n - tail.size(), fill) + std::string(tail);
}

std::vector<pattern_case> spike_patterns() {   // before counting starts
    std::vector<pattern_case> out;
    auto add = [&](std::string pattern, char fill, std::string_view tail) {
        out.push_back({.pattern = std::move(pattern), .subject_1k = ending(1024, fill, tail),
                       .subject_100k = ending(102400, fill, tail)});
    };
    add("^[a-z]+$", 'a', "");
    add("\\p{Letter}+", 'z', "\xc3\xa9");
    add(alternation200(), 'x', "zulu");
    add("^(?=.*\\d)(?=.*[a-z]).{8,}$", 'a', "1");
    add("(\\d{4})-(\\d{2})-(\\d{2})", 'x', "2026-10-09");
    add("xyzzy", 'x', "xyzzy");
    add("^(?:a|ab)*$", 'a', "!");
    add("(?<y>\\d{4})-(?<m>\\d{2})-(?<d>\\d{2})", 'x', "2026-10-09");
    return out;
}

// schemas[i] is {"pattern": cases[i].pattern}; subjects[2i] and [2i+1] its 1 KB and 100 KB subjects.
void patterns(const js::validator& validator, std::pmr::polymorphic_allocator<std::byte> schema_alloc,
              std::pmr::polymorphic_allocator<std::byte> validation_alloc,
              const std::vector<boost::json::value>& schemas, const std::vector<boost::json::value>& subjects) {
    for (std::size_t i = 0; i < schemas.size(); ++i) {
        const auto compiled = validator.compile(schemas[i], {}, schema_alloc);
        if (!BOOST_TEST(compiled.has_value())) {
            continue;
        }
        (void)compiled->is_valid(subjects[2 * i], {}, validation_alloc);
        (void)compiled->is_valid(subjects[2 * i + 1], {}, validation_alloc);
    }
}
```

  `main` builds `schemas` and `subjects` from `spike_patterns()` before
  `counting = true`, since Boost.JSON's default resource is the global
  heap, and calls `patterns` while `counting` is set, after the suites.
  So the only allocations counted are the library's.

  Expected after Step 2: `global_allocations` is 0, and
  `validation_memory.peak` holds the 100 KB searches' backtracking stacks
  (about 4 MiB for `^(?:a|ab)*$`, the spike's figure).
- [ ] **Step 2: Run it on the lanes.**

```bash
lanes libs/json_schema/test
grep -h 'allocation' bin/lane-*.log | head
lint_clean
```

  Expected: `lanes: green`; `lint: clean`. A global allocation is a bug
  of the path that made it: fix it there, never in the test.
- [ ] **Step 3: Commit.**

```bash
cd libs/json_schema
git add test/allocation_test.cpp
$G commit -m "json_schema: no global allocation across every dialect, formats, content and the SRELL patterns" \
  -- test/allocation_test.cpp
```

### Task E5.11: Round trips for every dialect, annotations and output of 2019-09

After E5.10.

**Files:**
- Create:
  - `test/suite/suite_round_trip.cpp` (one program for the five
    same-dialect round trips, its dialect from `argv`; see below);
  - `test/suite/suite_cross_dialect.cpp`;
  - `test/known_failures/draft2019-09-round-trip.json`,
    `draft7-round-trip.json`, `draft6-round-trip.json`,
    `draft4-round-trip.json`, `openapi3-0-round-trip.json` (recorded).
- Modify:
  - `test/suite/runner.hpp` (the round trip into OpenAPI 3.0 through a
    registry);
  - `test/annotations_test.cpp` (every dialect);
  - `test/output_suite_test.cpp` (2019-09);
  - `test/Jamfile`;
  - every list, by `record_all`.

**Interfaces:**
- Spec 16.4: every dialect the runner runs round-trips into itself with
  `ir::equivalent` and unchanged verdicts (the E2 to E5 review's I18), and
  every pair of dialects round-trips with unchanged verdicts where the
  emit succeeds.
- Into OpenAPI 3.0 (the review's I17), the emitted `components` cannot sit
  in the reloaded root, whose meta-schema refuses `components`. The round
  trip registers `{"components": {"schemas": <components>}}` under
  `urn:webcpp:components`, rewrites each `#/components/...` reference of
  the emitted schema to `urn:webcpp:components#/components/...` (a walk
  over its strings, iterative), and reloads with that registry.

- [ ] **Step 1: The same-dialect round trips.** One program per list, as
  E4.4's, so that each has its own list and its own line in the lanes.
  `test/suite/suite_round_trip.cpp` is compiled five times with two
  defines:

```cpp
#include "runner.hpp"

#include <webcpp/json_schema.hpp>

#include <string_view>

namespace js = webcpp::json_schema;

int main(int argc, char** argv) {
    const js::validator validator;
    constexpr auto target = js::dialect::WEBCPP_TEST_JSON_SCHEMA_DIALECT_TAG;
    webcpp::test::suite::run_options options{
        .root = WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE, .dialect = WEBCPP_TEST_JSON_SCHEMA_DIALECT_DIR,
        .known_failures = WEBCPP_TEST_JSON_SCHEMA_KNOWN, .record = argc > 1 && std::string_view(argv[1]) == "--record",
        .round_trip = target, .default_dialect = target};
    if constexpr (target == js::dialect::openapi3_0) {
        return webcpp::test::suite::run_openapi(validator, options, WEBCPP_TEST_JSON_SCHEMA_OPENAPI);
    } else {
        return webcpp::test::suite::run(validator, options);
    }
}
```

  `test/Jamfile` gains a rule beside `suite-runner`:

```
rule round-trip-runner ( dir : tag )
{
    webcpp.run suite-$(dir)-round-trip : suite/suite_round_trip.cpp
      : <library>/webcpp/third_party//json-schema-test-suite
        <define>WEBCPP_TEST_JSON_SCHEMA_DIALECT_TAG=$(tag)
        <define>"\"WEBCPP_TEST_JSON_SCHEMA_DIALECT_DIR=\\\"$(dir)\\\"\""
        <define>"\"WEBCPP_TEST_JSON_SCHEMA_KNOWN=\\\"$(.known)/$(dir)-round-trip.json\\\"\""
        <define>"\"WEBCPP_TEST_JSON_SCHEMA_OPENAPI=\\\"$(.openapi)\\\"\""
        [ webcpp.data $(KNOWN) $(OPENAPI) ]
        <dependency>known_failures/$(dir)-round-trip.json ;
}

round-trip-runner draft2019-09 : draft2019_09 ;
round-trip-runner draft7 : draft7 ;
round-trip-runner draft6 : draft6 ;
round-trip-runner draft4 : draft4 ;
round-trip-runner openapi3-0 : openapi3_0 ;
```

  The round trip into OpenAPI 3.0, in `run_case`'s round-trip block, when
  the target is `openapi3_0`:

```cpp
inline void point_into(boost::json::value& schema, std::string_view uri) {
    std::vector<boost::json::value*> work{&schema};
    while (!work.empty()) {
        boost::json::value* v = work.back();
        work.pop_back();
        if (auto* object = v->if_object()) {
            for (auto& member : *object) {
                if (member.key() == "$ref" && member.value().is_string() &&
                    member.value().get_string().starts_with("#/components/")) {
                    member.value() = std::string(uri) + std::string(member.value().get_string());
                } else {
                    work.push_back(&member.value());
                }
            }
        } else if (auto* array = v->if_array()) {
            for (auto& item : *array) {
                work.push_back(&item);
            }
        }
    }
}
```

  `run_case` then, for OpenAPI 3.0, builds `boost::json::object
  wrapper{{"components", {{"schemas", out->components}}}}`, adds it to a
  local `js::registry` under `urn:webcpp:components`, calls
  `point_into(out->schema, "urn:webcpp:components")`, and reloads with
  `again.registry = &registry`. The `$ref`s of `components` itself are
  rewritten the same way before the registry takes it.

  Create the five lists as `[]`, run them, and record:

```bash
for l in draft2019-09 draft7 draft6 draft4 openapi3-0; do record_known $l-round-trip || break; done
grep -h '"reason"' libs/json_schema/test/known_failures/*-round-trip.json | sort | uniq -c
```

  Expected: at most `E5: other dialects` reasons (the cross-draft cases),
  and in `openapi3-0-round-trip` none. A `not equivalent` or `emit failed`
  failure is not recordable: it is the emitter's or `equivalent`'s to fix,
  and then the rule of section E4 grows with it.
- [ ] **Step 2: Between dialects.** `test/suite/suite_cross_dialect.cpp`:

```cpp
#include "runner.hpp"

#include <webcpp/json_schema.hpp>

#include <array>
#include <iostream>
#include <string>

namespace js = webcpp::json_schema;

namespace {

struct pair_count {
    int emitted = 0;
    int unrepresentable = 0;
    int changed = 0;
};

void one_case(const js::validator& validator, const boost::json::object& c, js::dialect source, js::dialect target,
              pair_count& n) {
    const auto original = validator.compile(c.at("schema"), {.default_dialect = source});
    if (!original) {
        return;   // a known failure of the source dialect, listed there
    }
    auto out = js::emit(original->ir(), target);
    if (!out) {
        ++n.unrepresentable;
        return;
    }
    js::registry registry;
    if (target == js::dialect::openapi3_0) {
        boost::json::value components = boost::json::object{{"components", {{"schemas", out->components}}}};
        webcpp::test::suite::point_into(components, "urn:webcpp:components");
        webcpp::test::suite::point_into(out->schema, "urn:webcpp:components");
        (void)registry.add("urn:webcpp:components", components);
    }
    const auto reloaded = validator.compile(out->schema, {.registry = &registry, .default_dialect = target});
    ++n.emitted;
    for (const auto& t : c.at("tests").as_array()) {
        const auto& data = t.as_object().at("data");
        if (!reloaded || reloaded->is_valid(data) != original->is_valid(data)) {
            ++n.changed;
            std::cout << "changed: " << js::uri_of(source) << " -> " << js::uri_of(target) << ": "
                      << c.at("description").as_string() << '\n';
            return;
        }
    }
}

}  // namespace

int main() {
    const js::validator validator;
    constexpr std::array<std::pair<const char*, js::dialect>, 5> sources{{
        {"draft2020-12", js::dialect::draft2020_12}, {"draft2019-09", js::dialect::draft2019_09},
        {"draft7", js::dialect::draft7}, {"draft6", js::dialect::draft6}, {"draft4", js::dialect::draft4}}};
    constexpr std::array<js::dialect, 6> targets{js::dialect::draft2020_12, js::dialect::draft2019_09,
                                                 js::dialect::draft7, js::dialect::draft6, js::dialect::draft4,
                                                 js::dialect::openapi3_0};
    int changed = 0;
    for (const auto& [directory, source] : sources) {
        for (const js::dialect target : targets) {
            if (target == source) {
                continue;
            }
            pair_count n;
            webcpp::test::suite::for_each_case(WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE, directory,
                [&](const std::string& file, const boost::json::object& c) {
                    if (file.find('/') == std::string::npos) {   // required tests only
                        one_case(validator, c, source, target, n);
                    }
                });
            std::cout << "round trip " << directory << " -> " << js::uri_of(target) << ": " << n.emitted
                      << " emitted, " << n.unrepresentable << " unrepresentable, " << n.changed << " changed\n";
            changed += n.changed;
        }
    }
    return changed == 0 ? 0 : 1;
}
```

  `js::uri_of(dialect::openapi3_0)` is `"openapi3_0"` (E1.9), so each line
  names its target. No list: a changed verdict is a failure, and an
  unrepresentable case is counted, not failed. Append:

```
webcpp.run suite-cross-dialect : suite/suite_cross_dialect.cpp
  : <library>/webcpp/third_party//json-schema-test-suite ;
```

  Run it. Expected: FAIL on whatever pair changes a verdict; fix the
  loader or the emitter until every line ends `0 changed`.
- [ ] **Step 3: Annotations for every dialect, and 2019-09's output
  tests.** `annotations_test.cpp`'s `main` becomes a loop over `(dialect,
  version)`: `(draft2020_12, 2020)`, `(draft2019_09, 2019)`, `(draft7, 7)`,
  `(draft6, 6)`, `(draft4, 4)`, each compiling with `default_dialect` and
  filtering by `admits_all(compatibility, version)`.
  `resource_pointers` reads `id` for draft 4 in place of `$id`, as a
  parameter. The count of assertions is checked per dialect (`> 0`).
  `output_suite_test.cpp` loops over `draft2019-09` and `draft2020-12`,
  each with its own output schema registered and `default_dialect` set.

  Run both. Expected: pass. A failing annotation case in drafts 4 to 7 is
  checked against decision PD2 first: an unknown keyword there is inert.
- [ ] **Step 4: Record every list and run the lanes.**

```bash
record_all
grep -h '"reason"' libs/json_schema/test/known_failures/*.json | sort | uniq -c
lanes libs/json_schema/test
lint_clean
```

  Expected: `record_all` gains nothing; the `E5: other dialects` entries
  are gone from every list (cross-draft now runs as a user's program would,
  every dialect in one validator); `lanes: green`; `lint: clean`.
- [ ] **Step 5: Commit.**

```bash
cd libs/json_schema
F="test/suite/runner.hpp test/suite/suite_round_trip.cpp test/suite/suite_cross_dialect.cpp \
test/annotations_test.cpp test/output_suite_test.cpp test/Jamfile \
test/known_failures/draft2019-09-round-trip.json test/known_failures/draft7-round-trip.json \
test/known_failures/draft6-round-trip.json test/known_failures/draft4-round-trip.json \
test/known_failures/openapi3-0-round-trip.json `git diff --name-only -- test/known_failures | tr '\n' ' '`"
git add $F
$G commit -m "json_schema: every dialect round-trips into itself and into the others, verdicts unchanged" -- $F
```

### Task E5.12: Two plans, `explain()`, and `enum` as a hash table

After E5.11. The first of three optimization tasks (the E2 to E5 review's
I26).

**Decision PD10, what an optimization may change** (the review's I19).
`validate`'s errors and their order are pinned by `validate_test`,
`messages_test` and the output goldens, and an optimization that drops,
fuses or reorders instructions would change them. So the compiler builds
two block lists over one instruction pool:
- **the record plan**, which `validate` runs: only the optimizations that
  change nothing recorded (a large `enum` hashed, a small `$ref` target
  inlined with its evaluation path kept, the static `unevaluated*` filter);
- **the flag plan**, which `is_valid` runs: those and the ones that change
  what would be recorded (fused type checks, folded `true` and `false`,
  a one-member `allOf` inlined, cheap checks first, `required` fused into
  the property loop).

A block that no flag-only optimization changed is shared by both lists, so
the second costs only the blocks it rewrites. `machine<Traits, Sink>` picks
its list with `if constexpr (Sink::short_circuits)`.

**Files:**
- Create:
  - `include/webcpp/json_schema/detail/compiler/optimize.hpp`,
    `detail/compiler/explain.hpp`;
  - `include/webcpp/json_schema/detail/json/hash.hpp`;
  - `test/explain_test.cpp`, `test/hash_test.cpp`.
- Modify:
  - `include/webcpp/json_schema/detail/compiler/plan.hpp` (the two
    lists), `detail/compiler/opcode.hpp` (`assert_enum_hash = 39`),
    `detail/compiler/compile.hpp`;
  - `include/webcpp/json_schema/detail/vm/machine.hpp`;
  - `include/webcpp/json_schema/validator/schema.hpp` (`explain`);
  - `test/Jamfile`.

**Interfaces:**
- `basic_string<Traits> basic_schema::explain(allocator_type = {}) const`
  (spec 4.6): the flag plan's text.
- `detail::compile_plan(document, allocator, bool optimize = true)`, and
  `template <class Traits> std::pmr::string detail::explain(const
  ir::basic_document<Traits>&, const plan<Traits>&, plan_list which)`,
  `enum class plan_list : std::uint8_t { record, flag };`.
- The text: a block is a line `block <n> <resource URI>#<node pointer>`;
  each instruction a line of two spaces, its index, its opcode's name, its
  operands in words, ` ; ` and its absolute keyword location; blocks in
  order.
- `detail::hash<Number>(const boost::json::value&, std::pmr::memory_resource&)`,
  consistent with `detail::equal`: a number hashes its normalized
  decomposition, an object combines its members' hashes independently of
  order; iterative.
- An `enum` of more than 8 values compiles to `assert_enum_hash` over a
  table of the values' hashes, each bucket confirmed with `detail::equal`.

- [ ] **Step 1: Write the failing tests.** `test/explain_test.cpp`:

```cpp
#include <webcpp/json_schema.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/json/parse.hpp>

#include <string>

namespace js = webcpp::json_schema;

namespace {

const js::validator validator;

std::string plan(std::string_view schema, bool optimize, js::detail::plan_list which = js::detail::plan_list::flag) {
    const auto document = validator.load(boost::json::parse(schema));
    const auto compiled = js::detail::compile_plan(*document, {}, optimize);
    return std::string(js::detail::explain(*document, *compiled, which));
}

void the_text() {
    BOOST_TEST_EQ(plan(R"({"type": "string", "maxLength": 3})", false),
                  "block 0 urn:webcpp:root#\n"
                  "  0 assert_type string ; urn:webcpp:root#/type\n"
                  "  1 type_guard string 1 ; urn:webcpp:root#/maxLength\n"
                  "  2 assert_length max 3 ; urn:webcpp:root#/maxLength\n"
                  "  3 ret\n");
    const auto schema = validator.compile(boost::json::parse(R"({"type": "string", "maxLength": 3})"));
    BOOST_TEST_EQ(std::string(schema->explain()), plan(R"({"type": "string", "maxLength": 3})", true));
}

void a_large_enum_is_hashed_in_both_plans() {
    const char* nine = R"({"enum": [1, 2, 3, 4, 5, 6, 7, 8, 9]})";
    BOOST_TEST(plan(nine, true).find("assert_enum_hash") != std::string::npos);
    BOOST_TEST(plan(nine, true, js::detail::plan_list::record).find("assert_enum_hash") != std::string::npos);
    BOOST_TEST(plan(R"({"enum": [1, 2]})", true).find("assert_enum_hash") == std::string::npos);
    const auto s = validator.compile(boost::json::parse(nine));
    BOOST_TEST(s->is_valid(boost::json::parse("9.0")));
    BOOST_TEST(!s->is_valid(boost::json::parse("10")));
    BOOST_TEST_EQ(s->validate(boost::json::parse("10"))->errors()[0].kind(), js::error_kind::enum_value);
}

}  // namespace

int main() {
    the_text();
    a_large_enum_is_hashed_in_both_plans();
    return boost::report_errors();
}
```

`test/hash_test.cpp`:

```cpp
#include <webcpp/json_schema/detail/json/hash.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/decimal.hpp>
#include <boost/json/parse.hpp>

#include <memory_resource>
#include <string>
#include <unordered_set>

namespace js = webcpp::json_schema;
using d64 = boost::decimal::decimal64_t;

namespace {

std::size_t h(std::string_view text) {
    std::pmr::monotonic_buffer_resource scratch;
    return js::detail::hash<d64>(boost::json::parse(text), scratch);
}

}  // namespace

int main() {
    BOOST_TEST_EQ(h("1"), h("1.0"));
    BOOST_TEST_EQ(h("-0.0"), h("0"));
    BOOST_TEST_EQ(h(R"({"a": 1, "b": [true, null]})"), h(R"({"b": [true, null], "a": 1.0})"));
    BOOST_TEST_NE(h("[1, 2]"), h("[2, 1]"));
    BOOST_TEST_NE(h("18446744073709551615"), h("18446744073709551616"));
    std::unordered_set<std::size_t> seen;
    int collisions = 0;
    for (int i = 0; i < 10000; ++i) {
        const std::string value = i % 2 == 0 ? std::to_string(i) : "\"" + std::to_string(i) + "\"";
        collisions += seen.insert(h(value)).second ? 0 : 1;
    }
    BOOST_TEST(collisions < 10);
    return boost::report_errors();
}
```

  Append `webcpp.run explain : explain_test.cpp ;` and `webcpp.run hash :
  hash_test.cpp ;`. Run them. Expected: FAIL, `explain.hpp` not found.
- [ ] **Step 2: Implement** the two lists, the text and the hashed `enum`.
- [ ] **Step 3: The suites are the oracle** (spec 7.2):

```bash
lanes libs/json_schema/test
grep -h 'suite \|round trip' bin/lane-clang.log
lint_clean
```

  Expected: `lanes: green`, every runner `0 unexpected, 0 stale`, no list
  recorded again, `lint: clean`. A changed verdict or a changed golden is
  the optimization's bug, never a list entry.
- [ ] **Step 4: Commit.**

```bash
cd libs/json_schema
F="include/webcpp/json_schema/detail/compiler/optimize.hpp include/webcpp/json_schema/detail/compiler/explain.hpp \
include/webcpp/json_schema/detail/compiler/plan.hpp include/webcpp/json_schema/detail/compiler/opcode.hpp \
include/webcpp/json_schema/detail/compiler/compile.hpp include/webcpp/json_schema/detail/json/hash.hpp \
include/webcpp/json_schema/detail/vm/machine.hpp include/webcpp/json_schema/validator/schema.hpp \
test/explain_test.cpp test/hash_test.cpp test/Jamfile"
git add $F
$G commit -m "json_schema: a record plan and a flag plan, explain(), and a large enum as a hash table" -- $F
```

### Task E5.13: The flag plan's optimizations

After E5.12.

**Files:**
- Modify:
  - `include/webcpp/json_schema/detail/compiler/optimize.hpp`,
    `detail/compiler/opcode.hpp` (`assert_type_and_guard = 40`);
  - `include/webcpp/json_schema/detail/vm/machine.hpp`;
  - `test/explain_test.cpp`.

**Interfaces** (the flag plan only, PD10), each a pass over a block, run in
this order:
1. **`true` and `false` folded:** an `allOf` member `true` vanishes, a
   `not` of `false` vanishes, an `anyOf` holding `true` is `true`; then an
   `allOf` of one member is inlined.
2. **Fused type checks:** `assert_type` followed by a `type_guard` of one
   of its types becomes `assert_type_and_guard`, which fails and skips the
   guarded keywords at once.
3. **`required` fused with `properties`:** the names both list are checked
   in the property loop, and leave `assert_required`.
4. **Cheap checks first:** type, then `const` and `enum`, then numeric,
   then lengths, then the rest. A block that holds an `unevaluated*`
   contributor keeps its order, since annotations depend on it.

- [ ] **Step 1: Write the failing tests.** Append to `explain_test.cpp`,
  called from `main`:

```cpp
void fused_type_checks() {
    BOOST_TEST_EQ(plan(R"({"type": "string", "maxLength": 3})", true),
                  "block 0 urn:webcpp:root#\n"
                  "  0 assert_type_and_guard string 1 ; urn:webcpp:root#/type\n"
                  "  1 assert_length max 3 ; urn:webcpp:root#/maxLength\n"
                  "  2 ret\n");
    BOOST_TEST_EQ(plan(R"({"type": "string", "maxLength": 3})", true, js::detail::plan_list::record),
                  plan(R"({"type": "string", "maxLength": 3})", false));
}

void folding() {
    BOOST_TEST(plan(R"({"allOf": [true, {"minimum": 1}]})", true).find("all_of") == std::string::npos);
    BOOST_TEST(plan(R"({"allOf": [true, {"minimum": 1}]})", true, js::detail::plan_list::record).find("all_of") !=
               std::string::npos);
    BOOST_TEST(plan(R"({"not": false, "minimum": 1})", true).find("not_") == std::string::npos);
}

void cheap_first_and_required() {
    const std::string p = plan(R"({"pattern": "^a", "minLength": 2, "type": "string"})", true);
    BOOST_TEST(p.find("assert_type_and_guard") < p.find("assert_length"));
    BOOST_TEST(p.find("assert_length") < p.find("assert_pattern"));
    const std::string r = plan(R"({"properties": {"a": {"type": "integer"}}, "required": ["a"]})", true);
    BOOST_TEST(r.find("assert_required") == std::string::npos);
}

void the_record_plan_keeps_its_order() {
    const char* s = R"({"type": "object", "required": ["a"], "not": false,
                        "properties": {"a": {"pattern": "^a", "minLength": 2, "type": "string"}},
                        "allOf": [true, {"minProperties": 1}]})";
    BOOST_TEST_EQ(plan(s, true, js::detail::plan_list::record), plan(s, false));   // validate's errors unchanged
    BOOST_TEST_NE(plan(s, true), plan(s, false));
}
```

  The last function is what keeps `validate_test`'s and the goldens' order
  by construction: no optimization of this task reaches the record plan.
  Run it. Expected: FAIL, the flag plan unchanged.
- [ ] **Step 2: Implement** the four passes.
- [ ] **Step 3: The suites are the oracle.**

```bash
lanes libs/json_schema/test
grep -h 'suite \|round trip' bin/lane-clang.log
lint_clean
```

  Expected: as E5.12's Step 3.
- [ ] **Step 4: Commit.**

```bash
cd libs/json_schema
F="include/webcpp/json_schema/detail/compiler/optimize.hpp include/webcpp/json_schema/detail/compiler/opcode.hpp \
include/webcpp/json_schema/detail/vm/machine.hpp test/explain_test.cpp"
git add $F
$G commit -m "json_schema: is_valid's plan folded, fused and ordered cheapest first" -- $F
```

### Task E5.14: Inlined references and the static `unevaluated*` filter

After E5.13.

**Files:**
- Modify: `include/webcpp/json_schema/detail/compiler/optimize.hpp`,
  `detail/compiler/evaluated.hpp`, `detail/vm/machine.hpp`,
  `test/explain_test.cpp`.

**Interfaces** (both plans; nothing recorded changes):
- **Small non-recursive `$ref` targets inlined:** a target of at most 8
  instructions, on no cycle, is copied in place; its instructions keep
  their keyword locations, and the evaluation path of each keeps `$ref`.
- **The static `unevaluated*` filter** (spec 7.4): when every contributor
  is static (names, patterns, all) and reached only through `allOf` and
  `$ref`, the loop gets a compile-time filter and emits no mark.

- [ ] **Step 1: Write the failing tests.** Append to `explain_test.cpp`:

```cpp
void inlining_and_the_static_filter() {
    const char* ref = R"({"$defs": {"i": {"type": "integer"}}, "properties": {"a": {"$ref": "#/$defs/i"}}})";
    BOOST_TEST(plan(ref, false).find("jump") != std::string::npos);
    BOOST_TEST(plan(ref, true, js::detail::plan_list::record).find("jump") == std::string::npos);
    const char* s = R"({"allOf": [{"properties": {"a": true}}], "unevaluatedProperties": false})";
    BOOST_TEST(plan(s, false).find("mark_evaluated") != std::string::npos);
    BOOST_TEST(plan(s, true, js::detail::plan_list::record).find("mark_evaluated") == std::string::npos);
    const auto compiled = validator.compile(boost::json::parse(ref));
    const auto outcome = compiled->validate(boost::json::parse(R"({"a": "x"})"));
    BOOST_TEST_EQ(std::string(outcome->errors()[0].keyword_location()), "/properties/a/$ref/type");
}
```

  Run it. Expected: FAIL, the `jump` and the marks still there.
- [ ] **Step 2: Implement** both passes.
- [ ] **Step 3: The suites are the oracle,** as E5.12's Step 3, with
  `allocation_test` among them.
- [ ] **Step 4: Commit.**

```bash
cd libs/json_schema
F="include/webcpp/json_schema/detail/compiler/optimize.hpp include/webcpp/json_schema/detail/compiler/evaluated.hpp \
include/webcpp/json_schema/detail/vm/machine.hpp test/explain_test.cpp"
git add $F
$G commit -m "json_schema: small references inlined, and unevaluated keywords filtered at compile time" -- $F
```

### Task E5.15: The benchmark's programs

After E5.14. Written for decisions 2 and 3's recommended outcome: the
community benchmark runs from a pinned commit cloned into `.local/`, never
copied into the repository, and only our `results.json` is published.
Under the alternative (no community benchmark), E5.16 is dropped, and
E5.17's page shows `micro`'s two numbers alone.

**Files:**
- Create: `benchmark/Jamfile`, `benchmark/implementation/runner.cpp`,
  `benchmark/implementation/micro.cpp`.

**Interfaces:**
- `runner <schema.json> <instances.jsonl>` follows jsonschema-benchmark's
  implementation contract: it prints `cold,warm,compile,parse` in
  nanoseconds.
- `micro` compiles the bundled 2020-12 meta-schema 100 times and validates
  it against itself 1,000 times, and prints the two medians as
  `compile,warm` in nanoseconds.
- Both time with `std::chrono::steady_clock`: a benchmark is the world,
  which no library header touches. Both are explicit targets, never part
  of `test`, built for one toolset per b2 request.

- [ ] **Step 1: Write the programs.** `benchmark/implementation/runner.cpp`:

```cpp
#include <webcpp/json_schema.hpp>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

namespace js = webcpp::json_schema;
using clock_type = std::chrono::steady_clock;

namespace {

long long since(clock_type::time_point start) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(clock_type::now() - start).count();
}

std::string read(const char* path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        std::fprintf(stderr, "usage: runner <schema.json> <instances.jsonl>\n");
        return 2;
    }
    const std::string schema_text = read(argv[1]);
    std::vector<js::parsed_json> instances;
    std::ifstream lines(argv[2]);
    for (std::string line; std::getline(lines, line);) {
        auto parsed = js::parse(line);
        if (!parsed) {
            std::fprintf(stderr, "an instance does not parse\n");
            return 1;
        }
        instances.push_back(std::move(*parsed));
    }
    auto start = clock_type::now();
    const auto parsed = js::parse(schema_text);
    const long long parse = since(start);
    if (!parsed) {
        std::fprintf(stderr, "the schema does not parse\n");
        return 1;
    }
    const js::validator validator;
    start = clock_type::now();
    const auto schema = validator.compile(*parsed);
    const long long compile = since(start);
    if (!schema) {
        std::fprintf(stderr, "refused: %s\n", std::string(schema.error().where).c_str());
        return 1;
    }
    auto pass = [&] {
        const auto t = clock_type::now();
        for (const auto& i : instances) {
            (void)schema->is_valid(i.value);
        }
        return since(t);
    };
    const long long cold = pass();
    std::vector<long long> warm;
    for (int i = 0; i < 11; ++i) {
        warm.push_back(pass());
    }
    std::nth_element(warm.begin(), warm.begin() + 5, warm.end());
    std::printf("%lld,%lld,%lld,%lld\n", cold, warm[5], compile, parse);
}
```

`benchmark/implementation/micro.cpp`:

```cpp
#include <webcpp/json_schema.hpp>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

namespace js = webcpp::json_schema;
using clock_type = std::chrono::steady_clock;

namespace {

long long median(std::vector<long long> v) {
    std::nth_element(v.begin(), v.begin() + static_cast<std::ptrdiff_t>(v.size() / 2), v.end());
    return v[v.size() / 2];
}

std::string meta_schema_text() {
    std::string text;
    for (const std::string_view chunk :
         js::detail::find_meta_schema("https://json-schema.org/draft/2020-12/schema")->chunks) {
        text += chunk;
    }
    return text;
}

}  // namespace

int main() {
    const auto parsed = js::parse(meta_schema_text());
    const js::validator validator;
    std::vector<long long> compile;
    for (int i = 0; i < 100; ++i) {
        const auto t = clock_type::now();
        const auto schema = validator.compile(*parsed);
        compile.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(clock_type::now() - t).count());
    }
    const auto schema = validator.compile(*parsed);
    std::vector<long long> warm;
    for (int i = 0; i < 1000; ++i) {
        const auto t = clock_type::now();
        (void)schema->is_valid(parsed->value);
        warm.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(clock_type::now() - t).count());
    }
    std::printf("%lld,%lld\n", median(compile), median(warm));
}
```

`benchmark/Jamfile`:

```
project : requirements <library>/webcpp/json_schema//json_schema <variant>release ;

import webcpp ;

# The benchmark's programs: explicit, never part of test, built once per
# toolset by benchmark/run.py (b2 builds one toolset per request).
webcpp.link runner : implementation/runner.cpp ;
webcpp.link micro : implementation/micro.cpp ;
explicit runner micro ;
```

- [ ] **Step 2: Build and run them once,** natively and for wasip2:

```bash
$B2 --build-dir=bin/bench libs/json_schema/benchmark//runner libs/json_schema/benchmark//micro
$B2 --build-dir=bin/bench toolset=clang-wasip2 libs/json_schema/benchmark//micro
micro=$(find bin/bench -path '*benchmark*' -name 'micro*' -type f -perm -u+x | grep -v wasip | head -1)
"$micro"
wasmtime run "$(find bin/bench -path '*wasip2*' -name 'micro*.wasm' | head -1)"
lint_clean
```

  Expected: two lines `<compile>,<warm>` of positive numbers, and
  `lint: clean`. The numbers are not recorded: E5.16 runs on the owner's
  machine.
- [ ] **Step 3: Commit.**

```bash
cd libs/json_schema
F="benchmark/Jamfile benchmark/implementation/runner.cpp benchmark/implementation/micro.cpp"
git add $F
$G commit -m "json_schema: the benchmark's runner and micro-benchmark" -- $F
```

### Task E5.16: The benchmark's driver

After E5.15. Waits for the owner's decisions 2 and 3 (the benchmark's
commit, Blaze's commit and ajv's version).

**Files:** Create `benchmark/run.py`, `benchmark/README.md`.

**Interfaces:** `python3 benchmark/run.py <benchmark commit> <superproject
root>`:
- builds `runner` and `micro` natively (`toolset=clang`) and for wasip2
  (`toolset=clang-wasip2`), two b2 requests;
- clones `sourcemeta-research/jsonschema-benchmark` at the pinned commit
  into `.local/jsonschema-benchmark`, never into the tree, and runs its own
  `make` for Blaze and ajv in Docker;
- runs `runner` natively and under `wasmtime run` over the same schemas,
  five times each, and keeps the medians;
- writes `benchmark/results.json`: per schema and implementation the four
  medians; `micro`'s two; the machine (CPU, cores, memory, OS); the date;
  every version spec 17 lists: the compiler, Boost, wasmtime, Node, the
  benchmark's commit, Blaze's, ajv's, json_schema's commit.

- [ ] **Step 1: Write it.** `benchmark/run.py`:

```python
#!/usr/bin/env python3
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)
"""Runs jsonschema-benchmark for Blaze and ajv, and json_schema natively and under wasmtime.

Writes results.json beside this file. Run on the owner's machine only: the numbers are that
machine's, and the file records it. With --dry-run, prints the steps and runs none.
"""

import csv
import datetime
import json
import os
import pathlib
import platform
import statistics
import subprocess
import sys

HERE = pathlib.Path(__file__).resolve().parent
REPOSITORY = "https://github.com/sourcemeta-research/jsonschema-benchmark"
B2 = ["env", "-u", "CPATH", "-u", "CPLUS_INCLUDE_PATH", "-u", "C_INCLUDE_PATH", "b2",
      "--user-config=.local/user-config.jam"]
DRY = "--dry-run" in sys.argv


def run(command: list, cwd: pathlib.Path = HERE) -> str:
    print("$", " ".join(command), file=sys.stderr)
    if DRY:
        return ""
    return subprocess.run(command, check=True, capture_output=True, text=True, cwd=cwd).stdout


def built(root: pathlib.Path, toolset: str, name: str) -> list:
    build_dir = root / "bin" / f"bench-{toolset}"
    run(B2 + [f"--build-dir={build_dir}", f"toolset={toolset}",
              f"libs/json_schema/benchmark//{name}"], cwd=root)
    if DRY:
        return [name]
    found = sorted(p for p in build_dir.rglob(f"{name}*") if p.is_file() and os.access(p, os.X_OK)
                   or p.suffix == ".wasm")
    program = str(found[0])
    return ["wasmtime", "run", "--dir=/::/", program] if program.endswith(".wasm") else [program]


def medians(program: list, arguments: list, names: list, runs: int = 5) -> dict:
    samples = [run(program + arguments).strip() for _ in range(runs)]
    if DRY:
        return {}
    columns = zip(*[[int(x) for x in s.split(",")] for s in samples])
    return dict(zip(names, [statistics.median(c) for c in columns]))


def theirs(clone: pathlib.Path) -> tuple:
    run(["make", "IMPLEMENTATIONS=blaze ajv", "RUNS=5", "dist/report.csv"], cwd=clone)
    results, versions = {}, {}
    if DRY:
        return results, versions
    with open(clone / "dist" / "report.csv", encoding="utf-8") as report:
        rows = csv.DictReader(report)
        needed = {"implementation", "version", "name", "cold_ns", "warm_ns", "compile_ns"}
        if not needed <= set(rows.fieldnames or []):
            sys.exit(f"run.py: report.csv has {rows.fieldnames}; adjust to the pin's header")
        for row in rows:
            versions[row["implementation"]] = row["version"]
            results.setdefault(row["name"], {})[row["implementation"]] = {
                "cold": int(row["cold_ns"]), "warm": int(row["warm_ns"]),
                "compile": int(row["compile_ns"])}
    return results, versions


def machine() -> dict:
    if sys.platform == "darwin":
        cpu = run(["sysctl", "-n", "machdep.cpu.brand_string"])
        memory = run(["sysctl", "-n", "hw.memsize"])
    else:
        cpu = run(["sh", "-c", "grep -m1 'model name' /proc/cpuinfo | cut -d: -f2"])
        memory = run(["sh", "-c", "grep MemTotal /proc/meminfo"])
    return {"cpu": cpu.strip(), "cores": os.cpu_count(), "memory": memory.strip(),
            "os": platform.platform()}


def main() -> None:
    commit, root = sys.argv[1], pathlib.Path(sys.argv[2]).resolve()
    clone = root / ".local" / "jsonschema-benchmark"
    if not clone.exists():
        run(["git", "clone", REPOSITORY, str(clone)], cwd=root)
    run(["git", "-C", str(clone), "checkout", "--detach", commit], cwd=root)
    results, versions = theirs(clone)
    native, wasm = built(root, "clang", "runner"), built(root, "clang-wasip2", "runner")
    names = ["cold", "warm", "compile", "parse"]
    schemas = [] if DRY else sorted((clone / "schemas").iterdir())
    for schema in schemas:
        arguments = [str(schema / "schema.json"), str(schema / "instances.jsonl")]
        results.setdefault(schema.name, {})["webcpp"] = medians(native, arguments, names)
        results[schema.name]["webcpp-wasip2"] = medians(wasm, arguments, names)
    micro = {"native": medians(built(root, "clang", "micro"), [], ["compile", "warm"]),
             "wasip2": medians(built(root, "clang-wasip2", "micro"), [], ["compile", "warm"])}
    boost = run(["sh", "-c", "grep 'define BOOST_LIB_VERSION' "
                             ".local/boost-source/boost/version.hpp"], cwd=root)
    versions.update({
        "benchmark": commit,
        "json_schema": run(["git", "-C", str(HERE.parent), "rev-parse", "HEAD"]).strip(),
        "compiler": run(["sh", "-c", "clang++ --version | head -1"]).strip(),
        "boost": boost.split('"')[1] if '"' in boost else "",
        "wasmtime": run(["wasmtime", "--version"]).strip(),
        "node": run(["node", "--version"]).strip()})
    if DRY:
        return
    document = {"date": datetime.date.today().isoformat(), "machine": machine(),
                "versions": versions, "micro": micro, "results": results}
    (HERE / "results.json").write_text(json.dumps(document, indent=4, sort_keys=True) + "\n",
                                       encoding="utf-8")


if __name__ == "__main__":
    main()
```

  The CSV's column names are those `report.csv` holds at the pinned
  commit; `run.py` refuses another header by name, and the first run
  adjusts `needed` to the pin's header if it differs, in the same change
  as `results.json`.

  `benchmark/README.md` says what runs, where it clones, that nothing of
  the community benchmark is copied into the repository (decision 2), how
  to run it, and how the page is produced from `results.json`.
- [ ] **Step 2: Check it without running it.**

```bash
python3 libs/json_schema/benchmark/run.py 0000000 "$PWD" --dry-run 2>&1 | head -12
lint_clean
```

  Expected: the `$ git clone`, `$ make`, `$ env -u CPATH ... b2` lines in
  order, and `lint: clean` (Pyright and the 100-column rule read
  `run.py`).
- [ ] **Step 3: Commit.**

```bash
cd libs/json_schema
F="benchmark/run.py benchmark/README.md"
git add $F
$G commit -m "json_schema: the benchmark's driver, cloning the community benchmark at a pinned commit" -- $F
```

### Task E5.17: The benchmark's page, generated

Steps 1 and 2 after E5.15; Steps 3 and 4 after E5.16 and the owner's run.

**Files:**
- Create: `benchmark/report.py`, `benchmark/report_test.py`,
  `doc/benchmark.adoc` (generated, with the owner's `results.json`),
  `benchmark/results.json` (the owner's run).
- Modify: `benchmark/Jamfile`, `test/Jamfile`, `doc/json_schema.adoc`
  (the performance section, with the owner's files).

**Interfaces:**
- `report.py <results.json> <benchmark.adoc>` writes the page's section:
  the BSL notice (an `.adoc` file), the method and the machine, the table
  (per schema, each implementation's warm and cold medians), and the chart
  as an inline SVG inside a passthrough block. The page needs no image
  handling, which the doc toolchain lacks (the E2 to E5 review's I22).
- `test/Jamfile` declares it with S1, so the check compares the whole
  output, notice included:

```
webcpp.generated benchmark-report : ../benchmark/report.py : ../benchmark/results.json
  : ../doc/benchmark.adoc ;
webcpp.lane benchmark-report : generated-benchmark-report ;
```

- [ ] **Step 1: The generator and its test.** `benchmark/report.py`:

```python
#!/usr/bin/env python3
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)
"""Writes the benchmark's section of the page from results.json: run as report.py <in> <out>."""

import json
import math
import sys

NOTICE = """// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//
// Generated by benchmark/report.py from benchmark/results.json; never edited by hand.
"""


def chart(results: dict) -> str:
    """Per schema, each implementation's warm time over the fastest's, on a log axis."""
    names = sorted(results)
    implementations = sorted({i for r in results.values() for i in r})
    row, left = 18, 220
    height = row * len(names) * len(implementations) + 40
    out = [f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 640 {height}" '
           'font-family="system-ui" font-size="11" role="img">']
    y = 20
    for name in names:
        fastest = min(r["warm"] for r in results[name].values())
        for i, implementation in enumerate(implementations):
            if implementation not in results[name]:
                continue
            ratio = results[name][implementation]["warm"] / fastest
            bar = 4 + 120 * math.log10(ratio) if ratio > 1 else 4
            label = name if i == 0 else ""
            out.append(f'<text x="0" y="{y + 12}">{label}</text>')
            out.append(f'<rect x="{left}" y="{y}" width="{bar:.1f}" height="{row - 4}" '
                       f'class="i{i}"><title>{implementation}: {ratio:.2f}x</title></rect>')
            y += row
    out.append("</svg>")
    return "\n".join(out)


def table(results: dict) -> str:
    implementations = sorted({i for r in results.values() for i in r})
    head = "| Schema | " + " | ".join(f"{i} warm | {i} cold" for i in implementations)
    lines = ['[cols="1' + ",>1" * (2 * len(implementations)) + '",options="header"]', "|===",
             head]
    for name in sorted(results):
        cells = []
        for i in implementations:
            r = results[name].get(i)
            cells += [f"{r['warm'] / 1e6:.2f} ms", f"{r['cold'] / 1e6:.2f} ms"] if r else ["", ""]
        lines.append(f"| {name} | " + " | ".join(cells))
    lines.append("|===")
    return "\n".join(lines)


def page(document: dict) -> str:
    m, v = document["machine"], document["versions"]
    method = (f"Measured on {document['date']}: {m['cpu']}, {m['cores']} cores, {m['os']}. "
              f"jsonschema-benchmark at {v['benchmark']}; json_schema at {v['json_schema']}, "
              f"{v['compiler']}, Boost {v['boost']}, wasmtime {v['wasmtime']}, Node {v['node']}. "
              "Each figure is the median of five runs.")
    return (NOTICE + "\n" + method + "\n\n" + table(document["results"]) + "\n\n++++\n"
            + chart(document["results"]) + "\n++++\n")


def main() -> None:
    with open(sys.argv[1], encoding="utf-8") as source:
        document = json.load(source)
    with open(sys.argv[2], "w", encoding="utf-8", newline="\n") as out:
        out.write(page(document))


if __name__ == "__main__":
    main()
```

`benchmark/report_test.py`:

```python
#!/usr/bin/env python3
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)
"""Tests report.py on a document of two schemas and two implementations."""

import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import report  # noqa: E402

DOCUMENT = {
    "date": "2026-10-09",
    "machine": {"cpu": "cpu", "cores": 8, "memory": "16 GB", "os": "os"},
    "versions": {"benchmark": "b", "json_schema": "j", "compiler": "c", "boost": "1_92",
                 "wasmtime": "w", "node": "n"},
    "results": {"a": {"x": {"warm": 2000000, "cold": 3000000},
                      "y": {"warm": 1000000, "cold": 1000000}},
                "b": {"x": {"warm": 1000000, "cold": 1000000}}},
}


class ReportTest(unittest.TestCase):
    def test_the_page_opens_with_the_notice(self) -> None:
        self.assertTrue(report.page(DOCUMENT).startswith("// Copyright (c) 2026 WebCpp.org\n"))

    def test_the_table_holds_each_figure_in_milliseconds(self) -> None:
        table = report.table(DOCUMENT["results"])
        self.assertIn("| a | 2.00 ms | 3.00 ms | 1.00 ms | 1.00 ms", table)
        self.assertIn("| b | 1.00 ms | 1.00 ms |  | ", table)

    def test_the_chart_marks_the_ratio(self) -> None:
        svg = report.chart(DOCUMENT["results"])
        self.assertIn("<title>x: 2.00x</title>", svg)
        self.assertIn("<title>y: 1.00x</title>", svg)


if __name__ == "__main__":
    unittest.main()
```

  `benchmark/Jamfile` gains the test as an explicit target:

```
import notfile ;
path-constant HERE : . ;
notfile report-test : @report-test ;
actions report-test
{
    python3 "$(HERE)/report_test.py"
}
explicit report-test ;
```

```bash
$B2 -a libs/json_schema/benchmark//report-test
lint_clean
```

  Expected: `OK` from unittest's three tests, `lint: clean`.
- [ ] **Step 2: Commit the generator** (before the owner's run; the page
  does not include the section yet, so the doc build stays green):

```bash
cd libs/json_schema
F="benchmark/report.py benchmark/report_test.py benchmark/Jamfile"
git add $F
$G commit -m "json_schema: the benchmark's page section, generated from its results" -- $F
```

- [ ] **Step 3: The owner's run, then the page.** The owner runs
  `python3 libs/json_schema/benchmark/run.py <commit> "$PWD"` on their
  machine. Then the controller appends the `webcpp.generated
  benchmark-report` lines to `test/Jamfile`, adds
  `include::benchmark.adoc[]` to the page's performance section, and:

```bash
$B2 libs/json_schema/test//update-generated
$B2 -a libs/json_schema/test//generated-benchmark-report
$B2 -a --build-dir=bin/doc libs/json_schema/doc
lint_clean
```

  Expected: the check and the doc build exit 0; the page shows the method,
  the table and the chart, at 320, 768 and 1280 px wide without a
  horizontal scroll of the page (the table scrolls in its own box).
- [ ] **Step 4: Commit the owner's files.**

```bash
cd libs/json_schema
F="benchmark/results.json doc/benchmark.adoc doc/json_schema.adoc test/Jamfile"
git add $F
$G commit -m "json_schema: the benchmark's results, measured against Blaze and ajv" -- $F
```

  Then the controller runs `merge_track 5` again from
  `~/dev/webcpp-json-schema-e5`, which fast-forwards the canonical `main`
  by these commits. Expected: as E5.20's Step 3.

### Task E5.18: Conformance: the reasons closed, the counts, the known failures on the page

After E5.17's Step 2 (the owner's run is not needed).

**Files:**
- Create: `doc/counts.py`, `doc/known_failures.py`,
  `doc/known_failures.adoc` (generated), `doc/conformance.adoc`.
- Modify: `test/suite/runner.hpp` (`reason_for` keeps the permanent table
  alone), `test/Jamfile`, `doc/json_schema.adoc`.

**Interfaces:**
- `reason_for` returns the permanent table's reason, else `""`: every
  `E<N>:` rule leaves, so any failure that is not a recorded deviation is
  not recordable (the E2 to E5 review's I12).
- `doc/counts.py <library>` prints, per dialect and group (`required`,
  `optional`, `format`), `n-<group>-<dialect>=<total>` and
  `n-<group>-<dialect>-passed=<total minus listed>`, and for OpenAPI 3.0
  the filtered draft 4 subset plus `test/openapi`. It prints no zero:
  `tools/doc/counts.py` fails the build on one (AGENTS chapter 8), so a
  group with no test is left out.
- `doc/known_failures.py <list>... <output>` writes
  `doc/known_failures.adoc`: per list a table (file, case, test, reason),
  or "None." It is declared with S1, so the check compares its whole
  output, notice included, and the page includes the generated file from
  `doc/` (doc-check allows it; spec 18.12 needs no doc-tools extension).

- [ ] **Step 1: Close the reasons.** `reason_for`'s body becomes:

```cpp
inline std::string reason_for(std::string_view dialect, const test_id& id, const boost::json::value&) {
    const auto& permanent = permanent_reasons();
    if (const auto it = permanent.find({dialect, id.file, id.case_description, id.test_description});
        it != permanent.end()) {
        return std::string(it->second);
    }
    return "";
}
```

```bash
record_all
grep -h '"reason"' libs/json_schema/test/known_failures/*.json | grep -c '"E[0-9]:' || true
```

  Expected: `record_all` gains nothing and records nothing new, and the
  count is `0`. A `not recordable:` line names a failure a track left
  behind: it is fixed now, in the library.
- [ ] **Step 2: The counts.** `doc/counts.py`:

```python
#!/usr/bin/env python3
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)
"""Prints json_schema's conformance counts, from the pinned suite and the known-failure lists.

Run by the doc build with the library's directory; prints <name>=<number> lines and no zero.
"""

import json
import pathlib
import sys

DIALECTS = ["draft2020-12", "draft2019-09", "draft7", "draft6", "draft4"]
SCHEMA_OBJECT = {"title", "multipleOf", "maximum", "exclusiveMaximum", "minimum",
                 "exclusiveMinimum", "maxLength", "minLength", "pattern", "maxItems", "minItems",
                 "uniqueItems", "maxProperties", "minProperties", "required", "enum", "type",
                 "allOf", "oneOf", "anyOf", "not", "items", "properties", "additionalProperties",
                 "description", "format", "default", "$ref"}


def group(relative: str) -> str:
    if relative.startswith("optional/format/") or relative == "optional/format-assertion.json":
        return "format"
    return "optional" if relative.startswith("optional/") else "required"


def member_ok(key: str, value) -> bool:
    if key not in SCHEMA_OBJECT:
        return False
    if key == "type":
        return isinstance(value, str) and value != "null"
    if key in ("exclusiveMaximum", "exclusiveMinimum"):
        return isinstance(value, bool)
    return key != "items" or isinstance(value, dict)


def schema_object_only(schema) -> bool:
    """The runner's filter of the draft 4 cases OpenAPI 3.0 runs, in runner.hpp."""
    work = [schema]
    while work:
        node = work.pop()
        if not isinstance(node, dict):
            return False
        for key, value in node.items():
            if not member_ok(key, value):
                return False
            if key == "properties":
                work.extend(value.values())
            elif key in ("allOf", "anyOf", "oneOf"):
                work.extend(value)
            elif key in ("items", "not") or (key == "additionalProperties"
                                              and isinstance(value, dict)):
                work.append(value)
    return True


def listed(library: pathlib.Path, name: str) -> list:
    return json.loads((library / "test" / "known_failures" / f"{name}.json").read_text("utf-8"))


def emit(name: str, total: int, failed: int) -> None:
    if total > 0:
        print(f"n-{name}={total}")
        if total - failed > 0:
            print(f"n-{name}-passed={total - failed}")


def main() -> None:
    library = pathlib.Path(sys.argv[1]).resolve()
    suite = library.parent.parent / "third_party" / "libs" / "JSON-Schema-Test-Suite" / "tests"
    for dialect in DIALECTS:
        totals = {"required": 0, "optional": 0, "format": 0}
        base = suite / dialect
        for path in sorted(base.rglob("*.json")):
            cases = json.loads(path.read_text("utf-8"))
            totals[group(path.relative_to(base).as_posix())] += sum(len(c["tests"]) for c in cases)
        failed = {"required": 0, "optional": 0, "format": 0}
        for entry in listed(library, dialect):
            failed[group(entry["file"])] += 1
        for name, total in totals.items():
            emit(f"{name}-{dialect}", total, failed[name])
    openapi = 0
    for path in sorted((suite / "draft4").glob("*.json")):
        cases = json.loads(path.read_text("utf-8"))
        openapi += sum(len(c["tests"]) for c in cases if schema_object_only(c["schema"]))
    for path in sorted((library / "test" / "openapi").glob("*.json")):
        openapi += sum(len(c["tests"]) or 1 for c in json.loads(path.read_text("utf-8")))
    emit("openapi3-0", openapi, len(listed(library, "openapi3-0")))


if __name__ == "__main__":
    main()
```

  A refusal case counts as one test, as the runner counts it.
- [ ] **Step 3: The known failures, generated.** `doc/known_failures.py`:

```python
#!/usr/bin/env python3
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)
"""Writes the page's list of known failures: known_failures.py <list.json>... <output.adoc>."""

import json
import pathlib
import sys

NOTICE = """// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//
// Generated by doc/known_failures.py from test/known_failures/; never edited by hand.
"""


def cell(text: str) -> str:
    return text.replace("|", "\\|")


def section(path: pathlib.Path) -> str:
    entries = json.loads(path.read_text("utf-8"))
    out = [f"[discrete]\n==== {path.stem}\n"]
    if not entries:
        out.append("None.\n")
        return "\n".join(out)
    out.append('[cols="2,2,2,3",options="header"]\n|===\n| File | Case | Test | Reason')
    for e in entries:
        out.append(f"| {cell(e['file'])} | {cell(e['case'])} | {cell(e['test'])} "
                   f"| {cell(e['reason'])}")
    out.append("|===\n")
    return "\n".join(out)


def main() -> None:
    *lists, output = [pathlib.Path(a) for a in sys.argv[1:]]
    text = NOTICE + "\n" + "\n".join(section(p) for p in lists)
    pathlib.Path(output).write_text(text, encoding="utf-8", newline="\n")


if __name__ == "__main__":
    main()
```

  Append to `test/Jamfile`, the lists in the order the page shows them:

```
webcpp.generated known-failures : ../doc/known_failures.py
  : known_failures/draft2020-12.json known_failures/draft2019-09.json
    known_failures/draft7.json known_failures/draft6.json
    known_failures/draft4.json known_failures/openapi3-0.json
  : ../doc/known_failures.adoc ;
webcpp.lane known-failures : generated-known-failures ;
```

  `doc/conformance.adoc` holds, per dialect, a table of required,
  optional and format tests, each `{n-<group>-<dialect>-passed}` of
  `{n-<group>-<dialect>}`; the decisions that are deviations, each with
  its reason (PD2's inert unknown keywords in drafts 4 to 7, PD7's draft 4
  integers, PD9's local Bowtie lane); and `include::known_failures.adoc[]`.
  `json_schema.adoc` includes it after the emitting section.

```bash
$B2 libs/json_schema/test//update-generated
$B2 -a libs/json_schema/test//generated-known-failures
$B2 -a --build-dir=bin/doc libs/json_schema/doc
python3 libs/json_schema/doc/counts.py libs/json_schema | head -6
lint_clean
```

  Expected: the check and the doc build exit 0; the first counts read
  `n-required-draft2020-12=1301`, `n-required-draft2020-12-passed=1301`;
  `lint: clean`.
- [ ] **Step 4: Commit.**

```bash
cd libs/json_schema
F="test/suite/runner.hpp test/Jamfile doc/counts.py doc/known_failures.py doc/known_failures.adoc \
doc/conformance.adoc doc/json_schema.adoc"
git add $F
$G commit -m "json_schema: conformance counted from the suite, the known failures generated onto the page" -- $F
```

### Task E5.19: The remaining sections and their examples

After E5.18.

**Files:**
- Create:
  - `doc/dialects.adoc`, `doc/formats.adoc`, `doc/vocabularies.adoc`;
  - `example/json_schema_dialects.cpp`, `example/json_schema_formats.cpp`,
    `example/json_schema_vocabulary.cpp`, each with its `.expected`.
- Modify:
  - `doc/json_schema.adoc` (spec 18's sixteen sections, in order);
  - `doc/testing.adoc` (Bowtie with the `harness` tag, fuzzing, the
    lanes), `doc/limits.adoc` (formats, content and patterns allocate in
    the validation's allocator), `doc/guarantees.adoc`;
  - `example/Jamfile`.

- [ ] **Step 1: The examples.** `example/json_schema_dialects.cpp`:

```cpp
#include <webcpp/json_schema.hpp>

#include <boost/json/parse.hpp>

#include <iostream>
#include <string>

namespace js = webcpp::json_schema;

int main() {
    // tag::dialects[]
    const js::validator all;   // every dialect; without $schema, 2020-12
    const auto four = all.compile(boost::json::parse(R"({
        "$schema": "http://json-schema.org/draft-04/schema#",
        "type": "integer"
    })"));
    const auto seven = all.compile(boost::json::parse(R"({
        "$schema": "http://json-schema.org/draft-07/schema#",
        "type": "integer"
    })"));
    const auto one_point_zero = boost::json::parse("1.0");
    std::cout << std::boolalpha << four->is_valid(one_point_zero) << ' ' << seven->is_valid(one_point_zero) << '\n';
    const js::validator_for<js::draft2020_12> only;
    const auto refused = only.compile(boost::json::parse(R"({"$schema": "http://json-schema.org/draft-07/schema#"})"));
    std::cout << refused.error().code.message() << ": " << std::string(refused.error().what) << '\n';
    // end::dialects[]
}
```

  `example/json_schema_dialects.expected`:

```
false true
the dialect is outside the validator's set: http://json-schema.org/draft-07/schema#
```

`example/json_schema_formats.cpp`:

```cpp
#include <webcpp/json_schema.hpp>

#include <boost/json/parse.hpp>

#include <iostream>

namespace js = webcpp::json_schema;

int main() {
    // tag::formats[]
    const js::validator validator;
    const auto date = boost::json::parse(R"({"format": "date"})");
    const auto february_30 = boost::json::parse(R"("2026-02-30")");
    std::cout << std::boolalpha << validator.compile(date)->is_valid(february_30) << ' '
              << validator.compile(date, {.format_assertion = true})->is_valid(february_30) << '\n';
    const auto content = validator.compile(boost::json::parse(R"({
        "contentMediaType": "application/json",
        "contentSchema": {"required": ["a"]}
    })"), {.content_assertion = true});
    std::cout << content->is_valid(boost::json::parse(R"("{\"a\": 1}")")) << ' '
              << content->is_valid(boost::json::parse(R"("{}")")) << '\n';
    // end::formats[]
}
```

  `example/json_schema_formats.expected`:

```
true false
true false
```

`example/json_schema_vocabulary.cpp`:

```cpp
#include <webcpp/json_schema.hpp>

#include <boost/json/parse.hpp>

#include <array>
#include <iostream>
#include <optional>

namespace js = webcpp::json_schema;

namespace {

boost::system::result<void> a_boolean(const boost::json::value& v) {
    if (v.is_bool()) {
        return {};
    }
    return boost::system::errc::make_error_code(boost::system::errc::invalid_argument);
}

boost::system::result<bool> even(const js::user_call& call) {
    return !call.value.as_bool() || call.instance.as_int64() % 2 == 0;
}

}  // namespace

int main() {
    // tag::vocabulary[]
    const std::array<js::basic_user_keyword<js::default_traits>, 1> keywords{{
        {.name = "x-even", .check = a_boolean, .validate = even,
         .applies_to = js::ir::type_set{js::ir::type_set::integer}, .annotates = false, .write = std::nullopt},
    }};
    const std::array<js::basic_vocabulary<js::default_traits>, 1> vocabularies{{
        {.uri = "urn:example:even", .keywords = keywords},
    }};
    const js::validator validator;
    const auto schema = validator.compile(boost::json::parse(R"({"x-even": true})"), {.vocabularies = vocabularies});
    std::cout << std::boolalpha << schema->is_valid(boost::json::parse("4")) << ' '
              << schema->is_valid(boost::json::parse("5")) << '\n';
    const auto five = boost::json::parse("5");
    const auto outcome = schema->validate(five);
    std::cout << outcome->message(outcome->errors()[0]) << '\n';
    // end::vocabulary[]
}
```

  `example/json_schema_vocabulary.expected`:

```
true false
Expected the value to satisfy x-even.
```

  Append the three `webcpp.example` lines to `example/Jamfile`.
- [ ] **Step 2: The sections.**
  - `dialects.adoc` (spec 18.2): the six dialects, `$schema` and its
    accepted spellings (Review Focus 1), choosing a set with
    `validator_for`, `default_dialect`, with the `dialects` tag.
  - `formats.adoc` (spec 18.6): annotation or assertion, the 19 formats of
    `default_formats()`, a set of one's own, leap seconds and v1 (PD8),
    content, with the `formats` tag.
  - `vocabularies.adoc` (spec 18.9): the user keyword's parts, activation,
    conflicts, the emitter's `write`, with the `vocabulary` tag.
  - `testing.adoc` gains Bowtie, with `bowtie_webcpp`'s `harness` tag, and
    the fuzz lane; `limits.adoc` gains the allocation of formats, content
    and patterns.
  - `guarantees.adoc` holds spec 18.15's list, and E1's seventh, each with
    the test that pins it:
    1. a compiled schema is immutable and may be shared between threads
       (`threads_test`, and under ThreadSanitizer in the fuzz lane);
    2. `is_valid` and `validate` agree (every runner, and the fuzzer);
    3. `is_valid` allocates nothing on the common path
       (`allocation_test`);
    4. no function recurses on its input's depth (`hostile_test`);
    5. the library touches no network, disk or clock (the lint's include
       boundaries);
    6. a runaway pattern is an error, never a hang or an abort
       (`hostile_test`);
    7. `validate` runs the plan at most twice (`validate_test`).
  - `json_schema.adoc` includes the sections in spec 18's order:
    introduction, dialects, compiling, validating, output and messages,
    formats and content, numbers, references, vocabularies, emitting,
    allocation and limits and errors, conformance, performance (once the
    owner's run is in), testing, guarantees, reference.

```bash
$B2 -a --build-dir=bin/doc libs/json_schema/doc
lanes libs/json_schema/example
lint_clean
```

  Expected: the doc build exits 0 (doc-check finds every example shown);
  `lanes: green`; `lint: clean`. Look at the page at 320, 768 and 1280 px:
  no horizontal scroll of the page, every table in its own box, every
  listing readable.
- [ ] **Step 3: Commit.**

```bash
cd libs/json_schema
F="doc/json_schema.adoc doc/dialects.adoc doc/formats.adoc doc/vocabularies.adoc doc/testing.adoc doc/limits.adoc \
doc/guarantees.adoc example/Jamfile example/json_schema_dialects.cpp example/json_schema_dialects.expected \
example/json_schema_formats.cpp example/json_schema_formats.expected example/json_schema_vocabulary.cpp \
example/json_schema_vocabulary.expected"
git add $F
$G commit -m "json_schema: the whole page, every section with its compiled example" -- $F
```

### Task E5.20: E5 complete: readiness, the superproject's files, the merge

After E5.19 (and E5.17's Step 4 once the owner's run is in; the merge
does not wait for it, and the performance section joins with a later
fast-forward).

**Files:**
- Modify: `README.md`, `AGENTS.md` (the library's: the suite and its pin,
  the known-failures rule, the freeze, SRELL, the benchmark's machine;
  spec 18's last item).
- Modify, in the superproject branch `lib/json_schema-e5`: `README.md` and
  `AGENTS.md` (json_schema in chapter 1's table and layout, in the form
  pratt's row has).

- [ ] **Step 1: Every lane and every check, from scratch.**

```bash
cd ~/dev/webcpp-json-schema-e5
lanes libs/json_schema/test libs/json_schema/example
$B2 -a --build-dir=bin/doc libs/json_schema/doc
$B2 -a libs/json_schema/test//generated-meta-schemas libs/json_schema/test//generated-known-failures
$B2 -a libs/json_schema/benchmark//report-test
lint_clean
for t in $(find tools -name '*_test.py' -not -path '*/node_modules/*' | sort); do
  python3 "$t" > /dev/null 2>&1 || echo "FAILED $t"
done
python3 tools/ci/matrix.py plan --library json_schema
python3 tools/ci/matrix.py own-lanes --library json_schema
grep -h 'suite \|round trip' bin/lane-clang.log
```

  Expected:
  - `lanes: green`, every runner `0 unexpected, 0 stale`, every round-trip
    line `0 changed`;
  - the doc build, the two generated checks and the report test exit 0;
  - `lint: clean`; no `FAILED` line;
  - the plan names json_schema on native, wasip2, wasip3 and emscripten;
  - the own lanes are `meta-schemas`, `known-failures`, `fuzz`, `bowtie`
    and, once the owner's run is in, `benchmark-report`.

  The fuzz lane runs on Linux (E5.7's container), and the bowtie lane
  where Docker runs and Bowtie is pinned.
- [ ] **Step 2: The two READMEs and AGENTS.md files,** then commit both
  repositories' changes, by path:

```bash
cd libs/json_schema
git add README.md AGENTS.md
$G commit -m "json_schema: README and AGENTS.md for the whole library" -- README.md AGENTS.md
cd ../..
git add README.md AGENTS.md
$G commit -m "README and AGENTS.md: json_schema, a library of webcpp's own" -- README.md AGENTS.md
```
- [ ] **Step 3: The merge** (controller).

```bash
cd ~/dev/webcpp-json-schema-e5
merge_track 5
```

  Expected: `lanes: green`, `lint: clean`, a fast-forward of the canonical
  `main`, and no frozen file changed since `contract-1` unless the ledger
  approved it.

### Task P: Publication (controller, with the owner's go)

After E5.20, and after the controller has integrated the infra track into
the superproject's `main` (`SUPERPROJECT_BASE=main` in the ledger).

- [ ] **Step 1: The whole review,** on the most capable model:
  - `~/dev/webcpp-repos/json_schema` in full, against the spec;
  - the superproject branch `lib/json_schema-e5` against `main`;
  - a clean build of every lane and own lane;
  - the docs, the lint, every tools test, the CI plan.
- [ ] **Step 2: Ask the owner** for the go to create
  `webcpporg/json_schema` (AGENTS chapter 2, items 1 and 2), and,
  separately, whether to open Bowtie's pull request (spec 16.8). Nothing
  below happens without the first answer.
- [ ] **Step 3: Publish.**

```bash
test ! -e ~/dev/webcpp/libs/json_schema && echo absent
ids_ok "$CANONICAL" main
gh repo create webcpporg/json_schema --public --description "A header-only JSON Schema validator for C++20"
git -C "$CANONICAL" remote add origin git@github.com:webcpporg/json_schema.git   # its first remote
git -C "$CANONICAL" push -u origin main
cd ~/dev/webcpp
git checkout lib/json_schema-e5
$G rebase main
git submodule add ../json_schema.git libs/json_schema
git -C libs/json_schema log --oneline -1
git add .gitmodules libs/json_schema
$G commit -m "json_schema: a library of webcpp's own, as a submodule" -- .gitmodules libs/json_schema
ids_ok . main..HEAD
```

  Expected:
  - `absent` first: nothing but a registered submodule ever lived there;
  - `ids_ok` silent for the canonical `main` and for the superproject's
    range;
  - `git submodule add` clones the new remote into `libs/json_schema`, and
    its head is the canonical `main`'s.

  Then run the lanes and the lint from scratch in `~/dev/webcpp`,
  fast-forward `main` to `lib/json_schema-e5`, push, and watch both CIs
  until green, re-running json_schema's first run after the superproject's
  push.
- [ ] **Step 4: Close.** Remove the worktrees
  `~/dev/webcpp-json-schema-e<N>` with `git -C ~/dev/webcpp worktree
  remove`. Write the ledger's Changelog line where the config plan's rule
  puts it (`doc/changelog.adoc`, "New Libraries"), as its own commit by
  `$G`, and close the ledger.

---

## Decisions this plan makes for the owner

The spec is never edited. Two items below are proposed amendments (A1,
A2); the rest settle what the spec leaves open or what a ruling asked the
plan to choose. Each names where it applies.

**A1. Numbers: the exact decimal of the shortest text (proposed amendment,
open).**
- The ruling asks every instance-to-schema comparison, and `multipleOf`, to
  be exact: integers exactly, "a double by its exact binary value", the
  instance never rounded to `decimal64_t`. Spec 12 says an instance double
  is "converted once to `Number`".
- Both wordings fail required tests of the pinned suite:
  - "converted once to `Number`" rounds 2^64 to `1.844674407370955e19` in
    `decimal64_t`, so `{"maximum": 18446744073709551615}` accepts it
    (the reviewer's reproduction);
  - "its exact binary value" makes the double `1.1` greater than the schema
    literal `1.1`, so `{"exclusiveMinimum": 1.1}` accepts it, and makes
    `0.0075` no multiple of `0.0001`, which the suite expects valid.
- What the plan builds (E1.2): an `int64_t` or `uint64_t` is compared
  exactly; a double is read once as the exact decimal its shortest
  round-trip text denotes (at most 17 digits, in a 128-bit coefficient);
  a schema literal keeps its exact text where `Number` cannot hold it (E1.6's
  exact table). Every comparison and `multipleOf` runs on these exact
  decimals, never on a rounded instance.
- Proposed wording for spec 12, replacing "converted once to `Number`": "An
  instance integer is compared exactly. An instance double is read once as
  the decimal its shortest round-trip representation denotes, which is
  exact, and compared exactly with the schema's number; it is never rounded
  to `Number`."
- Open: the controller confirms the reading of the ruling, or the owner
  amends spec 12. E1.2 is written for this wording; under "exact binary
  value", E1.2's tests for `1.1` and `0.0075` flip and the suite's two cases
  become permanent known failures.

**A2. `format_entry::check_in` (proposed amendment).** Spec 10 asks the
default set to call `is_iri`, `is_iri_reference` and `is_regex` with the
validation's resource; spec 4.9's `format_entry` cannot carry it. E5.5
appends an optional `check_in` member (PD6). It changes an approved public
type by addition only.

**PD1. The English messages and the four renderers are E1's (E1.13).**
`outcome.hpp` is frozen at E1's end, and a header-only outcome implements
`message()` and `to_json()` in place, so what they produce is fixed there.
E1 also defines the message hook's interface (`message_hook.hpp`, frozen).
E3 does the rest of the messages: the `.pot` catalogue, the Boost.Locale
header that implements the hook, the output suite's proof, and a golden per
kind. The ruling says "E3 implements the messages"; this split is reported
as open item 3.

**PD2. An unknown keyword's kind carries its meaning (E2.7).** In 2019-09
and 2020-12 it is `unknown` (it annotates); in drafts 4 to 7 it is
`ignored` (inert); in OpenAPI 3.0 a non-`x-` one is refused. The IR forbids
the compiler to read the dialect ([ir-3]), so the loader decides. A
deviation from spec 6.2's "unknown" rows for drafts 4 to 7, which the
conformance chapter states.

**PD3. Equivalence is modulo the emitter's rewrites (section E4, E4.1).**
`defs` is not compared; a node whose one comparable keyword is a `ref` is
transparent; an `all_of` of one transparent node reads as its `ref`;
`ignored` keywords are not compared. So E4.4's and E5.11's round trips pass
by construction.

**PD4. A `$ref` with evaluated siblings is written `"allOf": [{"$ref":
...}]`** in drafts 4 to 7 and OpenAPI 3.0 (E4.3), the one form that keeps
the verdicts where `$ref` hides its siblings.

**PD5. `error_kind::schema` (41) is reserved:** no keyword raises it (E3.4).

**PD6. `format_entry` gains `check_in`** (amendment A2; E5.5).

**PD7. Draft 4's `integer` (E5.3).** An `int64_t` or `uint64_t` is an
integer; a double is one only when integral and outside both 64-bit ranges,
where Boost.JSON stores an integer literal it cannot hold. The suite's
`zeroTerminatedFloats` and `bignum` both pass; `1e60` counts as an integer,
the one deviation, stated in the conformance chapter.

**PD8. `time` admits leap seconds in `default_formats()` (E5.5).** This
settles the formats plan's flag that v1's refused policy is unreachable:
json_schema has no v1 dialect (spec 21 item 12), and a program wanting v1's
rule passes its own entry, which `format_test` pins. When v1 lands, its
dialect's default set takes the refused policy.

**PD9. The Bowtie lane is local (E5.8).** It needs Docker and a pinned
Bowtie, which the CI's own-lane job does not install; the harness's
transcript runs on every target. A deviation from spec 16.8's CI lane,
stated in the conformance chapter, until a superproject change adds both to
the job.

**PD10. Two plans (E5.12).** `validate` runs a record plan, touched only by
optimizations that change nothing recorded; `is_valid` runs a flag plan
with every optimization. So the optimizations never change an error, its
order or a golden, which the E2 to E5 review's I19 found they would.

**PD11. The benchmark's chart is an inline SVG inside `doc/benchmark.adoc`
(E5.17)**, not spec 18.13's separate `doc/benchmark.svg`: the doc toolchain
has no image handling.

**PD12. `output/message_hook.hpp` holds the hook's alias alone (E1.8)**,
which spec 4.11 puts in `output/messages.hpp`, so that the hook freezes
without the templates.

**PD13. `collect` lives in `output/error.hpp` (E1.5)**, not spec 3's
`output/outcome.hpp`, because the collecting sink needs it first;
`outcome.hpp` includes it.

**PD14. The name `validator` is well-formed at every point:** it is
`validator_for<draft2020_12>` from E1.9, and spec 4.5's six from E5.0b, when
the last dialect has its machinery. A test that means one dialect names
`validator_for<...>`.

**PD15. References are defined after resolution (E2.3):** the walker keeps
the keyword buffer of each node that holds an open `ref` or `dynamic_ref`,
and calls `define` once its targets are resolved, so the frozen document
is never mutated.

**PD16. With `validate_schema`, the meta-schema check runs before the walk
(E2.5);** `load()` never checks. OpenAPI 3.0's refusals are therefore
`meta_schema_violation` by default and `unknown_keyword` without the check
(E5.0b pins both).

**PD17. The URL policy links `/webcpp//boost_url`** (S4), replacing spec
3's `/boost//url`, by the owner's decision of 2026-10-09.

## What the owner must decide

1. **Third-party text inside a BSL-1.0 repository.** The bundled
   meta-schemas are "BSD 3-clause *or* AFL-3.0"; the OpenAPI 3.0 schema is
   Apache-2.0; AGENTS chapter 2 keeps webcpp's files BSL-1.0 only.
   - Recommended (a): commit them under `meta_schemas/` with
     `LICENSE-JSON-SCHEMA.txt` (BSD-3-Clause, chosen of the two) and
     `LICENSE-OPENAPI.txt`. Each generated header still opens with
     webcpp's BSL notice, which the lint requires, followed by a line
     naming the text's origin and licence.
   - Alternative (b): fetch them at build time from a pinned commit and
     generate the headers into the build directory; then the library is
     not usable from its headers alone without that step.
   - E2.2 and E5.0b are written for (a), with (b) named in each.
2. **jsonschema-benchmark has no licence** (no LICENSE at `724607b`).
   - Recommended: clone it at run time into `.local/`, never into the
     repository, publish only our `results.json`, and ask sourcemeta to add
     a licence.
   - Alternative: the micro-benchmark alone.
   - E5.15 to E5.17 are written for the recommended outcome; E5.16 waits.
3. **The benchmark's pins** (AGENTS chapter 6): the benchmark's commit,
   Blaze's commit (AGPL, run in its container, never linked) and ajv's
   version. E5.16 waits.
4. **Bowtie's pin**, and whether its lane stays local (PD9). E5.8's lane
   waits; its harness does not.
5. **Amendment A2** (`format_entry::check_in`), and the reading of
   amendment A1, which the controller may settle as the ruling's intent.

## Review disposition

`E1` is `WEBCPP_JSON_SCHEMA_PLAN-E1-review.md`, `E2` is
`WEBCPP_JSON_SCHEMA_PLAN-E2E5-review.md`. "Fixed" is the reviewer's fix or
its equivalent; "Otherwise" is a stated other fix.

| Finding | Disposition |
| --- | --- |
| E1 C1 canonical repository under `libs/` | Fixed: `~/dev/webcpp-repos/json_schema`, the `absent` check, `track_worktree`, Task P |
| E1 C2 S3 required on `main` | Fixed: `SUPERPROJECT_BASE=infra/shared`, `superproject_worktree` from the local mirror, Task 0 Step 1 |
| E1 C3 deep input through Boost.JSON's recursion | Fixed: the deep-input rule; E1.3 arena and iterative `deep_copy`; E1.6 clamped `max_depth`; E1.7 views; E1.15 measured bound and `hostile_test` with the reproductions |
| E1 C4 instance doubles rounded to `decimal64_t` | Otherwise: exact decimals of the shortest text (E1.2), amendment A1, open |
| E1 I1 S5 is `webcpp.data`, S2's target unused | Fixed: E1.14's `suite-runner` and the shared items table |
| E1 I2 nothing builds `boost_locale` | Otherwise: S4b in the infra plan, consumed by E3.2 |
| E1 I3 no headers-alone glob for `detail/`, `output/` | Fixed: one call per header directory, in each task (shared-file rule) |
| E1 I4 `except` names a file before it exists | Fixed: E1.2 adds no `except`; E4.1 needs none |
| E1 I5 code fails clang-tidy | Fixed: the clang-tidy rules in Global Constraints, applied in every listing |
| E1 I6 `basic_document::number` does not compile | Fixed: qualified `ir::number` (E1.7) |
| E1 I7 `validator` over six dialects ill-formed | Fixed: PD14 |
| E1 I8 `messages` needs `basic_message_hook` | Fixed: `message_hook.hpp` in E1.8, frozen (PD12) |
| E1 I9 parallel group not independent | Fixed: E1.1, E1.2 serial, then E1.3, E1.4 and E1.5 at once, by `task_worktree` and `merge_task` |
| E1 I10 frozen check after the merge | Fixed: `merge_track` diffs `$FROZEN` before the fast-forward |
| E1 I11 `record_known` may run the wrong binary | Fixed: `-a`, its own build dir, the native runner found and sorted |
| E1 I12 impossible allocation asserted | Fixed: E1.3's deep chain and wide spill tests |
| E1 I13 P4 re-run exponential | Fixed: at most two top-level runs (E1.12), guarantee 7, the nested `anyOf` test |
| E1 I14 collecting sink drops under `everything` | Fixed: the record's `counts` flags (E1.5); verbose keeps failed branches (E1.13) |
| E1 I15 E3 consumes unfrozen files | Fixed: record, outcome, error and hook frozen at E1.17; PD1 |
| E1 I16 shape tests red under the meta-check | Fixed: `compile_errors_test` uses `load()`; PD16; hostile compiles without the check |
| E1 I17 `reason_for` keyed by file | Fixed: permanent table, file, schema (E1.14); closed in E2.11 and E5.18 |
| E1 I18 a pattern overflows SRELL's compiler | Fixed: `shape_of` bounds (E1.4), measured (E1.15), hostile cases |
| E1 I19 E1.8 and E1.9 prose, too large | Fixed: E1.8 to E1.11, each with its code |
| E1 M1 "six counts" | Fixed: the eight counts (E1.7) |
| E1 M2 `check()` misses parts of invariants 1, 2, 7 | Fixed: `ir_test`'s refused cases (E1.7) |
| E1 M3 undefined `compile_options_type` | Fixed: `basic_compile_options<Traits>` (E1.9) |
| E1 M4 `{"items": 1}` code | Fixed: the child refused by the walk (E1.9) |
| E1 M5 duplicate key leaves a stale exact number | Fixed: E1.6's last-wins case |
| E1 M6 `aligned_alloc` alignment | Fixed: pratt's `aligned_memory` (E1.15) |
| E1 M7 track names in tracked files | Fixed: the text rule; comments say what is true now |
| E1 M8 `**` without globstar | Fixed: the `find tools` loop |
| E1 M9 `git add include` | Fixed: `git diff --name-only -- include`, by path |
| E1 M10 file structure lists absent files | Fixed: the file structure |
| E1 M11 `error_kind` values unpinned | Fixed: `values_are_fixed` cases (E1.5, E1.7) |
| E1 M12 `compile_errors_test` misses codes | Fixed: E1.9, E1.11 |
| E1 M13 unused `path` in `sink_test` | Fixed: E1.5 |
| E1 M14 stale S3 row | Fixed: the shared items table |
| E1 M15 inaccurate fail expectations | Fixed: E1.3, E1.6 |
| E1 M16 `keyword_location` holds views | Fixed: indices (E1.10) |
| E1 M17 machine lacks `allocator_type` | Fixed: E1.11 |
| E1 M18 ordering text | Fixed: the parallel map |
| E1 M19 `any_of` comment | Fixed: replaced by the two-run replay |
| E1 M20 Task 0 gates on S5 | Fixed: E1.14 checks S5 |
| E1 M21 "never add a remote" | Fixed: a clone's `origin` is the one remote (Global Constraints) |
| E1 M22 inline frames unstated | Fixed: 32 frames, tied to the common path (E1.11, E1.15) |
| E2 C1 E5 uses `js::validator` too early | Fixed: E5.0a and E5.0b, serial, before the parallel group |
| E2 C2 equivalence and emitter disagree | Fixed: PD3, with the emitter's pairs in `equivalent_test` |
| E2 C3 nothing produces `boost_locale` | Otherwise: S4b in the infra plan (open item 2 rechecks it) |
| E2 I1 repository location lines | Fixed: `$CANONICAL` everywhere, `track_worktree 5`, Task P |
| E2 I2 `<webcpp-dir>` in E2 to E5 | Fixed: `[ webcpp.data ... ]` in every line |
| E2 I3 E2 mutates a defined keyword | Fixed: PD15 |
| E2 I4 unknown keyword annotation by dialect | Fixed: PD2 |
| E2 I5 anchor inside an unknown keyword | Fixed: E2.3's test |
| E2 I6 meta-check order has no mechanism | Fixed: PD16, both orders pinned |
| E2 I7 compatibility test cannot fail | Fixed: E2.7's test, E2.5's cache and `what` |
| E2 I8 annotation test vs the suite | Fixed: `admits_all`, root-pointer keys, after E2.4 (E2.8) |
| E2 I9 overlapping parallel files | Fixed: the shared-file rule, each rebase's Expected, `runner.hpp` rule in E3.3 and E4.4 |
| E2 I10 E5's group unowned edits | Fixed: E5.0b owns them; `merge_e5` and `record_all` |
| E2 I11 `reason_for` blames a finished track | Fixed: E2.11, the E5 dialect rule, E5.18 |
| E2 I12 no permanent known failure | Fixed: `permanent_reasons` (E1.14), rows in E5.5, E5.18's check |
| E2 I13 output and round-trip lists start empty | Fixed: `record_known` and the output mode's code (E3.3, E4.4) |
| E2 I14 goldens miss kinds | Fixed: E3.4's two tables; `reference` recorded (E2.3); PD5 |
| E2 I15 `{count}` details unstated | Fixed: E1.11 stores every measured count |
| E2 I16 OpenAPI runner unspecified | Fixed: E5.0b's `run_openapi`, data, `"null"` filter, decision 1 (a)/(b), loader test |
| E2 I17 OpenAPI reload with `components` | Fixed: E5.11's registry and `point_into` |
| E2 I18 round trips and annotations stop at 2020-12 | Fixed: E5.11 |
| E2 I19 optimizations change error order | Otherwise: PD10, two plans |
| E2 I20 fuzz corpus copies the suite | Fixed: E5.7 seeds into `bin/`, commits only regressions, Docker command |
| E2 I21 Bowtie build and lane broken | Otherwise: `install.sh`'s sha, `compare.py`, a staged context, the pin (decision 4), PD9 |
| E2 I22 benchmark unspecified | Otherwise: E5.15 to E5.17, b2 twice, every version, PD11, the notice, page with the owner's files |
| E2 I23 known failures not on the page | Fixed: `known_failures.py` through S1, OpenAPI counted (E5.18), harness shown (E5.19) |
| E2 I24 staging by directory | Fixed: every commit by path, `-- <paths>` |
| E2 I25 docs and examples without code | Fixed: each example with its `.expected` |
| E2 I26 tasks too large | Fixed: E2.5 to E2.7 and E2.10; E3.1 to E3.4; E5.5, E5.9, E5.10; E5.12 to E5.14; E5.15 to E5.17; E5.18 to E5.20 |
| E2 I27 `vocabulary_test` incomplete | Fixed: E5.6 |
| E2 M1 overlong URL code | Fixed: `value_too_large` (E2.1) |
| E2 M2 untested registry paths | Fixed: E2.3 |
| E2 M3 use S1; CRLF; chunk bytes | Fixed: E2.2 |
| E2 M4 BSL notice in generated headers | Fixed: owner decision 1's text |
| E2 M5 E2.4 type, chain, scope | Fixed: E2.4 |
| E2 M6 commit after merge | Fixed: commit steps precede every merge |
| E2 M7 `validating.adoc` missing | Fixed: E2.11's Files |
| E2 M8 truncation wording, plurals | Fixed: E1.13 |
| E2 M9 renderer goldens | Fixed: E1.13 |
| E2 M10 E3.3 Files | Fixed: E3.4 lists `messages_test.cpp` |
| E2 M11 locale diagnostic, `mo.hpp`, page | Fixed: E3.2, E3.4 |
| E2 M12 `pattern_limit` row | Fixed: E3.4 |
| E2 M13 `equivalent` allocates by default | Fixed: E4.1's resource overload, the condition in code |
| E2 M14 emitter reuses E2's colouring | Fixed: `detail/emitter/reach.hpp`; base URI rule (E4.2) |
| E2 M15 missing emitter cells | Fixed: E4.3 |
| E2 M16 dereferenced error result | Fixed: `(reloaded && *reloaded)` (E4.4) |
| E2 M17 recursive row without code | Fixed: E5.1 |
| E2 M18 draft 4 `integer` across dialects | Fixed: E4.3's row, E5.11 |
| E2 M19 cross-dialect placeholders | Fixed: E5.11 |
| E2 M20 `check_in` and missing listings | Fixed: A2, owner decision 5, E5.5's Files and goldens, E5.10's patterns |
| E2 M21 fuzz timing, bodies, macOS guard | Fixed: E5.7 |
| E2 M22 harness location | Fixed: `example/bowtie_webcpp.cpp` (E5.8) |
| E2 M23 Python lines over 100 | Fixed: every listing within 100 columns |
| E2 M24 untested `default_dialect` and OpenAPI options | Fixed: E1.9's `compile_errors_test`, E5.0b's loader test |

Counts: 99 findings (E1: 4 Critical, 19 Important, 22 Minor; E2: 3
Critical, 27 Important, 24 Minor). 93 fixed as proposed, 6 fixed otherwise
(E1 C4, E1 I2, E2 C3, E2 I19, E2 I21, E2 I22), none declined. One stays
open: E1 C4's amendment A1.
