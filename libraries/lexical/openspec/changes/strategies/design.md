## Context

lexical 0.1.0 (the change `genesis`) answers each format through a function of its own, in four
shapes. Reviewing that interface on 2026-10-13, the owner asked for one entry point whose
behaviour a developer chooses: a format, a strategy that prepares the arguments, and a functor
(a judge) that receives them, with webcpp's implementations as ready strategies and Boost.Regex
as one a developer may choose for regular expressions, receiving the pattern in the form
Boost.Regex takes. The owner decided the same day that lexical continues as a work-in-progress
specification: nothing of this change is built. `reference/design.md` is the full design, with
the API, the headers, the tests and the documentation; this document records its decisions.

## Goals / Non-Goals

**Goals:**

- One signature for every format, so that formats can be handled by templates and tables.
- Webcpp's implementations as ready strategies; any callable as a developer's own.
- Boost.Regex as an optional strategy, its judge receiving the translated pattern.
- The compile-and-search interface json_schema's regex policy needs.

**Non-Goals:**

- Changing any verdict of 0.1.0's default behaviour (`date_time`'s new option defaults to
  0.1.0's answer).
- Running regular expressions in `is<regex>`: the default strategy still only checks syntax.

## Decisions

### D1. One entry point, strategies and judges

`template <class Format, class Strategy = typename Format::default_strategy> bool
is(std::string_view, Strategy const& = {})`, constrained by `strategy_for<Strategy, Format>`:
any callable `bool(std::string_view)`, and, when it declares `using format`, one of that format
alone. Formats are tags nesting their strategies (`time::rfc3339{.leap = ...}`,
`iri::rfc3987{.resource = ...}`, `regex::ecma262{...}`); a strategy webcpp ships gives its
configured grammar rule through `rule()`, and `is<F>(s, S{c})` equals `parse(s,
S{c}.rule()).has_value()`. `noexcept` follows the strategy. Sources: Boost.Geometry's
strategies (a default per type, others passed in), Boost.Math's policies (one object, several
decisions), Boost.URL's rules (a rule carries its configuration).

### D2. `date_time` takes `leap_seconds`

A `date-time` holds a `time`, and 0.1.0's `date_time_rule_t` always allows second 60, so
JSON Schema v1's `date-time` cannot be checked. `date_time::rfc3339` gains `leap`, defaulting to
`allowed`, which changes no answer by default.

### D3. Boost.Regex as a strategy, through a judge

`regex::via_boost_regex<Judge>` refuses a pattern over its length bound or not ECMA-262 without
calling the judge, then translates it (the prototype's translator, rewritten to webcpp's rules
and to lexical's tables) and hands the judge a `boost_regex_pattern` (the pattern in Boost's
Perl syntax over code points, the flags, the source). `boost_regex_compiles` is the ready judge;
`ecma262_traits` is the Boost.Regex traits class of ECMA-262's classes over code points. Only
`<webcpp/lexical/regex/boost_regex.hpp>` includes Boost.Regex, behind the include boundary's
`except`.

### D4. The matching interface json_schema needs

json_schema's regex policy compiles a schema's `pattern` once and searches instances with a
bounded number of steps. The same header offers it, built on D3: compiling an ECMA-262 pattern
into `boost::basic_regex<char32_t, ecma262_traits>` with the translator's flags, as a value that
says why it failed (`syntax` or `unsupported`); and a search over UTF-8 text with a step budget
(the prototype's: 10,000,000 steps, plus 64 per byte of text) that answers `found`,
`not_found` or `limit`. Its allocation goes through a memory resource or an allocator the
caller gives.

## Risks / Trade-offs

- [Two APIs during the move to `is<F>`] → O1 decides; until then 0.1.0's functions are the
  oracle of the new entry point's tests.
- [Boost.Regex cannot run every valid pattern] → the translator marks `unsupported`; the ready
  judge answers `false` for those, listed in a committed file, so the difference is known.
- [The translator allocates] → `via_boost_regex` is never `noexcept` and writes through the
  resource it is given.

## Migration Plan

Mechanical, listed in `reference/design.md` section 8: `is_time(s, leap)` becomes
`is<time>(s, time::rfc3339{.leap = leap})`, and so on for every function, in one version with
its changelog lines. The Boost.Regex header is new and changes nothing for a program that does
not include it.

## Open Questions

- **O1.** Keep the `is_x` functions as one-line forms of `is<F>`, or remove them?
  Recommended: remove them in the version that adds `is<F>`.
- **O2.** Composition (a strategy taking the strategies of its parts, `email` its
  `hostname`'s) in the same version, or later? Recommended: later.
- **O3.** Boost.Regex as an optional dependency of one header: needs a recorded decision.
- **O4.** Format tags in `webcpp::lexical` directly, or in a namespace of their own;
  strategies named by their standard (`rfc3339`, `ecma262`)?
- **O5.** `ecma262_traits` shipped by lexical (recommended), or left to the developer?
- **O6.** The matching interface (D4) in lexical, as json_schema's proposal assumes, or in
  json_schema itself?
