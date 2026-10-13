# webcpp shared superproject infrastructure Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Give the superproject `~/dev/webcpp` the shared pieces that
webcpp's new libraries idna, formats and json_schema need (the check of
generated files, `third_party/` with the JSON Schema Test Suite, the SRELL
mirror and its target, `/webcpp//boost_url`, `/webcpp//boost_locale` and
`/webcpp//boost_charconv` built from Boost's sources, and test data on WASI),
each a reviewed change, consumed by the library tracks from `infra/shared` and
integrated into `main` once stage 4 and the config plan have.

**Architecture:**
- One serial track of superproject changes, in the worktree
  `~/dev/webcpp-infra` on branch `infra/shared`. Each task is one S item. Nothing
  reaches `~/dev/webcpp` `main` (M) before stage 4's Task 14 has published and
  the config plan's Tasks 11 and 12 have integrated; until then the library
  tracks base their worktrees on `infra/shared` (section "Consumer interface"),
  and after it the controller rebases the track onto config's `main` (section
  "Integration"). Nobody pushes M until the owner's go.
- Everything new is a b2 rule, target or Jamfile with its tests in `tools/`,
  red first, as AGENTS.md chapter 10 asks of a change of the build:
  - S2: `third_party/Jamfile` (`/webcpp/third_party`) and the submodule
    `third_party/libs/JSON-Schema-Test-Suite`, given to a test by the target
    `json-schema-test-suite`;
  - S1: `webcpp.generated`, a module `tools/generated/generated.jam` with a
    Python driver, reusing `tools/oracle/compare.py`;
  - S4: `tools/boost/Jamfile`, a project whose source location is Boost's
    source tree, and the Jamroot's alias `/webcpp//boost_url`, which checks
    that tree's version; the CI's Boost action keeps the sources;
  - S4b: `/webcpp//boost_charconv` and `/webcpp//boost_locale` from the same
    tree, Boost.Locale without ICU, with the backends measured per platform
    (posix and std with iconv; winapi and std on Windows), refused by name on
    emscripten (Boost.Thread's mutex needs threads) and without exceptions;
  - S5: `webcpp.data`, a conditional usage requirement that appends
    wasmtime's `--dir` to `testing.launcher` on WASI;
  - S3: the local mirror repository of SRELL with three commits, its
    submodule `third_party/libs/srell` and the target `srell`.
- Outward actions (creating `webcpporg/srell`, pushing, filing upstream) are
  the controller's last step, after the owner's go.

**Tech Stack:** b2 5.5.3 (B2 1.92), Boost 1.92.0 (URL, Charconv, Locale
without ICU), Apple clang 21 and gcc-15 natively, wasi-sdk 34 with wasmtime 47.0.3, Emscripten 6.0.11 under
Node, Python 3.9+, SRELL 2026.07, JSON-Schema-Test-Suite `5b0ee16`, GitHub
Actions (the Boost action only).

**Spec:**
- `~/.buzz/PLANS/WEBCPP_IDNA_SPEC.md` section 12 (S1) and sections 3.1, 8.1,
  8.3, 13 risk 7 (binding);
- `~/.buzz/PLANS/WEBCPP_FORMATS_SPEC.md` section 8.3 (S2 to S5), 8.4 (the
  SRELL patch), 8.2, 9.1, 12, 13 risks 1, 2, 4, 10, and the two owner
  decisions at its end (binding);
- `~/.buzz/PLANS/WEBCPP_JSON_SCHEMA_SPEC.md` sections 3 (superproject files),
  4.2 (SRELL macros), 10, 15 (WASI `--dir`), 16.1 (the suite's path and
  defines), 20.3, 21 risks 1, 2, 5, 13 (binding where formats defers to it:
  the suite's path is `third_party/libs/JSON-Schema-Test-Suite`);
- `~/.buzz/PLANS/WEBCPP_DESIGN.md`, "Owner decisions after approval", those of
  2026-10-09 (compiled Boost libraries from the source tarball for every
  target, `/webcpp//boost_url` first; the SRELL mirror's three commits);
- research: `~/.buzz/RESEARCH/JSON_SCHEMA_ENGINE_SPIKE.md` sections 3, 4, 5, 8;
  `~/.buzz/RESEARCH/SRELL_ALLOCATION_SPIKE.md`, whose prototype's files are
  kept in `~/.buzz/RESEARCH/SRELL_ALLOCATION_SPIKE/spike/` (read only);
  `~/.buzz/RESEARCH/BOOST_LOCALE_ON_EVERY_TARGET.md` (Boost.Locale and
  Boost.Charconv measured on each target, 2026-10-09);
- the independent review
  `~/dev/webcpp/.superpowers/sdd/plan-reviews/WEBCPP_SHARED_INFRA_PLAN-review.md`
  and the controller's rulings on it, answered in "Review disposition" at the
  end; the config plan `~/.buzz/PLANS/WEBCPP_CONFIG_PLAN.md`, which integrates
  before this track.

## Global Constraints

**Hard rules, verbatim from the owner:**
- Commit with `git -c user.name=R -c user.email=github.overboard312@passmail.net commit`. No trailer, and never an agent co-author.
- The machine's global git identity never reaches a webcpp commit, as author or as committer (recorded in `WEBCPP_DESIGN.md`): every git command that writes a commit runs as `git -c user.name=R -c user.email=github.overboard312@passmail.net ...`, whether it is a commit, a rebase, a cherry-pick, a merge that is not a fast-forward, an `am` or an amend; and before each integration the controller checks `git log --format='%an %ae %cn %ce'` over the range, which prints only `R github.overboard312@passmail.net R github.overboard312@passmail.net`.
- Stage files by path. Never `git add -A`, and never `git add libs`. Never push or touch GitHub; the controller does that.
- English only. Never the em dash; use a plain dash. No case-insensitive `veru`+`s`. No "stage N" or "Task N" in tracked files.
- Every new source file carries the BSL-1.0 notice, "Copyright (c) 2026 WebCpp.org". Vendored or mirrored third-party code keeps its own licence and notice.
- Run b2 as `env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH b2 --user-config=.local/user-config.jam`. Verdicts use `-a`. Each parallel lane gets its own `--build-dir`.
- Never write in `~/dev/` followed by the banned word and `fi` (the old checkout), or in `.local/emsdk`.

**Where the work happens:**
- Worktree `~/dev/webcpp-infra`, branch `infra/shared`, made from `main` by
  the controller (Task 0). `~/dev/webcpp` is stage 4's checkout: no task
  writes there; only the controller integrates (section "Integration").
- Every command runs from `~/dev/webcpp-infra` unless a step says otherwise.
  In these steps `B2` stands for
  `env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH b2 --user-config=.local/user-config.jam`
  (set it in the shell with `B2='env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH b2 --user-config=.local/user-config.jam'`
  and run `$B2 ...`).
- `EM_CACHE` is `~/dev/webcpp/.local/emscripten-cache` (export it before a
  tools test that builds for emscripten); nothing writes inside the emsdk.
- Nothing reads from GitHub either, except the controller: the suite is
  cloned from `~/.buzz/REPOS/JSON-Schema-Test-Suite` (Task 0), SRELL comes
  from akenotsuki.com, Boost's archive from archives.boost.io.

**Text and names:**
- The S labels (S1 to S5) and plan task numbers never appear in a tracked
  file or a commit message; a commit subject is plain, prefixed by the part
  it touches (`tools: ...`, `third_party: ...`).
- A git body paragraph never starts with `word:`.
- Jam comments keep to 80 columns, Python to 100 columns with two blank
  lines before a top-level `def`; every Python file passes Pyright.
- The names this plan fixes: `/webcpp/third_party//json-schema-test-suite`,
  `WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE`, `/webcpp/third_party//srell`,
  `/webcpp//boost_url`, `-sBOOST_SOURCE=<dir>`, `.local/boost-source`,
  `webcpp.generated`, `generated-<name>`, `update-generated`, `webcpp.data`.

**The green of a superproject change** (every task's last verification; all
of it, read, not assumed):
1. The tools tests, each from the root, all passing:
   `for t in $(find tools -name '*_test.py' -not -path '*/node_modules/*' | sort); do python3 "$t" > "/tmp/infra-$(basename "$t").log" 2>&1 && echo "ok $t" || echo "FAILED $t"; done`.
   Expected: one `ok` line per file, no `FAILED`.
2. The lanes, from scratch, in parallel, each with its own build directory:
   ```
   $B2 -a --build-dir=bin/lane-clang test example > /tmp/infra-clang.log 2>&1 &
   $B2 -a --build-dir=bin/lane-gcc-15 toolset=gcc-15 test example > /tmp/infra-gcc.log 2>&1 &
   $B2 -a --build-dir=bin/lane-wasip2 toolset=clang-wasip2 testing.launcher=wasmtime test example > /tmp/infra-p2.log 2>&1 &
   $B2 -a --build-dir=bin/lane-wasip3 toolset=clang-wasip3 testing.launcher=wasmtime test example > /tmp/infra-p3.log 2>&1 &
   wait
   ```
   and, when `python3 tools/ci/matrix.py declares emscripten` prints `true`,
   `$B2 -a --build-dir=bin/lane-emscripten toolset=emscripten test example`.
   Expected: each exits 0, and `grep -c '^\.\.\.failed' /tmp/infra-*.log`
   prints 0 for each.
3. The own lanes: `$B2 -a --build-dir=bin/lane-oracle libs/xstate/test/oracle//oracle`;
   `$B2 -a --build-dir=bin/lane-http-p2 toolset=clang-wasip2 testing.launcher=wasmtime libs/wasi/test//http libs/wasi/example//http`
   and the same with `clang-wasip3` and `bin/lane-http-p3`. Expected: exit 0.
4. `$B2 -a --build-dir=bin/doc doc`. Expected: exit 0.
5. `tools/lint/lint.sh --clang-format .local/wasi-sdk/bin/clang-format --clang-tidy .local/wasi-sdk/bin/clang-tidy`.
   Expected: the last line is `lint: clean`.
6. A lint, test or flakiness failure met on the way is fixed (or, when it is
   not this track's to fix, reported by name in the task's report).

**Every task's report ends with a "Changelog" section** in Boost's style
(Added, Changed, Fixed, *Breaking change:*), grouped under "Tools and build"
or a library's name; each task below gives its lines. The controller merges
them into `~/.buzz/PLANS/WEBCPP_CHANGELOG_PENDING.md`.

## Review Focus

1. **A checkout without its third-party submodules** (a clone made without
   `--recursive`, a worktree whose `third_party/libs/*` was never
   initialised): a suite runner would find no file and pass on nothing, and a
   program including `<srell.hpp>` would fail with a bare "file not found".
   Expected: the build that needs it stops, naming
   `git submodule update --init third_party/libs/<name>`, and one that needs
   none runs. Pinned in Task 1 (`test_a_suite_not_checked_out_is_refused`)
   and Task 5 (`test_srell_not_checked_out_is_refused`).
2. **Boost's sources missing, partial, of another version, or the system's
   prebuilt Boost.URL linked instead** (Homebrew ships `libboost_url`): a
   silent ODR mismatch or a lane green against the wrong library. Expected:
   refused by name, naming both places looked at or both versions, and every
   link names the archive the build made, never `-lboost_url`. Pinned in Task
   3 (`test_missing_sources_are_refused_naming_the_fix`,
   `test_sources_of_another_version_are_refused`,
   `test_a_tree_without_the_library_s_sources_is_refused`,
   `test_boost_url_links_on_every_target`).
3. **A path that holds a space** (every scratch superproject under
   `$TMPDIR`, a data directory, the suite's define, wasmtime's `--dir`,
   `BOOST_SOURCE`): a word split in an action turns a pass into a failure, or
   into a program that read nothing. Expected: every one builds and runs.
   Pinned by the harness's spaced roots in every test, and in Task 4 by a data
   directory named `the data` (`test_a_test_reads_its_data_on_every_target`).
4. **A generator that fails halfway, or a stale generated file in the
   ordinary lanes:** `update-generated` must never leave a committed file half
   written, the check must name every differing output (not stop at the first),
   and the ordinary lanes must not run a generator (idna's needs the network).
   Pinned in Task 2 (`test_a_failing_generator_leaves_every_committed_file`,
   `test_a_generator_failing_after_one_output_leaves_every_committed_file`,
   `test_each_differing_output_fails_by_name`,
   `test_the_lane_is_listed_and_the_ordinary_lanes_skip_it`).
5. **Third-party code meeting webcpp's strictness:** SRELL's own headers warn
   under `-Wall -Wextra` (measured: an unused variable `boffset` at
   `srell.hpp:5416`), Boost.URL's sources are not written for webcpp's
   `-Werror`, and the lint would read the suite's and SRELL's files or analyse
   Boost's sources. Expected: their warnings are not ours, and the lint never
   reads them. Pinned in Task 1 (`test_third_party_code_is_never_read`), Task 3
   (`test_a_compiled_boost_library_s_sources_are_not_analysed`) and Task 5
   (`test_srell_matches_on_every_target` compiles under the project's
   `-Wextra -Werror`).

---

## Order, and what each task unblocks

The order puts first what the library tracks need first, and last the item
whose integration waits for the owner, since the track is integrated serially.
A library track uses an item as soon as `infra/shared` carries it (section
"Consumer interface"), long before integration.

| Task | Item | Unblocks |
| --- | --- | --- |
| 0 | worktree, local clones (controller) | everything below |
| 1 | S2: `third_party/`, the JSON Schema Test Suite | formats F1 (its suite slice, native and emscripten); json_schema E1's runner, natively and on emscripten |
| 2 | S1: `webcpp.generated` | idna I2 (with the owner's P8 decision), its one consumer: json_schema's generators write fixed paths and its checks are C++ tests. Cross-checked against the IDNA plan when it exists |
| 3 | S4: `/webcpp//boost_url` from Boost's sources | formats F3, then F4, F5, F6; json_schema E2's URL policy (its `build.jam` writes `<library>/webcpp//boost_url`, not `/boost//url`) |
| 3B | S4b: `/webcpp//boost_charconv`, `/webcpp//boost_locale` | json_schema E3.4 (the locale header and its test, emscripten included since the owner's acceptance in Task 3B's fix round 2) |
| 4 | S5: test data on WASI | formats F8; every json_schema sub-track's slice on wasip2 and wasip3 |
| 5 | S3: the SRELL mirror, `third_party/libs/srell`, `srell` | formats F7; json_schema E1's regex policy (E1.4), from `infra/shared` and the local mirror (section "Consumer interface"); in `main` only after Task 7 |
| 6 | upstream drafts in `~/.buzz/OUTBOX/` | the owner's filing |
| 7 | publication of the mirror and integration of Task 5 (controller, after the owner's go) | S3 in `main` |

## Consumer interface

Every line of this section is exact: a library track's plan copies it as it
is. The controller patches the formats and json_schema plans to it before
their dispatch; this plan never edits them.

**A worktree based on this track.** Until integration (section
"Integration"), a library track's superproject branch is based on
`infra/shared`, never on M's `main`, and every submodule comes from a local
clone, never from GitHub:

```bash
git -C ~/dev/webcpp worktree add <worktree> -b <branch> infra/shared
git -C <worktree> -c protocol.file.allow=always \
  -c submodule.third_party/libs/JSON-Schema-Test-Suite.url="$HOME/.buzz/REPOS/JSON-Schema-Test-Suite" \
  -c submodule.third_party/libs/srell.url="$HOME/.buzz/REPOS/srell.git" \
  submodule update --init third_party/libs/JSON-Schema-Test-Suite third_party/libs/srell
git -C <worktree> submodule status third_party/libs
```

Expected: one line per submodule, without a leading `-` or `+`:
`5b0ee1613e45fcc2bddac00e07c19cd49b00d8a8 third_party/libs/JSON-Schema-Test-Suite (...)`
and `<the mirror's third commit> third_party/libs/srell (...)`. Measured with
git 2.55: the `-c submodule.<path>.url` holds for that one command and writes
nothing into the repository's shared configuration, so M and the other
worktrees keep `.gitmodules`' URLs; git 2.38.1 and later refuse the `file`
transport for a submodule unless `protocol.file.allow=always`. A branch
already made is moved with
`git -C <worktree> -c user.name=R -c user.email=github.overboard312@passmail.net rebase infra/shared`,
then the same `submodule update`. Before `infra/shared` carries Task 5, `srell` is left
out of both lists. No branch that carries Task 5's commit is merged into M's
`main` before Task 7. These commands replace, in the consumer plans, their
`submodule update --init` of these paths and their checks that an item is on
`main`. Boost's sources: at Task 3's completion the controller copies
`~/dev/webcpp-infra/.local/boost-source` to `~/dev/webcpp/.local/boost-source`
(machine-local, never tracked), so a worktree whose `.local` links to M's has
it; otherwise `bash tools/ci/actions/boost/install.sh source .local/boost-source`.

**What a test or a library names:**

| Line in a Jamfile or a `build.jam` | What it gives |
| --- | --- |
| `<library>/webcpp/third_party//json-schema-test-suite` in a `webcpp.run`'s (or `webcpp.run-fail`'s) requirements | the define `WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE`, a string literal: the suite's root, absolute, with forward slashes and no trailing slash, which a test extends by concatenation (`WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE "/tests/draft2020-12/type.json"`; formats writes `WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE "/tests"`, json_schema `"/remotes"` for the remotes); every `*.json` of `tests`, `remotes`, `output-tests` and `annotations` as a dependency of the test; the suite's directory under wasmtime on wasip2 and wasip3 (Task 4); and a refusal by name when the submodule is not checked out. No path constant reaches a library: `JSON_SCHEMA_TEST_SUITE` lives in `third_party/Jamfile` alone |
| `<library>/webcpp/third_party//srell` | `#include <srell.hpp>`, a system header; `SRELL_NO_UBMOD`; `SRELL_NO_THROW` in a build without exceptions |
| `<library>/webcpp//boost_url` | Boost.URL's compiled library, every target |
| `<library>/webcpp//boost_charconv` | Boost.Charconv's compiled library, every target, without `__float128` (no quadmath) |
| `<library>/webcpp//boost_locale` | Boost.Locale's compiled library without ICU, with Boost.Charconv: backends posix and std with iconv on Linux, macOS (`-liconv`), wasip2, wasip3 and emscripten (single-threaded: `BOOST_HAS_PTHREADS` on Boost.Locale's sources alone, no `-pthread`); winapi and std on Windows; a refusal by name in a build without exceptions (Task 3B). Every program that names it, directly or through a library's target, declares `<exception-handling>on` in its own requirements (b2 ignores it as a usage requirement): without it, a build with `exception-handling=off`, the lint's twins included, stops whole on the refusal, which names that fix but not the program |
| `[ webcpp.data <directory> + ]` in a `webcpp.run`'s or `webcpp.run-fail`'s requirements | each directory, absolute (a `path-constant`), read at the same path on every target: under wasmtime as `wasmtime --dir="<directory>" <program>`, natively and on emscripten as the host's own files. A relative directory stops the build. It does nothing for `webcpp.example` or `webcpp.boost-test` |

`webcpp.data`, as a test Jamfile writes it for a directory of its own (the
define is the test's to name, `WEBCPP_TEST_*`):

```
import regex ;
import webcpp ;

path-constant KNOWN_FAILURES : known_failures ;
local known-failures = [ regex.replace $(KNOWN_FAILURES) "\\\\" "/" ] ;
webcpp.run suite-draft2020-12 : suite_test.cpp
  : <library>/webcpp/third_party//json-schema-test-suite
    <define>"\"WEBCPP_TEST_KNOWN_FAILURES=\\\"$(known-failures)\\\"\""
    [ webcpp.data $(KNOWN_FAILURES) ] ;
```

json_schema's `<webcpp-dir>$(X)` is written `[ webcpp.data $(X) ]`, at every
occurrence; its Task 0 check is
`grep -n 'rule data ( directories + )' tools/webcpp.jam`, which prints one
line once `infra/shared` carries Task 4.

## Integration (controller)

Nothing of this track reaches M's `main` while stage 4 runs (the owner's
2026-10-09 "overnight execution": every track local, no push without the
owner's go). The order, once stage 4 has published:

1. stage 4's Task 14 complete (its ledger's last line says the stage is
   complete);
2. the config plan's Tasks 11 and 12 (config integrates first, and retires
   `~/.buzz/PLANS/WEBCPP_CHANGELOG_PENDING.md`);
3. this track, Tasks 1, 2, 3, 3B and 4, rebased onto config's `main`;
4. Task 5, once Task 7 has published the mirror.

The steps, each time:

1. **The gate, checked just before the rebase:** the stage-4 ledger shows no
   task dispatched or committing to the superproject (its last line is a
   `complete`), and the config ledger shows its Task 12 complete with nothing
   dispatched after it. The time of the check goes into both ledgers.
2. **Rebase what may be integrated** (all of `infra/shared` once Task 7 is
   done; otherwise up to Task 4's commit):

```bash
cd ~/dev/webcpp-infra
T4=$(git log --format=%H -1 --grep='^tools: webcpp.data' infra/shared)
git checkout -B infra/integrate "$T4"           # before Task 7: Tasks 1, 2, 3, 3B and 4
# git checkout -B infra/integrate infra/shared  # after Task 7: Task 5 too, in place of the line above
git -c user.name=R -c user.email=github.overboard312@passmail.net rebase main
git log --format='%an %ae %cn %ce' main..infra/integrate | sort -u
```

   The last command prints one line, `R github.overboard312@passmail.net R github.overboard312@passmail.net`;
   any other line stops the integration (the commit is rewritten with the
   right identity, by `git -c user.name=R -c user.email=github.overboard312@passmail.net
   commit --amend --reset-author` on that commit, before anything else).

   A conflict in a file of "Conflicts with parallel tracks" is resolved by
   keeping both sides, additions only; then the task's own tests run again.
3. **The changelog.** Config's Task 12 has retired the pending file: each
   task's report keeps its "Changelog" lines in this track's ledger, and one
   commit on `infra/integrate`, `doc/releases: the Tools and build lines of
   the shared superproject infrastructure`, writes them into
   `doc/releases/0.1.0.adoc` under "Tools and build", in the form config's
   lint rule `changelog` checks:
   `git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "doc/releases: the Tools and build lines of the shared superproject infrastructure" -- doc/releases/0.1.0.adoc`.
   It is the one place where config's rule "in the same commit" is not met,
   because that file did not exist when these commits were made.
4. **The check in `~/dev/webcpp-infra`, every lane, from scratch:** after
   `git -C ~/dev/webcpp-infra submodule update --init` (the controller's read
   of the published libraries, trystero and config included) and
   `source ~/dev/webcpp/.local/env.sh`, the green of a superproject change,
   items 1 to 6, its item 2 with the emscripten lane, and its item 3 with
   every own lane `python3 tools/ci/matrix.py own-lanes` lists (xstate's and
   trystero's oracle, trystero's interop on native and emscripten, wasi's
   `http` on wasip2 and wasip3, config's own).
5. `git -C ~/dev/webcpp -c user.name=R -c user.email=github.overboard312@passmail.net merge --ff-only infra/integrate`,
   then, when Task 5 is still to come,
   `git -C ~/dev/webcpp-infra -c user.name=R -c user.email=github.overboard312@passmail.net rebase --onto infra/integrate "$T4" infra/shared`,
   and the identity check of step 2 again over `infra/integrate..infra/shared`.
6. **The same check in M**, after
   `git -C ~/dev/webcpp -c protocol.file.allow=always -c submodule.third_party/libs/JSON-Schema-Test-Suite.url="$HOME/.buzz/REPOS/JSON-Schema-Test-Suite" submodule update --init third_party/libs/JSON-Schema-Test-Suite`
   (and the same for `third_party/libs/srell` with `$HOME/.buzz/REPOS/srell.git`
   when Task 5 is in), with `~/dev/webcpp/.local/boost-source` in place. The
   result goes into both ledgers.
7. Tell the library tracks to rebase their branches onto M's `main`
   (`git -c user.name=R -c user.email=github.overboard312@passmail.net rebase main`);
   commits that `main` now carries drop out.
8. Nobody pushes M: its push is the owner's go (config's Task 15, or a later
   go), never this track's.

## Conflicts with parallel tracks

| Shared file | Changed here by | Changed in parallel by | Handling |
| --- | --- | --- | --- |
| `AGENTS.md` (superproject) | Tasks 1 to 5: chapter 1 (layout, prerequisites, build commands), 2 (checklist item 4, a new "Third-party code"), 5, 6, 9 (Jamfile API, tests, the Boost action), 10 (tools tests), 13 (roadmap) | stage 4 Task 12 rewrites chapters 1, 9 and 13; Tasks 5 to 11 may touch chapters 6 and 9. Config Task 10 (chapters 1, 2, 3, 6, 7, 8, 10, 11, 12, 13), Task 13 (2, 3, 6, 13), Task 15 (1) | rebase onto both, additions only; each of our edits is an inserted paragraph or row, re-read against their text; Task 12's "until webcpp builds them from `third_party/`" stays true (libsecp256k1 and libdatachannel are not built from it) |
| `README.md` | Tasks 1, 3, 5 (prerequisites, tests of the tools, licence) | stage 4 Task 12 (prerequisites, the libraries table); config Task 10 | rebase, keep both |
| `pyrightconfig.json` | not changed | config Task 5 (`tools/release`) | none |
| `Jamroot` | Tasks 1 (`use-project /webcpp/third_party`), 3 and 3B (`BOOST_SOURCE`, three aliases, the last `explicit` gains `boost_url boost_charconv boost_locale`) | stage 4: none planned. Config Tasks 6, 8, 9: `define-line`, `webcpp-version`, three targets appended before the last `explicit` | this track rebases onto config: additions only, both sets of names in the one `explicit` line |
| `tools/webcpp.jam` | Tasks 2 (module load, rule `generated`), 4 (feature `webcpp-data`, rule `data`) | stage 4 Task 7 (a lane's kind for Node on `declared-lanes` lines) | rebase; a generated lane runs Python only, so its kind is the oracle-free one Task 7 defines, decided at the rebase, and `generated_jam_test.py`'s exact line `gen words libs/gen/test` gains that kind with it |
| `tools/lint/lint.sh`, `tools/lint/lint_test.py` | Task 1 (third-party files), Task 3 (case) | stage 4 Task 9 (trystero lint-clean); config Task 5 (the rule `changelog`) | rebase, keep both |
| `tools/lint/compile_commands.py` | Task 3 (sources outside the tree) | stage 4 Tasks 7, 9 if the database learns new lanes | rebase |
| `tools/test/harness.py`, `tools/test/harness_test.py` | Task 1 (`third_party/libs` linked, `third_party_of_its_own`), Task 3 (`.local/boost-source`, helpers) | stage 4 Tasks 6, 7 | rebase |
| `tools/test/webcpp_jam_test.py` | Task 4 (cases) | stage 4 Task 7 | rebase, keep both case lists |
| `tools/ci/actions/boost/{action.yml,install.sh}`, `tools/ci/actions_test.py` | Task 3 | stage 4 Task 12 (new actions and their `actions_test.py` cases) | rebase, keep both case lists |
| `.gitmodules` | Tasks 1, 5 | stage 4 Task 14 (`libs/trystero`); config Task 15 (`libs/config`) | append-only; Task 5 never reaches `main` before Task 7 |
| `~/.buzz/PLANS/WEBCPP_CHANGELOG_PENDING.md` | never written: each task's lines wait in this track's ledger | appended to by stage 4 until it ends; emptied by config's Task 11, retired by its Task 12 | the lines go into `doc/releases/0.1.0.adoc` at integration (section "Integration", step 3) |
| M's push | none | config: nobody pushes M or a library between its Tasks 12 and 15 | this track pushes nothing; M's push is the owner's go |
| `~/dev/webcpp/.local/` | the controller, at Task 3's completion (`boost-source`) | stage 4 (its dependencies) | distinct entries |

The library tracks (idna and formats in `~/dev/webcpp-formats`, json_schema
in its worktrees) never edit these files; they consume each item by rebasing.

## File structure

```
~/dev/webcpp-infra/                    worktree of the superproject, branch infra/shared
  Jamroot                              + use-project third_party; BOOST_SOURCE; alias boost_url
  .gitmodules                          + the suite, + srell
  third_party/Jamfile                  /webcpp/third_party: json-schema-test-suite, srell
  third_party/libs/JSON-Schema-Test-Suite   submodule, 5b0ee16 (MIT)
  third_party/libs/srell               submodule, the mirror webcpporg/srell (BSD-2-Clause)
  tools/webcpp.jam                     + webcpp.generated, + webcpp.data and webcpp-data
  tools/generated/generated.jam        the rule of generated files (module webcpp-generated)
  tools/generated/generated.py         runs a generator, checks or writes its outputs
  tools/oracle/compare.py              + --files
  tools/oracle/compare_test.py         + file cases
  tools/boost/Jamfile                  /webcpp/boost-libraries: boost_url, boost_charconv, boost_locale
  tools/ci/actions/boost/install.sh    keeps libs/*/src and boost/version.hpp; `source <dir>`
  tools/ci/actions/boost/action.yml    its description
  tools/ci/actions_test.py             + cases of the Boost action
  tools/lint/lint.sh                   never reads third_party/libs/
  tools/lint/compile_commands.py       never lists a source outside the tree
  tools/lint/lint_test.py              + two cases
  tools/test/harness.py                third_party/libs and .local/boost-source linked in a scratch copy, helpers
  tools/test/harness_test.py           + one case
  tools/test/third_party_test.py       the suite, then SRELL
  tools/test/generated_jam_test.py     webcpp.generated
  tools/test/boost_libraries_test.py   /webcpp//boost_url, boost_charconv, boost_locale
  tools/test/webcpp_jam_test.py        + webcpp.data cases
  AGENTS.md, README.md
~/.buzz/REPOS/JSON-Schema-Test-Suite   local clone of the suite (Task 0)
~/.buzz/REPOS/srell.git                bare clone of the mirror, its local origin (Task 5)
~/.buzz/RESEARCH/SRELL_ALLOCATION_SPIKE/spike/    the spike's files, kept (copied 2026-10-09)
~/.buzz/RESEARCH/SRELL_ALLOCATION_SPIKE/mirror/   make_patch.py and edits.py as the mirror used them
~/.buzz/RESEARCH/BOOST_LOCALE_ON_EVERY_TARGET.md   the Boost.Locale and Charconv measurements
~/.buzz/OUTBOX/BOOST_URL_RELATIVE_REFERENCE_ISSUE.md
~/.buzz/OUTBOX/SRELL_NO_EXCEPTIONS_PATCH.md, SRELL_NO_EXCEPTIONS_PATCH.diff
~/.buzz/OUTBOX/SRELL_MEMORY_RESOURCE_PATCH.md, SRELL_MEMORY_RESOURCE_PATCH.diff
```

---

### Task 0: The worktree and its local clones (controller)

**Files:**
- Create: `~/dev/webcpp-infra` (worktree), `~/dev/webcpp-infra/.local/`
  (a copy of `user-config.jam`, links to `~/dev/webcpp/.local/{wasi-sdk,wit-bindgen,wasi-wit,mrdocs,emsdk,emscripten}`),
  `~/.buzz/REPOS/JSON-Schema-Test-Suite`
- Check: `~/.buzz/RESEARCH/SRELL_ALLOCATION_SPIKE/spike/` (the SRELL spike's
  files, copied there on 2026-10-09 from a job directory that may be purged)

**Interfaces:**
- Produces: the worktree on branch `infra/shared` at `main`'s commit, its
  libraries checked out at the commits `main` records, and a local clone of
  the suite holding `5b0ee1613e45fcc2bddac00e07c19cd49b00d8a8`.

- [ ] **Step 1: Make the worktree.**

```bash
git -C ~/dev/webcpp worktree add ~/dev/webcpp-infra -b infra/shared main
git -C ~/dev/webcpp-infra submodule update --init libs/xactor libs/xstate libs/pratt libs/wasi
```

Expected: `git -C ~/dev/webcpp-infra status --short` prints nothing.

- [ ] **Step 2: Its `.local/`.** The copied `user-config.jam` names
  `~/dev/webcpp/.local` by absolute path, so the toolsets are
  those of the main checkout; the links give the harness and the doc build
  what they look for under the root.

```bash
mkdir ~/dev/webcpp-infra/.local
cp ~/dev/webcpp/.local/user-config.jam ~/dev/webcpp-infra/.local/user-config.jam
for name in wasi-sdk wit-bindgen wasi-wit mrdocs emsdk emscripten; do
    ln -s ~/dev/webcpp/.local/$name ~/dev/webcpp-infra/.local/$name
done
```

Expected: `ls -l ~/dev/webcpp-infra/.local` shows the file and six links.

- [ ] **Step 3: Clone the suite** (the one read of GitHub, the controller's).

```bash
git clone https://github.com/json-schema-org/JSON-Schema-Test-Suite.git ~/.buzz/REPOS/JSON-Schema-Test-Suite
git -C ~/.buzz/REPOS/JSON-Schema-Test-Suite rev-parse '5b0ee16^{commit}'
```

Expected: `5b0ee1613e45fcc2bddac00e07c19cd49b00d8a8`.

- [ ] **Step 4: The spike's files are kept.**
  `ls ~/.buzz/RESEARCH/SRELL_ALLOCATION_SPIKE/spike ~/.buzz/RESEARCH/SRELL_ALLOCATION_SPIKE/spike/conftest`.
  Expected: `checking_resource.hpp`, `conf_orig_utf16.txt`, `conf_orig_utf32.txt`,
  `conf_orig_utf8.txt`, `conf_orig_utf8c.txt`, `conftest`, `count.cpp`,
  `edits.py`, `make_patch.py`, `out_orig.txt`, `run.sh`,
  `srell-memory-resource.diff`; and `conftest-data.h`, `conftest.cpp`.

- [ ] **Step 5: The baseline.** Run the green of a superproject change,
  items 1 and 5, in `~/dev/webcpp-infra` with
  `export EM_CACHE=~/dev/webcpp/.local/emscripten-cache`. Expected: every
  tools test `ok`, `lint: clean`. A failure here is recorded in the ledger
  with the commit it was seen at, and fixed or reported before Task 1.

---

### Task 1: `third_party/` and the JSON Schema Test Suite

**Files:**
- Create: `third_party/Jamfile`, `tools/test/third_party_test.py`
- Create (submodule): `third_party/libs/JSON-Schema-Test-Suite`, and `.gitmodules` gains it
- Modify: `Jamroot` (after `use-project /webcpp/doc-tools : tools/doc ;`),
  `tools/lint/lint.sh` (its header comment and the loop that lists files),
  `tools/lint/lint_test.py` (one case), `tools/test/harness.py` (`copy_tree`,
  a helper), `tools/test/harness_test.py` (one case), `AGENTS.md`, `README.md`

**Interfaces:**
- Consumes: nothing of this plan.
- Produces:
  - the project `/webcpp/third_party` (`third_party/Jamfile`), registered by
    the Jamroot;
  - the explicit target `/webcpp/third_party//json-schema-test-suite`, an
    alias whose usage requirements are
    `<define>"WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE=\"<suite directory, forward slashes>\""`
    and `<dependency>` on every `*.json` of the suite's `tests`, `remotes`,
    `output-tests` and `annotations`; a test names it with
    `<library>/webcpp/third_party//json-schema-test-suite`;
  - the path constant `JSON_SCHEMA_TEST_SUITE` in `third_party/Jamfile`
    (Task 4 passes it to `webcpp.data`); it reaches no library, since a
    constant passes only to child projects: a library uses the target and its
    define, as section "Consumer interface" writes them;
  - the lint never reads a file under `third_party/libs/`;
  - `harness.copy_tree` links the checkout's `third_party/libs` into a scratch
    superproject rather than copying it, and
    `harness.third_party_of_its_own(root: Path, name: str, checked_out: bool = True) -> Path`
    gives a case its own copy of one submodule (or an empty directory), so that
    no case writes through the link into the checkout.

- [ ] **Step 1: Write the failing tests.** Create `tools/test/third_party_test.py`:

```python
#!/usr/bin/env python3
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Checks third_party/: the JSON Schema Test Suite, pinned, reaches a test that names
/webcpp/third_party//json-schema-test-suite at the directory its define gives, natively and on
emscripten; a changed file of the suite runs such a test again; and a suite that is not checked out
stops a build that needs it, naming the command that checks it out, and no other build. Each case
builds a scratch superproject with the fixture library demo, whose path holds a space. Run with the
names of some cases to run only those."""

from __future__ import annotations

import os
import re
import subprocess
import sys
import time
from pathlib import Path

import harness

EMSCRIPTEN = ('toolset=emscripten',)

PASSED = re.compile(r'^\*\*passed\*\* (.*)$', re.MULTILINE)

SUITE = 'third_party/libs/JSON-Schema-Test-Suite'

# The commit the suite is pinned at, of 2026-09-21.
SUITE_COMMIT = '5b0ee1613e45fcc2bddac00e07c19cd49b00d8a8'

READER_JAMFILE = ('import webcpp ;\n'
                  '\n'
                  'webcpp.targets native wasip2 wasip3 emscripten ;\n'
                  '\n'
                  'webcpp.run reads_suite : reads_suite.cpp\n'
                  '  : <library>/webcpp/third_party//json-schema-test-suite ;\n')

READER_SOURCE = r'''#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

int main() {
    std::ifstream file(WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE "/tests/draft2020-12/type.json");
    std::string const text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
    if (text.empty() || text.front() != '[') {
        std::cout << "cannot read " WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE "\n";
        return 1;
    }
    std::cout << "read the suite\n";
    return 0;
}
'''


def passed(result: subprocess.CompletedProcess) -> set[str]:
    """The tests b2 reports as passed, by name: reads_suite for .../reads_suite.test."""
    return {path.rsplit('/', 1)[-1].removesuffix('.test') for path in PASSED.findall(result.stdout)}


def output_of(root: Path, name: str) -> str:
    """The text of the one file named name that b2 wrote under the scratch superproject's bin/."""
    found = sorted((root / 'bin').rglob(name))
    assert len(found) == 1, (name, found)
    return found[0].read_text()


def add_reader(root: Path) -> None:
    """Adds the library suite_reader, whose one test reads a file of the suite."""
    harness.add_library(root, 'suite_reader', READER_JAMFILE, {'reads_suite.cpp': READER_SOURCE})


def test_the_suite_is_pinned(_: Path) -> None:
    listed = subprocess.run(['git', '-C', str(harness.ROOT), 'ls-files', '-s', SUITE],
                            capture_output=True, text=True, check=True)
    assert listed.stdout.split()[:2] == ['160000', SUITE_COMMIT], listed.stdout


def test_the_suite_reaches_a_native_test(root: Path) -> None:
    add_reader(root)
    result = harness.run_b2(root, '-a', 'libs/suite_reader/test')
    harness.expect(result, True)
    assert passed(result) == {'reads_suite'}, (passed(result), result.stdout[-4000:])
    assert output_of(root, 'reads_suite.output').startswith('read the suite\n')


def test_the_suite_reaches_a_test_on_emscripten(root: Path) -> None:
    harness.link_emsdk(root)
    add_reader(root)
    result = harness.run_b2(root, '-a', *EMSCRIPTEN, 'libs/suite_reader/test')
    harness.expect(result, True)
    assert passed(result) == {'reads_suite'}, (passed(result), result.stdout[-4000:])
    assert output_of(root, 'reads_suite.output').startswith('read the suite\n')


def test_a_changed_suite_file_runs_the_test_again(root: Path) -> None:
    # The suite of the case's own: a date set through the link would be the checkout's.
    harness.third_party_of_its_own(root, 'JSON-Schema-Test-Suite')
    add_reader(root)
    harness.expect(harness.run_b2(root, 'libs/suite_reader/test'), True)
    again = harness.run_b2(root, 'libs/suite_reader/test')
    harness.expect(again, True)
    assert passed(again) == set(), again.stdout[-4000:]
    later = time.time() + 60
    os.utime(root / SUITE / 'tests/draft2020-12/type.json', (later, later))
    result = harness.run_b2(root, 'libs/suite_reader/test')
    harness.expect(result, True)
    assert passed(result) == {'reads_suite'}, (passed(result), result.stdout[-4000:])


def test_a_suite_not_checked_out_is_refused(root: Path) -> None:
    harness.third_party_of_its_own(root, 'JSON-Schema-Test-Suite', checked_out=False)
    add_reader(root)
    result = harness.run_b2(root, 'libs/suite_reader/test')
    harness.expect(result, False, 'third_party/libs/JSON-Schema-Test-Suite is not checked out',
                   'git submodule update --init third_party/libs/JSON-Schema-Test-Suite')
    # A build that needs no suite never looks for it.
    harness.expect(harness.run_b2(root, 'libs/demo/test//pass'), True)


CASES = [
    test_the_suite_is_pinned,
    test_the_suite_reaches_a_native_test,
    test_the_suite_reaches_a_test_on_emscripten,
    test_a_changed_suite_file_runs_the_test_again,
    test_a_suite_not_checked_out_is_refused,
]


if __name__ == '__main__':
    sys.exit(harness.run_cases('third_party_test', CASES, sys.argv[1:]))
```

And in `tools/lint/lint_test.py`, before `CASES`, add the case, and add
`test_third_party_code_is_never_read` to `CASES` after `test_banned_word`:

```python
def test_third_party_code_is_never_read(root):
    # Third-party code keeps its own style, licence and notices: a file under third_party/libs/
    # that breaks the lint's rules, and the suite's own files, plain files here rather than a
    # submodule, leave the lint clean.
    prepare(root)
    harness.third_party_of_its_own(root, 'JSON-Schema-Test-Suite')
    planted = root / 'third_party/libs/planted'
    planted.mkdir(parents=True)
    (planted / 'planted.cpp').write_text('int  main( ) {throw 1;}\n')
    (planted / 'planted.py').write_text('x=1\n' + '#' * 120 + '\n')
    (planted / 'notes.txt').write_text(f'{EM_DASH} {WORD}\n')
    commit(root, 'Third-party code')
    expect_clean(lint(root))
```

  In `tools/test/harness.py`, a scratch copy links `third_party/libs` (about
  4 MB of suite, and SRELL later), and a case that changes one submodule gets
  its own. In `copy_tree`, the function `left_out` gains, as its first lines,
  before `if Path(directory) != top:`:

```python
        if Path(directory) == top / 'third_party':
            return built(directory, names) | ({'libs'} & set(names))
```

  and, after the copy of `user-config.jam`:

```python
    libs = top / 'third_party/libs'
    if libs.is_dir():
        (scratch / 'third_party/libs').symlink_to(libs.resolve())
```

  its docstring ending "... without copying the toolchains themselves; and
  third_party/libs is a link to src's, which a case makes its own with
  third_party_of_its_own before it changes anything there." After
  `link_emsdk`:

```python
def third_party_of_its_own(root: Path, name: str, checked_out: bool = True) -> Path:
    """Gives the scratch superproject root a third_party/libs/<name> of its own, which a case may
    change: the link to the checkout's third_party/libs becomes a directory of links, one per
    entry, and <name> a copy of the checkout's, or, unless checked_out, an empty directory, as a
    clone without that submodule has it. Returns it."""
    libs = root / 'third_party/libs'
    if libs.is_symlink():
        source = libs.resolve()
        libs.unlink()
        libs.mkdir()
        for entry in source.iterdir():
            (libs / entry.name).symlink_to(entry)
    own = libs / name
    source = own.resolve()
    own.unlink()
    if checked_out:
        shutil.copytree(source, own, ignore=built)
    else:
        own.mkdir()
    return own
```

  In `tools/test/harness_test.py`, a case added to `CASES`, and the docstring
  gains "; a scratch superproject links the checkout's third-party code, and
  a case that changes it gets its own":

```python
def test_third_party_is_linked_and_made_a_case_s_own(root):
    libs = root / 'third_party/libs'
    checkout = (harness.ROOT / 'third_party/libs').resolve()
    assert libs.is_symlink() and libs.resolve() == checkout, libs
    own = harness.third_party_of_its_own(root, 'JSON-Schema-Test-Suite', checked_out=False)
    assert not libs.is_symlink() and own.is_dir() and not own.is_symlink(), libs
    assert not any(own.iterdir()), own
    # The checkout's own suite is untouched.
    assert (checkout / 'JSON-Schema-Test-Suite/tests/draft2020-12/type.json').is_file()
```

- [ ] **Step 2: Run them, to see them fail.**

Run: `python3 tools/test/third_party_test.py`
Expected: FAIL at `test_the_suite_is_pinned` (`assert [] == ['160000', '5b0ee16...']`);
then `python3 tools/test/third_party_test.py test_the_suite_reaches_a_native_test`
fails with `error: Unable to find file or target named` and
`'/webcpp/third_party//json-schema-test-suite'`.

Run: `python3 tools/lint/lint_test.py test_third_party_code_is_never_read`
Expected: FAIL, the lint naming `third_party/libs/planted/planted.cpp`
(clang-format, licence notice) and the em dash.

Run: `python3 tools/test/harness_test.py test_third_party_is_linked_and_made_a_case_s_own`
Expected: FAIL, `AttributeError: module 'harness' has no attribute 'third_party_of_its_own'`
(after Step 3 adds the submodule; before it, the link's assertion fails).

- [ ] **Step 3: Add the submodule** from the local clone, at its pin, with
  upstream's URL, so that nothing is fetched from GitHub.

```bash
git clone --no-checkout ~/.buzz/REPOS/JSON-Schema-Test-Suite third_party/libs/JSON-Schema-Test-Suite
git -C third_party/libs/JSON-Schema-Test-Suite checkout --detach 5b0ee1613e45fcc2bddac00e07c19cd49b00d8a8
git -C third_party/libs/JSON-Schema-Test-Suite remote set-url origin https://github.com/json-schema-org/JSON-Schema-Test-Suite.git
git submodule add https://github.com/json-schema-org/JSON-Schema-Test-Suite.git third_party/libs/JSON-Schema-Test-Suite
git submodule absorbgitdirs third_party/libs/JSON-Schema-Test-Suite
git ls-files -s third_party/libs/JSON-Schema-Test-Suite
```

Expected: `Adding existing repo at 'third_party/libs/JSON-Schema-Test-Suite' to the index`,
then `160000 5b0ee1613e45fcc2bddac00e07c19cd49b00d8a8 0	third_party/libs/JSON-Schema-Test-Suite`,
and `.gitmodules` holds:

```
[submodule "third_party/libs/JSON-Schema-Test-Suite"]
	path = third_party/libs/JSON-Schema-Test-Suite
	url = https://github.com/json-schema-org/JSON-Schema-Test-Suite.git
```

- [ ] **Step 4: Write `third_party/Jamfile`.**

```
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)
#
# The third-party code that webcpp's libraries and their tests use, each a
# submodule of its own under libs/, pinned at a commit and keeping its own
# licence, declared here as /webcpp/third_party//<target>, whose usage
# requirements give a program what it needs. A target whose submodule is not
# checked out stops the build that needs it, naming the command that checks
# it out; a build that needs none never looks.
import errors ;
import path ;
import regex ;

project /webcpp/third_party ;

# The JSON Schema Test Suite (MIT), json-schema-org/JSON-Schema-Test-Suite:
# test data, never compiled into a program. A test that names
# json-schema-test-suite reads it at run time from the directory
# WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE names, with forward slashes, the whole
# define quoted for a checkout whose path holds a space, and runs again when
# one of its files changes.
path-constant JSON_SCHEMA_TEST_SUITE : libs/JSON-Schema-Test-Suite ;
.suite = [ regex.replace $(JSON_SCHEMA_TEST_SUITE) "\\\\" "/" ] ;
.suite-files = [ glob-tree-ex libs/JSON-Schema-Test-Suite/tests
    libs/JSON-Schema-Test-Suite/remotes libs/JSON-Schema-Test-Suite/output-tests
    libs/JSON-Schema-Test-Suite/annotations : *.json ] ;

rule suite-checked-out ( properties * )
{
    if ! [ path.exists $(JSON_SCHEMA_TEST_SUITE)/tests/draft2020-12 ]
    {
        errors.user-error "third_party/libs/JSON-Schema-Test-Suite is not checked out: it"
            "holds no tests/draft2020-12. Check it out with `git submodule update --init"
            "third_party/libs/JSON-Schema-Test-Suite`." ;
    }
}

alias json-schema-test-suite : : <conditional>@suite-checked-out : :
    <define>"\"WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE=\\\"$(.suite)\\\"\""
    <dependency>$(.suite-files) ;

explicit json-schema-test-suite ;
```

In `Jamroot`, after `use-project /webcpp/doc-tools : tools/doc ;`:

```
# The third-party code the libraries and their tests use, each a submodule
# under third_party/libs/ (third_party/Jamfile).
use-project /webcpp/third_party : third_party ;
```

- [ ] **Step 5: The lint never reads third-party code.** In
  `tools/lint/lint.sh`, after the header sentence that ends "so that a new
  file is linted before it is added.", add the line:

```bash
# Third-party code under third_party/libs/ is never read: it keeps its own style, licence and
# notices.
```

  and in the loop that lists the files,
  replace

```bash
              if [ -f "${path}" ] && [ ! -L "${path}" ]; then
```

  with

```bash
              case "${path}" in
                  third_party/libs/*) continue ;;
              esac
              if [ -f "${path}" ] && [ ! -L "${path}" ]; then
```

- [ ] **Step 6: Run the tests, to see them pass.**

Run: `python3 tools/test/third_party_test.py`
Expected: five `<case>: ok` lines, then `third_party_test: ok`.

Run: `python3 tools/lint/lint_test.py test_third_party_code_is_never_read test_clean_tree_passes`
Expected: both `ok`, then `lint_test: ok`.

Run: `python3 tools/test/harness_test.py`
Expected: every case `ok`, then `harness_test: ok`.

- [ ] **Step 7: The texts.**
  - `AGENTS.md` chapter 1, "The layout", after the `libs/<name>/` entry:

```
  third_party/        third-party code the libraries and their tests use, not webcpp's: each
                      a submodule under libs/, pinned at a commit and keeping its own licence,
                      and the Jamfile that declares it as /webcpp/third_party//<target>
                      (chapter 2)
```

  - `AGENTS.md` chapter 2, a subsection after the checklist, before "## 3. Names":

```
### Third-party code

What a library or its tests use and webcpp does not own is never copied into
a library: it is a submodule under `third_party/libs/<name>`, pinned at a
commit, with the URL of its own repository, and `third_party/Jamfile`
declares it as `/webcpp/third_party//<target>`, whose usage requirements
give a program what it needs. It keeps its own licence and notices, and the
lint never reads it (chapter 6). Adding one is the owner's decision; moving
its pin is a change of its own, with its evidence. A target whose submodule
is not checked out stops the build that needs it, naming `git submodule
update --init third_party/libs/<name>`; a build that needs none never looks.

| Target | What it is | Licence |
| --- | --- | --- |
| `json-schema-test-suite` | the JSON Schema Test Suite at `5b0ee16` (2026-09-21), test data: a test that names it reads the suite at run time from the directory its define `WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE` gives, and runs again when a file of the suite changes | MIT |
```

  - `AGENTS.md` chapter 6, "What the lint enforces": its first sentence,
    "`tools/lint/lint.sh` lints the superproject and every library of `libs/`
    that is a repository: what git tracks and what it would track (untracked,
    not ignored), so a new file is linted before it is added.", becomes
    "`tools/lint/lint.sh` lints the superproject and every library of `libs/`
    that is a repository: what git tracks and what it would track (untracked,
    not ignored), so a new file is linted before it is added, but nothing
    under `third_party/libs/`, third-party code that keeps its own style and
    licence (chapter 2)."
  - `AGENTS.md` chapter 10, the list of the tests of the build and of the
    tools: add `tools/test/third_party_test.py` after
    `tools/test/harness_test.py`.
  - `README.md`, "The tests of the tools": "for the Jamroot, `tools/webcpp.jam`,
    third-party code, the oracle's rules, ..." ; "License": add the sentence
    "Third-party code under `third_party/libs/` keeps its own licence: the
    JSON Schema Test Suite is MIT."

- [ ] **Step 8: Verify.** The green of a superproject change, items 1 to 6
  (Global Constraints). Expected: as stated there.

- [ ] **Step 9: Commit.**

```bash
git add -- third_party/Jamfile tools/test/third_party_test.py
git -c user.name=R -c user.email=github.overboard312@passmail.net commit \
  -m "third_party: the JSON Schema Test Suite, pinned as a submodule, for the tests that read it" \
  -- .gitmodules third_party/Jamfile third_party/libs/JSON-Schema-Test-Suite Jamroot \
     tools/lint/lint.sh tools/lint/lint_test.py tools/test/third_party_test.py \
     tools/test/harness.py tools/test/harness_test.py AGENTS.md README.md
```

**Changelog** (the report's last section), Tools and build:
- Added `third_party/`, third-party code pinned as submodules: the JSON
  Schema Test Suite at `5b0ee16`, which a test reads through
  `/webcpp/third_party//json-schema-test-suite`.
- Changed the lint to never read third-party code under `third_party/libs/`.

---

### Task 2: `webcpp.generated`, the check of generated files

**Files:**
- Create: `tools/generated/generated.jam`, `tools/generated/generated.py`,
  `tools/test/generated_jam_test.py`
- Modify: `tools/webcpp.jam` (its header comment, the module loads after
  `import webcpp-drive ;`, a rule at its end), `tools/oracle/compare.py`,
  `tools/oracle/compare_test.py`, `AGENTS.md`, `README.md`

**Interfaces:**
- Consumes: `tools/oracle/compare.py`; `webcpp.lane` (an own lane that names
  no target).
- Produces:
  - `webcpp.generated <name> : <script> : <argument> * : <output> + ;` in a
    library's test Jamfile: the explicit target `generated-<name>` and, once
    per Jamfile, the explicit target `update-generated`;
  - the generator contract: `<script>` is a `.py` file relative to the
    Jamfile, run with the build's Python in the Jamfile's directory as
    `<script> <argument>... <path-1> ... <path-n>`, writing its i-th output at
    `<path-i>`; outputs are relative to the Jamfile;
  - `python3 tools/oracle/compare.py --files LEFT RIGHT`: exit 0 when both are
    files of the same bytes, 1 naming `compare: <path>: <reason>` otherwise
    (`does not exist`, `contents differ`, `a file here, a directory in ...`,
    `cannot be read ...`), 2 on a usage error;
  - its one consumer is idna (json_schema's generators write fixed paths, and
    its checks are C++ tests); idna's use, from its spec section 8.1, to be
    cross-checked against the IDNA plan when it exists:
    `webcpp.generated tables : ../tools/generate_tables.py : --cache <dir> : ../include/webcpp/idna/detail/tables.hpp generated/idna_test_v2.ipp generated/nfc_test.ipp ;`
    and `webcpp.lane tables : generated-tables ;`.

- [ ] **Step 1: Write the failing tests.** Create `tools/test/generated_jam_test.py`:

```python
#!/usr/bin/env python3
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Checks webcpp.generated (tools/generated/generated.jam): generated-<name> runs a library's Python
generator in its Jamfile's directory into the build directory, and passes when every output equals
its committed file, byte for byte, and fails naming each one that differs, is missing, or that the
generator did not write; update-generated writes them in place, and leaves every committed file as
it was when a generator fails; an own lane names the check, which `b2 declared-lanes` lists and the
ordinary lanes never build; and a wrong declaration stops the build by name. Each case builds a
scratch superproject with the fixture library demo, whose path holds a space. Run with the names of
some cases to run only those."""

from __future__ import annotations

import shutil
import sys
from pathlib import Path

import harness

LIBRARY = 'libs/gen'
CHECK = f'{LIBRARY}/test//generated-words'
UPDATE = f'{LIBRARY}/test//update-generated'
LANE = f'{LIBRARY}/test//words'

JAMFILE = ('import webcpp ;\n'
           '\n'
           'webcpp.generated words : gen.py : forty-two\n'
           '  : generated/words.txt generated/more/twice.txt ;\n'
           'webcpp.lane words : generated-words ;\n')

# The generator: from its Jamfile's directory, where it finds itself, it writes its word into the
# first output and twice into the second; fail makes it exit 1, fail-late exit 1 once it has
# written its first output, and skip-second leaves the second output unwritten.
GENERATOR = '''import sys
from pathlib import Path

if not Path('gen.py').is_file():
    sys.exit(3)
word, *outputs = sys.argv[1:]
if word == 'fail':
    sys.exit(1)
texts = [word + '\\n', word + ' ' + word + '\\n']
if word == 'skip-second':
    texts = texts[:1]
for output, text in zip(outputs, texts):
    Path(output).write_text(text)
    if word == 'fail-late':
        sys.exit(1)
'''

WORDS = 'forty-two\n'
TWICE = 'forty-two forty-two\n'


def add_generator(root: Path, jamfile: str = JAMFILE) -> Path:
    """Adds the library gen, whose test Jamfile declares the generator words, with what it writes
    committed; returns its test directory."""
    test = root / LIBRARY / 'test'
    (test / 'generated/more').mkdir(parents=True)
    (root / LIBRARY / 'build.jam').write_text('project /webcpp/gen ;\n')
    (test / 'Jamfile').write_text(jamfile)
    (test / 'gen.py').write_text(GENERATOR)
    (test / 'generated/words.txt').write_text(WORDS)
    (test / 'generated/more/twice.txt').write_text(TWICE)
    return test


def test_matching_outputs_pass(root: Path) -> None:
    add_generator(root)
    harness.expect(harness.run_b2(root, '-a', CHECK), True)
    harness.expect(harness.run_b2(root, '-a', LANE), True)


def test_each_differing_output_fails_by_name(root: Path) -> None:
    test = add_generator(root)
    (test / 'generated/words.txt').write_text('forty-three\n')
    (test / 'generated/more/twice.txt').write_text('forty-two  forty-two\n')
    harness.expect(harness.run_b2(root, '-a', CHECK), False,
                   'generated/words.txt: contents differ',
                   'generated/more/twice.txt: contents differ',
                   'update-generated writes it again')


def test_a_missing_committed_output_fails(root: Path) -> None:
    test = add_generator(root)
    (test / 'generated/more/twice.txt').unlink()
    harness.expect(harness.run_b2(root, '-a', CHECK), False,
                   'generated/more/twice.txt: does not exist')


def test_an_output_the_generator_does_not_write_fails(root: Path) -> None:
    add_generator(root, JAMFILE.replace('forty-two', 'skip-second'))
    harness.expect(harness.run_b2(root, '-a', CHECK), False, 'words/2/twice.txt: does not exist')


def test_update_generated_writes_them_in_place(root: Path) -> None:
    test = add_generator(root)
    (test / 'generated/words.txt').write_text('stale\n')
    shutil.rmtree(test / 'generated/more')
    harness.expect(harness.run_b2(root, '-a', UPDATE), True, 'generated: words: wrote')
    assert (test / 'generated/words.txt').read_text() == WORDS
    assert (test / 'generated/more/twice.txt').read_text() == TWICE
    harness.expect(harness.run_b2(root, '-a', CHECK), True)


def test_a_failing_generator_leaves_every_committed_file(root: Path) -> None:
    test = add_generator(root, JAMFILE.replace('forty-two', 'fail'))
    harness.expect(harness.run_b2(root, '-a', UPDATE), False, 'gen.py exited with status 1')
    assert (test / 'generated/words.txt').read_text() == WORDS
    assert (test / 'generated/more/twice.txt').read_text() == TWICE
    harness.expect(harness.run_b2(root, '-a', CHECK), False, 'gen.py exited with status 1')


def test_a_generator_failing_after_one_output_leaves_every_committed_file(root: Path) -> None:
    test = add_generator(root, JAMFILE.replace('forty-two', 'fail-late'))
    harness.expect(harness.run_b2(root, '-a', UPDATE), False, 'gen.py exited with status 1')
    assert (test / 'generated/words.txt').read_text() == WORDS
    assert (test / 'generated/more/twice.txt').read_text() == TWICE
    assert not list(test.rglob('*.generated')), sorted(test.rglob('*'))


def test_the_lane_is_listed_and_the_ordinary_lanes_skip_it(root: Path) -> None:
    test = add_generator(root)
    (test / 'generated/words.txt').write_text('stale\n')
    result = harness.run_b2(root, '-d0', 'declared-lanes')
    harness.expect(result, True)
    assert 'gen words libs/gen/test\n' in result.stdout, result.stdout
    # The library's ordinary lane builds nothing of it: it never runs a generator, which may need
    # the network, and so passes over the stale file.
    result = harness.run_b2(root, '-a', 'libs/gen/test')
    harness.expect(result, True)
    assert 'webcpp-generated' not in result.stdout, result.stdout[-4000:]


def test_a_wrong_declaration_is_refused(root: Path) -> None:
    test = add_generator(root)
    for jamfile, message in (
            (JAMFILE.replace('gen.py', 'gen.sh'), 'gen.sh is not a Python file'),
            (JAMFILE.replace('gen.py', 'missing.py'), 'libs/gen/test/missing.py does not exist'),
            (JAMFILE + 'webcpp.generated words : gen.py : x : generated/other.txt ;\n',
             'webcpp.generated words is declared twice in libs/gen/test/Jamfile'),
            (JAMFILE.replace('generated/more/twice.txt', 'generated/words.txt'),
             'generated/words.txt twice')):
        (test / 'Jamfile').write_text(jamfile)
        harness.expect(harness.run_b2(root, CHECK), False, message)


CASES = [
    test_matching_outputs_pass,
    test_each_differing_output_fails_by_name,
    test_a_missing_committed_output_fails,
    test_an_output_the_generator_does_not_write_fails,
    test_update_generated_writes_them_in_place,
    test_a_failing_generator_leaves_every_committed_file,
    test_a_generator_failing_after_one_output_leaves_every_committed_file,
    test_the_lane_is_listed_and_the_ordinary_lanes_skip_it,
    test_a_wrong_declaration_is_refused,
]


if __name__ == '__main__':
    sys.exit(harness.run_cases('generated_jam_test', CASES, sys.argv[1:]))
```

In `tools/oracle/compare_test.py`, add these cases before `CASES` and to
`CASES` before `test_a_usage_error_exits_2`, and change the usage case:

```python
def test_identical_files_pass(root: Path) -> None:
    write(root / 'left.txt', b'the same\n')
    write(root / 'right.txt', b'the same\n')
    result = compare('--files', root / 'left.txt', root / 'right.txt')
    expect(result, 0)
    assert result.stdout == '' and result.stderr == '', (result.stdout, result.stderr)


def test_differing_files_fail(root: Path) -> None:
    write(root / 'left.txt', b'the same\n')
    write(root / 'right.txt', b'the sane\n')
    expect(compare('--files', root / 'left.txt', root / 'right.txt'), 1,
           f'compare: {root / "left.txt"}: contents differ')


def test_a_missing_file_fails(root: Path) -> None:
    write(root / 'left.txt', b'here\n')
    expect(compare('--files', root / 'left.txt', root / 'missing.txt'), 1,
           f'compare: {root / "missing.txt"}: does not exist')
    expect(compare('--files', root / 'gone.txt', root / 'left.txt'), 1,
           f'compare: {root / "gone.txt"}: does not exist')


def test_a_directory_is_not_a_file(root: Path) -> None:
    write(root / 'left.txt', b'here\n')
    (root / 'directory').mkdir()
    expect(compare('--files', root / 'left.txt', root / 'directory'), 1,
           'a file here, a directory in')


def test_a_usage_error_exits_2(root: Path) -> None:
    left, _ = trees(root)
    expect(compare(left), 2, 'usage: compare.py [--files] LEFT RIGHT')
    expect(compare(), 2, 'usage: compare.py [--files] LEFT RIGHT')
    expect(compare('--files', left), 2, 'usage: compare.py [--files] LEFT RIGHT')
```

  and its docstring gains: "with --files, two identical files pass, and a
  differing byte, a missing file and a directory each fail, naming the path;".

- [ ] **Step 2: Run them, to see them fail.**

Run: `python3 tools/oracle/compare_test.py`
Expected: FAIL at `test_identical_files_pass` (status 2, `usage: compare.py LEFT RIGHT`).

Run: `python3 tools/test/generated_jam_test.py test_matching_outputs_pass`
Expected: FAIL; b2 prints `rule generated unknown in module webcpp` (or b2's
wording for an unknown rule) from `libs/gen/test/Jamfile`.

- [ ] **Step 3: `compare.py --files`.** In `tools/oracle/compare.py`, the
  docstring's usage becomes

```
Usage: compare.py [--files] LEFT RIGHT

With --files, LEFT and RIGHT are two files, compared by their bytes; a path that does not exist,
or is not a file, is a difference.
```

  add, after `differences`:

```python
def file_differences(left: str, right: str) -> list[tuple[str, str]]:
    """Every (path, reason) by which the file right differs from the file left: none when both
    are files of the same bytes."""
    found = [(path, 'does not exist') for path in (left, right) if not os.path.lexists(path)]
    if found:
        return found
    left_kind = kind(left)
    right_kind = kind(right)
    if left_kind != 'file' or right_kind != 'file':
        return [(left, f'a {left_kind} here, a {right_kind} in {right}')]
    left_bytes = read(left)
    right_bytes = read(right)
    if left_bytes is None or right_bytes is None:
        return [(left, 'cannot be read here or in ' + right)]
    return [] if left_bytes == right_bytes else [(left, 'contents differ')]
```

  and `main` becomes:

```python
def main(argv: list[str]) -> int:
    files = argv[:1] == ['--files']
    if files:
        argv = argv[1:]
    if len(argv) != 2:
        print('usage: compare.py [--files] LEFT RIGHT', file=sys.stderr)
        return 2
    compared = file_differences if files else differences
    different = compared(argv[0], argv[1])
    for path, reason in different:
        print(f'compare: {path}: {reason}', file=sys.stderr)
    return 1 if different else 0
```

Run: `python3 tools/oracle/compare_test.py`
Expected: every case `ok`, then `compare_test: ok`.

- [ ] **Step 4: The driver.** Create `tools/generated/generated.py`:

```python
#!/usr/bin/env python3
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Runs the generators of webcpp.generated, and checks or writes what they write.

Usage: generated.py check|update BUILD STEP...

A STEP is the words NAME DIRECTORY SCRIPT N ARGUMENT... M OUTPUT...: the Python script SCRIPT, run
in DIRECTORY as `SCRIPT ARGUMENT... PATH...`, writes each of the M committed files OUTPUT at the
PATH given in its place, BUILD/NAME/<position>/<file name>.

check runs each step, then compares each output with its committed file, byte for byte, with
tools/oracle/compare.py --files, which names every difference; it exits 1 when a generator fails
or an output differs. update runs every step and, only once every generator has succeeded and
written each of its outputs, copies them beside the committed files, then renames each over its
committed file, so that a generator that fails, or a copy that fails, leaves every committed file
as it was. Exit 0 when all is well, 1 on a failure, 2 on a usage error.
"""

from __future__ import annotations

import os
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

COMPARE = Path(__file__).resolve().parents[1] / 'oracle/compare.py'


@dataclass
class Step:
    """One generator: what it is named, where it runs, and what it writes."""
    name: str
    directory: Path
    script: Path
    arguments: list[str]
    outputs: list[Path]


class Usage(Exception):
    """The words given do not make steps."""


def count(words: list[str], at: int) -> int:
    """The count at words[at], which the words after it must hold."""
    try:
        value = int(words[at])
    except (IndexError, ValueError):
        raise Usage(f'a count was expected at word {at + 1}') from None
    if value < 0 or at + 1 + value > len(words):
        raise Usage(f'the count {value} at word {at + 1} runs past the last word')
    return value


def parse(words: list[str]) -> list[Step]:
    """The steps the words give, in order."""
    steps = []
    at = 0
    while at < len(words):
        if at + 3 > len(words):
            raise Usage('a step needs a name, a directory and a script')
        name, directory, script = words[at:at + 3]
        at += 3
        arguments_count = count(words, at)
        arguments = words[at + 1:at + 1 + arguments_count]
        at += 1 + arguments_count
        outputs_count = count(words, at)
        if outputs_count == 0:
            raise Usage(f'{name}: a generator writes at least one output')
        outputs = [Path(word) for word in words[at + 1:at + 1 + outputs_count]]
        at += 1 + outputs_count
        steps.append(Step(name, Path(directory), Path(script), arguments, outputs))
    if not steps:
        raise Usage('no step was given')
    return steps


def generate(build: Path, step: Step) -> list[Path] | None:
    """Runs step's generator into build, and returns where it was told to write each output;
    None, once it has said why, when the generator failed."""
    shutil.rmtree(build / step.name, ignore_errors=True)
    paths = [build / step.name / str(position) / output.name
             for position, output in enumerate(step.outputs, start=1)]
    for path in paths:
        path.parent.mkdir(parents=True, exist_ok=True)
    completed = subprocess.run([sys.executable, str(step.script), *step.arguments,
                                *map(str, paths)], cwd=step.directory, check=False)
    if completed.returncode != 0:
        print(f'generated: {step.name}: {step.script} exited with status '
              f'{completed.returncode}', file=sys.stderr)
        return None
    return paths


def check(build: Path, steps: list[Step]) -> int:
    failed = False
    for step in steps:
        paths = generate(build, step)
        if paths is None:
            failed = True
            continue
        for output, path in zip(step.outputs, paths):
            compared = subprocess.run([sys.executable, str(COMPARE), '--files', str(output),
                                       str(path)], check=False)
            if compared.returncode != 0:
                failed = True
                print(f'generated: {step.name}: {output} is not what {step.script.name} '
                      'writes; update-generated writes it again', file=sys.stderr)
    return 1 if failed else 0


def update(build: Path, steps: list[Step]) -> int:
    generated = []
    for step in steps:
        paths = generate(build, step)
        if paths is None:
            return 1
        missing = [path for path in paths if not path.is_file()]
        if missing:
            for path in missing:
                print(f'generated: {step.name}: {step.script.name} wrote no {path}',
                      file=sys.stderr)
            return 1
        generated.append((step, paths))
    # Every output is copied beside its committed file first, and renamed over it only once all
    # are there: os.replace is atomic, a copy is not.
    staged: list[tuple[Path, Path, str]] = []
    try:
        for step, paths in generated:
            for output, path in zip(step.outputs, paths):
                output.parent.mkdir(parents=True, exist_ok=True)
                beside = output.with_name(output.name + '.generated')
                staged.append((beside, output, step.name))
                shutil.copyfile(path, beside)
    except OSError as error:
        for beside, _, _ in staged:
            beside.unlink(missing_ok=True)
        print(f'generated: {error}', file=sys.stderr)
        return 1
    for beside, output, name in staged:
        os.replace(beside, output)
        print(f'generated: {name}: wrote {output}')
    return 0


def main(argv: list[str]) -> int:
    if len(argv) < 2 or argv[0] not in ('check', 'update'):
        print('usage: generated.py check|update BUILD STEP...', file=sys.stderr)
        return 2
    try:
        steps = parse(argv[2:])
    except Usage as error:
        print(f'generated.py: {error}', file=sys.stderr)
        return 2
    build = Path(argv[1])
    return check(build, steps) if argv[0] == 'check' else update(build, steps)


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
```

- [ ] **Step 5: The rule.** Create `tools/generated/generated.jam`:

```
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)
#
# The rule of a library's generated files, which tools/webcpp.jam offers a
# library's test Jamfile as webcpp.generated:
#
#     webcpp.generated <name> : <script> : <argument> * : <output> + ;
#
# A generated file is committed, and written only by its generator, the
# Python script <script>, which runs in the Jamfile's directory as
# `<script> <argument>... <path>...` and writes each output at the path
# given in its place. The explicit target generated-<name> has it write into
# the Jamfile's build directory and compares each output, byte for byte, with
# the committed file (tools/oracle/compare.py --files), naming every one that
# differs, is missing or was not written. The explicit target
# update-generated, declared once per Jamfile with its first
# webcpp.generated, runs every generator of the Jamfile the same way and,
# once all have succeeded, copies their outputs over the committed files:
# tools/generated/generated.py does both. Paths are relative to the Jamfile.
# A lane names generated-<name> (webcpp.lane), and the ordinary lanes never
# build it: a generator may need the network. Neither target compiles
# anything, so they leave the Jamroot's check of Boost out.
#
# This file is loaded as the module webcpp-generated. Its rule receives
# first where its Jamfile is, as tools/webcpp.jam's `where` finds it.
import errors ;
import feature ;
import modules ;
import notfile ;
import os ;
import path ;
import sequence ;

# This directory, absolute: an action starts in the directory b2 was
# started in.
.here = [ path.parent [ path.root [ path.make [ modules.binding $(__name__) ] ] [ path.pwd ] ] ] ;

if [ os.name ] = NT { .python = python ; } else { .python = python3 ; }

.driver = [ path.native $(.here)/generated.py ] ;

# The Jamfile a target belongs to, by its directory relative to the
# superproject, and the generator a target generated-<name> runs. Free, so
# that they play no part in a target's build directory.
feature.feature webcpp-generated : : free incidental ;
feature.feature webcpp-generated-name : : free incidental ;

# webcpp.generated <name> : <script> : <argument> * : <output> + ; the
# explicit targets generated-<name> and, once per Jamfile, update-generated.
rule generated ( found + : name : script : arguments * : outputs + )
{
    local key = $(found[3]) ;
    if $(name) in $(.names.$(key))
    {
        errors.user-error "webcpp.generated $(name) is declared twice in $(found[4])" ;
    }
    if $(script:S) != .py
    {
        errors.user-error "webcpp.generated $(name) in $(found[4]): $(script) is not a Python"
            "file; a generator is a Python script, run with the build's Python" ;
    }
    local file = [ path.root [ path.make $(script) ] $(found[2]) ] ;
    if ! [ path.exists $(file) ]
    {
        errors.user-error "webcpp.generated $(name): $(key)/$(script) does not exist; it is"
            "the generator that $(found[4]) names" ;
    }
    local committed ;
    for local output in $(outputs)
    {
        local native = [ path.native [ path.root [ path.make $(output) ] $(found[2]) ] ] ;
        if $(native) in $(committed)
        {
            errors.user-error "webcpp.generated $(name) in $(found[4]) names $(output) twice" ;
        }
        committed += $(native) ;
    }
    .names.$(key) += $(name) ;
    # The words of the driver's step: the name, the directory the script
    # runs in, the script, the arguments and the outputs, each list after
    # its count.
    .step.$(key).$(name) = $(name) [ path.native $(found[2]) ] [ path.native $(file) ]
        [ sequence.length $(arguments) ] $(arguments)
        [ sequence.length $(committed) ] $(committed) ;
    local build-dir = [ path.root [ $(found[1]).build-dir ] [ path.pwd ] ] ;
    .build.$(key) = [ path.native $(build-dir)/generated ] ;
    notfile generated-$(name) : @webcpp-generated%check
      : : <webcpp-generated>$(key) <webcpp-generated-name>$(name) <webcpp-boost-check>off ;
    $(found[1]).mark-target-as-explicit generated-$(name) ;
    if ! $(.update.$(key))
    {
        .update.$(key) = true ;
        notfile update-generated : @webcpp-generated%update
          : : <webcpp-generated>$(key) <webcpp-boost-check>off ;
        $(found[1]).mark-target-as-explicit update-generated ;
    }
}

rule check ( targets * : sources * : properties * )
{
    local key = [ feature.get-values <webcpp-generated> : $(properties) ] ;
    local name = [ feature.get-values <webcpp-generated-name> : $(properties) ] ;
    PYTHON on $(targets) = $(.python) ;
    DRIVER on $(targets) = $(.driver) ;
    BUILD on $(targets) = $(.build.$(key)) ;
    STEPS on $(targets) = $(.step.$(key).$(name)) ;
}

actions check
{
    "$(PYTHON)" "$(DRIVER)" check "$(BUILD)" "$(STEPS)"
}

# update-generated runs every generator its Jamfile declares by the time it
# runs.
rule update ( targets * : sources * : properties * )
{
    local key = [ feature.get-values <webcpp-generated> : $(properties) ] ;
    local steps ;
    for local name in $(.names.$(key))
    {
        steps += $(.step.$(key).$(name)) ;
    }
    PYTHON on $(targets) = $(.python) ;
    DRIVER on $(targets) = $(.driver) ;
    BUILD on $(targets) = $(.build.$(key)) ;
    STEPS on $(targets) = $(steps) ;
}

actions update
{
    "$(PYTHON)" "$(DRIVER)" update "$(BUILD)" "$(STEPS)"
}
```

In `tools/webcpp.jam`:
- the header comment, after the paragraph on `webcpp.drive`, gains:

```
# and the rule of a library's generated files, tools/generated/generated.jam's,
# which runs a Python generator and compares what it writes with the
# committed files:
#
#     webcpp.generated name : script : arguments * : outputs + ;
#
```

- after `import webcpp-drive ;`:

```
# The rule of generated files, loaded as the module webcpp-generated, as the
# documentation's are.
modules.load webcpp-generated : generated.jam
  : [ path.native [ path.join $(.tools) generated ] ] ;
import webcpp-generated ;
```

- at its end:

```
# The explicit target generated-<name>, which runs the Python generator
# script with arguments and compares what it writes with the committed
# outputs, and update-generated, their only writer
# (tools/generated/generated.jam).
rule generated ( name : script : arguments * : outputs + )
{
    webcpp-generated.generated [ where ] : $(name) : $(script) : $(arguments) : $(outputs) ;
}
```

- [ ] **Step 6: Run the tests, to see them pass.**

Run: `python3 tools/test/generated_jam_test.py`
Expected: nine `<case>: ok` lines, then `generated_jam_test: ok`.

Run: `python3 tools/test/oracle_jam_test.py`
Expected: `oracle_jam_test: ok` (compare.py's directory mode is unchanged).

- [ ] **Step 7: The texts.**
  - `AGENTS.md` chapter 1, "The layout", under `tools/`, after `drive/`:

```
    generated/        generated.jam, the rule of a library's generated files (chapter 9), and
                      generated.py, which runs a generator and checks or writes what it writes
```

  - `AGENTS.md` chapter 1, "The build commands", after the `update-expected` row:

```
| `b2 libs/<name>/test//generated-<name>` | a library's generated files checked against their generator: written into the build directory and compared, byte for byte (chapter 9) |
| `b2 libs/<name>/test//update-generated` | writes every generated file of that Jamfile again, the only writer of a generated file |
```

  - `AGENTS.md` chapter 5, the bullet "Nothing expected is written by hand",
    gains the sentence: "Nor is a generated file: only `update-generated`
    writes it (chapter 9)."
  - `AGENTS.md` chapter 9, after the "Driven tests" paragraph and its fixture
    example:

````
**Generated files** (`tools/generated/generated.jam`). A file that a script
writes from inputs it pins, such as a table of Unicode data, is committed and
written only by its generator. A library's test Jamfile declares it:

```
webcpp.generated <name> : <script> : <argument> * : <output> + ;
```

| Rule | Passes when | Notes |
| --- | --- | --- |
| `webcpp.generated` | every output that `<script>` writes equals its committed `<output>`, byte for byte | the explicit target `generated-<name>`: `tools/generated/generated.py` runs the Python script `<script>` in the Jamfile's directory as `<script> <argument>... <path>...`, each path where it writes one output, in the order given, under the Jamfile's build directory, and compares each with its committed file (`tools/oracle/compare.py --files`), naming every one that differs, is missing or was not written. The first declaration of a Jamfile also declares the explicit target `update-generated`, which runs each of its generators the same way and, once all have succeeded, copies their outputs over the committed files: a generator that fails leaves every committed file as it was. Paths are relative to the Jamfile; a script that is not a `.py` file or does not exist, a name declared twice and an output named twice stop the build. An own lane names the check, `webcpp.lane <lane> : generated-<name> ;`, which runs natively, as an oracle's does; the ordinary lanes never build it, since a generator may need the network |
````

  - `AGENTS.md` chapter 10: add `tools/test/generated_jam_test.py` to the
    list of the tests of the build and of the tools.
  - `README.md`, "The tests of the tools": "... the oracle's rules, the check
    of generated files, the rules of a WebAssembly component ...".

- [ ] **Step 8: Verify.** The green of a superproject change, items 1 to 6.
  Expected: as stated there.

- [ ] **Step 9: Commit.**

```bash
git add -- tools/generated/generated.jam tools/generated/generated.py tools/test/generated_jam_test.py
git -c user.name=R -c user.email=github.overboard312@passmail.net commit \
  -m "tools: webcpp.generated checks a library's generated files against their generator, update-generated writes them" \
  -- tools/generated/generated.jam tools/generated/generated.py tools/test/generated_jam_test.py \
     tools/webcpp.jam tools/oracle/compare.py tools/oracle/compare_test.py AGENTS.md README.md
```

**Changelog**, Tools and build:
- Added `webcpp.generated`, which checks a library's generated files against
  the Python generator that writes them, and `update-generated`, their only
  writer.
- Added `--files` to `tools/oracle/compare.py`, which compares two files byte
  for byte.

---

### Task 3: `/webcpp//boost_url`, built from Boost's sources

**Files:**
- Create: `tools/boost/Jamfile`, `tools/test/boost_libraries_test.py`
- Modify: `Jamroot` (after `path-constant WEBCPP_ROOT : . ;`, and the
  `explicit` list), `tools/ci/actions/boost/install.sh`,
  `tools/ci/actions/boost/action.yml` (its description),
  `tools/ci/actions_test.py`, `tools/test/harness.py`,
  `tools/lint/compile_commands.py` (`read` and the loop of the twins),
  `tools/lint/lint_test.py`, `AGENTS.md`, `README.md`
- Create (local, git-ignored): `.local/boost-source`

**Interfaces:**
- Consumes: the Jamroot's `boost-version ( directory )` (returns
  `<BOOST_VERSION> <name>`, such as `109200 1.92.0`), `.boost-found`,
  `.boost-include`.
- Produces:
  - `/webcpp//boost_url`: an alias over `/webcpp/boost-libraries//boost_url`,
    a static library of `libs/url/src/**/*.cpp` built once per variant on
    every target, usage requirements `<define>BOOST_URL_STATIC_LINK=1
    <define>BOOST_URL_NO_LIB=1`; a library names it in its main target's
    usage requirements, `<library>/webcpp//boost_url` (formats' `build.jam`,
    json_schema's in place of its spec's `<library>/boost//url`);
  - the source tree: `-sBOOST_SOURCE=<dir>` on b2's command line, else a
    user-config.jam's `modules.poke : BOOST_SOURCE : <dir> ;`, else
    `.local/boost-source`; the Jamroot's path constant
    `WEBCPP_BOOST_SOURCE`;
  - `bash tools/ci/actions/boost/install.sh source <directory>`, which fills a
    tree with `boost/version.hpp` and every `libs/<library>/src` of Boost
    1.92.0's archive; the action's `install` keeps the same in
    `<prefix>/source`, and `configure` writes the `modules.poke` line;
  - `harness.without_boost_source(root)`, `harness.boost_source()`,
    `harness.gcc_request(root) -> tuple[str, ...]`;
  - `compile_commands.of_the_tree(root, source)`.
- Boost.Charconv and Boost.Locale join the same way in Task 3B.

- [ ] **Step 1: Write the failing tests of the action.** In
  `tools/ci/actions_test.py`:
  - `boost_archive` writes the parts of a source tree the action keeps, and
    one it does not; its two lines
    `tree = scratch / 'archive/boost_1_92_0'` and
    `(tree / 'tools/build').mkdir(parents=True)` become:

```python
    tree = scratch / 'archive/boost_1_92_0'
    (tree / 'tools/build').mkdir(parents=True)
    for path, text in (('boost/version.hpp', '#define BOOST_VERSION 109200\n'),
                       ('libs/url/src/url.cpp', '// url\n'),
                       ('libs/url/src/detail/pattern.hpp', '// pattern\n'),
                       ('libs/json/src/src.cpp', '// json\n'),
                       ('libs/url/doc/index.html', '<p>not kept</p>\n')):
        (tree / path).parent.mkdir(parents=True, exist_ok=True)
        (tree / path).write_text(text)
```

  - a constant and two cases, added to `CASES` after
    `test_boost_install_fails_when_the_prefix_is_incomplete`:

```python
# What install keeps in <prefix>/source, and source writes into a directory: the archive's
# boost/version.hpp and every libs/<library>/src, nothing else.
KEPT_SOURCES = ['boost/version.hpp', 'libs/json/src/src.cpp', 'libs/url/src/detail/pattern.hpp',
                'libs/url/src/url.cpp']


def files_under(directory: Path) -> list[str]:
    return sorted(str(path.relative_to(directory)) for path in directory.rglob('*')
                  if path.is_file())


def test_boost_keeps_the_sources_of_its_compiled_libraries(scratch: Path) -> None:
    script, runner = boost_install_tree(scratch)
    prefix = scratch / 'prefix'
    result = run(script, 'install', prefix, env=runner)
    assert result.returncode == 0, (result.returncode, result.stdout, result.stderr)
    assert files_under(prefix / 'source') == KEPT_SOURCES, files_under(prefix / 'source')
    result = subprocess.run(['bash', str(script), 'configure', str(prefix)], capture_output=True,
                            text=True, check=False, cwd=scratch, env={**os.environ, **runner})
    assert result.returncode == 0, (result.returncode, result.stderr)
    config = (scratch / '.local/user-config.jam').read_text()
    assert f'modules.poke : BOOST_SOURCE : "{prefix}/source" ;\n' in config, config


def test_boost_source_fills_a_directory_alone(scratch: Path) -> None:
    # A machine whose Boost comes from a package manager has no sources: source fills
    # .local/boost-source from the same archive, checked the same way, with no runner around it.
    script, _ = boost_install_tree(scratch)
    directory = scratch / 'local sources'
    for _ in range(2):
        # The second run replaces the tree the first wrote.
        result = run(script, 'source', directory,
                     env={'RUNNER_OS': '', 'RUNNER_TEMP': '', 'GITHUB_PATH': ''})
        assert result.returncode == 0, (result.returncode, result.stdout, result.stderr)
        assert files_under(directory) == KEPT_SOURCES, files_under(directory)
    assert not (scratch / '.local').exists()
    assert not (scratch / 'local sources.filling').exists()


def test_boost_source_refuses_a_directory_it_did_not_write(scratch: Path) -> None:
    # A tree of Boost's sources is replaced; anything else (.local itself, a directory of the
    # user's) is left as it is, before anything is downloaded.
    script, _ = boost_install_tree(scratch)
    directory = scratch / 'mine'
    directory.mkdir()
    (directory / 'keep.txt').write_text('mine\n')
    result = run(script, 'source', directory,
                 env={'RUNNER_OS': '', 'RUNNER_TEMP': '', 'GITHUB_PATH': ''})
    assert result.returncode == 1, (result.returncode, result.stdout, result.stderr)
    assert f'install.sh: {directory} exists and holds no boost/version.hpp' in result.stderr
    assert files_under(directory) == ['keep.txt'], files_under(directory)
```

  and `test_boost_source_refuses_a_directory_it_did_not_write` joins `CASES`
  after `test_boost_source_fills_a_directory_alone`.

  - in `test_boost_refuses_an_empty_or_relative_prefix`, the usage text
    becomes `'usage: install.sh key | install <prefix> | configure <prefix> | source <directory>'`
    and its list of arguments gains `['source']` and `['source', '']`;
  - in `test_boost_install_fails_when_the_prefix_is_incomplete`, the message
    becomes `f'install.sh: {prefix} holds no Boost headers, no b2 or no sources'`.

- [ ] **Step 2: Run them, to see them fail.**

Run: `python3 tools/ci/actions_test.py test_boost_keeps_the_sources_of_its_compiled_libraries test_boost_source_fills_a_directory_alone`
Expected: FAIL, `prefix/source` absent (`[] == KEPT_SOURCES`); the second
with status 2 and the old usage.

- [ ] **Step 3: The action.** In `tools/ci/actions/boost/install.sh`:
  - the usage comment gains
    `#   install.sh source <directory> fills <directory> with what install keeps in <prefix>/source,`
    `#                                 for a machine whose Boost has no sources;`
    and the line "Only headers are installed: no webcpp library links a
    compiled Boost library." becomes "The headers are installed, and beside
    them, in <prefix>/source, the sources of Boost's compiled libraries,
    from which webcpp builds those it needs (the Jamroot's
    /webcpp//boost_url)."
  - after `digest()`:

```bash
# Stops when destination exists and is no tree of Boost's sources: .local itself, or a directory
# of the user's, is never replaced.
refuse_foreign() {
    if [ -e "$1" ] && [ ! -f "$1/boost/version.hpp" ]; then
        printf 'install.sh: %s exists and holds no boost/version.hpp, so it is left as it is\n' \
            "$1" >&2
        exit 1
    fi
}

# Copies into destination what webcpp builds Boost's compiled libraries from: boost/version.hpp,
# which the Jamroot compares with the configured Boost's, and every libs/<library>/src of the
# source tree. It fills a sibling first and moves it into place, so that a build reading the old
# tree never sees half of the new one.
keep_sources() {
    local tree="$1" destination="${2%/}"
    refuse_foreign "${destination}"
    local filling="${destination}.filling"
    rm -rf "${filling}"
    mkdir -p "${filling}/boost" "${filling}/libs"
    cp "${tree}/boost/version.hpp" "${filling}/boost/version.hpp"
    local src library
    for src in "${tree}"/libs/*/src; do
        [ -d "${src}" ] || continue
        library="$(basename "$(dirname "${src}")")"
        mkdir -p "${filling}/libs/${library}"
        cp -R "${src}" "${filling}/libs/${library}/src"
    done
    rm -rf "${destination}"
    mv "${filling}" "${destination}"
}

# Fills directory with the sources install keeps, from the same archive, checked the same way: a
# machine's .local/boost-source, where the Jamroot looks when no -sBOOST_SOURCE is given.
source_tree() {
    local directory="$1"
    refuse_foreign "${directory%/}"
    local work
    work="$(mktemp -d)"
    "${here}/../../download.sh" "${url}" "${sha256}" "${work}/${archive}"
    tar -xzf "${work}/${archive}" -C "${work}"
    keep_sources "${work}/boost_${version//./_}" "${directory}"
    rm -rf "${work}"
}
```

  - in `install`, after `cd "boost_${version//./_}"`:

```bash
    keep_sources "$(pwd)" "${prefix}/source"
```

  - `check` requires the sources too:

```bash
    if [ ! -f "${prefix}/include/boost/version.hpp" ] || [ ! -x "${b2}" ] \
        || [ ! -f "${prefix}/source/boost/version.hpp" ]; then
        printf 'install.sh: %s holds no Boost headers, no b2 or no sources\n' "${prefix}" >&2
        exit 1
    fi
```

  - `configure` writes, after the `using boost` line:

```bash
        printf 'modules.poke : BOOST_SOURCE : "%s/source" ;\n' "${prefix}"
```

  - `usage` prints
    `usage: install.sh key | install <prefix> | configure <prefix> | source <directory>`,
    and the final `case` gains:

```bash
    source)
        [ -n "${2-}" ] || usage
        source_tree "$2"
        ;;
```

  In `action.yml`, the description becomes: "Installs Boost 1.92.0's headers,
  the sources of its compiled libraries and its b2 in a cached prefix, from
  the source archive of archives.boost.io checked against its SHA-256, writes
  the `using boost` line and the sources' `modules.poke` of
  .local/user-config.jam, and puts b2 on PATH. On Linux, macOS and Windows."

Run: `python3 tools/ci/actions_test.py`
Expected: every case `ok`, then `actions_test: ok`.

- [ ] **Step 4: Fill the local source tree** (235 MB downloaded once, from
  archives.boost.io).

Run: `bash tools/ci/actions/boost/install.sh source .local/boost-source && grep '^#define BOOST_VERSION ' .local/boost-source/boost/version.hpp && ls .local/boost-source/libs/url/src | head -3`
Expected: `#define BOOST_VERSION 109200`, then `decode_view.cpp`, `detail`,
`error.cpp`.

- [ ] **Step 5: Write the failing tests of the build.** In
  `tools/test/harness.py`:
  - `copy_tree`'s docstring ends "Only src's user-config.jam comes from its
    .local/, and a link to its .local/boost-source when it has one, so that
    the copy builds with the same toolsets and Boost sources without copying
    them." and, after it copies the user-config:

```python
    sources = top / '.local/boost-source'
    if sources.exists():
        (scratch / '.local').mkdir(exist_ok=True)
        (scratch / '.local/boost-source').symlink_to(sources.resolve())
```

  - after `without_boost`:

```python
# A line of a user-config.jam that names Boost's sources.
POKE_BOOST_SOURCE = re.compile(r'^\s*modules\.poke\s*:\s*BOOST_SOURCE\s*:\s*"?([^";]+?)"?\s*;\s*$',
                               re.MULTILINE)


def boost_source() -> Path:
    """The tree of Boost's sources this checkout builds with: the one its user-config.jam names,
    else its .local/boost-source."""
    found = POKE_BOOST_SOURCE.search(user_config(ROOT).read_text())
    return Path(found.group(1)) if found else (ROOT / '.local/boost-source').resolve()


# A line of a user-config.jam that configures gcc, and the version it names.
USING_GCC = re.compile(r'^\s*using\s+gcc\s*(?::\s*([^\s:;]+))?', re.MULTILINE)


def gcc_request(root: Path) -> tuple[str, ...]:
    """The b2 request of the gcc that root's user-config.jam configures, toolset=gcc-<version>,
    or nothing when it configures none, as the CI's tools job, which registers clang alone."""
    found = USING_GCC.search(user_config(root).read_text())
    if not found:
        return ()
    return (f'toolset=gcc-{found.group(1)}',) if found.group(1) else ('toolset=gcc',)


def without_boost_source(root: Path) -> None:
    """Leaves the scratch superproject root without Boost's sources: its user-config.jam, the one
    b2 reads for it, names none, and its .local holds no boost-source."""
    configure(root, POKE_BOOST_SOURCE.sub('', user_config(root).read_text()))
    link = root / '.local/boost-source'
    if link.is_symlink():
        link.unlink()
    elif link.exists():
        shutil.rmtree(link)
```

  Create `tools/test/boost_libraries_test.py`:

```python
#!/usr/bin/env python3
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Checks the compiled Boost libraries the Jamroot builds from Boost's sources: /webcpp//boost_url
links natively, on wasip2, wasip3 and emscripten, and in a user's build without exceptions, each
time an archive of the build's own, never the system's; a program that uses Boost.URL without it
does not link; and the sources are -sBOOST_SOURCE=<dir>, else user-config.jam's BOOST_SOURCE, else
.local/boost-source, a missing tree,
a tree of another Boost version and a tree without the library's sources each stopping the build
by name, while a build that needs none never looks. Each case builds a scratch superproject with
the fixture library demo, whose path holds a space. Run with the names of some cases to run only
those."""

from __future__ import annotations

import re
import sys
from pathlib import Path

import harness

WASIP2 = ('toolset=clang-wasip2', 'testing.launcher=wasmtime')
WASIP3 = ('toolset=clang-wasip3', 'testing.launcher=wasmtime')
EMSCRIPTEN = ('toolset=emscripten',)

PASSED = re.compile(r'^\*\*passed\*\* (.*)$', re.MULTILINE)

URLS_JAMFILE = ('import webcpp ;\n'
                '\n'
                'webcpp.targets native wasip2 wasip3 emscripten ;\n'
                '\n'
                'webcpp.run urls : urls.cpp : <library>/webcpp//boost_url ;\n')

# ipv4_address(bytes_type) and to_bytes() are defined in Boost.URL's compiled sources alone, which
# is why formats needs the library (WEBCPP_FORMATS_SPEC section 8.2).
URLS_SOURCE = r'''#include <boost/url/ipv4_address.hpp>
#include <boost/url/parse.hpp>

#include <cstdio>

int main() {
    boost::urls::ipv4_address::bytes_type const bytes{{127, 0, 0, 1}};
    boost::urls::ipv4_address const address(bytes);
    auto const parsed = boost::urls::parse_uri("http://example.com/a?b#c");
    bool const ok = parsed.has_value() && parsed->host() == "example.com" &&
                    address.to_bytes() == bytes;
    std::puts(ok ? "parsed" : "not parsed");
    return ok ? 0 : 1;
}
'''


def passed(result):
    """The tests b2 reports as passed, by name."""
    return {path.rsplit('/', 1)[-1].removesuffix('.test') for path in PASSED.findall(result.stdout)}


def archives(root: Path) -> list[Path]:
    """Every libboost_url.a the build made under the scratch superproject's bin/."""
    return sorted((root / 'bin').rglob('libboost_url.a'))


def add_urls(root: Path, jamfile: str = URLS_JAMFILE) -> None:
    harness.add_library(root, 'urls', jamfile, {'urls.cpp': URLS_SOURCE})


def fake_sources(directory: Path, version: int, with_url: bool = True) -> Path:
    """Writes in directory a tree of Boost's sources that is only a version.hpp defining version,
    such as 109100, and, with_url, one source of Boost.URL; returns it."""
    (directory / 'boost').mkdir(parents=True)
    (directory / 'boost/version.hpp').write_text(f'#define BOOST_VERSION {version}\n')
    if with_url:
        (directory / 'libs/url/src').mkdir(parents=True)
        (directory / 'libs/url/src/fake.cpp').write_text('int fake_url() { return 0; }\n')
    return directory


def test_boost_url_links_on_every_target(root: Path) -> None:
    harness.link_emsdk(root)
    add_urls(root)
    gcc = harness.gcc_request(root)
    requests = ((), WASIP2, WASIP3, EMSCRIPTEN, ('exception-handling=off',),
                (*WASIP2, 'exception-handling=off'), *((gcc,) if gcc else ()))
    for count, request in enumerate(requests, start=1):
        result = harness.run_b2(root, '-d+2', *request, 'libs/urls/test')
        harness.expect(result, True)
        assert passed(result) == {'urls'}, (request, passed(result), result.stdout[-4000:])
        # One archive more for each variant, built from the sources: never the system's own
        # libboost_url, which Homebrew ships.
        assert len(archives(root)) == count, (request, archives(root))
        assert '-lboost_url' not in result.stdout, (request, result.stdout[-4000:])


def test_without_the_library_a_program_does_not_link(root: Path) -> None:
    add_urls(root, URLS_JAMFILE.replace(' : <library>/webcpp//boost_url', ''))
    result = harness.run_b2(root, '-a', 'libs/urls/test')
    harness.expect(result, False)
    assert re.search(r'^\.\.\.failed .*urls', result.stdout, re.MULTILINE), result.stdout[-4000:]


def test_missing_sources_are_refused_naming_the_fix(root: Path) -> None:
    harness.without_boost_source(root)
    add_urls(root)
    result = harness.run_b2(root, 'libs/urls/test')
    harness.expect(result, False,
                   "Boost's sources, which a compiled Boost library of /webcpp is built from, "
                   'were not found: no -sBOOST_SOURCE=<dir> was given',
                   'install.sh source .local/boost-source')
    harness.expect(harness.run_b2(root, 'libs/demo/test//pass'), True)


def test_sources_of_another_version_are_refused(root: Path) -> None:
    sources = fake_sources(root / 'old sources', 109100)
    add_urls(root)
    result = harness.run_b2(root, f'-sBOOST_SOURCE={sources}', 'libs/urls/test')
    harness.expect(result, False, "holds Boost 1.91.0's sources", 'is 1.92.0')


def test_a_tree_without_the_library_s_sources_is_refused(root: Path) -> None:
    sources = fake_sources(root / 'no url', 109200, with_url=False)
    add_urls(root)
    result = harness.run_b2(root, f'-sBOOST_SOURCE={sources}', 'libs/urls/test')
    harness.expect(result, False, 'libs/url/src holds no .cpp file')


def test_sources_named_by_user_config(root: Path) -> None:
    # The CI's path: the Boost action's modules.poke in user-config.jam, and no -sBOOST_SOURCE.
    harness.without_boost_source(root)
    text = harness.user_config(root).read_text()
    harness.configure(root, text + f'modules.poke : BOOST_SOURCE : "{harness.boost_source()}" ;\n')
    add_urls(root)
    result = harness.run_b2(root, 'libs/urls/test')
    harness.expect(result, True)
    assert passed(result) == {'urls'}, (passed(result), result.stdout[-4000:])
    missing = root / 'no sources'
    harness.configure(root, text + f'modules.poke : BOOST_SOURCE : "{missing}" ;\n')
    harness.expect(harness.run_b2(root, 'libs/urls/test'), False,
                   f"user-config.jam's BOOST_SOURCE, {missing}, holds no boost/version.hpp")


def test_the_command_line_wins(root: Path) -> None:
    harness.without_boost_source(root)
    fake_sources(root / '.local/boost-source', 109100)
    add_urls(root)
    result = harness.run_b2(root, f'-sBOOST_SOURCE={harness.boost_source()}', 'libs/urls/test')
    harness.expect(result, True)
    assert passed(result) == {'urls'}, (passed(result), result.stdout[-4000:])


CASES = [
    test_boost_url_links_on_every_target,
    test_without_the_library_a_program_does_not_link,
    test_missing_sources_are_refused_naming_the_fix,
    test_sources_of_another_version_are_refused,
    test_a_tree_without_the_library_s_sources_is_refused,
    test_sources_named_by_user_config,
    test_the_command_line_wins,
]


if __name__ == '__main__':
    sys.exit(harness.run_cases('boost_libraries_test', CASES, sys.argv[1:]))
```

  `test_the_command_line_wins` must read the real tree before
  `without_boost_source` edits the scratch, which it does: `boost_source()`
  reads `harness.ROOT`'s configuration, not the scratch's.

- [ ] **Step 6: Run them, to see them fail.**

Run: `python3 tools/test/boost_libraries_test.py test_boost_url_links_on_every_target`
Expected: FAIL, `error: Unable to find file or target named` `'/webcpp//boost_url'`.

- [ ] **Step 7: The Jamroot.** After `path-constant WEBCPP_ROOT : . ;`:

```
# The sources of the compiled Boost libraries that webcpp builds for every
# target (tools/boost/Jamfile), as /webcpp//boost_json is built from
# Boost.JSON's headers: a tree of Boost's source archive, at least its
# boost/version.hpp and the libs/<library>/src of each library built. It is
# -sBOOST_SOURCE=<dir> on the command line, which wins, else a
# user-config.jam's modules.poke of BOOST_SOURCE, else .local/boost-source,
# which `bash tools/ci/actions/boost/install.sh source .local/boost-source`
# fills. Its version must be the configured Boost's. Both are checked only
# when a target uses a compiled Boost library.
.boost-source-local = [ path.join [ path.root [ path.make $(WEBCPP_ROOT) ] [ path.pwd ] ]
    .local/boost-source ] ;
.boost-source-given = [ MATCH "^-sBOOST_SOURCE=(.+)$" : [ modules.peek : ARGV ] ] ;
.boost-source-origin = "-sBOOST_SOURCE=$(.boost-source-given[-1])" ;
if ! $(.boost-source-given)
{
    .boost-source-given = [ modules.peek : BOOST_SOURCE ] ;
    .boost-source-origin = "user-config.jam's BOOST_SOURCE, $(.boost-source-given[-1])," ;
}
.boost-source = $(.boost-source-local) ;
if $(.boost-source-given)
{
    .boost-source-given = $(.boost-source-given[-1]) ;
    .boost-source = [ path.root [ path.make $(.boost-source-given) ] [ path.pwd ] ] ;
}
path-constant WEBCPP_BOOST_SOURCE : [ path.native $(.boost-source) ] ;

# The check of a compiled Boost library's alias: the tree holds a
# boost/version.hpp, of the configured Boost's version.
rule boost-source-usable ( properties * )
{
    local shown = [ path.native $(.boost-source) ] ;
    local version = [ boost-version $(.boost-source) ] ;
    if ! $(version)
    {
        local looked = "no -sBOOST_SOURCE=<dir> was given, and $(shown) holds no boost/version.hpp" ;
        if $(.boost-source-given)
        {
            looked = "$(.boost-source-origin) holds no boost/version.hpp" ;
        }
        errors.user-error "Boost's sources, which a compiled Boost library of /webcpp is built"
            "from, were not found: $(looked). Fill .local/boost-source with `bash"
            "tools/ci/actions/boost/install.sh source .local/boost-source`, or give"
            "-sBOOST_SOURCE=<dir>." ;
    }
    if $(version[1]) != $(.boost-found[1])
    {
        errors.user-error "$(shown) holds Boost $(version[2])'s sources, and the configured"
            "Boost in $(.boost-include) is $(.boost-found[2]): a compiled Boost library is"
            "built from the sources of the configured Boost's version" ;
    }
}

# Boost's compiled libraries, built from those sources (tools/boost/Jamfile).
use-project /webcpp/boost-libraries : tools/boost ;

# Boost.URL, compiled: a library that needs it puts <library>/webcpp//boost_url
# in its main target's usage requirements, as one that needs Boost.JSON does
# with boost_json. The alias checks the sources before anything is built.
alias boost_url : tools/boost//boost_url : <conditional>@boost-source-usable ;
```

  and the last line's `explicit` list gains `boost_url`.

  Create `tools/boost/Jamfile`:

```
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)
#
# The compiled Boost libraries that webcpp's libraries need, built from
# Boost's sources for every target, static and once per variant, so that a
# program's own features, exception-handling=off among them, reach them. The
# sources are the Jamroot's WEBCPP_BOOST_SOURCE, whose version the Jamroot's
# alias of each library checks before anything is built: /webcpp//boost_url.
# They are Boost's code, not webcpp's, and compile without webcpp's
# warnings, as Boost's headers are read as system ones.
import errors ;

project /webcpp/boost-libraries
  : source-location $(WEBCPP_BOOST_SOURCE)
  : requirements <warnings>off <warnings-as-errors>off
  ;

# A tree without Boost.URL's sources gets an alias that stops the build
# naming them, never a lib without sources, which b2 would search for on the
# system, where a package's own libboost_url may be.
rule url-sources-missing ( properties * )
{
    errors.user-error "$(WEBCPP_BOOST_SOURCE)/libs/url/src holds no .cpp file, and"
        "/webcpp//boost_url is built from them: fill the tree from Boost's source"
        "archive (`bash tools/ci/actions/boost/install.sh source <dir>`)" ;
}

# Boost.URL, with the macros of its own libs/url/build/Jamfile: its sources
# see BOOST_URL_SOURCE, and it and its users BOOST_URL_STATIC_LINK=1 and
# BOOST_URL_NO_LIB=1.
.url = [ glob-tree-ex libs/url/src : *.cpp ] ;
if $(.url)
{
    lib boost_url : $(.url)
      : <link>static <define>BOOST_URL_SOURCE <define>BOOST_URL_STATIC_LINK=1
      : : <define>BOOST_URL_STATIC_LINK=1 <define>BOOST_URL_NO_LIB=1 ;
}
else
{
    alias boost_url : : <conditional>@url-sources-missing ;
}

explicit boost_url ;
```

- [ ] **Step 8: Run the tests of the build.**

Run: `export EM_CACHE=~/dev/webcpp/.local/emscripten-cache; python3 tools/test/boost_libraries_test.py`
Expected: seven `<case>: ok` lines, then `boost_libraries_test: ok`. If
Boost.URL does not build or run on emscripten (no measurement exists; wasip2
and wasip3 were measured), stop and report the failure to the controller
with its log: a target is never declared without a lane that proves it.

Run: `python3 tools/test/jamroot_test.py && python3 tools/test/webcpp_jam_test.py`
Expected: `jamroot_test: ok`, `webcpp_jam_test: ok` (the scratch copies now
carry `.local/boost-source`, which no fixture uses).

- [ ] **Step 9: The lint never analyses Boost's sources.** In
  `tools/lint/lint_test.py`, a case added to `CASES` after
  `test_a_dependency_s_warnings_are_not_ours`:

```python
def test_a_compiled_boost_library_s_sources_are_not_analysed(root):
    # Boost.URL's sources, which /webcpp//boost_url compiles from Boost's tree outside the tree
    # the lint reads (.local/boost-source, or a CI runner's prefix), are Boost's: the database
    # lists the program that links them, and none of theirs.
    prepare(root)
    append(root, 'libs/demo/test/Jamfile',
           'webcpp.run parses_url : parses_url.cpp : <library>/webcpp//boost_url ;\n')
    (root / 'libs/demo/test/parses_url.cpp').write_text(
        CPP + '\n'
        '#include <boost/url/parse.hpp>\n'
        '\n'
        'int main() {\n'
        '    return boost::urls::parse_uri("http://example.com").has_value() ? 0 : 1;\n'
        '}\n')
    commit(root / 'libs/demo', 'A program that links Boost.URL')
    completed = compile_database(root)
    assert completed.returncode == 0, completed.stdout[-6000:]
    files = {entry['file'] for entry in json.loads(
        (root / 'compile database/compile_commands.json').read_text())}
    assert str(root / 'libs/demo/test/parses_url.cpp') in files, sorted(files)
    assert not [file for file in files if '/libs/url/src/' in file], sorted(files)
    expect_clean(lint(root))
```

Run: `python3 tools/lint/lint_test.py test_a_compiled_boost_library_s_sources_are_not_analysed`
Expected: FAIL, the second assertion listing `.../libs/url/src/*.cpp`.

  In `tools/lint/compile_commands.py`, after `libraries`:

```python
def of_the_tree(root: str, source: str) -> bool:
    """Whether source, as b2 names it, is a file of the tree the lint reads: not a path outside
    root, nor one under its .local/, where a machine keeps what it fetched, Boost's sources among
    them, nor one under third_party/libs/, whose code is not webcpp's."""
    parts = Path(os.path.relpath(os.path.normpath(os.path.join(root, source)), root)).parts
    return parts[0] not in ('..', '.local') and parts[:2] != ('third_party', 'libs')
```

  In `read`, the guard

```python
        if source.startswith(f'{BUILD_DIR}/'):
            continue
```

  becomes

```python
        if source.startswith(f'{BUILD_DIR}/') or not of_the_tree(root, source):
            continue
```

  and in the loop over `compiles(dry_run(...))` of the twins,

```python
        if source.startswith(f'{BUILD_DIR}/') or first.setdefault(source, target) != target:
```

  becomes

```python
        if (source.startswith(f'{BUILD_DIR}/') or not of_the_tree(root, source)
                or first.setdefault(source, target) != target):
```

Run: `python3 tools/lint/lint_test.py test_a_compiled_boost_library_s_sources_are_not_analysed test_compile_database_lists_what_b2_builds`
Expected: both `ok`, then `lint_test: ok`.

- [ ] **Step 10: The texts.**
  - `AGENTS.md` chapter 1, "Prerequisites": "- Boost 1.92 or newer,
    installed;" becomes "- Boost 1.92 or newer, installed, and, for a library
    that links a compiled Boost library (Boost.URL), Boost's sources of the
    same version (below);", and after the paragraph that begins "When Boost
    cannot be used":

```
**Boost's sources.** A compiled Boost library that a webcpp library needs
is built from Boost's own sources, for every target, as Boost.JSON's
definitions are: `/webcpp//boost_url` (`tools/boost/Jamfile`). An installed
Boost has none (Homebrew's ships its own `libboost_url` and no
`libs/url/src`), so the build reads them from `-sBOOST_SOURCE=<dir>` on b2's
command line, which wins, else a user-config.jam's `modules.poke :
BOOST_SOURCE : <dir> ;`, else `.local/boost-source`: a tree with Boost's
`boost/version.hpp` and the `libs/<library>/src` of each library, which
`bash tools/ci/actions/boost/install.sh source .local/boost-source` fills
from the archive the CI uses, checked against its SHA-256. When a target
needs the tree, the build stops if it is missing, naming both places, if
its version is not the configured Boost's, naming both versions, and if it
lacks the library's sources; a build that needs none never looks.
```

  - `AGENTS.md` chapter 1, "The layout", under `tools/`: `boost/  Jamfile:
    the compiled Boost libraries built from Boost's sources
    (/webcpp//boost_url)`.
  - `AGENTS.md` chapter 2, checklist item 4: the sentences "Only Boost's
    header-only libraries are available: the CI installs Boost's headers
    alone, and no rule links a compiled Boost library." and "Compiled Boost
    libraries are on the roadmap (chapter 13)." become "A compiled Boost
    library is built by webcpp from Boost's sources (chapter 1): a library
    that needs Boost.URL adds `<library>/webcpp//boost_url` to its target's
    usage requirements, static, built once per variant with Boost.URL's own
    macros; never `/boost//url`, which would link the system's."
  - `AGENTS.md` chapter 9, "The actions", the `boost` bullet: "installs
    Boost's headers (`--with-headers`, since no library links a compiled
    Boost library)" becomes "installs Boost's headers (`--with-headers`),
    keeps in `<prefix>/source` the archive's `boost/version.hpp` and every
    `libs/<library>/src`, which `configure` names in `.local/user-config.jam`
    with `modules.poke : BOOST_SOURCE`, and"; and the bullet ends "`install.sh
    source <dir>` fills such a tree alone, for a machine whose Boost comes from
    elsewhere."
  - `AGENTS.md` chapter 13, the bullet "Compiled Boost libraries" becomes:
    "**Compiled Boost libraries.** Boost.URL is built from Boost's sources
    (`/webcpp//boost_url`); Boost.Locale and Boost.Charconv join the same way
    when a library needs them. A Boost.Test suite still compiles the
    framework's header-only form (chapters 2 and 9)."
  - `AGENTS.md` chapter 10: add `tools/test/boost_libraries_test.py`.
  - `README.md`, "Prerequisites": "- Boost 1.92 or newer, installed, and for a
    library that links Boost.URL its sources, which `bash
    tools/ci/actions/boost/install.sh source .local/boost-source` fetches;"
    and "The tests of the tools": "... the Jamroot, the compiled Boost
    libraries, `tools/webcpp.jam`, ...".

- [ ] **Step 11: Verify.** The green of a superproject change, items 1 to 6,
  and `actionlint .github/workflows/*.yml` when actionlint is on `PATH`
  (the action's YAML changed only in its description). Expected: as stated
  there; actionlint prints nothing.

- [ ] **Step 12: Commit.**

```bash
git add -- tools/boost/Jamfile tools/test/boost_libraries_test.py
git -c user.name=R -c user.email=github.overboard312@passmail.net commit \
  -m "tools: Boost.URL built from Boost's sources for every target, as /webcpp//boost_url" \
  -- Jamroot tools/boost/Jamfile tools/test/boost_libraries_test.py tools/test/harness.py \
     tools/ci/actions/boost/install.sh tools/ci/actions/boost/action.yml tools/ci/actions_test.py \
     tools/lint/compile_commands.py tools/lint/lint_test.py AGENTS.md README.md
```

**Changelog**, Tools and build:
- Added `/webcpp//boost_url`, Boost.URL's compiled library built from
  Boost's sources for every target; the build reads them from
  `-sBOOST_SOURCE=<dir>`, else `.local/boost-source`, and refuses a tree of
  another Boost version.
- Changed the CI's Boost action to keep the sources of Boost's compiled
  libraries, and added `install.sh source <dir>`, which fills a local tree.
- Changed the lint to never analyse a source outside the tree, such as
  Boost's.

- [ ] **Step 13 (controller, at the task's completion): Boost's sources for
  every worktree.** `cp -R ~/dev/webcpp-infra/.local/boost-source ~/dev/webcpp/.local/boost-source`
  (machine-local, never tracked; a worktree whose `.local` links to M's then
  has it). Expected: `grep '^#define BOOST_VERSION ' ~/dev/webcpp/.local/boost-source/boost/version.hpp`
  prints `#define BOOST_VERSION 109200`.

---

### Task 3B: `/webcpp//boost_charconv` and `/webcpp//boost_locale`

The 2026-10-09 decision's next libraries, for json_schema's locale header
(its E3.4). Measured before this plan (`~/.buzz/RESEARCH/BOOST_LOCALE_ON_EVERY_TARGET.md`):
Boost.Locale 1.92 builds without ICU with its posix and std backends and
iconv, and runs (messages from a `.mo`, plurals, iconv conversions; numbers
and dates in the C locale's form on WASI), on macOS (Apple clang and g++-15,
`-liconv`), Linux (g++-14 and clang-18, iconv in glibc), wasip2 and wasip3
(with exceptions, iconv in wasi-libc); it does not build on emscripten,
single-threaded there, and not without exceptions. Boost.Charconv builds and
runs on all four targets, with and without exceptions.

**What each target provides** (AGENTS.md chapter 2 gains this table):

| Target | `/webcpp//boost_locale` | `/webcpp//boost_charconv` |
| --- | --- | --- |
| native Linux (gcc, clang, glibc) | backends posix and std; iconv from glibc; no ICU (measured) | yes (measured) |
| native macOS (Apple clang, gcc) | backends posix and std; iconv from the system's libiconv, `-liconv` given by the target; no ICU (measured) | yes (measured) |
| native Windows (MSVC) | backends winapi and std, no iconv, no ICU: Boost's own configuration on Windows. Not measured here: no Windows machine; the first CI run that builds it is its measurement, and a failure is reported to the owner, never hidden | yes; not measured here |
| wasip2, wasip3 | backends posix and std; iconv from wasi-libc; number, currency and date formatting in the C locale's form (no locale data, no ICU); a catalog is read from a directory the test gives (`webcpp.data`) (measured) | yes (measured) |
| emscripten | refused by name: `shared/date_time.cpp`, `shared/generator.cpp` and `shared/localization_backend.cpp` include `<boost/thread/mutex.hpp>`, and Boost.Config stops with "Compiler threading support is not turned on" (`boost/config/requires_threads.hpp:47`), webcpp's emscripten being single-threaded (measured) | yes (measured) |
| any target, `exception-handling=off` | refused by name: Boost.Locale throws and catches unguarded (61 `throw`, 14 `try` in its sources, 8 `throw` in its headers) | yes (measured natively and on wasip2) |

**Files:**
- Modify: `tools/boost/Jamfile` (`import path ;`; Boost.Charconv, iconv,
  Boost.Locale), `Jamroot` (two aliases; the last `explicit` gains
  `boost_charconv boost_locale`), `tools/test/boost_libraries_test.py`
  (cases), `AGENTS.md` (chapter 2's checklist item 4 and its table, chapter
  13)

**Interfaces:**
- Consumes: Task 3's `boost-source-usable`, the project
  `/webcpp/boost-libraries` and its `WEBCPP_BOOST_SOURCE`; `install.sh`
  already keeps `libs/charconv/src` and `libs/locale/src`; `harness.gcc_request`.
- Produces: `<library>/webcpp//boost_charconv` (usage requirement
  `<define>BOOST_CHARCONV_NO_LIB=1`) and `<library>/webcpp//boost_locale`
  (usage requirements `<define>BOOST_LOCALE_NO_LIB=1`, Boost.Charconv, and
  `-liconv` on macOS), as the table says. json_schema's locale header and its
  test name `<library>/webcpp//boost_locale`, declare the targets native,
  wasip2 and wasip3 for that test, and give its catalogs with
  `[ webcpp.data <directory> ]` (Task 4).

- [ ] **Step 1: Write the failing tests.** In `tools/test/boost_libraries_test.py`,
  the imports gain `import shutil` and `import struct`, the docstring gains
  "; /webcpp//boost_charconv links on every target, with and without
  exceptions; /webcpp//boost_locale converts, and translates from a catalog,
  natively and on wasip2 and wasip3, and is refused by name on emscripten and
  without exceptions", and before `CASES`:

```python
CHARCONV_JAMFILE = ('import webcpp ;\n'
                    '\n'
                    'webcpp.targets native wasip2 wasip3 emscripten ;\n'
                    '\n'
                    'webcpp.run charconv : charconv.cpp : <library>/webcpp//boost_charconv ;\n')

CHARCONV_SOURCE = r'''#include <boost/charconv.hpp>

#include <cstdio>
#include <cstring>
#include <system_error>

int main() {
    double value = 0;
    char const text[] = "1234567.5";
    auto const parsed = boost::charconv::from_chars(text, text + std::strlen(text), value);
    char buffer[64];
    auto const written = boost::charconv::to_chars(buffer, buffer + sizeof buffer - 1, value);
    *written.ptr = '\0';
    std::printf("%s %d\n", buffer, parsed.ec == std::errc());
    return parsed.ec == std::errc() && std::strcmp(buffer, "1234567.5") == 0 ? 0 : 1;
}
'''

# translates reads its catalog natively alone until webcpp.data gives it the directory on WASI.
LOCALE_JAMFILE = r'''import regex ;
import webcpp ;

webcpp.targets native wasip2 wasip3 emscripten ;

path-constant CATALOGS : catalogs ;
local catalogs = [ regex.replace $(CATALOGS) "\\\\" "/" ] ;
webcpp.run converts : converts.cpp : <library>/webcpp//boost_locale ;
webcpp.run translates : translates.cpp
  : <library>/webcpp//boost_locale <define>"\"WEBCPP_TEST_CATALOGS=\\\"$(catalogs)\\\"\""
  : native ;
'''

TRANSLATES_ON_WASI = False

CONVERTS_SOURCE = r'''#include <boost/locale.hpp>

#include <iostream>
#include <string>

int main() {
    for (std::string const& backend :
         boost::locale::localization_backend_manager::global().get_all_backends()) {
        std::cout << backend << ' ';
    }
    boost::locale::generator generator;
    std::locale const locale = generator("en_US.UTF-8");
    bool const utf8 = std::use_facet<boost::locale::info>(locale).utf8();
    std::string const converted = boost::locale::conv::to_utf<char>("caf\xe9", "ISO-8859-1");
    std::cout << '|' << converted << '|' << utf8 << '\n';
    return converted == "caf\xc3\xa9" && utf8 ? 0 : 1;
}
'''

TRANSLATES_SOURCE = r'''#include <boost/locale.hpp>

#include <iostream>
#include <string>

int main() {
    boost::locale::generator generator;
    generator.add_messages_path(WEBCPP_TEST_CATALOGS);
    generator.add_messages_domain("probe");
    std::locale const locale = generator("pt_BR.UTF-8");
    std::string const translated = boost::locale::translate("Hello World").str(locale);
    std::string const converted = boost::locale::conv::to_utf<char>("caf\xe9", "ISO-8859-1");
    std::cout << translated << '|' << converted << '\n';
    return translated == "Ol\xc3\xa1 Mundo" && converted == "caf\xc3\xa9" ? 0 : 1;
}
'''


def mo(messages: dict[str, str]) -> bytes:
    """A GNU message catalog holding messages, msgid to msgstr, as msgfmt writes one, without its
    optional hash table."""
    keys = sorted(messages)
    texts = [key.encode() for key in keys] + [messages[key].encode() for key in keys]
    count = len(keys)
    offset = 28 + 16 * count
    table = []
    for text in texts:
        table += [len(text), offset]
        offset += len(text) + 1
    return (struct.pack('<7I', 0x950412DE, 0, count, 28, 28 + 8 * count, 0, 0)
            + struct.pack(f'<{4 * count}I', *table) + b''.join(text + b'\0' for text in texts))


def output_of(root: Path, name: str) -> str:
    """The text of the one file named name that b2 wrote under the scratch superproject's bin/."""
    found = sorted((root / 'bin').rglob(name))
    assert len(found) == 1, (name, found)
    return found[0].read_text()


def add_locales(root: Path) -> None:
    """Adds the library locales, whose tests convert, and translate from a catalog of their own."""
    harness.add_library(root, 'locales', LOCALE_JAMFILE,
                        {'converts.cpp': CONVERTS_SOURCE, 'translates.cpp': TRANSLATES_SOURCE})
    catalog = root / 'libs/locales/test/catalogs/pt_BR/LC_MESSAGES/probe.mo'
    catalog.parent.mkdir(parents=True)
    catalog.write_bytes(mo({'': 'Content-Type: text/plain; charset=UTF-8\n',
                            'Hello World': 'Olá Mundo'}))


def test_boost_charconv_links_on_every_target(root: Path) -> None:
    harness.link_emsdk(root)
    harness.add_library(root, 'numbers', CHARCONV_JAMFILE, {'charconv.cpp': CHARCONV_SOURCE})
    gcc = harness.gcc_request(root)
    for request in ((), WASIP2, WASIP3, EMSCRIPTEN, ('exception-handling=off',),
                    (*WASIP2, 'exception-handling=off'), *((gcc,) if gcc else ())):
        result = harness.run_b2(root, '-a', '-d+2', *request, 'libs/numbers/test')
        harness.expect(result, True)
        assert passed(result) == {'charconv'}, (request, passed(result), result.stdout[-4000:])
        assert output_of(root, 'charconv.output').startswith('1234567.5 1\n'), request
        assert '-lboost_charconv' not in result.stdout, (request, result.stdout[-4000:])
        shutil.rmtree(root / 'bin')


def test_boost_locale_on_native_wasip2_and_wasip3(root: Path) -> None:
    add_locales(root)
    gcc = harness.gcc_request(root)
    for request in ((), WASIP2, WASIP3, *((gcc,) if gcc else ())):
        wasi = request in (WASIP2, WASIP3)
        result = harness.run_b2(root, '-a', '-d+2', *request, 'libs/locales/test')
        harness.expect(result, True)
        expected = {'converts', 'translates'} if TRANSLATES_ON_WASI or not wasi else {'converts'}
        assert passed(result) == expected, (request, passed(result), result.stdout[-4000:])
        assert output_of(root, 'converts.output').startswith('posix std |café|1\n'), request
        if 'translates' in expected:
            assert output_of(root, 'translates.output').startswith(
                'Olá Mundo|café\n'), request
        assert '-lboost_locale' not in result.stdout, (request, result.stdout[-4000:])
        shutil.rmtree(root / 'bin')


def test_boost_locale_is_refused_on_emscripten(root: Path) -> None:
    harness.link_emsdk(root)
    add_locales(root)
    harness.expect(harness.run_b2(root, *EMSCRIPTEN, 'libs/locales/test'), False,
                   'Boost.Locale is not built for emscripten',
                   'Compiler threading support is not turned on')


def test_boost_locale_is_refused_without_exceptions(root: Path) -> None:
    add_locales(root)
    harness.expect(harness.run_b2(root, 'exception-handling=off', 'libs/locales/test'), False,
                   '/webcpp//boost_locale needs exceptions')
```

  and the four cases join `CASES` after `test_boost_url_links_on_every_target`.

- [ ] **Step 2: Run them, to see them fail.**

Run: `python3 tools/test/boost_libraries_test.py test_boost_charconv_links_on_every_target test_boost_locale_on_native_wasip2_and_wasip3`
Expected: FAIL, `error: Unable to find file or target named` `'/webcpp//boost_charconv'`
(then the same for `'/webcpp//boost_locale'`).

- [ ] **Step 3: The libraries.** In `tools/boost/Jamfile`, `import path ;`
  joins `import errors ;`, the header comment names
  `/webcpp//boost_charconv` and `/webcpp//boost_locale` beside
  `/webcpp//boost_url`, and after Boost.URL's `explicit boost_url ;`:

```
rule charconv-sources-missing ( properties * )
{
    errors.user-error "$(WEBCPP_BOOST_SOURCE)/libs/charconv/src holds no .cpp file, and"
        "/webcpp//boost_charconv is built from them: fill the tree from Boost's source"
        "archive (`bash tools/ci/actions/boost/install.sh source <dir>`)" ;
}

# Boost.Charconv, with the macros of its own build/Jamfile: its sources see
# BOOST_CHARCONV_SOURCE=1, and its users BOOST_CHARCONV_NO_LIB=1. Without
# GCC's quadmath, so without the __float128 overloads, on every target alike.
.charconv = [ glob libs/charconv/src/*.cpp ] ;
if $(.charconv)
{
    lib boost_charconv : $(.charconv)
      : <link>static <define>BOOST_CHARCONV_SOURCE=1
      : : <define>BOOST_CHARCONV_NO_LIB=1 ;
}
else
{
    alias boost_charconv : : <conditional>@charconv-sources-missing ;
}

explicit boost_charconv ;

# The system's iconv, which macOS keeps in a library of its own; glibc and
# wasi-libc have it in libc.
lib iconv : : <name>iconv ;

explicit iconv ;

# Boost.Locale without ICU (~/.buzz/RESEARCH/BOOST_LOCALE_ON_EVERY_TARGET.md
# measured it): the shared sources of its own build/Jamfile.v2 and its std
# backend on every target; with them the posix backend and iconv on Linux,
# macOS and WASI, and the winapi backend on Windows, as Boost configures it
# there. Refused on emscripten, whose single-threaded build Boost.Thread's
# mutex rejects, and without exceptions, which Boost.Locale throws
# unguarded.
.locale-shared = encoding/codepage shared/date_time shared/format shared/formatting
    shared/generator shared/iconv_codecvt shared/ids shared/localization_backend
    shared/message shared/mo_lambda util/codecvt_converter util/default_locale
    util/encoding util/info util/locale_data util/gregorian std/codecvt std/collate
    std/converter std/numeric std/std_backend ;
.locale-posix = posix/collate posix/converter posix/numeric posix/codecvt posix/posix_backend ;
.locale-winapi = win32/collate win32/converter win32/numeric win32/win_backend win32/lcid ;

rule locale-configuration ( properties * )
{
    if <exception-handling>off in $(properties)
    {
        errors.user-error "/webcpp//boost_locale needs exceptions: Boost.Locale throws and"
            "catches without a path for a build without them; build with exceptions, or"
            "leave Boost.Locale out" ;
    }
    if <toolset>emscripten in $(properties)
    {
        errors.user-error "Boost.Locale is not built for emscripten: three of its sources"
            "include <boost/thread/mutex.hpp>, which Boost.Config refuses in a"
            "single-threaded build (\"Compiler threading support is not turned on\"), and"
            "webcpp's emscripten is single-threaded" ;
    }
    if <target-os>windows in $(properties)
    {
        return <source>libs/locale/src/$(.locale-winapi).cpp
            <define>BOOST_LOCALE_NO_POSIX_BACKEND=1
            <define>_CRT_SECURE_NO_WARNINGS <define>_SCL_SECURE_NO_WARNINGS ;
    }
    return <source>libs/locale/src/$(.locale-posix).cpp
        <define>BOOST_LOCALE_WITH_ICONV=1 <define>BOOST_LOCALE_NO_WINAPI_BACKEND=1 ;
}

rule locale-sources-missing ( properties * )
{
    errors.user-error "$(WEBCPP_BOOST_SOURCE)/libs/locale/src holds no Boost.Locale"
        "sources, and /webcpp//boost_locale is built from them: fill the tree from"
        "Boost's source archive (`bash tools/ci/actions/boost/install.sh source <dir>`)" ;
}

if [ path.exists $(WEBCPP_BOOST_SOURCE)/libs/locale/src/shared/generator.cpp ]
{
    lib boost_locale : libs/locale/src/$(.locale-shared).cpp boost_charconv
      : <link>static <define>BOOST_LOCALE_SOURCE <conditional>@locale-configuration
      : : <define>BOOST_LOCALE_NO_LIB=1 <target-os>darwin:<library>iconv ;
}
else
{
    alias boost_locale : : <conditional>@locale-sources-missing ;
}

explicit boost_locale ;
```

  In `Jamroot`, after the alias `boost_url`:

```
# Boost.Charconv and Boost.Locale, compiled, as Boost.URL is; what Boost.Locale
# gives on each target is AGENTS.md chapter 2's table.
alias boost_charconv : tools/boost//boost_charconv : <conditional>@boost-source-usable ;
alias boost_locale : tools/boost//boost_locale : <conditional>@boost-source-usable ;
```

  and the last `explicit` gains `boost_charconv boost_locale`.

- [ ] **Step 4: Run the tests, to see them pass.**

Run: `export EM_CACHE=~/dev/webcpp/.local/emscripten-cache; python3 tools/test/boost_libraries_test.py`
Expected: eleven `<case>: ok` lines, then `boost_libraries_test: ok`. A
target that does not build or run as the table says (a backend list other
than `posix std`, a link that needs a library the table does not name) stops
the task: the controller gets the error and the log, and nothing is changed
to hide it.

Run: `python3 tools/lint/lint_test.py test_a_compiled_boost_library_s_sources_are_not_analysed`
Expected: `ok` (Boost.Locale's sources come from the same tree, outside the
one the lint reads).

- [ ] **Step 5: The texts.**
  - `AGENTS.md` chapter 2, checklist item 4, after the sentence on
    `<library>/webcpp//boost_url`: "Boost.Charconv and Boost.Locale likewise,
    `<library>/webcpp//boost_charconv` and `<library>/webcpp//boost_locale`:
    Boost.Locale without ICU, with what it gives on each target in this
    table:", followed by the table of this task's "What each target provides".
  - `AGENTS.md` chapter 13, the bullet "Compiled Boost libraries" becomes:
    "**Compiled Boost libraries.** Boost.URL, Boost.Charconv and Boost.Locale
    are built from Boost's sources (`/webcpp//boost_url`,
    `/webcpp//boost_charconv`, `/webcpp//boost_locale`). Boost.Locale has no
    build for emscripten, which is single-threaded, nor without exceptions,
    and it is not yet measured on Windows. A Boost.Test suite still compiles
    the framework's header-only form (chapters 2 and 9)."

- [ ] **Step 6: Verify.** The green of a superproject change, items 1 to 6.
  Expected: as stated there.

- [ ] **Step 7: Commit.**

```bash
git -c user.name=R -c user.email=github.overboard312@passmail.net commit \
  -m "tools: Boost.Charconv and Boost.Locale built from Boost's sources, as /webcpp//boost_charconv and /webcpp//boost_locale" \
  -- tools/boost/Jamfile Jamroot tools/test/boost_libraries_test.py AGENTS.md
```

**Changelog**, Tools and build:
- Added `/webcpp//boost_charconv`, Boost.Charconv's compiled library built
  from Boost's sources for every target.
- Added `/webcpp//boost_locale`, Boost.Locale's compiled library without ICU,
  built from Boost's sources natively, on wasip2 and on wasip3 (posix and std
  backends with iconv; winapi and std on Windows); it has no emscripten build
  and needs exceptions.

---

### Task 4: `webcpp.data`, test data on WASI

**Files:**
- Modify: `tools/webcpp.jam` (`import toolset ;`, a feature and two flags
  after `feature.feature webcpp-lane ...`, a rule after `run-fail`, its header
  comment), `tools/test/webcpp_jam_test.py`, `third_party/Jamfile`,
  `tools/test/third_party_test.py`, `tools/test/boost_libraries_test.py`,
  `AGENTS.md`

**Interfaces:**
- Consumes: Task 1's `third_party/Jamfile` and its `JSON_SCHEMA_TEST_SUITE`;
  Task 3B's `LOCALE_JAMFILE` and `TRANSLATES_ON_WASI`.
- Produces, the interface this plan defines for every consumer (section
  "Consumer interface" holds the lines they copy):
  - the rule `rule data ( directories + )` in `tools/webcpp.jam`, called as
    `[ webcpp.data <directory> + ]` inside a `webcpp.run`'s or
    `webcpp.run-fail`'s requirements (or a target's usage requirements); each
    directory absolute, as a `path-constant` gives it; the program reads it at
    the same path on every target; on wasip2 and wasip3 the command becomes
    `wasmtime --dir="<directory>" <program>`; a relative directory stops the
    build with `webcpp.data: <directory> is not absolute`; nothing for
    `webcpp.example` or `webcpp.boost-test`;
  - the free feature `webcpp-data`, whose values are wasmtime options,
    written only by `webcpp.data`; no feature `webcpp-dir` exists (json_schema
    writes `[ webcpp.data $(X) ]` where its plan wrote `<webcpp-dir>$(X)`);
  - `/webcpp/third_party//json-schema-test-suite` now reaches wasip2 and
    wasip3 tests;
  - Boost.Locale's catalog test runs on wasip2 and wasip3 too.

- [ ] **Step 1: Write the failing tests.** In `tools/test/webcpp_jam_test.py`,
  before `CASES`, and the three cases added to `CASES` after
  `test_no_variant_is_built_without_rtti`:

```python
# A test that reads a file of a directory whose name holds a space, which webcpp.data gives it.
DATA_JAMFILE = r'''import regex ;
import webcpp ;

webcpp.targets native wasip2 wasip3 ;

path-constant DATA : "the data" ;
local data = [ regex.replace $(DATA) "\\\\" "/" ] ;
webcpp.run reads_data : reads_data.cpp
  : <define>"\"WEBCPP_TEST_DATA=\\\"$(data)\\\"\"" [ webcpp.data $(DATA) ] ;
'''

READS_DATA_SOURCE = r'''#include <fstream>
#include <iostream>
#include <string>

int main() {
    std::ifstream file(WEBCPP_TEST_DATA "/answer.txt");
    std::string line;
    if (!std::getline(file, line)) {
        std::cout << "cannot read " WEBCPP_TEST_DATA "/answer.txt\n";
        return 1;
    }
    std::cout << line << '\n';
    return line == "42" ? 0 : 1;
}
'''


def add_data_reader(root, jamfile=DATA_JAMFILE):
    harness.add_library(root, 'data_reader', jamfile, {'reads_data.cpp': READS_DATA_SOURCE})
    (root / 'libs/data_reader/test/the data').mkdir()
    (root / 'libs/data_reader/test/the data/answer.txt').write_text('42\n')


def test_a_test_reads_its_data_on_every_target(root):
    add_data_reader(root)
    for request in ((), WASIP2, WASIP3):
        result = harness.run_b2(root, '-a', '-d+2', *request, 'libs/data_reader/test')
        harness.expect(result, True)
        assert passed(result) == {'reads_data'}, (request, passed(result), result.stdout[-4000:])
        assert output_of(root, 'reads_data.output').startswith('42\n'), request
        # wasmtime is given the directory after its own name, quoted whole; natively nothing is.
        given = re.search(r'wasmtime --dir="[^"]*/the data" ', result.stdout)
        assert bool(given) == bool(request), (request, result.stdout[-4000:])
        shutil.rmtree(root / 'bin')


def test_without_its_data_a_wasi_test_reads_nothing(root):
    # The reason webcpp.data exists: natively the program reads the host's files, under wasmtime
    # it sees none it was not given.
    add_data_reader(root, DATA_JAMFILE.replace(' [ webcpp.data $(DATA) ]', ''))
    harness.expect(harness.run_b2(root, '-a', 'libs/data_reader/test'), True)
    shutil.rmtree(root / 'bin')
    result = harness.run_b2(root, '-a', *WASIP2, 'libs/data_reader/test')
    harness.expect(result, False)
    assert output_of(root, 'reads_data.output').startswith('cannot read '), result.stdout[-4000:]


def test_data_must_be_absolute(root):
    add_data_reader(root, DATA_JAMFILE.replace('[ webcpp.data $(DATA) ]', '[ webcpp.data data ]'))
    harness.expect(harness.run_b2(root, 'libs/data_reader/test'), False,
                   'webcpp.data: data is not absolute')
```

  And in `tools/test/third_party_test.py`, a case added to `CASES` after
  `test_the_suite_reaches_a_test_on_emscripten`, with the constants
  `WASIP2 = ('toolset=clang-wasip2', 'testing.launcher=wasmtime')` and
  `WASIP3 = ('toolset=clang-wasip3', 'testing.launcher=wasmtime')` beside
  `EMSCRIPTEN`, `import shutil` among the imports, and "and on wasip2 and wasip3," added to the docstring after
  "natively and on emscripten":

```python
def test_the_suite_reaches_a_test_on_wasi(root: Path) -> None:
    add_reader(root)
    for request in (WASIP2, WASIP3):
        result = harness.run_b2(root, '-a', *request, 'libs/suite_reader/test')
        harness.expect(result, True)
        assert passed(result) == {'reads_suite'}, (request, passed(result), result.stdout[-4000:])
        assert output_of(root, 'reads_suite.output').startswith('read the suite\n'), request
        shutil.rmtree(root / 'bin')
```

- [ ] **Step 2: Run them, to see them fail.**

Run: `python3 tools/test/webcpp_jam_test.py test_a_test_reads_its_data_on_every_target test_without_its_data_a_wasi_test_reads_nothing`
Expected: the first FAILS on an unknown rule `webcpp.data`; the second passes
already (it pins the reason).

Run: `python3 tools/test/third_party_test.py test_the_suite_reaches_a_test_on_wasi`
Expected: FAIL, the wasip2 run's output `cannot read <suite directory>`.

  In `tools/test/boost_libraries_test.py`, Boost.Locale's catalog test joins
  wasip2 and wasip3: `TRANSLATES_ON_WASI = False` becomes
  `TRANSLATES_ON_WASI = True`, the comment above `LOCALE_JAMFILE` becomes
  "# translates reads its catalog on every target, from the directory
  webcpp.data gives it.", and `LOCALE_JAMFILE`'s last two lines

```
  : <library>/webcpp//boost_locale <define>"\"WEBCPP_TEST_CATALOGS=\\\"$(catalogs)\\\"\""
  : native ;
```

  become

```
  : <library>/webcpp//boost_locale <define>"\"WEBCPP_TEST_CATALOGS=\\\"$(catalogs)\\\"\""
    [ webcpp.data $(CATALOGS) ] ;
```

Run: `python3 tools/test/boost_libraries_test.py test_boost_locale_on_native_wasip2_and_wasip3`
Expected: FAIL on the unknown rule `webcpp.data`.

- [ ] **Step 3: The rule.** In `tools/webcpp.jam`, add `import toolset ;` to
  the imports, and after `feature.feature webcpp-lane : : free incidental ;`:

```
# What a test that reads the host's files at run time gets under wasmtime:
# each value is wasmtime's option --dir="<directory>", which preopens that
# directory of the host at the same path in the program, and webcpp.data
# writes it, on WASI alone. testing.jam's capture-output, the action of
# webcpp.run and webcpp.run-fail, puts the values after testing.launcher's
# own words, so that the command is `wasmtime --dir="<directory>" <program>`.
# Natively and on emscripten (-sNODERAWFS=1) a program reads the host's
# files already.
feature.feature webcpp-data : : free incidental ;
toolset.flags testing.capture-output LAUNCHER : <webcpp-data> : unchecked ;
toolset.uses-features testing.capture-output : <webcpp-data> : unchecked ;
```

  after the rule `run-fail`:

```
# The requirements, or usage requirements, that give a program of webcpp.run
# or webcpp.run-fail the directories at run time, each absolute, as a
# path-constant gives it: the program reads them at the same path on every
# target. Quoted whole, for a path that holds a space.
rule data ( directories + )
{
    local result ;
    for local directory in $(directories)
    {
        if ! [ path.is-rooted [ path.make $(directory) ] ]
        {
            errors.user-error "webcpp.data: $(directory) is not absolute; give the directory"
                "as a path-constant gives it" ;
        }
        result += <target-os>wasi:<webcpp-data>"--dir=\"$(directory)\"" ;
    }
    return $(result) ;
}
```

  and the header comment's list of rules gains, after `webcpp.link`:
  `#     webcpp.data directories + ;  (requirements: a run's data on WASI)`.

  In `third_party/Jamfile`, `import webcpp ;` joins the imports, the suite's
  comment says "... and runs again when one of its files changes; under
  wasmtime the directory is given to it (webcpp.data).", and the alias's
  usage requirements gain `[ webcpp.data $(JSON_SCHEMA_TEST_SUITE) ]`:

```
alias json-schema-test-suite : : <conditional>@suite-checked-out : :
    <define>"\"WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE=\\\"$(.suite)\\\"\""
    [ webcpp.data $(JSON_SCHEMA_TEST_SUITE) ]
    <dependency>$(.suite-files) ;
```

- [ ] **Step 4: Run the tests, to see them pass.**

Run: `python3 tools/test/webcpp_jam_test.py`
Expected: every case `ok`, then `webcpp_jam_test: ok`.

Run: `python3 tools/test/third_party_test.py`
Expected: six `<case>: ok` lines, then `third_party_test: ok`.

Run: `python3 tools/test/boost_libraries_test.py test_boost_locale_on_native_wasip2_and_wasip3`
Expected: `ok` (measured: `Olá Mundo|café` on wasip2 and wasip3).

- [ ] **Step 5: The texts.** `AGENTS.md` chapter 9:
  - the Jamfile API's code block gains the line
    `webcpp.data         <directory> + ;   (requirements)` and its table the row:

```
| `webcpp.data` | | requirements, or usage requirements, that give a program of `webcpp.run` or `webcpp.run-fail` the absolute directories, each read at the same path on every target: on wasip2 and wasip3 wasmtime preopens it (`wasmtime --dir="<directory>" <program>`); natively and on emscripten (`-sNODERAWFS=1`) the program reads the host's files already. A relative directory stops the build |
```

  - "Tests" gains the bullet: "- A test that reads files at run time is given
    their directory by a `path-constant`, a `WEBCPP_TEST_*` define with
    forward slashes, quoted whole, and `[ webcpp.data <directory> ]` in its
    requirements; `/webcpp/third_party//json-schema-test-suite` gives all
    three for the JSON Schema Test Suite."

- [ ] **Step 6: Verify.** The green of a superproject change, items 1 to 6.
  Expected: as stated there.

- [ ] **Step 7: Commit.**

```bash
git -c user.name=R -c user.email=github.overboard312@passmail.net commit \
  -m "tools: webcpp.data gives a WASI test a directory of the host's, and the JSON Schema Test Suite reaches wasip2 and wasip3" \
  -- tools/webcpp.jam tools/test/webcpp_jam_test.py third_party/Jamfile \
     tools/test/third_party_test.py tools/test/boost_libraries_test.py AGENTS.md
```

**Changelog**, Tools and build:
- Added `webcpp.data`, which gives a test a directory of the host's files on
  wasip2 and wasip3 (wasmtime's `--dir`), as natively and on emscripten.
- Changed `/webcpp/third_party//json-schema-test-suite` to reach tests on
  wasip2 and wasip3.

---

### Task 5: The SRELL mirror, `third_party/libs/srell` and `srell`

**Files:**
- Create (repository, its three commits): `third_party/libs/srell`, the local
  mirror, and its bare clone `~/.buzz/REPOS/srell.git`
- Create: `~/.buzz/RESEARCH/SRELL_ALLOCATION_SPIKE/make_patch.py`,
  `~/.buzz/RESEARCH/SRELL_ALLOCATION_SPIKE/edits.py`; append a section to
  `~/.buzz/RESEARCH/SRELL_ALLOCATION_SPIKE.md`
- Modify: `.gitmodules`, `third_party/Jamfile`,
  `tools/test/third_party_test.py`, `AGENTS.md`, `README.md`

**Interfaces:**
- Consumes: `third_party/Jamfile` (Task 1).
- Produces:
  - the mirror's `main`: (1) SRELL 2026.07's archive as it is, (2) the
    no-exceptions patch (`SRELL_NO_THROW` defined where the compiler has no
    exceptions, `re_detail::throw_bad_alloc()`), (3) the allocator patch
    (`srell::memory_resource`, `explicit basic_regex(memory_resource *)`,
    `basic_regex::get_memory_resource()`, a pmr `match_results` taking its
    allocator's resource);
  - `/webcpp/third_party//srell`, an explicit alias whose usage requirements
    are `<include>third_party/libs/srell` as a system directory,
    `<define>SRELL_NO_UBMOD`, and `<exception-handling>off:<define>SRELL_NO_THROW`;
    a program includes `<srell.hpp>`;
  - the submodule `third_party/libs/srell`, URL `../srell.git`
    (`webcpporg/srell`, which only Task 7 creates).

- [ ] **Step 1: Write the failing tests.** In `tools/test/third_party_test.py`,
  the docstring gains "; SRELL compiles under the project's warnings, matches
  with SRELL_NO_UBMOD and through a memory resource on every target, reports
  errors as values without exceptions, and, not checked out, stops a build
  that needs it", and before `CASES`:

```python
SRELL = 'third_party/libs/srell'

REGEX_JAMFILE = ('import webcpp ;\n'
                 '\n'
                 'webcpp.targets native wasip2 wasip3 emscripten ;\n'
                 '\n'
                 'webcpp.run matches : matches.cpp : <library>/webcpp/third_party//srell ;\n'
                 'webcpp.run without_exceptions : without_exceptions.cpp\n'
                 '  : <library>/webcpp/third_party//srell <exception-handling>off ;\n')

MATCHES_SOURCE = r'''#include <srell.hpp>

#include <cstddef>
#include <cstdio>
#include <memory_resource>
#include <string_view>

namespace {

// Counts what a regex and its results ask of it, and passes each request on.
class counting_resource : public std::pmr::memory_resource {
public:
    std::size_t calls = 0;

private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        ++calls;
        return std::pmr::new_delete_resource()->allocate(bytes, alignment);
    }
    void do_deallocate(void* pointer, std::size_t bytes, std::size_t alignment) override {
        std::pmr::new_delete_resource()->deallocate(pointer, bytes, alignment);
    }
    bool do_is_equal(std::pmr::memory_resource const& other) const noexcept override {
        return this == &other;
    }
};

}  // namespace

int main() {
    // SRELL's operator| on syntax_option_type is not constexpr.
    auto const flags = srell::regex_constants::ECMAScript | srell::regex_constants::quiet;
    std::string_view const subject = "h\xc3\xa9llo";
    srell::u8cregex const letters("^\\p{Letter}+$", flags);
    srell::u8ccmatch found;
    bool const matched =
        srell::regex_search(subject.data(), subject.data() + subject.size(), found, letters);
    // SRELL_NO_UBMOD: a leading (?i) is no ECMAScript, and does not compile.
    srell::u8cregex const modifier("(?i)abc", flags);
    counting_resource resource;
    srell::u8cregex through(&resource);
    through.assign("^\\p{Letter}+$", flags);
    srell::match_results<char const*, std::pmr::polymorphic_allocator<srell::csub_match>> results(
        &resource);
    bool const matched_through =
        srell::regex_search(subject.data(), subject.data() + subject.size(), results, through);
    std::printf("matched %d, modifier refused %d, through a resource %d, calls %d\n", matched,
                modifier.ecode() != 0, matched_through, resource.calls > 0);
    return matched && modifier.ecode() != 0 && matched_through && resource.calls > 0 ? 0 : 1;
}
'''

WITHOUT_EXCEPTIONS_SOURCE = r'''#ifndef SRELL_NO_THROW
#error "the srell target defines SRELL_NO_THROW in a build without exceptions"
#endif

#include <srell.hpp>

#include <cstdio>

int main() {
    srell::u8cregex const broken("^(abc]");
    srell::u8cregex const runaway("(a*)*b");
    srell::u8ccmatch results;
    char const subject[] = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    bool const found = srell::regex_search(subject, subject + sizeof subject - 1, results, runaway);
    std::printf("broken %d, runaway found %d, ecode %d\n", broken.ecode(), found, results.ecode());
    return broken.ecode() != 0 && !found && results.ecode() != 0 ? 0 : 1;
}
'''


def add_regexes(root: Path) -> None:
    """Adds the library regexes, whose tests include <srell.hpp>."""
    harness.add_library(root, 'regexes', REGEX_JAMFILE,
                        {'matches.cpp': MATCHES_SOURCE,
                         'without_exceptions.cpp': WITHOUT_EXCEPTIONS_SOURCE})


def test_srell_matches_on_every_target(root: Path) -> None:
    # The project's -Wall -Wextra -Werror holds: SRELL's own warnings (an unused variable in
    # srell.hpp) are not ours, its headers being system ones.
    harness.link_emsdk(root)
    add_regexes(root)
    gcc = harness.gcc_request(root)
    for request in ((), WASIP2, WASIP3, EMSCRIPTEN, *((gcc,) if gcc else ())):
        result = harness.run_b2(root, '-a', *request, 'libs/regexes/test')
        harness.expect(result, True)
        assert passed(result) == {'matches', 'without_exceptions'}, (
            request, passed(result), result.stdout[-4000:])
        assert output_of(root, 'matches.output').startswith(
            'matched 1, modifier refused 1, through a resource 1, calls 1\n'), request
        shutil.rmtree(root / 'bin')


def test_srell_not_checked_out_is_refused(root: Path) -> None:
    harness.third_party_of_its_own(root, 'srell', checked_out=False)
    add_regexes(root)
    result = harness.run_b2(root, 'libs/regexes/test')
    harness.expect(result, False, 'third_party/libs/srell is not checked out',
                   'git submodule update --init third_party/libs/srell')
    harness.expect(harness.run_b2(root, 'libs/demo/test//pass'), True)
```

  and the two cases at the end of `CASES`.

- [ ] **Step 2: Run them, to see them fail.**

Run: `python3 tools/test/third_party_test.py test_srell_matches_on_every_target`
Expected: FAIL, `error: Unable to find file or target named` `'/webcpp/third_party//srell'`.

- [ ] **Step 3: The mirror's first commit: the archive as it is.**

```bash
W=~/.buzz/.scratch/srell-mirror; mkdir -p "$W"
tools/ci/download.sh https://www.akenotsuki.com/misc/srell/releases/srell2026_07.zip \
  d384feae20c66b4d65bc240ee2a851aed26ca24dc4d73503eee3e81e238ec20e "$W/srell2026_07.zip"
git init -q -b main third_party/libs/srell
unzip -q "$W/srell2026_07.zip" -d third_party/libs/srell
cat > third_party/libs/srell/README.md <<'README'
# SRELL, as webcpp uses it

A mirror of SRELL (std::regex-like library) by Nozomu Katoo,
https://www.akenotsuki.com/misc/srell/en/, as webcpp
(https://github.com/webcpporg) uses it. SRELL's licence is its own, in
`license.txt` (BSD-2-Clause).

- The first commit imports SRELL 2026.07's release archive exactly:
  https://www.akenotsuki.com/misc/srell/releases/srell2026_07.zip, SHA-256
  d384feae20c66b4d65bc240ee2a851aed26ca24dc4d73503eee3e81e238ec20e, its 15
  files unchanged, and this README.
- Each later commit patches `srell.hpp` alone, and is offered upstream:
  compiling without exceptions, then routing every allocation through a
  `memory_resource` given at construction.
- `single-header/srell.hpp` stays as imported: it is unpatched, and webcpp
  does not use it. Include `<srell.hpp>`.

A new SRELL release is imported the same way, and the patches are rebased
onto it.
README
git -C third_party/libs/srell add -- README.md history_en.txt history_ja.txt license.txt misc \
  readme_en.txt readme_ja.txt single-header srell.hpp srell_ucfdata2.h srell_updata3.h unicode
git -C third_party/libs/srell status --short | grep -c '^A  '
git -C third_party/libs/srell status --short | grep -c '^??' || true
git -C third_party/libs/srell -c user.name=R -c user.email=github.overboard312@passmail.net commit -q \
  -m "SRELL 2026.07, imported from its release archive" \
  -m "The archive https://www.akenotsuki.com/misc/srell/releases/srell2026_07.zip, SHA-256 d384feae20c66b4d65bc240ee2a851aed26ca24dc4d73503eee3e81e238ec20e, unpacked as it is: its 15 files, unchanged, and the mirror's README.md, which says that single-header/srell.hpp stays unpatched and is not used by webcpp."
git -C third_party/libs/srell ls-files | wc -l
```

Expected: `16`, then `0` (nothing untracked), then the commit, then `16`.

- [ ] **Step 4: The second commit: compile without exceptions** (formats spec
  section 8.4). Apply it with this script, which refuses a base it does not
  recognise:

```bash
python3 - third_party/libs/srell/srell.hpp <<'EOF'
import sys
from pathlib import Path

path = Path(sys.argv[1])
text = path.read_text()
guard = ('//  A compiler without exceptions cannot compile a throw: SRELL then reports\n'
         '//  errors as values (SRELL_NO_THROW), and an allocation that fails ends the\n'
         '//  program (re_detail::throw_bad_alloc).\n'
         '#if !defined(__cpp_exceptions) && !defined(_CPPUNWIND) && !defined(SRELL_NO_THROW)\n'
         '#define SRELL_NO_THROW\n'
         '#endif\n'
         '\n')
anchor = '\nnamespace srell\n{\n'
assert text.count(anchor) == 1, text.count(anchor)
text = text.replace(anchor, '\n' + guard + 'namespace srell\n{\n')
memory = '//  ["rei_memory.hpp" ...\n\n\tnamespace re_detail\n\t{\n'
assert text.count(memory) == 1, text.count(memory)
helper = ('\n//  Raises std::bad_alloc where the compiler has exceptions, and ends the program\n'
          '//  where it has none.\n'
          'inline void throw_bad_alloc()\n'
          '{\n'
          '#if defined(__cpp_exceptions) || defined(_CPPUNWIND)\n'
          '\tthrow std::bad_alloc();\n'
          '#else\n'
          '\tstd::abort();\n'
          '#endif\n'
          '}\n')
head, _, tail = text.partition(memory)
assert tail.count('throw std::bad_alloc();') == 4, tail.count('throw std::bad_alloc();')
path.write_text(head + memory + helper + tail.replace('throw std::bad_alloc();', 'throw_bad_alloc();'))
EOF
git -C third_party/libs/srell diff --stat
```

Expected: `srell.hpp | 26 ++++++++++++++++++++++----` and
`1 file changed, 22 insertions(+), 4 deletions(-)`. Then check both builds
(the probe of the spike, `JSON_SCHEMA_ENGINE_SPIKE.md` section 3):

```bash
cat > "$W/probe.cpp" <<'EOF'
#include <srell.hpp>
#include <cstdio>
int main() {
    srell::u8cregex bad("^(abc]", srell::regex_constants::ECMAScript | srell::regex_constants::quiet);
    srell::u8cregex re("(a*)*b");
    srell::u8ccmatch m;
    const char subject[] = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    bool found = srell::regex_search(subject, subject + sizeof subject - 1, m, re);
    std::printf("bad=%d found=%d ecode=%d\n", bad.ecode(), found, m.ecode());
}
EOF
env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH clang++ -std=c++20 -fno-exceptions -DSRELL_NO_UBMOD \
  -Ithird_party/libs/srell "$W/probe.cpp" -o "$W/probe_noexc" && "$W/probe_noexc"
env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH clang++ -std=c++20 -DSRELL_NO_UBMOD \
  -Ithird_party/libs/srell "$W/probe.cpp" -o "$W/probe_exc" && { "$W/probe_exc"; echo "status $?"; }
```

Expected: `bad=104 found=0 ecode=111`; then
`libc++abi: terminating due to uncaught exception of type srell::regex_error: error_complexity`
and `status 134` (with exceptions, SRELL throws as before).

```bash
git -C third_party/libs/srell -c user.name=R -c user.email=github.overboard312@passmail.net commit -q \
  -m "srell.hpp: compile without exceptions" \
  -m "A compiler without exceptions (__cpp_exceptions and _CPPUNWIND undefined) cannot compile a throw. srell.hpp now defines SRELL_NO_THROW there, unless it is defined already, so that a regex_error is reported as a value, and each of the four \`throw std::bad_alloc();\` becomes re_detail::throw_bad_alloc(), which throws where exceptions exist and calls std::abort() where they do not. Nothing changes in a build with exceptions. single-header/srell.hpp is left as imported." \
  -- srell.hpp
```

- [ ] **Step 5: The third commit: every allocation through a memory
  resource.** The spike's generator, rebased on the second commit: its
  `edits.py` matched the spike's unconditional `std::abort();` at the five
  places that are now `throw_bad_alloc();`.

```bash
P=~/.buzz/RESEARCH/SRELL_ALLOCATION_SPIKE/mirror; S=~/.buzz/RESEARCH/SRELL_ALLOCATION_SPIKE/spike
mkdir -p "$P/orig" "$P/patched"
cp "$S/make_patch.py" "$S/edits.py" "$P/"
sed -i '' 's/std::abort();/throw_bad_alloc();/' "$P/edits.py"
grep -c 'throw_bad_alloc();' "$P/edits.py"
cp third_party/libs/srell/srell.hpp "$P/orig/srell.hpp"
(cd "$P" && python3 make_patch.py)
cp "$P/patched/srell.hpp" third_party/libs/srell/srell.hpp
rm -rf "$P/orig" "$P/patched"
git -C third_party/libs/srell diff --stat
```

Expected: `5`; then `re_compiler locals: 30 default-constructed, 4 with arguments`
and `51 edits applied`; then `1 file changed, 316 insertions(+), 106 deletions(-)`.

  Check it against the spike's harness and SRELL's conformance test: zero
  allocations outside the resource in every phase, identical results.

```bash
cd "$W"
M=~/dev/webcpp-infra/third_party/libs/srell
env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH clang++ -std=c++20 -O2 -fno-exceptions -DSRELL_NO_UBMOD \
  -DSPIKE_PMR -I"$M" "$S/count.cpp" -o count_pmr
./count_pmr > count_pmr.txt
grep -c ': 0 calls 0 bytes' count_pmr.txt
diff <(grep RESULT "$S/out_orig.txt") <(grep RESULT count_pmr.txt) && echo "results identical"
env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH clang++ -std=c++20 -O1 -g -fsanitize=address,undefined \
  -DSPIKE_PMR -I"$M" -I"$S" "$S/conftest/conftest.cpp" -o conf_pmr_asan
for t in utf8 utf8c utf16 utf32; do ./conf_pmr_asan $t > conf_$t.txt && cmp conf_$t.txt "$S/conf_orig_$t.txt" && echo "conftest $t identical"; done
SDK=~/dev/webcpp-infra/.local/wasi-sdk/bin
env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH "$SDK/clang++" --target=wasm32-wasip2 -fno-exceptions \
  -std=c++20 -O2 -DSPIKE_PMR -I"$M" -I"$S" "$S/conftest/conftest.cpp" -o conf_pmr_p2.wasm
for t in utf8 utf8c utf16 utf32; do wasmtime run conf_pmr_p2.wasm $t > conf_p2_$t.txt && cmp <(grep -v TestType "$S/conf_orig_$t.txt") <(grep -v TestType conf_p2_$t.txt) && echo "conftest $t identical on wasip2"; done
cd ~/dev/webcpp-infra
```

Expected: `45`, `results identical`, four `conftest <type> identical` lines
with no sanitizer report, and four `conftest <type> identical on wasip2`
lines (the mirror's own commit measured on wasip2, which the upstream draft
then cites).

```bash
git -C third_party/libs/srell -c user.name=R -c user.email=github.overboard312@passmail.net commit -q \
  -m "srell.hpp: every allocation through a memory_resource given at construction" \
  -m "basic_regex gains an explicit constructor taking a memory_resource pointer (std::pmr::memory_resource where <memory_resource> exists, an abstract class of SRELL's own otherwise), and every allocation of the regex, and of a match_results whose allocator is a std::pmr::polymorphic_allocator, goes through it: the compiled program, the character classes, the Boyer-Moore-Horspool tables, and the search state with its backtracking stack. A null resource keeps malloc, realloc and free, so nothing changes for existing code. The internal containers take the resource without a default, so that a container a later release adds fails to compile rather than fall back to malloc. get_allocator() now returns the vector's allocator. single-header/srell.hpp is left as imported." \
  -- srell.hpp
git -C third_party/libs/srell log --format='%h %s'
```

Expected: three lines, newest first: the memory_resource commit, "srell.hpp:
compile without exceptions", "SRELL 2026.07, imported from its release
archive".

  Append to `~/.buzz/RESEARCH/SRELL_ALLOCATION_SPIKE.md` a section "## The
  mirror's copy of the generator (2026-10-09)": `make_patch.py` and
  `edits.py` in `~/.buzz/RESEARCH/SRELL_ALLOCATION_SPIKE/mirror/` wrote the third
  commit of the mirror from its second; `edits.py` names `throw_bad_alloc();`
  where the spike's base had `std::abort();`; the measured counts above.

- [ ] **Step 6: The mirror's local origin, and the submodule.**

```bash
git clone -q --bare third_party/libs/srell ~/.buzz/REPOS/srell.git
git submodule add ../srell.git third_party/libs/srell
git submodule absorbgitdirs third_party/libs/srell
git ls-files -s third_party/libs/srell
```

Expected: `Adding existing repo at 'third_party/libs/srell' to the index`,
then `160000 <the third commit> 0	third_party/libs/srell`, and `.gitmodules`
holds:

```
[submodule "third_party/libs/srell"]
	path = third_party/libs/srell
	url = ../srell.git
```

- [ ] **Step 7: The target.** In `third_party/Jamfile`, the header comment
  names SRELL too, and after the suite's alias:

```
# SRELL 2026.07 (BSD-2-Clause), ECMAScript regular expressions in a header,
# from the mirror webcpporg/srell: the release archive, then a commit that
# compiles it without exceptions, then one that routes every allocation
# through a memory_resource. A program includes <srell.hpp>, whose headers
# are system ones, so that their warnings are not ours, as Boost's are in
# the Jamroot. Every program sees one SRELL: SRELL_NO_UBMOD (a leading (?i)
# is no ECMAScript) always, and SRELL_NO_THROW in a build without
# exceptions, given here and never in a header of webcpp's.
path-constant SRELL : libs/srell ;

rule srell-checked-out ( properties * )
{
    if ! [ path.exists $(SRELL)/srell.hpp ]
    {
        errors.user-error "third_party/libs/srell is not checked out: it holds no"
            "srell.hpp. Check it out with `git submodule update --init"
            "third_party/libs/srell`." ;
    }
}

alias srell : : <conditional>@srell-checked-out : :
    <include>$(SRELL)
    <toolset>gcc:<cxxflags>"-isystem\"$(SRELL)\""
    <toolset>clang:<cxxflags>"--system-header-prefix=\"srell\""
    <toolset>emscripten:<cxxflags>"--system-header-prefix=\"srell\""
    <toolset>msvc:<cxxflags>"\"/external:I$(SRELL)\""
    <define>SRELL_NO_UBMOD
    <exception-handling>off:<define>SRELL_NO_THROW ;

explicit srell ;
```

- [ ] **Step 8: Run the tests, to see them pass.**

Run: `export EM_CACHE=~/dev/webcpp/.local/emscripten-cache; python3 tools/test/third_party_test.py`
Expected: eight `<case>: ok` lines, then `third_party_test: ok`. If SRELL does
not build or run on emscripten (not measured before), stop and report it to
the controller with the log.

- [ ] **Step 9: The texts.**
  - `AGENTS.md` chapter 2, "Third-party code", the table gains:

```
| `srell` | SRELL 2026.07 from the mirror `webcpporg/srell`: the release archive, a commit that compiles it without exceptions, and one that routes every allocation through a `memory_resource` given at construction, both offered upstream. A program includes `<srell.hpp>`, a system header; every program gets `SRELL_NO_UBMOD`, and `SRELL_NO_THROW` without exceptions | BSD-2-Clause |
```

    and the paragraph gains: "A mirror under `webcpporg` holds a third party
    that webcpp patches: its first commit imports a release exactly, each
    patch is a commit of its own, offered upstream, and moving to a new
    release rebases the patches."
  - `README.md`, "License": "... the JSON Schema Test Suite is MIT, and SRELL
    is BSD-2-Clause."

- [ ] **Step 10: Verify.** The green of a superproject change, items 1 to 6.
  Expected: as stated there.

- [ ] **Step 11: Commit** in the superproject (the mirror's three commits are
  made above).

```bash
git -c user.name=R -c user.email=github.overboard312@passmail.net commit \
  -m "third_party: SRELL from the mirror webcpporg/srell, as /webcpp/third_party//srell" \
  -- .gitmodules third_party/libs/srell third_party/Jamfile tools/test/third_party_test.py \
     AGENTS.md README.md
```

**Changelog**, Tools and build:
- Added SRELL 2026.07 as `/webcpp/third_party//srell`, from the mirror
  `webcpporg/srell`: the release archive, a patch that compiles it without
  exceptions, and a patch that routes every allocation through a
  `memory_resource` given at construction.

---

### Task 6: The upstream drafts

Drafts only: nothing is filed or sent. Each is a markdown file under
`~/.buzz/OUTBOX/`, without frontmatter.

**Files:**
- Create: `~/.buzz/OUTBOX/BOOST_URL_RELATIVE_REFERENCE_ISSUE.md`,
  `~/.buzz/OUTBOX/SRELL_NO_EXCEPTIONS_PATCH.md`, `~/.buzz/OUTBOX/SRELL_NO_EXCEPTIONS_PATCH.diff`,
  `~/.buzz/OUTBOX/SRELL_MEMORY_RESOURCE_PATCH.md`,
  `~/.buzz/OUTBOX/SRELL_MEMORY_RESOURCE_PATCH.diff`

**Interfaces:**
- Consumes: the mirror's commits (Task 5); Boost 1.92 with its Boost.URL.
- Produces: three drafts the owner reviews, and that the controller files
  only on the owner's go (Task 7, Step 5).

- [ ] **Step 1: Reproduce the Boost.URL bug as the issue will show it.**

```bash
W=~/.buzz/.scratch/boost-url-issue; mkdir -p "$W"
cat > "$W/repro.cpp" <<'EOF'
#include <boost/url/parse.hpp>
#include <iostream>

int main() {
    for (char const* s : {"//example.com:abc/p", "//a@b@example.com/",
                          "http://example.com:abc/p", "http://a@b@example.com/"}) {
        auto r = boost::urls::parse_uri_reference(s);
        std::cout << s << " -> " << (r ? "accepted" : "rejected");
        if (r)
            std::cout << " host=[" << r->encoded_host() << "] port=[" << r->port()
                      << "] userinfo=[" << r->encoded_userinfo() << "] path=["
                      << r->encoded_path() << "]";
        std::cout << "\n";
    }
}
EOF
env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH clang++ -std=c++17 -I/opt/homebrew/opt/boost/include \
  "$W/repro.cpp" -L/opt/homebrew/opt/boost/lib -lboost_url -o "$W/repro"
DYLD_LIBRARY_PATH=/opt/homebrew/opt/boost/lib "$W/repro"
```

Expected:

```
//example.com:abc/p -> accepted host=[example.com] port=[] userinfo=[] path=[abc/p]
//a@b@example.com/ -> accepted host=[b] port=[] userinfo=[a] path=[@example.com/]
http://example.com:abc/p -> rejected
http://a@b@example.com/ -> rejected
```

- [ ] **Step 2: Write `BOOST_URL_RELATIVE_REFERENCE_ISSUE.md`.**

````markdown
# Draft issue for boostorg/url

Before filing: search boostorg/url's issues for "path-abempty", "relative-ref"
and "network-path"; file only if none reports it.

**Title:** parse_uri_reference accepts a network-path reference whose path does not start with "/"

**Body:**

`boost::urls::parse_uri_reference` accepts two network-path references that
RFC 3986 does not allow. With a scheme, the same strings are rejected, as
they should be.

| Input | Boost.URL 1.92.0 | RFC 3986 |
| --- | --- | --- |
| `//example.com:abc/p` | accepted: host `example.com`, empty port, path `abc/p` | invalid: `port = *DIGIT`; and after an authority the path is `path-abempty`, empty or starting with `/` |
| `//a@b@example.com/` | accepted: userinfo `a`, host `b`, path `@example.com/` | invalid: `@` may not appear in `reg-name`, and `@example.com/` is not `path-abempty` |
| `http://example.com:abc/p` | rejected | invalid |
| `http://a@b@example.com/` | rejected | invalid |

RFC 3986 section 4.2:

```
relative-part = "//" authority path-abempty
              / path-absolute
              / path-noscheme
              / path-empty
```

After `//authority`, the path must be `path-abempty`; here the authority
ends early and the rest is taken as a rootless path.

**Reproduction** (Boost 1.92.0, Apple clang 21, macOS 27; the same result on
wasm32-wasip2 with wasi-sdk 34):

```cpp
#include <boost/url/parse.hpp>
#include <iostream>

int main() {
    for (char const* s : {"//example.com:abc/p", "//a@b@example.com/",
                          "http://example.com:abc/p", "http://a@b@example.com/"}) {
        auto r = boost::urls::parse_uri_reference(s);
        std::cout << s << " -> " << (r ? "accepted" : "rejected");
        if (r)
            std::cout << " host=[" << r->encoded_host() << "] port=[" << r->port()
                      << "] userinfo=[" << r->encoded_userinfo() << "] path=["
                      << r->encoded_path() << "]";
        std::cout << "\n";
    }
}
```

Output:

```
//example.com:abc/p -> accepted host=[example.com] port=[] userinfo=[] path=[abc/p]
//a@b@example.com/ -> accepted host=[b] port=[] userinfo=[a] path=[@example.com/]
http://example.com:abc/p -> rejected
http://a@b@example.com/ -> rejected
```

Expected: the first two rejected, as the last two are.

Found by the JSON Schema Test Suite's `uri-reference` format tests ("a
non-numeric port in a network-path reference", "more than one at-sign in the
authority"), which a validator built on `parse_uri_reference` fails until it
rejects these itself.
````

- [ ] **Step 3: Write `SRELL_NO_EXCEPTIONS_PATCH.diff` and its message.** The
  patch is the mirror's second commit as git writes it, never typed by hand:

```bash
git -C third_party/libs/srell format-patch -1 HEAD~1 --stdout > ~/.buzz/OUTBOX/SRELL_NO_EXCEPTIONS_PATCH.diff
grep -E '^ (srell.hpp \||1 file changed)' ~/.buzz/OUTBOX/SRELL_NO_EXCEPTIONS_PATCH.diff
```

Expected: ` srell.hpp | 26 ++++++++++++++++++++++----` and
` 1 file changed, 22 insertions(+), 4 deletions(-)`. Then the message,
`SRELL_NO_EXCEPTIONS_PATCH.md`:

````markdown
# Draft message to SRELL's author: compiling without exceptions

Before sending: the controller confirms how the author takes contributions
(the contact of https://www.akenotsuki.com/misc/srell/en/), and the owner
approves this text.

**Subject:** SRELL 2026.07: a patch to compile with exceptions disabled

Hello,

Thank you for SRELL. We use it as the regular-expression engine of a C++
validator for JSON Schema that also builds for WebAssembly (wasm32-wasip2),
often without exceptions. SRELL_NO_THROW (or `quiet`) already reports
pattern errors and `error_complexity` as values, which is exactly what such a
build needs; what remains are the four `throw std::bad_alloc();` in
`srell.hpp` (2026.07 lines 1055, 1425, 1550 and 1561), which `-fno-exceptions`
rejects at compile time, and the need to define SRELL_NO_THROW by hand.

The attached patch, `SRELL_NO_EXCEPTIONS_PATCH.diff`, against 2026.07's
`srell.hpp`, changes nothing in a build
with exceptions. Without them (`__cpp_exceptions` and `_CPPUNWIND`
undefined) it defines SRELL_NO_THROW unless already defined, and replaces the
four throws with `re_detail::throw_bad_alloc()`, which throws `std::bad_alloc`
where exceptions exist and calls `std::abort()` where they do not. We checked
it with clang 21 natively and with wasi-sdk 34 for wasm32-wasip2, built
without exceptions and run under wasmtime 47: an invalid
pattern gives `ecode() == 104`, a runaway `(a*)*b` gives `error_complexity`,
and with exceptions the behaviour is the same as before. `single-header/srell.hpp`
would need the same change; we did not regenerate it.

We keep this as a commit of a mirror of SRELL (https://github.com/webcpporg/srell,
once published) and would be glad to drop it if SRELL takes it, or anything
like it.

With thanks,
R
````

- [ ] **Step 4: Write `SRELL_MEMORY_RESOURCE_PATCH.diff` and its message.**

```bash
git -C third_party/libs/srell format-patch -1 HEAD --stdout > ~/.buzz/OUTBOX/SRELL_MEMORY_RESOURCE_PATCH.diff
grep -c '^@@' ~/.buzz/OUTBOX/SRELL_MEMORY_RESOURCE_PATCH.diff
```

Expected: a number near the spike's 76 hunks (the base now has the
no-exceptions commit under it). The message below states the line counts
Task 5 Step 5 measured, `+316/-106`.

````markdown
# Draft message to SRELL's author: allocations through a memory_resource

Before sending: the same channel as the no-exceptions patch, after it; the
owner approves this text. Attach `SRELL_MEMORY_RESOURCE_PATCH.diff`.

**Subject:** SRELL 2026.07: routing allocations through a user's memory_resource

Hello,

A second, larger patch, which applies on top of the no-exceptions one. Our
validator lets its user choose where every allocation goes, so that a
validation of untrusted input can be bounded by the allocator it is given.
SRELL allocates with malloc and realloc, and at match time the backtracking
stack grows with the subject: about 16 bytes per subject byte for patterns
such as `^[a-z]+$`, so one search over a 10 MiB string leaves 256 MiB in use.

The attached patch (+316/-106 lines in srell.hpp, the hunks as counted by
`git format-patch`) adds a pointer to a memory resource, given at
construction, and changes nothing when it is null:

- `srell::memory_resource` is `std::pmr::memory_resource` where
  `<memory_resource>` exists (C++17), and an abstract class with the same
  `allocate` and `deallocate` otherwise, so C++98 and C++11 still compile;
- `explicit basic_regex(memory_resource *)`, then `assign(pattern, flags)`;
  `get_memory_resource()`; a copy keeps its resource;
- a `match_results<It, std::pmr::polymorphic_allocator<sub_match<It>>>` gives
  its allocator's resource to its search state, so a pmr match_results needs
  no new API;
- the internal containers (`simple_array`, `simple_stack`, `bitset`,
  `re_bmh`, the character classes, the group-name mapper, the search state,
  the compiler's state) take the resource in their constructors without a
  default, so that a container added later cannot fall back to malloc
  silently; `re_bmh` is constructed in the resource; growth without a
  resource still uses realloc;
- `match_results::get_allocator()` returns the vector's allocator, not a
  default-constructed one (a small bug of its own).

What we measured on this patch (clang 21, macOS; and wasm32-wasip2 under
wasmtime 47): SRELL's own `misc/conftest.cpp` gives byte-identical output to
unpatched SRELL, built with a checking resource under AddressSanitizer and
UndefinedBehaviorSanitizer natively, and on wasm32-wasip2; with a resource, 0
of 45 measured phases (9 patterns; compile, cold and warm matches on 1 KB and
100 KB subjects) allocate anywhere else, with results identical to unpatched
SRELL. In the measurement that led to the patch, the JSON Schema Test Suite's
885 regex tests also gave identical results.
Timings stay within noise, except searches that grow the stack, up to about
15% slower in some runs, since growth copies where realloc extended in
place.

We understand that a template allocator on basic_regex may suit SRELL
better, or that a non-pmr match_results may want a way to take a resource;
we are glad to rework it. `single-header/srell.hpp` would need the same
change; we did not regenerate it.

With thanks,
R
````

- [ ] **Step 5: Check the drafts.** `grep -n -i -E 'veru[s]' ~/.buzz/OUTBOX/BOOST_URL_RELATIVE_REFERENCE_ISSUE.md ~/.buzz/OUTBOX/SRELL_*`
  and `grep -n $'\xe2\x80\x94' ~/.buzz/OUTBOX/BOOST_URL_RELATIVE_REFERENCE_ISSUE.md ~/.buzz/OUTBOX/SRELL_*`.
  Expected: no output from either. Nothing is committed: the drafts are not
  in a repository.

**Changelog:** none (nothing of webcpp changes).

---

### Task 7: Publication of the mirror, and Task 5 into `main` (controller, after the owner's go)

**Files:**
- Create (outward, the owner's go): `github.com/webcpporg/srell`
- Modify: `~/dev/webcpp` `main` (fast-forward)

**Interfaces:**
- Consumes: `~/.buzz/REPOS/srell.git`, branch `infra/shared` with Task 5 at
  its tip, the drafts of Task 6.
- Produces: `webcpporg/srell` public, its `main` the three commits; Task 5
  in `main`.

- [ ] **Step 1: The owner's go,** recorded in the ledger with its date:
  create `webcpporg/srell`, public, and push the mirror's three commits.
- [ ] **Step 2: Create and push.**

```bash
gh repo create webcpporg/srell --public \
  --description "SRELL, ECMAScript regular expressions in a C++ header, as webcpp uses it: the release archive and two patches offered upstream"
git -C ~/.buzz/REPOS/srell.git push git@github.com:webcpporg/srell.git main
gh api repos/webcpporg/srell/commits --jq '.[].commit.message' | head -3
```

Expected: the three subjects, newest first, as Task 5 Step 5 printed them.

- [ ] **Step 3: Integrate Task 5** by the section "Integration", every step,
  with `infra/integrate` the whole of `infra/shared` (step 2's `checkout -B
  infra/integrate infra/shared`, after Tasks 1 to 4 are in M's `main`), when
  its gate holds. Expected: `git -C ~/dev/webcpp submodule status third_party/libs/srell`
  prints the third commit, without a `-` or `+`.
- [ ] **Step 4: Verify in `~/dev/webcpp`:** `python3 tools/test/third_party_test.py`
  prints `third_party_test: ok`. This task pushes nothing of M: M's push is the
  owner's go (config's Task 15, or a later one), never stage 4's Task 14 and
  never this track's.
- [ ] **Step 5: Upstream, on the owner's separate go:** file
  `BOOST_URL_RELATIVE_REFERENCE_ISSUE.md` on boostorg/url after its duplicate
  search, and send the two SRELL drafts by the channel the controller
  confirmed, the no-exceptions one first; record each link or date in the
  ledger.
- [ ] **Step 6: Close the worktree.** The mirror's only non-bare store besides
  `~/.buzz/REPOS/srell.git` is under the worktree's git directory
  (`absorbgitdirs`), so the bare clone is checked first:

```bash
git -C ~/.buzz/REPOS/srell.git log --format=%s main | head -3
git -C ~/dev/webcpp worktree remove --force ~/dev/webcpp-infra
git -C ~/dev/webcpp worktree list
```

  Expected: the three subjects, newest first; then a list without
  `~/dev/webcpp-infra` (`--force`: git refuses to remove a worktree that holds
  submodules without it). Only once every task of this plan is in M's `main`.

---

## Points for the owner

1. **The go for `webcpporg/srell`** (Task 7): a public repository holding
   SRELL's BSD-2-Clause code, its README and two patches. Until then Task 5
   stays on `infra/shared`, and the library tracks reach SRELL from the local
   mirror (section "Consumer interface").
2. **The go for the upstream drafts** (Task 6), and the channel to SRELL's
   author, which nobody has checked.
3. **Boost.Locale has no emscripten build** (measured): three of its sources
   include Boost.Thread's mutex, which Boost.Config refuses without threads,
   and webcpp's emscripten is single-threaded. `/webcpp//boost_locale`
   refuses emscripten by name; json_schema's locale header and its test leave
   emscripten out. The alternatives are the owner's: a threaded emscripten
   variant (`-pthread`, cross-origin isolation in a page), or a patch of
   Boost.Locale, offered upstream.
4. **Boost.Locale and Boost.Charconv on Windows are not measured** (no Windows
   machine): the target follows Boost's own Windows configuration (winapi and
   std backends, no iconv), and the first CI run that builds it is its
   measurement, reported if it fails.
5. **S1 runs Python generators only.** idna's generator is Python; json_schema
   does not use S1 (its generators write fixed paths, its checks are C++
   tests, its `.pot` check in `messages_test` as its spec 16.3 says). A
   program form is possible if the owner wants one rule for both.
6. **The mirror patches `srell.hpp` only**; `single-header/srell.hpp` stays as
   imported, which the mirror's README says (webcpp includes `srell.hpp`, and
   SRELL's tool that writes the single header is not public). The upstream
   drafts say so too.
7. **The DESIGN entry's figure** for the allocator patch, "+281/-106", counts
   only non-blank changed lines; the patch measures +316/-106 (git's count),
   which this plan and the drafts use. The controller corrects the entry.

## Review disposition

The independent review (3 Critical, 6 Important, 15 Minor) and the
controller's rulings on it, one line each:

- C1 (`generated.jam` does not parse): fixed as the review gives it, a `local build-dir` then `path.native $(build-dir)/generated` (Task 2 Step 5).
- C2 (`constexpr` flags in `matches.cpp`): fixed, `auto const flags`, with a comment on why (Task 5 Step 1).
- C3 (integration during stage 4 publishes this track): nothing reaches M's `main` before stage 4's Task 14 has published and config's Tasks 11 and 12 have integrated; the library tracks consume `infra/shared`; nobody pushes M without the owner's go (Architecture, "Consumer interface", "Integration", Task 7 Step 4).
- I1 (S5's spelling): this plan defines it, `[ webcpp.data <directory> + ]`, with no `webcpp-dir` feature; the exact consumer lines and json_schema's Task 0 check are in "Consumer interface" and Task 4's Interfaces; the json_schema plan is patched by the controller.
- I2 (SRELL before Task 7, submodules from GitHub): a consumer worktree is based on `infra/shared`, its submodules taken from the local clones with a one-command `-c submodule.<path>.url` and `protocol.file.allow=always`, measured with git 2.55 ("Consumer interface"); no branch carrying Task 5 is merged into `main` before Task 7.
- I3 (the suite's target had no consumer): `<library>/webcpp/third_party//json-schema-test-suite` and `WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE` are the documented interface, with how a test reaches `/tests` and `/remotes` ("Consumer interface", Task 1 Interfaces); the controller patches the formats and json_schema plans to it.
- I4 (the mirror's README): added to the first commit, saying that `single-header/srell.hpp` is unpatched and not used by webcpp; the commit's message and Step 3's counts follow (16 files).
- I5 (config missing from the conflicts): rows for the Jamroot's `explicit` line, AGENTS.md, README.md, the lint, `pyrightconfig.json`, `.gitmodules`, the pending changelog's retirement and config's no-push window; integration after config, the changelog lines into `doc/releases/0.1.0.adoc` (Integration step 3).
- I6 (the gate and the post-rebase check): the gate is both ledgers showing nothing dispatched, checked just before the rebase; the check after the rebase, and again in M after the fast-forward, runs every lane (native, gcc-15, wasip2, wasip3, emscripten), every own lane, the doc, the lint and every tools test.
- Minor 1 (`status --short` expected output): the step counts staged (`A  `) and untracked (`??`) lines instead.
- Minor 2 (the spike's files in a job directory): copied to `~/.buzz/RESEARCH/SRELL_ALLOCATION_SPIKE/spike/` on 2026-10-09, checked in Task 0 Step 4, read from there in Task 5 Step 5; the mirror's generator copy in `.../mirror/`.
- Minor 3 (the hand-typed hunk): both SRELL drafts attach `git format-patch` output (`SRELL_NO_EXCEPTIONS_PATCH.diff`, `SRELL_MEMORY_RESOURCE_PATCH.diff`); no diff is typed.
- Minor 4 (S1 has no json_schema consumer): the order table and Task 2's Interfaces say idna is its one consumer, to be cross-checked when the IDNA plan exists.
- Minor 5 (`keep_sources` deletes what it is given): `refuse_foreign` leaves a directory without `boost/version.hpp` untouched, before any download; the tree is filled beside and moved into place; a case pins the refusal and a second fill.
- Minor 6 (the `modules.poke` branch untested, its message wrong): the message names "user-config.jam's BOOST_SOURCE, <dir>,"; `test_sources_named_by_user_config` covers the CI's path.
- Minor 7 (gcc untested): `harness.gcc_request` adds the configured gcc to the boost_url, Charconv, Locale and SRELL cases, skipped where no gcc is configured (the CI's tools job registers clang alone; gcc reaches the targets in the libraries' gcc lanes).
- Minor 8 (`update-generated` not atomic): outputs are copied beside their committed files and renamed into place only once all are copied; a generator that fails after its first output is a new case.
- Minor 9 (the `declared-lanes` assertion and stage 4's kind field): the `tools/webcpp.jam` row of the conflicts table says the assertion moves with it.
- Minor 10 (scratch copies of `third_party/libs`): a scratch superproject links the checkout's `third_party/libs`, and `third_party_of_its_own` gives a case its own copy, so no case writes through the link; `harness_test.py` pins both.
- Minor 11 (AGENTS chapter 6 repeated its first sentence): the exclusion is merged into that sentence.
- Minor 12 (ambiguous "first two lines"): the two lines of `boost_archive` are quoted.
- Minor 13 (the drafts' wasip2 claims): Task 5 Step 5 runs SRELL's conformance test on wasip2 against the mirror's commit; the no-exceptions claim rests on `without_exceptions` on wasip2; the suite's 885 regex tests are worded as the spike's measurement.
- Minor 14 (closing the worktree): Task 7 Step 6 checks the bare clone's three commits, then `git worktree remove --force`.
- Minor 15 (DESIGN's +281/-106): no change here; Point 7 asks the controller to correct the entry.
- Controller, later: S4b added as Task 3B (`/webcpp//boost_charconv`, `/webcpp//boost_locale`), measured on every target before writing it (`BOOST_LOCALE_ON_EVERY_TARGET.md`), Boost.Locale refused by name on emscripten, the one target where it does not build.
- Controller, later: the git identity rule is in Global Constraints and in every command that rebases, merges or commits, with the check before each integration.
