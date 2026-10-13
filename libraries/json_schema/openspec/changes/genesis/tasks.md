# Tasks

Each task points to its section in `reference/plan.md`, which holds its files, interfaces,
steps, code and expected output. Group 0 is new with this proposal; its detail is in
`reference/STATUS.md`. A checked task is done in `reference/source/`.

## 0. Bring the snapshot up to the superproject's current rules

- [ ] 0.1 Recreate the library's repository from `reference/source/` as one import commit, in a superproject worktree at `libs/json_schema` (STATUS.md, "How to resume", steps 1 to 3)
- [ ] 0.2 `config.hpp` includes `<webcpp/config.hpp>` and defines `WEBCPP_JSON_SCHEMA_NO_EXCEPTIONS` when `WEBCPP_HAS_EXCEPTIONS` is not defined; its Doc Comment says what `WEBCPP_NO_EXCEPTIONS` does, as `libs/idna`'s does
- [ ] 0.3 `build.jam` adds `<library>/webcpp/config//config` to the alias, as `libs/document` and `libs/idna` do
- [ ] 0.4 `test/Jamfile` builds `config_test.cpp` three times: `config`, `config_all_off` (with `WEBCPP_NO_EXCEPTIONS`) and `config_developer_defined`; the test checks each case
- [ ] 0.5 Add `doc/changelog.adoc` with the "New Libraries" line; check `meta/libraries.json`'s maintainer against the superproject's chapter 2 (this snapshot already carries the address chapter 11 names)
- [ ] 0.6 Update the plan's shell functions to the superproject of today: `SUPERPROJECT_BASE=main`, no srell submodule in `superproject_worktree`, the commit identity of chapter 11
- [ ] 0.7 Re-read the superproject's `AGENTS.md` against the snapshot (lint rules, `webcpp.jam` rules, headers-alone, Doc Comments) and fix what differs; run the lint
- [ ] 0.8 Verify: every snapshot test green on the five lanes from scratch once task 1.4 is redone (STATUS.md, "What to verify first")
- [x] 0.9 The plan's Task 0: the canonical repository, the E1 worktree, the licences re-checked (plan: Task 0), done 2026-10-09

## 1. E1: runner, IR, loader, core keywords (2020-12)

- [x] 1.1 The skeleton, the configuration, the errors and the memory adapters (plan: Task E1.1)
- [x] 1.2 Exact numbers: the number concept, decimal traits, exact comparison and `multipleOf` (plan: Task E1.2)
- [x] 1.3 JSON equality, iterative copies into an arena, JSON Pointers and UTF-8 (plan: Task E1.3)
- [ ] 1.4 The regex policy over Boost.Regex through webcpp::lexical's ECMA-262 translator (plan: Task E1.4). REDO: done once over webcpp::regex (kept in `reference/source/` as history); redone on Boost.Regex after the owner abandoned SRELL and webcpp::regex on 2026-10-12; waits for lexical's public matching interface (lexical's change `strategies`, group 3)
- [x] 1.5 Errors, annotations, the evaluation events, the record and the two sinks (plan: Task E1.5)
- [x] 1.6 Traits, the dialect tags, and JSON text read exactly with its depth bounded (plan: Task E1.6)
- [x] 1.7 The IR: one copy of each value, pointers as steps, and the check of its invariants (plan: Task E1.7)
- [x] 1.8 The compile and validate options, the message hook, and the data types of later features (plan: Task E1.8)
- [x] 1.9 The 2020-12 loader, iterative over one copy of the schema (plan: Task E1.9)
- [ ] 1.10 The compiler: the plan, its lowering and its text (plan: Task E1.10). IN PROGRESS: written and passing natively and on wasip2, uncommitted; the files are in `reference/source-wip/`, whose README says what remains
- [ ] 1.11 The machine and `is_valid`: every E1 instruction's verdict (plan: Task E1.11)
- [ ] 1.12 `validate`, the outcome, at most two runs, lifetimes and threads (plan: Task E1.12)
- [ ] 1.13 The English messages and the four output formats (plan: Task E1.13)
- [ ] 1.14 The suite runner and its known failures (plan: Task E1.14)
- [ ] 1.15 The measured limits, allocation, hostile input (plan: Task E1.15)
- [ ] 1.16 Doc Comments, the reference, the page and its first examples (plan: Task E1.16)
- [ ] 1.17 E1 complete: review, the freeze of the contract, tag `contract-1` (plan: Task E1.17)

## 2. E2: the compiler and the VM completed; references; annotations

- [ ] 2.1 The URL policy over Boost.URL (plan: Task E2.1)
- [ ] 2.2 The bundled meta-schemas, generated and checked (plan: Task E2.2)
- [ ] 2.3 Resources, anchors, the registry, the resolver, and `$ref` (plan: Task E2.3)
- [ ] 2.4 `$dynamicRef`, the dynamic scope, and cycles refused (plan: Task E2.4)
- [ ] 2.5 Meta-schema validation at compile, and the registry's cache (plan: Task E2.5)
- [ ] 2.6 `$vocabulary` and custom meta-schemas (plan: Task E2.6)
- [ ] 2.7 Compatibility keywords, and what an unknown keyword becomes (plan: Task E2.7)
- [ ] 2.8 Annotations (plan: Task E2.8)
- [ ] 2.9 `unevaluated*` with marks, checkpoint and rewind (plan: Task E2.9)
- [ ] 2.10 The measured instance depth (plan: Task E2.10)
- [ ] 2.11 E2 complete: 2020-12 whole, the reasons cleaned, the page, the merge (plan: Task E2.11)

## 3. E3: the message catalogue, the Boost.Locale header, and the proofs of output

- [ ] 3.1 The `.pot` catalogue, printed from the table (plan: Task E3.1)
- [ ] 3.2 The Boost.Locale header (plan: Task E3.2)
- [ ] 3.3 Every report of the suite checked against the official output schema (plan: Task E3.3)
- [ ] 3.4 A golden per message kind, the page, the merge (plan: Task E3.4)

## 4. E4: the emitter

- [ ] 4.1 `ir::equivalent` (plan: Task E4.1)
- [ ] 4.2 The emitter, 2020-12 (plan: Task E4.2)
- [ ] 4.3 Every dialect's forms, and OpenAPI 3.0's emit side (plan: Task E4.3)
- [ ] 4.4 E4 complete: the round trip, the page, the merge (plan: Task E4.4)

## 5. E5: every other dialect, formats, vocabularies, the proofs and the page

- [ ] 5.0a The machinery of 2019-09, draft 7 and draft 6 (plan: Task E5.0a)
- [ ] 5.0b Draft 4, OpenAPI 3.0, the six-dialect `validator`, and every runner (plan: Task E5.0b)
- [ ] 5.1 2019-09 whole: `$recursiveRef` at run time (plan: Task E5.1)
- [ ] 5.2 Drafts 7 and 6 whole (plan: Task E5.2)
- [ ] 5.3 Draft 4 whole (plan: Task E5.3)
- [ ] 5.4 OpenAPI 3.0 whole, and its page (plan: Task E5.4)
- [ ] 5.5 Formats through `webcpp::lexical`, `default_formats()` and `format_entry::check_in` (plan: Task E5.5)
- [ ] 5.6 User vocabularies (plan: Task E5.6)
- [ ] 5.7 The fuzz lane (plan: Task E5.7)
- [ ] 5.8 The Bowtie harness and its lane (plan: Task E5.8)
- [ ] 5.9 Content (plan: Task E5.9)
- [ ] 5.10 Allocation, over every dialect, formats and patterns, with Boost.Regex's stated exception (plan: Task E5.10)
- [ ] 5.11 Round trips for every dialect, annotations and output of 2019-09 (plan: Task E5.11)
- [ ] 5.12 Two plans, `explain()`, and `enum` as a hash table (plan: Task E5.12)
- [ ] 5.13 The flag plan's optimizations (plan: Task E5.13)
- [ ] 5.14 Inlined references and the static `unevaluated*` filter (plan: Task E5.14)
- [ ] 5.15 The benchmark's programs (plan: Task E5.15)
- [ ] 5.16 The benchmark's driver, after the owner's pins (plan: Task E5.16)
- [ ] 5.17 The benchmark's page, generated (plan: Task E5.17)
- [ ] 5.18 Conformance: the reasons closed, the counts, the known failures on the page (plan: Task E5.18)
- [ ] 5.19 The remaining sections and their examples (plan: Task E5.19)
- [ ] 5.20 E5 complete: readiness, the superproject's files, the merge (plan: Task E5.20)

## 6. Publication

- [ ] 6.1 The whole review, then, with the owner's go, `webcpporg/json_schema` created by the owner and the submodule added (plan: Task P)
