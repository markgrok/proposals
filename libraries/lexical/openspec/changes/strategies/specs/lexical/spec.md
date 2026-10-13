## ADDED Requirements

### Requirement: Strategies: one entry point for every format
The library SHALL declare, in `<webcpp/lexical/is.hpp>`, `template <class Format, class Strategy = typename Format::default_strategy> requires strategy_for<Strategy, Format> constexpr bool is(std::string_view s, Strategy const& strategy = Strategy{})`, whose answer is `std::invoke(strategy, s)` and whose `noexcept` is `std::is_nothrow_invocable_v<Strategy const&, std::string_view>`. Every one of the 19 formats MUST be reachable through it, so that every check of the library has this one signature.

#### Scenario: Every format through the same call
- **WHEN** a program calls `lexical::is<lexical::email>(s)`, `lexical::is<lexical::time>(s)` and `lexical::is<lexical::regex>(s)`
- **THEN** each compiles with the same shape and answers what the format's default strategy answers

#### Scenario: A table of formats without wrappers
- **WHEN** a program stores `[](std::string_view s) { return lexical::is<lexical::date_time>(s); }` and the same for other formats in one `std::array` of `bool (*)(std::string_view)`
- **THEN** it compiles, and each entry answers its format

### Requirement: Strategies: the strategy concept
`strategy_for<Strategy, Format>` SHALL hold when `Strategy const&` is a `std::predicate` on `std::string_view` and, if `Strategy` declares a member type `format`, that type is `Format`. A strategy webcpp ships MUST declare `using format`, so that it serves its own format alone.

#### Scenario: A strategy of another format is refused
- **WHEN** a program calls `lexical::is<lexical::email>(s, lexical::time::rfc3339{})`
- **THEN** it fails to compile, the diagnostic naming the unsatisfied `strategy_for`

#### Scenario: A developer's lambda is a strategy of any format
- **WHEN** a program calls `lexical::is<lexical::uuid>(s, [](std::string_view text) { return text.size() == 36; })`
- **THEN** it compiles, and the answer is the lambda's

#### Scenario: A callable that takes no string is refused
- **WHEN** a program calls `lexical::is<lexical::uuid>(s, [](int) { return true; })`
- **THEN** it fails to compile

### Requirement: Strategies: format tags and their default strategies
Each format SHALL be an empty struct in `webcpp::lexical` that nests its strategies and names `default_strategy`: `date_time::rfc3339`, `date::rfc3339`, `time::rfc3339`, `duration::rfc3339`, `email::rfc5321`, `idn_email::rfc6531`, `hostname::rfc1123`, `idn_hostname::idna2008`, `ipv4::rfc2673`, `ipv6::rfc4291`, `uri::rfc3986`, `uri_reference::rfc3986`, `iri::rfc3987`, `iri_reference::rfc3987`, `uri_template::rfc6570`, `json_pointer::rfc6901`, `relative_json_pointer::draft`, `uuid::rfc4122` and `regex::ecma262` (names subject to open question O4). Every default strategy MUST be an aggregate whose configuration is set by designated initializers.

#### Scenario: The default answers as 0.1.0 did
- **WHEN** `is<F>(s)` and 0.1.0's `is_f(s)` are evaluated for every string of the unit tests and of the JSON-Schema-Test-Suite's format tests
- **THEN** they agree on every string, for every format

### Requirement: Strategies: configuration moves into strategies
The configuration of 0.1.0's overloads SHALL be members of the strategies: `time::rfc3339::leap` and `date_time::rfc3339::leap` (`leap_seconds`, default `allowed`), `relative_json_pointer::draft::manipulation` (`index_manipulation`, default `allowed`), `iri::rfc3987::resource` and `iri_reference::rfc3987::resource` (`std::pmr::memory_resource*`, the default resource when null), and `regex::ecma262::max_length` (65,536) and `max_nesting` (32).

#### Scenario: A leap second refused for JSON Schema v1
- **WHEN** a program calls `is<time>("23:59:60Z", time::rfc3339{.leap = leap_seconds::refused})`
- **THEN** the answer is `false`, while `is<time>("23:59:60Z")` answers `true`

#### Scenario: An IRI from the program's arena
- **WHEN** a program calls `is<iri>(s, iri::rfc3987{.resource = &arena})` on a non-ASCII IRI whose mapped URI exceeds 1,024 octets
- **THEN** the mapped copy is taken from `arena` and nothing from the global operator new

### Requirement: Strategies: date-time takes the leap-second policy
`date_time::rfc3339` SHALL have a `leap` member that its `time` part obeys, defaulting to `leap_seconds::allowed`; the suite's JSON Schema v1 `date-time` tests MUST run with `leap_seconds::refused`.

#### Scenario: Second 60 in a date-time
- **WHEN** `is<date_time>("1998-12-31T23:59:60Z", date_time::rfc3339{.leap = leap_seconds::refused})` is evaluated
- **THEN** the answer is `false`, and with the default strategy it is `true`

### Requirement: Strategies: a strategy gives its grammar rule
Every strategy webcpp ships for a format with a grammar rule (all but `ipv4`, `ipv6` and `regex`) SHALL have `rule()`, returning that rule configured as the strategy is, and for every configuration `c` and every string `s`, `is<F>(s, S{c})` MUST equal `boost::urls::grammar::parse(s, S{c}.rule()).has_value()`.

#### Scenario: A configured rule agrees with its strategy
- **WHEN** `time::rfc3339{.leap = leap_seconds::refused}.rule()` parses every time string of the unit tests
- **THEN** each verdict equals that of `is<time>` with the same strategy

### Requirement: Strategies: noexcept follows the strategy
`is` SHALL be `noexcept` exactly when its strategy's call operator is; every strategy webcpp ships MUST be `noexcept` but those of `iri` and `iri_reference`, which may throw `std::bad_alloc`, and `regex::via_boost_regex`.

#### Scenario: noexcept checked at compile time
- **WHEN** a test evaluates `noexcept(is<email>(s))`, `noexcept(is<iri>(s))` and `noexcept(is<uuid>(s, lambda))` with and without a `noexcept` lambda
- **THEN** they are `true`, `false`, and `true` only with the `noexcept` lambda

### Requirement: Strategies: Boost.Regex through a judge
`<webcpp/lexical/regex/boost_regex.hpp>` SHALL declare `template <class Judge> struct regex::via_boost_regex` (`using format = regex`, members `judge`, `resource`, `max_length`, `max_nesting`, with a deduction guide from the judge) and `struct boost_regex_pattern { std::u32string_view pattern; boost::regex_constants::syntax_option_type flags; std::string_view source; }`. Its call operator MUST answer `false` without reading a pattern longer than `max_length`, MUST answer `false` without calling the judge for a pattern that is not an ECMA-262 `Pattern[+UnicodeMode]`, and otherwise MUST return the judge's answer on the translation into Boost.Regex's Perl syntax, whose storage it owns until the judge returns.

#### Scenario: The judge never sees an invalid pattern
- **WHEN** `is<regex>(p, regex::via_boost_regex{counting_judge})` runs over every generated pattern `regex_v8` compares
- **THEN** the judge is called for exactly the patterns V8 accepts

#### Scenario: The judge receives the translation
- **WHEN** the judge is called for a pattern V8 accepts
- **THEN** it receives the pattern in Boost's Perl syntax over code points, the flags `perl`, `no_mod_m` and `no_mod_s` (and `icase` when the pattern's modifiers ask), and the source as written

### Requirement: Strategies: the ready judge and the traits
The same header SHALL declare `boost_regex_compiles`, a judge answering whether `boost::basic_regex<char32_t, ecma262_traits>` compiles the translation with the given flags and `no_except`, and `ecma262_traits`, the Boost.Regex traits class of ECMA-262's classes, properties and simple case folding over code points, built on lexical's own Unicode tables. A valid pattern Boost.Regex cannot run (a lookbehind of variable width) MUST be answered `false` by `boost_regex_compiles`, and the patterns of the generated set it so refuses MUST be listed, with their reason, in a committed file the test checks.

#### Scenario: Valid and runnable
- **WHEN** `is<regex>(p, regex::via_boost_regex{boost_regex_compiles{}})` runs over the generated patterns
- **THEN** it agrees with V8 on every pattern but those of the committed list, and on those it answers `false`

### Requirement: Strategies: Boost.Regex stays optional
Only `<webcpp/lexical/regex/boost_regex.hpp>` SHALL include `<boost/regex.hpp>`; `<webcpp/lexical.hpp>` and `<webcpp/lexical/regex.hpp>` MUST NOT, and `meta/include-boundaries.json` MUST keep its rule against `boost/regex` for every other header through its `except`. Only the tests and examples of that header name `/webcpp//boost_regex`.

#### Scenario: A program without Boost.Regex
- **WHEN** a program includes `<webcpp/lexical.hpp>` and `<webcpp/lexical/regex.hpp>` on a system without Boost.Regex's headers
- **THEN** it compiles and links

#### Scenario: The boundary refuses a second header
- **WHEN** a header other than `regex/boost_regex.hpp` includes `<boost/regex.hpp>`
- **THEN** the superproject's lint fails, naming the header and the boundary's reason

### Requirement: Strategies: the matching interface json_schema needs
The Boost.Regex header SHALL offer, for json_schema's regex policy, the compilation of an ECMA-262 pattern into `boost::basic_regex<char32_t, ecma262_traits>` as a value that says why it failed (`syntax` or `unsupported`), and a search of a compiled pattern over UTF-8 text with a step budget (by default 10,000,000 steps plus 64 per byte of text) answering `found`, `not_found` or `limit`, allocating only through the allocator or memory resource the caller gives (subject to open question O6).

#### Scenario: A catastrophic pattern stops at the budget
- **WHEN** `(a+)+$` is compiled and searched over 30 `a` followed by `b`
- **THEN** the search answers `limit`, within the budget, on every target

#### Scenario: The suite's regex tests
- **WHEN** every regex-dependent test of the JSON-Schema-Test-Suite runs through the compilation and the search
- **THEN** every one passes on native, wasip2, wasip3 and Emscripten
