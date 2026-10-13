## Context

webcpp::lexical began on 2026-10-09 as webcpp::formats, specified (`reference/spec.md`) and
planned task by task (`reference/plan.md`) as the format layer of webcpp::json_schema. Its
first regex format stood on SRELL and webcpp::regex; when the owner abandoned both on
2026-10-12, `is_regex` became a syntax validator of the library's own, which allocates
nothing. The library was renamed lexical on 2026-10-13, since "formats" read as Boost.Format or
`std::format` to a C++ developer, and published as 0.1.0 the same day.

This document records, retroactively, the decisions 0.1.0 is built on. What the owner asked for
next (one entry point, strategies and judges, Boost.Regex as an optional strategy) is the change
`strategies`.

## Goals / Non-Goals

**Goals:**

- Answer, for each of the 19 string formats JSON Schema names, whether a string is a
  well-formed instance, exactly as its RFC or ECMA-262 defines it, proven by the official
  JSON-Schema-Test-Suite and, for `regex`, by V8.
- Allocate nothing (the IRIs beyond 1,024 mapped octets aside, through a memory resource the
  program gives), raise nothing of its own, run natively and on wasip2, wasip3 and Emscripten.
- Compose with Boost.URL: every format but `regex` is a grammar rule a program can use inside
  its own parsers.

**Non-Goals:**

- Anything beyond syntax: no DNS lookup, no mailbox check, no URI resolution, no date
  arithmetic beyond the calendar's days, no regular expression ever run by `is_regex`.
- Parsing into values (a date into fields, a URI into components) beyond what the grammar
  rules return; Boost.URL's `url_view` is returned by the URI rules as Boost.URL gives it.
- The formats JSON Schema does not name.

## Decisions

### D1. A library of webcpp's own, checking the lexical space only

lexical ports nothing: its evidence is the RFCs it cites, its unit tests, the official suite run
directly against its validators, and V8 for `regex`. It answers what XML Schema calls the
lexical space of a format: whether a string is one of its literals. It never resolves, looks
up, compiles or runs.

### D2. A format is a Boost.URL rule and a function

Every format but `regex` is a rule in `webcpp::lexical::grammar` following Boost.URL's Rule
concept (`value_type`, `parse(char const*&, char const*) const`), and a validator `is_<name>`
that parses the whole string with it. The two agree on every input, which the unit tests
check. A rule that fails leaves its iterator where it was. `ipv4` and `ipv6` use Boost.URL's own
rules. Alternative rejected: validators of their own, beside Boost.URL, which a program could
not compose with its parsers.

### D3. Boost.URL is never modified; lexical guards it

Where Boost.URL 1.92 is more liberal than RFC 3986, lexical refuses what it wrongly accepts in
its own `uri_rule` and `uri_reference_rule`: a relative reference whose path after an authority
is neither empty nor led by `/`; `[` and `]` in a query; `#` in a fragment; an RFC 6874 zone
identifier, which RFC 9844 withdrew. Where Boost.URL is stricter (an `IPvFuture` led by `V`),
lexical keeps its verdict, since it never loosens Boost.URL. Each guard has a test that fails
the day the configured Boost changes, so a guard leaves in a change of its own.

### D4. IRIs are mapped to URIs

Boost.URL parses RFC 3986 only. lexical checks what RFC 3987 adds (the non-ASCII code points
and where they may be), maps each of their octets to `%XX` and lets Boost.URL judge the
structure. The mapped copy is written into a 1,024-octet inline buffer; beyond it, into memory
from a `std::pmr::memory_resource` the program gives (the default resource otherwise), the one
allocation of the library. An ASCII IRI is never copied.

### D5. Internationalized names through webcpp::idna's public interface

A host name's A-label is lowercased and judged by `idna::check_label`, one label at a time;
`idn_hostname_rule` and `idna_label_rule` hand their run to `idna::to_ascii` into a stack
buffer of `idna::max_domain_length` octets. A label's run stops at `.`, U+3002, U+FF0E and
U+FF61. An internationalized Mailbox's lengths are measured as written, in UTF-8. lexical never
works around an idna verdict: a wrong one becomes a red test in idna.

### D6. Regular expressions: a syntax validator of lexical's own

`is_regex` is a recursive-descent reader of well-formed UTF-8 that answers whether a pattern is
an ECMA-262 `Pattern[+UnicodeMode]`, early errors included, with ECMAScript 2025's modifiers and
duplicate group names, over Unicode 17.0.0 tables generated from pinned files. It allocates
nothing. A pattern longer than `max_regex_length` (65,536 bytes) or nested deeper than
`max_regex_nesting` (32) is answered `false` without being judged, which keeps a hostile
pattern within a WebAssembly program's stack. V8's verdicts on generated patterns are the
oracle. Alternatives rejected: SRELL and webcpp::regex (abandoned by the owner on 2026-10-12);
Boost.Regex as the validator (it allocates outside any memory resource, and its syntax is not
ECMA-262's).

### D7. Errors and exceptions

A validator answers `bool`; a rule returns `grammar::result<T>` (`boost::system::result`) with
Boost.URL's grammar errors, `error::mismatch` when nothing of the format starts and
`error::invalid` when it started and broke a constraint. lexical raises nothing of its own:
only an IRI function's allocation may throw. `WEBCPP_LEXICAL_NO_EXCEPTIONS` exists for symmetry
with the other libraries and changes nothing in lexical.

### D8. Tests

The official suite runs against the functions (no copy of its MIT data is committed);
`test/known_failures/<dialect>.json` lists the failures, each with its reason, and the runner
fails on an unlisted failure and on a listed test that passes. Every rule agrees with its
function. The regex tables and patterns are generated and checked by the own lane `generated`;
V8's verdicts are written by the own lane `oracle` alone, with the CI's Node. Every test runs on
native, wasip2, wasip3 and Emscripten.

## Risks / Trade-offs

- [Boost.URL changes a verdict] → each guard and kept deviation has a test that fails the day
  the configured Boost changes it (D3).
- [The regex validator's recursion overflows a small stack] → `max_regex_nesting`; the stack
  needed at the bound on wasip2 at `-O0` is to be measured again for the current validator
  (the change `strategies`, task 0.3).
- [A Node with another V8 writes other verdicts] → verdicts are written only by the oracle lane,
  with the CI's pinned Node.

## Migration Plan

None: a new library.

## Open Questions

None for 0.1.0. The open questions of what follows are the change `strategies`'s.
