---
title: "webcpp::json_schema: specification"
tags: [webcpp, json-schema, specification]
status: active
created: 2026-10-09
---

# webcpp::json_schema: specification

> **This copy, in the OpenSpec change `json-schema`.** The specification as
> the owner approved it on 2026-10-09, with every detail;
> `specs/libraries/json-schema/spec.md` is its structured form, and
> `design.md` its decisions. The "superseded" banner is removed (this
> proposal revives the library), the commit identity is the superproject's
> current one, and paths of the original machine are written under `~/`
> (the owner's planning notes, not published). Where this text names SRELL,
> webcpp::regex or `webcpp_regex` as the regex engine (D11, S3, 3, 3.1, 4.2,
> 4.5, 10, 13, 14, 20.1, 21), `design.md`'s decision "Regular expressions"
> supersedes it: `pattern` and `patternProperties` run on Boost.Regex 1.92
> through webcpp::lexical's ECMA-262 translator.

`webcpp::json_schema` is a library of webcpp's own (not a port): a
header-only C++20 JSON Schema validator that aims at the complete
specification, every published dialect, and is proven by the official
JSON-Schema-Test-Suite. Its dialect-neutral IR, loader, emitter and
validator are what `webcpp::zod` reuses later.

This specification turns the approved design into the contract the
implementation follows. Its sources:

- `~/.buzz/PLANS/JSON_SCHEMA_DESIGN.md`: the design, decisions D1 to D12 and
  sections S1 to S11 plus the benchmark entry, approved by the owner section
  by section on 2026-10-08 and 2026-10-09. Binding; never reopened here.
- Section 12 (increments, order, parallel structure), approved in
  conversation on 2026-10-09 and written in section 20 below.
- `~/.buzz/PLANS/WEBCPP_DESIGN.md`, "Owner decisions after approval": binding.
- `~/dev/webcpp/AGENTS.md`: the rules every library follows.
- Research: `~/.buzz/RESEARCH/JSON_SCHEMA_FOR_WEBCPP.md` (called R below),
  `JSON_SCHEMA_ENGINE_SPIKE.md` (called SPIKE), `ZOD_FOR_WEBCPP.md` (ZOD),
  `WASIP2_EXCEPTIONS_SPIKE.md`.
- `~/.buzz/PLANS/WEBCPP_LEXICAL_SPEC.md` and `WEBCPP_IDNA_SPEC.md`, written
  in parallel: json_schema uses `webcpp::lexical` only through the
  interface the design states (section 10).

Precedence: where this specification and the design differ, the design wins
and this file is corrected; where this file and `AGENTS.md` differ,
`AGENTS.md` wins. Section 22 lists every point this file settles that the
design left open, for the controller's cross-check.

## 1. Purpose and scope

**What it is.** A program loads a JSON Schema (a `boost::json::value`, or
JSON text read with exact numbers), the library compiles it once into an
immutable plan, and validates any number of instances against it, from any
number of threads, answering a boolean (`is_valid`) or a structured outcome
(`validate`) that renders as the specification's output formats.

**In scope:**

- dialects draft 4, draft 6, draft 7, 2019-09, 2020-12 and OpenAPI 3.0's
  Schema Object, load and emit (D2, S4, S9); v1 as one more policy type when
  it is published (S4);
- every vocabulary of 2020-12 and 2019-09 (core, applicator, unevaluated,
  validation, meta-data, format-annotation, format-assertion, content), the
  fixed keyword sets of drafts 4 to 7, `$vocabulary`, cross-draft references
  (S4);
- references: `$ref`, `$dynamicRef`, `$recursiveRef`, `$anchor`,
  `$dynamicAnchor`, `$recursiveAnchor`, embedded resources, a local
  registry, a user resolver, bundled meta-schemas (S5);
- annotations and `unevaluated*` (S6);
- the four output formats of 2020-12 section 12, a message hook, default
  English messages, an optional Boost.Locale header with a generated `.pot`
  (D4, S7);
- formats as annotations by default and as assertions on request, through
  `webcpp::lexical` (D10, S8); content assertion on request (S8);
- exact numbers (S3); the emitter (S9); a user vocabulary extension point
  (S4).

**Out of scope:**

- draft 3 (R 1.1);
- build-time compilation (code generation, constexpr schemas): the IR and the
  plan are what such a step would consume later (D9);
- any network, disk or clock: an unknown URI goes to the user's resolver
  (S5);
- applying `default` values or changing an instance in any way (R 6.3);
- v1's `list` and `hierarchical` output, until v1 lands (S7);
- joining Bowtie's public report, which needs the owner's go (S10).

**Consumers.** zod, after json_schema is implemented (D1): it builds IR in
code, compiles it, loads JSON Schema into IR (`fromJSONSchema`) and emits IR
(`toJSONSchema`). Section 11.4 lists what the emitter leaves room for.

## 2. Owner decisions

Every decision below is binding. D and S numbers are those of
`JSON_SCHEMA_DESIGN.md`; W marks a decision of `WEBCPP_DESIGN.md`'s "Owner
decisions after approval"; REL marks `WEBCPP_RELEASE_DESIGN.md`.

| Ref | Decision | Where this file applies it |
| --- | --- | --- |
| D1 | A library of webcpp's own aiming at the complete specification, proven by the official suite; zod comes after and reuses IR, loader, emitter, formats, validator | 1, 16, 20 |
| D2 | Every dialect is a policy type, machinery in `detail`; the developer picks the set at instantiation, `basic_validator<draft2020_12>` or several | 4.3, 4.5 |
| D3 | Each resource's `$schema` picks its dialect; a dialect outside the set refuses the load, naming it; no `$schema` means the first dialect of the list, an option overrides | 4.5, 6.1 |
| D4 | Output formats flag, basic, detailed, verbose as `boost::json`; a message hook in the validation options; default English; no global state; optional Boost.Locale header with a domain and a shipped `.pot`; json_schema stays header-only | 4.11, 9 |
| D5, D6 | Regex and URL engines behind template policies, with the 10% rule. D5's Boost.Regex default is superseded by D11, D6 is made precise by D12 (the design records both) | 4.2 |
| D7 | Blaze's ideas only, it is AGPL: no code | 7, 17 |
| D8 | Benchmark against Blaze and ajv with sourcemeta's jsonschema-benchmark; Bowtie as the compliance reference | 16.9, 17 |
| D9 | One engine compiled at runtime into an instruction list; `validate` only runs it; no separate interpreter; the suite is the oracle; build-time compilation out of scope | 7, 20 |
| D10 | `format` follows each dialect by default (annotation in 4 to 2020-12, assertion in v1); `format_assertion` forces assertion with the extensible set from the options; the suite runs both ways | 10 |
| D11 | Regex engine: `webcpp::regex`, whose default backend is SRELL through the mirror; the policy stays a template parameter, and its default, `webcpp_regex`, is the only one shipped | 4.2, 3 |
| D12 | URLs: Boost.URL, extended, never reimplemented; a program using the default URL policy links `/boost//url`; json_schema stays header-only | 4.2, 8 |
| S1 | Pipeline `boost::json -> load -> IR -> compile -> plan -> validate -> events -> output`, `IR -> emit(dialect)`; the components; policies grouped in one `traits`; `load`/`compile`/`validate` return `boost::system::result`; a throwing form only when exceptions are enabled | 3, 4 |
| S2 | `js::validator`, `js::validator_for<D...>`, `js::basic_validator<Traits, D...>`; `compile(doc, compile_options, allocator)` with designated initializers; the compiled schema self-contained, immutable, thread-safe; `is_valid` allocation-free on the common path; `validate(instance, validate_options, allocator)` returning an `outcome` with `errors()` and `to_json(format, storage_ptr)`; hook and resolver are function references; `schema->ir()`; `js::emit` | 4 |
| S3 | A pattern compiled once, at compile, through webcpp::regex, an invalid pattern fails compile, a runaway search is a validation error; Boost.URL as D12; locale header on every target with exceptions, refused by name under `WEBCPP_JSON_SCHEMA_NO_EXCEPTIONS`; numbers read exactly from text through a `basic_parser` handler, a `double` converted once, `Number` a parameter (`decimal64_t` default), integers exact, `multipleOf` exact | 4.2, 4.4, 12 |
| S4 | Vocabularies as building blocks in `detail`; the loader normalizes every draft into one IR; each resource carries its dialect; `$vocabulary` rules; unknown keywords annotate in 2019-09/2020-12, refused but `x-` in v1, option `unknown_keywords`; a public user-vocabulary extension point | 4.8, 5, 6 |
| S5 | Base URIs (`urn:webcpp:root` by default), anchors, `$ref` as a direct jump, `$dynamicRef` a runtime lookup only where needed, cycles that consume nothing refused at compile, a configurable instance depth limit with a measured default, a local registry, a synchronous resolver, bundled meta-schemas parsed when referenced, the suite's remotes registered, meta-schema validation at compile by default | 8 |
| S6 | A flat instruction list run by a small interpreter, operands as indices, keyword locations per instruction, iterative evaluation on an explicit stack, two modes by a template sink, the listed optimizations, static `unevaluated*` analysis with checkpoint and rewind, `call_user`, `explain()`; pratt not used | 7 |
| S7 | `js::error` fields, `error_kind` per assertion, `received` a pointer into the instance, the official output formats, lazy messages, a generated `.pot` (domain `webcpp_json_schema`, a context per kind) | 4.6, 9 |
| S8 | Format validators live in `webcpp::idna` and `webcpp::lexical`; json_schema's default set names `lexical::is_*`; content keywords annotate, an option asserts; the format set is extensible and local; an unknown format is an annotation | 10 |
| S9 | `js::emit(ir, dialect, options)` writes each dialect's forms, extracts shared nodes into `$defs`, cycles as `$ref`, deterministic order, carries annotations and user writers, `unrepresentable = error` or `drop`, a suite round trip, OpenAPI 3.0 as a dialect | 11 |
| S10 | Suite as a pinned submodule, a runner per dialect on every target, remotes registered, known failures that never lie, unit tests, allocation test, both exception modes, a 100,000-level hostile instance, libFuzzer, Bowtie harness, the benchmark run on demand on the owner's machine with `results.json` committed | 16, 17 |
| S11 | `config.hpp`, errors as values, `compile_or_throw` and the locale header behind the macro, pmr allocators, targets native, wasip2, wasip3, emscripten, header-only, names, the page, the changelog line, BSL-1.0, commits as R | 3, 13 to 19 |
| Bench | Benchmark results published on the generated page as a table and a chart, produced from a real run by a b2 target, never typed by hand | 17, 18 |
| S12 | Sub-tracks E1 to E5, the IR frozen after E1, E2 to E4 in parallel, E5 after, worktrees, serial superproject integration, at most 3 or 4 tracks at once, the order relative to the rest of webcpp | 20 |
| W alloc (2026-10-08) | Avoid dynamic allocation as far as the job allows; let the user customize the allocator of what is allocated; pmr compatible with Boost.JSON's `storage_ptr` | 14 |
| W exceptions (2026-10-08) | Every target builds with exceptions; a program's use of them is its developer's choice; wasip2 included | 13, 15 |
| W C3 (2026-10-08) | `WEBCPP_<LIB>_NO_EXCEPTIONS` in `config.hpp`, automatic under `BOOST_NO_EXCEPTIONS`; raise only through `boost::throw_exception`; every throwing API behind the macro | 13 |
| REL 1, 4, 6 | `<webcpp/config.hpp>` is a library of its own that every `config.hpp` includes; changelogs written with every change, Boost's format; no release until the owner asks | 13, 19 |

## 3. Layout and files

Developed inside the superproject as AGENTS.md chapter 1 says: until the
owner creates `webcpporg/json_schema`, `libs/json_schema` is a local
repository (`git init -b main`) in each sub-track's worktree (section 20).
One directory per component; a component that is wholly internal lives under
`detail/`.

```
libs/json_schema/
  build.jam                           project /webcpp/json_schema, alias json_schema
  meta/libraries.json                 "port-of": null, category ["Parsing", "Correctness"]
  meta/include-boundaries.json        section 3.1
  include/webcpp/json_schema.hpp      every public header but output/locale.hpp
  include/webcpp/json_schema/
    config.hpp                        WEBCPP_JSON_SCHEMA_NO_EXCEPTIONS
    errors.hpp                        errc, error_category, make_error_code, diagnostic, result
    traits.hpp                        default_traits, the traits concept
    json/parse.hpp                    parsed_json, parse: JSON text with exact numbers
    numbers/number.hpp                the number concept, number_traits, decomposed
    numbers/decimal.hpp               number_traits for decimal64_t and decimal128_t
    regex/regex_policy.hpp            the regex policy concept, match_result
    regex/webcpp_regex.hpp            webcpp_regex, the default policy, over webcpp::regex
    url/url_policy.hpp                the URL policy concept
    url/boost_url.hpp                 boost_url, the default policy
    dialects/dialect.hpp              enum dialect, the tags, the dialect_policy concept
    ir/ids.hpp                        the index types and range
    ir/number.hpp                     ir::number<Number>
    ir/keyword.hpp                    keyword_kind, the payloads, keyword
    ir/node.hpp                       type_set, node, resource, anchor
    ir/document.hpp                   basic_document: read, build, check
    ir/equivalent.hpp                 equivalent(a, b) (E4)
    registry/registry.hpp             basic_registry
    registry/resolver.hpp             resolver_ref
    vocabulary/vocabulary.hpp         basic_vocabulary, basic_user_keyword, user_call
    formats/format_set.hpp            format_entry, format_set
    formats/default_formats.hpp       default_formats(): webcpp::lexical (E5)
    validator/options.hpp             basic_compile_options, basic_validate_options
    validator/schema.hpp              basic_schema
    validator/validator.hpp           basic_validator, validator, validator_for
    output/error.hpp                  error_kind, error, error_details, annotation
    output/outcome.hpp                basic_outcome, output_format, collect
    output/messages.hpp               the English templates, basic_message_hook
    output/locale.hpp                 locale_messages: Boost.Locale (not in the convenience header)
    emitter/emit.hpp                  emit, emit_options, unrepresentable, emitted
    detail/json/                      equality, hashing, JSON Pointer, UTF-8, json_resource
    detail/dialects/                  keyword tables per vocabulary and dialect
    detail/loader/                    the walk and the normalization
    detail/meta_schemas/              generated headers holding the official meta-schemas
    detail/compiler/                  plan, opcodes, compile, analysis, optimizations
    detail/vm/                        machine, frames, the sink concept, the two sinks
    detail/output/                    the four renderers
    detail/emitter/                   the writer per dialect
  meta_schemas/                       the official meta-schemas as fetched, with README
                                      (URL, date, SHA-256 of each) and generate.py
  tools/make_pot.cpp, tools/Jamfile   prints the .pot from the template table
  i18n/webcpp_json_schema.pot         generated by tools/make_pot.cpp, checked by the build
  test/                               section 16
  example/                            the page's programs and their .expected
  bowtie/                             bowtie_webcpp.cpp, Dockerfile, README.md (16.8)
  benchmark/                          Jamfile, implementation/, results.json, report.py (17)
  doc/                                json_schema.adoc and its sections, mrdocs.yml,
                                      counts.py, benchmark.adoc and benchmark.svg (generated)
  README.md  AGENTS.md  LICENSE_1_0.txt  .gitignore  .gitattributes
  .github/workflows/ci.yml            library.yml with library: json_schema
```

Superproject files (changed serially, section 20.3): `.gitmodules` and
`third_party/libs/JSON-Schema-Test-Suite` (the suite, pinned, MIT; E1), and
what the WASI runner needs to reach the suite (section 15); `third_party/libs/srell`
is regex's dependency now, no longer json_schema's.

`build.jam`:

```
project /webcpp/json_schema ;

alias json_schema : : : :
    <include>include
    <library>/boost//headers
    <library>/webcpp//boost_json
    <library>/webcpp/regex//regex
    <library>/boost//url ;        # E2 on: the default URL policy (D12)
```

From E5 it adds `<library>/webcpp/lexical//lexical`. The Boost.Locale
header's link (`/boost//locale`) is required by its own test and by a
program that includes it, never by this alias.

### 3.1 Include boundaries (`meta/include-boundaries.json`)

| Headers | Must not include | Why |
| --- | --- | --- |
| `ir/*.hpp` | `webcpp/json_schema/detail/compiler`, `.../detail/vm`, `.../output`, `.../emitter`, `webcpp/regex`, `boost/url` | the IR is the contract; it knows no engine |
| `emitter/*.hpp`, `detail/emitter/*` | `.../detail/compiler`, `.../detail/vm`, `webcpp/regex` | emitting needs only the IR |
| everything but `output/locale.hpp` | `boost/locale` | only the locale header needs the compiled Boost.Locale |
| everything but `regex/webcpp_regex.hpp` | `webcpp/regex` | webcpp::regex is reached only through its policy |
| everything | `srell` | SRELL is reached only through webcpp::regex |
| everything but `url/boost_url.hpp` | `boost/url` | Boost.URL is reached only through its policy |
| everything but `formats/default_formats.hpp` | `webcpp/lexical` | formats is reached only through the default set |

## 4. Public API

`namespace js = webcpp::json_schema;` in the sketches, as examples alias it.
Every public symbol has a full Doc Comment (AGENTS.md chapter 7). Sketches
show signatures; bodies and `detail` are the implementation's.

### 4.1 Configuration and errors (`config.hpp`, `errors.hpp`)

`config.hpp` includes `<webcpp/config.hpp>` once the config library exists
and `<boost/config.hpp>` until then (one line to change, S12), and defines
`WEBCPP_JSON_SCHEMA_NO_EXCEPTIONS` exactly as pratt's does (AGENTS.md
chapter 6), with the `__MRDOCS__` definition for the reference.

```cpp
namespace webcpp::json_schema {

/** Why a load, a compile, an emit or a rendering failed. Values are fixed forever. */
enum class errc : int {
    not_a_schema = 1,           // a value where a schema must be is neither object nor boolean
                                // (or a boolean in draft 4 and OpenAPI 3.0)
    invalid_keyword_value = 2,  // a known keyword's value has the wrong shape
    meta_schema_violation = 3,  // the schema fails its meta-schema (validate_schema on)
    unknown_dialect = 4,        // $schema names a dialect outside the validator's set
    unknown_vocabulary = 5,     // a required vocabulary the validator does not know
    unknown_keyword = 6,        // a keyword refused by unknown_keywords::reject, v1 or OpenAPI 3.0
    invalid_uri = 7,            // an $id, $ref or base URI that is not a URI reference
    unresolvable_reference = 8, // no resource for a reference's URI
    resolver_failed = 9,        // the resolver returned an error for a URI
    invalid_json_pointer = 10,  // a fragment that is not a JSON Pointer, or points at nothing
    duplicate_resource = 11,    // two resources with one URI
    duplicate_anchor = 12,      // two anchors with one name in one resource
    reference_cycle = 13,       // a cycle of references that consumes no instance
    invalid_regex = 14,         // a pattern the regex policy refuses
    schema_too_deep = 15,       // the schema nests past max_schema_depth
    invalid_number = 16,        // a number the Number type cannot hold
    invalid_ir = 17,            // a hand-built IR breaks an invariant of section 5.3
    unrepresentable = 18,       // emit: the target dialect cannot express a node
    output_not_collected = 19,  // to_json: a format needs more than validate collected
    vocabulary_conflict = 20,   // a user keyword's name is taken in a resource
};

const boost::system::error_category& error_category() noexcept;   // "webcpp.json_schema"
boost::system::error_code make_error_code(errc e) noexcept;

/** A failure with the place it happened: an error code, where, and what. No allocation. */
struct diagnostic {
    boost::system::error_code code;
    boost::static_strings::static_string<256> where;  // a keyword location or a URI, cut with "..."
    boost::static_strings::static_string<256> what;   // the dialect, keyword or detail named
};

/** What every operation that can fail returns. */
template <class T>
using result = boost::system::result<T, diagnostic>;

/** Raises a diagnostic as boost::system::system_error, for result::value(). */
[[noreturn]] void throw_exception_from_error(const diagnostic& d,
                                             const boost::source_location& loc);

}  // namespace webcpp::json_schema
```

The category follows pratt's form: a `constexpr` category with a fixed
64-bit id (`"jschema"` in ASCII, `0x6a736368656d6100`), its name, the
`is_error_code_enum` specialization. `diagnostic` carries the place the
design asks every refusal to name (D3, S5, S9) without allocating, so a
failure under an exhausted allocator still reports.

### 4.2 Traits and policies (`traits.hpp`, `numbers/`, `regex/`, `url/`)

```cpp
namespace webcpp::json_schema {

struct default_traits {
    using number_type = boost::decimal::decimal64_t;
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    using regex_policy = webcpp_regex;
    using url_policy = boost_url<>;
};

template <class T>
concept traits = number<typename T::number_type>
              && regex_policy<typename T::regex_policy, typename T::allocator_type,
                              typename T::allocator_type>
              && url_policy<typename T::url_policy>
              && /* an allocator of std::byte */ true;

}
```

**Number** (`numbers/number.hpp`; section 12):

```cpp
struct uint128 { std::uint64_t high; std::uint64_t low; };   // portable: MSVC has no __int128

struct decomposed {               // |value| = coefficient * 10^exponent
    bool negative;
    uint128 coefficient;          // decimal64_t's fits in low; decimal128_t's needs 113 bits
    std::int32_t exponent;
};

template <class N> struct number_traits;   // the customization point

template <class N>
concept number = std::regular<N> && std::totally_ordered<N>
    && std::constructible_from<N, std::int64_t> && std::constructible_from<N, std::uint64_t>
    && requires(N n, std::string_view text) {
        { number_traits<N>::from_chars(text) } -> std::same_as<boost::system::result<N>>;
        { number_traits<N>::decompose(n) } -> std::same_as<decomposed>;
        { number_traits<N>::is_integer(n) } -> std::same_as<bool>;
        { number_traits<N>::to_chars(std::span<char>{}, n) } -> std::same_as<std::size_t>;
    };
```

`numbers/decimal.hpp` specializes `number_traits` for
`boost::decimal::decimal64_t` (the default) and `decimal128_t` (the larger
`Number` for bignums), through Boost.Decimal's `from_chars`, `frexp10`,
`trunc` and `to_chars`. A user specializes it for another type. No
floating-point specialization ships (S3).

**Regex** (`regex/regex_policy.hpp`, `regex/webcpp_regex.hpp`):

```cpp
enum class match_result : std::uint8_t { found, not_found, limit };  // limit: a runaway search

// SchemaAlloc: the schema's allocator (allocator_type); ValidationAlloc: the
// validation's, the one validate() and is_valid() are given.
template <class R, class SchemaAlloc, class ValidationAlloc>
concept regex_policy = requires(std::string_view text, const SchemaAlloc& schema_allocator,
                                const ValidationAlloc& validation_allocator,
                                std::size_t step_budget,
                                const typename R::template compiled<SchemaAlloc>& re) {
    // unanchored ECMA-262 u mode on UTF-8; an error is errc::invalid_regex
    { R::compile(text, schema_allocator, step_budget) }
        -> std::same_as<boost::system::result<typename R::template compiled<SchemaAlloc>>>;
    // text is well-formed UTF-8 (Boost.JSON validated it)
    { R::search(re, text, validation_allocator) } -> std::same_as<match_result>;
};

struct webcpp_regex {
    template <class Allocator>   // the schema's allocator, rebound to char
    using compiled = webcpp::regex::basic_regex<char,
        typename std::allocator_traits<Allocator>::template rebind_alloc<char>>;

    // compile: compiled<A>::compile(text, {.step_budget = step_budget}, rebound allocator);
    //     too_deep and every other error are errc::invalid_regex at the keyword's location.
    // search: webcpp::regex::search(text, m, re, match_valid_utf8) with
    //     match_results<const char*, the validation's allocator rebound>; found and
    //     not_found as the result; error::complexity and error::stack are
    //     match_result::limit; no other error can come from a search.
};
```

Where the arguments come from: `compile(doc, compile_options, allocator)`
passes its `allocator` and `compile_options.regex_step_budget` to
`R::compile` for every `pattern` and `patternProperties` key;
`validate(instance, validate_options, allocator)` and `is_valid` pass their
`allocator` to `R::search`. `traits` (4.2) checks
`regex_policy<regex_policy, allocator_type, allocator_type>`, since both
allocators are of `allocator_type`. A compiled regex is constructed in
place in the schema's storage and moved, never copied plainly, so it stays
on the schema's resource (regex spec 4.6, N4). Strings reach `search` from
Boost.JSON, which has validated their UTF-8, so the policy passes
`match_valid_utf8` (regex P17).

With the default `allocator_type`, `std::pmr::polymorphic_allocator<std::byte>`,
`compiled<...>` is `webcpp::regex::regex` itself. A pattern is compiled once,
at schema compile; `compile` failing is `errc::invalid_regex` naming the
pattern's keyword location (S3). A `limit` is a validation error of kind
`pattern_limit`, never an abort. SRELL's macros are usage requirements of
`/webcpp/third_party//srell`, which only `/webcpp/regex//regex` names;
json_schema names neither.

**URL** (`url/url_policy.hpp`, `url/boost_url.hpp`):

```cpp
template <class U>
concept url_policy = requires(std::string_view base, std::string_view ref,
                              boost::static_strings::static_string<U::capacity>& out) {
    // RFC 3986 section 5.2 resolution and section 6.2.2 normalization, fragment kept apart
    { U::resolve(base, ref, out) } -> std::same_as<boost::system::result<void>>;
    { U::is_absolute(ref) } -> std::same_as<bool>;
};

template <std::size_t Capacity = 4096>
struct boost_url { static constexpr std::size_t capacity = Capacity; /* boost::urls::
    parse_uri_reference, resolve into a static_url<Capacity>, normalize */ };
```

Resolution writes into a fixed buffer on the stack, then into the schema's
allocator; Boost.URL's own types never allocate on the heap here. A URI
longer than the capacity is `errc::invalid_uri`.

### 4.3 Dialects (`dialects/dialect.hpp`)

```cpp
enum class dialect : std::uint8_t {   // values fixed forever; v1 appended when published
    draft4 = 1, draft6 = 2, draft7 = 3, draft2019_09 = 4, draft2020_12 = 5, openapi3_0 = 6,
};

struct draft4       { static constexpr dialect id = dialect::draft4; };
struct draft6       { static constexpr dialect id = dialect::draft6; };
struct draft7       { static constexpr dialect id = dialect::draft7; };
struct draft2019_09 { static constexpr dialect id = dialect::draft2019_09; };
struct draft2020_12 { static constexpr dialect id = dialect::draft2020_12; };
struct openapi3_0   { static constexpr dialect id = dialect::openapi3_0; };

namespace detail {
template <class D> struct dialect_machinery;          // specialized in detail/dialects/<d>.hpp
template <class D> inline constexpr bool has_machinery = false;
template <> inline constexpr bool has_machinery<draft2020_12> = true;   // one line per dialect
}

template <class D>
concept dialect_policy = requires { { D::id } -> std::convertible_to<dialect>; }
                      && detail::has_machinery<D>;

/** The dialect's $schema URI; empty for OpenAPI 3.0, which no $schema names. */
constexpr std::string_view uri_of(dialect d) noexcept;
```

The tags are empty; each one's keyword table and normalization live in
`detail::dialect_machinery<D>` (D2). A future dialect is one more tag, one
more machinery, and its line of `has_machinery`. The concept reads
`has_machinery`, stated beside the tags, never the machinery itself, so its
answer cannot depend on which headers a translation unit included before
asking ([temp.expl.spec]/7, [temp.constr.atomic]/3); `basic_validator`
asserts that each of its dialects' machinery is defined. OpenAPI 3.0 is chosen only by `default_dialect` or by
standing first in the list, since its Schema Object carries no `$schema`.

### 4.4 Exact JSON text (`json/parse.hpp`)

```cpp
template <class Traits = default_traits>
struct basic_parsed_json {
    boost::json::value value;              // as Boost.JSON stores it, number_precision::precise
    /* detail: the exact number of every number whose text the value's double does not
       reproduce, keyed by its JSON Pointer; empty for most documents */
};
using parsed_json = basic_parsed_json<>;

template <class Traits = default_traits>
result<basic_parsed_json<Traits>> parse(std::string_view text,
                                        typename Traits::allocator_type allocator = {},
                                        boost::json::parse_options options = {});
```

`parse` runs a `boost::json::basic_parser` handler that builds the value and
reads each number's lexeme into `Number` (S3). Where the double's shortest
round-trip text denotes the same decimal value as the lexeme (most numbers),
nothing is recorded; otherwise the exact `Number` is recorded under the
number's JSON Pointer, and the loader prefers it. Duplicate keys keep the
last, in the value and in the table alike.

### 4.5 `basic_validator` and its options (`validator/`)

```cpp
enum class unknown_keywords : std::uint8_t {
    dialect_default,   // annotate in 2019-09 and 2020-12, keep inert in 4 to 7, reject in
                       // OpenAPI 3.0 (but x-), reject in v1 (but x-)
    annotate, ignore, reject,
};

template <class Traits>
struct basic_compile_options {
    basic_registry<Traits>* registry = nullptr;      // the user's resources; caches meta-schemas
    std::optional<resolver_ref> resolver;            // asked for a URI nothing else knows
    std::string_view base_uri;                       // the root's retrieval URI; else urn:webcpp:root
    std::optional<dialect> default_dialect;          // else the first of the validator's list
    bool format_assertion = false;                   // D10
    format_set formats = {};                         // E5: default_formats()
    bool content_assertion = false;                  // S8
    std::size_t regex_step_budget = 0;                // 0: the backend's default; unit is the
                                                      // configured backend's (14)
    unknown_keywords unknown = unknown_keywords::dialect_default;
    bool compatibility_keywords = true;              // section 6.2
    bool validate_schema = true;                     // S5: check against the meta-schema
    std::span<const basic_vocabulary<Traits>> vocabularies = {};
    std::size_t max_schema_depth = default_max_schema_depth;
};

enum class collect : std::uint8_t { errors, errors_and_annotations, everything };

template <class Traits>
struct basic_validate_options {
    std::optional<basic_message_hook<Traits>> messages;   // else the English templates
    collect what = collect::errors;
    std::size_t max_depth = default_max_depth;            // instance nesting, S5
};

template <class Traits, class... Dialects>
    requires traits<Traits> && (sizeof...(Dialects) > 0) && (dialect_policy<Dialects> && ...)
class basic_validator final {
public:
    using traits_type = Traits;
    using allocator_type = typename Traits::allocator_type;
    using document_type = ir::basic_document<Traits>;
    using schema_type = basic_schema<Traits>;
    using compile_options = basic_compile_options<Traits>;

    static constexpr std::array<dialect, sizeof...(Dialects)> dialects{Dialects::id...};

    result<document_type> load(const boost::json::value& schema,
                               const compile_options& options = {},
                               allocator_type allocator = {}) const;
    result<document_type> load(const basic_parsed_json<Traits>& schema,
                               const compile_options& options = {},
                               allocator_type allocator = {}) const;

    result<schema_type> compile(const boost::json::value& schema,
                                const compile_options& options = {},
                                allocator_type allocator = {}) const;
    result<schema_type> compile(const basic_parsed_json<Traits>& schema,
                                const compile_options& options = {},
                                allocator_type allocator = {}) const;
    result<schema_type> compile(const document_type& document,   // zod's path: built IR
                                const compile_options& options = {},
                                allocator_type allocator = {}) const;

#ifndef WEBCPP_JSON_SCHEMA_NO_EXCEPTIONS
    schema_type compile_or_throw(const boost::json::value& schema,
                                 const compile_options& options = {},
                                 allocator_type allocator = {}) const;
#endif
};

using validator = basic_validator<default_traits, draft2020_12, draft2019_09, draft7, draft6,
                                  draft4, openapi3_0>;
template <class... Dialects>
using validator_for = basic_validator<default_traits, Dialects...>;
```

`regex_step_budget` is passed as `options::step_budget` to every `pattern`
and `patternProperties` key's regex, documented with the unit the
configured backend honours, so that no schema author reads it as a bound it
is not: on SRELL until regex's M6, backtracking failures of groups and
alternatives from one start position, each up to a quantifier's span; on
SRELL after M6, every failure and every position a quantifier gives back,
per search (`honours<>::step_budget_per_search`); on Boost.Regex, iterator
steps per search.

A validator holds no state: it is the set of dialects, as a type, and it is
`final`. The header spells each `= {}` default of `compile_options` as
`= compile_options()`, the same value: clang-tidy's
`readability-trailing-comma` reads the comma after `= {}` in a parameter list
as an initializer list's. `compile`
is `load` then the compiler; compiling a `document_type` first runs its
`check()` (section 5.4), so a hand-built IR's fault is `errc::invalid_ir`, a
value. A `default_dialect` outside the set is `errc::unknown_dialect`.

### 4.6 `basic_schema`, `outcome`, `error` (`validator/schema.hpp`, `output/`)

```cpp
template <class Traits>
class basic_schema {           // a shared handle on the immutable compiled schema
public:
    using allocator_type = typename Traits::allocator_type;
    using validate_options = basic_validate_options<Traits>;
    using outcome_type = basic_outcome<Traits>;

    /** Short-circuits, records nothing; allocates only past the inline stack. */
    bool is_valid(const boost::json::value& instance, const validate_options& options = {},
                  allocator_type allocator = {}) const;

    /** Collects what options.what asks; fails only with a user keyword's own error. */
    result<outcome_type> validate(const boost::json::value& instance,
                                  const validate_options& options = {},
                                  allocator_type allocator = {}) const;

    const ir::basic_document<Traits>& ir() const noexcept;

    /** The plan, one instruction per line, with each one's schema location (E5). */
    basic_string<Traits> explain(allocator_type allocator = {}) const;

    allocator_type get_allocator() const noexcept;
};

enum class output_format : std::uint8_t { flag, basic, detailed, verbose };

template <class Traits>
class basic_outcome {          // keeps the schema alive; the instance must outlive it
public:
    bool valid() const noexcept;
    std::span<const error> errors() const noexcept;
    std::span<const annotation> annotations() const noexcept;   // E2
    basic_string<Traits> message(const error& e) const;          // the hook's or the template's
    result<boost::json::value> to_json(output_format format,
                                       boost::json::storage_ptr storage = {}) const;   // E3
};
using outcome = basic_outcome<default_traits>;

enum class error_kind : std::uint16_t {    // values fixed forever; appended only
    false_schema = 1, type = 2, enum_value = 3, const_value = 4,
    multiple_of = 5, maximum = 6, exclusive_maximum = 7, minimum = 8, exclusive_minimum = 9,
    max_length = 10, min_length = 11, pattern = 12, pattern_limit = 13, format = 14,
    max_items = 15, min_items = 16, unique_items = 17,
    contains = 18, min_contains = 19, max_contains = 20,
    max_properties = 21, min_properties = 22, required = 23, dependent_required = 24,
    properties = 25, pattern_properties = 26, additional_properties = 27,
    property_names = 28, dependent_schemas = 29, prefix_items = 30, items = 31,
    unevaluated_items = 32, unevaluated_properties = 33,
    all_of = 34, any_of = 35, one_of = 36, not_schema = 37, if_then_else = 38,
    reference = 39,          // $ref, $dynamicRef, $recursiveRef: a subschema had errors
    content = 40, schema = 41 /* a schema object had errors */, user = 42, depth_limit = 43,
};

struct error_details {         // one alternative per kind that has more to say
    std::variant<std::monostate,
                 std::span<const std::string_view>,   // required, dependent_required:
                                                      // the missing names; additional_*,
                                                      // unevaluated_*: the offending names
                 std::span<const std::size_t>,        // unique_items: the two equal indices;
                                                      // one_of: the matching indices
                 std::uint64_t,                       // length, item, property or match count
                 boost::system::error_code>           // user: the keyword's own error
        value;
};

class error {                  // views into the outcome's and the schema's storage
public:
    error_kind kind() const noexcept;
    std::string_view keyword() const noexcept;                    // as written: "maxItems"
    std::string_view instance_location() const noexcept;          // JSON Pointer
    std::string_view keyword_location() const noexcept;           // evaluation path, $ref segments
    std::string_view absolute_keyword_location() const noexcept;  // canonical URI + pointer
    const boost::json::value& expected() const noexcept;          // the keyword's value
    const boost::json::value& received() const noexcept;          // into the instance (S7)
    const error_details& details() const noexcept;
};

class annotation {
public:
    std::string_view keyword() const noexcept;
    std::string_view instance_location() const noexcept;
    std::string_view keyword_location() const noexcept;
    std::string_view absolute_keyword_location() const noexcept;
    const boost::json::value& value() const noexcept;   // the schema's constant, or computed
                                                        // (a properties name set) in the outcome
};
```

`basic_string<Traits>` is `std::basic_string<char, std::char_traits<char>,`
the traits' allocator rebound to `char>`: `std::pmr::string` by default.
`expected()` refers to the keyword's value in the compiled schema, which the
outcome keeps alive; `received()` points into the instance, which must
outlive the outcome (S7, documented); `to_json` copies both into the report.

### 4.7 Registry and resolver (`registry/`)

```cpp
/** The user's synchronous lookup of a URI; never the library's network or disk. */
using resolver_ref = boost::compat::function_ref<
    boost::system::result<boost::json::value>(std::string_view uri,
                                              const boost::json::storage_ptr& storage)>;

template <class Traits>
class basic_registry {
public:
    using allocator_type = typename Traits::allocator_type;
    explicit basic_registry(allocator_type allocator = {});

    /** Registers a document under its retrieval URI; copies it into the registry. */
    result<void> add(std::string_view uri, const boost::json::value& document);
    result<void> add(std::string_view uri, const basic_parsed_json<Traits>& document);

    bool contains(std::string_view uri) const noexcept;
    allocator_type get_allocator() const noexcept;
};
using registry = basic_registry<default_traits>;
```

The resolver receives the storage to build its value in, so its allocation
goes through the schema's allocator too (W alloc). Lookup order, the
registry's cache and the lifetime rule are section 8.

### 4.8 User vocabularies (`vocabulary/vocabulary.hpp`)

```cpp
struct user_call {
    const boost::json::value& value;      // the keyword's value in the schema
    const boost::json::value& instance;   // the instance location it applies to
};

template <class Traits>
struct basic_user_keyword {
    std::string_view name;
    /** Checks the keyword's value at load; an error refuses the load, naming the keyword. */
    boost::compat::function_ref<boost::system::result<void>(const boost::json::value&)> check;
    /** Decides an instance: true when it satisfies the keyword. */
    boost::compat::function_ref<boost::system::result<bool>(const user_call&)> validate;
    ir::type_set applies_to = ir::type_set::all();   // other instance types pass untouched
    bool annotates = false;                          // a passing keyword annotates its value
    /** Writes the keyword for a target dialect; without it the emitter copies the value. */
    std::optional<boost::compat::function_ref<boost::system::result<boost::json::value>(
        const boost::json::value&, dialect, const boost::json::storage_ptr&)>> write;
};

enum class activation : std::uint8_t {
    always,          // recognized in every resource
    when_declared,   // only where the resource's meta-schema lists the vocabulary's URI
};

template <class Traits>
struct basic_vocabulary {
    std::string_view uri;
    std::span<const basic_user_keyword<Traits>> keywords;
    activation active = activation::always;
};
```

A user keyword loads into the IR as `keyword_kind::user`, compiles to
`call_user`, and works as an official keyword: errors of kind `user`,
annotations, `$vocabulary` (S4). Its value is data, never a schema: a user
keyword is an assertion or an annotation, not an applicator. Its functions
must outlive every schema compiled with it (documented, with the format
set's identical rule). `validate` returning an error is an operational
failure: `validate()` returns it as the result's error, with the keyword's
location; `is_valid` answers `false`. A name that an active official keyword
or another user keyword already takes is `errc::vocabulary_conflict`.

### 4.9 Format set (`formats/`)

```cpp
struct format_entry {
    std::string_view name;
    boost::compat::function_ref<bool(std::string_view) noexcept> check;
};
using format_set = std::span<const format_entry>;

/** Every format of every dialect, each naming webcpp::lexical (E5). */
format_set default_formats() noexcept;    // in formats/default_formats.hpp
```

Section 10 says how the set is used.

### 4.10 Emitter (`emitter/emit.hpp`)

```cpp
enum class unrepresentable : std::uint8_t {
    error,   // errc::unrepresentable, naming the node's location (default)
    drop,    // omitted, and recorded at that node as "x-webcpp-dropped": [keyword names]
};

struct emit_options {
    unrepresentable on_unrepresentable = unrepresentable::error;
    bool write_schema_keyword = true;          // $schema at the root (none for OpenAPI 3.0)
    boost::json::storage_ptr storage = {};
};

struct emitted {
    boost::json::value schema;
    boost::json::object components;   // OpenAPI 3.0 only: the shared nodes, which the
                                      // caller places under components/schemas
};

template <class Traits>
result<emitted> emit(const ir::basic_document<Traits>& document, dialect target,
                     const emit_options& options = {});
template <class Traits>
result<emitted> emit(const ir::basic_document<Traits>& document, ir::node_id root,
                     dialect target, const emit_options& options = {});
```

### 4.11 Messages and locale (`output/messages.hpp`, `output/locale.hpp`)

```cpp
template <class Traits>
using basic_message_hook =
    boost::compat::function_ref<void(const error& e, basic_string<Traits>& out)>;

// output/locale.hpp, outside the convenience header:
#ifdef WEBCPP_JSON_SCHEMA_NO_EXCEPTIONS
#error "<webcpp/json_schema/output/locale.hpp> needs exceptions: Boost.Locale reports errors by throwing"
#endif
template <class Traits = default_traits>
class locale_messages {        // a ready hook: translate(context = kind, template), format
public:
    explicit locale_messages(const std::locale& locale);
    void operator()(const error& e, basic_string<Traits>& out) const;
};
```

The design's "a hook writing into a `std::string&`" is kept as a
`std::basic_string` of the traits' allocator, so the hook's text lands in
the validation's allocator.

### 4.12 A first program

```cpp
namespace js = webcpp::json_schema;

js::validator validator;
auto schema = validator.compile(boost::json::parse(R"({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "object",
    "required": ["name"]
})"));
if (!schema) { /* schema.error().code, .where, .what */ }

boost::json::value instance = boost::json::parse(R"({"age": 3})");
bool ok = schema->is_valid(instance);
auto outcome = schema->validate(instance);
auto report = outcome->to_json(js::output_format::basic);
```

## 5. The IR, a contract

The IR is the contract between the sub-tracks: frozen at the end of E1 (S12),
consumed by the compiler (E2), the output's locations (E3), the emitter (E4)
and, later, zod. Namespace `webcpp::json_schema::ir`.

### 5.1 Shape

```cpp
namespace webcpp::json_schema::ir {

enum class node_id : std::uint32_t {};
enum class keyword_id : std::uint32_t {};
enum class resource_id : std::uint32_t {};
enum class string_id : std::uint32_t {};
enum class value_id : std::uint32_t {};
enum class number_id : std::uint32_t {};
enum class pointer_id : std::uint32_t {};   // a JSON Pointer: the index of its last step
inline constexpr node_id no_node{0xFFFF'FFFFu};
inline constexpr value_id no_value{0xFFFF'FFFFu};
inline constexpr pointer_id empty_pointer{0xFFFF'FFFFu};   // "", where every chain starts

/** A slice of one of the document's side tables. */
struct range { std::uint32_t first = 0; std::uint32_t count = 0; };

/** One step of a JSON Pointer: the pointer it extends, then a member or an element. */
struct pointer_step {
    pointer_id parent = empty_pointer;   // empty_pointer, or a step before this one
    string_id name;                      // the member's name, as written, not escaped
    std::uint32_t index = 0;             // the element's index
    bool element = false;                // an element; else a member
};

/** A set of JSON types; integer is its own bit (validation 6.1.1). */
class type_set {
public:
    enum bit : std::uint8_t { null = 1, boolean = 2, object = 4, array = 8,
                              number = 16, string = 32, integer = 64 };
    static constexpr type_set all() noexcept;
    constexpr bool contains(bit b) const noexcept;
    std::uint8_t bits;
};

/** A number operand, exact: an integer stays an integer. */
template <class Number>
using number = std::variant<std::int64_t, std::uint64_t, Number>;

enum class node_kind : std::uint8_t { always_valid = 1 /* true */, never_valid = 2 /* false */,
                                      object = 3 };

struct node {
    node_kind kind;
    resource_id resource;   // the schema resource it belongs to
    pointer_id pointer;     // its JSON Pointer from the resource's root (empty_pointer for the root)
    range keywords;         // its keywords, contiguous in keywords(), in source order
};

struct resource {
    string_id uri;               // absolute, normalized, without fragment
    dialect source_dialect;      // for the emitter, $schema in output, explain: never semantics
    node_id root;
    resource_id parent;          // lexically enclosing resource; itself for the root
    std::uint16_t vocabularies;  // bits: core, applicator, unevaluated, validation, meta_data,
                                 // format_annotation, format_assertion, content (record only)
    bool recursive_anchor;       // 2019-09 "$recursiveAnchor": true at its root
};

struct anchor {
    resource_id resource;
    string_id name;
    node_id target;
    bool dynamic;                // $dynamicAnchor (also a plain anchor), or $recursiveAnchor
};

}  // namespace webcpp::json_schema::ir
```

### 5.2 Keywords

```cpp
enum class dynamic_form : std::uint8_t { dynamic /* $dynamicRef */, recursive /* $recursiveRef */ };

struct subschema        { node_id schema; };
struct subschema_list   { range nodes; };                  // node_id
struct named_subschema  { string_id name; node_id schema; };
struct named_subschemas { range entries; };                // named_subschema, source order
struct reference        { node_id target; string_id uri; };       // uri as written
struct dynamic_reference {
    node_id initial_target;     // the static resolution
    string_id uri;              // as written
    string_id anchor;           // the fragment's name; "" for $recursiveRef
    dynamic_form form;
    bool bookended;             // the initial target carries the matching dynamic anchor
                                // (2020-12) or is a resource root with $recursiveAnchor (2019-09);
                                // false compiles to a plain jump (S5)
};
struct types            { type_set set; bool integer_by_value; };  // false in draft 4 only
struct constant         { value_id value; };
struct constants        { range values; };                 // value_id
struct bound            { number_id value; };
struct count            { std::uint64_t value; };
struct text             { string_id value; };
struct format_payload   { string_id name; bool assertion; };       // per dialect and vocabulary
struct names            { range strings; };                // string_id, as written
struct dependency       { string_id name; range strings; };
struct dependencies     { range entries; };                // dependency
struct contains_payload { node_id schema; bool annotates_items; };  // true from 2020-12
struct user_payload     { std::uint16_t vocabulary; std::uint16_t keyword; };

using payload = std::variant<std::monostate, subschema, subschema_list, named_subschemas,
                             reference, dynamic_reference, types, constant, constants, bound,
                             count, text, format_payload, names, dependencies,
                             contains_payload, user_payload>;

struct keyword {
    keyword_kind kind;
    string_id name;      // as written in the source ("items", "additionalItems", "definitions");
                         // the canonical name when built in code
    value_id source;     // the keyword's value as written, or no_value when built in code
    payload data;        // the alternative the table below gives for kind
};
```

`keyword_kind`, its values (fixed from E1 on, appended only), and the payload
each one holds:

| Value | Kind | Payload | Notes |
| --- | --- | --- | --- |
| 1 | `ref` | `reference` | resolved (5.3, 2) |
| 2 | `dynamic_ref` | `dynamic_reference` | `$dynamicRef`, `$recursiveRef` |
| 3 | `defs` | `named_subschemas` | `$defs`, `definitions` |
| 4 | `comment` | `constant` | `$comment` |
| 10 | `all_of` | `subschema_list` | |
| 11 | `any_of` | `subschema_list` | |
| 12 | `one_of` | `subschema_list` | |
| 13 | `not_` | `subschema` | |
| 14 | `if_` | `subschema` | |
| 15 | `then_` | `subschema` | kept without `if_`, compiled to nothing |
| 16 | `else_` | `subschema` | likewise |
| 17 | `dependent_schemas` | `named_subschemas` | |
| 18 | `prefix_items` | `subschema_list` | `prefixItems`, or array `items` before 2020-12 |
| 19 | `items` | `subschema` | 2020-12 `items`, schema `items` before, or `additionalItems` |
| 20 | `contains` | `contains_payload` | |
| 21 | `properties` | `named_subschemas` | |
| 22 | `pattern_properties` | `named_subschemas` | the name is the pattern |
| 23 | `additional_properties` | `subschema` | |
| 24 | `property_names` | `subschema` | |
| 30 | `unevaluated_items` | `subschema` | |
| 31 | `unevaluated_properties` | `subschema` | |
| 40 | `type` | `types` | |
| 41 | `enum_` | `constants` | |
| 42 | `const_` | `constant` | |
| 43 | `multiple_of` | `bound` | greater than zero |
| 44 | `maximum` | `bound` | |
| 45 | `exclusive_maximum` | `bound` | draft 4's boolean form normalized (6.2) |
| 46 | `minimum` | `bound` | |
| 47 | `exclusive_minimum` | `bound` | |
| 48 | `max_length` | `count` | |
| 49 | `min_length` | `count` | |
| 50 | `pattern` | `text` | |
| 51 | `max_items` | `count` | |
| 52 | `min_items` | `count` | |
| 53 | `unique_items` | `constant` | the boolean as written |
| 54 | `max_contains` | `count` | |
| 55 | `min_contains` | `count` | |
| 56 | `max_properties` | `count` | |
| 57 | `min_properties` | `count` | |
| 58 | `required` | `names` | |
| 59 | `dependent_required` | `dependencies` | |
| 60 | `format` | `format_payload` | |
| 61 | `content_encoding` | `text` | |
| 62 | `content_media_type` | `text` | |
| 63 | `content_schema` | `subschema` | |
| 70 | `title` | `constant` | |
| 71 | `description` | `constant` | |
| 72 | `default_` | `constant` | |
| 73 | `deprecated` | `constant` | |
| 74 | `read_only` | `constant` | |
| 75 | `write_only` | `constant` | |
| 76 | `examples` | `constant` | |
| 80 | `discriminator` | `constant` | OpenAPI 3.0 annotation |
| 81 | `example` | `constant` | OpenAPI 3.0 annotation |
| 82 | `xml` | `constant` | OpenAPI 3.0 annotation |
| 83 | `external_docs` | `constant` | OpenAPI 3.0 annotation |
| 90 | `unknown` | `constant` | an unknown keyword; annotation or nothing by the option |
| 91 | `ignored` | `constant` | inert: kept for the emitter, never compiled (`$ref` siblings up to draft 7, draft 4's `exclusiveMaximum: false`, `additionalItems` beside schema `items`, `nullable` without `type`) |
| 92 | `user` | `user_payload` | a user vocabulary's keyword |

### 5.3 Invariants

Numbered so code, tests and reviews cite them (`[ir-3]`); a number is never
reused. `basic_document::check()` verifies 1 to 11, 13 and 15.

1. **Closed.** Every `node_id`, `string_id`, `value_id`, `number_id`,
   `resource_id`, `pointer_id` and `range` names an entry of the same
   document (`empty_pointer` included). The root is set. Every reserved node
   is defined exactly once.
2. **References resolved.** Every `ref` has a target node and every
   `dynamic_ref` an initial target; no URI is looked up after load. Their
   `uri` strings serve only the emitter, `explain` and the output. The IR is
   self-contained: it holds every resource the schema reaches, and nothing
   else (S5, "the compiled schema copies what it used").
3. **Dialect-neutral.** No consumer reads `resource::source_dialect` or
   `resource::vocabularies` for semantics. Every semantic difference between
   dialects is an explicit field: `types::integer_by_value`,
   `contains_payload::annotates_items`, `format_payload::assertion`,
   `dynamic_reference::form` and `bookended`, the `ignored` kind for
   siblings a dialect ignores, and keywords a resource's vocabularies switch
   off being loaded as `unknown`. The dialect stays only for the emitter,
   `$schema` in output, and `explain`.
4. **One keyword per kind per node,** except `unknown`, `ignored` and `user`,
   which may repeat with distinct names.
5. **Order.** A node's keywords keep the source order (loader) or insertion
   order (builder); every list (`properties`, `required`, `enum`) keeps its
   order. The emitter's order follows (section 11).
6. **Names.** The entries of one `named_subschemas` have distinct names.
7. **Numbers.** Every `bound` is exact: an integer that fits stays `int64_t`
   or `uint64_t`, any other value is a `Number` read from the lexeme when the
   schema came from `parse`, else from the double's shortest round-trip text
   (S3). `multiple_of` is greater than zero. `count` values are integers
   (draft 6 on accepts `2.0`).
8. **Text.** Strings are valid UTF-8.
9. **Resources.** Resource 0 is the root's. Each has an absolute, normalized
   base URI without fragment (`urn:webcpp:root` when none is given), unique
   in the document. Each node belongs to exactly one resource; a resource's
   root node has pointer `empty_pointer`.
10. **Anchors.** A plain name is unique within its resource, and so is a
    dynamic one; the target belongs to the anchor's resource.
11. **Values.** Every value lives in the document's storage and is immutable
    once loaded: it is owned, or named at a pointer inside a value that comes
    before it, owned or named so, where that pointer resolves.
12. **Sharing and cycles.** A node referenced from many places exists once;
    the node graph may hold cycles through `ref` and `dynamic_ref`. A cycle
    that consumes no instance is not an IR fault: the compiler refuses it
    (section 7.5).
13. **Relocatable.** No pointer inside: only indices. Copying a document with
    another allocator copies every table.
14. **Built-in-code locations.** A node built in code gets a synthesized
    pointer through its parent's keyword and index (`/properties/a/items`),
    so every error and annotation has a location.
15. **Pointers.** A JSON Pointer is held as steps: each step extends
    `empty_pointer` or a step that comes before it, by a member's name (a
    string of the document, not escaped) or an element's index. A pointer is
    never held as text: `write_pointer` spells it, escaped as RFC 6901 writes
    it, where it is reported (a diagnostic, an error's or an annotation's
    location). So the pointers of a document hold each token once, and a
    document's memory is linear in its schema, whatever the keys' lengths and
    the nesting's depth.

### 5.4 Reading, building, checking

```cpp
template <class Traits>
class basic_document {
public:
    using number_type = typename Traits::number_type;
    using allocator_type = typename Traits::allocator_type;
    explicit basic_document(allocator_type allocator = {});

    // reading
    node_id root() const noexcept;
    const node& at(node_id id) const noexcept;
    std::span<const keyword> keywords(node_id id) const noexcept;
    const keyword* find(node_id id, keyword_kind kind) const noexcept;
    std::span<const node_id> nodes(range r) const noexcept;
    std::span<const named_subschema> entries(range r) const noexcept;
    std::span<const string_id> strings(range r) const noexcept;
    std::span<const dependency> dependencies(range r) const noexcept;
    std::span<const value_id> values(range r) const noexcept;
    std::string_view string(string_id id) const noexcept;
    const boost::json::value& value(value_id id) const noexcept;
    const number<number_type>& number(number_id id) const noexcept;
    std::span<const resource> resources() const noexcept;
    std::span<const anchor> anchors() const noexcept;
    const pointer_step& step(pointer_id id) const noexcept;
    template <class String> void write_pointer(pointer_id id, String& out) const;  // appends it
    std::size_t size() const noexcept;               // nodes

    // building: the loader's, and zod's
    node_id reserve(resource_id resource, pointer_id pointer);
    pointer_id add_pointer(pointer_id parent, string_id name);         // a member
    pointer_id add_pointer(pointer_id parent, std::uint32_t element);  // an element
    void define(node_id id, node_kind kind, std::span<const keyword> keywords);  // once each
    void set_root(node_id id);
    resource_id add_resource(const resource& r);
    void add_anchor(const anchor& a);
    string_id intern(std::string_view text);
    value_id add_value(const boost::json::value& v);   // copied into the document's storage
    value_id add_value_at(value_id base, pointer_id pointer);  // named inside base, no copy
    number_id add_number(const number<number_type>& n);
    range add_nodes(std::span<const node_id> ids);
    range add_entries(std::span<const named_subschema> entries);
    range add_strings(std::span<const string_id> ids);
    range add_dependencies(std::span<const dependency> deps);
    range add_values(std::span<const value_id> ids);

    result<void> check() const;                      // invariants 1 to 11, 13 and 15
    allocator_type get_allocator() const noexcept;
};
using document = basic_document<default_traits>;
```

Nodes are reserved before they are defined, so a parent's keywords can name
children that a later step defines; keywords stay contiguous because each
node's are appended once, at `define`.

A value named inside another (`add_value_at`) is resolved from that other's
address one pointer at a time, each step looked up by Boost.JSON's index of
its object, never by scanning an object's keys; the loader names each value
one step inside its parent's view, so resolving and copying a document cost
its size. A copy resolves each named value again, in id order, from its
base's new address. `check()` reads each table once and spells a pointer
only for the fault it returns, so it costs the document's size, times a
logarithm for the sorts that find duplicate names, anchors and URIs.

**A constraint on nested resources (references).** A node's pointer is from
its resource's root (invariant 9), but the loader looks a number up in
`parse`'s table of exact numbers by its pointer from the document's root,
and a refusal names that pointer too. Today every node is in resource 0, so
the two pointers are one. When a nested resource starts a chain of its own at
`empty_pointer`, the walk must carry both: the resource's pointer for
`node::pointer`, and the document's for exact numbers and refusals.
Otherwise a nested bound whose double equals the root's at the same relative
path (`0.1` at `/$defs/x/minimum` and `0.10000000000000000001` at
`/minimum`) would silently take the root's exact value.

`ir/equivalent.hpp` (E4): `bool equivalent(const basic_document& a, node_id,
const basic_document& b, node_id)` compares two graphs structurally (kinds,
payloads, constants by JSON equality, numbers by value, the reached graph
including cycles), ignoring `name`, `source`, pointers and resource URIs: the
emitter's round trip test (section 16.4).

### 5.5 The freeze

At the end of E1 the IR headers (`ir/*.hpp` but `equivalent.hpp`) and the
sink concept (section 7.3) are frozen. A later change to either is a change
of the contract: proposed to the controller, applied in one change to every
open sub-track's branch, never by one track alone. Appending a
`keyword_kind`, an `errc` or an `error_kind` value is not a change of the
contract; reusing or reordering one is forbidden.

## 6. The loader

### 6.1 The walk

`load` walks the schema with an explicit work list in the document's
allocator (no recursion on the schema's depth, section 14), and for each
value in a schema position:

1. determines the resource's dialect on entering it: its `$schema`, else the
   enclosing resource's, else `default_dialect`, else the validator's first
   dialect (D3); a dialect outside the set is `errc::unknown_dialect`
   naming it; a `$schema` naming a custom meta-schema loads that meta-schema
   (section 8), reads its `$vocabulary` and takes its own dialect (S4);
2. reserves a node, records `$id`, anchors and `$schema`, then reads every
   member through the dialect's keyword table into IR keywords, reserving
   children and queuing them;
3. counts the schema's nesting, refusing past `max_schema_depth`
   (`errc::schema_too_deep`);
4. after the walk, resolves every reference (section 8), loading what the
   registry, the bundled meta-schemas or the resolver provide, and repeats
   until no reference is open;
5. checks shapes the meta-schema would refuse when `validate_schema` is
   off (`errc::invalid_keyword_value`), and, when on, leaves that to the
   meta-schema check of section 8.4.

### 6.2 Normalization per dialect

`4`, `6`, `7`, `19` and `20` are draft 4, 6, 7, 2019-09 and 2020-12; `OA` is
OpenAPI 3.0.

| Source | Dialects | IR |
| --- | --- | --- |
| boolean schema | 6, 7, 19, 20 | node `always_valid` / `never_valid` |
| boolean schema | 4, OA | `errc::not_a_schema` |
| `id` (4), `$id` (6 on) without fragment | 4, 6, 7, 19, 20 | a new resource, URI resolved against the enclosing base |
| `"$id": "#name"`, `"id": "#name"` | 4, 6, 7 | a plain anchor |
| `$id` with an empty fragment | 19, 20 | accepted, fragment dropped; a non-empty fragment is `errc::invalid_uri` |
| `$anchor` | 19, 20 | a plain anchor |
| `$dynamicAnchor` | 20 | a dynamic anchor and a plain anchor of the same name |
| `"$recursiveAnchor": true` | 19 | `resource::recursive_anchor`, and a dynamic anchor named `""` |
| `$ref` | 4, 6, 7, OA | `ref`; every sibling keyword becomes `ignored` |
| `$ref` | 19, 20 | `ref`, siblings evaluated |
| `$recursiveRef` | 19 | `dynamic_ref`, `form = recursive`, `anchor = ""` |
| `$dynamicRef` | 20 | `dynamic_ref`, `form = dynamic`; a pointer fragment is never bookended |
| `$defs` | 19, 20 | `defs` |
| `definitions` | 4, 6, 7; 19, 20 with `compatibility_keywords` | `defs`, name `definitions`; otherwise `unknown` |
| array `items` | 4, 6, 7, 19 | `prefix_items`, name `items` |
| schema `items` | 4, 6, 7, 19 | `items` |
| `additionalItems` beside array `items` | 4, 6, 7, 19 | `items`, name `additionalItems` |
| `additionalItems` beside schema `items` or alone | 4, 6, 7, 19 | `ignored` |
| `prefixItems`, `items` | 20 | `prefix_items`, `items` |
| `maximum: n`, `exclusiveMaximum: true` | 4, OA | `exclusive_maximum n`, name `maximum`; the boolean kept as `ignored` |
| `exclusiveMaximum: false`, or `true` alone | 4, OA | `maximum` as written, the boolean `ignored` |
| `exclusiveMaximum: n` | 6, 7, 19, 20 | `exclusive_maximum n` (and the minimum pair likewise) |
| `dependencies` | 4, 6, 7; 19, 20 with `compatibility_keywords` | each member: an array to a `dependent_required` entry, a schema to a `dependent_schemas` entry, both named `dependencies`; otherwise `unknown` |
| `dependentRequired`, `dependentSchemas` | 19, 20 | `dependent_required`, `dependent_schemas` |
| `type` with `integer` | 4 | `integer_by_value = false` (`1.0` is not an integer) |
| `type` | others | `integer_by_value = true` |
| `contains` | 6, 7, 19 | `annotates_items = false` |
| `contains` | 20 | `annotates_items = true` |
| `minContains`, `maxContains` | 19, 20 | `min_contains`, `max_contains` |
| `format` | 4, 6, 7, OA, 19 | `assertion = false` |
| `format` | 20 | `assertion` true when the resource's vocabularies hold `format-assertion` |
| `contentEncoding`, `contentMediaType` | 7, 19, 20 | `content_*` |
| `contentSchema` | 19, 20 | `content_schema` |
| `$comment` | 7, 19, 20 | `comment` |
| `nullable: true` beside `type` | OA | `null` added to `type`'s set; the keyword kept as `ignored` |
| `nullable` without `type` | OA | `ignored` |
| `discriminator`, `example`, `xml`, `externalDocs` | OA | their annotation kinds |
| a known keyword of a vocabulary the resource's meta-schema leaves out | 19, 20 | `unknown` (S4; `vocabulary.json`) |
| unknown keyword | 19, 20 | `unknown` (annotation by default) |
| unknown keyword | 4, 6, 7 | `unknown` (no annotation: these dialects have no annotation output) |
| unknown keyword but `x-` | OA | `errc::unknown_keyword` by default (the Schema Object lists what it supports) |
| `$schema`, `$vocabulary` | all | the resource's dialect and vocabularies, not keywords |
| a user vocabulary's keyword, active (4.8) | all | `user` |

A schema inside an unknown keyword is not loaded unless a reference reaches
it; then it is loaded at that point, and an `$id` inside it does not
register a resource (`optional/unknownKeyword.json`,
`refOfUnknownKeyword.json`). `$id` inside `enum` or `const` is data
(`optional/id.json`).

## 7. The compiler and the VM

### 7.1 The plan

`compile` lowers the IR into a plan in the schema's allocator: a flat
instruction list grouped in blocks, one block per compiled node, and the
constant tables. No pointers: indices only, so the plan can be copied or
serialized later (D9, S6).

```cpp
namespace webcpp::json_schema::detail {

struct instruction {
    opcode op;                // std::uint8_t
    std::uint8_t flags;
    std::uint16_t small;      // a short operand: a type mask, a count of children
    std::uint32_t a, b;       // indices into the plan's tables
    std::uint32_t location;   // the keyword location table: IR keyword_id, evaluation
                              // token, absolute location
};

struct block { std::uint32_t first, count; ir::node_id node; std::uint32_t flags; };

}
```

### 7.2 Instruction categories

| Category | Instructions (E1 marks the core subset E1 builds) |
| --- | --- |
| Assertion | `fail` (false schema), `assert_type`, `assert_const`, `assert_enum`, `assert_multiple_of`, the four bounds, `assert_length`, `assert_pattern`, `assert_item_count`, `assert_unique`, `assert_property_count`, `assert_required`, `assert_dependent_required` (E1); `assert_format`, `assert_content`, `call_user`, `assert_enum_hash` (E5) |
| Annotation | `annotate` (meta-data, `unknown`, format and content annotations, a user keyword that annotates) (E2) |
| Logical | `all_of`, `any_of`, `one_of`, `not`, `if_then_else` (E1) |
| Loop | `loop_properties` (properties, patternProperties and additionalProperties fused: name table, then patterns, then the rest), `loop_property_names`, `loop_dependent_schemas`, `loop_items` (prefix blocks and the rest), `loop_contains` (with its bounds) (E1); `loop_unevaluated_properties`, `loop_unevaluated_items` (E2) |
| Control | `type_guard` (skip the rest of the block unless the instance has a type), `return` (E1); `jump` (`$ref`), `dynamic_jump`, `enter_resource`, `leave_resource`, `mark_evaluated` (E2) |

Fused instructions and the optimizations of S6 (fused type checks, a large
`enum` as a hash table on a hash consistent with JSON equality, `required`
fused with `properties`, cheap checks first, `true`/`false` subschemas
folded, small non-recursive `$ref` targets inlined, `unevaluated*` resolved
statically) are E5's; until then each keyword compiles to its plain
instruction, and the suite is the oracle that E5 changes no verdict.

### 7.3 Iterative evaluation and the two modes

The VM is `detail::machine<Traits, Sink>`. It runs a block on an explicit
stack of frames (block, instruction, instance value, instance path token,
loop state, partial result, sink mark), in the validation's allocator with an
inline buffer first; an applicator pushes a child frame and resumes when the
child's result comes back. No instance or schema depth reaches the C++
stack (S6).

The sink is a template parameter, and its concept is the second frozen
contract (5.5), the one E3 consumes:

```cpp
namespace webcpp::json_schema::detail {

template <class S>
concept evaluation_sink = requires(S& sink, const event_context& at, const error& e,
                                   const annotation& a, typename S::mark m, bool valid) {
    { S::records } -> std::convertible_to<bool>;  // false: skip every call below, short-circuit
    sink.enter(at);              // a schema node is evaluated at an instance location
    sink.leave(at, valid);       // it ended, valid or not
    sink.fail(e);                // a keyword failed
    sink.annotate(a);            // a keyword produced an annotation
    { sink.checkpoint() } -> std::same_as<typename S::mark>;
    sink.discard(m);             // what was recorded since m no longer counts: a failing
                                 // anyOf alternative when another passed, the annotations
                                 // of a failed node; a sink that keeps everything marks it
};

}
```

`event_context` gives views of the current evaluation path (keyword tokens
with `$ref` segments), the instance path, and the absolute keyword location;
the sink builds JSON Pointers only when it records.

- **`is_valid`** runs `detail::flag_sink` (`records = false`): `allOf` stops
  at the first failure, `anyOf` at the first success unless the compiler
  marked it exhaustive (an `unevaluated*` consumer depends on its
  annotations), `oneOf` at the second success.
- **`validate`** runs `detail::collecting_sink`, which records what
  `collect` asks into the outcome's storage; failing branches are evaluated
  in full so every error is reported.

The two never disagree on validity: a test runs both over the whole suite
(16.5).

### 7.4 `unevaluated*`

The compiler finds, for each `unevaluated_properties` or `unevaluated_items`
node, its contributors: the keywords of the same node and of every in-place
applicator reachable from it (`allOf`, `anyOf`, `oneOf`, `if`, `then`,
`else`, `dependentSchemas`, `$ref`, `$dynamicRef`, never `not`) that
evaluate properties (`properties`, `patternProperties`,
`additionalProperties`, a nested `unevaluatedProperties`) or items
(`prefixItems`, `items`, `contains` when `annotates_items`, a nested
`unevaluatedItems`).

- **E2:** only contributors in such a scope emit `mark_evaluated(instance
  location, name or index)` into a mark stack; `checkpoint` and `rewind`
  around each branch drop the marks of a branch that failed; the
  `loop_unevaluated_*` instruction applies its schema to every member or item
  without a mark at its instance location. A schema without `unevaluated*`
  pays nothing.
- **E5:** when every contributor is static (names, patterns, "all", reached
  only through `allOf` and `$ref`), the loop gets a compile-time filter and
  no marks are emitted, as Blaze does (R 6.2).

### 7.5 References, cycles and the dynamic scope

`ref` compiles to `jump` to the target's block; a recursive schema is a loop
of the plan. A `dynamic_ref` that is not bookended is a plain `jump`; one that
is becomes `dynamic_jump(anchor)`, and only then does the plan carry
`enter_resource`/`leave_resource` at resource roots, to keep the dynamic
scope (S5). At run time `dynamic_jump` takes the outermost resource of the
scope that holds a dynamic anchor of that name (2020-12), or applies
2019-09's `$recursiveAnchor` rule for `form = recursive`; the suite is the
oracle for both.

Before lowering, the compiler builds the graph of in-place edges (the
applicators of 7.4 and `not`; every keyword that descends into a member, an
item, a property name or decoded content consumes) and refuses a cycle made
of in-place edges alone with `errc::reference_cycle`, naming the keyword
locations around it (S5). A `$dynamicRef` adds an edge to every candidate.
Reaching one node twice without a cycle is fine
(`infinite-loop-detection.json`).

### 7.6 The depth limit

`validate_options::max_depth` bounds instance nesting: descending past it is
an error of kind `depth_limit` at that instance location, and that branch
fails. `default_max_depth` is measured, not chosen: the largest power of two
at which validating the 2020-12 meta-schema against an instance nested to that
depth uses at most 1 MiB of validation memory on wasip2 at `-O0`. The
expectation is about 1024. `default_max_schema_depth` is measured the same way
on the loader and the compiler.

## 8. References and the registry

### 8.1 Base URIs and identification

The root's base URI is `compile_options::base_uri`, else `urn:webcpp:root`.
Each `$id` (or `id`) opens a resource whose URI is resolved against the
enclosing base by the URL policy (D12) and normalized (RFC 3986 section
6.2.2); a trailing empty fragment is dropped. A node is reachable by its
resource URI plus a JSON Pointer fragment, by a plain-name anchor, and by a
dynamic anchor (S5). A JSON Pointer that crosses into an embedded resource
follows 2020-12 core 9.2.1.

### 8.2 Lookup order

A reference's absolute URI (fragment removed) is looked up:

1. among the resources already loaded into this document;
2. in `compile_options::registry`, whose documents were added under their
   retrieval URIs;
3. among the bundled meta-schemas (8.3);
4. through `compile_options::resolver`, called once per URI; its error is
   `errc::resolver_failed` naming the URI; no resolver, or a URI none of the
   four knows, is `errc::unresolvable_reference` naming it.

Then the fragment resolves against that resource. Whatever was used is
copied into the IR, so neither the registry nor the resolved values need
outlive the compile (S5). The suite's `remotes/` are added to a registry
under `http://localhost:1234/` (S5).

### 8.3 Bundled meta-schemas

The official meta-schemas of 2020-12 and its eight vocabularies, 2019-09 and
its vocabularies, draft 7, draft 6, draft 4 (about 40 KB), and OpenAPI 3.0's
Schema Object definition (from the OpenAPI Initiative's `schema.json` for
3.0, Apache-2.0; E5), are text in `detail/meta_schemas/`, headers generated
by `meta_schemas/generate.py` from the files fetched into `meta_schemas/`
(URL, date and SHA-256 of each in its README); a check fails the build when a
header differs from what the generator writes. They are parsed only when a
schema references them (S5), and they are trusted: never validated against
themselves.

### 8.4 Meta-schema validation at compile

With `validate_schema` (the default), `compile` validates each resource
against its meta-schema, compiled once per compile call; a registry given in
the options keeps the compiled meta-schemas for later compiles (it is the
user's object, so this is no global state; a registry is not written by two
threads at once). A violation is `errc::meta_schema_violation` whose `where`
is the first error's instance location in the schema and whose `what` is its
keyword (S5).

### 8.5 `$vocabulary`

Read only from a meta-schema (2019-09, 2020-12). A required (`true`)
vocabulary the validator does not know, official or user, is
`errc::unknown_vocabulary` naming its URI; an optional unknown one is
ignored; a known vocabulary left out switches its keywords to `unknown` for
every resource using that meta-schema; `format-assertion` sets
`format_payload::assertion` (S4). Vocabularies are not inherited through
`$ref`.

## 9. Output and messages

### 9.1 Formats (2020-12 section 12, S7)

| Format | `to_json` writes | Needs `collect` |
| --- | --- | --- |
| `flag` | `{"valid": bool}` | any |
| `basic` | `{"valid": false, "errors": [unit...]}` flat, or `{"valid": true, "annotations": [unit...]}` | `errors`; annotations only when collected |
| `detailed` | the hierarchy following the schema, failing branches only, a node with no children removed, a node with one child replaced by it | `errors` |
| `verbose` | the full hierarchy, every node with `valid`, annotations included | `everything` |

A format that needs more than was collected is `errc::output_not_collected`.
An output unit has `valid`, `keywordLocation` (the evaluation path, with
`$ref`/`$dynamicRef` segments), `absoluteKeywordLocation` (always written),
`instanceLocation`, and `error` (the message) or `annotation`, with nested
`errors` or `annotations`. Units are written in evaluation order, so the
output is deterministic. v1's `list` and `hierarchical` come with v1. Every
renderer is iterative.

### 9.2 Messages

Each `error_kind` has an English template with named parameters, in one
table (`output/messages.hpp`): for example `type` is `"Expected {expected}
but found {received}."`, `required` is `"Required properties {names} are
missing."`, `max_length` is `"Expected at most {expected} characters but
found {count}."`. A message is built only when asked (`outcome::message`,
`to_json`), never by `is_valid` (S7). A hook in the validate options
replaces the templates entirely.

The `.pot` catalogue, `i18n/webcpp_json_schema.pot` (domain
`webcpp_json_schema`, `msgctxt` the kind's name, `msgid` its template), is
printed by `tools/make_pot.cpp`, a program that includes the same table, so
the catalogue cannot drift from the code; the build compares the committed
file with its output and fails on a difference. Never written by hand (S7).

`output/locale.hpp`'s `locale_messages` translates the template with
`boost::locale::translate(context, template)` in the domain and fills it
with `boost::locale::format`. It links `/boost//locale`, needs exceptions
(SPIKE 5: Boost.Locale throws unguarded), and is refused by name under
`WEBCPP_JSON_SCHEMA_NO_EXCEPTIONS` (S3); the core hook works in both modes.

## 10. Formats

- The format set is local: `compile_options::formats`, empty before E5,
  `default_formats()` from E5, extensible by the user (`format_entry`)
  (S8). Its functions must outlive the schemas compiled with them.
- `default_formats()` names `lexical::is_date_time`, `is_date`, `is_time`,
  `is_duration`, `is_email`, `is_idn_email`, `is_hostname`,
  `is_idn_hostname`, `is_ipv4`, `is_ipv6`, `is_uri`, `is_uri_reference`,
  `is_iri`, `is_iri_reference`, `is_uuid`, `is_uri_template`,
  `is_json_pointer`, `is_relative_json_pointer`, each `std::string_view` in,
  `bool` out, and `regex` through `<webcpp/lexical/regex.hpp>`'s
  `lexical::is_regex` (S8). Cross-checked against `WEBCPP_LEXICAL_SPEC.md`
  (controller, 2026-10-09): every `is_*` allocates nothing except
  `is_iri` and `is_iri_reference`, which map non-ASCII text into a 1 KiB
  inline buffer and beyond it use a `std::pmr::memory_resource&`; the
  default set calls those two through their resource overload with the
  validation's resource, so nothing allocates outside the given allocator.
  SRELL allocates through the given allocator by the mirror's allocator
  patch (owner, 2026-10-09; `~/.buzz/RESEARCH/SRELL_ALLOCATION_SPIKE.md`):
  the regex policy searches with the validation's allocator,
  `R::search(re, text, allocator)`, with a small inline buffer that
  overflows into it, and compiled regexes live in the schema's allocator;
  `is_regex` takes a `std::pmr::memory_resource&` the same way.
- A format asserts when `format_payload::assertion` is set (the
  `format-assertion` vocabulary) or `format_assertion` is on (D10); it then
  checks strings only (other types pass) and fails with kind `format`. An
  unknown format name is an annotation (S8). Otherwise `format` is an
  annotation of its name.
- Content (S8): `contentEncoding`, `contentMediaType` and `contentSchema`
  annotate from 2019-09 on (and draft 7's pair). With `content_assertion`:
  `base64` is decoded (a small decoder in `detail`), `application/json` is
  parsed with Boost.JSON into the validation's allocator, and
  `contentSchema` applies to the decoded value; a failure is kind `content`.
- The suite: required tests with the default; `optional/format/` with
  `format_assertion` on; and the same `optional/format/` cases run against
  `webcpp::lexical` directly, which belongs to the formats library (S8).

## 11. The emitter

### 11.1 Forms per dialect

`emit` writes the target's own forms (S9):

| IR | 2020-12 | 2019-09 | 6, 7 | 4 | OpenAPI 3.0 |
| --- | --- | --- | --- | --- | --- |
| `defs` | `$defs` | `$defs` | `definitions` | `definitions` | `emitted::components` |
| `prefix_items` + `items` | `prefixItems`, `items` | array `items`, `additionalItems` | likewise | likewise | unrepresentable |
| `exclusive_maximum` | number | number | number | `maximum` + `exclusiveMaximum: true` | as draft 4 |
| `dynamic_ref` dynamic | `$dynamicRef` | unrepresentable | unrepresentable | unrepresentable | unrepresentable |
| `dynamic_ref` recursive | unrepresentable | `$recursiveRef` | unrepresentable | unrepresentable | unrepresentable |
| `dependent_*` | split | split | `dependencies` | `dependencies` | unrepresentable |
| `type` with several types | array | array | array | array | one type plus `nullable: true`, else `anyOf` of single types |
| `const_` | `const` | `const` | `const` | `enum: [v]` | `enum: [v]` |
| `unevaluated_*` | as is | as is | unrepresentable | unrepresentable | unrepresentable |
| boolean nodes | `true`/`false` | likewise | likewise | `{}` / `{"not": {}}` | likewise |
| `contains`, `if_`, `property_names` | as is | as is | as is (6: no `if`) | unrepresentable | unrepresentable |
| annotations, `unknown` | as is | as is | as is | as is | `x-` names only, else unrepresentable |
| `user` | its `write`, else the value | likewise | likewise | likewise | likewise |

Unrepresentable means `errc::unrepresentable` naming the node, or under
`unrepresentable::drop` the keyword is left out and its name added to the
node's `"x-webcpp-dropped"` array (S9). `ignored` keywords are written back
only into their own dialect.

### 11.2 Sharing, cycles, order

- A node reached more than once, or by a cycle, is written once under the
  target's definitions and referenced by `$ref`; its name is its existing
  `defs` name, else `__schema<N>` in order of first reach (zod's own
  convention, ZOD 7). Every IR `ref` is written as a `$ref` to its target's
  location. Resources of other documents are embedded with their `$id`, in
  the target's forms.
- Keys follow the IR's keyword order (invariant 5), with `$schema`, `$id`,
  `$anchor`, `$dynamicAnchor` first; the output is identical for the same
  IR, byte for byte once serialized.
- The emitter is iterative.

### 11.3 Tested

The round trip (16.4) and goldens per form above (16.3).

### 11.4 Room left for zod

zod's `toJSONSchema` (ZOD 7) adds `reused: inline`, `cycles: throw`, `io`
and `override`; these become further `emit_options` members later without
breaking the ones above. zod's metadata travels as meta-data keywords in the
IR, its refinements as user keywords with a `write` that drops them, and its
output is checked against zod's own `toJSONSchema` (S9).

## 12. Numbers

- `Number` is `traits::number_type`, `decimal64_t` by default, any type
  modelling `number` (4.2); `decimal128_t` for bignums (S3).
- **Schema numbers** are exact (invariant 7): integers stay `int64_t` or
  `uint64_t`; other numbers come from the lexeme through `parse`, else from
  the double's shortest round-trip text.
- **Instance numbers** come from the `boost::json::value`: `int64_t` and
  `uint64_t` exactly. An instance `double` is read once as the decimal its
  shortest round-trip representation denotes (at most 17 digits, in a
  128-bit coefficient), which is exact, and compared exactly with the
  schema's number; it is never rounded to `Number` (amendment A1 of
  2026-10-09, controller ruling under the owner's overnight go, for the
  owner to confirm: "converted once to `Number`" rounded 2^64 under
  `maximum: 18446744073709551615`, and a double's exact binary value fails
  the suite's `exclusiveMinimum: 1.1` and `multipleOf 0.0001` cases).
- **Comparisons** between an integer and a `Number` are exact, on the
  `Number`'s decomposition, never by rounding the integer.
- **`multipleOf`**: two integers, `x % y == 0`. Otherwise, with `x = cx *
  10^ex`, `y = cy * 10^ey`: when `ex >= ey`, `x` is a multiple iff `(cx mod
  cy) * (10^(ex - ey) mod cy) mod cy == 0`, by modular exponentiation in
  128-bit arithmetic (a mulmod by doubling past 128 bits for `decimal128_t`);
  when `ex < ey`, iff `cy * 10^(ey - ex)` divides `cx`, false when that
  product exceeds 128 bits and `cx != 0`. Never `fmod`, which fails a
  required test (R 2.4).
- **Integers:** `type: integer` holds for `int64_t`, `uint64_t`, and a
  `double` with zero fraction when `integer_by_value` (every dialect but
  draft 4).
- **Equality** (`const`, `enum`, `uniqueItems`): numbers by mathematical
  value across kinds (`1` equals `1.0`), objects ignoring member order,
  arrays in order, strings by bytes.
- **Strings:** length counts code points, the bytes outside `0x80` to
  `0xBF`.
- **Limits:** a schema number the `Number` cannot hold (`1e400` in
  `decimal64_t`) is `errc::invalid_number`. Constants (`const`, `enum`)
  compare at the precision their `boost::json::value` holds, documented.

## 13. Errors and exceptions

- Errors as values: `load`, `compile`, `emit`, `to_json` and `validate`
  return `result<T>` with a `diagnostic`; a verdict is never an error (S1).
- `config.hpp` defines `WEBCPP_JSON_SCHEMA_NO_EXCEPTIONS` automatically under
  `BOOST_NO_EXCEPTIONS`, or by the developer (W C3). Behind `#ifndef` stand
  `compile_or_throw` and the whole of `output/locale.hpp` (S11). Every raise
  goes through `boost::throw_exception`; the lint's bare-throw rule and its
  no-exceptions twin check it (AGENTS.md chapter 6).
- The library compiles and works without exceptions: SRELL in quiet mode with
  the mirror's patch (D11), Boost.URL's and Boost.JSON's non-throwing APIs
  only (`if_object`, `if_contains`, `try_*`), no `.value()` on a result
  inside the library.
- `test/config_test.cpp` is built twice, as AGENTS.md says; every test and
  example runs with exceptions as the build asks, and the lint analyses each
  without them (both exception modes, S10).

## 14. Allocation

- **Two allocators** (S2): the schema's, given to `load` and `compile`,
  holds the IR, the plan, the constants, the compiled regexes (through
  `webcpp::regex::basic_regex`'s `Allocator`) and the `shared` control block
  (`std::allocate_shared`); the validation's, given to `validate` and
  `is_valid`, holds the frames, the marks, the dynamic scope, the collected
  events, the error strings, the messages and a search's state (through
  `match_results`); nothing allocates outside the two on the default backend
  (regex's guarantee 4). `to_json` writes into the `storage_ptr` it is given.
- **pmr by default** (W alloc): `std::pmr::polymorphic_allocator<std::byte>`.
  Boost.JSON allocates through `detail::json_resource`, a header-only adapter
  from `std::pmr::memory_resource` to `boost::container::pmr::memory_resource`
  (trystero's mechanism), held inside the compiled schema or the outcome and
  given as a non-owning `storage_ptr`, so no counted resource is created
  with `operator new`, and `boost::container::pmr`'s compiled globals are
  never used. A non-pmr allocator type gets the same adapter over it.
- **`is_valid` allocates nothing on the common path** (S2): its frames, marks
  and dynamic scope start in inline buffers; only a nesting past them
  allocates, in the validation's allocator.
- **No recursion on input depth** anywhere: loader, compiler, VM, renderers,
  emitter, `equivalent`, `check`.
- **Policies included** (S2): a policy allocates only through the allocator
  it is given (the concepts take it). The URL policy works in a stack buffer.
- **Proof:** `test/allocation_test.cpp` replaces `operator new` for itself and
  counts zero global allocations across `parse`, `compile` and `validate` of
  the suite's schemas, with counting resources upstream of both allocators,
  and zero allocations of any kind in `is_valid` within the inline depth. Its
  exceptions, stated on the page: what the user's resolver, hook, format or
  vocabulary functions allocate, and Boost.Locale's own allocations in the
  optional header.

## 15. Targets

- native, wasip2, wasip3 and emscripten, each a declared target with its lane
  (S11), the suite on each (S10). MSVC, GCC and Clang through the CI's
  native lanes.
- On WASI the suite reaches the runner through wasmtime's `--dir`, passed by
  the build's run script (the wasi redesign's rule, every WASI program gets
  one), or, if that rule is not yet in the superproject when E1 starts, by a
  `webcpp.jam` change that passes a directory to a test, integrated
  serially (S12). On emscripten, `-sNODERAWFS=1` gives it the host's files.
- Exceptions on every target as AGENTS.md says; the locale header's test runs
  natively, on wasip2 and wasip3 (SPIKE 5 measured it running with wasm
  exceptions) and links `/boost//locale`.
- The fuzz lane is native Linux clang only (libFuzzer, sanitizers); the
  Bowtie lane needs Docker; the benchmark runs natively and under wasmtime on
  the owner's machine.

## 16. Tests

All with lightweight_test and `webcpp.run`, written first, seen failing, on
every declared target unless said otherwise.

### 16.1 The suite runner (`test/suite/`)

- The suite is the submodule `third_party/libs/JSON-Schema-Test-Suite`,
  pinned at a commit (R used `5b0ee16`); moving it is a change of its own.
- One runner program per dialect, `suite-draft2020-12`, `suite-draft2019-09`,
  `suite-draft7`, `suite-draft6`, `suite-draft4`, `suite-openapi3-0` (S10),
  told its dialect and the suite's root by `WEBCPP_TEST_*` defines (since
  `webcpp.run` passes no arguments). It walks `tests/<dialect>/`, the
  optional groups and `optional/format/`, registers `remotes/` under
  `http://localhost:1234/`, compiles each case with the dialect as default
  (as the suite's README asks), and validates each test with `is_valid` and
  `validate`, which must agree. `optional/format/` and
  `optional/format-assertion.json` run with `format_assertion` on (D10). A
  case whose schema fails to load fails each of its tests, with the
  diagnostic printed.
- `suite-openapi3-0` runs the draft 4 cases whose schemas use only the
  Schema Object's keywords, with `default_dialect = openapi3_0`, and the
  cases of `test/openapi/*.json` (the suite's own format) for `nullable`,
  `discriminator`, `example` and the refused keywords.
- v1 has no runner until v1 is published (S10 "v1 later"); when its policy
  lands, its failing tests start as known failures (S4).
- **Known failures:** `test/known_failures/<dialect>.json`, an array of
  `{"file", "case", "test", "reason"}` (descriptions as the suite writes
  them). The runner fails on a failing test not listed, on a listed test
  that passes, and on a listed test it cannot find, so the list never lies
  (S10). During the sub-tracks the list names the track that removes each
  entry ("E2: references").

### 16.2 The annotation and output suites

- `test/annotations_test.cpp` (E2): `annotations/tests/*.json` (46 cases, 58
  tests), filtered by each dialect's `compatibility`, comparing
  `outcome::annotations()` with each assertion's location, keyword and
  expected values.
- `test/output_suite_test.cpp` (E3): `output-tests/draft2020-12` (and
  2019-09 in E5), and every basic, detailed and verbose report the suite run
  produces validated against the official output schema with json_schema
  itself.

### 16.3 Unit tests

- `ir_test`: building, `check()` refusing each broken invariant, copying with
  another allocator.
- `loader_<dialect>_test`: one case per row of the normalization table
  (6.2), comparing the IR built in code.
- `numbers_test`: exact `multipleOf` (the four required cases of R 2.4,
  `float-overflow`, both decimal types), integer-to-decimal comparisons,
  `parse` keeping lexemes.
- `equality_test`, `utf8_test`, `pointer_test`, `url_test`, `regex_test`
  (compile error as a value, `limit` on `(a*)*b`).
- `compile_errors_test`: every `errc` a load or compile can return, with its
  `where`.
- `explain_test` (E5): `explain()` goldens per optimization, the plan before
  and after.
- `output_test`, `messages_test` (E3): each renderer on scripted sink events
  (until E2 lands, then on real runs), each kind's template, a hook, the
  `.pot` check.
- `emitter_test` (E4): a golden per cell of the table of 11.1, both
  unrepresentable modes, sharing, cycles, key order.
- `vocabulary_test` (E5): a user keyword checked, compiled, failing, erring,
  annotating, written back, conflicting.
- `webcpp.headers-alone json_schema : ../include ;` and the include
  boundaries.

### 16.4 The round trip

The suite runner's round-trip mode, every dialect it runs: load, emit into the
same dialect, reload, then `ir::equivalent` holds and every test's verdict is
unchanged (S9). Then every other target dialect: where `emit` succeeds, the
verdicts must be the same; an unrepresentable case is counted and reported,
not failed.

### 16.5 Allocation, exceptions, hostile input

- `allocation_test` as section 14.
- `config_test` twice.
- `hostile_test`: an instance nested 100,000 levels (built with a
  `boost::json::monotonic_resource`, whose trivial deallocation also spares
  Boost.JSON's recursive destruction; parsed text needs
  `parse_options::max_depth` raised) gives a `depth_limit` error, no stack
  overflow, on every target (S10); a schema nested 100,000 levels gives
  `schema_too_deep`; a chain of 100,000 in-place `$ref`s compiles or fails
  as a value; catastrophic patterns give `pattern_limit`; an in-place
  reference cycle gives `reference_cycle`.

### 16.6 Fuzzing

`test/fuzz/loader_fuzz.cpp` (bytes to `parse` to `load`) and
`validator_fuzz.cpp` (a schema and an instance from one input), libFuzzer
with AddressSanitizer and UndefinedBehaviorSanitizer, corpus seeded from the
suite, an own lane `fuzz` native Linux clang only, at will locally and 1 to 2
minutes in CI (S10). Each crash found becomes a file under
`test/fuzz/regressions/`, replayed by an ordinary test on every target.

### 16.7 Agreement of the two modes

The suite runner checks `is_valid == validate()->valid()` on every test, so
the short-circuiting sink can never drift from the collecting one.

### 16.8 Bowtie

`bowtie/bowtie_webcpp.cpp` speaks IHOP v1 over standard input and output
(`start`, `dialect`, `run` with its `registry`, `stop`; R 3.6), every dialect
of `js::validator`, flag output first. It is also a `webcpp.example` with an
`.input` transcript of commands and its `.expected` answers, so every lane
tests it. `bowtie/Dockerfile` (Alpine, multi-stage) builds the container;
the own lane `bowtie` runs `bowtie run -V` and Bowtie's suite against it,
locally and in webcpp's CI (S10). A pull request to
bowtie-json-schema/bowtie only with the owner's go.

## 17. The benchmark

- Sourcemeta's jsonschema-benchmark (37 schemas with instances; R 7),
  pinned at a commit, against pinned Blaze and ajv, run through the
  benchmark's own implementation directories; webcpp adds
  `benchmark/implementation/`, a program that prints `cold,warm,compile,parse`
  in nanoseconds per schema, built natively and for wasip2 under wasmtime
  (S10, D8). Blaze is only run, never linked or copied (D7).
- `b2 libs/json_schema/benchmark//benchmark`, an explicit target, never part
  of `test`, run on demand on a stable machine, the owner's (S10). It writes
  `benchmark/results.json`: per schema and implementation the four timings
  (median of the runs), and the machine (CPU, cores, memory, OS), the date,
  and every version (compiler, Boost, wasmtime, the benchmark's commit,
  Blaze's commit, ajv's and Node's versions, json_schema's commit). The
  owner commits it.
- `benchmark/report.py` turns `results.json` into `doc/benchmark.adoc` (the
  table, with the method) and `doc/benchmark.svg` (the chart: per schema,
  warm time relative to the fastest, log scale), both generated, committed,
  never typed by hand; CI regenerates them and fails when they differ from
  the committed file (Bench).
- A local micro-benchmark beside it (R 7): compile time and warm validation
  of the 2020-12 meta-schema against itself, natively and under wasmtime.

## 18. Documentation

- Doc Comments on every public symbol (AGENTS.md chapter 7); MrDocs strict.
- `doc/json_schema.adoc`, titled `json_schema: JSON Schema validation for
  {cpp}`, sections in files beside it, every listing a compiled example with
  its `.expected`:
  1. introduction: a first validation;
  2. dialects and `$schema`; choosing a set;
  3. compiling: options, the registry, the resolver, meta-schema checks;
  4. validating: `is_valid`, `validate`, `outcome`, `error`;
  5. output formats and messages: the hook, the locale header;
  6. formats and content;
  7. numbers and exactness;
  8. references, anchors, dynamic references;
  9. user vocabularies;
  10. emitting, OpenAPI 3.0;
  11. allocation; limits (depth, the stack, hostile input); errors and
      exceptions;
  12. conformance: a table per dialect (required, optional and format tests,
      passed and known failures) from `doc/counts.py`, which counts the
      pinned suite's tests and the known-failure files: since CI proves the
      lists never lie, passed = total minus listed, with no run at doc time;
      each known failure with its reason (a doc-tools extension, serially
      integrated, if a page cannot include the lists as they are);
  13. performance: `doc/benchmark.adoc` and `doc/benchmark.svg`, generated
      (17);
  14. testing (the suite, the lanes, Bowtie, fuzzing);
  15. the guarantees, numbered, among them: a compiled schema is immutable
      and may be shared between threads; `is_valid` and `validate` agree;
      `is_valid` allocates nothing on the common path; no function recurses on
      its input's depth; the library touches no network, disk or clock; a
      runaway pattern is an error, never a hang or an abort;
  16. the reference.
- There is no "Differences" appendix (not a port); the conformance chapter
  holds every deviation from the specification, each with its reason.
- `README.md`, `AGENTS.md` (what is specific: the suite and its pin, the
  known-failures rule, the IR freeze, SRELL, the benchmark's machine).

## 19. Changelog

With every change, a line in Boost's release-notes form (REL 4, 6; S11): in
`libs/json_schema/doc/changelog.adoc` under "Unreleased" once the changelog
files exist, until then in the task report's "Changelog" section, which the
controller gathers into `~/.buzz/PLANS/WEBCPP_CHANGELOG_PENDING.md`. Until
json_schema is first published, its entry is one line under "New
Libraries": "json_schema: a header-only JSON Schema validator for C++20,
every dialect from draft 4 to 2020-12 and OpenAPI 3.0, proven by the official
test suite, from R."; from publication on, each user-visible change
adds a bullet under "Updated Libraries".

## 20. Sub-tracks, order and parallel structure (S12)

### 20.1 The sub-tracks

Each ends with a slice of the suite passing, counted by the runner with its
known-failure list, natively and on wasip2, wasip3 and emscripten.

**E1: runner, IR, loader, core keywords (2020-12).**
- The library's checklist files, `build.jam`, `meta/`, `config.hpp`,
  `errors.hpp`, `traits.hpp`, `numbers/`, `regex/` (the policy, written on
  webcpp::regex directly, starting after regex's R1 is merged; E1 is
  declared allocation-complete after regex's R4; regex's R9 adds the page
  link), `json/parse.hpp`, `dialects/dialect.hpp` with every tag.
- The IR, whole (every `keyword_kind`, `check()`), frozen at the end (5.5).
- The loader for 2020-12: every vocabulary's keywords into IR; one resource
  per document; any `$ref`, `$dynamicRef` or nested `$id` is
  `errc::unresolvable_reference` until E2.
- The compiler and the VM laid down for the core keywords (the E1 subset of
  7.2): the plan's format, iterative evaluation, both sinks, the sink concept
  frozen. D9 forbids a separate interpreter, so the first slice runs on the
  one engine, which E2 completes.
- `outcome` with `valid()` and `errors()`; `error`, `error_kind`; the depth
  limit; `allocation_test`, `hostile_test` (instance), `config_test`.
- The suite submodule, the runner, `known_failures/draft2020-12.json`.
- The page's skeleton and its first examples.
- Slice: about 770 of the 772 core tests of 2020-12 (R 3.3), and the optional
  `bignum`, `float-overflow`, `ecmascript-regex`, `non-bmp-regex`.

**E2: the compiler and the VM completed; references; annotations.**
- The URL policy (Boost.URL); `$id`, anchors, the registry, the resolver,
  the bundled meta-schemas and their generator; `$ref`, `$dynamicRef`,
  `$recursiveRef`'s IR (its runtime with 2019-09 in E5), the dynamic scope;
  cycle refusal; meta-schema validation at compile; `$vocabulary`;
  `compatibility_keywords`.
- Annotations, `outcome::annotations()`, `unevaluated*` with marks,
  checkpoint and rewind (7.4).
- Slice: 2020-12 required 1301 of 1301; the annotation suite for 2020-12;
  optional `anchor`, `dynamicRef`, `id`, `unknownKeyword`,
  `refOfUnknownKeyword`, `no-schema`, `dependencies-compatibility`.

**E3: output and messages.**
- The four renderers and `to_json`; the English templates, the hook,
  `outcome::message`; `tools/make_pot.cpp` and the `.pot` check; the locale
  header and its test.
- Built against the frozen sink concept with scripted events, then on real
  runs once rebased on E2.
- Slice: `output-tests/draft2020-12`; every report of the 2020-12 suite valid
  against the official output schema; a golden per message kind.

**E4: the emitter.**
- `emit` with every dialect's forms (11.1), OpenAPI 3.0's emit side,
  sharing, cycles, order, both unrepresentable modes; `ir::equivalent`.
- Slice: every 2020-12 suite schema round-trips (16.4) to an equivalent IR
  with unchanged verdicts (in full once rebased on E2); the goldens of 16.3.
  Round trips through other dialects need their loaders, so they join in E5.

**E5: everything else.**
- Loaders for 2019-09, draft 7, draft 6, draft 4 and OpenAPI 3.0 (with its
  meta-schema); cross-draft; formats through `webcpp::lexical`
  (`format_assertion`, the format-assertion vocabulary, `default_formats()`);
  content assertion; user vocabularies; the optimizations of 7.2 and the
  static `unevaluated*` filter; `explain()`; the fuzz lane; the Bowtie
  harness and lane; the benchmark; the generated conformance and benchmark
  documentation; output tests for 2019-09.
- Slice: required 100%, but listed known failures, on 2019-09 (1261), draft 7
  (929), draft 6 (841), draft 4 (618); every optional group of each,
  `optional/format/` included (874 tests in 2020-12) with `format_assertion`
  on; `optional/cross-draft`, `zeroTerminatedFloats`, draft 7's `content`,
  `format-assertion`; the cross-dialect round trips; `suite-openapi3-0`;
  Bowtie's run green locally; `results.json` from the owner's run.

### 20.2 Order

- Before json_schema: stage 4 (trystero, running) and the config library.
- Wave 1: idna, formats and E1, in parallel.
- After E1: E2, E3 and E4 in parallel, within the cap of 3 or 4 tracks
  implementing at once across webcpp, the machine being shared; the
  assignment of the rest of webcpp's work to waves 2 to 4 is the controller's
  plan.
- E5 after E2, E3 and E4, and after formats is published (json_schema needs
  formats only from E5).
- zod after json_schema.

### 20.3 Worktrees and integration

- Each sub-track works in its own worktree of the superproject,
  `~/dev/webcpp-json-schema-e<N>`, on branch `lib/json_schema-e<N>`, created
  from `main` (pratt's arrangement); inside, `libs/json_schema` is the
  library's repository, E2 to E4 each on a branch of it started from E1's
  result.
- The library's branches merge into its `main` one at a time, each rebased on
  the last, in the order they finish; superproject changes (`.gitmodules`,
  `third_party/`, `tools/`) are integrated serially, with a rebase (S12).
- Commits as `R <github.overboard312@passmail.net>`, no trailer; nothing pushed; the
  repository `webcpporg/json_schema` and its submodule are the owner's steps.
- Each sub-track ends green as AGENTS.md chapter 10 defines it, from scratch.

## 21. Risks

1. **Resolved.** SRELL's internal arrays used `malloc`/`realloc` outside the
   allocators given; the mirror's allocator work (regex's M1b, `a8ecf39`
   before it) routes every allocation through the backend's configured
   allocator, which the counting test (14) now confirms.
2. **Compiled Boost libraries in CI.** Boost.URL (E2 on) and Boost.Locale (the
   optional header) are compiled; CI installs Boost's headers alone (AGENTS.md
   chapter 13). formats meets the Boost.URL half first, in wave 1; json_schema
   reuses its answer. The locale test needs the same for Boost.Locale.
3. **The reading of E1 and E2.** D9's "no separate interpreter" makes E1 lay
   down the compiler and VM that E2 completes (20.1); the controller confirms.
4. **The sink as a second frozen contract.** E3 in parallel with E2 needs it
   (5.5, 7.3); a gap found late in it costs both tracks.
5. **The WASI runner's `--dir`** depends on the run-script rule of the wasi
   redesign or a `webcpp.jam` change (15).
6. **Boost.JSON's depth.** Its parser's default `max_depth` is 32 and its
   destruction recurses: the hostile test and the page must say how a user
   builds and frees a deep instance (16.5).
7. **Licences.** The suite (MIT) is test data only; the bundled official
   meta-schemas' licence must be confirmed before they ship in a header; the
   OpenAPI 3.0 schema is Apache-2.0; jsonschema-benchmark's licence is checked
   before pinning; Blaze is AGPL: run, never linked or copied (D7); ajv is MIT.
   A pin of a third party needs the owner's decision (AGENTS.md chapter 6).
8. **E2 is the largest track**: dynamic scope, 2019-09's recursive rule (its
   runtime in E5), `unevaluated*` with checkpoint and rewind, and meta-schema
   compile cost on every `compile` (measured by the benchmark's `compile`
   column; the registry cache mitigates).
9. **Exactness inside constants** is bounded by the `boost::json::value`
   (12); a user needing more parses with `parse` and a larger `Number`, which
   covers bounds but not constants.
10. **IDN formats tied to a Unicode version** (formats' risk, S8): a residue
    becomes known failures in E5.
11. **Resolved.** webcpp::regex refuses the escapes `\A`, `\Z` and `\z`
    before any engine sees them (regex P6), so `pattern` cannot carry them
    either; `lexical::is_regex` refuses them the same way for
    `format: regex` (SPIKE 7).
12. **v1** may change semantics before publication; nothing ships for it
    until then (S4).
13. **Shared superproject files** touched by idna, formats and E1 at once
    (`third_party/`, `.gitmodules`): serial integration (20.3).
14. **A non-default regex backend** changes `pattern`'s semantics
    (std::regex reads bytes) and allocation; json_schema's suite runs in the
    default configuration only, and its page links regex's matrix.
15. **Until regex's M6**, a hostile `pattern` on a long string is bounded
    per start position, not per search (regex spec 5.2.2).

## 22. Points this specification settles

The design left these open; each is this file's choice, for the controller's
cross-check and the owner's review:

1. `result<T>` carries a `diagnostic` (code, where, what) in fixed buffers,
   so every refusal names its place without allocating (4.1).
2. The message hook writes into a `std::basic_string` of the traits'
   allocator (`std::pmr::string` by default), not `std::string` (4.11).
3. The resolver also receives the `storage_ptr` to build its value in (4.7).
4. `is_valid` returns `bool`; a user keyword's own error makes it `false`,
   and makes `validate` fail with that error (4.8).
5. User keywords are assertions or annotations, never applicators (4.8);
   activation `always` or `when_declared`.
6. `definitions` and `dependencies` are honoured in 2019-09 and 2020-12 by
   default (`compatibility_keywords`), as their meta-schemas list them (6.2).
7. OpenAPI 3.0 refuses keywords its Schema Object does not support, but
   `x-`, by default; its runner is draft 4's compatible cases plus cases of
   webcpp's own (6.2, 16.1).
8. Dropped features are recorded as `"x-webcpp-dropped"`; extracted nodes are
   named `__schema<N>`; OpenAPI 3.0's shared nodes are returned in
   `emitted::components` (4.10, 11).
9. E1 lays down the compiler and the VM; E2 completes them; the sink concept
   is frozen with the IR (20.1, 5.5).
10. The depth limits' measurement (7.6), and the `.pot` printed by a C++
    program rather than parsed from C++ by a script (9.2).
11. The conformance table is computed from the suite and the known-failure
    lists, not from a run at doc time (18).

Approved by the owner on 2026-10-09.
