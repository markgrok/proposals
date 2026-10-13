<!-- The implementation plan of webcpp::formats (2026-10-09), every task with its code, kept as written: every task is done (reference/STATUS.md). Names, the regex engine and the commit identity changed since, as spec.md says at its top. -->

---
title: "webcpp::formats: implementation plan"
tags: [webcpp, formats, json-schema, boost-url, plan]
status: active
created: 2026-10-09
---

# formats Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build `webcpp::formats`, header-only C++20 validators and Boost.URL
grammar rules for the string formats of JSON Schema, with its tests, its run
of the official suite, its page, its examples and its CI, as a local
repository at `~/dev/webcpp-formats/libs/formats`, inside the superproject
worktree `~/dev/webcpp-formats` (branch `lib/formats`) that it shares with
idna, ready for the owner to publish as `webcpporg/formats`.

**Architecture:**
- **Every format is a Boost.URL rule** in `webcpp::formats::grammar`
  (`<name>_rule_t` and its `constexpr` object `<name>_rule`) that yields the
  text it matched, and a validator `is_<name>(std::string_view)` that is
  exactly `boost::urls::grammar::parse(s, <name>_rule).has_value()`.
- **ASCII grammars** (dates, UUID, host names' labels, e-mail's address
  literal, URI templates' expressions) are written with Boost.URL's
  combinators over formats' terminals, as the owner's decision 5 and spec 5.9
  say; `ipv4`, `ipv6`, `uri` are Boost.URL's own rules; `uri-reference` is
  Boost.URL's rule plus a three-line guard. The UTF-8 terminals (JSON Pointer
  tokens, template literals, local parts, IRIs) are hand-written. **IRIs**
  are checked code point by code point (RFC 3987 2.2, 4.1), mapped to a URI
  (3.1) and handed to Boost.URL. **IDN formats** ask `webcpp::idna`.
  **`regex`** is SRELL, in `<webcpp/formats/regex.hpp>` alone.
- **Nothing allocates** but an IRI's mapped copy beyond 1,024 octets and
  `is_regex`, both only through a `std::pmr::memory_resource`.

**Tech Stack:** C++20; b2 5.5.3; Boost 1.92 (Boost.URL's grammar, Boost.System,
Boost.Core's lightweight_test, Boost.JSON for the suite runner only);
`webcpp::idna`; SRELL 2026.07 from the local mirror of `webcpporg/srell`
(three commits: import, no-exceptions patch, allocator patch); wasi-sdk 34
and wasmtime 47.0.3; Emscripten 6.0.11; MrDocs 2026.9.29; Asciidoctor.js.

**Spec:** `~/.buzz/PLANS/WEBCPP_FORMATS_SPEC.md` (approved
2026-10-09; its two closing paragraphs are owner decisions and win over its
P6 and section 8.3's S4 row). Also read:
- `~/.buzz/PLANS/WEBCPP_IDNA_SPEC.md` sections 3.1 (the shared
  worktree) and 4, the idna API this plan calls (`check_label`, `to_ascii`,
  `max_label_length`, `max_domain_length`, `idna::error`);
- `~/.buzz/PLANS/WEBCPP_SHARED_INFRA_PLAN.md`, which builds S1
  to S5 on `infra/shared` in `~/dev/webcpp-infra`; the names this plan uses
  are the ones it fixes (`/webcpp/third_party//json-schema-test-suite`,
  `WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE`, `/webcpp/third_party//srell`,
  `/webcpp//boost_url`, `.local/boost-source`, `webcpp.data`), and its
  section "Integration" step 5 is how a track checks out the submodules;
- `~/.buzz/PLANS/WEBCPP_IDNA_PLAN.md`, idna's track, which shares
  `~/dev/webcpp-formats` and its branch with this one (Global Constraints,
  "Shared items");
- `~/dev/webcpp/AGENTS.md`, the superproject's rulebook, in
  full; `libs/pratt` and `libs/xactor` are the models;
- `~/.buzz/PLANS/WEBCPP_DESIGN.md`, "Owner decisions after
  approval" (2026-10-09: compiled Boost libraries from the source tarball;
  SRELL's allocator patch);
- research: `~/.buzz/RESEARCH/URI_TEMPLATE_AND_IRI_LIBRARIES.md` and its
  prototype `~/.buzz/RESEARCH/URI_TEMPLATE_AND_IRI_PROBE/handwritten.hpp`
  and `suite.py` (the 49 extra probes), `JSON_SCHEMA_ENGINE_SPIKE.md`
  sections 2.6, 4 and 6, `SRELL_ALLOCATION_SPIKE.md` sections 2 and 3,
  `IDNA_SPIKE.md` section 2.3.

## Global Constraints

**The owner's hard rules, verbatim in effect:**
- Commit with `git -c user.name=R -c user.email=github.overboard312@passmail.net commit`. No trailer of any kind, never an agent co-author.
- Stage files by path. Never `git add -A`, never `git add libs`. Never push, never add a remote, never touch GitHub; nothing reads from GitHub either (the third-party submodules come from local clones).
- English only. Never the em dash; use a plain dash. Never the word the pattern `veru[s]` matches, in any case, in a file, a file name or a commit. No "stage N" or "Task N" in tracked files or commit messages, and no S label (S1 to S5) either.
- Every new source file (`.hpp`, `.cpp`, `.py`, `.jam`, `Jamfile`, `build.jam`, `.yml`, `.adoc`) opens with the BSL-1.0 notice "Copyright (c) 2026 WebCpp.org" in the form `libs/pratt` uses (`//` for C++ and AsciiDoc, `#` for Jam, Python and YAML).
- b2 runs as `env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH b2 --user-config=.local/user-config.jam`, from the root of the superproject checkout the task works in. Below, **`B2`** stands for that whole prefix (in a shell: `B2='env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH b2 --user-config=.local/user-config.jam'`, then `$B2 ...`). A verdict uses `-a`. Each lane running at the same time as another gets its own `--build-dir`.
- Targets are native, wasip2, wasip3 and emscripten: every test Jamfile says `webcpp.targets native wasip2 wasip3 emscripten ;`.

**Where the work happens.**
- The superproject worktree `~/dev/webcpp-formats`, branch `lib/formats`
  (idna spec section 3.1), shared with idna's track. Never `~/dev/webcpp`:
  its Jamroot's `glob libs/*/build.jam` and its lint read every directory
  under `libs/`, and a half-built library there would break the stage-4
  lanes; only Task 11's publication touches it.
- The library is a git repository of its own at
  `~/dev/webcpp-formats/libs/formats` (`git init -b main`, Task 0), beside
  idna's at `~/dev/webcpp-formats/libs/idna`, both untracked by the
  superproject until the owner adds the submodules.
- A task marked **∥** works in a superproject worktree of its own,
  `~/dev/webcpp-formats-<slug>`, detached at its base (below), whose
  `libs/formats` is a git worktree of the library on branch `task/<slug>`
  (Task 0, Step 4). The controller rebases each task branch onto the
  library's `main`, reruns the task's verification there, and fast-forwards
  `main`: no merge commits (Task 0, Step 5).
- The libraries' own submodules (`libs/xactor` and the others) stay
  uninitialised in every formats checkout: formats needs none, and the
  Jamroot and the lint skip an empty `libs/<name>`.

**Shared items (S1 to S5) and the bases.** They are built by the infra track
on `infra/shared` in `~/dev/webcpp-infra`, in its order S2, S1, S4, S5, S3,
each a commit on top of the last, so a later commit of `infra/shared`
carries every earlier item. They reach formats one way only: the controller
rebases `lib/formats` onto `infra/shared` once the item a task needs is
committed there (`git -C ~/dev/webcpp-formats rebase infra/shared`), and,
once infra integrates into `main`, onto `main` (the duplicate commits drop).
idna's track is told before each rebase, since it shares the branch. Then the
submodules and Boost's sources are set up from local repositories, in
`~/dev/webcpp-formats` and in every task worktree made or moved after it
(the infra plan's "Integration", step 5):

```bash
# from the checkout's root, for each submodule its HEAD records and the
# directory does not hold yet
git clone --no-checkout ~/.buzz/REPOS/JSON-Schema-Test-Suite third_party/libs/JSON-Schema-Test-Suite \
  && git -C third_party/libs/JSON-Schema-Test-Suite checkout --detach 5b0ee1613e45fcc2bddac00e07c19cd49b00d8a8
git clone ~/.buzz/REPOS/srell.git third_party/libs/srell \
  && git -C third_party/libs/srell checkout --detach "$(git ls-tree HEAD third_party/libs/srell | awk '{print $3}')"
ln -sfn ~/dev/webcpp-infra/.local/boost-source .local/boost-source
git submodule status third_party/libs/JSON-Schema-Test-Suite third_party/libs/srell
```

Expected: each submodule shown at the commit `HEAD` records, without a `-` or
`+` before it. SRELL comes from the infra track's local mirror
`~/.buzz/REPOS/srell.git` until `webcpporg/srell` exists. The bases, each
named by the commit `lib/formats` has after its rebase, recorded in the
ledger:

| Base | `lib/formats` at | Brings | Tasks that start on it |
| --- | --- | --- | --- |
| `B0` | `main`'s commit (Task 0) | nothing shared | 1, 2, 3 |
| `B-S2` | `infra/shared` with the suite (infra Task 1) | S2 | 4 |
| `B-S1` | `infra/shared` with `webcpp.generated` (infra Task 2) | S2, S1 | idna I2's generated tables (idna's track), and every formats task once `libs/idna` holds them |
| `B-S4` | `infra/shared` with `/webcpp//boost_url` (infra Task 3) | S2, S1, S4 | 5, 7, 8, 9 |
| `B-S5` | `infra/shared` with `webcpp.data` (infra Task 4) | S2, S1, S4, S5 | 10 |
| `B-S3` | `infra/shared` with SRELL (infra Task 5) | all five | 6 |

A task starts on the latest base when it starts, never an older one; one
already running when `lib/formats` moves is moved before its integration
(`git -C ~/dev/webcpp-formats-<slug> checkout --detach lib/formats`, then the
block above), so a library `main` that names a shared item never meets a
superproject without it. S1 is in `lib/formats` before idna commits its
generated-table Jamfiles: idna's test Jamfile uses `webcpp.generated`, and a
Jamfile that fails to load stops every build of the checkout.

**The lanes** (`<t>` is the task's slug; run all five at once, each its own build directory):

```bash
B2 -a --build-dir=bin/formats-<t>-native libs/formats/test
B2 -a --build-dir=bin/formats-<t>-gcc toolset=gcc-15 libs/formats/test
B2 -a --build-dir=bin/formats-<t>-wasip2 toolset=clang-wasip2 testing.launcher=wasmtime libs/formats/test
B2 -a --build-dir=bin/formats-<t>-wasip3 toolset=clang-wasip3 testing.launcher=wasmtime libs/formats/test
B2 -a --build-dir=bin/formats-<t>-emscripten toolset=emscripten libs/formats/test
```

From the task that creates `example/` (Task 10) on, each lane also names
`libs/formats/example`. A lane is green when b2's last lines show
`...updated <n> targets...` and no `...failed updating` line. Export
`EM_CACHE=~/dev/webcpp/.local/emscripten-cache` before an emscripten lane.

**The lint and the page:**
- `tools/lint/lint.sh --clang-format .local/wasi-sdk/bin/clang-format --clang-tidy .local/wasi-sdk/bin/clang-tidy` ends with `lint: clean` before every commit. Below, **`LINT`**. It reads `libs/idna` too where it is checked out: idna's working tree is clean at a commit when formats' verdict runs.
- `B2 -a --build-dir=bin/formats-<t>-doc libs/formats/doc` exits 0 before every commit that touches a header or the page; only one doc build runs at a time on a checkout (AGENTS chapter 8), the idna track's included.

**C++ rules (AGENTS chapter 6, the spec sections 2 and 4).**
- Header-only C++20; `<webcpp/formats/config.hpp>` is the first include of every public header.
- Every public symbol has a full Doc Comment (AGENTS chapter 7); a `detail` symbol a one-sentence brief. A `@see "<title>"` names a section the page has in the same commit. No bare `@` in a Doc Comment: the at sign is written in words.
- No `throw`, `try` or `catch` in a header; formats raises nothing of its own. `WEBCPP_FORMATS_NO_EXCEPTIONS` exists and changes nothing.
- No global or static mutable state; no clock, file, network or environment in a header.
- Text is `std::string_view`; a rule yields `grammar::text` (`boost::core::string_view`).
- A function starts with its guards.
- **Rules (spec P1 to P3):** every `<name>_rule_t` has `using value_type = text;` (but `uri_reference_rule_t`, whose `value_type` is `boost::urls::url_view`), and a `parse(char const*& it, char const* end) const`. On success `it` is past the match; on failure `it` is unchanged. A rule returns `boost::urls::grammar::error::mismatch` when no input of its production starts at `it`, and `error::invalid` when one started and broke a constraint; an IDN rule, and a rule that holds one, returns the `idna::error` that refused it.
- Boost.URL 1.92's `variant_rule` returns `error::mismatch` when every alternative fails, discarding their errors (`boost/url/grammar/impl/variant_rule.hpp`): a rule whose error must reach its caller (idna's) is never an alternative of a `variant_rule`.
- Every `is_x(s)` is `boost::urls::grammar::parse(s, grammar::x_rule).has_value()` (`is_time` with its policy, the IRI functions with their resource), so a function and its rule never disagree; `is_ipv4`, `is_ipv6` and `is_uri` call Boost.URL's rules and `parse_uri`.
- `range_rule` takes a nested rule only through `boost::urls::grammar::ref(...)` (spec 4.3, risk 3), the rule an `inline constexpr` object.
- In a `grammar` namespace function, Boost.URL's grammar is spelled `detail::urlg::`: an alias in `grammar` would be a public name of the reference.
- clang-tidy's `readability-convert-member-functions-to-static` may report a `parse` that reads no member. A rule keeps `parse` a const member, as Boost.URL's Rule concept and the spec's section 4.2 declare it; the line clang-tidy names carries `// NOLINT(readability-convert-member-functions-to-static)` with that reason in the comment above.
- A string literal never puts a digit right after `\0`: `"...\0" "380"`, since `\0380` is the octal escape `\03`.

**Allocation (spec section 7, with the owner's 2026-10-09 SRELL decision).**
Nothing allocates, but: the IRI functions and rules beyond 1,024 mapped
octets (one buffer, from the given resource, else
`std::pmr::get_default_resource()`), and `is_regex`, only through the
resource it is given (`is_regex(pattern)` uses the default resource). The
spec's P6 ("allocates through SRELL's `malloc`") is superseded.

**Dependencies join where they are first used**, in `build.jam`'s alias,
always in spec 3.3's order (`<library>/boost//headers`,
`<library>/webcpp/idna//idna`, `<library>/webcpp//boost_url`,
`<library>/webcpp/regex//regex`) under one comment that names each:
Boost.URL's compiled part with IP and URIs (Task 5), idna with host names
(Task 8), webcpp::regex with regex (WEBCPP_REGEX_PLAN.md's Task F7). The
spec's F0 names idna in the skeleton's `build.jam`; it joins later here so
that the skeleton needs nothing that is not yet built.

**`config.hpp`** includes `<boost/config.hpp>` until `webcpp::config` exists,
and says the migration: that include becomes `<webcpp/config.hpp>`, as idna's
spec section 3.4 does.

**The suite.** `/webcpp/third_party//json-schema-test-suite` gives a test the
submodule `third_party/libs/JSON-Schema-Test-Suite` (pinned at `5b0ee16`):
its root as the define `WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE`, a dependency on
every file, a refusal by name when it is not checked out, and, from S5, the
directory on WASI. Known-failure lists are
`test/known_failures/<dialect>.json`, arrays of
`{"file", "case", "test", "reason"}` exactly as json_schema writes them: `file`
is the path under the dialect's directory (`optional/format/uuid.json`, and
`format/uuid.json` for v1), `case` and `test` the suite's descriptions. Every
list is empty.

**The page's sections, in this order** (each task adds its own at its place):
`introduction.adoc`, `dates.adoc`, `identifiers.adoc`, `addresses.adoc`,
`uris.adoc`, `templates.adoc`, `regex.adoc`, `composing.adoc`, `errors.adoc`,
`allocation.adoc`, `suite.adoc`, `guarantees.adoc`, `testing.adoc`, the
reference, `deviations.adoc` (the appendix "Known deviations"). Anchors are
`[#formats-<file stem>]`. A character list that holds `|` or a grave accent
is written in words in the page and in Doc Comments: a bar ends an AsciiDoc
table cell, and a grave accent breaks a code span.

**Reports.** Every task's report ends with a section "Changelog", in Boost's
release-notes style. formats is a new library: its one line is under "New
Libraries", written by Task 1 and refined, never multiplied, until the first
release. The controller copies it into
`~/.buzz/PLANS/WEBCPP_CHANGELOG_PENDING.md`. The line:

```
- formats: validators and Boost.URL grammar rules for the string formats of
  JSON Schema (dates and times, e-mail addresses, host names, IP addresses,
  URIs, IRIs, URI templates, JSON Pointers, UUIDs, regular expressions), for
  C++20, from R.
```

## Review Focus

1. **Hostile patterns in `is_regex`.** JSON Schema runs `format: regex` on instance strings, which an attacker writes. SRELL's compiler may recurse once per nested group, alternative, quantifier or lookaround, so 10,000 nested `(`, 10,000 nested `(?:a)*`, 10,000 nested lookaheads or 100,000 alternatives could overflow the 64 KiB default stack of wasm32-wasip2, wasm32-wasip3 and emscripten. The validator must answer, never trap, on every target. Pinned in Task 6 (measured on the three, then bounded if needed), and the 1 MiB inputs of every other format in Task 10's `allocation_test`.
2. **Embedded NUL bytes.** A `std::string_view` holding `\0` is judged on all its bytes: invalid wherever the grammar has no NUL (dates, UUID, IP, URIs, IRIs, templates, host names, e-mail), valid inside a JSON Pointer token (RFC 6901 `%x00-2E`) and as a literal in a regex. Pinned in Tasks 2, 3, 5, 6, 7, 8.
3. **Ill-formed UTF-8 where non-ASCII is otherwise valid** (guarantee 4): a JSON Pointer token, a template literal, an IRI path, an idn-email local part, an idn-hostname label, and a regex, which formats checks itself because SRELL's handling of ill-formed UTF-8 is unmeasured. Pinned in Tasks 1, 3, 6, 7, 9.
4. **The IRI's storage at its edges.** An ASCII IRI of any length never touches the resource (a `null_memory_resource` works); a non-ASCII IRI of exactly 1,024 mapped octets stays inline, ones of 1,025 and of 342 non-ASCII octets take one buffer; a resource that refuses makes `is_iri` propagate `std::bad_alloc`, never answer a wrong verdict. Pinned in Task 7.
5. **Lowercase duration designators** (`p1d`, `pt1h`). At `5b0ee16` every one of the 46 duration tests uses upper-case designators, so the suite is silent; RFC 3339 appendix A's ABNF governs, whose quoted letters RFC 5234 2.3 makes case-insensitive, as the controller ruled. Accepted, and pinned in Task 2.

## Decisions this plan makes for the owner

Each is stated here once, applied in the task named, and recorded on the page
where a reader meets it.

| Decision | Why | Where |
| --- | --- | --- |
| A public header `grammar.hpp` holds `grammar::text` and `grammar::result` | the two aliases of spec 4.2 need one home every format header includes; accepted by the controller | Task 1 |
| idna joins `build.jam` with host names, not in the skeleton | the skeleton then needs nothing not yet built; accepted by the controller | Task 8 |
| Duration designators match in either case | RFC 3339 appendix A with RFC 5234 2.3; the suite has no lower-case designator (controller's ruling) | Task 2, page "Dates and times" |
| A host name's A-labels are judged one label at a time, so `0a.xn--4db` is a `hostname` and `0a.א` no `idn-hostname` | spec P9; no suite test pairs an LTR label with an RTL A-label (controller's ruling) | Tasks 8, 9, page "Addresses" |
| A regex nested deeper than a measured bound is refused, if SRELL traps below it on a wasm target | an instance string must never crash the validator (controller's ruling) | Task 6, page "Known deviations" |
| A zone identifier is valid in a URI and an IRI | Boost.URL's verdict, kept by spec P11; the prototype's probe (`suite.py` EXTRA) expected invalid | Tasks 5, 7, page "Known deviations" |
| The e-mail rules dispatch the domain on `[` rather than through `variant_rule` | `variant_rule` replaces idna's error by `mismatch`, and spec section 6 asks for idna's reason | Tasks 8, 9 |
| The URI template's expression, the host names' labels, dates, UUIDs and the address literal use Boost.URL's combinators; the UTF-8 terminals and the IRI scan are hand-written | owner decision 5 and spec 5.9 for ASCII grammars; Boost.URL has no UTF-8 character sets | Tasks 2, 3, 7, 8 |
| Known-failure entries use json_schema's `file`, the path under the dialect's directory | an entry moves between the two lists unchanged | Task 4 |
| The suite runner skips a format whose validator is not yet written, by name, until every format has one | a format task registers itself as it lands; the list leaves when empty | Tasks 4 to 10 |

---

## Tasks at a glance

| Task | Spec | Base | Needs | Runs in parallel (∥) with |
| --- | --- | --- | --- | --- |
| 0. Set-up (controller) | | `B0` | | |
| 1. Skeleton and shared terminals | F0 | `B0` | nothing | idna I0 to I3 |
| 2. Dates and times | F1 | `B0` | Task 1 | ∥ 3, 5, 6 |
| 3. UUID and JSON Pointers | F2 | `B0` | Task 1 | ∥ 2, 5, 6 |
| 4. The suite runner | F1's runner | `B-S2` | S2; Tasks 2, 3 | ∥ 5, 6 |
| 5. IP addresses and URIs | F3 | `B-S4` | S4; Task 1 (Task 4 for its last step) | ∥ 2, 3, 4, 6 |
| 6. Regular expressions | F7 | `B-S3` | S3; Task 1 (Task 4 for its last step) | ∥ 2 to 5, 7 to 9 |
| 7. IRIs and URI templates | F4 | `B-S4` | Tasks 4, 5 | ∥ 6, 8 |
| 8. Host names and e-mail | F5 | `B-S4` (with `B-S1`) | Tasks 4, 5; idna I4 | ∥ 6, 7 |
| 9. IDN host names and IDN e-mail | F6 | `B-S4` (with `B-S1`) | Task 8; idna I5 | ∥ 6, 7 |
| 10. Every target, the page, readiness | F8 | `B-S5` or later | S5; Tasks 1 to 9; idna I6 (its page's anchors) | |
| 11. Review and publication (controller) | | | the owner's go | |

The tasks that need nothing come first: Task 1, then Tasks 2 and 3. The
controller starts Tasks 4, 5 and 6 as S2, S4 and S3 reach `lib/formats`.
Since infra builds S3 last, Task 6's base is the last to arrive.

## File structure

```
~/dev/webcpp-formats/                     the superproject worktree, branch lib/formats
  .local/                                 user-config.jam (a copy) and links (Task 0)
  third_party/libs/JSON-Schema-Test-Suite, third_party/libs/srell   local clones
  libs/idna/                              idna's repository (idna's track)
  libs/formats/                           this library's repository, branch main
    build.jam  LICENSE_1_0.txt  README.md  AGENTS.md  .gitignore  .gitattributes
    .github/workflows/ci.yml
    meta/libraries.json  meta/include-boundaries.json
    include/webcpp/formats.hpp              every public header but regex.hpp
    include/webcpp/formats/config.hpp       WEBCPP_FORMATS_NO_EXCEPTIONS
    include/webcpp/formats/grammar.hpp      grammar::text, grammar::result
    include/webcpp/formats/date_time.hpp    leap_seconds; date-time, date, time, duration
    include/webcpp/formats/uuid.hpp
    include/webcpp/formats/json_pointer.hpp json-pointer, relative-json-pointer
    include/webcpp/formats/ip.hpp           ipv4, ipv6
    include/webcpp/formats/uri.hpp          uri, uri-reference, iri, iri-reference
    include/webcpp/formats/uri_template.hpp
    include/webcpp/formats/hostname.hpp     hostname, idn-hostname, idna_label_rule
    include/webcpp/formats/email.hpp        email, idn-email
    include/webcpp/formats/regex.hpp        is_regex, the only header that includes SRELL
    include/webcpp/formats/detail/utf8.hpp         decode_utf8 (P12)
    include/webcpp/formats/detail/charsets.hpp     the ASCII sets; ucschar, iprivate, bidi
    include/webcpp/formats/detail/rules.hpp        exactly<N>, ci_literal_rule
    include/webcpp/formats/detail/uri_guard.hpp    refuses_relative_authority_path
    include/webcpp/formats/detail/iri_mapping.hpp  scan, check, map; the inline buffer
    include/webcpp/formats/detail/idn_scan.hpp     scan_idn_run
    test/Jamfile  test/.clang-tidy  test/require.hpp  test/counting_resource.hpp
    test/config_test.cpp  test/detail_test.cpp  test/allocation_test.cpp
    test/date_time_test.cpp  test/uuid_test.cpp  test/json_pointer_test.cpp
    test/ip_test.cpp  test/uri_test.cpp  test/iri_test.cpp  test/iri_resource_test.cpp
    test/uri_template_test.cpp  test/hostname_test.cpp  test/email_test.cpp
    test/regex_test.cpp  test/grammar_test.cpp  test/suite_test.cpp
    test/known_failures/{draft4,draft6,draft7,draft2019-09,draft2020-12,v1}.json
    example/Jamfile  example/.clang-tidy
    example/formats_quick_start.cpp  formats_grammar.cpp  formats_iri.cpp  formats_regex.cpp
      (each with its .expected)
    doc/Jamfile  doc/mrdocs.yml  doc/counts.py  doc/formats.adoc  doc/<section>.adoc
~/dev/webcpp-formats-<slug>/              a ∥ task's superproject worktree, detached
  libs/formats/                           a worktree of the library, branch task/<slug>
```

`grammar.hpp` is a public header the spec's tree does not list: the two
aliases of its section 4.2 need one home that every format header includes.

---

### Task 0: Set-up (controller)

- [ ] **Step 1: The superproject worktree and the library's repository.**
  If idna's track has not made `~/dev/webcpp-formats` yet:

```bash
git -C ~/dev/webcpp worktree add ~/dev/webcpp-formats -b lib/formats main
mkdir ~/dev/webcpp-formats/.local
cp ~/dev/webcpp/.local/user-config.jam ~/dev/webcpp-formats/.local/user-config.jam
for name in wasi-sdk wit-bindgen wasi-wit mrdocs emsdk emscripten; do
    ln -s ~/dev/webcpp/.local/$name ~/dev/webcpp-formats/.local/$name
done
```

  Then, in either case:

```bash
git init -b main ~/dev/webcpp-formats/libs/formats
git -C ~/dev/webcpp-formats status --short
```

Expected: `?? libs/formats/` (and `?? libs/idna/` once idna's repository
exists): the superproject records neither. `git -C ~/dev/webcpp status
--short` shows no `libs/formats`.

- [ ] **Step 2: The pre-flight.** From `~/dev/webcpp-formats`:

```bash
B2 -d0 --build-dir=bin/formats-preflight declared-targets; echo "status $?"
.local/wasi-sdk/bin/clang-format --version
ls .local/emsdk/upstream/emscripten/em++ .local/wasi-sdk/bin/clang++ .local/mrdocs/bin/mrdocs
```

Expected: the Jamroot loads (`status 0`), listing only the libraries checked
out in this worktree (idna's once it exists); clang-format prints wasi-sdk 34's version; the three tools exist. Record
`B0` (`git -C ~/dev/webcpp-formats rev-parse HEAD`), and the commit of
`~/dev/webcpp-formats/libs/idna` once it exists, in the ledger.

- [ ] **Step 3: The ledger.** Create the SDD workspace of this plan with the
  bases of Global Constraints, the spec's path, and one row per task with
  its needs (the table above) and its state.

- [ ] **Step 4: A worktree for a ∥ task**, run by the controller before the
  task starts (`<slug>` is the task's: `dates`, `ids`, `runner`, `uris`,
  `regex`, `iris`, `hosts`, `idn`; `<base>` is the commit of `lib/formats`
  the task's row names, the latest one):

```bash
git -C ~/dev/webcpp-formats worktree add --detach ~/dev/webcpp-formats-<slug> lib/formats
mkdir ~/dev/webcpp-formats-<slug>/.local
cp ~/dev/webcpp/.local/user-config.jam ~/dev/webcpp-formats-<slug>/.local/user-config.jam
for name in wasi-sdk wit-bindgen wasi-wit mrdocs emsdk emscripten; do
    ln -s ~/dev/webcpp/.local/$name ~/dev/webcpp-formats-<slug>/.local/$name
done
git -C ~/dev/webcpp-formats/libs/formats worktree add ~/dev/webcpp-formats-<slug>/libs/formats -b task/<slug> main
```

  then, from `~/dev/webcpp-formats-<slug>`, the submodule block of Global
  Constraints for the submodules its `HEAD` records. A task that needs idna
  (8, 9, 10) also gets
  `git -C ~/dev/webcpp-formats/libs/idna worktree add --detach ~/dev/webcpp-formats-<slug>/libs/idna <idna commit>`,
  at the idna commit that carries I4 (Task 8), I5 (Task 9) or I6 (Task 10).
  Expected: `git -C ~/dev/webcpp-formats-<slug> rev-parse HEAD` is `<base>`.
  The task runs every command from `~/dev/webcpp-formats-<slug>`.

- [ ] **Step 5: Integrating a ∥ task**, once its report is reviewed:
  1. If `lib/formats` moved since the task started, move the task's
     worktree to it: `git -C ~/dev/webcpp-formats-<slug> checkout --detach lib/formats`,
     then the submodule block.
  2. `git -C ~/dev/webcpp-formats-<slug>/libs/formats rebase main`. A
     conflict is resolved by keeping both sides' lines: the include list of
     `formats.hpp` sorted by name; the page's includes in the order of
     Global Constraints; the test Jamfile's lines in the order the tasks are
     numbered; `allocation_test.cpp`'s functions and calls both kept; the
     runner's `validators` sorted by format and `pending` sorted (it has no
     size to fix: `std::to_array` counts it); `build.jam`'s usage
     requirements in the order of Global Constraints under one comment that
     names each; `AGENTS.md`'s paragraphs both kept, in task order.
  3. In `~/dev/webcpp-formats-<slug>`, the task's five lanes, `LINT` and its
     doc build again, from scratch.
  4. `git -C ~/dev/webcpp-formats/libs/formats merge --ff-only task/<slug>`.
  5. The verdict in `~/dev/webcpp-formats` with `<t>` = `main`: the five
     lanes, `LINT` and the doc build, from scratch, at a moment the idna
     track agrees to (its working tree clean at a commit, no doc build of its
     own running), before the next integration.
  6. Remove the worktrees:
     `git -C ~/dev/webcpp-formats/libs/formats worktree remove ~/dev/webcpp-formats-<slug>/libs/formats`,
     the idna one likewise when there is one, then
     `git -C ~/dev/webcpp-formats worktree remove --force ~/dev/webcpp-formats-<slug>`
     (`--force`: its untracked `.local` and clones go with it).

### Task 1: The skeleton and the shared terminals (F0)

Works directly in `~/dev/webcpp-formats/libs/formats` on `main`, from the
superproject root `~/dev/webcpp-formats` at `B0` (nothing runs beside it in
this repository). Needs nothing.

**Files:**
- Create: `build.jam`, `LICENSE_1_0.txt` (a copy of `~/dev/webcpp/LICENSE_1_0.txt`), `.gitignore`, `.gitattributes`, `README.md`, `AGENTS.md`, `.github/workflows/ci.yml`
- Create: `meta/libraries.json`, `meta/include-boundaries.json`
- Create: `include/webcpp/formats.hpp`, `include/webcpp/formats/config.hpp`, `include/webcpp/formats/grammar.hpp`
- Create: `include/webcpp/formats/detail/utf8.hpp`, `detail/charsets.hpp`, `detail/rules.hpp`
- Create: `test/Jamfile`, `test/.clang-tidy`, `test/require.hpp`, `test/counting_resource.hpp`, `test/config_test.cpp`, `test/detail_test.cpp`, `test/allocation_test.cpp`
- Create: `doc/Jamfile`, `doc/mrdocs.yml`, `doc/formats.adoc`, `doc/introduction.adoc`, `doc/errors.adoc`, `doc/testing.adoc`

**Interfaces:**
- Consumes: nothing of formats; Boost.URL's grammar headers (`lut_chars`, `error`, `to_lower`), which link nothing in C++20 where `BOOST_URL_HAS_CXX20_CONSTEXPR` is defined: clang, gcc and the wasm and emscripten toolsets. MSVC never defines it (`boost/url/detail/config.hpp`, "MSVC ICEs on constexpr URL parsing"), so there the grammar's error category links Boost.URL's compiled part; MSVC is a CI lane only, which runs after Task 5 has added `/webcpp//boost_url`, and AGENTS.md says so from Task 5 on.
- Produces (every later task relies on these names and types):

```cpp
// <webcpp/formats/grammar.hpp>
namespace webcpp::formats::grammar {
using text = boost::core::string_view;
template <class T>
using result = boost::system::result<T>;
}

// <webcpp/formats/detail/utf8.hpp>
namespace webcpp::formats::detail {
constexpr bool decode_utf8(std::string_view s, std::size_t& i, char32_t& cp) noexcept;
}

// <webcpp/formats/detail/charsets.hpp>
namespace webcpp::formats::detail {
namespace urlg = boost::urls::grammar;
inline constexpr urlg::lut_chars ldh_chars;               // A-Z a-z 0-9 -
inline constexpr urlg::lut_chars hostname_chars;          // ldh_chars and .
inline constexpr urlg::lut_chars atext_chars;             // RFC 5322 3.2.3
inline constexpr urlg::lut_chars qtext_smtp_chars;        // RFC 5321 4.1.2
inline constexpr urlg::lut_chars quoted_pair_chars;       // %d32-126
inline constexpr urlg::lut_chars template_literal_chars;  // RFC 6570 2.1, erratum 6937
inline constexpr urlg::lut_chars varchar_chars;           // A-Z a-z 0-9 _
inline constexpr urlg::lut_chars template_operator_chars; // + # . / ; ? & = , ! @ |
constexpr bool is_ucschar(char32_t cp) noexcept;
constexpr bool is_iprivate(char32_t cp) noexcept;
constexpr bool is_bidi_format(char32_t cp) noexcept;
}

// <webcpp/formats/detail/rules.hpp>
namespace webcpp::formats::detail {
template <std::size_t N, class CharSet> struct exactly_rule_t;   // value_type grammar::text
template <std::size_t N, class CharSet>
constexpr exactly_rule_t<N, CharSet> exactly(const CharSet& chars) noexcept;
class ci_literal_rule;   // constexpr explicit ci_literal_rule(std::string_view); value_type grammar::text
}

// test/counting_resource.hpp
namespace webcpp::test {
class counting_resource;   // a std::pmr::memory_resource: allocations, deallocations, bytes
}
// test/allocation_test.cpp: global_allocations(), the calls of the replaced operator new
```

- [ ] **Step 1: The repository's files.**

`build.jam`:

```
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)
#
# formats, validators and Boost.URL grammar rules for the string formats of
# JSON Schema. A program uses it through /webcpp/formats//formats, which adds
# its headers and Boost's.

project /webcpp/formats ;

alias formats : : : : <include>include <library>/boost//headers ;
```

`.gitignore`:

```
doc/html/
```

`.gitattributes`:

```
* text=auto eol=lf
*.bat text eol=crlf
```

`meta/libraries.json`:

```json
{
    "key": "formats",
    "name": "formats",
    "authors": ["R"],
    "description": "Validators and Boost.URL grammar rules for the string formats of JSON Schema, for C++20.",
    "category": ["Parsing", "String"],
    "maintainers": ["R <github.overboard312@passmail.net>"],
    "cxxstd": "20",
    "port-of": null
}
```

`meta/include-boundaries.json` (Task 6 adds the `except` of `regex.hpp`):

```json
{
    "boundaries": [
        {
            "headers": [
                "include/webcpp/formats.hpp",
                "include/webcpp/formats/*.hpp",
                "include/webcpp/formats/detail/*.hpp"
            ],
            "must-not-include": ["srell"],
            "why": "only regex.hpp depends on SRELL"
        },
        {
            "headers": [
                "include/webcpp/formats.hpp",
                "include/webcpp/formats/*.hpp",
                "include/webcpp/formats/detail/*.hpp"
            ],
            "must-not-include": ["webcpp/idna/detail"],
            "why": "formats uses idna's public interface"
        },
        {
            "headers": [
                "include/webcpp/formats.hpp",
                "include/webcpp/formats/*.hpp",
                "include/webcpp/formats/detail/*.hpp"
            ],
            "must-not-include": ["boost/json"],
            "why": "JSON is the suite runner's, not the library's"
        }
    ]
}
```

`.github/workflows/ci.yml`: pratt's file
(`~/dev/webcpp/libs/pratt/.github/workflows/ci.yml`) with `pratt` replaced by
`formats` in its comment and in `library: formats`.

`AGENTS.md`:

```markdown
# Working on formats

webcpp's rules apply here: read the superproject's
[AGENTS.md](../../AGENTS.md)
(<https://github.com/webcpporg/webcpp/blob/main/AGENTS.md>) first. This file
holds only what is specific to formats.

- **A library of webcpp's own.** formats ports nothing: its evidence is the
  RFCs it cites, its unit tests, and the official JSON-Schema-Test-Suite run
  directly against its validators. It has no oracle and no twins.
- **A format is a rule and a function.** Every format is a Boost.URL rule in
  `webcpp::formats::grammar` and a validator `is_<name>`, which is the rule
  parsing the whole string. A change to one is a change to the other; the
  unit tests check that they agree on every input they hold.
- **Boost.URL is never modified.** formats composes Boost.URL's grammar
  library and adds terminals of its own; where Boost.URL is wrong, formats
  guards it and offers the fix upstream.
- **What raises.** formats raises nothing of its own; only a memory resource
  may throw, and only the IRI functions and `is_regex` take one.
```

`README.md`:

```markdown
# formats

formats is a header-only C++20 library of validators and Boost.URL grammar
rules for the string formats of JSON Schema, one of the libraries of
[webcpp](https://github.com/webcpporg/webcpp). Each format is a function that
takes a `std::string_view` and answers `bool`, allocating nothing, and a rule
of `webcpp::formats::grammar` that composes with Boost.URL's combinators.

formats is built and tested inside a checkout of the superproject:

    b2 libs/formats/test

Its page is published at <https://webcpporg.github.io/webcpp/libs/formats/>.
It is distributed under the Boost Software License, Version 1.0
([LICENSE_1_0.txt](LICENSE_1_0.txt)).
```

- [ ] **Step 2: The failing tests.**

`test/Jamfile`:

```
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

project : requirements <library>/webcpp/formats//formats ;

import webcpp ;

webcpp.targets native wasip2 wasip3 emscripten ;

webcpp.headers-alone formats : ../include ;

# config.hpp defines WEBCPP_FORMATS_NO_EXCEPTIONS exactly when the build has
# no exceptions, and keeps it when a developer defines it in a build that
# has them.
webcpp.compile config : config_test.cpp ;
webcpp.compile config_developer_defined : config_test.cpp
  : <define>WEBCPP_FORMATS_NO_EXCEPTIONS
    <define>WEBCPP_TEST_FORMATS_DEVELOPER_DEFINED ;

# The terminals every format shares: UTF-8, the character sets, the rules.
webcpp.run detail : detail_test.cpp ;
# What every validator allocates: nothing, but through a resource it is
# given.
webcpp.run allocation : allocation_test.cpp ;
```

`test/.clang-tidy`: xactor's file (`~/dev/webcpp/libs/xactor/test/.clang-tidy`),
which switches off `bugprone-unchecked-optional-access` (a test reads an
optional after `BOOST_TEST` asserted it, which the checker does not see) and
`bugprone-exception-escape.CheckMain`, each with its reason; its comment's
library names changed to formats'. `test/require.hpp`: pratt's
(`~/dev/webcpp/libs/pratt/test/require.hpp`), its guard renamed
`WEBCPP_TEST_FORMATS_REQUIRE_HPP`.

`test/config_test.cpp`: pratt's `libs/pratt/test/config_test.cpp` with
`pratt` replaced by `formats` and `PRATT` by `FORMATS` throughout (the include
`<webcpp/formats/config.hpp>`, the macros `WEBCPP_FORMATS_NO_EXCEPTIONS` and
`WEBCPP_TEST_FORMATS_DEVELOPER_DEFINED`).

`test/counting_resource.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// A memory resource that counts what goes through it, for the tests that check that formats
// allocates only through the resource it is given.

#ifndef WEBCPP_TEST_FORMATS_COUNTING_RESOURCE_HPP
#define WEBCPP_TEST_FORMATS_COUNTING_RESOURCE_HPP

#include <cstddef>
#include <memory_resource>

namespace webcpp::test {

/**
 A memory resource that counts its allocations, deallocations and bytes, and takes its memory
 from `std::pmr::new_delete_resource()`, so each allocation is also one call of the global
 operator new.
*/
class counting_resource final : public std::pmr::memory_resource {
public:
    std::size_t allocations = 0;
    std::size_t deallocations = 0;
    std::size_t bytes = 0;

private:
    void* do_allocate(std::size_t size, std::size_t alignment) override {
        ++allocations;
        bytes += size;
        return std::pmr::new_delete_resource()->allocate(size, alignment);
    }

    void do_deallocate(void* memory, std::size_t size, std::size_t alignment) override {
        ++deallocations;
        std::pmr::new_delete_resource()->deallocate(memory, size, alignment);
    }

    [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
};

}  // namespace webcpp::test

#endif  // WEBCPP_TEST_FORMATS_COUNTING_RESOURCE_HPP
```

`test/detail_test.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// The terminals every format shares: strict UTF-8 decoding (RFC 3629), the ASCII character
// sets, RFC 3987's code point classes, and the small rules exactly<N> and ci_literal_rule.

#include <webcpp/formats/detail/charsets.hpp>
#include <webcpp/formats/detail/rules.hpp>
#include <webcpp/formats/detail/utf8.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/url/grammar/digit_chars.hpp>
#include <boost/url/grammar/error.hpp>

#include <cstddef>
#include <iostream>
#include <string_view>

namespace detail = webcpp::formats::detail;
namespace urlg = boost::urls::grammar;

namespace {

/** A byte sequence and the code point it decodes to. */
struct decoded {
    std::string_view bytes;
    char32_t cp;
};

void well_formed_sequences_decode() {
    // RFC 3629 section 4's table, at each boundary of each length.
    for (const decoded& d : {decoded{std::string_view("\x00", 1), 0x0}, decoded{"\x7F", 0x7F},
                             decoded{"\xC2\x80", 0x80}, decoded{"\xDF\xBF", 0x7FF},
                             decoded{"\xE0\xA0\x80", 0x800}, decoded{"\xEF\xBF\xBF", 0xFFFF},
                             decoded{"\xF0\x90\x80\x80", 0x10000},
                             decoded{"\xF4\x8F\xBF\xBF", 0x10FFFF}}) {
        std::size_t i = 0;
        char32_t cp = 0;
        BOOST_TEST(detail::decode_utf8(d.bytes, i, cp));
        BOOST_TEST_EQ(static_cast<unsigned long>(cp), static_cast<unsigned long>(d.cp));
        BOOST_TEST_EQ(i, d.bytes.size());
    }
}

void ill_formed_sequences_are_refused() {
    // Overlong, surrogate, above U+10FFFF, a lone continuation byte, truncated, and the bytes no
    // sequence starts with; each leaves the index and the code point as they were.
    for (const std::string_view bytes :
         {"\xC0\xAF", "\xC1\xBF", "\xE0\x80\xAF", "\xF0\x80\x80\xAF", "\xED\xA0\x80", "\xED\xBF\xBF",
          "\xF4\x90\x80\x80", "\xF5\x80\x80\x80", "\x80", "\xBF", "\xC3", "\xE2\x82", "\xF0\x9F\x98",
          "\xFE", "\xFF", "\xC3\x28"}) {
        std::size_t i = 0;
        char32_t cp = U'x';
        if (!BOOST_TEST(!detail::decode_utf8(bytes, i, cp))) {
            std::cerr << "accepted: " << bytes.size() << " bytes\n";
        }
        BOOST_TEST_EQ(i, 0U);
        BOOST_TEST(cp == U'x');
    }
}

void a_sequence_decodes_where_the_index_says() {
    const std::string_view text = "a\xC3\xA9z";
    std::size_t i = 1;
    char32_t cp = 0;
    BOOST_TEST(detail::decode_utf8(text, i, cp));
    BOOST_TEST(cp == U'é');
    BOOST_TEST_EQ(i, 3U);
    std::size_t past = text.size();
    BOOST_TEST(!detail::decode_utf8(text, past, cp));
}

void ucschar_has_rfc_3987s_ranges() {
    for (const char32_t in : {0xA0U, 0xD7FFU, 0xF900U, 0xFDCFU, 0xFDF0U, 0xFFEFU, 0x10000U, 0x1FFFDU,
                              0x20000U, 0xDFFFDU, 0xE1000U, 0xEFFFDU}) {
        BOOST_TEST(detail::is_ucschar(in));
    }
    for (const char32_t out : {0x7FU, 0x9FU, 0xD800U, 0xE000U, 0xF8FFU, 0xFDD0U, 0xFDEFU, 0xFFF0U,
                               0xFFFDU, 0x1FFFEU, 0x1FFFFU, 0xE0000U, 0xE0FFFU, 0xEFFFEU, 0xF0000U,
                               0x10FFFDU}) {
        BOOST_TEST(!detail::is_ucschar(out));
    }
}

void iprivate_and_bidi_have_their_code_points() {
    for (const char32_t in : {0xE000U, 0xF8FFU, 0xF0000U, 0xFFFFDU, 0x100000U, 0x10FFFDU}) {
        BOOST_TEST(detail::is_iprivate(in));
    }
    for (const char32_t out : {0xDFFFU, 0xF900U, 0xFFFFEU, 0x10FFFEU, 0xA0U}) {
        BOOST_TEST(!detail::is_iprivate(out));
    }
    for (const char32_t in : {0x200EU, 0x200FU, 0x202AU, 0x202BU, 0x202CU, 0x202DU, 0x202EU}) {
        BOOST_TEST(detail::is_bidi_format(in));
    }
    for (const char32_t out : {0x200DU, 0x2029U, 0x202FU, 0x2066U}) {
        BOOST_TEST(!detail::is_bidi_format(out));
    }
}

void the_ascii_sets_hold_their_grammars() {
    BOOST_TEST(detail::ldh_chars('a') && detail::ldh_chars('Z') && detail::ldh_chars('7'));
    BOOST_TEST(detail::ldh_chars('-') && !detail::ldh_chars('.') && !detail::ldh_chars('_'));
    BOOST_TEST(detail::hostname_chars('.') && !detail::hostname_chars(':'));
    for (const char c : std::string_view("!#$%&'*+-/=?^_`{|}~aZ0")) {
        BOOST_TEST(detail::atext_chars(c));
    }
    for (const char c : std::string_view("()<>[]:;@\\,.\" ")) {
        BOOST_TEST(!detail::atext_chars(c));
    }
    BOOST_TEST(detail::qtext_smtp_chars(' ') && detail::qtext_smtp_chars('~'));
    BOOST_TEST(!detail::qtext_smtp_chars('"') && !detail::qtext_smtp_chars('\\'));
    BOOST_TEST(!detail::qtext_smtp_chars('\t') && !detail::qtext_smtp_chars('\x7F'));
    BOOST_TEST(detail::quoted_pair_chars(' ') && detail::quoted_pair_chars('"'));
    BOOST_TEST(!detail::quoted_pair_chars('\x1F') && !detail::quoted_pair_chars('\x7F'));
    // RFC 6570's literals with erratum 6937: the apostrophe is one.
    for (const char c : std::string_view("!#$&'()*+,-./09:;=?@AZ[]_az~")) {
        BOOST_TEST(detail::template_literal_chars(c));
    }
    for (const char c : std::string_view(" \"%<>\\^`{|}")) {
        BOOST_TEST(!detail::template_literal_chars(c));
    }
    BOOST_TEST(detail::varchar_chars('_') && !detail::varchar_chars('-') && !detail::varchar_chars('.'));
    for (const char c : std::string_view("+#./;?&=,!@|")) {
        BOOST_TEST(detail::template_operator_chars(c));
    }
    BOOST_TEST(!detail::template_operator_chars('-') && !detail::template_operator_chars('*'));
}

void exactly_matches_its_count_or_leaves_the_input() {
    constexpr auto four = detail::exactly<4>(urlg::digit_chars);
    const std::string_view text = "2026-10";
    char const* it = text.data();
    const auto year = four.parse(it, text.data() + text.size());
    BOOST_TEST(year.has_value() && *year == "2026");
    BOOST_TEST(it == text.data() + 4);
    for (const std::string_view refused : {"202", "20x6", ""}) {
        char const* at = refused.data();
        const auto r = four.parse(at, refused.data() + refused.size());
        BOOST_TEST(r.has_error() && r.error() == urlg::error::mismatch);
        BOOST_TEST(at == refused.data());
    }
}

void ci_literal_matches_in_any_case() {
    constexpr detail::ci_literal_rule ipv6("IPv6:");
    for (const std::string_view text : {"IPv6:::1", "ipv6:::1", "IPV6:::1"}) {
        char const* it = text.data();
        const auto r = ipv6.parse(it, text.data() + text.size());
        BOOST_TEST(r.has_value() && r->size() == 5);
        BOOST_TEST(it == text.data() + 5);
    }
    for (const std::string_view text : {"IPv4:1", "IPv6", "IP v6:"}) {
        char const* it = text.data();
        const auto r = ipv6.parse(it, text.data() + text.size());
        BOOST_TEST(r.has_error() && r.error() == urlg::error::mismatch);
        BOOST_TEST(it == text.data());
    }
}

}  // namespace

int main() {
    well_formed_sequences_decode();
    ill_formed_sequences_are_refused();
    a_sequence_decodes_where_the_index_says();
    ucschar_has_rfc_3987s_ranges();
    iprivate_and_bidi_have_their_code_points();
    the_ascii_sets_hold_their_grammars();
    exactly_matches_its_count_or_leaves_the_input();
    ci_literal_matches_in_any_case();
    return boost::report_errors();
}
```

`test/allocation_test.cpp` (each later task adds one function and its call
in `main`):

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Tests what formats allocates, by counting every call of the global operator new this program
// makes: nothing for every validator and rule, on long and hostile inputs, but the IRI functions
// beyond their inline buffer and is_regex, which allocate only through the resource they are
// given.

#include <boost/config.hpp>
#include <boost/core/lightweight_test.hpp>

#include <cstddef>
#include <cstdlib>
#include <memory_resource>
#include <new>
#include <string>

#ifdef _MSC_VER
#include <malloc.h>
#endif

#include "counting_resource.hpp"

namespace {

// The calls of the global operator new this program has made. It is mutable global state, which
// no library holds: it is this test program's own instrument, the one place where the replaced
// operator new below can record what it sees.
std::size_t global_new_calls = 0;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

/** The calls of the global operator new so far. */
std::size_t global_allocations() {
    return global_new_calls;
}

}  // namespace

// The replacements of the global operator new and operator delete, for this program alone. The
// standard library declares them with parameter names reserved to the implementation, which a
// program may not use, hence the exemption on each. None is ever inlined: GCC, seeing the malloc
// or the free inside one, would pair it with the other's call and warn of a mismatch.

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
BOOST_NOINLINE void* operator new(std::size_t size) {
    ++global_new_calls;
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
 Memory of `size` bytes aligned to `alignment`, from aligned_alloc, whose size must be a multiple
 of the alignment, or from _aligned_malloc under _MSC_VER, whose UCRT has no aligned_alloc.
*/
void* aligned_memory(std::size_t size, std::size_t alignment) {
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

// The aligned forms count too: libstdc++'s new_delete_resource allocates through them whatever
// the alignment, where libc++ uses the plain form for the default alignment.

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name)
BOOST_NOINLINE void* operator new(std::size_t size, std::align_val_t alignment) {
    ++global_new_calls;
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

/** The instrument counts: a string past any small-string buffer, and a resource's allocation. */
void the_instrument_counts() {
    const std::size_t before = global_allocations();
    {
        const std::string long_text(1000, 'x');
        BOOST_TEST_EQ(long_text.size(), 1000U);
    }
    BOOST_TEST_EQ(global_allocations() - before, 1U);
    webcpp::test::counting_resource resource;
    const std::size_t again = global_allocations();
    void* memory = resource.allocate(64, alignof(std::max_align_t));
    resource.deallocate(memory, 64, alignof(std::max_align_t));
    BOOST_TEST_EQ(resource.allocations, 1U);
    BOOST_TEST_EQ(resource.bytes, 64U);
    BOOST_TEST_EQ(global_allocations() - again, 1U);
}

}  // namespace

int main() {
    the_instrument_counts();
    return boost::report_errors();
}
```

- [ ] **Step 3: Run them and watch them fail.** From `~/dev/webcpp-formats`:

```bash
B2 --build-dir=bin/formats-skeleton-native libs/formats/test; echo "status $?"
```

Expected: FAIL before anything compiles. While the Jamfile loads,
`webcpp.headers-alone` stops the build with
`webcpp.headers-alone: ../include holds neither webcpp/formats.hpp nor a header under webcpp/formats/`,
and `status 1`.

- [ ] **Step 4: The headers.**

`include/webcpp/formats/config.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The configuration of formats, which each of its headers includes first: whether formats is built
 without exceptions.

 @see "Errors", in the guide.
*/
#ifndef WEBCPP_FORMATS_CONFIG_HPP
#define WEBCPP_FORMATS_CONFIG_HPP

// Boost.Config says whether the build has exceptions, until webcpp::config exists. The migration
// is one line here, this include becoming <webcpp/config.hpp>, with
// <library>/webcpp/config//config added to build.jam's alias in the same commit.
#include <boost/config.hpp>

// MrDocs parses formats as a build with exceptions does, where the macro is not defined: this
// definition, undone at once, is the one the reference lists.
#ifdef __MRDOCS__
/**
 Defined when formats is built without exceptions: automatically when the compiler has none
 (`BOOST_NO_EXCEPTIONS`), or by the developer to disable them in a build that has them.

 formats raises no exception of its own: a validator answers `false` and a rule returns its error,
 so the macro changes nothing in it. It is provided so that a program can set it for every webcpp
 library alike. Only a memory resource given to an IRI function or to `is_regex` may throw, and
 an exception formats raised would go through `boost::throw_exception`.

 @see "Errors", in the guide.
*/
#define WEBCPP_FORMATS_NO_EXCEPTIONS
#undef WEBCPP_FORMATS_NO_EXCEPTIONS
#endif

#if defined(BOOST_NO_EXCEPTIONS) && !defined(WEBCPP_FORMATS_NO_EXCEPTIONS)
#define WEBCPP_FORMATS_NO_EXCEPTIONS
#endif

#endif  // WEBCPP_FORMATS_CONFIG_HPP
```

`include/webcpp/formats/grammar.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The two types every rule of formats shares: the text a rule matched, and what a rule returns.

 @see "Errors", in the guide.
*/
#ifndef WEBCPP_FORMATS_GRAMMAR_HPP
#define WEBCPP_FORMATS_GRAMMAR_HPP

#include <webcpp/formats/config.hpp>

#include <boost/core/detail/string_view.hpp>
#include <boost/system/result.hpp>

namespace webcpp::formats::grammar {

/**
 The text a rule of formats matched, a view of its input.

 It is Boost.URL's text type, which converts to `std::string_view`.

 @see "Errors", in the guide.
*/
using text = boost::core::string_view;

/**
 What a rule returns: the value it matched, or the error that refused its input.

 @tparam T The rule's value type.
 @see "Errors", in the guide.
*/
template <class T>
using result = boost::system::result<T>;

}  // namespace webcpp::formats::grammar

#endif  // WEBCPP_FORMATS_GRAMMAR_HPP
```

The first two `@see` lines of this header send the reader to "Errors" until
the page has "Composing rules", which Task 9 writes and points them to.

`include/webcpp/formats/detail/utf8.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Strict UTF-8 decoding (RFC 3629), formats' own: no library reaches into another's detail.

#ifndef WEBCPP_FORMATS_DETAIL_UTF8_HPP
#define WEBCPP_FORMATS_DETAIL_UTF8_HPP

#include <webcpp/formats/config.hpp>

#include <array>
#include <cstddef>
#include <string_view>

namespace webcpp::formats::detail {

/**
 Decodes the code point at `s[i]` into `cp` and moves `i` past it; false, `i` and `cp` unchanged,
 for an ill-formed sequence (truncated, overlong, a surrogate, above U+10FFFF, a lone
 continuation byte) or `i` at the end.
*/
constexpr bool decode_utf8(std::string_view s, std::size_t& i, char32_t& cp) noexcept {
    if (i >= s.size()) {
        return false;
    }
    const auto lead = static_cast<unsigned char>(s[i]);
    if (lead < 0x80U) {
        cp = lead;
        ++i;
        return true;
    }
    std::size_t length = 0;
    if (lead >= 0xC2U && lead <= 0xDFU) {
        length = 2;
    } else if (lead >= 0xE0U && lead <= 0xEFU) {
        length = 3;
    } else if (lead >= 0xF0U && lead <= 0xF4U) {
        length = 4;
    } else {
        return false;
    }
    if (s.size() - i < length) {
        return false;
    }
    auto value = static_cast<char32_t>(lead & (0x7FU >> length));
    for (std::size_t k = 1; k < length; ++k) {
        const auto next = static_cast<unsigned char>(s[i + k]);
        if ((next & 0xC0U) != 0x80U) {
            return false;
        }
        value = static_cast<char32_t>((value << 6U) | (next & 0x3FU));
    }
    // The least code point of each length: below it, the sequence is overlong.
    constexpr std::array<char32_t, 5> least{0, 0, 0x80, 0x800, 0x10000};
    if (value < least[length] || value > 0x10FFFF || (value >= 0xD800 && value <= 0xDFFF)) {
        return false;
    }
    cp = value;
    i += length;
    return true;
}

}  // namespace webcpp::formats::detail

#endif  // WEBCPP_FORMATS_DETAIL_UTF8_HPP
```

`include/webcpp/formats/detail/charsets.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// The ASCII character sets of formats' grammars, as Boost.URL's lut_chars, and RFC 3987's code
// point classes.

#ifndef WEBCPP_FORMATS_DETAIL_CHARSETS_HPP
#define WEBCPP_FORMATS_DETAIL_CHARSETS_HPP

#include <webcpp/formats/config.hpp>

#include <boost/url/grammar/lut_chars.hpp>

namespace webcpp::formats::detail {

namespace urlg = boost::urls::grammar;

/** Letters, digits and the hyphen: the characters of a host name's label (RFC 1123 2.1). */
inline constexpr urlg::lut_chars ldh_chars(
    "-0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");

/** A host name's characters: its labels' and the dot. */
inline constexpr urlg::lut_chars hostname_chars(
    "-.0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");

/** RFC 5322 3.2.3's atext: letters, digits and the symbols below. */
inline constexpr urlg::lut_chars atext_chars(
    "!#$%&'*+-/=?^_`{|}~0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");

/** RFC 5321 4.1.2's qtextSMTP: the space and printable ASCII, but the double quote and the backslash. */
inline constexpr urlg::lut_chars qtext_smtp_chars([](char c) noexcept {
    return c >= ' ' && c <= '~' && c != '"' && c != '\\';
});

/** What may follow the backslash of RFC 5321 4.1.2's quoted-pairSMTP: %d32-126. */
inline constexpr urlg::lut_chars quoted_pair_chars([](char c) noexcept {
    return c >= ' ' && c <= '~';
});

/** RFC 6570 2.1's ASCII literals with erratum 6937: %x21 / %x23-24 / %x26-3B / %x3D / %x3F-5B / %x5D / %x5F / %x61-7A / %x7E. */
inline constexpr urlg::lut_chars template_literal_chars([](char c) noexcept {
    return c == 0x21 || (c >= 0x23 && c <= 0x24) || (c >= 0x26 && c <= 0x3B) || c == 0x3D ||
           (c >= 0x3F && c <= 0x5B) || c == 0x5D || c == 0x5F || (c >= 0x61 && c <= 0x7A) ||
           c == 0x7E;
});

/** RFC 6570 2.3's varchar but its pct-encoded triplets: letters, digits and the underscore. */
inline constexpr urlg::lut_chars varchar_chars(
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz");

/** RFC 6570 2.2's operators, op-reserve's included as the ABNF admits them. */
inline constexpr urlg::lut_chars template_operator_chars("+#./;?&=,!@|");

/** Whether `cp` is in RFC 3987 2.2's ucschar. */
constexpr bool is_ucschar(char32_t cp) noexcept {
    if ((cp >= 0xA0 && cp <= 0xD7FF) || (cp >= 0xF900 && cp <= 0xFDCF) ||
        (cp >= 0xFDF0 && cp <= 0xFFEF)) {
        return true;
    }
    // Plane 14 starts at U+E1000; every other supplementary plane up to 14 holds all but its last
    // two code points.
    if (cp >= 0xE0000 && cp <= 0xE0FFF) {
        return false;
    }
    return cp >= 0x10000 && cp <= 0xEFFFD && (cp & 0xFFFFU) <= 0xFFFDU;
}

/** Whether `cp` is in RFC 3987 2.2's iprivate. */
constexpr bool is_iprivate(char32_t cp) noexcept {
    return (cp >= 0xE000 && cp <= 0xF8FF) || (cp >= 0xF0000 && cp <= 0xFFFFD) ||
           (cp >= 0x100000 && cp <= 0x10FFFD);
}

/** Whether `cp` is a bidirectional formatting character, which RFC 3987 4.1 refuses in an IRI. */
constexpr bool is_bidi_format(char32_t cp) noexcept {
    return cp == 0x200E || cp == 0x200F || (cp >= 0x202A && cp <= 0x202E);
}

}  // namespace webcpp::formats::detail

#endif  // WEBCPP_FORMATS_DETAIL_CHARSETS_HPP
```

`include/webcpp/formats/detail/rules.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Small terminal rules that Boost.URL's grammar library does not have.

#ifndef WEBCPP_FORMATS_DETAIL_RULES_HPP
#define WEBCPP_FORMATS_DETAIL_RULES_HPP

#include <webcpp/formats/config.hpp>

#include <webcpp/formats/grammar.hpp>

#include <boost/url/grammar/ci_string.hpp>
#include <boost/url/grammar/error.hpp>

#include <cstddef>
#include <string_view>

namespace webcpp::formats::detail {

/** A rule that matches exactly `N` characters of `chars`, and yields them. */
template <std::size_t N, class CharSet>
struct exactly_rule_t {
    /** The rule yields the characters it matched. */
    using value_type = grammar::text;

    /** The set every matched character belongs to. */
    CharSet chars;

    /** Matches `N` characters of the set at `it`, or leaves `it` and answers mismatch. */
    grammar::result<grammar::text> parse(char const*& it, char const* end) const noexcept {
        if (static_cast<std::size_t>(end - it) < N) {
            return boost::urls::grammar::error::mismatch;
        }
        for (std::size_t i = 0; i < N; ++i) {
            if (!chars(it[i])) {
                return boost::urls::grammar::error::mismatch;
            }
        }
        char const* const first = it;
        it += N;
        return grammar::text(first, N);
    }
};

/** The rule that matches exactly `N` characters of `chars`. */
template <std::size_t N, class CharSet>
constexpr exactly_rule_t<N, CharSet> exactly(const CharSet& chars) noexcept {
    return exactly_rule_t<N, CharSet>{chars};
}

/** A rule that matches a literal with its ASCII letters in either case, and yields the text. */
class ci_literal_rule {
public:
    /** The rule yields the text it matched, in the input's case. */
    using value_type = grammar::text;

    /** The rule of `literal`, which outlives the rule (a string literal). */
    constexpr explicit ci_literal_rule(std::string_view literal) noexcept : literal_(literal) {}

    /** Matches the literal at `it`, or leaves `it` and answers mismatch. */
    grammar::result<grammar::text> parse(char const*& it, char const* end) const noexcept {
        if (static_cast<std::size_t>(end - it) < literal_.size()) {
            return boost::urls::grammar::error::mismatch;
        }
        for (std::size_t i = 0; i < literal_.size(); ++i) {
            if (boost::urls::grammar::to_lower(it[i]) != boost::urls::grammar::to_lower(literal_[i])) {
                return boost::urls::grammar::error::mismatch;
            }
        }
        char const* const first = it;
        it += literal_.size();
        return grammar::text(first, literal_.size());
    }

private:
    std::string_view literal_;
};

}  // namespace webcpp::formats::detail

#endif  // WEBCPP_FORMATS_DETAIL_RULES_HPP
```

`include/webcpp/formats.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 Every format of formats: include this one header for all of them. It leaves out
 `<webcpp/formats/regex.hpp>`, which brings SRELL, a regular expression engine a program that
 never validates a regex should not compile; that header is included by name.

 @see "Introduction", in the guide.
*/
#ifndef WEBCPP_FORMATS_HPP
#define WEBCPP_FORMATS_HPP

#include <webcpp/formats/config.hpp>

#include <webcpp/formats/grammar.hpp>

#endif  // WEBCPP_FORMATS_HPP
```

- [ ] **Step 5: Run the tests and watch them pass** on the five lanes of
  Global Constraints with `<t>` = `skeleton`. Expected: every lane green,
  with the `alone-formats...` tests of the public headers, `config`,
  `config_developer_defined`, `detail` and `allocation` passed;
  `B2 -d0 declared-targets` prints `formats native`, `formats wasip2`,
  `formats wasip3` and `formats emscripten`.
  If a lane fails to link `boost::urls::grammar::detail::error_cat`, that
  toolset lacks `BOOST_URL_HAS_CXX20_CONSTEXPR`: stop and report it, since
  Tasks 2 and 3 would then need S4 too.

- [ ] **Step 6: The page.** `doc/Jamfile`:

```
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

import webcpp ;

webcpp.doc formats : formats.adoc ;
webcpp.reference formats ;
```

`doc/mrdocs.yml`: pratt's file with `pratt` replaced by `formats` in its
comment (`show-enum-constants: true`, for `leap_seconds`).

`doc/formats.adoc`:

```
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

= formats: the string formats of JSON Schema for {cpp}
:toc: left
:toclevels: 1
:idprefix:
:sectanchors:
:example-caption!:
:attribute-missing: warn

formats is a header-only library of validators for the string formats of
JSON Schema, in {cpp}20, one of the libraries of
https://github.com/webcpporg/webcpp[webcpp]. Each format is a function that
takes a `std::string_view` and answers `bool`, allocating nothing, and a
Boost.URL grammar rule, which composes with Boost.URL's combinators into
grammars of a program's own.

include::introduction.adoc[]

include::errors.adoc[]

include::testing.adoc[]

[#reference]
== Reference

Every name formats declares, in the namespaces `webcpp::formats` and
`webcpp::formats::grammar`, from the Doc Comments of its headers. The other
sections teach the library; this one is where a name is looked up.

include::{reference}[leveloffset=+1]
```

`doc/introduction.adoc`:

```
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

[#formats-introduction]
== Introduction

JSON Schema names string formats, `date-time`, `email`, `hostname`, `uri`,
`regex` and the others, each defined by an RFC or by ECMA-262. formats checks
each of them as its reference defines it, and nothing more: a validator
answers whether a string is in the format, and a rule says where in a longer
input the format's text ends.

Every validator takes any byte string. One that is not well-formed UTF-8 is
invalid in every format, and in the formats that are ASCII, so is any byte
above 0x7F.

[%header,cols="2,3,4"]
|===
| Format | Function | Reference
| `date-time` | `is_date_time` | RFC 3339 5.6, 5.7
| `date` | `is_date` | RFC 3339 5.6 `full-date`
| `time` | `is_time` | RFC 3339 5.6 `full-time`
| `duration` | `is_duration` | RFC 3339 appendix A
| `email` | `is_email` | RFC 5321 4.1.2
| `idn-email` | `is_idn_email` | RFC 6531 3.3
| `hostname` | `is_hostname` | RFC 1123 2.1, RFC 5890 2.3.2.1
| `idn-hostname` | `is_idn_hostname` | RFC 5890 2.3.2.3, UTS 46
| `ipv4` | `is_ipv4` | RFC 2673 3.2
| `ipv6` | `is_ipv6` | RFC 4291 2.2
| `uri` | `is_uri` | RFC 3986 3
| `uri-reference` | `is_uri_reference` | RFC 3986 4.1
| `iri` | `is_iri` | RFC 3987 2.2
| `iri-reference` | `is_iri_reference` | RFC 3987 2.2
| `uri-template` | `is_uri_template` | RFC 6570 2
| `json-pointer` | `is_json_pointer` | RFC 6901 3
| `relative-json-pointer` | `is_relative_json_pointer` | draft-handrews-relative-json-pointer-01
| `uuid` | `is_uuid` | RFC 9562 4
| `regex` | `is_regex` | ECMA-262 `Pattern`, `u` mode
|===
```

`doc/errors.adoc`:

```
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

[#formats-errors]
== Errors

A validator answers `bool`: whether the string is in the format. A rule
answers a `grammar::result`, the text it matched or why it refused, and
formats has no error category of its own:

* `boost::urls::grammar::error::mismatch`: no text of the format starts
  where the rule was asked to match, so a `variant_rule` tries its next
  alternative;
* `boost::urls::grammar::error::invalid`: the format's text started there and
  broke one of its constraints, a month 13 or a leap second at the wrong
  minute;
* `boost::urls::grammar::error::leftover`, from `parse` on a whole string:
  the format's text ended before the string did;
* an error of `webcpp::idna`, from the rules of internationalized names: the
  reason UTS 46 or IDNA2008 refused the name.

formats raises no exception of its own, and builds without exceptions.
`WEBCPP_FORMATS_NO_EXCEPTIONS`, defined without exceptions or by the
developer, changes nothing in it: it exists so that a program can set it for
every webcpp library alike. Only a memory resource can throw, `std::bad_alloc`
from the default one, and only the functions that take a resource reach one.
```

`doc/testing.adoc`:

```
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

[#formats-testing]
== Testing

formats is tested natively, on wasm32-wasip2 and wasm32-wasip3 under
wasmtime, and on emscripten under node, from the superproject's root:

[source,bash]
----
b2 libs/formats/test
b2 toolset=clang-wasip2 testing.launcher=wasmtime libs/formats/test
b2 toolset=clang-wasip3 testing.launcher=wasmtime libs/formats/test
b2 toolset=emscripten libs/formats/test
----

Its {n-tests} tests are unit tests with Boost's lightweight_test: each
public header compiled alone, the configuration macro compiled twice, the
terminals every format shares, and what each validator allocates. The test
matrix of every lane is published at
https://webcpporg.github.io/webcpp/report/.
```

- [ ] **Step 7: Build the page and the reference.**

```bash
B2 -a --build-dir=bin/formats-skeleton-doc libs/formats/doc
```

Expected: exit 0, `libs/formats/doc/html/index.html` written, its reference
listing the macro `WEBCPP_FORMATS_NO_EXCEPTIONS` and the aliases
`grammar::text` and `grammar::result`.

- [ ] **Step 8: The lint.** `LINT`. Expected: `lint: clean`. (The include
  boundaries are seen failing in Task 6, where `<srell.hpp>` is on an
  include path; planted here, the line would also fail clang-tidy's
  compile.)

- [ ] **Step 9: Commit** in `~/dev/webcpp-formats/libs/formats`:

```bash
git add build.jam LICENSE_1_0.txt .gitignore .gitattributes README.md AGENTS.md \
  .github/workflows/ci.yml meta/libraries.json meta/include-boundaries.json \
  include/webcpp/formats.hpp include/webcpp/formats/config.hpp include/webcpp/formats/grammar.hpp \
  include/webcpp/formats/detail/utf8.hpp include/webcpp/formats/detail/charsets.hpp \
  include/webcpp/formats/detail/rules.hpp test/Jamfile test/.clang-tidy test/require.hpp \
  test/counting_resource.hpp test/config_test.cpp test/detail_test.cpp test/allocation_test.cpp \
  doc/Jamfile doc/mrdocs.yml doc/formats.adoc doc/introduction.adoc doc/errors.adoc doc/testing.adoc
git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "formats: the library's skeleton, its configuration, and the terminals every format shares"
```

The report's Changelog: the "New Libraries" line of Global Constraints.

### Task 2: Dates and times (F1) ∥

Worktree slug `dates`, `<base>` = `B0` (or the latest base when it starts). Needs Task 1 only; its suite slice
runs in Task 4. ∥ Tasks 3, 5, 6.

**Files:**
- Create: `include/webcpp/formats/date_time.hpp`, `test/date_time_test.cpp`, `doc/dates.adoc`
- Modify: `include/webcpp/formats.hpp` (one include), `test/Jamfile` (one line), `test/allocation_test.cpp` (one function and its call), `doc/formats.adoc` (one include, after `introduction.adoc`)

**Interfaces:**
- Consumes: Task 1's `grammar::text`, `grammar::result`, `detail::exactly<N>`.
- Produces (the spec's section 4, frozen once this lands):

```cpp
namespace webcpp::formats {
enum class leap_seconds { allowed, refused };
[[nodiscard]] bool is_date_time(std::string_view s) noexcept;
[[nodiscard]] bool is_date(std::string_view s) noexcept;
[[nodiscard]] bool is_time(std::string_view s, leap_seconds leap = leap_seconds::allowed) noexcept;
[[nodiscard]] bool is_duration(std::string_view s) noexcept;
}
namespace webcpp::formats::grammar {
struct date_rule_t;        // value_type text
struct time_rule_t;        // value_type text; leap_seconds leap = leap_seconds::allowed;
struct date_time_rule_t;   // value_type text
struct duration_rule_t;    // value_type text
inline constexpr date_rule_t date_rule{};
inline constexpr time_rule_t time_rule{};
inline constexpr date_time_rule_t date_time_rule{};
inline constexpr duration_rule_t duration_rule{};
}
namespace webcpp::formats::detail {
constexpr int days_in_month(int year, int month) noexcept;
}
```

- [ ] **Step 1: Write the failing test** `test/date_time_test.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// RFC 3339's dates, times and durations: its examples, every month's length, the Gregorian leap
// years, leap seconds at each offset under both policies, the case of T and Z, the fraction's
// length, appendix A's durations, and where each rule stops inside a larger grammar.

#include <webcpp/formats/date_time.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/url/grammar/delim_rule.hpp>
#include <boost/url/grammar/error.hpp>
#include <boost/url/grammar/parse.hpp>
#include <boost/url/grammar/tuple_rule.hpp>

#include <array>
#include <initializer_list>
#include <iostream>
#include <string>
#include <string_view>
#include <tuple>

namespace formats = webcpp::formats;
namespace fg = webcpp::formats::grammar;
namespace urlg = boost::urls::grammar;

namespace {

/** A string and whether the format holds it. */
struct sample {
    std::string_view text;
    bool valid;
};

/** Checks each sample with `is`, and that `is` and `rule` agree on it. */
template <class Is, class Rule>
void check(std::string_view name, std::initializer_list<sample> samples, Is is, const Rule& rule) {
    for (const sample& s : samples) {
        if (!BOOST_TEST_EQ(is(s.text), s.valid)) {
            std::cerr << name << ": \"" << s.text << "\"\n";
        }
        BOOST_TEST_EQ(is(s.text), urlg::parse(s.text, rule).has_value());
    }
}

/** `n` written with two digits, "07" for 7. */
std::string two_digits(int n) {
    return (n < 10 ? "0" : "") + std::to_string(n);
}

void rfc_3339_examples() {
    const auto is = [](std::string_view s) { return formats::is_date_time(s); };
    check("RFC 3339 5.8",
          {{"1985-04-12T23:20:50.52Z", true},
           {"1996-12-19T16:39:57-08:00", true},
           {"1990-12-31T23:59:60Z", true},
           {"1990-12-31T15:59:60-08:00", true},
           {"1937-01-01T12:00:27.87+00:20", true}},
          is, fg::date_time_rule);
}

void every_month_ends_where_it_should() {
    constexpr std::array<int, 12> last{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    for (int month = 1; month <= 12; ++month) {
        const int days = last[static_cast<std::size_t>(month - 1)];
        BOOST_TEST(formats::is_date("2021-" + two_digits(month) + "-" + two_digits(days)));
        BOOST_TEST(!formats::is_date("2021-" + two_digits(month) + "-" + two_digits(days + 1)));
        BOOST_TEST_EQ(webcpp::formats::detail::days_in_month(2021, month), days);
    }
    for (const std::string_view refused : {"2021-00-01", "2021-13-01", "2021-01-00"}) {
        BOOST_TEST(!formats::is_date(refused));
    }
}

void february_29_in_gregorian_leap_years_only() {
    const auto is = [](std::string_view s) { return formats::is_date(s); };
    check("leap years",
          {{"0400-02-29", true},
           {"2000-02-29", true},
           {"2020-02-29", true},
           {"0100-02-29", false},
           {"2100-02-29", false},
           {"2021-02-29", false}},
          is, fg::date_rule);
}

void leap_seconds_at_each_offset() {
    const auto allowed = [](std::string_view s) { return formats::is_time(s); };
    check("leap seconds allowed",
          {{"23:59:60Z", true},
           {"23:59:60+00:00", true},
           {"23:59:60-00:00", true},
           {"15:59:60-08:00", true},
           {"00:29:60-23:30", true},
           {"01:29:60+01:30", true},
           {"23:29:60+23:30", true},
           {"23:59:60+01:00", false},
           {"23:59:60-00:30", false},
           {"22:59:60Z", false},
           {"23:58:60Z", false},
           {"23:59:61Z", false}},
          allowed, fg::time_rule);
    const auto refused = [](std::string_view s) {
        return formats::is_time(s, formats::leap_seconds::refused);
    };
    check("leap seconds refused",
          {{"23:59:60Z", false},
           {"15:59:60-08:00", false},
           {"00:29:60-23:30", false},
           {"23:59:59Z", true}},
          refused, fg::time_rule_t{formats::leap_seconds::refused});
}

void times_and_their_parts() {
    const auto is = [](std::string_view s) { return formats::is_time(s); };
    check("times",
          {{"08:30:06z", true},
           {"08:30:06.1Z", true},
           {"08:30:06.123456789012345678901234567890Z", true},
           {"00:00:00+23:59", true},
           {"24:00:00Z", false},
           {"00:60:00Z", false},
           {"01:02:03+24:00", false},
           {"01:02:03+00:60", false},
           {"08:30:06.Z", false},
           {"08:30:06+01", false},
           {"08:30:06+0130", false},
           {"12:00:00", false},
           {"8:30:06Z", false},
           {"08:30:06Z ", false}},
          is, fg::time_rule);
}

void date_times_in_either_case() {
    const auto is = [](std::string_view s) { return formats::is_date_time(s); };
    check("date-times",
          {{"1963-06-19t08:30:06.283185z", true},
           {"1963-06-19T08:30:06Z", true},
           {"1963-06-19 08:30:06Z", false},
           {"1963-06-19T08:30Z", false},
           {"2021-02-29T00:00:00Z", false},
           {"1963-06-19T08:30:06Ztail", false}},
          is, fg::date_time_rule);
}

void durations_of_appendix_a() {
    const auto is = [](std::string_view s) { return formats::is_duration(s); };
    check("durations",
          {{"P1Y", true},
           {"P1M", true},
           {"P1W", true},
           {"P1D", true},
           {"PT1H", true},
           {"PT1M", true},
           {"PT1S", true},
           {"P1Y2M3DT4H5M6S", true},
           {"P1DT12H", true},
           {"PT1H30M", true},
           {"P0D", true},
           {"P", false},
           {"PT", false},
           {"P1H", false},
           {"PT1D", false},
           {"P1Y1W", false},
           {"P1WT1H", false},
           {"P1Y2D", false},
           {"PT1H2S", false},
           {"P1.5D", false},
           {"P-1D", false},
           {"-P1D", false},
           {"P1D1Y", false}},
          is, fg::duration_rule);
}

void duration_designators_in_either_case() {
    // RFC 5234 2.3: the quoted letters of RFC 3339 appendix A's ABNF are case-insensitive.
    const auto is = [](std::string_view s) { return formats::is_duration(s); };
    check("lowercase designators",
          {{"p1d", true}, {"pt1h", true}, {"P1y2m3dT4h5m6s", true}, {"p1w", true}},
          is, fg::duration_rule);
}

void nul_is_no_part_of_a_date_or_a_time() {
    BOOST_TEST(!formats::is_date(std::string_view("2020-01-01\0", 11)));
    BOOST_TEST(!formats::is_time(std::string_view("12:00:00\0Z", 10)));
    BOOST_TEST(!formats::is_date_time(std::string_view("2020-01-01T12:00:00Z\0", 21)));
    BOOST_TEST(!formats::is_duration(std::string_view("P1\0D", 4)));
}

/** Checks that `rule` refuses `text` with `expected` and leaves the iterator where it was. */
template <class Rule>
void leaves_its_input(const Rule& rule, std::string_view text, urlg::error expected) {
    char const* it = text.data();
    const auto r = rule.parse(it, text.data() + text.size());
    if (!BOOST_TEST(r.has_error() && r.error() == expected)) {
        std::cerr << "refusal of \"" << text << "\"\n";
    }
    BOOST_TEST(it == text.data());
}

void a_rule_that_fails_leaves_its_input() {
    leaves_its_input(fg::date_rule, "x2021-01-01", urlg::error::mismatch);
    leaves_its_input(fg::date_rule, "2021-02-29", urlg::error::invalid);
    leaves_its_input(fg::date_rule, "2021-1-01", urlg::error::invalid);
    leaves_its_input(fg::time_rule, "T12:00:00Z", urlg::error::mismatch);
    leaves_its_input(fg::time_rule, "12:00:00", urlg::error::invalid);
    leaves_its_input(fg::time_rule_t{formats::leap_seconds::refused}, "23:59:60Z",
                     urlg::error::invalid);
    leaves_its_input(fg::date_time_rule, "2021-01-01X00:00:00Z", urlg::error::invalid);
    leaves_its_input(fg::duration_rule, "1D", urlg::error::mismatch);
    leaves_its_input(fg::duration_rule, "PT", urlg::error::invalid);
}

void rules_stop_where_their_grammar_does() {
    constexpr auto interval =
        urlg::tuple_rule(fg::date_rule, urlg::squelch(urlg::delim_rule('/')), fg::date_rule);
    const auto dates = urlg::parse("2020-01-01/2021-02-03", interval);
    if (BOOST_TEST(dates.has_value())) {
        BOOST_TEST_EQ(std::get<0>(*dates), "2020-01-01");
        BOOST_TEST_EQ(std::get<1>(*dates), "2021-02-03");
    }
    const auto timed = urlg::parse("08:30:06Z/P1D", urlg::tuple_rule(fg::time_rule,
        urlg::squelch(urlg::delim_rule('/')), fg::duration_rule));
    if (BOOST_TEST(timed.has_value())) {
        BOOST_TEST_EQ(std::get<0>(*timed), "08:30:06Z");
        BOOST_TEST_EQ(std::get<1>(*timed), "P1D");
    }
    const auto leftover = urlg::parse("2020-01-01x", fg::date_rule);
    BOOST_TEST(leftover.has_error() && leftover.error() == urlg::error::leftover);
}

}  // namespace

int main() {
    rfc_3339_examples();
    every_month_ends_where_it_should();
    february_29_in_gregorian_leap_years_only();
    leap_seconds_at_each_offset();
    times_and_their_parts();
    date_times_in_either_case();
    durations_of_appendix_a();
    duration_designators_in_either_case();
    nul_is_no_part_of_a_date_or_a_time();
    a_rule_that_fails_leaves_its_input();
    rules_stop_where_their_grammar_does();
    return boost::report_errors();
}
```

`test/Jamfile`, after `detail`:

```
webcpp.run date_time : date_time_test.cpp ;
```

`test/allocation_test.cpp` gains, with `#include <webcpp/formats/date_time.hpp>`
and its call in `main`:

```cpp
/** Dates, times and durations allocate nothing, a fraction or a number of 100,000 digits included. */
void dates_and_times_allocate_nothing() {
    std::string fraction = "1985-04-12T23:20:50.";
    fraction.append(100'000, '9');
    fraction += 'Z';
    std::string duration = "P";
    duration.append(100'000, '7');
    duration += 'D';
    const std::size_t before = global_allocations();
    BOOST_TEST(webcpp::formats::is_date_time(fraction));
    BOOST_TEST(webcpp::formats::is_duration(duration));
    BOOST_TEST(webcpp::formats::is_date("2020-02-29"));
    BOOST_TEST(webcpp::formats::is_time("23:59:60Z"));
    BOOST_TEST(!webcpp::formats::is_time("23:59:60Z", webcpp::formats::leap_seconds::refused));
    BOOST_TEST_EQ(global_allocations() - before, 0U);
}
```

- [ ] **Step 2: Run it and watch it fail.**

```bash
B2 --build-dir=bin/formats-dates-native libs/formats/test//date_time
```

Expected: FAIL, `fatal error: 'webcpp/formats/date_time.hpp' file not found`.

- [ ] **Step 3: Write the header** `include/webcpp/formats/date_time.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 Dates, times and durations: RFC 3339's `date-time`, `full-date`, `full-time` and appendix A's
 `duration`, as validators and as Boost.URL rules.

 @see "Dates and times", in the guide.
*/
#ifndef WEBCPP_FORMATS_DATE_TIME_HPP
#define WEBCPP_FORMATS_DATE_TIME_HPP

#include <webcpp/formats/config.hpp>

#include <webcpp/formats/detail/rules.hpp>
#include <webcpp/formats/grammar.hpp>

#include <boost/url/grammar/delim_rule.hpp>
#include <boost/url/grammar/digit_chars.hpp>
#include <boost/url/grammar/error.hpp>
#include <boost/url/grammar/lut_chars.hpp>
#include <boost/url/grammar/optional_rule.hpp>
#include <boost/url/grammar/parse.hpp>
#include <boost/url/grammar/token_rule.hpp>
#include <boost/url/grammar/tuple_rule.hpp>
#include <boost/url/grammar/variant_rule.hpp>

#include <array>
#include <cstddef>
#include <string_view>

namespace webcpp::formats {

/**
 Whether a time may hold second 60, a leap second.

 @see "Dates and times", in the guide.
*/
enum class leap_seconds {
    /** Second 60 is valid when the time moved to UTC is 23:59, as RFC 3339 5.7 allows. */
    allowed,
    /** Second 60 is never valid, as JSON Schema v1's `time` asks. */
    refused,
};

namespace detail {

namespace urlg = boost::urls::grammar;

/** The value of a short run of ASCII digits: a year, a month, an hour. */
constexpr int digits_value(grammar::text digits) noexcept {
    int value = 0;
    for (const char digit : digits) {
        value = value * 10 + (digit - '0');
    }
    return value;
}

/** Whether `year` is a Gregorian leap year: divisible by 4, and not by 100 unless by 400. */
constexpr bool is_leap_year(int year) noexcept {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

/** The days of `month` (1 to 12) in `year`, Gregorian (RFC 3339 5.7). */
constexpr int days_in_month(int year, int month) noexcept {
    constexpr std::array<int, 12> days{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && is_leap_year(year)) {
        return 29;
    }
    return days[static_cast<std::size_t>(month - 1)];
}

/** Whether hh:mm:ss, `offset` minutes east of UTC, is a time of RFC 3339 5.6 and 5.7. */
constexpr bool is_valid_time(int hour, int minute, int second, int offset,
                             leap_seconds leap) noexcept {
    if (hour > 23 || minute > 59 || second > 60) {
        return false;
    }
    if (second < 60) {
        return true;
    }
    if (leap == leap_seconds::refused) {
        return false;
    }
    constexpr int minutes_per_day = 24 * 60;
    const int utc =
        ((hour * 60 + minute - offset) % minutes_per_day + minutes_per_day) % minutes_per_day;
    return utc == 23 * 60 + 59;
}

/** Two ASCII digits. */
inline constexpr auto two_digits_rule = exactly<2>(urlg::digit_chars);

/** RFC 3339 5.6 full-date's shape, 4DIGIT "-" 2DIGIT "-" 2DIGIT: its year, month and day. */
inline constexpr auto full_date_shape =
    urlg::tuple_rule(exactly<4>(urlg::digit_chars), urlg::squelch(urlg::delim_rule('-')),
                     two_digits_rule, urlg::squelch(urlg::delim_rule('-')), two_digits_rule);

/** RFC 3339 5.6 partial-time's shape, hh ":" mm ":" ss ["." 1*DIGIT]: its hour, minute and second. */
inline constexpr auto partial_time_shape = urlg::tuple_rule(
    two_digits_rule, urlg::squelch(urlg::delim_rule(':')), two_digits_rule,
    urlg::squelch(urlg::delim_rule(':')), two_digits_rule,
    urlg::squelch(urlg::optional_rule(urlg::tuple_rule(urlg::squelch(urlg::delim_rule('.')),
                                                       urlg::token_rule(urlg::digit_chars)))));

/** RFC 3339 5.6 time-numoffset's shape, ("+" / "-") hh ":" mm: its sign, hours and minutes. */
inline constexpr auto numeric_offset_shape =
    urlg::tuple_rule(urlg::delim_rule(urlg::lut_chars("+-")), two_digits_rule,
                     urlg::squelch(urlg::delim_rule(':')), two_digits_rule);

/** Reads RFC 3339's time-offset at `it` as minutes east of UTC; false, `it` unchanged, when none. */
inline bool parse_time_offset(char const*& it, char const* end, int& minutes) noexcept {
    if (it != end && (*it == 'Z' || *it == 'z')) {
        ++it;
        minutes = 0;
        return true;
    }
    char const* const first = it;
    const auto offset = urlg::parse(it, end, numeric_offset_shape);
    if (!offset) {
        it = first;
        return false;
    }
    const auto& [sign, hours_text, minutes_text] = *offset;
    const int hours = digits_value(hours_text);
    const int mins = digits_value(minutes_text);
    if (hours > 23 || mins > 59) {
        it = first;
        return false;
    }
    minutes = (sign == "-" ? -1 : 1) * (hours * 60 + mins);
    return true;
}

/** One designator of a duration, in either case (RFC 5234 2.3), matched and dropped. */
constexpr auto designator(char upper) noexcept {
    return urlg::squelch(urlg::delim_rule(urlg::lut_chars(upper) +
                                          urlg::lut_chars(static_cast<char>(upper - 'A' + 'a'))));
}

/** 1*DIGIT, any length: a duration's numbers are never converted, so none overflows. */
inline constexpr auto duration_digits_rule = urlg::token_rule(urlg::digit_chars);

// RFC 3339 appendix A, one rule per production, in the ABNF's order.
inline constexpr auto dur_second = urlg::tuple_rule(duration_digits_rule, designator('S'));
inline constexpr auto dur_minute =
    urlg::tuple_rule(duration_digits_rule, designator('M'), urlg::optional_rule(dur_second));
inline constexpr auto dur_hour =
    urlg::tuple_rule(duration_digits_rule, designator('H'), urlg::optional_rule(dur_minute));
inline constexpr auto dur_time =
    urlg::tuple_rule(designator('T'), urlg::variant_rule(dur_hour, dur_minute, dur_second));
inline constexpr auto dur_day = urlg::tuple_rule(duration_digits_rule, designator('D'));
inline constexpr auto dur_week = urlg::tuple_rule(duration_digits_rule, designator('W'));
inline constexpr auto dur_month =
    urlg::tuple_rule(duration_digits_rule, designator('M'), urlg::optional_rule(dur_day));
inline constexpr auto dur_year =
    urlg::tuple_rule(duration_digits_rule, designator('Y'), urlg::optional_rule(dur_month));
inline constexpr auto dur_date =
    urlg::tuple_rule(urlg::variant_rule(dur_day, dur_month, dur_year), urlg::optional_rule(dur_time));

/** RFC 3339 appendix A's duration: "P" (dur-date / dur-time / dur-week). */
inline constexpr auto duration_shape =
    urlg::tuple_rule(designator('P'), urlg::variant_rule(dur_date, dur_time, dur_week));

}  // namespace detail

namespace grammar {

/**
 The rule of RFC 3339 5.6's `full-date`: a year, a month and a day that exists.

 `4DIGIT "-" 2DIGIT "-" 2DIGIT`, ASCII digits only; the month 01 to 12, the day 01 to the month's
 length, February 29 only in a Gregorian leap year. It stops after the day.

 @see "Dates and times", in the guide.
*/
struct date_rule_t {
    /** The rule yields the text it matched. */
    using value_type = text;

    /**
     Matches a `full-date` at `it`.

     @param it The start of the input: past the match on success, else unchanged.
     @param end The end of the input.
     @return The text matched; `error::mismatch` when no digit starts it; `error::invalid` when it
     started and broke a constraint (Boost.URL's grammar errors).
    */
    [[nodiscard]] result<text> parse(char const*& it, char const* end) const noexcept;
};

/**
 The rule of RFC 3339 5.6's `full-time`: a time of day and its offset from UTC.

 Hours 00 to 23, minutes 00 to 59, seconds 00 to 60 with any number of fraction digits, then `Z`
 or `+hh:mm` or `-hh:mm` (hh 00 to 23, mm 00 to 59); `Z` in either case. Second 60 is valid only
 under @ref leap_seconds::allowed and when the time moved to UTC is 23:59. It stops after the
 offset.

 @see "Dates and times", in the guide.
*/
struct time_rule_t {
    /** The rule yields the text it matched. */
    using value_type = text;

    /** Whether second 60 may be valid. */
    leap_seconds leap = leap_seconds::allowed;

    /**
     Matches a `full-time` at `it`.

     @param it The start of the input: past the match on success, else unchanged.
     @param end The end of the input.
     @return The text matched; `error::mismatch` when no digit starts it; `error::invalid` when it
     started and broke a constraint.
    */
    [[nodiscard]] result<text> parse(char const*& it, char const* end) const noexcept;
};

/**
 The rule of RFC 3339 5.6's `date-time`: a `full-date`, `T` in either case, a `full-time`.

 Leap seconds are allowed, as every draft of JSON Schema, v1 included, says of `date-time`. It
 stops after the offset.

 @see "Dates and times", in the guide.
*/
struct date_time_rule_t {
    /** The rule yields the text it matched. */
    using value_type = text;

    /**
     Matches a `date-time` at `it`.

     @param it The start of the input: past the match on success, else unchanged.
     @param end The end of the input.
     @return The text matched; `error::mismatch` when no digit starts it; `error::invalid` when it
     started and broke a constraint.
    */
    [[nodiscard]] result<text> parse(char const*& it, char const* end) const noexcept;
};

/**
 The rule of RFC 3339 appendix A's `duration`, exactly: `P`, then a date part, a time part, or
 weeks alone.

 Years, months and days in that order and each optional after the first given, then `T` and
 hours, minutes and seconds likewise; or a number of weeks alone. No fraction, no sign, any
 number of digits. The designators are letters of the ABNF, so either case matches (RFC 5234
 2.3). It stops where the ABNF does: after `P1Y`, the `2D` of `P1Y2D` is not its.

 @see "Dates and times", in the guide.
*/
struct duration_rule_t {
    /** The rule yields the text it matched. */
    using value_type = text;

    /**
     Matches a `duration` at `it`.

     @param it The start of the input: past the match on success, else unchanged.
     @param end The end of the input.
     @return The text matched; `error::mismatch` when no `P` starts it; `error::invalid` when it
     started and broke the grammar.
    */
    [[nodiscard]] result<text> parse(char const*& it, char const* end) const noexcept;
};

/** The rule of RFC 3339's `full-date`. */
inline constexpr date_rule_t date_rule{};

/** The rule of RFC 3339's `full-time`, leap seconds allowed; `time_rule_t{leap_seconds::refused}` refuses them. */
inline constexpr time_rule_t time_rule{};

/** The rule of RFC 3339's `date-time`. */
inline constexpr date_time_rule_t date_time_rule{};

/** The rule of RFC 3339 appendix A's `duration`. */
inline constexpr duration_rule_t duration_rule{};

inline result<text> date_rule_t::parse(char const*& it, char const* end) const noexcept {
    if (it == end || !detail::urlg::digit_chars(*it)) {
        return detail::urlg::error::mismatch;
    }
    char const* const first = it;
    const auto date = detail::urlg::parse(it, end, detail::full_date_shape);
    if (!date) {
        it = first;
        return detail::urlg::error::invalid;
    }
    const auto& [year_text, month_text, day_text] = *date;
    const int year = detail::digits_value(year_text);
    const int month = detail::digits_value(month_text);
    const int day = detail::digits_value(day_text);
    if (month < 1 || month > 12 || day < 1 || day > detail::days_in_month(year, month)) {
        it = first;
        return detail::urlg::error::invalid;
    }
    return text(first, static_cast<std::size_t>(it - first));
}

inline result<text> time_rule_t::parse(char const*& it, char const* end) const noexcept {
    if (it == end || !detail::urlg::digit_chars(*it)) {
        return detail::urlg::error::mismatch;
    }
    char const* const first = it;
    const auto time = detail::urlg::parse(it, end, detail::partial_time_shape);
    int offset = 0;
    if (!time || !detail::parse_time_offset(it, end, offset)) {
        it = first;
        return detail::urlg::error::invalid;
    }
    const auto& [hour, minute, second] = *time;
    if (!detail::is_valid_time(detail::digits_value(hour), detail::digits_value(minute),
                               detail::digits_value(second), offset, leap)) {
        it = first;
        return detail::urlg::error::invalid;
    }
    return text(first, static_cast<std::size_t>(it - first));
}

inline result<text> date_time_rule_t::parse(char const*& it, char const* end) const noexcept {
    char const* const first = it;
    if (const auto date = date_rule.parse(it, end); !date) {
        return date.error();
    }
    if (it == end || (*it != 'T' && *it != 't')) {
        it = first;
        return detail::urlg::error::invalid;
    }
    ++it;
    if (const auto time = time_rule.parse(it, end); !time) {
        it = first;
        return detail::urlg::error::invalid;
    }
    return text(first, static_cast<std::size_t>(it - first));
}

inline result<text> duration_rule_t::parse(char const*& it, char const* end) const noexcept {
    if (it == end || (*it != 'P' && *it != 'p')) {
        return detail::urlg::error::mismatch;
    }
    char const* const first = it;
    if (const auto duration = detail::urlg::parse(it, end, detail::duration_shape); !duration) {
        it = first;
        return detail::urlg::error::invalid;
    }
    return text(first, static_cast<std::size_t>(it - first));
}

}  // namespace grammar

/**
 Whether `s` is an RFC 3339 `date-time`, JSON Schema's `date-time`.

 @param s Any byte string.
 @return `parse(s, grammar::date_time_rule).has_value()`.
 @see "Dates and times", in the guide.
*/
[[nodiscard]] inline bool is_date_time(std::string_view s) noexcept {
    return detail::urlg::parse(s, grammar::date_time_rule).has_value();
}

/**
 Whether `s` is an RFC 3339 `full-date`, JSON Schema's `date`.

 @param s Any byte string.
 @return `parse(s, grammar::date_rule).has_value()`.
 @see "Dates and times", in the guide.
*/
[[nodiscard]] inline bool is_date(std::string_view s) noexcept {
    return detail::urlg::parse(s, grammar::date_rule).has_value();
}

/**
 Whether `s` is an RFC 3339 `full-time`, JSON Schema's `time`.

 @param s Any byte string.
 @param leap Whether second 60 may be valid: @ref leap_seconds::allowed for drafts 7 to 2020-12,
 @ref leap_seconds::refused for v1.
 @return `parse(s, grammar::time_rule_t{leap}).has_value()`.
 @see "Dates and times", in the guide.
*/
[[nodiscard]] inline bool is_time(std::string_view s,
                                  leap_seconds leap = leap_seconds::allowed) noexcept {
    return detail::urlg::parse(s, grammar::time_rule_t{leap}).has_value();
}

/**
 Whether `s` is an RFC 3339 appendix A `duration`, JSON Schema's `duration`.

 @param s Any byte string.
 @return `parse(s, grammar::duration_rule).has_value()`.
 @see "Dates and times", in the guide.
*/
[[nodiscard]] inline bool is_duration(std::string_view s) noexcept {
    return detail::urlg::parse(s, grammar::duration_rule).has_value();
}

}  // namespace webcpp::formats

#endif  // WEBCPP_FORMATS_DATE_TIME_HPP
```

`include/webcpp/formats.hpp` gains `#include <webcpp/formats/date_time.hpp>`
after `config.hpp`, the format headers sorted by name.

- [ ] **Step 4: Run it and watch it pass.**
  `B2 --build-dir=bin/formats-dates-native libs/formats/test//date_time`.
  Expected: PASS. Then see the leap-second guard fail: in `is_valid_time`,
  replace `hour * 60 + minute - offset` by `hour * 60 + minute` and run the
  test again. Expected: FAIL, the standard error naming among others
  `leap seconds allowed: "15:59:60-08:00"` and `"23:59:60+01:00"`. Restore the line. Then the five lanes with `<t>` = `dates`:
  every lane green, `date_time` and `allocation` passed.

- [ ] **Step 5: The page.** `doc/dates.adoc`, included after
  `introduction.adoc` in `doc/formats.adoc`:

```
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

[#formats-dates]
== Dates and times

`is_date_time`, `is_date` and `is_time` check RFC 3339's `date-time`,
`full-date` and `full-time` (section 5.6), and `is_duration` its appendix A's
`duration`, exactly: ASCII digits only, a month 01 to 12, a day that exists
in its month of the Gregorian calendar (February 29 in a year divisible by 4,
and not by 100 unless by 400), hours 00 to 23, minutes 00 to 59, any number
of fraction digits, and an offset of `Z` or `+hh:mm`/`-hh:mm`. `T` and `Z`
match in either case, as RFC 3339's note says; so do the letters of a
duration, which are ABNF's quoted strings (RFC 5234 2.3).

Second 60 is a leap second. RFC 3339 allows it when the time, moved to UTC
by its offset, is 23:59, and `is_time(s)` follows it, as drafts 7 to 2020-12
of JSON Schema test. JSON Schema v1 says a `time` must not hold one:
`is_time(s, leap_seconds::refused)` is its `time`. `is_date_time` allows them
in every draft, v1 included, as the suite's `date-time` tests say. The date
of a leap second is not checked against IERS's list: no table, the same
answer every year.

A duration's numbers are never converted, so any number of digits is valid
and nothing overflows. `P1Y2D` is invalid: appendix A keeps years, months
and days in that order and contiguous, and a week stands alone.
```

- [ ] **Step 6: Build the page, run the lint.**
  `B2 -a --build-dir=bin/formats-dates-doc libs/formats/doc` exits 0 and
  the reference lists `leap_seconds` with its two enumerators, the four
  rule types and objects, and the four functions. `LINT`: `lint: clean`.

- [ ] **Step 7: Commit** in `~/dev/webcpp-formats-dates/libs/formats`:

```bash
git add include/webcpp/formats/date_time.hpp include/webcpp/formats.hpp test/date_time_test.cpp \
  test/Jamfile test/allocation_test.cpp doc/dates.adoc doc/formats.adoc
git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "formats: dates, times and durations of RFC 3339, with the leap-second policy of each draft"
```

The report's Changelog: the "New Libraries" line, unchanged.

### Task 3: UUID and JSON Pointers (F2) ∥

Worktree slug `ids`, `<base>` = `B0` (or the latest base when it starts). Needs Task 1 only; its suite slice
runs in Task 4. ∥ Tasks 2, 5, 6.

**Files:**
- Create: `include/webcpp/formats/uuid.hpp`, `include/webcpp/formats/json_pointer.hpp`, `test/uuid_test.cpp`, `test/json_pointer_test.cpp`, `doc/identifiers.adoc`
- Modify: `include/webcpp/formats.hpp` (two includes), `test/Jamfile` (two lines), `test/allocation_test.cpp` (one function and its call), `doc/formats.adoc` (one include, after `dates.adoc`'s place)

**Interfaces:**
- Consumes: Task 1's `grammar::text`, `grammar::result`, `detail::exactly<N>`, `detail::decode_utf8`.
- Produces:

```cpp
namespace webcpp::formats {
[[nodiscard]] bool is_uuid(std::string_view s) noexcept;
[[nodiscard]] bool is_json_pointer(std::string_view s) noexcept;
[[nodiscard]] bool is_relative_json_pointer(std::string_view s) noexcept;
}
namespace webcpp::formats::grammar {
struct uuid_rule_t;                    // value_type text
struct json_pointer_rule_t;            // value_type text
struct relative_json_pointer_rule_t;   // value_type text
inline constexpr uuid_rule_t uuid_rule{};
inline constexpr json_pointer_rule_t json_pointer_rule{};
inline constexpr relative_json_pointer_rule_t relative_json_pointer_rule{};
}
```

- [ ] **Step 1: Write the failing tests.** `test/uuid_test.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// RFC 9562 section 4's UUID: the canonical forms in any case, every wrong length and dash
// position, braces and the urn:uuid: prefix refused, and the rule inside a larger grammar.

#include <webcpp/formats/uuid.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/url/grammar/delim_rule.hpp>
#include <boost/url/grammar/error.hpp>
#include <boost/url/grammar/parse.hpp>
#include <boost/url/grammar/tuple_rule.hpp>

#include <iostream>
#include <string>
#include <string_view>

namespace formats = webcpp::formats;
namespace fg = webcpp::formats::grammar;
namespace urlg = boost::urls::grammar;

namespace {

/** Checks `text` against `valid`, and that the function and the rule agree. */
void check(std::string_view text, bool valid) {
    if (!BOOST_TEST_EQ(formats::is_uuid(text), valid)) {
        std::cerr << "uuid: \"" << text << "\"\n";
    }
    BOOST_TEST_EQ(formats::is_uuid(text), urlg::parse(text, fg::uuid_rule).has_value());
}

void canonical_forms_in_any_case() {
    check("2eb8aa08-aa98-11ea-b4aa-73b441d16380", true);
    check("2EB8AA08-AA98-11EA-B4AA-73B441D16380", true);
    check("2eb8aa08-AA98-11ea-B4Aa-73B441D16380", true);
    check("00000000-0000-0000-0000-000000000000", true);
    // The version and variant nibbles are not checked: the suite accepts version 15.
    check("99c17cbb-656f-f64a-940f-1a4568f03487", true);
}

void every_wrong_length_and_dash() {
    const std::string canonical = "2eb8aa08-aa98-11ea-b4aa-73b441d16380";
    for (std::size_t i = 0; i < canonical.size(); ++i) {
        std::string shorter = canonical;
        shorter.erase(i, 1);
        check(shorter, false);
        std::string longer = canonical;
        longer.insert(i, 1, canonical[i] == '-' ? '-' : 'a');
        check(longer, false);
    }
    check("2eb8aa08aa9811eab4aa73b441d16380", false);
    check("2eb8aa0-8aa98-11e-ab4aa7-3b441d16380", false);
    check("2eb8aa08-aa98-11ea-b4ga-73b441d16380", false);
}

void braces_prefixes_and_nul_are_refused() {
    check("{2eb8aa08-aa98-11ea-b4aa-73b441d16380}", false);
    check("urn:uuid:2eb8aa08-aa98-11ea-b4aa-73b441d16380", false);
    check(std::string_view("2eb8aa08-aa98-11ea-b4aa-73b441d16380\0", 37), false);
    check(std::string_view("2eb8aa08-aa98-11ea-b4aa-73b441d1\0" "380", 36), false);
}

void the_rule_inside_a_grammar() {
    const auto pair = urlg::parse(
        "2eb8aa08-aa98-11ea-b4aa-73b441d16380/00000000-0000-0000-0000-000000000000",
        urlg::tuple_rule(fg::uuid_rule, urlg::squelch(urlg::delim_rule('/')), fg::uuid_rule));
    BOOST_TEST(pair.has_value());
    const std::string_view broken = "2eb8aa08-aa98";
    char const* it = broken.data();
    const auto r = fg::uuid_rule.parse(it, broken.data() + broken.size());
    BOOST_TEST(r.has_error() && r.error() == urlg::error::invalid);
    BOOST_TEST(it == broken.data());
    const std::string_view other = "x2eb8aa08";
    char const* at = other.data();
    const auto m = fg::uuid_rule.parse(at, other.data() + other.size());
    BOOST_TEST(m.has_error() && m.error() == urlg::error::mismatch);
}

}  // namespace

int main() {
    canonical_forms_in_any_case();
    every_wrong_length_and_dash();
    braces_prefixes_and_nul_are_refused();
    the_rule_inside_a_grammar();
    return boost::report_errors();
}
```

`test/json_pointer_test.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// RFC 6901's JSON Pointer and draft-handrews-relative-json-pointer-01's relative one: RFC 6901
// section 5's twelve pointers, every escape, UTF-8 well-formed or refused, NUL as a character,
// and the draft's section 5.1 examples.

#include <webcpp/formats/json_pointer.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/url/grammar/error.hpp>
#include <boost/url/grammar/parse.hpp>

#include <iostream>
#include <string_view>

namespace formats = webcpp::formats;
namespace fg = webcpp::formats::grammar;
namespace urlg = boost::urls::grammar;

namespace {

void pointer(std::string_view text, bool valid) {
    if (!BOOST_TEST_EQ(formats::is_json_pointer(text), valid)) {
        std::cerr << "json-pointer: \"" << text << "\"\n";
    }
    BOOST_TEST_EQ(formats::is_json_pointer(text),
                  urlg::parse(text, fg::json_pointer_rule).has_value());
}

void relative(std::string_view text, bool valid) {
    if (!BOOST_TEST_EQ(formats::is_relative_json_pointer(text), valid)) {
        std::cerr << "relative-json-pointer: \"" << text << "\"\n";
    }
    BOOST_TEST_EQ(formats::is_relative_json_pointer(text),
                  urlg::parse(text, fg::relative_json_pointer_rule).has_value());
}

void rfc_6901_section_5() {
    for (const std::string_view text : {"", "/foo", "/foo/0", "/", "/a~1b", "/c%d", "/e^f", "/g|h",
                                        "/i\\j", "/k\"l", "/ ", "/m~0n"}) {
        pointer(text, true);
    }
}

void escapes() {
    pointer("/~0~1~0~1", true);
    pointer("/~01", true);
    pointer("/~2", false);
    pointer("/~", false);
    pointer("/foo~", false);
    pointer("/~a", false);
    pointer("foo", false);
    pointer("#/foo", false);
}

void utf8_and_nul() {
    pointer("/\xC3\xA9t\xC3\xA9", true);
    pointer("/\xF0\x9F\x98\x80", true);
    pointer("/\xC3", false);
    pointer("/\xC0\xAF", false);
    pointer("/\xED\xA0\x80", false);
    pointer("/a/\xFF", false);
    // RFC 6901's unescaped holds %x00: NUL is a character of a reference token.
    pointer(std::string_view("/a\0b", 4), true);
}

void relative_pointers_of_section_5_1() {
    for (const std::string_view text : {"0", "1/0", "2/highly/nested/objects", "0#", "1#",
                                        "0/objects", "1/nested/objects", "2/foo/0", "120/foo/bar",
                                        "0//"}) {
        relative(text, true);
    }
    for (const std::string_view text : {"", "/foo", "-1/foo", "+1/foo", "01/a", "01#", "0##",
                                        "1#/foo", "0/~2", "1foo", "0+1", "\xD9\xA1/foo"}) {
        relative(text, false);
    }
}

void where_the_rules_stop() {
    // A JSON Pointer holds every code point but its own separators, so inside a larger grammar
    // it runs to the end; a relative pointer's "#" ends it.
    const std::string_view text = "2#rest";
    char const* it = text.data();
    const auto r = fg::relative_json_pointer_rule.parse(it, text.data() + text.size());
    BOOST_TEST(r.has_value() && *r == "2#");
    const std::string_view broken = "/a~2";
    char const* at = broken.data();
    const auto p = fg::json_pointer_rule.parse(at, broken.data() + broken.size());
    BOOST_TEST(p.has_error() && p.error() == urlg::error::invalid);
    BOOST_TEST(at == broken.data());
    const std::string_view none = "x";
    char const* from = none.data();
    const auto m = fg::relative_json_pointer_rule.parse(from, none.data() + none.size());
    BOOST_TEST(m.has_error() && m.error() == urlg::error::mismatch);
}

}  // namespace

int main() {
    rfc_6901_section_5();
    escapes();
    utf8_and_nul();
    relative_pointers_of_section_5_1();
    where_the_rules_stop();
    return boost::report_errors();
}
```

`test/Jamfile`, after the date_time line's place:

```
webcpp.run uuid : uuid_test.cpp ;
webcpp.run json_pointer : json_pointer_test.cpp ;
```

`test/allocation_test.cpp` gains, with the two includes and its call:

```cpp
/** UUIDs and JSON Pointers allocate nothing, a pointer of 100,000 tokens included. */
void identifiers_allocate_nothing() {
    std::string pointer;
    for (int i = 0; i < 100'000; ++i) {
        pointer += "/\xC3\xA9~0";
    }
    const std::size_t before = global_allocations();
    BOOST_TEST(webcpp::formats::is_json_pointer(pointer));
    BOOST_TEST(webcpp::formats::is_relative_json_pointer("2/highly/nested/objects"));
    BOOST_TEST(webcpp::formats::is_uuid("2eb8aa08-aa98-11ea-b4aa-73b441d16380"));
    BOOST_TEST_EQ(global_allocations() - before, 0U);
}
```

- [ ] **Step 2: Run them and watch them fail.**

```bash
B2 --build-dir=bin/formats-ids-native libs/formats/test//uuid libs/formats/test//json_pointer
```

Expected: FAIL, `'webcpp/formats/uuid.hpp' file not found` and
`'webcpp/formats/json_pointer.hpp' file not found`.

- [ ] **Step 3: Write the headers.** `include/webcpp/formats/uuid.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 UUIDs, RFC 9562 section 4's string form, as a validator and a Boost.URL rule.

 @see "Identifiers", in the guide.
*/
#ifndef WEBCPP_FORMATS_UUID_HPP
#define WEBCPP_FORMATS_UUID_HPP

#include <webcpp/formats/config.hpp>

#include <webcpp/formats/detail/rules.hpp>
#include <webcpp/formats/grammar.hpp>

#include <boost/url/grammar/delim_rule.hpp>
#include <boost/url/grammar/error.hpp>
#include <boost/url/grammar/hexdig_chars.hpp>
#include <boost/url/grammar/parse.hpp>
#include <boost/url/grammar/tuple_rule.hpp>

#include <cstddef>
#include <string_view>

namespace webcpp::formats {

namespace detail {

namespace urlg = boost::urls::grammar;

/** RFC 9562 4's shape: 8HEXDIG "-" 4HEXDIG "-" 4HEXDIG "-" 4HEXDIG "-" 12HEXDIG, any case. */
inline constexpr auto uuid_shape = urlg::tuple_rule(
    exactly<8>(urlg::hexdig_chars), urlg::squelch(urlg::delim_rule('-')),
    exactly<4>(urlg::hexdig_chars), urlg::squelch(urlg::delim_rule('-')),
    exactly<4>(urlg::hexdig_chars), urlg::squelch(urlg::delim_rule('-')),
    exactly<4>(urlg::hexdig_chars), urlg::squelch(urlg::delim_rule('-')),
    exactly<12>(urlg::hexdig_chars));

}  // namespace detail

namespace grammar {

/**
 The rule of RFC 9562 section 4's UUID: 32 hexadecimal digits in groups of 8, 4, 4, 4 and 12,
 joined by hyphens.

 Either case; the version and variant nibbles are not checked; no braces and no `urn:uuid:`.
 It stops after the twelfth digit of the last group.

 @see "Identifiers", in the guide.
*/
struct uuid_rule_t {
    /** The rule yields the text it matched. */
    using value_type = text;

    /**
     Matches a UUID at `it`.

     @param it The start of the input: past the match on success, else unchanged.
     @param end The end of the input.
     @return The text matched; `error::mismatch` when no hexadecimal digit starts it;
     `error::invalid` when it started and broke the shape.
    */
    [[nodiscard]] result<text> parse(char const*& it, char const* end) const noexcept;
};

/** The rule of RFC 9562's UUID. */
inline constexpr uuid_rule_t uuid_rule{};

inline result<text> uuid_rule_t::parse(char const*& it, char const* end) const noexcept {
    if (it == end || !detail::urlg::hexdig_chars(*it)) {
        return detail::urlg::error::mismatch;
    }
    char const* const first = it;
    if (const auto uuid = detail::urlg::parse(it, end, detail::uuid_shape); !uuid) {
        it = first;
        return detail::urlg::error::invalid;
    }
    return text(first, static_cast<std::size_t>(it - first));
}

}  // namespace grammar

/**
 Whether `s` is a UUID in RFC 9562's string form, JSON Schema's `uuid`.

 @param s Any byte string.
 @return `parse(s, grammar::uuid_rule).has_value()`.
 @see "Identifiers", in the guide.
*/
[[nodiscard]] inline bool is_uuid(std::string_view s) noexcept {
    return detail::urlg::parse(s, grammar::uuid_rule).has_value();
}

}  // namespace webcpp::formats

#endif  // WEBCPP_FORMATS_UUID_HPP
```

`include/webcpp/formats/json_pointer.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 JSON Pointers, RFC 6901's and draft-handrews-relative-json-pointer-01's relative ones, as
 validators and Boost.URL rules.

 @see "Identifiers", in the guide.
*/
#ifndef WEBCPP_FORMATS_JSON_POINTER_HPP
#define WEBCPP_FORMATS_JSON_POINTER_HPP

#include <webcpp/formats/config.hpp>

#include <webcpp/formats/detail/utf8.hpp>
#include <webcpp/formats/grammar.hpp>

#include <boost/url/grammar/error.hpp>
#include <boost/url/grammar/parse.hpp>

#include <cstddef>
#include <optional>
#include <string_view>

namespace webcpp::formats {

namespace detail {

namespace urlg = boost::urls::grammar;

/**
 The length of the RFC 6901 json-pointer at the start of `s`: "/"-led reference tokens, each "~"
 followed by "0" or "1", every code point well-formed UTF-8; nothing when a token breaks a rule.
*/
constexpr std::optional<std::size_t> json_pointer_length(std::string_view s) noexcept {
    std::size_t i = 0;
    while (i < s.size() && s[i] == '/') {
        ++i;
        while (i < s.size() && s[i] != '/') {
            if (s[i] == '~') {
                if (i + 1 >= s.size() || (s[i + 1] != '0' && s[i + 1] != '1')) {
                    return std::nullopt;
                }
                i += 2;
                continue;
            }
            char32_t cp = 0;
            if (!decode_utf8(s, i, cp)) {
                return std::nullopt;
            }
        }
    }
    return i;
}

/**
 The length of the relative JSON pointer at the start of `s`, which starts with a digit: a
 non-negative integer without a leading zero, then "#" or a json-pointer; nothing when the
 pointer breaks a rule.
*/
constexpr std::optional<std::size_t> relative_json_pointer_length(std::string_view s) noexcept {
    std::size_t i = 1;
    if (s[0] != '0') {
        while (i < s.size() && s[i] >= '0' && s[i] <= '9') {
            ++i;
        }
    }
    if (i < s.size() && s[i] == '#') {
        return i + 1;
    }
    const std::optional<std::size_t> pointer = json_pointer_length(s.substr(i));
    if (!pointer) {
        return std::nullopt;
    }
    return i + *pointer;
}

}  // namespace detail

namespace grammar {

/**
 The rule of RFC 6901 section 3's json-pointer: zero or more reference tokens, each led by `/`.

 A token holds any code point but `/` and `~`, NUL included, and `~0` or `~1`; the input is
 decoded as UTF-8 and must be well-formed. The empty pointer is valid, so the rule matches empty
 text where no `/` starts the input, and never answers mismatch. Inside a larger grammar it runs
 to the end of the input, since a token holds every character the grammar could continue with.

 @see "Identifiers", in the guide.
*/
struct json_pointer_rule_t {
    /** The rule yields the text it matched. */
    using value_type = text;

    /**
     Matches a json-pointer at `it`.

     @param it The start of the input: past the match on success, else unchanged.
     @param end The end of the input.
     @return The text matched, empty when no `/` starts the input; `error::invalid` when a token
     holds a `~` that is not `~0` or `~1`, or ill-formed UTF-8.
    */
    [[nodiscard]] result<text> parse(char const*& it, char const* end) const noexcept;
};

/**
 The rule of draft-handrews-relative-json-pointer-01 section 3's relative-json-pointer: a
 non-negative integer, then `#` or a json-pointer.

 The integer is `0` or starts with 1 to 9, so `01` is refused; no index manipulation (`0+1`), as
 drafts 2019-09 to v1 cite this draft. It stops after the `#`, or where its json-pointer does.

 @see "Identifiers", in the guide.
*/
struct relative_json_pointer_rule_t {
    /** The rule yields the text it matched. */
    using value_type = text;

    /**
     Matches a relative-json-pointer at `it`.

     @param it The start of the input: past the match on success, else unchanged.
     @param end The end of the input.
     @return The text matched; `error::mismatch` when no digit starts it; `error::invalid` when
     its json-pointer breaks a rule.
    */
    [[nodiscard]] result<text> parse(char const*& it, char const* end) const noexcept;
};

/** The rule of RFC 6901's json-pointer. */
inline constexpr json_pointer_rule_t json_pointer_rule{};

/** The rule of the relative JSON pointer. */
inline constexpr relative_json_pointer_rule_t relative_json_pointer_rule{};

inline result<text> json_pointer_rule_t::parse(char const*& it, char const* end) const noexcept {
    const std::optional<std::size_t> length =
        detail::json_pointer_length(std::string_view(it, static_cast<std::size_t>(end - it)));
    if (!length) {
        return detail::urlg::error::invalid;
    }
    char const* const first = it;
    it += *length;
    return text(first, *length);
}

inline result<text> relative_json_pointer_rule_t::parse(char const*& it,
                                                        char const* end) const noexcept {
    if (it == end || *it < '0' || *it > '9') {
        return detail::urlg::error::mismatch;
    }
    const std::optional<std::size_t> length = detail::relative_json_pointer_length(
        std::string_view(it, static_cast<std::size_t>(end - it)));
    if (!length) {
        return detail::urlg::error::invalid;
    }
    char const* const first = it;
    it += *length;
    return text(first, *length);
}

}  // namespace grammar

/**
 Whether `s` is an RFC 6901 JSON Pointer, JSON Schema's `json-pointer`.

 @param s Any byte string.
 @return `parse(s, grammar::json_pointer_rule).has_value()`.
 @see "Identifiers", in the guide.
*/
[[nodiscard]] inline bool is_json_pointer(std::string_view s) noexcept {
    return detail::urlg::parse(s, grammar::json_pointer_rule).has_value();
}

/**
 Whether `s` is a relative JSON Pointer, JSON Schema's `relative-json-pointer`.

 @param s Any byte string.
 @return `parse(s, grammar::relative_json_pointer_rule).has_value()`.
 @see "Identifiers", in the guide.
*/
[[nodiscard]] inline bool is_relative_json_pointer(std::string_view s) noexcept {
    return detail::urlg::parse(s, grammar::relative_json_pointer_rule).has_value();
}

}  // namespace webcpp::formats

#endif  // WEBCPP_FORMATS_JSON_POINTER_HPP
```

`include/webcpp/formats.hpp` gains `#include <webcpp/formats/json_pointer.hpp>`
and `#include <webcpp/formats/uuid.hpp>`, sorted by name.

- [ ] **Step 4: Run them and watch them pass.**
  `B2 --build-dir=bin/formats-ids-native libs/formats/test//uuid libs/formats/test//json_pointer`:
  PASS. Then the five lanes with `<t>` = `ids`: every lane green, `uuid`,
  `json_pointer` and `allocation` passed.

- [ ] **Step 5: The page.** `doc/identifiers.adoc`, included after
  `dates.adoc`'s place in `doc/formats.adoc`:

```
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

[#formats-identifiers]
== Identifiers

`is_uuid` checks RFC 9562's string form of a UUID: 32 hexadecimal digits in
either case, in groups of 8, 4, 4, 4 and 12 joined by hyphens. It checks the
shape, not the version or the variant, as JSON Schema's suite asks; braces
and the `urn:uuid:` prefix are not part of the form.

`is_json_pointer` checks RFC 6901's JSON Pointer: the empty string, or
reference tokens each led by `/`, in which `~` is followed by `0` or `1`. A
token holds any other code point, NUL included, so the string is decoded as
UTF-8 and must be well-formed.

`is_relative_json_pointer` checks the relative JSON Pointer of
draft-handrews-relative-json-pointer-01, which drafts 2019-09, 2020-12 and v1
of JSON Schema cite: a non-negative integer without a leading zero, then `#`
or a JSON Pointer. The draft's later revisions add an index manipulation
(`0+1`), which no published draft of JSON Schema asks for, and which is
invalid here.
```

- [ ] **Step 6: Build the page, run the lint.**
  `B2 -a --build-dir=bin/formats-ids-doc libs/formats/doc` exits 0; `LINT`:
  `lint: clean`.

- [ ] **Step 7: Commit** in `~/dev/webcpp-formats-ids/libs/formats`:

```bash
git add include/webcpp/formats/uuid.hpp include/webcpp/formats/json_pointer.hpp \
  include/webcpp/formats.hpp test/uuid_test.cpp test/json_pointer_test.cpp test/Jamfile \
  test/allocation_test.cpp doc/identifiers.adoc doc/formats.adoc
git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "formats: UUIDs and JSON Pointers, absolute and relative"
```

The report's Changelog: the "New Libraries" line, unchanged.

### Task 4: The suite runner ∥

Worktree slug `runner`, `<base>` = `B-S2`: `lib/formats` rebased onto
`infra/shared` once infra's Task 1 is committed there, which brings
`/webcpp/third_party//json-schema-test-suite` and the submodule
`third_party/libs/JSON-Schema-Test-Suite` (checked out from
`~/.buzz/REPOS/JSON-Schema-Test-Suite` at `5b0ee16`, Global Constraints).
Needs S2, and Tasks 2 and 3 integrated (it maps their seven formats). ∥
Tasks 5 and 6. Every later format task registers its formats here, so Tasks
5 to 9 need this task integrated before their last step.

**Files:**
- Create: `test/suite_test.cpp`, `test/known_failures/draft4.json`, `draft6.json`, `draft7.json`, `draft2019-09.json`, `draft2020-12.json`, `v1.json`
- Modify: `test/Jamfile` (`import regex ;`, the known-failure lists' constant, the suite's run), `AGENTS.md` (one paragraph), `doc/testing.adoc` (one paragraph)

**Interfaces:**
- Consumes: infra's `/webcpp/third_party//json-schema-test-suite`, whose usage requirements give `WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE` (the suite's root, holding `tests/`), a dependency on every file, and a refusal by name when the submodule is not checked out; Tasks 2 and 3's validators.
- Produces: the program `suite` (native and emscripten until Task 10), and
  two arrays in `test/suite_test.cpp` that every later format task edits:

```cpp
constexpr std::array validators{ mapping{"<format>", [](std::string_view s) { return formats::is_<name>(s); }}, ... };
constexpr auto pending = std::to_array<std::string_view>({"<format>", ...});
```

  A format task adds its formats' `mapping` lines to `validators` (sorted by
  format) and removes them from `pending`; `std::to_array` counts the list,
  so no merge can leave it padded with empty names.
  `test/known_failures/<dialect>.json` is an array of
  `{"file": "optional/format/<file>.json", "case": "<case description>", "test": "<test description>", "reason": "<why, naming both Unicode versions>"}`
  (`"format/<file>.json"` in v1), json_schema's form, empty in every
  dialect.

- [ ] **Step 1: The known-failure lists.** Each of the six files holds:

```json
[]
```

- [ ] **Step 2: Write the runner** `test/suite_test.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Runs the JSON-Schema-Test-Suite's format tests against formats' validators, called directly:
// every file of tests/<dialect>/optional/format/ for draft4 to draft2020-12, and of
// tests/v1/format/. A string test passes when its validator's verdict is the test's "valid". A
// non-string test checks json_schema's rule that a format applies to strings only, so it must
// expect valid, and is counted apart. The run fails on a failure its dialect's known-failure list
// does not name, on a listed test that passes or that the suite does not hold, and on a format
// that maps to no validator. WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE names the suite's root, from
// /webcpp/third_party//json-schema-test-suite, and WEBCPP_TEST_FORMATS_KNOWN_FAILURES the lists'
// directory, from test/Jamfile. A known failure names its file as json_schema's lists do, by its
// path under the dialect's directory (optional/format/uuid.json).

#include <webcpp/formats.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/json.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <vector>

namespace formats = webcpp::formats;
namespace fs = std::filesystem;
namespace json = boost::json;

namespace {

/** A validator the suite's string tests run against. */
using validator = bool (*)(std::string_view);

/** A format the suite names, and the validator its string tests run against. */
struct mapping {
    std::string_view format;
    validator valid;
};

/** The validators, one per format the suite names, sorted by format; validator_for picks v1's time. */
constexpr std::array validators{
    mapping{"date", [](std::string_view s) { return formats::is_date(s); }},
    mapping{"date-time", [](std::string_view s) { return formats::is_date_time(s); }},
    mapping{"duration", [](std::string_view s) { return formats::is_duration(s); }},
    mapping{"json-pointer", [](std::string_view s) { return formats::is_json_pointer(s); }},
    mapping{"relative-json-pointer",
            [](std::string_view s) { return formats::is_relative_json_pointer(s); }},
    mapping{"time", [](std::string_view s) { return formats::is_time(s); }},
    mapping{"uuid", [](std::string_view s) { return formats::is_uuid(s); }},
};

/**
 The formats the suite names whose validators are not written yet: their string tests are counted
 and skipped by name, never silently. A change that writes a validator moves its format from here
 to validators; the list leaves the runner once it is empty.
*/
constexpr auto pending = std::to_array<std::string_view>({
    "email", "hostname", "idn-email", "idn-hostname", "ipv4", "ipv6", "iri", "iri-reference",
    "regex", "uri", "uri-reference", "uri-template",
});

/** The format the suite holds for json_schema's question, an unknown format: skipped by name. */
constexpr std::string_view unknown_format = "unknown";

/** A dialect of the suite, its directory under tests/, and its format tests' under that one. */
struct dialect {
    std::string_view name;
    std::string_view formats;
};

constexpr std::array dialects{
    dialect{"draft4", "optional/format"},
    dialect{"draft6", "optional/format"},
    dialect{"draft7", "optional/format"},
    dialect{"draft2019-09", "optional/format"},
    dialect{"draft2020-12", "optional/format"},
    dialect{"v1", "format"},
};

/** A test of the suite: its file, its case's description and its own description. */
using test_id = std::tuple<std::string, std::string, std::string>;

/** What a dialect's run counted. */
struct tally {
    std::size_t strings = 0;
    std::size_t known = 0;
    std::size_t pending = 0;
    std::size_t non_strings = 0;
    std::size_t skipped = 0;
};

/** A dialect's known failures, and those of them its run met. */
struct known_failures {
    std::set<test_id> listed;
    std::set<test_id> met;
};

/** Where a test is, and the validator its case's format maps to: null when pending or unknown. */
struct where {
    std::string_view dialect;
    std::string_view file;
    std::string_view case_description;
    std::string_view format;
    validator valid = nullptr;
};

/** The file at `path` parsed as JSON, or nothing, said on the standard error. */
std::optional<json::value> read_json(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << path.string() << ": cannot be opened\n";
        return std::nullopt;
    }
    std::ostringstream text;
    text << in.rdbuf();
    boost::system::error_code error;
    json::value value = json::parse(text.str(), error);
    if (error) {
        std::cerr << path.string() << ": not JSON: " << error.message() << '\n';
        return std::nullopt;
    }
    return value;
}

/** The string at `key` of `object`, or an empty view when it holds no string there. */
std::string_view string_at(const json::object& object, std::string_view key) {
    const json::value* value = object.if_contains(key);
    if (value == nullptr || !value->is_string()) {
        return {};
    }
    const json::string& s = value->get_string();
    return {s.data(), s.size()};
}

/** The validator of `format` in the dialect `name`: v1's time refuses leap seconds. */
validator validator_for(std::string_view format, std::string_view name) {
    if (format == "time" && name == "v1") {
        return [](std::string_view s) { return formats::is_time(s, formats::leap_seconds::refused); };
    }
    const auto found = std::ranges::find(validators, format, &mapping::format);
    return found == validators.end() ? nullptr : found->valid;
}

/** The known failures the list of the dialect `name` holds; a malformed list fails the run. */
std::set<test_id> read_known_failures(std::string_view name) {
    const fs::path path =
        fs::path(WEBCPP_TEST_FORMATS_KNOWN_FAILURES) / (std::string(name) + ".json");
    std::set<test_id> listed;
    const std::optional<json::value> list = read_json(path);
    if (!BOOST_TEST(list.has_value() && list->is_array())) {
        return listed;
    }
    for (const json::value& entry : list->get_array()) {
        const json::object* object = entry.if_object();
        if (!BOOST_TEST(object != nullptr && !string_at(*object, "file").empty() &&
                        !string_at(*object, "reason").empty())) {
            std::cerr << path.string() << ": an entry without its file, case, test and reason\n";
            continue;
        }
        listed.emplace(std::string(string_at(*object, "file")),
                       std::string(string_at(*object, "case")),
                       std::string(string_at(*object, "test")));
    }
    return listed;
}

/** Says why a test failed, on the standard error. */
void report(const where& at, std::string_view test, std::string_view why) {
    std::cerr << at.dialect << '/' << at.file << ": \"" << at.case_description << "\" / \"" << test
              << "\": " << why << '\n';
}

/** Runs one test of the case `at`. */
void run_test(const where& at, const json::object& test, known_failures& failures,
              tally& counted) {
    const json::value* data = test.if_contains("data");
    const json::value* expected = test.if_contains("valid");
    const std::string_view description = string_at(test, "description");
    if (!BOOST_TEST(data != nullptr && expected != nullptr && expected->is_bool())) {
        report(at, description, "a test without its data and its verdict");
        return;
    }
    if (!data->is_string()) {
        ++counted.non_strings;
        if (!BOOST_TEST(expected->get_bool())) {
            report(at, description, "a non-string instance expected invalid");
        }
        return;
    }
    if (at.format == unknown_format) {
        ++counted.skipped;
        return;
    }
    if (at.valid == nullptr) {
        ++counted.pending;
        return;
    }
    ++counted.strings;
    const json::string& text = data->get_string();
    const bool verdict = at.valid(std::string_view(text.data(), text.size()));
    const test_id id{std::string(at.file), std::string(at.case_description),
                     std::string(description)};
    const bool listed = failures.listed.contains(id);
    if (listed) {
        failures.met.insert(id);
        ++counted.known;
    }
    if (verdict == expected->get_bool()) {
        if (!BOOST_TEST(!listed)) {
            report(at, description, "passes, and its dialect's known-failure list names it");
        }
    } else if (!BOOST_TEST(listed)) {
        report(at, description,
               expected->get_bool() ? "expected valid, the validator says invalid"
                                    : "expected invalid, the validator says valid");
    }
}

/** Runs every case of the file at `path`. */
void run_file(const dialect& d, const fs::path& path, known_failures& failures,
              tally& counted) {
    const std::string_view name = d.name;
    // The file as json_schema's known-failure lists name it: under the dialect's directory.
    const std::string file = std::string(d.formats) + "/" + path.filename().string();
    const std::optional<json::value> document = read_json(path);
    if (!BOOST_TEST(document.has_value() && document->is_array())) {
        return;
    }
    for (const json::value& group : document->get_array()) {
        const json::object* object = group.if_object();
        const json::value* schema = object == nullptr ? nullptr : object->if_contains("schema");
        const json::value* tests = object == nullptr ? nullptr : object->if_contains("tests");
        if (!BOOST_TEST(schema != nullptr && schema->is_object() && tests != nullptr &&
                        tests->is_array())) {
            std::cerr << name << '/' << file << ": a case without its schema and its tests\n";
            continue;
        }
        const std::string_view format = string_at(schema->get_object(), "format");
        const where at{name, file, string_at(*object, "description"), format,
                       validator_for(format, name)};
        const bool is_pending = std::ranges::find(pending, format) != pending.end();
        if (!BOOST_TEST(at.valid != nullptr || is_pending || format == unknown_format)) {
            std::cerr << name << '/' << file << ": the format \"" << format
                      << "\" maps to no validator\n";
            continue;
        }
        for (const json::value& test : tests->get_array()) {
            if (const json::object* t = test.if_object(); BOOST_TEST(t != nullptr)) {
                run_test(at, *t, failures, counted);
            }
        }
    }
}

/** Runs every format test of the dialect `d`, and prints what it counted. */
void run_dialect(const dialect& d) {
    const fs::path directory =
        fs::path(WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE) / "tests" / d.name / d.formats;
    std::vector<fs::path> files;
    std::error_code error;
    for (const fs::directory_entry& entry : fs::directory_iterator(directory, error)) {
        if (entry.path().extension() == ".json") {
            files.push_back(entry.path());
        }
    }
    if (!BOOST_TEST(!error && !files.empty())) {
        std::cerr << directory.string() << ": no format tests: " << error.message() << '\n';
        return;
    }
    std::ranges::sort(files);
    known_failures failures{read_known_failures(d.name), {}};
    tally counted;
    for (const fs::path& path : files) {
        run_file(d, path, failures, counted);
    }
    for (const test_id& id : failures.listed) {
        if (!BOOST_TEST(failures.met.contains(id))) {
            std::cerr << d.name << ": the known failure \"" << std::get<0>(id) << "\" / \""
                      << std::get<1>(id) << "\" / \"" << std::get<2>(id)
                      << "\" is no string test of the suite\n";
        }
    }
    std::cout << d.name << ": " << counted.strings << " string tests, " << counted.known
              << " known failures, " << counted.pending << " pending, " << counted.non_strings
              << " non-string, " << counted.skipped << " skipped (unknown)\n";
}

}  // namespace

int main() {
    for (const dialect& d : dialects) {
        run_dialect(d);
    }
    return boost::report_errors();
}
```

`test/Jamfile` gains `import regex ;` after `project` and, after the format
tests:

```
# The JSON Schema Test Suite comes from /webcpp/third_party, which gives its
# root as WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE; the known-failure lists are
# formats' own, named with forward slashes, the define quoted whole, for a
# checkout whose path holds a space. Natively and on emscripten, whose
# -sNODERAWFS=1 reads the host's files; wasip2 and wasip3 join once the
# lists' directory is given to them too (webcpp.data).
path-constant KNOWN_FAILURES : known_failures ;
local known = [ regex.replace $(KNOWN_FAILURES) "\\\\" "/" ] ;
webcpp.run suite : suite_test.cpp
  : <library>/webcpp//boost_json
    <library>/webcpp/third_party//json-schema-test-suite
    <define>"\"WEBCPP_TEST_FORMATS_KNOWN_FAILURES=\\\"$(known)\\\"\""
    <dependency>[ glob known_failures/*.json ]
  : native emscripten ;
```

- [ ] **Step 3: See the runner fail on a planted defect** (the runner itself
  is the test here, so it is first seen catching a wrong verdict). In
  `validators`, change the `uuid` lambda to `return !formats::is_uuid(s);`
  and run:

```bash
B2 --build-dir=bin/formats-runner-native libs/formats/test//suite
```

  Expected: FAIL, with lines such as
  `draft2019-09/optional/format/uuid.json: "uuid format" / "all upper-case": expected valid, the validator says invalid`
  in the output b2 prints for the failed run. Then restore the lambda, and
  put in `test/known_failures/draft2020-12.json` the two entries
  `{"file": "optional/format/uuid.json", "case": "uuid format", "test": "wrong length", "reason": "planted"}`
  and
  `{"file": "optional/format/uuid.json", "case": "uuid format", "test": "no such test", "reason": "planted"}`.
  Expected: FAIL with
  `draft2020-12/optional/format/uuid.json: "uuid format" / "wrong length": passes, and its dialect's known-failure list names it`
  and
  `draft2020-12: the known failure "optional/format/uuid.json" / "uuid format" / "no such test" is no string test of the suite`.
  Then empty the submodule's checkout in the task worktree
  (`mv third_party/libs/JSON-Schema-Test-Suite /tmp/formats-suite-aside`) and
  run again: expected, b2 stops before compiling, naming
  `third_party/libs/JSON-Schema-Test-Suite is not checked out` and the
  command that checks it out. Move it back.
  Restore the list to `[]`.

- [ ] **Step 4: Run it and watch it pass.** The native and emscripten lanes
  of Global Constraints with `<t>` = `runner`, then the summary:

```bash
find bin/formats-runner-native -name 'suite.output' -exec cat {} +
```

Expected: `suite` passed on both lanes (skipped without a word on wasip2 and
wasip3), and:

```
draft4: 37 string tests, 0 known failures, 211 pending, 42 non-string, 1 skipped (unknown)
draft6: 71 string tests, 0 known failures, 277 pending, 60 non-string, 1 skipped (unknown)
draft7: 215 string tests, 0 known failures, 469 pending, 108 non-string, 1 skipped (unknown)
draft2019-09: 284 string tests, 0 known failures, 469 pending, 120 non-string, 1 skipped (unknown)
draft2020-12: 284 string tests, 0 known failures, 469 pending, 120 non-string, 1 skipped (unknown)
v1: 284 string tests, 0 known failures, 470 pending, 114 non-string, 0 skipped (unknown)
```

  If a date, time, duration, UUID or pointer test fails, the fix goes into
  that format's header with a unit test that reproduces it first, in this
  task's commit, and the report names it.

- [ ] **Step 5: AGENTS.md and the testing chapter.** `AGENTS.md` gains:

```markdown
- **The JSON Schema suite.** `test/suite_test.cpp` runs the format tests of
  the JSON-Schema-Test-Suite, the superproject's submodule
  `third_party/libs/JSON-Schema-Test-Suite`, against the validators. No
  copy of the suite's data is committed: it is MIT data, and every file of
  webcpp is BSL-1.0. `test/known_failures/<dialect>.json` lists the tests
  expected to fail, each with its reason, and only a residue tied to a
  Unicode version may be one; the runner fails on an unlisted failure and on
  a listed test that passes, so a list never lies. Moving the submodule is a
  change of its own, with every new failure fixed or listed in it.
```

  `doc/testing.adoc` gains, before its paragraph on the test matrix:

```
The official JSON-Schema-Test-Suite's format tests run against the
validators directly, in every draft from 4 to 2020-12 and v1: a string test
passes when the validator's verdict is the test's. The runner fails on a
failure its dialect's known-failure list does not name, and on a listed test
that passes.
```

- [ ] **Step 6: Build the page, run the lint.** The doc build with
  `<t>` = `runner` exits 0; `LINT`: `lint: clean` (the runner is analysed
  with and without exceptions, as every test is).

- [ ] **Step 7: Commit** in `~/dev/webcpp-formats-runner/libs/formats`:

```bash
git add test/suite_test.cpp test/known_failures/draft4.json test/known_failures/draft6.json \
  test/known_failures/draft7.json test/known_failures/draft2019-09.json \
  test/known_failures/draft2020-12.json test/known_failures/v1.json test/Jamfile AGENTS.md \
  doc/testing.adoc
git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "formats: the JSON Schema suite's format tests, run against the validators, with known-failure lists that never lie"
```

The report's Changelog: the "New Libraries" line, unchanged.

### Task 5: IP addresses and URIs (F3) ∥

Worktree slug `uris`, `<base>` = `B-S4`: `lib/formats` rebased onto
`infra/shared` once infra's Task 3 is committed there, which brings
`/webcpp//boost_url` (Boost.URL's compiled sources from Boost's source
tarball, per the owner's 2026-10-09 decision; `.local/boost-source` linked to
`~/dev/webcpp-infra/.local/boost-source`). Needs S4 and Task 1; its last
step needs Task 4 integrated. ∥ Tasks 2, 3, 4, 6.

**Files:**
- Create: `include/webcpp/formats/ip.hpp`, `include/webcpp/formats/uri.hpp`, `include/webcpp/formats/detail/uri_guard.hpp`, `test/ip_test.cpp`, `test/uri_test.cpp`, `doc/addresses.adoc`, `doc/uris.adoc`
- Modify: `build.jam`, `include/webcpp/formats.hpp`, `test/Jamfile`, `test/allocation_test.cpp`, `test/suite_test.cpp`, `doc/formats.adoc`, `AGENTS.md`

**Interfaces:**
- Consumes: S4's `<library>/webcpp//boost_url`; Task 1's `grammar::text`, `grammar::result`.
- Produces:

```cpp
namespace webcpp::formats {
[[nodiscard]] bool is_ipv4(std::string_view s) noexcept;
[[nodiscard]] bool is_ipv6(std::string_view s) noexcept;
[[nodiscard]] bool is_uri(std::string_view s) noexcept;
[[nodiscard]] bool is_uri_reference(std::string_view s) noexcept;
}
namespace webcpp::formats::grammar {
struct uri_reference_rule_t {   // Boost.URL's uri_reference_rule, then the guard
    using value_type = boost::urls::url_view;
    result<boost::urls::url_view> parse(char const*&, char const*) const noexcept;
};
inline constexpr uri_reference_rule_t uri_reference_rule{};
}
namespace webcpp::formats::detail {
bool refuses_relative_authority_path(boost::urls::url_view u) noexcept;
}
```

- [ ] **Step 1: Write the failing tests.** `test/ip_test.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// IPv4 (RFC 2673 3.2's dotted quad, RFC 3986's dec-octet) and IPv6 (RFC 4291 2.2), through
// Boost.URL's rules: RFC 4291 2.2's examples, leading zeros, zone ids and prefixes refused.

#include <webcpp/formats/ip.hpp>

#include <boost/core/lightweight_test.hpp>

#include <iostream>
#include <string_view>

namespace formats = webcpp::formats;

namespace {

void ipv4(std::string_view text, bool valid) {
    if (!BOOST_TEST_EQ(formats::is_ipv4(text), valid)) {
        std::cerr << "ipv4: \"" << text << "\"\n";
    }
}

void ipv6(std::string_view text, bool valid) {
    if (!BOOST_TEST_EQ(formats::is_ipv6(text), valid)) {
        std::cerr << "ipv6: \"" << text << "\"\n";
    }
}

void dotted_quads() {
    ipv4("192.168.0.1", true);
    ipv4("0.0.0.0", true);
    ipv4("255.255.255.255", true);
    ipv4("256.0.0.1", false);
    ipv4("192.168.0.01", false);
    ipv4("087.10.0.1", false);
    ipv4("1.2.3", false);
    ipv4("1.2.3.4.5", false);
    ipv4("0x7f000001", false);
    ipv4("1.2.3.4 ", false);
    ipv4(std::string_view("1.2.3.4\0", 8), false);
    ipv4("\xD9\xA1.2.3.4", false);
}

void rfc_4291_section_2_2() {
    for (const std::string_view text :
         {"ABCD:EF01:2345:6789:ABCD:EF01:2345:6789", "2001:DB8:0:0:8:800:200C:417A",
          "2001:DB8::8:800:200C:417A", "FF01::101", "::1", "::", "0:0:0:0:0:0:13.1.68.3",
          "0:0:0:0:0:FFFF:129.144.52.38", "::13.1.68.3", "::FFFF:129.144.52.38"}) {
        ipv6(text, true);
    }
}

void ipv6_refusals() {
    ipv6("fe80::1%eth0", false);
    ipv6("2001:db8::/32", false);
    ipv6("1:2:3:4:5:6:7:8:9", false);
    ipv6("1::2::3", false);
    ipv6("::ffff:192.168.0.01", false);
    ipv6("12345::", false);
    ipv6("[::1]", false);
    ipv6(std::string_view("::1\0", 4), false);
}

}  // namespace

int main() {
    dotted_quads();
    rfc_4291_section_2_2();
    ipv6_refusals();
    return boost::report_errors();
}
```

`test/uri_test.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// RFC 3986's URI and URI reference through Boost.URL: section 1.1.2's examples, the guard's two
// inputs refused (and the same with a scheme), the zone id verdict Boost.URL gives, and a pin of
// Boost.URL's own reading of the two inputs, which fails the day a Boost release fixes it, when
// the guard can leave.

#include <webcpp/formats/uri.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/url/grammar/delim_rule.hpp>
#include <boost/url/grammar/error.hpp>
#include <boost/url/grammar/parse.hpp>
#include <boost/url/grammar/tuple_rule.hpp>
#include <boost/url/parse.hpp>

#include <iostream>
#include <string_view>
#include <tuple>

namespace formats = webcpp::formats;
namespace fg = webcpp::formats::grammar;
namespace urlg = boost::urls::grammar;

namespace {

void uri(std::string_view text, bool valid) {
    if (!BOOST_TEST_EQ(formats::is_uri(text), valid)) {
        std::cerr << "uri: \"" << text << "\"\n";
    }
}

void reference(std::string_view text, bool valid) {
    if (!BOOST_TEST_EQ(formats::is_uri_reference(text), valid)) {
        std::cerr << "uri-reference: \"" << text << "\"\n";
    }
    BOOST_TEST_EQ(formats::is_uri_reference(text),
                  urlg::parse(text, fg::uri_reference_rule).has_value());
}

void rfc_3986_section_1_1_2() {
    for (const std::string_view text :
         {"ftp://ftp.is.co.za/rfc/rfc1808.txt", "http://www.ietf.org/rfc/rfc2396.txt",
          "ldap://[2001:db8::7]/c=GB?objectClass?one", "mailto:John.Doe@example.com",
          "news:comp.infosystems.www.servers.unix", "tel:+1-816-555-1212",
          "telnet://192.0.2.16:80/", "urn:oasis:names:specification:docbook:dtd:xml:4.1.2"}) {
        uri(text, true);
        reference(text, true);
    }
}

void uris_and_references() {
    uri("//example.com/p", false);
    uri("/abc", false);
    uri("http://example.com/a b", false);
    uri("http://example.com/%zz", false);
    uri(std::string_view("http://a/\0", 10), false);
    reference("//example.com/p", true);
    reference("", true);
    reference("#fragment", true);
    reference("../a/b?c", true);
    reference("\\\\WINDOWS\\fileshare", false);
    reference("http://example.com/a b", false);
}

void the_guard() {
    // RFC 3986 4.2: a relative reference with an authority has a path-abempty, empty or led by
    // "/". Boost.URL 1.92 reads these two as an authority and a path that is neither.
    reference("//example.com:abc/p", false);
    reference("//a@b@example.com/", false);
    uri("http://example.com:abc/p", false);
    uri("http://a@b@example.com/", false);
    reference("//example.com:80/p", true);
    reference("//a@example.com", true);
}

void boost_url_still_needs_the_guard() {
    // When this fails, Boost.URL refuses the input itself: the guard and its changelog line
    // leave, in the change that moves to that Boost release, and the_guard stays.
    const auto port = boost::urls::parse_uri_reference("//example.com:abc/p");
    if (BOOST_TEST(port.has_value())) {
        BOOST_TEST_EQ(port->encoded_host(), "example.com");
        BOOST_TEST_EQ(port->encoded_path(), "abc/p");
    }
    const auto at = boost::urls::parse_uri_reference("//a@b@example.com/");
    if (BOOST_TEST(at.has_value())) {
        BOOST_TEST_EQ(at->encoded_path(), "@example.com/");
    }
}

void zone_ids_keep_boost_urls_verdict() {
    // RFC 6874's zone id, which Boost.URL accepts and the suite does not test (the page's
    // "Known deviations").
    uri("http://[fe80::1%25eth0]/", true);
    reference("//[fe80::1%25eth0]/", true);
}

void the_rule_inside_a_grammar() {
    const std::string_view text = "//a@b@example.com/";
    char const* it = text.data();
    const auto r = fg::uri_reference_rule.parse(it, text.data() + text.size());
    BOOST_TEST(r.has_error() && r.error() == urlg::error::invalid);
    BOOST_TEST(it == text.data());
    const auto pair = urlg::parse(
        "a/b c/d", urlg::tuple_rule(fg::uri_reference_rule, urlg::squelch(urlg::delim_rule(' ')),
                                    fg::uri_reference_rule));
    if (BOOST_TEST(pair.has_value())) {
        BOOST_TEST_EQ(std::get<0>(*pair).buffer(), "a/b");
        BOOST_TEST_EQ(std::get<1>(*pair).buffer(), "c/d");
    }
}

}  // namespace

int main() {
    rfc_3986_section_1_1_2();
    uris_and_references();
    the_guard();
    boost_url_still_needs_the_guard();
    zone_ids_keep_boost_urls_verdict();
    the_rule_inside_a_grammar();
    return boost::report_errors();
}
```

`test/Jamfile`, after the identifier tests:

```
webcpp.run ip : ip_test.cpp ;
webcpp.run uri : uri_test.cpp ;
```

`test/allocation_test.cpp` gains, with `#include <webcpp/formats/ip.hpp>` and
`#include <webcpp/formats/uri.hpp>` and its call:

```cpp
/** IP addresses, URIs and URI references allocate nothing, a URI of 100,000 octets included. */
void uris_allocate_nothing() {
    std::string long_uri = "http://example.com/";
    long_uri.append(100'000, 'a');
    const std::size_t before = global_allocations();
    BOOST_TEST(webcpp::formats::is_ipv4("192.168.0.1"));
    BOOST_TEST(webcpp::formats::is_ipv6("2001:DB8::8:800:200C:417A"));
    BOOST_TEST(webcpp::formats::is_uri(long_uri));
    BOOST_TEST(webcpp::formats::is_uri_reference("//example.com/p"));
    BOOST_TEST(!webcpp::formats::is_uri_reference("//a@b@example.com/"));
    BOOST_TEST_EQ(global_allocations() - before, 0U);
}
```

- [ ] **Step 2: Run them and watch them fail.**
  `B2 --build-dir=bin/formats-uris-native libs/formats/test//ip libs/formats/test//uri`.
  Expected: FAIL, `'webcpp/formats/ip.hpp' file not found` and
  `'webcpp/formats/uri.hpp' file not found`.

- [ ] **Step 3: Write the headers, and link Boost.URL.** `build.jam`'s alias
  and comment become:

```
# formats, validators and Boost.URL grammar rules for the string formats of
# JSON Schema. A program uses it through /webcpp/formats//formats, which adds
# its headers and Boost's, and Boost.URL's compiled part, which ipv4_address
# needs (/webcpp//boost_url, built from Boost's sources for every target).

project /webcpp/formats ;

alias formats
  : : : : <include>include <library>/boost//headers
    <library>/webcpp//boost_url ;
```

`include/webcpp/formats/ip.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 IP addresses, as validators over Boost.URL's own rules, `boost::urls::ipv4_address_rule` and
 `boost::urls::ipv6_address_rule`, which are the formats' rules too.

 @see "Addresses", in the guide.
*/
#ifndef WEBCPP_FORMATS_IP_HPP
#define WEBCPP_FORMATS_IP_HPP

#include <webcpp/formats/config.hpp>

#include <boost/url/grammar/parse.hpp>
#include <boost/url/rfc/ipv4_address_rule.hpp>
#include <boost/url/rfc/ipv6_address_rule.hpp>

#include <string_view>

namespace webcpp::formats {

/**
 Whether `s` is an IPv4 address in dotted-quad form, JSON Schema's `ipv4`.

 RFC 2673 3.2's dotted quad, each part RFC 3986's dec-octet, 0 to 255 without a leading zero.

 @param s Any byte string.
 @return `parse(s, boost::urls::ipv4_address_rule).has_value()`.
 @see "Addresses", in the guide.
*/
[[nodiscard]] inline bool is_ipv4(std::string_view s) noexcept {
    return boost::urls::grammar::parse(s, boost::urls::ipv4_address_rule).has_value();
}

/**
 Whether `s` is an IPv6 address, JSON Schema's `ipv6`.

 RFC 4291 2.2's three text forms; no zone id, no prefix length.

 @param s Any byte string.
 @return `parse(s, boost::urls::ipv6_address_rule).has_value()`.
 @see "Addresses", in the guide.
*/
[[nodiscard]] inline bool is_ipv6(std::string_view s) noexcept {
    return boost::urls::grammar::parse(s, boost::urls::ipv6_address_rule).has_value();
}

}  // namespace webcpp::formats

#endif  // WEBCPP_FORMATS_IP_HPP
```

`include/webcpp/formats/detail/uri_guard.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// The guard over Boost.URL 1.92's uri_reference_rule, which accepts a relative reference whose
// path after an authority is not path-abempty. It leaves with the first Boost release that refuses
// those references itself (test/uri_test.cpp, boost_url_still_needs_the_guard).

#ifndef WEBCPP_FORMATS_DETAIL_URI_GUARD_HPP
#define WEBCPP_FORMATS_DETAIL_URI_GUARD_HPP

#include <webcpp/formats/config.hpp>

#include <boost/url/url_view.hpp>

namespace webcpp::formats::detail {

/** True when Boost.URL accepted a relative reference with an authority and a path that is not empty and not led by "/", which RFC 3986 4.2 refuses. */
inline bool refuses_relative_authority_path(boost::urls::url_view u) noexcept {
    return !u.has_scheme() && u.has_authority() && !u.encoded_path().empty() &&
           u.encoded_path().front() != '/';
}

}  // namespace webcpp::formats::detail

#endif  // WEBCPP_FORMATS_DETAIL_URI_GUARD_HPP
```

`include/webcpp/formats/uri.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 URIs and URI references (RFC 3986), as validators and Boost.URL rules: `uri` is Boost.URL's own
 rule, `boost::urls::uri_rule`, and `uri-reference` Boost.URL's with a guard of formats'.

 @see "URIs and IRIs", in the guide.
*/
#ifndef WEBCPP_FORMATS_URI_HPP
#define WEBCPP_FORMATS_URI_HPP

#include <webcpp/formats/config.hpp>

#include <webcpp/formats/detail/uri_guard.hpp>
#include <webcpp/formats/grammar.hpp>

#include <boost/url/grammar/error.hpp>
#include <boost/url/grammar/parse.hpp>
#include <boost/url/parse.hpp>
#include <boost/url/rfc/uri_reference_rule.hpp>
#include <boost/url/url_view.hpp>

#include <string_view>

namespace webcpp::formats {

namespace detail {

namespace urlg = boost::urls::grammar;

}  // namespace detail

namespace grammar {

/**
 The rule of RFC 3986 4.1's URI-reference: Boost.URL's `uri_reference_rule`, then a guard.

 Boost.URL 1.92 accepts `//example.com:abc/p` and `//a@b@example.com/`, a relative reference whose
 path after the authority is not `path-abempty` (RFC 3986 4.2); the guard refuses them. It yields
 Boost.URL's `url_view` of the reference, and stops where Boost.URL's rule does.

 @see "URIs and IRIs", in the guide.
*/
struct uri_reference_rule_t {
    /** The rule yields Boost.URL's view of the reference it matched. */
    using value_type = boost::urls::url_view;

    /**
     Matches a URI-reference at `it`.

     @param it The start of the input: past the match on success, else unchanged.
     @param end The end of the input.
     @return The view of the reference; Boost.URL's error when its rule refuses the input;
     `error::invalid` when the guard does.
    */
    [[nodiscard]] result<boost::urls::url_view> parse(char const*& it,
                                                      char const* end) const noexcept;
};

/** The rule of RFC 3986's URI-reference, guarded. */
inline constexpr uri_reference_rule_t uri_reference_rule{};

inline result<boost::urls::url_view> uri_reference_rule_t::parse(char const*& it,
                                                                 char const* end) const noexcept {
    char const* const first = it;
    auto reference = detail::urlg::parse(it, end, boost::urls::uri_reference_rule);
    if (!reference) {
        it = first;
        return reference.error();
    }
    if (detail::refuses_relative_authority_path(*reference)) {
        it = first;
        return detail::urlg::error::invalid;
    }
    return reference;
}

}  // namespace grammar

/**
 Whether `s` is an RFC 3986 URI, JSON Schema's `uri`: a scheme, and a fragment allowed.

 @param s Any byte string.
 @return `boost::urls::parse_uri(s).has_value()`.
 @see "URIs and IRIs", in the guide.
*/
[[nodiscard]] inline bool is_uri(std::string_view s) noexcept {
    return boost::urls::parse_uri(s).has_value();
}

/**
 Whether `s` is an RFC 3986 URI-reference, JSON Schema's `uri-reference`: a URI or a relative
 reference.

 @param s Any byte string.
 @return `parse(s, grammar::uri_reference_rule).has_value()`.
 @see "URIs and IRIs", in the guide.
*/
[[nodiscard]] inline bool is_uri_reference(std::string_view s) noexcept {
    return detail::urlg::parse(s, grammar::uri_reference_rule).has_value();
}

}  // namespace webcpp::formats

#endif  // WEBCPP_FORMATS_URI_HPP
```

`include/webcpp/formats.hpp` gains `ip.hpp` and `uri.hpp`, sorted.

- [ ] **Step 4: Run them and watch them pass.**
  `B2 --build-dir=bin/formats-uris-native libs/formats/test//ip libs/formats/test//uri`:
  PASS. If `boost_url_still_needs_the_guard` fails, the configured Boost
  already refuses the two inputs: report it, keep `the_guard`, and leave the
  guard in (its removal is a change of its own, the spec's section 5.7).
  Then see the guard fail: make `refuses_relative_authority_path` return
  `false` and run `uri` again. Expected: FAIL, `uri-reference:
  "//example.com:abc/p"` and `uri-reference: "//a@b@example.com/"` named on
  the standard error (and, once Step 9 registers the format, the suite's "a
  non-numeric port in a network-path reference" and "more than one at-sign
  in the authority"). Restore it. Then the five lanes with `<t>` = `uris`:
  every lane green, `ip`, `uri` and `allocation` passed on all four targets.

- [ ] **Step 5: Measure a program's size.** In the job's scratch directory
  `$SCRATCH` (never committed), `size_probe.cpp`:

```cpp
#include <webcpp/formats/uri.hpp>

int main(int argc, char** argv) {
    return argc > 1 && webcpp::formats::is_uri(argv[1]) ? 0 : 1;
}
```

  and `empty_probe.cpp`, `int main() { return 0; }`. Build Boost.URL for
  wasip2 release, then both programs with the release flags of a wasip2
  program (`B2 -n -a toolset=clang-wasip2 variant=release
  libs/formats/test//uri` prints the ones b2 uses; the command below is
  them, with Boost's include directory read from `.local/user-config.jam`),
  linking the archive b2 built:

```bash
B2 --build-dir=bin/formats-uris-size toolset=clang-wasip2 variant=release /webcpp//boost_url
ARCHIVE=$(find bin/formats-uris-size -name 'libboost_url*.a' | head -n 1)
BOOST_INCLUDE=$(sed -n 's/.*using boost.*<include>\([^ ]*\).*/\1/p' .local/user-config.jam)
for p in size_probe empty_probe; do
  .local/wasi-sdk/bin/clang++ --target=wasm32-wasip2 -std=c++20 -O2 -DNDEBUG \
    -fwasm-exceptions -mllvm -wasm-use-legacy-eh=false \
    -I libs/formats/include -isystem "$BOOST_INCLUDE" \
    "$SCRATCH/$p.cpp" "$ARCHIVE" -fwasm-exceptions -lunwind -o "$SCRATCH/$p.wasm"
  .local/wasi-sdk/bin/llvm-strip -o "$SCRATCH/$p.stripped.wasm" "$SCRATCH/$p.wasm"
  wc -c < "$SCRATCH/$p.stripped.wasm"
done
```

  Expected: two sizes in bytes. Record both in the ledger, and the
  difference on the page (Step 6), with the date, Boost 1.92 and wasi-sdk 34.

- [ ] **Step 6: The page.** `doc/addresses.adoc`, after
  `identifiers.adoc`'s place:

```
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

[#formats-addresses]
== Addresses

`is_ipv4` and `is_ipv6` are Boost.URL's own rules,
`boost::urls::ipv4_address_rule` and `boost::urls::ipv6_address_rule`, which
are also the rules a grammar of a program's own composes. An IPv4 address is
RFC 2673's dotted quad, each part 0 to 255 without a leading zero (RFC
3986's `dec-octet`), so `192.168.0.01` is invalid. An IPv6 address is one of
RFC 4291 2.2's three text forms, without a zone id or a prefix length.
```

  `doc/uris.adoc`, after `addresses.adoc`'s place:

```
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

[#formats-uris]
== URIs and IRIs

`is_uri` is Boost.URL's `parse_uri`: RFC 3986's `URI`, a scheme required and
a fragment allowed. `is_uri_reference` is Boost.URL's `parse_uri_reference`,
a URI or a relative reference, with one guard. RFC 3986 4.2 gives a relative
reference that has an authority a `path-abempty`, empty or led by `/`.
Boost.URL 1.92 accepts `//example.com:abc/p`, read as host `example.com`, an
empty port and the path `abc/p`, and `//a@b@example.com/`, read as user `a`,
host `b` and the path `@example.com/`. With a scheme it refuses both. formats
refuses a relative reference whose path after an authority is neither empty
nor led by `/`; the fix is proposed upstream, and the guard leaves with the
first Boost release that carries it.

A zone identifier of RFC 6874, `http://[fe80::1%25eth0]/`, is valid, as
Boost.URL says: RFC 3986 alone has no zone identifiers, and JSON Schema's
suite does not test one (the appendix "Known deviations").

Boost.URL is a compiled library: a program that uses these formats links
`/webcpp//boost_url`, which `/webcpp/formats//formats` brings. Measured on
2026-10-09 with Boost 1.92 and wasi-sdk 34, a stripped wasm32-wasip2 program
that calls `is_uri` is <difference> bytes larger than an empty one.
```

  `<difference>` is the number Step 5 measured, written as a number.

- [ ] **Step 7: The upstream issue.** It is drafted once, by the infra
  plan's Task 6, in `~/.buzz/OUTBOX/BOOST_URL_RELATIVE_REFERENCE_ISSUE.md`;
  formats writes no draft of its own. Read it: if it lacks something this
  task measured (Boost.URL's `encoded_host` and `encoded_path` of the two
  inputs, as `boost_url_still_needs_the_guard` pins them, or the suite's two
  test descriptions), the report says so to the controller, who has the
  infra track add it.

- [ ] **Step 8: AGENTS.md.** It gains:

```markdown
- **The uri-reference guard.** `detail/uri_guard.hpp` refuses what Boost.URL
  1.92's `uri_reference_rule` wrongly accepts: a relative reference whose
  path after an authority is not empty and not led by `/`. The fix is offered
  upstream (the owner's issue on boostorg/url); `uri_test`'s
  `boost_url_still_needs_the_guard` fails the day the configured Boost
  refuses those inputs itself, and the guard then leaves in a change of its
  own, with a changelog line, while `the_guard` stays.
- **Boost.URL's compiled part.** `/webcpp//boost_url`, which `build.jam`
  names, builds Boost.URL from Boost's sources for every target: `ipv4_address`
  is defined there. On MSVC, which never defines
  `BOOST_URL_HAS_CXX20_CONSTEXPR`, even the grammar's error category is, so no
  format links without it there.
```

- [ ] **Step 9: Register with the suite** (needs Task 4 integrated: rebase
  `task/uris` onto `main` first). In `test/suite_test.cpp`, `validators`
  gains, sorted by format:

```cpp
    mapping{"ipv4", [](std::string_view s) { return formats::is_ipv4(s); }},
    mapping{"ipv6", [](std::string_view s) { return formats::is_ipv6(s); }},
    mapping{"uri", [](std::string_view s) { return formats::is_uri(s); }},
    mapping{"uri-reference", [](std::string_view s) { return formats::is_uri_reference(s); }},
```

  and `pending` loses `"ipv4"`, `"ipv6"`, `"uri"`, `"uri-reference"`. Run the native and emscripten lanes: `suite` passes,
  and each dialect's `string tests` grows by, and `pending` falls by,
  draft4 121, draft6 146, draft7 146, draft2019-09 146, draft2020-12 146,
  v1 146.

- [ ] **Step 10: Build the page, run the lint.** The doc build with
  `<t>` = `uris` exits 0; `LINT`: `lint: clean`.

- [ ] **Step 11: Commit** in `~/dev/webcpp-formats-uris/libs/formats`:

```bash
git add build.jam include/webcpp/formats/ip.hpp include/webcpp/formats/uri.hpp \
  include/webcpp/formats/detail/uri_guard.hpp include/webcpp/formats.hpp test/ip_test.cpp \
  test/uri_test.cpp test/Jamfile test/allocation_test.cpp test/suite_test.cpp \
  doc/addresses.adoc doc/uris.adoc doc/formats.adoc AGENTS.md
git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "formats: IP addresses, URIs and URI references through Boost.URL, with the guard of a relative reference's path"
```

The report's Changelog: the "New Libraries" line, unchanged; and, for the
controller, what Step 7 found missing from
`~/.buzz/OUTBOX/BOOST_URL_RELATIVE_REFERENCE_ISSUE.md`, if anything.

### Task 6: Regular expressions (F7) ∥ (superseded by WEBCPP_REGEX_PLAN.md, Task F7)

Superseded: formats' regex code is written on webcpp::regex by
WEBCPP_REGEX_PLAN.md's Task F7 (commit f244094 on `task/regex`); the steps
below are not run.

Worktree slug `regex`, `<base>` = `B-S3`: `lib/formats` rebased onto
`infra/shared` once infra's Task 5 is committed there, which brings the
SRELL mirror with its three commits (SRELL 2026.07 imported exactly, SHA-256
`d384feae20c66b4d65bc240ee2a851aed26ca24dc4d73503eee3e81e238ec20e`; the
no-exceptions patch; the allocator patch, which gives `basic_regex` a
constructor from a `memory_resource*`), the submodule `third_party/libs/srell`,
checked out from the local mirror `~/.buzz/REPOS/srell.git` (Global
Constraints), and the target `/webcpp/third_party//srell`, whose usage
requirements are SRELL's directory as a system include directory,
`<define>SRELL_NO_UBMOD`, and `SRELL_NO_THROW` in a build with
`exception-handling=off`. Since infra builds S3 last, `B-S3` carries every
shared item. Needs S3 and Task 1; its last
step needs Task 4 integrated. ∥ Tasks 2 to 5 and 7 to 9.

**Files:**
- Create: `include/webcpp/formats/regex.hpp`, `test/regex_test.cpp`, `doc/regex.adoc`
- Modify: `build.jam`, `meta/include-boundaries.json`, `test/Jamfile`, `test/allocation_test.cpp`, `test/suite_test.cpp`, `doc/formats.adoc`, `AGENTS.md`

**Interfaces:**
- Consumes: S3's `<library>/webcpp/third_party//srell`, `<srell.hpp>`, `srell::u8cregex`, its constructor `explicit basic_regex(srell::memory_resource*)` (`std::pmr::memory_resource` in C++17 and later), `assign(const char*, std::size_t, flag_type)`, `ecode()`, `srell::regex_constants::ECMAScript | quiet`; Task 1's `detail::decode_utf8`.
- Produces (the spec's section 4.1 with the owner's 2026-10-09 overload):

```cpp
// <webcpp/formats/regex.hpp>, the only header that includes SRELL
namespace webcpp::formats {
[[nodiscard]] bool is_regex(std::string_view pattern, std::pmr::memory_resource& resource);
[[nodiscard]] bool is_regex(std::string_view pattern);   // the default resource
}
```

- [ ] **Step 1: Write the failing test** `test/regex_test.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// ECMA-262 patterns in u mode, through SRELL: SRELL's \A, \Z and \z refused, an escaped
// backslash before them accepted, a leading (?i) refused (SRELL_NO_UBMOD), ill-formed UTF-8
// refused, NUL a literal, ten thousand groups side by side and nested answered on every target,
// and every allocation through the resource given.

#include <webcpp/formats/regex.hpp>

#include <boost/core/lightweight_test.hpp>

#include <iostream>
#include <string>
#include <string_view>

#include "counting_resource.hpp"

namespace formats = webcpp::formats;

namespace {

void regex(std::string_view pattern, bool valid) {
    if (!BOOST_TEST_EQ(formats::is_regex(pattern), valid)) {
        std::cerr << "regex: \"" << pattern << "\"\n";
    }
}

void ecma_262_patterns() {
    regex("^[a-z]+$", true);
    regex("\\p{Letter}+", true);
    regex("(?<year>\\d{4})-(?<month>\\d{2})", true);
    regex("(?<=\\$\\d*)x", true);
    regex("^(abc]", false);
    regex("a{,5}", false);
    regex("a++", false);
    regex("[[:alpha:]]", false);
    regex("(?i:a)b", true);
    regex("", true);
}

void srell_extensions_are_refused() {
    regex("\\A", false);
    regex("abc\\Z", false);
    regex("abc\\z", false);
    regex("[\\z]", false);
    regex("\\\\A", true);
    regex("\\\\\\A", false);
    regex("(?i)abc", false);
}

void utf8_and_nul() {
    regex("caf\xC3\xA9", true);
    regex("\xF0\x9F\x90\xB2+", true);
    regex("caf\xC3", false);
    regex("a\xC0\xAF", false);
    regex("\xED\xA0\x80", false);
    regex(std::string_view("a\0b", 3), true);
}

/** The verdicts on hostile patterns are those measured on every target, wasm's and emscripten's 64 KiB default stacks included: each pattern compiles or is refused, never traps. */
void hostile_patterns() {
    std::string side_by_side;
    for (int i = 0; i < 100'000; ++i) {
        side_by_side += "(a)";
    }
    regex(side_by_side, true);
    std::string alternatives = "a";
    for (int i = 0; i < 100'000; ++i) {
        alternatives += "|a";
    }
    regex(alternatives, true);
    const std::string nested = std::string(10'000, '(') + "a" + std::string(10'000, ')');
    regex(nested, true);
    std::string quantified;
    for (int i = 0; i < 10'000; ++i) {
        quantified += "(?:";
    }
    quantified += "a";
    for (int i = 0; i < 10'000; ++i) {
        quantified += ")*";
    }
    regex(quantified, true);
    std::string lookaheads;
    for (int i = 0; i < 10'000; ++i) {
        lookaheads += "(?=";
    }
    lookaheads += "a";
    lookaheads.append(10'000, ')');
    regex(lookaheads, true);
}

void every_allocation_goes_through_the_resource() {
    webcpp::test::counting_resource resource;
    BOOST_TEST(formats::is_regex("(?<y>\\d{4})-\\p{Letter}+", resource));
    BOOST_TEST_GT(resource.allocations, 0U);
    BOOST_TEST_EQ(resource.deallocations, resource.allocations);
    webcpp::test::counting_resource refusals;
    BOOST_TEST(!formats::is_regex("^(abc]", refusals));
    BOOST_TEST_EQ(refusals.deallocations, refusals.allocations);
    // The allocation spike measured 23 allocations compiling ^[a-z]+$, which the allocator patch
    // routes through the resource with the same count: an allocation that escaped to malloc,
    // which the replaced operator new cannot see, would lower it.
    webcpp::test::counting_resource exact;
    BOOST_TEST(formats::is_regex("^[a-z]+$", exact));
    BOOST_TEST_EQ(exact.allocations, 23U);
}

}  // namespace

int main() {
    ecma_262_patterns();
    srell_extensions_are_refused();
    utf8_and_nul();
    hostile_patterns();
    every_allocation_goes_through_the_resource();
    return boost::report_errors();
}
```

`test/Jamfile`, after the URI tests:

```
webcpp.run regex : regex_test.cpp ;
```

`test/allocation_test.cpp` gains, with `#include <webcpp/formats/regex.hpp>`
and its call:

```cpp
/**
 is_regex allocates only through the resource it is given: every global allocation of the call is
 one of the counting resource's own, which takes its memory from the global heap.
*/
void a_regex_allocates_only_through_its_resource() {
    webcpp::test::counting_resource resource;
    const std::size_t before = global_allocations();
    BOOST_TEST(webcpp::formats::is_regex("(?<y>\\d{4})-(?<m>\\d{2})|\\p{Script=Greek}+", resource));
    BOOST_TEST(!webcpp::formats::is_regex("^(abc]", resource));
    BOOST_TEST_GT(resource.allocations, 0U);
    BOOST_TEST_EQ(global_allocations() - before, resource.allocations);
}
```

- [ ] **Step 2: Run it and watch it fail.**
  `B2 --build-dir=bin/formats-regex-native libs/formats/test//regex`.
  Expected: FAIL, `'webcpp/formats/regex.hpp' file not found`.

- [ ] **Step 3: Write the header, and bring SRELL.** `build.jam`'s alias
  gains `<library>/webcpp/third_party//srell` as its last usage requirement,
  and its comment, after the clause on Boost.URL, "; and SRELL, which only
  <webcpp/formats/regex.hpp> includes" (the order of Global Constraints). `meta/include-boundaries.json`'s
  first boundary gains
  `"except": ["include/webcpp/formats/regex.hpp"],` after its `headers`.

`include/webcpp/formats/regex.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 Regular expressions: whether a string is an ECMA-262 pattern in `u` mode, through SRELL. The one
 header of formats that includes SRELL, so `<webcpp/formats.hpp>` leaves it out.

 @see "Regular expressions", in the guide.
*/
#ifndef WEBCPP_FORMATS_REGEX_HPP
#define WEBCPP_FORMATS_REGEX_HPP

#include <webcpp/formats/config.hpp>

#include <webcpp/formats/detail/utf8.hpp>

#include <srell.hpp>

#include <cstddef>
#include <memory_resource>
#include <string_view>

namespace webcpp::formats {

namespace detail {

/** Whether `pattern` is well-formed UTF-8: SRELL's handling of anything else is not ECMA-262's. */
constexpr bool is_utf8(std::string_view pattern) noexcept {
    std::size_t i = 0;
    char32_t cp = 0;
    while (i < pattern.size()) {
        if (!decode_utf8(pattern, i, cp)) {
            return false;
        }
    }
    return true;
}

/** Whether `pattern` holds \A, \Z or \z, an unescaped backslash before one of them, which SRELL accepts as extensions and ECMA-262's u mode refuses. */
constexpr bool has_srell_anchor(std::string_view pattern) noexcept {
    for (std::size_t i = 0; i + 1 < pattern.size(); ++i) {
        if (pattern[i] != '\\') {
            continue;
        }
        const char escaped = pattern[i + 1];
        if (escaped == 'A' || escaped == 'Z' || escaped == 'z') {
            return true;
        }
        // The escaped character starts no escape of its own: "\\A" is a backslash, then A.
        ++i;
    }
    return false;
}

}  // namespace detail

/**
 Whether `pattern` is an ECMA-262 regular expression in `u` mode, JSON Schema's `regex`.

 The pattern must be well-formed UTF-8, must not hold SRELL's extensions `\A`, `\Z` and `\z`, and
 must compile with SRELL in its ECMAScript grammar, `SRELL_NO_UBMOD` refusing a leading `(?i)`.
 Every allocation SRELL makes to compile it goes through `resource`.

 @param pattern Any byte string.
 @param resource Where SRELL takes the memory of the compilation, all of it given back before the
 function returns.
 @return Whether the pattern compiles.
 @throws What `resource` throws when it cannot allocate (`std::bad_alloc` from the default one);
 formats raises nothing of its own.
 @see "Regular expressions", in the guide.
*/
[[nodiscard]] inline bool is_regex(std::string_view pattern, std::pmr::memory_resource& resource) {
    if (!detail::is_utf8(pattern) || detail::has_srell_anchor(pattern)) {
        return false;
    }
    srell::u8cregex compiled(&resource);
    compiled.assign(pattern.data(), pattern.size(),
                    srell::regex_constants::ECMAScript | srell::regex_constants::quiet);
    return compiled.ecode() == 0;
}

/**
 Whether `pattern` is an ECMA-262 regular expression in `u` mode, its compilation's memory taken
 from `std::pmr::get_default_resource()`.

 @param pattern Any byte string.
 @return Whether the pattern compiles.
 @throws What the default resource throws when it cannot allocate.
 @see "Regular expressions", in the guide.
*/
[[nodiscard]] inline bool is_regex(std::string_view pattern) {
    return is_regex(pattern, *std::pmr::get_default_resource());
}

}  // namespace webcpp::formats

#endif  // WEBCPP_FORMATS_REGEX_HPP
```

- [ ] **Step 4: Measure the hostile patterns** (Review Focus 1). Run the
  test on the five lanes with `<t>` = `regex`, and read each pattern's
  verdict of `hostile_patterns` on each, wasip2, wasip3 and emscripten
  above all (their default stack is 64 KiB). Read also the count
  `every_allocation_goes_through_the_resource` pins: if a lane counts other
  than 23 (the spike built the regex from its pattern, `is_regex` builds it
  with its resource and then assigns), the report gives the count and the
  reason, and the test pins the count measured, the same on the five lanes.
  - **If every lane passes,** SRELL compiles each shape within the default
    stack on every target: keep each `true`, and record the measurement in
    the ledger.
  - **If a lane traps or crashes** (wasmtime's `call stack exhausted`,
    node's `RangeError: Maximum call stack size exceeded`, a segmentation
    fault) on a shape: find the largest size of that shape that passes on
    wasip2, wasip3 and emscripten at b2's debug flags, by halving N in a
    scratch program that calls `is_regex` on it (never committed), as pratt
    measured its depth. Add to `regex.hpp`'s `detail` a bound for that
    shape, `inline constexpr std::size_t max_regex_nesting` (the depth of
    unescaped `(` outside a character class, which bounds the nested
    groups, quantified groups and lookaheads alike) or
    `max_regex_alternatives` (the count of unescaped `|` outside a class),
    each half the smallest N measured, rounded down to a power of two, with
    the measurement in its comment, and a guard in `is_regex`, after the
    UTF-8 check, that answers `false` beyond it. The shape's line becomes
    `regex(..., false);`, with a case at exactly the bound expecting `true`.
    The page's "Regular expressions" and "Known deviations" state each bound
    and why.
  - Either way, every lane is green before Step 5.

- [ ] **Step 5: Run it and watch it pass** on the five lanes, `regex` and
  `allocation` passed. Then see the extensions' guard fail: make
  `has_srell_anchor` return `false` and run `regex` natively. Expected: FAIL,
  `regex: "\A"`, `regex: "abc\Z"` and `regex: "abc\z"` named on the
  standard error, since SRELL accepts them. Restore it. Expected also:
  `LINT` analyses `regex.hpp` without exceptions (its twin) and stays clean;
  the include boundary holds: plant `#include <srell.hpp>` in `uri.hpp`, see
  `lint: failed: include boundaries` naming the line and "only regex.hpp
  depends on SRELL", remove it.

- [ ] **Step 6: The page.** `doc/regex.adoc`, after `templates.adoc`'s place
  (after `uris.adoc` until Task 7 lands):

```
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

[#formats-regex]
== Regular expressions

`is_regex` answers whether a string is an ECMA-262 regular expression in
`u` mode, the `regex` of JSON Schema, by compiling it with SRELL, a
header-only ECMAScript engine that reads UTF-8 directly. It is declared in
`<webcpp/formats/regex.hpp>`, the one header of formats that includes SRELL:
`<webcpp/formats.hpp>` leaves it out, and a program that never validates a
pattern does not compile the engine.

SRELL comes from the mirror `webcpporg/srell`: SRELL 2026.07 imported as
published, a patch that builds it without exceptions, and a patch that takes
every allocation from a `std::pmr::memory_resource`, both offered upstream.
Its macro `SRELL_NO_UBMOD`, set once by its target, refuses a leading
`(?i)`. SRELL also accepts `\A`, `\Z` and `\z`, extensions with no macro to
turn them off, which ECMA-262 refuses in `u` mode: formats refuses them
itself, and an escaped backslash before one (`\\A`) stays valid.

`is_regex(pattern, resource)` compiles in the memory of `resource` and gives
it all back before returning; `is_regex(pattern)` uses the default resource.
Nothing else in SRELL allocates.

A regex has no rule: SRELL compiles a whole string and reports no position,
so a rule could only consume all of its input, which composes with nothing.
```

- [ ] **Step 7: AGENTS.md.** It gains:

```markdown
- **SRELL.** `<webcpp/formats/regex.hpp>` is the only header that includes
  SRELL, which `meta/include-boundaries.json` holds. SRELL comes from the
  mirror `webcpporg/srell` (submodule `third_party/libs/srell`): the 2026.07
  zip imported exactly, then the no-exceptions patch, then the allocator
  patch, each a commit of its own offered upstream. The target
  `/webcpp/third_party//srell` gives SRELL's directory as a system one (its
  warnings are not formats'), `SRELL_NO_UBMOD` always, and `SRELL_NO_THROW` in
  a build without exceptions; no header of formats defines them, so formats
  and json_schema compile SRELL alike. formats refuses `\A`, `\Z` and `\z`
  itself, until SRELL offers a switch, and checks UTF-8 itself, since SRELL's
  handling of ill-formed UTF-8 is unmeasured.
```

- [ ] **Step 8: Register with the suite** (needs Task 4 integrated).
  `test/suite_test.cpp` gains `#include <webcpp/formats/regex.hpp>` after
  `<webcpp/formats.hpp>`, and `validators` gains, sorted:

```cpp
    mapping{"regex", [](std::string_view s) { return formats::is_regex(s); }},
```

  `pending` loses `"regex"`. The native and
  emscripten lanes: `suite` passes, and each dialect's `string tests` grows
  by, and `pending` falls by, draft4 0, draft6 0, draft7 14,
  draft2019-09 14, draft2020-12 14, v1 14.

- [ ] **Step 9: Build the page, run the lint.** The doc build with
  `<t>` = `regex` exits 0; `LINT`: `lint: clean`.

- [ ] **Step 10: Commit** in `~/dev/webcpp-formats-regex/libs/formats`:

```bash
git add build.jam meta/include-boundaries.json include/webcpp/formats/regex.hpp test/regex_test.cpp \
  test/Jamfile test/allocation_test.cpp test/suite_test.cpp doc/regex.adoc doc/formats.adoc AGENTS.md
git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "formats: regular expressions through SRELL, in their own header, allocating only through the resource given"
```

The report's Changelog: the "New Libraries" line, unchanged.

### Task 7: IRIs and URI templates (F4) ∥

Worktree slug `iris`, `<base>` = `B-S4` or later (the latest base when it starts).
Needs Tasks 4 and 5 integrated. ∥ Tasks 6 and 8.

**Files:**
- Create: `include/webcpp/formats/detail/iri_mapping.hpp`, `include/webcpp/formats/uri_template.hpp`, `test/iri_test.cpp`, `test/iri_resource_test.cpp`, `test/uri_template_test.cpp`, `doc/templates.adoc`
- Modify: `include/webcpp/formats/uri.hpp` (the IRI half), `include/webcpp/formats.hpp`, `test/Jamfile`, `test/allocation_test.cpp`, `test/suite_test.cpp`, `doc/uris.adoc`, `doc/formats.adoc`, `AGENTS.md`

**Interfaces:**
- Consumes: Task 5's `grammar::uri_reference_rule`, `boost::urls::parse_uri`; Task 1's `detail::decode_utf8`, `is_ucschar`, `is_iprivate`, `is_bidi_format`, `template_literal_chars`, `varchar_chars`, `template_operator_chars`; Boost.URL's `tuple_rule`, `variant_rule`, `optional_rule`, `range_rule`, `not_empty_rule`, `delim_rule`, `ref` and `boost::urls::pct_encoded_rule`, from which the template's expression is built (spec 5.9).
- Produces:

```cpp
namespace webcpp::formats {
[[nodiscard]] bool is_iri(std::string_view s);
[[nodiscard]] bool is_iri(std::string_view s, std::pmr::memory_resource& resource);
[[nodiscard]] bool is_iri_reference(std::string_view s);
[[nodiscard]] bool is_iri_reference(std::string_view s, std::pmr::memory_resource& resource);
[[nodiscard]] bool is_uri_template(std::string_view s) noexcept;
}
namespace webcpp::formats::grammar {
struct iri_rule_t {             // value_type text
    std::pmr::memory_resource* resource = nullptr;
    result<text> parse(char const*&, char const*) const;
};
struct iri_reference_rule_t {   // value_type text, the same member
    std::pmr::memory_resource* resource = nullptr;
    result<text> parse(char const*&, char const*) const;
};
struct uri_template_rule_t;     // value_type text, parse noexcept
inline constexpr iri_rule_t iri_rule{};
inline constexpr iri_reference_rule_t iri_reference_rule{};
inline constexpr uri_template_rule_t uri_template_rule{};
}
namespace webcpp::formats::detail {
inline constexpr std::size_t iri_inline_capacity = 1024;
constexpr char const* scan_iri(char const* first, char const* end) noexcept;
constexpr std::optional<std::size_t> mapped_length(std::string_view run) noexcept;
constexpr void map_iri(std::string_view run, std::span<char> out) noexcept;
template <class Parse>
bool with_mapped_iri(std::string_view run, std::pmr::memory_resource* resource, Parse parse);
}
```

- [ ] **Step 1: Write the failing tests.** `test/iri_test.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// RFC 3987's IRI and IRI reference, mapped to a URI that Boost.URL parses: 27 probes of the
// corners the suite misses, the inline buffer's edge, an ASCII IRI
// that never touches the resource, NUL, and where the rules stop inside a larger grammar.

#include <webcpp/formats/uri.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/url/grammar/delim_rule.hpp>
#include <boost/url/grammar/error.hpp>
#include <boost/url/grammar/parse.hpp>
#include <boost/url/grammar/tuple_rule.hpp>

#include <iostream>
#include <memory_resource>
#include <string>
#include <string_view>
#include <tuple>

#include "counting_resource.hpp"

namespace formats = webcpp::formats;
namespace fg = webcpp::formats::grammar;
namespace urlg = boost::urls::grammar;

namespace {

void iri(std::string_view text, bool valid) {
    if (!BOOST_TEST_EQ(formats::is_iri(text), valid)) {
        std::cerr << "iri: \"" << text << "\"\n";
    }
    BOOST_TEST_EQ(formats::is_iri(text), urlg::parse(text, fg::iri_rule).has_value());
}

void reference(std::string_view text, bool valid) {
    if (!BOOST_TEST_EQ(formats::is_iri_reference(text), valid)) {
        std::cerr << "iri-reference: \"" << text << "\"\n";
    }
    BOOST_TEST_EQ(formats::is_iri_reference(text),
                  urlg::parse(text, fg::iri_reference_rule).has_value());
}

void corner_probes() {
    iri("http://1.2.3.4.example.com/", true);
    iri("http://example.com:8o/", false);
    iri("http://ex ample.com/", false);
    iri("http://a/\xE2\x80\x8E", false);  // U+200E: RFC 3987 4.1's MUST NOT
    iri("http://a/\xEF\xBF\xBD", false);  // U+FFFD, outside ucschar
    iri("http://a/\xC2\x80", false);      // U+0080, a C1 control
    iri("http://a/\xC2\xA0", true);       // U+00A0, the first ucschar
    iri("http://a/\xEE\x80\x80", false);  // U+E000, iprivate in a path
    iri("http://a/?\xEE\x80\x80", true);  // in a query
    iri("http://a/#\xEE\x80\x80", false); // in a fragment
    iri("http://[v1.x]/", true);
    // RFC 6874's zone id: Boost.URL's verdict, which formats keeps (the page's known deviations).
    iri("http://[::1%25eth0]/", true);
    iri("http://a/\xC3", false);
    iri("http://a/\xC0\xAF", false);
    iri("http://a/\xED\xA0\x80", false);
    iri("http://\xC6\x92\xC3\xB8\xC3\xB8.\xC3\x9F\xC3\xA5r:/", true);  // an empty port
    iri("HTTP://a", true);
    iri("1http://a", false);
    iri("a:", true);
    iri("http://a:b@c:d@e/", false);
    reference("//a@b@c/", false);
    reference("//a:abc/p", false);
    reference("", true);
    reference("./\xC3\xA9:x", true);
    reference("\xC3\xA9:x", false);
    reference("#\xEE\x80\x80", false);
    reference("a b", false);
}

void non_ascii_only_where_rfc_3986_has_pct_encoded() {
    iri("h\xC3\xA9:x", false);                 // a scheme
    iri("http://a:8\xC3\xA9/", false);         // a port
    iri("http://[::\xC3\xA9]/", false);        // an IP literal
    iri("http://\xC3\xA9@\xC3\xA9/\xC3\xA9?\xC3\xA9#\xC3\xA9", true);
    reference("//\xC6\x92\xC3\xB8\xC3\xB8.\xC3\x9F\xC3\xA5r/?\xE2\x88\x82", true);
    iri(std::string_view("http://a/\0", 10), false);
}

void the_inline_buffer_and_the_resource() {
    // "http://a/" is 9 octets, each "é" maps to 6 and "x" to 1: 169 "é" and one "x" map to 1,024.
    std::string at_capacity = "http://a/";
    for (int i = 0; i < 169; ++i) {
        at_capacity += "\xC3\xA9";
    }
    at_capacity += 'x';
    webcpp::test::counting_resource inline_buffer;
    BOOST_TEST(formats::is_iri(at_capacity, inline_buffer));
    BOOST_TEST_EQ(inline_buffer.allocations, 0U);
    webcpp::test::counting_resource beyond;
    BOOST_TEST(formats::is_iri(at_capacity + "x", beyond));
    BOOST_TEST_EQ(beyond.allocations, 1U);
    BOOST_TEST_EQ(beyond.bytes, 1025U);
    BOOST_TEST_EQ(beyond.deallocations, 1U);
    // Any IRI of up to 341 octets maps within the buffer: "a:", 168 "é" and one "€" are 341.
    std::string worst = "a:";
    for (int i = 0; i < 168; ++i) {
        worst += "\xC3\xA9";
    }
    worst += "\xE2\x82\xAC";
    BOOST_TEST_EQ(worst.size(), 341U);
    webcpp::test::counting_resource none;
    BOOST_TEST(formats::is_iri_reference(worst, none));
    BOOST_TEST_EQ(none.allocations, 0U);
    // The spec's edge in non-ASCII octets alone: 341 map to 1,023, inline; 342 map to 1,026, one
    // buffer of exactly that length.
    std::string octets341;
    for (int i = 0; i < 113; ++i) {
        octets341 += "\xE2\x82\xAC";
    }
    octets341 += "\xC3\xA9";
    BOOST_TEST_EQ(octets341.size(), 341U);
    webcpp::test::counting_resource stays_inline;
    BOOST_TEST(formats::is_iri_reference(octets341, stays_inline));
    BOOST_TEST_EQ(stays_inline.allocations, 0U);
    std::string octets342;
    for (int i = 0; i < 171; ++i) {
        octets342 += "\xC3\xA9";
    }
    webcpp::test::counting_resource one_buffer;
    BOOST_TEST(formats::is_iri_reference(octets342, one_buffer));
    BOOST_TEST_EQ(one_buffer.allocations, 1U);
    BOOST_TEST_EQ(one_buffer.bytes, 1026U);
}

void an_ascii_iri_never_touches_the_resource() {
    std::string long_ascii = "http://example.com/";
    long_ascii.append(1'000'000, 'a');
    BOOST_TEST(formats::is_iri(long_ascii, *std::pmr::null_memory_resource()));
    BOOST_TEST(formats::is_iri_reference(long_ascii, *std::pmr::null_memory_resource()));
}

void the_rules_inside_a_grammar() {
    // An IRI ends at the first byte no IRI holds, here a space, with non-ASCII text on each side.
    const auto pair = urlg::parse(
        "http://a/\xC3\xA9 \xC3\xA9tail",
        urlg::tuple_rule(fg::iri_rule, urlg::squelch(urlg::delim_rule(' ')), fg::iri_reference_rule));
    if (BOOST_TEST(pair.has_value())) {
        BOOST_TEST_EQ(std::get<0>(*pair), "http://a/\xC3\xA9");
        BOOST_TEST_EQ(std::get<1>(*pair), "\xC3\xA9tail");
    }
    const std::string_view refused = "http://a/\xE2\x80\x8E rest";
    char const* it = refused.data();
    const auto r = fg::iri_rule.parse(it, refused.data() + refused.size());
    BOOST_TEST(r.has_error() && r.error() == urlg::error::invalid);
    BOOST_TEST(it == refused.data());
    const std::string_view none = " x";
    char const* at = none.data();
    const auto m = fg::iri_rule.parse(at, none.data() + none.size());
    BOOST_TEST(m.has_error() && m.error() == urlg::error::mismatch);
}

}  // namespace

int main() {
    corner_probes();
    non_ascii_only_where_rfc_3986_has_pct_encoded();
    the_inline_buffer_and_the_resource();
    an_ascii_iri_never_touches_the_resource();
    the_rules_inside_a_grammar();
    return boost::report_errors();
}
```

`test/iri_resource_test.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// A resource that refuses to allocate makes is_iri throw what it throws, never answer a verdict
// without its mapped copy. Built with exceptions whatever the build asks: it catches on purpose.

#include <webcpp/formats/uri.hpp>

#include <boost/core/lightweight_test.hpp>

#include <memory_resource>
#include <new>
#include <string>

namespace formats = webcpp::formats;

int main() {
    std::string beyond = "http://a/";
    for (int i = 0; i < 200; ++i) {
        beyond += "\xC3\xA9";
    }
    bool thrown = false;
    try {
        static_cast<void>(formats::is_iri(beyond, *std::pmr::null_memory_resource()));
    } catch (const std::bad_alloc&) {
        thrown = true;
    }
    BOOST_TEST(thrown);
    return boost::report_errors();
}
```

`test/uri_template_test.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// RFC 6570's URI Template with erratum 6937: section 1.2's templates of levels 1 to 4, the
// 22 probes of the corners the suite misses, NUL, and where the rule
// stops.

#include <webcpp/formats/uri_template.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/url/grammar/error.hpp>
#include <boost/url/grammar/parse.hpp>

#include <iostream>
#include <string_view>

namespace formats = webcpp::formats;
namespace fg = webcpp::formats::grammar;
namespace urlg = boost::urls::grammar;

namespace {

void uri_template(std::string_view text, bool valid) {
    if (!BOOST_TEST_EQ(formats::is_uri_template(text), valid)) {
        std::cerr << "uri-template: \"" << text << "\"\n";
    }
    BOOST_TEST_EQ(formats::is_uri_template(text),
                  urlg::parse(text, fg::uri_template_rule).has_value());
}

void rfc_6570_section_1_2() {
    for (const std::string_view text :
         {"{var}", "{hello}", "{+var}", "{+hello}", "{+path}/here", "here?ref={+path}", "X{#var}",
          "X{#hello}", "map?{x,y}", "{x,hello,y}", "{+x,hello,y}", "{+path,x}/here",
          "{#x,hello,y}", "{#path,x}/here", "X{.var}", "X{.x,y}", "{/var}", "{/var,x}/here",
          "{;x,y}", "{;x,y,empty}", "{?x,y}", "{?x,y,empty}", "?fixed=yes{&x}", "{&x,y,empty}",
          "{var:3}", "{var:30}", "{list}", "{list*}", "{keys}", "{keys*}", "{+path:6}/here",
          "{+list}", "{+list*}", "{+keys}", "{+keys*}", "{#path:6}/here", "{#list}", "{#list*}",
          "{#keys}", "{#keys*}", "X{.var:3}", "X{.list}", "X{.list*}", "X{.keys}", "X{.keys*}",
          "{/var:1,var}", "{/list}", "{/list*}", "{/list*,path:4}", "{/keys}", "{/keys*}",
          "{;hello:5}", "{;list}", "{;list*}", "{;keys}", "{;keys*}", "{?var:3}", "{?list}",
          "{?list*}", "{?keys}", "{?keys*}", "{&var:3}", "{&list}", "{&list*}", "{&keys}",
          "{&keys*}"}) {
        uri_template(text, true);
    }
}

void corner_probes() {
    uri_template("{", false);
    uri_template("}", false);
    uri_template("{{var}}", false);
    uri_template("{v:9999}", true);
    uri_template("{v*:3}", false);
    uri_template("{a b}", false);
    uri_template("{_a}", true);
    uri_template("{a.}", false);
    uri_template("{..a}", false);
    uri_template("{-var}", false);
    uri_template("{\xC3\xA4}", false);
    uri_template("a<b", false);
    uri_template("a\"b", false);
    uri_template("a^b", false);
    uri_template("a|b", false);
    uri_template("a\\b", false);
    uri_template("a`b", false);
    uri_template("a\xC2\x80" "b", false);
    uri_template("a\xEF\xBF\xBD" "b", false);
    uri_template("a\xC3", false);
    uri_template("{v:5}{w*}/\xC3\xA9{?x,y}", true);
    uri_template("{v:}", false);
}

void literals_and_names() {
    uri_template("", true);
    uri_template("a'b", true);           // erratum 6937
    uri_template("%20{v}%2f", true);
    uri_template("a%", false);
    uri_template("a%GG", false);
    uri_template("{%41b}", true);
    uri_template("{%}", false);
    uri_template("{v:10000}", false);
    uri_template("{v:0}", false);
    uri_template("{a,}", false);
    uri_template("{a..b}", false);
    uri_template("{a.b.c}", true);
    uri_template("/\xEE\x80\x80/\xF3\xB0\x80\x80", true);  // iprivate, BMP and plane 15
    uri_template(std::string_view("a\0b", 3), false);
}

void where_the_rule_stops() {
    const std::string_view text = "{a}/b }rest";
    char const* it = text.data();
    const auto r = fg::uri_template_rule.parse(it, text.data() + text.size());
    BOOST_TEST(r.has_value() && *r == "{a}/b");
    const std::string_view broken = "x{a";
    char const* at = broken.data();
    const auto b = fg::uri_template_rule.parse(at, broken.data() + broken.size());
    BOOST_TEST(b.has_error() && b.error() == urlg::error::invalid);
    BOOST_TEST(at == broken.data());
}

}  // namespace

int main() {
    rfc_6570_section_1_2();
    corner_probes();
    literals_and_names();
    where_the_rule_stops();
    return boost::report_errors();
}
```

`test/Jamfile`, after the regex line's place:

```
webcpp.run iri : iri_test.cpp ;
# A resource that refuses: is_iri propagates its std::bad_alloc, caught on
# purpose, so the program is built with exceptions whatever the build asks.
webcpp.run iri_resource : iri_resource_test.cpp : <exception-handling>on ;
webcpp.run uri_template : uri_template_test.cpp ;
```

`test/allocation_test.cpp` gains, with `#include <webcpp/formats/uri_template.hpp>`
and its call:

```cpp
/**
 IRIs allocate nothing for ASCII and up to 1,024 mapped octets, and beyond them one buffer from the
 resource given; templates allocate nothing, 100,000 expressions included.
*/
void iris_and_templates_allocate_within_their_bounds() {
    std::string within = "a:";
    for (int i = 0; i < 168; ++i) {
        within += "\xC3\xA9";
    }
    within += "\xE2\x82\xAC";
    std::string long_ascii = "http://example.com/";
    long_ascii.append(1'000'000, 'a');
    std::string beyond = "http://a/";
    for (int i = 0; i < 100'000; ++i) {
        beyond += "\xC3\xA9";
    }
    std::string long_template;
    for (int i = 0; i < 100'000; ++i) {
        long_template += "{+v:12}/x";
    }
    webcpp::test::counting_resource resource;
    const std::size_t before = global_allocations();
    BOOST_TEST(webcpp::formats::is_iri(within));
    BOOST_TEST(webcpp::formats::is_iri(long_ascii));
    BOOST_TEST(webcpp::formats::is_iri_reference("//\xC6\x92\xC3\xB8\xC3\xB8.example/"));
    BOOST_TEST(webcpp::formats::is_uri_template(long_template));
    BOOST_TEST_EQ(global_allocations() - before, 0U);
    BOOST_TEST(webcpp::formats::is_iri(beyond, resource));
    BOOST_TEST_EQ(resource.allocations, 1U);
    BOOST_TEST_EQ(resource.bytes, 9U + 600'000U);
    BOOST_TEST_EQ(global_allocations() - before, 1U);
}
```

- [ ] **Step 2: Run them and watch them fail.**
  `B2 --build-dir=bin/formats-iris-native libs/formats/test//iri libs/formats/test//iri_resource libs/formats/test//uri_template`.
  Expected: FAIL: `is_iri` is no member of `webcpp::formats` (iri_test,
  iri_resource_test), and `'webcpp/formats/uri_template.hpp' file not found`.

- [ ] **Step 3: Write the mapping** `include/webcpp/formats/detail/iri_mapping.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// RFC 3987's IRI, checked and mapped to a URI for Boost.URL: where an IRI ends inside a larger
// grammar, which code points it may hold where (2.2, 4.1), and its mapping (3.1 step 2),
// written into storage that is none for ASCII, an inline buffer up to 1,024 octets, and one buffer
// of a memory resource beyond.

#ifndef WEBCPP_FORMATS_DETAIL_IRI_MAPPING_HPP
#define WEBCPP_FORMATS_DETAIL_IRI_MAPPING_HPP

#include <webcpp/formats/config.hpp>

#include <webcpp/formats/detail/charsets.hpp>
#include <webcpp/formats/detail/utf8.hpp>

#include <array>
#include <cstddef>
#include <memory_resource>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace webcpp::formats::detail {

/** The mapped octets an IRI takes on the stack: any IRI of up to 341 octets, since mapping at most triples a length. */
inline constexpr std::size_t iri_inline_capacity = 1024;

/** Whether `byte` is one no IRI holds: an ASCII control, the space, the double quote, the less-than and greater-than signs, the backslash, the circumflex, the grave accent, a brace or the vertical bar. */
constexpr bool ends_iri(char byte) noexcept {
    const auto b = static_cast<unsigned char>(byte);
    return b <= 0x20U || b == 0x7FU || byte == '"' || byte == '<' || byte == '>' ||
           byte == '\\' || byte == '^' || byte == '`' || byte == '{' || byte == '|' || byte == '}';
}

/** Where the IRI that starts at `first` ends: at the first byte no IRI holds, or at `end`. */
constexpr char const* scan_iri(char const* first, char const* end) noexcept {
    char const* last = first;
    while (last != end && !ends_iri(*last)) {
        ++last;
    }
    return last;
}

/**
 The length of `run` mapped to a URI, or nothing when it holds ill-formed UTF-8, a bidirectional
 formatting character, or a non-ASCII code point that is not ucschar (nor iprivate in the query,
 the text after the first "?" before the first "#").
*/
constexpr std::optional<std::size_t> mapped_length(std::string_view run) noexcept {
    const std::size_t fragment = run.find('#');
    const std::size_t query = run.substr(0, fragment).find('?');
    std::size_t length = 0;
    std::size_t i = 0;
    while (i < run.size()) {
        if (static_cast<unsigned char>(run[i]) < 0x80U) {
            ++length;
            ++i;
            continue;
        }
        const std::size_t start = i;
        char32_t cp = 0;
        if (!decode_utf8(run, i, cp) || is_bidi_format(cp)) {
            return std::nullopt;
        }
        const bool in_query = query != std::string_view::npos && start > query && start < fragment;
        if (!is_ucschar(cp) && !(in_query && is_iprivate(cp))) {
            return std::nullopt;
        }
        length += 3 * (i - start);
    }
    return length;
}

/** Writes `run` mapped into `out`, which holds exactly its mapped length: each octet of a non-ASCII code point as %XX, uppercase. */
constexpr void map_iri(std::string_view run, std::span<char> out) noexcept {
    constexpr std::string_view hex = "0123456789ABCDEF";
    std::size_t o = 0;
    for (const char c : run) {
        const auto byte = static_cast<unsigned char>(c);
        if (byte < 0x80U) {
            out[o++] = c;
            continue;
        }
        out[o++] = '%';
        out[o++] = hex[byte >> 4U];
        out[o++] = hex[byte & 0x0FU];
    }
}

/**
 Checks `run`, maps it, and answers what `parse` answers of the mapped URI: `run` itself when it
 is ASCII, an inline buffer up to iri_inline_capacity, else one buffer from `resource`, or from
 the default resource when it is null.
*/
template <class Parse>
bool with_mapped_iri(std::string_view run, std::pmr::memory_resource* resource, Parse parse) {
    const std::optional<std::size_t> length = mapped_length(run);
    if (!length) {
        return false;
    }
    if (*length == run.size()) {
        return parse(run);
    }
    if (*length <= iri_inline_capacity) {
        std::array<char, iri_inline_capacity> buffer{};
        map_iri(run, std::span<char>(buffer.data(), *length));
        return parse(std::string_view(buffer.data(), *length));
    }
    std::pmr::vector<char> buffer(
        *length, std::pmr::polymorphic_allocator<char>(
                     resource != nullptr ? resource : std::pmr::get_default_resource()));
    map_iri(run, std::span<char>(buffer.data(), buffer.size()));
    return parse(std::string_view(buffer.data(), buffer.size()));
}

}  // namespace webcpp::formats::detail

#endif  // WEBCPP_FORMATS_DETAIL_IRI_MAPPING_HPP
```

- [ ] **Step 4: Write the IRI half of `uri.hpp`.** It gains
  `#include <webcpp/formats/detail/iri_mapping.hpp>`, `<memory_resource>`
  and `<cstddef>`; its file comment becomes "URIs and IRIs (RFC 3986, RFC
  3987), ..."; after `uri_reference_rule_t::parse`'s definition, in
  `namespace grammar`:

```cpp
/**
 The rule of RFC 3987 2.2's IRI: an IRI checked code point by code point, mapped to a URI, and
 parsed by Boost.URL's `parse_uri`.

 A non-ASCII code point must be well-formed UTF-8 and in `ucschar`, or `iprivate` in the query;
 the bidirectional formatting characters are refused anywhere (RFC 3987 4.1). Each octet of a
 non-ASCII code point maps to `%XX`, which RFC 3986 allows where RFC 3987 allows `ucschar`, so a
 non-ASCII scheme, port or IP literal is refused. The rule takes the run up to the first byte no
 IRI holds (an ASCII control, the space, the double quote, the less-than and greater-than signs,
 the backslash, the circumflex, the grave accent, the braces or the vertical bar) and validates it
 whole; it never matches a
 shorter prefix.

 @see "URIs and IRIs", in the guide.
*/
struct iri_rule_t {
    /** The rule yields the text it matched. */
    using value_type = text;

    /** The resource of a mapped URI longer than 1,024 octets; null for the default resource. */
    std::pmr::memory_resource* resource = nullptr;

    /**
     Matches an IRI at `it`.

     @param it The start of the input: past the match on success, else unchanged.
     @param end The end of the input.
     @return The text matched; `error::mismatch` when a byte no IRI holds is at `it`;
     `error::invalid` when the run is no IRI.
     @throws What the resource throws when it cannot allocate.
    */
    [[nodiscard]] result<text> parse(char const*& it, char const* end) const;
};

/**
 The rule of RFC 3987 2.2's IRI-reference: an IRI or a relative reference, checked and mapped as
 @ref iri_rule_t does, then parsed by @ref uri_reference_rule.

 The empty reference is valid, so the rule matches empty text where a byte no IRI holds is at
 the start, and never answers mismatch.

 @see "URIs and IRIs", in the guide.
*/
struct iri_reference_rule_t {
    /** The rule yields the text it matched. */
    using value_type = text;

    /** The resource of a mapped URI longer than 1,024 octets; null for the default resource. */
    std::pmr::memory_resource* resource = nullptr;

    /**
     Matches an IRI-reference at `it`.

     @param it The start of the input: past the match on success, else unchanged.
     @param end The end of the input.
     @return The text matched; `error::invalid` when the run is no IRI reference.
     @throws What the resource throws when it cannot allocate.
    */
    [[nodiscard]] result<text> parse(char const*& it, char const* end) const;
};

/** The rule of RFC 3987's IRI, with the default resource; `iri_rule_t{&resource}` names another. */
inline constexpr iri_rule_t iri_rule{};

/** The rule of RFC 3987's IRI-reference, with the default resource. */
inline constexpr iri_reference_rule_t iri_reference_rule{};

inline result<text> iri_rule_t::parse(char const*& it, char const* end) const {
    char const* const first = it;
    char const* const last = detail::scan_iri(first, end);
    if (last == first) {
        return detail::urlg::error::mismatch;
    }
    const std::string_view run(first, static_cast<std::size_t>(last - first));
    const bool valid = detail::with_mapped_iri(run, resource, [](std::string_view uri) {
        return boost::urls::parse_uri(uri).has_value();
    });
    if (!valid) {
        return detail::urlg::error::invalid;
    }
    it = last;
    return text(first, run.size());
}

inline result<text> iri_reference_rule_t::parse(char const*& it, char const* end) const {
    char const* const first = it;
    char const* const last = detail::scan_iri(first, end);
    const std::string_view run(first, static_cast<std::size_t>(last - first));
    const bool valid = detail::with_mapped_iri(run, resource, [](std::string_view uri) {
        return detail::urlg::parse(uri, uri_reference_rule).has_value();
    });
    if (!valid) {
        return detail::urlg::error::invalid;
    }
    it = last;
    return text(first, run.size());
}
```

  and, after `is_uri_reference`, in `namespace webcpp::formats`:

```cpp
/**
 Whether `s` is an RFC 3987 IRI, JSON Schema's `iri`.

 @param s Any byte string.
 @return `parse(s, grammar::iri_rule).has_value()`.
 @throws What the default resource throws when it cannot allocate, only for an IRI whose mapped
 URI exceeds 1,024 octets.
 @see "URIs and IRIs", in the guide.
*/
[[nodiscard]] inline bool is_iri(std::string_view s) {
    return detail::urlg::parse(s, grammar::iri_rule).has_value();
}

/**
 Whether `s` is an RFC 3987 IRI, a mapped URI beyond 1,024 octets taken from `resource`.

 @param s Any byte string.
 @param resource Where the mapped copy of a long non-ASCII IRI is allocated; an ASCII IRI never
 reaches it.
 @return `parse(s, grammar::iri_rule_t{&resource}).has_value()`.
 @throws What `resource` throws when it cannot allocate.
 @see "URIs and IRIs", in the guide.
*/
[[nodiscard]] inline bool is_iri(std::string_view s, std::pmr::memory_resource& resource) {
    return detail::urlg::parse(s, grammar::iri_rule_t{&resource}).has_value();
}

/**
 Whether `s` is an RFC 3987 IRI-reference, JSON Schema's `iri-reference`.

 @param s Any byte string.
 @return `parse(s, grammar::iri_reference_rule).has_value()`.
 @throws What the default resource throws when it cannot allocate.
 @see "URIs and IRIs", in the guide.
*/
[[nodiscard]] inline bool is_iri_reference(std::string_view s) {
    return detail::urlg::parse(s, grammar::iri_reference_rule).has_value();
}

/**
 Whether `s` is an RFC 3987 IRI-reference, a mapped URI beyond 1,024 octets taken from `resource`.

 @param s Any byte string.
 @param resource Where the mapped copy of a long non-ASCII reference is allocated.
 @return `parse(s, grammar::iri_reference_rule_t{&resource}).has_value()`.
 @throws What `resource` throws when it cannot allocate.
 @see "URIs and IRIs", in the guide.
*/
[[nodiscard]] inline bool is_iri_reference(std::string_view s,
                                           std::pmr::memory_resource& resource) {
    return detail::urlg::parse(s, grammar::iri_reference_rule_t{&resource}).has_value();
}
```

- [ ] **Step 5: Write `include/webcpp/formats/uri_template.hpp`:**

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 URI Templates, RFC 6570 section 2 with erratum 6937, as a validator and a Boost.URL rule.

 @see "URI templates", in the guide.
*/
#ifndef WEBCPP_FORMATS_URI_TEMPLATE_HPP
#define WEBCPP_FORMATS_URI_TEMPLATE_HPP

#include <webcpp/formats/config.hpp>

#include <webcpp/formats/detail/charsets.hpp>
#include <webcpp/formats/detail/utf8.hpp>
#include <webcpp/formats/grammar.hpp>

#include <boost/url/grammar/delim_rule.hpp>
#include <boost/url/grammar/digit_chars.hpp>
#include <boost/url/grammar/error.hpp>
#include <boost/url/grammar/hexdig_chars.hpp>
#include <boost/url/grammar/lut_chars.hpp>
#include <boost/url/grammar/not_empty_rule.hpp>
#include <boost/url/grammar/optional_rule.hpp>
#include <boost/url/grammar/parse.hpp>
#include <boost/url/grammar/range_rule.hpp>
#include <boost/url/grammar/tuple_rule.hpp>
#include <boost/url/grammar/variant_rule.hpp>
#include <boost/url/rfc/pct_encoded_rule.hpp>

#include <cstddef>
#include <string_view>

namespace webcpp::formats {

namespace detail {

namespace urlg = boost::urls::grammar;

/** The length of the pct-encoded triplet at `s[i]`: 3, or 0 when there is none. */
constexpr std::size_t pct_encoded_length(std::string_view s, std::size_t i) noexcept {
    return i < s.size() && s[i] == '%' && s.size() - i >= 3 && urlg::hexdig_chars(s[i + 1]) &&
                   urlg::hexdig_chars(s[i + 2])
               ? 3
               : 0;
}

/** RFC 6570's literals: one or more literal characters, pct-encoded triplets, ucschar or iprivate code points. */
struct template_literals_rule_t {
    /** The rule yields the literals it matched. */
    using value_type = grammar::text;

    /** Matches the literals at `it`, or leaves `it` and answers mismatch when none is there. */
    grammar::result<grammar::text> parse(char const*& it, char const* end) const noexcept {
        const std::string_view rest(it, static_cast<std::size_t>(end - it));
        std::size_t i = 0;
        while (i < rest.size()) {
            if (const std::size_t pct = pct_encoded_length(rest, i); pct != 0) {
                i += pct;
                continue;
            }
            if (static_cast<unsigned char>(rest[i]) < 0x80U) {
                if (!template_literal_chars(rest[i])) {
                    break;
                }
                ++i;
                continue;
            }
            std::size_t next = i;
            char32_t cp = 0;
            if (!decode_utf8(rest, next, cp) || !(is_ucschar(cp) || is_iprivate(cp))) {
                break;
            }
            i = next;
        }
        if (i == 0) {
            return urlg::error::mismatch;
        }
        char const* const first = it;
        it += i;
        return grammar::text(first, i);
    }
};

// RFC 6570 2.2 and 2.3's expression, written with Boost.URL's combinators, one rule per production;
// a nested rule reaches a range_rule through ref, so the range keeps it inline.

/** varchar's run: letters, digits, "_" and pct-encoded triplets, at least one. */
inline constexpr auto varchars_rule = urlg::not_empty_rule(boost::urls::pct_encoded_rule(varchar_chars));

/** A dot, then a run of varchars: a varname's dot is always followed by a varchar. */
inline constexpr auto dotted_varchars_rule =
    urlg::tuple_rule(urlg::squelch(urlg::delim_rule('.')), varchars_rule);

/** varname = varchar *( ["."] varchar ). */
inline constexpr auto varname_rule =
    urlg::range_rule(urlg::ref(varchars_rule), urlg::ref(dotted_varchars_rule), 1);

/** One decimal digit. */
inline constexpr auto digit_rule = urlg::delim_rule(urlg::digit_chars);

/** max-length = %x31-39 0*3DIGIT: 1 to 9999, no leading zero. */
inline constexpr auto max_length_rule = urlg::tuple_rule(
    urlg::delim_rule(urlg::lut_chars("123456789")), urlg::range_rule(urlg::ref(digit_rule), 0, 3));

/** modifier-level4 = prefix / explode, where prefix = ":" max-length and explode = "*". */
inline constexpr auto modifier_rule = urlg::variant_rule(
    urlg::tuple_rule(urlg::squelch(urlg::delim_rule(':')), max_length_rule), urlg::delim_rule('*'));

/** varspec = varname [ modifier-level4 ]. */
inline constexpr auto varspec_rule = urlg::tuple_rule(varname_rule, urlg::optional_rule(modifier_rule));

/** A comma, then a varspec: the rest of a variable-list. */
inline constexpr auto next_varspec_rule =
    urlg::tuple_rule(urlg::squelch(urlg::delim_rule(',')), varspec_rule);

/** expression = "{" [ operator ] variable-list "}", variable-list = varspec *( "," varspec ). */
inline constexpr auto expression_shape = urlg::tuple_rule(
    urlg::squelch(urlg::delim_rule('{')),
    urlg::optional_rule(urlg::delim_rule(template_operator_chars)),
    urlg::range_rule(urlg::ref(varspec_rule), urlg::ref(next_varspec_rule), 1),
    urlg::squelch(urlg::delim_rule('}')));

/** RFC 6570's expression as a terminal of the template: mismatch when no "{" is there, invalid when one starts and breaks. */
struct template_expression_rule_t {
    /** The rule yields the expression it matched, braces included. */
    using value_type = grammar::text;

    /** Matches an expression at `it`, or leaves `it`. */
    grammar::result<grammar::text> parse(char const*& it, char const* end) const noexcept {
        if (it == end || *it != '{') {
            return urlg::error::mismatch;
        }
        char const* const first = it;
        if (const auto expression = urlg::parse(it, end, expression_shape); !expression) {
            it = first;
            return urlg::error::invalid;
        }
        return grammar::text(first, static_cast<std::size_t>(it - first));
    }
};

/** A part of a template: literals or an expression. */
inline constexpr auto template_part_rule =
    urlg::variant_rule(template_literals_rule_t{}, template_expression_rule_t{});

/** RFC 6570 2's URI-Template, *( literals / expression ), its part passed by reference so the range keeps it inline. */
inline constexpr auto template_rule = urlg::range_rule(urlg::ref(template_part_rule));

}  // namespace detail

namespace grammar {

/**
 The rule of RFC 6570 section 2's URI-Template, with erratum 6937: literals and expressions.

 A literal is a printable ASCII character but the space, the double quote, the percent sign,
 the less-than and greater-than signs, the backslash, the circumflex, the grave accent, the braces
 and the vertical bar (the apostrophe is one, by the erratum), or a pct-encoded triplet, or a
 `ucschar` or `iprivate` code point. An expression is `{`, an optional operator (`op-reserve`'s included, as the ABNF admits them), and
 varspecs separated by commas, each a varname with `:` and a max-length of 1 to 9999, or `*`. The
 empty template is valid. The rule stops where the template does; an expression or a `%` that
 starts and breaks makes it fail.

 @see "URI templates", in the guide.
*/
struct uri_template_rule_t {
    /** The rule yields the text it matched. */
    using value_type = text;

    /**
     Matches a URI-Template at `it`.

     @param it The start of the input: past the match on success, else unchanged.
     @param end The end of the input.
     @return The text matched, possibly empty; `error::invalid` when an expression or a
     pct-encoded triplet starts and breaks.
    */
    [[nodiscard]] result<text> parse(char const*& it, char const* end) const noexcept;
};

/** The rule of RFC 6570's URI-Template. */
inline constexpr uri_template_rule_t uri_template_rule{};

inline result<text> uri_template_rule_t::parse(char const*& it, char const* end) const noexcept {
    char const* const first = it;
    if (const auto parts = detail::urlg::parse(it, end, detail::template_rule); !parts) {
        it = first;
        return parts.error();
    }
    if (it != end && (*it == '{' || *it == '%')) {
        it = first;
        return detail::urlg::error::invalid;
    }
    return text(first, static_cast<std::size_t>(it - first));
}

}  // namespace grammar

/**
 Whether `s` is an RFC 6570 URI Template of any level, JSON Schema's `uri-template`.

 @param s Any byte string.
 @return `parse(s, grammar::uri_template_rule).has_value()`.
 @see "URI templates", in the guide.
*/
[[nodiscard]] inline bool is_uri_template(std::string_view s) noexcept {
    return detail::urlg::parse(s, grammar::uri_template_rule).has_value();
}

}  // namespace webcpp::formats

#endif  // WEBCPP_FORMATS_URI_TEMPLATE_HPP
```

`include/webcpp/formats.hpp` gains `uri_template.hpp`, sorted.

- [ ] **Step 6: Run them and watch them pass.**
  `B2 --build-dir=bin/formats-iris-native libs/formats/test//iri libs/formats/test//iri_resource libs/formats/test//uri_template`:
  PASS. Then see two guards fail, each alone, restored at once: make
  `is_bidi_format` return `false` (expected FAIL, `iri: "http://a/\xE2\x80\x8E"`
  named, the U+200E probe accepted), then make `in_query` always `true`
  (expected FAIL, `iri: "http://a/#\xEE\x80\x80"` and
  `iri-reference: "#\xEE\x80\x80"` named, iprivate accepted in a
  fragment). Then the five lanes with `<t>` = `iris`: every lane green, `iri`,
  `iri_resource`, `uri_template` and `allocation` passed on all four
  targets (`iri_resource` on wasip2 and wasip3 runs wasm exceptions under
  wasmtime 47).

- [ ] **Step 7: The page.** `doc/uris.adoc` gains, after its paragraph on the
  guard:

```
`is_iri` and `is_iri_reference` check RFC 3987's IRI and IRI reference,
which allow non-ASCII code points, by mapping the IRI to a URI as RFC 3987
3.1 says and handing it to Boost.URL. Each non-ASCII code point must be
well-formed UTF-8 and in `ucschar`, or in `iprivate` within the query; the
bidirectional formatting characters U+200E, U+200F and U+202A to U+202E are
refused anywhere, as RFC 3987 4.1 requires. Each octet of a non-ASCII code
point then becomes `%XX`, which RFC 3986 allows exactly where RFC 3987 allows
`ucschar`, so a non-ASCII code point in a scheme, a port or an IP literal is
refused by Boost.URL. No IDNA is involved: an IRI's host is a registered
name of any `ucschar`.

The mapped URI needs contiguous storage. An ASCII IRI is parsed as it is,
never copied; a non-ASCII one is mapped into a buffer of 1,024 octets on the
stack, which holds any IRI of up to 341 octets; a longer one takes one buffer
of its mapped length from the memory resource given to
`is_iri(s, resource)`, or from `std::pmr::get_default_resource()`.
```

  `doc/templates.adoc`, after `uris.adoc`'s place:

```
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

[#formats-templates]
== URI templates

`is_uri_template` checks RFC 6570's URI Template of any level, without
expanding it: literals, and expressions in braces with an optional operator
and comma-separated variables, each with a prefix length of 1 to 9999 or an
explode `*`. Erratum 6937, verified in 2022, makes the apostrophe a literal,
as JSON Schema's suite expects. The operators RFC 6570 reserves for future
use, `= , ! @ |`, are accepted, as its ABNF accepts them. A literal may be a
`ucschar` or `iprivate` code point, so a template of an IRI is valid too.
```

- [ ] **Step 8: AGENTS.md.** It gains:

```markdown
- **Why IRIs are mapped.** Boost.URL parses RFC 3986 only. formats checks
  what RFC 3987 adds, the non-ASCII code points and where they may be, then
  maps each of their octets to `%XX` and lets Boost.URL judge the structure:
  a `%XX` is valid exactly where RFC 3987 allows `ucschar`. The mapped copy
  is the one allocation of formats' validators, and only beyond 1,024 mapped
  octets; an ASCII IRI is never copied.
```

- [ ] **Step 9: Register with the suite.** `validators` gains, sorted:

```cpp
    mapping{"iri", [](std::string_view s) { return formats::is_iri(s); }},
    mapping{"iri-reference", [](std::string_view s) { return formats::is_iri_reference(s); }},
    mapping{"uri-template", [](std::string_view s) { return formats::is_uri_template(s); }},
```

  `pending` loses `"iri"`, `"iri-reference"`, `"uri-template"`. The native and emscripten lanes: `suite` passes, and each
  dialect's `string tests` grows by, and `pending` falls by, draft4 0,
  draft6 41, draft7 76, draft2019-09 76, draft2020-12 76, v1 76.

- [ ] **Step 10: Build the page, run the lint.** The doc build with
  `<t>` = `iris` exits 0; `LINT`: `lint: clean`.

- [ ] **Step 11: Commit** in `~/dev/webcpp-formats-iris/libs/formats`:

```bash
git add include/webcpp/formats/detail/iri_mapping.hpp include/webcpp/formats/uri.hpp \
  include/webcpp/formats/uri_template.hpp include/webcpp/formats.hpp test/iri_test.cpp \
  test/iri_resource_test.cpp test/uri_template_test.cpp test/Jamfile test/allocation_test.cpp \
  test/suite_test.cpp doc/uris.adoc doc/templates.adoc doc/formats.adoc AGENTS.md
git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "formats: IRIs mapped to URIs for Boost.URL, and URI templates of RFC 6570 with erratum 6937"
```

The report's Changelog: the "New Libraries" line, unchanged.

### Task 8: Host names and e-mail addresses (F5) ∥

Worktree slug `hosts`, `<base>` = `B-S4` or later (it carries S1, which
idna's test Jamfile needs), plus a worktree of idna at the commit that
carries I4 (`check_label`). Needs Tasks 4 and 5
integrated and idna I4. ∥ Tasks 6 and 7.

**Files:**
- Create: `include/webcpp/formats/hostname.hpp`, `include/webcpp/formats/email.hpp`, `test/hostname_test.cpp`, `test/email_test.cpp`
- Modify: `build.jam`, `include/webcpp/formats.hpp`, `test/Jamfile`, `test/allocation_test.cpp`, `test/suite_test.cpp`, `doc/addresses.adoc`

**Interfaces:**
- Consumes: idna I4's `webcpp::idna::check_label(std::string_view, const idna::options& = {}) noexcept -> idna::result<void>`, `idna::max_label_length` (63), `idna::max_domain_length` (253), from `<webcpp/idna/validity.hpp>` and `<webcpp/idna/options.hpp>`; Boost.URL's `ipv4_address_rule`, `ipv6_address_rule`; Task 1's `ldh_chars`, `hostname_chars`, `atext_chars`, `qtext_smtp_chars`, `quoted_pair_chars`, `ci_literal_rule`, `decode_utf8`.
- Produces:

```cpp
namespace webcpp::formats {
[[nodiscard]] bool is_hostname(std::string_view s) noexcept;
[[nodiscard]] bool is_email(std::string_view s) noexcept;
}
namespace webcpp::formats::grammar {
struct hostname_rule_t;   // value_type text
struct email_rule_t;      // value_type text
inline constexpr hostname_rule_t hostname_rule{};
inline constexpr email_rule_t email_rule{};
}
namespace webcpp::formats::detail {
struct local_part_rule_t { using value_type = grammar::text; bool utf8 = false; /* parse */ };
inline constexpr local_part_rule_t local_part_rule{};
inline constexpr auto address_literal_rule = /* "[" (IPv4 / "IPv6:" IPv6) "]", Boost.URL's combinators */;
// ldh_label_rule, dotted_label_rule, labels_rule: token_rule(ldh_chars) inside a range_rule
template <class Host>
grammar::result<grammar::text> parse_mailbox(char const*& it, char const* end,
                                             const local_part_rule_t& local, const Host& host) noexcept;
}
```

- [ ] **Step 1: Write the failing tests.** `test/hostname_test.cpp` (Task 9
  adds its IDN half):

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Host names: RFC 1123 2.1's labels and lengths at 63/64 and 253/254, A-labels in either case
// judged as idn-hostname judges them, R-LDH labels, and the A-labels of strings IDNA2008 refuses
// that an IDNA library was measured accepting.

#include <webcpp/formats/hostname.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/url/grammar/error.hpp>
#include <boost/url/grammar/parse.hpp>

#include <iostream>
#include <string>
#include <string_view>

namespace formats = webcpp::formats;
namespace fg = webcpp::formats::grammar;
namespace urlg = boost::urls::grammar;

namespace {

void hostname(std::string_view text, bool valid) {
    if (!BOOST_TEST_EQ(formats::is_hostname(text), valid)) {
        std::cerr << "hostname: \"" << text << "\"\n";
    }
    BOOST_TEST_EQ(formats::is_hostname(text), urlg::parse(text, fg::hostname_rule).has_value());
}

void labels_and_names_at_their_limits() {
    const std::string label63(63, 'a');
    hostname(label63 + ".com", true);
    hostname(std::string(64, 'a') + ".com", false);
    // 253 octets: three labels of 63 and one of 61, joined by three dots.
    const std::string name253 = label63 + "." + label63 + "." + label63 + "." + std::string(61, 'a');
    BOOST_TEST_EQ(name253.size(), 253U);
    hostname(name253, true);
    hostname(name253 + "a", false);
}

void rfc_1123_labels() {
    for (const std::string_view text :
         {"www.example.com", "hostname", "h0stn4me", "1host", "host-name", "a--b.com",
          "ab--cd.example", "test.123"}) {
        hostname(text, true);
    }
    for (const std::string_view text :
         {"", ".", ".example", "example.", "a..b", "-hostname", "hostname-", "host_name",
          "example.com\n", "example.com ", "b\xC3\xBC" "cher.example", "example\xEF\xBC\x8E" "com"}) {
        hostname(text, false);
    }
    hostname(std::string_view("example.com\0", 12), false);
}

void a_labels_in_either_case() {
    hostname("xn--nxasmq6b", true);
    hostname("XN--NXASMQ6B", true);
    hostname("Xn--NxAsMq6B.example", true);
    hostname("xn--9n2bp8q.xn--9t4b11yi5a", true);
    hostname("xn--X", false);
    hostname("XN--aa---o47jg78q", false);
    hostname("xn--hello-txk", false);
    // A host name's labels are judged one by one: RFC 5893's Bidi rule across the labels
    // of a domain is idn-hostname's, so this is a host name and its U-label form, 0a.א, is no
    // idn-hostname.
    hostname("0a.xn--4db", true);
}

void a_labels_idna2008_refuses() {
    for (const std::string_view text :
         {"xn--07jt112bpxg.xn--9t4b11yi5a", "xn--chb89f", "xn--07jceefgh4c", "xn--al-0ea",
          "xn--l-fda", "xn--la-0ea", "xn--l-gda", "xn--S-jib3p", "xn--wva3j", "xn--5db1e",
          "xn--5db3e", "xn--defabc-k64e", "xn--vek", "xn--7a"}) {
        hostname(text, false);
    }
}

void the_rule_stops_and_reports() {
    const std::string_view text = "example.com:8080";
    char const* it = text.data();
    const auto r = fg::hostname_rule.parse(it, text.data() + text.size());
    BOOST_TEST(r.has_value() && *r == "example.com");
    const std::string_view refused = "xn--7a.example";
    char const* at = refused.data();
    const auto e = fg::hostname_rule.parse(at, refused.data() + refused.size());
    // The A-label's refusal is idna's own error.
    BOOST_TEST(e.has_error() && e.error().category() == webcpp::idna::error_category());
    BOOST_TEST(at == refused.data());
    const std::string_view none = ":80";
    char const* from = none.data();
    const auto m = fg::hostname_rule.parse(from, none.data() + none.size());
    BOOST_TEST(m.has_error() && m.error() == urlg::error::mismatch);
}

}  // namespace

int main() {
    labels_and_names_at_their_limits();
    rfc_1123_labels();
    a_labels_in_either_case();
    a_labels_idna2008_refuses();
    the_rule_stops_and_reports();
    return boost::report_errors();
}
```

`test/email_test.cpp` (Task 9 adds its IDN half):

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// E-mail addresses, RFC 5321 4.1.2's Mailbox: each production and its refusal, the local part at
// 64 and 65 octets, address literals in either case of their tag, a General-address-literal
// refused, and the rule inside a larger grammar.

#include <webcpp/formats/email.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/url/grammar/error.hpp>
#include <boost/url/grammar/parse.hpp>

#include <iostream>
#include <string>
#include <string_view>

namespace formats = webcpp::formats;
namespace fg = webcpp::formats::grammar;
namespace urlg = boost::urls::grammar;

namespace {

void email(std::string_view text, bool valid) {
    if (!BOOST_TEST_EQ(formats::is_email(text), valid)) {
        std::cerr << "email: \"" << text << "\"\n";
    }
    BOOST_TEST_EQ(formats::is_email(text), urlg::parse(text, fg::email_rule).has_value());
}

void dot_strings() {
    email("joe.bloggs@example.com", true);
    email("te~st@example.com", true);
    email("!#$%&'*+-/=?^_`{|}~@example.com", true);
    email("te.s.t@example.com", true);
    email(".test@example.com", false);
    email("test.@example.com", false);
    email("te..st@example.com", false);
    email("a,b@iana.org", false);
    email("a\tb@iana.org", false);
    email("\x7F@iana.org", false);
    email("a\xC3\xA9@iana.org", false);
    email(std::string_view("a\0@iana.org", 11), false);
}

void quoted_strings() {
    email("\"joe bloggs\"@example.com", true);
    email("\"joe..bloggs\"@example.com", true);
    email("\"joe@bloggs\"@example.com", true);
    email("\"\"@iana.org", true);
    email("\"\\a\"@iana.org", true);
    email("\"\\\"\"@iana.org", true);
    email("\"\\\\\"@iana.org", true);
    email("\"test\\\xC2\xA9\"@iana.org", false);
    email("\"test@iana.org", false);
    email("test\"@iana.org", false);
    email("\"test\"test@iana.org", false);
    email("\"a\tb\"@iana.org", false);
}

void local_parts_at_64_and_65_octets() {
    email(std::string(64, 'a') + "@example.com", true);
    email(std::string(65, 'a') + "@example.com", false);
    email("\"" + std::string(62, 'a') + "\"@example.com", true);
    email("\"" + std::string(63, 'a') + "\"@example.com", false);
}

void domains_and_address_literals() {
    for (const std::string_view text :
         {"joe.bloggs@[127.0.0.1]", "joe.bloggs@[IPv6:::1]", "a@[ipv6:::1]",
          "a@[IPV6:2001:db8::1]", "test@255.255.255.255", "test@io", "test@c--n.com",
          "test@xn--nxasmq6b.example"}) {
        email(text, true);
    }
    for (const std::string_view text :
         {"joe.bloggs@[127.0.0.300]", "test@[RFC-5322-domain-literal]", "a@[]", "a@[1.2.3]",
          "a@[1.2.3.4.5]", "test@[1.2.3.4", "test@a[255.255.255.255]", "test@-iana.org",
          "test@iana.org.", "test@iana..com", "a@b@c.org", "a@\xC3\xA9.org", "joe.bloggs@",
          "@example.com", "test@xn--7a.example", "a@b(c).org", "test@iana.org\n"}) {
        email(text, false);
    }
}

void the_rule_stops_and_reports() {
    const std::string_view text = "a@b.example>rest";
    char const* it = text.data();
    const auto r = fg::email_rule.parse(it, text.data() + text.size());
    BOOST_TEST(r.has_value() && *r == "a@b.example");
    const std::string_view broken = "a@[1.2.3";
    char const* at = broken.data();
    const auto b = fg::email_rule.parse(at, broken.data() + broken.size());
    BOOST_TEST(b.has_error() && b.error() == urlg::error::invalid);
    BOOST_TEST(at == broken.data());
    const std::string_view none = "@example.com";
    char const* from = none.data();
    const auto m = fg::email_rule.parse(from, none.data() + none.size());
    BOOST_TEST(m.has_error() && m.error() == urlg::error::mismatch);
    // A domain's A-label that idna refuses: idna's own error reaches the caller.
    const auto a_label = urlg::parse("test@xn--7a.example", fg::email_rule);
    BOOST_TEST(a_label.has_error() &&
               a_label.error().category() == webcpp::idna::error_category());
}

}  // namespace

int main() {
    dot_strings();
    quoted_strings();
    local_parts_at_64_and_65_octets();
    domains_and_address_literals();
    the_rule_stops_and_reports();
    return boost::report_errors();
}
```

`test/Jamfile`, after the URI template line's place:

```
webcpp.run hostname : hostname_test.cpp ;
webcpp.run email : email_test.cpp ;
```

`test/allocation_test.cpp` gains, with the two includes and its call:

```cpp
/** Host names and e-mail addresses allocate nothing: a 100,000-octet name, and A-labels idna checks. */
void hosts_allocate_nothing() {
    std::string long_name;
    while (long_name.size() < 100'000) {
        long_name += std::string(63, 'a') + ".";
    }
    long_name += "com";
    const std::string long_email = "a@" + long_name;
    const std::size_t before = global_allocations();
    BOOST_TEST(!webcpp::formats::is_hostname(long_name));
    BOOST_TEST(webcpp::formats::is_hostname("xn--9n2bp8q.xn--9t4b11yi5a"));
    BOOST_TEST(!webcpp::formats::is_hostname("xn--07jceefgh4c"));
    BOOST_TEST(webcpp::formats::is_email("\"joe bloggs\"@[IPv6:2001:db8::1]"));
    BOOST_TEST(!webcpp::formats::is_email(long_email));
    BOOST_TEST_EQ(global_allocations() - before, 0U);
}
```

- [ ] **Step 2: Run them and watch them fail.**
  `B2 --build-dir=bin/formats-hosts-native libs/formats/test//hostname libs/formats/test//email`.
  Expected: FAIL, `'webcpp/formats/hostname.hpp' file not found` and
  `'webcpp/formats/email.hpp' file not found`.

- [ ] **Step 3: Write the headers, and bring idna.** `build.jam`'s alias gains
  `<library>/webcpp/idna//idna` after Boost's headers, and its comment
  "webcpp::idna, for internationalized names,".

`include/webcpp/formats/hostname.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 Host names, RFC 1123's with their A-labels judged by webcpp::idna, as a validator and a Boost.URL
 rule.

 @see "Addresses", in the guide.
*/
#ifndef WEBCPP_FORMATS_HOSTNAME_HPP
#define WEBCPP_FORMATS_HOSTNAME_HPP

#include <webcpp/formats/config.hpp>

#include <webcpp/formats/detail/charsets.hpp>
#include <webcpp/formats/grammar.hpp>

#include <webcpp/idna/options.hpp>
#include <webcpp/idna/validity.hpp>

#include <boost/system/error_code.hpp>
#include <boost/url/grammar/ci_string.hpp>
#include <boost/url/grammar/delim_rule.hpp>
#include <boost/url/grammar/error.hpp>
#include <boost/url/grammar/parse.hpp>
#include <boost/url/grammar/range_rule.hpp>
#include <boost/url/grammar/token_rule.hpp>
#include <boost/url/grammar/tuple_rule.hpp>

#include <array>
#include <cstddef>
#include <string_view>

namespace webcpp::formats {

namespace detail {

namespace urlg = boost::urls::grammar;

/** Whether `label` starts with "xn--" in any case: an A-label (RFC 5890 2.3.2.1). */
constexpr bool has_ace_prefix(std::string_view label) noexcept {
    return label.size() >= 4 && (label[0] == 'x' || label[0] == 'X') &&
           (label[1] == 'n' || label[1] == 'N') && label[2] == '-' && label[3] == '-';
}

/** A label's characters, RFC 1123 2.1's letters, digits and hyphens, at least one. */
inline constexpr auto ldh_label_rule = urlg::token_rule(ldh_chars);

/** A dot, then a label: a name's dot is always between two labels. */
inline constexpr auto dotted_label_rule =
    urlg::tuple_rule(urlg::squelch(urlg::delim_rule('.')), ldh_label_rule);

/** A host name's labels, label *( "." label ), their nested rules by reference so the range keeps them inline. */
inline constexpr auto labels_rule =
    urlg::range_rule(urlg::ref(ldh_label_rule), urlg::ref(dotted_label_rule), 1);

/** Whether `label`, made of letters, digits and hyphens, has RFC 1123 2.1's length and ends: 1 to 63 octets, no hyphen first or last. */
constexpr bool is_ldh_label(std::string_view label) noexcept {
    return !label.empty() && label.size() <= idna::max_label_length && label.front() != '-' &&
           label.back() != '-';
}

/** Why idn-hostname refuses `label`, an LDH A-label: idna's error for it lowercased, as UTS 46 maps it, or none. */
inline boost::system::error_code check_a_label(std::string_view label) noexcept {
    std::array<char, idna::max_label_length> lower{};
    for (std::size_t i = 0; i < label.size(); ++i) {
        lower[i] = urlg::to_lower(label[i]);
    }
    if (const auto checked = idna::check_label(std::string_view(lower.data(), label.size()));
        !checked) {
        return checked.error();
    }
    return {};
}

/** Why `name`, a run of letters, digits, hyphens and dots, is no RFC 1123 host name, or none: its length, an empty label, a label's length or ends, or an A-label idna refuses. */
inline boost::system::error_code check_hostname(std::string_view name) noexcept {
    if (name.size() > idna::max_domain_length) {
        return urlg::error::invalid;
    }
    // The whole run as labels: a leading, trailing or double dot leaves input over (leftover).
    const auto labels = urlg::parse(name, labels_rule);
    if (!labels) {
        return urlg::error::invalid;
    }
    for (const grammar::text label : *labels) {
        if (!is_ldh_label(label)) {
            return urlg::error::invalid;
        }
        if (has_ace_prefix(label)) {
            if (const boost::system::error_code refused = check_a_label(label)) {
                return refused;
            }
        }
    }
    return {};
}

}  // namespace detail

namespace grammar {

/**
 The rule of a host name: RFC 1123 2.1's labels, with each A-label judged as `idn-hostname`
 judges it.

 ASCII only. Labels of 1 to 63 letters, digits and hyphens, no hyphen first or last, joined by
 single dots, no dot first or last; the name at most 253 octets. A label that starts with `xn--`
 in any case is an A-label: lowercased and checked by `webcpp::idna::check_label` (RFC 5891 5.4:
 decoded, IDNA2008, the Bidi rule of the label alone, canonical). Another label with `--` in its
 third and fourth places is valid, as RFC 1123 says. It takes the run of letters, digits, `-` and
 `.` at its start and validates it whole, never a shorter prefix.

 @see "Addresses", in the guide.
*/
struct hostname_rule_t {
    /** The rule yields the text it matched. */
    using value_type = text;

    /**
     Matches a host name at `it`.

     @param it The start of the input: past the match on success, else unchanged.
     @param end The end of the input.
     @return The text matched; `error::mismatch` when no letter, digit, `-` or `.` is at `it`;
     `error::invalid` when the run is no host name; idna's error when idna refuses an A-label.
    */
    [[nodiscard]] result<text> parse(char const*& it, char const* end) const noexcept;
};

/** The rule of a host name. */
inline constexpr hostname_rule_t hostname_rule{};

inline result<text> hostname_rule_t::parse(char const*& it, char const* end) const noexcept {
    char const* last = it;
    while (last != end && detail::hostname_chars(*last)) {
        ++last;
    }
    if (last == it) {
        return detail::urlg::error::mismatch;
    }
    const std::string_view name(it, static_cast<std::size_t>(last - it));
    if (const boost::system::error_code refused = detail::check_hostname(name)) {
        return refused;
    }
    char const* const first = it;
    it = last;
    return text(first, name.size());
}

}  // namespace grammar

/**
 Whether `s` is a host name, JSON Schema's `hostname`.

 @param s Any byte string.
 @return `parse(s, grammar::hostname_rule).has_value()`.
 @see "Addresses", in the guide.
*/
[[nodiscard]] inline bool is_hostname(std::string_view s) noexcept {
    return detail::urlg::parse(s, grammar::hostname_rule).has_value();
}

}  // namespace webcpp::formats

#endif  // WEBCPP_FORMATS_HOSTNAME_HPP
```

`include/webcpp/formats/email.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 E-mail addresses, RFC 5321's Mailbox, as a validator and a Boost.URL rule composed of Boost.URL's
 combinators over formats' terminals and Boost.URL's IP rules.

 @see "Addresses", in the guide.
*/
#ifndef WEBCPP_FORMATS_EMAIL_HPP
#define WEBCPP_FORMATS_EMAIL_HPP

#include <webcpp/formats/config.hpp>

#include <webcpp/formats/detail/charsets.hpp>
#include <webcpp/formats/detail/rules.hpp>
#include <webcpp/formats/detail/utf8.hpp>
#include <webcpp/formats/grammar.hpp>
#include <webcpp/formats/hostname.hpp>

#include <boost/system/error_code.hpp>
#include <boost/url/grammar/delim_rule.hpp>
#include <boost/url/grammar/error.hpp>
#include <boost/url/grammar/parse.hpp>
#include <boost/url/grammar/tuple_rule.hpp>
#include <boost/url/grammar/variant_rule.hpp>
#include <boost/url/rfc/ipv4_address_rule.hpp>
#include <boost/url/rfc/ipv6_address_rule.hpp>

#include <cstddef>
#include <optional>
#include <string_view>

namespace webcpp::formats {

namespace detail {

namespace urlg = boost::urls::grammar;

/** The longest Local-part RFC 5321 4.5.3.1.1 allows, in octets, a Quoted-string's quotes included. */
inline constexpr std::size_t max_local_part_length = 64;

/** The length of the Atom at the start of `s`: 1*atext, a UTF8-non-ascii sequence counting as atext when `utf8`. */
constexpr std::size_t atom_length(std::string_view s, bool utf8) noexcept {
    std::size_t i = 0;
    while (i < s.size()) {
        if (static_cast<unsigned char>(s[i]) < 0x80U) {
            if (!atext_chars(s[i])) {
                break;
            }
            ++i;
            continue;
        }
        std::size_t next = i;
        char32_t cp = 0;
        if (!utf8 || !decode_utf8(s, next, cp)) {
            break;
        }
        i = next;
    }
    return i;
}

/** The length of the Dot-string at the start of `s`, Atom *("." Atom); a dot no Atom follows is not its. */
constexpr std::size_t dot_string_length(std::string_view s, bool utf8) noexcept {
    std::size_t length = atom_length(s, utf8);
    if (length == 0) {
        return 0;
    }
    while (length < s.size() && s[length] == '.') {
        const std::size_t atom = atom_length(s.substr(length + 1), utf8);
        if (atom == 0) {
            break;
        }
        length += 1 + atom;
    }
    return length;
}

/** The length of the Quoted-string at the start of `s`, which is a double quote, quotes included; nothing when it breaks or never closes. */
constexpr std::optional<std::size_t> quoted_string_length(std::string_view s, bool utf8) noexcept {
    std::size_t i = 1;
    while (i < s.size()) {
        const char c = s[i];
        if (c == '"') {
            return i + 1;
        }
        if (c == '\\') {
            if (i + 1 >= s.size() || !quoted_pair_chars(s[i + 1])) {
                return std::nullopt;
            }
            i += 2;
            continue;
        }
        if (static_cast<unsigned char>(c) < 0x80U) {
            if (!qtext_smtp_chars(c)) {
                return std::nullopt;
            }
            ++i;
            continue;
        }
        char32_t cp = 0;
        if (!utf8 || !decode_utf8(s, i, cp)) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

/** RFC 5321 4.1.2's Local-part, or RFC 6531 3.3's when `utf8`: a Dot-string or a Quoted-string of at most 64 octets. */
struct local_part_rule_t {
    /** The rule yields the local part it matched. */
    using value_type = grammar::text;

    /** Whether UTF8-non-ascii is atext and qtext too (RFC 6531 3.3). */
    bool utf8 = false;

    /** Matches a local part at `it`: mismatch when none starts there, invalid when one breaks or is too long. */
    grammar::result<grammar::text> parse(char const*& it, char const* end) const noexcept {
        const std::string_view rest(it, static_cast<std::size_t>(end - it));
        std::size_t length = 0;
        if (!rest.empty() && rest.front() == '"') {
            const std::optional<std::size_t> quoted = quoted_string_length(rest, utf8);
            if (!quoted) {
                return urlg::error::invalid;
            }
            length = *quoted;
        } else {
            length = dot_string_length(rest, utf8);
            if (length == 0) {
                return urlg::error::mismatch;
            }
        }
        if (length > max_local_part_length) {
            return urlg::error::invalid;
        }
        char const* const first = it;
        it += length;
        return grammar::text(first, length);
    }
};

/** RFC 5321's Local-part. */
inline constexpr local_part_rule_t local_part_rule{};

/** RFC 5321 4.1.3's address-literal: "[" IPv4-address-literal "]" or "[IPv6:" IPv6-addr "]", the tag in any case. */
inline constexpr auto address_literal_rule = urlg::tuple_rule(
    urlg::squelch(urlg::delim_rule('[')),
    urlg::variant_rule(boost::urls::ipv4_address_rule,
                       urlg::tuple_rule(urlg::squelch(ci_literal_rule("IPv6:")),
                                        boost::urls::ipv6_address_rule)),
    urlg::squelch(urlg::delim_rule(']')));

/**
 Matches a Mailbox at `it`, `local` its local part and `host` its domain: the local part, the at
 sign, then an address literal where "[" follows, else `host`, whose error is returned as it is.

 Tip: the domain is dispatched here rather than made an alternative of a variant_rule, which
 answers mismatch when every alternative fails and would lose idna's reason.
*/
template <class Host>
grammar::result<grammar::text> parse_mailbox(char const*& it, char const* end,
                                             const local_part_rule_t& local,
                                             const Host& host) noexcept {
    char const* const first = it;
    if (const auto part = local.parse(it, end); !part) {
        return part.error();
    }
    if (it == end || *it != '@') {
        it = first;
        return urlg::error::invalid;
    }
    ++it;
    if (it != end && *it == '[') {
        if (const auto literal = urlg::parse(it, end, address_literal_rule); !literal) {
            it = first;
            return urlg::error::invalid;
        }
        return grammar::text(first, static_cast<std::size_t>(it - first));
    }
    if (const auto domain = host.parse(it, end); !domain) {
        it = first;
        if (domain.error() == urlg::error::mismatch) {
            return urlg::error::invalid;
        }
        return domain.error();
    }
    return grammar::text(first, static_cast<std::size_t>(it - first));
}

}  // namespace detail

namespace grammar {

/**
 The rule of RFC 5321 4.1.2's Mailbox: a local part, the at sign, and a domain or an address literal.

 The local part is a Dot-string of atoms (RFC 5322's `atext`) or a Quoted-string, at most 64
 octets. The domain is @ref hostname_rule, A-labels included, whose error, idna's for an A-label,
 the rule returns as it is; an address literal is `[` and an IPv4 address `]`, or `[IPv6:` and an
 IPv6 address `]`, the tag in any case: RFC 5321's General-address-literal needs a tag registered
 with IANA, and `IPv6` is the only one. It stops where the domain or the literal does.

 @see "Addresses", in the guide.
*/
struct email_rule_t {
    /** The rule yields the text it matched. */
    using value_type = text;

    /**
     Matches a Mailbox at `it`.

     @param it The start of the input: past the match on success, else unchanged.
     @param end The end of the input.
     @return The text matched; `error::mismatch` when no local part starts at `it`;
     `error::invalid` when the rest breaks the grammar; idna's error when idna refuses an A-label of
     the domain.
    */
    [[nodiscard]] result<text> parse(char const*& it, char const* end) const noexcept;
};

/** The rule of RFC 5321's Mailbox. */
inline constexpr email_rule_t email_rule{};

inline result<text> email_rule_t::parse(char const*& it, char const* end) const noexcept {
    return detail::parse_mailbox(it, end, detail::local_part_rule, hostname_rule);
}

}  // namespace grammar

/**
 Whether `s` is an RFC 5321 Mailbox, JSON Schema's `email`.

 @param s Any byte string.
 @return `parse(s, grammar::email_rule).has_value()`.
 @see "Addresses", in the guide.
*/
[[nodiscard]] inline bool is_email(std::string_view s) noexcept {
    return detail::urlg::parse(s, grammar::email_rule).has_value();
}

}  // namespace webcpp::formats

#endif  // WEBCPP_FORMATS_EMAIL_HPP
```

`include/webcpp/formats.hpp` gains `email.hpp` and `hostname.hpp`, sorted.

- [ ] **Step 4: Run them and watch them pass.**
  `B2 --build-dir=bin/formats-hosts-native libs/formats/test//hostname libs/formats/test//email`:
  PASS. A failure that points to idna (an A-label whose verdict is idna's)
  becomes a `validity_test` case in idna, red first (idna's spec section
  9), reported to the idna track; formats does not work around it. Then the
  five lanes with `<t>` = `hosts`: every lane green.

- [ ] **Step 5: The page.** `doc/addresses.adoc` gains:

```
[#formats-hostnames]
=== Host names

`is_hostname` checks RFC 1123 2.1's host names: labels of 1 to 63 letters,
digits and hyphens, no hyphen first or last, joined by single dots, the name
at most 253 octets, no empty label and so no trailing dot. A label that
starts with `xn--`, in any case, is an A-label, an internationalized label
in its ASCII form: formats lowercases it, as UTS 46 maps an ASCII label, and
asks `webcpp::idna` whether IDNA2008 accepts it, decoded, canonical and with
the Bidi rule of the label alone, the verdict `is_idn_hostname` gives that
label. Another label with `--` in its third and fourth places, an R-LDH
label such as `ab--cd`, is a valid host name, as RFC 1123 says, and not an
internationalized one (RFC 5890 2.3.1).

[#formats-email]
=== E-mail addresses

`is_email` checks RFC 5321 4.1.2's Mailbox, the address of an SMTP
envelope: a local part of atoms joined by dots or a quoted string, at most
64 octets, then `@` and a host name, or an address literal, `[192.0.2.1]` or
`[IPv6:2001:db8::1]`, the tag in any case. RFC 5321's other literals need a
tag registered with IANA, and `IPv6` is the only one, so
`[RFC-5322-domain-literal]` is invalid. RFC 5322's comments and display
names are not part of a Mailbox.

Its address literal is a composition of Boost.URL's combinators,
`tuple_rule`, `variant_rule`, `delim_rule` and `squelch`, over Boost.URL's
`ipv4_address_rule` and `ipv6_address_rule`. The domain is not a
`variant_rule` alternative: the rule looks for `[` and then asks the
literal's rule or `hostname_rule`, so that the reason a host name was
refused, idna's for an A-label, reaches the caller; Boost.URL's
`variant_rule` answers `mismatch` when every alternative fails.
```

- [ ] **Step 6: Register with the suite.** `validators` gains, sorted:

```cpp
    mapping{"email", [](std::string_view s) { return formats::is_email(s); }},
    mapping{"hostname", [](std::string_view s) { return formats::is_hostname(s); }},
```

  `pending` loses `"email"` and `"hostname"`. The
  native and emscripten lanes: `suite` passes, and each dialect's
  `string tests` grows by, and `pending` falls by, draft4 90, draft6 90,
  draft7 123, draft2019-09 123, draft2020-12 123, v1 123. A failure of an
  A-label goes to idna as in Step 4.

- [ ] **Step 7: Build the page, run the lint.** The doc build with
  `<t>` = `hosts` exits 0 (idna's headers reach the reference's parse
  through `/webcpp/idna//idna`); `LINT`: `lint: clean`, the boundary
  "formats uses idna's public interface" holding.

- [ ] **Step 8: Commit** in `~/dev/webcpp-formats-hosts/libs/formats`:

```bash
git add build.jam include/webcpp/formats/hostname.hpp include/webcpp/formats/email.hpp \
  include/webcpp/formats.hpp test/hostname_test.cpp test/email_test.cpp test/Jamfile \
  test/allocation_test.cpp test/suite_test.cpp doc/addresses.adoc
git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "formats: host names with their A-labels judged by idna, and e-mail addresses of RFC 5321"
```

The report's Changelog: the "New Libraries" line, unchanged.

### Task 9: IDN host names and IDN e-mail addresses (F6) ∥

Worktree slug `idn`, `<base>` = `B-S4` or later, plus a worktree of idna at
the commit that carries I5 (`to_ascii`). Needs Task 8 integrated and idna I5. ∥ Tasks
6 and 7.

**Files:**
- Create: `include/webcpp/formats/detail/idn_scan.hpp`, `test/grammar_test.cpp`, `doc/composing.adoc`
- Modify: `include/webcpp/formats/hostname.hpp`, `include/webcpp/formats/email.hpp`, `include/webcpp/formats/grammar.hpp` (its two `@see`), `test/hostname_test.cpp`, `test/email_test.cpp`, `test/Jamfile`, `test/allocation_test.cpp`, `test/suite_test.cpp`, `doc/addresses.adoc`, `doc/formats.adoc`

**Interfaces:**
- Consumes: idna I5's `webcpp::idna::to_ascii(std::span<char> output, std::string_view domain, const idna::options& = {}) noexcept -> idna::result<std::size_t>` from `<webcpp/idna/uts46.hpp>`, `idna::max_domain_length`, `idna::error`, `idna::error_category()`; Task 8's `hostname.hpp`, `email.hpp`, `detail::local_part_rule_t`, `detail::address_literal_rule`, `detail::parse_mailbox`.
- Produces:

```cpp
namespace webcpp::formats {
[[nodiscard]] bool is_idn_hostname(std::string_view s) noexcept;
[[nodiscard]] bool is_idn_email(std::string_view s) noexcept;
}
namespace webcpp::formats::grammar {
struct idn_hostname_rule_t;   // value_type text
struct idna_label_rule_t;     // value_type text
struct idn_email_rule_t;      // value_type text
inline constexpr idn_hostname_rule_t idn_hostname_rule{};
inline constexpr idna_label_rule_t idna_label_rule{};
inline constexpr idn_email_rule_t idn_email_rule{};
}
namespace webcpp::formats::detail {
enum class idn_run { domain, label };
constexpr char const* scan_idn_run(char const* first, char const* end, idn_run kind) noexcept;
inline constexpr local_part_rule_t idn_local_part_rule{true};
// idn_email_rule_t::parse is detail::parse_mailbox(it, end, idn_local_part_rule, idn_hostname_rule)
}
```

- [ ] **Step 1: Write the failing tests.** `test/hostname_test.cpp` gains,
  with their calls in `main`:

```cpp
void idn_hostname(std::string_view text, bool valid) {
    if (!BOOST_TEST_EQ(formats::is_idn_hostname(text), valid)) {
        std::cerr << "idn-hostname: \"" << text << "\"\n";
    }
    BOOST_TEST_EQ(formats::is_idn_hostname(text),
                  urlg::parse(text, fg::idn_hostname_rule).has_value());
}

void names_idna2008_refuses() {
    // Each invalid by a rule of IDNA2008 that an IDNA library was measured missing: DISALLOWED
    // exceptions, CONTEXTO, the Bidi rule across labels, a decoded A-label's derived property.
    for (const std::string_view text :
         {"\xEC\x8B\xA4\xE3\x80\xAE\xEB\xA1\x80.\xED\x85\x8C\xEC\x8A\xA4\xED\x8A\xB8",
          "\xD9\x80\xDF\xBA",
          "\xE3\x80\xB1\xE3\x80\xB2\xE3\x80\xB3\xE3\x80\xB4\xE3\x80\xB5\xE3\x80\xAE\xE3\x80\xAF\xE3\x80\xBB",
          "a\xC2\xB7l", "\xC2\xB7l", "l\xC2\xB7" "a", "l\xC2\xB7", "\xCE\xB1\xCD\xB5S",
          "\xCE\xB1\xCD\xB5", "\xD7\xB3\xD7\x91", "\xD7\xB4\xD7\x91", "def\xE3\x83\xBB" "abc",
          "\xE3\x83\xBB", "0a.\xD7\x90", "xn--7a"}) {
        idn_hostname(text, false);
    }
}

void idn_host_names() {
    for (const std::string_view text :
         {"\xEC\x8B\xA4\xEB\xA1\x80.\xED\x85\x8C\xEC\x8A\xA4\xED\x8A\xB8", "b\xC3\xBC" "cher.example",
          "xn--NXASMQ6B", "\xEF\xBC\x91\xEF\xBC\x92\xEF\xBC\x93", "a\xE2\x80\x8B" "b",
          "l\xC2\xB7l", "\xCE\xB1\xCD\xB5\xCE\xB2", "\xD7\x90\xD7\xB3\xD7\x91",
          "\xE3\x83\xBB\xE3\x81\x81", "\xD8\xA8\xD9\xA0\xD8\xA8", "a\xE3\x80\x82" "b",
          "a\xEF\xBC\x8E" "b", "a\xEF\xBD\xA1" "b", "\xEF\xBD\x98\xEF\xBD\x8E--nxasmq6b",
          "cafe\xCC\x81.com", "hostname", "1host"}) {
        idn_hostname(text, true);
    }
    for (const std::string_view text :
         {"", ".", "example.", "example\xE3\x80\x82", "a..b", "-hello", "hello-",
          "ab--cd.example", "b\xC3" "cher.example", "\xE0\xA4\x95\xE2\x80\x8D\xE0\xA4\xB7",
          "xn--example-", "xn---9uc"}) {
        idn_hostname(text, false);
    }
    // R-LDH: a host name (RFC 1123), never an internationalized one (RFC 5890 2.3.1).
    BOOST_TEST(formats::is_hostname("ab--cd.example"));
    // A host name's labels are judged one by one; an IDN's Bidi rule spans its labels.
    BOOST_TEST(formats::is_hostname("0a.xn--4db"));
    idn_hostname(std::string_view("b\xC3\xBC" "cher\0.example", 16), false);
    // The page's side-by-side table.
    idn_hostname("0a.xn--4db", false);
    idn_hostname("xn--nxasmq6b.example", true);
}

void idn_lengths() {
    // "xn--" and 59 octets of Punycode, from 55 "a" and one "ü": 63 octets, the limit; one more
    // "a" makes 64 (Python's punycode codec gives the same lengths).
    idn_hostname(std::string(55, 'a') + "\xC3\xBC", true);
    idn_hostname(std::string(56, 'a') + "\xC3\xBC", false);
    idn_hostname(std::string(64, 'a'), false);
    idn_hostname(std::string(63, 'a') + "." + std::string(63, 'a') + "." + std::string(63, 'a') +
                     "." + std::string(61, 'a'),
                 true);
}
```

`test/email_test.cpp` gains, with its call:

```cpp
void idn(std::string_view text, bool valid) {
    if (!BOOST_TEST_EQ(formats::is_idn_email(text), valid)) {
        std::cerr << "idn-email: \"" << text << "\"\n";
    }
    BOOST_TEST_EQ(formats::is_idn_email(text), urlg::parse(text, fg::idn_email_rule).has_value());
}

void idn_mailboxes() {
    for (const std::string_view text :
         {"\xEC\x8B\xA4\xEB\xA1\x80@\xEC\x8B\xA4\xEB\xA1\x80.\xED\x85\x8C\xEC\x8A\xA4\xED\x8A\xB8",
          "\xCE\xB4\xCE\xBF\xCE\xBA\xCE\xB9\xCE\xBC\xCE\xAE@example.com",
          "\"\xCE\xB4\xCE\xBF\xCE\xBA\xCE\xB9\xCE\xBC\xCE\xAE\"@example.com",
          "user@cafe\xCC\x81.com", "cafe\xCC\x81@example.com", "\xC2\x85@example.com",
          "\xEF\xBF\xBF@example.com", "\xF0\x9D\x95\x8F@example.com", "a@\xC3\xA9.org",
          "a@[IPv6:::1]", "joe.bloggs@example.com"}) {
        idn(text, true);
    }
    for (const std::string_view text :
         {"user\xEF\xBC\xA0" "example.com", "a\xC3@example.com", "\xCE\xB4@-example.com",
          "\xCE\xB4@example.com.", "\xCE\xB4@.example.com", "\xCE\xB4@example..com", "\xCE\xB4@",
          "a@b\xC2\xB7" "c.example", "a@ab--cd.example", "\"a\\\xC3\xA9\"@example.com", "2962"}) {
        idn(text, false);
    }
    std::string local32;
    for (int i = 0; i < 32; ++i) {
        local32 += "\xC3\xA9";
    }
    idn(local32 + "@example.com", true);
    idn(local32 + "a@example.com", false);
}
```

`test/grammar_test.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// formats' rules composed with Boost.URL's combinators: every public rule inside tuple_rule,
// variant_rule, optional_rule and range_rule; where each stops; a rule that fails as a whole
// rather than match a prefix; the host-and-port example of the page; and an IDN rule's error,
// which is idna's.

#include <webcpp/formats.hpp>

#include <webcpp/idna/errors.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/url/grammar/delim_rule.hpp>
#include <boost/url/grammar/optional_rule.hpp>
#include <boost/url/grammar/parse.hpp>
#include <boost/url/grammar/range_rule.hpp>
#include <boost/url/grammar/tuple_rule.hpp>
#include <boost/url/grammar/unsigned_rule.hpp>
#include <boost/url/grammar/variant_rule.hpp>

#include <cstdint>
#include <iostream>
#include <string_view>
#include <tuple>

namespace fg = webcpp::formats::grammar;
namespace idna = webcpp::idna;
namespace urlg = boost::urls::grammar;

namespace {

/** A comma, then an internationalized host name: the rest of a list of them. */
constexpr auto next_idn_host =
    urlg::tuple_rule(urlg::squelch(urlg::delim_rule(',')), fg::idn_hostname_rule);

/** A space, then a host name. */
constexpr auto next_host = urlg::tuple_rule(urlg::squelch(urlg::delim_rule(' ')), fg::hostname_rule);

/** Checks that `rule` matches `matched`, a prefix of `text`, and stops there. */
template <class Rule>
void stops(const Rule& rule, std::string_view text, std::string_view matched) {
    char const* it = text.data();
    const auto r = rule.parse(it, text.data() + text.size());
    if (!BOOST_TEST(r.has_value())) {
        std::cerr << "no match: \"" << text << "\"\n";
        return;
    }
    BOOST_TEST_EQ(std::string_view(text.data(), static_cast<std::size_t>(it - text.data())),
                  matched);
}

/** Checks that `rule` refuses `text` whole, rather than match a prefix, and leaves its input. */
template <class Rule>
void fails_whole(const Rule& rule, std::string_view text) {
    char const* it = text.data();
    const auto r = rule.parse(it, text.data() + text.size());
    if (!BOOST_TEST(r.has_error())) {
        std::cerr << "matched a prefix of: \"" << text << "\"\n";
    }
    BOOST_TEST(it == text.data());
}

void where_each_rule_stops() {
    stops(fg::date_time_rule, "2020-01-01T00:00:00Z/x", "2020-01-01T00:00:00Z");
    stops(fg::uuid_rule, "00000000-0000-0000-0000-000000000000x", "00000000-0000-0000-0000-000000000000");
    stops(fg::hostname_rule, "ex.ample_x", "ex.ample");
    stops(fg::idn_hostname_rule, "b\xC3\xBC" "cher.example/x", "b\xC3\xBC" "cher.example");
    stops(fg::idna_label_rule, "b\xC3\xBC" "cher.example", "b\xC3\xBC" "cher");
    stops(fg::idna_label_rule, "a\xE3\x80\x82" "b", "a");
    stops(fg::idna_label_rule, "a\xEF\xBC\x8E" "b", "a");
    stops(fg::idna_label_rule, "a\xEF\xBD\xA1" "b", "a");
    stops(fg::iri_rule, "http://a/\xC3\xA9>x", "http://a/\xC3\xA9");
    stops(fg::email_rule, "a@b.example,c@d.example", "a@b.example");
    stops(fg::idn_email_rule, "\xCE\xB4@b\xC3\xBC" "cher.example x",
          "\xCE\xB4@b\xC3\xBC" "cher.example");
    stops(fg::uri_template_rule, "{a}/b c", "{a}/b");
}

void runs_fail_whole() {
    fails_whole(fg::hostname_rule, "a..b:80");
    fails_whole(fg::idn_hostname_rule, "a\xC2\xB7" "b/x");
    fails_whole(fg::iri_rule, "http://a/\xE2\x80\x8E x");
}

void rules_inside_the_combinators() {
    const auto either = urlg::variant_rule(fg::date_time_rule, fg::date_rule, fg::time_rule);
    const auto date = urlg::parse("2020-01-01", either);
    BOOST_TEST(date.has_value() && date->index() == 1);
    const auto time = urlg::parse("12:00:00Z", either);
    BOOST_TEST(time.has_value() && time->index() == 2);
    const auto maybe = urlg::parse("", urlg::optional_rule(fg::uuid_rule));
    BOOST_TEST(maybe.has_value() && !maybe->has_value());
    const auto hosts = urlg::parse("a.example,b\xC3\xBC" "cher.example,c.example",
                                   urlg::range_rule(urlg::ref(fg::idn_hostname_rule),
                                                    urlg::ref(next_idn_host), 1));
    BOOST_TEST(hosts.has_value() && hosts->size() == 3);
    const auto ascii_hosts = urlg::parse(
        "a.example b.example", urlg::range_rule(urlg::ref(fg::hostname_rule), urlg::ref(next_host), 1));
    BOOST_TEST(ascii_hosts.has_value() && ascii_hosts->size() == 2);
    const auto refused_leap = urlg::parse(
        "23:59:60Z", urlg::variant_rule(fg::time_rule_t{webcpp::formats::leap_seconds::refused},
                                        fg::date_rule));
    BOOST_TEST(!refused_leap.has_value());
    const auto host_or_reference =
        urlg::parse("../a b", urlg::tuple_rule(urlg::optional_rule(fg::hostname_rule),
                                               fg::iri_reference_rule,
                                               urlg::squelch(urlg::delim_rule(' ')),
                                               fg::hostname_rule));
    BOOST_TEST(host_or_reference.has_value());
    const auto mailboxes = urlg::parse(
        "a@b.example;\"c d\"@[127.0.0.1]",
        urlg::tuple_rule(fg::email_rule, urlg::squelch(urlg::delim_rule(';')), fg::idn_email_rule));
    BOOST_TEST(mailboxes.has_value());
    const auto located = urlg::parse(
        "0/foo#2eb8aa08-aa98-11ea-b4aa-73b441d16380",
        urlg::tuple_rule(fg::relative_json_pointer_rule, urlg::squelch(urlg::delim_rule('#')),
                         fg::uuid_rule));
    BOOST_TEST(!located.has_value());  // a json-pointer token holds "#": the pointer runs to the end
    const auto template_and_iri = urlg::parse(
        "{a}/b http://a/\xC3\xA9",
        urlg::tuple_rule(fg::uri_template_rule, urlg::squelch(urlg::delim_rule(' ')), fg::iri_rule));
    BOOST_TEST(template_and_iri.has_value());
    const auto duration_and_reference = urlg::parse(
        "P1D ../x", urlg::tuple_rule(fg::duration_rule, urlg::squelch(urlg::delim_rule(' ')),
                                     fg::uri_reference_rule));
    BOOST_TEST(duration_and_reference.has_value());
    const auto pointer_and_label = urlg::parse(
        "/a/b", urlg::tuple_rule(fg::json_pointer_rule, urlg::optional_rule(fg::idna_label_rule)));
    BOOST_TEST(pointer_and_label.has_value());
    const auto iri_reference = urlg::parse("", fg::iri_reference_rule);
    BOOST_TEST(iri_reference.has_value());
}

void the_host_and_port_of_the_page() {
    constexpr auto idn_host_port = urlg::tuple_rule(
        fg::idn_hostname_rule,
        urlg::optional_rule(urlg::tuple_rule(urlg::squelch(urlg::delim_rule(':')),
                                             urlg::unsigned_rule<std::uint16_t>{})));
    const auto parsed = urlg::parse("b\xC3\xBC" "cher.example:8080", idn_host_port);
    if (BOOST_TEST(parsed.has_value())) {
        BOOST_TEST_EQ(std::get<0>(*parsed), "b\xC3\xBC" "cher.example");
        BOOST_TEST(std::get<1>(*parsed).has_value() && *std::get<1>(*parsed) == 8080);
    }
}

void an_idn_rules_error_is_idnas() {
    const auto contexto = urlg::parse("a\xC2\xB7" "b", fg::idn_hostname_rule);
    BOOST_TEST(contexto.has_error() && contexto.error() == idna::error::contexto);
    const auto hyphen = urlg::parse("-abc", fg::idna_label_rule);
    BOOST_TEST(hyphen.has_error() && hyphen.error() == idna::error::hyphen_at_start_or_end);
    const auto domain = urlg::parse("a@a\xC2\xB7" "b", fg::idn_email_rule);
    BOOST_TEST(domain.has_error() && domain.error().category() == idna::error_category());
}

}  // namespace

int main() {
    where_each_rule_stops();
    runs_fail_whole();
    rules_inside_the_combinators();
    the_host_and_port_of_the_page();
    an_idn_rules_error_is_idnas();
    return boost::report_errors();
}
```

`test/Jamfile`, after the email line:

```
# formats' rules composed with Boost.URL's combinators.
webcpp.run grammar : grammar_test.cpp ;
```

`test/allocation_test.cpp` gains, with its call:

```cpp
/** Internationalized names and addresses allocate nothing: idna works in fixed storage. */
void idn_allocates_nothing() {
    std::string long_idn;
    for (int i = 0; i < 4; ++i) {
        long_idn += "b\xC3\xBC" "cher.";
    }
    long_idn += "example";
    std::string hostile(100'000, 'a');
    hostile += "\xC3\xBC";
    const std::size_t before = global_allocations();
    BOOST_TEST(webcpp::formats::is_idn_hostname(long_idn));
    BOOST_TEST(!webcpp::formats::is_idn_hostname(hostile));
    BOOST_TEST(webcpp::formats::is_idn_email("\xCE\xB4@b\xC3\xBC" "cher.example"));
    BOOST_TEST_EQ(global_allocations() - before, 0U);
}
```

- [ ] **Step 2: Run them and watch them fail.**
  `B2 --build-dir=bin/formats-idn-native libs/formats/test//hostname libs/formats/test//email libs/formats/test//grammar`.
  Expected: FAIL: `is_idn_hostname`, `idn_hostname_rule`, `idna_label_rule`,
  `is_idn_email` and `idn_email_rule` are no members of their namespaces.

- [ ] **Step 3: Write the IDN halves.**
  `include/webcpp/formats/detail/idn_scan.hpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Where an internationalized name ends inside a larger grammar: UTS 46 may map any non-ASCII code
// point, so only ASCII ends a run.

#ifndef WEBCPP_FORMATS_DETAIL_IDN_SCAN_HPP
#define WEBCPP_FORMATS_DETAIL_IDN_SCAN_HPP

#include <webcpp/formats/config.hpp>

#include <webcpp/formats/detail/charsets.hpp>

namespace webcpp::formats::detail {

/** What an IDN run is: a whole domain name, or one label of it. */
enum class idn_run {
    /** A domain name: it holds "." and UTS 46's other label separators. */
    domain,
    /** One label: "." and the separators below end it. */
    label,
};

/** Whether the three bytes at `p` are U+3002, U+FF0E or U+FF61, which UTS 46 maps to ".". */
constexpr bool is_label_separator(char const* p, char const* end) noexcept {
    if (end - p < 3) {
        return false;
    }
    const auto b0 = static_cast<unsigned char>(p[0]);
    const auto b1 = static_cast<unsigned char>(p[1]);
    const auto b2 = static_cast<unsigned char>(p[2]);
    return (b0 == 0xE3U && b1 == 0x80U && b2 == 0x82U) ||
           (b0 == 0xEFU && b1 == 0xBCU && b2 == 0x8EU) ||
           (b0 == 0xEFU && b1 == 0xBDU && b2 == 0xA1U);
}

/**
 Where the IDN run that starts at `first` ends: at the first ASCII byte outside letters, digits,
 "-" and "."; a label's also at "." and the three separators of is_label_separator.
*/
constexpr char const* scan_idn_run(char const* first, char const* end, idn_run kind) noexcept {
    char const* last = first;
    while (last != end) {
        if (static_cast<unsigned char>(*last) >= 0x80U) {
            if (kind == idn_run::label && is_label_separator(last, end)) {
                break;
            }
            ++last;
            continue;
        }
        const bool ends = *last == '.' ? kind == idn_run::label : !ldh_chars(*last);
        if (ends) {
            break;
        }
        ++last;
    }
    return last;
}

}  // namespace webcpp::formats::detail

#endif  // WEBCPP_FORMATS_DETAIL_IDN_SCAN_HPP
```

  `include/webcpp/formats/hostname.hpp` gains
  `#include <webcpp/formats/detail/idn_scan.hpp>`,
  `#include <webcpp/idna/uts46.hpp>` and `<span>`; its file comment becomes
  "Host names, RFC 1123's and internationalized ones, ...". After
  `hostname_rule_t::parse`'s definition, in `namespace grammar`:

```cpp
/**
 The rule of an internationalized host name: UTS 46's processing and IDNA2008's validity, through
 `webcpp::idna::to_ascii`.

 idna's default options: UTS 46's mapping, nontransitional, every check, the DNS lengths, and
 IDNA2008's stricter validity (CONTEXTO, NV8 and XV8 refused). So fullwidth digits, an ignored
 U+200B, a name not in NFC and the dots U+3002, U+FF0E and U+FF61 are valid where IDNA2008 allows
 the result. It takes the run up to the first ASCII byte outside letters, digits, `-` and `.`
 (every non-ASCII byte belongs to it, since UTS 46 may map it) and validates it whole.

 @see "Addresses", in the guide.
*/
struct idn_hostname_rule_t {
    /** The rule yields the text it matched. */
    using value_type = text;

    /**
     Matches an internationalized host name at `it`.

     @param it The start of the input: past the match on success, else unchanged.
     @param end The end of the input.
     @return The text matched; `error::mismatch` when no name's byte is at `it`; else idna's error
     when idna refuses the run.
    */
    [[nodiscard]] result<text> parse(char const*& it, char const* end) const noexcept;
};

/**
 The rule of one label of an internationalized host name, a terminal of formats' own that asks
 `webcpp::idna` for its verdict.

 The run stops also at `.`, U+3002, U+FF0E and U+FF61, so a grammar composes labels with its own
 separators. The label is checked as @ref idn_hostname_rule checks a name of one label.

 @see "Composing rules", in the guide.
*/
struct idna_label_rule_t {
    /** The rule yields the text it matched. */
    using value_type = text;

    /**
     Matches one internationalized label at `it`.

     @param it The start of the input: past the match on success, else unchanged.
     @param end The end of the input.
     @return The text matched; `error::mismatch` when no label starts at `it`; else idna's error.
    */
    [[nodiscard]] result<text> parse(char const*& it, char const* end) const noexcept;
};

/** The rule of an internationalized host name. */
inline constexpr idn_hostname_rule_t idn_hostname_rule{};

/** The rule of one internationalized label. */
inline constexpr idna_label_rule_t idna_label_rule{};

inline result<text> idn_hostname_rule_t::parse(char const*& it, char const* end) const noexcept {
    char const* const last = detail::scan_idn_run(it, end, detail::idn_run::domain);
    if (last == it) {
        return detail::urlg::error::mismatch;
    }
    std::array<char, idna::max_domain_length> ascii{};
    if (const auto converted =
            idna::to_ascii(ascii, std::string_view(it, static_cast<std::size_t>(last - it)));
        !converted) {
        return converted.error();
    }
    char const* const first = it;
    it = last;
    return text(first, static_cast<std::size_t>(last - first));
}

inline result<text> idna_label_rule_t::parse(char const*& it, char const* end) const noexcept {
    char const* const last = detail::scan_idn_run(it, end, detail::idn_run::label);
    if (last == it) {
        return detail::urlg::error::mismatch;
    }
    std::array<char, idna::max_domain_length> ascii{};
    if (const auto converted =
            idna::to_ascii(ascii, std::string_view(it, static_cast<std::size_t>(last - it)));
        !converted) {
        return converted.error();
    }
    char const* const first = it;
    it = last;
    return text(first, static_cast<std::size_t>(last - first));
}
```

  and after `is_hostname`:

```cpp
/**
 Whether `s` is an internationalized host name, JSON Schema's `idn-hostname`.

 @param s Any byte string.
 @return `parse(s, grammar::idn_hostname_rule).has_value()`.
 @see "Addresses", in the guide.
*/
[[nodiscard]] inline bool is_idn_hostname(std::string_view s) noexcept {
    return detail::urlg::parse(s, grammar::idn_hostname_rule).has_value();
}
```

  `include/webcpp/formats/email.hpp`: in `detail`, after `local_part_rule`:

```cpp
/** RFC 6531 3.3's Local-part: atext and qtextSMTP with UTF8-non-ascii; quoted-pairSMTP stays ASCII. */
inline constexpr local_part_rule_t idn_local_part_rule{true};
```

  in `namespace grammar`, after `email_rule_t::parse`'s definition:

```cpp
/**
 The rule of RFC 6531 3.3's internationalized Mailbox: @ref email_rule with UTF-8 in the local part
 and an internationalized domain.

 `atext` and `qtextSMTP` also hold any well-formed non-ASCII UTF-8 sequence (RFC 6532 3.1, C1
 controls and noncharacters included), `quoted-pairSMTP` stays ASCII, the local part is at most 64
 octets and is not normalized; the domain is @ref idn_hostname_rule, whose error, idna's, the rule
 returns as it is, or an address literal.

 @see "Addresses", in the guide.
*/
struct idn_email_rule_t {
    /** The rule yields the text it matched. */
    using value_type = text;

    /**
     Matches an internationalized Mailbox at `it`.

     @param it The start of the input: past the match on success, else unchanged.
     @param end The end of the input.
     @return The text matched; `error::mismatch` when no local part starts at `it`;
     `error::invalid` when the rest breaks the grammar; idna's error when idna refuses the domain.
    */
    [[nodiscard]] result<text> parse(char const*& it, char const* end) const noexcept;
};

/** The rule of RFC 6531's Mailbox. */
inline constexpr idn_email_rule_t idn_email_rule{};

inline result<text> idn_email_rule_t::parse(char const*& it, char const* end) const noexcept {
    return detail::parse_mailbox(it, end, detail::idn_local_part_rule, idn_hostname_rule);
}
```

  and after `is_email`:

```cpp
/**
 Whether `s` is an RFC 6531 internationalized Mailbox, JSON Schema's `idn-email`.

 @param s Any byte string.
 @return `parse(s, grammar::idn_email_rule).has_value()`.
 @see "Addresses", in the guide.
*/
[[nodiscard]] inline bool is_idn_email(std::string_view s) noexcept {
    return detail::urlg::parse(s, grammar::idn_email_rule).has_value();
}
```

  `include/webcpp/formats/grammar.hpp`: its file comment's and `text`'s
  `@see` become `@see "Composing rules", in the guide.`

- [ ] **Step 4: Run them and watch them pass.**
  `B2 --build-dir=bin/formats-idn-native libs/formats/test//hostname libs/formats/test//email libs/formats/test//grammar`:
  PASS. A
  failure whose verdict is idna's becomes an idna test, red first, reported
  to the idna track. Then the five lanes with `<t>` = `idn`: every lane
  green, `hostname`, `email`, `grammar` and `allocation` passed on all four
  targets.

- [ ] **Step 5: The page.** `doc/addresses.adoc` gains, after its host
  names subsection and before its e-mail one:

```
[#formats-idn-hostnames]
=== Internationalized host names

`is_idn_hostname` checks a host name that may hold any script, by UTS 46's
processing and IDNA2008's validity, through `webcpp::idna`'s `to_ascii` with
its default options (link:{webcpp-libs}/idna/{webcpp-page}#idna-introduction[idna's
strict profile]). The name is mapped as UTS 46 says, so fullwidth digits, an
ignored U+200B, a name not in NFC and the ideographic full stop U+3002 are
valid where IDNA2008 accepts the result; then every label must be valid in
IDNA2008, CONTEXTO and the Bidi rule across the labels included, and the
name fit DNS's lengths in its ASCII form. An R-LDH label such as `ab--cd` is
a host name and not an internationalized one. A host name's A-labels are
judged one at a time, so `0a.xn--4db` is a valid `hostname` while its Unicode
form `0a.א` is no `idn-hostname`: the Bidi rule spans a domain's labels.
```

  and after its e-mail subsection:

```
`is_idn_email` checks RFC 6531's internationalized Mailbox: the local part
may hold any well-formed non-ASCII UTF-8, C1 controls and noncharacters
included, and is not normalized; the domain is an internationalized host
name or an address literal.
```

  The anchor `idna-introduction` is the one idna's page gives its
  introduction (`[#idna-introduction]`, which idna's plan writes in its I0;
  the doc build fails if it is missing, as AGENTS chapter 8 says). After the internationalized host names' paragraph, the verdicts the
  two formats give side by side (spec risk 11):

```
[%header,cols="3,2,2"]
|===
| String | `is_hostname` | `is_idn_hostname`
| `ab--cd.example` | valid | invalid
| `0a.xn--4db` | valid | invalid
| `0a.א` | invalid | invalid
| `xn--nxasmq6b.example` | valid | valid
| `bücher.example` | invalid | valid
|===
```

  The e-mail subsection's paragraph on its address literal gains, at its
  end: "A grammar of a program's own composes formats' rules the same way
  (<<formats-composing>>)." 

  `doc/composing.adoc`, after `regex.adoc`'s place:

```
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

[#formats-composing]
== Composing rules

Every format but `regex` is also a Boost.URL rule, in
`webcpp::formats::grammar`: an object whose type `<name>_rule_t` is public,
so a rule that takes a parameter, `time_rule_t{leap_seconds::refused}` or
`iri_rule_t{&resource}`, is spelled without a factory. A rule yields the
text it matched, as a `grammar::text`; `uri_reference_rule` yields
Boost.URL's `url_view`. `ipv4`, `ipv6` and `uri` are Boost.URL's own rules,
`boost::urls::ipv4_address_rule`, `ipv6_address_rule` and `uri_rule`.

A rule composes with Boost.URL's combinators, `tuple_rule`, `variant_rule`,
`optional_rule`, `range_rule`, into a grammar of a program's own. Where a
rule stops inside a longer input is part of its contract:

[%header,cols="2,5"]
|===
| Rule | Stops
| the ASCII rules | where their grammar does
| `hostname_rule` | at the first character outside letters, digits, `-` and `.`
| `idn_hostname_rule` | at the first ASCII character outside letters, digits, `-` and `.`; every non-ASCII byte belongs to the name, since UTS 46 may map it
| `idna_label_rule` | as `idn_hostname_rule`, and also at `.`, U+3002, U+FF0E and U+FF61
| `iri_rule`, `iri_reference_rule` | at the first byte no IRI holds: an ASCII control, the space, the double quote, the less-than and greater-than signs, the backslash, the circumflex, the grave accent, a brace or the vertical bar
| `json_pointer_rule` | at the end of the input: a token holds every other character
|===

Each validates the text it took and fails as a whole rather than match a
shorter prefix, and a rule that fails leaves its input where it was.

Two facts of Boost.URL shape a composition. A `range_rule` over a rule
larger than 128 bytes keeps it in a pool of Boost.URL's own, which
allocates: pass such a rule through `boost::urls::grammar::ref`, as formats
does. And `parse` of a whole string fails with `error::leftover` when the
rule stops early, which is how a validator refuses trailing input.
```

- [ ] **Step 6: Register with the suite.** `validators` gains, sorted:

```cpp
    mapping{"idn-email", [](std::string_view s) { return formats::is_idn_email(s); }},
    mapping{"idn-hostname", [](std::string_view s) { return formats::is_idn_hostname(s); }},
```

  `pending` loses `"idn-email"` and `"idn-hostname"`. The native and emscripten lanes: `suite` passes, and each dialect's
  `string tests` grows by, and `pending` falls by, draft4 0, draft6 0,
  draft7 110, draft2019-09 110, draft2020-12 110, v1 111. A failure whose
  verdict is idna's goes to idna, red first; the only entry a known-failure
  list may take is a residue of the Unicode version (idna's tables are
  17.0.0), its reason naming both versions, which the report lists.

- [ ] **Step 7: Build the page, run the lint.** The doc build with
  `<t>` = `idn` exits 0, idna's page built first for the link; `LINT`:
  `lint: clean`.

- [ ] **Step 8: Commit** in `~/dev/webcpp-formats-idn/libs/formats`:

```bash
git add include/webcpp/formats/detail/idn_scan.hpp include/webcpp/formats/hostname.hpp \
  include/webcpp/formats/email.hpp include/webcpp/formats/grammar.hpp test/hostname_test.cpp \
  test/email_test.cpp test/grammar_test.cpp test/Jamfile test/allocation_test.cpp \
  test/suite_test.cpp doc/addresses.adoc doc/composing.adoc doc/formats.adoc
git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "formats: internationalized host names and e-mail addresses through idna, and the rules composed with Boost.URL's combinators"
```

The report's Changelog: the "New Libraries" line, unchanged.

### Task 10: Every target, the page, readiness (F8)

Works in `~/dev/webcpp-formats/libs/formats` on `main`, from
`~/dev/webcpp-formats` at `B-S5` or later (S1 to S5 all there once Task 6
has run), with every earlier task integrated, and `libs/idna` at the commit
that carries I6 (its page complete, for the links). Needs S5, Tasks 1 to 9,
idna I6. Nothing runs beside it in this repository, and the idna track's
working tree is clean at a commit while it runs.

**Files:**
- Create: `example/Jamfile`, `example/.clang-tidy`, `example/formats_quick_start.cpp`, `example/formats_quick_start.expected`, `example/formats_grammar.cpp`, `example/formats_grammar.expected`, `example/formats_iri.cpp`, `example/formats_iri.expected`, `example/formats_regex.cpp`, `example/formats_regex.expected`, `doc/counts.py`, `doc/allocation.adoc`, `doc/suite.adoc`, `doc/guarantees.adoc`, `doc/deviations.adoc`
- Modify: `test/Jamfile` (the suite on every target), `test/suite_test.cpp` (the pending list leaves), `test/allocation_test.cpp` (a megabyte of every format), `test/date_time_test.cpp`, `test/detail_test.cpp`, `test/grammar_test.cpp` (their `(doc: #...)` comments), `doc/formats.adoc`, `doc/introduction.adoc`, `doc/uris.adoc`, `doc/regex.adoc`, `doc/composing.adoc`, `doc/testing.adoc`, `README.md`

**Interfaces:**
- Consumes: infra's `[ webcpp.data <directory> + ]` (`tools/webcpp.jam`), requirements that give a `webcpp.run` program absolute directories at the same paths on every target, on WASI as `wasmtime --dir`; `/webcpp/third_party//json-schema-test-suite`, whose usage requirements already carry `[ webcpp.data ]` of the suite's own directory; every validator and rule of Tasks 2 to 9.
- Produces: the published state of the library; the page's attributes `{n-suite-<dialect>}`, `{n-suite-passed-<dialect>}`, `{n-suite}`, `{n-suite-passed}`; the anchors `formats-invariant-1` to `formats-invariant-5`.

- [ ] **Step 1: The suite on every target.** In `test/Jamfile`, `suite` loses
  its `: native emscripten`. Run the wasip2 lane with `<t>` = `ready`.
  Expected: FAIL, `.../known_failures/draft4.json: cannot be opened` and the
  same for each dialect (the suite's own directory reaches the program
  through the suite's target; the lists' does not yet). Then its
  requirements gain `[ webcpp.data $(KNOWN_FAILURES) ]`, and the comment
  above it becomes:

```
# The JSON Schema Test Suite comes from /webcpp/third_party, which gives its
# root as WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE and its directory to a WASI
# program; the known-failure lists are formats' own, named with forward
# slashes, the define quoted whole, for a checkout whose path holds a
# space, and given to a WASI program by webcpp.data. On every target:
# natively, on emscripten through -sNODERAWFS=1, under wasmtime through
# --dir.
```

  Expected: PASS on wasip2 and wasip3, with the summary of Step 2.

- [ ] **Step 2: The pending list leaves.** Every format has its validator
  now, so `test/suite_test.cpp` loses the array `pending`, its comment, the
  field `pending` of `tally`, and its use: in `run_file` the line
  `const bool is_pending = ...` and the check becomes

```cpp
        if (!BOOST_TEST(at.valid != nullptr || format == unknown_format)) {
```

  in `run_test` the lines

```cpp
    if (at.valid == nullptr) {
        ++counted.pending;
        return;
    }
```

  leave, and the summary line becomes

```cpp
    std::cout << d.name << ": " << counted.strings << " string tests, " << counted.known
              << " known failures, " << counted.non_strings << " non-string, " << counted.skipped
              << " skipped (unknown)\n";
```

  The five lanes with `<t>` = `ready`; then
  `find bin/formats-ready-wasip2 -name 'suite.output' -exec cat {} +`.
  Expected on each of native, gcc, wasip2, wasip3 and emscripten:

```
draft4: 248 string tests, 0 known failures, 42 non-string, 1 skipped (unknown)
draft6: 348 string tests, 0 known failures, 60 non-string, 1 skipped (unknown)
draft7: 684 string tests, 0 known failures, 108 non-string, 1 skipped (unknown)
draft2019-09: 753 string tests, 0 known failures, 120 non-string, 1 skipped (unknown)
draft2020-12: 753 string tests, 0 known failures, 120 non-string, 1 skipped (unknown)
v1: 754 string tests, 0 known failures, 114 non-string, 0 skipped (unknown)
```

  3,540 string tests, every known-failure list empty. A list that holds an
  entry here holds only a residue tied to a Unicode version, its reason
  naming both versions, and the report names it for the owner.

- [ ] **Step 3: A megabyte of every format** (Review Focus 1, the spec's
  section 7). `test/allocation_test.cpp` gains, with its call:

```cpp
/**
 Every validator answers a hostile megabyte without a crash and without allocating, but through
 the resources it is given: digits, a token, a label and a code point repeated a million times.
*/
void every_format_answers_a_megabyte() {
    const std::string digits(1 << 20, '7');
    const std::string letters(1 << 20, 'a');
    std::string umlauts;
    for (int i = 0; i < (1 << 19); ++i) {
        umlauts += "\xC3\xBC";
    }
    std::string braces;
    for (int i = 0; i < (1 << 17); ++i) {
        braces += "{a,b}/x";
    }
    webcpp::test::counting_resource resource;
    const std::size_t before = global_allocations();
    namespace formats = webcpp::formats;
    BOOST_TEST(!formats::is_date_time(digits));
    BOOST_TEST(!formats::is_time(digits));
    BOOST_TEST(!formats::is_duration(digits));
    BOOST_TEST(!formats::is_uuid(letters));
    BOOST_TEST(!formats::is_json_pointer(letters));
    BOOST_TEST(formats::is_relative_json_pointer(digits));
    BOOST_TEST(!formats::is_ipv4(digits));
    BOOST_TEST(!formats::is_ipv6(digits));
    BOOST_TEST(!formats::is_uri(letters));
    BOOST_TEST(formats::is_uri_reference(letters));
    BOOST_TEST(formats::is_iri_reference(umlauts, resource));
    BOOST_TEST(formats::is_uri_template(braces));
    BOOST_TEST(!formats::is_hostname(letters));
    BOOST_TEST(!formats::is_idn_hostname(umlauts));
    BOOST_TEST(!formats::is_email(letters));
    BOOST_TEST(!formats::is_idn_email(umlauts));
    BOOST_TEST(formats::is_regex(letters, resource));
    BOOST_TEST_EQ(global_allocations() - before, resource.allocations);
}
```

  Run the five lanes: `allocation` passes on each, within b2's default
  timeouts. A validator that takes quadratic time or traps here is a defect
  of the task that wrote it, fixed with this test red first.

- [ ] **Step 4: The stack, measured.** In the job's scratch directory
  `$SCRATCH` (never committed), `stack_probe.cpp` calls, once each, the
  validators that hold the most on the stack: `is_iri` on an IRI mapping to
  exactly 1,024 octets (Task 7's `at_capacity`), `is_idn_hostname` and
  `is_idn_email` on 253-octet names of 63-octet IDN labels, and returns 0
  when all answer as expected. Build it for wasip2 at b2's debug flags
  (`B2 -n -a toolset=clang-wasip2 libs/formats/test//grammar` prints the
  ones b2 uses; the command below is them, with Boost's include directory
  read from `.local/user-config.jam`), with idna's include directory and a
  wasip2 debug `libboost_url` archive built here, at decreasing stack sizes:

```bash
B2 --build-dir=bin/formats-ready-stack toolset=clang-wasip2 /webcpp//boost_url
ARCHIVE=$(find bin/formats-ready-stack -name 'libboost_url*.a' | head -n 1)
BOOST_INCLUDE=$(sed -n 's/.*using boost.*<include>\([^ ]*\).*/\1/p' .local/user-config.jam)
for n in 65536 32768 16384 8192 4096; do
  .local/wasi-sdk/bin/clang++ --target=wasm32-wasip2 -std=c++20 -O0 -g \
    -fwasm-exceptions -mllvm -wasm-use-legacy-eh=false \
    -I libs/formats/include -I libs/idna/include -isystem "$BOOST_INCLUDE" \
    "$SCRATCH/stack_probe.cpp" "$ARCHIVE" -fwasm-exceptions -lunwind \
    -Wl,-z,stack-size=$n -o "$SCRATCH/stack_$n.wasm" \
  && wasmtime run "$SCRATCH/stack_$n.wasm"; echo "$n: $?"
done
```

  Expected: 0 for 65,536 and for each size down to the smallest that
  suffices, then a trap (`call stack exhausted`). Halve the interval
  between the last 0 and the first trap down to 256 bytes. Record the
  smallest stack that suffices in the ledger and in `doc/allocation.adoc`
  (Step 6), as "formats' deepest call needs <n> bytes of wasm32-wasip2's
  64 KiB default stack, at -O0 (measured 2026-10-<day>)", with the measured
  number and day.

- [ ] **Step 5: The examples.** `example/Jamfile`:

```
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

project : requirements <library>/webcpp/formats//formats ;

import webcpp ;

webcpp.targets native wasip2 wasip3 emscripten ;

# The guide's programs, each shown on the page with the output it prints.
webcpp.example formats_quick_start.cpp ;
webcpp.example formats_grammar.cpp ;
webcpp.example formats_iri.cpp ;
webcpp.example formats_regex.cpp ;
```

  `example/.clang-tidy`: pratt's `libs/pratt/example/.clang-tidy`.

  `example/formats_quick_start.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// A few validators, each a function of a std::string_view that answers bool.

#include <webcpp/formats.hpp>

#include <iostream>
#include <string_view>

namespace formats = webcpp::formats;

int main() {
    // tag::program[]
    struct check {
        std::string_view format;
        std::string_view text;
        bool (*valid)(std::string_view);
    };
    for (const check& c : {
             check{"date-time", "1985-04-12T23:20:50.52Z", [](std::string_view s) { return formats::is_date_time(s); }},
             check{"date", "2021-02-29", [](std::string_view s) { return formats::is_date(s); }},
             check{"email", "joe.bloggs@[IPv6:::1]", [](std::string_view s) { return formats::is_email(s); }},
             check{"hostname", "xn--nxasmq6b.example", [](std::string_view s) { return formats::is_hostname(s); }},
             check{"uri", "//example.com/p", [](std::string_view s) { return formats::is_uri(s); }},
             check{"uri-reference", "//example.com/p", [](std::string_view s) { return formats::is_uri_reference(s); }},
             check{"json-pointer", "/a~1b", [](std::string_view s) { return formats::is_json_pointer(s); }},
         }) {
        std::cout << c.format << ' ' << c.text << ": " << (c.valid(c.text) ? "valid" : "invalid")
                  << '\n';
    }
    // end::program[]
}
```

  (clang-format lays the list out; the lines above show its content.)
  `example/formats_quick_start.expected`:

```
date-time 1985-04-12T23:20:50.52Z: valid
date 2021-02-29: invalid
email joe.bloggs@[IPv6:::1]: valid
hostname xn--nxasmq6b.example: valid
uri //example.com/p: invalid
uri-reference //example.com/p: valid
json-pointer /a~1b: valid
```

  `example/formats_grammar.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// formats' rules composed with Boost.URL's combinators: an internationalized host and an optional
// port, and who refused what.

#include <webcpp/formats.hpp>

#include <boost/url/grammar/delim_rule.hpp>
#include <boost/url/grammar/optional_rule.hpp>
#include <boost/url/grammar/parse.hpp>
#include <boost/url/grammar/tuple_rule.hpp>
#include <boost/url/grammar/unsigned_rule.hpp>

#include <cstdint>
#include <iostream>
#include <string_view>

namespace urlg = boost::urls::grammar;
namespace fg = webcpp::formats::grammar;

int main() {
    // tag::host_port[]
    constexpr auto idn_host_port = urlg::tuple_rule(
        fg::idn_hostname_rule,
        urlg::optional_rule(urlg::tuple_rule(urlg::squelch(urlg::delim_rule(':')),
                                             urlg::unsigned_rule<std::uint16_t>{})));

    for (const std::string_view text : {"b\xC3\xBC" "cher.example:8080", "example.com",
                                        "a\xC2\xB7" "b.example:80", "example.com:99999"}) {
        const auto parsed = urlg::parse(text, idn_host_port);
        if (parsed.has_error()) {
            std::cout << text << " -> refused by " << parsed.error().category().name() << '\n';
            continue;
        }
        const auto& [host, port] = *parsed;
        std::cout << text << " -> host " << host;
        if (port) {
            std::cout << ", port " << *port;
        }
        std::cout << '\n';
    }
    // end::host_port[]
}
```

  `example/formats_grammar.expected` (UTF-8):

```
bücher.example:8080 -> host bücher.example, port 8080
example.com -> host example.com
a·b.example:80 -> refused by webcpp.idna
example.com:99999 -> refused by boost.url.grammar
```

  `example/formats_iri.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// IRIs with a memory resource of the program's own: a long IRI's mapped copy is taken from it, a
// short one stays on the stack.

#include <webcpp/formats.hpp>

#include <array>
#include <cstddef>
#include <iostream>
#include <memory_resource>
#include <string>

namespace formats = webcpp::formats;

int main() {
    // tag::resource[]
    std::array<std::byte, 4096> storage{};
    std::pmr::monotonic_buffer_resource resource(storage.data(), storage.size(),
                                                 std::pmr::null_memory_resource());
    std::string long_iri = "http://example.com/";
    for (int i = 0; i < 300; ++i) {
        long_iri += "\xC3\xA9";  // 300 "é": 1,819 octets once mapped, past the 1,024 on the stack
    }
    std::cout << "short: "
              << formats::is_iri("http://\xC6\x92\xC3\xB8\xC3\xB8.example/\xCF\x80", resource) << '\n';
    std::cout << "long: " << formats::is_iri(long_iri, resource) << '\n';
    std::cout << "bidi: " << formats::is_iri("http://a/\xE2\x80\x8E", resource) << '\n';
    // end::resource[]
}
```

  `example/formats_iri.expected`:

```
short: 1
long: 1
bidi: 0
```

  `example/formats_regex.cpp`:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// ECMA-262 patterns, compiled by webcpp::regex in a pool of the program's own.

#include <webcpp/formats/regex.hpp>

#include <iostream>
#include <memory_resource>
#include <string_view>

int main() {
    // tag::program[]
    std::pmr::unsynchronized_pool_resource pool;
    for (const std::string_view pattern :
         {"^\\p{Letter}+$", "(?<year>\\d{4})-(?<month>\\d{2})", "^(abc]", "\\Aabc\\z", "(?i)abc"}) {
        std::cout << pattern << " -> "
                  << (webcpp::formats::is_regex(pattern, pool) ? "valid" : "invalid") << '\n';
    }
    // end::program[]
}
```

  `example/formats_regex.expected`:

```
^\p{Letter}+$ -> valid
(?<year>\d{4})-(?<month>\d{2}) -> valid
^(abc] -> invalid
\Aabc\z -> invalid
(?i)abc -> invalid
```

  Run the five lanes, each now naming `libs/formats/example` too. Expected:
  each example built, run and compared on every target, every lane green.
  An output that differs is a defect of the example or of the library, never
  an `.expected` rewritten to match without that check.

- [ ] **Step 6: The page, complete.** `doc/counts.py`:

```python
#!/usr/bin/env python3
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Counts what formats' page says of the JSON-Schema-Test-Suite, which no other count holds.

tools/doc/counts.py runs this script with formats' directory as its one argument, and gives the
page each `name=<number>` line it prints as an attribute: for each dialect, the string tests of the
format files the runner reads, `n-suite-<dialect>`, and of those the ones expected to pass,
`n-suite-passed-<dialect>`, the first less the dialect's known failures; and their sums, `n-suite`
and `n-suite-passed`. The unknown format's tests are json_schema's and are not counted. A dialect
whose directory holds no format file is a fault, rather than a zero on the page. A fault is
written on the standard error, and the standard output holds only counts.

Usage: counts.py <library-directory>. Exit 0 printing the counts, 1 naming a fault, 2 on a usage
error.
"""

from __future__ import annotations

import json
import sys
from pathlib import Path

DIALECTS = [
    ('draft4', 'draft4/optional/format'),
    ('draft6', 'draft6/optional/format'),
    ('draft7', 'draft7/optional/format'),
    ('draft2019-09', 'draft2019-09/optional/format'),
    ('draft2020-12', 'draft2020-12/optional/format'),
    ('v1', 'v1/format'),
]
UNKNOWN = 'unknown'


class Fault(Exception):
    """A count that cannot be made, said to the user."""


def strings(directory: Path) -> int:
    """The string tests of every format file in `directory`, but the unknown format's."""
    files = sorted(directory.glob('*.json'))
    if not files:
        raise Fault(f'counts.py: no format tests in {directory}')
    count = 0
    for path in files:
        for case in json.loads(path.read_text(encoding='utf-8')):
            if case['schema'].get('format') == UNKNOWN:
                continue
            count += sum(1 for test in case['tests'] if isinstance(test['data'], str))
    return count


def known(library: Path, dialect: str) -> int:
    """The entries of the dialect's known-failure list."""
    path = library / 'test' / 'known_failures' / f'{dialect}.json'
    entries = json.loads(path.read_text(encoding='utf-8'))
    if not isinstance(entries, list):
        raise Fault(f'counts.py: {path} is not a list')
    return len(entries)


def main(argv: list[str]) -> int:
    """Prints the counts of the library at argv[1]."""
    if len(argv) != 2:
        print('usage: counts.py <library-directory>', file=sys.stderr)
        return 2
    library = Path(argv[1]).resolve()
    tests = library.parent.parent / 'third_party' / 'libs' / 'JSON-Schema-Test-Suite' / 'tests'
    lines: list[str] = []
    total = 0
    passed = 0
    try:
        for dialect, directory in DIALECTS:
            run = strings(tests / directory)
            expected = run - known(library, dialect)
            lines += [f'n-suite-{dialect}={run}', f'n-suite-passed-{dialect}={expected}']
            total += run
            passed += expected
    except (Fault, OSError, ValueError, KeyError) as fault:
        print(fault, file=sys.stderr)
        return 1
    lines += [f'n-suite={total}', f'n-suite-passed={passed}']
    print('\n'.join(lines))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
```

  Run `python3 libs/formats/doc/counts.py libs/formats`. Expected:

```
n-suite-draft4=248
n-suite-passed-draft4=248
n-suite-draft6=348
n-suite-passed-draft6=348
n-suite-draft7=684
n-suite-passed-draft7=684
n-suite-draft2019-09=753
n-suite-passed-draft2019-09=753
n-suite-draft2020-12=753
n-suite-passed-draft2020-12=753
n-suite-v1=754
n-suite-passed-v1=754
n-suite=3540
n-suite-passed=3540
```

  `doc/suite.adoc`, after `allocation.adoc`'s place:

```
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

[#formats-suite]
== The JSON Schema suite

The official JSON-Schema-Test-Suite holds, for each draft, the format tests
of `optional/format/`, and for v1 those of `format/`, which v1 requires. A
program of formats' tests runs each string test against the validator of its
format, on every target formats builds for: `is_time(s,
leap_seconds::refused)` for v1's `time`, the same call in every other draft.
A test whose instance is not a string checks json_schema's rule that a format
applies to strings only, and the `unknown` format json_schema's handling of a
format it does not know: neither is a question for a validator.

[%header,cols="3,2,2"]
|===
| Dialect | String tests | Passing
| draft 4 | {n-suite-draft4} | {n-suite-passed-draft4}
| draft 6 | {n-suite-draft6} | {n-suite-passed-draft6}
| draft 7 | {n-suite-draft7} | {n-suite-passed-draft7}
| draft 2019-09 | {n-suite-draft2019-09} | {n-suite-passed-draft2019-09}
| draft 2020-12 | {n-suite-draft2020-12} | {n-suite-passed-draft2020-12}
| v1 | {n-suite-v1} | {n-suite-passed-v1}
| all | {n-suite} | {n-suite-passed}
|===

A test expected to fail is listed in its dialect's known-failure list with
its reason; the only reason allowed is a difference of Unicode versions,
since idna's tables are those of Unicode 17.0.0. The program fails on a
failure no list names, and on a listed test that passes.
```

  `doc/allocation.adoc`, after `errors.adoc`'s place:

```
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

[#formats-allocation]
== Allocation

[%header,cols="3,5"]
|===
| Function or rule | Allocates
| every validator and rule but those below | never: Boost.URL's views and formats' rules hold no storage, idna works in fixed storage, and a `range_rule` keeps its rule inline
| `is_iri`, `is_iri_reference`, `iri_rule`, `iri_reference_rule` | never for an ASCII IRI or one whose mapped URI fits 1,024 octets; beyond, one buffer of the mapped length, from the resource given, else `std::pmr::get_default_resource()`
| `is_regex` | webcpp::regex's compilation, only through the resource given, else the default resource, and all of it given back before it returns
|===

The stack holds the rest: the IRI's 1,024 octets and idna's working storage
of a label. formats' deepest call needs <n> bytes of wasm32-wasip2's 64 KiB
default stack, at -O0 (measured 2026-10-<day>).
```

  (`<n>` and `<day>` are Step 4's measurement, written as numbers.)

  `doc/guarantees.adoc`, after `suite.adoc`'s place:

```
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

[#formats-guarantees]
== Guarantees

. [[formats-invariant-1]]For every format, `is_x(s)` and `parse(s, x_rule)` give the same answer for every string.
. [[formats-invariant-2]]No function or rule allocates but those the allocation table names, and those only through a memory resource.
. [[formats-invariant-3]]Nothing throws but a memory resource that cannot allocate.
. [[formats-invariant-4]]Every function refuses a string that is not well-formed UTF-8.
. [[formats-invariant-5]]A rule that fails leaves its iterator where it was.
```

  `doc/deviations.adoc`, after the reference:

```
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

[appendix]
[#formats-deviations]
== Known deviations

* **Zone identifiers.** `is_uri`, `is_uri_reference` and the IRI functions
  accept RFC 6874's zone identifier, `http://[fe80::1%25eth0]/`, as
  Boost.URL does. RFC 3986 alone has none, and JSON Schema's suite does not
  test one.
* **JSON Schema v1's `date-time`.** v1's text asks for RFC 9557's
  `date-time-ext` suffixes and for leap seconds checked against those
  published; no test of the suite holds either, and `is_date_time` is RFC
  3339's. When v1 is published they come as a new function or policy, never
  as a changed default.
* **SRELL's extensions.** SRELL accepts `\A`, `\Z` and `\z`, which ECMA-262
  refuses in `u` mode; webcpp::regex refuses them on SRELL.
* **Deep and wide patterns.** `is_regex` refuses, without judging it, a
  pattern whose groups nest deeper than `webcpp::regex::max_nesting` or that
  holds more than `webcpp::regex::max_alternatives` alternatives, its
  quantifiers counted, which ECMA-262 allows, so that its compilation fits
  the 64 KiB default stack of wasm and emscripten.
```

  `doc/formats.adoc` includes every section in Global Constraints' order:
  `introduction`, `dates`, `identifiers`, `addresses`, `uris`, `templates`,
  `regex`, `composing`, `errors`, `allocation`, `suite`, `guarantees`,
  `testing`, the reference, `deviations`.

  The listings join their sections, each a compiled example:
  `doc/introduction.adoc` after its table:

```
A first program, which checks a string of a few formats:

[source]
----
include::{examples}/formats_quick_start.cpp[tag=program,indent=0]
----

It prints:

[source]
----
include::{examples}/formats_quick_start.expected[]
----
```

  `doc/composing.adoc` after its first paragraph, the same form with
  `formats_grammar.cpp[tag=host_port,indent=0]` and
  `formats_grammar.expected`, introduced by "An internationalized host name
  and an optional port; a rule that refuses says whose rule it was, through
  its error's category:"; `doc/uris.adoc` after its storage paragraph, with
  `formats_iri.cpp[tag=resource,indent=0]` and `formats_iri.expected`,
  introduced by "A resource of a program's own, here a buffer that refuses
  to grow:"; `doc/regex.adoc` after its first paragraph, with
  `formats_regex.cpp[tag=program,indent=0]` and `formats_regex.expected`.

  `doc/testing.adoc`'s first paragraph names `libs/formats/example` beside
  the tests in each command, and its second sentence becomes: "Its
  {n-tests} tests are unit tests with Boost's lightweight_test, and the
  JSON-Schema-Test-Suite's {n-suite} string tests; its {n-examples} examples
  run on every target too, each compared with the output the page shows."

  The comments that cite a guarantee: `date_time_test.cpp`'s `check` gains
  "(doc: #formats-invariant-1)" in its comment; `detail_test.cpp`'s
  `ill_formed_sequences_are_refused` gains a comment line
  "// (doc: #formats-invariant-4)"; `grammar_test.cpp`'s `fails_whole`
  "(doc: #formats-invariant-5)"; `allocation_test.cpp`'s first comment
  "(doc: #formats-invariant-2)".

- [ ] **Step 7: The README.** It gains, after its first paragraph:

```markdown
## A minimal example

This is [example/formats_quick_start.cpp](example/formats_quick_start.cpp):

<!-- include::example/formats_quick_start.cpp[tag=program] -->
```

  then a ```` ```cpp ```` block holding exactly the region `program` of
  `example/formats_quick_start.cpp` as the doc build's include gives it,
  then "It prints:", `<!-- include::example/formats_quick_start.expected[] -->`
  and a plain block holding `formats_quick_start.expected`. Then a paragraph:
  "`<webcpp/formats.hpp>` includes every format but `regex`, whose header
  `<webcpp/formats/regex.hpp>` brings webcpp::regex. formats needs Boost's
  headers, Boost.URL's compiled part and webcpp::idna, which
  `/webcpp/formats//formats` brings, and webcpp::regex for `regex`. It runs
  natively, on wasm32-wasip2 and wasm32-wasip3, and on emscripten."

- [ ] **Step 8: Build the page.**
  `B2 -a --build-dir=bin/formats-ready-doc libs/formats/doc`. Expected: exit
  0; doc-check finds every example shown, every `(doc: #formats-invariant-N)`
  defined, every `@see` title a section, the README's block equal to its
  region, and the link into idna's page defined there. Then the root page:
  `B2 -a --build-dir=bin/formats-ready-doc doc`, exit 0, its table listing
  formats as "original".

- [ ] **Step 9: CI's plan, actionlint and idna's lanes.**

```bash
python3 tools/ci/matrix.py plan --library formats
python3 tools/ci/matrix.py own-lanes --library formats
```

  Expected: the plan holds formats in every native lane, `wasip2`, `wasip3`
  and `emscripten`; the own lanes are `[]`. Run actionlint 1.7.12, as the
  CI downloads it, on `libs/formats/.github/workflows/ci.yml`: no finding.
  Run idna's native lane, `B2 -a --build-dir=bin/formats-ready-idna libs/idna/test`:
  green, as before formats.

- [ ] **Step 10: Every lane from scratch, and the lint.** The five lanes with
  `<t>` = `ready`, each naming `libs/formats/test libs/formats/example`:
  every test and example passed on every target, `suite` among them on all
  four. `LINT`: `lint: clean`.

- [ ] **Step 11: Commit** in `~/dev/webcpp-formats/libs/formats`:

```bash
git add example/Jamfile example/.clang-tidy example/formats_quick_start.cpp \
  example/formats_quick_start.expected example/formats_grammar.cpp example/formats_grammar.expected \
  example/formats_iri.cpp example/formats_iri.expected example/formats_regex.cpp \
  example/formats_regex.expected doc/counts.py doc/allocation.adoc doc/suite.adoc \
  doc/guarantees.adoc doc/deviations.adoc doc/formats.adoc doc/introduction.adoc doc/uris.adoc \
  doc/regex.adoc doc/composing.adoc doc/testing.adoc test/Jamfile test/suite_test.cpp \
  test/allocation_test.cpp test/date_time_test.cpp test/detail_test.cpp test/grammar_test.cpp \
  README.md
git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "formats: the suite on every target, the examples, and the page with its conformance table and guarantees"
```

The report's Changelog: the "New Libraries" line, final.

### Task 11: Review and publication (controller only)

- [ ] **Step 1: The whole-library review** on the most capable model: the
  library's `main` in `~/dev/webcpp-formats/libs/formats` against the spec
  and this plan, every lane from scratch in `~/dev/webcpp-formats`, the
  page, the lint, the counts, the five lines of Review Focus each pinned by
  its test, and the section "Decisions this plan makes for the owner" each
  applied as stated.
- [ ] **Step 2: Ask the owner** before anything leaves the machine, with the
  report: the lanes, the suite's 3,540 string tests, the measured size and
  stack, the hostile-pattern outcome, the decisions section, and the infra
  track's upstream draft
  `~/.buzz/OUTBOX/BOOST_URL_RELATIVE_REFERENCE_ISSUE.md`, with what Task 5
  found it lacks, if anything.
- [ ] **Step 3: Publish, once the owner says yes**, and once infra has
  integrated every shared item into `~/dev/webcpp` `main` and idna is
  published or published with it (formats' `build.jam` names
  `/webcpp/idna//idna`). The owner creates `webcpporg/formats`; the controller
  pushes `~/dev/webcpp-formats/libs/formats`'s `main` to it; then, in
  `~/dev/webcpp` (the one moment this plan writes there), on a branch,
  `git submodule add ../formats.git libs/formats`, `.gitmodules` and
  `libs/formats` staged by path, merged as the owner directs; CI watched on
  both repositories until green, the library's first run rerun after the
  superproject's lands.
- [ ] **Step 4:** Remove the task worktrees left; `~/dev/webcpp-formats` stays
  until idna's track is done with it too. Ledger the library as published.

## Review disposition

The independent review of 2026-10-09
(`~/dev/webcpp/.superpowers/sdd/plan-reviews/WEBCPP_FORMATS_PLAN-review.md`):
3 Critical, 12 Important, 12 Minor: 26 applied, none declined, and M12 reported to the controller (not this plan's to change).

| Finding | Disposition |
| --- | --- |
| C1 library in the main checkout | Applied (ruling): `~/dev/webcpp-formats/libs/formats` in the shared worktree `~/dev/webcpp-formats`; Goal, Global Constraints, File structure, Tasks 0, 1, 10, 11 |
| C2 shared items and bases | Applied (ruling): "Shared items and the bases" in Global Constraints, rebase onto `infra/shared`, local clones, named bases `B0` to `B-S3`, S1 before idna's generated Jamfiles; every task's base line |
| C3 idna's error lost by `variant_rule` | Applied (ruling): `detail::parse_mailbox` dispatches the domain on `[`; `mailbox_rule`, `idn_mailbox_rule` and `mailbox_error` gone; `email_test` asserts idna's category; Doc Comments and page corrected |
| I1 suite target and define | Applied (ruling): `/webcpp/third_party//json-schema-test-suite`, `WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE`, `[ webcpp.data $(KNOWN_FAILURES) ]` in Task 10 with its exact first failure |
| I2 known-failure `file` | Applied (ruling): the path under the dialect's directory, in the runner and the planted entries |
| I3 Task 1's first failure | Applied: the `webcpp.headers-alone` message and status 1 |
| I4 tests' `.clang-tidy` | Applied: xactor's file, both settings |
| I5 `\0380` | Applied: `"...d1\0" "380"`, and a Global Constraints line on it |
| I6 merge protocol | Applied: `build.jam` order and comment, `AGENTS.md` paragraphs, `pending` as `std::to_array`, the lanes rerun in the task worktree after its rebase |
| I7 combinators, decisions section | Applied (ruling): the template expression and the host names' labels on Boost.URL's combinators; section "Decisions this plan makes for the owner" |
| I8 upstream issue drafted twice | Applied (ruling): Task 5 reads infra's `BOOST_URL_RELATIVE_REFERENCE_ISSUE.md` and reports what it lacks; no draft of formats' own |
| I9 markup the doc build refuses | Applied: character lists in words in the composing table and both Doc Comments; a Global Constraints line |
| I10 SRELL allocation proof blind to `malloc` | Applied: the resource's count pinned to the spike's 23 for `^[a-z]+$` (or the count measured, with its reason); malloc interposition not added, since the pinned count already falls when an allocation escapes |
| I11 hostile regex shapes | Applied: 100,000 alternatives, 10,000 nested quantified groups and lookaheads beside nested groups; measured on wasip2, wasip3 and emscripten; a bound per trapping shape |
| I12 guards never red | Applied: planted defects for the uri guard (Task 5), `is_bidi_format` and `in_query` (Task 7), `has_srell_anchor` (Task 6), the leap-second offset (Task 2) |
| M1 S label in a tracked file | Applied: the Jamfile comments name `webcpp.data` |
| M2 planted include in Task 1 | Applied: removed from Task 1; Task 6 keeps it, where it compiles |
| M3 MSVC | Applied: Task 1's Interfaces and Task 5's AGENTS.md paragraph |
| M4 probe commands | Applied: Boost's include directory read from `.local/user-config.jam`; Task 10 builds its own archive |
| M5 `idn_lengths` | Applied: 55 valid, 56 invalid, the measuring note gone |
| M6 341 and 342 octets | Applied: both, of non-ASCII octets alone |
| M7 `grammar_test` coverage | Applied: `hostname_rule`, `iri_reference_rule`, `time_rule_t{leap_seconds::refused}` in combinators; `range_rule` through `ref` |
| M8 Review Focus 5 wording | Applied: cites the ruling and the suite fact |
| M9 idna anchor | Applied: `idna-introduction`, no hedge; idna's plan is asked to write it in I0 |
| M10 side-by-side verdicts | Applied: a table in `addresses.adoc` (Task 9), and its two missing verdicts in `hostname_test` |
| M11 SRELL target text | Applied: the system include and `SRELL_NO_THROW` in Task 6's base line and AGENTS.md |
| M12 cross-track notes | Not this plan's to change: reported to the controller (json_schema's `<webcpp-dir>` against infra's `webcpp.data`; v1's `time` policy unreachable from json_schema's default formats) |
