---
title: "webcpp::json_schema: design (draft, decisions so far)"
tags: [webcpp, json-schema, design]
status: superseded
created: 2026-10-08
---

> **Superseded on 2026-10-12.** The owner abandoned SRELL, webcpp::regex, webcpp::json_schema, webcpp::formats, webcpp::idna and webcpp::zod (PLANS/WEBCPP_DESIGN.md, 2026-10-12). Kept for the record only.

# webcpp::json_schema: design (draft)

This file records the owner's decisions as the design conversation makes
them. It becomes the written spec once every section is approved.

Research: `~/.buzz/RESEARCH/JSON_SCHEMA_FOR_WEBCPP.md`,
`~/.buzz/RESEARCH/ZOD_FOR_WEBCPP.md`, and the engine spike
`~/.buzz/RESEARCH/JSON_SCHEMA_ENGINE_SPIKE.md` (in progress).

## Decisions so far (owner, 2026-10-08)

1. **Scope.** A library of webcpp's own, `webcpp::json_schema`, aiming at
   the complete JSON Schema specification, proven by the official
   JSON-Schema-Test-Suite. `webcpp::zod` starts only after json_schema is
   implemented, and reuses its dialect-neutral IR, loader, emitter,
   formats and validator.
2. **Dialects.** All of them: draft 4, 6, 7, 2019-09, 2020-12, and later
   v1 when it is published. Each dialect is a policy type whose machinery
   lives in `json_schema::detail`; the developer chooses, at
   instantiation, the set a program carries, e.g.
   `basic_validator<draft2020_12>` or
   `basic_validator<draft4, draft6, draft7, draft2019_09, draft2020_12>`.
   A future dialect is one more policy type.
3. **A document and the chosen set.** Each resource's `$schema` picks its
   dialect. A dialect outside the chosen set refuses the load, naming it.
   A document without `$schema` uses the first dialect of the list, which
   an option can override.
4. **Errors.** The specification's output formats (flag, basic, detailed,
   verbose) as `boost::json` values, plus a message hook passed in the
   validation options: a function receiving the structured error (keyword,
   instance and keyword locations, expected and received values) and
   returning the text; without a hook, default English messages. No global
   state. Locales: an optional header with a ready hook backed by
   Boost.Locale (`translate`, `format`, a message domain of the library's
   own, a shipped `.pot` catalogue); Boost.Locale is a compiled library, so
   only that header needs it, and json_schema stays header-only. Whether
   Boost.Locale builds for WASI is measured by the spike.
5. **Regex.** Boost.Regex is the default, behind a template policy. Suite
   tests that fail because of Boost.Regex stay written and are marked as
   known failures that never break CI. If more than 10% of the
   regex-dependent tests fail, look for an existing library and vendor it
   rather than write our own engine. Measured by the spike.
6. **URLs.** Boost.URL is the default, behind a template policy, with the
   same rule as regex.
7. **Blaze.** Its design ideas only (it is AGPL); no code.
9. **One engine, compiled at runtime.** Loading a schema compiles it once
   into an instruction list (references resolved, annotations analysed
   statically where possible, checks grouped), in the manner of Blaze;
   `validate` only runs that list. No separate interpreter: the official
   suite is the oracle. Build-time compilation (code generation or
   constexpr schemas) is out of scope; the IR and the instruction list are
   what such a step would consume later.
10. **`format`.** Each dialect follows its own specification by default:
    an annotation in 2019-09 and 2020-12 (and in drafts 4 to 7, the common
    reading), an assertion in v1. An option `format_assertion` forces
    assertion in any dialect, with the extensible format set passed in the
    options. The suite runs both ways: required tests with the default,
    `optional/format/` with assertion on.
11. **Regex engine: SRELL** (measured 0% failure on the 881 regex tests,
    header-only, BSD-2, errors as values), as the only shipped regex
    policy; Boost.Regex is not used at all (measured 18.7% failure and an
    abort without exceptions). SRELL has no git upstream (versioned zips),
    so it comes in as a mirror repository `webcpporg/srell`: one commit
    importing the upstream zip exactly (URL and SHA-256 in the message),
    our no-exceptions patch as a separate commit, each upstream release a
    new import commit; a submodule at `third_party/libs/srell`, exposed as
    `/webcpp/third_party//srell`; the patch is also offered upstream. The
    regex policy stays a template parameter.
12. **URLs: Boost.URL, extended, never reimplemented** (owner,
    2026-10-08). Boost.URL exposes a public grammar library
    (`boost::urls::grammar`: tuple/variant/optional/range/token/delim/
    literal rules, `lut_chars`, `parse`) and its RFC 3986 rules as public
    objects (`uri_rule`, `uri_reference_rule`, `relative_ref_rule`,
    `authority_rule`, `ipv4/ipv6_address_rule`, `pct_encoded_rule`,
    `pchars`...). Research: `~/.buzz/RESEARCH/URI_TEMPLATE_AND_IRI_LIBRARIES.md`
    (no usable library elsewhere: Boost and cppalliance have neither IRI
    nor RFC 6570; sourcemeta is AGPL; the rest expand only or fail).
    - `uri`: `parse_uri`, unchanged (100% of the suite).
    - `uri-reference`: `parse_uri_reference` plus a three-line guard (a
      relative reference with an authority whose path does not start with
      `/` is refused); a fix is proposed upstream to `boostorg/url`, and
      the guard leaves when a Boost release carries it.
    - `iri`/`iri-reference`: RFC 3987 section 3.1's own mapping: validate
      each non-ASCII code point against `ucschar` (and `iprivate` in the
      query only), percent-encode it into a buffer of the validation
      allocator, and hand the URI to Boost.URL (about 80 lines).
    - `uri-template` (RFC 6570): a small template grammar composed with
      Boost.URL's grammar combinators, its literals reusing the IRI range
      check (about 60 lines).
    - Boost.URL is a compiled library: a program using the default URL
      policy links `/boost//url`; json_schema itself stays header-only, and
      the URL policy stays a template parameter.
8. **Benchmark.** Against Blaze and ajv, with the community benchmark
   (sourcemeta-research/jsonschema-benchmark); Bowtie as the compliance
   reference.

## Sections approved

- **Section 1, architecture** (approved): boost::json -> load -> IR
  (dialect-neutral) -> compile -> plan -> validate -> events -> output;
  IR -> emit(dialect). Components: `ir` (public), `dialects` (public tags,
  machinery in detail), `loader`, `registry` (local), `compiler` (detail),
  `validator` (`basic_validator<Traits, Dialects...>`), `output` (four
  formats and the message hook), `formats` (extensible set), `emitter`,
  `numbers` (exact, `Number` parameter). Policies grouped in one `traits`.
  `load`/`compile`/`validate` return `boost::system::result`; a throwing
  form exists only when the config macro enables exceptions.
- **Section 2, public API** (approved, with the allocator made explicit):
  `js::validator` (all dialects), `js::validator_for<D...>`,
  `js::basic_validator<Traits, D...>`; `compile(doc, compile_options,
  allocator)` with designated-initializer options (`registry`, `resolver`,
  `default_dialect`, `format_assertion`, `formats`); the compiled schema is
  self-contained, immutable and thread-safe; `is_valid` (no allocation on
  the common path) and `validate(instance, validate_options, allocator)`
  returning an `outcome` with `errors()` and `to_json(format,
  storage_ptr)`; the message hook and the resolver are function
  references (no allocation), the hook writing into a `std::string&`;
  `schema->ir()` and `js::emit(node, dialect)`. Allocators: the type in
  the traits (pmr by default, compatible with Boost.JSON's storage_ptr),
  the instance passed at runtime, one for the schema's lifetime and one
  per validation; nothing allocates outside them, policies included; a
  test counts allocations.

- **Section 3, the engines underneath** (approved): regex = SRELL (the
  only shipped policy; compiled once at schema compile, an invalid pattern
  fails compile, a runaway search is a validation error, never an abort);
  URL = Boost.URL extended as in decision 12; locale = an optional header
  backed by Boost.Locale, on every target with exceptions, refused by name
  under `WEBCPP_JSON_SCHEMA_NO_EXCEPTIONS`, the core message hook working in
  both modes; numbers = read exactly from the JSON text through a
  `basic_parser` handler (a `double` already in a `boost::json::value` is
  converted once, documented), `Number` a template parameter
  (`decimal64_t` by default), integers exact in int64/uint64, `multipleOf`
  exact on coefficient and exponent, a larger `Number` for bignums.

- **Section 4, dialects and vocabularies** (approved, option (a)):
  vocabularies are the building blocks in `detail` (core, applicator,
  unevaluated, validation, meta-data, format-annotation,
  format-assertion, content; drafts 4 to 7 as fixed keyword sets); the
  loader normalizes every draft into the one IR (draft 4's boolean
  `exclusiveMaximum`, `dependencies`, array `items` with
  `additionalItems`, `$ref` siblings ignored up to draft 7,
  `$recursiveRef` as `$dynamicRef`, `definitions` as `$defs`); each
  resource carries its own dialect (cross-draft refs); `$vocabulary`: an
  unknown required vocabulary fails the load, an unknown optional one is
  ignored, `format-assertion` turns format assertion on; unknown keywords
  are annotations in 2019-09/2020-12 and refused (but `x-`) in v1, with an
  option `unknown_keywords` (annotate/ignore/reject); v1 becomes one more
  policy type when published, its suite tests marked as known failures
  until then. **A public extension point**: a user registers a vocabulary
  of their own in the compile options (its name, a handler reading the
  keyword into the IR, the instruction it compiles to), which then works
  as an official one; zod uses it for its refinements.

- **Section 5, references and registry** (approved, option (a)): every
  resource has a base URI (the root's from `compile`, else
  `urn:webcpp:root`; each `$id` opens a resource resolved by Boost.URL);
  reachable by JSON Pointer, `$anchor` (and drafts 6/7's `"$id": "#name"`)
  and `$dynamicAnchor`/`$recursiveAnchor`. `$ref` is resolved at compile
  into a direct jump of the plan; `$dynamicRef` is a runtime lookup in the
  dynamic scope only where a matching `$dynamicAnchor` exists, else a plain
  jump. A recursive schema is a loop of the plan; a cycle that consumes no
  instance (`{"$ref": "#"}`) is refused at compile, naming where; instance
  depth has a configurable limit with a measured default. `js::registry` is
  local; an unknown URI goes to the user's synchronous `resolver`
  (`result<boost::json::value>(uri)`), the library never touches network or
  disk; a resolver error is a compile error naming the URI; the compiled
  schema copies what it used. The official meta-schemas (2020-12 and its
  vocabularies, 2019-09, 7, 6, 4; about 40 KB) are bundled and parsed only
  when referenced. The suite's `remotes/` are registered into the registry
  under `http://localhost:1234` instead of serving them. **`compile`
  validates the schema against its meta-schema by default**, with an
  option to turn it off.

- **Section 6, the compiled plan** (approved): a flat instruction list (a
  mini bytecode run by a small interpreter, as Blaze does; ajv's
  generate-JavaScript approach needs a runtime compiler C++ and wasm do not
  have), operands as indices into constant tables, each instruction keeping
  its keyword location index, all in the schema's allocator. Iterative
  evaluation on an explicit stack in the validation's allocator, so no
  instance depth overflows the C++ stack (the depth limit is a validation
  error). One engine, two modes by a template output sink: `is_valid` stops
  at the first failure and records nothing; `validate` collects errors and
  annotations as the official formats need. Compile-time optimizations:
  fused type checks, large `enum` as a hash table, `required` with
  `properties`, cheap checks first, `true`/`false` subschemas folded, small
  `$ref` targets inlined. `unevaluated*`: static analysis marks which
  subschemas must report evaluation; runtime marks only there, with
  checkpoint and rewind across failing branches. A user vocabulary keyword
  compiles to a `call_user` instruction. `schema->explain()` prints the
  plan mapped to schema locations. The compile step is semantic analysis of
  a tree Boost.JSON has already parsed, not a text parser; pratt is not
  used here (no operator precedence in a schema; regex is SRELL's,
  `uri-template` Boost.URL's combinators, JSON Pointer trivial).

- **Section 7, output and messages** (approved, option (a)): `js::error`
  holds `kind` (`error_kind`, one per assertion of every draft: type,
  enum, const, false_schema, the numeric, string, array and object
  assertions, all_of/any_of/one_of/not/if_then_else,
  unevaluated_properties/items, content, user, depth_limit), `keyword`,
  `instance_location` and `keyword_location` (JSON Pointers),
  `absolute_keyword_location` (URI), `expected` (`boost::json::value`),
  `received` (a pointer into the instance, not a copy: the instance must
  outlive the `outcome`, documented; `to_json` copies into the report) and
  per-kind `details`. Official output formats of 2020-12 section 12 (flag,
  basic, detailed with failing branches only and single children collapsed,
  verbose with annotations), as `boost::json`; v1's list/hierarchical when
  v1 lands. Messages: default English templates per kind with parameters,
  built lazily (never by `is_valid`); the hook overrides them; the `.pot`
  catalogue (domain `webcpp_json_schema`, a context per kind) is generated
  from the template table by a tool, never by hand.
- **Section 8, formats** (approved, owner 2026-10-09): the format
  validators leave json_schema and become **two new webcpp libraries**,
  reusable by json_schema, zod and anything else:
  - **`webcpp::idna`** (repo `webcpporg/idna`, created with the owner's go):
    IDNA2008 / UTS 46. `punycode::encode`/`decode` (RFC 3492, errors as
    values), `to_ascii`/`to_unicode`, `is_valid_label`/`is_valid_domain`
    (PVALID, CONTEXTJ/CONTEXTO, NFC, the Bidi rule across labels);
    `detail::` holds UTF-8 decoding and the tables generated from Unicode
    17.0 as constexpr data (about 126 KB raw, 51 KB compressed, no heap) by
    a generator in `tools/` (Python, 600-800 lines, run only to update
    Unicode). Depends on `webcpp::config` only. Measured alternatives fail
    the 10% rule or other rules: ada-url/idna 16.3% on idn-hostname (also
    not header-only, global mutable state, no allocator); libidn2 2.2% but
    LGPL/GPL C; ICU 4.3% but a huge compiled dependency; cppalliance/punycode
    is IDNA2003 Nameprep (`~/.buzz/RESEARCH/IDNA_SPIKE.md`). ada's code is a
    reference for algorithms and table packing, never copied.
  - **`webcpp::formats`** (repo `webcpporg/formats`, created with the
    owner's go): `is_hostname`, `is_idn_hostname`, `is_email`,
    `is_idn_email`, `is_ipv4`, `is_ipv6`, `is_date_time`, `is_date`,
    `is_time`, `is_duration`, `is_uuid`, `is_uri`, `is_uri_reference`,
    `is_iri`, `is_iri_reference`, `is_uri_template`, `is_json_pointer`,
    `is_relative_json_pointer` (`string_view` in, `bool` out, nothing
    allocated); the same rules public in `formats::grammar` as Boost.URL
    rule objects (`hostname_rule`, `email_rule`, `idna_label_rule`,
    `date_time_rule`, `iri_rule`, `uri_template_rule`...), composable with
    Boost.URL's combinators; `<webcpp/formats/regex.hpp>` (`is_regex`,
    ECMA-262 via SRELL) a separate header so only it depends on SRELL;
    `detail::` for internals (the Boost.URL guard, character sets). ASCII
    grammars are written with Boost.URL's grammar library (its
    `ipv4_address_rule` already refuses leading zeros, `ipv6_address_rule`
    serves ipv6 and email's `[IPv6:...]`); IDN formats use custom rules
    (UTF-8 aware, consulting `webcpp::idna`) plugged into Boost.URL's
    combinators; Boost.URL itself is never modified.
  - Dependencies: idna <- config; formats <- idna, Boost.URL, SRELL (regex
    header only); json_schema <- formats (its default format set names
    `formats::is_*`); zod later <- formats, json_schema.
  - Size: about 1,700 to 1,900 lines of library code (idna ~1,000-1,200;
    formats ~700) plus the generator and tests. Target: 100% of the suite's
    format tests in every draft (Blaze proves it possible with its own
    IDNA2008; the IRI/template prototype passes 76/76, SRELL 881/881); a
    residue tied to a Unicode version is marked a known failure. The
    suite's `optional/format/` cases test `webcpp::formats` directly.
  - Content (`contentEncoding`, `contentMediaType`, `contentSchema`):
    annotations from 2019-09 on; an option turns on assertion (base64
    decode, JSON parse, `contentSchema` on the decoded content). The format
    set is extensible and local; an unknown format is an annotation.
- **Section 9, the emitter** (approved, option (a)):
  `js::emit(ir, dialect, options)` writes the dialect's own forms (`$defs`
  or `definitions`, `prefixItems` or array `items` with
  `additionalItems`, draft 4's boolean `exclusiveMaximum`, `$dynamicRef`
  or `$recursiveRef`), extracts nodes used more than once into `$defs`,
  turns cycles into `$ref`, keeps a deterministic key order, carries
  annotations and user vocabularies that registered a writer. What the
  target cannot express: `unrepresentable = error` by default (naming the
  place), or `drop` (recorded as an annotation). Tested by the whole suite
  as a round trip (load, emit, reload, same verdicts), and across dialects
  where both express the feature; zod's `toJSONSchema` is the oracle for
  zod's output later. **OpenAPI 3.0's Schema Object is a dialect** too,
  for load and emit (`nullable`, no type arrays, `discriminator`, its
  removed keywords); OpenAPI 3.1 is 2020-12 already.
- **Section 10, tests, CI and benchmark** (approved, option (a)): the
  official suite as a pinned submodule in `third_party/libs/`; one runner
  per dialect (4, 6, 7, 2019-09, 2020-12, OpenAPI 3.0, v1 later) on every
  target (native, wasip2, wasip3, emscripten; on wasm the suite reaches the
  program by wasmtime's `--dir`, through the build's run script); remotes
  registered into the registry; format tests with `format_assertion` on.
  Known failures: a tracked `test/known_failures/<dialect>.json` (test id
  and reason); CI fails both on a new failure and on a listed test that
  passes, so the list never lies. Also: the suite's `optional/format/`
  against `webcpp::formats` directly; the emitter round trip; unit tests
  (loader normalization per draft, compiler optimizations via `explain()`
  goldens, compile errors); an allocation-counting test (nothing outside
  the given allocators, `is_valid` allocation-free on the common path);
  both exception modes; a 100,000-level hostile instance (a depth error,
  no stack overflow); a libFuzzer target for loader and validator (local
  at will, a 1-2 minute budget in CI). The Bowtie harness (IHOP over
  stdin/stdout, plus its container definition) is part of the plan and
  runs locally and in webcpp's CI; joining Bowtie's public report (a pull
  request to bowtie-json-schema/bowtie) is an outward step taken only on
  the owner's go. **Benchmark**: sourcemeta's jsonschema-benchmark (37
  schemas) against pinned Blaze and ajv, native and under wasmtime; `b2
  benchmark` writes `results.json` (numbers, machine, date, versions),
  **run on demand on a stable machine (the owner's)**, the generated file
  committed; the page's table and chart are generated from it, and CI only
  checks the page reflects the committed file.
- **Section 11, webcpp's rules applied** (approved): `config.hpp`
  includes `<webcpp/config.hpp>` and defines
  `WEBCPP_JSON_SCHEMA_NO_EXCEPTIONS` (automatic without exceptions,
  definable by the developer); errors as values, `compile_or_throw` and the
  locale header behind the macro, the `bare-throw` lint and the
  no-exceptions twin check it; pmr allocators by default (type in the
  traits, instances at compile and validate, a counting test); targets
  native, wasip2, wasip3, emscripten, the suite on each; header-only
  (compiled dependencies only through the default URL policy via formats,
  and the optional Boost.Locale header; SRELL header-only as a submodule);
  `webcpp::json_schema`, `<webcpp/json_schema/...>`, `detail` for
  internals, macros prefixed `WEBCPP_JSON_SCHEMA_`; a page with complete
  Doc Comments, a strict MrDocs reference, a conformance table per dialect
  generated from the suite and the generated benchmarks; a changelog line
  in "Unreleased" with every change; BSL-1.0, "Copyright (c) 2026
  WebCpp.org", commits as R with no trailer.
- **Benchmarks in the documentation (owner, 2026-10-09):** the benchmark
  results (against Blaze and ajv) are published on json_schema's generated
  page, as a table and a chart with how they were measured, produced by a
  b2 target from a real run, never typed by hand. Detailed in section 10.

## Carried from webcpp's rules and the owner's earlier decisions

- Header-only C++20, `webcpp::json_schema`, headers under
  `<webcpp/json_schema/...>`; targets native, wasip2, wasip3, and
  emscripten once stage 4 brings it.
- Numbers are a template parameter, `boost::decimal::decimal64_t` by
  default; no floating point; `multipleOf` computed exactly (the research
  found Boost.Decimal's `fmod` fails a required test).
- Exceptions optional through a config macro, in the manner of
  `BOOST_ASIO_NO_EXCEPTIONS`, raised through `boost::throw_exception`;
  errors as values on the main path.
- Allocators customizable (the allocators stage's mechanism); avoid
  dynamic allocation as far as the job allows.
- No singletons: format sets, resolvers and hooks are passed in options.

## Spec written (2026-10-09)

`~/.buzz/PLANS/WEBCPP_JSON_SCHEMA_SPEC.md` (draft). Controller rulings on
its two open choices: E1 builds a first compiler and VM (decision 9 allows
no separate interpreter, so E1's slice passes on the real engine, which E2
completes); the output sink concept is frozen with the IR at the end of E1
(so E3 runs in parallel with E2). Risks carried to the plan: SRELL allocates
with malloc (measure the cost of an allocator patch in the mirror, then
the owner decides); Boost.URL is compiled while CI installs Boost headers
only (the formats plan builds it); the WASI runner needs wasmtime's `--dir`
before the wasi redesign's run scripts exist (a provisional `webcpp.jam`
change, migrated later); licences of the bundled meta-schemas and of
jsonschema-benchmark to check.
Owner (2026-10-09): WEBCPP_JSON_SCHEMA_SPEC.md approved; WEBCPP_CONFIG_SPEC.md approved
