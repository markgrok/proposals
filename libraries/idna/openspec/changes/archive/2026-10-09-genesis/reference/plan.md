---
title: "webcpp::idna: implementation plan"
tags: [webcpp, idna, unicode, punycode, plan]
status: active
created: 2026-10-09
---

# webcpp::idna Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build `webcpp::idna`, webcpp's header-only C++20 library of
internationalized domain names (Punycode, UTS 46 ToASCII and ToUnicode,
IDNA2008 validity over generated Unicode 17.0.0 tables), with its generator,
its conformance tests at 100%, its page and its CI, as the local repository
`libs/idna` of the integration space `~/dev/webcpp-formats`, each task built
in a worktree of its own, ready for the owner to publish as `webcpporg/idna`.

**Architecture:**
- **Public headers** (`errors.hpp`, `options.hpp`, `punycode.hpp`,
  `validity.hpp`, `uts46.hpp`, under `config.hpp`) are the spec's section 4
  contract, frozen for the formats track: span forms that never allocate, pmr
  forms that allocate from their resource alone, errors as values.
- **One label driver** (`detail/process.hpp`) streams UTS 46 section 4 label
  by label through one label's inline storage (`detail/label_buffer.hpp`, 256
  code points), with or without the mapping (`to_*` or `check_*`), into a
  sink of the ASCII form, the Unicode form, or nothing. The rules sit in
  `detail/nfc.hpp`, `detail/context.hpp` and `detail/bidi.hpp`, over lookups
  (`detail/properties.hpp`) into `constexpr` run lists that
  `tools/generate_tables.py` writes from the pinned Unicode files
  (`detail/tables.hpp`), with the test vectors it also writes.
- **Prototype, and what was run.** Every C++ file and the generator below were
  written and run before this plan, in `/tmp/idna-plan/proto/libs/idna`
  (scratch, not authoritative; this plan is). All 6,391 lines of
  IdnaTestV2.txt pass in both passes and all 20,034 lines of
  NormalizationTest.txt; JSON Schema's `idn-hostname` file (draft2020-12)
  passes 92 of 92 through `to_ascii` with the defaults. After the review, the
  library was built by b2 in a scratch superproject (`git archive` of
  `~/dev/webcpp` `HEAD`, `webcpp.generated` stubbed by an alias): every test
  and example passed on native Clang, GCC 15, wasip2, wasip3 and Emscripten;
  `tools/lint/lint.sh` ended `lint: clean`; `b2 -a libs/idna/doc` built the
  page; every test passed under ASan, UBSan and libc++'s extensive hardening.

**Tech Stack:** C++20, header-only; Boost 1.92 header-only (Config, System,
Core, Assert, lightweight_test); b2 5.5.3; wasi-sdk 34, wasmtime 47.0.3;
Emscripten 6.0.11 under Node; MrDocs 2026.9.29 and Asciidoctor.js; Python 3.9+
(standard library) for the generator; Unicode 17.0.0 and UTS 46 revision 35.

**Spec:** `~/.buzz/PLANS/WEBCPP_IDNA_SPEC.md` (approved 2026-10-09; binding;
its public API, section 4, is the contract). Also read: the superproject's
`AGENTS.md` (in the worktree), `~/.buzz/RESEARCH/IDNA_SPIKE.md`,
`~/.buzz/PLANS/WEBCPP_DESIGN.md` section "Owner decisions after approval",
`~/.buzz/PLANS/WEBCPP_SHARED_INFRA_PLAN.md` Task 2 (`webcpp.generated`),
`~/.buzz/PLANS/WEBCPP_FORMATS_PLAN.md` (Global Constraints, Task 0, and what
formats calls of idna). Models: `libs/pratt` and `libs/xactor`. The review
this revision answers:
`~/dev/webcpp/.superpowers/sdd/plan-reviews/WEBCPP_IDNA_PLAN-review.md`
(section "Review disposition" below).

## Global Constraints

- **The integration space.** `~/dev/webcpp-formats` (branch `lib/formats`, idna spec section 3.1) is an integration space only: only the controller writes in it, one integration at a time, idna's or formats'. idna's canonical repository is `~/dev/webcpp-formats/libs/idna` (`git init -b main`), untracked by the superproject, which formats consumes there. Never `~/dev/webcpp`: stage 4 builds there, and its Jamroot's `glob libs/*/build.jam` and its lint would read an unfinished library.
- **A worktree per task.** Every implementing task runs in `~/dev/webcpp-idna-<slug>`: a detached superproject worktree of `lib/formats` with `.local` links, whose `libs/idna` is a git worktree of the canonical repository on branch `task/<slug>` (procedure "Making a task's worktree"). Tasks marked parallel have separate worktrees by construction; two agents never share one. The controller rebases, verifies and fast-forwards each finished task into the canonical `main`, then runs the verdict in the integration space (procedure "Integrating a task").
- **Identity on every commit-writing git command.** Every git command that writes a commit, `commit`, `rebase` (and `rebase --continue`), `cherry-pick`, a `merge` that is not a fast-forward, `am`, `commit --amend`, runs as `git -c user.name=R -c user.email=github.overboard312@passmail.net ...`: the machine's global identity must never reach a webcpp commit, as author or committer. Before each integration, `git -C <repo> log --format='%an %ae %cn %ce' <range> | sort -u` prints only `R github.overboard312@passmail.net R github.overboard312@passmail.net`. Commits carry no trailer of any kind and never an agent co-author; a body paragraph never starts with `word:`.
- **Staging.** Stage files by path. Never `git add -A`, never `git add libs`. Never push, never add a remote, never touch GitHub: Task 14 is the controller's, after the owner's go.
- **Shared items** reach the integration space one way: the controller rebases `lib/formats` onto `infra/shared` once the item is committed there, then onto `main` once the infra track has integrated it, with the identity: `git -c user.name=R -c user.email=github.overboard312@passmail.net -C ~/dev/webcpp-formats rebase infra/shared` (or `rebase main`), at a moment no task of either track is integrating, and checks `git -C ~/dev/webcpp-formats log --format='%an %ae %cn %ce' main..lib/formats | sort -u` as above. A task's worktree made after it starts from the new `lib/formats`. idna commits its `webcpp.generated` Jamfile lines (Task 6) only in a worktree made after `lib/formats` carries S1.
- **S1, as the infra plan's Task 2 gives it:** `webcpp.generated <name> : <script> : <argument> * : <output> + ;` in a test Jamfile declares the explicit targets `generated-<name>` and `update-generated`; the script runs in the Jamfile's directory as `<script> <argument>... <path-1> ... <path-n>` and writes its i-th output at `<path-i>`; outputs are relative to the Jamfile; `tools/oracle/compare.py --files` names each output that differs (`compare: <path>: contents differ`). An own lane names the check: `webcpp.lane tables : generated-tables ;`.
- **Text.** English only. Never the em dash (U+2014); use a plain dash. No case-insensitive `veru`+`s` anywhere (files, names, commits). No "stage N", "Task N", nor the spec's labels (`I0` to `I6`, `P1` to `P8`, `S1`) in tracked files or commit messages (UTS 46's own step codes, `P4`, `V7`, `X4_2`, are names of the standard and stay).
- **Licence.** Every new source file (`.hpp`, `.cpp`, `.py`, `.adoc`, `Jamfile`, `build.jam`, `.yml`) opens with the BSL-1.0 notice "Copyright (c) 2026 WebCpp.org", `//` for C++ and AsciiDoc, `#` for Python, Jam and YAML, after a `#!` line. The Unicode data notice follows P8's proposal, provisionally (the controller's ruling of 2026-10-09; the owner's to confirm before publication): the page's appendix "Unicode data" carries the Unicode License v3 whole, each generated file names it in one comment line, and no `LICENSE-UNICODE.txt` is added.
- **b2.** Every call is `env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH b2 --user-config=.local/user-config.jam ...` from the root of the worktree at hand; below, `$B2` stands for it (`B2='env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH b2 --user-config=.local/user-config.jam'`). A direct compiler call is prefixed by `env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH` too. A verdict is built with `-a`. Lanes run at once, each with its own `--build-dir`.
- **Logs** go under `~/dev/webcpp/.superpowers/sdd/WEBCPP_IDNA_PLAN/<slug>/` (the task's slug; `integration-<slug>` for the controller's verdict), never `/tmp`.
- **Targets.** `native wasip2 wasip3 emscripten` in `test/Jamfile` and `example/Jamfile`. wasip2 and wasip3 take `testing.launcher=wasmtime`; emscripten is `toolset=emscripten` with no launcher. `export EM_CACHE=~/dev/webcpp/.local/emscripten-cache` before an emscripten build; `.local/emsdk` is read-only.
- **Dependencies.** Boost's header-only Config, System, Core and Assert, and the standard library; nothing compiled, nothing third-party. `config.hpp` includes `<boost/config.hpp>` until `webcpp::config` exists; the migration is that one line and `build.jam`'s alias gaining `<library>/webcpp/config//config`, in one commit with a changelog line.
- **C++.** Header-only C++20. Errors as values (`idna::result<T>`, category `webcpp.idna`). No `throw`, `try` or `catch` in a header. No global or static mutable state. Text as `std::string_view`. Guards first in every function. Span forms and validators never allocate; pmr forms allocate from their resource alone. Every test builds with the standard libraries' own checks (`_LIBCPP_HARDENING_MODE_EXTENSIVE`, `_GLIBCXX_ASSERTIONS`, `test/Jamfile`).
- **Errors** are fixed and never reused: 1 `invalid_utf8`, 2 `disallowed`, 3 `not_idna2008`, 4 `not_ldh`, 5 `not_nfc`, 6 `hyphen_in_third_and_fourth`, 7 `hyphen_at_start_or_end`, 8 `ace_prefix`, 9 `leading_combining_mark`, 10 `contextj`, 11 `contexto`, 12 `bidi`, 13 `invalid_punycode`, 14 `punycode_overflow`, 15 `empty_label`, 16 `label_too_long`, 17 `domain_too_long`, 18 `output_too_small`, 19 `storage_exhausted`. Category name `webcpp.idna`, identity `0x69646E6100000000`.
- **Measured lint facts** (wasi-sdk 34's clang-tidy with webcpp's `.clang-tidy`, as `lint.sh` runs it on every target): Boost 1.92's `result::operator*` calls `value()`, which throws, so a `noexcept` function reads a checked result with `unsafe_value()`; `readability-trailing-comma` misreads an empty `T{}` followed by a comma, so a default `options` is written `idna::options()`; a multi-line braced list ends with a trailing comma; `readability-qualified-auto` wants `auto*` for an array iterator where it is a pointer (libc++), which is a class elsewhere (MSVC), so a lookup keeps an index, never an iterator variable; a narrow string literal spells a non-ASCII byte as a three-digit octal escape (`"b\303\274cher"`), never a hex escape nor raw UTF-8; a `U""` literal uses `\u`/`\U`, and `\x` only for a value no universal character name may name; a Doc Comment has no `\u` (a command to the lint) and no colon after `@ref x`.
- **Python.** 3.9 or newer, standard library only, Pyright-clean with `pyrightconfig.json`, 100 columns, two blank lines before a top-level `def` or `class`.
- **Generated files** (`include/webcpp/idna/detail/tables.hpp`, `test/generated/*.ipp`) are written only by `tools/generate_tables.py`: in a build, through `update-generated`; never by hand.
- **ada-url** is read for algorithms and table packing only; nothing of it is copied, and no file of it enters the repository.
- **Lint.** `tools/lint/lint.sh --clang-format .local/wasi-sdk/bin/clang-format --clang-tidy .local/wasi-sdk/bin/clang-tidy` ends `lint: clean` in the task's worktree before its commit, and in the integration space at the verdict.
- **Every task's report ends with a "Changelog" section**, Boost's release-notes form, which the controller merges into `~/.buzz/PLANS/WEBCPP_CHANGELOG_PENDING.md` (idna has no `doc/changelog.adoc` yet). idna is new: its one line goes under "New Libraries", and later tasks refine that line rather than add bullets.

## Review Focus

1. **The four label separators of UTS 46**, `.`, U+3002, U+FF0E, U+FF61, alone, leading, trailing and between labels: `to_ascii` refuses each alone or at an edge (`empty_label`) and splits at each between labels (`a。b` is `a.b`), as JSON Schema's `idn-hostname` tests require; `check_domain`, which maps nothing, refuses U+FF0E as a code point (`disallowed`), never as a separator. Pinned in Task 11 (`separators`) and Task 9 (`separators_as_given`).
2. **Which error a name with several faults reports**: label errors in label order, then `bidi`, then `domain_too_long`, then `output_too_small` (`a_b.xn--zz` is `not_ldh`, `xn--zz.a_b` is `invalid_punycode`, `0a.א.a_b` is `not_ldh`, never `bidi`), and a label beyond the storage keeps an error found before it; formats returns that error as its rule's. Pinned in Task 11 (`the_first_error_wins`, `beyond_the_inline_storage`) and Task 9 (`process_test`).
3. **NUL and C0 controls inside a `std::string_view`**: `a\0b` is `not_ldh` by default, `not_idna2008` without the STD3 rules, and passes only with both off, as UTS 46 alone allows; `to_unicode` writes it as it is. Pinned in Task 11 (`control_characters`) and Task 9 (`control_characters_as_given`).
4. **The DNS root label**: `example.` is `empty_label` in `to_ascii` under `verify_dns_length` and no error in `to_unicode` (IdnaTestV2's X4_2 exempts only a final empty label), and both pass it through without `verify_dns_length`; `.example` is `empty_label` in both. Pinned in Task 11 (`the_root_label`).
5. **The inline storage's edge, after canonical decomposition**: 256 code points as given pass, 257 are `storage_exhausted` without `verify_dns_length` (`label_too_long` with it), and so is a label whose NFC check's copy outgrows the storage; in `to_ascii`, 255 `a` and one `é` decompose to 257 and are `storage_exhausted` in the span form while the pmr form converts them. Pinned in Task 9 (`the_inline_storage_edge`, `beyond_the_inline_storage`) and Task 11 (`a_label_that_decomposes_past_the_storage`).

---

## Execution map

| Task | Slug | Spec | Needs (integrated) | Shared item | Parallel with |
| --- | --- | --- | --- | --- | --- |
| 0 | - (controller) | - | - | none | - |
| 1 | `skeleton` | I0 | 0 | none | S1 (infra), formats F0 |
| 2 | `punycode` | I1 | 1 | none | 3, 4, 5, 8 |
| 3 | `tables` | I2, tables | 1 | none | 2 |
| 4 | `tables-doc` | I2, page | 3 | none | 2, 8 |
| 5 | `vectors` | I2, vectors | 3, 4 | none | 2, 8 |
| 6 | `lane` | I2, check | 5 | S1 in `lib/formats` | 7, 8, 9, 10, 11, 12 |
| 7 | `nfc` | I3 | 5 | none | 2, 6, 8 |
| 8 | `rules` | I4, rules | 3 | none | 2, 4, 5, 6, 7 |
| 9 | `validity` | I4, code | 2, 4, 7, 8 | none | 6 |
| 10 | `validity-doc` | I4, page | 9 | none | 6, 11 |
| 11 | `uts46` | I5, code | 9 | none | 6, 10 |
| 12 | `uts46-doc` | I5, page | 10, 11 | none | 6 |
| 13 | `readiness` | I6 | 6, 12 | S1 (its verification runs the lane) | - |
| 14 | - (controller) | publication | 13, owner's go | S1 in `main` | - |

Tasks 1 to 5 and 7 to 12 need no shared item; only Task 6 consumes S1.
"Parallel with" lists tasks whose worktrees may exist at once (each task's
own); the controller keeps at most three or four implementing tracks running
across idna, formats and the other plans, as the owner decided. Task 9
unblocks formats F5 (`check_label`), Task 11 formats F6 (`to_ascii`).

## Procedures (controller)

### Making a task's worktree

Before a task starts, from the latest `lib/formats` and the latest canonical
`main`, with `S` the task's slug:

```bash
S=<slug>
git -C ~/dev/webcpp-formats worktree add --detach ~/dev/webcpp-idna-$S lib/formats
mkdir ~/dev/webcpp-idna-$S/.local
cp ~/dev/webcpp/.local/user-config.jam ~/dev/webcpp-idna-$S/.local/user-config.jam
for name in wasi-sdk wit-bindgen wasi-wit mrdocs emsdk emscripten; do
    ln -s ~/dev/webcpp/.local/$name ~/dev/webcpp-idna-$S/.local/$name
done
git -C ~/dev/webcpp-formats/libs/idna worktree add ~/dev/webcpp-idna-$S/libs/idna -b task/$S main
mkdir -p ~/dev/webcpp/.superpowers/sdd/WEBCPP_IDNA_PLAN/$S
```

Task 1, whose repository has no commit yet, takes
`git -C ~/dev/webcpp-formats/libs/idna worktree add --orphan -b task/skeleton ~/dev/webcpp-idna-skeleton/libs/idna`
in place of the last `worktree add`. When `lib/formats` records the shared
submodules (after the infra track's suite and SRELL), run in the new worktree
the block the formats plan's Global Constraints give for them:

```bash
cd ~/dev/webcpp-idna-$S
git clone --no-checkout ~/.buzz/REPOS/JSON-Schema-Test-Suite third_party/libs/JSON-Schema-Test-Suite \
  && git -C third_party/libs/JSON-Schema-Test-Suite checkout --detach 5b0ee1613e45fcc2bddac00e07c19cd49b00d8a8
git clone ~/.buzz/REPOS/srell.git third_party/libs/srell \
  && git -C third_party/libs/srell checkout --detach "$(git ls-tree HEAD third_party/libs/srell | awk '{print $3}')"
ln -sfn ~/dev/webcpp-infra/.local/boost-source .local/boost-source
git submodule status third_party/libs/JSON-Schema-Test-Suite third_party/libs/srell
```

(each clone only for a submodule `HEAD` records). Expected:
`git -C ~/dev/webcpp-idna-$S/libs/idna branch --show-current` prints
`task/<slug>`; the superproject worktree holds no `libs/formats`, so idna's
lint and builds there read idna alone. The task runs every command from
`~/dev/webcpp-idna-<slug>`.

### Integrating a task

Once the task's report is reviewed, with no other integration (idna's or
formats') in flight:

```bash
S=<slug>; W=~/dev/webcpp-idna-$S; C=~/dev/webcpp-formats/libs/idna
git -C $C status --porcelain                       # prints nothing
git -C $W checkout --detach lib/formats            # only when lib/formats moved; then the submodule block
git -c user.name=R -c user.email=github.overboard312@passmail.net -C $W/libs/idna rebase main
git -C $W/libs/idna log --format='%an %ae %cn %ce' main..HEAD | sort -u
```

Expected: the identity line alone. A conflict is resolved by keeping both
sides (each task only adds lines to `test/Jamfile`, `example/Jamfile`,
`doc/idna.adoc`, `doc/introduction.adoc`, `doc/testing.adoc` and
`AGENTS.md`): the test Jamfile's runs in the order of the file structure
below, the page's includes in the order of Task 13's `idna.adoc`, a table's
rows in task order; then
`git -c user.name=R -c user.email=github.overboard312@passmail.net -C $W/libs/idna rebase --continue`.
Then the task's own verification again in `$W`, and:

```bash
git -C $C merge --ff-only task/$S
cd ~/dev/webcpp-formats
L=~/dev/webcpp/.superpowers/sdd/WEBCPP_IDNA_PLAN/integration-$S; mkdir -p $L
export EM_CACHE=~/dev/webcpp/.local/emscripten-cache
EXAMPLES="$(test -f libs/idna/example/Jamfile && echo libs/idna/example)"
$B2 -a --build-dir=bin/idna-native libs/idna/test $EXAMPLES > $L/native.log 2>&1 &
$B2 -a --build-dir=bin/idna-gcc toolset=gcc-15 libs/idna/test $EXAMPLES > $L/gcc.log 2>&1 &
$B2 -a --build-dir=bin/idna-wasip2 toolset=clang-wasip2 testing.launcher=wasmtime libs/idna/test $EXAMPLES > $L/wasip2.log 2>&1 &
$B2 -a --build-dir=bin/idna-wasip3 toolset=clang-wasip3 testing.launcher=wasmtime libs/idna/test $EXAMPLES > $L/wasip3.log 2>&1 &
$B2 -a --build-dir=bin/idna-emscripten toolset=emscripten libs/idna/test $EXAMPLES > $L/emscripten.log 2>&1 &
wait
grep -L -e '\.\.\.failed' $L/{native,gcc,wasip2,wasip3,emscripten}.log
$B2 -a --build-dir=bin/idna-doc libs/idna/doc > $L/doc.log 2>&1; echo "doc $?"
tools/lint/lint.sh --clang-format .local/wasi-sdk/bin/clang-format --clang-tidy .local/wasi-sdk/bin/clang-tidy > $L/lint.log 2>&1; tail -1 $L/lint.log
```

Expected: the five logs listed, `doc 0`, `lint: clean` (the lint reads
`libs/formats` too, at its last integrated commit). Then remove the worktrees:

```bash
git -C $C worktree remove $W/libs/idna && git -C $C branch -d task/$S
git -C ~/dev/webcpp-formats worktree remove --force $W
```

A task that needs another (the execution map) starts only once that one is
integrated.

### The task's lanes

Each task's verification step is this block, from its worktree:

```bash
cd ~/dev/webcpp-idna-<slug>
L=~/dev/webcpp/.superpowers/sdd/WEBCPP_IDNA_PLAN/<slug>
export EM_CACHE=~/dev/webcpp/.local/emscripten-cache
EXAMPLES="$(test -f libs/idna/example/Jamfile && echo libs/idna/example)"
$B2 -a --build-dir=bin/idna-native libs/idna/test $EXAMPLES > $L/native.log 2>&1 &
$B2 -a --build-dir=bin/idna-gcc toolset=gcc-15 libs/idna/test $EXAMPLES > $L/gcc.log 2>&1 &
$B2 -a --build-dir=bin/idna-wasip2 toolset=clang-wasip2 testing.launcher=wasmtime libs/idna/test $EXAMPLES > $L/wasip2.log 2>&1 &
$B2 -a --build-dir=bin/idna-wasip3 toolset=clang-wasip3 testing.launcher=wasmtime libs/idna/test $EXAMPLES > $L/wasip3.log 2>&1 &
$B2 -a --build-dir=bin/idna-emscripten toolset=emscripten libs/idna/test $EXAMPLES > $L/emscripten.log 2>&1 &
wait
grep -L -e '\.\.\.failed' $L/{native,gcc,wasip2,wasip3,emscripten}.log
$B2 -a --build-dir=bin/idna-doc libs/idna/doc > $L/doc.log 2>&1; echo "doc $?"
tools/lint/lint.sh --clang-format .local/wasi-sdk/bin/clang-format --clang-tidy .local/wasi-sdk/bin/clang-tidy > $L/lint.log 2>&1; tail -1 $L/lint.log
```

Expected, unless a step says more: `grep -L` lists all five logs (none holds
`...failed`), each ending `...updated <n> targets...`; `doc 0`;
`lint: clean`. Each task's verification step gives this block in full, with
its slug.

## File structure

```
~/dev/webcpp-formats/                      the integration space, branch lib/formats
  AGENTS.md, README.md                     + idna in the libraries (Task 13, by the controller)
  libs/idna/                               idna's own repository
    build.jam  LICENSE_1_0.txt  README.md  AGENTS.md  .gitignore  .gitattributes
    .github/workflows/ci.yml               the superproject's library.yml, library: idna
    meta/libraries.json                    "port-of": null
    meta/include-boundaries.json           no Boost.URL, Boost.JSON, SRELL or formats
    include/webcpp/idna.hpp                every public header
    include/webcpp/idna/config.hpp         WEBCPP_IDNA_NO_EXCEPTIONS
    include/webcpp/idna/errors.hpp         error, error_category, make_error_code, result
    include/webcpp/idna/options.hpp        options, max_label_length, max_domain_length, unicode_version
    include/webcpp/idna/punycode.hpp       punycode::encode, punycode::decode (span and pmr)
    include/webcpp/idna/validity.hpp       check_label, check_domain, is_valid_label, is_valid_domain
    include/webcpp/idna/uts46.hpp          to_ascii, to_unicode, unicode_result, unicode_string
    include/webcpp/idna/detail/utf8.hpp        strict UTF-8
    include/webcpp/idna/detail/tables.hpp      GENERATED: the Unicode 17.0.0 tables
    include/webcpp/idna/detail/properties.hpp  the lookups over the tables
    include/webcpp/idna/detail/label_buffer.hpp  inline_label, growing_label
    include/webcpp/idna/detail/nfc.hpp         NFC within a label's storage
    include/webcpp/idna/detail/context.hpp     RFC 5892 appendix A
    include/webcpp/idna/detail/bidi.hpp        RFC 5893 section 2
    include/webcpp/idna/detail/process.hpp     the label driver and its sinks
    tools/generate_tables.py               the generator
    tools/unicode.json                     the pinned inputs: URLs and SHA-256
    test/Jamfile  test/.clang-tidy  test/require.hpp
    test/idna_vectors.hpp  test/nfc_vectors.hpp   the vectors' types, which include the generated rows
    test/generated/idna_test_v2.ipp        GENERATED: IdnaTestV2.txt as rows
    test/generated/nfc_test.ipp            GENERATED: NormalizationTest.txt as rows
    test/known_failures.json               tracked known failures (empty)
    test/{config,errors,options,utf8,punycode,properties,vectors,nfc,context,bidi,process,validity,uts46,allocation}_test.cpp
    example/Jamfile  example/.clang-tidy
    example/idna_{punycode,validity,quick_start,allocation}.cpp  and their .expected
    doc/Jamfile  doc/mrdocs.yml  doc/counts.py
    doc/idna.adoc and its sections: introduction, punycode, converting, validity, errors,
      allocation, limits, unicode, guarantees, testing, unicode-data (appendix)
```

---
### Task 0: The integration space, the repository and the pre-flight (controller)

**Files:**
- Create (when the formats track has not): `~/dev/webcpp-formats` and its `.local/`.
- Create: `~/dev/webcpp-formats/libs/idna` (an empty repository), the ledger.

**Interfaces:**
- Produces: the integration space on `lib/formats`; the canonical repository `~/dev/webcpp-formats/libs/idna` on an unborn `main`; the baseline, recorded.

- [ ] **Step 1: The integration space,** unless the formats track made it
  (`git -C ~/dev/webcpp worktree list` names it). The libraries' own
  submodules stay uninitialised: idna needs none, and the Jamroot and the
  lint skip an empty `libs/<name>`.

```bash
git -C ~/dev/webcpp worktree add ~/dev/webcpp-formats -b lib/formats main
mkdir ~/dev/webcpp-formats/.local
cp ~/dev/webcpp/.local/user-config.jam ~/dev/webcpp-formats/.local/user-config.jam
for name in wasi-sdk wit-bindgen wasi-wit mrdocs emsdk emscripten; do
    ln -s ~/dev/webcpp/.local/$name ~/dev/webcpp-formats/.local/$name
done
```

Expected: `git -C ~/dev/webcpp-formats status --short` prints nothing (or
`?? libs/formats/` once formats' repository exists). The copied
`user-config.jam` names `~/dev/webcpp/.local` by absolute path.

- [ ] **Step 2: idna's repository.**

```bash
git init -b main ~/dev/webcpp-formats/libs/idna
git -C ~/dev/webcpp-formats status --short
```

Expected: `Initialized empty Git repository`, then `?? libs/idna/` among
the lines: the superproject records it not.

- [ ] **Step 3: The baseline,** from `~/dev/webcpp-formats`:

```bash
L=~/dev/webcpp/.superpowers/sdd/WEBCPP_IDNA_PLAN/baseline; mkdir -p $L
$B2 -d0 --build-dir=bin/idna-baseline declared-targets > $L/targets.log 2>&1; echo "status $?"
tools/lint/lint.sh --clang-format .local/wasi-sdk/bin/clang-format --clang-tidy .local/wasi-sdk/bin/clang-tidy > $L/lint.log 2>&1; tail -1 $L/lint.log
python3 -c "import urllib.request; urllib.request.urlopen('https://www.unicode.org/Public/17.0.0/idna/IdnaMappingTable.txt', timeout=30).read(64)"; echo "network $?"
```

Expected: `status 0`; `lint: clean` (a finding here is the base's, recorded
with its commit, never later read as idna's); `network 0` (Task 3's
generator downloads its inputs once). Record `git -C ~/dev/webcpp-formats
rev-parse HEAD` and the results in the ledger.

- [ ] **Step 4: P8, recorded.** The controller's ruling of 2026-10-09: the
  spec's proposal for the Unicode data notice is followed provisionally (the
  notice in the page's appendix, one comment line in each generated file, no
  `LICENSE-UNICODE.txt`), committed locally only, and put to the owner before
  publication (Task 14). Write it in the ledger.

- [ ] **Step 5: The ledger.** One row per task: its slug, its needs (the
  execution map), its worktree, its state.

### Task 1: The library's skeleton, its errors and its options

The spec's increment I0. Worktree `~/dev/webcpp-idna-skeleton` (procedure
"Making a task's worktree", with `--orphan`); every path below is under
`libs/idna/` unless it starts with `~`. The page's first section is
`[#idna-introduction]`, the anchor formats links.

**Files:**
- Create: `build.jam`, `LICENSE_1_0.txt` (a copy of `~/dev/webcpp/LICENSE_1_0.txt`), `README.md`, `AGENTS.md`, `.gitignore`, `.gitattributes`, `.github/workflows/ci.yml`
- Create: `meta/libraries.json`, `meta/include-boundaries.json`
- Create: `include/webcpp/idna.hpp`, `include/webcpp/idna/config.hpp`, `include/webcpp/idna/errors.hpp`, `include/webcpp/idna/options.hpp`
- Create: `test/Jamfile`, `test/.clang-tidy`, `test/config_test.cpp`, `test/errors_test.cpp`, `test/options_test.cpp`
- Create: `doc/Jamfile`, `doc/mrdocs.yml`, `doc/idna.adoc`, `doc/introduction.adoc`, `doc/converting.adoc`, `doc/errors.adoc`, `doc/testing.adoc`

**Interfaces:**
- Consumes: nothing of idna. The superproject's `webcpp.targets`, `webcpp.headers-alone`, `webcpp.compile`, `webcpp.run`, `webcpp.doc`, `webcpp.reference`.
- Produces (spec section 4.1, 4.2, 4.6; frozen):

```cpp
namespace webcpp::idna {
enum class error : int { invalid_utf8 = 1, /* ... as Global Constraints, to */ storage_exhausted = 19 };
const boost::system::error_category& error_category() noexcept;  // name "webcpp.idna"
boost::system::error_code make_error_code(error e) noexcept;
template <class T> using result = boost::system::result<T>;
struct options {
    bool check_hyphens = true; bool check_bidi = true; bool check_joiners = true;
    bool use_std3_ascii_rules = true; bool verify_dns_length = true;
    bool transitional_processing = false; bool ignore_invalid_punycode = false; bool idna2008 = true;
};
inline constexpr std::size_t max_label_length = 63;
inline constexpr std::size_t max_domain_length = 253;
inline constexpr std::string_view unicode_version = "17.0.0";
}  // namespace webcpp::idna
// WEBCPP_IDNA_NO_EXCEPTIONS: <webcpp/idna/config.hpp>
```

- The `@see` titles this task's headers use, each a section of the page from this task on: "Introduction", "Converting names", "Errors", "Testing". The page's attribute `{unicode-version}` (17.0.0) is defined here.

- [ ] **Step 1: The licence.**

```bash
cd ~/dev/webcpp-idna-skeleton
cp ~/dev/webcpp/LICENSE_1_0.txt libs/idna/LICENSE_1_0.txt
git -C libs/idna branch --show-current
```

Expected: `task/skeleton`.

- [ ] **Step 2: Write the failing tests and the files b2 needs to find them.**

`build.jam`:

```
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)
#
# idna, IDNA2008 and UTS 46 for C++20. A program uses it through
# /webcpp/idna//idna, which adds its headers and Boost's.

project /webcpp/idna ;

alias idna : : : : <include>include <library>/boost//headers ;
```

`meta/libraries.json`:

```json
{
    "key": "idna",
    "name": "idna",
    "authors": ["R"],
    "description": "IDNA2008 and UTS 46 for C++20: Punycode, ToASCII and ToUnicode, and label validity, over generated Unicode 17.0 tables.",
    "category": ["String"],
    "maintainers": ["R <github.overboard312@passmail.net>"],
    "cxxstd": "20",
    "port-of": null
}
```

`meta/include-boundaries.json` (Task 2 adds the `detail` glob once a `detail`
header exists: a glob that matches no file fails the lint):

```json
{
    "boundaries": [
        {
            "headers": ["include/webcpp/idna.hpp", "include/webcpp/idna/*.hpp"],
            "must-not-include": ["boost/url", "boost/json", "srell", "webcpp/formats"],
            "why": "idna depends on webcpp::config and header-only Boost alone"
        }
    ]
}
```

`.gitignore` holds the one line `doc/html/`; `.gitattributes`:

```
* text=auto eol=lf
*.bat text eol=crlf
```

`.github/workflows/ci.yml`:

```yaml
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

# idna's CI: the superproject's reusable workflow, which builds this commit inside a checkout of
# webcpp, on every lane idna declares, with its page, the lint and the test matrix.
name: CI

on:
  push:
    branches: [main]
  pull_request:
  workflow_dispatch:

permissions:
  contents: read

concurrency:
  group: ${{ github.workflow }}-${{ github.ref }}
  # As the superproject's: a run on main is the record of main, and is never cut short by the
  # next; a pull request's newer push replaces its older run.
  cancel-in-progress: ${{ github.ref != 'refs/heads/main' }}

jobs:
  ci:
    name: CI
    uses: webcpporg/webcpp/.github/workflows/library.yml@main
    with:
      library: idna
```

`test/.clang-tidy` (pratt's, whose reason holds here):

```yaml
# The tests inherit the superproject's configuration and switch off one
# option.
#
# bugprone-exception-escape.CheckMain treats every test's main as a function
# that should not throw. A test's main lets what a failing case throws, a
# std::length_error from a std::string construction or a
# boost::system::system_error from value() on a failed result, end the
# process uncaught, which fails the test as it should. The check stays on
# for everything else: a noexcept function, a destructor, a move or a swap.
---
InheritParentConfig: true
CheckOptions:
  - key: bugprone-exception-escape.CheckMain
    value: 'false'
```

`test/Jamfile`, which later tasks extend line by line; every test is built
with the standard libraries' own checks:

```
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

# The standard libraries' own checks are on in every test, libc++'s and
# libstdc++'s (each ignores the other's macro), so that a read past the end of
# a view traps instead of reading a neighbour's memory.
project : requirements <library>/webcpp/idna//idna
  <define>_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE
  <define>_GLIBCXX_ASSERTIONS ;

import webcpp ;

webcpp.targets native wasip2 wasip3 emscripten ;

webcpp.headers-alone idna : ../include ;

# config.hpp defines WEBCPP_IDNA_NO_EXCEPTIONS exactly when the build has
# no exceptions, and keeps it when a developer defines it in a build that
# has them.
webcpp.compile config : config_test.cpp ;
webcpp.compile config_developer_defined : config_test.cpp
  : <define>WEBCPP_IDNA_NO_EXCEPTIONS <define>WEBCPP_TEST_IDNA_DEVELOPER_DEFINED ;

webcpp.run errors : errors_test.cpp ;
webcpp.run options : options_test.cpp ;
```

`test/config_test.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// <webcpp/idna/config.hpp> defines WEBCPP_IDNA_NO_EXCEPTIONS exactly when the
// build has no exceptions, and keeps the developer's own definition in a build
// that has them. test/Jamfile builds this file twice: as the build asks, and
// with the macro defined as a developer defines it, on the command line, which
// WEBCPP_TEST_IDNA_DEVELOPER_DEFINED tells this file.

#include <webcpp/idna/config.hpp>

// Boost.Config, which config.hpp includes, has said whether the build has
// exceptions: without it, BOOST_NO_EXCEPTIONS would never be defined, and the
// checks below would pass in a build without exceptions too.
#if defined(__GNUC__) && defined(__cpp_exceptions) == defined(BOOST_NO_EXCEPTIONS)
#error "config.hpp does not include <boost/config.hpp>, which defines BOOST_NO_EXCEPTIONS"
#endif

#ifdef WEBCPP_TEST_IDNA_DEVELOPER_DEFINED
#ifndef WEBCPP_IDNA_NO_EXCEPTIONS
#error "config.hpp drops WEBCPP_IDNA_NO_EXCEPTIONS, which the developer defined"
#endif
#elif defined(BOOST_NO_EXCEPTIONS) != defined(WEBCPP_IDNA_NO_EXCEPTIONS)
#error "WEBCPP_IDNA_NO_EXCEPTIONS is not defined exactly when the build has no exceptions"
#endif
```

`test/errors_test.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <webcpp/idna/errors.hpp>

#include <boost/core/lightweight_test.hpp>

#include <array>
#include <string>
#include <string_view>
#include <utility>

namespace idna = webcpp::idna;

namespace {

/** Every enumerator with its fixed value and its message, as the guide's table spells them. */
constexpr auto spellings = std::to_array<std::pair<idna::error, std::string_view>>({
    {idna::error::invalid_utf8, "invalid UTF-8"},
    {idna::error::disallowed, "disallowed code point"},
    {idna::error::not_idna2008, "code point disallowed by IDNA2008"},
    {idna::error::not_ldh, "ASCII code point other than a letter, a digit or a hyphen"},
    {idna::error::not_nfc, "label not in Normalization Form C"},
    {idna::error::hyphen_in_third_and_fourth, "hyphens in the third and fourth positions"},
    {idna::error::hyphen_at_start_or_end, "hyphen at the start or end of a label"},
    {idna::error::ace_prefix, "label starts with xn--"},
    {idna::error::leading_combining_mark, "label starts with a combining mark"},
    {idna::error::contextj, "joiner outside its allowed context"},
    {idna::error::contexto, "code point outside its allowed context"},
    {idna::error::bidi, "Bidi rule violated"},
    {idna::error::invalid_punycode, "invalid Punycode"},
    {idna::error::punycode_overflow, "Punycode overflow"},
    {idna::error::empty_label, "empty label"},
    {idna::error::label_too_long, "label longer than 63 octets"},
    {idna::error::domain_too_long, "domain name longer than 253 octets"},
    {idna::error::output_too_small, "output too small"},
    {idna::error::storage_exhausted, "label beyond the working storage"},
});

void values_are_fixed() {
    int expected = 1;
    for (const auto& [e, message] : spellings) {
        BOOST_TEST_EQ(static_cast<int>(e), expected);
        ++expected;
    }
}

void the_category_is_webcpp_idna() {
    BOOST_TEST_EQ(std::string(idna::error_category().name()), "webcpp.idna");
    for (const auto& [e, message] : spellings) {
        const boost::system::error_code code = e;
        BOOST_TEST(code.category() == idna::error_category());
        BOOST_TEST_EQ(code.message(), message);
        BOOST_TEST(idna::make_error_code(e) == code);
    }
    BOOST_TEST_EQ(idna::error_category().message(0), "unknown webcpp.idna error");
    BOOST_TEST_EQ(idna::error_category().message(20), "unknown webcpp.idna error");
}

/** Two categories compare by their 64-bit identity, the ASCII of "idna" and four zero bytes. */
void the_identity_is_fixed() {
    const idna::detail::category other;
    BOOST_TEST(other == idna::error_category());
}

/** The message copied into a caller's buffer: cut to fit with its terminating zero, never written
    past the size given, and returned whole whatever the buffer. */
void a_message_fits_the_buffer_it_is_given() {
    const boost::system::error_category& category = idna::error_category();
    const int value = static_cast<int>(idna::error::invalid_punycode);
    constexpr std::string_view whole = "invalid Punycode";
    std::array<char, whole.size() + 2> buffer{};

    BOOST_TEST_EQ(std::string_view(category.message(value, nullptr, 8)), whole);

    buffer.fill('x');
    BOOST_TEST_EQ(std::string_view(category.message(value, buffer.data(), 0)), whole);
    BOOST_TEST_EQ(buffer[0], 'x');

    buffer.fill('x');
    BOOST_TEST_EQ(std::string_view(category.message(value, buffer.data(), 8)), whole);
    BOOST_TEST_EQ(std::string_view(buffer.data()), "invalid");
    BOOST_TEST_EQ(buffer[8], 'x');

    buffer.fill('x');
    BOOST_TEST_EQ(std::string_view(category.message(value, buffer.data(), whole.size() + 1)),
                  whole);
    BOOST_TEST_EQ(std::string_view(buffer.data()), whole);
    BOOST_TEST_EQ(buffer[whole.size() + 1], 'x');
}

void a_result_holds_an_error() {
    const idna::result<int> r = idna::error::invalid_utf8;
    BOOST_TEST(r.has_error());
    BOOST_TEST(r.error() == idna::error::invalid_utf8);
}

}  // namespace

int main() {
    values_are_fixed();
    the_category_is_webcpp_idna();
    the_identity_is_fixed();
    a_message_fits_the_buffer_it_is_given();
    a_result_holds_an_error();
    return boost::report_errors();
}
```

`test/options_test.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <webcpp/idna/options.hpp>

#include <boost/core/lightweight_test.hpp>

#include <string_view>

namespace idna = webcpp::idna;

namespace {

/** The defaults are the strict profile: every check of UTS 46 on, nontransitional, the DNS
    lengths verified, and IDNA2008 on top. */
void the_defaults_are_strict() {
    constexpr idna::options defaults;
    BOOST_TEST(defaults.check_hyphens);
    BOOST_TEST(defaults.check_bidi);
    BOOST_TEST(defaults.check_joiners);
    BOOST_TEST(defaults.use_std3_ascii_rules);
    BOOST_TEST(defaults.verify_dns_length);
    BOOST_TEST(!defaults.transitional_processing);
    BOOST_TEST(!defaults.ignore_invalid_punycode);
    BOOST_TEST(defaults.idna2008);
}

/** An aggregate: a caller names what changes and keeps the rest. */
void a_caller_names_what_changes() {
    constexpr idna::options uts46_alone{.idna2008 = false};
    BOOST_TEST(!uts46_alone.idna2008);
    BOOST_TEST(uts46_alone.check_bidi);
    BOOST_TEST(uts46_alone.verify_dns_length);
}

void the_limits_are_the_dns() {
    BOOST_TEST_EQ(idna::max_label_length, 63U);
    BOOST_TEST_EQ(idna::max_domain_length, 253U);
    BOOST_TEST_EQ(idna::unicode_version, std::string_view("17.0.0"));
}

}  // namespace

int main() {
    the_defaults_are_strict();
    a_caller_names_what_changes();
    the_limits_are_the_dns();
    return boost::report_errors();
}
```

- [ ] **Step 3: Run them and watch them fail.**

```bash
$B2 libs/idna/test
```

Expected: FAIL: the tests do not compile, `fatal error:
'webcpp/idna/errors.hpp' file not found` (and `config.hpp`, `options.hpp`),
or b2 stops at `webcpp.headers-alone`, which finds no header under
`../include` yet. Either way, nothing passes.

- [ ] **Step 4: Write the headers.**

`include/webcpp/idna/config.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The configuration of idna, which each of its headers includes first: whether
 idna is built without exceptions.

 @see "Testing", in the guide.
*/
#ifndef WEBCPP_IDNA_CONFIG_HPP
#define WEBCPP_IDNA_CONFIG_HPP

// webcpp::config does not exist yet: when it does, this line becomes
// #include <webcpp/config.hpp>, and build.jam adds /webcpp/config//config.
#include <boost/config.hpp>

// MrDocs parses idna as a build with exceptions does, where the macro is not
// defined: this definition, undone at once, is the one the reference lists.
#ifdef __MRDOCS__
/**
 Defined when idna is built without exceptions: automatically when the
 compiler has none (`BOOST_NO_EXCEPTIONS`), or by the developer to disable them
 in a build that has them.

 idna raises no exception of its own: every operation that can fail returns
 its error, so the macro changes nothing in it. It is provided so that a
 program can set it for every webcpp library alike. An exception idna raised
 would go through `boost::throw_exception`, and an API that throws would be
 absent while this is defined.

 @see "Testing", in the guide.
*/
#define WEBCPP_IDNA_NO_EXCEPTIONS
#undef WEBCPP_IDNA_NO_EXCEPTIONS
#endif

#if defined(BOOST_NO_EXCEPTIONS) && !defined(WEBCPP_IDNA_NO_EXCEPTIONS)
#define WEBCPP_IDNA_NO_EXCEPTIONS
#endif

#endif  // WEBCPP_IDNA_CONFIG_HPP
```

`include/webcpp/idna/errors.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The errors idna reports, as `boost::system::error_code` values of a category
 of its own, and the result type of every operation that can fail.

 @see "Errors", in the guide.
*/
#ifndef WEBCPP_IDNA_ERRORS_HPP
#define WEBCPP_IDNA_ERRORS_HPP

#include <webcpp/idna/config.hpp>

#include <boost/system/error_category.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/result.hpp>

#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>
#include <type_traits>

namespace webcpp::idna {

/**
 The errors idna reports, as `boost::system::error_code` values of the category
 @ref error_category.

 One enumerator per rule family, not per UTS 46 status code: the comment of
 each names the codes of UTS 46 and the sections of the RFCs it reports. The
 values start at 1, so an `error_code` of value 0 is no error.

 @note The values are fixed: they are never reused for a different meaning,
 so a stored value keeps its meaning across versions.
 @see "Errors", in the guide.
*/
enum class error : int {
    /** The input is not well-formed UTF-8 (RFC 3629). */
    invalid_utf8 = 1,

    /** A code point whose UTS 46 status is neither valid nor, in nontransitional processing,
        deviation (UTS 46 V7). */
    disallowed = 2,

    /** A code point UTS 46 marks NV8 or XV8: DISALLOWED in IDNA2008 (options::idna2008). */
    not_idna2008 = 3,

    /** An ASCII code point outside a-z, 0-9 and the hyphen (UTS 46 U1,
        options::use_std3_ascii_rules). */
    not_ldh = 4,

    /** A label not in Normalization Form C (UTS 46 V1, RFC 5891 section 5.4). */
    not_nfc = 5,

    /** Hyphens in both the third and the fourth positions (UTS 46 V2, options::check_hyphens). */
    hyphen_in_third_and_fourth = 6,

    /** A hyphen at the start or the end of a label (UTS 46 V3, options::check_hyphens). */
    hyphen_at_start_or_end = 7,

    /** A label that starts with "xn--" while options::check_hyphens is off (UTS 46 V4). */
    ace_prefix = 8,

    /** A label that starts with a combining mark (UTS 46 V6, RFC 5891 section 5.4). */
    leading_combining_mark = 9,

    /** A joiner outside the context RFC 5892 A.1 and A.2 allow (UTS 46 C1, C2,
        options::check_joiners). */
    contextj = 10,

    /** A code point outside the context RFC 5892 A.3 to A.9 allow (options::idna2008). */
    contexto = 11,

    /** A Bidi domain name with a label that breaks RFC 5893 section 2 (UTS 46 B1 to B6,
        options::check_bidi). */
    bidi = 12,

    /** Punycode that does not decode, an A-label that decodes to nothing or to ASCII alone, or
        one not in its canonical form (RFC 3492, UTS 46 P4, RFC 5891 section 5.4). */
    invalid_punycode = 13,

    /** Punycode whose arithmetic overflows 32 bits (RFC 3492 section 6.4, UTS 46 A3). */
    punycode_overflow = 14,

    /** An empty label (UTS 46 A4_2 and X4_2, options::verify_dns_length). */
    empty_label = 15,

    /** A label longer than 63 octets in its ASCII form (UTS 46 A4_2). */
    label_too_long = 16,

    /** A domain name longer than 253 octets in its ASCII form (UTS 46 A4_1). */
    domain_too_long = 17,

    /** The caller's span cannot hold the result. */
    output_too_small = 18,

    /** A label beyond the inline working storage, with no memory resource to hold it. */
    storage_exhausted = 19,
};

namespace detail {

/** The human message of an @ref error value, or the unknown message for a value it does not
    define. */
inline const char* message_of(int value) noexcept {
    switch (static_cast<error>(value)) {
        case error::invalid_utf8: return "invalid UTF-8";
        case error::disallowed: return "disallowed code point";
        case error::not_idna2008: return "code point disallowed by IDNA2008";
        case error::not_ldh: return "ASCII code point other than a letter, a digit or a hyphen";
        case error::not_nfc: return "label not in Normalization Form C";
        case error::hyphen_in_third_and_fourth: return "hyphens in the third and fourth positions";
        case error::hyphen_at_start_or_end: return "hyphen at the start or end of a label";
        case error::ace_prefix: return "label starts with xn--";
        case error::leading_combining_mark: return "label starts with a combining mark";
        case error::contextj: return "joiner outside its allowed context";
        case error::contexto: return "code point outside its allowed context";
        case error::bidi: return "Bidi rule violated";
        case error::invalid_punycode: return "invalid Punycode";
        case error::punycode_overflow: return "Punycode overflow";
        case error::empty_label: return "empty label";
        case error::label_too_long: return "label longer than 63 octets";
        case error::domain_too_long: return "domain name longer than 253 octets";
        case error::output_too_small: return "output too small";
        case error::storage_exhausted: return "label beyond the working storage";
    }
    return "unknown webcpp.idna error";
}

/**
 A constant category with a fixed identity, so it holds no mutable state.

 @note The identity is the ASCII of "idna" followed by four zero bytes.
*/
class category final : public boost::system::error_category {
public:
    /** Constructs the category with its fixed identity. */
    constexpr category() noexcept : boost::system::error_category(0x69646E6100000000ULL) {}

    /** The category's name, "webcpp.idna". */
    const char* name() const noexcept override { return "webcpp.idna"; }

    /** The human message of the error `value`, as @ref message_of spells it. */
    std::string message(int value) const override { return message_of(value); }

    /**
     Copies the human message of the error `value` into `buffer`, cut to fit `size` bytes with
     its terminating zero, and returns the whole message.
    */
    const char* message(int value, char* buffer, std::size_t size) const noexcept override {
        const char* text = message_of(value);
        if (buffer != nullptr && size != 0) {
            const std::string_view spelling(text);
            const std::size_t limit = size - 1;
            const std::size_t count = spelling.size() < limit ? spelling.size() : limit;
            std::memcpy(buffer, spelling.data(), count);
            buffer[count] = '\0';
        }
        return text;
    }
};

/** The one category of idna's errors, which @ref error_category returns. */
inline constexpr category the_category{};

}  // namespace detail

/**
 The category of every @ref error.

 Its `name()` is "webcpp.idna", and its `message(value)` the human message of
 the error, "invalid Punycode" for @ref error::invalid_punycode, or "unknown
 webcpp.idna error" for a value @ref error does not define.

 @return The category, a constant with a fixed identity.
 @see "Errors", in the guide.
*/
inline const boost::system::error_category& error_category() noexcept {
    return detail::the_category;
}

/**
 Makes the `error_code` of an @ref error, as its implicit conversion does.

 @param e The error.
 @return `e` as an `error_code` of the category @ref error_category.
*/
inline boost::system::error_code make_error_code(error e) noexcept {
    return {static_cast<int>(e), error_category()};
}

/**
 What an operation that can fail returns: a `T`, or the `error_code` of the
 failure.

 It is Boost.System's `result`. idna throws nothing of its own: every
 operation of it that can fail returns one, whose error is an @ref error.

 @tparam T The type of the value; `void` for an operation that returns none.
 @see "Errors", in the guide.
*/
template <class T>
using result = boost::system::result<T>;

}  // namespace webcpp::idna

namespace boost::system {
template <>
struct is_error_code_enum<webcpp::idna::error> : std::true_type {};
}  // namespace boost::system

#endif  // WEBCPP_IDNA_ERRORS_HPP
```

`include/webcpp/idna/options.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The options of every conversion and validation, the length limits of the DNS,
 and the version of Unicode the tables follow.

 @see "Converting names", in the guide.
*/
#ifndef WEBCPP_IDNA_OPTIONS_HPP
#define WEBCPP_IDNA_OPTIONS_HPP

#include <webcpp/idna/config.hpp>

#include <cstddef>
#include <string_view>

namespace webcpp::idna {

/**
 The flags of UTS 46 section 4, and IDNA2008's stricter validity.

 The defaults are the strict profile: every check of UTS 46 on, nontransitional
 processing, the DNS lengths verified, and IDNA2008's validity on top. An
 aggregate, so a caller names only what changes:
 `idna::options{.idna2008 = false}` is UTS 46 alone.

 @see "Converting names", in the guide.
*/
struct options {
    /** CheckHyphens: no hyphens in the third and fourth positions, none at either end (UTS 46
        V2, V3); when false, only the "xn--" prefix is refused (V4). */
    bool check_hyphens = true;

    /** CheckBidi: every label of a Bidi domain name satisfies RFC 5893 section 2. */
    bool check_bidi = true;

    /** CheckJoiners: the joiners U+200C and U+200D satisfy RFC 5892 A.1 and A.2. */
    bool check_joiners = true;

    /** UseSTD3ASCIIRules: the only ASCII code points are a-z, 0-9 and the hyphen (UTS 46 U1). */
    bool use_std3_ascii_rules = true;

    /** VerifyDnsLength: labels of 1 to 63 octets, a name of 1 to 253, and no empty root label. */
    bool verify_dns_length = true;

    /** Transitional_Processing, which UTS 46 deprecates: the deviations are mapped. */
    bool transitional_processing = false;

    /** IgnoreInvalidPunycode: an "xn--" label that does not decode is kept as it is, without an
        error. */
    bool ignore_invalid_punycode = false;

    /** IDNA2008's validity on top of UTS 46: a code point UTS 46 marks NV8 or XV8 is refused, and
        the CONTEXTO rules of RFC 5892 A.3 to A.9 apply. */
    bool idna2008 = true;
};

/** The longest label of a domain name in its ASCII form, in octets (RFC 1034 section 3.1). */
inline constexpr std::size_t max_label_length = 63;

/** The longest domain name in its ASCII form, in octets, without a trailing root label. */
inline constexpr std::size_t max_domain_length = 253;

/** The version of Unicode whose data the tables were generated from. */
inline constexpr std::string_view unicode_version = "17.0.0";

}  // namespace webcpp::idna

#endif  // WEBCPP_IDNA_OPTIONS_HPP
```

`include/webcpp/idna.hpp`, which later tasks extend by one `#include` each:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 Every public header of idna: Punycode, UTS 46's conversions, and IDNA2008's
 validity.

 @see "Introduction", in the guide.
*/
#ifndef WEBCPP_IDNA_HPP
#define WEBCPP_IDNA_HPP

#include <webcpp/idna/config.hpp>

#include <webcpp/idna/errors.hpp>
#include <webcpp/idna/options.hpp>

#endif  // WEBCPP_IDNA_HPP
```

- [ ] **Step 5: Run them and watch them pass,** natively:

```bash
$B2 libs/idna/test
```

Expected: `**passed**` for `errors`, `options` and each `alone-*` header,
`config` and `config_developer_defined` compiled, no `...failed`.

- [ ] **Step 6: The page.** `doc/Jamfile`:

```
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

import webcpp ;

webcpp.doc idna : idna.adoc ;
webcpp.reference idna ;
```

`doc/mrdocs.yml`:

```yaml
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)
#
# What idna's reference adds to tools/doc/mrdocs.yml.in, the settings every
# library's reference shares: only keys of how the reference is presented.

# Each enumerator has a section of its own, with its value and its whole Doc
# Comment: the values of error are fixed, and the enum's table of members
# would show each enumerator's brief alone.
show-enum-constants: true
```

`doc/idna.adoc` (later tasks add their `include::` lines, in the order of the
final file, Task 13):

```asciidoc
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

= idna: internationalized domain names for {cpp}
:toc: left
:toclevels: 1
:idprefix:
:sectanchors:
:example-caption!:
:attribute-missing: warn
:unicode-version: 17.0.0

idna is a header-only library of internationalized domain names in {cpp}20,
one of the libraries of https://github.com/webcpporg/webcpp[webcpp]:
Punycode (RFC 3492), Unicode's UTS 46 processing, to ASCII and to Unicode,
and the validity rules of IDNA2008 (RFC 5890 to 5893), over Unicode
{unicode-version} tables generated into its headers. Every operation returns
its error as a value, and every operation that writes into a caller's buffer
allocates nothing.

This page teaches idna concept by concept: why each exists, its smallest
example, its variations and its rules. Every program it shows was compiled
and run, natively and for wasm32-wasip2, wasm32-wasip3 and Emscripten, and
printed the output shown under it. The <<reference,reference>> that closes
it lists every name the headers declare.

include::introduction.adoc[]

include::converting.adoc[]

include::errors.adoc[]

include::testing.adoc[]

[#reference]
== Reference

Every name idna declares, in the namespaces `webcpp::idna` and
`webcpp::idna::punycode`, from the Doc Comments of its headers: each
function, type, enumerator and constant with its declaration, what it does,
what it asks of its caller and how it fails. The other sections teach the
library; this one is where a name is looked up.

include::{reference}[leveloffset=+1]
```

`doc/introduction.adoc` (Tasks 2, 10 and 12 add a row each to its table of
headers, and Task 12 a first example):

```asciidoc
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

[#idna-introduction]
== Introduction

The labels of a domain name were ASCII: letters, digits and the hyphen. An
internationalized domain name lets a label hold any script, `bücher.example`
or `παράδειγμα.δοκιμή`, and still reach the DNS, which stays ASCII: a label
that is not ASCII travels as an A-label, `xn--` followed by the label's code
points in Punycode, so `bücher.example` is looked up as
`xn--bcher-kva.example`.

Two standards say which labels are valid, and how a name a person typed
becomes one:

* *IDNA2008*, RFC 5890 to 5893, defines a valid label: the property of each
  code point, derived from Unicode (PVALID; CONTEXTJ and CONTEXTO, valid in
  a context the RFC states; DISALLOWED), Normalization Form C, the hyphens,
  and the Bidi rule of a name that mixes right-to-left and left-to-right
  labels. It maps nothing: `bücher` is a valid label, and `Bücher` is not.
* *UTS 46*, Unicode's IDNA Compatibility Processing, first maps what was
  typed (case, compatibility forms, the full stops of other scripts), then
  normalizes it, then checks it, by rules close to IDNA2008's but not the
  same: it accepts symbols such as `¡`, which IDNA2008 disallows, and has no
  CONTEXTO rules at all.

idna implements both, and its default profile is the stricter union of the
two: UTS 46's mapping, then IDNA2008's validity on top of UTS 46's. It is what
JSON Schema's `idn-hostname` format asks for, and what no UTS 46 library
measured against that format's tests gives alone: each accepts the labels
IDNA2008 refuses. `idna::options{.idna2008 = false}` is UTS 46 alone.

[#idna-introduction-headers]
=== Headers

`<webcpp/idna.hpp>` includes every public header; the names are in the
namespace `webcpp::idna`, and Punycode's in `webcpp::idna::punycode`:

[cols="1,3",options="header"]
|===
| Header | What it declares

| `<webcpp/idna/errors.hpp>`
| `idna::error`, its category, and `idna::result`.

| `<webcpp/idna/options.hpp>`
| `idna::options`, the DNS lengths, and the version of Unicode.
|===

[#idna-requirements]
=== Requirements

* *A {cpp}20 compiler.* idna's tests and examples are built and run on every
  native lane of webcpp's continuous integration, for wasm32-wasip2 and
  wasm32-wasip3 with the Clang of wasi-sdk, and with Emscripten under Node.
* *Boost's headers*, 1.92 or newer: Config, System, Core and Assert. Nothing
  to build or link, and nothing else.
```

`doc/converting.adoc` (Task 12 adds the conversions before its options):

```asciidoc
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

[#idna-converting]
== Converting names

[#idna-converting-options]
=== Options

Every conversion and every check takes an `idna::options`, the flags of
UTS 46 section 4 and one of idna's own, `idna2008`. The defaults are the
strict profile: every check on, nontransitional processing, the DNS lengths
verified, and IDNA2008's validity on top. `idna::options` is an aggregate, so
a program names only what changes: `idna::options{.idna2008 = false}` is
UTS 46 alone, and `idna::options{.verify_dns_length = false}` lets labels of
any length through.

[cols="2,2,1,4",options="header"]
|===
| Member | UTS 46's flag | Default | When true

| `check_hyphens`
| CheckHyphens
| true
| No hyphen in both the third and the fourth position of a label (V2), none
  at either end (V3). When false, only a label that starts with `xn--` is
  refused (V4).

| `check_bidi`
| CheckBidi
| true
| Every label of a Bidi domain name, one that holds an R, AL or AN code
  point, satisfies the six rules of RFC 5893 section 2.

| `check_joiners`
| CheckJoiners
| true
| U+200C ZERO WIDTH NON-JOINER and U+200D ZERO WIDTH JOINER stand only where
  RFC 5892 A.1 and A.2 allow them.

| `use_std3_ascii_rules`
| UseSTD3ASCIIRules
| true
| The only ASCII code points of a label are `a` to `z`, `0` to `9` and the
  hyphen (U1).

| `verify_dns_length`
| VerifyDnsLength
| true
| Every label is 1 to 63 octets in its ASCII form, the name 1 to 253, and a
  final empty root label is refused (A4_1, A4_2).

| `transitional_processing`
| Transitional_Processing
| false
| The deviations are mapped: `ß` to `ss`, `ς` to `σ`, the joiners removed.
  UTS 46 deprecates it.

| `ignore_invalid_punycode`
| IgnoreInvalidPunycode
| false
| A label that starts with `xn--` and does not decode is kept as it is,
  without an error.

| `idna2008`
| none
| true
| IDNA2008's validity on top of UTS 46's: a code point that UTS 46 marks NV8
  or XV8, DISALLOWED in IDNA2008, is refused, and the CONTEXTO rules of
  RFC 5892 A.3 to A.9 apply.
|===
```

`doc/errors.adoc`:

```asciidoc
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

[#idna-errors]
== Errors

Every operation of idna that can fail returns its error as a value: an
`idna::result<T>`, Boost.System's `result`, or, for `to_unicode`, which
always converts what it can, a struct whose `error` is an
`boost::system::error_code`. idna throws nothing of its own. Its errors are
the values of `idna::error`, of the category `webcpp.idna`, one per family
of rules rather than one per status code of UTS 46: IdnaTestV2, Unicode's
own conformance file, says an implementation "need only record that there
is an error". The first error found is the one reported: the labels are
processed in order, and the checks of a label in the order UTS 46 section 4.1
lists them, IDNA2008's last.

[cols="1,3,3,3",options="header"]
|===
| Value | Enumerator | Reports | Message

| 1 | `invalid_utf8` | input that is not well-formed UTF-8 (RFC 3629) | invalid UTF-8
| 2 | `disallowed` | UTS 46 V7: a status other than valid, or deviation when nontransitional; a U+002E inside one label | disallowed code point
| 3 | `not_idna2008` | a code point UTS 46 marks NV8 or XV8, DISALLOWED in IDNA2008 | code point disallowed by IDNA2008
| 4 | `not_ldh` | UTS 46 U1: ASCII other than `a` to `z`, `0` to `9` and the hyphen | ASCII code point other than a letter, a digit or a hyphen
| 5 | `not_nfc` | UTS 46 V1, RFC 5891 section 5.4 | label not in Normalization Form C
| 6 | `hyphen_in_third_and_fourth` | UTS 46 V2 | hyphens in the third and fourth positions
| 7 | `hyphen_at_start_or_end` | UTS 46 V3 | hyphen at the start or end of a label
| 8 | `ace_prefix` | UTS 46 V4: `xn--` while `check_hyphens` is off | label starts with xn--
| 9 | `leading_combining_mark` | UTS 46 V6, RFC 5891 section 5.4 | label starts with a combining mark
| 10 | `contextj` | UTS 46 C1, C2: RFC 5892 A.1, A.2 | joiner outside its allowed context
| 11 | `contexto` | RFC 5892 A.3 to A.9 | code point outside its allowed context
| 12 | `bidi` | UTS 46 B1 to B6: RFC 5893 section 2 | Bidi rule violated
| 13 | `invalid_punycode` | RFC 3492 decoding; UTS 46 P4; an A-label not in its canonical form | invalid Punycode
| 14 | `punycode_overflow` | RFC 3492 section 6.4; UTS 46 A3 | Punycode overflow
| 15 | `empty_label` | UTS 46 A4_2 and X4_2 | empty label
| 16 | `label_too_long` | UTS 46 A4_2: more than 63 octets | label longer than 63 octets
| 17 | `domain_too_long` | UTS 46 A4_1: more than 253 octets | domain name longer than 253 octets
| 18 | `output_too_small` | the caller's span cannot hold the result | output too small
| 19 | `storage_exhausted` | a label beyond the inline working storage, with no memory resource | label beyond the working storage
|===

The values are fixed: none changes, and none is reused for another meaning.
An `idna::error` converts to an `error_code` and compares with one,
`result.error() == idna::error::contexto`.
```

`doc/testing.adoc` (each later task adds its tests' rows; Task 13 writes it
whole):

```asciidoc
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

[#idna-testing]
== Testing

[#idna-testing-library]
=== How idna is tested

idna's tests are lightweight_test programs, one per subject, under `test/`,
each declared in `test/Jamfile` with webcpp's `webcpp.run`, and run on every
target the Jamfile declares:

[cols="1,3",options="header"]
|===
| Test | What it checks

| `errors`
| The values of `idna::error`, which are fixed, their messages, the
  category's name and identity, and the message cut to a caller's buffer.

| `options`
| The strict profile's defaults, the designated initializers of the
  aggregate, and the DNS lengths.
|===

Beside them, `test/Jamfile` compiles each of the {n-headers} headers alone,
and `config.hpp` twice, as the build asks and with
`WEBCPP_IDNA_NO_EXCEPTIONS` defined by the developer.

[#idna-testing-targets]
=== Targets and exceptions

`test/Jamfile` declares `webcpp.targets native wasip2 wasip3 emscripten`:
natively with any toolset, for wasm32-wasip2 and wasm32-wasip3 with the
toolsets `clang-wasip2` and `clang-wasip3` run by wasmtime, and with
Emscripten run by Node.

`<webcpp/idna/config.hpp>`, which every public header includes first,
defines `WEBCPP_IDNA_NO_EXCEPTIONS` when the build has no exceptions, and a
program may define it itself, as Boost.Asio's `BOOST_ASIO_NO_EXCEPTIONS`.
idna raises no exception of its own, so the macro changes nothing in it: it
exists so that a program can set it for every webcpp library alike. The
lint compiles every header and test a second time without exceptions.
```

`README.md`:

```markdown
# idna

idna is a header-only library of internationalized domain names for C++20,
one of the libraries of [webcpp](https://github.com/webcpporg/webcpp):
Punycode (RFC 3492), Unicode's UTS 46 processing, ToASCII and ToUnicode,
and the validity rules of IDNA2008 (RFC 5890 to 5893), over Unicode 17.0.0
tables generated into its headers.

Its default profile is UTS 46's mapping followed by IDNA2008's validity, the
strict profile JSON Schema's `idn-hostname` format asks for; `idna::options`
turns each check on or off. Every operation returns its error as a value,
the forms that write into a caller's buffer allocate nothing, and the forms
that return a string take a `std::pmr::memory_resource`.

`<webcpp/idna.hpp>` includes every public header; the names are in the
namespace `webcpp::idna`. idna needs Boost's headers and nothing to build or
link. It runs natively, on wasm32-wasip2 and wasm32-wasip3, and with
Emscripten.

## Building and testing

idna is developed inside the webcpp superproject, as a Boost library is
developed inside Boost:

    git clone --recursive https://github.com/webcpporg/webcpp
    cd webcpp
    b2 libs/idna/test libs/idna/example

builds the tests and the examples natively, runs every example and compares
its output with the `.expected` file beside it.
`toolset=clang-wasip2 testing.launcher=wasmtime`,
`toolset=clang-wasip3 testing.launcher=wasmtime` and `toolset=emscripten`
do the same for WebAssembly. A b2 project uses idna through
`/webcpp/idna//idna`.

## Documentation

idna's page, with its API reference, is published at
<https://webcpporg.github.io/webcpp/libs/idna/>; `b2 libs/idna/doc` builds
it into `doc/html/index.html`.

## License

Distributed under the [Boost Software License, Version 1.0](LICENSE_1_0.txt).
```

`AGENTS.md`:

```markdown
# Working on idna

webcpp's rules apply here: read the superproject's
[AGENTS.md](../../AGENTS.md)
(<https://github.com/webcpporg/webcpp/blob/main/AGENTS.md>) first. This file
holds only what is specific to idna.

## What idna is

A library of webcpp's own, not a port: `meta/libraries.json` has
`"port-of": null`, and idna has no oracle, no twins and no "Differences"
appendix. Its page, `doc/idna.adoc`, with the Doc Comments of its headers, is
what a change is checked against, and its conformance is Unicode's own test
files.

ada-url/idna is a reference for algorithms and table packing only: nothing of
it is copied, and no file of it enters this repository.

## The strict profile

`idna::options{}` is UTS 46 with every check on, nontransitional, the DNS
lengths verified, and `idna2008 = true`: IDNA2008's validity on top of
UTS 46's. UTS 46 alone accepts what IDNA2008 refuses (the code points it
marks NV8 and XV8, and every CONTEXTO violation, since it has no CONTEXTO
rules), and JSON Schema's `idn-hostname`, idna's first consumer through
`webcpp::formats`, asks for IDNA2008. The default never changes to a looser
one.

## Cautions

- **The errors are fixed.** `error`'s values 1 to 19 never change and are
  never reused; a new error takes the next value. The category's name,
  `webcpp.idna`, and its identity, `0x69646E6100000000`, never change either.
  `test/errors_test.cpp` pins them.
- **The public interface is a contract.** `webcpp::formats` codes against the
  signatures of `errors.hpp`, `options.hpp`, `punycode.hpp`, `validity.hpp`
  and `uts46.hpp`; a change to one is announced to it. Everything in
  `detail` may change freely.
- **No exceptions in a header.** The headers compile without exceptions,
  which the lint checks; a `noexcept` function reads a `result` it has
  checked with `unsafe_value()`, since Boost 1.92's `operator*` calls
  `value()`, which throws, and clang-tidy's `bugprone-exception-escape` then
  fails the function.
- **`config.hpp` comes first.** Every public header includes
  `<webcpp/idna/config.hpp>` first. It includes `<boost/config.hpp>` until
  `webcpp::config` exists; then that line becomes
  `#include <webcpp/config.hpp>`, and `build.jam`'s alias gains
  `<library>/webcpp/config//config`, in one commit.
- **Literals.** A narrow string spells a byte above 0x7F as a three-digit
  octal escape, `"b\303\274cher"`: a hex escape runs into the letters after
  it, and raw UTF-8 is read in the machine's code page by MSVC. A `U""`
  string uses `\u` and `\U`, and `\x` only for a value no universal character
  name may name. A default `options` is written `idna::options()`:
  clang-tidy's `readability-trailing-comma` misreads an empty `options{}`
  followed by a comma.

## The page

Each `@see "<title>"` of a Doc Comment names a top-level section of the page:
"Introduction", "Punycode", "Converting names", "Validity", "Errors",
"Allocation", "Testing". Renaming a section renames its `@see` lines in the
same commit; doc-check fails otherwise.
```

- [ ] **Step 7: Every lane, the page and the lint** (procedure "The task's
  lanes"; no example yet):

```bash
cd ~/dev/webcpp-idna-skeleton
L=~/dev/webcpp/.superpowers/sdd/WEBCPP_IDNA_PLAN/skeleton
export EM_CACHE=~/dev/webcpp/.local/emscripten-cache
EXAMPLES="$(test -f libs/idna/example/Jamfile && echo libs/idna/example)"
$B2 -a --build-dir=bin/idna-native libs/idna/test $EXAMPLES > $L/native.log 2>&1 &
$B2 -a --build-dir=bin/idna-gcc toolset=gcc-15 libs/idna/test $EXAMPLES > $L/gcc.log 2>&1 &
$B2 -a --build-dir=bin/idna-wasip2 toolset=clang-wasip2 testing.launcher=wasmtime libs/idna/test $EXAMPLES > $L/wasip2.log 2>&1 &
$B2 -a --build-dir=bin/idna-wasip3 toolset=clang-wasip3 testing.launcher=wasmtime libs/idna/test $EXAMPLES > $L/wasip3.log 2>&1 &
$B2 -a --build-dir=bin/idna-emscripten toolset=emscripten libs/idna/test $EXAMPLES > $L/emscripten.log 2>&1 &
wait
grep -L -e '\.\.\.failed' $L/{native,gcc,wasip2,wasip3,emscripten}.log
$B2 -a --build-dir=bin/idna-doc libs/idna/doc > $L/doc.log 2>&1; echo "doc $?"
tools/lint/lint.sh --clang-format .local/wasi-sdk/bin/clang-format --clang-tidy .local/wasi-sdk/bin/clang-tidy > $L/lint.log 2>&1; tail -1 $L/lint.log
```

Also run `$B2 -d0 --build-dir=bin/idna-targets declared-targets | grep '^idna '`:
Expected four lines naming `emscripten`, `native`, `wasip2` and `wasip3`, in
any order. Then plant `#include <boost/json.hpp>` in `errors.hpp`, run the
lint and see the include-boundaries rule name that line, and remove it
(`git -C libs/idna status --short` then shows only this task's new files).

- [ ] **Step 8: Commit.**

```bash
cd ~/dev/webcpp-idna-skeleton/libs/idna
git add build.jam LICENSE_1_0.txt README.md AGENTS.md .gitignore .gitattributes \
    .github/workflows/ci.yml meta/libraries.json meta/include-boundaries.json \
    include/webcpp/idna.hpp include/webcpp/idna/config.hpp include/webcpp/idna/errors.hpp \
    include/webcpp/idna/options.hpp test/Jamfile test/.clang-tidy test/config_test.cpp \
    test/errors_test.cpp test/options_test.cpp doc/Jamfile doc/mrdocs.yml doc/idna.adoc \
    doc/introduction.adoc doc/converting.adoc doc/errors.adoc doc/testing.adoc
git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "idna: the library's skeleton, its errors and its options"
git log --format='%an %ae %cn %ce'
```

Expected: `R github.overboard312@passmail.net R github.overboard312@passmail.net`.

- [ ] **Step 9: Report,** ending with:

```markdown
## Changelog

### New Libraries

- idna: IDNA2008 and UTS 46 for C++20, with Punycode and generated Unicode
  17.0 tables, from R.
```

### Task 2: Punycode, and strict UTF-8

The spec's increment I1. Worktree `~/dev/webcpp-idna-punycode`, from `main`
with Task 1 integrated. Runs at the same time as Tasks 3, 4, 5 and 8, each
in its own worktree.

**Files:**
- Create: `include/webcpp/idna/detail/utf8.hpp`, `include/webcpp/idna/punycode.hpp`
- Create: `test/utf8_test.cpp`, `test/punycode_test.cpp`, `test/allocation_test.cpp`
- Create: `example/Jamfile`, `example/.clang-tidy`, `example/idna_punycode.cpp`, `example/idna_punycode.expected`
- Create: `doc/punycode.adoc`, `doc/allocation.adoc`
- Modify: `include/webcpp/idna.hpp`, `meta/include-boundaries.json`, `test/Jamfile`, `doc/idna.adoc`, `doc/introduction.adoc`, `doc/testing.adoc`

**Interfaces:**
- Consumes: Task 1's `error`, `result`.
- Produces (spec section 4.3; frozen):

```cpp
namespace webcpp::idna::punycode {
[[nodiscard]] result<std::size_t> encode(std::span<char> output, std::u32string_view input) noexcept;
[[nodiscard]] result<std::size_t> decode(std::span<char32_t> output, std::string_view input) noexcept;
[[nodiscard]] result<std::pmr::string> encode(std::u32string_view input,
    std::pmr::memory_resource& resource = *std::pmr::get_default_resource());
[[nodiscard]] result<std::pmr::u32string> decode(std::string_view input,
    std::pmr::memory_resource& resource = *std::pmr::get_default_resource());
}
```

- Produces (detail, for Tasks 9 and 11):
  - `detail::utf8::decode(std::string_view text, std::size_t& i, char32_t& cp) -> bool`, `is_valid(std::string_view) -> bool`, `length(char32_t) -> std::size_t`, `encode(char32_t, char*) -> std::size_t`, all `constexpr noexcept`;
  - `detail::punycode::encode(std::u32string_view input, Put put) -> result<std::size_t>`, where `put(char) -> bool` takes each output char and says whether it could;
  - `detail::punycode::decode(std::span<char32_t>, std::string_view) noexcept -> result<std::size_t>`.

- [ ] **Step 1: Write the failing tests.** `test/utf8_test.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Strict UTF-8 decoding and encoding (RFC 3629 section 4's table of well-formed sequences), which
// every operation of idna runs its input through first.

#include <webcpp/idna/detail/utf8.hpp>

#include <boost/core/lightweight_test.hpp>

#include <array>
#include <cstddef>
#include <string_view>

namespace utf8 = webcpp::idna::detail::utf8;

namespace {

/** Decodes the one code point `text` holds, which must take all of it. */
void decodes_to(std::string_view text, char32_t expected) {
    std::size_t i = 0;
    char32_t cp = 0;
    BOOST_TEST(utf8::decode(text, i, cp));
    BOOST_TEST_EQ(i, text.size());
    BOOST_TEST_EQ(static_cast<unsigned long>(cp), static_cast<unsigned long>(expected));
    BOOST_TEST(utf8::is_valid(text));
}

/** Refuses `text` at its first byte, leaving the position where it was. */
void refuses(std::string_view text) {
    std::size_t i = 0;
    char32_t cp = 0x41;
    BOOST_TEST(!utf8::decode(text, i, cp));
    BOOST_TEST_EQ(i, 0U);
    BOOST_TEST_EQ(static_cast<unsigned long>(cp), 0x41UL);
    BOOST_TEST(!utf8::is_valid(text));
}

/** The boundaries of every row of RFC 3629's table. */
void well_formed_sequences() {
    decodes_to(std::string_view("\0", 1), 0x00);
    decodes_to("\177", 0x7F);
    decodes_to("\302\200", 0x80);
    decodes_to("\337\277", 0x7FF);
    decodes_to("\340\240\200", 0x800);
    decodes_to("\354\277\277", 0xCFFF);
    decodes_to("\355\200\200", 0xD000);
    decodes_to("\355\237\277", 0xD7FF);
    decodes_to("\356\200\200", 0xE000);
    decodes_to("\357\277\277", 0xFFFF);
    decodes_to("\360\220\200\200", 0x10000);
    decodes_to("\363\277\277\277", 0xFFFFF);
    decodes_to("\364\200\200\200", 0x100000);
    decodes_to("\364\217\277\277", 0x10FFFF);
}

void ill_formed_sequences() {
    refuses("\200");  // a lone continuation byte
    refuses("\277");
    refuses("\300\200");  // overlong, C0 and C1 never lead
    refuses("\301\277");
    refuses("\340\237\277");      // overlong three-byte form of U+07FF
    refuses("\360\217\277\277");  // overlong four-byte form of U+FFFF
    refuses("\355\240\200");      // the surrogate U+D800
    refuses("\355\277\277");      // the surrogate U+DFFF
    refuses("\364\220\200\200");  // U+110000
    refuses("\365\200\200\200");  // F5 to FF never lead
    refuses("\377");
    refuses("\303");  // truncated
    refuses("\342\202");
    refuses("\360\237\230");
    refuses("\303\050");  // a lead followed by no continuation
}

/** A string whose first sequence is good and second is not is refused as a whole. */
void a_string_is_valid_only_whole() {
    BOOST_TEST(utf8::is_valid(""));
    BOOST_TEST(utf8::is_valid("b\303\274cher"));
    BOOST_TEST(!utf8::is_valid("b\303\274\303"));
}

void encodes_every_length() {
    std::array<char, 4> out{};
    BOOST_TEST_EQ(utf8::encode(0x41, out.data()), 1U);
    BOOST_TEST_EQ(std::string_view(out.data(), 1), "A");
    BOOST_TEST_EQ(utf8::encode(0xFC, out.data()), 2U);
    BOOST_TEST_EQ(std::string_view(out.data(), 2), "\303\274");
    BOOST_TEST_EQ(utf8::encode(0x20AC, out.data()), 3U);
    BOOST_TEST_EQ(std::string_view(out.data(), 3), "\342\202\254");
    BOOST_TEST_EQ(utf8::encode(0x1F600, out.data()), 4U);
    BOOST_TEST_EQ(std::string_view(out.data(), 4), "\360\237\230\200");
    BOOST_TEST_EQ(utf8::length(0x7F), 1U);
    BOOST_TEST_EQ(utf8::length(0x7FF), 2U);
    BOOST_TEST_EQ(utf8::length(0xFFFF), 3U);
    BOOST_TEST_EQ(utf8::length(0x10FFFF), 4U);
}

}  // namespace

int main() {
    well_formed_sequences();
    ill_formed_sequences();
    a_string_is_valid_only_whole();
    encodes_every_length();
    return boost::report_errors();
}
```

`test/punycode_test.cpp`, RFC 3492 section 7.1's nineteen samples (checked
against Python's `punycode` codec on 2026-10-09) and the failures; `en32g`
and `ib9b` are the Punycode of U+110000 and U+D800, and 3,855 basic code
points before U+10FFFF overflow the encoder's 32 bits while 3,854 do not:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Punycode against RFC 3492 section 7.1's samples, both ways, and its failures.

#include <webcpp/idna/punycode.hpp>

#include <boost/core/lightweight_test.hpp>

#include <array>
#include <cstddef>
#include <memory_resource>
#include <string>
#include <string_view>

namespace idna = webcpp::idna;
namespace punycode = webcpp::idna::punycode;

namespace {

/** One sample of RFC 3492 section 7.1: its code points, and its Punycode as the RFC prints it,
    mixed-case annotation included. */
struct sample {
    std::u32string_view code_points;
    std::string_view encoded;
};

constexpr auto samples = std::to_array<sample>({
    {
        .code_points = U"\u0644\u064A\u0647\u0645\u0627\u0628\u062A\u0643\u0644\u0645\u0648\u0634"
                       U"\u0639\u0631\u0628\u064A\u061F",
        .encoded = "egbpdaj6bu4bxfgehfvwxn",
    },
    {
        .code_points = U"\u4ED6\u4EEC\u4E3A\u4EC0\u4E48\u4E0D\u8BF4\u4E2D\u6587",
        .encoded = "ihqwcrb4cv8a8dqg056pqjye",
    },
    {
        .code_points = U"\u4ED6\u5011\u7232\u4EC0\u9EBD\u4E0D\u8AAA\u4E2D\u6587",
        .encoded = "ihqwctvzc91f659drss3x8bo0yb",
    },
    {
        .code_points = U"Pro\u010Dprost\u011Bnemluv\u00ED\u010Desky",
        .encoded = "Proprostnemluvesky-uyb24dma41a",
    },
    {
        .code_points = U"\u05DC\u05DE\u05D4\u05D4\u05DD\u05E4\u05E9\u05D5\u05D8\u05DC\u05D0\u05DE"
                       U"\u05D3\u05D1\u05E8\u05D9\u05DD\u05E2\u05D1\u05E8\u05D9\u05EA",
        .encoded = "4dbcagdahymbxekheh6e0a7fei0b",
    },
    {
        .code_points = U"\u092F\u0939\u0932\u094B\u0917\u0939\u093F\u0928\u094D\u0926\u0940\u0915"
                       U"\u094D\u092F\u094B\u0902\u0928\u0939\u0940\u0902\u092C\u094B\u0932\u0938"
                       U"\u0915\u0924\u0947\u0939\u0948\u0902",
        .encoded = "i1baa7eci9glrd9b2ae1bj0hfcgg6iyaf8o0a1dig0cd",
    },
    {
        .code_points = U"\u306A\u305C\u307F\u3093\u306A\u65E5\u672C\u8A9E\u3092\u8A71\u3057\u3066"
                       U"\u304F\u308C\u306A\u3044\u306E\u304B",
        .encoded = "n8jok5ay5dzabd5bym9f0cm5685rrjetr6pdxa",
    },
    {
        .code_points = U"\uC138\uACC4\uC758\uBAA8\uB4E0\uC0AC\uB78C\uB4E4\uC774\uD55C\uAD6D\uC5B4"
                       U"\uB97C\uC774\uD574\uD55C\uB2E4\uBA74\uC5BC\uB9C8\uB098\uC88B\uC744\uAE4C",
        .encoded = "989aomsvi5e83db1d2a355cv1e0vak1dwrv93d5xbh15a0dt30a5jpsd879ccm6fea98c",
    },
    {
        .code_points =
            U"\u043F\u043E\u0447\u0435\u043C\u0443\u0436\u0435\u043E\u043D\u0438\u043D\u0435\u0433"
            U"\u043E\u0432\u043E\u0440\u044F\u0442\u043F\u043E\u0440\u0443\u0441\u0441\u043A\u0438",
        .encoded = "b1abfaaepdrnnbgefbaDotcwatmq2g4l",
    },
    {
        .code_points = U"Porqu\u00E9nopuedensimplementehablarenEspa\u00F1ol",
        .encoded = "PorqunopuedensimplementehablarenEspaol-fmd56a",
    },
    {
        .code_points = U"T\u1EA1isaoh\u1ECDkh\u00F4ngth\u1EC3ch\u1EC9n\u00F3iti\u1EBFngVi\u1EC7t",
        .encoded = "TisaohkhngthchnitingVit-kjcr8268qyxafd2f1b9g",
    },
    {
        .code_points = U"3\u5E74B\u7D44\u91D1\u516B\u5148\u751F",
        .encoded = "3B-ww4c5e180e575a65lsy2b",
    },
    {
        .code_points = U"\u5B89\u5BA4\u5948\u7F8E\u6075-with-SUPER-MONKEYS",
        .encoded = "-with-SUPER-MONKEYS-pc58ag80a8qai00g7n9n",
    },
    {
        .code_points = U"Hello-Another-Way-\u305D\u308C\u305E\u308C\u306E\u5834\u6240",
        .encoded = "Hello-Another-Way--fc4qua05auwb3674vfr0b",
    },
    {
        .code_points = U"\u3072\u3068\u3064\u5C4B\u6839\u306E\u4E0B2",
        .encoded = "2-u9tlzr9756bt3uc0v",
    },
    {
        .code_points = U"Maji\u3067Koi\u3059\u308B5\u79D2\u524D",
        .encoded = "MajiKoi5-783gue6qz075azm5e",
    },
    {
        .code_points = U"\u30D1\u30D5\u30A3\u30FCde\u30EB\u30F3\u30D0",
        .encoded = "de-jg4avhby1noc0d",
    },
    {
        .code_points = U"\u305D\u306E\u30B9\u30D4\u30FC\u30C9\u3067",
        .encoded = "d9juau41awczczp",
    },
    {
        .code_points = U"-> $1.00 <-",
        .encoded = "-> $1.00 <--",
    },
});

/** `text` with its ASCII letters in lowercase: the encoder writes no mixed-case annotation. */
std::string lowercase(std::string_view text) {
    std::string out(text);
    for (char& c : out) {
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }
    return out;
}

/** The basic code points are copied as they are, so only the part after the last delimiter is
    lowercase in what the encoder writes. */
std::string as_encoded(std::string_view rfc) {
    const std::size_t delimiter = rfc.rfind('-');
    if (delimiter == std::string_view::npos) {
        return lowercase(rfc);
    }
    return std::string(rfc.substr(0, delimiter + 1)) + lowercase(rfc.substr(delimiter + 1));
}

void rfc_3492_samples_both_ways() {
    for (const sample& s : samples) {
        std::array<char, 128> encoded{};
        const idna::result<std::size_t> written = punycode::encode(encoded, s.code_points);
        if (BOOST_TEST(written.has_value())) {
            BOOST_TEST_EQ(std::string_view(encoded.data(), written.value()), as_encoded(s.encoded));
        }
        std::array<char32_t, 128> decoded{};
        const idna::result<std::size_t> read = punycode::decode(decoded, s.encoded);
        if (BOOST_TEST(read.has_value())) {
            BOOST_TEST(std::u32string_view(decoded.data(), read.value()) == s.code_points);
        }
    }
}

/** Digits are read in either case; the basic code points keep theirs (RFC 3492 section 5). */
void case_of_digits_and_basic_code_points() {
    std::array<char32_t, 16> decoded{};
    const idna::result<std::size_t> lower = punycode::decode(decoded, "bcher-kva");
    const idna::result<std::size_t> upper = punycode::decode(decoded, "bcher-KVA");
    BOOST_TEST(lower.has_value() && upper.has_value());
    BOOST_TEST(std::u32string_view(decoded.data(), upper.value()) == U"b\u00FCcher");
    const idna::result<std::size_t> annotated = punycode::decode(decoded, "BCHER-kva");
    BOOST_TEST(std::u32string_view(decoded.data(), annotated.value()) == U"B\u00FCCHER");
}

void failures() {
    std::array<char32_t, 16> decoded{};
    // A delimiter at the very start consumes nothing, and is then no digit (xn---9uc).
    BOOST_TEST(punycode::decode(decoded, "-9uc").error() == idna::error::invalid_punycode);
    BOOST_TEST(punycode::decode(decoded, "a!b").error() == idna::error::invalid_punycode);
    BOOST_TEST(punycode::decode(decoded, "\303\274-kva").error() == idna::error::invalid_punycode);
    // A number cut short: "k" asks for another digit.
    BOOST_TEST(punycode::decode(decoded, "bcher-k").error() == idna::error::invalid_punycode);
    // A decoded value beyond U+10FFFF, then a surrogate.
    BOOST_TEST(punycode::decode(decoded, "en32g").error() == idna::error::invalid_punycode);
    BOOST_TEST(punycode::decode(decoded, "ib9b").error() == idna::error::invalid_punycode);
    // "a" is the first code point that is not basic, U+0080.
    BOOST_TEST(punycode::decode(decoded, "a").value() == 1U && decoded[0] == U'\x80');
    // Digits whose value overflows 32 bits (RFC 3492 section 6.4).
    BOOST_TEST(punycode::decode(decoded, "999999999999999999999").error() ==
               idna::error::punycode_overflow);
    std::array<char, 16> encoded{};
    BOOST_TEST(punycode::encode(encoded, U"\xD800").error() == idna::error::invalid_punycode);
    // The encoder's delta for U+10FFFF after h basic code points is (0x10FFFF - 0x80) * (h + 1),
    // past 32 bits from h = 3855 on (RFC 3492 section 6.4).
    std::array<char, 4096> long_output{};
    std::u32string near_the_limit(3854, U'a');
    near_the_limit.push_back(0x10FFFF);
    BOOST_TEST(punycode::encode(long_output, near_the_limit).has_value());
    std::u32string past_the_limit(3855, U'a');
    past_the_limit.push_back(0x10FFFF);
    BOOST_TEST(punycode::encode(long_output, past_the_limit).error() ==
               idna::error::punycode_overflow);
    BOOST_TEST(punycode::encode(encoded, std::u32string_view(U"\x110000", 1)).error() ==
               idna::error::invalid_punycode);
}

/** Every length below the result's is output_too_small, and nothing is written past it. */
void output_too_small_at_every_length() {
    const std::u32string_view input = U"b\u00FCcher";
    for (std::size_t size = 0; size < 9; ++size) {
        std::array<char, 9> encoded{};
        encoded.fill('#');
        const idna::result<std::size_t> written =
            punycode::encode(std::span<char>(encoded.data(), size), input);
        BOOST_TEST(written.error() == idna::error::output_too_small);
        BOOST_TEST_EQ(encoded[size], '#');
    }
    std::array<char, 9> encoded{};
    BOOST_TEST_EQ(punycode::encode(encoded, input).value(), 9U);
    for (std::size_t size = 0; size < 6; ++size) {
        std::array<char32_t, 6> decoded{};
        const idna::result<std::size_t> read =
            punycode::decode(std::span<char32_t>(decoded.data(), size), "bcher-kva");
        BOOST_TEST(read.error() == idna::error::output_too_small);
    }
}

void the_pmr_forms() {
    std::array<std::byte, 512> storage{};
    std::pmr::monotonic_buffer_resource resource(storage.data(), storage.size(),
                                                 std::pmr::null_memory_resource());
    const idna::result<std::pmr::string> encoded = punycode::encode(U"b\u00FCcher", resource);
    BOOST_TEST(encoded.has_value() && encoded.value() == "bcher-kva");
    const idna::result<std::pmr::u32string> decoded = punycode::decode("bcher-kva", resource);
    BOOST_TEST(decoded.has_value() && decoded.value() == U"b\u00FCcher");
    BOOST_TEST(punycode::decode("-9uc", resource).error() == idna::error::invalid_punycode);
}

}  // namespace

int main() {
    rfc_3492_samples_both_ways();
    case_of_digits_and_basic_code_points();
    failures();
    output_too_small_at_every_length();
    the_pmr_forms();
    return boost::report_errors();
}
```

`test/allocation_test.cpp`, which Tasks 9 and 11 extend to the validators and
the conversions. Its replaced `operator new` raises a requested alignment to a
pointer's: libstdc++'s `new_delete_resource` asks for `char32_t`'s 4, which
macOS's `aligned_alloc` refuses (measured: the GCC 15 lane aborted before the
fix):

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Tests what idna allocates, by counting every call of the global operator new this program makes:
// nothing in the span forms and the validators, whatever the input, and in the pmr forms only what
// their memory resource allocates.

#include <webcpp/idna/punycode.hpp>

#include <boost/config.hpp>
#include <boost/core/lightweight_test.hpp>

#include <array>
#include <cstddef>
#include <cstdlib>
#include <memory_resource>
#include <new>
#include <string>
#include <string_view>

#ifdef _MSC_VER
#include <malloc.h>
#endif

namespace idna = webcpp::idna;

namespace {

// The calls of the global operator new this program has made. It is mutable global state, which
// no library holds: it is this test program's own instrument, the one place where the replaced
// operator new below can record what it sees.
std::size_t global_allocations = 0;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

}  // namespace

// The replacements of the global operator new and operator delete, for this program alone, which
// count every allocation, an over-aligned one included. The standard library declares them with
// parameter names reserved to the implementation, which a program may not use, hence the exemption
// on each. None is ever inlined: GCC, seeing the malloc or the free inside one, would pair it with
// the other's call and warn of a mismatch (-Wmismatched-new-delete).

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
BOOST_NOINLINE void* operator new(std::size_t size) {
    ++global_allocations;
    void* memory = std::malloc(size == 0 ? 1 : size);
    if (memory == nullptr) {
        std::abort();
    }
    return memory;
}

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
BOOST_NOINLINE void operator delete(void* memory) noexcept {
    std::free(memory);
}

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
BOOST_NOINLINE void operator delete(void* memory, std::size_t /*size*/) noexcept {
    std::free(memory);
}

namespace {

/**
 Memory of `size` bytes aligned to `alignment`: from aligned_alloc, whose size must be a multiple
 of the alignment; or, under _MSC_VER, from _aligned_malloc.

 Tip: the UCRT of MSVC has no aligned_alloc, and memory from its _aligned_malloc is given back with
 _aligned_free, never free, hence the two below. libstdc++'s new_delete_resource asks for the
 alignment of what it allocates, 4 for char32_t, and macOS's aligned_alloc refuses an alignment
 below a pointer's, so the alignment is raised to that.
*/
void* aligned_memory(std::size_t size, std::size_t requested) {
    const std::size_t alignment = requested < sizeof(void*) ? sizeof(void*) : requested;
    const std::size_t rounded =
        size == 0 ? alignment : (size + alignment - 1) / alignment * alignment;
#ifdef _MSC_VER
    return _aligned_malloc(rounded, alignment);
#else
    return std::aligned_alloc(alignment, rounded);
#endif
}

/** Gives back what aligned_memory took. */
void free_aligned_memory(void* memory) noexcept {
#ifdef _MSC_VER
    _aligned_free(memory);
#else
    std::free(memory);
#endif
}

}  // namespace

/** The aligned allocation, from aligned_memory. */
// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
BOOST_NOINLINE void* operator new(std::size_t size, std::align_val_t alignment) {
    ++global_allocations;
    void* memory = aligned_memory(size, static_cast<std::size_t>(alignment));
    if (memory == nullptr) {
        std::abort();
    }
    return memory;
}

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
BOOST_NOINLINE void operator delete(void* memory, std::align_val_t /*alignment*/) noexcept {
    free_aligned_memory(memory);
}

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
BOOST_NOINLINE void operator delete(void* memory, std::size_t /*size*/,
                                    std::align_val_t /*alignment*/) noexcept {
    free_aligned_memory(memory);
}

namespace {

/** A memory resource that counts what it hands out, and takes it from the global operator new
    through std::pmr::new_delete_resource. */
class counting_resource final : public std::pmr::memory_resource {
public:
    /** The number of allocations made through it. */
    [[nodiscard]] std::size_t allocations() const noexcept { return allocations_; }

private:
    void* do_allocate(std::size_t size, std::size_t alignment) override {
        ++allocations_;
        return std::pmr::new_delete_resource()->allocate(size, alignment);
    }

    void do_deallocate(void* memory, std::size_t size, std::size_t alignment) override {
        std::pmr::new_delete_resource()->deallocate(memory, size, alignment);
    }

    [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }

    std::size_t allocations_ = 0;
};

/** Inputs that reach every path: long and many labels, every kind of failure, RTL labels, joiners,
    A-labels, and the NFC check's scratch copy. */
std::array<std::string, 14> hostile_inputs() {
    std::string many_labels;
    for (int i = 0; i < 500; ++i) {
        many_labels += "a.";
    }
    return {
        "b\303\274cher.example",
        "xn--bcher-kva.EXAMPLE",
        std::string(300, 'a'),
        many_labels,
        "\303\050",
        "xn--zz.xn--.xn---9uc.xn--ls8h",
        "0a.\327\220.\331\204\330\247",
        "a\342\200\214b.\340\244\225\340\245\215\342\200\215",
        "e\314\201.\303\251\314\201",
        "a\302\267l.\343\203\273",
        "\357\274\241\343\200\202\357\274\242",
        "ab--cd.-x-.a_b",
        std::string(70, 'a') + "\303\274",
        "",
    };
}

/**
 Every span form over every input: the number of results that succeeded.

 Tip: the count is checked, so no optimizer may drop a call whose result nothing reads.
*/
std::size_t exercise_span_forms(const std::array<std::string, 14>& inputs) {
    std::array<char, 1024> out{};
    std::array<char32_t, 1024> code_points{};
    std::size_t succeeded = 0;
    for (const std::string& input : inputs) {
        succeeded += idna::punycode::decode(code_points, input).has_value() ? 1 : 0;
    }
    succeeded += idna::punycode::encode(out, U"b\u00FCcher").has_value() ? 1 : 0;
    return succeeded;
}

/** The span forms allocate nothing, whatever the input. */
void span_forms_allocate_nothing() {
    const std::array<std::string, 14> inputs = hostile_inputs();
    const std::size_t before = global_allocations;
    const std::size_t succeeded = exercise_span_forms(inputs);
    BOOST_TEST_EQ(global_allocations, before);
    BOOST_TEST(succeeded > 0);
}

/** The pmr forms allocate through their resource alone: every call of the global operator new is
    one of the resource's, for the result or a long label's storage. */
void pmr_forms_allocate_through_their_resource() {
    const std::array<std::string, 14> inputs = hostile_inputs();
    counting_resource resource;
    const std::size_t before = global_allocations;
    for (const std::string& input : inputs) {
        static_cast<void>(idna::punycode::decode(input, resource));
    }
    static_cast<void>(
        idna::punycode::encode(U"b\u00FCcher-and-more-than-a-small-string", resource));
    BOOST_TEST_EQ(global_allocations - before, resource.allocations());
    BOOST_TEST(resource.allocations() > 0);
}

}  // namespace

int main() {
    span_forms_allocate_nothing();
    pmr_forms_allocate_through_their_resource();
    return boost::report_errors();
}
```

In `test/Jamfile`, after `webcpp.run options : options_test.cpp ;`:

```
webcpp.run utf8 : utf8_test.cpp ;
webcpp.run punycode : punycode_test.cpp ;
webcpp.run allocation : allocation_test.cpp ;
```

- [ ] **Step 2: Run them and watch them fail.**

```bash
cd ~/dev/webcpp-idna-punycode
$B2 libs/idna/test//utf8 libs/idna/test//punycode libs/idna/test//allocation
```

Expected: FAIL, `'webcpp/idna/detail/utf8.hpp' file not found` and
`'webcpp/idna/punycode.hpp' file not found`.

- [ ] **Step 3: Write the implementation.** `include/webcpp/idna/detail/utf8.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 Strict UTF-8 (RFC 3629): decoding one code point, encoding one, and checking a
 whole string.
*/
#ifndef WEBCPP_IDNA_DETAIL_UTF8_HPP
#define WEBCPP_IDNA_DETAIL_UTF8_HPP

#include <webcpp/idna/config.hpp>

#include <array>
#include <cstddef>
#include <string_view>

namespace webcpp::idna::detail::utf8 {

/**
 Decodes the code point that starts at `text[i]` into `cp` and moves `i` past it, or returns false
 and leaves both unchanged at an ill-formed sequence: truncated, overlong, a surrogate, above
 U+10FFFF, or a byte no sequence starts with.
*/
constexpr bool decode(std::string_view text, std::size_t& i, char32_t& cp) noexcept {
    if (i >= text.size()) {
        return false;
    }
    const auto byte = [&text](std::size_t at) { return static_cast<unsigned char>(text[at]); };
    const unsigned char lead = byte(i);
    if (lead < 0x80) {
        cp = lead;
        ++i;
        return true;
    }
    std::size_t count = 0;
    char32_t value = 0;
    unsigned char low = 0x80;
    unsigned char high = 0xBF;
    if (lead >= 0xC2 && lead <= 0xDF) {
        count = 1;
        value = lead & 0x1FU;
    } else if (lead >= 0xE0 && lead <= 0xEF) {
        count = 2;
        value = lead & 0x0FU;
        low = lead == 0xE0 ? 0xA0 : low;
        high = lead == 0xED ? 0x9F : high;
    } else if (lead >= 0xF0 && lead <= 0xF4) {
        count = 3;
        value = lead & 0x07U;
        low = lead == 0xF0 ? 0x90 : low;
        high = lead == 0xF4 ? 0x8F : high;
    } else {
        return false;
    }
    if (text.size() - i <= count) {
        return false;
    }
    for (std::size_t k = 1; k <= count; ++k) {
        const unsigned char next = byte(i + k);
        if (next < low || next > high) {
            return false;
        }
        low = 0x80;
        high = 0xBF;
        value = value << 6U | (next & 0x3FU);
    }
    cp = value;
    i += count + 1;
    return true;
}

/** Whether every byte of `text` belongs to a well-formed UTF-8 sequence. */
constexpr bool is_valid(std::string_view text) noexcept {
    std::size_t i = 0;
    char32_t cp = 0;
    while (i < text.size()) {
        if (!decode(text, i, cp)) {
            return false;
        }
    }
    return true;
}

/** The number of octets of the UTF-8 of the scalar value `cp`. */
constexpr std::size_t length(char32_t cp) noexcept {
    if (cp < 0x80) {
        return 1;
    }
    if (cp < 0x800) {
        return 2;
    }
    return cp < 0x10000 ? 3 : 4;
}

/** Writes the UTF-8 of the scalar value `cp` at `out`, which holds @ref length(cp) octets, and
    returns that length. */
constexpr std::size_t encode(char32_t cp, char* out) noexcept {
    const std::size_t size = length(cp);
    if (size == 1) {
        out[0] = static_cast<char>(cp);
        return 1;
    }
    constexpr std::array<unsigned char, 5> leads{0, 0, 0xC0, 0xE0, 0xF0};
    out[0] = static_cast<char>(leads[size] | (cp >> (6 * (size - 1))));
    for (std::size_t k = 1; k < size; ++k) {
        out[k] = static_cast<char>(0x80U | ((cp >> (6 * (size - 1 - k))) & 0x3FU));
    }
    return size;
}

}  // namespace webcpp::idna::detail::utf8

#endif  // WEBCPP_IDNA_DETAIL_UTF8_HPP
```

`include/webcpp/idna/punycode.hpp` (RFC 3492 sections 5, 6.1 to 6.4; the
encoder and decoder are split into `put_basic`, `put_integer`,
`smallest_from` and `read_integer` to keep each function under clang-tidy's
cognitive complexity of 25):

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 Punycode (RFC 3492): code points to the ASCII of an A-label without its "xn--"
 prefix, and back, with errors as values.

 @see "Punycode", in the guide.
*/
#ifndef WEBCPP_IDNA_PUNYCODE_HPP
#define WEBCPP_IDNA_PUNYCODE_HPP

#include <webcpp/idna/config.hpp>

#include <webcpp/idna/errors.hpp>

#include <boost/system/error_code.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory_resource>
#include <span>
#include <string>
#include <string_view>

namespace webcpp::idna {

namespace detail::punycode {

/** RFC 3492 section 5's parameters, and the 32-bit unsigned arithmetic of section 6.4. */
inline constexpr std::uint32_t base = 36;
/** RFC 3492's tmin. */
inline constexpr std::uint32_t tmin = 1;
/** RFC 3492's tmax. */
inline constexpr std::uint32_t tmax = 26;
/** RFC 3492's skew. */
inline constexpr std::uint32_t skew = 38;
/** RFC 3492's damp. */
inline constexpr std::uint32_t damp = 700;
/** RFC 3492's initial_bias. */
inline constexpr std::uint32_t initial_bias = 72;
/** RFC 3492's initial_n, the first code point that is not basic. */
inline constexpr std::uint32_t initial_n = 0x80;
/** The largest value of the 32-bit arithmetic. */
inline constexpr std::uint32_t max_value = std::numeric_limits<std::uint32_t>::max();

/** RFC 3492 section 6.1: the bias after a delta. */
constexpr std::uint32_t adapt(std::uint32_t delta, std::uint32_t points, bool first) noexcept {
    delta = first ? delta / damp : delta / 2;
    delta += delta / points;
    std::uint32_t k = 0;
    while (delta > (base - tmin) * tmax / 2) {
        delta /= base - tmin;
        k += base;
    }
    return k + ((base - tmin + 1) * delta / (delta + skew));
}

/** The threshold of the digit at position `k` under `bias`. */
constexpr std::uint32_t threshold(std::uint32_t k, std::uint32_t bias) noexcept {
    if (k <= bias) {
        return tmin;
    }
    return k >= bias + tmax ? tmax : k - bias;
}

/** The value of a digit, a to z (either case) then 0 to 9, or base for any other character. */
constexpr std::uint32_t digit_value(char c) noexcept {
    if (c >= 'a' && c <= 'z') {
        return static_cast<std::uint32_t>(c - 'a');
    }
    if (c >= 'A' && c <= 'Z') {
        return static_cast<std::uint32_t>(c - 'A');
    }
    if (c >= '0' && c <= '9') {
        return static_cast<std::uint32_t>(c - '0') + 26;
    }
    return base;
}

/** The lowercase character of a digit value below base. */
constexpr char digit_char(std::uint32_t d) noexcept {
    return d < 26 ? static_cast<char>('a' + d) : static_cast<char>('0' + (d - 26));
}

/** Whether `cp` is a Unicode scalar value: at most U+10FFFF, and no surrogate. */
constexpr bool is_scalar(char32_t cp) noexcept {
    return cp <= 0x10FFFF && (cp < 0xD800 || cp > 0xDFFF);
}

/** Hands the variable-length integer `q` (RFC 3492 section 3.3) to `put` under `bias`; false
    when `put` refuses a digit. */
template <class Put>
constexpr bool put_integer(std::uint32_t q, std::uint32_t bias, Put& put) {
    for (std::uint32_t k = base;; k += base) {
        const std::uint32_t t = threshold(k, bias);
        if (q < t) {
            break;
        }
        if (!put(digit_char(t + ((q - t) % (base - t))))) {
            return false;
        }
        q = (q - t) / (base - t);
    }
    return put(digit_char(q));
}

/** The smallest code point of `input` that is at least `n`. */
constexpr char32_t smallest_from(std::u32string_view input, std::uint32_t n) noexcept {
    char32_t found = 0x10FFFF;
    for (const char32_t cp : input) {
        if (cp >= n && cp < found) {
            found = cp;
        }
    }
    return found;
}

/** Hands the basic code points of `input` to `put`, then the delimiter when there was any: their
    number, or the first error. */
template <class Put>
constexpr result<std::uint32_t> put_basic(std::u32string_view input, Put& put) {
    std::uint32_t basic = 0;
    for (const char32_t cp : input) {
        if (!is_scalar(cp)) {
            return error::invalid_punycode;
        }
        if (cp >= initial_n) {
            continue;
        }
        if (!put(static_cast<char>(cp))) {
            return error::output_too_small;
        }
        ++basic;
    }
    if (basic > 0 && !put('-')) {
        return error::output_too_small;
    }
    return basic;
}

/**
 RFC 3492 section 6.3 over `input`, handing each output character to `put`, which returns false
 when it cannot take it: the number of characters handed, or the first error.
*/
template <class Put>
constexpr result<std::size_t> encode(std::u32string_view input, Put put) {
    if (input.size() >= max_value) {
        return error::punycode_overflow;
    }
    std::size_t written = 0;
    auto counted = [&](char c) {
        if (!put(c)) {
            return false;
        }
        ++written;
        return true;
    };
    const result<std::uint32_t> copied = put_basic(input, counted);
    if (copied.has_error()) {
        return copied.error();
    }
    const std::uint32_t basic = copied.unsafe_value();
    std::uint32_t n = initial_n;
    std::uint32_t delta = 0;
    std::uint32_t bias = initial_bias;
    for (std::uint32_t handled = basic; handled < input.size(); ++delta, ++n) {
        const char32_t next = smallest_from(input, n);
        if (next - n > (max_value - delta) / (handled + 1)) {
            return error::punycode_overflow;
        }
        delta += (next - n) * (handled + 1);
        n = next;
        for (const char32_t cp : input) {
            if (cp < n && ++delta == 0) {
                return error::punycode_overflow;
            }
            if (cp != n) {
                continue;
            }
            if (!put_integer(delta, bias, counted)) {
                return error::output_too_small;
            }
            bias = adapt(delta, handled + 1, handled == basic);
            delta = 0;
            ++handled;
        }
    }
    return written;
}

/**
 Reads the variable-length integer that starts at `input[in]` under `bias` and adds it, times its
 weights, to `i` (RFC 3492 section 6.2's inner loop): no error once a digit ends it.
*/
inline boost::system::error_code read_integer(std::string_view input, std::size_t& in,
                                              std::uint32_t& i, std::uint32_t bias) noexcept {
    std::uint32_t w = 1;
    for (std::uint32_t k = base;; k += base) {
        if (in >= input.size()) {
            return error::invalid_punycode;
        }
        const std::uint32_t digit = digit_value(input[in++]);
        if (digit >= base) {
            return error::invalid_punycode;
        }
        if (digit > (max_value - i) / w) {
            return error::punycode_overflow;
        }
        i += digit * w;
        const std::uint32_t t = threshold(k, bias);
        if (digit < t) {
            return {};
        }
        if (w > max_value / (base - t)) {
            return error::punycode_overflow;
        }
        w *= base - t;
    }
}

/**
 RFC 3492 section 6.2 over `input` into `output`: the number of code points written, or the first
 error.
*/
inline result<std::size_t> decode(std::span<char32_t> output, std::string_view input) noexcept {
    const std::size_t delimiter = input.rfind('-');
    const std::size_t basic = delimiter == std::string_view::npos ? 0 : delimiter;
    if (basic > output.size()) {
        return error::output_too_small;
    }
    for (std::size_t j = 0; j < basic; ++j) {
        if (static_cast<unsigned char>(input[j]) >= initial_n) {
            return error::invalid_punycode;
        }
        output[j] = static_cast<unsigned char>(input[j]);
    }
    std::size_t written = basic;
    std::uint32_t n = initial_n;
    std::uint32_t i = 0;
    std::uint32_t bias = initial_bias;
    for (std::size_t in = basic > 0 ? basic + 1 : 0; in < input.size(); ++i) {
        const std::uint32_t old_i = i;
        if (const boost::system::error_code failed = read_integer(input, in, i, bias)) {
            return failed;
        }
        const auto points = static_cast<std::uint32_t>(written + 1);
        bias = adapt(i - old_i, points, old_i == 0);
        if (i / points > max_value - n) {
            return error::punycode_overflow;
        }
        n += i / points;
        i %= points;
        if (n < initial_n || !is_scalar(n)) {
            return error::invalid_punycode;
        }
        if (written == output.size()) {
            return error::output_too_small;
        }
        std::copy_backward(output.begin() + static_cast<std::ptrdiff_t>(i),
                           output.begin() + static_cast<std::ptrdiff_t>(written),
                           output.begin() + static_cast<std::ptrdiff_t>(written) + 1);
        output[i] = n;
        ++written;
    }
    return written;
}

}  // namespace detail::punycode

/**
 Punycode (RFC 3492), the encoding of an A-label's code points after its "xn--"
 prefix, which UTS 46 and IDNA2008 apply to one label at a time.

 @see "Punycode", in the guide.
*/
namespace punycode {

/**
 Encodes code points as Punycode, into a caller's buffer, without the "xn--"
 prefix (RFC 3492 section 6.3).

 Basic code points, below U+0080, are copied as they are, case included, and
 then a delimiter `-` when there is any; the digits that follow are lowercase.

 @param output Where the characters are written; nothing is written past it.
 @param input The code points to encode.
 @return The number of characters written, or @ref error::invalid_punycode for
 a surrogate or a value above U+10FFFF, @ref error::punycode_overflow when the
 arithmetic overflows 32 bits, @ref error::output_too_small when `output`
 cannot hold the result.
 @note After an error, the contents of `output` are unspecified.
 @see "Punycode", in the guide.
*/
[[nodiscard]] inline result<std::size_t> encode(std::span<char> output,
                                                std::u32string_view input) noexcept {
    std::size_t at = 0;
    return detail::punycode::encode(input, [&](char c) noexcept {
        if (at == output.size()) {
            return false;
        }
        output[at++] = c;
        return true;
    });
}

/**
 Decodes Punycode without its "xn--" prefix into code points, into a caller's
 buffer (RFC 3492 section 6.2).

 The code points before the last delimiter are copied as they are, case
 included, as RFC 3492's mixed-case annotation leaves them; the digits after
 it are read in either case. A delimiter at the very start consumes nothing.

 @param output Where the code points are written; nothing is written past it.
 @param input The Punycode to decode.
 @return The number of code points written, or @ref error::invalid_punycode
 for a character that is not a digit, a non-basic code point before the
 delimiter, a truncated number, or a value that is basic, a surrogate or above
 U+10FFFF; @ref error::punycode_overflow; or @ref error::output_too_small.
 @note After an error, the contents of `output` are unspecified.
 @see "Punycode", in the guide.
*/
[[nodiscard]] inline result<std::size_t> decode(std::span<char32_t> output,
                                                std::string_view input) noexcept {
    return detail::punycode::decode(output, input);
}

/**
 Encodes code points as Punycode into a string of the given memory resource.

 @param input The code points to encode.
 @param resource Where the string's memory comes from.
 @return The Punycode, or the error the span form returns but
 @ref error::output_too_small.
 @see "Allocation", in the guide.
*/
[[nodiscard]] inline result<std::pmr::string> encode(
    std::u32string_view input,
    std::pmr::memory_resource& resource = *std::pmr::get_default_resource()) {
    std::pmr::string out(&resource);
    const result<std::size_t> written = detail::punycode::encode(input, [&out](char c) {
        out.push_back(c);
        return true;
    });
    if (written.has_error()) {
        return written.error();
    }
    return out;
}

/**
 Decodes Punycode into a string of code points of the given memory resource.

 @param input The Punycode to decode.
 @param resource Where the string's memory comes from.
 @return The code points, or the error the span form returns but
 @ref error::output_too_small.
 @see "Allocation", in the guide.
*/
[[nodiscard]] inline result<std::pmr::u32string> decode(
    std::string_view input,
    std::pmr::memory_resource& resource = *std::pmr::get_default_resource()) {
    // Every code point consumes at least one character, so the input's size bounds the output.
    std::pmr::u32string out(input.size(), U'\0', &resource);
    const result<std::size_t> written = detail::punycode::decode(out, input);
    if (written.has_error()) {
        return written.error();
    }
    out.resize(written.unsafe_value());
    return out;
}

}  // namespace punycode

}  // namespace webcpp::idna

#endif  // WEBCPP_IDNA_PUNYCODE_HPP
```

In `include/webcpp/idna.hpp`, after `#include <webcpp/idna/options.hpp>`:

```cpp
#include <webcpp/idna/punycode.hpp>
```

`meta/include-boundaries.json`, now that a `detail` header exists:

```json
{
    "boundaries": [
        {
            "headers": [
                "include/webcpp/idna.hpp",
                "include/webcpp/idna/*.hpp",
                "include/webcpp/idna/detail/*.hpp"
            ],
            "must-not-include": ["boost/url", "boost/json", "srell", "webcpp/formats"],
            "why": "idna depends on webcpp::config and header-only Boost alone"
        }
    ]
}
```

- [ ] **Step 4: Run them and watch them pass.**

```bash
$B2 libs/idna/test//utf8 libs/idna/test//punycode libs/idna/test//allocation
```

Expected: three `**passed**`.

- [ ] **Step 5: The example.** `example/Jamfile`, which later tasks extend:

```
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

project : requirements <library>/webcpp/idna//idna ;

import webcpp ;

webcpp.targets native wasip2 wasip3 emscripten ;

# The guide's programs, each shown on the page with the output it prints.
webcpp.example idna_punycode.cpp ;
```

`example/.clang-tidy` (pratt's, whose reason holds here):

```yaml
# The examples inherit the superproject's configuration and switch off one
# option.
#
# bugprone-exception-escape.CheckMain treats every example's main as a
# function that should not throw. An example is a short program that lets
# what the standard library may throw, a std::bad_alloc from a string that
# grows or a std::ios_base::failure from a stream, end the process uncaught,
# the way Boost's own examples do; that is main's ordinary role, not a
# defect. The check stays on for everything else: a noexcept function, a
# destructor, a move or a swap.
---
InheritParentConfig: true
CheckOptions:
  - key: bugprone-exception-escape.CheckMain
    value: 'false'
```

`example/idna_punycode.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <webcpp/idna/punycode.hpp>

#include <array>
#include <cstddef>
#include <iostream>
#include <string_view>

namespace idna = webcpp::idna;

int main() {
    // tag::encode[]
    // RFC 3492 section 7.1's sample (L): "3年B組金八先生".
    const std::u32string_view sample = U"3\u5E74B\u7D44\u91D1\u516B\u5148\u751F";
    std::array<char, 64> encoded{};
    const idna::result<std::size_t> written = idna::punycode::encode(encoded, sample);
    std::cout << std::string_view(encoded.data(), written.value()) << '\n';
    // end::encode[]

    // tag::decode[]
    std::array<char32_t, 64> decoded{};
    const idna::result<std::size_t> read =
        idna::punycode::decode(decoded, "3B-ww4c5e180e575a65lsy2b");
    std::cout << read.value() << " code points, the same: "
              << (std::u32string_view(decoded.data(), read.value()) == sample ? "yes" : "no")
              << '\n';
    // end::decode[]

    // tag::errors[]
    for (const std::string_view bad : {"-9uc", "bcher-k", "999999999999999999999"}) {
        std::cout << bad << " -> " << idna::punycode::decode(decoded, bad).error().message()
                  << '\n';
    }
    // end::errors[]
}
```

`example/idna_punycode.expected`, which the program printed:

```
3B-ww4c5e180e575a65lsy2b
8 code points, the same: yes
-9uc -> invalid Punycode
bcher-k -> invalid Punycode
999999999999999999999 -> Punycode overflow
```

- [ ] **Step 6: The page.** `doc/punycode.adoc`:

```asciidoc
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

[#idna-punycode]
== Punycode

Punycode, RFC 3492, writes a string of code points with the ASCII letters,
digits and hyphen alone, and reads it back: the code points below U+0080, the
basic ones, are copied first, then a hyphen, then numbers in base 36 that say
where each other code point goes. An A-label is `xn--` followed by the
Punycode of its U-label. `idna::punycode::encode` and `decode` are that
encoding, without the prefix, which the conversions add and remove.

[source]
----
include::{examples}/idna_punycode.cpp[tag=encode,indent=0]
----

`decode` gives the code points back, into a span of `char32_t`:

[source]
----
include::{examples}/idna_punycode.cpp[tag=decode,indent=0]
----

Both return the number of chars or code points written, or an error: a
character that is no digit, a number cut short, a value that is not a
Unicode scalar value, or arithmetic past 32 bits, which RFC 3492 section 6.4
refuses:

[source]
----
include::{examples}/idna_punycode.cpp[tag=errors,indent=0]
----

The program prints:

[listing]
----
include::{examples}/idna_punycode.expected[]
----

The digits are read in either case; the basic code points are copied as they
are, case included, as RFC 3492's mixed-case annotation leaves them, and
`encode` writes its digits in lowercase. A hyphen at the very start is no
delimiter, since nothing precedes it, so `-9uc` is refused, as RFC 3492
says: it is the A-label `xn---9uc`, which a careless decoder would read as
`xn--9uc`. Each function has a second form that returns a `std::pmr` string
from a memory resource (<<idna-allocation>>).
```

`doc/allocation.adoc` (Task 12 adds its example):

```asciidoc
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

[#idna-allocation]
== Allocation

idna allocates nothing it does not have to, and lets its user choose where
what it does allocate comes from. Every conversion has two forms:

* *A span form* writes into the caller's buffer, `std::span<char>` or
  `std::span<char32_t>`, and allocates nothing, whatever its input. The
  validators, `check_label` and `check_domain`, allocate nothing either.
* *A pmr form* returns a `std::pmr::string` or `std::pmr::u32string` from
  the `std::pmr::memory_resource` it is given, by default
  `std::pmr::get_default_resource()`. It allocates from that resource alone:
  its result, and a label's working storage beyond the inline capacity.

The test `allocation` replaces the global `operator new` for itself alone
and counts: no call across the span forms, over every kind of input, and in
the pmr forms, exactly the calls of their memory resource.
```

In `doc/idna.adoc`, after `include::introduction.adoc[]`, the line
`include::punycode.adoc[]`, and after `include::errors.adoc[]`, the line
`include::allocation.adoc[]`, each between blank lines.

In `doc/introduction.adoc`, the table of headers gains, after the row of
`options.hpp`:

```asciidoc

| `<webcpp/idna/punycode.hpp>`
| `punycode::encode` and `punycode::decode` (<<idna-punycode>>).
```

In `doc/testing.adoc`, the table of tests gains, before its closing `|===`:

```asciidoc

| `utf8`
| Strict UTF-8: the boundaries of every row of RFC 3629's table of
  well-formed sequences, and every kind of ill-formed one.

| `punycode`
| RFC 3492 section 7.1's samples, (A) to (S), both ways; digits in either
  case and the mixed-case annotation kept; a leading delimiter, a bad digit,
  a number cut short, a value out of Unicode, an overflow;
  `output_too_small` at every length below the result's; the pmr forms.

| `allocation`
| The global `operator new`, replaced for this program alone: no call in a
  span form, and in a pmr form only those of its memory resource.
```

and after the table's paragraph that begins "Beside them", the sentence
"`example/Jamfile` builds every program this page shows, runs it, and
compares what it printed with the `.expected` beside it." at its end.

- [ ] **Step 7: Every lane, the page and the lint** (procedure "The task's
  lanes"):

```bash
cd ~/dev/webcpp-idna-punycode
L=~/dev/webcpp/.superpowers/sdd/WEBCPP_IDNA_PLAN/punycode
export EM_CACHE=~/dev/webcpp/.local/emscripten-cache
EXAMPLES="$(test -f libs/idna/example/Jamfile && echo libs/idna/example)"
$B2 -a --build-dir=bin/idna-native libs/idna/test $EXAMPLES > $L/native.log 2>&1 &
$B2 -a --build-dir=bin/idna-gcc toolset=gcc-15 libs/idna/test $EXAMPLES > $L/gcc.log 2>&1 &
$B2 -a --build-dir=bin/idna-wasip2 toolset=clang-wasip2 testing.launcher=wasmtime libs/idna/test $EXAMPLES > $L/wasip2.log 2>&1 &
$B2 -a --build-dir=bin/idna-wasip3 toolset=clang-wasip3 testing.launcher=wasmtime libs/idna/test $EXAMPLES > $L/wasip3.log 2>&1 &
$B2 -a --build-dir=bin/idna-emscripten toolset=emscripten libs/idna/test $EXAMPLES > $L/emscripten.log 2>&1 &
wait
grep -L -e '\.\.\.failed' $L/{native,gcc,wasip2,wasip3,emscripten}.log
$B2 -a --build-dir=bin/idna-doc libs/idna/doc > $L/doc.log 2>&1; echo "doc $?"
tools/lint/lint.sh --clang-format .local/wasi-sdk/bin/clang-format --clang-tidy .local/wasi-sdk/bin/clang-tidy > $L/lint.log 2>&1; tail -1 $L/lint.log
```

Expected also: `idna_punycode` passing on each lane.

- [ ] **Step 8: Commit.**

```bash
cd ~/dev/webcpp-idna-punycode/libs/idna
git add include/webcpp/idna/detail/utf8.hpp include/webcpp/idna/punycode.hpp \
    include/webcpp/idna.hpp meta/include-boundaries.json test/Jamfile test/utf8_test.cpp \
    test/punycode_test.cpp test/allocation_test.cpp example/Jamfile example/.clang-tidy \
    example/idna_punycode.cpp example/idna_punycode.expected doc/punycode.adoc \
    doc/allocation.adoc doc/idna.adoc doc/introduction.adoc doc/testing.adoc
git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "idna: Punycode, and strict UTF-8"
git log --format='%an %ae %cn %ce' main..HEAD
```

Expected: the identity line alone.

- [ ] **Step 9: Report,** ending with:

```markdown
## Changelog

No change: the New Libraries line of idna stands as written.
```

### Task 3: The Unicode 17.0.0 tables, generated, and their lookups

The spec's increment I2, its tables. Worktree `~/dev/webcpp-idna-tables`,
from `main` with Task 1 integrated; runs at the same time as Task 2. The
Unicode data notice follows the provisional P8 ruling (Task 0, Step 4): the
generated file names the page's appendix, which Task 4 writes.

**Files:**
- Create: `tools/unicode.json`, `tools/generate_tables.py`
- Create (generated, by the generator): `include/webcpp/idna/detail/tables.hpp`
- Create: `include/webcpp/idna/detail/properties.hpp`, `test/properties_test.cpp`
- Modify: `test/Jamfile`, `doc/testing.adoc`

**Interfaces:**
- Consumes: Task 1's `config.hpp`.
- Produces, in `webcpp::idna::detail::tables` (generated; each a `constexpr` array; the enumerations' values are the generator's):

```cpp
inline constexpr std::string_view unicode_version = "17.0.0";
enum class status : std::uint8_t { valid = 0, mapped = 1, deviation = 2, ignored = 3, disallowed = 4 };
enum class bidi_class : std::uint8_t { other = 0, l, r, al, an, en, es, cs, et, on, bn, nsm };
enum class joining_type : std::uint8_t { other = 0, l, d, r, t };
enum class script : std::uint8_t { other = 0, greek, hebrew, hiragana, katakana, han };
enum class quick_check : std::uint8_t { yes = 0, no, maybe };
// uts46_runs (std::uint64_t: first << 32 | status, mark << 3, length << 4, offset << 9),
// uts46_mappings (char32_t); bidi_classes, joining_types, scripts, combining_classes, marks,
// nfc_quick_checks (std::uint32_t: first << 8 | value); decomposition_keys (char32_t),
// decomposition_slices (std::uint32_t: offset << 3 | length), decomposition_data (char32_t);
// composition_keys (std::uint64_t: first << 21 | second), composition_values (char32_t).
```

- Produces, in `webcpp::idna::detail` (for Tasks 7 to 11):

```cpp
struct uts46_entry { tables::status status; bool not_idna2008; std::u32string_view mapping; };
constexpr uts46_entry uts46_of(char32_t cp) noexcept;
constexpr tables::bidi_class bidi_class_of(char32_t cp) noexcept;
constexpr tables::joining_type joining_type_of(char32_t cp) noexcept;
constexpr tables::script script_of(char32_t cp) noexcept;
constexpr std::uint8_t combining_class_of(char32_t cp) noexcept;
constexpr bool is_mark(char32_t cp) noexcept;
constexpr tables::quick_check nfc_quick_check_of(char32_t cp) noexcept;
constexpr std::u32string_view decomposition_of(char32_t cp) noexcept;  // full, Hangul aside
constexpr char32_t composition_of(char32_t first, char32_t second) noexcept;  // 0: none
```

- Produces: `tools/generate_tables.py [--cache DIR] [TABLES]`, writing the tables at the path given or at their place in the library; Task 5 extends it to three outputs, S1's generator contract (`<script> <argument>... <path-1> ... <path-n>`, run in the Jamfile's directory).
- The test of the lookups is the target `properties`: the name `tables` belongs to the own lane of Task 6, and a b2 main target and an alias of one name in one Jamfile are refused (`No best alternative for libs/idna/test/tables`, measured), which would drop the test from every ordinary lane.

- [ ] **Step 1: Write the failing test.** `test/properties_test.cpp`, one value
  of each table, the spec's spot properties among them:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Spot properties of the generated Unicode 17.0.0 tables, read through detail/properties.hpp: one
// value of each table that a wrong packing, a shifted run or a misread file would change.

#include <webcpp/idna/detail/properties.hpp>

#include <boost/core/lightweight_test.hpp>

#include <string_view>

namespace detail = webcpp::idna::detail;
namespace tables = webcpp::idna::detail::tables;

namespace {

void uts46_statuses_and_mappings() {
    BOOST_TEST(detail::uts46_of(U'a').status == tables::status::valid);
    BOOST_TEST(!detail::uts46_of(U'a').not_idna2008);
    const detail::uts46_entry inverted_exclamation = detail::uts46_of(0x00A1);
    BOOST_TEST(inverted_exclamation.status == tables::status::valid);
    BOOST_TEST(inverted_exclamation.not_idna2008);
    const detail::uts46_entry capital_a = detail::uts46_of(U'A');
    BOOST_TEST(capital_a.status == tables::status::mapped);
    BOOST_TEST(capital_a.mapping == U"a");
    BOOST_TEST(detail::uts46_of(0x00AD).status == tables::status::ignored);
    const detail::uts46_entry sharp_s = detail::uts46_of(0x00DF);
    BOOST_TEST(sharp_s.status == tables::status::deviation);
    BOOST_TEST(sharp_s.mapping == U"ss");
    BOOST_TEST(detail::uts46_of(0x200D).status == tables::status::deviation);
    BOOST_TEST(detail::uts46_of(0x200D).mapping.empty());
    BOOST_TEST(detail::uts46_of(0x3002).mapping == U".");
    // The longest mapping of the table, U+FDFA, 18 code points.
    BOOST_TEST_EQ(detail::uts46_of(0xFDFA).mapping.size(), 18U);
    BOOST_TEST(detail::uts46_of(0xE000).status == tables::status::disallowed);
    BOOST_TEST(detail::uts46_of(0x10FFFF).status == tables::status::disallowed);
}

void bidi_classes() {
    BOOST_TEST(detail::bidi_class_of(U'a') == tables::bidi_class::l);
    BOOST_TEST(detail::bidi_class_of(0x05D0) == tables::bidi_class::r);
    BOOST_TEST(detail::bidi_class_of(0x0627) == tables::bidi_class::al);
    BOOST_TEST(detail::bidi_class_of(0x0660) == tables::bidi_class::an);
    BOOST_TEST(detail::bidi_class_of(U'0') == tables::bidi_class::en);
    BOOST_TEST(detail::bidi_class_of(0x0300) == tables::bidi_class::nsm);
    BOOST_TEST(detail::bidi_class_of(U' ') == tables::bidi_class::other);
    // An unassigned code point of the Hebrew block defaults to R (an @missing line).
    BOOST_TEST(detail::bidi_class_of(0x05FF) == tables::bidi_class::r);
}

void joining_types_and_combining_classes() {
    BOOST_TEST(detail::joining_type_of(0x0627) == tables::joining_type::r);
    BOOST_TEST(detail::joining_type_of(0x0628) == tables::joining_type::d);
    BOOST_TEST(detail::joining_type_of(0x0300) == tables::joining_type::t);
    BOOST_TEST(detail::joining_type_of(U'a') == tables::joining_type::other);
    BOOST_TEST_EQ(detail::combining_class_of(0x094D), 9U);
    BOOST_TEST_EQ(detail::combining_class_of(0x0300), 230U);
    BOOST_TEST_EQ(detail::combining_class_of(U'a'), 0U);
}

void scripts_and_marks() {
    BOOST_TEST(detail::script_of(0x0375) == tables::script::greek);
    BOOST_TEST(detail::script_of(0x03B2) == tables::script::greek);
    BOOST_TEST(detail::script_of(0x05D1) == tables::script::hebrew);
    BOOST_TEST(detail::script_of(0x3041) == tables::script::hiragana);
    BOOST_TEST(detail::script_of(0x30A2) == tables::script::katakana);
    BOOST_TEST(detail::script_of(0x30FB) == tables::script::other);  // KATAKANA MIDDLE DOT, Common
    BOOST_TEST(detail::script_of(0x4E00) == tables::script::han);
    BOOST_TEST(detail::is_mark(0x0300));
    BOOST_TEST(detail::is_mark(0x0903));  // Mc
    BOOST_TEST(!detail::is_mark(U'a'));
}

void normalization_data() {
    BOOST_TEST(detail::nfc_quick_check_of(U'a') == tables::quick_check::yes);
    BOOST_TEST(detail::nfc_quick_check_of(0x0340) == tables::quick_check::no);
    BOOST_TEST(detail::nfc_quick_check_of(0x0301) == tables::quick_check::maybe);
    BOOST_TEST(detail::decomposition_of(0x00E9) == U"e\u0301");
    // U+1FAF decomposes in full to four code points, the longest canonical decomposition.
    BOOST_TEST_EQ(detail::decomposition_of(0x1FAF).size(), 4U);
    BOOST_TEST(detail::decomposition_of(U'a').empty());
    BOOST_TEST(detail::decomposition_of(0xAC00).empty());  // Hangul is algorithmic
    BOOST_TEST_EQ(static_cast<unsigned long>(detail::composition_of(U'e', 0x0301)), 0xE9UL);
    // U+0958 is a composition exclusion: its pair composes to nothing.
    BOOST_TEST_EQ(static_cast<unsigned long>(detail::composition_of(0x0915, 0x093C)), 0UL);
}

void the_version() {
    BOOST_TEST_EQ(tables::unicode_version, std::string_view("17.0.0"));
}

}  // namespace

int main() {
    uts46_statuses_and_mappings();
    bidi_classes();
    joining_types_and_combining_classes();
    scripts_and_marks();
    normalization_data();
    the_version();
    return boost::report_errors();
}
```

In `test/Jamfile`, before the line `webcpp.run allocation : allocation_test.cpp ;`
(at the end of the file while the worktree does not hold it yet):

```
webcpp.run properties : properties_test.cpp ;
```

- [ ] **Step 2: Run it and watch it fail.**

```bash
cd ~/dev/webcpp-idna-tables
$B2 libs/idna/test//properties
```

Expected: FAIL, `'webcpp/idna/detail/properties.hpp' file not found`.

- [ ] **Step 3: Pin the inputs.** `tools/unicode.json` (the digests are the
  spec's section 8.3, checked again on 2026-10-09 against the files):

```json
{
    "version": "17.0.0",
    "files": [
        {
            "path": "idna/IdnaMappingTable.txt",
            "url": "https://www.unicode.org/Public/17.0.0/idna/IdnaMappingTable.txt",
            "sha256": "87f05505dc026fdb2bff16132bdc68a8014675836882a9a2b1844540ad3be382"
        },
        {
            "path": "idna/IdnaTestV2.txt",
            "url": "https://www.unicode.org/Public/17.0.0/idna/IdnaTestV2.txt",
            "sha256": "beb5d0be20e896189b03209a82fdc34f06351502bbd4b8e2523583fc2954d9cf"
        },
        {
            "path": "ucd/UnicodeData.txt",
            "url": "https://www.unicode.org/Public/17.0.0/ucd/UnicodeData.txt",
            "sha256": "2e1efc1dcb59c575eedf5ccae60f95229f706ee6d031835247d843c11d96470c"
        },
        {
            "path": "ucd/extracted/DerivedBidiClass.txt",
            "url": "https://www.unicode.org/Public/17.0.0/ucd/extracted/DerivedBidiClass.txt",
            "sha256": "4867b4b7f0731ed1bfcd34cc6251211ff1542541fce0734b6fbda139ee80b3a4"
        },
        {
            "path": "ucd/extracted/DerivedJoiningType.txt",
            "url": "https://www.unicode.org/Public/17.0.0/ucd/extracted/DerivedJoiningType.txt",
            "sha256": "f39ebe974825d6736aee15582250307aa532b2cfab3caf3f86bd23fddc9c5c4d"
        },
        {
            "path": "ucd/extracted/DerivedCombiningClass.txt",
            "url": "https://www.unicode.org/Public/17.0.0/ucd/extracted/DerivedCombiningClass.txt",
            "sha256": "191463abfbd202703c6fd6776a92a23ac44ec65e0476a7f95aa91ca492cef29b"
        },
        {
            "path": "ucd/DerivedNormalizationProps.txt",
            "url": "https://www.unicode.org/Public/17.0.0/ucd/DerivedNormalizationProps.txt",
            "sha256": "71fd6a206a2c0cdd41feb6b7f656aa31091db45e9cedc926985d718397f9e488"
        },
        {
            "path": "ucd/Scripts.txt",
            "url": "https://www.unicode.org/Public/17.0.0/ucd/Scripts.txt",
            "sha256": "9f5e50d3abaee7d6ce09480f325c706f485ae3240912527e651954d2d6b035bf"
        },
        {
            "path": "ucd/NormalizationTest.txt",
            "url": "https://www.unicode.org/Public/17.0.0/ucd/NormalizationTest.txt",
            "sha256": "5019ffd530751a741900c849c0e010332f142a3612234639bd200b82138a87db"
        },
        {
            "path": "iana/idna-tables-properties.csv",
            "url": "https://www.iana.org/assignments/idna-tables-12.0.0/idna-tables-properties.csv",
            "sha256": "160b72e33a377a84253393c572dc338879f39b93beccbb9cb9baebbf4d6dd743"
        }
    ]
}
```

- [ ] **Step 4: Write the generator.** `tools/generate_tables.py`:

```python
#!/usr/bin/env python3
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Writes idna's Unicode tables from the pinned Unicode files.

Usage: generate_tables.py [--cache DIR] [TABLES]

The inputs are the files tools/unicode.json pins, each by its URL and its SHA-256. A file missing
from the cache directory (default: bin/unicode/<version>/ of the superproject) is downloaded into
it; every file is hashed, and a digest that differs from the pinned one stops the generator, naming
the file and both digests, before anything is written.

The output is written at the path given, or by default at its place in the library:
include/webcpp/idna/detail/tables.hpp, the tables as constexpr data. Two runs write the same bytes:
no date, sorted data, "\\n" line endings. Nothing is written unless every check passed.

Before writing, the generator checks what the library assumes of the data, and stops naming the
assumption that fails: every status of IdnaMappingTable.txt is one of the five it knows; a code
point is PVALID, CONTEXTJ or CONTEXTO in IANA's IDNA2008 tables exactly when its UTS 46 status is
valid or deviation without an NV8 or XV8 mark, U+002E aside; the CONTEXTJ and CONTEXTO code points
are those of RFC 5892 appendix A; and no full canonical decomposition is longer than 4 code points.

Exit 0 when every output was written, 1 naming a fault, 2 on a usage error.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import io
import json
import sys
import urllib.request
from collections.abc import Iterator, Sequence
from pathlib import Path

LIBRARY = Path(__file__).resolve().parent.parent
SUPERPROJECT = LIBRARY.parent.parent
PINNED = LIBRARY / 'tools' / 'unicode.json'

TABLES = 'include/webcpp/idna/detail/tables.hpp'
OUTPUTS = (TABLES,)

LAST_CODE_POINT = 0x10FFFF
STATUSES = ('valid', 'mapped', 'deviation', 'ignored', 'disallowed')
BIDI_CLASSES = ('other', 'L', 'R', 'AL', 'AN', 'EN', 'ES', 'CS', 'ET', 'ON', 'BN', 'NSM')
BIDI_LONG_NAMES = {
    'Left_To_Right': 'L', 'Right_To_Left': 'R', 'Arabic_Letter': 'AL', 'Arabic_Number': 'AN',
    'European_Number': 'EN', 'European_Separator': 'ES', 'Common_Separator': 'CS',
    'European_Terminator': 'ET', 'Other_Neutral': 'ON', 'Boundary_Neutral': 'BN',
    'Nonspacing_Mark': 'NSM',
}
JOINING_TYPES = ('other', 'L', 'D', 'R', 'T')
SCRIPTS = ('other', 'Greek', 'Hebrew', 'Hiragana', 'Katakana', 'Han')
QUICK_CHECKS = ('Y', 'N', 'M')
MARK_CATEGORIES = ('Mn', 'Mc', 'Me')
CONTEXTJ = frozenset({0x200C, 0x200D})
CONTEXTO = frozenset({0x00B7, 0x0375, 0x05F3, 0x05F4, 0x30FB,
                      *range(0x0660, 0x066A), *range(0x06F0, 0x06FA)})
FULL_STOP = 0x002E
HANGUL_FIRST, HANGUL_LAST = 0xAC00, 0xD7A3
MAX_DECOMPOSITION = 4
MAPPING_LENGTH_BITS, MAPPING_OFFSET_BITS = 5, 23

NOTICE = """\
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)
//
// Generated by tools/generate_tables.py from the Unicode {version} data files: do not edit.
// The data derive from the Unicode Character Database, under the Unicode License v3; its notice
// is in the appendix "Unicode data" of idna's page.
"""


class Fault(Exception):
    """A reason the generator stops, said to the user."""


# Inputs


def pinned_files(pinned: Path) -> tuple[str, dict[str, dict[str, str]]]:
    """The Unicode version and each pinned file by its path in the cache."""
    data = json.loads(pinned.read_text(encoding='utf-8'))
    return data['version'], {entry['path']: entry for entry in data['files']}


def fetch(entry: dict[str, str], cache: Path) -> str:
    """The text of one pinned file, downloaded into the cache when missing, its digest checked."""
    path = cache / entry['path']
    if not path.is_file():
        path.parent.mkdir(parents=True, exist_ok=True)
        with urllib.request.urlopen(entry['url'], timeout=60) as response:
            body: bytes = response.read()
        partial = path.with_name(path.name + '.partial')
        partial.write_bytes(body)
        partial.replace(path)
    data = path.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if digest != entry['sha256']:
        raise Fault(f'{path}: SHA-256 {digest}, but tools/unicode.json pins {entry["sha256"]}')
    return data.decode('utf-8')


def records(text: str) -> Iterator[tuple[int, int, list[str]]]:
    """Each data line of a UCD file as its first and last code points and its other fields."""
    for line in text.splitlines():
        body = line.split('#', 1)[0].strip()
        if not body:
            continue
        fields = [field.strip() for field in body.split(';')]
        first, _, last = fields[0].partition('..')
        yield int(first, 16), int(last or first, 16), fields[1:]


def missing(text: str) -> Iterator[tuple[int, int, str]]:
    """Each `@missing` line of a UCD file as its range and its default value."""
    for line in text.splitlines():
        if line.startswith('# @missing:'):
            span, value = (part.strip() for part in line[len('# @missing:'):].split(';')[:2])
            first, _, last = span.partition('..')
            yield int(first, 16), int(last, 16), value


def property_of(text: str, names: Sequence[str], aliases: dict[str, str] | None = None
                ) -> list[int]:
    """A property's value for every code point, as an index into `names` (0 for any other)."""
    aliases = aliases or {}
    values = [0] * (LAST_CODE_POINT + 1)

    def index(name: str) -> int:
        name = aliases.get(name, name)
        return names.index(name) if name in names else 0

    for first, last, value in missing(text):
        values[first:last + 1] = [index(value)] * (last - first + 1)
    for first, last, fields in records(text):
        values[first:last + 1] = [index(fields[0])] * (last - first + 1)
    return values


# The UTS 46 mapping table


class Mapping:
    """The UTS 46 status, IDNA2008 mark and mapping of every code point."""

    def __init__(self, text: str) -> None:
        self.status = [STATUSES.index('disallowed')] * (LAST_CODE_POINT + 1)
        self.marked = [False] * (LAST_CODE_POINT + 1)
        self.target: list[tuple[int, ...]] = [()] * (LAST_CODE_POINT + 1)
        for first, last, fields in records(text):
            if fields[0] not in STATUSES:
                raise Fault(f'IdnaMappingTable.txt: unknown status {fields[0]!r} at {first:04X}')
            target = tuple(int(cp, 16) for cp in fields[1].split()) if len(fields) > 1 else ()
            marked = len(fields) > 2 and fields[2] in ('NV8', 'XV8')
            for cp in range(first, last + 1):
                self.status[cp] = STATUSES.index(fields[0])
                self.marked[cp] = marked
                self.target[cp] = target

    def idna2008_valid(self, cp: int) -> bool:
        """Whether IDNA2008 makes `cp` PVALID, CONTEXTJ or CONTEXTO, by UTS 46's table."""
        return STATUSES[self.status[cp]] in ('valid', 'deviation') and not self.marked[cp]


def check_against_iana(mapping: Mapping, text: str) -> None:
    """Stops unless the derivation from UTS 46's table agrees with IANA's IDNA2008 tables, and the
    context sets with RFC 5892 appendix A."""
    disagree: list[int] = []
    contextj: set[int] = set()
    contexto: set[int] = set()
    for row in csv.DictReader(io.StringIO(text)):
        first, _, last = row['Codepoint'].partition('-')
        prop = row['Property']
        for cp in range(int(first, 16), int(last or first, 16) + 1):
            if prop == 'UNASSIGNED':
                continue
            if prop == 'CONTEXTJ':
                contextj.add(cp)
            if prop == 'CONTEXTO':
                contexto.add(cp)
            if mapping.idna2008_valid(cp) != (prop != 'DISALLOWED') and cp != FULL_STOP:
                disagree.append(cp)
    if disagree:
        shown = ', '.join(f'U+{cp:04X}' for cp in disagree[:10])
        raise Fault(f'IANA idna-tables-12.0.0 and the derivation from UTS 46 disagree on {shown}')
    if contextj != CONTEXTJ or contexto != CONTEXTO:
        raise Fault('IANA idna-tables-12.0.0 lists other CONTEXTJ or CONTEXTO code points than '
                    'RFC 5892 appendix A')


# Normalization


def unicode_data(text: str) -> Iterator[tuple[int, int, list[str]]]:
    """Each code point or First/Last range of UnicodeData.txt with its fields."""
    pending: tuple[int, list[str]] | None = None
    for line in text.splitlines():
        fields = line.split(';')
        cp = int(fields[0], 16)
        if fields[1].endswith(', First>'):
            pending = (cp, fields)
            continue
        if pending is not None:
            yield pending[0], cp, pending[1]
            pending = None
            continue
        yield cp, cp, fields


def decompositions(text: str) -> dict[int, tuple[int, ...]]:
    """Every canonical decomposition of UnicodeData.txt, one level deep, Hangul aside."""
    found: dict[int, tuple[int, ...]] = {}
    for first, _, fields in unicode_data(text):
        if fields[5] and not fields[5].startswith('<'):
            found[first] = tuple(int(cp, 16) for cp in fields[5].split())
    return found


def full_decomposition(cp: int, immediate: dict[int, tuple[int, ...]]) -> tuple[int, ...]:
    """The full canonical decomposition of a code point, by applying the immediate ones until none
    applies."""
    pending = [cp]
    result: list[int] = []
    while pending:
        head = pending.pop(0)
        if head in immediate:
            pending[0:0] = immediate[head]
        else:
            result.append(head)
    return tuple(result)


def marks(text: str) -> list[int]:
    """1 for every code point of General_Category Mark, 0 for the others."""
    values = [0] * (LAST_CODE_POINT + 1)
    for first, last, fields in unicode_data(text):
        if fields[2] in MARK_CATEGORIES:
            values[first:last + 1] = [1] * (last - first + 1)
    return values


def composition_exclusions(text: str) -> set[int]:
    """The code points of Full_Composition_Exclusion."""
    excluded: set[int] = set()
    for first, last, fields in records(text):
        if fields[0] == 'Full_Composition_Exclusion':
            excluded.update(range(first, last + 1))
    return excluded


def quick_checks(text: str) -> list[int]:
    """NFC_Quick_Check of every code point, as an index into QUICK_CHECKS (Yes when absent)."""
    values = [0] * (LAST_CODE_POINT + 1)
    for first, last, fields in records(text):
        if fields[0] == 'NFC_QC':
            values[first:last + 1] = [QUICK_CHECKS.index(fields[1])] * (last - first + 1)
    return values


def combining_classes(text: str) -> list[int]:
    """Canonical_Combining_Class of every code point."""
    values = [0] * (LAST_CODE_POINT + 1)
    for first, last, fields in records(text):
        values[first:last + 1] = [int(fields[0])] * (last - first + 1)
    return values


# C++ output


def runs(values: Sequence[int]) -> list[tuple[int, int]]:
    """The values as runs: each first code point whose value differs from the one before it."""
    found: list[tuple[int, int]] = []
    for cp, value in enumerate(values):
        if not found or found[-1][1] != value:
            found.append((cp, value))
    return found


def rows(items: Sequence[str], indent: str = '    ', width: int = 100) -> str:
    """The items, comma-separated, as many on a line as fit the width."""
    lines: list[str] = []
    line = indent
    for item in items:
        piece = item + ','
        if len(line) + len(piece) + 1 > width and line.strip():
            lines.append(line.rstrip())
            line = indent
        line += piece + ' '
    if line.strip():
        lines.append(line.rstrip())
    return '\n'.join(lines)


def array(brief: str, kind: str, name: str, items: Sequence[str]) -> str:
    """A documented `inline constexpr std::array` of the items."""
    return (f'/** {brief} */\n'
            f'inline constexpr std::array<{kind}, {len(items)}> {name}{{\n'
            f'{rows(items)}\n'
            '};\n')


def enumeration(brief: str, name: str, enumerators: Sequence[tuple[str, str]]) -> str:
    """A documented enum class over std::uint8_t, each enumerator documented and valued in
    order."""
    body = '\n'.join(f'    /** {doc} */\n    {label} = {value},'
                     for value, (label, doc) in enumerate(enumerators))
    return f'/** {brief} */\nenum class {name} : std::uint8_t {{\n{body}\n}};\n'


def packed(values: Sequence[int]) -> list[str]:
    """The runs of a small property, each packed as its first code point shifted left by 8 and
    its value."""
    return [f'0x{first << 8 | value:X}' for first, value in runs(values)]


def uts46_rows(mapping: Mapping) -> tuple[list[str], list[str]]:
    """The UTS 46 runs, as packed values, and the code points of every mapping, in order."""
    offsets: dict[tuple[int, ...], int] = {}
    data: list[int] = []
    values: list[int] = []
    for cp in range(LAST_CODE_POINT + 1):
        target = mapping.target[cp]
        if target not in offsets:
            offsets[target] = len(data)
            data.extend(target)
        if len(target) >= 1 << MAPPING_LENGTH_BITS or offsets[target] >= 1 << MAPPING_OFFSET_BITS:
            raise Fault(f'the mapping of U+{cp:04X} does not fit the packed UTS 46 value')
        values.append(mapping.status[cp] | int(mapping.marked[cp]) << 3 | len(target) << 4
                      | offsets[target] << 9)
    return ([f'0x{first << 32 | value:X}' for first, value in runs(values)],
            [f'0x{cp:X}' for cp in data])


def tables_header(version: str, inputs: dict[str, str], mapping: Mapping) -> str:
    """The text of include/webcpp/idna/detail/tables.hpp."""
    check_against_iana(mapping, inputs['iana/idna-tables-properties.csv'])
    unicode = inputs['ucd/UnicodeData.txt']
    normalization = inputs['ucd/DerivedNormalizationProps.txt']
    immediate = decompositions(unicode)
    full = {cp: full_decomposition(cp, immediate) for cp in sorted(immediate)}
    longest = max(len(target) for target in full.values())
    if longest > MAX_DECOMPOSITION:
        raise Fault(f'a full canonical decomposition holds {longest} code points, more than '
                    f'the {MAX_DECOMPOSITION} the label storage assumes')
    excluded = composition_exclusions(normalization)
    composites = sorted((pair[0] << 21 | pair[1], cp) for cp, pair in immediate.items()
                        if len(pair) == 2 and cp not in excluded)
    decomposition_data: list[int] = []
    slices: list[str] = []
    for target in full.values():
        slices.append(f'0x{len(decomposition_data) << 3 | len(target):X}')
        decomposition_data.extend(target)
    uts46, mappings = uts46_rows(mapping)
    bidi = property_of(inputs['ucd/extracted/DerivedBidiClass.txt'], BIDI_CLASSES,
                       BIDI_LONG_NAMES)
    joining = property_of(inputs['ucd/extracted/DerivedJoiningType.txt'], JOINING_TYPES)
    scripts = property_of(inputs['ucd/Scripts.txt'], SCRIPTS)
    parts = [
        NOTICE.format(version=version),
        '/**\n The Unicode ' + version + ' tables idna reads: generated, and read through'
        ' detail/properties.hpp alone.\n*/\n',
        '#ifndef WEBCPP_IDNA_DETAIL_TABLES_HPP\n#define WEBCPP_IDNA_DETAIL_TABLES_HPP\n\n'
        '#include <webcpp/idna/config.hpp>\n\n#include <array>\n#include <cstdint>\n'
        '#include <string_view>\n\nnamespace webcpp::idna::detail::tables {\n',
        f'/** The version of Unicode the tables were generated from. */\n'
        f'inline constexpr std::string_view unicode_version = "{version}";\n',
        enumeration('A code point\'s status in the UTS 46 mapping table.', 'status', [
            ('valid', 'Kept, and valid.'), ('mapped', 'Replaced by its mapping.'),
            ('deviation', 'Kept, or replaced by its mapping in transitional processing.'),
            ('ignored', 'Removed.'), ('disallowed', 'Kept, and an error.')]),
        enumeration('The Bidi_Class values RFC 5893 names, and other for the rest.', 'bidi_class',
                    [(name.lower(), f'Bidi_Class {name}.' if name != 'other' else
                      'Any class RFC 5893 does not name.') for name in BIDI_CLASSES]),
        enumeration('The Joining_Type values RFC 5892 A.1 names, and other for the rest.',
                    'joining_type',
                    [(name.lower(), f'Joining_Type {name}.' if name != 'other' else
                      'Any other joining type.') for name in JOINING_TYPES]),
        enumeration('The scripts RFC 5892 A.4 to A.7 name, and other for the rest.', 'script',
                    [(name.lower(), f'Script {name}.' if name != 'other' else
                      'Any other script.') for name in SCRIPTS]),
        enumeration('A code point\'s NFC_Quick_Check value.', 'quick_check',
                    [('yes', 'Yes: it never changes under NFC.'), ('no', 'No: it never appears '
                      'in NFC.'), ('maybe', 'Maybe: it may compose with what precedes it.')]),
        '// clang-format off\n',
        array('UTS 46 runs: a first code point shifted left by 32, and the status in bits 0 to 2,'
              ' the NV8 or XV8 mark in bit 3, the mapping\'s length in bits 4 to 8 and its offset'
              ' in uts46_mappings from bit 9.', 'std::uint64_t', 'uts46_runs', uts46),
        array('The code points of every mapping of the UTS 46 table, one after the other.',
              'char32_t', 'uts46_mappings', mappings),
        array('Bidi_Class runs: a first code point shifted left by 8, and a bidi_class.',
              'std::uint32_t', 'bidi_classes', packed(bidi)),
        array('Joining_Type runs: a first code point shifted left by 8, and a joining_type.',
              'std::uint32_t', 'joining_types', packed(joining)),
        array('Script runs: a first code point shifted left by 8, and a script.',
              'std::uint32_t', 'scripts', packed(scripts)),
        array('Canonical_Combining_Class runs: a first code point shifted left by 8, and the'
              ' class.', 'std::uint32_t', 'combining_classes',
              packed(combining_classes(inputs['ucd/extracted/DerivedCombiningClass.txt']))),
        array('General_Category Mark runs: a first code point shifted left by 8, and 1 for a'
              ' mark.', 'std::uint32_t', 'marks', packed(marks(unicode))),
        array('NFC_Quick_Check runs: a first code point shifted left by 8, and a quick_check.',
              'std::uint32_t', 'nfc_quick_checks', packed(quick_checks(normalization))),
        array('The code points that have a canonical decomposition, Hangul syllables aside.',
              'char32_t', 'decomposition_keys', [f'0x{cp:X}' for cp in full]),
        array('Each decomposition_keys entry\'s full decomposition: its offset in'
              ' decomposition_data shifted left by 3, and its length.', 'std::uint32_t',
              'decomposition_slices', slices),
        array('The code points of every full canonical decomposition, one after the other.',
              'char32_t', 'decomposition_data', [f'0x{cp:X}' for cp in decomposition_data]),
        array('Each primary composite\'s pair, its first code point shifted left by 21 and its'
              ' second, Hangul aside.', 'std::uint64_t', 'composition_keys',
              [f'0x{key:X}' for key, _ in composites]),
        array('The primary composite of each composition_keys pair.', 'char32_t',
              'composition_values', [f'0x{cp:X}' for _, cp in composites]),
        '// clang-format on\n',
        '}  // namespace webcpp::idna::detail::tables\n\n'
        '#endif  // WEBCPP_IDNA_DETAIL_TABLES_HPP\n',
    ]
    return '\n'.join(parts)


# Main


def parse_arguments(argv: list[str]) -> argparse.Namespace:
    """The command line: the cache directory, and where the outputs go."""
    parser = argparse.ArgumentParser(description='Writes idna\'s Unicode tables and vectors.')
    parser.add_argument('--cache', type=Path, default=None,
                        help='where the pinned files are kept (default: bin/unicode/<version>)')
    parser.add_argument('outputs', type=Path, nargs='*',
                        help=f'where {", ".join(OUTPUTS)} are written, in that order '
                             '(default: their places in the library)')
    arguments = parser.parse_args(argv)
    if arguments.outputs and len(arguments.outputs) != len(OUTPUTS):
        parser.error(f'give no output path, or all {len(OUTPUTS)}')
    return arguments


def generate(cache: Path | None, outputs: Sequence[Path]) -> None:
    """Reads every pinned file, checks the assumptions, and writes every output, or nothing."""
    version, pinned = pinned_files(PINNED)
    cache = cache or SUPERPROJECT / 'bin' / 'unicode' / version
    inputs = {path: fetch(entry, cache) for path, entry in sorted(pinned.items())}
    mapping = Mapping(inputs['idna/IdnaMappingTable.txt'])
    texts = [tables_header(version, inputs, mapping)]
    paths = list(outputs) or [LIBRARY / relative for relative in OUTPUTS]
    for path, text in zip(paths, texts):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(text.encode('utf-8'))


def main(argv: list[str]) -> int:
    arguments = parse_arguments(argv)
    try:
        generate(arguments.cache, arguments.outputs)
    except (Fault, OSError, KeyError, ValueError) as fault:
        print(f'generate_tables.py: {fault}', file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
```

- [ ] **Step 5: Run it, twice, and see it refuse a wrong file.**

```bash
L=~/dev/webcpp/.superpowers/sdd/WEBCPP_IDNA_PLAN/tables
python3 libs/idna/tools/generate_tables.py
cp libs/idna/include/webcpp/idna/detail/tables.hpp $L/tables-first.hpp
python3 libs/idna/tools/generate_tables.py
cmp $L/tables-first.hpp libs/idna/include/webcpp/idna/detail/tables.hpp && echo same
grep -c 'inline constexpr std::array' libs/idna/include/webcpp/idna/detail/tables.hpp
rm -rf $L/badcache && cp -R bin/unicode/17.0.0 $L/badcache
echo x >> $L/badcache/ucd/Scripts.txt
python3 libs/idna/tools/generate_tables.py --cache $L/badcache $L/no-tables.hpp; echo "exit $?"
ls $L/no-tables.hpp
```

Expected: the first run downloads the ten files into `bin/unicode/17.0.0/`
and exits 0 silently (the IANA comparison passes); `same`; `13`; then
`generate_tables.py: <L>/badcache/ucd/Scripts.txt: SHA-256 17c98cb6178f7c39e4218e92b25edddcb438c0e382b6721d2255e023fca5d5a7, but tools/unicode.json pins 9f5e50d3abaee7d6ce09480f325c706f485ae3240912527e651954d2d6b035bf`,
`exit 1`, and `ls` finds no such file. The arrays hold 8,495 UTS 46 runs,
5,492 mapping code points, 1,252 Bidi runs, 928 joining runs, 171 script
runs, 606 combining-class runs, 655 mark runs, 242 quick-check runs, 2,081
decompositions over 3,450 code points and 961 compositions; the header is
about 331 KB.

- [ ] **Step 6: See each self-check refuse a planted defect.** Copy the
  generator aside, plant one defect at a time, run it into `$L`, and restore
  it after each:

```bash
cp libs/idna/tools/generate_tables.py $L/generate_tables.py.keep
G=libs/idna/tools/generate_tables.py
sed -i '' 's/^MAX_DECOMPOSITION = 4/MAX_DECOMPOSITION = 3/' $G
python3 $G $L/planted.hpp; echo "exit $?"; cp $L/generate_tables.py.keep $G
sed -i '' 's/ and cp != FULL_STOP:/:/' $G
python3 $G $L/planted.hpp; echo "exit $?"; cp $L/generate_tables.py.keep $G
sed -i '' "s/^STATUSES = ('valid', 'mapped', /STATUSES = ('valid', /" $G
python3 $G $L/planted.hpp; echo "exit $?"; cp $L/generate_tables.py.keep $G
ls $L/planted.hpp; git -C libs/idna diff --stat -- tools/generate_tables.py
```

Expected (measured on the prototype), each with `exit 1`:
`generate_tables.py: a full canonical decomposition holds 4 code points, more than the 3 the label storage assumes`;
`generate_tables.py: IANA idna-tables-12.0.0 and the derivation from UTS 46 disagree on U+002E`;
`generate_tables.py: IdnaMappingTable.txt: unknown status 'mapped' at 0041`;
then `ls` finds no `planted.hpp`, and the generator is unchanged.

- [ ] **Step 7: Write the lookups.** `include/webcpp/idna/detail/properties.hpp`
  (each lookup keeps an index, never an iterator variable, which
  `readability-qualified-auto` would ask to spell `auto*` where an array's
  iterator is a pointer and which is a class on MSVC):

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The Unicode properties idna reads, as lookups over the generated tables: each a
 binary search over sorted runs, with no state and no allocation.
*/
#ifndef WEBCPP_IDNA_DETAIL_PROPERTIES_HPP
#define WEBCPP_IDNA_DETAIL_PROPERTIES_HPP

#include <webcpp/idna/config.hpp>

#include <webcpp/idna/detail/tables.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace webcpp::idna::detail {

/** A code point's entry in the UTS 46 mapping table. */
struct uts46_entry {
    /** The code point's status. */
    tables::status status = tables::status::disallowed;

    /** Whether UTS 46 marks it NV8 or XV8: DISALLOWED in IDNA2008. */
    bool not_idna2008 = false;

    /** What a mapped or deviation code point is replaced by; empty for the others. */
    std::u32string_view mapping;
};

/**
 The index of the last element of the sorted `values` that is at most `key`: a run list's run of a
 code point, whose first element is always 0.

 Tip: an index rather than an iterator, which is a pointer on some standard libraries and a class
 on others, so that no declaration depends on which.
*/
template <class T, std::size_t N>
constexpr std::size_t last_at_most(const std::array<T, N>& values, T key) noexcept {
    return static_cast<std::size_t>(std::ranges::upper_bound(values, key) - values.begin()) - 1;
}

/** The index of `key` in the sorted `values`, or N when it is not there. */
template <class T, std::size_t N>
constexpr std::size_t index_of(const std::array<T, N>& values, T key) noexcept {
    const auto at =
        static_cast<std::size_t>(std::ranges::lower_bound(values, key) - values.begin());
    return at < N && values[at] == key ? at : N;
}

/** The value of the run of packed `runs` that holds `cp`: the low 8 bits of the last run whose
    first code point is at most `cp`. */
template <std::size_t N>
constexpr std::uint8_t packed_value(const std::array<std::uint32_t, N>& runs,
                                    char32_t cp) noexcept {
    const std::uint32_t key = static_cast<std::uint32_t>(cp) << 8U | 0xFFU;
    return static_cast<std::uint8_t>(runs[last_at_most(runs, key)] & 0xFFU);
}

/** The UTS 46 status, IDNA2008 mark and mapping of `cp`. */
constexpr uts46_entry uts46_of(char32_t cp) noexcept {
    const std::uint64_t key = static_cast<std::uint64_t>(cp) << 32U | 0xFFFFFFFFU;
    const auto value =
        static_cast<std::uint32_t>(tables::uts46_runs[last_at_most(tables::uts46_runs, key)]);
    return {
        .status = static_cast<tables::status>(value & 0x7U),
        .not_idna2008 = (value & 0x8U) != 0,
        .mapping =
            std::u32string_view(tables::uts46_mappings.data() + (value >> 9U), value >> 4U & 0x1FU),
    };
}

/** The Bidi_Class of `cp`, as RFC 5893 names it. */
constexpr tables::bidi_class bidi_class_of(char32_t cp) noexcept {
    return static_cast<tables::bidi_class>(packed_value(tables::bidi_classes, cp));
}

/** The Joining_Type of `cp`, as RFC 5892 A.1 names it. */
constexpr tables::joining_type joining_type_of(char32_t cp) noexcept {
    return static_cast<tables::joining_type>(packed_value(tables::joining_types, cp));
}

/** The script of `cp`, among those RFC 5892 A.4 to A.7 name. */
constexpr tables::script script_of(char32_t cp) noexcept {
    return static_cast<tables::script>(packed_value(tables::scripts, cp));
}

/** The Canonical_Combining_Class of `cp`. */
constexpr std::uint8_t combining_class_of(char32_t cp) noexcept {
    return packed_value(tables::combining_classes, cp);
}

/** Whether `cp` is of General_Category Mark (Mn, Mc or Me). */
constexpr bool is_mark(char32_t cp) noexcept {
    return packed_value(tables::marks, cp) != 0;
}

/** The NFC_Quick_Check of `cp`. */
constexpr tables::quick_check nfc_quick_check_of(char32_t cp) noexcept {
    return static_cast<tables::quick_check>(packed_value(tables::nfc_quick_checks, cp));
}

/** The full canonical decomposition of `cp`, empty when it has none or is a Hangul syllable. */
constexpr std::u32string_view decomposition_of(char32_t cp) noexcept {
    const std::size_t at = index_of(tables::decomposition_keys, cp);
    if (at == tables::decomposition_keys.size()) {
        return {};
    }
    const std::uint32_t slice = tables::decomposition_slices[at];
    return {tables::decomposition_data.data() + (slice >> 3U), slice & 0x7U};
}

/** The primary composite of the pair `first`, `second`, or 0 when there is none, Hangul aside. */
constexpr char32_t composition_of(char32_t first, char32_t second) noexcept {
    const std::uint64_t key = static_cast<std::uint64_t>(first) << 21U | second;
    const std::size_t at = index_of(tables::composition_keys, key);
    if (at == tables::composition_keys.size()) {
        return 0;
    }
    return tables::composition_values[at];
}

}  // namespace webcpp::idna::detail

#endif  // WEBCPP_IDNA_DETAIL_PROPERTIES_HPP
```

- [ ] **Step 8: Run the test and watch it pass, and check the target names.**

```bash
$B2 libs/idna/test//properties
$B2 -n -a --build-dir=bin/idna-dry libs/idna/test | grep -c properties_test
```

Expected: `**passed**`; a count above 0.

- [ ] **Step 9: The testing chapter.** In `doc/testing.adoc`, before the tests
  table's closing `|===`:

```asciidoc

| `properties`
| One value of every generated table, through its lookup.
```

- [ ] **Step 10: Every lane, the page and the lint** (procedure "The task's
  lanes"):

```bash
cd ~/dev/webcpp-idna-tables
L=~/dev/webcpp/.superpowers/sdd/WEBCPP_IDNA_PLAN/tables
export EM_CACHE=~/dev/webcpp/.local/emscripten-cache
EXAMPLES="$(test -f libs/idna/example/Jamfile && echo libs/idna/example)"
$B2 -a --build-dir=bin/idna-native libs/idna/test $EXAMPLES > $L/native.log 2>&1 &
$B2 -a --build-dir=bin/idna-gcc toolset=gcc-15 libs/idna/test $EXAMPLES > $L/gcc.log 2>&1 &
$B2 -a --build-dir=bin/idna-wasip2 toolset=clang-wasip2 testing.launcher=wasmtime libs/idna/test $EXAMPLES > $L/wasip2.log 2>&1 &
$B2 -a --build-dir=bin/idna-wasip3 toolset=clang-wasip3 testing.launcher=wasmtime libs/idna/test $EXAMPLES > $L/wasip3.log 2>&1 &
$B2 -a --build-dir=bin/idna-emscripten toolset=emscripten libs/idna/test $EXAMPLES > $L/emscripten.log 2>&1 &
wait
grep -L -e '\.\.\.failed' $L/{native,gcc,wasip2,wasip3,emscripten}.log
$B2 -a --build-dir=bin/idna-doc libs/idna/doc > $L/doc.log 2>&1; echo "doc $?"
tools/lint/lint.sh --clang-format .local/wasi-sdk/bin/clang-format --clang-tidy .local/wasi-sdk/bin/clang-tidy > $L/lint.log 2>&1; tail -1 $L/lint.log
```

Expected also: `properties` passing on each lane.

- [ ] **Step 11: Commit.**

```bash
cd ~/dev/webcpp-idna-tables/libs/idna
git add tools/unicode.json tools/generate_tables.py include/webcpp/idna/detail/tables.hpp \
    include/webcpp/idna/detail/properties.hpp test/properties_test.cpp test/Jamfile \
    doc/testing.adoc
git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "idna: the Unicode 17.0.0 tables, generated, and their lookups"
git log --format='%an %ae %cn %ce' main..HEAD
```

Expected: the identity line alone.

- [ ] **Step 12: Report,** ending with:

```markdown
## Changelog

No change: the New Libraries line of idna stands as written.
```

### Task 4: The page's Unicode section, the appendix, and what the tables cost

The spec's increment I2, its page and measurement. Worktree
`~/dev/webcpp-idna-tables-doc`, from `main` with Task 3 integrated.

**Files:**
- Create: `doc/unicode.adoc`, `doc/unicode-data.adoc`
- Modify: `doc/idna.adoc`, `AGENTS.md`

**Interfaces:**
- Consumes: Task 3's generator and `detail/properties.hpp`.
- Produces: the sections "Unicode and the tables" (`[#idna-unicode]`, with `[#idna-unicode-generator]`, `[#idna-unicode-compile]`, `[#idna-unicode-moving]`) and the appendix "Unicode data" (`[#idna-unicode-data]`), which Tasks 6, 9 and 12 refer to; AGENTS.md's section "Unicode", which Tasks 5 and 6 extend.

- [ ] **Step 1: The failing check.** Add to `doc/idna.adoc`, before
  `include::testing.adoc[]`, the line `include::unicode.adoc[]` between blank
  lines, and after `include::{reference}[leveloffset=+1]` a blank line and
  `include::unicode-data.adoc[]`; then:

```bash
cd ~/dev/webcpp-idna-tables-doc
$B2 -a --build-dir=bin/idna-doc libs/idna/doc
```

Expected: FAIL, Asciidoctor's warning that `unicode.adoc` (and
`unicode-data.adoc`) is not there.

- [ ] **Step 2: Measure what the tables cost** (the spec's section 8.2; Task
  12 measures the whole library again), with this program in
  `~/dev/webcpp/.superpowers/sdd/WEBCPP_IDNA_PLAN/tables-doc/cost.cpp`, never
  committed:

```cpp
#include <webcpp/idna/detail/properties.hpp>
#include <cstdio>
int main(int argc, char** argv) {
    const char32_t cp = argc > 1 ? static_cast<unsigned char>(argv[1][0]) : U'A';
    const auto entry = webcpp::idna::detail::uts46_of(cp);
    std::printf("%d %zu %d\n", static_cast<int>(entry.status), entry.mapping.size(),
                static_cast<int>(webcpp::idna::detail::bidi_class_of(cp)));
}
```

```bash
L=~/dev/webcpp/.superpowers/sdd/WEBCPP_IDNA_PLAN/tables-doc
E='env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH'
/usr/bin/time -p $E clang++ -std=c++20 -O2 -I libs/idna/include -I /opt/homebrew/opt/boost/include -c $L/cost.cpp -o $L/cost.o
$E .local/wasi-sdk/bin/clang++ --target=wasm32-wasip2 -std=c++20 -O2 -I libs/idna/include -I /opt/homebrew/opt/boost/include $L/cost.cpp -o $L/cost.wasm
printf '#include <cstdio>\nint main(int argc, char** argv) { std::printf("%%d %%s\\n", argc, argv[0]); }\n' > $L/base.cpp
$E .local/wasi-sdk/bin/clang++ --target=wasm32-wasip2 -std=c++20 -O2 $L/base.cpp -o $L/base.wasm
ls -l $L/cost.wasm $L/base.wasm
```

Expected (measured on the prototype): `real` about 0.64 s; the wasip2
program about 73 KB larger than the base. Record the numbers in the ledger;
when they differ from 0.6 s or 73 KB by more than a fifth, write yours in
the paragraph "What the tables cost" below.

- [ ] **Step 3: The sections.** `doc/unicode.adoc` (Task 6 adds the vectors
  and the lane to its generator's subsection, Task 12 the whole library's
  cost):

```asciidoc
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

[#idna-unicode]
== Unicode and the tables

idna's tables follow Unicode {unicode-version}, `idna::unicode_version`, and UTS 46
revision 35 for it. They are generated from Unicode's data files into
`<webcpp/idna/detail/tables.hpp>` as `constexpr` arrays in read-only data: no
table is built at run time, none is compressed, nothing is allocated and
nothing is shared mutable state, so any number of threads may call idna at
once. Each property is a sorted list of runs, looked up by binary search.

[cols="2,3,3",options="header"]
|===
| Table | From | Used by

| UTS 46 status, NV8 and XV8 mark, mapping
| `idna/IdnaMappingTable.txt`
| the mapping, V7, IDNA2008's property

| Bidi_Class
| `ucd/extracted/DerivedBidiClass.txt`
| RFC 5893

| Joining_Type
| `ucd/extracted/DerivedJoiningType.txt`
| RFC 5892 A.1

| Canonical_Combining_Class
| `ucd/extracted/DerivedCombiningClass.txt`
| NFC, and the virama of A.1 and A.2

| General_Category Mark
| `ucd/UnicodeData.txt`
| V6

| Script: Greek, Hebrew, Hiragana, Katakana, Han
| `ucd/Scripts.txt`
| RFC 5892 A.4 to A.7

| canonical decompositions, primary composites, NFC_Quick_Check
| `ucd/UnicodeData.txt`, `ucd/DerivedNormalizationProps.txt`
| NFC
|===

Hangul syllables are decomposed and composed by the arithmetic of Unicode
section 3.12, and take no table.

[#idna-unicode-generator]
=== The generator

`tools/generate_tables.py`, in idna's repository, writes the tables. Its
inputs are pinned in
`tools/unicode.json`, each by its URL under
`https://www.unicode.org/Public/{unicode-version}/` and its SHA-256, with IANA's
IDNA2008 tables, `idna-tables-12.0.0`, read only by its checks. A file its
cache lacks is downloaded, every file is hashed, and a digest that differs
stops it, naming the file and both digests, before it writes anything.

Before it writes, it checks what idna assumes of the data, and stops naming
the assumption that fails: every status of the mapping table is one of the
five it knows; IANA's IDNA2008 property of every code point IANA assigns one
agrees with the derivation from UTS 46's table, U+002E FULL STOP aside, which
IANA disallows and UTS 46 keeps as the separator no label holds; the
CONTEXTJ and CONTEXTO code points are those of RFC 5892 appendix A; and no
canonical decomposition is longer than 4 code points. Two runs write the same
bytes. Its cache is `bin/unicode/{unicode-version}/` of the superproject,
which it fills from unicode.org and iana.org when a file is missing. The data
are Unicode's, under the Unicode License v3, whose notice is the appendix
<<idna-unicode-data>>.

[#idna-unicode-compile]
=== What the tables cost

Only `<webcpp/idna/detail/properties.hpp>` includes the tables, and only
`uts46.hpp` and `validity.hpp` will include it: `punycode.hpp`, `errors.hpp`
and `options.hpp` cost nothing of them. A translation unit that includes
`properties.hpp` and looks up a code point compiles in 0.6 s natively at
`-O2` with Apple Clang 21, and the tables it reads add about 73 KB to a
wasm32-wasip2 program built at `-O2`.

[#idna-unicode-moving]
=== Moving to another version of Unicode

One change of its own: the version, URLs and digests of `tools/unicode.json`
and `idna::unicode_version` move together, the generator writes the tables
and the vectors again, every lane runs, the known failures are brought up to
date, each with its reason, and the changelog says so.
```

`doc/unicode-data.adoc`, the appendix with the Unicode License v3 as
`https://www.unicode.org/license.txt` gave it on 2026-10-09 (SHA-256
`e7a93b009565cfce55919a381437ac4db883e9da2126fa28b91d12732bc53d96`):

```asciidoc
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

[appendix#idna-unicode-data]
== Unicode data

idna's tables, `<webcpp/idna/detail/tables.hpp>`, and the conformance
vectors of its tests, `test/generated/`, are derived from the Unicode
Character Database and the files of UTS 46, version {unicode-version}, which Unicode,
Inc. publishes under the following notice. idna's code is under the Boost
Software License 1.0 alone.

[listing]
----
UNICODE LICENSE V3

COPYRIGHT AND PERMISSION NOTICE

Copyright © 1991-2026 Unicode, Inc.

NOTICE TO USER: Carefully read the following legal agreement. BY
DOWNLOADING, INSTALLING, COPYING OR OTHERWISE USING DATA FILES, AND/OR
SOFTWARE, YOU UNEQUIVOCALLY ACCEPT, AND AGREE TO BE BOUND BY, ALL OF THE
TERMS AND CONDITIONS OF THIS AGREEMENT. IF YOU DO NOT AGREE, DO NOT
DOWNLOAD, INSTALL, COPY, DISTRIBUTE OR USE THE DATA FILES OR SOFTWARE.

Permission is hereby granted, free of charge, to any person obtaining a
copy of data files and any associated documentation (the "Data Files") or
software and any associated documentation (the "Software") to deal in the
Data Files or Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, and/or sell
copies of the Data Files or Software, and to permit persons to whom the
Data Files or Software are furnished to do so, provided that either (a)
this copyright and permission notice appear with all copies of the Data
Files or Software, or (b) this copyright and permission notice appear in
associated Documentation.

THE DATA FILES AND SOFTWARE ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
THIRD PARTY RIGHTS.

IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS INCLUDED IN THIS NOTICE
BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES,
OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THE DATA
FILES OR SOFTWARE.

Except as contained in this notice, the name of a copyright holder shall
not be used in advertising or otherwise to promote the sale, use or other
dealings in these Data Files or Software without prior written
authorization of the copyright holder.
----
```

At the end of `AGENTS.md`:

```markdown

## Unicode

- **The version is 17.0.0**, UTS 46 revision 35, `idna::unicode_version`.
  `include/webcpp/idna/detail/tables.hpp` is generated by
  `tools/generate_tables.py` from the files `tools/unicode.json` pins by URL
  and SHA-256, and never edited by hand. Outside b2,
  `python3 libs/idna/tools/generate_tables.py` writes it again in place, from
  the cache `bin/unicode/17.0.0/` of the superproject, which it fills from the
  network when a file is missing.
- **IDNA2008's property is derived, not shipped.** A code point is PVALID,
  CONTEXTJ or CONTEXTO exactly when its UTS 46 status is valid or deviation
  without an NV8 or XV8 mark. The generator stops if IANA's
  `idna-tables-12.0.0` disagrees for any code point IANA assigns, U+002E
  aside, or if the CONTEXTJ and CONTEXTO sets differ from RFC 5892
  appendix A, whose rules `detail/context.hpp` hard-codes.
- **The label storage rests on a measurement.** `detail::max_label_code_points`
  is 256 because no full canonical decomposition is longer than 4 code
  points (U+1FAF); the generator stops if a new version breaks that.
- **Moving to another version** is a change of its own: `tools/unicode.json`
  (version, URLs, digests), `unicode_version` in `options.hpp` and the page's
  attribute `:unicode-version:` in `doc/idna.adoc` together,
  the generator run again, every lane, the known failures brought up to date
  with their reasons, the page's "Unicode and the tables", and a changelog
  line. Unicode 18.0.0 (UTS 46 revision 36) is out; the move is not made.
- **The data notice.** The generated files derive from Unicode's data, under
  the Unicode License v3: each names the page's appendix "Unicode data" in
  one comment line, the appendix holds the notice whole, and idna's code is
  BSL-1.0 alone.
```

- [ ] **Step 4: Every lane, the page and the lint** (procedure "The task's
  lanes"):

```bash
cd ~/dev/webcpp-idna-tables-doc
L=~/dev/webcpp/.superpowers/sdd/WEBCPP_IDNA_PLAN/tables-doc
export EM_CACHE=~/dev/webcpp/.local/emscripten-cache
EXAMPLES="$(test -f libs/idna/example/Jamfile && echo libs/idna/example)"
$B2 -a --build-dir=bin/idna-native libs/idna/test $EXAMPLES > $L/native.log 2>&1 &
$B2 -a --build-dir=bin/idna-gcc toolset=gcc-15 libs/idna/test $EXAMPLES > $L/gcc.log 2>&1 &
$B2 -a --build-dir=bin/idna-wasip2 toolset=clang-wasip2 testing.launcher=wasmtime libs/idna/test $EXAMPLES > $L/wasip2.log 2>&1 &
$B2 -a --build-dir=bin/idna-wasip3 toolset=clang-wasip3 testing.launcher=wasmtime libs/idna/test $EXAMPLES > $L/wasip3.log 2>&1 &
$B2 -a --build-dir=bin/idna-emscripten toolset=emscripten libs/idna/test $EXAMPLES > $L/emscripten.log 2>&1 &
wait
grep -L -e '\.\.\.failed' $L/{native,gcc,wasip2,wasip3,emscripten}.log
$B2 -a --build-dir=bin/idna-doc libs/idna/doc > $L/doc.log 2>&1; echo "doc $?"
tools/lint/lint.sh --clang-format .local/wasi-sdk/bin/clang-format --clang-tidy .local/wasi-sdk/bin/clang-tidy > $L/lint.log 2>&1; tail -1 $L/lint.log
```

- [ ] **Step 5: Commit.**

```bash
cd ~/dev/webcpp-idna-tables-doc/libs/idna
git add doc/unicode.adoc doc/unicode-data.adoc doc/idna.adoc AGENTS.md
git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "idna: the page's Unicode section and the Unicode data notice"
git log --format='%an %ae %cn %ce' main..HEAD
```

Expected: the identity line alone.

- [ ] **Step 6: Report,** with the measurements, ending with:

```markdown
## Changelog

No change: the New Libraries line of idna stands as written.
```

### Task 5: The conformance vectors, generated

The spec's increment I2, its vectors (P6). Worktree
`~/dev/webcpp-idna-vectors`, from `main` with Tasks 3 and 4 integrated.

**Files:**
- Modify: `tools/generate_tables.py`, `doc/testing.adoc`, `test/Jamfile`, `AGENTS.md`
- Create: `test/known_failures.json`, `test/require.hpp`, `test/idna_vectors.hpp`, `test/nfc_vectors.hpp`, `test/vectors_test.cpp`
- Create (generated): `test/generated/idna_test_v2.ipp`, `test/generated/nfc_test.ipp`

**Interfaces:**
- Consumes: Task 3's generator, its `Mapping` (`marked`, `idna2008_valid`) and `NOTICE`.
- Produces (for Tasks 7 and 11):

```cpp
namespace webcpp::test {
struct idna_vector {
    std::uint32_t line; std::string_view source;
    std::string_view to_unicode; bool to_unicode_error;
    std::string_view to_ascii_n; bool to_ascii_n_error;
    std::string_view to_ascii_t; bool to_ascii_t_error;
    bool strict; std::string_view known_failure;
};
inline constexpr std::array<idna_vector, 6391> idna_vectors;   // test/idna_vectors.hpp
struct nfc_vector { std::u32string_view source; std::u32string_view nfc; std::u32string_view nfd; };
inline constexpr std::array<nfc_vector, 20034> nfc_vectors;    // test/nfc_vectors.hpp
inline void require(bool held);                                // test/require.hpp
}
```

- Produces: `tools/generate_tables.py [--cache DIR] [TABLES IDNA_VECTORS NFC_VECTORS]`, the three outputs in that order, as S1's `webcpp.generated` hands their paths.

- [ ] **Step 1: Write the failing test.** `test/require.hpp` (xactor's and
  pratt's helper):

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// What a test's helper does with a check its caller cannot go on without. A test case that fails
// such a check itself returns (`if (!BOOST_TEST(...)) { return; }`); a helper cannot return from
// its caller, and a lightweight_test program may be built without exceptions, so the helper ends
// the program instead, with the errors counted so far.

#ifndef WEBCPP_TEST_IDNA_REQUIRE_HPP
#define WEBCPP_TEST_IDNA_REQUIRE_HPP

#include <boost/core/lightweight_test.hpp>

#include <cstdlib>

namespace webcpp::test {

// Ends the program as main would end it, with the errors counted so far, unless `held`, which is
// what BOOST_TEST returned: BOOST_TEST has already reported the failure.
inline void require(bool held) {
    if (!held) {
        std::exit(boost::report_errors());
    }
}

}  // namespace webcpp::test

#endif  // WEBCPP_TEST_IDNA_REQUIRE_HPP
```

`test/idna_vectors.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// The lines of IdnaTestV2.txt as data, which tools/generate_tables.py writes into
// generated/idna_test_v2.ipp, so that the conformance test reads no file on any target.

#ifndef WEBCPP_TEST_IDNA_IDNA_VECTORS_HPP
#define WEBCPP_TEST_IDNA_IDNA_VECTORS_HPP

#include <array>
#include <cstdint>
#include <string_view>

namespace webcpp::test {

/** One line of IdnaTestV2.txt, its blank fields resolved as the file's header says. */
struct idna_vector {
    /** The line's number in the pinned file. */
    std::uint32_t line;

    /** The source, in UTF-8; ill-formed where the file escapes a lone surrogate. */
    std::string_view source;

    /** toUnicode's result. */
    std::string_view to_unicode;

    /** Whether toUnicode records an error. */
    bool to_unicode_error;

    /** toAsciiN's result, nontransitional. */
    std::string_view to_ascii_n;

    /** Whether toAsciiN records an error. */
    bool to_ascii_n_error;

    /** toAsciiT's result, transitional. */
    std::string_view to_ascii_t;

    /** Whether toAsciiT records an error. */
    bool to_ascii_t_error;

    /** Whether the toUnicode field holds no NV8, XV8 or CONTEXTO code point, so that the strict
        profile must give the same results. */
    bool strict;

    /** Why the line is a known failure, from test/known_failures.json; empty for every other. */
    std::string_view known_failure;
};

#include "generated/idna_test_v2.ipp"

}  // namespace webcpp::test

#endif  // WEBCPP_TEST_IDNA_IDNA_VECTORS_HPP
```

`test/nfc_vectors.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// The lines of NormalizationTest.txt as data, which tools/generate_tables.py writes into
// generated/nfc_test.ipp, so that the NFC test reads no file on any target.

#ifndef WEBCPP_TEST_IDNA_NFC_VECTORS_HPP
#define WEBCPP_TEST_IDNA_NFC_VECTORS_HPP

#include <array>
#include <string_view>

namespace webcpp::test {

/** One line of NormalizationTest.txt: its first three columns. */
struct nfc_vector {
    /** Column 1, the source. */
    std::u32string_view source;

    /** Column 2, the NFC of every column 1 to 3. */
    std::u32string_view nfc;

    /** Column 3, the NFD. */
    std::u32string_view nfd;
};

#include "generated/nfc_test.ipp"

}  // namespace webcpp::test

#endif  // WEBCPP_TEST_IDNA_NFC_VECTORS_HPP
```

`test/vectors_test.cpp`; 3,084 is the count of strict lines the prototype's
generator wrote, the lines whose toUnicode field holds no code point UTS 46
marks NV8 or XV8 and none of CONTEXTO:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// The generated test vectors hold every line of the pinned files, their blank fields resolved, as
// the files' headers say: a check of the generator's output that the conformance tests rest on.

#include <boost/core/lightweight_test.hpp>

#include <algorithm>
#include <cstddef>
#include <string_view>

#include "idna_vectors.hpp"
#include "nfc_vectors.hpp"
#include "require.hpp"

using webcpp::test::idna_vector;
using webcpp::test::idna_vectors;
using webcpp::test::nfc_vectors;

namespace {

void every_line_is_there() {
    BOOST_TEST_EQ(nfc_vectors.size(), 20034U);
    BOOST_TEST_EQ(idna_vectors.front().line, 106U);
    BOOST_TEST_EQ(idna_vectors.back().line, 6508U);
}

/** "faß.de; ; ; xn--fa-hia.de; ; fass.de;": blank fields take the value the header names. */
void blank_fields_are_resolved() {
    const idna_vector& line = idna_vectors[1];
    BOOST_TEST_EQ(line.line, 107U);
    BOOST_TEST_EQ(line.source, std::string_view("fa\303\237.de"));
    BOOST_TEST_EQ(line.to_unicode, line.source);
    BOOST_TEST(!line.to_unicode_error);
    BOOST_TEST_EQ(line.to_ascii_n, std::string_view("xn--fa-hia.de"));
    BOOST_TEST_EQ(line.to_ascii_t, std::string_view("fass.de"));
    BOOST_TEST(line.strict);
    BOOST_TEST(line.known_failure.empty());
}

/** Line 548, a then the lone surrogate U+D900 then z, which the generator writes as its three
    bytes, ill-formed UTF-8. */
void a_surrogate_is_ill_formed() {
    const auto at =
        std::ranges::find(idna_vectors, 548U, &idna_vector::line) - idna_vectors.begin();
    if (!BOOST_TEST(at < std::ssize(idna_vectors))) {
        return;
    }
    const idna_vector& line = idna_vectors[static_cast<std::size_t>(at)];
    BOOST_TEST_EQ(line.source, std::string_view("a\355\244\200z"));
    BOOST_TEST(line.to_unicode_error && line.to_ascii_n_error && line.to_ascii_t_error);
}

/** The strict pass keeps the lines whose toUnicode field holds no NV8, XV8 or CONTEXTO code
    point: 3,084 of them in Unicode 17.0.0. */
void the_strict_lines() {
    const auto strict = std::ranges::count_if(idna_vectors, &idna_vector::strict);
    BOOST_TEST_EQ(strict, 3084);
}

void the_first_nfc_line() {
    BOOST_TEST(nfc_vectors.front().source == U"\u1E0A");
    BOOST_TEST(nfc_vectors.front().nfc == U"\u1E0A");
    BOOST_TEST(nfc_vectors.front().nfd == U"\u0044\u0307");
}

}  // namespace

int main() {
    // The cases below read lines by their index.
    webcpp::test::require(BOOST_TEST_EQ(idna_vectors.size(), 6391U));
    every_line_is_there();
    blank_fields_are_resolved();
    a_surrogate_is_ill_formed();
    the_strict_lines();
    the_first_nfc_line();
    return boost::report_errors();
}
```

`test/known_failures.json`:

```json
{
    "unicode": "17.0.0",
    "failures": []
}
```

In `test/Jamfile`, before the line `webcpp.run allocation : allocation_test.cpp ;`
(at the end of the file while the worktree does not hold it yet):

```
# The tests that compile the generated vectors, a large file of data each,
# which MSVC compiles only with /bigobj.
webcpp.run vectors : vectors_test.cpp : <toolset>msvc:<cxxflags>/bigobj ;
```

- [ ] **Step 2: Run it and watch it fail.**

```bash
cd ~/dev/webcpp-idna-vectors
$B2 libs/idna/test//vectors
```

Expected: FAIL, `'generated/idna_test_v2.ipp' file not found`.

- [ ] **Step 3: Extend the generator.** In `tools/generate_tables.py`,
  four edits, after which the file is the whole generator:

Replace the module docstring with:

```python
"""Writes idna's Unicode tables and its conformance vectors from the pinned Unicode files.

Usage: generate_tables.py [--cache DIR] [TABLES IDNA_VECTORS NFC_VECTORS]

The inputs are the files tools/unicode.json pins, each by its URL and its SHA-256. A file missing
from the cache directory (default: bin/unicode/<version>/ of the superproject) is downloaded into
it; every file is hashed, and a digest that differs from the pinned one stops the generator, naming
the file and both digests, before anything is written.

The three outputs are written at the paths given, in this order, or by default at their places in
the library: include/webcpp/idna/detail/tables.hpp, the tables as constexpr data;
test/generated/idna_test_v2.ipp, IdnaTestV2.txt as rows; test/generated/nfc_test.ipp, the first
three columns of NormalizationTest.txt as rows. webcpp.generated runs it with paths in its build
directory, and compares the outputs with the committed files, or copies them over the committed
files for update-generated. Two runs write the same bytes: no date, sorted data, "\\n" line
endings. Nothing is written unless every check passed.

Before writing, the generator checks what the library assumes of the data, and stops naming the
assumption that fails: every status of IdnaMappingTable.txt is one of the five it knows; a code
point is PVALID, CONTEXTJ or CONTEXTO in IANA's IDNA2008 tables exactly when its UTS 46 status is
valid or deviation without an NV8 or XV8 mark, U+002E aside; the CONTEXTJ and CONTEXTO code points
are those of RFC 5892 appendix A; no full canonical decomposition is longer than 4 code points; and
every entry of test/known_failures.json names a line of IdnaTestV2.txt by its number and source.

Exit 0 when every output was written, 1 naming a fault, 2 on a usage error.
"""
```

Replace the constants from `LIBRARY = ` to `OUTPUTS = (TABLES,)` with:

```python
LIBRARY = Path(__file__).resolve().parent.parent
SUPERPROJECT = LIBRARY.parent.parent
PINNED = LIBRARY / 'tools' / 'unicode.json'
KNOWN_FAILURES = LIBRARY / 'test' / 'known_failures.json'

TABLES = 'include/webcpp/idna/detail/tables.hpp'
IDNA_VECTORS = 'test/generated/idna_test_v2.ipp'
NFC_VECTORS = 'test/generated/nfc_test.ipp'
OUTPUTS = (TABLES, IDNA_VECTORS, NFC_VECTORS)
```

Insert this section before the line `# Main`:

```python
# Test vectors


def utf8_literal(text: str) -> str:
    """A C++ string literal of the UTF-8 of `text`: printable ASCII as it is, every other byte as
    a three-digit octal escape, which never runs into the character after it. A lone surrogate is
    encoded as its three bytes, which no well-formed UTF-8 holds."""
    out = ['"']
    for char in text:
        data = char.encode('utf-8', 'surrogatepass')
        for byte in data:
            if byte in (0x22, 0x5C):
                out.append('\\' + chr(byte))
            elif 0x20 <= byte < 0x7F:
                out.append(chr(byte))
            else:
                out.append(f'\\{byte:03o}')
    out.append('"')
    return ''.join(out)


def u32_literal(code_points: Sequence[int]) -> str:
    """A C++ U"" literal of the code points, each a hexadecimal escape."""
    return 'U"' + ''.join(f'\\x{cp:X}' for cp in code_points) + '"'


def unescape(field: str) -> str:
    """A field of IdnaTestV2.txt with its \\uXXXX escapes resolved, "" being the empty string."""
    if field == '""':
        return ''
    out: list[str] = []
    i = 0
    while i < len(field):
        if field.startswith('\\u', i):
            out.append(chr(int(field[i + 2:i + 6], 16)))
            i += 6
        else:
            out.append(field[i])
            i += 1
    return ''.join(out)


def known_failures(path: Path, lines: dict[int, str]) -> dict[int, str]:
    """The reason of each known failure, by line, after checking that each names a line of
    IdnaTestV2.txt with the same source."""
    if not path.is_file():
        return {}
    data = json.loads(path.read_text(encoding='utf-8'))
    found: dict[int, str] = {}
    for entry in data['failures']:
        if lines.get(entry['line']) != entry['source']:
            raise Fault(f'{path}: line {entry["line"]} of IdnaTestV2.txt is not '
                        f'{entry["source"]!r}')
        found[entry['line']] = entry['reason']
    return found


def idna_vectors(version: str, text: str, mapping: Mapping, failures_path: Path) -> str:
    """The text of test/generated/idna_test_v2.ipp: one row per line of IdnaTestV2.txt."""
    parsed: list[tuple[int, list[str]]] = []
    for number, line in enumerate(text.splitlines(), start=1):
        body = line.split('#', 1)[0]
        if body.strip():
            parsed.append((number, [field.strip() for field in body.split(';')]))
    failures = known_failures(failures_path, {number: fields[0] for number, fields in parsed})
    out: list[str] = []
    for number, fields in parsed:
        source = unescape(fields[0])
        to_unicode = unescape(fields[1]) if fields[1] else source
        unicode_status = fields[2] or '[]'
        to_ascii_n = unescape(fields[3]) if fields[3] else to_unicode
        ascii_n_status = fields[4] or unicode_status
        to_ascii_t = unescape(fields[5]) if fields[5] else to_ascii_n
        ascii_t_status = fields[6] or ascii_n_status
        strict = not any(mapping.marked[ord(char)] or ord(char) in CONTEXTO
                         for char in to_unicode)
        reason = failures.get(number, '')
        out.append('    idna_vector{' + ', '.join([
            str(number), utf8_literal(source), utf8_literal(to_unicode),
            str(unicode_status != '[]').lower(), utf8_literal(to_ascii_n),
            str(ascii_n_status != '[]').lower(), utf8_literal(to_ascii_t),
            str(ascii_t_status != '[]').lower(), str(strict).lower(), utf8_literal(reason),
        ]) + '},')
    return vector_file(version, 'IdnaTestV2.txt, a row per test line', 'idna_vector',
                       'idna_vectors', 'test/idna_vectors.hpp', out)


def nfc_vectors(version: str, text: str) -> str:
    """The text of test/generated/nfc_test.ipp: the source, NFC and NFD columns of every line of
    NormalizationTest.txt."""
    out: list[str] = []
    for line in text.splitlines():
        body = line.split('#', 1)[0].strip()
        if not body or body.startswith('@'):
            continue
        columns = [[int(cp, 16) for cp in field.split()] for field in body.split(';')[:3]]
        out.append('    nfc_vector{' + ', '.join(u32_literal(column) for column in columns) + '},')
    return vector_file(version, 'NormalizationTest.txt\'s first three columns, a row per line',
                       'nfc_vector', 'nfc_vectors', 'test/nfc_vectors.hpp', out)


def vector_file(version: str, what: str, kind: str, name: str, declared_in: str,
                rows: Sequence[str]) -> str:
    """A generated file of test vectors: the array `name` of `rows`, whose type `kind` the test
    header `declared_in` declares before it includes the file."""
    return (NOTICE.format(version=version)
            + f'//\n// {what}.\n// The array {name} of webcpp::test::{kind}, which {declared_in}\n'
            '// declares before it includes this file.\n\n'
            '// The rows are positional, as designated initializers would double the file, and\n'
            '// IdnaTestV2.txt tests the Bidi controls in its strings on purpose.\n'
            '// NOLINTBEGIN(modernize-use-designated-initializers, misc-misleading-bidirectional)\n'
            '// clang-format off\n'
            f'inline constexpr std::array<{kind}, {len(rows)}> {name}{{\n'
            + '\n'.join(rows) + '\n};\n// clang-format on\n'
            '// NOLINTEND(modernize-use-designated-initializers, misc-misleading-bidirectional)\n')


```

In `generate`, replace `texts = [tables_header(version, inputs, mapping)]`
with:

```python
    texts = [
        tables_header(version, inputs, mapping),
        idna_vectors(version, inputs['idna/IdnaTestV2.txt'], mapping, KNOWN_FAILURES),
        nfc_vectors(version, inputs['ucd/NormalizationTest.txt']),
    ]
```

- [ ] **Step 4: Run it, twice, and the test.**

```bash
L=~/dev/webcpp/.superpowers/sdd/WEBCPP_IDNA_PLAN/vectors
python3 libs/idna/tools/generate_tables.py
cp libs/idna/test/generated/idna_test_v2.ipp $L/idna-first.ipp
cp libs/idna/test/generated/nfc_test.ipp $L/nfc-first.ipp
python3 libs/idna/tools/generate_tables.py
cmp $L/idna-first.ipp libs/idna/test/generated/idna_test_v2.ipp && echo same-idna
cmp $L/nfc-first.ipp libs/idna/test/generated/nfc_test.ipp && echo same-nfc
git -C libs/idna diff --stat -- include/webcpp/idna/detail/tables.hpp
python3 libs/idna/tools/generate_tables.py a b; echo "exit $?"
$B2 libs/idna/test//vectors
```

Expected: `same-idna`, `same-nfc`; no change to `tables.hpp`; `give no
output path, or all 3` and `exit 2`; `**passed**`. The two `.ipp` files are
about 1.4 MB and 1.0 MB; each opens with the licence notice, the generator's
line and the Unicode data line.

- [ ] **Step 5: See the known-failures check refuse a wrong entry.**

```bash
cp libs/idna/test/known_failures.json $L/known_failures.keep
printf '{\n    "unicode": "17.0.0",\n    "failures": [\n        {"line": 107, "source": "fass.de", "reason": "planted"}\n    ]\n}\n' > libs/idna/test/known_failures.json
python3 libs/idna/tools/generate_tables.py $L/p1 $L/p2 $L/p3; echo "exit $?"
ls $L/p1; cp $L/known_failures.keep libs/idna/test/known_failures.json
git -C libs/idna status --short -- test/known_failures.json
```

Expected: `generate_tables.py: <path>/test/known_failures.json: line 107 of IdnaTestV2.txt is not 'fass.de'`
(line 107 is `faß.de`), `exit 1`, no `p1`; the list as it was.

- [ ] **Step 6: The testing chapter and AGENTS.md.** In `doc/testing.adoc`,
  before the tests table's closing `|===`:

```asciidoc

| `vectors`
| The generated vectors: every line of both files, the blank fields of
  IdnaTestV2.txt resolved, its lone surrogate kept ill-formed, the lines of
  the strict pass.
```

At the end of `AGENTS.md`'s section "Unicode":

```markdown
- **The vectors are generated too.** `test/generated/idna_test_v2.ipp` and
  `test/generated/nfc_test.ipp` hold IdnaTestV2.txt and the first three
  columns of NormalizationTest.txt as rows, so no test reads a file on any
  target. `test/known_failures.json` lists the IdnaTestV2 lines expected to
  fail, by number and source, each with its reason; the generator reads it
  and writes each reason into its line's row, so a change to the list is
  followed by a run of the generator. A listed line must fail under UTS 46
  alone, and the strict pass leaves it out. The list is empty, and may hold
  only a failure that comes from a difference between two versions of
  Unicode.
```

- [ ] **Step 7: Every lane, the page and the lint** (procedure "The task's
  lanes"):

```bash
cd ~/dev/webcpp-idna-vectors
L=~/dev/webcpp/.superpowers/sdd/WEBCPP_IDNA_PLAN/vectors
export EM_CACHE=~/dev/webcpp/.local/emscripten-cache
EXAMPLES="$(test -f libs/idna/example/Jamfile && echo libs/idna/example)"
$B2 -a --build-dir=bin/idna-native libs/idna/test $EXAMPLES > $L/native.log 2>&1 &
$B2 -a --build-dir=bin/idna-gcc toolset=gcc-15 libs/idna/test $EXAMPLES > $L/gcc.log 2>&1 &
$B2 -a --build-dir=bin/idna-wasip2 toolset=clang-wasip2 testing.launcher=wasmtime libs/idna/test $EXAMPLES > $L/wasip2.log 2>&1 &
$B2 -a --build-dir=bin/idna-wasip3 toolset=clang-wasip3 testing.launcher=wasmtime libs/idna/test $EXAMPLES > $L/wasip3.log 2>&1 &
$B2 -a --build-dir=bin/idna-emscripten toolset=emscripten libs/idna/test $EXAMPLES > $L/emscripten.log 2>&1 &
wait
grep -L -e '\.\.\.failed' $L/{native,gcc,wasip2,wasip3,emscripten}.log
$B2 -a --build-dir=bin/idna-doc libs/idna/doc > $L/doc.log 2>&1; echo "doc $?"
tools/lint/lint.sh --clang-format .local/wasi-sdk/bin/clang-format --clang-tidy .local/wasi-sdk/bin/clang-tidy > $L/lint.log 2>&1; tail -1 $L/lint.log
```

Expected also: `vectors` passing on each lane (on MSVC it needs the
`/bigobj` the Jamfile gives).

- [ ] **Step 8: Commit.**

```bash
cd ~/dev/webcpp-idna-vectors/libs/idna
git add tools/generate_tables.py test/known_failures.json test/require.hpp \
    test/idna_vectors.hpp test/nfc_vectors.hpp test/vectors_test.cpp test/Jamfile \
    test/generated/idna_test_v2.ipp test/generated/nfc_test.ipp doc/testing.adoc AGENTS.md
git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "idna: the conformance vectors, generated from Unicode's test files"
git log --format='%an %ae %cn %ce' main..HEAD
```

Expected: the identity line alone.

- [ ] **Step 9: Report,** ending with:

```markdown
## Changelog

No change: the New Libraries line of idna stands as written.
```

### Task 6: The own lane `tables`, the check of the generated files

The spec's increment I2, its check. Worktree `~/dev/webcpp-idna-lane`, from
`main` with Task 5 integrated, made only after `lib/formats` carries S1.

**Needs S1.** `webcpp.generated` is the infra track's
(`~/.buzz/PLANS/WEBCPP_SHARED_INFRA_PLAN.md`, Task 2), committed on
`infra/shared` in `~/dev/webcpp-infra`. Before this task's worktree is made,
the controller rebases the integration space onto it, at a moment no task of
either track is integrating:

```bash
git -c user.name=R -c user.email=github.overboard312@passmail.net -C ~/dev/webcpp-formats rebase infra/shared
git -C ~/dev/webcpp-formats log --format='%an %ae %cn %ce' main..lib/formats | sort -u
```

(`rebase main` in place of `infra/shared` once the infra track has
integrated it). Expected: the identity line alone, or nothing when
`lib/formats` holds no commit of its own.

**Files:**
- Modify: `test/Jamfile`, `doc/unicode.adoc`, `doc/testing.adoc`, `AGENTS.md`

**Interfaces:**
- Consumes (S1): `webcpp.generated <name> : <script> : <argument> * : <output> + ;` in a test Jamfile: the explicit targets `generated-<name>` and `update-generated`; the script runs in the Jamfile's directory as `<script> <argument>... <path-1> ... <path-n>` and writes its i-th output at `<path-i>`; outputs are relative to the Jamfile. `webcpp.lane <name> : <b2-target> + ;`, an own lane that names no target.
- Produces: the own lane `tables` of idna, `b2 -a libs/idna/test//tables`, and `b2 libs/idna/test//update-generated`.

- [ ] **Step 1: Check that S1 is there.**

```bash
cd ~/dev/webcpp-idna-lane
grep -n '^rule generated' tools/webcpp.jam && ls tools/generated/generated.jam tools/generated/generated.py
```

Expected: the rule's line and both files. Without them, stop and report:
the task waits for the rebase.

- [ ] **Step 2: Declare the lane.** At the end of `test/Jamfile`:

```
# The own lane tables: the generator runs again from the pinned Unicode
# files, downloading those its cache lacks, and every output is compared
# with the committed file. The ordinary lanes never run it.
webcpp.generated tables : ../tools/generate_tables.py
  : : ../include/webcpp/idna/detail/tables.hpp generated/idna_test_v2.ipp
      generated/nfc_test.ipp ;
webcpp.lane tables : generated-tables ;
```

- [ ] **Step 3: See the lane fail on a planted difference, and pass, and the
  names stay apart.**

```bash
$B2 -d0 --build-dir=bin/idna-lanes declared-lanes | grep '^idna '
$B2 -n -a --build-dir=bin/idna-dry libs/idna/test//tables 2>&1 | grep -c 'No best alternative'
$B2 -n -a --build-dir=bin/idna-dry libs/idna/test | grep -c properties_test
printf ' ' >> libs/idna/test/generated/nfc_test.ipp
$B2 -a --build-dir=bin/idna-lane libs/idna/test//tables; echo "exit $?"
$B2 --build-dir=bin/idna-lane libs/idna/test//update-generated
git -C libs/idna status --short
$B2 -a --build-dir=bin/idna-lane libs/idna/test//tables; echo "exit $?"
$B2 -n -a --build-dir=bin/idna-dry libs/idna/test | grep -c generate_tables
```

Expected: a line naming idna's lane `tables` in `libs/idna/test` (the infra
track may add a lane kind to the line); `0` (the lane's alias and the test
`properties` are distinct targets); a count above 0; the first lane run fails
naming `generated/nfc_test.ipp` (`contents differ`) and exits non-zero;
`update-generated` writes the file again and `git status` prints nothing
but this task's edits; the second run exits 0; the ordinary lane's dry run
names the generator `0` times.

- [ ] **Step 4: The page and AGENTS.md.** In `doc/unicode.adoc`, replace the
  two lines

```asciidoc
`tools/generate_tables.py`, in idna's repository, writes the tables. Its
inputs are pinned in
```

with

```asciidoc
`tools/generate_tables.py`, in idna's repository, writes the tables and the
conformance vectors of the tests (<<idna-testing>>). Its inputs are pinned in
```

and, after the paragraph that ends "is the appendix <<idna-unicode-data>>.",
insert:

```
The own lane `tables` runs the generator again, into the build directory, and
compares each of its outputs with the committed file, byte for byte, naming
each that differs; `update-generated` writes them in place, and is the one
writer of the generated files. The ordinary lanes never run the generator,
which may need the network; the CI runs the lane on its own.

[source,bash]
----
b2 -a libs/idna/test//tables
b2 libs/idna/test//update-generated
----

```

In `doc/testing.adoc`, after the paragraph that begins "Beside them", the
sentence "The own lane `tables`, `b2 -a libs/idna/test//tables`, checks the
generated files against the generator (<<idna-unicode-generator>>)." at its
end.

At the end of `AGENTS.md`'s section "Unicode":

```markdown
- **The check of the generated files** is the own lane `tables`:
  `b2 -a libs/idna/test//tables` runs the generator into the build directory
  and compares its three outputs with the committed files, and
  `b2 libs/idna/test//update-generated` writes them in place: in a build, it
  is the one writer of the generated files. The ordinary lanes never run it;
  it needs the network until the cache is filled.
```

- [ ] **Step 5: Every lane, the page and the lint** (procedure "The task's
  lanes"):

```bash
cd ~/dev/webcpp-idna-lane
L=~/dev/webcpp/.superpowers/sdd/WEBCPP_IDNA_PLAN/lane
export EM_CACHE=~/dev/webcpp/.local/emscripten-cache
EXAMPLES="$(test -f libs/idna/example/Jamfile && echo libs/idna/example)"
$B2 -a --build-dir=bin/idna-native libs/idna/test $EXAMPLES > $L/native.log 2>&1 &
$B2 -a --build-dir=bin/idna-gcc toolset=gcc-15 libs/idna/test $EXAMPLES > $L/gcc.log 2>&1 &
$B2 -a --build-dir=bin/idna-wasip2 toolset=clang-wasip2 testing.launcher=wasmtime libs/idna/test $EXAMPLES > $L/wasip2.log 2>&1 &
$B2 -a --build-dir=bin/idna-wasip3 toolset=clang-wasip3 testing.launcher=wasmtime libs/idna/test $EXAMPLES > $L/wasip3.log 2>&1 &
$B2 -a --build-dir=bin/idna-emscripten toolset=emscripten libs/idna/test $EXAMPLES > $L/emscripten.log 2>&1 &
wait
grep -L -e '\.\.\.failed' $L/{native,gcc,wasip2,wasip3,emscripten}.log
$B2 -a --build-dir=bin/idna-doc libs/idna/doc > $L/doc.log 2>&1; echo "doc $?"
tools/lint/lint.sh --clang-format .local/wasi-sdk/bin/clang-format --clang-tidy .local/wasi-sdk/bin/clang-tidy > $L/lint.log 2>&1; tail -1 $L/lint.log
```

- [ ] **Step 6: Commit.**

```bash
cd ~/dev/webcpp-idna-lane/libs/idna
git add test/Jamfile doc/unicode.adoc doc/testing.adoc AGENTS.md
git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "idna: the own lane tables, which checks the generated files"
git log --format='%an %ae %cn %ce' main..HEAD
```

Expected: the identity line alone.

- [ ] **Step 7: Report,** ending with:

```markdown
## Changelog

No change: the New Libraries line of idna stands as written.
```

### Task 7: NFC within a label's storage

The spec's increment I3. Worktree `~/dev/webcpp-idna-nfc`, from `main` with
Task 5 integrated.

**Files:**
- Create: `include/webcpp/idna/detail/label_buffer.hpp`, `include/webcpp/idna/detail/nfc.hpp`, `test/nfc_test.cpp`
- Modify: `test/Jamfile`, `doc/testing.adoc`

**Interfaces:**
- Consumes: Task 3's `combining_class_of`, `nfc_quick_check_of`, `decomposition_of`, `composition_of`; Task 5's `nfc_vectors`.
- Produces, in `webcpp::idna::detail` (for Task 9):

```cpp
inline constexpr std::size_t max_label_code_points = 256;
constexpr char lower(char32_t cp) noexcept;            // an ASCII code point's char, A-Z lowercase
class inline_label {                                    // never allocates
public:
    [[nodiscard]] bool push_back(char32_t cp) noexcept; // false when full
    void shrink(std::size_t size) noexcept;
    void clear() noexcept;
    [[nodiscard]] std::span<char32_t> span() noexcept;
    [[nodiscard]] std::u32string_view view() const noexcept;
    [[nodiscard]] std::span<char32_t> reset(std::size_t size) noexcept;  // size <= 256
    [[nodiscard]] std::string_view keep_ascii() noexcept;                // a copy, lowercase
};
class growing_label {                                   // inline, then a pmr vector
public:
    explicit growing_label(std::pmr::memory_resource& resource);
    [[nodiscard]] bool push_back(char32_t cp);          // always true; may throw what the resource throws
    // shrink, clear, span, view as inline_label; reset and keep_ascii allocate when they must
    [[nodiscard]] std::pmr::memory_resource& resource() const noexcept;
};
inline inline_label fresh_like(const inline_label&) noexcept;
inline growing_label fresh_like(const growing_label& label);
namespace nfc {
template <class Label> [[nodiscard]] bool append_decomposed(Label& label, char32_t cp);
inline void reorder(std::span<char32_t> text) noexcept;
constexpr char32_t compose_pair(char32_t first, char32_t second) noexcept;
inline std::size_t compose(std::span<char32_t> text) noexcept;
template <class Label> void compose_label(Label& label) noexcept(!growing<Label>);  // as built: a growing label may throw what its resource throws
enum class verdict : std::uint8_t { yes, no, exhausted };
template <class Label> [[nodiscard]] verdict is_nfc(std::u32string_view text, Label scratch);
}
```

- [ ] **Step 1: Write the failing test.** `test/nfc_test.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// NFC within a label's storage against every line of NormalizationTest.txt, and the storage
// itself: inline, and growing into a memory resource.

#include <webcpp/idna/detail/label_buffer.hpp>
#include <webcpp/idna/detail/nfc.hpp>

#include <boost/core/lightweight_test.hpp>

#include <array>
#include <cstddef>
#include <memory_resource>
#include <string_view>

#include "nfc_vectors.hpp"

namespace detail = webcpp::idna::detail;
namespace nfc = webcpp::idna::detail::nfc;

namespace {

/** `text` decomposed as it is appended, then composed: what the driver does to a mapped label. */
template <class Label>
bool normalize_into(Label& label, std::u32string_view text) {
    for (const char32_t cp : text) {
        if (!nfc::append_decomposed(label, cp)) {
            return false;
        }
    }
    nfc::compose_label(label);
    return true;
}

void every_line_of_normalization_test() {
    for (const webcpp::test::nfc_vector& line : webcpp::test::nfc_vectors) {
        for (const std::u32string_view column : {line.source, line.nfc, line.nfd}) {
            detail::inline_label label;
            BOOST_TEST(normalize_into(label, column));
            BOOST_TEST(label.view() == line.nfc);
        }
        BOOST_TEST(nfc::is_nfc(line.nfc, detail::inline_label{}) == nfc::verdict::yes);
        const bool unchanged = line.source == line.nfc;
        BOOST_TEST_EQ(nfc::is_nfc(line.source, detail::inline_label{}) == nfc::verdict::yes,
                      unchanged);
    }
}

/** Hangul syllables decompose and compose by Unicode section 3.12's arithmetic: GA, and GAG. */
void hangul() {
    detail::inline_label label;
    BOOST_TEST(nfc::append_decomposed(label, 0xAC01));
    BOOST_TEST(label.view() == U"\u1100\u1161\u11A8");
    nfc::compose_label(label);
    BOOST_TEST(label.view() == U"\uAC01");
}

/** A full inline label refuses the next code point; a growing one moves to its resource. */
void the_storage() {
    detail::inline_label full;
    for (std::size_t i = 0; i < detail::max_label_code_points; ++i) {
        BOOST_TEST(full.push_back(U'a'));
    }
    BOOST_TEST(!full.push_back(U'a'));
    BOOST_TEST(!nfc::append_decomposed(full, 0x00E9));

    std::array<std::byte, 4096> storage{};
    std::pmr::monotonic_buffer_resource resource(storage.data(), storage.size(),
                                                 std::pmr::null_memory_resource());
    detail::growing_label growing(resource);
    for (std::size_t i = 0; i <= detail::max_label_code_points; ++i) {
        BOOST_TEST(growing.push_back(U'a'));
    }
    BOOST_TEST_EQ(growing.view().size(), detail::max_label_code_points + 1);
    growing.shrink(3);
    BOOST_TEST(growing.view() == U"aaa");
    growing.clear();
    BOOST_TEST(growing.view().empty());
    BOOST_TEST_EQ(growing.reset(300).size(), 300U);
    BOOST_TEST_EQ(growing.reset(4).size(), 4U);
}

/** An A-label's chars are copied out, lowercase, before its code points are decoded in place. */
void an_a_label_kept_as_ascii() {
    detail::inline_label label;
    for (const char32_t cp : std::u32string_view(U"XN--Bcher-KVA")) {
        BOOST_TEST(label.push_back(cp));
    }
    BOOST_TEST_EQ(label.keep_ascii(), std::string_view("xn--bcher-kva"));
    BOOST_TEST_EQ(label.reset(9).size(), 9U);
    BOOST_TEST_EQ(label.keep_ascii().size(), 9U);
}

/** NFC_Quick_Check's Maybe normalizes a scratch copy: U+0301 composes with an e before it, not
    with an e-acute; a copy that does not fit the scratch storage says so. */
void is_nfc_maybe() {
    BOOST_TEST(nfc::is_nfc(U"e\u0301", detail::inline_label{}) == nfc::verdict::no);
    BOOST_TEST(nfc::is_nfc(U"\u00E9\u0301", detail::inline_label{}) == nfc::verdict::yes);
    std::array<char32_t, detail::max_label_code_points> text{};
    text.fill(0x00E9);  // NFC_Quick_Check Yes, and two code points decomposed
    text.back() = 0x0301;
    BOOST_TEST(nfc::is_nfc(std::u32string_view(text.data(), text.size()), detail::inline_label{}) ==
               nfc::verdict::exhausted);
}

}  // namespace

int main() {
    every_line_of_normalization_test();
    hangul();
    the_storage();
    an_a_label_kept_as_ascii();
    is_nfc_maybe();
    return boost::report_errors();
}
```

In `test/Jamfile`, before the line `webcpp.run allocation : allocation_test.cpp ;`
(at the end of the file while the worktree does not hold it yet):

```
webcpp.run nfc : nfc_test.cpp : <toolset>msvc:<cxxflags>/bigobj ;
```

- [ ] **Step 2: Run it and watch it fail.**

```bash
cd ~/dev/webcpp-idna-nfc
$B2 libs/idna/test//nfc
```

Expected: FAIL, `'webcpp/idna/detail/label_buffer.hpp' file not found`.

- [ ] **Step 3: Write the label storage.** `include/webcpp/idna/detail/label_buffer.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The working storage of one label's code points: inline, and for the forms that
 take a memory resource, a vector of that resource beyond the inline capacity.
*/
#ifndef WEBCPP_IDNA_DETAIL_LABEL_BUFFER_HPP
#define WEBCPP_IDNA_DETAIL_LABEL_BUFFER_HPP

#include <webcpp/idna/config.hpp>

#include <array>
#include <cstddef>
#include <memory_resource>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace webcpp::idna::detail {

/**
 The code points a label holds inline: every label that can pass the DNS lengths fits, since an
 A-label decodes to at most 59 code points and a full canonical decomposition is at most 4 long.
*/
inline constexpr std::size_t max_label_code_points = 256;

/** The char of the ASCII code point `cp`, an ASCII capital letter in lowercase: an A-label's
    letters are compared in either case (RFC 5890 section 2.3.2.1). */
constexpr char lower(char32_t cp) noexcept {
    const char c = static_cast<char>(cp);
    return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c;
}

/** A label's code points in fixed storage, which never allocates. */
class inline_label {
public:
    /** Appends `cp`, or returns false when the storage is full. */
    [[nodiscard]] bool push_back(char32_t cp) noexcept {
        if (size_ == data_.size()) {
            return false;
        }
        data_[size_++] = cp;
        return true;
    }

    /** Keeps the first `size` code points, `size` being at most the current size. */
    void shrink(std::size_t size) noexcept { size_ = size; }

    /** Removes every code point. */
    void clear() noexcept { size_ = 0; }

    /** The code points, which may be changed in place. */
    [[nodiscard]] std::span<char32_t> span() noexcept { return {data_.data(), size_}; }

    /** The code points. */
    [[nodiscard]] std::u32string_view view() const noexcept { return {data_.data(), size_}; }

    /** Makes the label `size` code points long, their values unspecified, for a decoder to
        write; `size` is at most the inline capacity. */
    [[nodiscard]] std::span<char32_t> reset(std::size_t size) noexcept {
        size_ = size;
        return span();
    }

    /** Copies the label, whose code points are all ASCII, to chars of its own with its letters
        in lowercase, and returns them. */
    [[nodiscard]] std::string_view keep_ascii() noexcept {
        for (std::size_t i = 0; i < size_; ++i) {
            ascii_[i] = lower(data_[i]);
        }
        return {ascii_.data(), size_};
    }

private:
    /** The code points. */
    std::array<char32_t, max_label_code_points> data_{};

    /** The chars @ref keep_ascii copies the label to. */
    std::array<char, max_label_code_points> ascii_{};

    /** The number of code points held. */
    std::size_t size_ = 0;
};

/** A label's code points inline, then in a vector of a memory resource once they outgrow it. */
class growing_label {
public:
    /** An empty label whose storage beyond the inline capacity comes from `resource`. */
    explicit growing_label(std::pmr::memory_resource& resource)
        : spill_(&resource), ascii_(&resource) {}

    /** Appends `cp`, moving the code points to the vector when the inline storage is full. */
    [[nodiscard]] bool push_back(char32_t cp) {
        if (!spilled_ && size_ < inline_.size()) {
            inline_[size_++] = cp;
            return true;
        }
        if (!spilled_) {
            spill_.assign(inline_.begin(), inline_.end());
            spilled_ = true;
        }
        spill_.push_back(cp);
        return true;
    }

    /** Keeps the first `size` code points, `size` being at most the current size. */
    void shrink(std::size_t size) noexcept {
        if (spilled_) {
            spill_.erase(spill_.begin() + static_cast<std::ptrdiff_t>(size), spill_.end());
        } else {
            size_ = size;
        }
    }

    /** Removes every code point, and goes back to the inline storage. */
    void clear() noexcept {
        size_ = 0;
        spill_.clear();
        spilled_ = false;
    }

    /** The code points, which may be changed in place. */
    [[nodiscard]] std::span<char32_t> span() noexcept {
        return spilled_ ? std::span<char32_t>(spill_) : std::span<char32_t>(inline_.data(), size_);
    }

    /** The code points. */
    [[nodiscard]] std::u32string_view view() const noexcept {
        return spilled_ ? std::u32string_view(spill_.data(), spill_.size())
                        : std::u32string_view(inline_.data(), size_);
    }

    /** Makes the label `size` code points long, their values unspecified, for a decoder to
        write. */
    [[nodiscard]] std::span<char32_t> reset(std::size_t size) {
        clear();
        if (size <= inline_.size()) {
            size_ = size;
        } else {
            spill_.assign(size, U'\0');
            spilled_ = true;
        }
        return span();
    }

    /** Copies the label, whose code points are all ASCII, to chars of its own with its letters
        in lowercase, and returns them. */
    [[nodiscard]] std::string_view keep_ascii() {
        ascii_.clear();
        for (const char32_t cp : view()) {
            ascii_.push_back(lower(cp));
        }
        return ascii_;
    }

    /** The memory resource the label grows into. */
    [[nodiscard]] std::pmr::memory_resource& resource() const noexcept {
        return *spill_.get_allocator().resource();
    }

private:
    /** The code points, until they outgrow it. */
    std::array<char32_t, max_label_code_points> inline_{};

    /** The number of code points held inline. */
    std::size_t size_ = 0;

    /** The code points, once they outgrew the inline storage. */
    std::pmr::vector<char32_t> spill_;

    /** The chars @ref keep_ascii copies the label to. */
    std::pmr::string ascii_;

    /** Whether the code points are in `spill_`. */
    bool spilled_ = false;
};

/** An empty inline label, the scratch storage of a check over an inline label. */
inline inline_label fresh_like(const inline_label& /*label*/) noexcept {
    return {};
}

/** An empty label of the resource `label` grows into, the scratch storage of a check over it. */
inline growing_label fresh_like(const growing_label& label) {
    return growing_label(label.resource());
}

}  // namespace webcpp::idna::detail

#endif  // WEBCPP_IDNA_DETAIL_LABEL_BUFFER_HPP
```

- [ ] **Step 4: Write NFC** (Unicode 17.0.0 section 3.11 and UAX 15:
  decomposition as code points are appended, Hangul by section 3.12's
  arithmetic, canonical ordering by a stable insertion sort, and UAX 15's
  composition in place, primary composites only).
  `include/webcpp/idna/detail/nfc.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 Normalization Form C (Unicode 17.0.0 section 3.11, UAX 15) within a label's
 storage: canonical decomposition as code points are appended, then canonical
 ordering and canonical composition in place.
*/
#ifndef WEBCPP_IDNA_DETAIL_NFC_HPP
#define WEBCPP_IDNA_DETAIL_NFC_HPP

#include <webcpp/idna/config.hpp>

#include <webcpp/idna/detail/properties.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <utility>

namespace webcpp::idna::detail::nfc {

/** The first Hangul syllable, and the Hangul constants of Unicode section 3.12. */
inline constexpr char32_t s_base = 0xAC00;
/** The first leading consonant jamo. */
inline constexpr char32_t l_base = 0x1100;
/** The first vowel jamo. */
inline constexpr char32_t v_base = 0x1161;
/** The code point before the first trailing consonant jamo. */
inline constexpr char32_t t_base = 0x11A7;
/** The number of leading consonants. */
inline constexpr char32_t l_count = 19;
/** The number of vowels. */
inline constexpr char32_t v_count = 21;
/** The number of trailing consonants, the absent one included. */
inline constexpr char32_t t_count = 28;
/** The syllables per leading consonant. */
inline constexpr char32_t n_count = v_count * t_count;
/** The number of Hangul syllables. */
inline constexpr char32_t s_count = l_count * n_count;

/** Appends the full canonical decomposition of `cp` to `label`, or returns false when the label's
    storage is full. */
template <class Label>
[[nodiscard]] bool append_decomposed(Label& label, char32_t cp) {
    if (cp >= s_base && cp < s_base + s_count) {
        const char32_t index = cp - s_base;
        const char32_t trailing = t_base + (index % t_count);
        if (!label.push_back(l_base + (index / n_count)) ||
            !label.push_back(v_base + (index % n_count / t_count))) {
            return false;
        }
        return trailing == t_base || label.push_back(trailing);
    }
    const std::u32string_view decomposition = decomposition_of(cp);
    if (decomposition.empty()) {
        return label.push_back(cp);
    }
    for (const char32_t part : decomposition) {
        if (!label.push_back(part)) {
            return false;
        }
    }
    return true;
}

/** Puts every run of non-starters in canonical order: a stable sort by combining class. */
inline void reorder(std::span<char32_t> text) noexcept {
    for (std::size_t i = 1; i < text.size(); ++i) {
        const std::uint8_t cc = combining_class_of(text[i]);
        if (cc == 0) {
            continue;
        }
        for (std::size_t j = i; j > 0 && combining_class_of(text[j - 1]) > cc; --j) {
            std::swap(text[j], text[j - 1]);
        }
    }
}

/** The primary composite of `first` and `second`, Hangul included, or 0 when there is none. */
constexpr char32_t compose_pair(char32_t first, char32_t second) noexcept {
    if (first >= l_base && first < l_base + l_count && second >= v_base &&
        second < v_base + v_count) {
        return s_base + ((((first - l_base) * v_count) + (second - v_base)) * t_count);
    }
    if (first >= s_base && first < s_base + s_count && (first - s_base) % t_count == 0 &&
        second > t_base && second < t_base + t_count) {
        return first + (second - t_base);
    }
    return composition_of(first, second);
}

/** Composes decomposed, canonically ordered `text` in place, and returns its new length. */
inline std::size_t compose(std::span<char32_t> text) noexcept {
    if (text.empty()) {
        return 0;
    }
    std::size_t starter = 0;
    // 256 for a text that starts with a non-starter, which then composes with nothing.
    int last_class = combining_class_of(text[0]) == 0 ? 0 : 256;
    std::size_t written = 1;
    for (std::size_t i = 1; i < text.size(); ++i) {
        const char32_t cp = text[i];
        const int cc = combining_class_of(cp);
        const char32_t composite = compose_pair(text[starter], cp);
        if (composite != 0 && (last_class < cc || last_class == 0)) {
            text[starter] = composite;
            continue;
        }
        if (cc == 0) {
            starter = written;
        }
        last_class = cc;
        text[written++] = cp;
    }
    return written;
}

/** Puts the decomposed code points of `label` in NFC, in place. */
template <class Label>
void compose_label(Label& label) noexcept {
    reorder(label.span());
    label.shrink(compose(label.span()));
}

/** What @ref is_nfc finds. */
enum class verdict : std::uint8_t {
    /** The text is in NFC. */
    yes,
    /** The text is not in NFC. */
    no,
    /** The scratch storage could not hold the text's normalization. */
    exhausted,
};

/**
 Whether `text` is in NFC: NFC_Quick_Check answers Yes or No at once, and for Maybe, `text` is
 normalized into the empty label `scratch` and compared.
*/
template <class Label>
[[nodiscard]] verdict is_nfc(std::u32string_view text, Label scratch) {
    bool maybe = false;
    std::uint8_t last_class = 0;
    for (const char32_t cp : text) {
        const std::uint8_t cc = combining_class_of(cp);
        if (cc != 0 && last_class > cc) {
            return verdict::no;
        }
        last_class = cc;
        const tables::quick_check check = nfc_quick_check_of(cp);
        if (check == tables::quick_check::no) {
            return verdict::no;
        }
        maybe = maybe || check == tables::quick_check::maybe;
    }
    if (!maybe) {
        return verdict::yes;
    }
    for (const char32_t cp : text) {
        if (!append_decomposed(scratch, cp)) {
            return verdict::exhausted;
        }
    }
    compose_label(scratch);
    return scratch.view() == text ? verdict::yes : verdict::no;
}

}  // namespace webcpp::idna::detail::nfc

#endif  // WEBCPP_IDNA_DETAIL_NFC_HPP
```

- [ ] **Step 5: Run it and watch it pass.**

```bash
$B2 libs/idna/test//nfc
```

Expected: `**passed**`, every line of NormalizationTest.txt holding.

- [ ] **Step 6: The page.** In `doc/testing.adoc`, before the tests table's
  closing `|===`:

```asciidoc

| `nfc`
| NFC within a label's storage against every line of
  NormalizationTest.txt, its quick check and its slow path, Hangul, and the
  storage, inline and growing.
```

- [ ] **Step 7: Every lane, the page and the lint** (procedure "The task's
  lanes"):

```bash
cd ~/dev/webcpp-idna-nfc
L=~/dev/webcpp/.superpowers/sdd/WEBCPP_IDNA_PLAN/nfc
export EM_CACHE=~/dev/webcpp/.local/emscripten-cache
EXAMPLES="$(test -f libs/idna/example/Jamfile && echo libs/idna/example)"
$B2 -a --build-dir=bin/idna-native libs/idna/test $EXAMPLES > $L/native.log 2>&1 &
$B2 -a --build-dir=bin/idna-gcc toolset=gcc-15 libs/idna/test $EXAMPLES > $L/gcc.log 2>&1 &
$B2 -a --build-dir=bin/idna-wasip2 toolset=clang-wasip2 testing.launcher=wasmtime libs/idna/test $EXAMPLES > $L/wasip2.log 2>&1 &
$B2 -a --build-dir=bin/idna-wasip3 toolset=clang-wasip3 testing.launcher=wasmtime libs/idna/test $EXAMPLES > $L/wasip3.log 2>&1 &
$B2 -a --build-dir=bin/idna-emscripten toolset=emscripten libs/idna/test $EXAMPLES > $L/emscripten.log 2>&1 &
wait
grep -L -e '\.\.\.failed' $L/{native,gcc,wasip2,wasip3,emscripten}.log
$B2 -a --build-dir=bin/idna-doc libs/idna/doc > $L/doc.log 2>&1; echo "doc $?"
tools/lint/lint.sh --clang-format .local/wasi-sdk/bin/clang-format --clang-tidy .local/wasi-sdk/bin/clang-tidy > $L/lint.log 2>&1; tail -1 $L/lint.log
```

Expected also: `nfc` passing on each lane.

- [ ] **Step 8: Commit.**

```bash
cd ~/dev/webcpp-idna-nfc/libs/idna
git add include/webcpp/idna/detail/label_buffer.hpp include/webcpp/idna/detail/nfc.hpp \
    test/nfc_test.cpp test/Jamfile doc/testing.adoc
git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "idna: NFC within a label's storage, inline or growing"
git log --format='%an %ae %cn %ce' main..HEAD
```

Expected: the identity line alone.

- [ ] **Step 9: Report,** ending with:

```markdown
## Changelog

No change: the New Libraries line of idna stands as written.
```

### Task 8: The contextual rules of RFC 5892 and the Bidi rule of RFC 5893

The spec's increment I4, its rules. Worktree `~/dev/webcpp-idna-rules`, from
`main` with Task 3 integrated.

**Files:**
- Create: `include/webcpp/idna/detail/context.hpp`, `include/webcpp/idna/detail/bidi.hpp`, `test/context_test.cpp`, `test/bidi_test.cpp`
- Modify: `test/Jamfile`, `doc/testing.adoc`

**Interfaces:**
- Consumes: Task 3's `joining_type_of`, `combining_class_of`, `script_of`, `bidi_class_of`.
- Produces, in `webcpp::idna::detail` (for Task 9):

```cpp
namespace context {
constexpr bool is_contextj(char32_t cp) noexcept;
constexpr bool is_contexto(char32_t cp) noexcept;
constexpr bool check_joiners(std::u32string_view label) noexcept;  // A.1, A.2, every occurrence
constexpr bool check_others(std::u32string_view label) noexcept;   // A.3 to A.9
}
namespace bidi {
struct label_bidi { bool rtl = false; bool satisfied = true; };
constexpr label_bidi check(std::u32string_view label) noexcept;   // label not empty: the caller checks
}
```

- [ ] **Step 1: Write the failing tests.** `test/context_test.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// The contextual rules of RFC 5892 appendix A, each allowed and refused, over a label's code
// points: the cases of the appendix, and those an implementation that lists the joining scripts by
// hand gets wrong (NKo, Mandaic, a non-joining neighbour).

#include <webcpp/idna/detail/context.hpp>

#include <boost/core/lightweight_test.hpp>

#include <string_view>

namespace context = webcpp::idna::detail::context;

namespace {

/** A.1: after a virama, or between L or D and R or D with only T between, at every occurrence. */
void zero_width_non_joiner() {
    BOOST_TEST(context::check_joiners(U"\u0915\u094D\u200C\u0937"));        // after a virama
    BOOST_TEST(context::check_joiners(U"\u0628\u200C\u0628"));              // D, D
    BOOST_TEST(context::check_joiners(U"\u0628\u200C\u0627"));              // D, R
    BOOST_TEST(context::check_joiners(U"\u0628\u064B\u200C\u064B\u0628"));  // T between
    BOOST_TEST(context::check_joiners(U"\u07CA\u200C\u07CA"));              // NKo, D, D
    BOOST_TEST(context::check_joiners(U"\u0841\u200C\u0841"));              // Mandaic, D, D
    BOOST_TEST(!context::check_joiners(U"\u0627\u200C\u0628"));             // R before
    BOOST_TEST(!context::check_joiners(U"\u0628\u0621\u200C\u0628"));       // U before, HAMZA
    BOOST_TEST(!context::check_joiners(U"\u0628\u200C\u0621\u0628"));       // U after
    BOOST_TEST(!context::check_joiners(U"\u200C\u0628"));
    BOOST_TEST(!context::check_joiners(U"\u0628\u200C"));
    BOOST_TEST(!context::check_joiners(U"a\u200Cb"));
    // The first occurrence is allowed, the second is not.
    BOOST_TEST(!context::check_joiners(U"\u0915\u094D\u200C\u0937x\u200Cy"));
}

/** A.2: only after a virama. */
void zero_width_joiner() {
    BOOST_TEST(context::check_joiners(U"\u0915\u094D\u200D\u0937"));
    BOOST_TEST(!context::check_joiners(U"\u0628\u200D\u0628"));
    BOOST_TEST(!context::check_joiners(U"\u200D"));
}

void middle_dot_keraia_geresh() {
    BOOST_TEST(context::check_others(U"l\u00B7l"));
    BOOST_TEST(!context::check_others(U"a\u00B7l"));
    BOOST_TEST(!context::check_others(U"\u00B7l"));
    BOOST_TEST(!context::check_others(U"l\u00B7a"));
    BOOST_TEST(!context::check_others(U"l\u00B7"));
    BOOST_TEST(context::check_others(U"\u03B1\u0375\u03B2"));
    BOOST_TEST(!context::check_others(U"\u03B1\u0375S"));
    BOOST_TEST(!context::check_others(U"\u03B1\u0375"));
    BOOST_TEST(context::check_others(U"\u05D1\u05F3"));
    BOOST_TEST(context::check_others(U"\u05D1\u05F4"));
    BOOST_TEST(!context::check_others(U"\u05F3\u05D1"));
    BOOST_TEST(!context::check_others(U"a\u05F4"));
}

void katakana_middle_dot_and_digits() {
    BOOST_TEST(context::check_others(U"\u30FB\u3041"));  // Hiragana
    BOOST_TEST(context::check_others(U"\u30A2\u30FB"));  // Katakana
    BOOST_TEST(context::check_others(U"\u4E00\u30FB"));  // Han
    BOOST_TEST(!context::check_others(U"def\u30FBabc"));
    BOOST_TEST(!context::check_others(U"\u30FB"));
    BOOST_TEST(context::check_others(U"\u0660\u0661"));
    BOOST_TEST(context::check_others(U"\u06F0\u06F1"));
    BOOST_TEST(!context::check_others(U"\u0660\u06F0"));
    BOOST_TEST(!context::check_others(U"\u06F9\u0669"));
}

void the_sets() {
    BOOST_TEST(context::is_contextj(0x200C) && context::is_contextj(0x200D));
    BOOST_TEST(!context::is_contextj(0x200B));
    for (const char32_t cp : U"\u00B7\u0375\u05F3\u05F4\u30FB\u0660\u0669\u06F0\u06F9") {
        if (cp != 0) {
            BOOST_TEST(context::is_contexto(cp));
        }
    }
    BOOST_TEST(!context::is_contexto(U'l'));
}

}  // namespace

int main() {
    zero_width_non_joiner();
    zero_width_joiner();
    middle_dot_keraia_geresh();
    katakana_middle_dot_and_digits();
    the_sets();
    return boost::report_errors();
}
```

`test/bidi_test.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// The six rules of RFC 5893 section 2 over one label, each satisfied and broken, and whether a
// label makes its domain a Bidi domain name.

#include <webcpp/idna/detail/bidi.hpp>

#include <boost/core/lightweight_test.hpp>

#include <string_view>

namespace bidi = webcpp::idna::detail::bidi;

namespace {

/** The label is RTL in RFC 5893's sense, and satisfies the rules or not. */
void is(std::u32string_view label, bool rtl, bool satisfied) {
    const bidi::label_bidi found = bidi::check(label);
    BOOST_TEST_EQ(found.rtl, rtl);
    BOOST_TEST_EQ(found.satisfied, satisfied);
}

void rtl_labels() {
    is(U"\u05D0\u05D1", true, true);
    is(U"\u0627\u0644", true, true);          // AL
    is(U"\u05D0\u0300", true, true);          // rule 3: R, then NSM
    is(U"\u05D01", true, true);               // rule 3: ends with EN
    is(U"\u05D0a", true, false);              // rule 2: L in an RTL label
    is(U"\u05D0-", true, false);              // rule 3: ends with ES
    is(U"\u05D00\u0660\u05D0", true, false);  // rule 4: EN and AN
}

void ltr_labels() {
    is(U"abc", false, true);
    is(U"a0", false, true);       // rule 6: ends with EN
    is(U"a\u0300", false, true);  // rule 6: L, then NSM
    is(U"a-", false, false);      // rule 6: ends with ES
    is(U"a\u05D0", true, false);  // rule 5: R in an LTR label
    is(U"a\u0660", true, false);  // rule 5: AN in an LTR label
}

/** Rule 1: the first code point is L, R or AL. */
void first_code_point() {
    is(U"0a", false, false);
    is(U"0\u05D0", true, false);
    is(U"\u0660", true, false);
    is(U"-a", false, false);
}

}  // namespace

int main() {
    rtl_labels();
    ltr_labels();
    first_code_point();
    return boost::report_errors();
}
```

In `test/Jamfile`, before the line `webcpp.run allocation : allocation_test.cpp ;`
(at the end of the file while the worktree does not hold it yet):

```
webcpp.run context : context_test.cpp ;
webcpp.run bidi : bidi_test.cpp ;
```

- [ ] **Step 2: Run them and watch them fail.**

```bash
cd ~/dev/webcpp-idna-rules
$B2 libs/idna/test//context libs/idna/test//bidi
```

Expected: FAIL, `'webcpp/idna/detail/context.hpp' file not found` and
`'webcpp/idna/detail/bidi.hpp' file not found`.

- [ ] **Step 3: Write the rules.** `include/webcpp/idna/detail/context.hpp`,
  RFC 5892 appendix A rule by rule; A.1 is checked at every occurrence of
  U+200C, scanning outward over Joining_Type T from that occurrence, and
  never by a script list:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The contextual rules of RFC 5892 appendix A over a label's code points:
 CONTEXTJ (A.1, A.2) for the joiners, CONTEXTO (A.3 to A.9) for the others.
*/
#ifndef WEBCPP_IDNA_DETAIL_CONTEXT_HPP
#define WEBCPP_IDNA_DETAIL_CONTEXT_HPP

#include <webcpp/idna/config.hpp>

#include <webcpp/idna/detail/properties.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace webcpp::idna::detail::context {

/** The combining class of a virama, which A.1 and A.2 accept before a joiner. */
inline constexpr std::uint8_t virama = 9;

/** Whether `cp` is an ARABIC-INDIC DIGIT, U+0660 to U+0669. */
constexpr bool is_arabic_indic_digit(char32_t cp) noexcept {
    return cp >= 0x0660 && cp <= 0x0669;
}

/** Whether `cp` is an EXTENDED ARABIC-INDIC DIGIT, U+06F0 to U+06F9. */
constexpr bool is_extended_arabic_indic_digit(char32_t cp) noexcept {
    return cp >= 0x06F0 && cp <= 0x06F9;
}

/** Whether `cp` is one of the CONTEXTJ code points, U+200C and U+200D. */
constexpr bool is_contextj(char32_t cp) noexcept {
    return cp == 0x200C || cp == 0x200D;
}

/** Whether `cp` is one of the CONTEXTO code points RFC 5892 appendix A gives a rule. */
constexpr bool is_contexto(char32_t cp) noexcept {
    return cp == 0x00B7 || cp == 0x0375 || cp == 0x05F3 || cp == 0x05F4 || cp == 0x30FB ||
           is_arabic_indic_digit(cp) || is_extended_arabic_indic_digit(cp);
}

/** A.1's regular expression around the ZERO WIDTH NON-JOINER at `at`: an L or D before it and an
    R or D after it, with only T between. */
constexpr bool joins_around(std::u32string_view label, std::size_t at) noexcept {
    std::size_t before = at;
    tables::joining_type type = tables::joining_type::t;
    while (type == tables::joining_type::t) {
        if (before == 0) {
            return false;
        }
        type = joining_type_of(label[--before]);
    }
    if (type != tables::joining_type::l && type != tables::joining_type::d) {
        return false;
    }
    std::size_t after = at;
    type = tables::joining_type::t;
    while (type == tables::joining_type::t) {
        if (++after == label.size()) {
            return false;
        }
        type = joining_type_of(label[after]);
    }
    return type == tables::joining_type::r || type == tables::joining_type::d;
}

/** Whether the joiner at `at` satisfies A.1 (U+200C) or A.2 (U+200D). */
constexpr bool joiner_allowed(std::u32string_view label, std::size_t at) noexcept {
    if (at > 0 && combining_class_of(label[at - 1]) == virama) {
        return true;
    }
    return label[at] == 0x200C && joins_around(label, at);
}

/** Whether every joiner of `label` satisfies its rule, A.1 or A.2. */
constexpr bool check_joiners(std::u32string_view label) noexcept {
    for (std::size_t at = 0; at < label.size(); ++at) {
        if (is_contextj(label[at]) && !joiner_allowed(label, at)) {
            return false;
        }
    }
    return true;
}

/** Whether `label` holds a code point of `kind`. */
template <class Predicate>
constexpr bool holds(std::u32string_view label, Predicate kind) noexcept {
    return std::ranges::any_of(label, kind);
}

/** Whether the CONTEXTO code point at `at` satisfies its rule, A.3 to A.9. */
constexpr bool other_allowed(std::u32string_view label, std::size_t at) noexcept {
    const char32_t cp = label[at];
    const bool has_before = at > 0;
    const bool has_after = at + 1 < label.size();
    switch (cp) {
        case 0x00B7:
            return has_before && has_after && label[at - 1] == U'l' && label[at + 1] == U'l';
        case 0x0375: return has_after && script_of(label[at + 1]) == tables::script::greek;
        case 0x05F3:
        case 0x05F4: return has_before && script_of(label[at - 1]) == tables::script::hebrew;
        case 0x30FB:
            return holds(label, [](char32_t other) {
                const tables::script s = script_of(other);
                return s == tables::script::hiragana || s == tables::script::katakana ||
                       s == tables::script::han;
            });
        default: break;
    }
    if (is_arabic_indic_digit(cp)) {
        return !holds(label, is_extended_arabic_indic_digit);
    }
    return !holds(label, is_arabic_indic_digit);
}

/** Whether every CONTEXTO code point of `label` satisfies its rule. */
constexpr bool check_others(std::u32string_view label) noexcept {
    for (std::size_t at = 0; at < label.size(); ++at) {
        if (is_contexto(label[at]) && !other_allowed(label, at)) {
            return false;
        }
    }
    return true;
}

}  // namespace webcpp::idna::detail::context

#endif  // WEBCPP_IDNA_DETAIL_CONTEXT_HPP
```

`include/webcpp/idna/detail/bidi.hpp`, RFC 5893 section 2 over bit sets of
Bidi classes (32-bit unsigned, so that no operand of a bitwise operator is
promoted to `int`):

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The Bidi rule of RFC 5893 section 2 over one label, and what a label tells of
 its domain: whether it makes the domain a Bidi domain name.
*/
#ifndef WEBCPP_IDNA_DETAIL_BIDI_HPP
#define WEBCPP_IDNA_DETAIL_BIDI_HPP

#include <webcpp/idna/config.hpp>

#include <webcpp/idna/detail/properties.hpp>

#include <cstdint>
#include <string_view>

namespace webcpp::idna::detail::bidi {

/** What one label says of the Bidi rule. */
struct label_bidi {
    /** Whether the label holds an R, AL or AN code point, which makes its domain a Bidi domain
        name (RFC 5893 section 1.4). */
    bool rtl = false;

    /** Whether the label satisfies the six rules of RFC 5893 section 2. */
    bool satisfied = true;
};

/** The one-bit set of the Bidi class `c`. */
constexpr std::uint32_t bit(tables::bidi_class c) noexcept {
    return 1U << static_cast<std::uint32_t>(c);
}

/** R, AL and AN: a label that holds one is an RTL label. */
inline constexpr std::uint32_t right_to_left =
    bit(tables::bidi_class::r) | bit(tables::bidi_class::al) | bit(tables::bidi_class::an);

/** The classes RFC 5893 rule 2 allows in an RTL label. */
inline constexpr std::uint32_t rtl_allowed =
    right_to_left | bit(tables::bidi_class::en) | bit(tables::bidi_class::es) |
    bit(tables::bidi_class::cs) | bit(tables::bidi_class::et) | bit(tables::bidi_class::on) |
    bit(tables::bidi_class::bn) | bit(tables::bidi_class::nsm);

/** The classes RFC 5893 rule 5 allows in an LTR label. */
inline constexpr std::uint32_t ltr_allowed =
    bit(tables::bidi_class::l) | bit(tables::bidi_class::en) | bit(tables::bidi_class::es) |
    bit(tables::bidi_class::cs) | bit(tables::bidi_class::et) | bit(tables::bidi_class::on) |
    bit(tables::bidi_class::bn) | bit(tables::bidi_class::nsm);

/** The classes an RTL label may end with before its NSMs (rule 3). */
inline constexpr std::uint32_t rtl_ends = bit(tables::bidi_class::r) | bit(tables::bidi_class::al) |
                                          bit(tables::bidi_class::en) | bit(tables::bidi_class::an);

/** The classes an LTR label may end with before its NSMs (rule 6). */
inline constexpr std::uint32_t ltr_ends = bit(tables::bidi_class::l) | bit(tables::bidi_class::en);

/** EN and AN, which an RTL label may not hold both of (rule 4). */
inline constexpr std::uint32_t numbers = bit(tables::bidi_class::en) | bit(tables::bidi_class::an);

/**
 RFC 5893 section 2 over `label`, which is not empty: its caller checks emptiness first.

 Tip: the rules' direction is that of the first code point, R or AL for RTL, as rule 1 leaves
 only L, R and AL there; a label that starts with L and holds R, AL or AN is RTL in RFC 5893's
 terms and fails rule 2, and fails rule 5 here, so the verdict is the same.
*/
constexpr label_bidi check(std::u32string_view label) noexcept {
    std::uint32_t seen = 0;
    tables::bidi_class last = tables::bidi_class::other;
    for (const char32_t cp : label) {
        const tables::bidi_class c = bidi_class_of(cp);
        seen |= bit(c);
        last = c == tables::bidi_class::nsm ? last : c;
    }
    label_bidi found{.rtl = (seen & right_to_left) != 0, .satisfied = true};
    const tables::bidi_class first = bidi_class_of(label.front());
    const bool rtl = first == tables::bidi_class::r || first == tables::bidi_class::al;
    if (!rtl && first != tables::bidi_class::l) {
        found.satisfied = false;  // rule 1
    } else if (rtl) {
        found.satisfied = (seen & ~rtl_allowed) == 0 && (bit(last) & rtl_ends) != 0 &&
                          (seen & numbers) != numbers;
    } else {
        found.satisfied = (seen & ~ltr_allowed) == 0 && (bit(last) & ltr_ends) != 0;
    }
    return found;
}

}  // namespace webcpp::idna::detail::bidi

#endif  // WEBCPP_IDNA_DETAIL_BIDI_HPP
```

- [ ] **Step 4: Run them and watch them pass.**

```bash
$B2 libs/idna/test//context libs/idna/test//bidi
```

Expected: two `**passed**`.

- [ ] **Step 5: The page.** In `doc/testing.adoc`, before the tests table's
  closing `|===`:

```asciidoc

| `context`, `bidi`
| Each rule of RFC 5892 appendix A and of RFC 5893 section 2, satisfied and
  broken, the joining scripts a hand-written list misses among them.
```

- [ ] **Step 6: Every lane, the page and the lint** (procedure "The task's
  lanes"):

```bash
cd ~/dev/webcpp-idna-rules
L=~/dev/webcpp/.superpowers/sdd/WEBCPP_IDNA_PLAN/rules
export EM_CACHE=~/dev/webcpp/.local/emscripten-cache
EXAMPLES="$(test -f libs/idna/example/Jamfile && echo libs/idna/example)"
$B2 -a --build-dir=bin/idna-native libs/idna/test $EXAMPLES > $L/native.log 2>&1 &
$B2 -a --build-dir=bin/idna-gcc toolset=gcc-15 libs/idna/test $EXAMPLES > $L/gcc.log 2>&1 &
$B2 -a --build-dir=bin/idna-wasip2 toolset=clang-wasip2 testing.launcher=wasmtime libs/idna/test $EXAMPLES > $L/wasip2.log 2>&1 &
$B2 -a --build-dir=bin/idna-wasip3 toolset=clang-wasip3 testing.launcher=wasmtime libs/idna/test $EXAMPLES > $L/wasip3.log 2>&1 &
$B2 -a --build-dir=bin/idna-emscripten toolset=emscripten libs/idna/test $EXAMPLES > $L/emscripten.log 2>&1 &
wait
grep -L -e '\.\.\.failed' $L/{native,gcc,wasip2,wasip3,emscripten}.log
$B2 -a --build-dir=bin/idna-doc libs/idna/doc > $L/doc.log 2>&1; echo "doc $?"
tools/lint/lint.sh --clang-format .local/wasi-sdk/bin/clang-format --clang-tidy .local/wasi-sdk/bin/clang-tidy > $L/lint.log 2>&1; tail -1 $L/lint.log
```

Expected also: `context` and `bidi` passing on each lane.

- [ ] **Step 7: Commit.**

```bash
cd ~/dev/webcpp-idna-rules/libs/idna
git add include/webcpp/idna/detail/context.hpp include/webcpp/idna/detail/bidi.hpp \
    test/context_test.cpp test/bidi_test.cpp test/Jamfile doc/testing.adoc
git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "idna: the contextual rules of RFC 5892 and the Bidi rule of RFC 5893"
git log --format='%an %ae %cn %ce' main..HEAD
```

Expected: the identity line alone.

- [ ] **Step 8: Report,** ending with:

```markdown
## Changelog

No change: the New Libraries line of idna stands as written.
```

### Task 9: The label driver, and IDNA2008 validity of labels and domains

The spec's increment I4, its code. Worktree `~/dev/webcpp-idna-validity`,
from `main` with Tasks 2, 4, 7 and 8 integrated. Unblocks formats F5
(`check_label`).

**Files:**
- Create: `include/webcpp/idna/detail/process.hpp`, `include/webcpp/idna/validity.hpp`, `test/process_test.cpp`, `test/validity_test.cpp`, `doc/validity.adoc`
- Modify: `include/webcpp/idna.hpp`, `test/allocation_test.cpp`, `test/Jamfile`, `doc/idna.adoc`, `doc/testing.adoc`

**Interfaces:**
- Consumes: Task 2's `utf8::is_valid`, `utf8::decode`, `utf8::encode`, `punycode::decode`, `detail::punycode::encode`; Task 3's lookups; Task 7's `inline_label`, `growing_label`, `fresh_like`, `lower`, `nfc::*`; Task 8's `context::check_joiners`, `context::check_others`, `bidi::check`.
- Produces (spec section 4.4; frozen):

```cpp
namespace webcpp::idna {
[[nodiscard]] result<void> check_label(std::string_view label, const options& opts = options()) noexcept;
[[nodiscard]] result<void> check_domain(std::string_view domain, const options& opts = options()) noexcept;
[[nodiscard]] bool is_valid_label(std::string_view label, const options& opts = options()) noexcept;
[[nodiscard]] bool is_valid_domain(std::string_view domain, const options& opts = options()) noexcept;
}
```

- Produces, in `webcpp::idna::detail` (for Task 11):

```cpp
enum class mode : std::uint8_t { map, as_given };
enum class form : std::uint8_t { ascii, unicode };
class span_sink { public: explicit span_sink(std::span<char>) noexcept; void put(std::string_view) noexcept;
                  std::size_t written() const noexcept; bool cut() const noexcept; };
class string_sink { public: explicit string_sink(std::pmr::string&) noexcept; void put(std::string_view); };
struct discard_sink { void put(std::string_view) noexcept; };
template <class Label, class Sink>
boost::system::error_code process(std::string_view domain, const options& opts, mode m, form f,
                                  Label& label, Sink& sink);
```

- Behaviour the driver fixes (spec sections 5.3, 5.4 and 6): ill-formed UTF-8 is refused before anything; labels are processed in order and, in the ASCII form, the first error stops the driver; within a label the checks run V1, V2/V3 (or V4), V6, V7, U1, CONTEXTJ, NV8/XV8, CONTEXTO, each on a label the driver has checked is not empty; an A-label that decodes to nothing is `invalid_punycode` at once, so no check reads it; the Bidi rule across labels and the domain's length are decided at the end; an A-label's letters are lowercased before it is decoded, and in `mode::as_given` its re-encoding must give it back; a label beyond the inline storage, while gathered or in the NFC check's copy, is `label_too_long` in the ASCII form under `verify_dns_length` and `storage_exhausted` otherwise, and it stops the driver keeping an error recorded before it.

- [ ] **Step 1: Write the failing tests.** `test/process_test.cpp`, the driver
  in both forms on what ends a label early:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// The label driver in both forms, on the inputs that end a label early: an A-label that decodes to
// nothing, and a label beyond the inline storage after an earlier error. test/Jamfile builds it
// with the standard library's assertions on, so that a read past an empty label traps.

#include <webcpp/idna/detail/process.hpp>

#include <boost/core/lightweight_test.hpp>

#include <array>
#include <string>
#include <string_view>

namespace detail = webcpp::idna::detail;
namespace idna = webcpp::idna;

namespace {

/** Runs the driver over `domain` in `f`, and returns what it wrote and its first error. */
std::pair<std::string, boost::system::error_code> run(std::string_view domain, detail::mode m,
                                                      detail::form f,
                                                      const idna::options& opts = idna::options()) {
    std::array<char, 1024> out{};
    detail::inline_label label;
    detail::span_sink sink(out);
    const boost::system::error_code found = detail::process(domain, opts, m, f, label, sink);
    return {std::string(out.data(), sink.written()), found};
}

/** "xn--" decodes to an empty label: invalid_punycode, and nothing is read of it (UTS 46 P4,
    IdnaTestV2.txt line 550). */
void an_a_label_that_decodes_to_nothing() {
    for (const detail::form f : {detail::form::ascii, detail::form::unicode}) {
        const auto [written, found] = run("xn--", detail::mode::map, f);
        BOOST_TEST(found == idna::error::invalid_punycode);
        BOOST_TEST(written.empty());
    }
    const auto [written, found] = run("a.xn--.b", detail::mode::map, detail::form::unicode);
    BOOST_TEST(found == idna::error::invalid_punycode);
    BOOST_TEST_EQ(written, "a..b");
    BOOST_TEST(run("xn--", detail::mode::as_given, detail::form::ascii).second ==
               idna::error::invalid_punycode);
}

/** A label beyond the storage stops the driver, but the Unicode form keeps the error recorded
    before it, and writes no more after the last label it ended. */
void the_first_error_survives_the_storage() {
    const std::string domain = "a_b." + std::string(300, 'a');
    const auto [written, found] = run(domain, detail::mode::map, detail::form::unicode);
    BOOST_TEST(found == idna::error::not_ldh);
    BOOST_TEST_EQ(written, "a_b");
}

}  // namespace

int main() {
    an_a_label_that_decodes_to_nothing();
    the_first_error_survives_the_storage();
    return boost::report_errors();
}
```

`test/validity_test.cpp`, with Review Focus 1, 3 and 5
(`separators_as_given`, `control_characters_as_given`,
`the_inline_storage_edge`, `beyond_the_inline_storage`):

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// IDNA2008's validity of labels and domains as they are given: every rule with the error it
// reports, the strings the JSON Schema suite's idn-hostname finds every UTS 46 library wrong on, as
// U-labels and as A-labels, and the DNS lengths at their limits.

#include <webcpp/idna/validity.hpp>

#include <boost/core/lightweight_test.hpp>

#include <string>
#include <string_view>

namespace idna = webcpp::idna;

namespace {

/** check_domain refuses `domain` with `expected`, and is_valid_domain agrees. */
void refuses(std::string_view domain, idna::error expected,
             const idna::options& opts = idna::options()) {
    const idna::result<void> found = idna::check_domain(domain, opts);
    if (BOOST_TEST(found.has_error())) {
        BOOST_TEST_EQ(found.error(), idna::make_error_code(expected));
    }
    BOOST_TEST(!idna::is_valid_domain(domain, opts));
}

/** check_domain accepts `domain`, and is_valid_domain agrees. */
void accepts(std::string_view domain, const idna::options& opts = idna::options()) {
    const idna::result<void> found = idna::check_domain(domain, opts);
    if (!BOOST_TEST(found.has_value())) {
        BOOST_TEST_EQ(found.error(), boost::system::error_code());
    }
    BOOST_TEST(idna::is_valid_domain(domain, opts));
}

/** The 15 invalid idn-hostname strings of the JSON Schema suite that UTS 46 alone accepts, as
    U-labels, and the 13 A-labels of its hostname tests. */
void what_uts46_alone_accepts() {
    refuses("\354\213\244\343\200\256\353\241\200.\355\205\214\354\212\244\355\212\270",
            idna::error::not_idna2008);                      // U+302E
    refuses("\331\200\337\272", idna::error::not_idna2008);  // U+0640 U+07FA
    refuses(
        "\343\200\261\343\200\262\343\200\263\343\200\264\343\200\265\343\200\256\343\200\257\343"
        "\200\273",
        idna::error::not_idna2008);
    refuses("a\302\267l", idna::error::contexto);
    refuses("\302\267l", idna::error::contexto);
    refuses("l\302\267a", idna::error::contexto);
    refuses("l\302\267", idna::error::contexto);
    // The suite's "α͵S", its S lowercase: as given, a capital letter is disallowed first.
    refuses("\316\261\315\265s", idna::error::contexto);
    refuses("\316\261\315\265", idna::error::contexto);
    refuses("\327\263\327\221", idna::error::contexto);  // geresh first
    refuses("\327\264\327\221", idna::error::contexto);  // gershayim first
    refuses("def\343\203\273abc", idna::error::contexto);
    refuses("\343\203\273", idna::error::contexto);
    refuses("0a.\327\220", idna::error::bidi);
    refuses("xn--7a", idna::error::not_idna2008);  // U+00A1
    for (const std::string_view a_label :
         {"xn--07jt112bpxg.xn--9t4b11yi5a", "xn--chb89f", "xn--07jceefgh4c"}) {
        refuses(a_label, idna::error::not_idna2008);
    }
    for (const std::string_view a_label : {
             "xn--al-0ea",
             "xn--l-fda",
             "xn--la-0ea",
             "xn--l-gda",
             "xn--s-jib3p",
             "xn--wva3j",
             "xn--5db1e",
             "xn--5db3e",
             "xn--defabc-k64e",
             "xn--vek",
         }) {
        refuses(a_label, idna::error::contexto);
    }
    refuses("0a.xn--4db", idna::error::bidi);
}

/** Their valid neighbours, and the same strings under UTS 46 alone. */
void what_idna2008_accepts() {
    accepts("l\302\267l");
    accepts("\316\261\315\265\316\262");
    accepts("\327\221\327\263");
    accepts("\343\203\273\343\201\201");
    accepts("a.\327\220");
    accepts("0a.b");  // no RTL label: not a Bidi domain name
    accepts("xn--bcher-kva.example");
    accepts("b\303\274cher.example");
    const idna::options uts46_alone{.idna2008 = false};
    accepts("a\302\267l", uts46_alone);
    accepts("xn--7a", uts46_alone);
    accepts("0a.\327\220", idna::options{.check_bidi = false});
}

/** An A-label is "xn--" in any ASCII case, decoded, validated and re-encoded. */
void a_labels() {
    BOOST_TEST(idna::is_valid_label("xn--bcher-kva"));
    BOOST_TEST(idna::is_valid_label("XN--BCHER-KVA"));
    BOOST_TEST(idna::check_label("xn--").error() == idna::error::invalid_punycode);
    BOOST_TEST(idna::check_label("xn--abc-").error() == idna::error::invalid_punycode);
    BOOST_TEST(idna::check_label("xn---9uc").error() == idna::error::invalid_punycode);
    BOOST_TEST(idna::check_label("xn--zz").error() == idna::error::invalid_punycode);
    BOOST_TEST(idna::check_label("xn--b\303\274cher").error() == idna::error::invalid_punycode);
    BOOST_TEST(idna::check_label("xn--ls8h").error() == idna::error::not_idna2008);  // U+1F4A9
    BOOST_TEST(idna::is_valid_label("xn--ls8h", idna::options{.idna2008 = false}));
    // A decoded label that starts with "xn--": V2 with CheckHyphens, V4 without.
    BOOST_TEST(idna::check_label("xn--xn---3ra").error() ==
               idna::error::hyphen_in_third_and_fourth);
    BOOST_TEST(idna::check_label("xn--xn---3ra", idna::options{.check_hyphens = false}).error() ==
               idna::error::ace_prefix);
}

/** NR-LDH labels, R-LDH labels, and every rule of a U-label as it is given, nothing mapped. */
void labels_as_given() {
    BOOST_TEST(idna::is_valid_label("example"));
    BOOST_TEST(idna::check_label("ab--cd").error() == idna::error::hyphen_in_third_and_fourth);
    BOOST_TEST(idna::is_valid_label("ab--cd", idna::options{.check_hyphens = false}));
    BOOST_TEST(idna::check_label("-ab").error() == idna::error::hyphen_at_start_or_end);
    BOOST_TEST(idna::check_label("ab-").error() == idna::error::hyphen_at_start_or_end);
    BOOST_TEST(idna::check_label("Example").error() == idna::error::disallowed);
    BOOST_TEST(idna::check_label("e\314\201").error() == idna::error::not_nfc);
    BOOST_TEST(idna::check_label("\314\201e").error() == idna::error::leading_combining_mark);
    BOOST_TEST(idna::check_label("a_b").error() == idna::error::not_ldh);
    const idna::options no_std3{.use_std3_ascii_rules = false};
    BOOST_TEST(idna::check_label("a_b", no_std3).error() == idna::error::not_idna2008);
    BOOST_TEST(idna::is_valid_label(
        "a_b", idna::options{.use_std3_ascii_rules = false, .idna2008 = false}));
    BOOST_TEST(idna::check_label("a\342\200\214b").error() == idna::error::contextj);
    BOOST_TEST(idna::is_valid_label("a\342\200\214b", idna::options{.check_joiners = false}));
    BOOST_TEST(idna::check_label("a.b").error() == idna::error::disallowed);
    BOOST_TEST(idna::check_label("\303").error() == idna::error::invalid_utf8);
    BOOST_TEST(idna::check_label("0\327\220").error() == idna::error::bidi);
}

/** Labels of 63 and 64 octets in their ASCII form, names of 253 and 254, and empty labels. */
void lengths() {
    const std::string label_63(63, 'a');
    BOOST_TEST(idna::is_valid_label(label_63));
    BOOST_TEST(idna::check_label(label_63 + "a").error() == idna::error::label_too_long);
    // 55 a and a u-umlaut make an A-label of 63 octets; 56 make one of 64.
    BOOST_TEST(idna::is_valid_label(std::string(55, 'a') + "\303\274"));
    BOOST_TEST(idna::check_label(std::string(56, 'a') + "\303\274").error() ==
               idna::error::label_too_long);
    const std::string name_253 =
        label_63 + "." + label_63 + "." + label_63 + "." + std::string(61, 'a');
    BOOST_TEST_EQ(name_253.size(), 253U);
    accepts(name_253);
    refuses(name_253 + "a", idna::error::domain_too_long);
    refuses("", idna::error::empty_label);
    refuses("a..b", idna::error::empty_label);
    refuses("a.", idna::error::empty_label);  // the root label, under VerifyDnsLength
    accepts("a.", idna::options{.verify_dns_length = false});
    accepts("a..b", idna::options{.verify_dns_length = false});
}

/** A label beyond the inline storage is too long for the DNS; without its lengths, it is
    storage_exhausted. */
void beyond_the_inline_storage() {
    const std::string long_label(300, 'a');
    const idna::options no_lengths{.verify_dns_length = false};
    BOOST_TEST(idna::check_label(long_label).error() == idna::error::label_too_long);
    BOOST_TEST(idna::check_label(long_label, no_lengths).error() == idna::error::storage_exhausted);
    // 128 e-acute and a combining acute fit as given, but the NFC check's scratch copy holds
    // their decomposition, 257 code points: the same two errors.
    std::string slow;
    for (int i = 0; i < 128; ++i) {
        slow += "\303\251";
    }
    slow += "\314\201";
    BOOST_TEST(idna::check_label(slow).error() == idna::error::label_too_long);
    BOOST_TEST(idna::check_label(slow, no_lengths).error() == idna::error::storage_exhausted);
}

/** Only U+002E separates labels as they are given: U+FF0E is a code point UTS 46 maps, so it is
    disallowed. */
void separators_as_given() {
    BOOST_TEST(idna::check_domain("a\357\274\216b").error() == idna::error::disallowed);
    BOOST_TEST(idna::check_domain("a\343\200\202b").error() == idna::error::disallowed);
    accepts("a.b");
}

/** NUL inside a label: ASCII outside the STD3 rules, and NV8 in UTS 46's table. */
void control_characters_as_given() {
    const std::string_view with_nul("a\0b", 3);
    BOOST_TEST(idna::check_label(with_nul).error() == idna::error::not_ldh);
    BOOST_TEST(idna::check_label(with_nul, idna::options{.use_std3_ascii_rules = false}).error() ==
               idna::error::not_idna2008);
    BOOST_TEST(idna::is_valid_label(
        with_nul, idna::options{.use_std3_ascii_rules = false, .idna2008 = false}));
}

/** The inline storage holds 256 code points of a label as given, and refuses the 257th. */
void the_inline_storage_edge() {
    const idna::options no_lengths{.verify_dns_length = false};
    BOOST_TEST(idna::is_valid_label(std::string(256, 'a'), no_lengths));
    BOOST_TEST(idna::check_label(std::string(257, 'a'), no_lengths).error() ==
               idna::error::storage_exhausted);
}

}  // namespace

int main() {
    what_uts46_alone_accepts();
    what_idna2008_accepts();
    a_labels();
    labels_as_given();
    lengths();
    beyond_the_inline_storage();
    separators_as_given();
    control_characters_as_given();
    the_inline_storage_edge();
    return boost::report_errors();
}
```

In `test/allocation_test.cpp`, replace `#include <webcpp/idna/punycode.hpp>`
with

```cpp
#include <webcpp/idna/punycode.hpp>
#include <webcpp/idna/validity.hpp>
```

replace the function `exercise_span_forms` and its comment with:

```cpp
/**
 Every span form and validator over every input, with the strict profile, UTS 46 alone and no DNS
 lengths: the number of results that succeeded.

 Tip: the count is checked, so no optimizer may drop a call whose result nothing reads.
*/
std::size_t exercise_span_forms(const std::array<std::string, 14>& inputs) {
    std::array<char, 1024> out{};
    std::array<char32_t, 1024> code_points{};
    std::size_t succeeded = 0;
    for (const idna::options& opts : {
             idna::options(),
             idna::options{.idna2008 = false},
             idna::options{.verify_dns_length = false},
         }) {
        for (const std::string& input : inputs) {
            succeeded += idna::is_valid_domain(input, opts) ? 1 : 0;
            succeeded += idna::is_valid_label(input, opts) ? 1 : 0;
            succeeded += idna::punycode::decode(code_points, input).has_value() ? 1 : 0;
        }
    }
    succeeded += idna::punycode::encode(out, U"b\u00FCcher").has_value() ? 1 : 0;
    return succeeded;
}

```

and the comment `/** The span forms allocate nothing, whatever the input. */`
with `/** The span forms and the validators allocate nothing, whatever the
input. */`.

In `test/Jamfile`, before the line `webcpp.run allocation : allocation_test.cpp ;`:

```
webcpp.run process : process_test.cpp ;
webcpp.run validity : validity_test.cpp ;
```

- [ ] **Step 2: Run them and watch them fail.**

```bash
cd ~/dev/webcpp-idna-validity
$B2 libs/idna/test//process libs/idna/test//validity libs/idna/test//allocation
```

Expected: FAIL, `'webcpp/idna/detail/process.hpp' file not found` and
`'webcpp/idna/validity.hpp' file not found`.

- [ ] **Step 3: Write the driver.** `include/webcpp/idna/detail/process.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The label driver uts46.hpp and validity.hpp share: UTS 46 section 4 streamed
 label by label, with or without its mapping, into a sink that takes the ASCII
 form, the Unicode form, or nothing.
*/
#ifndef WEBCPP_IDNA_DETAIL_PROCESS_HPP
#define WEBCPP_IDNA_DETAIL_PROCESS_HPP

#include <webcpp/idna/config.hpp>

#include <webcpp/idna/detail/bidi.hpp>
#include <webcpp/idna/detail/context.hpp>
#include <webcpp/idna/detail/label_buffer.hpp>
#include <webcpp/idna/detail/nfc.hpp>
#include <webcpp/idna/detail/properties.hpp>
#include <webcpp/idna/detail/utf8.hpp>
#include <webcpp/idna/errors.hpp>
#include <webcpp/idna/options.hpp>
#include <webcpp/idna/punycode.hpp>

#include <boost/system/error_code.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <span>
#include <string>
#include <string_view>

namespace webcpp::idna::detail {

/** Whether the driver maps each code point first, or takes the labels as they are given. */
enum class mode : std::uint8_t {
    /** UTS 46 section 4's Map and Normalize, then each label: to_ascii, to_unicode. */
    map,
    /** Each label as it is, split at U+002E only: check_label, check_domain. */
    as_given,
};

/** Which form of each label the driver hands to its sink. */
enum class form : std::uint8_t {
    /** The ASCII form, with the DNS lengths; the first error stops the driver. */
    ascii,
    /** The Unicode form; every error is recorded and the driver goes on. */
    unicode,
};

/** A sink of a caller's span: each piece written whole while it fits, and none after one did
    not. */
class span_sink {
public:
    /** A sink that writes into `output`. */
    explicit span_sink(std::span<char> output) noexcept : output_(output) {}

    /** Writes `piece`, whole, or marks the output cut. */
    void put(std::string_view piece) noexcept {
        if (cut_ || output_.size() - written_ < piece.size()) {
            cut_ = true;
            return;
        }
        std::ranges::copy(piece, output_.begin() + static_cast<std::ptrdiff_t>(written_));
        written_ += piece.size();
    }

    /** The number of chars written. */
    [[nodiscard]] std::size_t written() const noexcept { return written_; }

    /** Whether a piece did not fit. */
    [[nodiscard]] bool cut() const noexcept { return cut_; }

private:
    /** The caller's buffer. */
    std::span<char> output_;

    /** The chars written so far. */
    std::size_t written_ = 0;

    /** Whether a piece did not fit, after which nothing is written. */
    bool cut_ = false;
};

/** A sink of a string of a memory resource, which grows as pieces come. */
class string_sink {
public:
    /** A sink that appends to `output`. */
    explicit string_sink(std::pmr::string& output) noexcept : output_(&output) {}

    /** Appends `piece`. */
    void put(std::string_view piece) { output_->append(piece); }

private:
    /** The string appended to. */
    std::pmr::string* output_;
};

/** A sink that keeps nothing, for the validators. */
struct discard_sink {
    /** Drops `piece`. */
    void put(std::string_view /*piece*/) noexcept {}
};

/** Whether `text` holds only code points below U+0080. */
constexpr bool all_ascii(std::u32string_view text) noexcept {
    return std::ranges::all_of(text, [](char32_t cp) { return cp < 0x80; });
}

/** Whether `text` starts with "xn--", its letters in either case when `any_case`. */
constexpr bool has_ace_prefix(std::u32string_view text, bool any_case) noexcept {
    if (text.size() < 4) {
        return false;
    }
    const auto letter = [&](std::size_t i) {
        return any_case ? static_cast<char32_t>(lower(text[i])) : text[i];
    };
    return letter(0) == U'x' && letter(1) == U'n' && text[2] == U'-' && text[3] == U'-';
}

/** Whether `cp` is a lowercase ASCII letter, a digit or the hyphen. */
constexpr bool is_ldh(char32_t cp) noexcept {
    return (cp >= U'a' && cp <= U'z') || (cp >= U'0' && cp <= U'9') || cp == U'-';
}

/**
 UTS 46 section 4 over a domain, label by label, into a sink.

 Tip: one label's code points are held at a time, in `Label`, so the working storage is one
 label's whatever the domain's length.
*/
template <class Label, class Sink>
class processor {
public:
    /** A driver of `opts` in `m`, handing `f` to `sink`, with `label` as its working storage. */
    processor(const options& opts, mode m, form f, Label& label, Sink& sink) noexcept
        : opts_(opts), mode_(m), form_(f), label_(&label), sink_(&sink) {}

    /** Processes `domain`, and returns the first error, empty when there was none. */
    boost::system::error_code run(std::string_view domain) {
        if (!utf8::is_valid(domain)) {
            return error::invalid_utf8;
        }
        std::size_t i = 0;
        char32_t cp = 0;
        while (utf8::decode(domain, i, cp)) {
            if (!append(cp)) {
                return first_;
            }
        }
        if (end_label(true)) {
            finish();
        }
        return first_;
    }

private:
    /** Records `found` unless an error was recorded before, and says whether the driver goes
        on. */
    bool record(const boost::system::error_code& found) noexcept {
        if (!first_) {
            first_ = found;
        }
        return form_ == form::unicode;
    }

    /** The error of a label beyond the storage: label_too_long when the DNS lengths refuse it
        anyway, as they refuse every such label, storage_exhausted otherwise. */
    [[nodiscard]] error beyond_storage() const noexcept {
        const bool too_long = opts_.verify_dns_length && form_ == form::ascii;
        return too_long ? error::label_too_long : error::storage_exhausted;
    }

    /** Stops the driver at full storage, keeping an error recorded before: no form can go on
        without the label. */
    bool exhausted() noexcept {
        if (!first_) {
            first_ = beyond_storage();
        }
        stopped_ = true;
        return false;
    }

    /** Takes one code point of the input, mapped first in mode::map. */
    bool append(char32_t cp) {
        if (mode_ == mode::as_given) {
            if (cp == U'.') {
                return end_label(false);
            }
            return label_->push_back(cp) || exhausted();
        }
        const uts46_entry entry = uts46_of(cp);
        switch (entry.status) {
            case tables::status::ignored: return true;
            case tables::status::mapped:
                if (opts_.transitional_processing && cp == 0x1E9E) {
                    return emit(U's') && emit(U's');
                }
                return emit_all(entry.mapping);
            case tables::status::deviation:
                return opts_.transitional_processing ? emit_all(entry.mapping) : emit(cp);
            default: return emit(cp);
        }
    }

    /** Emits every code point of a mapping. */
    bool emit_all(std::u32string_view mapping) {
        return std::ranges::all_of(mapping, [this](char32_t cp) { return emit(cp); });
    }

    /** Ends the label at U+002E, or appends the decomposition of a mapped code point. */
    bool emit(char32_t cp) {
        if (cp == U'.') {
            return end_label(false);
        }
        return nfc::append_decomposed(*label_, cp) || exhausted();
    }

    /** Converts and validates the label held, hands it to the sink, and starts the next. */
    bool end_label(bool last) {
        if (stopped_) {
            return false;
        }
        if (mode_ == mode::map) {
            nfc::compose_label(*label_);
        }
        if (labels_ > 0) {
            put_separator();
        }
        ++labels_;
        const std::u32string_view text = label_->view();
        bool go_on = true;
        if (text.empty()) {
            go_on = empty(last);
        } else if (has_ace_prefix(text, mode_ == mode::as_given)) {
            go_on = ace(text);
        } else {
            go_on = plain(text);
        }
        label_->clear();
        return go_on;
    }

    /** UTS 46's empty label: an error under verify_dns_length, but for a final root label in the
        Unicode form (X4_2). */
    bool empty(bool last) noexcept {
        if (!opts_.verify_dns_length || (form_ == form::unicode && last && labels_ > 1)) {
            return true;
        }
        return record(error::empty_label);
    }

    /** A label that does not start with "xn--": validated, then written. */
    bool plain(std::u32string_view text) {
        const boost::system::error_code found =
            validate(text, opts_.transitional_processing, mode_ == mode::as_given);
        if (found && !record(found)) {
            return false;
        }
        note_bidi(text);
        return write(text);
    }

    /** A label that starts with "xn--": decoded, validated as a U-label, then written. */
    bool ace(std::u32string_view text) {
        if (!all_ascii(text)) {
            if (form_ == form::unicode) {
                put_unicode(text);
            }
            return record(error::invalid_punycode);
        }
        const std::string_view original = label_->keep_ascii();
        std::string_view encoded = original;
        encoded.remove_prefix(4);
        const result<std::size_t> decoded =
            punycode::decode(label_->reset(encoded.size()), encoded);
        if (decoded.has_error()) {
            return undecodable(original, decoded.error());
        }
        label_->shrink(decoded.unsafe_value());
        const std::u32string_view unicode = label_->view();
        if (unicode.empty()) {
            return record(error::invalid_punycode);
        }
        if (all_ascii(unicode) && !record(error::invalid_punycode)) {
            return false;
        }
        const boost::system::error_code found = validate(unicode, false, true);
        if (found && !record(found)) {
            return false;
        }
        if (mode_ == mode::as_given && !canonical(unicode, encoded) &&
            !record(error::invalid_punycode)) {
            return false;
        }
        note_bidi(unicode);
        return write(unicode);
    }

    /** An "xn--" label whose Punycode does not decode: kept as it is, with its error unless
        options::ignore_invalid_punycode. */
    bool undecodable(std::string_view original, const boost::system::error_code& failure) {
        if (form_ == form::unicode) {
            sink_->put(original);
        } else {
            put_ascii(original);
        }
        if (opts_.ignore_invalid_punycode) {
            return check_length(original.size());
        }
        return record(failure);
    }

    /** Whether re-encoding `unicode` gives `encoded`, lowercase, back: RFC 5891 section 5.4's
        round trip. */
    static bool canonical(std::u32string_view unicode, std::string_view encoded) noexcept {
        std::size_t at = 0;
        const result<std::size_t> same = detail::punycode::encode(unicode, [&](char c) noexcept {
            if (at == encoded.size() || encoded[at] != c) {
                return false;
            }
            ++at;
            return true;
        });
        return same.has_value() && at == encoded.size();
    }

    /**
     UTS 46 4.1's validity criteria over `text`, which is not empty, with IDNA2008's under
     options::idna2008, in the order of the specification's section 5.3: the first that fails.
    */
    boost::system::error_code validate(std::u32string_view text, bool transitional,
                                       bool check_nfc) {
        if (check_nfc) {
            const nfc::verdict normalized = nfc::is_nfc(text, fresh_like(*label_));
            if (normalized == nfc::verdict::exhausted) {
                return beyond_storage();
            }
            if (normalized == nfc::verdict::no) {
                return error::not_nfc;
            }
        }
        if (const boost::system::error_code hyphens = check_hyphens(text)) {
            return hyphens;
        }
        if (is_mark(text.front())) {
            return error::leading_combining_mark;
        }
        return check_code_points(text, transitional);
    }

    /** V2 and V3 under options::check_hyphens, V4 otherwise, over `text`, which is not empty. */
    [[nodiscard]] boost::system::error_code check_hyphens(std::u32string_view text) const noexcept {
        if (!opts_.check_hyphens) {
            if (has_ace_prefix(text, true)) {
                return error::ace_prefix;
            }
            return {};
        }
        if (text.size() >= 4 && text[2] == U'-' && text[3] == U'-') {
            return error::hyphen_in_third_and_fourth;
        }
        if (text.front() == U'-' || text.back() == U'-') {
            return error::hyphen_at_start_or_end;
        }
        return {};
    }

    /** V7, U1 and CONTEXTJ, then IDNA2008's NV8, XV8 and CONTEXTO, each over the whole label. */
    [[nodiscard]] boost::system::error_code check_code_points(std::u32string_view text,
                                                              bool transitional) const noexcept {
        const auto allowed = [transitional](char32_t cp) {
            const tables::status status = uts46_of(cp).status;
            return status == tables::status::valid ||
                   (status == tables::status::deviation && !transitional);
        };
        if (!std::ranges::all_of(text, allowed)) {
            return error::disallowed;
        }
        const auto ldh = [](char32_t cp) { return cp >= 0x80 || is_ldh(cp); };
        if (opts_.use_std3_ascii_rules && !std::ranges::all_of(text, ldh)) {
            return error::not_ldh;
        }
        if (opts_.check_joiners && !context::check_joiners(text)) {
            return error::contextj;
        }
        if (!opts_.idna2008) {
            return {};
        }
        if (std::ranges::any_of(text, [](char32_t cp) { return uts46_of(cp).not_idna2008; })) {
            return error::not_idna2008;
        }
        if (!context::check_others(text)) {
            return error::contexto;
        }
        return {};
    }

    /** Notes what a validated label says of the Bidi rule. */
    void note_bidi(std::u32string_view text) noexcept {
        const bidi::label_bidi found = bidi::check(text);
        any_rtl_ = any_rtl_ || found.rtl;
        any_unsatisfied_ = any_unsatisfied_ || !found.satisfied;
    }

    /** Writes a validated label in the driver's form. */
    bool write(std::u32string_view text) {
        if (form_ == form::unicode) {
            put_unicode(text);
            return true;
        }
        return write_ascii(text);
    }

    /** Writes the ASCII form of a label, "xn--" and its Punycode when it is not ASCII, and checks
        its length. */
    bool write_ascii(std::u32string_view text) {
        if (all_ascii(text)) {
            for (const char32_t cp : text) {
                const char c = static_cast<char>(cp);
                put_ascii(std::string_view(&c, 1));
            }
            return check_length(text.size());
        }
        put_ascii("xn--");
        const result<std::size_t> encoded = detail::punycode::encode(text, [this](char c) {
            put_ascii(std::string_view(&c, 1));
            return true;
        });
        if (encoded.has_error()) {
            return record(encoded.error());
        }
        return check_length(4 + encoded.unsafe_value());
    }

    /** UTS 46 A4_2 under options::verify_dns_length: a label of at most 63 octets. */
    bool check_length(std::size_t length) noexcept {
        if (opts_.verify_dns_length && length > max_label_length) {
            return record(error::label_too_long);
        }
        return true;
    }

    /** Hands a piece of the ASCII form to the sink, and counts it toward the domain's length. */
    void put_ascii(std::string_view piece) {
        ascii_length_ += piece.size();
        sink_->put(piece);
    }

    /** Hands the separator between two labels to the sink of either form. */
    void put_separator() {
        if (form_ == form::ascii) {
            put_ascii(".");
        } else {
            sink_->put(".");
        }
    }

    /** Hands the UTF-8 of `text` to the sink, a code point at a time. */
    void put_unicode(std::u32string_view text) {
        std::array<char, 4> octets{};
        for (const char32_t cp : text) {
            sink_->put(std::string_view(octets.data(), utf8::encode(cp, octets.data())));
        }
    }

    /** What is decided once every label is seen: the Bidi rule across labels, then the domain's
        length. */
    void finish() noexcept {
        if (opts_.check_bidi && any_rtl_ && any_unsatisfied_) {
            record(error::bidi);
        }
        if (form_ == form::ascii && opts_.verify_dns_length && ascii_length_ > max_domain_length) {
            record(error::domain_too_long);
        }
    }

    /** The flags of the processing. */
    const options& opts_;

    /** Whether code points are mapped first. */
    mode mode_;

    /** The form handed to the sink. */
    form form_;

    /** The working storage of the label at hand. */
    Label* label_;

    /** Where the output goes. */
    Sink* sink_;

    /** The first error recorded, empty while there is none. */
    boost::system::error_code first_;

    /** The labels ended so far. */
    std::size_t labels_ = 0;

    /** The octets of the ASCII form so far, separators included. */
    std::size_t ascii_length_ = 0;

    /** Whether a label holds R, AL or AN: the domain is a Bidi domain name. */
    bool any_rtl_ = false;

    /** Whether a validated label breaks the Bidi rule. */
    bool any_unsatisfied_ = false;

    /** Whether the storage ran out, which ends the processing. */
    bool stopped_ = false;
};

/** Runs a @ref processor over `domain`, and returns its first error. */
template <class Label, class Sink>
boost::system::error_code process(std::string_view domain, const options& opts, mode m, form f,
                                  Label& label, Sink& sink) {
    processor<Label, Sink> driver(opts, m, f, label, sink);
    return driver.run(domain);
}

}  // namespace webcpp::idna::detail

#endif  // WEBCPP_IDNA_DETAIL_PROCESS_HPP
```

- [ ] **Step 4: Write the validators.** `include/webcpp/idna/validity.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 IDNA2008's validity of a label or a domain name as it is given, without
 UTS 46's mapping: RFC 5891 section 5.4, RFC 5892 and RFC 5893.

 @see "Validity", in the guide.
*/
#ifndef WEBCPP_IDNA_VALIDITY_HPP
#define WEBCPP_IDNA_VALIDITY_HPP

#include <webcpp/idna/config.hpp>

#include <webcpp/idna/detail/label_buffer.hpp>
#include <webcpp/idna/detail/process.hpp>
#include <webcpp/idna/errors.hpp>
#include <webcpp/idna/options.hpp>

#include <boost/system/error_code.hpp>

#include <string_view>

namespace webcpp::idna {

/**
 Checks one label, in UTF-8, as it is given: a U-label, an A-label or an
 NR-LDH label.

 An A-label, "xn--" in any ASCII case, is decoded, validated as a U-label,
 re-encoded and compared with its input, ASCII letters in either case. Nothing
 is mapped, so a label not in NFC, or holding a code point UTS 46 would map
 (an ASCII capital letter among them), is refused. The Bidi rule is applied to
 the label as to a domain of one label. Nothing allocates.

 @param label The label, without a separator.
 @param opts The checks to apply; the defaults are the strict profile.
 @return Nothing, or the first @ref error found, in the order of the guide:
 @ref error::disallowed for a U+002E inside `label`, @ref error::storage_exhausted
 for a label beyond the inline storage when `opts.verify_dns_length` is off.
 @see "Validity", in the guide.
*/
[[nodiscard]] inline result<void> check_label(std::string_view label,
                                              const options& opts = options()) noexcept {
    if (!detail::utf8::is_valid(label)) {
        return error::invalid_utf8;
    }
    if (label.find('.') != std::string_view::npos) {
        return error::disallowed;
    }
    detail::inline_label storage;
    detail::discard_sink sink;
    const boost::system::error_code found =
        detail::process(label, opts, detail::mode::as_given, detail::form::ascii, storage, sink);
    if (found) {
        return found;
    }
    return {};
}

/**
 Checks a domain name, in UTF-8, as it is given: every label as
 @ref check_label checks it, the Bidi rule across the labels, and the DNS
 lengths.

 Labels are separated by U+002E alone. Nothing allocates.

 @param domain The domain name.
 @param opts The checks to apply; the defaults are the strict profile.
 @return Nothing, or the first @ref error found: the labels' in their order,
 then @ref error::bidi, then @ref error::domain_too_long.
 @see "Validity", in the guide.
*/
[[nodiscard]] inline result<void> check_domain(std::string_view domain,
                                               const options& opts = options()) noexcept {
    detail::inline_label storage;
    detail::discard_sink sink;
    const boost::system::error_code found =
        detail::process(domain, opts, detail::mode::as_given, detail::form::ascii, storage, sink);
    if (found) {
        return found;
    }
    return {};
}

/**
 Whether @ref check_label finds no error in `label`.

 @param label The label.
 @param opts The checks to apply.
 @return `check_label(label, opts).has_value()`.
 @see "Validity", in the guide.
*/
[[nodiscard]] inline bool is_valid_label(std::string_view label,
                                         const options& opts = options()) noexcept {
    return check_label(label, opts).has_value();
}

/**
 Whether @ref check_domain finds no error in `domain`.

 @param domain The domain name.
 @param opts The checks to apply.
 @return `check_domain(domain, opts).has_value()`.
 @see "Validity", in the guide.
*/
[[nodiscard]] inline bool is_valid_domain(std::string_view domain,
                                          const options& opts = options()) noexcept {
    return check_domain(domain, opts).has_value();
}

}  // namespace webcpp::idna

#endif  // WEBCPP_IDNA_VALIDITY_HPP
```

In `include/webcpp/idna.hpp`, after `#include <webcpp/idna/punycode.hpp>`
(Task 11 puts `uts46.hpp` between them):

```cpp
#include <webcpp/idna/validity.hpp>
```

- [ ] **Step 5: Run them and watch them pass.**

```bash
$B2 libs/idna/test//process libs/idna/test//validity libs/idna/test//allocation
```

Expected: three `**passed**`.

- [ ] **Step 6: See the hardened build catch a read of an empty label.** In
  `ace()`, replace the two guards

```cpp
        if (unicode.empty()) {
            return record(error::invalid_punycode);
        }
        if (all_ascii(unicode) && !record(error::invalid_punycode)) {
            return false;
        }
```

with the one the first draft had,

```cpp
        if ((unicode.empty() || all_ascii(unicode)) && !record(error::invalid_punycode)) {
            return false;
        }
```

and run `$B2 libs/idna/test//process`. Expected: FAIL, the program stops
(`Trace/BPT trap`, exit status 133, natively with libc++; libstdc++'s
assertion message with GCC): in the Unicode form, the empty label went on to
`check_hyphens`, whose `text.front()` the standard library's checks refuse.
Restore the two guards; `git -C libs/idna diff --stat -- include` shows only
this task's new files, and `process` passes again.

- [ ] **Step 7: The page.** `doc/validity.adoc` (Task 10 adds its example's
  listings):

```asciidoc
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

[#idna-validity]
== Validity

`idna::check_label` and `idna::check_domain` say whether a label, or a
domain name, is valid as it is given, by IDNA2008's rules and UTS 46's
validity criteria, without UTS 46's mapping: nothing is lowercased,
normalized or replaced, so a label with a capital letter, or one not in NFC,
is refused. They are the checks a program needs when a name must already be
in its final form, as a hostname's A-labels must. `is_valid_label` and
`is_valid_domain` say the same as a `bool`.

[#idna-validity-rules]
=== The rules

A label is checked in this order, and the first rule it breaks is its error;
`to_ascii` and `to_unicode` apply the same rules to each label after the
mapping, with the NFC check only for a decoded A-label:

. *Normalization Form C* (UTS 46 V1, RFC 5891 section 5.4): `not_nfc`.
. *Hyphens*, under `check_hyphens`: none in both the third and the fourth
  position (V2), `hyphen_in_third_and_fourth`, which keeps such labels free
  for a future prefix, `xn--` among them; none at either end (V3),
  `hyphen_at_start_or_end`. Without `check_hyphens`, a label that starts with
  `xn--` (V4), `ace_prefix`.
. *No leading combining mark* (V6, RFC 5891 section 5.4):
  `leading_combining_mark`.
. *UTS 46's status* (V7): every code point valid, or a deviation in
  nontransitional processing; `disallowed`.
. *STD3 ASCII*, under `use_std3_ascii_rules` (U1): ASCII only `a` to `z`,
  `0` to `9` and the hyphen; `not_ldh`.
. *CONTEXTJ*, under `check_joiners` (RFC 5892 A.1, A.2): U+200C after a
  virama, or between a code point of Joining_Type L or D and one of R or D,
  with only T between; U+200D after a virama; `contextj`.
. *IDNA2008*, under `idna2008`: no code point UTS 46 marks NV8 or XV8,
  `not_idna2008`; then the CONTEXTO rules, `contexto`: U+00B7 MIDDLE DOT
  between two `l` (A.3), U+0375 GREEK KERAIA before a Greek code point
  (A.4), U+05F3 and U+05F4, GERESH and GERSHAYIM, after a Hebrew one (A.5,
  A.6), U+30FB KATAKANA MIDDLE DOT in a label that holds Hiragana, Katakana
  or Han (A.7), and Arabic-Indic digits never beside extended ones (A.8,
  A.9).

Across the labels of a name, under `check_bidi`: when one label holds an R,
AL or AN code point, the name is a Bidi domain name, and then every label
must satisfy RFC 5893 section 2, `bidi`. `0a.א` fails: its first label
starts with a digit, which rule 1 refuses in a Bidi domain name. Last, under
`verify_dns_length`, the lengths of the ASCII form: `empty_label`,
`label_too_long` and `domain_too_long`.

idna derives IDNA2008's property of a code point from UTS 46's table: PVALID,
CONTEXTJ or CONTEXTO exactly when its status is valid or deviation and it
bears no NV8 or XV8 mark. The generator checks this against IANA's
IDNA2008 tables each time it runs (<<idna-unicode>>).
```

In `doc/idna.adoc`, after `include::converting.adoc[]`, the line
`include::validity.adoc[]` between blank lines. In `doc/testing.adoc`,
before the tests table's closing `|===`:

```asciidoc

| `process`
| The label driver in both forms on what ends a label early: an A-label that
  decodes to nothing, and a label beyond the storage after an earlier error.
  Like every test, built with the standard library's own checks on, so that
  a read past an empty label traps.

| `validity`
| `check_label` and `check_domain`: every rule with its error; the strings
  of the JSON Schema suite's `idn-hostname` that UTS 46 alone accepts, as
  U-labels and as A-labels; A-labels in either case and not canonical; the
  lengths at 63 and 64, 253 and 254; the inline storage's limit.
```

and the row of `allocation` reads "The global `operator new`, replaced for
this program alone: no call in a span form or a validator, and in a pmr form
only those of its memory resource."

- [ ] **Step 8: Every lane, the page and the lint** (procedure "The task's
  lanes"):

```bash
cd ~/dev/webcpp-idna-validity
L=~/dev/webcpp/.superpowers/sdd/WEBCPP_IDNA_PLAN/validity
export EM_CACHE=~/dev/webcpp/.local/emscripten-cache
EXAMPLES="$(test -f libs/idna/example/Jamfile && echo libs/idna/example)"
$B2 -a --build-dir=bin/idna-native libs/idna/test $EXAMPLES > $L/native.log 2>&1 &
$B2 -a --build-dir=bin/idna-gcc toolset=gcc-15 libs/idna/test $EXAMPLES > $L/gcc.log 2>&1 &
$B2 -a --build-dir=bin/idna-wasip2 toolset=clang-wasip2 testing.launcher=wasmtime libs/idna/test $EXAMPLES > $L/wasip2.log 2>&1 &
$B2 -a --build-dir=bin/idna-wasip3 toolset=clang-wasip3 testing.launcher=wasmtime libs/idna/test $EXAMPLES > $L/wasip3.log 2>&1 &
$B2 -a --build-dir=bin/idna-emscripten toolset=emscripten libs/idna/test $EXAMPLES > $L/emscripten.log 2>&1 &
wait
grep -L -e '\.\.\.failed' $L/{native,gcc,wasip2,wasip3,emscripten}.log
$B2 -a --build-dir=bin/idna-doc libs/idna/doc > $L/doc.log 2>&1; echo "doc $?"
tools/lint/lint.sh --clang-format .local/wasi-sdk/bin/clang-format --clang-tidy .local/wasi-sdk/bin/clang-tidy > $L/lint.log 2>&1; tail -1 $L/lint.log
```

Expected also: `process`, `validity` and `allocation` passing on each lane.

- [ ] **Step 9: Commit.**

```bash
cd ~/dev/webcpp-idna-validity/libs/idna
git add include/webcpp/idna/detail/process.hpp include/webcpp/idna/validity.hpp \
    include/webcpp/idna.hpp test/process_test.cpp test/validity_test.cpp \
    test/allocation_test.cpp test/Jamfile doc/validity.adoc doc/idna.adoc doc/testing.adoc
git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "idna: the label driver, and IDNA2008 validity of labels and domains"
git log --format='%an %ae %cn %ce' main..HEAD
```

Expected: the identity line alone.

- [ ] **Step 10: Report,** telling the formats track that `check_label` is
  ready, ending with:

```markdown
## Changelog

No change: the New Libraries line of idna stands as written.
```

### Task 10: The validity example, on the page

The spec's increment I4, its page. Worktree `~/dev/webcpp-idna-validity-doc`,
from `main` with Task 9 integrated; runs at the same time as Task 11.

**Files:**
- Create: `example/idna_validity.cpp`, `example/idna_validity.expected`
- Modify: `example/Jamfile`, `doc/validity.adoc`, `doc/introduction.adoc`

**Interfaces:**
- Consumes: Task 9's `check_label`, `is_valid_domain`, `is_valid_label`, `options`.
- Produces: the example `idna_validity` and its regions `labels`, `domains`, `uts46_alone`, which the page shows.

- [ ] **Step 1: The failing example.** In `example/Jamfile`, after the line
  of `idna_punycode.cpp`: `webcpp.example idna_validity.cpp ;`.
  `example/idna_validity.expected`:

```
example: valid
xn--bcher-kva: valid
l·l: valid
a·l: code point outside its allowed context
ab--cd: hyphens in the third and fourth positions
Example: disallowed code point
a.א: valid
0a.א: invalid
0a.b: valid
a·l under UTS 46 alone: valid
```

```bash
cd ~/dev/webcpp-idna-validity-doc
$B2 libs/idna/example//idna_validity
```

Expected: FAIL, `idna_validity.cpp` not found.

- [ ] **Step 2: The program.** `example/idna_validity.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <webcpp/idna/validity.hpp>

#include <iostream>
#include <string_view>

namespace idna = webcpp::idna;

int main() {
    // tag::labels[]
    for (const std::string_view label :
         {"example", "xn--bcher-kva", "l\302\267l", "a\302\267l", "ab--cd", "Example"}) {
        const idna::result<void> checked = idna::check_label(label);
        std::cout << label << ": " << (checked.has_value() ? "valid" : checked.error().message())
                  << '\n';
    }
    // end::labels[]

    // tag::domains[]
    for (const std::string_view domain : {"a.\327\220", "0a.\327\220", "0a.b"}) {
        std::cout << domain << ": " << (idna::is_valid_domain(domain) ? "valid" : "invalid")
                  << '\n';
    }
    // end::domains[]

    // tag::uts46_alone[]
    const idna::options uts46_alone{.idna2008 = false};
    std::cout << "a\302\267l under UTS 46 alone: "
              << (idna::is_valid_label("a\302\267l", uts46_alone) ? "valid" : "invalid") << '\n';
    // end::uts46_alone[]
}
```

```bash
$B2 libs/idna/example//idna_validity
```

Expected: `**passed**`: what it printed equals the `.expected`.

- [ ] **Step 3: The page.** In `doc/validity.adoc`, before the line
  `[#idna-validity-rules]`:

```
[source]
----
include::{examples}/idna_validity.cpp[tag=labels,indent=0]
----

An A-label, `xn--` in any ASCII case, is decoded, its U-label checked, and
re-encoded: an A-label that is not the canonical encoding of its U-label is
refused, as RFC 5891 section 5.4 asks. `check_domain` splits at U+002E alone,
since the other full stops are separators only after UTS 46's mapping, and
adds the rules that span the labels: the Bidi rule, and the lengths.

[source]
----
include::{examples}/idna_validity.cpp[tag=domains,indent=0]
----

The same check under UTS 46 alone accepts what IDNA2008's context rules
refuse:

[source]
----
include::{examples}/idna_validity.cpp[tag=uts46_alone,indent=0]
----

The program prints:

[listing]
----
include::{examples}/idna_validity.expected[]
----

```

In `doc/introduction.adoc`, the table of headers gains, at its end:

```asciidoc

| `<webcpp/idna/validity.hpp>`
| `check_label`, `check_domain` and their `is_valid_` forms (<<idna-validity>>).
```

- [ ] **Step 4: Every lane, the page and the lint** (procedure "The task's
  lanes"):

```bash
cd ~/dev/webcpp-idna-validity-doc
L=~/dev/webcpp/.superpowers/sdd/WEBCPP_IDNA_PLAN/validity-doc
export EM_CACHE=~/dev/webcpp/.local/emscripten-cache
EXAMPLES="$(test -f libs/idna/example/Jamfile && echo libs/idna/example)"
$B2 -a --build-dir=bin/idna-native libs/idna/test $EXAMPLES > $L/native.log 2>&1 &
$B2 -a --build-dir=bin/idna-gcc toolset=gcc-15 libs/idna/test $EXAMPLES > $L/gcc.log 2>&1 &
$B2 -a --build-dir=bin/idna-wasip2 toolset=clang-wasip2 testing.launcher=wasmtime libs/idna/test $EXAMPLES > $L/wasip2.log 2>&1 &
$B2 -a --build-dir=bin/idna-wasip3 toolset=clang-wasip3 testing.launcher=wasmtime libs/idna/test $EXAMPLES > $L/wasip3.log 2>&1 &
$B2 -a --build-dir=bin/idna-emscripten toolset=emscripten libs/idna/test $EXAMPLES > $L/emscripten.log 2>&1 &
wait
grep -L -e '\.\.\.failed' $L/{native,gcc,wasip2,wasip3,emscripten}.log
$B2 -a --build-dir=bin/idna-doc libs/idna/doc > $L/doc.log 2>&1; echo "doc $?"
tools/lint/lint.sh --clang-format .local/wasi-sdk/bin/clang-format --clang-tidy .local/wasi-sdk/bin/clang-tidy > $L/lint.log 2>&1; tail -1 $L/lint.log
```

Expected also: `idna_validity` passing on each lane.

- [ ] **Step 5: Commit.**

```bash
cd ~/dev/webcpp-idna-validity-doc/libs/idna
git add example/Jamfile example/idna_validity.cpp example/idna_validity.expected \
    doc/validity.adoc doc/introduction.adoc
git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "idna: the validity example, on the page"
git log --format='%an %ae %cn %ce' main..HEAD
```

Expected: the identity line alone.

- [ ] **Step 6: Report,** ending with:

```markdown
## Changelog

No change: the New Libraries line of idna stands as written.
```

### Task 11: UTS 46 ToASCII and ToUnicode

The spec's increment I5, its code. Worktree `~/dev/webcpp-idna-uts46`, from
`main` with Task 9 integrated; runs at the same time as Task 10. Unblocks
formats F6 (`to_ascii` for `idn-hostname` and `idna_label_rule`).

**Files:**
- Create: `include/webcpp/idna/uts46.hpp`, `test/uts46_test.cpp`
- Modify: `include/webcpp/idna.hpp`, `test/allocation_test.cpp`, `test/Jamfile`, `doc/testing.adoc`

**Interfaces:**
- Consumes: Task 9's `detail::process`, `mode`, `form`, `span_sink`, `string_sink`; Task 7's `inline_label`, `growing_label`; Task 5's `idna_vectors`.
- Produces (spec section 4.5; frozen):

```cpp
namespace webcpp::idna {
[[nodiscard]] result<std::size_t> to_ascii(std::span<char> output, std::string_view domain,
                                           const options& opts = options()) noexcept;
[[nodiscard]] result<std::pmr::string> to_ascii(std::string_view domain, const options& opts = options(),
    std::pmr::memory_resource& resource = *std::pmr::get_default_resource());
struct unicode_result { std::size_t size = 0; boost::system::error_code error; };
[[nodiscard]] unicode_result to_unicode(std::span<char> output, std::string_view domain,
                                        const options& opts = options()) noexcept;
struct unicode_string { std::pmr::string value; boost::system::error_code error; };
[[nodiscard]] unicode_string to_unicode(std::string_view domain, const options& opts = options(),
    std::pmr::memory_resource& resource = *std::pmr::get_default_resource());
}
```

- The orders of the errors (spec section 6, made exact): `to_ascii` reports a label's error in label order, then `bidi`, then `domain_too_long`, and `output_too_small` last, so a name over 253 octets is `domain_too_long` in a 253-char buffer (Review Focus 2); after an error, the span's contents are unspecified. `to_unicode` reports `output_too_small` whenever the buffer cut the result, else the first error recorded. A to_unicode label that fails keeps the form the processing gave it (mapped, normalized, decoded when it decodes), as IdnaTestV2 requires.

- [ ] **Step 1: Write the failing test.** `test/uts46_test.cpp`, all 6,391
  lines in both forms and both passes (the strict pass leaves out a listed
  known failure), the orders of the errors, `xn--` in the Unicode form, the
  storage after an earlier error, and Review Focus 1 to 5 (`separators`,
  `the_first_error_wins`, `control_characters`, `the_root_label`,
  `a_label_that_decomposes_past_the_storage`):

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// UTS 46 processing against every line of IdnaTestV2.txt, toUnicode, toAsciiN and toAsciiT, in
// both the span and the pmr forms: first with options::idna2008 off and every other flag on, as the
// file assumes, then the strict profile over the lines whose toUnicode field holds no NV8, XV8 or
// CONTEXTO code point. A line of test/known_failures.json must fail, and every other must pass.

#include <webcpp/idna/detail/utf8.hpp>
#include <webcpp/idna/uts46.hpp>

#include <boost/core/lightweight_test.hpp>

#include <array>
#include <cstddef>
#include <memory_resource>
#include <string>
#include <string_view>

#include "idna_vectors.hpp"

namespace idna = webcpp::idna;
using webcpp::test::idna_vector;
using webcpp::test::idna_vectors;

namespace {

/** Room for every result of the file, whose longest line is far below it. */
using buffer = std::array<char, 1024>;

/** Whether ToASCII gives `expected`, or an error when `error_expected`, in both forms. */
bool to_ascii_agrees(std::string_view source, const idna::options& opts, std::string_view expected,
                     bool error_expected) {
    buffer out{};
    const idna::result<std::size_t> span = idna::to_ascii(out, source, opts);
    const idna::result<std::pmr::string> owned = idna::to_ascii(source, opts);
    if (span.has_value() != owned.has_value() || span.has_error() != error_expected) {
        return false;
    }
    if (error_expected) {
        return span.error() == owned.error();
    }
    const std::string_view written(out.data(), span.value());
    return written == expected && owned.value() == expected;
}

/** Whether ToUnicode gives `expected`, with an error when `error_expected`, in both forms. A
    source that is not well-formed UTF-8 (a lone surrogate of the file) writes nothing and is
    invalid_utf8, which IdnaTestV2 allows ("Implementations that cannot work with ill-formed
    strings would skip test cases that contain them"). */
bool to_unicode_agrees(std::string_view source, const idna::options& opts,
                       std::string_view expected, bool error_expected) {
    buffer out{};
    const idna::unicode_result span = idna::to_unicode(out, source, opts);
    const idna::unicode_string owned = idna::to_unicode(source, opts);
    if (!idna::detail::utf8::is_valid(source)) {
        return span.size == 0 && span.error == idna::error::invalid_utf8 &&
               owned.error == idna::error::invalid_utf8;
    }
    const std::string_view written(out.data(), span.size);
    return written == expected && owned.value == expected && span.error == owned.error &&
           static_cast<bool>(span.error) == error_expected;
}

/** Whether every column of `line` holds under `opts`. */
bool line_holds(const idna_vector& line, const idna::options& opts) {
    idna::options transitional = opts;
    transitional.transitional_processing = true;
    return to_unicode_agrees(line.source, opts, line.to_unicode, line.to_unicode_error) &&
           to_ascii_agrees(line.source, opts, line.to_ascii_n, line.to_ascii_n_error) &&
           to_ascii_agrees(line.source, transitional, line.to_ascii_t, line.to_ascii_t_error);
}

/** A line holds unless it is a known failure, which must fail: the list never lies. */
void check_line(const idna_vector& line, const idna::options& opts, const char* pass) {
    const bool holds = line_holds(line, opts);
    if (holds == line.known_failure.empty()) {
        return;
    }
    const std::string message =
        std::string(pass) + " pass, IdnaTestV2.txt line " + std::to_string(line.line) +
        (holds ? ": passes, but test/known_failures.json lists it" : ": fails");
    BOOST_ERROR(message.c_str());
}

void idna_test_v2() {
    const idna::options uts46_alone{.idna2008 = false};
    for (const idna_vector& line : idna_vectors) {
        check_line(line, uts46_alone, "UTS 46");
    }
}

/** The strict pass leaves out a known failure, whose entry is the UTS 46 pass's: a line may fail
    there for a reason the strict pass never meets. */
void idna_test_v2_strict() {
    for (const idna_vector& line : idna_vectors) {
        if (line.strict && line.known_failure.empty()) {
            check_line(line, idna::options(), "strict");
        }
    }
}

/** With verify_dns_length, a name over 253 octets is domain_too_long, never output_too_small, in
    the 253 chars the guide says always suffice. */
void the_length_comes_before_the_buffer() {
    std::array<char, idna::max_domain_length> out{};
    std::string name;
    for (int i = 0; i < 30; ++i) {
        name += "abcdefghi.";
    }
    name += 'x';
    BOOST_TEST(idna::to_ascii(out, name).error() == idna::error::domain_too_long);
    std::array<char, 5> tiny{};
    BOOST_TEST(idna::to_ascii(tiny, "b\303\274cher.example").error() ==
               idna::error::output_too_small);
}

/** ToUnicode writes whole code points, and says when the buffer could not hold the result. */
void to_unicode_stops_at_a_code_point() {
    std::array<char, 2> out{};
    const idna::unicode_result cut = idna::to_unicode(out, "b\303\274cher");
    BOOST_TEST_EQ(cut.size, 1U);
    BOOST_TEST(cut.error == idna::error::output_too_small);
}

/** ToUnicode converts as much as it can: an error does not stop it. */
void to_unicode_goes_on() {
    buffer out{};
    const idna::unicode_result result = idna::to_unicode(out, "A..xn--bcher-kva");
    BOOST_TEST_EQ(std::string_view(out.data(), result.size), "a..b\303\274cher");
    BOOST_TEST(result.error == idna::error::empty_label);
    const idna::unicode_result ill_formed = idna::to_unicode(out, "a\303");
    BOOST_TEST_EQ(ill_formed.size, 0U);
    BOOST_TEST(ill_formed.error == idna::error::invalid_utf8);
    // "xn--" decodes to nothing (IdnaTestV2.txt line 550): an error, and nothing read of it.
    const idna::unicode_result empty = idna::to_unicode(out, "xn--");
    BOOST_TEST_EQ(empty.size, 0U);
    BOOST_TEST(empty.error == idna::error::invalid_punycode);
}

/** The strict profile's defaults, as idn-hostname asks: STD3 rules, the DNS lengths, IDNA2008. */
void the_defaults() {
    buffer out{};
    BOOST_TEST(idna::to_ascii(out, "a_b.example").error() == idna::error::not_ldh);
    BOOST_TEST(idna::to_ascii(out, "example.").error() == idna::error::empty_label);
    BOOST_TEST(idna::to_ascii(out, "\342\231\245.example").error() == idna::error::not_idna2008);
    const idna::result<std::size_t> mapped = idna::to_ascii(out, "B\303\234CHER.Example");
    BOOST_TEST_EQ(std::string_view(out.data(), mapped.value()), "xn--bcher-kva.example");
    // Transitional processing maps U+1E9E, capital sharp s, to "ss".
    const idna::result<std::size_t> sharp =
        idna::to_ascii(out, "\341\272\236", idna::options{.transitional_processing = true});
    BOOST_TEST_EQ(std::string_view(out.data(), sharp.value()), "ss");
}

/** An "xn--" label that does not decode is kept as it is, with no error, under
    ignore_invalid_punycode. */
void ignore_invalid_punycode() {
    buffer out{};
    BOOST_TEST(idna::to_ascii(out, "xn--zz.example").error() == idna::error::invalid_punycode);
    const idna::result<std::size_t> kept =
        idna::to_ascii(out, "xn--zz.example", idna::options{.ignore_invalid_punycode = true});
    BOOST_TEST_EQ(std::string_view(out.data(), kept.value()), "xn--zz.example");
}

/** With verify_dns_length off, a label beyond the inline storage is storage_exhausted in the span
    form, and converted by the pmr form, whose resource holds it. */
void beyond_the_inline_storage() {
    const std::string long_label(300, 'a');
    const idna::options no_lengths{.verify_dns_length = false};
    std::array<char, 400> out{};
    BOOST_TEST(idna::to_ascii(out, long_label, no_lengths).error() ==
               idna::error::storage_exhausted);
    BOOST_TEST(idna::to_ascii(out, long_label).error() == idna::error::label_too_long);
    const idna::result<std::pmr::string> owned = idna::to_ascii(long_label, no_lengths);
    BOOST_TEST(owned.has_value() && std::string_view(owned.value()) == long_label);
    // The storage ends the processing, but keeps the error recorded before it; and it is found
    // while the label is gathered, before the label's own checks (guide, "Limits").
    const idna::unicode_result after_error = idna::to_unicode(out, "a_b." + long_label);
    BOOST_TEST(after_error.error == idna::error::not_ldh);
    BOOST_TEST_EQ(std::string_view(out.data(), after_error.size), "a_b");
    BOOST_TEST(idna::to_ascii(out, "\357\277\277" + long_label).error() ==
               idna::error::label_too_long);
}

/** The four separators of UTS 46: each alone or at an edge leaves an empty label, and each
    between two labels splits them. */
void separators() {
    buffer out{};
    for (const std::string_view stop : {".", "\343\200\202", "\357\274\216", "\357\275\241"}) {
        const std::string alone(stop);
        BOOST_TEST(idna::to_ascii(out, alone).error() == idna::error::empty_label);
        BOOST_TEST(idna::to_ascii(out, alone + "example").error() == idna::error::empty_label);
        BOOST_TEST(idna::to_ascii(out, "example" + alone).error() == idna::error::empty_label);
        const idna::result<std::size_t> split = idna::to_ascii(out, "a" + alone + "b");
        BOOST_TEST_EQ(std::string_view(out.data(), split.value()), "a.b");
    }
}

/** A name with several faults reports the first label's, then the Bidi rule, then the length. */
void the_first_error_wins() {
    buffer out{};
    BOOST_TEST(idna::to_ascii(out, "a_b.xn--zz").error() == idna::error::not_ldh);
    BOOST_TEST(idna::to_ascii(out, "xn--zz.a_b").error() == idna::error::invalid_punycode);
    BOOST_TEST(idna::to_ascii(out, "0a.\327\220.a_b").error() == idna::error::not_ldh);
    BOOST_TEST(idna::to_ascii(out, "0a.\327\220").error() == idna::error::bidi);
}

/** NUL and the other C0 controls are ASCII outside the STD3 rules, and NV8 in UTS 46's table. */
void control_characters() {
    buffer out{};
    const std::string_view with_nul("a\0b", 3);
    BOOST_TEST(idna::to_ascii(out, with_nul).error() == idna::error::not_ldh);
    BOOST_TEST(
        idna::to_ascii(out, with_nul, idna::options{.use_std3_ascii_rules = false}).error() ==
        idna::error::not_idna2008);
    const idna::result<std::size_t> loose = idna::to_ascii(
        out, with_nul, idna::options{.use_std3_ascii_rules = false, .idna2008 = false});
    BOOST_TEST(std::string_view(out.data(), loose.value()) == with_nul);
    const idna::unicode_result shown = idna::to_unicode(out, with_nul);
    BOOST_TEST(std::string_view(out.data(), shown.size) == with_nul);
    BOOST_TEST(shown.error == idna::error::not_ldh);
}

/** A final empty label is the DNS root: refused by ToASCII under VerifyDnsLength, kept by
    ToUnicode, and passed through by both without it. */
void the_root_label() {
    buffer out{};
    BOOST_TEST(idna::to_ascii(out, "example.").error() == idna::error::empty_label);
    const idna::unicode_result shown = idna::to_unicode(out, "example.");
    BOOST_TEST_EQ(std::string_view(out.data(), shown.size), "example.");
    BOOST_TEST(!shown.error);
    const idna::options no_lengths{.verify_dns_length = false};
    const idna::result<std::size_t> kept = idna::to_ascii(out, "example.", no_lengths);
    BOOST_TEST_EQ(std::string_view(out.data(), kept.value()), "example.");
    BOOST_TEST(idna::to_unicode(out, ".example").error == idna::error::empty_label);
}

/** The inline storage holds a label after canonical decomposition: 255 a and an e-acute decompose
    to 257 code points, beyond it, while their NFC is 256. */
void a_label_that_decomposes_past_the_storage() {
    const std::string label = std::string(255, 'a') + "\303\251";
    const idna::options no_lengths{.verify_dns_length = false};
    std::array<char, 400> out{};
    BOOST_TEST(idna::to_ascii(out, label, no_lengths).error() == idna::error::storage_exhausted);
    const idna::result<std::pmr::string> owned = idna::to_ascii(label, no_lengths);
    BOOST_TEST(owned.has_value());
}

}  // namespace

int main() {
    idna_test_v2();
    idna_test_v2_strict();
    the_length_comes_before_the_buffer();
    to_unicode_stops_at_a_code_point();
    to_unicode_goes_on();
    the_defaults();
    ignore_invalid_punycode();
    beyond_the_inline_storage();
    separators();
    the_first_error_wins();
    control_characters();
    the_root_label();
    a_label_that_decomposes_past_the_storage();
    return boost::report_errors();
}
```

In `test/allocation_test.cpp`, replace the two includes of Task 9 with
`#include <webcpp/idna.hpp>`; in `exercise_span_forms`, before the line of
`is_valid_domain`:

```cpp
            succeeded += idna::to_ascii(out, input, opts).has_value() ? 1 : 0;
            succeeded += idna::to_unicode(out, input, opts).error ? 0 : 1;
```

and in `pmr_forms_allocate_through_their_resource`, before the line of
`punycode::decode`:

```cpp
        static_cast<void>(
            idna::to_ascii(input, idna::options{.verify_dns_length = false}, resource));
        static_cast<void>(idna::to_unicode(input, idna::options(), resource));
```

The file is then the whole test:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Tests what idna allocates, by counting every call of the global operator new this program makes:
// nothing in the span forms and the validators, whatever the input, and in the pmr forms only what
// their memory resource allocates.

#include <webcpp/idna.hpp>

#include <boost/config.hpp>
#include <boost/core/lightweight_test.hpp>

#include <array>
#include <cstddef>
#include <cstdlib>
#include <memory_resource>
#include <new>
#include <string>
#include <string_view>

#ifdef _MSC_VER
#include <malloc.h>
#endif

namespace idna = webcpp::idna;

namespace {

// The calls of the global operator new this program has made. It is mutable global state, which
// no library holds: it is this test program's own instrument, the one place where the replaced
// operator new below can record what it sees.
std::size_t global_allocations = 0;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

}  // namespace

// The replacements of the global operator new and operator delete, for this program alone, which
// count every allocation, an over-aligned one included. The standard library declares them with
// parameter names reserved to the implementation, which a program may not use, hence the exemption
// on each. None is ever inlined: GCC, seeing the malloc or the free inside one, would pair it with
// the other's call and warn of a mismatch (-Wmismatched-new-delete).

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
BOOST_NOINLINE void* operator new(std::size_t size) {
    ++global_allocations;
    void* memory = std::malloc(size == 0 ? 1 : size);
    if (memory == nullptr) {
        std::abort();
    }
    return memory;
}

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
BOOST_NOINLINE void operator delete(void* memory) noexcept {
    std::free(memory);
}

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
BOOST_NOINLINE void operator delete(void* memory, std::size_t /*size*/) noexcept {
    std::free(memory);
}

namespace {

/**
 Memory of `size` bytes aligned to `alignment`: from aligned_alloc, whose size must be a multiple
 of the alignment; or, under _MSC_VER, from _aligned_malloc.

 Tip: the UCRT of MSVC has no aligned_alloc, and memory from its _aligned_malloc is given back with
 _aligned_free, never free, hence the two below. libstdc++'s new_delete_resource asks for the
 alignment of what it allocates, 4 for char32_t, and macOS's aligned_alloc refuses an alignment
 below a pointer's, so the alignment is raised to that.
*/
void* aligned_memory(std::size_t size, std::size_t requested) {
    const std::size_t alignment = requested < sizeof(void*) ? sizeof(void*) : requested;
    const std::size_t rounded =
        size == 0 ? alignment : (size + alignment - 1) / alignment * alignment;
#ifdef _MSC_VER
    return _aligned_malloc(rounded, alignment);
#else
    return std::aligned_alloc(alignment, rounded);
#endif
}

/** Gives back what aligned_memory took. */
void free_aligned_memory(void* memory) noexcept {
#ifdef _MSC_VER
    _aligned_free(memory);
#else
    std::free(memory);
#endif
}

}  // namespace

/** The aligned allocation, from aligned_memory. */
// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
BOOST_NOINLINE void* operator new(std::size_t size, std::align_val_t alignment) {
    ++global_allocations;
    void* memory = aligned_memory(size, static_cast<std::size_t>(alignment));
    if (memory == nullptr) {
        std::abort();
    }
    return memory;
}

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
BOOST_NOINLINE void operator delete(void* memory, std::align_val_t /*alignment*/) noexcept {
    free_aligned_memory(memory);
}

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
BOOST_NOINLINE void operator delete(void* memory, std::size_t /*size*/,
                                    std::align_val_t /*alignment*/) noexcept {
    free_aligned_memory(memory);
}

namespace {

/** A memory resource that counts what it hands out, and takes it from the global operator new
    through std::pmr::new_delete_resource. */
class counting_resource final : public std::pmr::memory_resource {
public:
    /** The number of allocations made through it. */
    [[nodiscard]] std::size_t allocations() const noexcept { return allocations_; }

private:
    void* do_allocate(std::size_t size, std::size_t alignment) override {
        ++allocations_;
        return std::pmr::new_delete_resource()->allocate(size, alignment);
    }

    void do_deallocate(void* memory, std::size_t size, std::size_t alignment) override {
        std::pmr::new_delete_resource()->deallocate(memory, size, alignment);
    }

    [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }

    std::size_t allocations_ = 0;
};

/** Inputs that reach every path: long and many labels, every kind of failure, RTL labels, joiners,
    A-labels, and the NFC check's scratch copy. */
std::array<std::string, 14> hostile_inputs() {
    std::string many_labels;
    for (int i = 0; i < 500; ++i) {
        many_labels += "a.";
    }
    return {
        "b\303\274cher.example",
        "xn--bcher-kva.EXAMPLE",
        std::string(300, 'a'),
        many_labels,
        "\303\050",
        "xn--zz.xn--.xn---9uc.xn--ls8h",
        "0a.\327\220.\331\204\330\247",
        "a\342\200\214b.\340\244\225\340\245\215\342\200\215",
        "e\314\201.\303\251\314\201",
        "a\302\267l.\343\203\273",
        "\357\274\241\343\200\202\357\274\242",
        "ab--cd.-x-.a_b",
        std::string(70, 'a') + "\303\274",
        "",
    };
}

/**
 Every span form and validator over every input, with the strict profile, UTS 46 alone and no DNS
 lengths: the number of results that succeeded.

 Tip: the count is checked, so no optimizer may drop a call whose result nothing reads.
*/
std::size_t exercise_span_forms(const std::array<std::string, 14>& inputs) {
    std::array<char, 1024> out{};
    std::array<char32_t, 1024> code_points{};
    std::size_t succeeded = 0;
    for (const idna::options& opts : {
             idna::options(),
             idna::options{.idna2008 = false},
             idna::options{.verify_dns_length = false},
         }) {
        for (const std::string& input : inputs) {
            succeeded += idna::to_ascii(out, input, opts).has_value() ? 1 : 0;
            succeeded += idna::to_unicode(out, input, opts).error ? 0 : 1;
            succeeded += idna::is_valid_domain(input, opts) ? 1 : 0;
            succeeded += idna::is_valid_label(input, opts) ? 1 : 0;
            succeeded += idna::punycode::decode(code_points, input).has_value() ? 1 : 0;
        }
    }
    succeeded += idna::punycode::encode(out, U"b\u00FCcher").has_value() ? 1 : 0;
    return succeeded;
}

/** The span forms and the validators allocate nothing, whatever the input. */
void span_forms_allocate_nothing() {
    const std::array<std::string, 14> inputs = hostile_inputs();
    const std::size_t before = global_allocations;
    const std::size_t succeeded = exercise_span_forms(inputs);
    BOOST_TEST_EQ(global_allocations, before);
    BOOST_TEST(succeeded > 0);
}

/** The pmr forms allocate through their resource alone: every call of the global operator new is
    one of the resource's, for the result or a long label's storage. */
void pmr_forms_allocate_through_their_resource() {
    const std::array<std::string, 14> inputs = hostile_inputs();
    counting_resource resource;
    const std::size_t before = global_allocations;
    for (const std::string& input : inputs) {
        static_cast<void>(
            idna::to_ascii(input, idna::options{.verify_dns_length = false}, resource));
        static_cast<void>(idna::to_unicode(input, idna::options(), resource));
        static_cast<void>(idna::punycode::decode(input, resource));
    }
    static_cast<void>(
        idna::punycode::encode(U"b\u00FCcher-and-more-than-a-small-string", resource));
    BOOST_TEST_EQ(global_allocations - before, resource.allocations());
    BOOST_TEST(resource.allocations() > 0);
}

}  // namespace

int main() {
    span_forms_allocate_nothing();
    pmr_forms_allocate_through_their_resource();
    return boost::report_errors();
}
```

In `test/Jamfile`, before the line `webcpp.run allocation : allocation_test.cpp ;`:

```
webcpp.run uts46 : uts46_test.cpp : <toolset>msvc:<cxxflags>/bigobj ;
```

- [ ] **Step 2: Run them and watch them fail.**

```bash
cd ~/dev/webcpp-idna-uts46
$B2 libs/idna/test//uts46 libs/idna/test//allocation
```

Expected: FAIL, `'webcpp/idna/uts46.hpp' file not found` (and
`<webcpp/idna.hpp>` does not declare `to_ascii` yet).

- [ ] **Step 3: Write the conversions.** `include/webcpp/idna/uts46.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 UTS 46 processing with its mapping: ToASCII (section 4.2) and ToUnicode
 (section 4.3), each into a caller's buffer or into a string of a memory
 resource.

 @see "Converting names", in the guide.
*/
#ifndef WEBCPP_IDNA_UTS46_HPP
#define WEBCPP_IDNA_UTS46_HPP

#include <webcpp/idna/config.hpp>

#include <webcpp/idna/detail/label_buffer.hpp>
#include <webcpp/idna/detail/process.hpp>
#include <webcpp/idna/errors.hpp>
#include <webcpp/idna/options.hpp>

#include <boost/system/error_code.hpp>

#include <cstddef>
#include <memory_resource>
#include <span>
#include <string>
#include <string_view>

namespace webcpp::idna {

/**
 Converts a domain name to its ASCII form (UTS 46 section 4.2), into a
 caller's buffer.

 The name is mapped, normalized to NFC, split into labels, and each label is
 validated and, when it is not ASCII, written as "xn--" and its Punycode. With
 `opts.verify_dns_length`, the default, the result never exceeds
 @ref max_domain_length, so a `std::array<char, max_domain_length>` always
 holds it. Nothing allocates.

 @param output Where the ASCII form is written.
 @param domain The domain name, in UTF-8.
 @param opts The processing's flags; the defaults are the strict profile.
 @return The number of chars written, or the first @ref error found, a
 label's in their order, then @ref error::bidi, @ref error::domain_too_long,
 and @ref error::output_too_small last.
 @note After an error, the contents of `output` are unspecified.
 @see "Converting names", in the guide.
*/
[[nodiscard]] inline result<std::size_t> to_ascii(std::span<char> output, std::string_view domain,
                                                  const options& opts = options()) noexcept {
    detail::inline_label label;
    detail::span_sink sink(output);
    const boost::system::error_code found =
        detail::process(domain, opts, detail::mode::map, detail::form::ascii, label, sink);
    if (found) {
        return found;
    }
    if (sink.cut()) {
        return error::output_too_small;
    }
    return sink.written();
}

/**
 Converts a domain name to its ASCII form into a string of the given memory
 resource, which also holds a label beyond the inline working storage.

 @param domain The domain name, in UTF-8.
 @param opts The processing's flags.
 @param resource Where the string's memory, and a long label's, comes from.
 @return The ASCII form, or the first @ref error, as the span form orders them.
 @see "Allocation", in the guide.
*/
[[nodiscard]] inline result<std::pmr::string> to_ascii(
    std::string_view domain, const options& opts = options(),
    std::pmr::memory_resource& resource = *std::pmr::get_default_resource()) {
    detail::growing_label label(resource);
    std::pmr::string out(&resource);
    detail::string_sink sink(out);
    const boost::system::error_code found =
        detail::process(domain, opts, detail::mode::map, detail::form::ascii, label, sink);
    if (found) {
        return found;
    }
    return out;
}

/** What @ref to_unicode wrote into a caller's buffer, and the first error UTS 46 recorded. */
struct unicode_result {
    /** The number of chars written. */
    std::size_t size = 0;

    /** Empty when the processing succeeded; else its first @ref error, or
        @ref error::output_too_small when the buffer could not hold the whole result. */
    boost::system::error_code error;
};

/**
 Converts a domain name to its Unicode form (UTS 46 section 4.3), into a
 caller's buffer, as far as it can.

 As UTS 46 says, the conversion goes on after an error: each label is written
 as the processing leaves it, mapped and normalized, an A-label decoded when
 its Punycode decodes and kept as it is otherwise; no U+FFFD is inserted.
 Nothing allocates.

 @param output Where the Unicode form is written, in UTF-8; the writing stops
 at a code point boundary when it is full.
 @param domain The domain name, in UTF-8; ill-formed UTF-8 writes nothing.
 @param opts The processing's flags; the defaults are the strict profile.
 @return What was written, and the first error recorded.
 @see "Converting names", in the guide.
*/
[[nodiscard]] inline unicode_result to_unicode(std::span<char> output, std::string_view domain,
                                               const options& opts = options()) noexcept {
    detail::inline_label label;
    detail::span_sink sink(output);
    const boost::system::error_code found =
        detail::process(domain, opts, detail::mode::map, detail::form::unicode, label, sink);
    if (sink.cut()) {
        return {.size = sink.written(), .error = error::output_too_small};
    }
    return {.size = sink.written(), .error = found};
}

/** The string @ref to_unicode produced into a memory resource, and the first error UTS 46
    recorded. */
struct unicode_string {
    /** The Unicode form, in UTF-8. */
    std::pmr::string value;

    /** Empty when the processing succeeded; else its first @ref error. */
    boost::system::error_code error;
};

/**
 Converts a domain name to its Unicode form into a string of the given memory
 resource, as far as it can.

 @param domain The domain name, in UTF-8.
 @param opts The processing's flags.
 @param resource Where the string's memory, and a long label's, comes from.
 @return The Unicode form, and the first error recorded.
 @see "Allocation", in the guide.
*/
[[nodiscard]] inline unicode_string to_unicode(
    std::string_view domain, const options& opts = options(),
    std::pmr::memory_resource& resource = *std::pmr::get_default_resource()) {
    detail::growing_label label(resource);
    unicode_string out{.value = std::pmr::string(&resource), .error = {}};
    detail::string_sink sink(out.value);
    out.error =
        detail::process(domain, opts, detail::mode::map, detail::form::unicode, label, sink);
    return out;
}

}  // namespace webcpp::idna

#endif  // WEBCPP_IDNA_UTS46_HPP
```

`include/webcpp/idna.hpp` is now whole:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 Every public header of idna: Punycode, UTS 46's conversions, and IDNA2008's
 validity.

 @see "Introduction", in the guide.
*/
#ifndef WEBCPP_IDNA_HPP
#define WEBCPP_IDNA_HPP

#include <webcpp/idna/config.hpp>

#include <webcpp/idna/errors.hpp>
#include <webcpp/idna/options.hpp>
#include <webcpp/idna/punycode.hpp>
#include <webcpp/idna/uts46.hpp>
#include <webcpp/idna/validity.hpp>

#endif  // WEBCPP_IDNA_HPP
```

- [ ] **Step 4: Run them and watch them pass.**

```bash
$B2 libs/idna/test//uts46 libs/idna/test//allocation
```

Expected: two `**passed**`: every line of IdnaTestV2.txt holds in both
forms, under UTS 46 alone and under the strict profile, and no line is a
known failure.

- [ ] **Step 5: See the known-failures list fail both ways.** First a line
  that passes, listed as failing:

```bash
L=~/dev/webcpp/.superpowers/sdd/WEBCPP_IDNA_PLAN/uts46
cp libs/idna/test/known_failures.json $L/known_failures.keep
printf '{\n    "unicode": "17.0.0",\n    "failures": [\n        {"line": 107, "source": "fa\\u00df.de", "reason": "planted"}\n    ]\n}\n' > libs/idna/test/known_failures.json
python3 libs/idna/tools/generate_tables.py   # fills this worktree's bin/unicode/17.0.0/ first
$B2 libs/idna/test//uts46 > $L/planted-listed.log 2>&1; grep -c 'line 107: passes, but test/known_failures.json lists it' $L/planted-listed.log
cp $L/known_failures.keep libs/idna/test/known_failures.json
python3 libs/idna/tools/generate_tables.py
git -C libs/idna status --short
```

Expected: `1` (the UTS 46 pass reports it; the strict pass leaves a listed
line out); then the list and the generated files as they were (`git status`
names only this task's edits). Then lines that fail, listed as passing: in
`detail/process.hpp`'s `check_hyphens`, make the condition
`text.front() == U'-' || text.back() == U'-'` read `false`, run
`$B2 libs/idna/test//uts46 > $L/planted-failing.log 2>&1` and
`grep -c ': fails' $L/planted-failing.log`. Expected: 138 (measured on the
prototype, over both passes), the first `UTS 46 pass, IdnaTestV2.txt line
411: fails`. Restore the condition; `uts46` passes again.

- [ ] **Step 6: The testing chapter.** In `doc/testing.adoc`, before the
  tests table's closing `|===`:

```asciidoc

| `uts46`
| Every line of IdnaTestV2.txt, toUnicode, toAsciiN and toAsciiT, in the
  span and the pmr forms, under UTS 46 alone as the file assumes, then the
  strict profile over the lines whose toUnicode field holds no NV8, XV8 or
  CONTEXTO code point; and the orders of the errors.
```

- [ ] **Step 7: Every lane, the page and the lint** (procedure "The task's
  lanes"):

```bash
cd ~/dev/webcpp-idna-uts46
L=~/dev/webcpp/.superpowers/sdd/WEBCPP_IDNA_PLAN/uts46
export EM_CACHE=~/dev/webcpp/.local/emscripten-cache
EXAMPLES="$(test -f libs/idna/example/Jamfile && echo libs/idna/example)"
$B2 -a --build-dir=bin/idna-native libs/idna/test $EXAMPLES > $L/native.log 2>&1 &
$B2 -a --build-dir=bin/idna-gcc toolset=gcc-15 libs/idna/test $EXAMPLES > $L/gcc.log 2>&1 &
$B2 -a --build-dir=bin/idna-wasip2 toolset=clang-wasip2 testing.launcher=wasmtime libs/idna/test $EXAMPLES > $L/wasip2.log 2>&1 &
$B2 -a --build-dir=bin/idna-wasip3 toolset=clang-wasip3 testing.launcher=wasmtime libs/idna/test $EXAMPLES > $L/wasip3.log 2>&1 &
$B2 -a --build-dir=bin/idna-emscripten toolset=emscripten libs/idna/test $EXAMPLES > $L/emscripten.log 2>&1 &
wait
grep -L -e '\.\.\.failed' $L/{native,gcc,wasip2,wasip3,emscripten}.log
$B2 -a --build-dir=bin/idna-doc libs/idna/doc > $L/doc.log 2>&1; echo "doc $?"
tools/lint/lint.sh --clang-format .local/wasi-sdk/bin/clang-format --clang-tidy .local/wasi-sdk/bin/clang-tidy > $L/lint.log 2>&1; tail -1 $L/lint.log
```

Expected also: `uts46` and `allocation` passing on each lane.

- [ ] **Step 8: Commit.**

```bash
cd ~/dev/webcpp-idna-uts46/libs/idna
git add include/webcpp/idna/uts46.hpp include/webcpp/idna.hpp test/uts46_test.cpp \
    test/allocation_test.cpp test/Jamfile doc/testing.adoc
git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "idna: UTS 46 ToASCII and ToUnicode, conformant to IdnaTestV2"
git log --format='%an %ae %cn %ce' main..HEAD
```

Expected: the identity line alone.

- [ ] **Step 9: Report,** telling the formats track that `to_ascii` is
  ready, ending with:

```markdown
## Changelog

No change: the New Libraries line of idna stands as written.
```

### Task 12: The conversions on the page, the examples, the limits

The spec's increment I5, its page and measurements. Worktree
`~/dev/webcpp-idna-uts46-doc`, from `main` with Tasks 10 and 11 integrated.

**Files:**
- Create: `example/idna_quick_start.cpp`, `example/idna_quick_start.expected`, `example/idna_allocation.cpp`, `example/idna_allocation.expected`, `doc/limits.adoc`
- Modify: `example/Jamfile`, `doc/idna.adoc`, `doc/introduction.adoc`, `doc/converting.adoc`, `doc/allocation.adoc`, `doc/unicode.adoc`

**Interfaces:**
- Consumes: Task 11's `to_ascii`, `to_unicode`; Task 9's `check_domain`.
- Produces: the sections `[#idna-introduction-first]`, `[#idna-converting-processing]`, `[#idna-converting-to-ascii]`, `[#idna-converting-to-unicode]` and `[#idna-limits]`; the measurements of the stack and of the whole library's cost.

- [ ] **Step 1: The failing examples.** In `example/Jamfile`, after the line
  of `idna_validity.cpp`:

```
webcpp.example idna_quick_start.cpp ;
webcpp.example idna_allocation.cpp ;
```

`example/idna_quick_start.expected`:

```
Bücher.Example -> xn--bcher-kva.example
a·b.example -> error: code point outside its allowed context
bücher.example
```

`example/idna_allocation.expected`:

```
xn--bcher-kva.example
xn--bcher-kva.example
label beyond the working storage
300 octets
```

```bash
cd ~/dev/webcpp-idna-uts46-doc
$B2 libs/idna/example
```

Expected: FAIL, the two `.cpp` files not found.

- [ ] **Step 2: The programs.** `example/idna_quick_start.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// tag::program[]
#include <webcpp/idna.hpp>

#include <array>
#include <iostream>
#include <string_view>

namespace idna = webcpp::idna;

int main() {
    // tag::to_ascii[]
    std::array<char, idna::max_domain_length> ascii{};
    for (const std::string_view name : {"B\303\274cher.Example", "a\302\267b.example"}) {
        const idna::result<std::size_t> converted = idna::to_ascii(ascii, name);
        if (converted.has_error()) {
            std::cout << name << " -> error: " << converted.error().message() << '\n';
            continue;
        }
        std::cout << name << " -> " << std::string_view(ascii.data(), converted.value()) << '\n';
    }
    // end::to_ascii[]

    // tag::to_unicode[]
    std::array<char, 1024> unicode{};
    const idna::unicode_result shown = idna::to_unicode(unicode, "xn--bcher-kva.example");
    std::cout << std::string_view(unicode.data(), shown.size)
              << (shown.error ? " (with an error)" : "") << '\n';
    // end::to_unicode[]
}

// end::program[]
```

`example/idna_allocation.cpp` (its resource holds 16 KiB: the long label's
vector grows by doubling inside a monotonic resource, which frees nothing):

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <webcpp/idna/uts46.hpp>

#include <array>
#include <cstddef>
#include <iostream>
#include <memory_resource>
#include <string>
#include <string_view>

namespace idna = webcpp::idna;

int main() {
    // tag::span[]
    // A span form writes into the caller's buffer and allocates nothing.
    std::array<char, idna::max_domain_length> buffer{};
    const idna::result<std::size_t> written = idna::to_ascii(buffer, "b\303\274cher.example");
    std::cout << std::string_view(buffer.data(), written.value()) << '\n';
    // end::span[]

    // tag::pmr[]
    // A pmr form takes its memory from the resource it is given, here a buffer on the stack.
    std::array<std::byte, 16384> storage{};
    std::pmr::monotonic_buffer_resource resource(storage.data(), storage.size(),
                                                 std::pmr::null_memory_resource());
    const idna::result<std::pmr::string> owned =
        idna::to_ascii("b\303\274cher.example", idna::options(), resource);
    std::cout << owned.value() << '\n';
    // end::pmr[]

    // tag::long_label[]
    // Without the DNS lengths, a label of 300 code points is beyond the inline storage: the span
    // form refuses it, and the pmr form holds it in its resource.
    const std::string long_label(300, 'a');
    const idna::options no_lengths{.verify_dns_length = false};
    std::array<char, 512> wide{};
    std::cout << idna::to_ascii(wide, long_label, no_lengths).error().message() << '\n';
    std::cout << idna::to_ascii(long_label, no_lengths, resource).value().size() << " octets\n";
    // end::long_label[]
}
```

```bash
$B2 libs/idna/example
```

Expected: four `**passed**`.

- [ ] **Step 3: Measure the stack** (the spec's risk 6), with this program
  in `~/dev/webcpp/.superpowers/sdd/WEBCPP_IDNA_PLAN/uts46-doc/stack.cpp`,
  never committed, on wasip2 and wasip3 at `-O0` with a stack made smaller
  until the program traps:

```cpp
#include <webcpp/idna.hpp>
#include <array>
#include <cstdio>
namespace idna = webcpp::idna;
int main() {
    std::array<char, idna::max_domain_length> out{};
    int ok = 0;
    ok += idna::to_ascii(out, "xn--bcher-kva.b\303\274cher.example").has_value() ? 1 : 0;
    ok += idna::to_unicode(out, "xn--bcher-kva.example").size > 0 ? 1 : 0;
    ok += idna::check_domain("xn--bcher-kva.\303\251\314\201.example").has_value() ? 1 : 0;
    std::printf("%d\n", ok);
}
```

```bash
L=~/dev/webcpp/.superpowers/sdd/WEBCPP_IDNA_PLAN/uts46-doc
E='env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH'
for target in wasip2 wasip3; do for kb in 4 6 8 10 12 16; do
  $E .local/wasi-sdk/bin/clang++ --target=wasm32-$target -fwasm-exceptions -mllvm -wasm-use-legacy-eh=false \
    -std=c++20 -O0 -g -I libs/idna/include -I /opt/homebrew/opt/boost/include $L/stack.cpp \
    -o $L/stack.wasm -lunwind -Wl,-z,stack-size=$((kb*1024)) &&
  wasmtime run -W exceptions=y $L/stack.wasm > /dev/null 2>&1 && { echo "$target: $kb KiB"; break; }
done; done
```

Expected (measured on the prototype): `wasip2: 6 KiB` and `wasip3: 6 KiB`,
against wasm's default of 64 KiB. When yours differ, write them in
`doc/limits.adoc`'s paragraph "The stack" below.

- [ ] **Step 4: Measure the whole library's cost** (the spec's section 8.2:
  a translation unit that includes `<webcpp/idna.hpp>`, its compile time and
  object size, natively and on wasip2), with this program in `$L/cost.cpp`:

```cpp
#include <webcpp/idna.hpp>
#include <array>
#include <cstdio>
int main(int argc, char** argv) {
    std::array<char, webcpp::idna::max_domain_length> out{};
    const auto r = webcpp::idna::to_ascii(out, argc > 1 ? argv[1] : "b\303\274cher.example");
    if (!r) { std::puts(r.error().message().c_str()); return 1; }
    std::printf("%.*s\n", static_cast<int>(*r), out.data());
}
```

and `$L/base.cpp`, the same without idna:

```cpp
#include <array>
#include <cstdio>
int main(int argc, char** argv) {
    std::array<char, 253> out{};
    std::printf("%s %c\n", argc > 1 ? argv[1] : "x", out[0]);
}
```

```bash
for f in cost base; do
  /usr/bin/time -p $E clang++ -std=c++20 -O2 -I libs/idna/include -I /opt/homebrew/opt/boost/include -c $L/$f.cpp -o $L/$f.o
  /usr/bin/time -p $E .local/wasi-sdk/bin/clang++ --target=wasm32-wasip2 -std=c++20 -O2 -I libs/idna/include -I /opt/homebrew/opt/boost/include -c $L/$f.cpp -o $L/$f-wasip2.o
done
ls -l $L/*.o
```

Expected (measured on the prototype): `cost` 0.55 s and 182 KB natively,
0.74 s and 188 KB on wasip2; `base` 0.15 s and under 1 KB. Write yours in
the paragraph below when they differ by more than a fifth.

- [ ] **Step 5: The page.** In `doc/introduction.adoc`, before
  `[#idna-introduction-headers]`:

```asciidoc
[#idna-introduction-first]
=== A first example

This program converts two names to their ASCII form, and an A-label back to
its Unicode form, with the default options:

[source]
----
include::{examples}/idna_quick_start.cpp[tag=program,indent=0]
----

It prints:

[listing]
----
include::{examples}/idna_quick_start.expected[]
----

`Bücher.Example` is mapped first, its capitals lowered, so it is valid, and
its first label, which is not ASCII, becomes an A-label. `a·b.example` is
refused: U+00B7 MIDDLE DOT is valid in IDNA2008 only between two `l`, a
CONTEXTO rule that UTS 46 alone does not have. `to_unicode` decodes the
A-label back, and reports no error.

```

and the table of headers gains, after the row of `punycode.hpp`:

```asciidoc

| `<webcpp/idna/uts46.hpp>`
| `to_ascii` and `to_unicode` (<<idna-converting>>).
```

In `doc/converting.adoc`, after the line `== Converting names` and its blank
line, before `[#idna-converting-options]`:

```asciidoc
`idna::to_ascii` and `idna::to_unicode` are UTS 46's ToASCII and ToUnicode,
sections 4.2 and 4.3: the name is mapped, normalized to NFC, split into
labels at each full stop, and each label is converted and checked. The first
example (<<idna-introduction-first>>) calls both.

[#idna-converting-processing]
=== The processing

Each code point is looked up in UTS 46's mapping table: a code point that is
`valid` stays, one that is `mapped` is replaced by its mapping (`A` by `a`,
`ﬁ` by `fi`, U+3002 IDEOGRAPHIC FULL STOP by `.`), one that is `ignored` is
removed (U+00AD SOFT HYPHEN), a `deviation` stays unless the processing is
transitional (`ß`, `ς`, and the two joiners), and one that is `disallowed`
stays, to be refused when its label is checked. The result is normalized to
NFC and split at U+002E. A label that starts with `xn--` is decoded from
Punycode and checked as the U-label it holds; any other is checked as it is.
The checks, in their order, are those of <<idna-validity-rules>>.

idna does this one label at a time, in a working storage of one label's code
points, so a name of any length needs the same small storage
(<<idna-limits>>).

[#idna-converting-to-ascii]
=== To ASCII

`to_ascii(output, domain, opts)` writes the ASCII form of `domain` into the
caller's span and returns the number of chars written: each label that is not
ASCII as `xn--` and its Punycode, the others as they are, joined by `.`. A
name that fails any check has no ASCII form: the result is the first error
found, in the order of the labels, then `bidi` for the Bidi rule across the
labels, then `domain_too_long`, and `output_too_small` last, so a span too
small for a name that is too long anyway reports the name. With the DNS
lengths verified, the default, the ASCII form is at most
`idna::max_domain_length`, 253 chars, so a `std::array<char,
idna::max_domain_length>` always holds it.

[#idna-converting-to-unicode]
=== To Unicode

`to_unicode(output, domain, opts)` writes the Unicode form, in UTF-8, and
returns an `idna::unicode_result`: how many chars it wrote, and the first
error. As UTS 46 asks, the conversion goes on after an error, so that a name
can be shown even when it is not valid: each label is written as the
processing leaves it, mapped and normalized, an A-label decoded when its
Punycode decodes and kept as it is otherwise; no U+FFFD is put in place of
what failed. A final empty label, the DNS root of `example.`, is no error in
the Unicode form; any other empty label is. When the span is full, the
writing stops at a code point boundary and the error is `output_too_small`,
which takes the place of any other: the caller needs a larger span before
anything else. Input that is not well-formed UTF-8 writes nothing.

```

In `doc/allocation.adoc`, replace everything from the line `  its result, and
a label's working storage beyond the inline capacity.` to the end with:

```asciidoc
  its result, and a label's working storage beyond the inline capacity
  (<<idna-limits>>).

The span form of `to_ascii` needs no more than a buffer on the stack:

[source]
----
include::{examples}/idna_allocation.cpp[tag=span,indent=0]
----

Its pmr form, here with a `std::pmr::monotonic_buffer_resource` over a
buffer of the program's own, which allocates nothing from the heap either:

[source]
----
include::{examples}/idna_allocation.cpp[tag=pmr,indent=0]
----

The two forms differ for a label longer than the inline storage, which only
a program that turns the DNS lengths off can give: the span form refuses it,
and the pmr form holds it in its resource.

[source]
----
include::{examples}/idna_allocation.cpp[tag=long_label,indent=0]
----

The program prints:

[listing]
----
include::{examples}/idna_allocation.expected[]
----

The test `allocation` replaces the global `operator new` for itself alone
and counts: no call across every span form and validator, over long names,
long labels, every kind of failure and every option set, and in the pmr
forms, exactly the calls of their memory resource.
```

In `doc/unicode.adoc`, replace the subsection from `[#idna-unicode-compile]`
to the line before `[#idna-unicode-moving]` with:

```
[#idna-unicode-compile]
=== What the tables cost

Only `<webcpp/idna/detail/properties.hpp>` includes the tables, and only
`uts46.hpp` and `validity.hpp` include it: `punycode.hpp`, `errors.hpp` and
`options.hpp` cost nothing of them. A translation unit that includes
`<webcpp/idna.hpp>` and calls `to_ascii`, compiled at `-O2`, takes 0.55 s and
makes an object of 182 KB natively with Apple Clang 21, and 0.74 s and 188 KB
for wasm32-wasip2 with wasi-sdk 34, against 0.15 s and under 1 KB for the same
program without idna.

```

`doc/limits.adoc`:

```asciidoc
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

[#idna-limits]
== Limits

[#idna-limits-dns]
=== The DNS lengths

Under `verify_dns_length`, the default, a label is 1 to
`idna::max_label_length`, 63, octets in its ASCII form, and a name 1 to
`idna::max_domain_length`, 253, without a final root label, which is
refused: RFC 1034 section 3.1's limits, as UTS 46 section 4.2 states them. A
U-label is measured by its A-label: 55 `a` and a `ü` make an A-label of 63
octets, and 56 make one of 64.

[#idna-limits-storage]
=== The working storage

The conversions and the checks hold one label at a time, in a working
storage of 256 code points inside the call, about 1.3 KiB with the label's
ASCII form, and a second one while a label is checked for NFC the slow way.
Every label the DNS lengths accept fits: an A-label decodes to at most 59
code points, and a code point decomposes to at most 4, U+1FAF among them,
the longest canonical decomposition of Unicode {unicode-version}, which the generator
checks. A label held after canonical decomposition beyond 256 code points
occurs only with `verify_dns_length` off: the span forms and the validators
then return `storage_exhausted`, and the pmr forms take its storage from
their memory resource. With `verify_dns_length` on, such a label is
`label_too_long`, which it is, and so is a label whose NFC check's copy
outgrows the storage.

The storage fills while a label is gathered, before any of the label's
checks, so a label beyond it reports that first: U+FFFF followed by 300 `a`
is `label_too_long`, not `disallowed`. In `to_unicode`, the processing stops
there, keeping an error recorded before it, and its output ends with the last
label written before the long one.

[#idna-limits-stack]
=== The stack

The deepest call of idna, `to_ascii` on a label that needs the slow NFC
check, runs in 6 KiB of stack, measured on wasm32-wasip2 and wasm32-wasip3 in
a debug build (`-O0`) by shrinking the stack until it trapped, against the
64 KiB WebAssembly programs get by default. idna does not recurse.
```

In `doc/idna.adoc`, after `include::allocation.adoc[]`, the line
`include::limits.adoc[]` between blank lines.

- [ ] **Step 6: Every lane, the page and the lint** (procedure "The task's
  lanes"):

```bash
cd ~/dev/webcpp-idna-uts46-doc
L=~/dev/webcpp/.superpowers/sdd/WEBCPP_IDNA_PLAN/uts46-doc
export EM_CACHE=~/dev/webcpp/.local/emscripten-cache
EXAMPLES="$(test -f libs/idna/example/Jamfile && echo libs/idna/example)"
$B2 -a --build-dir=bin/idna-native libs/idna/test $EXAMPLES > $L/native.log 2>&1 &
$B2 -a --build-dir=bin/idna-gcc toolset=gcc-15 libs/idna/test $EXAMPLES > $L/gcc.log 2>&1 &
$B2 -a --build-dir=bin/idna-wasip2 toolset=clang-wasip2 testing.launcher=wasmtime libs/idna/test $EXAMPLES > $L/wasip2.log 2>&1 &
$B2 -a --build-dir=bin/idna-wasip3 toolset=clang-wasip3 testing.launcher=wasmtime libs/idna/test $EXAMPLES > $L/wasip3.log 2>&1 &
$B2 -a --build-dir=bin/idna-emscripten toolset=emscripten libs/idna/test $EXAMPLES > $L/emscripten.log 2>&1 &
wait
grep -L -e '\.\.\.failed' $L/{native,gcc,wasip2,wasip3,emscripten}.log
$B2 -a --build-dir=bin/idna-doc libs/idna/doc > $L/doc.log 2>&1; echo "doc $?"
tools/lint/lint.sh --clang-format .local/wasi-sdk/bin/clang-format --clang-tidy .local/wasi-sdk/bin/clang-tidy > $L/lint.log 2>&1; tail -1 $L/lint.log
```

Expected also: the four examples passing on each lane.

- [ ] **Step 7: Commit.**

```bash
cd ~/dev/webcpp-idna-uts46-doc/libs/idna
git add example/Jamfile example/idna_quick_start.cpp example/idna_quick_start.expected \
    example/idna_allocation.cpp example/idna_allocation.expected doc/limits.adoc \
    doc/idna.adoc doc/introduction.adoc doc/converting.adoc doc/allocation.adoc doc/unicode.adoc
git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "idna: the conversions on the page, their examples, and the limits"
git log --format='%an %ae %cn %ce' main..HEAD
```

Expected: the identity line alone.

- [ ] **Step 8: Report,** with both measurements, ending with:

```markdown
## Changelog

No change: the New Libraries line of idna stands as written.
```

### Task 13: The page complete, the README, and readiness

The spec's increment I6. Worktree `~/dev/webcpp-idna-readiness`, from `main`
with Tasks 6 and 12 integrated. The superproject's `AGENTS.md` and
`README.md` gain idna's lines in this worktree, as a patch; the controller
applies and commits it in the integration space (Step 9), staging only
those two files.

**Files:**
- Create: `doc/guarantees.adoc`, `doc/counts.py`
- Modify (whole): `doc/testing.adoc`, `doc/idna.adoc`, `README.md`
- Modify: `doc/allocation.adoc` (its last sentence cites guarantee 1)
- Modify (superproject, as a patch): `AGENTS.md` (chapter 1's table and layout), `README.md` (the libraries' table)

**Interfaces:**
- Consumes: every earlier task; the doc build's counts (`{n-tests-native}`, `{n-headers}`, `{n-examples}`, `{n-tests-<target>}`, `{n-examples-<target>}`) and a library's own `doc/counts.py`.
- Produces: the attributes `{n-idna-test-v2}` and `{n-nfc-vectors}`; the anchors `idna-invariant-1` to `idna-invariant-5`, which keep their numbers forever.

- [ ] **Step 1: The failing check.** In `doc/testing.adoc`, write the row of
  `uts46` with the count, `All {n-idna-test-v2} lines of IdnaTestV2.txt`,
  and build the page:

```bash
cd ~/dev/webcpp-idna-readiness
L=~/dev/webcpp/.superpowers/sdd/WEBCPP_IDNA_PLAN/readiness
$B2 -a --build-dir=bin/idna-doc libs/idna/doc > $L/doc-red.log 2>&1; echo "doc $?"
```

Expected: a non-zero status, Asciidoctor's warning that the attribute
`n-idna-test-v2` is missing (every Asciidoctor message fails the page).

- [ ] **Step 2: The counts.** `doc/counts.py`:

```python
#!/usr/bin/env python3
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Counts what idna's page says idna holds and webcpp's own counts do not.

tools/doc/counts.py runs this script with idna's directory as its one argument, and gives the page
each `name=<number>` line it prints as an attribute, beside the counts of every library:

* `n-idna-test-v2`, the lines of IdnaTestV2.txt that test/generated/idna_test_v2.ipp holds, one
  row each;
* `n-nfc-vectors`, the lines of NormalizationTest.txt that test/generated/nfc_test.ipp holds.

A count that finds nothing fails, rather than put a zero on the page. A fault is written on the
standard error, and the standard output holds only counts.

Usage: counts.py <library-directory>. Exit 0 printing the counts, 1 naming a fault, 2 on a usage
error.
"""

from __future__ import annotations

import sys
from pathlib import Path

# Each count, the generated file it reads, and how each of its rows starts.
ROWS = {
    'n-idna-test-v2': ('test/generated/idna_test_v2.ipp', '    idna_vector{'),
    'n-nfc-vectors': ('test/generated/nfc_test.ipp', '    nfc_vector{'),
}


class Fault(Exception):
    """A count that cannot be made, said to the user."""


def count(library: Path, relative: str, start: str) -> int:
    """The rows of a generated file: its lines that start with `start`."""
    path = library / relative
    rows = sum(1 for line in path.read_text(encoding='utf-8').splitlines()
               if line.startswith(start))
    if rows == 0:
        raise Fault(f'counts.py: no row starting with {start.strip()!r} in {path}')
    return rows


def main(argv: list[str]) -> int:
    if len(argv) != 1 or not Path(argv[0]).is_dir():
        print('usage: counts.py <library-directory>', file=sys.stderr)
        return 2
    library = Path(argv[0])
    try:
        counted = {name: count(library, relative, start)
                   for name, (relative, start) in ROWS.items()}
    except (Fault, OSError) as fault:
        print(fault, file=sys.stderr)
        return 1
    for name, value in counted.items():
        print(f'{name}={value}')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
```

```bash
python3 libs/idna/doc/counts.py libs/idna
```

Expected: `n-idna-test-v2=6391` and `n-nfc-vectors=20034`.

- [ ] **Step 3: The guarantees.** `doc/guarantees.adoc` (guarantee 4 says
  "returns": a failed call into a larger span may have written more, and its
  contents are unspecified):

```asciidoc
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

[#idna-guarantees]
== Guarantees

Each guarantee keeps its number and its anchor; code, tests and comments cite
them.

. [[idna-invariant-1]]No span form of a conversion and no validator
  allocates, whatever its input; a pmr form allocates from its memory
  resource alone.
. [[idna-invariant-2]]Nothing throws but the allocation of a memory resource
  a pmr form was given.
. [[idna-invariant-3]]The same input and options give the same output and
  the same error on every target.
. [[idna-invariant-4]]With `verify_dns_length`, `to_ascii` returns at most
  `idna::max_domain_length` chars, and reports a longer name as
  `domain_too_long`, never as `output_too_small` in a buffer of that size;
  after an error, a span form's buffer holds nothing to read.
. [[idna-invariant-5]]The tables are exactly what `tools/generate_tables.py`
  writes from the pinned Unicode {unicode-version} files.
```

In `doc/allocation.adoc`, its last sentence ends "exactly the calls of their
memory resource (<<idna-invariant-1>>)." in place of "... memory resource.".

- [ ] **Step 4: The testing chapter, whole.** `doc/testing.adoc`:

```asciidoc
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

[#idna-testing]
== Testing

[#idna-testing-library]
=== How idna is tested

idna's tests are lightweight_test programs, one per subject, under `test/`,
each declared in `test/Jamfile` with webcpp's `webcpp.run`. Every vector of
the conformance files is compiled into its test, so no test reads a file,
and every test runs on every target.

[cols="1,3",options="header"]
|===
| Test | What it checks

| `errors`
| The values of `idna::error`, which are fixed, their messages, the
  category's name and identity, and the message cut to a caller's buffer.

| `options`
| The strict profile's defaults, the designated initializers of the
  aggregate, and the DNS lengths.

| `utf8`
| Strict UTF-8: the boundaries of every row of RFC 3629's table of
  well-formed sequences, and every kind of ill-formed one.

| `punycode`
| RFC 3492 section 7.1's samples, (A) to (S), both ways; digits in either
  case and the mixed-case annotation kept; a leading delimiter, a bad digit,
  a number cut short, a value out of Unicode, an overflow;
  `output_too_small` at every length below the result's; the pmr forms.

| `properties`
| One value of every generated table, through its lookup.

| `vectors`
| The generated vectors: every line of both files, the blank fields of
  IdnaTestV2.txt resolved, its lone surrogate kept ill-formed, the lines of
  the strict pass.

| `nfc`
| NFC within a label's storage against all {n-nfc-vectors} lines of
  NormalizationTest.txt, its quick check and its slow path, Hangul, and the
  storage, inline and growing.

| `context`, `bidi`
| Each rule of RFC 5892 appendix A and of RFC 5893 section 2, satisfied and
  broken, the joining scripts a hand-written list misses among them.

| `process`
| The label driver in both forms on what ends a label early: an A-label that
  decodes to nothing, and a label beyond the storage after an earlier error.
  Like every test, built with the standard library's own checks on, so that
  a read past an empty label traps.

| `validity`
| `check_label` and `check_domain`: every rule with its error; the strings
  of the JSON Schema suite's `idn-hostname` that UTS 46 alone accepts, as
  U-labels and as A-labels; A-labels in either case and not canonical; the
  lengths at 63 and 64, 253 and 254; the inline storage's limit.

| `uts46`
| All {n-idna-test-v2} lines of IdnaTestV2.txt, toUnicode, toAsciiN and
  toAsciiT, in the span and the pmr forms, under UTS 46 alone as the file
  assumes, then the strict profile over the lines whose toUnicode field
  holds no NV8, XV8 or CONTEXTO code point; and the orders of the errors.

| `allocation`
| The global `operator new`, replaced for this program alone (<<idna-invariant-1>>).
|===

Beside them, `test/Jamfile` compiles each of the {n-headers} headers alone,
and `config.hpp` twice, as the build asks and with
`WEBCPP_IDNA_NO_EXCEPTIONS` defined by the developer. `example/Jamfile`
builds every program this page shows, runs it, and compares what it printed
with the `.expected` beside it.

[#idna-testing-conformance]
=== Conformance, and known failures

IdnaTestV2.txt is UTS 46's own conformance file. idna passes every line of
it, which `test/known_failures.json` records as an empty list: a line listed
there must fail in the pass under UTS 46 alone, which the strict pass then
leaves out, and every other line must pass, so the list never lies. The one
kind of entry it may hold is a line that fails because of a difference
between two versions of Unicode, the file's and the tables', with a reason
that names both. The lines with a lone surrogate, which no UTF-8 string
holds, are refused as `invalid_utf8`, as the file allows. JSON Schema's own
tests of `hostname` and `idn-hostname` run in `webcpp::formats`, against the
formats it builds on idna.

[#idna-testing-targets]
=== Targets and exceptions

`test/Jamfile` and `example/Jamfile` declare `webcpp.targets native wasip2
wasip3 emscripten`, and b2 builds each program for those alone: natively
with any toolset, for wasm32-wasip2 and wasm32-wasip3 with the toolsets
`clang-wasip2` and `clang-wasip3` run by wasmtime, and with Emscripten run
by Node.

`<webcpp/idna/config.hpp>`, which every public header includes first,
defines `WEBCPP_IDNA_NO_EXCEPTIONS` when the build has no exceptions, and a
program may define it itself, as Boost.Asio's `BOOST_ASIO_NO_EXCEPTIONS`.
idna raises no exception of its own, so the macro changes nothing in it: it
exists so that a program can set it for every webcpp library alike. The
lint compiles every header, test and example a second time without
exceptions.

From the root of a webcpp checkout:

`b2 libs/idna/test`::
The {n-tests-native} tests, natively.

`b2 libs/idna/example`::
The {n-examples} examples, each built, run, and its output compared.

`b2 libs/idna/test libs/idna/example toolset=clang-wasip2 testing.launcher=wasmtime`::
The same for wasm32-wasip2: {n-tests-wasip2} tests and {n-examples-wasip2}
examples. `toolset=clang-wasip3` does the same for wasm32-wasip3.

`b2 libs/idna/test libs/idna/example toolset=emscripten`::
The same with Emscripten, whose programs b2 runs with Node itself:
{n-tests-emscripten} tests and {n-examples-emscripten} examples.

`b2 -a libs/idna/test//tables`::
The own lane `tables`: the generator again, its outputs compared with the
committed files (<<idna-unicode-generator>>).

The test matrix of every lane is at
https://webcpporg.github.io/webcpp/report/.
```

- [ ] **Step 5: The page's main file, whole,** with every section in its
  place and the attribute `{unicode-version}`. `doc/idna.adoc`:

```asciidoc
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

= idna: internationalized domain names for {cpp}
:toc: left
:toclevels: 1
:idprefix:
:sectanchors:
:example-caption!:
:attribute-missing: warn
:unicode-version: 17.0.0

idna is a header-only library of internationalized domain names in {cpp}20,
one of the libraries of https://github.com/webcpporg/webcpp[webcpp]:
Punycode (RFC 3492), Unicode's UTS 46 processing, to ASCII and to Unicode,
and the validity rules of IDNA2008 (RFC 5890 to 5893), over Unicode
{unicode-version} tables generated into its headers. Every operation returns
its error as a value, and every operation that writes into a caller's buffer
allocates nothing.

This page teaches idna concept by concept: why each exists, its smallest
example, its variations and its rules. Every program it shows was compiled
and run, natively and for wasm32-wasip2, wasm32-wasip3 and Emscripten, and
printed the output shown under it. The <<reference,reference>> that closes
it lists every name the headers declare.

include::introduction.adoc[]

include::punycode.adoc[]

include::converting.adoc[]

include::validity.adoc[]

include::errors.adoc[]

include::allocation.adoc[]

include::limits.adoc[]

include::unicode.adoc[]

include::guarantees.adoc[]

include::testing.adoc[]

[#reference]
== Reference

Every name idna declares, in the namespaces `webcpp::idna` and
`webcpp::idna::punycode`, from the Doc Comments of its headers: each
function, type, enumerator and constant with its declaration, what it does,
what it asks of its caller and how it fails. The other sections teach the
library; this one is where a name is looked up.

include::{reference}[leveloffset=+1]

include::unicode-data.adoc[]
```

- [ ] **Step 6: The README, whole.** Its C++ block is the region `program` of
  `example/idna_quick_start.cpp`, which doc-check compares at every doc
  build. `README.md`:

````markdown
# idna

idna is a header-only library of internationalized domain names for C++20,
one of the libraries of [webcpp](https://github.com/webcpporg/webcpp):
Punycode (RFC 3492), Unicode's UTS 46 processing, ToASCII and ToUnicode,
and the validity rules of IDNA2008 (RFC 5890 to 5893), over Unicode 17.0.0
tables generated into its headers.

Its default profile is UTS 46's mapping followed by IDNA2008's validity, the
strict profile JSON Schema's `idn-hostname` format asks for; `idna::options`
turns each check on or off. Every operation returns its error as a value,
the forms that write into a caller's buffer allocate nothing, and the forms
that return a string take a `std::pmr::memory_resource`.

`<webcpp/idna.hpp>` includes every public header; the names are in the
namespace `webcpp::idna`. idna needs Boost's headers and nothing to build or
link. It runs natively, on wasm32-wasip2 and wasm32-wasip3, and with
Emscripten.

## A minimal example

This is [example/idna_quick_start.cpp](example/idna_quick_start.cpp), which
converts two names to their ASCII form and an A-label back to Unicode:

<!-- include::example/idna_quick_start.cpp[tag=program] -->
```cpp
#include <webcpp/idna.hpp>

#include <array>
#include <iostream>
#include <string_view>

namespace idna = webcpp::idna;

int main() {
    std::array<char, idna::max_domain_length> ascii{};
    for (const std::string_view name : {"B\303\274cher.Example", "a\302\267b.example"}) {
        const idna::result<std::size_t> converted = idna::to_ascii(ascii, name);
        if (converted.has_error()) {
            std::cout << name << " -> error: " << converted.error().message() << '\n';
            continue;
        }
        std::cout << name << " -> " << std::string_view(ascii.data(), converted.value()) << '\n';
    }

    std::array<char, 1024> unicode{};
    const idna::unicode_result shown = idna::to_unicode(unicode, "xn--bcher-kva.example");
    std::cout << std::string_view(unicode.data(), shown.size)
              << (shown.error ? " (with an error)" : "") << '\n';
}
```

It prints:

<!-- include::example/idna_quick_start.expected[] -->
```
Bücher.Example -> xn--bcher-kva.example
a·b.example -> error: code point outside its allowed context
bücher.example
```

## Building and testing

idna is developed inside the webcpp superproject, as a Boost library is
developed inside Boost:

    git clone --recursive https://github.com/webcpporg/webcpp
    cd webcpp
    b2 libs/idna/test libs/idna/example

builds the tests and the examples natively, runs every example and compares
its output with the `.expected` file beside it.
`toolset=clang-wasip2 testing.launcher=wasmtime`,
`toolset=clang-wasip3 testing.launcher=wasmtime` and `toolset=emscripten`
do the same for WebAssembly. A b2 project uses idna through
`/webcpp/idna//idna`.

## Documentation

idna's page, with its API reference, is published at
<https://webcpporg.github.io/webcpp/libs/idna/>; `b2 libs/idna/doc` builds
it into `doc/html/index.html`.

## Unicode data

idna's tables and the vectors of its tests are derived from Unicode's data
files, under the Unicode License v3, whose notice is the appendix "Unicode
data" of idna's page. idna's code is under the Boost Software License alone.

## License

Distributed under the [Boost Software License, Version 1.0](LICENSE_1_0.txt).
````

- [ ] **Step 7: The page builds, and reads well.**

```bash
$B2 -a --build-dir=bin/idna-doc libs/idna/doc > $L/doc.log 2>&1; echo "doc $?"
$B2 -a --build-dir=bin/idna-index doc > $L/index.log 2>&1; echo "index $?"
swift /tmp/webcpp-doc-shots/snap.swift libs/idna/doc/html/index.html 1280 $L/page-1280.png
swift /tmp/webcpp-doc-shots/snap.swift libs/idna/doc/html/index.html 320 $L/page-320.png
```

Expected: `doc 0`, `index 0` (the index lists idna from
`meta/libraries.json` and links its page); each snapshot prints
`scrollWidth=<w> innerWidth=<w>` with the two equal (no horizontal
overflow; measured on the prototype at both widths). The script takes the
page, the width and the PNG to write; when `/tmp/webcpp-doc-shots/snap.swift`
is gone, a browser at those widths does. Look at the introduction, the
options table, the errors table and the reference at 320 px: no text out of
the page outside a table's or a listing's own scroll box, no broken code
span; fix what does not read well.

- [ ] **Step 8: Every lane, the own lane, the page, the lint, the CI's plan and
  the scans** (procedure "The task's lanes", then the rest):

```bash
cd ~/dev/webcpp-idna-readiness
L=~/dev/webcpp/.superpowers/sdd/WEBCPP_IDNA_PLAN/readiness
export EM_CACHE=~/dev/webcpp/.local/emscripten-cache
EXAMPLES="$(test -f libs/idna/example/Jamfile && echo libs/idna/example)"
$B2 -a --build-dir=bin/idna-native libs/idna/test $EXAMPLES > $L/native.log 2>&1 &
$B2 -a --build-dir=bin/idna-gcc toolset=gcc-15 libs/idna/test $EXAMPLES > $L/gcc.log 2>&1 &
$B2 -a --build-dir=bin/idna-wasip2 toolset=clang-wasip2 testing.launcher=wasmtime libs/idna/test $EXAMPLES > $L/wasip2.log 2>&1 &
$B2 -a --build-dir=bin/idna-wasip3 toolset=clang-wasip3 testing.launcher=wasmtime libs/idna/test $EXAMPLES > $L/wasip3.log 2>&1 &
$B2 -a --build-dir=bin/idna-emscripten toolset=emscripten libs/idna/test $EXAMPLES > $L/emscripten.log 2>&1 &
wait
grep -L -e '\.\.\.failed' $L/{native,gcc,wasip2,wasip3,emscripten}.log
$B2 -a --build-dir=bin/idna-doc libs/idna/doc > $L/doc.log 2>&1; echo "doc $?"
tools/lint/lint.sh --clang-format .local/wasi-sdk/bin/clang-format --clang-tidy .local/wasi-sdk/bin/clang-tidy > $L/lint.log 2>&1; tail -1 $L/lint.log
```

```bash
$B2 -a --build-dir=bin/idna-lane libs/idna/test//tables > $L/tables.log 2>&1; echo "tables $?"
python3 tools/ci/matrix.py plan --library idna > $L/plan.json; echo "plan $?"
python3 tools/ci/matrix.py own-lanes --library idna > $L/own-lanes.json; echo "own-lanes $?"
git -C libs/idna grep -I -i -e 'veru[s]'; git -C libs/idna log --format='%an %ae %cn %ce %B' | grep -i -e 'veru[s]' -e 'co-authored' -e 'signed-off'
git -C libs/idna ls-files -z | xargs -0 python3 -c 'import sys; [print(p) for p in sys.argv[1:] if chr(0x2014) in open(p, encoding="utf-8", errors="ignore").read()]'; echo "scanned"
```

Expected: `tables 0`; the plan names idna's lanes on the native compilers,
wasip2, wasip3 and emscripten, and `own-lanes` names `tables` for idna; the
scans print nothing, then `scanned`. Run `actionlint` 1.7.12 on
`libs/idna/.github/workflows/ci.yml` when it is installed; the CI's
actionlint job runs it otherwise.

- [ ] **Step 9: The superproject's lists, as a patch for the controller.** In
  this worktree's `AGENTS.md`, chapter 1's table gains, after pratt's row:

```markdown
| idna | internationalized domain names: Punycode, UTS 46 and IDNA2008's validity over generated Unicode 17.0 tables, webcpp's own | native, wasip2, wasip3, emscripten; its own lane `tables` | a submodule at `libs/idna`; the first library with generated tables (its `tools/`) |
```

and the layout's line `libs/<name>/ ... (xactor, xstate, pratt, wasi)` names
idna after pratt. In this worktree's `README.md`, the table of libraries
gains, after pratt's row:

```markdown
| [idna](https://github.com/webcpporg/idna) | Header-only internationalized domain names: Punycode, UTS 46's ToASCII and ToUnicode, and IDNA2008's validity, over Unicode 17.0 tables generated into its headers, with no allocation in its span forms; a library of webcpp's own. | native, wasip2, wasip3, emscripten |
```

```bash
git -C ~/dev/webcpp-idna-readiness diff -- AGENTS.md README.md > $L/superproject.patch
git -C ~/dev/webcpp-idna-readiness checkout -- AGENTS.md README.md
```

Expected: the patch holds those three changed lines alone. The controller,
at the integration of this task, applies it in the integration space:

```bash
cd ~/dev/webcpp-formats
git diff --cached --quiet && git diff --quiet -- AGENTS.md README.md && echo clean
git apply ~/dev/webcpp/.superpowers/sdd/WEBCPP_IDNA_PLAN/readiness/superproject.patch
git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "AGENTS.md and README: idna, a library of webcpp's own" -- AGENTS.md README.md
git log -1 --format='%an %ae %cn %ce'
```

Expected: `clean` before the patch (nothing staged, no other hunk in the two
files); the identity line after it.

- [ ] **Step 10: Commit** (idna's repository):

```bash
cd ~/dev/webcpp-idna-readiness/libs/idna
git add doc/guarantees.adoc doc/counts.py doc/testing.adoc doc/idna.adoc doc/allocation.adoc README.md
git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "idna: the page complete, its guarantees, and the README"
git log --format='%an %ae %cn %ce' main..HEAD
```

Expected: the identity line alone.

- [ ] **Step 11: Report,** with the snapshots' paths, the CI plan's lanes and
  the patch's path, ending with:

```markdown
## Changelog

### New Libraries

- idna: IDNA2008 and UTS 46 for C++20, with Punycode and generated Unicode
  17.0 tables, from R.
```

### Task 14: The whole-branch review and publication (controller)

- [ ] **Step 1: The whole review,** on the most capable model:
  `~/dev/webcpp-formats/libs/idna` in full against the spec and this plan,
  the superproject commit of Task 13, every lane from scratch in the
  integration space, the own lane, the page, the lint, the CI plan, and
  `git -C ~/dev/webcpp-formats/libs/idna log --format='%an %ae %cn %ce' | sort -u`
  (the identity line alone). Run JSON Schema's `idn-hostname` file through
  `to_ascii` with the default options (formats runs it as its test; the
  prototype passed 92 of 92 of draft2020-12) and record the result.
- [ ] **Step 2: Rebase** `lib/formats` on `main` once the infra track has
  integrated S1:
  `git -c user.name=R -c user.email=github.overboard312@passmail.net -C ~/dev/webcpp-formats rebase main`,
  then `git -C ~/dev/webcpp-formats log --format='%an %ae %cn %ce' main..lib/formats | sort -u`
  (the identity line alone), and Step 1's lanes and the own lane again.
- [ ] **Step 3: Merge the changelog line** of Task 1's report into
  `~/.buzz/PLANS/WEBCPP_CHANGELOG_PENDING.md`, under "New Libraries", in
  alphabetical order (before pratt).
- [ ] **Step 4: Ask the owner** before anything leaves the machine: P8, the
  Unicode data notice (the provisional ruling, or a `LICENSE-UNICODE.txt`);
  creating `webcpporg/idna`, pushing `libs/idna`, adding the submodule; and
  when `lib/formats` (or idna's part of it) reaches `main`.
- [ ] **Step 5: Publish,** once the owner says yes: create the repository;
  `git -C ~/dev/webcpp-formats/libs/idna remote add origin <url>` (the URL the
  owner's new repository gives) and push `main`; in `~/dev/webcpp-formats`,
  `git submodule add ../idna.git libs/idna` (git adopts the existing
  repository) and
  `git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "libs: idna, a library of webcpp's own" -- .gitmodules libs/idna`;
  integrate into `main` in the order the owner chose; watch CI on both
  repositories until green, re-running idna's first run after the
  superproject is pushed.
- [ ] **Step 6:** Record the library as complete in the ledger; every idna
  task worktree is already removed (procedure "Integrating a task").

## Review disposition

The review `~/dev/webcpp/.superpowers/sdd/plan-reviews/WEBCPP_IDNA_PLAN-review.md`
(3 Critical, 9 Important, 13 Minor), with the controller's rulings of
2026-10-09 in `~/.buzz/PLANS/WEBCPP_DESIGN.md`. Every fix was run on the
prototype, and the b2 lanes, the lint and the page in a scratch superproject.

- **C1, fixed.** The test is `properties` (`test/properties_test.cpp`), the own lane keeps `tables`. Measured in the scratch superproject: before, `No best alternative for libs/idna/test/tables` and no `properties_test` in the ordinary lane; after, the lane resolves and the test is in every lane. Task 3 Step 8 and Task 6 Step 3 check it.
- **C2, fixed.** A Global Constraint: every commit-writing git command runs with `-c user.name=R -c user.email=github.overboard312@passmail.net`, in every commit, `rebase` and `rebase --continue` block; each task's commit, each integration and each rebase of `lib/formats` checks `git log --format='%an %ae %cn %ce'` of its range.
- **C3, fixed.** `ace()` returns at an empty decoded label before any check; `validate`, `check_hyphens` and `bidi::check` state that their label is not empty. Red first: every test builds with `_LIBCPP_HARDENING_MODE_EXTENSIVE` and `_GLIBCXX_ASSERTIONS` (`test/Jamfile`), `process_test` and `uts46_test` run `xn--` in the Unicode form, and Task 9 Step 6 plants the old guard and sees the trap (measured, exit status 133).
- **I1, fixed.** `vectors_test.cpp` names "a, then the lone surrogate U+D900, then z"; `to_ascii`'s `@return` has no colon after `@ref`. `rules.py doc-comments` is clean (measured), and the whole lint ends `lint: clean`.
- **I2, fixed.** Task 12's allocation text cites no guarantee; Task 13 adds `(<<idna-invariant-1>>)` with `guarantees.adoc`.
- **I3, fixed.** The NFC check's exhausted copy reports `label_too_long` under `verify_dns_length` in the ASCII form (`beyond_storage()`), and exhaustion keeps an error recorded before it; pinned in `validity_test`, `uts46_test` and `process_test`, and (3), exhaustion while a label is gathered, is written in `doc/limits.adoc` with `to_unicode`'s output ending at the last label written.
- **I4, fixed by the controller's ruling.** Every task runs in a worktree of its own; Task 6 is parallel only with tasks in other worktrees.
- **I5, fixed by the same ruling.** Only the controller writes in `~/dev/webcpp-formats`, one integration at a time, after the task branch is rebased in its own worktree (procedure "Integrating a task").
- **I6, fixed by the same ruling.** A task's worktree holds no `libs/formats`, so its lint and builds read idna alone; the integration space runs one verdict at a time; Task 13's superproject lines travel as a patch the controller applies and commits with explicit paths after checking nothing else is staged.
- **I7, fixed.** Logs under `~/dev/webcpp/.superpowers/sdd/WEBCPP_IDNA_PLAN/<slug>/`, never `/tmp`.
- **I8, fixed.** Planted defects: the generator's three self-checks (Task 3 Step 6), a wrong known-failures entry (Task 5 Step 5), and the known-failures list both ways (Task 11 Step 5); each message and count measured on the prototype.
- **I9, fixed.** Tasks 3 and 4 (tables, then their page and cost), 9 and 10 (driver and validators, then the example), 11 and 12 (conversions, then examples, limits and measurements). Differs from the review in one place: the "Validity" section's heading and its rules stay with the code of Task 9, since `validity.hpp`'s `@see "Validity"` needs that section for the strict reference to build.
- **M1, fixed.** Task 0 Step 4 records the provisional P8 ruling; Task 14 Step 4 puts P8 to the owner.
- **M2, fixed.** Task 12 Step 4 measures a translation unit of `<webcpp/idna.hpp>` natively and on wasip2 (0.55 s and 182 KB, 0.74 s and 188 KB, measured) and rewrites "What the tables cost".
- **M3, fixed.** Direct compiler calls run under `env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH` (Global Constraints, Tasks 4 and 12).
- **M4, fixed.** `snap.swift <page> <width> <png>`, with a browser as the fallback (Task 13 Step 7).
- **M5, fixed.** The four `declared-targets` lines, in any order.
- **M6, fixed.** The strict pass leaves a listed line out; an entry is the UTS 46 pass's (`uts46_test`, AGENTS.md, the testing chapter).
- **M7, fixed.** Task 5 Step 4 compares two runs of both `.ipp` files.
- **M8, fixed.** Task 0 Step 3 runs the lint once on the base.
- **M9, fixed.** Guarantee 4 says "returns"; `to_ascii` and Punycode's span forms say their buffer's contents are unspecified after an error.
- **M10, fixed.** `:unicode-version: 17.0.0` in `idna.adoc`, used by every section; AGENTS.md's "Moving to another version" names it.
- **M11, fixed.** Task 6's Expected reads "a line naming idna's lane `tables`".
- **M12, fixed.** The page says `update-generated` is the one writer; the run by hand appears in AGENTS.md alone, as the way outside b2.
- **M13, fixed.** `punycode_test` pins the encoder's overflow: 3,855 basic code points before U+10FFFF overflow, 3,854 do not (measured); Task 14 Step 5 spells the identity on its commit.
- **Found in the revision (1).** The real GCC 15 lane aborted `allocation_test`: libstdc++'s `new_delete_resource` asks `char32_t`'s alignment of 4, which macOS's `aligned_alloc` refuses; the replacement raises the alignment to a pointer's (Task 2).
- **Found in the revision (2).** The real lint, on every target, asked `auto*` for array iterators in `properties.hpp` and `vectors_test.cpp`; both keep indexes now (Tasks 3 and 5).
- **Formats coordination.** `[#idna-introduction]` is written in Task 1; idna's `webcpp.generated` lines are committed only in a worktree made after `lib/formats` carries S1 (Task 6); no doc build of idna's runs in the integration space but the controller's verdict.
