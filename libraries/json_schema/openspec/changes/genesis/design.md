## Context

webcpp::json_schema is a header-only C++20 JSON Schema validator of webcpp's own (not a port).
Its design was approved by the owner section by section on 2026-10-08 and 2026-10-09
(`reference/design-source.md`, decisions D1 to D12 and sections S1 to S12), turned into a
specification approved on 2026-10-09 (`reference/spec.md`), and into a plan of 58 tasks in
five sub-tracks E1 to E5 (`reference/plan.md`). Tasks E1.1 to E1.9 were implemented, E1.10 was
written and left uncommitted, and on 2026-10-12 the owner shelved the library together with
SRELL and webcpp::regex. This document is the approved design in OpenSpec's form, with the one
decision that changed since: regular expressions now run on Boost.Regex through
webcpp::lexical's ECMA-262 translator.

Constraints every decision below works within, from the superproject's rulebook (its
`AGENTS.md`):

- header-only C++20, namespace `webcpp::json_schema`, headers under `<webcpp/json_schema/...>`,
  internals in `detail`, macros `WEBCPP_JSON_SCHEMA_*`;
- targets native (clang, gcc, MSVC through CI), wasip2, wasip3 and Emscripten, every test on
  each unless its Jamfile says why not;
- errors as values; exceptions optional through `WEBCPP_JSON_SCHEMA_NO_EXCEPTIONS`, raised only
  through `boost::throw_exception`; the library compiles and works without them;
- no global or static mutable state, no clock, file, network, thread, environment or entropy
  in a library header;
- allocation through allocators the program gives (pmr by default), as little as the job
  allows;
- Boost and std before custom code; third-party code only as a pinned submodule by the owner's
  decision;
- BSL-1.0, "Copyright (c) 2026 WebCpp.org" on every source file; English; no em dash.

## Goals / Non-Goals

**Goals:**

- The complete JSON Schema specification for drafts 4, 6, 7, 2019-09 and 2020-12, and OpenAPI
  3.0's Schema Object, for load and emit; v1 as one more policy type when it is published.
- Every vocabulary of 2019-09 and 2020-12, the fixed keyword sets of drafts 4 to 7,
  `$vocabulary`, cross-draft references, annotations and `unevaluated*`.
- The four output formats of 2020-12 section 12, a message hook, default English messages and
  an optional Boost.Locale header with a generated `.pot` catalogue.
- Formats as annotations by default and assertions on request, through webcpp::lexical;
  content assertion on request; exact numbers; an emitter; user vocabularies.
- Proof by the official JSON-Schema-Test-Suite on every target, with known-failure lists that
  never lie; Bowtie's harness; a benchmark against Blaze and ajv.
- A compiled schema immutable and shareable between threads; `is_valid` and `validate` that
  never disagree; no recursion on input depth anywhere.

**Non-Goals:**

- Draft 3.
- Build-time compilation (code generation, `constexpr` schemas): the IR and the plan are what
  such a step would consume later.
- Any network, disk or clock access: an unknown URI goes to the user's resolver.
- Applying `default` values or changing an instance in any way.
- v1's `list` and `hierarchical` output, until v1 is published.
- Joining Bowtie's public report, which needs the owner's go.

## Decisions

### D1. A library of webcpp's own, aiming at the whole specification

json_schema is written for webcpp, not ported, and aims at the complete specification, proven by
the official suite; webcpp::zod starts only after it and reuses its IR, loader, emitter, formats
and validator.

- **Why:** no existing C++ validator meets webcpp's rules together: complete dialect coverage,
  WebAssembly targets, no exceptions, caller-given allocators and a permissive licence.
- **Alternatives:** port Blaze (AGPL, so not even its code may be copied, D7); adopt one of the
  other C++ validators the research compared (incomplete dialects, exceptions or global state,
  no allocator control); write zod first (it needs exactly this engine underneath).

### D2. Every dialect is a policy type; the developer picks the set

`basic_validator<Traits, Dialects...>` takes the dialects a program carries as empty tag types
(`draft2020_12`, `draft2019_09`, `draft7`, `draft6`, `draft4`, `openapi3_0`); each tag's
keyword tables and normalization live in `detail::dialect_machinery<D>`. `js::validator` carries
all six, `js::validator_for<D...>` a chosen set. A future dialect is one more tag, one more
machinery and its line of `detail::has_machinery`.

- **Why:** a program that needs one dialect does not carry the others' tables; adding v1 later
  touches no existing dialect. The concept reads `has_machinery`, stated beside the tags, so its
  answer cannot depend on include order ([temp.expl.spec]/7).
- **Alternatives:** a runtime enum only (every program carries every dialect); one dialect only
  (fails D1).

### D3. Each resource's `$schema` picks its dialect

A resource's `$schema` selects its dialect; a dialect outside the validator's set refuses the
load, naming it (`errc::unknown_dialect`). A resource without `$schema` takes the enclosing
resource's, else `compile_options::default_dialect`, else the first dialect of the list. Each
official URI is accepted with and without its empty fragment; any other spelling is refused by
name. OpenAPI 3.0 is chosen only by `default_dialect` or by standing first, since its Schema
Object carries no `$schema`.

- **Why:** cross-draft references are part of the suite; a silent fallback would validate with
  the wrong rules.
- **Alternatives:** one dialect per document (fails `optional/cross-draft`); guessing the dialect
  from keywords (ambiguous).

### D4. Output: the specification's formats, a message hook, no global state

Errors and annotations render as the four output formats of 2020-12 section 12 (`flag`, `basic`,
`detailed`, `verbose`) as `boost::json` values. Messages are built lazily, never by `is_valid`,
from one table of English templates with named parameters; a hook in the validate options (a
`function_ref` writing into a `basic_string` of the traits' allocator) replaces them. An
optional header, `output/locale.hpp`, provides a ready hook over Boost.Locale (`translate` with
the kind as context, `format`, domain `webcpp_json_schema`), refused by name under
`WEBCPP_JSON_SCHEMA_NO_EXCEPTIONS`; the `.pot` catalogue is printed by a C++ program from the
same table and checked by the build, never written by hand.

- **Why:** the official formats are what other implementations and Bowtie compare; a hook keeps
  localisation out of the core and out of global state; Boost.Locale is compiled and throws, so
  only one optional header needs it and json_schema stays header-only.
- **Alternatives:** formatted strings only (not comparable); a global message registry (global
  state); gettext directly (not portable to WASI).

### D5. Regular expressions: Boost.Regex 1.92 through webcpp::lexical's ECMA-262 translator

This decision supersedes design decisions 5 and 11 and section 3's regex engine (SRELL, later
webcpp::regex). `pattern` and `patternProperties` are compiled once, at schema compile, by the
traits' regex policy; the default policy, `boost_regex` in `regex/boost_regex.hpp`, consumes
webcpp::lexical's public matching interface: formats' translator reads ECMA-262's
`Pattern[+UnicodeMode]`, refuses what ECMA-262 refuses, and writes a Boost.Regex pattern over
code points with ECMA-262's meaning (a `char32_t` traits class over generated Unicode 17
tables), which Boost.Regex 1.92, header-only and unmodified, compiles; a search runs over the
UTF-8 text through a decoding iterator that spends a step budget.

- **The policy concept is unchanged** (`compile(text, schema_allocator, step_budget)`,
  `search(re, text, validation_allocator)` answering `found`, `not_found` or `limit`), so the
  compiler and the VM do not change; only the default policy and `build.jam` do.
- **Refusals:** a pattern ECMA-262 refuses is `errc::invalid_regex`; a valid pattern the engine
  cannot run as ECMA-262 runs it (an unbounded lookbehind that is not leading, lookbehind
  alternatives of different widths inside a group, a backreference into a quantified group or
  inside a lookbehind) or past the translator's bounds (nesting over 32) is the new, appended
  `errc::unsupported_regex = 21`. Both name the keyword's location and the pattern.
- **Runaway searches:** the step budget (`compile_options::regex_step_budget`, iterator steps
  per search; 0 means 10,000,000 plus 64 per byte of the text) turns catastrophic backtracking
  into `match_result::limit`, an error of kind `pattern_limit`, never an abort. formats owns
  Boost.Regex's configuration (`BOOST_REGEX_MAX_BLOCKS` raised; Boost's own `error_complexity`
  and `error_stack` caught with exceptions and answered `limit`).
- **Allocation, stated as an exception:** Boost.Regex allocates its compiled program (7 to 9
  global allocations per pattern) and its backtracking stack (4 KiB blocks, a process-wide
  cache of up to 16) through the global `operator new`. The translator's memory and
  `match_results` go through the given allocators. The two-allocator guarantee and the
  allocation-free `is_valid` hold for schemas without patterns; the page states the exception
  beside the resolver's and the hook's, and the allocation test measures it.
- **json_schema never copies the translator and never includes formats' `detail`.** The
  include boundaries allow `webcpp/lexical/regex.hpp` only in `regex/boost_regex.hpp`.
- **Why:** the owner abandoned SRELL and webcpp::regex on 2026-10-12. The translator over
  Boost.Regex passes 881 of 881 regex-dependent tests of the suite in every draft on all five
  lanes (native clang and gcc, wasip2 without exceptions, wasip3, Emscripten), gave zero wrong
  answers against V8 on 87,028 fuzz queries (every difference an `unsupported` refusal or a V8
  deviation), matches V8 on all 20,130 fuzzed patterns for `format: regex`, and already ships
  in formats for that format, so json_schema shares one engine with it.
- **Alternatives:** SRELL (abandoned by the owner; its 2026.06 counted-repeat regression gives
  wrong answers on a common path such as `(?:ab){1,2}` on `"ab"`); webcpp::regex (abandoned);
  raw Boost.Regex (18.7% of the regex tests fail, and a runaway search aborts without
  exceptions); `std::regex` (bytes, not code points, no `u` mode); a fork of Boost.Regex (11,413
  header lines carried, for allocation and lookbehind); an engine of webcpp's own (the research
  rule: vendor an existing library before writing one).
- **Trade-offs:** searches are 2x to 400x slower than SRELL's on large instances (compiles are
  faster); the allocation exception above; Boost's fixed state cap for a non-random-access
  iterator can still abort a long legitimate search on wasm32 (a documented limit, measured in
  the feasibility research, section 5.3).

### D6. URLs: Boost.URL, extended, never reimplemented

Base URIs, `$id` and `$ref` resolve and normalize (RFC 3986 sections 5.2 and 6.2.2) through the
URL policy, `boost_url<Capacity = 4096>` by default, writing into a fixed buffer, a URI longer
than the capacity being `errc::invalid_uri`. The URL formats (`uri`, `uri-reference`, `iri`,
`iri-reference`, `uri-template`) belong to webcpp::lexical, which extends Boost.URL with its
public grammar library. The default URL policy links `/webcpp//boost_url`, Boost.URL's compiled
part built by webcpp from Boost's sources for every target (replacing the specification's
`/boost//url`, owner's decision of 2026-10-09); json_schema itself stays header-only and the
policy stays a template parameter.

- **Why:** Boost.URL passes every `uri` test and exposes the grammar to extend; no other usable
  library exists for IRI or RFC 6570.
- **Alternatives:** a URI parser of webcpp's own (reimplementation); ada-url (not header-only,
  global state).

### D7. Blaze's ideas only

Blaze is AGPL: its design (a compiled instruction list, static `unevaluated*` analysis, fused
checks) informs this one; no code is copied or linked. It is only run, as a benchmark opponent.

### D8. Benchmark and compliance references

The benchmark is sourcemeta's jsonschema-benchmark (37 schemas) against pinned Blaze and ajv,
natively and under wasmtime, run on demand on the owner's machine (never in CI); `b2` writes
`benchmark/results.json`, and the page's table and chart are generated from it, never typed.
Bowtie is the compliance reference: a harness speaking IHOP over standard input and output.

### D9. One engine, compiled at runtime

Loading compiles a schema once into a flat instruction list (a mini bytecode with operands as
indices into constant tables, each instruction keeping its keyword location), run by a small
interpreter on an explicit stack. There is no separate tree-walking interpreter: the official
suite is the oracle from the first slice. Two modes share the engine through a template sink:
`flag_sink` (records nothing, short-circuits) for `is_valid`, `collecting_sink` for
`validate`.

- **Why:** one engine cannot drift from another; a flat plan of indices is relocatable,
  copyable and later serializable; no input depth reaches the C++ stack.
- **Alternatives:** a tree-walking interpreter (recursion on depth, slower, a second engine to
  keep in agreement); ajv's code generation (needs a runtime compiler, which C++ and WebAssembly
  do not have); build-time compilation (out of scope; the plan is what it would consume).

### D10. `format` follows each dialect, with an option to assert

`format` is an annotation in drafts 4 to 2020-12 by default (and an assertion in v1); an option
`format_assertion` forces assertion in any dialect, with the format set passed in the options;
the `format-assertion` vocabulary asserts in its resources. The suite runs both ways: required
tests with the default, `optional/format/` with assertion on.

### D11. Architecture: one pipeline, one traits type

`boost::json -> load -> IR -> compile -> plan -> validate -> events -> output`, and
`IR -> emit(dialect)`. Components: `ir` (public), `dialects` (public tags, machinery in
`detail`), `loader`, `registry`, `compiler` and `vm` (detail), `validator`, `output`, `formats`
(the extensible set), `emitter`, `numbers`. Policies are grouped in one `traits` type
(`number_type`, `allocator_type`, `regex_policy`, `url_policy`); `load`, `compile`, `validate`,
`emit` and `to_json` return `result<T>`, `boost::system::result<T, diagnostic>`, whose
`diagnostic` holds a code and two 256-byte `static_string`s, `where` and `what`, so a refusal
under an exhausted allocator still names its place.

- **Alternatives:** separate template parameters per policy (unwieldy signatures);
  `std::error_code` alone (no place named); exceptions on the main path (fails webcpp's rule).

### D12. The public API

`js::validator`, `js::validator_for<D...>`, `js::basic_validator<Traits, D...>`, stateless and
`final`; `compile(doc, compile_options, allocator)` with designated-initializer options
(`registry`, `resolver`, `base_uri`, `default_dialect`, `format_assertion`, `formats`,
`content_assertion`, `regex_step_budget`, `unknown`, `compatibility_keywords`,
`validate_schema`, `vocabularies`, `max_schema_depth`); `compile` also takes a
`basic_parsed_json` (exact numbers) and a built IR (zod's path, `check()` first). The compiled
`basic_schema` is a shared handle on an immutable, self-contained schema, safe to share between
threads. `is_valid(instance, options, allocator)` returns `bool`; `validate` returns an
`outcome` with `valid()`, `errors()`, `annotations()`, `message(error)` and `to_json(format,
storage)`. `schema->ir()` and `js::emit(ir, dialect, options)` expose the IR. A throwing
`compile_or_throw` exists only with exceptions.

### D13. Numbers are exact

`Number` is `traits::number_type`, `boost::decimal::decimal64_t` by default, `decimal128_t` for
bignums, any type modelling the `number` concept through `number_traits`; no floating point.
Schema integers stay `int64_t` or `uint64_t`; other schema numbers are read from the lexeme by
`js::parse` (a `boost::json::basic_parser` handler recording the exact value of every number
whose double does not reproduce its text). An instance integer is compared exactly; an instance
double is read once as the exact decimal its shortest round-trip text denotes and compared
exactly, never rounded to `Number` (amendment A1). `multipleOf` is exact on coefficients and
exponents by modular arithmetic in 128 bits, never `fmod`.

- **Why:** required suite tests fail otherwise: rounding to `decimal64_t` accepts 2^64 under
  `maximum: 18446744073709551615`; a double's binary value makes `1.1` exceed
  `exclusiveMinimum: 1.1` and `0.0075` no multiple of `0.0001`; Boost.Decimal's `fmod` fails a
  required test.
- **Alternatives:** `double` everywhere (fails the suite); "converted once to `Number`" (the
  specification's first wording, amended).

### D14. Vocabularies as building blocks; the loader normalizes every draft

Vocabularies (core, applicator, unevaluated, validation, meta-data, format-annotation,
format-assertion, content; drafts 4 to 7 as fixed keyword sets) are building blocks in `detail`.
The loader normalizes every draft into one dialect-neutral IR: draft 4's boolean
`exclusiveMaximum`, `dependencies`, array `items` with `additionalItems`, `$ref` siblings
ignored up to draft 7, `$recursiveRef` as a dynamic reference, `definitions` as `defs`. Each
resource carries its own dialect. `$vocabulary`: an unknown required vocabulary fails the load,
an unknown optional one is ignored, a known one left out turns its keywords `unknown`,
`format-assertion` turns assertion on. Unknown keywords annotate in 2019-09 and 2020-12, are
inert in drafts 4 to 7, and are refused in OpenAPI 3.0 and v1 (except `x-`), with an option
`unknown_keywords` (dialect default, annotate, ignore, reject). A public extension point lets a
user register a vocabulary (its URI, keywords with a `check`, a `validate`, the types it
applies to, whether it annotates, an optional `write` for the emitter, and an activation
`always` or `when_declared`), compiled to `call_user`; zod's refinements use it.

- **Why:** the compiler, the emitter and zod read one IR; no consumer reads the source dialect
  for semantics, so every difference is an explicit field ([ir-3]).
- **Alternatives:** one IR per dialect (every consumer multiplied); a separate validator per
  draft (cross-draft references impossible).

### D15. References and the registry

Every resource has a base URI (the root's from `compile_options::base_uri`, else
`urn:webcpp:root`; each `$id` opens a resource). Nodes are reachable by JSON Pointer, plain
anchors (and drafts 6/7's `"$id": "#name"`) and dynamic anchors. `$ref` is resolved at compile
into a direct jump; `$dynamicRef` is a runtime lookup in the dynamic scope only where it is
bookended, else a plain jump. A cycle of in-place references that consumes no instance is
refused at compile (`errc::reference_cycle`). Lookup order: the document's resources, the
user's `registry`, the bundled official meta-schemas (parsed only when referenced), the user's
synchronous `resolver`; the library never touches network or disk; what was used is copied into
the IR. `compile` validates each resource against its meta-schema by default
(`validate_schema`), and a given registry caches the compiled meta-schemas. The suite's
`remotes/` are registered under `http://localhost:1234/`.

### D16. The compiled plan and the VM

Blocks (one per compiled node, numbered breadth-first), instructions (`op`, `flags`, `small`,
`a`, `b`, `location`), and tables (operands, locations, compiled patterns, constants), in the
schema's allocator, indices only. The VM runs on an explicit stack of frames in the validation's
allocator with an inline buffer first; an applicator pushes a child frame. `unevaluated*`:
static analysis finds the contributors; only they emit `mark_evaluated`, with checkpoint and
rewind across failing branches; a schema without `unevaluated*` pays nothing. Later
optimizations (fused type checks, a large `enum` as a hash table, `required` with `properties`,
cheap checks first, folded boolean subschemas, inlined small `$ref` targets, a static
`unevaluated*` filter) apply to the flag plan only (PD10), so they never change a recorded
error. `schema->explain()` prints the plan with each instruction's schema location.

### D17. Output types and the frozen contract

`js::error` holds `kind` (`error_kind`, one per assertion of every draft, values 1 to 43 fixed),
`keyword`, `instance_location`, `keyword_location` (with `$ref` segments),
`absolute_keyword_location`, `expected` (the keyword's value) and `received` (a reference into
the instance, which must outlive the outcome), and per-kind `details`. At the end of E1 the IR
headers, the sink and its events, the output record, `output/error.hpp`, `output/outcome.hpp`
and `output/message_hook.hpp` freeze (tag `contract-1`): a later change is proposed and applied
to every open branch at once; appending a `keyword_kind`, `errc` or `error_kind` value is not a
change; reusing or reordering one is forbidden.

- **Why:** E2, E3 and E4 then run in parallel against a stable interface.

### D18. Formats through webcpp::lexical and webcpp::idna

Format validators live outside json_schema: webcpp::idna (IDNA2008 and UTS 46) and
webcpp::lexical (`is_date_time`, `is_email`, `is_hostname`, `is_idn_hostname`, `is_ipv4`,
`is_ipv6`, `is_uri`, `is_iri`, `is_uri_template`, `is_json_pointer`, `is_regex`, and the rest,
`string_view` in, `bool` out). json_schema's format set is local and extensible
(`compile_options::formats`, a span of `format_entry`); `default_formats()` names formats'
functions, calling the allocating ones through their resource overload (amendment A2,
`format_entry::check_in`). An unknown format is an annotation. Content (`contentEncoding`,
`contentMediaType`, `contentSchema`) annotates from 2019-09 on; `content_assertion` decodes
`base64`, parses `application/json` and applies `contentSchema`.

### D19. The emitter

`js::emit(ir, dialect, options)` writes each dialect's forms (`$defs` or `definitions`,
`prefixItems` or array `items` with `additionalItems`, draft 4's boolean `exclusiveMaximum`,
`$dynamicRef` or `$recursiveRef`, `dependencies` or the split pair, OpenAPI 3.0's `nullable`),
extracts nodes reached more than once into definitions (named by their `defs` name, else
`__schema<N>`), turns cycles into `$ref`, keeps a deterministic key order, writes a `$ref` with
evaluated siblings as `allOf` in drafts 4 to 7 (PD4), and treats what the target cannot express
as `errc::unrepresentable` (default) or drops it into `"x-webcpp-dropped"`. OpenAPI 3.0's
shared nodes come back in `emitted::components`. Proven by a round trip of the whole suite:
load, emit, reload, equivalent IR (`ir::equivalent`) and identical verdicts.

### D20. Deep input, allocation and exceptions

- **No recursion on input depth** anywhere: loader, compiler, VM, equality, hashing, copies,
  renderers, emitter, `equivalent`, `check`. Values are copied only by an iterative
  `detail::deep_copy` into a trivially deallocating arena; `js::parse` bounds Boost.JSON's
  recursive parser with `max_depth`, clamped to a depth measured safe on every target.
- **Two allocators:** the schema's (IR, plan, constants, the translator's memory for patterns,
  the shared control block) and the validation's (frames, marks, dynamic scope, collected
  events, strings, messages, a search's `match_results`). pmr by default; Boost.JSON allocates
  through a header-only adapter, never through `boost::container::pmr`'s compiled globals.
  Exceptions, stated on the page: the user's resolver, hook, format and vocabulary functions,
  Boost.Locale, and Boost.Regex's compiled programs and backtracking stack (D5).
- **Exceptions:** `config.hpp` includes `<webcpp/config.hpp>` first and defines
  `WEBCPP_JSON_SCHEMA_NO_EXCEPTIONS` when `WEBCPP_HAS_EXCEPTIONS` is not defined, or when the
  developer defines it; `compile_or_throw` and the locale header sit behind it.

### D21. Tests, the suite and known failures

The suite is the superproject's submodule `third_party/libs/JSON-Schema-Test-Suite` at `5b0ee16`,
read at run time (never copied: MIT test data). One runner per dialect on every target
compiles each case with the dialect as default, registers the remotes, runs `is_valid` and
`validate` on every test and requires them to agree. Known failures are
`test/known_failures/<runner>.json`, recorded from a native run; the runner fails on an unlisted
failure, on a listed test that passes and on a listed test it cannot find, so the lists never
lie. Also: unit tests per module, the annotation and output suites, the emitter round trip, an
allocation-counting test, both exception modes, a 100,000-level hostile instance, libFuzzer
targets (Linux clang lane), the Bowtie harness, and the benchmark.

### D22. Sub-tracks and their order

E1 (skeleton, numbers, regex, IR, the 2020-12 loader without references, the compiler and VM for
the core keywords, `is_valid`, `validate`, messages, the four renderers, the suite runner,
limits, the first page; ends with the freeze), then E2 (references, meta-schemas, annotations,
`unevaluated*`), E3 (the `.pot`, the locale header, the output proofs) and E4 (the emitter and
`ir::equivalent`) in parallel, then E5 (every other dialect, formats, user vocabularies, content,
fuzzing, Bowtie, optimizations, the benchmark, conformance and the rest of the page). Each ends
with a slice of the suite passing on every target.

### D23. Choices the plan made for the owner (binding unless the owner reverses one)

- **PD1:** the English messages and the four renderers are E1's (the outcome freezes there); E3
  does the catalogue, the locale header and the proofs.
- **PD2:** an unknown keyword's IR kind carries its meaning: `unknown` (annotates) in 2019-09 and
  2020-12, `ignored` (inert) in drafts 4 to 7, refused in OpenAPI 3.0.
- **PD3:** `ir::equivalent` is modulo the emitter's rewrites.
- **PD4:** a `$ref` with evaluated siblings is emitted as `allOf` in drafts 4 to 7 and OpenAPI.
- **PD5:** `error_kind::schema` (41) is reserved; no keyword raises it.
- **PD7:** draft 4's `integer`: an integral double outside both 64-bit ranges counts as an
  integer (`1e60`, the one stated deviation).
- **PD8:** `time` admits leap seconds in `default_formats()`.
- **PD9:** the Bowtie lane is local until the CI installs Docker and Bowtie.
- **PD10:** two plans: `validate` runs a record plan, `is_valid` a flag plan with every
  optimization.
- **PD11:** the benchmark chart is an inline SVG in `doc/benchmark.adoc`.
- **PD12:** `output/message_hook.hpp` holds the hook's alias alone, to freeze it.
- **PD13:** `collect` lives in `output/error.hpp`.
- **PD14:** `validator` is `validator_for<draft2020_12>` until every dialect has its machinery.
- **PD15:** references are defined after resolution, so the frozen document is never mutated.
- **PD16:** with `validate_schema`, the meta-schema check runs before the walk; `load()` never
  checks.
- **PD17:** the URL policy links `/webcpp//boost_url`.

## Risks / Trade-offs

- [webcpp::lexical's matching interface does not exist yet] -> E1.4's redo waits for formats to
  publish `compile_regex` and `search` (or equivalents); json_schema adapts to formats' names and
  never reaches formats' `detail`. Until then the snapshot builds only where webcpp::regex still
  exists (STATUS.md).
- [Boost.Regex allocates outside the given allocators] -> stated as an exception on the page and
  measured by the allocation test; schemas without patterns keep the full guarantee.
- [Boost's fixed state cap on wasm32] -> a long legitimate search can abort there; documented as
  a known limit, with the step budget set below the cap.
- [Search speed] -> 2x to 400x slower than SRELL on megabyte instances; the benchmark measures
  it; a faster policy can be added later without changing the concept.
- [Compiled Boost libraries in CI] -> Boost.URL and Boost.Locale are built from Boost's sources
  by the superproject (`/webcpp//boost_url`, `/webcpp//boost_locale`); formats meets it first.
- [The sink as a second frozen contract] -> a gap found late costs E2 and E3 both; the E1 review
  checks it before the freeze.
- [E2 is the largest track] -> dynamic scope, `unevaluated*`, meta-schema compile cost; the
  registry cache mitigates the last.
- [Licences of bundled text] -> the official meta-schemas are "BSD 3-clause or AFL-3.0", the
  OpenAPI 3.0 schema Apache-2.0, jsonschema-benchmark has no licence; owner decisions below.
- [Exactness inside constants] -> bounded by the `boost::json::value`'s precision; a user needing
  more parses with `js::parse` and a larger `Number` (covers bounds, not constants).
- [IDN formats tied to a Unicode version] -> a residue becomes known failures in E5.
- [v1 may change before publication] -> nothing ships for it until then.
- [Boost.JSON's recursive destruction and parser depth] -> the hostile test and the page say how
  to build and free a deep instance (a monotonic resource; `parse_options::max_depth`).

## Migration Plan

There is nothing to migrate: no program depends on json_schema. To resume the implementation
(details in `reference/STATUS.md`):

1. Recreate the library's repository from `reference/source/` (one import commit), add it to a
   superproject worktree at `libs/json_schema`, and apply `reference/source-wip/`.
2. Do group 0 of `tasks.md`: the snapshot follows the superproject's current rules.
3. Redo task 1.4 on formats' translator; commit E1.10 after its lanes and lint are green.
4. Continue with task 1.11 and the plan, in its order.

Rollback: the change is a proposal; withdrawing it deletes `openspec/changes/json-schema/`.

## Open Questions

- The owner's go to revive the library, and where its canonical local repository lives now.
- lexical's public matching interface (lexical's change `strategies`, group 3): its names, and whether its compiled pattern keeps the
  resource it was given (E1.4).
- `errc::unsupported_regex = 21`, appended by this change, and the allocation exception for
  Boost.Regex (D5): the owner confirms both.
- Amendment A1 (numbers, the exact decimal of the shortest text), which E1.2 implements; and A2
  (`format_entry::check_in`), which E5.5 needs.
- The licence of the bundled meta-schemas and the OpenAPI schema inside a BSL-1.0 repository
  (recommended: commit them with `LICENSE-JSON-SCHEMA.txt` and `LICENSE-OPENAPI.txt`).
- jsonschema-benchmark's missing licence, the benchmark's pins (its commit, Blaze, ajv) and
  Bowtie's pin.
