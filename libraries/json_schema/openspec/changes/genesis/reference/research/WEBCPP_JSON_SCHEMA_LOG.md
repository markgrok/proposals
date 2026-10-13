---
title: "webcpp::json_schema: ledger"
tags: [webcpp, json-schema]
status: active
created: 2026-10-09
---

# webcpp::json_schema: ledger

Plan: ~/.buzz/PLANS/WEBCPP_JSON_SCHEMA_PLAN.md
Spec: ~/.buzz/PLANS/WEBCPP_JSON_SCHEMA_SPEC.md

## Task 0: the repository, the E1 worktree, the licences (2026-10-09)

### infra/shared

- `infra/shared` is at `13e6e22` (third_party: SRELL from the mirror
  webcpporg/srell, as /webcpp/third_party//srell).
- `SUPERPROJECT_BASE=infra/shared`.
- S2 (`third_party/libs/JSON-Schema-Test-Suite`, the suite submodule) is at
  `5b0ee1613e45fcc2bddac00e07c19cd49b00d8a8`.
- S3 (`third_party/libs/srell`, the SRELL mirror submodule) is at
  `0b95517ee7be2d7ffb5abc9aa36f3f74d3621590`. The mirror
  (`~/.buzz/REPOS/srell.git`) carries five commits, not the three the plan
  text expected: `2c7e55b` SRELL 2026.07 imported from its release archive,
  `f722d3e` compile without exceptions, `67716ec` a moved-from regex copies
  and compiles a pattern again, `a8ecf39` every allocation through a
  memory_resource given at construction, `0b95517` README: the patches this
  mirror carries over SRELL 2026.07. Recorded as told; the plan's "three
  commits" line is out of date.
- The two aliases (`alias json-schema-test-suite`,
  `alias srell`) are present in `third_party/Jamfile` (lines 47 and 75).
- `rule data ( directories + )` is present in `tools/webcpp.jam` (line 334),
  so S5 (`webcpp.data`) is already on `infra/shared`; E1.14 does not need to
  wait for it.
- `test ! -e ~/dev/webcpp/libs/json_schema && echo absent` printed `absent`,
  as expected.

### Pending decision for E1.4

json_schema will use a new facade library, `webcpp::regex`, instead of
SRELL directly (the owner's decision, 2026-10-09). This does not change
Task 0's steps or outputs; it changes what E1.4 (regex) builds on. Recorded
here as pending for E1.4: E1.4 and every later regex use wait for
`webcpp::regex` and the spec amendment that replaces SRELL-direct with it.

### Licence findings (re-checked 2026-10-09, `gh api`, public metadata only)

- json-schema-spec: README's `## License` section reads "The contents of
  this repository are licensed under either the BSD 3-clause license *or*
  the Academic Free License v3.0."; `LICENSE` opens "Copyright (c) 2022 JSON
  Schema Specification Authors". Matches the plan's expectation.
- OpenAPI 3.0 schema (OAI/OpenAPI-Specification): SPDX `Apache-2.0`.
  Matches.
- `sourcemeta-research/jsonschema-benchmark`: head commit
  `724607bccb6bf0c5d3e8eebbe4b1a5aef75f3bd8`; `license` endpoint returns
  `404 Not Found`. Matches.
- `sourcemeta/blaze` (Blaze): the `license` endpoint's `spdx_id` is
  `NOASSERTION`, not `AGPL-3.0` as the plan's text states. GitHub's licence
  detector did not resolve a known SPDX id for the repository; this differs
  from the plan's expectation and from the Global Constraints' own
  characterization ("Blaze is AGPL, run and never linked or copied (D7)").
  Recorded as found; not re-verified against the repository's own licence
  file in this task. Flagged for whoever starts E5.8/E5.15 to confirm
  before relying on it.
- `ajv-validator/ajv`: SPDX `MIT`. Matches.
- `bowtie-json-schema/bowtie`: SPDX `NOASSERTION` (the plan only expected
  "whatever it is"; recorded as found, no discrepancy).

All eight `gh api` calls succeeded (none were denied by the permission
system), so none are recorded as "not checked".

### The repository and the E1 worktree

- `git init -b main ~/dev/webcpp-repos/json_schema`: created empty, as
  expected ("Initialized empty Git repository in
  .../webcpp-repos/json_schema/.git/").
- `track_worktree 1` created `~/dev/webcpp-json-schema-e1` (superproject
  worktree, branch `lib/json_schema-e1`, from `infra/shared`), with
  `libs/json_schema` cloned from the canonical repository and checked out
  on branch `e1` (unborn, no commits yet - the canonical repository is
  empty). Git printed "You appear to have cloned an empty repository." and
  "Switched to a new branch 'e1'", as expected.
- The submodule status lines for `third_party/libs/JSON-Schema-Test-Suite`
  and `third_party/libs/srell` carry neither `-` nor `+`. Matches.
- `libs/pratt` (a submodule of the superproject, not of json_schema) was
  empty and was initialized with
  `git submodule update --init libs/pratt` before the build, as the plan's
  note anticipates.
- `$B2 -a --build-dir=bin/check libs/pratt/test` built and ran pratt's test
  suite from scratch inside the e1 worktree: all targets passed
  ("...updated 95 targets..."), so the worktree builds. Matches.

### Worktree

- Path: `~/dev/webcpp-json-schema-e1`
- Superproject branch: `lib/json_schema-e1` (from `infra/shared`)
- `libs/json_schema` branch: `e1`, remote `origin` -> canonical repository
  only
- Canonical repository: `~/dev/webcpp-repos/json_schema` (empty, `main`)

### Changelog rule

No file under `libs/json_schema` holds a Changelog yet; the canonical
repository has no commits. The rule: Changelog lines for json_schema wait
in this ledger until a task adds them to a tracked `Changelog` file inside
`libs/json_schema`, in the Boost release-notes style used elsewhere in
webcpp; nothing is written to a generated or auto-maintained file by hand.

### Status

Task 0 complete. Every Expected matched what was found, except the S3
commit count (five, not three - recorded above) and the Blaze SPDX id
(`NOASSERTION`, not `AGPL-3.0` - recorded above, flagged for later
confirmation).
