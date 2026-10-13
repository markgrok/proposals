# webcpp::json_schema: where the implementation stopped, and how to resume

Written 2026-10-12, when this proposal revived the library. `plan.md` is the plan, every task
with its code; `../tasks.md` is the checklist; `source/` is the library's tree at the last
commit of its branch `e1`; `source-wip/` is the uncommitted work of task E1.10.

## Where it stopped, task by task

The library was developed in the superproject worktree `~/dev/webcpp-json-schema-e1` (branch
`lib/json_schema-e1` of the superproject), its `libs/json_schema` a local repository on branch
`e1`, 18 commits from 2026-10-09 to 2026-10-11, all as `R`. The canonical local
repository the plan names (`~/dev/webcpp-repos/json_schema`) was created empty and never
received a commit: E1.17's fast-forward was never reached. Nothing was pushed or published.

| Task | State | Commits on `e1` (subject after "json_schema: ") |
| --- | --- | --- |
| Plan's Task 0 | done 2026-10-09 | none in the library: the repository, the worktree, the licences re-checked |
| E1.1 skeleton, configuration, errors, memory adapters | done | `bba2c1b` the library's skeleton...; `02cb5ed` the allocator adapter serves every alignment...; `64144f4` a diagnostic's cut moves back three bytes at most... |
| E1.2 exact numbers | done | `1fe02ad` exact numbers, an instance double never rounded, multipleOf without floating point; `052b74d` a decimal literal of any length is read whole... |
| E1.3 JSON equality, copies, pointers, UTF-8 | done | `9055771` JSON equality, iterative copies into a trivially freed arena...; `8c48061` equal passes an allocation failure to its caller... |
| E1.4 the regex policy | done over webcpp::regex; **to redo** on Boost.Regex through formats | `61da457` the regex policy over webcpp::regex, in both allocators, with its step budget |
| E1.5 errors, annotations, events, record, sinks | done | `e0aa6f0` errors, annotations, the evaluation events, the record and the two sinks |
| E1.6 traits, dialects, exact JSON text | done | `56f39d6` traits, the dialect tags, and JSON text read exactly...; `786e123` exact_table orders its nodes instead of hashing them... |
| E1.7 the IR | done | `f100116` the IR, one copy of each value...; `3dce8b8` the IR refuses a bound that is not finite... |
| E1.8 options, message hook, later data types | done | `7df8a93` the compile and validate options...; `a483969` a registry moves, taking the source's allocator... |
| E1.9 the 2020-12 loader | done | `4b40434` the 2020-12 loader, iterative over one copy of the schema; `33b44ca` pointers held as steps, linear memory and lookups, and a dialect policy independent of include order; `80c3b69` check spells a pointer only for the fault it returns... |
| E1.10 the compiler | **in progress**, uncommitted | the files in `source-wip/`; its README says what is done and missing |
| E1.11 to E1.17, E2, E3, E4, E5, P | not started | |

`33b44ca` and `80c3b69` came from the review of E1.7 to E1.9: the IR holds JSON Pointers as
steps (invariant 15 of `spec.md` 5.3, written into the spec then), `check()` costs linear time,
and `dialect_policy` reads `has_machinery`, so include order cannot change its answer. The spec
in `spec.md` already holds these.

### The snapshot's tests

`source/test/Jamfile` declares, on native, wasip2, wasip3 and Emscripten: headers-alone builds
of every header directory; `config` and `config_developer_defined`; `errors`, `errors_throw`,
`memory`, `memory_throw`, `fancy_pointer_refused` (a compile diagnostic); `numbers`,
`numbers_oracle` (against `test/numbers/oracle.txt`, which `test/numbers/oracle.py` writes;
the own lane `oracle` checks it); `equality`, `deep_copy`, `pointer`, `utf8`, `json_memory`,
`json_throw` and two run-fail twins without exceptions; `sink`; `regex`; `dialect`, `parse`;
`ir`; `options`; `loader_draft2020_12`, `compile_errors`, `dialect_order`.

Measured on 2026-10-12, from scratch, in a scratch copy of the original superproject worktree
with `libs/json_schema` replaced by `source/` (which still had webcpp::regex beside it):

- native (Apple clang 21): `b2 -a libs/json_schema/test`: exit 0, 71 tests `**passed**`
  (two run-fail twins `failed-as-expected`), 536 targets;
- wasip2: `b2 -a toolset=clang-wasip2 testing.launcher=wasmtime libs/json_schema/test`: exit 0,
  71 `**passed**`, 680 targets;
- with `source-wip/` applied, native: 76 `**passed**`, and wasip2's `test//compiler` passes.

wasip3, Emscripten and gcc-15 were green at each task's merge (the plan's `lanes`), and not
re-run here.

### One edit to the snapshot

`source/meta/libraries.json`'s `maintainers` carried an address this repository does not
publish; it now holds the address the superproject's `AGENTS.md` (chapter 11) gives for
commits. Every other file is byte for byte the branch `e1` at `80c3b69`
(`git archive`).

## How to resume

Prerequisites are the superproject's (its `AGENTS.md`, chapter 1, "Prerequisites" and
"Machine-local setup"): Boost 1.92 with its sources in `.local/boost-source` (Boost.URL is built
from them), b2 5.5.3, a C++20 compiler, Python 3.9 or newer, wasi-sdk 34, wasmtime 47,
Emscripten 6.0.11 from emsdk, MrDocs 2026.9.29 and Node for the documentation, and a
`.local/user-config.jam` registering the toolsets.

1. **A superproject worktree** from `main` (the superproject's `main` already holds what the
   plan's "shared items" waited for: the suite's submodule at `5b0ee16`, its target
   `/webcpp/third_party//json-schema-test-suite`, `webcpp.data`, `webcpp.generated`,
   `/webcpp//boost_url`, `/webcpp//boost_locale`, and `libs/config`):

   ```bash
   git -C <superproject> worktree add ../webcpp-json-schema -b lib/json_schema main
   cd ../webcpp-json-schema
   git submodule update --init third_party/libs/JSON-Schema-Test-Suite libs/config
   ln -s <superproject>/.local .local
   ```

   Never put a library that is not a registered submodule into the superproject's own
   checkout: its Jamroot globs `libs/*/build.jam` and its lint reads it.
2. **The library's repository**, one import commit (the superproject's rule: a repository
   starts from one import commit, the history it came from is not carried):

   ```bash
   mkdir libs/json_schema
   cp -R <proposals>/openspec/changes/json-schema/reference/source/. libs/json_schema/
   git -C libs/json_schema init -b main
   cd libs/json_schema
   git add -- $(find . -type f -not -path './.git/*' | sort)   # every file by path
   git -c user.name=R -c user.email=github.overboard312@passmail.net \
       commit -m "json_schema: import, as the library stood after its 2020-12 loader"
   git checkout -b e1
   ```
3. **The work in progress:**

   ```bash
   cp -R <proposals>/openspec/changes/json-schema/reference/source-wip/include \
         <proposals>/openspec/changes/json-schema/reference/source-wip/test \
         <proposals>/openspec/changes/json-schema/reference/source-wip/meta .
   ```

   (`source-wip/README.md` itself is not copied.)
4. **webcpp::lexical beside it**, for task 1.4: at `libs/lexical` (with `libs/idna`, which it
   needs), from `webcpporg/lexical` once published, or from the formats track's local
   repository until then, at the commit that carries its Boost.Regex translator and the public
   matching interface `plan.md`'s E1.4 lists.
5. **Group 0 of `../tasks.md`**, then **task 1.4**, then E1.10's remaining points
   (`source-wip/README.md`), its commit, and task 1.11 on, in the plan's order. Each task
   follows its section of `plan.md`: the failing test first, seen failing, the implementation,
   the five lanes and the lint, the commit.

### The commands

From the superproject worktree's root. The plan writes `$B2` for
`env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH b2 --user-config=.local/user-config.jam`.

| What | Command |
| --- | --- |
| one test, while working on it | `$B2 libs/json_schema/test//<name>` (`regex`, `compiler`, `ir`, ...) |
| native, from scratch | `$B2 -a libs/json_schema/test libs/json_schema/example` |
| native with gcc 15 | `$B2 -a toolset=gcc-15 libs/json_schema/test libs/json_schema/example` |
| wasip2 | `$B2 -a toolset=clang-wasip2 testing.launcher=wasmtime libs/json_schema/test libs/json_schema/example` |
| wasip3 | `$B2 -a toolset=clang-wasip3 testing.launcher=wasmtime libs/json_schema/test libs/json_schema/example` |
| Emscripten | `$B2 -a toolset=emscripten libs/json_schema/test libs/json_schema/example` (never a `testing.launcher`) |
| the numbers oracle's own lane | `$B2 -a libs/json_schema/test//oracle` |
| the five lanes at once | the plan's `lanes libs/json_schema/test` (each lane in its own `--build-dir`) |
| the documentation, from E1.16 on | `$B2 -a libs/json_schema/doc` |
| the lint | `tools/lint/lint.sh --clang-format .local/wasi-sdk/bin/clang-format --clang-tidy .local/wasi-sdk/bin/clang-tidy`, last line `lint: clean` |

Only a build from scratch (`-a`) counts as evidence. A test reading the suite gets it through
`<library>/webcpp/third_party//json-schema-test-suite`, which defines
`WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE` and hands the directory to wasmtime.

## What to verify first

1. `source/` is the branch `e1` at `80c3b69` but the one edit above: 84 files.
2. After step 3, every file of `source-wip/` but its README is in place, and `test/Jamfile`
   ends with the two `compiler` lines.
3. Group 0's changes compile: `$B2 libs/json_schema/test//config` and its two twins.
4. After task 1.4, the whole test directory on the five lanes from scratch, and the lint. The
   expected count is the snapshot's 71 plus E1.10's 5 and group 0's `config_all_off`; `regex`'s
   cases change with the redo.
5. `test/compiler_test.cpp`'s three goldens, unchanged by the regex redo (the compiler reaches
   the policy only through its concept).

Before task 1.4 the snapshot does not build on a fresh superproject: `traits.hpp` includes
`regex/webcpp_regex.hpp`, which includes `<webcpp/regex.hpp>`, and webcpp::regex was never
published (it lived as local repositories in the original machine's worktrees). Group 0 and
task 1.4 therefore reach green together.

## Known issues

- **The regex change reaches beyond E1.4.** Besides the files E1.4 lists, the plan names SRELL
  or webcpp::regex in: E1.1's `build.jam`, `README.md` and `AGENTS.md` (the SRELL caution) and
  the include boundaries of E1.2, E1.4, E4.2 (done or to come: every `srell` and `webcpp/regex`
  entry becomes `boost/regex`); E1.6's `default_traits` (done: `webcpp_regex` becomes
  `boost_regex`); E1.15 (the regex limits: `regex_inline_bytes` is measured again for
  Boost.Regex's `match_results`, and the nesting bound is formats' 32); E3.4 (a note on SRELL's
  required-character scan answering `(a*)*b` quickly); E5.8 (the Bowtie Dockerfile copies
  SRELL: it copies formats' and idna's headers instead); E5.10 (allocation over patterns:
  Boost.Regex's stated exception instead of SRELL's allocator patch); E5.20 (the AGENTS.md
  cautions); the plan's Task 0 and `superproject_worktree` (no srell submodule).
- **formats' matching interface does not exist yet.** formats (being turned into production
  code by another track) exposes `is_regex` and keeps its translator in `detail`; E1.4 needs a
  public compile-and-search. json_schema must not reach `detail`.
- **The allocation guarantee narrows** for schemas with patterns (Boost.Regex's own
  allocations, `design.md` D5); E1.15's and E5.10's allocation tests are written for SRELL's
  allocator patch and change accordingly.
- **The plan's shell functions are dated:** `SUPERPROJECT_BASE=infra/shared` (now `main`), the
  srell submodule in `superproject_worktree`, and the canonical repository on the original
  machine; group 0 updates them.
- **E1.10's open points:** the lanes but native and wasip2, the lint, the `length_error` path
  (`source-wip/README.md`).
- **Not carried:** the ledger (`~/.buzz/WORK_LOGS/WEBCPP_JSON_SCHEMA_LOG.md`) and the plan
  reviews (`WEBCPP_JSON_SCHEMA_PLAN-E1-review.md`, `-E2E5-review.md`), private notes of the
  original machine; the plan's "Review disposition" section records every finding's outcome.

## Decisions pending

- The owner's go to revive json_schema, and where its canonical repository lives.
- `errc::unsupported_regex = 21` and the allocation exception for Boost.Regex (`design.md` D5).
- lexical's public matching interface (lexical's change `strategies`, group 3): names, and whether its compiled pattern keeps the
  resource it was given.
- Amendment A1 (numbers; E1.2 implements it) and A2 (`format_entry::check_in`, E5.5).
- The plan's "What the owner must decide": the licence of the bundled meta-schemas and the
  OpenAPI schema (recommended: commit them with `LICENSE-JSON-SCHEMA.txt` and
  `LICENSE-OPENAPI.txt`); jsonschema-benchmark's missing licence; the benchmark's pins (its
  commit, Blaze, ajv); Bowtie's pin and its lane.
- PD1's split of the messages between E1 and E3 (reported as open in the plan).
