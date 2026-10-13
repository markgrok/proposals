## Purpose

webcpp::lexical is a header-only C++20 library of webcpp's own that answers whether a string is a
well-formed instance of each of the 19 string formats JSON Schema names, exactly as its RFC or
ECMA-262 defines it, allocating nothing but where stated, on native, wasip2, wasip3 and
Emscripten, proven by the official JSON-Schema-Test-Suite and, for `regex`, by V8. These
requirements describe 0.1.0 as built (`reference/source/`).

## ADDED Requirements


### Requirement: Library: Header-only library in its namespace
The library SHALL be header-only C++20, in namespace `webcpp::lexical`, with the rules and the two shared types in `webcpp::lexical::grammar`, internals in `webcpp::lexical::detail`, public headers under `<webcpp/lexical/...>` (one per responsibility), macros prefixed `WEBCPP_LEXICAL_` and include guards `WEBCPP_LEXICAL_<PATH>_HPP` (`WEBCPP_LEXICAL_HPP`, `WEBCPP_LEXICAL_DETAIL_UTF8_HPP`). It was named formats (`webcpp::formats`, `<webcpp/formats/...>`, `WEBCPP_FORMATS_`) until it was renamed lexical, because "formats" read as Boost.Format or `std::format`; no name of the old spelling MUST remain. Its API is version 0.1.0, still unreleased, and it ports nothing: its evidence is the RFCs it cites, its unit tests, the official JSON-Schema-Test-Suite run directly against its validators, and V8 for regular expressions.

#### Scenario: A program uses the library without building it
- **WHEN** a b2 program names `/webcpp/lexical//lexical` and includes `<webcpp/lexical.hpp>`
- **THEN** it compiles and links without any library of lexical's own being built, since every header is self-contained

#### Scenario: No name of the former library remains
- **WHEN** the headers and the documentation are searched for `webcpp::formats`, `webcpp/formats` and `WEBCPP_FORMATS_`
- **THEN** none is found

### Requirement: Library: The formats and their functions
The library SHALL offer exactly these nineteen JSON Schema string formats, each as a function `is_<name>` (the format's hyphens written as underscores) taking a `std::string_view` and answering `bool`, and, but `regex`, as a rule of `webcpp::lexical::grammar`: `date-time` (`is_date_time`, RFC 3339 5.6 and 5.7), `date` (`is_date`, RFC 3339 `full-date`), `time` (`is_time`, RFC 3339 `full-time`), `duration` (`is_duration`, RFC 3339 appendix A), `email` (`is_email`, RFC 5321 4.1.2), `idn-email` (`is_idn_email`, RFC 6531 3.3), `hostname` (`is_hostname`, RFC 1123 2.1 and RFC 5890 2.3.2.1), `idn-hostname` (`is_idn_hostname`, RFC 5890 2.3.2.3 and UTS 46), `ipv4` (`is_ipv4`, RFC 2673 3.2), `ipv6` (`is_ipv6`, RFC 4291 2.2), `uri` (`is_uri`, RFC 3986 3), `uri-reference` (`is_uri_reference`, RFC 3986 4.1), `iri` (`is_iri`, RFC 3987 2.2), `iri-reference` (`is_iri_reference`, RFC 3987 2.2), `uri-template` (`is_uri_template`, RFC 6570 2), `json-pointer` (`is_json_pointer`, RFC 6901 3), `relative-json-pointer` (`is_relative_json_pointer`, draft-handrews-relative-json-pointer-01), `uuid` (`is_uuid`, RFC 9562 4) and `regex` (`is_regex`, ECMA-262 `Pattern` in `u` mode). Each validator MUST be `[[nodiscard]]`; each MUST be `noexcept` but the four IRI functions, which may throw `std::bad_alloc` through their memory resource. lexical checks each format as its reference defines it, and nothing more: a validator answers whether a string is in the format, and a rule says where in a longer input the format's text ends.

#### Scenario: The quick start program
- **WHEN** `example/lexical_quick_start.cpp` checks `1985-04-12T23:20:50.52Z`, `2021-02-29`, `joe.bloggs@[IPv6:::1]`, `xn--nxasmq6b.example`, `//example.com/p` (as uri and as uri-reference) and `/a~1b`
- **THEN** it prints `valid`, `invalid` (2021 is no leap year), `valid`, `valid`, `invalid` (a URI needs a scheme), `valid` and `valid` respectively

#### Scenario: A validator is a function of a view
- **WHEN** `lexical::is_uuid("2eb8aa08-aa98-11ea-b4aa-73b441d16380")` is called with a string literal, a `std::string` or a `std::string_view`
- **THEN** it returns `true` and takes no ownership of the argument

### Requirement: Library: Public headers and the convenience header
The library SHALL ship these public headers under `include/webcpp/lexical/`: `config.hpp`, `grammar.hpp`, `date_time.hpp`, `json_pointer.hpp`, `uuid.hpp`, `ip.hpp`, `uri.hpp`, `uri_template.hpp`, `hostname.hpp`, `email.hpp` and `regex.hpp`, and the convenience header `include/webcpp/lexical.hpp`, which MUST include `config.hpp`, `date_time.hpp`, `email.hpp`, `grammar.hpp`, `hostname.hpp`, `ip.hpp`, `json_pointer.hpp`, `uri.hpp`, `uri_template.hpp` and `uuid.hpp` but not `regex.hpp`. `regex.hpp` is included by name, because it brings the Unicode tables of ECMA-262's classes, which a program that never validates a regex should not compile. Every public header but `config.hpp` itself, and every header under `detail/`, MUST include `config.hpp` first (`config.hpp` includes `<webcpp/config.hpp>` first). The headers under `detail/` (`charsets.hpp`, `idn_scan.hpp`, `iri_mapping.hpp`, `rules.hpp`, `uri_guard.hpp`, `utf8.hpp`, `regex_validator.hpp`, `regex_tables.hpp`, `regex_unicode.hpp`) are internals, named but not frozen.

#### Scenario: The convenience header leaves the regex tables out
- **WHEN** a program includes only `<webcpp/lexical.hpp>` and calls `lexical::is_uri("http://a/")`
- **THEN** it compiles, and no header of `detail/regex_*.hpp` is included

#### Scenario: The regex header is included by name
- **WHEN** a program includes `<webcpp/lexical/regex.hpp>` and calls `lexical::is_regex("a")`
- **THEN** it compiles and returns `true`

### Requirement: Library: Build target and dependencies
`build.jam` SHALL declare `project /webcpp/lexical` and one alias `lexical` whose usage requirements are `<include>include`, `<library>/boost//headers`, `<library>/webcpp/config//config`, `<library>/webcpp/idna//idna` and `<library>/webcpp//boost_url`, between the markers `tag::dependencies[]` and `end::dependencies[]` which the documentation page includes. It MUST NOT name `/webcpp/regex//regex`, SRELL, Boost.Regex or Boost.JSON: `is_regex` is lexical's own validator and depends on no regex library, and JSON is the tests' dependency alone (`/webcpp//boost_json` in the requirements of `regex_v8` and `suite` only). The library needs Boost 1.92 or newer (Boost.URL and Boost.System), webcpp's configuration (`/webcpp/config//config`) and `webcpp::idna` for internationalized names.

#### Scenario: No regex engine in the build
- **WHEN** `build.jam` is read
- **THEN** it names `/webcpp/config//config`, `/webcpp/idna//idna` and `/webcpp//boost_url`, and names neither `/webcpp/regex//regex` nor any SRELL target

#### Scenario: One alias brings everything
- **WHEN** a program names `/webcpp/lexical//lexical`
- **THEN** the include path of lexical, Boost's headers, webcpp's configuration, idna and Boost.URL's compiled part all reach its compile and link lines

### Requirement: Library: Boost.URL's compiled part
The library MUST link `/webcpp//boost_url`, which builds Boost.URL from Boost's own sources for every target, because Boost.URL's grammar and the rules lexical uses are defined in headers (`BOOST_URL_CXX20_CONSTEXPR`) except `ipv4_address`, whose constructor from bytes and `to_bytes()` live in the compiled sources and which `ipv4_address_rule` needs. On MSVC, which never defines `BOOST_URL_HAS_CXX20_CONSTEXPR`, even the grammar's error category is defined in the compiled part, so no format links without it. The installed Boost's own `libboost_url` MUST NOT be relied on: the build uses the same sources for native, wasip2, wasip3 and emscripten.

#### Scenario: A program that validates an IPv4 address links
- **WHEN** a program calls `lexical::is_ipv4("192.168.0.1")` and links `/webcpp/lexical//lexical`
- **THEN** it links, `ipv4_address` being defined by `/webcpp//boost_url`, and returns `true`

### Requirement: Library: Boost.URL is composed and never modified
The library SHALL compose Boost.URL's grammar library and add terminals of its own; Boost.URL MUST NOT be modified, patched or copied. Where Boost.URL's verdict is wrong against the RFC, lexical guards it in `detail/uri_guard.hpp` (the guards belong to the URI formats) and offers the fix upstream, and a test pins Boost.URL's wrong verdict so that the day the configured Boost changes it, that test fails and the guard leaves in a change of its own with a changelog line. lexical never loosens Boost.URL: a verdict of Boost.URL stricter than the RFC is kept and recorded in the page's "Known deviations".

#### Scenario: A guard is retired by a failing test
- **WHEN** a Boost release makes Boost.URL refuse a `[` in a query
- **THEN** `uri_test`'s `boost_url_still_takes_brackets_in_a_query` fails, naming that the guard is no longer needed

#### Scenario: A stricter verdict is kept
- **WHEN** `is_uri("http://[V1.x]/")` is called, an `IPvFuture` led by an uppercase `V`
- **THEN** it returns `false`, as Boost.URL does, although RFC 5234 makes the ABNF's `"v"` case-insensitive, and `uri_test`'s `ipvfuture_keeps_boost_urls_verdict` pins it

### Requirement: Library: Rules are public objects of public types
Every format but `regex` SHALL also be a rule in `webcpp::lexical::grammar`: a `constexpr` object `<name>_rule` of a public type `<name>_rule_t`, so that a rule that takes a parameter is spelled without a factory (`time_rule_t{leap_seconds::refused}`, `iri_rule_t{&resource}`). The objects are `date_rule`, `time_rule`, `date_time_rule`, `duration_rule`, `uuid_rule`, `json_pointer_rule`, `relative_json_pointer_rule`, `uri_rule`, `uri_reference_rule`, `iri_rule`, `iri_reference_rule`, `uri_template_rule`, `hostname_rule`, `idn_hostname_rule`, `idna_label_rule`, `email_rule` and `idn_email_rule`. Each type MUST declare `value_type` and a member `parse(char const*& it, char const* end) const` (Boost.URL's Rule concept) that is `noexcept` but for the IRI rules'. `ipv4` and `ipv6` have no rule of lexical's: they are Boost.URL's own `boost::urls::ipv4_address_rule` and `ipv6_address_rule`, which `is_ipv4` and `is_ipv6` parse with. Rule types are documented as a contract, unlike Boost.URL's `implementation_defined`.

#### Scenario: A parameterized rule without a factory
- **WHEN** a grammar uses `fg::time_rule_t{webcpp::lexical::leap_seconds::refused}` in `variant_rule(..., fg::date_rule)` on `23:59:60Z`
- **THEN** the parse fails, as the refused policy does not take the leap second

#### Scenario: ipv4 and ipv6 are Boost.URL's rules
- **WHEN** `lexical::is_ipv6("2001:DB8::8:800:200C:417A")` is called
- **THEN** it returns `parse(s, boost::urls::ipv6_address_rule).has_value()`, which is `true`

### Requirement: Library: What a rule yields
A rule SHALL yield the text it matched, as `grammar::text`, a view of its input, with one exception: `uri_rule` and `uri_reference_rule` yield Boost.URL's `boost::urls::url_view`. Parsed values, such as a date's fields, are out of scope. `grammar::text` MUST be `boost::core::string_view` (Boost.URL's text type, which converts to `std::string_view`), declared in `<webcpp/lexical/grammar.hpp>` as `using text = boost::core::string_view;`, and `grammar::result<T>` MUST be `boost::system::result<T>`, the value a rule matched or the error that refused its input. Including `grammar.hpp` MUST NOT include any rule.

#### Scenario: A host and a port compose
- **WHEN** `urlg::parse("b\xC3\xBC" "cher.example:8080", urlg::tuple_rule(fg::idn_hostname_rule, urlg::optional_rule(urlg::tuple_rule(urlg::squelch(urlg::delim_rule(':')), urlg::unsigned_rule<std::uint16_t>{}))))` is evaluated
- **THEN** the first element of the tuple is the text `bücher.example` and the second holds the number 8080

### Requirement: Library: The rule contract, input left unchanged on failure
A rule that fails SHALL leave its iterator `it` where it was, and a rule that succeeds MUST leave it just past the text it matched. A rule reads its input in `[it, end)` alone and never the byte at or after `end`. Inside a larger grammar a rule stops where its format's text ends; where each rule stops is part of its contract: the ASCII rules where their grammar does, `hostname_rule` at the first character outside letters, digits, `-` and `.`, `idn_hostname_rule` at the first ASCII character outside those (every non-ASCII byte belongs to the name, since UTS 46 may map it), `idna_label_rule` as `idn_hostname_rule` and also at `.`, U+3002, U+FF0E and U+FF61, the e-mail rules where the domain or the address literal does, the IRI rules at the first byte no IRI holds, `uri_template_rule` at the first byte that is neither a literal nor the start of an expression, and `json_pointer_rule` at the end of the input.

#### Scenario: A rule stops where its format ends
- **WHEN** `date_time_rule` parses `2020-01-01T00:00:00Z/x`, `uuid_rule` parses `00000000-0000-0000-0000-000000000000x`, and `hostname_rule` parses `ex.ample_x`
- **THEN** they match `2020-01-01T00:00:00Z`, `00000000-0000-0000-0000-000000000000` and `ex.ample` respectively, and leave the iterator at the next byte

#### Scenario: A failed rule gives the input back
- **WHEN** `hostname_rule` parses `a..b:80`
- **THEN** it fails, and the iterator is at the start of the input

#### Scenario: A rule reads its view alone
- **WHEN** `ci_literal_rule("IPv6:")` parses `std::string_view("IPv6:", 4)`, a view that ends before the colon which follows it in memory
- **THEN** it answers `error::mismatch` and leaves the iterator at the start

### Requirement: Library: A rule fails whole rather than match a prefix
A rule SHALL validate the text it takes in full, and when that text breaks the format it MUST fail as a whole with `error::invalid` rather than match a shorter prefix, so that a grammar that composes the rule meets a failure and not a silent short match. The rules over a run (`hostname_rule`, `idn_hostname_rule`, `iri_rule`, `iri_reference_rule`, `uri_template_rule`, and the URI rules, whose guards make them refuse what Boost.URL would stop short of) take the whole run of bytes their format may hold. `error::mismatch` is reserved for text that cannot start the format at all, so that a `variant_rule` tries its next alternative.

#### Scenario: A run that breaks a constraint fails whole
- **WHEN** `idn_hostname_rule` parses `a\xC2\xB7b/x` (a middle dot between two letters, refused by IDNA's CONTEXTO rule)
- **THEN** the parse fails, and the iterator is unchanged, instead of matching `a`

#### Scenario: An IRI that holds a bidirectional mark fails whole
- **WHEN** `iri_rule` parses `http://a/\xE2\x80\x8E x`, the U+200E mark before a space
- **THEN** the parse fails, and the iterator is unchanged

### Requirement: Library: Errors are values of Boost.URL's grammar and of idna
The library SHALL have no error category of its own; a validator answers `bool` and a rule answers a `grammar::result`. A rule MUST give one of: `boost::urls::grammar::error::mismatch` (no text of the format starts where the rule was asked to match, so a `variant_rule` tries its next alternative); `boost::urls::grammar::error::invalid` (the format's text started there and broke one of its constraints, such as a month 13 or a leap second at the wrong minute); `boost::urls::grammar::error::leftover`, from Boost.URL's `parse` on a whole string when the format's text ended before the string did; or, from the rules of internationalized names, the error of `webcpp::idna` that refused the name (`idna::error::contexto`, `idna::error::hyphen_at_start_or_end`, `idna::error::bidi`), whose category is `webcpp.idna`. Boost.URL's `variant_rule` answers `mismatch` when every alternative fails, whatever each said, so lexical's e-mail rules look for `[` themselves rather than make the domain an alternative, and idna's reason reaches the caller.

#### Scenario: An internationalized rule's error is idna's
- **WHEN** `urlg::parse("a\xC2\xB7" "b", fg::idn_hostname_rule)` is evaluated
- **THEN** the error is `idna::error::contexto`

#### Scenario: An address's domain error is idna's too
- **WHEN** `urlg::parse("a@a\xC2\xB7" "b", fg::idn_email_rule)` is evaluated
- **THEN** the error's category is `idna::error_category()`

#### Scenario: A rule that fails by a bound names the grammar
- **WHEN** a grammar parses `example.com:99999` with an `unsigned_rule<std::uint16_t>` for the port
- **THEN** `example/lexical_grammar.cpp` prints `refused by boost.url.grammar`, and for `a·b.example:80` prints `refused by webcpp.idna`

### Requirement: Library: Trailing input is refused through leftover
A validator `is_x(s)` SHALL be exactly the rule parsing the whole string: `boost::urls::grammar::parse(s, x_rule).has_value()`, which fails with `error::leftover` when the rule stops before the end of `s`. So a validator refuses trailing input and a rule inside a grammar does not.

#### Scenario: Trailing bytes refused by the validator, accepted by the rule
- **WHEN** `is_uri("http://a/b c")` is called and `fg::uri_rule` is applied to the same text by `rule.parse(it, end)`
- **THEN** the validator returns `false` and the rule matches `http://a/b`, stopping before the space

### Requirement: Library: Configuration macro for exceptions
`config.hpp` SHALL include `<webcpp/config.hpp>` first and define `WEBCPP_LEXICAL_NO_EXCEPTIONS` when `WEBCPP_HAS_EXCEPTIONS` is not defined, keeping the macro when a developer defines it alone, so that a program can disable lexical's exceptions in a build that has them, and `WEBCPP_NO_EXCEPTIONS`, which a developer defines for every webcpp library alike, defines it too. Under `__MRDOCS__` the macro MUST be defined and undefined at once, so that the reference lists it while the parse goes on as a build with exceptions does. The macro changes nothing in lexical, which raises no exception of its own; it exists so that a program can set it for every webcpp library alike.

#### Scenario: The three builds of the configuration test
- **WHEN** `test/config_test.cpp` is built as the build asks (`config`), with `WEBCPP_NO_EXCEPTIONS` (`config_all_off`, which also defines `WEBCPP_TEST_LEXICAL_ALL_OFF`), and with `WEBCPP_LEXICAL_NO_EXCEPTIONS` on the command line (`config_developer_defined`, which also defines `WEBCPP_TEST_LEXICAL_DEVELOPER_DEFINED`)
- **THEN** the macro is defined exactly when `WEBCPP_HAS_EXCEPTIONS` is not in the first build, and is defined in the second and the third

#### Scenario: config.hpp includes webcpp's configuration
- **WHEN** `config_test.cpp` includes only `<webcpp/lexical/config.hpp>`
- **THEN** `WEBCPP_CONFIG_HPP` is defined, or the build fails with "config.hpp does not include <webcpp/config.hpp>"

### Requirement: Library: Nothing throws but an allocation that fails
lexical SHALL raise no exception of its own: a validator answers `false` and a rule returns its error, and the library builds without exceptions (`exception-handling=off`, wasm without exceptions). Only an allocation can throw, `std::bad_alloc` from the memory resource of an IRI function (the default resource, `std::pmr::get_default_resource()`, or the one given) or from the global `operator new`; any exception lexical raised MUST go through `boost::throw_exception`. `is_regex` allocates nothing and is `noexcept`: a pattern it refuses is answered `false`, never an exception. A resource that refuses makes `is_iri` and `is_iri_reference` throw what it throws and never answer a verdict without their mapped copy.

#### Scenario: A refusing resource reaches the caller
- **WHEN** `lexical::is_iri(beyond, *std::pmr::null_memory_resource())` is called with `http://a/` followed by 200 times `\xC3\xA9`, whose mapped URI exceeds 1,024 octets, in `iri_resource_test` (built with exceptions whatever the build asks)
- **THEN** `std::bad_alloc` is thrown, and the same for `is_iri_reference`

#### Scenario: No exception without exceptions
- **WHEN** the tests are built with `exception-handling=off`
- **THEN** every test but `iri_resource` (which is built with `<exception-handling>on`) builds and passes

### Requirement: Library: Allocation guarantees
No validator and no rule SHALL allocate, except the IRI functions and rules (`is_iri`, `is_iri_reference`, `iri_rule`, `iri_reference_rule`), which allocate nothing for an ASCII IRI or one whose mapped URI fits 1,024 octets (`detail::iri_inline_capacity`), and beyond that exactly one buffer of the mapped length, from the memory resource given, else `std::pmr::get_default_resource()`, given back before the function returns. Boost.URL's views and lexical's rules hold no storage, idna works in fixed storage, a `range_rule` keeps its rule inline (a rule larger than 128 bytes would go to Boost.URL's pool, which allocates, so lexical passes such rules through `boost::urls::grammar::ref`), and `is_regex` reads its pattern in place. The stack holds the rest: the IRI buffer of 1,024 octets, idna's working storage of a label, and the descent of `is_regex` into a pattern's groups, which `max_regex_nesting` bounds. The memory-resource overloads are `is_iri(s, std::pmr::memory_resource&)` and `is_iri_reference(s, std::pmr::memory_resource&)`, and `iri_rule_t{&resource}`, `iri_reference_rule_t{&resource}`; the default resource pointer is null.

#### Scenario: Within the inline buffer nothing is allocated
- **WHEN** `is_iri` is called on `a:` followed by 168 times `\xC3\xA9` and `\xE2\x82\xAC` (1,019 mapped octets), and on `http://example.com/` followed by 1,000,000 `a`
- **THEN** both return `true` and the global `operator new` is not called

#### Scenario: Beyond the buffer one allocation from the resource given
- **WHEN** `is_iri(beyond, resource)` is called with `http://a/` followed by 100,000 times `\xC3\xA9` and a counting resource
- **THEN** it returns `true`, the resource counts 1 allocation of 600,009 bytes and 1 deallocation

#### Scenario: Beyond the buffer, the default resource
- **WHEN** `is_iri_reference(beyond)` is called on the same text without a resource
- **THEN** it returns `true` and makes exactly one more call of the global `operator new`, given back before it returns

### Requirement: Library: The allocation test counts every call of operator new
`test/allocation_test.cpp` SHALL replace the global `operator new` and `operator delete`, plain and with `std::align_val_t`, for its own program, count every call, and check that lexical's validators and rules allocate nothing but through the resource they are given, on every target. The replaced forms MUST be `BOOST_NOINLINE` (so GCC does not pair the `malloc` with the `free` and warn), raise `std::bad_alloc` out of memory through `boost::throw_exception`, and the aligned `operator new` MUST raise the alignment to a pointer's size and round the size up (macOS's `aligned_alloc` refuses a smaller alignment), using `_aligned_malloc` under `_MSC_VER`. A direct call of `malloc`, `calloc` or `realloc` is not counted, and lexical calls none.

#### Scenario: The instrument counts
- **WHEN** a `std::string(1000, 'x')` is built and destroyed, and a `counting_resource` allocates and frees 64 bytes
- **THEN** the string makes 1 call of the global `operator new`, and the resource counts 1 allocation of 64 bytes which also makes 1 call

#### Scenario: Every alignment is counted and honoured
- **WHEN** a `counting_resource` allocates 24 bytes at each alignment 1, 2, 4, 8, 16, 32 and 64
- **THEN** each address is a multiple of its alignment, and each allocation makes exactly 1 call of the global `operator new`

#### Scenario: Exhaustion throws bad_alloc
- **WHEN** `operator new(std::numeric_limits<std::size_t>::max())` is called, plain and aligned to 64, in a build with exceptions
- **THEN** both throw `std::bad_alloc`

### Requirement: Library: Each family of formats allocates nothing on long and hostile input
`allocation_test` SHALL check, for each family, that the count of global `operator new` calls over the calls is zero: dates and times (a fraction of 100,000 `9` in a date-time, a duration of `P` and 100,000 `7` and `D`); identifiers (a JSON Pointer of 100,000 tokens `/\xC3\xA9~0`, `2/highly/nested/objects`, a UUID); addresses and URIs (a URI of 100,000 octets, `//a@b@example.com/`); host names and e-mails (a name of 100,000 octets, A-labels judged by idna, `"joe bloggs"@[IPv6:2001:db8::1]`); URI templates (100,000 expressions `{+v:12}/x`); and internationalized names, each call counted alone (`bücher.` four times plus `example`, 100,000 `a` and a `ü`, `δ@` mailboxes, a refused domain after labels already converted, and `idna_label_rule` on an A-label of 63 octets). The IRI family allocates as its own requirement says.

#### Scenario: A 100,000-digit fraction
- **WHEN** `is_date_time` is called on `1985-04-12T23:20:50.` followed by 100,000 `9` and `Z`
- **THEN** it returns `true` with no allocation

#### Scenario: A refused name allocates nothing
- **WHEN** `is_hostname` is called on a name of more than 100,000 octets made of labels of 63 `a`
- **THEN** it returns `false` with no allocation

### Requirement: Library: Every validator answers a hostile megabyte
Every validator SHALL answer an input of a megabyte without a crash and without allocating but through the resources it is given, on every target. `allocation_test` MUST call each with `1 << 20` repeated digits `7`, letters `a`, `ü` (`\303\274`, `1 << 19` times), or `{a,b}/x` (`1 << 17` times), and compare the count of global `operator new` calls with the allocations of the counting resource passed to the IRI function. `is_regex` refuses one past `max_regex_length` before it reads it.

#### Scenario: The verdicts on a megabyte
- **WHEN** the validators run on a megabyte of `7`: `is_date_time`, `is_time`, `is_duration`, `is_ipv4`, `is_ipv6`, `is_relative_json_pointer`
- **THEN** the first five answer `false` and `is_relative_json_pointer` answers `true` (an integer prefix with nothing after it)

#### Scenario: A megabyte of letters
- **WHEN** the validators run on a megabyte of `a`
- **THEN** `is_uuid`, `is_json_pointer`, `is_uri`, `is_hostname`, `is_email` and `is_regex` answer `false`, `is_uri_reference` answers `true`, and the allocations are exactly those of the counting resource

### Requirement: Library: UTF-8 rule shared by all
Every validator SHALL take any byte string, and one that is not well-formed UTF-8 (RFC 3629) MUST be invalid in every format; in the formats that are ASCII, so MUST be any byte above 0x7F. Every format that reads code points does so through lexical's own strict decoder, so that a verdict never depends on an ill-formed sequence being repaired or skipped. The rules apply the same strictness on their input range.

#### Scenario: A JSON Pointer with an ill-formed sequence
- **WHEN** `is_json_pointer` is called on `/a\xC3(` (a lead byte before an ASCII one)
- **THEN** it returns `false`

#### Scenario: An ASCII format refuses a byte above 0x7F
- **WHEN** `is_uuid` is called on a UUID in which one hexadecimal digit is replaced by `\xC3\xA9`
- **THEN** it returns `false`

### Requirement: Library: The strict UTF-8 decoder
`webcpp::lexical::detail::decode_utf8(std::string_view s, std::size_t& i, char32_t& cp)` SHALL decode the code point at `s[i]` into `cp` and move `i` past it, returning `true`; for an ill-formed sequence (truncated, overlong, a surrogate U+D800 to U+DFFF, above U+10FFFF, a lone continuation byte, or a lead byte 0xC0, 0xC1 or 0xF5 to 0xFF) or `i` at the end it MUST return `false` leaving `i` and `cp` unchanged. It is `constexpr` and `noexcept`, reads the view alone, and is lexical's own (about 20 lines) so that no library reaches into another's `detail`. RFC 3629 section 4's table is the contract.

#### Scenario: The boundaries of each length decode
- **WHEN** the decoder reads `\x00` (one byte), `\x7F`, `\xC2\x80`, `\xDF\xBF`, `\xE0\xA0\x80`, `\xED\x9F\xBF`, `\xEE\x80\x80`, `\xEF\xBF\xBF`, `\xF0\x90\x80\x80` and `\xF4\x8F\xBF\xBF`
- **THEN** it returns `U+0000`, `U+007F`, `U+0080`, `U+07FF`, `U+0800`, `U+D7FF`, `U+E000`, `U+FFFF`, `U+10000` and `U+10FFFF`, with `i` equal to each sequence's length

#### Scenario: Ill-formed sequences are refused
- **WHEN** the decoder reads `\xC0\xAF`, `\xE0\x80\xAF`, `\xF0\x80\x80\xAF`, `\xE0\x9F\xBF`, `\xED\xA0\x80`, `\xED\xBF\xBF`, `\xF4\x90\x80\x80`, `\xF5\x80\x80\x80`, `\x80`, `\xC3`, `\xE2\x82`, `\xF0\x9F\x98`, `\xFE`, `\xFF`, `\xC3\x28`, `\xE2\x82\xE2` or `\xF4\x8F\xBF\xC0`
- **THEN** it returns `false`, and `i` stays 0 and `cp` keeps its value

#### Scenario: A sequence decodes where the index says
- **WHEN** the decoder reads `a\xC3\xA9z` with `i` equal to 1
- **THEN** it returns `true` with `cp` equal to `U+00E9` and `i` equal to 3; and at `i` equal to the size it returns `false`

#### Scenario: A sequence ends with its view
- **WHEN** the decoder reads `\xC3\xA9`, `\xE2\x82\xAC` or `\xF0\x9F\x98\x80` cut one byte short, although the missing byte follows in memory
- **THEN** it returns `false` with `i` equal to 0

### Requirement: Library: Shared character sets and code point classes
`detail/charsets.hpp` SHALL hold the character sets and code point classes that formats share: `detail::is_ucschar` (RFC 3987's `ucschar`: U+00A0 to U+D7FF, U+F900 to U+FDCF, U+FDF0 to U+FFEF, planes 1 to 13 each but their last two code points, and plane 14 from U+E1000 but its last two), `detail::is_iprivate` (U+E000 to U+F8FF, and planes 15 and 16 but their last two code points), `detail::is_bidi_format` (U+200E, U+200F, U+202A to U+202E), and the ASCII sets `ldh_chars`, `hostname_chars` (letters, digits, `-` and `.`), `atext_chars`, `qtext_smtp_chars`, `quoted_pair_chars`, `template_literal_chars` (RFC 6570's literals with erratum 6937, which makes the apostrophe one), `varchar_chars` and `template_operator_chars`.

#### Scenario: ucschar at the edges of its ranges
- **WHEN** `is_ucschar` is called on U+00A0, U+D7FF, U+F900, U+FDCF, U+FDF0, U+FFEF, U+10000, U+1FFFD, U+20000, U+DFFFD, U+E1000 and U+EFFFD
- **THEN** it returns `true`; and `false` for U+007F, U+009F, U+D800, U+E000, U+F8FF, U+FDD0, U+FDEF, U+FFF0, U+FFFD, U+1FFFE, U+1FFFF, U+E0000, U+E0FFF, U+EFFFE, U+F0000 and U+10FFFD

#### Scenario: iprivate and the bidi formatting characters
- **WHEN** `is_iprivate` is called on U+E000, U+F8FF, U+F0000, U+FFFFD, U+100000 and U+10FFFD, and `is_bidi_format` on U+200E, U+200F and U+202A to U+202E
- **THEN** all return `true`; and `is_iprivate` returns `false` for U+DFFF, U+F900, U+FFFFE, U+10FFFE and U+00A0, and `is_bidi_format` for U+200D, U+2029, U+202F and U+2066

#### Scenario: The mail and template sets
- **WHEN** `atext_chars` is called on each of ``!#$%&'*+-/=?^_`{|}~`` and a letter or digit, and `template_literal_chars` on `'`, `[` and `]`
- **THEN** they return `true`; and `atext_chars` returns `false` for `()<>[]:;@\,."` and a space, `template_literal_chars` for a space, `"`, `%`, `<`, `>`, `\`, `^`, `` ` ``, `{`, `|` and `}`, and `varchar_chars` for `-` and `.`

### Requirement: Library: Small terminal rules Boost.URL lacks
`detail/rules.hpp` SHALL provide the terminals Boost.URL's grammar library does not have: `exactly<N>(chars)`, which matches exactly `N` characters of a set (`two_digits_rule` is `exactly<2>(digit_chars)`), `ci_literal_rule`, which matches a literal with its ASCII letters in either case and yields the input's text, and `repeat_rule(first, next, least = 0, most = unbounded)`, which matches a first repetition and then repetitions, from `least` to `most` of them. Each answers `error::mismatch` on failure and leaves the iterator, gives back a repetition that breaks after reading part of its input, and reads its view alone.

#### Scenario: exactly leaves the input when it fails
- **WHEN** `exactly<4>(digit_chars)` parses `2026-10`, then `202`, `20x6`, an empty text and `std::string_view("2026", 3)`
- **THEN** the first yields `2026` and leaves the iterator at offset 4, and the others answer `error::mismatch` with the iterator unchanged

#### Scenario: A literal in any case
- **WHEN** `ci_literal_rule("IPv6:")` parses `IPv6:::1`, `ipv6:::1` and `IPV6:::1`, then `IPv4:1`, `IPv6` and `IP v6:`
- **THEN** the first three match 5 octets, and the others answer `error::mismatch` with the iterator unchanged

#### Scenario: repeat_rule gives a broken repetition back
- **WHEN** `repeat_rule(digit, pair, 1, 3)` with `pair` a comma then a digit parses `1`, `1,2x`, `1,2,3,4` and `1,x`
- **THEN** it matches `1`, `1,2`, `1,2,3` and `1`; it refuses `x`, an empty text and `,1`; and `repeat_rule(ab, ab)` on `ac` matches the empty text and on `ababac` matches `abab`

### Requirement: Library: The guarantees of the page
The library SHALL keep seven invariants, published as the page's "Guarantees" section with the anchors `lexical-invariant-1` to `lexical-invariant-7`: (1) for every format but `regex`, `is_x(s)` and `parse(s, x_rule)` give the same answer for every string; (2) no function or rule allocates but those the allocation table names, the IRI functions, only through a memory resource; (3) nothing throws but an allocation that fails; (4) every function refuses a string that is not well-formed UTF-8; (5) a rule that fails leaves its iterator where it was; (6) `is_regex` answers every byte string, a value, and never throws: a pattern past one of its bounds is answered `false` before it can take more stack or time than the bounds allow; (7) the Unicode tables and the patterns compared with V8 are what their generators write from the pinned files, byte for byte. Each unit test checks that a validator and its rule agree on every input it holds.

#### Scenario: A validator and its rule agree
- **WHEN** `is_uuid(s)` and `urlg::parse(s, fg::uuid_rule).has_value()` are evaluated on every UUID string of `uuid_test`
- **THEN** the two answers are equal for each

#### Scenario: A pattern past a bound is a value
- **WHEN** `is_regex` is called on a pattern of `max_regex_length + 1` bytes
- **THEN** it returns `false`, neither throwing nor allocating

### Requirement: Library: Include boundaries
`meta/include-boundaries.json` SHALL keep, for `include/webcpp/lexical.hpp`, `include/webcpp/lexical/*.hpp` and `include/webcpp/lexical/detail/*.hpp`: the headers other than `regex.hpp` and `detail/regex_*.hpp` from including `webcpp/lexical/regex` and `webcpp/lexical/detail/regex_` (only `regex.hpp` brings the Unicode tables, which `lexical.hpp` leaves out); every header from including `webcpp/idna/detail` (lexical uses idna's public interface); every header from including `boost/json` (JSON is the tests', not the library's); and every header from including `boost/regex`, `webcpp/regex` and `srell` (`is_regex` is a syntax validator of lexical's own and depends on no regex library). Each boundary carries its reason, which the superproject's lint prints.

#### Scenario: The lint refuses a header crossing a boundary
- **WHEN** `include/webcpp/lexical/hostname.hpp` includes `<webcpp/lexical/detail/regex_tables.hpp>`
- **THEN** the superproject's lint fails, naming the header and "only regex.hpp brings the Unicode tables of ECMA-262's classes, which lexical.hpp leaves out"

#### Scenario: idna's detail is off limits
- **WHEN** a lexical header includes `<webcpp/idna/detail/...>`
- **THEN** the lint fails with "lexical uses idna's public interface"

### Requirement: Library: idna through its public interface
The library SHALL ask `webcpp::idna` through its public interface alone (`<webcpp/idna/options.hpp>`, `<webcpp/idna/uts46.hpp>`, `<webcpp/idna/validity.hpp>`, `<webcpp/idna/errors.hpp>`), never its `detail`. A verdict of idna's that a test of lexical finds wrong MUST become a test in idna, red first, reported to idna's track; lexical never works around it. A change of idna's Unicode version is seen by lexical's tests (for instance `hostname_test` converts every code point between two letters to check that U+3002, U+FF0E and U+FF61 remain the only dots UTS 46 maps).

#### Scenario: A verdict of idna that looks wrong
- **WHEN** a test of lexical finds `idna::check_label` accepting a label RFC 5891 refuses
- **THEN** a red test is added to idna, and lexical's own test of the case is not worked around

### Requirement: Library: Targets
The library SHALL run natively, on wasm32-wasip2 and wasm32-wasip3 (clang with wasi-sdk, under wasmtime), and on emscripten (under node), with exceptions or without. `test/Jamfile` and `example/Jamfile` MUST declare `webcpp.targets native wasip2 wasip3 emscripten ;`; the lanes `oracle` and `generated` run apart from the targets, each natively. The tests and examples are built and run from the superproject's root with `b2 libs/lexical/test libs/lexical/example`, `b2 toolset=clang-wasip2 testing.launcher=wasmtime libs/lexical/test libs/lexical/example`, `b2 toolset=clang-wasip3 testing.launcher=wasmtime libs/lexical/test libs/lexical/example` and `b2 toolset=emscripten libs/lexical/test libs/lexical/example`. On MSVC, which compiles the Unicode tables beside Boost.URL and idna only with `/bigobj`, the tests and examples take `<toolset>msvc:<cxxflags>/bigobj`, not measured because no Windows machine is available.

#### Scenario: The same tests run on WebAssembly
- **WHEN** `b2 toolset=clang-wasip2 testing.launcher=wasmtime libs/lexical/test` runs
- **THEN** every `webcpp.run` test of `test/Jamfile` is built for wasm32-wasip2 and run under wasmtime, with the suite's and the patterns' files given by `webcpp.data`

### Requirement: Library: Every public header compiles alone
`test/Jamfile` SHALL declare `webcpp.headers-alone lexical : ../include ;`, which compiles each public header, `regex.hpp` included, alone in its own translation unit on every target, so that none depends on an include made before it.

#### Scenario: A header missing an include
- **WHEN** `uri_template.hpp` uses `std::size_t` without including `<cstddef>`
- **THEN** the headers-alone target for that header fails to compile

### Requirement: Library: The unit tests and how they are registered
The tests SHALL be written with Boost's lightweight_test (`BOOST_TEST`, `BOOST_TEST_EQ`, `boost::report_errors()`), one `webcpp.run` per file in `test/Jamfile`: `detail` (UTF-8, the character sets, the small rules), `date_time`, `uuid`, `json_pointer`, `ip`, `uri`, `regex`, `regex_v8`, `hostname`, `email`, `iri`, `iri_resource` (`<exception-handling>on`), `uri_template`, `grammar` (every public rule inside `tuple_rule`, `variant_rule`, `optional_rule` and `range_rule`, where each stops, a rule that fails whole, the page's host-and-port example, and an IDN rule's error being idna's), `allocation` and `suite`, plus `config` compiled three times. The suite files, the known-failure lists and the patterns reach a WASI program through `webcpp.data` (the directory wasmtime is given with `--dir`) and an emscripten program through node's file system (`-sNODERAWFS=1`). The test matrix of every lane is published at https://webcpporg.github.io/report/.

#### Scenario: A new test file is one line
- **WHEN** `test/foo_test.cpp` is added with `webcpp.run foo : foo_test.cpp ;` in `test/Jamfile`
- **THEN** it is built and run on every target, and nothing else registers it

#### Scenario: Rules inside the combinators
- **WHEN** `urlg::variant_rule(fg::date_time_rule, fg::date_rule, fg::time_rule)` parses `2020-01-01` and `12:00:00Z`
- **THEN** the results have index 1 and 2 respectively

### Requirement: Library: Test helpers
The tests SHALL share two helpers in `test/`: `counting_resource.hpp`, a `webcpp::test::counting_resource` that is a `std::pmr::memory_resource` counting `allocations`, `deallocations` and `bytes`, taking its memory from `std::pmr::new_delete_resource()` so that each allocation is also one call of the global `operator new`, and equal only to itself; and `require.hpp`, `webcpp::test::require(bool held)`, which ends the program as `main` would, with `boost::report_errors()`, when a check a helper cannot go on without did not hold (a helper cannot return from its caller, and a lightweight_test program may have no exceptions).

#### Scenario: The counting resource counts
- **WHEN** a `counting_resource` allocates 64 bytes and deallocates them
- **THEN** `allocations` is 1, `deallocations` is 1 and `bytes` is 64

#### Scenario: A required check that fails
- **WHEN** `require(BOOST_TEST(false))` runs
- **THEN** the program exits with the count of errors so far

### Requirement: Library: The JSON Schema Test Suite runner
`test/suite_test.cpp` SHALL run the format tests of the official JSON-Schema-Test-Suite (the superproject's submodule `third_party/libs/JSON-Schema-Test-Suite`, of which `WEBCPP_TEST_JSON_SCHEMA_TEST_SUITE` names the root) directly against lexical's validators on every target: every file of `tests/<dialect>/optional/format/` for `draft4`, `draft6`, `draft7`, `draft2019-09` and `draft2020-12`, and of `tests/v1/format/`. A string test passes when its validator's verdict is the test's `valid`. The validators MUST be one per format in a constexpr table sorted and unique by format (checked by `static_assert`): `date`, `date-time`, `duration`, `email`, `hostname`, `idn-email`, `idn-hostname`, `ipv4`, `ipv6`, `iri`, `iri-reference`, `json-pointer`, `regex`, `relative-json-pointer`, `time`, `uri`, `uri-reference`, `uri-template`, `uuid`. No copy of the suite's data is committed: it is MIT data and every file of webcpp is BSL-1.0; moving the submodule is a change of its own, with every new failure fixed or listed.

#### Scenario: The suite passes
- **WHEN** the suite test runs on the pinned submodule
- **THEN** it counts 3,540 string tests over the six dialects, all passing, and prints one line per dialect such as `draft7: 684 string tests, 0 known failures, ... non-string, ... skipped (unknown)`

#### Scenario: A format with no validator fails the run
- **WHEN** the suite holds a file whose schema's format is none of the nineteen and not `unknown`
- **THEN** the run fails with `the format "<name>" maps to no validator`

### Requirement: Library: Dialect policies of the runner
The runner SHALL call the same validator in every dialect except where a dialect's text cites a policy: `is_time(s, leap_seconds::refused)` for v1's `time` (v1's suite inverts six leap-second tests of `time.json` as "This MUST NOT include leap seconds"), `is_relative_json_pointer(s, index_manipulation::refused)` for the relative JSON Pointers of `draft7` and `draft2019-09` (which cite draft-handrews-relative-json-pointer-01, without index manipulation), and the defaults (leap seconds allowed, index manipulation allowed) in every other dialect. `date-time` allows leap seconds in every dialect, v1 included. Each entry of this dialect table MUST name a dialect and a format that exist, else the run fails by name, because the suite tests no index manipulation and a misspelled entry would otherwise fall to the default validator without a word.

#### Scenario: v1 time refuses a leap second
- **WHEN** the runner judges v1's `time` test with data `23:59:60Z` expecting invalid
- **THEN** it runs `is_time(s, leap_seconds::refused)` and passes

#### Scenario: A misspelled policy entry fails
- **WHEN** an entry of the dialect table names the dialect `draft-7`
- **THEN** the run fails with `dialect_validators: the entry "relative-json-pointer" / "draft-7" names no dialect`

### Requirement: Library: Known failures
Each dialect SHALL have a list `test/known_failures/<dialect>.json`, a JSON array of objects `{file, case, test, reason}`, `file` by its path under the dialect's directory (`optional/format/uuid.json`), `case` and `test` by description, and `reason` non-empty. Only a residue tied to a Unicode version (idna's tables are Unicode 17.0.0 and a suite test may be written against another) MAY be listed, its reason naming both versions. Every list is empty today (`[]`), so lexical passes 100% of the suite. The runner MUST fail on a failure no list names, on a listed test that passes, on a listed test the suite does not hold, on a list that names a test twice, on an entry without all four fields, and on a known failure that names a test no validator judges (an `unknown` format, a non-string instance).

#### Scenario: A failure not listed
- **WHEN** a validator answers `false` for a test the suite expects valid and no list names it
- **THEN** the run fails with `<dialect>/<file>: "<case>" / "<test>": expected valid, the validator says invalid`

#### Scenario: A listed test that passes
- **WHEN** a list names a test that passes
- **THEN** the run fails with `passes, and its dialect's known-failure list names it`, so a list never lies

### Requirement: Library: What the runner does not judge
The runner SHALL treat a test whose instance is not a string as json_schema's rule that a format applies to strings only: it MUST expect `valid: true`, count the test apart (`non_strings`) and fail by name if it expects invalid. The `unknown` format (drafts 4 to 2020-12) is json_schema's question about a format it does not know, and its tests MUST be skipped by name and counted (`skipped`). The runner MUST fail on a case whose schema holds a keyword besides `format` and `$schema`, since it judges a test on its format alone and a keyword that changes a verdict would be misread. The suite's `pattern` and `patternProperties` keywords are json_schema's: lexical does not search, and every pattern they hold is valid for `is_regex` as for V8.

#### Scenario: A non-string instance
- **WHEN** a test of `ipv4.json` has data `12` (a number) and expects `valid: true`
- **THEN** the runner counts it as non-string without calling a validator

#### Scenario: A schema with another keyword
- **WHEN** a case's schema is `{"format": "uuid", "minLength": 3}`
- **THEN** the run fails with `the schema's keyword "minLength", which the runner does not judge`

### Requirement: Library: Counts on the page come from the data
`doc/counts.py` SHALL read the suite and the lists and print `name=<number>` lines that the page uses as attributes, never typed: per dialect `n-suite-<dialect>` (the string tests of the format files the runner reads, the `unknown` format's not counted) and `n-suite-passed-<dialect>` (the first less the dialect's known failures), their sums `n-suite` and `n-suite-passed`, `n-regex-patterns` (the patterns `regex_v8` compares with V8) and `n-regex-valid` (those V8 accepts). A dialect whose directory holds no format file, a list that is not an array and a count of zero MUST be faults written on the standard error with exit 1; a usage error exits 2.

#### Scenario: The counts of the pinned suite
- **WHEN** `doc/counts.py libs/lexical` runs on the pinned submodule
- **THEN** it prints `n-suite-draft4=248`, `n-suite-draft6=348`, `n-suite-draft7=684`, `n-suite-draft2019-09=753`, `n-suite-draft2020-12=753`, `n-suite-v1=754`, `n-suite=3540`, `n-suite-passed=3540`, `n-regex-patterns=24297` and `n-regex-valid=16475`

#### Scenario: A usage error
- **WHEN** `doc/counts.py` runs without its directory argument
- **THEN** it prints `usage: counts.py <library-directory>` on the standard error and exits 2

### Requirement: Library: The V8 oracle lane
The own lane `oracle` (`b2 -a libs/lexical/test/oracle//oracle`) SHALL ask V8, through the Node the lane pins (the CI's Node 26.7.0, no npm package), for its verdict on every pattern of `test/regex/cases/patterns.json` by `new RegExp(pattern, "u")`: `test/oracle/v8.mjs` writes `verdicts.json` in the output directory, one verdict a line, `1` when the constructor accepts and `0` on a `SyntaxError` (any other error MUST be rethrown), in the order of the patterns, and exits 2 with `usage: node v8.mjs <cases directory> <output directory>` when called without its arguments. The lane compares them with the committed `test/regex/expected/verdicts.json`; only `b2 libs/lexical/test/oracle//update-expected` writes that file. A Node whose V8 or Unicode differs may write other verdicts, which is a change of its own. The test `regex_v8` reads both files on every target, requires one verdict for each pattern, and fails on every pattern where `is_regex` differs from V8, naming the first twenty on the standard error.

#### Scenario: A verdict that differs from V8's
- **WHEN** `is_regex` answers `true` for a pattern V8 rejects with a SyntaxError
- **THEN** `regex_v8` fails, printing `pattern <n>: V8 answers invalid, is_regex the other: <pattern>`

#### Scenario: The oracle needs no package
- **WHEN** `test/oracle/package.json` is read
- **THEN** it is private, of type `module`, and lists no dependency

### Requirement: Library: Generated files are checked against their generators
`tools/generate_tables.py` and `tools/generate_patterns.py` SHALL write the files that no hand edits, and the own lane `generated` (`b2 -a libs/lexical/test//generated`) MUST run them again and compare each output with the committed file byte for byte, while `update-generated` copies them over. `generate_tables.py` writes `include/webcpp/lexical/detail/regex_tables.hpp` from the Unicode files that `tools/unicode.json` pins, each by URL and SHA-256 (Unicode 17.0.0, as idna's); a file missing from the cache directory is downloaded, every file is hashed, and a digest that differs from the pinned one stops the generator before anything is written, naming the file and both digests. `generate_patterns.py` writes `test/regex/cases/patterns.json`, a sorted JSON array of distinct patterns, one a line, every character outside printable ASCII escaped, from Python's `random.Random` with fixed seeds, the Unicode files, and every pattern of the suite. Two runs MUST write the same bytes (no date, sorted data, `\n` line endings), nothing is written unless every check passed, and each exits 0 on success, 1 naming a fault, 2 on a usage error. The ordinary lanes never run the generators.

#### Scenario: A hand edit of the tables is caught
- **WHEN** a byte of `regex_tables.hpp` is changed by hand and the `generated` lane runs
- **THEN** `generated-tables` fails, since the regenerated file differs from the committed one

#### Scenario: A pinned file with the wrong hash
- **WHEN** a downloaded Unicode file does not hash to the SHA-256 in `tools/unicode.json`
- **THEN** the generator exits 1 naming the file and both digests, and writes nothing

### Requirement: Library: Documentation page
The library SHALL have one documentation page, `doc/lexical.adoc`, titled "lexical: the string formats of JSON Schema for C++", built by `webcpp.doc lexical : lexical.adoc ;` and `webcpp.reference lexical ;` in `doc/Jamfile` (`b2 libs/lexical/doc`, into `doc/html/index.html`, published at https://webcpporg.github.io/libs/lexical/), with the attribute `unicode-version` of 17.0.0. It MUST include, in this order, the sections introduction, dates, identifiers, addresses, URIs, templates, regular expressions, composing rules, errors, allocation, the JSON Schema suite, guarantees, testing, the Reference (every name of `webcpp::lexical` and `webcpp::lexical::grammar`, from the Doc Comments of the headers, with `show-enum-constants: true` in `doc/mrdocs.yml` so that each enumerator has a section of its own), and the appendices "Unicode data" and "Changelog". Every public symbol MUST have a Doc Comment, each rule type's naming its RFC section and where it stops. Every program the page shows is compiled and run natively and on wasm32-wasip2, wasm32-wasip3 and emscripten, and printed the output shown.

#### Scenario: The index lists the library
- **WHEN** the superproject's `b2 doc` runs with `libs/lexical` present
- **THEN** the index lists lexical from its `meta/libraries.json`, linked to its page

#### Scenario: A missing Doc Comment
- **WHEN** a public function has no Doc Comment
- **THEN** the strict MrDocs reference fails the documentation build

### Requirement: Library: Examples
The library SHALL ship four examples in `example/`, declared in `example/Jamfile` by `webcpp.example`, each compiled and run on every target and compared with the `.expected` file beside it: `lexical_quick_start.cpp` (the table of the quick start), `lexical_grammar.cpp` (an internationalized host and an optional port, tag `host_port`), `lexical_iri.cpp` (an IRI with a memory resource of the program's own, tag `resource`) and `lexical_regex.cpp` (patterns judged, tag `program`). The page includes them by tag.

#### Scenario: The grammar example
- **WHEN** `lexical_grammar` runs on `bücher.example:8080`, `example.com`, `a·b.example:80` and `example.com:99999`
- **THEN** it prints `bücher.example:8080 -> host bücher.example, port 8080`, `example.com -> host example.com`, `a·b.example:80 -> refused by webcpp.idna` and `example.com:99999 -> refused by boost.url.grammar`

#### Scenario: The IRI example
- **WHEN** `lexical_iri` runs with a 4,096-byte monotonic buffer backed by `null_memory_resource()`
- **THEN** it prints `short: true`, `long: true` (the long IRI's mapped copy of 1,819 octets is taken from the buffer) and `bidi: false`

#### Scenario: The regex example
- **WHEN** `lexical_regex` runs
- **THEN** it prints `valid` for `^\p{Letter}+$`, `(?<year>\d{4})-(?<month>\d{2})`, `(?<=\$\d*)[0-9]` and `(?i:abc)`, and `invalid` for `^(abc]`, `\Aabc\z` and `(?i)abc`

### Requirement: Library: Changelog
`doc/changelog.adoc` SHALL be the page's appendix "Changelog": what changed in each version of webcpp, newest first, one line per change, under "Unreleased" (anchor `lexical-changelog-unreleased`, lines between `tag::unreleased[]` and `end::unreleased[]`), and every user-visible change, including the removal of a guard, MUST add a bullet. Today it lists: the library (validators and Boost.URL rules for JSON Schema's string formats), dates, times and durations of RFC 3339 with a leap-second policy for v1, UUIDs and absolute and relative JSON Pointers with an index manipulation policy, IP addresses and URIs and URI references through Boost.URL faithful to RFC 3986, IRIs and URI templates mapped to URIs within a memory resource, host names and e-mail addresses with internationalized ones judged by webcpp::idna, and ECMA-262 patterns in `u` mode checked by an allocation-free syntax validator with Unicode 17 tables.

#### Scenario: The first release notes
- **WHEN** `doc/changelog.adoc` is read
- **THEN** its "Unreleased" section holds seven bullets, the first "Added the library: validators and Boost.URL rules for JSON Schema's string formats."

### Requirement: Library: Repository files of a webcpp library
The repository SHALL hold `build.jam`, `meta/libraries.json` (key `lexical`, name `lexical`, `cxxstd` `"20"`, `"port-of": null`, category `["Parsing", "String"]`, the description "Validators and Boost.URL grammar rules for the string formats of JSON Schema, for C++20."), `meta/include-boundaries.json`, `README.md`, `AGENTS.md` (only what is specific to lexical, the superproject's rules applying), `LICENSE_1_0.txt`, `doc/` (the page and `mrdocs.yml`), `.gitignore` (at least `doc/html/`, `node_modules` and `.node-modules/`), `.gitattributes` (`* text=auto eol=lf`, `*.bat text eol=crlf`) and `.github/workflows/ci.yml`, which calls the superproject's reusable workflow `webcpporg/webcpp/.github/workflows/library.yml@main` with `library: lexical` on push to `main`, pull requests and manual dispatch, with `contents: read` permission and a concurrency group that never cancels a run on `main`. `README.md` MUST show the format table, the quick start with its output, how to build and test, the documentation location and the Unicode data notice.

#### Scenario: The CI is the superproject's
- **WHEN** `.github/workflows/ci.yml` is read
- **THEN** its one job `ci` uses `webcpporg/webcpp/.github/workflows/library.yml@main` with `library: lexical`

### Requirement: Library: Licence notice and text rules
Every source file (`.hpp`, `.cpp`, `.py`, `.jam`, `Jamfile`, `build.jam`, `.mjs`, `.yml`, `.adoc`) SHALL open with the notice "Copyright (c) 2026 WebCpp.org" and the Boost Software License 1.0 lines ("Distributed under the Boost Software License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)"), `//` for C++, JavaScript and AsciiDoc, `#` for the rest with a `#!` line first. Every file MUST be in English and without the em dash. lexical's code is under the Boost Software License alone; the Unicode data derived from the Unicode Character Database, version 17.0.0, which Unicode, Inc. publishes under the Unicode License v3 (Copyright 1991-2026 Unicode, Inc.), is covered by its notice, the appendix "Unicode data" of the page (anchor `lexical-unicode-data`), for `<webcpp/lexical/detail/regex_tables.hpp>` and `test/regex/cases/patterns.json`.

#### Scenario: A file without the notice fails the lint
- **WHEN** a new `.hpp` is added without the licence notice
- **THEN** the superproject's lint fails, naming the file

#### Scenario: The Unicode notice is on the page
- **WHEN** the page's appendix "Unicode data" is read
- **THEN** it holds the Unicode License V3 text and states that lexical's code is under the Boost Software License 1.0 alone

### Requirement: Dates: the formats of this area and their functions
The library SHALL provide, in namespace `webcpp::lexical`, one function per format of this area, each `[[nodiscard]]`, `noexcept`, taking any byte string as `std::string_view`, returning `bool`, and equal to `boost::urls::grammar::parse(s, <rule>).has_value()` for the rule of the same format: `is_date_time` (`date-time`, RFC 3339 5.6 and 5.7; rule `grammar::date_time_rule`), `is_date` (`date`, RFC 3339 5.6 `full-date`; `grammar::date_rule`), `is_time(s, leap_seconds leap = leap_seconds::allowed)` (`time`, RFC 3339 5.6 `full-time`; `grammar::time_rule`), `is_duration` (`duration`, RFC 3339 appendix A; `grammar::duration_rule`), `is_uuid` (`uuid`, RFC 9562 4; `grammar::uuid_rule`), `is_json_pointer` (`json-pointer`, RFC 6901 3; `grammar::json_pointer_rule`), `is_relative_json_pointer(s, index_manipulation manipulation = index_manipulation::allowed)` (`relative-json-pointer`; `grammar::relative_json_pointer_rule`), `is_ipv4` (`ipv4`, RFC 2673 3.2) and `is_ipv6` (`ipv6`, RFC 4291 2.2). The date and time functions MUST live in `<webcpp/lexical/date_time.hpp>`, `is_uuid` in `<webcpp/lexical/uuid.hpp>`, the two pointer functions in `<webcpp/lexical/json_pointer.hpp>` and the two address functions in `<webcpp/lexical/ip.hpp>`; each MUST include `<webcpp/lexical/config.hpp>` first.

#### Scenario: A function and its rule never disagree
- **WHEN** the tests of this area run each sample through `lexical::is_date_time` and through `urlg::parse(sample, fg::date_time_rule)`
- **THEN** both give the same verdict for every sample, and the same holds for `is_date` with `date_rule`, `is_time` with `time_rule`, `is_duration` with `duration_rule`, `is_uuid` with `uuid_rule`, `is_json_pointer` with `json_pointer_rule` and `is_relative_json_pointer` with `relative_json_pointer_rule_t{manipulation}`

#### Scenario: Every function takes any byte string
- **WHEN** `lexical::is_time(std::string_view("12:00:00Z\0", 10))` is called
- **THEN** it returns `false` without throwing, since NUL is no part of any of these formats except inside a JSON Pointer token

### Requirement: Dates: the JSON Schema drafts that use each format
The library SHALL cover these formats for the drafts that name them: `date-time` for drafts 4, 6, 7, 2019-09, 2020-12 and v1; `date` and `time` from draft 7; `duration` and `uuid` from draft 2019-09; `json-pointer` from draft 6; `relative-json-pointer` from draft 7; `ipv4` and `ipv6` for every draft. A validator MUST NOT depend on the draft except through the two policy arguments `leap_seconds` (for `time`) and `index_manipulation` (for `relative-json-pointer`), which the caller chooses per dialect.

#### Scenario: The policies are the only dialect knobs
- **WHEN** a caller validates `time` for v1 and for draft 2020-12
- **THEN** it calls `is_time(s, leap_seconds::refused)` for v1 and `is_time(s)` for 2020-12, and no other function of this area takes a dialect

### Requirement: Dates: out of scope
The library MUST NOT parse a date, a time or a duration into fields, MUST NOT convert a duration's numbers, MUST NOT check a leap second against the IERS list of published leap seconds, and MUST NOT implement the `date-time-ext` suffixes of RFC 9557 that JSON Schema v1's text mentions. Each rule yields only the text it matched, as `grammar::text`, a view of its input.

#### Scenario: A leap second on a day that never had one
- **WHEN** `lexical::is_date_time("2021-03-15T23:59:60Z")` is called
- **THEN** it returns `true`, because the date of a leap second is not looked up in any table and the answer is the same every year

### Requirement: Dates: the full-date shape
`grammar::date_rule` SHALL match RFC 3339 5.6's `full-date`, `4DIGIT "-" 2DIGIT "-" 2DIGIT`, with ASCII digits only (`0` to `9`; no other Unicode digit), exactly four year digits, and nothing before or after it inside the rule. `is_date` MUST reject a year of 3 or 5 digits, a month or a day of 1 or 3 digits, and a separator other than `-`.

#### Scenario: The digit counts
- **WHEN** `is_date` is called on `0000-01-01`, `9999-12-31`, `202-01-01`, `12021-01-01`, `2021-001-01`, `2021-01-001`, `2021-01-1` and `2021/01/01`
- **THEN** the first two return `true` and the other six return `false`

### Requirement: Dates: month lengths and the Gregorian calendar
`grammar::date_rule` SHALL accept a month 01 to 12 only, and a day from 01 to the length of that month in the proleptic Gregorian calendar (RFC 3339 5.7): 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 days, February having 29 in a year divisible by 4 and not by 100 unless by 400. `detail::days_in_month(year, month)` MUST return 0 for a month outside 1 to 12, and `detail::is_leap_year` MUST implement the same rule. The year `0000` MUST be valid.

#### Scenario: Every month's last day and the next
- **WHEN** `is_date` is called on `2021-<mm>-<last>` and on `2021-<mm>-<last+1>` for each month, and on `2021-00-01`, `2021-13-01` and `2021-01-00`
- **THEN** each last day is valid, each next day is invalid, and the three out-of-range dates are invalid

#### Scenario: Gregorian leap years
- **WHEN** `is_date` is called on `0400-02-29`, `2000-02-29`, `2020-02-29`, `0100-02-29`, `2100-02-29` and `2021-02-29`
- **THEN** the first three are valid (divisible by 400, 400, and 4 but not 100) and the last three are invalid (divisible by 100 but not 400, the same, and not by 4)

#### Scenario: A month that does not exist has no days
- **WHEN** `detail::days_in_month(2021, 0)` and `detail::days_in_month(2021, 13)` are called
- **THEN** both return 0

### Requirement: Dates: the full-time shape
`grammar::time_rule_t` SHALL match RFC 3339 5.6's `full-time`: `hh ":" mm ":" ss ["." 1*DIGIT]` followed by a `time-offset`, ASCII digits only, with the hour 00 to 23, the minute 00 to 59, the second 00 to 60, and a fraction of one or more digits of any length (no overflow, nothing converted). The offset MUST be `Z` or `z`, or `+` or `-` followed by `hh ":" mm` with hh 00 to 23 and mm 00 to 59 (`+00:00` and `-00:00` are both valid). An offset without minutes (`+01`), without a colon (`+0130`), a time without an offset, a dot without digits, and a one-digit hour are invalid. The rule MUST stop right after the offset.

#### Scenario: Parts and ranges of a time
- **WHEN** `is_time` is called on `08:30:06z`, `08:30:06.1Z`, `08:30:06.123456789012345678901234567890Z` and `00:00:00+23:59`
- **THEN** all four return `true`

#### Scenario: Refused times
- **WHEN** `is_time` is called on `24:00:00Z`, `00:60:00Z`, `01:02:03+24:00`, `01:02:03+00:60`, `08:30:06.Z`, `08:30:06+01`, `08:30:06+0130`, `12:00:00`, `8:30:06Z` and `08:30:06Z ` (trailing space)
- **THEN** every one returns `false`

### Requirement: Dates: leap seconds and the leap_seconds policy
The library SHALL define `enum class leap_seconds { allowed, refused }`. Under `allowed`, which is the default of `is_time` and of `grammar::time_rule`, second 60 MUST be valid exactly when the time moved to UTC by its offset is 23:59, that is when `(hour*60 + minute - offset_minutes)` taken modulo 1440 (a non-negative result) equals 1439; second 61 or more MUST be invalid. Under `refused`, second 60 MUST never be valid and every other verdict MUST be unchanged. `grammar::time_rule_t` MUST carry the policy in a public member `leap`, so `time_rule_t{leap_seconds::refused}` is the rule of JSON Schema v1's `time`. `is_date_time` and `grammar::date_time_rule` MUST always allow leap seconds, as the suite's `date-time` tests of every draft, v1 included, say.

#### Scenario: Leap seconds at each offset, allowed
- **WHEN** `is_time` is called on `23:59:60Z`, `23:59:60+00:00`, `23:59:60-00:00`, `15:59:60-08:00`, `00:29:60-23:30`, `01:29:60+01:30` and `23:29:60+23:30`
- **THEN** all seven return `true`

#### Scenario: Second 60 that is not at 23:59 UTC
- **WHEN** `is_time` is called on `23:59:60+01:00`, `23:59:60-00:30`, `22:59:60Z`, `23:58:60Z` and `23:59:61Z`
- **THEN** all five return `false`

#### Scenario: Leap seconds refused for v1
- **WHEN** `is_time(s, leap_seconds::refused)` is called on `23:59:60Z`, `15:59:60-08:00`, `00:29:60-23:30` and `23:59:59Z`
- **THEN** the first three return `false` and the last returns `true`

#### Scenario: date-time keeps leap seconds in every draft
- **WHEN** `is_date_time` is called on `1990-12-31T23:59:60Z` and on `1990-12-31T15:59:60-08:00`
- **THEN** both return `true`

### Requirement: Dates: the date-time composition
`grammar::date_time_rule` SHALL match RFC 3339 5.6's `date-time`, a `full-date`, then `T` or `t`, then a `full-time` (leap seconds allowed), and stop after the offset. A space or any other separator, a time without seconds, an invalid calendar date and trailing text MUST make `is_date_time` return `false`. The five examples of RFC 3339 5.8 MUST be valid.

#### Scenario: RFC 3339 section 5.8
- **WHEN** `is_date_time` is called on `1985-04-12T23:20:50.52Z`, `1996-12-19T16:39:57-08:00`, `1990-12-31T23:59:60Z`, `1990-12-31T15:59:60-08:00` and `1937-01-01T12:00:27.87+00:20`
- **THEN** all five return `true`

#### Scenario: Case of T and Z, and the refusals
- **WHEN** `is_date_time` is called on `1963-06-19t08:30:06.283185z`, `1963-06-19T08:30:06Z`, `1963-06-19 08:30:06Z`, `1963-06-19T08:30Z`, `2021-02-29T00:00:00Z` and `1963-06-19T08:30:06Ztail`
- **THEN** the first two return `true` and the last four return `false`

### Requirement: Dates: the duration grammar of RFC 3339 appendix A
`grammar::duration_rule` SHALL implement RFC 3339 appendix A's `duration` exactly, one rule per production in the ABNF's order: `"P" (dur-date / dur-time / dur-week)`, `dur-date = (dur-day / dur-month / dur-year) [dur-time]`, `dur-time = "T" (dur-hour / dur-minute / dur-second)`, `dur-year = 1*DIGIT "Y" [dur-month]`, `dur-month = 1*DIGIT "M" [dur-day]`, `dur-day = 1*DIGIT "D"`, `dur-hour = 1*DIGIT "H" [dur-minute]`, `dur-minute = 1*DIGIT "M" [dur-second]`, `dur-second = 1*DIGIT "S"`, `dur-week = 1*DIGIT "W"`. A week MUST stand alone; the designators MUST come in order, with a part skipped only at the start (so `P1Y2D` is not one duration); nothing may follow `P` or `T` empty; a fraction and a sign are invalid. The designators are letters of the ABNF, so each MUST match in either case (RFC 5234 2.3).

#### Scenario: Valid durations
- **WHEN** `is_duration` is called on `P1Y`, `P1M`, `P1W`, `P1D`, `PT1H`, `PT1M`, `PT1S`, `P1Y2M3DT4H5M6S`, `P1DT12H`, `PT1H30M` and `P0D`
- **THEN** all eleven return `true`

#### Scenario: Invalid durations
- **WHEN** `is_duration` is called on `P`, `PT`, `P1DT`, `P1H`, `PT1D`, `P1Y1W`, `P1WT1H`, `P1Y2D`, `PT1H2S`, `P1.5D`, `P-1D`, `-P1D`, `P1D1Y` and `PT1S1M`
- **THEN** all fourteen return `false`

#### Scenario: Lowercase designators
- **WHEN** `is_duration` is called on `p1d`, `pt1h`, `P1y2m3dT4h5m6s` and `p1w`
- **THEN** all four return `true`

### Requirement: Dates: a duration's numbers are never converted
The duration rule SHALL accept a number of any length, using a token of `1*DIGIT` without converting it, so that no digit count overflows, and the validators of this area MUST NOT allocate however long a fraction or a number is.

#### Scenario: A hundred thousand digits
- **WHEN** `is_duration` is called on `P` followed by 100,000 digits `7` and `D`, and `is_date_time` on a time whose fraction has 100,000 digits
- **THEN** both return `true` and the allocation test counts no allocation

### Requirement: Dates: where the rules stop inside a larger grammar
Each rule of this area SHALL stop where its grammar does, so a program can compose it with Boost.URL's combinators: `date_rule` after the day, `time_rule` and `date_time_rule` after the offset, `duration_rule` where the ABNF's production ends (after `P1Y`, the `2D` of `P1Y2D` is not its), without requiring the end of the input; `boost::urls::grammar::parse(s, rule)` MUST then answer `error::leftover` for text after a complete match.

#### Scenario: Two dates joined by a slash
- **WHEN** `urlg::parse("2020-01-01/2021-02-03", tuple_rule(fg::date_rule, squelch(delim_rule('/')), fg::date_rule))` is run
- **THEN** it succeeds with `2020-01-01` and `2021-02-03`

#### Scenario: A time and a duration
- **WHEN** `urlg::parse("08:30:06Z/P1D", tuple_rule(fg::time_rule, squelch(delim_rule('/')), fg::duration_rule))` is run
- **THEN** it succeeds with `08:30:06Z` and `P1D`

#### Scenario: The duration rule stops at the ABNF
- **WHEN** `fg::duration_rule.parse(it, end)` is run on `P1Y2D`
- **THEN** it returns `P1Y` and leaves `it` three bytes in, while `urlg::parse("2020-01-01x", fg::date_rule)` answers `error::leftover`

### Requirement: Dates: error values and the unchanged iterator of the date rules
Every rule of this area SHALL declare `using value_type = grammar::text` and `[[nodiscard]] grammar::result<grammar::text> parse(char const*& it, char const* end) const noexcept`. On success it MUST yield the text matched and move `it` past it. On failure it MUST leave `it` where it was and return a Boost.URL grammar error: `error::mismatch` when the first byte cannot start the format (no digit for `date_rule` and `time_rule`, no `P` or `p` for `duration_rule`), and `error::invalid` when the format started and then broke its shape or a range. `date_time_rule` MUST return the date rule's error when the date fails and `error::invalid` for a missing `T`/`t` or a failing time.

#### Scenario: What each rule leaves where it fails
- **WHEN** the rules parse `x2021-01-01` (`date_rule`), `2021-02-29` (`date_rule`), `2021-1-01` (`date_rule`), `T12:00:00Z` (`time_rule`), `12:00:00` (`time_rule`), `23:59:60Z` (`time_rule_t{leap_seconds::refused}`), `2021-01-01X00:00:00Z` (`date_time_rule`), `1D` (`duration_rule`) and `PT` (`duration_rule`)
- **THEN** the errors are `mismatch`, `invalid`, `invalid`, `mismatch`, `invalid`, `invalid`, `invalid`, `mismatch` and `invalid`, and `it` is the start of the input each time

### Requirement: Dates: the rule objects
The library SHALL provide the types `grammar::date_rule_t`, `grammar::time_rule_t` (with the public member `leap_seconds leap = leap_seconds::allowed`), `grammar::date_time_rule_t` and `grammar::duration_rule_t`, and the `inline constexpr` objects `grammar::date_rule`, `grammar::time_rule`, `grammar::date_time_rule` and `grammar::duration_rule`, built from `boost::urls::grammar` combinators (`tuple_rule`, `variant_rule`, `optional_rule`, `delim_rule`, `token_rule(digit_chars)`) and lexical's `detail::exactly<N>` rule for the fixed digit counts. Range checks MUST be made after the shape matched, on the digits as integers of a few digits only.

#### Scenario: A parameterized rule
- **WHEN** a program writes `fg::time_rule_t{lexical::leap_seconds::refused}`
- **THEN** it is a rule usable with `urlg::parse` that refuses second 60 and agrees with `is_time(s, leap_seconds::refused)`

### Requirement: Dates: ASCII digits and NUL
The date, time and duration rules SHALL accept only the ASCII bytes of their grammar and MUST treat a NUL byte as any other non-matching byte, whether it replaces a character or follows a complete match.

#### Scenario: NUL in each format
- **WHEN** the validators are called with `2020-01-01\0`, `12:00:00\0Z`, `12:00:00Z\0`, `2020-01-01T12:00:00Z\0`, `P1\0D` and `P1D\0`
- **THEN** every call returns `false`

### Requirement: Dates: the suite runner uses the policies
The JSON Schema Test Suite runner SHALL validate `time` for the dialect v1 with `is_time(s, leap_seconds::refused)` and every other dialect with `is_time(s)`; `date-time`, `date`, `duration`, `uuid`, `json-pointer`, `ipv4` and `ipv6` SHALL use the default function in every dialect; `relative-json-pointer` SHALL use `is_relative_json_pointer(s, index_manipulation::refused)` for drafts 7 and 2019-09 and the default in the others. No known failure of the suite MAY be listed for these formats.

#### Scenario: v1 time
- **WHEN** the suite's v1 `time.json` case holding `23:59:60Z` runs
- **THEN** the runner calls `is_time` with `leap_seconds::refused` and the expected invalid verdict holds

### Requirement: Dates: UUID shape
`grammar::uuid_rule` and `is_uuid` SHALL accept RFC 9562 4's string form, `8HEXDIG "-" 4HEXDIG "-" 4HEXDIG "-" 4HEXDIG "-" 12HEXDIG`, with hexadecimal digits in either case and in any mix of cases in one UUID. The version and variant nibbles MUST NOT be checked (the suite accepts version 15 and `f` as a variant), and the all-zero UUID MUST be valid. Braces, the `urn:uuid:` prefix, a missing or misplaced hyphen, any other length, a non-hexadecimal character and a trailing NUL MUST make `is_uuid` return `false`.

#### Scenario: Canonical forms in any case
- **WHEN** `is_uuid` is called on `2eb8aa08-aa98-11ea-b4aa-73b441d16380`, `2EB8AA08-AA98-11EA-B4AA-73B441D16380`, `2eb8aa08-AA98-11ea-B4Aa-73B441D16380`, `00000000-0000-0000-0000-000000000000`, `99c17cbb-656f-f64a-940f-1a4568f03487` and `2eb8aa08-aa98-11ea-f4aa-73b441d16380`
- **THEN** all six return `true`

#### Scenario: Wrong length, dash or character
- **WHEN** `is_uuid` is called on the canonical UUID with any one character removed, with any one character inserted, with any dash replaced by `a`, with any digit replaced by `-`, and on `2eb8aa08aa9811eab4aa73b441d16380` and `2eb8aa08-aa98-11ea-b4ga-73b441d16380`
- **THEN** every call returns `false`

#### Scenario: Braces, prefix and NUL
- **WHEN** `is_uuid` is called on `{2eb8aa08-aa98-11ea-b4aa-73b441d16380}`, `urn:uuid:2eb8aa08-aa98-11ea-b4aa-73b441d16380`, the canonical UUID followed by NUL, and the canonical UUID with a NUL in place of its 33rd character
- **THEN** every call returns `false`

### Requirement: Dates: UUID rule behaviour
`grammar::uuid_rule` SHALL return `error::mismatch` when no hexadecimal digit starts the input and `error::invalid` when one did and the shape broke, leaving `it` unchanged in both cases; on success it yields the 36 characters and stops after the twelfth digit of the last group, so it composes inside a larger grammar.

#### Scenario: Two UUIDs joined by a slash
- **WHEN** `urlg::parse` runs `tuple_rule(fg::uuid_rule, squelch(delim_rule('/')), fg::uuid_rule)` on `2eb8aa08-aa98-11ea-b4aa-73b441d16380/00000000-0000-0000-0000-000000000000`
- **THEN** it succeeds

#### Scenario: The two errors
- **WHEN** `fg::uuid_rule.parse` runs on `2eb8aa08-aa98` and on `x2eb8aa08`
- **THEN** the first answers `error::invalid` and the second `error::mismatch`, and `it` is unchanged in both

### Requirement: Dates: JSON Pointer grammar
`grammar::json_pointer_rule` and `is_json_pointer` SHALL accept RFC 6901 3's `json-pointer`, `*( "/" reference-token )`, where a reference token is any run of characters other than `/` and `~`, and the escapes `~0` and `~1`. The empty string MUST be valid. A string that does not start with `/` and is not empty (`foo`, `#/foo`) MUST be invalid. A `~` followed by anything else or by nothing (`/~2`, `/~`, `/foo~`, `/~a`) MUST be invalid. The twelve pointers of RFC 6901 section 5 MUST be valid, and so MUST `/foo/-`, `/~1.1`, `/~0~1~0~1` and `/~01`.

#### Scenario: RFC 6901 section 5
- **WHEN** `is_json_pointer` is called on ``, `/foo`, `/foo/0`, `/`, `/a~1b`, `/c%d`, `/e^f`, `/g|h`, `/i\j`, `/k"l`, `/ ` (slash, space) and `/m~0n`
- **THEN** all twelve return `true`

#### Scenario: Escapes and refusals
- **WHEN** `is_json_pointer` is called on `/~0~1~0~1`, `/~01`, `/foo/-`, `/~1.1`, `/~2`, `/~`, `/foo~`, `/~a`, `foo` and `#/foo`
- **THEN** the first four return `true` and the last six return `false`

### Requirement: Dates: JSON Pointer is UTF-8 and holds NUL
`is_json_pointer` SHALL decode the input as strict UTF-8 (RFC 3629, `detail::decode_utf8`) and MUST reject a token with an ill-formed sequence: a truncated one, an overlong one, a surrogate (`U+D800` to `U+DFFF`), a code point above `U+10FFFF`, a lone continuation byte or a lead byte such as `0xFF`. A NUL byte MUST be a valid character of a token, since RFC 6901's `unescaped` includes `%x00`.

#### Scenario: Well-formed and ill-formed UTF-8
- **WHEN** `is_json_pointer` is called on `/` followed by the bytes `C3 A9 74 C3 A9`, on `/` and `F0 9F 98 80`, on `/` and `C3`, `/` and `C0 AF`, `/` and `ED A0 80`, and on `/a/` and `FF`
- **THEN** the first two return `true` and the other four return `false`

#### Scenario: NUL is a character
- **WHEN** `is_json_pointer(std::string_view("/a\0b", 4))` is called
- **THEN** it returns `true`

### Requirement: Dates: JSON Pointer rule behaviour
`grammar::json_pointer_rule` SHALL match empty text, without moving `it`, when no `/` starts the input, and so MUST NOT answer `error::mismatch`; once a `/` starts it, the rule MUST run to the end of the input, because a token holds every character a surrounding grammar could continue with. It MUST return `error::invalid`, with `it` unchanged, when a token breaks a rule. The rule MUST read only the range `[it, end)`: a view that ends inside an escape (`/~` cut from `/~0`) is invalid.

#### Scenario: Empty match and a broken escape
- **WHEN** `fg::json_pointer_rule.parse` runs on `foo` and on `/a~2`
- **THEN** the first yields empty text with `it` unchanged and the second answers `error::invalid` with `it` unchanged

#### Scenario: A view that ends inside an escape
- **WHEN** `is_json_pointer(std::string_view("/~0", 2))` is called
- **THEN** it returns `false`, though the byte after the view would complete the escape

### Requirement: Dates: relative JSON Pointer grammar
`grammar::relative_json_pointer_rule_t` and `is_relative_json_pointer` SHALL accept `non-negative-integer [index-manipulation] ( json-pointer / "#" )`, where a non-negative integer is `0` or a digit 1 to 9 followed by any digits (ASCII only; no sign and no leading zero), and the json-pointer after the integer may be empty. `0`, `1/0`, `2/highly/nested/objects`, `0#`, `1#`, `0/objects`, `1/nested/objects`, `2/foo/0`, `120/foo/bar`, `0//`, `99#` and `100` MUST be valid; the empty string, `/foo`, `-1/foo`, `+1/foo`, `01/a`, `01#`, `0##`, `1#/foo`, `0/~2`, `1foo` and an Arabic-Indic digit (`D9 A1`) followed by `/foo` MUST be invalid.

#### Scenario: Examples of draft section 5.1
- **WHEN** `is_relative_json_pointer` is called on `0`, `1/0`, `2/highly/nested/objects`, `0#`, `1#`, `0/objects`, `1/nested/objects`, `2/foo/0`, `120/foo/bar`, `0//`, `99#` and `100`
- **THEN** all twelve return `true` under both policies

#### Scenario: Refused relative pointers
- **WHEN** `is_relative_json_pointer` is called on the empty string, `/foo`, `-1/foo`, `+1/foo`, `01/a`, `01#`, `0##`, `1#/foo`, `0/~2`, `1foo` and the two bytes `D9 A1` followed by `/foo`
- **THEN** all eleven return `false` under both policies

### Requirement: Dates: index manipulation policy
The library SHALL define `enum class index_manipulation { allowed, refused }`. Under `allowed`, the default of `is_relative_json_pointer` and of `relative_json_pointer_rule`, a sign (`+` or `-`) and a non-negative integer without a leading zero MAY follow the first integer, before a json-pointer or before `#`, as draft-bhutton-relative-json-pointer-00 section 3 has it (cited by draft 2020-12 and v1); lexical also accepts it before `#` (`1-1#`), as the draft's own example `0-1#` does though its ABNF omits it. Under `refused`, as in draft-handrews-relative-json-pointer-01 (cited by drafts 7 and 2019-09), no manipulation is valid. `relative_json_pointer_rule_t` MUST carry the policy in a public member `manipulation`, so `relative_json_pointer_rule_t{index_manipulation::refused}` is the rule of drafts 7 and 2019-09.

#### Scenario: Valid when allowed, invalid when refused
- **WHEN** `is_relative_json_pointer` is called on `0-1`, `0-1#`, `0+1/foo`, `1-1#`, `1+2#`, `0+0`, `0-0`, `0-0#`, `2+10/a` and `0+9`
- **THEN** each returns `true` with `index_manipulation::allowed` (and with no second argument) and `false` with `index_manipulation::refused`

#### Scenario: A sign that is no manipulation
- **WHEN** `is_relative_json_pointer` is called on `0+01`, `0-01#`, `0+`, `0-/foo`, `0+#`, `0-#`, `0+-1` and `0+1-1`
- **THEN** every call returns `false` under both policies

### Requirement: Dates: where the relative JSON Pointer rule stops
`grammar::relative_json_pointer_rule_t` SHALL return `error::mismatch` when no ASCII digit starts the input, and `error::invalid`, with `it` unchanged, when its json-pointer breaks a rule. A `#` MUST end the match. Where a sign is not a valid index manipulation (policy `refused`, no integer after the sign, or a leading zero after it), the rule MUST stop before the sign, as the empty json-pointer may end the match there; an integer is `0` alone when a digit follows it (`01#` yields `0`). The scanner `detail::relative_json_pointer_length` MUST itself refuse an empty input and an input not starting with a digit.

#### Scenario: Where the rule stops
- **WHEN** `fg::relative_json_pointer_rule.parse` runs on `0+1/a`, `0+#`, `2#rest`, `12x` and `01#`, and `relative_json_pointer_rule_t{index_manipulation::refused}.parse` runs on `0+1/a`
- **THEN** the matches are `0+1/a`, `0` (with `it` one byte in), `2#`, `12`, `0` and `0` (with `it` one byte in)

#### Scenario: Errors
- **WHEN** `fg::relative_json_pointer_rule.parse` runs on `x` and on `0/~2`
- **THEN** the first answers `error::mismatch` and the second `error::invalid`, both with `it` unchanged

#### Scenario: The scanner's own guard
- **WHEN** `detail::relative_json_pointer_length` is called with the empty string, `x` and `/0` and `index_manipulation::allowed`
- **THEN** each returns no value

### Requirement: Dates: allocation-free identifiers
`is_uuid`, `is_json_pointer` and `is_relative_json_pointer` SHALL allocate nothing, a pointer of 100,000 tokens and a megabyte of input included, and MUST answer in a single pass without recursion.

#### Scenario: A hostile megabyte
- **WHEN** `is_uuid` and `is_json_pointer` are called on a megabyte of `a`, and `is_relative_json_pointer` on a megabyte of `7`
- **THEN** they return `false`, `false` and `true` and the allocation test counts no allocation

### Requirement: Dates: ipv4 and ipv6 are Boost.URL's rules
`is_ipv4` SHALL be `boost::urls::grammar::parse(s, boost::urls::ipv4_address_rule).has_value()` and `is_ipv6` SHALL be the same with `boost::urls::ipv6_address_rule`; lexical MUST NOT define rules of its own for these two formats, and a program composing a grammar uses Boost.URL's two rules directly. An IPv4 address is RFC 2673 3.2's dotted quad whose parts are RFC 3986's `dec-octet`, 0 to 255 without a leading zero. An IPv6 address is one of RFC 4291 2.2's three text forms, without a zone identifier, a prefix length or brackets. Both MUST reject trailing text, a trailing NUL and a non-ASCII digit. `<webcpp/lexical/ip.hpp>` MUST include `<webcpp/lexical/config.hpp>` first, then Boost.URL's two rule headers and `grammar/parse.hpp`.

#### Scenario: Dotted quads
- **WHEN** `is_ipv4` is called on `192.168.0.1`, `0.0.0.0`, `255.255.255.255`, `256.0.0.1`, `192.168.0.01`, `087.10.0.1`, `1.2.3`, `1.2.3.4.5`, `0x7f000001`, `1.2.3.4 `, `1.2.3.4` followed by NUL, and the two bytes `D9 A1` followed by `.2.3.4`
- **THEN** the first three return `true` and the other nine return `false`

#### Scenario: RFC 4291 section 2.2
- **WHEN** `is_ipv6` is called on `ABCD:EF01:2345:6789:ABCD:EF01:2345:6789`, `2001:DB8:0:0:8:800:200C:417A`, `2001:DB8::8:800:200C:417A`, `FF01::101`, `::1`, `::`, `0:0:0:0:0:0:13.1.68.3`, `0:0:0:0:0:FFFF:129.144.52.38`, `::13.1.68.3` and `::FFFF:129.144.52.38`
- **THEN** all ten return `true`

#### Scenario: IPv6 refusals
- **WHEN** `is_ipv6` is called on `fe80::1%eth0`, `2001:db8::/32`, `1:2:3:4:5:6:7:8:9`, `1::2::3`, `::ffff:192.168.0.01`, `12345::`, `[::1]` and `::1` followed by NUL
- **THEN** all eight return `false`

### Requirement: Dates: RFC 3339 and the identifier tests
The tests of this area SHALL be `test/date_time_test.cpp`, `test/uuid_test.cpp`, `test/json_pointer_test.cpp` and `test/ip_test.cpp`, each a program with a `main` using `boost::core::lightweight_test`, and each checking, wherever a rule exists, that the function and the rule agree. `date_time_test` MUST cover RFC 3339 5.8's five examples, every month's last day and the next, February 29 for 0400, 0100, 2000, 2100, 2020 and 2021, leap seconds at each offset under both policies, `T`/`t` and `Z`/`z`, a fraction of one and of 30 digits, appendix A's duration forms and each refused shape, NUL, the errors and the unchanged iterator of every rule, and the rules inside a larger grammar. `json_pointer_test` MUST cover RFC 6901 section 5's twelve pointers, every `~` escape, ill-formed UTF-8, and the relative pointers of the draft's section 5.1 under both policies.

#### Scenario: A failing sample names itself
- **WHEN** a sample's verdict differs from the expected one
- **THEN** the test fails and prints the format and the exact string on standard error

### Requirement: URIs: Public headers and entry points
The URI formats SHALL live in two public headers, `<webcpp/lexical/uri.hpp>` (URI, URI reference, IRI, IRI reference) and `<webcpp/lexical/uri_template.hpp>` (URI template), both reachable through `<webcpp/lexical.hpp>`, with these validators in `webcpp::lexical`: `bool is_uri(std::string_view) noexcept`, `bool is_uri_reference(std::string_view) noexcept`, `bool is_iri(std::string_view)`, `bool is_iri(std::string_view, std::pmr::memory_resource&)`, `bool is_iri_reference(std::string_view)`, `bool is_iri_reference(std::string_view, std::pmr::memory_resource&)` and `bool is_uri_template(std::string_view) noexcept`, each `[[nodiscard]]` and each equal to `boost::urls::grammar::parse(s, <rule>).has_value()` for the rule of the same name in `webcpp::lexical::grammar`. The IRI functions MUST NOT be `noexcept`, because they may throw `std::bad_alloc` through a memory resource; the other three MUST be `noexcept`. Each format maps to the JSON Schema format name `uri`, `uri-reference`, `iri`, `iri-reference` and `uri-template`.

#### Scenario: A validator and its rule never disagree
- **WHEN** `test/uri_test.cpp`, `test/iri_test.cpp` and `test/uri_template_test.cpp` check any input with `is_uri`, `is_uri_reference`, `is_iri`, `is_iri_reference` or `is_uri_template`
- **THEN** the function's verdict equals `urlg::parse(text, fg::<name>_rule).has_value()` for the same input

#### Scenario: Throwing and non-throwing signatures
- **WHEN** the signatures of the seven validators are compared
- **THEN** `is_uri`, `is_uri_reference` and `is_uri_template` are `noexcept`, and the four `is_iri` and `is_iri_reference` overloads are not

### Requirement: URIs: Rules are objects with public types
Each format SHALL also be a Boost.URL grammar rule, an `inline constexpr` object in `webcpp::lexical::grammar` whose type is public: `uri_rule` of `uri_rule_t`, `uri_reference_rule` of `uri_reference_rule_t`, `iri_rule` of `iri_rule_t`, `iri_reference_rule` of `iri_reference_rule_t` and `uri_template_rule` of `uri_template_rule_t`. `uri_rule` and `uri_reference_rule` MUST have `value_type` `boost::urls::url_view`; the other three MUST have `value_type` `grammar::text`. Every rule MUST satisfy Boost.URL's Rule concept with `result<value_type> parse(char const*& it, char const* end) const`, MUST leave `it` unchanged when it fails, and MUST NOT depend on a global.

#### Scenario: A rule composes with Boost.URL's combinators
- **WHEN** `test/grammar_test.cpp` parses `"2020-01-01"` with `urlg::variant_rule(fg::uri_rule, fg::date_rule)`
- **THEN** the parse succeeds and the variant holds alternative index 1, the date, because `uri_rule` refuses a string with no scheme

#### Scenario: A rule that refuses leaves its iterator where it was
- **WHEN** `fg::uri_reference_rule.parse` is called on `"a/%zz"`
- **THEN** it returns an error and the iterator still points at the first character, although Boost.URL's own rule would have advanced it

#### Scenario: A template and an IRI in one grammar
- **WHEN** `"{a}/b http://a/b"` is parsed with `urlg::tuple_rule(fg::uri_template_rule, urlg::squelch(urlg::delim_rule(' ')), fg::iri_rule)`
- **THEN** the parse succeeds, yielding the template `{a}/b` and the IRI `http://a/b`

### Requirement: URIs: uri is RFC 3986 URI through Boost.URL
`is_uri` and `grammar::uri_rule` SHALL be Boost.URL's `boost::urls::uri_rule` (`parse_uri`), RFC 3986 section 3's `URI`, a scheme required and a fragment allowed, followed by lexical's guard (see the guard requirements). `uri_rule` MUST return Boost.URL's error when its rule refuses the input and `error::invalid` when the guard does, and MUST yield the `url_view` of the URI otherwise, stopping where Boost.URL's rule stops. A string that is the absolute-URI form of RFC 3986 4.3 without a fragment is valid too; the fragment is optional.

#### Scenario: The examples of RFC 3986 section 1.1.2
- **WHEN** `is_uri` and `is_uri_reference` check `ftp://ftp.is.co.za/rfc/rfc1808.txt`, `http://www.ietf.org/rfc/rfc2396.txt`, `ldap://[2001:db8::7]/c=GB?objectClass?one`, `mailto:John.Doe@example.com`, `news:comp.infosystems.www.servers.unix`, `tel:+1-816-555-1212`, `telnet://192.0.2.16:80/` and `urn:oasis:names:specification:docbook:dtd:xml:4.1.2`
- **THEN** every one is valid for both functions

#### Scenario: A URI needs a scheme and well-formed characters
- **WHEN** `is_uri` checks `//example.com/p`, `/abc`, `http://example.com/a b`, `http://example.com/%zz` and `"http://a/\0"` (with an embedded NUL)
- **THEN** every verdict is false
- **AND** `http://example.com/p?q#fragment` and `urn:a#` are valid

### Requirement: URIs: uri-reference is a URI or a relative reference
`is_uri_reference` and `grammar::uri_reference_rule` SHALL be Boost.URL's `boost::urls::uri_reference_rule` (`parse_uri_reference`), RFC 3986 section 4.1's `URI-reference`, a URI or a relative reference, followed by lexical's guard, which is the one of `uri_rule` plus the relative-reference guard. The empty string MUST be valid. `uri_reference_rule` MUST return Boost.URL's error when its rule refuses the input and `error::invalid` when the guard does.

#### Scenario: Relative references
- **WHEN** `is_uri_reference` checks `//example.com/p`, the empty string, `#fragment` and `../a/b?c`
- **THEN** every verdict is true

#### Scenario: Characters no reference holds
- **WHEN** `is_uri_reference` checks `\\WINDOWS\fileshare`, `http://example.com/a b` and `"//a/\0"` (with an embedded NUL)
- **THEN** every verdict is false

### Requirement: URIs: Guard against a relative reference with a path that is not path-abempty
`is_uri_reference` and `uri_reference_rule` (and so the IRI-reference forms) SHALL refuse a relative reference that has an authority and a path that is neither empty nor led by `/`, because RFC 3986 section 4.2 gives such a reference a `path-abempty`. The predicate `detail::refuses_relative_authority_path(url_view)` MUST be true exactly when the reference has no scheme, has an authority, and its encoded path is non-empty and does not start with `/`. This guard exists because Boost.URL 1.92 accepts `//example.com:abc/p`, reading host `example.com`, an empty port and the path `abc/p`, and `//a@b@example.com/`, reading user `a`, host `b` and the path `@example.com/`.

#### Scenario: Authority followed by a path not led by a slash
- **WHEN** `is_uri_reference` checks `//example.com:abc/p`, `//a@b@example.com/`, `//:abc/p`, `//example.com:abc`, `//h:1a/p` and `//[::1]x/p`
- **THEN** every verdict is false

#### Scenario: Authorities that stay valid
- **WHEN** `is_uri_reference` checks `//example.com:80/p`, `//a@example.com`, `//example.com`, `//example.com?q`, `//example.com#f`, `//example.com:/p`, `///p` and `//`
- **THEN** every verdict is true

#### Scenario: The guard is still needed
- **WHEN** `uri_test`'s `boost_url_still_needs_the_guard` parses `//example.com:abc/p` with `boost::urls::parse_uri_reference`
- **THEN** Boost.URL accepts it with `encoded_host()` equal to `example.com` and `encoded_path()` equal to `abc/p`, and `//a@b@example.com/` with `encoded_path()` equal to `@example.com/`; the test fails the day a Boost release refuses them, which is the signal to remove `refuses_relative_authority_path` in a change of its own with a changelog line

### Requirement: URIs: Guard against a reference that ends at its authority before a path segment
`uri_rule` and `uri_reference_rule` SHALL refuse with `error::invalid`, rather than match a shorter prefix, a reference that ends at its authority (path, query and fragment empty) before a character that starts a path segment: a `pchar` other than the colon, or the percent sign of a percent-encoded octet. The predicate `detail::refuses_authority_before_segment(url_view, next, end)` MUST be false when `next == end`, when there is no authority, or when the path, query or fragment is present. This guard exists because Boost.URL stops a URI at the second at sign of `http://a@b@example.com/`, so that inside a larger grammar its rule would match `http://a@b`. It stays after Boost.URL refuses the relative forms, because a fixed Boost would still stop a URI at that authority.

#### Scenario: A grammar meets invalid either way
- **WHEN** `fg::uri_reference_rule.parse` is called on `//a@b@example.com/`, `http://a@b@example.com/`, `//example.com:abc/p`, `http://example.com:abc/p`, `//[::1]x/p`, `http://[::1]x/p` and `http://h:1%41`
- **THEN** every call returns `error::invalid` and leaves the iterator at the start

#### Scenario: A character that starts no path segment ends the reference normally
- **WHEN** `uri_reference_rule` parses `http://h:80:x`, `//h:80:x`, `http://h c`, `http://h?q%zz` and `//h?q%zz`
- **THEN** it matches `http://h:80`, `//h:80`, `http://h`, `http://h?q` and `//h?q` respectively, and stops after them

#### Scenario: Two references in one grammar
- **WHEN** `a/b c/d` and `http://u@h:80 //h?q` are parsed with `tuple_rule(uri_reference_rule, squelch(delim_rule(' ')), uri_reference_rule)`
- **THEN** the first yields `a/b` and `c/d`, and the second yields `http://u@h:80` and `//h?q`

### Requirement: URIs: Guard against square brackets in a query
`is_uri`, `is_uri_reference`, `is_iri`, `is_iri_reference` and their rules SHALL refuse `[` and `]` in a query, because RFC 3986 section 3.2.2 holds them only around an IP literal and sections 3.3 to 3.5 hold them percent-encoded as `%5B` and `%5D`. The predicate `detail::refuses_bracket_in_query(url_view)` MUST be true when the reference has a query whose encoded form contains `[` or `]`. Inside a larger grammar the rules MUST answer `error::invalid` for it. An IP literal in the host (`http://[::1]/`) and `%5B` in a query MUST stay valid. Boost.URL 1.92 refuses brackets in a path and a fragment already, and accepts them in a query only.

#### Scenario: Brackets outside an IP literal
- **WHEN** `is_uri` and `is_uri_reference` check `http://a/?[x]`, `http://a/?]`, `http://a/?a=[1]`, `urn:a?[x]`, `http://a/[x]`, `http://a/p]q`, `http://a/#[x]` and `http://[x]@a/`
- **THEN** every verdict is false
- **AND** `is_uri_reference` refuses `?[x]`, `a?[x]`, `//h?]`, `/p?a=[1]` and `#[x]`

#### Scenario: IP literals and percent-encoded brackets stay valid
- **WHEN** `is_uri` checks `http://[::1]/`, `http://[::1]:80/p?q#f`, `http://a/?%5Bx%5D` and `http://[::1]/?%5B%5D#%5D`
- **THEN** every verdict is true
- **AND** `is_uri_reference` accepts `//[::1]/p` and `?%5B`

#### Scenario: The rule refuses it inside a grammar
- **WHEN** `uri_rule` parses `http://a/?[x] rest`, and `uri_reference_rule` parses `?[x] rest`
- **THEN** both return `error::invalid` with the iterator unchanged, where Boost.URL's rule would match up to the space

#### Scenario: The guard is still needed
- **WHEN** `boost_url_still_takes_brackets_in_a_query` parses `http://a/?[x]` with `parse_uri` and `?[x]` with `parse_uri_reference`
- **THEN** both succeed; the test fails the day Boost refuses them, when `refuses_bracket_in_query` may leave in a change of its own while `brackets_only_around_an_ip_literal` stays

### Requirement: URIs: Guard against a second number sign in a fragment
`is_uri`, `is_uri_reference`, `is_iri`, `is_iri_reference` and their rules SHALL refuse a `#` inside a fragment, because RFC 3986 section 3.5 gives a fragment `pchar`, `/` and `?` alone: the first `#` starts the fragment and a second is held only percent-encoded as `%23`. The predicate `detail::refuses_number_sign_in_fragment(url_view)` MUST be true when the reference has a fragment whose encoded form contains `#`. Boost.URL 1.92 adds `#` to its fragment characters on purpose (its commit "hash is fragment char"), so `http://a/#x#y` is a URI for Boost.URL with fragment `x#y`. Inside a larger grammar the rules MUST answer `error::invalid`.

#### Scenario: A second number sign
- **WHEN** `is_uri` and `is_uri_reference` check `http://a/#x#y`, `http://a/#x#`, `http://a/##`, `http://a/?q#x#y`, `urn:a##` and `http://[::1]/#?#`
- **THEN** every verdict is false
- **AND** `is_uri_reference` refuses `#x#y`, `##`, `a#x#y`, `//h/#x#` and `?q##`

#### Scenario: A percent-encoded number sign stays valid
- **WHEN** `is_uri` checks `http://a/#x%23y`, `http://a/#%23`, `http://a/?q#x?y/z` and `http://a/?q%23#x%23`
- **THEN** every verdict is true
- **AND** `is_uri_reference` accepts `#x%23y`, `#` and `a?%23#%23`

#### Scenario: Inside a grammar
- **WHEN** `uri_rule` parses `http://a/#x#y rest` and `uri_reference_rule` parses `#x#y rest` and `#x%23y rest`
- **THEN** the first two return `error::invalid` with the iterator unchanged, and the third matches `#x%23y`

#### Scenario: The guard is still needed
- **WHEN** `boost_url_still_takes_a_number_sign_in_a_fragment` parses `http://a/#x#y` with `parse_uri`
- **THEN** it succeeds with `encoded_fragment()` equal to `x#y`; the test fails the day Boost refuses it, when the guard may leave while `one_number_sign_before_the_fragment` stays

### Requirement: URIs: Guard against a zone identifier in an IP literal
`is_uri`, `is_uri_reference`, `is_iri`, `is_iri_reference` and their rules SHALL refuse a zone identifier in an IP literal, because RFC 3986 section 3.2.2's `IP-literal` holds an IPv6address or an IPvFuture and RFC 9844 (2025) obsoleted RFC 6874, which had added the zone identifier, and reverted its change to RFC 3986. The predicate `detail::refuses_zone_identifier(url_view)` MUST be true when the host type is IPv6 and the encoded host contains `%`. Boost.URL 1.92 takes RFC 6874's form. A `%25` outside the IP literal MUST stay valid. Inside a larger grammar the rules MUST answer `error::invalid`.

#### Scenario: Zone identifiers are refused
- **WHEN** `is_uri` and `is_uri_reference` check `http://[fe80::1%25eth0]/`, `http://[::1%25a]:80/p?q#f`, `http://u@[fe80::1%25%41]/`, `http://[::ffff:1.2.3.4%251]` and `urn://[::1%25x]`
- **THEN** every verdict is false
- **AND** `is_uri_reference` refuses `//[fe80::1%25eth0]/` and `//[::1%25x]`

#### Scenario: A percent sign elsewhere stays valid
- **WHEN** `is_uri` checks `http://[fe80::1]/`, `http://[::ffff:1.2.3.4]/%25`, `http://%25@h/?%25#%25`, `http://%25@[::1]:80/` and `http://u%41:p%25@[::1]/p?%25`
- **THEN** every verdict is true
- **AND** `is_uri_reference` accepts `//[::1]/%25`, `//a%25b@[fe80::1]/%25` and `//a%25b@[fe80::1]:8080?q%25`

#### Scenario: Inside a grammar
- **WHEN** `uri_reference_rule` parses `http://[::1%25x]/ rest` and `//[::1%25x] rest`
- **THEN** both return `error::invalid` with the iterator unchanged

#### Scenario: The guard is still needed
- **WHEN** `boost_url_still_takes_a_zone_identifier` parses `http://[fe80::1%25eth0]/` with `parse_uri`
- **THEN** it succeeds with `encoded_host()` equal to `[fe80::1%25eth0]`; the test fails the day Boost refuses it, when the guard may leave while `no_zone_identifier` stays

### Requirement: URIs: Guard composition and its life cycle
The guard SHALL be applied after Boost.URL's rule succeeds, in `uri_rule` as `refuses_authority_before_segment || refuses_bracket_in_query || refuses_number_sign_in_fragment || refuses_zone_identifier` and in `uri_reference_rule` with `refuses_relative_authority_path` first; when any is true the rule MUST restore the iterator and return `detail::urlg::error::invalid`. The IRI rules MUST reach the same guard because they parse their mapped URI with `uri_rule` and `uri_reference_rule`, so the four functions agree on every guard. Boost.URL MUST NOT be modified. Every guard MUST have a pin test (`boost_url_still_needs_the_guard`, `boost_url_still_takes_brackets_in_a_query`, `boost_url_still_takes_a_number_sign_in_a_fragment`, `boost_url_still_takes_a_zone_identifier`) that fails when the configured Boost refuses the input itself; a guard then leaves in a change of its own with a changelog line, while the RFC-based test of the same name without `boost_url_still` stays. Where Boost.URL is wrong, lexical SHALL offer the fix upstream.

#### Scenario: Both IRI forms apply the guards
- **WHEN** `is_iri` checks `http://a/?[x]`-style inputs with non-ASCII text (`http://a/?[\xC3\xA9]`, `http://\xC3\xA9/#x#y`, `http://[::1%25eth0]/`) and `is_iri_reference` checks `//\xC3\xA9:abc/p`
- **THEN** every verdict is false, as for the ASCII URI

#### Scenario: A guard leaves independently
- **WHEN** a future Boost refuses brackets in a query
- **THEN** `boost_url_still_takes_brackets_in_a_query` fails, and only `refuses_bracket_in_query` is removed; the other pin tests and guards are untouched

### Requirement: URIs: IPvFuture keeps Boost.URL's stricter verdict
lexical SHALL keep one verdict of Boost.URL 1.92 that RFC 3986 does not give, because it never loosens Boost.URL: an IPvFuture led by an uppercase `V` MUST be refused in `is_uri`, `is_uri_reference`, `is_iri` and `is_iri_reference`, although RFC 5234 section 2.3 makes ABNF's quoted strings case-insensitive and RFC 3986's `"v"` matches it. A lowercase `v` MUST stay valid. The deviation is recorded in the guide's "Known deviations" and in `AGENTS.md`.

#### Scenario: Uppercase V is refused, lowercase accepted
- **WHEN** `is_uri` checks `http://[V1.x]/`, `http://[v1.x]/` and `http://[vF.x:y]/`, and `is_uri_reference` checks `//[V1.x]/` and `//[v1a.!$]/`
- **THEN** the uppercase forms are false and the lowercase forms are true
- **AND** `is_iri("http://[V1.x]/")` is false and `is_iri("http://[v1.x]/")` is true

#### Scenario: The day Boost takes it
- **WHEN** `ipvfuture_keeps_boost_urls_verdict` fails because Boost.URL accepts `V`
- **THEN** the two verdicts above and `iri_test`'s on `http://[V1.x]/` flip to valid, and the deviation leaves the guide and `AGENTS.md`

### Requirement: URIs: Known deviations from RFC 3986
The URI formats SHALL differ from a literal reading of RFC 3986 only as the guide's "Known deviations" lists: lexical refuses four things Boost.URL accepts (a relative reference whose path after an authority is neither empty nor led by `/`; `[` or `]` in a query; `#` in a fragment; a zone identifier in an IP literal), its rules additionally refuse a reference that Boost.URL's would stop at an authority before a path segment (as at the second `@` of `http://a@b@example.com/`), and it keeps the uppercase-`V` IPvFuture refusal. These are all the differences found on 2026-10-09, when the verdicts of Boost.URL 1.92 and of lexical were compared with a transcription of RFC 3986's ABNF, on inputs that put every ASCII character but NUL and line feed into each part of a reference, and on references generated at random. The tests MUST pin each of these verdicts.

#### Scenario: Each deviation has a pinned verdict
- **WHEN** `test/uri_test.cpp` runs
- **THEN** `the_guard`, `brackets_only_around_an_ip_literal`, `one_number_sign_before_the_fragment`, `no_zone_identifier`, `ipvfuture_keeps_boost_urls_verdict` and `the_rule_inside_a_grammar` each assert the verdicts of their deviation

### Requirement: URIs: IRI is checked by code point and mapped to a URI
`is_iri` and `grammar::iri_rule` SHALL check RFC 3987 section 2.2's `IRI` by validating each non-ASCII code point, mapping the IRI to a URI as RFC 3987 section 3.1 says, and parsing the result with `uri_rule` (guard included); `is_iri_reference` and `grammar::iri_reference_rule` SHALL do the same for section 2.2's `IRI-reference` and parse with `uri_reference_rule`. Each non-ASCII code point MUST be well-formed UTF-8 (no truncated, overlong or surrogate sequence) and in `ucschar`, or in `iprivate` within the query. Boost.URL parses RFC 3986 only, so lexical checks what RFC 3987 adds and Boost.URL judges the structure: a `%XX` is valid exactly where RFC 3987 allows `ucschar`.

#### Scenario: ucschar boundaries
- **WHEN** `is_iri` checks `http://a/\xC2\xA0` (U+00A0), `http://a/\xC2\x80` (U+0080), `http://a/\xEF\xBF\xBD` (U+FFFD) and `http://a/\xEE\x80\x80` (U+E000)
- **THEN** U+00A0 is valid, being the first `ucschar`; U+0080 (a C1 control), U+FFFD (outside `ucschar`) and U+E000 (`iprivate` in a path) are refused

#### Scenario: Ill-formed UTF-8
- **WHEN** `is_iri` checks `http://a/\xC3` (truncated), `http://a/\xC0\xAF` (overlong) and `http://a/\xED\xA0\x80` (a surrogate)
- **THEN** every verdict is false

#### Scenario: Non-ASCII in hosts, userinfo, paths and queries
- **WHEN** `is_iri` checks `http://\xC3\xA9@\xC3\xA9/\xC3\xA9?\xC3\xA9#\xC3\xA9` and `http://\xC6\x92\xC3\xB8\xC3\xB8.\xC3\x9F\xC3\xA5r:/` (an empty port)
- **THEN** both are valid, and `is_iri_reference("//\xC6\x92\xC3\xB8\xC3\xB8.\xC3\x9F\xC3\xA5r/?\xE2\x88\x82")` is valid too

#### Scenario: Plain structure still fails
- **WHEN** `is_iri` checks `http://example.com:8o/`, `http://ex ample.com/`, `1http://a`, `http://a:b@c:d@e/` and `//\xC3\xA9/p`, and `HTTP://a`, `a:`
- **THEN** the first five are false (the last for lack of a scheme) and `HTTP://a` and `a:` are true

### Requirement: URIs: IRI mapping to percent-encoded octets
The mapping of an IRI to a URI (`detail::map_iri`) SHALL replace each octet of each non-ASCII code point with `%XX` using uppercase hexadecimal digits, and leave ASCII octets unchanged, so the mapped length is the IRI's length plus two for every non-ASCII octet. Because RFC 3986 allows `%XX` everywhere that RFC 3987 allows `ucschar`, Boost.URL MUST refuse a non-ASCII code point in a scheme or a port, and lexical MUST refuse it there. No IDNA is involved: an IRI's host is a registered name of any `ucschar`.

#### Scenario: Non-ASCII where RFC 3986 has no percent-encoding
- **WHEN** `is_iri` checks `h\xC3\xA9:x` (scheme) and `http://a:8\xC3\xA9/` (port)
- **THEN** both are false

#### Scenario: A non-ASCII host is accepted without IDNA
- **WHEN** `is_iri` checks `http://\xC6\x92\xC3\xB8\xC3\xB8.example/\xCF\x80`
- **THEN** the verdict is true

### Requirement: URIs: Bidirectional formatting characters are refused
`is_iri` and `is_iri_reference` SHALL refuse the bidirectional formatting characters U+200E, U+200F and U+202A to U+202E (`detail::is_bidi_format`) anywhere in an IRI, including the query, as RFC 3987 section 4.1 requires. Their neighbours MUST stay valid: U+200D, U+2010, U+2029 and U+202F are `ucschar`.

#### Scenario: Each formatting character
- **WHEN** `is_iri` checks `http://a/` followed by U+200E (`\xE2\x80\x8E`), U+200F (`\xE2\x80\x8F`), U+202A (`\xE2\x80\xAA`), U+202B, U+202C, U+202D, and `http://a/?` followed by U+202E (`\xE2\x80\xAE`)
- **THEN** every verdict is false

#### Scenario: The neighbours are valid
- **WHEN** `is_iri` checks `http://a/` followed by U+200D, U+2010, U+2029 or U+202F
- **THEN** every verdict is true

### Requirement: URIs: Non-ASCII is refused inside an IP literal
`is_iri` and `is_iri_reference` SHALL refuse any non-ASCII code point inside an IP literal (between the `[` that opens it in the authority and the end of the authority), before anything is mapped, because RFC 3987 takes its IP literal from RFC 3986, which is ASCII alone. The authority is located as RFC 3986 appendix B does: a scheme ends at a colon that comes before any `/`, `?` or `#`, and an authority starts with `//` and runs to the next `/`, `?` or `#`. This check MUST make the verdict independent of the zone guard.

#### Scenario: Non-ASCII in an IPv6 or IPvFuture literal
- **WHEN** `is_iri` checks `http://[::\xC3\xA9]/`, `http://[::1%25\xC3\xA9]/` and `http://[v1.\xC3\xA9]/`, and `is_iri_reference` checks `//[::1%25\xC3\xA9]/` and `//u@[::1%25e\xC3\xA9]:80/p`
- **THEN** every verdict is false

#### Scenario: Non-ASCII beside an IP literal
- **WHEN** `is_iri` checks `http://\xC3\xA9@[::1]/\xC3\xA9?\xC3\xA9` and `http://[::1]/\xC3\xA9`
- **THEN** both are valid

### Requirement: URIs: iprivate is valid in the query alone
`is_iri` and `is_iri_reference` SHALL accept `iprivate` code points (U+E000 to U+F8FF, U+F0000 to U+FFFFD, U+100000 to U+10FFFD) only in the query, defined as the text after the first `?` that precedes the first `#` (RFC 3986 appendix B: a valid IRI holds no `?` before its query and no `#` before its fragment). `ucschar` MUST be U+00A0 to U+D7FF, U+F900 to U+FDCF, U+FDF0 to U+FFEF and, in each supplementary plane from 1 to 14, U+x0000 to U+xFFFD except that plane 14 starts at U+E1000 and ends at U+EFFFD.

#### Scenario: iprivate in a query only
- **WHEN** `is_iri` checks `http://a/?\xEE\x80\x80`, `http://a/?x?\xEE\x80\x80`, `http://a/?\xF3\xB0\x80\x80` (U+F0000), `http://a/?\xEE\x80\x80#x`, `http://a/#\xEE\x80\x80`, `http://a/#?\xEE\x80\x80`, `http://a/?\xEE\x80\x80#\xEE\x80\x80` and `http://\xEE\x80\x80/?x`
- **THEN** the first four are true and the last four are false, a `?` inside a fragment starting no query

#### Scenario: Relative references
- **WHEN** `is_iri_reference` checks `?\xEE\x80\x80`, `#\xEE\x80\x80` and `x#y?\xEE\x80\x80`
- **THEN** the first is true and the other two are false

### Requirement: URIs: The IRI-reference guard applies to non-ASCII references
`is_iri_reference` and `iri_reference_rule` SHALL refuse a non-ASCII relative reference exactly as the guard of `uri_reference_rule` refuses an ASCII one, since the mapped URI is parsed by `uri_reference_rule`.

#### Scenario: Guard on a non-ASCII authority
- **WHEN** `is_iri_reference` checks `//\xC3\xA9:abc/p`, `//a@b@\xC3\xA9/`, `//\xC3\xA9:80/p` and `//\xC3\xA9`, and `is_iri` checks `//\xC3\xA9/p`
- **THEN** the first two are false, the third and fourth are true, and the `is_iri` input is false for lack of a scheme

#### Scenario: A first segment with a colon
- **WHEN** `is_iri_reference` checks `./\xC3\xA9:x` and `\xC3\xA9:x`
- **THEN** the first is true and the second is false

### Requirement: URIs: Where an IRI ends inside a larger grammar
`iri_rule` and `iri_reference_rule` SHALL take the run from the start to the first byte no IRI holds, and validate that run whole; they MUST NOT match a shorter prefix of a run that is invalid. The bytes no IRI holds (`detail::ends_iri`) are: every ASCII control (0x00 to 0x1F), the space, 0x7F, the double quote, the less-than and greater-than signs, the backslash, the circumflex, the grave accent, the left brace, the vertical bar and the right brace. Every other byte, brackets and percent signs included, belongs to the run. `iri_rule` MUST return `error::mismatch` when such a byte is at the start (an empty run), `error::invalid` when the run is no IRI, and the matched `grammar::text` otherwise. `iri_reference_rule` MUST accept the empty reference, so it matches empty text where a byte no IRI holds is at the start and never answers mismatch; it returns `error::invalid` when the run is no reference.

#### Scenario: Each terminating byte
- **WHEN** `iri_rule` parses `http://a/\xC3\xA9` followed by NUL, TAB, LF, 0x1F, the space, `"`, `<`, `>`, `\`, `^`, `` ` ``, `{`, `|`, `}` or 0x7F and then `x`
- **THEN** each parse matches `http://a/\xC3\xA9` and leaves the iterator 11 bytes in

#### Scenario: A run refused whole
- **WHEN** `iri_rule` parses `http://a/%zz rest`, `http://a/[x]` and `http://a/\xE2\x80\x8E rest`
- **THEN** each returns `error::invalid` with the iterator at the start, and never matches the valid prefix `http://a/`

#### Scenario: Mismatch and the empty reference
- **WHEN** `iri_rule` parses ` x`, and `iri_reference_rule` parses ` x`
- **THEN** the first returns `error::mismatch`, and the second returns empty text with the iterator unchanged

#### Scenario: Two IRIs in one grammar
- **WHEN** `http://a/\xC3\xA9 \xC3\xA9tail` is parsed with `tuple_rule(iri_rule, squelch(delim_rule(' ')), iri_reference_rule)`
- **THEN** the parse succeeds, yielding `http://a/\xC3\xA9` and `\xC3\xA9tail`

#### Scenario: A second number sign is refused whole
- **WHEN** `iri_rule` parses `http://a/#\xC3\xA9#x rest` and `iri_reference_rule` parses `#\xC3\xA9#x rest`
- **THEN** both return `error::invalid` with the iterator unchanged

### Requirement: URIs: The mapped URI buffer holds 1,024 octets
The mapped URI SHALL be written into storage chosen by length: for an ASCII IRI, none (the IRI is handed to Boost.URL as it is, never copied); for a mapped length up to `detail::iri_inline_capacity` (1,024 octets), an uninitialised `std::array<char, 1024>` on the stack, which holds any IRI of up to 341 octets since mapping at most triples a length; beyond that, one buffer of exactly the mapped length from the memory resource, released when the parse ends, also if it throws. The codepoints MUST be checked before anything is mapped, so an IRI refused for a code point takes no buffer, and `detail::mapped_length` MUST return nothing for it.

#### Scenario: The edge of the inline buffer
- **WHEN** `is_iri` is called with a counting resource on `http://a/` followed by 169 U+00E9 and an `x` (1,024 mapped octets), and then on the same string with one more `x`
- **THEN** the first makes 0 allocations, and the second makes 1 allocation of 1,025 bytes and 1 deallocation

#### Scenario: Any 341-octet IRI fits
- **WHEN** `is_iri_reference` is called with a counting resource on `a:` followed by 168 U+00E9 and U+20AC (341 octets), and on 113 U+20AC followed by one U+00E9 (341 octets, 1,023 mapped)
- **THEN** neither allocates
- **AND** 171 U+00E9 (342 octets, 1,026 mapped) allocates once, 1,026 bytes, and deallocates once

#### Scenario: A refused IRI takes no buffer
- **WHEN** `is_iri` and `is_iri_reference` are called with `std::pmr::null_memory_resource()` on `http://a/` followed by 1,000 U+00E9 and U+E000
- **THEN** both return false without throwing

### Requirement: URIs: Memory resource overloads
`is_iri(s, resource)`, `is_iri_reference(s, resource)` and the rule types `iri_rule_t{&resource}` and `iri_reference_rule_t{&resource}` SHALL take the mapped buffer of an IRI beyond 1,024 mapped octets from the `std::pmr::memory_resource` given. `iri_rule_t` and `iri_reference_rule_t` MUST have a public `std::pmr::memory_resource* resource` member, null by default, and a null resource MUST mean `std::pmr::get_default_resource()`; the one-argument functions and `iri_rule` and `iri_reference_rule` use it. An ASCII IRI MUST NOT reach the resource, however long.

#### Scenario: A resource of the program's own
- **WHEN** `example/lexical_iri.cpp` builds a 4,096-byte `std::pmr::monotonic_buffer_resource` with `std::pmr::null_memory_resource()` as upstream and calls `is_iri` with it on `http://\306\222\303\270\303\270.example/\317\200`, on `http://example.com/` followed by 300 U+00E9 (1,819 mapped octets), and on `http://a/\342\200\216`
- **THEN** it prints `short: true`, `long: true` and `bidi: false`

#### Scenario: A long ASCII IRI never touches the resource
- **WHEN** `is_iri` and `is_iri_reference` are called with `std::pmr::null_memory_resource()` on `http://example.com/` followed by a million `a`
- **THEN** both return true and the resource is never called

### Requirement: URIs: A failing resource throws bad_alloc, never a verdict
When the memory resource cannot allocate the buffer of an IRI beyond 1,024 mapped octets, `is_iri`, `is_iri_reference`, `iri_rule_t::parse` and `iri_reference_rule_t::parse` SHALL propagate what the resource throws (`std::bad_alloc` for `std::pmr::null_memory_resource()`), and MUST NOT answer a verdict without the mapped copy. This is the only exception lexical's validators can raise. `test/iri_resource_test.cpp` MUST be built with `<exception-handling>on` whatever the build asks, because it catches on purpose.

#### Scenario: null_memory_resource refuses a long IRI
- **WHEN** `is_iri` and `is_iri_reference` are called with `*std::pmr::null_memory_resource()` on `http://a/` followed by 200 U+00E9
- **THEN** each throws `std::bad_alloc`

### Requirement: URIs: IRI allocation counts
Beyond 1,024 mapped octets an IRI function SHALL make exactly one allocation of the mapped length and give it back: with a given resource, one call of that resource; with the default resource, one call of the global `operator new` and one `operator delete`. An ASCII IRI, and a non-ASCII one whose mapped URI fits 1,024 octets, MUST make no allocation. `is_uri`, `is_uri_reference` and `is_uri_template` MUST make none whatever the input.

#### Scenario: The allocation test
- **WHEN** `test/allocation_test.cpp` calls `is_iri` on `a:` followed by 168 U+00E9 and U+20AC, on `http://example.com/` followed by a million `a`, on `//\xC6\x92\xC3\xB8\xC3\xB8.example/` as an IRI reference, and `is_uri_template` on 100,000 repetitions of `{+v:12}/x`
- **THEN** the global allocation count does not change
- **AND** `is_iri(beyond, resource)` for `http://a/` followed by 100,000 U+00E9 makes one allocation of 600,009 bytes and one deallocation on `resource`, and `is_iri_reference(beyond)` adds one global allocation, each given back

### Requirement: URIs: URI template grammar
`is_uri_template` and `grammar::uri_template_rule` SHALL check RFC 6570 section 2's `URI-Template` of any level, `*( literals / expression )`, without expanding it, with erratum 6937 (verified in 2022) making the apostrophe a literal as JSON Schema's suite expects. The empty template MUST be valid. The rule MUST be built from Boost.URL combinators (`tuple_rule`, `variant_rule`, `optional_rule`, `range_rule` over `urlg::ref`), with one rule per production and a lexical `repeat_rule` wherever the ABNF repeats, so it yields text and builds no type-erased range.

#### Scenario: Templates of every level in RFC 6570 section 1.2
- **WHEN** `test/uri_template_test.cpp` checks its 66 templates, among them `{var}`, `{+path}/here`, `X{#hello}`, `map?{x,y}`, `X{.x,y}`, `{/var,x}/here`, `{;x,y,empty}`, `{?x,y}`, `?fixed=yes{&x}`, `{var:30}`, `{list*}`, `{+path:6}/here`, `{/list*,path:4}`, `{;hello:5}` and `{&keys*}`
- **THEN** every one is valid

#### Scenario: Apostrophe and the empty template
- **WHEN** `is_uri_template` checks `a'b` and the empty string
- **THEN** both are true

### Requirement: URIs: URI template literals
A literal in a template SHALL be one or more of: an ASCII character from `detail::template_literal_chars`, which is %x21, %x23-24, %x26-3B, %x3D, %x3F-5B, %x5D, %x5F, %x61-7A and %x7E (so every printable ASCII but the space, the double quote, the percent sign, the less-than and greater-than signs, the backslash, the circumflex, the grave accent, the braces and the vertical bar; the apostrophe is one by erratum 6937); a pct-encoded triplet (`%` and two hex digits); or a `ucschar` or `iprivate` code point as well-formed UTF-8, so a template of an IRI is valid. A literal run that meets a byte that is none of these MUST end there. A `%` not followed by two hex digits, a NUL, an ill-formed UTF-8 sequence, a noncharacter such as U+FDD0, a surrogate, an overlong sequence and a C1 control (U+0080) MUST make the template invalid.

#### Scenario: Characters that are not literals
- **WHEN** `is_uri_template` checks `a<b`, `a"b`, `a^b`, `a|b`, `a\b`, `` a`b ``, `a b`, `a%`, `a%2`, `a%GG`, `a%%41` and a template holding NUL
- **THEN** every verdict is false

#### Scenario: Non-ASCII literals
- **WHEN** `is_uri_template` checks `/\xEE\x80\x80/\xF3\xB0\x80\x80` (iprivate), `/\xC2\xA0/\xF0\x90\x80\x80` (ucschar), `/\xEF\xB7\x90`, `/\xED\xA0\x80`, `/\xC0\xAF`, `a\xC2\x80b`, `a\xEF\xBF\xBDb` and `a\xC3`
- **THEN** the first two are true and the others false

#### Scenario: Pct-encoded literals
- **WHEN** `is_uri_template` checks `%20{v}%2f`
- **THEN** the verdict is true

### Requirement: URIs: URI template expressions
An expression SHALL be `{`, an optional operator, a variable list and `}` (RFC 6570 sections 2.2 and 2.3). The operator MUST be any of `+ # . / ; ? & = , ! @ |` (`detail::template_operator_chars`), the reserved `op-reserve` operators `=`, `,`, `!`, `@` and `|` included because the ABNF admits them. A variable list MUST be one or more `varspec` separated by single commas, each a `varname` and an optional modifier. A `varname` MUST be one or more `varchar` (ASCII letter, digit, underscore or a pct-encoded triplet), with single dots allowed between runs; it MUST NOT start or end with a dot, or hold two dots in a row. The modifier MUST be either a prefix `:` followed by a max-length of 1 to 9999 with no leading zero (`%x31-39 0*3DIGIT`), or the explode `*`, not both and not repeated. An expression that is empty, has an empty variable list, a trailing comma or a non-`varchar` character MUST make the template invalid.

#### Scenario: Names
- **WHEN** `is_uri_template` checks `{_a}`, `{a.b.c}`, `{a.%41}`, `{%41b}`, `{a.}`, `{..a}`, `{a..b}`, `{a b}`, `{-var}`, `{\xC3\xA4}`, `{a%2}` and `{%}`
- **THEN** the first four are true and the others false

#### Scenario: Prefix and explode
- **WHEN** `is_uri_template` checks `{v:9999}`, `{v:1000}`, `{v:10000}`, `{v:0}`, `{v:01}`, `{v:}`, `{v*}`, `{v**}` and `{v*:3}`
- **THEN** `{v:9999}`, `{v:1000}` and `{v*}` are true and the others false

#### Scenario: Variable lists and operators
- **WHEN** `is_uri_template` checks `{}`, `{,}`, `{+}`, `{v,}`, `{a,}`, `{,v}`, `{,+var}`, `{=v}{!v}{@v}{|v}`, `{{var}}`, `{` and `}`
- **THEN** `{,v}` (the comma is an `op-reserve` operator) and `{=v}{!v}{@v}{|v}` are true, and all the others are false

#### Scenario: Mixed template
- **WHEN** `is_uri_template` checks `{v:5}{w*}/\xC3\xA9{?x,y}`
- **THEN** the verdict is true

### Requirement: URIs: Where the URI template rule stops
`uri_template_rule` SHALL stop at the first byte that is neither a literal nor the start of an expression, and match the text before it, possibly empty. An expression that starts with `{` and breaks, and a `%` that starts and breaks a pct-encoded triplet, MUST make the rule fail as a whole with `error::invalid` and leave the iterator unchanged, rather than match a shorter prefix. A byte that is a valid UTF-8 prefix cut by the end of the input, or a code point that is not a literal, ends the match before it.

#### Scenario: A template ends before what it cannot hold
- **WHEN** `uri_template_rule` parses `{a}/b }rest`, `x\xC3\xA9 y`, `x\xC3` and ` x`
- **THEN** it matches `{a}/b`, `x\xC3\xA9`, `x` and the empty text, stopping after each

#### Scenario: A broken expression or triplet fails whole
- **WHEN** `uri_template_rule` parses `x{a`, `x{a b}`, `{`, `x%G` and `x{a}%`
- **THEN** every call returns `error::invalid` and the iterator is at the start

### Requirement: URIs: URI template rule needs no allocation and does not throw
`is_uri_template` and `uri_template_rule_t::parse` SHALL be `noexcept` and allocate nothing, whatever the input: no `range_rule` of the template leaves Boost.URL's inline buffer (the part rule is held by reference with `urlg::ref`), and the only throw its analysis finds, `result::value()`, comes from a result that holds an error and is never called on one.

#### Scenario: A long template
- **WHEN** `is_uri_template` checks 100,000 repetitions of `{+v:12}/x`
- **THEN** it returns true with no allocation

### Requirement: URIs: Mapping to JSON Schema's formats
The JSON Schema Test Suite runner SHALL map the format names `uri` to `is_uri`, `uri-reference` to `is_uri_reference`, `iri` to `is_iri`, `iri-reference` to `is_iri_reference` and `uri-template` to `is_uri_template`. These formats apply from the drafts in which they exist: `uri` in every draft supported, `uri-reference` and `uri-template` from draft 6, `iri` and `iri-reference` from draft 7. No URI, IRI or URI template test of the suite MAY be on a known-failures list, since only a residue tied to a Unicode version may be one; every test of these five formats passes.

#### Scenario: The suite's URI formats pass
- **WHEN** `test/suite_test.cpp` runs the format tests of `uri`, `uri-reference`, `iri`, `iri-reference` and `uri-template` for every dialect
- **THEN** none fails, and no `test/known_failures/<dialect>.json` file names one of them

### Requirement: URIs: Boost.URL is a compiled dependency
The URI and IRI validators SHALL depend on Boost.URL's compiled part, so a program that uses them links `/webcpp//boost_url`, which `/webcpp/lexical//lexical` brings, built from Boost's sources for every target (`ipv4_address` is defined there, and on MSVC, which never defines `BOOST_URL_HAS_CXX20_CONSTEXPR`, even the grammar's error category). Measured on 2026-10-09 with Boost 1.92 and wasi-sdk 34, built with b2's release flags (`-O3`, wasm exceptions) and stripped at link time, a wasm32-wasip2 program that calls `is_uri` is 85,625 bytes larger than an empty one.

#### Scenario: A program links the library through the lexical target
- **WHEN** a b2 program names `/webcpp/lexical//lexical` and calls `webcpp::lexical::is_uri`
- **THEN** it links `/webcpp//boost_url` without naming it and the call resolves

### Requirement: Hosts: hostname validator and rule
`<webcpp/lexical/hostname.hpp>` SHALL declare `[[nodiscard]] bool is_hostname(std::string_view) noexcept` for JSON Schema's `hostname` and the Boost.URL rule `webcpp::lexical::grammar::hostname_rule` (an object of type `hostname_rule_t`, whose `value_type` is `grammar::text` and whose `parse(char const*& it, char const* end) const noexcept` yields a `result<text>`), and `is_hostname(s)` MUST equal `boost::urls::grammar::parse(s, grammar::hostname_rule).has_value()` for every string, so that the whole string must be one host name and trailing input is refused as `error::leftover`.

#### Scenario: Validator and rule agree
- **WHEN** `is_hostname("www.example.com")` is called and `parse("www.example.com", hostname_rule)` is evaluated
- **THEN** both are valid, and the rule's value equals the whole string

#### Scenario: Trailing input is refused by the validator
- **WHEN** `is_hostname("example.com ")` and `is_hostname("example.com\n")` are called
- **THEN** both are false

### Requirement: Hosts: hostname labels and lengths
`is_hostname` SHALL accept RFC 1123 2.1's host names and nothing else: ASCII only; labels of 1 to 63 octets made of letters, digits and hyphens (`-`), a hyphen neither first nor last; labels joined by single dots; no leading dot, no trailing dot (so the root label of `example.` is an empty last label and the name is invalid), no empty label; and the whole name at most 253 octets (`idna::max_domain_length`, labels `idna::max_label_length` = 63). A label may start with a digit, and a name of digits only such as `test.123` is valid. The characters outside `[A-Za-z0-9.-]`, the underscore, the space, a control character, NUL and every non-ASCII byte included, make the string invalid.

#### Scenario: Ordinary host names
- **WHEN** `is_hostname` is called with `www.example.com`, `hostname`, `h0stn4me`, `1host`, `host-name`, `a--b.com` and `test.123`
- **THEN** every call is true

#### Scenario: Malformed names
- **WHEN** `is_hostname` is called with the empty string, `.`, `.example`, `example.`, `a..b`, `-hostname`, `hostname-`, `host_name`, `example.com\n` and `example.com ` (with a trailing space)
- **THEN** every call is false

#### Scenario: Label length at 63 and 64 octets
- **WHEN** `is_hostname` is called with 63 times `a` followed by `.com`, and with 64 times `a` followed by `.com`
- **THEN** the first is true and the second is false

#### Scenario: Name length at 253 and 254 octets
- **WHEN** `is_hostname` is called with three labels of 63 `a` and one of 61 `a` joined by three dots (253 octets), and with the same name plus one more `a` (254 octets)
- **THEN** the first is true and the second is false

#### Scenario: A host name is ASCII
- **WHEN** `is_hostname` is called with `bücher.example` (U+00FC as UTF-8), with `example` U+FF0E `com` (FULLWIDTH FULL STOP), and with `example.com` followed by a NUL octet
- **THEN** every call is false

### Requirement: Hosts: hostname A-labels judged by idna
`is_hostname` and `hostname_rule` SHALL treat a label that starts with `xn--` in any case as an A-label (RFC 5890 2.3.2.1): the label MUST be lowercased with `boost::urls::grammar::to_lower` into a stack buffer of 63 octets and passed to `webcpp::idna::check_label`, which decodes it and applies IDNA2008 (RFC 5891 5.4), the Bidi rule of the label alone and the canonical-form check, so an A-label is valid in `hostname` exactly when it is valid as a one-label `idn-hostname`, in either case: `XN--NXASMQ6B` is valid in both formats. The label's length (at most 63 octets) and hyphen rules are checked before idna is asked. A label whose Punycode does not decode, whose decoded code point IDNA2008 does not allow, or whose decoded form is not canonical or is all ASCII is invalid.

#### Scenario: A-labels in any case
- **WHEN** `is_hostname` is called with `xn--nxasmq6b`, `XN--NXASMQ6B`, `Xn--NxAsMq6B.example` and `xn--9n2bp8q.xn--9t4b11yi5a`
- **THEN** every call is true

#### Scenario: A-labels that do not decode or are not canonical
- **WHEN** `is_hostname` is called with `xn--X`, `XN--aa---o47jg78q` and `xn--hello-txk`
- **THEN** every call is false

#### Scenario: A-labels IDNA2008 refuses
- **WHEN** `is_hostname` is called with `xn--07jt112bpxg.xn--9t4b11yi5a`, `xn--chb89f`, `xn--07jceefgh4c`, `xn--al-0ea`, `xn--l-fda`, `xn--la-0ea`, `xn--l-gda`, `xn--S-jib3p`, `xn--wva3j`, `xn--5db1e`, `xn--5db3e`, `xn--defabc-k64e`, `xn--vek` and `xn--7a`
- **THEN** every call is false, whatever an IDNA library without IDNA2008's validity might say

#### Scenario: The Bidi rule of the A-label alone
- **WHEN** `is_hostname` is called with `xn--a-0hc` (a then HEBREW LETTER ALEF), `xn--a-zhc` (ALEF then a) and `xn--1-0hc` (1 then ALEF)
- **THEN** every call is false, since a name of one label meets RFC 5893's Bidi rule alone

#### Scenario: A-labels at 63 and 64 octets
- **WHEN** `is_hostname` is called with `xn--` followed by 55 `a` and `-uve` (63 octets), with `XN--` followed by 55 `A` and `-UVE`, with the first followed by `.example`, and with `xn--` followed by 56 `a` and `-qye` (64 octets)
- **THEN** the first three are true and the last is false

### Requirement: Hosts: R-LDH labels are host names
`is_hostname` SHALL accept a label that has hyphens in its third and fourth places without being an A-label (an R-LDH label, RFC 5890 2.3.1, such as `ab--cd`), because RFC 1123 allows it and draft 4 of the JSON Schema suite says so; the same string MUST be refused by `is_idn_hostname`. The two formats differ here by lexical's decision, as they do on the Bidi rule across labels.

#### Scenario: R-LDH in hostname and not in idn-hostname
- **WHEN** `is_hostname("ab--cd.example")` and `is_idn_hostname("ab--cd.example")` are called
- **THEN** the first is true and the second is false

#### Scenario: R-LDH label in an e-mail domain
- **WHEN** `is_email("test@c--n.com")` is called
- **THEN** it is true

### Requirement: Hosts: hostname judges each A-label alone
`is_hostname` SHALL judge each A-label by itself and MUST NOT apply RFC 5893's Bidi rule across the labels of a name (a decision of lexical's own, written down as such in the docs); only `is_idn_hostname` checks the Bidi rule across labels.

#### Scenario: The side-by-side case
- **WHEN** `is_hostname("0a.xn--4db")` and `is_idn_hostname("0a.xn--4db")` are called
- **THEN** the first is true and the second is false, and the U-label form `0a.` followed by HEBREW LETTER ALEF is false in both formats

### Requirement: Hosts: hostname_rule extent and errors
`grammar::hostname_rule` SHALL take the run of letters, digits, hyphens and dots at the start of its input, validate that run whole, and MUST NOT match a shorter prefix of it. On success it advances `it` past the run and yields the run's text. It MUST return `boost::urls::grammar::error::mismatch` when the first byte is not a letter, digit, hyphen or dot; `error::invalid` when the run is no host name (an empty label, a hyphen first or last, a label over 63 or a name over 253 octets); and the `webcpp::idna` error (category `webcpp::idna::error_category()`) when idna refuses an A-label. On every error `it` MUST be left unchanged.

#### Scenario: The rule stops at a character outside the name
- **WHEN** `hostname_rule.parse` runs over `example.com:8080`
- **THEN** it yields `example.com` and `it` stops at the colon
- **AND** `ex.ample_x` yields `ex.ample`

#### Scenario: A refusal leaves the iterator and is invalid
- **WHEN** `hostname_rule.parse` runs over `a..b:80`, `-a.example/` and `a.b-`
- **THEN** each result is `error::invalid` and `it` is where it started, even though a prefix such as `a` would have been a name

#### Scenario: No name at all is a mismatch
- **WHEN** `hostname_rule.parse` runs over `:80`
- **THEN** the result is `error::mismatch`

#### Scenario: idna's reason reaches the caller
- **WHEN** `hostname_rule.parse` runs over `xn--7a.example`
- **THEN** the error's category is `webcpp::idna::error_category()` and `it` is unchanged

### Requirement: Hosts: idn-hostname validator and rule
`<webcpp/lexical/hostname.hpp>` SHALL declare `[[nodiscard]] bool is_idn_hostname(std::string_view) noexcept` for JSON Schema's `idn-hostname` and the rule `grammar::idn_hostname_rule` (type `idn_hostname_rule_t`, `value_type` `grammar::text`), and `is_idn_hostname(s)` MUST equal `parse(s, grammar::idn_hostname_rule).has_value()`. The rule SHALL hand its whole run to `webcpp::idna::to_ascii` with idna's default options (UTS 46's mapping, nontransitional processing, every check, `verify_dns_length`, and IDNA2008's stricter validity: CONTEXTO, NV8 and XV8 refused) into a stack `std::array<char, idna::max_domain_length>`, and the name is valid exactly when `to_ascii` accepts the run whole.

#### Scenario: Validator and rule agree
- **WHEN** `is_idn_hostname` and `parse(..., idn_hostname_rule)` are evaluated over `bücher.example` and over `example.`
- **THEN** both agree on every string, true for the first and false for the second

#### Scenario: Name with a normal script
- **WHEN** `is_idn_hostname` is called with U+C2E4 U+B840 `.` U+D14C U+C2A4 U+D2B8 (Korean labels) and with `xn--nxasmq6b.example`
- **THEN** both are true

### Requirement: Hosts: idn-hostname UTS 46 mapping
`is_idn_hostname` SHALL apply UTS 46's mapping before judging, so that a name is valid when IDNA2008 accepts its mapped form: fullwidth digits U+FF11 U+FF12 U+FF13 map to `123`; ZERO WIDTH SPACE U+200B is ignored (`a` U+200B `b`); a capitalised A-label such as `xn--NXASMQ6B` maps to lowercase; FULLWIDTH `x` and `n` before `--` form an A-label once mapped (U+FF58 U+FF4E `--nxasmq6b`); `e` followed by COMBINING ACUTE ACCENT (`cafe` U+0301 `.com`) is normalised to NFC; and the label separators IDEOGRAPHIC FULL STOP U+3002, FULLWIDTH FULL STOP U+FF0E and HALFWIDTH IDEOGRAPHIC FULL STOP U+FF61 are dots. Plain ASCII names such as `hostname` and `1host` are valid.

#### Scenario: Mapped names are valid
- **WHEN** `is_idn_hostname` is called with U+FF11 U+FF12 U+FF13, with `a` U+200B `b`, with `cafe` U+0301 `.com`, with `a` U+3002 `b`, with `a` U+FF0E `b`, with `a` U+FF61 `b`, and with U+FF58 U+FF4E `--nxasmq6b`
- **THEN** every call is true

#### Scenario: IDNA2008 contexts that hold
- **WHEN** `is_idn_hostname` is called with `l` U+00B7 `l` (MIDDLE DOT between two l), U+03B1 U+0375 U+03B2 (GREEK LOWER NUMERAL SIGN before a Greek letter), U+05D0 U+05F3 U+05D1 (GERESH after a Hebrew letter), U+30FB U+3041 (KATAKANA MIDDLE DOT beside a Hiragana letter) and U+0628 U+0660 U+0628 (ARABIC-INDIC DIGIT ZERO between Arabic letters)
- **THEN** every call is true

### Requirement: Hosts: idn-hostname IDNA2008 refusals
`is_idn_hostname` SHALL refuse every name IDNA2008 refuses, including the rules an IDNA library was measured missing: the DISALLOWED exceptions of RFC 5892 2.6 (ARABIC TATWEEL U+0640 and NKO LAJANYALAN U+07FA; the vertical kana repeat marks U+3031 to U+3035 and U+302E, U+302F, U+303B), CONTEXTO (RFC 5892 A.3 MIDDLE DOT not between two `l`, A.4 GREEK LOWER NUMERAL SIGN before no Greek letter, A.5 and A.6 GERESH and GERSHAYIM after no Hebrew letter, A.9 KATAKANA MIDDLE DOT in a label of no Hiragana, Katakana or Han), CONTEXTJ (ZERO WIDTH JOINER after no virama), the Bidi rule across labels, a Punycode that does not decode, and a decoded A-label whose code point is not allowed (`xn--7a`).

#### Scenario: CONTEXTO violations
- **WHEN** `is_idn_hostname` is called with `a` U+00B7 `l`, U+00B7 `l`, `l` U+00B7 `a`, `l` U+00B7, U+03B1 U+0375 `S`, U+03B1 U+0375, U+05F3 U+05D1, U+05F4 U+05D1, `def` U+30FB `abc` and U+30FB alone
- **THEN** every call is false

#### Scenario: DISALLOWED exceptions
- **WHEN** `is_idn_hostname` is called with U+0640 U+07FA, and with U+3031 U+3032 U+3033 U+3034 U+3035 U+302E U+302F U+303B
- **THEN** both are false

#### Scenario: CONTEXTJ and Punycode failures
- **WHEN** `is_idn_hostname` is called with U+0915 U+200D U+0937 (the joiner after no virama), `xn--example-` and `xn---9uc`
- **THEN** every call is false

#### Scenario: Bidi across labels
- **WHEN** `is_idn_hostname` is called with `0a.` followed by HEBREW LETTER ALEF U+05D0
- **THEN** it is false, with the error `idna::error::bidi`, while `is_hostname` judges the same labels by their A-label forms (see the side-by-side requirement)

#### Scenario: An A-label whose decoded code point is not allowed
- **WHEN** `is_idn_hostname("xn--7a")` is called
- **THEN** it is false

### Requirement: Hosts: idn-hostname malformed names
`is_idn_hostname` SHALL refuse the empty string, `.`, a trailing dot (`example.`) and a trailing IDEOGRAPHIC FULL STOP (`example` U+3002, an empty root label), an empty label (`a..b`), a hyphen first or last (`-hello`, `hello-`), an R-LDH label (`ab--cd.example`), ill-formed UTF-8 (`b` followed by the lone lead byte 0xC3 and `cher.example`), and a NUL octet inside the name.

#### Scenario: Malformed internationalized names
- **WHEN** `is_idn_hostname` is called with the empty string, `.`, `example.`, `example` U+3002, `a..b`, `-hello`, `hello-`, `ab--cd.example`, `b` 0xC3 `cher.example`, and `b` U+00FC `cher` NUL `.example`
- **THEN** every call is false

### Requirement: Hosts: idn-hostname lengths are those of the ASCII form
`is_idn_hostname` SHALL measure the DNS lengths in the name's ASCII form (labels of at most 63 octets, the name at most 253 octets, no trailing dot) and MUST NOT measure the octets as written, because idna's `to_ascii` is asked with `verify_dns_length`.

#### Scenario: A-label at 63 and 64 octets
- **WHEN** `is_idn_hostname` is called with 55 `a` followed by U+00FC (an A-label of 63 octets) and with 56 `a` followed by U+00FC (64 octets), and with 64 times `a`
- **THEN** the first is true and the other two are false

#### Scenario: Name at 253 and 254 octets
- **WHEN** `is_idn_hostname` is called with three labels of 63 `a` and one of 61 `a`, and with the last label of 62 `a`
- **THEN** the first is true and the second is false

#### Scenario: Written length differs from ASCII length
- **WHEN** `is_idn_hostname` is called with a name of labels of 63, 63, 63 and 61 FULLWIDTH LATIN SMALL LETTER A U+FF41 (753 octets written, 253 in the ASCII form)
- **THEN** it is true
- **AND** a name of three ASCII labels of 63 `a`, then U+00FC and 55 `a` (249 octets written, 255 in the ASCII form) is false
- **AND** `a`, one thousand SOFT HYPHEN U+00AD, `b` (2,002 octets written, `ab` in the ASCII form) is true

### Requirement: Hosts: idn_hostname_rule extent and errors
`grammar::idn_hostname_rule` SHALL take the run that starts at `it` and ends at the first ASCII byte that is not a letter, digit, `-` or `.`; every non-ASCII byte belongs to the run, since UTS 46 may map any non-ASCII code point (`detail::scan_idn_run` with `idn_run::domain`). It validates the run whole and MUST NOT match a shorter prefix. It MUST return `error::mismatch` when the run is empty; otherwise the `webcpp::idna` error that `to_ascii` returned (`idna::error::empty_label`, `hyphen_at_start_or_end`, `invalid_utf8`, `contexto`, `bidi`, `label_too_long`, `disallowed`, ...); `it` MUST be unchanged on every error.

#### Scenario: Stops at the first ASCII byte outside the name
- **WHEN** `idn_hostname_rule.parse` runs over `bücher.example:8080` and over `bücher.example/x`
- **THEN** it yields `bücher.example` and `it` stops at the colon or the slash

#### Scenario: Mismatch for no run
- **WHEN** `idn_hostname_rule.parse` runs over `:80` and over the empty string
- **THEN** both results are `error::mismatch`

#### Scenario: idna's reason
- **WHEN** `idn_hostname_rule.parse` runs over `a..b:80`, `-a.example/`, `b` 0xC3 `:80`, `a` U+00B7 `b` and `0a.` U+05D0
- **THEN** the errors are `idna::error::empty_label`, `idna::error::hyphen_at_start_or_end`, `idna::error::invalid_utf8`, `idna::error::contexto` and `idna::error::bidi`, each with `it` unchanged

#### Scenario: A refusal never matches a prefix
- **WHEN** `idn_hostname_rule.parse` runs over `a` U+00B7 `b/x`
- **THEN** it fails and `it` is unchanged, rather than match `a`

### Requirement: Hosts: hostile and oversized IDN input
The IDN rules SHALL work in idna's fixed storage and MUST stop a hostile run at that storage rather than process it: a label of 100,000 octets (100,000 times `a` followed by U+00FC) MUST be refused with `idna::error::label_too_long` by both `idn_hostname_rule` and `idna_label_rule`, with `it` unchanged.

#### Scenario: A 100,000-octet label
- **WHEN** `idn_hostname_rule.parse` and `idna_label_rule.parse` run over 100,000 times `a` followed by U+00FC
- **THEN** both return `idna::error::label_too_long`, `it` is unchanged, and no allocation is made

### Requirement: Hosts: idna_label_rule
`<webcpp/lexical/hostname.hpp>` SHALL declare `grammar::idna_label_rule` (type `idna_label_rule_t`, `value_type` `grammar::text`), a terminal of lexical's own, UTF-8-aware, that asks `webcpp::idna` for its verdict on one internationalized label, so that a grammar joins labels with separators of its own. It SHALL take the run of `idn_hostname_rule` and also stop at `.`, U+3002, U+FF0E and U+FF61, and validate that run whole with `idna::to_ascii` (default options, one label). It MUST return `error::mismatch` when no label starts at `it` (the first byte is a dot or one of the three separators, or the input is empty) and otherwise idna's error, `it` unchanged on every error.

#### Scenario: The label stops at the first dot
- **WHEN** `idna_label_rule.parse` runs over `bücher.example:8080`
- **THEN** it yields `bücher` and `it` stops at the dot

#### Scenario: The three separators end a label
- **WHEN** `idna_label_rule.parse` runs over `a` U+3002 `b`, `a` U+FF0E `b`, `a` U+FF61 `b`, and `example` U+3002 (the separator at the end of the input)
- **THEN** each yields `a` (the last yields `example`)

#### Scenario: No label starts at a separator
- **WHEN** `idna_label_rule.parse` runs over `.example` and over U+3002 `whole`
- **THEN** both results are `error::mismatch`

#### Scenario: idna's error for a label
- **WHEN** `idna_label_rule.parse` runs over `a` U+00B7 `l.example` and over `-abc`
- **THEN** the errors are `idna::error::contexto` and `idna::error::hyphen_at_start_or_end`

### Requirement: Hosts: only three code points map to a dot
`detail::is_label_separator` and `scan_idn_run` SHALL treat as separators exactly U+3002, U+FF0E and U+FF61 (the byte triples E3 80 82, EF BC 8E and EF BD A1), the only code points UTS 46 maps to a sequence holding `.` in idna's tables (Unicode 17.0.0), so that a label the rule accepts is one label. ONE DOT LEADER U+2024 and SMALL FULL STOP U+FE52, which IDNA2003 mapped to a dot, are disallowed. A test MUST convert every code point U+0080 to U+10FFFF (surrogates excluded) between two letters and fail when idna's tables map any other code point to a dot, so a Unicode version bump of idna that adds one fails it.

#### Scenario: Exactly three code points
- **WHEN** every scalar value from U+0080 to U+10FFFF is converted by `idna::to_ascii` as `a` + code point + `b`
- **THEN** exactly three of them produce an ASCII form that contains a dot: U+3002, U+FF0E and U+FF61

#### Scenario: Disallowed dot lookalikes
- **WHEN** `idna_label_rule.parse` runs over `a` U+2024 `b` and over `a` U+FE52 `b`
- **THEN** both return `idna::error::disallowed`

### Requirement: Hosts: labels joined by a grammar are judged alone
`idna_label_rule` SHALL judge each label alone, as `idn_hostname_rule` judges a name of one label; a grammar that joins labels into a name MUST check what spans the name itself, namely RFC 5893's Bidi rule across its labels (section 2) and the name's 253 octets in the ASCII form, or hand the whole name to `idn_hostname_rule`, which checks both.

#### Scenario: Two valid labels and no valid name
- **WHEN** `0a.` followed by U+05D0 is parsed with a `range_rule` of `idna_label_rule` and a dot-then-label rule
- **THEN** two labels are produced
- **AND** parsing the same text with `idn_hostname_rule` fails with `idna::error::bidi`

### Requirement: Hosts: email validator and rule
`<webcpp/lexical/email.hpp>` SHALL declare `[[nodiscard]] bool is_email(std::string_view) noexcept` for JSON Schema's `email` and `grammar::email_rule` (type `email_rule_t`, `value_type` `grammar::text`) for RFC 5321 4.1.2's `Mailbox = Local-part "@" ( Domain / address-literal )`, and `is_email(s)` MUST equal `parse(s, grammar::email_rule).has_value()`. RFC 5322's comments and display names are not part of a Mailbox.

#### Scenario: Validator and rule agree
- **WHEN** `is_email` and `parse(..., email_rule)` are evaluated over `joe.bloggs@example.com`
- **THEN** both are valid

#### Scenario: Comments are not part of a Mailbox
- **WHEN** `is_email("a@b(c).org")` is called
- **THEN** it is false

### Requirement: Hosts: email Dot-string local part
`is_email` SHALL accept as local part a Dot-string, `Atom *("." Atom)` with `Atom = 1*atext` and RFC 5322 3.2.3's `atext` (letters, digits and ``!#$%&'*+-/=?^_`{|}~``), and MUST refuse a leading dot, a trailing dot, two dots in a row, a comma, a tab, DEL, NUL, and any non-ASCII byte (a UTF-8 local part is valid only in `idn-email`).

#### Scenario: Atoms
- **WHEN** `is_email` is called with `joe.bloggs@example.com`, `te~st@example.com`, ``!#$%&'*+-/=?^_`{|}~@example.com`` and `te.s.t@example.com`
- **THEN** every call is true

#### Scenario: Broken Dot-strings
- **WHEN** `is_email` is called with `.test@example.com`, `test.@example.com`, `te..st@example.com`, `a,b@iana.org`, `a` TAB `b@iana.org`, DEL `@iana.org`, `a` U+00E9 `@iana.org` and `a` NUL `@iana.org`
- **THEN** every call is false

### Requirement: Hosts: email Quoted-string local part
`is_email` SHALL accept as local part a Quoted-string, `DQUOTE *QcontentSMTP DQUOTE`, where `qtextSMTP` is `%d32-33 / %d35-91 / %d93-126` (the space and printable ASCII but the double quote and the backslash) and `quoted-pairSMTP` is a backslash followed by `%d32-126`. A quoted string may hold dots, spaces and the at sign. It MUST refuse a string that never closes, a closing quote that is escaped, a control character or a NUL inside it, a backslash followed by a control character or by DEL or by a non-ASCII byte, a non-ASCII byte unescaped (UTF8-non-ascii is `qtextSMTP` only in `idn-email`), and anything between the closing quote and the at sign.

#### Scenario: Valid quoted strings
- **WHEN** `is_email` is called with `"joe bloggs"@example.com`, `"joe..bloggs"@example.com`, `"joe@bloggs"@example.com`, `""@iana.org`, `"\a"@iana.org`, `"\""@iana.org` and `"\\"@iana.org`
- **THEN** every call is true

#### Scenario: Invalid quoted strings
- **WHEN** `is_email` is called with `"test@iana.org`, `test"@iana.org`, `"test"test@iana.org`, `"a` TAB `b"@iana.org`, `"` U+00E9 `"@iana.org`, `"a` NUL `b"@iana.org`, `"a\` TAB `b"@iana.org`, `"a\` DEL `"@iana.org` and `"test\` U+00A9 `"@iana.org`
- **THEN** every call is false

### Requirement: Hosts: email local part length
`is_email` SHALL refuse a local part of more than 64 octets (RFC 5321 4.5.3.1.1), the quotes of a Quoted-string counted.

#### Scenario: 64 and 65 octets
- **WHEN** `is_email` is called with 64 times `a` then `@example.com`, with 65 times `a` then `@example.com`, with `"` 62 `a` `"@example.com` (64 octets) and with `"` 63 `a` `"@example.com` (65 octets)
- **THEN** the results are true, false, true, false

### Requirement: Hosts: email Mailbox length
`is_email` and `is_idn_email` SHALL refuse a Mailbox of more than 254 octets, RFC 5321 4.5.3.1.3's path of 256 octets less its two angle brackets (lexical's decision, recorded as amended 2026-10-09). The check applies to the whole Mailbox, local part, at sign and domain or address literal, and MUST make the rule fail as `error::invalid` with `it` unchanged rather than match a prefix.

#### Scenario: 254 and 255 octets
- **WHEN** `is_email` is called with 64 `a`, `@`, and a domain of two labels of 63 `a` and one of 61 `a` joined by dots (254 octets in all), and with the same string plus one `a`
- **THEN** the first is true and the second is false
- **AND** the same holds for a quoted local part of `"` 62 `a` `"` over that domain

#### Scenario: The longest address literal never exceeds the limit
- **WHEN** `is_email` is called with 64 `a` then `@[IPv6:ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255]` (117 octets)
- **THEN** it is true

#### Scenario: The rule refuses the whole Mailbox
- **WHEN** `email_rule.parse` runs over the 254-octet Mailbox followed by `a>rest`
- **THEN** the result is `error::invalid` and `it` is unchanged

### Requirement: Hosts: email domain is hostname_rule
The domain of `email_rule` SHALL be `hostname_rule`, A-labels included (decision P10 of the original spec): a domain of letters, digits and hyphens, an R-LDH label, a name of digits such as `255.255.255.255` and an A-label idna accepts are valid, while a U-label (`a@é.org`, ASCII only), a leading hyphen (`test@-iana.org`), a trailing dot (`test@iana.org.`), an empty label (`test@iana..com`), a second at sign (`a@b@c.org`), an empty domain (`joe.bloggs@`), an empty local part (`@example.com`), an A-label idna refuses (`test@xn--7a.example`) and a literal not at the start (`test@a[255.255.255.255]`) are invalid. A string with no at sign after the local part (`"joe"example.com`, `joe[127.0.0.1]`) is invalid.

#### Scenario: Valid domains
- **WHEN** `is_email` is called with `test@255.255.255.255`, `test@io`, `test@c--n.com` and `test@xn--nxasmq6b.example`
- **THEN** every call is true

#### Scenario: Invalid domains and shapes
- **WHEN** `is_email` is called with `test@-iana.org`, `test@iana.org.`, `test@iana..com`, `a@b@c.org`, `a@` U+00E9 `.org`, `joe.bloggs@`, `@example.com`, `test@xn--7a.example`, `test@iana.org` followed by a newline, `"joe"example.com` and `joe[127.0.0.1]`
- **THEN** every call is false

### Requirement: Hosts: email address literals
`is_email` SHALL accept RFC 5321 4.1.3's address literal as the domain: `[` IPv4 `]` or `[IPv6:` IPv6 `]`, the tag `IPv6` in any case (`ipv6`, `IPV6`). RFC 5321's General-address-literal needs a tag registered with IANA and `IPv6` is the only one, so `[RFC-5322-domain-literal]` and `[IPv7:::1]` are invalid. An empty literal, a literal of fewer or more than four IPv4 parts, an IPv4 part above 255, and a literal that never closes are invalid.

#### Scenario: Valid literals
- **WHEN** `is_email` is called with `joe.bloggs@[127.0.0.1]`, `joe.bloggs@[IPv6:::1]`, `a@[ipv6:::1]` and `a@[IPV6:2001:db8::1]`, and with `"joe bloggs"@[IPv6:2001:db8::1]`
- **THEN** every call is true

#### Scenario: Invalid literals
- **WHEN** `is_email` is called with `joe.bloggs@[127.0.0.300]`, `test@[RFC-5322-domain-literal]`, `a@[]`, `a@[1.2.3]`, `a@[1.2.3.4.5]`, `test@[1.2.3.4` and `a@[IPv7:::1]`
- **THEN** every call is false

### Requirement: Hosts: address literals use Boost.URL's addresses
The address of an address literal SHALL be judged by Boost.URL's `boost::urls::ipv4_address_rule` and `ipv6_address_rule` (RFC 3986's IPv4 `dec-octet` and RFC 4291's IPv6 text forms, without a zone id), not by RFC 5321 4.1.3's own `Snum` and `IPv6-comp`; this is a decision of lexical's, and the two verdicts it changes are the known differences. The literal is a composition of `tuple_rule`, `variant_rule`, `delim_rule` and `squelch` over those rules, the `IPv6:` tag matched by lexical's case-insensitive literal rule.

#### Scenario: Leading zero in an IPv4 part
- **WHEN** `is_email("a@[001.2.3.4]")` is called
- **THEN** it is false, although RFC 5321's `Snum` (1*3DIGIT) allows it

#### Scenario: `::` standing for one group
- **WHEN** `is_email` is called with `a@[IPv6:1:2:3:4:5:6::8]` and `a@[IPv6:1:2:3:4:5:6:7::]`
- **THEN** both are true, although RFC 5321's `IPv6-comp` refuses them and RFC 4291 allows them

### Requirement: Hosts: email_rule extent, errors and iterator
`grammar::email_rule` SHALL stop where its domain or its address literal does (it matches `a@b.example` in `a@b.example>rest` and in `a@b.example,c@d.example`, and the NUL in `a@iana.org` NUL is input left over, so `is_email` of that string is false while the rule yields `a@iana.org`). It MUST return `error::mismatch` when no local part starts at `it` (for example `@example.com`); `error::invalid` when the rest breaks the grammar (an unclosed or unterminated quoted string, a missing at sign, an empty domain or a character no domain holds as in `a@>`, a broken or untagged literal, a local part over 64 octets, a Mailbox over 254 octets); and the `webcpp::idna` error for an A-label the domain's `hostname_rule` refused. The domain's `mismatch` MUST be turned into `invalid`, and `it` MUST be unchanged on every error.

#### Scenario: Stopping inside a larger input
- **WHEN** `email_rule.parse` runs over `a@b.example>rest`
- **THEN** it yields `a@b.example`

#### Scenario: Invalid, not mismatch
- **WHEN** `email_rule.parse` runs over `a@[1.2.3`, `a@[IPv7:::1]`, `test"@iana.org`, `joe.bloggs@` and `a@>`
- **THEN** each result is `error::invalid` and `it` is unchanged

#### Scenario: Mismatch without a local part
- **WHEN** `email_rule.parse` runs over `@example.com`
- **THEN** the result is `error::mismatch`

#### Scenario: Unclosed quoted local part
- **WHEN** the local-part rule runs over `"`, `"abc` and `"a\"` (the closing quote escaped)
- **THEN** each result is `error::invalid` and `it` is unchanged
- **AND** it yields `"a b"` over `"a b"@`

### Requirement: Hosts: email reports idna's reason
`email_rule` and `idn_email_rule` SHALL dispatch on `[` themselves, asking the address-literal rule when the domain starts with `[` and otherwise the host rule (`hostname_rule` or `idn_hostname_rule`) directly, and MUST NOT make the domain an alternative of Boost.URL's `variant_rule`, which answers `mismatch` when every alternative fails and would discard idna's reason; so the `idna::error` that refused the domain reaches the caller, with `it` unchanged.

#### Scenario: A-label refused
- **WHEN** `parse("test@xn--7a.example", email_rule)` is evaluated
- **THEN** the error's category is `webcpp::idna::error_category()`

#### Scenario: IDN domain refused
- **WHEN** `idn_email_rule.parse` runs over U+03B4 `@a` U+00B7 `b.example>rest`
- **THEN** the error is `idna::error::contexto` and `it` is unchanged

### Requirement: Hosts: idn-email validator and rule
`<webcpp/lexical/email.hpp>` SHALL declare `[[nodiscard]] bool is_idn_email(std::string_view) noexcept` for JSON Schema's `idn-email` and `grammar::idn_email_rule` (type `idn_email_rule_t`) for RFC 6531 3.3's internationalized Mailbox, and `is_idn_email(s)` MUST equal `parse(s, grammar::idn_email_rule).has_value()`. It is `email_rule` with UTF-8 in the local part and `idn_hostname_rule` as the domain, or an address literal.

#### Scenario: Validator and rule agree
- **WHEN** `is_idn_email` is called with ASCII `joe.bloggs@example.com` and with `a@[IPv6:::1]`
- **THEN** both are true, as in `is_email`

#### Scenario: Korean mailbox
- **WHEN** `is_idn_email` is called with U+C2E4 U+B840 `@` U+C2E4 U+B840 `.` U+D14C U+C2A4 U+D2B8
- **THEN** it is true

### Requirement: Hosts: idn-email local part
In `is_idn_email` and `idn_email_rule`, `atext` and `qtextSMTP` SHALL also hold any well-formed non-ASCII UTF-8 sequence (RFC 6532 3.1: C1 controls and noncharacters included; ill-formed UTF-8 is refused by the library-wide strict decoder: truncated, overlong, surrogate, above U+10FFFF, lone continuation byte), `quoted-pairSMTP` MUST stay ASCII, the local part is at most 64 octets as written, and it is not normalized (a non-NFC local part is valid, as the suite says; the domain is normalized). A FULLWIDTH COMMERCIAL AT U+FF20 is no at sign.

#### Scenario: Non-ASCII local parts
- **WHEN** `is_idn_email` is called with U+03B4 U+03BF U+03BA U+03B9 U+03BC U+03AE `@example.com` (a Dot-string), the same in a Quoted-string, U+0085 `@example.com` (a C1 control), U+FFFF `@example.com` (a noncharacter), U+1D54F `@example.com` (beyond the BMP), `cafe` U+0301 `@example.com`, and `user@cafe` U+0301 `.com`
- **THEN** every call is true

#### Scenario: Refusals
- **WHEN** `is_idn_email` is called with `user` U+FF20 `example.com`, `a` 0xC3 `@example.com`, `"a\` U+00E9 `"@example.com`, `"a\` 0xC3 `"@example.com`, `"a\` 0xA9 `"@example.com` and `2962`
- **THEN** every call is false

#### Scenario: 64 octets in UTF-8
- **WHEN** `is_idn_email` is called with 32 times U+00E9 then `@example.com` and with the same local part followed by `a`
- **THEN** the first is true and the second is false

### Requirement: Hosts: idn-email domain
The domain of `idn_email_rule` SHALL be `idn_hostname_rule`, whose error (idna's) the rule returns as it is, or an address literal. A hyphen first, a trailing dot, a leading dot, an empty label, an empty domain, a MIDDLE DOT not between two `l` (CONTEXTO) and an R-LDH label (`a@ab--cd.example`) are invalid; a U-label such as `a@é.org` is valid. The domain also fits DNS's lengths in its ASCII form, as `is_idn_hostname` asks.

#### Scenario: Invalid domains
- **WHEN** `is_idn_email` is called with U+03B4 `@-example.com`, U+03B4 `@example.com.`, U+03B4 `@.example.com`, U+03B4 `@example..com`, U+03B4 `@`, `a@b` U+00B7 `c.example` and `a@ab--cd.example`
- **THEN** every call is false

#### Scenario: Valid U-label domain
- **WHEN** `is_idn_email("a@é.org")` is called
- **THEN** it is true

### Requirement: Hosts: idn-email lengths are measured as written
`is_idn_email` SHALL measure the 64 octets of the local part and the 254 octets of the whole Mailbox as written, in UTF-8, the form an SMTPUTF8 envelope carries (RFC 6531 extends RFC 5321's Mailbox without changing its limits, which RFC 5321 counts in octets), whatever the domain's ASCII form measures; the domain's own DNS lengths (63 per label, 253 per name) stay those of its ASCII form.

#### Scenario: Written form shorter than ASCII form
- **WHEN** `is_idn_email` is called with a local part of 32 times U+00E9 (64 octets), `@`, twelve labels of U+00FC (`ü.`, ASCII form `xn--tda`), then labels of 63 `a`, 63 `a` and 25 `a` (254 octets written, 249 octets in the domain's ASCII form, 314 for the whole Mailbox in ASCII)
- **THEN** it is true, and the same Mailbox plus one `a` is false, and the domain alone plus one `a` is a valid `idn-hostname`

#### Scenario: Fullwidth domain
- **WHEN** `is_idn_email` is called with 64 `a`, `@`, 62 times U+FF41 and `.aa` (254 octets written, 130 in the ASCII form), and with the same plus one `a`
- **THEN** the first is true and the second is false

#### Scenario: The rule refuses the whole Mailbox
- **WHEN** `idn_email_rule.parse` runs over the 254-octet Mailbox followed by `a>rest`
- **THEN** the result is `error::invalid` and `it` is unchanged

### Requirement: Hosts: idn_email_rule extent
`grammar::idn_email_rule` SHALL stop where its domain does: the `idn_hostname_rule` run ends at the first ASCII byte outside letters, digits, `-` and `.`. It MUST match `δ@bücher.example` in `δ@bücher.example x` and in `δ@bücher.example>rest`, and MUST return `error::mismatch` when no local part starts at `it`, `error::invalid` for the other grammar failures, and idna's error for a refused domain, as `email_rule` does.

#### Scenario: Stopping at a space
- **WHEN** `idn_email_rule.parse` runs over U+03B4 `@b` U+00FC `cher.example x`
- **THEN** it yields U+03B4 `@b` U+00FC `cher.example`

#### Scenario: Composed with an ASCII Mailbox
- **WHEN** `a@b.example;"c d"@[127.0.0.1]` is parsed with a `tuple_rule` of `email_rule`, `squelch(delim_rule(';'))` and `idn_email_rule`
- **THEN** the parse is valid

### Requirement: Hosts: use of webcpp::idna
The host rules SHALL depend on `webcpp::idna` through its public interface only (`<webcpp/idna/options.hpp>`, `uts46.hpp`, `validity.hpp`, `errors.hpp`) and MUST NOT include `webcpp/idna/detail` (the include-boundary check enforces it); a verdict of idna's that a lexical test finds wrong becomes a test in idna, red first, and lexical never works around it. The only idna calls are `idna::check_label` (a hostname's A-label, one lowercased label at a time) and `idna::to_ascii` with its default options (the IDN rules); the constants `idna::max_label_length` = 63 and `idna::max_domain_length` = 253 size the buffers. The Unicode data is idna's, Unicode 17.0.0 today, and the JSON Schema suite's known-failure lists for hostname, idn-hostname, email and idn-email are empty (a difference of Unicode versions would be the only allowed reason).

#### Scenario: No detail include
- **WHEN** the include-boundary check reads `hostname.hpp`, `email.hpp` and `detail/idn_scan.hpp`
- **THEN** none includes a header under `webcpp/idna/detail`

#### Scenario: The suite's host and mail formats pass
- **WHEN** the JSON Schema Test Suite's `hostname`, `idn-hostname`, `email` and `idn-email` string tests of every dialect run against `is_hostname`, `is_idn_hostname`, `is_email` and `is_idn_email`
- **THEN** every test passes with no entry in a known-failure list

### Requirement: Hosts: allocation-free host and mail validators
Every host and e-mail validator and rule (`is_hostname`, `is_idn_hostname`, `is_email`, `is_idn_email`, `hostname_rule`, `idn_hostname_rule`, `idna_label_rule`, `email_rule`, `idn_email_rule`) SHALL be `noexcept` and MUST NOT allocate: idna works in fixed storage (an `idna::max_label_length` buffer for an A-label, an `idna::max_domain_length` buffer for `to_ascii`), and a `range_rule` keeps its rule inline. The allocation test counts the global `operator new` around these calls.

#### Scenario: Hostile host names allocate nothing
- **WHEN** `is_hostname` and `is_email` are called with a name of over 100,000 octets (labels of 63 `a` joined by dots, ending in `com`) and `a@` plus that name, alongside `xn--9n2bp8q.xn--9t4b11yi5a`, `xn--07jceefgh4c` and `"joe bloggs"@[IPv6:2001:db8::1]`
- **THEN** the long ones are false, the valid ones true, and the global allocation count is unchanged

#### Scenario: IDN names allocate nothing
- **WHEN** `is_idn_hostname` and `is_idn_email` are called with a valid 253-octet IDN name, with 100,000 `a` followed by U+00FC, with a valid Mailbox over a long IDN domain and with a Mailbox whose domain is refused after labels were converted, and `idna_label_rule` runs over 55 `a` and U+00FC
- **THEN** the verdicts are as expected and the allocation count is unchanged

### Requirement: Hosts: where the host rules stop inside a longer input
Each host rule SHALL document and keep its stopping point, which is part of its contract: `hostname_rule` stops at the first character outside letters, digits, `-` and `.`; `idn_hostname_rule` at the first ASCII character outside letters, digits, `-` and `.`, every non-ASCII byte belonging to the name; `idna_label_rule` as `idn_hostname_rule` and also at `.`, U+3002, U+FF0E and U+FF61; `email_rule` and `idn_email_rule` where the domain or the address literal does. Each validates the text it took and fails as a whole rather than match a shorter prefix, and a rule that fails leaves its input where it was.

#### Scenario: Rules in one grammar
- **WHEN** `a.example,b` U+00FC `cher.example,c.example` is parsed with a `range_rule` of `idn_hostname_rule` (passed through `grammar::ref`) and a comma-then-host rule
- **THEN** three names are produced
- **AND** `a.example b.example` with a `range_rule` of `hostname_rule` and a space-then-host rule gives two

### Requirement: Hosts: composing with Boost.URL combinators
The host rules SHALL be Boost.URL rules usable inside `tuple_rule`, `variant_rule`, `optional_rule` and `range_rule`, yielding `grammar::text`, so that a program writes its own grammar: an internationalized host and an optional port is `urlg::tuple_rule(fg::idn_hostname_rule, urlg::optional_rule(urlg::tuple_rule(urlg::squelch(urlg::delim_rule(':')), urlg::unsigned_rule<std::uint16_t>{})))`, and a rule that refuses says whose it was through its error's category (`webcpp.idna` for idna's, `boost.url.grammar` for Boost.URL's). The example `example/lexical_grammar.cpp` prints exactly four lines for four inputs.

#### Scenario: Host and port
- **WHEN** the host-and-port grammar parses `bücher.example:8080`, `example.com`, `a·b.example:80` and `example.com:99999`
- **THEN** the output is `bücher.example:8080 -> host bücher.example, port 8080`, `example.com -> host example.com`, `a·b.example:80 -> refused by webcpp.idna` and `example.com:99999 -> refused by boost.url.grammar`

#### Scenario: A rule inside the combinators
- **WHEN** `bücher.example:8080` is parsed with that grammar in a test
- **THEN** the first element is `bücher.example` and the second holds 8080

#### Scenario: Optional and tuple composition
- **WHEN** `../a b` is parsed with `tuple_rule(optional_rule(hostname_rule), iri_reference_rule, squelch(delim_rule(' ')), hostname_rule)` and `/a/b` with `tuple_rule(json_pointer_rule, optional_rule(idna_label_rule))`
- **THEN** both parses are valid

### Requirement: Hosts: Boost.URL facts that shape composition
A grammar composed of the host rules MUST account for two facts of Boost.URL that lexical's docs state. A `range_rule` over a rule larger than 128 bytes keeps it in a pool of Boost.URL's own, which allocates, so such a rule is passed through `boost::urls::grammar::ref` as lexical does with its own nested label rules (`range_rule(ref(ldh_label_rule), ref(dotted_label_rule), 1)`); and `parse` of a whole string fails with `error::leftover` when the rule stops early, which is how a validator refuses trailing input. A grammar that needs idna's reason MUST NOT put an IDN rule as an alternative of `variant_rule`, which answers `mismatch` when every alternative fails.

#### Scenario: A range of hosts without allocation
- **WHEN** a `range_rule(ref(idn_hostname_rule), ref(next_idn_host), 1)` parses a comma-separated list
- **THEN** it parses without Boost.URL's pool allocating

#### Scenario: Leftover
- **WHEN** `parse("example.com ", hostname_rule)` is evaluated
- **THEN** the error is `error::leftover`

### Requirement: Hosts: hostname internals
`hostname.hpp` SHALL build `hostname_rule` from named detail pieces so each is testable in isolation: `detail::has_ace_prefix(label)` (`xn--` in any case, at least 4 octets); `ldh_label_rule` (`token_rule(ldh_chars)`, at least one letter, digit or hyphen); `dotted_label_rule` (a squelched dot and a label); `labels_rule` (a `range_rule` of the two by reference, minimum 1); `is_ldh_label(label)` (1 to 63 octets, no hyphen first or last); `check_a_label(label)` (length over 63 is `error::invalid`, otherwise lowercase and `idna::check_label`); and `check_hostname(name)` (the name over `idna::max_domain_length` is `error::invalid`, then the labels, a leading, trailing or double dot leaving input over, each label by `is_ldh_label` and each A-label by `check_a_label`). `detail::parse_idn_run(it, end, kind)` SHALL be shared by `idn_hostname_rule` and `idna_label_rule`.

#### Scenario: ACE prefix in any case
- **WHEN** `has_ace_prefix` is called with `xn--a`, `XN--a`, `Xn--` and `xn-a`
- **THEN** the first three are true and the last is false

#### Scenario: Shared run parser
- **WHEN** `idn_hostname_rule.parse` and `idna_label_rule.parse` run over `bücher.example`
- **THEN** the first yields `bücher.example` and the second `bücher`, both through `parse_idn_run`

### Requirement: Regex: is_regex and its header
`<webcpp/lexical/regex.hpp>` SHALL declare, in namespace `webcpp::lexical`, `[[nodiscard]] inline bool is_regex(std::string_view pattern) noexcept`, JSON Schema's `regex` format: it answers whether `pattern`, any byte string, is an ECMA-262 `Pattern[+UnicodeMode]` (a pattern read as `new RegExp(pattern, "u")` reads it), with ECMAScript 2025's modifiers and duplicate group names, early errors included. It SHALL take no `std::pmr::memory_resource` and no `error_code`: the answer is a `bool`, a value, for every byte string. The header SHALL also declare `inline constexpr std::size_t max_regex_length = 65536` and `inline constexpr std::size_t max_regex_nesting = 32`, and include only `<webcpp/lexical/config.hpp>`, `<webcpp/lexical/detail/regex_validator.hpp>`, `<cstddef>` and `<string_view>`. The version is 0.1.0.

#### Scenario: The quick-start program
- **WHEN** the example `lexical_regex.cpp` judges `^\p{Letter}+$`, `(?<year>\d{4})-(?<month>\d{2})`, `(?<=\$\d*)[0-9]`, `^(abc]`, `\Aabc\z`, `(?i)abc` and `(?i:abc)`
- **THEN** it prints `valid`, `valid`, `valid`, `invalid`, `invalid`, `invalid` and `valid` for them, in that order, each line written as `<pattern> -> <verdict>`

#### Scenario: Any byte string is answered
- **WHEN** `is_regex` is called with the empty string, a string holding a NUL, or ill-formed UTF-8
- **THEN** it returns `true`, `true` and `false` respectively, and does not throw

### Requirement: Regex: the header is outside the convenience header
`<webcpp/lexical.hpp>` SHALL NOT include `<webcpp/lexical/regex.hpp>`, so that a program that never validates a pattern does not compile the Unicode tables of ECMA-262's classes. `meta/include-boundaries.json` SHALL state that no header of lexical but `include/webcpp/lexical/regex.hpp` and `include/webcpp/lexical/detail/regex_*.hpp` includes `webcpp/lexical/regex` or `webcpp/lexical/detail/regex_`, and that no header of lexical includes `boost/regex`, `webcpp/regex` or `srell`: `is_regex` is a syntax validator of lexical's own, lexical runs no regular expression and depends on no regex library. `build.jam` SHALL name no regex target for it.

#### Scenario: The boundary file forbids a regex engine
- **WHEN** `meta/include-boundaries.json` is read
- **THEN** it holds a boundary whose `must-not-include` is `["boost/regex", "webcpp/regex", "srell"]` for every header of lexical, and a boundary with the `except` list `regex.hpp` and `detail/regex_*.hpp` whose `must-not-include` is `["webcpp/lexical/regex", "webcpp/lexical/detail/regex_"]`

#### Scenario: The superseded engine is gone
- **WHEN** the original specification's design of `is_regex` over `webcpp::regex` and SRELL is compared with the code
- **THEN** it is superseded: `is_regex` compiles nothing, takes no resource, and `regex.hpp` includes no header of another webcpp library

### Requirement: Regex: what a valid pattern is
`is_regex` SHALL answer `true` exactly when ECMA-262 accepts the pattern as a `Pattern` in `u` mode, early errors included, which is when `new RegExp(pattern, "u")` does not throw a SyntaxError, except for the bounds of the requirements "Regex: pattern length bound" and "Regex: group nesting bound", which answer `false` for patterns V8 accepts. The validator SHALL read the pattern as that grammar reads it and refuse what it refuses, Annex B's legacy syntax included: `\a`, `\-` outside a class, `\8`, an octal escape, Perl's `\A`, `\Z` and `\z`, a group of another dialect (`(?P<n>...)`, `(?#...)`, a leading `(?i)`), a property ECMA-262 does not name (`\p{letter}`, `\p{Script=Foo}`), a backreference to a group the pattern does not hold, and two groups of one name that a match could both hold.

#### Scenario: Valid means syntax and early errors
- **WHEN** `is_regex` is called with `a{3,1}`, `\1`, `\u{110000}` and `(a`
- **THEN** each answer is `false`: a numbers-out-of-order quantifier, a backreference to no group, a code point past U+10FFFF and an unterminated group are early errors or syntax errors in `u` mode

#### Scenario: Dialects of other engines
- **WHEN** `is_regex` is called with `(?P<n>a)`, `(?#comment)`, `(?i)`, `\Aabc\z` and `abc\Z`
- **THEN** each answer is `false`

### Requirement: Regex: what is not judged
`is_regex` SHALL compile no pattern and run none. It SHALL say nothing of how long a matcher takes on a valid pattern, nor of the limits an engine sets on the program it builds from one, nor of the flags of a `RegExp` other than `u`: the `v` flag's class set syntax is not read, so `[a&&b]` is judged as a class of the three characters, a valid `u`-mode class. A lookbehind of any width SHALL be valid, because the validator never measures one.

#### Scenario: Lookbehind of any width
- **WHEN** `is_regex` is called with `(?<=\$\d*)x`, `(?<=a+)b`, `(?<=a|bc)d` and `(?<=a*)d`
- **THEN** each answer is `true`

#### Scenario: A class that v mode would read otherwise
- **WHEN** `is_regex` is called with `[a&&b]`
- **THEN** the answer is `true`

### Requirement: Regex: the verdict of the reader
`detail::check_regex(std::string_view pattern, std::size_t max_nesting)` in `<webcpp/lexical/detail/regex_validator.hpp>` SHALL be `constexpr` and `noexcept` and return a `detail::regex_check { regex_verdict verdict; std::string_view why; }`. `regex_verdict` SHALL be a `std::uint8_t` enum class with `valid` (an ECMA-262 pattern in `u` mode), `invalid` (a SyntaxError in JavaScript) and `beyond_bounds` (nested deeper than the validator reads, and not judged). `why` SHALL be a string literal naming the first fault found and SHALL be empty exactly when the verdict is `valid`; once a fault is recorded, later faults SHALL NOT replace it. `is_regex` SHALL be `check_regex(pattern, max_regex_nesting).verdict == regex_verdict::valid` for a pattern of at most `max_regex_length` bytes, so `invalid` and `beyond_bounds` both answer `false`.

#### Scenario: Verdicts at a nesting bound
- **WHEN** `check_regex` is called with `((a))` and `max_nesting` 1, then 2, and with `(a)` and `max_nesting` 0
- **THEN** the verdicts are `beyond_bounds`, `valid` and `beyond_bounds`, and `why` is empty only for the `valid` one

#### Scenario: Invalid verdicts name a reason
- **WHEN** `check_regex` is called with `a)`, `(a`, `\1`, `a{3,1}`, `\u{110000}` and a lone byte `\303`
- **THEN** each verdict is `invalid` with a non-empty `why`

#### Scenario: Evaluated at compile time
- **WHEN** `static_assert(check_regex("(?<n>a)\\k<n>", max_regex_nesting).verdict == regex_verdict::valid)` and the same for `a{3,1}` with `regex_verdict::invalid` are compiled
- **THEN** they compile, and `noexcept(is_regex(std::string_view{}))` and `noexcept(check_regex(std::string_view{}, 0))` hold

### Requirement: Regex: well-formed UTF-8 and NUL
`is_regex` SHALL read a pattern as UTF-8 under RFC 3629 and answer `false` for a pattern that is not well-formed, before reading its grammar: a truncated sequence, an overlong one, a surrogate (`ED A0 80`), a code point above U+10FFFF (`F4 90 80 80`), a lone continuation byte, or a byte `C0`, `C1` or `F5` to `FF`. The decoder is lexical's own, `detail::decode_utf8` in `<webcpp/lexical/detail/utf8.hpp>`, which leaves `i` and `cp` unchanged on failure. Every code point of a well-formed pattern that no rule gives a meaning SHALL be a literal, a NUL (`U+0000`) included, and a code point above U+FFFF SHALL be one atom, not two.

#### Scenario: Ill-formed UTF-8 is refused
- **WHEN** `is_regex` is called with `caf\303` (a truncated sequence), `\377`, `\200`, `a\300\257`, `\340\200\257`, `\355\240\200` and `\364\220\200\200`
- **THEN** each answer is `false`

#### Scenario: Well-formed non-ASCII text and NUL
- **WHEN** `is_regex` is called with `caf\303\251`, `\360\237\220\262+`, `[\360\237\220\262-\360\237\220\263]`, the three bytes `a`, NUL, `b`, and the five bytes `[`, NUL, `-a]`
- **THEN** each answer is `true`

### Requirement: Regex: alternatives and empty patterns
The validator SHALL accept a Disjunction of Alternatives separated by `|`, each of which may be empty, and an empty pattern. A `)` with no open group SHALL be refused, as SHALL a group with no `)`.

#### Scenario: Empty parts
- **WHEN** `is_regex` is called with the empty pattern, `|`, `||`, `a|`, `|a`, `a||b`, `(|)`, `(?:)`, `()`, `a|(b|c)|d` and `(?:|a)*`
- **THEN** each answer is `true`

#### Scenario: Unbalanced and bare quantifiers
- **WHEN** `is_regex` is called with `a)|b`, `(a|b`, `|*` and `a|+`
- **THEN** each answer is `false`; the first reason is `an unmatched )` for `a)|b`

### Requirement: Regex: assertions
The validator SHALL accept the assertions `^`, `$`, `\b`, `\B` and the lookarounds, and SHALL refuse a quantifier after any assertion in `u` mode (Annex B's quantifiable lookahead does not exist there).

#### Scenario: Assertions are valid
- **WHEN** `is_regex` is called with `^`, `$`, `^$`, `\b`, `\B`, `^\b\w+\B$`, `a^b$c`, `(^)($)` and `^a|b$`
- **THEN** each answer is `true`

#### Scenario: A quantified assertion
- **WHEN** `is_regex` is called with `^*`, `$+`, `\b*`, `\B{2}`, `^{1}`, `$?` and `a^?`
- **THEN** each answer is `false`, the reason being `a quantifier after an assertion`

### Requirement: Regex: quantifiers and braces
The validator SHALL accept the quantifiers `*`, `+`, `?`, `{n}`, `{n,}` and `{n,m}`, each optionally followed by one `?` for laziness, and SHALL refuse in `u` mode: a `{` that does not open a complete quantifier (`a{`, `a{}`, `a{x}`, `a{1,2`, `a{1 }`, `a{,5}`), a `{` or `}` that is a literal (`{`, `}`, `a}`), a quantifier with nothing to repeat (`{1}`, `a|{1}`), a quantifier directly after another (`a**`, `a+*`, `a???`, `a{1}{2}`, `a++`), and `{n,m}` with `m` below `n`. The literal braces SHALL be valid only when escaped. Decimal numbers SHALL be read at any length and compared as numbers; a value past 2^32 - 1 SHALL be read as 2^32 - 1.

#### Scenario: Quantifiers
- **WHEN** `is_regex` is called with `a*`, `a+`, `a?`, `a{0}`, `a{3}`, `a{2,}`, `a{2,3}`, `a{0,0}`, `a*?`, `a??`, `a{2,3}?`, `(ab)+` and `a{1}b{2,}c*`
- **THEN** each answer is `true`

#### Scenario: Braces that are not quantifiers
- **WHEN** `is_regex` is called with `a{,5}`, `a{2,1}`, `a{3,1}`, `a{`, `a{}`, `a{x}`, `a{1,2`, `a{1 }`, `{`, `}`, `a}`, `{1}` and `a\{1}`
- **THEN** each answer is `false`

#### Scenario: Escaped braces
- **WHEN** `is_regex` is called with `\{`, `\}` and `a\{1\}`
- **THEN** each answer is `true`

#### Scenario: Doubled quantifiers
- **WHEN** `is_regex` is called with `a**`, `a+*`, `a???`, `a{1}{2}` and `a++`
- **THEN** each answer is `false`, the reason being `a quantifier with nothing to repeat`

### Requirement: Regex: groups
The validator SHALL accept a capturing group `( ... )`, a non-capturing group `(?: ... )` and a named group `(?<name> ... )`, and SHALL refuse a group with no `)`, a `)` with no group, `(?` followed by anything that is no group, lookaround, named group or modifier group (`(?x)`, `(?P<n>a)`, `(?#comment)`), and a named group whose name or `>` is missing. A group MAY be quantified.

#### Scenario: Groups
- **WHEN** `is_regex` is called with `(a)`, `(?:a)`, `((a))`, `(a)(b)(c)`, `(?<n>a)`, `(?<$>a)` and `(?<_a1>x)`
- **THEN** each answer is `true`

#### Scenario: Malformed groups
- **WHEN** `is_regex` is called with `(a`, `a)`, `(?:a`, `(?`, `(?x)`, `(?<n>`, `(?<>a)`, `(?<a>x`, `(?P<n>a)` and `(?#comment)`
- **THEN** each answer is `false`

### Requirement: Regex: group names
A group name (`RegExpIdentifierName`) SHALL start with `$`, `_` or a code point of ID_Start and continue with `$`, U+200C, U+200D or a code point of ID_Continue, where ID_Start and ID_Continue are the generated tables. A `\u` escape SHALL be allowed in a name, as `\uXXXX` or `\u{X...}`, and SHALL stand for the code point it names; an escaped surrogate pair `𝒜` SHALL be joined into the one code point it makes (U+1D49C). The name SHALL be tested after the escape is decoded, so `(?<0a>x)` is refused (U+0030 is `0`, no ID_Start). A name that is empty, unterminated, malformed or made of a character no identifier holds in that place SHALL be refused. Non-ASCII UTF-8 SHALL be allowed in a name.

#### Scenario: Names that are valid
- **WHEN** `is_regex` is called with `(?<\u{61}>x)`, `(?<\303\261>x)` (n with tilde), `(?<a𝒜>x)` and `(?<$>a)`
- **THEN** each answer is `true`

#### Scenario: Names that are not
- **WHEN** `is_regex` is called with `(?<1>a)`, `(?<0a>x)`, `(?<a-b>x)` and `(?<a b>x)`
- **THEN** each answer is `false`

#### Scenario: A name written two ways is one name
- **WHEN** `is_regex` is called with `(?<ab>x)\k<ab>`
- **THEN** the answer is `true`, since the escape decodes to `b`

### Requirement: Regex: duplicate group names
Following ECMA-262 2025 (22.2.1.1), two groups MAY share a name only when no match can hold both, which is when some disjunction holds them in different alternatives; otherwise the second group SHALL be refused, whatever its position, as a duplicate group name. The validator SHALL check each named group against the last earlier group of the same name, comparing the disjunctions that hold them from the outermost in: the first disjunction both are inside whose alternatives differ allows the pair, and the pair is refused when no such disjunction exists. A group inside a lookaround SHALL be judged in the disjunctions it sits in like any other.

#### Scenario: Duplicates in different alternatives
- **WHEN** `is_regex` is called with `(?<n>a)|(?<n>b)`, `(?:(?<n>a)|(?<n>b))\k<n>`, `(?:(?<n>a)|(?<n>b))|(?<n>c)`, `(?<=(?<n>a)|(?<n>b))c` and `(?<n>a)|(?<=(?<n>b))`
- **THEN** each answer is `true`

#### Scenario: Duplicates that both can match
- **WHEN** `is_regex` is called with `(?<n>a)(?<n>b)`, `((?<n>a)|b)(?<n>c)`, `(?<n>a)|(?:(?<n>b)|c)(?<n>d)` and `(?<n>(?<n>a))`
- **THEN** each answer is `false`, the reason being `a duplicate group name`

### Requirement: Regex: modifiers
The validator SHALL accept ECMAScript 2025's modifier groups `(?ims-ims:...)`: the letters `i`, `m` and `s`, each at most once in the whole list on either side of the single `-`, at least one letter in the list, and the `:` that opens the group; a modifier group MAY nest, and its group SHALL count for the nesting bound. It SHALL refuse a lone `(?-:a)`, a repeated letter (`(?ii:a)`, `(?i-i:a)`), a second `-` (`(?i-s-m:a)`), an unknown letter (`(?x:a)`), and a bare `(?i)` or `(?i` with no `:`, since `(?i)` is another dialect's syntax. The flag state is not used by the grammar: only the syntax of the list matters.

#### Scenario: Valid modifier groups
- **WHEN** `is_regex` is called with `(?i:a)`, `(?-i:a)`, `(?i-:a)`, `(?ims-:a)`, `(?i-m:a)`, `(?m:^a$)`, `(?s:.)` and `(?i:(?-i:a))`
- **THEN** each answer is `true`

#### Scenario: Invalid modifier groups
- **WHEN** `is_regex` is called with `(?-:a)`, `(?ii:a)`, `(?i-i:a)`, `(?i-s-m:a)`, `(?x:a)`, `(?i)`, `(?i` and `(?i:a`
- **THEN** each answer is `false`

### Requirement: Regex: lookarounds
The validator SHALL accept `(?=...)`, `(?!...)`, `(?<=...)` and `(?<!...)` with any disjunction inside, including an empty one, and a backreference inside. A lookaround SHALL count for the nesting bound, and SHALL NOT be quantified. A lookbehind SHALL be distinguished from a named group by the character after `(?<`: `=` or `!` makes it a lookbehind, anything else a named group; a lookaround SHALL NOT count as a capturing group.

#### Scenario: Lookarounds
- **WHEN** `is_regex` is called with `(?=a)`, `(?!a)`, `(?<=a)`, `(?<!a)`, `a(?=b)c`, `(?=(a))\1`, `(?=)`, `(?<=)`, `(?<!)`, `(?<=(a))\1` and `(?<=\1(a))b`
- **THEN** each answer is `true`

#### Scenario: Quantified or unterminated lookarounds
- **WHEN** `is_regex` is called with `(?=a)*`, `(?!a)+`, `(?<=a)?`, `(?<!a){2}`, `(?=a`, `(?<=a` and `(?<!a`
- **THEN** each answer is `false`

### Requirement: Regex: character escapes
In `u` mode the validator SHALL accept the class escapes `\d \D \w \W \s \S`, the control escapes `\t \n \v \f \r`, `\0` not followed by a decimal digit, `\cX` for an ASCII letter `X`, `\xHH`, the `\uXXXX` and `\u{...}` forms of the requirement "Regex: unicode escapes", `\p{...}` and `\P{...}`, and an identity escape of exactly the syntax characters `^ $ \ . * + ? ( ) [ ] { } |` and `/`. It SHALL refuse every other escape, which is an Annex B escape that `u` mode does not have: `\x` with fewer than two hexadecimal digits, `\c` with no letter or with a digit or `_`, `\00`, `\01` and any other octal escape, `\8` and `\9` (when no such group exists), a trailing backslash, and an identity escape of any other character (`\-`, `\a`, `\q`, `\_`, `\ `, `\'`, `\"`, `\:`, `\@`, `\#`, `\<`, `\N`, `\h`, `\e`, `\G`, `\K`, `\i`). The Perl escapes `\A`, `\Z` and `\z` SHALL be refused, but the escaped backslash before them SHALL leave the `A` a literal: `\\A` is valid and `\\\A` is not.

#### Scenario: Escapes that are valid
- **WHEN** `is_regex` is called with `\d\D\w\W\s\S`, `\t\n\v\f\r\0`, `\cA\cz`, `\x41\x4a\xFF` and `\^\$\\\.\*\+\?\(\)\[\]\{\}\|\/`
- **THEN** each answer is `true`

#### Scenario: Escapes that u mode refuses
- **WHEN** `is_regex` is called with `\x4`, `\xG1`, `\c`, `\c1`, `\c_`, `\00`, `\01`, `\8`, `\9`, `a\`, `\-`, `\a`, `\q` and `\N`
- **THEN** each answer is `false`

#### Scenario: Perl's anchors
- **WHEN** `is_regex` is called with `\A`, `abc\Z`, `abc\z`, `\\A` and `\\\A`
- **THEN** the answers are `false`, `false`, `false`, `true` and `false`

### Requirement: Regex: unicode escapes
The validator SHALL accept `\uXXXX` with exactly four hexadecimal digits, `\u{X...}` with one or more hexadecimal digits and a value of at most 0x10FFFF, and a `\uD800`-`\uDBFF` immediately followed by a `\uDC00`-`\uDFFF` as one code point. A lone surrogate escape SHALL be valid. It SHALL refuse `\u` with fewer than four digits, `\u{}`, an unterminated `\u{41`, and a value above 0x10FFFF.

#### Scenario: Valid unicode escapes
- **WHEN** `is_regex` is called with `Aé`, `😀`, `\uD83D` and `\u{41}\u{1F600}`
- **THEN** each answer is `true`

#### Scenario: Malformed unicode escapes
- **WHEN** `is_regex` is called with `\u004`, `\u`, `\u{}`, `\u{41` and `\u{110000}`
- **THEN** each answer is `false`

### Requirement: Regex: backreferences
The validator SHALL accept `\N` (`N` from 1, any number of digits) only when the pattern holds at least `N` capturing groups, counted over the whole pattern, those after the reference included, so that a forward reference such as `\1(a)` is valid; lookarounds and non-capturing groups SHALL NOT be counted, and named groups SHALL be. It SHALL accept `\k<name>` only when some group of the pattern, before or after it, has that name (names compared after their escapes are decoded), and SHALL refuse a `\k` that is not followed by `<name>`: in `u` mode `\k` is never a literal. A backreference SHALL NOT be allowed inside a class.

#### Scenario: Numbered backreferences
- **WHEN** `is_regex` is called with `(a)\1`, `\1(a)`, `(a)|\1` and `(a)(b)(c)(d)(e)(f)(g)(h)(i)(j)\10`
- **THEN** each answer is `true`

#### Scenario: Backreferences to no group
- **WHEN** `is_regex` is called with `\1`, `(?:a)\1`, `(a)\2` and `(a)\10`
- **THEN** each answer is `false`, the reason being `a backreference to a group that does not exist`

#### Scenario: Named backreferences
- **WHEN** `is_regex` is called with `\k<n>(?<n>a)`, `(?<n>a\k<n>)`, `(?<a>x)\k<a>(?<b>y)\k<b>`, `\k<m>(?<n>a)`, `\k<n>`, `\k`, `\k<>`, `\k<n`, `(a)\k<a>` and `(?<n>a)\k`
- **THEN** the first three answers are `true` and the others `false`

### Requirement: Regex: character classes
The validator SHALL accept `[...]` and `[^...]`, empty ones included (`[]` matches nothing, `[^]` anything), whose items are single characters, class escapes and ranges `a-b`. A `-` at the start or end of a class or beside a range SHALL be a literal. Inside a class `\b` SHALL be the backspace (U+0008), `\-` SHALL be an escaped hyphen, `\0` a NUL, and `[`, `(`, `)`, `{`, `}` and `|` literals. A class SHALL refuse a range whose first end is greater than its last (`[z-a]`), a range with a class escape at either end (`[\d-z]`, `[a-\d]`, `[\w-.]`, `[\D-z]`, `[\p{L}-z]`), `\B`, `\k`, a numbered backreference, `\c` with no letter, `\00`, an unknown escape (`\a`, `\z`) and a missing `]`; an unescaped `]` outside a class SHALL be refused, and a `]` after `[` closes the class (`[]a]` is an empty class followed by a lone `]`). Range ends written as escapes SHALL be compared by value (`[a-b]`, `[\x61-b]`, `[\cA-\cZ]`, `[\b-z]`).

#### Scenario: Valid classes
- **WHEN** `is_regex` is called with `[a]`, `[^a]`, `[]`, `[^]`, `[a-z0-9_]`, `[-a]`, `[a-]`, `[a-a]`, `[a-b]`, `[\b-z]`, `[\b\-\]\\]`, `[\d\D\s\S\w\W]`, `[[]` and `[(){}|]`
- **THEN** each answer is `true`

#### Scenario: Invalid classes
- **WHEN** `is_regex` is called with `[]a]`, `[^]a]`, `[`, `[a`, `[\]`, `]`, `[z-a]`, `[b-a]`, `[\d-z]`, `[a-\d]`, `[\w-.]`, `[\B]`, `[\k]`, `[\1]`, `[\c]`, `[\00]` and `[\a]`
- **THEN** each answer is `false`

#### Scenario: Class escapes in a class
- **WHEN** `is_regex` is called with `[\0]`, `[\-]` and `[\^\]\\]`
- **THEN** each answer is `true`

### Requirement: Regex: unicode property escapes
The validator SHALL accept `\p{...}` and `\P{...}` in `u` mode, in or out of a class, whose body, at most 64 bytes, is one of: a General_Category value by its name or alias (`L`, `Lu`, `Letter`, `Uppercase_Letter`, `LC`, `Cased_Letter`, `cntrl`, `digit`, `punct`); a binary property ECMA-262 lists by its name or alias, or one of ECMA-262's `ASCII`, `Any`, `Assigned`; `gc=` or `General_Category=` and a General_Category value; or `sc=`, `Script=`, `scx=` or `Script_Extensions=` and a script by its name or alias. It SHALL refuse everything else: a script or a long name with no `=` (`\p{Greek}`), a General_Category value after `Script=` (`\p{Script=L}`), a script after `gc=`, a non-script after `sc=`, a binary property with a value (`\p{ID_Start=Yes}`), a property that ECMA-262 does not list (`\p{Script_Extensions}` alone, `\p{letter}`), and a name that is not matched case-sensitively and exactly, with no whitespace (`\p{lu}`, `\p{ L}`, `\p{L }`), an empty body (`\p{}`), a missing `{` or `}` (`\pL`, `\p`, `\p{L`), a non-ASCII body (`\p{` U+00E9 `}`) and a body of 200 characters. A name SHALL be one of the generated sorted tables of the requirement "Regex: unicode tables". `\p{Script=Sidetic}` and `\p{sc=Tai_Yo}` (Unicode 17 scripts) SHALL be valid.

#### Scenario: Property forms
- **WHEN** `is_regex` is called with `\p{L}\p{Lu}\p{gc=Lu}\p{General_Category=Uppercase_Letter}`, `\p{Script=Greek}\p{sc=Grek}\p{scx=Grek}\p{Script_Extensions=Greek}`, `\p{ASCII}\p{Any}\p{Assigned}\p{ID_Start}\p{IDS}\p{Lower}\p{Emoji}`, `\p{digit}\p{cntrl}\p{punct}` and `\P{L}[\p{L}\d][^\p{L}a]`
- **THEN** each answer is `true`

#### Scenario: Properties that ECMA-262 does not name
- **WHEN** `is_regex` is called with `\p{Script=L}`, `\p{General_Category=Greek}`, `\p{sc=Foo}`, `\p{Greek}`, `\p{Lu=Lu}`, `\p{ID_Start=Yes}`, `\p{Script_Extensions}`, `\p{lu}`, `\p{ L}`, `\p{}`, `\p{L`, `\pL`, `\P{}` and `\p{letter}`
- **THEN** each answer is `false`

#### Scenario: Overlong and non-ASCII names
- **WHEN** `is_regex` is called with `\p{` followed by 200 letters `L` and `}`, and with `\p{` U+00E9 `}`
- **THEN** both answers are `false`

### Requirement: Regex: pattern length bound
`is_regex` SHALL answer `false` for a pattern longer than `max_regex_length` (65,536 bytes) without reading it: the validator rereads parts of a pattern (to count groups, to find the group a `\k` names and to compare duplicate names), so a bound on the length bounds the time. A pattern of exactly `max_regex_length` bytes SHALL be judged. The bound is the library's, and a pattern past it may be valid for V8.

#### Scenario: The longest pattern is judged
- **WHEN** `is_regex` is called with 65,536 letters `a`, and with 65,537
- **THEN** the answers are `true` and `false`

#### Scenario: Long patterns of other shapes
- **WHEN** `is_regex` is called with `\p{L}` repeated 13,107 times, and with `[` followed by `a-z` repeated 21,844 times and `]`
- **THEN** both answers are `true`

### Requirement: Regex: group nesting bound
`is_regex` SHALL answer `false` without judging a pattern whose groups, lookarounds and modifier groups nest deeper than `max_regex_nesting` (32), because the validator reads a group by calling itself, from `disjunction` through `group` and back, and the nesting bounds its stack. A pattern nested exactly 32 deep SHALL be judged. Capturing, non-capturing and named groups, the four lookarounds and modifier groups SHALL all count one level each; alternatives, quantifiers, backreferences and classes SHALL NOT count. `check_regex` SHALL answer `beyond_bounds` for such a pattern, with the reason `groups nested deeper than the bound`.

#### Scenario: Nesting at and past the bound
- **WHEN** `is_regex` is called with `a` wrapped in 32 and then in 33 levels of each of `(`...`)`, `(?:`...`)*`, `(?:`...`)?`, `(?=`...`)`, `(?<=`...`)`, `(?<!`...`)`, `(?i:`...`)` and `(?:a|`...`)`
- **THEN** each pattern of 32 levels is valid, and each of 33 is answered `false`

#### Scenario: Ten thousand levels
- **WHEN** `is_regex` is called with 10,000 `(`, an `a` and 10,000 `)`, and with 10,000 `(?:`, an `a` and 10,000 `)*`
- **THEN** both answers are `false`, without a crash, and without recursion past the bound

#### Scenario: Wide is not deep
- **WHEN** `is_regex` is called with `(a)` repeated 10,000 times, `a` followed by `|a` 5,000 times, `a*` 5,000 times, `(a)` followed by `\1` 5,000 times, and `[` followed by `\D` 5,000 times and `]`
- **THEN** every answer is `true`

### Requirement: Regex: the reader's own nesting ceiling
`detail::regex_validator` SHALL clamp the `max_nesting` it is given to `nesting_limit`, 64, whatever the caller asks, because the functions that find the disjunctions of a group keep their stack of places in arrays of `nesting_limit + 1` entries. `max_regex_nesting` (32) SHALL stay at or below that ceiling.

#### Scenario: The ceiling holds
- **WHEN** `check_regex` is called with a pattern of `a` wrapped in 65 levels of `(`...`)` and `max_nesting` 1000
- **THEN** the verdict is `beyond_bounds`

### Requirement: Regex: bounds are exactly two
`is_regex` SHALL have two bounds, `max_regex_length` and `max_regex_nesting`, and no bound on the number of alternatives, quantifiers, groups or classes. Both are `inline constexpr std::size_t`. Alternatives, groups side by side, repeated backreferences and a wide class SHALL be judged up to `max_regex_length`.

#### Scenario: No bound on alternatives
- **WHEN** `is_regex` is called with `a` followed by `|a` repeated 5,000 times
- **THEN** the answer is `true`

#### Scenario: Constants
- **WHEN** `max_regex_length` and `max_regex_nesting` are read
- **THEN** they are 65536 and 32, and `static_assert(max_regex_nesting >= 32)` holds in `regex_test`; `AGENTS.md`'s mention of a `max_regex_alternatives` does not describe a constant of the code, which has none

### Requirement: Regex: bounded time on hostile patterns
The validator SHALL do work bounded by a polynomial of the pattern's length, itself bounded by `max_regex_length`: it reads the pattern once, and for each group counted, each `\k` and each named group it rereads the pattern from its start with the same single-token reader (`next_token`, which treats an escape and a whole class as one token). It SHALL NOT backtrack on the pattern's own structure. `is_regex` SHALL answer a hostile megabyte without a crash.

#### Scenario: A megabyte
- **WHEN** `is_regex` is called with one million letters `a`
- **THEN** the answer is `false`, because the pattern is past `max_regex_length`

### Requirement: Regex: no allocation and no exceptions
`is_regex` SHALL allocate nothing, on any target, on valid, invalid and hostile patterns, through neither the global allocator nor a resource: it takes no `std::pmr::memory_resource`, works on the pattern's own bytes and keeps its state on the stack, to the depth `max_regex_nesting` allows. It SHALL NOT throw, SHALL be `noexcept`, and SHALL NOT depend on `WEBCPP_LEXICAL_NO_EXCEPTIONS`. `test/allocation_test.cpp` SHALL count the global allocations around `is_regex` on valid, invalid and hostile samples and require zero.

#### Scenario: The allocation test samples
- **WHEN** `allocation_test` calls `is_regex` on `a` (valid), `^\p{Letter}+$` (valid), `(?<y>\d{4})-(?<m>\d{2})|\p{Script=Greek}+` (valid), `(?<=a|bc)d` (valid), `[\D\S]` (valid), `(?i:k)(?<n>a)|(?<n>b)\k<n>` (valid), `^(abc]`, `a{3,1}`, `\k<m>(?<n>a)` and `\p{Greek}` (invalid), 32 and 33 nested groups, a pattern of `max_regex_length` letters, one byte longer, and a hostile nesting
- **THEN** each answer is the one expected and the global allocation count has not changed

#### Scenario: A megabyte answered without allocation
- **WHEN** `is_regex` is called with 2^20 letters `a` among the validators of the megabyte test
- **THEN** it answers `false`, and only the validators that were given a resource allocated

### Requirement: Regex: no rule
Lexical SHALL NOT provide a Boost.URL rule for `regex` (no `regex_rule`): a pattern has no grammar of its own inside a longer input, so `is_regex` judges a whole string. The invariant that for every format `is_x(s)` and `parse(s, x_rule)` agree SHALL hold for every format but `regex`.

#### Scenario: No rule is declared
- **WHEN** `<webcpp/lexical/regex.hpp>` is read
- **THEN** it declares `is_regex`, `max_regex_length` and `max_regex_nesting` and no grammar rule

### Requirement: Regex: always answers, never fails
`is_regex` SHALL answer every byte string with a value and never throw; a pattern past a bound SHALL be answered `false` before it can take more stack or time than the bounds allow (invariant 6 of `doc/guarantees.adoc`). `is_regex` SHALL be the one format that has no `grammar::result` and no Boost.URL error code: a refusal is `false`, and the reason, kept in `detail::regex_check::why`, is for tests and diagnostics.

#### Scenario: Reasons are kept for diagnostics
- **WHEN** `check_regex` is called with `a)`
- **THEN** `why` is `an unmatched )`

### Requirement: Regex: unicode tables
`include/webcpp/lexical/detail/regex_tables.hpp` SHALL be generated, in `webcpp::lexical::detail::regex_tables`, from the Unicode 17.0.0 data files, with a header that says it is generated and not to be edited, and SHALL hold: `inline constexpr std::string_view unicode_version = "17.0.0"`; `general_category_names` (80 sorted names: every name and alias of the 30 General_Category values, `LC`, `Cased_Letter`, and the `cntrl`, `digit` and `punct` aliases); `binary_names` (99 sorted names and aliases of the 53 binary properties ECMA-262 lists, plus `ASCII`, `Any` and `Assigned`); `script_names` (344 sorted names and aliases of the scripts some code point has, `Unknown`/`Zzzz` included); and `id_start` (684) and `id_continue` (799), sorted ranges of the code points of ID_Start and ID_Continue, each packed as `(first << 21) | last` in a `std::uint64_t`. The script `Katakana_Or_Hiragana`, no code point of which has it, SHALL have no name, as ECMA-262's table of the values of Script leaves it out.

#### Scenario: The tables answer
- **WHEN** `regex_test` reads the tables
- **THEN** `unicode_version` is `17.0.0`; `general_category_names` has `Lu` and `Uppercase_Letter` and not `Greek`; `script_names` has `Greek` and `Grek` and not `Lu`; `binary_names` has `Emoji` and not `Letter`; `id_start` holds `a` and not `1`; `id_continue` holds `1`

#### Scenario: The generated names read through the validator
- **WHEN** `is_regex` is called with `\p{Script=Katakana_Or_Hiragana}`
- **THEN** the answer is `false`, since the script has no name in `script_names`

### Requirement: Regex: reading the tables
`detail/regex_unicode.hpp` SHALL be the only header that reads `regex_tables.hpp`, and SHALL provide: `has_name(std::span<const std::string_view> names, std::string_view name)`, a binary search of a sorted table; `in_ranges(std::span<const std::uint64_t> ranges, char32_t cp)`, true when `cp` is in one of the packed ranges, found by an upper bound on `(cp << 21) | 0x1FFFFF`; `is_name_start(char32_t)`, true for `$`, `_` or a code point of ID_Start; and `is_name_part(char32_t)`, true for `$`, U+200C, U+200D or a code point of ID_Continue. All SHALL be `constexpr` and `noexcept`.

#### Scenario: Name characters
- **WHEN** `is_name_start` and `is_name_part` are called with `$`, `1`, U+200C and `-`
- **THEN** `is_name_start('$')` and `is_name_part('$')` and `is_name_part(U'‌')` hold, and `is_name_start('1')` and `is_name_part('-')` do not

### Requirement: Regex: the table generator
`tools/generate_tables.py` SHALL write `regex_tables.hpp` from the Unicode files pinned in `tools/unicode.json` (`ucd/DerivedCoreProperties.txt`, `ucd/PropertyAliases.txt`, `ucd/PropertyValueAliases.txt`, `ucd/ScriptExtensions.txt` and `ucd/Scripts.txt`, each by its `https://www.unicode.org/Public/17.0.0/` URL and SHA-256): a file missing from the cache directory (default `bin/unicode/<version>/` of the superproject, which idna's generator fills too) SHALL be downloaded, checked before it is kept, and every file hashed, and a digest that differs SHALL stop the generator naming the file and both digests before anything is written. Before writing, the generator SHALL check: each of ECMA-262's binary properties but `ASCII`, `Any` and `Assigned` is a property of `PropertyAliases.txt`; every script a file names is one `PropertyValueAliases.txt` names; every name is ASCII and names one value of its kind; ID_Start and ID_Continue each hold a code point. It SHALL write nothing unless every check passed, write through a sibling `.partial` file renamed over the target, and write the same bytes on two runs (no date, sorted data, `\n` endings). It exits 0 when written, 1 naming a fault, 2 on a usage error. `Unknown`, the value of the unassigned code points, which `Scripts.txt` does not list, SHALL be named although no file names it.

#### Scenario: A pinned digest differs
- **WHEN** a cached Unicode file does not hash to the SHA-256 in `tools/unicode.json`
- **THEN** the generator exits 1 naming the file and both digests, and writes no output

#### Scenario: Two runs write the same bytes
- **WHEN** the generator runs twice on the same cache
- **THEN** the two outputs are byte for byte equal

### Requirement: Regex: the generated patterns
`tools/generate_patterns.py` SHALL write `test/regex/cases/patterns.json`, a JSON array of distinct patterns, sorted, one a line, every character outside printable ASCII escaped (24,297 patterns today), of four kinds: patterns drawn at random from a grammar of atoms, groups of every kind, backreferences, assertions, quantifiers and, in 30% of them, one of 41 defects, with Python's `random.Random` seeds 2 to 9, 3,000 patterns each (each followed by four subjects that are drawn and dropped, so that the patterns are those of the study of Boost.Regex as an ECMA-262 engine); every name `\p{...}` accepts under each of its forms and the same names misspelt, read from the Unicode files `tools/unicode.json` pins; every `pattern`, every key of a `patternProperties` and every string that a test of `"format": "regex"` holds in the JSON-Schema-Test-Suite (the submodule `third_party/libs/JSON-Schema-Test-Suite`, which SHALL be checked out, else the generator exits 1 saying how); and a list of edge cases the grammar does not draw: ECMAScript 2025's modifiers and duplicate group names, lookbehinds of every width, escapes of every kind, classes and quantifiers at their edges, other dialects' groups. Two runs SHALL write the same bytes. Exit status is 0, 1 on a fault and 2 on a usage error.

#### Scenario: Patterns are sorted and distinct
- **WHEN** `patterns.json` is read
- **THEN** it is a JSON array of 24,297 distinct strings in sorted order, all of its bytes printable ASCII

#### Scenario: Missing suite
- **WHEN** the generator runs without the JSON-Schema-Test-Suite submodule checked out
- **THEN** it exits 1 with a message naming the directory and `git submodule update --init third_party/libs/JSON-Schema-Test-Suite`

### Requirement: Regex: the V8 oracle
`test/oracle/v8.mjs` SHALL be run by the pinned Node, 26.7.0 (V8 14.6, Unicode 17.0), as `node v8.mjs <cases directory> <output directory>`: for each pattern of `patterns.json` it SHALL evaluate `new RegExp(pattern, 'u')` and write `verdicts.json`, a JSON array with one integer a line, `1` when the constructor does not throw and `0` when it throws a `SyntaxError`, in the order of `patterns.json`; any other error SHALL be rethrown, and a missing argument SHALL exit with status 2. `test/oracle/Jamfile` SHALL declare the lane `oracle` (`webcpp.cases v8 : v8.mjs : ../regex/cases : ../regex/expected`), which needs no npm package and compares the oracle's output with the committed `test/regex/expected/verdicts.json` (24,297 verdicts, 16,475 of them `1`). `update-expected` SHALL be the only writer of `verdicts.json`, with the CI's Node: a Node whose V8 or Unicode differs may write other verdicts, which is a change of its own.

#### Scenario: Check the committed verdicts
- **WHEN** `b2 -a libs/lexical/test/oracle//oracle` runs
- **THEN** V8's verdicts for every pattern equal the committed `verdicts.json`

#### Scenario: Regenerate the verdicts
- **WHEN** `b2 libs/lexical/test/oracle//update-expected` runs after the patterns changed
- **THEN** `verdicts.json` is rewritten with one verdict a line, in the order of `patterns.json`

### Requirement: Regex: is_regex agrees with V8
`test/regex_v8_test.cpp` SHALL, on every target (native, wasip2, wasip3, emscripten), read `cases/patterns.json` and `expected/verdicts.json` from the directory `WEBCPP_TEST_LEXICAL_REGEX` names (set by `test/Jamfile`, the files reaching a WASI program through `webcpp.data`), require both to be JSON arrays of the same size with string patterns and integer verdicts, and require `is_regex(pattern) == (verdict == 1)` for every pattern. It SHALL print at most the first 20 differences, each with its index, V8's answer and the pattern, count all of them, and print `<n> patterns, <v> valid in V8, <d> verdicts of is_regex not V8's`. The count of differences SHALL be zero: on each of the 24,297 patterns (16,475 valid by V8) the verdict of `is_regex` is V8's. The grammar of the validator SHALL NOT be changed without running `regex_v8` on every target.

#### Scenario: Zero differences
- **WHEN** `b2 -a libs/lexical/test//regex_v8` runs
- **THEN** it prints `24297 patterns, 16475 valid in V8, 0 verdicts of is_regex not V8's` and passes

#### Scenario: A missing file
- **WHEN** a case file cannot be opened or is not a JSON array
- **THEN** the test reports the path on the standard error and fails

### Requirement: Regex: the JSON Schema suite's patterns
Every `pattern` and `patternProperties` key that the JSON-Schema-Test-Suite holds, and every string its tests of the format `regex` hold, SHALL be a pattern of the V8 comparison, and the verdict of `is_regex` on each SHALL be V8's. The suite's tests that need a regular expression to match are `json_schema`'s: lexical does not search.

#### Scenario: The suite's patterns are valid
- **WHEN** the patterns of the suite's schemas are given to `is_regex`
- **THEN** each is valid, as it is for V8

### Requirement: Regex: unit test cases of regex_test
`test/regex_test.cpp` SHALL cover, by function: alternatives and empty patterns, assertions, quantifiers and braces, groups and names, duplicate group names, modifiers, lookarounds, escapes, backreferences, classes, Unicode properties, UTF-8 and NUL, the tables, the verdicts, and hostile patterns (nested up to `max_regex_nesting` and refused past it, wide and long ones judged up to `max_regex_length`). A failure SHALL print the pattern on the standard error.

#### Scenario: The verdict helper checks the reason
- **WHEN** `regex_test` calls its `judged(pattern, verdict, max_nesting)` helper
- **THEN** it requires the verdict and requires that `why` be empty exactly when the verdict is `valid`

### Requirement: Regex: stack of the validator on WebAssembly
The deepest pattern that `is_regex` accepts, at `max_regex_nesting` levels, SHALL fit the 64 KiB stack of wasm32-wasip2 at `-O0`, and the figure measured for the previous implementation (about 20 KiB, 2026-10-12, wasi-sdk 34) does not hold for this validator and SHALL be measured again; a change of a bound or of the validator's recursion SHALL be measured the same way. The hostile patterns of `regex_test` SHALL run on every target.

#### Scenario: Hostile patterns on wasip2
- **WHEN** `regex_test` runs on wasm32-wasip2
- **THEN** the patterns of 32 and 33 nested groups, and 10,000 nested groups, are answered `true`, `false` and `false` with no stack overflow

### Requirement: Regex: the generated-file lane
The own lane `generated` (`test/Jamfile`: `webcpp.generated tables : ../tools/generate_tables.py : : ../include/webcpp/lexical/detail/regex_tables.hpp` and `webcpp.generated patterns : ../tools/generate_patterns.py : : regex/cases/patterns.json`, `webcpp.lane generated : generated-tables generated-patterns`) SHALL run both generators again into the build directory and compare each output with its committed file, byte for byte; `update-generated` SHALL write them in place and SHALL be the one writer of the generated files. The ordinary lanes SHALL NOT run the generators, since a generator may need the network. `regex_tables.hpp`, `patterns.json` and `verdicts.json` SHALL NOT be edited by hand.

#### Scenario: A drifted table
- **WHEN** `regex_tables.hpp` differs from the generator's output
- **THEN** `b2 -a libs/lexical/test//generated` fails

#### Scenario: Refreshing
- **WHEN** `b2 libs/lexical/test//update-generated` runs
- **THEN** both generated files are rewritten in place

### Requirement: Regex: the Unicode data notice
The Unicode License v3 notice (copyright 1991-2026 Unicode, Inc.) SHALL appear in the appendix "Unicode data" of lexical's page (`doc/unicode-data.adoc`, id `lexical-unicode-data`), which says that the tables of lexical's regular expressions, `regex_tables.hpp`, and the patterns `test/regex/cases/patterns.json` are derived from the Unicode Character Database, version `{unicode-version}`, and that lexical's code is under the Boost Software License 1.0 alone. The generated header SHALL point to the appendix in its own header comment.

#### Scenario: The header points to the notice
- **WHEN** `regex_tables.hpp` is read
- **THEN** its header comment says `Unicode License v3: its notice is in the appendix "Unicode data" of lexical's page.`

### Requirement: Regex: documentation of the page
`doc/regex.adoc` (section id `lexical-regex`) SHALL document `is_regex` with the example `example/lexical_regex.cpp` included from its `program` tag and its output `example/lexical_regex.expected`, and subsections `lexical-regex-how` (how a pattern is judged: `new RegExp(pattern, "u")` does not throw; Annex B, Perl's escapes, other dialects' groups, properties, backreferences, duplicate names; ill-formed UTF-8 refused, NUL a literal; valid means syntax and early errors only), `lexical-regex-bounds` (a table of `max_regex_length` 65,536 and `max_regex_nesting` 32 and what each bounds), `lexical-regex-allocation` (nothing, on any target), `lexical-regex-v8` (the comparison with V8, the seeds, the Node, the commands) and `lexical-regex-tables` (the Unicode tables and their generators, the `generated` lane and its commands). The page SHALL say that `<webcpp/lexical.hpp>` leaves `regex.hpp` out, that `is_regex` has no rule, and that a lookbehind of any width is allowed. The example SHALL run on every target and be compared with its expected output.

#### Scenario: The example output matches
- **WHEN** the example `lexical_regex` runs
- **THEN** its output equals `lexical_regex.expected` line for line

### Requirement: Regex: changelog entry
The changelog's unreleased section (`doc/changelog.adoc`, tag `unreleased`) SHALL state, for this feature, that ECMA-262 patterns in `u` mode are checked by an allocation-free syntax validator with Unicode 17 tables: "Added ECMA-262 patterns in u mode, checked by an allocation-free syntax validator, with Unicode 17 tables."

#### Scenario: The line is present
- **WHEN** `doc/changelog.adoc` is read
- **THEN** its unreleased section holds that sentence as a bullet
