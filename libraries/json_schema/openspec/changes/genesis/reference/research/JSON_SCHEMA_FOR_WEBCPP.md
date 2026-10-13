---
title: "JSON Schema for webcpp"
tags: [webcpp, json-schema, research]
status: superseded
created: 2026-10-08
---

> **Superseded on 2026-10-12.** The owner abandoned SRELL, webcpp::regex, webcpp::json_schema, webcpp::formats, webcpp::idna and webcpp::zod (PLANS/WEBCPP_DESIGN.md, 2026-10-12). Kept for the record only.

# JSON Schema for webcpp

Research input for `webcpp::json_schema`: a header-only C++20 library of
webcpp, on Boost 1.92 and Boost.JSON, built with b2 for native, wasip2, wasip3
and later emscripten; numbers as templates with Boost.Decimal `decimal64_t` as
the default; exceptions optional (`boost::throw_exception`); pmr allocators; no
global singletons; and a dialect-neutral IR, loader, emitter, format
validators and validator that a later C++ zod port reuses
(`~/.buzz/RESEARCH/ZOD_FOR_WEBCPP.md`, section 7, records what zod needs from
JSON Schema: `toJSONSchema` targets 2020-12, draft-07, draft-04 and OpenAPI 3.0,
and `fromJSONSchema`).

Sources were cloned shallowly on 2026-10-08 into
`~/.claude/jobs/fcf3a902/tmp/jsonschema-research/` (called `$R`
below):

| Clone | Commit | Date |
| --- | --- | --- |
| `JSON-Schema-Test-Suite` (https://github.com/json-schema-org/JSON-Schema-Test-Suite) | `5b0ee1613e45fcc2bddac00e07c19cd49b00d8a8` | 2026-09-21 |
| `json-schema-spec` (https://github.com/json-schema-org/json-schema-spec) | `4f56a9900674b27804f0ec32e3b7fdfa4efad695` | 2026-09-04 |
| `meta/` | the released meta-schemas and specs fetched from https://json-schema.org (`draft-04/schema`, `draft-06/schema`, `draft-07/schema`, `draft/2019-09/schema`, `draft/2020-12/schema`, their `meta/*` vocabularies and `output/schema`) | 2026-10-08 |

The C++ validators, Bowtie, formats and regex sections cite their own clones
in place.

## Summary

- **Target 2020-12, design for v1.** 2020-12 is the latest release; v1/2026
  is unpublished on 2026-10-08 but its spec and `tests/v1/` exist. v1 makes
  `format` an assertion, refuses unknown keywords except `x-`, drops
  `$vocabulary` and moves output to a separate document (section 1).
- **One IR, many loaders.** Every dialect (4, 6, 7, 2019-09, 2020-12, v1)
  maps onto one dialect-neutral IR with the dialect chosen per schema
  resource; the official meta-schemas ship inside the library (section 1.4).
- **Exact numbers are mandatory, not optional.** Required `multipleOf`
  tests break doubles, and `decimal64_t`'s `fmod` returns 0 for
  `1e308 multipleOf 0.123456789`, a required test; `multipleOf` must be
  exact on (coefficient, exponent), and schema numbers must come from the
  lexeme (section 2.4).
- **Annotations drive `unevaluated*`** (200 required tests in 2020-12);
  copy Blaze's static analysis with runtime marks only where needed
  (sections 2.2, 6).
- **No C++ library offers detailed or verbose output**; one evaluation
  event stream feeding renderers gives webcpp all formats (section 2.3).
- **Boost.Regex is not ECMA-262** (its `ECMAScript` flag is `perl`), and it
  aborts under `BOOST_NO_EXCEPTIONS` when its backtracking limit is hit;
  `\p{Letter}` is now in the required suite. An own header-only ECMA-262
  engine behind a policy is the recommendation (section 5).
- **Formats are hand-written grammars**: Boost.URL is compiled and has a
  `parse_uri_reference` bug and no IRI; Asio's address parsers do not build
  for wasip2; IDNA needs generated tables even with ICU (section 4).
- **Blaze is the reference design** (100% Bowtie on every draft, compiled
  instruction lists) but AGPL: ideas only (section 6).
- **Bowtie and jsonschema-benchmark** are the two ways to be measured
  against Blaze and ajv (sections 3.6, 7).

## 1. The drafts

### 1.1 Which releases exist, October 2026

- Released: draft-03, draft-04, draft-06, draft-07, 2019-09, 2020-12.
  https://json-schema.org/specification still lists 2020-12 as the current
  release with its vocabulary meta-schemas (`meta/core`, `meta/applicator`,
  `meta/unevaluated`, `meta/validation`, `meta/meta-data`,
  `meta/format-annotation`, `meta/format-assertion`, `meta/content`) and
  `output/schema` (fetched 2026-10-08).
- Next: **v1/2026**, the first "stable" release, is still unpublished on
  2026-10-08: https://json-schema.org/v1/2026/core.html and
  https://json-schema.org/v1/2026 both answer 404 (curl, 2026-10-08). The
  process says releases are published "on or about the first of January
  each year" and identified by year, at `https://json-schema.org/v1/2026/`
  (`$R/json-schema-spec/PROCESS.md`, "Release and Version", lines 62-106). The
  work in progress lives on `main` of json-schema-spec: the meta-schema
  `$R/json-schema-spec/specs/meta/meta.schema.json` declares
  `"$schema": "https://json-schema.org/v1"` and
  `"$id": "https://json-schema.org/v1/2026"`, and the test suite already has
  `tests/v1/`, whose README says v1 is "not yet released"
  (`$R/JSON-Schema-Test-Suite/README.md`, "How to Implement a Test Runner").
  Treat v1 as the target to design for and 2020-12 as the target to ship.
- Draft-03 is excluded: the suite calls draft-03 and draft-04 "frozen"
  (`README.md`, "Coverage"), and zod does not emit it.

### 1.2 Meta-schemas and vocabularies

| Dialect | `$schema` URI | Vocabularies |
| --- | --- | --- |
| draft-04 | `http://json-schema.org/draft-04/schema#` | none; one meta-schema (`meta/draft-04_schema.json`) |
| draft-06 | `http://json-schema.org/draft-06/schema#` | none |
| draft-07 | `http://json-schema.org/draft-07/schema#` | none |
| 2019-09 | `https://json-schema.org/draft/2019-09/schema` | core, applicator, validation, meta-data, content: `true`; **format: `false`** (`meta/draft_2019-09_schema.json`, `$vocabulary`) |
| 2020-12 | `https://json-schema.org/draft/2020-12/schema` | core, applicator, **unevaluated**, validation, meta-data, **format-annotation**, content, all `true`; `format-assertion` exists as a separate, optional vocabulary (`meta/draft_2020-12_schema.json`; validation spec section 7.2.2) |
| v1 | `https://json-schema.org/v1` (meta-schema `$id` `https://json-schema.org/v1/2026`) | none: `$vocabulary` was extracted into a proposal (`specs/proposals/vocabularies.md`, "Current Status"; ADR `adr/2024-05-08-extract-unstable-keywords.md`); the meta-schema keeps `$vocabulary` and `propertyDependencies` only as `"$comment": "Proposed keyword"` (`specs/meta/meta.schema.json` lines 146-151) |

Keywords per 2020-12 vocabulary (fetched from `https://json-schema.org/draft/2020-12/meta/<vocab>`):

| Vocabulary | Keywords |
| --- | --- |
| core | `$id $schema $ref $anchor $dynamicRef $dynamicAnchor $vocabulary $comment $defs` |
| applicator | `prefixItems items contains additionalProperties properties patternProperties dependentSchemas propertyNames if then else allOf anyOf oneOf not` |
| unevaluated | `unevaluatedItems unevaluatedProperties` |
| validation | `type const enum multipleOf maximum exclusiveMaximum minimum exclusiveMinimum maxLength minLength pattern maxItems minItems uniqueItems maxContains minContains maxProperties minProperties required dependentRequired` |
| meta-data | `title description default deprecated readOnly writeOnly examples` |
| format-annotation / format-assertion | `format` |
| content | `contentEncoding contentMediaType contentSchema` |

In 2019-09, `$recursiveRef`/`$recursiveAnchor` sit in core in place of
`$dynamicRef`/`$dynamicAnchor`, `additionalItems` and the `unevaluated*` pair
sit in applicator, and `prefixItems` does not exist (from
`https://json-schema.org/draft/2019-09/meta/<vocab>`). The 2020-12 meta-schema
still lists `definitions`, `dependencies`, `$recursiveAnchor` and
`$recursiveRef` at its root as deprecated compatibility properties
(`meta/draft_2020-12_schema.json`, `properties`).

### 1.3 Keywords added or changed, draft by draft

Draft-04 to draft-06 (compare `meta/draft-04_schema.json` and `meta/draft-06_schema.json`):
- `id` becomes `$id`; `const`, `contains`, `propertyNames`, `examples` are new.
- `exclusiveMaximum`/`exclusiveMinimum` change from booleans modifying
  `maximum`/`minimum` (draft-04: `{"type": "boolean", "default": false}`) to
  numbers in their own right.
- `true` and `false` become valid schemas everywhere (`tests/draft6/boolean_schema.json`).
- Formats `uri-reference`, `uri-template`, `json-pointer` are added (`tests/draft6/optional/format/`).

Draft-06 to draft-07: `if`/`then`/`else`, `$comment`, `readOnly`, `writeOnly`,
`contentEncoding`, `contentMediaType`; formats `date`, `time`, `idn-email`,
`idn-hostname`, `iri`, `iri-reference`, `relative-json-pointer`, `regex`
(`meta/draft-07_schema.json`; `tests/draft7/optional/format/`).

Draft-07 to 2019-09:
- **`$ref` siblings.** In draft-04/06/07, `$ref` replaces the whole schema
  object: sibling keywords are ignored (`tests/draft7/ref.json`, case "ref
  overrides any sibling keywords"). From 2019-09, `$ref` is an applicator like
  `allOf` and siblings are evaluated with it (`tests/draft2019-09/ref.json`,
  "ref applies alongside sibling keywords").
- `$anchor` (plain-name fragments; earlier drafts spelled them `"$id": "#foo"`).
- `$defs` replaces `definitions` (the latter remains a compatibility keyword).
- `dependencies` splits into `dependentRequired` (array form) and `dependentSchemas` (schema form).
- `$recursiveRef`/`$recursiveAnchor` (extensible meta-schemas).
- `unevaluatedProperties`/`unevaluatedItems`, which need annotations.
- `maxContains`/`minContains`, `deprecated`, `contentSchema`.
- `$vocabulary` and dialects; formats `duration`, `uuid`.
- `format` becomes annotation-only by default (2019-09 meta-schema sets the
  format vocabulary to `false`).
- Output formats (flag, basic, detailed, verbose) appear in core section 10.
Test files added in `tests/draft2019-09/` against draft7: `anchor`, `content`,
`defs`, `dependentRequired`, `dependentSchemas`, `maxContains`, `minContains`,
`recursiveRef`, `unevaluatedItems`, `unevaluatedProperties`, `vocabulary`;
removed `definitions`, `dependencies`.

2019-09 to 2020-12:
- **`items`/`additionalItems` versus `prefixItems`.** The array form of
  `items` becomes `prefixItems`; the single-schema `items` takes the role of
  `additionalItems`, which is removed.
- **`$recursiveRef` versus `$dynamicRef`.** `$dynamicRef`/`$dynamicAnchor`
  replace the recursive pair; a `$dynamicRef` acts as `$ref` unless the
  initially resolved target carries a matching `$dynamicAnchor`, in which case
  the outermost schema resource in the dynamic scope with that anchor wins
  (2020-12 core 8.2.3.2; `tests/draft2020-12/dynamicRef.json`, 44 tests).
- `contains` now produces an annotation (the matched indices) that
  `unevaluatedItems` consumes.
- `format` splits into the `format-annotation` (default) and `format-assertion` vocabularies.
- `unevaluated*` moves to its own vocabulary.
- Regexes "SHOULD be built with the u flag" (2020-12 core 6.4).
Test files: `additionalItems` and `recursiveRef` go, `dynamicRef` and `prefixItems` come.

2020-12 to v1 (work in progress; `specs/jsonschema-core.md`, change log
"draft-bhutton-json-schema-next", lines 2440ff., and the ADRs):
- `format` becomes an assertion by default, and the format tests move from
  `optional/format/` to the required `tests/v1/format/` (ADR
  `adr/2024-11-2-assertion-format.md`, voted 5 for, 1 neutral, 1 against).
- Unknown keywords: "Implementations MUST refuse to evaluate schemas which
  contain keywords which they do not know how to process"
  (`jsonschema-core.md`, "Handling of unrecognized or unsupported keywords");
  keywords starting with `x-` are implicit annotations ("Implicit annotation keywords").
- IRIs instead of URIs throughout; `$id` must not carry a fragment; duplicate IRIs SHOULD raise an error.
- `$vocabulary`, output formats and `propertyDependencies` are extracted into
  proposals (ADR `2024-05-08-extract-unstable-keywords.md`); output moves to
  its own document `specs/output/jsonschema-validation-output-machines.md`
  with the formats `flag`, `list` and `hierarchical`.
- Dialect determination is prioritised: `$schema`, then the parent
  resource's dialect, then external context (media type `schema` parameter,
  embedding document), then user configuration; "If the dialect is not
  specified through one of these methods, the implementation MUST refuse to
  process the schema" (`jsonschema-core.md`, "Dialect Determination").
- Compatibility promise: a schema written for `v1/<year>` keeps its meaning
  in every later `v1` release (`jsonschema-core.md`, "Specification
  Versioning and Compatibility").
- `tests/v1/` drops `vocabulary.json`, `defs.json` and `format.json`, adds
  `format/` (required) and `proposals/propertyDependencies/`.

### 1.4 Supporting every dialect at once

What the suite and the specs ask of a multi-dialect validator:

1. **Dialect per schema resource, not per document.** Determine the dialect
   on entering each resource (`$schema`, else the enclosing resource's, else
   the configured default). `optional/cross-draft.json` in 2019-09 and
   2020-12 checks that "refs to historic drafts are processed as historic
   drafts": a 2020-12 schema `$ref`s a draft-07-style remote and must apply
   draft-07 `items` semantics there.
2. **A default dialect.** 2020-12 leaves a missing `$schema` at the root
   "implementation-defined" (core 8.1.1); the suite says that for draft-07
   and earlier, and for v1, "implementations MUST be configured to expect the
   version matching the test directory name", while for 2019-09 and later
   they "SHOULD" detect it from `$schema` (`README.md`, "How to Implement a
   Test Runner"). `optional/no-schema.json` checks validation without
   `$schema`. The default is therefore an option of the loader, never a
   global.
3. **`$vocabulary`.** Read only when a schema is processed as a meta-schema
   (2020-12 core 8.1.2). A required (`true`) unknown vocabulary makes the
   loader refuse the schema; an unknown optional one is ignored; a known
   vocabulary absent from the list switches its keywords off, which
   `tests/draft2020-12/vocabulary.json` checks with a custom meta-schema
   without the validation vocabulary ("no validation: invalid number, but it
   still validates": valid). Vocabularies are not inherited through `$ref`
   (8.1.2.2). The same mechanism turns `format` into an assertion
   (`optional/format-assertion.json`, using
   `remotes/draft2020-12/format-assertion-{true,false}.json`).
4. **Bundled meta-schemas.** Tests `$ref` the official meta-schemas by URI
   (454 occurrences of `https://json-schema.org/draft/2020-12/schema`, 444 of
   the 2019-09 one, 441 of `https://json-schema.org/v1` across `tests/`), so
   every dialect's meta-schema and vocabulary meta-schemas must ship inside
   the library as resources the loader resolves offline.
5. **Keyword tables per dialect.** The same name means different things:
   `items` (draft-04 to 2019-09 array or schema versus 2020-12 schema),
   `exclusiveMaximum` (boolean versus number), `dependencies` versus the split
   pair, `$ref` with or without siblings, `id` versus `$id`, `definitions`
   versus `$defs`. Map each dialect's keywords onto one dialect-neutral IR
   (for example a "tuple prefix + rest" array node, and a single
   "exclusive bound" representation) so that the validator has one
   evaluator, and keep the source dialect on the node only where the
   semantics truly differ (ref-with-siblings, contains-annotates).
6. **Unknown keywords.** Collected as annotations in 2019-09/2020-12;
   `optional/refOfUnknownKeyword.json` requires `$ref` into the inside of an
   unknown keyword to work ("reference of a root arbitrary keyword"), and
   `optional/unknownKeyword.json` that `$id` inside an unknown keyword is not
   an identifier. In v1 they are a load error unless they start with `x-`.
   The IR must therefore keep the raw JSON of the schema reachable by JSON
   Pointer.

## 2. Core semantics a full implementation needs

### 2.1 Identification and references

- **Base IRI.** `$id` (`id` in draft-04) sets the base of a schema resource;
  a relative `$id` resolves against the enclosing base; a subschema with
  `$id` is an *embedded resource* (2020-12 core 8.2.1, 9.2). The initial base
  is the retrieval URI, or one the user supplies (8.2.1.1; v1 "Initial Base IRI").
- **Fragments.** `#/a/b` is a JSON Pointer (RFC 6901), `#name` a plain-name
  anchor (`$anchor`, or `$id: "#name"` before 2019-09). A JSON Pointer
  fragment is relative to the resource, and a pointer that crosses into an
  embedded resource is the subject of 2020-12 core 9.2.1 (v1
  "JSON Pointer fragment identifiers and embedded schema resources"); the
  canonical IRI of a subschema is the nearest resource's IRI plus a pointer.
- **Registry.** Load-time walk of every document builds a map from canonical
  IRI to node, including every `$anchor`, `$dynamicAnchor` and pointer
  location; `$ref` is resolved after the whole set of documents is known.
  References are resolved lazily for remote documents through a
  user-supplied resolver (the suite's `remotes/` mapped at
  `http://localhost:1234/`, see 3.4). No global registry: the registry is an
  object the user owns, preloaded with the bundled meta-schemas.
- **Dynamic scope.** The stack of schema resources entered during
  evaluation (2020-12 core 7.1; v1 "Lexical Scope and Dynamic Scope").
  `$dynamicRef` resolves its fragment against the outermost resource on that
  stack that declares the matching `$dynamicAnchor`, but only when the
  statically resolved target itself has the `$dynamicAnchor` ("bookending";
  `tests/draft2020-12/dynamicRef.json`). `$recursiveRef` (2019-09) is the
  same idea with `$recursiveAnchor: true` and no names. A compiled validator
  can resolve a `$dynamicRef` statically when only one candidate exists, and
  otherwise needs the scope at run time.
- **Cycles.** Schemas may be recursive (`tree.json`, `$ref: "#"`);
  evaluation terminates because each step descends into the instance, but a
  `$ref` loop that never consumes the instance (`{"$defs": {"a": {"$ref":
  "#/$defs/b"}, "b": {"$ref": "#/$defs/a"}}}`) must be detected (2020-12 core 9.4.1, "Guarding Against Infinite
  Recursion"). The detection cannot be naive: the required
  `infinite-loop-detection.json` checks that "evaluating the same schema
  location against the same data location twice is not a sign of an
  infinite loop" (an `allOf` reaching one `$defs` entry twice). Detect it at
  load time instead, as a cycle of references that crosses no keyword
  descending into the instance. The compiled form
  represents a reference as an index into a table of compiled subschemas, so
  cycles cost nothing at compile time.

### 2.2 Annotations, which `unevaluated*` needs

An annotation is kept only if the schema object that produced it and every
parent passed; annotations of a failed branch (including everything under a
failing `anyOf` alternative, and everything under `not`) are dropped (2020-12
core 7.7.1; v1 "Collecting Annotations"). What each keyword annotates
(2020-12 core 10.3 and 11; validation spec 7-9):

| Keyword | Annotation |
| --- | --- |
| `properties` | set of instance property names it validated |
| `patternProperties` | set of names matched by any pattern |
| `additionalProperties` | set of names it applied to |
| `unevaluatedProperties` | set of names it applied to |
| `prefixItems` | largest index applied to, or `true` if all |
| `items` (2020-12) | `true` if applied to any item |
| `items` array / `additionalItems` (2019-09 and earlier) | largest index / `true` |
| `contains` | 2020-12: array of matched indices (consumed by `unevaluatedItems`); 2019-09: none, and `unevaluatedItems` ignores `contains` |
| `unevaluatedItems` | `true` if applied to any |
| `title`, `description`, `default`, `deprecated`, `readOnly`, `writeOnly`, `examples` | the keyword's value |
| `format` (annotation vocabulary) | the format name |
| `contentEncoding`, `contentMediaType`, `contentSchema` | the value |
| unknown keywords | the value (SHOULD, 2020-12 core 6.5) |

`unevaluatedProperties` sees a property as evaluated if any adjacent keyword
(including through `$ref`, `$dynamicRef`, `allOf`, `anyOf`, `oneOf`, `if`/
`then`/`else`, `dependentSchemas` of passing branches, but not `not`)
annotated it at the same instance location. That forces two choices on a
validator: an `anyOf` cannot short-circuit when a later alternative might
annotate, and evaluation has to carry an "evaluated" set per instance
location. The suite tests this heavily: `unevaluatedProperties.json` 129 and
`unevaluatedItems.json` 71 required tests in 2020-12, and the separate
annotation suite `annotations/tests/*.json` (46 cases, 58 tests, format
`annotations/README.md`, with `compatibility` filters per dialect) checks
which annotation lands on which instance location.

Blaze's approach (static analysis to avoid tracking when the schema permits
it) is covered in section 6.

### 2.3 Output formats

2019-09 and 2020-12 define four (2020-12 core 12.2-12.4; schema
`https://json-schema.org/draft/2020-12/output/schema`, copy in
`meta/draft_2020-12_output_schema.json`):

- **flag**: `{"valid": false}`. Short-circuiting is RECOMMENDED.
- **basic**: a flat list of output units under `errors` (on failure) or
  `annotations` (on success).
- **detailed**: a hierarchy following the schema; nodes with no children
  are removed, a node with one child is replaced by it.
- **verbose**: the full hierarchy, every node with `valid`, including
  annotations of failed branches and passing results under `not`
  (example: https://json-schema.org/draft/2020-12/output/verbose-example).

An *output unit* has `valid` (boolean), `keywordLocation` (JSON Pointer
through the evaluation path, including `$ref`/`$dynamicRef` segments),
`absoluteKeywordLocation` (canonical IRI plus pointer, without by-reference
segments; required when the path crossed a `$ref` or `$dynamicRef`),
`instanceLocation` (JSON Pointer), and `error` or `annotation`, with nested
`errors` or `annotations` (12.3). A failing unit must carry `error` or
`errors` (output schema, `allOf[0]`). The basic example of 12.4.2 has
units such as
`{"keywordLocation": "/items/$ref/required", "absoluteKeywordLocation":
"https://example.com/polygon#/$defs/point/required", "instanceLocation":
"/1", "error": "Required property 'y' not found."}`.

v1's separate output document renames them `flag`, `list` (a root unit with
`valid` and `details`) and `hierarchical`, uses `evaluationPath` and
`schemaLocation` in place of `keywordLocation` and
`absoluteKeywordLocation`, and adds "dropped annotations" for debugging
(`specs/output/jsonschema-validation-output-machines.md`, "Output
Structure", "List"; `jsonschema-core.md`, "Output Formatting").

The suite's `output-tests/` (`draft2019-09`, `draft2020-12`, `v1`; 11 cases
of one test each, `output-tests/README.md`) check output content against the
`basic`/`list` shape and note the ambiguity of a single-unit `basic`.

Design consequence: produce one internal event stream (keyword evaluated,
locations, result, annotation or error id) and build each format from it, so
that flag stays allocation-free and short-circuiting, and the others are
renderers.

### 2.4 Number semantics

- The data model's number is "an arbitrary-precision, base-10 decimal number
  value" (2020-12 core 4.2.1); "integer" is "any number with a zero
  fractional part", so `1.0` is an integer (validation 6.1.1;
  `tests/draft2020-12/type.json`, "a float with zero fractional part is an
  integer"). Draft-04 is the exception: there `1.0` is *not* an integer
  (`tests/draft4/type.json`; `tests/draft4/optional/zeroTerminatedFloats.json`).
- Big numbers: `optional/bignum.json` (2020-12) validates 53-digit integers
  as `integer` and `number`, compares `maximum: 18446744073709551615`
  against `18446744073709551600`, and checks
  `exclusiveMaximum: 9.727837981879871e+26` against an equal value.
  `optional/float-overflow.json` expects `1e308` to be a multiple of `0.5`.
- `multipleOf` with decimals is in the required tests:
  `multipleOf.json` has `0.0075` multiple of `0.0001` (valid),
  `0.00751` (invalid), "float division = inf" (`1e308` with `multipleOf:
  0.123456789`: **invalid**) and "small multiple of large integer"
  (`12391239123` with `1e-08`: valid). Binary doubles fail the first group;
  naive decimal division fails the third.
- **Probe on Boost 1.92 `decimal64_t`** (`$R/probe/dec.cpp`, compiled with
  clang++ -std=c++20 against the Boost checkout in
  `.build/third_party/boost` of an earlier private project, `BOOST_LIB_VERSION "1_92"`):
  `fmod(0.0075, 0.0001) == 0`, `fmod(0.00751, 0.0001) == 1e-05`,
  `fmod(35, 1.5) == 0.5`, the bignum comparisons pass, but
  **`fmod(1e308, 0.123456789)` returns `0`** and the quotient `8.1e+308` looks
  integral, so a `multipleOf` written as `fmod(x, y) == 0` or as
  "quotient is an integer" fails a *required* test. `multipleOf` must be
  computed exactly on (coefficient, exponent): with `x = cx * 10^ex`,
  `y = cy * 10^ey`, `x` is a multiple of `y` iff `cx * 10^(ex-ey) mod cy == 0`
  when `ex >= ey` (modular exponentiation in 128-bit arithmetic), or `cx` is
  divisible by `cy * 10^(ey-ex)` otherwise.
- **Boost.JSON** stores numbers as `int64_t`, `uint64_t` or `double`
  (`boost/json/parse_options.hpp`: `number_precision::imprecise`, `precise`,
  `none`). The decimal value of a schema literal like `0.0001` is lost once
  it becomes a `double`. Two routes: (a) a `basic_parser` handler, which
  receives each number's text (`on_number_part`, `on_int64(..., string_view
  s, ...)`, `on_double(..., string_view s, ...)` in
  `boost/json/basic_parser.hpp` lines 175-202), to build decimals from the
  lexeme; (b) convert a `double` to the shortest round-trip decimal (the
  digits `to_chars` prints) and parse that into `decimal64_t`. Route (a) is
  exact and is what the loader should do for schemas; route (b) is enough
  for instances already held in a `boost::json::value` with
  `number_precision::precise`. Both give `decimal64_t` its 16 significant
  digits; beyond that a number rounds, which the bignum tests above tolerate
  (the 53-digit integers stay integral; the 20-digit bounds round to the
  same value and compare `<=`).

### 2.5 Strings and Unicode

- Length is the number of Unicode code points ("characters as defined by
  RFC 8259", validation 6.3.1), so a UTF-8 string is measured by counting
  non-continuation bytes; `maxLength.json`/`minLength.json` include
  "two graphemes is long enough" with a non-BMP character counting as one.
- Patterns are ECMA-262 regular expressions, never implicitly anchored, and
  SHOULD use the `u` flag (2020-12 core 6.4; v1 "Regular Expressions"
  restricts the recommended subset to literals, `.`, classes, quantifiers
  including lazy ones, `^`, `$`, groups and alternation). Section 5 covers
  what that means for Boost.Regex.
- `const`/`enum`/`uniqueItems` compare by JSON equality: numbers by
  mathematical value (`1` equals `1.0`), objects ignoring member order
  (`tests/draft2020-12/const.json`, `uniqueItems.json`).

## 3. The official test suite

### 3.1 Layout

`$R/JSON-Schema-Test-Suite/` (commit `5b0ee16`, 2026-09-21):

| Path | What |
| --- | --- |
| `tests/<dialect>/*.json` | required tests; dialects `draft3 draft4 draft6 draft7 draft2019-09 draft2020-12 v1`; `tests/latest -> draft2020-12` |
| `tests/<dialect>/optional/*.json` | optional groups (see 3.3) |
| `tests/<dialect>/optional/format/*.json` | `format` as an assertion, one file per format; in v1 this directory is `tests/v1/format/` and is required |
| `tests/v1/proposals/propertyDependencies/` | tests for the proposal (volatile) |
| `remotes/` | 79 schema files served as `http://localhost:1234/<path>` |
| `annotations/` | the dialect-neutral annotation suite (46 cases, 58 tests) |
| `output-tests/<dialect>/` | output-format tests and `output-schema.json` |
| `bin/jsonschema_suite` | Python helper: `check` (sanity checks), `flatten <version>`, `remotes` (prints one JSON object mapping each remote URL to its schema), `dump_remotes`, `serve` (a web server for the remotes) (`bin/jsonschema_suite` lines 687-730) |
| `test-schema.json` | the schema of a test file |
| `LICENSE` | MIT, "Copyright (c) 2012 Julian Berman" |

### 3.2 The shape of a case

A file is a JSON array of *test cases*; a case is
`{"description", "schema", "tests": [{"description", "data", "valid", "comment"?}], "comment"?, "specification"?}`
(`README.md`, "Sample Test Case"; `test-schema.json`). The suite guarantees
every `schema` is valid under its dialect (`README.md`, "Invariants &
Guarantees"); schema-validity tests are written as instances validated
against `{"$ref": "<meta-schema>"}`.

### 3.3 Counts (files, cases, tests) per dialect

Computed with a short Python script over `tests/` on commit `5b0ee16`:

| Dialect           | Required                                               | Optional (`optional/*.json`) | Format (`optional/format/`) |
| ----------------- | ------------------------------------------------------ | ---------------------------- | --------------------------- |
| draft4            | 30 / 160 / 618                                         | 6 / 32 / 100                 | 7 / 7 / 291                 |
| draft6            | 36 / 233 / 841                                         | 6 / 34 / 106                 | 10 / 10 / 409               |
| draft7            | 37 / 258 / 929                                         | 8 / 38 / 118                 | 19 / 26 / 793               |
| draft2019-09      | 46 / 373 / 1261                                        | 11 / 48 / 158                | 21 / 28 / 874               |
| draft2020-12      | 46 / 384 / 1301                                        | 13 / 50 / 162                | 21 / 28 / 874               |
| v1 (unreleased)   | 43 / 348 / 1135, plus required `format/` 20 / 27 / 868 | 11 / 65 / 287                | (moved to required)         |
| draft3 (excluded) | 25 / 105 / 437                                         | 3 / 10 / 22                  | 11 / 11 / 110               |

Total for the five target drafts: 4950 required tests, 644 optional and
3241 format tests; with v1, 6953 required.

The 2020-12 required tests by future increment (file-name grouping):
772 "core keywords" tests, 166 reference tests (`ref`, `refRemote`,
`dynamicRef`, `anchor`, `defs`, `infinite-loop-detection`), 200
`unevaluated*` tests and 163 vocabulary/annotation tests (`format` 133 as
annotation, `content` 18, `default` 7, `vocabulary` 5). Largest files:
`format` 133, `unevaluatedProperties` 129, `type` 80, `ref` 79,
`unevaluatedItems` 71, `uniqueItems` 69.

Optional groups per dialect (`tests/<dialect>/optional/`):

| Group | Dialects | What it needs |
| --- | --- | --- |
| `bignum` | 4 to 2020-12, v1 | arbitrary-size integers, high-precision comparison |
| `float-overflow` | 4 to 2020-12, v1 | `1e308` multiple of `0.5` |
| `zeroTerminatedFloats` | draft4 | `1.0` is not an integer in draft-04 |
| `ecmascript-regex` | 4 to 2020-12, v1 | ECMA-262 regex semantics in `pattern`/`patternProperties` |
| `non-bmp-regex` | 4 to 2020-12, v1 | regex over code points beyond U+FFFF |
| `id` | 4 to 2020-12, v1 | `$id` inside `enum`/`const` is not an identifier |
| `unknownKeyword` | 6 to 2020-12, v1 | `$id` inside an unknown keyword is not an identifier |
| `content` | draft7 | `contentMediaType`/`contentEncoding` as assertions |
| `cross-draft` | 7, 2019-09, 2020-12 | a `$ref` to another dialect's schema keeps that dialect |
| `anchor`, `dynamicRef` | 2019-09/2020-12, v1 | anchor and dynamic-ref edge cases |
| `dependencies-compatibility` | 2019-09, 2020-12, v1 | the old `dependencies` keyword still honoured |
| `no-schema` | 2019-09, 2020-12 | validation without `$schema` |
| `refOfUnknownKeyword` | 2019-09, 2020-12, v1 | `$ref` into an unknown keyword's value |
| `format-assertion` | 2020-12 | the format-assertion vocabulary through `$vocabulary` |
| `format-annotation` | v1 | `format` kept as annotation when asked |
| `format/*` | all | one file per format, format as assertion (counts in section 4) |

### 3.4 Remote references

The runner either loads `remotes/<path>` under the retrieval URI
`http://localhost:1234/<path>`, or reads `bin/jsonschema_suite remotes`,
which prints one JSON object of all of them (`README.md`, "Additional
Assumptions"). In 2020-12, `anchor`, `vocabulary`, `ref`, `refRemote`,
`dynamicRef` and the optional `unknownKeyword`, `format-assertion`,
`cross-draft`, `id` refer to them. A webcpp runner should preload them into
the user-owned registry through a resolver callback, never over the network,
so that the same runner works on wasip2/wasip3 without sockets.

### 3.5 Licence

MIT (`LICENSE`, "Copyright (c) 2012 Julian Berman"). The files can be
pinned and fetched by b2 like any third-party pin, or vendored with a
recorded decision.

### 3.6 Bowtie

Bowtie (https://github.com/bowtie-json-schema/bowtie, clone `$R/bowtie`,
commit `c581ad1`, 2026-10-01) runs every implementation in a container and
talks to it over IHOP v1, newline-delimited JSON on stdin/stdout, flushed per
line (`bowtie/docs/implementers.rst`; schemas
`bowtie/schemas/io/v1.json` and `bowtie/schemas/io/commands/{start,dialect,run,stop}.json`):

| Command | Request | Response |
| --- | --- | --- |
| `start` | `{"cmd": "start", "version": 1}` | `{"version": 1, "implementation": {"name", "language", "homepage", "issues", "source", "dialects": [...], "version"?, ...}}` (`schemas/models/implementation.json`) |
| `dialect` | `{"cmd": "dialect", "dialect": "<uri>"}`, the dialect for schemas without `$schema` | `{"ok": true}` or `false` ("cannot configure", not "unsupported") |
| `run` | `{"cmd": "run", "seq": <any>, "case": {"description", "schema", "registry"?: {uri: schema}, "tests": [...]}, "output": "flag" or "annotations"}` | `{"seq", "results": [{"valid": bool}, ...]}`, or `{"seq", "skipped": true, ...}`, or `{"seq", "errored": true, "context": {...}}` |
| `stop` | `{"cmd": "stop"}` | none; the harness exits |

The case's `registry` is the retrieval store for `$ref`, which is how Bowtie
delivers the suite's remotes. `"output": "annotations"` uses Bowtie's own
annotation format (`schemas/output/annotations.json`; `implementers.rst`
lines 459-462). Images are published as `ghcr.io/bowtie-json-schema/<id>`
(`bowtie/_containers.py:45`); harnesses are moving out of the repository
into their own repositories in the `bowtie-json-schema` organisation with
the topic `bowtie-harness` (`hatch_build.py` lines 60-112; only
`java-json-schema`, `js-hyperjump` and `python-jsonschema` remain in-tree).
C++ harnesses there: `cpp-blaze`, `cpp-jsoncons`, `cpp-valijson` and
`cpp-iris`. Bowtie already knows `v1` (`https://json-schema.org/v1`) as a
prerelease dialect (`data/dialects.json`).

**Joining Bowtie** takes a repository `bowtie-json-schema/cpp-<name>` with
an Alpine multi-stage Dockerfile and a small `bowtie_<name>.cpp` that reads
lines from stdin and answers; `cpp-blaze`'s `bowtie_blaze.cpp` is about 150
lines and a good template; `bowtie run -V` validates the replies. Nothing in
webcpp's design prevents it: the harness is a native program; the registry
maps onto webcpp's user-owned resource registry; `errored` maps onto an
error value. A wasip2 build could even run under wasmtime inside the image.

**Published compliance**, required suite only, computed from
`https://bowtie.report/<dialect>.json` (run of 2026-10-08, bowtie 2026.7.4;
the site is a JavaScript app but its data is plain JSON; the badge
endpoints `/badges/c++-<id>/compliance/<dialect>.json` agree):

| Implementation | 2020-12 (1301) | 2019-09 (1261) | 7 (929) | 6 (841) | 4 (618) |
| --- | --- | --- | --- | --- | --- |
| cpp-blaze (`5eeef87e`) | 100% | 100% | 100% | 100% | 100% |
| cpp-iris 0.1.0 | 100% | - | - | - | - |
| cpp-jsoncons 0.177.0 | 99.39% | 99.84% | 100% | 100% | 100% |
| cpp-valijson | - | - | 99.57% | - | 99.35% |
| rust-boon 0.6.1, rust-jsonschema 0.47.0 (references) | 100% | 100% | 100% | 100% | 100% |
| js-ajv 8.20.0 (reference) | 80.48% | 83.03% | 85.15% | 90.96% | 90.94% |

No `https://bowtie.report/v1.json` exists yet (404). pboettch's validator
has no harness.

## 4. Format validators

Sources: the subagent survey with probes in `$R/probe/` (Boost 1.92.0 headers
from Homebrew, `BOOST_VERSION 109200`; ICU 78.3; wasi-sdk 34 from
`~/dev/webcpp/.local/wasi-sdk`).

### 4.1 Formats per draft and their references

| Format | Since | Reference (2020-12 validation 7.3, https://json-schema.org/draft/2020-12/json-schema-validation) | Hard? |
| --- | --- | --- | --- |
| `date-time`, `date`, `time` | 4 (`date-time`), 7 | RFC 3339 5.6 `date-time`, `full-date`, `full-time`; v1: RFC 9557 `date-time-ext` (suffixes like `[Europe/Paris]`) | leap seconds checked by normalising to UTC 23:59:60 |
| `duration` | 2019-09 | RFC 3339 Appendix A (ISO 8601 ABNF) | strict ABNF: no fractions, no `P1Y2W` |
| `email` | 4 | RFC 5321 4.1.2 `Mailbox` (RFC 5322 before 2020-12) | moderate |
| `idn-email` | 7 | RFC 6531 3.3 | needs `idn-hostname` |
| `hostname` | 4 | RFC 1123 2.1, including Punycode A-labels (RFC 5891 4.4) | the suite decodes A-labels and applies IDNA2008 rules |
| `idn-hostname` | 7 | RFC 5890 2.3.2.3; v1 registry: "IDNA2008 using the UTS #46 mapping" | **hardest** |
| `ipv4`, `ipv6` | 4 | RFC 2673 3.2, RFC 4291 2.2 | easy; no zone ids, no prefixes |
| `uri`, `uri-reference` | 4, 6 | RFC 3986 | easy |
| `iri`, `iri-reference` | 7 | RFC 3987 | `ucschar`/`iprivate` ranges |
| `uuid` | 2019-09 | RFC 4122 (RFC 9562 obsoletes it) | easy |
| `uri-template` | 6 | RFC 6570 | moderate (`{v:10000}`, `{v:01}` invalid) |
| `json-pointer` | 6 | RFC 6901 5 | easy |
| `relative-json-pointer` | 7 | draft-handrews-relative-json-pointer | easy |
| `regex` | 7 | ECMA-262 | needs an ECMA-262 pattern parser (section 5) |

The per-draft lists come from the test directories
(`tests/draft4/optional/format/`: date-time, email, hostname, ipv4, ipv6,
uri; draft6 adds json-pointer, uri-reference, uri-template; draft7 adds date,
time, idn-email, idn-hostname, iri, iri-reference, relative-json-pointer,
regex; 2019-09 adds duration, uuid) and the change log of
`$R/json-schema-spec/specs/jsonschema-validation.md`. v1 keeps 17 formats in
the body (idn-hostname and idn-email move to the registry
`specs/registries/format.json`, with about 40 more names such as `int32`,
`decimal`, `base64url`), makes assertion the default, and says
implementations "MUST refuse to process any schema which contains a format
value it doesn't support" (`jsonschema-validation.md` lines 393-521).
v1's `time` "MUST NOT include leap seconds" while the v1 tests still accept
them: a conflict to report upstream.

Test counts per format (2020-12 `optional/format/`, v1 `format/`, which
are the same files with another `$schema`): date 81, date-time 43, duration
52, email 71, idn-email 24 (v1 25), hostname 64, idn-hostname 98, ipv4 41,
ipv6 51, uri 47, uri-reference 31, iri 26, iri-reference 21, uuid 29,
uri-template 47, json-pointer 40, relative-json-pointer 26, regex 8,
ecmascript-regex 12, unknown 7 (2020-12 only): 874 tests in 21 files (v1:
868 in 20).

**IDN in detail.** The `idn-hostname` tests mix UTS #46 mapping (fullwidth
digits mapped, zero-width space ignored, non-NFC U-label valid after
normalisation, U+3002/U+FF0E/U+FF61 as dots) with strict IDNA2008: RFC 5892
DISALLOWED exceptions (U+0640, U+07FA, U+302E, U+3031-3035, U+303B; `xn--7a`
decodes to U+00A1, DISALLOWED) and PVALID exceptions (ß, ς, U+0F0B, U+3007),
leading combining marks, CONTEXTO (middle dot, Greek keraia, Hebrew
geresh/gershayim, katakana middle dot, Arabic-Indic digit mixing), CONTEXTJ
(ZWJ/ZWNJ after a virama or by joining type), the Bidi rule (RFC 5893),
Punycode (RFC 3492) decode with re-encode round trip, and length limits on
the A-label (63 per label, 253 total). **ICU alone does not pass**:
`uidna_openUTS46` with STD3, CHECK_BIDI, CHECK_CONTEXTJ, CHECK_CONTEXTO and
NONTRANSITIONAL fails 8 of 92 idn-hostname and 6 of 58 hostname string
tests (trailing dot, DISALLOWED exceptions, non-ASCII hostname input)
(`$R/probe/idna_probe.cpp`). So a webcpp IDNA module needs generated tables
anyway: the UTS #46 mapping (`IdnaMappingTable.txt`), the RFC 5892 derived
property (IANA idna-tables), joining types, scripts, bidi classes and NFC
data, plus Punycode. Sourcemeta Core wrote its own (`core/src/core/idna`,
`core/src/core/punycode`, `core/src/core/unicode`).

### 4.2 What Boost 1.92 offers, measured

| Need | Boost | Verdict |
| --- | --- | --- |
| uri, uri-reference | Boost.URL `parse_uri`, `parse_uri_reference` return `system::result` | `parse_uri` 0/41 failures; `parse_uri_reference` 2/25: it accepts `//example.com:abc/p` and `//a@b@example.com/` (path-abempty not enforced after an authority in a relative-ref) (`$R/probe/url2.cpp`). **Not header-only**: `boost/url/src.hpp` is `#error "src.hpp is discontinued"`. It builds for wasm32-wasip2 with `-fno-exceptions` (19 sources, 2.97 MB at -O1, `$R/probe/url_wasi.cpp`). |
| iri, iri-reference | none | Boost.URL rejects non-ASCII: 5/20 and 7/15 failures |
| ipv4, ipv6 | Boost.URL `parse_ipv4_address`, `parse_ipv6_address` | 0/35 and 0/45 failures; reject zone ids and leading zeros as the tests want |
| ipv4, ipv6 | Boost.Asio `ip::make_address_v4/v6(str, ec)` | **unusable**: calls the OS `inet_pton` (`detail/impl/socket_ops.ipp:2847`), does not compile for wasip2 (`detail/socket_types.hpp:86` includes `<net/if.h>`), and natively accepts leading zeros, a NUL-truncated string and zone ids |
| uuid | Boost.UUID `string_generator` (throws, accepts braces: 2/23 failures); `boost::uuids::from_chars` (1.86+, non-throwing) | `from_chars` passes 23/23 when the result must consume the whole input |
| date-time, date, time, duration | Boost.DateTime | throws, no RFC 3339 leap seconds; a hand-written ABNF parser is simpler |
| Unicode, NFC, IDNA | Boost.Locale | compiled library; normalisation only with the ICU backend; no IDNA or Punycode at all (https://www.boost.org/doc/libs/latest/libs/locale/doc/html/using_localization_backends.html) |
| IDNA | ICU `uidna_*` | UTS #46 only, fails the tests above, large for wasm |
| exact numbers | Boost.Decimal (new in 1.91, header-only) | builds for wasip2 with `-fno-exceptions`; `multipleOf` must not use its `fmod` (section 2.4) |

**Recommendation.** Write every format as a small, header-only,
non-throwing ABNF parser over `std::string_view` returning a result, one
header per format, shared by the validator and the later zod port (zod's
`z.email()`, `z.iso.datetime()`, `z.ipv4()` and so on map onto the same
names). Boost.URL's grammar is the model for `uri` but is a compiled
library with a known bug, so do not depend on it for a header-only
library; Boost.UUID's `from_chars` is fine to reuse. Put IDNA in a
separate header with tables generated from the Unicode Character Database
and IANA's IDNA tables by a script, committed as generated files, so that
users who do not ask for `idn-*` pay nothing.

## 5. Regex

Sources: `$R/probe/regex_probe.cpp` and `$R/probe/features.cpp` (Boost
1.92 with and without ICU 78.3), the suite files named below.

### 5.1 What the suite asks of the regex engine

- Required (2020-12 and v1): `pattern.json` and `patternProperties.json`
  include `^\p{Letter}+$` ("requires unicode mode",
  `tests/draft2020-12/pattern.json` line 70, `patternProperties.json` line
  182). So Unicode property escapes are no longer optional.
- `optional/ecmascript-regex.json` (74 tests in 2020-12): `$` does not match
  before a trailing `\n`; `^` anchors only at the start; `\t`, `\cC`; `\d`,
  `\D`, `\w`, `\W` are ASCII-only (NKO digit zero, é and Bengali digits do
  not match `\d`/`\w`); `\s` includes U+00A0, U+FEFF, U+2029, U+2003 and
  excludes U+0001 and U+2013 (ECMA-262 WhiteSpace + LineTerminator,
  https://tc39.es/ecma262/#sec-white-space); `\p{Letter}`, `\p{digit}`.
  Note: `\p{digit}` is not a valid ECMA-262 property name, so a strict
  `new RegExp(..., "u")` throws on it; a suite bug worth reporting.
- `optional/non-bmp-regex.json` (12 tests): `^🐲*$` and `^.$` treat a
  non-BMP code point as one atom.
- `optional/format/ecmascript-regex.json` and `regex.json` (the `regex`
  format): invalid `\a`, `(?P<name>...)`, `(?P=n)`, `(?#...)`, `(?i)`,
  `^(abc]`; valid `(?<name>x)`, `\k<n>`, variable-width lookbehind
  `(?<=a+)b`, `[]`, `[^]`, `\cA`.

### 5.2 Boost.Regex against ECMA-262

- Header-only since 1.76 (https://www.boost.org/releases/1.76.0/), and
  builds for wasip2 and wasip3 (about 3.6 MB at -O1).
- **There is no ECMAScript dialect**: `boost/regex/v5/regbase.hpp` defines
  `perl = normal, ECMAScript = normal, JavaScript = normal`, and the docs
  say they "are all synonyms for perl"
  (https://www.boost.org/doc/libs/latest/libs/regex/doc/html/boost_regex/syntax/perl_syntax.html).
- Defaults differ: without `no_mod_m`/`no_mod_s` and `match_not_dot_newline`,
  `^`/`$` match per line and `.` matches `\n` (`^abc$` matched `"x\nabc"`).
- Narrow `boost::regex` on UTF-8 works on bytes: fails `\D`/`\W` on
  non-ASCII, `\s` on NBSP/FEFF/U+2029/EM SPACE, every `\p{Letter}` case
  (does not compile), and the non-BMP cases.
- `boost::u32regex` with ICU (`boost/regex/icu.hpp`, links ICU) passes
  `\p{Letter}`, non-BMP and NBSP, but fails the other way: `\d` and `\w`
  are Unicode-wide (NKO, é, Bengali match), and U+FEFF is not whitespace.
- Syntax: Boost accepts what ECMA rejects (`\a`, `\Z`, `\A`, `\z`, `a++`,
  `(?>a)`, `\Q...\E`, `(?i)`, `(?#...)`, `[[:alpha:]]`, `a{,5}`) and rejects
  what ECMA accepts (`[]`, `[^]`, variable-width lookbehind); `\uXXXX`
  means `[[:upper:]]` in Boost, so patterns must be translated to `\x{...}`.
- Exceptions: a bad pattern can be checked without throwing
  (`regex_constants::no_except` plus `basic_regex::status()`), but
  **matching** throws `error_complexity` through `boost::throw_exception`
  when its backtracking limit is reached
  (`perl_matcher_non_recursive.hpp:206,216`); under `BOOST_NO_EXCEPTIONS`
  that is an abort. `(a*)*b` against 40 `a`s aborted on wasip2 and wasip3.
  There is no non-throwing match API. For a library that validates
  untrusted schemas and instances without exceptions, that is
  disqualifying as the default engine.
- `std::regex` is worse (no named groups, no `\p`, catastrophic
  backtracking), which is what costs jsoncons its 2020-12 Bowtie points.

### 5.3 Options

| Option | Passes the suite | Header-only, no exceptions, wasm | Cost |
| --- | --- | --- | --- |
| Boost.Regex narrow + ECMA translation layer | no (`\p`, non-BMP) | header-only, aborts on complexity | small |
| Boost `u32regex` + ICU + translation | almost (fix `\d`/`\w`/`\s` by rewriting to explicit classes) | needs ICU, aborts on complexity | medium, heavy dependency |
| QuickJS `libregexp.c` + `libunicode.c` (MIT, ES2023, https://bellard.org/quickjs/) | yes | C, not header-only, vendoring needs a decision; backtracking with an interrupt hook; UTF-16/Latin-1 input | medium |
| RE2 | no lookaround or backrefs | compiled, C++ | - |
| CTRE | compile-time patterns only; good for format grammars, useless for `pattern` | header-only | - |
| **Own ECMA-262 engine** | yes by construction | header-only, code points over UTF-8, a step budget returning an error value instead of throwing, Unicode tables generated (shared with IDNA) | large (parser + backtracking matcher + property tables) |

**Recommendation:** a regex engine policy (concept) with an own
header-only ECMA-262 (`u` flag) engine as the default, structured like a
small backtracking VM with an explicit step budget, and fast paths
(literal prefix, `^.{m,n}$`, `.*`) in front as Blaze does; a Boost.Regex
adapter as an opt-in policy for users who already link it. The `regex`
format validator is the engine's parser alone. The Unicode tables
(General_Category, Script, Script_Extensions, binary properties) are
generated once from the UCD and shared with the IDNA module.

### 5.4 String length

`maxLength.json` "two graphemes is long enough" (`"💩💩"`, maxLength 2,
valid) and `minLength.json` "one grapheme is not long enough" (`"💩"`,
minLength 2, invalid) are the same from draft4 to v1. Count UTF-8 bytes
that are not `0x80-0xBF`. Boost.JSON rejects lone surrogates such as
`"\ud800"` by default (`$R/probe/surr.cpp`), and no 2020-12 or v1 test
contains one.

## 6. Existing C++ validators

Clones under `$R`: `blaze` (`f4c941e`, 2026-10-08), `core` (`57d452f`,
2026-10-08), `valijson` (`d358f96`, 2026-07-31), `jsoncons` (`8f5b66b`,
2026-10-07), `json-schema-validator` (pboettch, `0a5c207`, 2026-07-14).

### 6.1 Comparison

| | Sourcemeta Blaze + Core | jsoncons jsonschema | valijson | pboettch json-schema-validator |
| --- | --- | --- | --- | --- |
| Licence | AGPL-3.0-or-later or commercial (`blaze/LICENSE`, `core/LICENSE`) | BSL-1.0 (`jsoncons/LICENSE`) | BSD-2-Clause | MIT |
| Drafts | 3, 4, 6, 7, 2019-09, 2020-12; Core also knows OpenAPI 3.0-3.2 vocabularies (`core/src/core/jsonschema/include/sourcemeta/core/jsonschema_vocabularies.h`); no v1 | 4, 6, 7, 2019-09, 2020-12 (`doc/ref/jsonschema/jsonschema.md`) | 3 (deprecated), 4, 7 (`include/valijson/schema_parser.hpp:34-37`); 2020-12 only planned (`PLAN.md`) | 7 only |
| Bowtie, 2020-12 | 100% | 99.39% | - | no harness |
| Packaging | CMake libraries, C++23 (`core/cmake/common/defaults.cmake:3`) | header-only | header-only, C++17 | compiled library |
| JSON type | Core's own, with int64, double and arbitrary-precision Decimal | jsoncons `basic_json` | adapters, including Boost.JSON (`adapters/boost_json_adapter.hpp`, which casts uint64 to int64 at lines 337-338) | nlohmann |
| Regex | PCRE2 10.49 with JIT, `PCRE2_UTF \| PCRE2_UCP`, after an ECMA-262 translation (`core/src/core/regex/regex.cc`, `ecma262.h`) and fast paths | `std::regex` only (its `\p{...}` failures in Bowtie) | `std::regex`, Boost.Regex, or a template policy (`internal/regex.hpp`) | `std::regex` or Boost.Regex |
| Architecture | compiled to an instruction tree, 103 opcodes | compiled to a graph of virtual `keyword_validator` objects | interpreter over a `Constraint` tree with a visitor | virtual class tree walk |
| unevaluated* | static analysis, runtime marks only where needed | `evaluated_properties` sets returned upward | none | none |
| Output | Flag and Basic, plus simple, trace and JSON-LD; everything through one callback | message callback, JSON streaming, JSON Patch of defaults | list of errors | error-handler callback |
| Formats as assertions | all 2020-12 formats including IDN (own Unicode, IDNA, Punycode) | no IDN, iri, uuid, duration, uri-template | date, time, date-time, ipv4 | user checker; defaults for 11 formats |
| `multipleOf` | exact: integer `%`, otherwise exact Decimal (`core/src/core/json/json_value.cc:902-985`) | `std::remainder` on doubles versus ULP (`keyword_validator.hpp:1948-1953`) | double with relative tolerance | `std::remainder` on doubles |

Sources for each cell: the subagent survey of the clones above (file paths
quoted inline); compliance from `https://bowtie.report/draft2020-12.json`.

### 6.2 Blaze in detail, the reference design

- **Compile, then evaluate.** `compile()` produces a `Template` of
  `targets` (one instruction list per reference destination and per
  `$dynamicAnchor`), `labels` (dynamic-anchor hash to target), `extra`
  (cold metadata such as keyword locations, kept out of the hot struct) and
  the flags `dynamic` and `track` (`blaze/src/evaluator/include/sourcemeta/blaze/evaluator.h`).
  The template serialises to JSON (`compile_json.cc`), so a schema can be
  compiled offline; a JavaScript evaluator port exists (`ports/javascript`).
- **Instructions.** `{type, relative_instance_location, value, children,
  extra_index}` with a 26-way `std::variant` payload
  (`evaluator_instruction.h`, `evaluator_value.h`). 103 opcodes in five
  families (Assertion, Annotation, Logical, Loop, Control), dispatched
  through a `constexpr` table `HANDLERS<Track, Dynamic, HasCallback>[103]`
  (`evaluator_dispatch.h:2881-3015`). Fused "CISC" opcodes such as
  `AssertionTypeIntegerBounded` or `LoopPropertiesExactlyTypeStrictHash`
  cut dispatches.
- **Modes.** `FastValidation` (boolean only; merges `anyOf` of type checks
  into a bitset, flattens `allOf`) and `Exhaustive` (every branch,
  annotations) (`compiler.h:83-89`), plus tweaks: unrolling `properties`,
  an inline threshold of 50 instructions, `format_assertion`, a selective
  annotation set, and `max_instructions`/`max_depth` to bound untrusted
  schemas (`compiler.h:142-168`).
- **References.** `$ref` becomes `ControlJump(target)`; small non-recursive
  targets are inlined in a post-pass (`postprocess.h:321-351`);
  `$dynamicRef`/`$recursiveRef` become `ControlDynamicAnchorJump(anchor)`,
  resolved at run time by walking the stack of entered resources
  (`evaluator_dispatch.h:1586-1612`), and that machinery is compiled in only
  when the schema has dynamic references (`compile.cc:592-597`).
- **unevaluated\*.** `src/compiler/unevaluated.cc` computes, for each
  `unevaluated*` keyword, its static and dynamic dependencies. With none
  dynamic it compiles to `LoopPropertiesExcept` with a compile-time filter
  (names, prefixes, residual regexes) and tracks nothing at run time;
  otherwise only the dependency keywords emit evaluate marks into a vector
  that failed `anyOf`/`oneOf` branches rewind (`checkpoint()`/`rewind()`).
- **Paper.** Viotti and Mior, "Blaze: Compiling JSON Schema for 10x Faster
  Validation", arXiv 2503.02770 (https://arxiv.org/html/2503.02770): CISC
  instructions, inlining, `properties` unrolling, semi-perfect hashing of
  keys of 31 bytes or less, regex specialisation (prefix, length range,
  `.*`, `.+`, `core/src/core/regex/regex.cc:79-88`), static unevaluated
  analysis.

### 6.3 What webcpp should copy, and avoid

Copy:
1. **The split** between an immutable, shareable compiled schema (an
   instruction list per target, indices instead of pointers, so it can live
   in one pmr arena and be copied or serialised) and a per-call evaluator
   state.
2. **Specialisation by template parameters**: an output policy (flag,
   collecting, callback) and "has dynamic refs" as compile-time switches, so
   that `flag` pays nothing for annotations.
3. **Reference resolution at compile time**, inlining of small
   non-recursive targets, and dynamic scope only when used.
4. **Static analysis for `unevaluated*`**, with a runtime fallback of marks
   plus checkpoint/rewind; never sets of strings returned by value (the
   jsoncons pattern).
5. **One evaluation event callback** from which flag, basic, detailed,
   verbose, v1 list/hierarchical and Bowtie's annotation output are built.
   No C++ library offers detailed or verbose today.
6. **Regex fast paths** before the engine, and the engine as a policy.
7. **Exact `multipleOf`**, code-point string length, limits for untrusted
   schemas.
8. **The IR is not the instruction list.** Blaze compiles straight from
   JSON through Core's `SchemaFrame`. webcpp needs a dialect-neutral IR
   first (the zod port loads into it and emits from it), then lowers the IR
   into instructions; the interpreter of increment 1 can evaluate the IR
   directly and stay as the reference implementation that the compiled
   validator is tested against.

Avoid: copying Blaze or Core code (AGPL); `std::regex`; double-based
`multipleOf`; copying the dynamic scope per nested context (jsoncons
`common/eval_context.hpp:63-99`); applying `default` values as a side
effect (pboettch, jsoncons patches) inside validation.

## 7. Benchmarks

- **sourcemeta-research/jsonschema-benchmark**
  (https://github.com/sourcemeta-research/jsonschema-benchmark) is the
  validation-speed benchmark that matters: 37 real-world schemas with
  instances, each implementation in Docker printing `cold,warm,compile,parse`
  in nanoseconds; `make IMPLEMENTATIONS='blaze ajv' RUNS=5`, then
  `make dist/report.csv` and `make plots`. Latest CI run 37611006527 of
  2026-10-07 (artifact `report.csv`, saved at `$R/reports/sm/report.csv`),
  geometric mean of the ratio to the fastest per schema, warm: corvus-ts
  1.27x, corvus-rs 1.29x, corvus-cpp 1.36x, jsu-c 2.01x, **blaze 2.10x**,
  **ajv 9.67x**, boon 17.1x; cold: blaze 1.90x, ajv 34x. Corvus and JSU
  generate code ahead of time. Adding webcpp means adding an
  `implementations/<name>/` directory with a Dockerfile and a runner that
  prints the four timings; the comparison against ajv and Blaze then comes
  for free. A webcpp runner can also be run outside Docker on the same
  schema/instance files.
- **Bowtie's `bowtie perf`** (pyperf; `bowtie/benchmarks/`: the 2020-12
  meta-schema, nested schemas, the OpenAPI spec schema, useless keywords,
  and 18 keyword benchmarks under `keywords/`; weekly workflow
  `.github/workflows/benchmark.yml`; results at
  `https://bowtie.report/benchmarks/<dialect>.json`). It times the whole
  IHOP round trip per case, container IPC and per-case compilation included
  (`bowtie/_cli.py:3300-3302`), so it measures the harness more than the
  validator: in its 2020-12 data (2025-04-28) python-jsonschema "wins" 72
  of 112 tests. Useful only as a smoke test.
- **ebdrup/json-schema-benchmark** (Node.js only, draft-06, last pushed
  2024-07) compares JavaScript validators; not relevant for C++.
- A local micro-benchmark worth keeping in-tree: compile time and warm
  validation of the 2020-12 meta-schema against itself and against the
  suite's schemas, run natively and under wasmtime, so wasm overhead is
  visible.

## 8. Suggested increments

Each increment ends with a named slice of the official suite passing,
counted by a runner that reads the pinned suite (section 3) with the
remotes preloaded into a registry; required tests are a gate, optional ones
are reported. Sizes are rough: header lines of library code, excluding
tests and generated tables.

| # | Increment | Ends with | Size |
| --- | --- | --- | --- |
| 0 | **Runner and fixtures**: pin the suite (MIT) and the official meta-schemas; a test runner (Gherkin or Boost.Test) that walks `tests/<dialect>/`, loads remotes from `remotes/` under `http://localhost:1234/`, and prints a per-file score; an expected-failures list that each increment shrinks | the runner reports 0 of 1301 on 2020-12 | ~1k |
| 1 | **IR, loader and core keywords for 2020-12**: dialect-neutral IR in a pmr arena, the loader from `boost::json` (numbers from the lexeme via a `basic_parser` handler into `Number` = `decimal64_t` by default, exact `multipleOf`), JSON equality, code-point length, an interpreter over the IR for flag output; `pattern` through a temporary narrow Boost.Regex policy | 2020-12 required minus refs, unevaluated, vocabulary and `\p{...}`: about 770 of the 772 "core" tests (section 3.3) | ~5k |
| 2 | **References and dynamic scope**: base IRI resolution (RFC 3986 resolution written for the library), the user-owned registry and resolver callback, embedded resources, `$anchor`, JSON Pointer fragments, `$ref` with siblings, `$dynamicRef`, cycle detection at load time, validation of a schema against its meta-schema | + `ref`, `refRemote`, `anchor`, `defs`, `dynamicRef`, `infinite-loop-detection` (166 tests); optional `id`, `anchor`, `dynamicRef`, `unknownKeyword`, `refOfUnknownKeyword`, `no-schema` | ~3k |
| 3 | **Annotations, `unevaluated*` and output**: the evaluation event stream, annotation collection with drop-on-failure, `unevaluatedProperties`/`unevaluatedItems`, `$vocabulary`, flag/basic/detailed/verbose (2020-12) and list/hierarchical (v1), a Bowtie harness | 2020-12 required 1301/1301 (except `\p{Letter}` until increment 5); `annotations/tests` 58 tests; `output-tests/draft2020-12`; optional `format-assertion` (with a stub format set) | ~3.5k |
| 4 | **The other dialects**: loaders for 2019-09 (`$recursiveRef`, `items` array, `additionalItems`), draft-07, draft-06, draft-04 (`id`, boolean exclusive bounds, `1.0` not an integer) and v1 (unknown keywords refused, `x-` annotations, format assertion flag), all into the same IR; per-resource dialect switching | required 100% on 2019-09 (1261), 7 (929), 6 (841), 4 (618) and v1 (1135, without `format/`); optional `cross-draft`, `dependencies-compatibility`, `zeroTerminatedFloats`, `bignum`, `float-overflow` | ~2.5k |
| 5 | **ECMA-262 regex engine**: parser (also the `regex` format), backtracking VM over code points with a step budget and fast paths, generated Unicode property tables | `\p{Letter}` in required `pattern`/`patternProperties`; optional `ecmascript-regex` (74), `non-bmp-regex` (12), `format/regex`, `format/ecmascript-regex` | ~5k + tables |
| 6 | **Formats as assertions**: ABNF parsers for date-time, date, time, duration, email, hostname, ipv4, ipv6, uri, uri-reference, iri, iri-reference, uuid, uri-template, json-pointer, relative-json-pointer; then IDNA (UTS #46 mapping, IDNA2008 derived properties, CONTEXTJ/CONTEXTO, Bidi, NFC, Punycode) for hostname A-labels, idn-hostname and idn-email | `optional/format/` 874/874 on 2020-12 and the earlier drafts; v1 `format/` 868 (v1 required now 100%) | ~3k ASCII + ~3k IDNA + tables |
| 7 | **The emitter**: IR to JSON Schema for each dialect (2020-12, 2019-09, 7, 6, 4, v1, and OpenAPI 3.0 for zod), with the downgrade rules (prefixItems to array `items`, `$defs` to `definitions`, exclusive bounds to booleans) and an error for what a target cannot express | every required schema of every dialect round-trips: load, emit, reload, and the suite passes on the reloaded schema; emitting into another dialect passes the same tests where expressible | ~2k |
| 8 | **Compiled validator and benchmark**: lower the IR into an instruction list (reference targets, inlining, fused opcodes, static `unevaluated*` analysis, template specialisation per output policy), keep the interpreter as the oracle; a runner for sourcemeta-research/jsonschema-benchmark and the Bowtie harness on the compiled path | the whole suite passes on both paths with identical results; the 37 benchmark schemas timed natively and under wasmtime against Blaze and ajv | ~6k |

Order notes: increment 5 can run in parallel with 2-4 (it only plugs into
the regex policy); increment 6's ASCII half can start after 1. The zod port
can start after 3 for the validator and after 7 for `toJSONSchema`.
