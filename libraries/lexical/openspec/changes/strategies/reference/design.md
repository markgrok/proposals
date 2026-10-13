---
title: "webcpp::lexical: one entry point, strategies and judges"
tags: [webcpp, lexical, design, spec, strategies, boost-regex]
status: draft
created: 2026-10-13
---

# webcpp::lexical: one entry point, strategies and judges

A design proposal for the next version of `webcpp::lexical` (published as
0.1.0 on 2026-10-13, formerly `webcpp::formats`). It is written for the
owner's review; nothing here is built yet. Decisions the owner has not taken
are listed in section 12 and marked "open" where they appear.

## 1. What the owner asked for

In the owner's words (2026-10-13, translated):

- Every function of the library has the same signature, so that everything
  can be done through templates: `bool is<rule>(std::string_view s, functor)`.
- The rule decides with which parameters it instantiates the functor.
- The library ships a list of helper functors, so that a developer can use
  webcpp's own implementations, or choose another.
- `is_regex` receives a pattern written in ECMA-262. A developer who chooses
  the Boost.Regex strategy has the functor receive the pattern in the form
  Boost.Regex takes, so that the developer calls Boost.Regex themselves.
- The functor is the function that receives the `string_view` to validate
  together with every parameter it needs, which come from the strategy.

Today the 22 functions have four shapes: `is_x(s)` for 17 of them,
`is_time(s, leap_seconds)`, `is_relative_json_pointer(s, index_manipulation)`,
and `is_iri(s)` / `is_iri(s, memory_resource&)` with the same pair for
`is_iri_reference`. `is_regex` imposes webcpp's own validator.

## 2. Vocabulary

| Term | What it is | Who writes it |
| --- | --- | --- |
| **format** | A tag type naming one string format (`lexical::time`, `lexical::regex`). It fixes the input's language (for `regex`, an ECMA-262 `Pattern` in `u` mode) and names its default strategy. | webcpp |
| **strategy** | A callable `bool(std::string_view) const`, the whole answer for one format. It holds its configuration as data members (`leap`, `resource`), and may prepare arguments and hand them to a judge. | webcpp ships one or more per format; a developer may write their own |
| **judge** | The callable a strategy hands its prepared arguments to: the input, transformed or not, and every parameter the strategy computed. Its signature is set by the strategy that calls it. | the developer, or webcpp's ready judges |

A strategy that needs no judge is complete by itself (webcpp's own
implementations). A strategy that takes a judge is an adapter: it turns the
format's input into what the judge's engine reads, and lets the developer
decide with that engine (section 6, Boost.Regex).

## 3. The entry point

```cpp
namespace webcpp::lexical {

/** Whether `s` is an instance of `Format`, as `strategy` judges it. */
template <class Format, class Strategy = typename Format::default_strategy>
    requires strategy_for<Strategy, Format>
[[nodiscard]] constexpr bool is(std::string_view s, Strategy const& strategy = Strategy{})
    noexcept(std::is_nothrow_invocable_v<Strategy const&, std::string_view>)
{
    return std::invoke(strategy, s);
}

}  // namespace webcpp::lexical
```

Every format is called the same way:

```cpp
lexical::is<lexical::email>(s);                                       // webcpp's default
lexical::is<lexical::time>(s, lexical::time::rfc3339{.leap = lexical::leap_seconds::refused});
lexical::is<lexical::iri>(s, lexical::iri::rfc3987{.resource = &arena});
lexical::is<lexical::regex>(s, lexical::regex::via_boost_regex{my_judge});
lexical::is<lexical::uuid>(s, [](std::string_view text) { return my_uuid_check(text); });
```

`is` adds nothing to the strategy's answer: it is the one signature, the
default, and the check that a strategy belongs to the format. Its `noexcept`
is the strategy's. It is `constexpr` so that a strategy that is constexpr
stays so; webcpp's strategies on Boost.URL are not.

### 3.1 The concept

```cpp
template <class Strategy, class Format>
concept strategy_for =
    std::predicate<Strategy const&, std::string_view> &&
    (!requires { typename Strategy::format; } ||
     std::same_as<typename Strategy::format, Format>);
```

- Any callable `bool(std::string_view)` is a strategy for any format: a
  lambda, a function pointer, a developer's own class. The developer takes
  the whole answer in their hands.
- A strategy that declares `using format = F;`, which every strategy webcpp
  ships does, is a strategy for `F` alone. `is<lexical::email>(s,
  lexical::time::rfc3339{})` does not compile, with a message that names the
  two formats.

### 3.2 Tables of formats

With one signature, a program builds a table of formats with no wrapper per
format, which a JSON Schema validator's `format` keyword needs:

```cpp
using check = bool (*)(std::string_view);
constexpr std::array<std::pair<std::string_view, check>, 3> checks{{
    {"date-time", [](std::string_view s) { return lexical::is<lexical::date_time>(s); }},
    {"email", [](std::string_view s) { return lexical::is<lexical::email>(s); }},
    {"regex", [](std::string_view s) { return lexical::is<lexical::regex>(s); }},
}};
```

## 4. Formats and the strategies webcpp ships

Each format is an empty struct in `webcpp::lexical` that nests its
strategies and names its default (naming: open question O4). Every strategy
webcpp ships is an aggregate, so its configuration is set by designated
initializers, and declares `using format`.

| Format tag | Default strategy | Its configuration | Other strategies | `noexcept` |
| --- | --- | --- | --- | --- |
| `date_time` | `date_time::rfc3339` | `leap_seconds leap = allowed` | | yes |
| `date` | `date::rfc3339` | | | yes |
| `time` | `time::rfc3339` | `leap_seconds leap = allowed` | | yes |
| `duration` | `duration::rfc3339` | | | yes |
| `email` | `email::rfc5321` | | | yes |
| `idn_email` | `idn_email::rfc6531` | | | yes |
| `hostname` | `hostname::rfc1123` | | | yes |
| `idn_hostname` | `idn_hostname::idna2008` | | | yes |
| `ipv4` | `ipv4::rfc2673` | | | yes |
| `ipv6` | `ipv6::rfc4291` | | | yes |
| `uri` | `uri::rfc3986` | | | yes |
| `uri_reference` | `uri_reference::rfc3986` | | | yes |
| `iri` | `iri::rfc3987` | `std::pmr::memory_resource* resource = nullptr` (the default resource) | | no: `std::bad_alloc` beyond 1,024 mapped octets |
| `iri_reference` | `iri_reference::rfc3987` | as `iri` | | no |
| `uri_template` | `uri_template::rfc6570` | | | yes |
| `json_pointer` | `json_pointer::rfc6901` | | | yes |
| `relative_json_pointer` | `relative_json_pointer::draft` | `index_manipulation manipulation = allowed` | | yes |
| `uuid` | `uuid::rfc4122` | | | yes |
| `regex` | `regex::ecma262` | `std::size_t max_length = 65536`, `std::size_t max_nesting = 32` | `regex::via_boost_regex<Judge>` (section 6) | `ecma262` yes; `via_boost_regex` as section 6.4 says |

`date_time::rfc3339` gains the `leap` member that `time::rfc3339` has today
alone: a `date-time` holds a `time`, and today `is_date_time` cannot refuse
second 60 for JSON Schema v1. This is a correction the uniform design makes
visible, and it is tested (section 10).

The strategy of today's default behaviour is the default strategy of its
format, so `is<F>(s)` gives, for every `s`, the answer `is_f(s)` gives in
0.1.0. The one exception is the correction above, which keeps `allowed` as
its default and so changes no answer by default.

### 4.1 A strategy and its grammar rule

Every strategy webcpp ships for a format that has a Boost.URL rule (all but
`ipv4`, `ipv6` and `regex`, whose rules are Boost.URL's own or none) gives
that rule, configured as the strategy is:

```cpp
struct time {
    struct rfc3339 {
        using format = time;
        leap_seconds leap = leap_seconds::allowed;

        [[nodiscard]] constexpr grammar::time_rule_t rule() const noexcept { return {leap}; }
        [[nodiscard]] bool operator()(std::string_view s) const noexcept;
    };
    using default_strategy = rfc3339;
};
```

The guarantee of 0.1.0's page, "for every format but `regex`, `is_x(s)` and
`parse(s, x_rule)` give the same answer", becomes: for every strategy `S`
webcpp ships that has `rule()`, and every configuration of it,
`is<F>(s, S{...})` equals `parse(s, S{...}.rule()).has_value()`. A
developer's own strategy carries no such guarantee.

## 5. A developer's own strategy

The whole answer, for any format:

```cpp
// A company's stricter e-mail rule: webcpp's, and no IP literal.
struct no_ip_literal {
    using format = lexical::email;   // optional: binds it to email alone
    bool operator()(std::string_view s) const noexcept {
        return lexical::is<lexical::email>(s) && s.find('[') == std::string_view::npos;
    }
};
lexical::is<lexical::email>(s, no_ip_literal{});
```

A developer's strategy may call webcpp's (as above), wrap it, or replace it.
The documentation states that `is` then answers what the developer's
strategy answers, and no guarantee of the page applies to it.

## 6. Regular expressions on Boost.Regex

### 6.1 The question the strategy answers

`lexical::regex`'s input is always an ECMA-262 `Pattern[+UnicodeMode]`
(JSON Schema's `regex`). Two questions can be asked of it:

1. Is it a valid ECMA-262 pattern? This is a question of syntax and early
   errors, which `regex::ecma262` answers with webcpp's own validator,
   allocating nothing.
2. Is it valid, and can my engine run it? A developer who will run the
   patterns with Boost.Regex wants to refuse, at validation time, a valid
   pattern Boost.Regex cannot run (a lookbehind of variable width), and to
   compile with the flags and the syntax their engine will use.

`regex::via_boost_regex<Judge>` answers the second: it translates the
pattern into Boost.Regex's Perl syntax with ECMA-262's semantics, and hands
the translation to the developer's judge.

### 6.2 What the judge receives

```cpp
// <webcpp/lexical/regex/boost_regex.hpp>
namespace webcpp::lexical {

/** An ECMA-262 pattern translated for Boost.Regex: what a judge of via_boost_regex receives. */
struct boost_regex_pattern {
    /** The pattern in Boost's Perl syntax, over code points, with ECMA-262's semantics. */
    std::u32string_view pattern;
    /** The flags Boost.Regex must compile it with: perl, no_mod_m, no_mod_s, and icase when the
        pattern's modifiers ask for it. */
    boost::regex_constants::syntax_option_type flags;
    /** The pattern as it was written, in ECMA-262. */
    std::string_view source;
};

}  // namespace webcpp::lexical
```

The judge is any callable `bool(boost_regex_pattern const&)`. The
translation's storage belongs to the strategy and lives until the judge
returns; a judge that keeps the pattern copies it.

### 6.3 The strategy

```cpp
namespace webcpp::lexical {

template <class Judge>
struct regex::via_boost_regex {
    using format = regex;
    Judge judge;
    /** Where the translation is written; the default resource when null. */
    std::pmr::memory_resource* resource = nullptr;
    std::size_t max_length = max_regex_length;
    std::size_t max_nesting = max_regex_nesting;

    [[nodiscard]] bool operator()(std::string_view pattern) const;
};

template <class Judge>
regex::via_boost_regex(Judge) -> regex::via_boost_regex<Judge>;

}  // namespace webcpp::lexical
```

What `operator()` does, in order:

1. A pattern longer than `max_length` is answered `false` without being
   read, as `ecma262` does.
2. The pattern is translated. A pattern that is not an ECMA-262 pattern
   (the translator's `syntax`) is answered `false`, and the judge is not
   called: a judge only ever sees a valid ECMA-262 pattern.
3. Otherwise the judge is called with the translation, and its answer is
   the strategy's. A pattern the translator marks `unsupported` (valid, but
   beyond what Boost.Regex can run) is handed to the judge too, since only
   the judge's engine knows; the ready judge below answers `false` for it.

The translator is the one the feasibility prototype proved
(`RESEARCH/BOOST_REGEX_ECMASCRIPT_FEASIBILITY.md`: identical to V8 on 20,130
fuzzed patterns, every regex test of the JSON Schema Test Suite passing on
all five targets), rewritten to webcpp's rules: no `std::string` reason
carried, its output written through `resource`, its Unicode tables those
`detail/regex_tables.hpp` already generates for `ecma262`.

### 6.4 The ready judge, and the engine's traits

```cpp
namespace webcpp::lexical {

/** Whether Boost.Regex compiles the translation with ECMA-262's character classes: the pattern
    is valid and Boost.Regex can run it. */
struct boost_regex_compiles {
    [[nodiscard]] bool operator()(boost_regex_pattern const& p) const;
};

/** The Boost.Regex traits of ECMA-262 over code points: \p{...}, \w, \s, \d and case folding as
    ECMA-262 defines them, from webcpp's Unicode tables. A developer who runs the patterns uses
    boost::basic_regex<char32_t, ecma262_traits>. */
class ecma262_traits;

}  // namespace webcpp::lexical
```

`lexical::is<lexical::regex>(s, lexical::regex::via_boost_regex{lexical::boost_regex_compiles{}})`
answers "valid, and Boost.Regex runs it". A developer who keeps their own
compiled patterns writes their own judge, compiles with
`basic_regex<char32_t, ecma262_traits>` and the flags received, and
stores the result.

`noexcept`: `via_boost_regex` is never `noexcept`, since the translation
allocates. Boost.Regex is used with `no_except`, so `boost_regex_compiles`
reads `status()` and throws nothing of its own; an allocation that fails
throws `std::bad_alloc`, or terminates in a build without exceptions, as
every webcpp allocation does.

### 6.5 Boost.Regex stays optional

- Only `<webcpp/lexical/regex/boost_regex.hpp>` includes `<boost/regex.hpp>`.
  `<webcpp/lexical.hpp>` and `<webcpp/lexical/regex.hpp>` do not, and a
  program that never includes the new header neither needs nor links
  Boost.Regex.
- `meta/include-boundaries.json` keeps its rule "no `boost/regex`" for every
  header but that one, by its `except`.
- Boost.Regex 1.92 is header-only. The build gains `/webcpp//boost_regex`
  for the header's tests and examples alone. Adding the dependency needs the
  owner's decision (open question O3).

## 7. Composition (staged)

Some formats contain others: an `email`'s domain is a `hostname` or an
address literal, an `idn_hostname` is a sequence of labels judged by
`webcpp::idna`, a `date_time` holds a `date` and a `time`. A later step may
let a strategy take the strategies of its parts, as Boost.Math's policies
travel through the functions they configure:

```cpp
lexical::is<lexical::email>(s, lexical::email::rfc5321{.domain = my_hostname_strategy});
```

This document does not specify it: whether composition comes in the same
version is open question O2. The first step's strategies take no part
strategies, and nothing in section 3 or 4 prevents adding them as members
with defaults later, which changes no existing call.

## 8. What changes from 0.1.0

| 0.1.0 | This design |
| --- | --- |
| `is_date_time(s)` | `is<date_time>(s)` |
| `is_time(s, leap_seconds::refused)` | `is<time>(s, time::rfc3339{.leap = leap_seconds::refused})` |
| `is_relative_json_pointer(s, index_manipulation::refused)` | `is<relative_json_pointer>(s, relative_json_pointer::draft{.manipulation = index_manipulation::refused})` |
| `is_iri(s, resource)` | `is<iri>(s, iri::rfc3987{.resource = &resource})` |
| `is_regex(s)` | `is<regex>(s)` |
| (none) | `is<regex>(s, regex::via_boost_regex{judge})` |
| (none) | `is<F>(s, any_callable)` for every format |
| `grammar::time_rule_t{leap}` | unchanged, and also `time::rfc3339{leap}.rule()` |

Whether the 22 `is_x` functions stay as one-line forms of `is<F>`, or leave,
is open question O1. The enums `leap_seconds` and `index_manipulation`, the
constants `max_regex_length` and `max_regex_nesting`, the grammar rules and
`grammar::text` / `grammar::result` stay as they are.

## 9. Headers

| Header | Declares |
| --- | --- |
| `<webcpp/lexical/is.hpp>` | `is`, `strategy_for` |
| each format's header (`date_time.hpp`, `email.hpp`, ...) | its tag and strategies, beside its rule; it includes `is.hpp` |
| `<webcpp/lexical.hpp>` | every header but the two regex ones, as today |
| `<webcpp/lexical/regex.hpp>` | `regex`, `regex::ecma262`, the constants |
| `<webcpp/lexical/regex/boost_regex.hpp>` | `regex::via_boost_regex`, `boost_regex_pattern`, `boost_regex_compiles`, `ecma262_traits`; the one header that includes Boost.Regex |

## 10. Tests

Tests first, each red before its code, under the library's own rules.

- **Default equivalence.** For every format, every string of the existing
  tests and of the JSON Schema Test Suite: `is<F>(s)` equals 0.1.0's
  `is_f(s)`, kept as the oracle while the two exist.
- **Rule equivalence.** For every shipped strategy with `rule()`, over the
  same strings and each configuration value: `is<F>(s, S{c})` equals
  `parse(s, S{c}.rule()).has_value()`.
- **`date_time` leap seconds.** `1998-12-31T23:59:60Z` is a `date-time` with
  `leap = allowed` and is not with `refused`; JSON Schema v1's `date-time`
  tests run with `refused`.
- **The concept.** Compile-fail tests (`webcpp.compile-fail`): a strategy of
  another format; a callable that does not take a `string_view`; one that
  returns no `bool`.
- **`noexcept` follows the strategy.** `static_assert`s: `is<email>` is
  `noexcept`, `is<iri>` is not, `is<F>` with a `noexcept` lambda is, without
  is not.
- **A developer's strategy.** A lambda and a class with `using format`, for
  a format with a rule and for `regex`.
- **Boost.Regex.** Over the generated patterns `regex_v8` already compares:
  a judge that counts its calls is never called on a pattern V8 refuses; it
  receives, for every pattern V8 accepts, the translation the prototype
  writes; `boost_regex_compiles` answers what V8 answers on every pattern
  but those the translator marks `unsupported`, which are listed in a
  committed file with the reason, and checked to be exactly those.
- **Allocation.** The existing allocation test, through `is<F>`: every
  default strategy allocates nothing but the IRI ones beyond 1,024 octets;
  `via_boost_regex` allocates only through `resource` (a counting resource
  sees every byte, the global operator new none outside Boost.Regex's own
  compile in `boost_regex_compiles`, which is measured and stated).
- **Targets.** Everything on native, wasm32-wasip2, wasm32-wasip3 and
  emscripten, with and without exceptions, as today; MSVC and Apple Clang on
  CI.

## 11. Documentation

- The page gains a chapter "Strategies": the vocabulary of section 2, the
  entry point, the table of section 4, writing one's own strategy, and
  Boost.Regex with a complete example (validate with `via_boost_regex`, then
  compile and run with `ecma262_traits`).
- The migration table of section 8 goes to the changelog as one line per
  change, in the library's changelog form.
- The reference lists every tag, strategy and judge with Doc Comments.

## 12. Open questions for the owner

- **O1. The `is_x` functions.** Keep them as one-line forms of `is<F>`, or
  remove them? Recommendation: remove them in the version that adds `is<F>`,
  since 0.1.0 has just been published and the owner's goal is one
  signature; two spellings of every check would be the opposite.
- **O2. Composition (section 7)** in the same version, or after?
  Recommendation: after, as nothing here prevents it.
- **O3. Boost.Regex as an optional dependency** of
  `<webcpp/lexical/regex/boost_regex.hpp>` (a pin already exists, as Boost
  1.92 ships it). It needs a recorded decision.
- **O4. Names.** Tags directly in `webcpp::lexical` (`lexical::time`,
  `lexical::regex`), as above, or in a namespace of their own
  (`lexical::format::time`)? And strategies named by their standard
  (`time::rfc3339`, `regex::ecma262`), as above?
- **O5. `ecma262_traits` in lexical.** It is what makes Boost.Regex run the
  translation with ECMA-262's classes; without it a judge can only compile
  patterns that use no `\p{...}`. Ship it in lexical (recommended), or leave
  running patterns to the developer?

## 13. Where the design comes from

- **Boost.Geometry's strategies**: `bg::distance(a, b)` with a default
  strategy chosen for the geometry types, `bg::distance(a, b,
  strategy::haversine{})` with another; the library ships strategies and a
  developer may write their own. `is<F>(s, strategy)` follows it.
- **Boost.Math's policies**: one object carries several decisions and each
  function reads those it needs; the model for composition (section 7).
- **Boost.URL's rules**: a rule is an object that carries its
  configuration (`time_rule_t{leap}`), which strategies keep through
  `rule()`.
- **`std::invoke` and `std::predicate`**: a strategy is anything invocable
  as a predicate on the input.
