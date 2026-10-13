## Purpose

webcpp::json_schema is a header-only C++20 JSON Schema validator of webcpp's own, covering drafts
2020-12, 2019-09, 7, 6 and 4 and OpenAPI 3.0's Schema Object, on native, wasip2, wasip3 and
Emscripten, proven by the official JSON-Schema-Test-Suite. These requirements are the structured
form of `reference/spec.md` in the change that introduced it.

## ADDED Requirements

### Requirement: Header-only library in its namespace
The library SHALL be header-only C++20, in namespace `webcpp::json_schema`, with public headers under `<webcpp/json_schema/...>`, one per responsibility, internals in `webcpp::json_schema::detail`, macros prefixed `WEBCPP_JSON_SCHEMA_` and include guards `WEBCPP_JSON_SCHEMA_<PATH>_HPP`. A program MUST reach it through the b2 target `/webcpp/json_schema//json_schema`.

#### Scenario: A program uses the library without building it
- **WHEN** a b2 program names `/webcpp/json_schema//json_schema` and includes `<webcpp/json_schema.hpp>`
- **THEN** it compiles and links without any library of json_schema's own being built

### Requirement: Build target and its dependencies
`build.jam` SHALL declare `project /webcpp/json_schema` and one alias `json_schema` whose usage requirements are `<include>include`, `/boost//headers`, `/webcpp/config//config`, `/webcpp//boost_json`, `/webcpp//boost_url` and `/webcpp/lexical//lexical`. It MUST NOT name `/webcpp/regex//regex`, SRELL or `/boost//url`; Boost.Locale is linked only by the locale header's users.

#### Scenario: No abandoned engine in the build
- **WHEN** `build.jam` is read
- **THEN** it names neither `/webcpp/regex//regex` nor any SRELL target, and names `/webcpp/config//config`

### Requirement: Convenience header
`include/webcpp/json_schema.hpp` SHALL include every public header except `<webcpp/json_schema/output/locale.hpp>`, which a program includes itself because it needs Boost.Locale and exceptions.

#### Scenario: The convenience header leaves Boost.Locale out
- **WHEN** a program built without exceptions includes `<webcpp/json_schema.hpp>`
- **THEN** it compiles, and no Boost.Locale header is included

### Requirement: Include boundaries
`meta/include-boundaries.json` SHALL keep: the IR from the compiler, the VM, the output, the emitter, Boost.URL and Boost.Regex; the emitter from the compiler and the VM; `boost/locale` inside `output/locale.hpp`; `boost/url` inside `url/boost_url.hpp`; formats inside `formats/default_formats.hpp` and `regex/boost_regex.hpp` (which may include only `webcpp/lexical/regex.hpp`). No header MUST include `srell` or `webcpp/regex`.

#### Scenario: The lint refuses a header crossing a boundary
- **WHEN** a header under `ir/` includes `<webcpp/json_schema/detail/vm/sink.hpp>`
- **THEN** the superproject's lint fails, naming the header and the boundary's reason

### Requirement: Repository files of a webcpp library
The repository SHALL hold `build.jam`, `meta/libraries.json` (`"port-of": null`, category `["Parsing", "Correctness"]`), `meta/include-boundaries.json`, `README.md`, `AGENTS.md` (only what is specific to json_schema), `LICENSE_1_0.txt`, `doc/changelog.adoc`, `.gitignore` (at least `doc/html/`), `.gitattributes` and `.github/workflows/ci.yml` calling the superproject's `library.yml`.

#### Scenario: The index page lists the library
- **WHEN** the superproject's `b2 doc` runs with `libs/json_schema` present
- **THEN** the index lists json_schema from its `meta/libraries.json`, linked to its page

### Requirement: Licence notice and text rules
Every source file (`.hpp`, `.cpp`, `.py`, `.sh`, `.jam`, `Jamfile`, `build.jam`, `.yml`, `.adoc`, `.html`, a Dockerfile) SHALL open with the notice "Copyright (c) 2026 WebCpp.org" and the BSL-1.0 lines, `//` for C++ and AsciiDoc, `#` for the rest, a `#!` line first. Every file MUST be English, without the em dash, and without the banned word the superproject's lint checks.

#### Scenario: A file without the notice fails the lint
- **WHEN** a new `.hpp` is added without the licence notice
- **THEN** the lint fails, naming the file

### Requirement: Configuration macro for exceptions
`config.hpp` SHALL include `<webcpp/config.hpp>` first and define `WEBCPP_JSON_SCHEMA_NO_EXCEPTIONS` when `WEBCPP_HAS_EXCEPTIONS` is not defined, keeping it when the developer defines it alone. Every public header MUST include `config.hpp` first. Under `__MRDOCS__` the macro is defined and undefined at once, so the reference lists it.

#### Scenario: The three builds of the configuration test
- **WHEN** `test/config_test.cpp` is built as the build asks, with `WEBCPP_NO_EXCEPTIONS`, and with `WEBCPP_JSON_SCHEMA_NO_EXCEPTIONS` on the command line
- **THEN** the macro is defined exactly when `WEBCPP_HAS_EXCEPTIONS` is not, in the second build, and in the third

### Requirement: Throwing APIs only with exceptions
Every API that throws SHALL sit behind `#ifndef WEBCPP_JSON_SCHEMA_NO_EXCEPTIONS`: `basic_validator::compile_or_throw` and the whole of `output/locale.hpp`. Every raise MUST go through `boost::throw_exception`; no header holds a bare `throw`, and the library never calls `.value()` on a `result`.

#### Scenario: compile_or_throw absent without exceptions
- **WHEN** a program built with `exception-handling=off` calls `validator.compile_or_throw(schema)`
- **THEN** it fails to compile, while `validator.compile(schema)` compiles and returns a `result`

### Requirement: Error codes are fixed values
`errc` SHALL have the values: `not_a_schema` 1, `invalid_keyword_value` 2, `meta_schema_violation` 3, `unknown_dialect` 4, `unknown_vocabulary` 5, `unknown_keyword` 6, `invalid_uri` 7, `unresolvable_reference` 8, `resolver_failed` 9, `invalid_json_pointer` 10, `duplicate_resource` 11, `duplicate_anchor` 12, `reference_cycle` 13, `invalid_regex` 14, `schema_too_deep` 15, and the values of the next requirement.

#### Scenario: The values are pinned
- **WHEN** `test/errors_test.cpp` runs
- **THEN** each enumerator has the value listed, and no value is reused

### Requirement: Error codes after 15, and the category
`errc` SHALL continue with `invalid_number` 16, `invalid_ir` 17, `unrepresentable` 18, `output_not_collected` 19, `vocabulary_conflict` 20 and `unsupported_regex` 21; a new error MUST take the next value. The category's name SHALL be `webcpp.json_schema` and its fixed identity `0x6a736368656d6100`, with `make_error_code` and the `is_error_code_enum` specialization.

#### Scenario: An errc converts to an error code of the category
- **WHEN** `make_error_code(errc::invalid_regex)` is compared with `error_category()`
- **THEN** the code's category is the one named `webcpp.json_schema`, with id `0x6a736368656d6100`

### Requirement: Results carry a diagnostic
Every operation that can fail SHALL return `result<T>`, `boost::system::result<T, diagnostic>`, where `diagnostic` holds `code`, `where` (a keyword location or a URI) and `what` (the dialect, keyword or detail named), each a `static_string<256>` cut with `...` on a UTF-8 code point boundary. Building a diagnostic MUST NOT allocate.

#### Scenario: A refusal names its place under an exhausted allocator
- **WHEN** a load fails with `unknown_dialect` while the schema's allocator can give nothing more
- **THEN** the `diagnostic` still holds the code, the place and the dialect's URI

### Requirement: Default traits
`default_traits` SHALL define `number_type` as `boost::decimal::decimal64_t`, `allocator_type` as `std::pmr::polymorphic_allocator<std::byte>`, `regex_policy` as `boost_regex` and `url_policy` as `boost_url<>`. The `traits` concept MUST check the allocator of `std::byte` first, then the number concept, the regex policy over that allocator twice, and the URL policy.

#### Scenario: A traits type replaces one member
- **WHEN** a program derives `struct wide : default_traits { using number_type = boost::decimal::decimal128_t; }`
- **THEN** `wide` models `traits`, and its validators hold `decimal128_t` bounds

### Requirement: The number concept
`Number` SHALL model `number`: regular, totally ordered, constructible from `int64_t` and `uint64_t`, with `number_traits<N>` giving `from_chars(text)`, `decompose(n)` (sign, 128-bit coefficient, exponent), `is_integer(n)` and `to_chars(span, n)`. Specializations MUST exist for `decimal64_t` and `decimal128_t`; none for floating point.

#### Scenario: A floating-point Number is refused
- **WHEN** a traits type names `double` as its `number_type`
- **THEN** it does not model `traits`, since `number_traits<double>` is not specialized

### Requirement: The regex policy concept
A regex policy `R` SHALL provide `R::compile(text, schema_allocator, step_budget)` returning `result<R::compiled<SchemaAlloc>>` and `R::search(re, text, validation_allocator)` returning `match_result` (`found`, `not_found`, `limit`). The pattern is ECMA-262 in `u` mode, unanchored, over UTF-8; the searched text MUST be well-formed UTF-8.

#### Scenario: The default policy models the concept
- **WHEN** `regex_policy<boost_regex, alloc, alloc>` is asked with the default allocator, and with a non-polymorphic validation allocator
- **THEN** both hold

### Requirement: The default regex engine
The default policy, `boost_regex` in `regex/boost_regex.hpp`, SHALL run patterns on Boost.Regex 1.92, header-only, through webcpp::lexical's public ECMA-262 translator. json_schema MUST NOT copy the translator nor include any header under `webcpp/lexical/detail/`; it MUST NOT depend on SRELL or webcpp::regex.

#### Scenario: ECMA-262 semantics in u mode
- **WHEN** `^\p{Letter}+$` is searched in "École", `b` in "abc", and `^abc$` in "abc\n"
- **THEN** the answers are `found`, `found` (unanchored) and `not_found`

#### Scenario: A counted repeat matches at its minimum
- **WHEN** `(?:ab){1,2}` is searched in "ab"
- **THEN** the answer is `found`

### Requirement: Patterns compiled once, refusals as values
Every `pattern` and every `patternProperties` key SHALL be compiled once, at schema compile, one compiled regex per distinct text. A pattern ECMA-262 refuses MUST fail the compile with `errc::invalid_regex`; a valid pattern the engine cannot run as ECMA-262 does, or past the translator's bounds, with `errc::unsupported_regex`; both with `where` the keyword's location and `what` the pattern.

#### Scenario: An invalid pattern is named by place
- **WHEN** `{"properties": {"a": {"pattern": "("}}}` is compiled
- **THEN** the result is `invalid_regex` with `where` `/properties/a/pattern` and `what` `(`

#### Scenario: An unsupported lookbehind
- **WHEN** a schema holds `"pattern": "(?<=\\$\\d*)x"`
- **THEN** the compile fails with `unsupported_regex`, naming the keyword and the pattern

### Requirement: Runaway searches are errors, never aborts
A search SHALL spend a step budget, `compile_options::regex_step_budget` in iterator steps per search, 0 meaning 10,000,000 plus 64 per byte of the text. A spent budget MUST answer `match_result::limit`, which the keyword reports as an error of kind `pattern_limit`; a search MUST NOT hang, and MUST NOT abort through Boost's backtracking stack cap.

#### Scenario: Catastrophic backtracking
- **WHEN** `(a*)*b` is searched in 40 `a`, or `(x+x+)+y` in 30 `x`
- **THEN** the answer is `limit` within a bounded time, on every target, with and without exceptions

### Requirement: The URL policy
A URL policy SHALL resolve a reference against a base (RFC 3986 5.2), normalize it (6.2.2) with the fragment kept apart, and answer `is_absolute`, writing into a `static_string<Capacity>`. The default, `boost_url<4096>`, uses Boost.URL and allocates nothing on the heap; a URI longer than the capacity MUST be `errc::invalid_uri`.

#### Scenario: A relative $id resolves against its base
- **WHEN** a resource with base `http://example.com/root.json` holds `"$id": "nested/a.json"`
- **THEN** the nested resource's URI is `http://example.com/nested/a.json`

### Requirement: Dialect tags and values
`dialect` SHALL have the fixed values `draft4` 1, `draft6` 2, `draft7` 3, `draft2019_09` 4, `draft2020_12` 5, `openapi3_0` 6, appended only (v1 when published), with one empty tag type per value whose `id` names it, and `uri_of(dialect)` giving the `$schema` URI, empty for OpenAPI 3.0.

#### Scenario: The URI of a dialect
- **WHEN** `uri_of(dialect::draft2020_12)` is called
- **THEN** it returns `https://json-schema.org/draft/2020-12/schema`

### Requirement: The dialect set is chosen at instantiation
`basic_validator<Traits, Dialects...>` SHALL accept one or more tags modelling `dialect_policy`, which reads `detail::has_machinery<D>` stated beside the tags, so its answer MUST NOT depend on include order. `validator` SHALL carry all six dialects (2020-12 first) once each has its machinery; `validator_for<D...>` a chosen set.

#### Scenario: The answer is the same whatever was included first
- **WHEN** `dialect_policy<draft2020_12>` is asked before and after `validator.hpp` is included
- **THEN** both answers are true

### Requirement: Each resource's $schema picks its dialect
A resource's dialect SHALL be its `$schema`, else the enclosing resource's, else `default_dialect`, else the validator's first dialect. A dialect outside the set MUST be `errc::unknown_dialect` with `what` the URI as written. Each official URI is accepted with and without its empty fragment; any other spelling is refused.

#### Scenario: Spellings of $schema
- **WHEN** draft 7's URI is given without `#`, and 2020-12's with `#`
- **THEN** both load in a validator holding those dialects

#### Scenario: A dialect outside the set
- **WHEN** `validator_for<draft2020_12>` loads `{"$schema": "http://json-schema.org/draft-07/schema#"}`
- **THEN** the load fails with `unknown_dialect`, `what` holding that URI

### Requirement: Exact JSON text
`js::parse(text, allocator, options)` SHALL return a `basic_parsed_json` holding the `boost::json::value` and, for every number whose double does not denote its lexeme's value, the exact `Number` keyed by its JSON Pointer; duplicate keys keep the last. Boost.JSON's recursive parser MUST be bounded by `max_depth` clamped to a depth measured safe on every target; deeper text is an error value.

#### Scenario: A bound beyond the double's precision
- **WHEN** `{"minimum": 0.10000000000000000001}` is parsed and compiled
- **THEN** the bound compares as that exact decimal, not as the double `0.1`

### Requirement: A stateless validator with load and compile
`basic_validator` SHALL hold no state and be `final`. `load` SHALL return the IR document of a schema given as a `boost::json::value` or a `basic_parsed_json`; `compile` SHALL accept the same two and a built document, whose `check()` runs first so a hand-built IR's fault is `errc::invalid_ir`, a value. A `default_dialect` outside the set MUST be `unknown_dialect`.

#### Scenario: Compiling a temporary
- **WHEN** `validator.compile(boost::json::parse(text))` is called with a temporary value
- **THEN** the compiled schema stays valid after the temporary is destroyed

### Requirement: Compile options
`basic_compile_options` SHALL hold, with designated initializers: `registry` (null), `resolver` (none), `base_uri` (empty, meaning `urn:webcpp:root`), `default_dialect` (none), `format_assertion` (false), `formats`, `content_assertion` (false), `regex_step_budget` (0), `unknown` (`dialect_default`), `compatibility_keywords` (true), `validate_schema` (true), `vocabularies` (empty), `max_schema_depth` (measured default).

#### Scenario: Default options
- **WHEN** a schema is compiled with `compile_options()`
- **THEN** it is checked against its meta-schema, formats annotate, and `definitions` and `dependencies` are honoured

### Requirement: The compiled schema
`basic_schema` SHALL be a shared handle on an immutable, self-contained compiled schema: neither the registry, nor values a resolver returned, nor the source value MUST outlive the compile. It SHALL be safe to validate with one compiled schema from many threads at once, each with its own allocator, and give `ir()` and `get_allocator()`.

#### Scenario: Eight threads share one schema
- **WHEN** eight threads validate the same instances against one compiled schema, each with its own resource
- **THEN** every verdict and every outcome is identical to a single-threaded run

### Requirement: is_valid
`basic_schema::is_valid(instance, options, allocator)` SHALL return `bool`, stop at the first failure (`allOf` at the first failing child, `anyOf` at the first passing one unless an `unevaluated*` consumer needs its annotations, `oneOf` at the second passing one), record nothing and build no message. A user keyword's own error MUST make it `false`.

#### Scenario: A failing instance
- **WHEN** `{"type": "object", "required": ["name"]}` checks `{"age": 3}` with `is_valid`
- **THEN** it returns `false`, and no error or message is built

### Requirement: validate
`basic_schema::validate(instance, options, allocator)` SHALL return `result<outcome>` collecting what `validate_options::what` asks (`errors`, `errors_and_annotations`, `everything`), evaluating failing branches in full so every error is reported. It MUST fail as a `result` only with a user keyword's own error, carrying the keyword's location; a verdict is never an error.

#### Scenario: Every error is reported
- **WHEN** `{"properties": {"a": {"type": "string"}, "b": {"minimum": 3}}}` validates `{"a": 1, "b": 1}`
- **THEN** the outcome is invalid with two errors, at `/a` and `/b`

### Requirement: The two modes agree
For every instance and schema, `is_valid` SHALL return the same verdict as `validate(...)->valid()`. The suite runner MUST check this on every test of every runner.

#### Scenario: Agreement over the suite
- **WHEN** a suite runner validates a test
- **THEN** it fails the test if `is_valid` and `validate` disagree

### Requirement: The outcome
`basic_outcome` SHALL give `valid()`, `errors()`, `annotations()`, `message(error)` (the hook's text or the template's) and `to_json(format, storage)`. It MUST keep the compiled schema alive; the instance MUST outlive it (documented), since `received()` refers into it; `to_json` copies both into the report.

#### Scenario: An outcome outlives its schema handle
- **WHEN** the last `basic_schema` handle is destroyed while an outcome is kept
- **THEN** the outcome's errors, messages and `to_json` stay valid

### Requirement: The error type
`error` SHALL give `kind()`, `keyword()` as written, `instance_location()` and `keyword_location()` as JSON Pointers (the latter the evaluation path with `$ref` segments), `absolute_keyword_location()` (canonical URI and pointer), `expected()` (the keyword's value), `received()` (into the instance) and `details()`, all views into the outcome's and schema's storage.

#### Scenario: Locations of an error through a reference
- **WHEN** `{"$ref": "#/$defs/a", "$defs": {"a": {"maxItems": 1}}}` validates `[1, 2]`
- **THEN** the error's `keyword_location` is `/$ref/maxItems` and its absolute location ends with `#/$defs/a/maxItems`

### Requirement: Error kinds are fixed values
`error_kind` SHALL hold, values fixed and appended only: `false_schema` 1, `type` 2, `enum_value` 3, `const_value` 4, `multiple_of` 5, `maximum` 6, `exclusive_maximum` 7, `minimum` 8, `exclusive_minimum` 9, `max_length` 10, `min_length` 11, `pattern` 12, `pattern_limit` 13, `format` 14, `max_items` 15, `min_items` 16, `unique_items` 17, `contains` 18, `min_contains` 19, `max_contains` 20, and the next requirement's.

#### Scenario: A runaway pattern's kind
- **WHEN** a search reaches its step budget
- **THEN** the error's kind is `pattern_limit` (13), not `pattern` (12)

### Requirement: Error kinds from 21
`error_kind` SHALL continue: `max_properties` 21, `min_properties` 22, `required` 23, `dependent_required` 24, `properties` 25, `pattern_properties` 26, `additional_properties` 27, `property_names` 28, `dependent_schemas` 29, `prefix_items` 30, `items` 31, `unevaluated_items` 32, `unevaluated_properties` 33, `all_of` 34, `any_of` 35, `one_of` 36, `not_schema` 37, `if_then_else` 38, `reference` 39, `content` 40, `schema` 41 (reserved), `user` 42, `depth_limit` 43.

#### Scenario: The reserved kind is never raised
- **WHEN** the whole suite is validated
- **THEN** no error has kind `schema`

### Requirement: Error details
`error_details` SHALL hold one alternative per kind that has more to say: the missing or offending names (`required`, `dependent_required`, `additional_*`, `unevaluated_*`), indices (`unique_items`: the two equal; `one_of`: the matching ones), a count (length, item, property or match count), or the user keyword's own `error_code`.

#### Scenario: Missing names of required
- **WHEN** `{"required": ["a", "b"]}` validates `{}`
- **THEN** the error's details hold the names `a` and `b`

### Requirement: Annotations
`annotation` SHALL give `keyword()`, `instance_location()`, `keyword_location()`, `absolute_keyword_location()` and `value()`, the schema's constant or a computed value (a set of evaluated property names). The annotations of a failed node MUST be discarded.

#### Scenario: A title is annotated
- **WHEN** `{"title": "T"}` validates `1` with `collect::errors_and_annotations`
- **THEN** `annotations()` holds one annotation with keyword `title` and value `"T"`

### Requirement: Output formats
`to_json` SHALL write 2020-12 section 12's formats: `flag` `{"valid": bool}`; `basic` a flat list of units; `detailed` the hierarchy of failing branches, a node with no children removed and one with a single child replaced by it; `verbose` the full hierarchy with `valid` on every node and annotations. A format needing more than was collected MUST be `errc::output_not_collected`.

#### Scenario: Verbose needs everything
- **WHEN** an outcome collected with `collect::errors` is asked `to_json(output_format::verbose)`
- **THEN** the result is `output_not_collected`

### Requirement: Output units
An output unit SHALL have `valid`, `keywordLocation` (with `$ref` and `$dynamicRef` segments), `absoluteKeywordLocation` (always written), `instanceLocation`, and `error` (the message) or `annotation`, with nested `errors` or `annotations`. Units MUST be written in evaluation order, byte for byte deterministic, by iterative renderers.

#### Scenario: Deterministic output
- **WHEN** the same schema validates the same instance twice and both outcomes render `basic`
- **THEN** the two serialized reports are identical

### Requirement: English messages
Each `error_kind` SHALL have an English template with named parameters in one table in `output/messages.hpp` (`type`: "Expected {expected} but found {received}."; `required`: "Required properties {names} are missing."; `max_length`: "Expected at most {expected} characters but found {count}."). A message MUST be built only when asked (`message`, `to_json`), never by `is_valid`.

#### Scenario: The default message of type
- **WHEN** `{"type": "string"}` validates `1` and `message(e)` is asked
- **THEN** the text follows the `type` template with `string` and the received type

### Requirement: The message hook
`validate_options::messages` SHALL accept a `basic_message_hook`, a `function_ref` receiving the `error` and a `basic_string` of the traits' allocator to write into; when given it MUST replace the templates entirely. `output/message_hook.hpp` holds the alias alone.

#### Scenario: A hook replaces the templates
- **WHEN** a hook writing `"bad"` is passed and a validation fails
- **THEN** every error's message is `"bad"`

### Requirement: The message catalogue is generated
`i18n/webcpp_json_schema.pot` (domain `webcpp_json_schema`, `msgctxt` the kind's name, `msgid` its template) SHALL be printed by `tools/make_pot.cpp`, which includes the template table. The build MUST compare the committed file with the program's output and fail on a difference; the file is never written by hand.

#### Scenario: A template changes without the catalogue
- **WHEN** a template in the table changes and the `.pot` is not regenerated
- **THEN** the build's check fails, naming the file

### Requirement: The Boost.Locale header
`output/locale.hpp` SHALL provide `locale_messages`, a hook translating a template with `boost::locale::translate(context = kind, template)` in the domain and filling it with `boost::locale::format`. It MUST stop compilation under `WEBCPP_JSON_SCHEMA_NO_EXCEPTIONS` with an `#error` naming Boost.Locale, link `/webcpp//boost_locale`, and run natively, on wasip2 and on wasip3.

#### Scenario: Included without exceptions
- **WHEN** a program built without exceptions includes `output/locale.hpp`
- **THEN** compilation stops with the message that it needs exceptions because Boost.Locale throws

### Requirement: The resolver
`resolver_ref` SHALL be a `function_ref` of `result<boost::json::value>(std::string_view uri, const storage_ptr& storage)`, called synchronously, building its value in the schema's storage. The library MUST NOT touch network, disk or clock itself.

#### Scenario: A resolver serves an unknown URI
- **WHEN** a schema refers to `https://example.com/s.json`, unknown elsewhere, and a resolver returns a schema for it
- **THEN** the compile succeeds and the reference validates against that schema

### Requirement: The registry
`basic_registry` SHALL register documents under their retrieval URIs (`add` of a value or a parsed JSON, copied into the registry's allocator), answer `contains`, move taking the source's allocator and never be copied. It MAY cache compiled meta-schemas for later compiles; it is the user's object and MUST NOT be written by two threads at once.

#### Scenario: The suite's remotes
- **WHEN** the suite's `remotes/` are added to a registry under `http://localhost:1234/`
- **THEN** cases referring to them compile with that registry in the options

### Requirement: Reference lookup order
A reference's absolute URI, fragment removed, SHALL be looked up among the document's resources, then in `compile_options::registry`, then among the bundled meta-schemas, then through `compile_options::resolver`, called once per URI. A resolver error MUST be `errc::resolver_failed` naming the URI; no resource found MUST be `errc::unresolvable_reference` naming it.

#### Scenario: No resolver and an unknown URI
- **WHEN** a schema refers to `https://example.com/missing.json` with no registry entry and no resolver
- **THEN** the compile fails with `unresolvable_reference`, `where` holding that URI

### Requirement: Base URIs and resources
The root's base URI SHALL be `compile_options::base_uri`, else `urn:webcpp:root`. Each `$id` (draft 4's `id`) without a fragment MUST open a resource whose URI is resolved against the enclosing base and normalized; in 2019-09 and 2020-12 an empty fragment is dropped and a non-empty one is `errc::invalid_uri`. Two resources with one URI are `errc::duplicate_resource`.

#### Scenario: A fragment in a 2020-12 $id
- **WHEN** a 2020-12 schema holds `"$id": "http://example.com/a.json#foo"`
- **THEN** the load fails with `invalid_uri`

### Requirement: Anchors
A node SHALL be reachable by `$anchor` (2019-09, 2020-12), by drafts 4 to 7's `"$id": "#name"`, by `$dynamicAnchor` (2020-12, also a plain anchor of the same name) and by `$recursiveAnchor: true` (2019-09, a dynamic anchor named ""). Two anchors of one name and kind in one resource MUST be `errc::duplicate_anchor`.

#### Scenario: A plain-name fragment
- **WHEN** `{"$ref": "#item", "$defs": {"a": {"$anchor": "item", "type": "integer"}}}` validates `"x"`
- **THEN** the instance is invalid with a `type` error reached through the anchor

### Requirement: JSON Pointer fragments
A JSON Pointer fragment SHALL resolve within the resource the URI names, following 2020-12 core 9.2.1 when it crosses into an embedded resource. A fragment that is not a JSON Pointer, or points at nothing, MUST be `errc::invalid_json_pointer`.

#### Scenario: A pointer to nothing
- **WHEN** a schema holds `{"$ref": "#/$defs/missing"}`
- **THEN** the compile fails with `invalid_json_pointer`, naming the reference's location

### Requirement: Static references
`$ref` SHALL be resolved at compile into a direct jump to its target's block; a recursive schema MUST become a loop of the plan, and no URI is looked up after load. A node referenced from many places exists once.

#### Scenario: A recursive tree
- **WHEN** `{"type": "object", "properties": {"child": {"$ref": "#"}}}` validates an object nested 50 levels through `child`
- **THEN** the verdict is computed without recursion on the C++ stack

### Requirement: Dynamic references
A `$dynamicRef` whose initial target carries the matching `$dynamicAnchor` (bookended) SHALL compile to a runtime lookup taking the outermost resource of the dynamic scope that holds a dynamic anchor of that name; otherwise it MUST compile to a plain jump. `$recursiveRef` SHALL apply 2019-09's `$recursiveAnchor` rule. Only then does the plan track the dynamic scope.

#### Scenario: The suite's dynamicRef cases
- **WHEN** the 2020-12 runner runs `dynamicRef.json`
- **THEN** every test passes

### Requirement: Reference cycles that consume nothing are refused
Before lowering, the compiler SHALL build the graph of in-place edges (the in-place applicators, `not`, and every candidate of a `$dynamicRef`) and refuse a cycle made of them alone with `errc::reference_cycle`, naming the keyword locations around it. Reaching one node twice without such a cycle MUST be accepted.

#### Scenario: A self-reference that consumes nothing
- **WHEN** `{"$ref": "#"}` is compiled
- **THEN** the compile fails with `reference_cycle`

### Requirement: Bundled meta-schemas
The official meta-schemas of 2020-12 and its vocabularies, 2019-09 and its vocabularies, drafts 7, 6 and 4, and OpenAPI 3.0's Schema Object SHALL be text in generated headers under `detail/meta_schemas/`, written by `meta_schemas/generate.py` from fetched files whose URL, date and SHA-256 its README records. A check MUST fail the build when a header differs; they are parsed only when referenced and never validated against themselves.

#### Scenario: A header edited by hand
- **WHEN** a generated meta-schema header is edited without the generator
- **THEN** the generated-file check fails, naming it

### Requirement: Meta-schema validation at compile
With `validate_schema` (the default), `compile` SHALL validate each resource against its meta-schema before the walk; a violation MUST be `errc::meta_schema_violation` with `where` the first error's instance location in the schema and `what` its keyword. With it off, the loader refuses shapes the meta-schema would refuse with `errc::invalid_keyword_value`.

#### Scenario: A negative maxLength
- **WHEN** `{"maxLength": -1}` is compiled with the default options
- **THEN** the compile fails with `meta_schema_violation`, `where` `/maxLength`

### Requirement: Vocabularies of a meta-schema
`$vocabulary` SHALL be read only from a meta-schema (2019-09, 2020-12). A required vocabulary the validator does not know, official or user, MUST be `errc::unknown_vocabulary` naming its URI; an optional unknown one is ignored; a known vocabulary left out turns its keywords `unknown` in every resource using that meta-schema; `format-assertion` turns format assertion on. Vocabularies are not inherited through `$ref`.

#### Scenario: A required unknown vocabulary
- **WHEN** a schema's custom meta-schema requires `https://example.com/vocab/x` and no such vocabulary is registered
- **THEN** the load fails with `unknown_vocabulary` naming that URI

### Requirement: Unknown keywords
An unknown keyword SHALL be, by `unknown_keywords::dialect_default`, an annotation in 2019-09 and 2020-12, inert in drafts 4 to 7, and `errc::unknown_keyword` in OpenAPI 3.0 unless it starts with `x-`. The option MUST also offer `annotate`, `ignore` and `reject`. A schema inside an unknown keyword is loaded only when a reference reaches it, and an `$id` inside it registers no resource.

#### Scenario: A non-extension keyword in OpenAPI 3.0
- **WHEN** an OpenAPI 3.0 schema holds `{"foo": 1}` and meta-schema validation is off
- **THEN** the load fails with `unknown_keyword` naming `foo`

### Requirement: The IR's shape
The IR (`webcpp::json_schema::ir`) SHALL hold nodes (`always_valid`, `never_valid`, `object`; a resource, a pointer, a contiguous keyword range), resources (URI, source dialect, root, parent, vocabularies, recursive anchor), anchors, and pointers as steps (parent, name or element index), named only by index types (`node_id`, `keyword_id`, `resource_id`, `string_id`, `value_id`, `number_id`, `pointer_id`). It MUST hold no pointer inside.

#### Scenario: A copy with another allocator
- **WHEN** a document is copied into another allocator and the first is destroyed
- **THEN** the copy is complete and `check()` accepts it

### Requirement: Keyword kinds and payloads
`keyword_kind` SHALL take the values of `reference/spec.md` section 5.2 (`ref` 1 to `user` 92), each with its payload alternative (`subschema`, `subschema_list`, `named_subschemas`, `reference`, `dynamic_reference`, `types`, `constant`, `constants`, `bound`, `count`, `text`, `format_payload`, `names`, `dependencies`, `contains_payload`, `user_payload`). Values MUST be fixed, appended only.

#### Scenario: The values are pinned
- **WHEN** `ir_test` reads `keyword_kind::prefix_items` and `keyword_kind::unevaluated_properties`
- **THEN** they are 18 and 31

### Requirement: IR invariants are checked
`basic_document::check()` SHALL verify invariants 1 to 11, 13 and 15 of `reference/spec.md` 5.3 (closed ids, resolved references, one keyword per kind per node, distinct names, exact finite numbers, valid UTF-8, unique resource URIs, unique anchors, values owned or named in an earlier value, relocatable, pointers as steps), returning `errc::invalid_ir` naming the fault, at a cost linear in the document times a logarithm.

#### Scenario: A dangling node id
- **WHEN** a hand-built document names a `node_id` it never defined and is compiled
- **THEN** the compile fails with `invalid_ir`, naming the place

### Requirement: The IR is dialect-neutral
No consumer of the IR SHALL read `resource::source_dialect` or `resource::vocabularies` for semantics; every semantic difference between dialects MUST be an explicit field (`types::integer_by_value`, `contains_payload::annotates_items`, `format_payload::assertion`, `dynamic_reference::form` and `bookended`, the `ignored` and `unknown` kinds). The dialect serves only the emitter, `$schema` in output, and `explain`.

#### Scenario: The same IR from two drafts
- **WHEN** draft 7's `{"items": [{}], "additionalItems": false}` and 2020-12's `{"prefixItems": [{}], "items": false}` are loaded
- **THEN** both yield a `prefix_items` and an `items` keyword with the same subschemas

### Requirement: Building an IR in code
`basic_document` SHALL offer `reserve`, `add_pointer`, `define` (once per node, keywords contiguous), `set_root`, `add_resource`, `add_anchor`, `intern`, `add_value` (copied), `add_value_at` (named inside an earlier value, resolved through Boost.JSON's object index, never by scanning keys) and the `add_*` range builders. A node built in code MUST get a synthesized pointer through its parent's keyword.

#### Scenario: zod's path
- **WHEN** a document built in code is passed to `compile`
- **THEN** it is checked, compiled, and its errors carry synthesized locations such as `/properties/a/items`

### Requirement: The contract freeze
At the end of E1 the IR headers (`ir/ids.hpp`, `ir/number.hpp`, `ir/keyword.hpp`, `ir/node.hpp`, `ir/document.hpp`), `detail/vm/event.hpp`, `detail/vm/sink.hpp`, `detail/output/record.hpp`, `output/error.hpp`, `output/outcome.hpp` and `output/message_hook.hpp` SHALL freeze under the tag `contract-1`. A later change MUST be proposed and applied to every open branch at once; appending an enum value is not a change.

#### Scenario: A frozen file changes on one branch
- **WHEN** a sub-track's merge finds a frozen file changed since `contract-1` without approval
- **THEN** the merge stops before it fast-forwards

### Requirement: Structural equivalence of IRs
`ir::equivalent(a, node, b, node)` SHALL compare two graphs structurally (kinds, payloads, constants by JSON equality, numbers by value, the reached graph including cycles), ignoring names, sources, pointers, resource URIs, `defs` and `ignored` keywords, and treating a lone `ref` (and an `all_of` of one) as transparent.

#### Scenario: Equivalence after a round trip
- **WHEN** a schema is loaded, emitted in its own dialect and reloaded
- **THEN** `equivalent` holds between the two roots

### Requirement: The loader walks iteratively
`load` SHALL walk the schema with an explicit work list in the document's allocator: determine each resource's dialect on entry, reserve a node, record `$id`, anchors and `$schema`, read every member through the dialect's keyword table, then resolve every reference until none is open. Nesting past `max_schema_depth` MUST be `errc::schema_too_deep`.

#### Scenario: A schema nested 100,000 levels
- **WHEN** a schema of 100,000 nested `not` is loaded
- **THEN** the load fails with `schema_too_deep`, without a stack overflow

### Requirement: Boolean schemas
A boolean schema SHALL be an `always_valid` or `never_valid` node in drafts 6, 7, 2019-09 and 2020-12, and MUST be `errc::not_a_schema` in draft 4 and OpenAPI 3.0, as is a value that is neither object nor boolean where a schema must be.

#### Scenario: false in draft 4
- **WHEN** a draft 4 schema holds `{"not": false}`
- **THEN** the load fails with `not_a_schema`

### Requirement: Normalizing items
In drafts 4, 6, 7 and 2019-09, an array `items` SHALL load as `prefix_items` named `items`, a schema `items` as `items`, and `additionalItems` beside an array `items` as `items` named `additionalItems`; `additionalItems` beside a schema `items`, or alone, MUST load as `ignored`. In 2020-12, `prefixItems` and `items` load as themselves.

#### Scenario: additionalItems alone
- **WHEN** draft 7's `{"additionalItems": false}` validates `[1]`
- **THEN** the instance is valid

### Requirement: Normalizing exclusive bounds of draft 4
In draft 4 and OpenAPI 3.0, `maximum: n` with `exclusiveMaximum: true` SHALL load as `exclusive_maximum n` named `maximum`, the boolean kept as `ignored`; `exclusiveMaximum: false`, or `true` without `maximum`, keeps `maximum` as written with the boolean `ignored`; the minimum pair likewise. In drafts 6 on, the numeric form MUST load as itself.

#### Scenario: Draft 4's exclusive maximum
- **WHEN** draft 4's `{"maximum": 3, "exclusiveMaximum": true}` validates `3`
- **THEN** the instance is invalid

### Requirement: Compatibility keywords
`definitions` SHALL load as `defs` named `definitions` in drafts 4, 6, 7, and in 2019-09 and 2020-12 when `compatibility_keywords` is on (the default); `dependencies` MUST split each member into a `dependent_required` (array) or `dependent_schemas` (schema) entry named `dependencies`, under the same rule. With the option off they load as `unknown`.

#### Scenario: dependencies in 2020-12
- **WHEN** 2020-12's `{"dependencies": {"a": ["b"]}}` validates `{"a": 1}` with default options
- **THEN** the instance is invalid with a `dependent_required` error naming `b`

### Requirement: References hide their siblings up to draft 7
In drafts 4, 6, 7 and OpenAPI 3.0 a `$ref` SHALL make every sibling keyword `ignored`; in 2019-09 and 2020-12 siblings MUST be evaluated with the reference.

#### Scenario: A sibling of $ref in draft 7
- **WHEN** draft 7's `{"$ref": "#/definitions/a", "maxItems": 0, "definitions": {"a": {}}}` validates `[1]`
- **THEN** the instance is valid

### Requirement: Integers and contains per dialect
`type` with `integer` SHALL set `integer_by_value` false in draft 4 (`1.0` is not an integer) and true elsewhere. `contains` SHALL set `annotates_items` true only in 2020-12. `minContains` and `maxContains` MUST load in 2019-09 and 2020-12.

#### Scenario: 1.0 as an integer
- **WHEN** `{"type": "integer"}` validates `1.0` in draft 7 and in draft 4
- **THEN** it is valid in draft 7 and invalid in draft 4

### Requirement: OpenAPI 3.0's Schema Object
In OpenAPI 3.0, `nullable: true` beside `type` SHALL add `null` to the type set and keep the keyword as `ignored`; `nullable` without `type` is `ignored`; `discriminator`, `example`, `xml` and `externalDocs` MUST load as annotations. OpenAPI 3.0 is chosen only by `default_dialect` or by standing first in the list.

#### Scenario: A nullable string
- **WHEN** an OpenAPI 3.0 schema `{"type": "string", "nullable": true}` validates `null`
- **THEN** the instance is valid

### Requirement: Exact numbers inside nested resources
The loader SHALL look a number up in `parse`'s table of exact numbers by its pointer from the document's root, and refusals MUST name that pointer, while a node's `pointer` is from its resource's root; the walk carries both when a nested resource starts its own chain.

#### Scenario: Two bounds at the same relative path
- **WHEN** `/minimum` is `0.10000000000000000001` and an embedded resource's `/minimum` is `0.1`
- **THEN** each bound keeps its own exact value

### Requirement: The compiled plan
`compile` SHALL lower the IR into a plan in the schema's allocator: a flat instruction list (`op`, `flags`, `small`, `a`, `b`, `location`) grouped in blocks, one per node reached, numbered in breadth-first order from the root, with tables of operands, keyword locations and compiled patterns. The plan MUST hold indices only, so it can be copied whole.

#### Scenario: The plan of a small schema
- **WHEN** `{"type": "string", "maxLength": 3}` is compiled and explained
- **THEN** block 0 holds `assert_type string`, `type_guard string 1`, `assert_length max 3` and `ret`, each with its location

### Requirement: Lowering keywords to instructions
Each keyword that applies to one instance type SHALL open with `type_guard <type> <n>`, skipping its `n` instructions for another type. `properties`, `patternProperties` and `additionalProperties` MUST lower to one `loop_properties`; `prefixItems` and `items` to one `loop_items`; `if`, `then`, `else` to one `if_then_else`; `contains` with its bounds to one `loop_contains`; a `false` node to `fail`.

#### Scenario: Fused property loops
- **WHEN** `{"properties": {"a": {}}, "additionalProperties": false}` is compiled
- **THEN** the root block holds one `loop_properties` naming one name and an additional block

### Requirement: Opcode values are fixed
`detail::opcode` SHALL number instructions from 0 (`ret` 0, `type_guard` 1, `fail` 2, `assert_type` 3 to `assert_dependent_required` 17, `all_of` 18 to `if_then_else` 22, `loop_properties` 23 to `loop_contains` 27), each later instruction (`annotate`, `jump`, `dynamic_jump`, `enter_resource`, `leave_resource`, `mark_evaluated`, `loop_unevaluated_*`, `assert_format`, `assert_content`, `call_user`, `assert_enum_hash`) MUST take a new value.

#### Scenario: opcode_name
- **WHEN** `opcode_name(opcode::not_)` is called
- **THEN** it returns `not`

### Requirement: The VM evaluates iteratively
`detail::machine<Traits, Sink>` SHALL run a block on an explicit stack of frames (block, instruction, instance value, path token, loop state, partial result, sink mark) in the validation's allocator with an inline buffer first; an applicator pushes a child frame and resumes on its result. No instance or schema depth MUST reach the C++ stack.

#### Scenario: A deep instance against a recursive schema
- **WHEN** an array nested 100,000 levels is validated against `{"items": {"$ref": "#"}}`
- **THEN** the run ends with a `depth_limit` error and no stack overflow

### Requirement: The evaluation sink
The VM SHALL report through a sink modelling `detail::evaluation_sink`: `records`, `enter(at)`, `leave(at, valid)`, `fail(error)`, `annotate(annotation)`, `checkpoint()` and `discard(mark)`. `flag_sink` (`records` false) MUST skip every call and allow short-circuiting; `collecting_sink` records what `collect` asks into the outcome's storage, building JSON Pointers only when it records.

#### Scenario: A failing anyOf branch is discarded
- **WHEN** `{"anyOf": [{"type": "string"}, {"type": "integer"}]}` validates `1` with `validate`
- **THEN** the outcome is valid and holds no error from the first branch

### Requirement: unevaluated keywords
The compiler SHALL find each `unevaluatedProperties` or `unevaluatedItems` node's contributors (its own keywords and every in-place applicator reachable from it, never through `not`); only they emit `mark_evaluated`, with checkpoint and rewind dropping a failed branch's marks, and the loop MUST apply its schema to every member or item without a mark. A schema without `unevaluated*` pays nothing.

#### Scenario: Properties evaluated inside allOf
- **WHEN** `{"allOf": [{"properties": {"a": {}}}], "unevaluatedProperties": false}` validates `{"a": 1}` and `{"a": 1, "b": 2}`
- **THEN** the first is valid and the second invalid, naming `b`

### Requirement: The instance depth limit
`validate_options::max_depth` SHALL bound instance nesting: descending past it MUST be an error of kind `depth_limit` at that instance location, failing that branch. `default_max_depth` is measured: the largest power of two at which validating the 2020-12 meta-schema against an instance nested that deep uses at most 1 MiB of validation memory on wasip2 at `-O0`.

#### Scenario: A limit given by the caller
- **WHEN** `max_depth` is 8 and an instance nests arrays 9 deep against `{"items": {"$ref": "#"}}`
- **THEN** the outcome holds a `depth_limit` error at the ninth level

### Requirement: Optimizations change no verdict
The compiler's optimizations (fused type checks, a large `enum` as a hash table consistent with JSON equality, `required` fused with `properties`, cheap checks first, folded boolean subschemas, inlined small non-recursive `$ref` targets, a static `unevaluated*` filter) SHALL apply to the flag plan `is_valid` runs; `validate` MUST run a record plan they leave unchanged in what it records. The suite is the oracle.

#### Scenario: The suite before and after
- **WHEN** the suite runs with optimizations on
- **THEN** every verdict, every error and its order are the same as without them

### Requirement: explain
`basic_schema::explain(allocator)` SHALL return the plan as text, each block's header from its node's resource URI and pointer, each instruction on one line with its index, opcode name, operands in words and ` ; ` followed by its keyword's absolute location.

#### Scenario: Blocks in breadth-first order
- **WHEN** a schema with `properties` and `allOf` children is explained
- **THEN** the root is block 0, then the children in the order the walk reaches them

### Requirement: Schema numbers are exact
Every `bound` SHALL be exact: an integer that fits stays `int64_t` or `uint64_t`, any other value is a `Number` from the lexeme when the schema came from `parse`, else from the double's shortest round-trip text. `multipleOf` MUST be greater than zero. A schema number the `Number` cannot hold (`1e400` in `decimal64_t`) MUST be `errc::invalid_number`.

#### Scenario: A bound beyond decimal64_t
- **WHEN** `{"maximum": 1e400}` is compiled with the default traits
- **THEN** the compile fails with `invalid_number`

### Requirement: Instance numbers compare exactly
An instance `int64_t` or `uint64_t` SHALL be compared exactly. An instance double MUST be read once as the decimal its shortest round-trip text denotes (at most 17 digits, in a 128-bit coefficient) and compared exactly with the schema's number, never rounded to `Number`. Integer-to-decimal comparisons run on the decomposition.

#### Scenario: Edges of the representations
- **WHEN** the double 2^64 checks `maximum: 18446744073709551615`, and `1.1` checks `exclusiveMinimum: 1.1`
- **THEN** both are invalid

### Requirement: multipleOf without floating point
`multipleOf` SHALL be `x % y == 0` for two integers; otherwise, with `x = cx * 10^ex` and `y = cy * 10^ey`, it MUST be decided exactly by modular arithmetic in 128 bits (a mulmod by doubling past them for `decimal128_t`), never by `fmod`.

#### Scenario: Small multiples
- **WHEN** `0.0075` checks `multipleOf: 0.0001`, and `1e308` checks `multipleOf: 0.0001`
- **THEN** both are valid, without overflow

### Requirement: Equality and string length
`const`, `enum` and `uniqueItems` SHALL compare numbers by mathematical value across kinds (`1` equals `1.0`), objects ignoring member order, arrays in order, strings by bytes. String length MUST count code points, the bytes outside `0x80` to `0xBF`.

#### Scenario: A non-BMP character
- **WHEN** `{"maxLength": 1}` validates a string holding one emoji of four UTF-8 bytes
- **THEN** the instance is valid

### Requirement: The format set
`compile_options::formats` SHALL be a local, extensible span of `format_entry` (a name and a `noexcept` check over `std::string_view`, plus an optional `check_in` taking the validation's resource); `default_formats()` MUST name webcpp::lexical's `is_*` functions for every format of every dialect, `regex` through `<webcpp/lexical/regex.hpp>`. Its functions must outlive the schemas compiled with them.

#### Scenario: A user format
- **WHEN** a set holding `{"name": "even", ...}` is passed with `format_assertion`, and `{"format": "even"}` validates `"3"`
- **THEN** the instance is invalid with an error of kind `format`

### Requirement: Format as annotation or assertion
A `format` SHALL assert when `format_payload::assertion` is set (the `format-assertion` vocabulary) or `format_assertion` is on, checking strings only and failing with kind `format`; otherwise it MUST annotate its name. An unknown format name is an annotation in both modes.

#### Scenario: An invalid date by default and on request
- **WHEN** `{"format": "date"}` validates `"2026-13-01"` with default options, then with `format_assertion`
- **THEN** it is valid the first time and invalid the second

### Requirement: Content keywords
`contentEncoding`, `contentMediaType` and `contentSchema` SHALL annotate from 2019-09 on (and draft 7's pair). With `content_assertion`, `base64` MUST be decoded, `application/json` parsed with Boost.JSON into the validation's allocator, and `contentSchema` applied to the decoded value; a failure is kind `content`.

#### Scenario: Asserted JSON content
- **WHEN** `{"contentMediaType": "application/json"}` validates `"{"` with `content_assertion`
- **THEN** the instance is invalid with an error of kind `content`

### Requirement: User vocabularies
A program SHALL register vocabularies in `compile_options::vocabularies`: a URI, keywords (`name`, `check` at load, `validate`, `applies_to` types, `annotates`, optional `write`) and an activation, `always` or `when_declared` (only where the meta-schema lists its URI). A user keyword MUST load as `keyword_kind::user` and compile to `call_user`, its value data, never a subschema.

#### Scenario: A keyword of a user vocabulary
- **WHEN** a vocabulary declares `even` validating integers and `{"even": true}` validates `3`
- **THEN** the outcome holds an error of kind `user` at `/even`

### Requirement: User keyword failures and conflicts
A user keyword's `check` error SHALL refuse the load naming the keyword. Its `validate` error MUST make `validate()` fail with that error at the keyword's location and `is_valid` answer `false`. A name an active official keyword or another user keyword already takes is `errc::vocabulary_conflict`.

#### Scenario: A user keyword named type
- **WHEN** a vocabulary active always declares a keyword `type`
- **THEN** the load fails with `vocabulary_conflict`

### Requirement: The emitter writes each dialect's forms
`js::emit(document, [root,] dialect, options)` SHALL write the target's forms: `$defs` or `definitions` (OpenAPI 3.0: `emitted::components`), `prefixItems` or array `items` with `additionalItems`, numeric or draft 4's boolean `exclusiveMaximum`, `$dynamicRef` or `$recursiveRef`, split dependents or `dependencies`, `const` or a one-value `enum`, boolean nodes or `{}`/`{"not": {}}`, `nullable` for OpenAPI 3.0.

#### Scenario: 2020-12 emitted as draft 7
- **WHEN** `{"prefixItems": [{}], "items": false}` is emitted as draft 7
- **THEN** the result is `{"items": [{}], "additionalItems": false}` with draft 7's `$schema`

### Requirement: Unrepresentable features
What a target cannot express SHALL be `errc::unrepresentable` naming the node by default; under `unrepresentable::drop` the keyword MUST be left out and its name added to the node's `"x-webcpp-dropped"` array. A `$ref` with evaluated siblings is written as `allOf` of the reference and the siblings in drafts 4 to 7 and OpenAPI 3.0.

#### Scenario: unevaluatedProperties to draft 7
- **WHEN** `{"unevaluatedProperties": false}` is emitted as draft 7 with the default options
- **THEN** the result is `unrepresentable`, naming the node

### Requirement: Shared nodes, cycles and order in emitted schemas
A node reached more than once, or by a cycle, SHALL be written once under the target's definitions (its `defs` name, else `__schema<N>` in order of first reach) and referenced by `$ref`. Keys MUST follow the IR's order with `$schema`, `$id`, `$anchor`, `$dynamicAnchor` first, identical byte for byte for the same IR; the emitter is iterative.

#### Scenario: Two emits of one IR
- **WHEN** one document is emitted twice into the same dialect
- **THEN** the two serialized results are identical

### Requirement: The round trip
Every suite schema of every runner SHALL load, emit into its own dialect, reload to an equivalent IR, and give every test the same verdict. Emitting into each other dialect MUST give the same verdicts where `emit` succeeds; an unrepresentable case is counted and reported, not failed.

#### Scenario: The 2020-12 suite round trip
- **WHEN** the round-trip mode of the 2020-12 runner runs
- **THEN** every case reloads equivalent and every verdict is unchanged

### Requirement: Two allocators
The schema's allocator, given to `load` and `compile`, SHALL hold the IR, the plan, constants, the translator's memory for patterns and the shared control block; the validation's, given to `validate` and `is_valid`, the frames, marks, dynamic scope, events, strings, messages and a search's state. Nothing MUST allocate outside them except what the next requirement states.

#### Scenario: Counting resources upstream
- **WHEN** a schema without patterns is parsed, compiled and validated with counting resources and a replaced global `operator new`
- **THEN** the global count is zero

### Requirement: Stated allocation exceptions
The allocations outside the two allocators SHALL be stated on the page: the user's resolver, hook, format and vocabulary functions; Boost.Locale in the optional header; and Boost.Regex's compiled program (7 to 9 global allocations per pattern) and backtracking stack (4 KiB blocks, a process-wide cache). The allocation test MUST measure them, never hide them.

#### Scenario: A pattern's compile
- **WHEN** the allocation test compiles `{"pattern": "^[a-z]+$"}`
- **THEN** it reports Boost.Regex's global allocations as the stated exception, and the translator's memory in the schema's resource

### Requirement: is_valid allocates nothing on the common path
`is_valid` SHALL allocate nothing when the instance nests at most 32 levels and the schema holds no pattern: its frames, marks and dynamic scope start in inline buffers, and only a nesting past them allocates, in the validation's allocator.

#### Scenario: A shallow instance
- **WHEN** `is_valid` checks a 10-level instance against a pattern-free schema with a counting resource
- **THEN** the resource counts zero allocations

### Requirement: Boost.JSON storage through an adapter
Boost.JSON SHALL allocate through `detail::json_resource`, a header-only adapter from `std::pmr::memory_resource` to Boost.JSON's resource, held inside the compiled schema or the outcome and given as a non-owning `storage_ptr`; `boost::container::pmr`'s compiled globals MUST NOT be used. A non-pmr allocator type gets the same adapter.

#### Scenario: A non-polymorphic allocator
- **WHEN** a traits type names a custom allocator of `std::byte`
- **THEN** the library compiles and allocates through it, every block given back at its size and alignment

### Requirement: No recursion on input depth
No function of the library SHALL recurse on the depth of a JSON value, a schema, an IR or an output: the loader, compiler, VM, equality, hashing, copies, renderers, emitter, `equivalent` and `check` are iterative. A value MUST be copied only by `detail::deep_copy`, into a trivially deallocating arena, never by Boost.JSON's copy constructor.

#### Scenario: Equality of deep values
- **WHEN** two arrays nested 1,000,000 levels are compared by `const`
- **THEN** the comparison finishes without a stack overflow on every target

### Requirement: Targets
Every test and example SHALL declare native, wasip2, wasip3 and Emscripten, unless its Jamfile states why not (the threads test native only; the locale header native, wasip2 and wasip3; the fuzz lane Linux clang; the Bowtie lane Docker). On WASI the suite MUST reach a runner through `webcpp.data` (wasmtime's `--dir`); on Emscripten through `-sNODERAWFS=1`.

#### Scenario: The suite on wasip2
- **WHEN** `b2 toolset=clang-wasip2 testing.launcher=wasmtime libs/json_schema/test` runs
- **THEN** the suite runners read the suite and pass with their known-failure lists

### Requirement: Suite runners
One runner per dialect (`suite-draft2020-12`, `suite-draft2019-09`, `suite-draft7`, `suite-draft6`, `suite-draft4`, `suite-openapi3-0`) SHALL walk the pinned suite's required tests, optional groups and `optional/format/`, register the remotes, compile each case with its dialect as default and run `is_valid` and `validate`; `optional/format/` and `format-assertion.json` MUST run with `format_assertion` on. A case that fails to load fails its tests, printing the diagnostic.

#### Scenario: OpenAPI 3.0's runner
- **WHEN** `suite-openapi3-0` runs
- **THEN** it runs the draft 4 cases using only Schema Object keywords with `default_dialect = openapi3_0`, and the cases of `test/openapi/`

### Requirement: Known failures never lie
Each runner's known failures SHALL be `test/known_failures/<runner>.json`, entries of `file`, `case`, `test` and `reason`, recorded from a native run, never edited by hand. The runner MUST fail on an unlisted failing test, on a listed test that passes and on a listed test it cannot find.

#### Scenario: A fixed bug
- **WHEN** a listed test starts to pass
- **THEN** the runner fails until the entry is removed

### Requirement: Unit and proof tests
The tests SHALL include `ir_test`, a `loader_<dialect>_test` per normalization row, `numbers_test` with its exact oracle, equality, UTF-8, pointer, URL and regex tests, `compile_errors_test` (every `errc` with its `where`), `explain_test`, output and message tests, `emitter_test`, `vocabulary_test`, the annotation and output suites, headers-alone builds and the include boundaries.

#### Scenario: Every refusal is tested
- **WHEN** `compile_errors_test` runs
- **THEN** each `errc` a load or compile can return is produced once with its expected `where`

### Requirement: Hostile input
Tests SHALL prove: an instance nested 100,000 levels gives `depth_limit` without stack overflow on every target; a schema nested 100,000 levels gives `schema_too_deep`; a chain of 100,000 in-place `$ref`s compiles or fails as a value; catastrophic patterns give `pattern_limit`; an in-place reference cycle gives `reference_cycle`.

#### Scenario: The deep instance on Emscripten
- **WHEN** the hostile test runs on Emscripten
- **THEN** it passes with a `depth_limit` error

### Requirement: Fuzzing
`test/fuzz/loader_fuzz.cpp` (bytes to `parse` to `load`) and `validator_fuzz.cpp` (a schema and an instance from one input) SHALL run under libFuzzer with AddressSanitizer and UndefinedBehaviorSanitizer, corpus seeded from the suite, in an own lane `fuzz` on Linux clang, 1 to 2 minutes in CI. Each crash found MUST become a file under `test/fuzz/regressions/` replayed by an ordinary test on every target.

#### Scenario: A regression replayed
- **WHEN** a crash input is added to `test/fuzz/regressions/`
- **THEN** an ordinary test replays it on native, wasip2, wasip3 and Emscripten

### Requirement: Bowtie harness
`bowtie/bowtie_webcpp.cpp` SHALL speak IHOP v1 over standard input and output (`start`, `dialect`, `run` with its `registry`, `stop`) for every dialect of `js::validator`, and be a `webcpp.example` with an `.input` transcript and its `.expected` answers on every lane. A local lane MUST run Bowtie against its container; a pull request to Bowtie needs the owner's go.

#### Scenario: The transcript
- **WHEN** the example runs with its `.input`
- **THEN** its output equals its `.expected`

### Requirement: Benchmark
`b2 libs/json_schema/benchmark//benchmark`, never part of `test`, SHALL run sourcemeta's jsonschema-benchmark against pinned Blaze and ajv, natively and under wasmtime, on the owner's machine, writing `benchmark/results.json` (median timings per schema, machine, date, every version). The page's table and inline SVG chart MUST be generated from it, and CI fails when they differ from the committed file.

#### Scenario: A hand-edited result table
- **WHEN** `doc/benchmark.adoc` differs from what `report.py` writes from `results.json`
- **THEN** CI fails, naming the file

### Requirement: Documentation
Every public symbol SHALL have a Doc Comment, the MrDocs reference built strictly. `doc/json_schema.adoc` MUST hold its sections (introduction, dialects, compiling, validating, output and messages, formats and content, numbers, references, user vocabularies, emitting and OpenAPI 3.0, allocation and limits and errors, conformance, performance, testing, guarantees, reference), every listing a compiled example with its `.expected`.

#### Scenario: An example's output drifts
- **WHEN** an example's output no longer equals its `.expected`
- **THEN** `b2 libs/json_schema/example` fails

### Requirement: Conformance on the page
The conformance chapter SHALL give a table per dialect (required, optional and format tests, passed and known failures) computed by `doc/counts.py` from the pinned suite and the known-failure lists (passed is total minus listed, since the lists never lie), and every known failure with its reason; it MUST hold every deviation from the specification with its reason.

#### Scenario: A new known failure
- **WHEN** an entry is added to a known-failure list
- **THEN** the page's counts and its list of reasons change with it, with no run at doc time

### Requirement: Guarantees stated
The page SHALL state numbered guarantees: a compiled schema is immutable and may be shared between threads; `is_valid` and `validate` agree; `is_valid` allocates nothing on the common path; no function recurses on its input's depth; the library touches no network, disk or clock; a runaway pattern is an error, never a hang or an abort. Each MUST be proven by a named test.

#### Scenario: A guarantee without its test
- **WHEN** a guarantee is listed on the page
- **THEN** the testing chapter names the test that proves it

### Requirement: Changelog
Every change a user can see SHALL add its line in `doc/changelog.adoc` under "Unreleased", in the same commit, in Boost's release-notes form. Until first published, the entry MUST be the one line under "New Libraries": "json_schema: a header-only JSON Schema validator for C++20, every dialect from draft 4 to 2020-12 and OpenAPI 3.0, proven by the official test suite, from R."

#### Scenario: The superproject's release check
- **WHEN** `b2 check-release` runs with json_schema changed
- **THEN** it finds json_schema's line in its changelog

### Requirement: Sub-tracks end with a slice of the suite
The work SHALL proceed in sub-tracks E1 (2020-12 core, about 770 of 772 core tests and the optional `bignum`, `float-overflow`, `ecmascript-regex`, `non-bmp-regex`), E2 (2020-12 required 1301 of 1301 and the annotation suite), E3 (the output suite), E4 (the 2020-12 round trip) and E5 (every other dialect, optional and format groups), each MUST end green on every target from scratch.

#### Scenario: E1's end
- **WHEN** E1 is declared complete
- **THEN** its slice passes on native, wasip2, wasip3 and Emscripten from `b2 -a`, the lint is clean, and the contract is tagged `contract-1`

### Requirement: The snapshot follows the superproject's current rules
Before any further task, the library SHALL be brought up to the superproject's rules of 2026-10-12: `config.hpp` on `<webcpp/config.hpp>` and `WEBCPP_HAS_EXCEPTIONS`, `build.jam` with `/webcpp/config//config`, the configuration test built three times, `doc/changelog.adoc`, and the maintainer and commit identity of the superproject's `AGENTS.md`.

#### Scenario: Resuming from the snapshot
- **WHEN** the repository is recreated from the snapshot and group 0 of the tasks is done
- **THEN** every test of the snapshot passes on every target and the lint is clean, with the regex task redone
